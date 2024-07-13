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
#include <stdint.h>    /* Generic int types and bool_t */
#include <stddef.h>    /* Standard definition */
#include <kerror.h>    /* Kernel error codes */
#include <devtree.h>   /* Device tree manager */
#include <drivermgr.h> /* Driver manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <graphics.h>

/* Tracing feature */
#include <tracing.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

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
/** @brief Stores the currently selected driver */
static graphics_driver_t sDriver = {NULL};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void graphicsInit(void)
{
    const fdt_node_t*  kpGraphicsNode;
    const uint32_t*    kpUintProp;
    size_t             propLen;
    graphics_driver_t* pGraphicsDriver;
    kgeneric_driver_t* pGenDriver;

    KERNEL_TRACE_EVENT(TRACE_GRAPHICS_ENABLED, TRACE_GRAPHICS_INIT_ENTRY, 0);

    /* Get the FDT graphics node */
    kpGraphicsNode = fdtGetNodeByName(FDT_GRAPHICS_NODE_NAME);
    if(kpGraphicsNode == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_GRAPHICS_ENABLED,
                           TRACE_GRAPHICS_INIT_EXIT,
                           1,
                           -1);
        return;
    }

    /* Get the output driver */
    kpUintProp = fdtGetProp(kpGraphicsNode,
                            FDT_GRAPHICS_OUTPUT_DEV_PROP,
                            &propLen);
    if(kpUintProp != NULL && propLen == sizeof(uint32_t))
    {
        pGenDriver = driverManagerGetDeviceData(FDTTOCPU32(*kpUintProp));

        if(pGenDriver != NULL)
        {
            pGraphicsDriver = pGenDriver->pGraphicsDriver;
            if(pGraphicsDriver != NULL && pGraphicsDriver->pDriverCtrl != NULL)
            {
                sDriver = *pGraphicsDriver;
            }
        }
    }

    KERNEL_TRACE_EVENT(TRACE_GRAPHICS_ENABLED, TRACE_GRAPHICS_INIT_EXIT, 1, 0);
}

OS_RETURN_E graphicsDrawPixel(const uint32_t kX,
                              const uint32_t kY,
                              const uint32_t kRGBPixel)
{
    OS_RETURN_E error;

    if(sDriver.pDrawPixel != NULL)
    {
        error = sDriver.pDrawPixel(sDriver.pDriverCtrl, kX, kY, kRGBPixel);
    }
    else
    {
        error = OS_ERR_NULL_POINTER;
    }

    return error;
}

/************************************ EOF *************************************/