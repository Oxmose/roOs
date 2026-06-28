/*******************************************************************************
 * @file time_mgt.c
 *
 * @see time_mgt.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 17/05/2023
 *
 * @version 1.0
 *
 * @brief Kernel's time management methods.
 *
 * @details Kernel's time management method. Allow to define timers and keep
 * track on the system's time.
 *
 * @warning All the interrupt managers and timer sources drivers must be
 * initialized before using any of these functions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <cpu.h>           /* Core manager */
#include <panic.h>         /* Kernel panic */
#include <kheap.h>         /* Kernel heap */
#include <errno.h>         /* Errno values */
#include <stdint.h>        /* Generic int types */
#include <kerror.h>        /* Kernel error codes */
#include <kqueue.h>        /* Kernel queues */
#include <syslog.h>        /* Kernel Syslog */
#include <devtree.h>       /* Device tree lib */
#include <syscall.h>       /* Kernel system call */
#include <drivermgr.h>     /* Driver manager */
#include <interrupts.h>    /* Interrupt manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <time_mgt.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "TIME MGT"

/** @brief FDT Time node, time configuration node name */
#define FDT_TIMECONFIG_NODE_NAME "timeconfig"
/** @brief FDT Time node, main timer pHandle */
#define FDT_TIMECONFIG_MAIN_PROP "main"
/** @brief FDT Time node, RTC timer pHandle */
#define FDT_TIMECONFIG_RTC_PROP "rtc"
/** @brief FDT Time node, lifetime timer pHandle */
#define FDT_TIMECONFIG_LIFETIME_PROP "lifetime"
/** @brief FDT Time node, aux timer pHandles */
#define FDT_TIMECONFIG_AUX_PROP "aux"

/** @brief Defines the year of the Epoch */
#define TIMESPAMP_START_YEAR 1970ULL
/** @brief Defines the number of seconds in a leap year */
#define SEC_PER_LEAP_YEAR 31622400ULL
/** @brief Defines the number of seconds in a year */
#define SEC_PER_YEAR 31536000ULL
/** @brief Defines the number of seconds in a day */
#define SEC_PER_DAY 86400ULL
/** @brief Defines the number of seconds in an hour */
#define SEC_PER_HOUR 3600ULL
/** @brief Defines the number of seconds in a minute */
#define SEC_PER_MINUTES 60ULL
/** @brief Tells if the year X is a leap year. */
#define IS_LEAP_YEAR(X) ((((X) % 4 == 0) && ((X) % 100 != 0)) || \
                         ((X) % 400 == 0))

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/**
 * @brief Defines the parameters for the get time system call.
 */
typedef struct
{
    /** @brief Errno value to set */
    syscall_min_params_t errnoVal;
    /** @brief The clock ID to use */
    clockid_t clkId;
    /** @brief The time spec structure to use for the call */
    struct timespec* pTimeSpec;
} syscall_gettime_params_t;


