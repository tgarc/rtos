//#define LAB5
#ifdef LAB5
//*****************************************************************************
//
// Lab5.c - user programs, File system, stream data onto disk
// Jonathan Valvano, March 16, 2011, EE345M
//     You may implement Lab 5 without the oLED display
//*****************************************************************************
// PF1/IDX1 is user input select switch
// PE1/PWM5 is user input down switch 
#include <stdio.h>
#include <string.h>
#include "inc/hw_types.h"
#include "shell/shell.h"
#include "io/adc.h"
#include "util/os.h"
#include "inc/lm3s8962.h"
#include "io/edisk.h"
#include "io/file.h"
#include "io/freespace.h"
#include "util/macros.h"
#include "util/fifo.h"

#define TIMESLICE 2*TIME_1MS  // thread switch time in system time units
#define ADCFIFO_LENGTH 512

#define GPIO_PF0  (*((volatile unsigned long *)0x40025004))
#define GPIO_PF1  (*((volatile unsigned long *)0x40025008))
#define GPIO_PF2  (*((volatile unsigned long *)0x40025010))
#define GPIO_PF3  (*((volatile unsigned long *)0x40025020))
#define GPIO_PG1  (*((volatile unsigned long *)0x40026008))
// PF1/IDX1 is user input select switch
// PE1/PWM5 is user input down switch 
// PF0/PWM0 is debugging output on Systick
// PF2/LED1 is debugging output 
// PF3/LED0 is debugging output 
// PG1/PWM1 is debugging output 


unsigned long NumCreated;   // number of foreground threads created
unsigned long NumSamples;   // incremented every sample
unsigned long DataLost;     // data sent by Producer, but not received by Consumer
file_t TestFile;
static FIFO 			adcFifo;
static USHORT			adcFifoBlock[ADCFIFO_LENGTH]; // Holds raw 10 bit ADC values
int Running;                // true while robot is running


//******** Robot *************** 
// foreground thread, accepts data from producer
// inputs:  none
// outputs: none
void Robot(void){   
unsigned long data;      // ADC sample, 0 to 1023
unsigned long voltage;   // in mV,      0 to 3000
unsigned long time;      // in 10msec,  0 to 1000 
unsigned long t=0;
unsigned int status;
  OS_ClearTime();    
  DataLost = 0;          // new run with no lost data 
  printf("Robot running...");
	//status  = StartCritical();
  file_redirect_start("Robot",strlen("Robot"));
  printf("time(sec)\tdata(volts)\n\r");
  do{
    t++;
    time=OS_Time();       // 10ms resolution in this OS
		if(!(time%50)) LED_TOGGLE();
    FIFOGet(&adcFifo,&data);        // 1000 Hz sampling get from producer
    data = 10;
		voltage = (300*data)/1024;   // in mV
    printf("%0u.%02u\t%0u.%03u\n\r",time/100,time%100,voltage/1000,voltage%1000);
  }
  while(time < 1000);       // change this to mean 10 seconds
  file_redirect_end();
	//EndCritical(status);
  printf("done.\n\r");
  Running = 0;                // robot no longer running
  OS_Kill();
}
  
//************ButtonPush*************
// Called when Select Button pushed
// background threads execute once and return
void ButtonPush(void){
  if(Running==0){
    Running = 1;  // prevents you from starting two robot threads
    NumCreated += OS_AddThread(&Robot,128,1);  // start a 20 second run
  }
}
//************DownPush*************
// Called when Down Button pushed
// background threads execute once and return
void DownPush(void){

}



//******** Producer *************** 
// The Producer in this lab will be called from your ADC ISR
// A timer runs at 1 kHz, started by your ADC_Collect
// The timer triggers the ADC, creating the 1 kHz sampling
// Your ADC ISR runs when ADC data is ready
// Your ADC ISR calls this function with a 10-bit sample 
// sends data to the Robot, runs periodically at 1 kHz
// inputs:  none
// outputs: none
void Producer(unsigned short data){  
  if(Running){
    if(FIFOPut(&adcFifo,&data) == SUCCESS){     // send to Robot
      NumSamples++;
    } else{ 
      DataLost++;
    } 
  }
}
 
