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
#include <panic.h>        /* Kernel panic */
#include <kheap.h>        /* Kernel heap */
#include <string.h>       /* Memory manipulation */
#include <stdint.h>       /* Standard int types */
#include <stddef.h>       /* Standard definitions */
#include <kerror.h>       /* Kernel error codes */
#include <kqueue.h>       /* Kernel queues */
#include <memory.h>       /* Memory manager*/
#include <atomic.h>       /* Spinlocks */
#include <time_mgt.h>     /* Time management services */
#include <critical.h>     /* Kernel critical */
#include <ctrl_block.h>   /* Threads and processes control block */
#include <interrupts.h>   /* Interrupt manager */
#include <kerneloutput.h> /* Kernel output */

/* Configuration files */
#include <config.h>

/* Header file */
#include <scheduler.h>

/* Unit test header */
#include <test_framework.h>

/* Tracing feature */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "SCHEDULER"

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
        PANIC(ERROR, MODULE_NAME, MSG, TRUE);               \
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
static kernel_thread_t* _getNextThreadFromTable(thread_table_t* pTable);

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
kernel_thread_t* pCurrentThreadsPtr[MAX_CPU_COUNT] = {NULL};

/************************** Static global variables ***************************/
/** @brief The last TID given by the kernel. */
static u32_atomic_t sLastGivenTid;

/** @brief The number of thread in the system. */
static u32_atomic_t sThreadCount;

/** @brief Count of the number of times the scheduler was called per CPU. */
static uint64_t sScheduleCount[MAX_CPU_COUNT];

/** @brief Stores the thread tables for all CPUs */
static thread_table_t sThreadTables[MAX_CPU_COUNT];

/** @brief Stores the list of sleeping threads */
static thread_general_table_t sSleepingThreadsTable;

/** @brief Stores the list of zombie threads */
static thread_general_table_t sZombieThreadsTable;

/** @brief Pointers to the idle threads allocated per CPU. */
static kernel_thread_t* spIdleThread[MAX_CPU_COUNT];

/** @brief The number of time IDLE was scheduled during the last window. */
static uint64_t sIdleSchedCount[MAX_CPU_COUNT];

/** @brief Stores the calculated CPU load for the last window */
static double sCpuLoad[MAX_CPU_COUNT];

/** @brief Stores the scheduler interrupt line */
static uint32_t sSchedulerInterruptLine;

