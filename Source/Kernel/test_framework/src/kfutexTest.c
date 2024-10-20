
/*******************************************************************************
 * @file kfutexTest.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/06/2024
 *
 * @version 1.0
 *
 * @brief Testing framework futex testing.
 *
 * @details Testing framework futex testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <kfutex.h>
#include <scheduler.h>
#include <kerneloutput.h>
#include <uhashtable.h>
#include <memory.h>

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
extern uhashtable_t* spFutexTable;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
static volatile int32_t orderWait = 0;
static volatile int32_t orderVal  = 0;
static kfutex_t orderFutex;
static kfutex_t multipleFutex;
static volatile int32_t multipleFutexValue = 0;
static volatile uint32_t spinlock = 0;
static volatile uint32_t returnedThreads = 0;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

static void* testOrderRoutineWait(void* args);
static void* testOrderRoutineWake(void* args);
static void* testWaitMultiple(void* args);
static void* testWaitSameHandleValue(void* args);
static void* testWaitReleaseResources(void* args);
static void testOrder(void);
static void testMultiple(void);
static void testSameHandleValue(void);
static void testReleaseResources(void);
static void* testThread(void* args);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void* testOrderRoutineWait(void* args)
{
    int32_t tid;
    uint64_t timeWait;
    OS_RETURN_E error;
    KFUTEX_WAKE_REASON_E wakeReason;

    tid = (uint32_t)(uintptr_t)args;
    timeWait = (uint64_t)(tid + 1) * 500000000;

    error = schedSleep(timeWait);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_ORDER_WAIT_SLEEP(tid),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    kprintf("Wait thread %d, waited %lluns\n", tid, timeWait);

    /* Wait for futex */
    error = kfutexWait(&orderFutex, 0, &wakeReason);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_ORDER_WAIT_WAIT(tid),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_KFUTEX_ORDER_WAIT_WAITVAL(tid),
                           orderVal == tid,
                           tid,
                           orderVal,
                           TEST_KFUTEX_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_KFUTEX_ORDER_WAIT_WAITREASON(tid),
                           wakeReason == KFUTEX_WAKEUP_WAKE,
                           wakeReason,
                           KFUTEX_WAKEUP_WAKE,
                           TEST_KFUTEX_ENABLED);
    ++orderVal;
    kprintf("Wait thread %d, done on CPU %d order %d, reason %d\n", tid, cpuGetId(), orderVal, wakeReason);

    return NULL;
}

static void* testOrderRoutineWake(void* args)
{
    int32_t tid;
    uint64_t timeWait;
    OS_RETURN_E error;
    size_t      wakeCount;

    tid = (uint32_t)(uintptr_t)args;
    timeWait = (uint64_t)(tid + 11) * 500000000;

    kprintf("wake thread %d, sleeping %lluns\n", tid, timeWait);
    error = schedSleep(timeWait);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_ORDER_WAIT_SLEEP_WAKE(tid),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    kprintf("wake thread %d, waited %lluns\n", tid, timeWait);
    /* Wait for futex */
    orderWait = 1;
    wakeCount = 1;
    error = kfutexWake(&orderFutex, &wakeCount);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_ORDER_WAIT_WAKE(tid),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    return NULL;
}

static void* testWaitMultiple(void* args)
{
    OS_RETURN_E error;
    KFUTEX_WAKE_REASON_E reason;
    uintptr_t tid;

    tid = (uintptr_t)args;

    kprintf("Wait multiple waiting %d\n", tid);
    error = kfutexWait(&multipleFutex, 0, &reason);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_MULTIPLE_WAIT(tid),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    kprintf("Wait wake thread %d reason: %d\n", tid, reason);

    spinlockAcquire(&spinlock);
    ++returnedThreads;
    kprintf("Thread returned: %d\n", returnedThreads);
    spinlockRelease(&spinlock);

    return NULL;
}

