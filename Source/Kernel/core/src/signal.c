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
#include <critical.h>     /* Critical sections */
#include <scheduler.h>    /* Scheduler services */
#include <kerneloutput.h> /* Kernel output */

/* Configuration files */
#include <config.h>

/* Header file */
#include <signal.h>

/* Unit test header */
#include <test_framework.h>

/* Tracing feature */
#include <tracing.h>

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
    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_HANDLE_KILL_ENTRY,
                       0);

    /* We are terminating ourselves just go to the exit point */
    schedThreadExit(schedGetCurrentThread()->terminateCause,
                    THREAD_RETURN_STATE_KILLED,
                    NULL);

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_HANDLE_KILL_EXIT,
                       0);
}

static void _handleSignalSegFault(void)
{
    kernel_thread_t* pThread;

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_HANDLE_SEGFAULT_ENTRY,
                       0);

    pThread = schedGetCurrentThread();

    KERNEL_ERROR("Segfault | Type %d: %s (%d) accessing 0x%p at 0x%p\n",
                 pThread->errorTable.exceptionId,
                 pThread->pName,
                 pThread->tid,
                 pThread->errorTable.segfaultAddr,
                 pThread->errorTable.instAddr);
    cpuPrintStackTrace(pThread->errorTable.pExecVCpu);


    /* We are terminating ourselves just go to the exit point */
    schedThreadExit(THREAD_TERMINATE_CAUSE_SEGFAULT,
                    THREAD_RETURN_STATE_KILLED,
                    NULL);

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_HANDLE_SEGFAULT_EXIT,
                       0);
}

static void _handleSignalFloatingPointExc(void)
{
    kernel_thread_t* pThread;

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_HANDLE_FPE_ENTRY,
                       0);

    pThread = schedGetCurrentThread();

    KERNEL_ERROR("Floating Point Exception: %s (%d) at 0x%p\n",
                 pThread->pName,
                 pThread->tid,
                 pThread->errorTable.instAddr);
    /* We are terminating ourselves just go to the exit point */
    schedThreadExit(THREAD_TERMINATE_CAUSE_DIV_BY_ZERO,
                    THREAD_RETURN_STATE_KILLED,
                    NULL);

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_HANDLE_FPE_EXIT,
                       0);
}

static void _handleSignalIllegalInstructionExc(void)
{
    kernel_thread_t* pThread;

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_HANDLE_ILLEGAL_INST_ENTRY,
                       0);

    pThread = schedGetCurrentThread();

    KERNEL_ERROR("Illegal Instruction Exception: %s (%d) at 0x%p\n",
                 pThread->pName,
                 pThread->tid,
                 pThread->errorTable.instAddr);
    /* We are terminating ourselves just go to the exit point */
    schedThreadExit(THREAD_TERMINATE_CAUSE_ILLEGAL_INSTRUCTION,
                    THREAD_RETURN_STATE_KILLED,
                    NULL);

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_HANDLE_ILLEGAL_INST_EXIT,
                       0);
}

static void _handleSignalException(void)
{
    kernel_thread_t* pThread;

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_HANDLE_EXCEPTION_ENTRY,
                       0);

    pThread = schedGetCurrentThread();

    KERNEL_ERROR("CPU Exception %d: %s (%d) at 0x%p\n",
                 pThread->errorTable.exceptionId,
                 pThread->pName,
                 pThread->tid,
                 pThread->errorTable.instAddr);
    cpuManageThreadException(pThread);

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_HANDLE_EXCEPTION_EXIT,
                       0);
}

void signalInitSignals(kernel_thread_t* pThread)
{
    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_INIT_SIGNALS_ENTRY,
                       2,
                       KERNEL_TRACE_HIGH(pThread),
                       KERNEL_TRACE_LOW(pThread));

    /* Reset signal mask */
    pThread->signal = 0;

    /* Simply copy the initial signal handlers table */
    memcpy(pThread->signalHandlers,
           spInitSignalHandler,
           sizeof(void*) * THREAD_MAX_SIGNALS);

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_INIT_SIGNALS_EXIT,
                       2,
                       KERNEL_TRACE_HIGH(pThread),
                       KERNEL_TRACE_LOW(pThread));
}

