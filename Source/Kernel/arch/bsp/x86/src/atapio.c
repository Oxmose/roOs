/*******************************************************************************
 * @file atapio.c
 *
 * @see atapio.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 13/09/2024
 *
 * @version 2.0
 *
 * @brief Kernel's ATA PIO disk driver.
 *
 * @details Kernel's ATA PIO driver. Defines the functions and
 * structures used by the kernel to manage manage the ATA PIO disks.
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
#include <x86cpu.h>       /* CPU port manipulation */
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
#include <atapio.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "ATAPIO"

/** @brief FDT property for comm  */
#define ATAPIO_FDT_COMM_PROP "comm"
/** @brief FDT property for device */
#define ATAPIO_FDT_DEVICE_PROP "device"
/** @brief FDT property for type */
#define ATAPIO_FDT_TYPE_PROP "type"

/** @brief ATA data port offset. */
#define ATA_PIO_DATA_PORT_OFFSET    0x000
/** @brief ATA error port offset. */
#define ATA_PIO_ERROR_PORT_OFFSET   0x001
/** @brief ATA sector count port offset. */
#define ATA_PIO_SC_PORT_OFFSET      0x002
/** @brief ATA sector number port offset. */
#define ATA_PIO_LBALOW_PORT_OFFSET  0x003
/** @brief ATA cylinder low port offset. */
#define ATA_PIO_LBAMID_PORT_OFFSET  0x004
/** @brief ATA cylinder high port offset. */
#define ATA_PIO_LBAHIGH_PORT_OFFSET 0x005
/** @brief ATA head port offset. */
#define ATA_PIO_DEVICE_PORT_OFFSET  0x006
/** @brief ATA status port offset. */
#define ATA_PIO_COMMAND_PORT_OFFSET 0x007
/** @brief ATA control port offset. */
#define ATA_PIO_CONTROL_PORT_OFFSET 0x206

/** @brief ATA PIO identify command. */
#define ATA_PIO_IDENTIFY_COMMAND     0xEC
/** @brief ATA PIO LAB28 read command. */
#define ATA_PIO_READ_SECTORS28_COMMAND  0x20
/** @brief ATA PIO LAB28 write command. */
#define ATA_PIO_WRITE_SECTORS28_COMMAND 0x30
/** @brief ATA PIO LAB48 read command. */
#define ATA_PIO_READ_SECTORS48_COMMAND  0x24
/** @brief ATA PIO LAB48 write command. */
#define ATA_PIO_WRITE_SECTORS48_COMMAND 0x34
/** @brief ATA PIO flush command. */
#define ATA_PIO_FLUSH_SECTOR_COMMAND 0xE7

/** @brief ATA status busy flag. */
#define ATA_PIO_FLAG_BUSY 0x80
/** @brief ATA status error flag. */
#define ATA_PIO_FLAG_ERR  0x01

/** @brief ATA ssupported sector size. */
#define ATA_PIO_SECTOR_SIZE 512

/** @brief ATA PIO device type enumeration. */
typedef enum
{
    /** @brief ATA PIO Master device. */
    ATA_MASTER = 0,
    /** @brief ATA PIO Slave device. */
    ATA_SLAVE  = 1
} ATA_PIO_TYPE_E;

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the ATA PIO controller structure */
typedef struct
{
    /** @brief Device port. */
    uint16_t port;

    /** @brief Device type */
    ATA_PIO_TYPE_E type;

    /** @brief The VFS driver associated to the ATA PIO instance */
    vfs_driver_t vfsDriver;

    /** @brief Defines if the device supported LBA48 addressing */
    bool_t supportLBA48;

    /** @brief Drive size in bytes */
    size_t size;

    /** @brief The disk driver lock */
    mutex_t lock;

    /** @brief Sector buffer used during write operations */
    uint8_t pBufferSectors[2][ATA_PIO_SECTOR_SIZE];
} atapio_ctrl_t;


/**
 * @brief ATA PIO file descriptor used to keep track of where to access the
 * disk.
 */
typedef struct
{
    /** @brief Access permissions */
    bool_t isReadOnly;

    /** @brief Current offset in the disk */
    size_t offset;
} atapio_fd_t;

/** @brief Defines the device identify data structure */
typedef struct
{

#if 0 /* TODO: Complete */
    uint16_t reserved0 : 1;
    uint16_t retired0 : 1;
    uint16_t incompleteResponse : 1;
    uint16_t retired1 : 3;
    uint16_t fixedDevice : 1;
    uint16_t removableMedia : 1;
    uint16_t retired2 : 7;
    uint16_t deviceType : 1;

    uint16_t numCylinders;

    uint16_t specificConfiguration;

    uint16_t numHeads;

    uint16_t retired3[2];

    uint16_t numSectorsPerTrack;

    uint16_t vendor0[3];

    uint16_t retired4[2];

    uint16_t obsolete0;

    uint8_t firmwareRev[8];

    uint8_t modelNumber[40];

    uint8_t maxBlockTransfer;
    uint8_t vendor1;

    uint16_t trustedFeatureSupported : 1;
    uint16_t reserved1 : 15;

#endif
    uint16_t todo0[60];

    uint32_t numLBA28Sectors;

    uint16_t todo1[21];

    uint16_t downloadMicrocodeSuported : 1;
    uint16_t dmsQueued : 1;
    uint16_t cfaReserved : 1;
    uint16_t apmSupported : 1;
    uint16_t msnSupported : 1;
    uint16_t puisSupported : 1;
    uint16_t manualPowerUpReq : 1;
    uint16_t reserved0 : 1;
    uint16_t setMax : 1;
    uint16_t accoustics : 1;
    uint16_t lba48Supported : 1;
    uint16_t devConfOverlay : 1;
    uint16_t flushCacheSupported : 1;
    uint16_t flushCacheExtSupported : 1;
    uint16_t setTo1 : 1;
    uint16_t setTo0 : 1;

    uint16_t todo2[16];

    uint64_t numLBA48Sectors;

    uint16_t todo3[152];

} __attribute__((__packed__)) atapio_dev_data_t;

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
#define ATAPIO_ASSERT(COND, MSG, ERROR) {                   \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the ATA PIO driver to the system.
 *
 * @details Attaches the ATA PIO driver to the system. This function will
 * use the FDT to initialize the ATA PIO and retreive the ATA PIO parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _atapioAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief ATA PIO VFS open hook.
 *
 * @details ATA PIO VFS open hook. This function returns a handle to control the
 * ATA PIO driver through VFS.
 *
 * @param[in, out] pDrvCtrl The ATA PIO driver that was registered in the VFS.
 * @param[in] kpPath The path in the ATA PIO driver mount point.
 * @param[in] flags The open flags, must be O_RDWR.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _atapioVfsOpen(void*       pDrvCtrl,
                            const char* kpPath,
                            int         flags,
                            int         mode);

