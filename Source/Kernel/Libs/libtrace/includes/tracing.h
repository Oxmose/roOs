/*******************************************************************************
 * @file libtrace.h
 *
 * @see libtrace.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 22/04/2023
 *
 * @version 1.0
 *
 * @brief Tracing library main file.
 *
 * @details Tracing library main file. This library allows to trace events in
 * the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_TRACING_H_
#define __LIB_TRACING_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <trace_events.h> /* Trace event definitions */
#include <stdint.h>       /* Generic int types */
#include <config.h>       /* Kernel configuration */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

#ifdef _TRACING_ENABLED

/**
 * @brief Traces an event.
 *
 * @details Traces an event, the first parameter is the type of the event
 * and the subsequent parameters are the metadata associated with the event.
 * Metadata are interpreted as uint32_t.
 *
 */
#define KERNEL_TRACE_EVENT(...) kernel_trace_event(__VA_ARGS__);

#else

#define KERNEL_TRACE_EVENT(...)

#endif /* #ifdef _TRACING_ENABLED */

#ifdef _TRACING_ENABLED

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

/**
 * @brief Traces an event.
 *
 * @details Traces an event, the first parameter is the type of the event
 * and the subsequent parameters are the metadata associated with the event.
 *
 * @param[in] event The event identifier.
 * @param[in] field_count The number of metadata associated with the event.
 * @param ... The event metadata associated with the event. Metadata are
 * interpreted as uint32_t.
 */
void kernel_trace_event(const TRACE_EVENT_E event, const uint32_t field_count,
                        ...);

#endif /* #ifndef __LIB_TRACING_H_ */

#endif /* #ifdef _TRACING_ENABLED */

/************************************ EOF *************************************/