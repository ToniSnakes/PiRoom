#include "vec.h"
#include <stdlib.h>
#include <string.h>

struct vec_struct {
    size_t itemSize;
    unsigned int capacity;
    unsigned int count;
    void* data;
};

vec_t* vec_init(size_t itemSize, unsigned int capacity)
{
    vec_t* vec = malloc(sizeof(vec_t));
    *vec = (vec_t){
        .itemSize = itemSize + 1,
        .capacity = capacity,
        .count = 0,
        .data = calloc(capacity, itemSize)
    };
    return vec;
}

void vec_free(vec_t** vec)
{
    free((*vec)->data);
    free(*vec);
    *vec = NULL;
}

void vec_checkEnlarge(vec_t* vec)
{
    if (vec->count + 1 > vec->capacity) {
        if (vec->capacity == 0) {
            vec->capacity = 4;
            vec->data = calloc(4, vec->itemSize);
        } else {
            vec->data = realloc(vec->data, vec->capacity * 2 * vec->itemSize);
            memset(vec->data + vec->capacity * vec->itemSize, 0, vec->capacity * vec->itemSize);
            vec->capacity *= 2;
        }
    }
}

void* vec_allocItem(vec_t* vec)
{
    vec_checkEnlarge(vec);

    vec->count++;

    for (int i = 0; i < vec->count; i++) {
        char* available = (char*)(vec->data + i * vec->itemSize);
        if (*available == 0) {
            *available = 1;
            return available + 1;
        }
    }
}

void vec_freeItem(vec_t* vec, void* item)
{
    *((char*)item - 1) = 0;
    vec->count--;
}