/**
 * @brief ATA PIO VFS close hook.
 *
 * @details ATA PIO VFS close hook. This function closes a handle that was created
 * when calling the open function.
 *
 * @param[in, out] pDrvCtrl The ATA PIO driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _atapioVfsClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief ATA PIO VFS read hook.
 *
 * @details ATA PIO VFS read hook. This function read a string from the ATA PIO
 * volume.
 *
 * @param[in, out] pDrvCtrl The ATA PIO driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _atapioVfsRead(void*  pDrvCtrl,
                              void*  pHandle,
                              void*  pBuffer,
                              size_t count);

/**
 * @brief ATA PIO VFS write hook.
 *
 * @details ATA PIO VFS write hook. This function writes a string to the ATA PIO
 * volume.
 *
 * @param[in, out] pDrvCtrl The ATA PIO driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _atapioVfsWrite(void*       pDrvCtrl,
                               void*       pHandle,
                               const void* kpBuffer,
                               size_t      count);

/**
 * @brief ATA PIO VFS IOCTL hook.
 *
 * @details ATA PIO VFS IOCTL hook. This function performs the IOCTL for the
 * ATA PIO driver.
 *
 * @param[in, out] pDrvCtrl The ATA PIO driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _atapioVfsIOCTL(void*    pDriverData,
                               void*    pHandle,
                               uint32_t operation,
                               void*    pArgs);

/**
 * @brief ATA PIO VFS seek hook.
 *
 * @details ATA PIO VFS seek hook. This function performs a seek for the
 * ATA PIO driver.
 *
 * @param[in, out] pDrvCtrl The ATA PIO driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in, out] pArgs The arguments for the seek operation.
 *
 * @return The function returns the new offset from the beginning of the file on
 * success and -1 on error;
 */
static ssize_t _atapioVfsSeek(void*              pDriverData,
                              void*              pHandle,
                              seek_ioctl_args_t* pArgs);

/**
 * @brief ATA PIO VFS tell hook.
 *
 * @details ATA PIO VFS tell hook. This function performs a tell for the
 * ATA PIO driver.
 *
 * @param[in, out] pDrvCtrl The ATA PIO driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in, out] pArgs The arguments for the tell operation.
 *
 * @return The function returns the offset from the beginning of the file on
 * success and -1 on error;
 */
static ssize_t _atapioVfsTell(void* pDriverData,
                              void* pHandle,
                              void* pArgs);

/**
 * @brief Sets the LBA to the file descriptor.
 *
 * @details Sets the LBA to the file descriptor. The cursor is
 * positionned based on the sector size.
 *
 * @param[in, out] pDrvCtrl The ATA PIO driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in, out] lba The LBA value to set.
 */
static ssize_t _atapioSetLBA(atapio_ctrl_t* pCtrl,
                             atapio_fd_t*   pDesc,
                             uint64_t       lba);

/**
 * @brief Identifies a given ATA device if connected.
 *
 * @details Identify the ATA device given as parameter. The function will check
 * the presence of a device conected to the port pointed by the device argument.
 *
 * @param[in] pCtrl The controler for which the device is to be identified.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_ATA_DEVICE_ERROR is returned if an errored device is detected.
 * - OS_ERR_ATA_DEVICE_NOT_PRESENT is returned if the device was not detected.
 */
static OS_RETURN_E _atapioIdentify(atapio_ctrl_t* pCtrl);

/**
 * @brief ATA PIO 48bits LBA read prepare routine.
 *
 * @details ATA PIO 48bits LBA read prepare routine. This function sets the
 * registers to prepare a read from the disk. The parameters must be validated
 * before calling this function. The device must be locked when calling this
 * function.
 *
 * @param[in] pCtrl The ATA PIO driver that was registered in the VFS.
 * @param[in] sectorsToRead The number of sectors to read.
 * @param[in] sectorStart The first LBA sector to read.
 *
 * @return The function return the number of sectors prepared to read and -1 on
 * error.
 */
static int32_t _atapioPrepRead48(atapio_ctrl_t* pCtrl,
                                 uint16_t       sectorsToRead,
                                 uint64_t       sectorStart);
/**
 * @brief ATA PIO 28bits LBA read prepare routine.
 *
 * @details ATA PIO 28bits LBA read prepare routine. This function sets the
 * registers to prepare a read from the disk. The parameters must be validated
 * before calling this function. The device must be locked when calling this
 * function.
 *
 * @param[in] pCtrl The ATA PIO driver that was registered in the VFS.
 * @param[in] sectorsToRead The number of sectors to read.
 * @param[in] sectorStart The first LBA sector to read.
 *
 * @return The function return the number of sectors prepared to read and -1 on
 * error.
 */
static int32_t _atapioPrepRead28(atapio_ctrl_t* pCtrl,
                                 uint8_t        sectorsToRead,
                                 uint32_t       sectorStart);

/**
 * @brief ATA PIO 48bits LBA write prepare routine.
 *
 * @details ATA PIO 48bits LBA write prepare routine. This function sets the
 * registers to prepare a write to the disk. The parameters must be validated
 * before calling this function. The device must be locked when calling this
 * function.
 *
 * @param[in] pCtrl The ATA PIO driver that was registered in the VFS.
 * @param[in] sectorsToWrite The number of sectors to write.
 * @param[in] sectorStart The first LBA sector to write.
 *
 * @return The function return the number of sectors prepared to write and -1 on
 * error.
 */
