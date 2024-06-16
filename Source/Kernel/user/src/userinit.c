/*******************************************************************************
 * @file userinit.c
 *
 * @see userinit.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <kernelshell.h> /* User kernel's shell */

/* Header file */
#include <userinit.h>

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

void userInit(void)
{
    /* Initialize the kernel shell */
    kernelShellInit();
}

/************************************ EOF *************************************/