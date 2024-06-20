/*******************************************************************************
 * @file spinlock.h
 *
 * @see critical.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 01/04/2023
 *
 * @version 1.0
 *
 * @brief Kernel's spinlock management module.
 *
 * @details Kernel's spinlock management module. Defines the different basic
 * synchronization primitives used in the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CPU_SPINLOCK_H_
#define __CPU_SPINLOCK_H_

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

/** @brief Defines a kernel spinlock. */
typedef struct
{
    /** @brief The lock value */
    volatile uint32_t lock;

    /** @brief The interrupt state when acquiring the lock */
    uint8_t intState[MAX_CPU_COUNT];
} kernel_spinlock_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Initializes a spinlock.
 *
 * @details Initializes a spinlock. This function is safe in kernel mode.
 *
 * @param[out] LOCK The lock to initialize.
*/
#define KERNEL_SPINLOCK_INIT(LOCK) {    \
    LOCK.lock = 0;                      \
}


/** @brief Kernel spinlock initializer */
#define KERNEL_SPINLOCK_INIT_VALUE {0}

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
void spinlockAcquire(volatile uint32_t * pLock);

/**
 * @brief Unlocks a spinlock.
 *
 * @details Unlocks a spinlock. This function is safe in kernel mode.
 *
 * @param[out] pLock The pointer to the lock to unlock.
*/
void spinlockRelease(volatile uint32_t * pLock);

#endif /* #ifndef __CPU_SPINLOCK_H_ */

/************************************ EOF *************************************/