/**************************************************************
 * Class::  CSC-415-01 Summer 2024
 * Name:: Shez Rahman
 * Student IDs:: 916867424
 * GitHub-Name:: shezgo
 * Group-Name:: Independent
 * Project:: Basic File System
*
* File:: mfs.h
*
* Description:: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/


#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "b_io.h"

#include <dirent.h>
#include "directory_entry.h"
#include "fsInit.h"
#include <sys/stat.h>

#define FT_REGFILE	DT_REG
#define FT_DIRECTORY DT_DIR
#define FT_LINK	DT_LNK
#define MAX_ELEMENTS 30

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif

extern Bitmap *bm;      //Global declaration of the freespace bitmap
extern DE *rootGlobal; // Global declaration of the root directory
extern DE *cwdGlobal;  // Global declaration of the current working directory
extern char *cwdName; //Global array used to track cwd path string
extern VolumeControlBlock *vcb; // Global definition, always kept in memory 

// This structure is returned by fs_readdir to provide the caller with information
// about each file as it iterates through a directory
struct fs_diriteminfo
	{
    unsigned short d_reclen;    /* length of this record */
    unsigned char fileType;    
    char d_name[256]; 			/* filename max filename is 255 characters */
	};

// This is a private structure used only by fs_opendir, fs_readdir, and fs_closedir
// Think of this like a file descriptor but for a directory - one can only read
// from a directory.  This structure helps you (the file system) keep track of
// which directory entry you are currently processing so that everytime the caller
// calls the function readdir, you give the next entry in the directory
typedef struct
	{
	/*****TO DO:  Fill in this structure with what your open/read directory needs  *****/
	unsigned short  d_reclen;		/* length of this record */
	unsigned short	dirEntryPosition;	/* which directory entry position, like file pos */
	DE *directory;			/* Pointer to the loaded directory you want to iterate */
	struct fs_diriteminfo * di;		/* Pointer to the structure you return from read */
	unsigned short numEntries; //Number of entries in directory
	} fdDir;
/*
ppInfo is used as a return structure for parsePath info
Return: 
1. (int)Success or error - check if each index before le is a valid directory
^This is the true return value for parsePath. The rest will be in struct ppinfo.
2.  DE * parentPointer to parent loaded in memory
3. char * lastElement is the name of the last element in the path
4. int lastElementIndex - which entry is it inside the parent? -1 if not exist

*/
typedef struct ppinfo
{
	DE * parent; //Pointer to a parent directory array of DEs for the caller of parsePath
	char le[NAME+1]; //last element for parsePath
	int lei; //last element index for parsePath - used to access the actual DE as parent[lei]
	int isFile; //Is 1 if parent[lei] is a file.
	int parentExists; //Is 1 in case lei is -1, but ppi still contains the parent of the le
	char pathArray[MAX_ELEMENTS][NAME+1]; //Full path string with . or .. resolved by parsePath
	int maxElemIndex; // Used to count the elements in pathArray ie if maxElemIndex is 0, there's 1
}ppinfo;

// Key directory functions
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);

// Directory iteration functions
fdDir * fs_opendir(const char *pathname);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

// Misc directory functions
char * fs_getcwd(char *pathname, size_t size);
int fs_setcwd(char *pathname);   //linux chdir
int fs_isFile(char * filename);	//return 1 if file, 0 otherwise
int fs_isDir(char * pathname);		//return 1 if directory, 0 otherwise
int fs_delete(char* filename);	//removes a file

// Helper functions
int parsePath(char * passedPath, ppinfo * ppi); 
int findNameInDir(DE *parent, char* name); //returns index of DE in parent if found, -1 if not
int entryIsDir(DE *parent, int deIndex); // Checks if the DE in parent is a directory. 1 if true
int freeIfNotNeedDir(DE *dir); //Frees a dir only if not cwd, root, or null 
int findUnusedDE(DE *parent); //Find the first unused DE in a parent
int saveDir(DE *directory); // Writes an existing directory to disk
int isNullTerminated (char *str, size_t len);//1 if success, 0 if fail


// This is the strucutre that is filled in from a call to fs_stat
struct fs_stat
	{
	off_t     st_size;    		/* total size, in bytes */
	blksize_t st_blksize; 		/* blocksize for file system I/O */
	blkcnt_t  st_blocks;  		/* number of 512B blocks allocated */
	time_t    st_accesstime;   	/* time of last access */
	time_t    st_modtime;   	/* time of last modification */
	time_t    st_createtime;   	/* time of last status change */
	
	/* add additional attributes here for your file system */
	};

//Returns 0 if success, -1 if failure
int fs_stat(const char *path, struct fs_stat *buf);

#endif

