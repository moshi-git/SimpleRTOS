#include "rtos.h"

// stack address used to keep track when allocating stack memory for tasks
static void *stackArea; 
static TCB *currentActiveTask;
static TCB *taskList;
static TCB *endOfTaskList;

static void IdleTask(void) {
   
    for(;;) {

        asm volatile (
            "sleep" :: // sleep to avoid wasting CPU time
        );

    }
}

// Timer1 interrupt every 1 ms at 16MHz clock (mode 4)
// OCR1A = (CLOCK_FREQ / PRESCALER) * DESIRED_TIME_IN_S - 1
// to get a whole number PRESCALER = 8
// CLOCK_FREQ = 16 MHz
// DESIRED_TIME_IN_S = 0.001
// OCR1A = 0x07CF
static void InitTimer1Interrupt() {
    
    OCR1A = TIMER1_COMPARE_VALUE;

    TCCR1B |= TIMER1_MODE;
    
    TCCR1B |= TIMER1_PRESCALER_VALUE; 
    
}

static void ModifyDelays() {
    TCB *iter = taskList;

    while(iter) {

        if(iter->status == STATUS_DELAYED) {
            iter->delayUnits--;
            if(iter->delayUnits == 0) 
                iter->status = STATUS_READY;
        }

        iter = iter->next;
    }
}

static inline void SaveContext(void) __attribute__ ((always_inline));
static inline void SaveContext(void) {
    //disable interrupts
    asm volatile (
        // save status register
        "push r0\n"
        "in r0, __SREG__\n"
        "cli\n"
        "push r0\n"
        // save contex
        "push r1\n"
        "push r2\n"
        "push r3\n"
        "push r4\n"
        "push r5\n"
        "push r6\n"
        "push r7\n"
        "push r8\n"
        "push r9\n"
        "push r10\n"
        "push r11\n"
        "push r12\n"
        "push r13\n"
        "push r14\n"
        "push r15\n"
        "push r16\n"
        "push r17\n"
        "push r18\n"
        "push r19\n"
        "push r20\n"
        "push r21\n"
        "push r22\n"
        "push r23\n"
        "push r24\n"
        "push r25\n"
        "push r26\n"
        "push r27\n"
        "push r28\n"
        "push r29\n"
        "push r30\n"
        "push r31\n"
        // save stack pointer from current task struct TCB
        "lds r30, currentActiveTask\n" // lower address
        "lds r31, currentActiveTask + 1\n" // higher address
        // register z represents r30 and r31 register pair
        
        "clr r1\n"
        
        "in r0, __SP_L__\n" // lower address
        "st z+, r0\n" // post-increment on register z
        "in r0, __SP_H__\n" // higher address
        "st z+, r0\n" // post-increment on register z
        ::
    );
}

static void RestoreContext(void) __attribute__ ((naked));
static void RestoreContext(void) {
    
    asm volatile (
        // restore stack pointer from current task struct TCB
        "lds r26, currentActiveTask\n" // lower address
        "lds r27, currentActiveTask + 1\n" // higher address
         // register x represents r26 and r27 register pair
        "ld r0, x+\n" // post-increment on register x
        "out __SP_L__, r0\n" // lower address
        "ld r0, x+\n" // post-increment on register x
        "out __SP_H__, r0\n" // higher address

        // restore context
        "pop r31\n"
        "pop r30\n"
        "pop r29\n"
        "pop r28\n"
        "pop r27\n"
        "pop r26\n"
        "pop r25\n"
        "pop r24\n"
        "pop r23\n"
        "pop r22\n"
        "pop r21\n"
        "pop r20\n"
        "pop r19\n"
        "pop r18\n"
        "pop r17\n"
        "pop r16\n"
        "pop r15\n"
        "pop r14\n"
        "pop r13\n"
        "pop r12\n"
        "pop r11\n"
        "pop r10\n"
        "pop r9\n"
        "pop r8\n"
        "pop r7\n"
        "pop r6\n"
        "pop r5\n"
        "pop r4\n"
        "pop r3\n"
        "pop r2\n"
        "pop r1\n"
        
        "pop r0\n" // status register currently in r0

        "out __SREG__, r0\n" // restore status register
        "pop r0\n" // restore the real r0
        "reti\n" // return and enable interrupts, sets bit 7 in status register, global interrupt enable bit
        ::
    );
}

