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
	int bufferTracker;		 // holds the index inside the current block for tracking last read value
	int startBlock;			 // holds the starting block of the file
	int numBytesRead;		 // If this int reaches the file size, then end of file is reached.
	int eof;				 // The value is 0 if EOF has not been reached, and is 1 if reached.
	int fileSize;			 // The size of the file

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
		fcbArray[i].bufferTracker = -1;
		fcbArray[i].startBlock = -1;
		fcbArray[i].numBytesRead = -1;
		fcbArray[i].eof = -1;
		fcbArray[i].fileSize = -1;
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

	// Check that the file exists and is not a directory.
	if (((ppi.parent[ppi.lei].isDirectory == 1) &&
		 ppi.parent[0].LBAlocation != vcb->root_directory_block) ||
		(parseFlag == -1 && !(flags & O_CREAT)) ||
		(ppi.parent[ppi.lei].isDirectory == 1))
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

	// This should be executing for basic filepath exists case, why not?
	if (ppi.isFile)
	{
		printf("b_open ppi.isFile block\n");
		// Allows multiple fcb for the same file, can add mutex locks later.
		strcpy(fcbArray[returnFd].fileName, ppi.parent[ppi.lei].name);
		fcbArray[returnFd].buflen = ppi.parent[ppi.lei].size;
		fcbArray[returnFd].buf = malloc(vcb->block_size);
		fcbArray[returnFd].index = 0;
		fcbArray[returnFd].flags = flags;
		fcbArray[returnFd].blockTracker = ppi.parent[ppi.lei].LBAlocation;
		fcbArray[returnFd].bufferTracker = 0;
		fcbArray[returnFd].startBlock = ppi.parent[ppi.lei].LBAlocation;
		fcbArray[returnFd].numBytesRead = 0;
		fcbArray[returnFd].eof = 0;
		fcbArray[returnFd].fileSize = ppi.parent[ppi.lei].size;

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
				parentOfFile[x].dirNumBlocks = 0;
				parentOfFile[x].isDirectory = 0;
				parentOfFile[x].lastAccessed = (time_t)(-1);
				parentOfFile[x].lastModified = (time_t)(-1);
				parentOfFile[x].LBAindex = -1;
				parentOfFile[x].LBAlocation = -1;
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
				fcbArray[returnFd].bufferTracker = 0;
				fcbArray[returnFd].startBlock = parentOfFile[x].LBAlocation;
				fcbArray[returnFd].numBytesRead = 0;
				fcbArray[returnFd].eof = 0;
				fcbArray[returnFd].fileSize = parentOfFile[x].size;

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
			parentOfFile[x].dirNumBlocks = 0;
			parentOfFile[x].isDirectory = 0;
			parentOfFile[x].lastAccessed = (time_t)(-1);
			parentOfFile[x].lastModified = (time_t)(-1);
			parentOfFile[x].LBAindex = -1;
			parentOfFile[x].LBAlocation = -1;
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
			fcbArray[returnFd].bufferTracker = 0;
			fcbArray[returnFd].startBlock = parentOfFile[x].LBAlocation;
			fcbArray[returnFd].numBytesRead = 0;
			fcbArray[returnFd].eof = 0;
			fcbArray[returnFd].fileSize = parentOfFile[x].size;

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

*/
int b_write(b_io_fd fd, char *buffer, int count)
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
	if (fcbArray[fd].flags == 1)
	{
		printf("Read access not granted.\n");
		return 0;
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
		return -1;
	}

	// Check if the end of file will be reached with this call, trim readCount if needed.
	int readCount = count;
	if ((fcbArray[fd].numBytesRead + readCount) >= fcbArray[fd].fileSize)
	{
		readCount = fcbArray[fd].fileSize - fcbArray[fd].numBytesRead;
		fcbArray[fd].eof = 1;
		if (readCount == 0)
		{
			printf("End of file reached - nothing to read.\n");
			return 0; // 0 bytes copied into user's buffer
		}
	}

	// If user has partially read a file and the amount they're requesting now doesn't require
	// reading in a new block, or if only one block needs to be LBAread.
	if (readCount <= (vcb->block_size - fcbArray[fd].bufferTracker))
	{
		// If there's nothing in the fcb buffer, read a new block into the fcb buffer
		if (fcbArray[fd].bufferTracker == 0)
		{
			LBAread(fcbArray[fd].buf, 1, fcbArray[fd].blockTracker);
			fcbArray[fd].blockTracker += 1;
		}

		// Read count bytes from fcb buffer into user's buffer and update the buffer tracker
		memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].bufferTracker, readCount);
		fcbArray[fd].bufferTracker += readCount; // this should always be < block size.
		fcbArray[fd].numBytesRead += readCount;

		// If bufferTracker has stepped through the whole block, set it to 0 to reset the tracker.
		if (fcbArray[fd].bufferTracker == vcb->block_size)
		{
			fcbArray[fd].bufferTracker = 0;
		}
	}

	else
	{
		// If there is data in the buffer that I can read to user's buffer first
		if (fcbArray[fd].bufferTracker != 0)
		{
			int firstRead = vcb->block_size - fcbArray[fd].bufferTracker;
			memcpy(buffer, fcbArray[fd].buf + fcbArray[fd].bufferTracker, firstRead);
			buffer += firstRead;
			fcbArray[fd].numBytesRead += firstRead;
			readCount -= firstRead;
			fcbArray[fd].bufferTracker = 0;
			fcbArray[fd].blockTracker += 1;
		}

		// Calculate how many blocks to LBAread
		int numBlocks = readCount / vcb->block_size;
		if (numBlocks > 0)
		{
			// If you can just read numBlocks whole blocks directly from file to user's buffer
			// without remaining bytes
			if (numBlocks * vcb->block_size == readCount)
			{
				LBAread(buffer, numBlocks, fcbArray[fd].blockTracker);
				fcbArray[fd].blockTracker += numBlocks;
				fcbArray[fd].numBytesRead += (numBlocks * vcb->block_size);
				// but when does eof get triggered here?
				if (fcbArray[fd].eof == 1)
				{
					printf("End of file reached.\n");
					return 0;
				}
				else
				{
					return readCount;
				}
			}
		}

		// If there's a small amount of bytes left to read
		else if (numBlocks == 0 && readCount > 0)
		{
			LBAread(fcbArray[fd].buf, 1, fcbArray[fd].blockTracker);
			fcbArray[fd].blockTracker += 1;

			memcpy(buffer, fcbArray[fd].buf, readCount);
			fcbArray[fd].bufferTracker = readCount;
			fcbArray[fd].numBytesRead += readCount;

			if (fcbArray[fd].eof == 1)
			{
				printf("End of file reached.\n");
				return readCount;
			}
			else
			{
				return readCount;
			}
		}
	}

	if (fcbArray[fd].eof == 1)
	{
		printf("End of file reached.\n");
		return 0;
	}
	else
	{
		return readCount;
	}
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
	fcbArray[i].bufferTracker = -1;
	fcbArray[i].startBlock = -1;
	fcbArray[i].numBytesRead = -1;
	fcbArray[i].eof = -1;
	fcbArray[i].fileSize = -1;

	return 0;
}
