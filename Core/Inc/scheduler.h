#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PRIORITY_LEVELS     8
#define SYSTICK_FREQ_HZ         1000

/* 전역 변수 - 반드시 extern 선언 */
extern TCB_t *currentTask;
extern TCB_t *nextTask;
extern TCB_t *taskListHead;

void Scheduler_Init(void);
void Scheduler_AddTask(TCB_t *tcb);
void Scheduler_Schedule(void);
void Scheduler_Start(void);
void Scheduler_ContextSwitch(void);
TCB_t *Scheduler_GetHighestPriorityTask(void);

#ifdef __cplusplus
}
#endif

#endif
