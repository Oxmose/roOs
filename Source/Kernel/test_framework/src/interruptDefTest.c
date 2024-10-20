
/*******************************************************************************
 * @file interruptTest.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 14/05/2023
 *
 * @version 1.0
 *
 * @brief Testing framework interrupt testing.
 *
 * @details Testing framework interrupt testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <interrupts.h>
#include <cpuInterrupt.h>
#include <panic.h>
#include <kerneloutput.h>
#include <cpu.h>
#include <x86cpu.h>
#include <scheduler.h>

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
static volatile uint32_t counter = 0;
static volatile int32_t mainTid;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
void defIntRoutine(void* args);
void* testThread(void* args);
void interruptDefferTest(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void defIntRoutine(void* args)
{
    uint32_t i;

    TEST_POINT_ASSERT_INT(TEST_DEF_TID_VALUE,
                          schedGetCurrentThread()->tid != mainTid,
                          mainTid,
                          schedGetCurrentThread()->tid,
                          TEST_DEF_INTERRUPT_ENABLED);

    TEST_POINT_ASSERT_INT(TEST_DEF_PRIORITY,
                          schedGetCurrentThread()->priority == KERNEL_HIGHEST_PRIORITY,
                          KERNEL_HIGHEST_PRIORITY,
                          schedGetCurrentThread()->priority,
                          TEST_DEF_INTERRUPT_ENABLED);

    TEST_POINT_ASSERT_POINTER(TEST_DEF_INT_DEFER_INT_ARGS,
                              (uintptr_t)args == (uintptr_t)42,
                              (uintptr_t)42,
                              (uintptr_t)args,
                              TEST_DEF_INTERRUPT_ENABLED);

    for(i = 0; i < 100000; ++i)
    {
        ++counter;
    }
}

void* testThread(void* args)
{
    OS_RETURN_E error;

    mainTid = schedGetCurrentThread()->tid;

    (void)args;
    error = interruptDeferIsr(NULL, (void*)(uintptr_t)42);

    TEST_POINT_ASSERT_RCODE(TEST_DEF_INT_DEFER_INT_NULL,
                            error == OS_ERR_NULL_POINTER,
                            OS_ERR_NULL_POINTER,
                            error,
                            TEST_DEF_INTERRUPT_ENABLED);

    error = interruptDeferIsr(defIntRoutine, (void*)(uintptr_t)42);

    TEST_POINT_ASSERT_RCODE(TEST_DEF_INT_DEFER_INT,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_DEF_INTERRUPT_ENABLED);

    /* Schedule so we let the defered interrupt execute */
    schedSchedule();

    while(counter != 100000){}

    TEST_POINT_ASSERT_UINT(TEST_DEF_INT_VALUE,
                           counter == 100000,
                           100000,
                           counter,
                           TEST_DEF_INTERRUPT_ENABLED);


    TEST_FRAMEWORK_END();
    return NULL;
}

void interruptDefferTest(void)
{
    OS_RETURN_E error;
    kernel_thread_t* pTestThread;

    /* Spawn the test thread */
    error = schedCreateThread(&pTestThread, true,
                                    1,
                                    "DEF_INT_MAIN",
                                    0x1000,
                                    1,
                                    testThread,
                                    NULL);
    TEST_POINT_ASSERT_RCODE(TEST_DEF_INT_CREATE_MAIN_THREAD,
                            error == OS_NO_ERR,
                            OS_NO_ERR,
                            error,
                            TEST_DEF_INTERRUPT_ENABLED);
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