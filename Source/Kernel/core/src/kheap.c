/*******************************************************************************
 * @file kheap.c
 *
 * @see kheap.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 24/05/2023
 *
 * @version 1.0
 *
 * @brief Kernel's heap allocator.
 *
 * @details Kernel's heap allocator. Allow to dynamically allocate and dealocate
 * memory on the kernel's heap.
 *
 * @warning This allocator is not suited to allocate memory for the process, you
 * should only use it for the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <stdint.h>       /* Generic integer definitions */
#include <string.h>       /* Memory manipulation */
#include <critical.h>     /* Critical sections */
#include <kerneloutput.h> /* Kernel IO */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <kheap.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Num size. */
#define NUM_SIZES 32

/** @brief Memory chunk alignement. */
#define ALIGN 4

/** @brief Chunk minimal size. */
#define MIN_SIZE sizeof(list_t)

/** @brief Header size. */
#define HEADER_SIZE __builtin_offsetof(mem_chunk_t, pData)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Kernel's heap allocator list node. */
typedef struct list
{
    /** @brief Next node of the list. */
    struct list* pNext;
    /** @brief Previous node of the list. */
    struct list* pPrev;
} list_t;

/** @brief Kernel's heap allocator memory chunk representation. */
typedef struct
{
    /** @brief Memory chunk list. */
    list_t all;

    /** @brief Used flag. */
    bool_t used;

    /** @brief If used, the union contains the chunk's data, else a list of free
     * memory.
     */
    union
    {
        uint8_t* pData;
        list_t   free;
    };
} mem_chunk_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/** @brief Get the container */
#define CONTAINER(C, l, v) ((C*)(((char*)v) - (uintptr_t)&(((C*)0)->l)))

/** @brief Initializes the list */
#define LIST_INIT(v, l) _listInit(&v->l)

/** @brief Remove element from the list */
#define LIST_REMOVE_FROM(h, d, l)                           \
{                                                           \
    __typeof__(**h) **h_ = h, *d_ = d;                      \
    list_t* head = &(*h_)->l;                               \
    _removeFrom(&head, &d_->l);                            \
    if (head == NULL)                                       \
    {                                                       \
          *h_ = NULL;                                       \
    }                                                       \
    else                                                    \
    {                                                       \
        *h_ = CONTAINER(__typeof__(**h), l, head);          \
    }                                                       \
}

/** @brief Remove element to the list */
#define LIST_PUSH(h, v, l)                          \
{                                                   \
    __typeof__(*v) **h_ = h, *v_ = v;               \
    list_t* head = &(*h_)->l;                       \
    if (*h_ == NULL)                                \
    {                                               \
        head = NULL;                                \
    }                                               \
    _push(&head, &v_->l);                           \
    *h_ = CONTAINER(__typeof__(*v), l, head);       \
}

/** @brief Pop element from the list */
#define LIST_POP(h, l)                                  \
__extension__                                           \
({                                                      \
    __typeof__(**h) **h_ = h;                           \
    list_t* head = &(*h_)->l;                           \
    list_t* res = _pop(&head);                          \
    if (head == NULL)                                   \
    {                                                   \
           *h_ = NULL;                                  \
    }                                                   \
    else                                                \
    {                                                   \
          *h_ = CONTAINER(__typeof__(**h), l, head);    \
    }                                                   \
    CONTAINER(__typeof__(**h), l, res);                 \
})

