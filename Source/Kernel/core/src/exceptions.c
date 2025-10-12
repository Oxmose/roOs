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
#include <syslog.h>        /* Kernel Syslog */
#include <critical.h>      /* Critical sections */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <exceptions.h>

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
    if((COND) == false)                                     \
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
extern kernel_spinlock_t sKernelInterruptHandlerTableLock;

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

#if EXCEPTIONS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Initializing exception manager.");
#endif

    /* Get the CPU exception bounds */
    kpCpuIntConfig = cpuGetInterruptConfig();
    sMinExceptionNumber = kpCpuIntConfig->minExceptionLine;
    sMaxExceptionNumber = kpCpuIntConfig->maxExceptionLine;

    err = cpuRegisterExceptions();
    EXC_ASSERT(err == OS_NO_ERR,
               "Could not initialize exception manager.",
               err);
}

OS_RETURN_E exceptionRegister(const uint32_t   kExceptionLine,
                              custom_handler_t handler)
{

    if(kExceptionLine < sMinExceptionNumber ||
       kExceptionLine > sMaxExceptionNumber)
    {

        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    if(handler == NULL)
    {

        return OS_ERR_NULL_POINTER;
    }

    KERNEL_LOCK(sKernelInterruptHandlerTableLock);

    if(pKernelInterruptHandlerTable[kExceptionLine] != NULL)
    {
        KERNEL_UNLOCK(sKernelInterruptHandlerTableLock);

        return OS_ERR_ALREADY_EXIST;
    }

    pKernelInterruptHandlerTable[kExceptionLine] = handler;

#if EXCEPTIONS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Added exception %d handler at 0x%p",
           kExceptionLine,
           handler);
#endif

    KERNEL_UNLOCK(sKernelInterruptHandlerTableLock);

    return OS_NO_ERR;
}

OS_RETURN_E exceptionRemove(const uint32_t kExceptionLine)
{

    if(kExceptionLine < sMinExceptionNumber ||
       kExceptionLine > sMaxExceptionNumber)
    {

        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    KERNEL_LOCK(sKernelInterruptHandlerTableLock);

    if(pKernelInterruptHandlerTable[kExceptionLine] == NULL)
    {
        KERNEL_UNLOCK(sKernelInterruptHandlerTableLock);

        return OS_ERR_NO_SUCH_ID;
    }

    pKernelInterruptHandlerTable[kExceptionLine] = NULL;

#if EXCEPTIONS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Removed exception %d handle",
           kExceptionLine);
#endif

    KERNEL_UNLOCK(sKernelInterruptHandlerTableLock);

    return OS_NO_ERR;
}

/************************************ EOF *************************************/