static int32_t _atapioPrepWrite48(atapio_ctrl_t* pCtrl,
                                  uint16_t       sectorsToWrite,
                                  uint64_t       sectorStart);

/**
 * @brief ATA PIO 28bits LBA write prepare routine.
 *
 * @details ATA PIO 28bits LBA write prepare routine. This function sets the
 * registers to prepare a write to the disk. The parameters must be validated
 * before calling this function. The device must be locked when calling this
 * function.
 *
 * @param[in] pCtrl The ATA PIO driver that was registered in the VFS.
 * @param[in] sectorsToWrite The number of sectors to write.
 * @param[in] sectorStart The first LBA sector to write.
 *
 * @return The function return the number of sectors prepared to write and -1 on
 * error.
 */
static int32_t _atapioPrepWrite28(atapio_ctrl_t* pCtrl,
                                  uint8_t        sectorsToWrite,
                                  uint32_t       sectorStart);

/**
 * @brief Reads a sector from the disk.
 *
 * @details Reads a sector from the disk. The sector is not
 * checked and must be verified before calling this function. The device must be
 * locked when calling this function.
 *
 * @param[in] pCtrl The ATA PIO driver that was registered in the VFS.
 * @param[in] sector The sector to read.
 * @param[out] pBuffer The buffer to fill the data with.
 *
 * @return The function returns the number of bytes read or -1 on error.
 */
static int32_t _atapioReadSector(atapio_ctrl_t* pCtrl,
                                 uint64_t       sector,
                                 uint8_t*       pBuffer);

/**
 * @brief Flushes the disk cache.
 *
 * @details Flushed the disk cache after a write command.
 *
 * @param[in] pCtrl The ATA PIO driver that was registered in the VFS.
 *
 * @return The function returns the success or error status.
 */
