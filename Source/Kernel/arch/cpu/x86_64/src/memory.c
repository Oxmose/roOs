/*******************************************************************************
 * @file memory.c
 *
 * @see memory.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 09/06/2024
 *
 * @version 3.0
 *
 * @brief Kernel physical memory manager.
 *
 * @details This module is used to detect the memory mapping of the system and
 * manage physical and virtual memory as well as peripherals memory.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <kheap.h>         /* Kernel heap */
#include <panic.h>         /* Kernel PANIC */
#include <x86cpu.h>        /* X86 CPU API*/
#include <string.h>        /* Memory manipulation */
#include <stdint.h>        /* Standard int types */
#include <stddef.h>        /* Standard definitions */
#include <kqueue.h>        /* Kernel queue structure */
#include <kerror.h>        /* Kernel error types */
#include <signal.h>        /* Thread signals */
#include <syslog.h>        /* Kernel Syslog */
#include <devtree.h>       /* FDT library */
#include <stdbool.h>       /* Bool types */
#include <critical.h>      /* Kernel lock */
#include <core_mgt.h>      /* Core manager */
#include <scheduler.h>     /* Kernel scheduler */
#include <x86memory.h>     /* x86-64 memory definitions */
#include <exceptions.h>    /* Exception manager */
#include <cpuInterrupt.h>  /* CPU interrupt settings */

/* Configuration files */
#include <config.h>

/* Header file */
#include <memory.h>

/* Unit test header */
#include <test_framework.h>
#ifdef _TESTING_FRAMEWORK_ENABLED
#if TEST_MEMMGR_ENABLED
#define static
#endif
#endif

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "X86_64 MEM"

/** @brief Kernel's page directory entry count */
#define KERNEL_PGDIR_ENTRY_COUNT 512

/** @brief Kernel PML4 entry offset. */
#define PML4_ENTRY_OFFSET 39
/** @brief Kernel PML3 entry offset. */
#define PML3_ENTRY_OFFSET 30
/** @brief Kernel PML2 entry offset. */
#define PML2_ENTRY_OFFSET 21
/** @brief Kernel PML1 entry offset. */
#define PML1_ENTRY_OFFSET 12
/** @brief Kernel page entry mask */
#define PG_ENTRY_OFFSET_MASK 0x1FF

/** @brief Page directory flag: 4KB page size. */
#define PAGE_FLAG_PAGE_SIZE_4KB 0x0000000000000000
/** @brief Page directory flag: 2MB page size. */
#define PAGE_FLAG_PAGE_SIZE_2MB 0x0000000000000080
/** @brief Page directory flag: 1GB page size. */
#define PAGE_FLAG_PAGE_SIZE_1GB 0x0000000000000080

/** @brief Page flag: executable page. */
#define PAGE_FLAG_XD 0x8000000000000000
/** @brief Page flag: page accessed. */
#define PAGE_FLAG_ACCESSED 0x0000000000000020
/** @brief Page flag: cache disabled for the page. */
#define PAGE_FLAG_CACHE_DISABLED 0x0000000000000010
/** @brief Page flag: cache write policy set to write through. */
#define PAGE_FLAG_CACHE_WT 0x0000000000000008
/** @brief Page flag: cache write policy set to write back. */
#define PAGE_FLAG_CACHE_WB 0x0000000000000000
/** @brief Page flag: access permission set to user. */
#define PAGE_FLAG_USER_ACCESS 0x0000000000000004
/** @brief Page flag: access permission set to kernel. */
#define PAGE_FLAG_SUPER_ACCESS 0x0000000000000000
/** @brief Page flag: access permission set to read and write. */
#define PAGE_FLAG_READ_WRITE 0x0000000000000002
/** @brief Page flag: access permission set to read only. */
#define PAGE_FLAG_READ_ONLY 0x0000000000000000
/** @brief Page flag: page present. */
#define PAGE_FLAG_PRESENT 0x0000000000000001
/** @brief Page flag: page present. */
#define PAGE_FLAG_IS_HW 0x0000000000000400
/** @brief Page flag: page global. */
#define PAGE_FLAG_GLOBAL 0x0000000000000100
/** @brief Page flag: PAT */
#define PAGE_FLAG_PAT 0x0000000000000080
/** @brief Page flag: Copy On Write */
#define PAGE_FLAG_COW 0x0000000000000200
/** @brief Page flag: Write Combining */
#define PAGE_FLAG_CACHE_WC (PAGE_FLAG_CACHE_DISABLED |  \
                            PAGE_FLAG_CACHE_WT       |  \
                            PAGE_FLAG_PAT)

/** @brief Defines the physical memory linear paging entry */
#define KERNEL_MEM_PML4_ENTRY 510
/** @brief Defines the the kernel directory entry */
#define KERNEL_PML4_KERNEL_ENTRY 511

/** @brief Defines the the kernel temporary boot directory entry */
#define KERNEL_PML4_BOOT_TMP_ENTRY 1

/** @brief Page fault error code: page protection violation. */
#define PAGE_FAULT_ERROR_PROT_VIOLATION 0x1
/** @brief Page fault error code: fault on a write. */
#define PAGE_FAULT_ERROR_WRITE 0x2
/** @brief Page fault error code: fault in user mode. */
#define PAGE_FAULT_ERROR_USER 0x4
/** @brief Page fault error code: fault on instruction fetch. */
#define PAGE_FAULT_ERROR_EXEC 0x10

/** @brief Defines the maximal physical address for memory */
#define KERNEL_MAX_MEM_PHYS 0x8000000000ULL
/** @brief Represents 1GB */
#define KERNEL_MEM_1G 0x40000000ULL

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief
 * Defines a table of contiguous physical memory used for reference and
 * metadata management.
 */
typedef struct frame_meta_table_t
{
    /** @brief First frame in the table. */
    uintptr_t firstFrame;

    /** @brief Last frame in the table. */
    uintptr_t lastFrame;

    /** @brief Reference count table */
    uint16_t* pRefCountTable;

    /** @brief Table lock */
    kernel_spinlock_t lock;

    struct frame_meta_table_t* pNext;
} frame_meta_table_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the memory manager to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the memory manager to ensure correctness of
 * execution. Due to the critical nature of the memory manager, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define MEM_ASSERT(COND, MSG, ERROR) {                      \
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/**
 * @brief Align a value on boundaries. If not aligned, the value is aligned on
 * the next boundary.
 *
 * @param[in] VALUE The value to align.
 * @param[in] ALIGN_BOUND The boundary to use.
 */
#define ALIGN_UP(VALUE, ALIGN_BOUND) (((VALUE) + ((ALIGN_BOUND) - 1)) & \
                                      (~((ALIGN_BOUND) - 1)))

/**
 * @brief Align a value on boundaries. If not aligned, the value is aligned on
 * the previous boundary.
 *
 * @param[in] VALUE The value to align.
 * @param[in] ALIGN_BOUND The boundary to use.
 */
#define ALIGN_DOWN(VALUE, ALIGN_BOUND) ((VALUE) & (~((ALIGN_BOUND) - 1)))

/**
 * @brief Gets the virtual address in the linear physical-to-virtual memory
 * space.
 *
 * @param[in] PHYS_MEM_ADDR The physical memory address to convert.
 */
#define GET_VIRT_MEM_ADDR(PHYS_MEM_ADDR) (                            \
    _makeCanonical(((uintptr_t)(PHYS_MEM_ADDR) +                      \
                   (KERNEL_MEM_PML4_ENTRY * 512ULL * KERNEL_MEM_1G)), \
                   false)                                             \
)

/**
 * @brief Gets the physical address in the linear physical-to-virtual memory
 * space.
 *
 * @param[in] VIRT_MEM_ADDR The virtual memory address to convert.
 */
#define GET_PHYS_MEM_ADDR(VIRT_MEM_ADDR) (                              \
    _makeCanonical(((uintptr_t)(VIRT_MEM_ADDR) -                        \
                   (KERNEL_MEM_PML4_ENTRY * 512ULL * KERNEL_MEM_1G)),   \
                   true)                                                \
)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

#if MEMORY_MGR_DEBUG_ENABLED

/**
 * @brief Prints the kernel memory map.
 *
 * @details Prints the kernel memory map using the standard kernel output
 * methods. This function only shows the virtual memory ranges used by the
 * kernel.
 */
static void _printKernelMap(void);

#endif /* #if MEMORY_MGR_DEBUG_ENABLED */

/**
 * @brief Page fault handler.
 *
 * @details Page fault handler. Manages page fault occuring while a thread is
 * running. The handler might call a panic if the faul cannot be resolved.
 *
 * @param[in, out] pCurrentThread The thread executing while the page fault
 * occured.
 */
static void _pageFaultHandler(kernel_thread_t* pCurrentThread);

/**
 * @brief Makes the address passed as parameter canonical.
 *
 * @details Makes the address passed as parameter canonical. This means it
 * extends its last address bit sign.
 *
 * @param[in] kAddress The address to make canonical.
 * @param[in] kIsPhysical Tells if the address is physical or virtual.
 *
 * @return The canonical address is returned;
 */
static inline uintptr_t _makeCanonical(const uintptr_t kAddress,
                                       const bool      kIsPhysical);

/**
 * @brief Adds a free memory block to a memory list.
 *
 * @details Adds a free memory block to a memory list. The list will be kept
 * sorted by base address in a ascending fashion.
 *
 * @param[out] pList The memory list to add the block to.
 * @param[in] baseAddress The first address of the memory block to add.
 * @param[in] kLength The size, in bytes of the memory region to add.
 */
static void _addBlock(mem_list_t*  pList,
                      uintptr_t    baseAddress,
                      const size_t kLength);

/**
 * @brief Removes a memory block to a memory list.
 *
 * @details Removes a memory block to a memory list. The list will be kept
 * sorted by base address in a ascending fashion.
 *
 * @param[out] pList The memory list to remove the block from.
 * @param[in] baseAddress The first address of the memory block to remove.
 * @param[in] kLength The size, in bytes of the memory region to remove.
 */
static void _removeBlock(mem_list_t*  pList,
                         uintptr_t    baseAddress,
                         const size_t kLength);

/**
 * @brief Returns a block for a memory list and removes it.
 *
 * @details Returns a block for a memory list and removes it. The list will be
 * kept sorted by base address in a ascending fashion.
 *
 * @param[out] pList The memory list to get the block from.
 * @param[in] kLength The size, in bytes of the memory region to get.
 */
static uintptr_t _getBlock(mem_list_t* pList, const size_t kLength);

/**
 * @brief Returns a block from the end of a memory list and removes it.
 *
 * @details Returns a block from the end of a memory list and removes it.
 * The list will be kept sorted by base address in a ascending fashion.
 *
 * @param[out] pList The memory list to get the block from.
 * @param[in] kLength The size, in bytes of the memory region to get.
 */
static uintptr_t _getBlockFromEnd(mem_list_t* pList, const size_t kLength);

/**
 * @brief Kernel memory frame allocation.
 *
 * @details Kernel memory frame allocation. This method gets the desired number
 * of contiguous frames from the kernel frame pool and allocate them.
 *
 * @param[in] kFrameCount The number of desired frames to allocate.
 *
 * @return The address of the first frame of the contiguous block is
 * returned.
 */
static uintptr_t _allocateFrames(const size_t kFrameCount);

/**
 * @brief Memory frames release.
 *
 * @details Memory frames release. This method releases the memory frames
 * to the free frames pool. Releasing already free or out of bound frame will
 * generate a kernel panic.
 *
 * @param[in] kBaseAddress The base address of the contiguous frame pool to
 * release.
 * @param[in] kFrameCount The number of desired frames to release.
 */
static void _releaseFrames(const uintptr_t kBaseAddress,
                           const size_t    kFrameCount);

/**
 * @brief Kernel memory pages allocation.
 *
 * @details Kernel memory pages allocation. This method gets the desired number
 * of contiguous pages from the kernel pages pool and allocate them.
 *
 * @param[in] kPageCount The number of desired pages to allocate.
 *
 * @return The address of the first page of the contiguous block is
 * returned.
 */
static uintptr_t _allocateKernelPages(const size_t kPageCount);

/**
 * @brief Kernel memory page release.
 *
 * @details Kernel memory page release. This method rekeases the kernel pages
 * to the kernel pages pool. Releasing already free or out of bound pages will
 * generate a kernel panic.
 *
 * @param[in] kBaseAddress The base address of the contiguous pages pool to
 * release.
 * @param[in] kPageCount The number of desired pages to release.
 */
static void _releaseKernelPages(const uintptr_t kBaseAddress,
                                const size_t    kPageCount);


/**
 * @brief Tells if a memory region is already mapped in the a page table.
 *
 * @details Tells if a memory region is already mapped in the a page table.
 * Returns false if the region is not mapped, true otherwise.
 *
 * @param[in] kVirtualAddress The base virtual address to check for mapping.
 * @param[in] pageCount The number of pages to check for mapping.
 * @param[in] kPageDir The page directory to use for the search
 * @param[in] kCheckFull Tells if the full range shall be mapped to return true.
 *
 * @return Returns false if the region is not mapped, true otherwise.
 */
static bool _memoryMgrIsMapped(const uintptr_t kVirtualAddress,
                               size_t          pageCount,
                               const uintptr_t kPageDir,
                               const bool      kCheckFull);

/**
 * @brief Maps the virtual address to the physical address in a page directory.
 *
 * @details Maps the virtual address to the physical address in a page
 * directory. If the addresses or sizes are not correctly aligned, or if the
 * mapping already exists, an error is retuned. If the address is not in the
 * free memory frames pool and the klags does not mention hardware mapping, an
 * error is returned.
 *
 * @param[in] kVirtualAdderss The virtual address to map.
 * @param[in] kPhysicalAddress The physical address to map.
 * @param[in] kPageCount The number of pages to map.
 * @param[in] kFlags The flags used for mapping.
 * @param[in] kPageDir The page directory to use.
 *
 * @return The success or error status is returned.
 */
static OS_RETURN_E _memoryMgrMap(const uintptr_t kVirtualAddress,
                                 const uintptr_t kPhysicalAddress,
                                 const size_t    kPageCount,
                                 const uint32_t  kFlags,
                                 const uintptr_t kPageDir);

/**
 * @brief Unmaps the virtual address in a page directory.
 *
 * @details Unmaps the virtual address to the physical address in in a page
 * directory.
 *
 * @param[in] kVirtualAdderss The virtual address to unmap.
 * @param[in] kPageCount The number of pages to unmap.
 * @param[in] kPageDir The page directory to use.
 */
static OS_RETURN_E _memoryMgrUnmap(const uintptr_t kVirtualAddress,
                                   const size_t    kPageCount,
                                   const uintptr_t kPageDir);

/**
 * @brief Returns the physical address of a virtual address mapped in the
 * current page directory.
 *
 * @details Returns the physical address of a virtual address mapped in the
 * current page directory. If not found, MEMMGR_PHYS_ADDR_ERROR is returned.
 *
 * @param[in] kVirtualAddress The virtual address to lookup.
 * @param[in] kPageDir The page directory to use.
 * @param[out] pFlags The memory flags used for the mapping. Can be NULL.
 *
 * @returns The physical address of a virtual address mapped in the
 * current page directory. If not found, MEMMGR_PHYS_ADDR_ERROR is returned.
 */
static uintptr_t _memoryMgrGetPhysAddr(const uintptr_t kVirtualAddress,
                                       const uintptr_t kPageDir,
                                       uint32_t*       pFlags);

/**
 * @brief Detects the hardware memory presents in the system.
 *
 * @details Detects the hardware memory presents in the system. Each
 * memory range is tagged as memory. This function uses the FDT to
 * register the available memory and reserved memory.
 */
static void _memoryMgrDetectMemory(void);

/**
 * @brief Creates the frame metadata table.
 *
 * @brief Creates the frame metadata table. The memory for the metadata will
 * be allocated from the memory block themselves.
 */
static void _memoryMgrCreateFramesMeta(void);

/**
 * @brief Setups the memory tables used in the kernel.
 *
 * @details Setups the memory tables used in the kernel. The memory map is
 * generated, the pages and frames lists are also created.
 */
static void _memoryMgrInitKernelFreePages(void);

/**
 * @brief Maps a kernel section to a page directory mapped in virtual memory.
 *
 * @details Maps a kernel section to a page directory mapped in virtual memory.
 * Frames can be allocated if needed. The function checks if the previous kernel
 * section is not overlapping the current one.
 *
 * @param[out] pLastSectionStart Start virtual address of the previous kernel
 * section.
 * @param[out] pLastSectionEnd End virtual address of the previous kernel
 * section.
 * @param[in] kRegionStartAddr Start virtual address of the section to map.
 * @param[in] kRegionEndAddr End virtual address of the section to map.
 * @param[in] kFlags Mapping flags.
 */
static void _memoryMgrMapKernelRegion(uintptr_t*      pLastSectionStart,
                                      uintptr_t*      pLastSectionEnd,
                                      const uintptr_t kRegionStartAddr,
                                      const uintptr_t kRegionEndAddr,
                                      const uint32_t  kFlags);

/**
 * @brief Initializes paging structures for the kernel.
 *
 * @details Initializes paging structures for the kernel. This function will
 * select an available memory region to allocate the memory required for the
 * kernel. Then the kernel will be mapped to memory and paging is enabled for
 * the kernel.
 */
static void _memoryMgrMapKernel(void);

