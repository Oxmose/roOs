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
#include <stdint.h>             /* Generic int types */
#include <stddef.h>             /* Standard definitions */
#include <string.h>             /* String manipulation */
#include <cpu.h>                /* CPU management */
#include <cpu_interrupt.h>      /* CPU interrupts settings */
#include <panic.h>              /* Kernel panic */
#include <kerneloutput.h>      /* Kernel output methods */
#include <critical.h>           /* Critical sections */
#include <scheduler.h>          /* Kernel scheduler */

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
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Stores the handlers for each exception, defined in exceptions.h */
extern custom_handler_t kernel_interrupt_handlers[INT_ENTRY_COUNT];

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Handle a division by zero exception.
 *
 * @details Handles a divide by zero exception raised by the cpu. The thread
 * will just be killed.
 *
 * @param[in-out] current_thread The current thread at the moment of the division
 * by zero.
 */
static void _div_by_zero_handler(kernel_thread_t* current_thread);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _div_by_zero_handler(kernel_thread_t* current_thread)
{
    uint32_t int_id;

    int_id = current_thread->v_cpu.int_context.int_id;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_DIV_BY_ZERO, 1, current_thread->tid);

    /* If the exception line is not the divide by zero exception */
    EXC_ASSERT(int_id != DIV_BY_ZERO_LINE,
               "Divide by zero invocated with wrong exception line.",
               OS_ERR_INCORRECT_VALUE);

    /* TODO: Kill the process */
    PANIC(OS_ERR_INCORRECT_VALUE, MODULE_NAME, "Div by zero in kernel", TRUE);

}

void kernel_exception_init(void)
{
    OS_RETURN_E err;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_EXCEPTION_INIT_START, 0);

    KERNEL_DEBUG(EXCEPTIONS_DEBUG_ENABLED, "EXCEPTIONS",
                 "Initializing exception manager.");

    err = kernel_exception_register_handler(DIV_BY_ZERO_LINE,
                                            _div_by_zero_handler);
    EXC_ASSERT(err == OS_NO_ERR,
               "Could not initialize exception manager.",
               err);

    TEST_POINT_FUNCTION_CALL(exception_test, TEST_EXCEPTION_ENABLED);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_EXCEPTION_INIT_END, 0);
}

OS_RETURN_E kernel_exception_register_handler(const uint32_t exception_line,
                                              custom_handler_t handler)
{
    uint32_t int_state;

#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_EXCEPTION_REGISTER_START, 3,
                       exception_line,
                       (uintptr_t)handler & 0xFFFFFFFF,
                       (uintptr_t)handler >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_EXCEPTION_REGISTER_START, 3,
                       exception_line,
                       (uintptr_t)handler & 0xFFFFFFFF,
                       (uintptr_t)0);
#endif

    if((int32_t)exception_line < MIN_EXCEPTION_LINE ||
       exception_line > MAX_EXCEPTION_LINE)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_EXCEPTION_REGISTER_END, 2,
                           exception_line,
                           OR_ERR_UNAUTHORIZED_INTERRUPT_LINE);

        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    if(handler == NULL)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_EXCEPTION_REGISTER_END, 2,
                           exception_line,
                           OS_ERR_NULL_POINTER);

        return OS_ERR_NULL_POINTER;
    }

    ENTER_CRITICAL(int_state);

    if(kernel_interrupt_handlers[exception_line] != NULL)
    {
        EXIT_CRITICAL(int_state);
        KERNEL_TRACE_EVENT(EVENT_KERNEL_EXCEPTION_REGISTER_END, 2,
                           exception_line,
                           OS_ERR_INTERRUPT_ALREADY_REGISTERED);

        return OS_ERR_INTERRUPT_ALREADY_REGISTERED;
    }

    kernel_interrupt_handlers[exception_line] = handler;

    KERNEL_DEBUG(EXCEPTIONS_DEBUG_ENABLED, "EXCEPTIONS",
                 "Added exception %u handler at 0x%p", exception_line, handler);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_EXCEPTION_REGISTER_END, 2,
                       exception_line,
                       OS_NO_ERR);

    EXIT_CRITICAL(int_state);
    return OS_NO_ERR;
}

OS_RETURN_E kernel_exception_remove_handler(const uint32_t exception_line)
{
    uint32_t int_state;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_EXCEPTION_REMOVE_START, 1,
                       exception_line);

    if((int32_t)exception_line < MIN_EXCEPTION_LINE ||
       exception_line > MAX_EXCEPTION_LINE)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_EXCEPTION_REMOVE_END, 2,
                           exception_line,
                           OR_ERR_UNAUTHORIZED_INTERRUPT_LINE);

        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    ENTER_CRITICAL(int_state);

    if(kernel_interrupt_handlers[exception_line] == NULL)
    {
        EXIT_CRITICAL(int_state);
        KERNEL_TRACE_EVENT(EVENT_KERNEL_EXCEPTION_REMOVE_END, 2,
                           exception_line,
                           OS_ERR_INTERRUPT_NOT_REGISTERED);

        return OS_ERR_INTERRUPT_NOT_REGISTERED;
    }

    kernel_interrupt_handlers[exception_line] = NULL;

    KERNEL_DEBUG(EXCEPTIONS_DEBUG_ENABLED, "EXCEPTIONS",
                 "Removed exception %u handle", exception_line);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_EXCEPTION_REMOVE_END, 2,
                       exception_line,
                       OS_NO_ERR);

    EXIT_CRITICAL(int_state);

    return OS_NO_ERR;
}

/************************************ EOF *************************************/