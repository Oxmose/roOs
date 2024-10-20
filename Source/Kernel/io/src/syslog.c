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
#include <stdint.h>       /* Generic int types */
#include <stddef.h>       /* Standard definition */
#include <kerror.h>       /* Kernel error codes */
#include <stdbool.h>      /* Bool types */
#include <time_mgt.h>     /* Time manager */
#include <scheduler.h>    /* Kernel scheduler */
#include <ksemaphore.h>   /* Semaphore service */
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
#define SYSLOG_MESSAGE_MAX_LENGTH 2048

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
    bool isKernel;

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
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
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
static ksemaphore_t sSyslogSem;

/** @brief Stores if the service is initialized */
static bool sIsInit = false;

/** @brief Stores if the service is started */
static bool sIsStarted = false;

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

static void* _syslogRoutine(void* args)
{
    OS_RETURN_E    error;
    kqueue_node_t* pNode;
    syslog_msg_t*  pMsg;

    (void)args;

    /* We shoud never have to read logs when the service is not initialized. */
    while(sIsInit == false)
    {
        schedSchedule();
    }

    sIsStarted = true;

    while(true)
    {
        error = ksemWait(&sSyslogSem);
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
    spSyslogQueue = kQueueCreate(true);

    sIsInit = true;
}

void syslogStart(void)
{
    OS_RETURN_E error;

    /* Create the syslog thread */
    error = schedCreateThread(&spSyslogThread,
                              true,
                              SYSLOG_THREAD_PRIO,
                              SYSLOG_THREAD_NAME,
                              SYSLOG_THREAD_STACK_SIZE,
                              SYSLOG_THREAD_AFFINITY,
                              _syslogRoutine,
                              NULL);
    SYSLOG_ASSERT(error == OS_NO_ERR, "Failed to start syslog thread", error);

    /* Init the syslog semaphore */
    error = ksemInit(&sSyslogSem, 0, KSEMAPHORE_FLAG_QUEUING_PRIO);
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

    if(sIsInit == false)
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
    pNewNode = kQueueCreateNode(pSyslogMsg, false);
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
    snprintf(pMsg,
             SYSLOG_MESSAGE_MAX_LENGTH - 1,
             "%s | %02d/%02d/%04d %02d:%02d:%02d | "
             "Uptime: %llu:%02llu:%02llu:%03llu:%03llu:%03llu | "
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
    vsnprintf(pMsg + size,
              SYSLOG_MESSAGE_MAX_LENGTH - size - 1,
              kpMessage,
              args);
    __builtin_va_end(args);

    /* Setup the message */
    pSyslogMsg->level    = kLevel;
    pSyslogMsg->isKernel = true;
    pSyslogMsg->time     = time;
    pSyslogMsg->pMessage = pMsg;

    if(sIsStarted == false)
    {
        kprintf(pMsg);
        kprintf("\n");

        kfree(pMsg);
        kfree(pSyslogMsg);
        kQueueDestroyNode(&pNewNode);

        return OS_NO_ERR;
    }

    /* Add message to the queue */
    kQueuePush(pNewNode, spSyslogQueue);

    /* Release the semaphore */
    error = ksemPost(&sSyslogSem);
    if(error != OS_NO_ERR)
    {
        kfree(pMsg);
        kfree(pSyslogMsg);
        kQueueRemove(spSyslogQueue, pNewNode, true);
        kQueueDestroyNode(&pNewNode);
    }

    return error;
}

/************************************ EOF *************************************/