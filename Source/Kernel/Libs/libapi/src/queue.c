/*******************************************************************************
 * @file queue.c
 *
 * @see queue.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/05/2023
 *
 * @version 1.5
 *
 * @brief Queue structures.
 *
 * @details Queue structures. These queues are used as priority queue or regular
 * queues. A queue can virtually store every type of data and is just a wrapper.
 * This queue library is thread safe.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <panic.h>        /* Kernel panic */
#include <stddef.h>       /* Standard definitions */
#include <stdint.h>       /* Generic int types */
#include <string.h>       /* String manipulation */
#include <kerneloutput.h> /* Kernel output methods */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <queue.h>

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

queue_node_t* queueCreateNode(void*         pData,
                              queue_alloc_t allocator,
                              OS_RETURN_E*  pError)
{
    queue_node_t* pNewNode;

    /* Create new node */
    pNewNode = allocator.pMalloc(sizeof(queue_node_t));

    if(pNewNode == NULL)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_NO_MORE_MEMORY;
        }
        return NULL;
    }

    /* Init the structure */
    memset(pNewNode, 0, sizeof(queue_node_t));
    pNewNode->allocator = allocator;
    pNewNode->pData     = pData;
    if(pError != NULL)
    {
        *pError = OS_NO_ERR;
    }
    return pNewNode;
}

OS_RETURN_E queueDeleteNode(queue_node_t** ppNode)
{
    if(ppNode == NULL || *ppNode == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Check queue chaining */
    if((*ppNode)->enlisted != FALSE)
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    (*ppNode)->allocator.pFree(*ppNode);
    *ppNode = NULL;

    return OS_NO_ERR;
}

queue_t* queueCreate(queue_alloc_t allocator, OS_RETURN_E* pEerror)
{
    queue_t* newqueue;

    /* Create new node */
    newqueue = allocator.pMalloc(sizeof(queue_t));
    if(newqueue == NULL)
    {
        if(pEerror != NULL)
        {
            *pEerror = OS_ERR_NO_MORE_MEMORY;
        }
        return NULL;
    }

    /* Init the structure */
    memset(newqueue, 0, sizeof(queue_t));
    newqueue->allocator = allocator;

    if(pEerror != NULL)
    {
        *pEerror = OS_NO_ERR;
    }

    return newqueue;
}

OS_RETURN_E queueDelete(queue_t** ppQueue)
{
    if(ppQueue == NULL || *ppQueue == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Check queue chaining */
    if((*ppQueue)->pHead != NULL || (*ppQueue)->pTail != NULL)
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    (*ppQueue)->allocator.pFree(*ppQueue);
    *ppQueue = NULL;

    return OS_NO_ERR;
}

OS_RETURN_E queuePush(queue_node_t* pNode, queue_t* pQueue)
{
    KERNEL_DEBUG(QUEUE_DEBUG_ENABLED,
                 "QUEUE",
                 "Queue 0x%p in queue 0x%p",
                 pNode,
                 pQueue);

    if(pNode == NULL || pQueue == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

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

    KERNEL_DEBUG(QUEUE_DEBUG_ENABLED,
                 "QUEUE",
                 "Enqueue element 0x%p in queue 0x%p",
                 pNode,
                 pQueue);

    if(pNode->pNext != NULL &&
       pNode->pPrev != NULL &&
       pNode->pNext == pNode->pPrev)
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    return OS_NO_ERR;
}


OS_RETURN_E queuePushPrio(queue_node_t*   pNode,
                            queue_t*        pQueue,
                            const uintptr_t kPriority)
{
    queue_node_t* pCursor;

    KERNEL_DEBUG(QUEUE_DEBUG_ENABLED,
                 "QUEUE",
                 "Enqueue 0x%p in queue 0x%p",
                 pNode,
                 pQueue);

    if(pNode == NULL || pQueue == NULL)
    {
        KERNEL_ERROR("[QUEUE] Enqueue NULL");
        return OS_ERR_NULL_POINTER;
    }

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

    KERNEL_DEBUG(QUEUE_DEBUG_ENABLED,
                 "QUEUE",
                 "Enqueue element 0x%p in queue 0x%p",
                 pNode,
                 pQueue);

    if(pNode->pNext != NULL &&
       pNode->pPrev != NULL &&
       pNode->pNext == pNode->pPrev)
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    return OS_NO_ERR;
}

queue_node_t* queuePop(queue_t* pQueue, OS_RETURN_E* pError)
{
    queue_node_t* pNode;

    KERNEL_DEBUG(QUEUE_DEBUG_ENABLED,
                 "QUEUE",
                 "Dequeue element in queue 0x%p",
                 pQueue);

    if(pQueue == NULL)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_NULL_POINTER;
        }
        return NULL;
    }

    if(pError != NULL)
    {
        *pError = OS_NO_ERR;
    }

    /* If this queue is empty */
    if(pQueue->pHead == NULL)
    {
        return NULL;
    }

    /* Dequeue the last item */
    pNode = pQueue->pTail;

    KERNEL_DEBUG(QUEUE_DEBUG_ENABLED,
                 "QUEUE",
                 "Dequeue element 0x%p in queue 0x%p",
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

    if(pError != NULL)
    {
        *pError = OS_NO_ERR;
    }

    return pNode;
}

queue_node_t* queueFind(queue_t*pQueue, void* pData, OS_RETURN_E* pError)
{
    queue_node_t* pNode;

    KERNEL_DEBUG(QUEUE_DEBUG_ENABLED,
                 "QUEUE",
                 "Find data 0x%p in queue 0x%p",
                 pData,
                 pQueue);

    /* If this queue is empty */
    if(pQueue == NULL)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_NULL_POINTER;
        }
        return NULL;
    }

    /* Search for the data */
    pNode = pQueue->pHead;
    while(pNode != NULL && pNode->pData != pData)
    {
        pNode = pNode->pNext;
    }

    /* No such data */
    if(pNode == NULL)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_INCORRECT_VALUE;
        }
        return NULL;
    }

    if(pError != NULL)
    {
        *pError = OS_NO_ERR;
    }

    return pNode;
}

OS_RETURN_E queueRemove(queue_t* pQueue, queue_node_t* pNode)
{
    queue_node_t* pCursor;

    if(pQueue == NULL || pNode == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    KERNEL_DEBUG(QUEUE_DEBUG_ENABLED,
                 "QUEUE",
                 "Remove node node 0x%p in queue 0x%p",
                 pNode,
                 pQueue);

    /* Search for node in the queue*/
    pCursor = pQueue->pHead;
    while(pCursor != NULL && pCursor != pNode)
    {
        pCursor = pCursor->pNext;
    }

    if(pCursor == NULL)
    {
        return OS_ERR_INCORRECT_VALUE;
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

    return OS_NO_ERR;
}

/************************************ EOF *************************************/