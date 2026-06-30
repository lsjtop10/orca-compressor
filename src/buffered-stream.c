
#include "ownership.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "buffered-stream.h"


static inline bool isLittleEndian(void){
    // uint16_t test = 0x1234;
    // uint8_t* ptr = (uint8_t*)&test;
    // return ptr[0] == 0x34;
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    // 1. GCC/Clang이 빌드할 때 빅 엔디안인 걸 알면 
    // 이 함수 전체를 그냥 'return false;' 라는 단 한 줄의 상수로 치환해 버립니다.
    return false;
#elif defined(_MSC_VER) && defined(_M_PPC)
    // 2. 구형 MSVC 빅 엔디안 환경 대응용 안전장치
    return false;
#else
    // 3. 윈도우(x86_64), 리눅스, 안드로이드 등 전 세계 99%의 리틀 엔디안 환경
    // 컴파일러가 빌드 타임에 이 함수를 아예 'return true;' 상수로 박아버립니다!
    return true;
#endif
} 

struct BufferedInputStream {
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
};

Move(BufferedInputStream*) create_BufferedInputStream(Borrow(void*) inputStream,
                             size_t (*fetch)(const void* stream, uint8_t* buf, size_t bufSize)) {
    BufferedInputStream* s = malloc(sizeof(BufferedInputStream));

    reset_BufferedInputStream(s, inputStream, fetch);
    return s;
}

void reset_BufferedInputStream(BufferedInputStream* s, Borrow(void*) inputStream,
                              size_t (*fetch)(const void* stream, uint8_t* buf, size_t bufSize)){
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
bool hasNextBit_BufferedInputStream(BufferedInputStream* s) {

    // Offset이 7보다 크다는 말은 다음 바이트이 첫 번째 비트를
    if (s->offset > 7) {
        s->offset = 0;
        s->pos++;
    }
    // bit를 가져오는 연산은 바이트 내에서 이루어지므로
    // pos가 유효한 바이트이면 그 오프셋은 반드시 유효한 바이트이다.
    return hasNextByte_BufferedInputStream(s);
}

/**
 * @brief
 */
bool nextBit_BufferedInputStream(BufferedInputStream* s) {
    if (!hasNextBit_BufferedInputStream(s)) {
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

bool hasNextByte_BufferedInputStream(BufferedInputStream* s) {

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
uint8_t nextByte_BufferedInputStream(BufferedInputStream* s) {

    // offset이 0이 아니면 읽던 바이트 버리고 다음 바이트 읽도록 함.
    if (s->offset != 0) {
        s->pos++;
        s->offset = 7;
    }

    if (!hasNextByte_BufferedInputStream(s)) {
        return false;
        // TODO: 예외처리
    }

    uint8_t byte = s->buf[s->pos];

    s->pos++;
    s->totalReadBytes++;
    return byte;
}

void nextData_BufferedInputStream(BufferedInputStream* s, uint8_t* ptr, size_t size){
    for(size_t i = 0; i < size; i++){
        if(!hasNextByte_BufferedInputStream(s)){
            return;
        }
        if(isLittleEndian()){
            ptr[size - i - 1] = nextByte_BufferedInputStream(s);
        }else{
            ptr[i] = nextByte_BufferedInputStream(s);
        }
    }
}

size_t totalReadSize_BufferedInputStream(BufferedInputStream* s) { return s->totalReadBytes; }

void destroy_BufferedInputStream(Move(BufferedInputStream*) s){
    if(s != NULL){
        free(s);
    }
}

struct BufferedOutputStream {
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

};

Move(BufferedOutputStream*) create_BufferedOutputStream( Borrow(void*) outputStream,
                              size_t (*flush)(const void* stream, uint8_t* buf, size_t offset,
                                size_t bufSize)) {
    BufferedOutputStream* s = (BufferedOutputStream*)malloc(sizeof(BufferedOutputStream));
    reset_BufferedOutputStream(s, outputStream, flush);

    return s;
}

void reset_BufferedOutputStream(BufferedOutputStream* s, Borrow(void*) outputStream,
                              size_t (*flush)(const void* stream, uint8_t* buf, size_t offset,
                                size_t bufSize)){

    s->outputStream = (void*)outputStream;
    s->flush = flush;

    s->pos = 0;
    s->offset = 0;
    s->totalWritedBytes = 0;

}

/**
 * @brief Write a bit to the end of the stream.
 */
void writeBit_BufferedOutputStream(BufferedOutputStream* s, bool bit) {

    if (s->pos > STREAM_BUFFER_CAPACITY - 1) {
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
void packAndWriteByte_BufferedOutputStream(BufferedOutputStream* s, uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        uint8_t mask = 1 << (8 - s->offset - 1);
        bool bit = (byte & mask) != 0;
        writeBit_BufferedOutputStream(s, bit);
    }
}

void writeByte_BufferedOutputStream(BufferedOutputStream* s, uint8_t byte) {

    if (s->offset != 0) {
        s->pos++;
        s->offset = 0;
    }

    if (s->pos > STREAM_BUFFER_CAPACITY - 1) {
        s->flush(s->outputStream, s->buf, 0, s->pos);
        // TODO: 예외처리
        s->pos = 0;
    }

    s->buf[s->pos] = byte;
    s->pos++;
}


void writeData_BufferedOutputStream(BufferedOutputStream *s, uint8_t* ptr, size_t size){
    for(size_t i = 0; i < size; i++){
        if(isLittleEndian()){
            writeByte_BufferedOutputStream(s,ptr[size - i - 1] );
        }else{
            writeByte_BufferedOutputStream(s, ptr[i]);
        }
    }
}

size_t flush_BufferedOutputStream(BufferedOutputStream* s) {
    while (s->offset != 0) {
        writeBit_BufferedOutputStream(s, 0);
    }

    return s->flush(s->outputStream, s->buf, 0, s->pos);
}

size_t totalWritedSize_BufferedOutputStream(BufferedOutputStream* s) { return s->totalWritedBytes; }
void destroy_BufferedOutputStream(Move(BufferedOutputStream*) s){
    if(s != NULL){
        free(s);
    }
}
