#include "io/file.h"
#include "io/freespace.h"
#include "io/uart_stdio.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "util/semaphore.h"

// disk space structure
// [AVAILABLE BLOCK VECTOR][0x00000000 directory_size_bytes(4) thisBlockNumber][...]
// the first section of the directory is itself a directory entry for the directory
// file
static semaphore_t DiskMutex;
UINT NumReaders = 0;


file_t directoryFile;
BOOL directoryFileInitialized = FALSE;

static void
fileOpen(file_t* file,
		 directory_entry_t* dirEntry,
		 directory_location_t* dirLocation);

static void
fileReset(file_t* file);

static SF_STATUS
fileAppend(file_t* file, 
		   BYTE* buffer, 
		   size_t writeBytes, 
		   UINT startBlock);

static SF_STATUS
fileWritePartial(file_t* file, 
		         BYTE* buffer, 
		         size_t writeBytes, 
		         UINT startBlock);

static SF_STATUS
dirLookup(const char* fileName,
		  size_t fileNameSize,
		  directory_entry_t* dirEntry,
		  directory_location_t* dirLocation);

static SF_STATUS
dirEntryAt(directory_location_t* dirLocation,
		   directory_entry_t* dirEntry);
			 
static SF_STATUS
dirUpdate(file_t* file);

static file_t redirectFile;

static void printToRedirectFile(char* c,size_t bufferSize) {
	file_append(&redirectFile, (BYTE*)c, bufferSize);
}

void 
file_redirect_start(const char* fileName, 
	size_t fileNameSize) {
		if(file_open(fileName, fileNameSize, &redirectFile) != SUCCESS) {
			file_touch(fileName, fileNameSize);
			file_open(fileName, fileNameSize, &redirectFile);
		}
		
		UARTIOOutputRedirect(printToRedirectFile);
}

void
file_redirect_end(void) {
	UARTIOOutputRedirect(NULL);
}

SF_STATUS
fs_init(void) { 
	directory_entry_t dirFileEntry;
	directory_location_t dirFileLocation;

	dirFileLocation.directoryBlockStart = 0;
	dirFileLocation.directoryStartBlock = DIRECTORY_START_BLOCK;
		
	// get data about directory size
	//ASSERT(dirEntryAt(&dirFileLocation, &dirFileEntry) == SUCCESS);
	
	eDisk_ReadBlock(directoryFile.currentBlockData, 1);
	
	memcpy(&dirFileEntry, directoryFile.currentBlockData, sizeof(directory_entry_t)); 
	
	fileOpen(&directoryFile, &dirFileEntry, &dirFileLocation);

	OS_InitSemaphore(&DiskMutex,UNLOCKED);
	
	return (SUCCESS);
}

SF_STATUS
fs_format(void) {
	directory_entry_t dirSelfEntry;
	
	OS_Wait(&DiskMutex);
	
	Freespace_Format();
	Freespace_AllocateBlock();
	AssertAndRelease(Freespace_SaveFSV() == RES_OK,&DiskMutex);
	
	dirSelfEntry.skip = 1;
	dirSelfEntry.fileNameLength = 0;
	dirSelfEntry.fileSize = sizeof(directory_entry_t);
	dirSelfEntry.startingBlock = DIRECTORY_START_BLOCK;
	
	memcpy(directoryFile.currentBlockData, &dirSelfEntry, sizeof(dirSelfEntry));
	
	AssertAndRelease(eDisk_WriteBlock(directoryFile.currentBlockData, DIRECTORY_START_BLOCK) == RES_OK,&DiskMutex);
	
	fileReset(&directoryFile);
	
	OS_Signal(&DiskMutex);
	
	return (SUCCESS);
}

