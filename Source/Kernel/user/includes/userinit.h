/*******************************************************************************
 * @file userinit.h
 *
 * @see userinit.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 16/06/2024
 *
 * @version 1.0
 *
 * @brief Kernel's user entry point.
 *
 * @details Kernel's user entry point. This file gather the functions called
 * by the kernel just before starting the scheduler and executing the tast.
 * Users can use this function to add relevant code to their applications'
 * initialization or for other purposes.
 *
 * @warning All interrupts are disabled when calling the user initialization
 * functions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __USER_USERINIT_H_
#define __USER_USERINIT_H_

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
 * @brief Kernel's user entry point.
 *
 * @details Kernel's user entry point. This functions is called
 * by the kernel just before starting the scheduler and executing the tast.
 * Users can use this function to add relevant code to their applications'
 * initialization or for other purposes.
 */
void userInit(void);


#endif /* #ifndef __USER_USERINIT_H_ */

/************************************ EOF *************************************/