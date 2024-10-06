/*******************************************************************************
 * @file queue.h
 *
 * @see queue.c
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
 * This queue library is not thread safe.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_QUEUE_H_
#define __LIB_QUEUE_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stddef.h>    /* Standard definitons */
#include <stdint.h>    /* Generic int types */
#include <kerror.h>    /* Kernel error */
#include <stdbool.h>   /* Bool types */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Queue allocator structure. */
typedef struct
{
    /**
     * @brief The memory allocation function used by the allocator.
     *
     * @param[in] allocSize The size in bytes to be allocated.
     *
     * @return A pointer to the allocated memory is returned. NULL is returned
     * if no memory was allocated.
     */
    void*(*pMalloc)(size_t allocSize);

    /**
     * @brief The memory free function used by the allocator.
     *
     * @param[out] ptr The start address of the memory to free.
     */
    void(*pFree)(void* ptr);
} queue_alloc_t;

/** @brief Queue node structure. */
typedef struct queue_node
{
    /** @brief The allocator used by this node. */
    queue_alloc_t allocator;

    /** @brief Next node in the queue. */
    struct queue_node* pNext;
    /** @brief Previous node in the queue. */
    struct queue_node* pPrev;

    /** @brief Tell if the node is present in a queue or stands alone. */
    bool enlisted;

    /** @brief Node's priority, used when the queue is a priority queue. */
    uint64_t priority;

    /** @brief Node's data pointer. Store the address of the contained data. */
    void* pData;
} queue_node_t;

/** @brief Queue structure. */
typedef struct
{
    /** @brief The allocator used by this queue. */
    queue_alloc_t allocator;

    /** @brief Head of the queue. */
    queue_node_t* pHead;
    /** @brief Tail of the queue. */
    queue_node_t* pTail;

    /** @brief Current queue's size. */
    size_t size;
} queue_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Create an allocator structure.
 *
 * @param[in] MALLOC The memory allocation function used by the allocator.
 * @param[in] FREE The memory free function used by the allocator.
 */
#define QUEUE_ALLOCATOR(MALLOC, FREE) (queue_alloc_t){MALLOC, FREE}

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
 * @brief Creates a new queue node.
 *
 * @details Creates a node ready to be inserted in a queue. The data can be
 * modified later by accessing the data field of the node structure.
 *
 * @warning A node should be only used in one queue at most.
 *
 * @param[in] pData The pointer to the data to carry in the node.
 * @param[in] allocator The allocator to be used when allocating and freeing the
 * node.
 * @param[out] pError A pointer to the variable that contains the function
 * success state. May be NULL. The error values can be the following:
 * - OS_ERR_NO_MORE_MEMORY if the allocator failed to allocate memory for the node.
 * - OS_NO_ERR if no error is encountered.
 *
 * @return The node pointer is returned. In case of error NULL is returned.
 */
queue_node_t* queueCreateNode(void*         pData,
                              queue_alloc_t allocator,
                              OS_RETURN_E*  pError);

/**
 * @brief Deletes a queue node.
 *
 * @details Deletes a node from the memory. The node should not be used in any
 * queue. If it is the case, the function will return an error.
 *
 * @param[in, out] ppNode The node pointer of pointer to destroy.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_NULL_POINTER is returned if the pointer to the node is NULL.
 * - OS_ERR_UNAUTHORIZED_ACTION is returned if the node is still used in a list.
 */
OS_RETURN_E queueDeleteNode(queue_node_t** ppNode);

/**
 * @brief Creates an empty queue ready to be used.
 *
 * @details Creates and initializes a new queue. The returned queue is
 * ready to be used.
 *
 * @param[in] allocator The allocator to be used when allocating and freeing the
 * queue.
 * @param[out] pError A pointer to the variable that will contain the function
 * success state. May be NULL. The error values can be the following:
 * - OS_ERR_NO_MORE_MEMORY if the allocator failed to allocate memory for the
 * queue.
 * - OS_NO_ERR if no error is encountered.
 *
 * @return The queue pointer is returned. In case of error NULL is returned.
 */
