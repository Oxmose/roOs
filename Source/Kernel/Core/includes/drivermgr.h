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
    const char* pName;
    const char* pDescription;
    const char* pCompatible;
    const char* pVersion;

    OS_RETURN_E (*pDriverAttach)(const fdt_node_t*);
} driver_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Registers a new driver.
 *
 * @details Registers a new driver in the kernel's driver table.
 *
 * @param[in] driver_t DRIVER The driver to add to the driver table
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
