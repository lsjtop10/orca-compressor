
#include <minwindef.h>
#include <stdarg.h> 
#include "huffman.h"
#include "utils.h"

void printBin(uint8_t num) {
    for (int i = 7; i >= 0; --i) { // 8자리 숫자까지 나타냄
        int result = (num >> i) & 1;
        printf("%d", result);
    }
}

void printBuf(uint8_t* buf, size_t offset, size_t size, bool shouldPrintBin) {
    for (size_t i = offset; i < size; i++) {
        if (shouldPrintBin) {
            printBin(buf[i]);
        } else {
            printf("%02x", buf[i]);
        }
    }
}

size_t flush_StdOutStream(const void* s, uint8_t* buf, size_t offset, size_t bufSize, ErrorContext* err) {

    StdOutStream* ctx = (StdOutStream*)s;
    for (size_t i = offset; i < bufSize; i++) {
        switch (ctx->mode) {
        case bin:
            printBin(buf[i]);
            break;
        case ascii:
            printf("%c", buf[i]);
            break;
        case hex:
            printf("%02x", buf[i]);
            break;
        }
    }
    ((StdOutStream*)s)->cnt += bufSize - offset;
    return bufSize - offset;
}


typedef struct MemoryInputStream {
    uint8_t* bytes;
    size_t size;
    size_t ptr;

} MemoryInputStream;

MemoryInputStream* create_MemoryInputStream(void){
    MemoryInputStream* s = malloc(sizeof(MemoryInputStream));
    s->ptr = 0;
    s->size = 0;
    return s;
}

void fill_MemeoryInputStream(MemoryInputStream* const s, Borrow(uint8_t*)  arr, size_t size){ 
    s->bytes = malloc(size);
    memcpy(s->bytes, arr, size);
    s->size = size;
}

size_t fetch_MemoryInputStream(const void* stream, uint8_t* buf, size_t bufSize, ErrorContext* err) {
    MemoryInputStream* ctx = (MemoryInputStream*)stream;

    if(ctx->ptr >= ctx->size){
        append_ErrorContext(err, HF_STATE_END_OF_STREAM, "info: stream stream reaches end.");
        return 0;
    }

    size_t cur = 0;
    while (cur < bufSize) {
        if(ctx->ptr >= ctx->size) {
            return cur;
        }

        
        buf[cur] = ctx->bytes[ctx->ptr];
        ctx->ptr++;
        cur++;
    }

    return cur;
}

void clear_MemoryInputStream(MemoryInputStream* const s) {
    if (s->bytes != NULL) {
        free(s->bytes);
    }
    s->size = 0;
    s->ptr = 0;
}

void destroy_MemoryInputStream(MemoryInputStream *const s){
    if (s->bytes != NULL) {
        free(s->bytes);
    }

    free(s);
}


struct MemoryOutputStream {
    uint8_t* bytes;
    size_t capacity;
    size_t size;
};


MemoryOutputStream* create_MemoryOutputStream(void){
    MemoryOutputStream* ms = (MemoryOutputStream*) malloc(sizeof(MemoryOutputStream));
    ms->capacity = 64;
    ms->bytes = malloc(ms->capacity);
    ms->size = 0;

    return ms;
}

size_t flush_MemoryOutputStream(const void* stream, uint8_t* buf, size_t offset, size_t bufSize, ErrorContext* err) {
    MemoryOutputStream* ctx = (MemoryOutputStream*)stream;

    for (size_t i = offset; i < bufSize; i++) {
        if (ctx->size == ctx->capacity) { 
            uint8_t* tmp = realloc(ctx->bytes, ctx->capacity * 2);
            if(tmp == NULL){
                append_ErrorContext(err, HF_ERR_MEMALLOC_FAILED, "Error: Failed to allocate memory");
                return 0;
            }
            ctx->bytes = tmp;
            ctx->capacity = ctx->capacity * 2;

        }

        ctx->bytes[ctx->size] = buf[i];
        ctx->size++;
    }

    return bufSize - offset;
}

void clear_MemoryOutputStream(MemoryOutputStream* s) {
    s->size = 0;
}

void destroy_MemoryOutputStream(MemoryOutputStream* s) {
    if(s->bytes != NULL){
        free(s->bytes);
    }
    
    free(s);
}


Borrow(uint8_t*) borrowInternalBuf_MemoryOutputStream(MemoryOutputStream* s, Out(size_t*) size){
    *size = s->size;
    return s->bytes;
}

size_t getSize_MemoryOutputStream(MemoryOutputStream* s){
    return s->size;
}

//NOLINTNEXTLINE(misc-no-recusion)
void __debug_preOrderHuffmanTreeNode(HuffmanTreeNode* root) {
    /* NOLINT(misc-no-recursion) */
    if (root == NULL) {
        putchar('#');
        return;
    }

    if (root->left == NULL && root->right == NULL) {
        printf("%d", root->value);
    } else {
        putchar('-');
    }

    __debug_preOrderHuffmanTreeNode(root->left);
    __debug_preOrderHuffmanTreeNode(root->right);
}

char* createFormattedString(const char* format, ...) {
    if (!format) return NULL;

    va_list args;
    
    // 1. 첫 번째 가변 인자 탐색 시작 스캔 준비
    va_start(args, format);

    // 2. 💥 핵심 꼼수: 버퍼에 NULL, 크기에 0을 주면 vsnprintf는 출력을 하지 않고 
    //    이 포맷과 가변 인자들이 합쳐졌을 때 최종적으로 필요한 "글자 수"만 계산해서 반환합니다.
    va_list args_copy;
    va_copy(args_copy, args); // va_list는 한 번 소비되면 재사용이 불가능하므로 복사본 생성
    int len = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (len < 0) {
        va_end(args);
        return NULL;
    }

    // 3. 딱 필요한 바이트만큼 (+1 널 종료 문자) 힙 메모리 할당
    char* buffer = (char*)malloc(len + 1);
    if (!buffer) {
        va_end(args);
        return NULL;
    }

    // 4. 새로 파낸 힙 메모리에 진짜 안전하게 가변 문자열 주입
    int res = vsnprintf(buffer, len + 1, format, args);

    if(res < 0 || res != len){
        free(buffer);
        return NULL;
    }

    // 5. 가변 인자 자원 정리 해제
    va_end(args);

    return buffer; // 💡 Move(char*) 성격의 영수증 발행
}