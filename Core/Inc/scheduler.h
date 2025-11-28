/*
 * scheduler.h
 *
 *  Created on: Nov 28, 2025
 *      Author: user
 */

#ifndef INC_SCHEDULER_H_
#define INC_SCHEDULER_H_

uint8_t readyBitmap;
TCB_t *readyQueue[MAX_PRIORITY];      // head
TCB_t *readyQueueTail[MAX_PRIORITY];  // tail

TCB_t* Pop_From_ReadyList(uint8_t prio);
void Add_To_ReadyList(TCB_t *task);

void Task_StartScheduler(void);
void Task_EnterCritical(void);
void Task_ExitCritical(void);

#endif /* INC_SCHEDULER_H_ */
