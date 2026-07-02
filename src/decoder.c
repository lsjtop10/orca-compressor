#include <stdint.h>

#include "errors.h"

#include "huffman.h"
#include "buffered-stream.h"
#include "decoder.h"

// 비트 패킹 중 남는 비트는 허프만 코드 길이보다 항상 작거나 같다.
// 허프만 코드 길이는 최대 심볼수까지 나올 수 있으므로 최소 256비트보다는 커야
// 한다. 넉넉잡아 16바이트 = 64비트 정도만 할당해주자.

// NOLINTNEXTLINE(misc-no-recursion)
static bool tryFindNextSymbol(StreamDecoder* sd, Borrow(HuffmanTreeNode*) htRoot, Out(uint8_t*) symbol, ErrorContext* err) {

    // 0도 symbol일 수 있지만 마땅히 내보낼 값이 없다. 나중에 에러 처리 로직 도입하면
    // 에러를 반환할 예정이다.
    if (htRoot == NULL) {
        append_ErrorContext(err, HF_ERR_INVALID_ARG, "Error: htRoot is NULL");
        return false;
    }


    if (htRoot->left == NULL && htRoot->right == NULL) {
        *symbol = htRoot->value;
        return true;
    }

    bool bit;
    // TODO: 에러 전파 코드 필요.
    if (!tryNextBit_BufferedInputStream(sd->bis, &bit, err)) {
        if (isSurfaceCode_ErrorContext(err, HF_STATE_END_OF_STREAM)) {
            return false;
        }
        append_ErrorContext(err, HF_ERR_UNKOWN, "Error: failed to read next bit from input stream");
        return false;
    }

    if (htRoot->left != NULL && bit == false) {
        return tryFindNextSymbol(sd, htRoot->left, symbol, err);
    } else if (htRoot->right != NULL && bit == true) {
        return tryFindNextSymbol(sd, htRoot->right, symbol, err);
    } else {
        // 만약에 NextBit가 가라고 하는 방향에 노드가 없으면 에러.
        append_ErrorContext(err, HF_ERR_UNKOWN, "Error: No valid node found for the next bit");
        return false;
    }
}

void decodeStream_StreamDecoder(StreamDecoder* sd, Borrow(HuffmanTreeNode*) htRoot, ErrorContext* err) {

    for(uint64_t i = 0; i < sd->numOriginalBytes; i++){
        uint8_t symbol;
        if (!tryFindNextSymbol(sd, htRoot, &symbol, err)) {
            // 에러 처리
            append_ErrorContext(err, HF_ERR_UNKOWN, "Error: Failed to find next symbol");
            return;
        }
        if (!tryWriteByte_BufferedOutputStream(sd->bos, symbol, err)) {
            append_ErrorContext(err, HF_ERR_STREAM_FLUSH_FAILED, "Error: Failed to write byte to output stream");
            return;
        }
    }

    if(flush_BufferedOutputStream(sd->bos, err) == 0){
        append_ErrorContext(err, HF_ERR_STREAM_FLUSH_FAILED, "Error: Failed to flush output stream");
        return;
    }
}

// NOLINTNEXTLINE(misc-no-recursion)
HuffmanTreeNode* deserializeTree_HuffmanTreeSirializer(HuffmanTreeDeSirializer* s, ErrorContext* err) {
    int16_t val = 0;

    if(!tryNextData_BufferedInputStream(s->bis,(uint8_t*)&val, sizeof(uint16_t), err)) {
        append_ErrorContext(err, HF_ERR_STREAM_FETCH_FAILED, "Error: Failed to read next data from input stream");
        return NULL;
    }

    if(val == -2){return NULL;}
    HuffmanTreeNode* n = crate_HuffmanTreeNode(0, val);
    
    n->left = deserializeTree_HuffmanTreeSirializer(s, err);
    n->right = deserializeTree_HuffmanTreeSirializer(s, err); 

    return n;
}