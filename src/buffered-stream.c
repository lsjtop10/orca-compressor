
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "utils.h"
#include "buffered-stream.h"
#include "ownership.h"




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

    size_t (*fetch)(const void* stream, uint8_t* buf, size_t bufSize, ErrorContext* err);
};

Move(BufferedInputStream*)
    create_BufferedInputStream(Borrow(void*) inputStream,
                               size_t (*fetch)(const void* stream, uint8_t* buf, size_t bufSize,
                                               ErrorContext* err)) {
    BufferedInputStream* s = malloc(sizeof(BufferedInputStream));

    reset_BufferedInputStream(s, inputStream, fetch);
    return s;
}

void reset_BufferedInputStream(BufferedInputStream* s, Borrow(void*) inputStream,
                               size_t (*fetch)(const void* stream, uint8_t* buf, size_t bufSize,
                                               ErrorContext* err)) {
    s->inputStream = (void*)inputStream;
    s->fetch = fetch;

    s->bufSize = 0;
    s->offset = 0;
    s->pos = 0;
    s->totalReadBytes = 0;
}

static bool hasNextByte_BufferedInputStream(BufferedInputStream* s, ErrorContext* err) {
    if (peekSurfaceError_ErrorContext(err) != NULL) {
        return false;
    }

    // 일단 가리키는 idx가 작기만 하면 true
    if (s->pos < s->bufSize) {
        return true;
    }

    s->bufSize = s->fetch(s->inputStream, s->buf, STREAM_BUFFER_CAPACITY,err);

    //  fetch는 성공했으나 읽어온 데이터가 0인 경우 (정상 스트림 종료)
    // 혹은 이미 하위 fetch에서 EOS 상태를 꽂아주었다면 그것도 포함
    if (s->bufSize == 0 && isSurfaceCode_ErrorContext(err, HF_STATE_END_OF_STREAM)) {
        return false;
    }

    //  fetch 내부에서 진짜 치명적인 에러가 터진 경우
    if (peekSurfaceError_ErrorContext(err) != NULL) {
        // 이미 err 내부에 하위 에러가 있겠지만, 내 계층의 맥락을 상위로 전파(Propagate)
        append_ErrorContext(err, HF_ERR_STREAM_FETCH_FAILED,
                            "Failed to fetch data from the stream.");
        return false;
    }

    if (s->bufSize > 0) {
        s->offset = 0;
        s->pos = 0;
        return true;
    }

    return false;
}

/**
 * @brief Checks if current position is valid and current offset is valid. If not, fetches new data
 * from the stream.
 *
 */
static bool hasNextBit_BufferedInputStream(BufferedInputStream* s, ErrorContext* err) {

    // Offset이 7보다 크다는 말은 다음 바이트이 첫 번째 비트를 가맄키라는 말
    if (s->offset > 7) {
        s->offset = 0;
        s->pos++;
    }

    // bit를 가져오는 연산은 바이트 내에서 이루어지므로
    // pos가 유효한 바이트이면 그 오프셋은 반드시 유효한 바이트이다.
    return hasNextByte_BufferedInputStream(s, err);
}

bool tryNextBit_BufferedInputStream(BufferedInputStream* s, bool* bit, ErrorContext* err) {
    if (!hasNextBit_BufferedInputStream(s, err)) {
        return false;
    }

    // 가져오기 전 offset 범위 체크. 다음 bit가 무조건 있고 가정했기 때문에 그냥 올려도 무방함.
    uint8_t mask = 1 << (8 - s->offset - 1);
    *bit = (s->buf[s->pos] & mask) != 0;

    s->offset++;
    if (s->offset > 7) {
        s->offset = 0;
        s->pos++;
        s->totalReadBytes++;
    }

    return true;
}

bool tryNextByte_BufferedInputStream(BufferedInputStream* s, uint8_t* byte, ErrorContext* err) {
    if (!hasNextByte_BufferedInputStream(s, err)) {
        return false;
    }

    // offset이 0이 아니면 읽던 바이트 버리고 다음 바이트 읽도록 함.
    if (s->offset != 0) {
        s->pos++;
        s->offset = 7;
    }


    *byte = s->buf[s->pos];

    s->pos++;
    s->totalReadBytes++;

    return true;
}


bool tryNextData_BufferedInputStream(BufferedInputStream* s, void* ptr, size_t size,
                                     ErrorContext* err) {
    for (size_t i = 0; i < size; i++) {
        
        if (isLittleEndian()) {
            if (!tryNextByte_BufferedInputStream(s, &((uint8_t*)ptr)[size - i - 1], err)) {
                goto err_tryNextData_BufferedInputStream;
            }
        } else {
            if (!tryNextByte_BufferedInputStream(s, &((uint8_t*)ptr)[i], err)) {
                goto err_tryNextData_BufferedInputStream;
            }
        }
    }
    return true;

err_tryNextData_BufferedInputStream:
    if(isSurfaceCode_ErrorContext(err, HF_STATE_END_OF_STREAM)){
        return false;
    }
    if(peekSurfaceError_ErrorContext(err) != NULL){
        append_ErrorContext(err, HF_ERR_STREAM_FETCH_FAILED, "Failed to fetch data from the stream.");
        return false;
    }
    return false;
}

