/*******************************************************************************
 * @file kickstart.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief Kernel's main boot sequence.
 *
 * @warning At this point interrupts should be disabled.
 *
 * @details Kernel's booting sequence. Initializes the rest of the kernel and
 * performs GDT, IDT and TSS initialization. Initializes the hardware and
 * software core of the kernel before calling the scheduler.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <cpu.h>           /* CPU manager */
#include <uart.h>          /* UART driver */
#include <panic.h>         /* Kernel Panic */
#include <kheap.h>         /* Kernel heap */
#include <memory.h>        /* Memory manager */
#include <devtree.h>       /* Device tree manager */
#include <core_mgt.h>      /* Core manager */
#include <drivermgr.h>     /* Driver manager */
#include <interrupts.h>    /* Interrupt manager */
#include <exceptions.h>    /* Exception manager */
#include <kerneloutput.h>  /* Kernel logger */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
/* None */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "KICKSTART"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

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
#define KICKSTART_ASSERT(COND, MSG, ERROR) {                \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG, TRUE);               \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Main boot sequence, C kernel entry point.
 *
 * @details Main boot sequence, C kernel entry point. Initializes each basic
 * drivers for the kernel, then init the scheduler and start the system.
 *
 * @warning This function should never return. In case of return, the kernel
 * should be able to catch the return as an error.
 */
void kickstart(void);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/

/** @brief Kernel device tree loading virtual address in memory */
extern uintptr_t _KERNEL_DEV_TREE_BASE;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/* TODO: remove */
extern void scheduler_dummy_init(void);

void kickstart(void)
{
    /* Start testing framework */
    TEST_FRAMEWORK_START();

    KERNEL_TRACE_EVENT(TRACE_KICKSTART_ENABLED, TRACE_KICKSTART_ENTRY, 0);

    interruptDisable();

#if DEBUG_LOG_UART
    /* Init serial driver */
    uartDebugInit();
#endif

    /* TODO: remove */
    scheduler_dummy_init();

    KERNEL_INFO("UTK Kickstart\n");

    /* Validate architecture */
    cpuValidateArchitecture();
    KERNEL_SUCCESS("Architecture validated\n");

    /* Initialize kernel heap */
    kHeapInit();
    KERNEL_SUCCESS("Kernel heap initialized\n");

    /* Initialize the CPU */
    cpuInit();
    KERNEL_SUCCESS("CPU initialized\n");

    /* Initialize interrupts manager */
    interruptInit();
    KERNEL_SUCCESS("Interrupt manager initialized\n");

    /* Initialize exceptions manager */
    exceptionInit();
    KERNEL_SUCCESS("Exception manager initialized\n");

#if TEST_INTERRUPT_ENABLED
    TEST_FRAMEWORK_END();
#endif

    /* Init FDT */
    fdtInit((uintptr_t)&_KERNEL_DEV_TREE_BASE);
    KERNEL_SUCCESS("FDT initialized\n");

    /* Initialize the memory manager */
    memoryMgrInit();
    KERNEL_SUCCESS("Memory manager initialized\n");

    /* Init device manager */
    driverManagerInit();
    KERNEL_SUCCESS("Drivers initialized\n");

    /* Now that devices are configured, start the core manager, in charge of
     * starting other cores if needed. After calling this function all the
     * running cores excepted this one have their interrupt enabled.
     */
    coreMgtInit();

    TEST_POINT_FUNCTION_CALL(queue_test, TEST_OS_QUEUE_ENABLED);
    TEST_POINT_FUNCTION_CALL(kqueue_test, TEST_OS_KQUEUE_ENABLED);
    TEST_POINT_FUNCTION_CALL(vector_test, TEST_OS_VECTOR_ENABLED);
    TEST_POINT_FUNCTION_CALL(uhashtable_test, TEST_OS_UHASHTABLE_ENABLED);

#if TEST_KHEAP_ENABLED
    TEST_FRAMEWORK_END();
#endif

    TEST_POINT_ASSERT_RCODE(TEST_KICKSTART_END_ID,
                            TRUE,
                            OS_NO_ERR,
                            OS_NO_ERR,
                            TEST_KICKSTART_ENABLED);

#if !TEST_PANIC_ENABLED
    TEST_FRAMEWORK_END();
#endif

#if 0
    interruptRestore(1);
    uint32_t j;
    j = 0;
    for(j = 0; j < 100000; ++j)
    {
        if(j % 500 == 0)
        {
            KERNEL_DEBUG(1, "TIMETEST", ".");
        }
        cpuHalt();
    }
#endif

    KERNEL_TRACE_EVENT(TRACE_KICKSTART_ENABLED, TRACE_KICKSTART_EXIT, 0);

    /* TODO: Call scheduler */

    /* Once the scheduler is started, we should never come back here. */
    KICKSTART_ASSERT(FALSE, "Kickstart Returned", OS_ERR_UNAUTHORIZED_ACTION);
}
#undef MODULE_NAME

/************************************ EOF *************************************/