queue_t* queueCreate(queue_alloc_t allocator, OS_RETURN_E* pError);

/**
 * @brief Deletes a previously created queue.
 *
 * @details Delete a queue from the memory. If the queue is not empty an error
 * is returned.
 *
 * @param[in, out] ppQueue The queue pointer of pointer to destroy.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_NULL_POINTER is returned if the pointer to the queue is NULL.
 * - OS_ERR_UNAUTHORIZED_ACTION is returned if the queue is not empty.
 */
OS_RETURN_E queueDelete(queue_t** ppQueue);

/**
 * @brief Enlists a node in the queue.
 *
 * @details Enlists a node in the queue given as parameter. The data will be
 * placed in the tail of the queue.
 *
 * @param[in] pNode A now node to add in the queue.
 * @param[in, out] pQueue The queue to manage.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_NULL_POINTER is returned if the pointer to the queue or the node is
 *   NULL.
 */
OS_RETURN_E queuePush(queue_node_t* pNode, queue_t* pQueue);

/**
 * @brief Enlists a node in the queue.
 *
 * @details Enlist a node in the queue given as parameter. The data will be
 * placed in the queue with regard to the priority argument.
 *
 * @param[in] pNode A now node to add in the queue.
 * @param[in, out] pQueue The queue to manage.
 * @param[in] kPriority The element priority.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_NULL_POINTER is returned if the pointer to the queue or the node is
 *   NULL.
 */
OS_RETURN_E queuePushPrio(queue_node_t*  pNode,
                          queue_t*       pQueue,
                          const uint64_t kPriority);

/**
 * @brief Removes a node from a queue.
 *
 * @details Removes a node from the queue given as parameter. The retreived node
 * that is returned is the one placed in the head of the QUEUE.
 *
 * @param[in, out] pQueue The queue to manage.
 * @param[out] pError A pointer to the variable that contains the function
 * success state. May be NULL. The error values can be the following:
 * - OS_ERR_NULL_POINTER is returned if the pointer to the queue or the node is
 *   NULL.
 * - OS_NO_ERR if no error is encountered.
 *
 * @return The data pointer placed in the head of the queue is returned.
 */
queue_node_t* queuePop(queue_t* pQueue, OS_RETURN_E* pError);

/**
 * @brief Finds a node containing the data given as parameter in the queue.
 *
 * @details Find a node containing the data given as parameter in the queue.
 * An error is set if not any node is found.
 *
 * @param[in] pQueue The queue to search the data in.
 * @param[in] pData The data contained by the node to find.
 * @param[out] pError A pointer to the variable that contains the function
 * successstate. May be NULL. The error values can be the following:
 * - OS_ERR_NULL_POINTER is returned if the pointer to the queue or the node is
 *   NULL.
 * - OS_ERR_INCORRECT_VALUE is returned if the data could not be found in the
 * queue.
 * - OS_NO_ERR if no error is encountered.
 *
 * @return The function returns a pointer to the node if found, NULL otherwise.
 */
queue_node_t* queueFind(queue_t* pQueue, void* pData, OS_RETURN_E* pError);

/**
 * @brief Removes a node from a queue.
 *
 * @details Removes a node from a queue given as parameter. If the node is not
 * found, nothing is done and an error is returned.
 *
 * @param[in, out] pQueue The queue containing the node.
 * @param[in] pNode The node to remove.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_NULL_POINTER is returned if the pointer to the queue or the node is
 *   NULL.
 * - OS_ERR_INCORRECT_VALUE is returned if the data could not be found in the
 * queue.
 */
OS_RETURN_E queueRemove(queue_t* pQueue, queue_node_t* pNode);

#endif /* #ifndef __LIB_QUEUE_H_ */

/************************************ EOF *************************************/