/** @brief Scheduler global lock */
static kernel_spinlock_t sGlobalLock;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void* _idleRoutine(void* pArgs)
{
    uint8_t cpuId;

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED, TRACE_SHEDULER_IDLE, 0);

    cpuId = (uint8_t)(uintptr_t)pArgs;

    KERNEL_DEBUG(SCHED_DEBUG_ENABLED,
                MODULE_NAME,
                "IDLE %d started on CPU %d (%d)",
                pCurrentThreadsPtr[cpuId]->tid,
                cpuId,
                cpuGetId());
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
    uint8_t          cpuId;

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_THREAD_ENTRY_PT_ENTRY,
                       1,
                       pCurrentThreadsPtr[cpuGetId()]->tid);

    cpuId = cpuGetId();

    pCurrThread = pCurrentThreadsPtr[cpuId];

    /* Set start time */
    pCurrThread->startTime = timeGetUptime();

    /* Call the thread routine */
    pThreadReturnValue = pCurrThread->pEntryPoint(pCurrThread->pArgs);

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_THREAD_ENTRY_PT_EXIT,
                       3,
                       pCurrentThreadsPtr[cpuId]->tid,
                       (uint32_t)(((uint64_t)(uintptr_t)pThreadReturnValue) >> 32),
                       (uint32_t)(uintptr_t)pThreadReturnValue);

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
    uint8_t          cpuId;
    uint32_t         intState;

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_THREAD_EXIT_PT_ENTRY,
                       3,
                       pCurrentThreadsPtr[cpuGetId()]->tid,
                       kCause,
                       kRetState);

    KERNEL_ENTER_CRITICAL_LOCAL(intState);

    cpuId      = cpuGetId();
    pCurThread = pCurrentThreadsPtr[cpuId];

    /* Cannot exit idle thread */
    SCHED_ASSERT(pCurThread != spIdleThread[cpuId],
                 "Cannot exit IDLE thread",
                 OS_ERR_UNAUTHORIZED_ACTION);

    KERNEL_CRITICAL_LOCK(pCurThread->lock);

    /* Set new thread state and put in zombie queue */
    pCurThread->state = THREAD_STATE_ZOMBIE;

    KERNEL_CRITICAL_LOCK(sZombieThreadsTable.lock);
    kQueuePush(pCurThread->pThreadNode, sZombieThreadsTable.pThreadList);
    ++sZombieThreadsTable.threadCount;
    KERNEL_CRITICAL_UNLOCK(sZombieThreadsTable.lock);

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
        KERNEL_CRITICAL_LOCK(pCurThread->pJoiningThread->lock);

        if(pJoiningThread->state == THREAD_STATE_JOINING)
        {
            /* Release the joining thread */
            pJoiningThread->state = THREAD_STATE_READY;
            schedReleaseThread(pJoiningThread);
        }
        KERNEL_CRITICAL_UNLOCK(pCurThread->pJoiningThread->lock);
    }

    KERNEL_CRITICAL_UNLOCK(pCurThread->lock);

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_THREAD_EXIT_PT_EXIT,
                       3,
                       pCurrentThreadsPtr[cpuId]->tid,
                       kCause,
                       kRetState);

    /* Schedule thread, no need for interrupt, the context does not need to be
     * saved.
     */
    schedScheduleNoInt();

    /* We should never return */
    SCHED_ASSERT(FALSE,
                 "Thread retuned after exiting",
                 OS_ERR_UNAUTHORIZED_ACTION);

    KERNEL_EXIT_CRITICAL_LOCAL(intState);
}


static void _createIdleThreads(void)
{
    uint32_t       i;
    kqueue_node_t* pNewNode;

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_CREATE_IDLE_THREADS_ENTRY,
                       0);

    for(i = 0; i < MAX_CPU_COUNT; ++i)
    {
        /* Allocate new structure */
        spIdleThread[i] = kmalloc(sizeof(kernel_thread_t));
        SCHED_ASSERT(spIdleThread[i] != NULL,
                     "Failed to allocate IDLE thread",
                     OS_ERR_NO_MORE_MEMORY);

        /* Create a new node for the threads lists */
        pNewNode = kQueueCreateNode(spIdleThread[i], TRUE);

        /* Set the thread's information */
        spIdleThread[i]->affinity      = (1ULL << i);
        spIdleThread[i]->schedCpu      = i;
        spIdleThread[i]->tid           = atomicIncrement32(&sLastGivenTid);
        spIdleThread[i]->type          = THREAD_TYPE_KERNEL;
        spIdleThread[i]->priority      = KERNEL_LOWEST_PRIORITY;
        spIdleThread[i]->pArgs         = (void*)(uintptr_t)i;
        spIdleThread[i]->pEntryPoint   = _idleRoutine;
        spIdleThread[i]->pParentThread = NULL;
        memcpy(spIdleThread[i]->pName, "IDLE", 5);

        /* Set the thread's stack for both interrupt and main */
        spIdleThread[i]->stackEnd = cpuCreateKernelStack(KERNEL_STACK_SIZE);
        KERNEL_DEBUG(SCHED_DEBUG_ENABLED, MODULE_NAME, "Idle %d stack 0x%p\n", i, spIdleThread[i]->stackEnd);
        SCHED_ASSERT(spIdleThread[i]->stackEnd != (uintptr_t)NULL,
                     "Failed to allocate IDLE thread stack",
                     OS_ERR_NO_MORE_MEMORY);

        spIdleThread[i]->stackSize       = KERNEL_STACK_SIZE;
        spIdleThread[i]->kernelStackEnd  = spIdleThread[i]->stackEnd;
        spIdleThread[i]->kernelStackSize = KERNEL_STACK_SIZE;

        /* Allocate the vCPU */
        spIdleThread[i]->pVCpu = (void*)cpuCreateVirtualCPU(_threadEntryPoint,
                                                            spIdleThread[i]);
        SCHED_ASSERT(spIdleThread[i]->pVCpu != NULL,
                     "Failed to allocate IDLE thread VCPU",
                     OS_ERR_NO_MORE_MEMORY);

        /* Set idle to READY */
        spIdleThread[i]->state = THREAD_STATE_RUNNING;

        /* Set idle node */
        spIdleThread[i]->pThreadNode = pNewNode;

        ++sThreadTables[i].threadCount;
        atomicIncrement32(&sThreadCount);
    }

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_CREATE_IDLE_THREADS_EXIT,
                       0);
}