/**
 * @brief Releases the memory used by a process.
 *
 * @details Releases the memory used by a process. This function only releases
 * the frames that are used un user space and the frames that compose the user
 * land of the page directory.
 *
 * @param[in] kPhysTable The page directory physical address to release.
 * @param[in] kBaseVirtAddr The start virtual address at which the page dir
 * shall be released.
 * @param[in] kLevel The page directory level to release.
 */
static void _releasePageDir(const uintptr_t kPhysTable,
                            const uintptr_t kBaseVirtAddr,
                            const uint8_t   kLevel);

/**
 * @brief User memory pages allocation.
 *
 * @details User memory pages allocation. This method gets the desired number
 * of contiguous pages from the user pages pool and allocate them.
 *
 * @param[in] kPageCount The number of desired pages to allocate.
 * @param[in] kpProcess The process from which the pages should be allocated.
 * @param[in] kFromTop Tells if the pages must be allocated from the top or
 * the bottom of the memory space.
 *
 * @return The bottom address of the first page of the contiguous block is
 * returned.
 */
static uintptr_t _allocateUserPages(const size_t            kPageCount,
                                    const kernel_process_t* kpProcess,
                                    const bool              kFromTop);

/**
 * @brief User memory page release.
 *
 * @details User memory page release. This method releases the user pages
 * to the user pages pool. Releasing already free or out of bound pages will
 * generate a user panic.
 *
 * @param[in] kBaseAddress The base address of the contiguous pages pool to
 * release.
 * @param[in] kPageCount The number of desired pages to release.
 * @param[in] kpProcess The process to which the pages should be released.
 *
 */
static void _releaseUserPages(const uintptr_t         kBaseAddress,
                              const size_t            kPageCount,
                              const kernel_process_t* kpProcess);

/**
 * @brief Handles a CopyOnWrite event.
 *
 * @details Handles a CopyOnWrite event. If the faulted page is COW, copies it
 * if the reference count is greater than 1, otherwise simply set the page as
 * writable.
 *
 * @param[in] kFaultVirtAddr The virtual address that generated to fault.
 * @param[in] kPhysAddr The physical address that corresponds to the faulted
 * page.
 * @param[in] kpThread The thread that raised the COW exception.
 *
 * @return The function returns the success or error status.
 */
static OS_RETURN_E _memoryManageCOW(const uintptr_t        kFaultVirtAddr,
                                    const uintptr_t        kPhysAddr,
                                    const kernel_thread_t* kpThread);

/**
 * @brief Copies a page directory entry.
 *
 * @details Copies a page directory entry. This recursive function copies the
 * entierety of a page directory entry and send recursively copies all entries
 * of the copied page table.
 *
 * @param[in, out] pSrcLevel The page table entry to copy.
 * @param[out] pDstLevel The page table that receives the copy.
 * @param[in, out] pVirtAddress The virtual address that corresponds to the
 * first entry of the page table to copy. This address is updated at the end of
 * the copy.
 * @param[in] kVirtAddressMax The maximal virtual address to manage, after
 * reaching this value, the rest of the entries are ignored.
 * @param[in] kLevel The page table level in the page directory (4 to 1).
 * @param[in] kSetCow On copy, tells if the copied and source pages must be
 * set to Copy On Write.
 *
 * @return The function returns the success or error status.
 */
static OS_RETURN_E _copyPgDirEntry(uintptr_t*      pSrcLevel,
                                   uintptr_t*      pDstLevel,
                                   uintptr_t*      pVirtAddress,
                                   const uintptr_t kVirtAddressMax,
                                   const uint8_t   kLevel,
                                   const bool      kSetCOW);

/**
 * @brief Get the reference table and intex in the frames metadata tables.
 *
 * @details Get the reference table and intex in the frames metadata tables.
 * The function fils the ppTable and pEntryIdx buffers with the corresponding
 * values.
 *
 * @param[in] kPhysAddr The physical address for which the reference shall be
 * retrieved.
 * @param[out] ppTable The buffer the receives the pointer to the frame metadata
 * table.
 * @param[out] pEntryIdx The buffer that is filled with the entry index in the
 * frame metadata table that corresponds to the physical address.
 */
static inline void _getReferenceIndexTable(const uintptr_t      kPhysAddr,
                                           frame_meta_table_t** ppTable,
                                           size_t*              pEntryIdx);

/**
 * @brief Get the reference counter pointer for a given physical address and
 * locks the corresponding frame metadata table.
 *
 * @details Get the reference counter pointer for a given physical address and
 * locks the corresponding frame metadata table.
 *
 * @param[in] kPhysAddr The physical address for which the reference count
 * pointer shall be retrieved.
 *
 * @return The function returns the pointer to the reference counter for the
 * provided physical address.
 */
static uint16_t* _getAndLockReferenceCount(const uintptr_t kPhysAddr);

/**
 * @brief Unlocks the corresponding frame metadata table for a given physical
 * address.
 *
 * @details Unlocks the corresponding frame metadata table for a given physical
 * address.
 *
 * @param[in] kPhysAddr The physical address for which the frame metadata table
 * shall be unlocked.
 */
static void _unlockReferenceCount(const uintptr_t kPhysAddr);

/* TODO: Doc */
static inline uint64_t _translateFlags(const uint32_t kFlags);

static OS_RETURN_E _memoryMgrMapUser(uintptr_t*     pTableLevel,
                                     uintptr_t*     pVirtAddress,
                                     uintptr_t*     pPhysicalAddress,
                                     size_t*        pPageCount,
                                     const uint8_t  kLevel,
                                     const uint64_t kPageFlags);
static OS_RETURN_E _memoryMgrUnmapUser(uintptr_t*     pTableLevel,
                                       uintptr_t*     pVirtAddress,
                                       size_t*        pPageCount,
                                       const uint8_t  kLevel);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Kernel symbols mapping: Low AP startup address start. */
extern uint8_t _START_LOW_AP_STARTUP_ADDR;
/** @brief Kernel symbols mapping: High AP startup address start. */
extern uint8_t _END_LOW_AP_STARTUP_ADDR;
/** @brief Kernel symbols mapping: Bios call region address start. */
extern uint8_t _START_BIOS_CALL_ADDR;
/** @brief Kernel symbols mapping: Bios call region address end. */
extern uint8_t _END_BIOS_CALL_ADDR;
/** @brief Kernel symbols mapping: Low startup address start. */
extern uint8_t _START_LOW_STARTUP_ADDR;
/** @brief Kernel symbols mapping: Low startup address end. */
extern uint8_t _END_LOW_STARTUP_ADDR;
/** @brief Kernel symbols mapping: Code address start. */
extern uint8_t _START_TEXT_ADDR;
/** @brief Kernel symbols mapping: Code address end. */
extern uint8_t _END_TEXT_ADDR;
/** @brief Kernel symbols mapping: RO data address start. */
extern uint8_t _START_RO_DATA_ADDR;
/** @brief Kernel symbols mapping: RO data address end. */
extern uint8_t _END_RO_DATA_ADDR;
/** @brief Kernel symbols mapping: RW Data address start. */
extern uint8_t _START_RW_DATA_ADDR;
/** @brief Kernel symbols mapping: RW Data address end. */
extern uint8_t _END_RW_DATA_ADDR;
/** @brief Kernel symbols mapping: Stacks address start. */
extern uint8_t _KERNEL_STACKS_BASE;
/** @brief Kernel symbols mapping: Stacks address end. */
extern uint8_t _KERNEL_STACKS_SIZE;
/** @brief Kernel symbols mapping: Heap address start. */
extern uint8_t _KERNEL_HEAP_BASE;
/** @brief Kernel symbols mapping: Heap address end. */
extern uint8_t _KERNEL_HEAP_SIZE;
/** @brief Kernel symbols mapping: Kernel total memory start. */
extern uint8_t _KERNEL_MEMORY_START;
/** @brief Kernel symbols mapping: Kernel total memory end. */
extern uint8_t _KERNEL_MEMORY_END;

#ifdef _TESTING_FRAMEWORK_ENABLED
/** @brief Kernel symbols mapping: Test buffer start. */
extern uint8_t _KERNEL_TEST_BUFFER_BASE;
/** @brief Kernel symbols mapping: Test buffer size. */
extern uint8_t _KERNEL_TEST_BUFFER_SIZE;
#endif

/** @brief Kernel page directory intialized at boot */
extern uintptr_t _kernelPGDir[KERNEL_PGDIR_ENTRY_COUNT];

/** @brief Kernel frame-to-page entries */
extern uintptr_t _physicalMapDir[KERNEL_PGDIR_ENTRY_COUNT];

/************************* Exported global variables **************************/
/** @brief CPU physical addressing width */
uint8_t physAddressWidth = 0;

/** @brief CPU virtual addressing width */
uint8_t virtAddressWidth = 0;

/************************** Static global variables ***************************/
/** @brief Physical memory chunks list */
static mem_list_t sPhysMemList;

/** @brief Kernel free page list list */
static mem_list_t sKernelFreePagesList;

/** @brief Kernel virtual memory bounds */
static mem_range_t sKernelVirtualMemBounds;

/** @brief CPU physical addressing width mask */
static uintptr_t sPhysAddressWidthMask = 0;

/** @brief CPU virtual addressing canonical bound */
static uintptr_t sCanonicalBound = 0;

/** @brief Kernel page directory virtual pointer */
static uintptr_t* spKernelPageDir = (uintptr_t*)&_kernelPGDir;

/** @brief Memory manager main lock */
static kernel_spinlock_t sLock = KERNEL_SPINLOCK_INIT_VALUE;

/** @brief Frames metadata tables */
static frame_meta_table_t* spFramesMeta = NULL;



/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

#if MEMORY_MGR_DEBUG_ENABLED

static void _printKernelMap(void)
{
    kqueue_node_t* pMemNode;
    mem_range_t*   pMemRange;

#if MEMORY_MGR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "=== Kernel memory layout");
    syslog(SYSLOG_LEVEL_DEBUG,
                 MODULE_NAME,
                 "Startup AP  low 0x%p -> 0x%p | "PRIPTR"KB",
                 &_START_LOW_AP_STARTUP_ADDR,
                 &_END_LOW_AP_STARTUP_ADDR,
                 ((uintptr_t)&_END_LOW_AP_STARTUP_ADDR -
                 (uintptr_t)&_START_LOW_AP_STARTUP_ADDR) >> 10);
    syslog(SYSLOG_LEVEL_DEBUG,
                 MODULE_NAME,
                 "Startup low     0x%p -> 0x%p | "PRIPTR"KB",
                 &_START_LOW_STARTUP_ADDR,
                 &_END_LOW_STARTUP_ADDR,
                 ((uintptr_t)&_END_LOW_STARTUP_ADDR -
                 (uintptr_t)&_START_LOW_STARTUP_ADDR) >> 10);
    syslog(SYSLOG_LEVEL_DEBUG,
                 MODULE_NAME,
                 "Code            0x%p -> 0x%p | "PRIPTR"KB",
                 &_START_TEXT_ADDR,
                 &_END_TEXT_ADDR,
                 ((uintptr_t)&_END_TEXT_ADDR -
                 (uintptr_t)&_START_TEXT_ADDR) >> 10);
    syslog(SYSLOG_LEVEL_DEBUG,
                 MODULE_NAME,
                 "RO-Data         0x%p -> 0x%p | "PRIPTR"KB",
                 &_START_RO_DATA_ADDR,
                 &_END_RO_DATA_ADDR,
                 ((uintptr_t)&_END_RO_DATA_ADDR -
                 (uintptr_t)&_START_RO_DATA_ADDR) >> 10);
    syslog(SYSLOG_LEVEL_DEBUG,
                 MODULE_NAME,
                 "RW-Data         0x%p -> 0x%p | "PRIPTR"KB",
                 &_START_RW_DATA_ADDR,
                 &_END_RW_DATA_ADDR,
                 ((uintptr_t)&_END_RW_DATA_ADDR -
                 (uintptr_t)&_START_RW_DATA_ADDR) >> 10);
    syslog(SYSLOG_LEVEL_DEBUG,
                 MODULE_NAME,
                 "Stacks          0x%p -> 0x%p | "PRIPTR"KB",
                 &_KERNEL_STACKS_BASE,
                 &_KERNEL_STACKS_BASE + (uintptr_t)&_KERNEL_STACKS_SIZE,
                 ((uintptr_t)&_KERNEL_STACKS_SIZE) >> 10);
    syslog(SYSLOG_LEVEL_DEBUG,
                 MODULE_NAME,
                 "Heap            0x%p -> 0x%p | "PRIPTR"KB",
                 &_KERNEL_HEAP_BASE,
                 &_KERNEL_HEAP_BASE + (uintptr_t)&_KERNEL_HEAP_SIZE,
                 ((uintptr_t)&_KERNEL_HEAP_SIZE) >> 10);
#endif

    pMemNode = sPhysMemList.pQueue->pHead;
    while(pMemNode != NULL)
    {
        pMemRange = (mem_range_t*)pMemNode->pData;

#if MEMORY_MGR_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "Free physical memory regions 0x%p -> 0x%p",
               pMemRange->base,
               pMemRange->limit);
#endif

        pMemNode = pMemNode->pNext;
    }

    pMemNode = sKernelFreePagesList.pQueue->pHead;
    while(pMemNode != NULL)
    {
        pMemRange = (mem_range_t*)pMemNode->pData;

#if MEMORY_MGR_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "Free kernel virtual memory regions 0x%p -> 0x%p",
               pMemRange->base,
               pMemRange->limit);
#endif

        pMemNode = pMemNode->pNext;
    }
}

#endif /* #if MEMORY_MGR_DEBUG_ENABLED */

static void _pageFaultHandler(kernel_thread_t* pCurrentThread)
{
    uintptr_t       faultAddress;
    uintptr_t       physAddr;
    uint32_t        errorCode;
    uint32_t        flags;
    bool            staleEntry;
    OS_RETURN_E     error;

    /* Get the fault address and error code */
    __asm__ __volatile__ ("mov %%cr2, %0" : "=r"(faultAddress));
    errorCode = ((virtual_cpu_t*)pCurrentThread->pVCpu)->intContext.errorCode;

#if MEMORY_MGR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Page fault: 0x%p | Code: %x",
           faultAddress,
           errorCode);
#endif

    /* Check if the fault occured because we hit a stale TLB entry */
    physAddr = memoryMgrGetPhysAddr(faultAddress,
                                    pCurrentThread->pProcess,
                                    &flags);
    if(physAddr != MEMMGR_PHYS_ADDR_ERROR)
    {
        staleEntry = true;
        if((errorCode & PAGE_FAULT_ERROR_PROT_VIOLATION) ==
           PAGE_FAULT_ERROR_PROT_VIOLATION)
        {
            /* Check the privilege level */
            if((errorCode & PAGE_FAULT_ERROR_USER) == PAGE_FAULT_ERROR_USER &&
               (flags & MEMMGR_MAP_USER) != MEMMGR_MAP_USER)
            {
                staleEntry = false;
            }

            /* Check if execution is allowed */
            if((errorCode & PAGE_FAULT_ERROR_EXEC) == PAGE_FAULT_ERROR_EXEC &&
               (flags & MEMMGR_MAP_EXEC) != MEMMGR_MAP_EXEC)
            {
                staleEntry = false;
            }

            /* Check the access rights */
            if((errorCode & PAGE_FAULT_ERROR_WRITE) == PAGE_FAULT_ERROR_WRITE)
            {
                /* Check if the entry is set as COW */
                if((flags & MEMMGR_MAP_COW) == MEMMGR_MAP_COW)
                {
                    error = _memoryManageCOW(faultAddress,
                                             physAddr,
                                             pCurrentThread);
                    if(error != OS_NO_ERR)
                    {
                        staleEntry = false;
                    }
                }
                /* Check if the error is due to a stale entry */
                else if((flags & MEMMGR_MAP_RW) != MEMMGR_MAP_RW)
                {
                    staleEntry = false;
                }
            }
        }
        else if((errorCode & PAGE_FAULT_ERROR_EXEC) == PAGE_FAULT_ERROR_EXEC &&
                (flags & MEMMGR_MAP_EXEC) != MEMMGR_MAP_EXEC)
        {
            staleEntry = false;
        }
        else if(errorCode != 0)
        {
            staleEntry = false;
        }

        if(staleEntry == true)
        {

#if MEMORY_MGR_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Stale entry fault: 0x%p | Code: %x",
                   faultAddress,
                   errorCode);
#endif

            cpuInvalidateTlbEntry(faultAddress);
            return;
        }
    }

    /* Set reason page fault and reason data the address,
     * also get the reason code in the interrupt info
     */
    pCurrentThread->errorTable.exceptionId  = PAGE_FAULT_EXC_LINE;
    pCurrentThread->errorTable.segfaultAddr = faultAddress;
    pCurrentThread->errorTable.instAddr =
        cpuGetContextIP(pCurrentThread->pVCpu);
    pCurrentThread->errorTable.pExecVCpu = pCurrentThread->pVCpu;
    error = signalThread(pCurrentThread, THREAD_SIGNAL_SEGV);
    MEM_ASSERT(error == OS_NO_ERR, "Failed to signal segfault", error);
}

