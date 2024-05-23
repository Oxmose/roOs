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
#include <stdint.h>         /* Generic integer definitions */
#include <string.h>         /* Memory manipulation */
#include <kerneloutput.h>  /* Kernel IO */
#include <critical.h>       /* Critical sections */

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
#define HEADER_SIZE __builtin_offsetof(mem_chunk_t, data)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Kernel's heap allocator list node. */
typedef struct list
{
    /** @brief Next node of the list. */
    struct list* next;
    /** @brief Previous node of the list. */
    struct list* prev;
} list_t;

/** @brief Kernel's heap allocator memory chunk representation. */
typedef struct
{
    /** @brief Memory chunk list. */
    list_t all;

    /** @brief Used flag. */
    bool_t used;

    /** @brief If used, the union contains the chunk's data, else a list of free
     * mem.
     */
    union {
           uint8_t* data;
           list_t   free;
    };
} mem_chunk_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

#define CONTAINER(C, l, v) ((C*)(((char*)v) - (uintptr_t)&(((C*)0)->l)))

#define LIST_INIT(v, l) _list_init(&v->l)

#define LIST_REMOVE_FROM(h, d, l)                           \
{                                                           \
    __typeof__(**h) **h_ = h, *d_ = d;                      \
    list_t* head = &(*h_)->l;                               \
    _remove_from(&head, &d_->l);                            \
    if (head == NULL)                                       \
    {                                                       \
          *h_ = NULL;                                       \
    }                                                       \
    else                                                    \
    {                                                       \
        *h_ = CONTAINER(__typeof__(**h), l, head);          \
    }                                                       \
}

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
            next_##it = iter_##it->next;                                    \
        }                                                                   \
        __typeof__(*h)* it = CONTAINER(__typeof__(*h), l, iter_##it);       \

#define LIST_ITERATOR_END(it)                       \
    }while((iter_##it = next_##it));                \
}

#define LIST_ITERATOR_REMOVE_FROM(h, it, l) LIST_REMOVE_FROM(h, iter_##it, l)

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
static bool_t init = FALSE;

/* Heap data */
/** @brief Kernel's heap free memory chunks. */
static mem_chunk_t* free_chunk[NUM_SIZES] = { NULL };
/** @brief Kernel's heap first memory chunk. */
static mem_chunk_t* first_chunk;
/** @brief Kernel's heap last memory chunk. */
static mem_chunk_t* last_chunk;

/** @brief Quantity of free memory in the kernel's heap. */
static uint32_t mem_free;
/** @brief Quantity of initial free memory in the kernel's heap. */
static uint32_t kheap_init_free;
/** @brief Quantity of memory used to store meta data in the kernel's heap. */
static uint32_t mem_meta;

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Initializes the memory list.
 *
 * @details Initializes the memory list with the basic node value.
 *
 * @param[out] node The list's node to initialize.
 */
inline static void _list_init(list_t* node);

/**
 * @brief Inserts a node before the current node in the list.
 *
 * @details Inserts a node before the current node in the list.
 *
 * @param[in,out] current The current node.
 * @param[in,out] new The new node to insert before the current node.
 */
inline static void _insert_before(list_t* current, list_t* new);

/**
 * @brief Inserts a node after the current node in the list.
 *
 * @param[in,out] current The current node.
 * @param[in,out] new The new node to insert after the current node.
 */
inline static void _insert_after(list_t* current, list_t* new);

/**
 * @brief Removes a node from the list.
 *
 * @details Removes a node from the list.
 *
 * @param[out] node The node to remove from the list.
 */
inline static void _remove(list_t* node);

/**
 * @brief Pushes a node at the end of the list.
 *
 * @details Pushes a node at the end of the list.
 *
 * @param[out] list The list to be pushed.
 * @param[in] node The node to push to the list.
 */
inline static void _push(list_t** list, list_t* node);

/**
 * @brief Pops a node from the list.
 *
 * @details Pops a node from the list and returns it.
 *
 * @param[out] list The list to be poped from.
 *
 * @return The node poped from the list is returned.
 */
inline static list_t* _pop(list_t** list);


/**
 * @brief Removes a node from the list.
 *
 * @details Removes a node from the list.
 *
 * @param[out] list The list to remove the node from.
 * @param[out] node The node to remove from the list.
 */
inline static void _remove_from(list_t** list, list_t* node);


/**
 * @brief Initializes a memory chunk structure.
 *
 * @details Initializes a memory chunk structure.
 *
 * @param[out] chunk The chunk structure to initialize.
 */
inline static void _memory_chunk_init(mem_chunk_t* chunk);

/**
 * @brief Returns the size of a memory chunk.
 *
 * @param chunk The chunk to get the size of.
 *
 * @return The size of the memory chunk is returned.
 */
inline static uint32_t _memory_chunk_size(const mem_chunk_t* chunk);

/**
 * @brief Returns the slot of a memory chunk for the desired size.
 *
 * @details Returns the slot of a memory chunk for the desired size.
 *
 * @param[in] size The size of the chunk to get the slot of.
 *
 * @return The slot of a memory chunk for the desired size.
 */
inline static int32_t _memory_chunk_slot(uint32_t size);

/**
 * @brief Removes a memory chunk in the free memory chunks list.
 *
 * @details Removes a memory chunk in the free memory chunks list.
 *
 * @param[in, out] chunk The chunk to be removed from the list.
 */
inline static void _remove_free(mem_chunk_t* chunk);

/**
 * @brief Pushes a memory chunk in the free memory chunks list.
 *
 * @details Pushes a memory chunk in the free memory chunks list.
 *
 * @param[in, out] chunk The chunk to be placed in the list.
 */
inline static void _push_free(mem_chunk_t *chunk);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

inline static void _list_init(list_t* node)
{
    node->next = node;
    node->prev = node;
}

inline static void _insert_before(list_t* current, list_t* new)
{
    list_t* current_prev = current->prev;
    list_t* new_prev = new->prev;

    current_prev->next = new;
    new->prev = current_prev;
    new_prev->next = current;
    current->prev = new_prev;
}

inline static void _insert_after(list_t* current, list_t* new)
{
    list_t *current_next = current->next;
    list_t *new_prev = new->prev;

    current->next = new;
    new->prev = current;
    new_prev->next = current_next;
    current_next->prev = new_prev;
}

inline static void _remove(list_t* node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = node;
    node->prev = node;
}

inline static void _push(list_t** list, list_t* node)
{
    if (*list != NULL)
    {
        _insert_before(*list, node);
    }

    *list = node;
}

inline static list_t* _pop(list_t** list)
{

    list_t* top = *list;
    list_t* next_top = top->next;

    _remove(top);

    if (top == next_top)
    {
           *list = NULL;
    }
    else
    {
           *list = next_top;
    }

    return top;
}

inline static void _remove_from(list_t** list, list_t* node)
{
    if (*list == node)
    {
        _pop(list);
    }
    else
    {
        _remove(node);
    }
}

inline static void _memory_chunk_init(mem_chunk_t* chunk)
{
    LIST_INIT(chunk, all);
    chunk->used = FALSE;
    LIST_INIT(chunk, free);
}

inline static uint32_t _memory_chunk_size(const mem_chunk_t* chunk)
{
    return ((int8_t*)(chunk->all.next) - (int8_t*)(&chunk->all)) - HEADER_SIZE;
}

inline static int32_t _memory_chunk_slot(uint32_t size)
{
    int32_t n = -1;
    while(size > 0)
    {
        ++n;
        size /= 2;
    }
    return n;
}

inline static void _remove_free(mem_chunk_t* chunk)
{
    uint32_t len = _memory_chunk_size(chunk);
    int n = _memory_chunk_slot(len);

    LIST_REMOVE_FROM(&free_chunk[n], chunk, free);
    mem_free -= len;
}

inline static void _push_free(mem_chunk_t *chunk)
{
    uint32_t len = _memory_chunk_size(chunk);
    int n = _memory_chunk_slot(len);

    LIST_PUSH(&free_chunk[n], chunk, free);
    mem_free += len;
}

void kheap_init(void)
{
    mem_chunk_t* second;
    uint32_t len;
    int32_t  n;

    void* mem = &_KERNEL_HEAP_BASE;
    uint32_t size = (uint32_t)(uintptr_t)&_KERNEL_HEAP_SIZE;
    int8_t* mem_start = (int8_t*)
                        (((uintptr_t)mem + ALIGN - 1) & (~(ALIGN - 1)));
    int8_t* mem_end = (int8_t*)(((uintptr_t)mem + size) & (~(ALIGN - 1)));

#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_HEAP_INIT_START, 2,
                       (uintptr_t)mem & 0xFFFFFFFF,
                       (uintptr_t)mem >> 32);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_HEAP_INIT_START, 2,
                       (uintptr_t)mem & 0xFFFFFFFF,
                       (uintptr_t)0);