static void Scheduler(void) {
    // move to the separate stack space for scheduler
    asm volatile(
        "out __SP_L__, %A0\n"
        "out __SP_H__, %B0\n"
        :: "x" (RAMEND)
    );

    TCB *iter = NULL;
    TCB *prev = NULL;
    TCB *maxPriorityTaskPrev = NULL;
    TCB *maxPriorityTask = NULL;

    if(currentActiveTask && (currentActiveTask->status == STATUS_RUNNING))
        currentActiveTask->status = STATUS_READY;

    iter = taskList;

    while(iter) {
        if(iter->status == STATUS_READY) {
            maxPriorityTask = iter;
            break;
        }
        prev = iter;
        iter = iter->next;
    }

    maxPriorityTaskPrev = prev;
    prev = iter;
    iter = iter->next;

    // find the task with max priority that is ready for running
    while(iter) {
        if((iter->status == STATUS_READY) && (iter->priority < maxPriorityTask->priority)) {
            maxPriorityTaskPrev = prev;
            maxPriorityTask = iter;
        }
        prev = iter;
        iter = iter->next;
    }

    // every time a task with the highest priority is selected
    // put it at the end of the list
    // since the list is always traversed from the beginning
    // this ensures that if there is some other task with the same priority that is ready
    // it will be in some place in the list before and can be therefore selected
    // before the previous task
    // this means that round-robin is used for the tasks with same priority 
    // priority based scheduling is the main policy and
    // if there are several tasks with the same priority 
    // they will be scheduled by the round-robin algorithm
    if(maxPriorityTask->next) {
       if(maxPriorityTaskPrev) {
           maxPriorityTaskPrev->next = maxPriorityTask->next;
       } 
       else {
           taskList = taskList->next;
       }
       maxPriorityTask->next = NULL;
       endOfTaskList->next = maxPriorityTask;
       endOfTaskList = maxPriorityTask;
    }

    maxPriorityTask->status = STATUS_RUNNING;
    currentActiveTask = maxPriorityTask;

    RestoreContext();

}

static void PassControlToScheduler(void) {
    asm volatile (
        "ijmp" :: "z" (Scheduler) // jump to switch context routine 
    );
}

// timer interrupt activating every 1 ms
ISR(TIMER1_COMPA_vect, ISR_NAKED) {
    // current task interrupted
    // interrupts are disabled when entering ISR
    // saving context for the task from inside this ISR
    // saved status register value will have global interrupt enable bit disabled
    SaveContext();

    // 1 ms passed, reduce waiting time in units for the delayed tasks
    ModifyDelays();

    // invoke scheduler to determine next running task
    // see if any previously delayed task should now be made ready
    PassControlToScheduler();

}


static void InitTaskStackArea(TCB *newTask) {
    void *newTaskSPAfterInit;

    asm volatile (
        "in r18, __SREG__\n" 
        "cli\n"

        // save current stack pointer
        "in r26, __SP_L__\n"
        "in r27, __SP_H__\n"

        // set new task's stack pointer
        "out __SP_L__, %A1\n"
        "out __SP_H__, %B1\n"

        // task function stored as return address
        // when restoring context it will jump and execute
        // code in this function
        "push %A2\n"
        "push %B2\n"

        // push r0
        "ldi r19, 0\n" 
        "push r19\n"

        // push status register
        "ldi r19, 0x80\n" // set global interrupt enable bit
        "push r19\n"

        // push general registers
        "ldi r19, 0\n"
    
        "push r19\n" // r1
        "push r19\n" // r2
        "push r19\n" // r3
        "push r19\n" // r4
        "push r19\n" // r5
        "push r19\n" // r6
        "push r19\n" // r7
        "push r19\n" // r8
        "push r19\n" // r9
        "push r19\n" // r10
        "push r19\n" // r11
        "push r19\n" // r12
        "push r19\n" // r13
        "push r19\n" // r14
        "push r19\n" // r15
        "push r19\n" // r16
        "push r19\n" // r17
        "push r19\n" // r18
        "push r19\n" // r19
        "push r19\n" // r20
        "push r19\n" // r21
        "push r19\n" // r22
        "push r19\n" // r23
        "push r19\n" // r24
        "push r19\n" // r25
        "push r19\n" // r26
        "push r19\n" // r27
        "push r19\n" // r28
        "push r19\n" // r29
        "push r19\n" // r30
        "push r19\n" // r31
        
        // Store new task's stack pointer at return register
        "in %A0, __SP_L__\n"
        "in %B0, __SP_H__\n"

        // restore stack pointer
        "out __SP_L__, r26\n"
        "out __SP_H__, r27\n"

        // restore status register
        "out __SREG__, r18\n"

        // A0B0
        : "=r" (newTaskSPAfterInit) // output operands 
        // A1B1, A2B2
        : "r" (newTask->taskSP), "r" (newTask->function) // input operands
        : "r18", "r19", "r26", "r27" // list of clobbered registers
    );

    newTask->taskSP = newTaskSPAfterInit;
}

