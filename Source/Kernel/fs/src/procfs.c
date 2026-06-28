/*******************************************************************************
 * @file procsfs.c
 *
 * @see procsfs.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 13/10/2025
 *
 * @version 1.0
 *
 * @brief Kernel's process filesystem driver.
 *
 * @details Kernel's process filesystem driver. Defines the functions and
 * structures used by the kernel to manage the procfs entries.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <vfs.h>          /* Virtual File System*/
#include <kheap.h>        /* Kernel Heap */
#include <panic.h>        /* Kernel panic */
#include <stdint.h>       /* Standard int definitions */
#include <string.h>       /* String manipulation */
#include <kerror.h>       /* Kernel errors */

/* Configuration files */
#include <config.h>

/* Header file */
#include <procfs.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "PROCFS"

/** @brief Stores the procsfs entry directory name */
#define PROCFS_ROOT_DIR_PATH "/proc"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines the type of entry in the procfs */
typedef enum
{
    /** @brief Directory entry */
    PROCFS_ENTRY_DIR,
    /** @brief File entry */
    PROCFS_ENTRY_FILE
} PROCFS_ENTRY_TYPE_E;

/** @brief Defines a procfs entry */
typedef struct
{
    /** @brief Type of the entry */
    PROCFS_ENTRY_TYPE_E type;

    /** @brief Offset for directories */
    size_t offset;

    /** @brief Keeps open flags information. */
    int32_t openFlags;

    /**
     * @brief Entry structure that contains the file operations and other
     * attributes.
     */
    procfs_dir_entry_t entryData;

    /** @brief Driver specific data passed to internal VFS functions. */
    void* pDriverData;

    /** @brief File specific data passed to internal VFS functions. */
    void* pFileData;
} procfs_entry_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Asserts a condition and generates a kernel panic in case of failure.
 *
 * @details Asserts a condition and generates a kernel panic in case of failure.
 *
 * @param[in] COND The condition to verify.
 * @param[in] MSG The message to print in case of error.
 * @param[in] ERROR The error code.
 *
*/
#define PROCFS_ASSERT(COND, MSG, ERROR) {                   \
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Copies a ProcsFS entry handle.
 *
 * @details Copies a ProcsFS entry handle. This will copy the atttribute and
 * make a deep copy of allocated objects.
 *
 * @param[out] pDstEntry The entry buffer that received the copy.
 * @param[in] kpSrcEntry The source entry to copy.
 *
 * @return The function returns the success or error status.
 */
static OS_RETURN_E _copyProcFsEntryHandle(procfs_entry_t*       pDstEntry,
                                          const procfs_entry_t* kpSrcEntry);

 /**
 * @brief Opens a ProcFS entry.
 *
 * @details Opens a ProcFS entry. The function will find the entry if it exists
 * and fill the entry structure information.
 *
 * @param[in] pEntryLevel The current entry we are in the hierarchy.
 * @param[in] kpPath The path of the entry to open.
 * @param[in/out] pNextTokenIdx The index of the current token in the path.
 *
 * @return The function returns the found entry or NULL on error.
 */
static procfs_entry_t* _procGetEntry(procfs_entry_t* pEntryLevel,
                                     const char*     kpPath,
                                     ssize_t*        pNextTokenIdx);

