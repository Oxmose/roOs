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

/* None */

#endif /* #ifndef __I386_X86_MEMORY_H_ */

/************************************ EOF *************************************/