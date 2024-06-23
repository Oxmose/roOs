/*******************************************************************************
 * @file critical.h
 *
 * @see critical.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 01/04/2023
 *
 * @version 1.0
 *
 * @brief Kernel's concurency management module.
 *
 * @details Kernel's concurency management module. Defines the different basic
 * synchronization primitives used in the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CPU_CRITICAL_H_
#define __CPU_CRITICAL_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <interrupts.h> /* Interrupts management */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Enters a critical section in the kernel.
 *
 * @param[out] INT_STATE The critical state at section's entrance.
 *
 * @details Enters a critical section in the kernel. Save interrupt state and
 * disables interrupts.
 */
#define KERNEL_ENTER_CRITICAL_LOCAL(INT_STATE) {                    \
    INT_STATE = interruptDisable();                                 \
    KERNEL_TRACE_EVENT(TRACE_CRITICAL_SECTION_ENABLED,              \
                       TRACE_CPU_ENTER_CRITICAL,                    \
                       1,                                           \
                       INT_STATE);                                  \
}

/**
 * @brief Exits a critical section in the kernel.
 *
 * @param[in] INT_STATE The critical state at section's entrance.
 *
 * @details Exits a critical section in the kernel. Restore the previous
 * interrupt state.
 */
#define KERNEL_EXIT_CRITICAL_LOCAL(INT_STATE) {                 \
    interruptRestore(INT_STATE);                                \
    KERNEL_TRACE_EVENT(TRACE_CRITICAL_SECTION_ENABLED,          \
                       TRACE_CPU_EXIT_CRITICAL,                 \
                       1,                                       \
                       INT_STATE);                              \
}

/**
 * @brief Locks a spinlock.
 *
 * @details Locks a spinlock. This function is safe in kernel mode.
 *
 * @param[in, out] LOCK The lock to lock.
*/
#define KERNEL_CRITICAL_LOCK(LOCK) {                            \
    KERNEL_ENTER_CRITICAL_LOCAL((LOCK).intState[cpuGetId()]);   \
    spinlockAcquire(&((LOCK).lock));                            \
    KERNEL_TRACE_EVENT(TRACE_CRITICAL_SECTION_ENABLED,          \
                       TRACE_CPU_SPINLOCK_LOCK,                 \
                       1,                                       \
                       (LOCK).lock);                            \
}

/**
 * @brief Unlocks a spinlock.
 *
 * @details Unlocks a spinlock. This function is safe in kernel mode.
 *
 * @param[out] LOCK The lock to unlock.
*/
#define KERNEL_CRITICAL_UNLOCK(LOCK) {                          \
    spinlockRelease(&((LOCK).lock));                            \
    KERNEL_EXIT_CRITICAL_LOCAL((LOCK).intState[cpuGetId()]);    \
    KERNEL_TRACE_EVENT(TRACE_CRITICAL_SECTION_ENABLED,          \
                       TRACE_CPU_SPINLOCK_UNLOCK,               \
                       1,                                       \
                       LOCK);                                   \
}

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

#endif /* #ifndef __CPU_CRITICAL_H_ */

/************************************ EOF *************************************/