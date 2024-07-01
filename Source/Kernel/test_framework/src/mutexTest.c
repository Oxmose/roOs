
/*******************************************************************************
 * @file mutexTest.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 1.0
 *
 * @brief Testing framework mutex testing.
 *
 * @details Testing framework mutex testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <panic.h>
#include <scheduler.h>
#include <mutex.h>
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
static mutex_t excMutex;
static mutex_t orderMutex;
static mutex_t fifoMutex;
static mutex_t recMutex;
static mutex_t cancelMutex;
static mutex_t trylockMutex;
static mutex_t trylockMutexSync;
static mutex_t elevationMutex;

static volatile uint64_t mutexValueTest;
static volatile uint32_t lastTid;
static volatile uint32_t orderedTid;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

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
        error0 |= mutexLock(&excMutex);
        for(j = 0; j < 100; ++j)
            ++mutexValueTest;
        error1 |= mutexUnlock(&excMutex);
    }

    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_EXC1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_MUTEX_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_UNLOCK_EXC1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_MUTEX_ENABLED);
    return NULL;
}

static void* tesOrderRoutine(void* args)
{
    uint32_t tid;
    uint32_t getTid;
    OS_RETURN_E error0 = OS_NO_ERR;
    OS_RETURN_E error1 = OS_NO_ERR;

    tid = (uintptr_t)args;

    error0 |= mutexLock(&orderMutex);
    getTid = lastTid;
    lastTid = tid;
    error1 |= mutexUnlock(&orderMutex);

    kprintf("Thread %d returned\n", tid);

    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_ORDER1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_MUTEX_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_UNLOCK_ORDER1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_MUTEX_ENABLED);

    TEST_POINT_ASSERT_UINT(TEST_MUTEX_ORDER_TEST(tid),
                            getTid == tid + 1,
                            tid + 1,
                            getTid,
                            TEST_MUTEX_ENABLED);
    return NULL;
}

static void* testFifoRoutine(void* args)
{
    uint32_t tid;
    OS_RETURN_E error0 = OS_NO_ERR;
    OS_RETURN_E error1 = OS_NO_ERR;

    tid = (uintptr_t)args;

    error0 |= mutexLock(&fifoMutex);
    if(lastTid == tid + 1)
    {
        ++orderedTid;
    }
    lastTid = tid;
    error1 |= mutexUnlock(&fifoMutex);

    kprintf("Thread %d returned\n", tid);

    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_FIFO1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_MUTEX_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_UNLOCK_FIFO1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_MUTEX_ENABLED);;
    return NULL;
}

static void* testRecursiveRoutine(void* args)
{
    uint32_t tid;
    OS_RETURN_E error0 = OS_NO_ERR;
    OS_RETURN_E error1 = OS_NO_ERR;

    tid = (uintptr_t)args;

    for(volatile int i = 0; i < 1000; ++i)
    {
        error0 |= mutexLock(&recMutex);
        error0 |= mutexLock(&recMutex);
        error0 |= mutexLock(&recMutex);
        error0 |= mutexLock(&recMutex);
        error0 |= mutexLock(&recMutex);
        error1 |= mutexUnlock(&recMutex);
        error1 |= mutexUnlock(&recMutex);
        error1 |= mutexUnlock(&recMutex);
        error1 |= mutexUnlock(&recMutex);
        error1 |= mutexUnlock(&recMutex);
    }

    kprintf("Thread %d returned\n", tid);

    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_RECUR1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_MUTEX_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_UNLOCK_RECUR1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_MUTEX_ENABLED);;
    return NULL;
}

static void* testCancelRoutine(void* args)
{
    uint32_t tid;
    OS_RETURN_E error0 = OS_NO_ERR;

    tid = (uintptr_t)args;

    error0 = mutexLock(&cancelMutex);

    kprintf("Thread %d returned with satus %d\n", tid, error0);

    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_CANCEL1(tid),
                            error0 == OS_ERR_DESTROYED,
                            OS_ERR_DESTROYED,
                            error0,
                            TEST_MUTEX_ENABLED);
    return NULL;
}

static void* testTryLockRoutine(void* args)
{
    uint32_t tid;
    int32_t  level;
    OS_RETURN_E errorTry = OS_NO_ERR;
    OS_RETURN_E error0 = OS_NO_ERR;
    OS_RETURN_E error1 = OS_NO_ERR;
    uint32_t initBase;

    tid = (uintptr_t)args;

    initBase = KERNEL_LOWEST_PRIORITY / 2;

    error0 = mutexLock(&trylockMutexSync);
    errorTry = mutexTryLock(&trylockMutex, &level);
    if(tid > initBase)
    {
        mutexUnlock(&trylockMutex);
    }
    error1 = mutexUnlock(&trylockMutexSync);

    kprintf("Thread %d returned with state %d and value %d\n", tid, errorTry, level);

    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_TRYLOCK1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_MUTEX_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_UNLOCK_TRYLOCK1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_MUTEX_ENABLED);

    if(tid < initBase)
    {
        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_TRYLOCK_TRYLOCK1(tid),
                                errorTry == OS_ERR_BLOCKED,
                                OS_ERR_BLOCKED,
                                errorTry,
                                TEST_MUTEX_ENABLED);

        TEST_POINT_ASSERT_UINT(TEST_MUTEX_TRYLOCK_TEST(tid),
                                level == 0,
                                0,
                                level,
                                TEST_MUTEX_ENABLED);
    }
    else
    {
        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_TRYLOCK_TRYLOCK1(tid),
                                errorTry == OS_NO_ERR,
                                OS_NO_ERR,
                                errorTry,
                                TEST_MUTEX_ENABLED);

        TEST_POINT_ASSERT_INT(TEST_MUTEX_TRYLOCK_TEST(tid),
                              level == 1,
                              1,
                              level,
                              TEST_MUTEX_ENABLED);
    }
    return NULL;
}

static void* testElevationRoutine(void* args)
{
    uintptr_t prio;
    OS_RETURN_E error;
    kernel_thread_t* pCurThread;

    prio = (uintptr_t)args;

    pCurThread = schedGetCurrentThread();

    if(prio == 10)
    {
        error = mutexLock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_MUTEX_ELEVATION(0),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);

        while(elevationMutex.pWaitingList->size == 0){

        }

        kprintf("New thread waiting and prio is %d\n", pCurThread->priority);

        TEST_POINT_ASSERT_BYTE(TEST_MUTEX_ELEVATION_PRIO(0),
                               pCurThread->priority == 10,
                               10,
                               pCurThread->priority,
                               TEST_MUTEX_ENABLED);

        while(elevationMutex.pWaitingList->size == 1){

        }

        TEST_POINT_ASSERT_BYTE(TEST_MUTEX_ELEVATION_PRIO(1),
                               pCurThread->priority == 7,
                               7,
                               pCurThread->priority,
                               TEST_MUTEX_ENABLED);

        kprintf("New thread waiting and prio is %d\n", pCurThread->priority);

        while(elevationMutex.pWaitingList->size == 2){

        }

        TEST_POINT_ASSERT_BYTE(TEST_MUTEX_ELEVATION_PRIO(2),
                               pCurThread->priority == 7,
                               7,
                               pCurThread->priority,
                               TEST_MUTEX_ENABLED);

        kprintf("New thread waiting and prio is %d\n", pCurThread->priority);


        error = mutexUnlock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_UNLOCK_MUTEX_ELEVATION(0),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);

        kprintf("Unlocked the mutex and and prio is %d\n", pCurThread->priority);

        TEST_POINT_ASSERT_BYTE(TEST_MUTEX_ELEVATION_PRIO(3),
                               pCurThread->priority == 10,
                               10,
                               pCurThread->priority,
                               TEST_MUTEX_ENABLED);
    }
    else if(prio == 12)
    {
        schedSleep(100000000);

        error = mutexLock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_MUTEX_ELEVATION(1),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);

        TEST_POINT_ASSERT_BYTE(TEST_MUTEX_ELEVATION_PRIO(4),
                               pCurThread->priority == 7,
                               7,
                               pCurThread->priority,
                               TEST_MUTEX_ENABLED);

        kprintf("Unlocked thread and prio is %d\n", pCurThread->priority);

        error = mutexUnlock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_UNLOCK_MUTEX_ELEVATION(1),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);

        kprintf("Unlocked the mutex and and prio is %d\n", pCurThread->priority);

        TEST_POINT_ASSERT_BYTE(TEST_MUTEX_ELEVATION_PRIO(5),
                               pCurThread->priority == 12,
                               12,
                               pCurThread->priority,
                               TEST_MUTEX_ENABLED);
    }
    else if(prio == 9)
    {
        schedSleep(3000000000);

        error = mutexLock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_MUTEX_ELEVATION(2),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);

        TEST_POINT_ASSERT_BYTE(TEST_MUTEX_ELEVATION_PRIO(6),
                               pCurThread->priority == 9,
                               9,
                               pCurThread->priority,
                               TEST_MUTEX_ENABLED);

        kprintf("Unlocked thread and prio is %d\n", pCurThread->priority);

        error = mutexUnlock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_UNLOCK_MUTEX_ELEVATION(2),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);

        kprintf("Unlocked the mutex and and prio is %d\n", pCurThread->priority);

        TEST_POINT_ASSERT_BYTE(TEST_MUTEX_ELEVATION_PRIO(7),
                               pCurThread->priority == 9,
                               9,
                               pCurThread->priority,
                               TEST_MUTEX_ENABLED);
    }
    else if(prio == 7)
    {
        schedSleep(2000000000);

        error = mutexLock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_MUTEX_ELEVATION(3),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);

        TEST_POINT_ASSERT_BYTE(TEST_MUTEX_ELEVATION_PRIO(8),
                               pCurThread->priority == 7,
                               7,
                               pCurThread->priority,
                               TEST_MUTEX_ENABLED);

        kprintf("Unlocked thread and prio is %d\n", pCurThread->priority);

        error = mutexUnlock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_UNLOCK_MUTEX_ELEVATION(3),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);

        kprintf("Unlocked the mutex and and prio is %d\n", pCurThread->priority);

        TEST_POINT_ASSERT_BYTE(TEST_MUTEX_ELEVATION_PRIO(9),
                               pCurThread->priority == 7,
                               7,
                               pCurThread->priority,
                               TEST_MUTEX_ENABLED);
    }
    else
    {
        kprintf("Unsupported test priority\n");
        TEST_FRAMEWORK_END();
    }

    return NULL;
}

static void testMutualExc(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[100];

    error = mutexInit(&excMutex, 0);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_MUTEX_EXC0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }
    mutexValueTest = 0;

    error = mutexLock(&excMutex);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_EXC0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);

    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < 100; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        0,
                                        "MUTEX_MUTUALEXC_TEST",
                                        0x1000,
                                        (1ULL << (i % MAX_CPU_COUNT)),
                                        testMutualExcRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_THREADS_EXC0(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    kprintf("Gave mutex, waiting threads\n");
    /* Give mutex */
    error = mutexUnlock(&excMutex);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_UNLOCK_EXC0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < 100; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_JOIN_THREADS_EXC0(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    TEST_POINT_ASSERT_UINT(TEST_MUTEX_VALUE,
                            mutexValueTest == 1000000,
                            1000000,
                            mutexValueTest,
                            TEST_MUTEX_ENABLED);

MUTEX_TEST_END:
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

    error = mutexInit(&orderMutex, MUTEX_FLAG_QUEUING_PRIO);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_ORDER_MUTEX,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);

    error = mutexLock(&orderMutex);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_MUTEX_ORDER0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        KERNEL_LOWEST_PRIORITY - i,
                                        "NUTEX_ORDER_TEST",
                                        0x1000,
                                        (1ULL << (i % MAX_CPU_COUNT)),
                                        tesOrderRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_ORDER_THREAD(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    schedSleep(500000000);

    kprintf("Gave mutex, waiting threads\n");
    /* Give mutex */
    error = mutexUnlock(&orderMutex);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_UNLOCK_MUTEX_ORDER0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_JOIN_ORDER_THREADS(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

MUTEX_TEST_END:
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
    error = mutexInit(&fifoMutex, MUTEX_FLAG_QUEUING_FIFO);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_FIFO_MUTEX,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);

    error = mutexLock(&fifoMutex);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_MUTEX_FIFO0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        KERNEL_LOWEST_PRIORITY - i,
                                        "MUTEX_FIFO_TEST",
                                        0x1000,
                                        (1ULL << (i % MAX_CPU_COUNT)),
                                        testFifoRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_FIFO_THREADS(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    schedSleep(500000000);

    kprintf("Gave mutex, waiting threads\n");
    /* Give mutex */
    error = mutexUnlock(&fifoMutex);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_UNLOCK_FIFO0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_JOIN_FIFO_THREADS(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_FIFO_VALUE,
                            orderedTid != KERNEL_LOWEST_PRIORITY + 1,
                            0,
                            orderedTid,
                            TEST_MUTEX_ENABLED);
    kprintf("Returned with %d in a row\n", orderedTid);

MUTEX_TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void testRecursive(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[10];

    error = mutexInit(&recMutex, MUTEX_FLAG_RECURSIVE);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_MUTEX_RECUR0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < 10; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        0,
                                        "MUTEX_RECUR_TEST",
                                        0x1000,
                                        (1ULL << (i % MAX_CPU_COUNT)),
                                        testRecursiveRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_THREADS_RECUR(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    kprintf("Waiting threads\n");

    for(i = 0; i < 10; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_JOIN_THREADS_RECUR(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

MUTEX_TEST_END:
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

    error = mutexInit(&cancelMutex, 0);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_MUTEX_CANCEL,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);

    error = mutexLock(&cancelMutex);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_MUTEX_CANCEL0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < 100; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        0,
                                        "MUTEX_CANCEL_TEST",
                                        0x1000,
                                        (1ULL << (i % MAX_CPU_COUNT)),
                                        testCancelRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_THREADS_CANCEL(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    schedSleep(1000000000);

    kprintf("Destroyed mutex, waiting threads\n");
    /* Give mutex */
    error = mutexDestroy(&cancelMutex);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_DESTROY_MUTEX,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < 100; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_JOIN_THREADS_CANCEL(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }
MUTEX_TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void testTrylock(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[KERNEL_LOWEST_PRIORITY + 1];

    error = mutexInit(&trylockMutex, 0);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_MUTEX_TRYLOCK,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }


    error = mutexInit(&trylockMutexSync, MUTEX_FLAG_QUEUING_PRIO);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_MUTEX_SYNC_TRYLOCK,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);

    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    error = mutexLock(&trylockMutexSync);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_LOCK_MUTEX_TRYLOCK_SYNC,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        KERNEL_LOWEST_PRIORITY - i,
                                        "MUTEX_TRYLOCK_TEST",
                                        0x1000,
                                        (1ULL << (i % MAX_CPU_COUNT)),
                                        testTryLockRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_THREADS_TRYLOCK(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    schedSleep(1000000000);

    kprintf("Unlock mutex, waiting threads\n");
    /* Give mutex */
    error = mutexUnlock(&trylockMutexSync);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_SYNC_MUTEX_UNLOCK,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_JOIN_THREADS_TRYLOCK(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }
MUTEX_TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void testElevation(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[4];

    error = mutexInit(&elevationMutex, MUTEX_FLAG_PRIO_ELEVATION);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_MUTEX_ELEVATION,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    error = schedCreateKernelThread(&pThreads[0],
                                    10,
                                    "MUTEX_ELECATION_TEST",
                                    0x1000,
                                    (1ULL << (0 % MAX_CPU_COUNT)),
                                    testElevationRoutine,
                                    (void*)(uintptr_t)10);

    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_THREADS_ELEVATION(0),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    error = schedCreateKernelThread(&pThreads[1],
                                    12,
                                    "MUTEX_ELECATION_TEST",
                                    0x1000,
                                    (1ULL << (1 % MAX_CPU_COUNT)),
                                    testElevationRoutine,
                                    (void*)(uintptr_t)12);

    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_THREADS_ELEVATION(1),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    error = schedCreateKernelThread(&pThreads[2],
                                    9,
                                    "MUTEX_ELECATION_TEST",
                                    0x1000,
                                    (1ULL << (2 % MAX_CPU_COUNT)),
                                    testElevationRoutine,
                                    (void*)(uintptr_t)9);

    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_THREADS_ELEVATION(2),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    error = schedCreateKernelThread(&pThreads[3],
                                    7,
                                    "MUTEX_ELECATION_TEST",
                                    0x1000,
                                    (1ULL << (3 % MAX_CPU_COUNT)),
                                    testElevationRoutine,
                                    (void*)(uintptr_t)7);

    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_THREADS_ELEVATION(3),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < 4; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_MUTEX_JOIN_THREADS_ELEVATION(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_MUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }
MUTEX_TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void* testThread(void* args)
{
    (void)args;
    testMutualExc();
    kprintf("Mutual Exclusion Done\n");
    testOrder();
    kprintf("Order done\n");
    testFifo();
    kprintf("Fifo done\n");
    testRecursive();
    kprintf("Recursive done\n");
    testDestroy();
    kprintf("Destroy Done\n");
    testTrylock();
    kprintf("Trylock Done\n");
    testElevation();
    kprintf("Elevation done\n");

    TEST_FRAMEWORK_END();

    return NULL;
}

void mutexTest(void)
{
    OS_RETURN_E error;
    kernel_thread_t* pTestThread;

    /* Spawn the test thread */
    error = schedCreateKernelThread(&pTestThread,
                                    0,
                                    "MUTEX_MAIN_TEST",
                                    0x1000,
                                    1,
                                    testThread,
                                    NULL);
    TEST_POINT_ASSERT_RCODE(TEST_MUTEX_CREATE_THREAD0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_MUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

MUTEX_TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/