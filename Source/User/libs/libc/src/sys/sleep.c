/*******************************************************************************
 * @file sleep.c
 *
 * @see unistd.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/10/2024
 *
 * @version 1.0
 *
 * @brief Sleep functions familly for the roOs kernel.
 *
 * @details Sleep functions familly for the roOs kernel. Those functions might
 * rely on system calls to perform kernel-space operations.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/


/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <time.h>          /* Time library */
#include <errno.h>         /* Errno management */
#include <stddef.h>        /* Standard definitions */
#include <sys/types.h>     /* Systen types */
#include <sys/syscalls.h>  /* System calls */
#include <userKernelLib.h> /* User and kernel link lib */

/* Header file */
#include <unistd.h>

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

unsigned int sleep(unsigned int seconds)
{
    return syscallSleep(seconds);
}

unsigned int syscallSleep(unsigned int seconds)
{
    syscall_sleep_params_t params;
    struct timespec        startTime;
    struct timespec        endTime;
    uint64_t               totalStartTime;
    uint64_t               totalEndTime;
    int                    retVal;

    if(seconds == 0)
    {
        return 0;
    }

    params.sleepTimeNs = seconds * 1000000000;

    retVal = clock_gettime(CLOCK_MONOTONIC, &startTime);
    if(retVal != 0)
    {
        return seconds;
    }

    params.errnoVal = 0;

    syscallPerform(SYSCALL_SLEEP, &params);

    if(params.errnoVal != 0)
    {
        return params.sleepTimeNs;
    }

    retVal = clock_gettime(CLOCK_MONOTONIC, &endTime);
    if(retVal != 0)
    {
        return seconds;
    }

    totalStartTime = startTime.tv_sec * 1000000000 + startTime.tv_nsec;
    totalEndTime = endTime.tv_sec * 1000000000 + endTime.tv_nsec;

    if(totalStartTime + seconds * 1000000000 < totalEndTime)
    {
        return totalEndTime - totalStartTime - seconds * 1000000000;
    }
    else
    {
        return 0;
    }
}

/************************************ EOF *************************************/