// remove a file
SF_STATUS
file_rm(file_t* file) {
	UINT occupiedBlocks;
	
	//ASSERT(DriveFree.count > 0);
	OS_Wait(&DiskMutex);
	//UINT status = StartCritical();
	
	fileReset(file);
	fileReset(&directoryFile);
	file_skip(file, 0); // initialize
	
	if(file->initData.fileSize == 0) {
		occupiedBlocks = 1;
	}
	else if(file->initData.fileSize%BLOCK_DATA_BYTES == 0) {
		occupiedBlocks = file->initData.fileSize/BLOCK_DATA_BYTES;
	}
	else {
		occupiedBlocks = file->initData.fileSize/BLOCK_DATA_BYTES + 1;
	}
	
	while(occupiedBlocks > 0) {
		Freespace_ReleaseBlock(file->currentBlockNumber);
		
		file_skip(file, BLOCK_DATA_BYTES);
		
		occupiedBlocks --;
	}
	
	// remove this directory entry
	eDisk_ReadBlock(file->currentBlockData, file->directoryLocation.directoryStartBlock);
	file->currentBlockData[file->directoryLocation.directoryBlockStart] = 0xFF;
	eDisk_WriteBlock(file->currentBlockData, file->directoryLocation.directoryStartBlock);

	OS_Signal(&DiskMutex);
	//EndCritical();
	return (SUCCESS);
}

void dumpDirectoryBlock1(void) {
	int i;
	eDisk_ReadBlock(directoryFile.currentBlockData, 1);
	
	for(i=0;i<512/8;i++) {
		printf("%02x %02x %02x %02x %02x %02x %02x %02x\n\r",
			directoryFile.currentBlockData[8*i],
		  directoryFile.currentBlockData[8*i+1],
			directoryFile.currentBlockData[8*i+2],
		 directoryFile.currentBlockData[8*i+3],
		 directoryFile.currentBlockData[8*i+4],
		 directoryFile.currentBlockData[8*i+5],
		 directoryFile.currentBlockData[8*i+6],
		 directoryFile.currentBlockData[8*i+7]
		);
	}
}

void
file_ls(void) {
	static char fileNameBuffer[MAXIMUM_FILE_NAME_LENGTH];
	directory_entry_t nextDirectoryEntry;

	//ASSERT(DriveFree.count > 0);
	//OS_Wait(&DriveFree);
	
	// skip the first part which is the metadata for the directory
	// itselfF
	fileReset(&directoryFile);
	file_skip(&directoryFile, sizeof(directory_entry_t));
				
	while(file_read(&directoryFile, (BYTE*)(&nextDirectoryEntry), sizeof(nextDirectoryEntry))) {
		
		// read in file name and compare
		file_read(&directoryFile, (BYTE*)(&fileNameBuffer), nextDirectoryEntry.fileNameLength);

		fileNameBuffer[nextDirectoryEntry.fileNameLength] = '\0';
		
		if(nextDirectoryEntry.skip) continue;
		printf("%s Length: %d\n\r", fileNameBuffer, nextDirectoryEntry.fileSize);
	}
	//OS_Signal(&DriveFree);
}

// create an empty file
SF_STATUS
file_touch(const char* fileName, 
		   size_t fileNameSize) {
	directory_entry_t newDirEntry;
	int nextBlockNumber;
	int i;
				
	nextBlockNumber = Freespace_AllocateBlock();

	// make sure space could be allocated
	ASSERT(nextBlockNumber != -1);

	// build new directory entry
	newDirEntry.fileNameLength = fileNameSize;
	newDirEntry.fileSize = 0;
	newDirEntry.startingBlock = nextBlockNumber;
	newDirEntry.skip = 0;
				 
	//fileReset(&directoryFile);

	// append the new directory entry to the directory file
	ASSERT(file_append(&directoryFile, (BYTE*)&newDirEntry, sizeof(newDirEntry)) == SUCCESS);
	// append the file name to the directory file
	ASSERT(file_append(&directoryFile, (BYTE*)fileName, fileNameSize) == SUCCESS);
	
	fileReset(&directoryFile);
	file_skip(&directoryFile, 0);
	
	return (SUCCESS);
}

SF_STATUS
file_open(const char* fileName,
		  size_t fileNameSize,
		  file_t* fileData) {
	directory_entry_t dirEntry;
	directory_location_t dirLocation;


	ASSERT(dirLookup(fileName, fileNameSize, &dirEntry, &dirLocation) == SUCCESS);

	fileOpen(fileData, &dirEntry, &dirLocation);
	
	return (SUCCESS);
}

