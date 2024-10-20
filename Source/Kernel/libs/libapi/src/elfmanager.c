/*******************************************************************************
 * @file elfmanager.c
 *
 * @see elfmanager.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 18/10/2024
 *
 * @version 1.0
 *
 * @brief ELF file manager.
 *
 * @details ELF file manager. This file provides the interface to manage ELF
 * file, load ELF and populate memory with an ELF file.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <vfs.h>        /* Virtual FileSystem */
#include <panic.h>      /* Kernel panic */
#include <kheap.h>      /* Kernel heap */
#include <ioctl.h>      /* IOCTL operations */
#include <kerror.h>     /* Kernel error */
#include <syslog.h>     /* Kernel syslog */
#include <string.h>     /* String and memory manipulation */
#include <memory.h>     /* Memory management */
#include <stdint.h>     /* Standard integer types */
#include <scheduler.h>  /* Kernel scheduler */
#include <ctrl_block.h> /* Process control block */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <elfmanager.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "ELFMGR"

#ifdef ARCH_64_BITS
/** @brief Sets the ELF architecutre to 64 bits */
#define ELF_64_BITS
#else
/** @brief Sets the ELF architecutre to 32 bits */
#define ELF_32_BITS
#endif

#if defined(ARCH_X86_64)
/** @brief Set the ELF machine support to AMD64 */
#define ELF_MACHINE_SUPPORT 0x3E
#elif defined(ARCH_I386)
/** @brief Set the ELF machine support to x86 */
#define ELF_MACHINE_SUPPORT 0x03
#else
#error "Invalid ELF architecture machine support"
#endif

/** @brief ELF file class: 32 bits */
#define ELF_CLASS_32 1
/** @brief ELF file class: 64 bits */
#define ELF_CLASS_64 2
/** @brief ELF endianess: Little Endian */
#define ELF_ENDIANESS_LITTLE 1
/** @brief ELF endianess: Big Endian */
#define ELF_ENDIANESS_BIG 2
/** @brief ELF current version */
#define ELF_VERSION_CURRENT 1

/** @brief ELF object type: unknown */
#define ELF_TYPE_NONE 0x0000
/** @brief ELF object type: relocatable file */
#define ELF_TYPE_RELOC 0x0001
/** @brief ELF object type: executable file */
#define ELF_TYPE_EXEC 0x0002
/** @brief ELF object type: shared object */
#define ELF_TYPE_DYN 0x0003
/** @brief ELF object type: code file */
#define ELF_TYPE_CORE 0x0004
/** @brief ELF object type: low OS */
#define ELF_TYPE_LOOS 0xFE00
/** @brief ELF object type: high OS */
#define ELF_TYPE_HIOS 0xFEFF
/** @brief ELF object type: low processor */
#define ELF_TYPE_LOPROC 0xFF00
/** @brief ELF object type: high processor */
#define ELF_TYPE_HIPROC 0xFFFF

/** @brief ELF segment type: unused */
#define ELF_SEG_TYPE_NULL 0x00000000
/** @brief ELF segment type: loadable */
#define ELF_SEG_TYPE_LOAD 0x00000001
/** @brief ELF segment type: dynamic linking */
#define ELF_SEG_TYPE_DYNAMIC 0x00000002
/** @brief ELF segment type: interpreter info */
#define ELF_SEG_TYPE_INTERP 0x00000003
/** @brief ELF segment type: note info */
#define ELF_SEG_TYPE_NOTE 0x00000004
/** @brief ELF segment type: shared library */
#define ELF_SEG_TYPE_SHLIB 0x00000005
/** @brief ELF segment type: contains the program header itself */
#define ELF_SEG_TYPE_PHDR 0x00000006
/** @brief ELF segment type: thread local storage templace */
#define ELF_SEG_TYPE_TLS 0x00000007
/** @brief ELF segment type: low OS */
#define ELF_SEG_TYPE_LOOS 0x60000000
/** @brief ELF segment type: high OS */
#define ELF_SEG_TYPE_HIOS 0x6FFFFFFF
/** @brief ELF segment type: low processor */
#define ELF_SEG_TYPE_LOPROC 0x70000000
/** @brief ELF segment type: high processor */
#define ELF_SEG_TYPE_HIPROC 0x7FFFFFFF

/** @brief ELF segment flag: executable */
#define ELF_SEG_FLAG_X 0x1
/** @brief ELF segment flag: writeable */
#define ELF_SEG_FLAG_W 0x2
/** @brief ELF segment flag: readeable */
#define ELF_SEG_FLAG_R 0x4