//******** IdleTask  *************** 
// foreground thread, runs when no other work needed
// never blocks, never sleeps, never dies
// inputs:  none
// outputs: none
unsigned long Idlecount=0;
void IdleTask(void){ 
  while(1) { 
    Idlecount++;        // debugging 
  }
}

void FreespaceTest(void) {
	int status,i;
	UINT blocks[FILESYSTEM_BLOCKSIZE];
	
	eDisk_Init(0);
	//printf("FS_BLOCKSIZE: %d\r\n",FILESYSTEM_BLOCKSIZE);
// 	for(i=0;i<FILESYSTEM_BLOCKSIZE;i+=2) {
// 		status = Freespace_AllocateBlock();
// 		if(status == -1) printf("Allocation fail!\r\n");
// 	}
	Freespace_Allocate(blocks,1000);
	printf("Memory after allocating:\r\n");
	Freespace_PrintFSV();
	
	for(i=0;i<FILESYSTEM_BLOCKSIZE/2;i++) {
		status = Freespace_ReleaseBlock(i+MEM_STARTBLOCK);
		if(status == FAILURE) printf("Release fail!\r\n");
	}
	printf("Memory after releasing:\r\n");
	Freespace_PrintFSV();
	
	for(i=0;i<FILESYSTEM_BLOCKSIZE;i+=2) {
		status = Freespace_AllocateBlock();
	}
	Freespace_Format();
	printf("Memory after formatting:\r\n");
	Freespace_PrintFSV();
	
	for(i=0;i<FILESYSTEM_BLOCKSIZE;i+=2) {
		status = Freespace_AllocateBlock();
	}
	status = eDisk_Status(0);
	printf("Disk Status: %d\r\n",status);
	status = Freespace_SaveFSV();
	printf("Save return status: %d\r\n",status);
	Freespace_Format();
	Freespace_LoadFSV();
	printf("Attempt to save and load FSV:\r\n");
	Freespace_PrintFSV();
	
	
	OS_Kill();
}

//******** Interpreter **************
// your intepreter from Lab 4 
// foreground thread, accepts input from serial port, outputs to serial port
// inputs:  none
// outputs: none
extern void Interpreter(void); 
// add the following commands, remove commands that do not make sense anymore
// 1) format 
// 2) directory 
// 3) print file
// 4) delete file
// execute   file_Init();  after periodic interrupts have started

//*******************lab 5 main **********
int main(void){        // lab 5 real main
  OS_Init();           // initialize, disable interrupts
  Running = 0;         // robot not running
  DataLost = 0;        // lost data between producer and consumer
  NumSamples = 0;

//********initialize communication channels
  FIFOInit(&adcFifo,adcFifoBlock,ADCFIFO_LENGTH,sizeof(USHORT));
  ADC_Collect(0, 1000, &Producer); // start ADC sampling, channel 0, 1000 Hz

//*******attach background tasks***********
  OS_AddButtonTask(&ButtonPush,2);
  //OS_AddButtonTask(&DownPush,3);
  OS_AddPeriodicThread(disk_timerproc,10*TIME_1MS,5);

	fs_format();
  NumCreated = 0;
// create initial foreground threads
  NumCreated += OS_AddThread(&Interpreter,128,2); 
	//NumCreated += OS_AddThread(&Robot,128,2);
	//NumCreated += OS_AddThread(&FreespaceTest,128,2); 
  //NumCreated += OS_AddThread(&IdleTask,128,7);  // runs when nothing useful to do
 
  OS_Launch(TIMESLICE); // doesn't return, interrupts enabled in here
  return 0;             // this never executes
}


