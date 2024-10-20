/*******************************************************************************
 * @file syscall.c
 *
 * @see syscall.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <kerror.h>     /* Kernel errors */
#include <stddef.h>     /* Standard definitions */
#include <stdbool.h>    /* Standard bool type definition */
#include <scheduler.h>  /* Kernel scheduler */
#include <cpuSyscall.h> /* CPU system call manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <syscall.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "SYSCALL"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief System call handler table entry. */
typedef struct
{
    /**
     * @brief The system call handler function.
     *
     * @details The system call handler function. This function is up to return
     * or not depending on if a schedule happened.
     *
     * @param[out] pParams The system call parameters.
     */
    void (*pHandler)(void* pParams);
} syscall_handler_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
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

/** @brief System call handlers table */
static syscall_handler_t sSyscalTable[] = {
    /* SYSCALL_SLEEP */
    {
        .pHandler = schedSyscallHandleSleep
    },
    /* SYSCALL_SCHEDULE */
    {
        .pHandler = schedSyscallHandleSchedule
    },
    /* SYSCALL_FORK */
    {
        .pHandler = schedSyscallHandleFork
    },
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
OS_RETURN_E syscallPerform(const SYSCALL_ID_E kSysCallId, void* pParams)
{
    kernel_thread_t* pCurrThread;

    /* Check the system call ID */
    if(kSysCallId >= ARRAY_SIZE(sSyscalTable))
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    pCurrThread = schedGetCurrentThread();

    /* Issue the system call */
    if(pCurrThread->type == THREAD_TYPE_KERNEL)
    {
        cpuKernelSyscallRaise((uintptr_t)sSyscalTable[kSysCallId].pHandler,
                              pParams,
                              pCurrThread);
    }
    else
    {
        /* Currently we do not support user space */
        return OS_ERR_NOT_SUPPORTED;
    }

    return OS_NO_ERR;
}

/************************************ EOF *************************************/