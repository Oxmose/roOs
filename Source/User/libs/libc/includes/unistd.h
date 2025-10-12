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

#include <stddef.h>    /* Standard definitions */
#include <sys/types.h> /* System types */

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
 * @details Writes up to count bytes from the buffer starting at buf to the
 * file referred to by the file descriptor fd.
 *
 * @param[in] fd The file descriptor to write to.
 * @param[in] buf The buffer to write to the file descriptor.
 * @param[in] count The size in bytes to the data to write.
 *
 * @return The function returns the number of bytes effectively written to the
 * file descriptor.
 */
ssize_t write(int fd, const void* buf, size_t count);

/**
 * @brief Causes the calling thread to sleep.
 *
 * @details Causes the calling thread to sleep either until the
 * number of real-time seconds specified in seconds have elapsed or
 * until a signal arrives which is not ignored.
 *
 * @param[in] seconds The number of seconds to sleep.
 *
 * @return Zero if the requested time has elapsed, or the number of seconds
 *  left to sleep, if the call was interrupted by a signal handler.
 */
unsigned int sleep(unsigned int seconds);

/**
 * @brief Creates a new process by duplicating the calling process.
 *
 * @details Creates a new process by duplicating the calling process. The new
 * process is referred to as the child process. The calling process is referred
 * to as the parent process.
 *
 * @return On success, the PID of the child process is returned in the
 * parent, and 0 is returned in the child.  On failure, -1 is
 * returned in the parent, no child process is created, and errno is
 * set to indicate the error.
 */
pid_t fork(void);

#endif /* #ifndef __LIB_UNISTD_H_ */

/************************************ EOF *************************************/