/*******************************************************************************
 * @file scheduler.h
 *
 * @see scheduler.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 15/06/2024
 *
 * @version 5.0
 *
 * @brief Kernel's scheduler.
 *
 * @details Kernel's scheduler. Thread and process creation and management
 * functions are located in this file.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CORE_SCHEDULER_H_
#define __CORE_SCHEDULER_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h>     /* Standard int definitions */
#include <kerror.h>     /* Kernel error codes */
#include <signal.h>     /* Thread signals */
#include <kqueue.h>     /* Kernel queues */
#include <stdbool.h>    /* Bool types */
#include <ctrl_block.h> /* Kernel control blocks */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Scheduler's thread lowest priority. */
#define KERNEL_LOWEST_PRIORITY  63
/** @brief Scheduler's thread highest priority. */
#define KERNEL_HIGHEST_PRIORITY 0
/** @brief Scheduler's thread priority none. */
#define KERNEL_NONE_PRIORITY (KERNEL_LOWEST_PRIORITY + 1)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the thread information structure */
typedef struct
{
    /** @brief Thread's process identifier.  */
    int32_t pid;

    /** @brief Thread's identifier. */
    int32_t tid;

    /** @brief Thread's name. */
    char pName[THREAD_NAME_MAX_LENGTH + 1];

    /** @brief Thread's type. */
    THREAD_TYPE_E type;

    /** @brief Thread's current priority. */
    uint8_t priority;

    /** @brief Thread's current state. */
    THREAD_STATE_E currentState;

    /** @brief Thread's CPU affinity */
    uint64_t affinity;

    /** @brief Thread's currently mapped CPU */
    uint8_t schedCpu;

    /* Kernel stack end address */
    uintptr_t kStack;

    /* User stack end address */
    uintptr_t uStack;
} thread_info_t;


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

/**
 * @brief Initializes the scheduler service.
 *
 * @details Initializes the scheduler features and structures. The idle and
 * init threads are created. Once set, the scheduler starts to schedule the
 * threads.
 *
 * @warning This function will never return if the initialization was successful
 * and the scheduler started.
 */
void schedInit(void);

/**
 * @brief Calls the scheduler dispatch function.
 *
 * @details Calls the scheduler. This function will select the next thread to
 * schedule and execute it.
 *
 * @param[in] kForceSwitch For the current task to be switched.
 *
 * @warning The current thread's context must be saved before calling this
 * function. Usually, this function is only called in interrupt handlers after
 * the thread's context was saved. Use schedSchedule to save the context.
 */

void schedScheduleNoInt(const bool kForceSwitch);

/**
 * @brief Calls the scheduler dispatch function using a system call.
 *
 * @details Calls the scheduler dispatch function using a system call.
 * This function will select the next thread to schedule and execute it. The
 * context of the calling thread is saved before scheduling.
 */

void schedSchedule(void);

/**
 * @brief Puts the calling thread to sleep.
 *
 * @details Puts the calling thread to sleep for at least kTimeMs nanoseconds.
 * The sleeping time can be greater depending on the time granularity and the
 * system's load.
 *
 * @param[in] kTimeNs The number of nanoseconds to wait.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_UNAUTHORIZED_ACTION is returned if sleep was called by the idle
 * thread.
 */
OS_RETURN_E schedSleep(const uint64_t kTimeNs);

/**
 * @brief Returns the number of non dead threads.
 *
 * @details Returns the number of non dead threads.
 *
 * @returns The number of non dead threads.
 */
size_t schedGetThreadCount(void);

/**
 * @brief Returns the handle to the current running thread.
 *
 * @details Returns the handle to the current running thread. This value should
 * never be NULL as a thread should always be elected for running.
 *
 * @return A handle to the current running thread is returned.
 */
kernel_thread_t* schedGetCurrentThread(void);

