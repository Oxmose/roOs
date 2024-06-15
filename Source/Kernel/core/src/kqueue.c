/*******************************************************************************
 * @file kqueue.c
 *
 * @see kqueue.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/05/2023
 *
 * @version 1.5
 *
 * @brief Kernel specific queue structures.
 *
 * @details Kernel specific queue structures. These queues are used as priority
 * queue or regular queues. A queue can virtually store every type of data and
 * is just a wrapper.
 *
 * @warning This implementation is thread safe.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <kheap.h>        /* Kernel heap */
#include <panic.h>        /* Kernel panic */
#include <stddef.h>       /* Standard definitions */
#include <stdint.h>       /* Generic int types */
#include <string.h>       /* String manipulation */
#include <kerror.h>       /* Kernel errors definitions */
#include <kerneloutput.h> /* Kernel output methods */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <kqueue.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the kernel queues to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the kernel queues to ensure correctness of
 * execution. Due to the critical nature of the kernel queues, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define KQUEUE_ASSERT(COND, MSG, ERROR) {                    \
    if((COND) == FALSE)                                      \
    {                                                        \
        PANIC(ERROR, "KQUEUE", MSG, TRUE);                   \
    }                                                        \
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

kqueue_node_t* kQueueCreateNode(void* pData)
{
    kqueue_node_t* pNewNode;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_CREATE_NODE_ENTRY,
                       2,
                       0,
                       (uint32_t)pData);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_CREATE_NODE_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)pData >> 32),
                       (uint32_t)(uintptr_t)pData);
#endif

    /* Create new node */
    pNewNode = kmalloc(sizeof(kqueue_node_t));
    KQUEUE_ASSERT(pNewNode != NULL,
                  "Could not allocate knode",
                  OS_ERR_NO_MORE_MEMORY);


    /* Init the structure */
    memset(pNewNode, 0, sizeof(kqueue_node_t));
    pNewNode->pData = pData;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_CREATE_NODE_EXIT,
                       4,
                       0,
                       (uint32_t)pData,
                       0,
                       (uint32_t)pNewNode);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_CREATE_NODE_EXIT,
                       4,
                       (uint32_t)((uintptr_t)pData >> 32),
                       (uint32_t)(uintptr_t)pData,
                       (uint32_t)((uintptr_t)pNewNode >> 32),
                       (uint32_t)(uintptr_t)pNewNode);
#endif

    return pNewNode;
}

void kQueueDestroyNode(kqueue_node_t** ppNode)
{

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_DESTROY_NODE,
                       2,
                       0,
                       (uint32_t)ppNode);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_DESTROY_NODE,
                       2,
                       (uint32_t)((uintptr_t)ppNode >> 32),
                       (uint32_t)(uintptr_t)ppNode);
#endif

    KQUEUE_ASSERT((ppNode != NULL && *ppNode != NULL),
                  "Tried to delete a NULL node",
                  OS_ERR_NULL_POINTER);

    KQUEUE_ASSERT((*ppNode)->enlisted == FALSE,
                  "Tried to delete an enlisted node",
                  OS_ERR_UNAUTHORIZED_ACTION);

    kfree(*ppNode);
    *ppNode = NULL;
}

kqueue_t* kQueueCreate(void)
{
    kqueue_t* pNewQueue;

    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_CREATE_ENTRY,
                       0);

    /* Create new node */
    pNewQueue = kmalloc(sizeof(kqueue_t));
    KQUEUE_ASSERT(pNewQueue != NULL,
                  "Could not allocate kqueue",
                  OS_ERR_NO_MORE_MEMORY);

    /* Init the structure */
    memset(pNewQueue, 0, sizeof(kqueue_t));
    KERNEL_SPINLOCK_INIT(pNewQueue->lock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_CREATE_EXIT,
                       2,
                       0,
                       (uint32_t)pNewQueue);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_CREATE_EXIT,
                       2,
                       (uint32_t)((uintptr_t)pNewQueue >> 32),
                       (uint32_t)(uintptr_t)pNewQueue);
#endif

    return pNewQueue;
}

void kQueueDestroy(kqueue_t** ppQueue)
{

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_DESTROY,
                       2,
                       0,
                       (uint32_t)ppQueue);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_DESTROY,
                       2,
                       (uint32_t)((uintptr_t)ppQueue >> 32),
                       (uint32_t)(uintptr_t)ppQueue);
