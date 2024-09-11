/*******************************************************************************
 * @file diskmanager.c
 *
 * @see diskmanager.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 25/07/2024
 *
 * @version 1.0
 *
 * @brief Disk manager module.
 *
 * @details Disk manager module for roOs. The disk manager detects the presence
 * of medias in /dev/storage and tries to detect viable partitions. The disk
 * manager can be added partition table drivers to detect more systems.
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
#include <stdlib.h>       /* Standard lib */
#include <stdint.h>       /* Standard int definitions */
#include <string.h>       /* String manipulation */
#include <kerror.h>       /* Kernel errors */
#include <syslog.h>       /* Syslog service */

/* Configuration files */
#include <config.h>

/* Header file */
#include <diskmanager.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "DSKMGR"

/**
 * @brief Defines the path where the storage device should be registered to
 * be probed.
 */
#define DSKMGR_STORAGE_ROOT_PATH "/dev/storage"

/** @brief Defines the GPT header signature */
#define GPT_SIGNATURE 0x5452415020494645ULL

/** @brief Maximum accepted length for a partition name */
#define GPT_MAX_PART_NAME_LEN 456

/** @brief GPT partition attribute: bootable */
#define GPT_ATTRIBUTE_BOOTABLE 0x4

/** @brief GPT start CRC value */
#define GPT_CRC_START_VAL 0xFFFFFFFF

/** @brief Defines the MBR signature. */
#define MBR_SIGNATURE 0xAA55

/** @brief Maximal number of partitions in an MBR table */
#define MBR_PARTITION_COUNT 4

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the MBR partition structure. */
typedef struct
{
    /** @brief Drive attributes */
    uint8_t active;
    /** @brief Head Address of partition start */
    uint8_t startHead;
    /** @brief Cylinder Address of partition start */
    uint16_t startSectorCylinder;
    /** @brief Partition Type */
    uint8_t sysId;
    /** @brief Head Address of partition end */
    uint8_t endHead;
    /** @brief Cylinder Address of partition end */
    uint16_t endSectorCylinder;
    /** @brief LBA of partition start */
    uint32_t lbaStart;
    /** @brief Number of sectors in the partition */
    uint32_t size;
} __attribute__((__packed__)) partition_table_t;

/** @brief Defines the MBR sector structure. */
typedef struct
{
    /** @brief Boostrap code and data */
    uint8_t bootstrapCode[440];

    /** @brief Unique disk ID */
    uint32_t diskId;

    /** @brief Reserved */
    uint16_t reserved;

    /** @brief Partition Table Entries */
    partition_table_t partitions[4];

    /** @brief Valid bootsector signature */
    uint16_t signature;
} __attribute__((__packed__)) mbr_data_t;

/** @brief Defines the GPT Partition Table Header structure. */
typedef struct
{
    /** @brief GPT Signature */
    uint64_t signature;

    /** @brief GPT 	Revision number of header */
    uint32_t revision;

    /** @brief GPT Header size in little endian */
    uint32_t headerSize;

    /** @brief GPT CRC32 of header (offset +0 to +0x5b) in little endian, with
     * this field zeroed during calculation */
    uint32_t crc32Header;

    /** @brief Reserved; must be zero */
    uint32_t reserved0;

    /** @brief GPT Current LBA */
    uint64_t currentLBA;

    /** @brief GPT Backup LBA */
    uint64_t alternateHeaderLBA;

    /** @brief GPT 	First usable LBA  */
    uint64_t firstUsableBlock;

    /** @brief GPT Last usable LBA  */
    uint64_t lastUsableBlock;

    /** @brief GPT Disk GUID in mixed endian */
    uint8_t guid[16];

    /** @brief GPT Starting LBA of array of partition entries */
    uint64_t partitionArrayLBA;

    /** @brief GPT Number of partition entries in array */
    uint32_t partitionCount;

    /** @brief GPT Size of a single partition entry */
    uint32_t partitionArrayEntrySize;

    /** @brief GPT CRC32 of partition entries array in little endian */
    uint32_t crc32Array;

    /** @brief Reserved; must be zero */
    uint8_t reserved1[420];
} __attribute__((__packed__)) gpt_table_header_t;

