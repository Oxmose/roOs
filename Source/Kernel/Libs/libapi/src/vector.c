/*******************************************************************************
 * @file vector.c
 *
 * @see vector.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/05/2023
 *
 * @version 1.0
 *
 * @brief Vector structures.
 *
 * @details Vector structures. Vectors are used to dynamically store data, while
 * growing when needed. This type of vector can store data pointers and values
 * of the size of a pointer.
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
#include <kerror.h>  /* Kernel errors */

/* Configuration files */
#include <config.h>

/* Unit test header */
#include <test_framework.h>

/* Header file */
#include <vector.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/**
 * @brief Growth factor used when the vector has not space left.
 *
 * @warning This value must be greater than 1.
*/
#define VECTOR_GROWTH_FACTOR 2

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/


/**
 * @brief Used to grow the size of a vector. The new vector is filled with
 * previous data.
 *
 * @param[out] VECTOR The vector to update.
 * @param[out] NEW_SIZE The new size to be computed.
 * @param[out] NEW_ARRAY The array to receive the created memory region.
*/
#define GROW_VECTOR_SIZE(VECTOR, NEW_SIZE, NEW_ARRAY) {                     \
    if(VECTOR->capacity == VECTOR->size)                                    \
    {                                                                       \
        NEW_SIZE = VECTOR->capacity * VECTOR_GROWTH_FACTOR;                 \
        if(NEW_SIZE == 0)                                                   \
        {                                                                   \
            NEW_SIZE = VECTOR_GROWTH_FACTOR;                                \
        }                                                                   \
                                                                            \
        /* Check if did not overflow on the size */                         \
        if(NEW_SIZE <= VECTOR->capacity)                                    \
        {                                                                   \
            return OS_ERR_OUT_OF_BOUND;                                     \
        }                                                                   \
                                                                            \
        /* Allocate new array */                                            \
        NEW_ARRAY = VECTOR->allocator.pMalloc(NEW_SIZE * sizeof(void*));    \
        if(NEW_ARRAY == NULL)                                               \
        {                                                                   \
            return OS_ERR_NO_MORE_MEMORY;                                   \
        }                                                                   \
                                                                            \
        /* Copy array */                                                    \
        memcpy(NEW_ARRAY, VECTOR->ppArray, VECTOR->size * sizeof(void*));     \
                                                                            \
        /* Free old array */                                                \
        VECTOR->allocator.pFree(VECTOR->ppArray);                             \
        VECTOR->ppArray    = NEW_ARRAY;                                       \
        VECTOR->capacity = NEW_SIZE;                                        \
    }                                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
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
 * FUNCTIONS
 ******************************************************************************/

vector_t* vectorCreate(vector_alloc_t allocator,
                       void*          pInitData,
                       const size_t   kSize,
                       OS_RETURN_E*   pError)
{
    vector_t* pVector;
    size_t    i;

    if(allocator.pMalloc == NULL || allocator.pFree == NULL)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_NULL_POINTER;
        }
        return NULL;
    }

    pVector = allocator.pMalloc(sizeof(vector_t));
    if(pVector == NULL)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_NO_MORE_MEMORY;
        }
        return NULL;
    }

    /* Allocate the data */
    pVector->ppArray = NULL;
    if(kSize != 0)
    {
        pVector->ppArray = allocator.pMalloc(kSize * sizeof(void*));
        if(pVector->ppArray == NULL)
        {
            if(pError != NULL)
            {
                *pError = OS_ERR_NO_MORE_MEMORY;
            }
            return NULL;
        }
    }

    /* Initialize the data */
    for(i = 0; i < kSize; ++i)
    {
        pVector->ppArray[i] = pInitData;
    }

    /* Initialize the attributes */
    pVector->allocator = allocator;
    pVector->size      = kSize;
    pVector->capacity  = kSize;

    if(pError != NULL)
    {
        *pError = OS_NO_ERR;
    }
    return pVector;
}

