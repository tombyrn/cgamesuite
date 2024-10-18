#include "heap.h"

int main() {

    struct heap* h = create_heap(10, true);

    int data = 1000;
    heap_insert(h, (void*)&data, 50);
    heap_insert(h, (void*)&data, 10);
    heap_insert(h, (void*)&data, 1);
    heap_insert(h, (void*)&data, 5);
    heap_insert(h, (void*)&data, 25);
    heap_insert(h, (void*)&data, 75);
    heap_insert(h, (void*)&data, 100);
    print_heap(h);

    printf("Popped %d from %d\n", *(int*)pop(h), h->data[0].value);

    destory_heap(h);

    h = create_heap(10, false);

    data = 50;
    heap_insert(h, (void*)&data, 50);
    heap_insert(h, (void*)&data, 10);
    heap_insert(h, (void*)&data, 1);
    heap_insert(h, (void*)&data, 5);
    heap_insert(h, (void*)&data, 25);
    heap_insert(h, (void*)&data, 75);
    heap_insert(h, (void*)&data, 100);
    print_heap(h);

    printf("Popped %d from %d\n", *(int*)pop(h), h->data[0].value);

    destory_heap(h);
   
    return 0;
}