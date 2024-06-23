
/*******************************************************************************
 * @file schedulerTests.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/06/2024
 *
 * @version 1.0
 *
 * @brief Testing framework scheduler testing.
 *
 * @details Testing framework scheduler testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
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
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void schedulerTest(void)
{
    /* TODO: Tests do as for futex in a separate thread and mode the TEST_FRAMEWORK_END */
    TEST_POINT_ASSERT_POINTER(TEST_SCHEDULER_TODO,
                              NULL == NULL,
                              (uintptr_t)NULL,
                              (uintptr_t)NULL,
                              TEST_SCHEDULER_ENABLED);

    TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/