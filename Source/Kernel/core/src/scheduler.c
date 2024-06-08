/*******************************************************************************
 * @file scheduler.c
 *
 * @see scheduler.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 16/12/2017
 *
 * @version 3.0
 *
 * @brief Kernel's thread scheduler.
 *
 * @details Kernel's thread scheduler. Thread creation and management functions
 * are located in this file.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <cpu.h>        /* CPU API */
#include <ctrl_block.h> /* Threads and processes control block */

/* Configuration files */
#include <config.h>

/* Header file */
#include <scheduler.h>

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
 * @brief Assert macro used by the scheduler to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the scheduler to ensure correctness of
 * execution. Due to the critical nature of the scheduler, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define SCHED_ASSERT(COND, MSG, ERROR) {                    \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, "SCHED", MSG, TRUE);                   \
    }                                                       \
}

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/

/** @brief Pointer to the currently kernel thread */
kernel_thread_t* pCurrentThread[MAX_CPU_COUNT] = {NULL};

/* TODO: Remove */
kernel_thread_t dummy_thread[MAX_CPU_COUNT];

uint8_t vcpubuffer[MAX_CPU_COUNT * 72];

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

/* TODO: Remove */
void scheduler_dummy_init(void);
void scheduler_dummy_init(void)
{
    uint32_t i;
    for(i = 0; i < MAX_CPU_COUNT; ++i)
    {
        pCurrentThread[i] = &dummy_thread[i];
        dummy_thread[i].pVCpu = vcpubuffer + 72 * i;
    }
}

kernel_thread_t* schedGetCurrentThread(void)
{
    uint8_t cpuId;
    cpuId = cpuGetId();
    return pCurrentThread[cpuId];
}

/************************************ EOF *************************************/