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
    uint8_t highestPriority;

    /** @brief Stores the number of threads in the table */
    size_t threadCount;

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
    size_t threadCount;

    /**
     * @brief The list of threads in the table
     */
    kqueue_t* pThreadList;

    /** @brief List lock */
    kernel_spinlock_t lock;
} thread_sleep_table_t;

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
        PANIC(ERROR, "SCHED", MSG, TRUE);                   \
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
kernel_thread_t* pCurrentThread[MAX_CPU_COUNT] = {NULL};

/************************** Static global variables ***************************/
/** @brief The last TID given by the kernel. */
static volatile uint32_t sLastGivenTid;

/** @brief The number of thread in the system. */
static volatile uint32_t sThreadCount;

/** @brief Count of the number of times the scheduler was called per CPU. */
static uint64_t sScheduleCount[MAX_CPU_COUNT];

/** @brief Stores the thread tables for all CPUs */
static thread_table_t sThreadTables[MAX_CPU_COUNT];

/** @brief Stores the list of sleeping threads */
static thread_sleep_table_t sSleepingThreadsTable;

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

    cpuId = (uint8_t)(uintptr_t)pArgs;

    KERNEL_DEBUG(SCHED_DEBUG_ENABLED,
                 MODULE_NAME,
                 "IDLE started on CPU %d",
                 cpuId);
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

    pCurrThread = pCurrentThread[cpuGetId()];

    /* Set start time */
    pCurrThread->startTime = timeGetUptime();

    /* Call the thread routine */
    pThreadReturnValue = pCurrThread->pEntryPoint(pCurrThread->pArgs);

    /* TODO: Call the exit function */
    (void)pThreadReturnValue;

}

static void _createIdleThreads(void)
{
    uint32_t       i;
    kqueue_node_t* pNewNode;

    for(i = 0; i < MAX_CPU_COUNT; ++i)
    {
        /* Allocate new structure */
        spIdleThread[i] = kmalloc(sizeof(kernel_thread_t));
        SCHED_ASSERT(spIdleThread[i] != NULL,
                     "Failed to allocate IDLE thread",
                     OS_ERR_NO_MORE_MEMORY);

        /* Create a new node for the threads lists */
        pNewNode = kQueueCreateNode(spIdleThread[i]);
        SCHED_ASSERT(pNewNode != NULL,
                     "Failed to allocate IDLE thread node",
                     OS_ERR_NO_MORE_MEMORY);

        /* Set the thread's information */
        spIdleThread[i]->affinity    = (1ULL << i);
        spIdleThread[i]->tid         = sLastGivenTid++;
        spIdleThread[i]->type        = THREAD_TYPE_KERNEL;
        spIdleThread[i]->priority    = KERNEL_LOWEST_PRIORITY;
        spIdleThread[i]->pArgs       = (void*)i;
        spIdleThread[i]->pEntryPoint = (void*)_idleRoutine;
        memcpy(spIdleThread[i]->pName, "IDLE", 5);

        /* Set the thread's stack for both interrupt and main */
        spIdleThread[i]->stackEnd = cpuCreateKernelStack(KERNEL_STACK_SIZE);
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
        ++sThreadCount;
    }
}

static kernel_thread_t* _getNextThreadFromTable(thread_table_t* pTable)
{
    uint8_t        nextPrio;
    uint8_t        i;
    kqueue_node_t* pThreadNode;

    pThreadNode = NULL;

    /* Get the next thread node */
    nextPrio = pTable->highestPriority;
    if(nextPrio < KERNEL_NONE_PRIORITY)
    {
        /* Get a thread from the list */
        --pTable->threadCount;
        pThreadNode = kQueuePop(pTable->pReadyList[nextPrio]);
        SCHED_ASSERT(pThreadNode != NULL,
                     "Got a NULL thread node",
                     OS_ERR_NULL_POINTER);
    }

    /* Decrease the global highest priority if needed */
    for(i = nextPrio; i < KERNEL_NONE_PRIORITY; ++i)
    {
        /* Search for the next node */
        if(pTable->pReadyList[i]->pHead != NULL)
        {
            break;
        }
    }

    SCHED_ASSERT(pThreadNode != NULL,
                 "Got a NULL thread node",
                 OS_ERR_NULL_POINTER);

    pTable->highestPriority = i;
    return pThreadNode->pData;
}

