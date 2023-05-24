/*******************************************************************************
 * @file trace_events.h
 *
 * @see libtrace.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 22/04/2023
 *
 * @version 1.0
 *
 * @brief Trace event definitions.
 *
 * @details Trace event definitions. This file gathers the trace event types
 * and values.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TRACING_ENABLED

#ifndef __LIB_TRACE_EVENTS_H_
#define __LIB_TRACE_EVENTS_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <config.h> /* Kernel configuration */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/**
 * @brief Tracing events IDs
 */
typedef enum
{
    /** @brief Kernel Kickstart Start */
    EVENT_KERNEL_KICKSTART_START            = 1,
    /** @brief Kernel Kickstart End */
    EVENT_KERNEL_KICKSTART_END              = 2,
    /** @brief VGA Init Start */
    EVENT_KERNEL_VGA_INIT_START             = 3,
    /** @brief VGA Init End */
    EVENT_KERNEL_VGA_INIT_END               = 4,
    /** @brief CPU Setup GDT Start */
    EVENT_KERNEL_CPU_SET_GDT_START          = 5,
    /** @brief CPU Setup GDT End */
    EVENT_KERNEL_CPU_SET_GDT_END            = 6,
    /** @brief CPU Setup IDT Start" */
    EVENT_KERNEL_CPU_SET_IDT_START          = 7,
    /** @brief CPU Setup IDT End */
    EVENT_KERNEL_CPU_SET_IDT_END            = 8,
    /** @brief CPU Setup TSS Start" */
    EVENT_KERNEL_CPU_SET_TSS_START          = 9,
    /** @brief CPU Setup TSS End */
    EVENT_KERNEL_CPU_SET_TSS_END            = 10,
    /** @brief CPU Setup Start */
    EVENT_KERNEL_CPU_SETUP_START            = 11,
    /** @brief CPU Setup End */
    EVENT_KERNEL_CPU_SETUP_END              = 12,
    /** @brief CPU Raise Interrupt Start */
    EVENT_KERNEL_CPU_RAISE_INT_START        = 13,
    /** @brief CPU Raise Interrupt End */
    EVENT_KERNEL_CPU_RAISE_INT_END          = 14,
    /** @brief Kernel Panic Request */
    EVENT_KERNEL_PANIC                      = 15,
    /** @brief Kernel Panic Handler Start */
    EVENT_KERNEL_PANIC_HANDLER_START        = 16,
    /** @brief Kernel Panic Handler End */
    EVENT_KERNEL_PANIC_HANDLER_END          = 17,
    /** @brief Disable Interrupts */
    EVENT_KERNEL_CPU_DISABLE_INTERRUPT      = 18,
    /** @brief Enable Interrupts */
    EVENT_KERNEL_CPU_ENABLE_INTERRUPT       = 19,
    /** @brief Halt System */
    EVENT_KERNEL_HALT                       = 20,
    /** @brief Kernel General Interrupt Handler Start */
    EVENT_KERNEL_INTERRUPT_HANDLER_START    = 21,
    /** @brief Kernel General Interrupt Handler End*/
    EVENT_KERNEL_INTERRUPT_HANDLER_END      = 22,
    /** @brief Kernel Interrupt Init Start */
    EVENT_KERNEL_INTERRUPT_INIT_START       = 23,
    /** @brief Kernel Interrupt Init End*/
    EVENT_KERNEL_INTERRUPT_INIT_END         = 24,
    /** @brief Kernel Interrupt Set Driver Start */
    EVENT_KERNEL_INTERRUPT_SET_DRIVER_START = 25,
    /** @brief Kernel Interrupt Set Driver End */
    EVENT_KERNEL_INTERRUPT_SET_DRIVER_END   = 26,
    /** @brief Kernel Interrupt Register Interrupt Start */
    EVENT_KERNEL_INTERRUPT_REGISTER_START   = 27,
    /** @brief Kernel Interrupt Register Interrupt End */
    EVENT_KERNEL_INTERRUPT_REGISTER_END     = 28,
    /** @brief Kernel Interrupt Remove Interrupt Start */
    EVENT_KERNEL_INTERRUPT_REMOVE_START     = 29,
    /** @brief Kernel Interrupt Remove Interrupt End */
    EVENT_KERNEL_INTERRUPT_REMOVE_END       = 30,
    /** @brief Kernel Interrupt Register IRQ Start */
    EVENT_KERNEL_IRQ_REGISTER_START         = 31,
    /** @brief Kernel Interrupt Register IRQ End */
    EVENT_KERNEL_IRQ_REGISTER_END           = 32,
    /** @brief Kernel Interrupt Remove IRQ Start */
    EVENT_KERNEL_IRQ_REMOVE_START           = 33,
    /** @brief Kernel Interrupt Remove IRQ End */
    EVENT_KERNEL_IRQ_REMOVE_END             = 34,
    /** @brief Kernel Interrupt Restore Interrupt */
    EVENT_KERNEL_RESTORE_INTERRUPT          = 35,
    /** @brief Kernel Interrupt Disable Interrupt */
    EVENT_KERNEL_DISABLE_INTERRUPT          = 36,
    /** @brief Kernel Interrupt Set IRQ Mask */
    EVENT_KERNEL_SET_IRQ_MASK               = 37,
    /** @brief Kernel Interrupt Set IRQ EOI */
    EVENT_KERNEL_SET_IRQ_EOI                = 38,
    /** @brief Kernel Set Console Driver Start */
    EVENT_KERNEL_CONSOLE_SET_DRIVER_START   = 39,
    /** @brief Kernel Set Console Driver End */
    EVENT_KERNEL_CONSOLE_SET_DRIVER_END     = 40,
    /** @brief Kernel UART Driver Init Start */
    EVENT_KERNEL_UART_INIT_START            = 41,
    /** @brief Kernel UART Driver Init End */
    EVENT_KERNEL_UART_INIT_END              = 42,
    /** @brief Kernel Main Timer Handler */
    EVENT_KERNEL_MAIN_TIMER_HANDLER         = 43,
    /** @brief Kernel RTC Timer Handler */
    EVENT_KERNEL_RTC_TIMER_HANDLER          = 44,
    /** @brief Kernel Init Timer Manager Start */
    EVENT_KERNEL_INIT_TIME_MGT_START        = 45,
    /** @brief Kernel Init Timer Manager End */
    EVENT_KERNEL_INIT_TIME_MGT_END          = 46,
    /** @brief Kernel Get Current Uptime */
    EVENT_KERNEL_GET_CURR_UPTIME            = 47,
    /** @brief Kernel Get Tick Count */
    EVENT_KERNEL_GET_TICK_COUNT             = 48,
    /** @brief Kernel Timed Wait Nosched Start */
    EVENT_KERNEL_TIME_WAIT_NOSCHED_START    = 49,
    /** @brief Kernel Timed Wait Nosched End */
    EVENT_KERNEL_TIME_WAIT_NOSCHED_END      = 50,
    /** @brief Kernel Time Register Scheduler */
    EVENT_KERNEL_TIME_REG_SCHED             = 51,
    /** @brief Kernel Time Register RTC Manager */
    EVENT_KERNEL_TIME_REG_RTC_MGT           = 52,
    /** @brief Kernel Init PIC Start */
    EVENT_KERNEL_PIC_INIT_START             = 53,
    /** @brief Kernel Init PIC End */
    EVENT_KERNEL_PIC_INIT_END               = 54,
    /** @brief Kernel PIC Set IRQ Mask Start */
    EVENT_KERNEL_PIC_SET_IRQ_MASK_START     = 55,
    /** @brief Kernel PIC Set IRQ Mask End */
    EVENT_KERNEL_PIC_SET_IRQ_MASK_END       = 56,
    /** @brief Kernel PIC EOI Start */
    EVENT_KERNEL_PIC_EOI_START              = 57,
    /** @brief Kernel PIC EOI End */
    EVENT_KERNEL_PIC_EOI_END                = 58,
    /** @brief Kernel PIC Spurious Handler Start */
    EVENT_KERNEL_PIC_SPURIOUS_START         = 59,
    /** @brief Kernel PIC Spurious Handler End */
    EVENT_KERNEL_PIC_SPURIOUS_END           = 60,
    /** @brief Kernel PIC Disable */
    EVENT_KERNEL_PIC_DISABLE                = 61,
    /** @brief Kernel PIC Get Interrupt Line */
    EVENT_KERNEL_PIC_GET_INT_LINE           = 62,
    /** @brief Kernel PIT Dummy Handler */
    EVENT_KERNEL_PIT_DUMMY_HANDLER          = 63,
    /** @brief Kernel PIT Init Start */
    EVENT_KERNEL_PIT_INIT_START             = 64,
    /** @brief Kernel PIT Init End */
    EVENT_KERNEL_PIT_INIT_END               = 65,
    /** @brief Kernel PIT Enable Start */
    EVENT_KERNEL_PIT_ENABLE_START           = 66,
    /** @brief Kernel PIT Enable End*/
    EVENT_KERNEL_PIT_ENABLE_END             = 67,
    /** @brief Kernel PIT Disable Start */
    EVENT_KERNEL_PIT_DISABLE_START          = 68,
    /** @brief Kernel PIT Disable End */
    EVENT_KERNEL_PIT_DISABLE_END            = 69,
    /** @brief Kernel PIT Set Frequency Start */
    EVENT_KERNEL_PIT_SET_FREQ_START         = 70,
    /** @brief Kernel PIT Set Frequency End */
    EVENT_KERNEL_PIT_SET_FREQ_END           = 71,
    /** @brief Kernel PIT Set Handler */
    EVENT_KERNEL_PIT_SET_HANDLER            = 72,
    /** @brief Kernel PIT Remove Handler */
    EVENT_KERNEL_PIT_REMOVE_HANDLER         = 73,
    /** @brief Kernel RTC Dummy Handler */
    EVENT_KERNEL_RTC_DUMMY_HANDLER          = 74,
    /** @brief Kernel RTC Init Start */
    EVENT_KERNEL_RTC_INIT_START             = 75,
    /** @brief Kernel RTC Init End */
    EVENT_KERNEL_RTC_INIT_END               = 76,
    /** @brief Kernel RTC Enable Start */
    EVENT_KERNEL_RTC_ENABLE_START           = 77,
    /** @brief Kernel RTC Enable End */
    EVENT_KERNEL_RTC_ENABLE_END             = 78,
    /** @brief Kernel RTC Disable Start */
    EVENT_KERNEL_RTC_DISABLE_START          = 79,
    /** @brief Kernel RTC Disable End */
    EVENT_KERNEL_RTC_DISABLE_END            = 80,
    /** @brief Kernel RTC Set Frequency Start */
    EVENT_KERNEL_RTC_SET_FREQ_START         = 81,
    /** @brief Kernel RTC Set Frequency End */
    EVENT_KERNEL_RTC_SET_FREQ_END           = 82,
    /** @brief Kernel RTC Set Handler */
    EVENT_KERNEL_RTC_SET_HANDLER            = 83,
    /** @brief Kernel RTC Remove Handler */
    EVENT_KERNEL_RTC_REMOVE_HANDLER         = 84,
    /** @brief Kernel RTC Update Time Start */
    EVENT_KERNEL_RTC_UPDATE_TIME_START      = 85,
    /** @brief Kernel RTC Update Time End */
    EVENT_KERNEL_RTC_UPDATE_TIME_END        = 86,
    /** @brief Kernel Heap Init Start */
    EVENT_KERNEL_HEAP_INIT_START            = 87,
    /** @brief Kernel Heap Init End */
    EVENT_KERNEL_HEAP_INIT_END              = 88,
    /** @brief Kernel Heap Allocate */
    EVENT_KERNEL_HEAP_ALLOC                 = 89,
    /** @brief Kernel Heap Free */
    EVENT_KERNEL_HEAP_FREE                  = 90,
} TRACE_EVENT_E;
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
 * FUNCTIONS
 ******************************************************************************/

/* None */

#endif /* #ifndef __LIB_TRACE_EVENTS_H_ */

#endif /* #ifdef _TRACING_ENABLED */

/************************************ EOF *************************************/