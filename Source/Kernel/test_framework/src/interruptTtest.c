
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

/** @brief Master PIC spurious IRQ number. */
#define PIC_SPURIOUS_IRQ_MASTER 0x07
/** @brief Slave PIC spurious IRQ number. */
#define PIC_SPURIOUS_IRQ_SLAVE  0x0F

#define INT_PIC_IRQ_OFFSET 0x30

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

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void incrementer_handler(kernel_thread_t* curr_thread)
{
    (void)curr_thread;

    if(counter < UINT32_MAX)
    {
        counter += ((virtual_cpu_t*)(curr_thread->pVCpu))->intContext.intId;
    }
}

static void decrementer_handler(kernel_thread_t* curr_thread)
{
    if(counter > 0)
    {
        counter -= ((virtual_cpu_t*)(curr_thread->pVCpu))->intContext.intId;
    }
}

static void test_sw_interupts(void)
{
    uint32_t    cnt_val;
    uint32_t    i;
    uint32_t    int_state;
    OS_RETURN_E err;

    /* We just dont care about HW interrupt from PIC, disable them */
    _cpuOutB(0xFF, 0x21);
    _cpuOutB(0xFF, 0xA1);

    /* TEST REGISTER < MIN */
    err = interruptRegister(MIN_INTERRUPT_LINE - 1,
                                                incrementer_handler);
    TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_REG_BAD_HANDLER0_ID,
                            err == OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            err,
                            TEST_INTERRUPT_ENABLED);

    /* TEST REGISTER > MAX */
    err = interruptRegister(MAX_INTERRUPT_LINE + 1,
                                                incrementer_handler);
    TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_REG_BAD_HANDLER1_ID,
                            err == OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            err,
                            TEST_INTERRUPT_ENABLED);

    /* TEST REMOVE < MIN */
    err = interruptRemove(MIN_INTERRUPT_LINE - 1);
    TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_REM_BAD_HANDLER0_ID,
                            err == OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            err,
                            TEST_INTERRUPT_ENABLED);

    /* TEST REMOVE > MAX */
    err = interruptRemove(MAX_INTERRUPT_LINE + 1);
    TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_REM_BAD_HANDLER1_ID,
                            err == OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            OR_ERR_UNAUTHORIZED_INTERRUPT_LINE,
                            err,
                            TEST_INTERRUPT_ENABLED);

    /* TEST NULL HANDLER */
    err = interruptRegister(MIN_INTERRUPT_LINE, NULL);
    TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_REG_BAD_HANDLER2_ID,
                            err == OS_ERR_NULL_POINTER,
                            OS_ERR_NULL_POINTER,
                            err,
                            TEST_INTERRUPT_ENABLED);

    /* TEST REMOVE WHEN NOT REGISTERED */
    err = interruptRemove(MIN_INTERRUPT_LINE);
    TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_REM_BAD_HANDLER2_ID,
                            err == OS_ERR_INTERRUPT_NOT_REGISTERED,
                            OS_ERR_INTERRUPT_NOT_REGISTERED,
                            err,
                            TEST_INTERRUPT_ENABLED);

    /* TEST REGISTER WHEN ALREADY REGISTERED */
    err = interruptRegister(MIN_INTERRUPT_LINE,
                                               incrementer_handler);
    TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_REG_HANDLER0_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_INTERRUPT_ENABLED);

    err = interruptRegister(MIN_INTERRUPT_LINE,
                                                incrementer_handler);
    TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_REG_ALREADY_REG_HANDLER0_ID,
                            err == OS_ERR_INTERRUPT_ALREADY_REGISTERED,
                            OS_ERR_INTERRUPT_ALREADY_REGISTERED,
                            err,
                            TEST_INTERRUPT_ENABLED);

    /* INIT THINGS */
    err = interruptRemove(MIN_INTERRUPT_LINE);
    TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_REM_HANDLER0_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_INTERRUPT_ENABLED);

    cnt_val = 0;
    counter = 0;

    /* REGISTER FROM MIN TO MAX INC AND SEND INT, TEST COUNTER */
    for(i = MIN_INTERRUPT_LINE; i <= MAX_INTERRUPT_LINE; ++i)
    {
        if(i == PANIC_INT_LINE || i == SCHEDULER_SW_INT_LINE ||
           i == PIC_SPURIOUS_IRQ_MASTER + INT_PIC_IRQ_OFFSET ||
           i == PIC_SPURIOUS_IRQ_SLAVE + INT_PIC_IRQ_OFFSET ||
           i == 0xFF)
        {
            continue;
        }
        err = interruptRegister(i, incrementer_handler);
        TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_REG0_SWINT_HANDLER(i),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_INTERRUPT_ENABLED);
        cnt_val += i;
    }

    interruptRestore(1);

    for(i = MIN_INTERRUPT_LINE; i <= MAX_INTERRUPT_LINE; ++i)
    {
        if(i == PANIC_INT_LINE || i == SCHEDULER_SW_INT_LINE ||
           i == PIC_SPURIOUS_IRQ_MASTER + INT_PIC_IRQ_OFFSET ||
           i == PIC_SPURIOUS_IRQ_SLAVE + INT_PIC_IRQ_OFFSET ||
           i == 0xFF)
        {
            continue;
        }
        cpuRaiseInterrupt(i);
    }

    int_state = interruptDisable();

    TEST_POINT_ASSERT_UINT(TEST_INTERRUPT_SW_COUNTER_CHECK0_ID,
                           cnt_val == counter,
                           cnt_val,
                           counter,
                           TEST_INTERRUPT_ENABLED);

    for(i = MIN_INTERRUPT_LINE; i <= MAX_INTERRUPT_LINE; ++i)
    {
        if(i == PANIC_INT_LINE || i == SCHEDULER_SW_INT_LINE ||
           i == PIC_SPURIOUS_IRQ_MASTER + INT_PIC_IRQ_OFFSET ||
           i == PIC_SPURIOUS_IRQ_SLAVE + INT_PIC_IRQ_OFFSET ||
           i == 0xFF)
        {
            continue;
        }

        err = interruptRemove(i);
        TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_REM0_SWINT_HANDLER(i),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_INTERRUPT_ENABLED);
    }

    /* REGISTER FROM MIN TO MAX DEC AND SEND INT, TEST COUNTER */
    for(i = MIN_INTERRUPT_LINE; i <= MAX_INTERRUPT_LINE; ++i)
    {
        if(i == PANIC_INT_LINE || i == SCHEDULER_SW_INT_LINE ||
           i == PIC_SPURIOUS_IRQ_MASTER + INT_PIC_IRQ_OFFSET ||
           i == PIC_SPURIOUS_IRQ_SLAVE + INT_PIC_IRQ_OFFSET ||
           i == 0xFF)
        {
            continue;
        }
        err = interruptRegister(i, decrementer_handler);
        TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_REG1_SWINT_HANDLER(i),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_INTERRUPT_ENABLED);
        cnt_val -= i;
    }

    interruptRestore(int_state);
    for(i = MIN_INTERRUPT_LINE; i <= MAX_INTERRUPT_LINE; ++i)
    {
        if(i == PANIC_INT_LINE || i == SCHEDULER_SW_INT_LINE ||
           i == PIC_SPURIOUS_IRQ_MASTER + INT_PIC_IRQ_OFFSET ||
           i == PIC_SPURIOUS_IRQ_SLAVE + INT_PIC_IRQ_OFFSET ||
           i == 0xFF)
        {
            continue;
        }
        cpuRaiseInterrupt(i);
    }

    int_state = interruptDisable();

    TEST_POINT_ASSERT_UINT(TEST_INTERRUPT_SW_COUNTER_CHECK1_ID,
                           cnt_val == counter,
                           cnt_val,
                           counter,
                           TEST_INTERRUPT_ENABLED);

    for(i = MIN_INTERRUPT_LINE; i <= MAX_INTERRUPT_LINE; ++i)
    {
        if(i == PANIC_INT_LINE || i == SCHEDULER_SW_INT_LINE ||
           i == PIC_SPURIOUS_IRQ_MASTER + INT_PIC_IRQ_OFFSET ||
           i == PIC_SPURIOUS_IRQ_SLAVE + INT_PIC_IRQ_OFFSET ||
           i == 0xFF)
        {
            continue;
        }

        err = interruptRemove(i);
        TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_REM1_SWINT_HANDLER(i),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_INTERRUPT_ENABLED);
    }
}

void interruptTest(void)
{
    test_sw_interupts();

    TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/