// PAF: planned async framework

#include "paf.h"
#include "queue.h"
#include "vec.h"

#include <stdlib.h>
#include <string.h>

#define BUFFSIZE 1024 * sizeof(char)

struct paf_struct {
    size_t planSize;
    void (*handleStep)(const char*, void*);
    void (*createPlan)(const char*, void*);
    queue_t* tasks;
    // Efficiency could be improved with proper alignment
    vec_t* plans;
};

paf_t* paf_init(
    size_t planSize,
    void (*handleStep)(const char* message, void* plan),
    void (*createPlan)(const char* message, void* plan))
{
    paf_t* paf = malloc(sizeof(paf_t));
    *paf = (paf_t){
        .handleStep = handleStep,
        .createPlan = createPlan,
        .tasks = queue_init(8),
        .planSize = planSize,
        .plans = vec_init(BUFFSIZE + planSize, 4)
    };
}

void paf_free(paf_t** paf)
{
    queue_free(&(*paf)->tasks);
    vec_free(&(*paf)->plans);
    free(*paf);
    *paf = NULL;
}

// in a multithread environment this function would copy message and run create
// plan in another thread.
void paf_newMessage(paf_t* paf, char* message, void* planInit)
{
    void* plan = vec_allocItem(paf->plans);
    if (planInit != NULL)
        memcpy(plan, planInit, paf->planSize);

    char* msg = (char*)plan + paf->planSize;
    strncpy(msg, message, BUFFSIZE);

    paf->createPlan(msg, plan);
}

// in a multithread environment this function would dispatch the first item in
// the queue to a thread
void paf_dispatch(paf_t* paf)
{
    void* plan = queue_dequeue(paf->tasks);

    if (plan == NULL)
        return;

    paf->handleStep((char*)plan + paf->planSize, plan);
}

// this gets called at the end of a processing step
void paf_finishStep(paf_t* paf, void* plan)
{
    queue_enqueue(paf->tasks, plan);
}

// this gets called when whe are done with one message. Here we free up the
// plan.
void paf_finishFinalStep(paf_t* paf, void* plan)
{
    vec_freeItem(paf->plans, plan);
}