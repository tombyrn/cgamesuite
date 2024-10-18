#include "heap.h"

struct heap* create_heap(int capacity, bool isMin) {
    struct heap* h = calloc(1, sizeof(struct heap));

    if(isMin)
        h->min = true;
    else
        h->min = false;

    struct item* heap_data = calloc(capacity, sizeof(struct item));
    h->data = heap_data;

    h->capacity = capacity;
    h->size = 0;

    return h;
}

void heapify(struct heap* h, int n, int i) {
    int parent_index = (i-1) / 2;
    if(parent_index >= 0) {

        // MIN HEAP
        if(h->min) {
            if(h->data[i].value < h->data[parent_index].value) {
                struct item tmp = h->data[i];
                h->data[i] = h->data[parent_index];
                h->data[parent_index] = tmp;
                heapify(h, n, parent_index);
            }
        }
        // MAX HEAP
        else {
            if(h->data[i].value > h->data[parent_index].value) {
                struct item tmp = h->data[i];
                h->data[i] = h->data[parent_index];
                h->data[parent_index] = tmp;
                heapify(h, n, parent_index);
            }
        }
    }
}

void heap_insert(struct heap* h, void* data, int priority) {
    // if past capacity automatically increase heap size
    if(h->size >= h->capacity) {
        h->capacity *= 2;
        h->data = realloc(h->data, h->capacity);
    }

    h->size++;


    h->data[h->size-1].data = data;
    h->data[h->size-1].value = priority;

    heapify(h, h->size, h->size - 1);

}

void heap_remove(struct heap* h, void* data, int priority) {
    
}

void* pop(struct heap* h) {
    return h->data[0].data;
}

void print_heap(struct heap* h) {
    printf("[");
    for(int i = 0; i < h->size; i++) {
        printf("%d ", h->data[i].value);
    }
    printf("]\n");
    printf("Used %d of %d\n", h->size, h->capacity);
}

void destory_heap(struct heap* h) {
    free(h->data);
    free(h);
}