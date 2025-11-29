#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_

#include "task.h"

#define MAX_PRIORITY NUM_PRIORITY_LEVELS

extern uint8_t readyBitmap;
extern TCB_t *readyQueue[MAX_PRIORITY];
extern TCB_t *readyQueueTail[MAX_PRIORITY];
extern TCB_t *currentTask;
extern TCB_t *nextTask;

TCB_t* Pop_From_ReadyList(uint8_t prio);
void Add_To_ReadyList(TCB_t *task);

void Task_StartScheduler(void);
void Task_EnterCritical(void);
void Task_ExitCritical(void);
void Schedule(void);

#endif /* INC_SCHEDULER_H_ */
