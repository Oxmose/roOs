/*******************************************************************************
 * @file pic.c
 *
 * @see pic.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 2.0
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
#include <cpu.h>          /* CPU port manipulation */
#include <panic.h>        /* Kernel panic */
#include <stdint.h>       /* Generic int types */
#include <string.h>       /* String manipualtion */
#include <kerror.h>       /* Kernel error */
#include <devtree.h>      /* FDT driver */
#include <critical.h>     /* Kernel critical locks */
#include <drivermgr.h>    /* Driver manager */
#include <interrupts.h>   /* Interrupt manager */
#include <kerneloutput.h> /* Kernel output manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <pic.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for chaining  */
#define PIC_FDT_HASSLAVE_PROP "is-chained"
/** @brief FDT property for interrupt offset */
#define PIC_FDT_INTOFF_PROP   "int-offset"
/** @brief FDT property for comm ports */
#define PIC_FDT_COMM_PROP     "comm"

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
#define PIC0_BASE_INTERRUPT_LINE (sDrvCtrl.intOffset)
/** @brief Slave PIC Base interrupt line for the lowest IRQ. */
#define PIC1_BASE_INTERRUPT_LINE (PIC0_BASE_INTERRUPT_LINE + 8)

/** @brief PIC's cascading IRQ number. */
#define PIC_CASCADING_IRQ 2

/** @brief The PIC spurious irq mask. */
#define PIC_SPURIOUS_IRQ_MASK 0x80

/** @brief Master PIC spurious IRQ number. */
#define PIC_SPURIOUS_IRQ_MASTER 0x07
/** @brief Slave PIC spurious IRQ number. */
#define PIC_SPURIOUS_IRQ_SLAVE  0x0F

/** @brief PIC's minimal IRQ number. */
#define PIC_MIN_IRQ_LINE 0
/** @brief PIC's maximal IRQ number. */
#define PIC_MAX_IRQ_LINE 15

/** @brief Current module name */
#define MODULE_NAME "X86 PIC"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 PIC driver controler. */
typedef struct
{
    /** @brief CPU command port. */
    uint16_t cpuMasterCommPort;

    /** @brief CPU data port. */
    uint16_t cpuMasterDataPort;

    /** @brief CPU command port. */
    uint16_t cpuSlaveCommPort;

    /** @brief CPU data port. */
    uint16_t cpuSlaveDataPort;

    /** @brief Tells if the PIC has a slave */
    bool_t hasSlave;

    /** @brief PIC IRQ interrupt offset */
    uint8_t intOffset;
} pic_controler_t;

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
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the PIC driver to the system.
 *
 * @details Attaches the PIC driver to the system. This function will use the
 * FDT to initialize the PIC hardware and retreive the PIC parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _picAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Sets the IRQ mask for the desired IRQ number.
 *
 * @details Sets the IRQ mask for the IRQ number given as parameter.
 *
 * @param[in] kIrqNumber The IRQ number to enable/disable.
 * @param[in] kEnabled Must be set to TRUE to enable the IRQ or FALSE
 * to disable the IRQ.
 */
static void _picSetIRQMask(const uint32_t kIrqNumber, const bool_t kEnabled);

/**
 * @brief Acknowleges an IRQ.
 *
 * @details Acknowlege an IRQ by setting the End Of Interrupt bit for this IRQ.
 *
 * @param[in] kIrqNumber The irq number to Acknowlege.
 */
static void _picSetIRQEOI(const uint32_t kIrqNumber);

/**
 * @brief Checks if the serviced interrupt is a spurious
 * interrupt. The function also handles the spurious interrupt.
 *
 * @details Checks if the serviced interrupt is a spurious
 * interrupt. The function also handles the spurious interrupt.
 *
 * @param[in] kIntNumber The interrupt number of the interrupt to
 * test.
 *
 * @return The function will return the interrupt type.
 * - INTERRUPT_TYPE_SPURIOUS if the current interrupt is a spurious one.
 * - INTERRUPT_TYPE_REGULAR if the current interrupt is a regular one.
 */
static INTERRUPT_TYPE_E _picHandleSpurious(const uint32_t kIntNumber);

/**
 * @brief Returns the interrupt line attached to an IRQ.
 *
 * @details Returns the interrupt line attached to an IRQ. -1 is returned
 * if the IRQ number is not supported by the driver.
 *
 * @param[in] kIrqNumber The IRQ number of which to get the interrupt
 * line.
 *
 * @return The interrupt line attached to an IRQ. -1 is returned if the IRQ
 * number is not supported by the driver.
 */
