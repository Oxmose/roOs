/*******************************************************************************
 * @file ustarfs.c
 *
 * @see ustarfs.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 02/09/2024
 *
 * @version 2.0
 *
 * @brief Kernel's USTAR filesystem driver.
 *
 * @details Kernel's USTAR filesystem driver. Defines the functions and
 * structures used by the kernel to manage USTAR partitions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <vfs.h>          /* Virtual File System*/
#include <ioctl.h>        /* IOCTL commands */
#include <kmutex.h>       /* Kernel critical section */
#include <kheap.h>        /* Kernel heap */
#include <panic.h>        /* Kernel panic */
#include <string.h>       /* String manipulation */
#include <syslog.h>       /* Kernel syslog */
#include <stdbool.h>    /* Bool types */

/* Configuration files */
#include <config.h>

/* Header file */
#include <ustarfs.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Module name */
#define MODULE_NAME "USTAR"

/** @brief USTAR magic value. */
#define USTAR_MAGIC "ustar "
/** @brief USTAR maximal filename length. */
#define USTAR_FILENAME_MAX_LENGTH 100
/** @brief USTAR block size. */
#define USTAR_BLOCK_SIZE 512
/** @brief USTAR file size maximal length. */
#define USTAR_FSIZE_FIELD_LENGTH 12
/** @brief USTAR last edit maximal length. */
#define USTAR_LEDIT_FIELD_LENGTH 12
/** @brief USTAR file user ID maximal length. */
#define USTAR_UID_FIELD_LENGTH 8
/** @brief USTAR file mode maximal length. */
#define USTAR_MODE_FIELD_LENGTH 8
/** @brief USTAR file prefix maximal length. */
#define USTAR_PREFIX_NAME_LENGTH 155

/** @brief USTAR User read permission bitmask. */
#define T_UREAD  0x100
/** @brief USTAR User write permission bitmask. */
#define T_UWRITE 0x080
/** @brief USTAR User execute permission bitmask. */
#define T_UEXEC  0x040

/** @brief USTAR Group read permission bitmask. */
#define T_GREAD  0x020
/** @brief USTAR Group write permission bitmask. */
#define T_GWRITE 0x010
/** @brief USTAR Group execute permission bitmask. */
#define T_GEXEC  0x008

/** @brief USTAR Other read permission bitmask. */
#define T_OREAD  0x004
/** @brief USTAR Other write permission bitmask. */
#define T_OWRITE 0x002
/** @brief USTAR Other execute permission bitmask. */
#define T_OEXEC  0x001

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/**
 * @brief USTAR header block definition as per USTAR standard.
 */
typedef struct
{
    /** @brief USTAR file name */
    char fileName[USTAR_FILENAME_MAX_LENGTH];
    /** @brief USTAR file mode */
    char mode[8];
    /** @brief USTAR owner user id */
    char useId[8];
    /** @brief USTAR owner group id */
    char groupId[8];
    /** @brief Length of the file in bytes */
    char size[USTAR_FSIZE_FIELD_LENGTH];
    /** @brief Modify time of file */
    char lastEdited[12];
    /** @brief Checksum for header */
    char checksum[8];
    /** @brief Type of file */
    char type;
    /** @brief Name of linked file */
    char linkedFileName[USTAR_FILENAME_MAX_LENGTH];
    /** @brief USTAR magic value */
    char magic[6];
    /** @brief USTAR version */
    char ustarVersion[2];
    /** @brief Owner user name */
    char userName[32];
    /** @brief Owner group name */
    char groupName[32];
    /** @brief Device major number */
    char devMajor[8];
    /** @brief Device minor number */
    char devMinor[8];
    /** @brief Prefix for file name */
    char prefix[USTAR_PREFIX_NAME_LENGTH];
    /** @brief Unused padding */
    char padding[12];
} ustar_block_t;

/** @brief USTAR mount driver data */
typedef struct
{
    /** @brief Device file descriptor */
    int32_t devFd;

    /** @brief Mount lock */
    kmutex_t lock;
} ustar_mount_data_t;