#endif

    KQUEUE_ASSERT((ppQueue != NULL && *ppQueue != NULL),
                  "Tried to delete a NULL queue",
                  OS_ERR_NULL_POINTER);

    KQUEUE_ASSERT(((*ppQueue)->pHead == NULL && (*ppQueue)->pTail == NULL),
                  "Tried to delete a non empty queue",
                  OS_ERR_UNAUTHORIZED_ACTION);

    kfree(*ppQueue);
    *ppQueue = NULL;
}

void kQueuePush(kqueue_node_t* pNode, kqueue_t* pQueue)
{

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_PUSH_ENTRY,
                       4,
                       0,
                       (uint32_t)pNode,
                       0,
                       (uint32_t)pQueue);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_PUSH_ENTRY,
                       4,
                       (uint32_t)((uintptr_t)pNode >> 32),
                       (uint32_t)(uintptr_t)pNode,
                       (uint32_t)((uintptr_t)pQueue >> 32),
                       (uint32_t)(uintptr_t)pQueue);
#endif

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue try push knode 0x%p in kqueue 0x%p",
                 pNode,
                 pQueue);

    KQUEUE_ASSERT((pNode != NULL && pQueue != NULL),
                  "Cannot push with NULL knode or NULL kqueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_CRITICAL_LOCK(pQueue->lock);

    /* If this queue is empty */
    if(pQueue->pHead == NULL)
    {
        /* Set the first item */
        pQueue->pHead = pNode;
        pQueue->pTail = pNode;
        pNode->pNext  = NULL;
        pNode->pPrev  = NULL;
    }
    else
    {
        /* Just put on the head */
        pNode->pNext         = pQueue->pHead;
        pNode->pPrev         = NULL;
        pQueue->pHead->pPrev = pNode;
        pQueue->pHead        = pNode;
    }

    ++pQueue->size;
    pNode->enlisted = TRUE;


    KQUEUE_ASSERT((pNode->pNext != pNode->pPrev ||
                   pNode->pNext == NULL ||
                   pNode->pPrev == NULL),
                  "Cycle detected in KQueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_CRITICAL_UNLOCK(pQueue->lock);

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue pushed knode 0x%p in kqueue 0x%p",
                 pNode,
                 pQueue);
#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_PUSH_EXIT,
                       4,
                       0,
                       (uint32_t)pNode,
                       0,
                       (uint32_t)pQueue);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_PUSH_EXIT,
                       4,
                       (uint32_t)((uintptr_t)pNode >> 32),
                       (uint32_t)(uintptr_t)pNode,
                       (uint32_t)((uintptr_t)pQueue >> 32),
                       (uint32_t)(uintptr_t)pQueue);
#endif
}


void kQueuePushPrio(kqueue_node_t*  pNode,
                    kqueue_t*       pQueue,
                    const uintptr_t kPriority)
{
    kqueue_node_t* pCursor;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_PUSH_PRIO_ENTRY,
                       5,
                       0,
                       (uint32_t)pNode,
                       0,
                       (uint32_t)pQueue,
                       kPriority);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_PUSH_PRIO_ENTRY,
                       5,
                       (uint32_t)((uintptr_t)pNode >> 32),
                       (uint32_t)(uintptr_t)pNode,
                       (uint32_t)((uintptr_t)pQueue >> 32),
                       (uint32_t)(uintptr_t)pQueue,
                       kPriority);
#endif

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue try push prio knode 0x%p in kqueue 0x%p",
                 pNode,
                 pQueue);

    KQUEUE_ASSERT((pNode != NULL && pQueue != NULL),
                  "Cannot push with NULL knode or NULL kqueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_CRITICAL_LOCK(pQueue->lock);

    pNode->priority = kPriority;

    /* If this queue is empty */
    if(pQueue->pHead == NULL)
    {
        /* Set the first item */
        pQueue->pHead = pNode;
        pQueue->pTail = pNode;
        pNode->pNext  = NULL;
        pNode->pPrev  = NULL;
    }
    else
    {
        pCursor = pQueue->pHead;
        while(pCursor != NULL && pCursor->priority > kPriority)
        {
            pCursor = pCursor->pNext;
        }

        if(pCursor != NULL)
        {
            pNode->pNext   = pCursor;
            pNode->pPrev   = pCursor->pPrev;
            pCursor->pPrev = pNode;

            if(pNode->pPrev != NULL)
            {
                pNode->pPrev->pNext = pNode;
            }
            else
            {
                pQueue->pHead = pNode;
            }
        }
        else
        {
            /* Just put on the tail */
            pNode->pPrev         = pQueue->pTail;
            pNode->pNext         = NULL;
            pQueue->pTail->pNext = pNode;
            pQueue->pTail        = pNode;
        }
    }
    ++pQueue->size;
    pNode->enlisted = TRUE;

    KQUEUE_ASSERT((pNode->pNext != pNode->pPrev ||
                   pNode->pNext == NULL ||
                   pNode->pPrev == NULL),
                  "Cycle detected in KQueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_CRITICAL_UNLOCK(pQueue->lock);

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED, "KQUEUE",
                 "KQueue pushed knode 0x%p in kqueue 0x%p", pNode, pQueue);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_PUSH_PRIO_EXIT,
                       5,
                       0,
                       (uint32_t)pNode,
                       0,
                       (uint32_t)pQueue,
                       kPriority);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_PUSH_PRIO_EXIT,
                       5,
                       (uint32_t)((uintptr_t)pNode >> 32),
                       (uint32_t)(uintptr_t)pNode,
                       (uint32_t)((uintptr_t)pQueue >> 32),
                       (uint32_t)(uintptr_t)pQueue,
                       kPriority);
