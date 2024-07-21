/*******************************************************************************
 * @file syslog.c
 *
 * @see syslog.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 16/07/2024
 *
 * @version 1.0
 *
 * @brief System log services.
 *
 * @details System log services. This module provides the access to the syslog
 * services.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <panic.h>        /* Kernel panic */
#include <kheap.h>        /* Kernel heap */
#include <kqueue.h>       /* Kernel queues */
#include <string.h>       /* String manipulation */
#include <stdlib.h>       /* Standard library */
#include <stdint.h>       /* Generic int types and bool_t */
#include <stddef.h>       /* Standard definition */
#include <kerror.h>       /* Kernel error codes */
#include <time_mgt.h>     /* Time manager */
#include <scheduler.h>    /* Kernel scheduler */
#include <semaphore.h>    /* Semaphore service */
#include <kerneloutput.h> /* Kernel output */

/* Configuration files */
#include <config.h>

/* Header file */
#include <syslog.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the syslog thread priority */
#define SYSLOG_THREAD_PRIO 0
/** @brief Defines the syslog thread name */
#define SYSLOG_THREAD_NAME "syslog"
/** @brief Defines the syslog thread stack size */
#define SYSLOG_THREAD_STACK_SIZE 0x1000
/** @brief Defines the syslog thread cpu affinity */
#define SYSLOG_THREAD_AFFINITY 0
/** @brief Defines the maximal syslog size */
#define SYSLOG_MESSAGE_MAX_LENGTH 512

/** @brief Current module name */
#define MODULE_NAME "SYSLOG"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the syslog message */
typedef struct
{
    /** @brief The level message */
    SYSLOG_LEVEL_E level;

    /** @brief The space in which the massage was sent */
    bool_t isKernel;

    /** @brief The time at which the message was sent */
    uint64_t time;

    /** @brief The message that was sent */
    char* pMessage;
} syslog_msg_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the syslog to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the syslog to ensure correctness of
 * execution. Due to the critical nature of the syslog, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define SYSLOG_ASSERT(COND, MSG, ERROR) {                   \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}


/**
 * @brief Adds a padding sequence before a formated input.
 */
#define PAD_SEQ                                                         \
{                                                                       \
    strSize = strlen(tmpSeq);                                           \
                                                                        \
    while(paddingMod > strSize)                                         \
    {                                                                   \
        _syslogToBufferChar(pBuffer, &bufferPos, kSize, padCharMod);    \
        --paddingMod;                                                   \
    }                                                                   \
}

/**
 * @brief Get a sequence value argument.
 */
