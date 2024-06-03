/*******************************************************************************
 * @file uhashtable.c
 *
 * @see uhashtable.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/05/2023
 *
 * @version 1.0
 *
 * @brief Unsigned hash table structures.
 *
 * @details Unsigned hash table structures. Hash table are used to dynamically
 * store data, while growing when needed. This type of hash table can store data
 * pointers and values of the size of a pointer.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <stddef.h>  /* Standard definitions */
#include <stdint.h>  /* Generic int types */
#include <string.h>  /* String manipulation */
#include <kerror.h>  /* Kernel error codes */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <uhashtable.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/**
 * @brief Initial capacity and size of the hashtable.
 *
 * @warning Must be a power of 2.
 */
#define HT_INITIAL_SIZE 16

/** @brief Maximal factor size of the graveyard. */
#define HT_MAX_GRAVEYARD_FACTOR 0.3f

/**
 * @brief Maximal load factor (including graveyard).
 *
 * @warning: Must always be strictly less than 1.0
*/
#define HT_MAX_LOAD_FACTOR 0.7f

/** @brief FNV offset used for the hash function. */
#define FNV_OFFSET_BASIS 14695981039346656037UL

/** @brief Prime used in the FNV hash. */
#define FNV_PRIME 1099511628211UL

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief 64 bits hash function for uintptr_t keys.
 *
 * @details This function computes the 64 bits hash function for uintptr_t keys.
 * The algorithm used is the FNV-1a hash algorithm.
 *
 * @param[in] kKey The key to hash.
 *
 * @return The computed hash is returned.
 */
inline static uint64_t _uhash64(const uintptr_t kKey);

/**
 * @brief Sets the data for a given entry in the unsigned hash table.
 *
 * @details Sets the data for a given entry in the unsigned hash table. If the
 * entry does not exist it is created.
 *
 * @warning This function does not perform any check on the data, the key or the
 * table itself.
 *
 * @param[in,out] pTable The table to set the value.
 * @param[in] kKey The key to associate the data with.
 * @param[in] pData The data to set.
 *
 * @return The error status is returned.
 */
static OS_RETURN_E _uhashtableSetEntry(uhashtable_t*   pTable,
                                       const uintptr_t kKey,
                                       void*           pData);

/**
 * @brief Replaces an entry in the table, without allocating the entry.
 *
 * @details Replaces an entry in the table, without allocating the entry. The
 * entry beging used it the one given as parameter.
 *
 * @param[out] pTable The table to use.
 * @param[in] pEntry The entry to place.
 */
static void _uhashtableRehashEntry(uhashtable_t*       pTable,
                                   uhashtable_entry_t* pEntry);

/**
 * @brief Rehashes the table and grows the table by a certain factor.
 *
 * @details Rehash the table and grows the table by a certain factor. The growth
 * factor must be greater or equal to 1. In the latter case, only the rehashing
 * is done.
 *
 * @param[out] pTable The table to grow.
 * @param[in] kGrowth The growth factor to use.
 *
 * @return The error status is returned.
 */
static OS_RETURN_E _uhashtableRehash(uhashtable_t* pTable, const float kGrowth);


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

inline static uint64_t _uhash64(const uintptr_t kKey)
{
    uint64_t hash;
    size_t   i;

    /* We use the FNV-1a method to hash our integer */
    hash = FNV_OFFSET_BASIS;

    for(i = 0; i < sizeof(uintptr_t); ++i)
    {
        hash ^= (uint64_t)((uint8_t)(kKey >> (i * 8)));
        hash *= FNV_PRIME;
    }

    return hash;
}

static OS_RETURN_E _uhashtableSetEntry(uhashtable_t*     pTable,
                                         const uintptr_t kKey,
                                         void*           pData)
{
    uint64_t hash;
    size_t   entryIdx;

    /* Get the hash and ensure it does not overflow the current capacity */
    hash        = _uhash64(kKey);
    entryIdx = (size_t)(hash & (uint64_t)(pTable->capacity - 1));

    /* Search for the entry in the table. Note that due to the fact that our
     * load factor should always be under 1.0, there is at least one entry that
     * is NULL. Hence we cannot have an infinite loop here. */
    while(pTable->ppEntries[entryIdx] != NULL &&
          pTable->ppEntries[entryIdx]->isUsed == TRUE)
    {
        /* Found the entry */
        if(pTable->ppEntries[entryIdx]->key == kKey)
        {
            pTable->ppEntries[entryIdx]->pData = pData;
            return OS_NO_ERR;
        }

        /* Increment the index */
        entryIdx = (entryIdx + 1) % pTable->capacity;
    }

    /* If we are here, we did not find the data, allocate if needed */
    if(pTable->ppEntries[entryIdx] == NULL)
    {
        pTable->ppEntries[entryIdx] =
            pTable->allocator.pMalloc(sizeof(uhashtable_entry_t));
        if(pTable->ppEntries[entryIdx] == NULL)
        {
            return OS_ERR_NO_MORE_MEMORY;
        }

        ++pTable->size;
    }
    else
    {
        /* We found an entry from the graveyard, remove it */
        --pTable->graveyardSize;
    }

    /* Set the data */
    pTable->ppEntries[entryIdx]->key    = kKey;
    pTable->ppEntries[entryIdx]->pData  = pData;
    pTable->ppEntries[entryIdx]->isUsed = TRUE;

    return OS_NO_ERR;
}

