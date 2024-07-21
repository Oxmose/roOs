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
#include <syslog.h>       /* Kernel Syslog */
#include <critical.h>     /* Kernel critical locks */
#include <time_mgt.h>     /* Timers manager */
#include <core_mgt.h>     /* Core manager */
#include <drivermgr.h>    /* Driver manager */
#include <interrupts.h>   /* Interrupt manager */

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

    /** @brief LAPIC Timer internal frequency. One per CPU */
    uint32_t internalFrequency[SOC_CPU_COUNT];

    /** @brief Selected interrupt frequency. */
    uint32_t selectedFrequency;

    /** @brief Bus frequency divider. */
    uint32_t divider;

    /** @brief Keeps track on the LAPIC Timer enabled state. One per CPU */
    uint32_t disabledNesting[SOC_CPU_COUNT];

    /** @brief LAPIC base addresss */
    uintptr_t lapicBaseAddress;

    /** @brief Time base driver */
    const kernel_timer_t* kpBaseTimer;
} lapic_timer_ctrl_t;

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
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/** @brief Cast a pointer to a LAPIC driver controler */
#define GET_CONTROLER(PTR) ((lapic_timer_ctrl_t*)PTR)

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
 * @param[in] kCpuId The LAPIC Timer CPU id to calibrate
 *
 * @return The success state or error code.
 */
static OS_RETURN_E _lapicTimerCalibrate(const uint8_t kCpuId);

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
 * @param[in] kFreq The new frequency to be set to the LAPIC Timer.
 * @param[in] kCpuId The CPU to set the LAPIC timer of.
 *
 * @warning The value must be between in the LAPIC Timer frequency range.
 */
static void _lapicTimerSetFrequency(const uint32_t kFreq, const uint8_t kCpuId);

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
 * - OS_ERR_ALREADY_EXIST is returned if a handler is already
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
 * @brief Initializes a secondary core LAPIC Timer.
 *
 * @details Initializes a secondary core LAPIC Timer. This function
 * initializes the secondary core LAPIC timer interrupts and settings.
 * @param[in] kCpuId The CPU identifier for which we should enable the LAPIC
 * timer.
 */
static void _lapicTimerInitApCore(const uint8_t kCpuId);

/**
 * @brief Reads into the LAPIC controller memory.
 *
 * @details Reads into the LAPIC controller memory.
 *
 * @param[in] kBaseAddr The LAPIC base address in memory.
 * @param[in] kRegister The register to read.
 *
 * @return The value contained in the register.
 */
static inline uint32_t _lapicTimerRead(const uintptr_t kBaseAddr,
                                       const uint32_t  kRegister);

/**
 * @brief Writes to the LAPIC controller memory.
 *
 * @details Writes to the LAPIC controller memory.
 *
 * @param[in] kBaseAddr The LAPIC base address in memory.
 * @param[in] kRegister The register to write.
 * @param[in] kVal The value to write to the register.
 */
static inline void _lapicTimerWrite(const uintptr_t kBaseAddr,
                                    const uint32_t  kRegister,
                                    const uint32_t  kVal);

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
    .pDescription  = "X86 LAPIC Timer Driver for roOs",
    .pCompatible   = "x86,x86-lapic-timer",
    .pVersion      = "1.0",
    .pDriverAttach = _lapicTimerAttach
};

/** @brief LAPIC Timer API driver instance */
static lapic_timer_driver_t sAPIDriver = {
    .pInitApCore = _lapicTimerInitApCore
};

