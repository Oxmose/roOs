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
#include <stdint.h>        /* Generic int types */
#include <kerror.h>        /* Kernel error codes */
#include <interrupts.h>    /* Interrupt manager */
#include <kerneloutput.h> /* Kernel outputs */

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
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Stores the number of main kernel's timer tick since the
 * initialization of the time manager.
 */
static uint64_t sys_tick_count = 0;

/** @brief The kernel's main timer interrupt source.
 *
 *  @details The kernel's main timer interrupt source. If it's function pointers
 * are NULL, the driver is not initialized.
 */
static kernel_timer_t sys_main_timer = {
    .pGetFrequency  = NULL,
    .pSetFrequency  = NULL,
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
static kernel_timer_t sys_rtc_timer = {
    .pGetFrequency  = NULL,
    .pSetFrequency  = NULL,
    .pGetTimeNs     = NULL,
    .pSetTimeNs     = NULL,
    .pGetDate       = NULL,
    .pGetDaytime    = NULL,
    .pEnable        = NULL,
    .pDisable       = NULL,
    .pSetHandler    = NULL,
    .pRemoveHandler = NULL,
};

/** @brief Active wait counter per CPU. */
static volatile uint64_t active_wait = 0;

/** @brief Stores the routine to call the scheduler. */
void (*sSchedRoutine)(kernel_thread_t*) = NULL;

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

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _mainTimerHandler(kernel_thread_t* pCurrThread)
{
    uint64_t curr_time;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_MAIN_TIMER_HANDLER, 0);

    /* Add a tick count */
    ++sys_tick_count;

    if(sys_main_timer.pTickManager != NULL)
    {
        sys_main_timer.pTickManager();
    }

    if(sSchedRoutine != NULL)
    {
        /* We might never come back from here */
        sSchedRoutine(pCurrThread);
    }
    else
    {
        /* TODO: remove that and use lifetime timer is availabe */
        if(active_wait != 0 && sys_main_timer.pGetTimeNs != NULL)
        {
            /* Use precise time */
            curr_time = sys_main_timer.pGetTimeNs();
            if(active_wait <= curr_time)
            {
                active_wait = 0;
            }
        }
        else
        {
            /* Use ticks */
            if(active_wait <= sys_tick_count * 1000000000 /
                              sys_main_timer.pGetFrequency())
            {
                active_wait = 0;
            }
        }
    }

    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Time manager main handler");
}

static void _rtcTimerHandler(kernel_thread_t* pCurrThread)
{
    (void)pCurrThread;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_TIMER_HANDLER, 0);


    if(sys_rtc_timer.pTickManager != NULL)
    {
        sys_rtc_timer.pTickManager();
    }

    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Time manager RTC handler");
}

OS_RETURN_E timeMgtAddTimer(const kernel_timer_t* kpTimer,
                            const TIMER_TYPE_E    kType)
{
    OS_RETURN_E retCode;

    /* Check the main timer integrity */
    if(kpTimer == NULL ||
       kpTimer->pGetFrequency == NULL ||
       kpTimer->pSetFrequency == NULL ||
       kpTimer->pEnable == NULL ||
       kpTimer->pDisable == NULL ||
       kpTimer->pSetHandler == NULL ||
       kpTimer->pRemoveHandler == NULL ||
       kpTimer->pTickManager == NULL)

    {
        return OS_ERR_NULL_POINTER;
    }

    retCode = OS_NO_ERR;

    switch(kType)
    {
        case MAIN_TIMER:
            sys_main_timer = *kpTimer;
            retCode = sys_main_timer.pSetHandler(_mainTimerHandler);
            break;
        case RTC_TIMER:
            sys_rtc_timer = *kpTimer;
            retCode = sys_rtc_timer.pSetHandler(_rtcTimerHandler);
            break;
        default:
            /* TODO: Add other timers */
            retCode = OS_ERR_NOT_SUPPORTED;
    }

    if(retCode == OS_NO_ERR)
    {
        kpTimer->pEnable();
    }

    return retCode;
}

uint64_t timeGetUptime(void)
{
    if(sys_main_timer.pGetTimeNs == NULL)
    {
        if(sys_main_timer.pGetFrequency == NULL)
        {
            return 0;
        }

        return sys_tick_count * 1000000000ULL / sys_main_timer.pGetFrequency();
    }

    return sys_main_timer.pGetTimeNs();
}

uint64_t timeGetTicks(void)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_GET_TICK_COUNT,
                       2,
                       (uint32_t)sys_tick_count,
                       (uint32_t)(sys_tick_count >> 32));

    return sys_tick_count;
}

void timeWaitNoScheduler(const uint64_t ns)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_TIME_WAIT_NOSCHED_START,
                       2,
                       (uint32_t)ns,
                       (uint32_t)(ns >> 32));

    if(sSchedRoutine != NULL)
    {
        return;
    }

    /* TODO: Use lifetime timer if availabe and wait for count in this function
     * don't rely on ticks
     */
    if(sys_main_timer.pGetTimeNs != NULL)
    {
        /* Use precise time */
        active_wait = ns + sys_main_timer.pGetTimeNs();
    }
    else
    {
        /* Use ticks */
        active_wait = ns + sys_tick_count * 1000000000 /
                           sys_main_timer.pGetFrequency();
    }
    while(active_wait > 0){}

    KERNEL_TRACE_EVENT(EVENT_KERNEL_TIME_WAIT_NOSCHED_END,
                       2,
                       (uint32_t)ns,
                       (uint32_t)(ns >> 32));
}

OS_RETURN_E timeRegisterSchedRoutine(void(*pSchedRoutine)(kernel_thread_t*))
{
#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_TIME_REG_SCHED,
                       2,
                       (uintptr_t)pSchedRoutine & 0xFFFFFFFF,
                       (uintptr_t)pSchedRoutine >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_TIME_REG_SCHED,
                       2,
                       (uintptr_t)pSchedRoutine & 0xFFFFFFFF,
                       (uintptr_t)0);
#endif

    if(pSchedRoutine == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }


    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Registered scheduler routine at 0x%p",
                 pSchedRoutine);

    sSchedRoutine = pSchedRoutine;

    return OS_NO_ERR;
}

/************************************ EOF *************************************/