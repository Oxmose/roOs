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
#include <futex.h>         /* Futex library */
#include <memory.h>        /* Memory manager */
#include <devtree.h>       /* Device tree manager */
#include <console.h>       /* Kernel console */
#include <graphics.h>      /* Graphics manager */
#include <userinit.h>      /* User initialization */
#include <core_mgt.h>      /* Core manager */
#include <time_mgt.h>      /* Time manager */
#include <drivermgr.h>     /* Driver manager */
#include <scheduler.h>     /* Kernel scheduler */
#include <interrupts.h>    /* Interrupt manager */
#include <exceptions.h>    /* Exception manager */
#include <kerneloutput.h>  /* Kernel logger */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Tracing feature */
#include <tracing.h>

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
        PANIC(ERROR, MODULE_NAME, MSG);                     \
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
void kickstart(void)
{
    /* Start testing framework */
    TEST_FRAMEWORK_START();

    KERNEL_TRACE_EVENT(TRACE_KICKSTART_ENABLED, TRACE_KICKSTART_ENTRY, 0);

    /* Ensure interrupts are disabled */
    interruptDisable();

#if DEBUG_LOG_UART
    /* Init serial driver */
    uartDebugInit();
#endif

    KERNEL_INFO("roOs Kickstart\n");
    /* Initialize the scheduler */

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

    /* Init FDT */
    fdtInit((uintptr_t)&_KERNEL_DEV_TREE_BASE);
    KERNEL_SUCCESS("FDT initialized\n");

    /* Initialize the memory manager */
    memoryMgrInit();
    KERNEL_SUCCESS("Memory manager initialized\n");

    /* Init the scheduler */
    schedInit();
    KERNEL_SUCCESS("Scheduler initialized\n");

    /* Add cpu, exception and interrupt related tests here */
    TEST_POINT_FUNCTION_CALL(interruptTest, TEST_INTERRUPT_ENABLED);
    TEST_POINT_FUNCTION_CALL(exceptionTest, TEST_EXCEPTION_ENABLED);

    /* Init the defered interrupt servicing */
    interruptDeferInit();
    KERNEL_SUCCESS("Defered interrupts initialized\n");
    TEST_POINT_FUNCTION_CALL(interruptDefferTest, TEST_DEF_INTERRUPT_ENABLED);

    /* Init the futex library */
    futexLibInit();
    KERNEL_SUCCESS("Futex library initialized\n");

    /* Init device manager */
    driverManagerInit();
    KERNEL_SUCCESS("Drivers initialized\n");

    /* Init the time manager */
    timeInit();
    KERNEL_SUCCESS("Time manager initialized\n");

    /* Init the console */
    consoleInit();
    KERNEL_SUCCESS("Console initialized\n");

    /* Init the graphics manager */
    graphicsInit();
    KERNEL_SUCCESS("Graphics manager initialized\n");

    /* Now that devices are configured, start the core manager, in charge of
     * starting other cores if needed. After calling this function all the
     * running cores excepted this one have their interrupt enabled.
     */
    coreMgtInit();
    KERNEL_SUCCESS("Core manager initialized\n");

    /* Add library and core tests here */
    TEST_POINT_FUNCTION_CALL(kqueueTest, TEST_OS_KQUEUE_ENABLED);
    TEST_POINT_FUNCTION_CALL(queueTest, TEST_OS_QUEUE_ENABLED);
    TEST_POINT_FUNCTION_CALL(vectorTest, TEST_OS_VECTOR_ENABLED);
    TEST_POINT_FUNCTION_CALL(uhashtableTest, TEST_OS_UHASHTABLE_ENABLED);
    TEST_POINT_FUNCTION_CALL(semaphoreTest, TEST_SEMAPHORE_ENABLED);
    TEST_POINT_FUNCTION_CALL(mutexTest, TEST_MUTEX_ENABLED);
    TEST_POINT_FUNCTION_CALL(panicTest, TEST_PANIC_ENABLED);
    TEST_POINT_FUNCTION_CALL(signalTest, TEST_SIGNAL_ENABLED);

#ifndef _TESTING_FRAMEWORK_ENABLED
    /* Initialize the user functions */
    userInit();
    KERNEL_SUCCESS("User initialization done\n");
#endif

    KERNEL_TRACE_EVENT(TRACE_KICKSTART_ENABLED, TRACE_KICKSTART_EXIT, 0);

    /* Call first schedule */
    schedScheduleNoInt(TRUE);

    /* Once the scheduler is started, we should never come back here. */
    KICKSTART_ASSERT(FALSE, "Kickstart Returned", OS_ERR_UNAUTHORIZED_ACTION);
}

/************************************ EOF *************************************/