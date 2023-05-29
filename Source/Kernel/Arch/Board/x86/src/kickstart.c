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
#include <console.h>        /* Kernel console */
#include <vga_console.h>    /* VGA console driver */
#include <kernel_output.h>  /* Kernel logger */
#include <cpu.h>            /* CPU manager */
#include <panic.h>          /* Kernel Panic */
#include <uart.h>           /* UART driver */
#include <interrupts.h>     /* Interrupt manager */
#include <time_mgt.h>       /* Time management */
#include <pic.h>            /* PIC driver */
#include <pit.h>            /* PIT driver */
#include <rtc.h>            /* RTC driver */
#include <kheap.h>          /* Kernel heap */

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
 * FUNCTIONS
 ******************************************************************************/

/* TODO: remove */
extern void scheduler_dummy_init(void);

void kickstart(void)
{
    OS_RETURN_E ret_value;

    /* Start testing framework */
    TEST_FRAMEWORK_START();

    KERNEL_TRACE_EVENT(EVENT_KERNEL_KICKSTART_START, 0);

    kernel_interrupt_disable();

    /* TODO: remove */
    scheduler_dummy_init();

    /* Init serial driver */
#if DEBUG_LOG_UART
    uart_init();
    ret_value = console_set_selected_driver(uart_get_driver());
    KICKSTART_ASSERT(ret_value == OS_NO_ERR,
                     "Could not register UART driver",
                     ret_value);
#endif

    /* Register the VGA console driver for kernel console */
    vga_console_init();
    ret_value = console_set_selected_driver(vga_console_get_driver());
    KICKSTART_ASSERT(ret_value == OS_NO_ERR,
                     "Could not register VGA driver",
                     ret_value);

    console_clear_screen();

    KERNEL_INFO("UTK Kickstart\n");

    /* Initialize the CPU */
    cpu_init();
    KERNEL_SUCCESS("CPU initialized\n");

    /* Initialize interrupt manager */
    kernel_interrupt_init();
    KERNEL_SUCCESS("Interrupt manager initialized\n");

#if TEST_INTERRUPT_ENABLED
    TEST_FRAMEWORK_END();
#endif

    /* Validate architecture (must be done after interrupt init because of
     * potential kernel panic)
     */
    validate_architecture();
    KERNEL_SUCCESS("Architecture validated\n");

    /* Initialize kernel heap */
    kheap_init();
    KERNEL_SUCCESS("Kernel heap initialized\n");

    TEST_POINT_FUNCTION_CALL(queue_test, TEST_OS_QUEUE_ENABLED);
    TEST_POINT_FUNCTION_CALL(kqueue_test, TEST_OS_KQUEUE_ENABLED);
    TEST_POINT_FUNCTION_CALL(vector_test, TEST_OS_VECTOR_ENABLED);
    TEST_POINT_FUNCTION_CALL(uhashtable_test, TEST_OS_UHASHTABLE_ENABLED);

#if TEST_KHEAP_ENABLED
    TEST_FRAMEWORK_END();
#endif

    /* Initialize the PIC */
    pic_init();
    ret_value = kernel_interrupt_set_driver(pic_get_driver());
    KICKSTART_ASSERT(ret_value == OS_NO_ERR,
                     "Could register PIC in interrupt manager",
                     ret_value);
    KERNEL_SUCCESS("PIC initialized\n");

    /* Initialize the PIT */
    pit_init();
    KERNEL_SUCCESS("PIT initialized\n");

    /* Initialize the RTC */
    rtc_init();
    KERNEL_SUCCESS("RTC initialized\n");

    /* Initialize the time manager */
    ret_value = time_init(pit_get_driver(), rtc_get_driver());
    KICKSTART_ASSERT(ret_value == OS_NO_ERR,
                     "Could not initialize time manager",
                     ret_value);
    KERNEL_SUCCESS("Timer factory initialized\n");


    KERNEL_TRACE_EVENT(EVENT_KERNEL_KICKSTART_END, 0);

    TEST_POINT_ASSERT_RCODE(TEST_KICKSTART_END_ID,
                            TRUE,
                            OS_NO_ERR,
                            OS_NO_ERR,
                            TEST_KICKSTART_ENABLED);

#if !TEST_PANIC_ENABLED
    TEST_FRAMEWORK_END();
#endif

    /* Once the scheduler is started, we should never come back here. */
    KICKSTART_ASSERT(FALSE, "Kickstart Returned", OS_ERR_UNAUTHORIZED_ACTION);
}
#undef MODULE_NAME

/************************************ EOF *************************************/