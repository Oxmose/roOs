/*******************************************************************************
 * @file futex.h
 *
 * @see futex.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 20/06/2024
 *
 * @version 2.0
 *
 * @brief Kernel's futex API.
 *
 * @details Kernel's futex API. This module implements the futex management.
 * Futexes are used as a synchronization primitive and are the base block for
 * more advanced synchronization methods such as mutexes or semaphores.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __SYNC_FUTEX_H_
#define __SYNC_FUTEX_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h> /* Standard int definitions */
#include <kerror.h> /* Kernel error */
#include <kqueue.h> /* Kernel queues */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the wakeup reason of a futex. */
typedef enum
{
    /** @brief The thread was woken up because wake was called */
    FUTEX_WAKEUP_WAKE = 0,

    /** @brief The thread was woken up because the futex was destroyed */
    FUTEX_WAKEUP_DESTROYED = 1,

    /** @brief The futex was canceled. */
    FUTEX_WAKEUP_CANCEL = 2,
} FUTEX_WAKE_REASON_E;

/** @brief Futex structure definition. */
typedef struct
{
    /** @brief Futex atomic handle */
    volatile uint32_t* pHandle;

    /** @brief Futex alive state */
    volatile bool_t isAlive;
} futex_t;

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
 * @brief Initializes the futex library.
 *
 * @details Initializes the futex library. If the function was not able to
 * alocate the necessary resources, a kernel panic is generated.
 */
void futexLibInit(void);

/**
 * @brief Waits on a given futex.
 *
 * @details Waits on a given futex. This function receives the futex to wait and
 * the value to observe as parameters. Waiting on a locked futex will
 * un-schedule the calling thread. The wait success or error state is returned.
 *
 * @param[in, out] pFutex The futex object to wait.
 * @param[in] kWaitValue The value to observe when waiting the futex.
 * @param[out] pWakeReason The reason why the futex was awaken.
 *
 * @return The wait success or error state is returned.
 *
 * @warning This function is asynchronous, if a thread starts to wait just after
 * futexWake is called and no other thread was waiting on the futex, it will
 * wait until the next call to futexWake.
 * Waiting on a destroyed futex produces undefined behavior.
 */
OS_RETURN_E futexWait(futex_t*             pFutex,
                      const uint32_t       kWaitValue,
                      FUTEX_WAKE_REASON_E* pWakeReason);

/**
 * @brief Wakes a given futex.
 *
 * @details Wakes a given futex. This function receives the futex to wake and
 * the value to observe as parameters. Waking a futex will release kWakeCount
 * threads waiting on the futex if any exits. The waiting threads will be
 * re-scheduled. The wait success or error state is returned.
 *
 * @param[in, out] pFutex The futex object to wait.
 * @param[in] kWakeCount The nunber of thread to wake.
 *
 * @return The wake success or error state is returned.
 *
 * @warning This function is asynchronous, if a thread starts to wait just after
 * this function is called and no other thread was waiting on the futex, it will
 * wait until the next call to futexWake.
 * Waking on a destroyed futex produces undefined behavior.
 */
OS_RETURN_E futexWake(futex_t* pFutex, const uintptr_t kWakeCount);

#endif /* #ifndef __SYNC_FUTEX_H_ */

/************************************ EOF *************************************/