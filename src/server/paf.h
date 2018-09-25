#include <stdlib.h>

typedef struct paf_struct_t paf_t;

paf_t* paf_init(
    size_t planSize,
    void (*handleStep)(void* plan),
    void (*createPlan)(char* message, void* plan));
void paf_newMessage(paf_t* paf, char* message);
void paf_dispatch(paf_t* paf);
void paf_finishStep(paf_t* paf, void* plan);
void paf_finishFinalStep(paf_t* paf, void* plan);