#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <minwindef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "errors.h"
#include "ownership.h"
#include "huffman.h"

static inline bool isLittleEndian(void) {
    // uint16_t test = 0x1234;
    // uint8_t* ptr = (uint8_t*)&test;
    // return ptr[0] == 0x34;
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    // 1. GCC/Clang이 빌드할 때 빅 엔디안인 걸 알면
    // 이 함수 전체를 그냥 'return false;' 라는 단 한 줄의 상수로 치환해 버립니다.
    return false;
#elif defined(_MSC_VER) && defined(_M_PPC)
    // 2. 구형 MSVC 빅 엔디안 환경 대응용 안전장치
    return false;
#else
    // 3. 윈도우(x86_64), 리눅스, 안드로이드 등 전 세계 99%의 리틀 엔디안 환경.
    // build time에 바로 true 평가.
    return true;
#endif
}

typedef enum PrintMode {
    bin,
    ascii,
    hex,
} PrintMode;

typedef struct StdOutStream {
    uint64_t cnt;
    PrintMode mode;
} StdOutStream;

void printBin(uint8_t num);
void printBuf(uint8_t* buf, size_t offset, size_t size, bool shouldPrintBin); 


size_t flush_StdOutStream(const void* s, uint8_t* buf, size_t offset, size_t bufSize, ErrorContext* err);

typedef struct MemoryInputStream MemoryInputStream;
typedef struct MemoryOutputStream MemoryOutputStream;

MemoryInputStream* create_MemoryInputStream(void);
void fill_MemeoryInputStream(MemoryInputStream* const s, Borrow(uint8_t*)  arr, size_t size);
size_t fetch_MemoryInputStream(const void* stream, uint8_t* buf, size_t bufSize, ErrorContext* err);
void clear_MemoryInputStream(MemoryInputStream* const s);
void destroy_MemoryInputStream(MemoryInputStream* const s);


MemoryOutputStream* create_MemoryOutputStream(void);
size_t getSize_MemoryOutputStream(MemoryOutputStream*);
size_t flush_MemoryOutputStream(const void* stream, uint8_t* buf, size_t offset, size_t bufSize, ErrorContext* err);
void clear_MemoryOutputStream(MemoryOutputStream* s);
void destroy_MemoryOutputStream(MemoryOutputStream* s);

Borrow(uint8_t*) borrowInternalBuf_MemoryOutputStream(MemoryOutputStream* s, Out(size_t*) size);
/**
* @brief Takes ownership of the buffer frome the stream. The array must be freed.
*/
Move(uint8_t*) takeInternalBuf_MemoryOutputStream(MemoryOutputStream* s, Out(size_t*) size);


void __debug_preOrderHuffmanTreeNode(HuffmanTreeNode* root);
Move(char*) createFormattedString(const char* format, ...);
#endif