static OS_RETURN_E _atapioFlush(atapio_ctrl_t* pCtrl);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief ATA PIO driver instance. */
static driver_t sATAPIODriver = {
    .pName         = "ATA PIO Driver",
    .pDescription  = "ATA PIO Driver roOs.",
    .pCompatible   = "x86,x86-atapio",
    .pVersion      = "2.0",
    .pDriverAttach = _atapioAttach
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _atapioAttach(const fdt_node_t* pkFdtNode)
{
    const uint32_t* kpUint32Prop;
    const char*     kpStrProp;
    size_t          propLen;
    atapio_ctrl_t*  pCtrl;
    OS_RETURN_E     retCode;
    OS_RETURN_E     error;
    bool_t          isMutexSet;

    isMutexSet = FALSE;
    retCode = OS_NO_ERR;

    /* Create the driver controller structure */
    pCtrl = kmalloc(sizeof(atapio_ctrl_t));
    if(pCtrl == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pCtrl, 0, sizeof(atapio_ctrl_t));

    retCode = mutexInit(&pCtrl->lock,
                        MUTEX_FLAG_QUEUING_PRIO | MUTEX_FLAG_PRIO_ELEVATION);
    if(retCode != OS_NO_ERR)
    {
        goto ATTACH_END;
    }
    isMutexSet = TRUE;

    /* Get the type */
    kpUint32Prop = fdtGetProp(pkFdtNode, ATAPIO_FDT_TYPE_PROP, &propLen);
    if(kpUint32Prop == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pCtrl->type = (ATA_PIO_TYPE_E)FDTTOCPU32(*kpUint32Prop);
    if(pCtrl->type != ATA_MASTER && pCtrl->type != ATA_SLAVE)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Get the comm port */
    kpUint32Prop = fdtGetProp(pkFdtNode, ATAPIO_FDT_COMM_PROP, &propLen);
    if(kpUint32Prop == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pCtrl->port = (uint16_t)FDTTOCPU32(*kpUint32Prop);
    /* Identify the disk */
    retCode = _atapioIdentify(pCtrl);
    if(retCode != OS_NO_ERR)
    {
        goto ATTACH_END;
    }

    /* Get the device path */
    kpStrProp = fdtGetProp(pkFdtNode, ATAPIO_FDT_DEVICE_PROP, &propLen);
    if(kpStrProp == NULL || propLen  == 0)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Register the driver */
    pCtrl->vfsDriver = vfsRegisterDriver(kpStrProp,
                                         pCtrl,
                                         _atapioVfsOpen,
                                         _atapioVfsClose,
                                         _atapioVfsRead,
                                         _atapioVfsWrite,
                                         NULL,
                                         _atapioVfsIOCTL);
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
            ATAPIO_ASSERT(error == OS_NO_ERR,
                          "Failed to destroy mutex",
                          error);
        }
        if(pCtrl != NULL)
        {
            if(pCtrl->vfsDriver != VFS_DRIVER_INVALID)
            {
                error = vfsUnregisterDriver(&pCtrl->vfsDriver);
                ATAPIO_ASSERT(error == OS_NO_ERR,
                              "Failed to unregister VFS driver",
                              error);
            }

            kfree(pCtrl);
        }
    }

    return retCode;
}

static void* _atapioVfsOpen(void*       pDrvCtrl,
                            const char* kpPath,
                            int         flags,
                            int         mode)
{
    atapio_fd_t* pDesc;

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


    pDesc = kmalloc(sizeof(atapio_fd_t));
    if(pDesc == NULL)
    {
        return (void*)-1;
    }
    memset(pDesc, 0, sizeof(atapio_fd_t));

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

static int32_t _atapioVfsClose(void* pDrvCtrl, void* pHandle)
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

static ssize_t _atapioVfsRead(void*  pDrvCtrl,
                              void*  pHandle,
                              void*  pBuffer,
                              size_t count)
{
    atapio_fd_t*   pDesc;
    atapio_ctrl_t* pCtrl;
    OS_RETURN_E    error;
    uint64_t       maxSector;
    uint32_t       maxSectorBurst;
    uint64_t       sectorStart;
    uint64_t       sectorEnd;
    uint16_t       sectorsToRead;
    size_t         readVal;
    uint16_t       toRead;
    uint16_t       data;
    uint32_t       i;
    uint32_t       j;
    uint8_t        status;
    int32_t        prepResult;
    uint8_t*       pUintBuff;

    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    if(count == 0)
    {
        return 0;
    }

    pCtrl = pDrvCtrl;
    pDesc = pHandle;

    /* Check that the sector is valid */
    if(pDesc->offset >= pCtrl->size)
    {
        return 0;
    }

    error = mutexLock(&pCtrl->lock);
    if(error != OS_NO_ERR)
    {
        return -1;
    }

    pUintBuff = pBuffer;

    if(pCtrl->supportLBA48 == TRUE)
    {
        maxSector = 0x0000FFFFFFFFFFFF;
        maxSectorBurst = UINT16_MAX;
    }
    else
    {
        maxSector = 0x0FFFFFFF;
        maxSectorBurst = UINT16_MAX;
    }

    /* Check the sectors */
    sectorStart = pDesc->offset / ATA_PIO_SECTOR_SIZE;
    sectorEnd = (pDesc->offset + count) / ATA_PIO_SECTOR_SIZE;

    /* Remove last sector if the count is round */
    if((pDesc->offset + count) % ATA_PIO_SECTOR_SIZE == 0)
    {
        --sectorEnd;
    }

    /* Check start sector */
    if(sectorStart > maxSector)
    {
        return 0;
    }

    /* Check end sector */
    if(sectorEnd > maxSector)
    {
        sectorEnd = maxSector;
    }

    /* Read the sectors */
    readVal = pDesc->offset;
    data = 0;
    while(sectorStart <= sectorEnd)
    {
        /* Get the number of sectors to read */
        sectorsToRead = MIN(maxSectorBurst, (sectorEnd - sectorStart) + 1);

        if(pCtrl->supportLBA48 == TRUE)
        {
            prepResult = _atapioPrepRead48(pCtrl, sectorsToRead, sectorStart);
        }
        else
        {
            prepResult = _atapioPrepRead28(pCtrl, sectorsToRead, sectorStart);
        }

        if(prepResult <= 0)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failure preparing for reading disk");

            error = mutexUnlock(&pCtrl->lock);
            ATAPIO_ASSERT(error == OS_NO_ERR,
                          "Failed to unlock mutex",
                          error);
            return pDesc->offset - readVal;
        }

        /* Read the sectors */
        for(i = 0; i < sectorsToRead; ++i)
        {
            /* Wait until device is ready */
            status = _cpuInB(pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);
            if(status == 0x00)
            {
                syslog(SYSLOG_LEVEL_ERROR,
                       MODULE_NAME,
                       "Failure while reading disk");

                error = mutexUnlock(&pCtrl->lock);
                ATAPIO_ASSERT(error == OS_NO_ERR,
                              "Failed to unlock mutex",
                              error);

                return pDesc->offset - readVal;
            }
            while(((status & ATA_PIO_FLAG_BUSY) == ATA_PIO_FLAG_BUSY) &&
                  ((status & ATA_PIO_FLAG_ERR) != ATA_PIO_FLAG_ERR))
            {
                status = _cpuInB(pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);
            }
            if((status & ATA_PIO_FLAG_ERR) == ATA_PIO_FLAG_ERR)
            {
                syslog(SYSLOG_LEVEL_ERROR,
                       MODULE_NAME,
                       "Failure while reading disk");

                error = mutexUnlock(&pCtrl->lock);
                ATAPIO_ASSERT(error == OS_NO_ERR,
                              "Failed to unlock mutex",
                              error);
                return pDesc->offset - readVal;
            }

            toRead = ATA_PIO_SECTOR_SIZE;
            if(count >= 1)
            {
                /* Discard the first offset */
                for(j = 0; j < pDesc->offset % ATA_PIO_SECTOR_SIZE; j += 2)
                {
                    data = _cpuInW(pCtrl->port + ATA_PIO_DATA_PORT_OFFSET);
                    toRead -= sizeof(uint16_t);
                }

                /* If the last read word contains needed data */
                if(pDesc->offset % 2 != 0)
                {
                    *pUintBuff = (uint8_t)data;

                    pUintBuff += sizeof(uint8_t);
                    --count;

                    ++pDesc->offset;
                }

                /* Read the data to the buffer */
                while(count > 1 && toRead > 0)
                {
                    *((uint16_t*)pUintBuff) =
                        _cpuInW(pCtrl->port + ATA_PIO_DATA_PORT_OFFSET);

                    pUintBuff += sizeof(uint16_t);
                    count -= sizeof(uint16_t);
                    toRead -= sizeof(uint16_t);

                    pDesc->offset += sizeof(uint16_t);
                }

                /* Check if we should read one more byte */
                if(count == 1 && toRead > 0)
                {
                    *pUintBuff = (uint8_t)
                        _cpuInW(pCtrl->port + ATA_PIO_DATA_PORT_OFFSET);

                    --count;
                    pUintBuff += sizeof(uint8_t);
                    toRead += sizeof(uint16_t);

                    ++pDesc->offset;
                }
            }

            /* Check if we still have data in the buffer */
            while(toRead > 0)
            {
                (void)_cpuInW(pCtrl->port + ATA_PIO_DATA_PORT_OFFSET);

                toRead -= sizeof(uint16_t);
            }
        }

        sectorStart += sectorsToRead;
    }

    error = mutexUnlock(&pCtrl->lock);
    ATAPIO_ASSERT(error == OS_NO_ERR, "Failed to unlock mutex", error);

    return pDesc->offset - readVal;
}

static ssize_t _atapioVfsWrite(void*       pDrvCtrl,
                               void*       pHandle,
                               const void* kpBuffer,
                               size_t      count)
{
    atapio_fd_t*   pDesc;
    atapio_ctrl_t* pCtrl;
    OS_RETURN_E    error;
    uint64_t       maxSector;
    uint32_t       maxSectorBurst;
    uint64_t       sectorStart;
    uint64_t       sectorEnd;
    uint16_t       sectorsToWrite;
    size_t         writeVal;
    uint32_t       i;
    uint32_t       j;
    int32_t        retValue;
    uint8_t        status;
    bool_t         partialStart;
    bool_t         partialEnd;
    uint64_t       diff;
    int32_t        prepResult;
    const uint8_t* kpUintBuff;

    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    if(count == 0)
    {
        return 0;
    }

    pCtrl = pDrvCtrl;
    pDesc = pHandle;

    if(pDesc->isReadOnly == TRUE)
    {
        return -1;
    }

    /* Check that the sector is valid */
    if(pDesc->offset >= pCtrl->size)
    {
        return 0;
    }

    error = mutexLock(&pCtrl->lock);
    if(error != OS_NO_ERR)
    {
        return -1;
    }

    kpUintBuff = (const uint8_t*)kpBuffer;

    if(pCtrl->supportLBA48 == TRUE)
    {
        maxSector = 0x0000FFFFFFFFFFFF;
        maxSectorBurst = UINT16_MAX;
    }
    else
    {
        maxSector = 0x0FFFFFFF;
        maxSectorBurst = UINT16_MAX;
    }

    /* Check the sectors */
    sectorStart = pDesc->offset / ATA_PIO_SECTOR_SIZE;
    sectorEnd = (pDesc->offset + count) / ATA_PIO_SECTOR_SIZE;

    /* Remove last sector if the count is round */
    if((pDesc->offset + count) % ATA_PIO_SECTOR_SIZE == 0)
    {
        --sectorEnd;
    }

    /* Check start sector */
    if(sectorStart > maxSector)
    {
        return 0;
    }

    /* Check end sector */
    if(sectorEnd > maxSector)
    {
        diff = (pDesc->offset + count) % ATA_PIO_SECTOR_SIZE;
        /* Remove potential partial */
        if(diff != 0)
        {
            count -= diff;
            --sectorEnd;
        }
        diff = sectorEnd - maxSector;
        sectorEnd -= diff;
        count -= diff * ATA_PIO_SECTOR_SIZE;
    }

    /* If the file descriptor has an offset in the sector, we must first
     * read it.
     */
    if(pDesc->offset % ATA_PIO_SECTOR_SIZE != 0)
    {
        retValue = _atapioReadSector(pCtrl,
                                     sectorStart,
                                     pCtrl->pBufferSectors[0]);
        if(retValue != ATA_PIO_SECTOR_SIZE)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                    MODULE_NAME,
                    "Failure while reading disk");

            error = mutexUnlock(&pCtrl->lock);
            ATAPIO_ASSERT(error == OS_NO_ERR,
                          "Failed to unlock mutex",
                          error);
            return -1;
        }
        partialStart = TRUE;

        diff = MIN(count,
                   ATA_PIO_SECTOR_SIZE - (pDesc->offset % ATA_PIO_SECTOR_SIZE));

        /* Partial copy */
        memcpy(pCtrl->pBufferSectors[0] + pDesc->offset, kpUintBuff, diff);

        kpUintBuff += diff;
    }
    else
    {
        partialStart = FALSE;
    }

    /* If the last sector will not be fully overwritten, we need to read
     * it.
     */
    if(sectorStart != sectorEnd)
    {
        diff = (pDesc->offset + count) % ATA_PIO_SECTOR_SIZE;
        if(diff != 0)
        {
            retValue = _atapioReadSector(pCtrl,
                                         sectorEnd,
                                         pCtrl->pBufferSectors[1]);
            if(retValue != ATA_PIO_SECTOR_SIZE)
            {
                syslog(SYSLOG_LEVEL_ERROR,
                       MODULE_NAME,
                       "Failure while reading disk");
                error = mutexUnlock(&pCtrl->lock);
                ATAPIO_ASSERT(error == OS_NO_ERR,
                              "Failed to unlock mutex",
                              error);
                return -1;
            }

            partialEnd = TRUE;

            /* Partial copy */
            memcpy(pCtrl->pBufferSectors[1], kpUintBuff + count - diff, diff);
        }
        else
        {
            partialEnd = FALSE;
        }
    }
    else
    {
        partialEnd = FALSE;
    }

    /* Write the sectors */
    writeVal = pDesc->offset;
    while(sectorStart <= sectorEnd)
    {
        /* Get the number of sectors to write */
        sectorsToWrite = MIN(maxSectorBurst, (sectorEnd - sectorStart) + 1);

        if(pCtrl->supportLBA48 == TRUE)
        {
            prepResult = _atapioPrepWrite48(pCtrl, sectorsToWrite, sectorStart);
        }
        else
        {
            prepResult = _atapioPrepWrite28(pCtrl, sectorsToWrite, sectorStart);
        }
        if(prepResult <= 0)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failure preparing for writing disk");

            error = mutexUnlock(&pCtrl->lock);
            ATAPIO_ASSERT(error == OS_NO_ERR,
                          "Failed to unlock mutex",
                          error);

            return pDesc->offset - writeVal;
        }

        /* Write the sectors */
        for(i = 0; i < sectorsToWrite; ++i)
        {
            /* Wait until device is ready */
            status = _cpuInB(pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);
            if(status == 0x00)
            {
                syslog(SYSLOG_LEVEL_ERROR,
                       MODULE_NAME,
                       "Failure while writing disk");

                error = mutexUnlock(&pCtrl->lock);
                ATAPIO_ASSERT(error == OS_NO_ERR,
                              "Failed to unlock mutex",
                              error);

                return pDesc->offset - writeVal;
            }
            while(((status & ATA_PIO_FLAG_BUSY) == ATA_PIO_FLAG_BUSY) &&
                  ((status & ATA_PIO_FLAG_ERR) != ATA_PIO_FLAG_ERR))
            {
                status = _cpuInB(pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);
            }
            if((status & ATA_PIO_FLAG_ERR) == ATA_PIO_FLAG_ERR)
            {
                syslog(SYSLOG_LEVEL_ERROR,
                       MODULE_NAME,
                       "Failure while writing disk");

                error = mutexUnlock(&pCtrl->lock);
                ATAPIO_ASSERT(error == OS_NO_ERR,
                              "Failed to unlock mutex",
                              error);

                return pDesc->offset - writeVal;
            }

            /* Check if we need to write the partial start */
            if(partialStart == TRUE)
            {
                for(j = 0; j < ATA_PIO_SECTOR_SIZE; j += 2)
                {
                    _cpuOutW(*(uint16_t*)(pCtrl->pBufferSectors[0] + j),
                             pCtrl->port + ATA_PIO_DATA_PORT_OFFSET);
                }

                diff = MIN(count,
                           ATA_PIO_SECTOR_SIZE -
                           (pDesc->offset % ATA_PIO_SECTOR_SIZE));
                pDesc->offset += diff;
                count -= diff;
                partialStart = FALSE;
            }
            /* Check if this is the last sector and we have a partial end */
            else if(sectorStart == sectorEnd && partialEnd == TRUE)
            {
                for(j = 0; j < ATA_PIO_SECTOR_SIZE; j += 2)
                {
                    _cpuOutW(*(uint16_t*)(pCtrl->pBufferSectors[1] + j),
                             pCtrl->port + ATA_PIO_DATA_PORT_OFFSET);
                }

                pDesc->offset += count;
                count = 0;
                partialEnd = FALSE;
            }
            else
            {
                for(j = 0; j < ATA_PIO_SECTOR_SIZE; j += 2)
                {
                    _cpuOutW(*(uint16_t*)(kpUintBuff + j),
                             pCtrl->port + ATA_PIO_DATA_PORT_OFFSET);
                }

                kpUintBuff += ATA_PIO_SECTOR_SIZE;
                pDesc->offset += ATA_PIO_SECTOR_SIZE;
                count -= ATA_PIO_SECTOR_SIZE;
            }


            error = _atapioFlush(pCtrl);
            if(error != OS_NO_ERR)
            {
                syslog(SYSLOG_LEVEL_ERROR,
                       MODULE_NAME,
                       "Failure while flushing disk");

                error = mutexUnlock(&pCtrl->lock);
                ATAPIO_ASSERT(error == OS_NO_ERR,
                              "Failed to unlock mutex",
                              error);

                return pDesc->offset - writeVal;
            }

        }

        sectorStart += sectorsToWrite;
    }


    error = mutexUnlock(&pCtrl->lock);
    ATAPIO_ASSERT(error == OS_NO_ERR, "Failed to unlock mutex", error);

    return pDesc->offset - writeVal;
}