/** @brief Defines the GPT Partition Table Entry structure. */
typedef struct
{
    /** @brief GPT Partition Type GUID in mixed endian */
    uint8_t typeGUID[16];

    /** @brief GPT Partition Unique GUID in mixed endian */
    uint8_t uniqueGUID[16];

    /** @brief Partition's starting LBA. */
    uint64_t startingLBA;

    /** @brief Partition's ending LBA. */
    uint64_t endingLBA;

    /** @brief Partition's attributes */
    uint64_t attributes;

    /** @brief Partition's name */
    uint8_t name[GPT_MAX_PART_NAME_LEN];
} __attribute__((__packed__)) gpt_table_entry_t;

/** @brief Defines the partition driver partition structure. */
typedef struct
{
    /** @brief Sector count */
    uint32_t sectorCount;

    /** @brief LBA Start address */
    uint32_t lbaStart;

    /** @brief Filesystem Type */
    uint8_t type;

    /** @brief Bootable partition flags */
    bool_t active;

    /** @brief Partition's disk base path */
    const char* pDiskPath;

    /** @brief Partition associated driver */
    vfs_driver_t pDriver;

    /** @brief Partition lock */
    kernel_spinlock_t lock;
} partition_t;

/** @brief Disk manager file descriptor, stores the partition on which a file
 * is located.
 */
typedef struct
{
    /** @brief Associated partition */
    partition_t* pPart;

    /** @brief Associated partition file descriptor */
    int32_t partFd;

    /** @brief Start position of the partition associated to the descriptor */
    uint64_t partitionStart;

    /** @brief Absolute position of the cursor in the partition in bytes */
    uint64_t posByte;

    /** @brief Size of the partition in bytes */
    uint64_t sizeByte;
} dskmgr_desc_t;

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
#define DSKMGR_ASSERT(COND, MSG, ERROR) {                   \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Detects partition in a given path.
 *
 * @details Detects partition in a given path recursively. The function will
 * create the detected partition device on the same folder where they were
 * detected adding a suffix.
 *
 * @param[in] kpBasePath The path where to search for the partitions.
 */
static void _detectPartitions(const char* kpBasePath);

/**
 * @brief Detects partition in an MBR mount point.
 *
 * @details Detects partition in an MBR mount point. Creates the partitions
 * detected at the path provided. The driver shall create the device path
 * corresponding to the partitions it detected and return if the creation was
 * successful.
 *
 * @param[in] kpBasePath The device path where to search for partitions.
 *
 * @return The function shall return TRUE on success and FALSE on error.
 */
static bool_t _detectAndCreateMBRParts(const char* kpBasePath);

/**
 * @brief Detects partition in a GPT mount point.
 *
 * @details Detects partition in an GPT mount point. Creates the partitions
 * detected at the path provided. The driver shall create the device path
 * corresponding to the partitions it detected and return if the creation was
 * successful.
 *
 * @param[in] kpBasePath The device path where to search for partitions.
 *
 * @return The function shall return TRUE on success and FALSE on error.
 */
static bool_t _detectAndCreateGPTParts(const char* kpBasePath);

