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
#include <kheap.h>        /* Kernel heap */
#include <panic.h>        /* Kernel panic */
#include <stddef.h>       /* Standard defined types */
#include <stdint.h>       /* Standard int types and bool */
#include <kerror.h>       /* Kernel error codes */
#include <string.h>       /* Memory manipulation */
#include <kerneloutput.h> /* Kernel logging */

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
    /** @brief FDT Magic number */
    uint32_t magic;
    /** @brief FDT total size  */
    uint32_t size;
    /** @brief FDT structures offset */
    uint32_t offStructs;
    /** @brief FDT strings offset */
    uint32_t offStrings;
    /** @brief FDT reserved memory map */
    uint32_t offMemRsvMap;
    /** @brief FDT version */
    uint32_t version;
    /** @brief FDT last compatible version */
    uint32_t lastCompatVersion;
    /** @brief FDT boot CPU identifier */
    uint32_t bootCPUId;
    /** @brief FDT string table size */
    uint32_t sizeStrings;
    /** @brief FDT structures table size */
    uint32_t sizeStructs;
} fdt_header_t;

/** @brief pHandle struct */
typedef struct phandle_t
{
    /** @brief pHandle identifier */
    uint32_t id;
    /** @brief Link to the node */
    void*    pLink;

    /** @brief Next phandle in the list */
    struct phandle_t* pNext;
} phandle_t;

/** @brief Internal descriptor for the FDT */
typedef struct
{

    /** @brief Number of structures */
    uint32_t  nbStructs;

    /** @brief Struct table pointer */
    uint32_t*   pStructs;
    /** @brief String table pointer */
    const char* pStrings;

    /** @brief First root node of the FDT */
    fdt_node_t* pFirstNode;

    /** @brief FDT pHandle list */
    phandle_t* pHandleList;
} fdt_descriptor_t;

/** @brief Specific property actions */
typedef struct
{
    /** @brief Action name (property name) */
    const char*  pName;

    /** @brief Property name size */
    const size_t nameSize;

    /** @brief Action function pointer */
    void         (*pAction)(fdt_node_t*, fdt_property_t*);
} specprop_action_t;

/** @brief Specific property table */
typedef struct
{
    /** @brief Number of properties with specific action. */
    uint32_t          numberOfProps;
    /** @brief Table of actions */
    specprop_action_t pSpecProps[3];
} specprop_table_t;


/*******************************************************************************
 * MACROS
 ******************************************************************************/

/** @brief Align memory with FDT boundaries. */
#define ALIGN(VAL, ALIGN_V) \
        ((((VAL) + ((ALIGN_V) - 1)) / (ALIGN_V)) * (ALIGN_V))


/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Links a node to another in the FDT tree.
 *
 * @details Links a node to another in the FDT tree. This is used to reverse the
 * FDT read order (First Read Last Seen).
 *
 * @param[in, out] pNode The node to link.
 * @param[in, out] pNextNode The next node to link.
*/
static inline void linkNode(fdt_node_t* pNode, fdt_node_t* pNextNode);

/**
 * @brief Links a property to another in the FDT tree.
 *
 * @details Links a property to another in the FDT tree. This is used to reverse
 * the FDT read order (First Read Last Seen).
 *
 * @param[in, out] pProp The property to link.
 * @param[in, out] pLinkProp The next property to link.
*/
static inline void linkProperty(fdt_property_t* pProp,
                                fdt_property_t* pLinkProp);

/**
 * @brief Parses a node in the FDT.
 *
 * @details Parses a node in the FDT based on the current offset. The function
 * populates the node and returns it.
 *
 * @param[in, out] pOffset The current offset in the FDT. The offset is updated
 * by this function.
 * @param[in] kAddrCells Current address-cells value.
 * @param[in] kSizeCells Current size-cells value.
 *
 * @returns A new node parsed from the FDT is returned.
*/
static fdt_node_t* parseNode(uint32_t*     pOffset,
                             const uint8_t kAddrCells,
                             const uint8_t kSizeCells);

