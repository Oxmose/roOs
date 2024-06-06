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

#define TRACE_KICKSTART_ENABLED     0
#define TRACE_X86_PIC_ENABLED       0
#define TRACE_X86_PIT_ENABLED       0
#define TRACE_X86_RTC_ENABLED       0
#define TRACE_X86_UART_ENABLED      0
#define TRACE_X86_VGA_TEXT_ENABLED  0
#define TRACE_X86_TSC_ENABLED       0
#define TRACE_X86_CPU_ENABLED       0
#define TRACE_DRVMGR_ENABLED        0
#define TRACE_EXCEPTION_ENABLED     0
#define TRACE_INTERRUPT_ENABLED     0
#define TRACE_KHEAP_ENABLED         0
#define TRACE_KQUEUE_ENABLED        0
#define TRACE_X86_ACPI_ENABLED      1

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/**
 * @brief Tracing events IDs
 */
typedef enum
{
    TRACE_KICKSTART_ENTRY                           = 1,
    TRACE_KICKSTART_EXIT                            = 2,

    TRACE_X86_PIC_ATTACH_ENTRY                      = 3,
    TRACE_X86_PIC_ATTACH_EXIT                       = 4,
    TRACE_X86_PIC_SET_IRQ_MASK_ENTRY                = 5,
    TRACE_X86_PIC_SET_IRQ_MASK_EXIT                 = 6,
    TRACE_X86_PIC_SET_IRQ_EOI_ENTRY                 = 7,
    TRACE_X86_PIC_SET_IRQ_EOI_EXIT                  = 8,
    TRACE_X86_PIC_HANDLE_SPURIOUS_ENTRY             = 9,
    TRACE_X86_PIC_HANDLE_SPURIOUS_EXIT              = 10,
    TRACE_X86_PIC_GET_INT_LINE_ENTRY                = 11,
    TRACE_X86_PIC_GET_INT_LINE_EXIT                 = 12,

    TRACE_X86_PIT_ATTACH_ENTRY                      = 13,
    TRACE_X86_PIT_ATTACH_EXIT                       = 14,
    TRACE_X86_PIT_ENABLE_ENTRY                      = 15,
    TRACE_X86_PIT_ENABLE_EXIT                       = 16,
    TRACE_X86_PIT_DISABLE_ENTRY                     = 17,
    TRACE_X86_PIT_DISABLE_EXIT                      = 18,
    TRACE_X86_PIT_SET_FREQUENCY_ENTRY               = 19,
    TRACE_X86_PIT_SET_FREQUENCY_EXIT                = 20,
    TRACE_X86_PIT_GET_FREQUENCY_ENTRY               = 21,
    TRACE_X86_PIT_GET_FREQUENCY_EXIT                = 22,
    TRACE_X86_PIT_SET_HANDLER_ENTRY                 = 23,
    TRACE_X86_PIT_SET_HANDLER_EXIT                  = 24,
    TRACE_X86_PIT_REMOVE_HANDLER                    = 25,
    TRACE_X86_PIT_ACK_INTERRUPT                     = 26,

    TRACE_X86_RTC_ATTACH_ENTRY                      = 27,
    TRACE_X86_RTC_ATTACH_EXIT                       = 28,
    TRACE_X86_RTC_ENABLE_ENTRY                      = 29,
    TRACE_X86_RTC_ENABLE_EXIT                       = 30,
    TRACE_X86_RTC_DISABLE_ENTRY                     = 31,
    TRACE_X86_RTC_DISABLE_EXIT                      = 32,
    TRACE_X86_RTC_SET_FREQUENCY_ENTRY               = 33,
    TRACE_X86_RTC_SET_FREQUENCY_EXIT                = 34,
    TRACE_X86_RTC_GET_FREQUENCY_ENTRY               = 35,
    TRACE_X86_RTC_GET_FREQUENCY_EXIT                = 36,
    TRACE_X86_RTC_SET_HANDLER_ENTRY                 = 37,
    TRACE_X86_RTC_SET_HANDLER_EXIT                  = 38,
    TRACE_X86_RTC_REMOVE_HANDLER                    = 39,
    TRACE_X86_RTC_GET_DAYTIME_ENTRY                 = 40,
    TRACE_X86_RTC_GET_DAYTIME_EXIT                  = 41,
    TRACE_X86_RTC_GET_DATE_ENTRY                    = 42,
    TRACE_X86_RTC_GET_DATE_EXIT                     = 43,
    TRACE_X86_RTC_UPDATETIME_ENTRY                  = 44,
    TRACE_X86_RTC_UPDATETIME_EXIT                   = 45,
    TRACE_X86_RTC_ACK_INTERRUPT                     = 46,

    TRACE_X86_UART_ATTACH_ENTRY                     = 47,
    TRACE_X86_UART_ATTACH_EXIT                      = 48,
    TRACE_X86_UART_SET_LINE                         = 49,
    TRACE_X86_UART_SET_BUFFER                       = 50,
    TRACE_X86_UART_SET_BAUDRATE                     = 51,

    TRACE_X86_VGA_TEXT_ATTACH_ENTRY                 = 52,
    TRACE_X86_VGA_TEXT_ATTACH_EXIT                  = 53,
    TRACE_X86_VGA_TEXT_PUT_STRING_ENTRY             = 54,
    TRACE_X86_VGA_TEXT_PUT_STRING_EXIT              = 55,
    TRACE_X86_VGA_TEXT_PUT_CHAR_ENTRY               = 56,
    TRACE_X86_VGA_TEXT_PUT_CHAR_EXIT                = 57,

    TRACE_X86_TSC_ATTACH_ENTRY                      = 58,
    TRACE_X86_TSC_ATTACH_EXIT                       = 59,
    TRACE_X86_TSC_ENABLE                            = 60,
    TRACE_X86_TSC_DISABLE                           = 61,
    TRACE_X86_TSC_SET_FREQUENCY                     = 62,
    TRACE_X86_TSC_GET_FREQUENCY_ENTRY               = 63,
    TRACE_X86_TSC_GET_FREQUENCY_EXIT                = 64,
    TRACE_X86_TSC_SET_HANDLER                       = 65,
    TRACE_X86_TSC_REMOVE_HANDLER                    = 66,
    TRACE_X86_TSC_SET_TIME_NS                       = 67,

    TRACE_X86_CPU_SETUP_GDT_ENTRY                   = 68,
    TRACE_X86_CPU_SETUP_GDT_EXIT                    = 69,
    TRACE_X86_CPU_SETUP_IDT_ENTRY                   = 70,
    TRACE_X86_CPU_SETUP_IDT_EXIT                    = 71,
    TRACE_X86_CPU_SETUP_TSS_ENTRY                   = 72,
    TRACE_X86_CPU_SETUP_TSS_EXIT                    = 73,
    TRACE_X86_CPU_SETUP_CPU_ENTRY                   = 74,
    TRACE_X86_CPU_SETUP_CPU_EXIT                    = 75,
    TRACE_X86_CPU_RAISE_INT_ENTRY                   = 76,
    TRACE_X86_CPU_RAISE_INT_EXIT                    = 77,
    TRACE_X86_CPU_VALIDATE_ARCH_ENTRY               = 78,
    TRACE_X86_CPU_VALIDATE_ARCH_EXIT                = 79,
    TRACE_X86_CPU_ENTER_CRITICAL                    = 80,
    TRACE_X86_CPU_EXIT_CRITICAL                     = 81,
    TRACE_X86_CPU_SPINLOCK_LOCK                     = 82,
    TRACE_X86_CPU_SPINLOCK_UNLOCK                   = 83,
    TRACE_X86_CPU_KERNEL_PANIC                      = 84,
    TRACE_X86_CPU_KERNEL_PANIC_HANDLER              = 85,

    TRACE_DRV_MGR_INIT_ENTRY                        = 86,
    TRACE_DRV_MGR_INIT_EXIT                         = 87,

    TRACE_EXCEPTION_DIVBYZERO                       = 88,
    TRACE_EXCEPTION_INIT_ENTRY                      = 89,
    TRACE_EXCEPTION_INIT_EXIT                       = 90,
    TRACE_EXCEPTION_REGISTER_ENTRY                  = 91,
    TRACE_EXCEPTION_REGISTER_EXIT                   = 92,
    TRACE_EXCEPTION_REMOVE_ENTRY                    = 93,
    TRACE_EXCEPTION_REMOVE_EXIT                     = 94,

    TRACE_INTERRUPT_HANDLER_ENTRY                   = 95,
    TRACE_INTERRUPT_HANDLER_EXIT                    = 96,
    TRACE_INTERRUPT_INIT_ENTRY                      = 97,
    TRACE_INTERRUPT_INIT_EXIT                       = 98,
    TRACE_INTERRUPT_SET_DRIVER_ENTRY                = 99,
    TRACE_INTERRUPT_SET_DRIVER_EXIT                 = 100,
    TRACE_INTERRUPT_REGISTER_ENTRY                  = 101,
    TRACE_INTERRUPT_REGISTER_EXIT                   = 102,
    TRACE_INTERRUPT_REMOVE_ENTRY                    = 103,
    TRACE_INTERRUPT_REMOVE_EXIT                     = 104,
    TRACE_INTERRUPT_REGISTER_IRQ_ENTRY              = 105,
    TRACE_INTERRUPT_REGISTER_IRQ_EXIT               = 106,
    TRACE_INTERRUPT_REMOVE_IRQ_ENTRY                = 107,
    TRACE_INTERRUPT_REMOVE_IRQ_EXIT                 = 108,
    TRACE_INTERRUPT_INTERRUPT_RESTORE               = 109,
    TRACE_INTERRUPT_INTERRUPT_DISABLE               = 110,
    TRACE_INTERRUPT_IRQ_SET_MASK                    = 111,
    TRACE_INTERRUPT_IRQ_SET_EOI                     = 112,

    TRACE_KHEAP_MALLOC_ENTRY                        = 113,
    TRACE_KHEAP_MALLOC_EXIT                         = 114,
    TRACE_KHEAP_FREE_ENTRY                          = 115,
    TRACE_KHEAP_FREE_EXIT                           = 116,

    TRACE_KQUEUE_CREATE_NODE_ENTRY                  = 117,
    TRACE_KQUEUE_CREATE_NODE_EXIT                   = 118,
    TRACE_KQUEUE_DESTROY_NODE                       = 119,
    TRACE_KQUEUE_QUEUE_CREATE_ENTRY                 = 120,
    TRACE_KQUEUE_QUEUE_CREATE_EXIT                  = 121,
    TRACE_KQUEUE_QUEUE_DESTROY                      = 122,
    TRACE_KQUEUE_QUEUE_PUSH_ENTRY                   = 123,
    TRACE_KQUEUE_QUEUE_PUSH_EXIT                    = 124,
    TRACE_KQUEUE_QUEUE_PUSH_PRIO_ENTRY              = 125,
    TRACE_KQUEUE_QUEUE_PUSH_PRIO_EXIT               = 126,
    TRACE_KQUEUE_QUEUE_POP_ENTRY                    = 127,
    TRACE_KQUEUE_QUEUE_POP_EXIT                     = 128,
    TRACE_KQUEUE_QUEUE_FIND_ENTRY                   = 129,
    TRACE_KQUEUE_QUEUE_FIND_EXIT                    = 130,
    TRACE_KQUEUE_QUEUE_REMOVE_ENTRY                 = 131,
    TRACE_KQUEUE_QUEUE_REMOVE_EXIT                  = 132,

    TRACE_X86_ACPI_ATTACH_ENTRY                     = 133,
    TRACE_X86_ACPI_ATTACH_EXIT                      = 134,
    TRACE_X86_ACPI_PARSE_RSDP_ENTRY                 = 135,
    TRACE_X86_ACPI_PARSE_RSDP_EXIT                  = 136,
    TRACE_X86_ACPI_PARSE_RSDT_ENTRY                 = 137,
    TRACE_X86_ACPI_PARSE_RSDT_EXIT                  = 138,
    TRACE_X86_ACPI_PARSE_XSDT_ENTRY                 = 139,
    TRACE_X86_ACPI_PARSE_XSDT_EXIT                  = 140,
    TRACE_X86_ACPI_PARSE_DT_ENTRY                   = 141,
    TRACE_X86_ACPI_PARSE_DT_EXIT                    = 142,
    TRACE_X86_ACPI_PARSE_FADT_ENTRY                 = 143,
    TRACE_X86_ACPI_PARSE_FADT_EXIT                  = 144,
    TRACE_X86_ACPI_PARSE_DSDT_ENTRY                 = 145,
    TRACE_X86_ACPI_PARSE_DSDT_EXIT                  = 146,
    TRACE_X86_ACPI_PARSE_APIC_ENTRY                 = 147,
    TRACE_X86_ACPI_PARSE_APIC_EXIT                  = 148,
    TRACE_X86_ACPI_PARSE_HPET_ENTRY                 = 149,
    TRACE_X86_ACPI_PARSE_HPET_EXIT                  = 150,
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