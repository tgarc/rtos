#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/systick.h"
#include "driverlib/sysctl.h"
#include "util/macros.h"
#include "util/os.h"
#include "io/ping.h"

#define MAX_PING_HANDLERS 4

typedef struct {
	ULONG peripheral;
	ULONG portBase;
	ULONG pin;
	ULONG pulseStartTime;
	ULONG pulseEndTime;
	BOOL pulseStarted;
	void (*sampleHandler)(ULONG);
} ping_config_t;

static void busyWait(UINT usecs);

static void pingHandler(ping_config_t* ping);
static void ping1Handler(void);
static void ping2Handler(void);
static void ping3Handler(void);
static void ping4Handler(void);

UINT pingHandlersCount = 0;
UINT availableHandlers[MAX_PING_HANDLERS] = { 1, 1, 1, 1 };
ping_config_t pingConfigs[MAX_PING_HANDLERS];
void (*pingInterruptHandlers[MAX_PING_HANDLERS])(void) = {
	ping1Handler,
	ping2Handler,
	ping3Handler,
	ping4Handler
};

/*
 * prepares interrupt and pin for use with
 * the ping device
 *
 * peripheral - peripheral address
 * portBase - port base address
 * pin - pin mask for port
 * sampleHandler - callback to be called when sample is ready
 * 
 * returns -1 if failed. this indicates there are no more slots left
 *     returns a positive number which is the ID to be passed in to
 *     the ping_sample function to get one sample
 */
INT
ping_port_register(ULONG peripheral,
                   ULONG portBase,
                   ULONG pin,
                   void (*sampleHandler)(ULONG)) {
	int status = StartCritical();
	int i;
										 
	if(pingHandlersCount == MAX_PING_HANDLERS) {
		EndCritical(status);
		return -1;
	}
	
	pingHandlersCount++;
	
	for(i=0;i<MAX_PING_HANDLERS;i++) {
		if(availableHandlers[i]) {
			availableHandlers[i] = 0;
			break;
		}
	}
	
	pingConfigs[i].peripheral = peripheral;
	pingConfigs[i].portBase = portBase;
	pingConfigs[i].pin = pin;
	pingConfigs[i].sampleHandler = sampleHandler;
	
	SysCtlPeripheralEnable(peripheral);

	GPIOPortIntRegister(portBase, pingInterruptHandlers[i]);
	GPIOIntTypeSet(portBase, pin, GPIO_BOTH_EDGES);
	
	EndCritical(status);
	
	return i;
}

/*
 * initiate a single reading from the ping sensor
 * 
 * pingID - id of the sensor to read (obtained from ping_port_register)
 * 
 * returns nothing. This is asynchronous in that it will cause the sample
 *     handler registered with ping_port_register will be called and passed
 *     in the sample time.
 */
void 
ping_sample(INT pingID) {
	int peripheral = pingConfigs[pingID].peripheral;
	int portBase = pingConfigs[pingID].portBase;
	int pin = pingConfigs[pingID].pin;
	int status = StartCritical();
			
	// make the pin an output
	GPIOPinTypeGPIOOutput(portBase, pin);
							
	// send a 5 us pulse
	GPIOPinWrite(portBase, pin, 0);
	GPIOPinWrite(portBase, pin, pin);
	OS_BusyWait(5);
	GPIOPinWrite(portBase, pin, 0);
	
	// switch this pin to an input and enable interrupts
	// to wait for the pulse
	GPIOPinTypeGPIOInput(portBase, pin);
	GPIOPinIntEnable(portBase, pin);
	pingConfigs[pingID].pulseStarted = FALSE;
	EndCritical(status);
}

static void ping1Handler(void) {
	pingHandler(&pingConfigs[0]);
}

static void ping2Handler(void) {
	pingHandler(&pingConfigs[1]);
}

static void ping3Handler(void) {
	pingHandler(&pingConfigs[2]);
}

static void ping4Handler(void) {
	pingHandler(&pingConfigs[3]);
}

static void pingHandler(ping_config_t* ping) {
	int peripheral = ping->peripheral;
	int portBase = ping->portBase;
	int pin = ping->pin;
	int pulseStarted = ping->pulseStarted;
	UINT reload = OS_RawTimerReload();
	
	GPIOPinIntClear(portBase, pin);
	
	// this is the rising edge
	if(!pulseStarted && GPIOPinRead(portBase, pin)) {
		ping->pulseStartTime = reload - OS_RawTimerTime();
		ping->pulseStarted = TRUE;
	}
	else if(pulseStarted) {
		ping->pulseStarted = FALSE;
		ping->pulseEndTime = reload - OS_RawTimerTime();
		GPIOPinIntDisable(portBase, pin);
		ping->sampleHandler(OS_TimeDifference(ping->pulseEndTime, ping->pulseStartTime, reload));
	}
}
