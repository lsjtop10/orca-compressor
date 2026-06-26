#ifndef HUFFMAN_H_INCLUDED
#define HUFFMAN_H_INCLUDED

#include <stdbool.h>

#include <stddef.h>
#include <stdint.h>
#include "ownership.h"

#define MAX_SYMBOL_SIZE 255

typedef struct FreqTable {
    uint64_t freq[MAX_SYMBOL_SIZE];
} FreqTable;

void init_FreqTable(FreqTable* t);
void count_FreqTable(FreqTable* t, Borrow(uint8_t*) str, size_t strSize);
uint64_t lookupFreq_FreqTable(FreqTable* t, uint8_t c);


typedef struct HuffmanTreeNode {
    uint64_t freq;
    int16_t value;
    
    struct HuffmanTreeNode* left;
    struct HuffmanTreeNode* right;
} HuffmanTreeNode;

HuffmanTreeNode* make_HuffmanTreeNode(int freq, int16_t value);


typedef struct HuffmanTreeType {
    HuffmanTreeNode* root;
} HuffmanTreeType;

HuffmanTreeNode* build_HuffmanTree(Borrow(FreqTable*) t);

// 최악의 경우 허프만 트리는 심볼 개수만큼 깊어질 수 있으므로
// 한 코드 length는 256bit 패턴을 담을 수 있어야 한다. 
typedef struct{uint8_t bytes[32];} HuffmanCode;

bool getBit_HuffmanCode(HuffmanCode code, int at);
void setBit_HuffmanCode(HuffmanCode* code, int at, bool v);
void print_HuffmanCode(HuffmanCode code, size_t length);

typedef struct HuffmanCodeTable {
    HuffmanCode code[MAX_SYMBOL_SIZE];
    size_t length[MAX_SYMBOL_SIZE];
} HuffmanCodeTable;

void init_HuffmanCodeTable(HuffmanCodeTable* t);
HuffmanCode lookupCode_HuffmanCodeTable(HuffmanCodeTable* t, uint8_t c);
size_t lookupCodeLength_HuffmanCodeTable(HuffmanCodeTable* t, uint8_t c);
void setCode_HuffmanCodeTable(HuffmanCodeTable* t, uint8_t c, HuffmanCode code, size_t length);
void buildTable_HuffmanCodeTable(HuffmanCodeTable* t, Borrow(HuffmanTreeNode*) hTreeRoot, HuffmanCode code, size_t codeLength);



void _debug_dump_tree(Borrow(HuffmanTreeNode*) node, int depth);
#endif // HUFFMAN_H_INCLUDED
