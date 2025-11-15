/**************************************************************
 * Class::  CSC-415-01 Summer 2024
 * Name:: Shez Rahman
 * Student IDs:: 916867424
 * GitHub-Name:: shezgo
 * Group-Name:: Independent
 * Project:: Basic File System
 *
 * File:: directory_entry.c
 *
 * Description::
 *	This is used for directory_entry operations
 *
 *
 *
 **************************************************************/
#include "directory_entry.h"
#include "fsInit.h"

DE *initDir(int maxEntries, DE *parent, int parentIndex, char * ppile, Bitmap *bm)
{

    int BLOCKSIZE = vcb->block_size;
    int bytesNeeded = maxEntries * sizeof(DE);
    // int blocksNeeded = (bytesNeeded + BLOCKSIZE - 1) / BLOCKSIZE;
    int blocksNeeded = DIRECTORY_NUM_BLOCKS; // refactored for fixed directory size
    int bytesToAlloc = blocksNeeded * BLOCKSIZE;

    // Allocate memory for the directory
    DE *newDir = (DE *)malloc(bytesToAlloc);
    if (newDir == NULL)
    {
        fprintf(stderr, "New directory memory allocation failed\n");
        return NULL;
    }

    // Initialize all bytes to 0 using a for loop
    for (uint32_t i = 0; i < bytesToAlloc; i++)
    {
        ((char *)newDir)[i] = 0;
    }

    // Calculate number of entries that can fit inside the directory block(s)
    int actualEntries = bytesToAlloc / sizeof(DE);
    printf("From directory_entry.c->initDir:BEFORE fsAlloc:bm->fsNumBlocks:%d, blocksNeeded:%d\n", bm->fsNumBlocks, blocksNeeded);
    int newLoc = fsAlloc(bm, blocksNeeded);
    printf("From directory_entry.c->initDir: newLoc:%d bm->fsNumBlocks:%d, blocksNeeded:%d\n", newLoc, bm->fsNumBlocks, blocksNeeded);
    int entriesPerBlock = actualEntries / blocksNeeded; // Old code

    // Assign LBA locations to each directory entry
    for (int i = 0; i < actualEntries; i++)
    {
        int blockOffset = i / (BLOCKSIZE / sizeof(DE)); // Determine the block this entry belongs to
        // newDir[i].LBAlocation = newLoc + blockOffset;
        newDir[i].LBAlocation = -1; // DEs where i >=2 have starting locations of directories/files
        newDir[i].LBAindex = (i % entriesPerBlock) * sizeof(DE);
        newDir[i].size = -1;
        newDir[i].name[0] = '\0';
        newDir[i].timeCreation = (time_t)(-1);
        newDir[i].lastAccessed = (time_t)(-1);
        newDir[i].lastModified = (time_t)(-1);
        newDir[i].isDirectory = -1;
        newDir[i].dirNumBlocks = -1;
    }

    // Initialize . entry in the directory
    printf("From directory_entry.h->initDir: newLoc:%d blocksNeeded:%d\n", newLoc, blocksNeeded);
    time_t tc = time(NULL);
    printf("directory_entry tc test:%ld\n", tc);
    newDir[0].size = actualEntries * sizeof(DE);
    strcpy(newDir[0].name, ".");
    newDir[0].isDirectory = 1;
    newDir[0].timeCreation = tc;
    newDir[0].lastAccessed = tc;
    newDir[0].lastModified = tc;
    newDir[0].dirNumBlocks = blocksNeeded;
    newDir[0].LBAlocation = newLoc;

    // Initialize the .. (parent) entry in the directory. This inits root correctly as well.
    DE *dotdot = parent;

    // Root case
    if (dotdot == NULL)
    {
        dotdot = &newDir[0];
        vcb->root_directory_block = newDir[0].LBAlocation;
        vcb->root_num_blocks = blocksNeeded;
    }

    //Define the new directory's [1] index - works for both root or non-root case
    memcpy(&newDir[1], &dotdot[0], sizeof(DE));
    strcpy(newDir[1].name, "..");

    int writeReturn = LBAwrite((void *)newDir, blocksNeeded, newLoc);
    printf("initDir writeReturn:%d\n", writeReturn);
    if (writeReturn != blocksNeeded)
    {
        perror("Failed to initDir \n");
        exit(EXIT_FAILURE);
    }

    printf("directory_entry.c debug 1\n");

//* PICKUP DEBUG: This block is not executing at all! Has to be where the issues is.
    printf("parent pointer value: %p\n", (void *)parent);
    printf("parentIndex:%d\n",parentIndex);
printf("ppile pointer value: %p\n", (void *)ppile);

    if (parent != NULL && parentIndex >= 0 && ppile != NULL)
    {
    printf("directory_entry.c parent.name:%s\nparentIndex:%d\nppile:%s\n", 
     parent[0].name, parentIndex, ppile);
        printf("directory_entry.c debug 1.5\n");
        memcpy(&parent[parentIndex], &newDir[0], sizeof(DE));
        strcpy(parent[parentIndex].name, ppile);
        printf("directory_entry.c parent[parentIndex].name:%s\n", parent[parentIndex].name);
    }
    printf("directory_entry.c debug 2\n");

    if (parent != NULL)
    {
        printf("directory_entry.c debug 3\n");
        int writeReturn2 = LBAwrite((void *)parent, parent[0].dirNumBlocks, parent[0].LBAlocation);
        printf("initDir: parent writeReturn2:%d\n", writeReturn2);
        if (writeReturn2 != parent[0].dirNumBlocks)
        {
            perror("Failed to initDir (parent) \n");
            exit(EXIT_FAILURE);
        }
    }
    printf("directory_entry.c debug 4\n");
    return newDir;
}

