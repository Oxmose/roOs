
/*******************************************************************************
 * @file memgr_test.c
 *
 * @see test_framework.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 1.0
 *
 * @brief Testing framework memory manager testing.
 *
 * @details Testing framework memory manager testing.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <memory.h>
#include <panic.h>
#include <kerneloutput.h>
#include <string.h>
#include <kqueue.h>
#include <critical.h>

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
    /* TODO: Reenable */
#if 0
/************************* Imported global variables **************************/
extern kqueue_t* spPhysMemList;

extern kernel_spinlock_t sPhyMemListLock;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

extern inline void _checkMemoryType(const uintptr_t kPhysicalAddress,
                                    const uintptr_t kSize,
                                    bool_t*         pIsHardware,
                                    bool_t*         pIsMemory);
extern void _addBlock(kqueue_t*    pList,
                      uintptr_t    baseAddress,
                      const size_t length);
extern void _removeBlock(mem_list_t*  pList,
                         uintptr_t    baseAddress,
                         const size_t kLength);
extern uintptr_t _getBlock(mem_list_t* pList, const size_t kLength);
extern bool_t _memoryMgrIsMapped(const uintptr_t kVirtualAddress,
                                 size_t          pageCount);
extern OS_RETURN_E _memoryMgrMap(const uintptr_t kVirtualAddress,
                                 const uintptr_t kPhysicalAddress,
                                 const size_t    kPageCount,
                                 const uint32_t  kFlags);
extern OS_RETURN_E _memoryMgrUnmap(const uintptr_t kVirtualAddress,
                                   const size_t    kPageCount);
extern uintptr_t memoryMgrGetPhysAddr(const uintptr_t kVirtualAddress);
void* memoryKernelMap(const void*    kPhysicalAddress,
                      const size_t   kSize,
                      const uint32_t kFlags,
                      OS_RETURN_E*   pError);
OS_RETURN_E memoryKernelUnmap(const void* kVirtualAddress, const size_t kSize);
void* memoryKernelMapStack(const size_t kSize);
void memoryKernelUnmapStack(const uintptr_t kBaseAddress, const size_t kSize);
static inline uintptr_t _makeCanonical(const uintptr_t kAddress,
                                       const bool_t    kIsPhysical);