static void* testWaitSameHandleValue(void* args)
{
    OS_RETURN_E error;
    KFUTEX_WAKE_REASON_E reason;
    uintptr_t tid;

    tid = (uintptr_t)args;

    kprintf("Wait samehandle waiting %d\n", tid);

    error = kfutexWait(&multipleFutex, 0, &reason);


    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_SAMEHANDLE_WAIT(tid),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);

    //kprintf("Wait samehandle wake thread %d reason: %d\n", tid, reason);

    spinlockAcquire(&spinlock);
    ++returnedThreads;
    //kprintf("Thread samehandle returned: %d\n", returnedThreads);
    spinlockRelease(&spinlock);

    return NULL;
}

static void* testWaitReleaseResources(void* args)
{
    OS_RETURN_E error;
    KFUTEX_WAKE_REASON_E reason;
    uintptr_t tid;

    tid = (uintptr_t)args;

    kprintf("Wait release waiting %d\n", tid);

    error = kfutexWait(&multipleFutex, 0, &reason);

    if(tid == 10)
    {
        TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_RELEASE_WAIT(tid),
                                error == OS_ERR_DESTROYED,
                                OS_ERR_DESTROYED,
                                error,
                                TEST_KFUTEX_ENABLED);
    }
    else
    {
        TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_RELEASE_WAIT(tid),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KFUTEX_ENABLED);
    }
    //kprintf("Wait samehandle wake thread %d reason: %d\n", tid, reason);

    spinlockAcquire(&spinlock);
    ++returnedThreads;
    //kprintf("Thread samehandle returned: %d\n", returnedThreads);
    spinlockRelease(&spinlock);

    return NULL;
}

static void testOrder(void)
{
    kernel_thread_t* pWaitThreads[10];
    kernel_thread_t* pWakeThreads[10];
    uintptr_t i;
    OS_RETURN_E error;

    orderFutex.pHandle = &orderWait;
    orderFutex.isAlive = true;
    orderVal = 0;

    /* Spawn the wait thread */
    for(i = 0; i < 10; ++i)
    {
        error = schedCreateThread(&pWaitThreads[i], true,
                                0,
                                "FUTEX_WAIT_ORDER_TEST",
                                0x1000,
                                (1ULL << (i % SOC_CPU_COUNT)),
                                testOrderRoutineWait,
                                (void*)i);

        TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_CREATE_THREADS(i + 1),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KFUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto FUTEX_TEST_END;
        }
    }

    for(i = 0; i < 10; ++i)
    {
        error = schedCreateThread(&pWakeThreads[i], true,
                                0,
                                "FUTEX_WAKE_ORDER_TEST",
                                0x1000,
                                (1ULL << (i % SOC_CPU_COUNT)),
                                testOrderRoutineWake,
                                (void*)i);

        TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_CREATE_THREADS0(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KFUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto FUTEX_TEST_END;
        }
    }

    /* Wait threads */
    for(i = 0; i < 10; ++i)
    {
        error = schedJoinThread(pWaitThreads[i], NULL, NULL);
        TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_JOIN_THREADS(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KFUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto FUTEX_TEST_END;
        }
    }
    for(i = 0; i < 10; ++i)
    {
        error = schedJoinThread(pWakeThreads[i], NULL, NULL);
        TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_JOIN_THREADS0(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KFUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto FUTEX_TEST_END;
        }
    }

    return;

FUTEX_TEST_END:
    TEST_FRAMEWORK_END();
}

