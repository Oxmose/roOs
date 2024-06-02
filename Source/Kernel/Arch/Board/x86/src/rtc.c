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
#include <rtc.h>

/* Unit test header */
#include <test_framework.h>

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
        PANIC(ERROR, MODULE_NAME, MSG, TRUE);               \
    }                                                       \
}

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
 */
static void _rtcEnable(void);

/**
 * @brief Disables RTC ticks.
 *
 * @details Disables RTC ticks by setting the RTC's IRQ mask.
 */
static void _rtcDisable(void);

/**
 * @brief Sets the RTC's tick frequency.
 *
 * @details Sets the RTC's tick frequency. The value must be between 2Hz and
 * 8192Hz.
 *
 * @warning The value must be between 2Hz and 8192Hz. The lower boundary RTC
 * frequency will be selected (refer to the code to understand the 14 available
 * frequencies).
 *
 * @param[in] kFrequency The new frequency to be set to the RTC.
 */
static void _rtcSetFrequency(const uint32_t kFrequency);

/**
 * @brief Returns the RTC tick frequency in Hz.
 *
 * @details Returns the RTC tick frequency in Hz.
 *
 * @return The RTC tick frequency in Hz.
 */
static uint32_t _rtcGetFrequency(void);

/**
 * @brief Sets the RTC tick handler.
 *
 * @details Sets the RTC tick handler. This function will be called at each RTC
 * tick received.
 *
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
static OS_RETURN_E _rtcSetHandler(void(*pHandler)(kernel_thread_t*));

/**
 * @brief Removes the RTC tick handler.
 *
 * @details Removes the RTC tick handler.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the RTC interrupt line
 * is not allowed.
 * - OS_ERR_INTERRUPT_NOT_REGISTERED is returned if the RTC line has no handler
 * attached.
 */
static OS_RETURN_E _rtcRemoveHandler(void);

/**
 * @brief Returns the current date.
 *
 * @details Returns the current date in RTC date format.
 *
 * @returns The current date in in RTC date format
 */
static date_t _rtcGetDate(void);

/**
 * @brief Returns the current daytime in seconds.
 *
 * @details Returns the current daytime in seconds.
 *
 * @returns The current daytime in seconds.
 */
static time_t _rtcGetDaytime(void);

/**
 * @brief Updates the system's time and date.
 *
 * @details Updates the system's time and date. This function also reads the
 * CMOS registers. By doing that, the RTC registers are cleaned and the RTC able
 * to interrupt the CPU again.
 *
 * @param[out] pDate The date to update.
 * @param[out] pTime The time to update.
 *
 * @warning You MUST call that function in every RTC handler or the RTC will
 * never raise interrupt again.
 */
static void _rtcUpdateTime(date_t* pDate, time_t* pTime);

/**
 * @brief Sends EOI to RTC itself.
 *
 * @details Sends EOI to RTC itself. The RTC requires to acknoledge its
 * interrupts otherwise, no further interrupt is generated.
 */
static void _rtcAckowledgeInt(void);

/**
 * @brief Returns the RTC IRQ number.
 *
 * @details Returns the RTC IRQ number.
 *
 * @return The RTC IRQ number.
 */
static uint32_t _rtcGetIrq(void);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/** @brief RTC driver instance. */
driver_t x86RTCDriver = {
    .pName         = "X86 RTC Driver",
    .pDescription  = "X86 Real Time Clock Driver for UTK",
    .pCompatible   = "x86,x86-rtc",
    .pVersion      = "2.0",
    .pDriverAttach = _rtcAttach
};

/************************** Static global variables ***************************/
/** @brief RTC driver controler instance. */
static rtc_controler_t sDrvCtrl = {
    .cpuCommPort       = 0,
    .cpuDataPort       = 0,
    .irqNumber         = 0,
    .quartzFrequency   = 0,
    .selectedFrequency = 0,
    .frequencyLow      = 0,
    .frequencyHigh     = 0,
    .disabledNesting   = 0
};

