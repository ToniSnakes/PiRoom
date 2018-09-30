#include <stdlib.h>
#include <string.h>

#include "queue.h"

struct queue_struct {
    unsigned int capacity;
    unsigned int count;
    unsigned int offset;
    void** data;
};
typedef struct queue_struct queue_t;

const queue_t empty_queue = { .capacity = 0, .offset = 0, .count = 0, .data = NULL };

queue_t* queue_init(int capacity)
{
    queue_t* queue = malloc(sizeof(queue_t));
    *queue = (queue_t){ .capacity = capacity, .offset = 0, .count = 0, .data = malloc(capacity * sizeof(void*)) };
    return queue;
}

void queue_free(queue_t** queue)
{
    free((*queue)->data);
    free(*queue);
    *queue = NULL;
}

void queue_checkEnlarge(queue_t* queue)
{
    if (queue->count + 1 > queue->capacity) {
        if (queue->capacity < 1) {
            queue->data = malloc(4 * sizeof(void*));
            queue->capacity = 4;
            queue->offset = 0;
            queue->count = 0;
        } else {
            queue->data = realloc(queue->data, queue->capacity * 2 * sizeof(void*));
            memcpy(queue->data + queue->capacity, queue->data, queue->offset * sizeof(void*));
            queue->capacity *= 2;
        }
    }
}

void queue_enqueue(queue_t* queue, void* item)
{
    queue_checkEnlarge(queue);

    *(queue->data + (queue->count + queue->offset) % queue->capacity) = item;
    queue->count++;
}

void* queue_dequeue(queue_t* queue)
{
    if (queue->count == 0)
        return NULL;

    void* item = *(queue->data + queue->offset);

    queue->offset = (queue->offset + 1) % queue->capacity;
    queue->count--;

    return item;
}

unsigned int queue_count(queue_t* queue)
{
    return queue->count;
}