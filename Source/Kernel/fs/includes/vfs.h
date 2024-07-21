/*******************************************************************************
 * @file vfs.h
 *
 * @see vfs.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 18/07/2024
 *
 * @version 1.0
 *
 * @brief Virtual Filesystem driver.
 *
 * @details Virtual Filesystem driver. This virtual filesystem manages all mount
 * points in roOs, allows pluging various filesystems with the driver API and
 * provides the necessary API to manafe file and file-based drivers.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __FS_VFS_H_
#define __FS_VFS_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stddef.h>  /* Standard definitions */
#include <stdint.h>  /* Standard int definitions */
#include <kerror.h>  /* Kernel errors */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the invalid return error for the VFS */
#define VFS_DRIVER_INVALID NULL

/** @brief Defines the maximal length of a file name */
#define VFS_FILENAME_MAX_LENGTH 256

/** @brief Defines the VFS access permissions for read only */
#define O_RDONLY 4

/** @brief Defines the VFS access permissions for read / write */
#define O_RDWR 6


/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the file types supported by the VFS */
typedef enum
{
    /** @brief Type: file */
    VFS_FILE_TYPE_FILE = 0,
    /** @brief Type: directory  */
    VFS_FILE_TYPE_DIR  = 1
} VFS_FILE_TYPE_E;

/** @brief Defines the directory entry structure */
typedef struct
{
    /** @brief Directory entry name  */
    char pName[VFS_FILENAME_MAX_LENGTH];

    /** @brief Directory entry type */
    VFS_FILE_TYPE_E type;
} dirent_t;

/**
 * @brief Defines the function pointer for the open hook function.
 *
 * @details Defines the function pointer for the open hook function. This hook
 * will be called by the VFS once the driver is found based on the file path.
 * The part of the path that leads to the mouting point is stripped from the
 * path provided to this hook.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in] kpPath The path to the file to open. The part of the path that
 * leads to the mouting point is stripped from the path provided to this hook.
 * @param[in] flags The opening flags.
 * @param[in] mode The opening mode.
 *
 * @return The function shall return a handle to the opened file that will later
 * be passed to the driver for the other manipulation functions. This handle is
 * only used by the underlying driver and not modified by the VFS. On error the
 * function shall return -1 casted to void*.
 */
typedef void* (*VFS_OPEN_FUNC)(void*       pDriverData,
                               const char* kpPath,
                               int         flags,
                               int         mode);

/**
 * @brief Defines the function pointer for the close hook function.
 *
 * @details Defines the function pointer for the close hook function. This hook
 * will be called by the VFS once the driver is found based on the file path.
 * The part of the path that leads to the mouting point is stripped from the
 * path provided to this hook.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in, out] pFileData The driver's private file handle returned by open.
 *
 * @return The function shall return 0 when the close operation is successfull,
 * -1 otherwise.
 */
typedef int32_t (*VFS_CLOSE_FUNC)(void* pDriverData, void* pFileData);

/**
 * @brief Defines the function pointer for the read hook function.
 *
 * @details Defines the function pointer for the read hook function. This hook
 * will be called by the VFS once the driver is found based on the internal file
 * descriptors managed by the VFS.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in, out] pFileData The driver's private file handle returned by open.
 * @param[out] pBuffer The buffer receiving the bytes read.
 * @param[in] count The maximal number of bytes to read.
 *
 * @return The function shall return the number of byte read into the buffer or
 * -1 on error.
 */
typedef ssize_t (*VFS_READ_FUNC)(void*  pDriverData,
                                 void*  pFileData,
                                 void*  pBuffer,
                                 size_t count);

/**
 * @brief Defines the function pointer for the write hook function.
 *
 * @details Defines the function pointer for the write hook function. This hook
 * will be called by the VFS once the driver is found based on the internal file
 * descriptors managed by the VFS.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in, out] pFileData The driver's private file handle returned by open.
 * @param[in] pBuffer The buffer containing the bytes to write.
 * @param[in] count The maximal number of bytes to write.
 *
 * @return The function shall return the number of byte written from the buffer
 * or -1 on error.
 */
typedef ssize_t (*VFS_WRITE_FUNC)(void*       pDriverData,
                                  void*       pFileData,
                                  const void* pBuffer,
                                  size_t      count);

/**
 * @brief Defines the function pointer for the readdir hook function.
 *
 * @details Defines the function pointer for the readdir hook function. This
 * hook will be called by the VFS once the driver is found based on the internal
 * file descriptors managed by the VFS.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in, out] pFileData The driver's private file handle returned by open.
 * @param[out] pDirEntry The directory entry to fill by the driver.
 *
 * @return The function shall return 0 on reaching the end of the directory,
 * 1 success or -1 on error.
 */
typedef int32_t (*VFS_READDIR_FUNC)(void*     pDriverData,
                                    void*     pFileData,
                                    dirent_t* pDirEntry);

/**
 * @brief Defines the function pointer for the ioctl hook function.
 *
 * @details Defines the function pointer for the ioctl hook function. This
 * hook will be called by the VFS once the driver is found based on the internal
 * file descriptors managed by the VFS.
 *
 * @param[in, out] pDriverData The data provided by the driver when registering
 * itself.
 * @param[in, out] pFileData The driver's private file handle returned by open.
 * @param[in] operation The directory IOCTL operation idnetifier to execute.
 * @param[in, out] pArgs Optional IOCTL parameters.
 *
 * @return The function shall return whatever value required to be returned.
 */
