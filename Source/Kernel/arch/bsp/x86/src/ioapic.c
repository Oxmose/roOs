/*******************************************************************************
 * @file ioapic.c
 *
 * @see ioapic.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 05/06/2024
 *
 * @version 2.0
 *
 * @brief IO-APIC (IO advanced programmable interrupt controler) driver.
 *
 * @details IO-APIC (IO advanced programmable interrupt controler) driver.
 * Allows to remap the IO-APIC IRQ, set the IRQs mask and manage EoI for the
 * X86 IO-APIC.
 *
 * @warning This driver also use the LAPIC driver to function correctly.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <mmio.h>         /* MMIO accesses */
#include <acpi.h>         /* ACPI driver */
#include <lapic.h>        /* LAPIC driver */
#include <panic.h>        /* Kernel panic */
#include <kheap.h>        /* Kernel heap */
#include <stdint.h>       /* Generic int types */
#include <string.h>       /* String manipualtion */
#include <kerror.h>       /* Kernel error */
#include <memory.h>       /* Memory manager */
#include <devtree.h>      /* FDT driver */
#include <critical.h>     /* Kernel critical locks */
#include <drivermgr.h>    /* Driver manager */
#include <interrupts.h>   /* Interrupt manager */
#include <kerneloutput.h> /* Kernel output manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <ioapic.h>

/* Unit test header */
#include <test_framework.h>

/* Tracing feature */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for interrupt offset */
#define IOAPIC_FDT_INTOFF_PROP "int-offset"
/** @brief FDT property for acpi handle */
#define IOAPIC_FDT_ACPI_NODE_PROP "acpi-node"
/** @brief FDT property for lapic handle */
#define IOAPIC_FDT_LAPIC_NODE_PROP "lapic-node"
/** @brief FDT property for is-interrupt-driver */
#define IOAPIC_FDT_IS_INT_DRIVER_PROP "interrupt-controller"

/** @brief IO-APIC register selection. */
#define IOREGSEL 0x00
/** @brief IO-APIC data access register. */
#define IOWIN 0x10

/** @brief IO-ACPI memory size */
#define IOAPIC_MEM_SIZE 0x10

/** @brief IO-APIC ID register. */
#define IOAPICID  0x00
/** @brief IO-APIC version register. */
#define IOAPICVER 0x01
/** @brief IO-APIC arbitration id register. */
#define IOAPICARB 0x02
/** @brief IO-APIC redirection register base . */
#define IOREDTBLBASE  0x10
/** @brief IO-APIC indexed redirection low register. */
#define IOREDTBLXL(IRQ) ((IRQ) * 2 + IOREDTBLBASE)
/** @brief IO-APIC indexed redirection high register. */
#define IOREDTBLXH(IRQ) ((IRQ) * 2 + 1 + IOREDTBLBASE)

/** @brief IOAPIC Version register version value mask */
#define IOAPIC_VERSION_MASK 0x000000FF
/** @brief IOAPIC Version register redirection value mask */
#define IOAPIC_REDIR_MASK 0x00FF0000
/** @brief IOAPIC Version register version value shift */
#define IOAPIC_VERSION_SHIFT 0
/** @brief IOAPIC Version register redirection value shift */
#define IOAPIC_REDIR_SHIFT 16

/** @brief Current module name */
#define MODULE_NAME "X86 IO-APIC"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 IO-APIC driver controler. */
typedef struct io_apic_controler_t
{
    /** @brief IO-APIC base physical address */
    uintptr_t baseAddr;

    /** @brief IO-APIC mapping size */
    size_t mappingSize;

    /** @brief IO-APIC identifier. */
    uint8_t identifier;

    /** @brief IO-APIC version. */
    uint8_t version;

    /** @brief First IRQ handled by the current IO-APIC. */
    uint32_t gsib;

    /** @brief Last IRQ handled by the current IO-APIC. */
    uint8_t gsil;

    /** @brief Controler's lock */
    kernel_spinlock_t lock;

    /** @brief On system with multiple IO-APICs link to the next */
    struct io_apic_controler_t* pNext;
} io_apic_controler_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the IO-APIC to ensure correctness of execution.
 *
 * @details Assert macro used by the IO-APIC to ensure correctness of execution.
 * Due to the critical nature of the IO-APIC, any error generates a kernel
 * panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define IOAPIC_ASSERT(COND, MSG, ERROR) {                   \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG, TRUE);               \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the IO-APIC driver to the system.
 *
 * @details Attaches the IO-APIC driver to the system. This function will use
 * the FDT to initialize the IO-APIC hardware and retreive the IO-APIC
 * parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _ioapicAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Sets the IRQ mask for the desired IRQ number.
 *
 * @details Sets the IRQ mask for the IRQ number given as parameter.
 *
 * @param[in] kIrqNumber The IRQ number to enable/disable.
 * @param[in] kEnabled Must be set to TRUE to enable the IRQ or FALSE
 * to disable the IRQ.
 */