/** @brief ELF section type: unused */
#define ELF_SECTION_TYPE_NULL 0x00000000
/** @brief ELF section type: program data */
#define ELF_SECTION_TYPE_PROGBITS 0x00000001
/** @brief ELF section type: symbol table */
#define ELF_SECTION_TYPE_SYSTAB 0x00000002
/** @brief ELF section type: string table */
#define ELF_SECTION_TYPE_STRTAB 0x00000003
/** @brief ELF section type: relocation entries */
#define ELF_SECTION_TYPE_RELA 0x00000004
/** @brief ELF section type: symbols hash table */
#define ELF_SECTION_TYPE_HASH 0x00000005
/** @brief ELF section type: dynamic linking */
#define ELF_SECTION_TYPE_DYNAMIC 0x00000006
/** @brief ELF section type: note info */
#define ELF_SECTION_TYPE_NOTE 0x00000007
/** @brief ELF section type: program space with no data */
#define ELF_SECTION_TYPE_NOBITS 0x00000008
/** @brief ELF section type: relocation entries */
#define ELF_SECTION_TYPE_REL 0x00000009
/** @brief ELF section type: shared library */
#define ELF_SECTION_TYPE_SHLIB 0x0000000A
/** @brief ELF section type: dynamic linker symbol table */
#define ELF_SECTION_TYPE_DYNSYM 0x0000000B
/** @brief ELF section type: constructors array */
#define ELF_SECTION_TYPE_INIT_ARRAY 0x0000000E
/** @brief ELF section type: destructors array */
#define ELF_SECTION_TYPE_FINI_ARRAY 0x0000000F
/** @brief ELF section type: pre-contructors array */
#define ELF_SECTION_TYPE_PREINIT_ARRAY 0x00000010
/** @brief ELF section type: section group */
#define ELF_SECTION_TYPE_GROUP 0x00000011
/** @brief ELF section type: extended section indicies */
#define ELF_SECTION_TYPE_SYMTAB_SHNDX 0x00000012
/** @brief ELF section type: number of defined types */
#define ELF_SECTION_TYPE_NUM 0x00000013
/** @brief ELF section type: low OS */
#define ELF_SECTION_TYPE_LOOS 0x60000000
/** @brief ELF section type: high OS */
#define ELF_SECTION_TYPE_HIOS 0x6FFFFFFF
/** @brief ELF section type: low processor */
#define ELF_SECTION_TYPE_LOPROC 0x70000000
/** @brief ELF section type: high processor */
#define ELF_SECTION_TYPE_HIPROC 0x7FFFFFFF

/** @brief ELF section flag: writeable */
#define ELF_SECTION_FLAG_WRITE 0x00000001
/** @brief ELF section flag: occupies memory during execution */
#define ELF_SECTION_FLAG_ALLOC 0x00000002
/** @brief ELF section flag: executable */
#define ELF_SECTION_FLAG_EXECINSTR 0x00000004
/** @brief ELF section flag: can be merged */
#define ELF_SECTION_FLAG_MERGE 0x00000010
/** @brief ELF section flag: contains null terminated strings */
#define ELF_SECTION_FLAG_STRINGS 0x00000020
/** @brief ELF section flag: shInfo contains SHT index */
#define ELF_SECTION_FLAG_INFO_LINK 0x00000040
/** @brief ELF section flag: preserve order after combining */
#define ELF_SECTION_FLAG_LINK_ORDER 0x00000080
/** @brief ELF section flag: OS specific handling */
#define ELF_SECTION_FLAG_OS_NONCONFORMING 0x00000100
/** @brief ELF section flag: section is member of a group */
#define ELF_SECTION_FLAG_GROUP 0x00000200
/** @brief ELF section flag: section holds thread local data */
#define ELF_SECTION_FLAG_TLS 0x00000400
/** @brief ELF section flag: OS specific */
#define ELF_SECTION_FLAG_MASKOS 0x0FF00000
/** @brief ELF section flag: processor specific */
#define ELF_SECTION_FLAG_MASKPROC 0xF0000000
/** @brief ELF section flag: special ordering requirements */
#define ELF_SECTION_FLAG_ORDERED 0x40000000
/** @brief ELF section flag: exclude unless referenced or allocated */
#define ELF_SECTION_FLAG_EXCLUDE 0x80000000

