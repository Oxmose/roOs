/*******************************************************************************
 * @file scheduler.c
 *
 * @see scheduler.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 15/06/2024
 *
 * @version 5.0
 *
 * @brief Kernel's thread scheduler.
 *
 * @details Kernel's thread scheduler. Thread creation and management functions
 * are located in this file.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <cpu.h>          /* CPU API */
#include <vfs.h>          /* VFS SysFS entries */
#include <panic.h>        /* Kernel panic */
#include <kheap.h>        /* Kernel heap */
#include <string.h>       /* Memory manipulation */
#include <stdint.h>       /* Standard int types */
#include <stddef.h>       /* Standard definitions */
#include <stdlib.h>       /* Standard library */
#include <kerror.h>       /* Kernel error codes */
#include <kqueue.h>       /* Kernel queues */
#include <memory.h>       /* Memory manager*/
#include <atomic.h>       /* Spinlocks */
#include <syslog.h>       /* Kernel Syslog */
#include <signal.h>       /* Thread signals */
#include <stdbool.h>      /* Bool types */
#include <time_mgt.h>     /* Time management services */
#include <critical.h>     /* Kernel critical */
#include <ctrl_block.h>   /* Threads and processes control block */
#include <interrupts.h>   /* Interrupt manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <scheduler.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "SCHEDULER"

/** @brief Stores the sysfs threads entry directory name */
#define SCHED_SYSFS_THREADS_DIR_PATH "/sys/threads"

/**
 * @brief Defines the size of the window for which the CPU load is calculated
 * in ticks
 */
#define CPU_LOAD_TICK_WINDOW 100

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the thread table structure */
typedef struct
{
    /**
     * @brief Stores the current highest priority in the ready tables
     */
    volatile uint8_t highestPriority;

    /** @brief Stores the number of threads in the table */
    volatile size_t threadCount;

    /**
     * @brief The list of threads in the table, by priority
     */
    kqueue_t* pReadyList[KERNEL_LOWEST_PRIORITY + 1];

    /** @brief List lock */
    kernel_spinlock_t lock;
} thread_table_t;

/** @brief Defines the thread general table structure */
typedef struct
{
    /** @brief Stores the number of threads in the table */
    volatile size_t threadCount;

    /**
     * @brief The list of threads in the table
     */
    kqueue_t* pThreadList;

    /** @brief List lock */
    kernel_spinlock_t lock;
} thread_general_table_t;

/** @brief Scheduler VFS entry types */
typedef enum
{
    /** @brief Sched entry directory */
    SYSFS_THREAD_DIR = 0,
    /** @brief Sched entry file */
    SYSFS_THREAD_ENTRY = 1,
} SCHED_SYSFS_ENTRY_TYPE_E;

/** @brief Scheduler VFS descriptor */
typedef struct
{
    /** @brief Descriptor type */
    SCHED_SYSFS_ENTRY_TYPE_E type;

    /** @brief Current descriptor offset */
    size_t offset;

    /** @brief Linked thread id */
    int32_t tid;
} sched_vfs_entry_t;

/** @brief CPU statistics structure */
typedef struct
{
    /** @brief Stores the number of times the CPU was scheduled */
    uint64_t schedCount;

    /** @brief Stores the times spent in idle in the last window */
    uint64_t idleTimes[CPU_LOAD_TICK_WINDOW];

    /** @brief Stores the total time in the last window */
    uint64_t totalTimes[CPU_LOAD_TICK_WINDOW];

    /** @brief Stores the current idle time */
    uint64_t idleTime;

    /** @brief Stores the current total time */
    uint64_t totalTime;

    /** @brief Index in the times table */
    uint32_t timesIdx;

    /** @brief Stats lock */
    kernel_spinlock_t lock;
} cpu_stat_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the scheduler to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the scheduler to ensure correctness of
 * execution. Due to the critical nature of the scheduler, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define SCHED_ASSERT(COND, MSG, ERROR) {                    \
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief IDLE thread routine.
 *
 * @details IDLE thread routine. This thread should always be ready, it is the
 * only thread running when no other trhread are ready. It allows better power
 * consumption management and CPU usage computation.
 *
 * @param[in] pArgs The argument to send to the IDLE thread, usualy NULL or the
 * CPU id.
 *
 * @warning The IDLE thread routine should never return.
 */
static void* _idleRoutine(void* pArgs);

/**
 * @brief Thread entry point routine wrapper.
 *
 * @details Thread launch routine. Wrapper for the actual thread routine. The
 * wrapper will call the thread routine, pass its arguments and gather the
 * return value of the thread function to allow the joining thread to retreive
 * it. Some statistics about the thread might be added in this function.
 */
static void _threadEntryPoint(void);

/**
 * @brief Thread's exit point.
 *
 * @details Exit point of a thread. The function will release the resources of
 * the thread and manage its children. Put the thread
 * in a THREAD_STATE_ZOMBIE state. If an other thread is already joining the
 * active thread, then the joining thread will switch from blocked to ready
 * state.
 *
 * @param[in] kCause The thread termination cause.
 * @param[in] kRetState The thread return state.
 * @param[in] pRetVal The thread return value.
 */
static void _threadExitPoint(const THREAD_TERMINATE_CAUSE_E kCause,
                             const THREAD_RETURN_STATE_E    kRetState,
                             void*                          pRetVal);

/**
 * @brief Creates the IDLE threads.
 *
 * @details Creates the IDLE threads for the scheduler. One IDLE thread will be
 * created per CPU. The IDLE thread stack is the initial kernel stack used
 * during boot.
 * This function populated the idle thread array used by the scheduler.
 *
 */
static void _createIdleThreads(void);

/**
 * @brief Updates the list of sleeping threads.
 *
 * @details Updates the list of sleeping threads. This function retrieve the
 * threads for which their wakeup time is exceeded an releases them to the
 * scheduler.
 */
static void _updateSleepingThreads(void);

/**
 * @brief Cancels the sleep of a thread.
 *
 * @details Cancels the sleep of a thread and puts it back in the ready list.
 * The thread will start executing as if its sleeping time was elapsed.
 *
 * @param[in] pThread The thread to wakeup.
 */
static void _cancelSleepThread(kernel_thread_t* pThread);

/**
 * @brief Retreive the next thread to execute from a thread table.
 *
 * @details Retreive the next thread to execute from a thread table. The next
 * thread to execute is the one with the highest priority and that was executed
 * the last in that priority value. The thread node is removed for the thread
 * table.
 *
 * @param[out] pTable The table from which to retreive the thread node.
 *
 * @return The thread to execute is returned.
 */
static kernel_thread_t* _electNextThreadFromTable(thread_table_t* pTable);

/**
 * @brief Cleans a thread memory and resources.
 *
 * @details Cleans a thread memory and resources. The thread will be removed
 * from the memory. Before calling this function, the user must ensure the
 * thread is not used in any place in the system.
 *
 * @param[in] pThread The thread to clean.
 */
static void _schedCleanThread(kernel_thread_t* pThread);

/**
 * @brief Calls the scheduler dispatch function.
 *
 * @details Calls the scheduler. This function will select the next thread to
 * schedule and execute it.
 *
 * @param[in] pThread Unused.
 *
 * @warning The current thread's context must be saved before calling this
 * function. Usually, this function is only called in interrupt handlers after
 * the thread's context was saved.
 */

static void _schedScheduleHandler(kernel_thread_t* pThread);

/**
 * @brief Creates the threads sysfs directory for the scheduler.
 *
 * @details Creates the threads sysfs directory for the scheduler. This function
 * registers the sysfs directory and its driver.
 *
 * @return The function returns the success of error state.
 */
static OS_RETURN_E _schedInitSysFSThreads(void);

/**
 * @brief Scheduler threads sysfs entries open hook.
 *
 * @details Scheduler threads sysfs entries open hook. This function returns a
 * handle to control the sysfs threads entries.
 *
 * @param[in, out] pDrvCtrl The scheduler threads sysfs driver.
 * @param[in] kpPath The path in the threads sysfs entries
 * @param[in] flags The open flags.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _schedVfsThreadsOpen(void*       pDrvCtrl,
                                  const char* kpPath,
                                  int         flags,
                                  int         mode);

/**
 * @brief Scheduler threads sysfs entries close hook.
 *
 * @details Scheduler threads sysfs entries close hook. This function closes a
 * handle that was created when calling the open function.
 *
 * @param[in, out] pDrvCtrl The scheduler threads sysfs driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _schedVfsThreadsClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief Scheduler threads sysfs entries write hook.
 *
 * @details Scheduler threads sysfs entries write hook.
 *
 * @param[in, out] pDrvCtrl The scheduler threads sysfs driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _schedVfsThreadsWrite(void*       pDrvCtrl,
                                     void*       pHandle,
                                     const void* kpBuffer,
                                     size_t      count);

/**
 * @brief Scheduler threads sysfs entries read hook.
 *
 * @details Scheduler threads sysfs entries read hook.
 *
 * @param[in, out] pDrvCtrl The scheduler threads sysfs driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _schedVfsThreadsRead(void*  pDrvCtrl,
                                    void*  pHandle,
                                    void*  pBuffer,
                                    size_t count);

/**
 * @brief Scheduler threads sysfs entries ReadDir hook.
 *
 * @details Scheduler threads sysfs entries ReadDir hook. This function performs
 * the ReadDir for the threads sysfs driver.
 *
 * @param[in, out] pDrvCtrl The scheduler threads sysfs driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] pDirEntry The directory entry to fill by the driver.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _schedVfsThreadsReadDir(void*     pDriverData,
                                       void*     pHandle,
                                       dirent_t* pDirEntry);

/**
 * @brief Scheduler threads sysfs entries IOCTL hook.
 *
 * @details Scheduler threads sysfs entries IOCTL hook. This function performs
 *  the IOCTL for the threads sysfs driver.
 *
 * @param[in, out] pDrvCtrl The scheduler threads sysfs driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _schedVfsThreadsIOCTL(void*    pDriverData,
                                     void*    pHandle,
                                     uint32_t operation,
                                     void*    pArgs);

/**
 * @brief Releases a thread to the scheduler.
 *
 * @details Releases a thread to the scheduler. This function is used to
 * put back a thread in the scheduler after locking it in, for instance, a
 * semaphore.
 *
 * @param[in] pThread The thread to release.
 * @param[in] kIsLocked Tells if the thread lock is already acquired.
 */
static void _schedReleaseThread(kernel_thread_t* pThread,
                                const bool       kIsLocked);

/**
 * @brief Manages the thread's next state.
 *
 * @details Manages the thread's next state. Depending on the thread's status
 * and its next state, the thread actual next state us updated.
 *
 * @param[in, out] pThread The thread to manage.
 */
static void _manageNextState(kernel_thread_t* pThread);

/**
 * @brief Creates the init process.
 *
 * @details Creates the init process. The function will initialize the process
 * and its attributes and allocate the required resources.
 *
 * @param[out] ppProcess The handle buffer that receives the new process'
 * handle.
 * @param[in] kpName The name of the new process.
 *
 * @return The function returns the success or error status.
 */
static OS_RETURN_E _schedCreateInitProcess(kernel_process_t** ppProcess,
                                           const char*        kpName);
/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/**
 * @brief Pointers to the current thread. One per CPU.
 * Used in assembly code
 */
kernel_thread_t* pCurrentThreadsPtr[SOC_CPU_COUNT] = {NULL};


/************************** Static global variables ***************************/

/** @brief CPUs statistics */
static cpu_stat_t sCpuStats[SOC_CPU_COUNT];

/**
 * @brief Pointers to the current process. One per CPU.
 */
static kernel_process_t* spCurrentProcessPtr[SOC_CPU_COUNT] = {NULL};

/** @brief The last PID given by the kernel. */
static u32_atomic_t sLastGivenPid;

/** @brief The last TID given by the kernel. */
static u32_atomic_t sLastGivenTid;

/** @brief The number of thread in the system. */
static u32_atomic_t sThreadCount;

/** @brief The number of processes in the system. */
static u32_atomic_t sProcessCount;

/** @brief Stores the thread tables for all CPUs */
static thread_table_t sThreadTables[SOC_CPU_COUNT];

/** @brief Stores the list of sleeping threads */
static thread_general_table_t sSleepingThreadsTable[SOC_CPU_COUNT];

/** @brief Stores the list of all threads */
static thread_general_table_t sTotalThreadsList;

/** @brief Pointers to the idle threads allocated per CPU. */
static kernel_thread_t* spIdleThread[SOC_CPU_COUNT];

/** @brief Stores the scheduler interrupt line */
static uint32_t sSchedulerInterruptLine;

/** @brief Tells the scheduler initialization state */
static bool sIsInit = false;

/** @brief Tells if the scheduler is running */
static bool sIsRunning = false;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void* _idleRoutine(void* pArgs)
{
    (void)pArgs;


#if SCHED_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "IDLE %d started on CPU %d",
           schedGetCurrentThread()->tid,
           cpuGetId());
#endif

    while(true)
    {
        interruptRestore(1);
        cpuHalt();
    }

    /* If we return better go away and cry in a corner */
    SCHED_ASSERT(false, "IDLE returned", OS_ERR_UNAUTHORIZED_ACTION);
    return NULL;
}