#endif
}

kqueue_node_t* kQueuePop(kqueue_t* pQueue)
{
    kqueue_node_t* pNode;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_POP_ENTRY,
                       2,
                       0,
                       (uint32_t)pQueue);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_POP_ENTRY,
                       2,
                       (uint32_t)((uintptr_t)pQueue >> 32),
                       (uint32_t)(uintptr_t)pQueue);
#endif

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue try pop knode from kqueue 0x%p",
                 pQueue);

    KQUEUE_ASSERT(pQueue != NULL,
                  "Cannot pop NULL kqueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_CRITICAL_LOCK(pQueue->lock);

    /* If this queue is empty */
    if(pQueue->pHead == NULL)
    {
        KERNEL_CRITICAL_UNLOCK(pQueue->lock);

#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                           TRACE_KQUEUE_QUEUE_POP_EXIT,
                           4,
                           NULL,
                           NULL,
                           0,
                           (uint32_t)pQueue);
#else
        KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                           TRACE_KQUEUE_QUEUE_POP_EXIT,
                           4,
                           NULL,
                           NULL,
                           (uint32_t)((uintptr_t)pQueue >> 32),
                           (uint32_t)(uintptr_t)pQueue);
#endif
        return NULL;
    }

    /* Dequeue the last item */
    pNode = pQueue->pTail;

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "Pop knode 0x%p from kqueue 0x%p",
                 pNode,
                 pQueue);

    if(pNode->pPrev != NULL)
    {
        pNode->pPrev->pNext = NULL;
        pQueue->pTail = pNode->pPrev;
    }
    else
    {
        pQueue->pHead = NULL;
        pQueue->pTail = NULL;
    }

    --pQueue->size;

    pNode->pNext = NULL;
    pNode->pPrev = NULL;
    pNode->enlisted = FALSE;

    KERNEL_CRITICAL_UNLOCK(pQueue->lock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_POP_EXIT,
                       4,
                       0,
                       (uint32_t)pNode,
                       0,
                       (uint32_t)pQueue);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_POP_EXIT,
                       4,
                       (uint32_t)((uintptr_t)pNode >> 32),
                       (uint32_t)(uintptr_t)pNode,
                       (uint32_t)((uintptr_t)pQueue >> 32),
                       (uint32_t)(uintptr_t)pQueue);
#endif

    return pNode;
}

kqueue_node_t* kQueueFind(kqueue_t* pQueue, const void* kpData)
{
    kqueue_node_t* pNode;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_FIND_ENTRY,
                       4,
                       0,
                       (uint32_t)kpData,
                       0,
                       (uint32_t)pQueue);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_FIND_ENTRY,
                       4,
                       (uint32_t)((uintptr_t)kpData >> 32),
                       (uint32_t)(uintptr_t)kpData,
                       (uint32_t)((uintptr_t)pQueue >> 32),
                       (uint32_t)(uintptr_t)pQueue);
