#include <stdio.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"	
#include "inc/lm3s8962.h"
#include "util/fifo.h"

struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;

#define TX_THRESHOLD UART_FIFO_TX4_8
#define RX_THRESHOLD UART_FIFO_RX4_8
#define TX_FIFO_SIZE 16
#define RX_FIFO_SIZE 16
#define UART_FIFO_TYPE ULONG

static FIFO rxFifo;
static FIFO txFifo;
static UART_FIFO_TYPE rxDataBlock[RX_FIFO_SIZE];
static UART_FIFO_TYPE txDataBlock[TX_FIFO_SIZE];

void UARTIOInit(void) {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	FIFOInit(&rxFifo, rxDataBlock, sizeof(rxDataBlock)/sizeof(UART_FIFO_TYPE), sizeof(UART_FIFO_TYPE));
	FIFOInit(&txFifo, txDataBlock, sizeof(txDataBlock)/sizeof(UART_FIFO_TYPE), sizeof(UART_FIFO_TYPE));
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
			(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
			 UART_CONFIG_PAR_NONE));
	UARTFIFOLevelSet(UART0_BASE, TX_THRESHOLD, RX_THRESHOLD);
	IntEnable(INT_UART0);
	IntPrioritySet(INT_UART0, 3); // make this a higher priority to prevent deadlock

	UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_RT | UART_INT_RT);
}

static void swToTxFifo() {
	ULONG fifoItem;
	int status = StartCritical();
	while(UARTSpaceAvail(UART0_BASE) && txFifo.length > 0) {
		FIFOGet(&txFifo, &fifoItem);
		UARTCharPutNonBlocking(UART0_BASE, (char)fifoItem);
	}
	EndCritical(status);
}

static void hwToRxFifo() {
	ULONG fifoItem;
	int status = StartCritical();
	while(UARTCharsAvail(UART0_BASE) && rxFifo.length < rxFifo.capacity) {
		fifoItem = UARTCharGetNonBlocking(UART0_BASE);
		FIFOPut(&rxFifo, &fifoItem);
	}
	EndCritical(status);
}

/* Will not trigger if inside another interrupt of equal or higher priority. This causes
   a deadlock because the software fifo is never cleared. Infinite loop in fputc*/
void UARTIntHandler(void) {
	ULONG intStatus;
	int status = StartCritical();
	intStatus = UARTIntStatus(UART0_BASE, true);

	if(intStatus & UART_INT_TX) {
		UARTIntClear(UART0_BASE, UART_INT_TX); // acknowledge
		swToTxFifo();

		if(txFifo.length == 0){ // software TX FIFO is empty, no need to interrupt again
			UARTIntDisable(UART0_BASE, UART_INT_TX);
		}
	}
	if(intStatus & UART_INT_RX) {
		UARTIntClear(UART0_BASE, UART_INT_RX);
		hwToRxFifo();
	}
	if(intStatus & UART_INT_RT) { // receive fifo timeout
		UARTIntClear(UART0_BASE, UART_INT_RT);
		hwToRxFifo();
	}
	EndCritical(status);
}

static void (*redirect)(char* c,size_t sz) = NULL;
static BOOL doRedirect = FALSE;
static char CharBuff[50];
static charCnt = 0;

 void UARTIOOutputRedirect(void (*cb)(char c,size_t sz)) {
	if(doRedirect == FALSE) {
		doRedirect = TRUE;
	}
	else {
		doRedirect = FALSE;
		if(charCnt) {redirect(CharBuff,sizeof(char)*charCnt); charCnt = 0;}
	}
	
	redirect = cb;
}

int fputc(int c, FILE *f) {
	ULONG character = (ULONG)c;
	UINT failureCount = 20;
	
	if(doRedirect == TRUE) {// && f == stdout) {
		CharBuff[charCnt++] = (char)c;
		if(charCnt == 50 ) {redirect(CharBuff,sizeof(char)*charCnt); charCnt = 0;}
		
		return 0;
	}
	
	while(FIFOPut(&txFifo, &character) == FAILURE) {
		if(failureCount-- == 0) {
			swToTxFifo(); // manually flush
			failureCount = 20;
		}
	}

	UARTIntDisable(UART0_BASE, UART_INT_TX);
	swToTxFifo();
	UARTIntEnable(UART0_BASE, UART_INT_TX);
}

int fgetc(FILE *f) {
	int letter;
	// wait for chars to be available
	while(FIFOGet(&rxFifo, &letter) == FAILURE);

	return(letter);
}

char* fgets(char* str, int num, FILE* stream) {
	int putIndex = 0;

	while(1) {
		int nextChar = fgetc(stream);

		if(nextChar == '\r' || nextChar == '\n') {
			fputc('\r', NULL);
			fputc('\n', NULL);
			break;
		}
		else if(nextChar == '\b') {
			if(putIndex != 0) {
				fputc('\b', NULL);
				fputc(' ', NULL);
				fputc('\b', NULL);

				putIndex--;
			}
		}
		else {
			if(putIndex < num-1) {
				fputc(nextChar, NULL);
				str[putIndex] = nextChar;
				putIndex++;
			}
			else {
				fputc('\a', NULL);
			}
		}
	}

	str[putIndex] = '\0';

	return str;
}

int ferror(FILE *f) {
	return EOF;
}
