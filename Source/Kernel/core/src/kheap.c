/*******************************************************************************
 * @file kheap.c
 *
 * @see kheap.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 24/05/2023
 *
 * @version 1.0
 *
 * @brief Kernel's heap allocator.
 *
 * @details Kernel's heap allocator. Allow to dynamically allocate and dealocate
 * memory on the kernel's heap.
 *
 * @warning This allocator is not suited to allocate memory for the process, you
 * should only use it for the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <stdint.h>       /* Generic integer definitions */
#include <string.h>       /* Memory manipulation */
#include <critical.h>     /* Critical sections */
#include <kerneloutput.h> /* Kernel IO */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <kheap.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/


/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/


/*******************************************************************************
 * MACROS
 ******************************************************************************/

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Start address of the kernel's heap. */
extern uint8_t _KERNEL_HEAP_BASE;
/** @brief End address of the kernel's heap. */
extern uint8_t _KERNEL_HEAP_SIZE;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

static uintptr_t head;
static uintptr_t limit;

/** @brief Heap lock */
static kernel_spinlock_t sLock;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void kHeapInit(void)
{
    head  = (uintptr_t)&_KERNEL_HEAP_BASE;
    limit = head + (uintptr_t)&_KERNEL_HEAP_SIZE;

    KERNEL_SPINLOCK_INIT(sLock);

    KERNEL_DEBUG(KHEAP_DEBUG_ENABLED, "KHEAP",
                 "Kernel Heap Initialized at 0x%p", head);
}

void* kmalloc(size_t size)
{
    void* retVal;

    KERNEL_CRITICAL_LOCK(sLock);

    if(head + size <= limit)
    {
        retVal = (void*)head;
        head += size;
    }
    else
    {
        retVal = NULL;
    }

    KERNEL_CRITICAL_UNLOCK(sLock);

    return retVal;
}

void kfree(void* ptr)
{
    (void)ptr;
}

uint32_t kHeapGetFree(void)
{
    return limit - head;
}

/************************************ EOF *************************************/