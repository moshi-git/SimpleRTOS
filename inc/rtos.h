#ifndef RTOS_H
#define RTOS_H

#include "task.h"

//#define __AVR_ATmega328P__
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#define TIMER1_COMPARE_VALUE (0x07CF) // 1999 decimal
#define TIMER1_MODE (1 << WGM12) // mode 4, CTC on OCR1A
#define TIMER1_ENABLE_COMPARE_MATCH (1 << OCIE1A) // set interrupt on compare match
#define TIMER1_PRESCALER_VALUE (1 << CS11) // set prescaler to 8
extern void SimpleRTOS_Init(void);
extern void SimpleRTOS_Start(void);

extern TCB *SimpleRTOS_CreateTask(const char *taskName, uint8_t taskPriority, void (*taskFunctionPtr)(void));

extern void SimpleRTOS_SetTaskPriority(const char *taskName, uint8_t newTaskPriority);
extern int16_t SimpleRTOS_GetTaskPriority(const char *taskName);

extern void SimpleRTOS_SuspendTask(void);
extern void SimpleRTOS_ResumeTask(const char *taskName);
extern void SimpleRTOS_DelayTask(uint16_t delayTimeUnits); // every unit is 1 millisecond

extern TCB *SimpleRTOS_GetTaskByName(const char *taskName);

#endif // RTOS_H