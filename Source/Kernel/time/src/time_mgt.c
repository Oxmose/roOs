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

/** @brief Stores the routine to call the scheduler. */
static void (*sSchedRoutine)(kernel_thread_t*) = NULL;

/** @brief Timers manager global lock */
static kernel_spinlock_t managerLock = KERNEL_SPINLOCK_INIT_VALUE;

/** @brief Auxiliary timers list lock */
static kernel_spinlock_t auxTimersListLock = KERNEL_SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _mainTimerHandler(kernel_thread_t* pCurrThread)
{
    uint8_t cpuId;
    void    (*pCurrSched)(kernel_thread_t*);

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_MAIN_HANDLER_ENTRY,
                       1,
                       (uint32_t)pCurrThread->tid);

    cpuId = cpuGetId();

    /* Add a tick count */
    ++sSysTickCount[cpuId];

    KERNEL_CRITICAL_LOCK(managerLock);
    if(sSysMainTimer.pTickManager != NULL)
    {
        sSysMainTimer.pTickManager(sSysMainTimer.pDriverCtrl);
    }

    if(sSchedRoutine != NULL)
    {
        pCurrSched = sSchedRoutine;
        KERNEL_CRITICAL_UNLOCK(managerLock);
        /* We might never come back from here */
        pCurrSched(pCurrThread);
    }
    else
    {
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
        KERNEL_CRITICAL_UNLOCK(managerLock);
    }

    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Time manager main handler");

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

    KERNEL_CRITICAL_LOCK(managerLock);

    if(sSysRtcTimer.pTickManager != NULL)
    {
        sSysRtcTimer.pTickManager(sSysRtcTimer.pDriverCtrl);
    }

    KERNEL_CRITICAL_UNLOCK(managerLock);

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

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_AUX_ENTRY,
                       2,
                       0,
                       (uint32_t)kpTimer);
#else
    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_AUX_ENTRY,
                       2,
                       (uint32_t)(kpTimer >> 32),
                       (uint32_t)(kpTimer & 0xFFFFFFFF));
#endif

    /* Copy the timer */
    pTimerCopy = kmalloc(sizeof(kernel_timer_t));
    if(pTimerCopy == NULL)
    {
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_ADD_AUX_EXIT,
                           3,
                           0,
                           (uint32_t)kpTimer,
                           OS_ERR_NO_MORE_MEMORY);
#else
        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_ADD_AUX_EXIT,
                           3,
                           (uint32_t)(kpTimer >> 32),
                           (uint32_t)(kpTimer & 0xFFFFFFFF),
                           OS_ERR_NO_MORE_MEMORY);
#endif
        return OS_ERR_NO_MORE_MEMORY;
    }
    *pTimerCopy = *kpTimer;

    /* Create the new node */
    pNewNode = kQueueCreateNode(pTimerCopy);
    if(pNewNode == NULL)
    {
        kfree(pTimerCopy);

#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_ADD_AUX_EXIT,
                           3,
                           0,
                           (uint32_t)kpTimer,
                           OS_ERR_NULL_POINTER);
#else
        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_ADD_AUX_EXIT,
                           3,
                           (uint32_t)(kpTimer >> 32),
                           (uint32_t)(kpTimer & 0xFFFFFFFF),
                           OS_ERR_NULL_POINTER);
#endif
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

#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_ADD_AUX_EXIT,
                           3,
                           0,
                           (uint32_t)kpTimer,
                           OS_ERR_NULL_POINTER);
#else
        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_ADD_AUX_EXIT,
                           3,
                           (uint32_t)(kpTimer >> 32),
                           (uint32_t)(kpTimer & 0xFFFFFFFF),
                           OS_ERR_NULL_POINTER);
#endif
        return OS_ERR_NULL_POINTER;
    }

    /* Add the timer to the queue */
    kQueuePush(pNewNode, spAuxTimersQueue);

    KERNEL_CRITICAL_UNLOCK(auxTimersListLock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_AUX_EXIT,
                       3,
                       0,
                       (uint32_t)kpTimer,
                       OS_NO_ERR);
#else
    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_AUX_EXIT,
                       3,
                       (uint32_t)(kpTimer >> 32),
                       (uint32_t)(kpTimer & 0xFFFFFFFF),
                       OS_NO_ERR);
#endif
    return OS_NO_ERR;
}

OS_RETURN_E timeMgtAddTimer(const kernel_timer_t* kpTimer,
                            const TIMER_TYPE_E    kType)
{
    OS_RETURN_E retCode;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_TIMER_ENTRY,
                       3,
                       0,
                       (uint32_t)kpTimer,
                       kType);
#else
    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_TIMER_ENTRY,
                       3,
                       (uint32_t)(kpTimer >> 32),
                       (uint32_t)(kpTimer & 0xFFFFFFFF),
                       kType);
#endif

    /* Check the main timer integrity */
    if(kpTimer == NULL ||
       kpTimer->pGetFrequency == NULL ||
       kpTimer->pEnable == NULL ||
       kpTimer->pDisable == NULL ||
       kpTimer->pSetHandler == NULL ||
       kpTimer->pRemoveHandler == NULL)

    {
        KERNEL_ERROR("Timer misses mandatory hooks\n");

#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_ADD_TIMER_EXIT,
                           4,
                           0,
                           (uint32_t)kpTimer,
                           kType,
                           OS_ERR_NULL_POINTER);
#else
        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_ADD_TIMER_EXIT,
                           4,
                           (uint32_t)(kpTimer >> 32),
                           (uint32_t)(kpTimer & 0xFFFFFFFF),
                           kType,
                           OS_ERR_NULL_POINTER);
