//
// Created by wonjun on 25. 12. 1..
//

#include <stdint.h>
#include "semaphore.h"

void Semaphore_Init(Semaphore_t *sem, int32_t initialCount) {
    sem->count = initialCount;
    sem->waitListHead = NULL;
}

int 7Semaphore_Wait(Semaphore_t *sem, uint32_t timeout){

    return 0;
}