static inline uintptr_t _makeCanonical(const uintptr_t kAddress,
                                       const bool      kIsPhysical)
{
    if(kIsPhysical == false)
    {
        if((kAddress & (1ULL << (virtAddressWidth - 1))) != 0)
        {
            return kAddress | ~sCanonicalBound;
        }
        else
        {
            return kAddress & sCanonicalBound;
        }
    }
    else
    {
        return kAddress & sPhysAddressWidthMask;
    }
}

static void _addBlock(mem_list_t*  pList,
                      uintptr_t    baseAddress,
                      const size_t kLength)
{
    kqueue_node_t* pCursor;
    kqueue_node_t* pNewNode;
    kqueue_node_t* pSaveCursor;
    mem_range_t*   pRange;
    mem_range_t*   pNextRange;
    uintptr_t      limit;
    bool           merged;

    limit = baseAddress + kLength;

#if MEMORY_MGR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Adding memory block 0x%p -> 0x%p",
           baseAddress,
           limit);
#endif

    MEM_ASSERT(pList != NULL,
               "Tried to add a memory block to a NULL list",
               OS_ERR_NULL_POINTER);

    MEM_ASSERT((baseAddress & PAGE_SIZE_MASK) == 0 &&
               (kLength & PAGE_SIZE_MASK) == 0 &&
               kLength != 0,
               "Tried to add a non aligned block",
               OS_ERR_UNAUTHORIZED_ACTION);

    /* Manage rollover */
    MEM_ASSERT(limit > baseAddress,
               "Tried to add a rollover memory block",
               OS_ERR_INCORRECT_VALUE);

    KERNEL_LOCK(pList->lock);

    /* Try to merge the new block, the list is ordered by base address asc */
    pCursor = pList->pQueue->pHead;
    merged = false;
    while(pCursor != NULL)
    {
        pRange = (mem_range_t*)pCursor->pData;

        /* If the base address is lower than the new base and the limit is also
         * greather than the new limit, we are adding an already free block.
         */
        MEM_ASSERT((baseAddress < pRange->base &&
                    limit <= pRange->base)||
                   (baseAddress >= pRange->limit),
                   "Adding an already free block",
                   OS_ERR_UNAUTHORIZED_ACTION);

        /* If the new block is before but needs merging */
        if(baseAddress < pRange->base && limit == pRange->base)
        {

#if MEMORY_MGR_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Merging with block 0x%p -> 0x%p",
                   pRange->base,
                   pRange->limit);
#endif

            /* Extend left */
            pRange->base  = baseAddress;
            pCursor->priority = KERNEL_VIRTUAL_ADDR_MAX - baseAddress;
            merged = true;
        }
        /* If the new block is after but needs merging */
        else if(baseAddress == pRange->limit)
        {
#if MEMORY_MGR_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Merging with block 0x%p -> 0x%p",
                   pRange->base,
                   pRange->limit);
#endif

            /* Check if next if not overlapping */
            if(pCursor->pNext != NULL)
            {
                pNextRange = (mem_range_t*)pCursor->pNext->pData;

                /* See if we can merge this one too */
                if(pNextRange->base == limit)
                {
                    pSaveCursor = pCursor;
                    pCursor = pCursor->pNext;

                    pNextRange->base = pRange->base;
                    pCursor->priority = KERNEL_VIRTUAL_ADDR_MAX -
                                        pNextRange->base;
                    merged = true;


                    /* Remove node */
                    kfree(pSaveCursor->pData);
                    kQueueRemove(pList->pQueue, pSaveCursor, true);
                    kQueueDestroyNode(&pSaveCursor);
                }
                else if(((mem_range_t*)pCursor->pNext->pData)->base < limit)
                {
                    MEM_ASSERT(false,
                               "Adding an already free block",
                               OS_ERR_UNAUTHORIZED_ACTION);
                }
            }

            if(merged == false)
            {
                /* Extend up */
                pRange->limit = limit;
                merged = true;
            }
        }
        else if(baseAddress < pRange->base)
        {
            /* We are before this block, we already checked if not overlapping
             * just stop iterating
             */
            break;
        }
        else
        {
            /* Nothing to do with this block, continue */
            pCursor = pCursor->pNext;
            continue;
        }

        pCursor = pCursor->pNext;
    }

    /* If not merged, create a new block in the list */
    if(merged == false)
    {
        pRange = kmalloc(sizeof(mem_range_t));
        MEM_ASSERT(pRange != NULL,
                   "Failed to allocate new memory range",
                   OS_ERR_NO_MORE_MEMORY);

        pNewNode = kQueueCreateNode(pRange, true);

        pRange->base  = baseAddress;
        pRange->limit = limit;

        kQueuePushPrio(pNewNode,
                       pList->pQueue,
                       KERNEL_VIRTUAL_ADDR_MAX - baseAddress);

#if MEMORY_MGR_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "Added new block 0x%p -> 0x%p",
               baseAddress,
               limit);
#endif

    }

    KERNEL_UNLOCK(pList->lock);
}

static void _removeBlock(mem_list_t*  pList,
                         uintptr_t    baseAddress,
                         const size_t kLength)
{
    kqueue_node_t* pCursor;
    kqueue_node_t* pNewNode;
    kqueue_node_t* pSaveCursor;
    mem_range_t*   pRange;
    uintptr_t      limit;
    uintptr_t      saveLimit;

    MEM_ASSERT(pList != NULL,
               "Tried to remove a memory block from a NULL list",
               OS_ERR_NULL_POINTER);

    MEM_ASSERT((baseAddress & PAGE_SIZE_MASK) == 0 &&
               (kLength & PAGE_SIZE_MASK) == 0,
               "Tried to remove a non aligned block",
               OS_ERR_UNAUTHORIZED_ACTION);

    limit = baseAddress + kLength;

#if MEMORY_MGR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Removing memory block 0x%p -> 0x%p",
           baseAddress,
           limit);
#endif

    KERNEL_LOCK(pList->lock);

    /* Try to find all the regions that might be impacted */
    pCursor = pList->pQueue->pHead;
    while(pCursor != NULL && limit != 0)
    {
        pRange = (mem_range_t*)pCursor->pData;

        /* Check if fully contained */
        if(pRange->base >= baseAddress && pRange->limit <= limit)
        {
            pSaveCursor = pCursor;
            pCursor = pCursor->pNext;

#if MEMORY_MGR_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Removing block 0x%p -> 0x%p",
                   pRange->base,
                   pRange->limit);
#endif

            /* Update the rest to remove */
            baseAddress = pRange->limit;
            if(limit == pRange->limit)
            {
                limit = 0;
            }

            kfree(pSaveCursor->pData);
            kQueueRemove(pList->pQueue, pSaveCursor, true);
            kQueueDestroyNode(&pSaveCursor);
        }
        /* If up containted */
        else if(pRange->base < baseAddress && pRange->limit <= limit)
        {

#if MEMORY_MGR_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Reducing up block 0x%p -> 0x%p to 0x%p -> 0x%p",
                   pRange->base,
                   pRange->limit,
                   pRange->base,
                   baseAddress);
#endif

            pRange->limit = baseAddress;

            /* Update the rest to remove */
            if(limit == pRange->limit)
            {
                limit = 0;
            }
            else
            {
                baseAddress = pRange->limit;
            }
            pCursor = pCursor->pNext;
        }
        /* If down containted */
        else if(pRange->base >= baseAddress && pRange->limit > limit)
        {

#if MEMORY_MGR_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Reducing down block 0x%p -> 0x%p to 0x%p -> 0x%p",
                   pRange->base,
                   pRange->limit,
                   limit,
                   pRange->limit);
#endif

            /* Update the rest to remove */
            pRange->base = limit;

            /* We are done*/
            limit = 0;
        }
        /* If inside */
        else if(pRange->base < baseAddress && pRange->limit > limit)
        {
#if MEMORY_MGR_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Splitting block 0x%p -> 0x%p",
                   pRange->base,
                   pRange->limit);
#endif

            /* Save next limit */
            saveLimit = pRange->limit;

            /* Update the current region */
            pRange->limit = baseAddress;

            /* Get new base address */
            baseAddress = limit;

            /* Create new node */
            pRange = kmalloc(sizeof(mem_range_t));
            MEM_ASSERT(pRange != NULL,
                       "Failed to allocate new memory range",
                       OS_ERR_NO_MORE_MEMORY);

            pNewNode = kQueueCreateNode(pRange, true);

            pRange->base  = baseAddress;
            pRange->limit = saveLimit;

            kQueuePushPrio(pNewNode,
                           pList->pQueue,
                           KERNEL_VIRTUAL_ADDR_MAX - baseAddress);

#if MEMORY_MGR_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Added new block form removal 0x%p -> 0x%p",
                   baseAddress,
                   limit);
#endif

            /* We are done*/
            limit = 0;
        }
        else
        {
            pCursor = pCursor->pNext;
        }
    }

    KERNEL_UNLOCK(pList->lock);
}

static uintptr_t _getBlock(mem_list_t* pList, const size_t kLength)
{
    uintptr_t      retBlock;
    kqueue_node_t* pCursor;
    mem_range_t*   pRange;

    MEM_ASSERT((kLength & PAGE_SIZE_MASK) == 0,
               "Tried to get a non aligned block",
               OS_ERR_UNAUTHORIZED_ACTION);

    retBlock = 0;

    KERNEL_LOCK(pList->lock);

    /* Walk the list until we find a valid block */
    pCursor = pList->pQueue->pHead;
    while(pCursor != NULL)
    {
        pRange = (mem_range_t*)pCursor->pData;

        if(pRange->base + kLength <= pRange->limit ||
          ((pRange->base + kLength > pRange->base) && pRange->limit == 0))
        {
            retBlock = pRange->base;

            /* Reduce the node or remove it */
            if(pRange->base + kLength == pRange->limit)
            {

#if MEMORY_MGR_DEBUG_ENABLED
                syslog(SYSLOG_LEVEL_DEBUG,
                       MODULE_NAME,
                       "Removing block after alloc 0x%p -> 0x%p",
                       pRange->base,
                       pRange->limit);
#endif

                kfree(pCursor->pData);
                kQueueRemove(pList->pQueue, pCursor, true);
                kQueueDestroyNode(&pCursor);
            }
            else
            {
#if MEMORY_MGR_DEBUG_ENABLED
                syslog(SYSLOG_LEVEL_DEBUG,
                       MODULE_NAME,
                       "Reducing block after alloc 0x%p -> 0x%p to "
                       "0x%p -> 0x%p",
                       pRange->base,
                       pRange->limit,
                       pRange->base + kLength,
                       pRange->limit);
#endif

                pRange->base += kLength;
                pCursor->priority = KERNEL_VIRTUAL_ADDR_MAX - pRange->base;
            }
            break;
        }

        pCursor = pCursor->pNext;
    }

    KERNEL_UNLOCK(pList->lock);

    return retBlock;
}

static uintptr_t _getBlockFromEnd(mem_list_t* pList, const size_t kLength)
{
    uintptr_t      retBlock;
    kqueue_node_t* pCursor;
    mem_range_t*   pRange;

    MEM_ASSERT((kLength & PAGE_SIZE_MASK) == 0,
               "Tried to get a non aligned block",
               OS_ERR_UNAUTHORIZED_ACTION);

    retBlock = 0;

    KERNEL_LOCK(pList->lock);

    /* Walk the list until we find a valid block */
    pCursor = pList->pQueue->pTail;
    while(pCursor != NULL)
    {
        pRange = (mem_range_t*)pCursor->pData;

        if(pRange->base + kLength <= pRange->limit ||
           ((pRange->base + kLength > pRange->base) && pRange->limit == 0))
        {
            retBlock = pRange->limit - kLength;

            /* Reduce the node or remove it */
            if(pRange->base + kLength == pRange->limit)
            {

#if MEMORY_MGR_DEBUG_ENABLED
                syslog(SYSLOG_LEVEL_DEBUG,
                       MODULE_NAME,
                       "Removing block after alloc 0x%p -> 0x%p",
                       pRange->base,
                       pRange->limit);
#endif

                kfree(pCursor->pData);
                kQueueRemove(pList->pQueue, pCursor, true);
                kQueueDestroyNode(&pCursor);
            }
            else
            {
#if MEMORY_MGR_DEBUG_ENABLED
                syslog(SYSLOG_LEVEL_DEBUG,
                       MODULE_NAME,
                       "Reducing block after alloc 0x%p -> 0x%p to "
                       "0x%p -> 0x%p",
                       pRange->base,
                       pRange->limit,
                       pRange->base + kLength,
                       pRange->limit);
#endif

                pRange->limit -= kLength;
            }
            break;
        }

        pCursor = pCursor->pPrev;
    }

    KERNEL_UNLOCK(pList->lock);

    return retBlock;
}

static uintptr_t _allocateFrames(const size_t kFrameCount)
{
    uintptr_t physAddr;
    uintptr_t i;
    uint16_t* refCount;

    physAddr = _getBlock(&sPhysMemList, KERNEL_PAGE_SIZE * kFrameCount);

    if(physAddr != (uintptr_t)NULL)
    {
        /* Increment the reference count */
        for(i = 0; i < kFrameCount; ++i)
        {
            refCount = _getAndLockReferenceCount(physAddr);
            if(refCount != NULL)
            {
                MEM_ASSERT(*refCount == 0,
                           "Invalid reference count non zero",
                           OS_ERR_INCORRECT_VALUE);
                *refCount = 1;
            }
            _unlockReferenceCount(physAddr);
        }
    }

    return physAddr;
}

static void _releaseFrames(const uintptr_t kBaseAddress,
                           const size_t    kFrameCount)
{
    uintptr_t physAddr;
    uintptr_t i;
    uint16_t* refCount;

    physAddr = kBaseAddress;

    /* Increment the reference count */
    for(i = 0; i < kFrameCount; ++i)
    {
        refCount = _getAndLockReferenceCount(physAddr);
        if(refCount != NULL)
        {
            MEM_ASSERT(*refCount == 1,
                       "Released used frame",
                       OS_ERR_UNAUTHORIZED_ACTION);
            *refCount = *refCount - 1;
        }
        _unlockReferenceCount(physAddr);
    }

    _addBlock(&sPhysMemList,
              kBaseAddress,
              kFrameCount * KERNEL_PAGE_SIZE);
}

static uintptr_t _allocateKernelPages(const size_t kPageCount)
{
    return _getBlock(&sKernelFreePagesList, kPageCount * KERNEL_PAGE_SIZE);
}

static void _releaseKernelPages(const uintptr_t kBaseAddress,
                                const size_t    kPageCount)
{
    _addBlock(&sKernelFreePagesList,
              kBaseAddress,
              kPageCount * KERNEL_PAGE_SIZE);
}

static bool _memoryMgrIsMapped(const uintptr_t kVirtualAddress,
                               size_t          pageCount,
                               const uintptr_t kPageDir,
                               const bool      kCheckFull)
{
    uintptr_t  currVirtAddr;
    uintptr_t  nextPtable;
    uintptr_t* pPageTable[4];
    uint16_t   pmlEntry[4];
    int8_t     j;
    size_t     stride;

    MEM_ASSERT((kVirtualAddress & PAGE_SIZE_MASK) == 0,
               "Checking mapping for non aligned address",
               OS_ERR_INCORRECT_VALUE);

    currVirtAddr = kVirtualAddress;
    do
    {
        pmlEntry[3] = (currVirtAddr >> PML4_ENTRY_OFFSET) &
                        PG_ENTRY_OFFSET_MASK;
        pmlEntry[2] = (currVirtAddr >> PML3_ENTRY_OFFSET) &
                        PG_ENTRY_OFFSET_MASK;
        pmlEntry[1] = (currVirtAddr >> PML2_ENTRY_OFFSET) &
                        PG_ENTRY_OFFSET_MASK;
        pmlEntry[0] = (currVirtAddr >> PML1_ENTRY_OFFSET) &
                        PG_ENTRY_OFFSET_MASK;

        for(j = 3; j >= 0; --j)
        {
            if(j == 3)
            {
                pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(kPageDir);
            }
            else
            {
                nextPtable = _makeCanonical(pPageTable[j + 1][pmlEntry[j + 1]] &
                                            ~PAGE_SIZE_MASK,
                                            true);
                pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(nextPtable);
            }

            if(j != 0 &&
               (pPageTable[j][pmlEntry[j]] & PAGE_FLAG_PRESENT) == 0)
            {
                /* If the check is a full check and we have an unmapped
                 * region, return false
                 */
                if(kCheckFull == true)
                {
                    return false;
                }

                /* Check next level entry and either zeroise or set the
                 * page
                 */
                if(j == 3)
                {
                    stride = ((KERNEL_PGDIR_ENTRY_COUNT -
                                (pmlEntry[1] + 1)) *
                                KERNEL_PGDIR_ENTRY_COUNT *
                                KERNEL_PGDIR_ENTRY_COUNT) +
                                ((KERNEL_PGDIR_ENTRY_COUNT -
                                (pmlEntry[2] + 1)) *
                                KERNEL_PGDIR_ENTRY_COUNT) +
                                KERNEL_PGDIR_ENTRY_COUNT - pmlEntry[1];

                    currVirtAddr += KERNEL_PAGE_SIZE * stride;
                    pageCount -= MIN(pageCount, stride);

                    /* We are done with the rest of the hierarchy */
                    j = -1;
                }
                else if(j == 2)
                {
                    stride = ((KERNEL_PGDIR_ENTRY_COUNT -
                                (pmlEntry[2] + 1)) *
                                KERNEL_PGDIR_ENTRY_COUNT) +
                                KERNEL_PGDIR_ENTRY_COUNT - pmlEntry[1];

                    currVirtAddr += KERNEL_PAGE_SIZE * stride;
                    pageCount -= MIN(pageCount, stride);

                    /* We are dones with the rest of the hierarchy */
                    j = -1;
                }
                else if(j == 1)
                {
                    stride = KERNEL_PGDIR_ENTRY_COUNT - pmlEntry[1];
                    currVirtAddr += KERNEL_PAGE_SIZE * stride;
                    pageCount -= MIN(pageCount, stride);

                    /* We are dones with the rest of the hierarchy */
                    j = -1;
                }
            }
            else if(j == 0)
            {
                do
                {
                    if((pPageTable[j][pmlEntry[j]] & PAGE_FLAG_PRESENT) == 0)
                    {
                        /* If the check is a full check and we have an unmapped
                         * region, return false
                         */
                        if(kCheckFull == true)
                        {
                            return false;
                        }
                    }
                    else
                    {
                        /* If the check is not a full check and we have a mapp
                         * partial region, return true
                         */
                        if(kCheckFull == false)
                        {
                            return true;
                        }
                    }

                    currVirtAddr += KERNEL_PAGE_SIZE;
                    --pageCount;
                    ++pmlEntry[0];
                } while(pageCount > 0 &&
                        pmlEntry[0] < KERNEL_PGDIR_ENTRY_COUNT);
            }
        }
    } while(pageCount > 0);

    if(kCheckFull == false)
    {
        /* If the check is not a full check and we still have not returned, we
         * never reached a mapped region.
         */
        return false;
    }
    else
    {
        /* If the check is a full check and we still have not returned, we never
         * reached a non-mapped region.
         */
        return true;
    }
}

