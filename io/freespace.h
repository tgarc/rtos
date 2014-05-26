#ifndef __FREESPACE_H__
#define __FREESPACE_H__

#include "util/macros.h"
#include "io/edisk.h"

#define  BLOCKSIZE								512	 								          // bytes in a disk block
#define  FILESYSTEM_BYTESIZE			1048576							          // 1MB
#define	 FILESYSTEM_BLOCKSIZE			FILESYSTEM_BYTESIZE/BLOCKSIZE // blocks 
#define	 FSV_BYTESIZE					    FILESYSTEM_BLOCKSIZE/8	  		// bytes
#define  FSV_BLOCKSIZE				    1 //(INTCEIL(FSV_BYTESIZE/BLOCKSIZE) ? INTCEIL(FSV_BYTESIZE/BLOCKSIZE): 1) //blocks for FSV (at least 1)
#define  FSV_STARTBLOCK				    0
#define  MEM_STARTBLOCK						FSV_STARTBLOCK+FSV_BLOCKSIZE

#define  UNAVAILABLE						  1
#define  AVAILABLE							  0


UCHAR* Freespace_Vector(void);

////
// Print the FSV to shell
////
void Freespace_PrintFSV(void);

////
// Write the freespace bit vector to disk
////
DRESULT Freespace_SaveFSV(void);

////
// Load in freespace bit vector from disk
////
DRESULT Freespace_LoadFSV(void);

////
// Clear freespace vector
////
void Freespace_Format(void);

////
// Find the first available block
// returns: The absolute block number if a space is available, otherwise returns -1
////
int Freespace_AllocateBlock(void);
SF_STATUS Freespace_AllocateBlockAt(UINT blockNum);

////
// Finds space for multiple blocks at once
// /input: blockCount - number of blocks to allocate
// /input: allocatedBlocks - preallocated array of (at least) size blockCount
// /return: SUCCESS if space found, FAILURE if not enough space found
////
SF_STATUS Freespace_Allocate(UINT allocatedBlocks[],UINT blockCount);

////
// Releases several blocks at once
// /input: allocatedBlocks - an array holding the block numbers to release
// /input: blockCount - number of blocks in allocatedBlocks
// /return: FAILURE if attempt to release a bogus block number, SUCCESS otherwise
////
SF_STATUS Freespace_Release(UINT allocatedBlocks[],UINT blockCount);

////
// Mark a block number as free in the free space manager
////
SF_STATUS Freespace_ReleaseBlock(UINT blockNum);

#endif 
