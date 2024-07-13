/*******************************************************************************
 * @file memory_manager.h
 *
 * @see memory.c
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

#ifndef __MEMORY_MGR_
#define __MEMORY_MGR_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h> /* Standard int definitions */
#include <stddef.h> /* Standard definitions */
#include <kerror.h> /* Kernel errors */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Memory mapping flags: Read-Only mapping */
#define MEMMGR_MAP_RO 0x00000000
/** @brief Memory mapping flags: Read-Write mapping */
#define MEMMGR_MAP_RW 0x00000001
/** @brief Memory mapping flags: Execute mapping */
#define MEMMGR_MAP_EXEC 0x00000002

/** @brief Memory mapping flags: Kernel access only  */
#define MEMMGR_MAP_KERNEL 0x00000004
/** @brief Memory mapping flags: Kernel and user access */
#define MEMMGR_MAP_USER 0x00000000

/** @brief Memory mapping flags: Cache disabled */
#define MEMMGR_MAP_CACHE_DISABLED 0x00000008
/** @brief Memory mapping flags: Hardware */
#define MEMMGR_MAP_HARDWARE 0x00000008

/** @brief Kernel page size */
#define KERNEL_PAGE_SIZE 0x1000
/** @brief Page size mask */
#define PAGE_SIZE_MASK 0xFFF

/** @brief Defines the error for physical address */
#define MEMMGR_PHYS_ADDR_ERROR ((uintptr_t)0xFFFFFFFFFFFFFFFFULL)

/* @brief  */
/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines a memory range */
typedef struct
{
    /** @brief Range's base address. */
    uintptr_t base;

    /** @brief Range's limit. */
    uintptr_t limit;
} mem_range_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * @brief Inititalizes the kernel's memory manager.
 *
 * @brief Initializes the kernel's memory manager while detecting the system's
 * memory organization.
 */
void memoryMgrInit(void);

/**
 * @brief Maps a physical region (memory or hardware) in the kernel address
 * space.
 *
 * @details Maps a kernel virtual memory region to a memory region. The function
 * does not check if the physical region is already mapped and will create a
 * new mapping. The physical address and the size must be aligned on page
 * boundaries. If not, the mapping fails and NULL is returned.
 *
 * @param[in] kPhysicalAddress The physical address to map. Must be aligned on
 * page boundaries.
 * @param[in] kSize The size of the region to map in bytes. Must be aligned on
 * page boundaries.
 * @param[in] kFlags The mapping flags, see the MEM_MGR flags for more
 * infomation.
 * @param[out] pError The error buffer to store the operation's result. If NULL,
 * does not set the error value.
 *
 * @return The function returns the virtual base address of the mapped region.
 * NULL is returned on error.
 *
 * @warning The mapping does not remove the physical address from the free
 * free memory. Thus, if the user wants to ensure this memory region not to be
 * used later (or already used) memoryAllocFrames must be used to get a free
 * physical memory region. This does not apply to hardware mapping.
 */
void* memoryKernelMap(const void*    kPhysicalAddress,
                      const size_t   kSize,
                      const uint32_t kFlags,
                      OS_RETURN_E*   pError);

/**
 * @brief Maps a physical memory region in the kernel address space.
 *
 * @details Maps a physical memory region in the kernel address space. The
 * function allocated free memory frames to the kernel and creates a new
 * mapping. The size must be aligned on page boundaries. If not, the mapping
 * fails and NULL is returned.
 *
 * @param[in] kSize The size of the region to map in bytes. Must be aligned on
 * page boundaries.
 * @param[in] kFlags The mapping flags, see the MEM_MGR flags for more
 * infomation.
 * @param[out] pError The error buffer to store the operation's result. If NULL,
 * does not set the error value.
 *
 * @return The function returns the virtual base address of the mapped region.
 * NULL is returned on error.
 */
void* memoryKernelAllocate(const size_t   kSize,
                           const uint32_t kFlags,
                           OS_RETURN_E*   pError);

/**
 * @brief Unmaps a virtual region (memory or hardware) from the kernel address
 * space.
 *
 * @details Unmaps a virtual region (memory or hardware) from the kernel address
 * space. The virtual address and the size must be aligned on page
 * boundaries. If not, the unmapping fails and an error is returned.
 *
 * @param[in] kVirtualAddress The virtual address to unmap. Must be aligned on
 * page boundaries.
 * @param[in] kSize The size of the region to unmap map in bytes. Must be
 * aligned on page boundaries.
 * @param[in] kFlags The mapping flags, see the MEM_MGR flags for more
 * infomation.
 * @param[out] pError The error buffer to store the operation's result. If NULL,
 * does not set the error value.
 *
 * @return The function returns the virtual base address of the mapped region.
 * NULL is returned on error.
 */
OS_RETURN_E memoryKernelUnmap(const void* kVirtualAddress, const size_t kSize);

/**
 * @brief Returns the physical address of a virtual address mapped in the
 * current page directory.
 *
 * @details Returns the physical address of a virtual address mapped in the
 * current page directory. If not found, MEMMGR_PHYS_ADDR_ERROR is returned.
 *
 * @param[in] kVirtualAddress The virtual address to lookup.
 * @param[out] pFlags The memory flags used for the mapping. Can be NULL.
 *
 * @returns The physical address of a virtual address mapped in the
 * current page directory. If not found, MEMMGR_PHYS_ADDR_ERROR is returned.
 */
uintptr_t memoryMgrGetPhysAddr(const uintptr_t kVirtualAddress,
                               uint32_t* pFlags);

#endif /* #ifndef __MEMORY_MGR_ */

/************************************ EOF *************************************/