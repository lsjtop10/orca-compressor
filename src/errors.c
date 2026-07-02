#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errors.h"

typedef struct Error {
    ErrorCode code;

    /**
        Ownership: own.
        Must free after use.
    */
    char* msg;
    struct Error* next;
    struct Error* prev;
} Error;


static inline Move(Error*) create_Error(ErrorCode code, Borrow(char*) msg) {
    Error* elem = NULL;
    // 2. 내부 알맹이 노드 할당
    elem = malloc(sizeof(Error));
    if (elem == NULL)
        goto err_malloc_elem;

    elem->code = code;
    elem->next = NULL;
    elem->prev = NULL;

    // 3. Borrow한 문자열을 안전하게 내부 소유로 복제
    elem->msg = NULL;
    elem->msg = strdup(msg);
    if (elem->msg == NULL) {
        // NOLINTNEXTLINE(cert-err33-c)
        fputs("error: failed to dump error string\n", stderr);
        goto err_strdup_msg;
    }
    return elem;

err_strdup_msg:
    free(elem);
err_malloc_elem:
    return NULL;
}

inline ErrorCode getCode_Error(Error* err){
    return err->code;
}

inline Borrow(char*) getMsg_Error(Error* err){
    return err->msg;
}

static inline void destroy_Error(Error* elem) {
    free(elem->msg);
    elem->msg = NULL;
    free(elem);
}

struct ErrorContext {
    struct Error* head;
    struct Error* tail;
    int size;
};


Move(ErrorContext*) create_ErrorContext(void) {
    ErrorContext* ctx = malloc(sizeof(ErrorContext));
    if (ctx == NULL) {
        // NOLINTNEXTLINE(cert-err33-c)
        fputs("error: failed to create error context\n", stderr);
        goto err_malloc_ctx;
    }
    
    ctx->head = NULL;
    ctx->tail = NULL;
    ctx->head = create_Error(HF_ERR_UNKOWN, "HEAD");
    ctx->tail = create_Error(HF_ERR_UNKOWN, "TAIL");
    if (ctx->head == NULL || ctx->tail == NULL) {
        // NOLINTNEXTLINE(cert-err33-c)
        fputs("error: failed to create error context\n", stderr);
        goto err_create_error_elem;
    }

    ctx->size = 0;

    ctx->head->next = ctx->tail;
    ctx->tail->prev = ctx->head;
    return ctx;

err_create_error_elem:
    free(ctx->head);
    free(ctx->tail);
    free(ctx);
err_malloc_ctx:
    return NULL;
}

// append tail
void append_ErrorContext(ErrorContext* err, ErrorCode code, Borrow(char*) msg) {
    if (err == NULL) {
        return;
    }

    Error* newErr = create_Error(code, msg);
    if (newErr == NULL) {
        goto err_create_error_elem;
    }
    // newError가 추가될 위치의 이전 노드d

    Error* prevErr = err->tail->prev;

    prevErr->next = newErr;
    newErr->prev = prevErr;

    newErr->next = err->tail;
    err->tail->prev = newErr;


    err->size++;
    return;

err_create_error_elem:
    fputs("critical: failed to warp error\n", stderr);
    return;
}

bool contains_ErrorContext(ErrorContext* err, ErrorCode code) {
    // err이 null이면 코드가 애초에 존재할 수 없음.
    if (err == NULL) {
        return false;
    }

    Error* cur = err->head;

    while (cur != NULL) {
        if (cur->code == code) {
            return true;
        }
        cur = cur->next;
    }

    return false;
}

bool isRootCode_ErrorContext(ErrorContext *err, ErrorCode code){
    if(err == NULL){return false;}
    if (err->size == 0) {
        return false;
    }

    return err->head->next->code == code;
}


bool isSurfaceCode_ErrorContext(ErrorContext *err, ErrorCode code){
    if(err == NULL){return false;}
    if (err->size == 0) {
        return false;
    }
    return err->tail->prev->code == code;
}

inline void  printStd_ErrorContext(ErrorContext* err) {
    printFile_ErrorContext(err, stdout);
}

Move(Error*) consumeSurfaceError_ErrorContext(ErrorContext* err) {
    if (err == NULL) {
        return NULL;
    }

    Error* surfaceErr = err->tail->prev;
    if (err->size == 0) {
        return NULL;
    }

    // surfaceErr을 제거
    surfaceErr->prev->next = err->tail;
    err->tail->prev = surfaceErr->prev;

    err->size--;

    return surfaceErr;
}

Error* peekSurfaceError_ErrorContext(ErrorContext* err) {
    if (err == NULL) {
        return NULL;
    }

    if (err->size == 0) {
        return NULL;
    }

    return err->tail->prev;
}

void printFile_ErrorContext(ErrorContext* err, FILE* fp) {
    if (err == NULL) {
        return;
    }

    if (err->size == 0) {
        return;
    }

    Error* cur = err->head->next;

    while (cur != err->tail) {
        // NOLINTNEXTLINE(cert-err33-c)
        fprintf(fp, "Error: %s, code: %d s\n", cur->msg, cur->code);
        cur = cur->next;
    }
}

void destroy_ErrorContext(ErrorContext* err) {

    Error* current = err->head;

    while (current != NULL) {
        Error* nextNode = current->next;
        destroy_Error(current);
        current = nextNode;
    }

    free(err);
}