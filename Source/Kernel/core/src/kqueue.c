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
#include <syslog.h>       /* Kernel Syslog */
#include <stdbool.h>      /* Bool types */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <kqueue.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "KQUEUE"

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
    if((COND) == false)                                      \
    {                                                        \
        PANIC(ERROR, MODULE_NAME, MSG);                      \
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

kqueue_node_t* kQueueCreateNode(void* pData, const bool kIsCritical)
{
    kqueue_node_t* pNewNode;

    /* Create new node */
    pNewNode = kmalloc(sizeof(kqueue_node_t));
    if(pNewNode == NULL)
    {
        if(kIsCritical == true)
        {
            KQUEUE_ASSERT(false,
                          "Could not allocate knode",
                          OS_ERR_NO_MORE_MEMORY);
            return NULL;
        }
        else
        {
            return NULL;
        }
    }


    /* Init the structure */
    memset(pNewNode, 0, sizeof(kqueue_node_t));
    pNewNode->pData = pData;

    return pNewNode;
}

void kQueueInitNode(kqueue_node_t* pNode, void* pData)
{

    KQUEUE_ASSERT(pNode != NULL,
                  "Initializes a NULL node",
                  OS_ERR_NULL_POINTER);

    memset(pNode, 0, sizeof(kqueue_node_t));
    pNode->pData = pData;
}

void kQueueDestroyNode(kqueue_node_t** ppNode)
{

    KQUEUE_ASSERT((ppNode != NULL && *ppNode != NULL),
                  "Tried to delete a NULL node",
                  OS_ERR_NULL_POINTER);

    KQUEUE_ASSERT((*ppNode)->pQueuePtr == NULL,
                  "Tried to delete an enlisted node",
                  OS_ERR_UNAUTHORIZED_ACTION);

    kfree(*ppNode);
    *ppNode = NULL;
}

kqueue_t* kQueueCreate(const bool kIsCritical)
{
    kqueue_t* pNewQueue;

    /* Create new node */
    pNewQueue = kmalloc(sizeof(kqueue_t));
    if(pNewQueue == NULL)
    {
        if(kIsCritical == true)
        {
            KQUEUE_ASSERT(false,
                          "Could not allocate kqueue",
                          OS_ERR_NO_MORE_MEMORY);
        }
        else
        {
            return NULL;
        }
    }

    /* Init the structure */
    memset(pNewQueue, 0, sizeof(kqueue_t));
    KERNEL_SPINLOCK_INIT(pNewQueue->lock);

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

#if KQUEUE_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "KQueue try push knode 0x%p in kqueue 0x%p",
           pNode,
           pQueue);
#endif

    KQUEUE_ASSERT((pNode != NULL && pQueue != NULL),
                  "Cannot push with NULL knode or NULL kqueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_LOCK(pQueue->lock);

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
    pNode->pQueuePtr = pQueue;


    KQUEUE_ASSERT((pNode->pNext != pNode->pPrev ||
                   pNode->pNext == NULL ||
                   pNode->pPrev == NULL),
                  "Cycle detected in KQueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_UNLOCK(pQueue->lock);

#if KQUEUE_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "KQueue pushed knode 0x%p in kqueue 0x%p",
           pNode,
           pQueue);
#endif
}


void kQueuePushPrio(kqueue_node_t* pNode,
                    kqueue_t*      pQueue,
                    const uint64_t kPriority)
{
    kqueue_node_t* pCursor;

#if KQUEUE_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "KQueue try push prio knode 0x%p in kqueue 0x%p",
           pNode,
           pQueue);
#endif

    KQUEUE_ASSERT((pNode != NULL && pQueue != NULL),
                  "Cannot push with NULL knode or NULL kqueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_LOCK(pQueue->lock);

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
    pNode->pQueuePtr = pQueue;

    KQUEUE_ASSERT((pNode->pNext != pNode->pPrev ||
                   pNode->pNext == NULL ||
                   pNode->pPrev == NULL),
                  "Cycle detected in KQueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_UNLOCK(pQueue->lock);

#if KQUEUE_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "KQueue pushed knode 0x%p in kqueue 0x%p",
           pNode,
           pQueue);
#endif
}

kqueue_node_t* kQueuePop(kqueue_t* pQueue)
{
    kqueue_node_t* pNode;

#if KQUEUE_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "KQueue try pop knode from kqueue 0x%p",
           pQueue);
#endif

    KQUEUE_ASSERT(pQueue != NULL,
                  "Cannot pop NULL kqueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_LOCK(pQueue->lock);

    /* If this queue is empty */
    if(pQueue->pHead == NULL)
    {
        KERNEL_UNLOCK(pQueue->lock);
        return NULL;
    }

    /* Dequeue the last item */
    pNode = pQueue->pTail;

#if KQUEUE_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Pop knode 0x%p from kqueue 0x%p",
           pNode,
           pQueue);
#endif

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

    pNode->pQueuePtr = NULL;

    KERNEL_UNLOCK(pQueue->lock);

    return pNode;
}

