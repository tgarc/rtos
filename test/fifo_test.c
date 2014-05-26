//#define FIFO_TEST
#ifdef FIFO_TEST
#include "util/fifo.h"

int main() {
	int testData[] = { 0, 13, 11, 1, 30, 82, 911, 90, 55, 68 };
	int data[10];
	int i = 0;
	int j = 0;
  int readData;
	int putResult = SUCCESS;
	int getResult = SUCCESS;
	int rollResult = SUCCESS;
	int value = 0;
	FIFO fifo;

	fifo = FIFOCreate(data, sizeof(data)/sizeof(int), sizeof(int));

	// test the put function for failure
	for(i=0;i<sizeof(testData)/sizeof(int);i++) {
		if(FIFOPut(&fifo, &testData[i]) == FAILURE) {
			putResult = FAILURE;
			break;
		}
	}
	
	// make sure it doesn't write past the buffer
	if(FIFOPut(&fifo, &i) != FAILURE) {
		putResult = FAILURE;
	}
	
	if(FIFOPut(&fifo, &i) != FAILURE) {
		putResult = FAILURE;
	}
	
	if(FIFOPut(&fifo, &i) != FAILURE) {
		putResult = FAILURE;
	}
	
	// test the get function for failure
	for(i=0;i<sizeof(testData)/sizeof(int);i++) {
		if(FIFOGet(&fifo, &readData) == FAILURE) {
			getResult = FAILURE;
			break;
		}
		
		if(readData != testData[i]) {
			getResult = FAILURE;
			break;
		}
	}
	
	// make sure the head index and tail index can roll over successfully
	for(j=0;j<sizeof(testData)/sizeof(int);j++) {
		for(i=0;i<sizeof(testData)/sizeof(int)/2;i++) { // add half of the items
			if(FIFOPut(&fifo, &testData[i]) == FAILURE) {
				rollResult = FAILURE;
				break;
			}
		}
		
		for(i=0;i<sizeof(testData)/sizeof(int)/2;i++) { // flush fifo
			if(FIFOGet(&fifo, &readData) == FAILURE) {
				rollResult = FAILURE;
				break;
			}
			
			if(readData != testData[i]) {
				rollResult = FAILURE;
				break;
			}
		}
		
		for(i=0;i<sizeof(testData)/sizeof(int);i++) { // max out fifo
			if(FIFOPut(&fifo, &testData[i]) == FAILURE) {
				rollResult = FAILURE;
				break;
			}
		}
		
		// make sure we can't put any more in
		if(FIFOPut(&fifo, &i) != FAILURE) {
			rollResult = FAILURE;
			break;
		}
		
		for(i=0;i<sizeof(testData)/sizeof(int);i++) { // flush fifo
			if(FIFOGet(&fifo, &readData) == FAILURE) {
				rollResult = FAILURE;
				break;
			}
			
			if(readData != testData[i]) {
				rollResult = FAILURE;
				break;
			}
		}
		
		// make sure we can't read from empty FIFO
		if(FIFOGet(&fifo, &readData) != FAILURE) {
				rollResult = FAILURE;
				break;
		}
		
		if(rollResult == FAILURE) { break; }
	}

	value = getResult;
	value = putResult;
	value = rollResult;
	while(1) {}
}
#endif