static ssize_t _atapioVfsIOCTL(void*    pDriverData,
                               void*    pHandle,
                               uint32_t operation,
                               void*    pArgs)
{
    ssize_t retVal;

    switch(operation)
    {
        case VFS_IOCTL_FILE_SEEK:
            retVal = _atapioVfsSeek(pDriverData, pHandle, pArgs);
            break;
        case VFS_IOCTL_DEV_GET_SECTOR_SIZE:
            retVal = ATA_PIO_SECTOR_SIZE;
            break;
        case VFS_IOCTL_DEV_SET_LBA:
            retVal = _atapioSetLBA(pDriverData, pHandle, *(uint64_t*)pArgs);
            break;
        case VFS_IOCTL_FILE_TELL:
            retVal = _atapioVfsTell(pDriverData, pHandle, pArgs);
            break;
        case VFS_IOCTL_DEV_FLUSH:
            if(_atapioFlush(pDriverData) == OS_NO_ERR)
            {
                retVal = 0;
            }
            else
            {
                retVal = -1;
            }
            break;
        default:
            retVal = -1;
    }

    return retVal;
}

static ssize_t _atapioVfsSeek(void*              pDriverData,
                              void*              pHandle,
                              seek_ioctl_args_t* pArgs)
{
    atapio_fd_t*   pDesc;
    atapio_ctrl_t* pCtrl;

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

static ssize_t _atapioVfsTell(void* pDriverData,
                              void* pHandle,
                              void* pArgs)
{
    atapio_fd_t* pDesc;

    (void)pArgs;
    (void)pDriverData;

    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    pDesc = pHandle;

    return pDesc->offset;
}

static ssize_t _atapioSetLBA(atapio_ctrl_t* pCtrl,
                             atapio_fd_t*   pDesc,
                             uint64_t       lba)
{
    if(pDesc == NULL || pDesc == (void*)-1)
    {
        return -1;
    }

    if(pCtrl->size / ATA_PIO_SECTOR_SIZE < lba)
    {
        return -1;
    }

    pDesc->offset = ATA_PIO_SECTOR_SIZE * lba;

    return pDesc->offset;
}

static OS_RETURN_E _atapioIdentify(atapio_ctrl_t* pCtrl)
{
    uint8_t           status;
    uint16_t          i;
    uint16_t          data;
    atapio_dev_data_t devData;

#if ATA_PIO_DEBUG_ENABLED
#endif

#if ATA_PIO_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Identify ATA 0x%x on %s",
           pCtrl->port,
           ((pCtrl->type == ATA_MASTER) ? "MASTER" : "SLAVE"));
#endif

    /* Select slave or master */
    _cpuOutB(pCtrl->type == ATA_MASTER ? 0xA0 : 0xB0,
             pCtrl->port + ATA_PIO_DEVICE_PORT_OFFSET);

    /* Check is the device is connected */
    _cpuOutB(0x00, pCtrl->port + ATA_PIO_CONTROL_PORT_OFFSET);

    status = _cpuInB(pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);
    if(status == 0xFF)
    {
#if ATA_PIO_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "ATA 0x%x on %s not present",
               pCtrl->port,
               ((pCtrl->type == ATA_MASTER) ? "MASTER" : "SLAVE"));
#endif
        return OS_ERR_NO_SUCH_ID;
    }

    /* Select slave or master */
    _cpuOutB(pCtrl->type == ATA_MASTER ? 0xA0 : 0xB0,
             pCtrl->port + ATA_PIO_DEVICE_PORT_OFFSET);

    /* Write 0 to registers */
    _cpuOutB(0x00, pCtrl->port + ATA_PIO_SC_PORT_OFFSET);
    _cpuOutB(0x00, pCtrl->port + ATA_PIO_LBALOW_PORT_OFFSET);
    _cpuOutB(0x00, pCtrl->port + ATA_PIO_LBAMID_PORT_OFFSET);
    _cpuOutB(0x00, pCtrl->port + ATA_PIO_LBAHIGH_PORT_OFFSET);

    /* Send the identify command */
    _cpuOutB(ATA_PIO_IDENTIFY_COMMAND,
             pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);

    /* Get the IDENTIFY status */
    status = _cpuInB(pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);
    if(status == 0x00)
    {
#if ATA_PIO_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "ATA 0x%x on %s cannot identify",
               pCtrl->port,
               ((pCtrl->type == ATA_MASTER) ? "MASTER" : "SLAVE"));
