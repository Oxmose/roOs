
/*******************************************************************************
 * @file criticalTest.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 11/09/2024
 *
 * @version 1.0
 *
 * @brief Testing framework critical sections testing.
 *
 * @details Testing framework critical sections testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <cpu.h>
#include <stdint.h>
#include <critical.h>
#include <scheduler.h>
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
static volatile uint64_t criticalValueTest;
static kernel_spinlock_t lock = KERNEL_SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

static void* criticalLocalRoutine(void* args);
static void* criticalGlobalRoutine0(void* args);
static void* criticalGlobalRoutine1(void* args);
static void testLocal(void);
static void testGlobal0(void);
static void testGlobal1(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void* criticalLocalRoutine(void* args)
{
    uint32_t tid;
    uint32_t i;
    volatile uint32_t j;
    uint64_t save;
    uint32_t intState;

    tid = (uintptr_t)args;

    kprintf("Thread %d on CPU %d\n", tid, cpuGetId());

    for(i = 0; i < 1000000; ++i)
    {
        KERNEL_ENTER_CRITICAL_LOCAL(intState);
        save = criticalValueTest;
        for(j = 0; j < 100; ++j);
        criticalValueTest = save + 1;
        KERNEL_EXIT_CRITICAL_LOCAL(intState);
    }

    return NULL;
}

static void* criticalGlobalRoutine0(void* args)
{
    uint32_t tid;
    uint32_t i;
    volatile uint32_t j;
    uint64_t save;

    tid = (uintptr_t)args;

    kprintf("Thread %d on CPU %d\n", tid, cpuGetId());

    for(i = 0; i < 1000000; ++i)
    {
        KERNEL_LOCK(lock);
        save = criticalValueTest;
        for(j = 0; j < 100; ++j);
        criticalValueTest = save + 1;
        KERNEL_UNLOCK(lock);
    }

    return NULL;
}

static void* criticalGlobalRoutine1(void* args)
{
    uint32_t tid;
    uint32_t i;
    volatile uint32_t j;
    uint64_t save;

    tid = (uintptr_t)args;

    kprintf("Thread %d on CPU %d\n", tid, cpuGetId());

    for(i = 0; i < 1000000; ++i)
    {
        KernelLock(&lock);
        save = criticalValueTest;
        for(j = 0; j < 100; ++j);
        criticalValueTest = save + 1;
        KernelUnlock(&lock);
    }

    return NULL;
}

static void testLocal(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[10];

    criticalValueTest = 0;

    for(i = 0; i < 10; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        1,
                                        "LOCAL_CRITICAL_TEST",
                                        0x1000,
                                        0x1,
                                        criticalLocalRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_CREATE_THREADS_LOCAL(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_CRITICAL_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto TEST_END;
        }
    }

    for(i = 0; i < 10; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_JOIN_THREADS_LOCAL(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_CRITICAL_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto TEST_END;
        }
    }

    TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_LOCAL,
                           criticalValueTest == 10 * 1000000,
                           10 * 1000000,
                           criticalValueTest,
                           TEST_CRITICAL_ENABLED);

TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void testGlobal0(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[10];

    criticalValueTest = 0;

    for(i = 0; i < 10; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        1,
                                        "GLOBAL_CRITICAL_TEST",
                                        0x1000,
                                        (1 << (i % SOC_CPU_COUNT)),
                                        criticalGlobalRoutine0,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_CREATE_THREADS_GLOBAL0(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_CRITICAL_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto TEST_END;
        }
    }

    for(i = 0; i < 10; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_JOIN_THREADS_GLOBAL0(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_CRITICAL_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto TEST_END;
        }
    }

    TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_GLOBAL0,
                           criticalValueTest == 10 * 1000000,
                           10 * 1000000,
                           criticalValueTest,
                           TEST_CRITICAL_ENABLED);

TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void testGlobal1(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[10];

    criticalValueTest = 0;

    for(i = 0; i < 10; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        1,
                                        "GLOBAL_CRITICAL_TEST",
                                        0x1000,
                                        (1 << (i % SOC_CPU_COUNT)),
                                        criticalGlobalRoutine1,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_CREATE_THREADS_GLOBAL1(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_CRITICAL_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto TEST_END;
        }
    }

    for(i = 0; i < 10; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_JOIN_THREADS_GLOBAL1(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_CRITICAL_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto TEST_END;
        }
    }

    TEST_POINT_ASSERT_UINT(TEST_CRITICAL_VALUE_GLOBAL1,
                           criticalValueTest == 10 * 1000000,
                           10 * 1000000,
                           criticalValueTest,
                           TEST_CRITICAL_ENABLED);

TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void* testThread(void* args)
{
    (void)args;

    testLocal();
    kprintf("Local critical done\n");
    testGlobal0();
    kprintf("Global0 critical done\n");
    testGlobal1();
    kprintf("Global1 critical done\n");
    TEST_FRAMEWORK_END();

    return NULL;
}

void criticalTest(void)
{
    OS_RETURN_E error;
    kernel_thread_t* pTestThread;

    /* Spawn the test thread */
    error = schedCreateKernelThread(&pTestThread,
                                    0,
                                    "CRITICAL_TEST",
                                    0x1000,
                                    1,
                                    testThread,
                                    NULL);
    TEST_POINT_ASSERT_RCODE(TEST_CRITICAL_CREATE_TEST,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_CRITICAL_ENABLED);
    if(error != OS_NO_ERR)
    {
        goto TEST_END;
    }

TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/