/*******************************************************************************
 * @file vfs.c
 *
 * @see vfs.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <panic.h>        /* Kernel panic */
#include <kheap.h>        /* Kernel heap */
#include <stdint.h>       /* Standard int definitions */
#include <string.h>       /* String manipulation */
#include <kerror.h>       /* Kernel errors */
#include <vector.h>       /* Vectors library */
#include <kqueue.h>       /* Kernel queues */
#include <syslog.h>       /* Syslog service */
#include <stdbool.h>      /* Bool types */
#include <critical.h>     /* Kernel critical management */
#include <scheduler.h>    /* Kernel scheduler */

/* Configuration files */
#include <config.h>

/* Header file */
#include <vfs.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "VFS"

/** @brief Defines the initial number of file descriptors */
#define VFS_INITIAL_FD_COUNT 128

/** @brief Defines the VFS path node delimiter */
#define VFS_PATH_DELIMITER '/'

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines a VFS file descriptor table */
typedef struct
{
    /** @brief File descriptor table */
    vector_t* pFdTable;

    /** @brief Free file descriptor pool */
    kqueue_t* pFdFreePool;

    /** @brief File descriptor table lock */
    kernel_spinlock_t lock;
} vfs_fd_table_t;

/** @brief VFS internal driver structure */
typedef struct
{
    /** @brief The driver's internal data handle. */
    void* pDriverData;

    /** @brief The open function hook pointer. */
    vfs_open_func_t pOpen;
    /** @brief The close function hook pointer. */
    vfs_close_func_t pClose;
    /** @brief The read function hook pointer. */
    vfs_read_func_t pRead;
    /** @brief The write function hook pointer. */
    vfs_write_func_t pWrite;
    /** @brief The readdir function hook pointer. */
    vfs_readdir_func_t pReadDir;
    /** @brief The ioctl function hook pointer. */
    vfs_ioctl_func_t pIOCTL;

    /**
     * @brief Unmount function for the filesystem.
     *
     * @details Unmount function for the filesystem. This function will unlink a
     * directory to the associated device.
     *
     * @param[in, out] pDriverMountData The driver data generated when mounting
     * the filesystem.
     *
     * @return The function returns the success or error status.
     */
    OS_RETURN_E (*pUnmount)(void* pDriverMountData);
} vfs_driver_internal_t;

/** @brief Node structure used to keep track of the mounted points */
typedef struct vfs_node_t
{
    /** @brief The mount point path relative to the parent path */
    char* pMountPoint;

    /** @brief Stores the mount point path length */
    size_t mountPointLength;

    /** @brief Stores the offset in the path from the root to the end of this
     * mount point path.
     */
    size_t mountPointOffset;

    /** @brief The driver to use when accessing files at this mount point, if
     * NULL, this is a transient node to other mount points
     */
    vfs_driver_internal_t* pDriver;

    /** @brief The first child node of this node */
    struct vfs_node_t* pFirstChild;

    /** @brief The next sibling node of this node */
    struct vfs_node_t* pNextSibling;

    /** @brief The previous sibling node of this node */
    struct vfs_node_t* pPrevSibling;

    /** @brief The parent of this node */
    struct vfs_node_t* pParent;
} vfs_node_t;

/** @brief Stores the shared information for a file descriptor */
typedef struct
{
    /** @brief FD file path */
    char* pFilePath;

    /** @brief FD internal driver file handle */
    void* pFileHandle;

    /** @brief FD file driver */
    vfs_driver_internal_t* pDriver;

    /** @brief Stores the reference count */
    uint32_t refCount;

    /** @brief The share lock */
    kernel_spinlock_t lock;
} vfs_intern_fd_shared_t;

/** @brief Defines the VFS internal file descriptor */
typedef struct
{
    /** @brief The index of the FD in the FD table */
    uint32_t tableId;

    /** @brief The file descriptor shared data */
    vfs_intern_fd_shared_t* pShared;

    /** @brief Mode used when opening the file */
    int openMode;

    /** @brief Flags used when opening the file */
    int openFlags;
} vfs_internal_fd_t;

/** @brief Defines the generic descriptor for the generic VFS operations */
typedef struct
{
    /** @brief Descriptor mount point */
    vfs_node_t* pMountPt;

    /** @brief Contains the next dir entry */
    vfs_node_t* pNextChildCursor;
} vfs_generic_desc_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Asserts a condition and generates a kernel panic in case of failure.
 *
 * @details Asserts a condition and generates a kernel panic in case of failure.
 *
 * @param[in] COND The condition to verify.
 * @param[in] MSG The message to print in case of error.
 * @param[in] ERROR The error code.
 *
*/
#define VFS_ASSERT(COND, MSG, ERROR) {                      \
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Gets the index before the next delimiter in the path.
 *
 * @details Gets the index before the next delimiter in the path.
 *
 * @param[in] kpPath The path to use.
 *
 * @return The function returns the index before the next delimiter in the path.
 * -1 is returned when the function reached the end of the path.
 */
static ssize_t _getNextPathToken(const char* kpPath);

/**
 * @brief Cleans the path provided as parameter.
 *
 * @details Cleans the path provided as parameter. The function will strip
 * unwanted or redundant character and perform other cleaning tasks.
 *
 * @param[out] pCleanPath A pointer to the memory where to store the cleaned
 * path.
 * @param[in] kpOriginalPath The original path to clean.
 *
 * @return The function returns the size of the clean path.
 */
static size_t _cleanPath(char* pCleanPath, const char* kpOriginalPath);

/**
 * @brief Cleans a node from the mount point graph.
 *
 * @details Cleans a node from the mount point graph. The node will be removed
 * if none of its children implement a driver. This will also clean the
 * children nodes.
 *
 * @param[out] pNode The node to clean.
 *
 * @return The function returns true if the node implements a driver, false
 * otherwise and if cleaned.
 */
static bool _cleanNode(vfs_node_t* pNode);

/**
 * @brief Adds a node to the mount point graph.
 *
 * @details Adds a node to the mount point graph. If the path to the parent node
 * is not the direct path to the new node, transient nodes are also created.
 *
 * @param[out] pParent The parent of the node to create.
 * @param[in] kpRelPath The path of the node to create, relative to the parent
 * node path.
 * @param[in] The total size of the path of the node to create.
 *
 * @return The newly created node pointer is returned on success. Otherwise
 * NULL is returned on error.
 */
static vfs_node_t* _addNode(vfs_node_t*  pParent,
                            const char*  kpRelPath,
                            const size_t kPathSize);

/**
 * @brief Finds a mount node in the mount node graph.
 *
 * @details Finds a mount node in the mount node graph. The node can be searched
 * for driver or for exact match.
 *
 * @param[in] pRoot The root node to start the search from.
 * @param[in] kpPath The path to match when searching the node.
 * @param[in] kPatchSize The size of the path to search for.
 * @param[in] kSearchDriver Tells the function to finds the deepest node that
 * matches the path and implements a driver.
 * @param[in] kFindExactNode Tells the function to return the node pointer only
 * is the total path matches the node.
 *
 * @return The function return the node corresponding to the search criteria.
 * If no node is found, NULL is returned.
 */
static vfs_node_t* _findNodeFromPath(vfs_node_t* pRoot,
                                     const char* kpPath,
                                     size_t      kPathSize,
                                     const bool  kSearchDriver,
                                     const bool  kFindExactNode);

