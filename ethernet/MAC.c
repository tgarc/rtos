// MAC.c
// Runs on LM3S8962
// Low-level Ethernet interface

/* This example accompanies the book
   Embedded Systems: Real-Time Operating Systems for the Arm Cortex-M3, Volume 3,  
   ISBN: 978-1466468863, Jonathan Valvano, copyright (c) 2012

   Program 9.1, section 9.3

 Copyright 2012 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains

 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// This code is derived from ethernet.c - Driver for the Integrated Ethernet Controller
// Copyright (c) 2006-2010 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.

#include "lm3s8962.h"
#include "MAC.h"


//*****************************************************************************
//
// The following are defines for the Ethernet Controller PHY registers.
//
//*****************************************************************************
#define PHY_MR0                 0x00000000  // Ethernet PHY Management Register
                                            // 0 - Control
#define PHY_MR1                 0x00000001  // Ethernet PHY Management Register
                                            // 1 - Status
#define PHY_MR2                 0x00000002  // Ethernet PHY Management Register
                                            // 2 - PHY Identifier 1
#define PHY_MR3                 0x00000003  // Ethernet PHY Management Register
                                            // 3 - PHY Identifier 2
#define PHY_MR4                 0x00000004  // Ethernet PHY Management Register
                                            // 4 - Auto-Negotiation
                                            // Advertisement
#define PHY_MR5                 0x00000005  // Ethernet PHY Management Register
                                            // 5 - Auto-Negotiation Link
                                            // Partner Base Page Ability
#define PHY_MR6                 0x00000006  // Ethernet PHY Management Register
                                            // 6 - Auto-Negotiation Expansion
#define PHY_MR16                0x00000010  // Ethernet PHY Management Register
                                            // 16 - Vendor-Specific
#define PHY_MR17                0x00000011  // Ethernet PHY Management Register
                                            // 17 - Mode Control/Status
#define PHY_MR18                0x00000012  // Ethernet PHY Management Register
                                            // 18 - Diagnostic
#define PHY_MR19                0x00000013  // Ethernet PHY Management Register
                                            // 19 - Transceiver Control
#define PHY_MR23                0x00000017  // Ethernet PHY Management Register
                                            // 23 - LED Configuration
#define PHY_MR24                0x00000018  // Ethernet PHY Management Register
                                            // 24 -MDI/MDIX Control
#define PHY_MR27                0x0000001B  // Ethernet PHY Management Register
                                            // 27 - Special Control/Status
#define PHY_MR29                0x0000001D  // Ethernet PHY Management Register
                                            // 29 - Interrupt Status
#define PHY_MR30                0x0000001E  // Ethernet PHY Management Register
                                            // 30 - Interrupt Mask
#define PHY_MR31                0x0000001F  // Ethernet PHY Management Register
                                            // 31 - PHY Special Control/Status
typedef unsigned char	UCHAR;

UCHAR MACaddr[6];

//*****************************************************************************
// Reads from a PHY register.
// Inputs: ucRegAddr is the address of the PHY register to be accessed.
// Outputs: The contents of the PHY register specified by ucRegAddr.
unsigned long PHYRead(unsigned char ucRegAddr){
  // Wait for any pending transaction to complete.
  while(MAC_MCTL_R & MAC_MCTL_START){};
  // Program the PHY register address and initiate the transaction.
  MAC_MCTL_R = (((ucRegAddr << 3) & MAC_MCTL_REGADR_M) | MAC_MCTL_START);
  // Wait for the transaction to complete.
  while(MAC_MCTL_R & MAC_MCTL_START){};
  // Return the PHY data that was read.
  return(MAC_MRXD_R & MAC_MRXD_MDRX_M);
}
//*****************************************************************************
// Reads from a PHY register.
// Inputs: ucRegAddr is the address of the PHY register to be accessed.
//         data to be written
// Outputs: The contents of the PHY register specified by ucRegAddr.
void PHYWrite(unsigned char ucRegAddr, unsigned long data){
  // Wait for any pending transaction to complete.
  while(MAC_MCTL_R & MAC_MCTL_START){};
  MAC_MTXD_R = data & MAC_MTXD_MDTX_M;
  // Program the PHY register address and initiate the transaction.
  MAC_MCTL_R = (((ucRegAddr << 3) & MAC_MCTL_REGADR_M) | MAC_MCTL_WRITE | MAC_MCTL_START);
  // Wait for the transaction to complete.
  while(MAC_MCTL_R & MAC_MCTL_START){};
}

unsigned char* MAC_GetAddress(void) {
	static unsigned char addressLoaded = 0;
	unsigned long addrLo,addrHi;
	
	if(!addressLoaded) {
		addrLo = FLASH_USERREG0_R;
		addrHi = FLASH_USERREG1_R;
		MACaddr[0] = ((addrLo >>  0) & 0xff);
		MACaddr[1] = ((addrLo >>  8) & 0xff);
		MACaddr[2] = ((addrLo >> 16) & 0xff);
		MACaddr[3] = ((addrHi >>  0) & 0xff);
		MACaddr[4] = ((addrHi >>  8) & 0xff);
		MACaddr[5] = ((addrHi >> 16) & 0xff);
		
		addressLoaded = 1;
	}
	return MACaddr;
}

// Initialize Ethernet device, no interrupts
// Inputs: Pointer to 6 bytes MAC address
// Outputs: none
// Will hang if no lock occurs
void MAC_Init(unsigned char *pucMACAddr){
  volatile unsigned long delay;
  unsigned long ulTemp;
  unsigned char status = StartCritical();
  
  unsigned char *pucTemp = (unsigned char *)&ulTemp;
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF+SYSCTL_RCGC2_EPHY0+SYSCTL_RCGC2_EMAC0; // activate port F and Ethernet
  delay = SYSCTL_RCGC2_R;
  GPIO_PORTF_AFSEL_R |= 0x0C;       // enable alt funct on PF3-2, Ethernet LEDs
  GPIO_PORTF_DEN_R |= 0x0C;         // enable digital I/O on PF3-2
  MAC_RCTL_R &= ~MAC_RCTL_RXEN;     // Disable the Ethernet receiver.
  MAC_TCTL_R &= ~MAC_TCTL_TXEN;     // Disable Ethernet transmitter.

    // The MDC clock (2.5MHz max) is divided down from the system clock 
    //      F(mdc) = F(sys) / (2 * (div + 1))
    // For example, 50MHz/(2*(10+1)) = 2.27 MHz.
  MAC_MDV_R = (MAC_MDV_R & MAC_MDV_DIV_M)+10;

  pucTemp[0] = pucMACAddr[0];     // set 48-bit MAC address
  pucTemp[1] = pucMACAddr[1];     
  pucTemp[2] = pucMACAddr[2];
  pucTemp[3] = pucMACAddr[3];
  MAC_IA0_R = ulTemp;
  ulTemp = 0;
  pucTemp[0] = pucMACAddr[4];
  pucTemp[1] = pucMACAddr[5];
  MAC_IA1_R  = ulTemp;

  MAC_TCTL_R |=  MAC_TCTL_DUPLEX    // duplex mode
                +MAC_TCTL_CRC       // create CRC automatically
                +MAC_TCTL_PADEN;    // zero pad if too short

  MAC_RCTL_R |= MAC_RCTL_RSTFIFO;  // flush receiver
  MAC_RCTL_R |= MAC_RCTL_BADCRC;   // Reject bad FCS
	
	//MAC_IM_R |= MAC_IM_RXINTM; // Set to interrupt on frame receive
	//MAC_IACK_R |= MAC_IACK_RXINT; // Clear the interrupt flag
// 	MAC_IM_R |= 	MAC_IM_RXERM 		// Interrupt on rx error
// 							| MAC_IM_FOVM;		// Interrupt on rx overflow
// 	MAC_IACK_R |= MAC_IACK_RXER | MAC_IACK_FOV; // Clear interrupt flags
// 	NVIC_PRI10_R = (NVIC_PRI10_R&0xFF00FFFF)|0x00200000; // bits 21-23; set priority 1
//   NVIC_EN1_R |= NVIC_EN1_INT42;             // enable the ethernet handler in the NVIC
	
	MAC_RCTL_R |= MAC_RCTL_RSTFIFO;
	
  MAC_RCTL_R |= MAC_RCTL_RXEN;     // Enable the Ethernet receiver.
  MAC_TCTL_R |= MAC_TCTL_TXEN;     // Enable Ethernet transmitter.
  MAC_RCTL_R |= MAC_RCTL_RSTFIFO;  // flush receiver again
                               
  do{                         // wait for link
    ulTemp = PHYRead(PHY_MR1);
  } 
  while((ulTemp & PHY_MR1_LINK) == 0);   // check the Link bit
	
	EndCritical(status);
}


//*****************************************************************************
//  Read a packet from the Ethernet controller.
// Inputs: pucBuf is the pointer to the packet buffer.
//         lBufLen is the maximum number of bytes to be read into the buffer.
// Output: negated packet length -n if the packet is too large
//         positive packet length n if ok

// Based on the following table of how the receive frame is stored in the
// receive FIFO, this function will extract a packet from the FIFO and store
// it in the packet buffer that was passed in.
// Format of the data in the RX FIFO is as follows:
// +---------+----------+----------+----------+----------+
// |         | 31:24    | 23:16    | 15:8     | 7:0      |
// +---------+----------+----------+----------+----------+
// | Word 0  | DA 2     | DA 1     | FL MSB   | FL LSB   |
// +---------+----------+----------+----------+----------+
// | Word 1  | DA 6     | DA 5     | DA 4     | DA 3     |
// +---------+----------+----------+----------+----------+
// | Word 2  | SA 4     | SA 3     | SA 2     | SA 1     |
// +---------+----------+----------+----------+----------+
// | Word 3  | FT LSB   | FT MSB   | SA 6     | SA 5     |
// +---------+----------+----------+----------+----------+
// | Word 4  | DATA 4   | DATA 3   | DATA 2   | DATA 1   |
// +---------+----------+----------+----------+----------+
// | Word 5  | DATA 8   | DATA 7   | DATA 6   | DATA 5   |
// +---------+----------+----------+----------+----------+
// | Word 6  | DATA 12  | DATA 11  | DATA 10  | DATA 9   |
// +---------+----------+----------+----------+----------+
// | ...     |          |          |          |          |
// +---------+----------+----------+----------+----------+
// | Word X  | DATA n   | DATA n-1 | DATA n-2 | DATA n-3 |
// +---------+----------+----------+----------+----------+
// | Word Y  | FCS 4    | FCS 3    | FCS 2    | FCS 1    |
// +---------+----------+----------+----------+----------+
//
// Where FL is Frame Length, (FL + DA + SA + FT + DATA + FCS) Bytes.
// Where DA is Destination (MAC) Address.
// Where SA is Source (MAC) Address.
// Where FT is Frame Type (or Frame Length for Ethernet).
// Where DATA is Payload Data for the Ethernet Frame.
// Where FCS is the Frame Check Sequence.
long MAC_PacketGet(unsigned char *pucBuf, long lBufLen){
    unsigned long ulTemp;
    long lFrameLen, lTempLen;
    long i = 0;
    //
    // Read WORD 0 (see format above) from the FIFO, set the receive
    // Frame Length and store the first two bytes of the destination
    // address in the receive buffer.
    //
    ulTemp = MAC_DATA_R;
    lFrameLen = (long)(ulTemp & 0xFFFF);
    pucBuf[i++] = (unsigned char) ((ulTemp >> 16) & 0xff);
    pucBuf[i++] = (unsigned char) ((ulTemp >> 24) & 0xff);
		//ulTemp = MAC_DATA_R; // rest of DA
		
    //
    // Read all but the last WORD into the receive buffer.
    //
    lTempLen = (lBufLen < (lFrameLen - 6)) ? lBufLen : (lFrameLen - 6);
    while(i <= (lTempLen - 4))
    {
        *(unsigned long *)&pucBuf[i] = MAC_DATA_R;
        i += 4;
    }

    //
    // Read the last 1, 2, or 3 BYTES into the buffer
    //
    if(i < lTempLen)
    {
        ulTemp = MAC_DATA_R;
        if(i == lTempLen - 3)
        {
            pucBuf[i++] = ((ulTemp >>  0) & 0xff);
            pucBuf[i++] = ((ulTemp >>  8) & 0xff);
            pucBuf[i++] = ((ulTemp >> 16) & 0xff);
            i += 1;
        }
        else if(i == lTempLen - 2)
        {
            pucBuf[i++] = ((ulTemp >>  0) & 0xff);
            pucBuf[i++] = ((ulTemp >>  8) & 0xff);
            i += 2;
        }
        else if(i == lTempLen - 1)
        {
            pucBuf[i++] = ((ulTemp >>  0) & 0xff);
            i += 3;
        }
    }

    //
    // Read any remaining WORDS (that did not fit into the buffer).
    //
    while(i < (lFrameLen - 2))
    {
        ulTemp = MAC_DATA_R;
        i += 4;
    }

    //
    // If frame was larger than the buffer, return the "negative" frame length
    //
    lFrameLen -= 6;
    if(lFrameLen > lBufLen)
    {
        return(-lFrameLen);
    }

    //
    // Return the Frame Length
    //
    return(lFrameLen);
}
// Receive an Ethernet packet
// Inputs: pointer to a buffer
//         size of the buffer
// Outputs: positive size if received
//          negative number if received but didn't fit
// Will wait if no input
long MAC_ReceiveBlocking(unsigned char *pucBuf, long lBufLen){
  while((MAC_NP_R & MAC_NP_NPR_M) == 0){};   // Wait for a packet
  return MAC_PacketGet( pucBuf, lBufLen);
}

// Receive an Ethernet packet
// Inputs: pointer to a buffer
//         size of the buffer
// Outputs: positive size if received
//          negative number if received but didn't fit
// Will not wait if no input (returns 0 if no input)
long MAC_ReceiveNonBlocking(unsigned char *pucBuf, long lBufLen){
  if((MAC_NP_R & MAC_NP_NPR_M) == 0)return 0;   // return if no packet
  return MAC_PacketGet( pucBuf, lBufLen);        // return packet
}


// 
////*****************************************************************************
//// Send a packet to the Ethernet controller.
////
//// Inputs: pucBuf is the pointer to the packet buffer.
////         lBufLen is number of bytes in the packet to be transmitted.
//// Output: negated packet length -lBufLen if the packet is too large
////         positive packet length lBufLen if ok.
//// Puts a packet into the transmit FIFO of the controller.
////
//// Format of the data in the TX FIFO is as follows:
////
//// +---------+----------+----------+----------+----------+
//// |         | 31:24    | 23:16    | 15:8     | 7:0      |
//// +---------+----------+----------+----------+----------+
//// | Word 0  | DA 2     | DA 1     | PL MSB   | PL LSB   |
//// +---------+----------+----------+----------+----------+
//// | Word 1  | DA 6     | DA 5     | DA 4     | DA 3     |
//// +---------+----------+----------+----------+----------+
//// | Word 2  | SA 4     | SA 3     | SA 2     | SA 1     |
//// +---------+----------+----------+----------+----------+
//// | Word 3  | FT LSB   | FT MSB   | SA 6     | SA 5     |
//// +---------+----------+----------+----------+----------+
//// | Word 4  | DATA 4   | DATA 3   | DATA 2   | DATA 1   |
//// +---------+----------+----------+----------+----------+
//// | Word 5  | DATA 8   | DATA 7   | DATA 6   | DATA 5   |
//// +---------+----------+----------+----------+----------+
//// | Word 6  | DATA 12  | DATA 11  | DATA 10  | DATA 9   |
//// +---------+----------+----------+----------+----------+
//// | ...     |          |          |          |          |
//// +---------+----------+----------+----------+----------+
//// | Word X  | DATA n   | DATA n-1 | DATA n-2 | DATA n-3 |
//// +---------+----------+----------+----------+----------+
////
//// Where PL is Payload Length, (DATA) only
//// Where DA is Destination (MAC) Address
//// Where SA is Source (MAC) Address
//// Where FT is Frame Type (or Frame Length for Ethernet)
//// Where DATA is Payload Data for the Ethernet Frame
//static long  MACPacketSend(unsigned char *pucBuf, long lBufLen){
//    unsigned long ulTemp;
//    long i = 0;
//
//    //
//    // If the packet is too large, return the negative packet length as
//    // an error code.
//    //
//    if(lBufLen > (2048 - 2))
//    {
//        return(-lBufLen);
//    }
//
//    //
//    // Build and write WORD 0 (see format above) to the transmit FIFO.
//    //
//    ulTemp = (unsigned long)(lBufLen - 14);
//    ulTemp |= (pucBuf[i++] << 16);
//    ulTemp |= (pucBuf[i++] << 24);
//    MAC_DATA_R = ulTemp;
//
//    //
//    // Write each subsequent WORD n to the transmit FIFO, except for the last
//    // WORD (if the word does not contain 4 bytes).
//    //
//    while(i <= (lBufLen - 4))
//    {
//        MAC_DATA_R = *(unsigned long *)&pucBuf[i];
//        i += 4;
//    }
//
//    //
//    // Build the last word of the remaining 1, 2, or 3 bytes, and store
//    // the WORD into the transmit FIFO.
//    //
//    if(i != lBufLen)
//    {
//        if(i == (lBufLen - 3))
//        {
//            ulTemp  = (pucBuf[i++] <<  0);
//            ulTemp |= (pucBuf[i++] <<  8);
//            ulTemp |= (pucBuf[i++] << 16);
//            MAC_DATA_R = ulTemp;
//        }
//        else if(i == (lBufLen - 2))
//        {
//            ulTemp  = (pucBuf[i++] <<  0);
//            ulTemp |= (pucBuf[i++] <<  8);
//            MAC_DATA_R = ulTemp;
//        }
//        else if(i == (lBufLen - 1))
//        {
//            ulTemp  = (pucBuf[i++] <<  0);
//            MAC_DATA_R = ulTemp;
//        }
//    }
//
//    //
//    // Activate the transmitter
//    //
//    MAC_TR_R = MAC_TR_NEWTX;
//
//    //
//    // Return the Buffer Length transmitted.
//    //
//    return(lBufLen);
//}
//
////*****************************************************************************
//// Nonblocking Send a packet to the Ethernet controller.
////
//// Inputs: pucBuf is the pointer to the packet buffer.
////         lBufLen is number of bytes in the packet to be transmitted.
//// Output: 0 if Ethernet controller is busy
////         negated packet length -lBufLen if the packet is too large
////         positive packet length lBufLen if ok.
//// Puts a packet into the transmit FIFO of the controller.
////
//// The function will not wait for the transmission to complete.  
//long MAC_TransmitNonBlocking( unsigned char *pucBuf, long lBufLen){
//  if(MAC_TR_R & MAC_TR_NEWTX) return(0);
//  return(MACPacketSend(pucBuf, lBufLen));
//}
//
////*****************************************************************************
//// Blocking Send a packet to the Ethernet controller.
//// If busy, this function will wait for the controller
//// Inputs: pucBuf is the pointer to the packet buffer.
////         lBufLen is number of bytes in the packet to be transmitted.
//// Output: negated packet length -lBufLen if the packet is too large
////         positive packet length lBufLen if ok.
//// Puts a packet into the transmit FIFO of the controller.
////
//// The function will not wait for the transmission to complete.  
//long MAC_TransmitBlocking(unsigned char *pucBuf, long lBufLen){
//  while(MAC_TR_R & MAC_TR_NEWTX){}; // Wait for current packet
//  return(MACPacketSend(pucBuf, lBufLen));
//}


unsigned long Packet[400];   // debug, record last packet sent, room for 1500 bytes
//*****************************************************************************
// Blocking Send data to the Ethernet controller.
// If busy, this function will wait for the controller
// Inputs: pucBuf is the pointer to data (max is 1500)
//         lBufLen is number of bytes in the packet to be transmitted.
//         pucMACAddr[6] is the 48-bit destination address
// Output: negated packet length -lBufLen if the packet is too large
//         positive packet length lBufLen if ok.
// Puts a packet into the transmit FIFO of the controller.
// Uses the internal MAC address for the source address
// The function will not wait for the transmission to complete. 
// Format of the data in the TX FIFO is as follows:
//
// +---------+----------+----------+----------+----------+
// |         | 31:24    | 23:16    | 15:8     | 7:0      |
// +---------+----------+----------+----------+----------+
// | Word 0  | DA 2     | DA 1     | PL MSB   | PL LSB   |
// +---------+----------+----------+----------+----------+
// | Word 1  | DA 6     | DA 5     | DA 4     | DA 3     |
// +---------+----------+----------+----------+----------+
// | Word 2  | SA 4     | SA 3     | SA 2     | SA 1     |
// +---------+----------+----------+----------+----------+
// | Word 3  | FT LSB   | FT MSB   | SA 6     | SA 5     |
// +---------+----------+----------+----------+----------+
// | Word 4  | DATA 4   | DATA 3   | DATA 2   | DATA 1   |
// +---------+----------+----------+----------+----------+
// | Word 5  | DATA 8   | DATA 7   | DATA 6   | DATA 5   |
// +---------+----------+----------+----------+----------+
// | Word 6  | DATA 12  | DATA 11  | DATA 10  | DATA 9   |
// +---------+----------+----------+----------+----------+
// | ...     |          |          |          |          |
// +---------+----------+----------+----------+----------+
// | Word X  | DATA n   | DATA n-1 | DATA n-2 | DATA n-3 |
// +---------+----------+----------+----------+----------+
//
// Where PL is Payload Length, (DATA) only
// Where DA is Destination (MAC) Address
// Where SA is Source (MAC) Address
// Where FT is Frame Type (or Frame Length for Ethernet)
// Where DATA is Payload Data for the Ethernet Frame 
long MAC_SendData(unsigned char *pucBuf, long lBufLen, unsigned char *pucMACAddr) {
  unsigned long ulTemp;  int i; 
  int j=0;    // debug dump

  if(lBufLen > 1500) return -lBufLen;
  while(MAC_TR_R & MAC_TR_NEWTX){}; // Wait for current packet

  ulTemp = (unsigned long)(lBufLen);   //bits 15:0 are data length 
  ulTemp |= (pucMACAddr[0] << 16);     // destination address
  ulTemp |= (pucMACAddr[1] << 24);
  Packet[j++] =                // debug dump
  MAC_DATA_R = ulTemp;

  ulTemp  = (pucMACAddr[2] <<  0);
  ulTemp |= (pucMACAddr[3] <<  8);
  ulTemp |= (pucMACAddr[4] << 16);
  ulTemp |= (pucMACAddr[5] << 24);
  Packet[j++] =                // debug dump
  MAC_DATA_R = ulTemp;

  ulTemp = MAC_IA0_R;          // source address
  Packet[j++] =                // debug dump
  MAC_DATA_R = ulTemp;         // bytes 4:3:2:1

  ulTemp = MAC_IA1_R;          // bits 15-0 are source 6:5
  ulTemp |=  (lBufLen&0xFF00)<<8;  // MSB data length or type  
  ulTemp |=  (lBufLen&0xFF)<<24;   // LSB data length or type
  Packet[j++] =
  MAC_DATA_R = ulTemp;

  i = 0;
  // Write each subsequent WORD n to the transmit FIFO, except for the last
  // WORD (if the word does not contain 4 bytes).
  while(i <= (lBufLen - 4)) {
    Packet[j++] =             // debug dump
    MAC_DATA_R = *(unsigned long *)&pucBuf[i];
    i += 4;
  }
// Build the last word of the remaining 1, 2, or 3 bytes, and store
  if(i != lBufLen){
    if(i == (lBufLen - 3)){
      ulTemp  = (pucBuf[i++] <<  0);
      ulTemp |= (pucBuf[i++] <<  8);
      ulTemp |= (pucBuf[i++] << 16);
      Packet[j++] =             // debug dump
      MAC_DATA_R = ulTemp;
    }
    else if(i == (lBufLen - 2)){
      ulTemp  = (pucBuf[i++] <<  0);
      ulTemp |= (pucBuf[i++] <<  8);
      Packet[j++] =            // debug dump
      MAC_DATA_R = ulTemp;
    }
    else if(i == (lBufLen - 1)){
      ulTemp  = (pucBuf[i++] <<  0);
      Packet[j++] =             // debug dump
      MAC_DATA_R = ulTemp;
    }
  }
  MAC_TR_R = MAC_TR_NEWTX; // Activate
  return(lBufLen);
}


//ethernet globals
unsigned long ulUser0, ulUser1;
unsigned char Me[6];
unsigned char RcvMessage[MAXBUF];

FCP_Packet_t packet;
FCP_Packet_t packet1;
FCP_Packet_t packet2;
FCP_Packet_t packet3;
FCP_Packet_t inPacket;


void DataInit(void);


void Ethernet_Init(void){
	//DataInit(); //for debugging only
    // For the Ethernet Eval Kits, the MAC address will be stored in the
    // non-volatile USER0 and USER1 registers.  
  ulUser0 = FLASH_USERREG0_R;
  ulUser1 = FLASH_USERREG1_R;
  Me[0] = ((ulUser0 >>  0) & 0xff);
  Me[1] = ((ulUser0 >>  8) & 0xff);
  Me[2] = ((ulUser0 >> 16) & 0xff);
  Me[3] = ((ulUser1 >>  0) & 0xff);
  Me[4] = ((ulUser1 >>  8) & 0xff);
  Me[5] = ((ulUser1 >> 16) & 0xff);

  MAC_Init(Me);
	
}

void MakeInPkt(unsigned char *rcvBuf){
	unsigned char* data = rcvBuf+14;
	unsigned char* pktCpy = (unsigned char*)&inPacket;
	int i = 0;
	while(i < sizeof(FCP_Packet_t)){
		*pktCpy = *data;
		pktCpy += 1;
		data += 1;
		 i += 1;
	}
}


//for debugging only
void DataInit(void){
	packet.IRData[0] = 10;
	packet.IRData[1] = 20;
	packet.IRData[2] = 30;
	packet.IRData[3] = 40;
	packet.PingData[0] = 0;
	packet.PingData[1] = 0;
	packet.PingData[2] = 0;
	packet.PingData[3] = 0;
	packet.speed = 10;
	packet.softRight = 0;
	packet.softLeft = 0;
	packet.hardRight = 0;
	packet.hardLeft = 0;
	
	packet1.IRData[0] = 0;
	packet1.IRData[1] = 0;
	packet1.IRData[2] = 0;
	packet1.IRData[3] = 0;
	packet1.PingData[0] = 10;
	packet1.PingData[1] = 20;
	packet1.PingData[2] = 30;
	packet1.PingData[3] = 40;
	
	packet1.speed = -10;
	packet1.softRight = 1;
	packet1.softLeft = 0;
	packet1.hardRight = 0;
	packet1.hardLeft = 0;
	
	
	packet2.IRData[0] = 10;
	packet2.IRData[1] = 20;
	packet2.IRData[2] = 30;
	packet2.IRData[3] = 40;
	packet2.PingData[0] = 0;
	packet2.PingData[1] = 0;
	packet2.PingData[2] = 0;
	packet2.PingData[3] = 0;
	packet2.speed = 50;
	packet2.softRight = 0;
	packet2.softLeft = 1;
	packet2.hardRight = 0;
	packet2.hardLeft = 0;
	
	packet3.IRData[0] = 0;
	packet3.IRData[1] = 0;
	packet3.IRData[2] = 0;
	packet3.IRData[3] = 0;
	packet3.PingData[0] = 10;
	packet3.PingData[1] = 20;
	packet3.PingData[2] = 30;
	packet3.PingData[3] = 40;
	packet3.speed = -90;
	packet3.softRight = 0;
	packet3.softLeft = 0;
	packet3.hardRight = 0;
	packet3.hardLeft = 1;
	
}