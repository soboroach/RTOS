#include "scheduler.h"

/* 전역 변수 정의 - 여기서 실제 메모리 할당 */
TCB_t *currentTask = NULL;
TCB_t *nextTask = NULL;
TCB_t *taskListHead = NULL;

static TCB_t *lastScheduled[MAX_PRIORITY_LEVELS] = {NULL};

static TCB_t idleTaskTCB;
static uint32_t idleTaskStack[64];
static uint8_t idleTaskCreated = 0;

static void IdleTask_Func(void *params)
{
    (void)params;
    while (1) {
        __WFI();
    }
}

void Scheduler_Init(void)
{
    currentTask = NULL;
    nextTask = NULL;
    taskListHead = NULL;

    for (int i = 0; i < MAX_PRIORITY_LEVELS; i++) {
        lastScheduled[i] = NULL;
    }
}

void Scheduler_AddTask(TCB_t *tcb)
{
    __disable_irq();
    tcb->next = taskListHead;
    taskListHead = tcb;
    __enable_irq();
}

TCB_t *Scheduler_GetHighestPriorityTask(void)
{
    TCB_t *task;
    uint8_t highestPriority = 255;

    task = taskListHead;
    while (task != NULL) {
        if ((task->state == TASK_STATE_READY || task->state == TASK_STATE_RUNNING)) {
            if (task->priority < highestPriority) {
                highestPriority = task->priority;
            }
        }
        task = task->next;
    }

    if (highestPriority == 255) {
        return NULL;
    }

    TCB_t *firstCandidate = NULL;
    TCB_t *afterLast = NULL;
    uint8_t foundLast = 0;

    task = taskListHead;
    while (task != NULL) {
        if ((task->state == TASK_STATE_READY || task->state == TASK_STATE_RUNNING)
            && task->priority == highestPriority) {

            if (firstCandidate == NULL) {
                firstCandidate = task;
            }

            if (foundLast && afterLast == NULL) {
                afterLast = task;
            }

            if (task == lastScheduled[highestPriority]) {
                foundLast = 1;
            }
        }
        task = task->next;
    }

    TCB_t *selected = (afterLast != NULL) ? afterLast : firstCandidate;

    if (selected != NULL) {
        lastScheduled[highestPriority] = selected;
    }

    return selected;
}

void Scheduler_Schedule(void)
{
    __disable_irq();

    TCB_t *next = Scheduler_GetHighestPriorityTask();

    if (next == NULL) {
        __enable_irq();
        return;
    }

    if (currentTask != NULL && currentTask->state == TASK_STATE_RUNNING) {
        currentTask->state = TASK_STATE_READY;
    }

    nextTask = next;
    nextTask->state = TASK_STATE_RUNNING;

    __enable_irq();

    if (currentTask != nextTask) {
        Scheduler_ContextSwitch();
    }
}

void Scheduler_ContextSwitch(void)
{
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}

void Scheduler_Start(void)
{
    if (!idleTaskCreated) {
        Task_CreateStatic(&idleTaskTCB, idleTaskStack, sizeof(idleTaskStack),
                          IdleTask_Func, "Idle", NULL, MAX_PRIORITY_LEVELS - 1, 1);
        idleTaskCreated = 1;
    }

    NVIC_SetPriority(PendSV_IRQn, 0xFF);
    NVIC_SetPriority(SysTick_IRQn, 0xFE);
    SysTick_Config(SystemCoreClock / SYSTICK_FREQ_HZ);

    nextTask = Scheduler_GetHighestPriorityTask();

    if (nextTask == NULL) {
        while (1);
    }

    nextTask->state = TASK_STATE_RUNNING;
    currentTask = nextTask;

    __asm volatile (
        "LDR     R0, =nextTask          \n"
        "LDR     R1, [R0]               \n"
        "LDR     R0, [R1]               \n"
        "LDMIA   R0!, {R4-R11}          \n"
        "MSR     PSP, R0                \n"
        "MRS     R0, CONTROL            \n"
        "ORR     R0, R0, #2             \n"
        "MSR     CONTROL, R0            \n"
        "ISB                            \n"
        "CPSIE   I                      \n"
        "LDR     LR, =0xFFFFFFFD        \n"
        "BX      LR                     \n"
    );

    while (1);
}
