/*******************************************************************************
 * @file vector.h
 *
 * @see vector.c
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
 * @warning This implementation is not thread safe.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_VECTOR_H_
#define __LIB_VECTOR_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stddef.h> /* Standard definitons */
#include <stdint.h> /* Generic int types */
#include <kerror.h> /* Kernel error codes */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Vector allocator structure. */
typedef struct
{
    /**
     * @brief The memory allocation function used by the allocator.
     *
     * @param[in] allocSize The size in bytes to be allocated.
     *
     * @return A pointer to the allocated memory is returned. NULL is returned
     * if no memory was allocated.
     */
    void*(*pMalloc)(size_t allocSize);

    /**
     * @brief The memory free function used by the allocator.
     *
     * @param[out] ptr The start address of the memory to free.
     */
    void(*pFree)(void* ptr);
} vector_alloc_t;

/** @brief Vector structure. */
typedef struct
{
    /** @brief The allocator used by this vector. */
    vector_alloc_t allocator;

    /** @brief Storage array of the vector */
    void** ppArray;

    /** @brief Current vector's size. */
    size_t size;

    /** @brief Current vector's capacity. */
    size_t capacity;
} vector_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Create an allocator structure.
 *
 * @param[in] MALLOC The memory allocation function used by the allocator.
 * @param[in] FREE The memory free function used by the allocator.
 */
#define VECTOR_ALLOCATOR(MALLOC, FREE) (vector_alloc_t){MALLOC, FREE}

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

/**
 * @brief Creates a new vector.
 *
 * @details Creates a new vector of the size given as parameter. The vector will
 * allocate the required memory and initialize the memory with the value
 * provided as parameter.
 *
 * @param[in] allocator The allocator to be used when allocating and freeing the
 * vector.
 * @param[in] pInitData The initial data to set to the elements of the vector
 * during its creation.
 * @param[in] kSize The initial size of the vector.
 * @param[out] pError The error buffer to received the error status.
 *
 * @return A pointer to the created vector is returned.
 */
vector_t* vectorCreate(vector_alloc_t allocator,
                       void*          pInitData,
                       const size_t   kSize,
                       OS_RETURN_E*   pError);

/**
 * @brief Destroys a vector.
 *
 * @details Destroys a vector. The memory used by the vector is released,
 * however, if its elements were allocated dynamically, it is the responsability
 * of the user to release the memory of these elements.
 *
 * @param[out] pVector The vector to destroy.
 *
 * @return The error status is retuned.
 */
OS_RETURN_E vectorDestroy(vector_t* pVector);

/**
 * @brief Clears a vector.
 *
 * @details Clears a vector. The memory used by the vector is not released and
 * its capacity unchanged. If its elements were allocated dynamically, it is the
 * responsability of the user to release the memory of these elements.
 *
 * @param[out] pVector The vector to clear.
 *
 * @return The error status is retuned.
 */
OS_RETURN_E vectorClear(vector_t* pVector);

/**
 * @brief Performs a copy of a vector and returns it.
 *
 * @details Copies a vector to another vector. The function
 * initialize the destination vector before performing the copy.
 *
 * @param[in] pSrc The source vector.
 * @param[in] pError The error status buffer.
 *
 * @return The copy of the vector is returned.
 */
vector_t* vectorCopy(const vector_t* pSrc, OS_RETURN_E* pError);

/**
 * @brief Shrinks a vector.
 *
 * @details Shrinks a vector. The memory used by the vector that is empty is
 * released and its capacity set to fit the number of element that the vector
 * contains.
 *
 * @param[out] pVector The vector to shrink.
 *
 * @return The error status is retuned.
 */
OS_RETURN_E vectorSrink(vector_t* pVector);

/**
 * @brief Resizes a vector.
 *
 * @details Resizes a vector. If the size set is smaller than the current
 * capacity of the vector, the size is reduced but the capacity unchanged.
 *  If its elements were allocated dynamically, it is the responsability of the
 * user to release the memory of these elements. If the size is biger than the
 * vector's capacity, it is expanded to fit the new size.
 *
 * @param[out] pVector The vector to resize.
 * @param[in] kSize The size of apply to the vector.
 *
 * @return The error status is retuned.
 */
OS_RETURN_E vectorResize(vector_t* pVector, const size_t kSize);

/**
 * @brief Inserts an element in the vector at the position provided in the
 * parameters.
 *
 * @details Inserts an element in the vector at the position provided in the
 * parameters. If the position is greater than the size, an error is returned.
 *
 * @param[out] pVector The vector to modify.
 * @param[in] pData The data to insert.
 * @param[in] kPosition The position to insert the data to.
 *
 * @return The error status is retuned.
 */
OS_RETURN_E vectorInsert(vector_t*    pVector,
                         void*        pData,
                         const size_t kPosition);

/**
 * @brief Inserts an element at the end of the vector.
 *
 * @details Inserts an element at the end of the vector.
 *
 * @param[out] pVector The vector to modify.
 * @param[in] pData The data to insert.
 *
 * @return The error status is retuned.
 */
OS_RETURN_E vectorPush(vector_t* pVector, void* pData);

/**
 * @brief Removes an element at the end of the vector.
 *
 * @details Removes an element at the end of the vector.
 *
 * @param[out] pVector The vector to modify.
 * @param[out] ppData The data buffer to retreive the element that was removed.
 *
 * @return The error status is retuned.
 */
OS_RETURN_E vectorPop(vector_t* pVector, void** ppData);

/**
 * @brief Returns the value of an element.
 *
 * @details Returns the value of an element at the position provided in
 * parameters. Note that the vector's array can be accesses directly from the
 * structure's attributes. This function tests the bounds of the vector to
 * ensure the user accesses a position inside the vector.
 *
 * @param[in] pVector The vector to use.
 * @param[in] kPosition The position of the element to retrieve.
 * @param[out] ppData The data buffer to retreive the element.
 *
 * @return The error status is retuned.
 */
OS_RETURN_E vectorGet(const vector_t* pVector,
                      const size_t    kPosition,
                      void**          ppData);

/**
 * @brief Sets the value of an element.
 *
 * @details Sets the value of an element at the position provided in
 * parameters. Note that the vector's array can be accesses directly from the
 * structure's attributes. This function tests the bounds of the vector to
 * ensure the user accesses a position inside the vector.
 *
 * @param[out] pVector The vector to use.
 * @param[in] kPosition The position of the element to retrieve.
 * @param[in] pData The data to set at the given position.
 *
 * @return The error status is retuned.
 */
OS_RETURN_E vectorSet(vector_t* pVector, const size_t kPosition, void* pData);

#endif /* #ifndef __LIB_VECTOR_H_ */

/************************************ EOF *************************************/