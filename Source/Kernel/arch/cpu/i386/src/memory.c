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
#include <x86cpu.h>        /* X86 CPU API*/
#include <kheap.h>         /* Kernel heap */
#include <panic.h>         /* Kernel PANIC */
#include <string.h>        /* Memory manipulation */
#include <stdint.h>        /* Standard int types */
#include <stddef.h>        /* Standard definitions */
#include <kqueue.h>        /* Kernel queue structure */
#include <kerror.h>        /* Kernel error types */
#include <devtree.h>       /* FDT library */
#include <critical.h>      /* Kernel lock */
#include <core_mgt.h>      /* Core manager */
#include <x86memory.h>     /* I386 memory definitions */
#include <exceptions.h>    /* Exception manager */
#include <kerneloutput.h>  /* Kernel output */
#include <cpu_interrupt.h> /* CPU interrupt settings */

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

/* Tracing feature */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "I386 MEM"

/** @brief Only used at boot: page directory entry used for temporary mapping */
#define KERNEL_PGDIR_TEMP_ENTRY 1

/** @brief Kernel's page directory memory size. */
#define KERNEL_PGDIR_MEM_SIZE 4096
/** @brief Kernel's page directory entry count */
#define KERNEL_PGDIR_ENTRY_COUNT 1024
/** @brief Kernel page address mask. */
#define PG_ENTRY_ADDR_MASK 0xFFFFF000
/** @brief Kernel page directory entry offset. */
#define PG_DIR_ENTRY_OFFSET 22
/** @brief Kernel page table offset. */
#define PG_TABLE_ENTRY_OFFSET 12
/** @brief Kernel page table mask */
#define PG_TABLE_ENTRY_OFFSET_MASK 0x3FF

/** @brief Page directory flag: 4Kb page size. */
#define PAGE_FLAG_PAGE_SIZE_4KB 0x00000000
/** @brief Page directory flag: 4Mb page size. */
#define PAGE_FLAG_PAGE_SIZE_4MB 0x00000080

/** @brief Page flag: global page. */
#define PAGE_FLAG_GLOBAL 0x00000100
/** @brief Page flag: page dirty. */
#define PAGE_FLAG_DIRTY 0x00000080
/** @brief Page flag: page accessed. */
#define PAGE_FLAG_ACCESSED 0x00000020
/** @brief Page flag: cache disabled for the page. */
#define PAGE_FLAG_CACHE_DISABLED 0x00000010
/** @brief Page flag: cache write policy set to write through. */
#define PAGE_FLAG_CACHE_WT 0x00000008
/** @brief Page flag: cache write policy set to write back. */
#define PAGE_FLAG_CACHE_WB 0x00000000
/** @brief Page flag: access permission set to user. */
#define PAGE_FLAG_USER_ACCESS 0x00000004
/** @brief Page flag: access permission set to kernel. */
#define PAGE_FLAG_SUPER_ACCESS 0x00000000
/** @brief Page flag: access permission set to read and write. */
#define PAGE_FLAG_READ_WRITE 0x00000002
/** @brief Page flag: access permission set to read only. */
#define PAGE_FLAG_READ_ONLY 0x00000000
/** @brief Page flag: page present. */
#define PAGE_FLAG_PRESENT 0x00000001
/** @brief Page flag: page present. */
#define PAGE_FLAG_IS_HW 0x00000800

/** @brief Recursive page directory virtual address */
#define KERNEL_RECUR_PG_DIR_BASE 0xFFFFF000
/** @brief Recursive page table virtual address */
#define KERNEL_RECUR_PGTABLE_BASE(PGDIR_ENT) (  \
    0xFFC00000 +                                \
    ((PGDIR_ENT) * 0x1000)                      \
)

/** @brief Page fault error code: page protection violation. */
#define PAGE_FAULT_ERROR_PROT_VIOLATION 0x1
/** @brief Page fault error code: fault on a write. */
#define PAGE_FAULT_ERROR_WRITE 0x2
/** @brief Page fault error code: fault in user mode. */
#define PAGE_FAULT_ERROR_USER 0x4

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines a memory list */
typedef struct
{
    /** @brief The memory list structure  */
    kqueue_t* pQueue;

    /** @brief The memory list lock */
    kernel_spinlock_t lock;
} mem_list_t;

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
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG, TRUE);               \
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
 * @brief Checks the memory type (memory vs hardware) of a physical region.
 *
 * @details Checks the memory type (memory vs hardware) of a physical region.
 * The result are stored in the buffers given as parameter.
 *
 * @param[in] kPhysicalAddress The base address of the region to check.
 * @param[in] kSize The size in bytes of the region to check.
 * @param[out] pIsHardware The buffer to store the hardware result.
 * @param[out] pIsMemory The buffer to store the memory result.
 */
static inline void _checkMemoryType(const uintptr_t kPhysicalAddress,
                                    const uintptr_t kSize,
                                    bool_t*         pIsHardware,
                                    bool_t*         pIsMemory);

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
 * @details Kernel memory page allocation. This method rekeases the kernel pages
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
 * @brief Tells if a memory region is already mapped in the current page tables.
 *
 * @details Tells if a memory region is already mapped in the current page
 * tables. Returns FALSE if the region is not mapped, TRUE otherwise.
 *
 * @param[in] kVirtualAddress The base virtual address to check for mapping.
 * @param[in] pageCount The number of pages to check for mapping.
 *
 * @return Returns FALSE if the region is not mapped, TRUE otherwise.
 */
static bool_t _memoryMgrIsMapped(const uintptr_t kVirtualAddress,
                                 size_t          pageCount);

/**
 * @brief Maps the virtual address to the physical address in the current
 * address space.
 *
 * @details Maps the virtual address to the physical address in the current
 * address space. If the addresses or sizes are not correctly aligned, or if the
 * mapping already exists, an error is retuned. If the address is not in the
 * free memory frames pool and the klags does not mention hardware mapping, an
 * error is returned.
 *
 * @param[in] kVirtualAdderss The virtual address to map.
 * @param[in] kPhysicalAddress The physical address to map.
 * @param[in] kPageCount The number of pages to map.
 * @param[in] kFlags The flags used for mapping.
 *
 * @return The success or error status is returned.
 */
static OS_RETURN_E _memoryMgrMap(const uintptr_t kVirtualAddress,
                                 const uintptr_t kPhysicalAddress,
                                 const size_t    kPageCount,
                                 const uint32_t  kFlags);

/**
 * @brief Unmaps the virtual address in the current  address space.
 *
 * @details Unmaps the virtual address to the physical address in the current
 * address space. If the physical frame is memory and has no more reference,
 * the physical frame is release in the free frames pool.
 *
 * @param[in] kVirtualAdderss The virtual address to unmap.
 * @param[in] kPageCount The number of pages to unmap.
 */
static OS_RETURN_E _memoryMgrUnmap(const uintptr_t kVirtualAddress,
                                   const size_t    kPageCount);

/**
 * @brief Detects the hardware memory presents in the system.
 *
 * @details Detects the hardware memory presents in the system. Each
 * memory range is tagged as memory. This function uses the FDT to
 * register the available memory and reserved memory.
 */
static void _memoryMgrDetectMemory(void);

/**
 * @brief Setups the memory tables used in the kernel.
 *
 * @details Setups the memory tables used in the kernel. The memory map is
 * generated, the pages and frames lists are also created.
 */
static void _memoryMgrInitAddressTable(void);

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
static void _memoryMgrInitPaging(void);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Kernel symbols mapping: Low AP startup address start. */
extern uint8_t _START_LOW_AP_STARTUP_ADDR;
/** @brief Kernel symbols mapping: High AP startup address start. */
extern uint8_t _END_LOW_AP_STARTUP_ADDR;
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

#ifdef _TRACING_ENABLED
/** @brief Kernel symbols mapping: Trace buffer start. */
extern uint8_t _KERNEL_TRACE_BUFFER_BASE;
/** @brief Kernel symbols mapping: Trace buffer size. */
extern uint8_t _KERNEL_TRACE_BUFFER_SIZE;
#endif

