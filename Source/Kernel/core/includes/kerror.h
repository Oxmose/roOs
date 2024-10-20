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
 * @details Kernel error definitions. Contains the roOs error codes definition.
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
    /** @brief The resource already exists. */
    OS_ERR_ALREADY_EXIST                   = 4,
    /** @brief Unknown IRQ. */
    OS_ERR_NO_SUCH_IRQ                     = 5,
    /** @brief Missing required memory */
    OS_ERR_NO_MORE_MEMORY                  = 6,
    /** @brief Invalid value used */
    OS_ERR_INCORRECT_VALUE                 = 7,
    /** @brief Value out of bound */
    OS_ERR_OUT_OF_BOUND                    = 8,
    /** @brief Feature not supported */
    OS_ERR_NOT_SUPPORTED                   = 9,
    /** @brief Identifier not found */
    OS_ERR_NO_SUCH_ID                      = 10,
    /** @brief The resource was destroyed */
    OS_ERR_DESTROYED                       = 11,
    /** @brief The resource was not blocked */
    OS_ERR_NOT_BLOCKED                     = 12,
    /** @brief The resource was blocked */
    OS_ERR_BLOCKED                         = 13,
    /** @brief Page fault occured */
    OS_ERR_PAGE_FAULT                      = 14,
    /** @brief Canceled action */
    OS_ERR_CANCELED                        = 15,
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

