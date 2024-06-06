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
static uint64_t sSysTickCount = 0;

/** @brief The kernel's main timer interrupt source.
 *
 *  @details The kernel's main timer interrupt source. If it's function pointers
 * are NULL, the driver is not initialized.
 */
static kernel_timer_t sSysMainTimer = {
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
static kernel_timer_t sSysRtcTimer = {
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

/** @brief The kernel's lifetime timer.
 *
 *  @details The kernel's lifetime timer. If it's function pointers
 * are NULL, the driver is not initialized.
 */
static kernel_timer_t sSysLifetimeTimer = {
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
static volatile uint64_t sActiveWait = 0;

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
    /* Add a tick count */
    ++sSysTickCount;

    if(sSysMainTimer.pTickManager != NULL)
    {
        sSysMainTimer.pTickManager(sSysMainTimer.pDriverCtrl);
    }

    if(sSchedRoutine != NULL)
    {
        /* We might never come back from here */
        sSchedRoutine(pCurrThread);
    }
    else
    {
        /* Use coars active wait if not lifetime timer is present */
        if(sActiveWait != 0)
        {
            /* Use ticks */
            if(sActiveWait <= sSysTickCount * 1000000000 /
                    sSysMainTimer.pGetFrequency(sSysMainTimer.pDriverCtrl))
            {
                sActiveWait = 0;
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

    if(sSysRtcTimer.pTickManager != NULL)
    {
        sSysRtcTimer.pTickManager(sSysRtcTimer.pDriverCtrl);
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
       kpTimer->pRemoveHandler == NULL)

    {
        KERNEL_ERROR("Timer misses mandatory hooks\n");
        return OS_ERR_NULL_POINTER;
    }

    retCode = OS_NO_ERR;

    switch(kType)
    {
        case MAIN_TIMER:
            sSysMainTimer = *kpTimer;
            retCode = sSysMainTimer.pSetHandler(kpTimer->pDriverCtrl,
                                                 _mainTimerHandler);
            break;
        case RTC_TIMER:
            sSysRtcTimer = *kpTimer;
            retCode = sSysRtcTimer.pSetHandler(kpTimer->pDriverCtrl,
                                                _rtcTimerHandler);
            break;
        case LIFETIME_TIMER:
            sSysLifetimeTimer = *kpTimer;
            break;
        default:
            /* TODO: Add other timers */
            KERNEL_ERROR("Timer type %d not supported\n", kType);
            retCode = OS_ERR_NOT_SUPPORTED;
    }

    if(retCode == OS_NO_ERR)
    {
        kpTimer->pEnable(kpTimer->pDriverCtrl);
    }

    return retCode;
}

uint64_t timeGetUptime(void)
{
    if(sSysLifetimeTimer.pGetTimeNs != NULL)
    {
        return sSysLifetimeTimer.pGetTimeNs(sSysLifetimeTimer.pDriverCtrl);
    }
    else if(sSysMainTimer.pGetTimeNs != NULL)
    {
        return sSysMainTimer.pGetTimeNs(sSysMainTimer.pDriverCtrl);
    }
    else if(sSysMainTimer.pGetFrequency != NULL)
    {
        return sSysTickCount * 1000000000ULL /
               sSysMainTimer.pGetFrequency(sSysMainTimer.pDriverCtrl);
    }
    else
    {
        return 0;
    }
}

time_t timeGetDayTime(void)
{
    time_t time;

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

uint64_t timeGetTicks(void)
{
    return sSysTickCount;
}

void timeWaitNoScheduler(const uint64_t ns)
{
    uint64_t currTime;

    sActiveWait = 0;

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
            sActiveWait = ns + sSysTickCount * 1000000000 /
                         sSysMainTimer.pGetFrequency(sSysMainTimer.pDriverCtrl);
            while(sActiveWait > 0){}
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
}

OS_RETURN_E timeRegisterSchedRoutine(void(*pSchedRoutine)(kernel_thread_t*))
{
    if(pSchedRoutine == NULL)
    {
        KERNEL_ERROR("Invalid NULL scheduler routine\n");
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