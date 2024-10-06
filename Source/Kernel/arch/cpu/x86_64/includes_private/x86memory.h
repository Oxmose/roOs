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

/* None */

#endif /* #ifndef __I386_X86_MEMORY_H_ */

/************************************ EOF *************************************/