
/*******************************************************************************
 * @file ksemaphoreTest.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 14/05/2023
 *
 * @version 1.0
 *
 * @brief Testing framework semaphore testing.
 *
 * @details Testing framework semaphore testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <ksemaphore.h>
#include <scheduler.h>
#include <stdint.h>
#include <kerneloutput.h>
#include <kqueue.h>

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
static ksemaphore_t mutexSem;
static ksemaphore_t orderSem;
static ksemaphore_t fifoSem;
static ksemaphore_t cancelSem;
static ksemaphore_t trypendSem;
static ksemaphore_t trypendSemSync;
static ksemaphore_t semMultiple;
static volatile uint64_t mutexValueTest;
static volatile uint32_t lastTid;
static volatile uint32_t orderedTid;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

static void* testThread(void* args);


/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void* testMutualExcRoutine(void* args)
{
    uint32_t tid;
    uint32_t i;
    uint32_t j;
    OS_RETURN_E error0 = OS_NO_ERR;
    OS_RETURN_E error1 = OS_NO_ERR;

    tid = (uintptr_t)args;

    for(i = 0; i < 100; ++i)
    {
        error0 |= ksemWait(&mutexSem);
        for(j = 0; j < 100; ++j)
            ++mutexValueTest;
        error1 |= ksemPost(&mutexSem);
    }
    kprintf("Thread %d returned\n", tid);

    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_WAIT_SEM_KMUTEX1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_KSEMAPHORE_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_POST_SEM_KMUTEX1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_KSEMAPHORE_ENABLED);
    return NULL;
}

static void* tesOrderRoutine(void* args)
{
    uint32_t tid;
    uint32_t getTid;
    OS_RETURN_E error0 = OS_NO_ERR;
    OS_RETURN_E error1 = OS_NO_ERR;

    tid = (uintptr_t)args;

    error0 |= ksemWait(&orderSem);
    getTid = lastTid;
    lastTid = tid;
    error1 |= ksemPost(&orderSem);

    kprintf("Thread %d returned\n", tid);

    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_WAIT_SEM_ORDER1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_KSEMAPHORE_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_POST_SEM_ORDER1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_KSEMAPHORE_ENABLED);

    TEST_POINT_ASSERT_UINT(TEST_KSEMAPHORE_ORDER_TEST(tid),
                            getTid == tid + 1,
                            tid + 1,
                            getTid,
                            TEST_KSEMAPHORE_ENABLED);
    return NULL;
}

static void* testFifoRoutine(void* args)
{
    uint32_t tid;
    OS_RETURN_E error0 = OS_NO_ERR;
    OS_RETURN_E error1 = OS_NO_ERR;

    tid = (uintptr_t)args;

    error0 |= ksemWait(&fifoSem);
    if(lastTid == tid + 1)
    {
        ++orderedTid;
    }
    lastTid = tid;
    error1 |= ksemPost(&fifoSem);

    kprintf("Thread %d returned\n", tid);

    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_WAIT_SEM_FIFO1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_KSEMAPHORE_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_POST_SEM_FIFO1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_KSEMAPHORE_ENABLED);;
    return NULL;
}

static void* testCancelRoutine(void* args)
{
    uint32_t tid;
    OS_RETURN_E error0 = OS_NO_ERR;

    tid = (uintptr_t)args;

    error0 = ksemWait(&cancelSem);

    kprintf("Thread %d returned with satus %d\n", tid, error0);

    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_WAIT_SEM_CANCEL(tid),
                            error0 == OS_ERR_DESTROYED,
                            OS_ERR_DESTROYED,
                            error0,
                            TEST_KSEMAPHORE_ENABLED);
    return NULL;
}

static void* testTrypendRoutine(void* args)
{
    uint32_t tid;
    int32_t  level;
    OS_RETURN_E errorTry = OS_NO_ERR;
    OS_RETURN_E error0 = OS_NO_ERR;
    OS_RETURN_E error1 = OS_NO_ERR;
    int32_t initBase;

    tid = (uintptr_t)args;

    initBase = KERNEL_LOWEST_PRIORITY / 2;
    initBase = initBase - (KERNEL_LOWEST_PRIORITY - tid);

    error0 = ksemWait(&trypendSemSync);
    errorTry = ksemTryWait(&trypendSem, &level);
    error1 = ksemPost(&trypendSemSync);

    kprintf("Thread %d returned with state %d and value %d\n", tid, errorTry, level);

    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_WAIT_SEM_TRYPEND1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_KSEMAPHORE_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_POST_TRYPEND1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_KSEMAPHORE_ENABLED);

    if(initBase <= 0)
    {
        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_TRYWAIT_TRYPEND1(tid),
                                errorTry == OS_ERR_BLOCKED,
                                OS_ERR_BLOCKED,
                                errorTry,
                                TEST_KSEMAPHORE_ENABLED);

        TEST_POINT_ASSERT_UINT(TEST_KSEMAPHORE_TRYPEND_TEST(tid),
                                level == 0,
                                0,
                                level,
                                TEST_KSEMAPHORE_ENABLED);
    }
    else
    {
        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_TRYWAIT_TRYPEND1(tid),
                                errorTry == OS_NO_ERR,
                                OS_NO_ERR,
                                errorTry,
                                TEST_KSEMAPHORE_ENABLED);

        TEST_POINT_ASSERT_INT(TEST_KSEMAPHORE_TRYPEND_TEST(tid),
                              level == initBase,
                              initBase,
                              level,
                              TEST_KSEMAPHORE_ENABLED);
    }
    return NULL;
}


static void testMultiplePost(void)
{
    uint32_t i;
    OS_RETURN_E error;

    error = ksemInit(&semMultiple, 0, 0);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_KSEMAPHORE7,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);
    mutexValueTest = 0;

    for(i = 0; i < 100; ++i)
    {
        error = ksemPost(&semMultiple);
        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_POST_MULTIPLE(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KSEMAPHORE_ENABLED);
    }

    for(i = 0; i < 100; ++i)
    {
        error = ksemWait(&semMultiple);
        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_WAIT_MULTIPLE(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KSEMAPHORE_ENABLED);
    }

    error = ksemDestroy(&semMultiple);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_DESTROY_MULTIPLE,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);
}

static void testMutualExc(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[100];

    error = ksemInit(&mutexSem, 0, 0);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_KSEMAPHORE1,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);
    mutexValueTest = 0;

    for(i = 0; i < 100; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        0,
                                        "SEM_MUTUALEXC_TEST",
                                        0x1000,
                                        (1ULL << (i % SOC_CPU_COUNT)),
                                        testMutualExcRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_THREADS1(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KSEMAPHORE_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto SEM_TEST_END;
        }
    }

    kprintf("Gave semaphore, waiting threads\n");

    /* Give semaphore */
    error = ksemPost(&mutexSem);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_POST_SEM_KMUTEX0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto SEM_TEST_END;
    }

    for(i = 0; i < 100; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_JOIN_THREADS1(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KSEMAPHORE_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto SEM_TEST_END;
        }
    }

    TEST_POINT_ASSERT_UINT(TEST_KSEMAPHORE_KMUTEX_VALUE,
                            mutexValueTest == 1000000,
                            1000000,
                            mutexValueTest,
                            TEST_KSEMAPHORE_ENABLED);

