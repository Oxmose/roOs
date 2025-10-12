/*******************************************************************************
 * @file syscall.h
 *
 * @see syscall.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 12/10/2024
 *
 * @version 1.0
 *
 * @brief Kernel system call manager.
 *
 * @details Kernel system call manager. Used to register and handle system call
 * entry and exti points.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_SYSCALL_H_
#define __CORE_SYSCALL_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <kerror.h> /* Kernel errors */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/**
 * @brief Provides the list of available system call Ids
 *
 * @warning Any user library that use system call must be compatible with this
 * list.
 */
typedef enum
{
    /** @brief Performs a sleep system call */
    SYSCALL_SLEEP = 0,
    /** @brief Performs a schedule system call */
    SYSCALL_SCHEDULE = 1,
    /** @brief Performs a fork system call */
    SYSCALL_FORK = 2,
    /** @brief Performs a VFS write system call */
    SYSCALL_WRITE = 3,
    /** @brief Performs a clock get time system call */
    SYSCALL_CLOCK_GETTIME = 4,
} SYSCALL_ID_E;


/** @brief The syscall minimal parameters */
typedef int32_t syscall_min_params_t;

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
 * @brief Performs a system call.
 *
 * @details Performs a system call. The underlying CPU system call facility will
 * be called to perform the required operation and issue the system call.
 * The parameters for input and output are provided by the pParams parameter.
 *
 * @param[in] kSysCallId The system call identifier to use.
 * @param[in, out] pParams The system call parameters.
 *
 * @return The function returns the success or error status.
 */
OS_RETURN_E syscallPerform(const SYSCALL_ID_E kSysCallId, void* pParams);

/**
 * @brief Handles a system call request from the user space.
 *
 * @details Handles a system call request from the user space. This function
 * will execute the necessary process to handle the system call, call the
 * associated kernel function and setup the return arguments.
 *
 * @param[in] kSyscallId The system call ID to handle.
 * @param[in] pParams The parameters to pass to the system call handler.
 */
void syscallHandle(const SYSCALL_ID_E kSysCallId, void* pParams);

#endif /* #ifndef __CORE_SYSCALL_H_ */

/************************************ EOF *************************************/