static void _threadEntryPoint(void)
{
    void*            pThreadReturnValue;
    kernel_thread_t* pCurrThread;

    pCurrThread = schedGetCurrentThread();

    /* Set start time */
    pCurrThread->startTime = timeGetUptime();

    /* Call the thread routine */
    pThreadReturnValue = pCurrThread->pEntryPoint(pCurrThread->pArgs);

    /* Call the exit function */
    _threadExitPoint(THREAD_TERMINATE_CORRECTLY,
                     THREAD_RETURN_STATE_RETURNED,
                     pThreadReturnValue);
}

static void _threadExitPoint(const THREAD_TERMINATE_CAUSE_E kCause,
                             const THREAD_RETURN_STATE_E    kRetState,
                             void*                          pRetVal)
{
    kernel_thread_t* pJoiningThread;
    kernel_thread_t* pCurThread;
    uint32_t         intState;

    pCurThread = schedGetCurrentThread();

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pCurThread->lock);

    /* Cannot exit idle thread */
    SCHED_ASSERT(pCurThread != spIdleThread[pCurThread->schedCpu],
                 "Cannot exit IDLE thread",
                 OS_ERR_UNAUTHORIZED_ACTION);

    /* Set new thread state and put in zombie queue */
    pCurThread->currentState = THREAD_STATE_ZOMBIE;
    pCurThread->nextState = THREAD_STATE_ZOMBIE;

    /* Set the thread's stats and state */
    pCurThread->endTime        = timeGetUptime();
    pCurThread->retValue       = pRetVal;
    pCurThread->terminateCause = kCause;
    pCurThread->returnState    = kRetState;

    /* If this is the main thread, kill all other threads in the process */
    if(pCurThread->pProcess->pMainThread == pCurThread)
    {
        /* We will never return from here */
        KERNEL_UNLOCK(pCurThread->lock);
        /* TODO: */

        SCHED_ASSERT(false,
                     "Main thread returned from process kill",
                     OS_ERR_UNAUTHORIZED_ACTION);
    }

    /* Search for joining thread */
    if(pCurThread->pJoiningThread != NULL)
    {
        pJoiningThread = pCurThread->pJoiningThread;
        KERNEL_LOCK(pCurThread->pJoiningThread->lock);

        /* Release the joining thread */
        _schedReleaseThread(pJoiningThread, true);

        KERNEL_UNLOCK(pCurThread->pJoiningThread->lock);
    }

    KERNEL_UNLOCK(pCurThread->lock);

    /* Schedule thread, no need for interrupt, the context does not need to be
     * saved.
     */
    schedScheduleNoInt(true);

    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    /* We should never return */
    SCHED_ASSERT(false,
                 "Thread retuned after exiting",
                 OS_ERR_UNAUTHORIZED_ACTION);
}

static void _createIdleThreads(void)
{
    uint32_t i;

    for(i = 0; i < SOC_CPU_COUNT; ++i)
    {
        /* Init the process, at the moment of idle threads creation the kernel
         * main process is stores in spCurrentProcessPtr[0]
         */
        spCurrentProcessPtr[i] = spCurrentProcessPtr[0];

        /* Allocate new structure */
        spIdleThread[i] = kmalloc(sizeof(kernel_thread_t));
        SCHED_ASSERT(spIdleThread[i] != NULL,
                     "Failed to allocate IDLE thread",
                     OS_ERR_NO_MORE_MEMORY);

        /* Create a new node for the threads lists */
        spIdleThread[i]->pThreadNode  = kQueueCreateNode(spIdleThread[i], true);

        /* Set the thread's information */
        spIdleThread[i]->affinity           = (1ULL << i);
        spIdleThread[i]->schedCpu           = i;
        spIdleThread[i]->tid                = atomicIncrement32(&sLastGivenTid);
        spIdleThread[i]->type               = THREAD_TYPE_KERNEL;
        spIdleThread[i]->priority           = KERNEL_LOWEST_PRIORITY;
        spIdleThread[i]->pArgs              = (void*)(uintptr_t)i;
        spIdleThread[i]->pEntryPoint        = _idleRoutine;
        spIdleThread[i]->requestSchedule    = true;
        spIdleThread[i]->preemptionDisabled = false;
        spIdleThread[i]->pProcess           = spCurrentProcessPtr[i];

        memcpy(spIdleThread[i]->pName, "idle", 5);

        /* Init lock */
        KERNEL_SPINLOCK_INIT(spIdleThread[i]->lock);

        /* Set the thread's resources */
        spIdleThread[i]->pThreadResources = kQueueCreate(true);

        /* Set the thread's stack for both interrupt and main */
        spIdleThread[i]->kernelStackEnd =
            memoryKernelMapStack(KERNEL_STACK_SIZE);
        SCHED_ASSERT(spIdleThread[i]->kernelStackEnd != (uintptr_t)NULL,
                     "Failed to allocate IDLE thread stack",
                     OS_ERR_NO_MORE_MEMORY);

        spIdleThread[i]->stackEnd  = (uintptr_t)NULL;
        spIdleThread[i]->stackSize = 0;

        /* Allocate the vCPU */
        spIdleThread[i]->pThreadVCpu =
                (void*)cpuCreateVirtualCPU(_threadEntryPoint,
                                            spIdleThread[i]->kernelStackEnd);
        SCHED_ASSERT(spIdleThread[i]->pThreadVCpu != NULL,
                     "Failed to allocate IDLE thread VCPU",
                     OS_ERR_NO_MORE_MEMORY);
        spIdleThread[i]->pSignalVCpu =
                (void*)cpuCreateVirtualCPU(NULL,
                                           spIdleThread[i]->kernelStackEnd);
        SCHED_ASSERT(spIdleThread[i]->pSignalVCpu != NULL,
                     "Failed to allocate IDLE thread signal VCPU",
                     OS_ERR_NO_MORE_MEMORY);

        /* Set the current vCPU as the regular thread vCPU */
        spIdleThread[i]->pVCpu = spIdleThread[i]->pThreadVCpu;

        /* Set idle to READY */
        spIdleThread[i]->currentState = THREAD_STATE_RUNNING;
        spIdleThread[i]->nextState = THREAD_STATE_READY;

        /* Init signal */
        signalInitSignals(spIdleThread[i]);

        /* Add thread to thread list */
        spIdleThread[i]->pThreadListNode = kQueueCreateNode(spIdleThread[i],
                                                            true);
        kQueuePush(spIdleThread[i]->pThreadListNode,
                   sTotalThreadsList.pThreadList);
        ++sTotalThreadsList.threadCount;
        ++sThreadTables[i].threadCount;
        atomicIncrement32(&sThreadCount);
    }
}

