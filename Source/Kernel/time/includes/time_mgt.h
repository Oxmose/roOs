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
#include <critical.h>   /* Kernel critical */

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

/** @brief Defines the types of timers available. */
typedef enum
{
    /** @brief Main timer, used for scheduling and general time keeping. */
    MAIN_TIMER,
    /** @brief RTC timer, used real time clock information. */
    RTC_TIMER,
    /** @brief Auxiliary timers, can be used for general purpose. */
    AUX_TIMER,
    /** @brief Lifetime timer, keeps track of the uptime. */
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
     * @param[in, out] pDriverCtrl The driver controler used by the registered
     * console driver.
     *
     * @return The function should return the frequency of the timer source.
     */
    uint32_t (*pGetFrequency)(void* pDriverCtrl);

    /**
     * @brief Returns the time elasped since the last timer's reset in ns.
     *
     * @details Returns the time elasped since the last timer's reset in ns. The
     * timer can be set with the pSetTimeNs function.
     *
     * @param[in, out] pDriverCtrl The driver controler used by the registered
     * console driver.
     *
     * @return The time in nanosecond since the last timer reset is returned.
     */
    uint64_t (*pGetTimeNs)(void* pDriverCtrl);

    /**
     * @brief Sets the time elasped in ns.
     *
     * @details Sets the time elasped in ns. The timer can be get with the
     * pGetTimeNs function.
     *
     * @param[in, out] pDriverCtrl The driver controler used by the registered
     * console driver.
     * @param[in] kTimeNS The time in nanoseconds to set.
     */
    void (*pSetTimeNs)(void* pDriverCtrl, const uint64_t kTimeNS);

    /**
     * @brief Returns the current date.
     *
     * @details Returns the current date in RTC date format.
     *
     * @param[in, out] pDriverCtrl The driver controler used by the registered
     * console driver.
     *
     * @return The current date in in RTC date format
     */
    date_t (*pGetDate)(void* pDriverCtrl);

    /**
     * @brief Returns the current daytime.
     *
     * @details Returns the current daytime.
     *
     * @param[in, out] pDriverCtrl The driver controler used by the registered
     * console driver.
     *
     * @return The current daytime.
     */
    time_t (*pGetDaytime)(void* pDriverCtrl);

    /**
     * @brief The function should enable the timer's interrupt.
     *
     * @details The function should enable the timer's interrupt.
     *
     * @param[in, out] pDriverCtrl The driver controler used by the registered
     * console driver.
     */
    void (*pEnable)(void* pDriverCtrl);

    /**
     * @brief The function should disable the timer's interrupt.
     *
     * @details The function should disable the timer's interrupt.
     *
     * @param[in, out] pDriverCtrl The driver controler used by the registered
     * console driver.
     */
    void (*pDisable)(void* pDriverCtrl);

    /**
     * @brief The function should set the timer's tick handler.
     *
     * @details The function should set the timer's tick handler. The handler
     * will be called at each tick received.
     *
     * @param[in, out] pDriverCtrl The driver controler used by the registered
     * console driver.
     * @param[in] handler The handler of the timer's interrupt.
     *
     * @return The success state or the error code.
     * - OS_NO_ERR is returned if no error is encountered.
     * - OS_ERR_NULL_POINTER is returned if the handler is NULL.
     * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the timer interrupt
     *   line is not allowed.
     * - OS_ERR_NULL_POINTER is returned if the pointer to the handler is NULL.
     * - OS_ERR_ALREADY_EXIST is returned if a handler is already
     *   registered for the timer.
     */
    OS_RETURN_E (*pSetHandler)(void* pDriverCtrl,
                               void(*handler)(kernel_thread_t*));

    /**
     * @brief The function should remove the timer tick handler.
     *
     * @details The function should remove the timer tick handler.
     *
     * @param[in, out] pDriverCtrl The driver controler used by the registered
     * console driver.
     *
     * @return The success state or the error code.
     * - OS_NO_ERR is returned if no error is encountered.
     * - OR_ERR_UNAUTHORIZED_INTERRUPT_LINE is returned if the timer interrupt
     *   line is not allowed.
     * - OS_ERR_INTERRUPT_NOT_REGISTERED is returned if the timer line has no
     *   handler attached.
     */
    OS_RETURN_E (*pRemoveHandler)(void* pDriverCtrl);

    /** @brief This function can be used to execute an operation in the
     * driver at every tick of the timer.
     *
     * @param[in, out] pDriverCtrl The driver controler used by the registered
     * console driver.
     *
     * @details This function can be used to execute an operation in the
     * driver at every tick of the timer such as a tick acknowledge.
     */
    void (*pTickManager)(void* pDriverCtrl);

    /**
     * @brief Contains a pointer to the driver controler, set by the driver
     * at the moment of the initialization of this structure.
     */
    void* pDriverCtrl;
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
 * @brief Initializes the time manager.
 *
 * @details Initializes the time manager. The timers will be added based on the
 * FDT configuration. This time manager should be initialized after all drivers
 * were attached.
 */
void timeInit(void);

/**
 * @brief Returns the current uptime.
 *
 * @details Return the current uptime of the system in ns.
 *
 * @return The current uptime in ns.
 */
uint64_t timeGetUptime(void);

/**
 * @brief Returns the current daytime from RTC.
 *
 * @details Return the current daytime from RTC.
 *
 * @return The current daytime from RTC.
 */
time_t timeGetDayTime(void);

/**
 * @brief Returns the current date from RTC.
 *
 * @details Return the current date from RTC.
 *
 * @return The current date from RTC.
 */
date_t timeGetDate(void);

/**
 * @brief Returns the number of CPU's ticks since the CPU started.
 *
 * @details Returns the number of CPU's ticks since the CPU started.
 *
 * @param[in] kCpuId The CPU identifer of which to get the number of ticks.
 *
 * @return The number of CPU's ticks since the CPU started.
 */
uint64_t timeGetTicks(const uint8_t kCpuId);

/**
 * @brief Performs a wait for ms nanoseconds.
 *
 * @details Performs a wait for ms nanoseconds based on the kernel's main
 * timer.
 *
 * @param[in] kNs The time to wait in nanoseconds.
 *
 * @warning This function must only be called before the scheduler is
 * running. Otherwise the function will have undefined behavior.
 */
void timeWaitNoScheduler(const uint64_t kNs);

#endif /* #ifndef __TIME_TIME_MGT_H_ */

/************************************ EOF *************************************/