// Loads a directory into memory for manipulation. Currently loads all bytes in the shared LBA
// in addition to the desired directory.
// DEBUG: ensure root case is being handled
DE *loadDirDE(DE *dir)
{
    if (dir == NULL)
    {
        fprintf(stderr, "Cannot load NULL dir\n");
        return NULL;
    }
    if (dir->isDirectory == 0)
    {
        fprintf(stderr, "loadDir: DE is not a directory.\n");
        return NULL;
    }
    // DEBUG no name for root
    if (strcmp(dir->name, rootGlobal->name) == 0)
    {
        return rootGlobal;
    }

    // New code starts here
    // DEBUG storing dirNumBlocks is redundant
    void *buffer = malloc(dir->dirNumBlocks * vcb->block_size);

    if (buffer == NULL)
    {
        perror("Failed to allocate for buffer in loadDir\n");
        exit(EXIT_FAILURE);
    }
    // this is correct
    int readReturn = LBAread(buffer, dir->dirNumBlocks, dir->LBAlocation);

    if (readReturn != dir->dirNumBlocks)
    {
        perror("Failed to loadDirDE\n");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    return (DE *)buffer;
}

DE *loadDirLBA(int numBlocks, int startBlock)
{

    // New code starts here
    void *buffer = malloc(numBlocks * vcb->block_size);

    if (buffer == NULL)
    {
        perror("Failed to allocate for buffer in loadDir\n");
        exit(EXIT_FAILURE);
    }
    int readReturn = LBAread(buffer, numBlocks, startBlock);

    if (readReturn != numBlocks)
    {
        perror("Failed to loadDirLBA\n");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    return (DE *)buffer;
}

// Update a directory on disk. Returns 1 on success, -1 if failed.
// This should ONLY update a DE on disk from an entire loaded directory in memory
// PICKUP/DEBUG: Can I ensure this^ with reading into a buffer and freeing within this method?
//

void *print5DEs(DE *dir)
{
    for (int i = 0; i < 5; i++)
    {
        printf("name: %s\n"
               "size: %ld\n"
               "LBAlocation: %ld\n"
               "dirNumBlocks: %d\n",
               dir[i].name,
               dir[i].size,
               dir[i].LBAlocation,
               dir[i].dirNumBlocks);
    }
    printf("End print\n");
    return NULL;
}

int updateDELBA(DE *dir)
{
    /*
        To update a directory, rewrite it to its LBAlocation.
        Update lastAccessed and lastModified in directory[0]
        AND in its parent directory at correct index.

    */

    time_t tc = time(NULL);
    dir[0].lastAccessed = tc;
    dir[0].lastModified = tc;

    // Save the contents of the passed directory
    int ret = LBAwrite((void *)dir, dir[0].dirNumBlocks, dir[0].LBAlocation);
    if (ret != dir[0].dirNumBlocks)
    {
        perror("Failed to updateDELBA: LBAwrite \n");
        exit(EXIT_FAILURE);
    }
    print5DEs(dir);

    // Next, load, edit, and save this directory's metadata in its parent directory.
    DE *parent = loadDirLBA(dir[1].dirNumBlocks, dir[1].LBAlocation);
    print5DEs(parent);
    printf("updateDELBA after loading parent\n");
    int index = findNameInDir(parent, dir[0].name);
    if (index == -1)
    {
        fprintf(stderr, "File name not found in parent\n");
        return -1;
    }

    parent[index].lastAccessed = tc;
    parent[index].lastModified = tc;
    parent[index].LBAlocation = dir[0].LBAlocation;
    parent[index].LBAindex = dir[0].LBAindex;
    parent[index].size = dir[0].size;
    strcpy(parent[index].name, dir[0].name);
    parent[index].isDirectory = dir[0].isDirectory;
    parent[index].timeCreation = dir[0].timeCreation;
    parent[index].dirNumBlocks = dir[0].dirNumBlocks;

    // Save the contents of the parent of the passed directory
    ret = LBAwrite((void *)parent, parent[0].dirNumBlocks, parent[0].LBAlocation);
    if (ret != parent[0].dirNumBlocks)
    {
        perror("Failed to updateDELBA \n");
        free(parent);
        exit(EXIT_FAILURE);
    }

    free(parent);
    return ret;
}