/** @brief Local timer controller instance, used by AP core */
static lapic_timer_ctrl_t* spDrvCtrl;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _lapicTimerAttach(const fdt_node_t* pkFdtNode)
{
    const uint32_t*     kpUintProp;
    size_t              propLen;
    OS_RETURN_E         retCode;
    lapic_timer_ctrl_t* pDrvCtrl;
    kernel_timer_t*     pTimerDrv;
    lapic_driver_t*     pLapicDriver;

    pDrvCtrl  = NULL;
    pTimerDrv = NULL;

    retCode = OS_NO_ERR;

    /* Init structures */
    pDrvCtrl = kmalloc(sizeof(lapic_timer_ctrl_t));
    if(pDrvCtrl == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    spDrvCtrl = pDrvCtrl;
    memset(pDrvCtrl, 0, sizeof(lapic_timer_ctrl_t));

    pTimerDrv = kmalloc(sizeof(kernel_timer_t));
    if(pTimerDrv == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pTimerDrv, 0, sizeof(kernel_timer_t));
    pTimerDrv->pGetFrequency  = _lapicTimerGetFrequency;
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
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->interruptNumber = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));

#if LAPICT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Interrupt: %d",
           pDrvCtrl->interruptNumber);
#endif

    /* Get selected frequency */
    kpUintProp = fdtGetProp(pkFdtNode, LAPICT_FDT_SELFREQ_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->selectedFrequency = FDTTOCPU32(*kpUintProp);

    /* Get bus frequency divider */
    kpUintProp = fdtGetProp(pkFdtNode, LAPICT_FDT_DIVIDER_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
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
            retCode = OS_ERR_INCORRECT_VALUE;
            goto ATTACH_END;
    }

#if LAPICT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Selected Frequency: %dHz",
           pDrvCtrl->selectedFrequency);
#endif

    /* Get the LAPIC pHandle */
    kpUintProp = fdtGetProp(pkFdtNode, LAPICT_FDT_LAPIC_NODE_PROP, &propLen);
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

    /* Get the base timer pHandle */
    kpUintProp = fdtGetProp(pkFdtNode,
                           LAPICT_TIMER_FDT_BASE_TIMER_PROP,
                           &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Get the base timer driver */
    pDrvCtrl->kpBaseTimer = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
    if(pDrvCtrl->kpBaseTimer == NULL)
    {
        retCode = OS_ERR_NULL_POINTER;
        goto ATTACH_END;
    }
    if(pDrvCtrl->kpBaseTimer->pGetTimeNs == NULL)
    {
        retCode = OS_ERR_NOT_SUPPORTED;
        goto ATTACH_END;
    }

    /* Set the base address */
    pDrvCtrl->lapicBaseAddress = pLapicDriver->pGetBaseAddress();

    /* Init system times */
    pDrvCtrl->disabledNesting[0] = 1;

    /* Calibrate the LAPIC Timer */
    _lapicTimerCalibrate(0);

    /* Set LAPIC Timer frequency */
    _lapicTimerSetFrequency(pDrvCtrl->selectedFrequency, 0);

    /* Set interrupt EOI */
    _lapicTimerAckInterrupt(pDrvCtrl);

    /* Register the driver in the core manager */
    coreMgtRegLapicTimerDriver(&sAPIDriver);

    /* Set the API driver */
    retCode = driverManagerSetDeviceData(pkFdtNode, pTimerDrv);

ATTACH_END:
    if(retCode != OS_NO_ERR)
    {
        if(pDrvCtrl != NULL)
        {
            kfree(pDrvCtrl);
        }
        if(pTimerDrv != NULL)
        {
            kfree(pTimerDrv);
        }
        driverManagerSetDeviceData(pkFdtNode, NULL);
    }

#if LAPICT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "LAPIC Timer Initialization end");
#endif

    return retCode;
}