static void testMultiple(void)
{
    kernel_thread_t* pThreads[10];
    OS_RETURN_E error;
    uint32_t i;
    size_t wakeCount;

    multipleFutex.pHandle = &multipleFutexValue;
    multipleFutex.isAlive = true;

    returnedThreads = 0;
    multipleFutexValue = 0;

    /* Spawn the wait thread */
    for(i = 0; i < 10; ++i)
    {
        error = schedCreateThread(&pThreads[i], true,
                                0,
                                "FUTEX_MULTIPLE_TEST",
                                0x1000,
                                (1ULL << (i % SOC_CPU_COUNT)),
                                testWaitMultiple,
                                (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_MULTIPLE_CREATE_THREADS(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KFUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto FUTEX_TEST_END;
        }
    }

    error = schedSleep(2000000000ULL);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_MULTIPLE_SLEEP0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto FUTEX_TEST_END;
    }

    multipleFutexValue = 1;
    wakeCount = 5;
    error = kfutexWake(&multipleFutex, &wakeCount);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_MULTIPLE_WAKE,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        kprintf("ERROR Wake! in main %d\n", error);
        goto FUTEX_TEST_END;
    }

    kprintf("Waiting for test to end\n");

    error = schedSleep(2000000000ULL);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_MULTIPLE_SLEEP1,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto FUTEX_TEST_END;
    }

    TEST_POINT_ASSERT_UINT(TEST_KFUTEX_MULTIPLE_VALUE_RET,
                           returnedThreads == 5,
                           5,
                           returnedThreads,
                           TEST_KFUTEX_ENABLED);



    wakeCount = 5;
    error = kfutexWake(&multipleFutex, &wakeCount);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_MULTIPLE_WAKE1,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        kprintf("ERROR Wake! in main %d\n", error);
        goto FUTEX_TEST_END;
    }

    kprintf("Waiting for test to end\n");

    error = schedSleep(2000000000ULL);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_MULTIPLE_SLEEP2,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto FUTEX_TEST_END;
    }

    TEST_POINT_ASSERT_UINT(TEST_KFUTEX_MULTIPLE_VALUE_RET1,
                           returnedThreads == 10,
                           10,
                           returnedThreads,
                           TEST_KFUTEX_ENABLED);


    return;

FUTEX_TEST_END:
    TEST_FRAMEWORK_END();
}

static void testSameHandleValue(void)
{
    kernel_thread_t* pThreads[100];
    OS_RETURN_E error;
    uint32_t i;
    size_t wakeCount;

    multipleFutex.pHandle = &multipleFutexValue;
    multipleFutex.isAlive = true;

    multipleFutexValue = 0;
    returnedThreads = 0;

    /* Spawn the wait thread */
    for(i = 0; i < 100; ++i)
    {
        error = schedCreateThread(&pThreads[i], true,
                                0,
                                "FUTEX_SAMEHANDLE_TEST",
                                0x1000,
                                (1ULL << (i % SOC_CPU_COUNT)),
                                testWaitSameHandleValue,
                                (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_SAMEHANDLE_CREATE_THREADS(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KFUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto FUTEX_TEST_END;
        }
    }

    error = schedSleep(2000000000ULL);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_SAMEHANDLE_SLEEP0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto FUTEX_TEST_END;
    }

    multipleFutexValue = 0;
    wakeCount = 100;
    error = kfutexWake(&multipleFutex, &wakeCount);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_SAMEHANDLE_WAKE,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        kprintf("ERROR Wake! in main %d\n", error);
        goto FUTEX_TEST_END;
    }

    kprintf("Waiting for test to end\n");

    error = schedSleep(5000000000ULL);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_SAMEHANDLE_SLEEP1,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto FUTEX_TEST_END;
    }

    TEST_POINT_ASSERT_UINT(TEST_KFUTEX_SAMEHANDLE_VALUE_RET,
                           returnedThreads == 0,
                           0,
                           returnedThreads,
                           TEST_KFUTEX_ENABLED);

    kprintf("Actually waking now\n");

    multipleFutexValue = 1;
    wakeCount = 100;
    error = kfutexWake(&multipleFutex, &wakeCount);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_SAMEHANDLE_WAKE1,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        kprintf("ERROR Wake! in main %d\n", error);
        goto FUTEX_TEST_END;
    }

    kprintf("Waiting for test to end\n");

    error = schedSleep(1000000000ULL);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_SAMEHANDLE_SLEEP2,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto FUTEX_TEST_END;
    }

    TEST_POINT_ASSERT_UINT(TEST_KFUTEX_SAMEHANDLE_VALUE_RET1,
                           returnedThreads == 100,
                           100,
                           returnedThreads,
                           TEST_KFUTEX_ENABLED);
    return;

FUTEX_TEST_END:
    TEST_FRAMEWORK_END();
}

