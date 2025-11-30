#ifndef TASK_H
#define TASK_H

#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TASK_STATE_READY = 0,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_SUSPENDED
} TaskState_t;

typedef struct TCB {
    uint32_t *stackPointer;
    uint32_t *stackBase;
    uint32_t stackSize;
    void (*taskFunc)(void *);
    void *params;
    const char *name;
    uint8_t priority;
    uint8_t basePriority;
    TaskState_t state;
    uint32_t delayTicks;
    uint32_t timeSlice;
    uint32_t timeSliceRemain;
    struct TCB *next;
} TCB_t;

typedef void (*TaskFunction_t)(void *);

void Task_CreateStatic(TCB_t *tcb, uint32_t *stackBuffer, uint32_t stackSizeBytes,
                       TaskFunction_t taskFunc, const char *name, void *params,
                       uint8_t priority, uint32_t timeSlice);
void Task_Delay(uint32_t ticks);
void Task_Yield(void);
void Task_StartScheduler(void);
void Task_TickHandler(void);
void Task_ExitError(void);

#ifdef __cplusplus
}
#endif

#endif