#ifdef _TESTING_FRAMEWORK_ENABLED
/** @brief Kernel symbols mapping: Test buffer start. */
extern uint8_t _KERNEL_TEST_BUFFER_BASE;
/** @brief Kernel symbols mapping: Test buffer size. */
extern uint8_t _KERNEL_TEST_BUFFER_SIZE;
#endif

/** @brief Kernel page directory intialized at boot */
extern uintptr_t _kernelPGDir[KERNEL_PGDIR_ENTRY_COUNT];

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Physical memory chunks list */
static mem_list_t sPhysMemList;

/** @brief Kernel free page list list */
static mem_list_t sKernelFreePagesList;

/** @brief Kernel virtual memory bounds */
static mem_range_t sKernelVirtualMemBounds;

/** @brief Kernel physical memory bounds */
static mem_range_t* sKernelPhysicalMemBounds;

/** @brief Kernel physical memory bounds count */
static size_t sKernelPhysicalMemBoundsCount;

/** @brief Kernel page directory virtual pointer */
static uintptr_t* spKernelPageDir = (uintptr_t*)&_kernelPGDir;

/** @brief Memory manager main lock */
static kernel_spinlock_t sLock = KERNEL_SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

#if MEMORY_MGR_DEBUG_ENABLED

static void _printKernelMap(void)
{
    kqueue_node_t* pMemNode;
    mem_range_t*   pMemRange;

    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "=== Kernel memory layout");
    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Startup AP  low 0x%p -> 0x%p | "PRIPTR"KB",
                 &_START_LOW_AP_STARTUP_ADDR,
                 &_END_LOW_AP_STARTUP_ADDR,
                 ((uintptr_t)&_END_LOW_AP_STARTUP_ADDR -
                 (uintptr_t)&_START_LOW_AP_STARTUP_ADDR) >> 10);
    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Startup low     0x%p -> 0x%p | "PRIPTR"KB",
                 &_START_LOW_STARTUP_ADDR,
                 &_END_LOW_STARTUP_ADDR,
                 ((uintptr_t)&_END_LOW_STARTUP_ADDR -
                 (uintptr_t)&_START_LOW_STARTUP_ADDR) >> 10);
    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Code            0x%p -> 0x%p | "PRIPTR"KB",
                 &_START_TEXT_ADDR,
                 &_END_TEXT_ADDR,
                 ((uintptr_t)&_END_TEXT_ADDR -
                 (uintptr_t)&_START_TEXT_ADDR) >> 10);
    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "RO-Data         0x%p -> 0x%p | "PRIPTR"KB",
                 &_START_RO_DATA_ADDR,
                 &_END_RO_DATA_ADDR,
                 ((uintptr_t)&_END_RO_DATA_ADDR -
                 (uintptr_t)&_START_RO_DATA_ADDR) >> 10);
    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "RW-Data         0x%p -> 0x%p | "PRIPTR"KB",
                 &_START_RW_DATA_ADDR,
                 &_END_RW_DATA_ADDR,
                 ((uintptr_t)&_END_RW_DATA_ADDR -
                 (uintptr_t)&_START_RW_DATA_ADDR) >> 10);
    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Stacks          0x%p -> 0x%p | "PRIPTR"KB",
                 &_KERNEL_STACKS_BASE,
                 &_KERNEL_STACKS_BASE + (uintptr_t)&_KERNEL_STACKS_SIZE,
                 ((uintptr_t)&_KERNEL_STACKS_SIZE) >> 10);
    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Heap            0x%p -> 0x%p | "PRIPTR"KB",
                 &_KERNEL_HEAP_BASE,
                 &_KERNEL_HEAP_BASE + (uintptr_t)&_KERNEL_HEAP_SIZE,
                 ((uintptr_t)&_KERNEL_HEAP_SIZE) >> 10);

    pMemNode = sPhysMemList.pQueue->pHead;
    while(pMemNode != NULL)
    {
        pMemRange = (mem_range_t*)pMemNode->pData;
        KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Free physical memory regions 0x%p -> 0x%p",
                     pMemRange->base,
                     pMemRange->limit);

        pMemNode = pMemNode->pNext;
    }

    pMemNode = sKernelFreePagesList.pQueue->pHead;
    while(pMemNode != NULL)
    {
        pMemRange = (mem_range_t*)pMemNode->pData;
        KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Free kernel virtual memory regions 0x%p -> 0x%p",
                     pMemRange->base,
                     pMemRange->limit);

        pMemNode = pMemNode->pNext;
    }
}

#endif /* #if MEMORY_MGR_DEBUG_ENABLED */

static void _pageFaultHandler(kernel_thread_t* pCurrentThread)
{
    uintptr_t faultAddress;
    uintptr_t physAddr;
    uint32_t  errorCode;
    uint32_t  flags;
    bool_t    staleEntry;



    /* Get the fault address and error code */
    __asm__ __volatile__ ("mov %%cr2, %0" : "=r"(faultAddress));
    errorCode = ((virtual_cpu_t*)pCurrentThread->pVCpu)->intContext.errorCode;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_PAGE_FAULT_ENTRY,
                       3,
                       KERNEL_TRACE_HIGH(faultAddress),
                       KERNEL_TRACE_LOW(faultAddress),
                       pCurrentThread->tid);

    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Page fault: 0x%p | Code: %x\n",
                 faultAddress,
                 errorCode);

    /* Check if the fault occured because we hit a stale TLB entry */
    physAddr = memoryMgrGetPhysAddr(faultAddress, &flags);
    if(physAddr != MEMMGR_PHYS_ADDR_ERROR)
    {
        staleEntry = TRUE;
        if((errorCode & PAGE_FAULT_ERROR_PROT_VIOLATION) ==
           PAGE_FAULT_ERROR_PROT_VIOLATION)
        {
            /* Check the privilege level */
            if((errorCode & PAGE_FAULT_ERROR_USER) == PAGE_FAULT_ERROR_USER &&
               (flags & MEMMGR_MAP_USER) != MEMMGR_MAP_USER)
            {
                staleEntry = FALSE;
            }

            /* Check the access rights */
            if((errorCode & PAGE_FAULT_ERROR_WRITE) == PAGE_FAULT_ERROR_WRITE &&
               (flags & MEMMGR_MAP_RW) != MEMMGR_MAP_RW)
            {
                staleEntry = FALSE;
            }

        }
        else if(errorCode != 0)
        {
            staleEntry = FALSE;
        }

        if(staleEntry == TRUE)
        {
            KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                         MODULE_NAME,
                         "Stale entry fault: 0x%p | Code: %x\n",
                         faultAddress,
                         errorCode);
            cpuInvalidateTlbEntry(faultAddress);
            return;
        }
    }

    /* TODO: Terminate thread, set reason page fault and reason data the address,
     * also getthe reason code in the interrupt info */
    kernelPanicHandler(pCurrentThread);

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_PAGE_FAULT_EXIT,
                       3,
                       KERNEL_TRACE_HIGH(faultAddress),
                       KERNEL_TRACE_LOW(faultAddress),
                       pCurrentThread->tid);
}

