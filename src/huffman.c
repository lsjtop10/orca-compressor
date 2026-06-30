#include <corecrt_memory.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "huffman.h"
#include "ownership.h"
#include "pqueue.h"

static void increseFreq_FreqTable(FreqTable* t, uint8_t b);

// first가 second보다 작은 freq값을 지니면 true 
static bool hasLessFreq(void* first, void* second){
    return ((HuffmanTreeNode*)first)->freq < ((HuffmanTreeNode*)second)->freq;
}


static void increseFreq_FreqTable(FreqTable* t, uint8_t b) { t->freq[b]++; }

void init_FreqTable(FreqTable* t) { memset(t->freq, 0, sizeof(uint64_t) * MAX_SYMBOL_SIZE); }

void count_FreqTable(FreqTable* t, Borrow(uint8_t*) str, size_t strSize) {
    for (size_t i = 0; i < strSize; i++) {
        increseFreq_FreqTable(t, str[i]);
    }
}

// TODO: 정수 오버플로우 방지용 연산 필요
// 자료형을 UINT_64로 매우 크게 잡았지만
// 트리 빌드 과정에서 합산되기 때문에 오버플로우 발생할 가능성 있음.
uint64_t lookupFreq_FreqTable(FreqTable* t, uint8_t c) { return t->freq[c]; }


HuffmanTreeNode* crate_HuffmanTreeNode(int freq, int16_t value) {
    HuffmanTreeNode* n = (HuffmanTreeNode*)malloc(sizeof(HuffmanTreeNode));

    n->freq = freq;
    n->value = value;
    n->left = NULL;
    n->right = NULL;

    return n;
}

HuffmanTreeNode* build_HuffmanTree(Borrow(FreqTable*) t) {
    PQueue h;
    init_PQueue(&h, hasLessFreq);

    for (int i = 0; i < MAX_SYMBOL_SIZE; i++) {
        uint64_t freq = lookupFreq_FreqTable((FreqTable*)t, i);

        if (freq != 0) {
            insert_PQueue(&h, crate_HuffmanTreeNode(freq, i));
        }
    }

    while (h.size != 1) {
        HuffmanTreeNode* n1 = delete_PQueue(&h);
        HuffmanTreeNode* n2 = delete_PQueue(&h);

        HuffmanTreeNode* newNode = crate_HuffmanTreeNode(n1->freq + n2->freq, -1);
        newNode->left = n1;
        newNode->right = n2;

        insert_PQueue(&h, newNode);
    }

    HuffmanTreeNode* root = delete_PQueue(&h);
    // _debug_dump_tree(root, 0);
    return root;
}


bool getBit_HuffmanCode(HuffmanCode code, int at) { 
    int idx = at / 8;
    int offset = at % 8;

    return (code.bytes[idx] & (1 << (8 - offset - 1))) != 0; 
}

void setBit_HuffmanCode(HuffmanCode* code, int at, bool v) {

    int idx = at / 8;
    int offset = at % 8;

    if (v) {
        code->bytes[idx] = code->bytes[idx] | (1 << (8 - offset - 1));
    } else {
        code->bytes[idx] = code->bytes[idx] & ~(1 << (8 - offset - 1));
    }
}

void print_HuffmanCode(HuffmanCode code, size_t length){
    for(size_t i = 0; i < length; i++){
        printf("%d", (int)getBit_HuffmanCode(code, i));
    }
}


void init_HuffmanCodeTable(Mut(HuffmanCodeTable*) t) {
    memset(t->code, 0, sizeof(uint16_t) * MAX_SYMBOL_SIZE);
    memset(t->length, 0, sizeof(size_t) * MAX_SYMBOL_SIZE);
}

HuffmanCode lookupCode_HuffmanCodeTable(HuffmanCodeTable* t, uint8_t c) { return t->code[(int)c]; }
size_t lookupCodeLength_HuffmanCodeTable(HuffmanCodeTable* t, uint8_t c) { return t->length[(int)c]; }

void setCode_HuffmanCodeTable(HuffmanCodeTable* t, uint8_t c, HuffmanCode code, size_t length) {
    t->code[c] = code;
    t->length[c] = length;
}

void buildTable_HuffmanCodeTable(HuffmanCodeTable* t, Borrow(HuffmanTreeNode*) hTreeRoot, HuffmanCode code, size_t codeLength) {
    if (hTreeRoot == NULL) {
        return;
    }

    if (hTreeRoot->left == NULL && hTreeRoot->right == NULL) {
        t->code[hTreeRoot->value] = code;
        t->length[hTreeRoot->value] = codeLength;
        return; // 코드를 저장했으므로 더 이상 아래로 내려가지 않고 종료합니다.
    }

    // setbit가 포인터 넘기므로 값을 복사해 줘야 한다.
    HuffmanCode codeL = code;
    setBit_HuffmanCode(&codeL, codeLength, false);
    HuffmanCode codeR = code;
    setBit_HuffmanCode(&codeR, codeLength, true);

    if (hTreeRoot->left != NULL) {
        buildTable_HuffmanCodeTable(t, hTreeRoot->left, codeL, codeLength + 1);
    }

    if (hTreeRoot->right != NULL) {
        buildTable_HuffmanCodeTable(t, hTreeRoot->right, codeR, codeLength + 1);
    }
}


// 💡 [최종 합: 트리가 진짜 똑바로 생겼는지 숲을 보는 덤프 함수
void _debug_dump_tree(Borrow(HuffmanTreeNode*) node, int depth) {
    if (node == NULL) return;

    // 가독성을 위해 깊이만큼 들여쓰기 출력
    for (int i = 0; i < depth; i++) printf("  ");

    if (node->left == NULL && node->right == NULL) {
        // 리프 노드인 경우 심볼(char) 출력
        printf("└── [Leaf] '%c' (freq: %llu)\n", (char)node->value, node->freq);
    } else {
        // 내부 노드인 경우 빈도수만 출력
        printf("├── [Internal] (combined freq: %llu)\n", node->freq);
    }

    _debug_dump_tree(node->left, depth + 1);
    _debug_dump_tree(node->right, depth + 1);
}

void preOrder_HuffmanTree(HuffmanTreeNode* root) {
    if (root == NULL) {
        return;
    }

    printf("[freq:%llu char:%c] \n", root->freq, root->value == '\0' ? '.' : root->value);
    preOrder_HuffmanTree(root->left);
    preOrder_HuffmanTree(root->right);
}