static OS_RETURN_E _memoryMgrMap(const uintptr_t kVirtualAddress,
                                 const uintptr_t kPhysicalAddress,
                                 const size_t    kPageCount,
                                 const uint32_t  kFlags,
                                 const uintptr_t kPageDir)
{
    size_t       toMap;
    int8_t       j;
    uint64_t     mapFlags;
    uint64_t     mapPgdirFlags;
    bool         isMapped;
    uintptr_t    currVirtAddr;
    uintptr_t    currPhysAdd;
    uintptr_t    newPgTableFrame;
    uintptr_t*   pPageTable[4];
    uint16_t     pmlEntry[4];
    uintptr_t    nextPtable;
    ipi_params_t ipiParams;

    /* Check the alignements */
    if((kVirtualAddress & PAGE_SIZE_MASK) != 0 ||
       (kPhysicalAddress & PAGE_SIZE_MASK) != 0 ||
       kPageCount == 0)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Check the canonical address */
    if((kVirtualAddress & ~sCanonicalBound) != 0)
    {
        if((kVirtualAddress & ~sCanonicalBound) != ~sCanonicalBound)
        {
            return OS_ERR_INCORRECT_VALUE;
        }
    }

    if((kPhysicalAddress & ~sPhysAddressWidthMask) != 0)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

#if MEMORY_MGR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Mapping 0x%p to 0x%p -> 0x%p",
           kPhysicalAddress,
           kPhysicalAddress + kPageCount * KERNEL_PAGE_SIZE,
           kVirtualAddress);
#endif

    /* Check if the mapping already exists, check if we need to update one or
     * more page directory entries
     */
    isMapped = _memoryMgrIsMapped(kVirtualAddress, kPageCount, kPageDir, false);
    if(isMapped == true)
    {
        return OS_ERR_ALREADY_EXIST;
    }

    /* Get the flags */
    mapFlags = PAGE_FLAG_PRESENT | _translateFlags(kFlags);

    mapPgdirFlags = PAGE_FLAG_PAGE_SIZE_4KB |
                    PAGE_FLAG_SUPER_ACCESS  |
                    PAGE_FLAG_USER_ACCESS   |
                    PAGE_FLAG_READ_WRITE    |
                    PAGE_FLAG_CACHE_WB      |
                    PAGE_FLAG_XD            |
                    PAGE_FLAG_PRESENT;

    /* Apply the mapping */
    toMap = kPageCount;
    currVirtAddr = kVirtualAddress;
    currPhysAdd  = kPhysicalAddress;

    ipiParams.function = IPI_FUNC_TLB_INVAL;

    while(toMap != 0)
    {
        pmlEntry[3] = (currVirtAddr >> PML4_ENTRY_OFFSET) &
                        PG_ENTRY_OFFSET_MASK;
        pmlEntry[2] = (currVirtAddr >> PML3_ENTRY_OFFSET) &
                        PG_ENTRY_OFFSET_MASK;
        pmlEntry[1] = (currVirtAddr >> PML2_ENTRY_OFFSET) &
                        PG_ENTRY_OFFSET_MASK;
        pmlEntry[0] = (currVirtAddr >> PML1_ENTRY_OFFSET) &
                        PG_ENTRY_OFFSET_MASK;

        /* Setup entry in the four levels is needed  */
        for(j = 3; j >= 0; --j)
        {
            if(j == 3)
            {
                pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(kPageDir);
            }
            else
            {
                nextPtable = _makeCanonical(pPageTable[j + 1][pmlEntry[j + 1]] &
                                            ~PAGE_SIZE_MASK,
                                            true);
                pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(nextPtable);
            }

            if(j != 0 && (pPageTable[j][pmlEntry[j]] & PAGE_FLAG_PRESENT) == 0)
            {
                /* Allocate a new frame and map to temporary boot entry */
                newPgTableFrame = _allocateFrames(1);
                MEM_ASSERT(newPgTableFrame != 0,
                        "Allocated a NULL frame",
                        OS_ERR_NULL_POINTER);

                pPageTable[j][pmlEntry[j]] = (newPgTableFrame &
                                              sPhysAddressWidthMask) |
                                             mapPgdirFlags;

                /* Zeroise the page */
                memset((void*)GET_VIRT_MEM_ADDR(newPgTableFrame),
                       0,
                       KERNEL_PAGE_SIZE);
            }
            else if(j == 0)
            {
                /* Map as much as we can in this page table */
                do
                {
                    /* Set mapping and invalidate */
                    pPageTable[j][pmlEntry[0]] = (currPhysAdd &
                                                  sPhysAddressWidthMask) |
                                                 mapFlags;
                    cpuInvalidateTlbEntry(currVirtAddr);

                    /* Update other cores TLB */
                    ipiParams.pData = (void*)currVirtAddr;
                    cpuMgtSendIpi(CPU_IPI_BROADCAST_TO_OTHER,
                                  &ipiParams,
                                  true);

                    currVirtAddr += KERNEL_PAGE_SIZE;
                    currPhysAdd  += KERNEL_PAGE_SIZE;
                    --toMap;
                    ++pmlEntry[0];
                } while(toMap > 0 && pmlEntry[0] != KERNEL_PGDIR_ENTRY_COUNT);
            }
        }
    }

    return OS_NO_ERR;
}

static OS_RETURN_E _memoryMgrUnmap(const uintptr_t kVirtualAddress,
                                   const size_t    kPageCount,
                                   const uintptr_t kPageDir)
{
    size_t       toUnmap;
    bool         hasMapping;
    uint16_t     i;
    int8_t       j;
    uintptr_t    currVirtAddr;
    uintptr_t    nextPtable;
    uintptr_t*   pPageTable[4];
    uint16_t     pmlEntry[4];
    uintptr_t    physAddr;
    ipi_params_t ipiParams;

    /* Check the alignements */
    if((kVirtualAddress & PAGE_SIZE_MASK) != 0 ||
       kPageCount == 0)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Check the canonical address */
    if((kVirtualAddress & ~sCanonicalBound) != 0)
    {
        if((kVirtualAddress & ~sCanonicalBound) !=
           ~sCanonicalBound)
        {
            return OS_ERR_INCORRECT_VALUE;
        }
    }

    /* Check if the mapping already exists, check if we need to update one or
     * more page directory entries
     */
    hasMapping = _memoryMgrIsMapped(kVirtualAddress,
                                    kPageCount,
                                    kPageDir,
                                    true);
    if(hasMapping == false)
    {
        return OS_ERR_NO_SUCH_ID;
    }

    /* Apply the unmapping */
    toUnmap = kPageCount;
    currVirtAddr = kVirtualAddress;

    ipiParams.function = IPI_FUNC_TLB_INVAL;

    while(toUnmap != 0)
    {
        /* Skip unmapped regions */
        hasMapping = false;

        pmlEntry[3] = (currVirtAddr >> PML4_ENTRY_OFFSET) &
                        PG_ENTRY_OFFSET_MASK;
        pmlEntry[2] = (currVirtAddr >> PML3_ENTRY_OFFSET) &
                        PG_ENTRY_OFFSET_MASK;
        pmlEntry[1] = (currVirtAddr >> PML2_ENTRY_OFFSET) &
                        PG_ENTRY_OFFSET_MASK;
        pmlEntry[0] = (currVirtAddr >> PML1_ENTRY_OFFSET) &
                        PG_ENTRY_OFFSET_MASK;

        /* Get the memory mapping */
        for(j = 3; j >= 0; --j)
        {
            if(j == 3)
            {
                pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(kPageDir);
            }
            else
            {
                nextPtable =
                    _makeCanonical(pPageTable[j + 1][pmlEntry[j + 1]] &
                                   ~PAGE_SIZE_MASK,
                                   true);
                pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(nextPtable);
            }
        }

        /* Remove entry in the four levels is needed  */
        for(j = 0; j < 3 && toUnmap > 0; ++j)
        {
            /* If first level, unmap */
            if(j == 0)
            {
                do
                {
                    pPageTable[j][pmlEntry[0]] = 0;

                    cpuInvalidateTlbEntry(currVirtAddr);

                    /* Update other cores TLB */
                    ipiParams.pData = (void*)currVirtAddr;
                    cpuMgtSendIpi(CPU_IPI_BROADCAST_TO_OTHER,
                                  &ipiParams,
                                  true);
                    currVirtAddr += KERNEL_PAGE_SIZE;
                    --toUnmap;
                    ++pmlEntry[0];
                } while(toUnmap > 0 && pmlEntry[0] != KERNEL_PGDIR_ENTRY_COUNT);

                /* Check if we can clean this directory entries */
                hasMapping = false;
                for(i = 0; i < KERNEL_PGDIR_ENTRY_COUNT; ++i)
                {
                    if((pPageTable[j][i] & PAGE_FLAG_PRESENT) != 0)
                    {
                        hasMapping = true;
                        break;
                    }
                }

                if(hasMapping == false)
                {
                    /* Release the frames */
                    physAddr =
                        _makeCanonical(pPageTable[j + 1][pmlEntry[j + 1]] &
                                       ~PAGE_SIZE_MASK,
                                       true);
                    _releaseFrames(physAddr, 1);

                    /* Set the entry as unmapped in the previous level */
                    pPageTable[j + 1][pmlEntry[j + 1]] = 0;
                }
            }
            else
            {
                /* Check if we can clean this directory entries */
                for(i = 0; i < KERNEL_PGDIR_ENTRY_COUNT; ++i)
                {
                    if((pPageTable[j + 1][i] & PAGE_FLAG_PRESENT) != 0)
                    {
                        hasMapping = true;
                        break;
                    }
                }

                if(hasMapping == false)
                {
                    /* Release the frames */
                    physAddr =
                        _makeCanonical(pPageTable[j + 1][pmlEntry[j + 1]] &
                                       ~PAGE_SIZE_MASK,
                                       true);
                    _releaseFrames(physAddr, 1);

                    /* Set the entry as unmapped in the previous level */
                    pPageTable[j + 1][pmlEntry[j + 1]] = 0;
                }
            }
        }
    }

    return OS_NO_ERR;
}

static void _releasePageDir(const uintptr_t kPhysTable,
                            const uintptr_t kBaseVirtAddr,
                            const uint8_t   kLevel)
{
    uintptr_t   frameAddr;
    uintptr_t*  currentLevelPage;
    uintptr_t   virtAddr;
    uintptr_t   levelAddrCount;
    uint32_t    i;
    uint16_t*   refCount;

    MEM_ASSERT(kLevel > 0,
               "Invalid page directory level in release",
               OS_ERR_INCORRECT_VALUE);

    /* Allocate frames for mapping */
    currentLevelPage = (uintptr_t*)GET_VIRT_MEM_ADDR(kPhysTable);

    /* Get the address increase based on the level */
    switch(kLevel)
    {
        /* PML4 */
        case 4:
            levelAddrCount = 1ULL << PML4_ENTRY_OFFSET;
            break;
        /* PML3 */
        case 3:
            levelAddrCount = 1ULL << PML3_ENTRY_OFFSET;
            break;
        /* PML2 */
        case 2:
            levelAddrCount = 1ULL << PML2_ENTRY_OFFSET;
            break;
        /* PML 1 */
        case 1:
            levelAddrCount = 1ULL << PML1_ENTRY_OFFSET;
            break;
        default:
            MEM_ASSERT(false,
                       "Invalid page directory level in release",
                       OS_ERR_INCORRECT_VALUE);
            levelAddrCount = 0;
    }

    /* Check all entries of the current table */
    if(kLevel == 1)
    {
        for(i = 0; i < KERNEL_PGDIR_ENTRY_COUNT; ++i)
        {
            virtAddr = kBaseVirtAddr + i * levelAddrCount;

            /* Check if we still are in the low kernel
             * space
             */
            if(virtAddr < USER_MEMORY_START)
            {
                /* We do not release low-memory kernel
                 * frames
                 */
                continue;
            }
            /* Check if we are in the high kernel space*/
            else if(virtAddr >= USER_MEMORY_END)
            {
                /* The next address will not need any release */
                break;
            }
            else
            {
                /* If present and not hardware, release the frame */
                if((currentLevelPage[i] &
                    (PAGE_FLAG_PRESENT | PAGE_FLAG_IS_HW)) == PAGE_FLAG_PRESENT)
                {
                    frameAddr = _makeCanonical(currentLevelPage[i] &
                                               ~PAGE_SIZE_MASK,
                                               true);

                    /* Decrease the reference count */
                    refCount = _getAndLockReferenceCount(frameAddr);
                    MEM_ASSERT(*refCount > 0,
                               "Invalid reference count zero",
                               OS_ERR_INCORRECT_VALUE);
                    *refCount = *refCount - 1;

                    /* If 0, release the frame */
                    if(*refCount == 0)
                    {
                        _releaseFrames(frameAddr, 1);
                    }
                    _unlockReferenceCount(frameAddr);
                }
            }
        }
    }
    else
    {
        for(i = 0; i < KERNEL_PGDIR_ENTRY_COUNT; ++i)
        {
            virtAddr = kBaseVirtAddr + i * levelAddrCount;

            /* Check if we are in the high kernel space*/
            if(virtAddr >= USER_MEMORY_END)
            {
                /* The next address will not need any release */
                break;
            }

            /* If present, got to next level */
            if((currentLevelPage[i] & PAGE_FLAG_PRESENT) == PAGE_FLAG_PRESENT)
            {
                virtAddr = kBaseVirtAddr + (uintptr_t)i * levelAddrCount;
                frameAddr = _makeCanonical(currentLevelPage[i] &
                                            ~PAGE_SIZE_MASK,
                                            true);

                /* Release the next level if not in kernel zone */
                _releasePageDir(frameAddr, virtAddr, kLevel - 1);
            }
        }
    }

    /* Release the page table */
    _releaseFrames(kPhysTable, 1);
}

static uintptr_t _memoryMgrGetPhysAddr(const uintptr_t kVirtualAddress,
                                       const uintptr_t kPageDir,
                                       uint32_t*       pFlags)
{
    uintptr_t  retPhysAddr;
    uintptr_t  nextPtable;
    uintptr_t* pPageTable[4];
    uint16_t   pmlEntry[4];
    int8_t     j;

    retPhysAddr = MEMMGR_PHYS_ADDR_ERROR;

    pmlEntry[3] = (kVirtualAddress >> PML4_ENTRY_OFFSET) &
                    PG_ENTRY_OFFSET_MASK;
    pmlEntry[2] = (kVirtualAddress >> PML3_ENTRY_OFFSET) &
                    PG_ENTRY_OFFSET_MASK;
    pmlEntry[1] = (kVirtualAddress >> PML2_ENTRY_OFFSET) &
                    PG_ENTRY_OFFSET_MASK;
    pmlEntry[0] = (kVirtualAddress >> PML1_ENTRY_OFFSET) &
                    PG_ENTRY_OFFSET_MASK;

    for(j = 3; j >= 0; --j)
    {
        if(j == 3)
        {
            pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(kPageDir);
        }
        else
        {
            nextPtable =
                _makeCanonical(pPageTable[j + 1][pmlEntry[j + 1]] &
                                ~PAGE_SIZE_MASK,
                                true);
            pPageTable[j] = (uintptr_t*)GET_VIRT_MEM_ADDR(nextPtable);
        }

        if((pPageTable[j][pmlEntry[j]] & PAGE_FLAG_PRESENT) != 0)
        {
            if(j == 0)
            {
                if(pFlags != NULL)
                {
                    retPhysAddr = pPageTable[j][pmlEntry[j]];
                    *pFlags     = MEMMGR_MAP_KERNEL;

                    if((retPhysAddr & PAGE_FLAG_READ_WRITE) ==
                       PAGE_FLAG_READ_WRITE)
                    {
                        *pFlags |= MEMMGR_MAP_RW;
                    }
                    else
                    {
                        *pFlags |= MEMMGR_MAP_RO;
                    }
                    if((retPhysAddr & PAGE_FLAG_XD) != PAGE_FLAG_XD)
                    {
                        *pFlags |= MEMMGR_MAP_EXEC;
                    }
                    if((retPhysAddr & PAGE_FLAG_USER_ACCESS) ==
                       PAGE_FLAG_USER_ACCESS)
                    {
                        *pFlags |= MEMMGR_MAP_USER;
                    }
                    if((retPhysAddr & PAGE_FLAG_CACHE_DISABLED) ==
                       PAGE_FLAG_CACHE_DISABLED)
                    {
                        *pFlags |= MEMMGR_MAP_CACHE_DISABLED;
                    }
                    if((retPhysAddr & PAGE_FLAG_IS_HW) == PAGE_FLAG_IS_HW)
                    {
                        *pFlags |= MEMMGR_MAP_HARDWARE;
                    }
                    if((retPhysAddr & PAGE_FLAG_COW) == PAGE_FLAG_COW)
                    {
                        *pFlags |= MEMMGR_MAP_COW;
                    }

                    retPhysAddr = (retPhysAddr & sPhysAddressWidthMask) &
                                  ~PAGE_SIZE_MASK;
                }
                else
                {
                    retPhysAddr = (pPageTable[j][pmlEntry[j]] &
                                   sPhysAddressWidthMask) & ~PAGE_SIZE_MASK;
                }
            }
        }
        else
        {
            break;
        }
    }

    if(retPhysAddr != MEMMGR_PHYS_ADDR_ERROR)
    {
        retPhysAddr |= kVirtualAddress & PAGE_SIZE_MASK;
    }

    return retPhysAddr;
}