static inline void _checkMemoryType(const uintptr_t kPhysicalAddress,
                                    const uintptr_t kSize,
                                    bool_t*         pIsHardware,
                                    bool_t*         pIsMemory)
{
    uintptr_t limit;
    size_t    bytesOutMem;
    size_t    i;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_CHECK_MEM_TYPE_ENTRY,
                       4,
                       0,
                       (uint32_t)kPhysicalAddress,
                       0,
                       (uint32_t)kSize);

    *pIsHardware = FALSE;
    *pIsMemory   = FALSE;

    limit       = kPhysicalAddress + kSize;
    bytesOutMem = kSize;

    /* Check for overflow */
    if(limit == 0)
    {
        limit = limit - 1;
    }
    else if(limit < kPhysicalAddress)
    {
        *pIsMemory  = TRUE;
        *pIsHardware = TRUE;

        KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                           TRACE_X86_MEMMGR_CHECK_MEM_TYPE_EXIT,
                           6,
                           0,
                           (uint32_t)kPhysicalAddress,
                           0,
                           (uint32_t)kSize,
                           *pIsHardware,
                           *pIsMemory);
        return;
    }

    for(i = 0; i < sKernelPhysicalMemBoundsCount; ++i)
    {
        /* If overlapping low */
        if(kPhysicalAddress <= sKernelPhysicalMemBounds[i].base &&
           limit > sKernelPhysicalMemBounds[i].base)
        {
            bytesOutMem -= MIN(limit, sKernelPhysicalMemBounds[i].limit) -
                               sKernelPhysicalMemBounds[i].base;
        }
        else if(kPhysicalAddress > sKernelPhysicalMemBounds[i].base &&
                kPhysicalAddress < sKernelPhysicalMemBounds[i].limit)
        {
            bytesOutMem -= MIN(limit, sKernelPhysicalMemBounds[i].limit) -
                           kPhysicalAddress;
        }
    }

    /* If reduced the base / limit, it's in part memory */
    *pIsMemory = (bytesOutMem != kSize);

    /* If we did not completely consume the range, we have hardware */
    *pIsHardware = (bytesOutMem != 0);

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_CHECK_MEM_TYPE_EXIT,
                       6,
                       0,
                       (uint32_t)kPhysicalAddress,
                       0,
                       (uint32_t)kSize,
                       *pIsHardware,
                       *pIsMemory);
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
    bool_t         merged;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_ADD_BLOCK_ENTRY,
                       4,
                       0,
                       (uint32_t)baseAddress,
                       0,
                       (uint32_t)kLength);

    limit = baseAddress + kLength;

    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Adding memory block 0x%p -> 0x%p",
                 baseAddress,
                 limit);

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

    KERNEL_CRITICAL_LOCK(pList->lock);

    /* Try to merge the new block, the list is ordered by base address asc */
    pCursor = pList->pQueue->pHead;
    merged = FALSE;
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
            KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                         MODULE_NAME,
                         "Merging with block 0x%p -> 0x%p",
                         pRange->base,
                         pRange->limit);

            /* Extend left */
            pRange->base  = baseAddress;
            pCursor->priority = KERNEL_VIRTUAL_ADDR_MAX - baseAddress;
            merged = TRUE;
        }
        /* If the new block is after but needs merging */
        else if(baseAddress == pRange->limit)
        {
            KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                         MODULE_NAME,
                         "Merging with block 0x%p -> 0x%p",
                         pRange->base,
                         pRange->limit);

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
                    merged = TRUE;


                    /* Remove node */
                    kfree(pSaveCursor->pData);
                    kQueueRemove(pList->pQueue, pSaveCursor, TRUE);
                    kQueueDestroyNode(&pSaveCursor);
                }
                else if(((mem_range_t*)pCursor->pNext->pData)->base < limit)
                {
                    MEM_ASSERT(FALSE,
                               "Adding an already free block",
                               OS_ERR_UNAUTHORIZED_ACTION);
                }
            }

            if(merged == FALSE)
            {
                /* Extend up */
                pRange->limit = limit;
                merged = TRUE;
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
    if(merged == FALSE)
    {
        pRange = kmalloc(sizeof(mem_range_t));
        MEM_ASSERT(pRange != NULL,
                   "Failed to allocate new memory range",
                   OS_ERR_NO_MORE_MEMORY);

        pNewNode = kQueueCreateNode(pRange, TRUE);

        pRange->base  = baseAddress;
        pRange->limit = limit;

        kQueuePushPrio(pNewNode,
                       pList->pQueue,
                       KERNEL_VIRTUAL_ADDR_MAX - baseAddress);

        KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Added new block 0x%p -> 0x%p",
                     baseAddress,
                     limit);
    }

    KERNEL_CRITICAL_UNLOCK(pList->lock);

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_ADD_BLOCK_EXIT,
                       4,
                       0,
                       (uint32_t)baseAddress,
                       0,
                       (uint32_t)kLength);
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

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_REMOVE_BLOCK_ENTRY,
                       4,
                       0,
                       (uint32_t)baseAddress,
                       0,
                       (uint32_t)kLength);

    MEM_ASSERT(pList != NULL,
               "Tried to remove a memory block from a NULL list",
               OS_ERR_NULL_POINTER);

    MEM_ASSERT((baseAddress & PAGE_SIZE_MASK) == 0 &&
               (kLength & PAGE_SIZE_MASK) == 0,
               "Tried to remove a non aligned block",
               OS_ERR_UNAUTHORIZED_ACTION);

    limit = baseAddress + kLength;

    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Removing memory block 0x%p -> 0x%p",
                 baseAddress,
                 limit);

    KERNEL_CRITICAL_LOCK(pList->lock);

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

            KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                         MODULE_NAME,
                         "Removing block 0x%p -> 0x%p",
                         pRange->base,
                         pRange->limit);

            /* Update the rest to remove */
            baseAddress = pRange->limit;
            if(limit == pRange->limit)
            {
                limit = 0;
            }

            kfree(pSaveCursor->pData);
            kQueueRemove(pList->pQueue, pSaveCursor, TRUE);
            kQueueDestroyNode(&pSaveCursor);
        }
        /* If up containted */
        else if(pRange->base < baseAddress && pRange->limit <= limit)
        {

            KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                         MODULE_NAME,
                         "Reducing up block 0x%p -> 0x%p to 0x%p -> 0x%p",
                         pRange->base,
                         pRange->limit,
                         pRange->base,
                         baseAddress);

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
            KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                         MODULE_NAME,
                         "Reducing down block 0x%p -> 0x%p to 0x%p -> 0x%p",
                         pRange->base,
                         pRange->limit,
                         limit,
                         pRange->limit);

            /* Update the rest to remove */
            pRange->base = limit;

            /* We are done*/
            limit = 0;
        }
        /* If inside */
        else if(pRange->base < baseAddress && pRange->limit > limit)
        {
            KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                         MODULE_NAME,
                         "Splitting block 0x%p -> 0x%p",
                         pRange->base,
                         pRange->limit);

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

            pNewNode = kQueueCreateNode(pRange, TRUE);

            pRange->base  = baseAddress;
            pRange->limit = saveLimit;

            kQueuePushPrio(pNewNode,
                           pList->pQueue,
                           KERNEL_VIRTUAL_ADDR_MAX - baseAddress);

            KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                        MODULE_NAME,
                        "Added new block form removal 0x%p -> 0x%p",
                        baseAddress,
                        limit);

            /* We are done*/
            limit = 0;
        }
        else
        {
            pCursor = pCursor->pNext;
        }
    }

    KERNEL_CRITICAL_UNLOCK(pList->lock);

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_REMOVE_BLOCK_EXIT,
                       4,
                       0,
                       (uint32_t)baseAddress,
                       0,
                       (uint32_t)kLength);
}

static uintptr_t _getBlock(mem_list_t* pList, const size_t kLength)
{
    MEM_ASSERT((kLength & PAGE_SIZE_MASK) == 0,
               "Tried to get a non aligned block",
               OS_ERR_UNAUTHORIZED_ACTION);

    uintptr_t      retBlock;
    kqueue_node_t* pCursor;
    mem_range_t*   pRange;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_GET_BLOCK_ENTRY,
                       2,
                       0,
                       (uint32_t)kLength);

    retBlock = 0;

    KERNEL_CRITICAL_LOCK(pList->lock);

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
                KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                             MODULE_NAME,
                             "Removing block after alloc 0x%p -> 0x%p",
                             pRange->base,
                             pRange->limit);

                kfree(pCursor->pData);
                kQueueRemove(pList->pQueue, pCursor, TRUE);
                kQueueDestroyNode(&pCursor);
            }
            else
            {
                KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                             MODULE_NAME,
                             "Reducing block after alloc 0x%p -> 0x%p to "
                             "0x%p -> 0x%p",
                             pRange->base,
                             pRange->limit,
                             pRange->base + kLength,
                             pRange->limit);
                pRange->base += kLength;
                pCursor->priority = KERNEL_VIRTUAL_ADDR_MAX - pRange->base;
            }
            break;
        }

        pCursor = pCursor->pNext;
    }

    KERNEL_CRITICAL_UNLOCK(pList->lock);

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_GET_BLOCK_EXIT,
                       2,
                       0,
                       (uint32_t)kLength);

    return retBlock;
}

