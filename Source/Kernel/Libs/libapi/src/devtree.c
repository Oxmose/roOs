/*******************************************************************************
 * @file devtree.c
 *
 * @see devtree.h
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

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <stddef.h>        /* Standard defined types */
#include <stdint.h>        /* Standard int types and bool */
#include <kerneloutput.h> /* Kernel logging */
#include <panic.h>         /* Kernel panic */
#include <kerror.h>        /* Kernel error codes */
#include <kheap.h>         /* Kernel heap */
#include <string.h>        /* Memory manipulation */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <devtree.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "DEVTREE"

/** @brief FDT Magic number */
#define FDT_MAGIC_NUMBER 0xD00DFEED

/** @brief FDT Begin node identifier */
#define FDT_BEGIN_NODE 0x00000001

/** @brief FDT End node identifier */
#define FDT_END_NODE 0x00000002

/** @brief FDT Property identifier */
#define FDT_PROP 0x00000003

#define INIT_ADDR_CELLS 2
#define INIT_SIZE_CELLS 1

#define FDT_CELL_SIZE sizeof(uint32_t)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief File Device Tree Header structure. */
typedef struct
{
    uint32_t magic;
    uint32_t size;
    uint32_t offStructs;
    uint32_t offStrings;
    uint32_t offMemRsvMap;
    uint32_t version;
    uint32_t lastCompatVersion;
    uint32_t bootCPUId;
    uint32_t sizeStrings;
    uint32_t sizeStructs;
} fdt_header_t;

/** @brief pHandle struct */
typedef struct phandle_t
{
    uint32_t id;
    void*    pLink;

    struct phandle_t* pNext;
} phandle_t;

/** @brief Internal descriptor for the FDT */
typedef struct
{
    uint32_t  nbStructs;

    uint32_t*   pStructs;
    const char* pStrings;

    fdt_node_t* pFirstNode;

    phandle_t* pHandleList;
} fdt_descriptor_t;

/** @brief Specific property actions */
typedef struct
{
    const char*       pName;
    const size_t      nameSize;
    void        (*pAction)(fdt_node_t*, fdt_property_t*);
} specprop_action_t;

/** @brief Specific property table */
typedef struct
{
    uint32_t          numberOfProps;
    specprop_action_t pSpecProps[3];
} specprop_table_t;


/*******************************************************************************
 * MACROS
 ******************************************************************************/

#define ALIGN(VAL, ALIGN_V) \
        ((((VAL) + ((ALIGN_V) - 1)) / (ALIGN_V)) * (ALIGN_V))


/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

static inline void linkNode(fdt_node_t* pNode, fdt_node_t* pNextNode);
static inline void linkProperty(fdt_property_t* pProp,
                                fdt_property_t* pLinkProp);

static fdt_node_t* parseNode(uint32_t* pOffset,
                             const uint8_t kAddrCells,
                             const uint8_t kSizeCells);
static fdt_property_t* parseProperty(uint32_t* pOffset,
                                     fdt_node_t* pNode);
static void applyPropertyAction(fdt_node_t* pNode, fdt_property_t* pProperty);
static void applyActionAddressCells(fdt_node_t* pNode,
                                    fdt_property_t* pProperty);
static void applyActionSizeCells(fdt_node_t* pNode, fdt_property_t* pProperty);
static void applyActionPhandle(fdt_node_t* pNode,
                               fdt_property_t* pProperty);
static inline const void* fdtInternalReadProperty(const fdt_property_t* pProperty,
                                                  size_t* pReadSize);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
fdt_descriptor_t fdtDesc;

