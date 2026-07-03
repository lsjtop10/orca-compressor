#include <stdlib.h>
#include <string.h>

#include "serializer.h"
#include "ownership.h"

// FileMetaV1 직렬화
void writeFileMeta_Serializer(BufferedOutputStream* bos, FileMetaV1* meta, ErrorContext* err) {
    if (peekSurfaceError_ErrorContext(err) != NULL) {
        return;
    }

    for (int i = 0; i < 4; i++) {
        if (!tryWriteByte_BufferedOutputStream(bos, meta->signature[i], err)) {
            append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "serialize file meta: failed to write signature");
            return;
        }
    }

    if (!tryWriteByte_BufferedOutputStream(bos, meta->version, err)) {
        append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "serialize file meta: failed to write version");
        return;
    }

    if (!tryWriteData_BufferedOutputStream(bos, (uint8_t*)&meta->algorithm, sizeof(Algorithm), err)) {
        append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "serialize file meta: failed to write algorithm");
        return;
    }

    if (!tryWriteData_BufferedOutputStream(bos, (uint8_t*)&meta->originalNameLength, sizeof(uint16_t), err)) {
        append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "serialize file meta: failed to write original name length");
        return;
    }

    for (uint16_t i = 0; i < meta->originalNameLength; i++) {
        if (!tryWriteByte_BufferedOutputStream(bos, meta->name[i], err)) {
            append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "serialize file meta: failed to write original name");
            return;
        }
    }
}

// FileMetaV1 역직렬화
FileMetaV1* readAndCreateFileMeta_Serializer(BufferedInputStream* bis, ErrorContext* err) {
    if (peekSurfaceError_ErrorContext(err) != NULL) {
        return NULL;
    }

    int8_t signature[4];
    for (int i = 0; i < 4; i++) {
        if (!tryNextByte_BufferedInputStream(bis, (uint8_t*)&signature[i], err)) {
            append_ErrorContext(err, HF_ERR_STREAM_READ_FAILED, "deserialize file meta: failed to read signature");
            return NULL;
        }
    }

    if (memcmp(signature, "OCOP", 4) != 0) {
        append_ErrorContext(err, HF_ERR_UNSUPPORTED_FILE, "deserialize file meta: unsupported file signature");
        return NULL;
    }

    uint8_t version;
    if (!tryNextByte_BufferedInputStream(bis, &version, err)) {
        append_ErrorContext(err, HF_ERR_STREAM_READ_FAILED, "deserialize file meta: failed to read version");
        return NULL;
    }
    if (version != 1) {
        append_ErrorContext(err, HF_ERR_UNSUPPORTED_VERSION, "deserialize file meta: unsupported version");
        return NULL;
    }

    Algorithm algorithm;
    if (!tryNextData_BufferedInputStream(bis, &algorithm, sizeof(Algorithm), err)) {
        append_ErrorContext(err, HF_ERR_STREAM_READ_FAILED, "deserialize file meta: failed to read algorithm");
        return NULL;
    }

    uint16_t originalNameLength;
    if (!tryNextData_BufferedInputStream(bis, &originalNameLength, sizeof(uint16_t), err)) {
        append_ErrorContext(err, HF_ERR_STREAM_READ_FAILED, "deserialize file meta: failed to read original name length");
        return NULL;
    }

    FileMetaV1* fileMeta = malloc(sizeof(FileMetaV1) + originalNameLength);
    if (fileMeta == NULL) {
        append_ErrorContext(err, HF_ERR_MEMALLOC_FAILED, "deserialize file meta: failed to allocate memory");
        return NULL;
    }

    memcpy(fileMeta->signature, signature, 4);
    fileMeta->version = version;
    fileMeta->algorithm = algorithm;
    fileMeta->originalNameLength = originalNameLength;

    for (uint16_t i = 0; i < originalNameLength; i++) {
        if (!tryNextByte_BufferedInputStream(bis, (uint8_t*)&fileMeta->name[i], err)) {
            free(fileMeta);
            append_ErrorContext(err, HF_ERR_STREAM_READ_FAILED, "deserialize file meta: failed to read original name");
            return NULL;
        }
    }

    return fileMeta;
}

// ChunkInfoV1 직렬화
void writeChunkInfo_Serializer(BufferedOutputStream* bos, ChunkInfoV1* chunk, ErrorContext* err) {
    if (peekSurfaceError_ErrorContext(err) != NULL) {
        return;
    }

    if (!tryWriteData_BufferedOutputStream(bos, (uint8_t*)&chunk->size, sizeof(chunk->size), err)) {
        append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "serialize chunk: failed to write size");
        return;
    }
    if (!tryWriteByte_BufferedOutputStream(bos, chunk->version, err)) {
        append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "serialize chunk: failed to write version");
        return;
    }
    if (!tryWriteData_BufferedOutputStream(bos, (uint8_t*)&chunk->originalSize, sizeof(chunk->originalSize), err)) {
        append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "serialize chunk: failed to write original size");
        return;
    }
    if (!tryWriteData_BufferedOutputStream(bos, (uint8_t*)&chunk->treeSize, sizeof(chunk->treeSize), err)) {
        append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "serialize chunk: failed to write tree size");
        return;
    }
}