static uintptr_t _allocateFrames(const size_t kFrameCount)
{
    return _getBlock(&sPhysMemList, KERNEL_PAGE_SIZE * kFrameCount);
}

static void _releaseFrames(const uintptr_t kBaseAddress,
                           const size_t    kFrameCount)
{
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

static bool_t _memoryMgrIsMapped(const uintptr_t kVirtualAddress,
                                 size_t          pageCount)
{
    bool_t     isMapped;
    uint16_t   pgDirEntry;
    uint16_t   pgTableEntry;
    uint16_t   offset;
    uintptr_t  currVirtAddr;
    uintptr_t* pPgDirRecurEntry;
    uintptr_t* pPgTableRecurEntry;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_IS_MAPPED_ENTRY,
                       4,
                       0,
                       (uint32_t)kVirtualAddress,
                       0,
                       (uint32_t)pageCount);

    MEM_ASSERT((kVirtualAddress & PAGE_SIZE_MASK) == 0,
               "Checking mapping for non aligned address",
               OS_ERR_INCORRECT_VALUE);

    isMapped = FALSE;
    pPgDirRecurEntry = (uintptr_t*)KERNEL_RECUR_PG_DIR_BASE;
    currVirtAddr = kVirtualAddress;

    KERNEL_CRITICAL_LOCK(sLock);
    while(pageCount != 0)
    {
        /* Get entry indexes */
        pgDirEntry   = currVirtAddr >> PG_DIR_ENTRY_OFFSET;
        pgTableEntry = (currVirtAddr >> PG_TABLE_ENTRY_OFFSET) &
                       PG_TABLE_ENTRY_OFFSET_MASK;

        /* If the entry is empty */
        if((pPgDirRecurEntry[pgDirEntry] & PAGE_FLAG_PRESENT) == 0)
        {
            offset = KERNEL_PGDIR_ENTRY_COUNT - pgTableEntry;

            /* We will never have anything pagegd in this directory */
            pageCount -= MIN(pageCount, offset);
            currVirtAddr += offset * KERNEL_PAGE_SIZE;
        }
        else
        {
            /* Get page table recursive entry */
            pPgTableRecurEntry = (uintptr_t*)
                                 KERNEL_RECUR_PGTABLE_BASE(pgDirEntry);
            /* Check entries in the page table */
            while(pgTableEntry < KERNEL_PGDIR_ENTRY_COUNT &&
                  pageCount != 0)
            {
                if((pPgTableRecurEntry[pgTableEntry] & PAGE_FLAG_PRESENT) != 0)
                {
                    pageCount = 0;
                    isMapped = TRUE;
                    break;
                }
                else
                {
                    currVirtAddr += KERNEL_PAGE_SIZE;
                    ++pgTableEntry;
                    --pageCount;
                }
            }
        }
    }
    KERNEL_CRITICAL_UNLOCK(sLock);

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_IS_MAPPED_EXIT,
                       4,
                       0,
                       (uint32_t)kVirtualAddress,
                       0,
                       (uint32_t)pageCount);

    return isMapped;
}

static OS_RETURN_E _memoryMgrMap(const uintptr_t kVirtualAddress,
                                 const uintptr_t kPhysicalAddress,
                                 const size_t    kPageCount,
                                 const uint32_t  kFlags)
{
    size_t     toMap;
    uint32_t   mapFlags;
    uint32_t   mapPgdirFlags;
    bool_t     isMapped;
    bool_t     isHardware;
    bool_t     isMemory;
    uint16_t   pgDirEntry;
    uint16_t   pgTableEntry;
    uintptr_t  currVirtAddr;
    uintptr_t  currPhysAdd;
    uintptr_t* pPgDirRecurEntry;
    uintptr_t* pPgTableRecurEntry;
    uintptr_t  newPgTableFrame;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_MAP_ENTRY,
                       7,
                       0,
                       (uint32_t)kVirtualAddress,
                       0,
                       (uint32_t)kPhysicalAddress,
                       0,
                       (uint32_t)kPageCount,
                       kFlags);

    /* Check the alignements */
    if((kVirtualAddress & PAGE_SIZE_MASK) != 0 ||
       (kPhysicalAddress & PAGE_SIZE_MASK) != 0 ||
       kPageCount == 0)
    {
        KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                           TRACE_X86_MEMMGR_MAP_EXIT,
                           8,
                           0,
                           (uint32_t)kVirtualAddress,
                           0,
                           (uint32_t)kPhysicalAddress,
                           0,
                           (uint32_t)kPageCount,
                           kFlags,
                           OS_ERR_UNAUTHORIZED_ACTION);
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Check the memory type */
    _checkMemoryType(kPhysicalAddress,
                     kPageCount * KERNEL_PAGE_SIZE,
                     &isHardware,
                     &isMemory);

    /* If is hardware, check if flags are valid */
    if((isHardware == TRUE && isMemory == TRUE) ||
       (isHardware ==TRUE &&
        (kFlags & MEMMGR_MAP_HARDWARE) != MEMMGR_MAP_HARDWARE))
    {
        KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                           TRACE_X86_MEMMGR_MAP_EXIT,
                           8,
                           0,
                           (uint32_t)kVirtualAddress,
                           0,
                           (uint32_t)kPhysicalAddress,
                           0,
                           (uint32_t)kPageCount,
                           kFlags,
                           OS_ERR_UNAUTHORIZED_ACTION);
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Mapping 0x%p to 0x%p -> 0x%p, HW (%d) MEM(%d)",
                 kPhysicalAddress,
                 kPhysicalAddress + kPageCount * KERNEL_PAGE_SIZE,
                 kVirtualAddress,
                 isHardware,
                 isMemory);

    /* Check if the mapping already exists, check if we need to update one or
     * more page directory entries
     */
    isMapped = _memoryMgrIsMapped(kVirtualAddress, kPageCount);
    if(isMapped == TRUE)
    {
        KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                           TRACE_X86_MEMMGR_MAP_EXIT,
                           8,
                           0,
                           (uint32_t)kVirtualAddress,
                           0,
                           (uint32_t)kPhysicalAddress,
                           0,
                           (uint32_t)kPageCount,
                           kFlags,
                           OS_ERR_MAPPING_ALREADY_EXISTS);
        return OS_ERR_MAPPING_ALREADY_EXISTS;
    }

    /* Get the flags */
    mapFlags = PAGE_FLAG_PRESENT;
    if((kFlags & MEMMGR_MAP_KERNEL) == MEMMGR_MAP_KERNEL)
    {
        mapFlags |= PAGE_FLAG_SUPER_ACCESS;
    }
    else
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
    if((kFlags & MEMMGR_MAP_HARDWARE) == MEMMGR_MAP_HARDWARE)
    {
        mapFlags |= PAGE_FLAG_CACHE_DISABLED | PAGE_FLAG_IS_HW;
    }

    mapPgdirFlags = PAGE_FLAG_PAGE_SIZE_4KB |
                    PAGE_FLAG_SUPER_ACCESS  |
                    PAGE_FLAG_USER_ACCESS   |
                    PAGE_FLAG_READ_WRITE    |
                    PAGE_FLAG_CACHE_WB      |
                    PAGE_FLAG_PRESENT;

    /* Apply the mapping */
    toMap = kPageCount;
    currVirtAddr = kVirtualAddress;
    currPhysAdd  = kPhysicalAddress;
    pPgDirRecurEntry = (uintptr_t*)KERNEL_RECUR_PG_DIR_BASE;

    KERNEL_CRITICAL_LOCK(sLock);
    while(toMap != 0)
    {
        /* Get entry indexes */
        pgDirEntry = currVirtAddr >> PG_DIR_ENTRY_OFFSET;

        /* If the entry is empty */
        if((pPgDirRecurEntry[pgDirEntry] & PAGE_FLAG_PRESENT) == 0)
        {
            /* Allocate a new frame */
            newPgTableFrame = _allocateFrames(1);
            MEM_ASSERT(newPgTableFrame != 0,
                       "Allocated a NULL frame",
                       OS_ERR_NULL_POINTER);

            pPgDirRecurEntry[pgDirEntry] = newPgTableFrame | mapPgdirFlags;

            /* Get page table recursive entry */
            pPgTableRecurEntry = (uintptr_t*)
                                 KERNEL_RECUR_PGTABLE_BASE(pgDirEntry);
            cpuInvalidateTlbEntry((uintptr_t)pPgTableRecurEntry);
            /* Zeroize the frame */
            memset(pPgTableRecurEntry, 0, KERNEL_PAGE_SIZE);
        }
        else
        {
            /* Get page table recursive entry */
            pPgTableRecurEntry = (uintptr_t*)
                                 KERNEL_RECUR_PGTABLE_BASE(pgDirEntry);
        }

        /* Map as much as we can in this page table */
        pgTableEntry = (currVirtAddr >> PG_TABLE_ENTRY_OFFSET) &
                        PG_TABLE_ENTRY_OFFSET_MASK;
        do
        {
            /* Set mapping and invalidate */
            pPgTableRecurEntry[pgTableEntry] = currPhysAdd | mapFlags;
            cpuInvalidateTlbEntry(currVirtAddr);

            currVirtAddr += KERNEL_PAGE_SIZE;
            currPhysAdd  += KERNEL_PAGE_SIZE;
            --toMap;
            ++pgTableEntry;
        } while(toMap > 0 && pgTableEntry != KERNEL_PGDIR_ENTRY_COUNT - 1);
    }
    KERNEL_CRITICAL_UNLOCK(sLock);

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_MAP_EXIT,
                       8,
                       0,
                       (uint32_t)kVirtualAddress,
                       0,
                       (uint32_t)kPhysicalAddress,
                       0,
                       (uint32_t)kPageCount,
                       kFlags,
                       OS_NO_ERR);

    return OS_NO_ERR;
}

