#include "inc/lm3s8962.h"
#include "thread/thread.h"
#include "thread/systick.h"
#include "io/uart_stdio.h"
#include "macros.h"
#include "os.h"
#include "fifo.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "driverlib/interrupt.h"
#include "io/rit128x96x4.h"
#include "driverlib/sysctl.h"
#include "io/adc.h"
#include <stdio.h>

#define PF1 						(*((volatile unsigned long *)0x40025008)) >> 1 // Shift to return only the single bit
#define PE1							(*((volatile unsigned long *)0x40024008)) >> 1 // Shift to return only the single bit
#define G_FIFO_SIZE			32
#define GTIMER_PRESCALE 50 		// prescale in clock cycles [1us]
#define GTIMER_RELOAD 	10000 // reload value in prescale units [10ms]
#define ONESHOTTIMER_PRESCALE  49

static  void						(*DebounceHandler)(void) = NULL;
static	void						(*DownTask)(void) = NULL;
static	void						(*SelectTask)(void) = NULL;
static	void						(*OneShotTimerHandler)(void) = NULL;
static  ULONG						LastPF1, LastPE1;
static	FIFO						gFifo;
static	G_FIFO_TYPE			gFifoBlock[G_FIFO_SIZE];
static	ULONG						gTickCount; // 20ns Cycle counts
static	MailBox					gMailBox;

static void DebounceTimer(int reload,void (*Handler)(void));

ULONG DisabledInterruptsTime=0;
ULONG	MaxDisabledInterruptsTime=0;
void ClearInterruptTimer(void) {
	DisabledInterruptsTime = MaxDisabledInterruptsTime = 0;
}
void PrintInterruptTimerStats(void) {
	printf("Max time interrupts disabled: %ul us\r\n",MaxDisabledInterruptsTime/10);
	printf("Percentage time interrupts disabled: %% %ul us\r\n",DisabledInterruptsTime/10);
	printf("Total Run Time: %% %ul\r\n",5*OS_Time());
}

////
// Initializes edge triggered interrupting on the 'select'
// push button switch and assigns 'task' as the handler
// -T
////
int OS_AddButtonTask(void(*task)(void), ULONG priority) {
	volatile UCHAR i;
	UINT status = StartCritical();
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // Enable GPIO Port F
	
	SelectTask = task;									// Assign aperiodic task
	i = SYSCTL_RCGC2_R; 									// Wait at least 3 clock cycles
	
	GPIO_PORTF_DIR_R &= ~0x02; 						// Make GPIOF bit 1 input
	GPIO_PORTF_DEN_R |= 0x02;							// Enable digital I/O on PortF bit 1
	GPIO_PORTF_PUR_R |= 0x02;							// Pull up to VDD
	GPIO_PORTF_IS_R  &= ~0x02;						// make PFb1 edge sensitive
	GPIO_PORTF_IBE_R |= 0x02;							// both edges
	//GPIO_PORTF_IEV_R &= ~0x02;							// falling edges
	GPIO_PORTF_ICR_R  = 0x02;							// Clear the PFb1 interrupt flag
	GPIO_PORTF_IM_R  |= 0x02;							// Arm PFb1 interrupt
	NVIC_PRI7_R = (NVIC_PRI7_R&0xFF00FFFFF)|(priority << 21);	//Set priority (bits21-23)
	NVIC_EN0_R |= NVIC_EN0_INT30;		  		// Enable PORTF interrupts (interrupt 30)
	
	GPIO_PORTF_ICR_R  = 0x02;							// Clear the PFb1 interrupt flag
	EndCritical(status);
	LastPF1 = PF1;

	return SUCCESS;				//***Should return failure if can't add thread
}

// Adds task for the Down button
SF_STATUS OS_AddDownTask(void(*task)(void), unsigned long priority) {
	volatile UCHAR i;
	UINT status = StartCritical();
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOE; // Enable GPIO Port E
	
	DownTask = task;											// Assign aperiodic task
	i = SYSCTL_RCGC2_R; 									// Wait at least 3 clock cycles
	
	GPIO_PORTE_DIR_R &= ~0x02; 						// Make GPIOE bit 1 input
	GPIO_PORTE_DEN_R |= 0x02;							// Enable digital I/O on PortE bit 1
	GPIO_PORTE_PUR_R |= 0x02;							// Pull up to VDD
	GPIO_PORTE_IS_R  &= ~0x02;						// make PFb1 edge sensitive
	GPIO_PORTE_IBE_R |= 0x02;							// both edges
	GPIO_PORTE_ICR_R |= 0x02;							// Clear the PE1 interrupt flag
	GPIO_PORTE_IM_R  |= 0x02;							// Arm PE1 interrupt
	NVIC_PRI1_R = (NVIC_PRI1_R&0xFFFFFF00)|(priority << 5);	//Set priority (bits5-7)
	NVIC_EN0_R |= NVIC_EN0_INT4;		  		// Enable PORTE interrupts (interrupt 4)
	
	GPIO_PORTE_ICR_R |= 0x02;							// Clear the PE1 interrupt flag	
	LastPE1 = PE1;
	
	EndCritical(status);

	return SUCCESS;				//***Should return failure if can't add thread
}

