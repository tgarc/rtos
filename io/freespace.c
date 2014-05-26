
/********************************************************
Free space management
********************************************************/

#include "efile.h"
#include "freespace.h"
#include "util/macros.h"
#include <stdio.h>

// Bit vector holding availability status of each block on disk.
// Note that the block locations in this vector are relative
// to the next block location after the FSV
static UCHAR FreeSpaceVector[FSV_BYTESIZE];

/* #define SETBITn(b,n) ((b) |= (1<<(n))) */
/* #define GETBITn(b,n) (((b) & (1<<(n)))>>n) */
/* #define CLRBITn(b,n) ((b) & ~(1<<(n))) */

#define NUMITERATIONS FSV_BYTESIZE/255

UCHAR* Freespace_Vector(void) {
	return &FreeSpaceVector[0];
}

void Freespace_PrintFSV(void) {
	int i;
	
	printf("======Free Space Bit Vector======");
	for (i=0;i<FSV_BYTESIZE;i++)
	{
		if(i%8 == 0) printf("\r\n");
		printf("%#4X:%4X  ",i,FreeSpaceVector[i]);
	}
	printf("\r\n======End FSV======\r\n");
}

// Read in the disk's free space bit vector
DRESULT Freespace_SaveFSV(void) {
	UINT i;
	DRESULT status;
	
	for (i=0;i<FSV_BLOCKSIZE;i++) {
		status = eDisk_WriteBlock(FreeSpaceVector,FSV_STARTBLOCK+i);
// 		status = eDisk_Write(0,FreeSpaceVector,FSV_STARTBLOCK,255);
		if(status != RES_OK) return status;
	}
	
	return status;
	//return eDisk_Write(0,FreeSpaceVector,FSV_STARTBLOCK,FSV_BYTESIZE%255);
}

// Read in the disk's free space bit vector
DRESULT Freespace_LoadFSV(void) {
	UINT i;
	DRESULT status;
	
	for (i=0;i<FSV_BLOCKSIZE;i++) {
		status = eDisk_ReadBlock(FreeSpaceVector,FSV_STARTBLOCK+i);
		//status = eDisk_Read(0,FreeSpaceVector,FSV_STARTBLOCK,255);
		if(status != RES_OK) return status;
	}
	
	return status;
	//return eDisk_Read(0,FreeSpaceVector,FSV_STARTBLOCK,FSV_BYTESIZE%255);
}

void Freespace_Format(void) {
	UINT i;
	for (i=0;i<FSV_BYTESIZE;i++) {
		FreeSpaceVector[i] = AVAILABLE;
	}
}

////
// Finds space for multiple blocks at once
// /input: blockCount - number of blocks to allocate
// /input: allocatedBlocks - preallocated array of (at least) size blockCount
// /return: SUCCESS if space found, FAILURE if not enough space found
////
SF_STATUS Freespace_Allocate(UINT allocatedBlocks[],UINT blockCount) {
	UINT i,bit,n,j=0;

	ASSERT((blockCount > 0) && (blockCount <= FILESYSTEM_BLOCKSIZE));

	for(i=0; i < FSV_BYTESIZE ;i++) {
		if(FreeSpaceVector[i] == 0xFF) 
			continue;
		
		for(bit=0,n=1;bit < 8;bit++,n<<=1) {		
			// Allocate this block if it is available
			if((FreeSpaceVector[i] & n) == AVAILABLE) {
				FreeSpaceVector[i] |= n; // Mark block as used
				allocatedBlocks[j++] = (i<<3) + bit + MEM_STARTBLOCK;
				if(j == blockCount) return SUCCESS;
			}
		}
	}
	
	return FAILURE;
}



int Freespace_AllocateBlock(void) {
	UINT i,bit,n;

	for(i=0; i < FSV_BYTESIZE ;i++) {
		if(FreeSpaceVector[i] == 0xFF) continue;
		
		for(bit=0,n=1;bit < 8;bit++,n<<=1) {		
			// Allocate this block if it is available
			if((FreeSpaceVector[i] & n) == AVAILABLE) {
				FreeSpaceVector[i] |= n; // Mark block as used
				return (i<<3) + bit + MEM_STARTBLOCK;
			}
		}
	}
	
	return -1;
}


SF_STATUS Freespace_AllocateBlockAt(UINT blockNum) {
	FreeSpaceVector[blockNum/8] |= 1<<(blockNum%8);
	
	return SUCCESS;
}

////
// Releases several blocks at once
// /input: allocatedBlocks - an array holding the block numbers to release
// /input: blockCount - number of blocks in allocatedBlocks
// /return: FAILURE if attempt to release a bogus block number, SUCCESS otherwise
////
SF_STATUS Freespace_Release(UINT allocatedBlocks[],UINT blockCount) {
	//	UINT vectorByteAddr = blockNum / 8;
	//	UCHAR vectorBitOffset = blockNum % 8;
	int blockNum,i=0;
	
	while(i<blockCount) {
		blockNum = allocatedBlocks[i++]-MEM_STARTBLOCK;
		ASSERT(blockNum>=0);
		FreeSpaceVector[blockNum/8] &= ~(1<<(blockNum%8));
	}
	return SUCCESS;
}

SF_STATUS Freespace_ReleaseBlock(UINT blockNum) {
	//	UINT vectorByteAddr = blockNum / 8;
	//	UCHAR vectorBitOffset = blockNum % 8;

	ASSERT(blockNum >= MEM_STARTBLOCK);
	
	blockNum -= MEM_STARTBLOCK;
	
	FreeSpaceVector[blockNum/8] &= ~(1<<(blockNum%8));
	
	return SUCCESS;
}