static OS_RETURN_E _memoryMgrUnmap(const uintptr_t kVirtualAddress,
                                   const size_t    kPageCount)
{
    size_t       toUnmap;
    bool_t       hasMapping;
    uint16_t     pgDirEntry;
    uint16_t     pgTableEntry;
    uint16_t     offset;
    uint16_t     i;
    uintptr_t    currVirtAddr;
    uintptr_t*   pPgDirRecurEntry;
    uintptr_t*   pPgTableRecurEntry;
    ipi_params_t ipiParams;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_UNMAP_ENTRY,
                       4,
                       0,
                       (uint32_t)kVirtualAddress,
                       0,
                       (uint32_t)kPageCount);

    /* Check the alignements */
    if((kVirtualAddress & PAGE_SIZE_MASK) != 0 ||
       kPageCount == 0)
    {
        KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                           TRACE_X86_MEMMGR_UNMAP_EXIT,
                           5,
                           0,
                           (uint32_t)kVirtualAddress,
                           0,
                           (uint32_t)kPageCount,
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Apply the mapping */
    toUnmap = kPageCount;
    currVirtAddr = kVirtualAddress;
    pPgDirRecurEntry = (uintptr_t*)KERNEL_RECUR_PG_DIR_BASE;

    ipiParams.function = IPI_FUNC_TLB_INVAL;

    KERNEL_CRITICAL_LOCK(sLock);
    while(toUnmap != 0)
    {
        /* Get entry indexes */
        pgDirEntry   = currVirtAddr >> PG_DIR_ENTRY_OFFSET;

        /* If the entry is empty */
        if((pPgDirRecurEntry[pgDirEntry] & PAGE_FLAG_PRESENT) != 0)
        {
            /* Get page table recursive entry */
            pPgTableRecurEntry = (uintptr_t*)
                                 KERNEL_RECUR_PGTABLE_BASE(pgDirEntry);

            /* Unmap as much as we can in this page table */
            pgTableEntry = (currVirtAddr >> PG_TABLE_ENTRY_OFFSET) &
                            PG_TABLE_ENTRY_OFFSET_MASK;
            do
            {
                /* If present, unmap */
                if((pPgTableRecurEntry[pgTableEntry] & PAGE_FLAG_PRESENT) != 0)
                {
                    /* Set mapping and invalidate */
                    pPgTableRecurEntry[pgTableEntry] = 0;
                    cpuInvalidateTlbEntry(currVirtAddr);

                    /* Update other cores TLB */
                    ipiParams.pData = (void*)currVirtAddr;
                    coreMgtSendIpi(CORE_MGT_IPI_BROADCAST_TO_OTHER, &ipiParams);
                }

                currVirtAddr += KERNEL_PAGE_SIZE;
                --toUnmap;
                ++pgTableEntry;
            } while(toUnmap > 0 &&
                    pgTableEntry != KERNEL_PGDIR_ENTRY_COUNT - 1);

            /* Check if we can clean this directory entries */
            offset = pgTableEntry;
            pgTableEntry = (kVirtualAddress >> PG_TABLE_ENTRY_OFFSET) &
                            PG_TABLE_ENTRY_OFFSET_MASK;
            hasMapping = FALSE;

            /* Check before entry */
            for(i = 0; i < pgTableEntry; ++i)
            {
                if((pPgTableRecurEntry[i] & PAGE_FLAG_PRESENT) != 0)
                {
                    hasMapping = TRUE;
                    break;
                }
            }

            /* Check after entry */
            for(i = offset;
                hasMapping == FALSE && i < KERNEL_PGDIR_ENTRY_COUNT;
                ++i)
            {
                if((pPgTableRecurEntry[i] & PAGE_FLAG_PRESENT) != 0)
                {
                    hasMapping = TRUE;
                    break;
                }
            }

            if(hasMapping == FALSE)
            {
                /* Release frame and set as non present */
                _releaseFrames(pPgDirRecurEntry[pgDirEntry] &
                               PG_ENTRY_ADDR_MASK,
                               1);
                pPgDirRecurEntry[pgDirEntry] = 0;
                cpuInvalidateTlbEntry((uintptr_t)pPgTableRecurEntry);

                /* Update other cores TLB */
                ipiParams.pData = (void*)pPgTableRecurEntry;
                coreMgtSendIpi(CORE_MGT_IPI_BROADCAST_TO_OTHER, &ipiParams);
            }
        }
        else
        {
            pgTableEntry = (currVirtAddr >> PG_TABLE_ENTRY_OFFSET) &
                            PG_TABLE_ENTRY_OFFSET_MASK;

            /* Just skip the rest of the pages in this directory */
            offset = KERNEL_PGDIR_ENTRY_COUNT - pgTableEntry;
            if(toUnmap <= offset)
            {
                toUnmap = 0;
            }
            else
            {
                toUnmap -= offset;
                currVirtAddr += KERNEL_PAGE_SIZE * offset;
            }
        }
    }
    KERNEL_CRITICAL_UNLOCK(sLock);

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_UNMAP_EXIT,
                       5,
                       0,
                       (uint32_t)kVirtualAddress,
                       0,
                       (uint32_t)kPageCount,
                       OS_NO_ERR);

    return OS_NO_ERR;
}

