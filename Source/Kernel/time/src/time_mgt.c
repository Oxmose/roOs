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
#include <kheap.h>         /* Kernel heap */
#include <stdint.h>        /* Generic int types */
#include <kerror.h>        /* Kernel error codes */
#include <kqueue.h>        /* Kernel queues */
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

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

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
static uint64_t sSysTickCount[MAX_CPU_COUNT] = {0};

/** @brief Auxiliary timers list */
static kqueue_t* spAuxTimersQueue = NULL;

/** @brief Active wait counter per CPU. */
static volatile uint64_t sActiveWait[MAX_CPU_COUNT] = {0};

/** @brief Auxiliary timers list lock */
static kernel_spinlock_t auxTimersListLock = KERNEL_SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _mainTimerHandler(kernel_thread_t* pCurrThread)
{
    (void)pCurrThread;

    uint8_t cpuId;

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_MAIN_HANDLER_ENTRY,
                       1,
                       (uint32_t)pCurrThread->tid);

    cpuId = cpuGetId();

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
    kernel_timer_t* pTimerCopy;
    kqueue_node_t*  pNewNode;

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_AUX_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(kpTimer),
                       KERNEL_TRACE_LOW(kpTimer));

    /* Copy the timer */
    pTimerCopy = kmalloc(sizeof(kernel_timer_t));
    if(pTimerCopy == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_ADD_AUX_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(kpTimer),
                           KERNEL_TRACE_LOW(kpTimer),
                           OS_ERR_NO_MORE_MEMORY);

        return OS_ERR_NO_MORE_MEMORY;
    }
    *pTimerCopy = *kpTimer;

    /* Create the new node */
    pNewNode = kQueueCreateNode(pTimerCopy);
    if(pNewNode == NULL)
    {
        kfree(pTimerCopy);

        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_ADD_AUX_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(kpTimer),
                           KERNEL_TRACE_LOW(kpTimer),
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    KERNEL_CRITICAL_LOCK(auxTimersListLock);

    /* Create queue is it does not exist */
    if(spAuxTimersQueue == NULL)
    {
        spAuxTimersQueue = kQueueCreate();
    }
    if(spAuxTimersQueue == NULL)
    {
        KERNEL_CRITICAL_UNLOCK(auxTimersListLock);
        kQueueDestroyNode(&pNewNode);
        kfree(pTimerCopy);

        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_ADD_AUX_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(kpTimer),
                           KERNEL_TRACE_LOW(kpTimer),
                           OS_ERR_NULL_POINTER);

        return OS_ERR_NULL_POINTER;
    }

    /* Add the timer to the queue */
    kQueuePush(pNewNode, spAuxTimersQueue);

    KERNEL_CRITICAL_UNLOCK(auxTimersListLock);

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_AUX_EXIT,
                       3,
                       KERNEL_TRACE_HIGH(kpTimer),
                       KERNEL_TRACE_LOW(kpTimer),
                       OS_NO_ERR);
    return OS_NO_ERR;
}

OS_RETURN_E timeMgtAddTimer(const kernel_timer_t* kpTimer,
                            const TIMER_TYPE_E    kType)
{
    OS_RETURN_E retCode;

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_TIMER_ENTRY,
                       3,
                       KERNEL_TRACE_HIGH(kpTimer),
                       KERNEL_TRACE_LOW(kpTimer),
                       kType);

    /* Check the main timer integrity */
    if(kpTimer == NULL ||
       kpTimer->pGetFrequency == NULL ||
       kpTimer->pEnable == NULL ||
       kpTimer->pDisable == NULL ||
       kpTimer->pSetHandler == NULL ||
       kpTimer->pRemoveHandler == NULL)

    {
        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_ADD_TIMER_EXIT,
                           4,
                           KERNEL_TRACE_HIGH(kpTimer),
                           KERNEL_TRACE_LOW(kpTimer),
                           kType,
                           OS_ERR_NULL_POINTER);

        return OS_ERR_NULL_POINTER;
    }

    retCode = OS_NO_ERR;

    switch(kType)
    {
        case MAIN_TIMER:
            sSysMainTimer = *kpTimer;
            retCode = kpTimer->pSetHandler(kpTimer->pDriverCtrl,
                                           _mainTimerHandler);
            break;
        case RTC_TIMER:
            sSysRtcTimer = *kpTimer;
            retCode = kpTimer->pSetHandler(kpTimer->pDriverCtrl,
                                           _rtcTimerHandler);
            break;
        case LIFETIME_TIMER:
            sSysLifetimeTimer = *kpTimer;
            break;
        case AUX_TIMER:
            retCode = _timeMgtAddAuxTimer(kpTimer);
            break;
        default:
            retCode = OS_ERR_NOT_SUPPORTED;
    }

    /* The auxiliary timers will be enable on demand, as they might generate
     * unhandled interrupts.
     */
    if(retCode == OS_NO_ERR && kType != AUX_TIMER)
    {
        kpTimer->pEnable(kpTimer->pDriverCtrl);
    }

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_TIMER_EXIT,
                       4,
                       KERNEL_TRACE_HIGH(kpTimer),
                       KERNEL_TRACE_LOW(kpTimer),
                       kType,
                       retCode);

    return retCode;
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
        for(i = 0; i < MAX_CPU_COUNT; ++i)
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
    if(kCpuId < MAX_CPU_COUNT)
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