OS_RETURN_E vectorDestroy(vector_t* pVector)
{
    if(pVector == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Release the data */
    if(pVector->ppArray != NULL)
    {
        pVector->allocator.pFree(pVector->ppArray);
    }

    /* Reset the attributes */
    pVector->ppArray   = NULL;
    pVector->size      = 0;
    pVector->capacity  = 0;

    /* Free vector structure */
    pVector->allocator.pFree(pVector);

    return OS_NO_ERR;
}

OS_RETURN_E vectorClear(vector_t* pVector)
{
    if(pVector == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    pVector->size = 0;

    return OS_NO_ERR;
}

vector_t* vectorCopy(const vector_t* pSrc, OS_RETURN_E* pError)
{
    vector_t* pCopy;

    if(pSrc == NULL)
    {
        if(pError != NULL)
        {
            *pError = OS_ERR_NULL_POINTER;
        }
        return NULL;
    }

    pCopy = vectorCreate(pSrc->allocator, NULL, pSrc->capacity, pError);
    if(pCopy == NULL || *pError != OS_NO_ERR)
    {
        return NULL;
    }

    /* Here we do not need to copy the entire array, just the part that contains
     * valid data as size <= capacity.
     */
    pCopy->size = pSrc->size;
    memcpy(pCopy->ppArray, pSrc->ppArray, pSrc->size * sizeof(void*));

    if(pError != NULL)
    {
        *pError = OS_NO_ERR;
    }

    return pCopy;
}

OS_RETURN_E vectorSrink(vector_t* pVector)
{
    void* pNewArray;

    if(pVector == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Only resize if the capacity is different than the size */
    if(pVector->capacity > pVector->size)
    {
        if(pVector->size != 0)
        {
            /* Allocate new array */
            pNewArray = pVector->allocator.pMalloc(pVector->size *
                                                   sizeof(void*));
            if(pNewArray == NULL)
            {
                return OS_ERR_NO_MORE_MEMORY;
            }

            /* Copy array */
            memcpy(pNewArray, pVector->ppArray, pVector->size * sizeof(void*));

            /* Free old array */
            pVector->allocator.pFree(pVector->ppArray);
            pVector->ppArray  = pNewArray;
            pVector->capacity = pVector->size;
        }
        else
        {
            /* Free all memory */
            pVector->allocator.pFree(pVector->ppArray);
            pVector->ppArray  = NULL;
            pVector->capacity = 0;
        }
    }

    return OS_NO_ERR;
}

OS_RETURN_E vectorResize(vector_t* pVector, const size_t kSize)
{
    void* pNewArray;

    if(pVector == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* Only resize if the capacity is different than the capacity */
    if(pVector->capacity < kSize)
    {
        if(kSize != 0)
        {
            /* Allocate new array */
            pNewArray = pVector->allocator.pMalloc(kSize * sizeof(void*));
            if(pNewArray == NULL)
            {
                return OS_ERR_NO_MORE_MEMORY;
            }

            /* Copy array */
            memcpy(pNewArray,
                   pVector->ppArray,
                   MAX(pVector->size, kSize) * sizeof(void*));

            /* Free old array */
            pVector->allocator.pFree(pVector->ppArray);
            pVector->ppArray  = pNewArray;
            pVector->capacity = kSize;
            pVector->size     = kSize;
        }
        else
        {
            /* Free all memory */
            pVector->allocator.pFree(pVector->ppArray);
            pVector->ppArray  = NULL;
            pVector->capacity = 0;
            pVector->size     = 0;
        }
    }
    else
    {
        pVector->size = kSize;
    }

    return OS_NO_ERR;
}

OS_RETURN_E vectorInsert(vector_t*     pVector,
                          void*        pData,
                          const size_t kPosition)
{
    size_t newSize;
    size_t i;
    void** ppNewArray;

    if(pVector == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(kPosition > pVector->size)
    {
        return OS_ERR_OUT_OF_BOUND;
    }

    /* First, check if we should update the capacity of the vector */
    GROW_VECTOR_SIZE(pVector, newSize, ppNewArray);

    /* Move the old data and insert the new data */
    for(i = pVector->size; i > kPosition; --i)
    {
        pVector->ppArray[i] = pVector->ppArray[i - 1];
    }
    pVector->ppArray[kPosition] = pData;
    ++pVector->size;

    return OS_NO_ERR;
}

OS_RETURN_E vectorPush(vector_t* pVector, void* pData)
{
    size_t newSize;
    void** ppNewArray;

    if(pVector == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    /* First, check if we should update the capacity of the vector */
    GROW_VECTOR_SIZE(pVector, newSize, ppNewArray);

    /* Insert the new data */
    pVector->ppArray[pVector->size++] = pData;

    return OS_NO_ERR;
}

OS_RETURN_E vectorPop(vector_t* pVector, void** pData)
{
    if(pVector == NULL || pData == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(pVector->size == 0)
    {
        return OS_ERR_OUT_OF_BOUND;
    }

    /* Return the last data */
    *pData = pVector->ppArray[--pVector->size];

    return OS_NO_ERR;
}

OS_RETURN_E vectorGet(const vector_t* pVector,
                       const size_t   kPosition,
                       void**         ppData)
{
    if(pVector == NULL || ppData == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(kPosition >= pVector->size)
    {
        return OS_ERR_OUT_OF_BOUND;
    }

    /* Return the data */
    *ppData = pVector->ppArray[kPosition];

    return OS_NO_ERR;
}

OS_RETURN_E vectorSet(vector_t* pVector, const size_t kPposition, void* pData)
{
    if(pVector == NULL)
    {
        return OS_ERR_NULL_POINTER;
    }

    if(kPposition >= pVector->size)
    {
        return OS_ERR_OUT_OF_BOUND;
    }

    /* Sets the data */
    pVector->ppArray[kPposition] = pData;

    return OS_NO_ERR;
}

/************************************ EOF *************************************/