#endif

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue try find data 0x%p from kqueue 0x%p",
                 kpData,
                 pQueue);

    KQUEUE_ASSERT(pQueue != NULL,
                  "Cannot find in NULL kqueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_CRITICAL_LOCK(pQueue->lock);

    /* Search for the data */
    pNode = pQueue->pHead;
    while(pNode != NULL && pNode->pData != kpData)
    {
        pNode = pNode->pNext;
    }

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue found node 0x%p from kqueue 0x%p",
                 pNode,
                 pQueue);

    KERNEL_CRITICAL_UNLOCK(pQueue->lock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_FIND_EXIT,
                       6,
                       0,
                       (uint32_t)kpData,
                       0,
                       (uint32_t)pQueue,
                       0,
                       (uint32_t)pNode);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_FIND_EXIT,
                       6,
                       (uint32_t)((uintptr_t)kpData >> 32),
                       (uint32_t)(uintptr_t)kpData,
                       (uint32_t)((uintptr_t)pQueue >> 32),
                       (uint32_t)(uintptr_t)pQueue,
                       (uint32_t)((uintptr_t)pNode >> 32),
                       (uint32_t)(uintptr_t)pNode);
#endif

    return pNode;
}

void kQueueRemove(kqueue_t* pQueue, kqueue_node_t* pNode, const bool_t kPanic)
{
    kqueue_node_t* pCursor;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_REMOVE_ENTRY,
                       4,
                       0,
                       (uint32_t)pQueue,
                       0,
                       (uint32_t)pNode);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_REMOVE_ENTRY,
                       4,
                       (uint32_t)((uintptr_t)pQueue >> 32),
                       (uint32_t)(uintptr_t)pQueue,
                       (uint32_t)((uintptr_t)pNode >> 32),
                       (uint32_t)(uintptr_t)pNode);
#endif

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue try renove knode 0x%p from kqueue 0x%p",
                 pNode,
                 pQueue);

    KQUEUE_ASSERT((pNode != NULL && pQueue != NULL),
                  "Cannot remove with NULL knode or NULL kqueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_CRITICAL_LOCK(pQueue->lock);

    /* Search for node in the queue*/
    pCursor = pQueue->pHead;
    while(pCursor != NULL && pCursor != pNode)
    {
        pCursor = pCursor->pNext;
    }

    KQUEUE_ASSERT((pCursor != NULL || kPanic == FALSE),
                  "Could not find knode to remove",
                  OS_ERR_INCORRECT_VALUE);

    if(pCursor == NULL)
    {
        KERNEL_CRITICAL_UNLOCK(pQueue->lock);
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                           TRACE_KQUEUE_QUEUE_REMOVE_EXIT,
                           5,
                           0,
                           (uint32_t)pQueue,
                           0,
                           (uint32_t)pNode,
                           1);
#else
        KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                           TRACE_KQUEUE_QUEUE_REMOVE_EXIT,
                           5,
                           (uint32_t)((uintptr_t)pQueue >> 32),
                           (uint32_t)(uintptr_t)pQueue,
                           (uint32_t)((uintptr_t)pNode >> 32),
                           (uint32_t)(uintptr_t)pNode,
                           1);
#endif
        return;
    }

    /* Manage link */
    if(pCursor->pPrev != NULL && pCursor->pNext != NULL)
    {
        pCursor->pPrev->pNext = pCursor->pNext;
        pCursor->pNext->pPrev = pCursor->pPrev;
    }
    else if(pCursor->pPrev == NULL && pCursor->pNext != NULL)
    {
        pQueue->pHead = pCursor->pNext;
        pCursor->pNext->pPrev = NULL;
    }
    else if(pCursor->pPrev != NULL && pCursor->pNext == NULL)
    {
        pQueue->pTail = pCursor->pPrev;
        pCursor->pPrev->pNext = NULL;
    }
    else
    {
        pQueue->pHead = NULL;
        pQueue->pTail = NULL;
    }

    pNode->pNext = NULL;
    pNode->pPrev = NULL;

    pNode->enlisted = FALSE;

    KERNEL_CRITICAL_UNLOCK(pQueue->lock);

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue renoved knode 0x%p from kqueue 0x%p",
                 pNode, pQueue);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_REMOVE_EXIT,
                       5,
                       0,
                       (uint32_t)pQueue,
                       0,
                       (uint32_t)pNode,
                       0);
#else
    KERNEL_TRACE_EVENT(TRACE_KQUEUE_ENABLED,
                       TRACE_KQUEUE_QUEUE_REMOVE_EXIT,
                       5,
                       (uint32_t)((uintptr_t)pQueue >> 32),
                       (uint32_t)(uintptr_t)pQueue,
                       (uint32_t)((uintptr_t)pNode >> 32),
                       (uint32_t)(uintptr_t)pNode,
                       0);
#endif
}

/************************************ EOF *************************************/