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
#include <stdint.h>       /* Standard int types and bool_t */
#include <kerror.h>       /* Kernel error codes */
#include <string.h>       /* Memory manipulation */
#include <kerneloutput.h> /* Kernel logging */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <devtree.h>

/* Tracing feature */
#include <tracing.h>

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

/** @brief Initial address cells value */
#define INIT_ADDR_CELLS 2
/** @brief Initial size cells value */
#define INIT_SIZE_CELLS 1

/** @brief FDT cells size  */
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
    /** @brief Reserved memory pointer  */
    uintptr_t* pResMemory;

    /** @brief First root node of the FDT */
    fdt_node_t* pFirstNode;

    /** @brief FDT pHandle list */
    phandle_t* pHandleList;

    /** @brief FDT available memory regions list */
    fdt_mem_node_t* pFirstMemoryNode;

    /** @brief FDT reserved memory regions list */
    fdt_mem_node_t* pFirstReservedMemoryNode;
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
 * @param[in, out] pLinkNode The next node to link.
*/
static inline void _linkNode(fdt_node_t* pNode, fdt_node_t* pLinkNode);

/**
 * @brief Links a property to another in the FDT tree.
 *
 * @details Links a property to another in the FDT tree. This is used to reverse
 * the FDT read order (First Read Last Seen).
 *
 * @param[in, out] pProp The property to link.
 * @param[in, out] pLinkProp The next property to link.
*/
static inline void _linkProperty(fdt_property_t* pProp,
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
 * @param[in] kIsResMemSubNode Tells if the current node is a reserved-memory
 * sub-node.
 *
 * @return A new node parsed from the FDT is returned.
*/
static fdt_node_t* _parseNode(uint32_t*     pOffset,
                              const uint8_t kAddrCells,
                              const uint8_t kSizeCells,
                              const bool_t  kIsResMemSubNode);

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
 * @return A new property parsed from the FDT is returned.
*/
static fdt_property_t* _parseProperty(uint32_t* pOffset, fdt_node_t* pNode);

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
static void _applyPropertyAction(fdt_node_t* pNode, fdt_property_t* pProperty);

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
static void _applyActionAddressCells(fdt_node_t*     pNode,
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
static void _applyActionSizeCells(fdt_node_t* pNode, fdt_property_t* pProperty);

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
static void _applyActionPhandle(fdt_node_t*     pNode,
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
 * @return The pointer to the property cells is returned.
*/
static inline const void* _fdtInternalReadProp(const fdt_property_t* kpProperty,
                                               size_t*               pReadSize);

/**
 * @brief Parses the reserved memory section of the FDT.
 *
 * @details Parses the reserved memory section of the FDT. A list of reserved
 * memory region will be created in teh FDT descriptor.
 */
static void _parseReservedMemory(void);

/**
 * @brief Walks the FDT nodes starting from the root node and returns the
 * node with the requested name.
 *
 * @details Walks the FDT nodes starting from the root node and returns the
 * node with the requested name. The walk is
 * performed as a depth first search walk. For each node, the compatible is
 * compared to the list of registered drivers. If a driver is compatible, its
 * attach function is called. NULL is returned is the node is not found.
 *
 * @param[in] kpRootNode The root node to use.
 * @param[in] kpName The name of the node to find.
 *
 * @return The node with the requested name is returned or NULL if not found.
 */
static const fdt_node_t* _findFdtNode(const fdt_node_t* kpRootNode,
                                      const char*       kpName);

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
        {"phandle",         7, _applyActionPhandle},
        {"#address-cells", 14, _applyActionAddressCells},
        {"#size-cells",    11, _applyActionSizeCells},
    }
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static inline void _linkNode(fdt_node_t* pNode, fdt_node_t* pLinkNode)
{
    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED, TRACE_DEVTREE_LINK_NODE_ENTRY, 0);

    while(pNode->pNextNode != NULL)
    {
        pNode = pNode->pNextNode;
    }
    pLinkNode->pNextNode = NULL;
    pNode->pNextNode     = pLinkNode;

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED, TRACE_DEVTREE_LINK_NODE_EXIT, 0);
}

