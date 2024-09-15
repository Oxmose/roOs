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
/** @brief IOCTL Graphic Command: Draw Rectangle */
#define VFS_IOCTL_GRAPH_DRAWRECT 8
/** @brief IOCTL Graphic Command: Draw Line */
#define VFS_IOCTL_GRAPH_DRAWLINE 9
/** @brief IOCTL Graphic Command: Draw Bitmap */
#define VFS_IOCTL_GRAPH_DRAWBITMAP 10

/** @brief IOCTL File Command: Seek */
#define VFS_IOCTL_FILE_SEEK 11
/** @brief IOCTL File Command: Tell */
#define VFS_IOCTL_FILE_TELL 12

/** @brief IOCTL Device Command: Get Device Sector Size */
#define VFS_IOCTL_DEV_GET_SECTOR_SIZE 100
/** @brief IOCTL Device Command: Set LBA */
#define VFS_IOCTL_DEV_SET_LBA 101
/** @brief  IOCTL Device Command: Flush */
#define VFS_IOCTL_DEV_FLUSH 102

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief File IOCTL Seek arguments */
typedef enum
{
    /** @brief Set the absolute offset */
    SEEK_SET = 0,
    /** @brief Set the offset from the current position */
    SEEK_CUR = 1,
    /** @brief Set the offset after the end of the file */
    SEEK_END = 2
} SEEK_DIRECTION_E;

/** @brief File IOCTL Seek arguments */
typedef struct
{
    /** @brief Seek direction */
    SEEK_DIRECTION_E direction;

    /** @brief Seek offset */
    size_t offset;
} seek_ioctl_args_t;

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
