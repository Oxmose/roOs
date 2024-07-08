/*******************************************************************************
 * @file mutex.c
 *
 * @see mutex.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <panic.h>        /* Kernel panic */
#include <kqueue.h>       /* Kernel queue */
#include <stdint.h>       /* Standard int types */
#include <stddef.h>       /* Standard definitions */
#include <atomic.h>       /* Kernel spinlock */
#include <critical.h>     /* Kernel critical section */
#include <scheduler.h>    /* Kernel scheduler */

/* Configuration files */
#include <config.h>

/* Header file */
#include <mutex.h>

/* Unit test header */
#include <test_framework.h>

/* Tracing feature */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "MUTEX"

/** @brief Defines the maximum recursiveness level of a mutex */
#define MUTEX_MAX_RECURSIVENESS ((int32_t)INT32_MIN)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the state of a mutex when awakening */
typedef enum
{
    /** @brief The mutex was posted */
    MUTEX_UNLOCKED = 0,
    /** @brief The mutex was destroyed */
    MUTEX_DESTROYED = 1
} MUTEX_WAIT_STATUS;

/** @brief Defines the thread's private mutex data */
typedef struct
{
    /** @brief The thread pointer */
    kernel_thread_t* pThread;

    /** @brief The mutex wait status */
    MUTEX_WAIT_STATUS status;

    /** @brief The mutex associated to the data */
    mutex_t* pMutex;
} mutex_data_t;

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
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Releases a mutex resource used by a thread.
 *
 * @details Releases a mutex resource used by a thread. This prevents memory
 * and resource leaks when killing a thread.
 *
 * @param[in] pResource The resource to release.
 */
static void _mutexReleaseResource(void* pResource);

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
static void _mutexReleaseResource(void* pResource)
{
    mutex_data_t*  pData;
    kqueue_node_t* pNode;
    uint32_t       intState;

    KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                       TRACE_MUTEX_RELEASE_RESOURCE_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pResource),
                       KERNEL_TRACE_LOW(pResource));

    MUTEX_ASSERT(pResource != NULL, "NULL Mutex resource", OS_ERR_NULL_POINTER);

    /* The inly resource we manage are waiting thread's mutex node */
    pNode = pResource;
    pData = pNode->pData;

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pData->pMutex->lock);

    /* Check if the node is enlisted */
    if(pNode->pQueuePtr != NULL)
    {
        kQueueRemove(pData->pMutex->pWaitingList, pNode, TRUE);
    }

    KERNEL_LOCK(pData->pMutex->lock);
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                       TRACE_MUTEX_RELEASE_RESOURCE_EXIT,
                       2,
                       KERNEL_TRACE_HIGH(pResource),
                       KERNEL_TRACE_LOW(pResource));
}

OS_RETURN_E mutexInit(mutex_t* pMutex, const uint32_t kFlags)
{
    KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                       TRACE_MUTEX_INIT_ENTRY,
                       3,
                       KERNEL_TRACE_HIGH(pMutex),
                       KERNEL_TRACE_LOW(pMutex),
                       kFlags);

    /* Check parameters */
    if(pMutex == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_INIT_EXIT,
                           4,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           kFlags,
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    if((kFlags & MUTEX_FLAG_QUEUING_FIFO) == MUTEX_FLAG_QUEUING_FIFO &&
       (kFlags & MUTEX_FLAG_QUEUING_PRIO) == MUTEX_FLAG_QUEUING_PRIO)
    {
        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_INIT_EXIT,
                           4,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           kFlags,
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Setup the mutex */
    pMutex->pWaitingList = kQueueCreate(FALSE);
    if(pMutex->pWaitingList == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_INIT_EXIT,
                           4,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           kFlags,
                           OS_ERR_NO_MORE_MEMORY);
        return OS_ERR_NO_MORE_MEMORY;
    }

    pMutex->flags = kFlags;
    pMutex->lockState = 1;
    SPINLOCK_INIT(pMutex->lock);
    pMutex->isInit = TRUE;

    KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                       TRACE_MUTEX_INIT_EXIT,
                       4,
                       KERNEL_TRACE_HIGH(pMutex),
                       KERNEL_TRACE_LOW(pMutex),
                       kFlags,
                       OS_NO_ERR);

    return OS_NO_ERR;
}

