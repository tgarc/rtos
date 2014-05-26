#ifndef __DSP__
#define __DSP__

#include "util/macros.h"

#define FILTERSCALE					16384				// FIR filter scale factor
#define WINSCALE						128				// Windowing scale factor
#define FILTERLENGTH				51				// FIR filter length
#define FILTERLENGTH_TIMES2	FILTERLENGTH*2
#define FFTLENGTH						64
#define LOG2_FILTERSCALE		14
#define LOG2_WINSCALE				7
#define LOG2_ADCSCALE				10


// STMicro FFT
void cr4_fft_64_stm32(void *pssOUT, void *pssIN, unsigned short Nbin);

long DSP_Hamming64(void);
// Apply the FIR filter pointed to by h on x
short DSP_FIRFilter(long x);

#endif
