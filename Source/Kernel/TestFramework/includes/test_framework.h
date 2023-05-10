/*******************************************************************************
 * @file test_framework.h
 *
 * @see test_framework.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 25/04/2023
 *
 * @version 1.0
 *
 * @brief Testing framework.
 *
 * @details Testing framework. This modules allows to add dynamic test points
 * to the kernel an run a test suite.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __TEST_FRAMEWORK_TEST_FRAMEWORK_H_
#define __TEST_FRAMEWORK_TEST_FRAMEWORK_H_

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <test_list.h> /* Test list */

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

#define TEST_POINT_ASSERT_UINT(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {   \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        test_framework_assert_uint(ID, COND, EXPECTED, VALUE);              \
    }                                                                       \
}

#define TEST_POINT_ASSERT_INT(ID, COND, EXPECTED, VALUE, TEST_ENABLED)  {   \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        test_framework_assert_int(ID, COND, EXPECTED, VALUE);               \
    }                                                                       \
}

#define TEST_POINT_ASSERT_HUINT(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        test_framework_assert_uhint(ID, COND, EXPECTED, VALUE);             \
    }                                                                       \
}

#define TEST_POINT_ASSERT_HINT(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {   \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        test_framework_assert_hint(ID, COND, EXPECTED, VALUE);              \
    }                                                                       \
}

#define TEST_POINT_ASSERT_UBYTE(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        test_framework_assert_ubyte(ID, COND, EXPECTED, VALUE);             \
    }                                                                       \
}

#define TEST_POINT_ASSERT_BYTE(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {   \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        test_framework_assert_byte(ID, COND, EXPECTED, VALUE);              \
    }                                                                       \
}

#define TEST_POINT_ASSERT_UDWORD(ID, COND, EXPECTED, VALUE, TEST_ENABLED) { \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        test_framework_assert_udword(ID, COND, EXPECTED, VALUE);            \
    }                                                                       \
}

#define TEST_POINT_ASSERT_DWORD(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        test_framework_assert_dword(ID, COND, EXPECTED, VALUE);             \
    }                                                                       \
}

#define TEST_POINT_ASSERT_FLOAT(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        test_framework_assert_float(ID, COND, EXPECTED, VALUE);             \
    }                                                                       \
}

#define TEST_POINT_ASSERT_DOUBLE(ID, COND, EXPECTED, VALUE, TEST_ENABLED) { \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        test_framework_assert_double(ID, COND, EXPECTED, VALUE);            \
    }                                                                       \
}

#define TEST_POINT_ASSERT_RCODE(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        test_framework_assert_errcode(ID, COND, EXPECTED, VALUE);           \
    }                                                                       \
}

#define TEST_POINT_FUNCTION_CALL(FUNCTION_NAME, TEST_ENABLED){              \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        FUNCTION_NAME();                                                    \
    }                                                                       \
}

#define TEST_FRAMEWORK_START() test_framework_init();

#define TEST_FRAMEWORK_END() test_framework_end();

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

void test_framework_init(void);
void test_framework_end(void);

void test_framework_assert_uint(const uint32_t test_id,
                                const bool_t condition,
                                const uint32_t expected,
                                const uint32_t value);

void test_framework_assert_int(const uint32_t test_id,
                               const bool_t condition,
                               const int32_t expected,
                               const int32_t value);

void test_framework_assert_huint(const uint32_t test_id,
                                 const bool_t condition,
                                 const uint16_t expected,
                                 const uint16_t value);

void test_framework_assert_hint(const uint32_t test_id,
                                const bool_t condition,
                                const int16_t expected,
                                const int16_t value);

void test_framework_assert_ubyte(const uint32_t test_id,
                                 const bool_t condition,
                                 const uint8_t expected,
                                 const uint8_t value);

void test_framework_assert_byte(const uint32_t test_id,
                                const bool_t condition,
                                const uint8_t expected,
                                const uint8_t value);

void test_framework_assert_udword(const uint32_t test_id,
                                  const bool_t condition,
                                  const uint64_t expected,
                                  const uint64_t value);

void test_framework_assert_dword(const uint32_t test_id,
                                 const bool_t condition,
                                 const int64_t expected,
                                 const int64_t value);

void test_framework_assert_float(const uint32_t test_id,
                                 const bool_t condition,
                                 const float expected,
                                 const float value);

void test_framework_assert_double(const uint32_t test_id,
                                  const bool_t condition,
                                  const double expected,
                                  const double value);

void test_framework_assert_errcode(const uint32_t test_id,
                                   const bool_t condition,
                                   const OS_RETURN_E expected,
                                   const OS_RETURN_E value);

#else /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* None */

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

#define TEST_POINT_ASSERT_UINT(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_INT(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_HUINT(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_HINT(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_UBYTE(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_BYTE(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_UDWORD(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_DWORD(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_FLOAT(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_DOUBLE(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_ASSERT_ERRCODE(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_FUNCTION_CALL(FUNCTION_NAME, TEST_ENABLED)

#define TEST_FRAMEWORK_START()

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


#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

#endif /* #ifndef __TEST_FRAMEWORK_TEST_FRAMEWORK_H_ */

/************************************ EOF *************************************/