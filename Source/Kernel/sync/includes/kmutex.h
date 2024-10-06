/*******************************************************************************
 * @file kmutex.h
 *
 * @see kmutex.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/06/2024
 *
 * @version 4.0
 *
 * @brief Kernel mutex synchronization primitive.
 *
 * @details Kernel mutex synchronization primitive implementation. Avoids
 * priority inversion by allowing the user to set a priority to the mutex, then
 * all threads that acquire this mutex will see their priority elevated to the
 * mutex's priority level.
 *
 * @warning Mutex can only be used when the current system is running and the
 * scheduler initialized.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __SYNC_KMUTEX_H_
#define __SYNC_KMUTEX_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <kqueue.h>   /* Kernel queues */
#include <kfutex.h>   /* Kernel futex */
#include <stdint.h>   /* Standard int definitions */
#include <kerror.h>   /* Kernel error */
#include <atomic.h>   /* Spinlock structure */
#include <stdbool.h>  /* Bool types */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Mutex flag: mutex has FIFO queuing discipline. */
#define KMUTEX_FLAG_QUEUING_FIFO 0x00000001

/** @brief Mutex flag: mutex has priority based queuing discipline. */
#define KMUTEX_FLAG_QUEUING_PRIO 0x00000002

/** @brief Mutex flag: recursive mutex. */
#define KMUTEX_FLAG_RECURSIVE 0x00000004

/** @brief Mutex flag: priority elevation mutex. Must be paired with
 * KMUTEX_FLAG_QUEUING_PRIO for proper work.
 */
#define KMUTEX_FLAG_PRIO_ELEVATION 0x00000008

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Mutex structure definition. */
typedef struct
{

    /** @brief Mutex associated futex */
    kfutex_t futex;

    /** @brief Mutex flags */
    uint32_t flags;

    /** @brief Mutex lock. */
    kernel_spinlock_t lock;

    /** @brief Acquired thread pointer */
    kernel_thread_t* pAcquiredThread;

    /** @brief Acquired thread's initial priority */
    uint8_t acquiredThreadPriority;

    /** @brief Initialization state */
    volatile bool isInit;

    /** @brief Mutex lock state */
    volatile int32_t lockState;

    /** @brief Mutex recursive level */
    volatile uint32_t recLevel;

    /* @brief Number of waiting threads */
    volatile uint32_t nbWaitingThreads;
} kmutex_t;

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
OS_RETURN_E kmutexInit(kmutex_t* pMutex, const uint32_t kFlags);

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
OS_RETURN_E kmutexDestroy(kmutex_t* pMutex);

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
OS_RETURN_E kmutexLock(kmutex_t* pMutex);

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
OS_RETURN_E kmutexUnlock(kmutex_t* pMutex);

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
OS_RETURN_E kmutexTryLock(kmutex_t* pMutex, int32_t* pLockState);

#endif /* #ifndef __SYNC_KMUTEX_H_ */

/************************************ EOF *************************************/