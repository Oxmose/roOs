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

/** @brief Defines the thread sleeping table structure */
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

/** @brief Defines the thread information structure */
typedef struct
{
    /** @brief Thread's identifier. */
    int32_t tid;

    /** @brief Thread's name. */
    char* pName;

    /** @brief Thread's type. */
    THREAD_TYPE_E type;

    /** @brief Thread's current priority. */
    uint8_t* pPriority;

    /** @brief Thread's current state. */
    THREAD_STATE_E* pCurrentState;

    /** @brief Thread's CPU affinity */
    uint64_t* pAffinity;

    /** @brief Thread's currently mapped CPU */
    uint8_t* pSchedCpu;
} internal_thread_info_t;

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
    if((COND) == FALSE)                                     \
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

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/**
 * @brief Pointers to the current kernel thread. One per CPU.
 * Used in assembly code
 */
kernel_thread_t* pCurrentThreadsPtr[SOC_CPU_COUNT] = {NULL};

/************************** Static global variables ***************************/
/** @brief The last TID given by the kernel. */
static u32_atomic_t sLastGivenTid;

/** @brief The number of thread in the system. */
static u32_atomic_t sThreadCount;

/** @brief Count of the number of times the scheduler was called per CPU. */
static uint64_t sScheduleCount[SOC_CPU_COUNT];

/** @brief Stores the thread tables for all CPUs */
static thread_table_t sThreadTables[SOC_CPU_COUNT];

/** @brief Stores the list of sleeping threads */
static thread_general_table_t sSleepingThreadsTable[SOC_CPU_COUNT];

/** @brief Stores the list of zombie threads */
static thread_general_table_t sZombieThreadsTable;

/** @brief Pointers to the idle threads allocated per CPU. */
static kernel_thread_t* spIdleThread[SOC_CPU_COUNT];

/** @brief The number of time IDLE was scheduled during the last window. */
static uint64_t sIdleSchedCount[SOC_CPU_COUNT];

/** @brief Stores the thread's information table */
static kqueue_t* spThreadList;

/** @brief Thread information table lock */
static kernel_spinlock_t sThreadListLock;

/** @brief Stores the calculated CPU load for the last window */
static uint64_t sCpuLoad[SOC_CPU_COUNT];

/** @brief Stores the scheduler interrupt line */
static uint32_t sSchedulerInterruptLine;

/** @brief Tells if the scheduler was forced at least once */
static bool_t sFirstSched[SOC_CPU_COUNT] = {FALSE};

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

    while(TRUE)
    {
        interruptRestore(1);
        cpuHalt();
    }

    /* If we return better go away and cry in a corner */
    SCHED_ASSERT(FALSE, "IDLE returned", OS_ERR_UNAUTHORIZED_ACTION);
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
    pCurThread->nextState    = THREAD_STATE_ZOMBIE;
    pCurThread->currentState = THREAD_STATE_ZOMBIE;

    KERNEL_LOCK(sZombieThreadsTable.lock);
    kQueuePush(pCurThread->pThreadNode, sZombieThreadsTable.pThreadList);
    ++sZombieThreadsTable.threadCount;
    KERNEL_UNLOCK(sZombieThreadsTable.lock);

    /* Set the thread's stats and state */
    pCurThread->endTime        = timeGetUptime();
    pCurThread->retValue       = pRetVal;
    pCurThread->terminateCause = kCause;
    pCurThread->returnState    = kRetState;

    /* This thread's parent will inherit its children TODO: */


    /* Search for joining thread */
    if(pCurThread->pJoiningThread != NULL)
    {
        pJoiningThread = pCurThread->pJoiningThread;
        KERNEL_LOCK(pCurThread->pJoiningThread->lock);

        /* Release the joining thread */
        schedReleaseThread(pJoiningThread, TRUE, THREAD_STATE_READY, FALSE);

        KERNEL_UNLOCK(pCurThread->pJoiningThread->lock);
    }

    KERNEL_UNLOCK(pCurThread->lock);

    /* Schedule thread, no need for interrupt, the context does not need to be
     * saved.
     */
    schedScheduleNoInt(TRUE);

    KERNEL_EXIT_CRITICAL_LOCAL(intState);
    /* We should never return */
    SCHED_ASSERT(FALSE,
                 "Thread retuned after exiting",
                 OS_ERR_UNAUTHORIZED_ACTION);
}

