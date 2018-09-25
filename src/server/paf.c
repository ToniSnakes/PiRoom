// PAF: planned async framework

#include "paf.h"
#include "queue.h"

#include <stdlib.h>

typedef struct {
    unsigned int handle;
    void* plan;
} plan_t;

struct paf_struct_t {
    size_t planSize;
    void (*handleStep)(unsigned int, void*);
    queue_t* tasks;
};

paf_t* paf_init(size_t planSize, void (*handleStep)(unsigned int handle, void* plan))
{
    paf_t* paf = malloc(sizeof(paf_t));
    *paf = (paf_t){
        .tasks = queue_init(8),
        .handleStep = handleStep,
        .planSize = planSize
    };
}