static void _memoryMgrDetectMemory(void)
{
    uintptr_t      baseAddress;
    size_t         size;
    uintptr_t      kernelPhysStart;
    uintptr_t      kernelPhysEnd;
    kqueue_node_t* pMemNode;

    const fdt_mem_node_t* kpPhysMemNode;
    const fdt_mem_node_t* kpResMemNode;
    const fdt_mem_node_t* kpCursor;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_DETECT_MEM_ENTRY,
                       0);

    kpPhysMemNode = fdtGetMemory();
    MEM_ASSERT(kpPhysMemNode != NULL,
               "No physical memory detected in FDT",
               OS_ERR_NO_MORE_MEMORY);

    /* Now iterate on all memory nodes and add the regions */
    while(kpPhysMemNode != NULL)
    {
        /* Align the base address and size */
        baseAddress = ALIGN_UP(FDTTOCPU32(kpPhysMemNode->baseAddress),
                               KERNEL_PAGE_SIZE);
        size        = baseAddress - FDTTOCPU32(kpPhysMemNode->baseAddress);
        size        = ALIGN_DOWN(FDTTOCPU32(kpPhysMemNode->size) - size,
                                 KERNEL_PAGE_SIZE);

        KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Adding region 0x%p -> 0x%p | Aligned: 0x%p -> 0x%p",
                     FDTTOCPU32(kpPhysMemNode->baseAddress),
                     FDTTOCPU32(kpPhysMemNode->baseAddress) +
                     FDTTOCPU32(kpPhysMemNode->size),
                     baseAddress,
                     baseAddress + size);

        _addBlock(&sPhysMemList, baseAddress, size);

        kpPhysMemNode = kpPhysMemNode->pNextNode;
    }

    /* Remove reserved memory */
    kpResMemNode = fdtGetReservedMemory();
    kpCursor = kpResMemNode;
    while(kpCursor != NULL)
    {
        baseAddress = ALIGN_DOWN(FDTTOCPU32(kpCursor->baseAddress),
                                 KERNEL_PAGE_SIZE);
        size = ALIGN_UP(FDTTOCPU32(kpCursor->size), KERNEL_PAGE_SIZE);

        KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Removing reserved region 0x%p -> 0x%p",
                     baseAddress,
                     baseAddress + size);

        _removeBlock(&sPhysMemList, baseAddress, size);

        kpCursor = kpCursor->pNextNode;
    }

    /* Get kernel bounds */
    kernelPhysStart = (uintptr_t)&_KERNEL_MEMORY_START;
    kernelPhysEnd   = (uintptr_t)&_KERNEL_MEMORY_END;

#ifdef _TRACING_ENABLED
    /* If tracing is enabled, the end is after its buffer */
    kernelPhysEnd = (uintptr_t)&_KERNEL_TRACE_BUFFER_BASE +
                    (uintptr_t)&_KERNEL_TRACE_BUFFER_SIZE;
#endif

#ifdef _TESTING_FRAMEWORK_ENABLED
    /* If testing is enabled, the end is after its buffer */
    kernelPhysEnd = (uintptr_t)&_KERNEL_TEST_BUFFER_BASE +
                    (uintptr_t)&_KERNEL_TEST_BUFFER_SIZE;
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

    /* Now create the physical memory bounds array */
    sKernelPhysicalMemBoundsCount = 0;
    pMemNode = sPhysMemList.pQueue->pHead;
    while(pMemNode != NULL)
    {
        ++sKernelPhysicalMemBoundsCount;
        pMemNode = pMemNode->pNext;
    }
    sKernelPhysicalMemBounds = kmalloc(sizeof(mem_range_t) *
                                       sKernelPhysicalMemBoundsCount);
    pMemNode = sPhysMemList.pQueue->pHead;
    size = 0;
    while(pMemNode != NULL)
    {
        sKernelPhysicalMemBounds[size].base  =
            ((mem_range_t*)pMemNode->pData)->base;
        sKernelPhysicalMemBounds[size].limit =
            ((mem_range_t*)pMemNode->pData)->limit;
        ++size;
        pMemNode = pMemNode->pNext;
    }

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_DETECT_MEM_EXIT,
                       0);
}

static void _memoryMgrInitAddressTable(void)
{
    uintptr_t kernelVirtEnd;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_INIT_ADDRTABLE_ENTRY,
                       0);

    /* Initialize kernel pages */
    kernelVirtEnd   = (uintptr_t)&_KERNEL_MEMORY_END;

#ifdef _TRACING_ENABLED
    /* If tracing is enabled, the end is after its buffer */
    kernelVirtEnd = (uintptr_t)&_KERNEL_TRACE_BUFFER_BASE +
                    (uintptr_t)&_KERNEL_TRACE_BUFFER_SIZE;
#endif

#ifdef _TESTING_FRAMEWORK_ENABLED
    /* If testing is enabled, the end is after its buffer */
    kernelVirtEnd = (uintptr_t)&_KERNEL_TEST_BUFFER_BASE +
                    (uintptr_t)&_KERNEL_TEST_BUFFER_SIZE;
#endif

    /* Get actual physical address */
    kernelVirtEnd = ALIGN_UP(kernelVirtEnd, KERNEL_PAGE_SIZE);

    sKernelVirtualMemBounds.base  = kernelVirtEnd;
    sKernelVirtualMemBounds.limit = KERNEL_VIRTUAL_ADDR_MAX;

    /* Add free pages */
    _addBlock(&sKernelFreePagesList,
              kernelVirtEnd,
              KERNEL_VIRTUAL_ADDR_MAX - kernelVirtEnd + 1);

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_INIT_ADDRTABLE_EXIT,
                       0);
}

static void _memoryMgrMapKernelRegion(uintptr_t*      pLastSectionStart,
                                      uintptr_t*      pLastSectionEnd,
                                      const uintptr_t kRegionStartAddr,
                                      const uintptr_t kRegionEndAddr,
                                      const uint32_t  kFlags)
{
    uintptr_t* pRecurTableEntry;
    uintptr_t* pPgDirRecurEntry;
    uintptr_t  tmpPageTablePhysAddr;
    uintptr_t  kernelSectionStart;
    uintptr_t  kernelSectionEnd;
    uint16_t   pgDirEntry;
    uint16_t   pgTableEntry;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_MAP_KERNEL_ENTRY,
                       5,
                       0,
                       (uint32_t)kRegionStartAddr,
                       0,
                       (uint32_t)kRegionEndAddr,
                       kFlags);

    /* Align and check */
    kernelSectionStart = ALIGN_DOWN(kRegionStartAddr, KERNEL_PAGE_SIZE);
    kernelSectionEnd   = ALIGN_UP(kRegionEndAddr, KERNEL_PAGE_SIZE);

    MEM_ASSERT(*pLastSectionEnd <= kernelSectionStart,
               "Overlapping kernel memory sections",
               OS_ERR_NO_MORE_MEMORY);

    *pLastSectionStart = kernelSectionStart;
    *pLastSectionEnd   = kernelSectionEnd;

    /* Map per 4K pages */
    pPgDirRecurEntry = (uintptr_t*)KERNEL_RECUR_PG_DIR_BASE;
    while(kernelSectionStart < kernelSectionEnd)
    {
        pgTableEntry = (kernelSectionStart >> PG_TABLE_ENTRY_OFFSET) &
                        PG_TABLE_ENTRY_OFFSET_MASK;
        /* Get entry indexes */
        if(kernelSectionStart < KERNEL_MEM_OFFSET)
        {
            pgDirEntry = kernelSectionStart >> PG_DIR_ENTRY_OFFSET;
        }
        else
        {
            pgDirEntry = KERNEL_PGDIR_TEMP_ENTRY;
        }

        /* If the entry is empty */
        if((pPgDirRecurEntry[pgDirEntry] & PAGE_FLAG_PRESENT) == 0)
        {
            /* Allocate a new frame and map to temporary boot entry */
            tmpPageTablePhysAddr = _allocateFrames(1);
            MEM_ASSERT(tmpPageTablePhysAddr != 0,
                       "Allocated a NULL frame",
                       OS_ERR_NULL_POINTER);

            pPgDirRecurEntry[pgDirEntry] = tmpPageTablePhysAddr    |
                                           PAGE_FLAG_PAGE_SIZE_4KB |
                                           PAGE_FLAG_SUPER_ACCESS  |
                                           PAGE_FLAG_READ_WRITE    |
                                           PAGE_FLAG_CACHE_WB      |
                                           PAGE_FLAG_PRESENT;

            pRecurTableEntry = (uintptr_t*)
                               KERNEL_RECUR_PGTABLE_BASE(pgDirEntry);
            /* Zeroize the frame */
            memset(pRecurTableEntry, 0, KERNEL_PAGE_SIZE);
        }
        else
        {
            pRecurTableEntry = (uintptr_t*)
                               KERNEL_RECUR_PGTABLE_BASE(pgDirEntry);
        }

        if(kernelSectionStart >= KERNEL_MEM_OFFSET)
        {
            pRecurTableEntry[pgTableEntry] = (kernelSectionStart -
                                              KERNEL_MEM_OFFSET) |
                                             PAGE_FLAG_SUPER_ACCESS |
                                             PAGE_FLAG_CACHE_WB     |
                                             PAGE_FLAG_PRESENT;
        }
        else
        {
            pRecurTableEntry[pgTableEntry] = kernelSectionStart     |
                                             PAGE_FLAG_SUPER_ACCESS |
                                             PAGE_FLAG_CACHE_WB     |
                                             PAGE_FLAG_PRESENT;
        }

        /* By default we are read-only, check if write is required */
        if((kFlags & MEMMGR_MAP_RW) == MEMMGR_MAP_RW)
        {
            pRecurTableEntry[pgTableEntry] |= PAGE_FLAG_READ_WRITE;
        }

        kernelSectionStart += KERNEL_PAGE_SIZE;
    }

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_MAP_KERNEL_EXIT,
                       5,
                       0,
                       (uint32_t)kRegionStartAddr,
                       0,
                       (uint32_t)kRegionEndAddr,
                       kFlags);
}

