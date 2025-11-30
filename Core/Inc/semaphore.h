//
// Created by wonjun on 25. 12. 1..
//

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <stdint.h>

#include "task.h"

typedef struct {
    volatile int32_t count;
    TCB_t *waitListHead;  // 대기 중인 태스크들
} Semaphore_t;

void Semaphore_Init(Semaphore_t *sem, int32_t initialCount);
int  Semaphore_Wait(Semaphore_t *sem, uint32_t timeout);
void Semaphore_Signal(Semaphore_t *sem);

#endif