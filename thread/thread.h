#ifndef THREAD_H
#define THREAD_H

#include "util/macros.h"
#include "util/list.h"

#define MAX_THREADS 					10
#define MAX_TASKS 						5
#define TCB_STACK_SIZE 				512
#define THREAD_STACK_BOTTOM 	TCB_STACK_SIZE - 1
#define THREAD_STACK_TOP 			0

#define DEFAULT_THREAD_ID 		0

#define	PERIODIC_INT_PRESCALE	499 // 10us in [20ns units]
#define PERIODIC_INT_RELOAD		20	// 20*5us = 0.1 [ms] periodic interrupt period
#define	PERIODIC_INT_1MS			100	// 1ms in PERIODIC_INT_TIMEOUT units

#define HIGHEST_PRIORITY 0
#define LOWEST_PRIORITY 7
#define PRIORITY_RANGE (LOWEST_PRIORITY-HIGHEST_PRIORITY)
#define PRIORITY_IS_HIGHER(p1,p2) (p1<p2)

enum {
	SAVED_R4,
	SAVED_R5,
	SAVED_R6,
	SAVED_R7,
	SAVED_R8,
	SAVED_R9,
	SAVED_R10,
	SAVED_R11,
	SAVED_R0,
	SAVED_R1,
	SAVED_R2,
	SAVED_R3,
	SAVED_R12,
	SAVED_LR,
	SAVED_PC,
	SAVED_PSR,
	SAVED_REGS
};

typedef struct TCB {
	list_node_t _node;
	ULONG* sp;
	void (*task)(void);
	UINT id;
	UINT priority;
	UINT activePriority;
	UINT sleep;
	list_t* blocked;
	UCHAR stackID;
#ifdef OS_DEBUG
	UINT numRuns;
#endif
} TCB;

typedef struct {
	UINT period; // period of periodic task
	UINT timer;
#ifdef OS_DEBUG
	UINT totalRuns;
	UINT jitterSums;
	UINT lastTime; // system time last time this was entered
	int maxJitter;
	int minJitter;
	BOOL firstRun;
	BOOL minAndMaxSet;
	int jittersHistogram[100];
#endif
	void (*task)(void);
} Task;

// ******** OS_Init ************
// initialize operating system, disable interrupts until OS_Launch
// initialize OS controlled I/O: serial, ADC, systick, select switch and timer2 
// input:  none
// output: none
void OS_Init(void);

//******** OS_Launch *************** 
// start the scheduler, enable interrupts
// Inputs: number of 20ns clock cycles for each time slice
//         you may select the units of this parameter
// Outputs: none (does not return)
// In Lab 2, you can ignore the theTimeSlice field
// In Lab 3, you should implement the user-defined TimeSlice field
void OS_Launch(ULONG theTimeSlice);

// ******** OS_Sleep ************
// place this thread into a dormant state
// input:  number of msec to sleep
// output: none
// You are free to select the time resolution for this function
// OS_Sleep(0) implements cooperative multitasking
void OS_Sleep(ULONG sleepTime); 

unsigned long GetCurrentThreadID(void);

//******** OS_AddThread *************** 
// add a foregound thread to the scheduler
// Inputs: pointer to a void/void foreground task
//         number of bytes allocated for its stack
//         priority (0 is highest)
// Outputs: 1 if successful, 0 if this thread can not be added
// stack size must be divisable by 8 (aligned to double word boundary)
// In Lab 2, you can ignore both the stackSize and priority fields
// In Lab 3, you can ignore the stackSize fields
SF_STATUS OS_AddThread(void(*task)(void), unsigned long stackSize, unsigned long priority);

SF_STATUS RemoveCurrentThread(void);
void ThreadSleepMSTick(void);
void SleepCurrentThread(ULONG sleepTimeMS);
list_t* ActiveTCBList(void);

//******** OS_AddPeriodicThread *************** 
// add a background periodic task
// typically this function receives the highest priority
// Inputs: pointer to a void/void background function
//         period given in system time units
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// You are free to select the time resolution for this function
// This task does not have a Thread ID
// In lab 2, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, this command will be called 0 1 or 2 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
SF_STATUS OS_AddPeriodicThread(void (*task)(void), ULONG period, ULONG priority);

#ifdef OS_DEBUG
void PrintNumRuns(void);
void PrintThreadJitters(void);
Task* GetPeriodicTask(UINT index);
#endif

#endif
