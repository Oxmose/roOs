/*******************************************************************************
 * @file lapic-timer.c
 *
 * @see lapic-timer.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 06/06/2024
 *
 * @version 1.0
 *
 * @brief Local APIC (Advanced programmable interrupt controler) timer driver.
 *
 * @details Local APIC (Advanced programmable interrupt controler) timer driver.
 * Manages  the X86 LAPIC timer using the LAPIC driver. The LAPIC timer can be
 * used for main timer or auxiliary timer.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <mmio.h>         /* Memory mapped accessses */
#include <lapic.h>        /* LAPIC driver */
#include <kheap.h>        /* Kernel heap */
#include <panic.h>        /* Kernel panic */
#include <string.h>       /* Memory manipulation */
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
#include <lapic_timer.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for inetrrupt  */
#define LAPICT_FDT_INT_PROP "interrupts"
/** @brief FDT property for frequency */
#define LAPICT_FDT_SELFREQ_PROP "freq"
/** @brief FDT property for frequency divider */
#define LAPICT_FDT_DIVIDER_PROP "bus-freq-divider"
/** @brief FDT property for main timer */
#define LAPICT_FDT_ISMAIN_PROP  "is-main"
/** @brief FDT property for base-timer */
#define LAPICT_TIMER_FDT_BASE_TIMER_PROP "base-timer"
/** @brief FDT property for LAPIC */
#define LAPICT_FDT_LAPIC_NODE_PROP "lapic-node"

/** @brief LAPIC local vector table timer register's offset. */
#define LAPIC_TIMER 0x0320
/** @brief LAPIC timer initial count register's offset. */
#define LAPIC_TICR 0x0380
/** @brief LAPIC timer current count register's offset. */
#define LAPIC_TCCR 0x0390
/** @brief LAPIC timer divide configuration register's offset. */
#define LAPIC_TDCR 0x03E0
/** @brief LAPIC local vector table timer register's offset. */

/** @brief LAPIC Timer divider value : 1. */
#define LAPICT_DIVIDER_1 0xB
/** @brief LAPIC Timer divider value : 2. */
#define LAPICT_DIVIDER_2 0x0
/** @brief LAPIC Timer divider value : 4. */
#define LAPICT_DIVIDER_4 0x1
/** @brief LAPIC Timer divider value : 8. */
#define LAPICT_DIVIDER_8 0x2
/** @brief LAPIC Timer divider value : 16. */
#define LAPICT_DIVIDER_16 0x3
/** @brief LAPIC Timer divider value : 32. */
#define LAPICT_DIVIDER_32 0x8
/** @brief LAPIC Timer divider value : 64. */
#define LAPICT_DIVIDER_64 0x9
/** @brief LAPIC Timer divider value : 128. */
#define LAPICT_DIVIDER_128 0xA

/** @brief LAPIC Timer mode flag: periodic. */
#define LAPIC_TIMER_MODE_PERIODIC 0x20000

/** @brief LAPIC Timer vector interrupt mask. */
#define LAPIC_LVT_INT_MASKED 0x10000

/** @brief Calibration time in NS: 10ms */
#define LAPICT_CALIBRATION_DELAY 1000000

