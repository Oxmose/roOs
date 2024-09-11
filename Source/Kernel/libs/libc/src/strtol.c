/*******************************************************************************
 * @file strtol.c
 *
 * @see stdlib.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 08/01/2018
 *
 * @version 1.0
 *
 * @brief strtol function. To be used with stdlib.h header.
 *
 * @details strtol function. To be used with stdlib.h header.
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

long strtol(const char *pStr, char **ppEnd, int base)
{
    long value;
    long sign;
    const char* newStr;

    /* If base is unknown just return */
    if (base > 16)
    {
        if(ppEnd != NULL)
        {
            *ppEnd = (char*)pStr;
        }
        return 0;
    }

    /* Check sign */
    if (pStr[0] == '-')
    {
        sign = -1;
        newStr = pStr + 1;
    }
    else
    {
        sign = 1;
        newStr = pStr;
    }

    value = (long)strtoul(pStr, ppEnd, base);
    if(ppEnd != NULL)
    {
        if(*ppEnd == newStr)
        {
            *ppEnd = (char*)pStr;
        }
    }

    return value * sign;
}

/************************************ EOF *************************************/