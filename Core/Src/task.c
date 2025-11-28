/*
 * task.c
 *
 *  Created on: Nov 28, 2025
 *      Author: user
 */

// 기본 타임 슬라이스 정의 (예: 10ms)
#include <task.h>
#include <core_cm4.h>
#include <scheduler.h>
#define DEFAULT_TIME_SLICE 10

TCB_t *currentTask = NULL;
TCB_t *delayedList = NULL;
volatile uint32_t systemTicks = 0;

static void Add_To_DelayedList(TCB_t *task);
static void Remove_From_DelayedList(TCB_t *task);
static void Remove_From_ReadyList(TCB_t *task);

TCB_t *Task_Create(TaskFunction_t func, const char *name, uint32_t stackSize,
				   void *params, uint8_t priority, uint32_t timeSlice)
{
	if (func == NULL)
	{
		return NULL;
	}

	// 1. TCB 할당
	TCB_t *newTask = (TCB_t *)malloc(sizeof(TCB_t));
	if (newTask == NULL)
		return NULL;

	// 2. 스택 할당
	newTask->stackBase = (uint32_t *)malloc(stackSize);
	if (newTask->stackBase == NULL)
	{
		free(newTask);
		return NULL;
	}

	// 3. TCB 필드 초기화
	newTask->taskFunc = func;
	newTask->stackSize = stackSize;
	newTask->priority = priority;
	newTask->basePriority = priority;
	newTask->state = TASK_STATE_READY;
	newTask->params = params;
	newTask->next = NULL;
	newTask->wakeupTime = 0;

	// 라운드 로빈 관련 초기화
	newTask->timeSlice = (timeSlice > 0) ? timeSlice : DEFAULT_TIME_SLICE;
	newTask->remainingTime = newTask->timeSlice;

	// 태스크 ID 할당
	static uint8_t nextTaskID = 0;
	newTask->taskID = nextTaskID++;

	// 이름 복사
	if (name != NULL)
	{
		strncpy(newTask->name, name, MAX_TASK_NAME_LEN - 1);
		newTask->name[MAX_TASK_NAME_LEN - 1] = '\0';
	}
	else
	{
		newTask->name[0] = '\0';
	}

	// 4. 스택 초기화 (ARM Cortex-M 기준)
	uint32_t *stackTop = newTask->stackBase + (stackSize / sizeof(uint32_t));

	// 하드웨어 자동 저장 레지스터 (exception frame)
	*(--stackTop) = 0x01000000;		  // xPSR (Thumb bit)
	*(--stackTop) = (uint32_t)func;	  // PC
	*(--stackTop) = 0xFFFFFFFD;		  // LR (EXC_RETURN)
	*(--stackTop) = 0;				  // R12
	*(--stackTop) = 0;				  // R3
	*(--stackTop) = 0;				  // R2
	*(--stackTop) = 0;				  // R1
	*(--stackTop) = (uint32_t)params; // R0 (파라미터)

	// 소프트웨어 저장 레지스터
	*(--stackTop) = 0; // R11
	*(--stackTop) = 0; // R10
	*(--stackTop) = 0; // R9
	*(--stackTop) = 0; // R8
	*(--stackTop) = 0; // R7
	*(--stackTop) = 0; // R6
	*(--stackTop) = 0; // R5
	*(--stackTop) = 0; // R4

	newTask->stackPointer = stackTop;

	Add_To_ReadyList(newTask);

	return newTask;
}

void Task_Suspend(TCB_t *task)
{
	if (task == NULL)
		return;

	Task_EnterCritical();

	if (task->state == TASK_STATE_READY)
	{
		Remove_From_ReadyList(task);
	}
	else if (task->state == TASK_STATE_BLOCKED)
	{
		Remove_From_DelayedList(task);
	}

	task->state = TASK_STATE_SUSPENDED;

	// 현재 실행 중인 태스크를 suspend하면 컨텍스트 스위칭 필요
	if (task == currentTask)
	{
		Task_ExitCritical();
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		return;
	}

	Task_ExitCritical();
}

/*---------------------------------------------------------------------------
 * Task_Resume - 일시 중지된 태스크를 재개
 *---------------------------------------------------------------------------*/
void Task_Resume(TCB_t *task)
{
	if (task == NULL)
		return;
	if (task->state != TASK_STATE_SUSPENDED)
		return;

	Task_EnterCritical();

	task->state = TASK_STATE_READY;
	task->remainingTime = task->timeSlice;
	Add_To_ReadyList(task);

	// 재개된 태스크가 현재 태스크보다 높은 우선순위면 스케줄링
	uint8_t needSwitch = (currentTask != NULL && task->priority < currentTask->priority);

	Task_ExitCritical();

	if (needSwitch)
	{
		SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	}
}

/*---------------------------------------------------------------------------
 * Task_Yield - 현재 태스크가 자발적으로 CPU 양보
 *---------------------------------------------------------------------------*/
