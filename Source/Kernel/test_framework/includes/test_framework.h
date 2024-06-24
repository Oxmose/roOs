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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <test_list.h> /* Test list */
#include <kerror.h> /* Kernel error */

#ifdef _TESTING_FRAMEWORK_ENABLED

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
        testFrameworkAssertUint(ID, COND, EXPECTED, VALUE);              \
    }                                                                       \
}

#define TEST_POINT_ASSERT_INT(ID, COND, EXPECTED, VALUE, TEST_ENABLED)  {   \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        testFrameworkAssertInt(ID, COND, EXPECTED, VALUE);               \
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
        testFrameworkAssertHint(ID, COND, EXPECTED, VALUE);              \
    }                                                                       \
}

#define TEST_POINT_ASSERT_UBYTE(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        testFrameworkAssertUbyte(ID, COND, EXPECTED, VALUE);             \
    }                                                                       \
}

#define TEST_POINT_ASSERT_BYTE(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {   \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        testFrameworkAssertByte(ID, COND, EXPECTED, VALUE);              \
    }                                                                       \
}

#define TEST_POINT_ASSERT_UDWORD(ID, COND, EXPECTED, VALUE, TEST_ENABLED) { \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        testFrameworkAssertUdword(ID, COND, EXPECTED, VALUE);            \
    }                                                                       \
}

#define TEST_POINT_ASSERT_DWORD(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        testFrameworkAssertDword(ID, COND, EXPECTED, VALUE);             \
    }                                                                       \
}

#define TEST_POINT_ASSERT_FLOAT(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        testFrameworkAssertFloat(ID, COND, EXPECTED, VALUE);             \
    }                                                                       \
}

#define TEST_POINT_ASSERT_DOUBLE(ID, COND, EXPECTED, VALUE, TEST_ENABLED) { \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        testFrameworkAssertDouble(ID, COND, EXPECTED, VALUE);            \
    }                                                                       \
}

#define TEST_POINT_ASSERT_RCODE(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        testFrameworkAssertErrCode(ID, COND, EXPECTED, VALUE);           \
    }                                                                       \
}

#define TEST_POINT_ASSERT_POINTER(ID, COND, EXPECTED, VALUE, TEST_ENABLED) {  \
    if(TEST_ENABLED)                                                          \
    {                                                                         \
        testFrameworkAssertPointer(ID, COND, EXPECTED, VALUE);             \
    }                                                                         \
}

#define TEST_POINT_FUNCTION_CALL(FUNCTION_NAME, TEST_ENABLED){              \
    if(TEST_ENABLED)                                                        \
    {                                                                       \
        FUNCTION_NAME();                                                    \
    }                                                                       \
}

#define TEST_FRAMEWORK_START() testFrameworkInit();

#define TEST_FRAMEWORK_END() testFrameworkEnd();

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

void testFrameworkInit(void);
void testFrameworkEnd(void);

void testFrameworkAssertUint(const uint32_t test_id,
                                const bool_t condition,
                                const uint32_t expected,
                                const uint32_t value);

void testFrameworkAssertInt(const uint32_t test_id,
                               const bool_t condition,
                               const int32_t expected,
                               const int32_t value);

void testFrameworkAssertHuint(const uint32_t test_id,
                                 const bool_t condition,
                                 const uint16_t expected,
                                 const uint16_t value);

void testFrameworkAssertHint(const uint32_t test_id,
                                const bool_t condition,
                                const int16_t expected,
                                const int16_t value);

void testFrameworkAssertUbyte(const uint32_t test_id,
                                 const bool_t condition,
                                 const uint8_t expected,
                                 const uint8_t value);

void testFrameworkAssertByte(const uint32_t test_id,
                                const bool_t condition,
                                const uint8_t expected,
                                const uint8_t value);

void testFrameworkAssertUdword(const uint32_t test_id,
                                  const bool_t condition,
                                  const uint64_t expected,
                                  const uint64_t value);

void testFrameworkAssertDword(const uint32_t test_id,
                                 const bool_t condition,
                                 const int64_t expected,
                                 const int64_t value);

void testFrameworkAssertFloat(const uint32_t test_id,
                                 const bool_t condition,
                                 const float expected,
                                 const float value);

void testFrameworkAssertDouble(const uint32_t test_id,
                                  const bool_t condition,
                                  const double expected,
                                  const double value);

void testFrameworkAssertErrCode(const uint32_t test_id,
                                   const bool_t condition,
                                   const OS_RETURN_E expected,
                                   const OS_RETURN_E value);

void testFrameworkAssertPointer(const uint32_t test_id,
                                   const bool_t condition,
                                   const uintptr_t expected,
                                   const uintptr_t value);

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

#define TEST_POINT_ASSERT_RCODE(ID, COND, EXPECTED, VALUE, TEST_ENABLED)

#define TEST_POINT_FUNCTION_CALL(FUNCTION_NAME, TEST_ENABLED)

#define TEST_FRAMEWORK_START()

#define TEST_FRAMEWORK_END()

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