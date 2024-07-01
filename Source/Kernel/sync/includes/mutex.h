/*******************************************************************************
 * @file mutex.h
 *
 * @see mutex.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/06/2024
 *
 * @version 4.0
 *
 * @brief Mutex synchronization primitive.
 *
 * @details Mutex synchronization primitive implementation. Avoids priority
 * inversion by allowing the user to set a priority to the mutex, then all
 * threads that acquire this mutex will see their priority elevated to the
 * mutex's priority level.
 *
 * @warning Mutex can only be used when the current system is running and the
 * scheduler initialized.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __SYNC_MUTEX_H_
#define __SYNC_MUTEX_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <kqueue.h>   /* Kernel queues */
#include <stdint.h>   /* Standard int definitions */
#include <kerror.h>   /* Kernel error */
#include <atomic.h>   /* Spinlock structure */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Mutex flag: mutex has FIFO queuing discipline. */
#define MUTEX_FLAG_QUEUING_FIFO 0x00000001

/** @brief Mutex flag: mutex has priority based queuing discipline. */
#define MUTEX_FLAG_QUEUING_PRIO 0x00000002

/** @brief Mutex flag: recursive mutex. */
#define MUTEX_FLAG_RECURSIVE 0x00000004

/** @brief Mutex flag: priority elevation mutex. */
#define MUTEX_FLAG_PRIO_ELEVATION 0x00000008

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Mutex structure definition. */
typedef struct
{
    /** @brief Mutex lock state */
    volatile int32_t lockState;

    /** @brief Mutex waiting list */
    kqueue_t* pWaitingList;

    /** @brief Initialization state */
    bool_t isInit;

    /** @brief Mutex flags */
    uint32_t flags;

    /** @brief Mutex lock. */
    spinlock_t lock;

    /** @brief Acquired thread's initial priority */
    uint8_t acquiredThreadPriority;

    /** @brief Acquired thread pointer */
    kernel_thread_t* pAcquiredThread;
} mutex_t;

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
 * @brief Initializes the mutex structure.
 *
 * @details Initializes the mutex structure. The initial state of a
 * mutex is available.
 *
 * @param[out] pMutex The pointer to the mutex to initialize.
 * @param[in] kFlags The mutex flags to be used.
 *
 * @return The success state or the error code.
 */
OS_RETURN_E mutexInit(mutex_t* pMutex, const uint32_t kFlags);

/**
 * @brief Destroys the mutex given as parameter.
 *
 * @details Destroys the mutex given as parameter. Also unlock all the
 * threads locked on this mutex. Using a destroyed mutex produces
 * undefined behaviors.
 *
 * @param[in, out] pMutex The mutex to destroy.
 *
 * @return The success state or the error code.
 *
 * @warning Using a non-initialized or destroyed mutex produces undefined
 * behavior.
 */
OS_RETURN_E mutexDestroy(mutex_t* pMutex);

/**
 * @brief Locks on the mutex given as parameter.
 *
 * @details Locks on the mutex given as parameter. The calling thread will
 * block on this call until the mutex is aquired.
 *
 * @param[in] pMutex The mutex to lock.
 *
 * @return The success state or the error code.
 *
 * @warning Using a non-initialized or destroyed mutex produces undefined
 * behavior.
 */
OS_RETURN_E mutexLock(mutex_t* pMutex);

/**
 * @brief Unlocks the mutex given as parameter.
 *
 * @param[in] pMutex The mutex to unlock.
 *
 * @return The success state or the error code.
 *
 * @warning Only the mutex thread owner can unlock a mutex. Using a
 * non-initialized or destroyed mutex produces undefined behavior.
 */
OS_RETURN_E mutexUnlock(mutex_t* pMutex);

/**
 * @brief Try to lock on the mutex given as parameter.
 *
 * @details Try to lock on the mutex mutex as parameter. The function will
 * return the current mutex lock state. If possible the function will
 * aquire the mutex.
 *
 * @param[in] pMutex The mutex to lock.
 * @param[out] pLockState The buffer that receives the mutex lock state.
 *
 * @return The success state or the error code.
 *
 * @warning Using a non-initialized or destroyed mutex produces undefined
 * behavior.
 */
OS_RETURN_E mutexTryLock(mutex_t* pMutex, int32_t* pLockState);

#endif /* #ifndef __SYNC_MUTEX_H_ */

/************************************ EOF *************************************/