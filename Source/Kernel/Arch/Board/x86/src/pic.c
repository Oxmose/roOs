/*******************************************************************************
 * @file pic.c
 *
 * @see pic.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 17/05/2023
 *
 * @version 1.0
 *
 * @brief PIC (programmable interrupt controler) driver.
 *
 * @details PIC (programmable interrupt controler) driver. Allows to remmap
 * the PIC IRQ, set the IRQs mask and manage EoI for the X86 PIC.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <cpu_interrupt.h> /* Interrupt management */
#include <kernel_output.h> /* Kernel output methods */
#include <stdint.h>        /* Generic int types */
#include <cpu.h>           /* CPU management */
#include <interrupts.h>    /* Interrupts manager */
#include <panic.h>         /* Kernel Panic */
#include <critical.h>      /* Critical Sections */

/* Configuration files */
#include <config.h>

/* Header file */
#include <pic.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Master PIC CPU command port. */
#define PIC_MASTER_COMM_PORT 0x20
/** @brief Master PIC CPU data port. */
#define PIC_MASTER_DATA_PORT 0x21
/** @brief Slave PIC CPU command port. */
#define PIC_SLAVE_COMM_PORT  0xA0
/** @brief Slave PIC CPU data port. */
#define PIC_SLAVE_DATA_PORT  0xA1

/** @brief PIC End of Interrupt command. */
#define PIC_EOI 0x20

/** @brief PIC ICW4 needed flag. */
#define PIC_ICW1_ICW4      0x01
/** @brief PIC single mode flag. */
#define PIC_ICW1_SINGLE    0x02
/** @brief PIC call address interval 4 flag. */
#define PIC_ICW1_INTERVAL4 0x04
/** @brief PIC trigger level flag. */
#define PIC_ICW1_LEVEL     0x08
/** @brief PIC initialization flag. */
#define PIC_ICW1_INIT      0x10

/** @brief PIC 8086/88 (MCS-80/85) mode flag. */
#define PIC_ICW4_8086        0x01
/** @brief PIC auto (normal) EOI flag. */
#define PIC_ICW4_AUTO        0x02
/** @brief PIC buffered mode/slave flag. */
#define PIC_ICW4_BUF_SLAVE   0x08
/** @brief PIC buffered mode/master flag. */
#define PIC_ICW4_BUF_MASTER  0x0C
/** @brief PIC special fully nested (not) flag. */
#define PIC_ICW4_SFNM        0x10

/** @brief Read ISR command value */
#define PIC_READ_ISR 0x0B

/** @brief Master PIC Base interrupt line for the lowest IRQ. */
#define PIC0_BASE_INTERRUPT_LINE INT_PIC_IRQ_OFFSET
/** @brief Slave PIC Base interrupt line for the lowest IRQ. */
#define PIC1_BASE_INTERRUPT_LINE (INT_PIC_IRQ_OFFSET + 8)

/** @brief PIC's cascading IRQ number. */
#define PIC_CASCADING_IRQ 2

/** @brief The PIC spurious irq mask. */
#define PIC_SPURIOUS_IRQ_MASK 0x80

/** @brief Master PIC spurious IRQ number. */
#define PIC_SPURIOUS_IRQ_MASTER 0x07
/** @brief Slave PIC spurious IRQ number. */
#define PIC_SPURIOUS_IRQ_SLAVE  0x0F

