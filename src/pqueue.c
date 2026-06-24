#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "pqueue.h"

// 재정의.

void init_PQueue(PQueue* h, bool (*hasHigherPriouty)(void* a, void* b)) {
    for (int i = 0; i < MAX_HEAP_SIZE; i++) {
        h->data[i] = NULL;
    }

    h->size = 0;
    h->hasHigherPriouty = hasHigherPriouty;
}

static bool isFull_Heap(PQueue* h) { return h->size >= MAX_HEAP_SIZE; }

static bool isEmpty_Heap(PQueue* h) { return h->size == 0; }

static void upHeap_Heap(PQueue* h) {
    int idx = h->size;
    HeapElement tmp = h->data[idx];

    // 자기가 더 큰 우선순위를 지니면
    while ((idx != 1) && (h->hasHigherPriouty(h->data[idx / 2], h->data[idx]))) {
        h->data[idx] = h->data[idx / 2];
        idx = idx / 2;
    }

    h->data[idx] = tmp;
}

void insert_PQueue(PQueue* h, HeapElement node) {
    if (isFull_Heap(h)) {
        printf("Heap Overflow");
        return;
    }

    h->size++;
    h->data[h->size] = node;
    upHeap_Heap(h);
}

static void downHeap_Heap(PQueue* h) {
    int parent = 1;
    int maxChild = 2;
    HeapElement tmp = h->data[parent];

    // Paerent기반이 아니라 child 기반으로 범위 검사해야 함.ㅠㅠ
    while ((maxChild <= h->size)) {
        if ((maxChild < h->size) && h->hasHigherPriouty(h->data[maxChild + 1], h->data[maxChild])) {
            maxChild = maxChild + 1;
        }

        if (h->hasHigherPriouty(tmp, h->data[maxChild])) {
            break;
        }

        h->data[parent] = h->data[maxChild];

        parent = maxChild;
        maxChild = parent * 2;
    }

    h->data[parent] = tmp;
}

HeapElement delete_PQueue(PQueue* h) {
    if (isEmpty_Heap(h)) {
        printf("heap is empty");
        return NULL;
    }

    HeapElement root = h->data[1];
    h->data[1] = h->data[h->size];
    h->size--;

    downHeap_Heap(h);
    return root;
}