OS_RETURN_E mutexDestroy(mutex_t* pMutex)
{
    uint32_t       intState;
    kqueue_node_t* pWaitNode;
    mutex_data_t*  pData;

    KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                       TRACE_MUTEX_DESTROY_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pMutex),
                       KERNEL_TRACE_LOW(pMutex));

    /* Check parameters */
    if(pMutex == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_DESTROY_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    if(pMutex->isInit == FALSE || pMutex->pWaitingList == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_DESTROY_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Clear the mutex and wakeup all threads */
    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pMutex->lock);

    pMutex->isInit = FALSE;

    /* Release all waiting threads */
    pWaitNode = kQueuePop(pMutex->pWaitingList);
    while(pWaitNode != NULL)
    {
        pData = pWaitNode->pData;
        pData->status = MUTEX_DESTROYED;

        schedReleaseThread(pData->pThread, FALSE, THREAD_STATE_READY, FALSE);
        pWaitNode = kQueuePop(pMutex->pWaitingList);
    }

    kQueueDestroy(&pMutex->pWaitingList);

    KERNEL_UNLOCK(pMutex->lock);

    /* Schedule in case more prioritary thread were scheduled */
    schedSchedule();

    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                       TRACE_MUTEX_DESTROY_EXIT,
                       3,
                       KERNEL_TRACE_HIGH(pMutex),
                       KERNEL_TRACE_LOW(pMutex),
                       OS_NO_ERR);

    return OS_NO_ERR;
}

OS_RETURN_E mutexLock(mutex_t* pMutex)
{
    OS_RETURN_E       error;
    uint32_t          intState;
    kqueue_node_t     mutexNode;
    kernel_thread_t*  pCurThread;
    mutex_data_t      data;
    thread_resource_t threadRes;
    void*             pResourceHandle;

    KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                       TRACE_MUTEX_LOCK_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pMutex),
                       KERNEL_TRACE_LOW(pMutex));

    /* Check parameters */
    if(pMutex == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_LOCK_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    if(pMutex->isInit == FALSE || pMutex->pWaitingList == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_LOCK_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    pCurThread = schedGetCurrentThread();

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pMutex->lock);

    if(pMutex->lockState > 0)
    {
        /* Acquire one value */
        pMutex->lockState = 0;
        pMutex->pAcquiredThread = pCurThread;
        pMutex->acquiredThreadPriority = pCurThread->priority;
        KERNEL_UNLOCK(pMutex->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);

        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_LOCK_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           OS_NO_ERR);

        return OS_NO_ERR;
    }
    else if((pMutex->flags & MUTEX_FLAG_RECURSIVE) == MUTEX_FLAG_RECURSIVE &&
            pCurThread == pMutex->pAcquiredThread)
    {
        /* If the mutex is recursive, allow the lock */
        if(pMutex->lockState > MUTEX_MAX_RECURSIVENESS)
        {
            --pMutex->lockState;
            error = OS_NO_ERR;
        }
        else
        {
            error = OS_ERR_OUT_OF_BOUND;
        }

        KERNEL_UNLOCK(pMutex->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);

        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_LOCK_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           error);
        return error;
    }

    /* Create a new queue node */
    data.pThread = pCurThread;
    data.pMutex  = pMutex;
    data.status  = MUTEX_DESTROYED;
    kQueueInitNode(&mutexNode, &data);

    /* Add the node to the queue, if no flags are defined for the queuing
     * discipline, default to FIFO.
     */
    if((pMutex->flags & MUTEX_FLAG_QUEUING_PRIO) == MUTEX_FLAG_QUEUING_PRIO)
    {
        kQueuePushPrio(&mutexNode, pMutex->pWaitingList, pCurThread->priority);
    }
    else
    {
        kQueuePush(&mutexNode, pMutex->pWaitingList);
    }

    /* Add the resource to the thread so it can be removed on kill */
    threadRes.pReleaseResource = _mutexReleaseResource;
    threadRes.pResourceData    = &mutexNode;
    pResourceHandle = schedThreadAddResource(&threadRes);
    if(pResourceHandle == NULL)
    {
        kQueueRemove(pMutex->pWaitingList, &mutexNode, TRUE);
        KERNEL_UNLOCK(pMutex->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);

        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_LOCK_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           OS_ERR_INCORRECT_VALUE);

        return OS_ERR_INCORRECT_VALUE;
    }

    /* Set the thread as waiting */
    schedWaitThreadOnResource(THREAD_WAIT_RESOURCE_MUTEX);

    /* If priority elevation is enabled, elevate if needed */
    if((pMutex->flags & MUTEX_FLAG_PRIO_ELEVATION) == MUTEX_FLAG_PRIO_ELEVATION
       &&
       pMutex->pAcquiredThread->priority > pCurThread->priority)
    {
        schedUpdatePriority(pMutex->pAcquiredThread, pCurThread->priority);
    }

    /* Release the mutex lock */
    KERNEL_UNLOCK(pMutex->lock);

    /* Schedule */
    schedSchedule();
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    /* Ensure the node was released */
    MUTEX_ASSERT(mutexNode.pQueuePtr == NULL,
                 "Failed to delist mutex node",
                 OS_ERR_UNAUTHORIZED_ACTION);

    /* Release the resource */
    error = schedThreadRemoveResource(pResourceHandle);
    MUTEX_ASSERT(error == OS_NO_ERR, "Failed to remove mutex resource", error);

    /* We are back from scheduling, check if the mutex is still alive */
    if(data.status == MUTEX_DESTROYED)
    {
        error = OS_ERR_DESTROYED;
    }
    else
    {
        error = OS_NO_ERR;
    }

    KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                       TRACE_MUTEX_LOCK_EXIT,
                       3,
                       KERNEL_TRACE_HIGH(pMutex),
                       KERNEL_TRACE_LOW(pMutex),
                       error);

    return error;

}

