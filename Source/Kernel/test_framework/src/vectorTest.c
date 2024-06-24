/*******************************************************************************
 * @file vectorTest.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 14/05/2023
 *
 * @version 1.0
 *
 * @brief Testing framework vector testing.
 *
 * @details Testing framework vector testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <vector.h>
#include <stdint.h>
#include <kheap.h>
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
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void vectorTest(void)
{
    size_t i;
    uint64_t data;
    uint64_t data2;
    vector_t* vector;
    vector_t* vector_cpy;
    OS_RETURN_E err;

    vector = vectorCreate(VECTOR_ALLOCATOR(kmalloc, kfree), (void*)0, 0, &err);
    TEST_POINT_ASSERT_RCODE(TEST_VECTOR_CREATE0_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_CREATE1_ID,
                             vector != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)vector,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_CREATE2_ID,
                             vector->size == 0,
                             (uint64_t)0,
                             (uint64_t)vector->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_CREATE3_ID,
                             vector->capacity == 0,
                             (uint64_t)0,
                             (uint64_t)vector->capacity,
                             TEST_OS_VECTOR_ENABLED);

    for(i = 0; i < 20; ++i)
    {
        err = vectorPush(vector, (void*)i);
        TEST_POINT_ASSERT_RCODE(TEST_VECTOR_PUSHBURST0_ID(i * 2),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_PUSHBURST0_ID(i * 2 + 1),
                                 vector->size == i + 1,
                                 (uint64_t)i + 1,
                                 (uint64_t)(uintptr_t)vector->size,
                                 TEST_OS_VECTOR_ENABLED);
    }

    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET0_ID,
                             vector->size == 20,
                             (uint64_t)20,
                             (uint64_t)vector->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET1_ID,
                             vector->capacity == 32,
                             (uint64_t)32,
                             (uint64_t)vector->capacity,
                             TEST_OS_VECTOR_ENABLED);

    for(i = 0; i < vector->size; ++i)
    {
        vectorGet(vector, i, (void**)&data);
        TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST0_ID(i * 2),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST0_ID(i * 2 + 1),
                                 (uint64_t)(uintptr_t)data == i,
                                 (uint64_t)i,
                                 (uint64_t)(uintptr_t)data,
                                 TEST_OS_VECTOR_ENABLED);
    }

    for(i = 0; i < 30; i += 2)
    {
        err = vectorInsert(vector, (void*)(i + 100), i);
        TEST_POINT_ASSERT_RCODE(TEST_VECTOR_INSERTBURST0_ID(i * 2),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_INSERTBURST0_ID(i * 2 + 1),
                                 (uint64_t)(uintptr_t)vector->size == i / 2 + 20 + 1,
                                 (uint64_t)i / 2 + 20 + 1,
                                 (uint64_t)(uintptr_t)vector->size,
                                 TEST_OS_VECTOR_ENABLED);
    }

    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_INSERT0_ID,
                             vector->size == 35,
                             (uint64_t)35,
                             (uint64_t)vector->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_INSERT1_ID,
                             vector->capacity == 64,
                             (uint64_t)64,
                             (uint64_t)vector->capacity,
                             TEST_OS_VECTOR_ENABLED);

    for(i = 0; i < vector->size; ++i)
    {
        vectorGet(vector, i, (void**)&data);

        if(i < 30)
        {
            data2 = (i % 2 ? i / 2 : i + 100);
        }
        else
        {
            data2 = 14 + i - 29;
        }
        TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST1_ID(i * 3),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST1_ID(i * 3 + 1),
                                 (uint64_t)(uintptr_t)data == (uint64_t)(uintptr_t)vector->ppArray[i],
                                 (uint64_t)(uintptr_t)vector->ppArray[i],
                                 (uint64_t)(uintptr_t)data,
                                 TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST1_ID(i * 3 + 2),
                                 (uint64_t)(uintptr_t)data == data2,
                                 (uint64_t)data2,
                                 (uint64_t)(uintptr_t)data,
                                 TEST_OS_VECTOR_ENABLED);
    }
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET2_ID,
                             vector->size == 35,
                             (uint64_t)35,
                             (uint64_t)vector->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET3_ID,
                             vector->capacity == 64,
                             (uint64_t)64,
                             (uint64_t)vector->capacity,
                             TEST_OS_VECTOR_ENABLED);

    for(i = 0; i < 6; i++)
    {
        data2 = 19 - i;

        err = vectorPop(vector, (void**)&data);
        TEST_POINT_ASSERT_RCODE(TEST_VECTOR_POPBURST0_ID(i * 2),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_POPBURST0_ID(i * 2 + 1),
                                 (uint64_t)(uintptr_t)data == data2,
                                 (uint64_t)data2,
                                 (uint64_t)(uintptr_t)data,
                                 TEST_OS_VECTOR_ENABLED);
    }
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_POP0_ID,
                             vector->size == 29,
                             (uint64_t)29,
                             (uint64_t)vector->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_POP1_ID,
                             vector->capacity == 64,
                             (uint64_t)64,
                             (uint64_t)vector->capacity,
                             TEST_OS_VECTOR_ENABLED);

    for(i = 0; i < vector->size; ++i)
    {
        if(i < 35)
        {
            data2 = (i % 2 ? (i / 2) : i + 100);
        }
        else
        {
            data2 = 14 + i - 29;
        }

        vectorGet(vector, i, (void**)&data);
        TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST2_ID(i * 2),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST2_ID(i * 2 + 1),
                                 (uint64_t)(uintptr_t)data == data2,
                                 (uint64_t)data2,
                                 (uint64_t)(uintptr_t)data,
                                 TEST_OS_VECTOR_ENABLED);

    }

    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET4_ID,
                             vector->size == 29,
                             (uint64_t)29,
                             (uint64_t)vector->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET5_ID,
                             vector->capacity == 64,
                             (uint64_t)64,
                             (uint64_t)vector->capacity,
                             TEST_OS_VECTOR_ENABLED);

    for(i = 0; i < vector->size; i++)
    {
        err = vectorSet(vector, i, (void*)i);
        TEST_POINT_ASSERT_RCODE(TEST_VECTOR_SETBURST0_ID(i * 2),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_SETBURST0_ID(i * 2 + 1),
                                 (uint64_t)(uintptr_t)i == (uint64_t)(uintptr_t)vector->ppArray[i],
                                 (uint64_t)(uintptr_t)vector->ppArray[i],
                                 (uint64_t)(uintptr_t)i,
                                 TEST_OS_VECTOR_ENABLED);
    }

    for(i = 0; i < vector->size; ++i)
    {
        vectorGet(vector, i, (void**)&data);
        TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST3_ID(i * 2),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST3_ID(i * 2 + 1),
                                 (uint64_t)(uintptr_t)data == i,
                                 (uint64_t)i,
                                 (uint64_t)(uintptr_t)data,
                                 TEST_OS_VECTOR_ENABLED);
    }

    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET6_ID,
                             vector->size == 29,
                             (uint64_t)29,
                             (uint64_t)vector->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GET7_ID,
                             vector->capacity == 64,
                             (uint64_t)64,
                             (uint64_t)vector->capacity,
                             TEST_OS_VECTOR_ENABLED);

    err = vectorResize(vector, 20);
    TEST_POINT_ASSERT_RCODE(TEST_VECTOR_RESIZE0_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_RESIZE1_ID,
                             vector->size == 20,
                             (uint64_t)20,
                             (uint64_t)vector->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_RESIZE2_ID,
                             vector->capacity == 64,
                             (uint64_t)64,
                             (uint64_t)vector->capacity,
                             TEST_OS_VECTOR_ENABLED);

    for(i = 0; i < vector->size; ++i)
    {
        vectorGet(vector, i, (void**)&data);
        TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST4_ID(i * 2),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST4_ID(i * 2 + 1),
                                 (uint64_t)(uintptr_t)data == i,
                                 (uint64_t)i,
                                 (uint64_t)(uintptr_t)data,
                                 TEST_OS_VECTOR_ENABLED);
    }

    err = vectorResize(vector, 80);
    TEST_POINT_ASSERT_RCODE(TEST_VECTOR_RESIZE3_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_RESIZE4_ID,
                             vector->size == 80,
                             (uint64_t)80,
                             (uint64_t)vector->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_RESIZE5_ID,
                             vector->capacity == 80,
                             (uint64_t)80,
                             (uint64_t)vector->capacity,
                             TEST_OS_VECTOR_ENABLED);

    for(i = 0; i < 20; ++i)
    {
        vectorGet(vector, i, (void**)&data);
        TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST5_ID(i * 2),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST5_ID(i * 2 + 1),
                                 (uint64_t)(uintptr_t)data == i,
                                 (uint64_t)i,
                                 (uint64_t)(uintptr_t)data,
                                 TEST_OS_VECTOR_ENABLED);
    }

    err = vectorResize(vector, 20);
    TEST_POINT_ASSERT_RCODE(TEST_VECTOR_RESIZE6_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_RESIZE7_ID,
                             vector->size == 20,
                             (uint64_t)20,
                             (uint64_t)vector->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_RESIZE8_ID,
                             vector->capacity == 80,
                             (uint64_t)80,
                             (uint64_t)vector->capacity,
                             TEST_OS_VECTOR_ENABLED);

    err = vectorSrink(vector);
    TEST_POINT_ASSERT_RCODE(TEST_VECTOR_SHRINK0_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_SHRINK1_ID,
                             vector->size == 20,
                             (uint64_t)20,
                             (uint64_t)vector->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_SHRINK2_ID,
                             vector->capacity == 20,
                             (uint64_t)20,
                             (uint64_t)vector->capacity,
                             TEST_OS_VECTOR_ENABLED);

    for(i = 0; i < vector->size; ++i)
    {
        vectorGet(vector, i, (void**)&data);
        TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST6_ID(i * 2),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST6_ID(i * 2 + 1),
                                 (uint64_t)(uintptr_t)data == i,
                                 (uint64_t)i,
                                 (uint64_t)(uintptr_t)data,
                                 TEST_OS_VECTOR_ENABLED);
    }

    vector_cpy = vectorCopy(vector, &err);
    TEST_POINT_ASSERT_RCODE(TEST_VECTOR_COPY0_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_COPY1_ID,
                             vector_cpy != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)vector_cpy,
                             TEST_OS_VECTOR_ENABLED);

    for(i = 0; i < vector->size; ++i)
    {
        err = vectorGet(vector, i, (void**)&data);
        TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST7_ID(i * 4),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_VECTOR_ENABLED);
        err = vectorGet(vector_cpy, i, (void**)&data2);
        TEST_POINT_ASSERT_RCODE(TEST_VECTOR_GETBURST7_ID(i * 4 + 1),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST7_ID(i * 4 + 2),
                                 (uint64_t)(uintptr_t)data == (uint64_t)(uintptr_t)data2,
                                 (uint64_t)(uintptr_t)data,
                                 (uint64_t)(uintptr_t)data2,
                                 TEST_OS_VECTOR_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_GETBURST7_ID(i * 4 + 3),
                                 (uint64_t)(uintptr_t)data == i,
                                 (uint64_t)i,
                                 (uint64_t)(uintptr_t)data,
                                 TEST_OS_VECTOR_ENABLED);
    }
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_COPY2_ID,
                             vector->size == vector_cpy->size,
                             (uint64_t)vector->size,
                             (uint64_t)(uintptr_t)vector_cpy->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_COPY3_ID,
                             vector->capacity == vector_cpy->capacity,
                             (uint64_t)vector->capacity,
                             (uint64_t)(uintptr_t)vector_cpy->capacity,
                             TEST_OS_VECTOR_ENABLED);

    err = vectorClear(vector);
    TEST_POINT_ASSERT_RCODE(TEST_VECTOR_CLEAR0_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_CLEAR1_ID,
                             vector->size == 0,
                             (uint64_t)0,
                             (uint64_t)(uintptr_t)vector->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_CLEAR2_ID,
                             vector->capacity == 20,
                             (uint64_t)20,
                             (uint64_t)(uintptr_t)vector->capacity,
                             TEST_OS_VECTOR_ENABLED);

    err = vectorDestroy(vector_cpy);
    TEST_POINT_ASSERT_RCODE(TEST_VECTOR_DESTROY0_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_DESTROY1_ID,
                             vector_cpy->size == 0,
                             (uint64_t)0,
                             (uint64_t)(uintptr_t)vector_cpy->size,
                             TEST_OS_VECTOR_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_VECTOR_DESTROY2_ID,
                             vector_cpy->capacity == 0,
                             (uint64_t)0,
                             (uint64_t)(uintptr_t)vector_cpy->capacity,
                             TEST_OS_VECTOR_ENABLED);

    TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/