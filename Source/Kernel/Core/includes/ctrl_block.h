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

#include <stdint.h>       /* Generic int types */
#include <cpu.h>          /* CPU structures */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

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
typedef struct
{
    /** @brief Thread's virtual CPU context, must be at the begining of the
     * structure for easie interface with assembly.
     */
    virtual_cpu_t v_cpu;

    /**************************************
     * Thread properties
     *************************************/

    /** @brief Thread's identifier. */
    int32_t tid;

    /** @brief Thread's name. */
    char name[THREAD_NAME_MAX_LENGTH];

    /** @brief Thread's type. */
    THREAD_TYPE_E type;

    /**************************************
     * State management
     *************************************/

    /** @brief Thread's current priority. */
    uint8_t priority;

    /** @brief Thread's current state. */
    THREAD_STATE_E state;

    /** @brief Thread's wait type. This is inly relevant when the thread's state
     * is THREAD_STATE_WAITING.
     */
    THREAD_WAIT_TYPE_E block_type;

    /**************************************
     * System interface
     *************************************/

    /** @brief Thread's start arguments. */
    void* args;

    /** @brief Thread's routine. */
    void* (*entry_point)(void*);

    /** @brief Thread's return value. */
    void* ret_val;

    /** @brief Thread's return state. This is only relevant when the thread
     * returned.
     */
    THREAD_RETURN_STATE_E return_state;

    /** @brief Thread's return state. This is only relevant when when
     * return state is not THREAD_RETURN_STATE_RETURNED.
     */
    THREAD_TERMINATE_CAUSE_E terminate_cause;

    /**************************************
     * Stacks
     *************************************/

    /** @brief Thread's stack. */
    uintptr_t stack;

    /** @brief Thread's stack size. */
    uint32_t stack_size;

    /** @brief Thread's interrupt stack. */
    uintptr_t int_stack;

    /** @brief Thread's interrupt stack size. */
    uint32_t int_stack_size;

    /**************************************
     * Time management
     *************************************/

    /** @brief Wake up time limit for the sleeping thread. */
    uint64_t wakeup_time;

    /** @brief Thread's start time. */
    uint64_t start_time;

    /** @brief Thread's end time. */
    uint64_t end_time;
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