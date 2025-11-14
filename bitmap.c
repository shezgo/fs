/**************************************************************
 * Class::  CSC-415-01 Summer 2024
 * Name:: Shez Rahman
 * Student IDs:: 916867424
 * GitHub-Name:: shezgo
 * Group-Name:: Independent
 * Project:: Basic File System
*
* File:: bitmap.c
*
* Description:: 
*	This is used to interact with the free space bitmap
*	
*	
*
**************************************************************/
#ifndef BITMAP_C
#define BITMAP_C

#include "bitmap.h"

// Function to LBAwrite the freespace bitmap
int mapToDisk(Bitmap *bm)
{
    int ret = LBAwrite(bm->bitmap, bm->mapNumBlocks, 1);
    return ret;
}
// Function to set a bit (mark block as used). Returns 1 if success, -1 if failure.
int setBit(Bitmap *bm, int blockNumber)
{
    if (blockNumber < bm->fsNumBlocks)
    {
        int byteIndex = blockNumber / 8;
        int bitIndex = blockNumber % 8;
        bm->bitmap[byteIndex] |= (1 << bitIndex);
        mapToDisk(bm);

        vcb->free_blocks--;
        writeVCBtoDisk(vcb);

        return 1;
    }
    else
    {
        fprintf(stderr, "Invalid blockNumber\n");
        return -1;
    }
}
// Function to clear a bit (mark block as free). Returns 1 if success, -1 if failure.
int clearBit(Bitmap *bm, int blockNumber)
{
    if (blockNumber < bm->fsNumBlocks)
    {
        int byteIndex = blockNumber / 8;
        int bitIndex = blockNumber % 8;
        bm->bitmap[byteIndex] &= ~(1 << bitIndex);
        mapToDisk(bm);

        vcb->free_blocks++;
        writeVCBtoDisk(vcb);

        return 1;
    }
    else
    {
        fprintf(stderr, "Invalid blockNumber\n");
        return -1;
    }
}
// Function to check if a block is 1 (used). Returns 1 if block (bit) is being used.
int isBitUsed(Bitmap *bm, int blockNumber)
{
    if (blockNumber < bm->fsNumBlocks)
    {
        int byteIndex = blockNumber / 8;
        int bitIndex = blockNumber % 8;
        return (bm->bitmap[byteIndex] & (1 << bitIndex)) != 0;
    }
    else
    {
        fprintf(stderr, "Invalid blockNumber\n");
        return -1;
    }
}

// Depending on requested number of blocks from user, return a starting block index to be used.
int fsAlloc(Bitmap *bm, int req)
{
    // Check if req is a valid value
    if (req <= 0 || req > bm->fsNumBlocks)
    {
        fprintf(stderr, "Invalid request size\n");
        return -1;
    }

    int consecutiveFreeBlocks = 0;
    int startBlock = -1;

    // Iterate through each bit in the bitmap
    for (int byteIndex = 0; byteIndex < bm->bitmapSize; byteIndex++)
    {
        // If the byte is not all 1s, there's at least one free bit in it
        if (bm->bitmap[byteIndex] != 0xFF)
        {
            for (int bitIndex = 0; bitIndex < 8; bitIndex++)
            {
                int blockNumber = byteIndex * 8 + bitIndex;

                // Check if the current block is free
                if ((bm->bitmap[byteIndex] & (1 << bitIndex)) == 0)
                {
                    // If this is the first free block in the current sequence, start tracking
                    if (consecutiveFreeBlocks == 0)
                    {
                        startBlock = blockNumber;
                    }
                    consecutiveFreeBlocks++;

                    // If we found a successful sequence
                    if (consecutiveFreeBlocks == req)
                    {
                        for (int i = startBlock; i < startBlock + req; i++)
                        {
                            setBit(bm, i); // mark the blocks as used for the space requester
                        }

                        vcb->free_blocks -= req;
                        writeVCBtoDisk(vcb);

                        return startBlock;
                    }
                }
                else
                {
                    // Reset if an occupied block is found before req num of blocks are found
                    consecutiveFreeBlocks = 0;
                    startBlock = -1;
                }

                // If the current block number exceeds the number of file system blocks
                if (blockNumber >= bm->fsNumBlocks)
                {
                    return -1;
                }
            }
        }
        else
        {
            // Reset if the byte is all 1s
            consecutiveFreeBlocks = 0;
            startBlock = -1;
        }
    }

    // If the entire bitmap has been traversed and no contiguous blocks were found
    return -1;
}

