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
    .get_frequency  = NULL,
    .set_frequency  = NULL,
    .get_time_ns    = NULL,
    .set_time_ns    = NULL,
    .get_date       = NULL,
    .get_daytime    = NULL,
    .enable         = NULL,
    .disable        = NULL,
    .set_handler    = NULL,
    .remove_handler = NULL,
    .get_irq        = NULL
};

/** @brief The kernel's RTC timer interrupt source.
 *
 *  @details The kernel's RTC timer interrupt source. If it's function pointers
 * are NULL, the driver is not initialized.
 */
static kernel_timer_t sys_rtc_timer = {
    .get_frequency  = NULL,
    .set_frequency  = NULL,
    .get_time_ns    = NULL,
    .set_time_ns    = NULL,
    .get_date       = NULL,
    .get_daytime    = NULL,
    .enable         = NULL,
    .disable        = NULL,
    .set_handler    = NULL,
    .remove_handler = NULL,
    .get_irq        = NULL
};

/** @brief Active wait counter per CPU. */
static volatile uint64_t active_wait = 0;

/** @brief Stores the routine to call the scheduler. */
void (*schedule_routine)(kernel_thread_t*) = NULL;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief The kernel's main timer interrupt handler.
 *
 * @details The kernel's main timer interrupt handler. This must be connected to
 * the main timer of the system.
 *
 * @param[in, out] curr_thread Current thread at the moment of the interrupt.
 */
static void _time_main_timer_handler(kernel_thread_t* curr_thread);

/**
 * @brief The kernel's RTC timer interrupt handler.
 *
 * @details The kernel's RTC timer interrupt handler. This must be connected to
 * the RTC timer of the system.
 *
 * @param[in, out] curr_thread Current thread at the moment of the interrupt.
 */
static void _time_rtc_timer_handler(kernel_thread_t* curr_thread);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _time_main_timer_handler(kernel_thread_t* curr_thread)
{
    uint64_t curr_time;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_MAIN_TIMER_HANDLER, 0);

    /* Add a tick count */
    ++sys_tick_count;

    /* EOI */
    kernel_interrupt_set_irq_eoi(sys_main_timer.get_irq());

    if(sys_main_timer.tickManager != NULL)
    {
        sys_main_timer.tickManager();
    }

    if(schedule_routine != NULL)
    {
        /* We might never come back from here */
        schedule_routine(curr_thread);
    }
    else
    {
        /* TODO: remove that and use lifetime timer is availabe */
        if(active_wait != 0 && sys_main_timer.get_time_ns != NULL)
        {
            /* Use precise time */
            curr_time = sys_main_timer.get_time_ns();
            if(active_wait <= curr_time)
            {
                active_wait = 0;
            }
        }
        else
        {
            /* Use ticks */
            if(active_wait <= sys_tick_count * 1000000000 /
                              sys_main_timer.get_frequency())
            {
                active_wait = 0;
            }
        }
    }

    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Time manager main handler");
}

static void _time_rtc_timer_handler(kernel_thread_t* curr_thread)
{
    (void)curr_thread;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_RTC_TIMER_HANDLER, 0);


    if(sys_rtc_timer.tickManager != NULL)
    {
        sys_rtc_timer.tickManager();
    }

    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Time manager RTC handler");

    /* EOI */
    kernel_interrupt_set_irq_eoi(sys_rtc_timer.get_irq());
}

OS_RETURN_E timeMgtAddTimer(const kernel_timer_t* kpTimer,
                            const TIMER_TYPE_E kType)
{
    OS_RETURN_E retCode;

    /* TODO: Add tracing */

    /* Check the main timer integrity */
    if(kpTimer == NULL ||
       kpTimer->get_frequency == NULL ||
       kpTimer->set_frequency == NULL ||
       kpTimer->enable == NULL ||
       kpTimer->disable == NULL ||
       kpTimer->set_handler == NULL ||
       kpTimer->remove_handler == NULL ||
       kpTimer->get_irq == NULL)

    {
        return OS_ERR_NULL_POINTER;
    }

    retCode = OS_NO_ERR;

    switch(kType)
    {
        case MAIN_TIMER:
            sys_main_timer = *kpTimer;
            retCode = sys_main_timer.set_handler(_time_main_timer_handler);
            if(retCode == OS_NO_ERR)
            {
                sys_main_timer.enable();
            }
            break;
        case RTC_TIMER:
            sys_rtc_timer = *kpTimer;
            retCode = sys_rtc_timer.set_handler(_time_rtc_timer_handler);
            if(retCode == OS_NO_ERR)
            {
                sys_rtc_timer.enable();
            }
            break;
        default:
            /* TODO: Add other timers */
            retCode = OS_ERR_NOT_SUPPORTED;
    }

    return retCode;
}

uint64_t time_get_current_uptime(void)
{
    if(sys_main_timer.get_time_ns == NULL)
    {
        if(sys_main_timer.get_frequency == NULL)
        {
            return 0;
        }

        return sys_tick_count * 1000000000ULL / sys_main_timer.get_frequency();
    }

    return sys_main_timer.get_time_ns();
}

uint64_t time_get_tick_count(void)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_GET_TICK_COUNT, 2,
                       (uint32_t)sys_tick_count,
                       (uint32_t)(sys_tick_count >> 32));

    return sys_tick_count;
}

void time_wait_no_sched(const uint64_t ns)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_TIME_WAIT_NOSCHED_START, 2,
                       (uint32_t)ns,
                       (uint32_t)(ns >> 32));

    if(schedule_routine != NULL)
    {
        return;
    }

    /* TODO: Use lifetime timer is availabe and wait for count in this function
     * don't rely on ticks
     */
    if(sys_main_timer.get_time_ns != NULL)
    {
        /* Use precise time */
        active_wait = ns + sys_main_timer.get_time_ns();
    }
    else
    {
        /* Use ticks */
        active_wait = ns + sys_tick_count * 1000000000 /
                           sys_main_timer.get_frequency();
    }
    while(active_wait > 0){}

    KERNEL_TRACE_EVENT(EVENT_KERNEL_TIME_WAIT_NOSCHED_END, 2,
                       (uint32_t)ns,
                       (uint32_t)(ns >> 32));
}

OS_RETURN_E time_register_scheduler(void(*scheduler_call)(kernel_thread_t*))
{
#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_TIME_REG_SCHED, 2,
                       (uintptr_t)scheduler_call & 0xFFFFFFFF,
                       (uintptr_t)scheduler_call >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_TIME_REG_SCHED, 2,
                       (uintptr_t)scheduler_call & 0xFFFFFFFF,
                       (uintptr_t)0);
#endif

    if(scheduler_call == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }


    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Registered scheduler routine at 0x%p", scheduler_call);

    schedule_routine = scheduler_call;

    return OS_NO_ERR;
}

/************************************ EOF *************************************/