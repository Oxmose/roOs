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

#define TRACE_KICKSTART_ENABLED 1
#define TRACE_X86_PIC_ENABLED   1
#define TRACE_X86_PIT_ENABLED   1
#define TRACE_X86_RTC_ENABLED   1

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
    TRACE_X86_RTC_ACK_INTERRUPT                     = 46

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