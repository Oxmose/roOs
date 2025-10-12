/*******************************************************************************
 * @file kfutex.h
 *
 * @see kfutex.c
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

#ifndef __SYNC_KFUTEX_H_
#define __SYNC_KFUTEX_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h>  /* Standard int definitions */
#include <kerror.h>  /* Kernel error */
#include <kqueue.h>  /* Kernel queues */
#include <stdbool.h> /* Bool types */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the maximal number of waiting threads on a futex */
#define KFUTEX_MAX_WAIT_COUNT 4096

/** @brief Futex flag: futex has FIFO queuing discipline. */
#define KFUTEX_FLAG_QUEUING_FIFO 0x00000001

/** @brief Futex flag: futex has priority based queuing discipline. */
#define KFUTEX_FLAG_QUEUING_PRIO 0x00000002

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the wakeup reason of a futex. */
typedef enum
{
    /** @brief The thread was woken up because wake was called */
    KFUTEX_WAKEUP_WAKE = 0,

    /** @brief The thread was woken up because the futex was destroyed */
    KFUTEX_WAKEUP_DESTROYED = 1,

    /** @brief The futex was canceled. */
    KFUTEX_WAKEUP_CANCEL = 2,
} KFUTEX_WAKE_REASON_E;

/** @brief Futex structure definition. */
typedef struct
{
    /** @brief Futex atomic handle */
    volatile int32_t* pHandle;

    /** @brief Futex alive state */
    volatile bool isAlive;

        /** @brief Waiting queue discipline */
    uint32_t queuingDiscipline;

    /** @brief Number of waiting threads */
    uint32_t nbWaitingThreads;
} kfutex_t;

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
void kfutexLibInit(void);

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
OS_RETURN_E kfutexWait(kfutex_t*             pFutex,
                       const int32_t         kWaitValue,
                       KFUTEX_WAKE_REASON_E* pWakeReason);

/**
 * @brief Wakes a given futex.
 *
 * @details Wakes a given futex. This function receives the futex to wake and
 * the value to observe as parameters. Waking a futex will release kWakeCount
 * threads waiting on the futex if any exits. The waiting threads will be
 * re-scheduled. The wait success or error state is returned.
 *
 * @param[in, out] pFutex The futex object to wait.
 * @param[in] pWakeCount The nunber of thread to wake. This value is updated
 * with the actual number of thread that were woken up.
 *
 * @return The wake success or error state is returned.
 *
 * @warning This function is asynchronous, if a thread starts to wait just after
 * this function is called and no other thread was waiting on the futex, it will
 * wait until the next call to futexWake.
 * Waking on a destroyed futex produces undefined behavior.
 */
OS_RETURN_E kfutexWake(kfutex_t* pFutex, size_t* pWakeCount);

#endif /* #ifndef __SYNC_KFUTEX_H_ */

/************************************ EOF *************************************/