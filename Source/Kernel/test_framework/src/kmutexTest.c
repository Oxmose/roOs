
/*******************************************************************************
 * @file kmutexTest.c
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
#include <kmutex.h>
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
static kmutex_t excMutex;
static kmutex_t orderMutex;
static kmutex_t fifoMutex;
static kmutex_t recMutex;
static kmutex_t cancelMutex;
static kmutex_t trylockMutex;
static kmutex_t trylockMutexSync;
static kmutex_t elevationMutex;

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
        error0 |= kmutexLock(&excMutex);
        for(j = 0; j < 100; ++j)
            ++mutexValueTest;
        error1 |= kmutexUnlock(&excMutex);
    }

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_EXC1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_KMUTEX_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_UNLOCK_EXC1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_KMUTEX_ENABLED);
    return NULL;
}

static void* tesOrderRoutine(void* args)
{
    uint32_t tid;
    uint32_t getTid;
    OS_RETURN_E error0 = OS_NO_ERR;
    OS_RETURN_E error1 = OS_NO_ERR;

    tid = (uintptr_t)args;

    error0 |= kmutexLock(&orderMutex);
    getTid = lastTid;
    lastTid = tid;
    error1 |= kmutexUnlock(&orderMutex);

    kprintf("Thread %d returned\n", tid);

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_ORDER1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_KMUTEX_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_UNLOCK_ORDER1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_KMUTEX_ENABLED);

    TEST_POINT_ASSERT_UINT(TEST_KMUTEX_ORDER_TEST(tid),
                            getTid == tid + 1,
                            tid + 1,
                            getTid,
                            TEST_KMUTEX_ENABLED);
    return NULL;
}

static void* testFifoRoutine(void* args)
{
    uint32_t tid;
    OS_RETURN_E error0 = OS_NO_ERR;
    OS_RETURN_E error1 = OS_NO_ERR;

    tid = (uintptr_t)args;

    error0 |= kmutexLock(&fifoMutex);
    if(lastTid == tid + 1)
    {
        ++orderedTid;
    }
    lastTid = tid;
    error1 |= kmutexUnlock(&fifoMutex);

    kprintf("Thread %d returned\n", tid);

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_FIFO1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_KMUTEX_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_UNLOCK_FIFO1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_KMUTEX_ENABLED);;
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
        error0 |= kmutexLock(&recMutex);
        error0 |= kmutexLock(&recMutex);
        error0 |= kmutexLock(&recMutex);
        error0 |= kmutexLock(&recMutex);
        error0 |= kmutexLock(&recMutex);
        error1 |= kmutexUnlock(&recMutex);
        error1 |= kmutexUnlock(&recMutex);
        error1 |= kmutexUnlock(&recMutex);
        error1 |= kmutexUnlock(&recMutex);
        error1 |= kmutexUnlock(&recMutex);
    }

    kprintf("Thread %d (%d) returned\n", tid, schedGetCurrentThread()->tid);

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_RECUR1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_KMUTEX_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_UNLOCK_RECUR1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_KMUTEX_ENABLED);;
    return NULL;
}

static void* testCancelRoutine(void* args)
{
    uint32_t tid;
    OS_RETURN_E error0 = OS_NO_ERR;

    tid = (uintptr_t)args;

    error0 = kmutexLock(&cancelMutex);

    kprintf("Thread %d returned with satus %d\n", tid, error0);

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_CANCEL1(tid),
                            error0 == OS_ERR_DESTROYED,
                            OS_ERR_DESTROYED,
                            error0,
                            TEST_KMUTEX_ENABLED);
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

    error0 = kmutexLock(&trylockMutexSync);
    errorTry = kmutexTryLock(&trylockMutex, &level);
    if(tid > initBase)
    {
        kmutexUnlock(&trylockMutex);
    }
    error1 = kmutexUnlock(&trylockMutexSync);

    kprintf("Thread %d returned with state %d and value %d\n", tid, errorTry, level);

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_TRYLOCK1(tid),
                            error0 == OS_NO_ERR,
                            OS_NO_ERR,
                            error0,
                            TEST_KMUTEX_ENABLED);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_UNLOCK_TRYLOCK1(tid),
                            error1 == OS_NO_ERR,
                            OS_NO_ERR,
                            error1,
                            TEST_KMUTEX_ENABLED);

    if(tid < initBase)
    {
        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_TRYLOCK_TRYLOCK1(tid),
                                errorTry == OS_ERR_BLOCKED,
                                OS_ERR_BLOCKED,
                                errorTry,
                                TEST_KMUTEX_ENABLED);

        TEST_POINT_ASSERT_UINT(TEST_KMUTEX_TRYLOCK_TEST(tid),
                                level == 0,
                                0,
                                level,
                                TEST_KMUTEX_ENABLED);
    }
    else
    {
        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_TRYLOCK_TRYLOCK1(tid),
                                errorTry == OS_NO_ERR,
                                OS_NO_ERR,
                                errorTry,
                                TEST_KMUTEX_ENABLED);

        TEST_POINT_ASSERT_INT(TEST_KMUTEX_TRYLOCK_TEST(tid),
                              level == 1,
                              1,
                              level,
                              TEST_KMUTEX_ENABLED);
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
        error = kmutexLock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_KMUTEX_ELEVATION(0),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);

        while(elevationMutex.nbWaitingThreads == 0){

        }
        schedSleep(1000000);

        kprintf("New thread waiting and prio is %d\n", pCurThread->priority);

        TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(0),
                               pCurThread->priority == 10,
                               10,
                               pCurThread->priority,
                               TEST_KMUTEX_ENABLED);

        while(elevationMutex.nbWaitingThreads == 1){

        }
        schedSleep(1000000);

        kprintf("New thread waiting and prio is %d\n", pCurThread->priority);

        TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(1),
                               pCurThread->priority == 7,
                               7,
                               pCurThread->priority,
                               TEST_KMUTEX_ENABLED);

        while(elevationMutex.nbWaitingThreads == 2){

        }
        schedSleep(1000000);

        kprintf("New thread waiting and prio is %d\n", pCurThread->priority);
        TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(2),
                               pCurThread->priority == 7,
                               7,
                               pCurThread->priority,
                               TEST_KMUTEX_ENABLED);



        error = kmutexUnlock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_UNLOCK_KMUTEX_ELEVATION(0),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);

        kprintf("Unlocked the mutex and and prio is %d\n", pCurThread->priority);

        TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(3),
                               pCurThread->priority == 10,
                               10,
                               pCurThread->priority,
                               TEST_KMUTEX_ENABLED);
    }
    else if(prio == 12)
    {
        schedSleep(200000000);

        error = kmutexLock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_KMUTEX_ELEVATION(1),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);

        TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(4),
                               pCurThread->priority == 12,
                               12,
                               pCurThread->priority,
                               TEST_KMUTEX_ENABLED);

        kprintf("Unlocked thread and prio is %d\n", pCurThread->priority);

        error = kmutexUnlock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_UNLOCK_KMUTEX_ELEVATION(1),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);

        kprintf("Unlocked the mutex and and prio is %d\n", pCurThread->priority);

        TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(5),
                               pCurThread->priority == 12,
                               12,
                               pCurThread->priority,
                               TEST_KMUTEX_ENABLED);
    }
    else if(prio == 9)
    {
        schedSleep(6000000000);

        error = kmutexLock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_KMUTEX_ELEVATION(2),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);

        TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(6),
                               pCurThread->priority == 9,
                               9,
                               pCurThread->priority,
                               TEST_KMUTEX_ENABLED);

        kprintf("Unlocked thread and prio is %d\n", pCurThread->priority);

        error = kmutexUnlock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_UNLOCK_KMUTEX_ELEVATION(2),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);

        kprintf("Unlocked the mutex and and prio is %d\n", pCurThread->priority);

        TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(7),
                               pCurThread->priority == 9,
                               9,
                               pCurThread->priority,
                               TEST_KMUTEX_ENABLED);
    }
    else if(prio == 7)
    {
        schedSleep(4000000000);

        error = kmutexLock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_KMUTEX_ELEVATION(3),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);

        TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(8),
                               pCurThread->priority == 7,
                               7,
                               pCurThread->priority,
                               TEST_KMUTEX_ENABLED);

        kprintf("Unlocked thread and prio is %d\n", pCurThread->priority);

        error = kmutexUnlock(&elevationMutex);
        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_UNLOCK_KMUTEX_ELEVATION(3),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);

        kprintf("Unlocked the mutex and and prio is %d\n", pCurThread->priority);

        TEST_POINT_ASSERT_BYTE(TEST_KMUTEX_ELEVATION_PRIO(9),
                               pCurThread->priority == 7,
                               7,
                               pCurThread->priority,
                               TEST_KMUTEX_ENABLED);
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

    error = kmutexInit(&excMutex, 0);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_KMUTEX_EXC0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }
    mutexValueTest = 0;

    error = kmutexLock(&excMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_EXC0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);

    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < 100; ++i)
    {
        error = schedCreateThread(&pThreads[i], true,
                                        0,
                                        "MUTEX_MUTUALEXC_TEST",
                                        0x1000,
                                        (1ULL << (i % SOC_CPU_COUNT)),
                                        testMutualExcRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_THREADS_EXC0(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    kprintf("Gave mutex, waiting threads\n");
    /* Give mutex */
    error = kmutexUnlock(&excMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_UNLOCK_EXC0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < 100; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_JOIN_THREADS_EXC0(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    TEST_POINT_ASSERT_UINT(TEST_KMUTEX_VALUE,
                            mutexValueTest == 1000000,
                            1000000,
                            mutexValueTest,
                            TEST_KMUTEX_ENABLED);

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

    error = kmutexInit(&orderMutex, KMUTEX_FLAG_QUEUING_PRIO);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_ORDER_KMUTEX,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);

    error = kmutexLock(&orderMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_KMUTEX_ORDER0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedCreateThread(&pThreads[i], true,
                                        KERNEL_LOWEST_PRIORITY - i,
                                        "NUTEX_ORDER_TEST",
                                        0x1000,
                                        (1ULL << (i % SOC_CPU_COUNT)),
                                        tesOrderRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_ORDER_THREAD(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    schedSleep(500000000);

    kprintf("Gave mutex, waiting threads\n");
    /* Give mutex */
    error = kmutexUnlock(&orderMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_UNLOCK_KMUTEX_ORDER0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_JOIN_ORDER_THREADS(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);
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
    error = kmutexInit(&fifoMutex, KMUTEX_FLAG_QUEUING_FIFO);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_FIFO_KMUTEX,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);

    error = kmutexLock(&fifoMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_KMUTEX_FIFO0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedCreateThread(&pThreads[i], true,
                                        KERNEL_LOWEST_PRIORITY - i,
                                        "MUTEX_FIFO_TEST",
                                        0x1000,
                                        (1ULL << (i % SOC_CPU_COUNT)),
                                        testFifoRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_FIFO_THREADS(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    schedSleep(500000000);

    kprintf("Gave mutex, waiting threads\n");
    /* Give mutex */
    error = kmutexUnlock(&fifoMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_UNLOCK_FIFO0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_JOIN_FIFO_THREADS(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_FIFO_VALUE,
                            orderedTid != KERNEL_LOWEST_PRIORITY + 1,
                            0,
                            orderedTid,
                            TEST_KMUTEX_ENABLED);
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

    error = kmutexInit(&recMutex, KMUTEX_FLAG_RECURSIVE);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_KMUTEX_RECUR0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < 10; ++i)
    {
        error = schedCreateThread(&pThreads[i], true,
                                        0,
                                        "MUTEX_RECUR_TEST",
                                        0x1000,
                                        (1ULL << (i % SOC_CPU_COUNT)),
                                        testRecursiveRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_THREADS_RECUR(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    kprintf("Waiting threads\n");

    for(i = 0; i < 10; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_JOIN_THREADS_RECUR(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);
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

    error = kmutexInit(&cancelMutex, 0);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_KMUTEX_CANCEL,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);

    error = kmutexLock(&cancelMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_KMUTEX_CANCEL0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < 100; ++i)
    {
        error = schedCreateThread(&pThreads[i], true,
                                        0,
                                        "MUTEX_CANCEL_TEST",
                                        0x1000,
                                        (1ULL << (i % SOC_CPU_COUNT)),
                                        testCancelRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_THREADS_CANCEL(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    schedSleep(1000000000);

    kprintf("Destroyed mutex, waiting threads\n");
    /* Give mutex */
    error = kmutexDestroy(&cancelMutex);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_DESTROY_KMUTEX,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < 100; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_JOIN_THREADS_CANCEL(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);
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

    error = kmutexInit(&trylockMutex, 0);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_KMUTEX_TRYLOCK,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }


    error = kmutexInit(&trylockMutexSync, KMUTEX_FLAG_QUEUING_PRIO);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_KMUTEX_SYNC_TRYLOCK,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);

    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    error = kmutexLock(&trylockMutexSync);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_LOCK_KMUTEX_TRYLOCK_SYNC,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedCreateThread(&pThreads[i], true,
                                        KERNEL_LOWEST_PRIORITY - i,
                                        "MUTEX_TRYLOCK_TEST",
                                        0x1000,
                                        (1ULL << (i % SOC_CPU_COUNT)),
                                        testTryLockRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_THREADS_TRYLOCK(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto MUTEX_TEST_END;
        }
    }

    schedSleep(1000000000);

    kprintf("Unlock mutex, waiting threads\n");
    /* Give mutex */
    error = kmutexUnlock(&trylockMutexSync);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_SYNC_KMUTEX_UNLOCK,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < KERNEL_LOWEST_PRIORITY + 1; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_JOIN_THREADS_TRYLOCK(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);
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

    error = kmutexInit(&elevationMutex,
                       KMUTEX_FLAG_PRIO_ELEVATION |
                       KMUTEX_FLAG_QUEUING_PRIO);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_KMUTEX_ELEVATION,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    error = schedCreateThread(&pThreads[0], true,
                                    10,
                                    "MUTEX_ELECATION_TEST",
                                    0x1000,
                                    (1ULL << (0 % SOC_CPU_COUNT)),
                                    testElevationRoutine,
                                    (void*)(uintptr_t)10);

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_THREADS_ELEVATION(0),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    error = schedCreateThread(&pThreads[1], true,
                                    12,
                                    "MUTEX_ELECATION_TEST",
                                    0x1000,
                                    (1ULL << (1 % SOC_CPU_COUNT)),
                                    testElevationRoutine,
                                    (void*)(uintptr_t)12);

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_THREADS_ELEVATION(1),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    error = schedCreateThread(&pThreads[2], true,
                                    9,
                                    "MUTEX_ELECATION_TEST",
                                    0x1000,
                                    (1ULL << (2 % SOC_CPU_COUNT)),
                                    testElevationRoutine,
                                    (void*)(uintptr_t)9);

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_THREADS_ELEVATION(2),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    error = schedCreateThread(&pThreads[3], true,
                                    7,
                                    "MUTEX_ELECATION_TEST",
                                    0x1000,
                                    (1ULL << (3 % SOC_CPU_COUNT)),
                                    testElevationRoutine,
                                    (void*)(uintptr_t)7);

    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_THREADS_ELEVATION(3),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto MUTEX_TEST_END;
    }

    for(i = 0; i < 4; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_JOIN_THREADS_ELEVATION(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KMUTEX_ENABLED);
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
    testRecursive();
    kprintf("Recursive done\n");
    testMutualExc();
    kprintf("Mutual Exclusion Done\n");
    testOrder();
    kprintf("Order done\n");
    testFifo();
    kprintf("Fifo done\n");
    testTrylock();
    kprintf("Trylock Done\n");
    testDestroy();
    kprintf("Destroy Done\n");
    testElevation();
    kprintf("Elevation done\n");
#if 0
#else
    (void)testMutualExc;
    (void)testOrder;
    (void)testFifo;
    (void)testRecursive;
    (void)testDestroy;
    (void)testTrylock;
    (void)testElevation;
#endif
    TEST_FRAMEWORK_END();

    return NULL;
}

void kmutexTest(void)
{
    OS_RETURN_E error;
    kernel_thread_t* pTestThread;

    /* Spawn the test thread */
    error = schedCreateThread(&pTestThread, true,
                                    0,
                                    "MUTEX_MAIN_TEST",
                                    0x1000,
                                    1,
                                    testThread,
                                    NULL);
    TEST_POINT_ASSERT_RCODE(TEST_KMUTEX_CREATE_THREAD0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KMUTEX_ENABLED);
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