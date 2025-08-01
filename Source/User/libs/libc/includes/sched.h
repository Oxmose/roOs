/*******************************************************************************
 * @file sched.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/10/2024
 *
 * @version 1.0
 *
 * @brief Scheduler port for roOs.
 *
 * @details Scheduler port for roOs. This port is not intended to be complete
 * and provides API for roOs.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_SCHED_H_
#define __LIB_SCHED_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
/* None */

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
 * @brief Causes the calling thread to relinquish the CPU.
 *
 * @details Causes the calling thread to relinquish the CPU. The thread is moved
 * to the end of the queue for its static priority and a new thread gets to run.
 *
 * @return The function returns 0 for success, or -1 for failure (in which case
 * errno is set appropriately).
 */
int sched_yield(void);

#endif /* #ifndef __LIB_SCHED_H_ */

/************************************ EOF *************************************/