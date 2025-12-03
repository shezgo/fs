/**************************************************************
 * Class::  CSC-415-01 Summer 2024
 * Name:: Shez Rahman
 * Student IDs:: 916867424
 * GitHub-Name:: shezgo
 * Group-Name:: Independent
 * Project:: Basic File System
 *
 * File:: mfs.c
 *
 * Description::
 *	This is the file system interface.
 *	This is the interface needed by the driver to interact with
 *	your filesystem.
 *
 **************************************************************/
#include "mfs.h"
#include "fsInit.h"
#include "fsLow.h"

//*************************************************************************************************
// Helper functions
//*************************************************************************************************
// Returns index of the DE with name parameter in parent. Returns index on success, -1 on failure.
int findNameInDir(DE *parent, char *name)
{
    if (parent == NULL)
    {
        fprintf(stderr, "Parent is null\n");
        return -1;
    }
    if (name == NULL)
    {
        fprintf(stderr, "File name is null\n");
        return -1;
    }
    int numEntries = parent[0].size / sizeof(DE);

    printf("findNameInDir: sizeof(DE):%ld\n", sizeof(DE));
    printf("findNameInDir: parent[0].size:%ld\n", parent[0].size);
    printf("findNameInDir: numEntries:%d\n", numEntries);

    for (int i = 0; i < numEntries; i++)
    {
        printf("findNameInDir: parent[%d].name:%s\n", i, parent[i].name);
        if (strcmp(parent[i].name, name) == 0)
        {
            return i;
        }
    }
    return -1;
}

