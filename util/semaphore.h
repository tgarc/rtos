#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include "util/list.h"
#include "thread/thread.h"
#include "util/macros.h"

#define LOCKED		0
#define UNLOCKED	1

typedef struct {
	list_t* queue; // List of blocked threads
	long count; // semaphore value
} semaphore_t;

SF_STATUS OS_InitSemaphore(semaphore_t *s,ULONG val);
void OS_Wait(semaphore_t *s);
void OS_Signal(semaphore_t *s);

SF_STATUS AssertAndRelease(BOOL condition,semaphore_t *semPtr);

// Old semaphore stuff
//typedef ULONG semaphore_t;
// void OS_bWait(semaphore_t *s);
// void OS_bSignal(semaphore_t *s);

#endif
