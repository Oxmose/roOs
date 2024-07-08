
/*******************************************************************************
 * @file signalTests.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 07/07/2024
 *
 * @version 1.0
 *
 * @brief Testing framework signal testing.
 *
 * @details Testing framework signal testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <signal.h>
#include <kerneloutput.h>
#include <scheduler.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <test_framework.h>


/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define TEST_DIV_BY_ZERO        0
#define TEST_DIV_BY_ZERO_RET    100
#define TEST_SEGFAULT           1
#define TEST_SEGFAULT_RET       200
#define TEST_ILLEGAL_INST       2
#define TEST_ILLEGAL_INST_RET   300
#define TEST_SIGNAL_SELF        3
#define TEST_SIGNAL_SELF_RET    400
#define TEST_SIGNAL_REGULAR     4
#define TEST_SIGNAL_REGULAR_RET 500

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
static kernel_thread_t* pNewThread;
static volatile kernel_thread_t* pNewThreadHandle;
static uint32_t retValue;
static volatile uint32_t lastVal = 0;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void signalHandlerDivZero(void)
{
    kprintf("Div by zero handler\n");

    /* Check we are in the correct thread */
    TEST_POINT_ASSERT_POINTER(TEST_SIGNAL_DIV_ZERO_THREAD,
                          schedGetCurrentThread() == pNewThreadHandle,
                          (uintptr_t)pNewThreadHandle,
                          (uintptr_t)schedGetCurrentThread(),
                          TEST_SIGNAL_ENABLED);

    retValue = TEST_DIV_BY_ZERO_RET;
    schedThreadExit(THREAD_TERMINATE_CORRECTLY,
                    THREAD_RETURN_STATE_RETURNED,
                    NULL);
}

static void signalHandlerSegfault(void)
{
    kprintf("Segfault handler\n");
    /* Check we are in the correct thread */
    TEST_POINT_ASSERT_POINTER(TEST_SIGNAL_SEGFAULT_THREAD,
                          schedGetCurrentThread() == pNewThreadHandle,
                          (uintptr_t)pNewThreadHandle,
                          (uintptr_t)schedGetCurrentThread(),
                          TEST_SIGNAL_ENABLED);

    retValue = TEST_SEGFAULT_RET;
    schedThreadExit(THREAD_TERMINATE_CORRECTLY,
                    THREAD_RETURN_STATE_RETURNED,
                    NULL);

}

static void signalHandlerIllegalInst(void)
{
    kprintf("Illegal instruction handler\n");
    /* Check we are in the correct thread */
    TEST_POINT_ASSERT_POINTER(TEST_SIGNAL_ILLEGAL_INST_THREAD,
                          schedGetCurrentThread() == pNewThreadHandle,
                          (uintptr_t)pNewThreadHandle,
                          (uintptr_t)schedGetCurrentThread(),
                          TEST_SIGNAL_ENABLED);

    retValue = TEST_ILLEGAL_INST_RET;
    schedThreadExit(THREAD_TERMINATE_CORRECTLY,
                    THREAD_RETURN_STATE_RETURNED,
                    NULL);
}

static void signalHandlerSelf(void)
{
    kprintf("Self signal handler\n");
    /* Check we are in the correct thread */
    TEST_POINT_ASSERT_POINTER(TEST_SIGNAL_SELF_SIGNAL_THREAD,
                          schedGetCurrentThread() == pNewThreadHandle,
                          (uintptr_t)pNewThreadHandle,
                          (uintptr_t)schedGetCurrentThread(),
                          TEST_SIGNAL_ENABLED);

    retValue = TEST_SIGNAL_SELF_RET;
    schedThreadExit(THREAD_TERMINATE_CORRECTLY,
                    THREAD_RETURN_STATE_RETURNED,
                    NULL);

}

static void signalHandlerRegular(void)
{
    kprintf("Regular handler\n");
    /* Check we are in the correct thread */
    TEST_POINT_ASSERT_POINTER(TEST_SIGNAL_REGULAR_THREAD,
                          schedGetCurrentThread() == pNewThreadHandle,
                          (uintptr_t)pNewThreadHandle,
                          (uintptr_t)schedGetCurrentThread(),
                          TEST_SIGNAL_ENABLED);

    retValue = TEST_SIGNAL_REGULAR_RET;
    schedThreadExit(THREAD_TERMINATE_CORRECTLY,
                    THREAD_RETURN_STATE_RETURNED,
                    NULL);
}

