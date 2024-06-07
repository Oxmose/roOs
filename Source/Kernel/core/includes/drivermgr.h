/*******************************************************************************
 * @file drivermgr.h
 *
 * @see drivermgr.c
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

#ifndef __CORE_DRIVERMGR_H_
#define __CORE_DRIVERMGR_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <kerror.h>  /* Kernel error codes */
#include <devtree.h> /* FDT library */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the generic definition for a driver used in the kernel */
typedef struct
{
    /** @brief Driver's name. */
    const char* pName;
    /** @brief Driver's description. */
    const char* pDescription;
    /** @brief Driver's compatible string. */
    const char* pCompatible;
    /** @brief Driver's version. */
    const char* pVersion;

    /**
     * @brief Driver's attatch function.
     *
     * @details Driver's attatch function. This function is called when a device
     * is detected and is compatible with the driver's compatible string.
     * It should initialize the driver and / or device.
     *
     * @param[in] pkFdtNode The FDT node that matches the driver's compatible.
     *
     * @return The functon should return the error state of the driver.
     */
    OS_RETURN_E (*pDriverAttach)(const fdt_node_t* pkFdtNode);
} driver_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Registers a new driver.
 *
 * @details Registers a new driver in the kernel's driver table.
 *
 * @param[in] DRIVER The driver to add to the driver table.
 */
#define DRIVERMGR_REG(DRIVER)                                                 \
    driver_t* DRVENT_##DRIVER __attribute__ ((section (".utk_driver_tbl"))) = \
        &DRIVER;

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
 * @brief Initializes the driver manager.
 *
 * @details Initializes the driver manager. This function will walk the FDT
 * and try to probe the registered drivers to initialize them and attach them.
 * In case of hard error, a panic is raised.
 */
void driverManagerInit(void);

/**
 * @brief Registers the device data of a node.
 *
 * @details Registers the device data of a node. The data can be any structure
 * or information that the driver corresponding to the phandle has defined.
 * If a data was already associated, it is overriden.
 *
 * @param[in] pkFdtNode The FDT pHandle node to associate the data with.
 * @param[in] pData The data to associate.
 *
 * @return The success or error status is returned.
 */
OS_RETURN_E driverManagerSetDeviceData(const fdt_node_t* pkFdtNode,
                                       void*             pData);

/**
 * @brief Returns the device data of a given phandle.
 *
 * @details Returns the device data of a given phandle. The data can
 * be any structure or information that the driver corresponding to the phandle
 * has defined. If nothing was defined, NULL is returned.
 *
 * @param[in] kHandle The FDT pHandle of the device controller to get.
 *
 * @return A pointer to the device controller is returned. NULL is returned if
 * no data was defined.
 */
void* driverManagerGetDeviceData(const uint32_t kHandle);

#endif /* #ifndef __CORE_DRIVERMGR_H_ */

/************************************ EOF *************************************/
