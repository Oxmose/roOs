/*******************************************************************************
 * @file diskmanager.h
 *
 * @see diskmanager.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 25/07/2024
 *
 * @version 1.0
 *
 * @brief Disk manager module.
 *
 * @details Disk manager module for roOs. The disk manager detects the presence
 * of medias in /dev/storage and tries to detect viable partitions. The disk
 * manager can be added partition table drivers to detect more systems.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __FS_DISKMANAGER_H_
#define __FS_DISKMANAGER_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stddef.h>  /* Standard definitions */
#include <stdbool.h> /* Bool types */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/**
 * @brief Defines the generic definition for a partition table driver used in
 * the kernel
 */
typedef struct
{
    /**
     * @brief Creates the partitions detected at the path provided.
     *
     * @details Creates the partitions detected at the path provided. The driver
     * shall create the device path corresponding to the partitions it detected
     * and return if the creation was successful.
     *
     * @param[in] kpRootPath The device path where to search for partitions.
     *
     * @return The function shall return true on success and false on error.
     */
    bool (*createPartitions)(const char* kpRootPath);
} dskmgr_driver_t;

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
 * @brief Initializes the disk manager.
 *
 * @details Initializes the disk manager. Detect and create disk partitions
 * in the system. On error, the initialization generates a kernel panic.
 */
void diskManagerInit(void);

#endif /* #ifndef __FS_DISKMANAGER_H_ */

/************************************ EOF *************************************/
