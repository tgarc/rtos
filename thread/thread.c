#include "thread.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "inc/lm3s8962.h"
#include "util/macros.h"
#include "util/os.h"
#include "util/list.h"
#include "io/rit128x96x4.h"
#include "io/adc.h"

static TCB* 	currentTCB = NULL;
static TCB* 	head = NULL;
static TCB* 	tail = NULL;
static TCB*		defaultThread = NULL;
static ULONG 	NextThreadID = 0; // 0 reserved for the default thread
static UINT 	numPeriodicTasks = 0;
static UINT   highestPriority = LOWEST_PRIORITY+1;
static UINT   priorityCounts[PRIORITY_RANGE];
static Task 	tasks[MAX_TASKS];

#ifdef OS_DEBUG
static UINT 	threadSwitches = 0;
#endif

// Statically allocated thread/stack space
static list_t activeTCBList;
static TCB		tcbSpace[MAX_THREADS];
static ULONG	Stacks[MAX_THREADS][TCB_STACK_SIZE];
static UCHAR	Available[MAX_THREADS];

list_t* ActiveTCBList(void) {
	return &activeTCBList;
}

#ifdef OS_DEBUG
void PrintNumRuns(void) {
	int i=0;
	TCB* tcb = currentTCB;
	
	for(i=0;i<list_size(&activeTCBList);i++) {
		printf("Thread %d ran %d times\n\r", tcb->id, tcb->numRuns);
		tcb = (TCB*)list_next(tcb);
	}
	printf("\n\r");
}

#endif

static UINT determineHighestPriority(void) {
	int i;
	
	for(i=HIGHEST_PRIORITY;i<LOWEST_PRIORITY;i++) {
		if(priorityCounts[i] != 0) {
			return i;
		}
	}
	
	return LOWEST_PRIORITY+1;
}

/*
 * Moves the currentTCB pointer to the next thread's TCB.
 * This should be called during a context switch
 *
 * return - returns a pointer to the new current TCB after the switch
 */
TCB* changeThread() {
	TCB* lastTCB;
	currentTCB->activePriority = currentTCB->priority; // reset it's priority once it has been run
	
	lastTCB = currentTCB; // Save the current TCB
	currentTCB = (TCB*)list_next(currentTCB); // Grab the next TCB
	
	if (lastTCB->blocked) { // Remove the last TCB from active list if it has been blocked
		list_move(lastTCB->blocked,lastTCB,NULL,NULL);
	}
	
	while(PRIORITY_IS_HIGHER(highestPriority, currentTCB->activePriority) || currentTCB->sleep) {
		if(currentTCB->activePriority > 0) {
			currentTCB->activePriority--; // raise its priority if it wasn't run
		}
		currentTCB = (TCB*)list_next(currentTCB);
	}
	/*// skips sleeping threads
	while(currentTCB->sleep) {
		currentTCB = (TCB*)list_next(currentTCB);
	}*/

#ifdef OS_DEBUG
	threadSwitches++;
	/*switch(currentTCB->id) {
		case 2:
			PC4_ON();
			break;
		case 3:
			PC5_ON();
			break;
		case 4:
			PC6_ON();
			break;
		case 5:
			PC7_ON();
			break;
	}*/
	
	currentTCB->numRuns++;
#endif

	return currentTCB;
}

/*
 * This is just an accessor for the current thread's TCB.
 * It does not perform any changes
 *
 * return - a pointer to the current TCB
 */
TCB* currentThread() {	
/*	switch(currentTCB->id) {
		case 2:
			PC4_OFF();
			break;
		case 3:
			PC5_OFF();
			break;
		case 4:
			PC6_OFF();
			break;
		case 5:
			PC7_OFF();
			break;
	}*/
	return currentTCB;
}

/*
 * Adds a sleep value to the currently running thread.
 * The sleep value of its TCB is decremented every millisecond
 * Until it reaches zero, then the thread is woken up again
 *
 * sleepTimeMs - sleep time in milliseconds
 */
void OS_Sleep(ULONG sleepTimeMS) {
	int status = StartCritical();
	currentTCB->sleep = sleepTimeMS; // sets the sleep time
	priorityCounts[currentTCB->priority]--;
	highestPriority = determineHighestPriority();
	EndCritical(status);
	OS_Suspend(); // actually switches threads
}

/*
 * Task to be invoked every millisecond. This decrements
 * all sleep counters for all sleeping TCBs once. Multiple
 * calls to this will eventually cause the sleep counters
 * for each thread to reach zero
 */
