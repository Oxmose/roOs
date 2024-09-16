/*******************************************************************************
 * @file ramdisk.c
 *
 * @see ramdisk.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 25/07/2024
 *
 * @version 2.0
 *
 * @brief Kernel's ram disk driver.
 *
 * @details Kernel's ram disk driver. Defines the functions and
 * structures used by the kernel to manage manage the ram disk.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <vfs.h>          /* Virtual File System*/
#include <ioctl.h>        /* IOCTL commands */
#include <panic.h>        /* Kernel panic */
#include <kheap.h>        /* Kernel heap */
#include <mutex.h>        /* Kernel mutex */
#include <stdint.h>       /* Standard int definitions */
#include <string.h>       /* String manipulation */
#include <kerror.h>       /* Kernel errors */
#include <memory.h>       /* Memory manager */
#include <syslog.h>       /* Syslog service */
#include <critical.h>     /* Kernel critical management */
#include <drivermgr.h>    /* Driver manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <ramdisk.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "RAMDISK"

/** @brief FDT property for regs  */
#define RAMDISK_FDT_REG_PROP "reg"
/** @brief FDT property for device */
#define RAMDISK_FDT_DEVICE_PROP "device"

/** @brief The size in bytes of a RamDisk block */
#define RAMDISK_BLOCK_SIZE 512

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the RamDisk controller structure */
typedef struct
{
    /** @brief Start address in virtual memory */
    void* startVirtAddr;

    /** @brief Size of the ramdisk in bytes */
    size_t size;

    /** @brief The VFS driver associated to the RamDisk */
    vfs_driver_t vfsDriver;

    /** @brief The RamDisk driver lock */
    mutex_t lock;
} ramdisk_ctrl_t;


/**
 * @brief RamDisk file descriptor used to keep track of where to access the
 * ram disk.
 */
typedef struct
{
    /** @brief Access permissions */
    bool_t isReadOnly;

    /** @brief Current read offset */
    size_t offset;
} ramdisk_fd_t;

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
#define RAMDISK_ASSERT(COND, MSG, ERROR) {                  \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the RamDisk driver to the system.
 *
 * @details Attaches the RamDisk driver to the system. This function will
 * use the FDT to initialize the RamDisk and retreive the RamDisk parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _ramdiskAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief RamDisk VFS open hook.
 *
 * @details RamDisk VFS open hook. This function returns a handle to control the
 * RamDisk driver through VFS.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
 * @param[in] kpPath The path in the RamDisk driver mount point.
 * @param[in] flags The open flags, must be O_RDWR.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _ramdiskVfsOpen(void*       pDrvCtrl,
                             const char* kpPath,
                             int         flags,
                             int         mode);

/**
 * @brief RamDisk VFS close hook.
 *
 * @details RamDisk VFS close hook. This function closes a handle that was created
 * when calling the open function.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _ramdiskVfsClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief RamDisk VFS read hook.
 *
 * @details RamDisk VFS read hook. This function read a string from the RamDisk
 * volume.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _ramdiskVfsRead(void*  pDrvCtrl,
                               void*  pHandle,
                               void*  pBuffer,
                               size_t count);

/**
 * @brief RamDisk VFS write hook.
 *
 * @details RamDisk VFS write hook. This function writes a string to the RamDisk
 * volume.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _ramdiskVfsWrite(void*       pDrvCtrl,
                                void*       pHandle,
                                const void* kpBuffer,
                                size_t      count);

/**
 * @brief RamDisk VFS IOCTL hook.
 *
 * @details RamDisk VFS IOCTL hook. This function performs the IOCTL for the
 * RamDisk driver.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _ramdiskVfsIOCTL(void*    pDriverData,
                                void*    pHandle,
                                uint32_t operation,
                                void*    pArgs);


/**
 * @brief RamDisk VFS seek hook.
 *
 * @details RamDisk VFS seek hook. This function performs a seek for the
 * RamDisk driver.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in, out] pArgs The arguments for the seek operation.
 *
 * @return The function returns the new offset from the beginning of the file on
 * success and -1 on error;
 */
static ssize_t _ramdiskVfsSeek(void*              pDriverData,
                               void*              pHandle,
                               seek_ioctl_args_t* pArgs);

/**
 * @brief RamDisk VFS tell hook.
 *
 * @details RamDisk VFS tell hook. This function performs a tell for the
 * RamDisk driver.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in, out] pArgs The arguments for the tell operation.
 *
 * @return The function returns the offset from the beginning of the file on
 * success and -1 on error;
 */
static ssize_t _ramdiskVfsTell(void* pDriverData,
                               void* pHandle,
                               void* pArgs);

/**
 * @brief Sets the simulated LBA to the file descriptor.
 *
 * @details Sets the simulated LBA to the file descriptor. The cursor is
 * positionned based on the simulated sector size.
 *
 * @param[in, out] pDrvCtrl The RamDisk driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in, out] lba The LBA value to set
 */
