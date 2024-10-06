/*******************************************************************************
 * @file syslog.h
 *
 * @see syslog.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 16/07/2024
 *
 * @version 1.0
 *
 * @brief System log services.
 *
 * @details System log services. This module provides the access to the syslog
 * services.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __IO_SYSLOG_H_
#define __IO_SYSLOG_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h> /* Generic int types */
#include <stddef.h> /* Standard definition */
#include <kerror.h> /* Kernel error codes */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines the system log criticality levels */
typedef enum
{
    /** @brief Syslog level: critical */
    SYSLOG_LEVEL_CRITICAL = 0,
    /** @brief Syslog level: error */
    SYSLOG_LEVEL_ERROR = 1,
    /** @brief Syslog level: warning */
    SYSLOG_LEVEL_WARNING = 2,
    /** @brief Syslog level: info */
    SYSLOG_LEVEL_INFO = 3,
    /** @brief Syslog level: debug */
    SYSLOG_LEVEL_DEBUG = 4,
    /** @brief Defines the maximal syslog level */
    SYSLOG_LEVEL_MAX
} SYSLOG_LEVEL_E;

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
 * @brief Initializes the syslog.
 *
 * @details Initializes the syslog. Creates syslog queue and related structures.
 */
void syslogInit(void);

/**
 * @brief Starts the syslog service.
 *
 * @details Starts the syslog service. Creates the thread that will save or
 * display the logs.
 */
void syslogStart(void);

/**
 * @brief Logs a message to the system logs.
 *
 * @details Logs a message to the system logs. The message will be copied and
 * saved to the syslog buffer and output when needed.
 *
 * @param[in] kLevel The log criticality level.
 * @param[in] kpModule The log module name.
 * @param[in] kpMessage The formated message.
 * @param[in] ... Additional arguments for the formated message
 *
 * @return The success or error state is returned.
 */
OS_RETURN_E syslog(const SYSLOG_LEVEL_E kLevel,
                   const char*          kpModule,
                   const char*          kpMessage,
                   ...);

#endif /* #ifndef __IO_SYSLOG_H_ */

/************************************ EOF *************************************/