SEM_TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void testOrder(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[KERNEL_LOWEST_PRIORITY + 1];

    lastTid = KERNEL_LOWEST_PRIORITY + 1;

    error = ksemInit(&orderSem, 0, KSEMAPHORE_FLAG_QUEUING_PRIO);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_KSEMAPHORE2,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        KERNEL_LOWEST_PRIORITY - i,
                                        "SEM_ORDER_TEST",
                                        0x1000,
                                        (1ULL << (i % SOC_CPU_COUNT)),
                                        tesOrderRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_THREADS2(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KSEMAPHORE_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto SEM_TEST_END;
        }
    }

    schedSleep(500000000);

    kprintf("Gave semaphore, waiting threads\n");
    /* Give semaphore */
    error = ksemPost(&orderSem);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_POST_SEM_ORDER0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto SEM_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_JOIN_THREADS2(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KSEMAPHORE_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto SEM_TEST_END;
        }
    }

SEM_TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void testFifo(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[KERNEL_LOWEST_PRIORITY + 1];

    lastTid = KERNEL_LOWEST_PRIORITY + 1;
    orderedTid = 0;
    error = ksemInit(&fifoSem, 0, KSEMAPHORE_FLAG_QUEUING_FIFO);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_KSEMAPHORE3,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        KERNEL_LOWEST_PRIORITY - i,
                                        "SEM_FIFO_TEST",
                                        0x1000,
                                        (1ULL << (i % SOC_CPU_COUNT)),
                                        testFifoRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_THREADS3(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KSEMAPHORE_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto SEM_TEST_END;
        }
    }

    schedSleep(500000000);

    kprintf("Gave semaphore, waiting threads\n");
    /* Give semaphore */
    error = ksemPost(&fifoSem);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_POST_SEM_FIFO0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto SEM_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_JOIN_THREADS3(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KSEMAPHORE_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto SEM_TEST_END;
        }
    }

    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_FIFO_VALUE,
                            orderedTid != KERNEL_LOWEST_PRIORITY + 1,
                            0,
                            orderedTid,
                            TEST_KSEMAPHORE_ENABLED);
    kprintf("Returned with %d in a row\n", orderedTid);