/**
 * @brief Partition VFS open hook.
 *
 * @details Partition VFS open hook. This function returns a handle to control the
 * VFS partition nodes.
 *
 * @param[in, out] pDrvCtrl The partition VFS driver.
 * @param[in] kpPath The path in the VFS mount point table.
 * @param[in] flags The open flags.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _dskmgrVfsOpen(void*       pDrvCtrl,
                            const char* kpPath,
                            int         flags,
                            int         mode);

/**
 * @brief Partition VFS close hook.
 *
 * @details Partition VFS close hook. This function closes a handle that was
 * created when calling the open function.
 *
 * @param[in, out] pDrvCtrl The partition VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _dskmgrVfsClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief Partition VFS write hook.
 *
 * @details Partition VFS write hook.
 *
 * @param[in, out] pDrvCtrl The partition VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _dskmgrVfsWrite(void*       pDrvCtrl,
                               void*       pHandle,
                               const void* kpBuffer,
                               size_t      count);

/**
 * @brief Partition VFS write hook.
 *
 * @details Partition VFS write hook.
 *
 * @param[in, out] pDrvCtrl The partition VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _dskmgrVfsRead(void*  pDrvCtrl,
                              void*  pHandle,
                              void*  pBuffer,
                              size_t count);

/**
 * @brief Partition VFS ReadDir hook.
 *
 * @details Partition VFS ReadDir hook. This function performs the ReadDir for the
 * Partition driver.
 *
 * @param[in, out] pDrvCtrl The partition VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] pDirEntry The directory entry to fill by the driver.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _dskmgrVfsReadDir(void*     pDriverData,
                                 void*     pHandle,
                                 dirent_t* pDirEntry);

/**
 * @brief Partition VFS IOCTL hook.
 *
 * @details Partition VFS IOCTL hook. This function performs the IOCTL for the
 * Partition driver.
 *
 * @param[in, out] pDrvCtrl The partition VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _dskmgrVfsIOCTL(void*    pDriverData,
                               void*    pHandle,
                               uint32_t operation,
                               void*    pArgs);

/**
 * @brief Partition VFS seek hook.
 *
 * @details Partition VFS seek hook. This function performs a seek for the
 * Partition driver.
 *
 * @param[in, out] pDrvCtrl The Partition driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in, out] pArgs The arguments for the seek operation.
 *
 * @return The function returns the new offset from the beginning of the file on
 * success and -1 on error;
 */
static ssize_t _dskmgrVfsSeek(void*              pDriverData,
                              void*              pHandle,
                              seek_ioctl_args_t* pArgs);

/**
 * @brief Adds calculation to the initial CRC32 value.
 *
 * @details Adds calculation to the initial CRC32 value. To start a new CRC
 * calculation, use the ild crc value GPT_CRC_START_VAL.
 *
 * @param[in] kOldCrcValue The old CRC32 value to update.
 * @param[in] kpBuffer The buffer to calculate.
 * @param[in] kSize The size of the buffer to calculate.
 *
 * @return The function returns the newly calculated CRC32 value.
 */
static inline uint32_t _gptCrcAdd(const uint32_t kOldCrcValue,
                                  const uint8_t* kpBuffer,
                                  const size_t   kSize);

/**
 * @brief Ends the calculation to the CRC32 value.
 *
 * @details Ends the calculation to the CRC32 value. The function returns the
 * actual CRC32 value after computation.
 *
 * @param[in] kOldCrcValue The old CRC32 value to update.
 *
 * @return The function returns the newly calculated final CRC32 value.
 */
static inline uint32_t _gptCrcEnd(const uint32_t kOldCrcValue);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Start address of the registered kernel partition table driver */
extern uintptr_t _START_PART_TABLE_ADDR;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Stores the partition table managers */
static dskmgr_driver_t sPartTableMgr[] = {
    {_detectAndCreateGPTParts},
    {_detectAndCreateMBRParts},
    {NULL}
};

/** @brief Stores the CRC32 table for the GPT checksum calculation */
static uint32_t sCrc32Table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static bool_t _detectAndCreateMBRParts(const char* kpBasePath)
{
    mbr_data_t   mbrData;
    int32_t      fd;
    ssize_t      readSize;
    uint32_t     partId;
    partition_t* pPart;
    char*        pPath;
    size_t       pathSize;

    /* Open the path to the mount point */
    fd = vfsOpen(kpBasePath, O_RDONLY, 0);
    if(fd < 0)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               "Failed to open partition at %s",
               kpBasePath);
        return FALSE;
    }

#if DISKMGR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Checking MBR at %s",
           kpBasePath);
