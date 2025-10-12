/*******************************************************************************
 * @file time.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/10/2024
 *
 * @version 1.0
 *
 * @brief Time port for roOs.
 *
 * @details Time port for roOs. This port is not inteded to be complete and
 * provides API for roOs.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_TIME_H_
#define __LIB_TIME_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stddef.h>    /* Standard definitions */
#include <sys/types.h> /* System types */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Real arithmetic type capable of representing times. */
typedef int64_t time_t;

/**
 * @brief Structure holding an interval broken down into seconds and
 * nanoseconds.
 */
struct timespec
{
    /** @brief Whole seconds (valid values are >= 0) */
    time_t tv_sec;

    /** @brief Nanoseconds (valid values are [0, 999999999]) */
    long int tv_nsec;
};

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
 * @brief Writes to a file descriptor.
 *
 * @details Writes up to count bytes from the buffer starting at pBuffer to the
 * file referred to by the file descriptor fileFd.
 *
 * @param[in] clk_id The identifier of the particular clock on which to act.
 * @param[in] tp The timespec structure to fill with the time.
 *
 * @return The function returns 0 for success, or -1 for failure (in which case
 * errno is set appropriately).
 */
int clock_gettime(clockid_t clk_id, struct timespec *tp);


/**
 * @brief Suspends the execution of the calling thread.
 *
 * @details Suspends the execution of the calling thread until
 * either at least the time specified in *duration has elapsed, or
 * the delivery of a signal that triggers the invocation of a
 * handler in the calling thread or that terminates the process.
 *
 * @param[in] duration It is used to specify intervals of time with nanosecond
 * precision.
 * @param[out] rem Can be NULL, can then be used to call nanosleep() again and
 * complete an uncomplete pause.
 *
 * @return The function returns 0 for success, or -1 for failure (in which case
 * errno is set appropriately).
 */
int nanosleep(const struct timespec *duration,
              struct timespec* rem);

#endif /* #ifndef __LIB_TIME_H_ */

/************************************ EOF *************************************/