static int32_t _picGetInterruptLine(const uint32_t kIrqNumber);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/** @brief PIC driver instance. */
driver_t x86PICDriver = {
    .pName         = "X86 PIC Driver",
    .pDescription  = "X86 Programable Interrupt Controler Driver for UTK",
    .pCompatible   = "x86,x86-pic",
    .pVersion      = "2.0",
    .pDriverAttach = _picAttach
};


/************************** Static global variables ***************************/
/** @brief PIC interrupt driver instance. */
static interrupt_driver_t sPicDriver = {
    .driver_set_irq_mask     = _picSetIRQMask,
    .driver_set_irq_eoi      = _picSetIRQEOI,
    .driver_handle_spurious  = _picHandleSpurious,
    .driver_get_irq_int_line = _picGetInterruptLine
};

/** @brief PIC driver controler instance */
static pic_controler_t sDrvCtrl = {
    .cpuMasterCommPort = 0,
    .cpuMasterDataPort = 0,
    .cpuSlaveCommPort  = 0,
    .cpuSlaveDataPort  = 0,
    .hasSlave          = FALSE,
    .intOffset         = 0
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _picAttach(const fdt_node_t* pkFdtNode)
{
    const uintptr_t* ptrProp;
    size_t           propLen;
    OS_RETURN_E      retCode;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_INIT_START, 0);

    /* Check for slave */
    if(fdtGetProp(pkFdtNode, PIC_FDT_HASSLAVE_PROP, &propLen) != NULL)
    {
        sDrvCtrl.hasSlave = TRUE;
    }

    /* Get IRQ offset */
    ptrProp = fdtGetProp(pkFdtNode, PIC_FDT_INTOFF_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t))
    {
        KERNEL_ERROR("Failed to retreive the PIC IRQ offset from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.intOffset = (uint8_t)FDTTOCPU32(*ptrProp);


    /* Get the com ports */
    ptrProp = fdtGetProp(pkFdtNode, PIC_FDT_COMM_PROP, &propLen);
    if(sDrvCtrl.hasSlave == TRUE)
    {
        if(ptrProp == NULL || propLen != sizeof(uintptr_t) * 4)
        {
            KERNEL_ERROR("Failed to retreive the PIC COMM from FDT.\n");
            retCode = OS_ERR_INCORRECT_VALUE;
            goto ATTACH_END;
        }
        sDrvCtrl.cpuMasterCommPort = (uint8_t)FDTTOCPU32(*ptrProp);
        sDrvCtrl.cpuMasterDataPort = (uint8_t)FDTTOCPU32(*(ptrProp + 1));
        sDrvCtrl.cpuSlaveCommPort  = (uint8_t)FDTTOCPU32(*(ptrProp + 2));
        sDrvCtrl.cpuSlaveDataPort  = (uint8_t)FDTTOCPU32(*(ptrProp + 3));
    }
    else
    {
        if(ptrProp == NULL || propLen != sizeof(uintptr_t) * 2)
        {
            KERNEL_ERROR("Failed to retreive the PIC COMM from FDT.\n");
            retCode = OS_ERR_INCORRECT_VALUE;
            goto ATTACH_END;
        }
        sDrvCtrl.cpuMasterCommPort = (uint8_t)FDTTOCPU32(*ptrProp);
        sDrvCtrl.cpuMasterDataPort = (uint8_t)FDTTOCPU32(*(ptrProp + 1));
    }

    /* Initialize the master, remap IRQs */
    _cpu_outb(PIC_ICW1_ICW4 | PIC_ICW1_INIT, sDrvCtrl.cpuMasterCommPort);
    _cpu_outb(PIC0_BASE_INTERRUPT_LINE, sDrvCtrl.cpuMasterDataPort);
    _cpu_outb(0x4, sDrvCtrl.cpuMasterDataPort);
    _cpu_outb(PIC_ICW4_8086, sDrvCtrl.cpuMasterDataPort);
    /* Set EOI */
    _cpu_outb(PIC_EOI, sDrvCtrl.cpuMasterCommPort);
    /* Disable all IRQs */
    _cpu_outb(0xFF, sDrvCtrl.cpuMasterDataPort);

    if(sDrvCtrl.hasSlave == TRUE)
    {
        /* Initialize the slave, remap IRQs */
        _cpu_outb(PIC_ICW1_ICW4 | PIC_ICW1_INIT, sDrvCtrl.cpuSlaveCommPort);
        _cpu_outb(PIC1_BASE_INTERRUPT_LINE, sDrvCtrl.cpuSlaveDataPort);
        _cpu_outb(0x2,  sDrvCtrl.cpuSlaveDataPort);
        _cpu_outb(PIC_ICW4_8086,  sDrvCtrl.cpuSlaveDataPort);
        /* Set EOI for */
        _cpu_outb(PIC_EOI, sDrvCtrl.cpuSlaveCommPort);
        /* Disable all IRQs */
        _cpu_outb(0xFF, sDrvCtrl.cpuSlaveDataPort);
    }

    /* Register as interrupt controler */
    retCode = kernel_interrupt_set_driver(&sPicDriver);
    PIC_ASSERT(retCode == OS_NO_ERR,
               "Could register PIC in interrupt manager",
               retCode);

ATTACH_END:
    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_INIT_END, 1, (uintptr_t)retCode);

    KERNEL_DEBUG(PIC_DEBUG_ENABLED, MODULE_NAME, "PIC Initialization end");

    return retCode;
}