#endif

    /* Read the potential MBR */
    readSize = vfsRead(fd, &mbrData, sizeof(mbr_data_t));
    if(readSize == sizeof(mbr_data_t))
    {
        if(mbrData.signature == MBR_SIGNATURE)
        {
            for(partId = 0; partId < MBR_PARTITION_COUNT; ++partId)
            {
                /* Check if the entry is unused */
                if(mbrData.partitions[partId].sysId == 0)
                {
                    continue;
                }

                /* Register a partition driver for the detected partition */
                pPart = kmalloc(sizeof(partition_t));
                if(pPart == NULL)
                {
                    syslog(SYSLOG_LEVEL_ERROR,
                           MODULE_NAME,
                           "Failed to allocate memory for MBR partition");
                    continue;
                }

                /* Create new path */
                pathSize = strlen(kpBasePath);
                pathSize += 3;

                pPath = kmalloc(pathSize);
                if(pPath == NULL)
                {
                    syslog(SYSLOG_LEVEL_ERROR,
                           MODULE_NAME,
                           "Failed to allocate memory for MBR path");
                    kfree(pPart);
                    continue;
                }
                memcpy(pPath, kpBasePath, pathSize - 3);
                pPath[pathSize - 3] = 'p';
                pPath[pathSize - 2] = '0' + partId;
                pPath[pathSize - 1] = 0;

                pPart->sectorCount = mbrData.partitions[partId].size;
                pPart->lbaStart = mbrData.partitions[partId].lbaStart;
                pPart->active = mbrData.partitions[partId].active;
                pPart->type = mbrData.partitions[partId].sysId;
                KERNEL_SPINLOCK_INIT(pPart->lock);

                pPart->pDriver = vfsRegisterDriver(pPath,
                                                   pPart,
                                                   _dskmgrVfsOpen,
                                                   _dskmgrVfsClose,
                                                   _dskmgrVfsRead,
                                                   _dskmgrVfsWrite,
                                                   _dskmgrVfsReadDir,
                                                   _dskmgrVfsIOCTL);

                if(pPart->pDriver == VFS_DRIVER_INVALID)
                {
                    syslog(SYSLOG_LEVEL_ERROR,
                           MODULE_NAME,
                           "Failed to create the VFS partition for MBR");
                    kfree(pPart);
                    kfree(pPath);
                    continue;
                }
#if DISKMGR_DEBUG_ENABLED
                syslog(SYSLOG_LEVEL_DEBUG,
                       MODULE_NAME,
                       "Added partition at %s",
                       pPath);
#endif
                /* Now just reset the pPath to the original one to keep a copy
                 * in the partition's structure
                 */
                pPath[pathSize - 3] = 0;
                pPart->pDiskPath = pPath;
            }
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to read partition first sector. Read %dB",
               readSize);
        return FALSE;
    }

    vfsClose(fd);

    return TRUE;
}

static bool_t _detectAndCreateGPTParts(const char* kpBasePath)
{
    gpt_table_header_t tableHeader;
    gpt_table_entry_t  tableEntry;
    seek_ioctl_args_t  seekArgs;
    uint64_t           lba;
    int32_t            fd;
    ssize_t            readSize;
    size_t             entrySize;
    uint32_t           partId;
    partition_t*       pPart;
    char*              pPath;
    size_t             pathSize;
    uint32_t           i;
    uint8_t            guidAdd;
    int32_t            retVal;
    uint32_t           crc32Val;
    uint32_t           savedCrc;

    /* Open the path to the mount point */
    fd = vfsOpen(kpBasePath, O_RDONLY, 0);
    if(fd < 0)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               "Failed to open partition at %s",
               kpBasePath);
        return FALSE;
    }

    /* Moving to LBA 1 */
    lba = 1;
    retVal = vfsIOCTL(fd, VFS_IOCTL_DEV_SET_LBA, &lba);
    if(retVal < 0)
    {
        syslog(SYSLOG_LEVEL_ERROR, MODULE_NAME, "Failed to seek GPT header");
        return FALSE;
    }

#if DISKMGR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Checking GPT at %s",
           kpBasePath);
