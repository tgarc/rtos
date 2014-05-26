#ifdef EDISK_TEST

#include <stdio.h>
#include <string.h>
#include "inc/hw_types.h"
#include "inc/lm3s8962.h"
#include "io/rit128x96x4.h"
#include "io/adc.h"
#include "util/os.h"
#include "thread/thread.h"
#include "util/semaphore.h"
#include "io/edisk.h"

BYTE buffer1[512];
BYTE buffer2[512];

void diskInit(void) {
	DSTATUS initStatus = eDisk_Init(0);
	DRESULT writeResult;
	DRESULT readResult;
	int i;
	int errorCount = 0;
	
	for(i=0;i<512;i++) {
		buffer1[i] = 0xDE;
	}
	
	writeResult = eDisk_WriteBlock(buffer1,0);
	readResult = eDisk_ReadBlock(buffer2,0);
	
	for(i=0;i<512;i++) {
		if(buffer2[i] != 0xDE) {
			errorCount++;
		}
	}
	
	OS_Kill();
}

int main(void) {
	OS_Init();
	
	OS_AddThread(&diskInit, 128, 0);
	
	OS_Launch(500);
}

#endif