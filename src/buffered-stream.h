#ifndef BUFFERED_STREAM_H_INCLUDED
#define BUFFERED_STREAM_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ownership.h"

#define STREAM_BUFFER_CAPACITY 4096

typedef struct BufferedInputStream BufferedInputStream;
Move(BufferedInputStream*)
    create_BufferedInputStream(Borrow(void*) inputStream,
                              size_t (*fetch)(const void* stream, uint8_t* buf, size_t bufSize));

void reset_BufferedInputStream(BufferedInputStream* s, Borrow(void*) inputStream,
                              size_t (*fetch)(const void* stream, uint8_t* buf, size_t bufSize));
/**
 * @brief
 */
bool hasNextBit_BufferedInputStream(BufferedInputStream* s);

/**
 * @brief
 */
bool nextBit_BufferedInputStream(BufferedInputStream* s);

bool hasNextByte_BufferedInputStream(BufferedInputStream* s);
uint8_t nextByte_BufferedInputStream(BufferedInputStream* s);
size_t totalReadSize_BufferedInputStream(BufferedInputStream* s);

void nextData_BufferedInputStream(BufferedInputStream* s, uint8_t* ptr, size_t size);
void destroy_BufferedInputStream(Move(BufferedInputStream*) s);

typedef struct BufferedOutputStream BufferedOutputStream;

Move(BufferedOutputStream*)
    create_BufferedOutputStream(Borrow(void*) outputStream,
                               size_t (*flush)(const void* stream, uint8_t* buf, size_t offset,
                                               size_t bufSize));

void reset_BufferedOutputStream(BufferedOutputStream* s, Borrow(void*) outputStream,
                               size_t (*flush)(const void* stream, uint8_t* buf, size_t offset,
                                               size_t bufSize));

/**
 * @brief Write a bit to the end of the stream.
 */
void writeBit_BufferedOutputStream(BufferedOutputStream* s, bool bit);

/**
 * @brief Write a byte to the stream sequentially from the current bit position.
 */
void packAndWriteByte_BufferedOutputStream(BufferedOutputStream* s, uint8_t byte);
void writeByte_BufferedOutputStream(BufferedOutputStream* s, uint8_t byte);

void writeData_BufferedOutputStream(BufferedOutputStream* s, uint8_t* ptr, size_t size);
/**
 * Zerofill remaing bits and flushes buffer to stream.
 */
size_t flush_BufferedOutputStream(BufferedOutputStream* s);
size_t totalWritedSize_BufferedOutputStream(BufferedOutputStream* s);
void destroy_BufferedOutputStream(Move(BufferedOutputStream*) s);

#endif