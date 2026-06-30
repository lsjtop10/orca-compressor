#include <_mingw_mac.h>
#include <stdint.h>
#include <stdio.h>

#include "buffered-stream.h"
#include "encoder.h"
#include "huffman.h"

void encodeStream_StreamEncoder(StreamEncoder* se, Borrow(HuffmanCodeTable*) ct) {

    // 더 이상 가져올 바이트가 없을 때까지
    while (hasNextByte_BufferedInputStream(se->bis)) {

        // 0부터 size - 1까지 buf를 순회하면서
        uint8_t symbol = nextByte_BufferedInputStream(se->bis);
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

static void _serializeTree_HuffmanTreeSirializer(HuffmanTreeSirializer* s,
                                                 Borrow(HuffmanTreeNode*) htRoot) {
    int16_t val = 0;

    if (htRoot == NULL) {
        val = -2;
        putchar('#');
        writeData_BufferedOutputStream(s->bos, (uint8_t*)&val, sizeof(int16_t));
        return;
    }

    val = htRoot->value;
    writeData_BufferedOutputStream(s->bos, (uint8_t*)&val, sizeof(int16_t));
    if(val == -1){putchar('0');}
    else{putchar(val);}

    _serializeTree_HuffmanTreeSirializer(s, htRoot->left);
    _serializeTree_HuffmanTreeSirializer(s, htRoot->right);
}

void serializeTree_HuffmanTreeSirializer(HuffmanTreeSirializer* s,
                                         Borrow(HuffmanTreeNode*) htRoot) {
    _serializeTree_HuffmanTreeSirializer(s, htRoot);
    flush_BufferedOutputStream(s->bos);
}