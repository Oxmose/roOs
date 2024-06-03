
/*******************************************************************************
 * @file interrupt_test.c
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
        counter += curr_thread->vCpu.intContext.intId;
    }
}

static void decrementer_handler(kernel_thread_t* curr_thread)
{
    if(counter > 0)
    {
        counter -= curr_thread->vCpu.intContext.intId;
    }
}

static void test_sw_interupts_lock(void)
{
    OS_RETURN_E err;
    uint32_t    int_state;
    uint32_t    cnt_val;

    err = interruptRegister(MIN_INTERRUPT_LINE,
                                                incrementer_handler);
    TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_LOCK_REG_HANDLER0_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_INTERRUPT_ENABLED);

    err = interruptRegister(MIN_INTERRUPT_LINE + 1,
                                                decrementer_handler);
    TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_LOCK_REG_HANDLER1_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_INTERRUPT_ENABLED);

    /* Should not raise any interrupt */
    cnt_val = counter;
    __asm__ __volatile__("int %0" :: "i" (0x80));
    __asm__ __volatile__("int %0" :: "i" (0x80));
    __asm__ __volatile__("int %0" :: "i" (0x80));

    TEST_POINT_ASSERT_UINT(TEST_INTERRUPT_SW_LOCK_CHECK0_ID,
                           cnt_val == counter,
                           cnt_val,
                           counter,
                           TEST_INTERRUPT_ENABLED);

    interruptRestore(1);

    TEST_POINT_ASSERT_UINT(TEST_INTERRUPT_SW_LOCK_CHECK1_ID,
                           cnt_val == counter,
                           cnt_val,
                           counter,
                           TEST_INTERRUPT_ENABLED);


    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE));

    TEST_POINT_ASSERT_UINT(TEST_INTERRUPT_SW_LOCK_CHECK2_ID,
                           cnt_val + 3 * MIN_INTERRUPT_LINE == counter,
                           cnt_val + 3 * MIN_INTERRUPT_LINE,
                           counter,
                           TEST_INTERRUPT_ENABLED);

    cnt_val = counter;

    interruptDisable();
    int_state = 0;

    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE));

    TEST_POINT_ASSERT_UINT(TEST_INTERRUPT_SW_LOCK_CHECK3_ID,
                           cnt_val == counter,
                           cnt_val,
                           counter,
                           TEST_INTERRUPT_ENABLED);

    interruptRestore(int_state);

    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE));

    TEST_POINT_ASSERT_UINT(TEST_INTERRUPT_SW_LOCK_CHECK4_ID,
                           cnt_val == counter,
                           cnt_val,
                           counter,
                           TEST_INTERRUPT_ENABLED);

    interruptRestore(int_state);

    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE));

    TEST_POINT_ASSERT_UINT(TEST_INTERRUPT_SW_LOCK_CHECK5_ID,
                           cnt_val == counter,
                           cnt_val,
                           counter,
                           TEST_INTERRUPT_ENABLED);

    interruptRestore(1);

    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE));

    TEST_POINT_ASSERT_UINT(TEST_INTERRUPT_SW_LOCK_CHECK6_ID,
                           cnt_val + MIN_INTERRUPT_LINE == counter,
                           cnt_val,
                           counter,
                           TEST_INTERRUPT_ENABLED);

    interruptRestore(1);
    interruptRestore(1);
    int_state = interruptDisable();

    cnt_val = counter;

    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE));

    TEST_POINT_ASSERT_UINT(TEST_INTERRUPT_SW_LOCK_CHECK7_ID,
                           cnt_val == counter,
                           cnt_val,
                           counter,
                           TEST_INTERRUPT_ENABLED);


    err = interruptRemove(MIN_INTERRUPT_LINE);
    TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_LOCK_REM_HANDLER0_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_INTERRUPT_ENABLED);

    err = interruptRemove(MIN_INTERRUPT_LINE + 1);
    TEST_POINT_ASSERT_RCODE(TEST_INTERRUPT_SW_LOCK_REM_HANDLER1_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_INTERRUPT_ENABLED);
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
        if(i == PANIC_INT_LINE ||
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

    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 0));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 1));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 2));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 3));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 4));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 5));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 6));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 7));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 8));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 9));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 11));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 12));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 13));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 14));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 15));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 16));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 17));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 18));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 19));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 20));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 21));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 22));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 24));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 25));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 26));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 27));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 28));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 29));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 30));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 32));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 33));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 34));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 35));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 36));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 37));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 38));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 39));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 40));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 41));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 42));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 43));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 44));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 45));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 46));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 47));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 48));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 49));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 50));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 51));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 52));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 53));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 54));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 55));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 56));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 57));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 58));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 59));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 60));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 61));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 62));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 63));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 64));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 65));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 66));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 67));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 68));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 69));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 70));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 71));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 72));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 73));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 74));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 75));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 76));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 77));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 78));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 79));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 80));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 81));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 82));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 83));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 84));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 85));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 86));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 87));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 88));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 89));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 90));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 91));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 92));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 93));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 94));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 95));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 96));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 97));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 98));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 99));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 100));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 101));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 102));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 103));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 104));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 105));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 106));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 107));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 108));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 109));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 110));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 111));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 112));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 113));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 114));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 115));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 116));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 117));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 118));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 119));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 120));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 121));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 122));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 123));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 124));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 125));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 126));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 127));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 128));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 129));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 130));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 131));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 132));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 133));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 134));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 135));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 136));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 137));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 138));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 139));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 140));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 141));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 142));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 143));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 144));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 145));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 146));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 147));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 148));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 149));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 150));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 151));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 152));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 153));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 154));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 155));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 156));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 157));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 158));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 159));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 160));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 161));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 162));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 163));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 164));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 165));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 166));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 167));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 168));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 169));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 170));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 171));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 172));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 173));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 174));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 175));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 176));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 177));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 178));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 179));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 180));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 181));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 182));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 183));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 184));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 185));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 186));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 187));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 188));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 189));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 190));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 191));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 192));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 193));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 194));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 195));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 196));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 197));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 198));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 199));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 200));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 201));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 202));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 203));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 204));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 205));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 206));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 207));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 208));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 209));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 210));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 211));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 212));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 213));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 214));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 215));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 216));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 217));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 218));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 219));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 220));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 221));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 222));

    int_state = interruptDisable();

    TEST_POINT_ASSERT_UINT(TEST_INTERRUPT_SW_COUNTER_CHECK0_ID,
                           cnt_val == counter,
                           cnt_val,
                           counter,
                           TEST_INTERRUPT_ENABLED);

    for(i = MIN_INTERRUPT_LINE; i <= MAX_INTERRUPT_LINE; ++i)
    {
        if(i == PANIC_INT_LINE ||
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
        if(i == PANIC_INT_LINE ||
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

    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 0));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 1));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 2));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 3));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 4));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 5));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 6));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 7));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 8));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 9));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 11));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 12));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 13));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 14));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 15));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 16));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 17));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 18));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 19));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 20));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 21));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 22));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 24));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 25));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 26));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 27));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 28));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 29));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 30));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 32));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 33));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 34));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 35));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 36));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 37));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 38));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 39));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 40));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 41));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 42));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 43));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 44));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 45));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 46));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 47));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 48));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 49));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 50));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 51));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 52));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 53));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 54));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 55));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 56));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 57));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 58));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 59));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 60));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 61));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 62));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 63));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 64));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 65));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 66));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 67));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 68));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 69));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 70));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 71));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 72));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 73));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 74));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 75));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 76));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 77));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 78));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 79));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 80));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 81));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 82));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 83));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 84));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 85));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 86));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 87));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 88));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 89));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 90));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 91));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 92));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 93));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 94));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 95));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 96));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 97));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 98));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 99));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 100));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 101));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 102));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 103));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 104));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 105));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 106));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 107));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 108));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 109));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 110));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 111));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 112));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 113));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 114));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 115));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 116));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 117));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 118));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 119));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 120));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 121));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 122));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 123));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 124));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 125));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 126));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 127));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 128));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 129));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 130));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 131));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 132));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 133));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 134));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 135));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 136));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 137));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 138));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 139));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 140));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 141));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 142));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 143));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 144));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 145));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 146));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 147));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 148));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 149));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 150));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 151));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 152));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 153));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 154));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 155));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 156));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 157));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 158));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 159));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 160));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 161));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 162));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 163));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 164));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 165));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 166));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 167));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 168));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 169));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 170));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 171));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 172));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 173));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 174));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 175));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 176));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 177));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 178));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 179));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 180));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 181));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 182));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 183));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 184));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 185));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 186));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 187));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 188));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 189));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 190));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 191));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 192));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 193));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 194));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 195));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 196));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 197));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 198));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 199));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 200));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 201));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 202));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 203));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 204));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 205));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 206));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 207));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 208));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 209));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 210));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 211));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 212));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 213));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 214));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 215));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 216));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 217));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 218));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 219));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 220));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 221));
    __asm__ __volatile__("int %0" :: "i" (MIN_INTERRUPT_LINE + 222));

    int_state = interruptDisable();

    TEST_POINT_ASSERT_UINT(TEST_INTERRUPT_SW_COUNTER_CHECK1_ID,
                           cnt_val == counter,
                           cnt_val,
                           counter,
                           TEST_INTERRUPT_ENABLED);

    for(i = MIN_INTERRUPT_LINE; i <= MAX_INTERRUPT_LINE; ++i)
    {
        if(i == PANIC_INT_LINE ||
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

void interrupt_test(void)
{
    test_sw_interupts();
    test_sw_interupts_lock();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/