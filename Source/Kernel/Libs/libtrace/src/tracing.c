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
#include <critical.h> /* Kernel locks */

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
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Initializes the tracing feature of the kernel.
 *
 * @details Initializes the tracing feature of the kernel. The trace buffer
 * is initialized and the tracing feature ready to be used after this function
 * is called.
 */
static void _kernelTraceInit(void);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Trace buffer address set by linker */
extern uint32_t _KERNEL_TRACE_BUFFER_BASE[];
/** @brief Trace buffer size set by linker */
extern size_t _KERNEL_TRACE_BUFFER_SIZE;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief Trace buffer sCursor */
static size_t sCursor;

static volatile uint32_t sTraceLock = KERNEL_SPINLOCK_INIT_VALUE;

/** @brief Tells if the tracing was sEnabled. */
static bool_t sEnabled = FALSE;

/** @brief Trace buffer address */
static uint32_t* spTraceBuffer = (uint32_t*)&_KERNEL_TRACE_BUFFER_BASE;
/** @brief Trace buffer size */
static size_t sTraceBufferSize = (size_t)&_KERNEL_TRACE_BUFFER_SIZE;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _kernelTraceInit(void)
{
    /** Init the buffer */
    memset(spTraceBuffer, 0, sTraceBufferSize);

    /* Init the trace buffer header */
    spTraceBuffer[0] = TRACE_LIB_MAGIC;
    spTraceBuffer[1] = TRACE_LIB_VERSION;

    /* Init the sCursor after the header */
    sCursor = TRACE_LIB_HEADER_LEN;

    sEnabled = TRUE;
}

void kernelTraceEvent(const TRACE_EVENT_E kEvent,
                      const uint32_t      kFieldCount,
                      ...)
{
    __builtin_va_list args;
    uint32_t          i;
    uint64_t          timestamp;

    KERNEL_SPINLOCK_LOCK(sTraceLock);

    /* Init the tracing feature is needed */
    if(sEnabled == FALSE)
    {
        _kernelTraceInit();
    }

    /* Get the variadic arguments */
    __builtin_va_start(args, kFieldCount);

    /* Check the sCursor position and cycle if needed */
    if(sCursor + sizeof(kEvent) > sTraceBufferSize)
    {
        sCursor = TRACE_LIB_HEADER_LEN;
    }

    /* Write the event */
    spTraceBuffer[sCursor++] = (uint32_t)kEvent;

    /* Write the timestamp */
    timestamp = timeGetTicks(); // TODO: Use actual time getter
    spTraceBuffer[sCursor++] = (uint32_t)timestamp;
    spTraceBuffer[sCursor++] = (uint32_t)(timestamp >> 32);

    /* Write all metadata */
    for(i = 0; i < kFieldCount; ++i)
    {
        spTraceBuffer[sCursor++] = __builtin_va_arg(args, uint32_t);
    }

    __builtin_va_end(args);

    KERNEL_SPINLOCK_UNLOCK(sTraceLock);
}

#endif /* #ifdef _TRACING_ENABLED */

/************************************ EOF *************************************/