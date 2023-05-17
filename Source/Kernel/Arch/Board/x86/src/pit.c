/*******************************************************************************
 * @file pit.c
 *
 * @see pit.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 17/05/2023
 *
 * @version 1.0
 *
 * @brief PIT (Programmable interval timer) driver.
 *
 * @details PIT (Programmable interval timer) driver. Used as the basic timer
 * source in the kernel. This driver provides basic access to the PIT.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <time_mgt.h>      /* Time Management */
#include <kerror.h>        /* Kernel Errors */
#include <cpu.h>           /* CPU Management */
#include <cpu_interrupt.h> /* CPU Interrupts Settings */
#include <interrupts.h>    /* Interrupt Manager*/
#include <critical.h>      /* Critical Sections */
#include <kernel_output.h> /* Kernel Outputs */
#include <panic.h>         /* Kernel Panic */

/* Configuration files */
#include <config.h>

/* Header file */
#include <pit.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief PIT CPU command port. */
#define PIT_COMM_PORT     0x34
/** @brief PIT CPU data port. */
#define PIT_DATA_PORT     0x40
/** @brief PIT set tick frequency divider command. */
#define PIT_COMM_SET_FREQ 0x43

/** @brief Base PIT's quartz frequency. */
#define PIT_QUARTZ_FREQ 0x1234DD
/** @brief Kernel's PIT base tick frequency. */
#define PIT_INIT_FREQ   100
/** @brief PIT minimal tick frequency. */
#define PIT_MIN_FREQ    20
/** @brief PIT maximal tick frequency. */
#define PIT_MAX_FREQ    8000

/** @brief Current module name */
#define MODULE_NAME "X86 PIT"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the PIT to ensure correctness of execution.
 *
 * @details Assert macro used by the PIT to ensure correctness of execution.
 * Due to the critical nature of the PIT, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define PIT_ASSERT(COND, MSG, ERROR) {                      \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG, TRUE);               \
    }                                                       \
}

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief Keeps track on the PIT enabled state. */
static uint32_t disabled_nesting;

/** @brief Keeps track of the PIT tick frequency. */
static uint32_t tick_freq;

/** @brief PIT driver instance. */
static kernel_timer_t pit_driver = {
    .get_frequency  = pit_get_frequency,
    .set_frequency  = pit_set_frequency,
    .get_time_ns    = NULL,
    .set_time_ns    = NULL,
    .enable         = pit_enable,
    .disable        = pit_disable,
    .set_handler    = pit_set_handler,
    .remove_handler = pit_remove_handler,
    .get_irq        = pit_get_irq
};

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Initial PIT interrupt handler.
 *
 * @details PIT interrupt handler set at the initialization of the PIT.
 * Dummy routine setting EOI.
 *
 * @param[in, out] curr_thread The current thread at the interrupt.
 */
static void _dummy_handler(kernel_thread_t* curr_thread);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _dummy_handler(kernel_thread_t* curr_thread)
{
    (void)curr_thread;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_DUMMY_HANDLER, 0);

    /* EOI */
    kernel_interrupt_set_irq_eoi(PIT_IRQ_LINE);
}

void pit_init(void)
{
    OS_RETURN_E err;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_INIT_START, 0);

    /* Init system times */
    disabled_nesting = 1;

    /* Set PIT frequency */
    pit_set_frequency(PIT_INIT_FREQ);

    /* Set PIT interrupt handler */
    err = kernel_interrupt_register_irq_handler(PIT_IRQ_LINE, _dummy_handler);
    PIT_ASSERT(err == OS_NO_ERR, "Could not set PIT handler", err);

    KERNEL_DEBUG(PIT_DEBUG_ENABLED, MODULE_NAME, "PIT Initialization end");

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_INIT_END, 1, PIT_INIT_FREQ);

    /* Enable PIT IRQ */
    pit_enable();
}

void pit_enable(void)
{
    uint32_t int_state;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_ENABLE_START, 0);

    ENTER_CRITICAL(int_state);

    if(disabled_nesting > 0)
    {
        --disabled_nesting;
    }

    KERNEL_DEBUG(PIT_DEBUG_ENABLED, MODULE_NAME, "Enable (nesting %d)",
                 disabled_nesting);

    if(disabled_nesting == 0)
    {
        kernel_interrupt_set_irq_mask(PIT_IRQ_LINE, 1);
    }

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_ENABLE_END, 0);

    EXIT_CRITICAL(int_state);
}