static inline void _linkProperty(fdt_property_t* pProp,
                                 fdt_property_t* pLinkProp)
{
    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED, TRACE_DEVTREE_LINK_PROP_ENTRY, 0);

    while(pProp->pNextProp != NULL)
    {
        pProp = pProp->pNextProp;
    }
    pLinkProp->pNextProp = NULL;
    pProp->pNextProp     = pLinkProp;

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED, TRACE_DEVTREE_LINK_PROP_EXIT, 0);
}

static inline const void* _fdtInternalReadProp(const fdt_property_t* kpProperty,
                                               size_t*               pReadSize)
{
    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED, TRACE_DEVTREE_READ_PROP_ENTRY, 0);

    if(kpProperty == NULL)
    {
        if(pReadSize != NULL)
        {
            *pReadSize = 0;
        }
        KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                           TRACE_DEVTREE_READ_PROP_EXIT,
                           0);
        return NULL;
    }

    if(pReadSize != NULL)
    {
        *pReadSize = kpProperty->length;
    }

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED, TRACE_DEVTREE_READ_PROP_EXIT, 0);

    return kpProperty->pCells;
}

static void _applyActionPhandle(fdt_node_t*     pNode,
                                fdt_property_t* pProperty)
{
    phandle_t* pNewHandle;

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_ACTION_PHANDLE_ENTRY,
                       0);

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

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_ACTION_PHANDLE_EXIT,
                       0);
}

static void _applyActionAddressCells(fdt_node_t*     pNode,
                                     fdt_property_t* pProperty)
{
    size_t          propertySize;
    const uint32_t* pPropertyPtr;

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_ACTION_ADDRCELLS_ENTRY,
                       0);

    propertySize = sizeof(uint32_t);
    pPropertyPtr = _fdtInternalReadProp(pProperty, &propertySize);
    if(propertySize == sizeof(uint32_t))
    {
        pNode->addrCells = FDTTOCPU32(*pPropertyPtr);
    }
    else
    {
        KERNEL_ERROR("Incorrect read size in address-cells property\n");
    }
    KERNEL_DEBUG(DTB_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Address size is now %d",
                 pNode->addrCells);

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_ACTION_ADDRCELLS_EXIT,
                       0);
}

static void _applyActionSizeCells(fdt_node_t*     pNode,
                                  fdt_property_t* pProperty)
{
    size_t          propertySize;
    const uint32_t* pPropertyPtr;

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_ACTION_SIZECELLS_ENTRY,
                       0);

    propertySize = sizeof(uint32_t);
    pPropertyPtr = _fdtInternalReadProp(pProperty, &propertySize);
    if(propertySize == sizeof(uint32_t))
    {
        pNode->sizeCells = FDTTOCPU32(*pPropertyPtr);
    }
    else
    {
        KERNEL_ERROR("Incorrect read size in size-cells property\n");
    }

    KERNEL_DEBUG(DTB_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Size size is now %d",
                 pNode->sizeCells);

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_ACTION_SIZECELLS_EXIT,
                       0);
}

static void _applyPropertyAction(fdt_node_t* pNode, fdt_property_t* pProperty)
{
    size_t i;
    size_t propNameLength;

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_APPLY_PROP_ACTION_ENTRY,
                       0);

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

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_APPLY_PROP_ACTION_EXIT,
                       0);
}