static void _ioapicSetIrqMask(const uint32_t kIrqNumber, const bool_t kEnabled);

/**
 * @brief Sets the IRQ mask for the desired IRQ number on a given controller.
 *
 * @details Sets the IRQ mask for the IRQ number given as parameter on a given
 * controller.
 *
 * @param[in, out] pCtrl The controller to use.
 * @param[in] kIrqNumber The IRQ number to enable/disable.
 * @param[in] kEnabled Must be set to TRUE to enable the IRQ or FALSE
 * to disable the IRQ.
 */
static inline void _ioapicSetIrqMaskFor(io_apic_controler_t* pCtrl,
                                        const uint32_t       kIrqNumber,
                                        const bool_t         kEnabled);

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
static INTERRUPT_TYPE_E _ioapicHandleSpurious(const uint32_t kIntNumber);

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
static int32_t _ioapicGetInterruptLine(const uint32_t kIrqNumber);

/**
 * @brief Reads into the IO APIC controller memory.
 *
 * @details Reads into the IO APIC controller memory.
 *
 * @param[in] kpCtrl The controller to use.
 * @param[in] kRegister The register to read.
 *
 * @return The value contained in the register.
 */
static inline uint32_t _ioapicRead(const io_apic_controler_t* kpCtrl,
                                   const uint32_t             kRegister);

/**
 * @brief Writes to the IO APIC controller memory.
 *
 * @details Writes to the IO APIC controller memory.
 *
 * @param[in] kpCtrl The controller to use.
 * @param[in] kRegister The register to write.
 * @param[in] kVal The value to write to the register.
 */
static inline void _ioapicWrite(const io_apic_controler_t* kpCtrl,
                                const uint32_t             kRegister,
                                const uint32_t             kVal);
/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief PIC driver instance. */
static driver_t sX86IOAPICDriver = {
    .pName         = "X86 IO-APIC Driver",
    .pDescription  = "X86 Advanced Programable Interrupt Controler Driver for UTK",
    .pCompatible   = "x86,x86-io-apic",
    .pVersion      = "2.0",
    .pDriverAttach = _ioapicAttach
};

/** @brief IO-APIC interrupt driver instance. */
static interrupt_driver_t sIOAPICDriver = {
    .pSetIrqMask          = _ioapicSetIrqMask,
    .pSetIrqEOI           = NULL,
    .pHandleSpurious      = _ioapicHandleSpurious,
    .pGetIrqInterruptLine = _ioapicGetInterruptLine
};

/** @brief IO-APIC driver controler instance */
static io_apic_controler_t* spDrvCtrl = NULL;

/** @brief IOAPIC ACPI driver handle */
static const acpi_driver_t* skpACPIDriver;

/** @brief IRQ interrupt offset */
static uint8_t sIntOffset;