void ThreadSleepMSTick(void) {
	int status = StartCritical();
	UINT i;
	TCB* curr = currentTCB;

	for(i=0;i<list_size(&activeTCBList);i++) {
	  if(curr->sleep) {
			curr->sleep--;
			if(curr->sleep == 0) {
				priorityCounts[currentTCB->priority]++;
				if(PRIORITY_IS_HIGHER(currentTCB->priority, highestPriority)) {
					highestPriority = currentTCB->priority;
				}
			}
		}
	  curr = (TCB*)list_next(curr);
	}

	EndCritical(status);
}

/*
 * Returns the ID of the current thread
 */
ULONG GetCurrentThreadID(void) {
	return currentTCB->id;
}

/*
 * This is used by OS_Kill to unlink the
 * currently running thread's TCB from the
 * list.
 *
 * return - TODO
 */
SF_STATUS RemoveCurrentThread(void) {
	int status = StartCritical();

	// Don't allow removal of the default thread
	if(currentTCB->id == DEFAULT_THREAD_ID) {
		EndCritical(status);
		return (FAILURE);
	}
	
	Available[currentTCB->stackID] = TRUE; // release threads memory
	/* unlink this tcb */
	list_unlink(currentTCB);

	priorityCounts[currentTCB->priority]--;
	highestPriority = determineHighestPriority();
	
	EndCritical(status);
	return (SUCCESS);
}

/*
 * Adding an always running thread enables the system to 
 * keep running even when all other threads are sleeping
 * or killed.
 */
static void defaultTask(void) {
	for(;;) {
		OS_Suspend();//OS_Kill();
	}
}

#ifdef OS_DEBUG
void PrintThreadJitters(void) {
	int i;
	int j;
	for(i=0;i<numPeriodicTasks;i++) {
		printf("Periodic %d\n\r", i);
		printf("\tMin jitter: %d us\n\r", tasks[i].minJitter);
		printf("\tMax jitter: %d us\n\r", tasks[i].maxJitter);
		printf("\tAverage jitter: %d us\n\r", (tasks[i].jitterSums/tasks[i].totalRuns));
		/*for(j=0;j<100;j++) {
			printf("%d: %d ", j, tasks[i].jittersHistogram[j]);
		}*/
	}
	printf("\n\r");
}
#endif

void executeTask(int i, UINT thisTime) {
	if(tasks[i].task != NULL) {
#ifdef OS_DEBUG
		if(tasks[i].firstRun) {
			tasks[i].firstRun = FALSE;
			tasks[i].jitterSums = 0;
			tasks[i].totalRuns = 0;
		}
		else {
			//UINT thisTime = start;
			int period = thisTime - tasks[i].lastTime;
			int jitter = tasks[i].period - period;
			if(thisTime < tasks[i].lastTime) {
				period = (((UINT)-1)-tasks[i].lastTime)+thisTime;
				jitter = tasks[i].period - period;
			}
			
			if(!tasks[i].minAndMaxSet) {
				tasks[i].minJitter = jitter;
				tasks[i].maxJitter = jitter;
				tasks[i].minAndMaxSet = TRUE;
			}
			
			if(jitter < tasks[i].minJitter) {
				tasks[i].minJitter = jitter;
			}
			if(jitter > tasks[i].maxJitter) {
				tasks[i].maxJitter = jitter;
			}
			
			/*tasks[i].totalRuns++;
			tasks[i].jitterSums += jitter;*/
			//tasks[i].jittersHistogram[jitter]++;
				
			
		}
		tasks[i].lastTime = OS_Time();
#endif
		tasks[i].task();
	}
}

void Timer2AIntHandler(void) {
	TIMER2_ICR_R = TIMER_ICR_TATOCINT;	// Acknowledge periodic interrupt
	executeTask(0, OS_Time());
}

// ***************** Timer0A_Init ****************
// Activate Timer0A interrupts to run user task periodically
// Inputs:  task is a pointer to a user function
//          period in usec
// Outputs: none
void Timer2AInit(unsigned short period, unsigned short priority){
	volatile long delay;
	int status = StartCritical();
	SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER2; // 0) activate timer2

	delay = SYSCTL_RCGC1_R;

	TIMER2_CTL_R &= ~0x00000001;					    // 1) disable timer2A during setup
	TIMER2_CFG_R = 0x00000004;								// 2) configure for 16-bit timer mode
	TIMER2_TAMR_R = 2;							          // 3) configure for periodic mode
	TIMER2_TAILR_R = period-1;			          // 4) reload value
	TIMER2_TAPR_R = PERIODIC_INT_PRESCALE;    // 5) timer2A prescale
	TIMER2_ICR_R = 0x00000001;				        // 6) clear timer2A timeout flag
	TIMER2_IMR_R |= 0x00000001;								// 7) arm timeout interrupt
	IntPrioritySet(INT_TIMER2A, priority);
	NVIC_EN0_R |= NVIC_EN0_INT23;							// 9) enable interrupt 23 in NVIC
	TIMER2_CTL_R |= 0x00000001;					      // 10) enable timer2A
	EndCritical(status);
}

