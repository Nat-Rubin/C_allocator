#include <stdlib.h>

typedef struct Node_t{
    int size;
    struct Node_t *next;
    struct Node_t *prev;
    bool is_free;
} node_t;

void* alloc(size_t);
void* calloc(size_t, size_t);
void* realloc(void*, size_t);
void free(void*);