static kernel_thread_t* _electNextThreadFromTable(thread_table_t* pTable)
{
    uint8_t        nextPrio;
    uint8_t        i;
    kqueue_node_t* pThreadNode;

    pThreadNode = NULL;

    KERNEL_LOCK(pTable->lock);

    /* Get the next thread node */
    nextPrio = pTable->highestPriority;
    if(nextPrio < KERNEL_NONE_PRIORITY)
    {
        /* Get a thread from the list */
        --pTable->threadCount;
        pThreadNode = kQueuePop(pTable->pReadyList[nextPrio]);

        /* Lock the thread */
        KERNEL_LOCK(((kernel_thread_t*)pThreadNode->pData)->lock);
    }
    SCHED_ASSERT(pThreadNode != NULL,
                 "Got a NULL thread node",
                 OS_ERR_NULL_POINTER);

    /* Decrease the global highest priority if needed */
    for(i = nextPrio; i < KERNEL_NONE_PRIORITY; ++i)
    {
        /* Search for the next node */
        if(pTable->pReadyList[i]->pHead != NULL)
        {
            break;
        }
    }
    pTable->highestPriority = i;

    KERNEL_UNLOCK(pTable->lock);

    return pThreadNode->pData;
}

static void _updateSleepingThreads(void)
{
    uint64_t       currTime;
    kqueue_node_t* pThreadNode;
    kqueue_node_t* pCursor;
    uint8_t        cpuId;

    cpuId = cpuGetId();

    KERNEL_LOCK(sSleepingThreadsTable[cpuId].lock);
    if(sSleepingThreadsTable[cpuId].threadCount != 0)
    {
        /* Get the current time and compate to threads in the sleeping list */
        currTime = timeGetUptime();
        pCursor = sSleepingThreadsTable[cpuId].pThreadList->pTail;
        while(pCursor != NULL)
        {
            /* Check if we have to wake up the thread */
            if(pCursor->priority > currTime)
            {
                /* Stop here, the list if sorted */
                break;
            }

            /* Update cursor before we remove the node */
            pCursor = pCursor->pPrev;

            /* Remove the node from the list */
            --sSleepingThreadsTable[cpuId].threadCount;
            pThreadNode = kQueuePop(sSleepingThreadsTable[cpuId].pThreadList);
            SCHED_ASSERT(pThreadNode != NULL,
                         "Got a NULL thread node",
                         OS_ERR_NULL_POINTER);

            /* Release the thread */
            _schedReleaseThread(pThreadNode->pData, false);
        }
    }
    KERNEL_UNLOCK(sSleepingThreadsTable[cpuId].lock);
}

static void _cancelSleepThread(kernel_thread_t* pThread)
{
    kqueue_node_t* pCursor;
    uint8_t        cpuId;
    uint64_t       wakeupTime;
    bool           found;

    /* Check if sleeping */
    if(pThread->currentState != THREAD_STATE_SLEEPING)
    {
        if(pThread->nextState == THREAD_STATE_SLEEPING)
        {
            pThread->nextState = THREAD_STATE_READY;
        }
        return;
    }

    wakeupTime = pThread->wakeupTime;
    found = false;
    for(cpuId = 0; cpuId < SOC_CPU_COUNT; ++cpuId)
    {
        KERNEL_LOCK(sSleepingThreadsTable[cpuId].lock);
        if(sSleepingThreadsTable[cpuId].threadCount != 0)
        {
            /* Get the current time and compate to threads in the sleeping list */
            pCursor = sSleepingThreadsTable[cpuId].pThreadList->pTail;
            while(pCursor != NULL)
            {
                /* Check if we have to wake up the thread */
                if(pCursor->priority > wakeupTime)
                {
                    /* Stop here, the list if sorted */
                    break;
                }

                /* If found remove the node */
                if(pCursor == pThread->pThreadNode)
                {
                    kQueueRemove(sSleepingThreadsTable[cpuId].pThreadList,
                                 pCursor,
                                 true);
                    --sSleepingThreadsTable[cpuId].threadCount;

                    /* Release the thread */
                    _schedReleaseThread(pThread, true);

                    found = true;
                    break;
                }
                else
                {
                    /* Update cursor before we remove the node */
                    pCursor = pCursor->pPrev;
                }
            }
            if(found == true)
            {
                KERNEL_UNLOCK(sSleepingThreadsTable[cpuId].lock);
                break;
            }
        }
        KERNEL_UNLOCK(sSleepingThreadsTable[cpuId].lock);
    }
}

static void _schedCleanThread(kernel_thread_t* pThread)
{
    kqueue_node_t*     pRes;
    kqueue_t*          pResQueue;
    thread_resource_t* pResourceInfo;
    OS_RETURN_E        error;

    SCHED_ASSERT(pThread != schedGetCurrentThread(),
                 "Threads cannot clean themselves!",
                 OS_ERR_UNAUTHORIZED_ACTION);

    SCHED_ASSERT(pThread->currentState == THREAD_STATE_ZOMBIE,
                 "Can only clean zombies!",
                 OS_ERR_UNAUTHORIZED_ACTION);

    /* Clear the resources */
    pResQueue = pThread->pThreadResources;
    pRes = kQueuePop(pResQueue);
    while(pRes != NULL)
    {
        /* Release the resource */
        pResourceInfo = pRes->pData;
        pResourceInfo->pReleaseResource(pResourceInfo->pResourceData);

        kQueueDestroyNode(&pRes);

        pRes = kQueuePop(pResQueue);
    }
    kQueueDestroy((kqueue_t**)&pThread->pThreadResources);

    /* Destroy the kernel stack */
    memoryKernelUnmapStack(pThread->kernelStackEnd,
                           pThread->kernelStackSize);

    /* Destroy the regular stack */
    if(pThread->stackEnd != (uintptr_t)NULL)
    {
        /* TODO: */
    }

    /* Destroy the virutal CPUs */
    cpuDestroyVirtualCPU((uintptr_t)pThread->pThreadVCpu);
    cpuDestroyVirtualCPU((uintptr_t)pThread->pSignalVCpu);

    /* Clear the active thread node, it should not be in any queue */
    kQueueDestroyNode((kqueue_node_t**)&pThread->pThreadNode);

    /* Release the thread info node */
    KERNEL_LOCK(sTotalThreadsList.lock);
    kQueueRemove(sTotalThreadsList.pThreadList,
                 (kqueue_node_t*)pThread->pThreadListNode,
                 true);
    kQueueDestroyNode((kqueue_node_t**)&pThread->pThreadListNode);
    --sTotalThreadsList.threadCount;
    KERNEL_UNLOCK(sTotalThreadsList.lock);

    /* Remove from process link */
    KERNEL_LOCK(pThread->pProcess->lock);
    if(pThread->pNext != NULL)
    {
        pThread->pNext->pPrev = pThread->pPrev;
    }
    else if(pThread->pProcess->pThreadListTail == pThread)
    {
        pThread->pProcess->pThreadListTail = pThread->pPrev;
    }
    if(pThread->pPrev != NULL)
    {
        pThread->pPrev->pNext = pThread->pNext;
    }
    error = uhashtableRemove(pThread->pProcess->pThreadTable,
                             (uintptr_t)pThread,
                             NULL);
    SCHED_ASSERT(error == OS_NO_ERR,
                 "Failed to remove thread from process threads table",
                 error);
    KERNEL_UNLOCK(pThread->pProcess->lock);

    /* Decrement thread count */
    atomicDecrement32(&sThreadCount);

    /* Free the thread */
    kfree(pThread);
}


static OS_RETURN_E _schedInitSysFSThreads(void)
{
    vfs_driver_t sysfsThreadDriver;

    /* Register the driver */
    sysfsThreadDriver = vfsRegisterDriver(SCHED_SYSFS_THREADS_DIR_PATH,
                                          NULL,
                                          _schedVfsThreadsOpen,
                                          _schedVfsThreadsClose,
                                          _schedVfsThreadsRead,
                                          _schedVfsThreadsWrite,
                                          _schedVfsThreadsReadDir,
                                          _schedVfsThreadsIOCTL);
    if(sysfsThreadDriver == VFS_DRIVER_INVALID)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    return OS_NO_ERR;
}