kqueue_node_t* kQueueFind(kqueue_t* pQueue, const void* kpData)
{
    kqueue_node_t* pNode;

#if KQUEUE_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "KQueue try find data 0x%p from kqueue 0x%p",
           kpData,
           pQueue);
#endif

    KQUEUE_ASSERT(pQueue != NULL,
                  "Cannot find in NULL kqueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_LOCK(pQueue->lock);

    /* Search for the data */
    pNode = pQueue->pHead;
    while(pNode != NULL && pNode->pData != kpData)
    {
        pNode = pNode->pNext;
    }

#if KQUEUE_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "KQueue found node 0x%p from kqueue 0x%p",
           pNode,
           pQueue);
#endif

    KERNEL_UNLOCK(pQueue->lock);

    return pNode;
}

void kQueueRemove(kqueue_t* pQueue, kqueue_node_t* pNode, const bool kPanic)
{
#if KQUEUE_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "KQueue try renove knode 0x%p from kqueue 0x%p",
           pNode,
           pQueue);
#endif

    KQUEUE_ASSERT((pNode != NULL && pQueue != NULL),
                  "Cannot remove with NULL knode or NULL kqueue",
                  OS_ERR_NULL_POINTER);

    KERNEL_LOCK(pQueue->lock);

    if(pNode->pQueuePtr != pQueue)
    {
        if(kPanic == true)
        {
            KQUEUE_ASSERT(false,
                          "Could not find knode to remove",
                          OS_ERR_INCORRECT_VALUE);
        }
        KERNEL_UNLOCK(pQueue->lock);
        return;
    }

    /* Manage link */
    if(pNode->pPrev != NULL && pNode->pNext != NULL)
    {
        pNode->pPrev->pNext = pNode->pNext;
        pNode->pNext->pPrev = pNode->pPrev;
    }
    else if(pNode->pPrev == NULL && pNode->pNext != NULL)
    {
        pQueue->pHead = pNode->pNext;
        pNode->pNext->pPrev = NULL;
    }
    else if(pNode->pPrev != NULL && pNode->pNext == NULL)
    {
        pQueue->pTail = pNode->pPrev;
        pNode->pPrev->pNext = NULL;
    }
    else
    {
        pQueue->pHead = NULL;
        pQueue->pTail = NULL;
    }

    pNode->pNext = NULL;
    pNode->pPrev = NULL;

    pNode->pQueuePtr = NULL;

    --pQueue->size;

    KERNEL_UNLOCK(pQueue->lock);

#if KQUEUE_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "KQueue renoved knode 0x%p from kqueue 0x%p",
           pNode, pQueue);
#endif
}

size_t kQueueSize(const kqueue_t* kpQueue)
{
    KQUEUE_ASSERT(kpQueue != NULL,
                  "Cannot get size of NULL kqueue",
                  OS_ERR_NULL_POINTER);

    return kpQueue->size;
}

/************************************ EOF *************************************/