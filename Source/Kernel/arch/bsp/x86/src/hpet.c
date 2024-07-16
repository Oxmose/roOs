/*******************************************************************************
 * @file hpet.c
 *
 * @see hpet.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 14/07/2024
 *
 * @version 1.0
 *
 * @brief HPET (High Precision Event Timer) driver.
 *
 * @details HPET (High Precision Event Timer) driver. Timer
 * source in the kernel. This driver provides basic access to the HPET and
 * its features.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <mmio.h>         /* Memory mapped accessses */
#include <acpi.h>         /* ACPI driver */
#include <kheap.h>        /* Kernel heap */
#include <panic.h>        /* Kernel panic */
#include <string.h>       /* Memory manipulation */
#include <stdint.h>       /* Generic int types */
#include <kerror.h>       /* Kernel error */
#include <memory.h>       /* Memory manager */
#include <critical.h>     /* Kernel critical locks */
#include <time_mgt.h>     /* Timers manager */
#include <drivermgr.h>    /* Driver manager */
#include <interrupts.h>   /* Interrupt manager */
#include <kerneloutput.h> /* Kernel output manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <hpet.h>

/* Unit test header */
#include <test_framework.h>

/* Tracing feature */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for inetrrupt  */
#define HPET_FDT_INT_PROP "interrupts"
/** @brief FDT property for frequency */
#define HPET_FDT_FREQ_PROP "freq"
/** @brief FDT property for acpi handle */
#define HPET_FDT_ACPI_NODE_PROP "acpi-node"

/** @brief Defines the mask for the main counter tick period */
#define HPET_CAPABILITIES_PERIOD_MASK 0xFFFFFFFF00000000ULL
/** @brief Defines the shift for the main counter tick period */
#define HPET_CAPABILITIES_PERIOD_SHIFT 32
/** @brief Defines the mask for the size of the main counter (32/64) */
#define HPET_CAPABILITIES_SIZE_MASK 0x0000000000002000ULL
/** @brief Defines the shift for the size of the main counter (32/64) */
#define HPET_CAPABILITIES_SIZE_SHIFT 13
/** @brief Defines the mask for the number of comparators */
#define HPET_CAPABILITIES_COUNT_MASK 0x0000000000001F00ULL
/** @brief Defines the shift for the number of comparators */
#define HPET_CAPABILITIES_COUNT_SHIFT 8

/** @brief Enable count bit in the general configuration register */
#define HPET_CONFIGURATION_ENABLE_COUNT 0x1ULL

/** @brief Current module name */
#define MODULE_NAME "X86 HPET"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief The HPET comparators register structure */
typedef struct
{
    /** @brief Stores the configuration and capabilities data */
    volatile uint64_t configurationReg;

    /** @brief Stores the comparator value */
    volatile uint64_t comparatorValue;

    /** @brief Stores the FSB interrupt routing */
    volatile uint64_t interruptRouting;

    /** @brief Padding */
    volatile uint64_t padding;
} __attribute__((packed)) hpet_comparator_t;

/** @brief The HPET register structure */
typedef struct
{
    /** @brief General capabilities and ID register */
    volatile uint64_t capabilitiesId;

    /** @brief Padding */
    volatile uint64_t padding0;

    /** @brief HPET general configuration register */
    volatile uint64_t configuration;

    /** @brief Padding */
    volatile uint64_t padding1;

    /** @brief Stores the current interrupt status for the HPET */
    volatile uint64_t interruptStatus;

    /** @brief Padding */
    volatile uint8_t padding2[0xC8];

    /** @brief Stores the counter value */
    volatile uint64_t counterValue;

    /** @brief Padding */
    volatile uint64_t padding3;

    /** @brief Variable size array of the comparator registers */
    hpet_comparator_t comparator[];

} __attribute__((packed)) hpet_registers_t;

/** @brief x86 HPET Timer driver controler. */
typedef struct
{
    /** @brief HPET Timer interrupt number. */
    uint8_t interruptNumber;

    /* @brief Stores if the counter is 32bits or 64bits wide */
    bool_t countIs64Bits;

    /** @brief Number of supported comparators */
    uint8_t comparatorsCount;

    /** @brief Selected interrupt frequency. */
    uint32_t selectedFrequency;

    /** @brief Keeps track on the HPET enabled state. */
    uint32_t disabledNesting;

    /** @brief HPET registers mapped in memory */
    hpet_registers_t* pRegisters;

    /** @brief Stores the base tick period of the HPET */
    uint32_t basePeriod;

    /** @brief Time base driver */
    const kernel_timer_t* kpBaseTimer;
} hpet_ctrl_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the HPET to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the HPET to ensure correctness of
 * execution. Due to the critical nature of the HPET, any error generates
 * a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define LAPICT_ASSERT(COND, MSG, ERROR) {                   \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG, TRUE);               \
    }                                                       \
}

