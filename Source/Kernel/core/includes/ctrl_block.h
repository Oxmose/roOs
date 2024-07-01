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
 * @details ernel control block structures definitions. The files contains all
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

#include <stdint.h>   /* Generic int types */
#include <stddef.h>   /* Standard definition type */
#include <atomic.h>   /* Critical sections spinlock */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* The afinity is defined as a 64 bits bitmask */
#if MAX_CPU_COUNT > 64
#error "Affinity cannot manage enough processor"
#endif

/** @brief Maximal thead's name length. */
#define THREAD_NAME_MAX_LENGTH 32

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

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

/** @brief Thread waiting types. */
typedef enum
{
    /** @brief The thread is waiting to acquire a resource (e.g mutex, sem). */
    THREAD_WAIT_TYPE_RESOURCE,
    /** @brief The thread is waiting to acquire an IO entry. */
    THREAD_WAIT_TYPE_IO
} THREAD_WAIT_TYPE_E;

/** @brief Defines the type of waiting resource */
typedef enum
{
    /** @brief Thread is waiting on a futex */
    THREAD_WAIT_RESOURCE_FUTEX,
    /** @brief Thread is waiting on a semapore */
    THREAD_WAIT_RESOURCE_SEMAPHORE,
    /** @brief Thread is waiting on a mutex */
    THREAD_WAIT_RESOURCE_MUTEX,
} THREAD_WAIT_RESOURCE_TYPE_E;

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
    THREAD_TERMINATE_CAUSE_PANIC
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

/** @brief This is the representation of the thread for the kernel. */
typedef struct kernel_thread_t
{
    /** @brief Thread's virtual CPU context, must be at the begining of the
     * structure for easy interface with assembly.
     */
    void* pVCpu;

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

    /** @brief Thread's routine. */
    void* (*pEntryPoint)(void*);

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
    THREAD_STATE_E state;

    /** @brief Thread's wait type. This is inly relevant when the thread's state
     * is THREAD_STATE_WAITING.
     */
    THREAD_WAIT_TYPE_E blockType;

    /** @brief Type of resource the thread is blocked on. */
    THREAD_WAIT_RESOURCE_TYPE_E resourceBlockType;

    /** @brief Blocking resource */
    void* pBlockingResource;

    /** @brief Associated queue node in the scheduler */
    void* pThreadNode;

    /** @brief Thread's CPU affinity */
    uint64_t affinity;

    /** @brief Thread's currently mapped CPU */
    uint8_t schedCpu;

    /** @brief Parent thread */
    struct kernel_thread_t* pParentThread;

    /** @brief Joining thread */
    struct kernel_thread_t* pJoiningThread;

    /** @brief Currently joined thread */
    struct kernel_thread_t* pJoinedThread;

    /** @brief The thread's structure lock */
    kernel_spinlock_t lock;
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