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
#include <syslog.h>       /* Kernel syslog */
#include <kerror.h>       /* Kernel error codes */
#include <string.h>       /* String manipulation */
#include <syslog.h>       /* Kernel Syslog */
#include <devtree.h>      /* FDT library */

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
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Walks the FDT nodes starting from the current node.
 *
 * @details Walks the FDT nodes starting from the current node. The walk is
 * performed as a depth first search walk. For each node, the compatible is
 * compared to the list of registered drivers. If a driver is compatible, its
 * attach function is called.
 *
 * @param[in] pkNode The node to start the walk from.
 */
static void _walkFdtNodes(const fdt_node_t* pkNode);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Start address of the registered kernel drivers table */
extern uintptr_t _START_DRV_TABLE_ADDR;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _walkFdtNodes(const fdt_node_t* pkNode)
{
    const char* kpCompatible;
    const char* kpStatus;
    driver_t*   pDriver;
    uintptr_t   driverTableCursor;
    size_t      propLen;
    OS_RETURN_E retCode;

    if(pkNode == NULL)
    {
        return;
    }

    /* Manage disabled nodes */
    kpStatus = fdtGetProp(pkNode, STATUS_PROP_NAME, &propLen);
    if(kpStatus == NULL || (propLen == 5 && strcmp(kpStatus, "okay") == 0))
    {
        /* Get the node compatible */
        kpCompatible = fdtGetProp(pkNode, COMPATIBLE_PROP_NAME, &propLen);
        if(kpCompatible != NULL && propLen > 0)
        {

#if DEVMGR_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Detected %s",
                   kpCompatible);
#endif

            /* Get the head of the registered drivers section */
            driverTableCursor = (uintptr_t)&_START_DRV_TABLE_ADDR;
            pDriver = *(driver_t**)driverTableCursor;

            /* Compare with the list of registered drivers */
            while(pDriver != NULL)
            {
                if(strcmp(pDriver->pCompatible, kpCompatible) == 0)
                {
                    retCode = pDriver->pDriverAttach(pkNode);
                    if(retCode == OS_NO_ERR)
                    {
                        syslog(SYSLOG_LEVEL_INFO,
                               MODULE_NAME,
                               "%s attached successfully.",
                               pDriver->pName);
                    }
                    else
                    {
                        syslog(SYSLOG_LEVEL_ERROR,
                               MODULE_NAME,
                               "Failed to attach driver %s. Error %d",
                               pDriver->pName,
                               retCode);
                    }
                    break;
                }
                driverTableCursor += sizeof(uintptr_t);
                pDriver = *(driver_t**)driverTableCursor;
            }

            if(pDriver == NULL)
            {
                syslog(SYSLOG_LEVEL_ERROR,
                       MODULE_NAME,
                       "Driver for %s not found",
                       kpCompatible);
            }
        }
    }

    /* Got to next nodes */
    _walkFdtNodes(fdtGetChild(pkNode));
    _walkFdtNodes(fdtGetNextNode(pkNode));
}

void driverManagerInit(void)
{
    const fdt_node_t* kpFdtRootNode;
    driver_t*         pDriver;
    uintptr_t         driverTableCursor;

    /* Display list of registered drivers */
    driverTableCursor = (uintptr_t)&_START_DRV_TABLE_ADDR;
    pDriver = *(driver_t**)driverTableCursor;

    while(pDriver != NULL)
    {
        syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "%s - %s",
                    pDriver->pName,
                    pDriver->pDescription);
        driverTableCursor += sizeof(uintptr_t);
        pDriver = *(driver_t**)driverTableCursor;
    }

    /* Get the FDT root node and walk it to register drivers */
    kpFdtRootNode = fdtGetRoot();
    if(kpFdtRootNode == NULL)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to get FDT root node in driver manager.");
        return;
    }

    /* Perform the registration */
    _walkFdtNodes(kpFdtRootNode);
}

OS_RETURN_E driverManagerSetDeviceData(const fdt_node_t* pkFdtNode,
                                       void*             pData)
{
    OS_RETURN_E retCode;

    /* Check parameters */
    if(pkFdtNode != NULL)
    {
        /* Not clean, but this avoids a lot of function calls and
         * processing.
         */
        ((fdt_node_t*)pkFdtNode)->pDevData = pData;
        retCode = OS_NO_ERR;
    }
    else
    {
        retCode = OS_ERR_NULL_POINTER;
    }

    return retCode;
}

void* driverManagerGetDeviceData(const uint32_t kHandle)
{
    void*             pDevData;
    const fdt_node_t* kpNode;

    /* Get the node and return the device data */
    pDevData = NULL;
    kpNode = fdtGetNodeByHandle(kHandle);
    if(kpNode != NULL)
    {
        pDevData = kpNode->pDevData;
    }

    return pDevData;
}

/************************************ EOF *************************************/