void Task_Yield(void)
{
	Task_EnterCritical();

	if (currentTask != NULL && currentTask->state == TASK_STATE_RUNNING)
	{
		currentTask->remainingTime = currentTask->timeSlice;
		currentTask->state = TASK_STATE_READY;

		// 같은 우선순위 큐의 맨 뒤로 이동
		Add_To_ReadyList(currentTask);
	}

	Task_ExitCritical();

	// PendSV 트리거
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/*---------------------------------------------------------------------------
 * Task_Delay - 지정된 틱 수만큼 태스크 블록
 *---------------------------------------------------------------------------*/
void Task_Delay(uint32_t ticks)
{
	if (ticks == 0)
	{
		Task_Yield();
		return;
	}

	Task_EnterCritical();

	currentTask->state = TASK_STATE_BLOCKED;
	currentTask->wakeupTime = systemTicks + ticks;

	Add_To_DelayedList(currentTask);

	Task_ExitCritical();

	// PendSV 트리거
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/*---------------------------------------------------------------------------
 * Task_DelayUntil - 주기적 태스크를 위한 절대 시간 기반 딜레이
 *---------------------------------------------------------------------------*/
void Task_DelayUntil(uint32_t *prevWakeTime, uint32_t period)
{
	if (prevWakeTime == NULL || period == 0)
		return;

	Task_EnterCritical();

	*prevWakeTime += period;

	// 이미 시간이 지났으면 딜레이 없이 반환
	int32_t timeToWait = (int32_t)(*prevWakeTime - systemTicks);
	if (timeToWait <= 0)
	{
		Task_ExitCritical();
		return;
	}

	currentTask->state = TASK_STATE_BLOCKED;
	currentTask->wakeupTime = *prevWakeTime;

	Add_To_DelayedList(currentTask);

	Task_ExitCritical();

	// PendSV 트리거
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

/*---------------------------------------------------------------------------
 * 헬퍼 함수: Delayed 리스트에 wakeupTime 순으로 삽입
 *---------------------------------------------------------------------------*/
static void Add_To_DelayedList(TCB_t *task)
{
	task->next = NULL;

	if (delayedList == NULL)
	{
		delayedList = task;
		return;
	}

	// wakeupTime이 가장 빠르면 맨 앞에 삽입
	if (task->wakeupTime < delayedList->wakeupTime)
	{
		task->next = delayedList;
		delayedList = task;
		return;
	}

	// 적절한 위치 찾기
	TCB_t *current = delayedList;
	while (current->next != NULL && current->next->wakeupTime <= task->wakeupTime)
	{
		current = current->next;
	}
	task->next = current->next;
	current->next = task;
}

/*---------------------------------------------------------------------------
 * 헬퍼 함수: Delayed 리스트에서 태스크 제거
 *---------------------------------------------------------------------------*/
static void Remove_From_DelayedList(TCB_t *task)
{
	if (delayedList == NULL || task == NULL)
		return;

	if (delayedList == task)
	{
		delayedList = task->next;
		task->next = NULL;
		return;
	}

	TCB_t *current = delayedList;
	while (current->next != NULL && current->next != task)
	{
		current = current->next;
	}

	if (current->next == task)
	{
		current->next = task->next;
		task->next = NULL;
	}
}

/*---------------------------------------------------------------------------
 * 헬퍼 함수: Ready 리스트에서 특정 태스크 제거
 *---------------------------------------------------------------------------*/
static void Remove_From_ReadyList(TCB_t *task)
{
	if (task == NULL)
		return;

	uint8_t prio = task->priority;

	if (readyQueue[prio] == NULL)
		return;

	// 첫 번째 노드인 경우
	if (readyQueue[prio] == task)
	{
		readyQueue[prio] = task->next;
		if (readyQueue[prio] == NULL)
		{
			readyQueueTail[prio] = NULL;
			readyBitmap &= ~(1 << prio);
		}
		else if (task == readyQueueTail[prio])
		{
			readyQueueTail[prio] = NULL;
		}
		task->next = NULL;
		return;
	}

	// 중간/마지막 노드 찾기
	TCB_t *current = readyQueue[prio];
	while (current->next != NULL && current->next != task)
	{
		current = current->next;
	}

	if (current->next == task)
	{
		current->next = task->next;
		if (task == readyQueueTail[prio])
		{
			readyQueueTail[prio] = current;
		}
		task->next = NULL;
	}
}

/*---------------------------------------------------------------------------
 * SysTick에서 호출: delayed 태스크 깨우기 및 타임 슬라이스 처리
 *---------------------------------------------------------------------------*/
void Task_TickHandler(void)
{
	systemTicks++;

	// 1. Delayed 리스트에서 깨어날 태스크 처리
	while (delayedList != NULL && delayedList->wakeupTime <= systemTicks)
	{
		TCB_t *task = delayedList;
		delayedList = task->next;
		task->next = NULL;

		task->state = TASK_STATE_READY;
		task->remainingTime = task->timeSlice;
		Add_To_ReadyList(task);
	}

	// 2. 현재 태스크의 타임 슬라이스 감소 (라운드 로빈)
	if (currentTask != NULL && currentTask->state == TASK_STATE_RUNNING)
	{
		if (currentTask->remainingTime > 0)
		{
			currentTask->remainingTime--;
		}

		// 타임 슬라이스 소진 시 컨텍스트 스위칭
		if (currentTask->remainingTime == 0)
		{
			currentTask->remainingTime = currentTask->timeSlice;
			currentTask->state = TASK_STATE_READY;
			Add_To_ReadyList(currentTask);
			SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
		}
	}
}