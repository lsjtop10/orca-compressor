#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include "ownership.h"
#include <minwindef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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


size_t flush_StdOutStream(const void* s, uint8_t* buf, size_t offset, size_t bufSize);

typedef struct MemoryInputStream MemoryInputStream;
typedef struct MemoryOutputStream MemoryOutputStream;

MemoryInputStream* create_MemoryInputStream(void);
void fill_MemeoryInputStream(MemoryInputStream* const s, Borrow(uint8_t*)  arr, size_t size);
size_t fetch_MemoryInputStream(const void* stream, uint8_t* buf, size_t bufSize);
void clear_MemoryInputStream(MemoryInputStream* const s);
void destroy_MemoryInputStream(MemoryInputStream* const s);


MemoryOutputStream* create_MemoryOutStream(void);
size_t getSize_MemoryOutStream(MemoryOutputStream*);
size_t flush_MemeoryOutStream(const void* stream, uint8_t* buf, size_t offset, size_t bufSize);
void clear_MemoryOutStream(MemoryOutputStream* s);
void destroy_MemoryOutStream(MemoryOutputStream* s);

Borrow(uint8_t*) borrowInternalBuf_MemoryOutStream(MemoryOutputStream* s, Out(size_t*) size);
/**
* @brief Takes ownership of the buffer frome the stream. The array must be freed.
*/
Move(uint8_t*) takeInternalBuf_MemoryOutStream(MemoryOutputStream* s, Out(size_t*) size);

#endif