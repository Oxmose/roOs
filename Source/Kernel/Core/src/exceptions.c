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
#include <cpu_interrupt.h> /* CPU interrupts settings */

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
extern custom_handler_t kernelInterruptHandlerTable[INT_ENTRY_COUNT];

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _divByZeroHandler(kernel_thread_t* pCurrThread)
{
    uint32_t intId;

    intId = pCurrThread->vCpu.intContext.intId;



    /* If the exception line is not the divide by zero exception */
    EXC_ASSERT(intId != DIV_BY_ZERO_LINE,
               "Divide by zero invocated with wrong exception line.",
               OS_ERR_INCORRECT_VALUE);

    /* TODO: Kill the process */
    PANIC(OS_ERR_INCORRECT_VALUE, MODULE_NAME, "Div by zero in kernel", TRUE);

}

void exceptionInit(void)
{
    OS_RETURN_E err;



    KERNEL_DEBUG(EXCEPTIONS_DEBUG_ENABLED, "EXCEPTIONS",
                 "Initializing exception manager.");

    err = exceptionRegister(DIV_BY_ZERO_LINE, _divByZeroHandler);
    EXC_ASSERT(err == OS_NO_ERR,
               "Could not initialize exception manager.",
               err);

    TEST_POINT_FUNCTION_CALL(exception_test, TEST_EXCEPTION_ENABLED);


}

OS_RETURN_E exceptionRegister(const uint32_t   kExceptionLine,
                              custom_handler_t handler)
{
    uint32_t intState;

    if((int32_t)kExceptionLine < MIN_EXCEPTION_LINE ||
       kExceptionLine > MAX_EXCEPTION_LINE)
    {
        KERNEL_ERROR("Invalid registered exception line: %d\n", kExceptionLine);


        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    if(handler == NULL)
    {
        KERNEL_ERROR("NULL registered exception handler\n");


        return OS_ERR_NULL_POINTER;
    }

    ENTER_CRITICAL(intState);

    if(kernelInterruptHandlerTable[kExceptionLine] != NULL)
    {
        EXIT_CRITICAL(intState);
        KERNEL_ERROR("Invalid registered exception %d: already registered\n",
                     kExceptionLine);


        return OS_ERR_INTERRUPT_ALREADY_REGISTERED;
    }

    kernelInterruptHandlerTable[kExceptionLine] = handler;

    KERNEL_DEBUG(EXCEPTIONS_DEBUG_ENABLED,
                 "EXCEPTIONS",
                 "Added exception %u handler at 0x%p",
                 kExceptionLine,
                 handler);

    EXIT_CRITICAL(intState);



    return OS_NO_ERR;
}

OS_RETURN_E exceptionRemove(const uint32_t kExceptionLine)
{
    uint32_t intState;



    if((int32_t)kExceptionLine < MIN_EXCEPTION_LINE ||
       kExceptionLine > MAX_EXCEPTION_LINE)
    {
        KERNEL_ERROR("Invalid removed exception line: %d\n", kExceptionLine);


        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    ENTER_CRITICAL(intState);

    if(kernelInterruptHandlerTable[kExceptionLine] == NULL)
    {
        EXIT_CRITICAL(intState);
        KERNEL_ERROR("Exception line not registered: %d\n", kExceptionLine);


        return OS_ERR_INTERRUPT_NOT_REGISTERED;
    }

    kernelInterruptHandlerTable[kExceptionLine] = NULL;

    KERNEL_DEBUG(EXCEPTIONS_DEBUG_ENABLED,
                 "EXCEPTIONS",
                 "Removed exception %u handle",
                 kExceptionLine);



    EXIT_CRITICAL(intState);

    return OS_NO_ERR;
}

/************************************ EOF *************************************/