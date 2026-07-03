#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <windows.h>
#include <sys/stat.h>


#include "errors.h"
#include "file.h"
#include "ownership.h"
#include "utils.h"



struct PlatformFileCtx {
    FILE* f;

    size_t nameLength;
    char* name;
};

/**
 * @brief UTF-8 문자열을 윈도우 전용 UTF-16 wchar_t 배열로 동적 할당 변환합니다.
 * @return Move(wchar_t*) 사용 후 반드시 free() 처리해야 함.
 */
static MustUse Move(wchar_t*) convert_ToWChar_Utils(Borrow(char*) utf8Str) {
    if (utf8Str == NULL)
        return NULL;

    // 1단계 (1-Pass): 필요한 wchar_t 배열의 '버퍼 크기(글자 수)'를 Windows 커널에 먼저 질의
    // 마지막 인자에 0을 주면 변환은 안 하고 필요한 크기만 반환합니다 (vsnprintf 꼼수와 동일).
    int wLen = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);
    if (wLen <= 0)
        return NULL;

    // 2단계: 필요한 글자 수만큼 정확하게 힙 메모리 파내기
    wchar_t* wBuf = (wchar_t*)malloc(wLen * sizeof(wchar_t));
    if (wBuf == NULL)
        return NULL;

    // 3단계: 파낸 안전한 공간에 진짜 UTF-16 데이터 주입 및 검증
    int result = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, wBuf, wLen);
    if (result <= 0) {
        free(wBuf); // 실패 시 자가 수거
        return NULL;
    }

    return wBuf; // Move 계약 성립
}

Move(PlatformFileCtx*)
    open_PlatformFileCtx(const char* utf8Path, const char* mode, Mut(ErrorContext*) err) {

    PlatformFileCtx* fCtx = (PlatformFileCtx*)malloc(sizeof(PlatformFileCtx));
    if (fCtx == NULL) {
        append_ErrorContext(err, HF_ERR_MEMALLOC_FAILED, "open file: failed to allocate context");
        goto err_malloc_fctx;
    }

    fCtx->name = fCtx->name = strdup(utf8Path);
    if (fCtx->name == NULL) {
        // NOLINTNEXTLINE(cert-err33-c)
        append_ErrorContext(err, HF_ERR_MEMALLOC_FAILED, "open file: failed to dump error string");
        goto err_strdup_name;
    }
    fCtx->nameLength = strlen(fCtx->name) + 1;

    wchar_t* wFilename = NULL;
    wchar_t* wMode = NULL;
    wFilename = convert_ToWChar_Utils(utf8Path);
    wMode = convert_ToWChar_Utils(mode);
    if (wFilename == NULL || wMode == NULL) {
        append_ErrorContext(err, HF_ERR_CONVERT_WCHAR_FAILED,
                            "open file: failed to convert path to wchar");
        goto err_convert_wchar;
    }

    // 윈도우 전용 와이드 오프너 호출
    FILE* fp = NULL;
    fp = _wfopen(wFilename, wMode);

    if (fp == NULL) {
        const int sysErr = errno; // 💡 OS가 심어둔 실패 원인 코드 박제

        const char* errMsg =
            createFormattedString("Windows _wfopen failed for '%s'. OS Reason: %s (errno: %d)",
                                  utf8Path, strerror(sysErr), sysErr);

        append_ErrorContext(err, HF_ERR_FILE_OPEN_FAILED, errMsg);
        goto err_wfopen;
    }

    fCtx->f = fp;

    return fCtx;

err_wfopen:
err_convert_wchar:
    free(wMode);
    free(wFilename);
err_strdup_name:
    free(fCtx->name);
    free(fCtx);
err_malloc_fctx:
    return NULL;
}

bool trySeek_PlatformFileCtx(PlatformFileCtx* ctx, uint64_t offset, ErrorContext* err) {
    if (ctx->f == NULL) {
        append_ErrorContext(err, HF_ERR_INVALID_FILE_CONTEXT, "seek file: invalid file context");
        return false;
    }

    int result = _fseeki64(ctx->f, (__int64)offset, SEEK_SET);

    if (result != 0) {
        // 실패 직후 C 런타임 에러 번호 백업
        int errnum = errno;

        // 에러 컨텍스트에 사람이 읽을 수 있는 표준 에러 메시지(strerror) 바인딩
        // 예: "Invalid argument", "Permission denied" 등
        append_ErrorContext(err, HF_ERR_FILE_SEEK_FAILED, strerror(errnum));

        // 디버깅 편의를 위해 파일 자체의 에러 상태 플래그가 켜졌다면 클리어
        if (ferror(ctx->f)) {
            clearerr(ctx->f);
        }

        return false;
    }
    return true;
}

