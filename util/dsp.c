#include "util/dsp.h"
#include "io/rit128x96x4.h"

// FIR filter coefficients in signed Q17.14 fp format
static const long
h[51]=
{0,-7,-45,-64,5,78,-46,-355,-482,-138,329,177,-722,-1388,-767,697,1115,-628,
 -2923,-2642,1025,4348,1820,-8027,-19790,56862,-19790,-8027,1820,4348,1025,
 -2642,-2923,-628,1115,697,-767,-1388,-722,177,329,-138,-482,-355,-46,78,5,
 -64,-45,-7,0
};

//hamming window LUT in signed Q1.6
static const char
hamming64[FFTLENGTH] =
{
 +10, +10, +11, +12, +14, +17, +20, +24, +28, +32, +37, +42, +47, +53, +58, +64,
 +70, +76, +82, +87, +93, +98,+103,+108,+112,+116,+119,+122,+124,+126,+127,+127,
+127,+127,+126,+124,+122,+119,+116,+112,+108,+103, +98, +93, +87, +82, +76, +70,
 +64, +58, +53, +47, +42, +37, +32, +28, +24, +20, +17, +14, +12, +11, +10, +10
};

long 
DSP_Hamming64(void) {
	static UINT index=0;
	return hamming64[(index == FFTLENGTH)? (index=0):index++];
}

////
// Implements FIR filter with a circular buffer
// /input h - filter impulse response
// /input x - new sample
// /return y[n] the filtered output
// delayLine contains the M+1 most recent samples, 
// i.e., delayLine = {x[n],x[n-1],x[n-2],...,x[n-(M-1)],x[n-M]}
//
// Credit to the following source for providing a starting point:
// http://ptolemy.eecs.berkeley.edu/eecs20/week12/implementation.html
////
short 
DSP_FIRFilter(long x) {
	static long delayLine[FILTERLENGTH_TIMES2] = {0};
	static UINT n=FILTERLENGTH; // current sample number mod M
	UINT m; 
	long long result=0;

	delayLine[n] = delayLine[n-FILTERLENGTH] = x;		// insert sample into delay line

	for(m=0;m < FILTERLENGTH;m++) {
		result +=delayLine[n-m]*h[m];
	}

	if (++n == FILTERLENGTH_TIMES2) n = FILTERLENGTH; //set back to start of buffer
	
	// return the truncated result
	return (short)(result/FILTERSCALE);
}