/** @brief CPU's spurious interrupt line */
static uint32_t sSpuriousIntLine;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _ioapicAttach(const fdt_node_t* pkFdtNode)
{
    const uint32_t*       kpUintProp;
    io_apic_controler_t*  pNewDrvCtrl;
    lapic_driver_t*       pLapicDriver;
    const io_apic_node_t* kpIOAPICNode;
    OS_RETURN_E           retCode;
    size_t                propLen;
    uint8_t               i;
    uint32_t              ioapicVerRegister;
    uintptr_t             ioApicPhysAddr;
    size_t                toMap;

    KERNEL_TRACE_EVENT(TRACE_X86_IOAPIC_ENABLED,
                       TRACE_X86_IOAPIC_ATTACH_ENTRY,
                       0);

    retCode = OS_NO_ERR;

    /* Get CPU's spurious interrupt line */
    sSpuriousIntLine = cpuGetInterruptConfig()->spuriousInterruptLine;

    /* Get IRQ offset */
    kpUintProp = fdtGetProp(pkFdtNode, IOAPIC_FDT_INTOFF_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sIntOffset = (uint8_t)FDTTOCPU32(*kpUintProp);

    /* Get the ACPI pHandle */
    kpUintProp = fdtGetProp(pkFdtNode, IOAPIC_FDT_ACPI_NODE_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Get the ACPI driver */
    skpACPIDriver = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
    if(skpACPIDriver == NULL)
    {
        retCode = OS_ERR_NULL_POINTER;
        goto ATTACH_END;
    }

    /* Get the LAPIC pHandle */
    kpUintProp = fdtGetProp(pkFdtNode, IOAPIC_FDT_LAPIC_NODE_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Get the LAPIC driver */
    pLapicDriver = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
    if(pLapicDriver == NULL)
    {
        retCode = OS_ERR_NULL_POINTER;
        goto ATTACH_END;
    }

    /* Set IRQ EOI for delegated by the LAPIC */
    sIOAPICDriver.pSetIrqEOI = pLapicDriver->pSetIrqEOI;

    /* Get the IOAPICs */
    KERNEL_DEBUG(IOAPIC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Attaching %d IOAPICs",
                  skpACPIDriver->pGetIOAPICCount());

    kpIOAPICNode = skpACPIDriver->pGetIOAPICList();
    while(kpIOAPICNode != NULL)
    {
        KERNEL_DEBUG(IOAPIC_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Attaching IOAPIC with ID %d at 0x%p",
                     kpIOAPICNode->ioApic.ioApicId,
                     kpIOAPICNode->ioApic.ioApicAddr);

        pNewDrvCtrl = kmalloc(sizeof(io_apic_controler_t));
        if(pNewDrvCtrl == NULL)
        {
            retCode = OS_ERR_NO_MORE_MEMORY;
            goto ATTACH_END;
        }
        memset(pNewDrvCtrl, 0, sizeof(io_apic_controler_t));

        /* Set the spinlock and driver */
        KERNEL_SPINLOCK_INIT(pNewDrvCtrl->lock);

        /* Map the IO APIC */
        ioApicPhysAddr = kpIOAPICNode->ioApic.ioApicAddr & ~PAGE_SIZE_MASK;
        toMap = IOAPIC_MEM_SIZE + (ioApicPhysAddr & PAGE_SIZE_MASK);
        toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

        pNewDrvCtrl->baseAddr = (uintptr_t)memoryKernelMap((void*)ioApicPhysAddr,
                                                toMap,
                                                MEMMGR_MAP_HARDWARE |
                                                MEMMGR_MAP_KERNEL   |
                                                MEMMGR_MAP_RW,
                                                &retCode);
        if(pNewDrvCtrl->baseAddr == (uintptr_t)NULL || retCode != OS_NO_ERR)
        {
            goto ATTACH_END;
        }
        pNewDrvCtrl->baseAddr |= kpIOAPICNode->ioApic.ioApicAddr &
                                 PAGE_SIZE_MASK;
        pNewDrvCtrl->mappingSize = toMap;

        /* Setup the controller */
        pNewDrvCtrl->identifier = kpIOAPICNode->ioApic.ioApicId;
        pNewDrvCtrl->gsib = kpIOAPICNode->ioApic.globalSystemInterruptBase;

        /* Get the version and max IRQ number */
        ioapicVerRegister = _ioapicRead(pNewDrvCtrl, IOAPICVER);
        pNewDrvCtrl->version = (ioapicVerRegister & IOAPIC_VERSION_MASK) >>
                                IOAPIC_VERSION_SHIFT;
        pNewDrvCtrl->gsil = pNewDrvCtrl->gsil + 1 +
            ((ioapicVerRegister & IOAPIC_REDIR_MASK) >> IOAPIC_REDIR_SHIFT);

        /* Link IO APIC controller */
        pNewDrvCtrl->pNext = spDrvCtrl;
        spDrvCtrl = pNewDrvCtrl;

        KERNEL_DEBUG(IOAPIC_DEBUG_ENABLED,
                     MODULE_NAME,
                     "IOAPIC ID %d:\n"
                     "\tPhysical Address: 0x%p\n"
                     "\tVirtual Address: 0x%p\n"
                     "\tIdentifier: %d\n"
                     "\tVersion: %d\n"
                     "\tMin IRQ: %d\n"
                     "\tIRQ Limit: %d\n",
                     pNewDrvCtrl->identifier,
                     kpIOAPICNode->ioApic.ioApicAddr,
                     pNewDrvCtrl->baseAddr,
                     pNewDrvCtrl->identifier,
                     pNewDrvCtrl->version,
                     pNewDrvCtrl->gsib,
                     pNewDrvCtrl->gsil);

        /* Disable all IRQ for this  IOAPIC */
        for(i = pNewDrvCtrl->gsib; i < pNewDrvCtrl->gsil; ++i)
        {
            _ioapicSetIrqMaskFor(pNewDrvCtrl, i, FALSE);
        }

        /* Go to next */
        kpIOAPICNode = kpIOAPICNode->pNext;
    }

    /* Register if needed */
    if(fdtGetProp(pkFdtNode, IOAPIC_FDT_IS_INT_DRIVER_PROP, &propLen) != NULL)
    {
        /* Register as interrupt controler */
        retCode = interruptSetDriver(&sIOAPICDriver);
        IOAPIC_ASSERT(retCode == OS_NO_ERR,
                      "Failed to register IO-APIC in interrupt manager",
                      retCode);
    }

ATTACH_END:
    if(retCode != OS_NO_ERR)
    {
        while(spDrvCtrl != NULL)
        {
            pNewDrvCtrl = spDrvCtrl->pNext;
            kfree(spDrvCtrl);
            if(spDrvCtrl->baseAddr != (uintptr_t)NULL)
            {
                if(memoryKernelUnmap((void*)spDrvCtrl->baseAddr,
                                     spDrvCtrl->mappingSize))
                {
                    KERNEL_ERROR("Failed to unmap IO-APIC memory\n");
                }
            }
            spDrvCtrl = pNewDrvCtrl;
        }
    }

    KERNEL_DEBUG(IOAPIC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "IO-APIC Initialization end");

    KERNEL_TRACE_EVENT(TRACE_X86_IOAPIC_ENABLED,
                       TRACE_X86_IOAPIC_ATTACH_EXIT,
                       1,
                       (uint32_t)retCode);

    return retCode;
}

static void _ioapicSetIrqMask(const uint32_t kIrqNumber, const bool_t kEnabled)
{
    uint32_t             remapIrq;
    io_apic_controler_t* pCtrl;

    KERNEL_TRACE_EVENT(TRACE_X86_IOAPIC_ENABLED,
                       TRACE_X86_IOAPIC_SET_IRQ_MASK_ENTRY,
                       2,
                       kIrqNumber,
                       (uint32_t)kEnabled);

    KERNEL_DEBUG(IOAPIC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Request to mask IRQ %d: %d",
                 kIrqNumber,
                 (uint32_t)kEnabled);

    /* Get the remapped IRQ */
    remapIrq = skpACPIDriver->pGetRemapedIrq(kIrqNumber);

    /* Search for the correct IO APIC controller */
    pCtrl = spDrvCtrl;
    while(pCtrl != NULL)
    {
        if(pCtrl->gsib <= remapIrq && pCtrl->gsil > remapIrq)
        {
            break;
        }
        pCtrl = pCtrl->pNext;
    }

    IOAPIC_ASSERT(pCtrl != NULL, "No such IRQ", OS_ERR_NO_SUCH_IRQ);

    _ioapicSetIrqMaskFor(pCtrl, remapIrq, kEnabled);

    KERNEL_TRACE_EVENT(TRACE_X86_IOAPIC_ENABLED,
                       TRACE_X86_IOAPIC_SET_IRQ_MASK_EXIT,
                       2,
                       kIrqNumber,
                       (uint32_t)kEnabled);
}

static inline void _ioapicSetIrqMaskFor(io_apic_controler_t* pCtrl,
                                        const uint32_t       kIrqNumber,
                                        const bool_t         kEnabled)
{
    uint32_t entryLow;
    uint32_t remapIrq;

    KERNEL_TRACE_EVENT(TRACE_X86_IOAPIC_ENABLED,
                       TRACE_X86_IOAPIC_SET_IRQ_MASK_FOR_ENTRY,
                       4,
                       KERNEL_TRACE_HIGH(pCtrl),
                       KERNEL_TRACE_LOW(pCtrl),
                       kIrqNumber,
                       (uint32_t)kEnabled);

    IOAPIC_ASSERT(kIrqNumber >= pCtrl->gsib && kIrqNumber < pCtrl->gsil,
                  "No such IRQ for current IOAPIC",
                  OS_ERR_NO_SUCH_IRQ);

    /* Update the IRQ for the table */
    remapIrq = kIrqNumber - pCtrl->gsib;

    KERNEL_CRITICAL_LOCK(pCtrl->lock);

    /* Set the mask, IO APIC uses physical destination only to core 0 */
    entryLow = _ioapicGetInterruptLine(kIrqNumber);
    if(kEnabled == FALSE)
    {
        entryLow  |= (1 << 16);
    }
    else
    {
        entryLow  &= ~(1 << 16);
    }
    _ioapicWrite(pCtrl, IOREDTBLXL(remapIrq), entryLow);

    KERNEL_CRITICAL_UNLOCK(pCtrl->lock);

    KERNEL_DEBUG(IOAPIC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Mask IRQ %d (%d): %d",
                 kIrqNumber,
                 remapIrq,
                 (uint32_t)kEnabled);

    KERNEL_TRACE_EVENT(TRACE_X86_IOAPIC_ENABLED,
                       TRACE_X86_IOAPIC_SET_IRQ_MASK_FOR_EXIT,
                       4,
                       KERNEL_TRACE_HIGH(pCtrl),
                       KERNEL_TRACE_LOW(pCtrl),
                       kIrqNumber,
                       (uint32_t)kEnabled);
}

static INTERRUPT_TYPE_E _ioapicHandleSpurious(const uint32_t kIntNumber)
{
    INTERRUPT_TYPE_E intType;

    KERNEL_TRACE_EVENT(TRACE_X86_IOAPIC_ENABLED,
                       TRACE_X86_IOAPIC_HANDLE_SPURIOUS_ENTRY,
                       1,
                       kIntNumber);

    intType = INTERRUPT_TYPE_REGULAR;

    /* Check for LAPIC spurious interrupt. */
    if(kIntNumber == sSpuriousIntLine)
    {
        sIOAPICDriver.pSetIrqEOI(kIntNumber);
        intType = INTERRUPT_TYPE_SPURIOUS;
    }

    KERNEL_DEBUG(IOAPIC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Spurious IRQ ? %d : %d",
                 kIntNumber,
                 intType);

    KERNEL_TRACE_EVENT(TRACE_X86_IOAPIC_ENABLED,
                       TRACE_X86_IOAPIC_HANDLE_SPURIOUS_EXIT,
                       2,
                       kIntNumber,
                       intType);

    return intType;
}

static int32_t _ioapicGetInterruptLine(const uint32_t kIrqNumber)
{
    uint32_t remapIrq;

    KERNEL_TRACE_EVENT(TRACE_X86_IOAPIC_ENABLED,
                       TRACE_X86_IOAPIC_GET_INT_LINE_ENTRY,
                       1,
                       kIrqNumber);

    remapIrq = skpACPIDriver->pGetRemapedIrq(kIrqNumber);

    KERNEL_TRACE_EVENT(TRACE_X86_IOAPIC_ENABLED,
                       TRACE_X86_IOAPIC_GET_INT_LINE_EXIT,
                       2,
                       kIrqNumber,
                       sIntOffset + remapIrq);

    return sIntOffset + remapIrq;
}

static inline uint32_t _ioapicRead(const io_apic_controler_t* kpCtrl,
                                   const uint32_t kRegister)
{
    /* Set IOREGSEL */
    _mmioWrite32((void*)(kpCtrl->baseAddr + IOREGSEL), kRegister);

    /* Get register value */
    return _mmioRead32((void*)(kpCtrl->baseAddr + IOWIN));
}

static inline void _ioapicWrite(const io_apic_controler_t* kpCtrl,
                                const uint32_t kRegister,
                                const uint32_t kVal)
{
    /* Set IOREGSEL */
    _mmioWrite32((void*)(kpCtrl->baseAddr + IOREGSEL), kRegister);

    /* Set register value */
    _mmioWrite32((void*)(kpCtrl->baseAddr + IOWIN), kVal);
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG(sX86IOAPICDriver);

/************************************ EOF *************************************/