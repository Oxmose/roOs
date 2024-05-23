/*******************************************************************************
 * @file drivermgr.c
 *
 * @see drivermgr.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 25/05/2024
 *
 * @version 1.0
 *
 * @brief Kernel's driver and device manager.
 *
 * @details Kernel's driver and device manager. Used to register, initialize and
 * manage the drivers used in the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Includes */
#include <kerror.h>        /* Kernel error codes */
#include <string.h>        /* String manipulation */
#include <devtree.h>       /* FDT library */
#include <kerneloutput.h> /* Kernel output services */

/* Configuration files */
#include <config.h>

/* Header file */
#include <drivermgr.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "DRIVER_MGR"

/** @brief Compatible property name in FDT */
#define COMPATIBLE_PROP_NAME "compatible"

/** @brief Status property name in FDT */
#define STATUS_PROP_NAME "status"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the driver manager to ensure correctness of
 * execution.
 *
 * @details Assert macro used by the driver manager to ensure correctness of
 * execution. Due to the critical nature of the driver manager, any error
 * generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define DRVMGR_ASSERT(COND, MSG, ERROR) {                   \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG, TRUE);               \
    }                                                       \
}

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Start of the registered kernel drivers table */
extern uintptr_t _START_DRV_TABLE_ADDR;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

static void walkFdtNodes(const fdt_node_t* pkNode);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void walkFdtNodes(const fdt_node_t* pkNode)
{
    const char* pCompatible;
    const char* pStatus;
    driver_t*   pDriver;
    uintptr_t   driverTableCursor;
    size_t      propLen;
    OS_RETURN_E retCode;

    if(pkNode == NULL)
    {
        return;
    }

    /* Manage disabled nodes */
    pStatus = fdtGetProp(pkNode, STATUS_PROP_NAME, &propLen);
    if(pStatus == NULL || (propLen == 5 && strcmp(pStatus, "okay") == 0))
    {
        /* Get the node compatible */
        pCompatible = fdtGetProp(pkNode, COMPATIBLE_PROP_NAME, &propLen);
        if(pCompatible != NULL && propLen > 0)
        {
            KERNEL_DEBUG(DEVMGR_DEBUG_ENABLED,
                        MODULE_NAME,
                        "Detected %s",
                        pCompatible);

            /* Get the head of the registered drivers section */
            driverTableCursor = (uintptr_t)&_START_DRV_TABLE_ADDR;
            pDriver = *(driver_t**)driverTableCursor;

            /* Compare with the list of registered drivers */
            while(pDriver != NULL)
            {
                if(strcmp(pDriver->pCompatible, pCompatible) == 0)
                {
                    retCode = pDriver->pDriverAttach(pkNode);
                    if(retCode == OS_NO_ERR)
                    {
                        KERNEL_SUCCESS("%s attached successfully.\n",
                                       pDriver->pName);
                        /* TODO: Add to device list*/
                    }
                    else
                    {
                        KERNEL_ERROR("Failed to attach driver %s. Error %d\n",
                                    pDriver->pName,
                                    retCode);
                    }
                    break;
                }
                else
                {
                    KERNEL_DEBUG(DEVMGR_DEBUG_ENABLED,
                                MODULE_NAME,
                                "%s not compatible with %s.",
                                pDriver->pCompatible,
                                pCompatible);
                }
                driverTableCursor += sizeof(uintptr_t);
                pDriver = *(driver_t**)driverTableCursor;
            }
        }
    }

    /* Got to next nodes */
    walkFdtNodes(fdtGetChild(pkNode));
    walkFdtNodes(fdtGetNextNode(pkNode));
}

void driverManagerInit(void)
{
    const fdt_node_t* pFdtRootNode;

#if DEVMGR_DEBUG_ENABLED
    driver_t* pDriver;
    uintptr_t driverTableCursor;

    /* Display list of registered drivers */
    driverTableCursor = (uintptr_t)&_START_DRV_TABLE_ADDR;
    pDriver = *(driver_t**)driverTableCursor;
    KERNEL_DEBUG(DEVMGR_DEBUG_ENABLED, MODULE_NAME, "List of drivers");
    while(pDriver != NULL)
    {
        KERNEL_DEBUG(DEVMGR_DEBUG_ENABLED,
                     MODULE_NAME,
                     "%s - %s",
                     pDriver->pName,
                     pDriver->pDescription);
        driverTableCursor += sizeof(uintptr_t);
        pDriver = *(driver_t**)driverTableCursor;
    }

    KERNEL_DEBUG(DEVMGR_DEBUG_ENABLED, MODULE_NAME, "------------------------");
#endif

    /* Get the FDT root node and walk it to register drivers */
    pFdtRootNode = fdtGetRoot();
    if(pFdtRootNode == NULL)
    {
        KERNEL_ERROR("Failed to get FDT root node in driver manager.\n");
        return;
    }

    /* Perform the registration */
    walkFdtNodes(pFdtRootNode);
}

/************************************ EOF *************************************/