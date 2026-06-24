#include "encoder.h"
#include "huffman.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdio.h>

// 비트 패킹 중 남는 비트는 허프만 코드 길이보다 항상 작거나 같다.
// 허프만 코드 길이는 최대 심볼수까지 나올 수 있으므로 최소 256비트보다는 커야
// 한다. 넉넉잡아 16바이트 = 64비트 정도만 할당해주자.

struct StreamEncoder {
    uint8_t* rawBuf;
    size_t rawBufCapacity;
    // 완성된 한 바이트가 들어 있는 개수.
    size_t rawBufSize;

    uint8_t* encodedBuf;
    size_t encodedBufCapacity;
    size_t encodedBufSize;

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
    size_t (*fetchRawData)(const void* stream, uint8_t* buf, size_t offset, size_t bufSize);

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
    size_t (*flushEncodedData)(const void* stream, uint8_t* buf, size_t offset, size_t bufSize);
};

StreamEncoder* create_StreamEncoder(StreamEncoderConfiguration cfg) {
    StreamEncoder* encoder = (StreamEncoder*)malloc(sizeof(StreamEncoder));

    encoder->rawBuf = malloc(cfg.rawBufSize);
    encoder->rawBufCapacity = cfg.rawBufSize;
    encoder->encodedBufSize = 0;

    encoder->encodedBuf = malloc(cfg.encodedBufSize);
    encoder->encodedBufCapacity = cfg.encodedBufSize;
    encoder->encodedBufSize = 0;

    encoder->rawDataStrem = cfg.rawDataStrem;
    encoder->fetchRawData = cfg.fetchRawData;

    encoder->encodedDataStream = cfg.encodedDataStream;
    encoder->flushEncodedData = cfg.flushEncodedData;

    return encoder;
}

void encodeStream_StreamEncoder(StreamEncoder* se, Borrow(HuffmanCodeTable*) ct) {

    // 바이트를 가져온다.
    se->rawBufSize = se->fetchRawData(se->rawDataStrem, se->rawBuf, 0, se->rawBufCapacity);

    if (se->rawBufSize == 0) {
        // FIXME: bufsize == 0일 때 early return.
        // 에러처리
        return;
    }

    // 한 바이트에서 현재 기록할 bit의 idx. MSB가 0이고 LSB가 7.
    int offset = 0;

    // 더 이상 가져올 바이트가 없을 때까지
    while (se->rawBufSize > 0) {

        // 0부터 size - 1까지 buf를 순회하면서
        for (size_t i = 0; i < se->rawBufSize; i++) {
            uint8_t symbol = se->rawBuf[i];

            HuffmanCode code = lookupCode_HuffmanCodeTable(ct, symbol);
            size_t length = lookupCodeLength_HuffmanCodeTable(ct, symbol);

            for (size_t j = 0; j < length; j++) {

                // 오버플로우 안 나겠지...
                // Unsinged 자료형에서 오버플로우는 모듈러 연산처럼 작동함.
                // 모듈러가 같으려면 두 값이 같거나 Mod한 수만큼 차이가 나야 하는데
                // offset은 커봐야 7이니 걱정 안 해도 괜찮겠다.

                if (se->encodedBufSize >= se->encodedBufCapacity) {
                    se->flushEncodedData(se->encodedDataStream, se->encodedBuf, 0,
                                         se->encodedBufSize);
                    se->encodedBufSize = 0;
                }


                printf("idx %lld, offset %d \n", se->encodedBufSize, offset);

                if (getBit_HuffmanCode(code, j)) {
                    se->encodedBuf[se->encodedBufSize] =
                        se->encodedBuf[se->encodedBufSize] | (1 << (8 - offset - 1));
                } else {
                    se->encodedBuf[se->encodedBufSize] =
                        se->encodedBuf[se->encodedBufSize] & ~(1 << (8 - offset - 1));
                }

                offset++;
                if (offset > 7) {
                    se->encodedBufSize++;
                    offset = 0;
                }
            }
        }

        se->rawBufSize = se->fetchRawData(se->rawDataStrem, se->rawBuf, 0, se->rawBufCapacity);
    }

    // 뒤에 남은 값은 0으로 패킹
    if (offset != 0) {
        for (int i = offset + 1; i <= 7; i++) {
            se->encodedBuf[se->encodedBufSize] =
                se->encodedBuf[se->encodedBufSize] & ~(1 << (8 - i - 1));
        }
    }

    // 남은 output 버퍼를 flush한다.
    // 불완전한 바이트를 0으로 패킹해 완전한 바이트로 만들었으므로 size ++
    se->encodedBufSize++;
    se->flushEncodedData(se->encodedDataStream, se->encodedBuf, 0, se->encodedBufSize);
}

void destroy_StreamEncoder(StreamEncoder* se) {

    if (se->encodedBuf != NULL) {
        free(se->encodedBuf);
    }

    if (se->rawBuf != NULL) {
        free(se->rawBuf);
    }

    if (se != NULL) {
        free(se);
    }
}