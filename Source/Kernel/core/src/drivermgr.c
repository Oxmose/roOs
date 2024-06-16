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
#include <kerror.h>       /* Kernel error codes */
#include <string.h>       /* String manipulation */
#include <devtree.h>      /* FDT library */
#include <kerneloutput.h> /* Kernel output services */

/* Configuration files */
#include <config.h>

/* Header file */
#include <drivermgr.h>

/* Tracing feature */
#include <tracing.h>

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
            KERNEL_DEBUG(DEVMGR_DEBUG_ENABLED,
                        MODULE_NAME,
                        "Detected %s",
                        kpCompatible);

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
                        KERNEL_SUCCESS("%s attached successfully.\n",
                                       pDriver->pName);
                    }
                    else
                    {
                        KERNEL_ERROR("Failed to attach driver %s. Error %d\n",
                                    pDriver->pName,
                                    retCode);
                    }
                    break;
                }
                driverTableCursor += sizeof(uintptr_t);
                pDriver = *(driver_t**)driverTableCursor;
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

    KERNEL_TRACE_EVENT(TRACE_DRVMGR_ENABLED, TRACE_DRV_MGR_INIT_ENTRY, 0);

    /* Display list of registered drivers */
    driverTableCursor = (uintptr_t)&_START_DRV_TABLE_ADDR;
    pDriver = *(driver_t**)driverTableCursor;
    KERNEL_INFO("List of registered drivers\n");
    while(pDriver != NULL)
    {
        KERNEL_INFO("%s - %s\n",
                    pDriver->pName,
                    pDriver->pDescription);
        driverTableCursor += sizeof(uintptr_t);
        pDriver = *(driver_t**)driverTableCursor;
    }

    KERNEL_INFO("------------------------\n");

    /* Get the FDT root node and walk it to register drivers */
    kpFdtRootNode = fdtGetRoot();
    if(kpFdtRootNode == NULL)
    {
        KERNEL_TRACE_EVENT(TRACE_DRVMGR_ENABLED, TRACE_DRV_MGR_INIT_EXIT, -1);
        KERNEL_ERROR("Failed to get FDT root node in driver manager.\n");
        return;
    }

    /* Perform the registration */
    _walkFdtNodes(kpFdtRootNode);

    KERNEL_TRACE_EVENT(TRACE_DRVMGR_ENABLED, TRACE_DRV_MGR_INIT_EXIT, 0);
}

OS_RETURN_E driverManagerSetDeviceData(const fdt_node_t* pkFdtNode,
                                       void*             pData)
{
    OS_RETURN_E retCode;

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_DRVMGR_ENABLED,
                       TRACE_DRV_MGR_SETDEVDATA_ENTRY,
                       4,
                       0,
                       (uint32_t)pkFdtNode,
                       0,
                       (uint32_t)pData);
#else
    KERNEL_TRACE_EVENT(TRACE_DRVMGR_ENABLED,
                       TRACE_DRV_MGR_SETDEVDATA_ENTRY,
                       4,
                       (uint32_t)((uintptr_t)pkFdtNode >> 32),
                       (uint32_t)(uintptr_t)pkFdtNode,
                       (uint32_t)((uintptr_t)pData >> 32),
                       (uint32_t)(uintptr_t)pData);
#endif

    /* Check parameters */
    if(pkFdtNode != NULL)
    {
        /* Not clean, but this avoids a lot of function calls and
         * processing.
         * TODO: Clean this.
         */
        ((fdt_node_t*)pkFdtNode)->pDevData = pData;
        retCode = OS_NO_ERR;
    }
    else
    {
        retCode = OS_ERR_NULL_POINTER;
    }

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_DRVMGR_ENABLED,
                       TRACE_DRV_MGR_SETDEVDATA_EXIT,
                       5,
                       0,
                       (uint32_t)pkFdtNode,
                       0,
                       (uint32_t)pData,
                       retCode);
#else
    KERNEL_TRACE_EVENT(TRACE_DRVMGR_ENABLED,
                       TRACE_DRV_MGR_SETDEVDATA_EXIT,
                       5,
                       (uint32_t)((uintptr_t)pkFdtNode >> 32),
                       (uint32_t)(uintptr_t)pkFdtNode,
                       (uint32_t)((uintptr_t)pData >> 32),
                       (uint32_t)(uintptr_t)pData,
                       retCode);
#endif

    return retCode;
}

void* driverManagerGetDeviceData(const uint32_t kHandle)
{
    void*             pDevData;
    const fdt_node_t* kpNode;

    KERNEL_TRACE_EVENT(TRACE_DRVMGR_ENABLED,
                       TRACE_DRV_MGR_GETDEVDATA_ENTRY,
                       1,
                       kHandle);

    /* Get the node and return the device data */
    pDevData = NULL;
    kpNode = fdtGetNodeByHandle(kHandle);
    if(kpNode != NULL)
    {
        pDevData = kpNode->pDevData;
    }

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_DRVMGR_ENABLED,
                       TRACE_DRV_MGR_GETDEVDATA_EXIT,
                       3,
                       kHandle,
                       0,
                       (uint32_t)pDevData);
#else
    KERNEL_TRACE_EVENT(TRACE_DRVMGR_ENABLED,
                       TRACE_DRV_MGR_GETDEVDATA_EXIT,
                       3,
                       kHandle,
                       (uint32_t)((uintptr_t)pDevData >> 32),
                       (uint32_t)(uintptr_t)pDevData);
#endif

    return pDevData;
}

/************************************ EOF *************************************/