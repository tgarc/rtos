//*****************************************************************************
//
// rit128x96x4.h - Prototypes for the driver for the RITEK 128x96x4 graphical
//                   OLED display.
//
// Copyright (c) 2007-2010 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 6075 of the EK-LM3S8962 Firmware Package.
//
// 
//*****************************************************************************
// Additional functions added by Jonathan Valvano 10/27/2010
// added a fixed point output, line plot, bar plot, and dBfs plot

//*****************************************************************************

#ifndef __RIT128X96X4_H__
#define __RIT128X96X4_H__

//*****************************************************************************
//
// Prototypes for the driver APIs.
//
//*****************************************************************************
extern void RIT128x96x4Clear(void);
extern void RIT128x96x4StringDraw(const char *pcStr,
                                    unsigned long ulX,
                                    unsigned long ulY,
                                    unsigned char ucLevel);
extern void RIT128x96x4ImageDraw(const unsigned char *pucImage,
                                   unsigned long ulX,
                                   unsigned long ulY,
                                   unsigned long ulWidth,
                                   unsigned long ulHeight);
extern void RIT128x96x4Init(unsigned long ulFrequency);
extern void RIT128x96x4Enable(unsigned long ulFrequency);
extern void RIT128x96x4Disable(void);
extern void RIT128x96x4DisplayOn(void);
extern void RIT128x96x4DisplayOff(void);

/****************RIT128x96x4DecOut2***************
 output 2 digit signed integer number to ASCII string
 format signed 32-bit 
 range -9 to 99
 Input: signed 32-bit integer, position, level 
 Output: none
 Examples
  82  to "82"
  1   to " 1" 
 -3   to "-3" 
 */ 
extern void RIT128x96x4DecOut2(unsigned long num, unsigned long ulX,
                      unsigned long ulY, unsigned char ucLevel);

extern void RIT128x96x4DecOut5(long num, unsigned long ulX,
                      unsigned long ulY, unsigned char ucLevel);
extern void RIT128x96x4FixOut2(long num, unsigned long ulX,
                      unsigned long ulY, unsigned char ucLevel);
extern void RIT128x96x4UDecOut3(unsigned long num, unsigned long ulX,
                      unsigned long ulY, unsigned char ucLevel);
extern void RIT128x96x4UDecOut4(unsigned long num, unsigned long ulX,
                      unsigned long ulY, unsigned char ucLevel);
extern void  RIT128x96x4FixOut22(long num, unsigned long ulX,
                      unsigned long ulY, unsigned char ucLevel);

/****************Int2Str***************
 converts signed integer number to ASCII string
 format signed 32-bit 
 range -99999 to +99999
 Input: signed 32-bit integer 
 Output: null-terminated string exactly 7 characters plus null
 Examples
  12345 to " 12345"  
 -82100 to "-82100"
   -102 to "  -102" 
     31 to "    31" 
 */ 
extern void Int2Str(long const num, char *string);
/****************Int2Str2***************
 converts signed integer number to ASCII string
 format signed 32-bit 
 range -9 to 99
 Input: signed 32-bit integer 
 Output: null-terminated string exactly 2 characters plus null
 Examples
  82  to "82"
  1   to " 1" 
 -3   to "-3" 
 */ 
extern void Int2Str2(long const n, char *string);

 /****************Fix4Str***************
 converts fixed point number to ASCII string
 format signed 32-bit with resolution 0.0001
 range -9.9999 to +9.9999
 Input: signed 32-bit integer part of fixed point number
 Output: null-terminated string exactly 7 characters plus null
 Examples
  12345 to " 1.2345"  
 -82100 to "-8.2100"
  -1002 to " -.1002" 
     31 to " 0.0031" 
 */ 
extern void Fix4Str(long const num, char *string);

/****************Fix3Str***************
 converts fixed point number to ASCII string
 format signed 32-bit with resolution 0.01
 range -99.999 to +99.999
 Input: signed 32-bit integer part of fixed point number
 Output: null-terminated string exactly 7 characters plus null
 Examples
  12345 to " 12.345"  
 -82100 to "-82.100"
  -1002 to " -1.002" 
     31 to "  0.031"   
 */ 
extern void Fix3Str(long const num, char *string);

/****************Fix2Str***************
 converts fixed point number to ASCII string
 format signed 32-bit with resolution 0.01
 range -999.99 to +999.99
 Input: signed 32-bit integer part of fixed point number
 Output: null-terminated string exactly 8 characters plus null
 Examples
  12345 to " 123.45"  
 -82100 to "-821.00"
   -102 to "  -1.02" 
     31 to "   0.31" 
 */ 
extern void Fix2Str(long const num, char *string);

/****************Fix2Str***************
 converts fixed point number to ASCII string
 format signed 32-bit with resolution 0.01
 range -999.99 to +999.99
 Input: signed 32-bit integer part of fixed point number
 Output: null-terminated string exactly 8 characters plus null
 Examples
  345 to "3.45"  
  100 to "1.00"
   99 to "0.99" 
   31 to "0.31"  
 */ 
void Fix22Str(long const num, char *string);

// *************** RIT128x96x4PlotClear ********************
// Clear the graphics buffer, set X coordinate to 0
// It does not output to display until RIT128x96x4ShowPlot called
// Inputs: ymin and ymax are range of the plot
// four numbers are displayed along left edge of plot
// y0,y1,y2,y3, can be -9 to 99, any number outside this range is skipped
// y3 --          hash marks at number           Ymax
//     |
//    --          hash marks between numbers     Ymin+(5*Yrange)/6
//     |
// y2 --                                         Ymin+(4*Yrange)/6
//     |
//    --                                         Ymin+(3*Yrange)/6
//     |
// y1 --                                         Ymin+(2*Yrange)/6
//     |
//    --                                         Ymin+(1*Yrange)/6
//     |
// y0 --                                         Ymin
// Outputs: none
extern void RIT128x96x4PlotClear(long ymin, long ymax, long y0, long y1, long y2, long y3);
extern void RIT128x96x4PlotReClear(void);

extern void RIT128x96x4PlotPoint(long y);
extern void RIT128x96x4PlotBar(long y);
extern void RIT128x96x4PlotdBfs(long y);
extern void RIT128x96x4PlotNext(void);
extern void RIT128x96x4ShowPlot(void);

///
// Display a message on the oLED
// cDevice is 0 for top, 1 for bottom
// cLine is 0 to 5
// string is an ASCII character string
// lVal is a number to display 
// fix  0 is integer
///
extern void oLED_Message (char cDevice, char cLine, const char *pcStr, long lVal);

// Only prints a string
void 
oLED_StringMessage (char cDevice, char cLine, const char *pcStr);

///
// Initialize the oLED and its semaphore
///
void oLED_Init(void);

#endif // __RIT128X96X4_H__
