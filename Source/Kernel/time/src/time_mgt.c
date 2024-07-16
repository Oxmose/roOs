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
#include <panic.h>         /* Kernel panic */
#include <kheap.h>         /* Kernel heap */
#include <stdint.h>        /* Generic int types */
#include <kerror.h>        /* Kernel error codes */
#include <kqueue.h>        /* Kernel queues */
#include <devtree.h>       /* Device tree lib */
#include <drivermgr.h>     /* Driver manager */
#include <interrupts.h>    /* Interrupt manager */
#include <kerneloutput.h>  /* Kernel outputs */

/* Configuration files */
#include <config.h>

/* Header file */
#include <time_mgt.h>

/* Tracing feature */
#include <tracing.h>

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


/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

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
    if((COND) == FALSE)                                     \
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
 */
static void _mainTimerHandler(kernel_thread_t* pCurrThread);

/**
 * @brief The kernel's RTC timer interrupt handler.
 *
 * @details The kernel's RTC timer interrupt handler. This must be connected to
 * the RTC timer of the system.
 *
 * @param[in, out] pCurrThread Current thread at the moment of the interrupt.
 */
static void _rtcTimerHandler(kernel_thread_t* pCurrThread);

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
static uint64_t sSysTickCount[SOC_CPU_COUNT] = {0};

/** @brief Auxiliary timers list */
static kqueue_t* spAuxTimersQueue = NULL;

/** @brief Active wait counter per CPU. */
static volatile uint64_t sActiveWait[SOC_CPU_COUNT] = {0};

/** @brief Auxiliary timers list lock */
static kernel_spinlock_t sAuxTimersListLock = KERNEL_SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _mainTimerHandler(kernel_thread_t* pCurrThread)
{
    uint8_t cpuId;

    cpuId = cpuGetId();

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_MAIN_HANDLER_ENTRY,
                       1,
                       (uint32_t)pCurrThread->tid);


    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Time manager main handler");

    /* Add a tick count */
    ++sSysTickCount[cpuId];

    if(sSysMainTimer.pTickManager != NULL)
    {
        sSysMainTimer.pTickManager(sSysMainTimer.pDriverCtrl);
    }

    /* Use coarse active wait if not lifetime timer is present */
    if(sActiveWait[cpuId] != 0)
    {
        /* Use ticks */
        if(sActiveWait[cpuId] <= sSysTickCount[cpuId] * 1000000000 /
                sSysMainTimer.pGetFrequency(sSysMainTimer.pDriverCtrl))
        {
            sActiveWait[cpuId] = 0;
        }
    }

    /* The main time triggered, we need to schedule the thread */
    pCurrThread->requestSchedule = TRUE;

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_MAIN_HANDLER_EXIT,
                       1,
                       (uint32_t)pCurrThread->tid);
}

static void _rtcTimerHandler(kernel_thread_t* pCurrThread)
{
    (void)pCurrThread;

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_RTC_HANDLER_ENTRY,
                       1,
                       (uint32_t)pCurrThread->tid);

    if(sSysRtcTimer.pTickManager != NULL)
    {
        sSysRtcTimer.pTickManager(sSysRtcTimer.pDriverCtrl);
    }

    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Time manager RTC handler");

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_RTC_HANDLER_EXIT,
                       1,
                       (uint32_t)pCurrThread->tid);
}

