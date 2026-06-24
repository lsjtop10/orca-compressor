#ifndef ENCODER_H_INCLUDED
#define ENCODER_H_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include "huffman.h"
#include "ownership.h"

// 힙 영역에 자체 포인터를 갖는 변수이고 자체 상태 캡슐화가 매우 중요한 구조체라서 불투명 포인터 사용.
typedef struct StreamEncoderConfiguration{
    size_t encodedBufSize;
    size_t rawBufSize;

    const void* rawDataStrem;
    size_t (*fetchRawData)(const void* stream, uint8_t* buf, size_t offset, size_t bufSize);

    const void* encodedDataStream;
    size_t (*flushEncodedData)(const void* stream, uint8_t* buf, size_t offset, size_t bufSize);
} StreamEncoderConfiguration;

typedef struct StreamEncoder StreamEncoder;
StreamEncoder* create_StreamEncoder(StreamEncoderConfiguration cfg);
void encodeStream_StreamEncoder(StreamEncoder* se, Borrow(HuffmanCodeTable*) ct);
void destroy_StreamEncoder(StreamEncoder* se);



#endif