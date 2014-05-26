#ifndef __OS_H
#define __OS_H

// fill these depending on your clock        
#define TIME_1MS  1000          
#define TIME_2MS  2*TIME_1MS

#include "util/semaphore.h"

typedef USHORT					G_FIFO_TYPE;
typedef struct {
	semaphore_t DataValid;
	semaphore_t BoxFree;
	ULONG	Data;
}MailBox;

void PrintInterruptTimerStats(void);
void ClearInterruptTimer(void);

// Wait 'reload' number of cycles and then call Handler function
void OS_OneShotTimer(int reload,void (*Handler)(void));

//******** OS_Id *************** 
// returns the thread ID for the currently running thread
// Inputs: none
// Outputs: Thread ID, number greater than zero 
unsigned long OS_Id(void);

//******** OS_AddButtonTask *************** 
// add a background task to run whenever the Select button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed that the user task will run to completion and return
// This task can not spin, block, loop, sleep, or kill
// This task can call OS_Signal  OS_bSignal	 OS_AddThread
// This task does not have a Thread ID
// In labs 2 and 3, this command will be called 0 or 1 times
// In lab 2, the priority field can be ignored
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
int OS_AddButtonTask(void(*task)(void), ULONG priority);

//******** OS_AddDownTask *************** 
// add a background task to run whenever the Down arror button is pushed
// Inputs: pointer to a void/void background function
//         priority 0 is highest, 5 is lowest
// Outputs: 1 if successful, 0 if this thread can not be added
// It is assumed user task will run to completion and return
// This task can not spin block loop sleep or kill
// It can call issue OS_Signal, it can call OS_AddThread
// This task does not have a Thread ID
// In lab 2, this function can be ignored
// In lab 3, this command will be called will be called 0 or 1 times
// In lab 3, there will be up to four background threads, and this priority field 
//           determines the relative priority of these four threads
SF_STATUS OS_AddDownTask(void(*task)(void), unsigned long priority);

void OS_RemoveDownTask(void);

// ******** OS_Kill ************
// kill the currently running thread, release its TCB memory
// input:  none
// output: none
void OS_Kill(void); 

// ******** OS_Suspend ************
// suspend execution of currently running thread
// scheduler will choose another thread to execute
// Can be used to implement cooperative multitasking 
// Same function as OS_Sleep(0)
// input:  none
// output: none
void OS_Suspend(void);
 
// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(unsigned long size);

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(G_FIFO_TYPE data);  

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
G_FIFO_TYPE OS_Fifo_Get(void);

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero  if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void);

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void);

// ******** OS_MailBox_Send ************
// enter mail into the MailBox
// Inputs:  data to be sent
// Outputs: none
// This function will be called from a foreground thread
// It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(unsigned long data);

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
unsigned long OS_MailBox_Recv(void);

// ******** OS_Time ************
// reads a timer value 
// Inputs:  none
// Outputs: time in 20ns units, 0 to max
// The time resolution should be at least 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_TimeDifference have the same resolution and precision 
unsigned long OS_Time(void);
unsigned long OS_RawTimerTime(void);
unsigned long OS_RawTimerReload(void);
void OS_BusyWait(unsigned long waitUs);

void OS_ClearTime(void);
void Timer1A_Init(void);
void Timer3A_Init(void);
void Timer3B_Init(void);
void LED_Init(void);

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 20ns units 
// The time resolution should be at least 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
unsigned long OS_TimeDifference(unsigned long start, unsigned long stop, unsigned long rollover);

#endif
