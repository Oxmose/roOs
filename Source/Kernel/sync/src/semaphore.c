/*******************************************************************************
 * @file semaphore.c
 *
 * @see semaphore.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 24/06/2024
 *
 * @version 2.0
 *
 * @brief Kernel's semaphore API.
 *
 * @details Kernel's semaphore API. This module implements the semaphore
 * management. The semaphore are used to synchronyse the threads. The semaphore
 * waiting list is a FIFO with no regards of the waiting threads priority.
 *
 * @warning Semaphores can only be used when the current system is running and
 * the scheduler initialized.
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
#include <semaphore.h>

/* Unit test header */
#include <test_framework.h>

/* Tracing feature */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "SEMAPHORE"

/** @brief Defines the maximal semaphore wake value */
#define SEMAPHORE_MAX_LEVEL 0x7FFFFFFF

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the state of a semaphore when awakening */
typedef enum
{
    /** @brief The semaphore was posted */
    SEMAPHORE_POSTED = 0,
    /** @brief The semaphore was destroyed */
    SEMAPHORE_DESTROYED = 1
} SEMAPHORE_WAIT_STATUS;

/** @brief Defines the thread's private semaphore data */
typedef struct
{
    /** @brief The thread pointer */
    kernel_thread_t* pThread;

    /** @brief The semaphore wait status */
    SEMAPHORE_WAIT_STATUS status;

    /** @brief The semaphore associated to the data */
    semaphore_t* pSem;
} semaphore_data_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the semaphore to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the semaphore to ensure correctness of
 * execution. Due to the critical nature of the semaphore, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define SEMAPHORE_ASSERT(COND, MSG, ERROR) {                \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Releases a semaphore resource used by a thread.
 *
 * @details Releases a semaphore resource used by a thread. This prevents memory
 * and resource leaks when killing a thread.
 *
 * @param[in] pResource The resource to release.
 */
static void _semaphoreReleaseResource(void* pResource);

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

static void _semaphoreReleaseResource(void* pResource)
{
    semaphore_data_t* pData;
    kqueue_node_t*    pNode;

    KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                       TRACE_SEMAPHORE_RELEASE_RESOURCE_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pResource),
                       KERNEL_TRACE_LOW(pResource));

    SEMAPHORE_ASSERT(pResource != NULL,
                     "NULL Semaphore resource",
                     OS_ERR_NULL_POINTER);

    /* The inly resource we manage are waiting thread's mutex node */
    pNode = pResource;
    pData = pNode->pData;

    KERNEL_LOCK(pData->pSem->lock);

    /* Check if the node is enlisted */
    if(pNode->pQueuePtr != NULL)
    {
        kQueueRemove(pData->pSem->pWaitingList, pNode, TRUE);
    }

    KERNEL_UNLOCK(pData->pSem->lock);

    KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                       TRACE_SEMAPHORE_RELEASE_RESOURCE_EXIT,
                       2,
                       KERNEL_TRACE_HIGH(pResource),
                       KERNEL_TRACE_LOW(pResource));
}

OS_RETURN_E semInit(semaphore_t*   pSem,
                    const int32_t  kInitLevel,
                    const uint32_t kFlags)
{
    KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                       TRACE_SEMAPHORE_INIT_ENTRY,
                       4,
                       KERNEL_TRACE_HIGH(pSem),
                       KERNEL_TRACE_LOW(pSem),
                       kInitLevel,
                       kFlags);

    /* Check parameters */
    if(pSem == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                           TRACE_SEMAPHORE_INIT_EXIT,
                           5,
                           KERNEL_TRACE_HIGH(pSem),
                           KERNEL_TRACE_LOW(pSem),
                           kInitLevel,
                           kFlags,
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    if((kFlags & SEMAPHORE_FLAG_QUEUING_FIFO) == SEMAPHORE_FLAG_QUEUING_FIFO &&
       (kFlags & SEMAPHORE_FLAG_QUEUING_PRIO) == SEMAPHORE_FLAG_QUEUING_PRIO)
    {
        KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                           TRACE_SEMAPHORE_INIT_EXIT,
                           5,
                           KERNEL_TRACE_HIGH(pSem),
                           KERNEL_TRACE_LOW(pSem),
                           kInitLevel,
                           kFlags,
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Setup the semaphore */
    pSem->pWaitingList = kQueueCreate(FALSE);
    if(pSem->pWaitingList == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                           TRACE_SEMAPHORE_INIT_EXIT,
                           5,
                           KERNEL_TRACE_HIGH(pSem),
                           KERNEL_TRACE_LOW(pSem),
                           kInitLevel,
                           kFlags,
                           OS_ERR_NO_MORE_MEMORY);
        return OS_ERR_NO_MORE_MEMORY;
    }

    if((kFlags & SEMAPHORE_FLAG_BINARY) == SEMAPHORE_FLAG_BINARY)
    {
        if(kInitLevel != 0)
        {
            pSem->level = 1;
        }
        else
        {
            pSem->level = 0;
        }
    }
    else
    {
        pSem->level = kInitLevel;
    }
    pSem->flags = kFlags;
    KERNEL_SPINLOCK_INIT(pSem->lock);
    pSem->isInit = TRUE;

    KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                       TRACE_SEMAPHORE_INIT_EXIT,
                       5,
                       KERNEL_TRACE_HIGH(pSem),
                       KERNEL_TRACE_LOW(pSem),
                       kInitLevel,
                       kFlags,
                       OS_ERR_NO_MORE_MEMORY);

    return OS_NO_ERR;
}

