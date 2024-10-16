/*******************************************************************************
 * @file futex.c
 *
 * @see futex.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <kheap.h>        /* Kernel heap */
#include <panic.h>        /* Kernel panic */
#include <stdint.h>       /* Standard int types */
#include <stddef.h>       /* Standard definitions */
#include <kqueue.h>       /* Kernel queue */
#include <memory.h>       /* Memory manager */
#include <atomic.h>       /* Kernel spinlock */
#include <critical.h>     /* Kernel critical sections */
#include <scheduler.h>    /* Kernel scheduler */
#include <ctrl_block.h>   /* Kernel control blocks definitions */
#include <uhashtable.h>   /* Kernel Uint Hash table */

/* Configuration files */
#include <config.h>

/* Header file */
#include <futex.h>

/* Unit test header */
#include <test_framework.h>
#ifdef _TESTING_FRAMEWORK_ENABLED
#if TEST_FUTEX_ENABLED
#define static
#endif
#endif

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "FUTEX"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Futex data structure definition. */
typedef struct
{
    /** @brief The thread's node waiting on the futex */
    kqueue_t* pWaitingThreads;

    /** @brief Number of locked thread on the futex  */
    volatile uintptr_t nbWaitingThreads;

    /** @brief The futex data lock */
    kernel_spinlock_t lock;
} futex_data_t;