/**
 * @brief Adds a driver with a given path to the mount point graph.
 *
 * @details Adds a driver with a given path to the mount point graph. The
 * node can be added to already existing mount point, it will be created as a
 * child of the parent node.
 *
 * @param[in] kpPath The path of the driver to add.
 * @param[in, out] pInternalDriver The driver to add.
 *
 * @return On success, the function returns the pointer to the newly created
 * node that implements the driver. Otherwise, NULL is returned on error.
 */
static vfs_node_t* _vfsAddDriver(const char*            kpPath,
                                 vfs_driver_internal_t* pInternalDriver);

/**
 * @brief Removes a driver from the mount point graph.
 *
 * @details Removes a driver from the mount point graph. The graph is cleaned
 * after removing the driver.
 *
 * @param[out] pNode The node to remove.
 *
 * @return The function returns the success or error state.
 */
static OS_RETURN_E _vfsRemoveDriver(vfs_node_t* pNode);

/**
 * @brief Gets the file internal file descriptor associated to a generic file
 * descriptor.
 *
 * @details Gets the file internal file descriptor associated to a generic file
 * descriptor.
 *
 * @param[in, out] pTable The process file descriptor table.
 * @param[in] kFd The generic file descriptor to use.
 * @param[out] ppInternalFd A buffer to the internal file descriptor pointer to
 * retrieve.
 *
 * @return The function returns the success or error state.
 */
static OS_RETURN_E _getFd(vfs_fd_table_t*     pTable,
                          const int32_t       kFd,
                          vfs_internal_fd_t** ppInternalFd);

/**
 * @brief Releases a file descriptor to the free fd table.
 *
 * @details Releases a file descriptor to the free fd table. The resources
 * associated to the file descritptor are freed.
 *
 * @param[in, out] pTable The process file descriptor table.
 * @param[in] fd The generic file descriptor to use.
 */
static void _releaseFileDescriptor(vfs_fd_table_t* pTable, const int32_t fd);

/**
 * @brief Creates a new file descriptor.
 *
 * @details Creates a new file descriptor. The descriptor is populated with the
 * data given as parameter.
 *
 * @param[in, out] pTable The process file descriptor table.
 * @param[in] pDriver The driver to associate to the file descriptor.
 * @param[in] pFileHandle The file handle generated by the underlying driver.
 * @param[in] kpPath The absolute path of the file corresponding to the fd.
 * @param[in] kFlags The flags used when opening the fd.
 * @param[in] kMode The mode used when opening the fd.
 *
 * @return The function returns the value of the newly created file descriptor.
 * On error the function returns -1.
 */
static int32_t _createFileDescriptor(vfs_fd_table_t*        pTable,
                                     vfs_driver_internal_t* pDriver,
                                     void*                  pFileHandle,
                                     const char*            kpPath,
                                     const int              kFlags,
                                     const int              kMode);

/**
 * @brief Generic VFS open hook.
 *
 * @details Generic VFS open hook. This function returns a handle to control the
 * VFS generic nodes.
 *
 * @param[in, out] pDrvCtrl The generic VFS driver.
 * @param[in] kpPath The path in the VFS mount point table.
 * @param[in] flags The open flags.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _vfsGenericOpen(void*       pDrvCtrl,
                             const char* kpPath,
                             int         flags,
                             int         mode);

/**
 * @brief Generic VFS close hook.
 *
 * @details Generic VFS close hook. This function closes a handle that was
 * created when calling the open function.
 *
 * @param[in, out] pDrvCtrl The generic VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _vfsGenericClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief Generic VFS write hook.
 *
 * @details Generic VFS write hook.
 *
 * @param[in, out] pDrvCtrl The generic VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _vfsGenericWrite(void*       pDrvCtrl,
                                void*       pHandle,
                                const void* kpBuffer,
                                size_t      count);

/**
 * @brief Generic VFS read hook.
 *
 * @details Generic VFS read hook.
 *
 * @param[in, out] pDrvCtrl The generic VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _vfsGenericRead(void*  pDrvCtrl,
                               void*  pHandle,
                               void*  pBuffer,
                               size_t count);

/**
 * @brief Generic VFS ReadDir hook.
 *
 * @details Generic VFS ReadDir hook. This function performs the ReadDir for the
 * Generic driver.
 *
 * @param[in, out] pDrvCtrl The generic VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] pDirEntry The directory entry to fill by the driver.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _vfsGenericReadDir(void*     pDriverData,
                                  void*     pHandle,
                                  dirent_t* pDirEntry);

/**
 * @brief Generic VFS IOCTL hook.
 *
 * @details Generic VFS IOCTL hook. This function performs the IOCTL for the
 * Generic driver.
 *
 * @param[in, out] pDrvCtrl The generic VFS driver.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _vfsGenericIOCTL(void*    pDriverData,
                                void*    pHandle,
                                uint32_t operation,
                                void*    pArgs);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Start address of the registered kernel fs table */
extern uintptr_t _START_FS_TABLE_ADDR;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief VFS mount point graph */
static vfs_node_t* spRootPoint = NULL;

/** @brief VFS generic driver */
static vfs_driver_internal_t sVfsGenericDriver = {
    .pDriverData = NULL,
    .pOpen       = _vfsGenericOpen,
    .pClose      = _vfsGenericClose,
    .pRead       = _vfsGenericRead,
    .pWrite      = _vfsGenericWrite,
    .pReadDir    = _vfsGenericReadDir,
    .pIOCTL      = _vfsGenericIOCTL
};

/** @brief Kernel file descriptor table lock */
static kernel_spinlock_t sMountPointLock;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static size_t _cleanPath(char* pCleanPath, const char* kpOriginalPath)
{
    size_t size;
    size_t newSize;
    size_t i;
    bool   firstDelim;

    size = strlen(kpOriginalPath);

    /* Remove trailing delimiter */
    while(size > 0 && kpOriginalPath[size - 1] == VFS_PATH_DELIMITER)
    {
        --size;
    }

    /* Copy while removing multiple delimiters */
    firstDelim = false;
    newSize = 0;
    for(i = 0; i < size; ++i)
    {
        if(kpOriginalPath[i] == VFS_PATH_DELIMITER)
        {
            if(firstDelim == true)
            {
                continue;
            }
            else
            {
                firstDelim = true;
            }
        }
        else
        {
            firstDelim = false;
        }
        pCleanPath[newSize++] = kpOriginalPath[i];
    }

    pCleanPath[newSize] = 0;

    return newSize;
}

static ssize_t _getNextPathToken(const char* kpPath)
{
    ssize_t nextStop;

    nextStop = 0;

    /* Try to find the next delimiting character */
    while(*kpPath != 0 && *kpPath != VFS_PATH_DELIMITER)
    {
        ++nextStop;
        ++kpPath;
    }

    /* Check for end of path */
    if(nextStop == 0 && *kpPath != VFS_PATH_DELIMITER)
    {
        nextStop = -1;
    }

    return nextStop;
}

static vfs_node_t* _findNodeFromPath(vfs_node_t* pRoot,
                                     const char* kpPath,
                                     size_t      kPathSize,
                                     const bool  kSearchDriver,
                                     const bool  kFindExactNode)
{
    ssize_t     nextInternalPathStop;
    vfs_node_t* kpFoundNode;
    int32_t     compare;

    /* Check values */
    if(pRoot == NULL)
    {
        return NULL;
    }

#if VFS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Searching %s from %s",
           kpPath,
           pRoot->pMountPoint);
