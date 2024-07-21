/*******************************************************************************
 * @file test_framework.c
 *
 * @see test_framework.h
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

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <kerneloutput.h> /* Kernel output */
#include <stdint.h>        /* Standard C ints */
#include <stddef.h>        /* Standard definitions */
#include <panic.h>         /* Kernel panic */
#include <kerror.h>        /* Kernel errors */
#include <cpu.h>          /*  CPU API */
#include <critical.h>     /* Kernel critical */

/* Configuration files */
#include <config.h>

/* Header file */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Testing framework version. */
#define TEST_FRAMEWORK_VERSION "0.5"

/** @brief Defines the current module's name. */
#define MODULE_NAME "TEST FRAMEWORK"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

typedef enum
{
    TEST_TYPE_BYTE    = 0,
    TEST_TYPE_UBYTE   = 1,
    TEST_TYPE_HALF    = 2,
    TEST_TYPE_UHALF   = 3,
    TEST_TYPE_WORD    = 4,
    TEST_TYPE_UWORD   = 5,
    TEST_TYPE_DWORD   = 6,
    TEST_TYPE_UDWORD  = 7,
    TEST_TYPE_FLOAT   = 8,
    TEST_TYPE_DOUBLE  = 9,
    TEST_TYPE_RCODE   = 10,
    TEST_TYPE_POINTER = 11
} TEST_ITEM_TYPE_E;

typedef struct test_item
{
    bool_t   status;
    uint64_t value;
    uint64_t expected;
    uint32_t id;

    TEST_ITEM_TYPE_E type;

    struct test_item* next;
} test_item_t;


typedef union
{
    float    float_value;
    uint32_t raw_value;
} float_raw_t;

typedef union
{
    double   double_value;
    uint64_t raw_value;
} double_raw_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/


/**
 * @brief Asserts a condition and generates a kernel panic in case of failure.
 *
 * @details Asserts a condition and generates a kernel panic in case of failure.
 *
 * @param[in] COND The condition to verify.
 * @param[in] MSG The message to print in case of error.
 * @param[in] ERROR The error code.
 *
*/
#define TEST_ASSERT(COND, MSG, ERROR) {                     \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}


/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Test buffer pool base address defined by linker. */
extern uint8_t _KERNEL_TEST_BUFFER_BASE;
/** @brief Test buffer pool size defined by linker. */
extern uint8_t _KERNEL_TEST_BUFFER_SIZE;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Number of tests executed during the suite. */
static uint32_t test_count = 0;

/** @brief Number of tests executed during the suite that failed. */
static uint32_t failures = 0;

/** @brief Number of tests executed during the suite that succeeded. */
static uint32_t success = 0;

/** @brief Memory pool head pointer. */
static uint8_t* memoryPoolHead = NULL;

/** @brief Test items list head */
test_item_t* test_list = NULL;

/** @brief Final test when allocation failed */
test_item_t null_test_item = {
    .expected = -1,
    .id       = -1,
    .type     = TEST_TYPE_BYTE,
    .value    = -1,
    .status   = FALSE
};

/** @brief Test spinlock */
static kernel_spinlock_t sLock;
/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

static void * _getTestMemory(const size_t size);

static void _initTestItem(const uint32_t test_id,
                            const bool_t condition,
                            const uint64_t expected,
                            const uint64_t value,
                            test_item_t** item);

