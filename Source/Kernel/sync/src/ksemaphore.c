/*******************************************************************************
 * @file ksemaphore.c
 *
 * @see ksemaphore.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 24/06/2024
 *
 * @version 2.0
 *
 * @brief Kernel's semaphore API.
 *
 * @details Kernel's semaphore API. This module implements the semaphore
 * management. The semaphore are used to synchronyse the threads. The semaphore
 * waiting list is a FIFO with no regards of the waiting threads priority.
 *
 * @warning Semaphores can only be used when the current system is running and
 * the scheduler initialized.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <panic.h>        /* Kernel panic */
#include <kfutex.h>       /* Kernel futex */
#include <kqueue.h>       /* Kernel queue */
#include <stdint.h>       /* Standard int types */
#include <stddef.h>       /* Standard definitions */
#include <atomic.h>       /* Kernel spinlock */
#include <critical.h>     /* Kernel critical section */
#include <scheduler.h>    /* Kernel scheduler */

/* Configuration files */
#include <config.h>

/* Header file */
#include <ksemaphore.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "KSEMAPHORE"

/** @brief Defines the maximal semaphore wake value */
#define SEMAPHORE_MAX_LEVEL 0x7FFFFFFF

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the semaphore to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the semaphore to ensure correctness of
 * execution. Due to the critical nature of the semaphore, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define SEMAPHORE_ASSERT(COND, MSG, ERROR) {                \
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

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
OS_RETURN_E ksemInit(ksemaphore_t*   pSem,
                     const int32_t  kInitLevel,
                     const uint32_t kFlags)
{

    /* Check parameters */
    if(pSem == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if((kFlags & KSEMAPHORE_FLAG_QUEUING_FIFO) ==
       KSEMAPHORE_FLAG_QUEUING_FIFO &&
       (kFlags & KSEMAPHORE_FLAG_QUEUING_PRIO) == KSEMAPHORE_FLAG_QUEUING_PRIO)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Setup the semaphore */
    if((kFlags & KSEMAPHORE_FLAG_BINARY) == KSEMAPHORE_FLAG_BINARY)
    {
        if(kInitLevel != 0)
        {
            pSem->level = 1;
        }
        else
        {
            pSem->level = 0;
        }
    }
    else
    {
        pSem->level = kInitLevel;
    }
    pSem->flags = kFlags;
    pSem->lockState = pSem->level > 0 ? 1 : 0;
    KERNEL_SPINLOCK_INIT(pSem->lock);

    /* Setup the futex */
    pSem->futex.pHandle = &pSem->lockState;
    pSem->futex.isAlive = true;
    if((kFlags & KSEMAPHORE_FLAG_QUEUING_PRIO) == KSEMAPHORE_FLAG_QUEUING_PRIO)
    {
        pSem->futex.queuingDiscipline = KFUTEX_FLAG_QUEUING_PRIO;
    }
    else
    {
        pSem->futex.queuingDiscipline = KFUTEX_FLAG_QUEUING_FIFO;
    }

    pSem->isInit = true;
    return OS_NO_ERR;
}

OS_RETURN_E ksemDestroy(ksemaphore_t* pSem)
{
    /* Check parameters */
    if(pSem == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(pSem->isInit == false)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Clear the semaphore and wakeup all threads */
    KERNEL_LOCK(pSem->lock);

    pSem->isInit = false;

    /* Release all waiting threads */
    pSem->lockState = 1;
    pSem->level = SEMAPHORE_MAX_LEVEL;
    kfutexWake(&pSem->futex, KFUTEX_MAX_WAIT_COUNT);
    pSem->futex.isAlive = false;

    KERNEL_UNLOCK(pSem->lock);

    return OS_NO_ERR;
}
OS_RETURN_E ksemWait(ksemaphore_t* pSem)
{
    OS_RETURN_E          error;
    uint32_t             intState;
    KFUTEX_WAKE_REASON_E wakeReason;

    /* Check parameters */
    if(pSem == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pSem->lock);

    if(pSem->isInit == false)
    {
        KERNEL_UNLOCK(pSem->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        return OS_ERR_INCORRECT_VALUE;
    }

    if(pSem->level > 0)
    {
        /* Acquire one value */
        --pSem->level;
        if(pSem->level <= 0)
        {
            pSem->lockState = 0;
        }
        KERNEL_UNLOCK(pSem->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);

        return OS_NO_ERR;
    }


    /* Wait on the futex an re-wait if canceled */
    do
    {
        KERNEL_UNLOCK(pSem->lock);
        error = kfutexWait(&pSem->futex, 0, &wakeReason);
        KERNEL_LOCK(pSem->lock);

        /* In the case the futex was woken up during the wait */
        if(error == OS_ERR_NOT_BLOCKED)
        {
            /* Check the lock state */
            if(pSem->lockState == 1 && pSem->level > 0)
            {
                /* Get the semaphore */
                error = OS_NO_ERR;
                wakeReason = KFUTEX_WAKEUP_WAKE;
            }
            else
            {
                /* Go for another round */
                error = OS_ERR_CANCELED;
                wakeReason = KFUTEX_WAKEUP_CANCEL;
            }
        }

    } while(error == OS_ERR_CANCELED &&
            wakeReason == KFUTEX_WAKEUP_CANCEL);

    /* Lock the semaphore */
    if(wakeReason == KFUTEX_WAKEUP_WAKE && pSem->isInit == true)
    {
        /* Acquire one value */
        if(pSem->level <= 0)
        {
            pSem->lockState = 0;
        }
        KERNEL_UNLOCK(pSem->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);

        return OS_NO_ERR;
    }
    else
    {
        KERNEL_UNLOCK(pSem->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        return OS_ERR_DESTROYED;
    }
}

OS_RETURN_E ksemPost(ksemaphore_t* pSem)
{
    uint32_t    intState;
    OS_RETURN_E error;
    /* Check parameters */
    if(pSem == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    KERNEL_ENTER_CRITICAL_LOCAL(intState);
    KERNEL_LOCK(pSem->lock);

    if(pSem->isInit == false)
    {
        KERNEL_UNLOCK(pSem->lock);
        KERNEL_EXIT_CRITICAL_LOCAL(intState);
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Give one value*/
    if(((pSem->flags & KSEMAPHORE_FLAG_BINARY) != KSEMAPHORE_FLAG_BINARY ||
        pSem->level <= 0) &&
        pSem->level < SEMAPHORE_MAX_LEVEL)
    {
        ++pSem->level;
    }

    if(pSem->level > 0)
    {
        pSem->lockState = 1;
        /* Release thread */
        error = kfutexWake(&pSem->futex, 1);

        if(error == OS_NO_ERR)
        {
            --pSem->level;
            if(pSem->level <= 0)
            {
                /* We wokeup a thread, set the sem as locked */
                pSem->lockState = 0;
            }
        }
        else if(error == OS_ERR_NO_SUCH_ID)
        {
            /* Nothing to wakeup, its ok */
            error = OS_NO_ERR;
        }
    }
    else
    {
        error = OS_NO_ERR;
    }

    SEMAPHORE_ASSERT(error == OS_NO_ERR,
                     "Error while releasing semaphore\n",
                     error);

    KERNEL_UNLOCK(pSem->lock);
    KERNEL_EXIT_CRITICAL_LOCAL(intState);

    return OS_NO_ERR;
}

OS_RETURN_E ksemTryWait(ksemaphore_t* pSem, int32_t* pValue)
{
    OS_RETURN_E error;

    /* Check parameters */
    if(pSem == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(pSem->isInit == false)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    KERNEL_LOCK(pSem->lock);
    if(pValue != NULL)
    {
        *pValue = pSem->level;
    }

    if(pSem->level > 0)
    {
        /* Acquire one value */
        --pSem->level;
        if(pSem->level <= 0)
        {
            pSem->lockState = 0;
        }
        error = OS_NO_ERR;
    }
    else
    {
        error = OS_ERR_BLOCKED;
    }

    KERNEL_UNLOCK(pSem->lock);

    return error;
}

/************************************ EOF *************************************/