/*******************************************************************************
 * @file userinit.c
 *
 * @see userinit.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 16/06/2024
 *
 * @version 1.0
 *
 * @brief Kernel's user entry point.
 *
 * @details Kernel's user entry point. This file gather the functions called
 * by the kernel just before starting the scheduler and executing the tast.
 * Users can use this function to add relevant code to their applications'
 * initialization or for other purposes.
 *
 * @warning All interrupts are disabled when calling the user initialization
 * functions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <vfs.h>         /* VFS service */
#include <syslog.h>      /* Kernel syslog */
#include <kernelshell.h> /* User kernel's shell */

/* Header file */
#include <userinit.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the init ram disk device path */
#define INITRD_DEV_PATH "/dev/storage/ramdisk0"

/** @brief Defines the init ram disk mount point */
#define INITRD_MNT_PATH "/initrd"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

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

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void userInit(void)
{
    OS_RETURN_E error;

    /* Mount the init ram disk */
    error = vfsMount(INITRD_MNT_PATH, INITRD_DEV_PATH, NULL);
    if(error != OS_NO_ERR)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               "USER",
               "Failed to mount init ramdisk, error %d",
               error);
    }

    /* Create the init process */

    /* Initialize the kernel shell TODO: Remove */
    kernelShellInit();
}

/************************************ EOF *************************************/