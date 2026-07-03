#ifndef BUFFERED_STREAM_H_INCLUDED
#define BUFFERED_STREAM_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "errors.h"
#include "ownership.h"

#define STREAM_BUFFER_CAPACITY 4096

typedef struct BufferedInputStream BufferedInputStream;
Move(BufferedInputStream*)
    create_BufferedInputStream(Borrow(void*) inputStream,
                               size_t (*fetch)(const void* stream, uint8_t* buf, size_t bufSize,
                                               ErrorContext* err));

void reset_BufferedInputStream(BufferedInputStream* s, Borrow(void*) inputStream,
                               size_t (*fetch)(const void* stream, uint8_t* buf, size_t bufSize,
                                               ErrorContext* err));

/**
 * @brief
 */

bool tryNextBit_BufferedInputStream(BufferedInputStream* s, bool* bit, ErrorContext* err);
bool tryNextByte_BufferedInputStream(BufferedInputStream* s, uint8_t* byte, ErrorContext* err);

size_t totalReadSize_BufferedInputStream(BufferedInputStream* s);

/**
 * @brief Reads `size` bytes encoded in big-endian format from the stream and stores the value in
 * host byte order. if the stream does not have enough bytes, returns false and sets the error
 * context.
 * @details This function reads `size` bytes from the stream and stores the value in host
 * byte order. The bytes are read in big-endian format, meaning the most significant byte is read
 * first. If the stream does not have enough bytes to read, the function returns false and sets the
 * error context. DO NOT USE THIS FUNCTION TO READ A SINGLE BYTE, OR BYTE ARRAY. USE
 * TRYNEXTBYTE_BUFFEREDINPUTSTREAM INSTEAD. MUST USE THIS FUNCTION TO READ A PRIMITIVE TYPE.
 * @param s The buffered input stream to read from.
 * @param ptr A pointer to the memory where the read bytes will be stored.
 * @param size The number of bytes to read.
 * @param err A pointer to an ErrorContext structure for error reporting.
 * @return Returns true if the read operation was successful, false otherwise.
 */
bool tryNextData_BufferedInputStream(BufferedInputStream* s, void* ptr, size_t size,
                                     ErrorContext* err);
void setLimit_BufferedInputStream(BufferedInputStream* s, uint64_t limit);
void clearLimit_BufferedInputStream(BufferedInputStream* s);
void destroy_BufferedInputStream(Move(BufferedInputStream*) s);

typedef struct BufferedOutputStream BufferedOutputStream;

Move(BufferedOutputStream*)
    create_BufferedOutputStream(Borrow(void*) outputStream,
                                size_t (*flush)(const void* stream, uint8_t* buf, size_t offset,
                                                size_t bufSize, ErrorContext* err));

void reset_BufferedOutputStream(BufferedOutputStream* s, Borrow(void*) outputStream,
                                size_t (*flush)(const void* stream, uint8_t* buf, size_t offset,
                                                size_t bufSize, ErrorContext* err));

/**
 * @brief Write a bit to the end of the stream.
 */
bool tryWriteBit_BufferedOutputStream(BufferedOutputStream* s, bool bit, ErrorContext* err);

/**
 * @brief Write a byte to the stream sequentially from the current bit position.
 */
bool tryPackAndWriteByte_BufferedOutputStream(BufferedOutputStream* s, uint8_t byte,
                                           ErrorContext* err);
bool tryWriteByte_BufferedOutputStream(BufferedOutputStream* s, uint8_t byte, ErrorContext* err);

/**
 * @brief Writes `size` bytes encoded in big-endian format to the stream and stores the value in
 * host byte order. if the stream does not have enough bytes, returns false and sets the error
 * context.
 * @details This function writes `size` bytes to the stream and stores the value in host
 * byte order. The bytes are written in big-endian format, meaning the most significant byte is
 * written first. If the stream does not have enough space to write, the function returns false and
 * sets the error context. DO NOT USE THIS FUNCTION TO WRITE A SINGLE BYTE, OR BYTE ARRAY. USE
 * WRITEBYTE_BUFFEREDOUTPUTSTREAM INSTEAD. MUST USE THIS FUNCTION TO WRITE A PRIMITIVE TYPE.
 * @param s The buffered output stream to write to.
 * @param ptr A pointer to the memory where the bytes to write are stored.
 * @param size The number of bytes to write.
 * @param err A pointer to an ErrorContext structure for error reporting.
 * @return Returns true if the write operation was successful, false otherwise.
 */
bool tryWriteData_BufferedOutputStream(BufferedOutputStream* s, uint8_t* ptr, size_t size,
                                       ErrorContext* err);

/**
 * Zerofill remaing bits and flushes buffer to stream.
 */
size_t flush_BufferedOutputStream(BufferedOutputStream* s, ErrorContext* err);
size_t totalWritedSize_BufferedOutputStream(BufferedOutputStream* s);
void destroy_BufferedOutputStream(Move(BufferedOutputStream*) s);

#endif