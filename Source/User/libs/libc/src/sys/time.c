/*******************************************************************************
 * @file sleep.c
 *
 * @see time.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 27/10/2024
 *
 * @version 1.0
 *
 * @brief Time functions familly for the roOs kernel.
 *
 * @details Time functions familly for the roOs kernel. Those functions might
 * rely on system calls to perform kernel-space operations.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/


/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <errno.h>         /* Errno management */
#include <stddef.h>        /* Standard definitions */
#include <sys/types.h>     /* System types  */
#include <sys/syscalls.h>  /* System calls */
#include <userKernelLib.h> /* User and kernel link lib */

/* Header file */
#include <time.h>

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
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    return syscallClockGettime(clk_id, tp);
}

int nanosleep(const struct timespec *duration,
              struct timespec* rem)
{
    return syscallNanosleep(duration, rem);
}

int syscallClockGettime(clockid_t clkId, struct timespec* pTp)
{
    syscall_gettime_params_t params;
    params.clkId = clkId;
    params.pTimeSpec = pTp;
    params.errnoVal = 0;

    syscallPerform(SYSCALL_CLOCK_GETTIME, &params);

    if(params.errnoVal != 0)
    {
        errno = params.errnoVal;
        return -1;
    }

    return 0;
}

int syscallNanosleep(const struct timespec* pDuration,
                     struct timespec*       pRem)
{
    syscall_sleep_params_t params;
    struct timespec        startTime;
    struct timespec        endTime;
    uint64_t               totalStartTime;
    uint64_t               totalEndTime;
    uint64_t               sleepTimeNs;
    int                    retVal;

    if(pDuration == NULL)
    {
        errno = EFAULT;
        return -1;
    }
    if(pDuration->tv_nsec < 0 ||
       pDuration->tv_nsec > 999999999)
    {
        errno = EINVAL;
        return -1;
    }

    sleepTimeNs = pDuration->tv_sec * 1000000000 + pDuration->tv_nsec;
    params.sleepTimeNs = sleepTimeNs;

    retVal = clock_gettime(CLOCK_MONOTONIC, &startTime);
    if(retVal != 0)
    {
        if(pRem != NULL)
        {
            pRem->tv_nsec = pDuration->tv_nsec;
            pRem->tv_sec = pDuration->tv_sec;
        }
        errno = EFAULT;
        return -1;
    }

    params.errnoVal = 0;

    syscallPerform(SYSCALL_SLEEP, &params);

    if(params.errnoVal != 0)
    {
        if(pRem != NULL)
        {
            pRem->tv_nsec = pDuration->tv_nsec;
            pRem->tv_sec = pDuration->tv_sec;
        }
        errno = params.errnoVal;
        return -1;
    }

    retVal = clock_gettime(CLOCK_MONOTONIC, &endTime);
    if(retVal != 0)
    {
        if(pRem != NULL)
        {
            pRem->tv_nsec = pDuration->tv_nsec;
            pRem->tv_sec = pDuration->tv_sec;
        }
        errno = EFAULT;
        return -1;
    }

    totalStartTime = startTime.tv_sec * 1000000000 + startTime.tv_nsec;
    totalEndTime = endTime.tv_sec * 1000000000 + endTime.tv_nsec;

    if(totalStartTime + sleepTimeNs < totalEndTime)
    {
        if(pRem != NULL)
        {
            pRem->tv_nsec = pDuration->tv_nsec -
                            (endTime.tv_nsec - startTime.tv_nsec);
            pRem->tv_sec = pDuration->tv_sec -
                           (endTime.tv_sec - startTime.tv_sec);
        }
        errno = EINTR;
        return -1;
    }
    else
    {
        return 0;
    }
}


/************************************ EOF *************************************/