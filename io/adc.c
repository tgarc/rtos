#include "adc.h"
#include "inc/lm3s8962.h"
#include "util/macros.h"
#include "util/os.h"
#include "util/dsp.h"

static void 		(*ADC_Task)(USHORT) = NULL;
static void 		(*ADC_Task4)(USHORT,USHORT,USHORT,USHORT) = NULL;

void ADC0_SS3Handler(void){
  ADC0_ISC_R = ADC_ISC_IN3;             // acknowledge ADC sequence 3 completion
  if (ADC_Task != NULL) {
		ADC_Task(ADC0_SSFIFO3_R&ADC_SSFIFO3_DATA_M);
  }
}

void ADC0_SS1Handler(void){
  ADC0_ISC_R = ADC_ISC_IN1;             // acknowledge ADC sequence 1 completion
  if (ADC_Task4 != NULL) {
		ADC_Task4(ADC0_SSFIFO1_R&ADC_SSFIFO1_DATA_M,
							ADC0_SSFIFO1_R&ADC_SSFIFO1_DATA_M,
							ADC0_SSFIFO1_R&ADC_SSFIFO1_DATA_M,
							ADC0_SSFIFO1_R&ADC_SSFIFO1_DATA_M);
  }
}

///
//
// Modified from Dr.Valvano's ADCT0ATrigger example
// /param channelNum - ADC channel number to record from
// /param fs - sample frequency in hertz (100-10000 Hz)
// /param task - Task to call when a sample has been collected.
//							 Callback function must be of the type:
//							 void ADC_Task(unsigned short ADC_Value)
// /return - void
//
///
SF_STATUS ADC_Collect(UCHAR channelNum, UINT fs, void (*task)(USHORT)){
	UINT status;
  
	// channelNum must be 0-3 (inclusive) corresponding to ADC0 through ADC3
  ASSERT(channelNum <= 3);
	ASSERT(200 <= fs && fs <= 500000);
	
  status = StartCritical();
  // **** general initialization ****
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0;    // activate timer0
	SYSCTL_RCGC0_R |= SYSCTL_RCGC0_ADC;       // activate ADC
  SYSCTL_RCGC0_R &= ~SYSCTL_RCGC0_ADCSPD_M; // clear ADC sample speed field
  SYSCTL_RCGC0_R += SYSCTL_RCGC0_ADCSPD125K;// configure for 125K ADC max sample rate (default setting)
	
	ADC_Task = task;
					
	TIMER0_CTL_R &= ~TIMER_CTL_TAEN;          // disable timer0A during setup
	TIMER0_CTL_R |= TIMER_CTL_TAOTE;          // enable timer0A trigger to ADC
	TIMER0_CFG_R = TIMER_CFG_16_BIT;          // configure for 16-bit timer mode
	// **** timer0A initialization ****
	TIMER0_TAMR_R = TIMER_TAMR_TAMR_PERIOD;   // configure for periodic mode
	TIMER0_TAPR_R = 49;                 			// prescale value for trigger (1us)
	TIMER0_TAILR_R = (1000000/fs)-1;           // reload value in prescale units
	TIMER0_IMR_R &= ~TIMER_IMR_TATOIM;        // disable timeout (rollover) interrupt
	TIMER0_CTL_R |= TIMER_CTL_TAEN;           // enable timer0A 16-b, periodic, no interrupts

  // **** ADC initialization ****
                                            // sequencer 0 is highest priority (default setting)
                                            // sequencer 1 is second-highest priority (default setting)
                                            // sequencer 2 is third-highest priority (default setting)
                                            // sequencer 3 is lowest priority (default setting)
  ADC0_SSPRI_R = (ADC_SSPRI_SS0_1ST|ADC_SSPRI_SS1_2ND|ADC_SSPRI_SS2_3RD|ADC_SSPRI_SS3_4TH);
  ADC_ACTSS_R &= ~ADC_ACTSS_ASEN3;          // disable sample sequencer 3
  ADC0_EMUX_R &= ~ADC_EMUX_EM3_M;           // clear SS3 trigger select field
  ADC0_EMUX_R += ADC_EMUX_EM3_TIMER; 				// configure for timer event
  ADC0_SSMUX3_R &= ~ADC_SSMUX3_MUX0_M;      // clear SS3 1st sample input select field
                                            // configure for 'channelNum' as first sample input
  ADC0_SSMUX3_R += (channelNum<<ADC_SSMUX3_MUX0_S);
  ADC0_SSCTL3_R = (0                        // settings for 1st sample:
                   & ~ADC_SSCTL3_TS0        // read pin specified by ADC0_SSMUX3_R (default setting)
                   | ADC_SSCTL3_IE0         // raw interrupt asserted here
                   | ADC_SSCTL3_END0        // sample is end of sequence (default setting, hardwired)
                   & ~ADC_SSCTL3_D0);       // differential mode not used (default setting)
  ADC0_IM_R |= ADC_IM_MASK3;                // enable SS3 interrupts
  ADC_ACTSS_R |= ADC_ACTSS_ASEN3;           // enable sample sequencer 3
	
	//ADC0_TMLB_R = 1; // For TESTING ONLY! (Produces simulated values)
	
  // **** interrupt initialization ****
                                            
  NVIC_PRI4_R = (NVIC_PRI4_R&0xFFFF00FF)|0x00000000; // bits 13-15
  NVIC_EN0_R |= NVIC_EN0_INT17;             // enable interrupt 17 in NVIC
  
	EndCritical(status);
	
	return (SUCCESS);
}