/** @brief Cast a pointer to a LAPIC driver controler */
#define GET_CONTROLER(PTR) ((hpet_ctrl_t*)PTR)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Attaches the HPET driver to the system.
 *
 * @details Attaches the HPET driver to the system. This function will
 * use the FDT to initialize the HPEThardware  and retreive the HPET
 * Timer parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _hpetAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Initializes the HPET.
 *
 * @details Initializes the HPET with the values stores in the HPET controller.
 * This function searched for a HPET in the ACPI and use the first found.
 * Other HPETs are not used.
 *
 * @param[in, out] pCtrl The HPET controller to initialize.
 * @param[in] pkFdtNode The FDT node used to initialize the driver.
 *
 * @return The success of error state is returned.
 */
static OS_RETURN_E _hpetInit(hpet_ctrl_t*      pCtrl,
                             const fdt_node_t* pkFdtNode);

/**
 * @brief Returns the time elasped since the last timer's reset in ns.
 *
 * @details Returns the time elasped since the last timer's reset in ns. The
 * timer can be set with the pSetTimeNs function.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The time in nanosecond since the last timer reset is returned.
 */
static uint64_t _hpetGetTimeNs(void* pDrvCtrl);

/**
 * @brief Enables the HPET comparator 0 interrupt.
 *
 * @details Enables the HPET comparator 0 interrupt. Only one comparator is
 * allowed per instance.
 *
 * @param[in, out] pDrvCtrl The HPET controller to use.
 */
static void _hpetEnable(void* pDrvCtrl);

/**
 * @brief Disables the HPET comparator 0 interrupt.
 *
 * @details Disables the HPET comparator 0 interrupt. Only one comparator is
 * allowed per instance.
 *
 * @param[in, out] pDrvCtrl The HPET controller to use.
 */
static void _hpetDisable(void* pDrvCtrl);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief HPET driver instance. */
static driver_t sX86HPETDriver = {
    .pName         = "X86 HPET Driver",
    .pDescription  = "X86 High Precision Event Timer for roOs.",
    .pCompatible   = "x86,x86-hpet",
    .pVersion      = "1.0",
    .pDriverAttach = _hpetAttach
};

/** @brief Local timer controller instance */
static hpet_ctrl_t sDrvCtrl;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _hpetAttach(const fdt_node_t* pkFdtNode)
{
    const uint32_t* kpUintProp;
    size_t          propLen;
    OS_RETURN_E     retCode;
    kernel_timer_t* pTimerDrv;

    pTimerDrv = NULL;

    retCode = OS_NO_ERR;

    /* Init structures */
    memset(&sDrvCtrl, 0, sizeof(hpet_ctrl_t));

    pTimerDrv = kmalloc(sizeof(kernel_timer_t));
    if(pTimerDrv == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pTimerDrv, 0, sizeof(kernel_timer_t));
# if 0
    pTimerDrv->pGetFrequency  = _hpetGetFrequency;
    pTimerDrv->pSetHandler    = _hpetSetHandler;
    pTimerDrv->pRemoveHandler = _hpetRemoveHandler;
    pTimerDrv->pTickManager   = _hpetAckInterrupt;
#endif
    pTimerDrv->pEnable        = _hpetEnable;
    pTimerDrv->pDisable       = _hpetDisable;
    pTimerDrv->pGetTimeNs     = _hpetGetTimeNs;
    pTimerDrv->pDriverCtrl    = &sDrvCtrl;

    /* Get interrupt lines */
    kpUintProp = fdtGetProp(pkFdtNode, HPET_FDT_INT_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.interruptNumber = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));

    KERNEL_DEBUG(HPET_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Interrupt: %d",
                 sDrvCtrl.interruptNumber);

    /* Get selected frequency */
    kpUintProp = fdtGetProp(pkFdtNode, HPET_FDT_FREQ_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.selectedFrequency = FDTTOCPU32(*kpUintProp);

    /* Initializes the HPET with the required frequency and interrupt */
    retCode = _hpetInit(&sDrvCtrl, pkFdtNode);
    if(retCode != OS_NO_ERR)
    {
        goto ATTACH_END;
    }

    /* Set the API driver */
    retCode = driverManagerSetDeviceData(pkFdtNode, pTimerDrv);

ATTACH_END:
    if(retCode != OS_NO_ERR)
    {
        if(pTimerDrv != NULL)
        {
            kfree(pTimerDrv);
        }
        driverManagerSetDeviceData(pkFdtNode, NULL);
    }

    KERNEL_DEBUG(HPET_DEBUG_ENABLED,
                 MODULE_NAME,
                 "HPET Initialization end");
    return retCode;
}