// 2nd timer for periodic threads
void Timer2BInit(unsigned short period, unsigned short priority){
	volatile long delay;
	int status = StartCritical();
	SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER2; // 0) activate timer2

	delay = SYSCTL_RCGC1_R;

	TIMER2_CTL_R &= ~0x00000100;					    // 1) disable timer2A during setup
	TIMER2_CFG_R = 0x00000004;								// 2) configure for 16-bit timer mode
	TIMER2_TBMR_R = 2;							          // 3) configure for periodic mode
	TIMER2_TBILR_R = period-1;			          // 4) reload value
	TIMER2_TBPR_R = PERIODIC_INT_PRESCALE;    // 5) timer2A prescale
	TIMER2_ICR_R = 0x00000100;				        // 6) clear timer2A timeout flag
	TIMER2_IMR_R |= 0x00000100;								// 7) arm timeout interrupt
	IntPrioritySet(INT_TIMER2B, priority);
	NVIC_EN0_R |= NVIC_EN0_INT24;							// 9) enable interrupt 23 in NVIC
	TIMER2_CTL_R |= 0x00000100;					      // 10) enable timer2A
	EndCritical(status);
}

void Timer2BIntHandler(void) {
	TIMER2_ICR_R = TIMER_ICR_TBTOCINT; // Acknowledge periodic interrupt
	executeTask(1, OS_Time());
}

// Sets the sleep timer to decrement sleep count at 1ms interval
void Timer1BInit(void){
	volatile long delay;
	int status = StartCritical();
	SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER1; // 0) activate timer2

	delay = SYSCTL_RCGC1_R;

	TIMER1_CTL_R &= ~0x00000100;					    // 1) disable timer2A during setup
	TIMER1_CFG_R = 0x00000004;								// 2) configure for 16-bit timer mode
	TIMER1_TBMR_R = 2;							    // 3) configure for periodic mode
	TIMER1_TBILR_R = 499;			                // 4) 1 ms period
	TIMER1_TBPR_R = 99;                       // 5) 2us timer2A prescale
	TIMER1_ICR_R = 0x00000100;				        // 6) clear timer2A timeout flag
	TIMER1_IMR_R |= 0x00000100;								// 7) arm timeout interrupt
	IntPrioritySet(INT_TIMER1B, 4);
	NVIC_EN0_R |= NVIC_EN0_INT22;							// 9) enable interrupt 23 in NVIC
	TIMER1_CTL_R |= 0x00000100;					      // 10) enable timer2A
	EndCritical(status);
}

void Timer1BIntHandler(void) {
	TIMER1_ICR_R = TIMER_ICR_TBTOCINT; // Acknowledge periodic interrupt
	ThreadSleepMSTick();
}

/*
 * Creates a periodic task using timer2 interrupts. The task will be executed
 * every 'period' count of 10 microseconds
 *
 * task - function pointer to the function to be invoked every periodMs milliseconds
 * period - the period in 10 microsecond units at which the task is to be performed. range: 1-65535
 * priority - priority is value to be specified in the NVIC for this thread
 */
SF_STATUS OS_AddPeriodicThread(void (*task)(void), ULONG period, ULONG priority) {
	int i;
	ASSERT(1 <= period && period <= 65535);
	tasks[numPeriodicTasks].task = task;//periodicTask = task;
#ifdef OS_DEBUG
	tasks[numPeriodicTasks].period = period;
	tasks[numPeriodicTasks].lastTime = 0;
	tasks[numPeriodicTasks].minJitter = 0;
	tasks[numPeriodicTasks].maxJitter = 0;
	tasks[numPeriodicTasks].firstRun = TRUE;
	tasks[numPeriodicTasks].minAndMaxSet = FALSE;
	for(i=0;i<100;i++) {
		tasks[numPeriodicTasks].jittersHistogram[i] = 0;
	}
#endif
	
	numPeriodicTasks++;
	
	if(numPeriodicTasks == 1) {
		Timer2AInit(period, priority);
	}
	else if(numPeriodicTasks == 2) {
		Timer2BInit(period, priority);
	}

	return (SUCCESS);
}

void OSRemovePeriodicThread(void (*task)(void)) {
	TIMER2_CTL_R &= ~0x00000001;
}

/*
 * Adds a new thread to the TCB list and initializes it
 *
 * task - task that new thread executes
 * stackSize - unused
 * priority - unused
 */
