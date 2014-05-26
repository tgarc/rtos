/*
  FCP - file communication protocal
  tomas garcia
*/

#ifndef __FCP_H__
#define __FCP_H__

#define SIZEOF_IRDATA						16
#define SIZEOF_PINGDATA					16
#define SIZEOF_FCPPACKET				SIZEOF_IRDATA+SIZEOF_PINGDATA

typedef struct FCP_Packet_t{
	unsigned long PingData[4];
	unsigned long IRData[4];
	signed char speed; //-90,-80,-70...-10 = % speed reverse direction
	signed char speed0;
	signed char speed1;
										//0 = both motors stopped
											//10,20,30,...90 = % speed forward direction
	
	//turning directions-only 1 of these can be set per packet sent
	//if one of these set, ^speed will be ignored and turning speed will be set by motor board (doesnt have to be done this way)
	// motors will resume ^speed after turn
	signed char softRight;	//1 if need to turn soft right, 0 otherwise
	signed char softLeft;		
	signed char hardRight;
	signed char hardLeft;
}	FCP_Packet_t;


////
// Initialize the MAC and semaphores
////
void FCP_Init(void);

// Wait for connection from a client and return its MAC address
// input: ca - 6 byte buffer for storing connected client's MAC address
void FCP_Listen(unsigned char *ca);

// Find a server to connect to and return its MAC address
// input: sa - 6 byte buffer for storing server's MAC address
void FCP_Connect(unsigned char *sa);

////
// Send a FCP packet to a particular MAC address
// If currently transmitting, waits until previous data is sent
// packet - pointer to packet containing sensor data
// macAddr - receivers MAC address
////
void FCP_SendPacketBlocking(FCP_Packet_t *packet,unsigned char *macAddr);

////
// Send a FCP packet to a particular MAC address
// returns 0 if transmitter is busy
// packet - pointer to packet containing sensor data
// macAddr - receivers MAC address
// return - 0 if transmission is in process
////
int FCP_SendPacketNonBlocking(FCP_Packet_t *packet,unsigned char *macAddr);

// Read a packet from the HW buffer
// return 1 if successfully received
// return 0 if no packet or receiver is busy
int FCP_ReceiveNonBlocking(FCP_Packet_t *packet);

// Read a packet from the HW buffer
// Blocks until a packet is received
void FCP_ReceiveBlocking(FCP_Packet_t *packet);

////
// Much like ADC_Collect, enables receiving packets on interrupt.
// Each time a packet is received it is copied into the user buffer
// and the handler function is called.
////
void FCP_Accept(void (*SensorDataHandler)(void), FCP_Packet_t *packet);

#endif
