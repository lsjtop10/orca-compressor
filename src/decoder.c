#include "Decoder.h"
#include "buffered-stream.h"
#include "huffman.h"
#include <stdint.h>

// 비트 패킹 중 남는 비트는 허프만 코드 길이보다 항상 작거나 같다.
// 허프만 코드 길이는 최대 심볼수까지 나올 수 있으므로 최소 256비트보다는 커야
// 한다. 넉넉잡아 16바이트 = 64비트 정도만 할당해주자.


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
    bool bit = nextBit_BufferedInputStream(sd->bis);
    // TODO: 에러 전파 코드 필요.
    if (!hasNext) {
        return 0;
    }

    if (htRoot->left != NULL && bit == false) {
        return findNextSymbol(sd, htRoot->left);
    } else if (htRoot->right != NULL && bit == true) {
        return findNextSymbol(sd, htRoot->right);
    } else {
        // 만약에 NextBit가 가라고 하는 방향에 노드가 없으면 에러.
        return 0;
    }
}

void decodeStream_StreamDecoder(StreamDecoder* sd, Borrow(HuffmanTreeNode*) htRoot) {

    for(int i = 0; i < sd->numOriginalBytes; i++){
        uint8_t symbol = findNextSymbol(sd, htRoot);
        writeByte_BufferedOutputStream(sd->bos, symbol);
    }

    flush_BufferedOutputStream(sd->bos);
}

HuffmanTreeNode* deserializeTree_HuffmanTreeSirializer(HuffmanTreeDeSirializer* s){
    int16_t val = 0;
    nextData_BufferedInputStream(s->bis,(uint8_t*)&val, sizeof(uint16_t));

    if(val == -2){return NULL;}
    HuffmanTreeNode* n = crate_HuffmanTreeNode(0, val);
    
    n->left = deserializeTree_HuffmanTreeSirializer(s);
    n->right = deserializeTree_HuffmanTreeSirializer(s); 

    return n;
}