static fdt_property_t* _parseProperty(uint32_t*   pOffset,
                                      fdt_node_t* pNode)
{
    fdt_property_t* pProperty;
    const char*     pName;
    size_t          length;
    const uint32_t* pInitProperty;

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_PARSE_PROP_ENTRY,
                       0);

    /* Check if start property */
    if(FDTTOCPU32(sFdtDesc.pStructs[*pOffset]) != FDT_PROP)
    {
        KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                           TRACE_DEVTREE_PARSE_PROP_EXIT,
                           0);
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
        memcpy(pProperty->pCells,
               sFdtDesc.pStructs + *pOffset,
               pProperty->length);
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

    _applyPropertyAction(pNode, pProperty);

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_PARSE_PROP_EXIT,
                       0);

    return pProperty;
}

static fdt_node_t* _parseNode(uint32_t*     pOffset,
                              const uint8_t kAddrCells,
                              const uint8_t kSizeCells,
                              const bool_t  kIsResMemSubNode)
{
    fdt_node_t*     pNode;
    fdt_node_t*     pChildNode;
    fdt_mem_node_t* pMemNode;
    fdt_mem_node_t* pMemNodeCursor;
    fdt_property_t* pProperty;
    const char*     pInitName;
    size_t          length;
    uint32_t        cursor;
    size_t          i;
    bool_t          isResMem;

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_PARSE_NODE_ENTRY,
                       0);

    /* Check if start node */
    if(FDTTOCPU32(sFdtDesc.pStructs[*pOffset]) != FDT_BEGIN_NODE)
    {
        KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                           TRACE_DEVTREE_PARSE_NODE_EXIT,
                           0);
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

    isResMem = (strcmp(pNode->pName, "reserved-memory") == 0);

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
            pChildNode = _parseNode(pOffset,
                                    pNode->addrCells,
                                    pNode->sizeCells,
                                    isResMem);
            if(pChildNode != NULL)
            {
                if(pNode->pFirstChildNode == NULL)
                {
                    pChildNode->pNextNode = NULL;
                    pNode->pFirstChildNode = pChildNode;
                }
                else
                {
                    _linkNode(pNode->pFirstChildNode, pChildNode);
                }
            }
        }
        else if(cursor == FDT_PROP)
        {
            pProperty = _parseProperty(pOffset, pNode);
            if(pProperty != NULL)
            {
                if(pNode->pProps == NULL)
                {
                    pProperty->pNextProp = NULL;
                    pNode->pProps = pProperty;
                }
                else
                {
                    _linkProperty(pNode->pProps, pProperty);
                }

                /* If node is memory */
                if(strncmp(pNode->pName, "memory@", 7) == 0 &&
                   strcmp(pProperty->pName, "reg") == 0)
                {
                    for(i = 0;
                        i < pProperty->length / sizeof(uintptr_t);
                        i += 2)
                    {
                        pMemNode = kmalloc(sizeof(fdt_mem_node_t));
                        if(pMemNode == NULL)
                        {
                            PANIC(OS_ERR_NO_MORE_MEMORY,
                                MODULE_NAME,
                                "Failed to allocate new memory node",
                                TRUE);
                        }
                        pMemNode->baseAddress =
                            *(((uintptr_t*)pProperty->pCells) + i);
                        pMemNode->size =
                            *(((uintptr_t*)pProperty->pCells) + 1 + i);

                        pMemNode->pNextNode = NULL;

                        KERNEL_DEBUG(DTB_DEBUG_ENABLED,
                                    MODULE_NAME,
                                    "Adding memory region at 0x%p of size 0x%x",
                                    pMemNode->baseAddress,
                                    pMemNode->size);
                        if(sFdtDesc.pFirstMemoryNode != NULL)
                        {
                            pMemNodeCursor = sFdtDesc.pFirstMemoryNode;
                            while(pMemNodeCursor->pNextNode != NULL)
                            {
                                pMemNodeCursor = pMemNodeCursor->pNextNode;
                            }
                            pMemNodeCursor->pNextNode = pMemNode;
                        }
                        else
                        {
                            sFdtDesc.pFirstMemoryNode = pMemNode;
                        }
                    }
                }
                /* If node is subnode of reserved-memory */
                else if(kIsResMemSubNode)
                {
                    if(strcmp(pProperty->pName, "reg") == 0)
                    {
                        pMemNode = kmalloc(sizeof(fdt_mem_node_t));
                        if(pMemNode == NULL)
                        {
                            PANIC(OS_ERR_NO_MORE_MEMORY,
                                MODULE_NAME,
                                "Failed to allocate new memory node",
                                TRUE);
                        }
                        pMemNode->baseAddress =
                            *((uintptr_t*)pProperty->pCells);
                        pMemNode->size =
                            *(((uintptr_t*)pProperty->pCells) + 1);

                        pMemNode->pNextNode = NULL;

                        KERNEL_DEBUG(DTB_DEBUG_ENABLED,
                                    MODULE_NAME,
                                    "Adding reserved memory region at 0x%p of "
                                    "size 0x%x",
                                    pMemNode->baseAddress,
                                    pMemNode->size);
                        if(sFdtDesc.pFirstReservedMemoryNode != NULL)
                        {
                            pMemNodeCursor = sFdtDesc.pFirstReservedMemoryNode;
                            while(pMemNodeCursor->pNextNode != NULL)
                            {
                                pMemNodeCursor = pMemNodeCursor->pNextNode;
                            }
                            pMemNodeCursor->pNextNode = pMemNode;
                        }
                        else
                        {
                            sFdtDesc.pFirstReservedMemoryNode = pMemNode;
                        }
                    }
                }
            }
        }
        else
        {
            ++(*pOffset);
            if(cursor == FDT_END_NODE)
            {
                KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                                   TRACE_DEVTREE_PARSE_NODE_EXIT,
                                   0);
                return pNode;
            }
        }
    }

    KERNEL_ERROR("Unexpected end of node\n");

    kfree(pNode->pName);
    kfree(pNode);

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_PARSE_NODE_EXIT,
                       0);

    return NULL;
}