static ssize_t _ramdiskSetLBA(ramdisk_ctrl_t* pCtrl,
                              ramdisk_fd_t*   pDesc,
                              uint64_t        lba);
/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief RamDisk driver instance. */
static driver_t sRAMDISKDriver = {
    .pName         = "RamDisk Driver",
    .pDescription  = "RamDisk Driver roOs.",
    .pCompatible   = "roOs,roOs-ramdisk",
    .pVersion      = "2.0",
    .pDriverAttach = _ramdiskAttach
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _ramdiskAttach(const fdt_node_t* pkFdtNode)
{
    const uintptr_t* kpPtrProp;
    const char*      kpStrProp;
    size_t           propLen;
    ramdisk_ctrl_t*  pCtrl;
    OS_RETURN_E      retCode;
    OS_RETURN_E      error;
    bool_t           isMutexSet;

    isMutexSet = FALSE;

    /* Create the driver controller structure */
    pCtrl = kmalloc(sizeof(ramdisk_ctrl_t));
    if(pCtrl == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pCtrl, 0, sizeof(ramdisk_ctrl_t));

    retCode = mutexInit(&pCtrl->lock,
                        MUTEX_FLAG_QUEUING_PRIO | MUTEX_FLAG_PRIO_ELEVATION);
    if(retCode != OS_NO_ERR)
    {
        goto ATTACH_END;
    }
    isMutexSet = TRUE;

    /* Get the registers, giving the base physical address and size */
    kpPtrProp = fdtGetProp(pkFdtNode, RAMDISK_FDT_REG_PROP, &propLen);
    if(kpPtrProp == NULL || propLen != sizeof(uintptr_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
#ifdef ARCH_64_BITS
    pCtrl->size = FDTTOCPU64(*(kpPtrProp + 1));
    pCtrl->startVirtAddr = (void*)FDTTOCPU64(*kpPtrProp);
#else
    pCtrl->size = FDTTOCPU32(*(kpPtrProp + 1));
    pCtrl->startVirtAddr = (void*)FDTTOCPU32(*kpPtrProp);
#endif

    /* Map the ramdisk in memory */
    pCtrl->startVirtAddr = memoryKernelMap(pCtrl->startVirtAddr,
                                           pCtrl->size,
                                           MEMMGR_MAP_KERNEL |
                                           MEMMGR_MAP_RW     |
                                           MEMMGR_MAP_HARDWARE,
                                           &retCode);
    if(retCode != OS_NO_ERR)
    {
        goto ATTACH_END;
    }


    /* Get the device path */
    kpStrProp = fdtGetProp(pkFdtNode, RAMDISK_FDT_DEVICE_PROP, &propLen);
    if(kpStrProp == NULL || propLen  == 0)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Register the driver */
    pCtrl->vfsDriver = vfsRegisterDriver(kpStrProp,
                                         pCtrl,
                                         _ramdiskVfsOpen,
                                         _ramdiskVfsClose,
                                         _ramdiskVfsRead,
                                         _ramdiskVfsWrite,
                                         NULL,
                                         _ramdiskVfsIOCTL);
    if(pCtrl->vfsDriver == VFS_DRIVER_INVALID)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }


ATTACH_END:
    if(retCode != OS_NO_ERR)
    {
        if(isMutexSet == TRUE)
        {
            error = mutexDestroy(&pCtrl->lock);
            RAMDISK_ASSERT(error == OS_NO_ERR,
                           "Failed to destroy mutex",
                           error);
        }
        if(pCtrl != NULL)
        {
            if(pCtrl->startVirtAddr != NULL)
            {
                error = memoryKernelUnmap(pCtrl->startVirtAddr, pCtrl->size);
                RAMDISK_ASSERT(error == OS_NO_ERR,
                               "Failed to unmap RamDisk",
                               error);
            }
            if(pCtrl->vfsDriver != VFS_DRIVER_INVALID)
            {
                error = vfsUnregisterDriver(&pCtrl->vfsDriver);
                RAMDISK_ASSERT(error == OS_NO_ERR,
                               "Failed to unregister VFS driver",
                               error);
            }

            kfree(pCtrl);
        }
    }

    return retCode;
}

static void* _ramdiskVfsOpen(void*       pDrvCtrl,
                             const char* kpPath,
                             int         flags,
                             int         mode)
{
    ramdisk_fd_t* pDesc;

    (void)mode;

    if(pDrvCtrl == NULL)
    {
        return (void*)-1;
    }

    /* The path must be empty */
    if((*kpPath == '/' && *(kpPath + 1) != 0) || *kpPath != 0)
    {
        return (void*)-1;
    }


    pDesc = kmalloc(sizeof(ramdisk_fd_t));
    if(pDesc == NULL)
    {
        return (void*)-1;
    }
    memset(pDesc, 0, sizeof(ramdisk_fd_t));

    if((flags & O_RDWR) == O_RDWR)
    {
        pDesc->isReadOnly = FALSE;
    }
    else
    {
        pDesc->isReadOnly = TRUE;
    }

    return pDesc;
}

static int32_t _ramdiskVfsClose(void* pDrvCtrl, void* pHandle)
{
    (void)pDrvCtrl;

    if(pHandle != NULL && pHandle != (void*)-1)
    {
        kfree(pHandle);
        return 0;
    }
    else
    {
        return -1;
    }
}

static ssize_t _ramdiskVfsRead(void*  pDrvCtrl,
                               void*  pHandle,
                               void*  pBuffer,
                               size_t count)
{
    ssize_t         maxRead;
    ramdisk_fd_t*   pDesc;
    ramdisk_ctrl_t* pCtrl;
    OS_RETURN_E     error;

    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }


    pCtrl = pDrvCtrl;
    pDesc = pHandle;

    error = mutexLock(&pCtrl->lock);
    if(error != OS_NO_ERR)
    {
        return -1;
    }

    if(pDesc->offset <= pCtrl->size)
    {
        maxRead = MIN(count, pCtrl->size - pDesc->offset);
        memcpy(pBuffer, (char*)pCtrl->startVirtAddr + pDesc->offset, maxRead);
    }
    else
    {
        maxRead = 0;
    }
    pDesc->offset += maxRead;

    error = mutexUnlock(&pCtrl->lock);
    RAMDISK_ASSERT(error == OS_NO_ERR, "Failed to unlock mutex", error);

    return maxRead;
}

static ssize_t _ramdiskVfsWrite(void*       pDrvCtrl,
                                void*       pHandle,
                                const void* kpBuffer,
                                size_t      count)
{
    ssize_t         maxWrite;
    ramdisk_fd_t*   pDesc;
    ramdisk_ctrl_t* pCtrl;
    OS_RETURN_E     error;

    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    pCtrl = pDrvCtrl;
    pDesc = pHandle;

    if(pDesc->isReadOnly == TRUE)
    {
        return -1;
    }

    error = mutexLock(&pCtrl->lock);
    if(error != OS_NO_ERR)
    {
        return -1;
    }

    if(pDesc->offset <= pCtrl->size)
    {
        maxWrite = MIN(count, pCtrl->size - pDesc->offset);
        memcpy((char*)pCtrl->startVirtAddr + pDesc->offset, kpBuffer, maxWrite);
    }
    else
    {
        maxWrite = 0;
    }

    pDesc->offset += maxWrite;

    error = mutexUnlock(&pCtrl->lock);
    RAMDISK_ASSERT(error == OS_NO_ERR, "Failed to unlock mutex", error);

    return maxWrite;
}

static ssize_t _ramdiskVfsIOCTL(void*    pDriverData,
                                void*    pHandle,
                                uint32_t operation,
                                void*    pArgs)
{
    ssize_t retVal;

    switch(operation)
    {
        case VFS_IOCTL_FILE_SEEK:
            retVal = _ramdiskVfsSeek(pDriverData, pHandle, pArgs);
            break;
        case VFS_IOCTL_DEV_GET_SECTOR_SIZE:
            retVal = RAMDISK_BLOCK_SIZE;
            break;
        case VFS_IOCTL_DEV_SET_LBA:
            retVal = _ramdiskSetLBA(pDriverData, pHandle, *(uint64_t*)pArgs);
            break;
        case VFS_IOCTL_FILE_TELL:
            retVal = _ramdiskVfsTell(pDriverData, pHandle, pArgs);
            break;
        default:
            retVal = -1;
    }

    return retVal;
}

static ssize_t _ramdiskVfsSeek(void*              pDriverData,
                               void*              pHandle,
                               seek_ioctl_args_t* pArgs)
{
    ramdisk_fd_t*   pDesc;
    ramdisk_ctrl_t* pCtrl;

    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    pDesc = pHandle;
    pCtrl = pDriverData;

    if(pArgs->direction == SEEK_SET)
    {
        pDesc->offset = pArgs->offset;
    }
    else if(pArgs->direction == SEEK_CUR)
    {
        pDesc->offset += pArgs->offset;
    }
    else if(pArgs->direction == SEEK_END)
    {
        pDesc->offset = pCtrl->size + pArgs->offset;
    }
    else
    {
        return -1;
    }

    return pDesc->offset;
}

static ssize_t _ramdiskVfsTell(void* pDriverData,
                               void* pHandle,
                               void* pArgs)
{
    ramdisk_fd_t* pDesc;

    (void)pArgs;
    (void)pDriverData;

    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    pDesc = pHandle;

    return pDesc->offset;
}

static ssize_t _ramdiskSetLBA(ramdisk_ctrl_t* pCtrl,
                              ramdisk_fd_t*   pDesc,
                              uint64_t        lba)
{
    (void)pCtrl;

    if(pDesc == NULL || pDesc == (void*)-1)
    {
        return -1;
    }

    pDesc->offset = RAMDISK_BLOCK_SIZE * lba;

    return pDesc->offset;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sRAMDISKDriver);

/************************************ EOF *************************************/