#endif

    kpFoundNode = NULL;

    /* Get the current node path */
    nextInternalPathStop = _getNextPathToken(kpPath);

    /* End of path */
    if(nextInternalPathStop < 0)
    {
        if(kFindExactNode == true ||
            (kSearchDriver == true && pRoot->pDriver != NULL))
        {
            return pRoot;
        }
        else
        {
            return NULL;
        }
    }

    /* Check if the current node can handle the path */
    if(nextInternalPathStop == (ssize_t)pRoot->mountPointLength)
    {
        compare = strncmp(kpPath,
                          pRoot->pMountPoint,
                          nextInternalPathStop);
        if(compare == 0)
        {
            kpPath += nextInternalPathStop;
            if(*kpPath == VFS_PATH_DELIMITER)
            {
                ++nextInternalPathStop;
                ++kpPath;
            }

            /* Check if we reached the end of the path */
            if(*kpPath == 0)
            {
                if(kFindExactNode == true ||
                   (kSearchDriver == true && pRoot->pDriver != NULL))
                {
                    return pRoot;
                }
                else
                {
                    return NULL;
                }
            }

            /* Check if other mount point might be managing this path */
            kpFoundNode = _findNodeFromPath(pRoot->pFirstChild,
                                            kpPath,
                                            kPathSize - nextInternalPathStop,
                                            kSearchDriver,
                                            kFindExactNode);

            /* If the found node is still NULL, this means we are the
             * manager if we have a driver registered.
             */
            if(kpFoundNode == NULL && kFindExactNode == false &&
               (kSearchDriver == false || pRoot->pDriver != NULL))
            {
                kpFoundNode = pRoot;
            }
        }
    }
    else if(nextInternalPathStop > (ssize_t)pRoot->mountPointLength)
    {
        compare = 1;
    }
    else
    {
        compare = -1;
    }

    /* Nodes are sorted in the lexicographic order, if <= 0 means that we
    * already tried all the nodes that could have fit the path
    */
    if(compare > 0)
    {
        /* If we did not find the driver, try the next sibling */
        kpFoundNode = _findNodeFromPath(pRoot->pNextSibling,
                                        kpPath,
                                        kPathSize,
                                        kSearchDriver,
                                        kFindExactNode);
    }

    return kpFoundNode;
}

static vfs_node_t* _addNode(vfs_node_t*  pParent,
                            const char*  kpRelPath,
                            const size_t kPathSize)
{
    vfs_node_t* pNewNode;
    vfs_node_t* pChild;

    /* Allocate the new node */
    pNewNode = kmalloc(sizeof(vfs_node_t));
    if(pNewNode == NULL)
    {
        return NULL;
    }

    /* Allocate new node path */
    pNewNode->pMountPoint = kmalloc(kPathSize + 1);
    if(pNewNode->pMountPoint == NULL)
    {
        kfree(pNewNode);
        return NULL;
    }

    /* Init path */
    memcpy(pNewNode->pMountPoint, kpRelPath, kPathSize);
    pNewNode->pMountPoint[kPathSize] = 0;

#if VFS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Adding node at %s from %s",
           pNewNode->pMountPoint,
           pParent->pMountPoint);
#endif

    /* Init the new node */
    pNewNode->mountPointLength = kPathSize;
    pNewNode->mountPointOffset = pParent->mountPointOffset +
                                 kPathSize +
                                 1;
    pNewNode->pDriver = NULL;
    pNewNode->pFirstChild = NULL;
    pNewNode->pParent = pParent;

    /* Link in lexicographic order */
    pChild = pParent->pFirstChild;
    if(pChild != NULL)
    {
        while(pChild != NULL)
        {
            /* If it is less, update*/
            if(pNewNode->mountPointLength < pChild->mountPointLength)
            {
                if(pChild->pPrevSibling != NULL)
                {
                    pChild->pPrevSibling->pNextSibling = pNewNode;
                }
                else
                {
                    pChild->pParent->pFirstChild = pNewNode;
                }
                pNewNode->pPrevSibling = pChild->pPrevSibling;
                pNewNode->pNextSibling = pChild;
                pChild->pPrevSibling = pNewNode;

                break;
            }
            else if(pNewNode->mountPointLength > pChild->mountPointLength)
            {
                if(pChild->pNextSibling == NULL)
                {
                    pNewNode->pPrevSibling = pChild;
                    pNewNode->pNextSibling = NULL;
                    pChild->pNextSibling = pNewNode;

                    break;
                }
                else
                {
                    pChild = pChild->pNextSibling;
                }
            }
            else
            {
                if(strcmp(pNewNode->pMountPoint, pChild->pMountPoint) < 0)
                {
                    if(pChild->pPrevSibling != NULL)
                    {
                        pChild->pPrevSibling->pNextSibling = pNewNode;
                    }
                    else
                    {
                        pChild->pParent->pFirstChild = pNewNode;
                    }
                    pNewNode->pPrevSibling = pChild->pPrevSibling;
                    pNewNode->pNextSibling = pChild;
                    pChild->pPrevSibling = pNewNode;

                    break;
                }
                else
                {
                    if(pChild->pNextSibling == NULL)
                    {
                        pNewNode->pPrevSibling = pChild;
                        pNewNode->pNextSibling = NULL;
                        pChild->pNextSibling = pNewNode;

                        break;
                    }
                    else
                    {
                        pChild = pChild->pNextSibling;
                    }
                }
            }
        }

        VFS_ASSERT(pChild != NULL,
                   "pChild was null when adding node",
                   OS_ERR_UNAUTHORIZED_ACTION);
    }
    else
    {
        pParent->pFirstChild = pNewNode;
        pNewNode->pPrevSibling = NULL;
        pNewNode->pNextSibling = NULL;
    }

    return pNewNode;
}

static bool _cleanNode(vfs_node_t* pNode)
{
    bool        hasDriver;
    vfs_node_t* pChild;

    /* Check if we have a driver */
    if(pNode->pDriver != NULL)
    {
        return true;
    }

    /* Check if children have drivers */
    hasDriver = false;
    pChild = pNode->pFirstChild;
    while(pChild != NULL)
    {
        hasDriver = _cleanNode(pChild);
        if(hasDriver == true)
        {
            break;
        }

        pChild = pChild->pNextSibling;
    }

    /* If the children have no drivers, clean ourselves */
    if(hasDriver == false)
    {
        /* Unlink */
        if(pNode->pParent->pFirstChild == pNode)
        {
            pNode->pParent->pFirstChild = pNode->pNextSibling;
        }
        else
        {
            pNode->pPrevSibling->pNextSibling = pNode->pNextSibling;
        }

        if(pNode->pNextSibling != NULL)
        {
            pNode->pNextSibling->pPrevSibling = pNode->pPrevSibling;
        }
        kfree(pNode->pMountPoint);
        kfree(pNode);
    }

    return hasDriver;
}

static vfs_node_t* _vfsAddDriver(const char*            kpPath,
                                 vfs_driver_internal_t* pInternalDriver)
{
    vfs_node_t* pNode;
    vfs_node_t* pFirstNode;
    ssize_t     pathOffset;

#if VFS_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Adding FS at %s", kpPath);
#endif

    /* Find the parent node */
    pNode = _findNodeFromPath(spRootPoint,
                              kpPath,
                              strlen(kpPath),
                              false,
                              false);
    if(pNode == NULL)
    {
        return VFS_DRIVER_INVALID;
    }

    pFirstNode = NULL;

    /* Create intermediate nodes if necessary */
    pathOffset = _getNextPathToken(kpPath +
                                   pNode->mountPointOffset);

    while(pathOffset >= 0)
    {
        /* Create the intermediate node */
        pNode = _addNode(pNode,
                         kpPath +
                         pNode->mountPointOffset,
                         pathOffset);
        if(pNode == NULL)
        {
            break;
        }

        if(pFirstNode == NULL)
        {
            pFirstNode = pNode;
        }

        /* Advance path  */
        pathOffset = _getNextPathToken(kpPath +
                                       pNode->mountPointOffset);
    }

    /* On error remove all nodes */
    if(pNode == NULL)
    {
        _cleanNode(pFirstNode);
        return VFS_DRIVER_INVALID;
    }

    /* Fill the driver node with the driver */
    pNode->pDriver = pInternalDriver;

    return pNode;
}

