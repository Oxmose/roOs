/*******************************************************************************
 * @file kmutex.c
 *
 * @see kmutex.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <panic.h>        /* Kernel panic */
#include <kfutex.h>       /* Kernel futex */
#include <kqueue.h>       /* Kernel queue */
#include <stdint.h>       /* Standard int types */
#include <stddef.h>       /* Standard definitions */
#include <atomic.h>       /* Kernel spinlock */
#include <stdbool.h>      /* Bool types */
#include <critical.h>     /* Kernel critical section */
#include <scheduler.h>    /* Kernel scheduler */

/* Configuration files */
#include <config.h>

/* Header file */
#include <kmutex.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "KMUTEX"

/** @brief Defines the maximum recursiveness level of a mutex */
#define MUTEX_MAX_RECURSIVENESS (UINT32_MAX)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the mutex to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the mutex to ensure correctness of
 * execution. Due to the critical nature of the mutex, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define MUTEX_ASSERT(COND, MSG, ERROR) {                    \
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
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
OS_RETURN_E kmutexInit(kmutex_t* pMutex, const uint32_t kFlags)
{
    /* Check parameters */
    if(pMutex == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if((kFlags & KMUTEX_FLAG_QUEUING_FIFO) == KMUTEX_FLAG_QUEUING_FIFO &&
       (kFlags & KMUTEX_FLAG_QUEUING_PRIO) == KMUTEX_FLAG_QUEUING_PRIO)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    if((kFlags & KMUTEX_FLAG_PRIO_ELEVATION) == KMUTEX_FLAG_PRIO_ELEVATION &&
       (kFlags & KMUTEX_FLAG_QUEUING_PRIO) != KMUTEX_FLAG_QUEUING_PRIO)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Setup the mutex */
    pMutex->flags = kFlags;
    pMutex->lockState = 1;
    pMutex->recLevel = 0;
    pMutex->nbWaitingThreads = 0;
    KERNEL_SPINLOCK_INIT(pMutex->lock);

    /* Setup the futex */
    pMutex->futex.pHandle = &pMutex->lockState;
    pMutex->futex.isAlive = true;

    if((kFlags & KMUTEX_FLAG_QUEUING_PRIO) == KMUTEX_FLAG_QUEUING_PRIO)
    {
        pMutex->futex.queuingDiscipline = KFUTEX_FLAG_QUEUING_PRIO;
    }
    else
    {
        pMutex->futex.queuingDiscipline = KFUTEX_FLAG_QUEUING_FIFO;
    }

    pMutex->isInit = true;
    return OS_NO_ERR;
}

OS_RETURN_E kmutexDestroy(kmutex_t* pMutex)
{
    /* Check parameters */
    if(pMutex == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(pMutex->isInit == false)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Clear the mutex and wakeup all threads */
    KERNEL_LOCK(pMutex->lock);

    pMutex->isInit = false;

    /* Release all waiting threads */
    pMutex->lockState = 1;
    pMutex->recLevel = 0;
    pMutex->nbWaitingThreads = 0;
    kfutexWake(&pMutex->futex, KFUTEX_MAX_WAIT_COUNT);
    pMutex->futex.isAlive = false;

    KERNEL_UNLOCK(pMutex->lock);

    return OS_NO_ERR;
}

OS_RETURN_E kmutexLock(kmutex_t* pMutex)
{
    OS_RETURN_E          error;
    uint32_t             intState;
    kernel_thread_t*     pCurThread;
    KFUTEX_WAKE_REASON_E wakeReason;

    /* Check parameters */
    if(pMutex == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pMutex->lock);

    if(pMutex->isInit == false)
    {
        KERNEL_UNLOCK(pMutex->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        return OS_ERR_INCORRECT_VALUE;
    }

    pCurThread = schedGetCurrentThread();

    if(pMutex->lockState > 0)
    {
        /* Acquire one value */
        pMutex->lockState = 0;
        pMutex->pAcquiredThread = pCurThread;
        pMutex->acquiredThreadPriority = pCurThread->priority;
        KERNEL_UNLOCK(pMutex->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);

        return OS_NO_ERR;
    }
    else if((pMutex->flags & KMUTEX_FLAG_RECURSIVE) == KMUTEX_FLAG_RECURSIVE &&
            pCurThread == pMutex->pAcquiredThread)
    {
        /* If the mutex is recursive, allow the lock */
        if(pMutex->recLevel < MUTEX_MAX_RECURSIVENESS)
        {
            ++pMutex->recLevel;
            error = OS_NO_ERR;
        }
        else
        {
            error = OS_ERR_OUT_OF_BOUND;
        }

        KERNEL_UNLOCK(pMutex->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        return error;
    }

    /* If priority elevation is enabled, elevate if needed */
    if((pMutex->flags & KMUTEX_FLAG_PRIO_ELEVATION) ==
       KMUTEX_FLAG_PRIO_ELEVATION
       &&
       pMutex->pAcquiredThread->priority > pCurThread->priority)
    {
        error = schedUpdatePriority(pMutex->pAcquiredThread,
                                    pCurThread->priority);
        if(error != OS_NO_ERR)
        {
            KERNEL_UNLOCK(pMutex->lock);
            KERNEL_EXIT_CRITICAL_LOCAL(intState);
            return error;
        }
    }

    /* Wait on the futex an re-wait if canceled */
    ++pMutex->nbWaitingThreads;
    do
    {
        KERNEL_UNLOCK(pMutex->lock);
        error = kfutexWait(&pMutex->futex, 0, &wakeReason);
        KERNEL_LOCK(pMutex->lock);

        /* In the case the futex was woken up during the wait */
        if(error == OS_ERR_NOT_BLOCKED)
        {
            /* Check the lock state */
            if(pMutex->lockState == 1 && pMutex->nbWaitingThreads <= 1)
            {
                /* Get the semaphore */
                error = OS_NO_ERR;
                wakeReason = KFUTEX_WAKEUP_WAKE;
            }
            else
            {
                /* Go for another round */
                error = OS_ERR_CANCELED;
                wakeReason = KFUTEX_WAKEUP_CANCEL;
            }
        }

    } while(error == OS_ERR_CANCELED &&
            wakeReason == KFUTEX_WAKEUP_CANCEL);

    /* Lock the mutex */
    if(wakeReason == KFUTEX_WAKEUP_WAKE && pMutex->isInit == true)
    {
        /* Acquire one value */
        pMutex->lockState = 0;
        pMutex->pAcquiredThread = pCurThread;
        pMutex->acquiredThreadPriority = pCurThread->priority;
        --pMutex->nbWaitingThreads;
        KERNEL_UNLOCK(pMutex->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);

        return OS_NO_ERR;
    }
    else
    {
        KERNEL_UNLOCK(pMutex->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        return OS_ERR_DESTROYED;
    }
}

OS_RETURN_E kmutexUnlock(kmutex_t* pMutex)
{
    uint32_t         intState;
    kernel_thread_t* pCurThread;
    OS_RETURN_E      error;
    bool             schedule;

    /* Check parameters */
    if(pMutex == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pMutex->lock);


    if(pMutex->isInit == false)
    {
        KERNEL_UNLOCK(pMutex->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        return OS_ERR_INCORRECT_VALUE;
    }

    pCurThread = schedGetCurrentThread();
    /* Only the owner can unlock the mutex */
    if(pCurThread != pMutex->pAcquiredThread)
    {
        KERNEL_UNLOCK(pMutex->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    if((pMutex->flags & KMUTEX_FLAG_RECURSIVE) != KMUTEX_FLAG_RECURSIVE ||
        pMutex->recLevel == 0)
    {
        schedule = false;
        pMutex->pAcquiredThread = NULL;

        /* If elevation was made, set back our original priority */
        if((pMutex->flags & KMUTEX_FLAG_PRIO_ELEVATION) ==
           KMUTEX_FLAG_PRIO_ELEVATION &&
           pMutex->acquiredThreadPriority > pCurThread->priority)
        {
            schedule = true;
            error = schedUpdatePriority(pCurThread,
                                        pMutex->acquiredThreadPriority);
            MUTEX_ASSERT(error == OS_NO_ERR,
                         "Failed to change thread priority",
                         error);
        }

        /* Release one thread */
        pMutex->lockState = 1;
        error = kfutexWake(&pMutex->futex, 1);

        if(error == OS_NO_ERR)
        {
            /* We wokeup a thread, set the mutex as locked */
            pMutex->lockState = 0;
        }
        else if(error == OS_ERR_NO_SUCH_ID)
        {
            /* Nothing to wakeup, its ok */
            error = OS_NO_ERR;
        }

        MUTEX_ASSERT(error == OS_NO_ERR,
                     "Error while releasing mutex\n",
                     error);
        KERNEL_UNLOCK(pMutex->lock);

        if(schedule == true)
        {
            schedSchedule();
        }
    }
    else
    {
        --pMutex->recLevel;
        KERNEL_UNLOCK(pMutex->lock);
    }
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    return OS_NO_ERR;
}

OS_RETURN_E kmutexTryLock(kmutex_t* pMutex, int32_t* pLockState)
{
    OS_RETURN_E      error;
    kernel_thread_t* pCurThread;

    /* Check parameters */
    if(pMutex == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(pMutex->isInit == false)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    KERNEL_LOCK(pMutex->lock);

    if(pLockState != NULL)
    {
        *pLockState = pMutex->lockState;
    }

    if(pMutex->lockState > 0)
    {
        pCurThread = schedGetCurrentThread();

        /* Acquire one value */
        pMutex->lockState = 0;
        pMutex->pAcquiredThread = pCurThread;
        pMutex->acquiredThreadPriority = pCurThread->priority;
        error = OS_NO_ERR;
    }
    else
    {
        error = OS_ERR_BLOCKED;
    }

    KERNEL_UNLOCK(pMutex->lock);

    return error;
}

/************************************ EOF *************************************/