//*************************************************************************************************
// Checks if the DE in parent is a directory. 1 if true, 0 if false.
int entryIsDir(DE *parent, int deIndex)
{
    if (parent == NULL)
    {
        fprintf(stderr, "Parent is null\n");
        return 0;
    }

    if (parent[deIndex].isDirectory == 1)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//*************************************************************************************************
// Checks if the DE in parent is a file. 1 if true, 0 if false.
int entryIsFile(DE *parent, int deIndex)
{
    if (parent == NULL)
    {
        fprintf(stderr, "Parent is null\n");
        return 0;
    }

    if (parent[deIndex].isDirectory == 0 && parent[deIndex].name[0] != '\0')
    {
        printf("Entry is a file.\n");
        return 1;
    }
    else
    {
        return 0;
    }
}

//*************************************************************************************************
// Frees a dir only if not cwd, root, or null.
// Return 1 indicates freeing the dir, 0 indicates no memory freed.
int freeIfNotNeedDir(DE *dir)
{
    if (dir != NULL)
    {
        if (dir[0].LBAlocation != cwdGlobal[0].LBAlocation)
        {
            if (dir[0].LBAlocation != rootGlobal[0].LBAlocation)
            {
                free(dir);
                return 1;
            }
        }
    }
    else
    {
        return 0;
    }
}

//*************************************************************************************************
// Find first unused DE in a parent DE. Returns -1 if failed, returns index of DE if success.
int findUnusedDE(DE *parent)
{
    if (parent == NULL)
    {
        fprintf(stderr, "Parent is null\n");
        return 0;
    }

    int numDEs = parent->size / sizeof(DE);
    printf("from findUnusedDE - parent->size:%ld sizeof(DE):%ld\n", parent->size, sizeof(DE));
    for (int i = 2; i < numDEs; i++)
    {
        if (parent[i].name[0] == '\0')
        {
            printf("findUnusedDE index:%d\n", i);
            return i;
        }
    }

    return -1;
}

//*************************************************************************************************
// Write an existing directory to disk; return -1 if failed, 1 if success
int saveDir(DE *directory)
{
    if (directory == NULL)
    {
        fprintf(stderr, "Directory is null\n");
        return -1;
    }
    
    int returnInt = LBAwrite(directory, directory[0].dirNumBlocks, directory[0].LBAlocation);
    if(returnInt == directory[0].dirNumBlocks)
    {
    return 1;
    }
    else
    {
        fprintf(stderr, "saveDir: LBAwrite failure.\n");
        return -1;
    }
}

//*************************************************************************************************
/*
parsePath loads the parent in a path and finds if the file (last element) exists or not.
When using this, use parsePath(char * path, &ppi)
Return values of this function:
1. (int) 0 for Success, -1 for error - check if each index before le is a valid directory
Returns -2 for root
^This is the true return value for parsePath. The rest will be in struct ppinfo.
2.  DE * parentPointer to parent loaded in memory
3. char * lastElement is the name of the last element in the path
4. int lastElementIndex - which entry is it inside the parent? -1 if does not exist
*/
int parsePath(char *passedPath, ppinfo *ppi)
{
    if (passedPath == NULL)
    {
        fprintf(stderr, "passedPath is null\n");
        return -1;
    }
    char *path = malloc(CWD_SIZE);
    if (path == NULL)
    {
        fprintf(stderr, "path is null\n");
        return -1;
    }
    strcpy(path, passedPath); // duplicates string, allocates memory

    if (path == NULL)
    {
        perror("strcpy failed");
        return -1;
    }

    DE *start;

    if (path[0] == '/')
    {
        if (rootGlobal != NULL)
        {
            start = rootGlobal;
        }
        else
        {
            fprintf(stderr, "Root is null\n");
        }
    }
    else
    {
        if (cwdGlobal != NULL)
        {
            start = cwdGlobal;
        }
    }

    DE *parent = start;
    ppi->isFile = 0;

    char *saveptr;
    char *token1 = strtok_r(path, "/", &saveptr);
    // Special case: If the only token is /, then it’ll return null
    if (token1 == NULL)
    {
        if (path[0] != '/') // ensure the path is not root
        {
            return -1; // Invalid path
        }
        ppi->parent = parent;
        ppi->le = NULL;
        ppi->lei = 0; // this means path is root
        return -2;    // Unique return val for root path
    }
    char *token2;

    do
    {
        ppi->le = token1;

        // if findNameInDir can't find token1, it returns -1 to ppi->lei.
        ppi->lei = findNameInDir(parent, token1);
        token2 = strtok_r(NULL, "/", &saveptr);
// Success: If token2 is null then token1 is the last element.
        // If token2 is not null, that tells you token1 has to exist and must be a directory.
        if (token2 == NULL)
        {
            ppi->parent = parent;

            if (entryIsFile(parent, ppi->lei) == 1)
            {
                fprintf(stderr, "mfs.c:parsePath: parent[ppi->lei] is a file.\n");
                ppi->isFile = 1;
                return -1;
            }
            if(ppi->lei  == -1)
            {
                return -1;
            }
            return (0);
        }

        // This triggers if at any point in the path, an element doesn't exist
        if (ppi->lei < 0) // the name doesn’t exist, invalid path
        {
            fprintf(stderr, "Invalid path\n");
            return -1;
        }

        // Helper function EntryisDir - if parent[ppi->lei] is NOT a directory, error?
        if (entryIsDir(parent, ppi->lei) == 0 && entryIsFile(parent, ppi->lei) != 1)
        {
            fprintf(stderr, "mfs.c:parsePath: parent[ppi->lei] is not a directory. \n");
            return -1;
        }

        if (entryIsFile(parent, ppi->lei) == 1)
        {
            fprintf(stderr, "mfs.c:parsePath: parent[ppi->lei] is a file.\n");
            ppi->isFile = 1;
            return -1;
        }
        // Now we know token 1 does exist, is valid, and is a directory. So we want to load it/get
        // that dir parsePath
        if (ppi->lei >= (parent->size / sizeof(DE)))
        {
            fprintf(stderr, "ppi->lei is out of bounds");
            return -1;
        }

        DE *temp = loadDirLBA(parent[ppi->lei].dirNumBlocks, parent[ppi->lei].LBAlocation);

        // Helper function freeIfNotNeedDir(parent)
        freeIfNotNeedDir(parent); // not null, not cwd, not root
        parent = temp;
        token1 = token2;

    } while (token2 != NULL);
    // if the index is invalid, exit
    // If the index was valid but not a directory, exit.
    // If it was, then valid!
}

//*************************************************************************************************
// End helper functions
//*************************************************************************************************

// Make a directory; return -1 if fails, 2 if directory already exists, 0 if success.
int fs_mkdir(const char *path, mode_t mode)
{
    if (path == NULL)
    {
        fprintf(stderr, "Path is null\n");
        return -1;
    }

    // Create a writable copy of the path
    char *pathCopy = strdup(path);
    if (pathCopy == NULL)
    {
        fprintf(stderr, "Failed to duplicate path\n");
        return -1;
    }
    ppinfo ppi;
    int parseFlag = parsePath(pathCopy, &ppi);
    // free(pathCopy); //this statement alters ppi.le for some reason
    //  If parsePath fails
    if (parseFlag != 0)
    {
        fprintf(stderr, "parsePath failed\n");
        return (parseFlag);
    }

    // If ppi.lei is not -1, then the directory already exists. Return 2.
    if (ppi.lei != -1)
    {
        fprintf(stderr, "Directory already exists\n");
        return (2);
    }

    int x = findUnusedDE(ppi.parent);

    if (x == -1)
    {
        fprintf(stderr, "No unused DE in parent");
        return -1;
    }

    DE *newDir = initDir(MAX_ENTRIES, ppi.parent, x, ppi.le, bm);

    if (newDir == NULL)
    {
        fprintf(stderr, "Unable to create newDir\n");
        return -1;
    }

    /*
        printf("ppi.parent time creation:%ld\n", ppi.parent->timeCreation);
        memcpy(&(ppi.parent[x]), newDir, sizeof(DE)); // this is supposed to set newDir to ppi.parent[x].
        // then ppi.le is supposed to be the name...but ppi.le might not be correct.
        printf("from fs_mkdir ppi.le:%s\n", ppi.le);
        strncpy(ppi.parent[x].name, ppi.le, sizeof(ppi.parent[x].name) - 1);
        ppi.parent[x].name[sizeof(ppi.parent[x].name) - 1] = '\0';
        printf("from fs_mkdir ppi.parent[x].name:%s\n", ppi.parent[x].name);


        int uDRet = updateDELBA(newDir);
        */
    freeIfNotNeedDir(newDir);

    return 0;
}

//*************************************************************************************************
// Open a directory. Returns NULL if fails. This should also load a directory into memory.
fdDir *fs_opendir(const char *pathname)
{
    if (pathname == NULL)
    {
        fprintf(stderr, "Path is null\n");
        return NULL;
    }
    // Create a writable copy of the path
    char *pathCopy = strdup(pathname);
    if (pathCopy == NULL)
    {
        fprintf(stderr, "Failed to duplicate path\n");
        return NULL;
    }
    ppinfo ppi;
    int parseFlag = parsePath(pathCopy, &ppi);
    // free(pathCopy);

    // If the directory wasn't found, return failure.
    if (ppi.lei == -1)
    {
        fprintf(stderr, "Directory does not exist\n");
        return NULL;
    }

    DE *thisDir = loadDirLBA((ppi.parent[ppi.lei]).dirNumBlocks, (ppi.parent[ppi.lei]).LBAlocation);

    if (thisDir == NULL)
    {
        fprintf(stderr, "File is not a directory\n");
        return NULL;
    }

    // x counts the number of DEs in thisDir
    int cntEntries = thisDir->size / sizeof(DE);
    /*int x = 0;

    while ((thisDir[x].name[0] == '\0') && x < cntEntries)
    {
        ++x;
    }
    if (x < cntEntries)
    {
        */
    fdDir *fdDirIP = malloc(sizeof(fdDir));

    if (fdDirIP == NULL)
    {
        fprintf(stderr, "fdDir malloc failed");
        freeIfNotNeedDir(thisDir);
        return NULL;
    }

    for (uint32_t i = 0; i < sizeof(fdDir); i++)
    {
        ((char *)fdDirIP)[i] = 0;
    }

    fdDirIP->di = malloc(sizeof(struct fs_diriteminfo));
    if (fdDirIP->di == NULL)
    {
        fprintf(stderr, "fdDirIP->di failed");
        free(fdDirIP);
        freeIfNotNeedDir(thisDir);
        return NULL;
    }
    for (uint32_t i = 0; i < sizeof(struct fs_diriteminfo); i++)
    {
        ((char *)fdDirIP->di)[i] = 0;
    }
    // DEBUG open doesn't actually return a name yet
    fdDirIP->di->d_reclen = sizeof(struct fs_diriteminfo);
    fdDirIP->di->fileType = thisDir->isDirectory == 1 ? FT_DIRECTORY : FT_REGFILE;
    strncpy(fdDirIP->di->d_name, (ppi.parent[ppi.lei]).name, 255);
    fdDirIP->di->d_name[strlen(thisDir->name)] = '\0';
    fdDirIP->directory = thisDir;
    fdDirIP->numEntries = cntEntries;
    fdDirIP->dirEntryPosition = 0;

    return fdDirIP;
    /*}
    else
    {
        return NULL;
    }
    */
}
//*************************************************************************************************
struct fs_diriteminfo *fs_readdir(fdDir *dirp)
{
    if (dirp == NULL)
    {
        fprintf(stderr, "Directory is invalid");
        return NULL;
    }

    if (dirp->dirEntryPosition >= dirp->numEntries)
    {
        return NULL;
    }

    // Skip past any null DEs in the directory
    while ((dirp->directory[dirp->dirEntryPosition]).name[0] == '\0' &&
           dirp->dirEntryPosition < dirp->numEntries)
    {
        dirp->dirEntryPosition++;
    }

    if (dirp->dirEntryPosition >= dirp->numEntries)
    {
        return NULL;
    }

    DE *newDE = &(dirp->directory[dirp->dirEntryPosition]);
    // printf("from fs_readdir newDE->name:%s\n", newDE->name);
    strncpy(dirp->di->d_name, newDE->name, sizeof(newDE->name));
    // printf("from fs_readdir dirp->di->d_name:%s\n", dirp->di->d_name);
    dirp->di->fileType = newDE->isDirectory == 1 ? FT_DIRECTORY : FT_REGFILE;
    dirp->dirEntryPosition++;

    return dirp->di;
}
//*************************************************************************************************
// closedir frees the resources from opendir
int fs_closedir(fdDir *dirp)
{
    if (dirp == NULL)
    {
        fprintf(stderr, "Directory doesn't exist");
        return 0;
    }
    freeIfNotNeedDir(dirp->directory);
    free(dirp->di);
    free(dirp);
    return 1;
}
//*************************************************************************************************
// Return 0 if success, -1 if fail
int fs_setcwd(char *pathname)
{
    ppinfo ppi;

    if(strcmp(pathname, "..") == 0)
    {
        DE * newCwd = loadDirLBA(cwdGlobal[1].dirNumBlocks, cwdGlobal[1].LBAlocation);
        if(cwdGlobal[0].LBAlocation != rootGlobal[0].LBAlocation)
        {
            free(cwdGlobal);
        }
        cwdGlobal = newCwd;
        if(cwdGlobal[0].LBAlocation == rootGlobal[0].LBAlocation)
        {
            strcpy(cwdName, "/");
            return 0;
        }
        strcpy(cwdName, cwdGlobal[0].name);
        return 0;
    }

    if(strcmp(pathname, ".")== 0)
    {
        return 0;
    }


    int parseFlag = parsePath(pathname, &ppi);

    // If parsePath fails, return error
    if (parseFlag == -1)
    {
        fprintf(stderr, "Invalid pathname\n");
        return (parseFlag);
    }
    // If parsePath resolves to root
    if (ppi.lei == -2)
    {
        cwdGlobal = rootGlobal;
        strcpy(cwdName, "/");
        return 0;
    }

    if (fs_isFile(pathname) == 1)
    {
        fprintf(stderr, "Path is not a directory\n");
        return -1;
    }

    // If the path is valid, update the current working directory.
    if (fs_isDir(pathname) == 1)
    {
        if(cwdGlobal[0].LBAlocation != rootGlobal[0].LBAlocation)
        {
            free(cwdGlobal);
        }
        
        cwdGlobal = loadDirLBA(ppi.parent[ppi.lei].dirNumBlocks, ppi.parent[ppi.lei].LBAlocation);

        if (pathname[0] == '/')
        {
            strcpy(cwdName, pathname);
            return 0;
        }

        char *fullPath = malloc(CWD_SIZE);
        if (fullPath == NULL)
        {
            fprintf(stderr, "fullPath malloc failed.\n");
            return -1;
        }

        strcpy(fullPath, cwdName);
        if(strcmp(fullPath, "/") != 0)
        {
            strcat(fullPath, "/");
        }
        strcat(fullPath, pathname);
        strcpy(cwdName, fullPath);

        free(fullPath);
        return 0;
    }
}
//*************************************************************************************************
// Returns 1 if success, 0 if fail
int isNullTerminated(char *str, size_t len)
{
    if (str == NULL)
        return 0; // Check for null pointer

    for (int i = 0; i < len; ++i)
    {
        if (str[i] == '\0')
        {
            return 1;
        }
    }
    return 0;
}

//*************************************************************************************************
// Copies the cwd into the user's buffer pathname. Return the pathname if success, NULL if error.
char *fs_getcwd(char *pathname, size_t size)
{
    if (pathname == NULL)
    {
        fprintf(stderr, "Null pathname");
        return NULL;
    }

    if (size < CWD_SIZE)
    {
        fprintf(stderr, "Buffer size is too small");
        return NULL;
    }

    strncpy(pathname, cwdName, size);

    if (pathname == NULL)
    {
        fprintf(stderr, "fs_getcwd: Null pathname");
        return NULL;
    }

    return pathname;
}

//*************************************************************************************************
// Return 1 if file, 0 if not
int fs_isFile(char *filename)
{
    if (filename == NULL)
    {
        fprintf(stderr, "Null filename");
        return 0;
    }
    ppinfo ppi;
    parsePath(filename, &ppi);
    if (ppi.parent[ppi.lei].isDirectory == 0 && ppi.parent[ppi.lei].name[0] != '\0')
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//*************************************************************************************************
// Returns 1 if path is a directory, 0 if fail.
int fs_isDir(char *pathname)
{

    if (pathname == NULL)
    {
        fprintf(stderr, "Null pathname");
        return 0;
    }
    ppinfo ppi;
    parsePath(pathname, &ppi);
    if (ppi.parent[ppi.lei].isDirectory == 1 && ppi.parent[ppi.lei].name[0] != '\0')
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

//*************************************************************************************************
// Returns 0 if success, -1 if failure
int fs_stat(const char *path, struct fs_stat *buf)
{
    if (path == NULL)
    {
        fprintf(stderr, "Path is null\n");
        return -1;
    }

    // Create a writable copy of the path
    char *pathCopy = strdup(path);
    if (pathCopy == NULL)
    {
        fprintf(stderr, "Failed to duplicate path\n");
        return -1;
    }
    ppinfo ppi;
    int parseFlag = parsePath(pathCopy, &ppi);
    free(pathCopy);

    DE *de = &(ppi.parent[ppi.lei]);
    buf->st_size = de->size;
    buf->st_blksize = vcb->block_size;
    buf->st_blocks = vcb->total_blocks;
    buf->st_accesstime = de->lastAccessed;
    buf->st_modtime = de->lastModified;
    buf->st_createtime = de->timeCreation;
    return 0;
}

//*************************************************************************************************
// Removes a file. Returns 0 if success, -1 if failure.
int fs_delete(char *filename)
{
    /*
    Searches for the filename in cwd
    Check if filename exists in curdir.
    Check if this filename is a file or directory.
    If file:
    1. clear the bytes (set to 0) from curdir[fileIndex].LBAlocation to LBAlocation + numBlocks
        -set all blocks for the filesize to 0
    2. clearBit in bitmap to mark the newly freed blocks
    3. "delete the DE" at curdir[fileIndex] by resetting to default values for each DE in initDir.
    3.5 rewrite curdir to disk
    4. return 0 if success

    If the filename doesn't exist in curdir:
    1. display a message saying file not found in curdir
    2. return -1
    */

    int x = findNameInDir(cwdGlobal, filename);
    if (x == -1)
    {
        fprintf(stderr, "File not found in current directory.\n");
        return -1;
    }
    else if (x > 1)
    {
        if (cwdGlobal[x].isDirectory == 1)
        {
            fprintf(stderr, "Path is a directory, not file.\n");
            return -1;
        }
        // Calculate how many blocks the file takes up.
        int numBlocks = (cwdGlobal[x].size + vcb->block_size - 1) / vcb->block_size;
        if(numBlocks < 10)
        {
            numBlocks = 10;
        }
        char *emptyFile = (char *)malloc(numBlocks * vcb->block_size);
        int writeReturn = LBAwrite(emptyFile, numBlocks, cwdGlobal[x].LBAlocation);
        if (writeReturn == numBlocks)
        {
            fsRelease(bm, cwdGlobal[x].LBAlocation, numBlocks);
        
            cwdGlobal[x].LBAlocation = -1; // DEs where i >=2 have starting locations of directories/files
            cwdGlobal[x].size = -1;
            for (int i = 0; i < NAME + 1; i++)
            {
                cwdGlobal[x].name[i] = '\0';
            }
            cwdGlobal[x].timeCreation = (time_t)(-1);
            cwdGlobal[x].lastAccessed = (time_t)(-1);
            cwdGlobal[x].lastModified = (time_t)(-1);
            cwdGlobal[x].isDirectory = -1;
            cwdGlobal[x].dirNumBlocks = -1;

            int writeReturn2 = LBAwrite(cwdGlobal, cwdGlobal[0].dirNumBlocks, cwdGlobal[0].LBAlocation);
            if (writeReturn2 == cwdGlobal[0].dirNumBlocks)
            {
                return 0;
            }
            else
            {
                fprintf(stderr, "fs_delete: updating cwd to disk failed.\n");
            }
        }
        else
        {
            fprintf(stderr, "fs_delete: LBAwrite failed.\n");
            return -1;
        }
    }
}
// Removes an empty directory. Returns 0 if success, -1 if failure.
int fs_rmdir(const char *pathname)
{
    /*
    rmdir() removes the directory represented by ‘pathname’ if it is empty.
    IF the directory is not empty then this function will not succeed.
    Use parsePath on the pathname
    Case 1: parsePath returns -2, Root case?
    print error and return -1
    Case 2: parsePath returns 0 for success and has valid ppi->lei
    Case 3: returns 0 but invalid ppi->lei,
    Case 4: parsePath returns -1 for failure meaning invalid pathname

    If parsePath succeeds:
    check if the directory is empty. If so,
    delete the directory and its metadata in parent.

    If the directory is not empty, error message.
    */
    if (pathname == NULL)
    {
        fprintf(stderr, "Path is null\n");
        return -1;
    }

    // Create a writable copy of the path
    char *pathCopy = strdup(pathname);
    if (pathCopy == NULL)
    {
        fprintf(stderr, "Failed to duplicate path.\n");
        return -1;
    }
    ppinfo ppi;
    int parseFlag = parsePath(pathCopy, &ppi);
    //  If parsePath fails
    if (parseFlag != 0)
    {
        fprintf(stderr, "parsePath failed.\n");
        return (parseFlag);
    }

    if (parseFlag == -2)
    {
        fprintf(stderr, "Cannot delete root.\n");
        return -1;
    }

    if (parseFlag == -1)
    {
        fprintf(stderr, "parsePath failed.\n");
    }

    if (parseFlag == 0)
    {
        /*
        Check if ppi.parent[ppi.lei] points to a dir or file.
        the last element in path will be at parent[lei].
        If it's a file, return error.
        If it's a directory:
        -Load the directory into memory (by opening it)
        -If it has any DEs in any index
            - print "directory is not empty" error
            - Free this directory from memory.
            - return -1
        -If it's empty, delete itself and its metadata in its parent.
            -Write empty buffer to disk at parent[lei].lbalocation
            -Reset parent[lei].attributes
            -Rewrite parent to disk at parent[0].lbalocation
            -Free dir

            any bm updates needed?
        */
        if (ppi.parent[ppi.lei].isDirectory == 0)
        {
            fprintf(stderr, "path is a file, not directory.\n");
            return -1;
        }

        DE *dir = loadDirLBA(ppi.parent[ppi.lei].dirNumBlocks, ppi.parent[ppi.lei].LBAlocation);

        int numDEs = (ppi.parent[0].size / sizeof(DE));

        for (int i = 2; i < numDEs; i++)
        {
            if (dir[i].name[0] != '\0')
            {
                printf("ppi.parent[ppi.lei].name: %s\n", ppi.parent[ppi.lei].name);
                printf("dir[%d].name:%s\n", i, dir[i].name);
                fprintf(stderr, "Directory is not empty - cannot rmdir.\n");
                free(dir);
                return -1;
            }
        }

        // If code has made it here, path is a valid and empty directory.

        // First overwrite the directory on disk, then clear metadata in parent.
        char *emptyBuf = (char *)malloc(dir[0].dirNumBlocks * vcb->block_size);
        int writeRet = LBAwrite(emptyBuf, dir[0].dirNumBlocks, dir[0].LBAlocation);
        if (writeRet == ppi.parent[ppi.lei].dirNumBlocks)
        {
            fsRelease(bm, dir[0].LBAlocation, dir[0].dirNumBlocks);
        
            // Reset DE values at ppi.parent[ppi.lei]
            ppi.parent[ppi.lei].LBAlocation = -1; // DEs where i >=2 have starting locations of directories/files
            ppi.parent[ppi.lei].size = -1;
            for (int i = 0; i < NAME + 1; i++)
            {
                ppi.parent[ppi.lei].name[i] = '\0';
            }
            ppi.parent[ppi.lei].timeCreation = (time_t)(-1);
            ppi.parent[ppi.lei].lastAccessed = (time_t)(-1);
            ppi.parent[ppi.lei].lastModified = (time_t)(-1);
            ppi.parent[ppi.lei].isDirectory = -1;
            ppi.parent[ppi.lei].dirNumBlocks = -1;
            int writeRet2 = LBAwrite(ppi.parent, ppi.parent[0].dirNumBlocks, ppi.parent[0].LBAlocation);
            if (writeRet2 == ppi.parent[0].dirNumBlocks)
            {
                free(dir);
                return 0;
            }
            else
            {
                fprintf(stderr, "fs_rmdir: updating parent to disk failed.\n");
            }
        }
        else
        {
            fprintf(stderr, "fs_rmdir: LBAwrite failed.\n");
            free(dir);
            return -1;
        }
    }
}