static void _createIdleThreads(void)
{
    uint32_t                i;
    kqueue_node_t*          pNewNode;
    internal_thread_info_t* pInfoNode;

    for(i = 0; i < SOC_CPU_COUNT; ++i)
    {
        /* Allocate new structure */
        spIdleThread[i] = kmalloc(sizeof(kernel_thread_t));
        SCHED_ASSERT(spIdleThread[i] != NULL,
                     "Failed to allocate IDLE thread",
                     OS_ERR_NO_MORE_MEMORY);

        /* Create a new node for the threads lists */
        pNewNode = kQueueCreateNode(spIdleThread[i], TRUE);

        /* Set the thread's information */
        spIdleThread[i]->affinity           = (1ULL << i);
        spIdleThread[i]->schedCpu           = i;
        spIdleThread[i]->tid                = atomicIncrement32(&sLastGivenTid);
        spIdleThread[i]->type               = THREAD_TYPE_KERNEL;
        spIdleThread[i]->priority           = KERNEL_LOWEST_PRIORITY;
        spIdleThread[i]->pArgs              = (void*)(uintptr_t)i;
        spIdleThread[i]->pEntryPoint        = _idleRoutine;
        spIdleThread[i]->pParentThread      = NULL;
        spIdleThread[i]->requestSchedule    = TRUE;
        spIdleThread[i]->preemptionDisabled = FALSE;

        memcpy(spIdleThread[i]->pName, "idle", 5);

        /* Init lock */
        KERNEL_SPINLOCK_INIT(spIdleThread[i]->lock);

        /* Set the thread's resources */
        spIdleThread[i]->pThreadResources = kQueueCreate(TRUE);

        /* Set the thread's stack for both interrupt and main */
        spIdleThread[i]->stackEnd = cpuCreateKernelStack(KERNEL_STACK_SIZE);
        SCHED_ASSERT(spIdleThread[i]->stackEnd != (uintptr_t)NULL,
                     "Failed to allocate IDLE thread stack",
                     OS_ERR_NO_MORE_MEMORY);

        spIdleThread[i]->stackSize       = KERNEL_STACK_SIZE;
        spIdleThread[i]->kernelStackEnd  = spIdleThread[i]->stackEnd;
        spIdleThread[i]->kernelStackSize = KERNEL_STACK_SIZE;

        /* Allocate the vCPU */
        spIdleThread[i]->pThreadVCpu =
                                  (void*)cpuCreateVirtualCPU(_threadEntryPoint,
                                                             spIdleThread[i]);
        SCHED_ASSERT(spIdleThread[i]->pThreadVCpu != NULL,
                     "Failed to allocate IDLE thread VCPU",
                     OS_ERR_NO_MORE_MEMORY);
        spIdleThread[i]->pSignalVCpu =
                                  (void*)cpuCreateVirtualCPU(NULL,
                                                             spIdleThread[i]);
        SCHED_ASSERT(spIdleThread[i]->pSignalVCpu != NULL,
                     "Failed to allocate IDLE thread signal VCPU",
                     OS_ERR_NO_MORE_MEMORY);

        /* Set the current vCPU as the regular thread vCPU */
        spIdleThread[i]->pVCpu = spIdleThread[i]->pThreadVCpu;

        /* Set idle to READY */
        spIdleThread[i]->currentState  = THREAD_STATE_RUNNING;
        spIdleThread[i]->nextState     = THREAD_STATE_READY;

        /* Set the information node */
        pInfoNode = kmalloc(sizeof(internal_thread_info_t));
        SCHED_ASSERT(pInfoNode != NULL,
                     "Failed to allocated IDLE info node",
                     OS_ERR_NO_MORE_MEMORY);
        pInfoNode->tid           = spIdleThread[i]->tid;
        pInfoNode->pName         = spIdleThread[i]->pName;
        pInfoNode->type          = spIdleThread[i]->type;
        pInfoNode->pPriority     = &spIdleThread[i]->priority;
        pInfoNode->pCurrentState = &spIdleThread[i]->currentState;
        pInfoNode->pAffinity     = &spIdleThread[i]->affinity;
        pInfoNode->pSchedCpu     = &spIdleThread[i]->schedCpu;
        spIdleThread[i]->pInfoNode = kQueueCreateNode(pInfoNode, TRUE);
        kQueuePush(spIdleThread[i]->pInfoNode, spThreadList);

        /* Set idle node */
        spIdleThread[i]->pThreadNode = pNewNode;

        /* Init signal */
        signalInitSignals(spIdleThread[i]);

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

    /* Lock the thread */
    KERNEL_UNLOCK(pTable->lock);
    KERNEL_LOCK(((kernel_thread_t*)pThreadNode->pData)->lock);

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
            schedReleaseThread(pThreadNode->pData,
                               FALSE,
                               THREAD_STATE_READY,
                               FALSE);
        }
    }
    KERNEL_UNLOCK(sSleepingThreadsTable[cpuId].lock);
}

