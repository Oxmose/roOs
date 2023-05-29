/*******************************************************************************
 * @file interrupts.c
 *
 * @see interrupts.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 31/03/2023
 *
 * @version 3.0
 *
 * @brief Interrupt manager.
 *
 * @details Interrupt manager. Allows to attach ISR to interrupt lines and
 * manage IRQ used by the CPU. We also define the general interrupt handler
 * here.
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
#include <kernel_output.h>      /* Kernel output methods */
#include <critical.h>           /* Critical sections */
#include <scheduler.h>          /* Kernel scheduler */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <interrupts.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module's name */
#define MODULE_NAME "INTERRUPTS"

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
/** @brief Stores the handlers for each interrupt. not static because used
 * by the exceptions module.
 */
custom_handler_t kernel_interrupt_handlers[INT_ENTRY_COUNT];

/************************** Static global variables ***************************/
/** @brief The current interrupt driver to be used by the kernel. */
static interrupt_driver_t interrupt_driver;

/** @brief Stores the number of spurious interrupts since the initialization of
 * the kernel.
 */
static uint64_t spurious_interrupt_count;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Initial placeholder for the IRQ mask driver.
 *
 * @param irq_number Unused.
 * @param enabled Unused.
 */
static void _init_driver_set_irq_mask(const uint32_t irq_number,
                                      const bool_t enabled);

/**
 * @brief Initial placeholder for the IRQ EOI driver.
 *
 * @param irq_number Unused.
 */
static void _init_driver_set_irq_eoi(const uint32_t irq_number);

/**
 * @brief Initial placeholder for the spurious handler driver.
 *
 * @param int_number Unused.
 */
static INTERRUPT_TYPE_E _init_driver_handle_spurious(const uint32_t int_number);

/**
 * @brief Initial placeholder for the get int line driver.
 *
 * @param irq_number Unused.
 */
static int32_t _init_driver_get_irq_int_line(const uint32_t irq_number);

/**
 * @brief Kernel's spurious interrupt handler.
 *
 * @details Spurious interrupt handler. This function should only be
 * called by an assembly interrupt handler. The function will handle spurious
 * interrupts.
 */
static void _spurious_handler(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _init_driver_set_irq_mask(const uint32_t irq_number,
                                     const bool_t enabled)
{
    (void)irq_number;
    (void)enabled;
}

static void _init_driver_set_irq_eoi(const uint32_t irq_number)
{
    (void)irq_number;
}

static INTERRUPT_TYPE_E _init_driver_handle_spurious(const uint32_t int_number)
{
    (void)int_number;
    return INTERRUPT_TYPE_REGULAR;
}

static int32_t _init_driver_get_irq_int_line(const uint32_t irq_number)
{
    (void)irq_number;
    return 0;
}

static void _spurious_handler(void)
{
    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME,
                 "Spurious interrupt %d", spurious_interrupt_count);

    ++spurious_interrupt_count;

    return;
}

void kernel_interrupt_handler(void)
{
    custom_handler_t handler;
    kernel_thread_t* current_thread;
    uint32_t         int_id;


    /* Get the current thread */
    current_thread = scheduler_get_current_thread();
    int_id         = current_thread->v_cpu.int_context.int_id;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_HANDLER_START, 2,
                       int_id, current_thread->tid);

    /* If interrupts are disabled */
    if(_cpu_get_saved_interrupt_state(&current_thread->v_cpu) == 0 &&
       int_id != PANIC_INT_LINE &&
       int_id != SCHEDULER_SW_INT_LINE &&
       int_id >= MIN_INTERRUPT_LINE)
    {
        KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME,
                     "Blocked interrupt %u", int_id);
        return;
    }

    if(int_id == PANIC_INT_LINE)
    {
        panic_handler(current_thread);
    }

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME, "Int %d", int_id);

    /* Check for spurious interrupt */
    if(interrupt_driver.driver_handle_spurious(int_id) ==
       INTERRUPT_TYPE_SPURIOUS)
    {
        _spurious_handler();
        return;
    }

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME, "Non spurious %d",
                 int_id);

    /* Select custom handlers */
    if(int_id < INT_ENTRY_COUNT &&
       kernel_interrupt_handlers[int_id] != NULL)
    {
        handler = kernel_interrupt_handlers[int_id];
    }
    else
    {
        handler = panic_handler;
    }

    /* Execute the handler */
    handler(current_thread);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_HANDLER_END, 2,
                       int_id, current_thread->tid);
}