/** @brief System V ABI identifier */
#define ELF_SYSTEMV_ABI 0
/** @brief System V ABI version */
#define ELF_ABI_VERSION 0

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief The ELF header format in memory */
typedef struct
{
    /** @brief ELF magic */
    uint8_t eiMag[4];

    /** @brief File class (32 bites or 64 bits) */
    uint8_t eiClass;

    /** @brief The ELF endianess */
    uint8_t eiData;

    /** @brief The ELF version */
    uint8_t eiVersion;

    /** @brief The ELF OS ABI */
    uint8_t eiABI;

    /** @brief The ELF OS ABI Version */
    uint8_t eiABIVersion;

    /** @brief Padding */
    uint8_t eiPad[7];

    /** @brief Identifies the object file type. */
    uint16_t eType;

    /** @brief Identifies the ISA used */
    uint16_t eMachine;

    /** @brief The ELF version */
    uint32_t eVersion;

    /** @brief The entry point virtual address */
    uintptr_t eEntry;

    /** @brief The program header table offset in the file */
    uintptr_t ePhOff;

    /** @brief The section header table offset in the file */
    uintptr_t eShOff;

    /** @brief The ELF flags */
    uint32_t eFlags;

    /** @brief Size of the ELF header in bytes */
    uint16_t eEhSize;

    /** @brief Size of a program header entry in bytes */
    uint16_t ePhEntSize;

    /** @brief Number of entries in the program headers table. */
    uint16_t ePhNum;

    /** @brief Size of a section header entry in bytes */
    uint16_t eShEntSize;

    /** @brief Number of entries in the section headers table. */
    uint16_t eShNum;

    /**
     * @brief The index of the section header that contains the section names
     * in the section header tables.
     */
    uint16_t eShStrNdx;
} __attribute__ ((packed)) elf_header_t;

/** @brief The ELF program header format in memory */
typedef struct
{
    /** @brief The segment type */
    uint32_t pType;

#ifdef ELF_64_BITS
    /** @brief The segment flags */
    uint32_t pFlags;
#endif

    /** @brief Offset of the segment in the file */
    uintptr_t pOffset;

    /** @brief Virtual address of the segment in memory */
    uintptr_t pVAddr;

    /** @brief Physical address of the segment in memory */
    uintptr_t pPAddr;

    /** @brief Size of the segment in the file in bytes */
    uintptr_t pFileSz;

    /** @brief Size of the segment in the memory in bytes */
    uintptr_t pMemSz;

#ifdef ELF_32_BITS
    /** @brief The segment flags */
    uint32_t pFlags;
#endif

    /** @brief Alignement in memory */
    uintptr_t pAlign;

} __attribute__ ((packed)) elf_pheader_t;

/** @brief The ELF section header format in memory */
typedef struct
{
    /** @brief The offset of the section name in the string section */
    uint32_t shName;

    /** @brief The section type */
    uint32_t shType;

    /** @brief The section flags */
    uintptr_t shFlags;

    /** @brief Virtual address of the section in memory */
    uintptr_t shAddr;

    /** @brief Offset of the section in the file */
    uintptr_t shOffset;

    /** @brief Size of the section in the file in bytes */
    uintptr_t shSize;

    /** @brief Contains the section index of an associated section */
    uint32_t shLink;

    /** @brief Contains extra information about the section */
    uint32_t shInfo;

    /** @brief Contains the required alignment of the section */
    uintptr_t shAddrAlig;

    /**
     * @brief Contains the size, in bytes, of each entry, for sections that
     * contain fixed-size entries.
     */
    uintptr_t shEntSize;
} __attribute__ ((packed)) elf_sheader_t;

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
#define ELFMGR_ASSERT(COND, MSG, ERROR) {                   \
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}


/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Checks a file to validate the ELF format.
 *
 * @details Checks a file to validate the ELF format. This function will load
 * the required parts of the ELF. The function returns an error if the file is
 * not valid.
 *
 * @param[in] kpHeader The ELF header to check.
 *
 * @return The function return OS_NO_ERR if the file is valid or an error
 * otherwise.
 */
static OS_RETURN_E _checkFile(const elf_header_t* kpHeader);

/**
 * @brief Loads a relocatable ELF.
 *
 * @details Loads a relocatable ELF. The function loads the ELF and
 * allocates the required memory regions in the process memory space.
 *
 * @param[in] kFileFd The ELF file descriptor.
 * @param[in] kpHeader The ELF header.
 *
 * @return The function return the success or error status.
 */
static OS_RETURN_E _loadElfReloc(const int32_t       kFileFd,
                                 const elf_header_t* kpHeader);

