/*******************************************************************************
 * @file memmove.c
 *
 * @see string.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 03/10/2017
 *
 * @version 1.0
 *
 * @brief memmove function. To be used with string.h header.
 *
 * @details memmove function. To be used with string.h header.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <stddef.h> /* Standard definitions */

/* Header file */
#include <string.h>

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

void *memmove(void *dst, const void *src, size_t n)
{
    const char *p = src;
    char *q = dst;
#if defined(__i386__) || defined(__x86_64__)
    if (q < p) {
        __asm__ __volatile__("cld ; rep ; movsb"
                 : "+c" (n), "+S"(p), "+D"(q));
    } else {
        p += (n - 1);
        q += (n - 1);
        __asm__ __volatile__("std ; rep ; movsb"
                 : "+c" (n), "+S"(p), "+D"(q));
    }
#else
    if (q < p) {
        while (n--) {
            *q++ = *p++;
        }
    } else {
        p += n;
        q += n;
        while (n--) {
            *--q = *--p;
        }
    }
#endif
    return dst;
}

/************************************ EOF *************************************/