static OS_RETURN_E _vfsRemoveDriver(vfs_node_t* pNode)
{
    if(pNode == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Remove driver and clean node */
    pNode->pDriver = NULL;
    _cleanNode(pNode);

    return OS_NO_ERR;
}

static int32_t _createFileDescriptor(vfs_fd_table_t*        pTable,
                                     vfs_driver_internal_t* pDriver,
                                     void*                  pFileHandle,
                                     const char*            kpPath,
                                     const int              kFlags,
                                     const int              kMode)
{
    kqueue_node_t*     pFdNode;
    vfs_internal_fd_t* pInternalFd;
    size_t             pathSize;
    OS_RETURN_E        error;

    pathSize = strlen(kpPath);

    KERNEL_LOCK(pTable->lock);

    /* Get a new file descriptor */
    pFdNode = kQueuePop(pTable->pFdFreePool);
    if(pFdNode == NULL)
    {
        KERNEL_UNLOCK(pTable->lock);
        return -1;
    }

    /* Initialize the FD */
    pInternalFd = pFdNode->pData;
    pInternalFd->pShared = kmalloc(sizeof(vfs_intern_fd_shared_t));
    if(pInternalFd->pShared == NULL)
    {
        kQueuePush(pFdNode, pTable->pFdFreePool);
        KERNEL_UNLOCK(pTable->lock);
        return -1;
    }

    KERNEL_SPINLOCK_INIT(pInternalFd->pShared->lock);
    KERNEL_LOCK(pInternalFd->pShared->lock);

    pInternalFd->pShared->pFilePath = kmalloc(pathSize + 1);
    if(pInternalFd->pShared->pFilePath == NULL)
    {
        KERNEL_UNLOCK(pInternalFd->pShared->lock);
        kfree(pInternalFd->pShared);
        kQueuePush(pFdNode, pTable->pFdFreePool);
        KERNEL_UNLOCK(pTable->lock);
        return -1;
    }

    /* Set the file descriptor in the table */
    error = vectorSet(pTable->pFdTable,
                      pInternalFd->tableId,
                      pFdNode);
    VFS_ASSERT(error == OS_NO_ERR,
               "Failed to register FD in the FD table",
               error);

    memcpy(pInternalFd->pShared->pFilePath, kpPath, pathSize);
    pInternalFd->pShared->pFilePath[pathSize] = 0;

    pInternalFd->openFlags   = kFlags;
    pInternalFd->openMode    = kMode;

    pInternalFd->pShared->pDriver     = pDriver;
    pInternalFd->pShared->pFileHandle = pFileHandle;
    pInternalFd->pShared->refCount    = 1;

    KERNEL_UNLOCK(pInternalFd->pShared->lock);
    KERNEL_UNLOCK(pTable->lock);

    return pInternalFd->tableId;
}

static void _releaseFileDescriptor(vfs_fd_table_t* pTable, const int32_t fd)
{
    vfs_internal_fd_t* pInternalFd;
    kqueue_node_t*     pFdNode;
    OS_RETURN_E        error;

    /* Check that the FD is open */
    error = vectorGet(pTable->pFdTable, fd, (void**)&pFdNode);
    VFS_ASSERT(error == OS_NO_ERR && pFdNode != NULL,
               "Invalid FD released",
               OS_ERR_NO_SUCH_ID);

    pInternalFd = pFdNode->pData;
    KERNEL_LOCK(pInternalFd->pShared->lock);
    /* Release the resources if not shared */
    if(pInternalFd->pShared->refCount == 0)
    {
        if(pInternalFd->pShared->pFilePath != NULL)
        {
            kfree(pInternalFd->pShared->pFilePath);
        }
        kfree(pInternalFd->pShared);
    }
    KERNEL_UNLOCK(pInternalFd->pShared->lock);

    /* Release the FD */
    error = vectorSet(pTable->pFdTable, fd, NULL);
    VFS_ASSERT(error == OS_NO_ERR && pFdNode != NULL,
               "Invalid FD released",
               OS_ERR_NO_SUCH_ID);

    kQueuePush(pFdNode, pTable->pFdFreePool);
}

static OS_RETURN_E _getFd(vfs_fd_table_t*     pTable,
                          const int32_t       kFd,
                          vfs_internal_fd_t** ppInternalFd)
{
    OS_RETURN_E    error;
    kqueue_node_t* pFdNode;

    /* Check that the FD is valid */
    if((size_t)kFd >= pTable->pFdTable->size)
    {
        return OS_ERR_NO_SUCH_ID;
    }

    /* Check that the FD is open */
    error = vectorGet(pTable->pFdTable, kFd, (void**)&pFdNode);
    if(error != OS_NO_ERR)
    {
        return error;
    }

    if(pFdNode == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Set the internal FD */
    *ppInternalFd = pFdNode->pData;

    return OS_NO_ERR;
}


static void* _vfsGenericOpen(void*       pDrvCtrl,
                             const char* kpPath,
                             int         flags,
                             int         mode)
{
    vfs_node_t*         pMountPt;
    vfs_generic_desc_t* pDesc;


    (void)pDrvCtrl;
    (void)flags;
    (void)mode;

    /* Check if this is an exact node in the mount points */
    pMountPt = _findNodeFromPath(spRootPoint,
                                 kpPath,
                                 strlen(kpPath),
                                 false,
                                 true);
    if(pMountPt != NULL)
    {
        pDesc = kmalloc(sizeof(vfs_generic_desc_t));
        if(pDesc == NULL)
        {
            return (void*) -1;
        }
        memset(pDesc, 0, sizeof(vfs_generic_desc_t));
        pDesc->pMountPt = pMountPt;
        return pDesc;
    }

    return (void*)-1;
}

static int32_t _vfsGenericClose(void* pDrvCtrl, void* pHandle)
{
    (void)pDrvCtrl;

    /* Check if it was correctly opened */
    if(pHandle != NULL && pHandle != (void*)-1)
    {
        kfree(pHandle);
        return 0;
    }


    return -1;
}

static ssize_t _vfsGenericWrite(void*       pDrvCtrl,
                                void*       pHandle,
                                const void* kpBuffer,
                                size_t      count)
{
    (void)pDrvCtrl;
    (void)pHandle;
    (void)kpBuffer;
    (void)count;

    /* At the moment we do not this operation */
    return -1;
}

static ssize_t _vfsGenericRead(void*  pDrvCtrl,
                               void*  pHandle,
                               void*  pBuffer,
                               size_t count)
{
    (void)pDrvCtrl;
    (void)pHandle;
    (void)pBuffer;
    (void)count;

    /* At the moment we do not this operation */
    return -1;
}

static int32_t _vfsGenericReadDir(void*     pDriverData,
                                  void*     pHandle,
                                  dirent_t* pDirEntry)
{
    vfs_generic_desc_t* pDesc;

    (void)pDriverData;

    /* Check if it was correctly opened */
    if(pHandle == NULL || pHandle == (void*)-1)
    {
        return -1;
    }

    pDesc = pHandle;
    if(pDesc->pMountPt == NULL)
    {
        return -1;
    }

    if(pDesc->pNextChildCursor == NULL)
    {
        pDesc->pNextChildCursor = pDesc->pMountPt->pFirstChild;
        if(pDesc->pNextChildCursor == NULL)
        {
            return 0;
        }
    }
    else if(pDesc->pNextChildCursor == (void*)-1)
    {
        return -1;
    }

    /* Copy name */
    strncpy(pDirEntry->pName,
            pDesc->pNextChildCursor->pMountPoint,
            VFS_FILENAME_MAX_LENGTH);
    pDirEntry->pName[VFS_FILENAME_MAX_LENGTH] = 0;

    /* Check if the node has children (it is a folder) */
    if(pDesc->pNextChildCursor->pFirstChild != NULL)
    {
        pDirEntry->type = VFS_FILE_TYPE_DIR;
    }
    else
    {
        pDirEntry->type = VFS_FILE_TYPE_FILE;
    }

    /* Check if there is another sibling */
    if(pDesc->pNextChildCursor->pNextSibling != NULL)
    {
        pDesc->pNextChildCursor = pDesc->pNextChildCursor->pNextSibling;
        return 1;
    }
    else
    {
        pDesc->pNextChildCursor = (void*)-1;
    }

    return 0;
}

static ssize_t _vfsGenericIOCTL(void*    pDriverData,
                                void*    pHandle,
                                uint32_t operation,
                                void*    pArgs)
{
    (void)pDriverData;
    (void)pHandle;
    (void)operation;
    (void)pArgs;

    /* At the moment we do not support this operation */
    return -1;
}

void vfsInit(void)
{
    /* Initialize the mount point graph */
    KERNEL_SPINLOCK_INIT(sMountPointLock);
    spRootPoint = kmalloc(sizeof(vfs_node_t));
    VFS_ASSERT(spRootPoint != NULL,
               "Failed to allocate the VFS root node",
               OS_ERR_NO_MORE_MEMORY);
    spRootPoint->pMountPoint = kmalloc(sizeof(char));
    VFS_ASSERT(spRootPoint->pMountPoint != NULL,
               "Failed to allocate the VFS root node path",
               OS_ERR_NO_MORE_MEMORY);
    spRootPoint->pMountPoint[0]   = 0;
    spRootPoint->mountPointLength = 0;
    spRootPoint->mountPointOffset = 1;
    spRootPoint->pDriver          = NULL;
    spRootPoint->pFirstChild      = NULL;
    spRootPoint->pNextSibling     = NULL;
    spRootPoint->pPrevSibling     = NULL;
    spRootPoint->pParent          = NULL;
}

OS_RETURN_E vfsCreateProcessFdTable(kernel_process_t* pProcess)
{
    OS_RETURN_E        error;
    OS_RETURN_E        newError;
    int32_t            i;
    vfs_internal_fd_t* pNewFd;
    kqueue_node_t*     pNewNode;
    vfs_fd_table_t*    pNewTable;

    /* Allocate the fd table */
    pNewTable = kmalloc(sizeof(vfs_fd_table_t));
    if(pNewTable == NULL)
    {
        return OS_ERR_NO_MORE_MEMORY;
    }

    /* Initialize the kernel file descriptor table */
    pNewTable->pFdTable = vectorCreate(VECTOR_ALLOCATOR(kmalloc, kfree),
                                       NULL,
                                       VFS_INITIAL_FD_COUNT,
                                       &error);
    if(error != OS_NO_ERR)
    {
        kfree(pNewTable);
        return error;
    }

    pNewTable->pFdFreePool = kQueueCreate(false);
    if(pNewTable->pFdFreePool == NULL)
    {
        error = vectorDestroy(pNewTable->pFdTable);
        VFS_ASSERT(error == OS_NO_ERR,
                   "Failed to destroy fd table vector",
                   error);

        kfree(pNewTable);
        return OS_ERR_NO_MORE_MEMORY;
    }

    error = OS_NO_ERR;
    for(i = 0; i < VFS_INITIAL_FD_COUNT; ++i)
    {
        pNewFd = kmalloc(sizeof(vfs_internal_fd_t));
        if(pNewFd == NULL)
        {
            error = OS_ERR_NO_MORE_MEMORY;
            break;
        }
        pNewFd->tableId = i;

        pNewNode = kQueueCreateNode(pNewFd, false);
        if(pNewNode == NULL)
        {
            kfree(pNewFd);
            error = OS_ERR_NO_MORE_MEMORY;
            break;
        }

        error = vectorSet(pNewTable->pFdTable, i, NULL);
        if(error != OS_NO_ERR)
        {
            kQueueDestroyNode(&pNewNode);
            kfree(pNewFd);
            break;
        }

        kQueuePush(pNewNode, pNewTable->pFdFreePool);
    }

    /* Check if we had an error */
    if(error != OS_NO_ERR)
    {
        pNewNode = kQueuePop(pNewTable->pFdFreePool);
        while(pNewNode != NULL)
        {
            kfree(pNewNode->pData);
            kQueueDestroyNode(&pNewNode);
        }

        kQueueDestroy(&pNewTable->pFdFreePool);

        newError = vectorDestroy(pNewTable->pFdTable);
        VFS_ASSERT(newError == OS_NO_ERR,
                   "Failed to destroy fd table vector",
                   newError);

        kfree(pNewTable);
    }
    else
    {
        pProcess->pFdTable = pNewTable;
        KERNEL_SPINLOCK_INIT(pNewTable->lock);
    }

    return error;
}

OS_RETURN_E vfsDestroyProcessFdTable(kernel_process_t* pProcess)
{
    OS_RETURN_E        error;
    size_t             i;
    vfs_internal_fd_t* pInternalFd;
    kqueue_node_t*     pNode;
    vfs_fd_table_t*    pTable;

    pTable = pProcess->pFdTable;

    KERNEL_LOCK(pTable->lock);

    /* Release the resources */
    for(i = 0; i < pTable->pFdTable->size; ++i)
    {
        /* Close all descriptors */
        error = vectorGet(pTable->pFdTable, i, (void**)&pNode);
        VFS_ASSERT(error == OS_NO_ERR,
                   "Failed to get fd table vector",
                   error);

        if(pNode != NULL)
        {
            pInternalFd = pNode->pData;
            /* Close the FD */
            KERNEL_LOCK(pInternalFd->pShared->lock);

            /* Release the resources if not shared */
            if(pInternalFd->pShared->refCount == 1)
            {
                if(pInternalFd->pShared->pDriver->pClose != NULL)
                {
                    pInternalFd->pShared->pDriver->pClose(
                                    pInternalFd->pShared->pDriver->pDriverData,
                                    pInternalFd->pShared->pFileHandle);
                }
                if(pInternalFd->pShared->pFilePath != NULL)
                {
                    kfree(pInternalFd->pShared->pFilePath);
                }
                kfree(pInternalFd->pShared);
            }
            else
            {
                --pInternalFd->pShared->refCount;
            }
            KERNEL_UNLOCK(pInternalFd->pShared->lock);

            /* Remove the node */
            kfree(pNode->pData);
            kQueueDestroyNode(&pNode);
        }

    }
    /* Release the free fd and the free queue */
    pNode = kQueuePop(pTable->pFdFreePool);
    while(pNode != NULL)
    {
        kfree(pNode->pData);
        kQueueDestroyNode(&pNode);
    }
    kQueueDestroy(&pTable->pFdFreePool);

    error = vectorDestroy(pTable->pFdTable);
    VFS_ASSERT(error == OS_NO_ERR,
               "Failed to destroy fd table vector",
               error);

    kfree(pTable);

    return OS_NO_ERR;
}

OS_RETURN_E vfsCopyProcessFdTable(kernel_process_t* pDstProcess,
                                  kernel_process_t* pSrcProcess)
{
    OS_RETURN_E        error;
    OS_RETURN_E        newError;
    int32_t            i;
    vfs_internal_fd_t* pNewFd;
    vfs_internal_fd_t* pSrcFd;
    kqueue_node_t*     pNewNode;
    kqueue_node_t*     pSrcNode;
    vfs_fd_table_t*    pNewTable;
    vfs_fd_table_t*    pSrcTable;

    /* Allocate the fd table */
    pNewTable = kmalloc(sizeof(vfs_fd_table_t));
    if(pNewTable == NULL)
    {
        return OS_ERR_NO_MORE_MEMORY;
    }

    pSrcTable = pSrcProcess->pFdTable;
    KERNEL_LOCK(pSrcTable->lock);
    /* Initialize the kernel file descriptor table */
    pNewTable->pFdTable = vectorCreate(VECTOR_ALLOCATOR(kmalloc, kfree),
                                       NULL,
                                       pSrcTable->pFdTable->size,
                                       &error);
    if(error != OS_NO_ERR)
    {
        KERNEL_UNLOCK(pSrcTable->lock);
        kfree(pNewTable);
        return error;
    }

    pNewTable->pFdFreePool = kQueueCreate(false);
    if(pNewTable->pFdFreePool == NULL)
    {
        KERNEL_UNLOCK(pSrcTable->lock);
        error = vectorDestroy(pNewTable->pFdTable);
        VFS_ASSERT(error == OS_NO_ERR,
                   "Failed to destroy fd table vector",
                   error);

        kfree(pNewTable);
        return OS_ERR_NO_MORE_MEMORY;
    }

    error = OS_NO_ERR;
    for(i = 0; i < (int32_t)pSrcTable->pFdTable->size; ++i)
    {
        pNewFd = kmalloc(sizeof(vfs_internal_fd_t));
        if(pNewFd == NULL)
        {
            error = OS_ERR_NO_MORE_MEMORY;
            break;
        }
        pNewFd->tableId = i;

        pNewNode = kQueueCreateNode(pNewFd, false);
        if(pNewNode == NULL)
        {
            kfree(pNewFd);
            error = OS_ERR_NO_MORE_MEMORY;
            break;
        }

        /* Check if the fd is used */
        error = vectorGet(pSrcTable->pFdTable, i, (void**)&pSrcNode);
        if(error != OS_NO_ERR)
        {
            kQueueDestroyNode(&pNewNode);
            kfree(pNewFd);
            break;
        }

        if(pSrcNode != NULL)
        {
            /* If used, copy the FD and add to the fd used table */
            pSrcFd = pSrcNode->pData;
            memcpy(pNewFd, pSrcFd, sizeof(vfs_internal_fd_t));
            KERNEL_LOCK(pNewFd->pShared->lock);
            error = vectorSet(pNewTable->pFdTable, i, pNewNode);
            if(error != OS_NO_ERR)
            {
                kQueueDestroyNode(&pNewNode);
                kfree(pNewFd);
                KERNEL_UNLOCK(pNewFd->pShared->lock);
                break;
            }
            else
            {
                /* Increment the reference count */
                ++pNewFd->pShared->refCount;
            }
            KERNEL_UNLOCK(pNewFd->pShared->lock);
        }
        else
        {
            error = vectorSet(pNewTable->pFdTable, i, NULL);
            if(error != OS_NO_ERR)
            {
                kQueueDestroyNode(&pNewNode);
                kfree(pNewFd);
                KERNEL_UNLOCK(pNewFd->pShared->lock);
                break;
            }

            /* If not used, put in the free descriptors */
            kQueuePush(pNewNode, pNewTable->pFdFreePool);
        }
    }

    /* Check if we had an error */
    if(error != OS_NO_ERR)
    {
        /* Discard the last operation that was already released */
        --i;

        /* Release the resources */
        while(i >= 0)
        {
            newError = vectorGet(pNewTable->pFdTable, i, (void**)&pNewNode);
            VFS_ASSERT(newError == OS_NO_ERR,
                       "Failed to get fd table vector",
                       newError);
            if(pNewNode != NULL)
            {
                pNewFd = pNewNode->pData;
                KERNEL_LOCK(pNewFd->pShared->lock);
                /* Release the resources if not shared */
                if(pNewFd->pShared->refCount == 1)
                {
                    if(pNewFd->pShared->pDriver->pClose != NULL)
                    {
                        pNewFd->pShared->pDriver->pClose(
                                        pNewFd->pShared->pDriver->pDriverData,
                                        pNewFd->pShared->pFileHandle);
                    }
                    if(pNewFd->pShared->pFilePath != NULL)
                    {
                        kfree(pNewFd->pShared->pFilePath);
                    }
                    kfree(pNewFd->pShared);
                }
                else
                {
                    /* Decrement the reference count */
                    --pNewFd->pShared->refCount;
                }
                KERNEL_UNLOCK(pNewFd->pShared->lock);

                kfree(pNewNode->pData);
                kQueueDestroyNode(&pNewNode);
            }

            --i;
        }

        /* Release the free fd and the free queue */
        pNewNode = kQueuePop(pNewTable->pFdFreePool);
        while(pNewNode != NULL)
        {
            kfree(pNewNode->pData);
            kQueueDestroyNode(&pNewNode);
        }
        kQueueDestroy(&pNewTable->pFdFreePool);

        newError = vectorDestroy(pNewTable->pFdTable);
        VFS_ASSERT(newError == OS_NO_ERR,
                   "Failed to destroy fd table vector",
                   newError);

        kfree(pNewTable);
    }
    else
    {
        pDstProcess->pFdTable = pNewTable;
        KERNEL_SPINLOCK_INIT(pNewTable->lock);
    }

    KERNEL_UNLOCK(pSrcTable->lock);

    return error;
}

vfs_driver_t vfsRegisterDriver(const char*      kpPath,
                               void*            pDriverData,
                               vfs_open_func_t    pOpen,
                               vfs_close_func_t   pClose,
                               vfs_read_func_t    pRead,
                               vfs_write_func_t   pWrite,
                               vfs_readdir_func_t pReadDir,
                               vfs_ioctl_func_t   pIOCTL)
{
    vfs_driver_t           newDriver;
    vfs_driver_internal_t* pInternalHandle;
    size_t                 pathLen;
    char*                  pCleanPath;

    if(kpPath == NULL)
    {
        return VFS_DRIVER_INVALID;
    }

    pathLen = strlen(kpPath);
    if(pathLen == 0)
    {
        return VFS_DRIVER_INVALID;
    }

    /* Allocate the clean path buffer */
    pCleanPath = kmalloc(pathLen + 1);
    if(pCleanPath == NULL)
    {
        return VFS_DRIVER_INVALID;
    }
    _cleanPath(pCleanPath, kpPath);
    pathLen = strlen(pCleanPath);

    pCleanPath[pathLen] = VFS_PATH_DELIMITER;
    pCleanPath[pathLen + 1] = 0;

    KERNEL_LOCK(sMountPointLock);

    /* Check something is already mounted in the mount point */
    if(_findNodeFromPath(spRootPoint, pCleanPath, pathLen, true, true) != NULL)
    {
        KERNEL_UNLOCK(sMountPointLock);
        kfree(pCleanPath);
        return VFS_DRIVER_INVALID;
    }

    /* Allocate the internal handle and setup */
    pInternalHandle = kmalloc(sizeof(vfs_driver_internal_t));
    if(pInternalHandle == NULL)
    {
        KERNEL_UNLOCK(sMountPointLock);
        kfree(pCleanPath);
        return VFS_DRIVER_INVALID;
    }
    pInternalHandle->pDriverData = pDriverData;
    pInternalHandle->pOpen       = pOpen;
    pInternalHandle->pClose      = pClose;
    pInternalHandle->pRead       = pRead;
    pInternalHandle->pWrite      = pWrite;
    pInternalHandle->pReadDir    = pReadDir;
    pInternalHandle->pIOCTL      = pIOCTL;
    pInternalHandle->pUnmount    = NULL;
    /* Add the node */
    newDriver = (vfs_driver_t)_vfsAddDriver(pCleanPath, pInternalHandle);

    /* On error, free memory */
    if(newDriver == VFS_DRIVER_INVALID)
    {
        kfree(pInternalHandle);
    }

    KERNEL_UNLOCK(sMountPointLock);

    kfree(pCleanPath);

    return newDriver;
}

OS_RETURN_E vfsUnregisterDriver(vfs_driver_t* pDriver)
{
    vfs_node_t* pNode;
    OS_RETURN_E error;

    if(pDriver == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    KERNEL_LOCK(sMountPointLock);

    if(*pDriver == VFS_DRIVER_INVALID)
    {
        return OS_ERR_NO_SUCH_ID;
    }

    /* Remove the node */
    pNode = (vfs_node_t*)(*pDriver);
    error = _vfsRemoveDriver(pNode);
    if(error == OS_NO_ERR)
    {
        /* Update the parameter */
        *pDriver = VFS_DRIVER_INVALID;
    }

    KERNEL_UNLOCK(sMountPointLock);

    return error;
}

int32_t vfsOpen(const char* kpPath, int flags, int mode)
{
    vfs_node_t*            pMountPt;
    vfs_driver_internal_t* pDriver;
    void*                  pHandle;
    int32_t                fd;
    size_t                 pathSize;
    char*                  pCleanPath;
    vfs_fd_table_t*        pTable;

    pathSize = strlen(kpPath);
    pCleanPath = kmalloc(pathSize);
    if(pCleanPath == NULL)
    {
        return -1;
    }
    pathSize = _cleanPath(pCleanPath, kpPath);

    /* Find the file mount point */
    pMountPt = _findNodeFromPath(spRootPoint,
                                 pCleanPath,
                                 pathSize,
                                 true,
                                 false);

    /* If there is a driver */
    if(pMountPt != NULL)
    {
        /* Redirect the value */
        if(pMountPt->pDriver->pOpen == NULL)
        {
            kfree(pCleanPath);
            return -1;
        }

        if(pathSize >= pMountPt->mountPointOffset)
        {
            pathSize = pMountPt->mountPointOffset;
        }
        pDriver = pMountPt->pDriver;
    }
    else
    {
        /* Check if this is an exact node in the mount points */
        pMountPt = _findNodeFromPath(spRootPoint,
                                     pCleanPath,
                                     pathSize,
                                     false,
                                     true);
        if(pMountPt != NULL)
        {
            /* Handle with the generic VFS driver */
            pDriver = &sVfsGenericDriver;
            pathSize = 0;
        }
        else
        {
            kfree(pCleanPath);
            return -1;
        }
    }

    pHandle = pDriver->pOpen(pDriver->pDriverData,
                             pCleanPath + pathSize,
                             flags,
                             mode);
    /* Failed to open */
    if(pHandle == (void*)-1)
    {
        kfree(pCleanPath);
        return -1;
    }

    /* Get a free file descriptor and set its attributes */
    pTable = schedGetCurrentProcess()->pFdTable;
    fd = _createFileDescriptor(pTable,
                               pDriver,
                               pHandle,
                               pCleanPath,
                               flags,
                               mode);
    if(fd < 0)
    {
        /* Close the file and return */
        if(pDriver->pClose != NULL)
        {
            pDriver->pClose(pDriver->pDriverData, pHandle);
        }
        kfree(pCleanPath);
        return -1;
    }

    kfree(pCleanPath);
    return fd;
}

int32_t vfsClose(int32_t fd)
{
    vfs_fd_table_t*    pTable;
    vfs_internal_fd_t* pInternalFd;
    OS_RETURN_E        error;
    int32_t            retVal;

    pTable = schedGetCurrentProcess()->pFdTable;

    KERNEL_LOCK(pTable->lock);

    /* Check that the FD is valid */
    error = _getFd(pTable, fd, &pInternalFd);
    if(error != OS_NO_ERR)
    {
        return -1;
    }

    /* Lock and check is shared */
    KERNEL_LOCK(pInternalFd->pShared->lock);
    /* Release the resources if not shared */
    if(pInternalFd->pShared->refCount == 1)
    {
        /* Close to the driver */
        if(pInternalFd->pShared->pDriver->pClose != NULL)
        {
            retVal = pInternalFd->pShared->pDriver->pClose(
                                    pInternalFd->pShared->pDriver->pDriverData,
                                    pInternalFd->pShared->pFileHandle);
        }
        else
        {
            retVal = 0;
        }
    }
    else
    {
        retVal = 0;
    }
    --pInternalFd->pShared->refCount;
    KERNEL_UNLOCK(pInternalFd->pShared->lock);

    if(retVal < 0)
    {
        KERNEL_UNLOCK(pTable->lock);
        return -1;
    }

    /* Release the FD on success */
    _releaseFileDescriptor(pTable, fd);

    KERNEL_UNLOCK(pTable->lock);

    return 0;
}

ssize_t vfsRead(int32_t fd, void* pBuffer, size_t count)
{
    OS_RETURN_E            error;
    ssize_t                bytesRead;
    vfs_internal_fd_t*     pInternalFd;
    vfs_driver_internal_t* pDriver;
    vfs_fd_table_t*        pTable;

    pTable = schedGetCurrentProcess()->pFdTable;

    KERNEL_LOCK(pTable->lock);
    error = _getFd(pTable, fd, &pInternalFd);
    KERNEL_UNLOCK(pTable->lock);

    if(error != OS_NO_ERR)
    {
        return -1;
    }

    if((pInternalFd->openFlags & VFS_PERM_READ) == 0)
    {
        return -1;
    }

    pDriver = pInternalFd->pShared->pDriver;

    if(pDriver->pRead != NULL)
    {
        bytesRead = pDriver->pRead(pDriver->pDriverData,
                                   pInternalFd->pShared->pFileHandle,
                                   pBuffer,
                                   count);
    }
    else
    {
        bytesRead = -1;
    }

    return bytesRead;
}

ssize_t vfsWrite(int32_t fd, const void* pBuffer, size_t count)
{
    OS_RETURN_E            error;
    ssize_t                bytesWritten;
    vfs_internal_fd_t*     pInternalFd;
    vfs_driver_internal_t* pDriver;
    vfs_fd_table_t*        pTable;

    pTable = schedGetCurrentProcess()->pFdTable;

    KERNEL_LOCK(pTable->lock);
    error = _getFd(pTable, fd, &pInternalFd);
    KERNEL_UNLOCK(pTable->lock);

    if(error != OS_NO_ERR)
    {
        return -1;
    }

    if((pInternalFd->openFlags & VFS_PERM_WRITE) == 0)
    {
        return -1;
    }

    pDriver = pInternalFd->pShared->pDriver;

    if(pDriver->pWrite != NULL)
    {
        bytesWritten = pDriver->pWrite(pDriver->pDriverData,
                                       pInternalFd->pShared->pFileHandle,
                                       pBuffer,
                                       count);
    }
    else
    {
        bytesWritten = -1;
    }

    return bytesWritten;
}

int32_t vfsReaddir(int32_t fd, dirent_t* pDirEntry)
{
    OS_RETURN_E            error;
    ssize_t                retVal;
    vfs_internal_fd_t*     pInternalFd;
    vfs_driver_internal_t* pDriver;
    vfs_fd_table_t*        pTable;

    pTable = schedGetCurrentProcess()->pFdTable;

    KERNEL_LOCK(pTable->lock);
    error = _getFd(pTable, fd, &pInternalFd);
    KERNEL_UNLOCK(pTable->lock);

    if(error != OS_NO_ERR)
    {
        return -1;
    }

    if((pInternalFd->openFlags & VFS_PERM_READ) == 0)
    {
        return -1;
    }

    pDriver = pInternalFd->pShared->pDriver;

    KERNEL_UNLOCK(pTable->lock);

    if(pDriver->pReadDir != NULL)
    {
        retVal = pDriver->pReadDir(pDriver->pDriverData,
                                   pInternalFd->pShared->pFileHandle,
                                   pDirEntry);
    }
    else
    {
        retVal = -1;
    }

    return retVal;
}

int32_t vfsIOCTL(int32_t fd, uint32_t operation, void* pArgs)
{
    OS_RETURN_E            error;
    ssize_t                retVal;
    vfs_internal_fd_t*     pInternalFd;
    vfs_driver_internal_t* pDriver;
    vfs_fd_table_t*        pTable;

    pTable = schedGetCurrentProcess()->pFdTable;

    KERNEL_LOCK(pTable->lock);
    error = _getFd(pTable, fd, &pInternalFd);
    KERNEL_UNLOCK(pTable->lock);

    if(error != OS_NO_ERR)
    {
        return -1;
    }

    if((pInternalFd->openFlags & VFS_PERM_READ) == 0)
    {
        KERNEL_UNLOCK(pTable->lock);
        return -1;
    }

    pDriver = pInternalFd->pShared->pDriver;

    KERNEL_UNLOCK(pTable->lock);

    if(pDriver->pIOCTL != NULL)
    {
        retVal = pDriver->pIOCTL(pDriver->pDriverData,
                                 pInternalFd->pShared->pFileHandle,
                                 operation,
                                 pArgs);
    }
    else
    {
        retVal = -1;
    }

    return retVal;
}

OS_RETURN_E vfsMount(const char* kpPath,
                     const char* kpDevPath,
                     const char* kpFsName)
{
    fs_driver_t* pDriver;
    uintptr_t    driverTableCursor;
    OS_RETURN_E  retCode;
    void*        pDriverMountData;
    vfs_node_t*  pNewNode;

    /* Search for the FS if provided */
    if(kpFsName != NULL)
    {
#if VFS_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "Searching FS driver for %s",
               kpFsName);
#endif

        /* Display list of registered drivers */
        driverTableCursor = (uintptr_t)&_START_FS_TABLE_ADDR;
        pDriver = *(fs_driver_t**)driverTableCursor;

        while(pDriver != NULL)
        {
#if VFS_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Searching FS driver for %s, comparing %s",
                   kpFsName,
                   pDriver->pName);
#endif
            if(strcmp(pDriver->pName, kpFsName) == 0)
            {
                break;
            }
            driverTableCursor += sizeof(uintptr_t);
            pDriver = *(fs_driver_t**)driverTableCursor;
        }

        if(pDriver == NULL)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Could not find FS %s.",
                   kpFsName);
            return OS_ERR_NO_SUCH_ID;
        }

        /* Mount with driver */
        retCode = pDriver->pMount(kpPath, kpDevPath, &pDriverMountData);
    }
    else
    {
        retCode = OS_ERR_NOT_SUPPORTED;

        /* Display list of registered drivers */
        driverTableCursor = (uintptr_t)&_START_FS_TABLE_ADDR;
        pDriver = *(fs_driver_t**)driverTableCursor;

        while(pDriver != NULL)
        {
#if VFS_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Mounting FS driver for %s: %s",
                   kpDevPath,
                   pDriver->pName);
#endif
            retCode = pDriver->pMount(kpPath, kpDevPath, &pDriverMountData);
            if(retCode == OS_NO_ERR)
            {
                break;
            }
            driverTableCursor += sizeof(uintptr_t);
            pDriver = *(fs_driver_t**)driverTableCursor;
        }
        if(pDriver == NULL)
        {
            retCode = OS_ERR_NOT_SUPPORTED;
        }
    }

    /* On mount success, add the driver */
    if(retCode == OS_NO_ERR)
    {
        pNewNode = vfsRegisterDriver(kpPath,
                                     pDriverMountData,
                                     pDriver->pOpen,
                                     pDriver->pClose,
                                     pDriver->pRead,
                                     pDriver->pWrite,
                                     pDriver->pReadDir,
                                     pDriver->pIOCTL);

        if(pNewNode != VFS_DRIVER_INVALID)
        {
            /* Add the driver data for unmount */
            pNewNode->pDriver->pUnmount = pDriver->pUnmount;
        }
        else
        {
            retCode = OS_ERR_NOT_SUPPORTED;
        }
    }

    return retCode;
}

