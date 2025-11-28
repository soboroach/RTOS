/*
 * scheduler.c
 *
 *  Created on: Nov 28, 2025
 *      Author: user
 */



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

