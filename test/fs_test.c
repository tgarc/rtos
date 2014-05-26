//#define FS_TEST
#ifdef FS_TEST
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
#include "io/freespace.h"
#include "io/file.h"

BYTE buffer[1024];
file_t testFile;
file_t testFile2;

DSTATUS diskInit(void) {
	DSTATUS initStatus = eDisk_Init(0);

	return initStatus;
}

int main(void) {
	int i;
	UINT bytesRead;
	UINT totalBytesRead;
	UINT data;
	OS_Init();
	EnableInterrupts();
	
	printf("Initializing disk...");
	// initialize disk
	if(diskInit()) {
		printf("[FAILURE]\n\r");
		for(;;) {}
	}
	printf("[SUCCESS]\n\r");
	
	printf("Formatting disk...");
	if(fs_format() != SUCCESS) {
		printf("[FAILURE]\n\r");
		for(;;) {}
	}
	printf("[SUCCESS]\n\r");
	
	printf("Initializing file system...");
	if(fs_init() != SUCCESS) {
		printf("[FAILURE]\n\r");
		for(;;) {}
	}
	printf("[SUCCESS]\n\r");
	
	printf("Creating a new file, \"file1\"...");
	if(file_touch("file1", strlen("file1")) != SUCCESS) {
		printf("[FAILURE]\n\r");
		for(;;) {}
	}
	printf("[SUCCESS]\n\r");
	
	printf("Opening the file...");
	if(file_open("file1", strlen("file1"), &testFile) != SUCCESS) {
		printf("[FAILURE]\n\r");
		for(;;) {}
	}
	printf("[SUCCESS]\n\r");
// 	UINT bytesRemaining;
// 	UINT currentBlockNumber;
// 	UINT currentBlockPosition;
// 	directory_location_t directoryLocation;
// 	directory_entry_t initData;
// 	BYTE currentBlockData[BLOCK_SIZE_BYTES];
	printf("Verifying file entry contents...\n\r");
	printf("\tdirectory entry block: %d\n\r", testFile.directoryLocation.directoryStartBlock);
	printf("\tdirectory entry offset: %d\n\r", testFile.directoryLocation.directoryBlockStart);
	printf("\tfile size is %d\n\r", testFile.initData.fileSize);
	printf("\tfile start block is %d\n\r", testFile.initData.startingBlock);
	
	printf("Appending to file...");
	if(file_append(&testFile, "hello, world", strlen("hello, world")) != SUCCESS) {
		printf("[FAILURE]\n\r");
		for(;;) {}
	}
	printf("[SUCCESS]\n\r");
	
	file_open("file1", strlen("file1"), &testFile2);
	printf("Verifying file contents...");
	if(file_read(&testFile2, buffer, sizeof(buffer)) == -1) {
		printf("[FAILURE]\n\r");
		for(;;) {}
	}
	buffer[strlen("hello, world")] = '\0';
	printf("Read string: %s from file\n\r", buffer);
	
	printf("Append more than one block...");
	for(i=0;i<512;i++) {
		// should append 512*4 bytes = 4 blocks + 1 for hello, world text
		if(file_append(&testFile, (BYTE*)(&i), sizeof(i)) != SUCCESS) {
			printf("[FAILURE] %d\n\r", i);
			for(;;) {}
		}
	}
	printf("[SUCCESS]\n\r");
	
	file_open("file1", strlen("file1"), &testFile2);
	printf("Verifying file contents...");
	
	buffer[0] = 0;
	bytesRead = file_read(&testFile2, buffer, strlen("hello, world"));
	totalBytesRead = 0;
	
	if(bytesRead != strlen("hello, world")) {
		printf("[FAILURE]\n\r");
		for(;;) {}
	}
	
	buffer[strlen("hello, world")] = '\0';
	printf("first section read was %s ", buffer);
	
	bytesRead = file_read(&testFile2, buffer, sizeof(buffer)); // read 1024 bytes 2 blocks
	for(i=0;i<256;i++) {
		//printf("uint at %d is %d\n\r", i*4, UINT_AT(&buffer[i*4]));
		
		if(UINT_AT(&buffer[i*4]) != i) {
			printf("[FAILURE] buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
			for(;;) {}
		}
	}
	
	bytesRead = file_read(&testFile2, buffer, sizeof(buffer)); // read 1024 bytes 4 blocks
	totalBytesRead += bytesRead;
	
	for(i=256;i<512;i++) {
		//printf("uint at %d is %d\n\r", i*4, UINT_AT(&buffer[(i-256)*4]));
		
		if(UINT_AT(&buffer[(i-256)*4]) != i) {
			printf("[FAILURE] buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
			for(;;) {}
		}
	}
	
	printf("[SUCCESS]\n\r");
	
	printf("Removing file1...");
	if(file_rm(&testFile2) != SUCCESS) {
		printf("[FAILURE]\n\r");
		for(;;){}
	}
	printf("[SUCCESS]\n\r");
	
	printf("Verifying removal...");
	if(file_open("file1", strlen("file1"), &testFile2) == SUCCESS) {
		printf("[FAILURE]\n\r");
		for(;;){}
	}
	printf("[SUCCESS]\n\r");
	
	file_touch("file1", strlen("file1"));
	file_touch("file2", strlen("file2"));
	file_touch("file3", strlen("file3"));
	
	if(file_open("file1", strlen("file1"), &testFile2) == FAILURE) {
		printf("[FAILURE]\n\r");
		for(;;){}
	}
	
	for(i=0;i<256;i++) {
		UINT_AT(&buffer[i*4]) = i*2;
	}
	
	printf("\n\n\n\n\r");
	file_append(&testFile2, buffer, sizeof(buffer));
	
	for(i=0;i<1024;i++) {
		buffer[i] = 0;
	}
	
	printf("\n\n\n\n\r");
	file_open("file1", strlen("file1"), &testFile);
	
	printf("\n\n\n\n\r");
	bytesRead = file_read(&testFile, buffer, sizeof(buffer));
	if(bytesRead != sizeof(buffer)) {
		printf("[FAILURE] bytes read = %d\n\r", bytesRead);
		for(;;) {}
	}
	
	for(i=0;i<256;i++) {
		//printf("buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
		if(UINT_AT(&buffer[i*4]) != i*2) {
			printf("[FAILURE] buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
			for(;;){}
		}
	}
	
	// file 2
	if(file_open("file2", strlen("file2"), &testFile2) == FAILURE) {
		printf("[FAILURE]\n\r");
		for(;;){}
	}
	
	for(i=0;i<256;i++) {
		UINT_AT(&buffer[i*4]) = i*3;
	}
	
	printf("\n\n\n\n\r");
	file_append(&testFile2, buffer, sizeof(buffer));
	
	for(i=0;i<1024;i++) {
		buffer[i] = 0;
	}
	
	printf("\n\n\n\n\r");
	file_open("file2", strlen("file2"), &testFile);
	
	printf("\n\n\n\n\r");
	bytesRead = file_read(&testFile, buffer, sizeof(buffer));
	if(bytesRead != sizeof(buffer)) {
		printf("[FAILURE] bytes read = %d\n\r", bytesRead);
		for(;;) {}
	}
	
	for(i=0;i<256;i++) {
		//printf("buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
		if(UINT_AT(&buffer[i*4]) != i*3) {
			printf("[FAILURE] buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
			for(;;){}
		}
	}
	
	// file 3
	if(file_open("file3", strlen("file3"), &testFile2) == FAILURE) {
		printf("[FAILURE]\n\r");
		for(;;){}
	}
	
	for(i=0;i<256;i++) {
		UINT_AT(&buffer[i*4]) = i*4;
	}
	
	printf("\n\n\n\n\r");
	file_append(&testFile2, buffer, sizeof(buffer));
	
	for(i=0;i<1024;i++) {
		buffer[i] = 0;
	}
	
	printf("\n\n\n\n\r");
	file_open("file3", strlen("file3"), &testFile);
	
	printf("\n\n\n\n\r");
	bytesRead = file_read(&testFile, buffer, sizeof(buffer));
	if(bytesRead != sizeof(buffer)) {
		printf("[FAILURE] bytes read = %d\n\r", bytesRead);
		for(;;) {}
	}
	
	for(i=0;i<256;i++) {
		//printf("buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
		if(UINT_AT(&buffer[i*4]) != i*4) {
			printf("[FAILURE] buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
			for(;;){}
		}
	}
	
	// make sure nothing overwrote another file
	// file 1
	file_open("file1", strlen("file1"), &testFile);
	
	printf("\n\n\n\n\r");
	bytesRead = file_read(&testFile, buffer, sizeof(buffer));
	if(bytesRead != sizeof(buffer)) {
		printf("[FAILURE] bytes read = %d\n\r", bytesRead);
		for(;;) {}
	}
	
	for(i=0;i<256;i++) {
		//printf("buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
		if(UINT_AT(&buffer[i*4]) != i*2) {
			printf("[FAILURE] buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
			for(;;){}
		}
	}
	// file 2
	file_open("file2", strlen("file2"), &testFile);
	
	printf("\n\n\n\n\r");
	bytesRead = file_read(&testFile, buffer, sizeof(buffer));
	if(bytesRead != sizeof(buffer)) {
		printf("[FAILURE] bytes read = %d\n\r", bytesRead);
		for(;;) {}
	}
	
	for(i=0;i<256;i++) {
		//printf("buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
		if(UINT_AT(&buffer[i*4]) != i*3) {
			printf("[FAILURE] buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
			for(;;){}
		}
	}
	// file 3
	file_open("file3", strlen("file3"), &testFile);
	
	printf("\n\n\n\n\r");
	bytesRead = file_read(&testFile, buffer, sizeof(buffer));
	if(bytesRead != sizeof(buffer)) {
		printf("[FAILURE] bytes read = %d\n\r", bytesRead);
		for(;;) {}
	}
	
	for(i=0;i<256;i++) {
		//printf("buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
		if(UINT_AT(&buffer[i*4]) != i*4) {
			printf("[FAILURE] buffer[%d] = %d\n\r", i*4, UINT_AT(&buffer[i*4]));
			for(;;){}
		}
	}
	
	printf("[SUCCESS]\n\r");
	
	printf("Test overwriting...");
	data = 42;
	file_open("file1", strlen("file1"), &testFile);
	file_write(&testFile, (BYTE*)&data, sizeof(UINT));
	file_open("file1", strlen("file1"), &testFile2);

	bytesRead = file_read(&testFile2, (BYTE*)&data, sizeof(data));
	
	if(bytesRead != sizeof(UINT)) {
		printf("[FAILURE] bytesRead = %d\n\r", bytesRead);
		for(;;) {}
	}
	
	if(data != 42) {
		printf("[FAILURE] data = %d\n\r", data);
		for(;;) {}
	}
	
	if(file_read(&testFile2, (BYTE*)&data, sizeof(data)) != 0) {
		printf("[FAILURE] read didn't terminate\n\r");
		for(;;) {}
	}
	
	printf("[SUCCESS]");
	
	file_ls();
	for(;;) {}
}

#endif