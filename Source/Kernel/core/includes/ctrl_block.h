/*******************************************************************************
 * @file ctrl_block.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/04/2023
 *
 * @version 3.0
 *
 * @brief Kernel control block structures definitions.
 *
 * @details Kernel control block structures definitions. The files contains all
 *  the data relative to the objects management in the system (thread structure,
 * thread state, etc.).
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_CTRL_BLOCK_H_
#define __CORE_CTRL_BLOCK_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h>     /* Generic int types */
#include <stddef.h>     /* Standard definition type */
#include <atomic.h>     /* Critical sections spinlock */
#include <stdbool.h>    /* Bool types */
#include <kqueue.h>     /* Kernel queue */
#include <uhashtable.h> /* Hahsing table */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* The afinity is defined as a 64 bits bitmask */
#if SOC_CPU_COUNT > 64
#error "Affinity cannot handle enough processor"
#endif

/** @brief Maximal thread's name length. */
#define THREAD_NAME_MAX_LENGTH 32

/** @brief Maximal number of signals a thread can support */
#define THREAD_MAX_SIGNALS 32

/** @brief Maximal process' name length. */
#define PROCESS_NAME_MAX_LENGTH 32

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Kernel thread forward declaration */
struct kernel_thread_t;

/** @brief Thread's scheduling state. */
typedef enum
{
    /** @brief Thread's scheduling state: running. */
    THREAD_STATE_RUNNING,
    /** @brief Thread's scheduling state: running to be elected. */
    THREAD_STATE_READY,
    /** @brief Thread's scheduling state: sleeping. */
    THREAD_STATE_SLEEPING,
    /** @brief Thread's scheduling state: zombie. */
    THREAD_STATE_ZOMBIE,
    /** @brief Thread's scheduling state: joining. */
    THREAD_STATE_JOINING,
    /** @brief Thread's scheduling state: waiting. */
    THREAD_STATE_WAITING,
} THREAD_STATE_E;

/** @brief Defines the type of waiting resource */
typedef enum
{
    /** @brief Thread is waiting on a futex */
    THREAD_WAIT_RESOURCE_KFUTEX,
    /** @brief Thread is waiting on a semapore */
    THREAD_WAIT_RESOURCE_KSEMAPHORE,
    /** @brief Thread is waiting on a mutex */
    THREAD_WAIT_RESOURCE_KMUTEX,
} THREAD_RESOURCE_TYPE_E;

/** @brief Defines the possitble return state of a thread. */
typedef enum
{
    /** @brief The thread returned normally. */
    THREAD_RETURN_STATE_RETURNED,
    /** @brief The thread was killed before exiting normally. */
    THREAD_RETURN_STATE_KILLED,
} THREAD_RETURN_STATE_E;

/** @brief Thread's abnomarl exit cause. */
typedef enum
{
    /** @brief The thread returned normally.  */
    THREAD_TERMINATE_CORRECTLY,
    /** @brief The thread was killed because of a division by zero. */
    THREAD_TERMINATE_CAUSE_DIV_BY_ZERO,
    /** @brief The thread was killed by a panic condition. */
    THREAD_TERMINATE_CAUSE_PANIC,
    /** @brief The thread was killed by another thread */
    THREAD_TERMINATE_CAUSE_KILLED,
    /** @brief The thread was killed due to an illegal instruction */
    THREAD_TERMINATE_CAUSE_ILLEGAL_INSTRUCTION,
    /** @brief The thread was killed due to a segmentation fault */
    THREAD_TERMINATE_CAUSE_SEGFAULT,
} THREAD_TERMINATE_CAUSE_E;

/**
 * @brief Define the thread's types in the kernel.
 */
typedef enum
{
    /** @brief Kernel thread type, create by and for the kernel. */
    THREAD_TYPE_KERNEL,
    /** @brief User thread type, created by the kernel for the user. */
    THREAD_TYPE_USER
} THREAD_TYPE_E;

/**
 * @brief Defines a thread resource type.
 */
typedef struct
{
    /** @brief Data used by the release resource function. */
    void* pResourceData;

    /**
     * @brief Release resource function.
     *
     * @details Release resource function. This function shall release the
     * memory and resources used by the resource.
     *
     * @param[in] pResourceData The resource data used by the release function.
     */
    void (*pReleaseResource)(void* pResourceData);

    /** @brief The waiting list on which the thread waits for the resource, can
     * be NULL is no wait.
     */
    void* pWaitingQueue;

    /** @brief The list handle created by the kernel. */
    void* pQueueNode;

    /** @brief The resource handle created by the kernel. */
    void* pResourceNode;

    /** @brief The thread waiting on the resource */
    struct kernel_thread_t* pThread;
} thread_resource_t;

/**
 * @brief Defines a thread error information table.
 */
typedef struct
{
    /** @brief Error information defined by segfault: address */
    uintptr_t segfaultAddr;

    /** @brief Error information defined by exceptions: type of exception */
    uint32_t exceptionId;

    /** @brief Error information defined error: address of the error */
    uintptr_t instAddr;

    /** @brief Stores the virtual CPU at the moment of the error */
    void* pExecVCpu;
} thread_error_table_t;

/**
 * @brief This is the representation of a process for the kernel.
 */
