#ifndef BUFFERED_STREAM_H_INCLUDED
#define BUFFERED_STREAM_H_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "ownership.h"

#define STREAM_BUFFER_CAPACITY 10

// DO NOT CHANGE ANY FIELDS
// CHANGING FIELD WILL CAUSE MEMORY CORRUPTION
typedef struct BufferedInputStream {
    uint8_t buf[STREAM_BUFFER_CAPACITY];
    size_t bufSize;

    // DO NOT ACCESS DIRACTLY. MUST CALL totalReadSize
    size_t totalReadBytes;

    size_t pos;
    int offset;

    /**
    *@brief Pointer to a struct containing context of the raw data stream.
    * `ownership`: borrow
    */
    void* inputStream;

    /**
     * @brief Callback function fetching bytes from the stream.
     * @details This Callback function fetches bytes at ot less then the bufSize.   
     * **Params**:
     * **stream**: A struct containing context of the stream.
     * **buf**: The Pointer to the start address of the buffer.
     * from the offset.
     * **bufSize**: Size of the buffer.
     * 
     * Return:
     * Num of bytes feched. Return 0 when fails to fetch raw data.
     */

    size_t (*fetch)(const void* stream, uint8_t* buf,size_t bufSize);
} BufferedInputStream;


void init_BufferdInputStream(BufferedInputStream* s, Borrow(void*) inputStream, size_t (*fetch)(const void* stream, uint8_t* buf, size_t bufSize));

/**
* @brief 
*/
bool hasNextBit_BufferdInputStream(BufferedInputStream* s);

/**
* @brief 
*/
bool nextBit_BufferdInputStream(BufferedInputStream* s);

bool hasNextByte_BufferdInputStream(BufferedInputStream* s);
uint8_t nextByte_BufferdInputStream(BufferedInputStream* s);
size_t totalReadSize_BufferdInputStream(BufferedInputStream* s);

// bool hasNextMultiByte();
// size_t nextMultiByte(uint8_t* ptr, size_t size);


// DO NOT CHANGE ANY FIELDS
// CHANGING FIELD WILL CAUSE MEMORY CORRUPTION
typedef struct BufferedOutputStream {
    uint8_t buf[STREAM_BUFFER_CAPACITY];
    size_t pos;
    int offset;

    size_t totalWritedBytes;

    /**
    * @brief Pointer to a struct containing context of the output stream. 
    * `ownership`: borrow
    */
    void* outputStream;

      /**
     * @brief Callback function flushing bytes from the stream.
     * @details This Callback function flushes bytes at ot less then the bufSize.   
     * **Params**:
     * `stream`: A struct containing context of the stream.
     * `buf`: The Pointer to the start address of the buffer.
     * `offset`: The offset from start address from the buffer. Must fill data
     * from the offset.
     * `bufSize`: Size of the buffer.
     * 
     * **Return**:
     * Num of bytes flushed. Return 0 when fails to flush data.
     */
    size_t (*flush)(const void* stream, uint8_t* buf, size_t offset, size_t bufSize);

} BufferedOutptStream;

void init_BufferdOutputStream(BufferedOutptStream* s, Borrow(void*) outputStream, size_t (*flush)(const void* stream, uint8_t* buf, size_t offset, size_t bufSize));

/** 
* @brief Write a bit to the end of the stream. 
*/
void writeBit_BufferedOutputStream(BufferedOutptStream* s,bool bit);

/**
 * @brief Write a byte to the stream sequentially from the current bit position.
 */
void packAndWriteByte_BufferedOutputStream(BufferedOutptStream* s,uint8_t byte);
void writeByte_BufferedOutputStream(BufferedOutptStream* s, uint8_t byte);
/**
* Zerofill remaing bits and flushes buffer to stream.
*/
size_t flush_BufferedOutputStream(BufferedOutptStream* s);
size_t totalWritedSize_BufferdOutputStream(BufferedOutptStream* s);


#endif