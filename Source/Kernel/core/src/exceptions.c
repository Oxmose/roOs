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
#include <cpu.h>           /* CPU management */
#include <panic.h>         /* Kernel panic */
#include <stdint.h>        /* Generic int types */
#include <stddef.h>        /* Standard definitions */
#include <string.h>        /* String manipulation */
#include <critical.h>      /* Critical sections */
#include <scheduler.h>     /* Kernel scheduler */
#include <kerneloutput.h>  /* Kernel output methods */

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
        PANIC(ERROR, MODULE_NAME, MSG, TRUE);               \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Handle a division by zero exception.
 *
 * @details Handles a divide by zero exception raised by the cpu. The thread
 * will just be killed.
 *
 * @param[in, out] pCurrThread The current thread at the moment of the division
 * by zero.
 */
static void _divByZeroHandler(kernel_thread_t* pCurrThread);

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

static void _divByZeroHandler(kernel_thread_t* pCurrThread)
{
    KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                       TRACE_EXCEPTION_DIVBYZERO,
                       (uint32_t)pCurrThread->tid);

    /* TODO: Kill the process */
    (void)pCurrThread;
    PANIC(OS_ERR_INCORRECT_VALUE, MODULE_NAME, "Div by zero in kernel", TRUE);
}

void exceptionInit(void)
{
    OS_RETURN_E                   err;
    const cpu_interrupt_config_t* pCpuIntConfig;

    KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED, TRACE_EXCEPTION_INIT_ENTRY, 0);

    KERNEL_DEBUG(EXCEPTIONS_DEBUG_ENABLED, "EXCEPTIONS",
                 "Initializing exception manager.");

    /* Get the CPU exception bounds */
    pCpuIntConfig = cpuGetInterruptConfig();
    sMinExceptionNumber = pCpuIntConfig->minExceptionLine;
    sMaxExceptionNumber = pCpuIntConfig->maxExceptionLine;

    /* Register the division by zero handler */
    err = exceptionRegister(DIV_BY_ZERO_LINE, _divByZeroHandler);
    EXC_ASSERT(err == OS_NO_ERR,
               "Could not initialize exception manager.",
               err);

    TEST_POINT_FUNCTION_CALL(exception_test, TEST_EXCEPTION_ENABLED);

    KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED, TRACE_EXCEPTION_INIT_EXIT, 0);
}

OS_RETURN_E exceptionRegister(const uint32_t   kExceptionLine,
                              custom_handler_t handler)
{
#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                       TRACE_EXCEPTION_REGISTER_ENTRY,
                       3,
                       0,
                       (uint32_t)handler,
                       kExceptionLine);
#else
    KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                       TRACE_EXCEPTION_REGISTER_ENTRY,
                       3,
                       (uint32_t)((uintptr_t)handler >> 32),
                       (uint32_t)(uintptr_t)handler,
                       kExceptionLine);
#endif

    if(kExceptionLine < sMinExceptionNumber ||
       kExceptionLine > sMaxExceptionNumber)
    {
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REGISTER_EXIT,
                           4,
                           0,
                           (uint32_t)handler,
                           kExceptionLine,
                           OR_ERR_UNAUTHORIZED_INTERRUPT_LINE);
#else
        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REGISTER_EXIT,
                           4,
                           (uint32_t)((uintptr_t)handler >> 32),
                           (uint32_t)(uintptr_t)handler,
                           kExceptionLine,
                           OR_ERR_UNAUTHORIZED_INTERRUPT_LINE);
#endif

        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    if(handler == NULL)
    {
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REGISTER_EXIT,
                           4,
                           0,
                           (uint32_t)handler,
                           kExceptionLine,
                           OS_ERR_NULL_POINTER);
#else
        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REGISTER_EXIT,
                           4,
                           (uint32_t)((uintptr_t)handler >> 32),
                           (uint32_t)(uintptr_t)handler,
                           kExceptionLine,
                           OS_ERR_NULL_POINTER);
#endif

        return OS_ERR_NULL_POINTER;
    }

    KERNEL_CRITICAL_LOCK(kernelInterruptHandlerTableLock);

    if(pKernelInterruptHandlerTable[kExceptionLine] != NULL)
    {
        KERNEL_CRITICAL_UNLOCK(kernelInterruptHandlerTableLock);
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REGISTER_EXIT,
                           4,
                           0,
                           (uint32_t)handler,
                           kExceptionLine,
                           OS_ERR_INTERRUPT_ALREADY_REGISTERED);
#else
        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REGISTER_EXIT,
                           4,
                           (uint32_t)((uintptr_t)handler >> 32),
                           (uint32_t)(uintptr_t)handler,
                           kExceptionLine,
                           OS_ERR_INTERRUPT_ALREADY_REGISTERED);
#endif

        return OS_ERR_INTERRUPT_ALREADY_REGISTERED;
    }

    pKernelInterruptHandlerTable[kExceptionLine] = handler;

    KERNEL_DEBUG(EXCEPTIONS_DEBUG_ENABLED,
                 "EXCEPTIONS",
                 "Added exception %u handler at 0x%p",
                 kExceptionLine,
                 handler);

    KERNEL_CRITICAL_UNLOCK(kernelInterruptHandlerTableLock);

#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REGISTER_EXIT,
                           4,
                           0,
                           (uint32_t)handler,
                           kExceptionLine,
                           OS_NO_ERR);
#else
        KERNEL_TRACE_EVENT(TRACE_EXCEPTION_ENABLED,
                           TRACE_EXCEPTION_REGISTER_EXIT,
                           4,
                           (uint32_t)((uintptr_t)handler >> 32),
                           (uint32_t)(uintptr_t)handler,
                           kExceptionLine,
                           OS_NO_ERR);
#endif

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