#endif

    mem_free = 0;
    kheap_init_free = 0;
    mem_meta = 0;
    first_chunk = NULL;
    last_chunk = NULL;

    first_chunk = (mem_chunk_t*)mem_start;
    second = first_chunk + 1;

    last_chunk = ((mem_chunk_t*)mem_end) - 1;

    _memory_chunk_init(first_chunk);
    _memory_chunk_init(second);

    _memory_chunk_init(last_chunk);
    _insert_after(&first_chunk->all, &second->all);
    _insert_after(&second->all, &last_chunk->all);

    first_chunk->used = TRUE;
    last_chunk->used = TRUE;

    len = _memory_chunk_size(second);
    n   = _memory_chunk_slot(len);

    LIST_PUSH(&free_chunk[n], second, free);
    mem_free = len;
    kheap_init_free = mem_free;
    mem_meta = sizeof(mem_chunk_t) * 2 + HEADER_SIZE;

    init = TRUE;

    KERNEL_DEBUG(KHEAP_DEBUG_ENABLED, "KHEAP",
                 "Kernel Heap Initialized at 0x%p", mem_start);

#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_HEAP_INIT_END, 3,
                       (uintptr_t)mem & 0xFFFFFFFF,
                       (uintptr_t)mem >> 32,
                       kheap_init_free);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_HEAP_INIT_END, 3,
                       (uintptr_t)mem & 0xFFFFFFFF,
                       (uintptr_t)0,
                       kheap_init_free);