static kernel_thread_t* _getNextThreadFromTable(thread_table_t* pTable)
{
    uint8_t        nextPrio;
    uint8_t        i;
    kqueue_node_t* pThreadNode;

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_GET_NEXT_THREAD_FROM_TABLE_ENTRY,
                       0);

    pThreadNode = NULL;

    KERNEL_CRITICAL_LOCK(pTable->lock);
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

    KERNEL_CRITICAL_UNLOCK(pTable->lock);

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_GET_NEXT_THREAD_FROM_TABLE_EXIT,
                       1,
                       ((kernel_thread_t*)pThreadNode->pData)->tid);

    return pThreadNode->pData;
}

static void _updateSleepingThreads(void)
{
    uint64_t       currTime;
    kqueue_node_t* pThreadNode;
    kqueue_node_t* pCursor;

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_UPDATE_SLEEPING_THREADS_ENTRY,
                       0);

    KERNEL_CRITICAL_LOCK(sSleepingThreadsTable.lock);
    if(sSleepingThreadsTable.threadCount != 0)
    {
        /* Get the current time and compate to threads in the sleeping list */
        currTime = timeGetUptime();
        pCursor = sSleepingThreadsTable.pThreadList->pTail;
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
            --sSleepingThreadsTable.threadCount;
            pThreadNode = kQueuePop(sSleepingThreadsTable.pThreadList);
            SCHED_ASSERT(pThreadNode != NULL,
                         "Got a NULL thread node",
                         OS_ERR_NULL_POINTER);

            /* Release the thread */
            schedReleaseThread(pThreadNode->pData);
        }
    }
    KERNEL_CRITICAL_UNLOCK(sSleepingThreadsTable.lock);

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_UPDATE_SLEEPING_THREADS_EXIT,
                       0);
}

static void _schedCleanThread(kernel_thread_t* pThread)
{
    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_CLEAN_THREAD_ENTRY,
                       1,
                       pThread->tid);

    KERNEL_CRITICAL_LOCK(pThread->lock);

    /* Destroy the kernel stack */
    cpuDestroyKernelStack(pThread->kernelStackEnd,
                          pThread->kernelStackSize);

    /* TODO: Destroy the regular stack */

    /* Destroy the virutal CPU */
    cpuDestroyVirtualCPU((uintptr_t)pThread->pVCpu);

    /* Clear the active thread node, it should not be in any queue */
    kQueueDestroyNode((kqueue_node_t**)&pThread->pThreadNode);

    KERNEL_CRITICAL_UNLOCK(pThread->lock);

    /* Free the thread */
    kfree(pThread);

    /* Decrement thread count */
    atomicDecrement32(&sThreadCount);

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_CLEAN_THREAD_EXIT,
                       1,
                       pThread->tid);
}

