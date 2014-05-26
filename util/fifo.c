#include <string.h>
#include "fifo.h"
#include "util\os.h"
#include "util\semaphore.h"

/*
 * Sets up and returns a FIFO struct, containing all data about the state of of
 * a FIFO data structure.
 *
 * fifoDataPtr - a pointer to the block of memory which is to contain the fifo
 *    most likely an array
 * fifoCapacity - the number of elements that can be contained in the block
 *    pointed to by fifoDataPtr of the given length
 * itemSizeBytes - the size of each item in the fifo in bytes ( result of sizeof )
 *
 * returns - a FIFO struct ready to be manipulated by the other FIFO functions
 */
void FIFOInit(FIFO* result, void* fifoDataPtr, size_t fifoCapacity, size_t itemSizeBytes) {
	OS_InitSemaphore(&(result->fifoFree),UNLOCKED); // Initialize the binary semaphore

	result->fifoDataPtr = fifoDataPtr;
	result->capacity = fifoCapacity;
	result->length = 0;
	result->itemSizeBytes = itemSizeBytes;
	result->tailIndex = 0;
	result->headIndex = 0;
}

/*
 * Adds an item to the FIFO
 * fifo - fifo structure that was initialized and created by FIFOCreate
 * item - a pointer to the item that is to be added to the FIFO
 * 
 * returns - SUCCESS if putting is successful
 *           FAILURE if putting is not successful (i.e. the structure is full)
 */
SF_STATUS FIFOPut(FIFO* fifo, void* item) {
	int status = 0;
	ASSERT(fifo != NULL && fifo->fifoDataPtr != NULL);
	ASSERT(fifo->length < fifo->capacity);
	ASSERT(fifo->fifoFree.count > 0); // Make sure FIFO is free before continuing

	OS_Wait(&(fifo->fifoFree));
	//status = StartCritical();

	// copy in the item
	memcpy(((char*)fifo->fifoDataPtr) + fifo->tailIndex*fifo->itemSizeBytes, item, 
		fifo->itemSizeBytes);
	
	// move the tail index up. wrap around if necessary
	fifo->tailIndex += 1;
	fifo->tailIndex %= fifo->capacity;
	
	fifo->length ++;

	OS_Signal(&(fifo->fifoFree));
	//EndCritical(status);
	return (SUCCESS);
}

/*
 * Gets oldest item from the FIFO
 * fifo - fifo structure that was initialized and created by FIFOCreate
 * destination - a pointer to the place where the item is to be copied
 * 
 * returns - SUCCESS if getting is successful
 *           FAILURE if getting is not successful (i.e. the structure is empty)
 */
SF_STATUS FIFOGet(FIFO* fifo, void* destination) {
	int status = 0;
	ASSERT(fifo != NULL && fifo->fifoDataPtr != NULL);
	ASSERT(fifo->length > 0);
	ASSERT(fifo->fifoFree.count > 0); // Make sure FIFO is free before continuing

	OS_Wait(&(fifo->fifoFree)); // Lock the mutex

	// copy the item
	memcpy(destination, ((char*)fifo->fifoDataPtr) + fifo->headIndex*fifo->itemSizeBytes, 
		fifo->itemSizeBytes);
	
	// move the head index up. wrap around if necesssary
	fifo->headIndex += 1;
	fifo->headIndex %= fifo->capacity;
	
	fifo->length --;
	
	OS_Signal(&(fifo->fifoFree));
	
	return (SUCCESS);
}

SF_STATUS FIFOGetBlocking(FIFO* fifo, void* destination) {
	int status = 0;
	ASSERT(fifo != NULL && fifo->fifoDataPtr != NULL);
	ASSERT(fifo->length > 0);
	
	OS_Wait(&(fifo->fifoFree));
	
		
	// copy the item
	memcpy(destination, ((char*)fifo->fifoDataPtr) + fifo->headIndex*fifo->itemSizeBytes, 
		fifo->itemSizeBytes);
	
	// move the head index up. wrap around if necesssary
	fifo->headIndex += 1;
	fifo->headIndex %= fifo->capacity;
	
	fifo->length --;
	
	OS_Signal(&(fifo->fifoFree));
	
	return (SUCCESS);
}

SF_STATUS FIFOPutBlocking(FIFO* fifo, void* item) {
	int status = 0;
	ASSERT(fifo != NULL && fifo->fifoDataPtr != NULL);
	ASSERT(fifo->length < fifo->capacity);

	OS_Wait(&(fifo->fifoFree));

	// copy in the item
	memcpy(((char*)fifo->fifoDataPtr) + fifo->tailIndex*fifo->itemSizeBytes, item, 
		fifo->itemSizeBytes);
	
	// move the tail index up. wrap around if necessary
	fifo->tailIndex += 1;
	fifo->tailIndex %= fifo->capacity;
	
	fifo->length ++;

	OS_Signal(&(fifo->fifoFree));
	
	return (SUCCESS);
}