bool tryMove_PlatformFileCtx(PlatformFileCtx* ctx, int delta, ErrorContext* err) {
    if (ctx->f == NULL) {
        append_ErrorContext(err, HF_ERR_INVALID_FILE_CONTEXT, "move file: invalid file context");
        return false;
    }

    int result = _fseeki64(ctx->f, (__int64)delta, SEEK_CUR);

    if (result != 0) {
        // 실패 직후 C 런타임 에러 번호 백업
        int errnum = errno;

        // 에러 컨텍스트에 사람이 읽을 수 있는 표준 에러 메시지(strerror) 바인딩
        // 예: "Invalid argument", "Permission denied" 등
        append_ErrorContext(err, HF_ERR_FILE_SEEK_FAILED, strerror(errnum));

        // 디버깅 편의를 위해 파일 자체의 에러 상태 플래그가 켜졌다면 클리어
        if (ferror(ctx->f)) {
            clearerr(ctx->f);
        }

        return false;
    }
    return true;
}

uint64_t getFileSize_PlatformFileCtx(PlatformFileCtx* ctx, ErrorContext* err) {
    int fd = _fileno(ctx->f);
    if (fd == -1) {
        append_ErrorContext(err, HF_ERR_INVALID_ARG, "get file size: invalid file descriptor");
        return 0;
    }

    struct _stat64 st;
    if (_fstat64(fd, &st) != 0) {
        append_ErrorContext(err, HF_ERR_FILE_IO_FAILED, "get file size: failed to query file status");
        return 0;
    }

    return (uint64_t)st.st_size;
}

size_t readBytes_PlatformFileCtx(PlatformFileCtx* ctx, uint8_t ptr[], size_t maxSize,
                                 ErrorContext* err) {
    if (ctx->f == NULL) {
        append_ErrorContext(err, HF_ERR_INVALID_FILE_CONTEXT, "read file: invalid file context");
        return 0;
    }

    size_t bytesRead = fread(ptr, 1, maxSize, ctx->f);
    // EOF에 도달했더라도 일단 읽어오는 데 성공했으면
    // 에러를 반환해선 안 됨.
    if (bytesRead < maxSize && !feof(ctx->f)) {
        int errnum = errno;
        append_ErrorContext(err, HF_ERR_STREAM_READ_FAILED, strerror(errnum));
        return 0;
    }

    if (bytesRead == 0 && feof(ctx->f)) {
        append_ErrorContext(err, HF_STATE_END_OF_STREAM,
                            "End of stream reached while reading data");
        return 0;
    }

    return bytesRead;

}

size_t writeBytes_PlatformFileCtx(PlatformFileCtx* ctx, const uint8_t ptr[], size_t size,
                                  ErrorContext* err) {
    if (ctx->f == NULL) {
        append_ErrorContext(err, HF_ERR_INVALID_FILE_CONTEXT, "write file: invalid file context");
        return 0;
    }

    size_t bytesWritten = fwrite(ptr, 1, size, ctx->f);
    if (bytesWritten != size) {
        int errnum = errno;
        append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, strerror(errnum));
        return 0;
    }
    return bytesWritten;
}

void close_PlatformFileCtx(PlatformFileCtx* ctx) {
    if(ctx->f != NULL){
        fclose(ctx->f);
    }
    ctx->f = NULL;
    free(ctx->name);
    ctx->name = NULL;

    free(ctx);
}

FileMetaV1* fillFileMeta_FileMetaV1(PlatformFileCtx* ctx, ErrorContext* err){

    FileMetaV1* fileMeta = malloc(sizeof(FileMetaV1) + ctx->nameLength);
    if (fileMeta == NULL) {
        append_ErrorContext(err, HF_ERR_MEMALLOC_FAILED, "fill file meta: failed to allocate memory");
        return NULL;
    }

    memcpy(fileMeta->signature, FILE_SIGNATURE, 4);
    fileMeta->version = 1;
    fileMeta->algorithm = ALGORITHM_HUFFMAN;
    fileMeta->originalNameLength = ctx->nameLength;

    memcpy(fileMeta->name, ctx->name, ctx->nameLength);
    return fileMeta;
}

void destroyFileMeta_FileMetaV1(FileMetaV1* meta){
    free(meta);
}
