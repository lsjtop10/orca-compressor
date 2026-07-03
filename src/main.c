#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "errors.h"
#include "file.h"
#include "utils.h"
#include "huffman.h"
#include "buffered-stream.h"
#include "encoder.h"
#include "decoder.h"
#include "serializer.h"

typedef enum { MODE_NONE, MODE_COMPRESS, MODE_DECOMPRESS } ProgramMode;

typedef struct {
    ProgramMode mode;
    const char* inputPath;
    const char* outputPath;
} CliOptions;

bool parse_cli_options(int argc, char* argv[], CliOptions* outOpt, ErrorContext* err) {
    outOpt->mode = MODE_NONE;
    outOpt->inputPath = NULL;
    outOpt->outputPath = NULL;

    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];

        if (arg[0] == '-') {
            char flag = '\0';
            if (strcmp(arg, "-c") == 0 || strcmp(arg, "--compress") == 0)
                flag = 'c';
            if (strcmp(arg, "-d") == 0 || strcmp(arg, "--decompress") == 0)
                flag = 'd';
            if (strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0)
                flag = 'o';

            switch (flag) {
            case 'c':
                if (outOpt->mode != MODE_NONE) {
                    append_ErrorContext(err, HF_ERR_INVALID_ARG,
                                        "parse options: operation mode specified multiple times");
                    return false;
                }
                outOpt->mode = MODE_COMPRESS;
                break;

            case 'd':
                if (outOpt->mode != MODE_NONE) {
                    append_ErrorContext(err, HF_ERR_INVALID_ARG,
                                        "parse options: operation mode specified multiple times");
                    return false;
                }
                outOpt->mode = MODE_DECOMPRESS;
                break;

            case 'o':
                if (outOpt->outputPath != NULL) {
                    append_ErrorContext(err, HF_ERR_INVALID_ARG,
                                        "parse options: output path specified multiple times");
                    return false;
                }
                if (i + 1 >= argc) {
                    append_ErrorContext(err, HF_ERR_INVALID_ARG,
                                        "parse options: option -o/--output requires an argument");
                    return false;
                }
                outOpt->outputPath = argv[++i];
                break;

            default:
                append_ErrorContext(err, HF_ERR_INVALID_ARG,
                                    createFormattedString("parse options: unknown option '%s'", arg));
                return false;
            }
        }
        else {
            if (outOpt->inputPath != NULL) {
                append_ErrorContext(
                    err, HF_ERR_INVALID_ARG,
                    createFormattedString("parse options: multiple input files specified ('%s' and '%s')",
                                          outOpt->inputPath, arg));
                return false;
            }
            outOpt->inputPath = arg;
        }
    }

    if (outOpt->mode == MODE_NONE) {
        append_ErrorContext(err, HF_ERR_INVALID_ARG,
                            "parse options: either -c (compress) or -d (decompress) must be specified");
        return false;
    }
    if (outOpt->inputPath == NULL) {
        append_ErrorContext(err, HF_ERR_INVALID_ARG, "parse options: input file path is required");
        return false;
    }

    return true;
}

static void run_compress(PlatformFileCtx* srcFile, PlatformFileCtx* dstFile, ErrorContext* err) {
    if (peekSurfaceError_ErrorContext(err) != NULL) return;

    BufferedInputStream* bis = create_BufferedInputStream(srcFile, fetch_PlatformFileCtx);
    BufferedOutputStream* bos = create_BufferedOutputStream(dstFile, flush_PlatformFileCtx);

    // 1. 빈도수 계산
    FreqTable freqTable;
    init_FreqTable(&freqTable);
    countFreqFromStream_StreamEncoder(bis, &freqTable, err);
    if (peekSurfaceError_ErrorContext(err) != NULL) goto cleanup;

    // 2. 입력 파일 리셋
    if (!trySeek_PlatformFileCtx(srcFile, 0, err)) {
        append_ErrorContext(err, HF_ERR_FILE_SEEK_FAILED, "compress: failed to seek src file to start");
        goto cleanup;
    }
    reset_BufferedInputStream(bis, srcFile, fetch_PlatformFileCtx);

    // 3. 트리 구축 및 코드 테이블 빌드
    HuffmanTreeNode* root = build_HuffmanTree(&freqTable);
    if (root == NULL) {
        append_ErrorContext(err, HF_ERR_UNKOWN, "compress: failed to build Huffman tree");
        goto cleanup;
    }

    HuffmanCodeTable codeTable;
    init_HuffmanCodeTable(&codeTable);
    HuffmanCode code;
    memset(&code, 0, sizeof(HuffmanCode));
    buildTable_HuffmanCodeTable(&codeTable, root, code, 0);

    // 4. 메타데이터 생성 및 저장
    FileMetaV1* meta = fillFileMeta_FileMetaV1(srcFile, err);
    if (meta == NULL) {
        destroy_HuffmanTreeNode(root);
        goto cleanup;
    }
    writeFileMeta_Serializer(bos, meta, err);

    if (peekSurfaceError_ErrorContext(err) != NULL) {
        destroy_HuffmanTreeNode(root);
        goto cleanup;
    }

    // 5. 청크 헤더 영역 예약
    uint64_t chunkHeaderPos = totalWritedSize_BufferedOutputStream(bos);
    ChunkInfoV1 chunk;
    memset(&chunk, 0, sizeof(ChunkInfoV1));
    writeChunkInfo_Serializer(bos, &chunk, err);
    if (peekSurfaceError_ErrorContext(err) != NULL) {
        destroy_HuffmanTreeNode(root);
        goto cleanup;
    }

    // 6. 트리 직렬화
    uint64_t treeStartPos = totalWritedSize_BufferedOutputStream(bos);
    serializeTree_Serializer(bos, root, err);
    uint64_t treeEndPos = totalWritedSize_BufferedOutputStream(bos);
    uint32_t treeSize = (uint32_t)(treeEndPos - treeStartPos);
    if (peekSurfaceError_ErrorContext(err) != NULL) {
        destroy_HuffmanTreeNode(root);
        goto cleanup;
    }

    // 7. 데이터 인코딩
    uint64_t dataStartPos = treeEndPos;
    StreamEncoder encoder;
    encoder.bis = bis;
    encoder.bos = bos;
    encodeStream_StreamEncoder(&encoder, &codeTable, err);
    uint64_t dataEndPos = totalWritedSize_BufferedOutputStream(bos);
    uint64_t compressedDataSize = dataEndPos - dataStartPos;
    if (peekSurfaceError_ErrorContext(err) != NULL) {
        destroy_HuffmanTreeNode(root);
        goto cleanup;
    }

    // 8. 청크 헤더 채워 쓰기
    chunk.version = CURRENT_ARCHIVE_FORMAT_VERSION;
    chunk.size = compressedDataSize;
    chunk.originalSize = getFileSize_PlatformFileCtx(srcFile, err);
    chunk.treeSize = treeSize;

    if (!trySeek_PlatformFileCtx(dstFile, chunkHeaderPos, err)) {
        append_ErrorContext(err, HF_ERR_FILE_SEEK_FAILED, "compress: failed to seek dst file to chunk header position");
        destroy_HuffmanTreeNode(root);
        goto cleanup;
    }
    //
    reset_BufferedOutputStream(bos, dstFile, flush_PlatformFileCtx);

    writeChunkInfo_Serializer(bos, &chunk, err);
    flush_BufferedOutputStream(bos, err);

    destroy_HuffmanTreeNode(root);

cleanup:
    destroyFileMeta_FileMetaV1(meta);
    destroy_BufferedInputStream(bis);
    destroy_BufferedOutputStream(bos);
}

