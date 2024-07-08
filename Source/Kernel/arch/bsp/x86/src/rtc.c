/*******************************************************************************
 * @file rtc.c
 *
 * @see rtc.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 25/05/2024
 *
 * @version 2.0
 *
 * @brief RTC (Real Time Clock) driver.
 *
 * @details RTC (Real Time Clock) driver. Used as the kernel's time base. Timer
 * source in the kernel. This driver provides basic access to the RTC.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <kheap.h>        /* Kernel heap */
#include <panic.h>        /* Kernel panic */
#include <x86cpu.h>       /* CPU port manipulation */
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
#include <rtc.h>

/* Unit test header */
#include <test_framework.h>

/* Tracing feature */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for inetrrupt  */
#define RTC_FDT_INT_PROP        "interrupts"
/** @brief FDT property for comm ports */
#define RTC_FDT_COMM_PROP       "comm"
/** @brief FDT property for comm ports */
#define RTC_FDT_QUARTZ_PROP     "qartz-freq"
/** @brief FDT property for frequency */
#define RTC_FDT_SELFREQ_PROP    "freq"
/** @brief FDT property for frequency range */
#define RTC_FDT_FREQRANGE_PROP  "freq-range"
/** @brief FDT property for main timer */
#define RTC_FDT_ISRTC_PROP      "is-rtc"

/** @brief Initial RTC rate */
#define RTC_INIT_RATE 10

/* CMOS registers  */
/** @brief CMOS seconds register id. */
#define CMOS_SECONDS_REGISTER 0x00
/** @brief CMOS minutes register id. */
#define CMOS_MINUTES_REGISTER 0x02
/** @brief CMOS hours register id. */
#define CMOS_HOURS_REGISTER   0x04
/** @brief CMOS day of the week register id. */
#define CMOS_WEEKDAY_REGISTER 0x06
/** @brief CMOS day register id. */
#define CMOS_DAY_REGISTER     0x07
/** @brief CMOS month register id. */
#define CMOS_MONTH_REGISTER   0x08
/** @brief CMOS year register id. */
#define CMOS_YEAR_REGISTER    0x09
/** @brief CMOS century register id. */
#define CMOS_CENTURY_REGISTER 0x00

/* CMOS setings */
/** @brief CMOS NMI disabler bit. */
#define CMOS_NMI_DISABLE_BIT 0x01
/** @brief CMOS RTC enabler bit. */
#define CMOS_ENABLE_RTC      0x40
/** @brief CMOS A register id. */
#define CMOS_REG_A           0x0A
/** @brief CMOS B register id. */
#define CMOS_REG_B           0x0B
/** @brief CMOS C register id. */
#define CMOS_REG_C           0x0C

