#include "Decoder.h"
#include "huffman.h"
#include "ownership.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// 비트 패킹 중 남는 비트는 허프만 코드 길이보다 항상 작거나 같다.
// 허프만 코드 길이는 최대 심볼수까지 나올 수 있으므로 최소 256비트보다는 커야
// 한다. 넉넉잡아 16바이트 = 64비트 정도만 할당해주자.

struct StreamDecoder {
    uint8_t* rawBuf;
    size_t rawBufCapacity;
    size_t rawBufSize;

    uint8_t* encodedBuf;
    size_t encodedBufCapacity;
    size_t encodedBufSize;

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

    size_t encodedBufIdx;
    int encodedBufOffset;
};

Move(StreamDecoder*) create_StreamDecoder(StreamDecoderConfiguration cfg) {
    StreamDecoder* decoder = (StreamDecoder*)malloc(sizeof(StreamDecoder));

    decoder->rawBuf = malloc(cfg.rawBufSize);
    decoder->rawBufCapacity = cfg.rawBufSize;
    decoder->rawBufSize = 0;

    decoder->encodedBuf = malloc(cfg.encodedBufSize);
    decoder->encodedBufCapacity = cfg.encodedBufSize;
    decoder->encodedBufSize = 0;

    decoder->numOriginalBytes = cfg.numOriginalBytes;

    decoder->rawDataStrem = cfg.rawDataStrem;
    decoder->flushRawData = cfg.flushRawData;

    decoder->encodedDataStream = cfg.encodedDataStream;
    decoder->fetchEncodedData = cfg.fetchEncodedData;

    return decoder;
}

static bool nextBit(StreamDecoder* sd) {


    if(sd->encodedBufOffset >= 8){
        sd->encodedBufIdx++;
        sd->encodedBufOffset = 0;
    }

    if (sd->encodedBufIdx >= sd->encodedBufSize) {
        sd->encodedBufIdx = 0;
        sd->encodedBufSize = 0;
    } 

    if (sd->encodedBufSize == 0) {
        sd->encodedBufSize =
            sd->fetchEncodedData(sd->encodedDataStream, sd->encodedBuf, 0, sd->encodedBufCapacity);
    }

    uint8_t mask = 1 << (8 - sd->encodedBufOffset - 1);
    bool bit = (sd->encodedBuf[sd->encodedBufIdx] & mask) != 0;
    // 먼저 offset에서 읽은 다음 1 증가.
    sd->encodedBufOffset++;
    return bit;
}

static uint8_t findNextSymbol(StreamDecoder* sd, Borrow(HuffmanTreeNode*) htRoot) {

    // 0도 symbol일 수 있지만 마땅히 내보낼 값이 없다. 나중에 에러 처리 로직 도입하면
    // 에러를 반환할 예정이다.
    if (htRoot == NULL) {
        return 0;
    }

    if (htRoot->left == NULL && htRoot->right == NULL) {
        return htRoot->value;
    }

    bool hasNext = true;
    bool bit = nextBit(sd);
    // TODO: 에러 전파 코드 필요.
    if (!hasNext) {
        return 0;
    }

    if (htRoot->left != NULL && bit == false) {
        findNextSymbol(sd, htRoot->left);
    } else if (htRoot->right != NULL && bit == true) {
        findNextSymbol(sd, htRoot->right);
    } else {
        // 만약에 NextBit가 가라고 하는 방향에 노드가 없으면 에러.
        return 0;
    }
}

void decodeStream_StreamDecoder(StreamDecoder* sd, Borrow(HuffmanTreeNode*) htRoot) {

    sd->encodedBufIdx = 0;
    sd->encodedBufOffset = 0;

    for(int i = 0; i < sd->numOriginalBytes; i++){
        uint8_t symbol = findNextSymbol(sd, htRoot);

        // 넣기 전에 꽉 찻는지 확인
        if(sd->rawBufSize >= sd->rawBufCapacity){
            sd->flushRawData(sd->rawDataStrem, sd->rawBuf, 0, sd->rawBufSize);
            sd->rawBufSize = 0;
        }

        sd->rawBuf[sd->rawBufSize] = symbol;
        // 넣고 나면 유효한 데이터가 한 개 더 많아진다.
        sd->rawBufSize++;

    }

    sd->flushRawData(sd->rawDataStrem, sd->rawBuf, 0, sd->rawBufSize);
}

void destroy_StreamDecoder(StreamDecoder* sd) {

    if (sd->encodedBuf != NULL) {
        free(sd->encodedBuf);
    }

    if (sd->rawBuf != NULL) {
        free(sd->rawBuf);
    }

    if (sd != NULL) {
        free(sd);
    }
}