specprop_table_t specPropTable = {
    .numberOfProps = 3,
    .pSpecProps = {
        {"phandle", 7, applyActionPhandle},
        {"#address-cells", 14, applyActionAddressCells},
        {"#size-cells", 11, applyActionSizeCells},
    }
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static inline void linkNode(fdt_node_t* pNode, fdt_node_t* pLinkNode)
{
    while(pNode->pNextNode != NULL)
    {
        pNode = pNode->pNextNode;
    }
    pLinkNode->pNextNode = NULL;
    pNode->pNextNode = pLinkNode;
}

static inline void linkProperty(fdt_property_t* pProp,
                                fdt_property_t* pLinkProp)
{
    while(pProp->pNextProp != NULL)
    {
        pProp = pProp->pNextProp;
    }
    pLinkProp->pNextProp = NULL;
    pProp->pNextProp = pLinkProp;
}

static inline const void* fdtInternalReadProperty(const fdt_property_t* pProperty,
                                                  size_t* pReadSize)
{
    if(pProperty == NULL)
    {
        if(pReadSize != NULL)
        {
            *pReadSize = 0;
        }
        return NULL;
    }

    if(pReadSize != NULL)
    {
        *pReadSize = pProperty->length;
    }

    return pProperty->pCells;
}

static void applyActionPhandle(fdt_node_t* pNode,
                               fdt_property_t* pProperty)
{
    phandle_t* pNewHandle;

    pNewHandle = kmalloc(sizeof(phandle_t));
    if(pNewHandle == NULL)
    {
        PANIC(OS_ERR_NO_MORE_MEMORY,
              MODULE_NAME,
              "Failed to allocate new handle",
              TRUE);
    }

    pNewHandle->id    = FDTTOCPU32(*pProperty->pCells);
    pNewHandle->pLink = (void*)pNode;
    pNewHandle->pNext = fdtDesc.pHandleList;
    fdtDesc.pHandleList = pNewHandle;

    KERNEL_DEBUG(DTB_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Added new handle for %s: %d",
                 pNode->pName,
                 pNewHandle->id);
}

static void applyActionAddressCells(fdt_node_t* pNode,
                                    fdt_property_t* pProperty)
{
    uint32_t        propertySize;
    const uint32_t* pPropertyPtr;

    propertySize = sizeof(uint32_t);
    pPropertyPtr = fdtInternalReadProperty(pProperty, &propertySize);
    if(propertySize == sizeof(uint32_t))
    {
        pNode->addrCells = FDTTOCPU32(*pPropertyPtr);
    }
    else
    {
        KERNEL_ERROR("Incorrect read size in property\n");
    }
    KERNEL_DEBUG(DTB_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Address size is now %d",
                 pNode->addrCells);
}

static void applyActionSizeCells(fdt_node_t* pNode,
                                 fdt_property_t* pProperty)
{
    uint32_t        propertySize;
    const uint32_t* pPropertyPtr;

    propertySize = sizeof(uint32_t);
    pPropertyPtr = fdtInternalReadProperty(pProperty, &propertySize);
    if(propertySize == sizeof(uint32_t))
    {
        pNode->sizeCells = FDTTOCPU32(*pPropertyPtr);
    }
    else
    {
        KERNEL_ERROR("Incorrect read size in property\n");
    }

    KERNEL_DEBUG(DTB_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Size size is now %d",
                 pNode->sizeCells);
}

static void applyPropertyAction(fdt_node_t* pNode, fdt_property_t* pProperty)
{
    size_t i;
    size_t propNameLength;

    propNameLength = strlen(pProperty->pName);

    /* Check all properties */
    for(i = 0; i < specPropTable.numberOfProps; ++i)
    {
        if(propNameLength == specPropTable.pSpecProps[i].nameSize &&
           specPropTable.pSpecProps[i].pAction != NULL &&
           strcmp(pProperty->pName, specPropTable.pSpecProps[i].pName) == 0)
        {
            specPropTable.pSpecProps[i].pAction(pNode, pProperty);
        }
    }
}

static fdt_property_t* parseProperty(uint32_t* pOffset,
                                     fdt_node_t* pNode)
{
    fdt_property_t* pProperty;
    const char*     pName;
    size_t          length;
    const uint32_t* pInitProperty;

    /* Check if start property */
    if(FDTTOCPU32(fdtDesc.pStructs[*pOffset]) != FDT_PROP)
    {
        return NULL;
    }
    ++(*pOffset);

    /* Allocate new property */
    pProperty = kmalloc(sizeof(fdt_property_t));
    if(pProperty == NULL)
    {
        PANIC(OS_ERR_NO_MORE_MEMORY,
              MODULE_NAME,
              "Failed to allocate new property",
              TRUE);
    }
    memset(pProperty, 0, sizeof(fdt_property_t));


    pInitProperty = (uint32_t*)&fdtDesc.pStructs[*pOffset];
    *pOffset += 2;

    /* Get the length and name */
    pProperty->length = FDTTOCPU32(pInitProperty[0]);
    pName = fdtDesc.pStrings + FDTTOCPU32(pInitProperty[1]);
    length = strlen(pName);

    pProperty->pName = kmalloc(length + 1);
    if(pProperty->pName == NULL)
    {
        PANIC(OS_ERR_NO_MORE_MEMORY,
              MODULE_NAME,
              "Failed to allocate new string",
              TRUE);
    }
    memcpy(pProperty->pName, pName, length);
    pProperty->pName[length] = 0;

    /* Copy the property cells */
    if(pProperty->length != 0)
    {
        pProperty->pCells = kmalloc(pProperty->length);
        if(pProperty->pCells == NULL)
        {
            PANIC(OS_ERR_NO_MORE_MEMORY,
                MODULE_NAME,
                "Failed to allocate new property cells",
                TRUE);
        }
        memcpy(pProperty->pCells, fdtDesc.pStructs + *pOffset, pProperty->length);
    }
    else
    {
        pProperty->pCells = NULL;
    }

    KERNEL_DEBUG(DTB_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Read property %s of length %u",
                 pProperty->pName,
                 pProperty->length);

    *pOffset += ALIGN(pProperty->length, FDT_CELL_SIZE) / FDT_CELL_SIZE;

    applyPropertyAction(pNode, pProperty);

    return pProperty;
}

static fdt_node_t* parseNode(uint32_t* pOffset,
                             const uint8_t kAddrCells,
                             const uint8_t kSizeCells)
{
    fdt_node_t*     pNode;
    fdt_node_t*     pChildNode;
    fdt_property_t* pProperty;
    const char*     pInitName;
    size_t          length;
    uint32_t        cursor;

    /* Check if start node */
    if(FDTTOCPU32(fdtDesc.pStructs[*pOffset]) != FDT_BEGIN_NODE)
    {
        return NULL;
    }

    ++(*pOffset);

    /* Allocate new node */
    pNode = kmalloc(sizeof(fdt_node_t));
    if(pNode == NULL)
    {
        PANIC(OS_ERR_NO_MORE_MEMORY,
              MODULE_NAME,
              "Failed to allocate new node",
              TRUE);
    }
    memset(pNode, 0, sizeof(fdt_node_t));

    pNode->addrCells = kAddrCells;
    pNode->sizeCells = kSizeCells;

    /* Get name and copy */
    pInitName = (char*)&fdtDesc.pStructs[*pOffset];
    length    = strlen(pInitName);

    pNode->pName = kmalloc(length + 1);
    if(pNode->pName == NULL)
    {
        PANIC(OS_ERR_NO_MORE_MEMORY,
              MODULE_NAME,
              "Failed to allocate new string",
              TRUE);
    }
    memcpy(pNode->pName, pInitName, length);
    pNode->pName[length] = 0;

    KERNEL_DEBUG(DTB_DEBUG_ENABLED, MODULE_NAME, "Read node %s", pNode->pName);

    /* Update offset */
    *pOffset += ALIGN(length + 1, FDT_CELL_SIZE) / FDT_CELL_SIZE;

    /* Walk the node until we reache the number of structs */
    while(fdtDesc.nbStructs > *pOffset)
    {
        cursor = FDTTOCPU32(fdtDesc.pStructs[*pOffset]);
        if(cursor == FDT_BEGIN_NODE)
        {
            /* New Child */
            pChildNode = parseNode(pOffset, pNode->addrCells, pNode->sizeCells);
            if(pChildNode != NULL)
            {
                if(pNode->pFirstChildNode == NULL)
                {
                    pChildNode->pNextNode = NULL;
                    pNode->pFirstChildNode = pChildNode;
                }
                else
                {
                    linkNode(pNode->pFirstChildNode, pChildNode);
                }
            }
        }
        else if(cursor == FDT_PROP)
        {
            pProperty = parseProperty(pOffset, pNode);
            if(pProperty != NULL)
            {
                if(pNode->pProps == NULL)
                {
                    pProperty->pNextProp = NULL;
                    pNode->pProps = pProperty;
                }
                else
                {
                    linkProperty(pNode->pProps, pProperty);
                }
            }
        }
        else
        {
            ++(*pOffset);
            if(cursor == FDT_END_NODE)
            {
                return pNode;
            }
        }
    }

    KERNEL_ERROR("Unexpected end of node\n");

    kfree(pNode->pName);
    kfree(pNode);

    return NULL;
}

void fdtInit(const uintptr_t kStartAddr)
{
    uint32_t i;

    const fdt_header_t* pHeader;
    fdt_node_t*         pNode;

    KERNEL_DEBUG(DTB_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Initializing device tree from 0x%p",
                 kStartAddr);

    pHeader = (fdt_header_t*)kStartAddr;

    /* Check magic */
    if(FDTTOCPU32(pHeader->magic) != FDT_MAGIC_NUMBER)
    {
        PANIC(OS_ERR_INCORRECT_VALUE,
              MODULE_NAME,
              "Invalid FDT magic",
              TRUE);
    }

    memset(&fdtDesc, 0, sizeof(fdt_descriptor_t));

    /* Get the structs and strings addresses */
    fdtDesc.pStructs = (uint32_t*)(kStartAddr + FDTTOCPU32(pHeader->offStructs));
    fdtDesc.pStrings = (char*)(kStartAddr + FDTTOCPU32(pHeader->offStrings));

    fdtDesc.nbStructs = FDTTOCPU32(pHeader->sizeStructs) / sizeof(uint32_t);

    /* Now start parsing the first level */
    for(i = 0; i < fdtDesc.nbStructs; ++i)
    {
        /* Get the node and add to root */
        pNode = parseNode(&i, INIT_ADDR_CELLS, INIT_SIZE_CELLS);
        if(pNode != NULL)
        {
            /* Link node */
            if(fdtDesc.pFirstNode == NULL)
            {
                pNode->pNextNode = NULL;
                fdtDesc.pFirstNode = pNode;
            }
            else
            {
                linkNode(fdtDesc.pFirstNode, pNode);
            }
        }
    }

    TEST_POINT_FUNCTION_CALL(devtreeTest, TEST_DEVTREE_ENABLED);
}

const void* fdtGetProp(const fdt_node_t* pkFdtNode,
                       const char* pkName,
                       size_t* pReadSize)
{
    const fdt_property_t* pProp;
    void*                 retVal;

    if(pkFdtNode == NULL || pkName == NULL)
    {
        if(pReadSize != NULL)
        {
            *pReadSize = 0;
        }
        return NULL;
    }

    pProp  = pkFdtNode->pProps;
    retVal = NULL;
    while(pProp != NULL)
    {
        /* Check name */
        if(strcmp(pProp->pName, pkName) == 0)
        {
            /* Setup return, on NULL to tell the property is here, return 1 */
            if(pProp->pCells == NULL)
            {
                retVal = (void*)1;
                if(pReadSize != NULL)
                {
                    *pReadSize = 0;
                }
            }
            else
            {
                retVal = (void*)pProp->pCells;
                if(pReadSize != NULL)
                {
                    *pReadSize = pProp->length;
                }
            }

            break;
        }

        pProp = pProp->pNextProp;
    }

    return retVal;
}

const fdt_node_t* fdtGetRoot(void)
{
    return fdtDesc.pFirstNode;
}

const fdt_node_t* fdtGetNextNode(const fdt_node_t* pkNode)
{
    if(pkNode != NULL)
    {
        return pkNode->pNextNode;
    }

    return NULL;
}

const fdt_node_t* fdtGetChild(const fdt_node_t* pkNode)
{
    if(pkNode != NULL)
    {
        return pkNode->pFirstChildNode;
    }

    return NULL;
}

const fdt_property_t* fdtGetFirstProp(const fdt_node_t* pkNode)
{
    if(pkNode != NULL)
    {
        return pkNode->pProps;
    }

    return NULL;
}
const fdt_property_t* fdtGetNextProp(const fdt_property_t* pkProp)
{
    if(pkProp != NULL)
    {
        return pkProp->pNextProp;
    }

    return NULL;
}

const fdt_node_t* fdtGetNodeByHandle(const uint32_t kHandleId)
{
    const phandle_t* pHandle;

    pHandle = fdtDesc.pHandleList;
    while(pHandle != NULL)
    {
        if(pHandle->id == kHandleId)
        {
            return (fdt_node_t*)pHandle->pLink;
        }
        pHandle = pHandle->pNext;
    }

    return NULL;
}

bool_t fdtMatchCompatible(const fdt_node_t* pkNode, const char* pkCompatible)
{
    const char* pkCompatibleProp;
    size_t      propLen;

    if(pkNode == NULL || pkCompatible == NULL)
    {
        return FALSE;
    }

    /* Get comaptible and check */
    propLen = sizeof(char*);
    pkCompatibleProp = fdtGetProp(pkNode, "compatible", &propLen);

    if(pkCompatibleProp != NULL && propLen > 0)
    {
        if(strncmp(pkCompatibleProp, pkCompatible, propLen) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/************************************ EOF *************************************/