#define GET_SEQ_VAL(VAL, ARGS, LENGTH_MOD)                     \
{                                                              \
                                                               \
    /* Harmonize length */                                     \
    if(LENGTH_MOD > 8)                                         \
    {                                                          \
        LENGTH_MOD = 8;                                        \
    }                                                          \
                                                               \
    switch(LENGTH_MOD)                                         \
    {                                                          \
        case 1:                                                \
            VAL = (__builtin_va_arg(ARGS, uint32_t) & 0xFF);   \
            break;                                             \
        case 2:                                                \
            VAL = (__builtin_va_arg(ARGS, uint32_t) & 0xFFFF); \
            break;                                             \
        case 4:                                                \
            VAL = __builtin_va_arg(ARGS, uint32_t);            \
            break;                                             \
        case 8:                                                \
            VAL = __builtin_va_arg(ARGS, uint64_t);            \
            break;                                             \
        default:                                               \
            VAL = __builtin_va_arg(ARGS, uint32_t);            \
    }                                                          \
                                                               \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* TODO: Once we have a snprintf and equivalent, replace the formating functions
 * by those.
 */

/**
 * @brief Syslog thread routine.
 *
 * @details Syslog thread routine. This routine will print syslog message that
 * were sent.
 *
 * @param[in] args Unused.
 *
 * @return The function shall never return.
 */
static void* _syslogRoutine(void* args);

/**
 * @brief Formats a string to a buffer in memory.
 *
 * @details Formats a string to a buffer in memory. The buffer is not filled
 * more than its size.
 *
 * @param[out] pBuffer The buffer to fill.
 * @param[in] kSize The maximal size of the buffer.
 * @param[in] kFmt The format string to use.
 * @param[in] args Additional parameters for the format.
 */
static void _syslogFormatArgs(char*             pBuffer,
                              const size_t      kSize,
                              const char*       kpFmt,
                              __builtin_va_list args);

/**
 * @brief Formats a string to a buffer in memory.
 *
 * @details Formats a string to a buffer in memory. The buffer is not filled
 * more than its size.
 *
 * @param[out] pBuffer The buffer to fill.
 * @param[in] kSize The maximal size of the buffer.
 * @param[in] kFmt The format string to use.
 * @param[in] ... Additional parameters for the format.
 */
static inline void _syslogFormat(char*        pBuffer,
                                 const size_t kSize,
                                 const char*  kpFmt,
                                 ...);

/**
 * @brief Converts a string to upper case characters.
 *
 * @details Transforms all lowercase character of a NULL terminated string to
 * uppercase characters.
 *
 * @param[in, out] pString The string to tranform.
 */
static inline void _toUpper(char* pString);

/**
 * @brief Converts a string to upper case characters.
 *
 * @details Transforms all uppercase character of a NULL terminated string to
 * lowercase characters.
 *
 * @param[in, out] pString The string to tranform.
 */
static inline void _toLower(char* pString);

/**
 * @brief Copies a character to the buffer.
 *
 * @details Copies a character to the buffer. The offset of  the buffer is
 * updated in the function and the function does not copy more than the size
 * of the buffer.
 *
 * @param[out] pBuffer The buffer to fill.
 * @param[in, out] pOffset The pointer to the buffer offset. It is updated by
 * the copy.
 * @param[in] kSize The maximal size of the buffer.
 * @param[in] kChar The character to copy.
 */
static inline void _syslogToBufferChar(char*        pBuffer,
                                       size_t*      pOffset,
                                       const size_t kSize,
                                       const char   kChar);

/**
 * @brief Copies a string to the buffer.
 *
 * @details Copies a string to the buffer. The offset of  the buffer is
 * updated in the function and the function does not copy more than the size
 * of the buffer.
 *
 * @param[out] pBuffer The buffer to fill.
 * @param[in, out] pOffset The pointer to the buffer offset. It is updated by
 * the copy.
 * @param[in] kSize The maximal size of the buffer.
 * @param[in] kStr The string to copy.
 */
static inline void _syslogToBufferString(char*        pBuffer,
                                         size_t*      pOffset,
                                         const size_t kSize,
                                         const char*  kStr);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Stores the log list */
static kqueue_t* spSyslogQueue;

/** @brief Stores the syslog thread */
static kernel_thread_t* spSyslogThread;

/** @brief Stores the syslog semaphore */
static semaphore_t sSyslogSem;

/** @brief Stores if the service is initialized */
static bool_t sIsInit = FALSE;

/** @brief Stores if the service is started */
static bool_t sIsStarted = FALSE;

/** @brief Stores the syslog tags */
const char* sSyslogTags[SYSLOG_LEVEL_MAX + 1] = {
    "CRITICAL",
    "ERROR",
    "WARNING",
    "INFO",
    "DEBUG",
    "UNKNOWN"
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static inline void _toUpper(char* pString)
{
    /* For each character of the string */
    while(*pString != 0)
    {
        /* If the character is lowercase, makes it uppercase */
        if(*pString > 96 && *pString < 123)
        {
            *pString = *pString - 32;
        }
        ++pString;
    }
}

static inline void _toLower(char* pString)
{
    /* For each character of the string */
    while(*pString != 0)
    {
        /* If the character is uppercase, makes it lowercase */
        if(*pString > 64 && *pString < 91)
        {
            *pString = *pString + 32;
        }
        ++pString;
    }
}

static inline void _syslogToBufferChar(char*        pBuffer,
                                       size_t*      pOffset,
                                       const size_t kSize,
                                       const char   kChar)
{
    if(kSize - 2 > *pOffset)
    {
        pBuffer[*pOffset] = kChar;
        *pOffset += 1;
    }
}

static inline void _syslogToBufferString(char*        pBuffer,
                                         size_t*      pOffset,
                                         const size_t kSize,
                                         const char*  kStr)
{
    while(kSize - 2 > *pOffset && *kStr != 0)
    {
        pBuffer[*pOffset] = *kStr;
        *pOffset += 1;
        ++kStr;
    }
}

static void _syslogFormatArgs(char*             pBuffer,
                              const size_t      kSize,
                              const char*       kpFmt,
                              __builtin_va_list args)
{
    size_t   pos;
    size_t   strLength;
    uint64_t seqVal;
    size_t   strSize;
    uint8_t  modifier;
    uint8_t  lengthMod;
    uint8_t  paddingMod;
    bool_t   upperMod;
    char     padCharMod;
    char     tmpSeq[128];
    char*    pArgsValue;
    size_t   bufferPos;

    bufferPos  = 0;
    modifier   = 0;
    lengthMod  = 4;
    paddingMod = 0;
    upperMod   = FALSE;
    padCharMod = ' ';
    strLength  = strlen(kpFmt);

    for(pos = 0; pos < strLength; ++pos)
    {
        if(kpFmt[pos] == '%')
        {
            /* If we encouter this character in a modifier sequence, it was
             * just an escape one.
             */
            modifier = !modifier;
            if(modifier)
            {
                continue;
            }
            else
            {
                _syslogToBufferChar(pBuffer, &bufferPos, kSize, kpFmt[pos]);
            }
        }
        else if(modifier)
        {
            switch(kpFmt[pos])
            {
                /* Length mods */
                case 'h':
                    lengthMod /= 2;
                    continue;
                case 'l':
                    lengthMod *= 2;
                    continue;

                /* Specifier mods */
                case 's':
                    pArgsValue = __builtin_va_arg(args, char*);
                    _syslogToBufferString(pBuffer,
                                          &bufferPos,
                                          kSize,
                                          pArgsValue);

                    break;
                case 'd':
                case 'i':
                    GET_SEQ_VAL(seqVal, args, lengthMod);
                    memset(tmpSeq, 0, sizeof(tmpSeq));
                    itoa(seqVal, tmpSeq, 10);
                    PAD_SEQ
                    _syslogToBufferString(pBuffer,
                                          &bufferPos,
                                          kSize,
                                          tmpSeq);
                    break;
                case 'u':
                    GET_SEQ_VAL(seqVal, args, lengthMod);
                    memset(tmpSeq, 0, sizeof(tmpSeq));
                    uitoa(seqVal, tmpSeq, 10);
                    PAD_SEQ
                    _syslogToBufferString(pBuffer, &bufferPos, kSize, tmpSeq);
                    break;
                case 'X':
                    upperMod = TRUE;
                    __attribute__ ((fallthrough));
                case 'x':
                    GET_SEQ_VAL(seqVal, args, lengthMod);
                    memset(tmpSeq, 0, sizeof(tmpSeq));
                    uitoa(seqVal, tmpSeq, 16);
                    PAD_SEQ
                    if(upperMod == TRUE)
                    {
                        _toUpper(tmpSeq);
                    }
                    else
                    {
                        _toLower(tmpSeq);
                    }
                    _syslogToBufferString(pBuffer, &bufferPos, kSize, tmpSeq);
                    break;
                case 'P':
                    upperMod = TRUE;
                    __attribute__ ((fallthrough));
                case 'p':
                    paddingMod  = 2 * sizeof(uintptr_t);
                    padCharMod = '0';
                    lengthMod = sizeof(uintptr_t);
                    GET_SEQ_VAL(seqVal, args, lengthMod);
                    memset(tmpSeq, 0, sizeof(tmpSeq));
                    uitoa(seqVal, tmpSeq, 16);
                    PAD_SEQ
                    if(upperMod == TRUE)
                    {
                        _toUpper(tmpSeq);
                    }
                    else
                    {
                        _toLower(tmpSeq);
                    }
                    _syslogToBufferString(pBuffer, &bufferPos, kSize, tmpSeq);
                    break;
                case 'c':
                    lengthMod = sizeof(char);
                    GET_SEQ_VAL(tmpSeq[0], args, lengthMod);
                    _syslogToBufferChar(pBuffer, &bufferPos, kSize, tmpSeq[0]);
                    break;

                /* Padding mods */
                case '0':
                    if(paddingMod == 0)
                    {
                        padCharMod = '0';
                    }
                    else
                    {
                        paddingMod *= 10;
                    }
                    continue;
                case '1':
                    paddingMod = paddingMod * 10 + 1;
                    continue;
                case '2':
                    paddingMod = paddingMod * 10 + 2;
                    continue;
                case '3':
                    paddingMod = paddingMod * 10 + 3;
                    continue;
                case '4':
                    paddingMod = paddingMod * 10 + 4;
                    continue;
                case '5':
                    paddingMod = paddingMod * 10 + 5;
                    continue;
                case '6':
                    paddingMod = paddingMod * 10 + 6;
                    continue;
                case '7':
                    paddingMod = paddingMod * 10 + 7;
                    continue;
                case '8':
                    paddingMod = paddingMod * 10 + 8;
                    continue;
                case '9':
                    paddingMod = paddingMod * 10 + 9;
                    continue;
                default:
                    continue;
            }
        }
        else
        {
            _syslogToBufferChar(pBuffer, &bufferPos, kSize, kpFmt[pos]);
        }

        /* Reinit mods */
        lengthMod  = 4;
        paddingMod = 0;
        upperMod   = FALSE;
        padCharMod = ' ';
        modifier   = 0;
    }

    pBuffer[bufferPos] = 0;
}

static void _syslogFormat(char*        pBuffer,
                          const size_t kSize,
                          const char*  kpFmt,
                          ...)
{
    __builtin_va_list args;

    __builtin_va_start(args, kpFmt);
    _syslogFormatArgs(pBuffer, kSize, kpFmt, args);
    __builtin_va_end(args);
}

static void* _syslogRoutine(void* args)
{
    OS_RETURN_E    error;
    kqueue_node_t* pNode;
    syslog_msg_t*  pMsg;

    (void)args;

    /* We shoud never have to read logs when the service is not initialized. */
    while(sIsInit == FALSE)
    {
        schedSchedule();
    }

    sIsStarted = TRUE;

    while(TRUE)
    {
        error = semWait(&sSyslogSem);
        if(error == OS_NO_ERR)
        {
            /* Get the message node */
            pNode = kQueuePop(spSyslogQueue);
            if(pNode != NULL)
            {
                pMsg = pNode->pData;
                if(pMsg != NULL)
                {

                    kprintf(pMsg->pMessage);
                    kprintf("\n");

                    /* Free the message */
                    kfree(pMsg->pMessage);
                    kfree(pMsg);
                }

                /* Free the node */
                kQueueDestroyNode(&pNode);
            }
            else
            {
                KERNEL_ERROR("Syslog got a NULL message node\n");
            }
        }
        else
        {
            KERNEL_ERROR("Failed to wait on syslog semaphore. Error %d\n",
                         error);
        }
    }

    return NULL;
}

void syslogInit(void)
{
    /* Create the messages queue */
    spSyslogQueue = kQueueCreate(TRUE);

    sIsInit = TRUE;
}

void syslogStart(void)
{
    OS_RETURN_E error;

    /* Create the syslog thread */
    error = schedCreateKernelThread(&spSyslogThread,
                                    SYSLOG_THREAD_PRIO,
                                    SYSLOG_THREAD_NAME,
                                    SYSLOG_THREAD_STACK_SIZE,
                                    SYSLOG_THREAD_AFFINITY,
                                    _syslogRoutine,
                                    NULL);
    SYSLOG_ASSERT(error == OS_NO_ERR, "Failed to start syslog thread", error);

    /* Init the syslog semaphore */
    error = semInit(&sSyslogSem, 0, SEMAPHORE_FLAG_QUEUING_PRIO);
    SYSLOG_ASSERT(error == OS_NO_ERR, "Faield to init syslog semaphore", error);
}

OS_RETURN_E syslog(const SYSLOG_LEVEL_E kLevel,
                   const char*          kpModule,
                   const char*          kpMessage,
                   ...)
{
    uint64_t          time;
    date_t            date;
    time_t            dayTime;
    char*             pMsg;
    const char*       pTag;
    kqueue_node_t*    pNewNode;
    syslog_msg_t*     pSyslogMsg;
    OS_RETURN_E       error;
    size_t            size;
    __builtin_va_list args;
    kernel_thread_t*  pThread;
    int32_t           tid;
    const char*       pThreadName = "No Thread";

    if(sIsInit == FALSE)
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    if(kpMessage == NULL || kpModule == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Allocate the message */
    pMsg = kmalloc(SYSLOG_MESSAGE_MAX_LENGTH);
    if(pMsg == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }
    pSyslogMsg = kmalloc(sizeof(syslog_msg_t));
    if(pSyslogMsg == NULL)
    {
        kfree(pMsg);
        return OS_ERR_NULL_POINTER;
    }
    pNewNode = kQueueCreateNode(pSyslogMsg, FALSE);
    if(pNewNode == NULL)
    {
        kfree(pMsg);
        kfree(pSyslogMsg);
        return OS_ERR_NULL_POINTER;
    }

    switch(kLevel)
    {
        case SYSLOG_LEVEL_CRITICAL:
            pTag = sSyslogTags[SYSLOG_LEVEL_CRITICAL];
            break;
        case SYSLOG_LEVEL_ERROR:
            pTag = sSyslogTags[SYSLOG_LEVEL_ERROR];
            break;
        case SYSLOG_LEVEL_WARNING:
            pTag = sSyslogTags[SYSLOG_LEVEL_WARNING];
            break;
        case SYSLOG_LEVEL_INFO:
            pTag = sSyslogTags[SYSLOG_LEVEL_INFO];
            break;
        case SYSLOG_LEVEL_DEBUG:
        default:
            pTag = sSyslogTags[SYSLOG_LEVEL_DEBUG];
    }

    /* Get time and date */
    time = timeGetUptime();
    date = timeGetDate();
    dayTime = timeGetDayTime();

    /* Format the message header */
    pThread = schedGetCurrentThread();
    if(pThread != NULL)
    {
        pThreadName = pThread->pName;
        tid = pThread->tid;
    }
    else
    {
        tid = -1;
    }
    _syslogFormat(pMsg,
                  SYSLOG_MESSAGE_MAX_LENGTH - 1,
                  "%s | %02d/%02d/%04d %02d:%02d:%02d | "
                  "Uptime: %llu:%02llu:%02llu:%03llu:%03llu:%03llu  | "
                  "%s (%d) | %s | ",
                  pTag,
                  date.day, date.month, date.year,
                  dayTime.hours, dayTime.minutes, dayTime.seconds,
                  time / 360000000000LLU,
                  (time / 60000000000LLU) % 60LLU,
                  (time / 1000000000LLU) % 60LLU,
                  (time / 1000000LLU) % 1000LLU,
                  (time / 1000LLU) % 1000LLU,
                  (time % 1000LLU),
                  pThreadName,
                  tid,
                  kpModule);

    /* Format the message */
    size = strlen(pMsg);
    __builtin_va_start(args, kpMessage);
    _syslogFormatArgs(pMsg + size,
                      SYSLOG_MESSAGE_MAX_LENGTH - size - 1,
                      kpMessage,
                      args);
    __builtin_va_end(args);

    /* Setup the message */
    pSyslogMsg->level    = kLevel;
    pSyslogMsg->isKernel = TRUE;
    pSyslogMsg->time     = time;
    pSyslogMsg->pMessage = pMsg;


    if(sIsStarted == FALSE)
    {
        kprintf(pMsg);
        kprintf("\n");
    }

    /* Add message to the queue */
    kQueuePush(pNewNode, spSyslogQueue);

    /* Release the semaphore */
    error = semPost(&sSyslogSem);
    if(error != OS_NO_ERR)
    {
        kfree(pMsg);
        kfree(pSyslogMsg);
        kQueueRemove(spSyslogQueue, pNewNode, TRUE);
        kQueueDestroyNode(&pNewNode);
    }

    return error;
}

/************************************ EOF *************************************/