static void _updateSleepingThreads(void)
{
    uint64_t       currTime;
    kqueue_node_t* pThreadNode;
    kqueue_node_t* pCursor;

    KERNEL_CRITICAL_LOCK(sSleepingThreadsTable.lock);
    if(sSleepingThreadsTable.threadCount != 0)
    {
        /* Get the current time and compate to threads in the sleeping list */
        currTime = timeGetUptime();
        pCursor = sSleepingThreadsTable.pThreadList->pTail;
        while(pCursor != NULL)
        {
            KERNEL_DEBUG(SCHED_DEBUG_ENABLED, MODULE_NAME, "Should wakeup? %llu, now is %llu", pCursor->priority, currTime);
            /* Check if we have to wake up the thread */
            if(pCursor->priority > currTime)
            {
                KERNEL_DEBUG(SCHED_DEBUG_ENABLED, MODULE_NAME, "NO");
                /* Stop here, the list if sorted */
                break;
            }

            KERNEL_DEBUG(SCHED_DEBUG_ENABLED, MODULE_NAME, "YES");

            /* Update cursor before we remove the node */
            pCursor = pCursor->pPrev;

            /* Remove the node from the list */
            --sSleepingThreadsTable.threadCount;
            pThreadNode = kQueuePop(sSleepingThreadsTable.pThreadList);
            SCHED_ASSERT(pThreadNode != NULL,
                         "Got a NULL thread node",
                         OS_ERR_NULL_POINTER);

            /* Release the thread */
            releaseThread(pThreadNode->pData);
        }
    }
    KERNEL_CRITICAL_UNLOCK(sSleepingThreadsTable.lock);
}

void schedInit(void)
{
    uint32_t    i;
    uint32_t    j;
    OS_RETURN_E error;

    /* Init values */
    sLastGivenTid = 0;
    sThreadCount  = 0;
    memset(pCurrentThread, 0, sizeof(kernel_thread_t*) * MAX_CPU_COUNT);
    memset(sScheduleCount, 0, sizeof(uint64_t) * MAX_CPU_COUNT);
    memset(sIdleSchedCount, 0, sizeof(sizeof(uint64_t) * MAX_CPU_COUNT));
    KERNEL_SPINLOCK_INIT(sGlobalLock);

    /* Initialize the thread table */
    for(j = 0; j < MAX_CPU_COUNT; ++j)
    {
        sThreadTables[j].highestPriority = KERNEL_LOWEST_PRIORITY;
        sThreadTables[j].threadCount     = 0;
        KERNEL_SPINLOCK_INIT(sThreadTables[j].lock);
        for(i = 0; i <= KERNEL_LOWEST_PRIORITY; ++i)
        {
            sThreadTables[j].pReadyList[i]  = kQueueCreate();

            SCHED_ASSERT(sThreadTables[j].pReadyList[i] != NULL,
                        "Failed to allocate scheduler threads lists",
                        OS_ERR_NO_MORE_MEMORY);
        }
    }

    /* Initialize the sleeping threads list */
    sSleepingThreadsTable.threadCount = 0;
    sSleepingThreadsTable.pThreadList = kQueueCreate();
    KERNEL_SPINLOCK_INIT(sSleepingThreadsTable.lock);

    SCHED_ASSERT(sSleepingThreadsTable.pThreadList != NULL,
                 "Failed to allocate scheduler sleeping threads list",
                 OS_ERR_NO_MORE_MEMORY);

    /* Create the idle process for each CPU */
    _createIdleThreads();

    /* Set idle as current thread for all CPUs */
    for(i = 0; i < MAX_CPU_COUNT; ++i)
    {
        pCurrentThread[i] = spIdleThread[i];
    }

    /* Register the software interrupt line for scheduling */
    sSchedulerInterruptLine = cpuGetInterruptConfig()->schedulerInterruptLine;
    error = interruptRegister(sSchedulerInterruptLine, schedScheduleHandler);
    SCHED_ASSERT(error == OS_NO_ERR,
                 "Failed to register scheduler interrupt",
                 error);

    /* Register the scheduler routine to the main system timer routine */
    error = timeRegisterSchedRoutine(schedSchedule);
    SCHED_ASSERT(error == OS_NO_ERR,
                 "Failed to register the scheduler routine to the main timer",
                 error);

    KERNEL_DEBUG(SCHED_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Scheduler initialization end");
}

void schedSchedule(void)
{
    uint8_t          cpuId;
    uint8_t          currPrio;
    uint8_t          nextPrio;
    thread_table_t*  pCurrentTable;
    kernel_thread_t* pThread;

    /* Get current CPU ID */
    cpuId = cpuGetId();
    pCurrentTable = &sThreadTables[cpuId];

    /* Wakeup sleeping threads if needed */
    _updateSleepingThreads();

    KERNEL_CRITICAL_LOCK(pCurrentTable->lock);

    pThread = pCurrentThread[cpuId];

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
            releaseThread(pThread);
            KERNEL_CRITICAL_LOCK(sThreadTables[cpuId].lock);

            /* Elect the new thread*/
            pCurrentThread[cpuId] = _getNextThreadFromTable(pCurrentTable);
        }
    }
    else
    {
        /* Otherwise just select a new thread */
        pCurrentThread[cpuId] = _getNextThreadFromTable(pCurrentTable);
    }

    KERNEL_CRITICAL_UNLOCK(sThreadTables[cpuId].lock);

    /* Set new state */
    pThread = pCurrentThread[cpuId];
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

    /* We should never comme back */
    cpuRestoreContext(pThread);

    SCHED_ASSERT(FALSE,
                 "Schedule returned",
                 OS_ERR_UNAUTHORIZED_ACTION);
}