OS_RETURN_E vfsUnmount(const char* kpPath)
{
    size_t                 pathLen;
    char*                  pCleanPath;
    bool                   addDelimiter;
    vfs_node_t*            pDriverNode;
    OS_RETURN_E            retCode;

    if(kpPath == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    pathLen = strlen(kpPath);
    if(pathLen == 0)
    {
        return OS_ERR_NO_SUCH_ID;
    }

    /* Check trailing delimiter*/
    if(kpPath[pathLen - 1] != VFS_PATH_DELIMITER)
    {
        ++pathLen;
        addDelimiter = true;
    }
    else
    {
        addDelimiter = false;
    }

    /* Allocate the clean path buffer */
    pCleanPath = kmalloc(pathLen + 1);
    if(pCleanPath == NULL)
    {
        return OS_ERR_NO_MORE_MEMORY;
    }
    _cleanPath(pCleanPath, kpPath);
    pathLen = strlen(pCleanPath);

    if(addDelimiter == true)
    {
        pCleanPath[pathLen]     = VFS_PATH_DELIMITER;
        pCleanPath[pathLen + 1] = 0;
    }

    KERNEL_LOCK(sMountPointLock);

    /* Check something is already mounted in the mount point */
    pDriverNode = _findNodeFromPath(spRootPoint,
                                    pCleanPath,
                                    pathLen,
                                    true,
                                    true);
    kfree(pCleanPath);
    KERNEL_UNLOCK(sMountPointLock);
    if(pDriverNode == NULL)
    {
        return OS_ERR_NO_SUCH_ID;
    }

    /* Call umount if exists */
    if(pDriverNode->pDriver->pUnmount != NULL)
    {
        retCode =
            pDriverNode->pDriver->pUnmount(pDriverNode->pDriver->pDriverData);
    }
    else
    {
        retCode = OS_NO_ERR;
    }

    /* Remove driver */
    if(retCode == OS_NO_ERR)
    {
        retCode = vfsUnregisterDriver((vfs_driver_t*)pDriverNode);
    }

    return retCode;
}
/************************************ EOF *************************************/