static OS_RETURN_E _lapicTimerCalibrate(const uint8_t kCpuId)
{
    uint64_t              startTime;
    uint64_t              endTime;
    uint64_t              period;
    uint32_t              lapicTimerCount;
    uintptr_t             kLapicBaseAddress;
    const kernel_timer_t* kpBaseTimer;

    kLapicBaseAddress = spDrvCtrl->lapicBaseAddress;
    kpBaseTimer = spDrvCtrl->kpBaseTimer;

    /* Set the LAPIC Timer frequency divider */
    _lapicTimerWrite(kLapicBaseAddress, LAPIC_TDCR, spDrvCtrl->divider);

    /* Write the initial count to the counter */
    _lapicTimerWrite(kLapicBaseAddress, LAPIC_TICR, 0xFFFFFFFF);

    /* Get start time */
    startTime = kpBaseTimer->pGetTimeNs(kpBaseTimer->pDriverCtrl);

    /* Wait a little bit */
    do
    {
        endTime = kpBaseTimer->pGetTimeNs(kpBaseTimer->pDriverCtrl);
    } while(endTime < startTime + LAPICT_CALIBRATION_DELAY);

    /* Now that we waited LAPICT_CALIBRATION_DELAY ns calculate the frequency */
    lapicTimerCount = 0xFFFFFFFF - _lapicTimerRead(kLapicBaseAddress,
                                                   LAPIC_TCCR);

    /* If the period is smaller than the tick count, we cannot calibrate */
    period = (endTime - startTime);
    if(period < lapicTimerCount)
    {
        return OS_ERR_OUT_OF_BOUND;
    }

    /* Get the actual frequency and compute the interrupt count */
    spDrvCtrl->internalFrequency[kCpuId] =
        1000000000 / (period / lapicTimerCount);

#if LAPICT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "LAPIC Timer calibration\n"
           "\tPeriod %lluns\n"
           "\tCount %d\n"
           "\tTick %lluns\n"
           "\tFrequency %u\n"
           "\tSelected counter initial value: %u",
           period,
           lapicTimerCount,
           period / (lapicTimerCount),
           spDrvCtrl->internalFrequency);
#endif

    return OS_NO_ERR;
}

static void _lapicTimerDummyHandler(kernel_thread_t* pCurrThread)
{
    (void)pCurrThread;

    PANIC(OS_ERR_UNAUTHORIZED_ACTION,
          MODULE_NAME,
          "LAPIC Timer Dummy handler called");

    return;
}

static void _lapicTimerEnable(void* pDrvCtrl)
{
    lapic_timer_ctrl_t* pLAPICTimerCtrl;
    uint32_t            lapicInitCount;
    uint8_t             cpuId;
    uint32_t            intState;

    pLAPICTimerCtrl = GET_CONTROLER(pDrvCtrl);

    KERNEL_ENTER_CRITICAL_LOCAL(intState);

    cpuId = cpuGetId();

    if(pLAPICTimerCtrl->disabledNesting[cpuId] > 0)
    {
        --pLAPICTimerCtrl->disabledNesting[cpuId];
    }

#if LAPICT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Enable (nesting %d) on %d",
           pLAPICTimerCtrl->disabledNesting[cpuId], cpuId);
#endif

    if(pLAPICTimerCtrl->disabledNesting[cpuId] == 0)
    {
        /* Set the frequency to set the init counter */
        lapicInitCount = pLAPICTimerCtrl->internalFrequency[cpuId] /
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
    }

    KERNEL_EXIT_CRITICAL_LOCAL(intState);
}

static void _lapicTimerDisable(void* pDrvCtrl)
{
    lapic_timer_ctrl_t* pLAPICTimerCtrl;
    uint8_t             cpuId;
    uint32_t            intState;

    pLAPICTimerCtrl = GET_CONTROLER(pDrvCtrl);

    KERNEL_ENTER_CRITICAL_LOCAL(intState);

    cpuId = cpuGetId();

    if(pLAPICTimerCtrl->disabledNesting[cpuId] < UINT32_MAX)
    {
        ++pLAPICTimerCtrl->disabledNesting[cpuId];
    }

    /* Disable interrupt */
    _lapicTimerWrite(pLAPICTimerCtrl->lapicBaseAddress,
                     LAPIC_TIMER,
                     LAPIC_LVT_INT_MASKED);

    /* Set counter to 0 */
    _lapicTimerWrite(pLAPICTimerCtrl->lapicBaseAddress, LAPIC_TICR, 0);

    KERNEL_EXIT_CRITICAL_LOCAL(intState);
}

