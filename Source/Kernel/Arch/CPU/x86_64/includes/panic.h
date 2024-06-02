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

#include <cpu.h>          /* CPU structures */
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
        kernelPanic(ERROR, MODULE, MSG, __FILE__, __LINE__);           \
    }                                                                   \
    else                                                                \
    {                                                                   \
        /* TODO: process_panic(ERROR);  */                              \
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
 * @param[in, out] curr_thread The thread that generated the panic
 *
 * @warning Panic should never be called, it must only be used as an interrupt
 * handler.
 */
void kernelPanicHandler(kernel_thread_t* curr_thread);

/**
 * @brief Causes a kernel panic.
 *
 * @details Causes a kernel panic. This will raise an interrupt to generate the
 * panic.
 *
 * @param[in] errorCode The error code to display on the kernel panic's screen.
 * @param[in] module The module that generated the panic. Can be empty when not
 * relevant.
 * @param[in] msg The message to display in the kernel's panig screen.
 * @param[in] file The name of the source file where the panic was called.
 * @param[in] line The line at which the panic was called.
 */
void kernelPanic(const uint32_t errorCode,
                  const char* module,
                  const char* msg,
                  const char* file,
                  const size_t line);

#endif /* #ifndef __CPU_PANIC_H_ */

/************************************ EOF *************************************/