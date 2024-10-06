/*******************************************************************************
 * @file signal.c
 *
 * @see signal.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 03/07/2024
 *
 * @version 1.0
 *
 * @brief Kernel thread signaling manager.
 *
 * @details Kernel thread signaling manager. Signal are used to communicate
 * between threads. A signal is handled the next time the thread is scheduled.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <cpu.h>          /* CPU API */
#include <string.h>       /* Memory manipulation */
#include <stdint.h>       /* Standard int types */
#include <stddef.h>       /* Standard definitions */
#include <kerror.h>       /* Kernel error codes */
#include <syslog.h>       /* Kernel Syslog */
#include <critical.h>     /* Critical sections */
#include <scheduler.h>    /* Scheduler services */

/* Configuration files */
#include <config.h>

/* Header file */
#include <signal.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "SIGNAL"

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
 * @brief Handles the kill signal.
 *
 * @details Handles the kill signal. The executing thread will call its
 * exit point and never return.
 */
static void _handleSignalKill(void);

/**
 * @brief Handles a segfault signal.
 *
 * @details Handles a segfault signal. The executing thread will call its
 * exit point and never return.
 */
static void _handleSignalSegFault(void);

/**
 * @brief Handles a floating point exception signal.
 *
 * @details Handles a floating point exception signal. The executing thread will
 * call its exit point and never return.
 */
static void _handleSignalFloatingPointExc(void);

/**
 * @brief Handles an illegal instruction exception signal.
 *
 * @details Handles an illegal instruction exception signal. The executing
 * thread will call its exit point and never return.
 */
static void _handleSignalIllegalInstructionExc(void);

/**
 * @brief Handles a CPU exception signal.
 *
 * @details Handles a CPU exception signal. The CPU internal function will be
 * called to handle the exception. This might kill the thread.
 */
