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
/* TODO: Doc */
void cpuKernelSyscallRaise(const uintptr_t syscallHandler,
                           void*           pParams,
                           const uintptr_t kKernelStack);

void cpuSaveSyscallContext(const uintptr_t syscallReturnAddress);
void cpuRestoreSyscallContext(const kernel_thread_t* kpThread);

#endif /* #ifndef __CPU_CPUSYSCALL_H_ */

/************************************ EOF *************************************/