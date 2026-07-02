#include <stdint.h>
#include <stdio.h>

#include "encoder.h"
#include "huffman.h"


#include "buffered-stream.h"

void encodeStream_StreamEncoder(StreamEncoder* se, Borrow(HuffmanCodeTable*) ct,
                                ErrorContext* err) {

    uint8_t symbol;

    // 더 이상 가져올 바이트가 없을 때까지
    while (tryNextByte_BufferedInputStream(se->bis, &symbol, err)) {

        // 0부터 size - 1까지 buf를 순회하면서
        HuffmanCode code = lookupCode_HuffmanCodeTable((HuffmanCodeTable*)ct, symbol);
        size_t length = lookupCodeLength_HuffmanCodeTable((HuffmanCodeTable*)ct, symbol);

        for (size_t j = 0; j < length; j++) {
            bool bit = getBit_HuffmanCode(code, j);
            if (!tryWriteBit_BufferedOutputStream(se->bos, bit, err)) {
                append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "Error: Failed to write bit to output stream");
                return;
            }
        }
    }

    // 남아있는 EOF는 호출자 입장에서 관심사가 아니므로 consume.
    while(isSurfaceCode_ErrorContext(err, HF_STATE_END_OF_STREAM)) {
        consumeSurfaceError_ErrorContext(err);
    }

    // 다른 에러가 발생했으면 에러 래핑.
    if (peekSurfaceError_ErrorContext(err) != NULL) {
        append_ErrorContext(err, HF_ERR_STREAM_READ_FAILED, "Error: Failed to read byte from input stream");
        return;
    }
    
    // 남은 output 버퍼를 flush한다.
    // 불완전한 바이트를 0으로 패킹해 완전한 바이트로 만들었으므로 size ++
    if(flush_BufferedOutputStream(se->bos, err) == 0){
        append_ErrorContext(err, HF_ERR_STREAM_FLUSH_FAILED, "Error: Failed to flush output stream");
        return;
    }
}

// NOLINTNEXTLINE(misc-no-recursion)
static void serializeTreeImpl_HuffmanTreeSirializer(HuffmanTreeSirializer* s,
                                                    Borrow(HuffmanTreeNode*) htRoot, ErrorContext* err) {
    int16_t val = 0;

    if (htRoot == NULL) {
        val = -2;
        if (!tryWriteData_BufferedOutputStream(s->bos, (uint8_t*)&val, sizeof(int16_t), err)) {
            append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "Error: Failed to write data to output stream");
            return;
        }
        return;
    }

    val = htRoot->value;
    if (!tryWriteData_BufferedOutputStream(s->bos, (uint8_t*)&val, sizeof(int16_t), err)) {
        append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "Error: Failed to write data to output stream");
        return;
    }
    if (val == -1) {
        putchar('0');
    } else {
        putchar(val);
    }

    serializeTreeImpl_HuffmanTreeSirializer(s, htRoot->left, err);
    serializeTreeImpl_HuffmanTreeSirializer(s, htRoot->right, err);
}

void serializeTree_HuffmanTreeSirializer(HuffmanTreeSirializer* s,
                                         Borrow(HuffmanTreeNode*) htRoot, ErrorContext* err) {
    serializeTreeImpl_HuffmanTreeSirializer(s, htRoot, err);
    if(flush_BufferedOutputStream(s->bos, err) == 0){
        append_ErrorContext(err, HF_ERR_STREAM_FLUSH_FAILED, "Error: Failed to flush output stream");
        return;
    }
}