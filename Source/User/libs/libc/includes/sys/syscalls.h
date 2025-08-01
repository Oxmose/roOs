/*******************************************************************************
 * @file syscalls.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/10/2024
 *
 * @version 1.0
 *
 * @brief System call utilities for roOs.
 *
 * @details System call utilities for roOs.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __LIB_SYS_SYSCALLS_H_
#define __LIB_SYS_SYSCALLS_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <time.h>      /* Time types */
#include <stddef.h>    /* Standard definitions */
#include <sys/types.h> /* System types */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/
/* None */

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/**
 * @brief The syscall minimal parameters, this is errno. When contructing
 * a system call parameter structure, this attribute must be put at the very
 * begining of the structure.
 */
typedef int32_t syscall_min_params_t;

/**
 * @brief Provides the list of available system call Ids
 *
 * @warning This list must be compatible with the list defined by the kernel.
 */
typedef enum
{
    /** @brief Performs a sleep system call */
    SYSCALL_SLEEP = 0,
    /** @brief Performs a schedule system call */
    SYSCALL_SCHEDULE = 1,
    /** @brief Performs a fork system call */
    SYSCALL_FORK = 2,
    /** @brief Performs a VFS write system call */
    SYSCALL_WRITE = 3,
    /** @brief Performs a clock get time system call */
    SYSCALL_CLOCK_GETTIME = 4,
} SYSCALL_ID_E;

/**
 * @brief Defines the parameters for the sleep system call.
 */
typedef struct
{
    /** @brief Errno value to set */
    syscall_min_params_t errnoVal;
    /** @brief Amount of time in nano seconds to sleep */
    uint64_t sleepTimeNs;
} syscall_sleep_params_t;

/**
 * @brief Defines the parameters for the write system call.
 */
typedef struct
{
    /** @brief Errno value to set */
    syscall_min_params_t errnoVal;
    /** @brief Size in bytes to write */
    size_t toWrite;
    /** @brief Size in bytes written */
    ssize_t written;
    /** @brief The buffer to write */
    const void* pBuffer;
    /** @brief The file descriptor to use */
    int32_t fd;
} syscall_write_params_t;

/**
 * @brief Defines the parameters for the get time system call.
 */
typedef struct
{
    /** @brief Errno value to set */
    syscall_min_params_t errnoVal;
    /** @brief The clock ID to use */
    clockid_t clkId;
    /** @brief The time spec structure to use for the call */
    struct timespec* pTimeSpec;
} syscall_gettime_params_t;

/**
 * @brief Defines the parameters for the scheduler yield call.
 */
typedef struct
{
    /** @brief Errno value to set */
    syscall_min_params_t errnoVal;
} syscall_sched_yield_params_t;

/** @brief Fork system call parameters */
typedef struct
{
    /** @brief Errno value */
    syscall_min_params_t errnoVal;

    /**
     * @brief When the return code is OS_NO_ERROR, this contains the new
     * forked process PID.
     */
    int32_t newPid;
} syscall_fork_param_t;

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
 * @brief See write function for more details.
 *
 * @details See write function for more details.
 *
 * @param[in] fileFd See write function for more details.
 * @param[in] pBuffer See write function for more details.
 * @param[in] count See write function for more details.
 *
 * @return See write function for more details.
 */
ssize_t syscallWrite(int fileFd, const void* pBuffer, size_t count);

/**
 * @brief See sleep function for more details.
 *
 * @details See sleep function for more details.
 *
 * @param[in] seconds See sleep function for more details.
 *
 * @return See sleep function for more details.
 */
unsigned int syscallSleep(unsigned int seconds);

/**
 * @brief See clock_gettime function for more details.
 *
 * @details See clock_gettime function for more details.
 *
 * @param[in] clkId See clock_gettime function for more details.
 * @param[in] pTp See clock_gettime function for more details.
 *
 * @return See clock_gettime function for more details.
 */
int syscallClockGettime(clockid_t clkId, struct timespec* pTp);

/**
 * @brief See nanosleep function for more details.
 *
 * @details See nanosleep function for more details.
 *
 * @param[in] pDuration See nanosleep function for more details.
 * @param[in] pRem See nanosleep function for more details.
 *
 * @return See nanosleep function for more details.
 */
int syscallNanosleep(const struct timespec* pDuration,
                     struct timespec*       pRem);

/**
 * @brief See sched_yield function for more details.
 *
 * @details See sched_yield function for more details.
 *
 * @return See sched_yield function for more details.
 */
int syscallSchedYield(void);

/**
 * @brief See fork function for more details.
 *
 * @details See fork function for more details.
 *
 * @return See fork function for more details.
 */
pid_t syscallFork(void);

#endif /* #ifndef __LIB_SYS_SYSCALLS_H_ */

/************************************ EOF *************************************/