void schedInit(void)
{
    uint32_t    i;
    uint32_t    j;
    OS_RETURN_E error;

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_INIT_ENTRY,
                       0);

    /* Init values */
    sLastGivenTid = 0;
    sThreadCount  = 0;
    memset(pCurrentThreadsPtr, 0, sizeof(kernel_thread_t*) * MAX_CPU_COUNT);
    memset(sScheduleCount, 0, sizeof(uint64_t) * MAX_CPU_COUNT);
    memset(sIdleSchedCount, 0, sizeof(uint64_t) * MAX_CPU_COUNT);
    KERNEL_SPINLOCK_INIT(sGlobalLock);

    /* Initialize the thread table */
    for(j = 0; j < MAX_CPU_COUNT; ++j)
    {
        sThreadTables[j].highestPriority = KERNEL_LOWEST_PRIORITY;
        sThreadTables[j].threadCount     = 0;
        KERNEL_SPINLOCK_INIT(sThreadTables[j].lock);
        for(i = 0; i <= KERNEL_LOWEST_PRIORITY; ++i)
        {
            sThreadTables[j].pReadyList[i]  = kQueueCreate(TRUE);

            SCHED_ASSERT(sThreadTables[j].pReadyList[i] != NULL,
                        "Failed to allocate scheduler threads lists",
                        OS_ERR_NO_MORE_MEMORY);
        }
    }

    /* Initialize the sleeping threads list */
    sSleepingThreadsTable.threadCount = 0;
    sSleepingThreadsTable.pThreadList = kQueueCreate(TRUE);
    KERNEL_SPINLOCK_INIT(sSleepingThreadsTable.lock);

    SCHED_ASSERT(sSleepingThreadsTable.pThreadList != NULL,
                 "Failed to allocate scheduler sleeping threads list",
                 OS_ERR_NO_MORE_MEMORY);

    /* Initialize the zombie threads list */
    sZombieThreadsTable.threadCount = 0;
    sZombieThreadsTable.pThreadList = kQueueCreate(TRUE);
    KERNEL_SPINLOCK_INIT(sZombieThreadsTable.lock);

    SCHED_ASSERT(sZombieThreadsTable.pThreadList != NULL,
                 "Failed to allocate scheduler zombie threads list",
                 OS_ERR_NO_MORE_MEMORY);

    /* Create the idle process for each CPU */
    _createIdleThreads();

    /* Set idle as current thread for all CPUs */
    for(i = 0; i < MAX_CPU_COUNT; ++i)
    {
        pCurrentThreadsPtr[i] = spIdleThread[i];
    }

    /* Register the software interrupt line for scheduling */
    sSchedulerInterruptLine = cpuGetInterruptConfig()->schedulerInterruptLine;
    error = interruptRegister(sSchedulerInterruptLine, schedScheduleHandler);
    SCHED_ASSERT(error == OS_NO_ERR,
                 "Failed to register scheduler interrupt",
                 error);

    KERNEL_DEBUG(SCHED_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Scheduler initialization end");

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_INIT_EXIT,
                       0);

    TEST_POINT_FUNCTION_CALL(schedulerTest, TEST_SCHEDULER_ENABLED);
}