static OS_RETURN_E _timeMgtAddAuxTimer(const kernel_timer_t* kpTimer)
{
    kqueue_node_t* pNewNode;

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_AUX_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(kpTimer),
                       KERNEL_TRACE_LOW(kpTimer));

    KERNEL_LOCK(sAuxTimersListLock);

    /* Create queue is it does not exist */
    if(spAuxTimersQueue == NULL)
    {
        spAuxTimersQueue = kQueueCreate(FALSE);
        if(spAuxTimersQueue == NULL)
        {
            KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                               TRACE_TIME_MGT_ADD_AUX_EXIT,
                               3,
                               KERNEL_TRACE_HIGH(kpTimer),
                               KERNEL_TRACE_LOW(kpTimer),
                               OS_ERR_NO_MORE_MEMORY);
            return OS_ERR_NO_MORE_MEMORY;
        }
    }

    KERNEL_UNLOCK(sAuxTimersListLock);

    /* Create the new node */
    pNewNode = kQueueCreateNode((void*)kpTimer, FALSE);
    if(pNewNode == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_ADD_AUX_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(kpTimer),
                           KERNEL_TRACE_LOW(kpTimer),
                           OS_ERR_NO_MORE_MEMORY);
        return OS_ERR_NO_MORE_MEMORY;
    }

    /* Add the timer to the queue */
    kQueuePush(pNewNode, spAuxTimersQueue);

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_AUX_EXIT,
                       3,
                       KERNEL_TRACE_HIGH(kpTimer),
                       KERNEL_TRACE_LOW(kpTimer),
                       OS_NO_ERR);
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

    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED, MODULE_NAME, "Adding main timer");
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

    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED, MODULE_NAME, "Added main timer");

    /* Get the RTC driver */
    kpUintProp = fdtGetProp(kpTimerNode,
                            FDT_TIMECONFIG_RTC_PROP,
                            &propLen);
    if(kpUintProp != NULL)
    {
        KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED, MODULE_NAME, "Adding RTC timer");
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

        KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED, MODULE_NAME, "Added RTC timer");
    }


    /* Get the lifetime driver */
    kpUintProp = fdtGetProp(kpTimerNode,
                            FDT_TIMECONFIG_LIFETIME_PROP,
                            &propLen);
    if(kpUintProp != NULL)
    {
        KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Adding lifetime timer");

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

        KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Added lifetime timer");
    }

    /* Get other auxiliary timers */
    kpUintProp = fdtGetProp(kpTimerNode,
                            FDT_TIMECONFIG_AUX_PROP,
                            &propLen);
    if(kpUintProp != NULL)
    {
        for(i = 0; i < propLen / sizeof(uint32_t); ++i)
        {
            KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                         MODULE_NAME,
                         "Adding aux timer %d",
                         i);

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
            KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                         MODULE_NAME,
                         "Added aux timer %d",
                         i);
        }
    }
}

uint64_t timeGetUptime(void)
{
    uint64_t time;
    uint64_t maxTick;
    uint32_t i;

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_GET_UPTIME_ENTRY,
                       0);

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
        for(i = 0; i < SOC_CPU_COUNT; ++i)
        {
            if(maxTick < sSysTickCount[i])
            {
                maxTick = sSysTickCount[i];
            }
        }
        time = maxTick * 1000000000ULL /
               sSysMainTimer.pGetFrequency(sSysMainTimer.pDriverCtrl);
    }
    else
    {
        time = 0;
    }

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_GET_UPTIME_EXIT,
                       2,
                       (uint32_t)(time >> 32),
                       (uint32_t)time);

    return time;
}

time_t timeGetDayTime(void)
{
    time_t time;

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_GET_DAYTIME_ENTRY,
                       0);

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

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_GET_DAYTIME_EXIT,
                       3,
                       time.hours,
                       time.minutes,
                       time.seconds);

    return time;
}

uint64_t timeGetTicks(const uint8_t kCpuId)
{
    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_GET_TICKS,
                       0);
    if(kCpuId < SOC_CPU_COUNT)
    {
        return sSysTickCount[kCpuId];
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

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_WAIT_NO_SCHED_ENTRY,
                       2,
                       (uint32_t)(ns >> 32),
                       (uint32_t)ns);

    cpuId = cpuGetId();

    sActiveWait[cpuId] = 0;

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
            sActiveWait[cpuId] = ns + sSysTickCount[cpuId] * 1000000000 /
                         sSysMainTimer.pGetFrequency(sSysMainTimer.pDriverCtrl);
            while(sActiveWait[cpuId] > 0){}
        }
        else
        {
            KERNEL_ERROR("Failed to active wait, no time source present.\n");
        }
    }
    else
    {
        /* Get current time form lifetimer and wait */
        currTime = sSysLifetimeTimer.pGetTimeNs(sSysLifetimeTimer.pDriverCtrl);
        while(sSysLifetimeTimer.pGetTimeNs(sSysLifetimeTimer.pDriverCtrl) <
              currTime + ns){}
    }

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_WAIT_NO_SCHED_ENTRY,
                       2,
                       (uint32_t)(ns >> 32),
                       (uint32_t)ns);
}

/************************************ EOF *************************************/