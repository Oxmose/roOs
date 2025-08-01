/*******************************************************************************
 * @file sched.c
 *
 * @see sched.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/10/2024
 *
 * @version 1.0
 *
 * @brief Scheduler functions familly for the roOs kernel.
 *
 * @details Scheduler functions familly for the roOs kernel. Those functions
 * might rely on system calls to perform kernel-space operations.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/


/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <errno.h>         /* Errno management */
#include <stddef.h>        /* Standard definitions */
#include <sys/syscalls.h>  /* System calls */
#include <userKernelLib.h> /* User and kernel link lib */

/* Header file */
#include <sched.h>
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

int sched_yield(void)
{
    return syscallSchedYield();
}

pid_t fork(void)
{
    return syscallFork();
}

int syscallSchedYield(void)
{
    syscall_sched_yield_params_t params;
    params.errnoVal = 0;

    syscallPerform(SYSCALL_SCHEDULE, &params);

    if(params.errnoVal != 0)
    {
        errno = params.errnoVal;
        return -1;
    }

    return 0;
}

pid_t syscallFork(void)
{
    syscall_fork_param_t params;
    params.errnoVal = 0;
    params.newPid   = 0;

    syscallPerform(SYSCALL_FORK, &params);

    if(params.newPid < 0)
    {
        errno = params.errnoVal;
        return -1;
    }

    return params.newPid;
}


/************************************ EOF *************************************/