/** @brief USTAR file types */
typedef enum
{
    /** @brief Normal file */
    FILE = 0,
    /** @brief Hard link */
    HARD_LINK = 1,
    /** @brief Symbolic link */
    SYM_LINK = 2,
    /** @brief Character device */
    CHAR_DEV = 3,
    /** @brief Block device */
    BLOCK_DEV = 4,
    /** @brief Directory */
    DIRECTORY = 5,
    /** @brief Named pipe (FIFO) */
    NAMED_PIPE = 6

} USTAR_FILE_TYPE_E;

/** @brief USTAR internal file descriptor */
typedef struct
{
    /** @brief Current offset in the file */
    ssize_t offset;

    /** @brief File start offset in the device */
    ssize_t devFdOffset;

    /** @brief Size of the file */
    size_t fileSize;

    /** @brief Type of file */
    USTAR_FILE_TYPE_E type;

    /** @brief File name */
    char name[USTAR_FILENAME_MAX_LENGTH];
} ustar_fd_t;

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
#define USTAR_ASSERT(COND, MSG, ERROR) {                    \
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Mount function for the filesystem.
 *
 * @details Mount function for the filesystem. This function will link a
 * directory to the associated device and use the filesystem to access it.
 *
 * @param[in] kpPath The path of the directory to mount to.
 * @param[in] kpDevPath The path of the device to mount.
 * @param[in, out] pDriverMountData Data generated by the driver when during
 * the mount operation.
 *
 * @return The function returns the success or error status.
 */
static OS_RETURN_E _ustarMount(const char* kpPath,
                               const char* kpDevPath,
                               void**      pDriverMountData);

/**
 * @brief Unmount function for the filesystem.
 *
 * @details Unmount function for the filesystem. This function will unlink a
 * directory to the associated device.
 *
 * @param[in, out] pDriverMountData The driver data generated when mounting
 * the filesystem.
 *
 * @return The function returns the success or error status.
 */
static OS_RETURN_E _ustarUnmount(void* pDriverMountData);

/**
 * @brief USTAR VFS open hook.
 *
 * @details USTAR VFS open hook. This function returns a handle to control the
 * ustar driver through VFS.
 *
 * @param[in, out] pDrvCtrl The ustar driver that was registered in the VFS.
 * @param[in] kpPath The path in the ustar driver mount point.
 * @param[in] flags The open flags.
 * @param[in] mode File open mode.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _ustarVfsOpen(void*       pDrvCtrl,
                           const char* kpPath,
                           int         flags,
                           int         mode);

/**
 * @brief USTAR VFS close hook.
 *
 * @details USTAR VFS close hook. This function closes a handle that was
 * created when calling the open function.
 *
 * @param[in, out] pDrvCtrl The ustar driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _ustarVfsClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief USTAR VFS read hook.
 *
 * @details USTAR VFS read hook. This function reads from a USTAR file.
 *
 * @param[in, out] pDrvCtrl The USTAR driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _ustarVfsRead(void*  pDrvCtrl,
                             void*  pHandle,
                             void*  pBuffer,
                             size_t count);

/**
 * @brief USTAR VFS write hook.
 *
 * @details USTAR VFS write hook. This functio writes a file in the USTAR
 * partition.
 *
 * @param[in, out] pDrvCtrl The USTAR driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _ustarVfsWrite(void*       pDrvCtrl,
                              void*       pHandle,
                              const void* kpBuffer,
                              size_t      count);

/**
 * @brief USTAR VFS IOCTL hook.
 *
 * @details USTAR VFS IOCTL hook. This function performs the IOCTL for the USTAR
 * driver.
 *
 * @param[in, out] pDrvCtrl The USTAR driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _ustarVfsIOCTL(void*    pDriverData,
                              void*    pHandle,
                              uint32_t operation,
                              void*    pArgs);

/**
 * @brief USTAR VFS ReadDir hook.
 *
 * @details USTAR VFS ReadDir hook. This function performs the ReadDir for the
 * USTAR driver.
 *
 * @param[in, out] pDrvCtrl The ustar VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] pDirEntry The directory entry to fill by the driver.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _ustarVfsReadDir(void*     pDriverData,
                                void*     pHandle,
                                dirent_t* pDirEntry);
/**
 * @brief Fills the USTAR block with the next file in the partition.
 *
 * @details Fills the USTAR block with the next file in the partition. This
 * file might not be in the same folder as the previous file.
 *
 * @param[in] devFd The USTAR partition file descriptor.
 * @param[in,out] pBlock The filer header's block to use to get the next header.
 * This will be filled with the next header once the operation completed.
 * @param[in,out] pBlockId The block ID of the current file. This will be filled
 * with the block ID of the next file in the partition.
 */
