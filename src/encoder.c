#include <stdint.h>

#include "encoder.h"
#include "buffered-stream.h"
#include "huffman.h"


void encodeStream_StreamEncoder(StreamEncoder* se, Borrow(HuffmanCodeTable*) ct) {

    // 더 이상 가져올 바이트가 없을 때까지
    while (hasNextByte_BufferdInputStream(se->bis)) {

        // 0부터 size - 1까지 buf를 순회하면서
        uint8_t symbol = nextByte_BufferdInputStream(se->bis);
        HuffmanCode code = lookupCode_HuffmanCodeTable((HuffmanCodeTable*)ct, symbol);
        size_t length = lookupCodeLength_HuffmanCodeTable((HuffmanCodeTable*)ct, symbol);

        for (size_t j = 0; j < length; j++) {
            
            bool bit = getBit_HuffmanCode(code, j);
            writeBit_BufferedOutputStream(se->bos, bit);
        }
    
    }
    // 남은 output 버퍼를 flush한다.
    // 불완전한 바이트를 0으로 패킹해 완전한 바이트로 만들었으므로 size ++

    flush_BufferedOutputStream(se->bos);
}