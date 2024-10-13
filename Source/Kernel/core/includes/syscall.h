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

/** @brief Provides the list of available system call Ids */
typedef enum
{
    /** @brief Performs a sleep system call */
    SYSCALL_SLEEP = 0,
    /** @brief Performs a schedule system call */
    SYSCALL_SCHEDULE = 1,
    /** @brief Performs a fork system call */
    SYSCALL_FORK = 2
} SYSCALL_ID_E;

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
 * @param[in] kSysCallId The system identifier to use.
 * @param[in, out] pParams The system call parameters.
 *
 * @return The function returns the success or error status.
 */
OS_RETURN_E syscallPerform(const SYSCALL_ID_E kSysCallId, void* pParams);

#endif /* #ifndef __CORE_SYSCALL_H_ */

/************************************ EOF *************************************/