static void _memoryMgrInitPaging(void)
{
    uintptr_t  kernelSectionStart;
    uintptr_t  kernelSectionEnd;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_INIT_MAPPING_ENTRY,
                       0);

    kernelSectionStart = 0;
    kernelSectionEnd   = 0;

    /* Clear the low entries used during boot */
    spKernelPageDir[0] = 0;

    /* Set recursive mapping */
    spKernelPageDir[KERNEL_PGDIR_ENTRY_COUNT - 1] =
        ((uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET) |
        PAGE_FLAG_PAGE_SIZE_4KB                          |
        PAGE_FLAG_SUPER_ACCESS                           |
        PAGE_FLAG_READ_WRITE                             |
        PAGE_FLAG_PRESENT;

    /* Update the whole page table */
    cpuSetPageDirectory((uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);

    /* Map kernel code */
    _memoryMgrMapKernelRegion(&kernelSectionStart,
                              &kernelSectionEnd,
                              (uintptr_t)&_START_LOW_AP_STARTUP_ADDR,
                              (uintptr_t)&_END_LOW_AP_STARTUP_ADDR,
                              MEMMGR_MAP_RO | MEMMGR_MAP_EXEC);
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

#ifdef _TRACING_ENABLED
    _memoryMgrMapKernelRegion(&kernelSectionStart,
                              &kernelSectionEnd,
                              (uintptr_t)&_KERNEL_TRACE_BUFFER_BASE,
                              (uintptr_t)&_KERNEL_TRACE_BUFFER_BASE +
                              (uintptr_t)&_KERNEL_TRACE_BUFFER_SIZE,
                              MEMMGR_MAP_RW);
#endif

#ifdef _TESTING_FRAMEWORK_ENABLED
    _memoryMgrMapKernelRegion(&kernelSectionStart,
                              &kernelSectionEnd,
                              (uintptr_t)&_KERNEL_TEST_BUFFER_BASE,
                              (uintptr_t)&_KERNEL_TEST_BUFFER_BASE +
                              (uintptr_t)&_KERNEL_TEST_BUFFER_SIZE,
                              MEMMGR_MAP_RW);
#endif

    /* Copy temporary entry to kernel entry and clear temp  */
    spKernelPageDir[KERNEL_MEM_OFFSET >> PG_DIR_ENTRY_OFFSET] =
        spKernelPageDir[KERNEL_PGDIR_TEMP_ENTRY];

    spKernelPageDir[KERNEL_PGDIR_TEMP_ENTRY] = 0;

    /* Update the whole page table */
    cpuSetPageDirectory((uintptr_t)spKernelPageDir - KERNEL_MEM_OFFSET);

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_INIT_MAPPING_EXIT,
                       0);
}

void memoryMgrInit(void)
{
    OS_RETURN_E error;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_INIT_ENTRY,
                       0);

    /* Initialize structures */
    sPhysMemList.pQueue = kQueueCreate(TRUE);
    KERNEL_SPINLOCK_INIT(sPhysMemList.lock);

    sKernelFreePagesList.pQueue = kQueueCreate(TRUE);
    KERNEL_SPINLOCK_INIT(sKernelFreePagesList.lock);

    /* Detect the memory */
    _memoryMgrDetectMemory();

    /* Setup the address table */
    _memoryMgrInitAddressTable();

    /* Map the kernel */
    _memoryMgrInitPaging();

    /* Registers the page fault handler */
    error = exceptionRegister(PAGE_FAULT_EXC_LINE, _pageFaultHandler);
    MEM_ASSERT(error == OS_NO_ERR,
               "Failed to register the page fault handler",
               error);

#if MEMORY_MGR_DEBUG_ENABLED
    _printKernelMap();
#endif

    TEST_POINT_FUNCTION_CALL(memmgrTest, TEST_MEMMGR_ENABLED);

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_INIT_EXIT,
                       0);
}

void* memoryKernelMap(const void*    kPhysicalAddress,
                      const size_t   kSize,
                      const uint32_t kFlags,
                      OS_RETURN_E*   pError)
{
    uintptr_t   kernelPages;
    size_t      pageCount;
    OS_RETURN_E error;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_KERNELMAP_ENTRY,
                       5,
                       0,
                       (uint32_t)(uintptr_t)kPhysicalAddress,
                       0,
                       (uint32_t)kSize,
                       kFlags);

    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Mapping physical address 0x%p (%dB) | Flags: 0x%x",
                 kPhysicalAddress,
                 kSize,
                 kFlags);

    /* Check size */
    if((kSize & PAGE_SIZE_MASK) != 0 || kSize < KERNEL_PAGE_SIZE)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_INCORRECT_VALUE;
        }
        KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                           TRACE_X86_MEMMGR_KERNELMAP_EXIT,
                           8,
                           0,
                           (uint32_t)(uintptr_t)kPhysicalAddress,
                           0,
                           (uint32_t)kSize,
                           kFlags,
                           OS_ERR_INCORRECT_VALUE,
                           0,
                           0);
        return NULL;
    }

    pageCount = kSize / KERNEL_PAGE_SIZE;

    /* Allocate pages */
    kernelPages = _allocateKernelPages(pageCount);
    if(kernelPages == 0)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_NO_MORE_MEMORY;
        }
        KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                           TRACE_X86_MEMMGR_KERNELMAP_EXIT,
                           8,
                           0,
                           (uint32_t)(uintptr_t)kPhysicalAddress,
                           0,
                           (uint32_t)kSize,
                           kFlags,
                           OS_ERR_NO_MORE_MEMORY,
                           0,
                           0);
        return NULL;
    }

    /* Apply mapping */
    error = _memoryMgrMap(kernelPages,
                          (uintptr_t)kPhysicalAddress,
                          pageCount,
                          kFlags | MEMMGR_MAP_KERNEL | PAGE_FLAG_GLOBAL);
    if(error != OS_NO_ERR)
    {
        _releaseKernelPages(kernelPages, pageCount);
        if(pError != NULL)
        {
            *pError = error;
        }
        KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                           TRACE_X86_MEMMGR_KERNELMAP_EXIT,
                           8,
                           0,
                           (uint32_t)(uintptr_t)kPhysicalAddress,
                           0,
                           (uint32_t)kSize,
                           kFlags,
                           error,
                           0,
                           0);
        return NULL;
    }
    else
    {
        if(pError != NULL)
        {
            *pError = OS_NO_ERR;
        }
        KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                           TRACE_X86_MEMMGR_KERNELMAP_EXIT,
                           8,
                           0,
                           (uint32_t)(uintptr_t)kPhysicalAddress,
                           0,
                           (uint32_t)kSize,
                           kFlags,
                           OS_NO_ERR,
                           0,
                           (uint32_t)kernelPages);
        return (void*)kernelPages;
    }
}