static void _lapicTimerSetFrequency(const uint32_t kFreq, const uint8_t kCpuId)
{
    uint32_t            lapicInitCount;

    if(kFreq == 0)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "LAPIC Timer selected frequency is too low");
        return;
    }

    lapicInitCount = spDrvCtrl->internalFrequency[kCpuId] / kFreq;

    if(lapicInitCount == 0)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "LAPIC Timer selected frequency is too high");
        return;
    }


    /* Write the initial count to the counter */
    _lapicTimerWrite(spDrvCtrl->lapicBaseAddress,
                     LAPIC_TICR,
                     lapicInitCount);
    spDrvCtrl->selectedFrequency = kFreq;
}

static uint32_t _lapicTimerGetFrequency(void* pDrvCtrl)
{
    lapic_timer_ctrl_t* pLAPICTimerCtrl;

    pLAPICTimerCtrl = GET_CONTROLER(pDrvCtrl);

    return pLAPICTimerCtrl->selectedFrequency;
}

static OS_RETURN_E _lapicTimerSetHandler(void* pDrvCtrl,
                                         void(*pHandler)(kernel_thread_t*))
{
    OS_RETURN_E         err;
    lapic_timer_ctrl_t* pLapicTimerCtrl;

    if(pHandler == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    pLapicTimerCtrl = GET_CONTROLER(pDrvCtrl);

    _lapicTimerDisable(pDrvCtrl);

    err = interruptRegister(pLapicTimerCtrl->interruptNumber, pHandler);
    if(err != OS_NO_ERR)
    {

        return err;
    }

#if LAPICT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "New LAPIC TIMER handler set 0x%p",
           pHandler);
#endif

    _lapicTimerEnable(pDrvCtrl);

    return err;
}

static OS_RETURN_E _lapicTimerRemoveHandler(void* pDrvCtrl)
{

#if LAPICT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Default LAPCI Timer handler set 0x%p",
           _lapicTimerDummyHandler);
#endif
    return _lapicTimerSetHandler(pDrvCtrl, _lapicTimerDummyHandler);
}

static void _lapicTimerAckInterrupt(void* pDrvCtrl)
{
    lapic_timer_ctrl_t* pLapicTimerCtrl;

    pLapicTimerCtrl = GET_CONTROLER(pDrvCtrl);

    /* Set EOI */
    interruptIRQSetEOI(pLapicTimerCtrl->interruptNumber);
}

static void _lapicTimerInitApCore(const uint8_t kCpuId)
{

    /* We are in a secondary core (AP core), just setup the counter as all
     * LAPIC timers should have the same frequency
     */
    spDrvCtrl->disabledNesting[kCpuId] = 1;

    /* Calibrate the timer */
    _lapicTimerCalibrate(kCpuId);

    /* Set LAPIC Timer frequency */
    _lapicTimerSetFrequency(spDrvCtrl->selectedFrequency, kCpuId);

    /* Enable the timer is needed based on the main cpu */
    if(spDrvCtrl->disabledNesting[0] == 0)
    {
        _lapicTimerEnable(spDrvCtrl);
    }

    /* Set interrupt EOI */
    _lapicTimerAckInterrupt(spDrvCtrl);
}

static inline uint32_t _lapicTimerRead(const uintptr_t kBaseAddr,
                                       const uint32_t  kRegister)
{
    return _mmioRead32((void*)(kBaseAddr + kRegister));
}

static inline void _lapicTimerWrite(const uintptr_t kBaseAddr,
                                    const uint32_t  kRegister,
                                    const uint32_t  kVal)
{
    _mmioWrite32((void*)(kBaseAddr + kRegister), kVal);
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG(sX86LAPICTDriver);

/************************************ EOF *************************************/