// PAF: planned async framework

#include "paf.h"
#include "queue.h"

#include <stdlib.h>

struct paf_struct_t {
    size_t planSize;
    void (*handleStep)(void*);
    void (*createPlan)(char*, void*);
    queue_t* tasks;
};

paf_t* paf_init(
    size_t planSize,
    void (*handleStep)(void* plan),
    void (*createPlan)(char* message, void* plan))
{
    paf_t* paf = malloc(sizeof(paf_t));
    *paf = (paf_t){
        .handleStep = handleStep,
        .createPlan = createPlan,
        .tasks = queue_init(8),
        .planSize = planSize
    };
}

void paf_free(paf_t** paf)
{
    queue_free(&(*paf)->tasks);
    free(*paf);
    *paf = NULL;
}

// in a multithread environment this function would copy message and run create
// plan in another thread.
void paf_newMessage(paf_t* paf, char* message)
{
    // for more efficiency we could manually allocate a space for the plan.
    void* plan = malloc(paf->planSize);
    paf->createPlan(message, plan);
    queue_enqueue(paf->tasks, plan);
}

// in a multithread environment this function would dispatch the first item in
// the queue to a thread
void paf_dispatch(paf_t* paf)
{
    void* plan = queue_dequeue(paf->tasks);

    if (plan == NULL)
        return;

    paf->handleStep(plan);
}

// this gets called at the end of a processing step
void paf_finishStep(paf_t* paf, void* plan)
{
    queue_enqueue(paf->tasks, plan);
}

// this gets called when whe are done with one message. If we ever manally
// allocate the plan, we can properly free it up right here.
void paf_finishFinalStep(paf_t* paf, void* plan)
{
    free(plan);
}