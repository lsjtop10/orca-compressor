#ifndef ENCODER_H_INCLUDED
#define ENCODER_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

#include "ownership.h"
#include "errors.h"

#include "huffman.h"
#include "buffered-stream.h"


typedef struct StreamEncoder {
    /**
    `Ownership`: borrow
    */
   BufferedInputStream *bis;
   
    /**
    `Ownership`: borrow
    */
   BufferedOutputStream *bos;
}StreamEncoder;

void encodeStream_StreamEncoder(StreamEncoder* se, Borrow(HuffmanCodeTable*) ct, ErrorContext* err);

typedef struct{
    BufferedOutputStream* bos;
} HuffmanTreeSirializer;

void serializeTree_HuffmanTreeSirializer(HuffmanTreeSirializer* ts, Borrow(HuffmanTreeNode*) htRoot, ErrorContext* err);

#endif