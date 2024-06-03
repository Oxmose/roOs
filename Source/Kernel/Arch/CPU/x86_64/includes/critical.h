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

#ifndef __X86_64_CRITICAL_H_
#define __X86_64_CRITICAL_H_

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
#define ENTER_CRITICAL(INT_STATE) {         \
    INT_STATE = interruptDisable(); \
}

/**
 * @brief Exits a critical section in the kernel.
 *
 * @param[in] INT_STATE The critical state at section's entrance.
 *
 * @details Exits a critical section in the kernel. Restore the previous
 * interrupt state.
 */
#define EXIT_CRITICAL(INT_STATE) {           \
    interruptRestore(INT_STATE);     \
}

/**
 * @brief Locks a spinlock.
 *
 * @details Locks a spinlock. This function is safe in kernel mode.
 *
 * @param[in, out] LOCK The lock to lock.
*/
#define KERNEL_SPINLOCK_LOCK(LOCK) {    \
    cpuSpinlockAcquire(&LOCK);           \
}

/**
 * @brief Unlocks a spinlock.
 *
 * @details Unlocks a spinlock. This function is safe in kernel mode.
 *
 * @param[out] LOCK The lock to unlock.
*/
#define KERNEL_SPINLOCK_UNLOCK(LOCK) {  \
    cpuSpinlockRelease(&LOCK);         \
}

/**
 * @brief Initializes a spinlock.
 *
 * @details Initializes a spinlock. This function is safe in kernel mode.
 *
 * @param[out] LOCK The lock to initialize.
*/
#define KERNEL_SPINLOCK_INIT(LOCK) {                        \
    (volatile uint32_t)LOCK = KERNEL_SPINLOCK_INIT_VALUE;   \
}

/** @brief Kernel spinlock initializer */
#define KERNEL_SPINLOCK_INIT_VALUE 0

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


#endif /* #ifndef __X86_64_CRITICAL_H_ */

/************************************ EOF *************************************/