void OS_RemoveDownTask(void) {
	UINT status = StartCritical();
	GPIO_PORTE_IM_R  &= ~0x02;				// Disarm PE1 interrupt
	NVIC_EN0_R &= ~NVIC_EN0_INT4;		  // Disable PORTE interrupts (interrupt 4)
	EndCritical(status);
}

////
// Reads and re-enables 'down' button
////
static void DebouncePE1(void) {
	LastPE1 = PE1;
	GPIO_PORTE_ICR_R |= 0x02;					// Acknowledge interrupt
	GPIO_PORTE_IM_R  |= 0x02;					// Arm PE1 interrupt
}

////
// Reads and re-enables 'select' button
////
static void DebouncePF1(void) {
	LastPF1 = PF1;
	GPIO_PORTF_ICR_R |= 0x02;					// Acknowledge interrupt
	GPIO_PORTF_IM_R  |= 0x02;					// Arm PFb1 interrupt
}

////
// Handles a 'select' button push
////
void GPIOPortFHandler(void) {
	GPIO_PORTF_IM_R  &= ~0x02;				// Disarm PFb1 interrupt
	// Call function immediately on interrupt
	if (SelectTask != NULL && LastPF1 == 1) {	
		SelectTask();										
	}	
	
	DebounceTimer(10000,DebouncePF1);
}

////
// Handles a 'down' button push
////
void GPIOPortEHandler(void) {
	static BOOL firstTime = TRUE;
	if(firstTime) {
		GPIO_PORTE_ICR_R |= 0x02;
		firstTime = FALSE;
		return;
	}
	
	GPIO_PORTE_IM_R  &= ~0x02;				// Disarm PE1 interrupt	
	// Call function immediately on interrupt
	if (DownTask != NULL && LastPE1 == 1) {	
		DownTask();										
	}
	
	DebounceTimer(10000,DebouncePE1);
}




void Timer0B_Init(void){
	volatile int i;
	UINT status = StartCritical();
	SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0; // 0) activate timer1
	
	gTickCount = 0;
	i--;i++; // wait for timer to start
	
  TIMER0_CTL_R &= ~0x00000100;     // 1) disable timer1A during setup
  TIMER0_CFG_R = 0x00000004;       // 2) configure for 16-bit timer mode
  TIMER0_TBMR_R = 2;      				 // 3) configure for periodic mode
	TIMER0_TBILR_R = GTIMER_PRESCALE-1;     				 // 4) reload value = 1us
  TIMER0_TBPR_R = 49;              // 1 us
  TIMER0_ICR_R = 0x00000100;       // 5) clear timer1A timeout flag
  TIMER0_IMR_R |= 0x00000100;      // 6) arm timeout interrupt
  IntPrioritySet(INT_TIMER0B, 0);
  NVIC_EN0_R |= NVIC_EN0_INT20;    // 8) enable interrupt 19 in NVIC
  TIMER0_CTL_R |= 0x00000100;      // 9) enable timer1A
	
  EndCritical(status);
}

void Timer0BIntHandler(void) {
	TIMER0_ICR_R = TIMER_ICR_TBTOCINT;// acknowledge timer0B timeout
	gTickCount++;
}

void Timer1A_Init(void){
	volatile int i;
	UINT status = StartCritical();
	SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER1; // 0) activate timer1
	
	gTickCount = 0;
	i--;i++; // wait for timer to start
	
  TIMER1_CTL_R &= ~0x00000001;     		// 1) disable timer1A during setup
  TIMER1_CFG_R = TIMER_CFG_16_BIT;    // 2) configure for 16-bit timer mode
  TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;	// 3) configure for periodic mode
	TIMER1_TAILR_R = GTIMER_RELOAD-1;  	// 4) reload value = 1us
  TIMER1_TAPR_R = GTIMER_PRESCALE-1;  // 
	
  TIMER1_ICR_R = 0x00000001;       // 5) clear timer1A timeout flag
  TIMER1_IMR_R |= 0x00000001;      // 6) arm timeout interrupt
  NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|0x00004000; // 7) priority 2
  NVIC_EN0_R |= NVIC_EN0_INT21;    // 8) enable interrupt 21 in NVIC
  TIMER1_CTL_R |= 0x00000001;      // 9) enable timer1A
	
  EndCritical(status);
}