/**
 * @brief Creates a new thread in the thread table.
 *
 * @details Creates a new thread added in the ready threads table. The thread
 * might not be directly scheduled depending on its priority and the current
 * system's load.
 * A handle to the thread is given as parameter and set on success.
 *
 * @param[out] ppThread The pointer to the thread structure. This is the handle
 * of the thread for the user.
 * @param[in] kIsKernel Tells if the created thread is a kernel or user thread.
 * @param[in] kPriority The priority of the thread.
 * @param[in] kpName The name of the thread.
 * @param[in] kStackSize The thread's stack size in bytes, must be a multiple of
 * the system's page size.
 * @param[in] kAffinitySet The CPU affinity set for this thread.
 * @param[in] pRoutine The thread routine to be executed.
 * @param[in] args The arguments to be used by the thread.
 *
 * @return The success state or the error code.
 * - OS_NO_ERR is returned if no error is encountered.
 * - OS_ERR_FORBIDEN_PRIORITY is returned if the desired priority cannot be
 * aplied to the thread.
 * - OS_ERR_MALLOC is returned if the system could not allocate memory for the
 * new thread.
 * - OS_ERR_OUT_OF_BOUND if the desired stack size if out of the system's stack
 * size bounds.
 * - OS_ERR_UNAUTHORIZED_ACTION is the stack is not a multiple of the system's
 * page size.
 */
OS_RETURN_E schedCreateThread(kernel_thread_t** ppThread,
                              const bool        kIsKernel,
                              const uint8_t     kPriority,
                              const char*       kpName,
                              const size_t      kStackSize,
                              const uint64_t    kAffinitySet,
                              void*             (*pRoutine)(void*),
                              void*             args);

/**
 * @brief Remove a thread from the threads table.
 *
 * @details Removes a thread from the threads table. The function will wait for
 * the thread to finish before removing it.
 *
 * @param[in] pThread The handle of thread structure.
 * @param[out] ppRetVal The buffer to store the value returned by the thread's
 * routine.
 * @param[out] pTerminationCause The buffer to store the termination cause of
 * the thread.
 *
 * @return The success state or the error code. OS_NO_ERR is returned if no
 * error is encountered. Otherwise an error code is set.
 */
OS_RETURN_E schedJoinThread(kernel_thread_t*          pThread,
                            void**                    ppRetVal,
                            THREAD_TERMINATE_CAUSE_E* pTerminationCause);


/**
 * @brief Returns the CPU load in percent.
 *
 * @details Returns the CPU load in percent. The CPU load is the amount of time
 * the CPU is actually working and not idling. The CPU load is base on a time
 * window defined by the scheduler. If the CPU does not exists, 0
 * is returned.
 *
 * @param[in] kCpuId The CPU to get the load of. If the CPU does not exists, 0
 * is returned.
 *
 * @return The CPU load in percent is returned. If the CPU does not exists, 0
 * is returned.
 */
uint64_t schedGetCpuLoad(const uint8_t kCpuId);

/**
 * @brief Updates the thread's priority.
 *
 * @details Updates the thread's priority. This will affect both running and
 * not running threads. The thread and it's belonging table are updated.
 *
 * ­@param[out] pThread The thread to update.
 * @param[in] kPrio The new priority to set.
 */
OS_RETURN_E schedUpdatePriority(kernel_thread_t* pThread, const uint8_t kPrio);

/**
 * @brief Terminates a thread.
 *
 * @details Terminates a thread. The thread will terminate itself the next time
 * it is scheduled. The termination is done through signals.
 *
 * @param[in] pThread The thread to terminate.
 * @param[in] kCause The termination cause to set.
 *
 * @return The function returnsthe error or success status.
 */
OS_RETURN_E schedTerminateThread(kernel_thread_t*               pThread,
                                 const THREAD_TERMINATE_CAUSE_E kCause);

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
void schedThreadExit(const THREAD_TERMINATE_CAUSE_E kCause,
                     const THREAD_RETURN_STATE_E    kRetState,
                     void*                          pRetVal);

/**
 * @brief Fills the thread table given as parameter with up to kTableSize
 * threads executing in the system.
 *
 * @details Fills the thread table given as parameter with up to kTableSize
 * threads executing in the system. The function returns the number of
 * threads that were actually filled in the table.
 *
 * @param[out] pThreadTable The table to fill.
 * @param[in] kTableSize The size of the table in number of entries.
 *
 * @return The function returns the number of threads that were filled in the
 * table.
 */
size_t schedGetThreadsIds(int32_t* pThreadTable, const size_t kTableSize);

/**
 * @brief Fills the thread information structure.
 *
 * @details Fills the thread information structure with the information of the
 * thread with the specified thread id.
 *
 * @param[out] pInfo The thread information structure to fill.
 * @param[in] kTid The identifier of the thread to get the information of.
 *
 * @return The function returns the success or error status.
 */
OS_RETURN_E schedGetThreadInfo(thread_info_t* pInfo, const int32_t kTid);

