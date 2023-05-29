
/*******************************************************************************
 * @file kheap_test.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 14/05/2023
 *
 * @version 1.0
 *
 * @brief Testing framework kernel heap testing.
 *
 * @details Testing framework kernel heap testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <kheap.h>
#include <panic.h>
#include <kernel_output.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <test_framework.h>


/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

#define KHEAP_BLOCK_HEADER_SIZE (sizeof(uintptr_t) * 4)

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
extern uint8_t _KERNEL_HEAP_BASE;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
static void* address[200]   = {NULL};
static uint32_t sizes[200];
static uint64_t addr_expected[200];
static uint64_t allocated_expected[200];

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void kheap_test(void)
{
    uint32_t i;
    uint64_t next_addr;
    uint64_t mem_free;
    uint64_t new_mem_free;

    void* start_addr = kmalloc(sizeof(uintptr_t) * 2);
    uint64_t heap_base = (uint64_t)(uintptr_t)&_KERNEL_HEAP_BASE;

    addr_expected[0] = heap_base + KHEAP_BLOCK_HEADER_SIZE * 2;
    TEST_POINT_ASSERT_UDWORD(TEST_KHEAP_START_ID,
                             (uint64_t)(uintptr_t)start_addr == addr_expected[0],
                             addr_expected[0],
                             (uint64_t)(uintptr_t)start_addr,
                             TEST_KHEAP_ENABLED);

    next_addr = addr_expected[0] + sizeof(uintptr_t) * 2;
    mem_free  = kheap_get_free();
    for(i = 0; i < 200; ++i)
    {
        sizes[i] = sizeof(uintptr_t) * 2 * (i + 1);
        address[i] = kmalloc(sizes[i]);

        addr_expected[i] = next_addr + KHEAP_BLOCK_HEADER_SIZE;

        TEST_POINT_ASSERT_UDWORD(TEST_KHEAP_ALLOC0_ID(i),
                                 (uint64_t)(uintptr_t)address[i] <= addr_expected[i],
                                 addr_expected[i],
                                 (uint64_t)(uintptr_t)address[i],
                                 TEST_KHEAP_ENABLED);

        allocated_expected[i] = (uint64_t)(uintptr_t)address[i] - next_addr + sizes[i];
        new_mem_free = kheap_get_free();
        TEST_POINT_ASSERT_UDWORD(TEST_KHEAP_MEM_FREE0_ID(i),
                                 new_mem_free == mem_free - allocated_expected[i],
                                 mem_free - allocated_expected[i],
                                 new_mem_free,
                                 TEST_KHEAP_ENABLED);

        mem_free = new_mem_free;
        addr_expected[i] = (uint64_t)(uintptr_t)address[i];
        next_addr = (uint64_t)(uintptr_t)address[i] + sizes[i];
    }

    for(i = 0; i < 200; ++i)
    {
        kfree(address[i]);
    }

    mem_free = kheap_get_free();
    for(i = 0; i < 200; ++i)
    {
        sizes[i] = sizeof(uintptr_t) * 2 * (i + 1);
        address[i] = kmalloc(sizes[i]);

        TEST_POINT_ASSERT_UDWORD(TEST_KHEAP_ALLOC1_ID(i),
                                 (uint64_t)(uintptr_t)address[i] == addr_expected[i],
                                 addr_expected[i],
                                 (uint64_t)(uintptr_t)address[i],
                                 TEST_KHEAP_ENABLED);

        new_mem_free = kheap_get_free();
        TEST_POINT_ASSERT_UDWORD(TEST_KHEAP_MEM_FREE1_ID(i),
                                 new_mem_free == mem_free - allocated_expected[i],
                                 mem_free - allocated_expected[i],
                                 new_mem_free,
                                 TEST_KHEAP_ENABLED);

        mem_free = new_mem_free;
    }
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/