static void* _schedVfsThreadsOpen(void*       pDrvCtrl,
                                  const char* kpPath,
                                  int         flags,
                                  int         mode)
{
    (void)pDrvCtrl;
    (void)mode;

    sched_vfs_entry_t* pEntry;
    int32_t            tid;
    kqueue_node_t*     pNode;
    char*              pStr;

    if(flags != O_RDONLY)
    {
        return (void*)-1;
    }

    pEntry = kmalloc(sizeof(sched_vfs_entry_t));
    if(pEntry == NULL)
    {
        return (void*)-1;
    }

    pEntry->offset = 0;

    /* Check if we want to open a thread entry of the sysfs directory */
    if(strlen(kpPath) != 0)
    {
        /* Get the tid */
        tid = strtol(kpPath, &pStr, 10);
        if(kpPath == pStr)
        {
            kfree(pEntry);
            return (void*)-1;
        }

        /* This is an entry, check if it exists */
        KERNEL_LOCK(sTotalThreadsList.lock);
        pNode = sTotalThreadsList.pThreadList->pHead;
        while(pNode != NULL)
        {
            if(((kernel_thread_t*)pNode->pData)->tid == tid)
            {
                pEntry->tid = tid;
                break;
            }
            pNode = pNode->pNext;
        }
        KERNEL_UNLOCK(sTotalThreadsList.lock);

        if(pNode == NULL)
        {
            kfree(pEntry);
            return (void*)-1;
        }

        pEntry->type = SYSFS_THREAD_ENTRY;
    }
    else
    {
        /* This is a directory */
        pEntry->type = SYSFS_THREAD_DIR;
    }

    return pEntry;
}

static int32_t _schedVfsThreadsClose(void* pDrvCtrl, void* pHandle)
{
    (void)pDrvCtrl;

    if(pHandle != NULL)
    {
        kfree(pHandle);
        return 0;
    }

    return -1;
}

static ssize_t _schedVfsThreadsWrite(void*       pDrvCtrl,
                                     void*       pHandle,
                                     const void* kpBuffer,
                                     size_t      count)
{
    (void)pDrvCtrl;
    (void)pHandle;
    (void)kpBuffer;
    (void)count;

    /* Not supported */
    return -1;
}

static ssize_t _schedVfsThreadsRead(void*  pDrvCtrl,
                                    void*  pHandle,
                                    void*  pBuffer,
                                    size_t count)
{
    kqueue_node_t*          pNode;
    kernel_thread_t*        pThread;
    int32_t                 formatSize;
    char*                   pState;
    char*                   pType;
    sched_vfs_entry_t*      pEntry;
    char                    buffer[512];

    (void)pDrvCtrl;

    if(pHandle == NULL || pBuffer == NULL)
    {
        return -1;
    }

    if(count == 0)
    {
        return 0;
    }

    pEntry = pHandle;
    if(pEntry->type != SYSFS_THREAD_ENTRY)
    {
        return -1;
    }
    /* Get the thread info */
    KERNEL_LOCK(sTotalThreadsList.lock);
    pNode = sTotalThreadsList.pThreadList->pTail;

    /* Got to the offset node */
    while(pNode != NULL)
    {
        pThread = pNode->pData;
        if(pThread->tid == pEntry->tid)
        {
            break;
        }
        pNode = pNode->pPrev;
    }

    /* We reached the end of threads, return -1 */
    if(pNode == NULL)
    {
        KERNEL_UNLOCK(sTotalThreadsList.lock);
        return -1;
    }

    /* Format */
    switch(pThread->type)
    {
        case THREAD_TYPE_KERNEL:
            pType = "kernel";
            break;
        case THREAD_TYPE_USER:
            pType = "user";
            break;
        default:
            pType = "unknown";
    }
    switch(pThread->currentState)
    {
        case THREAD_STATE_RUNNING:
            pState = "RUNNING";
            break;
        case THREAD_STATE_READY:
            pState = "READY";
            break;
        case THREAD_STATE_SLEEPING:
            pState = "SLEEPING";
            break;
        case THREAD_STATE_ZOMBIE:
            pState = "ZAOMBIE";
            break;
        case THREAD_STATE_JOINING:
            pState = "JOINING";
            break;
        case THREAD_STATE_WAITING:
            pState = "WAITING";
            break;
        default:
            pState = "UNKNOWN";
    }
    formatSize = snprintf(buffer,
                          512,
                          "PID: %d\n"
                          "Process: %s\n"
                          "TID: %d\n"
                          "Name: %s\n"
                          "Priority: %d\n"
                          "Type: %s\n"
                          "State: %s\n"
                          "Affinity: 0x%lx\n"
                          "CPU: %d",
                          pThread->pProcess->pid,
                          pThread->pProcess->pName,
                          pThread->tid,
                          pThread->pName,
                          pThread->priority,
                          pType,
                          pState,
                          pThread->affinity,
                          pThread->schedCpu);
    KERNEL_UNLOCK(sTotalThreadsList.lock);
    /* Add null terminator */
    ++formatSize;

    if(formatSize < 0)
    {
        return -1;
    }
    if(pEntry->offset >= (size_t)formatSize)
    {
        return 0;
    }

    formatSize = MIN(count, formatSize - pEntry->offset);
    memcpy(pBuffer, buffer + pEntry->offset, formatSize);
    pEntry->offset += formatSize;

    return formatSize;
}

static int32_t _schedVfsThreadsReadDir(void*     pDriverData,
                                       void*     pHandle,
                                       dirent_t* pDirEntry)
{
    sched_vfs_entry_t*      pEntry;
    size_t                  i;
    kqueue_node_t*          pNode;
    kernel_thread_t*        pThread;

    (void)pDriverData;

    if(pHandle == NULL || pDirEntry == NULL)
    {
        return -1;
    }

    pEntry = pHandle;

    if(pEntry->type != SYSFS_THREAD_DIR)
    {
        return -1;
    }

    KERNEL_LOCK(sTotalThreadsList.lock);
    pNode = sTotalThreadsList.pThreadList->pTail;

    /* Got to the offset node */
    for(i = 0; i < pEntry->offset && pNode != NULL; ++i)
    {
        pNode = pNode->pPrev;
    }

    /* We reached the end of threads, return -1 */
    if(pNode == NULL)
    {
        KERNEL_UNLOCK(sTotalThreadsList.lock);
        return -1;
    }


    /* Update the offset */
    ++pEntry->offset;

    /* Otherwise, set the entry */
    pThread = pNode->pData;
    snprintf(pDirEntry->pName, VFS_FILENAME_MAX_LENGTH, "%d", pThread->tid);

    pDirEntry->type = VFS_FILE_TYPE_FILE;

    if(pNode->pPrev == NULL)
    {
        KERNEL_UNLOCK(sTotalThreadsList.lock);
        return 0;
    }
    else
    {
        KERNEL_UNLOCK(sTotalThreadsList.lock);
        return 1;
    }
}

static ssize_t _schedVfsThreadsIOCTL(void*    pDriverData,
                                     void*    pHandle,
                                     uint32_t operation,
                                     void*    pArgs)
{
    (void)pDriverData;
    (void)pHandle;
    (void)operation;
    (void)pArgs;

    /* Not supported */
    return -1;
}

static void _schedScheduleHandler(kernel_thread_t* pThread)
{
    (void)pThread;

    /* Just call the scheduler */
    schedScheduleNoInt(false);
}

static void _schedReleaseThread(kernel_thread_t* pThread,
                                const bool       kIsLocked)
{
    uint64_t     i;
    uint8_t      cpuId;
    uint64_t     lastCpuLoad;
    uint64_t     cpuLoad;
    uint32_t     intState;

    /* Get the CPU list to release to */
    lastCpuLoad = 1000;
    cpuId       = SOC_CPU_COUNT;

    KERNEL_ENTER_CRITICAL_LOCAL(intState);

    /* Update the thread state and push to queue */
    if(kIsLocked == false)
    {
        KERNEL_LOCK(pThread->lock);
    }

    /* We do not add back zombie threads */
    if(pThread->currentState == THREAD_STATE_ZOMBIE)
    {
        if(kIsLocked == false)
        {
            KERNEL_UNLOCK(pThread->lock);
        }

        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        return;
    }

    if(pThread->affinity != 0)
    {
        for(i = 0; i < SOC_CPU_COUNT; ++i)
        {
            if(((1ULL << i) & pThread->affinity) != 0)
            {
                KERNEL_LOCK(sCpuStats[i].lock);
                if(sCpuStats[i].totalTime != 0)
                {
                    cpuLoad = 100 -
                              (100 * sCpuStats[i].idleTime /
                               sCpuStats[i].totalTime);
                }
                else
                {
                    cpuLoad = 0;
                }
                KERNEL_UNLOCK(sCpuStats[i].lock);
                if(lastCpuLoad > cpuLoad)
                {
                    lastCpuLoad = cpuLoad;
                    cpuId       = i;
                }
            }
        }
    }
    else
    {
        for(i = 0; i < SOC_CPU_COUNT; ++i)
        {
            KERNEL_LOCK(sCpuStats[i].lock);
            if(sCpuStats[i].totalTime != 0)
            {
                cpuLoad = 100 -
                         (100 * sCpuStats[i].idleTime / sCpuStats[i].totalTime);
            }
            else
            {
                cpuLoad = 0;
            }
            KERNEL_UNLOCK(sCpuStats[i].lock);

            if(lastCpuLoad > cpuLoad)
            {
                lastCpuLoad = cpuLoad;
                cpuId       = i;
            }
        }
    }

    SCHED_ASSERT(cpuId != SOC_CPU_COUNT,
                 "Failed to find a CPU to release the thread",
                 OS_ERR_INCORRECT_VALUE);
    pThread->currentState = THREAD_STATE_READY;
    pThread->schedCpu  = cpuId;

    KERNEL_LOCK(sThreadTables[cpuId].lock);

    kQueuePush(pThread->pThreadNode,
               sThreadTables[cpuId].pReadyList[pThread->priority]);
    ++sThreadTables[cpuId].threadCount;
    if(sThreadTables[cpuId].highestPriority > pThread->priority)
    {
        sThreadTables[cpuId].highestPriority = pThread->priority;
    }

    KERNEL_UNLOCK(sThreadTables[cpuId].lock);

    if(kIsLocked == false)
    {
        KERNEL_UNLOCK(pThread->lock);
    }

#if SCHED_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Mapped thread %d to CPU %d",
           pThread->tid,
           cpuId);