/** @brief Futex waiting thread structure */
typedef struct
{
    /** @brief The threadwaiting on the futex */
    kernel_thread_t* pWaitingThread;

    /** @brief The value waiting to be observed */
    volatile uint32_t waitValue;

    /** @brief Wakeup reason */
    volatile FUTEX_WAKE_REASON_E wakeReason;

    /** @brief Waiting futex Identifier */
    volatile uintptr_t identifier;
} futex_waiting_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the futex lib to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the futex lib to ensure correctness of
 * execution. Due to the critical nature of the futex lib, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define FUTEX_ASSERT(COND, MSG, ERROR) {                    \
    if((COND) == FALSE)                                     \
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
/** @brief Futex table, contains the currently alive futexes */
static uhashtable_t* spFutexTable = NULL;

/** @brief Futex table lock */
static kernel_spinlock_t sLock;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void futexLibInit(void)
{

    OS_RETURN_E err;

    /* Create the futex hashtable */
    spFutexTable = uhashtableCreate(UHASHTABLE_ALLOCATOR(kmalloc, kfree), &err);
    FUTEX_ASSERT(err == OS_NO_ERR, "Could not initialize futex table", err);

    KERNEL_SPINLOCK_INIT(sLock);
}

OS_RETURN_E futexWait(futex_t*             pFutex,
                      const uint32_t       kWaitValue,
                      FUTEX_WAKE_REASON_E* pWakeReason)
{
    OS_RETURN_E     error;
    uintptr_t       identifier;
    futex_waiting_t waiting;
    kqueue_node_t   waitingNode;
    futex_data_t*   pFutexData;
    uint32_t        intState;

    /* Check parameters */
    if(pFutex == NULL || pFutex->pHandle == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Get the identifier, we use the physical address of the handle */
    identifier = memoryMgrGetPhysAddr((uintptr_t)pFutex->pHandle, NULL);
    if(identifier == MEMMGR_PHYS_ADDR_ERROR)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    if(pFutex->isAlive != TRUE || *pFutex->pHandle != kWaitValue)
    {
        return OS_ERR_NOT_BLOCKED;
    }

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(sLock);

    /* Check if the handle exists */
    error = uhashtableGet(spFutexTable, identifier, (void**)&pFutexData);
    if(error == OS_ERR_NO_SUCH_ID)
    {
        /* Create the futex data */
        pFutexData = kmalloc(sizeof(futex_data_t));
        FUTEX_ASSERT(pFutexData != NULL,
                     "Failed to allocate futex",
                     OS_ERR_NO_MORE_MEMORY);

        /* Create the futex waiting queue */
        pFutexData->pWaitingThreads = kQueueCreate(FALSE);
        if(pFutexData->pWaitingThreads == NULL)
        {
            KERNEL_UNLOCK(sLock);
            KERNEL_EXIT_CRITICAL_LOCAL(intState);
            kfree(pFutexData);
            return OS_ERR_NO_MORE_MEMORY;
        }
        FUTEX_ASSERT(pFutexData->pWaitingThreads != NULL,
                     "Failed to allocate futex queue",
                     OS_ERR_NO_MORE_MEMORY);

        pFutexData->nbWaitingThreads = 0;
        KERNEL_SPINLOCK_INIT(pFutexData->lock);

        error = uhashtableSet(spFutexTable, identifier, (void*)pFutexData);
        FUTEX_ASSERT(error == OS_NO_ERR, "Failed to create futex", error);
    }
    else
    {
        FUTEX_ASSERT(error == OS_NO_ERR, "Failed to get futex", error);
    }
    ++pFutexData->nbWaitingThreads;

    KERNEL_UNLOCK(sLock);

    waiting.pWaitingThread = schedGetCurrentThread();
    waiting.waitValue      = kWaitValue;
    waiting.wakeReason     = FUTEX_WAKEUP_WAKE;
    waiting.identifier     = identifier;

    kQueueInitNode(&waitingNode, &waiting);
    do
    {
        /* Set thread as waiting */
        schedWaitThreadOnResource(THREAD_WAIT_RESOURCE_FUTEX);

        KERNEL_LOCK(pFutexData->lock);

        /* Add the node to the waiting thread */
        kQueuePush(&waitingNode, pFutexData->pWaitingThreads);

        KERNEL_UNLOCK(pFutexData->lock);

        /* Request schedule */
        schedSchedule();
    /* Check the new value, maybe we were woken up by a spurious event */
    } while(pFutex->isAlive == TRUE && *(pFutex->pHandle) == kWaitValue);


    KERNEL_LOCK(sLock);

    /* Release the resource */
    --pFutexData->nbWaitingThreads;

    /* See if we should remove the futex */
    if(pFutex->isAlive == FALSE && 0 == pFutexData->nbWaitingThreads)
    {
        error = uhashtableRemove(spFutexTable, identifier, (void**)&pFutexData);
        KERNEL_UNLOCK(sLock);

        FUTEX_ASSERT(error == OS_NO_ERR, "Failed to remove futex", error);
        kQueueDestroy(&pFutexData->pWaitingThreads);

        /* Destroy the data */
        kfree(pFutexData);
    }
    else
    {
        KERNEL_UNLOCK(sLock);
    }

    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    /* Ensure the node was released */
    FUTEX_ASSERT(waitingNode.pQueuePtr == NULL,
                 "Failed to delist futex node",
                 OS_ERR_UNAUTHORIZED_ACTION);

    if(pWakeReason != NULL)
    {
        *pWakeReason = waiting.wakeReason;
    }

    /* We returned from schedule, check the current state of the futex */
    if(waiting.wakeReason == FUTEX_WAKEUP_DESTROYED)
    {
        error = OS_ERR_DESTROYED;
    }
    else
    {
        error = OS_NO_ERR;
    }

    return error;
}

OS_RETURN_E futexWake(futex_t* pFutex, const uintptr_t kWakeCount)
{
    OS_RETURN_E      error;
    uintptr_t        identifier;
    uintptr_t        i;
    futex_waiting_t* pWaiting;
    futex_data_t*    pFutexData;
    kqueue_node_t*   pWaitingNode;
    kqueue_node_t*   pSaveNode;
    kernel_thread_t* pThread;
    uint32_t         intState;

    /* Check parameters */
    if(pFutex == NULL || pFutex->pHandle == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Get the identifier, we use the physical address of the handle */
    identifier = memoryMgrGetPhysAddr((uintptr_t)pFutex->pHandle, NULL);
    if(identifier == MEMMGR_PHYS_ADDR_ERROR)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Find the futex */
    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(sLock);
    error = uhashtableGet(spFutexTable, identifier, (void**)&pFutexData);
    KERNEL_UNLOCK(sLock);
    if(error != OS_NO_ERR)
    {
        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        return error;
    }

    KERNEL_LOCK(pFutexData->lock);

    /* Wake up the next threads to wake */
    pWaitingNode = pFutexData->pWaitingThreads->pTail;

    for(i = 0; i < kWakeCount && pWaitingNode != NULL; ++i)
    {
        pWaiting = pWaitingNode->pData;

        /* Check if the wake value is different than the wait value */
        if(pWaiting->waitValue != *pFutex->pHandle)
        {
            if(pFutex->isAlive == TRUE)
            {
                pWaiting->wakeReason = FUTEX_WAKEUP_WAKE;
            }
            else
            {
                pWaiting->wakeReason = FUTEX_WAKEUP_DESTROYED;
            }
            pThread = pWaiting->pWaitingThread;
            pSaveNode = pWaitingNode;
            pWaitingNode = pWaitingNode->pPrev;
            kQueueRemove(pFutexData->pWaitingThreads,
                         pSaveNode,
                         TRUE);
            schedReleaseThread(pThread, FALSE, THREAD_STATE_READY, FALSE);
        }
        else
        {
            pWaitingNode = pWaitingNode->pPrev;
        }
    }

    KERNEL_UNLOCK(pFutexData->lock);

    /* Schedule in case more prioritary thread were scheduled */
    schedSchedule();

    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    return OS_NO_ERR;
}

/************************************ EOF *************************************/