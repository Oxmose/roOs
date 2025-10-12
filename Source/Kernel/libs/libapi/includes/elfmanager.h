/*******************************************************************************
 * @file elfmanager.h
 *
 * @see elfmanager.c
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

#ifndef __LIB_ELFMANAGER_H_
#define __LIB_ELFMANAGER_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <kerror.h>     /* Kernel error */
#include <stdint.h>     /* Standard integer types */
#include <ctrl_block.h> /* Process control block */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

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
 * @brief Loads an ELF file in the process memory space.
 *
 * @details Loads an ELF file in the process memory space. This function assumes
 * that the process user memory space is empty. The function loads the ELF and
 * allocates the required memory regions in the process memory space. The
 * entry point address is updated on success.
 *
 * @param[in] kpElfPath The path to the ELF file to load.
 * @param[out] pEntryPoint The buffer that receives the ELF program entry point.
 *
 * @return The function returns the success or error status.
 */
OS_RETURN_E elfManagerLoadElf(const char* kpElfPath,
                              uintptr_t*  pEntryPoint);

#endif /* #ifndef __LIB_ELFMANAGER_H_ */

/************************************ EOF *************************************/