/** @brief Current module name */
#define MODULE_NAME "X86 RTC"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 RTC driver controler. */
typedef struct
{
    /** @brief CPU command port. */
    uint16_t cpuCommPort;

    /** @brief CPU data port. */
    uint16_t cpuDataPort;

    /** @brief RTC IRQ number. */
    uint8_t irqNumber;

    /** @brief Main quarts frequency. */
    uint32_t quartzFrequency;

    /** @brief Selected interrupt frequency. */
    uint32_t selectedFrequency;

    /** @brief Frequency range low. */
    uint32_t frequencyLow;

    /** @brief Frequency range low. */
    uint32_t frequencyHigh;

    /** @brief Keeps track on the RTC enabled state. */
    uint32_t disabledNesting;

    /** @brief Driver's lock */
    spinlock_t lock;
} rtc_controler_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the RTC to ensure correctness of execution.
 *
 * @details Assert macro used by the RTC to ensure correctness of execution.
 * Due to the critical nature of the RTC, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define RTC_ASSERT(COND, MSG, ERROR) {                      \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/** @brief Cast a pointer to a RTC driver controler */
#define GET_CONTROLER(PTR) ((rtc_controler_t*)PTR)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Attaches the RTC driver to the system.
 *
 * @details Attaches the RTC driver to the system. This function will use the
 * FDT to initialize the RTC hardware and retreive the RTC parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _rtcAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Initial RTC interrupt handler.
 *
 * @details RTC interrupt handler set at the initialization of the RTC.
 * Dummy routine setting EOI.
 *
 * @param[in] pCurrThread Unused, the current thread at the
 * interrupt.
 */
static void _rtcDummyHandler(kernel_thread_t* pCurrThread);

/**
 * @brief Enables RTC ticks.
 *
 * @details Enables RTC ticks by clearing the RTC's IRQ mask.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 */
static void _rtcEnable(void* pDrvCtrl);

/**
 * @brief Disables RTC ticks.
 *
 * @details Disables RTC ticks by setting the RTC's IRQ mask.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 */
static void _rtcDisable(void* pDrvCtrl);

/**
 * @brief Sets the RTC's tick frequency.
 *
 * @details Sets the RTC's tick frequency. The value must be between 2Hz and
 * 8192Hz.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 * @param[in] kFrequency The new frequency to be set to the RTC.
 *
 * @warning The value must be between 2Hz and 8192Hz. The lower boundary RTC
 * frequency will be selected (refer to the code to understand the 14 available
 * frequencies).
 */
static void _rtcSetFrequency(void* pDrvCtrl, const uint32_t kFrequency);

/**
 * @brief Returns the RTC tick frequency in Hz.
 *
 * @details Returns the RTC tick frequency in Hz.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The RTC tick frequency in Hz.
 */
static uint32_t _rtcGetFrequency(void* pDrvCtrl);

/**
 * @brief Sets the RTC tick handler.
 *
 * @details Sets the RTC tick handler. This function will be called at each RTC
 * tick received.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 * @param[in] pHandler The handler of the RTC interrupt.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_NULL_POINTER is returned if the handler is NULL.
 * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the RTC interrupt line
 *   is not allowed.
 * - OS_ERR_NULL_POINTER is returned if the pointer to the handler is NULL.
 * - OS_ERR_INTERRUPT_ALREADY_REGISTERED is returned if a handler is already
 *   registered for the RTC.
 */
static OS_RETURN_E _rtcSetHandler(void* pDrvCtrl,
                                  void(*pHandler)(kernel_thread_t*));

/**
 * @brief Removes the RTC tick handler.
 *
 * @details Removes the RTC tick handler.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the RTC interrupt line
 * is not allowed.
 * - OS_ERR_INTERRUPT_NOT_REGISTERED is returned if the RTC line has no handler
 * attached.
 */
static OS_RETURN_E _rtcRemoveHandler(void* pDrvCtrl);

/**
 * @brief Returns the current date.
 *
 * @details Returns the current date in RTC date format.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The current date in in RTC date format
 */
static date_t _rtcGetDate(void* pDrvCtrl);

/**
 * @brief Returns the current daytime in seconds.
 *
 * @details Returns the current daytime in seconds.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 *
 * @return The current daytime in seconds.
 */
static time_t _rtcGetDaytime(void* pDrvCtrl);

/**
 * @brief Updates the system's time and date.
 *
 * @details Updates the system's time and date. This function also reads the
 * CMOS registers. By doing that, the RTC registers are cleaned and the RTC able
 * to interrupt the CPU again.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 * @param[out] pDate The date to update.
 * @param[out] pTime The time to update.
 *
 * @warning You MUST call that function in every RTC handler or the RTC will
 * never raise interrupt again.
 */
static void _rtcUpdateTime(void* pDrvCtrl, date_t* pDate, time_t* pTime);

/**
 * @brief Sends EOI to RTC itself.
 *
 * @details Sends EOI to RTC itself. The RTC requires to acknoledge its
 * interrupts otherwise, no further interrupt is generated.
 *
 * @param[in, out] pDrvCtrl The driver controler used by the registered
 * console driver.
 */
static void _rtcAckowledgeInt(void* pDrvCtrl);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief RTC driver instance. */
static driver_t sX86RTCDriver = {
    .pName         = "X86 RTC Driver",
    .pDescription  = "X86 Real Time Clock Driver for roOs",
    .pCompatible   = "x86,x86-rtc",
    .pVersion      = "2.0",
    .pDriverAttach = _rtcAttach
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _rtcAttach(const fdt_node_t* pkFdtNode)
{
    const uint32_t*  kpUintProp;
    size_t           propLen;
    OS_RETURN_E      retCode;
    int8_t           prevOred;
    int8_t           prevRate;
    rtc_controler_t* pDrvCtrl;
    kernel_timer_t*  pTimerDrv;

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED, TRACE_X86_RTC_ATTACH_ENTRY, 0);

    pDrvCtrl  = NULL;
    pTimerDrv = NULL;

    /* Init structures */
    pDrvCtrl = kmalloc(sizeof(rtc_controler_t));
    if(pDrvCtrl == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pDrvCtrl, 0, sizeof(rtc_controler_t));
    SPINLOCK_INIT(pDrvCtrl->lock);

    pTimerDrv = kmalloc(sizeof(kernel_timer_t));
    if(pTimerDrv == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }

    pTimerDrv->pGetFrequency  = _rtcGetFrequency;
    pTimerDrv->pGetTimeNs     = NULL;
    pTimerDrv->pSetTimeNs     = NULL;
    pTimerDrv->pGetDate       = _rtcGetDate;
    pTimerDrv->pGetDaytime    = _rtcGetDaytime;
    pTimerDrv->pEnable        = _rtcEnable;
    pTimerDrv->pDisable       = _rtcDisable;
    pTimerDrv->pSetHandler    = _rtcSetHandler;
    pTimerDrv->pRemoveHandler = _rtcRemoveHandler;
    pTimerDrv->pTickManager   = _rtcAckowledgeInt;
    pTimerDrv->pDriverCtrl    = pDrvCtrl;

    /* Get IRQ lines */
    kpUintProp = fdtGetProp(pkFdtNode, RTC_FDT_INT_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->irqNumber = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "IRQ: %d",
                 pDrvCtrl->irqNumber);

    /* Get communication ports */
    kpUintProp = fdtGetProp(pkFdtNode, RTC_FDT_COMM_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->cpuCommPort = (uint16_t)FDTTOCPU32(*kpUintProp);
    pDrvCtrl->cpuDataPort = (uint16_t)FDTTOCPU32(*(kpUintProp + 1));

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "COMM: 0x%x | DATA: 0x%x",
                 pDrvCtrl->cpuCommPort,
                 pDrvCtrl->cpuDataPort);

    /* Get quartz frequency */
    kpUintProp = fdtGetProp(pkFdtNode, RTC_FDT_QUARTZ_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->quartzFrequency = FDTTOCPU32(*kpUintProp);

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Quartz Frequency: %dHz",
                 pDrvCtrl->quartzFrequency);

    /* Get selected frequency */
    kpUintProp = fdtGetProp(pkFdtNode, RTC_FDT_SELFREQ_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->selectedFrequency = FDTTOCPU32(*kpUintProp);

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Selected Frequency: %dHz",
                 pDrvCtrl->selectedFrequency);

    /* Get the frequency range */
    kpUintProp = fdtGetProp(pkFdtNode, RTC_FDT_FREQRANGE_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->frequencyLow  = (uint32_t)FDTTOCPU32(*kpUintProp);
    pDrvCtrl->frequencyHigh = (uint32_t)FDTTOCPU32(*(kpUintProp + 1));

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Frequency Range: %dHz / %dHz",
                 pDrvCtrl->frequencyLow,
                 pDrvCtrl->frequencyHigh);

    /* Check if frequency is within bounds */
    if(pDrvCtrl->frequencyLow > pDrvCtrl->selectedFrequency ||
       pDrvCtrl->frequencyHigh < pDrvCtrl->selectedFrequency)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Init system times */
    pDrvCtrl->disabledNesting = 1;

    /* Init CMOS IRQ8 */
    _cpuOutB((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_B, pDrvCtrl->cpuCommPort);
    prevOred = _cpuInB(pDrvCtrl->cpuDataPort);
    _cpuOutB((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_B, pDrvCtrl->cpuCommPort);
    _cpuOutB(prevOred | CMOS_ENABLE_RTC, pDrvCtrl->cpuDataPort);

    /* Init CMOS IRQ8 rate */
    _cpuOutB((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_A, pDrvCtrl->cpuCommPort);
    prevRate = _cpuInB(pDrvCtrl->cpuDataPort);
    _cpuOutB((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_A, pDrvCtrl->cpuCommPort);
    _cpuOutB((prevRate & 0xF0) | RTC_INIT_RATE, pDrvCtrl->cpuDataPort);

    /* Set RTC frequency */
    _rtcSetFrequency(pDrvCtrl, pDrvCtrl->selectedFrequency);

    /* Just dummy read register C to unlock interrupt */
    _rtcAckowledgeInt(pDrvCtrl);

    /* Check if we should register as main timer */
    if(fdtGetProp(pkFdtNode, RTC_FDT_ISRTC_PROP, &propLen) != NULL)
    {
        retCode = timeMgtAddTimer(pTimerDrv, RTC_TIMER);
        if(retCode != OS_NO_ERR)
        {
            goto ATTACH_END;
        }
    }
    else
    {
        retCode = timeMgtAddTimer(pTimerDrv, AUX_TIMER);
        if(retCode != OS_NO_ERR)
        {
            goto ATTACH_END;
        }
    }

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
    }

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_ATTACH_EXIT,
                       1,
                       (uint32_t)retCode);

    KERNEL_DEBUG(RTC_DEBUG_ENABLED, MODULE_NAME, "RTC Initialized");
    return retCode;

}

static void _rtcDummyHandler(kernel_thread_t* pCurrThread)
{
    (void)pCurrThread;

    PANIC(OS_ERR_UNAUTHORIZED_ACTION,
          MODULE_NAME,
          "RTC Dummy handler called");

    return;
}

static void _rtcEnable(void* pDrvCtrl)
{
    rtc_controler_t* pRtcCtrl;

    pRtcCtrl = GET_CONTROLER(pDrvCtrl);

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_ENABLE_ENTRY,
                       1,
                       pRtcCtrl->disabledNesting);

    KERNEL_LOCK(pRtcCtrl->lock);

    if(pRtcCtrl->disabledNesting > 0)
    {
        --pRtcCtrl->disabledNesting;
    }

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Enable RTC (nesting %d, freq %d)",
                 pRtcCtrl->disabledNesting,
                 pRtcCtrl->selectedFrequency);
    if(pRtcCtrl->disabledNesting == 0 && pRtcCtrl->selectedFrequency != 0)
    {
        interruptIRQSetMask(pRtcCtrl->irqNumber, TRUE);
    }

    KERNEL_UNLOCK(pRtcCtrl->lock);

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_ENABLE_EXIT,
                       1,
                       pRtcCtrl->disabledNesting);
}

