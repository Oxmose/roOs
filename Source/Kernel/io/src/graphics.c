/*******************************************************************************
 * @file graphics.c
 *
 * @see graphics.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <vfs.h>       /* Virtual Filesystem */
#include <ioctl.h>     /* IOCTL commands */
#include <string.h>    /* String manipulation */
#include <stdint.h>    /* Generic int types and bool_t */
#include <stddef.h>    /* Standard definition */
#include <syslog.h>    /* Syslog service */
#include <kerror.h>    /* Kernel error codes */
#include <devtree.h>   /* Device tree manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <graphics.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the current module name */
#define MODULE_NAME "GRAPHICS"

/** @brief FDT graphics node name */
#define FDT_GRAPHICS_NODE_NAME "graphics"

/** @brief FDT property for the graphics output device property */
#define FDT_GRAPHICS_OUTPUT_DEV_PROP "outputdev"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Execute a function if it exists.
 *
 * @param[in] FUNC The function to execute.
 * @param[in, out] ... Function parameters.
 */
#define EXEC_IF_SET(DRIVER, FUNC, ...) { \
    if(DRIVER.FUNC != NULL)              \
    {                                    \
        DRIVER.FUNC(__VA_ARGS__);        \
    }                                    \
}

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
/** @brief Stores the graphic device file descriptor */
static int32_t sGraphicDev = -1;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void graphicsInit(void)
{
    const fdt_node_t* kpGraphicsNode;
    const char*       kpStrProp;
    size_t            propLen;

    /* Get the FDT graphics node */
    kpGraphicsNode = fdtGetNodeByName(FDT_GRAPHICS_NODE_NAME);
    if(kpGraphicsNode == NULL)
    {
        return;
    }

    kpStrProp = fdtGetProp(kpGraphicsNode,
                           FDT_GRAPHICS_OUTPUT_DEV_PROP,
                           &propLen);
    if(kpStrProp != NULL && propLen > 0)
    {
        sGraphicDev = vfsOpen(kpStrProp, O_RDWR, 0);
        if(sGraphicDev < 0)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failed to open graphic device");
        }
    }
}

OS_RETURN_E graphicsDrawPixel(const uint32_t kX,
                              const uint32_t kY,
                              const uint32_t kRGBPixel)
{
    int32_t                    retVal;
    graph_ioctl_args_drawpixel ioctlArgs;

    if(sGraphicDev >= 0)
    {
        ioctlArgs.x = kX;
        ioctlArgs.y = kY;
        ioctlArgs.rgbPixel = kRGBPixel;
        retVal = vfsIOCTL(sGraphicDev, VFS_IOCTL_GRAPH_DRAWPIXEL, &ioctlArgs);
    }
    else
    {
        return OS_ERR_NOT_SUPPORTED;
    }

    if(retVal < 0)
    {
        return OS_ERR_INCORRECT_VALUE;
    }

    return OS_NO_ERR;
}

/************************************ EOF *************************************/