/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the Time manager to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the Time manager to ensure correctness of
 * execution. Due to the critical nature of the Time manager, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define TIME_ASSERT(COND, MSG, ERROR) {                     \
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief The kernel's main timer interrupt handler.
 *
 * @details The kernel's main timer interrupt handler. This must be connected to
 * the main timer of the system.
 *
 * @param[in, out] pCurrThread Current thread at the moment of the interrupt.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _mainTimerHandler(kernel_thread_t* pCurrThread);

/**
 * @brief The kernel's RTC timer interrupt handler.
 *
 * @details The kernel's RTC timer interrupt handler. This must be connected to
 * the RTC timer of the system.
 *
 * @param[in, out] pCurrThread Current thread at the moment of the interrupt.
 *
 * @return Returns if the scheduler must be called on return.
 */
static bool _rtcTimerHandler(kernel_thread_t* pCurrThread);

/**
 * @brief Adds an auxiliary timer to the list.
 *
 * @details Adds an auxiliary timer to the list. The AUX timer list might be
 * initialized if not already and the timer is added to the list.
 *
 * @param[in] kpTimer The timer driver to add.
 *
 * @return The success or error state is returned.
 */
static OS_RETURN_E _timeMgtAddAuxTimer(const kernel_timer_t* kpTimer);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief Stores the number of CPUs in the system */
static uint32_t sCpuCount;

/** @brief The kernel's main timer interrupt source.
 *
 *  @details The kernel's main timer interrupt source. If it's function pointers
 * are NULL, the driver is not initialized.
 */
static kernel_timer_t sSysMainTimer = {
    .pGetFrequency  = NULL,
    .pGetTimeNs     = NULL,
    .pSetTimeNs     = NULL,
    .pGetDate       = NULL,
    .pGetDaytime    = NULL,
    .pEnable        = NULL,
    .pDisable       = NULL,
    .pSetHandler    = NULL,
    .pRemoveHandler = NULL
};

/** @brief The kernel's RTC timer interrupt source.
 *
 *  @details The kernel's RTC timer interrupt source. If it's function pointers
 * are NULL, the driver is not initialized.
 */
static kernel_timer_t sSysRtcTimer = {
    .pGetFrequency  = NULL,
    .pGetTimeNs     = NULL,
    .pSetTimeNs     = NULL,
    .pGetDate       = NULL,
    .pGetDaytime    = NULL,
    .pEnable        = NULL,
    .pDisable       = NULL,
    .pSetHandler    = NULL,
    .pRemoveHandler = NULL,
};

/** @brief The kernel's lifetime timer.
 *
 *  @details The kernel's lifetime timer. If it's function pointers
 * are NULL, the driver is not initialized.
 */
static kernel_timer_t sSysLifetimeTimer = {
    .pGetFrequency  = NULL,
    .pGetTimeNs     = NULL,
    .pSetTimeNs     = NULL,
    .pGetDate       = NULL,
    .pGetDaytime    = NULL,
    .pEnable        = NULL,
    .pDisable       = NULL,
    .pSetHandler    = NULL,
    .pRemoveHandler = NULL,
};

/** @brief Stores the number of main kernel's timer tick since the
 * initialization of the time manager.
 */
static uint64_t* spSysTickCount;

/** @brief Auxiliary timers list */
static kqueue_t* spAuxTimersQueue = NULL;

/** @brief Active wait counter per CPU. */
static volatile uint64_t* spActiveWait;

/** @brief Auxiliary timers list lock */
static kernel_spinlock_t sAuxTimersListLock = KERNEL_SPINLOCK_INIT_VALUE;

/** @brief Stores the number of reconds in a month for year and leap years */
static uint64_t sSecPerMonth[2][12] = {
    /* Year */
    {
        2678400, 2419200, 2678400, 2592000, 2678400, 2592000,
        2678400, 2678400, 2592000, 2678400, 2592000, 2678400
    },
    /* Leap year */
    {
        2678400, 2505600, 2678400, 2592000, 2678400, 2592000,
        2678400, 2678400, 2592000, 2678400, 2592000, 2678400
    }
};
/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static bool _mainTimerHandler(kernel_thread_t* pCurrThread)
{
    uint8_t cpuId;

    (void)pCurrThread;

    cpuId = cpuGetId();

#if TIME_MGT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Time manager main handler");
#endif

    /* Add a tick count */
    ++spSysTickCount[cpuId];

    if(sSysMainTimer.pTickManager != NULL)
    {
        sSysMainTimer.pTickManager(sSysMainTimer.pDriverCtrl);
    }

    /* Use coarse active wait if not lifetime timer is present */
    if(spActiveWait[cpuId] != 0)
    {
        /* Use ticks */
        if(spActiveWait[cpuId] <= spSysTickCount[cpuId] * 1000000000 /
                sSysMainTimer.pGetFrequency(sSysMainTimer.pDriverCtrl))
        {
            spActiveWait[cpuId] = 0;
        }
    }

    return true;
}

static bool _rtcTimerHandler(kernel_thread_t* pCurrThread)
{
    (void)pCurrThread;

    if(sSysRtcTimer.pTickManager != NULL)
    {
        sSysRtcTimer.pTickManager(sSysRtcTimer.pDriverCtrl);
    }

#if TIME_MGT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Time manager RTC handler");
#endif

    return false;
}

static OS_RETURN_E _timeMgtAddAuxTimer(const kernel_timer_t* kpTimer)
{
    kqueue_node_t* pNewNode;

    KERNEL_LOCK(sAuxTimersListLock);

    /* Create queue is it does not exist */
    if(spAuxTimersQueue == NULL)
    {
        spAuxTimersQueue = kQueueCreate(false);
        if(spAuxTimersQueue == NULL)
        {
            return OS_ERR_NO_MORE_MEMORY;
        }
    }

    KERNEL_UNLOCK(sAuxTimersListLock);

    /* Create the new node */
    pNewNode = kQueueCreateNode((void*)kpTimer, false);
    if(pNewNode == NULL)
    {
        return OS_ERR_NO_MORE_MEMORY;
    }

    /* Add the timer to the queue */
    kQueuePush(pNewNode, spAuxTimersQueue);
    return OS_NO_ERR;
}

void timeInit(void)
{
    OS_RETURN_E           retCode;
    const fdt_node_t*     kpTimerNode;
    const uint32_t*       kpUintProp;
    size_t                propLen;
    size_t                i;
    const kernel_timer_t* kpTimer;

    /* Get the number of CPUs */
    sCpuCount = cpuGetCount();

    /* Initialize the structures */
    spSysTickCount = kmalloc(sizeof(uint64_t) * sCpuCount);
    TIME_ASSERT(spSysTickCount != NULL,
                "Failed to allocate timing structure.",
                OS_ERR_NO_MORE_MEMORY);
    spActiveWait = kmalloc(sizeof(uint64_t) * sCpuCount);
    TIME_ASSERT(spActiveWait != NULL,
                "Failed to allocate timing structure.",
                OS_ERR_NO_MORE_MEMORY);

    /* Get the FDT timers node */
    kpTimerNode = fdtGetNodeByName(FDT_TIMECONFIG_NODE_NAME);
    TIME_ASSERT(kpTimerNode != NULL,
                "The FDT must contain a timer configurartion.",
                OS_ERR_INCORRECT_VALUE);

    /* Get the main timer driver */
    kpUintProp = fdtGetProp(kpTimerNode,
                            FDT_TIMECONFIG_MAIN_PROP,
                            &propLen);
    TIME_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t),
                "The FDT time configuration must contain on and only one main.",
                OS_ERR_INCORRECT_VALUE);

    kpTimer = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
    TIME_ASSERT(kpTimer != NULL &&
                kpTimer->pGetFrequency != NULL &&
                kpTimer->pEnable != NULL &&
                kpTimer->pDisable != NULL &&
                kpTimer->pSetHandler != NULL &&
                kpTimer->pRemoveHandler != NULL &&
                kpTimer->pDriverCtrl != NULL,
                "Invalid main timer driver",
                OS_ERR_NULL_POINTER);
    sSysMainTimer = *kpTimer;
    retCode = sSysMainTimer.pSetHandler(sSysMainTimer.pDriverCtrl,
                                        _mainTimerHandler);
    TIME_ASSERT(retCode == OS_NO_ERR,
                "Failed to setup the main timer",
                retCode);
    sSysMainTimer.pEnable(sSysMainTimer.pDriverCtrl);