static void _uhashtableRehashEntry(uhashtable_t*       pTable,
                                   uhashtable_entry_t* pEntry)
{
    uint64_t hash;
    size_t   entryIdx;

    /* Get the hash and ensure it does not overflow the current capacity */
    hash        = _uhash64(pEntry->key);
    entryIdx = (size_t)(hash & (uint64_t)(pTable->capacity - 1));

    /* Search for the entry in the table. Note that due to the fact that our
     * load factor should always be under 1.0, there is at least one entry that
     * is NULL. Hence we cannot have an infinite loop here. */
    while(pTable->ppEntries[entryIdx] != NULL &&
          pTable->ppEntries[entryIdx]->isUsed == TRUE)
    {
        /* Found the entry */
        if(pTable->ppEntries[entryIdx]->key == pEntry->key)
        {
            pTable->ppEntries[entryIdx]->pData = pEntry->pData;
            return;
        }

        /* Increment the index */
        entryIdx = (entryIdx + 1) % pTable->capacity;
    }

    /* Set the data */
    pTable->ppEntries[entryIdx] = pEntry;
}

static OS_RETURN_E _uhashtableRehash(uhashtable_t* pTable, const float kGrowth)
{
    size_t               i;
    size_t               newCapacity;
    size_t               oldCapacity;
    uhashtable_entry_t** ppNewEnt;
    uhashtable_entry_t** ppOldEnt;

    newCapacity = (size_t)((float)pTable->capacity * kGrowth);

    /* Check if did not overflow on the size */
    if(newCapacity <= pTable->capacity)
    {
        return OS_ERR_OUT_OF_BOUND;
    }

    /* Save old entries */
    ppOldEnt  = pTable->ppEntries;
    oldCapacity = pTable->capacity;

    /* Create a new entry table */
    ppNewEnt = pTable->allocator.pMalloc(sizeof(uhashtable_entry_t*) *
                                           newCapacity);
    if(ppNewEnt == NULL)
    {
        return OS_ERR_NO_MORE_MEMORY;
    }

    memset(ppNewEnt, 0, sizeof(uhashtable_entry_t*) * newCapacity);
    pTable->capacity = newCapacity;
    pTable->ppEntries  = ppNewEnt;

    /* Rehash the table, removes fragmentation and graveyard. */
    for(i = 0; i < oldCapacity; ++i)
    {
        if(ppOldEnt[i] != NULL)
        {
            /* Check if it was an entry that was used */
            if(ppOldEnt[i]->isUsed == TRUE)
            {
                _uhashtableRehashEntry(pTable, ppOldEnt[i]);
            }
            else
            {
                /* The entry was not used, we can free it */
                pTable->allocator.pFree(ppOldEnt[i]);
            }
        }
    }

    /* Clean data */
    pTable->graveyardSize = 0;
    pTable->allocator.pFree(ppOldEnt);

    return OS_NO_ERR;
}

uhashtable_t* uhashtableCreate(uhashtable_alloc_t allocator,
                                OS_RETURN_E*      pError)
{
    uhashtable_t* pTable;

    if(allocator.pFree == NULL || allocator.pMalloc == NULL)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_NULL_POINTER;
        }
        return NULL;
    }

    /* Initialize the table */
    pTable = allocator.pMalloc(sizeof(uhashtable_t));
    if(pTable == NULL)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_NO_MORE_MEMORY;
        }
        return NULL;
    }

    pTable->ppEntries = allocator.pMalloc(sizeof(uhashtable_entry_t*) *
                                      HT_INITIAL_SIZE);

    if(pTable->ppEntries == NULL)
    {
        allocator.pFree(pTable);
        if(pError != NULL)
        {
            *pError = OS_ERR_NO_MORE_MEMORY;
        }
        return NULL;
    }

    memset(pTable->ppEntries, 0, sizeof(uhashtable_entry_t*) * HT_INITIAL_SIZE);
    pTable->allocator     = allocator;
    pTable->capacity      = HT_INITIAL_SIZE;
    pTable->size          = 0;
    pTable->graveyardSize = 0;

    if(pError != NULL)
    {
        *pError = OS_NO_ERR;
    }

    return pTable;
}