static void _memoryMgrDetectMemory(void)
{
    uintptr_t             baseAddress;
    size_t                size;
    uintptr_t             frameEntry;
    uintptr_t             kernelPhysStart;
    uintptr_t             kernelPhysEnd;
    const fdt_mem_node_t* kpPhysMemNode;
    const fdt_mem_node_t* kpResMemNode;
    const fdt_mem_node_t* kpCursor;

    kpPhysMemNode = fdtGetMemory();
    MEM_ASSERT(kpPhysMemNode != NULL,
               "No physical memory detected in FDT",
               OS_ERR_NO_MORE_MEMORY);

    /* Now iterate on all memory nodes and add the regions */
    while(kpPhysMemNode != NULL)
    {
        /* Align the base address and size */
        baseAddress = ALIGN_UP(FDTTOCPU64(kpPhysMemNode->baseAddress),
                               KERNEL_PAGE_SIZE);
        size        = baseAddress - FDTTOCPU64(kpPhysMemNode->baseAddress);
        size        = ALIGN_DOWN(FDTTOCPU64(kpPhysMemNode->size) - size,
                                 KERNEL_PAGE_SIZE);

#if MEMORY_MGR_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "Adding region 0x%p -> 0x%p | Aligned: 0x%p -> 0x%p",
               FDTTOCPU64(kpPhysMemNode->baseAddress),
               FDTTOCPU64(kpPhysMemNode->baseAddress) +
               FDTTOCPU64(kpPhysMemNode->size),
               baseAddress,
               baseAddress + size);
#endif

        MEM_ASSERT(baseAddress + size < KERNEL_MAX_MEM_PHYS,
                   "Kernel does not support physical memory over 512BG",
                   OS_ERR_NOT_SUPPORTED);

        /* Add to the page-to-frame directory */
        frameEntry = baseAddress / KERNEL_MEM_1G;
        if((_physicalMapDir[frameEntry] & PAGE_FLAG_PRESENT) == 0)
        {
            _physicalMapDir[frameEntry] = (frameEntry * KERNEL_MEM_1G) |
                                          PAGE_FLAG_PAGE_SIZE_1GB      |
                                          PAGE_FLAG_SUPER_ACCESS       |
                                          PAGE_FLAG_CACHE_WB           |
                                          PAGE_FLAG_READ_WRITE         |
                                          PAGE_FLAG_GLOBAL             |
                                          PAGE_FLAG_XD                 |
                                          PAGE_FLAG_PRESENT;
        }

        /* Add block to the free frames */
        _addBlock(&sPhysMemList, baseAddress, size);

        /* Go to next node */
        kpPhysMemNode = kpPhysMemNode->pNextNode;
    }

    /* Remove reserved memory */
    kpResMemNode = fdtGetReservedMemory();
    kpCursor = kpResMemNode;
    while(kpCursor != NULL)
    {
        baseAddress = ALIGN_DOWN(FDTTOCPU64(kpCursor->baseAddress),
                                 KERNEL_PAGE_SIZE);
        size = ALIGN_UP(FDTTOCPU64(kpCursor->size), KERNEL_PAGE_SIZE);

#if MEMORY_MGR_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "Removing reserved region 0x%p -> 0x%p",
               baseAddress,
               baseAddress + size);
#endif

        _removeBlock(&sPhysMemList, baseAddress, size);

        kpCursor = kpCursor->pNextNode;
    }

    /* Get kernel bounds */
    kernelPhysStart = (uintptr_t)&_KERNEL_MEMORY_START;
#ifdef _TESTING_FRAMEWORK_ENABLED
    /* If testing is enabled, the end is after its buffer */
    kernelPhysEnd = (uintptr_t)&_KERNEL_TEST_BUFFER_BASE +
                    (uintptr_t)&_KERNEL_TEST_BUFFER_SIZE;
#else
    kernelPhysEnd   = (uintptr_t)&_KERNEL_MEMORY_END;
#endif

    /* Get actual physical address */
    kernelPhysStart = ALIGN_DOWN(kernelPhysStart - KERNEL_MEM_OFFSET,
                                  KERNEL_PAGE_SIZE);
    kernelPhysEnd   = ALIGN_UP(kernelPhysEnd - KERNEL_MEM_OFFSET,
                                KERNEL_PAGE_SIZE);

    /* Remove the kernel physical memory */
    _removeBlock(&sPhysMemList,
                 kernelPhysStart,
                 kernelPhysEnd - kernelPhysStart);
}

