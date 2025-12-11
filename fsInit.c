/**************************************************************
 * Class::  CSC-415-01 Summer 2024
 * Name:: Shez Rahman
 * Student IDs:: 916867424
 * GitHub-Name:: shezgo
 * Group-Name:: Independent
 * Project:: Basic File System
 *
 * File:: fsInit.c
 *
 * Description:: Main driver for file system assignment.
 *
 * This file is where you will start and initialize your system
 *
 **************************************************************/
#include "fsInit.h"

uint8_t magicNumber;
VolumeControlBlock *vcb = NULL; // Global definition, always kept in memory
DE *rootGlobal = NULL;			// Global definition, always kept in memory
DE *cwdGlobal = NULL;			// Global definition, always kept in memory
char *cwdName = NULL;			// Global char* used to track cwd path string
Bitmap *bm = NULL;				// Global declaration of the freespace bitmap
uint8_t *bitmap_bm = NULL;		// Actual bitmap array
int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize)
{
	// This error check guarantees that the vcb can fit in block 0.
	if (blockSize < sizeof(VolumeControlBlock))
	{
		fprintf(stderr, "Error: Block size (%ld) is less than sizeof(VolumeControlBlock) (%ld)\n",
				blockSize, sizeof(VolumeControlBlock));
		exit(EXIT_FAILURE);
	}

	vcb = loadVCBtoMem(blockSize);

	// ********************************************************************************************
	// If the file system has already been mounted,  read in all variables into
	// memory so they are initialized.
	// ********************************************************************************************
/*
	if (vcb->signature == 0x1A)
	{
		printf("Loading mounted file system\n");
		bitmap_bm = loadBMtoMem(blockSize);
		bm = initBitmap(numberOfBlocks, blockSize, bitmap_bm);

		printf("bm->mapNumBlocks:%d\n", bm->mapNumBlocks);
		rootGlobal = loadDirLBA(vcb->root_num_blocks, vcb->root_directory_block);
		printf("fsInit RELOAD rootGlobal.dirNumBlocks:%d\n", rootGlobal[0].dirNumBlocks);
		// Always start cwd from root when starting up file system.
		printf("\nfsInit isBitUsed(bm, 12): %d\n\n", isBitUsed(bm, 12));

		if (rootGlobal == NULL)
		{
			fprintf(stderr, "Initializing root failed\n");
			return -1;
		}

		cwdGlobal = rootGlobal;
		// Output statement to check if cwdGlobal is initializing to rootGlobal
		printf("fsInit.c line 60: cwdGlobal->name: %s\n", cwdGlobal->name);
		printf("rootGlobal->name: %s\n", rootGlobal->name);
		// Initialize a global current working directory name string

		cwdName = (char *)malloc(CWD_SIZE);

		if (cwdName == NULL)

		{
			fprintf(stderr, "cwdName memory allocation failed\n");

			return -1;
		}

		for (uint32_t i = 0; i < CWD_SIZE; i++)

		{

			cwdName[i] = '\0';
		}

		strcpy(cwdName, "/");
		// end init cwdName
		printf("fsInit.c line 75: cwdName: %s\n", cwdName);
		return 0;
	}
	// ********************************************************************************************
	// End if file system is already mounted
	// ********************************************************************************************
*/
	printf("Mounting file system\n");

	// If the volume hasn't been initialized
	// Clear all memory for vcb before initializing and writing
	for (uint32_t i = 0; i < blockSize; i++)
	{
		((char *)vcb)[i] = 0;
	}

	// Initialize Global pointer to keep track of current working directory

	cwdName = (char *)malloc(CWD_SIZE);

	if (cwdName == NULL)

	{
		fprintf(stderr, "cwdName memory allocation failed\n");

		return -1;
	}

	for (uint32_t i = 0; i < CWD_SIZE; i++)

	{
		cwdName[i] = '\0';
	}

	printf("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks,
		   blockSize);

	bm = initBitmap(numberOfBlocks, blockSize, NULL);

	vcb->block_size = blockSize;
	vcb->total_blocks = numberOfBlocks;
	vcb->free_blocks = vcb->total_blocks - vcb->fsmap_num_blocks - 2; // subtract blocks for bitmap, vcb, root
	vcb->signature = 0x1A;											  // This is an arbitrary number to check if already initialized
	vcb->fsmap_start_block = 1;
	vcb->fsmap_end_block = bm->mapNumBlocks;
	vcb->fsmap_num_blocks = bm->mapNumBlocks;
	vcb->root_directory_block = vcb->fsmap_end_block + 1;
	vcb->root_num_blocks = ROOTNUMBLOCKS;

	// Initialize and write root directory to disk. VCB root_start_block gets initialized
	// in the initDir function, so writing vcb to disk must happen after.
	printf("fsInit.c: before rootGlobal init\n");
	rootGlobal = initDir(MAX_ENTRIES, NULL, -1, NULL, bm);
	printf("fsInit.c: rootGlobal.dirNumBlocks:%d\n", rootGlobal[0].dirNumBlocks);
	// Current working directory always starts at root when starting file system.
	cwdGlobal = rootGlobal;
	strcpy(cwdName, "/"); // Having cwdName is redundant to cwdGlobal

	int vcbWriteReturn = LBAwrite(vcb, 1, 0);
	if (vcbWriteReturn == 1)
	{
		setBit(bm, 0);
	}
	else
	{
		fprintf(stderr, "fsInit.c: Failed to LBAwrite VCB\n");
	}

	printf("sizeof(DE):%ld\n", sizeof(DE));

	return 0;
}

void exitFileSystem()
{
	free(vcb);
	free(rootGlobal);
	free(cwdName);
	free(bitmap_bm);
	free(bm);
	printf("System exiting\n");
}