#if TIME_MGT_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Added main timer");
#endif

    /* Get the RTC driver */
    kpUintProp = fdtGetProp(kpTimerNode,
                            FDT_TIMECONFIG_RTC_PROP,
                            &propLen);
    if(kpUintProp != NULL)
    {
        TIME_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t),
                    "The FDT time configuration must contain at most one RTC.",
                    OS_ERR_INCORRECT_VALUE);

        kpTimer = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
        TIME_ASSERT(kpTimer != NULL &&
                    kpTimer->pEnable != NULL &&
                    kpTimer->pDisable != NULL &&
                    kpTimer->pGetDaytime != NULL &&
                    kpTimer->pGetDate != NULL &&
                    kpTimer->pDriverCtrl != NULL,
                    "Invalid RTC timer driver",
                    OS_ERR_NULL_POINTER);
        sSysRtcTimer = *kpTimer;
        retCode = sSysRtcTimer.pSetHandler(sSysRtcTimer.pDriverCtrl,
                                           _rtcTimerHandler);
        TIME_ASSERT(retCode == OS_NO_ERR,
                    "Failed to setup the RTC timer",
                    retCode);
        sSysRtcTimer.pEnable(sSysRtcTimer.pDriverCtrl);
#if TIME_MGT_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Added RTC timer");
#endif

    }


    /* Get the lifetime driver */
    kpUintProp = fdtGetProp(kpTimerNode,
                            FDT_TIMECONFIG_LIFETIME_PROP,
                            &propLen);
    if(kpUintProp != NULL)
    {
        TIME_ASSERT(kpUintProp != NULL && propLen == sizeof(uint32_t),
                    "The FDT time configuration must contain at most one "
                    "lifetime.",
                    OS_ERR_INCORRECT_VALUE);

        kpTimer = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
        TIME_ASSERT(kpTimer != NULL &&
                    kpTimer->pGetTimeNs != NULL &&
                    kpTimer->pEnable != NULL &&
                    kpTimer->pDisable != NULL &&
                    kpTimer->pDriverCtrl != NULL,
                    "Invalid lifetime timer driver",
                    OS_ERR_NULL_POINTER);
        sSysLifetimeTimer = *kpTimer;
        sSysLifetimeTimer.pEnable(sSysLifetimeTimer.pDriverCtrl);

#if TIME_MGT_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Added lifetime timer");
#endif
    }

    /* Get other auxiliary timers */
    kpUintProp = fdtGetProp(kpTimerNode,
                            FDT_TIMECONFIG_AUX_PROP,
                            &propLen);
    if(kpUintProp != NULL)
    {
        for(i = 0; i < propLen / sizeof(uint32_t); ++i)
        {
            kpTimer = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));
            TIME_ASSERT(kpTimer != NULL &&
                        kpTimer->pEnable != NULL &&
                        kpTimer->pDisable != NULL &&
                        kpTimer->pDriverCtrl != NULL,
                        "Invalid aux timer driver",
                        OS_ERR_NULL_POINTER);
            retCode = _timeMgtAddAuxTimer(kpTimer);
            TIME_ASSERT(retCode == OS_NO_ERR,
                        "Failed to add auxiliary timer",
                        retCode);