size_t totalReadSize_BufferedInputStream(BufferedInputStream* s) { return s->totalReadBytes; }

void destroy_BufferedInputStream(Move(BufferedInputStream*) s) {
    if (s != NULL) {
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
    size_t (*flush)(const void* stream, uint8_t* buf, size_t offset, size_t bufSize,
                    ErrorContext* err);
};

Move(BufferedOutputStream*)
    create_BufferedOutputStream(Borrow(void*) outputStream,
                                size_t (*flush)(const void* stream, uint8_t* buf, size_t offset,
                                                size_t bufSize, ErrorContext* err)) {
    BufferedOutputStream* s = (BufferedOutputStream*)malloc(sizeof(BufferedOutputStream));

    reset_BufferedOutputStream(s, outputStream, flush);

    return s;
}

void reset_BufferedOutputStream(BufferedOutputStream* s, Borrow(void*) outputStream,
                                size_t (*flush)(const void* stream, uint8_t* buf, size_t offset,
                                                size_t bufSize, ErrorContext* err)) {

    s->outputStream = (void*)outputStream;
    s->flush = flush;

    s->pos = 0;
    s->offset = 0;
    s->totalWritedBytes = 0;
}

/**
 * @brief Write a bit to the end of the stream.
 */
bool tryWriteBit_BufferedOutputStream(BufferedOutputStream* s, bool bit, ErrorContext* err) {

    if (s->pos > STREAM_BUFFER_CAPACITY - 1) {
        s->flush(s->outputStream, s->buf, 0, STREAM_BUFFER_CAPACITY, err);
        if (peekSurfaceError_ErrorContext(err) != NULL) {
            goto err_flush;
        }

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
    return true;

err_flush:
    if (isSurfaceCode_ErrorContext(err, HF_STATE_END_OF_STREAM)) {
        return false;
    }

    append_ErrorContext(err, HF_ERR_STREAM_FLUSH_FAILED,
                        "Failed to flush data to the stream.");
    return false;


}

/**
 * @brief Write a byte to the stream sequentially from the current bit position.
 */
bool tryPackAndWriteByte_BufferedOutputStream(BufferedOutputStream* s, uint8_t byte,
                                              ErrorContext* err) {
    for (int i = 0; i < 8; i++) {
        uint8_t mask = 1 << (8 - s->offset - 1);
        bool bit = (byte & mask) != 0;
        if (!tryWriteBit_BufferedOutputStream(s, bit, err)) {
            append_ErrorContext(err, HF_ERR_STREAM_FLUSH_FAILED,
                                "Failed to write a bit to the stream.");
            return false;
        }
    }
    return true;
}

bool tryWriteByte_BufferedOutputStream(BufferedOutputStream* s, uint8_t byte, ErrorContext* err) {

    if (s->offset != 0) {
        s->pos++;
        s->offset = 0;
    }

    if (s->pos > STREAM_BUFFER_CAPACITY - 1) {
        s->flush(s->outputStream, s->buf, 0, s->pos, err);
        if(s->flush == 0)
        if (peekSurfaceError_ErrorContext(err) != NULL) {
            goto err_flush;
        }
        s->pos = 0;
    }

    s->buf[s->pos] = byte;
    s->pos++;
    return true;
    
err_flush:
    if (isSurfaceCode_ErrorContext(err, HF_STATE_END_OF_STREAM)) {
        return false;
    }

    append_ErrorContext(err, HF_ERR_STREAM_FLUSH_FAILED,
                            "Failed to flush data to the stream.");
    return false;

}

bool tryWriteData_BufferedOutputStream(BufferedOutputStream* s, uint8_t* ptr, size_t size,
                                       ErrorContext* err) {
    for (size_t i = 0; i < size; i++) {
        if (isLittleEndian()) {
            if (!tryWriteByte_BufferedOutputStream(s, ptr[size - i - 1], err)) {
                goto err_write_byte;
            }
        } else {
            if (!tryWriteByte_BufferedOutputStream(s, ptr[i], err)) {
                goto err_write_byte;
            }
        }
    }
    return true;

err_write_byte:
    if (isSurfaceCode_ErrorContext(err, HF_STATE_END_OF_STREAM)) {
        return false;
    }else {
        append_ErrorContext(err, HF_ERR_STREAM_FLUSH_FAILED,
                            "Failed to flush data to the stream.");
        return false;
    }
    return false;
}

size_t flush_BufferedOutputStream(BufferedOutputStream* s, ErrorContext* err) {
    while (s->offset != 0) {
        if (!tryWriteBit_BufferedOutputStream(s, 0, err)) {
            // Handle error
        }
    }

    return s->flush(s->outputStream, s->buf, 0, s->pos, err);
}

size_t totalWritedSize_BufferedOutputStream(BufferedOutputStream* s) { return s->totalWritedBytes; }
void destroy_BufferedOutputStream(Move(BufferedOutputStream*) s) {
    if (s != NULL) {
        free(s);
    }
}