static void _ustarGetNextFile(int32_t        devFd,
                              ustar_block_t* pBlock,
                              uint32_t*      pBlockId);

/**
 * @brief Checks if a USTAR header block is valid.
 *
 * @details Checks if a USTAR header block is valid. The integrity of the header
 * is validated with the checksum, magix and other values contained in the
 * header block.
 *
 * @param[in] kpBlock The header block to check.
 *
 * @return OS_NO_ERR is retuned is the block is valid. Otherwise, an error is
 * returned.
 */
inline static OS_RETURN_E _ustarCheckBlock(const ustar_block_t* kpBlock);

/**
 * @brief Translates the decimal value given as parameter to an octal value
 * stored in a string.
 *
 * @details Translates the decimal value given as parameter to an octal value
 * stored in a string.
 *
 * @param[out] pOct The buffer used to store the octal value.
 * @param[in] value The value to translate.
 * @param[in] size The size of the buffer used to receive the octal value.
 */
inline static void _uint2oct(char* pOct, uint32_t value, size_t size);

/**
 * @brief Translates the octal value giben as parameter to its decimal value.
 *
 * @details Translates the octal value giben as parameter to its decimal value.
 *
 * @param[in] kpOct The octal value to translated.
 * @param[in] size The size of the buffer that contains the octal value.
 *
 * @return The decimal value of the octal value stored in the string is
 * returned.
 */
inline static uint32_t _oct2uint(const char* kpOct, size_t size);

/**
 * @brief USTAR VFS seek hook.
 *
 * @details USTAR VFS seek hook. This function performs a seek for the
 * USTAR driver.
 *
 * @param[in, out] pDrvCtrl The USTAR driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in, out] pArgs The arguments for the seek operation.
 *
 * @return The function returns the new offset from the beginning of the file on
 * success and -1 on error;
 */
static ssize_t _ustarVfsSeek(void*              pDriverData,
                             void*              pHandle,
                             seek_ioctl_args_t* pArgs);