#endif

    /* Read potential GPT header */
    readSize = vfsRead(fd, &tableHeader, sizeof(gpt_table_header_t));
    if(readSize == sizeof(gpt_table_header_t))
    {
        if(tableHeader.signature == GPT_SIGNATURE)
        {
            /* Compute the header CRC */
            savedCrc = tableHeader.crc32Header;
            tableHeader.crc32Header = 0;
            crc32Val = _gptCrcAdd(GPT_CRC_START_VAL,
                                  (uint8_t*)&tableHeader,
                                  (uint8_t*)&tableHeader.reserved1 -
                                  (uint8_t*)&tableHeader);
            crc32Val = _gptCrcEnd(crc32Val);
            if(crc32Val != savedCrc)
            {
                syslog(SYSLOG_LEVEL_ERROR,
                       MODULE_NAME,
                       "Failed to match CRC32 header table: %x vs %x",
                       savedCrc,
                       crc32Val);
                return FALSE;
            }

            /* TODO: Compute the table CRC */

            /* For each entry, parse it */
            entrySize = MIN(tableHeader.partitionArrayEntrySize,
                            sizeof(gpt_table_entry_t));
            lba = 2;
            retVal = vfsIOCTL(fd, VFS_IOCTL_DEV_SET_LBA, &lba);
            if(retVal < 0)
            {
                syslog(SYSLOG_LEVEL_ERROR,
                       MODULE_NAME,
                       "Failed to seek GPT entry");
                return FALSE;
            }

            for(partId = 0; partId < tableHeader.partitionCount; ++partId)
            {
                readSize = vfsRead(fd, &tableEntry, entrySize);
                if(readSize != (ssize_t)entrySize)
                {
                    syslog(SYSLOG_LEVEL_ERROR,
                           MODULE_NAME,
                           "Failed to read GPT entry. Read size %dB.",
                           readSize);
                    return FALSE;
                }

                /* Seek the rest of the entry if it was bigger */
                seekArgs.direction = SEEK_CUR;
                seekArgs.offset = tableHeader.partitionArrayEntrySize -
                                  readSize;
                retVal = vfsIOCTL(fd, VFS_IOCTL_FILE_SEEK, &seekArgs);
                if(retVal < 0)
                {
                    syslog(SYSLOG_LEVEL_ERROR,
                           MODULE_NAME,
                           "Failed to seek GPT entry");
                    return FALSE;
                }

                /* Check if used */
                guidAdd = 0;
                for(i = 0; i < 16; ++i)
                {
                    guidAdd |= tableEntry.typeGUID[i];
                }
                if(guidAdd == 0)
                {
                    continue;
                }

                /* Register a partition driver for the detected partition */
                pPart = kmalloc(sizeof(partition_t));
                if(pPart == NULL)
                {
                    syslog(SYSLOG_LEVEL_ERROR,
                           MODULE_NAME,
                           "Failed to allocate memory for GPT partition");
                    continue;
                }

                /* Create new path */
                pathSize = strlen(kpBasePath);
                pathSize += 4;

                pPath = kmalloc(pathSize);
                if(pPath == NULL)
                {
                    syslog(SYSLOG_LEVEL_ERROR,
                           MODULE_NAME,
                           "Failed to allocate memory for MBR path");
                    kfree(pPart);
                    continue;
                }
                memcpy(pPath, kpBasePath, pathSize - 4);
                pPath[pathSize - 4] = 'p';
                if(partId >= 26)
                {
                    pPath[pathSize - 3] = 'a' + (partId / 26) - 1;
                    pPath[pathSize - 2] = 'a' + (partId % 26);
                    pPath[pathSize - 1] = 0;
                }
                else
                {
                    pPath[pathSize - 3] = 'a' + partId;
                    pPath[pathSize - 2] = 0;
                    pPath[pathSize - 1] = 0;
                }

                pPart->sectorCount = tableEntry.endingLBA -
                                     tableEntry.startingLBA;
                pPart->lbaStart = tableEntry.startingLBA;
                pPart->active = (tableEntry.attributes &
                                 GPT_ATTRIBUTE_BOOTABLE) != 0;
                pPart->type = 0xFF;
                KERNEL_SPINLOCK_INIT(pPart->lock);

                pPart->pDriver = vfsRegisterDriver(pPath,
                                                   pPart,
                                                   _dskmgrVfsOpen,
                                                   _dskmgrVfsClose,
                                                   _dskmgrVfsRead,
                                                   _dskmgrVfsWrite,
                                                   _dskmgrVfsReadDir,
                                                   _dskmgrVfsIOCTL);

                if(pPart->pDriver == VFS_DRIVER_INVALID)
                {
                    syslog(SYSLOG_LEVEL_ERROR,
                           MODULE_NAME,
                           "Failed to create the VFS partition for MBR");
                    kfree(pPart);
                    kfree(pPath);
                    continue;
                }
#if DISKMGR_DEBUG_ENABLED
                syslog(SYSLOG_LEVEL_DEBUG,
                       MODULE_NAME,
                       "Added partition at %s",
                       pPath);
#endif
                /* Now just reset the pPath to the original one to keep a copy
                 * in the partition's structure
                 */
                pPath[pathSize - 4] = 0;
                pPart->pDiskPath = pPath;
            }
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to read partition first sector. Read %dB",
               readSize);
        return FALSE;
    }

    vfsClose(fd);

    return TRUE;
}

