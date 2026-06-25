#ifndef DECODER_H_INCLUDED
#define DECODER_H_INCLUDED

#include <stdint.h>
#include "ownership.h"
#include "huffman.h"
#include "buffered-stream.h"

typedef struct StreamDecoder{
    /**
    `Ownership`: borrow
    */
    BufferedInputStream* bis;
    
    /**
    `Ownership`: borrow
    */
    BufferedOutputStream* bos;

    uint64_t numOriginalBytes;

} StreamDecoder;

void decodeStream_StreamDecoder(StreamDecoder* const se, Borrow(HuffmanTreeNode*)  huffmanTreeRoot);

#endif