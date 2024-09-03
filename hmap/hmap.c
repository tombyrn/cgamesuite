#include "hmap.h"


// hash function for a string key
int hash(char *key) {
    int hash = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        hash += (int)key[i];
    }
    return hash % TABLE_SIZE;
}

// insert value into hashmap
// inserting the same key will overwrite values
void insert(struct hmap* hm, char *key, void* value) {
    int h = hash(key);
    struct slot* insert_slot = &(hm->table[h]);

    // collision found when insertion slot already exists and has a different key
    while(insert_slot && insert_slot->key && strcmp(insert_slot->key, key) != 0) {
        struct slot* prev_slot = insert_slot;
        insert_slot = insert_slot->next;

        // allocate new slot if needed
        if(insert_slot == NULL) {
            insert_slot = calloc(1, sizeof(struct slot));

            // connect linked list
            prev_slot->next = insert_slot;
        }
    }

    // insert or overwrite hashmap value
    insert_slot->key = key;
    insert_slot->value = value;
    insert_slot->next = NULL;
}

// get value from hashmap
void* get(struct hmap* hm, char* key) {
    void* value = NULL;
    int h = hash(key);

    struct slot* retrieval_slot = &(hm->table[h]);

    while(strcmp(retrieval_slot->key, key) != 0 && retrieval_slot->next)
        retrieval_slot = retrieval_slot->next;

    value = retrieval_slot->value;

    return value;
}

// print all hashmap values
void print_hmap(struct hmap* hm, bool abbreviate) {
    for(int i = 0; i < hm->table_size; i++) {
        struct slot* curr = &(hm->table[i]);
        if(abbreviate && !curr->key) continue;

        printf("slot %d:\n", i);
        while(curr != NULL) {
            printf("\tkey:%s\tvalue: %d\n", curr->key, curr->value ? 1 : 0);
            curr = curr->next;
        }
    }
}

// initialize hashmap struct
void hmap_init(struct hmap* hm) {
    hm->table_size = TABLE_SIZE;
    hm->table = calloc(sizeof(struct slot), TABLE_SIZE);
}