/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief USTAR driver instance. */
static fs_driver_t sUSTARDriver = {
    .pName         = "ustar",
    .pMount        = _ustarMount,
    .pUnmount      = _ustarUnmount,
    .pOpen         = _ustarVfsOpen,
    .pClose        = _ustarVfsClose,
    .pRead         = _ustarVfsRead,
    .pWrite        = _ustarVfsWrite,
    .pReadDir      = _ustarVfsReadDir,
    .pIOCTL        = _ustarVfsIOCTL
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _ustarMount(const char* kpPath,
                               const char* kpDevPath,
                               void**      pDriverMountData)
{
    (void)kpPath;
    ssize_t             readSize;
    int32_t             retCode;
    ustar_mount_data_t* pData;
    ustar_block_t       currentBlock;
    seek_ioctl_args_t   seekArgs;
    OS_RETURN_E         err;

    pData = kmalloc(sizeof(ustar_mount_data_t));
    if(pData == NULL)
    {
        return OS_ERR_NO_MORE_MEMORY;
    }

    /* Open the device file descriptor */
    pData->devFd = vfsOpen(kpDevPath, O_RDWR, 0);
    if(pData->devFd < 0)
    {
        kfree(pData);
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Read the first 512 bytes (USTAR block) */
    seekArgs.direction = SEEK_SET;
    seekArgs.offset = 0;
    retCode = vfsIOCTL(pData->devFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    if(retCode < 0)
    {
        kfree(pData);
        return OS_ERR_INCORRECT_VALUE;
    }
    readSize = vfsRead(pData->devFd, &currentBlock, USTAR_BLOCK_SIZE);
    if(readSize != USTAR_BLOCK_SIZE)
    {
        kfree(pData);
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Check if USTAR */
    err = _ustarCheckBlock(&currentBlock);
    if(err != OS_NO_ERR)
    {
        kfree(pData);
        return err;
    }

    err = kmutexInit(&pData->lock, KMUTEX_FLAG_QUEUING_PRIO);
    if(err != OS_NO_ERR)
    {
        kfree(pData);
        return err;
    }

    *pDriverMountData = pData;
    return OS_NO_ERR;
}

static OS_RETURN_E _ustarUnmount(void* pDriverMountData)
{
    ustar_mount_data_t* pData;
    int32_t             retCode;
    ssize_t             readSize;
    ustar_block_t       currentBlock;
    seek_ioctl_args_t   seekArgs;
    OS_RETURN_E         err;

    if(pDriverMountData == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }
    pData = (ustar_mount_data_t*)pDriverMountData;

     /* Read the first 512 bytes (USTAR block) */
    seekArgs.direction = SEEK_SET;
    seekArgs.offset = 0;
    retCode = vfsIOCTL(pData->devFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    if(retCode < 0)
    {
        return OS_ERR_INCORRECT_VALUE;
    }
    readSize = vfsRead(pData->devFd, &currentBlock, USTAR_BLOCK_SIZE);
    if(readSize != USTAR_BLOCK_SIZE)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Check if USTAR */
    err = _ustarCheckBlock(&currentBlock);
    if(err != OS_NO_ERR)
    {
        return err;
    }

    /* Close the file descriptor */
    retCode = vfsClose(pData->devFd);
    if(retCode != 0)
    {
        kfree(pData);
        return OS_ERR_INCORRECT_VALUE;
    }

    kfree(pData);
    return OS_NO_ERR;
}

static void* _ustarVfsOpen(void*       pDrvCtrl,
                           const char* kpPath,
                           int         flags,
                           int         mode)
{
    ustar_mount_data_t* pData;
    ustar_block_t       currentBlock;
    bool                found;
    OS_RETURN_E         err;
    uint32_t            blockId;
    seek_ioctl_args_t   seekArgs;
    int32_t             retCode;
    ssize_t             readSize;
    ustar_fd_t*         pFileDesc;
    size_t              pathLen;
    size_t              fileLen;
    (void)mode;

    if(pDrvCtrl == NULL || kpPath == NULL)
    {
        return (void*)-1;
    }

    if(flags != O_RDONLY)
    {
        return (void*)-1;
    }

    pData = (ustar_mount_data_t*)pDrvCtrl;

    /* USTAR max path is 100 character */
    if(strlen(kpPath) > USTAR_FILENAME_MAX_LENGTH)
    {
        return (void*)-1;
    }

    /* If we open the root */
    if(*kpPath == 0)
    {
         /* Create the file descriptor */
        pFileDesc = kmalloc(sizeof(ustar_fd_t));
        if(pFileDesc != NULL)
        {
            /* Setup file descriptor */
            pFileDesc->offset = 0;
            pFileDesc->type = DIRECTORY;
            pFileDesc->devFdOffset = 0;
            pFileDesc->fileSize = 0;
            pFileDesc->name[0] = 0;
        }
        else
        {
            pFileDesc = (void*)-1;
        }

        return pFileDesc;
    }

#if USTAR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Opening %s", kpPath);
#endif

    err = kmutexLock(&pData->lock);
    if(err != OS_NO_ERR)
    {
        return (void*)-1;
    }

    /* Read the first 512 bytes (USTAR block) */
    seekArgs.direction = SEEK_SET;
    seekArgs.offset = 0;
    retCode = vfsIOCTL(pData->devFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    if(retCode < 0)
    {
        err = kmutexUnlock(&pData->lock);
        USTAR_ASSERT(err == OS_NO_ERR,  "Failed to unlock acquired mutex", err);
        return (void*)-1;
    }
    readSize = vfsRead(pData->devFd, &currentBlock, USTAR_BLOCK_SIZE);
    if(readSize != USTAR_BLOCK_SIZE)
    {
        err = kmutexUnlock(&pData->lock);
        USTAR_ASSERT(err == OS_NO_ERR,  "Failed to unlock acquired mutex", err);
        return (void*)-1;
    }

    err = _ustarCheckBlock(&currentBlock);
    if(err != OS_NO_ERR)
    {
        err = kmutexUnlock(&pData->lock);
        USTAR_ASSERT(err == OS_NO_ERR,  "Failed to unlock acquired mutex", err);
        return (void*)-1;
    }

    found   = false;
    blockId = 0;

    /* Search for the file, if first filename character is NULL, we reached the
     * end of the search
     */
    pathLen = strlen(kpPath);
    while(currentBlock.fileName[0] != 0)
    {
#if USTAR_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "Checking %s",
               currentBlock.fileName);
#endif
        /* If the current file is a directory */
        fileLen = strlen(currentBlock.fileName);
        if(currentBlock.fileName[fileLen - 1] == '/')
        {
            if((kpPath[pathLen - 1] != '/') && (pathLen == fileLen - 1))
            {
                if(strncmp(kpPath, currentBlock.fileName, pathLen) == 0)
                {
                    found = true;
                    err = _ustarCheckBlock(&currentBlock);
                    break;
                }
            }
        }
        else if(strncmp(kpPath,
                        currentBlock.fileName,
                        USTAR_FILENAME_MAX_LENGTH) == 0)
        {
            found = true;
            err = _ustarCheckBlock(&currentBlock);
            break;
        }
        _ustarGetNextFile(pData->devFd, &currentBlock, &blockId);
    }

    pFileDesc = (void*)-1;

    if(found == true && err == OS_NO_ERR)
    {
        /* Create the file descriptor */
        pFileDesc = kmalloc(sizeof(ustar_fd_t));
        if(pFileDesc != NULL)
        {
            /* Setup file descriptor */
            pFileDesc->offset = 0;
            pFileDesc->type = currentBlock.type - '0';
            pFileDesc->devFdOffset = vfsIOCTL(pData->devFd,
                                              VFS_IOCTL_FILE_TELL,
                                              NULL);
            if(pFileDesc->devFdOffset < 0)
            {
                kfree(pFileDesc);
                pFileDesc = (void*)-1;
            }

            fileLen = strlen(currentBlock.fileName);
            memcpy(pFileDesc->name, currentBlock.fileName, fileLen);
            pFileDesc->name[fileLen] = 0;

            /* Get the file data */
            pFileDesc->fileSize = _oct2uint(currentBlock.size,
                                            USTAR_FSIZE_FIELD_LENGTH);
#if USTAR_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                MODULE_NAME,
                "Opened %s",
                currentBlock.fileName);
#endif
        }
        else
        {
            pFileDesc = (void*)-1;
        }
    }

    err = kmutexUnlock(&pData->lock);
    USTAR_ASSERT(err == OS_NO_ERR,  "Failed to unlock acquired mutex", err);
    return pFileDesc;
}

static int32_t _ustarVfsClose(void* pDrvCtrl, void* pHandle)
{
    ustar_fd_t* pFileDesc;

    if(pDrvCtrl == NULL || pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    pFileDesc = (ustar_fd_t*)pHandle;

    /* Free the descriptor */
    pFileDesc->devFdOffset = -1;
    pFileDesc->offset = -1;
    kfree(pHandle);

    return 0;
}

static ssize_t _ustarVfsRead(void*  pDrvCtrl,
                             void*  pHandle,
                             void*  pBuffer,
                             size_t count)
{
    ustar_fd_t*         pFileDesc;
    ustar_block_t       currentBlock;
    ssize_t             readSize;
    ssize_t             devReadSize;
    ssize_t             dataRead;
    ssize_t             retVal;
    size_t              blockOffset;
    ustar_mount_data_t* pData;
    seek_ioctl_args_t   seekArgs;
    OS_RETURN_E         err;

    if(pDrvCtrl == NULL ||
       pHandle == NULL ||
       pHandle == (void*)-1 ||
       pBuffer == NULL)
    {
        return -1;
    }

    /* Check handle */
    pFileDesc = (ustar_fd_t*)pHandle;
    if(pFileDesc->devFdOffset < 0 ||
       pFileDesc->offset < 0 ||
       pFileDesc->type != FILE)
    {
        return -1;
    }

    pData = (ustar_mount_data_t*)pDrvCtrl;

    /* Get the maximal size to read */
    readSize = MIN(count, pFileDesc->fileSize - pFileDesc->offset);
    if(readSize == 0)
    {
        return 0;
    }

    /* Set the device position */
    seekArgs.direction = SEEK_SET;
    seekArgs.offset    = pFileDesc->devFdOffset +
                         ((pFileDesc->offset / USTAR_BLOCK_SIZE) *
                          USTAR_BLOCK_SIZE);

    err = kmutexLock(&pData->lock);
    if(err != OS_NO_ERR)
    {
        return -1;
    }

    retVal = vfsIOCTL(pData->devFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    if(retVal < 0)
    {
        err = kmutexUnlock(&pData->lock);
        USTAR_ASSERT(err == OS_NO_ERR, "Failed to unlock acquired mutex", err);
        return -1;
    }

    /* Read while we can */
    retVal = 0;
    while(readSize > 0)
    {
        /* Read the current block */
        devReadSize = vfsRead(pData->devFd, &currentBlock, USTAR_BLOCK_SIZE);
        if(devReadSize != USTAR_BLOCK_SIZE)
        {
            err = kmutexUnlock(&pData->lock);
            USTAR_ASSERT(err == OS_NO_ERR,
                         "Failed to unlock acquired mutex",
                         err);
            return -1;
        }

        /* Get how much data we should read from the current block */
        blockOffset = pFileDesc->offset % USTAR_BLOCK_SIZE;
        if(blockOffset != 0)
        {
            dataRead = MIN((ssize_t)(USTAR_BLOCK_SIZE - blockOffset), readSize);
        }
        else
        {
            dataRead = MIN(USTAR_BLOCK_SIZE, readSize);
        }

        /* Copy to buffer */
        memcpy((char*)pBuffer + retVal,
               ((uint8_t*)&currentBlock) + blockOffset,
               dataRead);

        /* Update offsets */
        readSize -= dataRead;
        retVal   += dataRead;
        pFileDesc->offset += dataRead;
    }

    err = kmutexUnlock(&pData->lock);
    USTAR_ASSERT(err == OS_NO_ERR, "Failed to unlock acquired mutex", err);

    return retVal;
}

static ssize_t _ustarVfsWrite(void*       pDrvCtrl,
                              void*       pHandle,
                              const void* kpBuffer,
                              size_t      count)
{
    (void)pDrvCtrl;
    (void)pHandle;
    (void)kpBuffer;
    (void)count;

    /* Not supported */
    return -1;
}

static ssize_t _ustarVfsIOCTL(void*    pDriverData,
                              void*    pHandle,
                              uint32_t operation,
                              void*    pArgs)
{
    ssize_t retVal;

    switch(operation)
    {
        case VFS_IOCTL_FILE_SEEK:
            retVal = _ustarVfsSeek(pDriverData, pHandle, pArgs);
            break;
        default:
            retVal = -1;
    }

    return retVal;
}

static int32_t _ustarVfsReadDir(void*     pDriverData,
                                void*     pHandle,
                                dirent_t* pDirEntry)
{
    ustar_mount_data_t* pData;
    ustar_block_t       currentBlock;
    uint32_t            foundCount;
    OS_RETURN_E         err;
    uint32_t            blockId;
    seek_ioctl_args_t   seekArgs;
    int32_t             retCode;
    ustar_fd_t*         pFileDesc;
    size_t              pathSize;
    size_t              filePathSize;
    ssize_t             readSize;
    ssize_t             firstOffset;
    bool                found;

    if(pDriverData == NULL || pHandle == NULL || pDirEntry == NULL)
    {
        return -1;
    }

    pFileDesc = pHandle;
    if(pFileDesc->type != DIRECTORY || pFileDesc->offset == -1)
    {
        return -1;
    }

#if USTAR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Reading directory %s",
           pFileDesc->name);
#endif

    pData = (ustar_mount_data_t*)pDriverData;

    err = kmutexLock(&pData->lock);
    if(err != OS_NO_ERR)
    {
        return -1;
    }

    /* Read the first 512 bytes (USTAR block) */
    seekArgs.direction = SEEK_SET;
    seekArgs.offset = 0;
    retCode = vfsIOCTL(pData->devFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    if(retCode < 0)
    {
        err = kmutexUnlock(&pData->lock);
        USTAR_ASSERT(err == OS_NO_ERR,  "Failed to unlock acquired mutex", err);
        return -1;
    }
    readSize = vfsRead(pData->devFd, &currentBlock, USTAR_BLOCK_SIZE);
    if(readSize != USTAR_BLOCK_SIZE)
    {
        err = kmutexUnlock(&pData->lock);
        USTAR_ASSERT(err == OS_NO_ERR,  "Failed to unlock acquired mutex", err);
        return -1;
    }

    err = _ustarCheckBlock(&currentBlock);
    if(err != OS_NO_ERR)
    {
        err = kmutexUnlock(&pData->lock);
        USTAR_ASSERT(err == OS_NO_ERR,  "Failed to unlock acquired mutex", err);
        return -1;
    }

    foundCount = 0;
    blockId = 0;
    pathSize = strlen(pFileDesc->name);
    firstOffset = pFileDesc->offset;
    /* Search for the file, if first filename character is NULL, we reached the
     * end of the search
     */
    while(currentBlock.fileName[0] != 0)
    {
        err = _ustarCheckBlock(&currentBlock);
        if(err != OS_NO_ERR)
        {
            err = kmutexUnlock(&pData->lock);
            USTAR_ASSERT(err == OS_NO_ERR, "Failed to unlock acquired mutex", err);
            return -1;
        }

        /* Check if we are in the root */
        if(pathSize == 0)
        {
            for(filePathSize = 0;
                *(currentBlock.fileName + filePathSize) != 0;
                ++filePathSize)
            {
                if(*(currentBlock.fileName + filePathSize) == '/')
                {
                    ++filePathSize;
                    break;
                }
            }
            if(*(currentBlock.fileName + filePathSize) == 0)
            {
                if(foundCount == pFileDesc->offset)
                {
                    if(pFileDesc->type == FILE)
                    {
                        pDirEntry->type  = VFS_FILE_TYPE_FILE;
                    }
                    else
                    {
                        pDirEntry->type  = VFS_FILE_TYPE_DIR;
                    }

                    filePathSize = strlen(currentBlock.fileName);

                    if(filePathSize - pathSize > VFS_FILENAME_MAX_LENGTH)
                    {
                        err = kmutexUnlock(&pData->lock);
                        USTAR_ASSERT(err == OS_NO_ERR,
                                     "Failed to unlock acquired mutex",
                                     err);
                        return -1;
                    }
                    memcpy(pDirEntry->pName,
                           currentBlock.fileName + pathSize,
                           filePathSize - pathSize);
                    pDirEntry->pName[filePathSize - pathSize] = 0;

                    ++pFileDesc->offset;
                    ++foundCount;
                    break;
                }
                else
                {
                    ++foundCount;
                }
            }
        }
        /* Check if the path is the same */
        else if(strncmp(pFileDesc->name,
                        currentBlock.fileName,
                        pathSize) == 0)
        {
            /* Check if this is the same folder */
            if(pathSize != strlen(currentBlock.fileName))
            {
                /* Check if this is a direct child */
                found = true;
                for(filePathSize = pathSize;
                    *(currentBlock.fileName + filePathSize) != 0;
                    ++filePathSize)
                {
                    if(*(currentBlock.fileName + filePathSize) == '/' &&
                    *(currentBlock.fileName + filePathSize + 1) != 0)
                    {
                        found = false;
                        break;
                    }
                }

                /* If this is a folder */
                if(found == true)
                {
                    if(foundCount == pFileDesc->offset)
                    {
                        if(pFileDesc->type == FILE)
                        {
                            pDirEntry->type  = VFS_FILE_TYPE_FILE;
                        }
                        else
                        {
                            pDirEntry->type  = VFS_FILE_TYPE_DIR;
                        }

                        filePathSize = strlen(currentBlock.fileName);

                        if(filePathSize - pathSize > VFS_FILENAME_MAX_LENGTH)
                        {
                            err = kmutexUnlock(&pData->lock);
                            USTAR_ASSERT(err == OS_NO_ERR,
                                        "Failed to unlock acquired mutex",
                                        err);
                            return -1;
                        }
                        memcpy(pDirEntry->pName,
                            currentBlock.fileName + pathSize,
                            filePathSize - pathSize);
                        pDirEntry->pName[filePathSize - pathSize] = 0;

                        ++pFileDesc->offset;
                        ++foundCount;
                        break;
                    }
                    else
                    {
                        ++foundCount;
                    }
                }
            }
        }
        _ustarGetNextFile(pData->devFd, &currentBlock, &blockId);
    }

    err = kmutexUnlock(&pData->lock);
    USTAR_ASSERT(err == OS_NO_ERR, "Failed to unlock acquired mutex", err);

    /* If we found the the same as the offset */
    if(firstOffset != pFileDesc->offset)
    {
        return 1;
    }
    else
    {
        pFileDesc->offset = -1;
        return -1;
    }
}

static void _ustarGetNextFile(int32_t        devFd,
                              ustar_block_t* pBlock,
                              uint32_t*      pBlockId)
{
    OS_RETURN_E       err;
    uint32_t          size;
    int32_t           retCode;
    ssize_t           readSize;
    seek_ioctl_args_t seekArgs;

    /* We loop over all possible empty names (removed files) */
    do
    {
        err = _ustarCheckBlock(pBlock);
        if(err != OS_NO_ERR)
        {
            /* Block not found */
            pBlock->fileName[0] = 0;
            return;
        }

        /* Next block is current block + file size / block_size */
        size = _oct2uint(pBlock->size, USTAR_FSIZE_FIELD_LENGTH - 1) +
               USTAR_BLOCK_SIZE;

        if(size % USTAR_BLOCK_SIZE != 0)
        {
            size += USTAR_BLOCK_SIZE - (size % USTAR_BLOCK_SIZE);
        }

        *pBlockId = *pBlockId + size / USTAR_BLOCK_SIZE;

        /* Read the first 512 bytes (USTAR block) */
        seekArgs.direction = SEEK_SET;
        seekArgs.offset = (size_t)(*pBlockId * USTAR_BLOCK_SIZE);
        retCode = vfsIOCTL(devFd, VFS_IOCTL_FILE_SEEK, &seekArgs);

        if(retCode < 0)
        {
            pBlock->fileName[0] = 0;
            return;
        }
        readSize = vfsRead(devFd, pBlock, USTAR_BLOCK_SIZE);
        if(readSize != USTAR_BLOCK_SIZE)
        {
            pBlock->fileName[0] = 0;
            return;
        }
    } while(pBlock->fileName[0] == 0);
}

inline static OS_RETURN_E _ustarCheckBlock(const ustar_block_t* kpBlock)
{
    if(strncmp(kpBlock->magic, USTAR_MAGIC, 6) != 0)
    {
        return OS_ERR_INCORRECT_VALUE;
    }
    return OS_NO_ERR;
}

inline static void _uint2oct(char* pOct, uint32_t value, size_t size)
{
    char     tmp[32];
    uint32_t pos;

    pos = 0;
    memset(pOct, '0', size);

    if (value == 0)
    {
         return;
    }

    /* Fill temp buffer */
    while (value != 0)
    {
        tmp[pos++] = '0' + (value % 8);
        value /= 8;
    }

    while(pos != 0)
    {
        pOct[size - pos - 1] = tmp[pos - 1];
        --pos;
    }

    /* NULL terminate */
    pOct[size - 1] = 0;
}

inline static uint32_t _oct2uint(const char* kpOct, size_t size)
{
    uint32_t out;
    uint32_t i;

    out = 0;
    i = 0;
    while (i < size && kpOct[i])
    {
        out = (out << 3) | (uint32_t)(kpOct[i++] - '0');
    }
    return out;
}

static ssize_t _ustarVfsSeek(void*              pDriverData,
                             void*              pHandle,
                             seek_ioctl_args_t* pArgs)
{
    ustar_fd_t* pFileDesc;

    (void)pDriverData;

    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    pFileDesc = pHandle;

    if(pArgs->direction == SEEK_SET)
    {
        if(pArgs->offset <= pFileDesc->fileSize)
        {
            pFileDesc->offset = pArgs->offset;
        }
    }
    else if(pArgs->direction == SEEK_CUR)
    {
        if(pFileDesc->offset + pArgs->offset <= pFileDesc->fileSize)
        {
            pFileDesc->offset += pArgs->offset;
        }
    }

    return pFileDesc->offset;
}

/***************************** DRIVER REGISTRATION ****************************/
VFS_REG_FS(sUSTARDriver);

/************************************ EOF *************************************/