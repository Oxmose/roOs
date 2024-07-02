/*******************************************************************************
 * @file atomic.h
 *
 * @see cpu_sync.s
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 01/04/2023
 *
 * @version 1.0
 *
 * @brief Kernel's atomic management module.
 *
 * @details Kernel's atomic management module. Defines the different basic
 * synchronization primitives used in the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CPU_ATOMIC_H_
#define __CPU_ATOMIC_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h> /* Standard int definitions */
#include <config.h> /* Configuration */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines an unsigned 32 bits atomic value. */
typedef volatile uint32_t u32_atomic_t;

/** @brief Defines a regular spinlock. */
typedef volatile uint32_t spinlock_t;

/** @brief Defines a kernel spinlock. */
typedef struct
{
    /** @brief The lock value */
    spinlock_t lock;

    /** @brief The interrupt state when acquiring the lock */
    volatile uint8_t intState[MAX_CPU_COUNT];
} kernel_spinlock_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Initializes a kernel spinlock.
 *
 * @details Initializes a kernel spinlock. This function is safe in kernel mode.
 *
 * @param[out] LOCK The lock to initialize.
*/
#define KERNEL_SPINLOCK_INIT(LOCK) {    \
    LOCK.lock = 0;                      \
}


/** @brief Kernel spinlock initializer */
#define KERNEL_SPINLOCK_INIT_VALUE {0}

/**
 * @brief Initializes a regular spinlock.
 *
 * @details Initializes a regular spinlock.
 *
 * @param[out] LOCK The lock to initialize.
*/
#define SPINLOCK_INIT(LOCK) {   \
    LOCK = 0;                   \
}


/** @brief Sinlock initializer */
#define SPINLOCK_INIT_VALUE 0

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
void spinlockAcquire(spinlock_t* pLock);

/**
 * @brief Unlocks a spinlock.
 *
 * @details Unlocks a spinlock. This function is safe in kernel mode.
 *
 * @param[out] pLock The pointer to the lock to unlock.
*/
void spinlockRelease(spinlock_t* pLock);

/**
 * @brief Atomically increments a value in memory.
 *
 * @details Atomically increments a value in memory. Thus function is safe in
 * kernel mode.
 *
 * @param[out] pValue The pointer to the value to atomically increment.
 *
 * @return The value of the atomic before incrementing is returned.
 */
uint32_t atomicIncrement32(u32_atomic_t* pValue);

/**
 * @brief Atomically decrements a value in memory.
 *
 * @details Atomically decrements a value in memory. Thus function is safe in
 * kernel mode.
 *
 * @param[out] pValue The pointer to the value to atomically decrement.
 *
 * @return The value of the atomic before incrementing is returned.
 */
uint32_t atomicDecrement32(u32_atomic_t* pValue);

#endif /* #ifndef __CPU_ATOMIC_H_ */

/************************************ EOF *************************************/