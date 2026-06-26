
#include "utils.h"
#include "huffman.h"
#include <minwindef.h>

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

size_t flush_StdOutStream(const void* s, uint8_t* buf, size_t offset, size_t bufSize) {

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

size_t fetch_MemoryInputStream(const void* stream, uint8_t* buf, size_t bufSize) {
    MemoryInputStream* ctx = (MemoryInputStream*)stream;
    size_t cnt = 0;

    size_t cur = 0;
    while (ctx->ptr < ctx->size && cur < bufSize) {
        buf[cur] = ctx->bytes[ctx->ptr];
        ctx->ptr++;
        cur++;
        cnt++;
    }

    return cnt;
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


MemoryOutputStream* create_MemoryOutStream(void){
    MemoryOutputStream* ms = (MemoryOutputStream*) malloc(sizeof(MemoryOutputStream));
    ms->capacity = 64;
    ms->bytes = malloc(ms->capacity);
    ms->size = 0;

    return ms;
}

size_t flush_MemeoryOutStream(const void* stream, uint8_t* buf, size_t offset, size_t bufSize) {
    MemoryOutputStream* ctx = (MemoryOutputStream*)stream;

    for (size_t i = offset; i < bufSize; i++) {
        if (ctx->size == ctx->capacity) {      
            ctx->bytes = realloc(ctx->bytes, ctx->capacity * 2);
            // TODO: 널체크 해야 하는데 간단하게 사용할 객체라 생략.
            ctx->capacity = ctx->capacity * 2;

        }

        ctx->bytes[ctx->size] = buf[i];
        ctx->size++;
    }

    return bufSize - offset;
}

void clear_MemoryOutStream(MemoryOutputStream* s) {
    s->size = 0;
}

void destroy_MemoryOutStream(MemoryOutputStream* s) {
    if(s->bytes != NULL){
        free(s->bytes);
    }
    
    free(s);
}


Borrow(uint8_t*) borrowInternalBuf_MemoryOutStream(MemoryOutputStream* s, Out(size_t*) size){
    *size = s->size;
    return s->bytes;
}

/**
* @brief Takes ownership of the buffer frome the stream. The array must be freed.
*/
Move(uint8_t*) takeInternalBuf_MemoryOutStream(MemoryOutputStream* s, Out(size_t*) size){
    *size = s->size;
    uint8_t* bytePtr = s->bytes;
    clear_MemoryOutStream(s);
    return bytePtr;
}

size_t getSize_MemoryOutStream(MemoryOutputStream* s){
    return s->size;
}

void __debug_preOrderHuffmanTreeNode(HuffmanTreeNode* root) {
    /* NOLINT(misc-no-recursion) */
    if (root == NULL) {
        putchar('#');
        return;
    }

    if (root->left == NULL && root->right == NULL) {
        putchar(root->value);
    } else {
        putchar('0');
    }

    __debug_preOrderHuffmanTreeNode(root->left);
    __debug_preOrderHuffmanTreeNode(root->right);
}