int
file_skip(file_t* file,
		  UINT bytesToSkip) {
	UINT currentBlockNumber = file->currentBlockNumber;
	UINT currentBlockPosition = file->currentBlockPosition;
	UINT fileBytesRemaining = file->bytesRemaining;
	UINT bytesRead = 0;
				
	// make sure we haven't reached the end of the file
	if(fileBytesRemaining == 0) {
		file->currentBlockNumber = file->initData.startingBlock;
		return 0;
	}

	// if the current block is set to 0 this indicates
	// that the currentBlock buffer has not yet been
	// initialized
	if(currentBlockNumber == 0) {
		currentBlockNumber = file->initData.startingBlock;
		
		// read in current block into file structure
		if(eDisk_ReadBlock(file->currentBlockData, currentBlockNumber) != RES_OK) {
			return -1;
		}
	}

	while(fileBytesRemaining > 0 && bytesToSkip > 0) {
		// check if there are unread bytes remain in the current block
		if(currentBlockPosition < BLOCK_DATA_BYTES) {
			// assume we are reading the rest of the current block
			UINT currentBlockBytesToRead = 
				MIN((BLOCK_DATA_BYTES - currentBlockPosition),
					(fileBytesRemaining));

			currentBlockBytesToRead = MIN(currentBlockBytesToRead, bytesToSkip);
			
			// adjust counter values
			fileBytesRemaining -= currentBlockBytesToRead;
			bytesToSkip -= currentBlockBytesToRead;
			currentBlockPosition += currentBlockBytesToRead;
		}
		else { // time to read the next block in
			UINT nextBlockNumber = (UINT)(file->currentBlockData[BLOCK_NEXT_INDEX]);
			currentBlockNumber = nextBlockNumber;
			currentBlockPosition = 0;
			
			if(eDisk_ReadBlock(file->currentBlockData, currentBlockNumber) != RES_OK) {
				return -1;
			}
		}
	}
	
	file->currentBlockNumber = currentBlockNumber;
	file->currentBlockPosition = currentBlockPosition;
	file->bytesRemaining = fileBytesRemaining;

	return bytesRead;
}

// read in data starting at current file
// read location, populate the buffer
// return how many bytes were read
int
file_read(file_t* file, 
		  BYTE* buffer, 
		  size_t bufferSize) {
	UINT currentBlockNumber = file->currentBlockNumber;
	UINT currentBlockPosition = file->currentBlockPosition;
	UINT fileBytesRemaining = file->bytesRemaining;
	UINT bytesRead = 0;

	//ASSERT(DriveFree.count > 0);
	//OS_Wait(&ReadersMutex);
				
	// make sure we haven't reached the end of the file
	if(fileBytesRemaining == 0) {
		return 0;
	}

	// if the current block is set to 0 this indicates
	// that the currentBlock buffer has not yet been
	// initialized
	if(currentBlockNumber == 0) {
		currentBlockNumber = file->initData.startingBlock;

		// read in current block into file structure
		if(eDisk_ReadBlock(file->currentBlockData, currentBlockNumber) != RES_OK) {
			return -1;
		}
	}
	
	while(bytesRead < bufferSize && fileBytesRemaining > 0) {
		// check if there are unread bytes remain in the current block
		if(currentBlockPosition < BLOCK_DATA_BYTES) {
			// assume we are reading the rest of the current block
			UINT currentBlockBytesToRead = 
				MIN(BLOCK_DATA_BYTES - currentBlockPosition, // data bytes left in block
					bufferSize - bytesRead); // bytes left in buffer
			
			// is the number of bytes left in the file less than the number of
			// bytes left in the current block and buffer?
			if(fileBytesRemaining < currentBlockBytesToRead) {
				currentBlockBytesToRead = fileBytesRemaining;
			}

			// copy the bytes
			memcpy(&buffer[bytesRead], &file->currentBlockData[currentBlockPosition], currentBlockBytesToRead);
			
			// adjust counter values
			fileBytesRemaining -= currentBlockBytesToRead;
			currentBlockPosition += currentBlockBytesToRead;
			bytesRead += currentBlockBytesToRead;
		}
		else { // time to read the next block in
			UINT nextBlockNumber = (UINT)(file->currentBlockData[BLOCK_NEXT_INDEX]);
			currentBlockNumber = nextBlockNumber;
			currentBlockPosition = 0;
			
			// read in next block of file
			if(eDisk_ReadBlock(file->currentBlockData, currentBlockNumber) != RES_OK) {
				return -1;
			}
		}
	}
	
	file->currentBlockNumber = currentBlockNumber;
	file->currentBlockPosition = currentBlockPosition;
	file->bytesRemaining = fileBytesRemaining;

	return bytesRead;
}

