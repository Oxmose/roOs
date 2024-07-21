/*******************************************************************************
 * @file ioctl.h
 *
 * @see vfs.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 18/07/2024
 *
 * @version 1.0
 *
 * @brief IOCTL commands definition.
 *
 * @details Defines the IOCTL standard commands used in roOs.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __FS_IOCTL_H_
#define __FS_IOCTL_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief IOCTL Console Command: Restore Cursor */
#define VFS_IOCTL_CONS_RESTORE_CURSOR 0
/** @brief IOCTL Console Command: Save Cursor */
#define VFS_IOCTL_CONS_SAVE_CURSOR 1
/** @brief IOCTL Console Command: Scroll */
#define VFS_IOCTL_CONS_SCROLL 2
/** @brief IOCTL Console Command: Set Colorscheme */
#define VFS_IOCTL_CONS_SET_COLORSCHEME 3
/** @brief IOCTL Console Command: Save Colorscheme */
#define VFS_IOCTL_CONS_SAVE_COLORSCHEME 4
/** @brief IOCTL Console Command: Clear Screen */
#define VFS_IOCTL_CONS_CLEAR 5
/** @brief IOCTL Console Command: Flush Device */
#define VFS_IOCTL_CONS_FLUSH 6

/** @brief IOCTL Graphic Command: Draw Pixel */
#define VFS_IOCTL_GRAPH_DRAWPIXEL 7

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

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

/* None */

#endif /* #ifndef __FS_IOCTL_H_ */

/************************************ EOF *************************************/
