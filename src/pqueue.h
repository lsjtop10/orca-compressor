#ifndef HEAP_H_INCLUDED
#define HEAP_H_INCLUDED

#include <stddef.h>
#include <stdbool.h>

#define MAX_HEAP_SIZE 260

typedef size_t Datum;

typedef void* HeapElement;
typedef struct PQueue{
    HeapElement data[MAX_HEAP_SIZE];
    int size;

    //첫 번째 원소의 우선순위가 높으면 true 반환
    bool (*hasHigherPriouty)(HeapElement first, HeapElement second);
} PQueue;

void init_PQueue(PQueue* h, bool (*hasHigherPriouty)(void* a, void* b));
void insert_PQueue(PQueue* h, HeapElement node);

HeapElement delete_PQueue(PQueue* h);
void print__PQueue(PQueue* h);

int getSize_PQueue(PQueue* h);
void* getData_PQueue(PQueue* h);

#endif // HEAP_H_INCLUDED
