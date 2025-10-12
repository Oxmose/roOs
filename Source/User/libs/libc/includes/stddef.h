/*******************************************************************************
 * @file stddef.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief Standard definitions for the kernel.
 *
 * @details Standard definitions for the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_STDDEF_H_
#define __LIB_STDDEF_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h> /* Generic int types */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief NULL definition. */
#define NULL ((void *)0)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/


/**
 * @brief Defines size_t type as a renaming for __SIZE_TYPE__.
 */
typedef __SIZE_TYPE__ size_t;

/**
 * @brief Defines size_t type as a renaming for __SIZE_TYPE__.
 */
typedef long long int ssize_t;

#ifndef __PTRDIFF_TYPE__
#error __PTRDIFF_TYPE__ not defined
#endif

/**
 * @brief Defines ptrdiff_t type as a renaming for __PTRDIFF_TYPE__.
 */
typedef __PTRDIFF_TYPE__ ptrdiff_t;

/**
 * @brief Defines uintptr_t type as address type.
 */
#ifdef ARCH_64_BITS
typedef uint64_t uintptr_t;
#elif defined(ARCH_32_BITS)
typedef uint32_t uintptr_t;
#else
#error Architecture is not supported by the standard library
#endif

#ifdef ARCH_64_BITS
/** @brief Defines the format for a pointer */
#define PRIPTR  "%lu"
#elif defined(ARCH_32_BITS)
/** @brief Defines the format for a pointer */
#define PRIPTR  "%u"
#else
#error Architecture is not supported by the standard library
#endif

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Defines the MIN function, return the minimal value between two
 * variables.
 *
 * @param[in] x The first value to compare.
 * @param[in] y The second value to compare.
 *
 * @return The smallest value.
 */
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/**
 * @brief Defines the MAX function, return the maximal value between two
 * variables.
 *
 * @param[in] x The first value to compare.
 * @param[in] y The second value to compare.
 *
 * @return The biggest value.
 */
#define MAX(x, y) ((x) < (y) ? (y) : (x))

/**
 * @brief Defines the ABS function, return the absolute value of a variable.
 *
 * @param[in] x The value to absolute.
 *
 * @return The absolute value.
 */
#define ABS(X) ((X) < 0 ? -(X) : (X))

/**
 * @brief Returns the number of elements in a statically allocated array.
 *
 * @param[in] x The array to calculate the size of.
 *
 * @return THe number of elements in the array.
 *
 * @warning This macro shall only by used with statically allocated arrays.
 */
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

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

/* None */

#endif /* #ifndef __LIB_STDDEF_H_ */

/************************************ EOF *************************************/