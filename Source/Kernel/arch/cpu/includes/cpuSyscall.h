/*******************************************************************************
 * @file cpuSyscall.h
 *
 * @see cpuSyscall.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 11/10/2024
 *
 * @version 1.0
 *
 * @brief CPU system call services.
 *
 * @details CPU system call services. Raise system calls and call the handlers
 * based on the provided functions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CPU_CPUSYSCALL_H_
#define __CPU_CPUSYSCALL_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <kerror.h>     /* Kernel errors */
#include <ctrl_block.h> /* Kernel thread blocks */

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
 * @brief Raises a system call from the kernel space.
 *
 * @details Raises a system call from the kernel space. The function will
 * perform a system call after saving the required context. The parameters as
 * passed as-is.
 *
 * @param[in] syscallHandler The function that should handle the system call.
 * @param[in] pParams The parameters to pass to the system call handler.
 * @param[in] pCurrThread The current thread that required the system call to
 * be raised.
 *
 * @warning This function must only be called from kernel space.
 */
void cpuKernelSyscallRaise(const uintptr_t  syscallHandler,
                           void*            pParams,
                           kernel_thread_t* pCurrThread);

/**
 * @brief Raises a system call from the kernel space.
 *
 * @details Raises a system call from the kernel space. The function will
 * perform a system call after saving the required context. The parameters as
 * passed as-is.
 *
 * @param[in] syscallHandler The function that should handle the system call.
 * @param[in] pParams The parameters to pass to the system call handler.
 * @param[in] pCurrThread The current thread that required the system call to
 * be raised.
 *
 * @warning This function must only be called from kernel space.
 */
void cpuSwitchKernelSyscallContext(const uintptr_t  syscallReturnAddress,
                                   kernel_thread_t* pCurrThread);

/**
 * @brief Restores the context of a thread after returning from a system call.
 *
 * @details Restores the context of a thread after returning from a system call.
 * This function perform the restore for a kernel thread.
 *
 * @param[in] kpThread The thread for which the context should be restored.
 *
 * @warning This function must only be called from kernel space.
 */
void cpuRestoreKernelSyscallContext(const kernel_thread_t* kpThread);

#endif /* #ifndef __CPU_CPUSYSCALL_H_ */

/************************************ EOF *************************************/