typedef struct queue_struct_t queue_t;

queue_t* queue_init(int capacity);
void queue_free(queue_t** queue);
void queue_enqueue(queue_t* queue, void* item);
void* queue_dequeue(queue_t* queue);
unsigned int queue_count(queue_t* queue);