void kernel_interrupt_init(void)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_INIT_START, 0);
    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME,
                 "Initializing interrupt manager.");

    /* Blank custom interrupt handlers */
    memset(kernel_interrupt_handlers,
           0,
           sizeof(custom_handler_t) * INT_ENTRY_COUNT);

    /* Attach the special PANIC interrupt for when we don't know what to do */
    kernel_interrupt_handlers[PANIC_INT_LINE] = panic_handler;

    /* Init state */
    kernel_interrupt_disable();
    spurious_interrupt_count = 0;

    /* Init driver */
    interrupt_driver.driver_get_irq_int_line = _init_driver_get_irq_int_line;
    interrupt_driver.driver_handle_spurious  = _init_driver_handle_spurious;
    interrupt_driver.driver_set_irq_eoi      = _init_driver_set_irq_eoi;
    interrupt_driver.driver_set_irq_mask     = _init_driver_set_irq_mask;

    TEST_POINT_FUNCTION_CALL(interrupt_test, TEST_INTERRUPT_ENABLED);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_INIT_END, 0);
}

OS_RETURN_E kernel_interrupt_set_driver(const interrupt_driver_t* driver)
{
    uint32_t int_state;

#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_SET_DRIVER_START, 2,
                       (uintptr_t)driver & 0xFFFFFFFF,
                       (uintptr_t)driver >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_SET_DRIVER_START, 2,
                       (uintptr_t)driver & 0xFFFFFFFF,
                       (uintptr_t)0);
#endif


    if(driver == NULL ||
       driver->driver_set_irq_eoi == NULL ||
       driver->driver_set_irq_mask == NULL ||
       driver->driver_handle_spurious == NULL ||
       driver->driver_get_irq_int_line == NULL)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_SET_DRIVER_END, 1,
                           OS_ERR_NULL_POINTER);

        return OS_ERR_NULL_POINTER;
    }

    ENTER_CRITICAL(int_state);

    interrupt_driver = *driver;

    EXIT_CRITICAL(int_state);

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME,
                 "Set new interrupt driver at 0x%p.", driver);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_SET_DRIVER_END, 1, OS_NO_ERR);

    return OS_NO_ERR;
}

OS_RETURN_E kernel_interrupt_register_int_handler(const uint32_t interrupt_line,
                                                  custom_handler_t handler)
{
    uint32_t int_state;

#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_REGISTER_START, 3,
                       interrupt_line,
                       (uintptr_t)handler & 0xFFFFFFFF,
                       (uintptr_t)handler >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_REGISTER_START, 3,
                       interrupt_line,
                       (uintptr_t)handler & 0xFFFFFFFF,
                       (uintptr_t)0);
#endif

    if(interrupt_line < MIN_INTERRUPT_LINE ||
       interrupt_line > MAX_INTERRUPT_LINE)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_REGISTER_END, 2,
                           interrupt_line,
                           OR_ERR_UNAUTHORIZED_INTERRUPT_LINE);

        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    if(handler == NULL)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_REGISTER_END, 2,
                           interrupt_line,
                           OS_ERR_NULL_POINTER);

        return OS_ERR_NULL_POINTER;
    }

    ENTER_CRITICAL(int_state);

    if(kernel_interrupt_handlers[interrupt_line] != NULL)
    {
        EXIT_CRITICAL(int_state);
        KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_REGISTER_END, 2,
                           interrupt_line,
                           OS_ERR_INTERRUPT_ALREADY_REGISTERED);

        return OS_ERR_INTERRUPT_ALREADY_REGISTERED;
    }

    kernel_interrupt_handlers[interrupt_line] = handler;

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME,
                 "Added INT %u handler at 0x%p", interrupt_line, handler);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_REGISTER_END, 2,
                       interrupt_line,
                       OS_NO_ERR);

    EXIT_CRITICAL(int_state);

    return OS_NO_ERR;
}