/** @brief RTC driver instance. */
static kernel_timer_t sRtcDriver = {
    .get_frequency  = _rtcGetFrequency,
    .set_frequency  = _rtcSetFrequency,
    .get_time_ns    = NULL,
    .set_time_ns    = NULL,
    .get_date       = _rtcGetDate,
    .get_daytime    = _rtcGetDaytime,
    .enable         = _rtcEnable,
    .disable        = _rtcDisable,
    .set_handler    = _rtcSetHandler,
    .remove_handler = _rtcRemoveHandler,
    .get_irq        = _rtcGetIrq,
    .tickManager    = _rtcAckowledgeInt
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _rtcAttach(const fdt_node_t* pkFdtNode)
{
    const uintptr_t* ptrProp;
    size_t           propLen;
    OS_RETURN_E      retCode;
    int8_t           prevOred;
    int8_t           prevRate;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_INIT_START, 0);

    /* Get IRQ lines */
    ptrProp = fdtGetProp(pkFdtNode, RTC_FDT_INT_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t) * 2)
    {
        KERNEL_ERROR("Failed to retreive the IRQ from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.irqNumber = (uint8_t)FDTTOCPU32(*(ptrProp + 1));

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "IRQ: %d",
                 sDrvCtrl.irqNumber);

    /* Get communication ports */
    ptrProp = fdtGetProp(pkFdtNode, RTC_FDT_COMM_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t) * 2)
    {
        KERNEL_ERROR("Failed to retreive the CPU comm from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.cpuCommPort = (uint16_t)FDTTOCPU32(*ptrProp);
    sDrvCtrl.cpuDataPort = (uint16_t)FDTTOCPU32(*(ptrProp + 1));

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "COMM: 0x%x | DATA: 0x%x",
                 sDrvCtrl.cpuCommPort,
                 sDrvCtrl.cpuDataPort);

    /* Get quartz frequency */
    ptrProp = fdtGetProp(pkFdtNode, RTC_FDT_QUARTZ_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t))
    {
        KERNEL_ERROR("Failed to retreive the quartz frequency from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.quartzFrequency = FDTTOCPU32(*ptrProp);

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Quartz Frequency: %dHz",
                 sDrvCtrl.quartzFrequency);

    /* Get selected frequency */
    ptrProp = fdtGetProp(pkFdtNode, RTC_FDT_SELFREQ_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t))
    {
        KERNEL_ERROR("Failed to retreive the selected frequency from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.selectedFrequency = FDTTOCPU32(*ptrProp);

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Selected Frequency: %dHz",
                 sDrvCtrl.selectedFrequency);

    /* Get the frequency range */
    ptrProp = fdtGetProp(pkFdtNode, RTC_FDT_FREQRANGE_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t) * 2)
    {
        KERNEL_ERROR("Failed to retreive the CPU comm from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.frequencyLow  = (uint32_t)FDTTOCPU32(*ptrProp);
    sDrvCtrl.frequencyHigh = (uint32_t)FDTTOCPU32(*(ptrProp + 1));

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Frequency Range: %dHz / %dHz",
                 sDrvCtrl.frequencyLow,
                 sDrvCtrl.frequencyHigh);

    /* Check if frequency is within bounds */
    if(sDrvCtrl.frequencyLow > sDrvCtrl.selectedFrequency ||
       sDrvCtrl.frequencyHigh < sDrvCtrl.selectedFrequency)
    {
        KERNEL_ERROR("Selected RTC frequency is not within range.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Init system times */
    sDrvCtrl.disabledNesting = 1;

    /* Init CMOS IRQ8 */
    _cpuOutB((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_B, sDrvCtrl.cpuCommPort);
    prevOred = _cpuInB(sDrvCtrl.cpuDataPort);
    _cpuOutB((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_B, sDrvCtrl.cpuCommPort);
    _cpuOutB(prevOred | CMOS_ENABLE_RTC, sDrvCtrl.cpuDataPort);

    /* Init CMOS IRQ8 rate */
    _cpuOutB((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_A, sDrvCtrl.cpuCommPort);
    prevRate = _cpuInB(sDrvCtrl.cpuDataPort);
    _cpuOutB((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_A, sDrvCtrl.cpuCommPort);
    _cpuOutB((prevRate & 0xF0) | RTC_INIT_RATE, sDrvCtrl.cpuDataPort);

    /* Set RTC frequency */
    _rtcSetFrequency(sDrvCtrl.selectedFrequency);

    /* Just dummy read register C to unlock interrupt */
    _rtcAckowledgeInt();

    /* Check if we should register as main timer */
    if(fdtGetProp(pkFdtNode, RTC_FDT_ISRTC_PROP, &propLen) != NULL)
    {
        retCode = timeMgtAddTimer(&sRtcDriver, RTC_TIMER);
        if(retCode != OS_NO_ERR)
        {
            KERNEL_ERROR("Failed to set RTC driver as RTC timer. Error %d\n",
                         retCode);
            goto ATTACH_END;
        }
    }
    else
    {
        retCode = timeMgtAddTimer(&sRtcDriver, AUX_TIMER);
        if(retCode != OS_NO_ERR)
        {
            KERNEL_ERROR("Failed to set RTC driver as RTC timer. Error %d\n",
                         retCode);
            goto ATTACH_END;
        }
    }

ATTACH_END:
    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_INIT_END,
                       2,
                       sDrvCtrl.selectedFrequency,
                       (uintptr_t)retCode);
    KERNEL_DEBUG(RTC_DEBUG_ENABLED, MODULE_NAME, "RTC Initialized");
    return retCode;

}

static void _rtcDummyHandler(kernel_thread_t* pCurrThread)
{
    (void)pCurrThread;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_DUMMY_HANDLER, 0);

    KERNEL_DEBUG(RTC_DEBUG_ENABLED, MODULE_NAME, "RTC Interrupt");

    /* EOI */
    kernel_interrupt_set_irq_eoi(sDrvCtrl.irqNumber);
}

static void _rtcEnable(void)
{
    uint32_t intState;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_ENABLE_START, 0);

    ENTER_CRITICAL(intState);

    if(sDrvCtrl.disabledNesting > 0)
    {
        --sDrvCtrl.disabledNesting;
    }

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Enable RTC (nesting %d, freq %d)",
                 sDrvCtrl.disabledNesting,
                 sDrvCtrl.selectedFrequency);
    if(sDrvCtrl.disabledNesting == 0 && sDrvCtrl.selectedFrequency != 0)
    {
        kernel_interrupt_set_irq_mask(sDrvCtrl.irqNumber, 1);
    }

    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_ENABLE_END, 0);

    EXIT_CRITICAL(intState);
}

static void _rtcDisable(void)
{
    uint32_t intState;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_DISABLE_START,
                       1,
                       sDrvCtrl.disabledNesting);

    ENTER_CRITICAL(intState);

    if(sDrvCtrl.disabledNesting < UINT32_MAX)
    {
        ++sDrvCtrl.disabledNesting;
    }

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Disable RTC (nesting %d)",
                 sDrvCtrl.disabledNesting);
    kernel_interrupt_set_irq_mask(sDrvCtrl.irqNumber, 0);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_DISABLE_END,
                       1,
                       sDrvCtrl.disabledNesting);

    EXIT_CRITICAL(intState);
}