SEM_TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void testDestroy(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[100];

    error = ksemInit(&cancelSem, 0, 0);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_KSEMAPHORE4,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);

    for(i = 0; i < 100; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        0,
                                        "SEM_CANCEL_TEST",
                                        0x1000,
                                        (1ULL << (i % SOC_CPU_COUNT)),
                                        testCancelRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_THREADS4(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KSEMAPHORE_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto SEM_TEST_END;
        }
    }

    schedSleep(1000000000);

    kprintf("Destroyed semaphore, waiting threads\n");
    /* Give semaphore */
    error = ksemDestroy(&cancelSem);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_POST_SEM_CANCEL0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto SEM_TEST_END;
    }

    for(i = 0; i < 100; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_JOIN_THREADS4(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KSEMAPHORE_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto SEM_TEST_END;
        }
    }
SEM_TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void testTrypend(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[KERNEL_LOWEST_PRIORITY + 1];

    error = ksemInit(&trypendSem, KERNEL_LOWEST_PRIORITY / 2, 0);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_KSEMAPHORE5,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);

    error = ksemInit(&trypendSemSync, 0, KSEMAPHORE_FLAG_QUEUING_PRIO);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_KSEMAPHORE6,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        KERNEL_LOWEST_PRIORITY - i,
                                        "SEM_TRYPEND_TEST",
                                        0x1000,
                                        (1ULL << (i % SOC_CPU_COUNT)),
                                        testTrypendRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_THREADS5(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KSEMAPHORE_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto SEM_TEST_END;
        }
    }

    schedSleep(1000000000);

    kprintf("Post semaphore, waiting threads\n");
    /* Give semaphore */
    error = ksemPost(&trypendSemSync);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_POST_SEM_TRYPEND0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto SEM_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_JOIN_THREADS5(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KSEMAPHORE_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto SEM_TEST_END;
        }
    }
SEM_TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void* testThread(void* args)
{
    (void)args;

    testMultiplePost();
    kprintf("Multiple Post done\n");
    testMutualExc();
    kprintf("Mutual Exclusion Done\n");
    testOrder();
    kprintf("Order done\n");
    testFifo();
    kprintf("Fifo done\n");
    testDestroy();
    kprintf("Destroy Done\n");
    testTrypend();
    kprintf("Trypend Done\n");
#if 0
#else
    (void)testMutualExc;
    (void)testOrder;
    (void)testFifo;
    (void)testDestroy;
    (void)testTrypend;
#endif
    TEST_FRAMEWORK_END();

    return NULL;
}

void ksemaphoreTest(void)
{
    OS_RETURN_E error;
    kernel_thread_t* pTestThread;

    /* Spawn the test thread */
    error = schedCreateKernelThread(&pTestThread,
                                    0,
                                    "SEM_MAIN_TEST",
                                    0x1000,
                                    1,
                                    testThread,
                                    NULL);
    TEST_POINT_ASSERT_RCODE(TEST_KSEMAPHORE_CREATE_THREAD0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KSEMAPHORE_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto SEM_TEST_END;
    }

SEM_TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/