/*******************************************************************************
 * @file stdbool.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 05/10/2023
 *
 * @version 1.0
 *
 * @brief Kernel's bool type.
 *
 * @details Define basics bool types for the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/


#ifndef __LIB_STDBOOL_H_
#define __LIB_STDBOOL_H_


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


#if defined(__GNUC__) || \
    (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901)

/** @brief False value for the bool type. */
#define false	0
/** @brief True value for the bool type. */
#define true	1

#else

/** @brief Boolean primitive type. */
typedef enum
{
    /** @brief False value for the bool type. */
	false = 0,
    /** @brief True value for the bool type. */
	true = 1
} _Bool;

/** @brief False constant. */
#define	false false
/** @brief True constant. */
#define	true true

#endif

/** @brief Defines the bool type. */
#define bool _Bool

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

#endif /* #ifndef __LIB_STDBOOL_H_ */

/************************************ EOF *************************************/