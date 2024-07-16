/*******************************************************************************
 * @file critical.c
 *
 * @see critical.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 10/07/2024
 *
 * @version 1.0
 *
 * @brief Kernel critical section implementation.
 *
 * @details Kernel critical section implementation. Uses spinlock and interrupt
 * management to create critical sections.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <stdint.h>           /* Standard int definitions */
#include <atomic.h>           /* Atomic sections */
#include <interrupts.h>       /* Interrupts management */

/* Configuration files */
#include <config.h>

/* Header file */
#include <critical.h>

/* Unit test header */
#include <test_framework.h>

/* Tracing feature */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
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

void KernelLock(kernel_spinlock_t* pLock)
{
    uint32_t interruptState;

    /* Get interrupt state, disable interrupts, lock spinlock */
    interruptState = interruptDisable();
    spinlockAcquire(&pLock->lock);
    pLock->interruptState = interruptState;
}

void KernelUnlock(kernel_spinlock_t* pLock)
{
    while(pLock->lock == 0){} // TODO: Remove

    /* Unlock the spinlock and restore the interrupt state */
    spinlockRelease(&pLock->lock);
    interruptRestore(pLock->interruptState);
}

/************************************ EOF *************************************/