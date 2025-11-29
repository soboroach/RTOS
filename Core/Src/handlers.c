/*
 * handlers.c
 *
 *  Created on: Nov 29, 2025
 *      Author: user
 */


#include "stm32f4xx.h"  // 또는 사용하는 MCU 헤더
#include "scheduler.h"
#include "task.h"

/*---------------------------------------------------------------------------
 * PendSV_Handler - 컨텍스트 스위칭 수행
 *
 * Cortex-M에서 예외 진입 시 하드웨어가 자동으로 저장하는 레지스터:
 *   xPSR, PC, LR, R12, R3-R0 (8개)
 *
 * 소프트웨어로 저장해야 하는 레지스터:
 *   R4-R11 (8개)
 *---------------------------------------------------------------------------*/
__attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile (
        // 인터럽트 비활성화
        "CPSID   I                      \n"

        // currentTask가 NULL이면 스킵 (첫 실행 시)
        "LDR     R0, =currentTask       \n"
        "LDR     R1, [R0]               \n"
        "CBZ     R1, _load_next         \n"

        // === 현재 태스크 컨텍스트 저장 ===
        "MRS     R2, PSP                \n"  // PSP 가져오기
        "STMDB   R2!, {R4-R11}          \n"  // R4-R11 저장 (스택에 push)
        "STR     R2, [R1]               \n"  // stackPointer 업데이트 (TCB 첫 필드)

    "_load_next:                        \n"
        // === 다음 태스크로 전환 ===
        "LDR     R0, =currentTask       \n"
        "LDR     R1, =nextTask          \n"
        "LDR     R2, [R1]               \n"  // nextTask 로드
        "STR     R2, [R0]               \n"  // currentTask = nextTask

        // === 다음 태스크 컨텍스트 복원 ===
        "LDR     R0, [R2]               \n"  // stackPointer 로드
        "LDMIA   R0!, {R4-R11}          \n"  // R4-R11 복원
        "MSR     PSP, R0                \n"  // PSP 설정

        // 인터럽트 활성화
        "CPSIE   I                      \n"

        // Thread mode + PSP 사용으로 복귀
        "LDR     LR, =0xFFFFFFFD        \n"
        "BX      LR                     \n"
    );
}

/*---------------------------------------------------------------------------
 * SysTick_Handler - 시스템 틱 처리
 *---------------------------------------------------------------------------*/
void SysTick_Handler(void)
{
    // HAL 사용 시 (선택적)
    // HAL_IncTick();

    // RTOS 틱 처리
    Task_TickHandler();
}
