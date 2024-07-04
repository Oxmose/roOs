/*******************************************************************************
 * @file exceptions.c
 *
 * @see exceptions.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 29/05/2023
 *
 * @version 2.0
 *
 * @brief Exceptions manager.
 *
 * @warning These functions must be called during or after the interrupts are
 * set.
 *
 * @details Exception manager. Allows to attach ISR to exceptions lines.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <panic.h>         /* Kernel panic */
#include <stdint.h>        /* Generic int types */
#include <stddef.h>        /* Standard definitions */
#include <string.h>        /* String manipulation */
#include <critical.h>      /* Critical sections */
#include <kerneloutput.h>  /* Kernel output */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <exceptions.h>

/* Tracing feature */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module's name */
#define MODULE_NAME "EXCEPTIONS"

/** @brief Divide by zero exception line. */
#define DIV_BY_ZERO_LINE 0x00

/** @brief Illegal instruction exception line. */
#define INVALID_INSTRUCTION_LINE 0x6

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the exception manager to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the exception manager to ensure correctness of
 * execution. Due to the critical nature of the exception manager, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define EXC_ASSERT(COND, MSG, ERROR) {                      \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/

/** @brief Stores the handlers for each interrupt, defined in interrupt.c. */
extern custom_handler_t* pKernelInterruptHandlerTable;

/** @brief Interrupt table lock */
extern kernel_spinlock_t kernelInterruptHandlerTableLock;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Stores the CPU's maximal exception numbers. */
static uint32_t sMaxExceptionNumber;

/** @brief Stores the CPU's minimal exception numbers. */
static uint32_t sMinExceptionNumber;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void exceptionInit(void)
{
    OS_RETURN_E                   err;
    const cpu_interrupt_config_t* kpCpuIntConfig;

    KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED, TRACE_EXCEPTION_INIT_ENTRY, 0);

    KERNEL_DEBUG(EXCEPTIONS_DEBUG_ENABLED, "EXCEPTIONS",
                 "Initializing exception manager.");

    /* Get the CPU exception bounds */
    kpCpuIntConfig = cpuGetInterruptConfig();
    sMinExceptionNumber = kpCpuIntConfig->minExceptionLine;
    sMaxExceptionNumber = kpCpuIntConfig->maxExceptionLine;

    err = cpuRegisterExceptions();
    EXC_ASSERT(err == OS_NO_ERR,
               "Could not initialize exception manager.",
               err);

    KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED, TRACE_EXCEPTION_INIT_EXIT, 0);
}

OS_RETURN_E exceptionRegister(const uint32_t   kExceptionLine,
                              custom_handler_t handler)
{
    KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                       TRACE_EXCEPTION_REGISTER_ENTRY,
                       3,
                       KERNEL_TRACE_HIGH(handler),
                       KERNEL_TRACE_LOW(handler),
                       kExceptionLine);

    if(kExceptionLine < sMinExceptionNumber ||
       kExceptionLine > sMaxExceptionNumber)
    {
        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REGISTER_EXIT,
                           4,
                           KERNEL_TRACE_HIGH(handler),
                           KERNEL_TRACE_LOW(handler),
                           kExceptionLine,
                           OR_ERR_UNAUTHORIZED_INTERRUPT_LINE);

        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    if(handler == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REGISTER_EXIT,
                           4,
                           KERNEL_TRACE_HIGH(handler),
                           KERNEL_TRACE_LOW(handler),
                           kExceptionLine,
                           OS_ERR_NULL_POINTER);

        return OS_ERR_NULL_POINTER;
    }

    KERNEL_CRITICAL_LOCK(kernelInterruptHandlerTableLock);

    if(pKernelInterruptHandlerTable[kExceptionLine] != NULL)
    {
        KERNEL_CRITICAL_UNLOCK(kernelInterruptHandlerTableLock);

        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REGISTER_EXIT,
                           4,
                           KERNEL_TRACE_HIGH(handler),
                           KERNEL_TRACE_LOW(handler),
                           kExceptionLine,
                           OS_ERR_INTERRUPT_ALREADY_REGISTERED);

        return OS_ERR_INTERRUPT_ALREADY_REGISTERED;
    }

    pKernelInterruptHandlerTable[kExceptionLine] = handler;

    KERNEL_DEBUG(EXCEPTIONS_DEBUG_ENABLED,
                 "EXCEPTIONS",
                 "Added exception %u handler at 0x%p",
                 kExceptionLine,
                 handler);

    KERNEL_CRITICAL_UNLOCK(kernelInterruptHandlerTableLock);

        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REGISTER_EXIT,
                           4,
                           KERNEL_TRACE_HIGH(handler),
                           KERNEL_TRACE_LOW(handler),
                           kExceptionLine,
                           OS_NO_ERR);

    return OS_NO_ERR;
}

OS_RETURN_E exceptionRemove(const uint32_t kExceptionLine)
{
    KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                       TRACE_EXCEPTION_REMOVE_ENTRY,
                       1,
                       kExceptionLine);

    if(kExceptionLine < sMinExceptionNumber ||
       kExceptionLine > sMaxExceptionNumber)
    {
        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REMOVE_EXIT,
                           2,
                           kExceptionLine,
                           OR_ERR_UNAUTHORIZED_INTERRUPT_LINE);

        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    KERNEL_CRITICAL_LOCK(kernelInterruptHandlerTableLock);

    if(pKernelInterruptHandlerTable[kExceptionLine] == NULL)
    {
        KERNEL_CRITICAL_UNLOCK(kernelInterruptHandlerTableLock);

        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REMOVE_EXIT,
                           2,
                           kExceptionLine,
                           OS_ERR_INTERRUPT_NOT_REGISTERED);

        return OS_ERR_INTERRUPT_NOT_REGISTERED;
    }

    pKernelInterruptHandlerTable[kExceptionLine] = NULL;

    KERNEL_DEBUG(EXCEPTIONS_DEBUG_ENABLED,
                 "EXCEPTIONS",
                 "Removed exception %u handle",
                 kExceptionLine);

    KERNEL_CRITICAL_UNLOCK(kernelInterruptHandlerTableLock);

    KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                       TRACE_EXCEPTION_REMOVE_EXIT,
                       2,
                       kExceptionLine,
                       OS_NO_ERR);

    return OS_NO_ERR;
}

/************************************ EOF *************************************/