#endif
        return OS_ERR_NO_SUCH_ID;
    }

    /* Wait until device is ready */
    while(((status & ATA_PIO_FLAG_BUSY) == ATA_PIO_FLAG_BUSY) &&
          ((status & ATA_PIO_FLAG_ERR) != ATA_PIO_FLAG_ERR))
    {
        status = _cpuInB(pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);
    }

    if((status & ATA_PIO_FLAG_ERR) == ATA_PIO_FLAG_ERR)
    {
#if ATA_PIO_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "ATA 0x%x on %s error: 0x%x",
               pCtrl->port,
               ((pCtrl->type == ATA_MASTER) ? "MASTER" : "SLAVE"),
               status);
#endif
        return OS_ERR_INCORRECT_VALUE;
    }

    /* The device data information is now ready to be read */
    memset(&devData, 0, sizeof(atapio_dev_data_t));
    for(i = 0; i < 256; ++i)
    {
        data = _cpuInW(pCtrl->port + ATA_PIO_DATA_PORT_OFFSET);
        *(((uint16_t*)&devData) + i) = data;
    }

    pCtrl->supportLBA48 = devData.lba48Supported;

    if(pCtrl->supportLBA48 == TRUE)
    {
        pCtrl->size = (size_t)devData.numLBA48Sectors * ATA_PIO_SECTOR_SIZE;
    }
    else
    {
        pCtrl->size = (size_t)devData.numLBA28Sectors * ATA_PIO_SECTOR_SIZE;
    }