/**
 * @brief Loads an executable ELF.
 *
 * @details Loads an executable ELF and replaces the process memory space with
 * the ELF data. This function assumes that the process user memory space is
 * empty. The function loads the ELF and allocates the required memory regions
 * in the process memory space.
 *
 * @param[in] kFileFd The ELF file descriptor.
 * @param[in] kpHeader The ELF header.
 *
 * @return The function return the success or error status.
 */
static OS_RETURN_E _loadElfExec(const int32_t       kFileFd,
                                const elf_header_t* kpHeader);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief The ELF magic */
static const uint8_t sElfMagic[] = {0x7F, 0x45, 0x4C, 0x46};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _checkFile(const elf_header_t* kpHeader)
{
    uint8_t i;

    /* Check the magic */
    for(i = 0; i < ARRAY_SIZE(sElfMagic); ++i)
    {
        if(kpHeader->eiMag[i] != sElfMagic[i])
        {
            return OS_ERR_INCORRECT_VALUE;
        }
    }

    /* Check the CPU support */
#ifdef ELF_64_BITS
    if(kpHeader->eiClass != ELF_CLASS_64)
    {
        return OS_ERR_NOT_SUPPORTED;
    }
#else
    if(kpHeader->eiClass != ELF_CLASS_32)
    {
        return OS_ERR_NOT_SUPPORTED;
    }
#endif

#ifdef ARCH_LITTLE_ENDIAN
    if(kpHeader->eiData != ELF_ENDIANESS_LITTLE)
    {
        return OS_ERR_NOT_SUPPORTED;
    }
#else
    if(kpHeader->eiData != ELF_ENDIANESS_BIG)
    {
        return OS_ERR_NOT_SUPPORTED;
    }
#endif

    /* Check the ABI and machine support */
    if(kpHeader->eiVersion != ELF_VERSION_CURRENT ||
       kpHeader->eiABI != ELF_SYSTEMV_ABI         ||
       kpHeader->eiABIVersion != ELF_ABI_VERSION  ||
       kpHeader->eMachine != ELF_MACHINE_SUPPORT  ||
       kpHeader->eVersion != ELF_VERSION_CURRENT)
    {
        return OS_ERR_NOT_SUPPORTED;
    }

    /* Check the type support */
    if(kpHeader->eType != ELF_TYPE_EXEC && kpHeader->eType != ELF_TYPE_RELOC)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Check the integrity */
    if(kpHeader->eShEntSize != sizeof(elf_sheader_t) ||
       kpHeader->ePhEntSize != sizeof(elf_pheader_t))
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    return OS_NO_ERR;
}

static OS_RETURN_E _loadElfReloc(const int32_t       kFileFd,
                                 const elf_header_t* kpHeader)
{
    /* TODO: Implement relocation */
    (void)kFileFd;
    (void)kpHeader;
    return OS_ERR_NOT_SUPPORTED;
}

