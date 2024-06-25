/*******************************************************************************
 * @file kqueue.h
 *
 * @see kqueue.c
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

#ifndef __CORE_KQUEUE_H_
#define __CORE_KQUEUE_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stddef.h>    /* Standard definitons */
#include <stdint.h>    /* Generic int types */
#include <critical.h>  /* Kernel locks */
/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Queue node structure. */
typedef struct kqueue_node
{
    /** @brief Next node in the queue. */
    struct kqueue_node* pNext;
    /** @brief Previous node in the queue. */
    struct kqueue_node* pPrev;

    /** @brief Tell if the node is present in a queue or stands alone. */
    volatile bool_t enlisted;

    /** @brief Node's priority, used when the queue is a priority queue. */
    volatile uint64_t priority;

    /** @brief Node's data pointer. Store the address of the contained data. */
    void* pData;
} kqueue_node_t;

/** @brief Queue structure. */
typedef struct
{
    /** @brief Head of the queue. */
    kqueue_node_t* pHead;
    /** @brief Tail of the queue. */
    kqueue_node_t* pTail;

    /** @brief Current queue's size. */
    volatile size_t size;

    /** @brief Queue's lock */
    kernel_spinlock_t lock;
} kqueue_t;

/*******************************************************************************
 * MACROS
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

/**
 * @brief Creates a new queue node.
 *
 * @details Creates a node ready to be inserted in a queue. The data can be
 * modified later by accessing the data field of the node structure.
 *
 * @warning A node should be only used in one queue at most.
 *
 * @param[in] pData The pointer to the data to carry in the node.
 * @param[in] kIsCritical Tells if the creation of the queue is critical. If it
 * is TRUE and the queue cannot be created, a kernel panic is generated.
 *
 * @return The node pointer is returned.
 */
kqueue_node_t* kQueueCreateNode(void* pData, const bool_t kIsCritical);

/**
 * @brief Initializes a new queue node.
 *
 * @details Initializes a node ready to be inserted in a queue. The data can be
 * modified later by accessing the data field of the node structure.
 *
 * @warning A node should be only used in one queue at most.
 *
 * @param[in] pNode The pointer to the node to initialize to carry in the node.
 * @param[in] pData The pointer to the data to carry in the node.
 * @param[in] kIsCritical Tells if the creation of the queue is critical. If it
 * is TRUE and the queue cannot be created, a kernel panic is generated.
 */
void kQueueInitNode(kqueue_node_t* pNode,
                    void*          pData);

/**
 * @brief Deletes a queue node.
 *
 * @details Deletes a node from the memory. The node should not be used in any
 * queue. If it is the case, the function will return an error.
 *
 * @param[in, out] ppNode The node pointer of pointer to destroy.
 */
void kQueueDestroyNode(kqueue_node_t** ppNode);

/**
 * @brief Creates an empty queue ready to be used.
 *
 * @details Creates and initializes a new queue. The returned queue is
 * ready to be used.
 *
 * @param[in] kIsCritical Tells if the creation of the queue is critical. If it
 * is TRUE and the queue cannot be created, a kernel panic is generated.
 */
kqueue_t* kQueueCreate(const bool_t kIsCritical);

/**
 * @brief Deletes a previously created queue.
 *
 * @details Delete a queue from the memory. If the queue is not empty an error
 * is returned.
 *
 * @param[in, out] ppQueue The queue pointer of pointer to destroy.
 */
void kQueueDestroy(kqueue_t** ppQueue);

/**
 * @brief Enlists a node in the queue.
 *
 * @details Enlists a node in the queue given as parameter. The data will be
 * placed in the tail of the queue.
 *
 * @param[in, out] pNode A now node to add in the queue.
 * @param[in, out] pQueue The queue to manage.
 */
void kQueuePush(kqueue_node_t* pNode, kqueue_t* pQueue);

/**
 * @brief Enlists a node in the queue.
 *
 * @details Enlist a node in the queue given as parameter. The data will be
 * placed in the queue with regard to the priority argument.
 *
 * @param[in, out] pNode A now node to add in the queue.
 * @param[in, out] pQueue The queue to manage.
 * @param[in] kPriority The element priority.
 */
void kQueuePushPrio(kqueue_node_t* pNode,
                    kqueue_t*      pQueue,
                    const uint64_t kPriority);

/**
 * @brief Removes a node from a queue.
 *
 * @details Removes a node from the queue given as parameter. The retreived node
 * that is returned is the one placed in the head of the QUEUE.
 *
 * @param[in, out] pQueue The queue to manage.
 *
 * @return The data pointer placed in the head of the queue is returned.
 */
kqueue_node_t* kQueuePop(kqueue_t* pQueue);

/**
 * @brief Finds a node containing the data given as parameter in the queue.
 *
 * @details Find a node containing the data given as parameter in the queue.
 * An error is set if not any node is found.
 *
 * @param[in] pQueue The queue to search the data in.
 * @param[in] data The data contained by the node to find.
 *
 * @return The function returns a pointer to the node if found, NULL otherwise.
 */
kqueue_node_t* kQueueFind(kqueue_t* pQueue, const void* kpData);

/**
 * @brief Removes a node from a queue.
 *
 * @details Removes a node from a queue given as parameter. If the node is not
 * found, nothing is done and an error is returned.
 *
 * @param[in, out] pQueue The queue containing the node.
 * @param[in, out] pNode The node to remove.
 * @param[in] kPanic Tells if the function should generate a kernel panic if the
 * node is not found.
 */
void kQueueRemove(kqueue_t* pQueue, kqueue_node_t* pNode, const bool_t kPanic);


#endif /* #ifndef __CORE_KQUEUE_H_ */

/************************************ EOF *************************************/