static void* otherThread(void* args)
{
    volatile uintptr_t value = 0;
    OS_RETURN_E error;

    pNewThreadHandle = schedGetCurrentThread();

    kprintf("Registering signals\n");

    error = signalRegister(THREAD_SIGNAL_ILL, signalHandlerIllegalInst);
    error |= signalRegister(THREAD_SIGNAL_FPE, signalHandlerDivZero);
    error |= signalRegister(THREAD_SIGNAL_USR1, signalHandlerSelf);
    error |= signalRegister(THREAD_SIGNAL_SEGV, signalHandlerSegfault);
    error |= signalRegister(THREAD_SIGNAL_USR2, signalHandlerRegular);
    TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_REGISTER(++lastVal),
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);

    kprintf("Generating signal\n");
    switch((uintptr_t)args)
    {
        case TEST_DIV_BY_ZERO:
            value = value / value;
            break;
        case TEST_SEGFAULT:
            value = *(uintptr_t*)value;
            break;
        case TEST_ILLEGAL_INST:
            __asm__ __volatile__("ud2");
            break;
        case TEST_SIGNAL_SELF:
            error = signalThread(schedGetCurrentThread(), THREAD_SIGNAL_USR1);
            TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_SIGNAL_SELF_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);
            if(error != OS_NO_ERR)
            {
                return NULL;
            }
            break;
        default:
            break;
    }

    while(1)
    {
        schedSleep(10000000);
    }

    return NULL;
}

static void* testThread(void* args)
{
    OS_RETURN_E error;
    (void)args;

    /* Division by zero */
    kprintf("Div by zero signal\n");
    error = schedCreateKernelThread(&pNewThread,
                                    1,
                                    "DEF_SIG_HAND",
                                    0x1000,
                                    1,
                                    otherThread,
                                    (void*)TEST_DIV_BY_ZERO);
    TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_CREATE_DIV_ZERO_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);
    error = schedJoinThread(pNewThread, NULL, NULL);
    TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_JOIN_DIV_ZERO_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_SIGNAL_RETVAL_DIV_ZERO,
                           retValue == TEST_DIV_BY_ZERO_RET,
                           TEST_DIV_BY_ZERO_RET,
                           retValue,
                           TEST_SIGNAL_ENABLED);

    /* Segfault */
    kprintf("Segfault signal\n");
    error = schedCreateKernelThread(&pNewThread,
                                    1,
                                    "DEF_SIG_HAND",
                                    0x1000,
                                    1,
                                    otherThread,
                                    (void*)TEST_SEGFAULT);
    TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_CREATE_SEGFAULT_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);
    error = schedJoinThread(pNewThread, NULL, NULL);
    TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_JOIN_SEGFAULT_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_SIGNAL_RETVAL_SEGFAULT,
                           retValue == TEST_SEGFAULT_RET,
                           TEST_SEGFAULT_RET,
                           retValue,
                           TEST_SIGNAL_ENABLED);

    /* Illegal instruction */
    kprintf("Illegal instruction signal\n");
    error = schedCreateKernelThread(&pNewThread,
                                    1,
                                    "DEF_SIG_HAND",
                                    0x1000,
                                    1,
                                    otherThread,
                                    (void*)TEST_ILLEGAL_INST);
    TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_CREATE_ILLEGAL_INST_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);
    error = schedJoinThread(pNewThread, NULL, NULL);
    TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_JOIN_ILLEGAL_INST_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_SIGNAL_RETVAL_ILLEGAL_INST,
                           retValue == TEST_ILLEGAL_INST_RET,
                           TEST_ILLEGAL_INST_RET,
                           retValue,
                           TEST_SIGNAL_ENABLED);

    /* Custom signal */
    kprintf("Custom signal\n");
    error = schedCreateKernelThread(&pNewThread,
                                    1,
                                    "DEF_SIG_HAND",
                                    0x1000,
                                    1,
                                    otherThread,
                                    (void*)TEST_SIGNAL_REGULAR);
    TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_CREATE_REGULAR_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);
    schedSleep(1000000000);
    error = signalThread(pNewThread, THREAD_SIGNAL_USR2);
    TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_SIGNAL_REGULAR_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);
    error = schedJoinThread(pNewThread, NULL, NULL);
    TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_JOIN_REGULAR_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_SIGNAL_RETVAL_REGULAR,
                           retValue == TEST_SIGNAL_REGULAR_RET,
                           TEST_SIGNAL_REGULAR_RET,
                           retValue,
                           TEST_SIGNAL_ENABLED);

    /* Self signal */
    kprintf("Self signal\n");
    error = schedCreateKernelThread(&pNewThread,
                                    1,
                                    "DEF_SIG_HAND",
                                    0x1000,
                                    1,
                                    otherThread,
                                    (void*)TEST_SIGNAL_SELF);
    TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_CREATE_SIGNAL_SELF_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);
    error = schedJoinThread(pNewThread, NULL, NULL);
    TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_JOIN_SIGNAL_SELF_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_SIGNAL_RETVAL_SIGNAL_SELF,
                           retValue == TEST_SIGNAL_SELF_RET,
                           TEST_SIGNAL_SELF_RET,
                           retValue,
                           TEST_SIGNAL_ENABLED);


    TEST_FRAMEWORK_END();
    return NULL;
}

void signalTest(void)
{
    OS_RETURN_E error;
    kernel_thread_t* pTestThread;

    /* Spawn the test thread */
    error = schedCreateKernelThread(&pTestThread,
                                    1,
                                    "DEF_SIG_MAIN",
                                    0x1000,
                                    1,
                                    testThread,
                                    NULL);
    TEST_POINT_ASSERT_RCODE(TEST_SIGNAL_CREATE_MAIN_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_SIGNAL_ENABLED);
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