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
 * @brief i386 Memory management function and definitions.
 *
 * @details i386 Memory management function and definitions. Defines address
 * ranges and features.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __I386_X86_MEMORY_H_
#define __I386_X86_MEMORY_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the limit address allocable by the kernel (excludes recursive
 * mapping). */
#define KERNEL_VIRTUAL_ADDR_MAX 0xFFBFFFFF

/**
 * @brief Kernel virtual memory offset
 * @warning This value should be updated to fit other configuration files
 */
#define KERNEL_MEM_OFFSET 0xE0000000

/** @brief Kernel physical memory offset */
#define KERNEL_MEM_START 0x00100000

/** @brief User total memory start. */
#define USER_MEMORY_START 0x00100000ULL

/** @brief User total memory end. */
#define USER_MEMORY_END 0xE00000000ULL

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


/**
 * @brief Defines the structure that contains the memory information for a
 * process.
 */
typedef struct
{
    /** @brief The physical address of the process page directory. */
    uintptr_t pageDir;

    /** @brief The free page table of the process. */
    mem_list_t freePageTable;

    /** @brief The memory management lock for the process */
    kernel_spinlock_t lock;
} memproc_info_t;
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

/* None */

#endif /* #ifndef __I386_X86_MEMORY_H_ */

/************************************ EOF *************************************/