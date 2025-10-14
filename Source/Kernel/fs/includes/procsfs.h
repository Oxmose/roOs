/*******************************************************************************
 * @file procsfs.h
 *
 * @see procsfs.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 13/10/2025
 *
 * @version 1.0
 *
 * @brief Kernel's process filesystem driver.
 *
 * @details Kernel's process filesystem driver. Defines the functions and
 * structures used by the kernel to manage the procfs entries.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/


#ifndef __FS_PROCFS_H_
#define __FS_PROCFS_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <vfs.h>    /* VFS definitions */
#include <kerror.h> /* Kernel errors */
#include <stdint.h> /* Standard int types */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines the file operations for a procfs entry. */
typedef struct
{
    /** @brief FS open function, see vfs_open_func_t type for more information */
    vfs_open_func_t pOpen;
    /** @brief FS close function, see vfs_close_func_t type for more
     * information */
    vfs_close_func_t pClose;
    /** @brief FS read function, see vfs_read_func_t type for more information */
    vfs_read_func_t pRead;
    /** @brief FS write function, see vfs_write_func_t type for more
     * information */
    vfs_write_func_t pWrite;
    /** @brief FS read dir function, see vfs_readdir_func_t type for more
     * information */
    vfs_readdir_func_t pReadDir;
    /** @brief FS ioctl function, see vfs_ioctl_func_t type for more
     * information */
    vfs_ioctl_func_t pIOCTL;
} procfs_file_operations_t;

/** @brief Defines how a procfs entry is represented in the kernel. */
typedef struct procfs_dir_entry
{
    /** @brief Name of the procfs entry. */
    const char* name;
    /** @brief Open mode of the procfs entry. */
    int32_t mode;

    /** @brief File operations registered for this procfs entry. */
    procfs_file_operations_t* fops;

    /**
     * @brief Parent of the procfs entry. Can be NULL is the entry is located
     * at the root of the procfs.
     */
    struct procfs_dir_entry* pParent;

    /** @brief Next entry in the procfs entry directory. */
    struct procfs_dir_entry* pNext;

    /**
     * @brief Sub directories of the procfs entry in case the entry is a
     * directory.
     */
    struct procfs_dir_entry* pSubDir;
} procfs_dir_entry_t;

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
 * @brief Creates a procfs directory.
 *
 * @details Creates a procfs directory. A procfs directory has no file operation
 * and has for only purpose to gather other directories or entries.
 *
 * @param[in] kpName The name of the directory to create.
 * @param[in] kpParentName The name of the parent directory to this entry. If
 * NULL, the new directory will be created at the root of the procfs.
 * @param[out] ppDirectory The structure created for the directory is filled
 * in this buffer. The value can be NULL in case of error.
 *
 * @return The function returns the error or success status.
 */
OS_RETURN_E procfsCreateDir(const char*         kpName,
                            const char*         kpParentName,
                            procfs_dir_entry_t* ppDirectory);

/**
 * @brief Removes a procfs directory.
 *
 * @details Removes a procfs directory. The directory must be empty to be
 * removed.
 *
 * @param[in/out] ppDirectory The directory structure of the directory to
 * remove.
 *
 * @return The function returns the error or success status.
 */
OS_RETURN_E procfsRemoveDir(procfs_dir_entry_t** ppDirectory);

/**
 * @brief Creates a new procfs entry.
 *
 * @details Creates a new procfs entry. The entry is created within the
 * specified directory and uses the file operations provided by the user.
 *
 * @param[in] kpName The name of the new procfs entry.
 * @param[in] kMode The open mode of the new procfs entry.
 * @param[in] kpParent The structure of the parent procfs entry. If NULL, the
 * new entry will be created at the root of the procfs.
 * @param[in] kpFops File operations used by the new procfs entry.
 * @param[out] pEntry The structure created for the entry is filled in this
 * buffer. The value can be NULL in case of error.
 *
 * @return The function returns the error or success status.
 */
OS_RETURN_E procfsCreateEntry(const char*                     kpName,
                              const uint32_t                  kMode,
                              const procfs_dir_entry_t*       kpParent,
                              const procfs_file_operations_t* kpFops,
                              procfs_dir_entry_t**            pEntry);

/**
 * @brief Removes an existing procfs entry.
 *
 * @details Removes an existing procfs entry. All file descriptors used in this
 * entry will return invalid codes once the entry is removed.
 *
 * @param[in] kpName The name of the procfs entry to remove.
 * @param[in] kpParent The structure of the parent directory of the procfs entry
 * to remove. If NULL, the parent is considered the root of the procfs.
 *
 * @return The function returns the error or success status.
 */
OS_RETURN_E procfsRemoveEntry(const char*               kpName,
                              const procfs_dir_entry_t* kpParent);

#endif /* #ifndef __FS_PROCFS_H_ */

/************************************ EOF *************************************/
