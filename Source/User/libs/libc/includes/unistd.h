/*******************************************************************************
 * @file unistd.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/10/2024
 *
 * @version 1.0
 *
 * @brief Unistd port for roOs.
 *
 * @details Unistd port for roOs. This port is not inteded to be conplete and 
 * provides API for roOs.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_UNISTD_H_
#define __LIB_UNISTD_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stddef.h> /* Standard definitions */

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
 * @brief Writes to a file descriptor.
 * 
 * @details Writes up to count bytes from the buffer starting at pBuffer to the 
 * file referred to by the file descriptor fileFd.
 * 
 * @param fileFd The file descriptor to write to.
 * @param pBuffer The buffer to write to the file descriptor.
 * @param count The size in bytes to the data to write.
 * 
 * @return The function returns the number of bytes effectively written to the
 * file descriptor. 
 */
ssize_t write(int fileFd, const void* pBuffer, size_t count);

#endif /* #ifndef __LIB_UNISTD_H_ */

/************************************ EOF *************************************/