void signalManage(kernel_thread_t* pThread)
{
    int32_t i;

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_MANAGE_SIGNALS_ENTRY,
                       2,
                       pThread->tid,
                       pThread->signal);

    /* Check for no signals */
    if(pThread->signal == 0)
    {
        KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                           TRACE_SIGNAL_MANAGE_SIGNALS_EXIT,
                           2,
                           pThread->tid,
                           pThread->signal);
        return;
    }

    /* Highest priority is 0 */
    for(i = THREAD_MAX_SIGNALS - 1; i >= 0; --i)
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

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_MANAGE_SIGNALS_EXIT,
                       2,
                       pThread->tid,
                       pThread->signal);
}

OS_RETURN_E signalRegister(const THREAD_SIGNAL_E kSignal,
                           void                  (*pHandler)(void))
{
    kernel_thread_t* pThread;
    uint32_t         intState;

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_REGISTER_SIGNAL_ENTRY,
                       3,
                       kSignal,
                       KERNEL_TRACE_HIGH(pHandler),
                       KERNEL_TRACE_LOW(pHandler));

    if(pHandler == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                           TRACE_SIGNAL_REGISTER_SIGNAL_EXIT,
                           4,
                           kSignal,
                           KERNEL_TRACE_HIGH(pHandler),
                           KERNEL_TRACE_LOW(pHandler),
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    if(kSignal >= THREAD_MAX_SIGNALS)
    {
        KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                           TRACE_SIGNAL_REGISTER_SIGNAL_EXIT,
                           4,
                           kSignal,
                           KERNEL_TRACE_HIGH(pHandler),
                           KERNEL_TRACE_LOW(pHandler),
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    pThread = schedGetCurrentThread();

    /* Register the handler in the table */
    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pThread->lock);
    pThread->signalHandlers[kSignal] = (void*)pHandler;
    KERNEL_UNLOCK(pThread->lock);
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_REGISTER_SIGNAL_EXIT,
                       4,
                       kSignal,
                       KERNEL_TRACE_HIGH(pHandler),
                       KERNEL_TRACE_LOW(pHandler),
                       OS_NO_ERR);
    return OS_NO_ERR;
}

OS_RETURN_E signalThread(kernel_thread_t*      pThread,
                         const THREAD_SIGNAL_E kSignal)
{
    OS_RETURN_E error;
    uint32_t    intState;

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_SIGNAL_THREAD_ENTRY,
                       3,
                       kSignal,
                       KERNEL_TRACE_HIGH(pThread),
                       KERNEL_TRACE_LOW(pThread));

    if(pThread == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                           TRACE_SIGNAL_SIGNAL_THREAD_EXIT,
                           4,
                           kSignal,
                           KERNEL_TRACE_HIGH(pThread),
                           KERNEL_TRACE_LOW(pThread),
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    if(kSignal >= THREAD_MAX_SIGNALS)
    {
        KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                           TRACE_SIGNAL_SIGNAL_THREAD_EXIT,
                           4,
                           kSignal,
                           KERNEL_TRACE_HIGH(pThread),
                           KERNEL_TRACE_LOW(pThread),
                           OS_ERR_INCORRECT_VALUE);
        return OS_ERR_INCORRECT_VALUE;
    }

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pThread->lock);
    if(pThread->currentState != THREAD_STATE_ZOMBIE &&
       pThread->signalHandlers[kSignal] != NULL)
    {
        pThread->signal |= (1ULL << kSignal);
        error = OS_NO_ERR;
    }
    else
    {
        error = OS_ERR_NO_SUCH_ID;
    }
    KERNEL_UNLOCK(pThread->lock);
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    KERNEL_TRACE_EVENT(TRACE_SIGNAL_ENABLED,
                       TRACE_SIGNAL_SIGNAL_THREAD_EXIT,
                       4,
                       kSignal,
                       KERNEL_TRACE_HIGH(pThread),
                       KERNEL_TRACE_LOW(pThread),
                       error);

    return error;
}

/************************************ EOF *************************************/