/**
 * @brief Parses a property in the FDT.
 *
 * @details Parses a property in the FDT based on the current offset. The
 * function populates the property and returns it.
 *
 * @param[in, out] pOffset The current offset in the FDT. The offset is updated
 * by this function.
 * @param[in] pNode The node from which we should parse the property from.
 *
 * @returns A new property parsed from the FDT is returned.
*/
static fdt_property_t* parseProperty(uint32_t*   pOffset,
                                     fdt_node_t* pNode);

/**
 * @brief Applies an action on a node for a specific property.
 *
 * @details Applies an action on a node for a specific property. Some properties
 * such as address-cells and size-cells have specific effect on parsing. This
 * function applies the action based on the property given as parameter.
 *
 * @param[in, out] pNode The node where the property was found. Might be
 * modified by the specific property action.
 * @param[in] pProperty The property to apply.
*/
static void applyPropertyAction(fdt_node_t* pNode, fdt_property_t* pProperty);

/**
 * @brief Applies an action on a node for a specific property.
 *
 * @details Applies an action on a node for a specific property. Some properties
 * such as address-cells and size-cells have specific effect on parsing. This
 * function applies the action based on the property given as parameter.
 *
 * @param[in, out] pNode The node where the property was found. Might be
 * modified by the specific property action.
 * @param[in] pProperty The property to apply.
*/
static void applyActionAddressCells(fdt_node_t*     pNode,
                                    fdt_property_t* pProperty);

/**
 * @brief Applies an action on a node for a specific property.
 *
 * @details Applies an action on a node for a specific property. Some properties
 * such as address-cells and size-cells have specific effect on parsing. This
 * function applies the action based on the property given as parameter.
 *
 * @param[in, out] pNode The node where the property was found. Might be
 * modified by the specific property action.
 * @param[in] pProperty The property to apply.
*/
static void applyActionSizeCells(fdt_node_t* pNode, fdt_property_t* pProperty);

/**
 * @brief Applies an action on a node for a specific property.
 *
 * @details Applies an action on a node for a specific property. Some properties
 * such as address-cells and size-cells have specific effect on parsing. This
 * function applies the action based on the property given as parameter.
 *
 * @param[in, out] pNode The node where the property was found. Might be
 * modified by the specific property action.
 * @param[in] pProperty The property to apply.
*/
static void applyActionPhandle(fdt_node_t*     pNode,
                               fdt_property_t* pProperty);

/**
 * @brief Reads a property from the internal FDT.
 *
 * @details Reads a property from the internal FDT. The function reads the
 * property and gets its size. The size is updated in pReadSize.
 *
 * @param[in] kpProperty The property to read.
 * @param[out] pReadSize The buffer that receives the property size.
 *
 * @returns The pointer to the property cells is returned.
*/
static inline const void* fdtInternalReadProperty(const fdt_property_t* kpProperty,
                                                  size_t*               pReadSize);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief The current contructed FDT desriptor. */
static fdt_descriptor_t sFdtDesc;

