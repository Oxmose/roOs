
/*******************************************************************************
 * @file atomicsTest.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 11/09/2024
 *
 * @version 1.0
 *
 * @brief Testing framework atomics testing.
 *
 * @details Testing framework atomics testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <critical.h>
#include <scheduler.h>
#include <stdint.h>
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
static volatile uint64_t lockValueTest;
static u32_atomic_t incValueTest;
static u32_atomic_t decValueTest;
static spinlock_t lock = SPINLOCK_INIT_VALUE;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

static void* spinlockTestRoutine(void* args);
static void* atomicIncRoutine(void* args);
static void* atomicDecRoutine(void* args);
static void testSpinlock(void);
static void testIncrement(void);
static void testDecrement(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void* spinlockTestRoutine(void* args)
{
    uint32_t tid;
    uint32_t i;
    volatile uint32_t j;
    uint64_t save;

    tid = (uintptr_t)args;

    kprintf("Thread %d on CPU %d\n", tid, cpuGetId());

    for(i = 0; i < 1000000; ++i)
    {
        spinlockAcquire(&lock);
        save = lockValueTest;
        for(j = 0; j < 100; ++j);
        lockValueTest = save + 1;
        spinlockRelease(&lock);
    }

    return NULL;
}

static void* atomicIncRoutine(void* args)
{
    uint32_t tid;
    uint32_t i;

    tid = (uintptr_t)args;

    kprintf("Thread %d on CPU %d\n", tid, cpuGetId());

    for(i = 0; i < 1000000; ++i)
    {
        atomicIncrement32(&incValueTest);
    }

    return NULL;
}

static void* atomicDecRoutine(void* args)
{
    uint32_t tid;
    uint32_t i;

    tid = (uintptr_t)args;

    kprintf("Thread %d on CPU %d\n", tid, cpuGetId());

    for(i = 0; i < 1000000; ++i)
    {
        atomicDecrement32(&decValueTest);
    }

    return NULL;
}

static void testSpinlock(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[10];

    lockValueTest = 0;

    for(i = 0; i < 10; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        1,
                                        "SPINLOCK_TEST",
                                        0x1000,
                                        (1 << (i % SOC_CPU_COUNT)),
                                        spinlockTestRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_ATOMICS_CREATE_THREADS_SPINLOCK(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_ATOMICS_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto TEST_END;
        }
    }

    for(i = 0; i < 10; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_ATOMICS_JOIN_THREADS_SPINLOCK(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_ATOMICS_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto TEST_END;
        }
    }

    TEST_POINT_ASSERT_UINT(TEST_ATOMICS_VALUE_SPINLOCK,
                           lockValueTest == 10 * 1000000,
                           10 * 1000000,
                           lockValueTest,
                           TEST_ATOMICS_ENABLED);

TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void testIncrement(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[10];

    incValueTest = 0;

    for(i = 0; i < 10; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        1,
                                        "ATOMIC_INC_TEST",
                                        0x1000,
                                        (1 << (i % SOC_CPU_COUNT)),
                                        atomicIncRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_ATOMICS_CREATE_THREADS_INC(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_ATOMICS_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto TEST_END;
        }
    }

    for(i = 0; i < 10; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_ATOMICS_JOIN_THREADS_INC(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_ATOMICS_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto TEST_END;
        }
    }

    TEST_POINT_ASSERT_UINT(TEST_ATOMICS_VALUE_INC,
                           incValueTest == 10 * 1000000,
                           10 * 1000000,
                           incValueTest,
                           TEST_ATOMICS_ENABLED);

TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void testDecrement(void)
{
    uint32_t i;
    OS_RETURN_E error;
    kernel_thread_t* pThreads[10];

    decValueTest = 10 * 1000000;

    for(i = 0; i < 10; ++i)
    {
        error = schedCreateKernelThread(&pThreads[i],
                                        1,
                                        "ATOMIC_DEC_TEST",
                                        0x1000,
                                        (1 << (i % SOC_CPU_COUNT)),
                                        atomicDecRoutine,
                                        (void*)(uintptr_t)i);

        TEST_POINT_ASSERT_RCODE(TEST_ATOMICS_CREATE_THREADS_DEC(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_ATOMICS_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto TEST_END;
        }
    }

    for(i = 0; i < 10; ++i)
    {
        error = schedJoinThread(pThreads[i], NULL, NULL);

        TEST_POINT_ASSERT_RCODE(TEST_ATOMICS_JOIN_THREADS_DEC(i),
                                error == OS_NO_ERR,
                                OS_NO_ERR,
                                error,
                                TEST_ATOMICS_ENABLED);
        if(error != OS_NO_ERR)
        {
            goto TEST_END;
        }
    }

    TEST_POINT_ASSERT_UINT(TEST_ATOMICS_VALUE_DEC,
                           decValueTest == 0,
                           0,
                           decValueTest,
                           TEST_ATOMICS_ENABLED);

TEST_END:
    if(error != OS_NO_ERR)
    {
        TEST_FRAMEWORK_END();
    }
}

static void* testThread(void* args)
{
    (void)args;

    testSpinlock();
    kprintf("Spinlock done\n");
    testIncrement();
    kprintf("Increment done\n");
    testDecrement();
    kprintf("Decrement done\n");
    TEST_FRAMEWORK_END();

    return NULL;
}

void atomicsTest(void)
{
    OS_RETURN_E error;
    kernel_thread_t* pTestThread;

    /* Spawn the test thread */
    error = schedCreateKernelThread(&pTestThread,
                                    0,
                                    "ATOMICS_TEST",
                                    0x1000,
                                    1,
                                    testThread,
                                    NULL);
    TEST_POINT_ASSERT_RCODE(TEST_ATOMICS_CREATE_TEST,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_ATOMICS_ENABLED);
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