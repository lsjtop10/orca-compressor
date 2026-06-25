
#include "ownership.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "buffered-stream.h"

void init_BufferdInputStream(BufferedInputStream* s, Borrow(void*) inputStream,
                             size_t (*fetch)(const void* stream, uint8_t* buf, size_t bufSize)) {

    s->inputStream = (void*)inputStream;
    s->fetch = fetch;

    s->bufSize = 0;
    s->offset = 0;
    s->pos = 0;
    s->totalReadBytes = 0;
}

/**
 * @brief Checks if current position is valid and current offset is valid. If not, fetches new data
 * from the stream.
 *
 */
bool hasNextBit_BufferdInputStream(BufferedInputStream* s) {

    // Offset이 7보다 크다는 말은 다음 바이트이 첫 번째 비트를
    if (s->offset > 7) {
        s->offset = 0;
        s->pos++;
    }
    // bit를 가져오는 연산은 바이트 내에서 이루어지므로
    // pos가 유효한 바이트이면 그 오프셋은 반드시 유효한 바이트이다.
    return hasNextByte_BufferdInputStream(s);
}

/**
 * @brief
 */
bool nextBit_BufferdInputStream(BufferedInputStream* s) {
    if (!hasNextBit_BufferdInputStream(s)) {
        return false;
        // TODO: 예외처리
    }

    // 가져오기 전 offset 범위 체크. 다음 bit가 무조건 있고 가정했기 때문에 그냥 올려도 무방함.
    uint8_t mask = 1 << (8 - s->offset - 1);
    bool bit = (s->buf[s->pos] & mask) != 0;

    s->offset++;
    if (s->offset > 7) {
        s->offset = 0;
        s->pos++;
        s->totalReadBytes++;
    }

    return bit;
}

bool hasNextByte_BufferdInputStream(BufferedInputStream* s) {

    // 일단 가리키는 idx가 작기만 하면 true
    if (s->pos < s->bufSize) {
        return true;
    }

    s->bufSize = s->fetch(s->inputStream, s->buf, STREAM_BUFFER_CAPACITY);
    // TODO: 에러 발생 시 재시도 처리

    if (s->bufSize > 0) {
        s->offset = 0;
        s->pos = 0;
        return true;
    }

    return false;
}

// If there is remaing bits at a byte. It will be discerded.
uint8_t nextByte_BufferdInputStream(BufferedInputStream* s) {

    // offset이 0이 아니면 읽던 바이트 버리고 다음 바이트 읽도록 함.
    if (s->offset != 0) {
        s->pos++;
        s->offset = 7;
    }

    if (!hasNextByte_BufferdInputStream(s)) {
        return false;
        // TODO: 예외처리
    }

    uint8_t byte = s->buf[s->pos];

    s->pos++;
    s->totalReadBytes++;
    return byte;
}

size_t totalReadSize_BufferdInputStream(BufferedInputStream* s) { return s->totalReadBytes; }

void init_BufferdOutputStream(BufferedOutptStream* s, Borrow(void*) outputStream,
                              size_t (*flush)(const void* stream, uint8_t* buf, size_t offset,
                                              size_t bufSize)) {
    s->outputStream = (void*)outputStream;
    s->flush = flush;

    s->pos = 0;
    s->offset = 0;
    s->totalWritedBytes = 0;
}

/**
 * @brief Write a bit to the end of the stream.
 */
void writeBit_BufferedOutputStream(BufferedOutptStream* s, bool bit) {

    if (s->pos > STREAM_BUFFER_CAPACITY) {
        s->flush(s->outputStream, s->buf, 0, STREAM_BUFFER_CAPACITY);
        s->pos = 0;
        s->offset = 0;
    }

    if (bit) {
        s->buf[s->pos] = s->buf[s->pos] | (1 << (8 - s->offset - 1));
    } else {
        s->buf[s->pos] = s->buf[s->pos] & ~(1 << (8 - s->offset - 1));
    }

    s->offset++;
    if (s->offset > 7) {
        s->offset = 0;
        s->pos++;
        s->totalWritedBytes++;
    }
}

/**
 * @brief Write a byte to the stream sequentially from the current bit position.
 */
void packAndWriteByte_BufferedOutputStream(BufferedOutptStream* s, uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        uint8_t mask = 1 << (8 - s->offset - 1);
        bool bit = (byte & mask) != 0;
        writeBit_BufferedOutputStream(s, bit);
    }
}

void writeByte_BufferedOutputStream(BufferedOutptStream* s, uint8_t byte) {

    if (s->offset != 0) {
        s->pos++;
        s->offset = 0;
    }

    if (s->pos > STREAM_BUFFER_CAPACITY) {
        s->flush(s->outputStream, s->buf, 0, s->pos);
        // TODO: 예외처리
        s->pos = 0;
    }

    s->buf[s->pos] = byte;
    s->pos++;
}

size_t flush_BufferedOutputStream(BufferedOutptStream* s) {
    while(s->offset != 0){
        writeBit_BufferedOutputStream(s, 0);
    }
  
    return s->flush(s->outputStream, s->buf, 0, s->pos);
}

size_t totalWritedSize_BufferdOutputStream(BufferedOutptStream* s){
    return s->totalWritedBytes;
}