OS_RETURN_E semDestroy(semaphore_t* pSem)
{
    uint32_t          intState;
    kqueue_node_t*    pWaitNode;
    semaphore_data_t* pData;

    KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                       TRACE_SEMAPHORE_DESTROY_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pSem),
                       KERNEL_TRACE_LOW(pSem));

    /* Check parameters */
    if(pSem == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                           TRACE_SEMAPHORE_DESTROY_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pSem),
                           KERNEL_TRACE_LOW(pSem),
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    if(pSem->isInit == FALSE || pSem->pWaitingList == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                           TRACE_SEMAPHORE_DESTROY_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pSem),
                           KERNEL_TRACE_LOW(pSem),
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Clear the semaphore and wakeup all threads */
    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pSem->lock);

    pSem->isInit = FALSE;

    /* Release all waiting threads */
    pWaitNode = kQueuePop(pSem->pWaitingList);
    while(pWaitNode != NULL)
    {
        pData = pWaitNode->pData;
        pData->status = SEMAPHORE_DESTROYED;

        schedReleaseThread(pData->pThread, FALSE, THREAD_STATE_READY, FALSE);
        pWaitNode = kQueuePop(pSem->pWaitingList);
    }

    kQueueDestroy(&pSem->pWaitingList);

    KERNEL_UNLOCK(pSem->lock);

    /* Schedule in case more prioritary thread were scheduled */
    schedSchedule();

    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                       TRACE_SEMAPHORE_DESTROY_EXIT,
                       3,
                       KERNEL_TRACE_HIGH(pSem),
                       KERNEL_TRACE_LOW(pSem),
                       OS_NO_ERR);
    return OS_NO_ERR;
}

OS_RETURN_E semWait(semaphore_t* pSem)
{
    OS_RETURN_E       error;
    uint32_t          intState;
    kqueue_node_t     semNode;
    kernel_thread_t*  pCurThread;
    semaphore_data_t  data;
    thread_resource_t threadRes;
    void*             pResourceHandle;

    KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                       TRACE_SEMAPHORE_WAIT_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pSem),
                       KERNEL_TRACE_LOW(pSem));

    /* Check parameters */
    if(pSem == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                           TRACE_SEMAPHORE_WAIT_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pSem),
                           KERNEL_TRACE_LOW(pSem),
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    if(pSem->isInit == FALSE || pSem->pWaitingList == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                           TRACE_SEMAPHORE_WAIT_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pSem),
                           KERNEL_TRACE_LOW(pSem),
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pSem->lock);

    if(pSem->level > 0)
    {
        /* Acquire one value */
        --pSem->level;
        KERNEL_UNLOCK(pSem->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);

        KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                           TRACE_SEMAPHORE_WAIT_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pSem),
                           KERNEL_TRACE_LOW(pSem),
                           OS_NO_ERR);

        return OS_NO_ERR;
    }

    /* We were not able to acquire the semaphore, put in waiting list */
    pCurThread = schedGetCurrentThread();

    /* Create a new queue node */
    data.pThread = pCurThread;
    data.status = SEMAPHORE_DESTROYED;
    kQueueInitNode(&semNode, &data);

    /* Add the node to the queue, if no flags are defined for the queuing
     * discipline, default to FIFO
     */
    if((pSem->flags & SEMAPHORE_FLAG_QUEUING_PRIO) ==
       SEMAPHORE_FLAG_QUEUING_PRIO)
    {
        kQueuePushPrio(&semNode, pSem->pWaitingList, pCurThread->priority);
    }
    else
    {
        kQueuePush(&semNode, pSem->pWaitingList);
    }

    threadRes.pReleaseResource = _semaphoreReleaseResource;
    threadRes.pResourceData    = &semNode;
    pResourceHandle = schedThreadAddResource(&threadRes);
    if(pResourceHandle == NULL)
    {
        kQueueRemove(pSem->pWaitingList, &semNode, TRUE);
        KERNEL_UNLOCK(pSem->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);

        KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                           TRACE_SEMAPHORE_WAIT_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pSem),
                           KERNEL_TRACE_LOW(pSem),
                           OS_ERR_INCORRECT_VALUE);

        return OS_ERR_INCORRECT_VALUE;
    }

    /* Set the thread as waiting */
    schedWaitThreadOnResource(THREAD_WAIT_RESOURCE_SEMAPHORE);

    /* Release the semaphore lock */
    KERNEL_UNLOCK(pSem->lock);

    /* Schedule */
    schedSchedule();
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    /* Ensure the node was released */
    SEMAPHORE_ASSERT(semNode.pQueuePtr == NULL,
                     "Failed to delist semaphore node",
                     OS_ERR_UNAUTHORIZED_ACTION);

    /* Release the resource */
    error = schedThreadRemoveResource(pResourceHandle);
    SEMAPHORE_ASSERT(error == OS_NO_ERR,
                     "Failed to remove semaphore resource",
                     error);

    /* We are back from scheduling, check if the semaphore is still alive */
    if(data.status == SEMAPHORE_DESTROYED)
    {
        error = OS_ERR_DESTROYED;
    }
    else
    {
        error = OS_NO_ERR;
    }

    KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                       TRACE_SEMAPHORE_WAIT_EXIT,
                       3,
                       KERNEL_TRACE_HIGH(pSem),
                       KERNEL_TRACE_LOW(pSem),
                       error);

    return error;
}

