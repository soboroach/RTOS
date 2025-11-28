/*
 * task_internal.h
 *
 *  Created on: Nov 28, 2025
 *      Author: user
 */

#ifndef INC_TASK_INTERNAL_H_
#define INC_TASK_INTERNAL_H_


// 스택 초기화 (ARM Cortex-M용)
Uint32_t* Task_InitStack(
	uint32_t *stackTop,
	TaskFunction_t func,
	void *params
);

// 틱 핸들러에서 호출 (delay 처리)
void Task_TickHandler(void);

// Ready 리스트에서 태스크 추가
void Task_AddToReadyList

#endif /* INC_TASK_INTERNAL_H_ */