// performs resizing of a file to maximum memory usage
SF_STATUS
file_resize(file_t* file,
		    size_t newSize) {
		return (SUCCESS);
}

// write wrtieBytes bytes of data from buffer to file
// replaces the current contents of the file
// return how many bytes were actually written
SF_STATUS
file_write(file_t* file, 
		   BYTE* buffer, 
		   size_t writeBytes) {
	UINT occupiedBlocks;
	SF_STATUS status;
	UINT bytesLeftInBlock;
				 
	//ASSERT(DriveFree.count > 0);
	OS_Wait(&DiskMutex);
				 
	fileReset(file);
	fileReset(&directoryFile);
	file_skip(file, 0); // initialize
	
	if(file->initData.fileSize == 0) {
		occupiedBlocks = 1;
	}
	else if(file->initData.fileSize%BLOCK_DATA_BYTES == 0) {
		occupiedBlocks = file->initData.fileSize/BLOCK_DATA_BYTES;
	}
	else {
		occupiedBlocks = file->initData.fileSize/BLOCK_DATA_BYTES + 1;
	}
	
	while(occupiedBlocks > 0) {
		Freespace_ReleaseBlock(file->currentBlockNumber);
		
		file_skip(file, BLOCK_DATA_BYTES);
		
		occupiedBlocks --;
	}
	
	file->initData.startingBlock = Freespace_AllocateBlock();
	
	AssertAndRelease(file->initData.startingBlock != -1,&DiskMutex);
	
	file->initData.fileSize = 0;
	
	dirUpdate(file);

	//file_append(file, buffer, writeBytes);
	if(file->initData.fileSize == 0) {
		bytesLeftInBlock = BLOCK_DATA_BYTES;
	}
	else if(file->initData.fileSize%BLOCK_DATA_BYTES == 0) {
		bytesLeftInBlock = 0;
	}
	else {
		bytesLeftInBlock = BLOCK_DATA_BYTES-(file->initData.fileSize%BLOCK_DATA_BYTES);
	}
					
	fileReset(file);
					
	// go to end
	file_skip(file, file->initData.fileSize);

	if(bytesLeftInBlock == 0) {
		INT newBlock = Freespace_AllocateBlock();
		
		memcpy(&file->currentBlockData[BLOCK_NEXT_INDEX], (BYTE*)(&newBlock), sizeof(newBlock));
		AssertAndRelease(eDisk_WriteBlock(file->currentBlockData, file->currentBlockNumber) == RES_OK,&DiskMutex);
		AssertAndRelease(eDisk_ReadBlock(file->currentBlockData, newBlock) == RES_OK,&DiskMutex);
		
		file->currentBlockNumber = newBlock;
		file->currentBlockPosition = 0;
		
		bytesLeftInBlock = BLOCK_DATA_BYTES;
	}
					
	// write remainder of the current block data to be block
	// aligned
	if(bytesLeftInBlock < writeBytes) {
		INT newBlock = Freespace_AllocateBlock();
		memcpy(&file->currentBlockData[file->currentBlockPosition], buffer, bytesLeftInBlock);

		AssertAndRelease(newBlock != -1,&DiskMutex);

		UINT_AT(&file->currentBlockData[BLOCK_NEXT_INDEX]) = newBlock;
		
		AssertAndRelease(eDisk_WriteBlock(file->currentBlockData, file->currentBlockNumber) == RES_OK,&DiskMutex);
		file->initData.fileSize += bytesLeftInBlock;
		
		buffer = &buffer[bytesLeftInBlock];
		status = fileAppend(file, buffer, writeBytes-bytesLeftInBlock, newBlock);
		OS_Signal(&DiskMutex);
		return status;
	}
	else {
		memcpy(&file->currentBlockData[file->currentBlockPosition], buffer, writeBytes);
		AssertAndRelease(eDisk_WriteBlock(file->currentBlockData, file->currentBlockNumber) == RES_OK,&DiskMutex);
	}

	file->initData.fileSize += writeBytes;
	AssertAndRelease(dirUpdate(file) == SUCCESS,&DiskMutex);
	// save the free space vector
	AssertAndRelease(Freespace_SaveFSV() == RES_OK,&DiskMutex);
	
	fileReset(file);
	
	OS_Signal(&DiskMutex);
	return (SUCCESS);
}