static void _rtcSetFrequency(const uint32_t kFrequency)
{
    uint32_t    prevRate;
    uint32_t    rate;
    uint32_t    intState;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_SET_FREQ_START, 1, kFrequency);

    if(kFrequency < sDrvCtrl.frequencyLow ||
       kFrequency > sDrvCtrl.frequencyHigh)
    {
        sDrvCtrl.selectedFrequency = 0;
        KERNEL_ERROR("RTC timer frequency out of bound %u not in [%u:%u]\n",
                     kFrequency,
                     sDrvCtrl.frequencyLow,
                     sDrvCtrl.frequencyHigh);
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

    ENTER_CRITICAL(intState);

    /* Disable RTC IRQ */
    _rtcDisable();

    /* Set clock frequency */
     /* Init CMOS IRQ8 rate */
    _cpuOutB((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_A, sDrvCtrl.cpuCommPort);
    prevRate = _cpuInB(sDrvCtrl.cpuDataPort);
    _cpuOutB((CMOS_NMI_DISABLE_BIT << 7) | CMOS_REG_A, sDrvCtrl.cpuCommPort);
    _cpuOutB((prevRate & 0xF0) | rate, sDrvCtrl.cpuDataPort);

    sDrvCtrl.selectedFrequency = (sDrvCtrl.quartzFrequency >> (rate - 1));

    KERNEL_DEBUG(RTC_DEBUG_ENABLED,
                 MODULE_NAME,
                 "New RTC rate set (%d: %dHz)",
                 rate,
                 sDrvCtrl.selectedFrequency);

    /* Enable RTC IRQ */
    _rtcEnable();

    EXIT_CRITICAL(intState);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_SET_FREQ_END, 1, kFrequency);

}

static uint32_t _rtcGetFrequency(void)
{
    return sDrvCtrl.selectedFrequency;
}

static OS_RETURN_E _rtcSetHandler(void(*pHandler)(kernel_thread_t*))
{
    OS_RETURN_E err;
    uint32_t    intState;

#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_SET_HANDLER, 2,
                       (uintptr_t)handler & 0xFFFFFFFF,
                       (uintptr_t)handler >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_SET_HANDLER, 2,
                       (uintptr_t)handler & 0xFFFFFFFF,
                       (uintptr_t)0);
