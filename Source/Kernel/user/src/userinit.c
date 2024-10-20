/*******************************************************************************
 * @file userinit.c
 *
 * @see userinit.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 16/06/2024
 *
 * @version 1.0
 *
 * @brief Kernel's user entry point.
 *
 * @details Kernel's user entry point. This file gather the functions called
 * by the kernel just before starting the scheduler and executing the test.
 * Users can use this function to add relevant code to their applications'
 * initialization or for other purposes.
 *
 * @warning All interrupts are disabled when calling the user initialization
 * functions.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <vfs.h>         /* VFS service */
#include <panic.h>       /* Kernel panic */
#include <syslog.h>      /* Kernel syslog */
#include <string.h>      /* String manipulation */
#include <scheduler.h>   /* Kernel scheduler */
#include <elfmanager.h>  /* ELF manager */
#include <kernelshell.h> /* User kernel's shell */

/* Configuration files */
#include <config.h>

/* Header file */
#include <userinit.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the current module name */
#define MODULE_NAME "USERINIT"

/** @brief Defines the init ram disk device path */
#define INITRD_DEV_PATH "/dev/storage/ramdisk0"

/** @brief Defines the init ram disk mount point */
#define INITRD_MNT_PATH "/initrd"

/** @brief Defines the init process config file path */
#define INIT_CONFIG_PATH "/initrd/.roos_init"

/** @brief Defines the init ELF path configuration variable */
#define CONF_INIT_PATH_VAR_NAME "INIT="
/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Creates the init process.
 *
 * @details Creates the init process. The init process ELF is loaded from the
 * ramdisk and started.
 *
 * @return The function returns the success or error status.
 */
static OS_RETURN_E _createInit(void);

/**
 * @brief Reads a line from the file descriptor.
 *
 * @details Reads a line from the file descriptor. Puts the line in the buffer.
 * The function copies data until the line feed character is encountered in the
 * file of the buffer size is exceeded.
 *
 * @param[in] kFd The file descriptor to use.
 * @param[out] pBuffer The buffer to fill with the line.
 * @param[in] kSize The size of the buffer.
 *
 * @return The function returns the line size.
 */
static ssize_t _readLine(const int32_t kFd, char* pBuffer, const size_t kSize);

/**
 * @brief User shutdown thread routine.
 *
 * @details User shutdown thread routine. This routine will wait for the init
 * thread to return, join it and shutdown the user space.
 *
 * @param[in] args The init thread handle.
 *
 * @return Always return NULL.
 */
static void* _userShutdown(void* args);

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

static ssize_t _readLine(const int32_t kFd, char* pBuffer, const size_t kSize)
{
    size_t  read;
    ssize_t readBytes;

    read = 0;
    while(read < kSize - 1)
    {
        readBytes = vfsRead(kFd, pBuffer + read, 1);
        if(readBytes < 0)
        {
            return -1;
        }
        else if(readBytes == 0)
        {
            break;
        }
        else if(pBuffer[read] == '\n')
        {
            ++read;
            break;
        }

        ++read;
    }

    pBuffer[read] = 0;
    return (ssize_t)read;
}

static OS_RETURN_E _createInit(void)
{
    kernel_thread_t* pInitThread;
    kernel_thread_t* pInitReturnThread;
    int32_t          fileFd;
    char*            pInitPath;
    char             pBuffer[512];
    ssize_t          readBytes;
    uintptr_t        entryPoint;
    OS_RETURN_E      error;

    /* Open the init config */
    fileFd = vfsOpen(INIT_CONFIG_PATH, O_RDONLY, 0);
    if(fileFd < 0)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to open init configuration",
               fileFd);
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Read the configuration */
    pInitPath = NULL;
    while(true)
    {
        readBytes = _readLine(fileFd, pBuffer, 512);
        if(readBytes == 512)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Configuration line is greater than 511 character.",
                   OS_ERR_INCORRECT_VALUE);
            fileFd = vfsClose(fileFd);
            if(fileFd < 0)
            {
                syslog(SYSLOG_LEVEL_ERROR,
                       MODULE_NAME,
                       "Failed to close init configuration",
                       fileFd);
            }
            return OS_ERR_INCORRECT_VALUE;
        }
        else if(readBytes < 0)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failed to read configuration",
                   fileFd);
            fileFd = vfsClose(fileFd);
            if(fileFd < 0)
            {
                syslog(SYSLOG_LEVEL_ERROR,
                       MODULE_NAME,
                       "Failed to close init configuration",
                       fileFd);
            }
            return OS_ERR_INCORRECT_VALUE;
        }
        else if(readBytes == 0)
        {
            break;
        }

        /* Try to get the init configuration */
        if(strncmp(pBuffer,
                   CONF_INIT_PATH_VAR_NAME,
                   strlen(CONF_INIT_PATH_VAR_NAME)) == 0)
        {
            pInitPath = pBuffer + strlen(CONF_INIT_PATH_VAR_NAME);
            break;
        }
    }


    if(pInitPath == NULL)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to get init path from configuration",
               OS_ERR_NO_SUCH_ID);
        fileFd = vfsClose(fileFd);
        if(fileFd < 0)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failed to close init configuration",
                   fileFd);
        }

        return OS_ERR_NO_SUCH_ID;
    }

    syslog(SYSLOG_LEVEL_INFO, MODULE_NAME, "Loading init from %s", pInitPath);

    /* Load the init ELF */
    error = elfManagerLoadElf(pInitPath, &entryPoint);
    if(error == OS_NO_ERR)
    {

        /* Create the init thread */
        error = schedCreateThread(&pInitThread,
                                  false,
                                  KERNEL_HIGHEST_PRIORITY,
                                  "init",
                                  KERNEL_STACK_SIZE,
                                  0,
                                  (void*)entryPoint,
                                  NULL);
        if(error == OS_NO_ERR)
        {
            /* Create the init return thread */
            error = schedCreateThread(&pInitReturnThread,
                                      true,
                                      KERNEL_HIGHEST_PRIORITY,
                                      "shutdown",
                                      KERNEL_STACK_SIZE,
                                      0,
                                      _userShutdown,
                                      pInitThread);
            if(error != OS_NO_ERR)
            {
                PANIC(error,
                      MODULE_NAME,
                      "Failed to create init return thread");
            }
        }
        else
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failed to create init thread",
                   error);
        }
    }
    else
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to load init ELF",
               error);
    }

    fileFd = vfsClose(fileFd);
    if(fileFd < 0)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to close init configuration",
               fileFd);
    }

    return error;
}

static void* _userShutdown(void* args)
{
    kernel_thread_t* pInitThread;
    OS_RETURN_E      error;

    pInitThread = args;

    while(1)
    {
        schedSleep(1000000000);
    }

    error = schedJoinThread(pInitThread, NULL, NULL);
    if(error != OS_NO_ERR)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to join init thread. Error %d",
               error);
    }

    /* TODO: We should shutdown the system here */

    return NULL;
}

void userInit(void)
{
    OS_RETURN_E error;

    /* Mount the init ram disk */
    error = vfsMount(INITRD_MNT_PATH, INITRD_DEV_PATH, NULL);
    if(error != OS_NO_ERR)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to mount init ramdisk, error %d",
               error);
        return;
    }

    /* Create the init process */
    error = _createInit();
    if(error != OS_NO_ERR)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to create the init process, error %d",
               error);
    }

    /* Initialize the kernel shell */
    kernelShellInit();
}

/************************************ EOF *************************************/