OS_RETURN_E uhashtableDestroy(uhashtable_t* pTable)
{
    size_t i;

    if(pTable == NULL || pTable->ppEntries == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Free the memory used by the table */
    for(i = 0; i < pTable->capacity; ++i)
    {
        if(pTable->ppEntries[i] != NULL)
        {
            pTable->allocator.pFree(pTable->ppEntries[i]);
        }
    }
    pTable->allocator.pFree(pTable->ppEntries);

    /* Reset attributes */
    pTable->size           = 0;
    pTable->capacity       = 0;
    pTable->graveyardSize = 0;
    pTable->ppEntries        = NULL;

    /* Free memory */
    pTable->allocator.pFree(pTable);

    return OS_NO_ERR;
}

OS_RETURN_E uhashtableGet(const uhashtable_t* pTable,
                          const uintptr_t     kKey,
                          void**              ppData)
{
    uint64_t hash;
    size_t   entryIdx;

    if(pTable == NULL || ppData == NULL || pTable->ppEntries == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Get the hash and ensure it does not overflow the current capacity */
    hash        = _uhash64(kKey);
    entryIdx = (size_t)(hash & (uint64_t)(pTable->capacity - 1));

    /* Search for the entry in the table. Note that due to the fact that our
     * load factor should always be under 1.0, there is at least one entry that
     * is NULL. Hence we cannot have an infinite loop here. */
    while(pTable->ppEntries[entryIdx] != NULL)
    {
        /* Found the entry */
        if(pTable->ppEntries[entryIdx]->key == kKey &&
           pTable->ppEntries[entryIdx]->isUsed == TRUE)
        {
            *ppData = pTable->ppEntries[entryIdx]->pData;
            return OS_NO_ERR;
        }

        /* Increment the index */
        entryIdx = (entryIdx + 1) % pTable->capacity;
    }

    return OS_ERR_INCORRECT_VALUE;
}

OS_RETURN_E uhashtableSet(uhashtable_t*   pTable,
                          const uintptr_t kKey,
                          void*           pData)
{
    OS_RETURN_E err;

    if(pTable == NULL || pTable->ppEntries == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Check if the current load is under the threshold, double the size */
    if((float)pTable->capacity * HT_MAX_LOAD_FACTOR <
       pTable->size + pTable->graveyardSize)
    {
        err = _uhashtableRehash(pTable, 2);
        if(err != OS_NO_ERR)
        {
            return err;
        }
    }

    /* Insert the entry */
    return _uhashtableSetEntry(pTable, kKey, pData);
}

OS_RETURN_E uhashtableRemove(uhashtable_t*    pTable,
                              const uintptr_t kKey,
                              void**          ppData)
{
    uint64_t    hash;
    size_t      entryIdx;
    OS_RETURN_E err;

    if(pTable == NULL || pTable->ppEntries == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Get the hash and ensure it does not overflow the current capacity */
    hash        = _uhash64(kKey);
    entryIdx = (size_t)(hash & (uint64_t)(pTable->capacity - 1));

    /* Check if the graveyard load is under the threshold */
    if((float)pTable->capacity * HT_MAX_GRAVEYARD_FACTOR <
       pTable->graveyardSize)
    {
        /* Rehash with a growth factor or 1 to keep the same capacity */
        err = _uhashtableRehash(pTable, 1);
        if(err != OS_NO_ERR)
        {
            return err;
        }
    }

    /* Search for the entry in the table. Note that due to the fact that our
     * load factor should always be under 1.0, there is at least one entry that
     * is NULL. Hence we cannot have an infinite loop here. */
    while(pTable->ppEntries[entryIdx] != NULL)
    {
        /* Found the entry */
        if(pTable->ppEntries[entryIdx]->key == kKey &&
           pTable->ppEntries[entryIdx]->isUsed == TRUE)
        {
            if(ppData != NULL)
            {
                *ppData = pTable->ppEntries[entryIdx]->pData;
            }
            pTable->ppEntries[entryIdx]->isUsed = FALSE;

            ++pTable->graveyardSize;
            --pTable->size;

            return OS_NO_ERR;
        }

        /* Increment the index */
        entryIdx = (entryIdx + 1) % pTable->capacity;
    }

    return OS_ERR_INCORRECT_VALUE;
}

/************************************ EOF *************************************/