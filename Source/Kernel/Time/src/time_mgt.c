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
#include <kernel_output.h> /* Kernel outputs */

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
static uint64_t sys_tick_count;

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
    .enable         = NULL,
    .disable        = NULL,
    .set_handler    = NULL,
    .remove_handler = NULL,
    .get_irq        = NULL
};

/** @brief Active wait counter per CPU. */
static volatile uint64_t active_wait;

/** @brief Stores the routine to call the scheduler. */
void (*schedule_routine)(kernel_thread_t*) = NULL;

/** @brief RTC interrupt manager */
void (*rtc_int_manager)(void) = NULL;

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

    if(schedule_routine != NULL)
    {
        /* We might never come back from here */
        schedule_routine(curr_thread);
    }
    else
    {
        if(active_wait != 0)
        {
            curr_time = sys_main_timer.get_time_ns();
            if(active_wait <= curr_time)
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

    if(rtc_int_manager != NULL)
    {
        rtc_int_manager();
    }

    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Time manager RTC handler");

    /* EOI */
    kernel_interrupt_set_irq_eoi(sys_rtc_timer.get_irq());
}

OS_RETURN_E time_init(const kernel_timer_t* main_timer,
                      const kernel_timer_t* rtc_timer)
{
    OS_RETURN_E err;
#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_INIT_TIME_MGT_START, 4,
                       (uintptr_t)main_timer & 0xFFFFFFFF,
                       (uintptr_t)main_timer >> 32,
                       (uintptr_t)rtc_timer & 0xFFFFFFFF,
                       (uintptr_t)rtc_timer >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_INIT_TIME_MGT_START, 4,
                       (uintptr_t)main_timer & 0xFFFFFFFF,
                       (uintptr_t)0,
                       (uintptr_t)rtc_timer & 0xFFFFFFFF,
                       (uintptr_t)0);
#endif

    /* Check the main timer integrity */
    if(main_timer == NULL ||
       main_timer->get_frequency == NULL ||
       main_timer->set_frequency == NULL ||
       main_timer->enable == NULL ||
       main_timer->disable == NULL ||
       main_timer->set_handler == NULL ||
       main_timer->remove_handler == NULL ||
       main_timer->get_irq == NULL)

    {
        return OS_ERR_NULL_POINTER;
    }
    sys_main_timer = *main_timer;

    /* Check the rtc timer integrity */
    if(rtc_timer != NULL)
    {
        if(rtc_timer->get_frequency != NULL &&
           rtc_timer->set_frequency != NULL &&
           rtc_timer->enable != NULL &&
           rtc_timer->disable != NULL &&
           rtc_timer->set_handler != NULL &&
           rtc_timer->remove_handler != NULL &&
           rtc_timer->get_irq != NULL)
        {
            sys_rtc_timer = *rtc_timer;
        }
        else
        {
            return OS_ERR_NULL_POINTER;
        }
    }

    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Time manager Initialization");

    /* Init the system's values */
    sys_tick_count = 0;
    active_wait    = 0;

    /* Sets all the possible timer interrutps */
    if(sys_main_timer.set_time_ns != NULL)
    {
        sys_main_timer.set_time_ns(0);
    }
    sys_main_timer.set_frequency(KERNEL_MAIN_TIMER_FREQ);

    err = sys_main_timer.set_handler(_time_main_timer_handler);
    if(err != OS_NO_ERR)
    {
        return err;
    }

    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Initialized main timer");

    if(sys_rtc_timer.set_frequency != NULL)
    {
        sys_rtc_timer.set_frequency(KERNEL_RTC_TIMER_FREQ);

        err = sys_rtc_timer.set_handler(_time_rtc_timer_handler);
        if(err != OS_NO_ERR)
        {
            return err;
        }
    }

    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Initialized RTC timer");

    /* Enables all the possible timers */
    sys_main_timer.enable();
    if(sys_rtc_timer.set_frequency != NULL)
    {
        sys_rtc_timer.enable();
    }

    KERNEL_TRACE_EVENT(EVENT_KERNEL_INIT_TIME_MGT_END, 0);

    return OS_NO_ERR;
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
    active_wait = ns;
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

OS_RETURN_E time_register_rtc_manager(void (*rtc_manager)(void))
{
#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_TIME_REG_RTC_MGT, 2,
                       (uintptr_t)rtc_manager & 0xFFFFFFF,
                       (uintptr_t)rtc_manager >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_TIME_REG_RTC_MGT, 2,
                       (uintptr_t)rtc_manager & 0xFFFFFFF,
                       (uintptr_t)0);
#endif

    if(rtc_manager == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    KERNEL_DEBUG(TIME_MGT_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Registered RTC routine at 0x%p", rtc_manager);

    rtc_int_manager = rtc_manager;

    return OS_NO_ERR;
}

/************************************ EOF *************************************/