static void _rtcDisable(void* pDrvCtrl)
{
    rtc_controler_t* pRtcCtrl;

    pRtcCtrl = GET_CONTROLER(pDrvCtrl);

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_DISABLE_ENTRY,
                       1,
                       pRtcCtrl->disabledNesting);

    KERNEL_LOCK(pRtcCtrl->lock);

    if(pRtcCtrl->disabledNesting < UINT32_MAX)
    {
        ++pRtcCtrl->disabledNesting;
    }

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Disable RTC (nesting %d)",
                 pRtcCtrl->disabledNesting);
    interruptIRQSetMask(pRtcCtrl->irqNumber, FALSE);

    KERNEL_UNLOCK(pRtcCtrl->lock);

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_DISABLE_EXIT,
                       1,
                       pRtcCtrl->disabledNesting);
}

static void _rtcSetFrequency(void* pDrvCtrl, const uint32_t kFrequency)
{
    uint32_t         prevRate;
    uint32_t         rate;
    rtc_controler_t* pRtcCtrl;

    pRtcCtrl = GET_CONTROLER(pDrvCtrl);

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_SET_FREQUENCY_ENTRY,
                       1,
                       kFrequency);

    if(kFrequency < pRtcCtrl->frequencyLow ||
       kFrequency > pRtcCtrl->frequencyHigh)
    {

        KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                           TRACE_X86_RTC_SET_FREQUENCY_EXIT,
                           2,
                           kFrequency,
                           (uint32_t)pRtcCtrl->selectedFrequency);

        KERNEL_ERROR("RTC timer frequency out of bound %u not in [%u:%u]\n",
                     kFrequency,
                     pRtcCtrl->frequencyLow,
                     pRtcCtrl->frequencyHigh);
        return;
    }

    /* Choose the closest rate to the frequency */
    if(kFrequency < 4)
    {
        rate = 15;
    }
    else if(kFrequency < 8)
    {
        rate = 14;
    }
    else if(kFrequency < 16)
    {
        rate = 13;
    }
    else if(kFrequency < 32)
    {
        rate = 12;
    }
    else if(kFrequency < 64)
    {
        rate = 11;
    }
    else if(kFrequency < 128)
    {
        rate = 10;
    }
    else if(kFrequency < 256)
    {
        rate = 9;
    }
    else if(kFrequency < 512)
    {
        rate = 8;
    }
    else if(kFrequency < 1024)
    {
        rate = 7;
    }
    else if(kFrequency < 2048)
    {
        rate = 6;
    }
    else if(kFrequency < 4096)
    {
        rate = 5;
    }
    else if(kFrequency < 8192)
    {
        rate = 4;
    }
    else
    {
        rate = 3;
    }

    KERNEL_LOCK(pRtcCtrl->lock);

    /* Set clock frequency */
     /* Init CMOS IRQ8 rate */
    _cpuOutB((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_A, pRtcCtrl->cpuCommPort);
    prevRate = _cpuInB(pRtcCtrl->cpuDataPort);
    _cpuOutB((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_A, pRtcCtrl->cpuCommPort);
    _cpuOutB((prevRate & 0xF0) | rate, pRtcCtrl->cpuDataPort);

    pRtcCtrl->selectedFrequency = (pRtcCtrl->quartzFrequency >> (rate - 1));

    KERNEL_UNLOCK(pRtcCtrl->lock);

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "New RTC rate set (%d: %dHz)",
                 rate,
                 pRtcCtrl->selectedFrequency);


    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_SET_FREQUENCY_EXIT,
                       2,
                       kFrequency,
                       (uint32_t)pRtcCtrl->selectedFrequency);
}

