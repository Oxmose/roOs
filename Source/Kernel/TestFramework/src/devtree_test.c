
/*******************************************************************************
 * @file devtree_test.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 1.0
 *
 * @brief Testing framework device tree testing.
 *
 * @details Testing framework device tree testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <devtree.h>
#include <panic.h>
#include <kerneloutput.h>
#include <string.h>

/* Configuration files */
#include <config.h>

/* Header file */
#include <test_framework.h>


/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

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
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

void walkNodes(const fdt_node_t* pkNode, const uint8_t kLevel);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void walkNodes(const fdt_node_t* pkNode, const uint8_t kLevel)
{
    uint8_t i;
    const fdt_property_t* pProp;

    if(pkNode == NULL)
    {
        return;
    }

    for(i = 0; i < kLevel; ++i)
    {
        kprintf("  ");
    }
    kprintf("-> %s\n", pkNode->pName);
    pProp = fdtGetFirstProp(pkNode);
    while(pProp != NULL)
    {
        for(i = 0; i < kLevel; ++i)
        {
            kprintf("  ");
        }

        kprintf("   | %s\n", pProp->pName);
        pProp = fdtGetNextProp(pProp);
    }
    walkNodes(fdtGetChild(pkNode), kLevel + 1);
    walkNodes(fdtGetNextNode(pkNode), kLevel);
}

