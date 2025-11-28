/*
 * task.c
 *
 *  Created on: Nov 28, 2025
 *      Author: user
 */

// 기본 타임 슬라이스 정의 (예: 10ms)
#define DEFAULT_TIME_SLICE 10

TCB_t *currentTask = NULL;
TCB_t *delayedList = NULL;
volatile uint32_t systemTicks = 0;

static void Add_To_DelayedList(TCB_t *task);
static void Remove_From_DelayedList(TCB_t *task);
static void Remove_From_ReadyList(TCB_t *task);

TCB_t* Task_Create(TaskFunction_t func, const char *name, uint32_t stackSize,
        void *params, uint8_t priority, uint32_t timeSlice) {
    if(func == NULL){
        return NULL;
    }

    // 1. TCB 할당
    TCB_t *newTask = (TCB_t *)malloc(sizeof(TCB_t));
    if(newTask == NULL) return NULL;

    // 2. 스택 할당
    newTask->stackBase = (uint32_t *)malloc(stackSize);
    if (newTask->stackBase == NULL){
        free(newTask);
        return NULL;
    }

    // 3. TCB 필드 초기화
    newTask->taskFunc = func;
    newTask->stackSize = stackSize;
    newTask->priority = priority;
    newTask->basePriority = priority;
    newTask->state = TASK_READY;
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
    if (name != NULL) {
        strncpy(newTask->name, name, MAX_TASK_NAME_LEN - 1);
        newTask->name[MAX_TASK_NAME_LEN - 1] = '\0';
    } else {
        newTask->name[0] = '\0';
    }

    // 4. 스택 초기화 (ARM Cortex-M 기준)
    uint32_t *stackTop = newTask->stackBase + (stackSize / sizeof(uint32_t));

    // 하드웨어 자동 저장 레지스터 (exception frame)
    *(--stackTop) = 0x01000000;           // xPSR (Thumb bit)
    *(--stackTop) = (uint32_t)func;       // PC
    *(--stackTop) = 0xFFFFFFFD;           // LR (EXC_RETURN)
    *(--stackTop) = 0;                    // R12
    *(--stackTop) = 0;                    // R3
    *(--stackTop) = 0;                    // R2
    *(--stackTop) = 0;                    // R1
    *(--stackTop) = (uint32_t)params;     // R0 (파라미터)

    // 소프트웨어 저장 레지스터
    *(--stackTop) = 0;                    // R11
    *(--stackTop) = 0;                    // R10
    *(--stackTop) = 0;                    // R9
    *(--stackTop) = 0;                    // R8
    *(--stackTop) = 0;                    // R7
    *(--stackTop) = 0;                    // R6
    *(--stackTop) = 0;                    // R5
    *(--stackTop) = 0;                    // R4

    newTask->stackPointer = stackTop;

    Add_To_ReadyList(newTask);

    return newTask;
}