static OS_RETURN_E _hpetInit(hpet_ctrl_t*      pCtrl,
                             const fdt_node_t* pkFdtNode)
{
    const uint32_t*      kpUintProp;
    const hpet_node_t*   kpHpetNode;
    const acpi_driver_t* kpACPIDriver;
    const hpet_desc_t*   kpDesc;
    uintptr_t            basePhysAddr;
    size_t               propLen;
    size_t               hpetSize;
    OS_RETURN_E          retCode;

    /* Get the ACPI pHandle */
    kpUintProp = fdtGetProp(pkFdtNode, HPET_FDT_ACPI_NODE_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto INIT_END;
    }

    /* Get the ACPI driver */
    kpACPIDriver = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
    if(kpACPIDriver == NULL)
    {
        retCode = OS_ERR_NULL_POINTER;
        goto INIT_END;
    }

    /* Get the HPET list */
    kpHpetNode = kpACPIDriver->pGetHPETList();
    if(kpHpetNode == NULL)
    {
        retCode = OS_ERR_NOT_SUPPORTED;
        goto INIT_END;
    }

    /* We only support one HPET, get the first */
    kpDesc = &kpHpetNode->hpet;

    /* Get the HPET size */
    hpetSize = sizeof(hpet_registers_t) +
               sizeof(hpet_comparator_t) * kpDesc->comparatorCount;

    /* Align address and update size to map */
    basePhysAddr = kpDesc->address & ~PAGE_SIZE_MASK;
    hpetSize += kpDesc->address - basePhysAddr;

    /* Align size and map the memory */
    hpetSize = (hpetSize + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;
    pCtrl->pRegisters = memoryKernelMap((void*)basePhysAddr,
                                        hpetSize,
                                        MEMMGR_MAP_HARDWARE |
                                        MEMMGR_MAP_KERNEL   |
                                        MEMMGR_MAP_RW,
                                        &retCode);
    if(retCode != OS_NO_ERR)
    {
        goto INIT_END;
    }

    pCtrl->pRegisters = (void*)((uintptr_t)pCtrl->pRegisters |
                                (kpDesc->address & PAGE_SIZE_MASK));

    /* Enable the count */
    pCtrl->pRegisters->configuration |= HPET_CONFIGURATION_ENABLE_COUNT;

    /* Init the controller */
    pCtrl->basePeriod = (pCtrl->pRegisters->capabilitiesId &
                         HPET_CAPABILITIES_PERIOD_MASK) >>
                         HPET_CAPABILITIES_PERIOD_SHIFT;

    pCtrl->countIs64Bits = (pCtrl->pRegisters->capabilitiesId &
                            HPET_CAPABILITIES_SIZE_MASK) >>
                            HPET_CAPABILITIES_SIZE_SHIFT;

    pCtrl->comparatorsCount = (pCtrl->pRegisters->capabilitiesId &
                               HPET_CAPABILITIES_COUNT_MASK) >>
                               HPET_CAPABILITIES_COUNT_SHIFT;
    ++pCtrl->comparatorsCount;
    KERNEL_DEBUG(HPET_DEBUG_ENABLED,
                 MODULE_NAME,
                 "============ HPET\n"
                 "\tBase Period: %dfs\n"
                 "\tCounter Size: %d\n"
                 "\tComparators Count: %d\n"
                 "\tConfiguration: 0x%llx\n",
                 pCtrl->basePeriod,
                 pCtrl->countIs64Bits ? 64 : 32,
                 pCtrl->comparatorsCount,
                 pCtrl->pRegisters->configuration);

INIT_END:
    return retCode;
}

static uint64_t _hpetGetTimeNs(void* pDrvCtrl)
{
    uint64_t     timeTicks;
    hpet_ctrl_t* pCtrl;

    pCtrl = GET_CONTROLER(pDrvCtrl);

    /* Read the timer counter */
    timeTicks = pCtrl->pRegisters->counterValue;

    /* Now multiply the number of tick by the period */
    timeTicks *= pCtrl->basePeriod / 1000000;

    return timeTicks;
}

static void _hpetEnable(void* pDrvCtrl)
{
    (void)pDrvCtrl;
}

static void _hpetDisable(void* pDrvCtrl)
{
    (void)pDrvCtrl;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG(sX86HPETDriver);

/************************************ EOF *************************************/