// Set the corresponding bits in the bitmap to being free. Return 1 if successful, -1 if failed
int fsRelease(Bitmap *bm, int startBlock, int count)
{
    /*
    Make sure params are valid
    Set the bits in the bm to being free. Anything else?
    Return
    */
    if (bm == NULL)
    {
        fprintf(stderr, "fsRelease: Bitmap is null\n");
        return -1;
    }
    if (startBlock < 0 || (startBlock + count) > bm->fsNumBlocks)
    {
        fprintf(stderr, "fsRelease: Invalid blocks");
        return -1;
    }
    if (count < 1)
    {
        fprintf(stderr, "fsRelease: Invalid count\n");
        return -1;
    }

    int clearRet;

    for (int i = startBlock; i < startBlock + count; i++)
    {
        clearRet = clearBit(bm, i);
        if (clearRet == -1)
        {
            fprintf(stderr, "fsRelease: clearBit failed\n");
            return -1;
        }
    }

    vcb->free_blocks += count;
    writeVCBtoDisk(vcb);

    return 1;
}

Bitmap *initBitmap(int fsNumBlocks, int blockSize, uint8_t *bm_bitmap)
{

    // Allocate and initialize the memory for the Bitmap struct
    Bitmap *bm = (Bitmap *)malloc(sizeof(Bitmap));
    if (bm == NULL)
    {
        fprintf(stderr, "Failed to allocate memory for Bitmap structure\n");
        exit(EXIT_FAILURE);
    }
    for (uint32_t i = 0; i < sizeof(Bitmap); i++)
    {
        ((char *)bm)[i] = 0;
    }
 
    // Allocate the size for bitmap - Convert the num bytes to num bits for bitmap malloc
    int blocksToBitsInBytes = (fsNumBlocks + 7) / 8;

    bm->fsNumBlocks = fsNumBlocks;
    // Below operation works because of int division.
    int roundedBytes = ((blocksToBitsInBytes + blockSize - 1) / blockSize) * blockSize;

    //IF file system has not been initialized yet, this function will receive null as a parameter.
    if (bm_bitmap == NULL)
    {

        bm->bitmap = (uint8_t *)malloc(roundedBytes);
        if (bm->bitmap == NULL)
        {
            fprintf(stderr, "Failed to allocate memory for bitmap\n");
            free(bm);
            exit(EXIT_FAILURE);
        }


        // Initialize all bits in the bitmap to 0 (free)
        for (int i = 0; i < roundedBytes; i++)
        {
            bm->bitmap[i] = 0;
        }
 
        /*Set the bits that we know are going to be occupied. This includes the VCB in logical b0,
        but physical block 0 contains Professor's partition table. Also set blocks needed
        to store the free space map.
        Per "steps for milestone 1 pdf" do not free the bitmap buffer. Keep it in memory and
        LBAwrite whenever you manipulate the buffer. */

        bm->mapNumBlocks = (blocksToBitsInBytes + blockSize - 1) / blockSize;
        bm->bitmapSize = bm->mapNumBlocks * blockSize;

        for (int i = 1; i <= 1 + (bm->mapNumBlocks - 1); i++)
        {
            setBit(bm, i);
        }

    }

    else
    {
        bm->bitmap = bm_bitmap;
        bm->mapNumBlocks = (blocksToBitsInBytes + blockSize - 1) / blockSize;
        bm->bitmapSize = bm->mapNumBlocks * blockSize;
    }

    //  LBAwrite the bitmap at correct locations
    mapToDisk(bm);

    return bm;

}

uint8_t *loadBMtoMem(int blockSize)
{
    printf("from loadBmtoMem: vcb->fsmap_num_blocks:%d\n", vcb->fsmap_num_blocks);
    uint8_t *buffer = malloc(blockSize * vcb->fsmap_num_blocks);
    if (buffer == NULL)
    {
        perror("Failed to load bitmap to memory\n");
        exit(EXIT_FAILURE);
    }

    int bmLoadReturn = LBAread(buffer, vcb->fsmap_num_blocks, vcb->fsmap_start_block);

    return buffer;
}

#endif