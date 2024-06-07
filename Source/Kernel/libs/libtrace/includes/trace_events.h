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

#define TRACE_ALL_ENABLED 0

#define TRACE_KICKSTART_ENABLED         (TRACE_ALL_ENABLED | 0)
#define TRACE_X86_PIC_ENABLED           (TRACE_ALL_ENABLED | 0)
#define TRACE_X86_PIT_ENABLED           (TRACE_ALL_ENABLED | 0)
#define TRACE_X86_RTC_ENABLED           (TRACE_ALL_ENABLED | 0)
#define TRACE_X86_UART_ENABLED          (TRACE_ALL_ENABLED | 0)
#define TRACE_X86_VGA_TEXT_ENABLED      (TRACE_ALL_ENABLED | 0)
#define TRACE_X86_TSC_ENABLED           (TRACE_ALL_ENABLED | 0)
#define TRACE_X86_CPU_ENABLED           (TRACE_ALL_ENABLED | 0)
#define TRACE_DRVMGR_ENABLED            (TRACE_ALL_ENABLED | 0)
#define TRACE_EXCEPTION_ENABLED         (TRACE_ALL_ENABLED | 0)
#define TRACE_INTERRUPT_ENABLED         (TRACE_ALL_ENABLED | 0)
#define TRACE_KHEAP_ENABLED             (TRACE_ALL_ENABLED | 0)
#define TRACE_KQUEUE_ENABLED            (TRACE_ALL_ENABLED | 0)
#define TRACE_X86_ACPI_ENABLED          (TRACE_ALL_ENABLED | 0)
#define TRACE_X86_IOAPIC_ENABLED        (TRACE_ALL_ENABLED | 0)
#define TRACE_X86_LAPIC_ENABLED         (TRACE_ALL_ENABLED | 0)
#define TRACE_X86_LAPIC_TIMER_ENABLED   (TRACE_ALL_ENABLED | 0)
#define TRACE_CONS_ENABLED              (TRACE_ALL_ENABLED | 0)
#define TRACE_KOUTPUT_ENABLED           (TRACE_ALL_ENABLED | 0)
#define TRACE_DEVTREE_ENABLED           (TRACE_ALL_ENABLED | 0)
#define TRACE_TIME_MGT_ENABLED          (TRACE_ALL_ENABLED | 1)


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
    TRACE_X86_ACPI_GET_REMAPED_IRQ_ENTRY            = 151,
    TRACE_X86_ACPI_GET_REMAPED_IRQ_EXIT             = 152,

    TRACE_X86_IOAPIC_ATTACH_ENTRY                   = 153,
    TRACE_X86_IOAPIC_ATTACH_EXIT                    = 154,
    TRACE_X86_IOAPIC_SET_IRQ_MASK_ENTRY             = 155,
    TRACE_X86_IOAPIC_SET_IRQ_MASK_EXIT              = 156,
    TRACE_X86_IOAPIC_SET_IRQ_MASK_FOR_ENTRY         = 157,
    TRACE_X86_IOAPIC_SET_IRQ_MASK_FOR_EXIT          = 158,
    TRACE_X86_IOAPIC_HANDLE_SPURIOUS_ENTRY          = 159,
    TRACE_X86_IOAPIC_HANDLE_SPURIOUS_EXIT           = 160,
    TRACE_X86_IOAPIC_GET_INT_LINE_ENTRY             = 161,
    TRACE_X86_IOAPIC_GET_INT_LINE_EXIT              = 162,

    TRACE_X86_LAPIC_ATTACH_ENTRY                    = 163,
    TRACE_X86_LAPIC_ATTACH_EXIT                     = 164,
    TRACE_X86_LAPIC_SET_IRQ_EOI_ENTRY               = 165,
    TRACE_X86_LAPIC_SET_IRQ_EOI_EXIT                = 166,

    TRACE_X86_LAPIC_TIMER_ATTACH_ENTRY              = 167,
    TRACE_X86_LAPIC_TIMER_ATTACH_EXIT               = 168,
    TRACE_X86_LAPIC_TIMER_CALIBRATE_ENTRY           = 169,
    TRACE_X86_LAPIC_TIMER_CALIBRATE_EXIT            = 170,
    TRACE_X86_LAPIC_TIMER_ENABLE_ENTRY              = 171,
    TRACE_X86_LAPIC_TIMER_ENABLE_EXIT               = 172,
    TRACE_X86_LAPIC_TIMER_DISABLE_ENTRY             = 173,
    TRACE_X86_LAPIC_TIMER_DISABLE_EXIT              = 174,
    TRACE_X86_LAPIC_TIMER_SET_FREQUENCY_ENTRY       = 175,
    TRACE_X86_LAPIC_TIMER_SET_FREQUENCY_EXIT        = 176,
    TRACE_X86_LAPIC_TIMER_GET_FREQUENCY             = 177,
    TRACE_X86_LAPIC_TIMER_SET_HANDLER_ENTRY         = 178,
    TRACE_X86_LAPIC_TIMER_SET_HANDLER_EXIT          = 179,
    TRACE_X86_LAPIC_TIMER_REMOVE_HANDLER            = 180,
    TRACE_X86_LAPIC_TIMER_ACK_INTERRUPT             = 181,

    TRACE_DRV_MGR_SETDEVDATA_ENTRY                  = 182,
    TRACE_DRV_MGR_SETDEVDATA_EXIT                   = 183,
    TRACE_DRV_MGR_GETDEVDATA_ENTRY                  = 184,
    TRACE_DRV_MGR_GETDEVDATA_EXIT                   = 185,

    TRACE_CONS_SET_DRIVER_ENTRY                     = 186,
    TRACE_CONS_SET_DRIVER_EXIT                      = 187,
    TRACE_CONS_CLEAR_ENTRY                          = 188,
    TRACE_CONS_CLEAR_EXIT                           = 189,
    TRACE_CONS_PUT_CURSOR_ENTRY                     = 190,
    TRACE_CONS_PUT_CURSOR_EXIT                      = 191,
    TRACE_CONS_SAVE_CURSOR_ENTRY                    = 192,
    TRACE_CONS_SAVE_CURSOR_EXIT                     = 193,
    TRACE_CONS_RESTORE_CURSOR_ENTRY                 = 194,
    TRACE_CONS_RESTORE_CURSOR_EXIT                  = 195,
    TRACE_CONS_SCROLL_ENTRY                         = 196,
    TRACE_CONS_SCROLL_EXIT                          = 197,
    TRACE_CONS_SET_COLORSCHEME_ENTRY                = 198,
    TRACE_CONS_SET_COLORSCHEME_EXIT                 = 199,
    TRACE_CONS_SAVE_COLORSCHEME_ENTRY               = 200,
    TRACE_CONS_SAVE_COLORSCHEME_EXIT                = 201,
    TRACE_CONS_PUT_STRING_ENTRY                     = 202,
    TRACE_CONS_PUT_STRING_EXIT                      = 203,
    TRACE_CONS_PUT_CHAR_ENTRY                       = 204,
    TRACE_CONS_PUT_CHAR_EXIT                        = 205,

    TRACE_KOUTPUT_FORMATER_ENTRY                    = 206,
    TRACE_KOUTPUT_FORMATER_EXIT                     = 207,
    TRACE_KOUTPUT_TAGPRINTF_ENTRY                   = 208,
    TRACE_KOUTPUT_TAGPRINTF_EXIT                    = 209,
    TRACE_KOUTPUT_TOBUFFER_STR_ENTRY                = 210,
    TRACE_KOUTPUT_TOBUFFER_STR_EXIT                 = 211,
    TRACE_KOUTPUT_TOBUFFER_CHAR_ENTRY               = 212,
    TRACE_KOUTPUT_TOBUFFER_CHAR_EXIT                = 213,
    TRACE_KOUTPUT_KPRINTF_ENTRY                     = 214,
    TRACE_KOUTPUT_KPRINTF_EXIT                      = 215,
    TRACE_KOUTPUT_KPRINTFERROR_ENTRY                = 216,
    TRACE_KOUTPUT_KPRINTFERROR_EXIT                 = 217,
    TRACE_KOUTPUT_KPRINTFSUCCESS_ENTRY              = 218,
    TRACE_KOUTPUT_KPRINTFSUCCESS_EXIT               = 219,
    TRACE_KOUTPUT_KPRINTFINFO_ENTRY                 = 220,
    TRACE_KOUTPUT_KPRINTFINFO_EXIT                  = 221,
    TRACE_KOUTPUT_KPRINTFDEBUG_ENTRY                = 222,
    TRACE_KOUTPUT_KPRINTFDEBUG_EXIT                 = 223,
    TRACE_KOUTPUT_KPRINTFFLUSH_ENTRY                = 224,
    TRACE_KOUTPUT_KPRINTFFLUSH_EXIT                 = 225,

    TRACE_DEVTREE_LINK_NODE_ENTRY                   = 226,
    TRACE_DEVTREE_LINK_NODE_EXIT                    = 227,
    TRACE_DEVTREE_LINK_PROP_ENTRY                   = 228,
    TRACE_DEVTREE_LINK_PROP_EXIT                    = 229,
    TRACE_DEVTREE_READ_PROP_ENTRY                   = 230,
    TRACE_DEVTREE_READ_PROP_EXIT                    = 231,
    TRACE_DEVTREE_ACTION_PHANDLE_ENTRY              = 232,
    TRACE_DEVTREE_ACTION_PHANDLE_EXIT               = 233,
    TRACE_DEVTREE_ACTION_ADDRCELLS_ENTRY            = 234,
    TRACE_DEVTREE_ACTION_ADDRCELLS_EXIT             = 235,
    TRACE_DEVTREE_ACTION_SIZECELLS_ENTRY            = 236,
    TRACE_DEVTREE_ACTION_SIZECELLS_EXIT             = 237,
    TRACE_DEVTREE_APPLY_PROP_ACTION_ENTRY           = 238,
    TRACE_DEVTREE_APPLY_PROP_ACTION_EXIT            = 239,
    TRACE_DEVTREE_PARSE_PROP_ENTRY                  = 240,
    TRACE_DEVTREE_PARSE_PROP_EXIT                   = 241,
    TRACE_DEVTREE_PARSE_NODE_ENTRY                  = 242,
    TRACE_DEVTREE_PARSE_NODE_EXIT                   = 243,
    TRACE_DEVTREE_INIT_ENTRY                        = 244,
    TRACE_DEVTREE_INIT_EXIT                         = 245,
    TRACE_DEVTREE_GET_PROP_ENTRY                    = 246,
    TRACE_DEVTREE_GET_PROP_EXIT                     = 247,
    TRACE_DEVTREE_GET_HANDLE_ENTRY                  = 248,
    TRACE_DEVTREE_GET_HANDLE_EXIT                   = 249,

    TRACE_TIME_MGT_MAIN_HANDLER_ENTRY               = 250,
    TRACE_TIME_MGT_MAIN_HANDLER_EXIT                = 251,
    TRACE_TIME_MGT_RTC_HANDLER_ENTRY                = 252,
    TRACE_TIME_MGT_RTC_HANDLER_EXIT                 = 253,
    TRACE_TIME_MGT_ADD_AUX_ENTRY                    = 254,
    TRACE_TIME_MGT_ADD_AUX_EXIT                     = 255,
    TRACE_TIME_MGT_ADD_TIMER_ENTRY                  = 256,
    TRACE_TIME_MGT_ADD_TIMER_EXIT                   = 257,
    TRACE_TIME_MGT_GET_UPTIME_ENTRY                 = 258,
    TRACE_TIME_MGT_GET_UPTIME_EXIT                  = 259,
    TRACE_TIME_MGT_GET_DAYTIME_ENTRY                = 260,
    TRACE_TIME_MGT_GET_DAYTIME_EXIT                 = 261,
    TRACE_TIME_MGT_GET_TICKS                        = 262,
    TRACE_TIME_MGT_WAIT_NO_SCHED_ENTRY              = 263,
    TRACE_TIME_MGT_WAIT_NO_SCHED_EXIT               = 264,
    TRACE_TIME_MGT_REG_SCHED_ENTRY                  = 265,
    TRACE_TIME_MGT_REG_SCHED_EXIT                   = 266,
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