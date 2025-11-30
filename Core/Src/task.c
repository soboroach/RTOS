#include "task.h"
#include "scheduler.h"
#include <string.h>

#define INITIAL_XPSR  0x01000000UL

void Task_ExitError(void)
{
    __disable_irq();
    while (1);
}

static uint32_t *Task_InitStack(uint32_t *stackTop,
                                TaskFunction_t taskFunc,
                                void *params)
{
    uint32_t *sp = stackTop;

    *(--sp) = INITIAL_XPSR;
    *(--sp) = (uint32_t)taskFunc;
    *(--sp) = (uint32_t)Task_ExitError;
    *(--sp) = 0x12121212UL;
    *(--sp) = 0x03030303UL;
    *(--sp) = 0x02020202UL;
    *(--sp) = 0x01010101UL;
    *(--sp) = (uint32_t)params;

    *(--sp) = 0x11111111UL;
    *(--sp) = 0x10101010UL;
    *(--sp) = 0x09090909UL;
    *(--sp) = 0x08080808UL;
    *(--sp) = 0x07070707UL;
    *(--sp) = 0x06060606UL;
    *(--sp) = 0x05050505UL;
    *(--sp) = 0x04040404UL;

    return sp;
}

void Task_CreateStatic(TCB_t *tcb,
                       uint32_t *stackBuffer,
                       uint32_t stackSizeBytes,
                       TaskFunction_t taskFunc,
                       const char *name,
                       void *params,
                       uint8_t priority,
                       uint32_t timeSlice)
{
    memset(tcb, 0, sizeof(TCB_t));

    tcb->stackBase = stackBuffer;
    tcb->stackSize = stackSizeBytes;
    tcb->taskFunc = taskFunc;
    tcb->params = params;
    tcb->name = name;
    tcb->priority = priority;
    tcb->basePriority = priority;
    tcb->state = TASK_STATE_READY;
    tcb->delayTicks = 0;
    tcb->timeSlice = timeSlice;
    tcb->timeSliceRemain = timeSlice;
    tcb->next = NULL;

    uint32_t stackWords = stackSizeBytes / sizeof(uint32_t);
    uint32_t *stackTop = &stackBuffer[stackWords];
    tcb->stackPointer = Task_InitStack(stackTop, taskFunc, params);

    Scheduler_AddTask(tcb);
}

void Task_Delay(uint32_t ticks)
{
    if (ticks == 0) return;

    __disable_irq();

    if (currentTask != NULL) {
        currentTask->delayTicks = ticks;
        currentTask->state = TASK_STATE_BLOCKED;
    }

    __enable_irq();

    Scheduler_Schedule();
}

void Task_Yield(void)
{
    Scheduler_Schedule();
}

void Task_StartScheduler(void)
{
    Scheduler_Start();
}

void Task_TickHandler(void)
{
    TCB_t *task;
    uint8_t needSchedule = 0;

    task = taskListHead;
    while (task != NULL) {
        if (task->state == TASK_STATE_BLOCKED && task->delayTicks > 0) {
            task->delayTicks--;
            if (task->delayTicks == 0) {
                task->state = TASK_STATE_READY;
                needSchedule = 1;
            }
        }
        task = task->next;
    }

    if (currentTask != NULL && currentTask->state == TASK_STATE_RUNNING) {
        if (currentTask->timeSliceRemain > 0) {
            currentTask->timeSliceRemain--;
        }
        if (currentTask->timeSliceRemain == 0) {
            currentTask->timeSliceRemain = currentTask->timeSlice;
            needSchedule = 1;
        }
    }

    if (needSchedule) {
        Scheduler_Schedule();
    }
}