OS_RETURN_E memoryKernelUnmap(const void* kVirtualAddress, const size_t kSize)
{
    size_t      pageCount;
    OS_RETURN_E error;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_KERNELUNMAP_ENTRY,
                       4,
                       0,
                       (uint32_t)(uintptr_t)kVirtualAddress,
                       0,
                       (uint32_t)kSize);

    KERNEL_DEBUG(MEMORY_MGR_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Unmapping virtual address 0x%p (%dB)",
                 kVirtualAddress,
                 kSize);

    /* Check size */
    if((kSize & PAGE_SIZE_MASK) != 0 || kSize < KERNEL_PAGE_SIZE)
    {
        KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                           TRACE_X86_MEMMGR_KERNELUNMAP_ENTRY,
                           5,
                           0,
                           (uint32_t)(uintptr_t)kVirtualAddress,
                           0,
                           (uint32_t)kSize,
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    pageCount = kSize / KERNEL_PAGE_SIZE;

    /* Check if actually kernel addresses */
    if((uintptr_t)kVirtualAddress < sKernelVirtualMemBounds.base ||
       (uintptr_t)kVirtualAddress >= sKernelVirtualMemBounds.limit)
    {
        KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                           TRACE_X86_MEMMGR_KERNELUNMAP_ENTRY,
                           5,
                           0,
                           (uint32_t)(uintptr_t)kVirtualAddress,
                           0,
                           (uint32_t)kSize,
                           OS_ERR_OUT_OF_BOUND);
        return OS_ERR_OUT_OF_BOUND;
    }

    /* Unmap */
    error = _memoryMgrUnmap((uintptr_t)kVirtualAddress, pageCount);

    /* Release the kernel page if correctly unmaped */
    if(error == OS_NO_ERR)
    {
        _releaseKernelPages((uintptr_t)kVirtualAddress, pageCount);
    }

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_KERNELUNMAP_EXIT,
                       5,
                       0,
                       (uint32_t)(uintptr_t)kVirtualAddress,
                       0,
                       (uint32_t)kSize,
                       error);

    return error;
}

void* memoryKernelMapStack(const size_t kSize)
{
    size_t      pageCount;
    size_t      mappedCount;
    size_t      i;
    OS_RETURN_E error;
    uintptr_t   pageBaseAddress;
    uintptr_t   newFrame;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_KERNEL_MAP_STACK_ENTRY,
                       2,
                       0,
                       (uint32_t)kSize);


    /* Get the page count */
    pageCount = ALIGN_UP(kSize, KERNEL_PAGE_SIZE) / KERNEL_PAGE_SIZE;

    /* Request the pages + 1 to catch overflow (not mapping the last page)*/
    pageBaseAddress = _allocateKernelPages(pageCount + 1);
    if(pageBaseAddress == 0)
    {
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
                              MEMMGR_MAP_RW | MEMMGR_MAP_KERNEL);
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
                newFrame = memoryMgrGetPhysAddr(pageBaseAddress +
                                                KERNEL_PAGE_SIZE * i,
                                                NULL);
                MEM_ASSERT(newFrame != MEMMGR_PHYS_ADDR_ERROR,
                           "Invalid physical frame",
                           OS_ERR_INCORRECT_VALUE);
                _releaseFrames(newFrame, 1);
            }

            _memoryMgrUnmap(pageBaseAddress, mappedCount);
        }
        _releaseKernelPages(pageBaseAddress, pageCount + 1);

        pageBaseAddress = (uintptr_t)NULL;
    }

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_KERNEL_MAP_STACK_EXIT,
                       4,
                       0,
                       (uint32_t)kSize,
                       0,
                       (uint32_t)pageBaseAddress);

    return (void*)pageBaseAddress;
}

void memoryKernelUnmapStack(const uintptr_t kBaseAddress, const size_t kSize)
{
    size_t    pageCount;
    size_t    i;
    uintptr_t frameAddr;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_KERNEL_UNMAP_STACK_ENTRY,
                       4,
                       0,
                       (uint32_t)kSize,
                       0,
                       (uint32_t)kBaseAddress);

    MEM_ASSERT((kBaseAddress & PAGE_SIZE_MASK) == 0 &&
                (kSize & PAGE_SIZE_MASK) == 0 &&
                kSize != 0,
                "Unmaped kernel stack with invalid parameters",
                OS_ERR_INCORRECT_VALUE);

    /* Get the page count */
    pageCount = kSize / KERNEL_PAGE_SIZE;

    /* Free the frames and memory */
    for(i = 0; i < pageCount; ++i)
    {
        frameAddr = memoryMgrGetPhysAddr(kBaseAddress + KERNEL_PAGE_SIZE * i,
                                         NULL);
        MEM_ASSERT(frameAddr != MEMMGR_PHYS_ADDR_ERROR,
                   "Invalid physical frame",
                   OS_ERR_INCORRECT_VALUE);
        _releaseFrames(frameAddr, 1);
    }

    /* Unmap the memory */
    _memoryMgrUnmap(kBaseAddress, pageCount);
    _releaseKernelPages(kBaseAddress, pageCount + 1);

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_KERNEL_UNMAP_STACK_EXIT,
                       4,
                       0,
                       (uint32_t)kSize,
                       0,
                       (uint32_t)kBaseAddress);
}

uintptr_t memoryMgrGetPhysAddr(const uintptr_t kVirtualAddress,
                               uint32_t* pFlags)
{
    uintptr_t  retPhysAddr;
    uintptr_t* pPgDirRecurEntry;
    uintptr_t* pPgTableRecurEntry;
    uint16_t   pgDirEntry;
    uint16_t   pgTableEntry;

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_GET_PHYS_ADDR_ENTRY,
                       2,
                       0,
                       kVirtualAddress);

    retPhysAddr = MEMMGR_PHYS_ADDR_ERROR;

    /* Search the entry in the page directory */
    pPgDirRecurEntry = (uintptr_t*)KERNEL_RECUR_PG_DIR_BASE;

    /* Get entry indexes */
    pgDirEntry   = kVirtualAddress >> PG_DIR_ENTRY_OFFSET;
    pgTableEntry = (kVirtualAddress >> PG_TABLE_ENTRY_OFFSET) &
                   PG_TABLE_ENTRY_OFFSET_MASK;

    KERNEL_CRITICAL_LOCK(sLock);

    /* If the entry is empty */
    if((pPgDirRecurEntry[pgDirEntry] & PAGE_FLAG_PRESENT) != 0)
    {
        /* Get page table recursive entry */
        pPgTableRecurEntry = (uintptr_t*)KERNEL_RECUR_PGTABLE_BASE(pgDirEntry);
        if((pPgTableRecurEntry[pgTableEntry] & PAGE_FLAG_PRESENT) != 0)
        {
            if(pFlags != NULL)
            {
                retPhysAddr = pPgTableRecurEntry[pgTableEntry];
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

                retPhysAddr &= PG_ENTRY_ADDR_MASK;
            }
            else
            {
                retPhysAddr = pPgTableRecurEntry[pgTableEntry] &
                              PG_ENTRY_ADDR_MASK;
            }

        }
    }

    KERNEL_CRITICAL_UNLOCK(sLock);
    if(retPhysAddr != MEMMGR_PHYS_ADDR_ERROR)
    {
        retPhysAddr |= kVirtualAddress & PAGE_SIZE_MASK;
    }

    KERNEL_TRACE_EVENT(TRACE_X86_MEMMGR_ENABLED,
                       TRACE_X86_MEMMGR_GET_PHYS_ADDR_EXIT,
                       4,
                       0,
                       kVirtualAddress,
                       0,
                       retPhysAddr);
    return retPhysAddr;
}

/************************************ EOF *************************************/