/**
 * @brief ProcFS entries open hook.
 *
 * @details ProcFS entries open hook. This function returns a
 * handle to control the procfs entries entries.
 *
 * @param[in, out] pDrvCtrl The ProcFS driver.
 * @param[in] kpPath The path of the entry to open.
 * @param[in] flags The open flags.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _procVfsOpen(void*       pDrvCtrl,
                          const char* kpPath,
                          int         flags,
                          int         mode);

/**
 * @brief ProcFS entries close hook.
 *
 * @details ProcFS entries close hook. This function closes a
 * handle that was created when calling the open function.
 *
 * @param[in, out] pDrvCtrl The ProcFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _procVfsClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief ProcFS entries write hook.
 *
 * @details ProcFS entries write hook.
 *
 * @param[in, out] pDrvCtrl The ProcFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _procVfsWrite(void*       pDrvCtrl,
                             void*       pHandle,
                             const void* kpBuffer,
                             size_t      count);

/**
 * @brief ProcFS entries read hook.
 *
 * @details ProcFS entries read hook.
 *
 * @param[in, out] pDrvCtrl The ProcFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _procVfsRead(void*  pDrvCtrl,
                            void*  pHandle,
                            void*  pBuffer,
                            size_t count);

/**
 * @brief ProcFS entries ReadDir hook.
 *
 * @details ProcFS entries ReadDir hook. This function performs
 * the ReadDir for the procfs driver.
 *
 * @param[in, out] pDrvCtrl The ProcFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] pDirEntry The directory entry to fill by the driver.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _procVfsReadDir(void*     pDriverData,
                               void*     pHandle,
                               dirent_t* pDirEntry);

/**
 * @brief ProcFS entries IOCTL hook.
 *
 * @details ProcFS entries IOCTL hook. This function performs
 *  the IOCTL for the procfs driver.
 *
 * @param[in, out] pDrvCtrl The ProcFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _procVfsIOCTL(void*    pDriverData,
                             void*    pHandle,
                             uint32_t operation,
                             void*    pArgs);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief ProcFS root and directory file operations */
static procfs_file_operations_t sProcFsFops = {
    .pOpen = NULL,
    .pClose = NULL,
    .pRead = NULL,
    .pWrite = NULL,
    .pReadDir = NULL,
    .pIOCTL = NULL
};