#endif

    TEST_POINT_FUNCTION_CALL(kheap_test, TEST_KHEAP_ENABLED);
}

void* kmalloc(size_t size)
{
    size_t       n;
    mem_chunk_t* chunk;
    mem_chunk_t* chunk2;
    size_t       size2;
    size_t       len;
    uint32_t     int_state;

    KERNEL_DEBUG(KHEAP_DEBUG_ENABLED, "KHEAP",
                 "Kheap allocating %uB (%uB free, %uB used)",
                 size, mem_free,
                 kheap_init_free - mem_free);

    if(init == FALSE || size == 0)
    {
        return NULL;
    }

    ENTER_CRITICAL(int_state);

    size = (size + ALIGN - 1) & (~(ALIGN - 1));

    if (size < MIN_SIZE)
    {
         size = MIN_SIZE;
    }

    n = _memory_chunk_slot(size - 1) + 1;

    if (n >= NUM_SIZES)
    {
        EXIT_CRITICAL(int_state);
        return NULL;
    }

    while(free_chunk[n] == 0)
    {
        ++n;
        if (n >= NUM_SIZES)
        {
            EXIT_CRITICAL(int_state);
            return NULL;
        }
    }

    chunk = LIST_POP(&free_chunk[n], free);
    size2 = _memory_chunk_size(chunk);
    len = 0;

    if (size + sizeof(mem_chunk_t) <= size2)
    {
        chunk2 = (mem_chunk_t*)(((int8_t*)chunk) + HEADER_SIZE + size);

        _memory_chunk_init(chunk2);

        _insert_after(&chunk->all, &chunk2->all);

        len = _memory_chunk_size(chunk2);
        n = _memory_chunk_slot(len);

        LIST_PUSH(&free_chunk[n], chunk2, free);

        mem_meta += HEADER_SIZE;
        mem_free += len;
    }

    chunk->used = TRUE;

    mem_free -= size2;

    KERNEL_DEBUG(KHEAP_DEBUG_ENABLED, "KHEAP",
                 "Kheap allocated 0x%p -> %uB (%uB free, %uB used)",
                 chunk->data, size2 - len - HEADER_SIZE, mem_free,
                 kheap_init_free - mem_free);

    EXIT_CRITICAL(int_state);

#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_HEAP_ALLOC, 4,
                       (uintptr_t)chunk->data & 0xFFFFFFFF,
                       (uintptr_t)chunk->data >> 32,
                       size,
                       mem_free);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_HEAP_ALLOC, 4,
                       (uintptr_t)chunk->data & 0xFFFFFFFF,
                       (uintptr_t)0,
                       size,
                       mem_free);
#endif

    return chunk->data;
}

void kfree(void* ptr)
{
    uint32_t     used;
    mem_chunk_t* chunk;
    mem_chunk_t* next;
    mem_chunk_t* prev;
    uint32_t     int_state;

    if(init == FALSE || ptr == NULL)
    {
        return;
    }

    ENTER_CRITICAL(int_state);

    chunk = (mem_chunk_t*)((int8_t*)ptr - HEADER_SIZE);
    next = CONTAINER(mem_chunk_t, all, chunk->all.next);
    prev = CONTAINER(mem_chunk_t, all, chunk->all.prev);

    used = _memory_chunk_size(chunk);

    if (next->used == FALSE)
    {
        _remove_free(next);
        _remove(&next->all);

        mem_meta -= HEADER_SIZE;
        mem_free += HEADER_SIZE;
    }

    if (prev->used == FALSE)
    {
        _remove_free(prev);
        _remove(&chunk->all);

        _push_free(prev);
        mem_meta -= HEADER_SIZE;
        mem_free += HEADER_SIZE;
    }
    else
    {
        chunk->used = FALSE;
        LIST_INIT(chunk, free);
        _push_free(chunk);
    }

    KERNEL_DEBUG(KHEAP_DEBUG_ENABLED, "KHEAP",
                 "[KHEAP] Kheap freed 0x%p -> %uB", ptr, used);

#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_HEAP_FREE, 3,
                       (uintptr_t)ptr & 0xFFFFFFFF,
                       (uintptr_t)ptr >> 32,
                       mem_free);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_HEAP_FREE, 3,
                       (uintptr_t)ptr & 0xFFFFFFFF,
                       (uintptr_t)0,
                       mem_free);
#endif

    EXIT_CRITICAL(int_state);
}

uint32_t kheap_get_free(void)
{
    return mem_free;
}

/************************************ EOF *************************************/