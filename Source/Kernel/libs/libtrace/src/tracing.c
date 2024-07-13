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
#include <cpu.h>      /* CPU API */
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
#define TRACE_LIB_MAGIC 0x1ACEAC1DU

/** @brief Trace library file version */
#define TRACE_LIB_VERSION 1U

/** @brief Trace library header length */
#define TRACE_LIB_HEADER_LEN 9

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

#define TRACE_CURSOR_POS(CPUID, CURSOR) (CPUID * sTraceBufferSize + CURSOR)

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
extern uint8_t _KERNEL_TRACE_BUFFER_BASE[];
/** @brief Trace buffer size set by linker */
extern size_t _KERNEL_TRACE_BUFFER_SIZE;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief Trace buffer sCursor */
static size_t sCursor[SOC_CPU_COUNT] = {0};

/** @brief Tracing lock */
static volatile uint32_t sTraceLock[SOC_CPU_COUNT] = {0};

/** @brief Tells if the tracing was sEnabled. */
static bool_t sEnabled = FALSE;

/** @brief Trace buffer address */
static uint8_t* spTraceBuffer = (uint8_t*)_KERNEL_TRACE_BUFFER_BASE;
/** @brief Trace buffer size */
static size_t sTraceBufferSize = 0;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/* Imported from platform */
extern uint64_t tracingTimerGetTick(void);

static void _kernelTraceInit(void)
{
    size_t  offset;
    uint32_t i;

    sTraceBufferSize = ((size_t)&_KERNEL_TRACE_BUFFER_SIZE) / SOC_CPU_COUNT;
    for(i = 0; i < SOC_CPU_COUNT; ++i)
    {
        /* Init the buffer */
        offset = i * sTraceBufferSize;
        memset(spTraceBuffer + offset, 0, sTraceBufferSize);

        /* Init the trace buffer header */
        *((uint32_t*)(spTraceBuffer + offset)) = TRACE_LIB_MAGIC;
        offset += sizeof(uint32_t);
        *((uint32_t*)(spTraceBuffer + offset)) = TRACE_LIB_VERSION;
        offset += sizeof(uint32_t);
        *((uint8_t*)(spTraceBuffer + offset)) = i;
        offset += sizeof(uint8_t);

        /* Init the sCursor after the header */
        sCursor[i] = TRACE_LIB_HEADER_LEN;
    }
    sEnabled = TRUE;
}

void kernelTraceEvent(const TRACE_EVENT_E kEvent,
                      const uint32_t      kFieldCount,
                      ...)
{
    __builtin_va_list args;
    uint32_t          i;
    uint64_t          timestamp;
    uint8_t           cpuId;

    timestamp = tracingTimerGetTick();
    cpuId     = cpuGetId();

    spinlockAcquire(&sTraceLock[0]);

    /* Init the tracing feature is needed */
    if(sEnabled == FALSE)
    {
        _kernelTraceInit();
    }

    spinlockRelease(&sTraceLock[0]);

    spinlockAcquire(&sTraceLock[cpuId]);

    /* Check the sCursor position and cycle if needed */
    if(sizeof(uint32_t) * (3 + kFieldCount) + sCursor[cpuId] >=
       sTraceBufferSize)
    {
        sCursor[cpuId] = TRACE_LIB_HEADER_LEN;
        while(1);
    }

    /* Write the event */
    *((uint32_t*)(spTraceBuffer + TRACE_CURSOR_POS(cpuId, sCursor[cpuId]))) = (uint32_t)kEvent;
    sCursor[cpuId] += sizeof(uint32_t);
    *((uint64_t*)(spTraceBuffer + TRACE_CURSOR_POS(cpuId, sCursor[cpuId]))) = timestamp;
    sCursor[cpuId] += sizeof(uint64_t);

    /* Get the variadic arguments */
    __builtin_va_start(args, kFieldCount);

    /* Write all metadata */
    for(i = 0; i < kFieldCount; ++i)
    {
        *((uint32_t*)(spTraceBuffer + TRACE_CURSOR_POS(cpuId, sCursor[cpuId]))) = __builtin_va_arg(args, uint32_t);
        sCursor[cpuId] += sizeof(uint32_t);
    }

    __builtin_va_end(args);

    spinlockRelease(&sTraceLock[cpuId]);
}

#endif /* #ifdef _TRACING_ENABLED */

/************************************ EOF *************************************/