typedef struct kernel_process_t
{
    /**************************************
     * Process properties
     *************************************/
    /** @brief Process' identifier. */
    int32_t pid;

    /** @brief Process' name. */
    char pName[PROCESS_NAME_MAX_LENGTH + 1];

    /**************************************
     * Scheduler management
     *************************************/

    /** @brief Process' parent process */
    struct kernel_process_t* pParent;

    /** @brief List of children processes */
    kqueue_t* pChildren;

    /**
     * @brief Process' main thread, this is also the first thread in the
     * thread list.
     */
    struct kernel_thread_t* pMainThread;

    /** @brief Tails of the threads list. */
    struct kernel_thread_t* pThreadListTail;

    /** @brief Table of the thread list. */
    uhashtable_t* pThreadTable;

    /**************************************
     * Resources management
     *************************************/

    /** @brief Stores the process futex table. */
    uhashtable_t* pFutexTable;

    /** @brief Stores the process futex table lock */
    kernel_spinlock_t futexTableLock;

    /** @brief Stores the memory management data for the process */
    void* pMemoryData;

    /** @brief Stores the file descriptors for the process */
    void* pFdTable;

    /** @brief The process' structure lock */
    kernel_spinlock_t lock;
} kernel_process_t;

/** @brief This is the representation of the thread for the kernel. */
typedef struct kernel_thread_t
{
    /** @brief Thread's virtual CPU context, must be at the begining of the
     * structure for easy interface with assembly.
     */
    void* pVCpu;

    /** @brief Thread's regular virtual CPU context */
    void* pThreadVCpu;

    /** @brief Thread's signal virtual CPU context */
    void* pSignalVCpu;

    /**************************************
     * Thread properties
     *************************************/

    /** @brief Thread's identifier. */
    int32_t tid;

    /** @brief Thread's name. */
    char pName[THREAD_NAME_MAX_LENGTH + 1];

    /** @brief Thread's type. */
    THREAD_TYPE_E type;

    /**************************************
     * System interface
     *************************************/

    /** @brief Thread's start arguments. */
    void* pArgs;

    /** @brief Thread's entry point. */
    void* pEntryPoint;

    /** @brief Thread's routine. */
    void* (*pRoutine)(void*);

    /** @brief Thread's return value. */
    void* retValue;

    /** @brief Thread's return state. This is only relevant when the thread
     * returned.
     */
    THREAD_RETURN_STATE_E returnState;

    /** @brief Thread's return state. This is only relevant when when
     * return state is not THREAD_RETURN_STATE_RETURNED.
     */
    THREAD_TERMINATE_CAUSE_E terminateCause;

    /**************************************
     * Stacks
     *************************************/

    /** @brief Thread's stack. */
    uintptr_t stackEnd;

    /** @brief Thread's stack size. */
    size_t    stackSize;

    /** @brief Thread's interrupt stack. */
    uintptr_t kernelStackEnd;

    /** @brief Thread's interrupt stack size. */
    size_t    kernelStackSize;

    /**************************************
     * Time management
     *************************************/

    /** @brief Wake up time limit for the sleeping thread. */
    uint64_t wakeupTime;

    /** @brief Thread's start time. */
    uint64_t startTime;

    /** @brief Thread's end time. */
    uint64_t endTime;

    /**************************************
     * Scheduler management
     *************************************/

    /** @brief Thread's current priority. */
    uint8_t priority;

    /** @brief Thread's current state. */
    THREAD_STATE_E currentState;

    /** @brief Thread's next state. */
    THREAD_STATE_E nextState;

    /** @brief Associated queue node in the scheduler */
    kqueue_node_t* pThreadNode;

    /** @brief Thread list node */
    kqueue_node_t* pThreadListNode;

    /** @brief Thread's CPU affinity */
    uint64_t affinity;

    /** @brief Thread's currently mapped CPU */
    uint8_t schedCpu;

    /** @brief Tells if the thread should be scheduled */
    bool requestSchedule;

    /** @brief Tells if the thread has preemption disabled */
    bool preemptionDisabled;

    /** @brief Process */
    struct kernel_process_t* pProcess;

    /** @brief Joining thread */
    struct kernel_thread_t* pJoiningThread;

    /** @brief Currently joined thread */
    struct kernel_thread_t* pJoinedThread;

    /**************************************
     * Signals
     *************************************/

    /** @brief Last signals sent to the thread */
    uint32_t signal;

    /** @brief Thread's signal handlers table */
    void* signalHandlers[THREAD_MAX_SIGNALS];

    /** @brief Thread's error table */
    thread_error_table_t errorTable;

    /**************************************
     * Resources management
     *************************************/

    /** @brief Type of resource the thread is blocked on. */
    THREAD_RESOURCE_TYPE_E resourceBlockType;

    /** @brief Resource queue pointer */
    kqueue_t* pThreadResources;

    /** @brief The thread's structure lock */
    kernel_spinlock_t lock;

    /** @brief Thread's next node in the list */
    struct kernel_thread_t* pNext;

    /** @brief Thread's previous node in the list */
    struct kernel_thread_t* pPrev;
} kernel_thread_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/* None */

#endif /* #ifndef __CORE_CTRL_BLOCK_H_ */

/************************************ EOF *************************************/