#if TIME_MGT_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Added aux timer %d", i);
#endif
        }
    }
}

uint64_t timeGetUptime(void)
{
    uint64_t time;
    uint64_t maxTick;
    uint32_t i;

    if(sSysLifetimeTimer.pGetTimeNs != NULL)
    {
        time = sSysLifetimeTimer.pGetTimeNs(sSysLifetimeTimer.pDriverCtrl);
    }
    else if(sSysMainTimer.pGetTimeNs != NULL)
    {
        time = sSysMainTimer.pGetTimeNs(sSysMainTimer.pDriverCtrl);
    }
    else if(sSysMainTimer.pGetFrequency != NULL)
    {
        /* Get the highest time tick */
        maxTick = 0;
        for(i = 0; i < sCpuCount; ++i)
        {
            if(maxTick < spSysTickCount[i])
            {
                maxTick = spSysTickCount[i];
            }
        }
        time = maxTick * 1000000000ULL /
               sSysMainTimer.pGetFrequency(sSysMainTimer.pDriverCtrl);
    }
    else
    {
        time = 0;
    }

    return time;
}

daytime_t timeGetDayTime(void)
{
    daytime_t time;

    if(sSysRtcTimer.pGetDaytime != NULL)
    {
        time = sSysRtcTimer.pGetDaytime(sSysRtcTimer.pDriverCtrl);
    }
    else
    {
        time.hours   = 0;
        time.minutes = 0;
        time.seconds = 0;
    }

    return time;
}

