/*******************************************************************************
 * @file uhashtable_test.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 14/05/2023
 *
 * @version 1.0
 *
 * @brief Testing framework unsigned hashtable testing.
 *
 * @details Testing framework unsigned hashtable testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <uhashtable.h>
#include <stdint.h>
#include <kheap.h>

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
static uint32_t g_seed = 0x21025;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

static uint32_t _random_get(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static uint32_t _random_get(void)
{
    g_seed = (214013*g_seed+2531011);
    return g_seed;
}

void uhashtable_test(void)
{
    size_t       i;
    uhashtable_t* table;
    uint32_t     data;
    OS_RETURN_E  err;

    table = uhashtableCreate(UHASHTABLE_ALLOCATOR(kmalloc, kfree), &err);
    TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_CREATE0_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_CREATE1_ID,
                             table != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)table,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_CREATE2_ID,
                             table->size == 0,
                             (uint64_t)0,
                             (uint64_t)table->size,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_CREATE3_ID,
                             table->capacity == 16,
                             (uint64_t)16,
                             (uint64_t)table->capacity,
                             TEST_OS_UHASHTABLE_ENABLED);

    for(i = 0; i < 26; ++i)
    {
        err = uhashtableSet(table, i, (void*)(i * 10));
        TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_SETBURST0_ID(i),
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_OS_UHASHTABLE_ENABLED);
    }

    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET0_ID,
                             table->size == 26,
                             (uint64_t)26,
                             (uint64_t)table->size,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET1_ID,
                             table->capacity == 64,
                             (uint64_t)64,
                             (uint64_t)table->capacity,
                             TEST_OS_UHASHTABLE_ENABLED);

    for(i = 0; i < 26; ++i)
    {
        err = uhashtableGet(table, i, (void**)&data);
        TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_GETBURST0_ID(i * 2),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_UHASHTABLE_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GETBURST0_ID(i * 2 + 1),
                                 (uint64_t)(uintptr_t)data == i * 10,
                                 (uint64_t)i * 10,
                                 (uint64_t)(uintptr_t)data,
                                 TEST_OS_UHASHTABLE_ENABLED)
    }

    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GET0_ID,
                             table->size == 26,
                             (uint64_t)26,
                             (uint64_t)table->size,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GET1_ID,
                             table->capacity == 64,
                             (uint64_t)64,
                             (uint64_t)table->capacity,
                             TEST_OS_UHASHTABLE_ENABLED);

    for(i = 0; i < 26; i += 2)
    {
        err = uhashtableSet(table, i, (void*)(i * 100));
        TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_SETBURST1_ID(i),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_UHASHTABLE_ENABLED);
    }

    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET2_ID,
                             table->size == 26,
                             (uint64_t)26,
                             (uint64_t)table->size,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET3_ID,
                             table->capacity == 64,
                             (uint64_t)64,
                             (uint64_t)table->capacity,
                             TEST_OS_UHASHTABLE_ENABLED);

    for(i = 0; i < 26; i += 2)
    {
        err = uhashtableSet(table, i, (void*)(i * 1000));
        TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_SETBURST2_ID(i),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_UHASHTABLE_ENABLED);
    }

    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET4_ID,
                             table->size == 26,
                             (uint64_t)26,
                             (uint64_t)table->size,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET5_ID,
                             table->capacity == 64,
                             (uint64_t)64,
                             (uint64_t)table->capacity,
                             TEST_OS_UHASHTABLE_ENABLED);

    for(i = 0; i < 26; ++i)
    {
        err = uhashtableGet(table, i, (void**)&data);
        TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_GETBURST1_ID(i * 2),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_UHASHTABLE_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GETBURST1_ID(i * 2 + 1),
                                 (uint64_t)(uintptr_t)data == i * (i % 2 == 0 ? 1000 : 10),
                                 (uint64_t)i * (i % 2 == 0 ? 1000 : 10),
                                 (uint64_t)(uintptr_t)data,
                                 TEST_OS_UHASHTABLE_ENABLED)
    }

    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GET2_ID,
                             table->size == 26,
                             (uint64_t)26,
                             (uint64_t)table->size,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GET3_ID,
                             table->capacity == 64,
                             (uint64_t)64,
                             (uint64_t)table->capacity,
                             TEST_OS_UHASHTABLE_ENABLED);

    for(i = 0; i < 26; ++i)
    {
        if(i % 2 == 0)
        {
            err = uhashtableRemove(table, i, NULL);
            TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_REMOVEBURST0_ID(i),
                                    err == OS_NO_ERR,
                                    OS_NO_ERR,
                                    err,
                                    TEST_OS_UHASHTABLE_ENABLED);
        }
    }
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_REMOVE0_ID,
                             table->size == 13,
                             (uint64_t)13,
                             (uint64_t)table->size,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_REMOVE1_ID,
                             table->capacity == 64,
                             (uint64_t)64,
                             (uint64_t)table->capacity,
                             TEST_OS_UHASHTABLE_ENABLED);

    for(i = 0; i < 30; ++i)
    {
        err = uhashtableGet(table, i, (void**)&data);

        if(err != OS_NO_ERR)
        {
            TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_GETBURST2_ID(i * 2),
                                    err == OS_ERR_NO_SUCH_ID,
                                    OS_ERR_NO_SUCH_ID,
                                    err,
                                    TEST_OS_UHASHTABLE_ENABLED);
            TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GETBURST2_ID(i * 2 + 1),
                                     i % 2 == 0 || i > 25,
                                     (uint64_t)i,
                                     (uint64_t)(uintptr_t)i,
                                     TEST_OS_UHASHTABLE_ENABLED);
        }
        else
        {
            TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GETBURST2_ID(i * 2 + 1),
                                     (uint64_t)(uintptr_t)data == i * (i % 2 == 0 ? 1000 : 10),
                                     (uint64_t)i * (i % 2 == 0 ? 1000 : 10),
                                     (uint64_t)(uintptr_t)data,
                                     TEST_OS_UHASHTABLE_ENABLED);
        }
    }
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_REMOVE2_ID,
                             table->size == 13,
                             (uint64_t)13,
                             (uint64_t)table->size,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_REMOVE3_ID,
                             table->capacity == 64,
                             (uint64_t)64,
                             (uint64_t)table->capacity,
                             TEST_OS_UHASHTABLE_ENABLED);

    err = uhashtableDestroy(table);
    TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_DESTROY0_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_DESTROY1_ID,
                             table->size == 0,
                             (uint64_t)0,
                             (uint64_t)table->size,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_DESTROY2_ID,
                             table->capacity == 0,
                             (uint64_t)0,
                             (uint64_t)table->capacity,
                             TEST_OS_UHASHTABLE_ENABLED);

    for(i = 0; i < 30; ++i)
    {
        err = uhashtableGet(table, i, (void**)&data);
        TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_GETBURST3_ID(i),
                                err == OS_ERR_NULL_POINTER,
                                OS_ERR_NULL_POINTER,
                                err,
                                TEST_OS_UHASHTABLE_ENABLED);
    }

    table = uhashtableCreate(UHASHTABLE_ALLOCATOR(kmalloc, kfree), &err);
    TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_CREATE4_ID,
                            err == OS_NO_ERR,
                            OS_NO_ERR,
                            err,
                            TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_CREATE5_ID,
                             table != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)table,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_CREATE6_ID,
                             table->size == 0,
                             (uint64_t)0,
                             (uint64_t)table->size,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_CREATE7_ID,
                             table->capacity == 16,
                             (uint64_t)16,
                             (uint64_t)table->capacity,
                             TEST_OS_UHASHTABLE_ENABLED);

    uint32_t* table_data = kmalloc(sizeof(uint32_t) * 200);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_ALLOC0_ID,
                             table_data != NULL,
                             (uint64_t)1,
                             (uint64_t)(uintptr_t)table_data,
                             TEST_OS_UHASHTABLE_ENABLED);

    for(i = 0; i < 200; ++i)
    {
        table_data[i] = _random_get();
        err = uhashtableSet(table, i, (void*)(uintptr_t)table_data[i]);
        TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_SETBURST3_ID(i),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_UHASHTABLE_ENABLED);
    }
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET6_ID,
                             table->size == 200,
                             (uint64_t)200,
                             (uint64_t)table->size,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_SET7_ID,
                             table->capacity == 512,
                             (uint64_t)512,
                             (uint64_t)table->capacity,
                             TEST_OS_UHASHTABLE_ENABLED);

    for(i = 0; i < 200; ++i)
    {
        err = uhashtableGet(table, i, (void**)&data);

        TEST_POINT_ASSERT_RCODE(TEST_UHASHTABLE_GETBURST4_ID(i * 2),
                                err == OS_NO_ERR,
                                OS_NO_ERR,
                                err,
                                TEST_OS_UHASHTABLE_ENABLED);
        TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_GETBURST4_ID(i * 2 + 1),
                                 (uint64_t)(uintptr_t)data == table_data[i],
                                 (uint64_t)table_data[i],
                                 (uint64_t)(uintptr_t)data,
                                 TEST_OS_UHASHTABLE_ENABLED)
    }

    err = uhashtableDestroy(table);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_DESTROY3_ID,
                             err == OS_NO_ERR,
                             (uint64_t)OS_NO_ERR,
                             (uint64_t)err,
                             TEST_OS_UHASHTABLE_ENABLED);

    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_DESTROY4_ID,
                             table->size == 0,
                             (uint64_t)0,
                             (uint64_t)table->size,
                             TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_ASSERT_UDWORD(TEST_UHASHTABLE_DESTROY5_ID,
                             table->capacity == 0,
                             (uint64_t)0,
                             (uint64_t)table->capacity,
                             TEST_OS_UHASHTABLE_ENABLED);

    kfree(table_data);

    TEST_FRAMEWORK_END();
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/