/** @brief Current module name */
#define MODULE_NAME "X86 PIC"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the PIC to ensure correctness of execution.
 *
 * @details Assert macro used by the PIC to ensure correctness of execution.
 * Due to the critical nature of the PIC, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define PIC_ASSERT(COND, MSG, ERROR) {                      \
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
/** @brief PIC driver instance. */
static interrupt_driver_t pic_driver = {
    .driver_set_irq_mask     = pic_set_irq_mask,
    .driver_set_irq_eoi      = pic_set_irq_eoi,
    .driver_handle_spurious  = pic_handle_spurious_irq,
    .driver_get_irq_int_line = pic_get_irq_int_line
};

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void pic_init(void)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_INIT_START, 0);

    /* Initialize the master, remap IRQs */
    _cpu_outb(PIC_ICW1_ICW4 | PIC_ICW1_INIT, PIC_MASTER_COMM_PORT);
    _cpu_outb(PIC0_BASE_INTERRUPT_LINE, PIC_MASTER_DATA_PORT);
    _cpu_outb(0x4,  PIC_MASTER_DATA_PORT);
    _cpu_outb(PIC_ICW4_8086,  PIC_MASTER_DATA_PORT);

    /* Initialize the slave, remap IRQs */
    _cpu_outb(PIC_ICW1_ICW4 | PIC_ICW1_INIT, PIC_SLAVE_COMM_PORT);
    _cpu_outb(PIC1_BASE_INTERRUPT_LINE, PIC_SLAVE_DATA_PORT);
    _cpu_outb(0x2,  PIC_SLAVE_DATA_PORT);
    _cpu_outb(PIC_ICW4_8086,  PIC_SLAVE_DATA_PORT);

    /* Set EOI for both PICs. */
    _cpu_outb(PIC_EOI, PIC_MASTER_COMM_PORT);
    _cpu_outb(PIC_EOI, PIC_SLAVE_COMM_PORT);

    /* Disable all IRQs */
    _cpu_outb(0xFF, PIC_MASTER_DATA_PORT);
    _cpu_outb(0xFF, PIC_SLAVE_DATA_PORT);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_INIT_END, 0);

    KERNEL_DEBUG(PIC_DEBUG_ENABLED, MODULE_NAME, "PIC Initialization end");
}

void pic_set_irq_mask(const uint32_t irq_number, const bool_t enabled)
{
    uint8_t  init_mask;
    uint32_t int_state;
    uint32_t cascading_number;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SET_IRQ_MASK_START,
                       2,
                       irq_number,
                       enabled);

    PIC_ASSERT(irq_number <= PIC_MAX_IRQ_LINE,
               "Could not find PIC IRQ",
               OS_ERR_NO_SUCH_IRQ);

    ENTER_CRITICAL(int_state);

    /* Manage master PIC */
    if(irq_number < 8)
    {
        /* Retrieve initial mask */
        init_mask = _cpu_inb(PIC_MASTER_DATA_PORT);

        /* Set new mask value */
        if(enabled == FALSE)
        {
            init_mask |= 1 << irq_number;
        }
        else
        {
            init_mask &= ~(1 << irq_number);
        }

        /* Set new mask */
        _cpu_outb(init_mask, PIC_MASTER_DATA_PORT);
    }

    /* Manage slave PIC. WARNING, cascading will be enabled */
    if(irq_number > 7)
    {
        /* Set new IRQ number */
        cascading_number = irq_number - 8;

        /* Retrieve initial mask */
        init_mask = _cpu_inb(PIC_MASTER_DATA_PORT);

        /* Set new mask value */
        init_mask &= ~(1 << PIC_CASCADING_IRQ);

        /* Set new mask */
        _cpu_outb(init_mask, PIC_MASTER_DATA_PORT);

        /* Retrieve initial mask */
        init_mask = _cpu_inb(PIC_SLAVE_DATA_PORT);

        /* Set new mask value */
        if(enabled == FALSE)
        {
            init_mask |= 1 << cascading_number;
        }
        else
        {
            init_mask &= ~(1 << cascading_number);
        }

        /* Set new mask */
        _cpu_outb(init_mask, PIC_SLAVE_DATA_PORT);

        /* If all is masked then disable cascading */
        if(init_mask == 0xFF)
        {
            /* Retrieve initial mask */
            init_mask = _cpu_inb(PIC_MASTER_DATA_PORT);

            /* Set new mask value */
            init_mask  |= 1 << PIC_CASCADING_IRQ;

            /* Set new mask */
            _cpu_outb(init_mask, PIC_MASTER_DATA_PORT);
        }
    }

    KERNEL_DEBUG(PIC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "New PIC Mask M: 0x%02x S: 0x%02x",
                 _cpu_inb(PIC_MASTER_DATA_PORT),
                 _cpu_inb(PIC_SLAVE_DATA_PORT));

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SET_IRQ_MASK_END,
                       2,
                       irq_number,
                       enabled);

    EXIT_CRITICAL(int_state);
}

