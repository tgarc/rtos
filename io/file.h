#ifndef FILE_H
#define FILE_H

#include <stdlib.h>
#include "util/macros.h"
#include "io/freespace.h"

#define DIRECTORY_START_BLOCK MEM_STARTBLOCK
#define MAXIMUM_FILE_NAME_LENGTH 64
#define BLOCK_SIZE_BYTES 512
#define BLOCK_NEXT_PTR_BYTES 4
#define BLOCK_DATA_BYTES (BLOCK_SIZE_BYTES - BLOCK_NEXT_PTR_BYTES)
#define BLOCK_NEXT_INDEX BLOCK_DATA_BYTES
#define MIN(x,y) (((x)<(y))?(x):(y))
#define INT_AT(ptr) (*((INT*)(ptr)))
#define UINT_AT(ptr) (*((UINT*)(ptr)))

typedef struct {
	UINT skip;
	UINT fileNameLength;
	UINT fileSize;
	UINT startingBlock;
} directory_entry_t;

typedef struct {
	UINT directoryBlockStart; // index of file entry in directory
	UINT directoryStartBlock; // block at which this entry starts in the directory
} directory_location_t;

typedef struct {
	UINT bytesRemaining;
	UINT currentBlockNumber;
	UINT currentBlockPosition;
	directory_location_t directoryLocation;
	directory_entry_t initData;
	BYTE currentBlockData[BLOCK_SIZE_BYTES];
} file_t;

// initialize the directory file. call this 
// first
SF_STATUS
fs_init(void);

SF_STATUS
fs_format(void);

// remove a file from the directory
SF_STATUS
file_rm(file_t* file);

void
file_ls(void);

void 
file_redirect_start(const char* fileName, 
	size_t fileNameSize);

void
file_redirect_end(void);

// create a directory listing
SF_STATUS
file_touch(const char* fileName, 
		   size_t fileNameSize);

// find a particular directory entry
SF_STATUS
file_open(const char* fileName,
		  size_t fileNameSize,
		  file_t* fileData);

int
file_skip(file_t* file,
		  UINT bytesToSkip);

// read in data, populate the buffer
// return how many bytes were actually read
int
file_read(file_t* file, 
		  BYTE* buffer, 
		  size_t bufferSize);

// performs resizing of a file to maximum memory usage
// and free blocks which are not needed by this file
SF_STATUS
file_resize(file_t* file,
		    size_t newSize);

// write wrtieBytes bytes of data from buffer to file
// replaces the current contents of the file
// return how many bytes were actually written
SF_STATUS
file_write(file_t* file, 
		   BYTE* buffer, 
		   size_t writeBytes);

// write wrtieBytes bytes of data from buffer to file
// appends to the current contents of the file
// return how many bytes were actually written
SF_STATUS
file_append(file_t* file, 
		    BYTE* buffer, 
		    size_t writeBytes);

#endif