static void _memoryMgrCreateFramesMeta(void)
{
    size_t                size;
    uintptr_t             refCountPages;
    uintptr_t             refCountFrames;
    uintptr_t             base;
    uintptr_t             limit;
    uintptr_t             blockSize;
    OS_RETURN_E           error;
    kqueue_node_t*        pNode;
    mem_range_t*          pRange;
    frame_meta_table_t*   pMetaTable;
    frame_meta_table_t*   pCursor;
    frame_meta_table_t*   pLastCursor;

    /* Create the frame meta table */
    pNode = sPhysMemList.pQueue->pHead;
    while(pNode != NULL)
    {
        pRange = pNode->pData;

        /* Allocate a new node in the frame meta table */
        pMetaTable = kmalloc(sizeof(frame_meta_table_t));
        MEM_ASSERT(pMetaTable != NULL,
                   "Failed to allocate frame meta table",
                   OS_ERR_NO_MORE_MEMORY);
        KERNEL_SPINLOCK_INIT(pMetaTable->lock);


        /* Allocate the reference count table from this block by iteration */
        base = pRange->base;
        limit = pRange->limit;
        blockSize = limit - base;
        while(true)
        {
            /* Get the size in bytes of the reference count table */
            size = (limit - base) / KERNEL_PAGE_SIZE * sizeof(uint16_t);
            size = ALIGN_UP(size, KERNEL_PAGE_SIZE);

            if(size + (limit - base) <= blockSize || base >= limit)
            {
                break;
            }
            base += KERNEL_PAGE_SIZE;
        }

        MEM_ASSERT(base < limit,
                   "Failed to allocate frame meta reference count table, the "
                   "block is too small.",
                   OS_ERR_NO_MORE_MEMORY);

        /* Get the frames */
        refCountFrames = pRange->base;

        /* Update the range */
        pRange->base = base;
        pNode->priority = KERNEL_VIRTUAL_ADDR_MAX - base;

        /* Setup the meta table info */
        pMetaTable->firstFrame = base;
        pMetaTable->lastFrame  = limit;

        size /= KERNEL_PAGE_SIZE;
        refCountPages = _allocateKernelPages(size);
        MEM_ASSERT(refCountPages != (uintptr_t)NULL,
                   "Failed to allocate frame meta reference count table",
                   OS_ERR_NO_MORE_MEMORY);

        /* Map and initialize the table */
        error = _memoryMgrMap(refCountPages,
                              refCountFrames,
                              size,
                              MEMMGR_MAP_RW | MEMMGR_MAP_KERNEL,
                              (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);
        MEM_ASSERT(error == OS_NO_ERR,
                   "Failed to map frame meta reference count table",
                   OS_ERR_NO_MORE_MEMORY);

        pMetaTable->pRefCountTable = (uint16_t*)refCountPages;
        memset(pMetaTable->pRefCountTable, 0, size * KERNEL_PAGE_SIZE);

        /* Link the table */
        pLastCursor = NULL;
        pCursor = spFramesMeta;
        while(pCursor != NULL)
        {
            if(pCursor->firstFrame > pMetaTable->firstFrame)
            {
                break;
            }
            pLastCursor = pCursor;
            pCursor = pCursor->pNext;
        }
        if(pLastCursor == NULL)
        {
            pMetaTable->pNext = spFramesMeta;
            spFramesMeta = pMetaTable;
        }
        else
        {
            pLastCursor->pNext = pMetaTable;
            pMetaTable->pNext = pCursor;
        }
        pNode = pNode->pNext;
    }
}

static void _memoryMgrInitKernelFreePages(void)
{
    uintptr_t kernelVirtEnd;

#ifdef _TESTING_FRAMEWORK_ENABLED
    /* If testing is enabled, the end is after its buffer */
    kernelVirtEnd = (uintptr_t)&_KERNEL_TEST_BUFFER_BASE +
                    (uintptr_t)&_KERNEL_TEST_BUFFER_SIZE;
#else
    /* Initialize kernel pages */
    kernelVirtEnd   = (uintptr_t)&_KERNEL_MEMORY_END;
#endif

    /* Get actual physical address */
    kernelVirtEnd = ALIGN_UP(kernelVirtEnd, KERNEL_PAGE_SIZE);

    sKernelVirtualMemBounds.base  = kernelVirtEnd;
    sKernelVirtualMemBounds.limit = KERNEL_VIRTUAL_ADDR_MAX;

    /* Add free pages */
    _addBlock(&sKernelFreePagesList,
              kernelVirtEnd,
              KERNEL_VIRTUAL_ADDR_MAX - kernelVirtEnd + 1);
}

static void _memoryMgrMapKernelRegion(uintptr_t*      pLastSectionStart,
                                      uintptr_t*      pLastSectionEnd,
                                      const uintptr_t kRegionStartAddr,
                                      const uintptr_t kRegionEndAddr,
                                      const uint32_t  kFlags)
{
    int8_t     i;
    uintptr_t  tmpPageTablePhysAddr;
    uintptr_t  kernelSectionStart;
    uintptr_t  kernelSectionEnd;
    uint16_t   pmlEntry[4];
    uintptr_t* pPageTable[4];
    uintptr_t  nextPtable;

    /* Align and check */
    kernelSectionStart = ALIGN_DOWN(kRegionStartAddr, KERNEL_PAGE_SIZE);
    kernelSectionEnd   = ALIGN_UP(kRegionEndAddr, KERNEL_PAGE_SIZE);

    MEM_ASSERT(*pLastSectionEnd <= kernelSectionStart,
               "Overlapping kernel memory sections",
               OS_ERR_NO_MORE_MEMORY);

    *pLastSectionStart = kernelSectionStart;
    *pLastSectionEnd   = kernelSectionEnd;

    /* Map per 4K pages in the temporary boot entry */
    while(kernelSectionStart < kernelSectionEnd)
    {
        /* Get entry indexes */
        pmlEntry[0] = (kernelSectionStart >> PML1_ENTRY_OFFSET) &
                       PG_ENTRY_OFFSET_MASK;
        pmlEntry[1] = (kernelSectionStart >> PML2_ENTRY_OFFSET) &
                       PG_ENTRY_OFFSET_MASK;
        pmlEntry[2] = (kernelSectionStart >> PML3_ENTRY_OFFSET) &
                       PG_ENTRY_OFFSET_MASK;
        if(kernelSectionStart < KERNEL_MEM_OFFSET)
        {
            pmlEntry[3] = (kernelSectionStart >> PML4_ENTRY_OFFSET) &
                           PG_ENTRY_OFFSET_MASK;
        }
        else
        {
            pmlEntry[3] = KERNEL_PML4_BOOT_TMP_ENTRY;
        }

        /* Setup entry in the four levels is needed  */
        for(i = 3; i >= 0; --i)
        {
            if(i == 3)
            {
                pPageTable[i] = spKernelPageDir;
            }
            else
            {
                nextPtable = _makeCanonical(pPageTable[i + 1][pmlEntry[i + 1]] &
                                            ~PAGE_SIZE_MASK,
                                            true);
                pPageTable[i] = (uintptr_t*)GET_VIRT_MEM_ADDR(nextPtable);
            }

            if((pPageTable[i][pmlEntry[i]] & PAGE_FLAG_PRESENT) == 0)
            {
                /* Allocate a new frame and map to temporary boot entry */
                tmpPageTablePhysAddr = _allocateFrames(1);
                MEM_ASSERT(tmpPageTablePhysAddr != 0,
                           "Allocated a NULL frame",
                           OS_ERR_NULL_POINTER);

                pPageTable[i][pmlEntry[i]] = tmpPageTablePhysAddr    |
                                             PAGE_FLAG_PAGE_SIZE_4KB |
                                             PAGE_FLAG_SUPER_ACCESS  |
                                             PAGE_FLAG_USER_ACCESS   |
                                             PAGE_FLAG_READ_WRITE    |
                                             PAGE_FLAG_CACHE_WB      |
                                             PAGE_FLAG_GLOBAL        |
                                             PAGE_FLAG_PRESENT;

                /* Zeroize the table */
                if(i != 0)
                {
                    memset((void*)GET_VIRT_MEM_ADDR(tmpPageTablePhysAddr),
                           0,
                           KERNEL_PAGE_SIZE);
                }
                else
                {
                    /* Last level, set the entry */
                    if(kernelSectionStart >= KERNEL_MEM_OFFSET)
                    {
                        pPageTable[i][pmlEntry[i]] = (kernelSectionStart -
                                                      KERNEL_MEM_OFFSET)       |
                                                      PAGE_FLAG_PAGE_SIZE_4KB  |
                                                      PAGE_FLAG_SUPER_ACCESS   |
                                                      PAGE_FLAG_CACHE_WB       |
                                                      PAGE_FLAG_GLOBAL         |
                                                      PAGE_FLAG_PRESENT;
                    }
                    else
                    {
                        pPageTable[i][pmlEntry[i]] =  kernelSectionStart      |
                                                      PAGE_FLAG_PAGE_SIZE_4KB |
                                                      PAGE_FLAG_SUPER_ACCESS  |
                                                      PAGE_FLAG_CACHE_WB      |
                                                      PAGE_FLAG_GLOBAL        |
                                                      PAGE_FLAG_PRESENT;
                    }

                    /* Set the flags */
                    if((kFlags & MEMMGR_MAP_RW) == MEMMGR_MAP_RW)
                    {
                        pPageTable[i][pmlEntry[i]] |= PAGE_FLAG_READ_WRITE;
                    }
                    if((kFlags & MEMMGR_MAP_EXEC) != MEMMGR_MAP_EXEC)
                    {
                        pPageTable[i][pmlEntry[i]] |= PAGE_FLAG_XD;
                    }

                }
            }
        }

        kernelSectionStart += KERNEL_PAGE_SIZE;
    }
}

static void _memoryMgrMapKernel(void)
{
    uintptr_t  kernelSectionStart;
    uintptr_t  kernelSectionEnd;

    kernelSectionStart = 0;
    kernelSectionEnd   = 0;

    /* Map kernel code */
    _memoryMgrMapKernelRegion(&kernelSectionStart,
                              &kernelSectionEnd,
                              (uintptr_t)&_START_LOW_AP_STARTUP_ADDR,
                              (uintptr_t)&_END_LOW_AP_STARTUP_ADDR,
                              MEMMGR_MAP_RO | MEMMGR_MAP_EXEC);
    _memoryMgrMapKernelRegion(&kernelSectionStart,
                              &kernelSectionEnd,
                              (uintptr_t)&_START_BIOS_CALL_ADDR,
                              (uintptr_t)&_END_BIOS_CALL_ADDR,
                              MEMMGR_MAP_RW | MEMMGR_MAP_EXEC);
    _memoryMgrMapKernelRegion(&kernelSectionStart,
                              &kernelSectionEnd,
                              (uintptr_t)&_START_TEXT_ADDR,
                              (uintptr_t)&_END_TEXT_ADDR,
                              MEMMGR_MAP_RO | MEMMGR_MAP_EXEC);

    /* Map kernel RO data */
    _memoryMgrMapKernelRegion(&kernelSectionStart,
                              &kernelSectionEnd,
                              (uintptr_t)&_START_RO_DATA_ADDR,
                              (uintptr_t)&_END_RO_DATA_ADDR,
                              MEMMGR_MAP_RO);

    /* Map kernel RW data, stack and heap */
    _memoryMgrMapKernelRegion(&kernelSectionStart,
                              &kernelSectionEnd,
                              (uintptr_t)&_START_RW_DATA_ADDR,
                              (uintptr_t)&_END_RW_DATA_ADDR,
                              MEMMGR_MAP_RW);
    _memoryMgrMapKernelRegion(&kernelSectionStart,
                              &kernelSectionEnd,
                              (uintptr_t)&_KERNEL_STACKS_BASE,
                              (uintptr_t)&_KERNEL_STACKS_BASE +
                              (uintptr_t)&_KERNEL_STACKS_SIZE,
                              MEMMGR_MAP_RW);
    _memoryMgrMapKernelRegion(&kernelSectionStart,
                              &kernelSectionEnd,
                              (uintptr_t)&_KERNEL_HEAP_BASE,
                              (uintptr_t)&_KERNEL_HEAP_BASE +
                              (uintptr_t)&_KERNEL_HEAP_SIZE,
                              MEMMGR_MAP_RW);

#ifdef _TESTING_FRAMEWORK_ENABLED
    _memoryMgrMapKernelRegion(&kernelSectionStart,
                              &kernelSectionEnd,
                              (uintptr_t)&_KERNEL_TEST_BUFFER_BASE,
                              (uintptr_t)&_KERNEL_TEST_BUFFER_BASE +
                              (uintptr_t)&_KERNEL_TEST_BUFFER_SIZE,
                              MEMMGR_MAP_RW);
#endif

    /* Copy temporary entry to kernel entry and clear temp  */
    spKernelPageDir[KERNEL_PML4_KERNEL_ENTRY] =
        spKernelPageDir[KERNEL_PML4_BOOT_TMP_ENTRY];
    spKernelPageDir[KERNEL_PML4_BOOT_TMP_ENTRY] = 0;

    /* Update the whole page table */
    cpuSetPageDirectory((uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);
}

static uintptr_t _allocateUserPages(const size_t            kPageCount,
                                    const kernel_process_t* kpProcess,
                                    const bool              kFromTop)
{
    memproc_info_t* pMemProcInfo;

    pMemProcInfo = kpProcess->pMemoryData;

    if(kFromTop == true)
    {
        return _getBlockFromEnd(&pMemProcInfo->freePageTable,
                                kPageCount * KERNEL_PAGE_SIZE);
    }
    else
    {
        return _getBlock(&pMemProcInfo->freePageTable,
                         kPageCount * KERNEL_PAGE_SIZE);
    }

}

static void _releaseUserPages(const uintptr_t         kBaseAddress,
                              const size_t            kPageCount,
                              const kernel_process_t* kpProcess)
{
    memproc_info_t* pMemProcInfo;

    pMemProcInfo = kpProcess->pMemoryData;

    _addBlock(&pMemProcInfo->freePageTable,
              kBaseAddress,
              kPageCount * KERNEL_PAGE_SIZE);
}

static OS_RETURN_E _copyPgDirEntry(uintptr_t*      pSrcLevel,
                                   uintptr_t*      pDstLevel,
                                   uintptr_t*      pVirtAddress,
                                   const uintptr_t kVirtAddressMax,
                                   const uint8_t   kLevel,
                                   const bool      kSetCOW)
{
    uint32_t    addrEntryIdx;
    uintptr_t   virtAddrAdd;
    uintptr_t   frameAddr;
    uintptr_t   srcNextDirLevelFrame;
    uintptr_t   dstNextDirLevelFrame;
    uintptr_t   srcNextDirLevelPage;
    uintptr_t   dstNextDirLevelPage;
    OS_RETURN_E error;
    uint16_t*   refCount;

    /* Get the entry index */
    switch(kLevel)
    {
        /* PML4 */
        case 4:
            addrEntryIdx = (*pVirtAddress >> PML4_ENTRY_OFFSET) &
                           PG_ENTRY_OFFSET_MASK;
            virtAddrAdd = 1ULL << PML4_ENTRY_OFFSET;
            break;
        /* PML3 */
        case 3:
            addrEntryIdx = (*pVirtAddress >> PML3_ENTRY_OFFSET) &
                           PG_ENTRY_OFFSET_MASK;
            virtAddrAdd = 1ULL << PML3_ENTRY_OFFSET;
            break;
        /* PML2 */
        case 2:
            addrEntryIdx = (*pVirtAddress >> PML2_ENTRY_OFFSET) &
                           PG_ENTRY_OFFSET_MASK;
            virtAddrAdd = 1ULL << PML2_ENTRY_OFFSET;
            break;
        /* PML 1 */
        case 1:
            addrEntryIdx = (*pVirtAddress >> PML1_ENTRY_OFFSET) &
                           PG_ENTRY_OFFSET_MASK;
            virtAddrAdd = 1ULL << PML1_ENTRY_OFFSET;
            break;
        default:
            return OS_ERR_INCORRECT_VALUE;
    }

    error = OS_NO_ERR;

    /* Check all entries of the current table */
    while(*pVirtAddress < kVirtAddressMax &&
          addrEntryIdx < KERNEL_PGDIR_ENTRY_COUNT)
    {

        /* If mapped in the source, create an entry for the destination, map
         * both source and destination table and update destination entry in
         * table.
         */
        if((pSrcLevel[addrEntryIdx] & PAGE_FLAG_PRESENT) != 0)
        {
            /* If not last level, we are mapping a physical frame that is part
             * of the page directory.
             */
            if(kLevel != 1)
            {
                /* Allocate the new entry for the destination and get the entry
                 * for the source
                 */
                dstNextDirLevelFrame = _allocateFrames(1);
                if(dstNextDirLevelFrame == (uintptr_t)NULL)
                {
                    error = OS_ERR_NO_MORE_MEMORY;
                    break;
                }
                srcNextDirLevelFrame = _makeCanonical(pSrcLevel[addrEntryIdx] &
                                                      ~PAGE_SIZE_MASK,
                                                      true);
                srcNextDirLevelPage = GET_VIRT_MEM_ADDR(srcNextDirLevelFrame);
                dstNextDirLevelPage = GET_VIRT_MEM_ADDR(dstNextDirLevelFrame);

                /* Clear the new page table */
                memset((uintptr_t*)dstNextDirLevelPage,
                       0,
                       KERNEL_PGDIR_ENTRY_COUNT * sizeof(uintptr_t));

                /* Set the mapping flags */
                pDstLevel[addrEntryIdx] = dstNextDirLevelFrame |
                                          (pSrcLevel[addrEntryIdx] &
                                            ~(sPhysAddressWidthMask &
                                            (~PAGE_SIZE_MASK)));


                /* Copy next level, pVirtAddress will be updated there */
                error = _copyPgDirEntry((uintptr_t*)srcNextDirLevelPage,
                                        (uintptr_t*)dstNextDirLevelPage,
                                        pVirtAddress,
                                        kVirtAddressMax,
                                        kLevel - 1,
                                        kSetCOW);

                /* Stop on error */
                if(error != OS_NO_ERR)
                {
                    break;
                }
            }
            else
            {
                /* Set the source and destination as COW and read only */
                if((pSrcLevel[addrEntryIdx] & PAGE_FLAG_IS_HW) == 0)
                {
                    frameAddr = _makeCanonical(pSrcLevel[addrEntryIdx] &
                                               ~PAGE_SIZE_MASK,
                                               true);

                    refCount = _getAndLockReferenceCount(frameAddr);
                    if(*refCount < UINT16_MAX)
                    {
                        *refCount = *refCount + 1;
                        _unlockReferenceCount(frameAddr);
                    }
                    else
                    {
                        _unlockReferenceCount(frameAddr);
                        error = OS_ERR_NO_MORE_MEMORY;
                        break;
                    }

                    /* If the page was Read/Write, set as Read only and COW */
                    if((pSrcLevel[addrEntryIdx] & PAGE_FLAG_READ_WRITE) ==
                       PAGE_FLAG_READ_WRITE &&
                       kSetCOW == true)
                    {
                        pSrcLevel[addrEntryIdx] = PAGE_FLAG_COW |
                                                  (pSrcLevel[addrEntryIdx] &
                                                  ~PAGE_FLAG_READ_WRITE);
                    }
                }
                pDstLevel[addrEntryIdx] = pSrcLevel[addrEntryIdx];
                *pVirtAddress += virtAddrAdd;
            }
        }
        else
        {
            /* Nothing to do here, continue */
            *pVirtAddress += virtAddrAdd;
        }

        /* Go to next entry */
        ++addrEntryIdx;
    }

    /* On error, if level is PML4, clear the destination process page
     * directory
     */
    if(kLevel == 4 && error != OS_NO_ERR)
    {
        _releasePageDir(GET_PHYS_MEM_ADDR(pDstLevel), 0, 4);
    }

    return error;
}

static OS_RETURN_E _memoryMgrMapUser(uintptr_t*     pTableLevel,
                                     uintptr_t*     pVirtAddress,
                                     uintptr_t*     pPhysicalAddress,
                                     size_t*        pPageCount,
                                     const uint8_t  kLevel,
                                     const uint64_t kPageFlags)
{
    uint32_t    addrEntryIdx;
    uintptr_t   nextDirLevelFrame;
    uintptr_t   nextDirLevelPage;
    uintptr_t   startPageCount;
    size_t      initPageCount;
    uintptr_t   initVirtAddr;
    OS_RETURN_E error;
    OS_RETURN_E internalError;

    if(*pPageCount == 0)
    {
        return OS_NO_ERR;
    }

    /* Get the entry index */
    switch(kLevel)
    {
        /* PML4 */
        case 4:
            addrEntryIdx = (*pVirtAddress >> PML4_ENTRY_OFFSET) &
                           PG_ENTRY_OFFSET_MASK;
            initPageCount = *pPageCount;
            initVirtAddr  = *pVirtAddress;
            break;
        /* PML3 */
        case 3:
            addrEntryIdx = (*pVirtAddress >> PML3_ENTRY_OFFSET) &
                           PG_ENTRY_OFFSET_MASK;
            break;
        /* PML2 */
        case 2:
            addrEntryIdx = (*pVirtAddress >> PML2_ENTRY_OFFSET) &
                           PG_ENTRY_OFFSET_MASK;
            break;
        /* PML 1 */
        case 1:
            addrEntryIdx = (*pVirtAddress >> PML1_ENTRY_OFFSET) &
                           PG_ENTRY_OFFSET_MASK;
            break;
        default:
            return OS_ERR_INCORRECT_VALUE;
    }

    error = OS_NO_ERR;
    /* Check all entries of the current table */
    while(*pPageCount > 0 && addrEntryIdx < KERNEL_PGDIR_ENTRY_COUNT)
    {
        startPageCount = *pPageCount;

        /* If not already mapped, create a new table and init */
        if((pTableLevel[addrEntryIdx] & PAGE_FLAG_PRESENT) == 0)
        {
            /* If not last level, we are mapping a physical frame that is part
             * of the page directory.
             */
            if(kLevel != 1)
            {
                /* Allocate the new entry for the table */
                nextDirLevelFrame = _allocateFrames(1);
                if(nextDirLevelFrame == (uintptr_t)NULL)
                {
                    error = OS_ERR_NO_MORE_MEMORY;
                    break;
                }
                nextDirLevelPage = GET_VIRT_MEM_ADDR(nextDirLevelFrame);

                /* Clear the new page table */
                memset((uintptr_t*)nextDirLevelPage,
                       0,
                       KERNEL_PGDIR_ENTRY_COUNT * sizeof(uintptr_t));

                /* Set the mapping flags */
                pTableLevel[addrEntryIdx] = nextDirLevelFrame       |
                                            PAGE_FLAG_PAGE_SIZE_4KB |
                                            PAGE_FLAG_SUPER_ACCESS  |
                                            PAGE_FLAG_USER_ACCESS   |
                                            PAGE_FLAG_READ_WRITE    |
                                            PAGE_FLAG_CACHE_WB      |
                                            PAGE_FLAG_XD            |
                                            PAGE_FLAG_PRESENT;


                /* Map next level, pVirtAddress will be updated there */
                error = _memoryMgrMapUser((uintptr_t*)nextDirLevelPage,
                                          pVirtAddress,
                                          pPhysicalAddress,
                                          pPageCount,
                                          kLevel - 1,
                                          kPageFlags);

                /* Stop on error */
                if(error != OS_NO_ERR)
                {
                    /* The recursive partially errored mapping was released*/
                    pTableLevel[addrEntryIdx] = 0;
                    _releaseFrames(nextDirLevelFrame, 1);
                    *pPageCount = startPageCount;
                    break;
                }
            }
            else
            {
                /* Set the mapping flags */
                pTableLevel[addrEntryIdx] = *pPhysicalAddress | kPageFlags;

                /* Update position */
                *pVirtAddress += KERNEL_PAGE_SIZE;
                *pPhysicalAddress += KERNEL_PAGE_SIZE;
                *pPageCount -= 1;
            }
        }
        else
        {
            /* If not in the last level, just get the mapping and pursue */
            if(kLevel != 1)
            {

                /* Get the entry and map it */
                nextDirLevelFrame = _makeCanonical(pTableLevel[addrEntryIdx] &
                                                   ~PAGE_SIZE_MASK,
                                                   true);
                nextDirLevelPage = GET_VIRT_MEM_ADDR(nextDirLevelFrame);

                /* Pursue */
                error = _memoryMgrMapUser((uintptr_t*)nextDirLevelPage,
                                          pVirtAddress,
                                          pPhysicalAddress,
                                          pPageCount,
                                          kLevel - 1,
                                          kPageFlags);

                /* Stop on error */
                if(error != OS_NO_ERR)
                {
                    /* The recursive partially errored mapping was released*/
                    *pPageCount = startPageCount;
                    break;
                }
            }
            else
            {
                /* This page is already mapped, error */
                error = OS_ERR_ALREADY_EXIST;
                break;
            }
        }

        /* Go to next entry */
        ++addrEntryIdx;
    }

    /* On error, release the mapped memory in the last level */
    if(kLevel == 4 && error != OS_NO_ERR && initPageCount - *pPageCount != 0)
    {
        initPageCount = initPageCount - *pPageCount;
        internalError = _memoryMgrUnmapUser(pTableLevel,
                                            &initVirtAddr,
                                            &initPageCount,
                                            4);
        MEM_ASSERT(internalError == OS_NO_ERR,
                   "Failed to unmap already mapped memory",
                   internalError);
    }

    return error;
}

static OS_RETURN_E _memoryMgrUnmapUser(uintptr_t*     pTableLevel,
                                       uintptr_t*     pVirtAddress,
                                       size_t*        pPageCount,
                                       const uint8_t  kLevel)
{
    uint32_t    i;
    uint32_t    addrEntryIdx;
    uintptr_t   nextDirLevelFrame;
    uintptr_t*  nextDirLevelPage;
    OS_RETURN_E error;

    if(*pPageCount == 0)
    {
        return OS_NO_ERR;
    }

    /* Get the entry index */
    switch(kLevel)
    {
        /* PML4 */
        case 4:
            addrEntryIdx = (*pVirtAddress >> PML4_ENTRY_OFFSET) &
                           PG_ENTRY_OFFSET_MASK;
            break;
        /* PML3 */
        case 3:
            addrEntryIdx = (*pVirtAddress >> PML3_ENTRY_OFFSET) &
                           PG_ENTRY_OFFSET_MASK;
            break;
        /* PML2 */
        case 2:
            addrEntryIdx = (*pVirtAddress >> PML2_ENTRY_OFFSET) &
                           PG_ENTRY_OFFSET_MASK;
            break;
        /* PML 1 */
        case 1:
            addrEntryIdx = (*pVirtAddress >> PML1_ENTRY_OFFSET) &
                           PG_ENTRY_OFFSET_MASK;
            break;
        default:
            return OS_ERR_INCORRECT_VALUE;
    }

    error = OS_NO_ERR;
    /* Check all entries of the current table */
    while(*pPageCount > 0 && addrEntryIdx < KERNEL_PGDIR_ENTRY_COUNT)
    {
        /* If mapped unmap what needs to be mapped */
        if((pTableLevel[addrEntryIdx] & PAGE_FLAG_PRESENT) != 0)
        {
            /* If not last level, we are mapping a physical frame that is part
             * of the page directory.
             */
            if(kLevel != 1)
            {
                /* Get the entry and map it */
                nextDirLevelFrame = _makeCanonical(pTableLevel[addrEntryIdx] &
                                                   ~PAGE_SIZE_MASK,
                                                   true);
                nextDirLevelPage = (uintptr_t*)
                                   GET_VIRT_MEM_ADDR(nextDirLevelFrame);


                /* Unmap next level, pVirtAddress will be updated there */
                error = _memoryMgrUnmapUser(nextDirLevelPage,
                                            pVirtAddress,
                                            pPageCount,
                                            kLevel - 1);

                if(error == OS_NO_ERR)
                {
                    /* Check if we can release the frame */
                    for(i = 0; i < KERNEL_PGDIR_ENTRY_COUNT; ++i)
                    {
                        if((nextDirLevelPage[i] & PAGE_FLAG_PRESENT) != 0)
                        {
                            break;
                        }
                        if(i == KERNEL_PGDIR_ENTRY_COUNT)
                        {
                            _releaseFrames(nextDirLevelFrame, 1);
                            pTableLevel[addrEntryIdx] = 0;
                        }
                    }
                }
                else
                {
                    /* Stop on error */
                    break;
                }
            }
            else
            {
                /* Unset the mapping */
                pTableLevel[addrEntryIdx] = 0;

                /* Update position */
                *pVirtAddress += KERNEL_PAGE_SIZE;
                *pPageCount -= 1;
            }
        }
        else
        {
            return OS_ERR_NO_SUCH_ID;
        }

        /* Go to next entry */
        ++addrEntryIdx;
    }

    return error;
}

static OS_RETURN_E _memoryManageCOW(const uintptr_t        kFaultVirtAddr,
                                    const uintptr_t        kPhysAddr,
                                    const kernel_thread_t* kpThread)
{
    uint16_t*       refCount;
    uintptr_t       baseVirt;
    uintptr_t       newFrame;
    uintptr_t       newPage;
    int8_t          i;
    uint32_t        pmlEntry[4];
    uintptr_t*      pPageTable[4];
    uintptr_t       nextPtable;
    uintptr_t       newEntryValue;
    memproc_info_t* pProcessMem;

    /* Lock the process to avoid frame modification during the mapping */
    pProcessMem = kpThread->pProcess->pMemoryData;
    KERNEL_LOCK(pProcessMem->lock);

    /* Update the page table and the reference count */
    refCount = _getAndLockReferenceCount(kPhysAddr);
    MEM_ASSERT(*refCount > 0,
               "Invalid reference count zero",
               OS_ERR_INCORRECT_VALUE);

    baseVirt = GET_VIRT_MEM_ADDR(kPhysAddr);
    /* If the reference count is greater than 1, we need to copy
     * the frame.
     */
    if(*refCount > 1)
    {

        /* Release the reference */
        *refCount = *refCount - 1;
        _unlockReferenceCount(kPhysAddr);

        /* Allocate the new frame */
        newFrame = _allocateFrames(1);
        if(newFrame == (uintptr_t)NULL)
        {
            refCount = _getAndLockReferenceCount(kPhysAddr);
            *refCount = *refCount + 1;
            _unlockReferenceCount(kPhysAddr);
            return OS_ERR_NO_MORE_MEMORY;
        }
        newPage = GET_VIRT_MEM_ADDR(newFrame);

        /* Copy the frame */
        memcpy((uintptr_t*)newPage, (uintptr_t*)baseVirt, KERNEL_PAGE_SIZE);
    }
    else
    {
        _unlockReferenceCount(kPhysAddr);
        newFrame = _makeCanonical(kPhysAddr, true);
    }


    /* Update the mapping */
    pmlEntry[3] = (kFaultVirtAddr >> PML4_ENTRY_OFFSET) & PG_ENTRY_OFFSET_MASK;
    pmlEntry[2] = (kFaultVirtAddr >> PML3_ENTRY_OFFSET) & PG_ENTRY_OFFSET_MASK;
    pmlEntry[1] = (kFaultVirtAddr >> PML2_ENTRY_OFFSET) & PG_ENTRY_OFFSET_MASK;
    pmlEntry[0] = (kFaultVirtAddr >> PML1_ENTRY_OFFSET) & PG_ENTRY_OFFSET_MASK;

    for(i = 3; i >= 0; --i)
    {
        if(i == 3)
        {
            pPageTable[i] = (uintptr_t*)GET_VIRT_MEM_ADDR(pProcessMem->pageDir);
        }
        else
        {
            nextPtable = _makeCanonical(pPageTable[i + 1][pmlEntry[i + 1]] &
                                        ~PAGE_SIZE_MASK,
                                        true);
            pPageTable[i] = (uintptr_t*)GET_VIRT_MEM_ADDR(nextPtable);
        }
    }

    /* Get the flags */
    newEntryValue = pPageTable[0][pmlEntry[0]] &
                    ~(sPhysAddressWidthMask & ~PAGE_SIZE_MASK);

    /* Remove COW and add new address */
    newEntryValue =  (newEntryValue | PAGE_FLAG_READ_WRITE) & ~PAGE_FLAG_COW;
    pPageTable[0][pmlEntry[0]] = newEntryValue | newFrame;

    KERNEL_UNLOCK(pProcessMem->lock);

    return OS_NO_ERR;
}

static inline void _getReferenceIndexTable(const uintptr_t      kPhysAddr,
                                           frame_meta_table_t** ppTable,
                                           size_t*              pEntryIdx)
{
    /* Search for the entry */
    *ppTable = spFramesMeta;
    while(*ppTable != NULL)
    {
        if(kPhysAddr >= (*ppTable)->firstFrame &&
           kPhysAddr <= (*ppTable)->lastFrame)
        {
            break;
        }
        *ppTable = (*ppTable)->pNext;
    }

    MEM_ASSERT(*ppTable != NULL,
               "Failed to find physical address in frames meta table",
               OS_ERR_NO_SUCH_ID);

    /* Calculate the id in the index */
    *pEntryIdx = (kPhysAddr - (*ppTable)->firstFrame) >> PML1_ENTRY_OFFSET;
}

static uint16_t* _getAndLockReferenceCount(const uintptr_t kPhysAddr)
{
    size_t              entryIdx;
    frame_meta_table_t* pTable;

    if(spFramesMeta == NULL)
    {
        return 0;
    }

    _getReferenceIndexTable(kPhysAddr, &pTable, &entryIdx);
    KERNEL_LOCK(pTable->lock);

    return &pTable->pRefCountTable[entryIdx];
}

static void _unlockReferenceCount(const uintptr_t kPhysAddr)
{
    size_t              entryIdx;
    frame_meta_table_t* pTable;

    if(spFramesMeta == NULL)
    {
        return;
    }

    _getReferenceIndexTable(kPhysAddr, &pTable, &entryIdx);
    KERNEL_UNLOCK(pTable->lock);
}

static uint64_t _translateFlags(const uint32_t kFlags)
{
    uint64_t mapFlags;

    mapFlags = 0;

    if((kFlags & MEMMGR_MAP_KERNEL) == MEMMGR_MAP_KERNEL)
    {
        mapFlags |= PAGE_FLAG_SUPER_ACCESS | PAGE_FLAG_GLOBAL;
    }
    if((kFlags & MEMMGR_MAP_USER) == MEMMGR_MAP_USER)
    {
        mapFlags |= PAGE_FLAG_USER_ACCESS;
    }
    if((kFlags & MEMMGR_MAP_RW) == MEMMGR_MAP_RW)
    {
        mapFlags |= PAGE_FLAG_READ_WRITE;
    }
    else
    {
        mapFlags |= PAGE_FLAG_READ_ONLY;
    }
    if((kFlags & MEMMGR_MAP_CACHE_DISABLED) == MEMMGR_MAP_CACHE_DISABLED)
    {
        mapFlags |= PAGE_FLAG_CACHE_DISABLED;
    }
    else
    {
        mapFlags |= PAGE_FLAG_CACHE_WB;
    }
    if((kFlags & MEMMGR_MAP_WRITE_COMBINING) == MEMMGR_MAP_WRITE_COMBINING)
    {
        mapFlags |= PAGE_FLAG_CACHE_WC;
    }
    if((kFlags & MEMMGR_MAP_EXEC) != MEMMGR_MAP_EXEC)
    {
        mapFlags |= PAGE_FLAG_XD;
    }
    if((kFlags & MEMMGR_MAP_HARDWARE) == MEMMGR_MAP_HARDWARE)
    {
        mapFlags |= PAGE_FLAG_CACHE_DISABLED | PAGE_FLAG_IS_HW;
    }
    if((kFlags & MEMMGR_MAP_COW) == MEMMGR_MAP_COW)
    {
        mapFlags |= PAGE_FLAG_COW;
    }

    return mapFlags;
}

void memoryMgrInit(void)
{
    OS_RETURN_E error;

    /* Initialize structures */
    sPhysMemList.pQueue = kQueueCreate(true);
    KERNEL_SPINLOCK_INIT(sPhysMemList.lock);

    sKernelFreePagesList.pQueue = kQueueCreate(true);
    KERNEL_SPINLOCK_INIT(sKernelFreePagesList.lock);

    sPhysAddressWidthMask = ((1ULL << physAddressWidth) - 1);
    sCanonicalBound       = ((1ULL << (virtAddressWidth - 1)) - 1);

    /* Clear the low entries used during boot */
    spKernelPageDir[0] = 0;

    /* Setup the memory frames mapping */
    spKernelPageDir[KERNEL_MEM_PML4_ENTRY] =
        ((uintptr_t)_physicalMapDir - KERNEL_MEM_OFFSET) |
        PAGE_FLAG_SUPER_ACCESS       |
        PAGE_FLAG_CACHE_WB           |
        PAGE_FLAG_READ_WRITE         |
        PAGE_FLAG_PRESENT;


    /* Setup the kernel free pages */
    _memoryMgrInitKernelFreePages();

    /* Detect the memory */
    _memoryMgrDetectMemory();

    /* Update the whole page table */
    cpuSetPageDirectory((uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);

    /* Map the kernel */
    _memoryMgrMapKernel();

    /* Creates the frames metadata */
    _memoryMgrCreateFramesMeta();

    /* Registers the page fault handler */
    error = exceptionRegister(PAGE_FAULT_EXC_LINE, _pageFaultHandler);
    MEM_ASSERT(error == OS_NO_ERR,
               "Failed to register the page fault handler",
               error);

    /* Setup the PAT as follows:
     * WC UC- WT WB UC UC- WT WB
     */
    __asm__ __volatile__ (
        "mov $0x277, %%rcx\n\t"
        "rdmsr\n\t"
        "and $0xFFFFFFFFF8FFFFFF, %%rdx\n\t"
        "or  $0x01000000, %%rdx\n\t"
        "wrmsr\n\t"
        :::"rax", "rcx", "rdx");

#if MEMORY_MGR_DEBUG_ENABLED
    _printKernelMap();
#endif
}

void* memoryKernelMap(const void*    kPhysicalAddress,
                      const size_t   kSize,
                      const uint32_t kFlags,
                      OS_RETURN_E*   pError)
{
    uintptr_t   kernelPages;
    size_t      pageCount;
    OS_RETURN_E error;

#if MEMORY_MGR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Mapping physical address 0x%p (%dB) | Flags: 0x%x",
           kPhysicalAddress,
           kSize,
           kFlags);
#endif

    /* Check size */
    if((kSize & PAGE_SIZE_MASK) != 0 || kSize < KERNEL_PAGE_SIZE)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_INCORRECT_VALUE;
        }
        return NULL;
    }

    pageCount = kSize / KERNEL_PAGE_SIZE;

    KERNEL_LOCK(sLock);

    /* Allocate pages */
    kernelPages = _allocateKernelPages(pageCount);
    if(kernelPages == 0)
    {
        KERNEL_UNLOCK(sLock);

        if(pError != NULL)
        {
            *pError = OS_ERR_NO_MORE_MEMORY;
        }
        return NULL;
    }

    /* Apply mapping */
    error = _memoryMgrMap(kernelPages,
                          (uintptr_t)kPhysicalAddress,
                          pageCount,
                          kFlags | MEMMGR_MAP_KERNEL | PAGE_FLAG_GLOBAL,
                          (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);
    if(error != OS_NO_ERR)
    {
        _releaseKernelPages(kernelPages, pageCount);
        kernelPages = (uintptr_t)NULL;
    }

    KERNEL_UNLOCK(sLock);

    if(pError != NULL)
    {
        *pError = error;
    }
    return (void*)kernelPages;
}

OS_RETURN_E memoryKernelUnmap(const void* kVirtualAddress, const size_t kSize)
{
    size_t      pageCount;
    OS_RETURN_E error;

#if MEMORY_MGR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Unmapping virtual address 0x%p (%dB)",
           kVirtualAddress,
           kSize);
#endif

    /* Check size */
    if((kSize & PAGE_SIZE_MASK) != 0 || kSize < KERNEL_PAGE_SIZE)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    pageCount = kSize / KERNEL_PAGE_SIZE;

    /* Check if actually kernel addresses */
    if((uintptr_t)kVirtualAddress < sKernelVirtualMemBounds.base ||
       (uintptr_t)kVirtualAddress >= sKernelVirtualMemBounds.limit)
    {
        return OS_ERR_OUT_OF_BOUND;
    }

    KERNEL_LOCK(sLock);

    /* Unmap */
    error = _memoryMgrUnmap((uintptr_t)kVirtualAddress,
                            pageCount,
                            (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);

    /* Release the kernel page if correctly unmaped */
    if(error == OS_NO_ERR)
    {
        _releaseKernelPages((uintptr_t)kVirtualAddress, pageCount);
    }

    KERNEL_UNLOCK(sLock);

    return error;
}

uintptr_t memoryMapStack(const size_t      kSize,
                         const bool        kIsKernel,
                         kernel_process_t* pProcess)
{
    size_t             pageCount;
    size_t             mappedCount;
    size_t             i;
    OS_RETURN_E        error;
    uintptr_t          pageBaseAddress;
    uintptr_t          newFrame;
    uintptr_t          mapFlags;
    memproc_info_t*    pProcMem;
    uintptr_t          pgDir;
    kernel_spinlock_t* pLock;

    /* Get the page count */
    pageCount = ALIGN_UP(kSize, KERNEL_PAGE_SIZE) / KERNEL_PAGE_SIZE;

    if(kIsKernel == true)
    {
        pLock = &sLock;
        pgDir = (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET;
    }
    else
    {
        pProcMem = pProcess->pMemoryData;
        pLock = &pProcMem->lock;
        pgDir = pProcMem->pageDir;
    }

    KERNEL_LOCK(*pLock);

    /* Request the pages + 1 to catch overflow (not mapping the last page)*/
    if(kIsKernel == true)
    {
        pageBaseAddress = _allocateKernelPages(pageCount + 1);
        if(pageBaseAddress == 0)
        {
            KERNEL_UNLOCK(*pLock);
            return (uintptr_t)NULL;
        }
        mapFlags = MEMMGR_MAP_RW | MEMMGR_MAP_KERNEL;
    }
    else
    {
        pageBaseAddress = _allocateUserPages(pageCount + 1, pProcess, true);
        if(pageBaseAddress == 0)
        {
            KERNEL_UNLOCK(*pLock);
            return (uintptr_t)NULL;
        }
        mapFlags = MEMMGR_MAP_RW | MEMMGR_MAP_USER;
    }

    /* Now map, we do not need contiguous frames */
    for(i = 0; i < pageCount; ++i)
    {
        newFrame = _allocateFrames(1);
        if(newFrame == 0)
        {
            break;
        }

        error = _memoryMgrMap(pageBaseAddress + i * KERNEL_PAGE_SIZE,
                              newFrame,
                              1,
                              mapFlags,
                              pgDir);
        if(error != OS_NO_ERR)
        {
            /* On error, release the frame */
            _releaseFrames(newFrame, 1);
            break;
        }
    }

    /* Check if everything is mapped, if not unmap and return */
    if(i < pageCount)
    {
        if(i != 0)
        {
            mappedCount = i;
            /* Release frames */
            for(i = 0; i < mappedCount; ++i)
            {
                newFrame = _memoryMgrGetPhysAddr(pageBaseAddress +
                                                 KERNEL_PAGE_SIZE * i,
                                                 pgDir,
                                                 NULL);
                MEM_ASSERT(newFrame != MEMMGR_PHYS_ADDR_ERROR,
                           "Invalid physical frame",
                           OS_ERR_INCORRECT_VALUE);
                _releaseFrames(newFrame, 1);
            }

            _memoryMgrUnmap(pageBaseAddress, mappedCount, pgDir);
        }
        if(kIsKernel == true)
        {
            _releaseKernelPages(pageBaseAddress, pageCount + 1);
        }
        else
        {
            _releaseUserPages(pageBaseAddress, pageCount + 1, pProcess);
        }

        pageBaseAddress = (uintptr_t)NULL;
    }

    KERNEL_UNLOCK(*pLock);

    if(pageBaseAddress != (uintptr_t)NULL)
    {
        pageBaseAddress += (pageCount * KERNEL_PAGE_SIZE);
    }
    return pageBaseAddress;
}

void memoryUnmapStack(const uintptr_t   kEndAddress,
                      const size_t      kSize,
                      const bool        kIsKernel,
                      kernel_process_t* pProcess)
{
    size_t          pageCount;
    size_t          i;
    uintptr_t       frameAddr;
    uintptr_t       baseAddress;
    memproc_info_t* pProcMem;

    MEM_ASSERT((kEndAddress & PAGE_SIZE_MASK) == 0 &&
                (kSize & PAGE_SIZE_MASK) == 0 &&
                kSize != 0,
                "Unmaped kernel stack with invalid parameters",
                OS_ERR_INCORRECT_VALUE);

    /* Get the page count */
    pageCount = kSize / KERNEL_PAGE_SIZE;
    baseAddress = kEndAddress - kSize;

    pProcMem = pProcess->pMemoryData;

    KERNEL_LOCK(sLock);

    /* Free the frames and memory */
    for(i = 0; i < pageCount; ++i)
    {
        frameAddr = _memoryMgrGetPhysAddr(baseAddress + KERNEL_PAGE_SIZE * i,
                                          pProcMem->pageDir,
                                          NULL);
        MEM_ASSERT(frameAddr != MEMMGR_PHYS_ADDR_ERROR,
                   "Invalid physical frame",
                   OS_ERR_INCORRECT_VALUE);
        _releaseFrames(frameAddr, 1);
    }

    /* Unmap the memory */
    _memoryMgrUnmap(baseAddress, pageCount, pProcMem->pageDir);
    if(kIsKernel == true)
    {
        _releaseKernelPages(baseAddress, pageCount + 1);
    }
    else
    {
        _releaseUserPages(baseAddress, pageCount + 1, pProcess);
    }

    KERNEL_UNLOCK(sLock);
}

uintptr_t memoryMgrGetPhysAddr(const uintptr_t         kVirtualAddress,
                               const kernel_process_t* kpProcess,
                               uint32_t*               pFlags)
{
    uintptr_t       retPhysAddr;
    memproc_info_t* pMemInfo;

    pMemInfo = kpProcess->pMemoryData;

    KERNEL_LOCK(sLock);

    retPhysAddr = _memoryMgrGetPhysAddr(kVirtualAddress,
                                        pMemInfo->pageDir,
                                        pFlags);

    KERNEL_UNLOCK(sLock);

    return retPhysAddr;
}

void* memoryKernelAllocate(const size_t   kSize,
                           const uint32_t kFlags,
                           OS_RETURN_E*   pError)
{
    size_t      pageCount;
    size_t      mappedCount;
    size_t      i;
    OS_RETURN_E error;
    OS_RETURN_E internalError;
    uintptr_t   pageBaseAddress;
    uintptr_t   newFrame;

#if MEMORY_MGR_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Allocating address %dB | Flags: 0x%x",
           kSize,
           kFlags);
#endif

    /* Check size */
    if((kSize & PAGE_SIZE_MASK) != 0 || kSize < KERNEL_PAGE_SIZE)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_INCORRECT_VALUE;
        }
        return NULL;
    }

    /* Check flags */
    if((kFlags & MEMMGR_MAP_HARDWARE) == MEMMGR_MAP_HARDWARE)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_INCORRECT_VALUE;
        }
        return NULL;
    }

    /* Get the page count */
    pageCount = kSize / KERNEL_PAGE_SIZE;

    KERNEL_LOCK(sLock);

    /* Request the pages */
    pageBaseAddress = _allocateKernelPages(pageCount);
    if(pageBaseAddress == 0)
    {
        KERNEL_UNLOCK(sLock);
        if(pError != NULL)
        {
            *pError = OS_ERR_NO_MORE_MEMORY;
        }
        return NULL;
    }

    /* Now map, we do not need contiguous frames */
    for(i = 0; i < pageCount; ++i)
    {
        newFrame = _allocateFrames(1);
        if(newFrame == 0)
        {
            break;
        }

        error = _memoryMgrMap(pageBaseAddress + i * KERNEL_PAGE_SIZE,
                              newFrame,
                              1,
                              kFlags,
                              (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);
        if(error != OS_NO_ERR)
        {
            /* On error, release the frame */
            _releaseFrames(newFrame, 1);
            break;
        }
    }

    /* Check if everything is mapped, if not unmap and return */
    if(i < pageCount)
    {
        if(i != 0)
        {
            mappedCount = i;
            /* Release frames */
            for(i = 0; i < mappedCount; ++i)
            {
                newFrame = _memoryMgrGetPhysAddr(pageBaseAddress +
                                                 KERNEL_PAGE_SIZE * i,
                                                 (uintptr_t)spKernelPageDir -
                                                 KERNEL_MEM_OFFSET,
                                                 NULL);
                MEM_ASSERT(newFrame != MEMMGR_PHYS_ADDR_ERROR,
                           "Invalid physical frame",
                           OS_ERR_INCORRECT_VALUE);
                _releaseFrames(newFrame, 1);
            }

            internalError = _memoryMgrUnmap(pageBaseAddress,
                                            mappedCount,
                                            (uintptr_t)spKernelPageDir -
                                            KERNEL_MEM_OFFSET);
            MEM_ASSERT(internalError == OS_NO_ERR,
                       "Failed to unmapp mapped memory",
                       internalError);
        }
        _releaseKernelPages(pageBaseAddress, pageCount);

        pageBaseAddress = (uintptr_t)NULL;
    }

    KERNEL_UNLOCK(sLock);

    if(pError != NULL)
    {
        *pError = error;
    }
    return (void*)pageBaseAddress;
}

