
/*******************************************************************************
 * @file kqueue_test.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 14/05/2023
 *
 * @version 1.0
 *
 * @brief Testing framework kernel queue testing.
 *
 * @details Testing framework kernel queue testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <kqueue.h>
#include <panic.h>
#include <kerneloutput.h>

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

void kqueue_test(void)
{
    kqueue_node_t* nodes[40] = { NULL };
    kqueue_node_t* find;
    kqueue_t*      queue = NULL;
    uint32_t       old_size;
    uint32_t       sorted[40];
    uint32_t       unsorted[10] = {0, 3, 5, 7, 4, 1, 8, 9, 6, 2};

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
    nodes[0] = kqueue_create_node((void*) 0);
    TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_CREATE_NODE0_ID,
                             nodes[0] != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)nodes[0],
                             TEST_OS_KQUEUE_ENABLED);

    /* Delete node */
    kqueue_delete_node(&nodes[0]);
    TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_DELETE_NODE0_ID,
                             nodes[0] == NULL,
                             (uint64_t)(uintptr_t)NULL,
                             (uint64_t)(uintptr_t)nodes[0],
                             TEST_OS_KQUEUE_ENABLED);

    /* Create node */
    nodes[0] = kqueue_create_node((void*) 0);
    TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_CREATE_NODE1_ID,
                             nodes[0] != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)nodes[0],
                             TEST_OS_KQUEUE_ENABLED);

    /* Create queue */
    queue = kqueue_create_queue();
    TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_CREATE_QUEUE0_ID,
                             queue != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)queue,
                             TEST_OS_KQUEUE_ENABLED);

    /* Delete queue */
    kqueue_delete_queue(&queue);
    TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_DELETE_QUEUE0_ID,
                             queue == NULL,
                             (uint64_t)(uintptr_t)NULL,
                             (uint64_t)(uintptr_t)queue,
                             TEST_OS_KQUEUE_ENABLED);

    /* Create queue */
    queue = kqueue_create_queue();
    TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_CREATE_QUEUE1_ID,
                             queue != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)queue,
                             TEST_OS_KQUEUE_ENABLED);

    /* Enqueue node */
    old_size = queue->size;
    kqueue_push(nodes[0], queue);
    TEST_POINT_ASSERT_UINT(TEST_KQUEUE_PUSH0_ID,
                           old_size + 1 == queue->size,
                           old_size + 1,
                           queue->size,
                           TEST_OS_KQUEUE_ENABLED);

    /* Dequeue node */
    nodes[0] = kqueue_pop(queue);
    TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_POP0_ID,
                             nodes[0] != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)nodes[0],
                             TEST_OS_KQUEUE_ENABLED);

    /* Create more nodes */
    for(uint8_t i = 0; i < 40; ++i)
    {
        nodes[i] = kqueue_create_node((void*) (uintptr_t)unsorted[i % 10]);
        TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_CREATE_NODEBURST0_ID(i),
                                 nodes[i] != NULL,
                                 (uint64_t)1,
                                 (uint64_t)(uintptr_t)nodes[i],
                                 TEST_OS_KQUEUE_ENABLED);
    }

    /* Enqueue nodes with prio */
    for(uint8_t i = 0; i < 40; ++i)
    {
        old_size = queue->size;
        kqueue_push_prio(nodes[i], queue, (uintptr_t)nodes[i]->data);
        TEST_POINT_ASSERT_UINT(TEST_KQUEUE_PUSHPRIOBURST0_ID(i),
                               old_size + 1 == queue->size,
                               old_size + 1,
                               queue->size,
                               TEST_OS_KQUEUE_ENABLED);
    }

    /* Dequeue nodes and check order */
    for(uint8_t i = 0; i < 40; ++i)
    {
        old_size = queue->size;
        nodes[i] = kqueue_pop(queue);
        TEST_POINT_ASSERT_UINT(TEST_KQUEUE_POPBURST0_ID(i * 3),
                               old_size - 1 == queue->size,
                               old_size - 1,
                               queue->size,
                               TEST_OS_KQUEUE_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_POPBURST0_ID(i * 3 + 1),
                                 nodes[i] != NULL,
                                 (uint64_t)1,
                                 (uint64_t)(uintptr_t)nodes[i],
                                 TEST_OS_KQUEUE_ENABLED);
        TEST_POINT_ASSERT_UINT(TEST_KQUEUE_POPBURST0_ID(i * 3 + 2),
                                (uint32_t)(uintptr_t)nodes[i]->data == sorted[i],
                                (uint32_t)sorted[i],
                                (uint32_t)(uintptr_t)nodes[i]->data,
                                TEST_OS_KQUEUE_ENABLED);
    }

    TEST_POINT_ASSERT_UINT(TEST_KQUEUE_SIZE0_ID,
                            queue->size == 0,
                            0,
                            queue->size,
                            TEST_OS_KQUEUE_ENABLED);


    /* Delete nodes */
    for(uint8_t i = 0; i < 40; ++i)
    {
        kqueue_delete_node(&nodes[i]);
        TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_DELETENODEBURST0_ID(i),
                                 nodes[i] == NULL,
                                 (uint64_t)(uintptr_t)NULL,
                                 (uint64_t)(uintptr_t)nodes[i],
                                 TEST_OS_KQUEUE_ENABLED);
    }

    /* Create more nodes */
    for(uint8_t i = 0; i < 40; ++i)
    {
        nodes[i] = kqueue_create_node((void*) (uintptr_t)unsorted[i % 10]);
        TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_CREATE_NODEBURST1_ID(i),
                                 nodes[i] != NULL,
                                 (uint64_t)1,
                                 (uint64_t)(uintptr_t)nodes[i],
                                 TEST_OS_KQUEUE_ENABLED);
    }

    /* Enqueue without prio */
    for(uint8_t i = 0; i < 40; ++i)
    {
        old_size = queue->size;
        kqueue_push(nodes[i], queue);
        TEST_POINT_ASSERT_UINT(TEST_KQUEUE_PUSHBURST0_ID(i),
                               old_size + 1 == queue->size,
                               old_size + 1,
                               queue->size,
                               TEST_OS_KQUEUE_ENABLED);
    }

    /* Find a present node */
    find = kqueue_find(queue, (void*) 9);
    TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_CREATE_FIND0_ID,
                             find != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)find,
                             TEST_OS_KQUEUE_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_KQUEUE_CREATE_FIND1_ID,
                           (uint32_t)(uintptr_t)find->data == 9,
                           9,
                           (uint32_t)(uintptr_t)find->data,
                           TEST_OS_KQUEUE_ENABLED);

    /* Find a not present node */
    find = kqueue_find(queue, (void*) 42);
    TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_CREATE_FIND2_ID,
                             find == NULL,
                             (uint64_t)(uintptr_t)NULL,
                             (uint64_t)(uintptr_t)find,
                             TEST_OS_KQUEUE_ENABLED);

    /* Dequeue nodes and check "non order" */
    for(uint8_t i = 0; i < 40; ++i)
    {
        old_size = queue->size;
        nodes[i] = kqueue_pop(queue);
        TEST_POINT_ASSERT_UINT(TEST_KQUEUE_POPBURST1_ID(i * 3),
                               old_size - 1 == queue->size,
                               old_size - 1,
                               queue->size,
                               TEST_OS_KQUEUE_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_POPBURST1_ID(i * 3 + 1),
                                 nodes[i] != NULL,
                                 (uint64_t)1,
                                 (uint64_t)(uintptr_t)nodes[i],
                                 TEST_OS_KQUEUE_ENABLED);
        TEST_POINT_ASSERT_UINT(TEST_KQUEUE_POPBURST1_ID(i * 3 + 2),
                                (uint32_t)(uintptr_t)nodes[i]->data == unsorted[i%10],
                                (uint32_t)unsorted[i%10],
                                (uint32_t)(uintptr_t)nodes[i]->data,
                                TEST_OS_KQUEUE_ENABLED);
    }

    TEST_POINT_ASSERT_UINT(TEST_KQUEUE_SIZE1_ID,
                            queue->size == 0,
                            0,
                            queue->size,
                            TEST_OS_KQUEUE_ENABLED);

    /* Dequeue node on empty queue */
    find = kqueue_pop(queue);
    TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_POP1_ID,
                             find == NULL,
                             (uint64_t)(uintptr_t)NULL,
                             (uint64_t)(uintptr_t)find,
                             TEST_OS_KQUEUE_ENABLED);

    /* Delete queue */
    kqueue_delete_queue(&queue);
    TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_DELETE_QUEUE1_ID,
                             queue == NULL,
                             (uint64_t)(uintptr_t)NULL,
                             (uint64_t)(uintptr_t)queue,
                             TEST_OS_KQUEUE_ENABLED);

    /* Delete nodes */
    for(uint8_t i = 0; i < 40; ++i)
    {
        kqueue_delete_node(&nodes[i]);
        TEST_POINT_ASSERT_UDWORD(TEST_KQUEUE_DELETENODEBURST1_ID(i),
                                 nodes[i] == NULL,
                                 (uint64_t)(uintptr_t)NULL,
                                 (uint64_t)(uintptr_t)nodes[i],
                                 TEST_OS_KQUEUE_ENABLED);
    }
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/