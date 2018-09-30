#include <stdlib.h>

typedef struct vec_struct vec_t;

vec_t* vec_init(size_t itemSize, unsigned int capacity);
void vec_free(vec_t** vec);
void* vec_allocItem(vec_t* vec);
void vec_freeItem(vec_t* vec, void* item);