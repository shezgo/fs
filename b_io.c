/**************************************************************
 * Class::  CSC-415-01 Summer 2024
 * Name:: Shez Rahman
 * Student IDs:: 916867424
 * GitHub-Name:: shezgo
 * Group-Name:: Independent
 * Project:: Basic File System
 *
 * File:: b_io.c
 *
 * Description:: Basic File System - Key File I/O Operations
 *
 **************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> // for malloc
#include <string.h> // for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb
{
	/** TODO add al the information you need in the file control block **/
	char *buf;				 // holds the open file buffer
	char fileName[NAME + 1]; // holds the name of the file
	int index;				 // holds the current position in the buffer
	int buflen;				 // holds how many valid bytes are in the buffer
	int flags;				 // holds the permissions value for O_RDONLY (0), O_WRONLY (1), O_RDWR (2)
	int blockTracker;		 // holds the current block for tracking which can differ from start block
	int startBlock;			 // holds the starting block of the file
	int numBytesRead;		 // If this int reaches the file size, then end of file is reached.
	int eof;				 // The value is 0 if EOF has not been reached, and is 1 if reached.
	int fileSize;			 // The size of the file
	DE *parent;
	int parentLei;

} b_fcb;

b_fcb fcbArray[MAXFCBS];

int startup = 0; // Indicates that this has not been initialized

// Method to initialize our file system
void b_init()
{
	// init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
	{
		fcbArray[i].buf = NULL; // indicates a free fcbArray
		strcpy(fcbArray[i].fileName, "");
		fcbArray[i].buflen = -1;
		fcbArray[i].index = -1;
		fcbArray[i].flags = -1;
		fcbArray[i].blockTracker = -1;
		fcbArray[i].startBlock = -1;
		fcbArray[i].numBytesRead = -1;
		fcbArray[i].eof = -1;
		fcbArray[i].fileSize = -1;
		fcbArray[i].parent = NULL;
		fcbArray[i].parentLei = -1;
	}

	startup = 1;
}

// Method to get a free FCB element
b_io_fd b_getFCB()
{
	for (int i = 0; i < MAXFCBS; i++)
	{
		if (fcbArray[i].buf == NULL)
		{
			return i; // Not thread safe (But do not worry about it for this assignment)
		}
	}
	return (-1); // all in use
}

// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY (0), O_WRONLY (1), or O_RDWR (2)
b_io_fd b_open(char *filename, int flags)
{
	/*
	 add a field to ppinfo - isFile

So, in b_open, what cases do I need?
1. If path is an existing file - done
2. If path is a valid pathname up until the 2nd to last element, which does not exist and will be the name for the new file created by touch.
3. If path is not a valid pathname up until the last element, or up until the second to last element?? clarify.

what's my current case? If the file exists.

what if b_open somehow edited the path for parsePath to process by trimming it?

Check if the full path is a directory. - done
- If so, return not a file error.
Check if the full path is an existing file with parsePath - done
- If so, create the fcb in fcbArray and return the fd (index) of it.

If neither of the above, check if the path until 2nd to last element is a valid directory:
Pass a trimmed path into parsePath. No handling for absolute/relative needed here.
	How:
	char *pathCopy = malloc(CWD_SIZE);
	if (path == NULL)
	{
		fprintf(stderr, "path is null\n");
		return -1;
	}
	strcpy(pathCopy, filename);

	if (pathCopy == NULL)
	{
		perror("strcpy failed");
		return -1;
	}

	char *saveptr;
	char *token1 = strtok_r(pathCopy, '/', &saveptr);

	char *token2;

start do loop.
	Will it ever just be one token? Yes, in this case, create the file in cwd.

	Otherwise, loop through. Keep pulling token2 from strtok_r.
	If token2 is NOT NULL:
		We add '/' and token1 to our new path with strcat.
		Keep looping.
	If token2 is NULL:
		we do NOT add token1 to our new path - token1 will be the filename.
		exit the loop
	Call parsePath on the newPath.
	if path is a valid directory, create a new file with token1 filename
	in the dir, save in its parent DE.
	Create the file itself - what does this entail? Clearing bytes? etc.


If this path is valid and is a directory, create the new file.
Else, return an error.
	*/

	if (filename == NULL)
	{
		fprintf(stderr, "path is null\n");
		return -1;
	}

	ppinfo ppi;
	int parseFlag = parsePath(filename, &ppi);

	//*DEBUG this block is executing and returning -1 to cmd_cat.
	// Check that the file exists and is not a directory.
	if ((ppi.parent[ppi.lei].isDirectory == 1) ||
		(parseFlag == -1 && !(ppi.isFile)))
	{
		printf("parseFlag: %d\n", parseFlag);
		printf("ppi.parent[ppi.lei].LBAlocation:%ld\n", ppi.parent[ppi.lei].LBAlocation);
		printf("vcb->root_directory_block:%d\n", vcb->root_directory_block);
		printf("rootGlobal.LBAlocation:%ld\n", rootGlobal->LBAlocation);
		fprintf(stderr, "b_open: parsePath failed or path is a directory\n");
		return -1;
	}

	// Continue if valid.
	b_io_fd returnFd;

	if (startup == 0)
		b_init(); // Initialize our system

	returnFd = b_getFCB(); // get our own file descriptor
						   // check for error - all used FCB's
	if (returnFd == -1)
	{
		fprintf(stderr, "Error: fcbArray is already full\n");
		return -1;
	}

	// This should be executing for basic filepath exists case.
	if (ppi.isFile)
	{
		printf("b_open ppi.isFile block\n");
		// Allows multiple fcb for the same file, can add mutex locks later.
		strcpy(fcbArray[returnFd].fileName, ppi.parent[ppi.lei].name);
		fcbArray[returnFd].buflen = ppi.parent[ppi.lei].size; // DEBUG should this be vcb->block_size
		fcbArray[returnFd].buf = malloc(vcb->block_size);
		for (int i = 0; i < vcb->block_size; i++)
		{
			fcbArray[returnFd].buf[i] = '0';
		}
		fcbArray[returnFd].index = 0;
		fcbArray[returnFd].flags = flags;
		fcbArray[returnFd].blockTracker = ppi.parent[ppi.lei].LBAlocation;
		fcbArray[returnFd].startBlock = ppi.parent[ppi.lei].LBAlocation;
		fcbArray[returnFd].numBytesRead = 0;
		fcbArray[returnFd].eof = 0;
		fcbArray[returnFd].fileSize = ppi.parent[ppi.lei].size;
		fcbArray[returnFd].parent = ppi.parent;
		fcbArray[returnFd].parentLei = ppi.lei;

		printf("ppi.parent[fd].startBlock:%ld\nppi.parent[fd].size:%ld\n", ppi.parent[ppi.lei].LBAlocation, ppi.parent[ppi.lei].size);
		printf("fcb[fd].startBlock:%d\nfcb[fd].fileSize:%d\n", fcbArray[returnFd].startBlock, fcbArray[returnFd].fileSize);

		if (fcbArray[returnFd].buf == NULL)
		{
			fprintf(stderr, "b_open: Memory allocation failed.\n");
			b_close(returnFd);
			return -1;
		}
		return (returnFd);
	}

	// Path does not lead to an existing file, so trim off the last element in path,
	// verify that the path up until the filename exists, and create that filename in
	// the parent directory.
	if (flags & O_CREAT)
	{
		char *pathCopy = malloc(CWD_SIZE);

		strcpy(pathCopy, filename);

		if (pathCopy == NULL)
		{
			perror("strcpy failed");
			return -1;
		}

		char *saveptr;
		char *token1 = strtok_r(pathCopy, "/", &saveptr);

		char *token2;
		int cwdFlag = 0; // set to 1 if creating a file within cwd
		char *newPath = malloc(CWD_SIZE);
		if (newPath == NULL)
		{
			fprintf(stderr, "b_open: newPath malloc failure.\n");
			return -1;
		}

		do
		{
			token2 = strtok_r(NULL, "/", &saveptr);
			cwdFlag++;

			// If all that is given is the filename, create that.
			if (cwdFlag == 1 && token2 == NULL)
			{
				DE *parentOfFile = cwdGlobal;

				int x = findUnusedDE(parentOfFile);
				parentOfFile[x].dirNumBlocks = FILE_NUM_BLOCKS;
				parentOfFile[x].isDirectory = 0;
				parentOfFile[x].lastAccessed = (time_t)(-1);
				parentOfFile[x].lastModified = (time_t)(-1);
				parentOfFile[x].LBAindex = -1;

				int newLoc = fsAlloc(bm, FILE_NUM_BLOCKS);
				parentOfFile[x].LBAlocation = newLoc;
				strcpy(parentOfFile[x].name, token1);
				parentOfFile[x].size = 0;
				parentOfFile[x].timeCreation = (time_t)(-1);

				// Update parentOfFile
				int saveRet = saveDir(parentOfFile);
				if (saveRet != 1)
				{
					fprintf(stderr, "b_open: could not saveDir.\n");
					return -1;
				}

				strcpy(fcbArray[returnFd].fileName, parentOfFile[x].name);
				fcbArray[returnFd].buflen = parentOfFile[x].size;
				fcbArray[returnFd].buf = malloc(vcb->block_size);
				fcbArray[returnFd].index = 0;
				fcbArray[returnFd].flags = flags;
				fcbArray[returnFd].blockTracker = parentOfFile[x].LBAlocation;
				fcbArray[returnFd].startBlock = parentOfFile[x].LBAlocation;
				fcbArray[returnFd].numBytesRead = 0;
				fcbArray[returnFd].eof = 0;
				fcbArray[returnFd].fileSize = parentOfFile[x].size;
				fcbArray[returnFd].parent = parentOfFile;
				fcbArray[returnFd].parentLei = x;

				if (fcbArray[returnFd].buf == NULL)
				{
					fprintf(stderr, "b_open: Memory allocation failed.\n");
					b_close(returnFd);
					free(newPath);
					return -1;
				}
				free(newPath);
				return (returnFd);
			}

			if (token2 != NULL)
			{
				// Add '/' and token1 to newPath
				strcat(newPath, "/");
				strcat(newPath, token2);
				token1 = token2;
			}
		} while (token2 != NULL);

		/*
		If the code has made it here, newPath is necessary and has been built, but no file created yet.
		If newPath is a valid dir via parsePath, create a new
		file with token1 as name in newPath as parent.

		This code will never run if parsePath fails on the first path..right?
		*/
		ppinfo ppi2;
		int ppRet = parsePath(newPath, &ppi2);
		if (ppRet != -1)
		{
			/*
			TODO:
				ppi2.parent[ppi2.lei] IS the parent
				Create a DE to describe the child file in this parent.
				LBAlocation -1 and size will be 0.
				LBAlocation and size to be updated with cat or other methods that fill the file.

			*/
			DE *parentOfFile = loadDirLBA(ppi2.parent[ppi2.lei].LBAlocation,
										  ppi2.parent[ppi2.lei].dirNumBlocks);

			int x = findUnusedDE(parentOfFile);
			parentOfFile[x].dirNumBlocks = FILE_NUM_BLOCKS;
			parentOfFile[x].isDirectory = 0;
			parentOfFile[x].lastAccessed = (time_t)(-1);
			parentOfFile[x].lastModified = (time_t)(-1);
			parentOfFile[x].LBAindex = -1;

			int newLoc = fsAlloc(bm, FILE_NUM_BLOCKS);
			parentOfFile[x].LBAlocation = newLoc;
			strcpy(parentOfFile[x].name, token1);
			parentOfFile[x].size = 0;
			parentOfFile[x].timeCreation = (time_t)(-1);

			// Update parentOfFile
			int saveRet = saveDir(parentOfFile);
			if (saveRet != 1)
			{
				fprintf(stderr, "b_open: could not saveDir.\n");
				return -1;
			}

			strcpy(fcbArray[returnFd].fileName, parentOfFile[x].name);
			fcbArray[returnFd].buflen = parentOfFile[x].size;
			fcbArray[returnFd].buf = malloc(vcb->block_size);
			fcbArray[returnFd].index = 0;
			fcbArray[returnFd].flags = flags;
			fcbArray[returnFd].blockTracker = parentOfFile[x].LBAlocation;
			fcbArray[returnFd].startBlock = parentOfFile[x].LBAlocation;
			fcbArray[returnFd].numBytesRead = 0;
			fcbArray[returnFd].eof = 0;
			fcbArray[returnFd].fileSize = parentOfFile[x].size;
			fcbArray[returnFd].parent = parentOfFile;
			fcbArray[returnFd].parentLei = x;

			if (fcbArray[returnFd].buf == NULL)
			{
				fprintf(stderr, "b_open: Memory allocation failed.\n");
				b_close(returnFd);
				return -1;
			}

			free(parentOfFile);
			free(newPath);
			return (returnFd);
		}

		free(newPath);
	}
	return (returnFd); // all set
}

