
/*******************************************************************************
 * @file queue_test.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 14/05/2023
 *
 * @version 1.0
 *
 * @brief Testing framework queue testing.
 *
 * @details Testing framework queue testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <queue.h>
#include <stdint.h>
#include <kheap.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <test_framework.h>


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
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void queue_test(void)
{
    OS_RETURN_E   error = OS_ERR_NULL_POINTER;
    queue_node_t* nodes[40] = { NULL };
    queue_node_t* find;
    queue_t*      queue = NULL;
    uint32_t      sorted[40];
    uint32_t      unsorted[10] = {0, 3, 5, 7, 4, 1, 8, 9, 6, 2};

    int8_t j = -1;
    for(uint32_t i = 0; i < 40; ++i)
    {
        if(i % 4 == 0)
        {
            ++j;
        }
        sorted[i] = j;
    }

    /* Create node */
    nodes[0] = queueCreateNode((void*) 0, QUEUE_ALLOCATOR(kmalloc, kfree), &error);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_CREATE_NODE0_ID,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_CREATE_NODE1_ID,
                             nodes[0] != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)nodes[0],
                             TEST_OS_QUEUE_ENABLED);

    /* Delete node */
    error = queueDeleteNode(&nodes[0]);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_DELETE_NODE0_ID,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_DELETE_NODE1_ID,
                             nodes[0] == NULL,
                             (uint64_t)(uintptr_t)NULL,
                             (uint64_t)(uintptr_t)nodes[0],
                             TEST_OS_QUEUE_ENABLED);

    error = OS_ERR_NULL_POINTER;

    /* Create node */
    nodes[0] = queueCreateNode((void*) 0, QUEUE_ALLOCATOR(kmalloc, kfree), &error);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_CREATE_NODE2_ID,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_CREATE_NODE3_ID,
                             nodes[0] != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)nodes[0],
                             TEST_OS_QUEUE_ENABLED);

    error = OS_ERR_NULL_POINTER;

    /* Create queue */
    queue = queueCreate(QUEUE_ALLOCATOR(kmalloc, kfree), &error);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_CREATE0_ID,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_CREATE1_ID,
                             queue != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)queue,
                             TEST_OS_QUEUE_ENABLED);

    /* Delete queue */
    error = queueDelete(&queue);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_DELETE0_ID,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_DELETE1_ID,
                             queue == NULL,
                             (uint64_t)(uintptr_t)NULL,
                             (uint64_t)(uintptr_t)queue,
                             TEST_OS_QUEUE_ENABLED);

    error = OS_ERR_NULL_POINTER;

    /* Create queue */
    queue = queueCreate(QUEUE_ALLOCATOR(kmalloc, kfree),&error);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_CREATE2_ID,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_CREATE3_ID,
                             queue != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)queue,
                             TEST_OS_QUEUE_ENABLED);

    /* Enqueue node */
    error = queuePush(nodes[0], queue);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_PUSH0_ID,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_OS_QUEUE_ENABLED);

    /* Delete node */
    error = queueDeleteNode(&nodes[0]);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_DELETE_NODE2_ID,
                            error == OS_ERR_UNAUTHORIZED_ACTION,
                            OS_ERR_UNAUTHORIZED_ACTION,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_DELETE_NODE3_ID,
                             nodes[0] != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)nodes[0],
                             TEST_OS_QUEUE_ENABLED);

    /* Enqueue NULL node */
    error = queuePush(NULL, queue);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_PUSH1_ID,
                            error == OS_ERR_NULL_POINTER,
                            OS_ERR_NULL_POINTER,
                            error,
                            TEST_OS_QUEUE_ENABLED);

    /* Delete queue */
    error = queueDelete(&queue);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_DELETE2_ID,
                            error == OS_ERR_UNAUTHORIZED_ACTION,
                            OS_ERR_UNAUTHORIZED_ACTION,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_DELETE3_ID,
                             queue != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)queue,
                             TEST_OS_QUEUE_ENABLED);

    error = OS_ERR_NULL_POINTER;

    /* Dequeue node */
    nodes[0] = queuePop(queue, &error);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_POP0_ID,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_POP1_ID,
                             nodes[0] != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)nodes[0],
                             TEST_OS_QUEUE_ENABLED);

    error = OS_ERR_NULL_POINTER;

    /* Create more nodes */
    for(uint8_t i = 0; i < 40; ++i)
    {
        nodes[i] = queueCreateNode((void*) (uintptr_t)unsorted[i % 10], QUEUE_ALLOCATOR(kmalloc, kfree), &error);
        TEST_POINT_ASSERT_RCODE(TEST_QUEUE_CREATE_NODEBURST0_ID(i * 2),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_OS_QUEUE_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_CREATE_NODEBURST0_ID(i * 2 + 1),
                                 nodes[i] != NULL,
                                 (uint64_t)1,
                                 (uint64_t)(uintptr_t)nodes[i],
                                 TEST_OS_QUEUE_ENABLED);
        error = OS_ERR_NULL_POINTER;
    }

    /* Enqueue nodes with prio */
    for(uint8_t i = 0; i < 40; ++i)
    {
        error = queuePushPrio(nodes[i], queue, (uintptr_t)nodes[i]->pData);
        TEST_POINT_ASSERT_RCODE(TEST_QUEUE_PUSHPRIOBURST0_ID(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_OS_QUEUE_ENABLED);
    }

    error = OS_ERR_NULL_POINTER;

    /* Dequeue nodes and check order */
    for(uint8_t i = 0; i < 40; ++i)
    {
        nodes[i] = queuePop(queue, &error);
        TEST_POINT_ASSERT_RCODE(TEST_QUEUE_POPBURST0_ID(i * 3),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_OS_QUEUE_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_POPBURST0_ID(i * 3 + 1),
                                 nodes[i] != NULL,
                                 (uint64_t)1,
                                 (uint64_t)(uintptr_t)nodes[i],
                                 TEST_OS_QUEUE_ENABLED);
        TEST_POINT_ASSERT_UINT(TEST_QUEUE_POPBURST0_ID(i * 3 + 2),
                                (uint32_t)(uintptr_t)nodes[i]->pData == sorted[i],
                                (uint32_t)sorted[i],
                                (uint32_t)(uintptr_t)nodes[i]->pData,
                                TEST_OS_QUEUE_ENABLED);

        error = OS_ERR_NULL_POINTER;
    }

    TEST_POINT_ASSERT_UINT(TEST_QUEUE_SIZE0_ID,
                            queue->size == 0,
                            0,
                            queue->size,
                            TEST_OS_QUEUE_ENABLED);


    /* Delete nodes */
    for(uint8_t i = 0; i < 40; ++i)
    {
        error = queueDeleteNode(&nodes[i]);
        TEST_POINT_ASSERT_RCODE(TEST_QUEUE_DELETENODEBURST0_ID(i * 2),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_OS_QUEUE_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_DELETENODEBURST0_ID(i * 2 + 1),
                                 nodes[i] == NULL,
                                 (uint64_t)(uintptr_t)NULL,
                                 (uint64_t)(uintptr_t)nodes[i],
                                 TEST_OS_QUEUE_ENABLED);
    }

    /* Create more nodes */
    for(uint8_t i = 0; i < 40; ++i)
    {
        nodes[i] = queueCreateNode((void*) (uintptr_t)unsorted[i % 10], QUEUE_ALLOCATOR(kmalloc, kfree), &error);
        TEST_POINT_ASSERT_RCODE(TEST_QUEUE_CREATE_NODEBURST1_ID(i * 2),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_OS_QUEUE_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_CREATE_NODEBURST1_ID(i * 2 + 1),
                                 nodes[i] != NULL,
                                 (uint64_t)1,
                                 (uint64_t)(uintptr_t)nodes[i],
                                 TEST_OS_QUEUE_ENABLED);

        error = OS_ERR_NULL_POINTER;
    }

    /* Enqueue without prio */
    for(uint8_t i = 0; i < 40; ++i)
    {
        error = queuePush(nodes[i], queue);
        TEST_POINT_ASSERT_RCODE(TEST_QUEUE_PUSHBURST0_ID(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_OS_QUEUE_ENABLED);
    }

    error = OS_ERR_NULL_POINTER;

    /* Find a present node */
    find =  queueFind(queue, (void*) 9, &error);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_CREATE_FIND0_ID,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_CREATE_FIND1_ID,
                             find != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)find,
                             TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_QUEUE_CREATE_FIND2_ID,
                            (uint32_t)(uintptr_t)find->pData == 9,
                            9,
                            (uint32_t)(uintptr_t)find->pData,
                            TEST_OS_QUEUE_ENABLED);

    error = OS_ERR_NULL_POINTER;

    /* Find a not present node */
    find = queueFind(queue, (void*) 42, &error);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_CREATE_FIND3_ID,
                            error == OS_ERR_INCORRECT_VALUE,
                            OS_ERR_INCORRECT_VALUE,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_CREATE_FIND4_ID,
                             find == NULL,
                             (uint64_t)(uintptr_t)NULL,
                             (uint64_t)(uintptr_t)find,
                             TEST_OS_QUEUE_ENABLED);

    error = OS_ERR_NULL_POINTER;

    /* Dequeue nodes and check "non order" */
    for(uint8_t i = 0; i < 40; ++i)
    {
        nodes[i] = queuePop(queue, &error);
        TEST_POINT_ASSERT_RCODE(TEST_QUEUE_POPBURST1_ID(i * 3),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_OS_QUEUE_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_POPBURST1_ID(i * 3 + 1),
                                 nodes[i] != NULL,
                                 (uint64_t)1,
                                 (uint64_t)(uintptr_t)nodes[i],
                                 TEST_OS_QUEUE_ENABLED);
        TEST_POINT_ASSERT_UINT(TEST_QUEUE_POPBURST1_ID(i * 3 + 2),
                                (uint32_t)(uintptr_t)nodes[i]->pData == unsorted[i%10],
                                (uint32_t)unsorted[i%10],
                                (uint32_t)(uintptr_t)nodes[i]->pData,
                                TEST_OS_QUEUE_ENABLED);

        error = OS_ERR_NULL_POINTER;
    }

    TEST_POINT_ASSERT_UINT(TEST_QUEUE_SIZE1_ID,
                            queue->size == 0,
                            0,
                            queue->size,
                            TEST_OS_QUEUE_ENABLED);

    /* Dequeue node on empty queue */
    find = queuePop(queue, &error);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_POP2_ID,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_POP3_ID,
                             find == NULL,
                             (uint64_t)(uintptr_t)NULL,
                             (uint64_t)(uintptr_t)find,
                             TEST_OS_QUEUE_ENABLED);

    /* Delete queue */
    error = queueDelete(&queue);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_DELETE4_ID,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_DELETE5_ID,
                             queue == NULL,
                             (uint64_t)(uintptr_t)NULL,
                             (uint64_t)(uintptr_t)queue,
                             TEST_OS_QUEUE_ENABLED);

    /* Enqueue node on NULL queue */
    error = queuePush(nodes[0], queue);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_PUSH2_ID,
                            error == OS_ERR_NULL_POINTER,
                            OS_ERR_NULL_POINTER,
                            error,
                            TEST_OS_QUEUE_ENABLED);

    /* Dequeue node on NULL queue */
    find = queuePop(queue, &error);
    TEST_POINT_ASSERT_RCODE(TEST_QUEUE_POP4_ID,
                            error == OS_ERR_NULL_POINTER,
                            OS_ERR_NULL_POINTER,
                            error,
                            TEST_OS_QUEUE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_POP5_ID,
                             find == NULL,
                             (uint64_t)(uintptr_t)NULL,
                             (uint64_t)(uintptr_t)find,
                             TEST_OS_QUEUE_ENABLED);

    /* Delete nodes */
    for(uint8_t i = 0; i < 40; ++i)
    {
        error = queueDeleteNode(&nodes[i]);
        TEST_POINT_ASSERT_RCODE(TEST_QUEUE_DELETENODEBURST1_ID(i * 2),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_OS_QUEUE_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_QUEUE_DELETENODEBURST1_ID(i * 2 + 1),
                                 nodes[i] == NULL,
                                 (uint64_t)(uintptr_t)NULL,
                                 (uint64_t)(uintptr_t)nodes[i],
                                 TEST_OS_QUEUE_ENABLED);
    }
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/