// Same as Collect except collects all 4 channels simultaneously
SF_STATUS ADC_Collect4(UINT fs, void (*task)(USHORT,USHORT,USHORT,USHORT)){
	UINT status;

	ASSERT(200 <= fs && fs <= 500000);
	
  status = StartCritical();
  // **** general initialization ****
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0;    // activate timer0
	SYSCTL_RCGC0_R |= SYSCTL_RCGC0_ADC;       // activate ADC
  SYSCTL_RCGC0_R &= ~SYSCTL_RCGC0_ADCSPD_M; // clear ADC sample speed field
  SYSCTL_RCGC0_R += SYSCTL_RCGC0_ADCSPD125K;// configure for 125K ADC max sample rate (default setting)
	
	ADC_Task4 = task;
					
	TIMER0_CTL_R &= ~TIMER_CTL_TAEN;          // disable timer0A during setup
	TIMER0_CTL_R |= TIMER_CTL_TAOTE;          // enable timer0A trigger to ADC
	TIMER0_CFG_R = TIMER_CFG_16_BIT;          // configure for 16-bit timer mode
	// **** timer0A initialization ****
	TIMER0_TAMR_R = TIMER_TAMR_TAMR_PERIOD;   // configure for periodic mode
	TIMER0_TAPR_R = 49;                 			// prescale value for trigger (1us)
	TIMER0_TAILR_R = (1000000/fs)-1;           // reload value in prescale units
	TIMER0_IMR_R &= ~TIMER_IMR_TATOIM;        // disable timeout (rollover) interrupt
	TIMER0_CTL_R |= TIMER_CTL_TAEN;           // enable timer0A 16-b, periodic, no interrupts

  // **** ADC initialization ****
                                            // sequencer 0 is highest priority (default setting)
                                            // sequencer 1 is second-highest priority (default setting)
                                            // sequencer 2 is third-highest priority (default setting)
                                            // sequencer 3 is lowest priority (default setting)
  ADC0_SSPRI_R = (ADC_SSPRI_SS0_1ST|ADC_SSPRI_SS1_2ND|ADC_SSPRI_SS2_3RD|ADC_SSPRI_SS3_4TH);
  ADC_ACTSS_R &= ~ADC_ACTSS_ASEN1;          // disable sample sequencer 3
  ADC0_EMUX_R &= ~ADC_EMUX_EM1_M;           // clear SS3 trigger select field
  ADC0_EMUX_R += ADC_EMUX_EM1_TIMER; 				// configure for timer event
  ADC0_SSMUX1_R &= ~ADC_SSMUX1_MUX0_M;      // clear SS3 1st sample input select field
                                            // configure for 'channelNum' as first sample input
	ADC0_SAC_R = ADC_SAC_AVG_2X; // 2 times averaging
  //ADC0_SSMUX1_R += (channelNum<<ADC_SSMUX1_MUX0_S);
	ADC_SSMUX1_R = ((0 << ADC_SSMUX1_MUX0_S) |
									(1 << ADC_SSMUX1_MUX1_S) |
									(2 << ADC_SSMUX1_MUX2_S) |
									(3 << ADC_SSMUX1_MUX3_S));
  ADC0_SSCTL1_R = (0                        // settings for 1st sample:
                   & ~ADC_SSCTL1_TS0        // read pin specified by ADC0_SSMUX3_R (default setting)
                   | ADC_SSCTL1_IE3         // raw interrupt asserted here
                   | ADC_SSCTL1_END3        // sample is end of sequence (default setting, hardwired)
                   & ~ADC_SSCTL1_D0);       // differential mode not used (default setting)
  ADC0_IM_R |= ADC_IM_MASK1;                // enable SS1 interrupts
  ADC_ACTSS_R |= ADC_ACTSS_ASEN1;           // enable sample sequencer 1

	
	//ADC0_TMLB_R = 1; // For TESTING ONLY! (Produces simulated values)
	
  // **** interrupt initialization ****
                                            
  NVIC_PRI4_R = (NVIC_PRI4_R&0xFFFF00FF)|0x00000000; // bits 13-15
  NVIC_EN0_R |= NVIC_EN0_INT17;             // enable interrupt 17 in NVIC
  
	EndCritical(status);
	
	return (SUCCESS);
}

