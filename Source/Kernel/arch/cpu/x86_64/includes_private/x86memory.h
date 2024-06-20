/*******************************************************************************
 * @file x86memory.h
 *
 * @see memory.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief x86-64 Memory management function and definitions.
 *
 * @details x86-64 Memory management function and definitions. Defines address
 * ranges and features.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __X664_X86_MEMORY_H_
#define __X664_X86_MEMORY_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the virtual address with supported */
#define KERNEL_VIRTUAL_ADDR_WIDTH 48

/** @brief Defines the limit address allocable by the kernel (excludes recursive
 * mapping). */
#define KERNEL_VIRTUAL_ADDR_MAX 0xFFFFFFFFFFFFEFFF

/**
 * @brief Kernel virtual memory offset
 * @warning This value should be updated to fit other configuration files
 */
#define KERNEL_MEM_OFFSET 0xFFFFFFFF80000000

/** @brief Kernel physical memory offset */
#define KERNEL_MEM_START 0x00100000

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

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
 * @brief Maps a stack in the kernel memory region and returns its address.
 *
 * @details Maps a stack in the kernel memory region and returns its address.
 * One more page after the stack is allocated but not mapped to catch overflows.
 * The required frames are also allocated.
 *
 * @param[in] kSize The size of the stack. If not aligned with the kernel page
 * size, the actual mapped size will be aligned up on page boundaries.
 *
 * @return The base address of the stack in kernel memory is returned.
 */
void* memoryKernelMapStack(const size_t kSize);

/**
 * @brief Unmaps a stack in the kernel memory region and frees the associated
 * physical memory.
 *
 * @details Maps a stack in the kernel memory region and frees the associated
 * physical memory.
 * The additional overflow page is also freed.
 *
 * @param[in] kBaseAddress The base address of the stack to unmap. If not
 * aligned with the kernel page size, a panic is generated.
 * @param[in] kSize The size of the stack. If not aligned with the kernel page
 * size, a panic is generated.
 */
void memoryKernelUnmapStack(const uintptr_t kBaseAddress, const size_t kSize);

#endif /* #ifndef __I386_X86_MEMORY_H_ */

/************************************ EOF *************************************/