void pit_disable(void)
{
    uint32_t int_state;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_DISABLE_START, 1, disabled_nesting);

    ENTER_CRITICAL(int_state);

    if(disabled_nesting < UINT32_MAX)
    {
        ++disabled_nesting;
    }

    KERNEL_DEBUG(PIT_DEBUG_ENABLED, MODULE_NAME, "Disable (nesting %d)",
                 disabled_nesting);
    kernel_interrupt_set_irq_mask(PIT_IRQ_LINE, 0);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_DISABLE_END, 1, disabled_nesting);

    EXIT_CRITICAL(int_state);
}

void pit_set_frequency(const uint32_t freq)
{
    uint32_t int_state;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_SET_FREQ_START, 1, freq);

    PIT_ASSERT((freq >= PIT_MIN_FREQ && freq <= PIT_MAX_FREQ),
               "PIT timer frequency out of bound",
                OS_ERR_INCORRECT_VALUE);

    ENTER_CRITICAL(int_state);

    /* Disable PIT IRQ */
    pit_disable();

    tick_freq  = freq;

    /* Set clock frequency */
    uint16_t tick_freq = (uint16_t)((uint32_t)PIT_QUARTZ_FREQ / freq);
    _cpu_outb(PIT_COMM_SET_FREQ, PIT_COMM_PORT);
    _cpu_outb(tick_freq & 0x00FF, PIT_DATA_PORT);
    _cpu_outb(tick_freq >> 8, PIT_DATA_PORT);

    KERNEL_DEBUG(PIT_DEBUG_ENABLED, MODULE_NAME, "New PIT frequency set (%d)", freq);

    EXIT_CRITICAL(int_state);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_SET_FREQ_END, 1, freq);

    /* Enable PIT IRQ */
    pit_enable();
}

uint32_t pit_get_frequency(void)
{
    return tick_freq;
}

OS_RETURN_E pit_set_handler(void(*handler)(kernel_thread_t*))
{
    OS_RETURN_E err;
    uint32_t    int_state;

#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_SET_HANDLER, 2,
                       (uintptr_t)handler & 0xFFFFFFFF,
                       (uintptr_t)handler >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_SET_HANDLER, 2,
                       (uintptr_t)handler & 0xFFFFFFFF,
                       (uintptr_t)0);
#endif

    if(handler == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    ENTER_CRITICAL(int_state);

    pit_disable();

    /* Remove the current handler */
    err = kernel_interrupt_remove_irq_handler(PIT_IRQ_LINE);
    if(err != OS_NO_ERR)
    {
        EXIT_CRITICAL(int_state);
        pit_enable();
        return err;
    }

    err = kernel_interrupt_register_irq_handler(PIT_IRQ_LINE, handler);
    if(err != OS_NO_ERR)
    {
        EXIT_CRITICAL(int_state);
        pit_enable();
        return err;
    }

    KERNEL_DEBUG(PIT_DEBUG_ENABLED, MODULE_NAME, "New PIT handler set 0x%p", handler);

    EXIT_CRITICAL(int_state);
    pit_enable();

    return err;
}

OS_RETURN_E pit_remove_handler(void)
{
    KERNEL_DEBUG(PIT_DEBUG_ENABLED, MODULE_NAME, "Default PIT handler set 0x%p",
                 _dummy_handler);
#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_REMOVE_HANDLER, 2,
                       (uintptr_t)_dummy_handler & 0xFFFFFFFF,
                       (uintptr_t)_dummy_handler >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_REMOVE_HANDLER, 2,
                       (uintptr_t)_dummy_handler & 0xFFFFFFFF,
                       (uintptr_t)0);
#endif

    return pit_set_handler(_dummy_handler);
}

uint32_t pit_get_irq(void)
{
    return PIT_IRQ_LINE;
}

const kernel_timer_t* pit_get_driver(void)
{
    return &pit_driver;
}

/************************************ EOF *************************************/