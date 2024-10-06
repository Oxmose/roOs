/*******************************************************************************
 * @file ksemaphore.h
 *
 * @see ksemaphore.c
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

#ifndef __SYNC_KSEMAPHORE_H_
#define __SYNC_KSEMAPHORE_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <kqueue.h>   /* Kernel queues */
#include <kfutex.h>   /* Kernel futex */
#include <stdint.h>   /* Standard int definitions */
#include <kerror.h>   /* Kernel error */
#include <atomic.h>   /* Spinlock structure */
#include <stdbool.h>  /* Bool types */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Semaphore flag: semaphore has FIFO queuing discipline. */
#define KSEMAPHORE_FLAG_QUEUING_FIFO 0x00000001

/** @brief Semaphore flag: semaphore has priority based queuing discipline. */
#define KSEMAPHORE_FLAG_QUEUING_PRIO 0x00000002

/** @brief Semaphore flag: binary semaphore. */
#define KSEMAPHORE_FLAG_BINARY 0x00000004

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Semaphore structure definition. */
typedef struct
{
    /** @brief Semaphore level counter */
    volatile int32_t level;

    /** @brief Semaphore lock state */
    volatile int32_t lockState;

    /** @brief Semaphore associated futex */
    kfutex_t futex;

    /** @brief Initialization state */
    bool isInit;

    /** @brief Semaphore flags */
    uint32_t flags;

    /** @brief Semaphore lock. */
    kernel_spinlock_t lock;
} ksemaphore_t;

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
 * @brief Initializes the semaphore structure.
 *
 * @details Initializes the semaphore structure. The initial state of a
 * semaphore is given by the init_level parameter.
 *
 * @param[out] pSem The pointer to the semaphore to initialize.
 * @param[in] kInitLevel The initial value to set the semaphore with.
 * @param[in] kFlags The semaphore flags to be used.
 *
 * @return The success state or the error code.
 */
OS_RETURN_E ksemInit(ksemaphore_t*   pSem,
                     const int32_t  kInitLevel,
                     const uint32_t kFlags);

/**
 * @brief Destroys the semaphore given as parameter.
 *
 * @details Destroys the semaphore given as parameter. Also unlock all the
 * threads locked on this semaphore. Using a destroyed semaphore produces
 * undefined behaviors.
 *
 * @param[in, out] pSem The semaphore to destroy.
 *
 * @return The success state or the error code.
 *
 * @warning Using a non-initialized or destroyed semaphore produces undefined
 * behavior.
 */
OS_RETURN_E ksemDestroy(ksemaphore_t* pSem);

/**
 * @brief Pends on the semaphore given as parameter.
 *
 * @details Pends on the semaphore given as parameter. The calling thread will
 * block on this call until the semaphore is aquired.
 *
 * @param[in] pSem The semaphore to pend.
 *
 * @return The success state or the error code.
 *
 * @warning Using a non-initialized or destroyed semaphore produces undefined
 * behavior.
 */
OS_RETURN_E ksemWait(ksemaphore_t* pSem);

/**
 * @brief Post the semaphore given as parameter.
 *
 * @param[in] pSem The semaphore to post.
 *
 * @return The success state or the error code.
 *
 * @warning Using a non-initialized or destroyed semaphore produces undefined
 * behavior.
 */
OS_RETURN_E ksemPost(ksemaphore_t* pSem);

/**
 * @brief Try to pend on the semaphore given as parameter.
 *
 * @details Try to pend on the mutex semaphore as parameter. The function will
 * return the current semaphore level. If possible the function will
 * aquire the semaphore.
 *
 * @param[in] pSem The semaphore to pend.
 * @param[out] pValue The buffer that receives the semaphore level.
 *
 * @return The success state or the error code.
 *
 * @warning Using a non-initialized or destroyed semaphore produces undefined
 * behavior.
 */
OS_RETURN_E ksemTryWait(ksemaphore_t* pSem, int32_t* pValue);

#endif /* #ifndef __SYNC_KSEMAPHORE_H_ */

/************************************ EOF *************************************/