void schedScheduleNoInt(void)
{
    uint8_t          cpuId;
    uint8_t          currPrio;
    uint8_t          nextPrio;
    thread_table_t*  pCurrentTable;
    kernel_thread_t* pThread;

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_SCHEDULE_ENTRY,
                       1,
                       pCurrentThreadsPtr[cpuGetId()]->tid);

    SCHED_ASSERT(cpuGeIntState() == 0,
                 "Called scheduler no int with interrupt enabled",
                 OS_ERR_UNAUTHORIZED_ACTION);

    /* Get current CPU ID */
    cpuId = cpuGetId();
    pCurrentTable = &sThreadTables[cpuId];

    /* Wakeup sleeping threads if needed */
    _updateSleepingThreads();

    KERNEL_CRITICAL_LOCK(pCurrentTable->lock);

    pThread = pCurrentThreadsPtr[cpuId];

    /* If the current process can still run */
    if(pThread->state == THREAD_STATE_RUNNING)
    {
        nextPrio = pCurrentTable->highestPriority;
        currPrio = pThread->priority;

        /* But there might be a higher prio or another thread with same
         * priority.
         */
        if(nextPrio < currPrio ||
           pCurrentTable->pReadyList[currPrio]->pHead != NULL)
        {
            /* Put back the thread in the list, the highest priority will be
             * updated by the next _getNextThreadFromTable.
             * We do not have to manage the highest priority because we know
             * it is currently equal or higher.
             */
            KERNEL_CRITICAL_UNLOCK(sThreadTables[cpuId].lock);

            schedReleaseThread(pThread);

            /* Elect the new thread*/
            pCurrentThreadsPtr[cpuId] = _getNextThreadFromTable(pCurrentTable);
        }
        else
        {
            KERNEL_CRITICAL_UNLOCK(sThreadTables[cpuId].lock);
        }
    }
    else
    {
        KERNEL_CRITICAL_UNLOCK(sThreadTables[cpuId].lock);

        /* Otherwise just select a new thread */
        pCurrentThreadsPtr[cpuId] = _getNextThreadFromTable(pCurrentTable);
    }

    /* Set new state */
    pThread = pCurrentThreadsPtr[cpuId];
    pThread->state = THREAD_STATE_RUNNING;

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
    sCpuLoad[cpuId] = 100 - (100 *
                      (double)sIdleSchedCount[cpuId] /
                      (double)CPU_LOAD_TICK_WINDOW);
    ++sScheduleCount[cpuId];

    KERNEL_DEBUG(SCHED_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Selected thread %d for RUN on CPU %d (%d)",
                 pThread->tid,
                 cpuId,
                 (uint32_t)sIdleSchedCount[cpuId]);

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_SCHEDULE_EXIT,
                       1,
                       pCurrentThreadsPtr[cpuId]->tid);
    /* We should never comme back */
    cpuRestoreContext(pThread);

    SCHED_ASSERT(FALSE,
                 "Schedule returned",
                 OS_ERR_UNAUTHORIZED_ACTION);
}

void schedSchedule(void)
{
    /* Just generate a scheduler interrupt */
    cpuRaiseInterrupt(sSchedulerInterruptLine);
}

void schedScheduleHandler(kernel_thread_t* pThread)
{
    (void)pThread;

    /* Just call the scheduler */
    schedScheduleNoInt();
}

void schedReleaseThread(kernel_thread_t* pThread)
{
    uint64_t i;
    uint8_t  cpuId;
    double   lastCpuLoad;
    uint32_t intState;

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_RELEASE_THREAD_ENTRY,
                       1,
                       pThread->tid);

    KERNEL_ENTER_CRITICAL_LOCAL(intState);

    /* Get the CPU list to release to */
    lastCpuLoad = 1000.0;
    cpuId       = MAX_CPU_COUNT;

    if(pThread->affinity != 0)
    {
        for(i = 0; i < MAX_CPU_COUNT; ++i)
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
        for(i = 0; i < MAX_CPU_COUNT; ++i)
        {
            if(lastCpuLoad > sCpuLoad[i])
            {
                lastCpuLoad = sCpuLoad[i];
                cpuId       = i;
            }
        }
    }

    SCHED_ASSERT(cpuId != MAX_CPU_COUNT,
                 "Failed to find a CPU to relase the thread",
                 OS_ERR_INCORRECT_VALUE);

    /* Update the thread state and push to queue */
    pThread->state    = THREAD_STATE_READY;
    pThread->schedCpu = cpuId;

    KERNEL_CRITICAL_LOCK(sThreadTables[cpuId].lock);

    kQueuePush(pThread->pThreadNode,
               sThreadTables[cpuId].pReadyList[pThread->priority]);
    ++sThreadTables[cpuId].threadCount;
    if(sThreadTables[cpuId].highestPriority > pThread->priority)
    {
        sThreadTables[cpuId].highestPriority = pThread->priority;
    }

    KERNEL_CRITICAL_UNLOCK(sThreadTables[cpuId].lock);

    KERNEL_DEBUG(SCHED_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Mapped thread %d to CPU %d",
                 pThread->tid,
                 cpuId);

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_RELEASE_THREAD_EXIT,
                       2,
                       pThread->tid,
                       cpuId);

    KERNEL_EXIT_CRITICAL_LOCAL(intState);
}