// write wrtieBytes bytes of data from buffer to file
// appends to the current contents of the file
// return how many bytes were actually written
SF_STATUS
file_append(file_t* file, 
		    BYTE* buffer, 
		    size_t writeBytes) {
	UINT bytesLeftInBlock;
					int i;
	
	OS_Wait(&DiskMutex);
					
	if(file->initData.fileSize == 0) {
		bytesLeftInBlock = BLOCK_DATA_BYTES;
	}
	else if(file->initData.fileSize%BLOCK_DATA_BYTES == 0) {
		bytesLeftInBlock = 0;
	}
	else {
		bytesLeftInBlock = BLOCK_DATA_BYTES-(file->initData.fileSize%BLOCK_DATA_BYTES);
	}
					
	fileReset(file);
					
	// go to end
	file_skip(file, file->initData.fileSize);
	
	if(bytesLeftInBlock == 0) {
		INT newBlock = Freespace_AllocateBlock();
		
		memcpy(&file->currentBlockData[BLOCK_NEXT_INDEX], (BYTE*)(&newBlock), sizeof(newBlock));
		AssertAndRelease(eDisk_WriteBlock(file->currentBlockData, file->currentBlockNumber) == RES_OK,&DiskMutex);
		AssertAndRelease(eDisk_ReadBlock(file->currentBlockData, newBlock) == RES_OK,&DiskMutex);
		
		file->currentBlockNumber = newBlock;
		file->currentBlockPosition = 0;
		
		bytesLeftInBlock = BLOCK_DATA_BYTES;
	}
					
	// write remainder of the current block data to be block
	// aligned
	if(bytesLeftInBlock < writeBytes) {
		INT newBlock = Freespace_AllocateBlock();
		
		memcpy(&file->currentBlockData[file->currentBlockPosition], buffer, bytesLeftInBlock);

		AssertAndRelease(newBlock != -1,&DiskMutex);

		UINT_AT(&file->currentBlockData[BLOCK_NEXT_INDEX]) = newBlock;
		
		AssertAndRelease(eDisk_WriteBlock(file->currentBlockData, file->currentBlockNumber) == RES_OK,&DiskMutex);
		file->initData.fileSize += bytesLeftInBlock;
		
		buffer = &buffer[bytesLeftInBlock];
		i = fileAppend(file, buffer, writeBytes-bytesLeftInBlock, newBlock);
		OS_Signal(&DiskMutex);
		return i;
	}
	else {
		memcpy(&file->currentBlockData[file->currentBlockPosition], buffer, writeBytes);
		AssertAndRelease(eDisk_WriteBlock(file->currentBlockData, file->currentBlockNumber) == RES_OK,&DiskMutex);
	}

	file->initData.fileSize += writeBytes;
	AssertAndRelease(dirUpdate(file) == SUCCESS,&DiskMutex);
	// save the free space vector
	AssertAndRelease(Freespace_SaveFSV() == RES_OK,&DiskMutex);
	
	fileReset(file);

	OS_Signal(&DiskMutex);
	return (SUCCESS);
}

