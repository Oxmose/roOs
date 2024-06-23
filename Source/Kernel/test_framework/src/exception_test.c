
/*******************************************************************************
 * @file exception_test.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 14/05/2023
 *
 * @version 1.0
 *
 * @brief Testing framework exception testing.
 *
 * @details Testing framework exception testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <exceptions.h>
#include <cpu_interrupt.h>
#include <panic.h>
#include <kerneloutput.h>
#include <cpu.h>
#include <x86cpu.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <test_framework.h>


/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Divide by zero exception line. */
#define DIV_BY_ZERO_LINE           0x00

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

static void _end(void)
{
    TEST_POINT_ASSERT_RCODE(TEST_EXCEPTION_DIV_HANDLER1_ID,
                            TRUE,
                            TRUE,
                            TRUE,
                            TEST_EXCEPTION_ENABLED);
    TEST_FRAMEWORK_END();
}

static void _dummy(kernel_thread_t* curr_thread)
{
    /* Update the return of interrupt instruction pointer */
#ifdef ARCH_32_BITS
    ((virtual_cpu_t*)(curr_thread->pVCpu))->intContext.eip = (uintptr_t)_end;
#else
    ((virtual_cpu_t*)(curr_thread->pVCpu))->intContext.rip = (uintptr_t)_end;
#endif

    TEST_POINT_ASSERT_RCODE(TEST_EXCEPTION_DIV_HANDLER0_ID,
                            TRUE,
                            TRUE,
                            TRUE,
                            TEST_EXCEPTION_ENABLED);
}

void exception_test(void)
{
    OS_RETURN_E err;
    uint32_t minExc;
    uint32_t maxExc;

    const cpu_interrupt_config_t* pConfig;

    pConfig = cpuGetInterruptConfig();
    minExc = pConfig->minExceptionLine;
    maxExc = pConfig->maxExceptionLine;

    /* TEST REGISTER < MIN */
    err = exceptionRegister(minExc - 1, _dummy);
    TEST_POINT_ASSERT_RCODE(TEST_EXCEPTION_REGISTER_MIN_ID,
                            err == OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            err,
                            TEST_EXCEPTION_ENABLED);

    /* TEST REGISTER > MAX */
    err = exceptionRegister(maxExc + 1, _dummy);
    TEST_POINT_ASSERT_RCODE(TEST_EXCEPTION_REGISTER_MAX_ID,
                            err == OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            err,
                            TEST_EXCEPTION_ENABLED);

    /* TEST REMOVE < MIN */
    err = exceptionRemove(minExc - 1);
    TEST_POINT_ASSERT_RCODE(TEST_EXCEPTION_REMOVE_MIN_ID,
                            err == OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            err,
                            TEST_EXCEPTION_ENABLED);

    /* TEST REMOVE > MAX */
    err = exceptionRemove(maxExc + 1);
    TEST_POINT_ASSERT_RCODE(TEST_EXCEPTION_REMOVE_MAX_ID,
                            err == OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            err,
                            TEST_EXCEPTION_ENABLED);

    /* TEST NULL HANDLER */
    err = exceptionRegister(minExc, NULL);
    TEST_POINT_ASSERT_RCODE(TEST_EXCEPTION_REGISTER_NULL_ID,
                            err == OS_ERR_NULL_POINTER,
                            OS_ERR_NULL_POINTER,
                            err,
                            TEST_EXCEPTION_ENABLED);

    /* TEST REMOVE WHEN NOT REGISTERED */
    err = exceptionRemove(minExc);
    TEST_POINT_ASSERT_RCODE(TEST_EXCEPTION_REMOVE_REGISTERED_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_EXCEPTION_ENABLED);

     /* TEST REMOVE WHEN NOT REGISTERED */
    err = exceptionRemove(minExc);
    TEST_POINT_ASSERT_RCODE(TEST_EXCEPTION_REMOVE_NONREGISTERED_ID,
                            err == OS_ERR_INTERRUPT_NOT_REGISTERED,
                            OS_ERR_INTERRUPT_NOT_REGISTERED,
                            err,
                            TEST_EXCEPTION_ENABLED);

    /* TEST REGISTER WHEN ALREADY REGISTERED */
    err = exceptionRegister(minExc, _dummy);
    TEST_POINT_ASSERT_RCODE(TEST_EXCEPTION_REGISTER_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_EXCEPTION_ENABLED);

    err = exceptionRegister(DIV_BY_ZERO_LINE, _dummy);
    TEST_POINT_ASSERT_RCODE(TEST_EXCEPTION_ALREADY_REGISTERED_ID,
                            err == OS_ERR_INTERRUPT_ALREADY_REGISTERED,
                            OS_ERR_INTERRUPT_ALREADY_REGISTERED,
                            err,
                            TEST_EXCEPTION_ENABLED);


    /* Test exception */
    volatile int i = 0;
    volatile int m = 5 / i;
    (void)m;

    TEST_POINT_ASSERT_RCODE(TEST_EXCEPTION_NOT_CAUGHT_ID,
                            FALSE,
                            TRUE,
                            FALSE,
                            TEST_EXCEPTION_ENABLED);
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/