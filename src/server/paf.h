#include <stdlib.h>

typedef struct paf_struct paf_t;

paf_t* paf_init(
    size_t planSize,
    void (*handleStep)(const char* message, void* plan),
    void (*createPlan)(const char* message, void* plan));
void paf_free(paf_t** paf);

void paf_newMessage(paf_t* paf, char* message, void* planInit);
void paf_dispatch(paf_t* paf);
void paf_finishStep(paf_t* paf, void* plan);
void paf_finishFinalStep(paf_t* paf, void* plan);