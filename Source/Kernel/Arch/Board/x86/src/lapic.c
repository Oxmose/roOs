/*******************************************************************************
 * @file lapic.c
 *
 * @see lapic.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 06/06/2024
 *
 * @version 2.0
 *
 * @brief Local APIC (Advanced programmable interrupt controler) driver.
 *
 * @details Local APIC (Advanced programmable interrupt controler) driver.
 * Manages x86 IRQs from the IO-APIC. IPI (inter processor interrupt) are also
 * possible thanks to the driver.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <mmio.h>         /* MMIO accesses */
#include <acpi.h>         /* ACPI driver */
#include <panic.h>        /* Kernel panic */
#include <kheap.h>        /* Kernel heap */
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
#include <lapic.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for acpi handle */
#define LAPIC_FDT_ACPI_NODE_PROP "acpi-node"

/** @brief LAPIC ID register's offset. */
#define LAPIC_ID 0x0020
/** @brief LAPIC version register's offset. */
#define LAPIC_VER 0x0030
/** @brief LAPIC trask priority register's offset. */
#define LAPIC_TPR 0x0080
/** @brief LAPIC arbitration policy register's offset. */
#define LAPIC_APR 0x0090
/** @brief LAPIC processor priority register's offset. */
#define LAPIC_PPR 0x00A0
/** @brief LAPIC EOI register's offset. */
#define LAPIC_EOI 0x00B0
/** @brief LAPIC remote read register's offset. */
#define LAPIC_RRD 0x00C0
/** @brief LAPIC logical destination register's offset. */
#define LAPIC_LDR 0x00D0
/** @brief LAPIC destination format register's offset. */
#define LAPIC_DFR 0x00E0
/** @brief LAPIC Spurious interrupt vector register's offset. */
#define LAPIC_SVR 0x00F0
/** @brief LAPIC in service register's offset. */
#define LAPIC_ISR 0x0100
/** @brief LAPIC trigger mode register's offset. */
#define LAPIC_TMR 0x0180
/** @brief LAPIC interrupt request register's offset. */
#define LAPIC_IRR 0x0200
/** @brief LAPIC error status register's offset. */
#define LAPIC_ESR 0x0280
/** @brief LAPIC interrupt command (low) register's offset. */
#define LAPIC_ICRLO 0x0300
/** @brief LAPIC interrupt command (high) register's offset. */
#define LAPIC_ICRHI 0x0310
/** @brief LAPIC local vector table timer register's offset. */
#define LAPIC_TIMER 0x0320
/** @brief LAPIC local vector table thermal sensor register's offset. */
#define LAPIC_THERMAL 0x0330
/** @brief LAPIC local vector table PMC register's offset. */
#define LAPIC_PERF 0x0340
/** @brief LAPIC local vector table lint0 register's offset. */
#define LAPIC_LINT0 0x0350
/** @brief LAPIC local vector table lint1 register's offset. */
#define LAPIC_LINT1 0x0360
/** @brief LAPIC local vector table error register's offset. */
#define LAPIC_ERROR 0x0370

/** @brief LAPIC delivery mode fixed. */
#define ICR_FIXED 0x00000000
/** @brief LAPIC delivery mode lowest priority. */
#define ICR_LOWEST 0x00000100
/** @brief LAPIC delivery mode SMI. */
#define ICR_SMI 0x00000200
/** @brief LAPIC delivery mode NMI. */
#define ICR_NMI 0x00000400
/** @brief LAPIC delivery mode init IPI. */
#define ICR_INIT 0x00000500
/** @brief LAPIC delivery mode startup IPI. */
#define ICR_STARTUP 0x00000600
/** @brief LAPIC delivery mode external. */
#define ICR_EXTERNAL 0x00000700

/** @brief LAPIC destination mode physical. */
#define ICR_PHYSICAL 0x00000000
/** @brief LAPIC destination mode logical. */
#define ICR_LOGICAL 0x00000800

/** @brief LAPIC Delivery status idle. */
#define ICR_IDLE 0x00000000
/** @brief LAPIC Delivery status pending. */
#define ICR_SEND_PENDING 0x00001000

