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
#include <vfs.h>           /* Virtual File System */
#include <uart.h>          /* UART driver */
#include <panic.h>         /* Kernel Panic */
#include <kheap.h>         /* Kernel heap */
#include <futex.h>         /* Futex library */
#include <syslog.h>        /* Syslog services */
#include <memory.h>        /* Memory manager */
#include <syslog.h>        /* Kernel Syslog */
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
#include <diskmanager.h>   /* Disk manager */

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
    uint32_t highPart;
    uint32_t lowPart;
    uint64_t entryTime;

    /* Get time */
    __asm__ __volatile__ ("rdtsc" : "=a"(lowPart), "=d"(highPart));
    entryTime = 1000000000000UL / 3600000000 *
           (((uint64_t)highPart << 32) | (uint64_t)lowPart);

    /* Return to ns and apply offset */
    entryTime /= 1000;
    /* Start testing framework */
    TEST_FRAMEWORK_START();

    /* Ensure interrupts are disabled */
    interruptDisable();

#if DEBUG_LOG_UART
    /* Init serial driver */
    uartDebugInit();
#endif

    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "roOs Kickstart");


    /* Initialize kernel heap */
    kHeapInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Kernel heap initialized");

    /* Init the system logger */
    syslogInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Syslog initialized");
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Start time: %lluns", entryTime);

    /* Validate architecture */
    cpuValidateArchitecture();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Architecture validated");

    /* Initialize the CPU */
    cpuInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "CPU initialized");

    /* Initialize interrupts manager */
    interruptInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Interrupt manager initialized");

    /* Initialize exceptions manager */
    exceptionInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Exception manager initialized");

    /* Init FDT */
    fdtInit((uintptr_t)&_KERNEL_DEV_TREE_BASE);
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "FDT initialized");

    /* Initialize the memory manager */
    memoryMgrInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Memory manager initialized");

    /* Init the scheduler */
    schedInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Scheduler initialized");

    /* Add cpu, exception and interrupt related tests here */
    TEST_POINT_FUNCTION_CALL(interruptTest, TEST_INTERRUPT_ENABLED);
    TEST_POINT_FUNCTION_CALL(exceptionTest, TEST_EXCEPTION_ENABLED);

    /* Start the system logger */
    syslogStart();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Syslog started");

    /* Init the defered interrupt servicing */
    interruptDeferInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Defered interrupts initialized");
    TEST_POINT_FUNCTION_CALL(interruptDefferTest, TEST_DEF_INTERRUPT_ENABLED);

    /* Init the futex library */
    futexLibInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Futex library initialized");

    /* Init the VFS driver */
    vfsInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "VFS initialized");

    /* Init device manager */
    driverManagerInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Drivers initialized");

    /* Init disk manager */
    diskManagerInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Disk manager initialized");

    /* Init the time manager */
    timeInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Time manager initialized");

    /* Init the console */
    consoleInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Console initialized");

    /* Init the graphics manager */
    graphicsInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Graphics manager initialized");

    /* Now that devices are configured, start the core manager, in charge of
     * starting other cores if needed. After calling this function all the
     * running cores excepted this one have their interrupt enabled.
     */
    coreMgtInit();
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Core manager initialized");

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
    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "User initialization done");
#endif

    /* Call first schedule */
    schedScheduleNoInt(TRUE);

    /* Once the scheduler is started, we should never come back here. */
    KICKSTART_ASSERT(FALSE, "Kickstart Returned", OS_ERR_UNAUTHORIZED_ACTION);
}

/************************************ EOF *************************************/