date_t timeGetDate(void)
{
    date_t date;

    if(sSysRtcTimer.pGetDaytime != NULL)
    {
        date = sSysRtcTimer.pGetDate(sSysRtcTimer.pDriverCtrl);
    }
    else
    {
        date.day     = 0;
        date.month   = 0;
        date.weekday = 0;
        date.year    = 0;
    }

    return date;
}

uint64_t timeGetTicks(const uint8_t kCpuId)
{
    if(kCpuId < sCpuCount)
    {
        return spSysTickCount[kCpuId];
    }
    else
    {
        return 0;
    }
}

void timeWaitNoScheduler(const uint64_t ns)
{
    uint64_t currTime;
    uint8_t  cpuId;

    cpuId = cpuGetId();

    spActiveWait[cpuId] = 0;

    if(sSysLifetimeTimer.pGetTimeNs == NULL)
    {
        if(sSysMainTimer.pGetTimeNs != NULL)
        {
            /* Use precise main timer time */
            currTime = sSysMainTimer.pGetTimeNs(sSysMainTimer.pDriverCtrl);
            while(sSysMainTimer.pGetTimeNs(sSysMainTimer.pDriverCtrl) <
                  currTime + ns){}
        }
        else if(sSysMainTimer.pGetFrequency != NULL)
        {
            /* Use ticks */
            spActiveWait[cpuId] = ns + spSysTickCount[cpuId] * 1000000000 /
                         sSysMainTimer.pGetFrequency(sSysMainTimer.pDriverCtrl);
            while(spActiveWait[cpuId] > 0){}
        }
        else
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failed to active wait, no time source present.");
        }
    }
    else
    {
        /* Get current time form lifetimer and wait */
        currTime = sSysLifetimeTimer.pGetTimeNs(sSysLifetimeTimer.pDriverCtrl);
        while(sSysLifetimeTimer.pGetTimeNs(sSysLifetimeTimer.pDriverCtrl) <
              currTime + ns){}
    }
}

/*************************
 * SYSTEM CALL HANDLERS
 *************************/

void timeSyscallHandleClockGetTime(void* pParams)
{
    syscall_gettime_params_t* pClockParam;
    daytime_t                 time;
    date_t                    date;
    uint64_t                  currTime;
    uint32_t                  years;
    uint32_t                  leaps;
    int32_t                   i;
    uint8_t                   leapIdx;

    if(pParams == NULL)
    {
        return;
    }

    pClockParam = pParams;
    if(pClockParam->pTimeSpec == NULL)
    {
        pClockParam->errnoVal = EFAULT;
        return;
    }

    if(pClockParam->clkId == CLOCK_REALTIME)
    {
        time = timeGetDayTime();
        date = timeGetDate();

        /* Manage years */
        years = 0;
        leaps = 0;
        for(i = TIMESPAMP_START_YEAR; i < date.year; ++i)
        {
            if(IS_LEAP_YEAR(i) == true)
            {
                ++leaps;
            }
            else
            {
                ++years;
            }
        }
        currTime = years * SEC_PER_YEAR + leaps * SEC_PER_LEAP_YEAR;

        /* Manage month */
        leapIdx = IS_LEAP_YEAR(date.year) ? 1 : 0;
        for(i = 0; i < date.month - 1; ++i)
        {
            currTime += sSecPerMonth[leapIdx][i];
        }

        /* Manage days */
        currTime += (date.day - 1) * SEC_PER_DAY;

        /* Manage the rest of the clock */
        currTime += time.hours * SEC_PER_HOUR +
                    time.minutes * SEC_PER_MINUTES +
                    time.seconds;

        /* Convert to nanoseconds */
        currTime *= 1000000000;
    }
    else if(pClockParam->clkId == CLOCK_MONOTONIC)
    {
        currTime = timeGetUptime();
    }
    else
    {
        pClockParam->errnoVal = EINVAL;
        return;
    }

    pClockParam->pTimeSpec->tv_sec  = currTime / 1000000000;
    pClockParam->pTimeSpec->tv_nsec = currTime % 1000000000;
}

/************************************ EOF *************************************/