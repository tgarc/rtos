#define controller
#ifdef controller
#include <stdio.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "io/adc.h"
#include "util/os.h"
#include "thread/thread.h"
#include "util/semaphore.h"
#include "io/ping.h"
#include "util/fifo.h"
#include "ethernet/MAC.h"
#include "thread/thread.h"

#define V_SOUND 3432 // Units ??

// Controller gains
#define KPw 5
#define KIw 1

// Usable sensor readings in cm
#define IR_MIN 6	
#define IR_MAX 80
#define PING_MIN 4
#define PING_MAX 400
#define IR_MAX_DIFF 20
#define PING_MAX_DIFF 50
#define ABS_MIN_SPEED 10
#define ABS_MAX_SPEED 99
#define ABS_MAX_SPEED_IN_CM  10  // This is just a guess
#define IR_MIN_CM 10
#define IR_MAX_CM	70

#define DIFF_THRESHOLD 10
#define DIST_THRESHOLD 5
#define TURN_SPEED_S	45
#define TURN_SPEED_F	90
#define TURN_SPEED2_S	45
#define TURN_SPEED2_F	99

#define R 4 // Radius of wheel (4.35cm)
#define L 15 // Wheel base (distance between wheel) 15cm

static const long IR_volt_LUT[13] = {977, 927, 853, 713, 512, 403, 338, 285, 233, 189, 158, 140, 124};
static const long IR_rng_LUT[13] = {5, 7, 8, 10, 15, 20, 25, 30, 40, 50, 60, 70, 80};

ULONG ping1;
ULONG ping2;
ULONG ping3;
ULONG ping4;
ULONG ir0;
ULONG ir1;

void sample1Handler(ULONG s) {
	ping1 = (s*V_SOUND)/20000;
}

void sample2Handler(ULONG s) {
	ping2 = (s*V_SOUND)/20000;
}

void sample3Handler(ULONG s) {
	ping3 = (s*V_SOUND)/20000;
}