// One shot timer for OS_oneshotTimer
void Timer3A_Init(void){
	volatile UINT delay;
	UCHAR status = StartCritical();
	
	SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER3; 	// 0) activate timer3
	
	delay = SYSCTL_RCGC1_R; 				 				// wait for timer to start
	
  TIMER3_CTL_R &= ~TIMER_CTL_TAEN;     		// 1) disable timer3A during setup
  TIMER3_CFG_R = TIMER_CFG_16_BIT;       		// 2) configure for 16-bit timer mode
  TIMER3_TAMR_R = TIMER_TAMR_TAMR_1_SHOT; 	// 3) configure for one shot mode
	//TIMER3_TAILR_R = reload;     		 				// 4) reload value 6.5536 ms
  TIMER3_TAPR_R = ONESHOTTIMER_PRESCALE;		// 5) 1 us prescale
	
	TIMER3_ICR_R = 0x00000001;       // 5) clear timer1A timeout flag
  TIMER3_IMR_R |= 0x00000001;      // 6) arm timeout interrupt
  NVIC_PRI8_R = (NVIC_PRI8_R&0x00FFFFFF)|0x60000000; // 7) priority 3
  NVIC_EN1_R |= NVIC_EN1_INT35;    // 8) enable interrupt 35 in NVIC
  TIMER3_CTL_R |= TIMER_CTL_TAEN;
	
	EndCritical(status);
}

// One shot timer for debouncing
void Timer3B_Init(void){
	volatile UINT delay;
	UCHAR status = StartCritical();
	
	SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER3; 	// 0) activate timer3
	
	delay = SYSCTL_RCGC1_R; 				 				// wait for timer to start
	
  TIMER3_CTL_R &= ~TIMER_CTL_TBEN;     		// 1) disable timer3B during setup
  TIMER3_CFG_R = TIMER_CFG_16_BIT;       		// 2) configure for 16-bit timer mode
  TIMER3_TBMR_R = TIMER_TBMR_TBMR_1_SHOT; 	// 3) configure for one shot mode
	//TIMER3_TAILR_R = reload;     		 				// 4) reload value 6.5536 ms
  TIMER3_TBPR_R = ONESHOTTIMER_PRESCALE;		// 5) 1 us prescale
	
	TIMER3_ICR_R = TIMER_ICR_TBTOCINT;       // 5) clear timer3B timeout flag
  TIMER3_IMR_R |= TIMER_IMR_TBTOIM;      // 6) arm timeout interrupt
  NVIC_PRI9_R = (NVIC_PRI9_R&0xFFFFFF00)|0x00000060; // 7) priority 3
  NVIC_EN1_R |= NVIC_EN1_INT36;    			// 8) enable interrupt 36 in NVIC
  TIMER3_CTL_R |= TIMER_CTL_TBEN;
	
	EndCritical(status);
}

// For OS_OneShotTimer
void Timer3A_Handler(void) {
	TIMER3_ICR_R = TIMER_ICR_TATOCINT; // Acknowledge timeout
	
	if(OneShotTimerHandler != NULL)
		OneShotTimerHandler();
}

// For debouncing
void Timer3B_Handler(void) {
	TIMER3_ICR_R = TIMER_ICR_TBTOCINT; // Acknowledge timeout
	
	if(DebounceHandler != NULL)
		DebounceHandler();
}

// For general OS use, starts the timer and calls Handler on interrupt
void OS_OneShotTimer(int reload,void (*Handler)(void)) {
	OneShotTimerHandler = Handler;
	TIMER3_TAILR_R = reload;     		 				// reload value 
	TIMER3_IMR_R |= 0x00000001;      				// arm timeout interrupt
	TIMER3_CTL_R |= TIMER_CTL_TAEN;					// Start timer
}

// For debouncing buttons
static void DebounceTimer(int reload,void (*Handler)(void)) {
	DebounceHandler = Handler;
	TIMER3_TBILR_R = reload;     		 				// reload value 
	TIMER3_IMR_R |= TIMER_IMR_TBTOIM;      	// arm timeout interrupt
	TIMER3_CTL_R |= TIMER_CTL_TBEN;					// Start timer
}

void Timer1AIntHandler(void) {
	TIMER1_ICR_R = TIMER_ICR_TATOCINT;// acknowledge timer0A timeout
	gTickCount++;
}

////
// Read the 10ms Timer1A
unsigned long OS_Time(void) {
	return (gTickCount);
}

unsigned long OS_RawTimerTime(void) {
	return TIMER1_TAR_R;
}

unsigned long OS_RawTimerReload(void) {
	return GTIMER_RELOAD-1;
}

void OS_BusyWait(unsigned long waitUs) {
	unsigned long runningCount = 0;
	unsigned long reload = OS_RawTimerReload();
	
	while(runningCount < waitUs) {
		int start = reload - OS_RawTimerTime();
		int end;
		
		do {
			end = reload - OS_RawTimerTime();
		} while(OS_TimeDifference(end/50, start/50, reload) < 1);
		
		runningCount ++;
	}
}

