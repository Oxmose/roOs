/*******************************************************************************
 * @file write.c
 *
 * @see unistd.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/10/2024
 *
 * @version 1.0
 *
 * @brief Write functions familly for the roOs kernel.
 *
 * @details Write functions familly for the roOs kernel. Those functions might
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

ssize_t write(int fd, const void* buf, size_t count)
{
    return syscallWrite(fd, buf, count);
}

ssize_t syscallWrite(int fileFd, const void* pBuffer, size_t count)
{
    syscall_write_params_t params;

    params.written = -2;
    params.pBuffer = pBuffer;
    params.fd = fileFd;
    params.toWrite = count;
    params.errnoVal = 0;

    syscallPerform(SYSCALL_WRITE, &params);

    if(params.written == -2)
    {
        errno = EINTR;
        return -1;
    }

    if(params.written < 0)
    {
        return params.written;
    }
    else
    {
        errno = params.errnoVal;
        return params.written;
    }
}

/************************************ EOF *************************************/