OS_RETURN_E schedSleep(const uint64_t kTimeNs)
{
    kernel_thread_t* pCurrThread;

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_SLEEP_ENTRY,
                       3,
                       pCurrentThreadsPtr[cpuGetId()]->tid,
                       kTimeNs >> 32,
                       (uint32_t)kTimeNs);

    /* Check the current thread */
    pCurrThread = pCurrentThreadsPtr[cpuGetId()];
    if(pCurrThread == spIdleThread[cpuGetId()])
    {
        KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                           TRACE_SHEDULER_SLEEP_EXIT,
                           4,
                           pCurrentThreadsPtr[cpuGetId()]->tid,
                           kTimeNs >> 32,
                           (uint32_t)kTimeNs,
                           OS_ERR_UNAUTHORIZED_ACTION);
        return OS_ERR_UNAUTHORIZED_ACTION;
    }


    /* Get the current time and set as sleeping*/
    pCurrThread->wakeupTime = timeGetUptime() + kTimeNs;

    KERNEL_CRITICAL_LOCK(sSleepingThreadsTable.lock);

    /* Put the thread in the sleeping queue */
    kQueuePushPrio(pCurrThread->pThreadNode,
                   sSleepingThreadsTable.pThreadList,
                   pCurrThread->wakeupTime);
    ++sSleepingThreadsTable.threadCount;
    pCurrThread->state = THREAD_STATE_SLEEPING;

    KERNEL_CRITICAL_UNLOCK(sSleepingThreadsTable.lock);

    /* Request scheduling */
    cpuRaiseInterrupt(sSchedulerInterruptLine);

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_SLEEP_EXIT,
                       4,
                       pCurrentThreadsPtr[cpuGetId()]->tid,
                       kTimeNs >> 32,
                       (uint32_t)kTimeNs,
                       OS_NO_ERR);

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

