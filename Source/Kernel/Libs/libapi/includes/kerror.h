/*******************************************************************************
 * @file kerror.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief Kernel error definitions.
 *
 * @details Kernel error definitions. Contains the UTK error codes definition.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_KERROR_H_
#define __LIB_KERROR_H_

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

/** @brief System return states enumeration. */
typedef enum
{
    /** @brief No error occured. */
    OS_NO_ERR                              = 0,
    /** @brief A null pointer was detected. */
    OS_ERR_NULL_POINTER                    = 1,
    /** @brief Unauthorized action from kernel. */
    OS_ERR_UNAUTHORIZED_ACTION             = 2,
    /** @brief Unauthorized interrupt line used. */
    OR_ERR_UNAUTHORIZED_INTERRUPT_LINE     = 3,
    /** @brief Interrupt handler was already registered. */
    OS_ERR_INTERRUPT_ALREADY_REGISTERED    = 4,
    /** @brief Interrupt is not registered. */
    OS_ERR_INTERRUPT_NOT_REGISTERED        = 5,
    /** @brief Unknown IRQ. */
    OS_ERR_NO_SUCH_IRQ                     = 6,
    /** @brief Missing required memory */
    OS_ERR_NO_MORE_MEMORY                  = 7,
} OS_RETURN_E;

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

#endif /* #ifndef __LIB_KERROR_H_ */

/************************************ EOF *************************************/

