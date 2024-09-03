#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define TABLE_SIZE 256

struct slot {
    char* key;
    void* value;
    struct slot* next;
};

struct hmap {
    struct slot* table;
    int table_size;
};

void insert(struct hmap* hm, char* key, void* value);
void* get(struct hmap* hm, char* key);
void print_hmap(struct hmap* hm, bool abbreviate);
void hmap_init(struct hmap* hm);