/** @brief Talbe that contains the specific properties actions. */
static specprop_table_t sSpecPropTable = {
    .numberOfProps = 3,
    .pSpecProps = {
        {"phandle",         7, applyActionPhandle},
        {"#address-cells", 14, applyActionAddressCells},
        {"#size-cells",    11, applyActionSizeCells},
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
    pNode->pNextNode     = pLinkNode;
}

static inline void linkProperty(fdt_property_t* pProp,
                                fdt_property_t* pLinkProp)
{
    while(pProp->pNextProp != NULL)
    {
        pProp = pProp->pNextProp;
    }
    pLinkProp->pNextProp = NULL;
    pProp->pNextProp     = pLinkProp;
}

static inline const void* fdtInternalReadProperty(const fdt_property_t* kpProperty,
                                                  size_t*               pReadSize)
{
    if(kpProperty == NULL)
    {
        if(pReadSize != NULL)
        {
            *pReadSize = 0;
        }
        return NULL;
    }

    if(pReadSize != NULL)
    {
        *pReadSize = kpProperty->length;
    }

    return kpProperty->pCells;
}

static void applyActionPhandle(fdt_node_t*     pNode,
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
    pNewHandle->pNext = sFdtDesc.pHandleList;
    sFdtDesc.pHandleList = pNewHandle;

    KERNEL_DEBUG(DTB_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Added new handle for %s: %d",
                 pNode->pName,
                 pNewHandle->id);
}

static void applyActionAddressCells(fdt_node_t*     pNode,
                                    fdt_property_t* pProperty)
{
    size_t          propertySize;
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

static void applyActionSizeCells(fdt_node_t*     pNode,
                                 fdt_property_t* pProperty)
{
    size_t          propertySize;
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
    for(i = 0; i < sSpecPropTable.numberOfProps; ++i)
    {
        if(propNameLength == sSpecPropTable.pSpecProps[i].nameSize &&
           sSpecPropTable.pSpecProps[i].pAction != NULL &&
           strcmp(pProperty->pName, sSpecPropTable.pSpecProps[i].pName) == 0)
        {
            sSpecPropTable.pSpecProps[i].pAction(pNode, pProperty);
        }
    }
}

static fdt_property_t* parseProperty(uint32_t*   pOffset,
                                     fdt_node_t* pNode)
{
    fdt_property_t* pProperty;
    const char*     pName;
    size_t          length;
    const uint32_t* pInitProperty;

    /* Check if start property */
    if(FDTTOCPU32(sFdtDesc.pStructs[*pOffset]) != FDT_PROP)
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


    pInitProperty = (uint32_t*)&sFdtDesc.pStructs[*pOffset];
    *pOffset += 2;

    /* Get the length and name */
    pProperty->length = FDTTOCPU32(pInitProperty[0]);
    pName = sFdtDesc.pStrings + FDTTOCPU32(pInitProperty[1]);
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
        memcpy(pProperty->pCells, sFdtDesc.pStructs + *pOffset, pProperty->length);
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

static fdt_node_t* parseNode(uint32_t*     pOffset,
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
    if(FDTTOCPU32(sFdtDesc.pStructs[*pOffset]) != FDT_BEGIN_NODE)
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
    pInitName = (char*)&sFdtDesc.pStructs[*pOffset];
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
    while(sFdtDesc.nbStructs > *pOffset)
    {
        cursor = FDTTOCPU32(sFdtDesc.pStructs[*pOffset]);
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

    memset(&sFdtDesc, 0, sizeof(fdt_descriptor_t));

    /* Get the structs and strings addresses */
    sFdtDesc.pStructs = (uint32_t*)(kStartAddr + FDTTOCPU32(pHeader->offStructs));
    sFdtDesc.pStrings = (char*)(kStartAddr + FDTTOCPU32(pHeader->offStrings));

    sFdtDesc.nbStructs = FDTTOCPU32(pHeader->sizeStructs) / sizeof(uint32_t);

    /* Now start parsing the first level */
    for(i = 0; i < sFdtDesc.nbStructs; ++i)
    {
        /* Get the node and add to root */
        pNode = parseNode(&i, INIT_ADDR_CELLS, INIT_SIZE_CELLS);
        if(pNode != NULL)
        {
            /* Link node */
            if(sFdtDesc.pFirstNode == NULL)
            {
                pNode->pNextNode = NULL;
                sFdtDesc.pFirstNode = pNode;
            }
            else
            {
                linkNode(sFdtDesc.pFirstNode, pNode);
            }
        }
    }

    TEST_POINT_FUNCTION_CALL(devtreeTest, TEST_DEVTREE_ENABLED);
}

const void* fdtGetProp(const fdt_node_t* pkFdtNode,
                       const char*       pkName,
                       size_t*           pReadSize)
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
    return sFdtDesc.pFirstNode;
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

    pHandle = sFdtDesc.pHandleList;
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

/************************************ EOF *************************************/