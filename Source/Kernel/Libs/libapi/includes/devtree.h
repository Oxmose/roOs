/*******************************************************************************
 * @file devtree.h
 *
 * @see devtree.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 22/05/2024
 *
 * @version 1.0
 *
 * @brief Device file tree driver.
 *
 * @details  This file provides the device file tree driver used by UTK.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_DEVTREE_H_
#define __LIB_DEVTREE_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stddef.h> /* Standard defined types */
#include <stdint.h> /* Standard int types and bool */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief FDT property descriptor */
typedef struct fdt_property_t
{
    char*     pName;
    size_t    length;
    uint32_t* pCells;

    struct fdt_property_t* pNextProp;
} fdt_property_t;

/** @brief FDT node descriptor */
typedef struct fdt_node_t
{
    char*   pName;
    uint8_t addrCells;
    uint8_t sizeCells;

    fdt_property_t* pProps;

    struct fdt_node_t* pParentNode;
    struct fdt_node_t* pNextNode;
    struct fdt_node_t* pFirstChildNode;
} fdt_node_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define FDTTOCPU32(X)
#define FDTTOCPU64(X)
#else
#define FDTTOCPU32(X) (__builtin_bswap32(X))
#define FDTTOCPU64(X) (__builtin_bswap64(X))
#endif

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

void fdtInit(const uintptr_t kStartAddr);

const void* fdtGetProp(const fdt_node_t* pkFdtNode,
                       const char* pkName,
                       size_t* pReadSize);

const fdt_node_t* fdtGetRoot(void);
const fdt_node_t* fdtGetNextNode(const fdt_node_t* pkNode);
const fdt_node_t* fdtGetChild(const fdt_node_t* pkNode);
const fdt_property_t* fdtGetFirstProp(const fdt_node_t* pkNode);
const fdt_property_t* fdtGetNextProp(const fdt_property_t* pkProp);
const fdt_node_t* fdtGetNodeByHandle(const uint32_t kHandleId);

bool_t fdtMatchCompatible(const fdt_node_t* pkNode, const char* pkCompatible);

#endif /* #ifndef __LIB_DEVTREE_H_ */

/************************************ EOF *************************************/