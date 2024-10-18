#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct item {
    int value;
    void* data;
};

struct heap {
    bool min;
    struct item* data;
    unsigned int size;
    unsigned int capacity;
};

struct heap* create_heap(int capacity, bool isMin);
void heapify(struct heap* h, int n, int i);
void heap_insert(struct heap* h, void* data, int priority);
void* pop(struct heap* h);
void print_heap(struct heap* h);
void destory_heap(struct heap* h);