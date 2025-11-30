/*
 * scheduler.c
 *
 *  Created on: Nov 28, 2025
 *      Author: user
 */
#include "scheduler.h"
#include "task.h"

#include <stddef.h>      // NULL 정의
#include <stdint.h>
#include <string.h>      // strncpy 정의
#include "stm32f4xx.h"

uint8_t readyBitmap = 0;
TCB_t *readyQueue[MAX_PRIORITY] = {0};
TCB_t *readyQueueTail[MAX_PRIORITY] = {0};

TCB_t *currentTask = NULL;
TCB_t *nextTask = NULL;
static volatile uint32_t criticalNesting = 0;


TCB_t* Pop_From_ReadyList(uint8_t prio) {
    TCB_t *task = readyQueue[prio];

    if (task == NULL) return NULL;

    readyQueue[prio] = task->next;

    if (readyQueue[prio] == NULL) {
        readyQueueTail[prio] = NULL;
        readyBitmap &= ~(1 << prio);
    }

    task->next = NULL;
    return task;
}

void Add_To_ReadyList(TCB_t *task) {
    uint8_t prio = task->priority;
    task->next = NULL;

    if (readyQueue[prio] == NULL) {
        // 큐가 비어있으면 head = tail = task
        readyQueue[prio] = task;
    } else {
        // 뒤에 연결
        readyQueueTail[prio]->next = task;
    }
    readyQueueTail[prio] = task;

    readyBitmap |= (1 << prio);
}

/*---------------------------------------------------------------------------
 * Task_EnterCritical - 크리티컬 섹션 진입 (중첩 지원)
 *---------------------------------------------------------------------------*/
void Task_EnterCritical(void) {
    __disable_irq();
    criticalNesting++;
}

/*---------------------------------------------------------------------------
 * Task_ExitCritical - 크리티컬 섹션 탈출 (중첩 지원)
 *---------------------------------------------------------------------------*/
void Task_ExitCritical(void) {
    if (criticalNesting > 0) {
        criticalNesting--;
        if (criticalNesting == 0) {
            __enable_irq();
        }
    }
}

/*---------------------------------------------------------------------------
 * 스케줄러: 다음 실행할 태스크 선택
 *---------------------------------------------------------------------------*/
static TCB_t* Select_NextTask(void) {
    // 가장 높은 우선순위(낮은 숫자)의 Ready 태스크 선택
    if (readyBitmap == 0) {
        return NULL;  // Idle 태스크로 전환 필요
    }

    // CLZ(Count Leading Zeros)로 최고 우선순위 찾기
    uint8_t highestPrio = __CLZ(__RBIT(readyBitmap));

    return Pop_From_ReadyList(highestPrio);
}

/*---------------------------------------------------------------------------
 * Schedule - 스케줄링 수행 (PendSV 트리거 전 호출)
 *---------------------------------------------------------------------------*/
void Schedule(void) {
    Task_EnterCritical();

    nextTask = Select_NextTask();

    if (nextTask != NULL && nextTask != currentTask) {
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }

    Task_ExitCritical();
}

/*---------------------------------------------------------------------------
 * Task_StartScheduler - 스케줄러 시작
 *---------------------------------------------------------------------------*/
void Task_StartScheduler(void) {
    // PendSV와 SysTick을 최저 우선순위로 설정
    NVIC_SetPriority(PendSV_IRQn, 0xFF);
    NVIC_SetPriority(SysTick_IRQn, 0xFF);

    // 첫 번째 태스크 선택
    currentTask = Select_NextTask();

    if (currentTask == NULL) {
        return;  // 실행할 태스크 없음
    }

    currentTask->state = TASK_STATE_RUNNING;

    // 첫 태스크 시작 (어셈블리로 구현)
    __set_PSP((uint32_t)currentTask->stackPointer);
    __set_CONTROL(0x02);  // PSP 사용, Thread 모드
    __ISB();

    // 첫 태스크로 점프
    __asm volatile (
        "LDR R0, %0          \n"  // stackPointer 로드
        "LDMIA R0!, {R4-R11} \n"  // R4-R11 복원
        "MSR PSP, R0         \n"  // PSP 업데이트
        "MOV LR, #0xFFFFFFFD \n"  // EXC_RETURN (PSP, Thread mode)
        "BX LR               \n"  // 태스크로 분기
        :
        : "m" (currentTask->stackPointer)
        : "r0"
    );
}