/** @brief Current module name */
#define MODULE_NAME "X86 LAPIC TIMER"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 LAPIC Timer driver controler. */
typedef struct
{
    /** @brief LAPIC Timer interrupt number. */
    uint8_t interruptNumber;

    /** @brief LAPIC Timer internal frequency. */
    uint32_t internalFrequency;

    /** @brief Selected interrupt frequency. */
    uint32_t selectedFrequency;

    /** @brief Bus frequency divider. */
    uint32_t divider;

    /** @brief Frequency range low. */
    uint32_t frequencyLow;

    /** @brief Frequency range low. */
    uint32_t frequencyHigh;

    /** @brief Keeps track on the LAPIC Timer enabled state. */
    uint32_t disabledNesting;

    /** @brief LAPIC base addresss */
    uintptr_t lapicBaseAddress;

    /** @brief Driver's lock */
    kernel_spinlock_t lock;
} lapic_timer_controler_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the LAPIC TIMER to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the LAPIC Timer to ensure correctness of
 * execution. Due to the critical nature of the LAPIC Timer, any error generates
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
#define GET_CONTROLER(PTR) ((lapic_timer_controler_t*)PTR)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Attaches the LAPIC Timer driver to the system.
 *
 * @details Attaches the LAPIC Timer driver to the system. This function will
 * use the FDT to initialize the LAPIC Timer hardware and retreive the LAPIC
 * Timer parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _lapicTimerAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Calibrates the LAPIC Timer frequency.
 *
 * @details Calibrates the LAPIC Timer frequency. The LAPIC Timer has a base
 * frequency that needs to be detected. We use an alternate time base to
 * calculate it.
 *
 * @param[in] pDrvCtrl The LAPIC Timer controller.
 * @param[in] pTimeBaseDriver The time base driver to use for the qualibration,
 * it must support the getTimeNS function.
 *
 * @return The success state or error code.
 */
static OS_RETURN_E _lapicTimerCalibrate(lapic_timer_controler_t* pDrvCtrl,
                                        kernel_timer_t* pTimeBaseDriver);

/**
 * @brief Initial LAPIC Timer interrupt handler.
 *
 * @details LAPIC Timer interrupt handler set at the initialization of the LAPIC
 * Timer. Dummy routine setting EOI.
 *
 * @param[in] pCurrThread Unused, the current thread at the
 * interrupt.
 */
static void _lapicTimerDummyHandler(kernel_thread_t* pCurrThread);

/**
 * @brief Enables LAPIC Timer ticks.
 *
 * @details Enables LAPIC Timer ticks by clearing the LAPIC Timer's IRQ mask.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 */
static void _lapicTimerEnable(void* pDrvCtrl);

/**
 * @brief Disables LAPIC Timer ticks.
 *
 * @details Disables LAPIC Timer ticks by setting the LAPIC Timer's IRQ mask.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 */
static void _lapicTimerDisable(void* pDrvCtrl);

/**
 * @brief Sets the LAPIC Timer's tick frequency.
 *
 * @details Sets the LAPIC Timer's tick frequency. The value must be between the
 * LAPIC Timer frequency range.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 * @param[in] kFreq The new frequency to be set to the LAPIC Timer.
 *
 * @warning The value must be between in the LAPIC Timer frequency range.
 */
static void _lapicTimerSetFrequency(void* pDrvCtrl, const uint32_t kFreq);

/**
 * @brief Returns the LAPIC Timer tick frequency in Hz.
 *
 * @details Returns the LAPIC Timer tick frequency in Hz.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The LAPIC Timer tick frequency in Hz.
 */
static uint32_t _lapicTimerGetFrequency(void* pDrvCtrl);

/**
 * @brief Sets the LAPIC Timer tick handler.
 *
 * @details Sets the LAPIC Timer tick handler. This function will be called at
 * each LAPIC Timer tick received.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 * @param[in] pHandler The handler of the LAPIC Timer interrupt.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_NULL_POINTER is returned if the handler is NULL.
 * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the LAPIC Timer
 * interrupt line is not allowed.
 * - OS_ERR_NULL_POINTER is returned if the pointer to the handler is NULL.
 * - OS_ERR_INTERRUPT_ALREADY_REGISTERED is returned if a handler is already
 * registered for the LAPIC Timer.
 */
static OS_RETURN_E _lapicTimerSetHandler(void* pDrvCtrl,
                                  void(*pHandler)(kernel_thread_t*));

/**
 * @brief Removes the LAPIC Timer tick handler.
 *
 * @details Removes the LAPIC Timer tick handler.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the LAPIC Timer interrupt
 * line is not allowed.
 * - OS_ERR_INTERRUPT_NOT_REGISTERED is returned if the LAPIC Timer line has no
 * handler attached.
 */
static OS_RETURN_E _lapicTimerRemoveHandler(void* pDrvCtrl);

