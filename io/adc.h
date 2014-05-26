#ifndef __ADC_H__
#define __ADC_H__
#include "util/macros.h"

extern void ADC_Open(void); 
extern unsigned short ADC_In(unsigned int channelNum); 
extern SF_STATUS ADC_Collect(UCHAR channelNum, UINT fs, void (*task)(USHORT));
SF_STATUS ADC_Collect4(UINT fs, void (*task)(USHORT,USHORT,USHORT,USHORT));
void ADC_StopCollect(void);

#endif