static void _picSetIRQMask(const uint32_t kIrqNumber, const bool_t kEnabled)
{
    uint8_t  initMask;
    uint32_t intState;
    uint32_t cascadingNumber;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SET_IRQ_MASK_START,
                       2,
                       kIrqNumber,
                       kEnabled);

    PIC_ASSERT(kIrqNumber <= PIC_MAX_IRQ_LINE,
               "Could not find PIC IRQ",
               OS_ERR_NO_SUCH_IRQ);

    ENTER_CRITICAL(intState);

    /* Manage master PIC */
    if(kIrqNumber < 8)
    {
        /* Retrieve initial mask */
        initMask = _cpu_inb(sDrvCtrl.cpuMasterDataPort);

        /* Set new mask value */
        if(kEnabled == FALSE)
        {
            initMask |= 1 << kIrqNumber;
        }
        else
        {
            initMask &= ~(1 << kIrqNumber);
        }

        /* Set new mask */
        _cpu_outb(initMask, sDrvCtrl.cpuMasterDataPort);

        KERNEL_DEBUG(PIC_DEBUG_ENABLED,
                     MODULE_NAME,
                     "New PIC Mask M: 0x%02x",
                     _cpu_inb(sDrvCtrl.cpuMasterDataPort));
    }

    /* Manage slave PIC. WARNING, cascading will be enabled */
    if(kIrqNumber > 7)
    {
        PIC_ASSERT(sDrvCtrl.hasSlave == TRUE,
                   "Could not find PIC IRQ (chained)",
                   OS_ERR_NO_SUCH_IRQ);

        /* Set new IRQ number */
        cascadingNumber = kIrqNumber - 8;

        /* Retrieve initial mask */
        initMask = _cpu_inb(sDrvCtrl.cpuMasterDataPort);

        /* Set new mask value */
        initMask &= ~(1 << PIC_CASCADING_IRQ);

        /* Set new mask */
        _cpu_outb(initMask, sDrvCtrl.cpuMasterDataPort);

        /* Retrieve initial mask */
        initMask = _cpu_inb(sDrvCtrl.cpuSlaveDataPort);

        /* Set new mask value */
        if(kEnabled == FALSE)
        {
            initMask |= 1 << cascadingNumber;
        }
        else
        {
            initMask &= ~(1 << cascadingNumber);
        }

        /* Set new mask */
        _cpu_outb(initMask, sDrvCtrl.cpuSlaveDataPort);

        /* If all is masked then disable cascading */
        if(initMask == 0xFF)
        {
            /* Retrieve initial mask */
            initMask = _cpu_inb(sDrvCtrl.cpuMasterDataPort);

            /* Set new mask value */
            initMask  |= 1 << PIC_CASCADING_IRQ;

            /* Set new mask */
            _cpu_outb(initMask, sDrvCtrl.cpuMasterDataPort);
        }

        KERNEL_DEBUG(PIC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "New PIC Mask M: 0x%02x S: 0x%02x",
                 _cpu_inb(sDrvCtrl.cpuMasterDataPort),
                 _cpu_inb(sDrvCtrl.cpuSlaveDataPort));
    }



    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SET_IRQ_MASK_END,
                       2,
                       kIrqNumber,
                       kEnabled);

    EXIT_CRITICAL(intState);
}