void _killQemu(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void * _getTestMemory(const size_t size)
{
    uint8_t * address;

    if((uint32_t)(uintptr_t)&_KERNEL_TEST_BUFFER_SIZE +
       &_KERNEL_TEST_BUFFER_BASE >
       memoryPoolHead + size)
    {
        address = memoryPoolHead;
        memoryPoolHead += size;
    }
    else
    {
        address = NULL;
    }

    return address;
}

static void _initTestItem(const uint32_t test_id,
                            const bool_t condition,
                            const uint64_t expected,
                            const uint64_t value,
                            test_item_t** item)
{
    KERNEL_LOCK(sLock);

    /* Create test item and allocate memory */
    *item = _getTestMemory(sizeof(test_item_t));
    if(*item != NULL)
    {
        /* Setup information */
        (*item)->id       = test_id;
        (*item)->status   = condition;
        (*item)->expected = expected;
        (*item)->value    = value;

        /* If the condition is expected then increment */
        if(condition == TRUE)
        {
            ++success;
        }
        else
        {
            ++failures;
        }
        ++test_count;

        /* Link test */
        (*item)->next = test_list;
        test_list = *item;
    }
    else
    {
        /* Link NULL test item */
        ++failures;
        ++test_count;

        /* Link test */
        if(test_list != &null_test_item)
        {

            null_test_item.next = test_list;
            test_list = &null_test_item;
        }

        *item = &null_test_item;
    }

    KERNEL_UNLOCK(sLock);
}

void _killQemu(void)
{
    while(1)
    {
        __asm__ __volatile__("outw %w0, %w1" : : "ax" (0x2000), "Nd" (0x604));
        __asm__ ("hlt");
    }
}

void testFrameworkInit(void)
{
    KERNEL_SPINLOCK_INIT(sLock);
    memoryPoolHead = &_KERNEL_TEST_BUFFER_BASE;
}

void testFrameworkEnd(void)
{
    uint32_t i;

    test_item_t* test_cursor;

    /* Print output header */
    kprintf("\n#-------- TESTING SECTION START --------#\n");
    kprintf("{\n");
    kprintf("\t\"version\": \"" TEST_FRAMEWORK_VERSION "\",\n");
    kprintf("\t\"name\": \"" TEST_FRAMEWORK_TEST_NAME "\",\n");
    kprintf("\t\"number_of_tests\": %d,\n", test_count);
    kprintf("\t\"failures\": %d,\n", failures);
    kprintf("\t\"success\": %d,\n", success);
    kprintf("\t\"test_suite\": {\n");

    test_cursor = test_list;

    for(i = 0; i < test_count && test_cursor != NULL; ++i)
    {
        kprintf("\t\t\"%d\": {\n", test_cursor->id);

        kprintf("\t\t\t\"result\": %lu,\n", test_cursor->value);
        kprintf("\t\t\t\"expected\": %lu,\n", test_cursor->expected);
        kprintf("\t\t\t\"status\": %d,\n", test_cursor->status);
        kprintf("\t\t\t\"type\": %d\n", test_cursor->type);

        kprintf("\t\t}");
        if(i < test_count - 1)
        {
            kprintf(",\n");
        }
        else
        {
            kprintf("\n");
        }

        test_cursor = test_cursor->next;
    }

    kprintf("\t}\n");
    kprintf("}\n");
    kprintf("#-------- TESTING SECTION END --------#\n");

    _killQemu();
    while(1)
    {
        cpuClearInterrupt();
        cpuHalt();
    }
}

void testFrameworkAssertUint(const uint32_t test_id,
                                const bool_t condition,
                                const uint32_t expected,
                                const uint32_t value)
{
    test_item_t* item;

    /* Init */
    _initTestItem(test_id,
                    condition,
                    (uint64_t)expected,
                    (uint64_t)value,
                    &item);

    /* Set additional data */
    item->type = TEST_TYPE_UDWORD;
}

void testFrameworkAssertInt(const uint32_t test_id,
                               const bool_t condition,
                               const int32_t expected,
                               const int32_t value)
{
    test_item_t* item;

    /* Init */
    _initTestItem(test_id,
                    condition,
                    (uint64_t)expected,
                    (uint64_t)value,
                    &item);

    /* Set additional data */
    item->type = TEST_TYPE_DWORD;
}

void testFrameworkAssertHuint(const uint32_t test_id,
                                 const bool_t condition,
                                 const uint16_t expected,
                                 const uint16_t value)
{
    test_item_t* item;

    /* Init */
    _initTestItem(test_id,
                    condition,
                    (uint64_t)expected,
                    (uint64_t)value,
                    &item);

    /* Set additional data */
    item->type = TEST_TYPE_UHALF;
}

void testFrameworkAssertHint(const uint32_t test_id,
                                const bool_t condition,
                                const int16_t expected,
                                const int16_t value)
{
    test_item_t* item;

    /* Init */
    _initTestItem(test_id,
                    condition,
                    (uint64_t)expected,
                    (uint64_t)value,
                    &item);

    /* Set additional data */
    item->type = TEST_TYPE_HALF;
}

void testFrameworkAssertUbyte(const uint32_t test_id,
                                 const bool_t condition,
                                 const uint8_t expected,
                                 const uint8_t value)
{
    test_item_t* item;

    /* Init */
    _initTestItem(test_id,
                    condition,
                    (uint64_t)expected,
                    (uint64_t)value,
                    &item);

    /* Set additional data */
    item->type = TEST_TYPE_UBYTE;
}

void testFrameworkAssertByte(const uint32_t test_id,
                                const bool_t condition,
                                const uint8_t expected,
                                const uint8_t value)
{
    test_item_t* item;

    /* Init */
    _initTestItem(test_id,
                    condition,
                    (uint64_t)expected,
                    (uint64_t)value,
                    &item);

    /* Set additional data */
    item->type = TEST_TYPE_BYTE;
}

void testFrameworkAssertUdword(const uint32_t test_id,
                                  const bool_t condition,
                                  const uint64_t expected,
                                  const uint64_t value)
{
    test_item_t* item;

    /* Init */
    _initTestItem(test_id,
                    condition,
                    expected,
                    value,
                    &item);

    /* Set additional data */
    item->type = TEST_TYPE_UDWORD;
}

void testFrameworkAssertDword(const uint32_t test_id,
                                 const bool_t condition,
                                 const int64_t expected,
                                 const int64_t value)
{
    test_item_t* item;

    /* Init */
    _initTestItem(test_id,
                    condition,
                    (uint64_t)expected,
                    (uint64_t)value,
                    &item);

    /* Set additional data */
    item->type = TEST_TYPE_DWORD;
}

void testFrameworkAssertFloat(const uint32_t test_id,
                                 const bool_t condition,
                                 const float expected,
                                 const float value)
{
    test_item_t* item;
    float_raw_t  float_conv_expected;
    float_raw_t  float_conv_value;

    /* Init */
    float_conv_expected.float_value = expected;
    float_conv_value.float_value = value;
    _initTestItem(test_id,
                    condition,
                    (uint64_t)float_conv_expected.raw_value,
                    (uint64_t)float_conv_value.raw_value,
                    &item);

    /* Set additional data */
    item->type = TEST_TYPE_FLOAT;
}

void testFrameworkAssertDouble(const uint32_t test_id,
                                  const bool_t condition,
                                  const double expected,
                                  const double value)
{
    test_item_t* item;
    double_raw_t double_conv_expected;
    double_raw_t double_conv_value;

    /* Init */
    double_conv_expected.double_value = expected;
    double_conv_value.double_value = value;
    _initTestItem(test_id,
                    condition,
                    double_conv_expected.raw_value,
                    double_conv_value.raw_value,
                    &item);

    /* Set additional data */
    item->type = TEST_TYPE_DOUBLE;
}

void testFrameworkAssertErrCode(const uint32_t test_id,
                                   const bool_t condition,
                                   const OS_RETURN_E expected,
                                   const OS_RETURN_E value)
{
    test_item_t* item;

    /* Init */
    _initTestItem(test_id,
                    condition,
                    (uint64_t)expected,
                    (uint64_t)value,
                    &item);

    /* Set additional data */
    item->type = TEST_TYPE_RCODE;
}

void testFrameworkAssertPointer(const uint32_t test_id,
                                   const bool_t condition,
                                   const uintptr_t expected,
                                   const uintptr_t value)
{
    test_item_t* item;

    /* Init */
    _initTestItem(test_id,
                    condition,
                    (uint64_t)expected,
                    (uint64_t)value,
                    &item);

    /* Set additional data */
    item->type = TEST_TYPE_POINTER;
}



#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/