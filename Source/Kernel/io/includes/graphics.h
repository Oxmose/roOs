/*******************************************************************************
 * @file graphics.h
 *
 * @see graphics.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 10/07/2024
 *
 * @version 1.0
 *
 * @brief Graphics display drivers abtraction.
 *
 * @details  Graphics display drivers abtraction. This module allows to write
 * to a screen using a graphic driver.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __IO_GRAPHICS_H_
#define __IO_GRAPHICS_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h> /* Generic int types and bool_t */
#include <stddef.h> /* Standard definition */
#include <kerror.h> /* Kernel error codes */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the IOCTL arguments for a draw pixel operation. */
typedef struct
{
    /** @brief X position */
    uint32_t x;

    /** @brief Y position */
    uint32_t y;

    /** @brief RGB pixel value */
    uint32_t rgbPixel;
} graph_ioctl_args_drawpixel;

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
 * @brief Initializes the graphics library.
 *
 * @details Initializes the graphics library. On error, this function fails
 * silently.
 */
void graphicsInit(void);

/**
 * @brief Draws a 32bits color pixel to the graphics controller.
 *
 * @details Draws a 32bits color pixel to the graphics controller. The driver
 * handles the overflow and might not print the pixel if the parameters are
 * invalid.
 *
 * @param[in] x The horizontal position where to draw the pixel.
 * @param[in] y The vertical position where to draw the pixel.
 * @param[in] rgbPixel The 32bits color pixel to draw.
 *
 * @return The success or error status is returned.
 */
OS_RETURN_E graphicsDrawPixel(const uint32_t kX,
                              const uint32_t kY,
                              const uint32_t kRGBPixel);

#endif /* #ifndef __IO_GRAPHICS_H_ */

/************************************ EOF *************************************/