#endif

    KERNEL_EXIT_CRITICAL_LOCAL(intState);
}

static void _manageNextState(kernel_thread_t* pThread)
{
    uint8_t cpuId;

    /* If thread is zombie, exist */
    if(pThread->currentState == THREAD_STATE_ZOMBIE ||
       pThread->nextState == THREAD_STATE_ZOMBIE)
    {
        return;
    }

    /* Check if the thread has signals, in that case it needs to run */
    if(pThread->signal != 0)
    {
        pThread->nextState = THREAD_STATE_READY;
    }
    /* Check if the thread is elected for sleeping */
    else if(pThread->nextState == THREAD_STATE_SLEEPING)
    {
        cpuId = cpuGetId();

        /* Add to the sleeping queue */
        KERNEL_LOCK(sSleepingThreadsTable[cpuId].lock);

        kQueuePushPrio(pThread->pThreadNode,
                       sSleepingThreadsTable[cpuId].pThreadList,
                       pThread->wakeupTime);
        ++sSleepingThreadsTable[cpuId].threadCount;

        KERNEL_UNLOCK(sSleepingThreadsTable[cpuId].lock);
    }

    /* Otherwise, just update the current state */
    pThread->currentState = pThread->nextState;
}

static OS_RETURN_E _schedCreateInitProcess(kernel_process_t** ppProcess,
                                           const char*        kpName)
{
    OS_RETURN_E       error;
    kernel_process_t* pProcess;

    if(ppProcess == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Allocate the structure */
    pProcess = kmalloc(sizeof(kernel_process_t));
    if(pProcess == NULL)
    {
        return OS_ERR_NO_MORE_MEMORY;
    }
    memset(pProcess, 0, sizeof(kernel_process_t));

    /* Create process memory information */
    pProcess->pMemoryData = memoryKernelGetPageTable();
    if(pProcess->pMemoryData == NULL)
    {
        kfree(pProcess);
        return OS_ERR_NULL_POINTER;
    }

    /* Create the thread table */
    pProcess->pThreadTable = uhashtableCreate(UHASHTABLE_ALLOCATOR(kmalloc,
                                                                   kfree),
                                                &error);
    if(error != OS_NO_ERR)
    {
        kfree(pProcess);
        return error;
    }

    /* Create the futex table */
    pProcess->pFutexTable = uhashtableCreate(UHASHTABLE_ALLOCATOR(kmalloc,
                                                                  kfree),
                                               &error);
    if(error != OS_NO_ERR)
    {

        SCHED_ASSERT(uhashtableDestroy(pProcess->pThreadTable) == OS_NO_ERR,
                     "Failed to remove threads table.",
                     error);
        kfree(pProcess);
        return error;
    }

    /* Setup the process attributes */
    pProcess->pid = atomicIncrement32(&sLastGivenPid);
    pProcess->pParent = schedGetCurrentProcess();
    memcpy(pProcess->pName,
           kpName,
           MIN(PROCESS_NAME_MAX_LENGTH, strlen(kpName) + 1));
    pProcess->pName[PROCESS_NAME_MAX_LENGTH] = 0;

    KERNEL_SPINLOCK_INIT(pProcess->futexTableLock);
    KERNEL_SPINLOCK_INIT(pProcess->lock);

    *ppProcess = pProcess;

    atomicIncrement32(&sProcessCount);
    return OS_NO_ERR;
}

void schedInit(void)
{
    uint32_t    i;
    uint32_t    j;
    OS_RETURN_E error;

    /* Init values */
    sLastGivenTid = 0;
    sLastGivenPid = 0;
    sThreadCount  = 0;
    sProcessCount = 0;
    memset(pCurrentThreadsPtr, 0, sizeof(kernel_thread_t*) * SOC_CPU_COUNT);
    memset(spCurrentProcessPtr, 0, sizeof(kernel_process_t) * SOC_CPU_COUNT);
    memset(sCpuStats, 0, sizeof(cpu_stat_t) * SOC_CPU_COUNT);

    /* Create the kernel main process */
    error = _schedCreateInitProcess(&spCurrentProcessPtr[0], "ROOS_KERNEL");
    SCHED_ASSERT(error == OS_NO_ERR,
                 "Failed to create main process.",
                 OS_ERR_NO_MORE_MEMORY);

    /* Initialize the thread table */
    for(j = 0; j < SOC_CPU_COUNT; ++j)
    {
        KERNEL_SPINLOCK_INIT(sCpuStats[j].lock);
        sThreadTables[j].highestPriority = KERNEL_LOWEST_PRIORITY;
        sThreadTables[j].threadCount     = 0;
        KERNEL_SPINLOCK_INIT(sThreadTables[j].lock);
        for(i = 0; i <= KERNEL_LOWEST_PRIORITY; ++i)
        {
            sThreadTables[j].pReadyList[i]  = kQueueCreate(true);
        }
        /* Initialize the sleeping threads list */
        sSleepingThreadsTable[j].threadCount = 0;
        sSleepingThreadsTable[j].pThreadList = kQueueCreate(true);
        KERNEL_SPINLOCK_INIT(sSleepingThreadsTable[j].lock);
    }

    /* Initialize the global threads list */
    sTotalThreadsList.threadCount = 0;
    sTotalThreadsList.pThreadList = kQueueCreate(true);
    KERNEL_SPINLOCK_INIT(sTotalThreadsList.lock);

    /* Create the idle process for each CPU */
    _createIdleThreads();

    spCurrentProcessPtr[0]->pMainThread = spIdleThread[0];
    /* Set idle as current thread for all CPUs and link process */
    for(i = 0; i < SOC_CPU_COUNT; ++i)
    {
        error = uhashtableSet(spCurrentProcessPtr[0]->pThreadTable,
                              (uintptr_t)spIdleThread[i],
                              NULL);
        SCHED_ASSERT(error == OS_NO_ERR,
                     "Failed to register idle thread in process",
                     error);

        if(i != 0)
        {
            spIdleThread[i]->pPrev = spIdleThread[i - 1];
        }
        else
        {
            spIdleThread[i]->pPrev = NULL;
        }

        if(i == SOC_CPU_COUNT - 1)
        {
            spCurrentProcessPtr[0]->pThreadListTail = spIdleThread[i];
            spIdleThread[i]->pNext = NULL;
        }
        else
        {
            spIdleThread[i]->pNext = spIdleThread[i + 1];
        }
        pCurrentThreadsPtr[i] = spIdleThread[i];
    }

    /* Register the software interrupt line for scheduling */
    sSchedulerInterruptLine = cpuGetInterruptConfig()->schedulerInterruptLine;
    error = interruptRegister(sSchedulerInterruptLine, _schedScheduleHandler);
    SCHED_ASSERT(error == OS_NO_ERR,
                 "Failed to register scheduler interrupt",
                 error);

    /* Create the thread sysfs entries */
    error = _schedInitSysFSThreads();
    SCHED_ASSERT(error == OS_NO_ERR,
                 "Failed to register scheduler threads sysfs",
                 error);

    sIsInit = true;

#if SCHED_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Scheduler initialization end");
#endif
}

void schedScheduleNoInt(const bool kForceSwitch)
{
    uint8_t          cpuId;
    uint8_t          currPrio;
    uint8_t          nextPrio;
    uint64_t         upTime;
    thread_table_t*  pCurrentTable;
    kernel_thread_t* pThread;

    SCHED_ASSERT(cpuGetIntState() == 0,
                 "Called scheduler no int with interrupt enabled",
                 OS_ERR_UNAUTHORIZED_ACTION);

    /* Get current CPU ID */
    cpuId = cpuGetId();

    sIsRunning = true;
    pThread = pCurrentThreadsPtr[cpuId];

    /* Update the CPU statistics */
    upTime = timeGetUptime();
    KERNEL_LOCK(sCpuStats[cpuId].lock);
    if(pThread == spIdleThread[cpuId])
    {
        sCpuStats[cpuId].idleTimes[sCpuStats[cpuId].timesIdx] =
            upTime -
            sCpuStats[cpuId].idleTimes[sCpuStats[cpuId].timesIdx];
    }
    sCpuStats[cpuId].totalTimes[sCpuStats[cpuId].timesIdx] =
        upTime -
        sCpuStats[cpuId].totalTimes[sCpuStats[cpuId].timesIdx];

    sCpuStats[cpuId].totalTime +=
        sCpuStats[cpuId].totalTimes[sCpuStats[cpuId].timesIdx];
    sCpuStats[cpuId].idleTime +=
        sCpuStats[cpuId].idleTimes[sCpuStats[cpuId].timesIdx];
    KERNEL_UNLOCK(sCpuStats[cpuId].lock);

    /* Wakeup sleeping threads if needed */
    _updateSleepingThreads();

    KERNEL_LOCK(pThread->lock);
    /* Update the thread states */
    _manageNextState(pThread);

    pCurrentTable = &sThreadTables[cpuId];
    KERNEL_LOCK(pCurrentTable->lock);

    /* If the current process can still run */
    if(pThread->nextState == THREAD_STATE_READY &&
       pThread->preemptionDisabled == false)
    {
        nextPrio = pCurrentTable->highestPriority;
        currPrio = pThread->priority;

        /* But there might be a higher prio or another thread with same
         * priority and schedule was requested.
         */
        if(nextPrio < currPrio ||
           (pCurrentTable->pReadyList[currPrio]->pHead != NULL &&
           (pThread->requestSchedule == true || kForceSwitch == true)))
        {
            /* Put back the thread in the list, the highest priority will be
             * updated by the next _electNextThreadFromTable.
             * We do not have to manage the highest priority because we know
             * it is currently equal or higher.
             */
            KERNEL_UNLOCK(pCurrentTable->lock);

            _schedReleaseThread(pThread, true);

            KERNEL_UNLOCK(pThread->lock);

            /* Elect the new thread*/
            pCurrentThreadsPtr[cpuId] =
                _electNextThreadFromTable(pCurrentTable);
            pThread = pCurrentThreadsPtr[cpuId];
        }
        else
        {
            KERNEL_UNLOCK(pCurrentTable->lock);
        }
    }
    else if(pThread->nextState != THREAD_STATE_READY)
    {
        SCHED_ASSERT(pThread->preemptionDisabled == false,
                     "Non running thread has preemption disabled",
                     OS_ERR_UNAUTHORIZED_ACTION);

        KERNEL_UNLOCK(pCurrentTable->lock);
        KERNEL_UNLOCK(pThread->lock);

        /* Otherwise just select a new thread */
        pCurrentThreadsPtr[cpuId] = _electNextThreadFromTable(pCurrentTable);
        pThread = pCurrentThreadsPtr[cpuId];
    }

    SCHED_ASSERT(pThread->currentState == THREAD_STATE_READY,
                 "Scheduled non ready thread",
                 OS_ERR_UNAUTHORIZED_ACTION);

    /* Update the new process and switch memory config */
    cpuUpdateMemoryConfig(pThread);

    spCurrentProcessPtr[cpuId] = pThread->pProcess;
    pThread->currentState    = THREAD_STATE_RUNNING;
    pThread->requestSchedule = false;

    signalManage(pThread);

    KERNEL_UNLOCK(pThread->lock);

    /* Update the CPU statistics */
    KERNEL_LOCK(sCpuStats[cpuId].lock);
    sCpuStats[cpuId].timesIdx = (sCpuStats[cpuId].timesIdx + 1) %
                                CPU_LOAD_TICK_WINDOW;

    sCpuStats[cpuId].totalTime -=
        sCpuStats[cpuId].totalTimes[sCpuStats[cpuId].timesIdx];
    sCpuStats[cpuId].idleTime -=
        sCpuStats[cpuId].idleTimes[sCpuStats[cpuId].timesIdx];

    if(pThread == spIdleThread[cpuId])
    {
        sCpuStats[cpuId].idleTimes[sCpuStats[cpuId].timesIdx] = upTime;
    }
    else
    {
        sCpuStats[cpuId].idleTimes[sCpuStats[cpuId].timesIdx] = 0;
    }
    sCpuStats[cpuId].totalTimes[sCpuStats[cpuId].timesIdx] = upTime;


    ++sCpuStats[cpuId].schedCount;
    KERNEL_UNLOCK(sCpuStats[cpuId].lock);

#if SCHED_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Selected thread %d for RUN on CPU %d",
           pThread->tid,
           cpuId);
#endif

    /* We should never come back */
    cpuRestoreContext(pThread);

    SCHED_ASSERT(false,
                 "Schedule returned",
                 OS_ERR_UNAUTHORIZED_ACTION);
}

