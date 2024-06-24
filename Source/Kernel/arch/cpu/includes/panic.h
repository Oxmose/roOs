/*******************************************************************************
 * @file panic.h
 *
 * @see panic.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 22/04/2023
 *
 * @version 1.0
 *
 * @brief Panic feature of the kernel.
 *
 * @details Kernel panic functions. Displays the CPU registers, the faulty
 * instruction, the interrupt ID and cause for a kernel panic. For a process
 * panic the panic will kill the process.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CPU_PANIC_H_
#define __CPU_PANIC_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h>       /* Generic int types */
#include <stddef.h>       /* Pointer types */
#include <ctrl_block.h>   /* Thread's control block */

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

/**
 * @brief Raises a kernel panic with error code and collect other data.
 *
 * @param[in] ERROR The error code forthe panic.
 * @param[in] MODULE The module that generated the panic. Can be empty when not
 * relevant.
 * @param[in] MSG The panic message used for kernel panic.
 * @param[in] IS_KERNEL Set to TRUE for a kernel panic or false for a process
 * panic.
 */
#define PANIC(ERROR, MODULE, MSG, IS_KERNEL) {                          \
    if(IS_KERNEL == TRUE)                                               \
    {                                                                   \
        kernelPanic(ERROR, MODULE, MSG, __FILE__, __LINE__);            \
    }                                                                   \
    else                                                                \
    {                                                                   \
        /* TODO: processPanic(ERROR);  */                               \
        kernelPanic(ERROR, MODULE, MSG, __FILE__, __LINE__);            \
    }                                                                   \
}

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
 * @brief Displays the kernel panic screen.
 *
 * @details Displays the kernel panic screen. This screen dumps the CPU registers
 * and the stack state before the panic occured.
 *
 * @param[in, out] pCurrThread The thread that generated the panic
 *
 * @warning Panic should never be called, it must only be used as an interrupt
 * handler.
 */
void kernelPanicHandler(kernel_thread_t* pCurrThread);

/**
 * @brief Causes a kernel panic.
 *
 * @details Causes a kernel panic. This will raise an interrupt to generate the
 * panic.
 *
 * @param[in] kErrorCode The error code to display on the kernel panic's screen.
 * @param[in] kpModule The module that generated the panic. Can be empty when
 * not relevant.
 * @param[in] kpMsg The message to display in the kernel's panig screen.
 * @param[in] kpFile The name of the source file where the panic was called.
 * @param[in] kLine The line at which the panic was called.
 */
void kernelPanic(const uint32_t kErrorCode,
                 const char*    kpModule,
                 const char*    kpMsg,
                 const char*    kpFile,
                 const size_t   kLine);

#endif /* #ifndef __CPU_PANIC_H_ */

/************************************ EOF *************************************/