OS_RETURN_E mutexUnlock(mutex_t* pMutex)
{
    kqueue_node_t*   pWaitNode;
    mutex_data_t*    pData;
    mutex_data_t*    pCursorData;
    uint32_t         intState;
    kernel_thread_t* pCurThread;
    uint8_t          highPrio;

    KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                       TRACE_MUTEX_UNLOCK_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pMutex),
                       KERNEL_TRACE_LOW(pMutex));

    /* Check parameters */
    if(pMutex == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_UNLOCK_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    if(pMutex->isInit == FALSE || pMutex->pWaitingList == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_UNLOCK_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    pCurThread = schedGetCurrentThread();

    /* Clear the mutex and wakeup waiting thread */
    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pMutex->lock);

    /* Only the owner can unlock the mutex */
    if(pCurThread != pMutex->pAcquiredThread)
    {
        KERNEL_UNLOCK(pMutex->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);

        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_UNLOCK_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           OS_ERR_UNAUTHORIZED_ACTION);
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    if((pMutex->flags & MUTEX_FLAG_RECURSIVE) != MUTEX_FLAG_RECURSIVE ||
        pMutex->lockState == 0)
    {
        /* If elevation was made, set back our original priority */
        if((pMutex->flags & MUTEX_FLAG_PRIO_ELEVATION) ==
           MUTEX_FLAG_PRIO_ELEVATION &&
           pMutex->acquiredThreadPriority > pCurThread->priority)
        {
            schedUpdatePriority(pCurThread, pMutex->acquiredThreadPriority);
        }

        /* Release one thread */
        pWaitNode = kQueuePop(pMutex->pWaitingList);
        if(pWaitNode != NULL)
        {
            pData = pWaitNode->pData;
            pData->status = MUTEX_UNLOCKED;
            pMutex->acquiredThreadPriority = pData->pThread->priority;
            pMutex->pAcquiredThread = pData->pThread;

            /* Check of priority inheritance is required */
            if((pMutex->flags & MUTEX_FLAG_PRIO_ELEVATION) ==
               MUTEX_FLAG_PRIO_ELEVATION)
            {
                highPrio = pData->pThread->priority;
                /* Find the highest priority and if needed elevate the next
                 * thread
                 */
                pWaitNode = pMutex->pWaitingList->pHead;
                while(pWaitNode != NULL)
                {
                    pCursorData = pWaitNode->pData;
                    if(highPrio > pCursorData->pThread->priority)
                    {
                        highPrio = pCursorData->pThread->priority;
                    }

                    pWaitNode = pWaitNode->pNext;
                }

                /* Elevate if needed */
                if(highPrio < pData->pThread->priority)
                {
                    schedUpdatePriority(pData->pThread, highPrio);
                }
            }
            KERNEL_UNLOCK(pMutex->lock);
            schedReleaseThread(pData->pThread, FALSE, THREAD_STATE_READY, TRUE);
        }
        else
        {
            pMutex->lockState = 1;
            KERNEL_UNLOCK(pMutex->lock);
        }
    }
    else
    {
        ++pMutex->lockState;
        KERNEL_UNLOCK(pMutex->lock);
    }
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                       TRACE_MUTEX_UNLOCK_EXIT,
                       3,
                       KERNEL_TRACE_HIGH(pMutex),
                       KERNEL_TRACE_LOW(pMutex),
                       OS_NO_ERR);

    return OS_NO_ERR;
}

OS_RETURN_E mutexTryLock(mutex_t* pMutex, int32_t* pLockState)
{
    OS_RETURN_E      error;
    uint32_t         intState;
    kernel_thread_t* pCurThread;

    KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                       TRACE_MUTEX_TRYLOCK_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pMutex),
                       KERNEL_TRACE_LOW(pMutex));

    /* Check parameters */
    if(pMutex == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_TRYLOCK_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    if(pMutex->isInit == FALSE || pMutex->pWaitingList == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                           TRACE_MUTEX_TRYLOCK_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pMutex),
                           KERNEL_TRACE_LOW(pMutex),
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
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
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    KERNEL_TRACE_EVENT(TRACE_MUTEX_ENABLED,
                       TRACE_MUTEX_TRYLOCK_EXIT,
                       3,
                       KERNEL_TRACE_HIGH(pMutex),
                       KERNEL_TRACE_LOW(pMutex),
                       error);

    return error;
}

/************************************ EOF *************************************/