void OS_ClearTime(void) {
	gTickCount = 0;
}

// ******** OS_TimeDifference ************
// Calculates difference between two times
// Inputs:  two times measured with OS_Time
// Outputs: time difference in 20ns units 
// The time resolution should be at least 1us, and the precision at least 12 bits
// It is ok to change the resolution and precision of this function as long as 
//   this function and OS_Time have the same resolution and precision 
ULONG OS_TimeDifference(unsigned long stop, unsigned long start, unsigned long rollover) {
	// detect rollover
	if(stop < start) {
		return stop + (rollover - start);
	}
	
	return (stop-start);
}

// ******** OS_MailBox_Init ************
// Initialize communication channel
// Inputs:  none
// Outputs: none
void OS_MailBox_Init(void) {
	OS_InitSemaphore(&gMailBox.DataValid,FALSE);
	OS_InitSemaphore(&gMailBox.BoxFree,TRUE);
}

// // ******** OS_MailBox_Send ************
// // enter mail into the MailBox
// // Inputs:  data to be sent
// // Outputs: none
// // This function will be called from a foreground thread
// // It will spin/block if the MailBox contains data not yet received 
void OS_MailBox_Send(unsigned long data) {
	//OS_Wait(&gMailBox.BoxFree); // Wait for box to be free
	gMailBox.Data = data;
	//OS_Signal(&gMailBox.DataValid);
}

// ******** OS_MailBox_Recv ************
// remove mail from the MailBox
// Inputs:  none
// Outputs: data received
// This function will be called from a foreground thread
// It will spin/block if the MailBox is empty 
unsigned long OS_MailBox_Recv(void) {
	unsigned long result;
	//OS_Wait(&gMailBox.DataValid);
	result = gMailBox.Data;
	//OS_Signal(&gMailBox.BoxFree);
	return result;
}

// ******** OS_Fifo_Init ************
// Initialize the Fifo to be empty
// Inputs: size
// Outputs: none 
// In Lab 2, you can ignore the size field
// In Lab 3, you should implement the user-defined fifo size
// In Lab 3, you can put whatever restrictions you want on size
//    e.g., 4 to 64 elements
//    e.g., must be a power of 2,4,8,16,32,64,128
void OS_Fifo_Init(unsigned long size) {
	// ignores size for now
	FIFOInit(&gFifo,gFifoBlock,G_FIFO_SIZE,sizeof(G_FIFO_TYPE));
}

// ******** OS_Fifo_Put ************
// Enter one data sample into the Fifo
// Called from the background, so no waiting 
// Inputs:  data
// Outputs: true if data is properly saved,
//          false if data not saved, because it was full
// Since this is called by interrupt handlers 
//  this function can not disable or enable interrupts
int OS_Fifo_Put(G_FIFO_TYPE data) {
	return FIFOPut(&gFifo,&data);
}

// ******** OS_Fifo_Get ************
// Remove one data sample from the Fifo
// Called in foreground, will spin/block if empty
// Inputs:  none
// Outputs: data 
G_FIFO_TYPE OS_Fifo_Get(void) {
	G_FIFO_TYPE result;

	// spin while empty
	while(FIFOGet(&gFifo,&result) == FAILURE){};
	
	return result;
}

// ******** OS_Fifo_Size ************
// Check the status of the Fifo
// Inputs: none
// Outputs: returns the number of elements in the Fifo
//          greater than zero if a call to OS_Fifo_Get will return right away
//          zero or less than zero if the Fifo is empty 
//          zero or less than zero  if a call to OS_Fifo_Get will spin or block
long OS_Fifo_Size(void) {
	return gFifo.length;
}

void OS_Kill(void) {
	RemoveCurrentThread(); // removes it from the TCB structure
	OS_Suspend(); // forces a thread switch and stops this thread
}

void OS_Suspend(void)
{
	IntPendSet(FAULT_PENDSV);
}

void LED_Init(void) {
	volatile UINT delay;
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // Enable GPIO Port F
	delay = SYSCTL_RCGC2_R;
	GPIO_PORTF_DIR_R |= 0x01;
	GPIO_PORTF_DEN_R |= 0x01;
}

void GPIOPortC_Init(void) {
	volatile UINT delay;
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOC; // Enable GPIO Port C
	delay = SYSCTL_RCGC2_R;
	
	GPIO_PORTC_AFSEL_R = 0;
	GPIO_PORTC_DIR_R |= (0x10 | 0x20 | 0x40 | 0x80); // Make GPIOC bits 4-7 output
	GPIO_PORTC_DEN_R |= (0x10 | 0x20 | 0x40 | 0x80);	// Enable digital I/O on PortC bits 4-7
}

unsigned long OS_Id(void) {
	return GetCurrentThreadID();
}