static void _schedCleanThread(kernel_thread_t* pThread)
{
    kqueue_node_t*     pRes;
    kqueue_t*          pResQueue;
    thread_resource_t* pResourceInfo;

    SCHED_ASSERT(pThread != schedGetCurrentThread(),
                 "Thread cannot clean themselves!",
                 OS_ERR_UNAUTHORIZED_ACTION);

    KERNEL_LOCK(pThread->lock);

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
    cpuDestroyKernelStack(pThread->kernelStackEnd,
                          pThread->kernelStackSize);

    /* TODO: Destroy the regular stack */

    /* Destroy the virutal CPUs */
    cpuDestroyVirtualCPU((uintptr_t)pThread->pThreadVCpu);
    cpuDestroyVirtualCPU((uintptr_t)pThread->pSignalVCpu);

    /* Clear the active thread node, it should not be in any queue */
    kQueueDestroyNode((kqueue_node_t**)&pThread->pThreadNode);

    /* Release the thread info node */
    kQueueRemove(spThreadList, (kqueue_node_t*)pThread->pInfoNode, TRUE);
    kfree(((kqueue_node_t*)(pThread->pInfoNode))->pData);
    kQueueDestroyNode((kqueue_node_t**)&pThread->pInfoNode);

    /* Decrement thread count */
    atomicDecrement32(&sThreadCount);

    KERNEL_UNLOCK(pThread->lock);

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

    sched_vfs_entry_t*      pEntry;
    int32_t                 tid;
    kqueue_node_t*          pNode;
    internal_thread_info_t* pInfo;
    char*                   pStr;

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
        KERNEL_LOCK(sThreadListLock);
        pNode = spThreadList->pHead;
        while(pNode != NULL)
        {
            pInfo = (internal_thread_info_t*)pNode->pData;
            if(pInfo->tid == tid)
            {
                pEntry->tid = tid;
                break;
            }
            pNode = pNode->pNext;
        }
        KERNEL_UNLOCK(sThreadListLock);

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
    internal_thread_info_t* pInfo;
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
    KERNEL_LOCK(sThreadListLock);
    pNode = spThreadList->pTail;

    /* Got to the offset node */
    while(pNode != NULL)
    {
        pInfo = pNode->pData;
        if(pInfo->tid == pEntry->tid)
        {
            break;
        }
        pNode = pNode->pPrev;
    }

    /* We reached the end of threads, return -1 */
    if(pNode == NULL)
    {
        KERNEL_UNLOCK(sThreadListLock);
        return -1;
    }

    /* Format */
    switch(pInfo->type)
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
    switch(*pInfo->pCurrentState)
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
                          "Id: %d\n"
                          "Name: %s\n"
                          "Priority: %d\n"
                          "Type: %s\n"
                          "State: %s\n"
                          "Affinity: 0x%lx\n"
                          "CPU: %d",
                          pInfo->tid,
                          pInfo->pName,
                          *pInfo->pPriority,
                          pType,
                          pState,
                          *pInfo->pAffinity,
                          *pInfo->pSchedCpu);
    KERNEL_UNLOCK(sThreadListLock);
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
    internal_thread_info_t* pInfo;

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

    KERNEL_LOCK(sThreadListLock);
    pNode = spThreadList->pTail;

    /* Got to the offset node */
    for(i = 0; i < pEntry->offset && pNode != NULL; ++i)
    {
        pNode = pNode->pPrev;
    }

    /* We reached the end of threads, return -1 */
    if(pNode == NULL)
    {
        KERNEL_UNLOCK(sThreadListLock);
        return -1;
    }


    /* Update the offset */
    ++pEntry->offset;

    /* Otherwise, set the entry */
    pInfo = pNode->pData;
    snprintf(pDirEntry->pName, VFS_FILENAME_MAX_LENGTH, "%d", pInfo->tid);

    pDirEntry->type = VFS_FILE_TYPE_FILE;

    if(pNode->pPrev == NULL)
    {
        KERNEL_UNLOCK(sThreadListLock);
        return 0;
    }
    else
    {
        KERNEL_UNLOCK(sThreadListLock);
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
    schedScheduleNoInt(FALSE);
}