static uint32_t _rtcGetFrequency(void* pDrvCtrl)
{
    rtc_controler_t* pRtcCtrl;

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_GET_FREQUENCY_ENTRY,
                       0);

    pRtcCtrl = GET_CONTROLER(pDrvCtrl);

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_GET_FREQUENCY_EXIT,
                       1,
                       pRtcCtrl->selectedFrequency);

    return pRtcCtrl->selectedFrequency;
}

static OS_RETURN_E _rtcSetHandler(void* pDrvCtrl,
                                  void(*pHandler)(kernel_thread_t*))
{
    OS_RETURN_E      err;
    rtc_controler_t* pRtcCtrl;

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_SET_HANDLER_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pHandler),
                       KERNEL_TRACE_LOW(pHandler));

    if(pHandler == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                           TRACE_X86_RTC_SET_HANDLER_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pHandler),
                           KERNEL_TRACE_LOW(pHandler),
                           (uint32_t)OS_ERR_NULL_POINTER);

        return OS_ERR_NULL_POINTER;
    }

    pRtcCtrl = GET_CONTROLER(pDrvCtrl);

    _rtcDisable(pDrvCtrl);

    err = interruptIRQRegister(pRtcCtrl->irqNumber, pHandler);
    if(err != OS_NO_ERR)
    {
        KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                           TRACE_X86_RTC_SET_HANDLER_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pHandler),
                           KERNEL_TRACE_LOW(pHandler),
                           (uint32_t)err);

        return err;
    }

    KERNEL_DEBUG(RTC_DEBUG_ENABLED, MODULE_NAME, "New RTC handler set (0x%p)",
                 pHandler);

    _rtcEnable(pDrvCtrl);

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_SET_HANDLER_EXIT,
                       3,
                       KERNEL_TRACE_HIGH(pHandler),
                       KERNEL_TRACE_LOW(pHandler),
                       (uint32_t)err);

    return err;
}