OS_RETURN_E kernel_interrupt_remove_int_handler(const uint32_t interrupt_line)
{
    uint32_t int_state;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_REMOVE_START, 1,
                       interrupt_line);

    if(interrupt_line < MIN_INTERRUPT_LINE ||
       interrupt_line > MAX_INTERRUPT_LINE)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_REMOVE_END, 2,
                           interrupt_line,
                           OR_ERR_UNAUTHORIZED_INTERRUPT_LINE);

        return OR_ERR_UNAUTHORIZED_INTERRUPT_LINE;
    }

    ENTER_CRITICAL(int_state);

    if(kernel_interrupt_handlers[interrupt_line] == NULL)
    {
        EXIT_CRITICAL(int_state);

        KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_REMOVE_END, 2,
                           interrupt_line,
                           OS_ERR_INTERRUPT_NOT_REGISTERED);

        return OS_ERR_INTERRUPT_NOT_REGISTERED;
    }

    kernel_interrupt_handlers[interrupt_line] = NULL;

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME,
                 "Removed interrupt %u handle", interrupt_line);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_INTERRUPT_REMOVE_END, 2,
                       interrupt_line,
                       OS_NO_ERR);

    EXIT_CRITICAL(int_state);

    return OS_NO_ERR;
}

OS_RETURN_E kernel_interrupt_register_irq_handler(const uint32_t irq_number,
                                                  custom_handler_t handler)
{
    int32_t     int_line;
    OS_RETURN_E ret_code;

#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_IRQ_REGISTER_START, 3,
                       irq_number,
                       (uintptr_t)handler & 0xFFFFFFFF,
                       (uintptr_t)handler >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_IRQ_REGISTER_START, 3,
                       irq_number,
                       (uintptr_t)handler & 0xFFFFFFFF,
                       (uintptr_t)0);
#endif

    /* Get the interrupt line attached to the IRQ number. */
    int_line = interrupt_driver.driver_get_irq_int_line(irq_number);

    if(int_line < 0)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_IRQ_REGISTER_END, 1,
                           OS_ERR_NO_SUCH_IRQ);
        return OS_ERR_NO_SUCH_IRQ;
    }

    ret_code = kernel_interrupt_register_int_handler(int_line, handler);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_IRQ_REGISTER_END, 1, ret_code);

    return ret_code;
}

OS_RETURN_E kernel_interrupt_remove_irq_handler(const uint32_t irq_number)
{
    int32_t     int_line;
    OS_RETURN_E ret_code;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_IRQ_REMOVE_START, 1, irq_number);

    /* Get the interrupt line attached to the IRQ number. */
    int_line = interrupt_driver.driver_get_irq_int_line(irq_number);

    if(int_line < 0)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_IRQ_REMOVE_END, 2,
                           irq_number,
                           OS_ERR_NO_SUCH_IRQ);

        return OS_ERR_NO_SUCH_IRQ;
    }

    ret_code = kernel_interrupt_remove_int_handler(int_line);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_IRQ_REMOVE_END, 2, irq_number, ret_code);

    return ret_code;
}

void kernel_interrupt_restore(const uint32_t prev_state)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_RESTORE_INTERRUPT, 1, prev_state);
    if(prev_state != 0)
    {
        KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME, "Enabled HW INT");
        _cpu_set_interrupt();
    }
}

uint32_t kernel_interrupt_disable(void)
{
    uint32_t prev_state;

    prev_state = _cpu_get_interrupt_state();

    KERNEL_TRACE_EVENT(EVENT_KERNEL_DISABLE_INTERRUPT, 1, prev_state);

    if(prev_state == 0)
    {
        return 0;
    }

    _cpu_clear_interrupt();

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME, "Disabled HW INT");

    return prev_state;
}

void kernel_interrupt_set_irq_mask(const uint32_t irq_number,
                                   const uint32_t enabled)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_SET_IRQ_MASK, 2, irq_number, enabled);

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED, MODULE_NAME,
                 "IRQ Mask change: %u %u", irq_number, enabled);

    interrupt_driver.driver_set_irq_mask(irq_number, enabled);
}

void kernel_interrupt_set_irq_eoi(const uint32_t irq_number)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_SET_IRQ_EOI, 1, irq_number);

    KERNEL_DEBUG(INTERRUPTS_DEBUG_ENABLED,  "INTERRUPTS", "IRQ EOI: %u",
                 irq_number);

    interrupt_driver.driver_set_irq_eoi(irq_number);
}

/************************************ EOF *************************************/