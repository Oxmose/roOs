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
#include <kqueue.h>       /* Kernel queue */
#include <stdint.h>       /* Standard int types */
#include <stddef.h>       /* Standard definitions */
#include <spinlock.h>     /* Kernel spinlock */
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

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

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

OS_RETURN_E semInit(semaphore_t*   pSem,
                    const int32_t  kInitLevel,
                    const uint32_t kFlags)
{
    /* Check parameters */
    if(pSem == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if((kFlags & SEMAPHORE_FLAG_QUEUING_FIFO) == SEMAPHORE_FLAG_QUEUING_FIFO &&
       (kFlags & SEMAPHORE_FLAG_QUEUING_PRIO) == SEMAPHORE_FLAG_QUEUING_PRIO)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Setup the semaphore */
    pSem->level = kInitLevel;
    pSem->flags = kFlags;
    pSem->pWaitingList = kQueueCreate(FALSE);
    if(pSem->pWaitingList == NULL)
    {
        return OS_ERR_NO_MORE_MEMORY;
    }

    SPINLOCK_INIT(pSem->lock);
    pSem->isInit = TRUE;

    /* TODO: Add the resource to the process */

    return OS_NO_ERR;
}

OS_RETURN_E semDestroy(semaphore_t* pSem)
{
    uint32_t       intState;
    kqueue_node_t* pWaitNode;

    /* Check parameters */
    if(pSem == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(pSem->isInit == FALSE)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Clear the semaphore and wakeup all threads */
    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    spinlockAcquire(&pSem->lock);

    pSem->isInit = FALSE;

    /* Release all waiting threads */
    pWaitNode = kQueuePop(pSem->pWaitingList);
    while(pWaitNode != NULL)
    {
        schedReleaseThread(pWaitNode->pData);
        pWaitNode = kQueuePop(pSem->pWaitingList);
    }

    kQueueDestroy(&pSem->pWaitingList);

    spinlockRelease(&pSem->lock);
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    /* TODO: Remove the resource to the process */

    return OS_NO_ERR;
}

OS_RETURN_E semWait(semaphore_t* pSem)
{
    OS_RETURN_E      error;
    uint32_t         intState;
    kqueue_node_t*   pSemNode;
    kernel_thread_t* pCurThread;

    /* Check parameters */
    if(pSem == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(pSem->isInit == FALSE || pSem->pWaitingList == NULL)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    spinlockAcquire(&pSem->lock);

    if(pSem->level > 0)
    {
        /* Acquire one value */
        --pSem->level;
        spinlockRelease(&pSem->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);

        return OS_NO_ERR;
    }

    /* We were not able to acquire the semaphore, put in waiting list */
    pCurThread = schedGetCurrentThread();

    /* Create a new queue node */
    pSemNode = kQueueCreateNode(pCurThread, FALSE);
    if(pSemNode == NULL)
    {
        spinlockRelease(&pSem->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);

        return OS_ERR_NO_MORE_MEMORY;
    }

    /* Add the node to the queue, if no flags are defined for the queuing
     * discipline, default to FIFO
     */
    if((pSem->flags & SEMAPHORE_FLAG_QUEUING_PRIO) ==
       SEMAPHORE_FLAG_QUEUING_PRIO)
    {
        kQueuePushPrio(pSemNode, pSem->pWaitingList, pCurThread->priority);
    }
    else
    {
        kQueuePush(pSemNode, pSem->pWaitingList);
    }

    /* Release the semaphore lock */
    spinlockRelease(&pSem->lock);

    /* Set the therad as waiting */
    pCurThread->state             = THREAD_STATE_WAITING;
    pCurThread->blockType         = THREAD_WAIT_TYPE_RESOURCE;
    pCurThread->resourceBlockType = THREAD_WAIT_RESOURCE_SEMAPHORE;

    /* Schedule */
    schedSchedule();
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    /* We are back from scheduling, check if the semaphore is still alive */
    if(pSem->isInit == FALSE)
    {
        error = OS_ERR_DESTROYED;
    }
    else
    {
        error = OS_NO_ERR;
    }

    return error;
}

OS_RETURN_E semPost(semaphore_t* pSem)
{
    kqueue_node_t* pWaitNode;
    uint32_t       intState;

    /* Check parameters */
    if(pSem == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(pSem->isInit == FALSE || pSem->pWaitingList == NULL)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    spinlockAcquire(&pSem->lock);
    pWaitNode = kQueuePop(pSem->pWaitingList);

    /* If we need to release a thread */
    if(pWaitNode != NULL)
    {
        schedReleaseThread(pWaitNode->pData);
    }
    else
    {
        /* We do not need to release a waiting thread, increase the semaphore
         * level.
         */
        ++pSem->level;
    }

    spinlockRelease(&pSem->lock);
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    return OS_NO_ERR;
}

OS_RETURN_E semTryWait(semaphore_t* pSem, int32_t* pValue)
{
    /* Check parameters */
    if(pSem == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(pSem->isInit == FALSE)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    spinlockAcquire(&pSem->lock);
    if(pValue != NULL)
    {
        *pValue = pSem->level;
    }

    if(pSem->level > 0)
    {
        /* Acquire one value */
        --pSem->level;
        spinlockRelease(&pSem->lock);
        return OS_NO_ERR;
    }
    else
    {
        spinlockRelease(&pSem->lock);
        return OS_ERR_BLOCKED;
    }
}

/************************************ EOF *************************************/