static OS_RETURN_E _rtcRemoveHandler(void* pDrvCtrl)
{
    KERNEL_DEBUG(RTC_DEBUG_ENABLED, MODULE_NAME, "Default RTC handler set 0x%p",
                 _rtcDummyHandler);

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED, TRACE_X86_RTC_REMOVE_HANDLER, 0);

    return _rtcSetHandler(pDrvCtrl, _rtcDummyHandler);
}

static time_t _rtcGetDaytime(void* pDrvCtrl)
{
    time_t retTime;
    date_t retDate;

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_GET_DAYTIME_ENTRY,
                       0);
    _rtcUpdateTime(pDrvCtrl, &retDate, &retTime);

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_GET_DAYTIME_EXIT,
                       0);
    return retTime;
}

static date_t _rtcGetDate(void* pDrvCtrl)
{
    time_t retTime;
    date_t retDate;

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_GET_DATE_ENTRY,
                       0);

    _rtcUpdateTime(pDrvCtrl, &retDate, &retTime);

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_GET_DATE_EXIT,
                       0);

    return retDate;
}

static void _rtcUpdateTime(void* pDrvCtrl, date_t* pDate, time_t* pTime)
{
    uint8_t          century;
    uint8_t          regB;
    uint16_t         commPort;
    uint16_t         dataPort;
    rtc_controler_t* pRtcCtrl;

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_UPDATETIME_ENTRY,
                       0);

    pRtcCtrl = GET_CONTROLER(pDrvCtrl);
    commPort = pRtcCtrl->cpuCommPort;
    dataPort = pRtcCtrl->cpuDataPort;

    KERNEL_LOCK(pRtcCtrl->lock);

    /* Select CMOS seconds register and read */
    _cpuOutB(CMOS_SECONDS_REGISTER, commPort);
    pTime->seconds = _cpuInB(dataPort);

    /* Select CMOS minutes register and read */
    _cpuOutB(CMOS_MINUTES_REGISTER, commPort);
    pTime->minutes = _cpuInB(dataPort);

    /* Select CMOS hours register and read */
    _cpuOutB(CMOS_HOURS_REGISTER, commPort);
    pTime->hours = _cpuInB(dataPort);

    /* Select CMOS day register and read */
    _cpuOutB(CMOS_DAY_REGISTER, commPort);
    pDate->day = _cpuInB(dataPort);

    /* Select CMOS month register and read */
    _cpuOutB(CMOS_MONTH_REGISTER, commPort);
    pDate->month = _cpuInB(dataPort);

    /* Select CMOS years register and read */
    _cpuOutB(CMOS_YEAR_REGISTER, commPort);
    pDate->year = _cpuInB(dataPort);

    /* Select CMOS century register and read */
    if(CMOS_CENTURY_REGISTER != 0)
    {
        _cpuOutB(CMOS_CENTURY_REGISTER, commPort);
        century = _cpuInB(dataPort);
    }
    else
    {
        century = CURRENT_YEAR / 100;
    }

    /* Convert BCD to binary if necessary */
    _cpuOutB(CMOS_REG_B, commPort);
    regB = _cpuInB(dataPort);

    if((regB & 0x04) == 0)
    {
        pTime->seconds = (pTime->seconds & 0x0F) + ((pTime->seconds / 16) * 10);
        pTime->minutes = (pTime->minutes & 0x0F) + ((pTime->minutes / 16) * 10);
        pTime-> hours = ((pTime->hours & 0x0F) +
                         (((pTime->hours & 0x70) / 16) * 10)) |
                         (pTime->hours & 0x80);
        pDate->day = (pDate->day & 0x0F) + ((pDate->day / 16) * 10);
        pDate->month = (pDate->month & 0x0F) + ((pDate->month / 16) * 10);
        pDate->year = (pDate->year & 0x0F) + ((pDate->year / 16) * 10);

        if(CMOS_CENTURY_REGISTER != 0)
        {
            century = (century & 0x0F) + ((century / 16) * 10);
        }
    }

    /*  Convert to 24H */
    if((regB & 0x02) == 0 && (pTime->hours & 0x80) >= 1)
    {
        pTime->hours = ((pTime->hours & 0x7F) + 12) % 24;
    }

    /* Get year */
    if(CMOS_CENTURY_REGISTER != 0)
    {
        pDate->year += century * 100;
    }
    else
    {
        pDate->year = pDate->year + 2000;
    }

    /* Compute week day and day time */
    pDate->weekday = ((pDate->day + pDate->month + pDate->year + pDate->year /
                       4)
                       + 1) % 7 + 1;

    KERNEL_UNLOCK(pRtcCtrl->lock);

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED,
                       TRACE_X86_RTC_UPDATETIME_EXIT,
                       0);

    KERNEL_DEBUG(RTC_DEBUG_ENABLED, MODULE_NAME, "Updated RTC");
}

static void _rtcAckowledgeInt(void* pDrvCtrl)
{
    rtc_controler_t* pRtcCtrl;

    pRtcCtrl = GET_CONTROLER(pDrvCtrl);

    KERNEL_TRACE_EVENT(TRACE_X86_RTC_ENABLED, TRACE_X86_RTC_ACK_INTERRUPT, 0);

    /* Clear C Register */
    _cpuOutB(CMOS_REG_C, pRtcCtrl->cpuCommPort);
    _cpuInB(pRtcCtrl->cpuDataPort);

    /* Set EOI */
    interruptIRQSetEOI(pRtcCtrl->irqNumber);
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG(sX86RTCDriver);

/************************************ EOF *************************************/