#ifndef ENCODER_H_INCLUDED
#define ENCODER_H_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include "huffman.h"
#include "ownership.h"
#include "buffered-stream.h"

// 힙 영역에 자체 포인터를 갖는 변수이고 자체 상태 캡슐화가 매우 중요한 구조체라서 불투명 포인터 사용.
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

void encodeStream_StreamEncoder(StreamEncoder* se, Borrow(HuffmanCodeTable*) ct);
#endif