#endif

    if(pHandler == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    ENTER_CRITICAL(intState);

    _rtcDisable();

    /* Remove the current handler */
    err = kernel_interrupt_remove_irq_handler(sDrvCtrl.irqNumber);
    if(err != OS_NO_ERR && err != OS_ERR_INTERRUPT_NOT_REGISTERED)
    {
        EXIT_CRITICAL(intState);
        KERNEL_ERROR("Failed to remove RTC irqHandler. Error: %d\n", err);
        _rtcEnable();
        return err;
    }

    err = kernel_interrupt_register_irq_handler(sDrvCtrl.irqNumber, pHandler);
    if(err != OS_NO_ERR)
    {
        EXIT_CRITICAL(intState);
        KERNEL_ERROR("Failed to register RTC irqHandler. Error: %d\n", err);
        return err;
    }

    KERNEL_DEBUG(RTC_DEBUG_ENABLED, MODULE_NAME, "New RTC handler set (0x%p)",
                 pHandler);

    EXIT_CRITICAL(intState);

    _rtcEnable();

    return err;
}

static OS_RETURN_E _rtcRemoveHandler(void)
{
    KERNEL_DEBUG(RTC_DEBUG_ENABLED, MODULE_NAME, "Default RTC handler set 0x%p",
                 _rtcDummyHandler);

#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_REMOVE_HANDLER, 2,
                       (uintptr_t)_rtcDummyHandler & 0xFFFFFFFF,
                       (uintptr_t)_rtcDummyHandler >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_REMOVE_HANDLER, 2,
                       (uintptr_t)_rtcDummyHandler & 0xFFFFFFFF,
                       (uintptr_t)0);
#endif

    return _rtcSetHandler(_rtcDummyHandler);
}

static time_t _rtcGetDaytime(void)
{
    time_t retTime;
    date_t retDate;
    _rtcUpdateTime(&retDate, &retTime);
    return retTime;
}

static date_t _rtcGetDate(void)
{
    time_t retTime;
    date_t retDate;
    _rtcUpdateTime(&retDate, &retTime);
    return retDate;
}

static void _rtcUpdateTime(date_t* pDate, time_t* pTime)
{
    uint8_t century;
    uint8_t regB;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_UPDATE_TIME_START, 0);

    /* Select CMOS seconds register and read */
    _cpuOutB(CMOS_SECONDS_REGISTER, sDrvCtrl.cpuCommPort);
    pTime->seconds = _cpuInB(sDrvCtrl.cpuDataPort);

    /* Select CMOS minutes register and read */
    _cpuOutB(CMOS_MINUTES_REGISTER, sDrvCtrl.cpuCommPort);
    pTime->minutes = _cpuInB(sDrvCtrl.cpuDataPort);

    /* Select CMOS hours register and read */
    _cpuOutB(CMOS_HOURS_REGISTER, sDrvCtrl.cpuCommPort);
    pTime->hours = _cpuInB(sDrvCtrl.cpuDataPort);

    /* Select CMOS day register and read */
    _cpuOutB(CMOS_DAY_REGISTER, sDrvCtrl.cpuCommPort);
    pDate->day = _cpuInB(sDrvCtrl.cpuDataPort);

    /* Select CMOS month register and read */
    _cpuOutB(CMOS_MONTH_REGISTER, sDrvCtrl.cpuCommPort);
    pDate->month = _cpuInB(sDrvCtrl.cpuDataPort);

    /* Select CMOS years register and read */
    _cpuOutB(CMOS_YEAR_REGISTER, sDrvCtrl.cpuCommPort);
    pDate->year = _cpuInB(sDrvCtrl.cpuDataPort);

    /* Select CMOS century register and read */
    if(CMOS_CENTURY_REGISTER != 0)
    {
        _cpuOutB(CMOS_CENTURY_REGISTER, sDrvCtrl.cpuCommPort);
        century = _cpuInB(sDrvCtrl.cpuDataPort);
    }
    else
    {
        century = CURRENT_YEAR / 100;
    }

    /* Convert BCD to binary if necessary */
    _cpuOutB(CMOS_REG_B, sDrvCtrl.cpuCommPort);
    regB = _cpuInB(sDrvCtrl.cpuDataPort);

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

    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_UPDATE_TIME_END, 0);

    KERNEL_DEBUG(RTC_DEBUG_ENABLED, MODULE_NAME, "Updated RTC");
}

static void _rtcAckowledgeInt(void)
{
    /* Clear C Register */
    _cpuOutB(CMOS_REG_C, sDrvCtrl.cpuCommPort);
    _cpuInB(sDrvCtrl.cpuDataPort);
}

static uint32_t _rtcGetIrq(void)
{
    return sDrvCtrl.irqNumber;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG(x86RTCDriver);

/************************************ EOF *************************************/