static void _handleSignalException(void);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief Signal handlers table */
static void* spInitSignalHandler[THREAD_MAX_SIGNALS] = {
    NULL,
    NULL,
    NULL,
    NULL,
    _handleSignalIllegalInstructionExc, // THREAD_SIGNAL_ILL
    NULL,
    NULL,
    NULL,
    _handleSignalFloatingPointExc,      // THREAD_SIGNAL_FPE
    _handleSignalKill,                  // THREAD_SIGNAL_KILL
    NULL,                               // THREAD_SIGNAL_USR1
    _handleSignalSegFault,              // THREAD_SIGNAL_SEGV
    NULL,                               // THREAD_SIGNAL_USR2
    NULL,
    NULL,
    NULL,
    _handleSignalException,             // THREAD_SIGNAL_EXC
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _handleSignalKill(void)
{

    /* We are terminating ourselves just go to the exit point */
    schedThreadExit(schedGetCurrentThread()->terminateCause,
                    THREAD_RETURN_STATE_KILLED,
                    NULL);
}

static void _handleSignalSegFault(void)
{
    kernel_thread_t* pThread;

    pThread = schedGetCurrentThread();

    syslog(SYSLOG_LEVEL_ERROR,
           MODULE_NAME,
           "Segfault | Type %d: %s (%d) accessing 0x%p at 0x%p",
           pThread->errorTable.exceptionId,
           pThread->pName,
           pThread->tid,
           pThread->errorTable.segfaultAddr,
           pThread->errorTable.instAddr);

    cpuCoreDump(pThread->errorTable.pExecVCpu);


    /* We are terminating ourselves just go to the exit point */
    schedThreadExit(THREAD_TERMINATE_CAUSE_SEGFAULT,
                    THREAD_RETURN_STATE_KILLED,
                    NULL);
}

static void _handleSignalFloatingPointExc(void)
{
    kernel_thread_t* pThread;

    pThread = schedGetCurrentThread();

    syslog(SYSLOG_LEVEL_ERROR,
           MODULE_NAME,
           "Floating Point Exception: %s (%d) at 0x%p",
           pThread->pName,
           pThread->tid,
           pThread->errorTable.instAddr);

    /* We are terminating ourselves just go to the exit point */
    schedThreadExit(THREAD_TERMINATE_CAUSE_DIV_BY_ZERO,
                    THREAD_RETURN_STATE_KILLED,
                    NULL);
}

static void _handleSignalIllegalInstructionExc(void)
{
    kernel_thread_t* pThread;

    pThread = schedGetCurrentThread();

    syslog(SYSLOG_LEVEL_ERROR,
           MODULE_NAME,
           "Illegal Instruction Exception: %s (%d) at 0x%p",
           pThread->pName,
           pThread->tid,
           pThread->errorTable.instAddr);

    /* We are terminating ourselves just go to the exit point */
    schedThreadExit(THREAD_TERMINATE_CAUSE_ILLEGAL_INSTRUCTION,
                    THREAD_RETURN_STATE_KILLED,
                    NULL);
}

static void _handleSignalException(void)
{
    kernel_thread_t* pThread;

    pThread = schedGetCurrentThread();

    syslog(SYSLOG_LEVEL_ERROR,
           MODULE_NAME,
           "CPU Exception %d: %s (%d) at 0x%p",
           pThread->errorTable.exceptionId,
           pThread->pName,
           pThread->tid,
           pThread->errorTable.instAddr);

    cpuManageThreadException(pThread);
}

void signalInitSignals(kernel_thread_t* pThread)
{

    /* Reset signal mask */
    pThread->signal = 0;

    /* Simply copy the initial signal handlers table */
    memcpy(pThread->signalHandlers,
           spInitSignalHandler,
           sizeof(void*) * THREAD_MAX_SIGNALS);
}

void signalManage(kernel_thread_t* pThread)
{
    int32_t i;

    /* Check for no signals */
    if(pThread->signal == 0)
    {
        return;
    }

    /* Highest priority is 0 */
    for(i = 0; i < THREAD_MAX_SIGNALS; ++i)
    {
        /* Find the next signal to handle */
        if(((1ULL << i) & pThread->signal) != 0 &&
           pThread->signalHandlers[i] != NULL)
        {
            /* Redirect execution and clear signal */
            cpuRequestSignal(pThread, pThread->signalHandlers[i]);
            pThread->signal &= ~(1ULL << i);
            break;
        }
    }
}

OS_RETURN_E signalRegister(const THREAD_SIGNAL_E kSignal,
                           void                  (*pHandler)(void))
{
    kernel_thread_t* pThread;

    if(pHandler == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(kSignal >= THREAD_MAX_SIGNALS)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    pThread = schedGetCurrentThread();

    /* Register the handler in the table */
    KERNEL_LOCK(pThread->lock);
    pThread->signalHandlers[kSignal] = (void*)pHandler;
    KERNEL_UNLOCK(pThread->lock);
    return OS_NO_ERR;
}

OS_RETURN_E signalThread(kernel_thread_t*      pThread,
                         const THREAD_SIGNAL_E kSignal)
{
    OS_RETURN_E error;

    if(pThread == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(kSignal >= THREAD_MAX_SIGNALS)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    KERNEL_LOCK(pThread->lock);

    /* Check if the thread is still valid */
    if(schedIsThreadValid(pThread) == false)
    {
        KERNEL_UNLOCK(pThread->lock);
        return OS_ERR_NO_SUCH_ID;
    }

    if(pThread->signalHandlers[kSignal] != NULL)
    {
        pThread->signal |= (1ULL << kSignal);

        KERNEL_UNLOCK(pThread->lock);
        /* Release the thread */
        error = schedSetThreadToReady(pThread);
    }
    else
    {
        KERNEL_UNLOCK(pThread->lock);
        error = OS_ERR_INCORRECT_VALUE;
    }

    return error;
}

/************************************ EOF *************************************/