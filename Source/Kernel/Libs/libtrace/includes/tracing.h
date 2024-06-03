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

#include <stdint.h>       /* Generic int types */
#include <config.h>       /* Kernel configuration */
#include <trace_events.h> /* Trace event definitions */

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
#define KERNEL_TRACE_EVENT(...) kernelTraceEvent(__VA_ARGS__);

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
 * @param[in] kEvent The event identifier.
 * @param[in] kFieldCount The number of metadata associated with the event.
 * @param ... The event metadata associated with the event. Metadata are
 * interpreted as uint32_t.
 */
void kernelTraceEvent(const TRACE_EVENT_E kEvent,
                      const uint32_t      kFieldCount,
                      ...);

#endif /* #ifndef __LIB_TRACING_H_ */

#endif /* #ifdef _TRACING_ENABLED */

/************************************ EOF *************************************/