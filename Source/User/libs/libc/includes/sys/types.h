/*******************************************************************************
 * @file types.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/10/2024
 *
 * @version 1.0
 *
 * @brief Lib C types for roOs.
 *
 * @details Lib C types for roOs.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_SYS_TYPES_H_
#define __LIB_SYS_TYPES_H_

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
/** @brief Clock ID for the clock and timer functions */
typedef enum
{
    /**
     * @brief System-wide realtime clock. Setting this clock requires
     * appropriate privileges.
     */
    CLOCK_REALTIME = 0,
    /**
     * @brief Clock that cannot be set and represents monotonic time since some
     * unspecified starting point.
     */
    CLOCK_MONOTONIC = 1,
    /**
     * @brief High-resolution per-process timer from the CPU.
     */
    CLOCK_PROCESS_CPUTIME_ID = 2,
} clockid_t;

/** @brief pid_t is a type used for storing process IDs, process group IDs, and
 * session IDs. It is a signed integer type.
 */
typedef int pid_t;

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
/* None */

#endif /* #ifndef __LIB_SYS_TYPES_H_ */

/************************************ EOF *************************************/