#endif
        return OS_ERR_NULL_POINTER;
    }

    retCode = OS_NO_ERR;

    KERNEL_CRITICAL_LOCK(managerLock);
    switch(kType)
    {
        case MAIN_TIMER:
            sSysMainTimer = *kpTimer;
            KERNEL_CRITICAL_UNLOCK(managerLock);
            retCode = kpTimer->pSetHandler(kpTimer->pDriverCtrl,
                                           _mainTimerHandler);
            break;
        case RTC_TIMER:
            sSysRtcTimer = *kpTimer;
            KERNEL_CRITICAL_UNLOCK(managerLock);
            retCode = kpTimer->pSetHandler(kpTimer->pDriverCtrl,
                                           _rtcTimerHandler);
            break;
        case LIFETIME_TIMER:
            sSysLifetimeTimer = *kpTimer;
            KERNEL_CRITICAL_UNLOCK(managerLock);
            break;
        case AUX_TIMER:
            KERNEL_CRITICAL_UNLOCK(managerLock);
            retCode = _timeMgtAddAuxTimer(kpTimer);
            break;
        default:
            KERNEL_CRITICAL_UNLOCK(managerLock);
            KERNEL_ERROR("Timer type %d not supported\n", kType);
            retCode = OS_ERR_NOT_SUPPORTED;
    }

    /* The auxiliary timers will be enable on demand, as they might generate
     * unhandled interrupts.
     */
    if(retCode == OS_NO_ERR && kType != AUX_TIMER)
    {
        kpTimer->pEnable(kpTimer->pDriverCtrl);
    }

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_TIMER_EXIT,
                       4,
                       0,
                       (uint32_t)kpTimer,
                       kType,
                       retCode);
#else
    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_ADD_TIMER_EXIT,
                       4,
                       (uint32_t)(kpTimer >> 32),
                       (uint32_t)(kpTimer & 0xFFFFFFFF),
                       kType,
                       retCode);
#endif

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

    KERNEL_CRITICAL_LOCK(managerLock);

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
    /* TODO: Check RTC */
    else
    {
        time = 0;
    }

    KERNEL_CRITICAL_UNLOCK(managerLock);

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_GET_UPTIME_EXIT,
                       2,
                       (uint32_t)(time >> 32),
                       (uint32_t)(time & 0xFFFFFFFF));

    return time;
}

time_t timeGetDayTime(void)
{
    time_t time;

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_GET_DAYTIME_ENTRY,
                       0);

    KERNEL_CRITICAL_LOCK(managerLock);
    if(sSysRtcTimer.pGetDaytime != NULL)
    {
        time = sSysRtcTimer.pGetDaytime(sSysRtcTimer.pDriverCtrl);
        KERNEL_CRITICAL_UNLOCK(managerLock);
    }
    else
    {
        KERNEL_CRITICAL_UNLOCK(managerLock);
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
                       (uint32_t)(ns & 0xFFFFFFFF));

    if(sSchedRoutine != NULL)
    {
        return;
    }

    cpuId = cpuGetId();

    sActiveWait[cpuId] = 0;

    /* We can lock here, the is no scheduler using this */
    KERNEL_CRITICAL_LOCK(managerLock);
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
        else/* TODO: Check aux and RTC*/
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
    KERNEL_CRITICAL_UNLOCK(managerLock);

    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_WAIT_NO_SCHED_ENTRY,
                       2,
                       (uint32_t)(ns >> 32),
                       (uint32_t)(ns & 0xFFFFFFFF));
}

OS_RETURN_E timeRegisterSchedRoutine(void(*pSchedRoutine)(kernel_thread_t*))
{
#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_REG_SCHED_ENTRY,
                       2,
                       0,
                       (uint32_t)pSchedRoutine);
#else
    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_REG_SCHED_ENTRY,
                       2,
                       (uint32_t)(pSchedRoutine >> 32),
                       (uint32_t)(pSchedRoutine & 0xFFFFFFFF));
#endif

    KERNEL_CRITICAL_LOCK(managerLock);
    if(pSchedRoutine == NULL)
    {
        KERNEL_CRITICAL_UNLOCK(managerLock);
        KERNEL_ERROR("Invalid NULL scheduler routine\n");
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_REG_SCHED_EXIT,
                           3,
                           0,
                           (uint32_t)pSchedRoutine,
                           OS_ERR_NULL_POINTER);
#else
        KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                           TRACE_TIME_MGT_REG_SCHED_EXIT,
                           3,
                           (uint32_t)(pSchedRoutine >> 32),
                           (uint32_t)(pSchedRoutine & 0xFFFFFFFF),
                           OS_ERR_NULL_POINTER);
#endif
        return OS_ERR_NULL_POINTER;
    }


    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Registered scheduler routine at 0x%p",
                 pSchedRoutine);

    sSchedRoutine = pSchedRoutine;
    KERNEL_CRITICAL_LOCK(managerLock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_REG_SCHED_EXIT,
                       3,
                       0,
                       (uint32_t)pSchedRoutine,
                       OS_NO_ERR);
#else
    KERNEL_TRACE_EVENT(TRACE_TIME_MGT_ENABLED,
                       TRACE_TIME_MGT_REG_SCHED_EXIT,
                       3,
                       (uint32_t)(pSchedRoutine >> 32),
                       (uint32_t)(pSchedRoutine & 0xFFFFFFFF),
                       OS_NO_ERR);
#endif
    return OS_NO_ERR;
}

/************************************ EOF *************************************/