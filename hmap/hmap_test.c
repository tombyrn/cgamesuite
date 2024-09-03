#include "hmap.h"

int main() {

    struct hmap hm;
    hmap_init(&hm);

    // insert int
    char* key = "first key";
    int value = 5;
    insert(&hm, key, (void*)&value);

    // retrieve int
    int test_int_get = *(int*)get(&hm, key);
    printf("%d\n", test_int_get);

    // overwrite key with string
    char* b = "new value";
    insert(&hm, key, (void*)b);

    // retrieve string
    char* test_str_get = (char*)get(&hm, key);
    printf("%s\n", test_str_get);

    // insert another key
    char* key_two = "second key";
    insert(&hm, key_two, (void*) key);

    // test collision
    char *c = "lies";
    char *d = "foes";
    int num = 1;
    int num2 = 20;
    insert(&hm, c, &num);
    insert(&hm, d, &num2);

    // print abbreviated hmap values
    print_hmap(&hm, 1);
    return 0;
}