void schedScheduleHandler(kernel_thread_t* pThread)
{
    (void)pThread;

    /* Just call the scheduler */
    schedSchedule();
}

void releaseThread(kernel_thread_t* pThread)
{
    uint64_t i;
    uint8_t  cpuId;
    double   lastCpuLoad;
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
    pThread->state = THREAD_STATE_READY;

    KERNEL_CRITICAL_LOCK(sThreadTables[cpuId].lock);

    kQueuePush(pThread->pThreadNode,
               sThreadTables[cpuId].pReadyList[pThread->priority]);
    if(sThreadTables[cpuId].highestPriority > pThread->priority)
    {
        ++sThreadTables[cpuId].threadCount;
        sThreadTables[cpuId].highestPriority = pThread->priority;
    }

    KERNEL_CRITICAL_UNLOCK(sThreadTables[cpuId].lock);

    KERNEL_DEBUG(SCHED_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Mapped thread %d to CPU %d",
                 pThread->tid,
                 cpuId);
}


OS_RETURN_E schedSleep(const uint64_t kTimeNs)
{
    kernel_thread_t* pCurrThread;

    /* Check the current thread */
    pCurrThread = pCurrentThread[cpuGetId()];
    if(pCurrThread == spIdleThread[cpuGetId()])
    {
        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    /* Get the current time and set as sleeping*/
    pCurrThread->wakeupTime = timeGetUptime() + kTimeNs;
    pCurrThread->state      = THREAD_STATE_SLEEPING;

    KERNEL_CRITICAL_LOCK(sSleepingThreadsTable.lock);

    /* Put the thread in the sleeping queue */
    kQueuePushPrio(pCurrThread->pThreadNode,
                   sSleepingThreadsTable.pThreadList,
                   pCurrThread->wakeupTime);
    ++sSleepingThreadsTable.threadCount;

    KERNEL_CRITICAL_UNLOCK(sSleepingThreadsTable.lock);

    /* Request scheduling */
    cpuRaiseInterrupt(sSchedulerInterruptLine);

    return OS_NO_ERR;
}

size_t schedGetThreadCount(void)
{
    return sThreadCount;
}

kernel_thread_t* schedGetCurrentThread(void)
{
    return pCurrentThread[cpuGetId()];
}

int32_t schedGetTid(void)
{
    return pCurrentThread[cpuGetId()]->tid;
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
    pNewNode = kQueueCreateNode(pNewThread);
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
        kStackSize == 0)
    {
        error = OS_ERR_INCORRECT_VALUE;
        goto SCHED_CREATE_KTHREAD_END;
    }

    memset(pNewThread, 0, sizeof(kernel_thread_t));

    /* Set the thread's information */
    pNewThread->affinity    = kAffinitySet;
    pNewThread->tid         = sLastGivenTid++;
    pNewThread->type        = THREAD_TYPE_KERNEL;
    pNewThread->priority    = kPriority;
    pNewThread->pArgs       = args;
    pNewThread->pEntryPoint = pRoutine;
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

    /* Release the thread and increase the global number of threads */
    releaseThread(pNewThread);

    KERNEL_CRITICAL_LOCK(sGlobalLock);
    ++sThreadCount;
    KERNEL_CRITICAL_UNLOCK(sGlobalLock);

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
            kfree(pNewNode);
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
    (void)pThread;
    (void)ppRetVal;
    (void)pTerminationCause;

    return OS_ERR_NOT_SUPPORTED;
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
/************************************ EOF *************************************/