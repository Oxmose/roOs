/*******************************************************************************
 * @file stdlib.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief Kernel's standard lib functions.
 *
 * @details Standard lib functions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_STDLIB_H_
#define __LIB_STDLIB_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h> /* Generic int types */
#include <stddef.h> /* Standard definitions */

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
 * FUNCTIONS
 ******************************************************************************/

/**
 * @brief Convert a signed integer value to a string.
 *
 * @details Convert a signed integer value to a string and inject the conversion
 * result in the buffer given as parameter.
 *
 * @param[in] value The value to convert.
 * @param[out] buf The buffer to receive the convertion's result.
 * @param[in] base The base of the signed integer to convert.
 */
void itoa(int64_t value, char* buf, uint32_t base);

/**
 * @brief Convert a unsigned integer value to a string.
 *
 * @details Convert a unsigned integer value to a string and inject the
 * conversion result in the buffer given as parameter.
 *
 * @param[in] value The value to convert.
 * @param[out] buf The buffer to receive the convertion's result.
 * @param[in] base The base of the unsigned integer to convert.
 */
void uitoa(uint64_t value, char* buf, uint32_t base);

/**
 * @brief Formats a string to a buffer in memory.
 *
 * @details Formats a string to a buffer in memory. The buffer is not filled
 * more than its size.
 *
 * @param[out] pBuffer The buffer to fill.
 * @param[in] size The maximal size of the buffer.
 * @param[in] kpFmt The format string to use.
 * @param[in] ... Additional parameters for the format.
 */
int snprintf(char* pBuffer, size_t size, const char * kpFmt, ...);

/**
 * @brief Formats a string to a buffer in memory.
 *
 * @details Formats a string to a buffer in memory. The buffer is not filled
 * more than its size.
 *
 * @param[out] pBuffer The buffer to fill.
 * @param[in] size The maximal size of the buffer.
 * @param[in] kpFmt The format string to use.
 * @param[in] args Additional parameters for the format.
 */
int vsnprintf(char*             pBuffer,
              size_t            size,
              const char*       kpFmt,
              __builtin_va_list args);
#endif /* #ifndef __LIB_STDLIB_H_ */

/************************************ EOF *************************************/