static void _detectPartitions(const char* kpBasePath)
{
    int32_t          fd;
    int32_t          retVal;
    dirent_t         dirEntry;
    size_t           pathLen;
    size_t           baseLen;
    char*            pNewPath;
    dskmgr_driver_t* pDriver;

#if DISKMGR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Detecting partitions in %s",
           kpBasePath);
#endif

    /* Open the folder */
    fd = vfsOpen(kpBasePath, O_RDONLY, 0);
    if(fd < 0)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Disk manager failed to open the storage location");
        return;
    }
    baseLen = strlen(kpBasePath);

    /* Now for all entries in the storage path, either go deeper or detect */
    retVal = vfsReaddir(fd, &dirEntry);
    if(retVal >= 0)
    {
        pathLen  = strlen(dirEntry.pName) + baseLen + 2;
        pNewPath = kmalloc(pathLen);
        DSKMGR_ASSERT(pNewPath != NULL,
                        "Failed to allocated memory",
                        OS_ERR_NO_MORE_MEMORY);
        snprintf(pNewPath, pathLen, "%s/%s", kpBasePath, dirEntry.pName);
        if(dirEntry.type != VFS_FILE_TYPE_DIR)
        {
#if DISKMGR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Detected device at %s",
           pNewPath);
#endif
            /* Loop over all registered partition table drivers */
            pDriver = &sPartTableMgr[0];

            /* Compare with the list of registered drivers */
            while(pDriver->createPartitions != NULL)
            {
                /* Call the detection */
                if(pDriver->createPartitions(pNewPath) == TRUE)
                {
                    break;
                }
                ++pDriver;
            }
        }
        else
        {
            /* Recursively detect the partition */
            _detectPartitions(pNewPath);
        }
        kfree(pNewPath);
        retVal = vfsReaddir(fd, &dirEntry);
    }

    vfsClose(fd);
}

void diskManagerInit(void)
{
    /* Start detecting partitions */
    _detectPartitions(DSKMGR_STORAGE_ROOT_PATH);
}