OS_RETURN_E semPost(semaphore_t* pSem)
{
    kqueue_node_t*    pWaitNode;
    semaphore_data_t* pData;
    uint32_t          intState;

    KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                       TRACE_SEMAPHORE_POST_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pSem),
                       KERNEL_TRACE_LOW(pSem));

    /* Check parameters */
    if(pSem == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                           TRACE_SEMAPHORE_POST_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pSem),
                           KERNEL_TRACE_LOW(pSem),
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    if(pSem->isInit == FALSE || pSem->pWaitingList == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                           TRACE_SEMAPHORE_POST_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pSem),
                           KERNEL_TRACE_LOW(pSem),
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pSem->lock);
    pWaitNode = kQueuePop(pSem->pWaitingList);

    /* If we need to release a thread */
    if(pWaitNode != NULL)
    {
        pData = pWaitNode->pData;
        pData->status = SEMAPHORE_POSTED;
        KERNEL_UNLOCK(pSem->lock);
        schedReleaseThread(pData->pThread, FALSE, THREAD_STATE_READY, TRUE);
    }
    else
    {
        /* We do not need to release a waiting thread, increase the semaphore
         * level.
         */
        if(((pSem->flags & SEMAPHORE_FLAG_BINARY) != SEMAPHORE_FLAG_BINARY ||
            pSem->level <= 0) &&
            pSem->level < SEMAPHORE_MAX_LEVEL)
        {
            ++pSem->level;
        }
        KERNEL_UNLOCK(pSem->lock);
    }
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                       TRACE_SEMAPHORE_POST_EXIT,
                       3,
                       KERNEL_TRACE_HIGH(pSem),
                       KERNEL_TRACE_LOW(pSem),
                       OS_NO_ERR);

    return OS_NO_ERR;
}

OS_RETURN_E semTryWait(semaphore_t* pSem, int32_t* pValue)
{
    OS_RETURN_E error;

    KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                       TRACE_SEMAPHORE_TRYWAIT_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pSem),
                       KERNEL_TRACE_LOW(pSem));

    /* Check parameters */
    if(pSem == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                           TRACE_SEMAPHORE_TRYWAIT_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pSem),
                           KERNEL_TRACE_LOW(pSem),
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    if(pSem->isInit == FALSE)
    {
        KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                           TRACE_SEMAPHORE_TRYWAIT_EXIT,
                           3,
                           KERNEL_TRACE_HIGH(pSem),
                           KERNEL_TRACE_LOW(pSem),
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    KERNEL_LOCK(pSem->lock);
    if(pValue != NULL)
    {
        *pValue = pSem->level;
    }

    if(pSem->level > 0)
    {
        /* Acquire one value */
        --pSem->level;
        error = OS_NO_ERR;
    }
    else
    {
        error = OS_ERR_BLOCKED;
    }
    KERNEL_UNLOCK(pSem->lock);

    KERNEL_TRACE_EVENT(TRACE_SEMAPHORE_ENABLED,
                       TRACE_SEMAPHORE_TRYWAIT_EXIT,
                       3,
                       KERNEL_TRACE_HIGH(pSem),
                       KERNEL_TRACE_LOW(pSem),
                       error);

    return error;
}

/************************************ EOF *************************************/