/** @brief Get first iterator of a list */
#define LIST_ITERATOR_BEGIN(h, l, it)                                       \
{                                                                           \
    __typeof__(*h) *h_ = h;                                                 \
      list_t* last_##it = h_->l.prev, *iter_##it = &h_->l, *next_##it;      \
    do                                                                      \
    {                                                                       \
        if (iter_##it == last_##it)                                         \
        {                                                                   \
            next_##it = NULL;                                               \
        }                                                                   \
        else                                                                \
        {                                                                   \
            next_##it = iter_##it->pNext;                                    \
        }                                                                   \
        __typeof__(*h)* it = CONTAINER(__typeof__(*h), l, iter_##it);       \
/** @brief Get end iterator of a list */
#define LIST_ITERATOR_END(it)                       \
    }while((iter_##it = next_##it));                \
}

/** @brief Remove element from lsit iterator */
#define LIST_ITERATOR_REMOVE_FROM(h, it, l) LIST_REMOVE_FROM(h, iter_##it, l)

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Initializes the memory list.
 *
 * @details Initializes the memory list with the basic node value.
 *
 * @param[out] pNode The list's node to initialize.
 */
inline static void _listInit(list_t* pNode);

/**
 * @brief Inserts a node before the current node in the list.
 *
 * @details Inserts a node before the current node in the list.
 *
 * @param[in,out] pCurrent The current node.
 * @param[in,out] pNew The new node to insert before the current node.
 */
inline static void _insertBefore(list_t* pCurrent, list_t* pNew);

/**
 * @brief Inserts a node after the current node in the list.
 *
 * @param[in,out] pCurrent The current node.
 * @param[in,out] pNew The new node to insert after the current node.
 */
inline static void _insertAfter(list_t* pCurrent, list_t* pNew);

/**
 * @brief Removes a node from the list.
 *
 * @details Removes a node from the list.
 *
 * @param[out] pNode The node to remove from the list.
 */
inline static void _remove(list_t* pNode);

/**
 * @brief Pushes a node at the end of the list.
 *
 * @details Pushes a node at the end of the list.
 *
 * @param[out] ppList The list to be pushed.
 * @param[in] pNode The node to push to the list.
 */
inline static void _push(list_t** ppList, list_t* pNode);

/**
 * @brief Pops a node from the list.
 *
 * @details Pops a node from the list and returns it.
 *
 * @param[out] ppList The list to be poped from.
 *
 * @return The node poped from the list is returned.
 */
inline static list_t* _pop(list_t** ppList);

/**
 * @brief Removes a node from the list.
 *
 * @details Removes a node from the list.
 *
 * @param[out] ppList The list to remove the node from.
 * @param[out] pNode The node to remove from the list.
 */
inline static void _removeFrom(list_t** ppList, list_t* pNode);

/**
 * @brief Initializes a memory chunk structure.
 *
 * @details Initializes a memory chunk structure.
 *
 * @param[out] pChunk The chunk structure to initialize.
 */
inline static void _memoryChunkInit(mem_chunk_t* pChunk);

/**
 * @brief Returns the size of a memory chunk.
 *
 * @param pChunk The chunk to get the size of.
 *
 * @return The size of the memory chunk is returned.
 */
inline static uint32_t _memoryChunkSize(const mem_chunk_t* pChunk);

/**
 * @brief Returns the slot of a memory chunk for the desired size.
 *
 * @details Returns the slot of a memory chunk for the desired size.
 *
 * @param[in] size The size of the chunk to get the slot of.
 *
 * @return The slot of a memory chunk for the desired size.
 */
inline static int32_t _memoryChunkSlot(uint32_t size);

/**
 * @brief Removes a memory chunk in the free memory chunks list.
 *
 * @details Removes a memory chunk in the free memory chunks list.
 *
 * @param[in, out] pChunk The chunk to be removed from the list.
 */
inline static void _removeFree(mem_chunk_t* pChunk);

/**
 * @brief Pushes a memory chunk in the free memory chunks list.
 *
 * @details Pushes a memory chunk in the free memory chunks list.
 *
 * @param[in, out] pChunk The chunk to be placed in the list.
 */
inline static void _pushFree(mem_chunk_t *pChunk);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Start address of the kernel's heap. */
extern uint8_t _KERNEL_HEAP_BASE;
/** @brief End address of the kernel's heap. */
extern uint8_t _KERNEL_HEAP_SIZE;

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/

/** @brief Kernel's heap initialization state. */
static bool_t sInit = FALSE;

/* Heap data */
/** @brief Kernel's heap free memory chunks. */
static mem_chunk_t* spFreeChunk[NUM_SIZES] = { NULL };
/** @brief Kernel's heap first memory chunk. */
static mem_chunk_t* spFirstChunk;
/** @brief Kernel's heap last memory chunk. */
static mem_chunk_t* spLastChunk;

/** @brief Quantity of free memory in the kernel's heap. */
static uint32_t sMemFree;
/** @brief Quantity of initial free memory in the kernel's heap. */
static uint32_t sKheapInitFree;
/** @brief Quantity of memory used to store meta data in the kernel's heap. */
static uint32_t sMemMeta;

/** @brief Heap lock */
static kernel_spinlock_t sLock;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

inline static void _listInit(list_t* pNode)
{
    pNode->pNext = pNode;
    pNode->pPrev = pNode;
}

inline static void _insertBefore(list_t* pCurrent, list_t* pNew)
{
    list_t* pCurrentPrev = pCurrent->pPrev;
    list_t* pNewPrev     = pNew->pPrev;

    pCurrentPrev->pNext = pNew;
    pNew->pPrev         = pCurrentPrev;
    pNewPrev->pNext     = pCurrent;
    pCurrent->pPrev     = pNewPrev;
}

inline static void _insertAfter(list_t* pCurrent, list_t* pNew)
{
    list_t *pCurrentNext = pCurrent->pNext;
    list_t *pNewPrev     = pNew->pPrev;

    pCurrent->pNext     = pNew;
    pNew->pPrev         = pCurrent;
    pNewPrev->pNext     = pCurrentNext;
    pCurrentNext->pPrev = pNewPrev;
}

inline static void _remove(list_t* pNode)
{
    pNode->pPrev->pNext = pNode->pNext;
    pNode->pNext->pPrev = pNode->pPrev;

    pNode->pNext = pNode;
    pNode->pPrev = pNode;
}

inline static void _push(list_t** ppList, list_t* pNode)
{
    if (*ppList != NULL)
    {
        _insertBefore(*ppList, pNode);
    }

    *ppList = pNode;
}

inline static list_t* _pop(list_t** ppList)
{

    list_t* top = *ppList;
    list_t* nextTop = top->pNext;

    _remove(top);

    if (top == nextTop)
    {
           *ppList = NULL;
    }
    else
    {
           *ppList = nextTop;
    }

    return top;
}

inline static void _removeFrom(list_t** ppList, list_t* pNode)
{
    if (*ppList == pNode)
    {
        _pop(ppList);
    }
    else
    {
        _remove(pNode);
    }
}

inline static void _memoryChunkInit(mem_chunk_t* pChunk)
{
    LIST_INIT(pChunk, all);
    pChunk->used = FALSE;
    LIST_INIT(pChunk, free);
}

inline static uint32_t _memoryChunkSize(const mem_chunk_t* pChunk)
{
    return ((int8_t*)(pChunk->all.pNext) - (int8_t*)(&pChunk->all)) - HEADER_SIZE;
}

inline static int32_t _memoryChunkSlot(uint32_t size)
{
    int32_t n = -1;
    while(size > 0)
    {
        ++n;
        size /= 2;
    }
    return n;
}

inline static void _removeFree(mem_chunk_t* pChunk)
{
    uint32_t len = _memoryChunkSize(pChunk);
    int n = _memoryChunkSlot(len);

    LIST_REMOVE_FROM(&spFreeChunk[n], pChunk, free);
    sMemFree -= len;
}

inline static void _pushFree(mem_chunk_t *pChunk)
{
    uint32_t len = _memoryChunkSize(pChunk);
    int n = _memoryChunkSlot(len);

    LIST_PUSH(&spFreeChunk[n], pChunk, free);
    sMemFree += len;
}

void kHeapInit(void)
{
    mem_chunk_t* pSecond;
    uint32_t     len;
    int32_t      n;

    void*    pMem       = &_KERNEL_HEAP_BASE;
    uint32_t size      = (uint32_t)(uintptr_t)&_KERNEL_HEAP_SIZE;
    int8_t*  pMemStart = (int8_t*)
                        (((uintptr_t)pMem + ALIGN - 1) & (~(ALIGN - 1)));
    int8_t*  pMemEnd   = (int8_t*)(((uintptr_t)pMem + size) & (~(ALIGN - 1)));

    sMemFree       = 0;
    sKheapInitFree = 0;
    sMemMeta       = 0;
    spFirstChunk   = NULL;
    spLastChunk    = NULL;

    spFirstChunk = (mem_chunk_t*)pMemStart;
    pSecond = spFirstChunk + 1;
    spLastChunk = ((mem_chunk_t*)pMemEnd) - 1;

    _memoryChunkInit(spFirstChunk);
    _memoryChunkInit(pSecond);
    _memoryChunkInit(spLastChunk);

    _insertAfter(&spFirstChunk->all, &pSecond->all);
    _insertAfter(&pSecond->all, &spLastChunk->all);

    spFirstChunk->used = TRUE;
    spLastChunk->used  = TRUE;

    len = _memoryChunkSize(pSecond);
    n   = _memoryChunkSlot(len);

    LIST_PUSH(&spFreeChunk[n], pSecond, free);
    sMemFree = len;
    sKheapInitFree = sMemFree;
    sMemMeta = sizeof(mem_chunk_t) * 2 + HEADER_SIZE;

    KERNEL_SPINLOCK_INIT(sLock);

    sInit = TRUE;

    KERNEL_DEBUG(KHEAP_DEBUG_ENABLED, "KHEAP",
                 "Kernel Heap Initialized at 0x%p", pMemStart);

    TEST_POINT_FUNCTION_CALL(kheap_test, TEST_KHEAP_ENABLED);
}

void* kmalloc(size_t size)
{
    size_t       n;
    mem_chunk_t* pChunk;
    mem_chunk_t* pChunk2;
    size_t       size2;
    size_t       len;

    KERNEL_TRACE_EVENT(TRACE_KHEAP_ENABLED,
                       TRACE_KHEAP_MALLOC_ENTRY,
                       3,
                       (uint32_t)((uint64_t)size >> 32),
                       (uint32_t)size,
                       sMemFree);

    KERNEL_DEBUG(KHEAP_DEBUG_ENABLED,
                 "KHEAP",
                 "Kheap allocating %uB (%uB free, %uB used)",
                 size,
                 sMemFree,
                 sKheapInitFree - sMemFree);

    if(sInit == FALSE || size == 0)
    {
        KERNEL_TRACE_EVENT(TRACE_KHEAP_ENABLED,
                           TRACE_KHEAP_MALLOC_EXIT,
                           3,
                           (uint32_t)((uint64_t)size >> 32),
                           (uint32_t)size,
                           NULL,
                           NULL,
                           sMemFree);
        return NULL;
    }

    KERNEL_CRITICAL_LOCK(sLock);

    size = (size + ALIGN - 1) & (~(ALIGN - 1));

    if (size < MIN_SIZE)
    {
         size = MIN_SIZE;
    }

    n = _memoryChunkSlot(size - 1) + 1;

    if (n >= NUM_SIZES)
    {
        KERNEL_CRITICAL_UNLOCK(sLock);
        return NULL;
    }

    while(spFreeChunk[n] == 0)
    {
        ++n;
        if (n >= NUM_SIZES)
        {
            KERNEL_CRITICAL_UNLOCK(sLock);
            KERNEL_TRACE_EVENT(TRACE_KHEAP_ENABLED,
                               TRACE_KHEAP_MALLOC_EXIT,
                               5,
                               (uint32_t)((uint64_t)size >> 32),
                               (uint32_t)size,
                               NULL,
                               NULL,
                               sMemFree);
            return NULL;
        }
    }

    pChunk = LIST_POP(&spFreeChunk[n], free);
    size2 = _memoryChunkSize(pChunk);
    len = 0;

    if (size + sizeof(mem_chunk_t) <= size2)
    {
        pChunk2 = (mem_chunk_t*)(((int8_t*)pChunk) + HEADER_SIZE + size);

        _memoryChunkInit(pChunk2);

        _insertAfter(&pChunk->all, &pChunk2->all);

        len = _memoryChunkSize(pChunk2);
        n = _memoryChunkSlot(len);

        LIST_PUSH(&spFreeChunk[n], pChunk2, free);

        sMemMeta += HEADER_SIZE;
        sMemFree += len;
    }

    pChunk->used = TRUE;

    sMemFree -= size2;

    KERNEL_DEBUG(KHEAP_DEBUG_ENABLED,
                 "KHEAP",
                 "Kheap allocated 0x%p -> %uB (%uB free, %uB used)",
                 pChunk->pData,
                 size2 - len - HEADER_SIZE,
                 sMemFree,
                 sKheapInitFree - sMemFree);

    KERNEL_CRITICAL_UNLOCK(sLock);


#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KHEAP_ENABLED,
                       TRACE_KHEAP_MALLOC_EXIT,
                       5,
                       (uint32_t)((uint64_t)size >> 32),
                       (uint32_t)size,
                       0,
                       (uint32_t)pChunk->pData,
                       sMemFree);
#else
    KERNEL_TRACE_EVENT(TRACE_KHEAP_ENABLED,
                       TRACE_KHEAP_MALLOC_EXIT,
                       5,
                       (uint32_t)((uint64_t)size >> 32),
                       (uint32_t)size,
                       (uint32_t)(pChunk->pData >> 32),
                       (uint32_t)(pChunk->pData & 0xFFFFFFFF),
                       sMemFree);
#endif
    return pChunk->pData;
}

void kfree(void* ptr)
{
    mem_chunk_t* pChunk;
    mem_chunk_t* pNext;
    mem_chunk_t* pPrev;

#if KHEAP_DEBUG_ENABLED
    uint32_t     used;
#endif

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KHEAP_ENABLED,
                       TRACE_KHEAP_FREE_ENTRY,
                       3,
                       0,
                       (uint32_t)ptr,
                       sMemFree);
#else
    KERNEL_TRACE_EVENT(TRACE_KHEAP_ENABLED,
                       TRACE_KHEAP_FREE_ENTRY,
                       3,
                       (uint32_t)(ptr >> 32),
                       (uint32_t)(ptr & 0xFFFFFFFF),
                       sMemFree);
#endif

    if(sInit == FALSE || ptr == NULL)
    {
#ifdef ARCH_32_BITS
        KERNEL_TRACE_EVENT(TRACE_KHEAP_ENABLED,
                           TRACE_KHEAP_FREE_EXIT,
                           4,
                           0,
                           (uint32_t)ptr,
                           -1,
                           sMemFree);
#else
        KERNEL_TRACE_EVENT(TRACE_KHEAP_ENABLED,
                           TRACE_KHEAP_FREE_EXIT,
                           4,
                           (uint32_t)(ptr >> 32),
                           (uint32_t)(ptr & 0xFFFFFFFF),
                           -1,
                           sMemFree);
#endif
        return;
    }

    KERNEL_CRITICAL_LOCK(sLock);

    pChunk = (mem_chunk_t*)((int8_t*)ptr - HEADER_SIZE);
    pNext = CONTAINER(mem_chunk_t, all, pChunk->all.pNext);
    pPrev = CONTAINER(mem_chunk_t, all, pChunk->all.pPrev);

#if KHEAP_DEBUG_ENABLED
    used = _memoryChunkSize(pChunk);
#endif

    if (pNext->used == FALSE)
    {
        _removeFree(pNext);
        _remove(&pNext->all);

        sMemMeta -= HEADER_SIZE;
        sMemFree += HEADER_SIZE;
    }

    if (pPrev->used == FALSE)
    {
        _removeFree(pPrev);
        _remove(&pChunk->all);

        _pushFree(pPrev);
        sMemMeta -= HEADER_SIZE;
        sMemFree += HEADER_SIZE;
    }
    else
    {
        pChunk->used = FALSE;
        LIST_INIT(pChunk, free);
        _pushFree(pChunk);
    }
#if KHEAP_DEBUG_ENABLED
    KERNEL_DEBUG(KHEAP_DEBUG_ENABLED,
                 "KHEAP",
                 "[KHEAP] Kheap freed 0x%p -> %uB",
                 ptr,
                 used);
#endif

    KERNEL_CRITICAL_UNLOCK(sLock);

#ifdef ARCH_32_BITS
    KERNEL_TRACE_EVENT(TRACE_KHEAP_ENABLED,
                       TRACE_KHEAP_FREE_EXIT,
                       4,
                       0,
                       (uint32_t)ptr,
                       0,
                       sMemFree);
#else
    KERNEL_TRACE_EVENT(TRACE_KHEAP_ENABLED,
                       TRACE_KHEAP_FREE_EXIT,
                       4,
                       (uint32_t)(ptr >> 32),
                       (uint32_t)(ptr & 0xFFFFFFFF),
                       sMemFree);
#endif
}

uint32_t kHeapGetFree(void)
{
    return sMemFree;
}

/************************************ EOF *************************************/