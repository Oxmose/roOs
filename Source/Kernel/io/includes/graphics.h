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

/** @brief Defines a rectangle for the graphic driver */
typedef struct
{
    /** @brief X start position */
    uint32_t x;

    /** @brief Y start position */
    uint32_t y;

    /** @brief Rectangle width */
    uint32_t width;

    /** @brief Rectangle height */
    uint32_t height;

    /** @brief Rectangle color */
    uint32_t color;
} graph_rect_t;

/** @brief Defines a line for the graphic driver */
typedef struct
{
    /** @brief X start position */
    uint32_t xStart;

    /** @brief Y start position */
    uint32_t yStart;

    /** @brief X end position */
    uint32_t xEnd;

    /** @brief Y end position */
    uint32_t yEnd;

    /** @brief Line color */
    uint32_t color;
} graph_line_t;

/** @brief Defines a bitmap for the graphic driver */
typedef struct
{
    /** @brief X start position */
    uint32_t x;

    /** @brief Y start position */
    uint32_t y;

    /** @brief Bitmap width */
    uint32_t width;

    /** @brief Bitmap height */
    uint32_t height;

    /** @brief Bitmap data */
    const uint32_t* kpData;
} graph_bitmap_t;

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
 * @param[in] kX The horizontal position where to draw the pixel.
 * @param[in] kY The vertical position where to draw the pixel.
 * @param[in] kRGBPixel The 32bits color pixel to draw.
 *
 * @return The success or error status is returned.
 */
OS_RETURN_E graphicsDrawPixel(const uint32_t kX,
                              const uint32_t kY,
                              const uint32_t kRGBPixel);

/**
 * @brief Draws a 32bits color rectangle to the graphics controller.
 *
 * @details Draws a 32bits color rectangle to the graphics controller. The
 * driver handles the overflow and might not print the rectangle if the
 * parameters are invalid.
 *
 * @param[in] kX The horizontal position where to draw the rectangle.
 * @param[in] kY The vertical position where to draw the rectangle.
 * @param[in] kWidth The width of the rectangle.
 * @param[in] kHeight The height of the rectangle.
 * @param[in] kColor The 32bits color rectangle to draw.
 *
 * @return The success or error status is returned.
 */
OS_RETURN_E graphicsDrawRectangle(const uint32_t kX,
                                  const uint32_t kY,
                                  const uint32_t kWidth,
                                  const uint32_t kHeight,
                                  const uint32_t kColor);

/**
 * @brief Draws a 32bits color line to the graphics controller.
 *
 * @details Draws a 32bits color line to the graphics controller. The
 * driver handles the overflow and might not print the line if the
 * parameters are invalid.
 *
 * @param[in] kStartX The horizontal position where start the line.
 * @param[in] kStartY The vertical position where start the line.
 * @param[in] kEndX The horizontal position where to end the line.
 * @param[in] kEndY The vertical position where to end the line.
 * @param[in] kColor The 32bits color line to draw.
 *
 * @return The success or error status is returned.
 */
OS_RETURN_E graphicsDrawLine(const uint32_t kStartX,
                             const uint32_t kStartY,
                             const uint32_t kEndX,
                             const uint32_t kEndY,
                             const uint32_t kColor);

/**
 * @brief Draws a 32bits color bitmap to the graphics controller.
 *
 * @details Draws a 32bits color bitmap to the graphics controller. The driver
 * handles the overflow and might not print the bitmap if the parameters are
 * invalid.
 *
 * @param[in] kX The horizontal position where to draw the bitmap.
 * @param[in] kY The vertical position where to draw the bitmap.
 * @param[in] kWidth The width of the bitmap.
 * @param[in] kHeight The height of the bitmap.
 * @param[in] kpBuffer The 32bits color bitmap data.
 *
 * @return The success or error status is returned.
 */
OS_RETURN_E graphicsDrawBitmap(const uint32_t  kX,
                               const uint32_t  kY,
                               const uint32_t  kWidth,
                               const uint32_t  kHeight,
                               const uint32_t* kpBuffer);

#endif /* #ifndef __IO_GRAPHICS_H_ */

/************************************ EOF *************************************/