int32_t schedGetTid(void)
{
    kernel_thread_t* pCur;
    uint32_t         intState;

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    pCur = pCurrentThreadsPtr[cpuGetId()];
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    return pCur->tid;
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
    kqueue_node_t*   pNewNode;
    OS_RETURN_E      error;

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_CREATE_THREAD_ENTRY,
                       2,
                       (uint32_t)(((uint64_t)(uintptr_t)ppThread) >> 32),
                       (uint32_t)(uintptr_t)ppThread);

    pNewThread = NULL;
    pNewNode   = NULL;
    error      = OS_NO_ERR;

    /* Allocate new structure */
    pNewThread = kmalloc(sizeof(kernel_thread_t));
    if(pNewThread == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }

    /* Create a new node for the threads lists */
    pNewNode = kQueueCreateNode(pNewThread, FALSE);
    if(pNewNode == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }

    /* Validate parameters */
    if((kAffinitySet >> MAX_CPU_COUNT) != 0 ||
        kPriority > KERNEL_LOWEST_PRIORITY ||
        ppThread == NULL ||
        pRoutine == NULL ||
        kpName   == NULL ||
        kStackSize == 0)
    {
        error = OS_ERR_INCORRECT_VALUE;
        goto SCHED_CREATE_KTHREAD_END;
    }

    memset(pNewThread, 0, sizeof(kernel_thread_t));

    /* Set the thread's information */
    pNewThread->affinity      = kAffinitySet;
    pNewThread->tid           = atomicIncrement32(&sLastGivenTid);
    pNewThread->type          = THREAD_TYPE_KERNEL;
    pNewThread->priority      = kPriority;
    pNewThread->pArgs         = args;
    pNewThread->pEntryPoint   = pRoutine;
    pNewThread->pParentThread = pCurrentThreadsPtr[cpuGetId()];
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

    /* Allocate the vCPU */
    pNewThread->pVCpu = (void*)cpuCreateVirtualCPU(_threadEntryPoint,
                                                   pNewThread);
    if(pNewThread->pVCpu == NULL)
    {
        error = OS_ERR_NO_MORE_MEMORY;
        goto SCHED_CREATE_KTHREAD_END;
    }

    /* Set thread to READY */
    pNewThread->state = THREAD_STATE_READY;

    /* Set the node node */
    pNewThread->pThreadNode = pNewNode;

    /* Initialize the lock */
    KERNEL_SPINLOCK_INIT(pNewThread->lock);

    /* Increase the global number of threads */
    atomicIncrement32(&sThreadCount);

    /* Release the thread */
    schedReleaseThread(pNewThread);

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
            if(pNewThread->pVCpu != NULL)
            {
                cpuDestroyVirtualCPU((uintptr_t)pNewThread->pVCpu);
            }
            kfree(pNewThread);
        }
        if(pNewNode != NULL)
        {
            kQueueDestroyNode(&pNewNode);
        }

        *ppThread = NULL;
    }
    else
    {
        *ppThread = pNewThread;
    }

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_CREATE_THREAD_EXIT,
                       4,
                       (uint32_t)(((uint64_t)(uintptr_t)ppThread) >> 32),
                       (uint32_t)(uintptr_t)ppThread,
                       pNewThread->tid,
                       error);

    return error;
}

OS_RETURN_E schedJoinThread(kernel_thread_t*          pThread,
                            void**                    ppRetVal,
                            THREAD_TERMINATE_CAUSE_E* pTerminationCause)
{
    kernel_thread_t* pCurThread;

    if(pThread == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                           TRACE_SHEDULER_JOIN_THREAD_ENTRY,
                           1,
                           -1);

        KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                           TRACE_SHEDULER_JOIN_THREAD_EXIT,
                           2,
                           -1,
                           OS_ERR_NULL_POINTER);
        return OS_ERR_NULL_POINTER;
    }

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_JOIN_THREAD_ENTRY,
                       1,
                       pThread->tid);

    pCurThread = pCurrentThreadsPtr[cpuGetId()];

    if(pCurThread == spIdleThread[cpuGetId()])
    {
        KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                           TRACE_SHEDULER_JOIN_THREAD_EXIT,
                           2,
                           pThread->tid,
                           OS_ERR_UNAUTHORIZED_ACTION);
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    KERNEL_CRITICAL_LOCK(pCurThread->lock);
    KERNEL_CRITICAL_LOCK(pThread->lock);

    /* Check if a thread is already joining */
    if(pThread->pJoiningThread != NULL)
    {
        KERNEL_CRITICAL_UNLOCK(pThread->lock);
        KERNEL_CRITICAL_UNLOCK(pCurThread->lock);

        KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                           TRACE_SHEDULER_JOIN_THREAD_EXIT,
                           2,
                           pThread->tid,
                           OS_ERR_UNAUTHORIZED_ACTION);
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    /* Check if the thread is already exited */
    if(pThread->state == THREAD_STATE_ZOMBIE)
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
        KERNEL_CRITICAL_LOCK(sZombieThreadsTable.lock);
        kQueueRemove(sZombieThreadsTable.pThreadList,
                     pThread->pThreadNode,
                     TRUE);
        --sZombieThreadsTable.threadCount;
        KERNEL_CRITICAL_UNLOCK(sZombieThreadsTable.lock);

        /* Clean the thread */
        KERNEL_CRITICAL_UNLOCK(pThread->lock);
        _schedCleanThread(pThread);
        KERNEL_CRITICAL_UNLOCK(pCurThread->lock);

        KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                           TRACE_SHEDULER_JOIN_THREAD_EXIT,
                           2,
                           pThread->tid,
                           OS_NO_ERR);

        return OS_NO_ERR;
    }

    /* The thread is no yet exited, add to the joining thread and lock the
     * caller thread.
     */

    pCurThread->state = THREAD_STATE_JOINING;
    pCurThread->pJoinedThread = pThread;
    pThread->pJoiningThread = pCurThread;

    KERNEL_CRITICAL_UNLOCK(pThread->lock);
    KERNEL_CRITICAL_UNLOCK(pCurThread->lock);

    /* Request scheduling */
    cpuRaiseInterrupt(sSchedulerInterruptLine);

    /* Remove thread from zombie list */
    KERNEL_CRITICAL_LOCK(sZombieThreadsTable.lock);
    kQueueRemove(sZombieThreadsTable.pThreadList,
                 pThread->pThreadNode,
                 TRUE);
    --sZombieThreadsTable.threadCount;
    KERNEL_CRITICAL_UNLOCK(sZombieThreadsTable.lock);

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

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_JOIN_THREAD_EXIT,
                       2,
                       pThread->tid,
                       OS_NO_ERR);

    return OS_NO_ERR;
}