static void run_decompress(PlatformFileCtx* srcFile, PlatformFileCtx* dstFile, ErrorContext* err) {
    if (peekSurfaceError_ErrorContext(err) != NULL) return;

    BufferedInputStream* bis = create_BufferedInputStream(srcFile, fetch_PlatformFileCtx);
    BufferedOutputStream* bos = create_BufferedOutputStream(dstFile, flush_PlatformFileCtx);

    // 1. 메타데이터 역직렬화
    FileMetaV1* meta = readAndCreateFileMeta_Serializer(bis, err);
    if (meta == NULL) goto cleanup;
    destroyFileMeta_FileMetaV1(meta);

    // 2. 청크 헤더 역직렬화
    ChunkInfoV1 chunk;
    readChunkInfo_Serializer(bis, &chunk, err);
    if (peekSurfaceError_ErrorContext(err) != NULL) goto cleanup;

    // 3. 트리 역직렬화
    HuffmanTreeNode* root = deserializeTree_Serializer(bis, chunk.treeSize, err);
    if (root == NULL) {
        append_ErrorContext(err, HF_ERR_UNKOWN, "decompress: failed to deserialize Huffman tree");
        goto cleanup;
    }

    // 4. 읽기 제한(limit) 설정
    uint64_t currentPos = totalReadSize_BufferedInputStream(bis);
    setLimit_BufferedInputStream(bis, currentPos + chunk.size);

    // 5. 데이터 디코딩
    StreamDecoder decoder;
    decoder.bis = bis;
    decoder.bos = bos;
    decoder.numOriginalBytes = chunk.originalSize;
    decodeStream_StreamDecoder(&decoder, root, err);

    // 6. 제한 해제
    clearLimit_BufferedInputStream(bis);

    destroy_HuffmanTreeNode(root);

cleanup:
    destroy_BufferedInputStream(bis);
    destroy_BufferedOutputStream(bos);
}

int main(int argc, char** argv) {
    ErrorContext* err = NULL;
    PlatformFileCtx* srcFile = NULL;
    PlatformFileCtx* dstFile = NULL;

    err = create_ErrorContext();
    if (err == NULL) {
        fprintf(stderr, "main: failed to create error context\n");
        return 1;
    }

    CliOptions options;
    if (!parse_cli_options(argc, argv, &options, err)) {
        printStd_ErrorContext(err);
        goto cleanup;
    }

    srcFile = open_PlatformFileCtx(options.inputPath, "rb", err);
    if (srcFile == NULL) {
        printStd_ErrorContext(err);
        goto cleanup;
    }

    dstFile = open_PlatformFileCtx(options.outputPath, "wb", err);
    if (dstFile == NULL) {
        printStd_ErrorContext(err);
        goto cleanup;
    }

    if (options.mode == MODE_COMPRESS) {
        run_compress(srcFile, dstFile, err);
        if (peekSurfaceError_ErrorContext(err) != NULL) {
            append_ErrorContext(err, HF_ERR_UNKOWN, "compress: failed to compress");
        }
    } else if (options.mode == MODE_DECOMPRESS) {
        run_decompress(srcFile, dstFile, err);
        if (peekSurfaceError_ErrorContext(err) != NULL) {
            append_ErrorContext(err, HF_ERR_UNKOWN, "decompress: failed to decompress");
        }
    }

    if (peekSurfaceError_ErrorContext(err) != NULL) {
        printStd_ErrorContext(err);
        goto cleanup;
    }

    close_PlatformFileCtx(srcFile);
    close_PlatformFileCtx(dstFile);
    destroy_ErrorContext(err);
    return 0;

cleanup:
    if (srcFile != NULL) close_PlatformFileCtx(srcFile);
    if (dstFile != NULL) close_PlatformFileCtx(dstFile);
    destroy_ErrorContext(err);
    return 1;
}
