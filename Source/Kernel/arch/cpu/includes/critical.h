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

/** @brief Defines a kernel spinlock. */
typedef struct
{
    /** @brief The lock value */
    volatile uint32_t lock;

    /** @brief The interrupt state when acquiring the lock */
    uint32_t intState;
} kernel_spinlock_t;

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
#define ENTER_CRITICAL(INT_STATE) {                  \
    INT_STATE = interruptDisable();                  \
    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,        \
                       TRACE_X86_CPU_ENTER_CRITICAL, \
                       1,                            \
                       INT_STATE);                   \
}

/**
 * @brief Exits a critical section in the kernel.
 *
 * @param[in] INT_STATE The critical state at section's entrance.
 *
 * @details Exits a critical section in the kernel. Restore the previous
 * interrupt state.
 */
#define EXIT_CRITICAL(INT_STATE) {                  \
    interruptRestore(INT_STATE);                    \
    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,       \
                       TRACE_X86_CPU_EXIT_CRITICAL, \
                       1,                           \
                       INT_STATE);                  \
}

/**
 * @brief Locks a spinlock.
 *
 * @details Locks a spinlock. This function is safe in kernel mode.
 *
 * @param[in, out] LOCK The lock to lock.
*/
#define KERNEL_CRITICAL_LOCK(LOCK) {                \
    cpuSpinlockAcquire(&((LOCK).lock));             \
    ENTER_CRITICAL((LOCK).intState);                \
    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,       \
                       TRACE_X86_CPU_SPINLOCK_LOCK, \
                       1,                           \
                       LOCK);                       \
}

/**
 * @brief Unlocks a spinlock.
 *
 * @details Unlocks a spinlock. This function is safe in kernel mode.
 *
 * @param[out] LOCK The lock to unlock.
*/
#define KERNEL_CRITICAL_UNLOCK(LOCK) {                  \
    cpuSpinlockRelease(&((LOCK).lock));                 \
    EXIT_CRITICAL((LOCK).intState);                     \
    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,           \
                       TRACE_X86_CPU_SPINLOCK_UNLOCK,   \
                       1,                               \
                       LOCK);                           \
}

/**
 * @brief Initializes a spinlock.
 *
 * @details Initializes a spinlock. This function is safe in kernel mode.
 *
 * @param[out] LOCK The lock to initialize.
*/
#define KERNEL_SPINLOCK_INIT(LOCK) {    \
    LOCK.lock = 0;                      \
    LOCK.intState = 0;                  \
}


/** @brief Kernel spinlock initializer */
#define KERNEL_SPINLOCK_INIT_VALUE {.lock = 0, .intState = 0}

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
 * @brief Locks a spinlock.
 *
 * @details Locks a spinlock. This function is safe in kernel mode.
 *
 * @param[in, out] pLock The pointer to the lock to lock.
*/
void cpuSpinlockAcquire(volatile uint32_t * pLock);

/**
 * @brief Unlocks a spinlock.
 *
 * @details Unlocks a spinlock. This function is safe in kernel mode.
 *
 * @param[out] pLock The pointer to the lock to unlock.
*/
void cpuSpinlockRelease(volatile uint32_t * pLock);

#endif /* #ifndef __CPU_CRITICAL_H_ */

/************************************ EOF *************************************/