/** @brief LAPIC Level deassert enable flag. */
#define ICR_DEASSERT 0x00000000
/** @brief LAPIC Level deassert disable flag. */
#define ICR_ASSERT 0x00004000

/** @brief LAPIC trigger mode edge. */
#define ICR_EDGE 0x00000000
/** @brief LAPIC trigger mode level. */
#define ICR_LEVEL 0x00008000

/** @brief LAPIC destination shorthand none. */
#define ICR_NO_SHORTHAND 0x00000000
/** @brief LAPIC destination shorthand self only. */
#define ICR_SELF 0x00040000
/** @brief LAPIC destination shorthand all and self. */
#define ICR_ALL_INCLUDING_SELF 0x00080000
/** @brief LAPIC destination shorthand all but self. */
#define ICR_ALL_EXCLUDING_SELF 0x000C0000

/** @brief LAPIC destination flag shift. */
#define ICR_DESTINATION_SHIFT 24

/** @brief Current module name */
#define MODULE_NAME "X86 LAPIC"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 LAPIC driver controler. */
typedef struct lapic_controler_t
{
    /** @brief LAPIC base physical address */
    uintptr_t basePhysAddr;

    /** @brief List of present LAPICs from the ACPI. */
    const lapic_node_t* pLapicList;
} lapic_controler_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the LAPIC to ensure correctness of execution.
 *
 * @details Assert macro used by the LAPIC to ensure correctness of execution.
 * Due to the critical nature of the LAPIC, any error generates a kernel
 * panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define LAPIC_ASSERT(COND, MSG, ERROR) {                    \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG, TRUE);               \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the LAPIC driver to the system.
 *
 * @details Attaches the LAPIC driver to the system. This function will use
 * the FDT to initialize the LAPIC hardware and retreive the LAPIC parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _lapicAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Sets END OF INTERRUPT for the current CPU Local APIC.
 *
 * @details Sets END OF INTERRUPT for the current CPU Local APIC.
 *
 * @param[in] kInterruptLine The interrupt line for which the EOI should be set.
 */
static void _lapicSetIrqEOI(const uint32_t kInterruptLine);

/**
 * @brief Returns the base address of the local APIC.
 *
 * @details Returns the base address of the local APIC.
 *
 * @return The base address of the LAPIC is returned.
 */
uintptr_t _lapicGetBaseAddress(void);

/**
 * @brief Reads into the LAPIC controller memory.
 *
 * @details Reads into the LAPIC controller memory.
 *
 * @param[in] kRegister The register to read.
 *
 * @return The value contained in the register.
 */
inline static uint32_t _lapicRead(const uint32_t kRegister);

/**
 * @brief Writes to the LAPIC controller memory.
 *
 * @details Writes to the LAPIC controller memory.
 *
 * @param[in] kRegister The register to write.
 * @param[in] kVal The value to write to the register.
 */
inline static void _lapicWrite(const uint32_t kRegister, const uint32_t kVal);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief PIC driver instance. */
static driver_t sX86LAPICDriver = {
    .pName         = "X86 Local APIC Driver",
    .pDescription  = "X86 Local Advanced Programable Interrupt Controler Driver for UTK",
    .pCompatible   = "x86,x86-lapic",
    .pVersion      = "2.0",
    .pDriverAttach = _lapicAttach
};

/** @brief LAPIC API driver. */
static lapic_driver_t sAPIDriver = {
    .pSetIrqEOI      = _lapicSetIrqEOI,
    .pGetBaseAddress = _lapicGetBaseAddress
};

/** @brief LAPIC driver controler instance. There will be only on for all
 * lapics, no need for dynamic allocation
 */