OS_RETURN_E memoryKernelFree(const void* kVirtualAddress, const size_t kSize)
{
    size_t      pageCount;
    size_t      i;
    uintptr_t   frameAddr;
    OS_RETURN_E error;

    if(((uintptr_t)kVirtualAddress & PAGE_SIZE_MASK) != 0)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    if((kSize & PAGE_SIZE_MASK) != 0 || kSize < KERNEL_PAGE_SIZE)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Get the page count */
    pageCount = kSize / KERNEL_PAGE_SIZE;

    KERNEL_LOCK(sLock);

    /* Free the frames and memory */
    for(i = 0; i < pageCount; ++i)
    {
        frameAddr = _memoryMgrGetPhysAddr((uintptr_t)kVirtualAddress +
                                          KERNEL_PAGE_SIZE * i,
                                          (uintptr_t)spKernelPageDir -
                                          KERNEL_MEM_OFFSET,
                                          NULL);
        MEM_ASSERT(frameAddr != MEMMGR_PHYS_ADDR_ERROR,
                   "Invalid physical frame",
                   OS_ERR_INCORRECT_VALUE);
        _releaseFrames(frameAddr, 1);
    }

    /* Unmap the memory */
    error = _memoryMgrUnmap((uintptr_t)kVirtualAddress,
                            pageCount,
                            (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);
    MEM_ASSERT(error == OS_NO_ERR,
               "Invalid unmapping frame",
               OS_ERR_INCORRECT_VALUE);

    /* Release pages */
    _releaseKernelPages((uintptr_t)kVirtualAddress, pageCount);

    KERNEL_UNLOCK(sLock);

    return error;
}

void* memoryCreateProcessMemoryData(void)
{
    memproc_info_t* pMemProcInfo;

    /* Create the memory structure */
    pMemProcInfo = kmalloc(sizeof(memproc_info_t));
    if(pMemProcInfo == NULL)
    {
        return NULL;
    }

    /* Create the page directory */
    if(schedIsInit() == true)
    {
        /* Allocate a frame for the page directory */
        pMemProcInfo->pageDir = (uintptr_t)NULL;
    }
    else
    {
        /* When the scheduler is not initialized, use the kernel page dir */
        pMemProcInfo->pageDir = (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET;
    }

    /* Create the free page table */
    pMemProcInfo->freePageTable.pQueue = kQueueCreate(false);
    if(pMemProcInfo->freePageTable.pQueue == NULL)
    {
        kfree(pMemProcInfo);
        return NULL;
    }
    KERNEL_SPINLOCK_INIT(pMemProcInfo->freePageTable.lock);

    /* Add free pages */
    _addBlock(&pMemProcInfo->freePageTable,
              USER_MEMORY_START,
              USER_MEMORY_END - USER_MEMORY_START);

    KERNEL_SPINLOCK_INIT(pMemProcInfo->lock);

    return pMemProcInfo;
}

void memoryDestroyProcessMemoryData(void* pMemoryData)
{
    memproc_info_t* pMemProcInfo;

    pMemProcInfo = pMemoryData;

    MEM_ASSERT(pMemProcInfo->pageDir !=
               (uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET,
               "Tried to release kernel page directory",
               OS_ERR_UNAUTHORIZED_ACTION);

    KERNEL_LOCK(pMemProcInfo->lock);

    /* Destroy the page directory */
    _releasePageDir(pMemProcInfo->pageDir, 0, 4);

    /* Destroy the free page table */
    KERNEL_LOCK(pMemProcInfo->freePageTable.lock);

    kQueueClean(pMemProcInfo->freePageTable.pQueue, true);
    kQueueDestroy(&pMemProcInfo->freePageTable.pQueue);

    KERNEL_UNLOCK(pMemProcInfo->freePageTable.lock);
    KERNEL_UNLOCK(pMemProcInfo->lock);

    /* Release the memory structure */
    kfree(pMemProcInfo);
}

OS_RETURN_E memoryCloneProcessMemory(kernel_process_t* pDstProcess)
{
    memproc_info_t* pSrcMemProcInfo;
    memproc_info_t* pDstMemProcInfo;
    uintptr_t       pSrcPgDir;
    uintptr_t       pDstPgDir;
    OS_RETURN_E     error;
    uintptr_t       addrSpace;
    kqueue_node_t*  pNewNode;
    kqueue_node_t*  pNode;
    mem_range_t*    pNewRange;

    pSrcMemProcInfo = schedGetCurrentProcess()->pMemoryData;
    pDstMemProcInfo = pDstProcess->pMemoryData;

    /* Check that the free page is empty and that the pdgir is NULL */
    if(pDstMemProcInfo->pageDir != (uintptr_t)NULL)
    {
        return OS_ERR_INCORRECT_VALUE;
    }
    /* Clean the queue just in case */
    kQueueClean(pDstMemProcInfo->freePageTable.pQueue, true);

    /* Allocate the frame for the destrination page directory */
    pDstMemProcInfo->pageDir = _allocateFrames(1);
    if(pDstMemProcInfo->pageDir == (uintptr_t)NULL)
    {
        return OS_ERR_NO_MORE_MEMORY;
    }

    /* Copy the free pages of the current process */
    KERNEL_LOCK(pSrcMemProcInfo->freePageTable.lock);
    pNode = pSrcMemProcInfo->freePageTable.pQueue->pHead;
    while(pNode != NULL)
    {
        pNewRange = kmalloc(sizeof(mem_range_t));
        if(pNewRange == NULL)
        {
            KERNEL_UNLOCK(pSrcMemProcInfo->freePageTable.lock);
            error = OS_ERR_NO_MORE_MEMORY;
            goto CLONE_CLEANUP;
        }
        pNewNode = kQueueCreateNode(pNewRange, false);
        if(pNewNode == NULL)
        {
            kfree(pNewRange);
            KERNEL_UNLOCK(pSrcMemProcInfo->freePageTable.lock);
            error = OS_ERR_NO_MORE_MEMORY;
            goto CLONE_CLEANUP;
        }

        pNewRange->base  = ((mem_range_t*)pNode->pData)->base;
        pNewRange->limit = ((mem_range_t*)pNode->pData)->limit;
        kQueuePushPrio(pNewNode,
                       pDstMemProcInfo->freePageTable.pQueue,
                       KERNEL_VIRTUAL_ADDR_MAX - pNewRange->base);

        pNode = pNode->pNext;
    }
    KERNEL_UNLOCK(pSrcMemProcInfo->freePageTable.lock);

    KERNEL_LOCK(pSrcMemProcInfo->lock);

    /* Map the source and destination page directories */
    pSrcPgDir = GET_VIRT_MEM_ADDR(pSrcMemProcInfo->pageDir);
    pDstPgDir = GET_VIRT_MEM_ADDR(pDstMemProcInfo->pageDir);

    /* Clear the destination page directory and set the start address */
    memset((uintptr_t*)pDstPgDir,
           0,
           sizeof(uintptr_t*) * KERNEL_PGDIR_ENTRY_COUNT);
    addrSpace = USER_MEMORY_START;

    /* Copy the user-land space */
    error = _copyPgDirEntry((uintptr_t*)pSrcPgDir,
                            (uintptr_t*)pDstPgDir,
                            &addrSpace,
                            USER_MEMORY_END,
                            4,
                            true);
    if(error != OS_NO_ERR)
    {
        /* The faulted copy page dir has already cleared the new page dir */
        pDstMemProcInfo->pageDir = (uintptr_t)NULL;
        goto CLONE_CLEANUP;
    }
    MEM_ASSERT(addrSpace == USER_MEMORY_END,
               "Invalid mapping for user space",
               OS_ERR_INCORRECT_VALUE);

    /* Mapp the high-kernel space */
    ((uintptr_t*)pDstPgDir)[KERNEL_MEM_PML4_ENTRY] =
        ((uintptr_t*)pSrcPgDir)[KERNEL_MEM_PML4_ENTRY];
    ((uintptr_t*)pDstPgDir)[KERNEL_PML4_KERNEL_ENTRY] =
        ((uintptr_t*)pSrcPgDir)[KERNEL_PML4_KERNEL_ENTRY];

    /* The source process is the running one, invalidate its whole TLB to
     * account for COW update
     */
    cpuSetPageDirectory(pSrcMemProcInfo->pageDir);

CLONE_CLEANUP:
    if(error != OS_NO_ERR)
    {
        kQueueClean(pDstMemProcInfo->freePageTable.pQueue, true);
        if(pDstMemProcInfo->pageDir != (uintptr_t)NULL)
        {
            _releaseFrames(pDstMemProcInfo->pageDir, 1);
            pDstMemProcInfo->pageDir = (uintptr_t)NULL;
        }
    }

    KERNEL_UNLOCK(pSrcMemProcInfo->lock);
    return error;
}

uintptr_t memoryGetUserStartAddr(void)
{
    return (uintptr_t)USER_MEMORY_START;
}

uintptr_t memoryGetUserEndAddr(void)
{
    return (uintptr_t)USER_MEMORY_END;
}

uintptr_t memoryAllocFrames(const size_t kFrameCount)
{
    return _allocateFrames(kFrameCount);
}

void memoryReleaseFrame(const uintptr_t kBaseAddress,
                        const size_t    kFrameCount)
{
    return _releaseFrames(kBaseAddress, kFrameCount);
}

OS_RETURN_E memoryUserMapDirect(const void*       kPhysicalAddress,
                                const void*       kVirtualAddress,
                                const size_t      kSize,
                                const uint32_t    kFlags,
                                kernel_process_t* pProcess)
{
    memproc_info_t* pMemProcInfo;
    uint64_t        flags;
    size_t          pageCount;
    uintptr_t       startVirt;
    uintptr_t       startPhys;
    uintptr_t*      pPageDir;
    OS_RETURN_E     error;

    pMemProcInfo = pProcess->pMemoryData;

    /* Aligne memory */
    if((kSize & PAGE_SIZE_MASK) != 0 ||
       kSize < KERNEL_PAGE_SIZE ||
       ((uintptr_t)kVirtualAddress & PAGE_SIZE_MASK) != 0 ||
       ((uintptr_t)kPhysicalAddress & PAGE_SIZE_MASK) != 0)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    pageCount = kSize / KERNEL_PAGE_SIZE;

    /* Get flags */
    flags = PAGE_FLAG_PRESENT | _translateFlags(kFlags);

    KERNEL_LOCK(pMemProcInfo->lock);

    /* Check if the mapping already exists */
    if(_memoryMgrIsMapped((uintptr_t)kVirtualAddress,
                          pageCount,
                          pMemProcInfo->pageDir,
                          false) == true)
    {
        KERNEL_UNLOCK(pMemProcInfo->lock);
        return OS_ERR_ALREADY_EXIST;
    }

    /* Temporary map the process page directory */
    pPageDir = (uintptr_t*)GET_VIRT_MEM_ADDR(pMemProcInfo->pageDir);

    /* Map the data */
    startVirt = (uintptr_t)kVirtualAddress;
    startPhys = (uintptr_t)kPhysicalAddress;
    error = _memoryMgrMapUser(pPageDir,
                              &startVirt,
                              &startPhys,
                              &pageCount,
                              4,
                              flags);

    if(error != OS_NO_ERR)
    {
        /* Remove from user free pages */
        _removeBlock(&pMemProcInfo->freePageTable,
                     (uintptr_t)kVirtualAddress,
                     kSize);
    }

    KERNEL_UNLOCK(pMemProcInfo->lock);

    return error;
}

OS_RETURN_E memoryUserUnmap(const void*       kVirtualAddress,
                            const size_t      kSize,
                            kernel_process_t* pProcess)
{
    memproc_info_t* pMemProcInfo;
    size_t          pageCount;
    uintptr_t       startVirt;
    uintptr_t*      pPageDir;
    OS_RETURN_E     error;

    pMemProcInfo = pProcess->pMemoryData;

    /* Aligne memory */
    if((kSize & PAGE_SIZE_MASK) != 0 ||
       kSize < KERNEL_PAGE_SIZE ||
       ((uintptr_t)kVirtualAddress & PAGE_SIZE_MASK) != 0)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    pageCount = kSize / KERNEL_PAGE_SIZE;

    KERNEL_LOCK(pMemProcInfo->lock);

    /* Check if the mapping already exists */
    if(_memoryMgrIsMapped((uintptr_t)kVirtualAddress,
                          pageCount,
                          pMemProcInfo->pageDir,
                          true) == true)
    {
        KERNEL_UNLOCK(pMemProcInfo->lock);
        return OS_ERR_NO_SUCH_ID;
    }

    /* Temporary map the process page directory */
    pPageDir = (uintptr_t*)GET_VIRT_MEM_ADDR(pMemProcInfo->pageDir);

    /* Unmap the data */
    startVirt = (uintptr_t)kVirtualAddress;
    error = _memoryMgrUnmapUser(pPageDir, &startVirt, &pageCount, 4);
    MEM_ASSERT(error == OS_NO_ERR, "Failed to unmap mapped memory", error);

    KERNEL_UNLOCK(pMemProcInfo->lock);

    return error;
}

/************************************ EOF *************************************/