void schedSchedule(void)
{
    /* Request schedule */
    schedGetCurrentThread()->requestSchedule = true;

    /* Just generate a scheduler interrupt */
    cpuRaiseInterrupt(sSchedulerInterruptLine);
}

OS_RETURN_E schedSleep(const uint64_t kTimeNs)
{
    kernel_thread_t* pCurrThread;
    uint32_t         intState;

    /* Check the current thread */
    pCurrThread = schedGetCurrentThread();
    if(pCurrThread == spIdleThread[pCurrThread->schedCpu])
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pCurrThread->lock);
    /* Get the current time and set as sleeping*/
    pCurrThread->wakeupTime = timeGetUptime() + kTimeNs;
    pCurrThread->nextState = THREAD_STATE_SLEEPING;
    KERNEL_UNLOCK(pCurrThread->lock);

    /* Request scheduling */
    schedSchedule();
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    /* On return, check that the wakeup time is readched */
    if(pCurrThread->wakeupTime > timeGetUptime())
    {
        return OS_ERR_CANCELED;
    }

    return OS_NO_ERR;
}

size_t schedGetThreadCount(void)
{
    return sThreadCount;
}

kernel_thread_t* schedGetCurrentThread(void)
{
    kernel_thread_t* pCur;
    uint32_t         intState;

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    pCur = pCurrentThreadsPtr[cpuGetId()];
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    return pCur;
}