void sample4Handler(ULONG s) {
	ping4 = (s*V_SOUND)/20000;
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

static void swap(ULONG* s1, ULONG* s2) {
	ULONG temp = *s1;
	
	*s1 = *s2;
	*s2 = temp;
}

static ULONG median(ULONG s1, ULONG s2, ULONG s3) {
	unsigned long result;
	
	if(s1>s2) {
		if(s2 > s3) result = s2;
		else result = (s1>s3) ? s3 : s1;
	}
	else {
		if(s3 > s2) result = s2;
		else result = (s1>s3) ? s1 : s3;
	}
	
	return result;
}

static ULONG ir1FilteredValue(ULONG sample) {
	static BOOL initialized = FALSE;
	static ULONG xMinus2 = 0;
	static ULONG xMinus1 = 0;
	static ULONG medianValue = 0;
	
	if(!initialized) {
		xMinus2 = sample;
		xMinus1 = sample;
		initialized = TRUE;
	}
	
	medianValue = median(sample, xMinus2, xMinus1);
	
	xMinus2 = xMinus1;
	xMinus1 = sample;
	
	return medianValue;
}

static ULONG ir2FilteredValue(ULONG sample) {
	static BOOL initialized = FALSE;
	static ULONG xMinus2 = 0;
	static ULONG xMinus1 = 0;
	static ULONG medianValue = 0;
	
	if(!initialized) {
		xMinus2 = sample;
		xMinus1 = sample;
		initialized = TRUE;
	}
	
	medianValue = median(sample, xMinus2, xMinus1);
	
	xMinus2 = xMinus1;
	xMinus1 = sample;
	
	return medianValue;
}

static ULONG ir3FilteredValue(ULONG sample) {
	static BOOL initialized = FALSE;
	static ULONG xMinus2 = 0;
	static ULONG xMinus1 = 0;
	static ULONG medianValue = 0;
	
	if(!initialized) {
		xMinus2 = sample;
		xMinus1 = sample;
		initialized = TRUE;
	}
	
	medianValue = median(sample, xMinus2, xMinus1);
	
	xMinus2 = xMinus1;
	xMinus1 = sample;
	
	return medianValue;
}

static ULONG ir4FilteredValue(ULONG sample) {
	static BOOL initialized = FALSE;
	static ULONG xMinus2 = 0;
	static ULONG xMinus1 = 0;
	static ULONG medianValue = 0;
	
	if(!initialized) {
		xMinus2 = sample;
		xMinus1 = sample;
		initialized = TRUE;
	}
	
	medianValue = median(sample, xMinus2, xMinus1);
	
	xMinus2 = xMinus1;
	xMinus1 = sample;
	
	return medianValue;
}

static USHORT g_adc0,g_adc1,g_adc2,g_adc3;

void Producer(USHORT adc0, USHORT adc1, USHORT adc2, USHORT adc3) {
	int status = StartCritical();
	g_adc0 = adc0;
	g_adc1 = adc1;
	g_adc2 = adc2;
	g_adc3 = adc3;
	EndCritical(status);
}

// Returns the range in [cm] from the 10-bit ADC value
long IR_voltage2range(USHORT val) {
	int i=0;
	long range;
	
	while(i < 13 && val < IR_volt_LUT[i]) {i++;}; // Find the two points to interpolate
	if (i == 0 || i == 13) range = IR_rng_LUT[i==0?i:12]; // beyond usable range
	else { // Interpolate
		range = ((IR_rng_LUT[i-1]-IR_rng_LUT[i])*(val - IR_volt_LUT[i]) 
					+ IR_rng_LUT[i]*(IR_volt_LUT[i-1]-IR_volt_LUT[i])) / (IR_volt_LUT[i-1]-IR_volt_LUT[i]);
	}
	
	return range;
}

void GetNewWheelSpeeds(void) {
	FCP_Packet_t pkt;
	static long v_prev=0,w_prev=0,e_sum=0;
	static long dW_prev,dE_prev,dN_prev,dS_prev;
	long v,w,e_side,e_front; // translational velocity (cm/s), angular velocity (deg/s), position error
	long v_l,v_r, p1, p2, p3, p4, i1, i2, i3, i4;
	long dW,dE,dN,dS,dNW,dNE,dNNW,dNNE; // distance to object in each 
	long ir[3];
														// cardinal direction relative to robot's reference frame
	long i=0;
	int status;
	char macAddr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	
	// Grab current sensor data
while(1) {	
	
	status = StartCritical();
	p1 = packet.PingData[0] = ping1/10; p2 = packet.PingData[1] = ping2/10; p3 = packet.PingData[2] = ping3/10; p4 = packet.PingData[3] = ping4/10;
	i1 = ADC_In(0); i2 = ADC_In(1); i3 = ADC_In(2); i4 = ADC_In(3);
	EndCritical(status);
	//i++;
	
	pkt.PingData[0] = (p1 > PING_MIN && p3 < PING_MAX) ? p1 : -1; 
	pkt.PingData[2] = (p3 > PING_MIN && p3 < PING_MAX) ? p3 : -1;
	pkt.IRData[0] = IR_voltage2range(ir1FilteredValue(i1)); // Right
	pkt.IRData[1] = IR_voltage2range(ir2FilteredValue(i2)); // Left
	pkt.IRData[2] = IR_voltage2range(ir3FilteredValue(i3));	// Right Front
	pkt.IRData[3] = IR_voltage2range(ir4FilteredValue(i4));	// Left Front
	
	// Convert readings to [cm]
	dW = pkt.IRData[1];//pkt.PingData[0];
	dE = pkt.IRData[0];
	dNW = pkt.IRData[3];
	dNE = pkt.IRData[2];
	dNNW = pkt.IRData[2];
	dNNE = pkt.IRData[3];
	
	v = 5; // Set constant velocity in cm/s
	//if (ABS(dW - dW_prev) > PING_MAX_DIFF || dW < 0) {
// 	if ( dW > IR_MAX_CM || ABS(dW-dW_prev) > 30) {
// 		if ((dW_prev-PING_MIN) < PING_MIN) {
// 			for(i=0;i <6; i++) {
// 				pkt.speed0 = 45;//w = 60;		//too close to wall, saccade maneuver
// 				pkt.speed1 = 70;
// 				MAC_SendData((char*)&pkt, sizeof(pkt), macAddr);
// 				OS_Sleep(100);
// 			}
// 		}
// 		else {
// 			for(i=0;i < 6; i++) {
// 				pkt.speed0 = 45;//w = 60;		//too close to wall, saccade maneuver
// 				pkt.speed1 = 70;
// 				MAC_SendData((char*)&pkt, sizeof(pkt), macAddr);
// 				OS_Sleep(100);
// 			}
// 		}
// 	}
// 	//else if (ABS(dE - dE_prev) > PING_MAX_DIFF || dE < 0) {
// 	else if (dE > IR_MAX_CM || ABS(dE-dE_prev) > 30) {
// 		if ((dE_prev-PING_MIN) < PING_MIN) {
// 			for(i=0;i < 6; i++) {
// 				pkt.speed0 = 70;//w = 60;		//too close to wall, saccade maneuver
// 				pkt.speed1 = 45;
// 				MAC_SendData((char*)&pkt, sizeof(pkt), macAddr);
// 				OS_Sleep(100);
// 			}
// 		}
// 		else {
// 			for(i=0;i < 6; i++) {
// 				pkt.speed0 = 70;//w = 60;		//too close to wall, saccade maneuver
// 				pkt.speed1 = 45;
// 				MAC_SendData((char*)&pkt, sizeof(pkt), macAddr);
// 				OS_Sleep(100);
// 			}
// 		}
// 	}
// 	else {
		e_front = dNW-dNE;
		e_side = dW-dE; // error is the difference of distance between walls
		w = KPw*e_front; // + KIw*(e+e_sum) // Keep robot in middle of two walls
// 		if( dNW < DIST_THRESHOLD ) {
// 			pkt.speed0 = TURN_SPEED2_F;
// 			pkt.speed1 = TURN_SPEED2_S;
// 		}
// 		else if ( dNE < DIST_THRESHOLD ) {
// 			pkt.speed0 = TURN_SPEED2_S;
// 			pkt.speed1 = TURN_SPEED2_F;
// 		}
		if(e_front > DIFF_THRESHOLD) {
			pkt.speed0 = TURN_SPEED_S;
			pkt.speed1 = TURN_SPEED_F;
		}
		else if ( e_front < -DIFF_THRESHOLD ) { // && (dE-dE_prev) < 0) {
		//else if ( (dE-dE_prev) < 2) {
			pkt.speed0 = TURN_SPEED_F;
			pkt.speed1 = TURN_SPEED_S;
		}
		else if ( e_side > DIFF_THRESHOLD) {
			pkt.speed0 = TURN_SPEED_S;
			pkt.speed1 = TURN_SPEED_F;
		}
		else if ( e_side < -DIFF_THRESHOLD) {
			pkt.speed0 = TURN_SPEED_F;
			pkt.speed1 = TURN_SPEED_S;
		}
		else {
			pkt.speed0 = 99;
			pkt.speed1 = 99;
		}
//	}
	//w = 0;
	// Translate to left and right wheel velocities
	// PI/180 ~ 57.30 -> 5730 in 2.2 Fixed point
	// PI ~ 3.14 -> 314 in 2.2 Fixed point
	// 180.00 -> 18000 in 2.2 Fixed point
	// but 2.2FP * 2.2FP -> 4.4FP => 180.00 -> 1,800,000
// 	pkt->speed0 = 50*(314*(2*v*5730 - w*L)/(2*R*1800000)); // left wheel
// 	pkt->speed1 = 50*(314*(2*v*5730 + w*L)/(2*R*1800000)); // right wheel
	//pkt->speed0 = 20*(314*(2*v - w*L)/(2*R*18000)); // left wheel
	//pkt->speed1 = 20*(314*(2*v + w*L)/(2*R*18000)); // right wheel
// 	else {	
// 		pkt.speed0 = 65;
// 		pkt.speed1 = 65;
// 	}
	
	printf("L_Speed: %d, R_Speed: %d\n\r", pkt.speed0, pkt.speed1);
	printf("IR_RB: %d, IR_LB: %d, IR_RF: %d, IR4_LF: %d\r\n", i1,i2,i3,i4);
	printf("Right back ");
	printf("%d\n\r", pkt.IRData[0]);
	printf("Left back ");
	printf("%d\n\r", pkt.IRData[1]);
	
	printf("\n\rRight Front ");
	printf("%d", pkt.IRData[2]);
	printf("\n\r");
	printf("Left front ");
	printf("%d", pkt.IRData[3]);
	printf("\n\r");
	
	MAC_SendData((char*)&pkt, sizeof(pkt), macAddr);
	OS_Sleep(50);
	dE_prev = dE; dW_prev = dW; e_sum += e_front;
	// Truncate values to ends of valid PWM values
// 	if(ABS(pkt->speed0) <= ABS_MIN_SPEED) pkt->speed0 = 0;
// 	else if(ABS(pkt->speed0) >= ABS_MAX_SPEED) pkt->speed0 = (pkt->speed0>0) ? ABS_MAX_SPEED : -ABS_MAX_SPEED;
// 	if(ABS(pkt->speed1) <= ABS_MIN_SPEED) pkt->speed1 = 0;
// 	else if(ABS(pkt->speed1) >= ABS_MAX_SPEED) pkt->speed1 = (pkt->speed1>0) ? ABS_MAX_SPEED : -ABS_MAX_SPEED;
	}
}

void Ping_Collect(void) {
	static BOOL init = FALSE;
	static int pingID1,pingID2,pingID3,pingID4;
	
	if(!init) {
		pingID1 = ping_port_register(SYSCTL_PERIPH_GPIOG, GPIO_PORTG_BASE, GPIO_PIN_0, sample1Handler);
		pingID3 = ping_port_register(SYSCTL_PERIPH_GPIOC, GPIO_PORTC_BASE, GPIO_PIN_5, sample3Handler);
		init = TRUE;
	}
	
	for(;;) {
		ping_sample(pingID1);
		ping_sample(pingID3);
		OS_Sleep(50);
	}
}

static void updateWheelSpeeds(void) {
	char macAddr[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	FCP_Packet_t packet;
	long p1, p2, p3, p4, i1, i2, i3, i4;
	int status;
	
	status = StartCritical();
	p1 = packet.PingData[0] = ping1/10; p2 = packet.PingData[1] = ping2/10; p3 = packet.PingData[2] = ping3/10; p4 = packet.PingData[3] = ping4/10;
	i1 = ADC_In(0); i2 = ADC_In(1); i3 = ADC_In(2); i4 = ADC_In(3);
	EndCritical(status);
	
	// Convert readings to [cm]
	packet.PingData[0] = (p1 > PING_MIN && p3 < PING_MAX) ? p1 : -1; 
	packet.PingData[2] = (p3 > PING_MIN && p3 < PING_MAX) ? p3 : -1;
	packet.IRData[0] = IR_voltage2range(ir1FilteredValue(i1));
	packet.IRData[1] = IR_voltage2range(ir2FilteredValue(i2));
	packet.IRData[2] = IR_voltage2range(ir3FilteredValue(i3));
	packet.IRData[3] = IR_voltage2range(ir4FilteredValue(i4));

//	printf("Ping1: %d, Ping3: %d\n\r", p1, p3);
	//printf("Ping 1: %d, Ping 3: %d\r\n", ping1, ping3);
	//printf("IR1: %d, IR2: %d, IR3: %d, IR4: %d\r\n", i1,i2,i3,i4);
	//printf("Right back ");
	//printf("%d\n\r", packet.IRData[0]);
	printf("Left back ");
	printf("%d\n\r", packet.IRData[1]);
	
	//printf("\n\rRight Front ");
	//printf("%d", packet.IRData[2]);
	printf("\n\r");
	printf("Left front ");
	printf("%d", packet.IRData[3]);
	printf("\n\r");
	
	//GetNewWheelSpeeds(&packet);
	
	MAC_SendData((char*)&packet, sizeof(packet), macAddr);
}

int main() {
	OS_Init();
	
	printf("Starting...\n\r");
	//OS_AddThread(printThread, 128, 3);
	Ethernet_Init();
	//ADC_Collect4(2000, Producer); // Collect IR data
	
	//OS_AddThread(Ping_Collect,128,1); // Collect Ping data
	//OS_AddPeriodicThread(updateWheelSpeeds, PERIODIC_INT_1MS*100, 0); // run every 100ms
	OS_AddThread(GetNewWheelSpeeds,128,0);
	
	OS_Launch(100);
}
#endif