/** @brief ProcFS root entry. */
static procfs_entry_t sProcFsRootEntry = {
    .pDriverData = NULL,
    .pFileData = NULL,
    .type = PROCFS_ENTRY_DIR,
    .offset = 0,
    .openFlags = O_RDONLY,
    .entryData = {
        .name = "\0",
        .mode = 0444,
        .fops = &sProcFsFops,
        .pParent = NULL,
        .pNext = NULL,
        .pSubDir = NULL,
        .pData = &sProcFsRootEntry
    }
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _copyProcFsEntryHandle(procfs_entry_t*       pDstEntry,
                                          const procfs_entry_t* kpSrcEntry)
{
    size_t nameLen;

    /* Copy the raw data */
    memcpy(pDstEntry, kpSrcEntry, sizeof(procfs_entry_t));

    /* Make a deep copy of the name */
    nameLen = strlen(kpSrcEntry->entryData.name);
    pDstEntry->entryData.name = kmalloc(nameLen + 1);
    if(pDstEntry->entryData.name == NULL)
    {
        return OS_ERR_NO_MORE_MEMORY;
    }

    memcpy(pDstEntry->entryData.name, kpSrcEntry->entryData.name, nameLen);
    pDstEntry->entryData.name[nameLen] = 0;

    /* Update the offset */
    pDstEntry->offset = 0;

    return OS_NO_ERR;
}

static procfs_entry_t* _procGetEntry(procfs_entry_t* pEntryLevel,
                                     const char*     kpPath,
                                     ssize_t*        pNextTokenIdx)
{
    procfs_entry_t* pNextEntry;

    /* Get the next token */
    *pNextTokenIdx = vfsUtilGetNextPathToken(kpPath);

    /* Check if the tocker is valid */
    if(*pNextTokenIdx <= 0 || pEntryLevel == NULL)
    {
        /* We arrived at the end of the path, nothing to be found... */
        return NULL;
    }

    /* Compare with current level */
    while(pEntryLevel != NULL)
    {
        if(strncmp(pEntryLevel->entryData.name, kpPath, *pNextTokenIdx) == 0)
        {
            /* Found the entry, check if this is a file or directory */
            if(pEntryLevel->type == PROCFS_ENTRY_DIR)
            {
                /* Check if we are at the end of the path */
                if(*(kpPath + *pNextTokenIdx) != 0 &&
                   *(kpPath + *pNextTokenIdx + 1) != 0)
                {
                    /* Continue searching */
                    if(pEntryLevel->entryData.pSubDir != NULL)
                    {
                        pNextEntry = (procfs_entry_t*)
                                     pEntryLevel->entryData.pSubDir->pData;
                        return _procGetEntry(pNextEntry,
                                             kpPath + *pNextTokenIdx + 1,
                                             pNextTokenIdx);
                    }
                    else
                    {
                        return NULL;
                    }
                }
                else
                {
                    return pEntryLevel;
                }
            }
            else
            {
                return pEntryLevel;
            }
        }
        else
        {
            /* Go to next entry */
            if(pEntryLevel->entryData.pNext != NULL)
            {
                pNextEntry = (procfs_entry_t*)
                             pEntryLevel->entryData.pNext->pData;
                return _procGetEntry(pNextEntry, kpPath, pNextTokenIdx);
            }
            else
            {
                return NULL;
            }
        }
    }

    /* Entry was not found */
    return NULL;
}

static void* _procVfsOpen(void*       pDrvCtrl,
                          const char* kpPath,
                          int         flags,
                          int         mode)
{
    procfs_entry_t*       pEntry;
    const procfs_entry_t* kpSourceEntry;
    ssize_t               nextTokenIdx;
    OS_RETURN_E           returnVal;
    void*                 pFileHandle;

    (void)pDrvCtrl;

    /* Allocate the new structure */
    pEntry = kmalloc(sizeof(procfs_entry_t));
    if(pEntry == NULL)
    {
        return (void*)-1;
    }

    /* Check if we want to open an entry of a directory directory */
    if(kpPath[0] != 0)
    {
        /* Get the entry */
        nextTokenIdx = 0;

        /* Nothing to find */
        if(sProcFsRootEntry.entryData.pSubDir == NULL)
        {
            kfree(pEntry);
            return (void*)-1;
        }
        kpSourceEntry = _procGetEntry(
            sProcFsRootEntry.entryData.pSubDir->pData,
            kpPath,
            &nextTokenIdx
        );
        if(kpSourceEntry != NULL)
        {
            /* If this is a file, it needs to be opened by the underlying
             * driver.
             */
            if(kpSourceEntry->type == PROCFS_ENTRY_FILE)
            {
                /* Open the entry */
                if((kpSourceEntry->entryData.mode & mode) == mode &&
                   kpSourceEntry->entryData.fops->pOpen != NULL)
                {
                    /* Open using the underlying driver */
                    pFileHandle = kpSourceEntry->entryData.fops->pOpen(
                        kpSourceEntry->pDriverData,
                        kpPath + nextTokenIdx,
                        flags,
                        mode
                    );

                    if(pFileHandle != (void*)-1)
                    {
                        /* Copy the data */
                        returnVal = _copyProcFsEntryHandle(pEntry,
                                                           kpSourceEntry);
                        pEntry->entryData.mode = mode;
                        pEntry->openFlags = flags;
                        if(returnVal != OS_NO_ERR)
                        {
                            if(kpSourceEntry->entryData.fops->pClose != NULL)
                            {
                                kpSourceEntry->entryData.fops->pClose(
                                    kpSourceEntry->pDriverData,
                                    pFileHandle
                                );
                            }

                            kfree(pEntry);
                            pEntry = (void*)-1;
                        }
                        else
                        {
                            /* Update the file data */
                            pEntry->pFileData = pFileHandle;
                        }

                    }
                    else
                    {
                        kfree(pEntry);
                        pEntry = pFileHandle;
                    }
                }
                else
                {
                    kfree(pEntry);
                    pEntry = (void*)-1;
                }
            }
            else if(flags == O_RDONLY)
            {
                /* Copy the entry */
                returnVal = _copyProcFsEntryHandle(pEntry, kpSourceEntry);

                /* Check return value */
                if(returnVal != OS_NO_ERR)
                {
                    kfree(pEntry);
                    pEntry = (void*)-1;
                }
            }
            else
            {
                kfree(pEntry);
                pEntry = (void*)-1;
            }
        }
        else
        {
            kfree(pEntry);
            pEntry = (void*)-1;
        }
    }
    else
    {
        if(flags == O_RDONLY)
        {
            /* This is the root directory */
            returnVal = _copyProcFsEntryHandle(pEntry, &sProcFsRootEntry);

            /* Check return value */
            if(returnVal != OS_NO_ERR)
            {
                kfree(pEntry);
                pEntry = (void*)-1;
            }
        }
        else
        {
            kfree(pEntry);
            pEntry = (void*)-1;
        }
    }

    return pEntry;
}

static int32_t _procVfsClose(void* pDrvCtrl, void* pHandle)
{
    procfs_entry_t* pEntry;
    int32_t         retVal;

    (void)pDrvCtrl;

    if(pHandle == NULL)
    {
        return -1;
    }
    pEntry = (procfs_entry_t*)pHandle;

    /* Call close if entry */
    if(pEntry->type == PROCFS_ENTRY_FILE)
    {
        if(pEntry->entryData.fops->pClose != NULL)
        {
            retVal = pEntry->entryData.fops->pClose(pEntry->pDriverData,
                                                    pEntry->pFileData);
            if(retVal != 0)
            {
                return retVal;
            }
        }
    }

    /* Release name memory */
    kfree(pEntry->entryData.name);

    /* Release the handle */
    kfree(pHandle);

    return 0;
}

static ssize_t _procVfsWrite(void*       pDrvCtrl,
                             void*       pHandle,
                             const void* kpBuffer,
                             size_t      count)
{
    procfs_entry_t* pEntry;

    (void)pDrvCtrl;

    if(pHandle == NULL || kpBuffer == NULL)
    {
        return -1;
    }

    pEntry = (procfs_entry_t*)pHandle;

    /* Check the write capability */
    if(pEntry->entryData.fops->pWrite != NULL &&
       (pEntry->openFlags & O_RDWR) == O_RDWR)
    {
        return pEntry->entryData.fops->pWrite(pEntry->pDriverData,
                                              pEntry->pFileData,
                                              kpBuffer,
                                              count);
    }

    /* Not supported */
    return -1;
}

static ssize_t _procVfsRead(void*  pDrvCtrl,
                            void*  pHandle,
                            void*  pBuffer,
                            size_t count)
{
    procfs_entry_t* pEntry;

    (void)pDrvCtrl;

    if(pHandle == NULL || pBuffer == NULL)
    {
        return -1;
    }

    pEntry = (procfs_entry_t*)pHandle;

    /* Check the write capability */
    if(pEntry->entryData.fops->pRead != NULL &&
       (pEntry->openFlags & O_RDONLY) == O_RDONLY)
    {
        return pEntry->entryData.fops->pRead(pEntry->pDriverData,
                                             pEntry->pFileData,
                                             pBuffer,
                                             count);
    }

    /* Not supported */
    return -1;
}

static int32_t _procVfsReadDir(void*     pDriverData,
                               void*     pHandle,
                               dirent_t* pDirEntry)
{
    size_t              nameLen;
    size_t              i;
    procfs_entry_t*     pEntry;
    procfs_dir_entry_t* pNextDirEntry;

    (void)pDriverData;

    if(pHandle == NULL || pDirEntry == NULL)
    {
        return -1;
    }

    pEntry = (procfs_entry_t*)pHandle;

    /* Check the read capability */
    if(pEntry->type == PROCFS_ENTRY_DIR)
    {
        pDirEntry->type = VFS_FILE_TYPE_DIR;

        if(pEntry->offset == 0)
        {
            /* Return current */
            pDirEntry->pName[0] = '.';
            pDirEntry->pName[1] = 0;
            pDirEntry->type = VFS_FILE_TYPE_DIR;
            ++pEntry->offset;
            return 1;
        }
        else if(pEntry->offset == 1)
        {
            /* Return parent */
            pDirEntry->pName[0] = '.';
            pDirEntry->pName[1] = '.';
            pDirEntry->pName[2] = 0;

            ++pEntry->offset;
            if(pEntry->entryData.pNext == NULL)
            {
                return 0;
            }
            return 1;
        }
        else
        {
            pNextDirEntry = pEntry->entryData.pSubDir;
            for(i = 2; i < pEntry->offset && pNextDirEntry != NULL; ++i)
            {
                pNextDirEntry = pNextDirEntry->pNext;
            }

            if(pNextDirEntry != NULL)
            {
                ++pEntry->offset;

                nameLen = strlen(pNextDirEntry->name);
                memcpy(pDirEntry->pName,
                       pNextDirEntry->name,
                       MIN(sizeof(pDirEntry->pName) - 1, nameLen));
                pDirEntry->pName[nameLen] = 0;

                if(pNextDirEntry->pNext == NULL)
                {
                    return 0;
                }
                else
                {
                    return 1;
                }
            }
            else
            {
                return -1;
            }
        }
    }
    else if(pEntry->entryData.fops->pReadDir != NULL)
    {
        return pEntry->entryData.fops->pReadDir(pEntry->pDriverData,
                                                pEntry->pFileData,
                                                pDirEntry);
    }

    /* Not supported */
    return -1;
}

static ssize_t _procVfsIOCTL(void*    pDriverData,
                             void*    pHandle,
                             uint32_t operation,
                             void*    pArgs)
{
    procfs_entry_t* pEntry;

    (void)pDriverData;

    if(pHandle == NULL)
    {
        return -1;
    }

    pEntry = (procfs_entry_t*)pHandle;

    /* Check the write capability */
    if(pEntry->entryData.fops->pIOCTL != NULL &&
       (pEntry->openFlags & O_RDWR) == O_RDWR)
    {
        return pEntry->entryData.fops->pIOCTL(pEntry->pDriverData,
                                              pEntry->pFileData,
                                              operation,
                                              pArgs);
    }

    /* Not supported */
    return -1;
}

void procfsInit(void)
{
    vfs_driver_t procFsDriver;

    /* Register the driver */
    procFsDriver = vfsRegisterDriver(PROCFS_ROOT_DIR_PATH,
                                     NULL,
                                     _procVfsOpen,
                                     _procVfsClose,
                                     _procVfsRead,
                                     _procVfsWrite,
                                     _procVfsReadDir,
                                     _procVfsIOCTL);

    PROCFS_ASSERT(procFsDriver != VFS_DRIVER_INVALID,
                  "Failed to initialize the ProcFS driver.",
                  OS_ERR_INCORRECT_VALUE);
}

OS_RETURN_E procfsCreateDir(const char*          kpName,
                            const char*          kpParentPath,
                            procfs_dir_entry_t** ppDirectory)
{
    procfs_entry_t*      pParent;
    procfs_entry_t*      pNewDirectory;
    procfs_dir_entry_t*  pDirEntry;
    procfs_dir_entry_t** pEmplaceDirEntry;
    ssize_t              nextTokenIdx;
    size_t               nameLen;
    int32_t              cmpRet;

    if(ppDirectory == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    *ppDirectory = NULL;

    /* Check new name */
    if(kpName == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }
    else if(kpName[0] == 0)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Search for parent */
    if(kpParentPath != NULL)
    {
        pParent = _procGetEntry(&sProcFsRootEntry, kpParentPath, &nextTokenIdx);
    }
    else
    {
        pParent = &sProcFsRootEntry;
    }

    if(pParent != NULL)
    {
        /* Prepare the path */
        for(nameLen = 0; kpName[nameLen] != 0; ++nameLen)
        {
            /* Check there is no delimiter in the path */
            if(kpName[nameLen] == VFS_PATH_DELIMITER)
            {
                return OS_ERR_INCORRECT_VALUE;
            }
        }

        /* Search in parent if entry exists */
        pDirEntry = pParent->entryData.pSubDir;
        pEmplaceDirEntry = &pParent->entryData.pSubDir;
        while(pDirEntry != NULL)
        {
            cmpRet = strncmp(pDirEntry->name, kpName, nameLen);

            /* If exists, return error */
            if(cmpRet == 0)
            {
                return OS_ERR_ALREADY_EXIST;
            }
            else if(cmpRet < 0)
            {
                /* Get the last entry that is lexicographicaly less */
                pEmplaceDirEntry = &pDirEntry->pNext;
            }
            else
            {
                break;
            }

            pDirEntry = pDirEntry->pNext;
        }

        /* Create the entry */
        pNewDirectory = kmalloc(sizeof(procfs_entry_t));
        if(pNewDirectory == NULL)
        {
            return OS_ERR_NO_MORE_MEMORY;
        }
        pNewDirectory->entryData.name = kmalloc(nameLen + 1);
        if(pNewDirectory->entryData.name == NULL)
        {
            kfree(pNewDirectory);
            return OS_ERR_NO_MORE_MEMORY;
        }

        /* Populate entry data */
        memcpy(pNewDirectory->entryData.name, kpName, nameLen);
        pNewDirectory->entryData.name[nameLen] = 0;
        pNewDirectory->type = PROCFS_ENTRY_DIR;
        pNewDirectory->openFlags = O_RDONLY;
        pNewDirectory->offset = 0;
        pNewDirectory->pDriverData = NULL;
        pNewDirectory->pFileData = NULL;
        pNewDirectory->entryData.fops = &sProcFsFops;
        pNewDirectory->entryData.mode = 0444;
        pNewDirectory->entryData.pData = pNewDirectory;

        /* Apply link */
        pNewDirectory->entryData.pNext = *pEmplaceDirEntry;
        *pEmplaceDirEntry = &pNewDirectory->entryData;

        pNewDirectory->entryData.pParent = &pParent->entryData;
        pNewDirectory->entryData.pSubDir = NULL;

        /* Create copy to send to user */
        *ppDirectory = &pNewDirectory->entryData;
    }
    else
    {
        return OS_ERR_NO_SUCH_ID;
    }

    return OS_NO_ERR;
}

OS_RETURN_E procfsRemoveDir(procfs_dir_entry_t** ppDirectory)
{
    procfs_dir_entry_t*  pEntry;
    procfs_dir_entry_t*  pParent;
    procfs_dir_entry_t*  pPrevious;
    procfs_dir_entry_t** ppRemovePlace;

    if(ppDirectory == NULL || *ppDirectory == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    pEntry = *ppDirectory;

    /* Check if directory */
    if(((procfs_entry_t*)pEntry->pData)->type != PROCFS_ENTRY_DIR)
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    /* Check if empty */
    if(pEntry->pSubDir != NULL)
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    pParent = pEntry->pParent;
    ppRemovePlace = &pParent->pSubDir;
    pPrevious = pParent->pSubDir;

    /* Find the link */
    while(pPrevious != NULL)
    {
        if(pPrevious == pEntry)
        {
            break;
        }
        ppRemovePlace = &pPrevious->pNext;
        pPrevious = pPrevious->pNext;
    }

    if(pPrevious == NULL)
    {
        return OS_ERR_NO_SUCH_ID;
    }

    /* Unlink */
    *ppRemovePlace = pEntry->pNext;

    /* Clean the entry */
    kfree(pEntry->name);
    kfree(pEntry->pData);

    *ppDirectory = NULL;
    return OS_NO_ERR;
}

OS_RETURN_E procfsCreateEntry(const char*               kpName,
                              const uint32_t            kMode,
                              procfs_dir_entry_t*       pParent,
                              procfs_file_operations_t* pFops,
                              procfs_dir_entry_t**      pEntry)
{
    size_t               nameLen;
    int32_t              cmpRet;
    procfs_dir_entry_t*  pCursor;
    procfs_dir_entry_t** ppEmplace;
    procfs_entry_t*      pNewEntry;

    /* Check the inputs */
    if(kpName == NULL || pEntry == NULL || pFops == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }
    else if(kpName[0] == 0)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Prepare the path */
    for(nameLen = 0; kpName[nameLen] != 0; ++nameLen)
    {
        /* Check there is no delimiter in the path */
        if(kpName[nameLen] == VFS_PATH_DELIMITER)
        {
            return OS_ERR_INCORRECT_VALUE;
        }
    }

    /* Get root if needed */
    if(pParent == NULL)
    {
        pParent = &sProcFsRootEntry.entryData;
    }

    /* Search of the parent already has an entry with the same name */
    ppEmplace = &pParent->pSubDir;
    pCursor = pParent->pSubDir;
    while(pCursor != NULL)
    {
        cmpRet = strncmp(pCursor->name, kpName, nameLen);

        /* If exists, return error */
        if(cmpRet == 0)
        {
            return OS_ERR_ALREADY_EXIST;
        }
        else if(cmpRet < 0)
        {
            /* Get the last entry that is lexicographicaly less */
            ppEmplace = &pCursor->pNext;
        }
        else
        {
            break;
        }

        pCursor = pCursor->pNext;
    }

    /* Create the entry */
    pNewEntry = kmalloc(sizeof(procfs_entry_t));
    if(pNewEntry == NULL)
    {
        return OS_ERR_NO_MORE_MEMORY;
    }
    pNewEntry->entryData.name = kmalloc(nameLen + 1);
    if(pNewEntry->entryData.name == NULL)
    {
        kfree(pNewEntry);
        return OS_ERR_NO_MORE_MEMORY;
    }

    /* Populate entry data */
    memcpy(pNewEntry->entryData.name, kpName, nameLen);
    pNewEntry->entryData.name[nameLen] = 0;
    pNewEntry->type = PROCFS_ENTRY_FILE;
    pNewEntry->offset = 0;
    pNewEntry->pDriverData = NULL;
    pNewEntry->pFileData = NULL;
    pNewEntry->openFlags = O_RDWR;
    pNewEntry->entryData.fops = pFops;
    pNewEntry->entryData.mode = kMode;
    pNewEntry->entryData.pData = pNewEntry;

    /* Apply link */
    pNewEntry->entryData.pNext = *ppEmplace;
    *ppEmplace = &pNewEntry->entryData;

    pNewEntry->entryData.pParent = pParent;
    pNewEntry->entryData.pSubDir = NULL;

    /* Create copy to send to user */
    *pEntry = &pNewEntry->entryData;

    return OS_NO_ERR;
}

OS_RETURN_E procfsRemoveEntry(const char*         kpName,
                              procfs_dir_entry_t* pParent)
{
    size_t               nameLen;
    int32_t              cmpRet;
    procfs_dir_entry_t*  pCursor;
    procfs_dir_entry_t** ppRemove;

    /* Check the inputs */
    if(kpName == NULL || pParent == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }
    else if(kpName[0] == 0)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Prepare the path */
    for(nameLen = 0; kpName[nameLen] != 0; ++nameLen)
    {
        /* Check there is no delimiter in the path */
        if(kpName[nameLen] == VFS_PATH_DELIMITER)
        {
            return OS_ERR_INCORRECT_VALUE;
        }
    }

    pCursor = pParent->pSubDir;
    ppRemove = &pCursor->pSubDir;
    while(pCursor != NULL)
    {
        cmpRet = strncmp(pCursor->name, kpName, nameLen);

        /* If exists, return error */
        if(cmpRet == 0)
        {
            break;
        }

        ppRemove = &pCursor->pNext;
        pCursor = pCursor->pNext;
    }

    /* Return if not found */
    if(pCursor == NULL)
    {
        return OS_ERR_NO_SUCH_ID;
    }

    /* Check if directory */
    if(((procfs_entry_t*)pCursor->pData)->type != PROCFS_ENTRY_FILE)
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    /* Remove link */
    *ppRemove = pCursor->pNext;

    /* Free memory */
    kfree(pCursor->name);
    kfree(pCursor->pData);

    return OS_NO_ERR;
}
/************************************ EOF *************************************/