void ADC_StopCollect(void) {
	ADC0_IM_R &= ~ADC_IM_MASK3;  //disable SS3 interrupts
	TIMER0_CTL_R &= ~TIMER_CTL_TAEN;          // disable timer0A
	ADC_ACTSS_R &= ~ADC_ACTSS_ASEN3;          // disable sample sequencer 3
}

///
// Configure sample sequencer 2 as a processor(i.e. software)
// initiated sample sequence
///
void
ADC_Open(void)
{
	SYSCTL_RCGC0_R |= SYSCTL_RCGC0_ADC;	    // Enable the ADC Clock
  SYSCTL_RCGC0_R &= ~SYSCTL_RCGC0_ADCSPD_M; // clear ADC sample speed field
  SYSCTL_RCGC0_R += SYSCTL_RCGC0_ADCSPD125K;// configure for 125K ADC max sample rate (default setting)
	ADC_ACTSS_R &= ~ADC_ACTSS_ASEN2;          // disable sample sequencer 2
                                            // sequencer 0 is highest priority (default setting)
                                            // sequencer 1 is second-highest priority (default setting)
                                            // sequencer 2 is third-highest priority (default setting)
                                            // sequencer 3 is lowest priority (default setting)
  ADC0_SSPRI_R = (ADC_SSPRI_SS0_1ST|ADC_SSPRI_SS1_2ND|ADC_SSPRI_SS2_3RD|ADC_SSPRI_SS3_4TH);
  ADC0_EMUX_R &= ~ADC_EMUX_EM2_M;           // clear SS3 trigger select field
  ADC0_EMUX_R += ADC_EMUX_EM2_PROCESSOR;    // configure for software trigger event (default setting)
  ADC0_SSCTL2_R = (0                        // Settings For 1st Sample:
                   & ~ADC_SSCTL2_TS0        // read pin specified by ADC0_SSMUX2_R (default setting)
                   | ADC_SSCTL2_IE0         // raw interrupt asserted here
                   | ADC_SSCTL2_END0        // sample is end of sequence (default setting, hardwired)
                   & ~ADC_SSCTL2_D0);       // differential mode not used (default setting)
  ADC0_IM_R &= ~ADC_IM_MASK2;               // disable SS2 interrupts (default setting)
  ADC_ACTSS_R |= ADC_ACTSS_ASEN2;           // enable sample sequencer 2
}

///
// Collect a sample from specified ADC channel
// \param channelNum Channel number of ADC to use
// \return The 10 bit result from SSFIFO3
///
unsigned short
ADC_In(unsigned int channelNum)
{	
  unsigned short usResult;
	//long status = StartCritical();
	
  ADC_ACTSS_R &= ~ADC_ACTSS_ASEN2;          // disable sample sequencer 3
  ADC0_SSMUX2_R &= ~ADC_SSMUX2_MUX0_M;      // clear SS3 1st sample input select field
                                            // configure for 'channelNum' as first sample input
  ADC0_SSMUX2_R += (channelNum<<ADC_SSMUX2_MUX0_S); 
  ADC_ACTSS_R |= ADC_ACTSS_ASEN2;           // enable sample sequencer 3
	
  ADC0_PSSI_R = ADC_PSSI_SS2;               // initiate SS3
  while(!(ADC0_RIS_R&ADC_RIS_INR2)){};    // poll raw interrupt to check for conversion completion
  // Get the last 10 bits for result
  usResult = ADC0_SSFIFO2_R&ADC_SSFIFO2_DATA_M; 
  ADC0_ISC_R |= ADC_ISC_IN2;                 // acknowledge completion of current conversion
  
	//EndCritical(status);
  return usResult;
}