void schedInit(void)
{
    uint32_t    i;
    uint32_t    j;
    OS_RETURN_E error;

    /* Init values */
    sLastGivenTid = 0;
    sThreadCount  = 0;
    memset(pCurrentThreadsPtr, 0, sizeof(kernel_thread_t*) * SOC_CPU_COUNT);
    memset(sScheduleCount, 0, sizeof(uint64_t) * SOC_CPU_COUNT);
    memset(sIdleSchedCount, 0, sizeof(uint64_t) * SOC_CPU_COUNT);

    /* Initialize the thread table */
    for(j = 0; j < SOC_CPU_COUNT; ++j)
    {
        sThreadTables[j].highestPriority = KERNEL_LOWEST_PRIORITY;
        sThreadTables[j].threadCount     = 0;
        KERNEL_SPINLOCK_INIT(sThreadTables[j].lock);
        for(i = 0; i <= KERNEL_LOWEST_PRIORITY; ++i)
        {
            sThreadTables[j].pReadyList[i]  = kQueueCreate(TRUE);
        }
        /* Initialize the sleeping threads list */
        sSleepingThreadsTable[j].threadCount = 0;
        sSleepingThreadsTable[j].pThreadList = kQueueCreate(TRUE);
        KERNEL_SPINLOCK_INIT(sSleepingThreadsTable[j].lock);
    }

    /* Initialize the zombie threads list */
    sZombieThreadsTable.threadCount = 0;
    sZombieThreadsTable.pThreadList = kQueueCreate(TRUE);
    KERNEL_SPINLOCK_INIT(sZombieThreadsTable.lock);

    /* Initialize the thread information table */
    spThreadList = kQueueCreate(TRUE);
    KERNEL_SPINLOCK_INIT(sThreadListLock);

    /* Create the idle process for each CPU */
    _createIdleThreads();

    /* Set idle as current thread for all CPUs */
    for(i = 0; i < SOC_CPU_COUNT; ++i)
    {
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

#if SCHED_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Scheduler initialization end");
#endif
}

void schedScheduleNoInt(const bool_t kForceSwitch)
{
    uint8_t          cpuId;
    uint8_t          currPrio;
    uint8_t          nextPrio;
    thread_table_t*  pCurrentTable;
    kernel_thread_t* pThread;

    SCHED_ASSERT(cpuGetIntState() == 0,
                 "Called scheduler no int with interrupt enabled",
                 OS_ERR_UNAUTHORIZED_ACTION);


    /* Wakeup sleeping threads if needed */
    _updateSleepingThreads();

    /* Get current CPU ID */
    cpuId = cpuGetId();
    sFirstSched[cpuId] = TRUE;

    pThread = pCurrentThreadsPtr[cpuId];
    KERNEL_LOCK(pThread->lock);
    pCurrentTable = &sThreadTables[cpuId];
    KERNEL_LOCK(pCurrentTable->lock);

    /* If the current process can still run */
    if(pThread->currentState == THREAD_STATE_RUNNING &&
       pThread->preemptionDisabled == FALSE)
    {
        nextPrio = pCurrentTable->highestPriority;
        currPrio = pThread->priority;

        /* But there might be a higher prio or another thread with same
         * priority and schedule was requested.
         */
        if(nextPrio < currPrio ||
           (pCurrentTable->pReadyList[currPrio]->pHead != NULL &&
           (pThread->requestSchedule == TRUE || kForceSwitch == TRUE)))
        {
            /* Put back the thread in the list, the highest priority will be
             * updated by the next _electNextThreadFromTable.
             * We do not have to manage the highest priority because we know
             * it is currently equal or higher.
             */
            KERNEL_UNLOCK(pCurrentTable->lock);

            schedReleaseThread(pThread, TRUE, THREAD_STATE_READY, FALSE);

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
    else if(pThread->currentState != THREAD_STATE_RUNNING)
    {
        SCHED_ASSERT(pThread->preemptionDisabled == FALSE,
                     "Non running thread has preemption disabled",
                     OS_ERR_UNAUTHORIZED_ACTION);

        KERNEL_UNLOCK(pCurrentTable->lock);
        KERNEL_UNLOCK(pThread->lock);

        /* Otherwise just select a new thread */
        pCurrentThreadsPtr[cpuId] = _electNextThreadFromTable(pCurrentTable);
        pThread = pCurrentThreadsPtr[cpuId];
    }

    SCHED_ASSERT(pThread->nextState == THREAD_STATE_READY,
                 "Scheduled non ready thread",
                 OS_ERR_UNAUTHORIZED_ACTION);
    pThread->currentState    = THREAD_STATE_RUNNING;
    pThread->requestSchedule = FALSE;

    signalManage(pThread);

    KERNEL_UNLOCK(pThread->lock);

    /* Update the load */
    if(pThread == spIdleThread[cpuId] &&
       sIdleSchedCount[cpuId] < CPU_LOAD_TICK_WINDOW)
    {
        ++sIdleSchedCount[cpuId];
    }
    else if(pThread != spIdleThread[cpuId] && sIdleSchedCount[cpuId] > 0)
    {
        --sIdleSchedCount[cpuId];
    }

    sCpuLoad[cpuId] = sIdleSchedCount[cpuId];

    ++sScheduleCount[cpuId];

#if SCHED_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Selected thread %d for RUN on CPU %d (%d)",
           pThread->tid,
           cpuId,
           (uint32_t)sIdleSchedCount[cpuId]);
#endif

    /* We should never come back */
    cpuRestoreContext(pThread);

    SCHED_ASSERT(FALSE,
                 "Schedule returned",
                 OS_ERR_UNAUTHORIZED_ACTION);
}

void schedSchedule(void)
{
    /* Request schedule */
    schedGetCurrentThread()->requestSchedule = TRUE;

    /* Just generate a scheduler interrupt */
    cpuRaiseInterrupt(sSchedulerInterruptLine);
}

void schedReleaseThread(kernel_thread_t*     pThread,
                        const bool_t         kIsLocked,
                        const THREAD_STATE_E kState,
                        const bool_t         kSchedSameCpu)
{
    uint64_t     i;
    uint8_t      cpuId;
    uint64_t     lastCpuLoad;
    uint32_t     intState;
    int32_t      requestSched;
    ipi_params_t ipiParams;

    KERNEL_ENTER_CRITICAL_LOCAL(intState);

    /* Get the CPU list to release to */
    lastCpuLoad = 1000.0;
    cpuId       = SOC_CPU_COUNT;

    if(pThread->affinity != 0)
    {
        for(i = 0; i < SOC_CPU_COUNT; ++i)
        {
            if(((1ULL << i) & pThread->affinity) != 0)
            {
                if(lastCpuLoad > sCpuLoad[i])
                {
                    lastCpuLoad = sCpuLoad[i];
                    cpuId       = i;
                }
            }
        }
    }
    else
    {
        for(i = 0; i < SOC_CPU_COUNT; ++i)
        {
            if(lastCpuLoad > sCpuLoad[i])
            {
                lastCpuLoad = sCpuLoad[i];
                cpuId       = i;
            }
        }
    }

    SCHED_ASSERT(cpuId != SOC_CPU_COUNT,
                 "Failed to find a CPU to relase the thread",
                 OS_ERR_INCORRECT_VALUE);

    /* Update the thread state and push to queue */
    if(kIsLocked == FALSE)
    {
        KERNEL_LOCK(pThread->lock);
    }

    /* We do not add back zombie threads */
    if(pThread->currentState == THREAD_STATE_ZOMBIE)
    {
        if(kIsLocked == FALSE)
        {
            KERNEL_UNLOCK(pThread->lock);
        }

        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        return;
    }

    KERNEL_LOCK(sThreadTables[cpuId].lock);

    requestSched = -1;

    kQueuePush(pThread->pThreadNode,
               sThreadTables[cpuId].pReadyList[pThread->priority]);
    ++sThreadTables[cpuId].threadCount;
    if(sThreadTables[cpuId].highestPriority > pThread->priority)
    {
        sThreadTables[cpuId].highestPriority = pThread->priority;
        /* Request scheduling */
        requestSched = cpuId;
    }

    pThread->currentState = kState;
    pThread->nextState    = kState;
    pThread->schedCpu     = cpuId;

    KERNEL_UNLOCK(sThreadTables[cpuId].lock);

    if(kIsLocked == FALSE)
    {
        KERNEL_UNLOCK(pThread->lock);
    }

    if(sFirstSched[cpuId] == TRUE && requestSched != -1)
    {
        if(requestSched == cpuGetId())
        {
            if(kSchedSameCpu == TRUE)
            {
                /* Check if the context is saved */
                if(cpuIsVCPUSaved(schedGetCurrentThread()->pVCpu) == TRUE)
                {
                    schedScheduleNoInt(TRUE);
                }
                else
                {
                    schedSchedule();
                }
            }
        }
        else
        {
            /* Request schedule on other CPU */
            ipiParams.function = IPI_FUNC_SCHEDULE;
            cpuMgtSendIpi(CPU_IPI_SEND_TO(requestSched), &ipiParams, TRUE);
        }
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

OS_RETURN_E schedSleep(const uint64_t kTimeNs)
{
    kernel_thread_t* pCurrThread;
    uint32_t         intState;
    uint8_t          cpuId;

    /* Check the current thread */
    pCurrThread = schedGetCurrentThread();
    if(pCurrThread == spIdleThread[pCurrThread->schedCpu])
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }


    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pCurrThread->lock);

    cpuId = cpuGetId();

    /* Get the current time and set as sleeping*/
    pCurrThread->wakeupTime = timeGetUptime() + kTimeNs;

    KERNEL_LOCK(sSleepingThreadsTable[cpuId].lock);

    /* Put the thread in the sleeping queue */
    pCurrThread->currentState = THREAD_STATE_SLEEPING;
    kQueuePushPrio(pCurrThread->pThreadNode,
                   sSleepingThreadsTable[cpuId].pThreadList,
                   pCurrThread->wakeupTime);
    ++sSleepingThreadsTable[cpuId].threadCount;

    KERNEL_UNLOCK(sSleepingThreadsTable[cpuId].lock);
    KERNEL_UNLOCK(pCurrThread->lock);

    /* Request scheduling */
    schedSchedule();
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

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
    internal_thread_info_t* pThreadInfo;
    kernel_thread_t*        pNewThread;
    kqueue_node_t*          pNewNode;
    OS_RETURN_E             error;

    pNewThread   = NULL;
    pNewNode     = NULL;
    pThreadInfo  = NULL;
    error        = OS_NO_ERR;

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

    /* Allocate new structure */
    pNewThread = kmalloc(sizeof(kernel_thread_t));
    if(pNewThread == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }
    memset(pNewThread, 0, sizeof(kernel_thread_t));

    /* Create a new node for the threads lists */
    pNewNode = kQueueCreateNode(pNewThread, FALSE);
    if(pNewNode == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }

    /* Set the thread's information */
    pNewThread->affinity           = kAffinitySet;
    pNewThread->tid                = atomicIncrement32(&sLastGivenTid);
    pNewThread->type               = THREAD_TYPE_KERNEL;
    pNewThread->priority           = kPriority;
    pNewThread->pArgs              = args;
    pNewThread->pEntryPoint        = pRoutine;
    pNewThread->pParentThread      = schedGetCurrentThread();
    pNewThread->requestSchedule    = TRUE;
    pNewThread->preemptionDisabled = FALSE;

    strncpy(pNewThread->pName, kpName, THREAD_NAME_MAX_LENGTH);
    pNewThread->pName[THREAD_NAME_MAX_LENGTH] = 0;

    /* Set the thread's stack for both interrupt and main */
    pNewThread->stackEnd = cpuCreateKernelStack(kStackSize);
    if(pNewThread->stackEnd == (uintptr_t)NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }

    pNewThread->stackSize       = kStackSize;
    pNewThread->kernelStackEnd  = pNewThread->stackEnd;
    pNewThread->kernelStackSize = kStackSize;

    /* Allocate the vCPUs */
    pNewThread->pThreadVCpu = (void*)cpuCreateVirtualCPU(_threadEntryPoint,
                                                         pNewThread);
    if(pNewThread->pThreadVCpu == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }
    pNewThread->pSignalVCpu = (void*)cpuCreateVirtualCPU(NULL,
                                                         pNewThread);
    if(pNewThread->pSignalVCpu == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }

    /* Set the current vCPU as the regular thread vCPU */
    pNewThread->pVCpu = pNewThread->pThreadVCpu;

    /* Set thread to READY */
    pNewThread->currentState  = THREAD_STATE_READY;
    pNewThread->nextState     = THREAD_STATE_READY;

    /* Set the node node */
    pNewThread->pThreadNode = pNewNode;

    /* Set the thread's resources */
    pNewThread->pThreadResources = kQueueCreate(FALSE);
    if(pNewThread->pThreadResources == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }

    /* Initialize the lock */
    KERNEL_SPINLOCK_INIT(pNewThread->lock);

    /* Set the information node */
    pThreadInfo = kmalloc(sizeof(internal_thread_info_t));

    if(pThreadInfo == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }
    pThreadInfo->tid           = pNewThread->tid;
    pThreadInfo->pName         = pNewThread->pName;
    pThreadInfo->type          = pNewThread->type;
    pThreadInfo->pPriority     = &pNewThread->priority;
    pThreadInfo->pCurrentState = &pNewThread->currentState;
    pThreadInfo->pAffinity     = &pNewThread->affinity;
    pThreadInfo->pSchedCpu     = &pNewThread->schedCpu;

    pNewThread->pInfoNode = kQueueCreateNode(pThreadInfo, FALSE);
    if(pNewThread->pInfoNode == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }
    kQueuePush(pNewThread->pInfoNode, spThreadList);

    /* Init signal */
    signalInitSignals(pNewThread);

    /* Increase the global number of threads */
    atomicIncrement32(&sThreadCount);

    /* Release the thread */
    schedReleaseThread(pNewThread, FALSE, THREAD_STATE_READY, TRUE);

SCHED_CREATE_KTHREAD_END:
    if(error != OS_NO_ERR)
    {
        if(pNewThread != NULL)
        {
            if(pNewThread->stackEnd != (uintptr_t)NULL)
            {
                cpuDestroyKernelStack(pNewThread->kernelStackEnd,
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
            if(pNewThread->pInfoNode != NULL)
            {
                kQueueDestroyNode((kqueue_node_t**)&pNewThread->pInfoNode);
            }

            kfree(pNewThread);
        }
        if(pNewNode != NULL)
        {
            kQueueDestroyNode(&pNewNode);
        }
        if(pThreadInfo != NULL)
        {
            kfree(pThreadInfo);
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

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pThread->lock);
    KERNEL_LOCK(pCurThread->lock);

    /* Check if a thread is already joining */
    if(pThread->pJoiningThread != NULL)
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

        /* Remove thread from zombie list */
        KERNEL_LOCK(sZombieThreadsTable.lock);
        kQueueRemove(sZombieThreadsTable.pThreadList,
                     pThread->pThreadNode,
                     TRUE);
        --sZombieThreadsTable.threadCount;
        KERNEL_UNLOCK(sZombieThreadsTable.lock);
        KERNEL_UNLOCK(pCurThread->lock);
        KERNEL_UNLOCK(pThread->lock);
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
    pCurThread->currentState  = THREAD_STATE_JOINING;

    KERNEL_UNLOCK(pCurThread->lock);
    KERNEL_UNLOCK(pThread->lock);

    /* Request scheduling */
    schedSchedule();
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    pCurThread->pJoinedThread = NULL;

    /* Remove thread from zombie list */
    KERNEL_LOCK(sZombieThreadsTable.lock);
    kQueueRemove(sZombieThreadsTable.pThreadList,
                 pThread->pThreadNode,
                 TRUE);
    --sZombieThreadsTable.threadCount;

    KERNEL_UNLOCK(sZombieThreadsTable.lock);

    /* We returned from schedule, get the return value and clean the thread */
    if(ppRetVal != NULL)
    {
        *ppRetVal = pThread->retValue;
    }
    if(pTerminationCause != NULL)
    {
        *pTerminationCause = pThread->terminateCause;
    }

    _schedCleanThread(pThread);

    return OS_NO_ERR;
}

uint64_t schedGetCpuLoad(const uint8_t kCpuId)
{
    if(kCpuId < SOC_CPU_COUNT)
    {
        return sCpuLoad[kCpuId];
    }
    else
    {
        return 0xFFFFFFFFFFFFFFFF;
    }
}

void schedWaitThreadOnResource(const THREAD_WAIT_RESOURCE_TYPE_E kResource)
{
    kernel_thread_t* pThread;

    pThread = schedGetCurrentThread();

    KERNEL_LOCK(pThread->lock);

    pThread->currentState      = THREAD_STATE_WAITING;
    pThread->resourceBlockType = kResource;

    KERNEL_UNLOCK(pThread->lock);
}

void schedUpdatePriority(kernel_thread_t* pThread, const uint8_t kPrio)
{
    uint8_t cpuId;

    /* Check parameters */
    if(kPrio > KERNEL_LOWEST_PRIORITY || kPrio == pThread->priority)
    {
        return;
    }

    KERNEL_LOCK(pThread->lock);
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
                     TRUE);

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
}

void* schedThreadAddResource(const thread_resource_t* kpResource)
{
    kernel_thread_t* pThread;
    kqueue_node_t*   pNewNode;

    /* Check the parameters */
    if(kpResource == NULL || kpResource->pReleaseResource == NULL)
    {
        return NULL;
    }

    pThread = schedGetCurrentThread();
    KERNEL_LOCK(pThread->lock);

    /* Create the new node for the resource */
    pNewNode = kQueueCreateNode((void*)kpResource, FALSE);
    if(pNewNode == NULL)
    {
        KERNEL_UNLOCK(pThread->lock);
        return NULL;
    }

    /* Add the node to the thread's queue */
    kQueuePush(pNewNode, pThread->pThreadResources);
    KERNEL_UNLOCK(pThread->lock);

    return pNewNode;
}

OS_RETURN_E schedThreadRemoveResource(void* pResourceHandle)
{
    kernel_thread_t* pThread;
    kqueue_node_t*   pNode;

    /* Check the parameters */
    if(pResourceHandle == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    pNode = pResourceHandle;
    pThread = schedGetCurrentThread();

    KERNEL_LOCK(pThread->lock);

    /* Check the handle validity */
    if(pNode->pQueuePtr != pThread->pThreadResources)
    {
        KERNEL_UNLOCK(pThread->lock);

        return OS_ERR_INCORRECT_VALUE;
    }
    kQueueRemove(pThread->pThreadResources, pNode, TRUE);
    kQueueDestroyNode(&pNode);

    KERNEL_UNLOCK(pThread->lock);
    return OS_NO_ERR;
}

OS_RETURN_E schedTerminateThread(kernel_thread_t*               pThread,
                                 const THREAD_TERMINATE_CAUSE_E kCause)
{
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
        SCHED_ASSERT(FALSE,
                     "Exit point returned on terminate",
                     OS_ERR_UNAUTHORIZED_ACTION);
    }

    /* Different thread, directly set as terminating */
    pThread->terminateCause = kCause;
    KERNEL_UNLOCK(pThread->lock);
    error = signalThread(pThread, THREAD_SIGNAL_KILL);

    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    return error;
}

void schedThreadExit(const THREAD_TERMINATE_CAUSE_E kCause,
                     const THREAD_RETURN_STATE_E    kRetState,
                     void*                          pRetVal)
{
    _threadExitPoint(kCause, kRetState, pRetVal);
}

size_t schedGetThreads(thread_info_t* pThreadTable, const size_t kTableSize)
{
    size_t                  i;
    kqueue_node_t*          pNode;
    internal_thread_info_t* pInfo;

    if(pThreadTable == NULL)
    {
        return 0;
    }

    KERNEL_LOCK(sThreadListLock);
    pNode = spThreadList->pHead;
    for(i = 0; i < kTableSize && pNode != NULL; ++i)
    {
        pInfo = (internal_thread_info_t*)pNode->pData;

        pThreadTable[i].tid          = pInfo->tid;
        pThreadTable[i].type         = pInfo->type;
        pThreadTable[i].priority     = *(pInfo->pPriority);
        pThreadTable[i].currentState = *(pInfo->pCurrentState);
        pThreadTable[i].affinity     = *(pInfo->pAffinity);
        pThreadTable[i].schedCpu     = *(pInfo->pSchedCpu);
        memcpy(pThreadTable[i].pName, pInfo->pName, THREAD_NAME_MAX_LENGTH);

        pNode = pNode->pNext;
    }
    KERNEL_UNLOCK(sThreadListLock);

    return i;
}

void schedDisablePreemption(void)
{
    kernel_thread_t* pThread;

    pThread = schedGetCurrentThread();
    KERNEL_LOCK(pThread->lock);

    pThread->preemptionDisabled = TRUE;

    KERNEL_UNLOCK(pThread->lock);
}

void schedEnablePreemption(void)
{
    kernel_thread_t* pThread;

    pThread = schedGetCurrentThread();
    KERNEL_LOCK(pThread->lock);

    pThread->preemptionDisabled = FALSE;

    KERNEL_UNLOCK(pThread->lock);
}


/************************************ EOF *************************************/