OS_RETURN_E schedCreateKernelThread(kernel_thread_t** ppThread,
                                    const uint8_t     kPriority,
                                    const char*       kpName,
                                    const size_t      kStackSize,
                                    const uint64_t    kAffinitySet,
                                    void*             (*pRoutine)(void*),
                                    void*             args)
{
    kernel_thread_t* pNewThread;
    OS_RETURN_E      error;
    int32_t          newTid;

    pNewThread      = NULL;
    error           = OS_NO_ERR;

    /* Validate parameters */
    if((kAffinitySet >> SOC_CPU_COUNT) != 0 ||
        kPriority > KERNEL_LOWEST_PRIORITY ||
        ppThread == NULL ||
        pRoutine == NULL ||
        kpName   == NULL ||
        kStackSize == 0)
    {
        error = OS_ERR_INCORRECT_VALUE;
        goto SCHED_CREATE_KTHREAD_END;
    }

    newTid = atomicIncrement32(&sLastGivenTid);
    if(newTid >= INT32_MAX || newTid == 0)
    {
        atomicDecrement32(&sLastGivenTid);
        return OS_ERR_NO_MORE_MEMORY;
    }

    /* Allocate new structure */
    pNewThread = kmalloc(sizeof(kernel_thread_t));
    if(pNewThread == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }
    memset(pNewThread, 0, sizeof(kernel_thread_t));

    /* Create a new node for the threads lists */
    pNewThread->pThreadNode = kQueueCreateNode(pNewThread, false);
    if(pNewThread->pThreadNode == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }

    /* Set the thread's information */
    pNewThread->affinity           = kAffinitySet;
    pNewThread->tid                = newTid;
    pNewThread->type               = THREAD_TYPE_KERNEL;
    pNewThread->priority           = kPriority;
    pNewThread->pArgs              = args;
    pNewThread->pEntryPoint        = pRoutine;
    pNewThread->requestSchedule    = true;
    pNewThread->preemptionDisabled = false;

    strncpy(pNewThread->pName, kpName, THREAD_NAME_MAX_LENGTH);
    pNewThread->pName[THREAD_NAME_MAX_LENGTH] = 0;

    /* Set the thread's stack for both interrupt and main */
    pNewThread->kernelStackEnd = memoryKernelMapStack(kStackSize);
    if(pNewThread->kernelStackEnd == (uintptr_t)NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }
    pNewThread->kernelStackSize = kStackSize;

    /* No user stack for kernel threads */
    pNewThread->stackEnd  = (uintptr_t)NULL;
    pNewThread->stackSize = 0;

    /* Allocate the vCPUs */
    pNewThread->pThreadVCpu =
        (void*)cpuCreateVirtualCPU(_threadEntryPoint,
                                   pNewThread->kernelStackEnd);
    if(pNewThread->pThreadVCpu == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }
    pNewThread->pSignalVCpu =
        (void*)cpuCreateVirtualCPU(NULL,
                                   pNewThread->kernelStackEnd);
    if(pNewThread->pSignalVCpu == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }

    /* Set the current vCPU as the regular thread vCPU */
    pNewThread->pVCpu = pNewThread->pThreadVCpu;

    /* Set thread to READY */
    pNewThread->currentState = THREAD_STATE_READY;
    pNewThread->nextState    = THREAD_STATE_READY;

    /* Set the thread's resources */
    pNewThread->pThreadResources = kQueueCreate(false);
    if(pNewThread->pThreadResources == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }

    /* Initialize the lock */
    KERNEL_SPINLOCK_INIT(pNewThread->lock);

    /* Init signal */
    signalInitSignals(pNewThread);

     /* Setup the process link */
    pNewThread->pProcess = schedGetCurrentProcess();
    pNewThread->pNext = NULL;

    KERNEL_LOCK(pNewThread->pProcess->lock);

    KERNEL_LOCK(pNewThread->pProcess->pThreadListTail->lock);
    pNewThread->pProcess->pThreadListTail->pNext = pNewThread;
    KERNEL_UNLOCK(pNewThread->pProcess->pThreadListTail->lock);

    pNewThread->pPrev = pNewThread->pProcess->pThreadListTail;
    pNewThread->pProcess->pThreadListTail = pNewThread;
    error = uhashtableSet(pNewThread->pProcess->pThreadTable,
                          (uintptr_t)pNewThread,
                          NULL);
    KERNEL_UNLOCK(pNewThread->pProcess->lock);
    if(error != OS_NO_ERR)
    {
        goto SCHED_CREATE_KTHREAD_END;
    }


    /* Add to the global list */
    KERNEL_LOCK(sTotalThreadsList.lock);
    pNewThread->pThreadListNode = kQueueCreateNode(pNewThread, false);
    if(pNewThread->pThreadListNode == NULL)
    {
        KERNEL_UNLOCK(sTotalThreadsList.lock);
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }
    kQueuePush(pNewThread->pThreadListNode, sTotalThreadsList.pThreadList);
    ++sTotalThreadsList.threadCount;
    KERNEL_UNLOCK(sTotalThreadsList.lock);

    /* Increase the global number of threads */
    atomicIncrement32(&sThreadCount);

    /* Release the thread */
    _schedReleaseThread(pNewThread, false);

SCHED_CREATE_KTHREAD_END:
    if(error != OS_NO_ERR)
    {
        if(pNewThread != NULL)
        {
            if(pNewThread->stackEnd != (uintptr_t)NULL)
            {
                /* TODO: */
            }
            if(pNewThread->kernelStackEnd != (uintptr_t)NULL)
            {

                memoryKernelUnmapStack(pNewThread->kernelStackEnd,
                                      pNewThread->kernelStackSize);

            }
            if(pNewThread->pThreadVCpu != NULL)
            {
                cpuDestroyVirtualCPU((uintptr_t)pNewThread->pThreadVCpu);
            }
            if(pNewThread->pSignalVCpu != NULL)
            {
                cpuDestroyVirtualCPU((uintptr_t)pNewThread->pSignalVCpu);
            }
            if(pNewThread->pThreadResources != NULL)
            {
                kQueueDestroy((kqueue_t**)&pNewThread->pThreadResources);
            }
            if(pNewThread->pThreadListNode != NULL)
            {
                KERNEL_LOCK(sTotalThreadsList.lock);
                kQueueRemove(sTotalThreadsList.pThreadList,
                             pNewThread->pThreadListNode,
                             true);
                kQueueDestroyNode((kqueue_node_t**)
                                  &pNewThread->pThreadListNode);
                KERNEL_UNLOCK(sTotalThreadsList.lock);
            }
            if(pNewThread->pThreadNode != NULL)
            {
                kQueueDestroyNode((kqueue_node_t**)&pNewThread->pThreadNode);
            }

            if(pNewThread->pProcess != NULL)
            {
                KERNEL_LOCK(pNewThread->pProcess->lock);

                if(pNewThread->pProcess->pThreadListTail == pNewThread)
                {
                    pNewThread->pProcess->pThreadListTail = pNewThread->pPrev;
                }

                if(pNewThread->pNext != NULL)
                {
                    KERNEL_LOCK(pNewThread->pNext->lock);
                    pNewThread->pNext->pPrev = pNewThread->pPrev;
                    KERNEL_UNLOCK(pNewThread->pNext->lock);
                }

                if(pNewThread->pPrev != NULL)
                {
                    KERNEL_LOCK(pNewThread->pPrev->lock);
                    pNewThread->pPrev->pNext = pNewThread->pNext;
                    KERNEL_UNLOCK(pNewThread->pPrev->lock);
                }

                pNewThread->pPrev = pNewThread->pProcess->pThreadListTail;
                pNewThread->pProcess->pThreadListTail = pNewThread;
                uhashtableRemove(pNewThread->pProcess->pThreadTable,
                                 (uintptr_t)pNewThread,
                                 NULL);

                KERNEL_UNLOCK(pNewThread->pProcess->lock);
            }

            kfree(pNewThread);
        }

        *ppThread = NULL;
    }
    else
    {
        *ppThread = pNewThread;
    }

    return error;
}