/**
 * @brief Acknowledge interrupt.
 *
 * @details Acknowledge interrupt.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 */
static void _lapicTimerAckInterrupt(void* pDrvCtrl);

/**
 * @brief Reads into the LAPIC controller memory.
 *
 * @details Reads into the LAPIC controller memory.
 *
 * @param[in] kBasePhysAddr The LAPIC base address in memory.
 * @param[in] kRegister The register to read.
 *
 * @return The value contained in the register.
 */
inline static uint32_t _lapicTimerRead(const uintptr_t kBasePhysAddr,
                                       const uint32_t kRegister);

/**
 * @brief Writes to the LAPIC controller memory.
 *
 * @details Writes to the LAPIC controller memory.
 *
 * @param[in] kBasePhysAddr The LAPIC base address in memory.
 * @param[in] kRegister The register to write.
 * @param[in] kVal The value to write to the register.
 */
inline static void _lapicTimerWrite(const uintptr_t kBasePhysAddr,
                                    const uint32_t kRegister,
                                    const uint32_t kVal);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief LAPIC Timer driver instance. */
static driver_t sX86LAPICTDriver = {
    .pName         = "X86 LAPIC Timer Driver",
    .pDescription  = "X86 LAPIC Timer Driver for UTK",
    .pCompatible   = "x86,x86-lapic-timer",
    .pVersion      = "1.0",
    .pDriverAttach = _lapicTimerAttach
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _lapicTimerAttach(const fdt_node_t* pkFdtNode)
{
    const uint32_t*          kpUintProp;
    size_t                   propLen;
    OS_RETURN_E              retCode;
    lapic_timer_controler_t* pDrvCtrl;
    kernel_timer_t*          pTimerDrv;
    kernel_timer_t*          pBaseTimer;
    lapic_driver_t*          pLapicDriver;

    KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_TIMER_ENABLED,
                       TRACE_X86_LAPIC_TIMER_ATTACH_ENTRY,
                       0);

    pDrvCtrl  = NULL;
    pTimerDrv = NULL;

    retCode = OS_NO_ERR;

    /* Init structures */
    pDrvCtrl = kmalloc(sizeof(lapic_timer_controler_t));
    if(pDrvCtrl == NULL)
    {
        KERNEL_ERROR("Failed to allocate LAPIC Timer driver controler.\n");
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pDrvCtrl, 0, sizeof(lapic_timer_controler_t));
    KERNEL_SPINLOCK_INIT(pDrvCtrl->lock);

    pTimerDrv = kmalloc(sizeof(kernel_timer_t));
    if(pTimerDrv == NULL)
    {
        KERNEL_ERROR("Failed to allocate LAPIC Timer driver instance.\n");
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }

    pTimerDrv->pGetFrequency  = _lapicTimerGetFrequency;
    pTimerDrv->pSetFrequency  = _lapicTimerSetFrequency;
    pTimerDrv->pGetTimeNs     = NULL;
    pTimerDrv->pSetTimeNs     = NULL;
    pTimerDrv->pGetDate       = NULL;
    pTimerDrv->pGetDaytime    = NULL;
    pTimerDrv->pEnable        = _lapicTimerEnable;
    pTimerDrv->pDisable       = _lapicTimerDisable;
    pTimerDrv->pSetHandler    = _lapicTimerSetHandler;
    pTimerDrv->pRemoveHandler = _lapicTimerRemoveHandler;
    pTimerDrv->pTickManager   = _lapicTimerAckInterrupt;
    pTimerDrv->pDriverCtrl    = pDrvCtrl;

    /* Get interrupt lines */
    kpUintProp = fdtGetProp(pkFdtNode, LAPICT_FDT_INT_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
    {
        KERNEL_ERROR("Failed to retreive the IRQ from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->interruptNumber = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));

    KERNEL_DEBUG(LAPICT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Interrupt: %d",
                 pDrvCtrl->interruptNumber);

    /* Get selected frequency */
    kpUintProp = fdtGetProp(pkFdtNode, LAPICT_FDT_SELFREQ_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        KERNEL_ERROR("Failed to retreive the selected frequency from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->selectedFrequency = FDTTOCPU32(*kpUintProp);

    /* Get bus frequency divider */
    kpUintProp = fdtGetProp(pkFdtNode, LAPICT_FDT_DIVIDER_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        KERNEL_ERROR("Failed to retreive the bus divider from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->divider = FDTTOCPU32(*kpUintProp);
    switch(pDrvCtrl->divider)
    {
        case 1:
            pDrvCtrl->divider = LAPICT_DIVIDER_1;
            break;
        case 2:
            pDrvCtrl->divider = LAPICT_DIVIDER_2;
            break;
        case 4:
            pDrvCtrl->divider = LAPICT_DIVIDER_4;
            break;
        case 8:
            pDrvCtrl->divider = LAPICT_DIVIDER_8;
            break;
        case 16:
            pDrvCtrl->divider = LAPICT_DIVIDER_16;
            break;
        case 32:
            pDrvCtrl->divider = LAPICT_DIVIDER_32;
            break;
        case 64:
            pDrvCtrl->divider = LAPICT_DIVIDER_64;
            break;
        case 128:
            pDrvCtrl->divider = LAPICT_DIVIDER_128;
            break;
        default:
            KERNEL_ERROR("Unsuported frequency divider, please use: \n"
                         "1, 2, 4, 8, 16, 32, 64, 128\n");
            retCode = OS_ERR_INCORRECT_VALUE;
            goto ATTACH_END;
    }

    KERNEL_DEBUG(LAPICT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Selected Frequency: %dHz",
                 pDrvCtrl->selectedFrequency);

    /* Get the LAPIC pHandle */
    kpUintProp = fdtGetProp(pkFdtNode, LAPICT_FDT_LAPIC_NODE_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        KERNEL_ERROR("Failed to retreive the LAPIC handle FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Get the LAPIC driver */
    pLapicDriver = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
    if(pLapicDriver == NULL)
    {
        KERNEL_ERROR("Failed to retreive the LAPIC driver.\n");
        retCode = OS_ERR_NULL_POINTER;
        goto ATTACH_END;
    }

    /* Get the base timer pHandle */
    kpUintProp = fdtGetProp(pkFdtNode,
                           LAPICT_TIMER_FDT_BASE_TIMER_PROP,
                           &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        KERNEL_ERROR("Failed to retreive the base timer handle FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Get the base timer driver */
    pBaseTimer = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
    if(pBaseTimer == NULL)
    {
        KERNEL_ERROR("Failed to retreive the base timer driver.\n");
        retCode = OS_ERR_NULL_POINTER;
        goto ATTACH_END;
    }
    if(pBaseTimer->pGetTimeNs == NULL)
    {
        KERNEL_ERROR("Base timer driver does not support getTimeNS.\n");
        retCode = OS_ERR_NOT_SUPPORTED;
        goto ATTACH_END;
    }

    /* Set the base address */
    pDrvCtrl->lapicBaseAddress = pLapicDriver->pGetBaseAddress();

    /* Init system times */
    pDrvCtrl->disabledNesting = 1;

    /* Calibrate the LAPIC Timer */
    _lapicTimerCalibrate(pDrvCtrl, pBaseTimer);

    /* Set LAPIC Timer frequency */
    _lapicTimerSetFrequency(pDrvCtrl, pDrvCtrl->selectedFrequency);

    /* Set interrupt EOI */
    _lapicTimerAckInterrupt(pDrvCtrl);

    /* Check if we should register as main timer */
    if(fdtGetProp(pkFdtNode, LAPICT_FDT_ISMAIN_PROP, &propLen) != NULL)
    {
        retCode = timeMgtAddTimer(pTimerDrv, MAIN_TIMER);
        if(retCode != OS_NO_ERR)
        {
            KERNEL_ERROR("Failed to set LAPIC Timer driver as main timer."
                         " Error %d\n",
                         retCode);
            goto ATTACH_END;
        }
    }
    else
    {
        retCode = timeMgtAddTimer(pTimerDrv, AUX_TIMER);
        if(retCode != OS_NO_ERR)
        {
            KERNEL_ERROR("Failed to set LAPIC Timer driver as aux timer."
                         " Error %d\n",
                         retCode);
            goto ATTACH_END;
        }
    }

ATTACH_END:
    if(retCode != OS_NO_ERR)
    {
        KERNEL_ERROR("Failed to attach LAPIC Timer. Error %d.\n", retCode);
        if(pDrvCtrl != NULL)
        {
            kfree(pDrvCtrl);
        }
        if(pTimerDrv != NULL)
        {
            kfree(pTimerDrv);
        }
    }

    KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_TIMER_ENABLED,
                       TRACE_X86_LAPIC_TIMER_ATTACH_EXIT,
                       1,
                       (uint32_t)retCode);

    KERNEL_DEBUG(LAPICT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "LAPIC Timer Initialization end");
    return retCode;
}

static OS_RETURN_E _lapicTimerCalibrate(lapic_timer_controler_t* pDrvCtrl,
                                        kernel_timer_t* pTimeBaseDriver)
{
    uint64_t  startTime;
    uint64_t  endTime;
    uint64_t  period;
    uint32_t  lapicTimerCount;
    uintptr_t kLapicBaseAddress;

    kLapicBaseAddress = pDrvCtrl->lapicBaseAddress;

    /* Set the LAPIC Timer frequency divider */
    _lapicTimerWrite(kLapicBaseAddress, LAPIC_TDCR, pDrvCtrl->divider);

    /* Write the initial count to the counter */
    _lapicTimerWrite(kLapicBaseAddress, LAPIC_TICR, 0xFFFFFFFF);

    /* Get start time */
    startTime = pTimeBaseDriver->pGetTimeNs(pTimeBaseDriver->pDriverCtrl);

    /* Wait a little bit */
    do
    {
        endTime = pTimeBaseDriver->pGetTimeNs(pTimeBaseDriver->pDriverCtrl);
    } while(endTime < startTime + LAPICT_CALIBRATION_DELAY);

    /* Now that we waited LAPICT_CALIBRATION_DELAY ns calculate the frequency */
    lapicTimerCount = 0xFFFFFFFF - _lapicTimerRead(kLapicBaseAddress,
                                                   LAPIC_TCCR);

    /* If the period is smaller than the tick count, we cannot calibrate */
    period = (endTime - startTime);
    if(period < lapicTimerCount)
    {
        KERNEL_ERROR("LAPIC Timer calibration base timer not precise enough\n");
        return OS_ERR_OUT_OF_BOUND;
    }

    /* Get the actual frequency and compute the interrupt count */
    pDrvCtrl->internalFrequency = 1000000000 / (period / lapicTimerCount);

    KERNEL_DEBUG(LAPICT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "LAPIC Timer calibration\n"
                 "\tPeriod %lluns\n"
                 "\tCount %d\n"
                 "\tTick %lluns\n"
                 "\tFrequency %u\n"
                 "\tSelected counter initial value: %u\n",
                 period,
                 lapicTimerCount,
                 period / (lapicTimerCount),
                 pDrvCtrl->internalFrequency);

    return OS_NO_ERR;
}

static void _lapicTimerDummyHandler(kernel_thread_t* pCurrThread)
{
    (void)pCurrThread;

    PANIC(OS_ERR_UNAUTHORIZED_ACTION,
          MODULE_NAME,
          "LAPIC Timer Dummy handler called",
          TRUE);

    return;
}

static void _lapicTimerEnable(void* pDrvCtrl)
{
    lapic_timer_controler_t* pLAPICTimerCtrl;
    uint32_t                 lapicInitCount;

    pLAPICTimerCtrl = GET_CONTROLER(pDrvCtrl);

    KERNEL_SPINLOCK_LOCK(pLAPICTimerCtrl->lock);
    /* Set the frequency to set the init counter */
    lapicInitCount = pLAPICTimerCtrl->internalFrequency /
                     pLAPICTimerCtrl->selectedFrequency;

    /* Write the initial count to the counter */
    _lapicTimerWrite(pLAPICTimerCtrl->lapicBaseAddress,
                     LAPIC_TICR,
                     lapicInitCount);

    /* Enable interrupts */
    _lapicTimerWrite(pLAPICTimerCtrl->lapicBaseAddress,
                     LAPIC_TIMER,
                     pLAPICTimerCtrl->interruptNumber |
                     LAPIC_TIMER_MODE_PERIODIC);

    KERNEL_SPINLOCK_UNLOCK(pLAPICTimerCtrl->lock);
}

static void _lapicTimerDisable(void* pDrvCtrl)
{
    lapic_timer_controler_t* pLAPICTimerCtrl;

    pLAPICTimerCtrl = GET_CONTROLER(pDrvCtrl);

    KERNEL_SPINLOCK_LOCK(pLAPICTimerCtrl->lock);

    /* Disable interrupt */
    _lapicTimerWrite(pLAPICTimerCtrl->lapicBaseAddress,
                     LAPIC_TIMER,
                     LAPIC_LVT_INT_MASKED);

    /* Set counter to 0 */
    _lapicTimerWrite(pLAPICTimerCtrl->lapicBaseAddress, LAPIC_TICR, 0);

    KERNEL_SPINLOCK_UNLOCK(pLAPICTimerCtrl->lock);
}

static void _lapicTimerSetFrequency(void* pDrvCtrl, const uint32_t kFreq)
{
    lapic_timer_controler_t* pLAPICTimerCtrl;
    uint32_t                 lapicInitCount;


    if(kFreq == 0)
    {
        KERNEL_ERROR("LAPIC Timer selected frequency is too low");
        return;
    }

    pLAPICTimerCtrl = GET_CONTROLER(pDrvCtrl);

    KERNEL_SPINLOCK_LOCK(pLAPICTimerCtrl->lock);

    lapicInitCount = pLAPICTimerCtrl->internalFrequency /
                     kFreq;

    if(lapicInitCount == 0)
    {
        KERNEL_SPINLOCK_UNLOCK(pLAPICTimerCtrl->lock);
        KERNEL_ERROR("LAPIC Timer selected frequency is too high");
        return;
    }


    /* Write the initial count to the counter */
    _lapicTimerWrite(pLAPICTimerCtrl->lapicBaseAddress,
                     LAPIC_TICR,
                     lapicInitCount);
    pLAPICTimerCtrl->selectedFrequency = kFreq;

    KERNEL_SPINLOCK_UNLOCK(pLAPICTimerCtrl->lock);
}

static uint32_t _lapicTimerGetFrequency(void* pDrvCtrl)
{
    lapic_timer_controler_t* pLAPICTimerCtrl;

    pLAPICTimerCtrl = GET_CONTROLER(pDrvCtrl);

    return pLAPICTimerCtrl->selectedFrequency;
}

static OS_RETURN_E _lapicTimerSetHandler(void* pDrvCtrl,
                                         void(*pHandler)(kernel_thread_t*))
{
    OS_RETURN_E      err;
    lapic_timer_controler_t* pLapicTimerCtrl;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_TIMER_ENABLED,
                       TRACE_X86_LAPIC_TIMER_SET_HANDLER_ENTRY,
                       2,
                       0,
                       (uintptr_t)pHandler & 0xFFFFFFFF);
#else
    KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_TIMER_ENABLED,
                       TRACE_X86_LAPIC_TIMER_SET_HANDLER_ENTRY,
                       2,
                       (uintptr_t)pHandler >> 32,
                       (uintptr_t)pHandler & 0xFFFFFFFF);
#endif

    if(pHandler == NULL)
    {
        KERNEL_ERROR("Tried to set LAPIC Timer handler to NULL.\n");

#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_TIMER_ENABLED,
                           TRACE_X86_LAPIC_TIMER_SET_HANDLER_EXIT,
                           3,
                           0,
                           (uintptr_t)pHandler & 0xFFFFFFFF,
                           (uint32_t)OS_ERR_NULL_POINTER);
#else
        KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_TIMER_ENABLED,
                           TRACE_X86_LAPIC_TIMER_SET_HANDLER_EXIT,
                           3,
                           (uintptr_t)pHandler >> 32,
                           (uintptr_t)pHandler & 0xFFFFFFFF,
                           (uint32_t)OS_ERR_NULL_POINTER);
#endif

        return OS_ERR_NULL_POINTER;
    }

    pLapicTimerCtrl = GET_CONTROLER(pDrvCtrl);

    _lapicTimerDisable(pDrvCtrl);

    KERNEL_SPINLOCK_LOCK(pLapicTimerCtrl->lock);

    err = interruptRegister(pLapicTimerCtrl->interruptNumber, pHandler);
    if(err != OS_NO_ERR)
    {
        KERNEL_SPINLOCK_UNLOCK(pLapicTimerCtrl->lock);
        KERNEL_ERROR("Failed to register LAPIC Timer irqHandler. Error: %d\n",
                     err);

#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_TIMER_ENABLED,
                           TRACE_X86_LAPIC_TIMER_SET_HANDLER_EXIT,
                           3,
                           0,
                           (uintptr_t)pHandler & 0xFFFFFFFF,
                           (uint32_t)err);
#else
        KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_TIMER_ENABLED,
                           TRACE_X86_LAPIC_TIMER_SET_HANDLER_EXIT,
                           3,
                           (uintptr_t)pHandler >> 32,
                           (uintptr_t)pHandler & 0xFFFFFFFF,
                           (uint32_t)err);
#endif

        return err;
    }

    KERNEL_SPINLOCK_UNLOCK(pLapicTimerCtrl->lock);

    KERNEL_DEBUG(LAPICT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "New LAPIC TIMER handler set 0x%p",
                 pHandler);

    _lapicTimerEnable(pDrvCtrl);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_TIMER_ENABLED,
                       TRACE_X86_LAPIC_TIMER_SET_HANDLER_EXIT,
                       3,
                       0,
                       (uintptr_t)pHandler & 0xFFFFFFFF,
                       (uint32_t)err);
#else
    KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_TIMER_ENABLED,
                       TRACE_X86_LAPIC_TIMER_SET_HANDLER_EXIT,
                       3,
                       (uintptr_t)pHandler >> 32,
                       (uintptr_t)pHandler & 0xFFFFFFFF,
                       (uint32_t)err);
#endif

    return err;
}

static OS_RETURN_E _lapicTimerRemoveHandler(void* pDrvCtrl)
{
    KERNEL_DEBUG(PIT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Default LAPCI Timer handler set 0x%p",
                 _lapicTimerDummyHandler);

    KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_TIMER_ENABLED,
                       TRACE_X86_LAPIC_TIMER_REMOVE_HANDLER,
                       0);
    return _lapicTimerSetHandler(pDrvCtrl, _lapicTimerDummyHandler);
}

static void _lapicTimerAckInterrupt(void* pDrvCtrl)
{
    lapic_timer_controler_t* pLapicTimerCtrl;

    pLapicTimerCtrl = GET_CONTROLER(pDrvCtrl);

    KERNEL_TRACE_EVENT(TRACE_X86_LAPIC_TIMER_ENABLED,
                       TRACE_X86_LAPIC_TIMER_ACK_INTERRUPT,
                       0);

    /* Set EOI */
    interruptIRQSetEOI(pLapicTimerCtrl->interruptNumber);
}

inline static uint32_t _lapicTimerRead(const uintptr_t kBasePhysAddr,
                                       const uint32_t kRegister)
{
    return _mmioRead32((void*)(kBasePhysAddr + kRegister));
}

inline static void _lapicTimerWrite(const uintptr_t kBasePhysAddr,
                                    const uint32_t kRegister,
                                    const uint32_t kVal)
{
    _mmioWrite32((void*)(kBasePhysAddr + kRegister), kVal);
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG(sX86LAPICTDriver);

/************************************ EOF *************************************/