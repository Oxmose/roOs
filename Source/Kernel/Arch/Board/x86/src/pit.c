/*******************************************************************************
 * @file pit.c
 *
 * @see pit.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 24/05/2024
 *
 * @version 2.0
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
#include <cpu.h>          /* CPU port manipulation */
#include <panic.h>        /* Kernel panic */
#include <stdint.h>       /* Generic int types */
#include <kerror.h>       /* Kernel error */
#include <critical.h>     /* Kernel critical locks */
#include <time_mgt.h>     /* Timers manager */
#include <drivermgr.h>    /* Driver manager */
#include <interrupts.h>   /* Interrupt manager */
#include <kerneloutput.h> /* Kernel output manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <pit.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for chaining  */
#define PIT_FDT_HASSLAVE_PROP   "is-chained"
/** @brief FDT property for inetrrupt  */
#define PIT_FDT_INT_PROP        "interrupts"
/** @brief FDT property for comm ports */
#define PIT_FDT_COMM_PROP       "comm"
/** @brief FDT property for comm ports */
#define PIT_FDT_QUARTZ_PROP     "qartz-freq"
/** @brief FDT property for frequency */
#define PIT_FDT_SELFREQ_PROP    "freq"
/** @brief FDT property for frequency range */
#define PIT_FDT_FREQRANGE_PROP  "freq-range"
/** @brief FDT property for main timer */
#define PIT_FDT_ISMAIN_PROP     "is-main"

/** @brief PIT set tick frequency divider command. */
#define PIT_COMM_SET_FREQ 0x43

/** @brief Current module name */
#define MODULE_NAME "X86 PIT"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 PIT driver controler. */
typedef struct
{
    /** @brief CPU command port. */
    uint16_t cpuCommPort;

    /** @brief CPU data port. */
    uint16_t cpuDataPort;

    /** @brief PIT IRQ number. */
    uint8_t irqNumber;

    /** @brief Main quarts frequency. */
    uint32_t quartzFrequency;

    /** @brief Selected interrupt frequency. */
    uint32_t selectedFrequency;

    /** @brief Frequency range low. */
    uint32_t frequencyLow;

    /** @brief Frequency range low. */
    uint32_t frequencyHigh;

    /** @brief Keeps track on the PIT enabled state. */
    uint32_t disabledNesting;

} pit_controler_t;

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
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

static OS_RETURN_E _pitAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Initial PIT interrupt handler.
 *
 * @details PIT interrupt handler set at the initialization of the PIT.
 * Dummy routine setting EOI.
 *
 * @param[in, out] curr_thread The current thread at the interrupt.
 */
static void _dummy_handler(kernel_thread_t* curr_thread);

/**
 * @brief Enables PIT ticks.
 *
 * @details Enables PIT ticks by clearing the PIT's IRQ mask.
 */
void pit_enable(void);

/**
 * @brief Disables PIT ticks.
 *
 * @details Disables PIT ticks by setting the PIT's IRQ mask.
 */
void pit_disable(void);

/**
 * @brief Sets the PIT's tick frequency.
 *
 * @details Sets the PIT's tick frequency. The value must be between 20Hz and
 * 8000Hz.
 *
 * @warning The value must be between 20Hz and 8000Hz
 *
 * @param[in] freq The new frequency to be set to the PIT.
 */
void pit_set_frequency(const uint32_t freq);

/**
 * @brief Returns the PIT tick frequency in Hz.
 *
 * @details Returns the PIT tick frequency in Hz.
 *
 * @return The PIT tick frequency in Hz.
 */
uint32_t pit_get_frequency(void);

/**
 * @brief Sets the PIT tick handler.
 *
 * @details Sets the PIT tick handler. This function will be called at each PIT
 * tick received.
 *
 * @param[in] handler The handler of the PIT interrupt.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_NULL_POINTER is returned if the handler is NULL.
  * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the PIT interrupt line
  * is not allowed.
 * - OS_ERR_NULL_POINTER is returned if the pointer to the handler is NULL.
 * - OS_ERR_INTERRUPT_ALREADY_REGISTERED is returned if a handler is already
 * registered for the PIT.
 */
OS_RETURN_E pit_set_handler(void(*handler)(kernel_thread_t*));

/**
 * @brief Removes the PIT tick handler.
 *
 * @details Removes the PIT tick handler.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the PIT interrupt line
 * is not allowed.
 * - OS_ERR_INTERRUPT_NOT_REGISTERED is returned if the PIT line has no handler
 * attached.
 */