#if ATA_PIO_DEBUG_ENABLED

    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "ATA 0x%x on %s size: %dB",
           pCtrl->port,
           ((pCtrl->type == ATA_MASTER) ? "MASTER" : "SLAVE"),
           pCtrl->size);
#endif

    return OS_NO_ERR;
}

static int32_t _atapioPrepRead48(atapio_ctrl_t* pCtrl,
                                 uint16_t       sectorsToRead,
                                 uint64_t       sectorStart)
{
    /* Set sector to read */
    _cpuOutB((pCtrl->type == ATA_MASTER ? 0x40 : 0x50),
             pCtrl->port + ATA_PIO_DEVICE_PORT_OFFSET);

    /* Send the number of sectors to read (high) */
    _cpuOutB(sectorsToRead >> 8, pCtrl->port + ATA_PIO_SC_PORT_OFFSET);

    /* Send LBA 4, 5, 6 */
    _cpuOutB((sectorStart >> 24) & 0xFF,
             pCtrl->port + ATA_PIO_LBALOW_PORT_OFFSET);
    _cpuOutB((sectorStart >> 32) & 0xFF,
             pCtrl->port + ATA_PIO_LBAMID_PORT_OFFSET);
    _cpuOutB((sectorStart >> 36) & 0xFF,
             pCtrl->port + ATA_PIO_LBAHIGH_PORT_OFFSET);

    /* Send the number of sectors to read (low) */
    _cpuOutB(sectorsToRead & 0xFF, pCtrl->port + ATA_PIO_SC_PORT_OFFSET);

    /* Send LBA 1, 2, 3 */
    _cpuOutB(sectorStart & 0xFF,
             pCtrl->port + ATA_PIO_LBALOW_PORT_OFFSET);
    _cpuOutB((sectorStart >> 8) & 0xFF,
             pCtrl->port + ATA_PIO_LBAMID_PORT_OFFSET);
    _cpuOutB((sectorStart >> 16) & 0xFF,
             pCtrl->port + ATA_PIO_LBAHIGH_PORT_OFFSET);

    /* Send Read sector command */
    _cpuOutB(ATA_PIO_READ_SECTORS48_COMMAND,
             pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);

    return (int32_t)sectorsToRead;
}

static int32_t _atapioPrepRead28(atapio_ctrl_t* pCtrl,
                                 uint8_t        sectorsToRead,
                                 uint32_t       sectorStart)
{
    /* Set sector to read */
    _cpuOutB((pCtrl->type == ATA_MASTER ? 0xE0 : 0xF0) |
             ((sectorStart >> 24) & 0x0F),
             pCtrl->port + ATA_PIO_DEVICE_PORT_OFFSET);

    /* Send the number of sectors to read */
    _cpuOutB(sectorsToRead, pCtrl->port + ATA_PIO_SC_PORT_OFFSET);

    /* Send LBA 1, 2, 3 */
    _cpuOutB(sectorStart & 0xFF, pCtrl->port + ATA_PIO_LBALOW_PORT_OFFSET);
    _cpuOutB((sectorStart >> 8) & 0xFF,
             pCtrl->port + ATA_PIO_LBAMID_PORT_OFFSET);
    _cpuOutB((sectorStart >> 16) & 0xFF,
             pCtrl->port + ATA_PIO_LBAHIGH_PORT_OFFSET);

    /* Send Read sector command */
    _cpuOutB(ATA_PIO_READ_SECTORS28_COMMAND,
             pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);

    return (int32_t)sectorsToRead;
}