static void* _dskmgrVfsOpen(void*       pDrvCtrl,
                            const char* kpPath,
                            int         flags,
                            int         mode)
{
    partition_t*      pPart;
    dskmgr_desc_t*    pDesc;
    uint32_t          sectorSize;
    int32_t           retVal;
    seek_ioctl_args_t seekArgs;

    /* The path must be empty */
    if((*kpPath == '/' && *(kpPath + 1) != 0) || *kpPath != 0)
    {
        return (void*)-1;
    }

    pPart = pDrvCtrl;

    /* Open a handle to the disk from the partition */
    pDesc = kmalloc(sizeof(dskmgr_desc_t));
    if(pDesc == NULL)
    {
        return (void*)-1;
    }

    pDesc->partFd = vfsOpen(pPart->pDiskPath, flags, mode);
    if(pDesc->partFd < 0)
    {
        kfree(pDesc);
        return (void*)-1;
    }

    /* Get the sector size */
    retVal = vfsIOCTL(pDesc->partFd,
                      VFS_IOCTL_DEV_GET_SECTOR_SIZE,
                      &sectorSize);
    if(retVal < 0 || sectorSize == 0)
    {
        vfsClose(pDesc->partFd);
        kfree(pDesc);
        return (void*)-1;
    }

    /* Place the cursor at the begining of the partition on the disk */
    seekArgs.direction = SEEK_SET;
    seekArgs.offset    = (uint64_t)pPart->lbaStart * sectorSize;
    retVal = vfsIOCTL(pDesc->partFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    if(retVal < 0)
    {
        vfsClose(pDesc->partFd);
        kfree(pDesc);
        return (void*)-1;
    }
    pDesc->pPart          = pPart;
    pDesc->partitionStart = (uint64_t)pPart->lbaStart * sectorSize;
    pDesc->posByte        = 0;
    pDesc->sizeByte       = (uint64_t)pPart->sectorCount * sectorSize;

    return pDesc;
}

static int32_t _dskmgrVfsClose(void* pDrvCtrl, void* pHandle)
{
    int32_t        partFd;
    dskmgr_desc_t* pDesc;

    (void)pDrvCtrl;

    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    /* Clear the descriptor */
    pDesc = pHandle;
    partFd = pDesc->partFd;

    kfree(pDesc);

    return vfsClose(partFd);
}

static ssize_t _dskmgrVfsWrite(void*       pDrvCtrl,
                               void*       pHandle,
                               const void* kpBuffer,
                               size_t      count)
{
    dskmgr_desc_t* pDesc;

    (void)pDrvCtrl;

    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    pDesc = pHandle;

    /* Check the size */
    count = MIN(count, pDesc->sizeByte - pDesc->posByte);

    /* Write and update cursor */
    count = vfsWrite(pDesc->partFd, kpBuffer, count);
    if(count > 0)
    {
        pDesc->posByte += count;
    }

    return count;
}

static ssize_t _dskmgrVfsRead(void*  pDrvCtrl,
                              void*  pHandle,
                              void*  pBuffer,
                              size_t count)
{
    dskmgr_desc_t* pDesc;
    ssize_t        actualCount;

    (void)pDrvCtrl;

    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    pDesc = pHandle;

    /* Check the size */
    count = MIN(count, pDesc->sizeByte - pDesc->posByte);

    /* Read and update cursor */
    actualCount = vfsWrite(pDesc->partFd, pBuffer, count);
    if(actualCount > 0)
    {
        pDesc->posByte += actualCount;
    }

    return actualCount;
}

static int32_t _dskmgrVfsReadDir(void*     pDriverData,
                                 void*     pHandle,
                                 dirent_t* pDirEntry)
{
    dskmgr_desc_t* pDesc;

    (void)pDriverData;

    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    pDesc = pHandle;

    return vfsReaddir(pDesc->partFd, pDirEntry);
}

static ssize_t _dskmgrVfsIOCTL(void*    pDriverData,
                               void*    pHandle,
                               uint32_t operation,
                               void*    pArgs)
{
    ssize_t retVal;

    switch(operation)
    {
        case VFS_IOCTL_FILE_SEEK:
            retVal = _dskmgrVfsSeek(pDriverData, pHandle, pArgs);
            break;
        default:
            retVal = -1;
    }

    return retVal;
}

static ssize_t _dskmgrVfsSeek(void*              pDriverData,
                              void*              pHandle,
                              seek_ioctl_args_t* pArgs)
{
    dskmgr_desc_t*    pDesc;
    ssize_t           retVal;
    seek_ioctl_args_t seekArgs;

    (void)pDriverData;

    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    pDesc = pHandle;

    seekArgs.direction = SEEK_SET;

    retVal = -1;

    if(pArgs->direction == SEEK_SET)
    {
        if(pArgs->offset <= pDesc->sizeByte)
        {
            pDesc->posByte = pArgs->offset;
            seekArgs.offset = pDesc->partitionStart + pDesc->posByte;
            retVal = vfsIOCTL(pDesc->partFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
        }
    }
    else if(pArgs->direction == SEEK_CUR)
    {
        if(pDesc->posByte + pArgs->offset <= pDesc->sizeByte)
        {
            pDesc->posByte += pArgs->offset;
            seekArgs.offset = pDesc->partitionStart + pDesc->posByte;
            retVal = vfsIOCTL(pDesc->partFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
        }
    }

    if(retVal < 0)
    {
        return -1;
    }
    else
    {
        return pDesc->posByte;
    }
}

static inline uint32_t _gptCrcAdd(uint32_t       oldCrcValue,
                                  const uint8_t* kpBuffer,
                                  const size_t   kSize)
{
    size_t i;
    for(i = 0; i < kSize; ++i)
    {
        oldCrcValue = sCrc32Table[(oldCrcValue ^ *(kpBuffer + i)) & 0xFF] ^
                      (oldCrcValue >> 8);
    }

    return oldCrcValue;
}

static inline uint32_t _gptCrcEnd(const uint32_t kOldCrcValue)
{
    return kOldCrcValue ^ 0xFFFFFFFF;
}

/************************************ EOF *************************************/