// ChunkInfoV1 역직렬화
void readChunkInfo_Serializer(BufferedInputStream* bis, ChunkInfoV1* chunk, ErrorContext* err) {
    if (peekSurfaceError_ErrorContext(err) != NULL) {
        return;
    }

    if (!tryNextData_BufferedInputStream(bis, &chunk->size, sizeof(chunk->size), err)) {
        append_ErrorContext(err, HF_ERR_STREAM_READ_FAILED, "deserialize chunk: failed to read size");
        return;
    }
    if (!tryNextByte_BufferedInputStream(bis, &chunk->version, err)) {
        append_ErrorContext(err, HF_ERR_STREAM_READ_FAILED, "deserialize chunk: failed to read version");
        return;
    }
    if (!tryNextData_BufferedInputStream(bis, &chunk->originalSize, sizeof(chunk->originalSize), err)) {
        append_ErrorContext(err, HF_ERR_STREAM_READ_FAILED, "deserialize chunk: failed to read original size");
        return;
    }
    if (!tryNextData_BufferedInputStream(bis, &chunk->treeSize, sizeof(chunk->treeSize), err)) {
        append_ErrorContext(err, HF_ERR_STREAM_READ_FAILED, "deserialize chunk: failed to read tree size");
        return;
    }
}

// HuffmanTree 직렬화 재귀 헬퍼
static void serializeTreeImpl_Serializer(BufferedOutputStream* bos, Borrow(HuffmanTreeNode*) htRoot, ErrorContext* err) {
    int16_t val = 0;

    if (htRoot == NULL) {
        val = -2;
        if (!tryWriteData_BufferedOutputStream(bos, (uint8_t*)&val, sizeof(int16_t), err)) {
            append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "serialize tree: failed to write null node");
            return;
        }
        return;
    }

    val = htRoot->value;
    if (!tryWriteData_BufferedOutputStream(bos, (uint8_t*)&val, sizeof(int16_t), err)) {
        append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "serialize tree: failed to write node value");
        return;
    }

    serializeTreeImpl_Serializer(bos, htRoot->left, err);
    serializeTreeImpl_Serializer(bos, htRoot->right, err);
}

// HuffmanTree 직렬화 진입점
void serializeTree_Serializer(BufferedOutputStream* bos, Borrow(HuffmanTreeNode*) htRoot, ErrorContext* err) {
    serializeTreeImpl_Serializer(bos, htRoot, err);
    if (flush_BufferedOutputStream(bos, err) == 0) {
        append_ErrorContext(err, HF_ERR_STREAM_FLUSH_FAILED, "serialize tree: failed to flush stream");
        return;
    }
}

// HuffmanTree 역직렬화 재귀 헬퍼
static HuffmanTreeNode* deserializeTreeImpl_Serializer(BufferedInputStream* bis, ErrorContext* err) {
    if (peekSurfaceError_ErrorContext(err) != NULL) {
        return NULL;
    }

    int16_t val = 0;
    if (!tryNextData_BufferedInputStream(bis, (uint8_t*)&val, sizeof(int16_t), err)) {
        return NULL;
    }

    if (val == -2) {
        return NULL;
    }

    HuffmanTreeNode* n = crate_HuffmanTreeNode(0, val);
    if (n == NULL) {
        append_ErrorContext(err, HF_ERR_MEMALLOC_FAILED, "deserialize tree: failed to allocate memory");
        return NULL;
    }

    n->left = deserializeTreeImpl_Serializer(bis, err);
    n->right = deserializeTreeImpl_Serializer(bis, err);

    if (peekSurfaceError_ErrorContext(err) != NULL) {
        destroy_HuffmanTreeNode(n);
        return NULL;
    }

    return n;
}

// HuffmanTree 역직렬화 진입점
HuffmanTreeNode* deserializeTree_Serializer(BufferedInputStream* bis, uint32_t treeSize, ErrorContext* err) {
    if (peekSurfaceError_ErrorContext(err) != NULL) {
        return NULL;
    }

    uint64_t startPos = totalReadSize_BufferedInputStream(bis);
    setLimit_BufferedInputStream(bis, startPos + treeSize);

    HuffmanTreeNode* root = deserializeTreeImpl_Serializer(bis, err);

    clearLimit_BufferedInputStream(bis);
    return root;
}