typedef int32_t (*VFS_IOCTL_FUNC)(void*    pDriverData,
                                  void*    pFileData,
                                  uint32_t operation,
                                  void*    pArgs);

/** @brief Defines the VFS driver handle */
typedef void* vfs_driver_t;

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
 * @brief Initializes the VFS driver.
 *
 * @details Initializes the VFS driver. Creates the entry mount point and
 * allocates the resources used by the VFS. On error, the initialization
 * generates a kernel panic.
 */
void vfsInit(void);

/**
 * @brief Registers a new driver in the VFS for the given path.
 *
 * @details Registers a new driver in the VFS for the given path.
 * When registering, the driver can provide a private handle to its internal
 * data through the parameter pDriverData.
 *
 * @param[in] kpPath The root path handled by the driver.
 * @param[in] pDriverData The handle to the private data used by the driver.
 * This handle will be passed as parameter in the hook functions.
 * @param[in] pOpen The open function hook pointer.
 * @param[in] pClose The close function hook pointer.
 * @param[in] pRead The read function hook pointer.
 * @param[in] pWrite The write function hook pointer.
 * @param[in] pReadDir The readdir function hook pointer.
 * @param[in] pIOCTL The ioctl function hook pointer.
 *
 * @return On success the VFS driver handle is returned, otherwise
 * VFS_DRIVER_INVALID is returned on error.
 */
vfs_driver_t vfsRegisterDriver(const char*      kpPath,
                               void*            pDriverData,
                               VFS_OPEN_FUNC    pOpen,
                               VFS_CLOSE_FUNC   pClose,
                               VFS_READ_FUNC    pRead,
                               VFS_WRITE_FUNC   pWrite,
                               VFS_READDIR_FUNC pReadDir,
                               VFS_IOCTL_FUNC   pIOCTL);

/**
 * @brief Unregisters a registered VFS driver using its handle.
 *
 * @details Unregisters a registered VFS driver using its handle. The handle was
 * returned when registering the driver.
 *
 * @param[out] pDriver The pointer to the VFS driver handle. When successfully
 * unregistered, the pointer handle is set to VFS_DRIVER_INVALID.
 *
 * @return The function returns the success or error state.
 */
OS_RETURN_E vfsUnregisterDriver(vfs_driver_t* pDriver);

/**
 * @brief Opens and possibly create a file.
 *
 * @details Opens and possibly create a file. The function opens a file
 * specified by its path. If the file does not exist and the flags are set to
 * O_CREAT, the file will be created.
 *
 * @param[in] kpPath The path to the file to open.
 * @param[in] flags The opening flags.
 * @param[in] mode The opening mode.
 *
 * @return The function returns a file descriptor pointing to the opened file.
 * On error, the function returns -1.
 */
int32_t vfsOpen(const char* kpPath, int flags, int mode);

/**
 * @brief Closes an opened file.
 *
 * @details Closes an opened file. The function releases the resources
 * allocated to the file in the system.
 *
 * @param[in] fd The file descriptor of the file to close.
 *
 * @return The function returns 0 on success.
 * On error, the function returns -1.
 */
int32_t vfsClose(int32_t fd);

/**
 * @brief Reads bytes from a file.
 *
 * @details Reads bytes from a file. The function returns the number of bytes
 * read from the file.
 *
 * @param[in] fd The file descriptor of the file to read.
 * @param[out] pBuffer The buffer receiving the bytes to read.
 * @param[in] count The maximal number of bytes to read.
 *
 * @return The function returns the number of byte read into the buffer or
 * -1 on error.
 */
ssize_t vfsRead(int32_t fd, void*  pBuffer, size_t count);

/**
 * @brief Writes bytes to a file.
 *
 * @details Writes bytes to a file. The function returns the number of bytes
 * written to the file.
 *
 * @param[in] fd The file descriptor of the file to write.
 * @param[out] pBuffer The buffer containing the bytes to write.
 * @param[in] count The maximal number of bytes to write.
 *
 * @return The function returns the number of byte written into the file or
 * a negative value on error.
 */
ssize_t vfsWrite(int32_t fd, const void* pBuffer, size_t count);

/**
 * @brief Reads a directory entry.
 *
 * @details Reads a directory entry. The function fills a pointer to a
 * dirent structure representing the next directory entry in the directory
 * stream pointed to by pDirEntry. It returns 0 on reaching the end of the
 * directory stream or -1 if an error occurred.
 *
 * @param[in] fd The file descriptor of the file to use.
 * @param[out] pDirEntry The directory entry to fill by the driver.
 *
 * @return The function returns 0 on reaching the end of the
 * directory stream or -1 if an error occurred. Otherwise the function returns
 * 1.
 */
int32_t vfsReaddir(int32_t fd, dirent_t* pDirEntry);

/**
 * @brief Performs an IOCTL command on a file.
 *
 * @details Performs an IOCTL command on a file. The function sends the IOCTL to
 * the underlying driver to be processed.
 *
 * @param[in] fd The file descriptor of the file to use.
 * @param[in] operation The IOCTL command to send.
 * @param[in, out] pArgs Optional IOCTL parameters.
 *
 * @return The function return whatever value required to be returned by the
 * IOCTL command.
 */
int32_t vfsIOCTL(int32_t fd, uint32_t operation, void* pArgs);

#endif /* #ifndef __FS_VFS_H_ */

/************************************ EOF *************************************/
