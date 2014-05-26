#ifndef FIFO_H
#define FIFO_H

#include <stdlib.h>
#include "util/macros.h"
#include "util/semaphore.h"

typedef struct {
	void* fifoDataPtr; // pointer to contiguous region of memory where FIFO is stored
	size_t capacity; // total number of elements allowed in the FIFO
	size_t length; // number of elements currently in the FIFO
	size_t itemSizeBytes; // size of each item in the FIFO
	UINT tailIndex; // where to insert next
	UINT headIndex; // index of oldest item
	semaphore_t fifoFree;
} FIFO;

void FIFOInit(FIFO* result, void* fifoDataPtr, size_t fifoCapacity, size_t itemSizeBytes);
SF_STATUS FIFOPut(FIFO* fifo, void* item);
SF_STATUS FIFOGet(FIFO* fifo, void* destination);
SF_STATUS FIFOPutBlocking(FIFO* fifo, void* item);
SF_STATUS FIFOGetBlocking(FIFO* fifo, void* destination);

#endif
