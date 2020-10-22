#ifndef TASK_H
#define TASK_H

#define TASK_NAME_MAX_LENGTH (16)
#define TASK_STACK_AREA_SIZE (256) // in bytes
#define IDLE_TASK_PRIORITY (256) // first value that is greater than max 8-bit number

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>

#define STATUS_DELAYED (0x1)
#define STATUS_SUSPENDED (0x2)
#define STATUS_READY (0x4)
#define STATUS_RUNNING (0x8)

typedef struct TCB {
    // at offset zero in struct TCB so when using asm directives
    // it can be accessed by using only the starting address of struct TCB 
    void *taskSP; // task stack pointer
    struct TCB *next;
    uint16_t delayUnits; // in units of 1 ms each, 1000 units = 1 s delay
    void (*function)();
    char name[TASK_NAME_MAX_LENGTH];
    uint16_t priority;
    uint8_t status;
} TCB;

#endif // TASK_H	
        			 
       			

        			