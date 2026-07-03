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
                append_ErrorContext(err, HF_ERR_STREAM_WRITE_FAILED, "encode stream: failed to write bit");
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
        append_ErrorContext(err, HF_ERR_STREAM_READ_FAILED, "encode stream: failed to read input byte");
        return;
    }
    
    // 남은 output 버퍼를 flush한다.
    // 불완전한 바이트를 0으로 패킹해 완전한 바이트로 만들었으므로 size ++
    if(flush_BufferedOutputStream(se->bos, err) == 0){
        append_ErrorContext(err, HF_ERR_STREAM_FLUSH_FAILED, "encode stream: failed to flush output stream");
        return;
    }
}

void countFreqFromStream_StreamEncoder(BufferedInputStream* bis, Mut(FreqTable*) t, ErrorContext* err) {
    if (peekSurfaceError_ErrorContext(err) != NULL) {
        return;
    }

    for (uint8_t symbol; tryNextByte_BufferedInputStream(bis, &symbol, err);) {
        t->freq[symbol]++;
    }

    while (isSurfaceCode_ErrorContext(err, HF_STATE_END_OF_STREAM)) {
        consumeSurfaceError_ErrorContext(err);
    }
}