void devtreeTest(void)
{
    const fdt_node_t*     pkNode;
    const fdt_property_t* pkProp;
    const void*           pProp;
    size_t                propLen;

    /* TEST CORRECT PARSING */
    pkNode = fdtGetRoot();
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_PARSE,
                              pkNode != NULL,
                              0xDEADC0DE,
                              (uintptr_t)pkNode,
                              TEST_DEVTREE_ENABLED);

    /* TEST FOR WALKING */
    walkNodes(pkNode, 0);

    kprintf("------------ END OF FDT ------------\n");

    /* TEST TO GET ROOT COMPATIBLE */
    pkNode = fdtGetRoot();
    pProp = fdtGetProp(pkNode, "compatible", &propLen);
    TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETPROP0,
                           propLen - 1 == (size_t)strlen("utk,utk-fdt-v1"),
                           (size_t)strlen("utk,utk-fdt-v1"),
                           propLen - 1,
                           TEST_DEVTREE_ENABLED);
    TEST_POINT_ASSERT_BYTE(TEST_DEVTREE_GETPROP1,
                           strcmp("utk,utk-fdt-v1", pProp) == 0,
                           0,
                           strcmp("utk,utk-fdt-v1", pProp) == 0,
                           TEST_DEVTREE_ENABLED);


    /* TEST FIRST PROP */
    pkProp = fdtGetFirstProp(pkNode);
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETFIRSTPROP0,
                              pkProp != NULL,
                              0xDEADC0DE,
                              (uintptr_t)pkProp,
                              TEST_DEVTREE_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETFIRSTPROP1,
                           strcmp(pkProp->pName, "compatible") == 0,
                           0,
                           strcmp(pkProp->pName, "compatible"),
                           TEST_DEVTREE_ENABLED);

    /* TEST NEXT PROP */
    pkProp = fdtGetNextProp(pkProp);
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTPROP0,
                              pkProp != NULL,
                              0xDEADC0DE,
                              (uintptr_t)pkProp,
                              TEST_DEVTREE_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETNEXTPROP1,
                           strcmp(pkProp->pName, "#address-cells") == 0,
                           0,
                           strcmp(pkProp->pName, "#address-cells"),
                           TEST_DEVTREE_ENABLED);

    pkProp = fdtGetNextProp(pkProp);
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTPROP2,
                              pkProp != NULL,
                              0xDEADC0DE,
                              (uintptr_t)pkProp,
                              TEST_DEVTREE_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETNEXTPROP3,
                           strcmp(pkProp->pName, "#size-cells") == 0,
                           0,
                           strcmp(pkProp->pName, "#size-cells"),
                           TEST_DEVTREE_ENABLED);

    pkProp = fdtGetNextProp(pkProp);
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTPROP4,
                              pkProp == NULL,
                              (uintptr_t)NULL,
                              (uintptr_t)pkProp,
                              TEST_DEVTREE_ENABLED);

    /* TEST FIRST CHILD */
    pkNode = fdtGetChild(pkNode);
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETCHILD0,
                              pkNode != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)pkNode,
                              TEST_DEVTREE_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETCHILD1,
                           strcmp(pkNode->pName, "cpus") == 0,
                           0,
                           strcmp(pkNode->pName, "cpus"),
                           TEST_DEVTREE_ENABLED);

    pkNode = fdtGetChild(pkNode);
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETCHILD2,
                              pkNode != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)pkNode,
                              TEST_DEVTREE_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETCHILD3,
                           strcmp(pkNode->pName, "cpu@0") == 0,
                           0,
                           strcmp(pkNode->pName, "cpu@0"),
                           TEST_DEVTREE_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETCHILD4,
                              fdtGetChild(pkNode) == NULL,
                              (uintptr_t)NULL,
                              (uintptr_t)fdtGetChild(pkNode),
                              TEST_DEVTREE_ENABLED);

    /* TEST NEXT NODE */
    pkNode = fdtGetNextNode(pkNode);
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTNODE0,
                              pkNode != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)pkNode,
                              TEST_DEVTREE_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETNEXTNODE1,
                           strcmp(pkNode->pName, "cpu@1") == 0,
                           0,
                           strcmp(pkNode->pName, "cpu@1"),
                           TEST_DEVTREE_ENABLED);
    pkNode = fdtGetNextNode(pkNode);
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTNODE2,
                              pkNode != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)pkNode,
                              TEST_DEVTREE_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETNEXTNODE3,
                           strcmp(pkNode->pName, "cpu@2") == 0,
                           0,
                           strcmp(pkNode->pName, "cpu@2"),
                           TEST_DEVTREE_ENABLED);
    pkNode = fdtGetNextNode(pkNode);
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTNODE4,
                              pkNode != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)pkNode,
                              TEST_DEVTREE_ENABLED);
    TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETNEXTNODE5,
                           strcmp(pkNode->pName, "cpu@3") == 0,
                           0,
                           strcmp(pkNode->pName, "cpu@3"),
                           TEST_DEVTREE_ENABLED);
    pkNode = fdtGetNextNode(pkNode);
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNEXTNODE6,
                              pkNode == NULL,
                              (uintptr_t)NULL,
                              (uintptr_t)pkNode,
                              TEST_DEVTREE_ENABLED);
#if 0
    /* TEST GET BY NAME */
    pkNode = fdtGetNodeFromName("NONODE");
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNODEBYNAME0,
                              pkNode == NULL,
                              (uintptr_t)NULL,
                              (uintptr_t)pkNode,
                              TEST_DEVTREE_ENABLED);

    pkNode = fdtGetNodeFromName("intctrl");
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETNODEBYNAME1,
                              pkNode != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)pkNode,
                              TEST_DEVTREE_ENABLED);
    /* TEST GET BY HANDLE */
    pProp = fdtGetProp(pkNode, "phandle", &propLen);
    TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETHANDLE0,
                           propLen == sizeof(uint32_t),
                           sizeof(uint32_t),
                           propLen,
                           TEST_DEVTREE_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_DEVTREE_GETHANDLE1,
                              pProp != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)pProp,
                              TEST_DEVTREE_ENABLED);
#endif
    pkNode = fdtGetNodeByHandle(3);
    TEST_POINT_ASSERT_UINT(TEST_DEVTREE_GETHANDLE2,
                           strcmp(pkNode->pName, "interrupt-controller") == 0,
                           0,
                           strcmp(pkNode->pName, "interrupt-controller"),
                           TEST_DEVTREE_ENABLED);
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/