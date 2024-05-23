/*******************************************************************************
 * @file kerneloutput.h
 *
 * @see kerneloutput.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 2.1
 *
 * @brief Kernel's output methods.
 *
 * @details Simple output functions to print messages to screen. These are
 * really basic output too allow early kernel boot output and debug. These
 * functions can be used in interrupts handlers since no lock is required to use
 * them. This also makes them non thread safe.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __IO_KERNEL_OUTPUT_H_
#define __IO_KERNEL_OUTPUT_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* None */

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

/* Defines the output macros that can be enabled or disabled at compile time */
#if KERNEL_LOG_LEVEL >= INFO_LOG_LEVEL
#define KERNEL_INFO(...)    kernel_info(__VA_ARGS__)
#define KERNEL_SUCCESS(...) kernel_success(__VA_ARGS__)
#else
#define KERNEL_INFO(...)
#define KERNEL_SUCCESS(...)
#endif

#if KERNEL_LOG_LEVEL >= ERROR_LOG_LEVEL
#define KERNEL_ERROR(...) kernel_error(__VA_ARGS__)
#else
#define KERNEL_ERROR(...)
#endif

#if KERNEL_LOG_LEVEL >= DEBUG_LOG_LEVEL
#define KERNEL_DEBUG(ENABLED, MODULE, STR, ...)                         \
do {                                                                    \
    if(ENABLED)                                                         \
    {                                                                   \
        kernel_debug(" " MODULE " | " STR " | " __FILE__ ":%d\n",       \
                     ##__VA_ARGS__, __LINE__);                          \
    }                                                                   \
} while(0);
#else
#define KERNEL_DEBUG(...)
#endif

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
 * @brief Prints a formated string to the screen.
 *
 * @details Prints the desired string to the screen. This uses the generic
 * graphic driver to output data.
 *
 * @param[in] fmt The format string to output.
 * @param[in] ... format's parameters.
 */
void kernel_printf(const char *fmt, ...);

/**
 * @brief Prints the desired string to the screen.
 *
 * @details Prints the desired string to the screen. Adds a red [ERROR] tag at
 * the beggining of the string before printing it. This uses the generic
 * graphic driver to output data.
 *
 * @param[in] fmt The format string to output.
 * @param[in] ... format's parameters.
 */
void kernel_error(const char *fmt, ...);

/**
 * @brief Prints the desired string to the screen.
 *
 * @details Prints the desired string to the screen. Adds a green [OK] tag at
 * the beggining of the string before printing it. This uses the generic
 * graphic driver to output data.
 *
 * @param[in] fmt The format string to output.
 * @param[in] ... format's parameters.
 */
void kernel_success(const char *fmt, ...);

/**
 * @brief Prints the desired string to the screen.
 *
 * @details Prints the desired string to the screen. Adds a cyan [INFO] tag at
 * the beggining of the string before printing it. This uses the generic
 * graphic driver to output data.
 *
 * @param[in] fmt The format string to output.
 * @param[in] ... format's parameters.
 */
void kernel_info(const char *fmt, ...);

/**
 * @brief Prints the desired string to the screen.
 *
 * @details Prints the desired string to the screen. Adds a yellow [DEBUG] tag
 * at the beggining of the string before printing it. This uses the generic
 * graphic driver to output data.
 *
 * @param[in] fmt The format string to output.
 * @param[in] ... format's parameters.
 */
void kernel_debug(const char *fmt, ...);

/**
 * @brief Prints the desired string to the screen.
 *
 * @details Prints the desired string to the screen. Adds a yello [WARNING] tag
 * at the beggining of the string before printing it. This uses the generic
 * graphic driver to output data.
 *
 * @param[in] fmt The format string to output.
 * @param[in] ... format's parameters.
 */
void kernel_warning(const char *fmt, ...);

/**
 * @brief Prints a string to the screen attached to the arguments list.
 *
 * @details Prints the desired string to the screen with the argument list given
 * as parameter, this is conveniently be used by printf.
 *
 * @warning This function should only be called when the kernel is fully
 * initialized.
 *
 * @param[in] str The format string to output.
 * @param[in] args The arguments list.
 */
void kernel_doprint(const char* str, __builtin_va_list args);

#endif /* #ifndef __IO_KERNEL_OUTPUT_H_ */

/************************************ EOF *************************************/