// Interface to seek function
int b_seek(b_io_fd fd, off_t offset, int whence)
{
	if (startup == 0)
		b_init(); // Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); // invalid file descriptor
	}

	return (0); // Change this
}

// Interface to write function
/*
Write is going to write count bytes from the user's buffer into
the file at fd's location.

Confirm: what's fcb.buf used for? Why is it separate from the
user's buffer, passed into the function?
fcb tracks reading for a specific file - its buffer is malloced for block_size bytes.
So if you want to track where an FD is regarding a file,
use/update:
fcbArray[].index,
fcbArray[].flags (at open),
fcbArray[].blockTracker,
fcbArray[].startBlock (stays the same but use as reference)
fcbArray[].fileSize,
startBlock and fileSize can be used together to determine blocks,
eof when numBytesRead == fileSize.

PICKUP: refactor fcbArray in conjunction with write pseudocode.

So what are the steps to write from a buffer into an fd?
Write as many bytes from the buffer into the fd as you can.
How big can the buffer potentially be? "Infinitely" long, but we
know the count requested.

We know fd.buf is blocksize bytes (512).

Count can either be: <= blocksize or > blocksize.

If count <= blocksize:
write count bytes from buffer to fd
update fd's index = index + count
is index used for reading, writing, or both
to change the index, you'd call b_seek.
^what if index was at 200 and count was blocksize? you'd fill
fd's buffer past blocksize. Instead, need to check index + count
and handle any spillage/multiple block situations.

*/
int b_write(b_io_fd fd, char *buffer, int count)
{
	if (fcbArray[fd].flags == O_RDONLY)
	{
		printf("Write access not granted.\n");
		return -1;
	}

	if (startup == 0)
		b_init(); // Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); // invalid file descriptor
	}

	if (fcbArray[fd].buf == NULL || buffer == NULL) // File not open for this descriptor
	{
		printf("Invalid fcb buffer or user buffer.\n");
		return -1;
	}

	if (count < 0)
	{
		printf("Invalid value for count.\n");
		return -1;
	}

	int writeCount = count;

	// If writing less than a block from fcbArray[fd]'s current index:
	if (fcbArray[fd].index + writeCount <= vcb->block_size)
	{
		printf("b_write: first clause\n");
		memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer, writeCount);
		int writeRet = LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].blockTracker);

		int writeStop = (((fcbArray[fd].blockTracker - fcbArray[fd].startBlock) * vcb->block_size) + fcbArray[fd].index + writeCount);
		if (fcbArray[fd].index + writeCount == vcb->block_size)
		{
			fcbArray[fd].index = 0;
			fcbArray[fd].blockTracker++;
		}
		else
		{
			fcbArray[fd].index += writeCount;
		}
		// printf("b_write: writeCount:%d\nfcbArray[fd].index:%d\nfcbArray[fd].blockTracker:%d\nfcbArray[fd].startBlock:%d\n",writeCount, fcbArray[fd].index, fcbArray[fd].blockTracker,fcbArray[fd].startBlock);
		//  Check/update fileSize if larger than before.

		printf("b_write: writeStop:%d\n", writeStop);
		if (writeStop > fcbArray[fd].fileSize)
		{
			fcbArray[fd].fileSize = writeStop;
			printf("1 b_write: fcbArray[fd].fileSize:%d\n", fcbArray[fd].fileSize);

			fcbArray[fd].parent[fcbArray[fd].parentLei].size = writeStop;
			printf("2 b_write: fcbArray[fd].parent[fcbArray[fd].parentLei].size: %ld\n", fcbArray[fd].parent[fcbArray[fd].parentLei].size);

			saveDir(fcbArray[fd].parent);
		}

		printf("3 b_write: fcb[fd].fileSize:%d\nfcb[fd].startBlock:%d\n", fcbArray[fd].fileSize, fcbArray[fd].startBlock);
		printf("4 b_write: fcb.parent[lei].size:%ld\nfcb.parent[lei].startBlock:%ld\n",
			   fcbArray[fd].parent[fcbArray[fd].parentLei].size, fcbArray[fd].parent[fcbArray[fd].parentLei].LBAlocation);

		printf("b_write: fcbArray[fd].parentLei:%d\n", fcbArray[fd].parentLei);

		return writeCount;
	}

	// DEBUG PICKUP: Ensure file sizes update properly from below and onwards.
	else
	{
		/*
		If we'll need to write multiple times.

		Cases:
		1.
		1a. index > 0, fill up fcb.buf, write first block to disk.
		1b. Then check how many bytes left to write.
		If >= 1 block left, write as many
		as possible straight to disk with an lbawrite call.
		1c. Write any left over bytes to buf and zero out the rest.
		LBAwrite that last block.



		*/

		/*
		If index > 0, write enough bytes to increment block and reset index to 0.
		Subtract these bytes from writeCount.
		*/
		if (fcbArray[fd].index > 0)
		{
			memcpy(fcbArray[fd].buf + fcbArray[fd].index, buffer,
				   vcb->block_size - fcbArray[fd].index);
			printf("b_write: filling current block before crossing into next block\n");
			LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].blockTracker);
			writeCount = writeCount - (vcb->block_size - fcbArray[fd].index);
			// Now, have written count - writeCount bytes.
			fcbArray[fd].fileSize += (vcb->block_size - fcbArray[fd].index);
			fcbArray[fd].blockTracker++;
			fcbArray[fd].index = 0;
		}

		/*
		Write as many full blocks as you can straight to disk in this block,
		then use fcb buf for remaining bytes in next code block.
		*/
		if (writeCount / vcb->block_size >= 1)
		{
			int numBlocks = writeCount / vcb->block_size;
			LBAwrite(buffer + (count - writeCount), numBlocks, fcbArray[fd].blockTracker);
			fcbArray[fd].blockTracker += numBlocks;
			writeCount = writeCount - (numBlocks * vcb->block_size);
			fcbArray[fd].fileSize += (numBlocks * vcb->block_size);
			// Now, have written count - writeCount bytes.
		}

		/*
		Write any remaining bytes using fcb buf
		use memcpy.
		Starting location from buffer will be?
		buffer + (count - writeCount)
		*/
		if (writeCount / vcb->block_size < 1 && !(writeCount <= 0))
		{

			memcpy(fcbArray[fd].buf, buffer + (count - writeCount), writeCount);
			LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].blockTracker);
			fcbArray[fd].fileSize += writeCount;
			fcbArray[fd].index += writeCount;
			writeCount = writeCount - writeCount;
		}
	}
	// Check/update fileSize if needed.
	int writeStop = ((fcbArray[fd].blockTracker - fcbArray[fd].startBlock) * vcb->block_size + fcbArray[fd].index + writeCount);
	saveDir(fcbArray[fd].parent);

	return count - writeCount;
}

// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read(b_io_fd fd, char *buffer, int count)
{
	if (fcbArray[fd].flags == O_WRONLY)
	{
		printf("Read access not granted.\n");
		return -1;
	}

	if (startup == 0)
		b_init(); // Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); // invalid file descriptor
	}

	if (fcbArray[fd].buf == NULL) // File not open for this descriptor
	{
		printf("b_read: File not open for this descriptor.\n");
		return -1;
	}

	int readCount = count;
	int actualCount = 0; // This will count how many bytes we actually read into user's buffer.

	// Check if the end of file will be reached with this call, trim readCount if needed.
	if ((fcbArray[fd].numBytesRead + readCount) >= fcbArray[fd].fileSize)
	{
		//printf("b_read block 1:fcbArray[fd].fileSize:%d\n", fcbArray[fd].fileSize);
		//printf("fd.fileSize:%d\nfd.numBytesRead:%d", fcbArray[fd].fileSize, fcbArray[fd].numBytesRead);
		readCount = fcbArray[fd].fileSize - fcbArray[fd].numBytesRead;

		fcbArray[fd].eof = 1;
		//printf("b_read: readCount trimmed from count:%d to %d\n", count, readCount);
		if (readCount == 0)
		{
			printf("b_read: End of file reached - nothing to read.\n");
			return 0; // 0 bytes copied into user's buffer
		}
	}

	// If we need to read in the first block from disk.
	if (fcbArray[fd].blockTracker == fcbArray[fd].startBlock && fcbArray[fd].index == 0)
	{
		//printf("b_read: block 2\n");
		int readRet = LBAread(fcbArray[fd].buf, 1, fcbArray[fd].startBlock);
		if (readRet != 1)
		{
			fprintf(stderr, "b_read: LBAread error 1.\n");
		}
		// Do not update fd's blockTracker here - that only updates once the fd's index > blocksize.
	}

	// Part 1: if no new block needs to be read into the fd's buffer.
	if (readCount < (vcb->block_size - fcbArray[fd].index))
	{
		//printf("b_read: block 3\n");
		memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].index, readCount);
		fcbArray[fd].index += readCount;
		fcbArray[fd].numBytesRead += readCount;
		actualCount += readCount;
		return actualCount;
	}

	// Clear the current fd's buffer. In following code blocks, more blocks may be read from disk.
	if (readCount >= (vcb->block_size - fcbArray[fd].index))
	{
		//printf("b_read: block 4\n");
		memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].index, (vcb->block_size - fcbArray[fd].index));
		//printf("fd.buf:\n%s", fcbArray[fd].buf);
		//printf("\nbuffer:\n%s", buffer);
		//printf("write size:%d\n", (vcb->block_size - fcbArray[fd].index));
		fcbArray[fd].blockTracker++;
		actualCount += (vcb->block_size - fcbArray[fd].index);
		fcbArray[fd].index = 0;
		readCount = readCount - actualCount;
		fcbArray[fd].numBytesRead += actualCount;
	}

	// Read in blocks that can be read in full straight to user's buffer.
	if (readCount / vcb->block_size > 0)
	{
		//printf("b_read: block 5\n");
		int numBlocks = readCount / vcb->block_size;
		int readRet = LBAread(buffer + actualCount, numBlocks, fcbArray[fd].blockTracker);
			//DEBUG statements
		//printf("readCount:%d\nnumBlocks:%d\nreadRet:%d\n", readCount, numBlocks, readRet);
		if (readRet != numBlocks)
		{
			fprintf(stderr, "b_read: LBAread error 2.\n");
			return -1;
		}

		fcbArray[fd].blockTracker += numBlocks;
		// No changes needed to index, handled in previous if block.
		actualCount += (vcb->block_size * numBlocks);
		readCount = readCount - (vcb->block_size * numBlocks);
		fcbArray[fd].numBytesRead += actualCount;
	}

	// Read in any leftover bytes needed after full block chunks.
	if (readCount > 0)
	{
		//printf("b_read: block 6\n");
		LBAread(fcbArray[fd].buf, 1, fcbArray[fd].blockTracker);
		memcpy(buffer + actualCount, fcbArray[fd].buf + fcbArray[fd].index, readCount);
		fcbArray[fd].index += readCount;
		fcbArray[fd].numBytesRead += readCount;
		actualCount += readCount;
		readCount = readCount - readCount;
		
	}

	if (fcbArray[fd].eof == 1)
	{
		printf("b_read: 3End of file reached.\n");
	}

	return actualCount;
}

// Interface to Close the file. Returns -1 if failed, 0 if success.
int b_close(b_io_fd fd)
{
	if (fd < 0 || fd >= MAXFCBS)
	{
		fprintf(stderr, "b_close: fd is out of bounds.\n");
		return -1;
	}
	int i = fd;
	free(fcbArray[i].buf);
	fcbArray[i].buf = NULL;			  // indicates a free fcbArray
	strcpy(fcbArray[i].fileName, ""); // same as fcbArray[i].fileName[0] = '\0';
	fcbArray[i].buflen = -1;
	fcbArray[i].index = -1;
	fcbArray[i].flags = -1;
	fcbArray[i].blockTracker = -1;
	fcbArray[i].startBlock = -1;
	fcbArray[i].numBytesRead = -1;
	fcbArray[i].eof = -1;
	fcbArray[i].fileSize = -1;
	fcbArray[i].parentLei = -1;

	if (fcbArray[i].parent != rootGlobal && fcbArray[i].parent != cwdGlobal)
	{
		free(fcbArray[i].parent);
	}

	return 0;
}
