/*******************************************************************************
 * @file signal.h
 *
 * @see signal.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 03/07/2024
 *
 * @version 1.0
 *
 * @brief Kernel thread signaling manager.
 *
 * @details Kernel thread signaling manager. Signal are used to communicate
 * between threads. A signal is handled the next time the thread is scheduled.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_SIGNAL_H_
#define __CORE_SIGNAL_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h>     /* Generic int types */
#include <stddef.h>     /* Standard definition type */
#include <kerror.h>     /* Kernel errors */
#include <ctrl_block.h> /* Thread control block */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the thread signals */
typedef enum
{
    /** @brief Signal: floating illegal instruction exception. */
    THREAD_SIGNAL_ILL = 4,
    /** @brief Signal: floating point exception. */
    THREAD_SIGNAL_FPE = 8,
    /** @brief Signal: terminates the thread. */
    THREAD_SIGNAL_KILL = 9,
    /** @brief Signal: user defined signal. */
    THREAD_SIGNAL_USR1 = 10,
    /** @brief Signal: segfault occured. */
    THREAD_SIGNAL_SEGV = 11,
    /** @brief Signal: user defined signal. */
    THREAD_SIGNAL_USR2 = 12,
    /** @brief Signal: unknown exception occured */
    THREAD_SIGNAL_EXC  = 16,

    THREAD_SIGNAL_MAX_VALUE
} THREAD_SIGNAL_E;

#if (THREAD_MAX_SIGNALS < THREAD_SIGNAL_MAX_VALUE)
#error "To many signals defined"
#endif

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
 * @brief Initializes the signals for a thread.
 *
 * @brief Initializes the signals for a thread. The signal table is setup by
 * default handlers and the signal mask reset.
 *
 * @param[in] pThread The thread to initialize.
 */
void signalInitSignals(kernel_thread_t* pThread);


/**
 * @brief Registers a new signal handler for the current thread.
 *
 * @details Registers a new signal handler for the current thread. The handler
 * is added to the thread signal handers table and is effective immediately.
 *
 * @param[in] kSignal The signal to register the handler for.
 * @param[in] pHandler The handler to register.
 *
 * @return The function returns the error or success status.
 */
OS_RETURN_E signalRegister(const THREAD_SIGNAL_E kSignal,
                           void                  (*pHandler)(void));

/**
 * @brief Signals a thread.
 *
 * @details Signals a thread. The next time the thread is scheduled, the
 * execution flow will be redirected to the signal handler.
 *
 * @param[in] pThread The thread to signal.
 * @param[in] kSignal The signal to send.
 *
 * @return The function returnsthe error or success status.
 */
OS_RETURN_E signalThread(kernel_thread_t*      pThread,
                         const THREAD_SIGNAL_E kSignal);


/**
 * @brief Manages a thread signals.
 *
 * @details Manages a thread signals. Will get the last signal sent to the
 * thread and apply the according action.
 *
 * @param[in] pThread The thread to manage.
 *
 * @warning This function shall only be called in a interrupt context as it
 * changes the virtual CPU state saved at interrupts boundaries. The thread
 * lock should be already taken.
 */
void signalManage(kernel_thread_t* pThread);

#endif /* #ifndef __CORE_SIGNAL_H_ */

/************************************ EOF *************************************/