static OS_RETURN_E _loadElfExec(const int32_t       kFileFd,
                                const elf_header_t* kpHeader)
{
    uint32_t          i;
    uintptr_t         userStartVirtAddr;
    uintptr_t         userEndVirtAddr;
    elf_pheader_t*    pProgHeader;
    seek_ioctl_args_t seekArgs;
    OS_RETURN_E       error;
    int32_t           opResult;
    ssize_t           readSize;
    uint32_t          mappingFlags;
    size_t            toMap;
    size_t            toCopy;
    size_t            copied;
    size_t            mapped;
    size_t            toUnmap;
    uintptr_t         newFrame;
    uint8_t*          tmpAddr;
    uint8_t*          pDataBuffer;
    kernel_process_t* pProcess;

    /* Get the user memory bounds */
    userStartVirtAddr = memoryGetUserStartAddr();
    userEndVirtAddr   = memoryGetUserEndAddr();

    /* Set file position */
    seekArgs.direction = SEEK_SET;
    seekArgs.offset    = kpHeader->ePhOff;
    opResult = vfsIOCTL(kFileFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
    if(opResult != (int32_t)kpHeader->ePhOff)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Load all segments headers */
    pProgHeader = kmalloc(sizeof(elf_pheader_t) * kpHeader->ePhNum);
    if(pProgHeader == NULL)
    {
        return OS_ERR_NO_MORE_MEMORY;
    }
    readSize = vfsRead(kFileFd,
                       pProgHeader,
                       sizeof(elf_pheader_t) * kpHeader->ePhNum);
    if(readSize != (ssize_t)sizeof(elf_pheader_t) * kpHeader->ePhNum)
    {
        kfree(pProgHeader);
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Validate all segments */
    for(i = 0; i < kpHeader->ePhNum; ++i)
    {
        /* Only check the loadable segments */
        if(pProgHeader[i].pType != ELF_SEG_TYPE_LOAD)
        {
            continue;
        }

        /* Check size validity */
        if(pProgHeader[i].pFileSz > pProgHeader[i].pMemSz)
        {
            kfree(pProgHeader);
            return OS_ERR_INCORRECT_VALUE;
        }

        /* Check that everything can be mapped */
        if(pProgHeader[i].pVAddr < userStartVirtAddr ||
           pProgHeader[i].pVAddr >= userEndVirtAddr  ||
           pProgHeader[i].pVAddr + pProgHeader[i].pMemSz > userEndVirtAddr)
        {
            kfree(pProgHeader);
            return OS_ERR_OUT_OF_BOUND;
        }

        /* Check alignement */
        if(pProgHeader[i].pAlign > KERNEL_PAGE_SIZE)
        {
            /* At the moment we do not support allocating memory with different
             * alignements
             */
            kfree(pProgHeader);
            return OS_ERR_NOT_SUPPORTED;
        }
    }

    /* Allocate the buffer memory */
    pDataBuffer = kmalloc(KERNEL_PAGE_SIZE);
    if(pDataBuffer == NULL)
    {
        kfree(pProgHeader);
        return OS_ERR_NO_MORE_MEMORY;
    }

    /* Load all segments */
    pProcess = schedGetCurrentProcess();
    for(i = 0; i < kpHeader->ePhNum; ++i)
    {
        /* Only check the loadable segments */
        if(pProgHeader[i].pType != ELF_SEG_TYPE_LOAD)
        {
            continue;
        }

        /* Get the mapping flags */
        mappingFlags = MEMMGR_MAP_USER;
        if((pProgHeader[i].pFlags & ELF_SEG_FLAG_R) == ELF_SEG_FLAG_R)
        {
            if((pProgHeader[i].pFlags & ELF_SEG_FLAG_W) == ELF_SEG_FLAG_W)
            {
                mappingFlags |= MEMMGR_MAP_RW;
            }
            else
            {
               mappingFlags |= MEMMGR_MAP_RO;
            }
        }
        if((pProgHeader[i].pFlags & ELF_SEG_FLAG_X) == ELF_SEG_FLAG_X)
        {
            mappingFlags |= MEMMGR_MAP_EXEC;
        }

        /* Set file position */
        seekArgs.direction = SEEK_SET;
        seekArgs.offset    = pProgHeader[i].pOffset;
        opResult = vfsIOCTL(kFileFd, VFS_IOCTL_FILE_SEEK, &seekArgs);
        if(opResult != (int32_t)pProgHeader[i].pOffset)
        {
            break;
        }

        /* Align mapping */
        toMap = (pProgHeader[i].pMemSz + (KERNEL_PAGE_SIZE - 1)) &
                ~(KERNEL_PAGE_SIZE - 1);

        /* Map the temporary memory and copy, at the same time, map to the user
         * land
         */
        copied = 0;
        for(mapped = 0; mapped < toMap; mapped += KERNEL_PAGE_SIZE)
        {
            /* Read the data */
            toCopy = MIN(KERNEL_PAGE_SIZE, pProgHeader[i].pFileSz - copied);
            if(toCopy > 0)
            {
                readSize = vfsRead(kFileFd, pDataBuffer, toCopy);
                if(readSize != (ssize_t)toCopy)
                {
                    error = OS_ERR_INCORRECT_VALUE;
                    break;
                }
            }

            newFrame = memoryAllocFrames(1);
            if(newFrame == (uintptr_t)NULL)
            {
                error = OS_ERR_NO_MORE_MEMORY;
                break;
            }

            /* Map the temporary frame */
            tmpAddr = memoryKernelMap((void*)newFrame,
                                      KERNEL_PAGE_SIZE,
                                      MEMMGR_MAP_KERNEL | MEMMGR_MAP_RW,
                                      &error);
            if(error != OS_NO_ERR)
            {
                memoryReleaseFrame(newFrame, 1);
                break;
            }

            /* Copy the data */
            if(toCopy > 0)
            {
                memcpy(tmpAddr, pDataBuffer, toCopy);
                copied += toCopy;
            }

            /* Zeorize the rest */
            if(toCopy < KERNEL_PAGE_SIZE)
            {
                memset(tmpAddr + toCopy, 0, KERNEL_PAGE_SIZE - toCopy);
            }

            /* Unmap the temporary frame */
            error = memoryKernelUnmap(tmpAddr, KERNEL_PAGE_SIZE);
            ELFMGR_ASSERT(error == OS_NO_ERR,
                          "Failed to unmap allocated memory",
                          error);

            /* Map the frame to the user space */
            error = memoryUserMapDirect((void*)newFrame,
                                        (void*)(pProgHeader[i].pVAddr + mapped),
                                        KERNEL_PAGE_SIZE,
                                        mappingFlags,
                                        pProcess);
            if(error != OS_NO_ERR)
            {
                memoryReleaseFrame(newFrame, 1);
                break;
            }
        }

        /* On error release memory */
        if(error != OS_NO_ERR)
        {
            /* Unmap all already mapped data and release the frames */
            for(toUnmap = 0; toUnmap < mapped; toUnmap += KERNEL_PAGE_SIZE)
            {
                newFrame = memoryMgrGetPhysAddr(pProgHeader[i].pVAddr +
                                                toUnmap,
                                                pProcess,
                                                NULL);
                memoryReleaseFrame(newFrame, 1);
            }
            memoryUserUnmap((void*)pProgHeader[i].pVAddr, mapped, pProcess);
            ELFMGR_ASSERT(error == OS_NO_ERR,
                          "Failed to unmap mapped memory",
                          error);

            break;
        }
    }

    /* On error release memory */
    if(i < kpHeader->ePhNum && i > 0)
    {
        do
        {
            /* Only check the loadable segments */
            if(pProgHeader[i].pType != ELF_SEG_TYPE_LOAD)
            {
                /* Align mapping */
                mapped = (pProgHeader[i].pMemSz + (KERNEL_PAGE_SIZE - 1)) &
                         ~(KERNEL_PAGE_SIZE - 1);

                /* Unmap all already mapped data and release the frames */
                for(toUnmap = 0; toUnmap < mapped; toUnmap += KERNEL_PAGE_SIZE)
                {
                    newFrame = memoryMgrGetPhysAddr(pProgHeader[i].pVAddr +
                                                    toUnmap,
                                                    pProcess,
                                                    NULL);
                    memoryReleaseFrame(newFrame, 1);
                }
                error = memoryUserUnmap((void*)pProgHeader[i].pVAddr,
                                        mapped,
                                        pProcess);
                ELFMGR_ASSERT(error == OS_NO_ERR,
                              "Failed to unmap mapped memory",
                              error);
            }

            --i;
        } while(i != 0);
    }

    /* Free the buffers */
    kfree(pDataBuffer);
    kfree(pProgHeader);

    return error;
}

OS_RETURN_E elfManagerLoadElf(const char* kpElfPath,
                              uintptr_t*  pEntryPoint)
{
    int32_t           fileFd;
    OS_RETURN_E       error;
    elf_header_t      header;
    ssize_t           readBytes;

    /* Open the file */
    fileFd = vfsOpen(kpElfPath, O_RDONLY, 0);
    if(fileFd < 0)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Get the header */
    readBytes = vfsRead(fileFd, &header, sizeof(elf_header_t));
    if(readBytes != sizeof(elf_header_t))
    {
        /* Close the file */
        fileFd = vfsClose(fileFd);
        if(fileFd != 0)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failed to close ELF file. Error %d",
                   fileFd);
        }
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Check the ELF */
    error = _checkFile(&header);
    if(error != OS_NO_ERR)
    {
        fileFd = vfsClose(fileFd);
        if(fileFd != 0)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failed to close ELF file. Error %d",
                   fileFd);
        }
        return error;
    }

    /* Check the file type and perform the load */
    if(header.eType == ELF_TYPE_RELOC)
    {
        error = _loadElfReloc(fileFd, &header);
    }
    else if(header.eType == ELF_TYPE_EXEC)
    {
        error = _loadElfExec(fileFd, &header);
    }
    else
    {
        error = OS_ERR_NOT_SUPPORTED;
    }

    if(error != OS_NO_ERR)
    {
        fileFd = vfsClose(fileFd);
        if(fileFd != 0)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failed to close ELF file. Error %d",
                   fileFd);
        }

        return error;
    }

    /* Set the entry point */
    *pEntryPoint = header.eEntry;

    /* Close the file */
    fileFd = vfsClose(fileFd);
    if(fileFd != 0)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to close ELF file. Error %d",
               fileFd);
    }

    return OS_NO_ERR;
}

/************************************ EOF *************************************/