// if the location of the dirEntry for a file
// is already known then this function can be
// used to read the dirEntry based on that 
// information. This is used by fs_init
// because the dirEntry information for the 
// directory file is at a fixed location 
// (right before the rest of the directory file)
static SF_STATUS
dirEntryAt(directory_location_t* dirLocation,
		       directory_entry_t* dirEntry) {
	static file_t dirEntryHandle;

	dirEntryHandle.bytesRemaining = sizeof(directory_entry_t);
	dirEntryHandle.currentBlockNumber = dirLocation->directoryStartBlock;
	dirEntryHandle.currentBlockPosition = dirLocation->directoryBlockStart;
						 
	eDisk_ReadBlock(dirEntryHandle.currentBlockData, dirLocation->directoryStartBlock);

	ASSERT(file_read(&dirEntryHandle, (BYTE*)dirEntry, sizeof(directory_entry_t)) != -1);

	return (SUCCESS);
}

static SF_STATUS
dirUpdate(file_t* file) {
	ASSERT(eDisk_ReadBlock(file->currentBlockData, file->directoryLocation.directoryStartBlock) == RES_OK);
	memcpy(&file->currentBlockData[file->directoryLocation.directoryBlockStart], &file->initData, sizeof(directory_entry_t));
	ASSERT(eDisk_WriteBlock(file->currentBlockData, file->directoryLocation.directoryStartBlock) == RES_OK);
	
	return (SUCCESS);
}

// look up a file name in the directory
// and return initial data about it through
// the dirEntry and dirLocation structs
//
// fileName - char buffer containing the file
//     name
// fileNameSize - length of fileName buffer
// dirEntry - location where the information
//     about the directory entry for this file
//     is stored
// dirLocation - location information in directory
//     where dirEntry was found
//
// return - SUCCESS if found file, FAILURE otherwise
static SF_STATUS
dirLookup(const char* fileName,
		  size_t fileNameSize,
		  directory_entry_t* dirEntry,
		  directory_location_t* dirLocation) {
	static char fileNameBuffer[MAXIMUM_FILE_NAME_LENGTH];
	directory_entry_t nextDirectoryEntry;

	// skip the first part which is the metadata for the directory
	// itselfF
	fileReset(&directoryFile);
	file_skip(&directoryFile, sizeof(directory_entry_t));
				
	while(file_read(&directoryFile, (BYTE*)(&nextDirectoryEntry), sizeof(nextDirectoryEntry))) {
		if(fileNameSize != nextDirectoryEntry.fileNameLength || nextDirectoryEntry.skip) {
			// if the length of the file name doesn't match, no need to
			// compare. just skip
			file_skip(&directoryFile, nextDirectoryEntry.fileNameLength);
		}
		else {
			UINT dirBlockStart = directoryFile.currentBlockPosition - sizeof(nextDirectoryEntry);
			UINT dirStartBlock = directoryFile.currentBlockNumber;

			// read in file name and compare
			file_read(&directoryFile, (BYTE*)(&fileNameBuffer), nextDirectoryEntry.fileNameLength);

			fileNameBuffer[nextDirectoryEntry.fileNameLength] = '\0';
			
			// if the file name matches then initialize the file 
			// structure
			if(!strncmp(fileNameBuffer, fileName, fileNameSize)) {
				// a match was found, so copy in this data
				memcpy(dirEntry, &nextDirectoryEntry, sizeof(directory_entry_t));
				dirLocation->directoryBlockStart = dirBlockStart;
				dirLocation->directoryStartBlock = dirStartBlock;

				return (SUCCESS);
			}
		}
	}

	return (FAILURE);
}

// initialize the file structure for a file whose
// directory entry is contained in 
// dirEntry and whose directory entry location is
// contained within dirLocation. This is more generic
// than the public file_open function and is used
// to treat the directory just like another file
// whose starting block is known
//
// preconditions - dirEntry and dirLocation must
//      be initialized (using dirLookup or dirEntryAt)
// postconditions - file is initialized
//
// file - file struct to initialize
// dirEntry - directory entry for this file, obtained
//     specifying the file's size and starting block
// dirLocation - specifies where to find the entry
//     for this file in the directory. This information
//     is used for deleting a file
static void
fileOpen(file_t* file,
		 directory_entry_t* dirEntry,
		 directory_location_t* dirLocation) {
	file->bytesRemaining = dirEntry->fileSize;
	file->currentBlockNumber = 0;
	file->currentBlockPosition = 0;
	memcpy(&file->directoryLocation, dirLocation, sizeof(directory_location_t));
	memcpy(&file->initData, dirEntry, sizeof(directory_entry_t));
}

