#ifdef PING_TEST

#include <stdio.h>
#include <string.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "io/adc.h"
#include "util/os.h"
#include "thread/thread.h"
#include "util/semaphore.h"
#include "io/ping.h"
#include "util/fifo.h"

#define V_SOUND 3432

ULONG distance1;
ULONG distance2;
ULONG distance3;
ULONG distance4;
ULONG ir0;
ULONG ir1;

void Ethernet_Handler(void) {
}

void sample1Handler(ULONG s) {
	distance1 = (s*V_SOUND)/20000;
}

void sample2Handler(ULONG s) {
	distance2 = (s*V_SOUND)/20000;
}

void sample3Handler(ULONG s) {
	distance3 = (s*V_SOUND)/20000;
}

void sample4Handler(ULONG s) {
	distance4 = (s*V_SOUND)/20000;
}

static void pingThread(void) {
	INT pingID1 = ping_port_register(SYSCTL_PERIPH_GPIOG, GPIO_PORTG_BASE, GPIO_PIN_0, sample1Handler);
	INT pingID2 = ping_port_register(SYSCTL_PERIPH_GPIOF, GPIO_PORTF_BASE, GPIO_PIN_1, sample2Handler);
	INT pingID3 = ping_port_register(SYSCTL_PERIPH_GPIOC, GPIO_PORTC_BASE, GPIO_PIN_5, sample3Handler);
	INT pingID4 = ping_port_register(SYSCTL_PERIPH_GPIOB, GPIO_PORTB_BASE, GPIO_PIN_4, sample4Handler);
	
	// samples every 1/10 second
	for(;;) {
		ping_sample(pingID1);
		ping_sample(pingID2);
		ping_sample(pingID3);
		ping_sample(pingID4);
		OS_Sleep(100);
	}
}

static void printDistanceSample(ULONG distance) {
	ULONG intPart;
	ULONG fracPart;
	
	intPart = distance/1000;
	fracPart = distance%1000;
	printf("distance to target is: %d.%03d meters\n\r", intPart, fracPart);
}

static ULONG irSampleToDistance(ULONG sample) {
	
}

static void printThread(void) {
	ULONG counter = 0;
	
	for(;;) {
		OS_Sleep(1000);
		/*printf("%d\n\r", counter++);
		printf("Sensor 1 ");
		printDistanceSample(distance1);
		printf("Sensor 2 ");
		printDistanceSample(distance2);
		printf("Sensor 3 ");
		printDistanceSample(distance3);
		printf("Sensor 4 ");
		printDistanceSample(distance4);*/
		printf("IR 0 ");
		printf("%d\n\r", ADC_In(0));
		printf("IR 1 ");
		printf("%d", ADC_In(1));
		printf("\n\r");
	}
}

int main() {
	OS_Init();

	printf("Starting...\n\r");
	OS_AddThread(pingThread, 128, 0);
	OS_AddThread(printThread, 128, 3);
	
	OS_Launch(200);
}
#endif