static void _picSetIRQEOI(const uint32_t kIrqNumber)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_EOI_START, 1, kIrqNumber);

    PIC_ASSERT(kIrqNumber <= PIC_MAX_IRQ_LINE,
               "Could not find PIC IRQ",
               OS_ERR_NO_SUCH_IRQ);


    /* End of interrupt signal */
    if(kIrqNumber > 7)
    {
        PIC_ASSERT(sDrvCtrl.hasSlave == TRUE,
                   "Could not find PIC IRQ (chained)",
                   OS_ERR_NO_SUCH_IRQ);

        _cpu_outb(PIC_EOI, sDrvCtrl.cpuSlaveCommPort);
    }
    _cpu_outb(PIC_EOI, sDrvCtrl.cpuMasterCommPort);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_EOI_END, 1, kIrqNumber);

    KERNEL_DEBUG(PIC_DEBUG_ENABLED, MODULE_NAME, "PIC IRQ EOI");
}

static INTERRUPT_TYPE_E _picHandleSpurious(const uint32_t kIntNumber)
{
    uint8_t  isrVal;
    uint32_t irqNumber;

    irqNumber = kIntNumber - PIC0_BASE_INTERRUPT_LINE;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_START, 1, kIntNumber);

    KERNEL_DEBUG(PIC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Spurious handling %d",
                 irqNumber);

    /* Check if regular soft interrupt */
    if(irqNumber > PIC_MAX_IRQ_LINE)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                           2,
                           kIntNumber,
                           INTERRUPT_TYPE_REGULAR);
        return INTERRUPT_TYPE_REGULAR;
    }

    /* Check the PIC type */
    if(irqNumber > 7)
    {
        PIC_ASSERT(sDrvCtrl.hasSlave == TRUE,
                   "Could not find spurious PIC IRQ (chained)",
                   OS_ERR_NO_SUCH_IRQ);

        /* This is not a potential spurious irq */
        if(irqNumber != PIC_SPURIOUS_IRQ_SLAVE)
        {
            KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                               2,
                               kIntNumber,
                               INTERRUPT_TYPE_REGULAR);
            return INTERRUPT_TYPE_REGULAR;
        }

        /* Read the ISR mask */
        _cpu_outb(PIC_READ_ISR, sDrvCtrl.cpuSlaveCommPort);
        isrVal = _cpu_inb(sDrvCtrl.cpuSlaveCommPort);
        if((isrVal & PIC_SPURIOUS_IRQ_MASK) != 0)
        {
            KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                               2,
                               kIntNumber,
                               INTERRUPT_TYPE_REGULAR);
            return INTERRUPT_TYPE_REGULAR;
        }
        else
        {
            /* Send EOI on master */
            _picSetIRQEOI(PIC_CASCADING_IRQ);
            KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                               2,
                               kIntNumber,
                               INTERRUPT_TYPE_SPURIOUS);

            return INTERRUPT_TYPE_SPURIOUS;
        }
    }
    else
    {
        /* This is not a potential spurious irq */
        if(irqNumber != PIC_SPURIOUS_IRQ_MASTER)
        {
            KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                               2,
                               kIntNumber,
                               INTERRUPT_TYPE_REGULAR);
            return INTERRUPT_TYPE_REGULAR;
        }

        /* Read the ISR mask */
        _cpu_outb(PIC_READ_ISR, sDrvCtrl.cpuMasterCommPort);
        isrVal = _cpu_inb(sDrvCtrl.cpuMasterCommPort);
        if((isrVal & PIC_SPURIOUS_IRQ_MASK) != 0)
        {
            KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                               2,
                               kIntNumber,
                               INTERRUPT_TYPE_REGULAR);
            return INTERRUPT_TYPE_REGULAR;
        }
        else
        {
            KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_SPURIOUS_END,
                               2,
                               kIntNumber,
                               INTERRUPT_TYPE_SPURIOUS);
            return INTERRUPT_TYPE_SPURIOUS;
        }
    }
}

static int32_t _picGetInterruptLine(const uint32_t kIrqNumber)
{
    if(kIrqNumber > PIC_MAX_IRQ_LINE)
    {
        KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_GET_INT_LINE, 2, kIrqNumber, -1);
        return -1;
    }

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PIC_GET_INT_LINE,
                       2,
                       kIrqNumber,
                       kIrqNumber + PIC0_BASE_INTERRUPT_LINE);
    return kIrqNumber + PIC0_BASE_INTERRUPT_LINE;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG(x86PICDriver);

/************************************ EOF *************************************/