SF_STATUS OS_AddThread(void(*task)(void), unsigned long stackSize, unsigned long priority) {
	TCB *newThread;
	UINT stackID;
	int status = StartCritical(); 			// need to change this to semaphores later.

	//** Find some available memory
	for(stackID=0; stackID < MAX_THREADS && !Available[stackID]; stackID++) {};
	if(!Available[stackID]) {				// Check if space was found otherwise return failure
		EndCritical(status); 		
		return (FAILURE);
	}
	
	// Claim the new stack and TCB
	Available[stackID] = FALSE; 							
	newThread = &tcbSpace[stackID];
	
	// First thread is the default thread
	if(NextThreadID == DEFAULT_THREAD_ID) {
		currentTCB = defaultThread = newThread;
	}
	
	//** Insert thread into the scheduler
	list_append(&activeTCBList, newThread, NULL);
	
	//** Initialize the thread	
	newThread->sp = &Stacks[stackID][THREAD_STACK_BOTTOM-(SAVED_REGS-1)];
	newThread->sp[SAVED_PC] = (ULONG)task;
	newThread->sp[SAVED_PSR] = 0x01000000; // sets the T bit
	newThread->task = task;
	newThread->id = NextThreadID++;
	newThread->priority = priority;
	newThread->activePriority = priority;
	newThread->sleep = 0;
	newThread->blocked = FALSE;
	newThread->stackID = stackID;
#ifdef OS_DEBUG
	newThread->numRuns = 0;
#endif
	
	// highest priority is really the lowest number
	if(PRIORITY_IS_HIGHER(priority, highestPriority)) {
		highestPriority = priority;
	}
	
	priorityCounts[priority]++;
	
	EndCritical(status); // All done
	
	return (SUCCESS);
}

void InitializeStacks(void) {
	UINT i;
	
	for(i=0;i<MAX_THREADS;i++) {
		Available[i] = TRUE;
	}

#ifdef OS_DEBUG
	for(i=0;i<MAX_THREADS;i++) {
		Stacks[i][TCB_STACK_SIZE-8] = 0x00000000; 	//R00
		Stacks[i][TCB_STACK_SIZE-7] = 0x01010101;		//R01
		Stacks[i][TCB_STACK_SIZE-6] = 0x02020202;		//R02
		Stacks[i][TCB_STACK_SIZE-5] = 0x03030303;		//R03
		Stacks[i][TCB_STACK_SIZE-16] = 0x04040404;	//R04
		Stacks[i][TCB_STACK_SIZE-15] = 0x05050505;	//R05
		Stacks[i][TCB_STACK_SIZE-14] = 0x06060606;	//R06
		Stacks[i][TCB_STACK_SIZE-13] = 0x07070707;	//R07
		Stacks[i][TCB_STACK_SIZE-12] = 0x08080808;	//R08
		Stacks[i][TCB_STACK_SIZE-11] = 0x09090909;	//R09
		Stacks[i][TCB_STACK_SIZE-10] = 0x10101010;	//R10
		Stacks[i][TCB_STACK_SIZE-9] = 0x11111111;		//R11
		Stacks[i][TCB_STACK_SIZE-4] = 0x12121212;		//R12
	}
#endif
}

static void initializePriorityCounts(void) {
	int i;
	
	for(i=0;i<PRIORITY_RANGE;i++) {
		priorityCounts[i] = 0;
	}
}

//// 
// Disable interrupts, initialize peripherals
////
void OS_Init(void) {
	DisableInterrupts();
	
	SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN |
                 SYSCTL_XTAL_8MHZ);
	
	//Timer0B_Init();
	Timer1A_Init();											 	// global timer
	Timer1BInit();											 	// thread sleep timer
	Timer3A_Init();											 	// For the oneshot timer
	Timer3B_Init(); 											// Debounce timer
	ADC_Open();								 					 	// For SW ADC interrupting
	
	UARTIOInit(); // For interpreter
	LED_Init();	// 8962 Status LED	
	oLED_Init();
	
	// Initialize ethernet communication
	//FCP_Init();
	
	//eDisk_Init(0); // SDC
	//fs_init();			// Initialize the file system	
	
#ifdef DEBUG_PINS
//  GPIOPortC_Init();										 // debug output signals
#endif

	IntPrioritySet(FAULT_PENDSV, 0);
	InitializeStacks();									 // Initialize stack space
	initializePriorityCounts();
	list_init(&activeTCBList);
	
	// Add the default (always running) thread
	OS_AddThread(defaultTask, 128, LOWEST_PRIORITY);	// ensures that default task is always the first one to be executed
	defaultThread->sp = &(Stacks[defaultThread->stackID][THREAD_STACK_BOTTOM]);
	*(defaultThread->sp) = (ULONG)defaultTask;
	
	// Add the sleep timer thread
	//OS_AddPeriodicThread(ThreadSleepMSTick, PERIODIC_INT_1MS, 2);
}

void OS_Launch(ULONG timeSlice) {	
	SysTickInit(timeSlice);
	EnableUserStack();
	StartFirstThread(defaultThread); // Start the default thread
}

Task* GetPeriodicTask(UINT index) {
	return &tasks[index];
}