static void testReleaseResources(void)
{
    kernel_thread_t* pThreads[10];
    OS_RETURN_E error;
    uint32_t i;
    uintptr_t identifier;
    void* value;
    size_t wakeCount;

    multipleFutex.pHandle = &multipleFutexValue;
    multipleFutex.isAlive = true;

    multipleFutexValue = 0;
    returnedThreads = 0;

    identifier = memoryMgrGetPhysAddr((uintptr_t)&multipleFutexValue,
                                      schedGetCurrentProcess(),
                                      NULL);
    if(identifier == MEMMGR_PHYS_ADDR_ERROR)
    {
        TEST_POINT_ASSERT_POINTER(TEST_KFUTEX_RELEASE_GET_ID,
                                  identifier != MEMMGR_PHYS_ADDR_ERROR,
                                  identifier,
                                  0,
                                  TEST_KFUTEX_ENABLED);
        goto FUTEX_TEST_END;
    }


    /* Spawn the wait thread */
    for(i = 0; i < 10; ++i)
    {
        error = schedCreateThread(&pThreads[i], true,
                                0,
                                "FUTEX_RELEASE_TEST",
                                0x1000,
                                (1ULL << (i % SOC_CPU_COUNT)),
                                testWaitReleaseResources,
                                (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_RELEASE_CREATE_THREADS(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_KFUTEX_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto FUTEX_TEST_END;
        }
    }

    error = schedSleep(2000000000ULL);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_RELEASE_SLEEP0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto FUTEX_TEST_END;
    }

    multipleFutexValue = 1;
    wakeCount = 10;
    error = kfutexWake(&multipleFutex, &wakeCount);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_RELEASE_WAKE,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);

    error = schedSleep(2000000000ULL);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_RELEASE_SLEEP1,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto FUTEX_TEST_END;
    }

    TEST_POINT_ASSERT_UINT(TEST_KFUTEX_RELEASE_VALUE_RET0,
                           returnedThreads == 10,
                           10,
                           returnedThreads,
                           TEST_KFUTEX_ENABLED);

    error = uhashtableGet(spFutexTable, identifier, &value);

    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_RELEASE_GET_TABLE0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);

    multipleFutexValue = 0;
    returnedThreads = 0;
    error = schedCreateThread(&pThreads[0], true,
                            0,
                            "FUTEX_RELEASE_TEST",
                            0x1000,
                            0,
                            testWaitReleaseResources,
                            (void*)(uintptr_t)10);

    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_RELEASE_CREATE_THREADS0,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto FUTEX_TEST_END;
    }

    error = schedSleep(2000000000ULL);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_RELEASE_SLEEP2,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto FUTEX_TEST_END;
    }

    multipleFutexValue = 1;
    multipleFutex.isAlive = false;
    wakeCount = 10;
    error = kfutexWake(&multipleFutex, &wakeCount);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_RELEASE_WAKE1,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);

    error = schedSleep(2000000000ULL);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_RELEASE_SLEEP3,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto FUTEX_TEST_END;
    }

    TEST_POINT_ASSERT_UINT(TEST_KFUTEX_RELEASE_VALUE_RET1,
                           returnedThreads == 1,
                           1,
                           returnedThreads,
                           TEST_KFUTEX_ENABLED);

    error = uhashtableGet(spFutexTable, identifier, &value);

    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_RELEASE_GET_TABLE1,
                            error == OS_ERR_NO_SUCH_ID,
                            OS_ERR_NO_SUCH_ID,
                            error,
                            TEST_KFUTEX_ENABLED);

    return;

FUTEX_TEST_END:
    TEST_FRAMEWORK_END();
}

static void* testThread(void* args)
{
    (void)args;

    testOrder();
    kprintf("Order Test Done\n");
    testMultiple();
    kprintf("Multiple Test Done\n");
    testSameHandleValue();
    kprintf("Same Handle Test Done\n");
    testReleaseResources();
    kprintf("Release Test Done\n");

    TEST_FRAMEWORK_END();

    return NULL;
}

void kfutexTest(void)
{
    OS_RETURN_E error;
    kernel_thread_t* pTestThread;

    /* Spawn the test thread */
    error = schedCreateThread(&pTestThread, true,
                                    0,
                                    "FUTEX_MAIN_TEST",
                                    0x1000,
                                    1,
                                    testThread,
                                    NULL);
    TEST_POINT_ASSERT_RCODE(TEST_KFUTEX_CREATE_THREADS(0),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_KFUTEX_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto FUTEX_TEST_END;
    }

FUTEX_TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/