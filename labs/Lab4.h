#ifndef __LAB4_H__
#define __LAB4_H__

#define ADC_TRIG_NONE				0
#define ADC_TRIG_ALWAYS			1     // Always (continuously sample)
#define ADC_TRIG_ONESHOT		2
#define PLOTMODE_FFT				0
#define PLOTMODE_VOLT				1

void SetTrigger(UCHAR trigType);

// Toggle the FIR filter On/Off, return the status
BOOL ToggleFilter(void);
// Set to voltage or fft plot mode
void SetPlotMode(UCHAR plotmode);

#endif