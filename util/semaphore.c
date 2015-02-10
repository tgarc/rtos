// Blocked semaphores with round robin scheduling
// NOTE: there is an assumption that once memory
// is grabbed for a new blocked list that it will never
// be released. i.e. there is no functionality for releasing
// the memory for a semaphore once it is allocated

#include "util/semaphore.h"
#include "thread/thread.h"

#define MAX_SEMAPHORES MAX_THREADS

static list_t	BlockList[MAX_SEMAPHORES];
static UCHAR	Available[MAX_SEMAPHORES];

// Internal functions
static void 		InitializeSpace(void);
static list_t* 		GetNextAvailableList(void);
//static void 		PrintThreads(semaphore_t *s);

static void InitializeSpace(void) {
	UINT listID;
	for(listID=0;listID<MAX_SEMAPHORES;listID++) {
			Available[listID] = TRUE;
	}
}

static list_t* GetNextAvailableList(void) {
	UINT listID;
	for(listID=0; listID < MAX_SEMAPHORES && !Available[listID]; listID++) {};

	if (listID == MAX_SEMAPHORES || !Available[listID]) {
		return NULL;
	}
	
	Available[listID] = FALSE;
	return &BlockList[listID];
}

SF_STATUS OS_InitSemaphore(semaphore_t *s,ULONG val) {
	static UCHAR SpaceInitialized = FALSE;
	list_t *newList;
	
	// Do we need to move blocked threads from the queue here?

	// Make sure list space is initialized
	if (!SpaceInitialized) {InitializeSpace(); SpaceInitialized = TRUE;}

	newList = GetNextAvailableList();
	ASSERT(newList != NULL)

	s->queue = newList;
	list_init(s->queue);
	s->count = val;
	
	return SUCCESS;
}

void OS_Wait(semaphore_t *s)
{
	TCB *thread;
	long status = StartCritical();
	
	if(--(s->count) < 0) {	// Decrement counter and block if new count < 0
		thread = (TCB*)currentThread();
		thread->blocked = s->queue;	// Flag thread as blocked and point to the semaphore queue
		OS_Suspend(); // Set pendSV to switch threads
		// Wait until change thread to remove thread from the active list
	}
	
	EndCritical(status);
}

// static void PrintThreads(semaphore_t *s) {
// 	TCB *head = (TCB*)list_head(s->queue),*thread = head;
// 	
// 	printf("Blocked List\n\r");
// 	do {
// 		printf("%d\n\r",thread->id);
// 		thread = (TCB*)list_next(thread);
// 	} while(thread != head);
// 	printf("\n\r");
// 	
// 	printf("TCB List\n\r");
// 	head = (TCB*)list_head(ActiveTCBList());
// 	thread = head;
// 	do {
// 		printf("%d\n\r",thread->id);
// 		thread = (TCB*)list_next(thread);
// 	} while(thread != head);
// 	printf("\n\r");
// }

void OS_Signal(semaphore_t *s)
{
	TCB *head;
	long status = StartCritical();
	
	if(++(s->count) >= 0) {		// Increment counter and if new count >=0
		// Remove head from blocked threads (if it exists)
		if( (head = (TCB*)list_head(s->queue)) != NULL) { // Get head of blocked list
			head->blocked = FALSE;  // Clear blocked flag
			list_move(ActiveTCBList(),head,NULL,NULL); // Add back to active TCB list
		}
	}
	
	EndCritical(status);
}

SF_STATUS AssertAndRelease(BOOL condition,semaphore_t *semPtr) 
{
	if(!(condition)) { OS_Signal((semPtr)); return (FAILURE); }
}

// void OS_bWait(semaphore_t *s)
// {
// 	USHORT status;
// 	status = StartCritical();
// 	
// 	while(*s == LOCKED) {
// 		EnableInterrupts();
// 		//OS_Suspend();
// 		DisableInterrupts();
// 	}
// 	*s = LOCKED;
// 	EndCritical(status);
// }

// void OS_bSignal(semaphore_t *s)
// {
// 	*s = UNLOCKED;
// }
