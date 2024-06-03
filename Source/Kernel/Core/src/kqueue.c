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
 * @warning This implementation is not thread safe.
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

    /* Create new node */
    pNewNode = kmalloc(sizeof(kqueue_node_t));
    KQUEUE_ASSERT(pNewNode != NULL,
                  "Could not allocate knode",
                  OS_ERR_NO_MORE_MEMORY);


    /* Init the structure */
    memset(pNewNode, 0, sizeof(kqueue_node_t));
    pNewNode->pData = pData;

    return pNewNode;
}

void kQueueDestroyNode(kqueue_node_t** ppNode)
{
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

    /* Create new node */
    pNewQueue = kmalloc(sizeof(kqueue_t));
    KQUEUE_ASSERT(pNewQueue != NULL,
                  "Could not allocate kqueue",
                  OS_ERR_NO_MORE_MEMORY);

    /* Init the structure */
    memset(pNewQueue, 0, sizeof(kqueue_t));

    return pNewQueue;
}

void kQueueDestroy(kqueue_t** ppQueue)
{
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
    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue try push knode 0x%p in kqueue 0x%p",
                 pNode,
                 pQueue);

    KQUEUE_ASSERT((pNode != NULL && pQueue != NULL),
                  "Cannot push with NULL knode or NULL kqueue",
                  OS_ERR_NULL_POINTER);

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

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue pushed knode 0x%p in kqueue 0x%p",
                 pNode,
                 pQueue);
}


void kQueuePushPrio(kqueue_node_t* pNode,
                    kqueue_t*      pQueue,
                    const uint32_t kPriority)
{
    kqueue_node_t* pCursor;

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue try push prio knode 0x%p in kqueue 0x%p",
                 pNode,
                 pQueue);

    KQUEUE_ASSERT((pNode != NULL && pQueue != NULL),
                  "Cannot push with NULL knode or NULL kqueue",
                  OS_ERR_NULL_POINTER);

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

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED, "KQUEUE",
                 "KQueue pushed knode 0x%p in kqueue 0x%p", pNode, pQueue);
}

kqueue_node_t* kQueuePop(kqueue_t* pQueue)
{
    kqueue_node_t* pNode;

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue try pop knode from kqueue 0x%p",
                 pQueue);

    KQUEUE_ASSERT(pQueue != NULL,
                  "Cannot pop NULL kqueue",
                  OS_ERR_NULL_POINTER);

    /* If this queue is empty */
    if(pQueue->pHead == NULL)
    {
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

    return pNode;
}

kqueue_node_t* kQueueFind(const kqueue_t* kpQueue, const void* kpData)
{
    kqueue_node_t* pNode;

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue try find data 0x%p from kqueue 0x%p",
                 kpData,
                 kpQueue);

    KQUEUE_ASSERT(kpQueue != NULL,
                  "Cannot find in NULL kqueue",
                  OS_ERR_NULL_POINTER);

    /* Search for the data */
    pNode = kpQueue->pHead;
    while(pNode != NULL && pNode->pData != kpData)
    {
        pNode = pNode->pNext;
    }

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue found node 0x%p from kqueue 0x%p",
                 pNode,
                 kpQueue);

    return pNode;
}

void kQueueRemove(kqueue_t* pQueue, kqueue_node_t* pNode, const bool_t kPanic)
{
    kqueue_node_t* pCursor;

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue try renove knode 0x%p from kqueue 0x%p",
                 pNode,
                 pQueue);

    KQUEUE_ASSERT((pNode != NULL && pQueue != NULL),
                  "Cannot remove with NULL knode or NULL kqueue",
                  OS_ERR_NULL_POINTER);

    /* Search for node in the queue*/
    pCursor = pQueue->pHead;
    while(pCursor != NULL && pCursor != pNode)
    {
        pCursor = pCursor->pNext;
    }

    KQUEUE_ASSERT((pCursor != NULL || kPanic == FALSE),
                  "Could not find knode to remove",
                  OS_ERR_INCORRECT_VALUE);

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

    KERNEL_DEBUG(KQUEUE_DEBUG_ENABLED,
                 "KQUEUE",
                 "KQueue renoved knode 0x%p from kqueue 0x%p",
                 pNode, pQueue);
}

/************************************ EOF *************************************/