/**
 * @brief Disables preemption for the current thread.
 *
 * @details Disables preemption for the current thread. Interrupts are still
 * handled but the thread is not switched.
 */
void schedDisablePreemption(void);

/**
 * @brief Enables preemption for the current thread.
 *
 * @details Enables preemption for the current thread. The thread might be
 * switched.
 */
void schedEnablePreemption(void);

/**
 * @brief Returns the handle to the current running process.
 *
 * @details Returns the handle to the current running process. This value should
 * never be NULL as a process should always be elected for running.
 *
 * @return A handle to the current running process is returned.
 */
kernel_process_t* schedGetCurrentProcess(void);

/**
 * @brief Sets a thread to the ready state.
 *
 * @details Sets a thread to the ready state. This function releases the thread
 * to the ready list and manage it appartenance to any other scheduler lists.
 *
 * @param[out] pThread The thread to set to ready.
 *
 * @return The function returns the success or error status.
 */
OS_RETURN_E schedSetThreadToReady(kernel_thread_t* pThread);

/**
 * @brief Sets the current thread to the waiting state.
 *
 * @details Sets the current thread to the waiting state. This function prevents
 * putting back the thread to the ready list when the scheduler kicks in.
 *
 * @return The function returns the success or error status.
 */
OS_RETURN_E schedThreadSetWaiting(void);

/**
 * @brief Tells if the sheduler has been initialized.
 *
 * @return true is returned if the scheduler has been initialized. false
 * otherwise.
 */
bool schedIsInit(void);

/**
 * @brief Tells if the sheduler is running
 *
 * @return true is returned if the scheduler is running. false
 * otherwise.
 */
bool schedIsRunning(void);

/**
 * @brief Returns if a thread is valid.
 *
 * @details Returns if a thread is valid. A thread is valid if it is still
 * registered in the process that owns it.
 *
 * @param[in] pThread The thread to check.
 *
 * @return true is returned is the thread is value, false otherwise.
 */
bool schedIsThreadValid(kernel_thread_t* pThread);

/**
 * @brief Tells if the thread is an idle thread.
 *
 * @details Tells if the thread is an idle thread.
 *
 * @param[in] kpThread The thread to test.
 *
 * @return The function returns true is the thread is an idle thread, false
 * otherwise.
 */
bool schedIsIdleThread(const kernel_thread_t* kpThread);

/**
 * @brief Forks the current process.
 *
 * @details Forks the current process. A complete copy of the current process
 * will be done and memory will be marked as COW for both new and current
 * process. Only the calling thread will be copied to the new process.
 *
 * @param[out] pNewPid The new forked process PID. This value is 0 for the new
 * process and the actual new process PID for the caller. -1 is set on error.
 *
 * @return The function returns the success or error status.
 */
OS_RETURN_E schedFork(int32_t* pNewPid);

/*************************
 * SYSTEM CALL HANDLERS
 *************************/

/**
 * @brief System call handler to sleep.
 *
 * @details System call handler to sleep. Puts the calling thread to sleep for
 * at least the required amount of time. The sleeping time can be greater
 * depending on the time granularity and the system's load.
 *
 * @warning This function must be called only after handling the associated
 * system call. Otherwise the current thread's context is not correctly saved.
 *
 * @param[out] pParams The pointer to the sleep parameter structure.
 */
void schedSyscallHandleSleep(void* pParams);

/**
 * @brief System call handler to schedule the current thread.
 *
 * @details System call handler to schedule the current thread. Calls the
 * scheduler dispatch function.This function will select the next thread to
 * schedule and execute it. The context of the calling thread is saved before
 * scheduling.
 *
 * @warning This function must be called only after handling the associated
 * system call. Otherwise the current thread's context is not correctly saved.
 *
 * @param[out] pParams The pointer to the schedule parameter structure.
 */
void schedSyscallHandleSchedule(void* pParams);

/**
 * @brief System call handler to fork the current process.
 *
 * @details System call handler to fork the current process. A complete copy of
 * the current process will be done and memory will be marked as COW for both
 * new and current process. Only the calling thread will be copied to the new
 * process.
 *
 * @warning This function must be called only after handling the associated
 * system call. Otherwise the current thread's context is not correctly saved.
 *
 * @param[out] pParams The pointer to the fork parameter structure.
 */
void schedSyscallHandleFork(void* pParams);

#endif /* #ifndef __CORE_SCHEDULER_H_ */

/************************************ EOF *************************************/