static int32_t _atapioPrepWrite48(atapio_ctrl_t* pCtrl,
                                  uint16_t       sectorsToWrite,
                                  uint64_t       sectorStart)
{
    /* Set sector to write */
    _cpuOutB((pCtrl->type == ATA_MASTER ? 0x40 : 0x50),
             pCtrl->port + ATA_PIO_DEVICE_PORT_OFFSET);

    /* Send the number of sectors to write (high) */
    _cpuOutB(sectorsToWrite >> 8, pCtrl->port + ATA_PIO_SC_PORT_OFFSET);

    /* Send LBA 4, 5, 6 */
    _cpuOutB((sectorStart >> 24) & 0xFF,
             pCtrl->port + ATA_PIO_LBALOW_PORT_OFFSET);
    _cpuOutB((sectorStart >> 32) & 0xFF,
             pCtrl->port + ATA_PIO_LBAMID_PORT_OFFSET);
    _cpuOutB((sectorStart >> 36) & 0xFF,
             pCtrl->port + ATA_PIO_LBAHIGH_PORT_OFFSET);

    /* Send the number of sectors to write (low) */
    _cpuOutB(sectorsToWrite & 0xFF, pCtrl->port + ATA_PIO_SC_PORT_OFFSET);

    /* Send LBA 1, 2, 3 */
    _cpuOutB(sectorStart & 0xFF,
             pCtrl->port + ATA_PIO_LBALOW_PORT_OFFSET);
    _cpuOutB((sectorStart >> 8) & 0xFF,
             pCtrl->port + ATA_PIO_LBAMID_PORT_OFFSET);
    _cpuOutB((sectorStart >> 16) & 0xFF,
             pCtrl->port + ATA_PIO_LBAHIGH_PORT_OFFSET);

    /* Send Write sector command */
    _cpuOutB(ATA_PIO_WRITE_SECTORS48_COMMAND,
                pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);

    return (int32_t)sectorsToWrite;
}

static int32_t _atapioPrepWrite28(atapio_ctrl_t* pCtrl,
                                  uint8_t        sectorsToWrite,
                                  uint32_t       sectorStart)
{
    /* Set sector to read */
    _cpuOutB((pCtrl->type == ATA_MASTER ? 0xE0 : 0xF0) |
             ((sectorStart >> 24) & 0x0F),
             pCtrl->port + ATA_PIO_DEVICE_PORT_OFFSET);

    /* Send the number of sectors to read */
    _cpuOutB(sectorsToWrite, pCtrl->port + ATA_PIO_SC_PORT_OFFSET);

    /* Send LBA 1, 2, 3 */
    _cpuOutB(sectorStart & 0xFF, pCtrl->port + ATA_PIO_LBALOW_PORT_OFFSET);
    _cpuOutB((sectorStart >> 8) & 0xFF,
             pCtrl->port + ATA_PIO_LBAMID_PORT_OFFSET);
    _cpuOutB((sectorStart >> 16) & 0xFF,
             pCtrl->port + ATA_PIO_LBAHIGH_PORT_OFFSET);

    /* Send Read sector command */
    _cpuOutB(ATA_PIO_WRITE_SECTORS28_COMMAND,
             pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);

    return (int32_t)sectorsToWrite;
}

static int32_t _atapioReadSector(atapio_ctrl_t* pCtrl,
                                 uint64_t       sector,
                                 uint8_t*       pBuffer)
{
    int32_t  prepResult;
    uint8_t  status;
    uint32_t i;

    if(pCtrl->supportLBA48 == TRUE)
    {
        prepResult = _atapioPrepRead48(pCtrl, 1, sector);
    }
    else
    {
        prepResult = _atapioPrepRead28(pCtrl, 1, sector);
    }

    if(prepResult <= 0)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failure preparing for reading disk");

        return -1;
    }

    /* Wait until device is ready */
    status = _cpuInB(pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);
    if(status == 0x00)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failure while reading disk");
        return -1;
    }
    while(((status & ATA_PIO_FLAG_BUSY) == ATA_PIO_FLAG_BUSY) &&
          ((status & ATA_PIO_FLAG_ERR) != ATA_PIO_FLAG_ERR))
    {
        status = _cpuInB(pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);
    }
    if((status & ATA_PIO_FLAG_ERR) == ATA_PIO_FLAG_ERR)
    {
        syslog(SYSLOG_LEVEL_ERROR,
                MODULE_NAME,
                "Failure while reading disk");
        return -1;
    }

    /* Read sector */
    for(i = 0; i < ATA_PIO_SECTOR_SIZE; i += 2)
    {
        *((uint16_t*)pBuffer) = _cpuInW(pCtrl->port + ATA_PIO_DATA_PORT_OFFSET);
        pBuffer += sizeof(uint16_t);
    }

    return ATA_PIO_SECTOR_SIZE;
}

static OS_RETURN_E _atapioFlush(atapio_ctrl_t* pCtrl)
{
    uint8_t status;

    if(pCtrl->supportLBA48 == TRUE)
    {
        _cpuOutB(pCtrl->type == ATA_MASTER ? 0x40 : 0x50,
                 pCtrl->port + ATA_PIO_DEVICE_PORT_OFFSET);
    }
    else
    {
        _cpuOutB(pCtrl->type == ATA_MASTER ? 0xE0 : 0xF0,
                 pCtrl->port + ATA_PIO_DEVICE_PORT_OFFSET);
    }

    /* Issue flush */
    _cpuOutB(ATA_PIO_FLUSH_SECTOR_COMMAND,
             pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);


    /* Wait until device is ready */
    status = _cpuInB(pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);
    if(status == 0x00)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failure while flushing disk");
        return OS_ERR_INCORRECT_VALUE;
    }
    while(((status & ATA_PIO_FLAG_BUSY) == ATA_PIO_FLAG_BUSY) &&
          ((status & ATA_PIO_FLAG_ERR) != ATA_PIO_FLAG_ERR))
    {
        status = _cpuInB(pCtrl->port + ATA_PIO_COMMAND_PORT_OFFSET);
    }
    if((status & ATA_PIO_FLAG_ERR) == ATA_PIO_FLAG_ERR)
    {
        syslog(SYSLOG_LEVEL_ERROR,
                MODULE_NAME,
                "Failure while flushing disk");
        return OS_ERR_INCORRECT_VALUE;
    }

    return OS_NO_ERR;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sATAPIODriver);

/************************************ EOF *************************************/