double schedGetCpuLoad(const uint8_t kCpuId)
{
    if(kCpuId < MAX_CPU_COUNT)
    {
        return sCpuLoad[kCpuId];
    }
    else
    {
        return 0;
    }
}

void schedWaitThreadOnResource(kernel_thread_t*                  pThread,
                               const THREAD_WAIT_RESOURCE_TYPE_E kResource,
                               void*                             pResourceData)
{
    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_WAIT_ON_RESOURCE_ENTRY,
                       2,
                       pThread->tid,
                       kResource);

    KERNEL_CRITICAL_LOCK(pThread->lock);

    pThread->state             = THREAD_STATE_WAITING;
    pThread->blockType         = THREAD_WAIT_TYPE_RESOURCE;
    pThread->resourceBlockType = kResource;
    pThread->pBlockingResource = pResourceData;

    KERNEL_CRITICAL_UNLOCK(pThread->lock);

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_WAIT_ON_RESOURCE_EXIT,
                       2,
                       pThread->tid,
                       kResource);
}

void schedUpdatePriority(kernel_thread_t* pThread, const uint8_t kPrio)
{
    uint8_t cpuId;

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_UPDATE_PRIORITY_ENTRY,
                       2,
                       pThread->tid,
                       kPrio);

    /* Check parameters */
    if(kPrio > KERNEL_LOWEST_PRIORITY || kPrio == pThread->priority)
    {
        KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                           TRACE_SHEDULER_UPDATE_PRIORITY_EXIT,
                           3,
                           pThread->tid,
                           pThread->priority,
                           kPrio);
        return;
    }

    KERNEL_CRITICAL_LOCK(pThread->lock);
    /* Check if the thread is running, if it is not, we have to update the
     * belonging table when ready
     */
    if(pThread->state == THREAD_STATE_READY)
    {
        cpuId = pThread->schedCpu;

        /* Update the table */
        KERNEL_CRITICAL_LOCK(sThreadTables[cpuId].lock);

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
        KERNEL_CRITICAL_UNLOCK(sThreadTables[cpuId].lock);
    }

    pThread->priority = kPrio;
    KERNEL_CRITICAL_UNLOCK(pThread->lock);

    KERNEL_TRACE_EVENT(TRACE_SCHEDULER_ENABLED,
                       TRACE_SHEDULER_UPDATE_PRIORITY_EXIT,
                       3,
                       pThread->tid,
                       pThread->priority,
                       kPrio);
}

/************************************ EOF *************************************/