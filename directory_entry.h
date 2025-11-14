/**************************************************************
 * Class::  CSC-415-01 Summer 2024
 * Name:: Shez Rahman
 * Student IDs:: 916867424
 * GitHub-Name:: shezgo
 * Group-Name:: Independent
 * Project:: Basic File System
 *
 * File:: directory_entry.h
 *
 * Description:: Holds directory entry struct and operations.
 *
 *
 *
 **************************************************************/

#ifndef _DIRECTORY_ENTRY_H
#define _DIRECTORY_ENTRY_H

#include "volume_control_block.h"
#include "bitmap.h"
#include <time.h>
#include <stdint.h>
#include <string.h>

#define NAME 31
#define DIRECTORY_NUM_BLOCKS 5
#define FILE_NUM_BLOCKS 10

extern VolumeControlBlock *vcb; // Global definition, always kept in memory 
extern Bitmap *bm;      //Global declaration of the freespace bitmap

//To find the logical address of a DE within its parent directory, just use
//index in its parent directory * sizeof(DE).
typedef struct DE
{
    long size;           // size of the file in bytes, or actualEntries * sizeof(DE) if directory
    long LBAlocation;    // location where the directory entry is stored
    long LBAindex;       // index within LBAlocation
    char name[NAME + 1]; // Name of the directory entry
    time_t timeCreation; // the time the DE was created
    time_t lastAccessed; // the time the DE was last accessed
    time_t lastModified; // the time the DE was last modified

    int16_t isDirectory; // checks if it is a directory. 1 if directory, 0 if not.
    int32_t dirNumBlocks; // Number of blocks if it's a directory. If not dir or null, -1.


} DE;

// Since this is a pointer, we'll want this loaded into RAM. For root dir, parent points to null.
// so just pass in null and set parentIndex to -1.
// initDir creates the directory and assigns it as a DE to its parent directory
DE *initDir(int maxEntries, DE *parent, int parentIndex, char * ppile, Bitmap *bm);
DE *loadDirDE(DE *dir); //loads a directory into memory
DE *loadDirLBA(int numBlocks, int startBlock); //Creates and loads a dir into memory using LBA
int updateDELBA(DE *dir); // Updates a DE on disk
void *print5DEs(DE *dir); //Prints first 5 DEs from a directory
#endif