#endif
/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void memmgrTest(void)
{
    /* TODO: Reenable */
#if 0
    mem_range_t* pRange;
    kqueue_node_t* pNode;
    uint32_t addBlockSeq;
    int i;

    /* Check that init is correct */
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_INIT_0,
                              spPhysMemList != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)NULL,
                              TEST_MEMMGR_ENABLED);

    /* Check that first node corresponds */
    pNode = spPhysMemList->pHead;
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_INIT_1,
                              pNode != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)NULL,
                              TEST_MEMMGR_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_INIT_2,
                              pNode->pNext == NULL,
                              (uintptr_t)NULL,
                              (uintptr_t)pNode->pNext,
                              TEST_MEMMGR_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_INIT_3,
                              pNode->pData != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)NULL,
                              TEST_MEMMGR_ENABLED);
    pRange = (mem_range_t*)pNode->pData;
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_INIT_4,
                              pRange->base == 0x100000,
                              (uintptr_t)0x100000,
                              (uintptr_t)pRange->base,
                              TEST_MEMMGR_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_INIT_5,
                              pRange->limit == 0x10000000,
                              (uintptr_t)0x10000000,
                              (uintptr_t)pRange->limit,
                              TEST_MEMMGR_ENABLED);

    /* Add tests */

    /* Add a new blocks, not contiguous */
    uintptr_t baseAddr[6];
    baseAddr[0] = 0x20000000;
    baseAddr[1] = 0x21000000;
    baseAddr[2] = 0x22000000;
    baseAddr[3] = 0x23000000;
    baseAddr[4] = 0x24000000;
    baseAddr[5] = 0x25000000;
    _addBlock(spPhysMemList, baseAddr[1], 0x4000);
    _addBlock(spPhysMemList, baseAddr[5], 0x4000);
    _addBlock(spPhysMemList, baseAddr[0], 0x4000);
    _addBlock(spPhysMemList, baseAddr[2], 0x4000);
    _addBlock(spPhysMemList, baseAddr[3], 0x4000);
    _addBlock(spPhysMemList, baseAddr[4], 0x4000);

    addBlockSeq = 0;
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              spPhysMemList != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)NULL,
                              TEST_MEMMGR_ENABLED);

    pNode = spPhysMemList->pHead;
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              pNode != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)NULL,
                              TEST_MEMMGR_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              pNode != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)NULL,
                              TEST_MEMMGR_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              pNode->pNext != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)NULL,
                              TEST_MEMMGR_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              pNode->pData != NULL,
                              (uintptr_t)0xDEADC0DE,
                              (uintptr_t)NULL,
                              TEST_MEMMGR_ENABLED);
    pRange = (mem_range_t*)pNode->pData;
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              pRange->base == 0x100000,
                              (uintptr_t)0x100000,
                              (uintptr_t)pRange->base,
                              TEST_MEMMGR_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              pRange->limit == 0x10000000,
                              (uintptr_t)0x10000000,
                              (uintptr_t)pRange->limit,
                              TEST_MEMMGR_ENABLED);
    for(i = 0; i < 6; ++i)
    {
        pNode = pNode->pNext;
        pRange = (mem_range_t*)pNode->pData;
        TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                                pRange->base == baseAddr[i],
                                (uintptr_t)baseAddr[i],
                                (uintptr_t)pRange->base,
                                TEST_MEMMGR_ENABLED);
        TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                                pRange->limit == baseAddr[i] + 0x4000,
                                (uintptr_t)baseAddr[i] + 0x4000,
                                (uintptr_t)pRange->limit,
                                TEST_MEMMGR_ENABLED);
    }
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              pNode->pNext == NULL,
                              (uintptr_t)NULL,
                              (uintptr_t)pNode->pNext,
                              TEST_MEMMGR_ENABLED);

    /* Add a new blocks, extend down */
    _addBlock(spPhysMemList, 0x1FFFF000, 0x1000);
    pNode = spPhysMemList->pHead;
    pNode = pNode->pNext;
    pRange = (mem_range_t*)pNode->pData;
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              pRange->base == 0x1FFFF000,
                              (uintptr_t)0x1FFFF000,
                              (uintptr_t)pRange->base,
                              TEST_MEMMGR_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                            pRange->limit == 0x1FFFF000 + 0x5000,
                            (uintptr_t)0x1FFFF000 + 0x5000,
                            (uintptr_t)pRange->limit,
                            TEST_MEMMGR_ENABLED);
    i = 0;
    pNode = spPhysMemList->pHead;
    while(pNode != NULL)
    {
        pNode = pNode->pNext;
        ++i;
    }
    TEST_POINT_ASSERT_UINT(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                            i == 7,
                            (uintptr_t)7,
                            (uintptr_t)i,
                            TEST_MEMMGR_ENABLED);


    /* Add a new blocks, extend up */
    _addBlock(spPhysMemList, 0x20004000, 0x1000);
    pNode = spPhysMemList->pHead;
    pNode = pNode->pNext;
    pRange = (mem_range_t*)pNode->pData;
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              pRange->base == 0x1FFFF000,
                              (uintptr_t)0x1FFFF000,
                              (uintptr_t)pRange->base,
                              TEST_MEMMGR_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                            pRange->limit == 0x1FFFF000 + 0x6000,
                            (uintptr_t)0x1FFFF000 + 0x6000,
                            (uintptr_t)pRange->limit,
                            TEST_MEMMGR_ENABLED);
    i = 0;
    pNode = spPhysMemList->pHead;
    while(pNode != NULL)
    {
        pNode = pNode->pNext;
        ++i;
    }
    TEST_POINT_ASSERT_UINT(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                            i == 7,
                            (uintptr_t)7,
                            (uintptr_t)i,
                            TEST_MEMMGR_ENABLED);

    /* Add a new blocks, overwrite */
    _addBlock(spPhysMemList, 0x1FFFE000, 0x10000);
    pNode = spPhysMemList->pHead;
    pNode = pNode->pNext;
    pRange = (mem_range_t*)pNode->pData;
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              pRange->base == 0x1FFFE000,
                              (uintptr_t)0x1FFFE000,
                              (uintptr_t)pRange->base,
                              TEST_MEMMGR_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                            pRange->limit == 0x1FFFE000 + 0x10000,
                            (uintptr_t)0x1FFFE000 + 0x10000,
                            (uintptr_t)pRange->limit,
                            TEST_MEMMGR_ENABLED);
    i = 0;
    pNode = spPhysMemList->pHead;
    while(pNode != NULL)
    {
        pNode = pNode->pNext;
        ++i;
    }
    TEST_POINT_ASSERT_UINT(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                            i == 7,
                            (uintptr_t)7,
                            (uintptr_t)i,
                            TEST_MEMMGR_ENABLED);

    /* Add a new blocks, extend up, merge */
    _addBlock(spPhysMemList, 0x2000A000, 0xFF6000);
    pNode = spPhysMemList->pHead;
    pNode = pNode->pNext;
    pRange = (mem_range_t*)pNode->pData;
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              pRange->base == 0x1FFFE000,
                              (uintptr_t)0x1FFFE000,
                              (uintptr_t)pRange->base,
                              TEST_MEMMGR_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                            pRange->limit == 0x1FFFE000 + 0x1006000,
                            (uintptr_t)0x1FFFF000 + 0x1006000,
                            (uintptr_t)pRange->limit,
                            TEST_MEMMGR_ENABLED);
    i = 0;
    pNode = spPhysMemList->pHead;
    while(pNode != NULL)
    {
        pNode = pNode->pNext;
        ++i;
    }
    TEST_POINT_ASSERT_UINT(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                            i == 6,
                            (uintptr_t)6,
                            (uintptr_t)i,
                            TEST_MEMMGR_ENABLED);


    /* Add a new blocks, overwrite merge */
    _addBlock(spPhysMemList, 0x1FFFD000, 0x3003000);
    pNode = spPhysMemList->pHead;
    pNode = pNode->pNext;
    pRange = (mem_range_t*)pNode->pData;
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              pRange->base == 0x1FFFD000,
                              (uintptr_t)0x1FFFD000,
                              (uintptr_t)pRange->base,
                              TEST_MEMMGR_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                            pRange->limit == 0x1FFFD000 + 0x3007000,
                            (uintptr_t)0x1FFFD000 + 0x3007000,
                            (uintptr_t)pRange->limit,
                            TEST_MEMMGR_ENABLED);
    i = 0;
    pNode = spPhysMemList->pHead;
    while(pNode != NULL)
    {
        pNode = pNode->pNext;
        ++i;
    }
    TEST_POINT_ASSERT_UINT(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                            i == 4,
                            (uintptr_t)4,
                            (uintptr_t)i,
                            TEST_MEMMGR_ENABLED);

    /* Add a new blocks, overwrite merge twice */
    _addBlock(spPhysMemList, 0x1FFFC000, 0x5009000);
    pNode = spPhysMemList->pHead;
    pNode = pNode->pNext;
    pRange = (mem_range_t*)pNode->pData;
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                              pRange->base == 0x1FFFC000,
                              (uintptr_t)0x1FFFC000,
                              (uintptr_t)pRange->base,
                              TEST_MEMMGR_ENABLED);
    TEST_POINT_ASSERT_POINTER(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                            pRange->limit == 0x1FFFC000 + 0x5009000,
                            (uintptr_t)0x1FFFC000 + 0x5009000,
                            (uintptr_t)pRange->limit,
                            TEST_MEMMGR_ENABLED);
    i = 0;
    pNode = spPhysMemList->pHead;
    while(pNode != NULL)
    {
        pNode = pNode->pNext;
        ++i;
    }
    TEST_POINT_ASSERT_UINT(TEST_MEMMGR_ADDBLOCK(addBlockSeq++),
                            i == 2,
                            (uintptr_t)2,
                            (uintptr_t)i,
                            TEST_MEMMGR_ENABLED);

    /*TODO: Test remove block */

#else
    TEST_POINT_ASSERT_UINT(TEST_MEMMGR_ADDBLOCK(0),
                           0 == 0,
                           0,
                           0,
                           TEST_MEMMGR_ENABLED);

    TEST_FRAMEWORK_END();
#endif
}

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

/************************************ EOF *************************************/