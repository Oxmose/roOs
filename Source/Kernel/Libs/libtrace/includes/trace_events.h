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