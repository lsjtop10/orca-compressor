#ifndef DECODER_H_INCLUDED
#define DECODER_H_INCLUDED

#include <stdint.h>
#include "ownership.h"
#include "huffman.h"

typedef struct StreamDecoderConfiguration{
    size_t encodedBufSize;
    size_t rawBufSize;
    
    // Num of original symbols of original data(num of bytes of original data) 
    uint64_t numOriginalBytes;

 /**
    @brief Pointer to a struct containing context of the raw data stream.
    */
    const void* rawDataStrem;
    /**
     * @brief This callback function fetches raw data(uncompressed data) from the
     * stream.
     * @param stream A struct containing context of the stream
     * @param buf The Pointer to the start address of the buffer
     * @param offset The offset from start address from the buffer. Must fill data
     * from the offset.
     * @param bufSize Size of the buffer.
     *
     * @return Num of bytes feched. Return 0 when fails to fetch raw data.
     */
    size_t (*fetchEncodedData)(const void* stream, uint8_t* buf, size_t offset, size_t bufSize);
    
    /**
    @brief Pointer to a struct containing context of the encoded data stream.
    */
    const void* encodedDataStream;
    /**
     * @brief This callback function flushes encoded data(compressed data) to the
     * stream.
     * @param stream A struct containing context of the stream(i.e file pointer).
     * @param buf The Pointer to the start address of the buffer.
     * @param offset The offset from start address from the buffer. Must fill data
     * from the offset.
     * @param bufSize Size of the buffer.
     *
     * @return Num of bytes flushed.
     */
    size_t (*flushRawData)(const void* stream, uint8_t* buf, size_t offset, size_t bufSize);
} StreamDecoderConfiguration;


typedef struct StreamDecoder StreamDecoder;

Move(StreamDecoder*) create_StreamDecoder(StreamDecoderConfiguration cfg);
void decodeStream_StreamDecoder(StreamDecoder* const se, Borrow(HuffmanTreeNode*)  huffmanTreeRoot);
void destroy_StreamDecoder(Move(StreamDecoder*) sd);

#endif