void pic_set_irq_eoi(const uint32_t irq_number)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_EOI_START, 1, irq_number);

    PIC_ASSERT(irq_number <= PIC_MAX_IRQ_LINE,
               "Could not find PIC IRQ",
               OS_ERR_NO_SUCH_IRQ);

    /* End of interrupt signal */
    if(irq_number > 7)
    {
        _cpu_outb(PIC_EOI, PIC_SLAVE_COMM_PORT);
    }
    _cpu_outb(PIC_EOI, PIC_MASTER_COMM_PORT);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_EOI_END, 1, irq_number);

    KERNEL_DEBUG(PIC_DEBUG_ENABLED, MODULE_NAME, "PIC IRQ EOI");
}

INTERRUPT_TYPE_E pic_handle_spurious_irq(const uint32_t int_number)
{
    uint8_t  isr_val;
    uint32_t irq_number;

    irq_number = int_number - INT_PIC_IRQ_OFFSET;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_START, 1, int_number);

    KERNEL_DEBUG(PIC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Spurious handling %d",
                 irq_number);

    /* Check if regular soft interrupt */
    if(irq_number > PIC_MAX_IRQ_LINE)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                           2,
                           int_number,
                           INTERRUPT_TYPE_REGULAR);
        return INTERRUPT_TYPE_REGULAR;
    }

    /* Check the PIC type */
    if(irq_number > 7)
    {
        /* This is not a potential spurious irq */
        if(irq_number != PIC_SPURIOUS_IRQ_SLAVE)
        {
            KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                               2,
                               int_number,
                               INTERRUPT_TYPE_REGULAR);
            return INTERRUPT_TYPE_REGULAR;
        }

        /* Read the ISR mask */
        _cpu_outb(PIC_READ_ISR, PIC_SLAVE_COMM_PORT);
        isr_val = _cpu_inb(PIC_SLAVE_COMM_PORT);
        if((isr_val & PIC_SPURIOUS_IRQ_MASK) != 0)
        {
            KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                               2,
                               int_number,
                               INTERRUPT_TYPE_REGULAR);
            return INTERRUPT_TYPE_REGULAR;
        }
        else
        {
            /* Send EOI on master */
            pic_set_irq_eoi(PIC_CASCADING_IRQ);
            KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                               2,
                               int_number,
                               INTERRUPT_TYPE_SPURIOUS);

            return INTERRUPT_TYPE_SPURIOUS;
        }
    }
    else
    {
        /* This is not a potential spurious irq */
        if(irq_number != PIC_SPURIOUS_IRQ_MASTER)
        {
            KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                               2,
                               int_number,
                               INTERRUPT_TYPE_REGULAR);
            return INTERRUPT_TYPE_REGULAR;
        }

        /* Read the ISR mask */
        _cpu_outb(PIC_READ_ISR, PIC_MASTER_COMM_PORT);
        isr_val = _cpu_inb(PIC_MASTER_COMM_PORT);
        if((isr_val & PIC_SPURIOUS_IRQ_MASK) != 0)
        {
            KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                               2,
                               int_number,
                               INTERRUPT_TYPE_REGULAR);
            return INTERRUPT_TYPE_REGULAR;
        }
        else
        {
            KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                               2,
                               int_number,
                               INTERRUPT_TYPE_SPURIOUS);
            return INTERRUPT_TYPE_SPURIOUS;
        }
    }
}

void pic_disable(void)
{
    uint32_t int_state;

    ENTER_CRITICAL(int_state);

    /* Disable all IRQs */
    _cpu_outb(0xFF, PIC_MASTER_DATA_PORT);
    _cpu_outb(0xFF, PIC_SLAVE_DATA_PORT);

    KERNEL_DEBUG(PIC_DEBUG_ENABLED, MODULE_NAME, "PIC Disabled");

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_DISABLE, 0);

    EXIT_CRITICAL(int_state);
}

int32_t pic_get_irq_int_line(const uint32_t irq_number)
{

    if(irq_number > PIC_MAX_IRQ_LINE)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_GET_INT_LINE, 2, irq_number, -1);
        return -1;
    }

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_GET_INT_LINE,
                       2,
                       irq_number,
                       irq_number + INT_PIC_IRQ_OFFSET);
    return irq_number + INT_PIC_IRQ_OFFSET;
}

const interrupt_driver_t* pic_get_driver(void)
{
    return &pic_driver;
}

/************************************ EOF *************************************/