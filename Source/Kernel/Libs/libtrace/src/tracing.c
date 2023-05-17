/*******************************************************************************
 * @file libtrace.c
 *
 * @see libtrace.h
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

#ifdef _TRACING_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <stdint.h>   /* Generic integer definitions */
#include <string.h>   /* Memory manipulation */
#include <time_mgt.h> /* System time management */

/* Configuration files */
#include <config.h>

/* Header file */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Trace library magic */
#define TRACE_LIB_MAGIC ((uint32_t)0x1ACEAC1D)

/** @brief Trace library file version */
#define TRACE_LIB_VERSION ((uint32_t)1)

/** @brief Trace library header length */
#define TRACE_LIB_HEADER_LEN 2

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Trace buffer address set by linker */
extern uint32_t* _KERNEL_TRACE_BUFFER_BASE;
/** @brief Trace buffer size set by linker */
extern size_t _KERNEL_TRACE_BUFFER_SIZE;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief Trace buffer cursor */
static size_t cursor;

/** @brief Tells if the tracing was enabled. */
static bool_t enabled = FALSE;

/** @brief Trace buffer address */
static uint32_t* trace_buffer = (uint32_t*)&_KERNEL_TRACE_BUFFER_BASE;
/** @brief Trace buffer size */
static size_t trace_buffer_size = (size_t)&_KERNEL_TRACE_BUFFER_SIZE;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Initializes the tracing feature of the kernel.
 *
 * @details Initializes the tracing feature of the kernel. The trace buffer
 * is initialized and the tracing feature ready to be used after this function
 * is called.
 */
static void kernel_trace_init(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void kernel_trace_init(void)
{
    /** Init the buffer */
    memset(trace_buffer, 0, trace_buffer_size);

    /* Init the trace buffer header */
    trace_buffer[0] = TRACE_LIB_MAGIC;
    trace_buffer[1] = TRACE_LIB_VERSION;

    /* Init the cursor after the header */
    cursor = TRACE_LIB_HEADER_LEN;

    enabled = TRUE;
}

void kernel_trace_event(const TRACE_EVENT_E event, const uint32_t field_count,
                        ...)
{
    __builtin_va_list args;
    uint32_t          i;
    uint64_t          timestpamp;

    /* Init the tracing feature is needed */
    if(enabled == FALSE)
    {
        kernel_trace_init();
    }

    /* Get the variadic arguments */
    __builtin_va_start(args, field_count);

    /* Check the cursor position and cycle if needed */
    if(cursor + sizeof(event) > trace_buffer_size)
    {
        cursor = TRACE_LIB_HEADER_LEN;
    }

    /* Write the event */
    trace_buffer[cursor++] = (uint32_t)event;

    /* Write the timestamp */
    timestpamp = time_get_current_uptime();
    trace_buffer[cursor++] = (uint32_t)timestpamp;
    trace_buffer[cursor++] = (uint32_t)(timestpamp >> 32);

    /* Write all metadata */
    for(i = 0; i < field_count; ++i)
    {
        trace_buffer[cursor++] = __builtin_va_arg(args, uint32_t);
    }

    __builtin_va_end(args);
}

#endif /* #ifdef _TRACING_ENABLED */

/************************************ EOF *************************************/