// used to reset a file for reading. makes sure next
// call to file_read reads from the beginning of the
// file
//
// preconditions - file must be initialized
// postconditions - file is reset for reading
//
// file - file to reset for reading. must be initialized
static void
fileReset(file_t* file) {
	file->bytesRemaining = file->initData.fileSize;
	file->currentBlockNumber = 0;
	file->currentBlockPosition = 0;
}

// used to append to a file from the start of
// a block.
// preconditions - startBlock must be newly allocated.
//     this function will overwrite the block starting
//     at the beginning and makes the assumption that
//     all blocks after startBlock should be newly 
//     allocated. file must be initialized
// postconditions - if successful, file data 
//     specified in buffer has been appended to the 
//     file from startBlock onwards and all blocks 
//     in the file are correctly linked together. 
//     file is reset for reading
// 
// file - file structure representing file to be
//     written to
// buffer - pointer to data to be written
// writeBytes - number of bytes to write from buffer
// startBlock - block at which to start writing
//
// return - SUCCESS if successful, FAILURE otherwise
static SF_STATUS
fileAppend(file_t* file, 
		   BYTE* buffer, 
		   size_t writeBytes, 
		   UINT startBlock) {
	UINT totalBytes = writeBytes;
	UINT currentBlockNumber = startBlock;
	UINT wholeBlocksRemaining = writeBytes/BLOCK_DATA_BYTES;
				 int i;
	// write all whole blocks
	while(wholeBlocksRemaining > 0) {
		UINT savedBlockData;

		// write block data
		wholeBlocksRemaining --;
		writeBytes -= BLOCK_DATA_BYTES;
		memcpy(file->currentBlockData, buffer, BLOCK_DATA_BYTES);
		buffer = &buffer[BLOCK_DATA_BYTES];

		// allocate another block if there is more data
		if(writeBytes > 0) {
			INT_AT(&file->currentBlockData[BLOCK_NEXT_INDEX]) = Freespace_AllocateBlock();

			ASSERT(INT_AT(&file->currentBlockData[BLOCK_NEXT_INDEX]) != -1);
		}
		
		ASSERT(eDisk_WriteBlock(file->currentBlockData, currentBlockNumber) == RES_OK);

		// get next block number
		currentBlockNumber = INT_AT(&file->currentBlockData[BLOCK_NEXT_INDEX]);
	}

	if(writeBytes > 0) {
		// write final (partial) block
		ASSERT(fileWritePartial(file, buffer, writeBytes, currentBlockNumber) == SUCCESS);
	}

	file->initData.fileSize += totalBytes;
	
	// save the free space vector
	ASSERT(Freespace_SaveFSV() == RES_OK);
	ASSERT(dirUpdate(file) == SUCCESS);
	
	// a successive read
	fileReset(file);
	

	return (SUCCESS);
}

// writes partial data to a block. in
// other words, the number of bytes 
// to be written must not exceed the
// number of data bytes per block.
// this helper function is used for writing
// the remainder of data after writing all
// full blocks of data
// 
// preconditions - writeBytes must be <=
//     BLOCK_DATA_BYTES 
// postconditions - the specified block
//     has had writeBytes written to it
//     and file is reset for reading
//
// file - file structure representing file to be
//     written to
// buffer - data to write
// writeBytes - number of bytes to write (<= BLOCK_DATA_BYTES)
// startBlock - block which will be modified
//
// return - SUCCESS if successful, FAILURE otherwise
static SF_STATUS
fileWritePartial(file_t* file, 
		         BYTE* buffer, 
		         size_t writeBytes, 
		         UINT startBlock) {
	ASSERT(writeBytes <= BLOCK_DATA_BYTES);
	memcpy(file->currentBlockData, buffer, writeBytes);

	ASSERT(eDisk_WriteBlock(file->currentBlockData, startBlock) == RES_OK);

	fileReset(file);

	return (SUCCESS);
}
