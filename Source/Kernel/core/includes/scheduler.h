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

/* None */

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
 * @warning The current thread's context must be saved before calling this
 * function. Usually, this function is only called in interrupt handlers after
 * the thread's context was saved. Use schedSchedule to save the context.
 */

void schedScheduleNoInt(void);

/**
 * @brief Calls the scheduler dispatch function by generating an interrupt.
 *
 * @details Calls the scheduler dispatch function by generating an interrupt.
 * This function will select the next thread to schedule and execute it. The
 * context of the calling thread is saved before scheduling.
 */

void schedSchedule(void);

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

void schedScheduleHandler(kernel_thread_t* pThread);

/**
 * @brief Releases a thread to the scheduler.
 *
 * @details Releases a thread to the scheduler. This function is used to
 * put back a thread in the scheduler after locking it in, for instance, a
 * semaphore.
 *
 * @param[in] pThread The thread to release.
 */
void schedReleaseThread(kernel_thread_t* pThread);

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
 * @brief Returns the TID of the current executing thread.
 *
 * @details Returns the TID of the current executing thread.
 *
 * @returns The TID of the current executing thread.
 */
int32_t schedGetTid(void);

/**
 * @brief Creates a new kernel thread in the thread table.
 *
 * @details Creates a new thread added in the ready threads table. The thread
 * might not be directly scheduled depending on its priority and the current
 * system's load.
 * A handle to the thread is given as parameter and set on success.
 *
 * @warning These are kernel threads, sharing the kernel memory space and using
 * the kernel memory map and heap.
 *
 * @param[out] ppThread The pointer to the thread structure. This is the handle
 * of the thread for the user.
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
OS_RETURN_E schedCreateKernelThread(kernel_thread_t** ppThread,
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
double schedGetCpuLoad(const uint8_t kCpuId);

#endif /* #ifndef __CORE_SCHEDULER_H_ */

/************************************ EOF *************************************/
