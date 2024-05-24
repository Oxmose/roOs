/*******************************************************************************
 * @file time_mgt.h
 *
 * @see time_mgt.c
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

#ifndef __TIME_TIME_MGT_H_
#define __TIME_TIME_MGT_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h>     /* Generic int types */
#include <stddef.h>     /* Standard definitions */
#include <ctrl_block.h> /* Kernel structures */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Date structure. */
typedef struct
{
    /** @brief Day of the week. */
    uint16_t weekday;
    /** @brief Day of the month. */
    uint16_t day;
    /** @brief Month of the year. */
    uint16_t month;
    /** @brief Year. */
    uint16_t year;
} date_t;

/** @brief Time structure. */
typedef struct
{
    /** @brief Hours. */
    uint8_t hours;
    /** @brief Minutes. */
    uint8_t minutes;
    /** @brief Seconds. */
    uint8_t seconds;
} time_t;

typedef enum
{
    MAIN_TIMER,
    RTC_TIMER,
    AUX_TIMER,
    LIFETIME_TIMER
} TIMER_TYPE_E;

/**
 * @brief The kernel's timer driver abstraction.
 */
typedef struct
{
    /**
     * @brief The function should return the frequency of the timer source.
     *
     * @details The function should return the frequency of the timer source.
     *
     * @return The function should return the frequency of the timer source.
     */
    uint32_t (*get_frequency)(void);

    /**
     * @brief The function should allow the kernel to set the frequency of a
     * timer source.
     *
     * @details The function should allow the kernel to set the frequency of a
     * timer source. The frequency is defined in Hz.
     *
     * @param[in] frequency The frequency to apply to the timer source.
     */
    void (*set_frequency)(const uint32_t frequency);

    /**
     * @brief Returns the time elasped since the last timer's reset in ns.
     *
     * @details Returns the time elasped since the last timer's reset in ns. The
     * timer can be set with the set_time_ns function.
     *
     * @return The time in nanosecond since the last timer reset is returned.
     */
    uint64_t (*get_time_ns)(void);

    /**
     * @brief Sets the time elasped in ns.
     *
     * @details Sets the time elasped in ns. The timer can be get with the
     * get_time_ns function.
     *
     * @param[in] time_ns The time in nanoseconds to set.
     */
    void (*set_time_ns)(const uint64_t time_ns);

    /**
     * @brief Returns the current date.
     *
     * @details Returns the current date in RTC date format.
     *
     * @returns The current date in in RTC date format
     */
    date_t (*get_date)(void);

    /**
     * @brief Returns the current daytime.
     *
     * @details Returns the current daytime.
     *
     * @returns The current daytime.
     */
    time_t (*get_daytime)(void);

    /**
     * @brief The function should enable the timer's interrupt.
     *
     * @details The function should enable the timer's interrupt.
     */
    void (*enable)(void);

    /**
     * @brief The function should disable the timer's interrupt.
     *
     * @details The function should disable the timer's interrupt.
     */
    void (*disable)(void);

    /**
     * @brief The function should set the timer's tick handler.
     *
     * @details The function should set the timer's tick handler. The handler
     * will be called at each tick received.
     *
     * @param[in] handler The handler of the timer's interrupt.
     *
     * @return The success state or the error code.
     * - OS_NO_ERR is returned if no error is encountered.
     * - OS_ERR_NULL_POINTER is returned if the handler is NULL.
     * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the timer interrupt
     *   line is not allowed.
     * - OS_ERR_NULL_POINTER is returned if the pointer to the handler is NULL.
     * - OS_ERR_INTERRUPT_ALREADY_REGISTERED is returned if a handler is already
     *   registered for the timer.
     */
    OS_RETURN_E (*set_handler)(void(*handler)(kernel_thread_t*));

    /**
     * @brief The function should remove the timer tick handler.
     *
     * @details The function should remove the timer tick handler.
     *
     * @return The success state or the error code.
     * - OS_NO_ERR is returned if no error is encountered.
     * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the timer interrupt
     *   line is not allowed.
     * - OS_ERR_INTERRUPT_NOT_REGISTERED is returned if the timer line has no
     *   handler attached.
     */
    OS_RETURN_E (*remove_handler)(void);

    /**
     * @brief The function should return the IRQ line associated to the timer
     * source.
     *
     * @details The function should return the IRQ line associated to the timer
     * source.
     *
     * @return The function should return the IRQ line associated to the timer
     * source.
     */
    uint32_t (*get_irq)(void);

    /** @brief This function can be used to execute an operation in the
     * driver at every tick of the timer.
     *
     * @details This function can be used to execute an operation in the
     * driver at every tick of the timer such as a tick acknowledge.
     */
    void (*tickManager)(void);
} kernel_timer_t;

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
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * @brief Adds a timer to the manager.
 *
 * @details Adds a timer to the managerr. Set the basic time structures
 * and interrupts.
 *
 * @param[in] kpTimer The timer to add.
 * @param[in] kType The timer type..
 *
 * @warning All the interrupt managers and timer sources drivers must be
 * initialized before using this function.
 *
 * @return The success state or the error code.
 */
OS_RETURN_E timeMgtAddTimer(const kernel_timer_t* kpTimer,
                            const TIMER_TYPE_E kType);

/**
 * @brief Returns the current uptime.
 *
 * @details Return the current uptime of the system in ns.
 *
 * @return The current uptime in ns.
 */
uint64_t time_get_current_uptime(void);

/**
 * @brief Returns the number of system's ticks since the system started.
 *
 * @details Returns the number of system's ticks since the system started.
 *
 * @returns The number of system's ticks since the system started.
 */
uint64_t time_get_tick_count(void);

/**
 * @brief Performs a wait for ms nanoseconds.
 *
 * @details Performs a wait for ms nanoseconds based on the kernel's main
 * timer.
 *
 * @param[in] ns The time to wait in nanoseconds.
 *
 * @warning This function must only be called before the scheduler is
 * initialized. Otherwise the function will immediatly return.
 */
void time_wait_no_sched(const uint64_t ns);

/**
 * @brief Registers the function to call the system's scheduler.
 *
 * @details Registers the function to call the system's scheduler. This function
 * will be called at each tick of the main timer.
 *
 * @param[in] scheduler_call The scheduling routine to call every tick.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_NULL_POINTER if the scheduler routine pointer is NULL.
 */
OS_RETURN_E time_register_scheduler(void(*scheduler_call)(kernel_thread_t*));


#endif /* #ifndef __TIME_TIME_MGT_H_ */

/************************************ EOF *************************************/