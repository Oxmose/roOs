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
     * @param[in] pNode The FDT node that matches the driver's compatible.
     *
     * @return The functon should return the error state of the driver.
     */
    OS_RETURN_E (*pDriverAttach)(const fdt_node_t* pNode);
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

#endif /* #ifndef __CORE_DRIVERMGR_H_ */

/************************************ EOF *************************************/