static lapic_controler_t sDrvCtrl = {
    .basePhysAddr = 0,
    .pLapicList   = NULL
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _lapicAttach(const fdt_node_t* pkFdtNode)
{
    const uint32_t*      kpUintProp;
    const acpi_driver_t* skpACPIDriver;
    OS_RETURN_E          retCode;
    size_t               propLen;

#if LAPIC_DEBUG_ENABLED
    const lapic_node_t*  kpLAPICNode;
#endif

    KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_ENABLED,
                       TRACE_X86_LAPIC_ATTACH_ENTRY,
                       0);

    retCode = OS_NO_ERR;

    /* Get the ACPI pHandle */
    kpUintProp = fdtGetProp(pkFdtNode, LAPIC_FDT_ACPI_NODE_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        KERNEL_ERROR("Failed to retreive the LAPIC ACPI handle FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Get the ACPI driver */
    skpACPIDriver = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
    if(skpACPIDriver == NULL)
    {
        KERNEL_ERROR("Failed to retreive the LAPIC ACPI driver.\n");
        retCode = OS_ERR_NULL_POINTER;
        goto ATTACH_END;
    }

    /* Get the LAPIC base address */
    sDrvCtrl.basePhysAddr = skpACPIDriver->pGetLAPICBaseAddress();

    /* Get the LAPICs */
    KERNEL_DEBUG(LAPIC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Attaching %d LAPICs with base address 0x%p",
                 skpACPIDriver->pGetLAPICCount(),
                 sDrvCtrl.basePhysAddr);

    /* Get the LAPIC list */
    sDrvCtrl.pLapicList = skpACPIDriver->pGetLAPICList();

    /* Enable all interrupts */
    _lapicWrite(LAPIC_TPR, 0);

    /* Set logical destination mode */
    _lapicWrite(LAPIC_DFR, 0xffffffff);
    _lapicWrite(LAPIC_LDR, 0x01000000);

    /* Set spurious inetrrupt vector */
    _lapicWrite(LAPIC_SVR, 0x100 | SPURIOUS_INT_LINE);

#if LAPIC_DEBUG_ENABLED
    kpLAPICNode = sDrvCtrl.pLapicList;
    while(kpLAPICNode != NULL)
    {
        KERNEL_DEBUG(LAPIC_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Attaching LAPIC with ID %d at CPU %d",
                     kpLAPICNode->lapic.lapicId,
                     kpLAPICNode->lapic.cpuId);
        /* Go to next */
        kpLAPICNode = kpLAPICNode->pNext;
    }
#endif

ATTACH_END:
    if(retCode == OS_NO_ERR)
    {
        /* Set the API driver */
        retCode = driverManagerSetDeviceData(pkFdtNode, &sAPIDriver);
    }
    else
    {
        KERNEL_ERROR("Failed to attach LAPIC driver. Error %d.\n", retCode);
    }

    KERNEL_DEBUG(LAPIC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "LAPIC Initialization end");

    KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_ENABLED,
                       TRACE_X86_LAPIC_ATTACH_EXIT,
                       1,
                       (uint32_t)retCode);

    return retCode;
}

static void _lapicSetIrqEOI(const uint32_t kInterruptLine)
{
    KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_ENABLED,
                       TRACE_X86_LAPIC_SET_IRQ_EOI_ENTRY,
                       1,
                       kIrqNumber);

    LAPIC_ASSERT(kInterruptLine < MAX_INTERRUPT_LINE,
                 "Could not EOI IRQ (IRQ line to big)",
                 OS_ERR_NO_SUCH_IRQ);

    _lapicWrite(LAPIC_EOI, 0);

    KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_ENABLED,
                       TRACE_X86_LAPIC_SET_IRQ_EOI_EXIT,
                       1,
                       kIrqNumber);
}

uintptr_t _lapicGetBaseAddress(void)
{
    return sDrvCtrl.basePhysAddr;
}

inline static uint32_t _lapicRead(const uint32_t kRegister)
{
    return _mmioRead32((void*)(sDrvCtrl.basePhysAddr + kRegister));
}

inline static void _lapicWrite(const uint32_t kRegister, const uint32_t kVal)
{
    _mmioWrite32((void*)(sDrvCtrl.basePhysAddr + kRegister), kVal);
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG(sX86LAPICDriver);

/************************************ EOF *************************************/