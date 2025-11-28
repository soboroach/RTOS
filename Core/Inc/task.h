#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <stdbool.h>

/*---------------------------------------------------------------------------
 * 설정
 *---------------------------------------------------------------------------*/
#define MAX_TASKS              16
#define NUM_PRIORITY_LEVELS    8     // 우선순위 0~7
#define LOWEST_PRIORITY        (NUM_PRIORITY_LEVELS - 1)
#define HIGHEST_PRIORITY       0

/*---------------------------------------------------------------------------
 * 타입 정의
 *---------------------------------------------------------------------------*/
typedef void (*TaskFunction_t)(void *params);

typedef enum {
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_SUSPENDED,
    TASK_STATE_TERMINATED
} TaskState_t;

typedef struct TCB {
    uint32_t        *stackPointer;
    uint32_t        *stackBase;
    uint32_t        stackSize;

    uint8_t         taskID;
    char            name[12];

    uint8_t         priority;
    uint8_t         basePriority;
    TaskState_t     state;

    uint32_t        wakeupTime;

    TaskFunction_t  taskFunc;
    void            *params;

    struct TCB      *next;      // 같은 우선순위 큐 내 연결
} TCB_t;

/*---------------------------------------------------------------------------
 * 공개 API (사용자용)
 *---------------------------------------------------------------------------*/

// 태스크 생성/삭제
TCB_t*  Task_Create(TaskFunction_t func, const char *name,
                    uint32_t stackSize, void *params, uint8_t priority);
void    Task_Delete(TCB_t *task);

// 태스크 제어
void    Task_Suspend(TCB_t *task);
void    Task_Resume(TCB_t *task);
void    Task_Yield(void);
void    Task_Delay(uint32_t ticks);
void    Task_DelayUntil(uint32_t *prevWakeTime, uint32_t period);

// 우선순위
void    Task_SetPriority(TCB_t *task, uint8_t newPriority);
uint8_t Task_GetPriority(TCB_t *task);

// 정보 조회
TCB_t*      Task_GetCurrent(void);
TaskState_t Task_GetState(TCB_t *task);

// 스케줄러
void    Task_StartScheduler(void);
void    Task_EnterCritical(void);
void    Task_ExitCritical(void);

#endif /* TASK_H */