static void _parseReservedMemory(void)
{
    uint64_t*       pCursor;
    uintptr_t       startAddr;
    uintptr_t       size;
    fdt_mem_node_t* pNode;
    fdt_mem_node_t* pNodeCursor;

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_PARSE_RESERVED_MEM_ENTRY,
                       0);

    /* Register the first node */
    pCursor = (uint64_t*)sFdtDesc.pResMemory;
    startAddr = (uintptr_t)FDTTOCPU64(pCursor[0]);
    size      = (uintptr_t)FDTTOCPU64(pCursor[1]);

    KERNEL_DEBUG(DTB_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Parsing reserved memory regions at 0x%p with 0x%p 0x%p",
                 sFdtDesc.pResMemory,
                 startAddr,
                 size);

    while(startAddr != 0 && size != 0)
    {
        KERNEL_DEBUG(DTB_DEBUG_ENABLED,
                     MODULE_NAME,
                     "Adding reserved memory region at 0x%p of size 0x%x",
                     startAddr,
                     size);
        pNode = kmalloc(sizeof(fdt_mem_node_t));
        if(pNode == NULL)
        {
            PANIC(OS_ERR_NO_MORE_MEMORY,
                  MODULE_NAME,
                  "Failed to allocate new reserved memory node",
                  TRUE);
        }

        pNode->baseAddress = startAddr;
        pNode->size        = size;

        pNode->pNextNode = NULL;

        if(sFdtDesc.pFirstReservedMemoryNode != NULL)
        {
            pNodeCursor = sFdtDesc.pFirstMemoryNode;
            while(pNodeCursor->pNextNode != NULL)
            {
                pNodeCursor = pNodeCursor->pNextNode;
            }
            pNodeCursor->pNextNode = pNode;
        }
        else
        {
            sFdtDesc.pFirstMemoryNode = pNode;
        }

        pCursor   += 2;
        startAddr = (uintptr_t)FDTTOCPU64(pCursor[0]);
        size      = (uintptr_t)FDTTOCPU64(pCursor[1]);
    }

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_PARSE_RESERVED_MEM_EXIT,
                       0);
}

