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

#include <atomic.h>     /* Atomic sturctures */

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
 * @brief Enters a local critical section in the kernel.
 *
 * @param[out] INT_STATE The local critical state at section's entrance.
 *
 * @details Enters a local critical section in the kernel. Save interrupt state
 * and disables interrupts.
 */
#define KERNEL_ENTER_CRITICAL_LOCAL(INT_STATE) {                    \
    INT_STATE = interruptDisable();                                 \
}

/**
 * @brief Exits a local critical section in the kernel.
 *
 * @param[in] INT_STATE The local critical state at section's entrance.
 *
 * @details Exits a local critical section in the kernel. Restore the previous
 * interrupt state.
 */
#define KERNEL_EXIT_CRITICAL_LOCAL(INT_STATE) {                 \
    interruptRestore(INT_STATE);                                \
}

/**
 * @brief Locks a spinlock.
 *
 * @details Locks a spinlock. This function is safe in kernel mode.
 *
 * @param[in, out] LOCK The lock to lock.
*/
#define KERNEL_LOCK(LOCK) {                                     \
    kernelLock(&(LOCK));                                        \
}

/**
 * @brief Unlocks a spinlock.
 *
 * @details Unlocks a spinlock. This function is safe in kernel mode.
 *
 * @param[out] LOCK The lock to unlock.
*/
#define KERNEL_UNLOCK(LOCK) {                                   \
    kernelUnlock(&(LOCK));                                      \
}

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief See interrupt.h */
extern void interruptRestore(const uint32_t kPreviousState);
extern uint32_t interruptDisable(void);

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/**
 * @brief Locks a kernel spinlock.
 *
 * @details Locks a kernel spinlock. This function is safe in kernel mode.
 *
 * @param[in, out] pLock The pointer to the lock to lock.
*/
void kernelLock(kernel_spinlock_t* pLock);

/**
 * @brief Unlocks a kernel spinlock.
 *
 * @details Unlocks a kernel spinlock. This function is safe in kernel mode.
 *
 * @param[out] pLock The pointer to the lock to unlock.
*/
void kernelUnlock(kernel_spinlock_t* pLock);

#endif /* #ifndef __CPU_CRITICAL_H_ */

/************************************ EOF *************************************/