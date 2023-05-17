/******************************************************************************
 * @file test_list.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 10/05/2023
 *
 * @version 1.0
 *
 * @brief Testing framework functions and list.
 *
 * @details Testing framework functions and list. This file gathers the enable
 * flags for unit testing as well as the testing functions declarations.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __TEST_FRAMEWORK_TEST_LIST_H_
#define __TEST_FRAMEWORK_TEST_LIST_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/*************************************************
 * TESTING ENABLE FLAGS
 ************************************************/
#define TEST_INTERRUPT_ENABLED                    1
#define TEST_PANIC_ENABLED                        0
#define TEST_KICKSTART_ENABLED                    0

/*************************************************
 * TEST IDENTIFIERS
 ************************************************/

#define TEST_KICKSTART_END_ID                           0

#define TEST_PANIC_SUCCESS_ID                           1

#define TEST_INTERRUPT_SW_LOCK_REG_HANDLER0_ID          2
#define TEST_INTERRUPT_SW_LOCK_REG_HANDLER1_ID          3
#define TEST_INTERRUPT_SW_LOCK_CHECK0_ID                4
#define TEST_INTERRUPT_SW_LOCK_CHECK1_ID                5
#define TEST_INTERRUPT_SW_LOCK_CHECK2_ID                6
#define TEST_INTERRUPT_SW_LOCK_CHECK3_ID                7
#define TEST_INTERRUPT_SW_LOCK_CHECK4_ID                8
#define TEST_INTERRUPT_SW_LOCK_CHECK5_ID                9
#define TEST_INTERRUPT_SW_LOCK_CHECK6_ID                10
#define TEST_INTERRUPT_SW_LOCK_CHECK7_ID                11
#define TEST_INTERRUPT_SW_LOCK_REM_HANDLER0_ID          12
#define TEST_INTERRUPT_SW_LOCK_REM_HANDLER1_ID          13
#define TEST_INTERRUPT_SW_REG_BAD_HANDLER0_ID           14
#define TEST_INTERRUPT_SW_REG_BAD_HANDLER1_ID           15
#define TEST_INTERRUPT_SW_REM_BAD_HANDLER0_ID           16
#define TEST_INTERRUPT_SW_REM_BAD_HANDLER1_ID           17
#define TEST_INTERRUPT_SW_REG_BAD_HANDLER2_ID           18
#define TEST_INTERRUPT_SW_REM_BAD_HANDLER2_ID           19
#define TEST_INTERRUPT_SW_REG_HANDLER0_ID               20
#define TEST_INTERRUPT_SW_REG_ALREADY_REG_HANDLER0_ID   21
#define TEST_INTERRUPT_SW_REM_HANDLER0_ID               22
#define TEST_INTERRUPT_SW_COUNTER_CHECK0_ID             23
#define TEST_INTERRUPT_SW_COUNTER_CHECK1_ID             24
#define TEST_INTERRUPT_SW_REG0_SWINT_HANDLER(IDVAL)     (25 + IDVAL)
#define TEST_INTERRUPT_SW_REM0_SWINT_HANDLER(IDVAL)     \
    (TEST_INTERRUPT_SW_REG0_SWINT_HANDLER(MAX_INTERRUPT_LINE) + IDVAL)
#define TEST_INTERRUPT_SW_REG1_SWINT_HANDLER(IDVAL)     \
    (TEST_INTERRUPT_SW_REM0_SWINT_HANDLER(MAX_INTERRUPT_LINE) + IDVAL)
#define TEST_INTERRUPT_SW_REM1_SWINT_HANDLER(IDVAL)     \
    (TEST_INTERRUPT_SW_REG1_SWINT_HANDLER(MAX_INTERRUPT_LINE) + IDVAL)

/** @brief Current test name */
#define TEST_FRAMEWORK_TEST_NAME "Interrupt Suite"

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
 * FUNCTIONS
 ******************************************************************************/

void interrupt_test(void);

#endif /* #ifndef __TEST_FRAMEWORK_TEST_LIST_H_ */

/************************************ EOF *************************************/