static const fdt_node_t* _findFdtNode(const fdt_node_t* kpRootNode,
                                      const char*       kpName)
{
    const fdt_node_t* kpRetNode;

    if(kpRootNode == NULL)
    {
        return NULL;
    }

    /* Check if the name is found */
    if(strcmp(kpRootNode->pName, kpName) == 0)
    {
        return kpRootNode;
    }

    /* Got to next nodes */
    kpRetNode = _findFdtNode(fdtGetChild(kpRootNode), kpName);
    if(kpRetNode == NULL)
    {
        kpRetNode = _findFdtNode(fdtGetNextNode(kpRootNode), kpName);
    }

    return kpRetNode;
}

void fdtInit(const uintptr_t kStartAddr)
{
    uint32_t i;

    const fdt_header_t* pHeader;
    fdt_node_t*         pNode;

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED, TRACE_DEVTREE_INIT_ENTRY, 0);

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
    sFdtDesc.pStructs = (uint32_t*)(kStartAddr +
                                    FDTTOCPU32(pHeader->offStructs));
    sFdtDesc.pStrings = (char*)(kStartAddr + FDTTOCPU32(pHeader->offStrings));
    sFdtDesc.pResMemory = (uintptr_t*)(kStartAddr +
                                    FDTTOCPU32(pHeader->offMemRsvMap));

    sFdtDesc.nbStructs = FDTTOCPU32(pHeader->sizeStructs) / sizeof(uint32_t);

    /* Get the reseved memory regions */
    _parseReservedMemory();

    /* Now start parsing the first level */
    for(i = 0; i < sFdtDesc.nbStructs; ++i)
    {
        /* Get the node and add to root */
        pNode = _parseNode(&i, INIT_ADDR_CELLS, INIT_SIZE_CELLS, FALSE);
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
                _linkNode(sFdtDesc.pFirstNode, pNode);
            }
        }
    }

    TEST_POINT_FUNCTION_CALL(devtreeTest, TEST_DEVTREE_ENABLED);

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED, TRACE_DEVTREE_INIT_EXIT, 0);
}

const void* fdtGetProp(const fdt_node_t* pkFdtNode,
                       const char*       pkName,
                       size_t*           pReadSize)
{
    const fdt_property_t* pProp;
    void*                 retVal;

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED, TRACE_DEVTREE_GET_PROP_ENTRY, 0);

    if(pkFdtNode == NULL || pkName == NULL)
    {
        if(pReadSize != NULL)
        {
            *pReadSize = 0;
        }
        KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                           TRACE_DEVTREE_GET_PROP_EXIT,
                           0);
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

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED, TRACE_DEVTREE_GET_PROP_EXIT, 0);

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

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                       TRACE_DEVTREE_GET_HANDLE_ENTRY,
                       0);

    pHandle = sFdtDesc.pHandleList;
    while(pHandle != NULL)
    {
        if(pHandle->id == kHandleId)
        {
            KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED,
                               TRACE_DEVTREE_GET_HANDLE_EXIT,
                               0);
            return (fdt_node_t*)pHandle->pLink;
        }
        pHandle = pHandle->pNext;
    }

    KERNEL_TRACE_EVENT(TRACE_DEVTREE_ENABLED, TRACE_DEVTREE_GET_PROP_EXIT, 0);

    return NULL;
}

const fdt_mem_node_t* fdtGetMemory(void)
{
    return sFdtDesc.pFirstMemoryNode;
}

const fdt_mem_node_t* fdtGetReservedMemory(void)
{
    return sFdtDesc.pFirstReservedMemoryNode;
}

const fdt_node_t* fdtGetNodeByName(const char* kpName)
{
    return _findFdtNode(sFdtDesc.pFirstNode, kpName);
}

/************************************ EOF *************************************/