OS_RETURN_E schedJoinThread(kernel_thread_t*          pThread,
                            void**                    ppRetVal,
                            THREAD_TERMINATE_CAUSE_E* pTerminationCause)
{
    kernel_thread_t* pCurThread;
    uint8_t          cpuId;
    uint32_t         intState;

    if(pThread == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    pCurThread = schedGetCurrentThread();

    if(pCurThread == pThread)
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    /* Check if we are not joining idle */
    for(cpuId = 0; cpuId < SOC_CPU_COUNT; ++cpuId)
    {
        if(pThread == spIdleThread[cpuId] || pCurThread == spIdleThread[cpuId])
        {
            return OS_ERR_UNAUTHORIZED_ACTION;
        }
    }

    /* Check if the thread is in the same process */
    if(pCurThread->pProcess != pThread->pProcess)
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    /* Check if the thread is the process's main thread */
    if(pCurThread->pProcess->pMainThread == pThread)
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    do
    {
        KERNEL_ENTER_CRITICAL_LOCAL(intState);
        KERNEL_LOCK(pThread->lock);
        KERNEL_LOCK(pCurThread->lock);

        /* Check if the thread is still valid */
        if(schedIsThreadValid(pThread) == false)
        {
            KERNEL_UNLOCK(pCurThread->lock);
            KERNEL_UNLOCK(pThread->lock);
            KERNEL_EXIT_CRITICAL_LOCAL(intState);
            return OS_ERR_NO_SUCH_ID;
        }

        /* Check if a thread is already joining */
        if(pThread->pJoiningThread != NULL &&
           pThread->pJoiningThread != pCurThread)
        {
            KERNEL_UNLOCK(pCurThread->lock);
            KERNEL_UNLOCK(pThread->lock);
            KERNEL_EXIT_CRITICAL_LOCAL(intState);
            return OS_ERR_UNAUTHORIZED_ACTION;
        }

        /* Check if the thread is already exited */
        if(pThread->currentState == THREAD_STATE_ZOMBIE)
        {
            if(ppRetVal != NULL)
            {
                *ppRetVal = pThread->retValue;
            }
            if(pTerminationCause != NULL)
            {
                *pTerminationCause = pThread->terminateCause;
            }
            KERNEL_UNLOCK(pCurThread->lock);
            KERNEL_EXIT_CRITICAL_LOCAL(intState);

            /* Clean the thread */
            _schedCleanThread(pThread);

            return OS_NO_ERR;
        }

        /* The thread is no yet exited, add to the joining thread and lock the
         * caller thread.
         */

        pThread->pJoiningThread   = pCurThread;
        pCurThread->pJoinedThread = pThread;
        pCurThread->nextState     = THREAD_STATE_JOINING;

        KERNEL_UNLOCK(pCurThread->lock);
        KERNEL_UNLOCK(pThread->lock);

        /* Request scheduling */
        schedSchedule();

        /* Retry to join after wake up */
    } while(true);

    return OS_NO_ERR;
}

uint64_t schedGetCpuLoad(const uint8_t kCpuId)
{
    uint64_t cpuLoad;

    if(kCpuId < SOC_CPU_COUNT)
    {
        KERNEL_LOCK(sCpuStats[kCpuId].lock);
        if(sCpuStats[kCpuId].totalTime != 0)
        {
            cpuLoad = 100 -
                      (100 * sCpuStats[kCpuId].idleTime /
                       sCpuStats[kCpuId].totalTime);
        }
        else
        {
            cpuLoad = 0;
        }
        KERNEL_UNLOCK(sCpuStats[kCpuId].lock);

        return cpuLoad;
    }
    else
    {
        return 0xFFFFFFFFFFFFFFFF;
    }
}

OS_RETURN_E schedUpdatePriority(kernel_thread_t* pThread, const uint8_t kPrio)
{
    uint8_t cpuId;

    /* Check parameters */
    if(kPrio > KERNEL_LOWEST_PRIORITY)
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

    if(kPrio == pThread->priority)
    {
        return OS_NO_ERR;
        KERNEL_UNLOCK(pThread->lock);
    }

    cpuId = pThread->schedCpu;
    KERNEL_LOCK(sThreadTables[cpuId].lock);
    /* Check if the thread is running, if it is not, we have to update the
     * belonging table when ready
     */
    if(((kqueue_node_t*)pThread->pThreadNode)->pQueuePtr ==
       sThreadTables[cpuId].pReadyList[pThread->priority])
    {

        /* Update the table */
        kQueueRemove(sThreadTables[cpuId].pReadyList[pThread->priority],
                     pThread->pThreadNode,
                     true);

        kQueuePush(pThread->pThreadNode,
                   sThreadTables[cpuId].pReadyList[kPrio]);

        /* Update the highest priority */
        if(sThreadTables[cpuId].highestPriority > kPrio)
        {
            sThreadTables[cpuId].highestPriority = kPrio;
        }
    }
    KERNEL_UNLOCK(sThreadTables[cpuId].lock);

    pThread->priority = kPrio;
    KERNEL_UNLOCK(pThread->lock);

    return OS_NO_ERR;
}

OS_RETURN_E schedTerminateThread(kernel_thread_t*               pThread,
                                 const THREAD_TERMINATE_CAUSE_E kCause)
{
    /* TODO Redo this function is not ok concurrecncy*/
    uint8_t     cpuId;
    OS_RETURN_E error;
    uint32_t    intState;

    if(pThread == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Cannot terminate idle thread */
    for(cpuId = 0; cpuId < SOC_CPU_COUNT; ++cpuId)
    {
        if(pThread == spIdleThread[cpuId])
        {
            return OS_ERR_UNAUTHORIZED_ACTION;
        }
    }

    /* Nothing to do if zombie */
    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pThread->lock);

    /* Check if the thread is still valid */
    if(schedIsThreadValid(pThread) == false)
    {
        KERNEL_UNLOCK(pThread->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        return OS_ERR_NO_SUCH_ID;
    }

    if(pThread->currentState == THREAD_STATE_ZOMBIE)
    {
        KERNEL_UNLOCK(pThread->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        return OS_ERR_NO_SUCH_ID;
    }

    /* If it is running */
    if(schedGetCurrentThread() == pThread)
    {
        KERNEL_UNLOCK(pThread->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);

        /* If we are terminating ourselves just go to the exit point */
        _threadExitPoint(kCause, THREAD_RETURN_STATE_KILLED, NULL);
        SCHED_ASSERT(false,
                     "Exit point returned on terminate",
                     OS_ERR_UNAUTHORIZED_ACTION);
    }

    /* Different thread, directly set as terminating */
    pThread->terminateCause = kCause;
    KERNEL_UNLOCK(pThread->lock);
    error = signalThread(pThread, THREAD_SIGNAL_KILL);

    if(error == OS_NO_ERR)
    {
        /* Set the thread to the highest priority */
        error = schedUpdatePriority(pThread, KERNEL_HIGHEST_PRIORITY);
        SCHED_ASSERT(error == OS_NO_ERR,
                     "Failed to change thread priority",
                     error);
    }

    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    return error;
}

void schedThreadExit(const THREAD_TERMINATE_CAUSE_E kCause,
                     const THREAD_RETURN_STATE_E    kRetState,
                     void*                          pRetVal)
{
    _threadExitPoint(kCause, kRetState, pRetVal);
}

size_t schedGetThreadsIds(int32_t* pThreadTable, const size_t kTableSize)
{
    size_t           i;
    kqueue_node_t*   pNode;
    kernel_thread_t* pThread;

    if(pThreadTable == NULL)
    {
        return 0;
    }

    KERNEL_LOCK(sTotalThreadsList.lock);
    pNode = sTotalThreadsList.pThreadList->pHead;
    for(i = 0; i < kTableSize && pNode != NULL; ++i)
    {
        pThread = (kernel_thread_t*)pNode->pData;
        pThreadTable[i] = pThread->tid;
        pNode = pNode->pNext;
    }
    KERNEL_UNLOCK(sTotalThreadsList.lock);

    return i;
}

OS_RETURN_E schedGetThreadInfo(thread_info_t* pInfo, const int32_t kTid)
{
    kqueue_node_t*   pNode;
    kernel_thread_t* pThread;

    if(pInfo == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    KERNEL_LOCK(sTotalThreadsList.lock);
    pNode = sTotalThreadsList.pThreadList->pHead;
    while(pNode != NULL)
    {
        pThread = (kernel_thread_t*)pNode->pData;
        if(pThread->tid == kTid)
        {
            pInfo->pid          = pThread->pProcess->pid;
            pInfo->tid          = pThread->tid;
            pInfo->type         = pThread->type;
            pInfo->priority     = pThread->priority;
            pInfo->currentState = pThread->currentState;
            pInfo->affinity     = pThread->affinity;
            pInfo->schedCpu     = pThread->schedCpu;
            memcpy(pInfo->pName, pThread->pName, THREAD_NAME_MAX_LENGTH);

            KERNEL_UNLOCK(sTotalThreadsList.lock);
            return OS_NO_ERR;
        }
        pNode = pNode->pNext;
    }
    KERNEL_UNLOCK(sTotalThreadsList.lock);

    return OS_ERR_NO_SUCH_ID;
}

void schedDisablePreemption(void)
{
    kernel_thread_t* pThread;

    pThread = schedGetCurrentThread();
    KERNEL_LOCK(pThread->lock);

    pThread->preemptionDisabled = true;

    KERNEL_UNLOCK(pThread->lock);
}

void schedEnablePreemption(void)
{
    kernel_thread_t* pThread;

    pThread = schedGetCurrentThread();
    KERNEL_LOCK(pThread->lock);

    pThread->preemptionDisabled = false;

    KERNEL_UNLOCK(pThread->lock);
}

kernel_process_t* schedGetCurrentProcess(void)
{
    kernel_process_t* pCur;
    uint32_t          intState;

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    pCur = spCurrentProcessPtr[cpuGetId()];
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    return pCur;
}

OS_RETURN_E schedSetThreadToReady(kernel_thread_t* pThread)
{
    OS_RETURN_E retCode;

    KERNEL_LOCK(pThread->lock);

    /* Check if the thread is still valid */
    if(schedIsThreadValid(pThread) == false)
    {
        KERNEL_UNLOCK(pThread->lock);
        return OS_ERR_NO_SUCH_ID;
    }

    if(pThread->currentState != THREAD_STATE_ZOMBIE)
    {
        if(pThread->currentState == THREAD_STATE_SLEEPING ||
           pThread->nextState == THREAD_STATE_SLEEPING)
        {
            _cancelSleepThread(pThread);
            retCode = OS_NO_ERR;
        }
        else if(pThread->currentState != THREAD_STATE_RUNNING)
        {
            if(pThread->nextState != THREAD_STATE_READY)
            {
                _schedReleaseThread(pThread, true);
            }
            retCode = OS_NO_ERR;
        }
        else
        {
            retCode = OS_NO_ERR;
        }
        pThread->nextState = THREAD_STATE_READY;
    }
    else
    {
        retCode = OS_ERR_NO_SUCH_ID;
    }
    KERNEL_UNLOCK(pThread->lock);

    return retCode;
}

OS_RETURN_E schedThreadSetWaiting(void)
{
    kernel_thread_t* pThread;
    OS_RETURN_E retCode;

    pThread = schedGetCurrentThread();

    KERNEL_LOCK(pThread->lock);
    if(pThread->currentState == THREAD_STATE_RUNNING)
    {
        pThread->nextState = THREAD_STATE_WAITING;
        retCode = OS_NO_ERR;
    }
    else
    {
        retCode = OS_ERR_UNAUTHORIZED_ACTION;
    }
    KERNEL_UNLOCK(pThread->lock);

    return retCode;
}

bool schedIsInit(void)
{
    return sIsInit;
}

bool schedIsRunning(void)
{
    return sIsRunning;
}

bool schedIsThreadValid(kernel_thread_t* pThread)
{
    OS_RETURN_E err;
    void*       data;

    KERNEL_LOCK(pThread->pProcess->lock);
    err = uhashtableGet(pThread->pProcess->pThreadTable,
                        (uintptr_t)pThread,
                        &data);
    KERNEL_UNLOCK(pThread->pProcess->lock);

    return (err == OS_NO_ERR);
}

OS_RETURN_E schedForkProcess(kernel_process_t** ppNewProcess)
{
    OS_RETURN_E       error;
    kernel_process_t* pProcess;
    kernel_process_t* pCurrentProcess;

    if(ppNewProcess == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Allocate the structure */
    pProcess = kmalloc(sizeof(kernel_process_t));
    if(pProcess == NULL)
    {
        return OS_ERR_NO_MORE_MEMORY;
    }
    memset(pProcess, 0, sizeof(kernel_process_t));

    /* Create process memory information TODO: */
    pProcess->pMemoryData = memoryKernelGetPageTable();
    if(pProcess->pMemoryData == NULL)
    {
        kfree(pProcess);
        return OS_ERR_NULL_POINTER;
    }

    /* Copy the mapping of the current process */

    /* Create the thread table */
    pProcess->pThreadTable = uhashtableCreate(UHASHTABLE_ALLOCATOR(kmalloc,
                                                                   kfree),
                                                &error);
    if(error != OS_NO_ERR)
    {
        kfree(pProcess);
        return error;
    }

    /* Create the futex table */
    pProcess->pFutexTable = uhashtableCreate(UHASHTABLE_ALLOCATOR(kmalloc,
                                                                  kfree),
                                               &error);
    if(error != OS_NO_ERR)
    {

        SCHED_ASSERT(uhashtableDestroy(pProcess->pThreadTable) == OS_NO_ERR,
                     "Failed to remove threads table.",
                     error);
        kfree(pProcess);
        return error;
    }

    /* Create the main thread (copy from the current thread) */

    /* Setup the process attributes */
    pCurrentProcess = schedGetCurrentProcess();

    pProcess->pid = atomicIncrement32(&sLastGivenPid);
    memcpy(pProcess->pName, pCurrentProcess->pName, PROCESS_NAME_MAX_LENGTH);
    pProcess->pName[PROCESS_NAME_MAX_LENGTH] = 0;

    KERNEL_SPINLOCK_INIT(pProcess->futexTableLock);
    KERNEL_SPINLOCK_INIT(pProcess->lock);

    pProcess->pParent = pCurrentProcess;
    /* TODO: Add to parent's children */

    /* Release main thread */


    *ppNewProcess = pProcess;

    atomicIncrement32(&sProcessCount);
    return OS_NO_ERR;
}
/************************************ EOF *************************************/