OS_RETURN_E pit_remove_handler(void);

/**
 * @brief Returns the PIT IRQ number.
 *
 * @details Returns the PIT IRQ number.
 *
 * @return The PIT IRQ number.
 */
uint32_t pit_get_irq(void);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/** @brief PIT driver instance. */
driver_t x86PITDriver = {
    .pName         = "X86 PIT Driver",
    .pDescription  = "X86 Programable Interval Timer Driver for UTK",
    .pCompatible   = "x86,x86-pit",
    .pVersion      = "2.0",
    .pDriverAttach = _pitAttach
};

/************************** Static global variables ***************************/
/** @brief PIT driver controler instance. */
static pit_controler_t sDrvCtrl = {
    .cpuCommPort       = 0,
    .cpuDataPort       = 0,
    .irqNumber         = 0,
    .quartzFrequency   = 0,
    .selectedFrequency = 0,
    .frequencyLow      = 0,
    .frequencyHigh     = 0,
    .disabledNesting   = 0
};

/** @brief PIT timer driver instance. */
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
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _pitAttach(const fdt_node_t* pkFdtNode)
{
    const uintptr_t* ptrProp;
    size_t           propLen;
    OS_RETURN_E      retCode;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_INIT_START, 0);

    /* Get IRQ lines */
    ptrProp = fdtGetProp(pkFdtNode, PIT_FDT_INT_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t) * 2)
    {
        KERNEL_ERROR("Failed to retreive the IRQ from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.irqNumber = (uint8_t)FDTTOCPU32(*(ptrProp + 1));

    KERNEL_DEBUG(PIT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "IRQ: %d",
                 sDrvCtrl.irqNumber);

    /* Get communication ports */
    ptrProp = fdtGetProp(pkFdtNode, PIT_FDT_COMM_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t) * 2)
    {
        KERNEL_ERROR("Failed to retreive the CPU comm from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.cpuCommPort = (uint16_t)FDTTOCPU32(*ptrProp);
    sDrvCtrl.cpuDataPort = (uint16_t)FDTTOCPU32(*(ptrProp + 1));

    KERNEL_DEBUG(PIT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "COMM: 0x%x | DATA: 0x%x",
                 sDrvCtrl.cpuCommPort,
                 sDrvCtrl.cpuDataPort);

    /* Get quartz frequency */
    ptrProp = fdtGetProp(pkFdtNode, PIT_FDT_QUARTZ_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t))
    {
        KERNEL_ERROR("Failed to retreive the quartz frequency from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.quartzFrequency = FDTTOCPU32(*ptrProp);

    KERNEL_DEBUG(PIT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Quartz Frequency: %dHz",
                 sDrvCtrl.quartzFrequency);

    /* Get selected frequency */
    ptrProp = fdtGetProp(pkFdtNode, PIT_FDT_SELFREQ_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t))
    {
        KERNEL_ERROR("Failed to retreive the selected frequency from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.selectedFrequency = FDTTOCPU32(*ptrProp);

    KERNEL_DEBUG(PIT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Selected Frequency: %dHz",
                 sDrvCtrl.selectedFrequency);

    /* Get the frequency range */
    ptrProp = fdtGetProp(pkFdtNode, PIT_FDT_FREQRANGE_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t) * 2)
    {
        KERNEL_ERROR("Failed to retreive the CPU comm from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.frequencyLow  = (uint32_t)FDTTOCPU32(*ptrProp);
    sDrvCtrl.frequencyHigh = (uint32_t)FDTTOCPU32(*(ptrProp + 1));

    KERNEL_DEBUG(PIT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Frequency Range: %dHz / %dHz",
                 sDrvCtrl.frequencyLow,
                 sDrvCtrl.frequencyHigh);

    /* Check if frequency is within bounds */
    if(sDrvCtrl.frequencyLow > sDrvCtrl.selectedFrequency ||
       sDrvCtrl.frequencyHigh < sDrvCtrl.selectedFrequency)
    {
        KERNEL_ERROR("Selected PIT frequency is not within range.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Init system times */
    sDrvCtrl.disabledNesting = 1;

    /* Set PIT frequency */
    pit_set_frequency(sDrvCtrl.selectedFrequency);

    /* Check if we should register as main timer */
    if(fdtGetProp(pkFdtNode, PIT_FDT_ISMAIN_PROP, &propLen) != NULL)
    {
        retCode = timeMgtAddTimer(&pit_driver, MAIN_TIMER);
        if(retCode != OS_NO_ERR)
        {
            KERNEL_ERROR("Failed to set PIT driver as main timer.\n");
            goto ATTACH_END;
        }
    }
    else
    {
        retCode = timeMgtAddTimer(&pit_driver, AUX_TIMER);
        if(retCode != OS_NO_ERR)
        {
            KERNEL_ERROR("Failed to set PIT driver as main timer.\n");
            goto ATTACH_END;
        }
    }

ATTACH_END:
    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_INIT_END, 2,
                       sDrvCtrl.selectedFrequency,
                       (uintptr_t)retCode);
    KERNEL_DEBUG(PIT_DEBUG_ENABLED, MODULE_NAME, "PIT Initialization end");
    return retCode;
}

static void _dummy_handler(kernel_thread_t* curr_thread)
{
    (void)curr_thread;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_DUMMY_HANDLER, 0);

    /* EOI */
    kernel_interrupt_set_irq_eoi(sDrvCtrl.irqNumber);
}

void pit_enable(void)
{
    uint32_t int_state;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_ENABLE_START, 0);

    ENTER_CRITICAL(int_state);

    if(sDrvCtrl.disabledNesting > 0)
    {
        --sDrvCtrl.disabledNesting;
    }

    KERNEL_DEBUG(PIT_DEBUG_ENABLED, MODULE_NAME, "Enable (nesting %d)",
                 sDrvCtrl.disabledNesting);

    if(sDrvCtrl.disabledNesting == 0)
    {
        kernel_interrupt_set_irq_mask(sDrvCtrl.irqNumber, 1);
    }

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_ENABLE_END, 0);

    EXIT_CRITICAL(int_state);
}

void pit_disable(void)
{
    uint32_t int_state;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_DISABLE_START,
                       1,
                       sDrvCtrl.disabledNesting);

    ENTER_CRITICAL(int_state);

    if(sDrvCtrl.disabledNesting < UINT32_MAX)
    {
        ++sDrvCtrl.disabledNesting;
    }

    KERNEL_DEBUG(PIT_DEBUG_ENABLED, MODULE_NAME, "Disable (nesting %d)",
                 sDrvCtrl.disabledNesting);
    kernel_interrupt_set_irq_mask(sDrvCtrl.irqNumber, 0);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_DISABLE_END,
                       1,
                       sDrvCtrl.disabledNesting);

    EXIT_CRITICAL(int_state);
}

void pit_set_frequency(const uint32_t freq)
{
    uint32_t int_state;
    uint16_t tick_freq;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_SET_FREQ_START, 1, freq);

    if(sDrvCtrl.frequencyLow > freq || sDrvCtrl.frequencyHigh < freq)
    {
        KERNEL_ERROR("Selected PIT frequency is not within range.\n");
        return;
    }

    ENTER_CRITICAL(int_state);

    /* Disable PIT IRQ */
    pit_disable();

    sDrvCtrl.selectedFrequency = freq;

    /* Set clock frequency */
    tick_freq = (uint16_t)(sDrvCtrl.quartzFrequency / freq);
    _cpu_outb(PIT_COMM_SET_FREQ, sDrvCtrl.cpuCommPort);
    _cpu_outb(tick_freq & 0x00FF, sDrvCtrl.cpuDataPort);
    _cpu_outb(tick_freq >> 8, sDrvCtrl.cpuDataPort);

    KERNEL_DEBUG(PIT_DEBUG_ENABLED, MODULE_NAME, "New PIT frequency set (%d)", freq);

    EXIT_CRITICAL(int_state);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIT_SET_FREQ_END, 1, freq);

    /* Enable PIT IRQ */
    pit_enable();
}

uint32_t pit_get_frequency(void)
{
    return sDrvCtrl.selectedFrequency;
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
    err = kernel_interrupt_remove_irq_handler(sDrvCtrl.irqNumber);
    if(err != OS_NO_ERR && err != OS_ERR_INTERRUPT_NOT_REGISTERED)
    {
        EXIT_CRITICAL(int_state);
        KERNEL_ERROR("Failed to remove PIT irqHandler. Error: %d\n", err);
        pit_enable();
        return err;
    }

    err = kernel_interrupt_register_irq_handler(sDrvCtrl.irqNumber, handler);
    if(err != OS_NO_ERR)
    {
        EXIT_CRITICAL(int_state);
        KERNEL_ERROR("Failed to register PIT irqHandler. Error: %d\n", err);
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
    return sDrvCtrl.irqNumber;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG(x86PITDriver);

/************************************ EOF *************************************/