// //*****************test programs*************************
unsigned char buffer[512];
#define MAXBLOCKS 100
void diskError(char* errtype, unsigned long n){
  printf(errtype);
  printf(" disk error %u",n);
  OS_Kill();
}
void TestDisk(void){  DSTATUS result;  unsigned short block;  int i; unsigned long n;
  // simple test of eDisk
  printf("\n\rEE345M/EE380L, Lab 5 eDisk test\n\r");
  result = eDisk_Init(0);  // initialize disk
  if(result) diskError("eDisk_Init",result);
  printf("Writing blocks\n\r");
  n = 1;    // seed
  for(block = 0; block < MAXBLOCKS; block++){
    for(i=0;i<512;i++){
      n = (16807*n)%2147483647; // pseudo random sequence
      buffer[i] = 0xFF&n;        
    }
    GPIO_PF3 = 0x08;     // PF3 high for 100 block writes
    if(eDisk_WriteBlock(buffer,block))diskError("eDisk_WriteBlock",block); // save to disk
    GPIO_PF3 = 0x00;     
  }  
  printf("Reading blocks\n\r");
  n = 1;  // reseed, start over to get the same sequence
  for(block = 0; block < MAXBLOCKS; block++){
    GPIO_PF2 = 0x04;     // PF2 high for one block read
    if(eDisk_ReadBlock(buffer,block))diskError("eDisk_ReadBlock",block); // read from disk
    GPIO_PF2 = 0x00;
    for(i=0;i<512;i++){
      n = (16807*n)%2147483647; // pseudo random sequence
      if(buffer[i] != (0xFF&n)){
        printf("Read data not correct, block=%u, i=%u, expected %u, read %u\n\r",block,i,(0xFF&n),buffer[i]);
        OS_Kill();
      }      
    }
  }  
  printf("Successful test of %u blocks\n\r",MAXBLOCKS);
  OS_Kill();
}
void RunTest(void){
  NumCreated += OS_AddThread(&TestDisk,128,1);  
}
//******************* test main1 **********
// SYSTICK interrupts, period established by OS_Launch
// Timer interrupts, period established by first call to OS_AddPeriodicThread
int testmain1(void){   // testmain1
  OS_Init();           // initialize, disable interrupts

//*******attach background tasks***********
  OS_AddPeriodicThread(&disk_timerproc,10*TIME_1MS,0);   // time out routines for disk
  OS_AddButtonTask(&RunTest,2);
  
  NumCreated = 0 ;
// create initial foreground threads
  NumCreated += OS_AddThread(&TestDisk,128,1);  
  //NumCreated += OS_AddThread(&IdleTask,128,3); 
 
  OS_Launch(10*TIME_1MS); // doesn't return, interrupts enabled in here
  return 0;               // this never executes
}

// void TestFile(void){   int i; char data; 
//   printf("\n\rEE345M/EE380L, Lab 5 eFile test\n\r");
//   // simple test of eFile
//   if(fs_init())              diskError("file_Init",0); 
//   if(fs_format())            diskError("file_Format",0); 
//   file_ls();
//   if(file_touch("file1",strlen("file1")))     diskError("file_Create",0);
//   if(file_open("file1",strlen("file1"),&TestFile))      diskError("file_open",0);
//   for(i=0;i<1000;i++){
//     if(file_write('a'+i%26))   diskError("file_write",i);
//     if(i%52==51){
//       if(file_write('\n'))     diskError("file_write",i);  
//       if(file_write('\r'))     diskError("file_write",i);
//     }
//   }
//   file_ls();
//   if(file_open("file1",strlen("file1"),&TestFile))      diskError("file_open",0);
//   for(i=0;i<1000;i++){
//     if(file_read(&data) == 0)   diskError("file_ReadNext",i);
//     Serial_OutChar(data);
//   }
//   if(file_rm("file1",strlen("file1")))     diskError("file_rm",0);
//   file_ls();
//   printf("Successful test of creating a file\n\r");
//   OS_Kill();
// }

// //******************* test main2 **********
// // SYSTICK interrupts, period established by OS_Launch
// // Timer interrupts, period established by first call to OS_AddPeriodicThread
// int testmain2(void){ 
//   OS_Init();           // initialize, disable interrupts

// //*******attach background tasks***********
//   OS_AddPeriodicThread(&disk_timerproc,10*TIME_1MS,0);   // time out routines for disk
//   
//   NumCreated = 0 ;
// // create initial foreground threads
//   NumCreated += OS_AddThread(&TestFile,128,1);  
//   NumCreated += OS_AddThread(&IdleTask,128,3); 
//  
//   OS_Launch(10*TIME_1MS); // doesn't return, interrupts enabled in here
//   return 0;               // this never executes
// }
#endif