TCB *SimpleRTOS_CreateTask(const char *taskName, uint8_t taskPriority, void (*taskFunctionPtr)(void)) {

    TCB *iter = taskList;

    while(iter) {
        // prevent creating tasks with the same name
        if(strncmp(iter->name, taskName, sizeof(iter->name)) == 0)
            return NULL;
        iter = iter->next;
    }

    stackArea -= TASK_STACK_AREA_SIZE; // grows downwards
    
    TCB *task;
    // memory allocated for TCB + task personal stack area
    // grows upwards
    task = stackArea - sizeof(TCB); 

    // start of stack area for new task
    task->taskSP = (void *) task - 1;
    task->next = NULL;
    task->priority = taskPriority;
    task->delayUnits = 0;
    task->function = taskFunctionPtr;
    strncpy(task->name, taskName, sizeof(task->name));
    task->status = STATUS_READY;

    InitTaskStackArea(task);
    
    task->next = taskList;
    taskList = task;
    
    return task;
}

TCB *SimpleRTOS_GetTaskByName(const char *taskName) {
    TCB *iter = taskList;

    while (iter) {
        if (strncmp(iter->name, taskName, sizeof(iter->name)) == 0)
            return iter;
        iter = iter->next;
    }

    return NULL;
}

void SimpleRTOS_SetTaskPriority(const char *taskName, uint8_t newTaskPriority) {
    TCB* task = SimpleRTOS_GetTaskByName(taskName);

    uint8_t statusRegister = SREG;
    cli();

    if(task) 
        task->priority = newTaskPriority;

    SaveContext();

    PassControlToScheduler();

    SREG = statusRegister;
}

int16_t SimpleRTOS_GetTaskPriority(const char *taskName) {
    TCB* task = SimpleRTOS_GetTaskByName(taskName);

    if(task)
        return task->priority;
    
    return -1;
}

void SimpleRTOS_SuspendTask(void) {
    
    uint8_t statusRegister = SREG;
    cli();

    currentActiveTask->status = STATUS_SUSPENDED;

    SaveContext();

    PassControlToScheduler();

    SREG = statusRegister;

}

void SimpleRTOS_ResumeTask(const char *taskName) {
    TCB *taskToResume = SimpleRTOS_GetTaskByName(taskName);

    if(taskToResume) {

        uint8_t statusRegister = SREG;
        cli();

        taskToResume->status = STATUS_READY;

        SaveContext();

        PassControlToScheduler();

        SREG = statusRegister;

    }
}

void SimpleRTOS_DelayTask(uint16_t delayTimeUnits) {
    
    if(delayTimeUnits == 0)
        return;

    currentActiveTask->delayUnits = delayTimeUnits;

    uint8_t statusRegister = SREG;
    cli();

    currentActiveTask->status = STATUS_DELAYED;

    SaveContext();

    PassControlToScheduler();

    SREG = statusRegister;
}

void SimpleRTOS_Init(void) {
 
    stackArea = (void *) RAMEND; 
    currentActiveTask = NULL;
    taskList = NULL;
    endOfTaskList = NULL;

    // create idle task
 
    stackArea -= TASK_STACK_AREA_SIZE; // grows downwards
    
    TCB *task;
    // memory allocated for TCB + task personal stack area
    // grows upwards
    task = stackArea - sizeof(TCB); 

    // start of stack area for new task
    task->taskSP = (void *) task - 1;
    task->next = NULL;
    task->priority = IDLE_TASK_PRIORITY;
    task->delayUnits = 0;
    task->function = IdleTask;
    strncpy(task->name, "IdleTask", sizeof("IdleTask"));
    task->status = STATUS_READY;

    InitTaskStackArea(task);
    
    taskList = task;
    endOfTaskList = task;
    
    InitTimer1Interrupt();
    
}

void SimpleRTOS_Start(void) {
   
    cli();

    TIMSK1 |= TIMER1_ENABLE_COMPARE_MATCH;

    PassControlToScheduler();

}
