/*
  FCP - file communication protocal
  tomas garcia
*/

#include "ethernet/MAC.h"
#include "fcp.h"
#include "inc/lm3s8962.h"
#include "util/os.h"
#include "util/semaphore.h" // OS dependent semaphore definition

#define SIZEOF_MACHEADER 	16
#define SIZEOF_BUFFER 		SIZEOF_FCPPACKET+SIZEOF_MACHEADER  // Need additional room for MAC header

static 	unsigned char BroadcastAddr[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static 	unsigned char txBuffer[SIZEOF_BUFFER];
static	unsigned char rxBuffer[SIZEOF_BUFFER];
static  FCP_Packet_t	*rxPacket = NULL;
static  void (*FCP_Handler)(void) = NULL;
static  semaphore_t rxMutex;  
static	semaphore_t txMutex;

static  unsigned char ReceiverErrors = 0;
static  unsigned char OverFlowErrors = 0;
static  unsigned char PacketTooLargeErrors = 0;
static  unsigned char TXerrors = 0;

void FCP_Init(void) {
	OS_InitSemaphore(&txMutex,UNLOCKED);
	OS_InitSemaphore(&rxMutex,UNLOCKED);
	MAC_Init(MAC_GetAddress());
}

// Find a server to connect to and return its MAC address
void FCP_Connect(unsigned char *sa) {
		// Wait for a response from the network
		do {
			MAC_SendData(txBuffer,0,BroadcastAddr);
			OS_Sleep(50); // Wait 50ms
		}while((MAC_NP_R & MAC_NP_NPR_M) == 0); 
		
		MAC_PacketGet(txBuffer, 12); // Copy the received message up to the source address
		
		// Copy the server's address into user buffer
		sa[0] = txBuffer[6];
		sa[1] = txBuffer[7];
		sa[2] = txBuffer[8];
		sa[3] = txBuffer[9];
		sa[4] = txBuffer[10];
		sa[5] = txBuffer[11];	
}

// Wait for connection from a client and return its MAC address
void FCP_Listen(unsigned char *ca) {	
	while((MAC_NP_R & MAC_NP_NPR_M) == 0){};   // Wait for a packet
	MAC_PacketGet(rxBuffer, 12); // Just copy up to the source address
	
	// Copy the client's address into user buffer
	ca[0] = rxBuffer[6];
	ca[1] = rxBuffer[7];
	ca[2] = rxBuffer[8];
	ca[3] = rxBuffer[9];
	ca[4] = rxBuffer[10];
	ca[5] = rxBuffer[11];
}

int FCP_SendPacketNonBlocking(FCP_Packet_t *packet,unsigned char *macAddr) {
	int i,txByteCount=0;
	unsigned char *ucPtr;

	if(txMutex.count <= 0) return 0;	// Make sure mutex has been released
	if(MAC_TR_R & MAC_TR_NEWTX) return 0; // Make sure hardware is not transmitting
	
	OS_Wait(&txMutex);
	
	// Cast and copy the Ping data
	ucPtr = (unsigned char *)packet->PingData;
	for(i=0;i<SIZEOF_PINGDATA;i++) { txBuffer[txByteCount++] = ucPtr[i]; }

	// Cast and copy the IR data
	ucPtr = (unsigned char *)packet->IRData;
	for(i=0;i<SIZEOF_IRDATA;i++) { txBuffer[txByteCount++] = ucPtr[i]; }

	// Broadcast over MAC
	i = MAC_SendData(txBuffer,SIZEOF_FCPPACKET,macAddr);
	if(i < 0) {
		PacketTooLargeErrors++;
	}
	
	OS_Signal(&txMutex);
	
	return 1;
}

void FCP_SendPacketBlocking(FCP_Packet_t *packet,unsigned char *macAddr) {
	int i,txByteCount=0;
	unsigned char *ucPtr;

	OS_Wait(&txMutex);
	
	// Cast and copy the Ping data
	ucPtr = (unsigned char *)packet->PingData;
	for(i=0;i<SIZEOF_PINGDATA;i++) { txBuffer[txByteCount++] = ucPtr[i]; }

	// Cast and copy the IR data
	ucPtr = (unsigned char *)packet->IRData;
	for(i=0;i<SIZEOF_IRDATA;i++) { txBuffer[txByteCount++] = ucPtr[i]; }

	// Broadcast over MAC
	i = MAC_SendData(txBuffer,SIZEOF_FCPPACKET,macAddr);
	if(i < 0) {
		PacketTooLargeErrors++;
	}
	OS_Signal(&txMutex);
}

void Ethernet_Handler(void) {
	int i,rxByteCount=0;
	unsigned char *ucPtr;
	
// 	if(MAC_RIS_R & MAC_IACK_RXER) { // Receiver Errors (like bad CRC)
// 		MAC_IACK_R |= MAC_IACK_RXER;
// 		ReceiverErrors++;
// 	}
// 	if(MAC_RIS_R & MAC_IACK_FOV) {  // RX Overflow Error
// 		MAC_IACK_R |= MAC_IACK_FOV; // Acknowledge interrupt
// 		MAC_RCTL_R |= MAC_RCTL_RSTFIFO; // flush FIFO
// 		OverFlowErrors++;
// 	}
// 	
// 	if(MAC_RIS_R & MAC_IACK_TXER) {  // TX Error
// 		TXerrors++; 
// 	}
	//MAC_IACK_TXEMP          0x00000004  // Clear Transmit FIFO Empty
	
	MAC_IACK_R |= MAC_IACK_RXINT; // Acknowledge frame received interrupt
	MAC_PacketGet(rxBuffer,SIZEOF_BUFFER); // Copy received data
	
	if(FCP_Handler != NULL && rxPacket != NULL) {
		rxByteCount=14; // Skip the first 14 bytes to get directly to the payload
		
		// Copy ping data into the handler's buffer
		ucPtr = (unsigned char *)rxPacket->PingData;
		for(i=0; i<SIZEOF_PINGDATA; i++) { ucPtr[i] = rxBuffer[rxByteCount++]; }

		// Copy IR data into the handler's buffer
		ucPtr = (unsigned char *)rxPacket->IRData;
		for(i=0;i<SIZEOF_IRDATA;i++) { ucPtr[i] = rxBuffer[rxByteCount++] ; }
		
		FCP_Handler();
	}
}

// Read a packet from the HW buffer
int FCP_ReceiveNonBlocking(FCP_Packet_t *packet) {
	int i,rxByteCount;
	unsigned char *ucPtr;
	
	if(!(MAC_NP_R & MAC_NP_NPR_M)) { // Make sure theres a frame
		return 0;
	}
	if(rxMutex.count <= 0) {
		return 0;
	}
	
	OS_Wait(&rxMutex);
		
	// Copy the data from the HW fifo to the receive buffer
	MAC_PacketGet(rxBuffer,SIZEOF_BUFFER);

	rxByteCount=14; // Skip the first 14 bytes to get directly to the payload
	
	// Copy ping data into the handler's buffer
	ucPtr = (unsigned char *)packet->PingData;
	for(i=0; i<SIZEOF_PINGDATA; i++) { ucPtr[i] = rxBuffer[rxByteCount++]; }

	// Copy IR data into the handler's buffer
	ucPtr = (unsigned char *)packet->IRData;
	for(i=0;i<SIZEOF_IRDATA;i++) { ucPtr[i] = rxBuffer[rxByteCount++] ; }

	OS_Signal(&rxMutex);
	
	return 1;
}

// Read a packet from the HW buffer
void FCP_ReceiveBlocking(FCP_Packet_t *packet) {
	int i,rxByteCount;
	unsigned char *ucPtr;
	
	OS_Wait(&rxMutex);
		
	// Copy the data from the HW fifo to the receive buffer
	MAC_PacketGet(rxBuffer,SIZEOF_BUFFER);

	rxByteCount=14; // Skip the first 14 bytes to get directly to the payload
	
	// Copy ping data into the handler's buffer
	ucPtr = (unsigned char *)packet->PingData;
	for(i=0; i<SIZEOF_PINGDATA; i++) { ucPtr[i] = rxBuffer[rxByteCount++]; }

	// Copy IR data into the handler's buffer
	ucPtr = (unsigned char *)packet->IRData;
	for(i=0;i<SIZEOF_IRDATA;i++) { ucPtr[i] = rxBuffer[rxByteCount++] ; }

	OS_Signal(&rxMutex);
	
}

// Much like ADC_Collect, enables receiving packets on interrupt.
// Each time a packet is received it is copied into the user buffer
// and the handler function is called.
void FCP_Accept(void (*SensorDataHandler)(void), FCP_Packet_t *packet) {
	FCP_Handler = SensorDataHandler;
	rxPacket = packet;
	
	/// Enable interrupts
	MAC_RCTL_R &= ~MAC_RCTL_RXEN;     // Disable the Ethernet receiver.
	MAC_IM_R |= MAC_IM_RXINTM; 				// Set to interrupt on frame receive
	NVIC_PRI10_R = (NVIC_PRI10_R&0xFF00FFFF)|0x00200000; 
																		// bits 21-23; set priority 1
  NVIC_EN1_R |= NVIC_EN1_INT42;     // enable the ethernet handler in the NVIC
	MAC_IACK_R |= MAC_IACK_RXINT; 		// Clear the interrupt flag
	MAC_RCTL_R |= MAC_RCTL_RSTFIFO;		// Flush the FIFO
	MAC_RCTL_R |= MAC_RCTL_RXEN;     	// Enable the Ethernet receiver.
}

// void FCP_Receive(FCP_Packet_t *packet){
//   int i;
// 	unsigned long ulTemp,ulFrameLen;
// 	
// 	OS_Wait(&rxMutex);
// 	
// 	// Don't care about header data
// 	for(i=0;i<4;i++)	
// 		ulTemp = MAC_DATA_R;
// 		
// 	// Copy ping data into the handler's buffer
// 	for(i=0; i<4; i++) 
// 		packet->PingData[i] = MAC_DATA_R;

// 	// Copy IR data into the handler's buffer
// 	for(i=0;i<4;i++) 
// 		packet->IRData[i] = MAC_DATA_R;

// 	ulTemp = MAC_DATA_R; // Should be just one more word..

// 	OS_Signal(&rxMutex);
// }


// void FCP_SendPacketBlocking(FCP_Packet_t *packet){
//   unsigned long ulTemp;
//   int i; 

//   OS_Wait(&txMutex); // Wait for any previous transmitters

//   while(MAC_TR_R & MAC_TR_NEWTX){}; // Wait for current packet

//   ulTemp = (unsigned long)(SIZEOF_FCPPACKET);   //bits 15:0 are data length 
//   ulTemp |= (BroadcastAddr[0] << 16);     // destination address
//   ulTemp |= (BroadcastAddr[1] << 24);
//   MAC_DATA_R = ulTemp;

//   ulTemp  = (BroadcastAddr[2] <<  0);
//   ulTemp |= (BroadcastAddr[3] <<  8);
//   ulTemp |= (BroadcastAddr[4] << 16);
//   ulTemp |= (BroadcastAddr[5] << 24);
//   MAC_DATA_R = ulTemp;

//   ulTemp = MAC_IA0_R;          // source address
//   MAC_DATA_R = ulTemp;         // bytes 4:3:2:1

//   ulTemp = MAC_IA1_R;          // bits 15-0 are source 6:5
//   ulTemp |=  (SIZEOF_FCPPACKET&0xFF00)<<8;  // MSB data length or type  
//   ulTemp |=  (SIZEOF_FCPPACKET&0xFF)<<24;   // LSB data length or type
//   MAC_DATA_R = ulTemp;

//   // Cast and copy the Ping data
// 	for(i=0;i<SIZEOF_PINGDATA/4;i++)
// 		{ MAC_DATA_R = *(unsigned long *)&packet->PingData[i]; }

// 	// Cast and copy the IR data
// 	for(i=0;i<SIZEOF_IRDATA/4;i++)
// 		{ MAC_DATA_R = *(unsigned long *)&packet->IRData[i]; }
// 	
// 	// Build the last word of the remaining 1, 2, or 3 bytes, and store (not necessary here)

// 	MAC_TR_R = MAC_TR_NEWTX; // Activate

// 	OS_Signal(&txMutex);
// }

// SF_STATUS FCP_SendPacketNonBlocking(FCP_Packet_t *packet){
//   unsigned long ulTemp;
//   int i; 

//   ASSERT(txMutex.count > 0); // Make sure there is no current transmission

//   ASSERT(!(MAC_TR_R & MAC_TR_NEWTX)); // Make sure hardware transmission is complete

//   ulTemp = (unsigned long)(SIZEOF_FCPPACKET);   //bits 15:0 are data length 
//   ulTemp |= (BroadcastAddr[0] << 16);     // destination address
//   ulTemp |= (BroadcastAddr[1] << 24);
//   MAC_DATA_R = ulTemp;

//   ulTemp  = (BroadcastAddr[2] <<  0);
//   ulTemp |= (BroadcastAddr[3] <<  8);
//   ulTemp |= (BroadcastAddr[4] << 16);
//   ulTemp |= (BroadcastAddr[5] << 24);
//   MAC_DATA_R = ulTemp;

//   ulTemp = MAC_IA0_R;          // source address
//   MAC_DATA_R = ulTemp;         // bytes 4:3:2:1

//   ulTemp = MAC_IA1_R;          // bits 15-0 are source 6:5
//   ulTemp |=  (SIZEOF_FCPPACKET&0xFF00)<<8;  // MSB data length or type  
//   ulTemp |=  (SIZEOF_FCPPACKET&0xFF)<<24;   // LSB data length or type
//   MAC_DATA_R = ulTemp;

//   // Cast and copy the Ping data
// 	for(i=0;i<SIZEOF_PINGDATA/4;i++)
// 		{ MAC_DATA_R = *(unsigned long *)&packet->PingData[i]; }

// 	// Cast and copy the IR data
// 	for(i=0;i<SIZEOF_IRDATA/4;i++)
// 		{ MAC_DATA_R = *(unsigned long *)&packet->IRData[i]; }
// 	
// 	// Build the last word of the remaining 1, 2, or 3 bytes, and store (not necessary here)

// 	MAC_TR_R = MAC_TR_NEWTX; // Activate

// 	OS_Signal(&txMutex);
// 	
//   return SUCCESS;
// }
