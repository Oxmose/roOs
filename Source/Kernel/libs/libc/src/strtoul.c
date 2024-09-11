/*******************************************************************************
 * @file strtoul.c
 *
 * @see stdlib.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 08/01/2018
 *
 * @version 1.0
 *
 * @brief strtoul function. To be used with stdlib.h header.
 *
 * @details strtoul function. To be used with stdlib.h header.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <stdint.h> /* Generic integer definitions */

/* Configuration files */
#include <config.h>

/* Header file */
#include <stdlib.h>

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
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

unsigned long strtoul(const char *pStr, char **ppEnd, int base)
{
    unsigned long value;

    value = 0;
    while(*pStr >= '0' && *pStr <= '9')
    {
        value = (value * base) + (*pStr - '0');
        ++pStr;
    }

    if(ppEnd != NULL)
    {
        *ppEnd = (char*)pStr;
    }

    return value;
}

/************************************ EOF *************************************/