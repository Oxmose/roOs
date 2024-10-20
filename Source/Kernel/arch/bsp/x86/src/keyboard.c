/*******************************************************************************
 * @file keyboard.c
 *
 * @see keyboard.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 03/07/2024
 *
 * @version 2.0
 *
 * @brief Keyboard driver (PS2/USB) for the kernel.
 *
 * @details Keyboard driver (PS2/USB) for the kernel. Enables the user inputs
 * through the keyboard.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <vfs.h>       /* Virtual File System*/
#include <ioctl.h>     /* IOCTL commands */
#include <panic.h>     /* Kernel panic */
#include <kheap.h>     /* Memory allocation */
#include <x86cpu.h>    /* CPU port manipulation */
#include <stdint.h>    /* Generic int types */
#include <string.h>    /* String manipualtion */
#include <kerror.h>    /* Kernel error */
#include <devtree.h>   /* Device tree */
#include <critical.h>  /* Kernel locks */
#include <drivermgr.h> /* Driver manager service */
#include <semaphore.h> /* Kernel semaphores */

/* Configuration files */
#include <config.h>

/* Header file */
#include <keyboard.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "X86 Keyboard"

/** @brief FDT property for comm ports */
#define KBD_FDT_COMM_PROP "comm"
/** @brief FDT property for interrupt  */
#define KBD_FDT_INT_PROP "interrupts"
/** @brief FDT property for device path */
#define KBD_FDT_DEVICE_PROP "device"

/** @brief Cast a pointer to a keyboard driver controler */
#define GET_CONTROLER(PTR) ((kbd_controler_t*)PTR)

/** @brief Defines the maximal size of the keyboard input buffer */
#define KBD_INPUT_BUFFER_SIZE 128

/** @brief Defines the read available status on the keyboard */
#define KBD_INT_STATUS_DATA_AVAILABLE 0x01

/** @brief Keyboard specific key code: backspace. */
#define KEY_BACKSPACE '\b'
/** @brief Keyboard specific key code: tab. */
#define KEY_TAB '\t'
/** @brief Keyboard specific key code: return. */
#define KEY_RETURN '\n'
/** @brief Keyboard specific key code: left shift. */
#define KEY_LSHIFT 0x0400
/** @brief Keyboard specific key code: right shift. */
#define KEY_RSHIFT 0x0500

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Keyboard code to key mapping. */
typedef struct
{
    /** @brief Regular mapping. */
    uint16_t regular[128];
    /** @brief Maj mapping. */
    uint16_t shifted[128];
} key_mapper_t;


/** @brief x86 Keyboard driver controler. */
typedef struct
{
    /** @brief CPU command port. */
    uint16_t cpuCommPort;

    /** @brief CPU data port. */
    uint16_t cpuDataPort;

    /** @brief The keyboard IRQ number */
    uint32_t irqNumber;

    /** @brief Current start keyboard input buffer cursor */
    size_t inputBufferStartCursor;

    /** @brief Current end keyboard input buffer cursor */
    size_t inputBufferEndCursor;

    /** @brief Input buffer */
    uint8_t* pInputBuffer;

    /** @brief Input buffer lock */
    kernel_spinlock_t inputBufferLock;

    /** @brief Input buffer semaphore */
    semaphore_t inputBufferSem;

    /** @brief Keyboard state flags */
    uint32_t flags;

    /** @brief Stores the VFS driver */
    vfs_driver_t vfsDriver;

    /** @brief Driver's lock */
    kernel_spinlock_t lock;
} kbd_controler_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the keyboard to ensure correctness of execution.
 *
 * @details Assert macro used by the keyboard to ensure correctness of
 * execution. Due to the critical nature of the keyboard, any error generates a
 * kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define KBD_ASSERT(COND, MSG, ERROR) {                      \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Attaches the keyboard driver to the system.
 *
 * @details Attaches the keyboard driver to the system. This function will use
 * the FDT to initialize the keyboard hardware and retreive the keyboard
 * parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _kbdAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Handles a keyboard interrupt.
 *
 * @details Handles a keyboard interrupt. Fills the input buffer with the input
 * data and unblock a thread if it is blocked on the input.
 *
 * @param[in] pCurrentThread Unused.
 */
static void _kbdInterruptHandler(kernel_thread_t* pCurrentThread);

/**
 * @brief Reads data from the keyboard input buffer.
 *
 * @details Reads data from the keyboard input buffer. The function returns the
 * number of bytes read. If the buffer is empty, the function is blocking until
 * the buffer is filled with the required number of bytes.
 *
 * @param[in] pDrvCtrl The driver to be used.
 * @param[out] pBuffer The buffer used to receive data.
 * @param[in] kBufferSize The number of bytes to read.
 *
 * @return The function returns the number of bytes read or -1 on error.
 */
static ssize_t _kbdRead(void*        pDrvCtrl,
                        char*        pBuffer,
                        const size_t kBufferSize);

/**
 * @brief Parses a keyboard keycode.
 *
 * @details Parses the keycode given as parameter and execute the corresponding
 * action.
 *
 * @param[in] kKey The keycode to parse.
 *
 * @return When the keycode corresponds to a character, the chacater is
 * returned, otherwise, the NULL character is returned.
 */
static char _manageKeycode(const int8_t kKey);

/**
 * @brief Keyboard VFS open hook.
 *
 * @details Keyboard VFS open hook. This function returns a handle to control the
 * keyboard driver through VFS.
 *
 * @param[in, out] pDrvCtrl The keyboard driver that was registered in the VFS.
 * @param[in] kpPath The path in the keyboard driver mount point.
 * @param[in] flags The open flags, must be O_RDONLY.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _kbdVfsOpen(void*       pDrvCtrl,
                         const char* kpPath,
                         int         flags,
                         int         mode);

/**
 * @brief Keyboard VFS close hook.
 *
 * @details Keyboard VFS close hook. This function closes a handle that was
 * created when calling the open function.
 *
 * @param[in, out] pDrvCtrl The keyboard driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _kbdVfsClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief Keyboard VFS read hook.
 *
 * @details Keyboard VFS read hook. This function reads a string from the
 * keyboard.
 *
 * @param[in, out] pDrvCtrl The keyboard driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[out] pBuffer The buffer that receives the string to read.
 * @param[in] count The number of bytes of the string to read.
 *
 * @return The function returns the number of bytes read or -1 on error;
 */
static ssize_t _kbdVfsRead(void*  pDrvCtrl,
                           void*  pHandle,
                           void*  pBuffer,
                           size_t count);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Keyboard driver instance. */
static driver_t sX86KeyboardDriver = {
    .pName         = "X86 Keyboard Driver",
    .pDescription  = "X86 Keyboard Driver for roOs",
    .pCompatible   = "x86,x86-generic-keyboard",
    .pVersion      = "2.0",
    .pDriverAttach = _kbdAttach
};

/** @brief Stores the keyboard used for input, only one can be used */
static kbd_controler_t* spInputCtrl = NULL;

/** @brief Keyboard map. */
static const key_mapper_t ksQwertyMap = {
    .regular =
    {
        0,
        0,   // ESCAPE
        '1',
        '2',
        '3',
        '4',
        '5',
        '6',
        '7',
        '8',
        '9',                      // 10
        '0',
        '-',
        '=',
        KEY_BACKSPACE,   // BACKSPACE
        KEY_TAB,   // TAB
        'q',
        'w',
        'e',
        'r',
        't',                    // 20
        'y',
        'u',
        'i',
        'o',
        'p',
        0,   // MOD ^
        0,   // MOD ¸
        KEY_RETURN,   // ENTER
        0,   // VER MAJ
        'a',                    // 30
        's',
        'd',
        'f',
        'g',
        'h',
        'j',
        'k',
        'l',
        ';',
        0,   // MOD `           // 40
        0,
        KEY_LSHIFT,   // LEFT SHIFT
        '<',
        'z',
        'x',
        'c',
        'v',
        'b',
        'n',
        'm',                    // 50
        ',',
        '.',
        0, // é
        KEY_RSHIFT,    // RIGHT SHIFT
        0,
        0,     // ALT left / right
        ' ',
        0,
        0,     // F1
        0,     // F2               // 60
        0,     // F3
        0,     // F4
        0,     // F5
        0,     // F6
        0,     // F7
        0,     // F8
        0,     // F9
        0,     // SCROLL LOCK
        0,     // PAUSE
        0,                     // 70
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,                  // 80
        0,
        0
    },

    .shifted =
    {
        0,
        0,   // ESCAPE
        '!',
        '"',
        '/',
        '$',
        '%',
        '?',
        '&',
        '*',
        '(',                      // 10
        ')',
        '_',
        '+',
        KEY_BACKSPACE,   // BACKSPACE
        KEY_TAB,         // TAB
        'Q',
        'W',
        'E',
        'R',
        'T',                    // 20
        'Y',
        'U',
        'I',
        'O',
        'P',
        0,   // MOD ^
        0,   // MOD ¨
        KEY_RETURN,   // ENTER
        0,   // VER MAJ
        'A',                    // 30
        'S',
        'D',
        'F',
        'G',
        'H',
        'J',
        'K',
        'L',
        ':',
        0,   // MOD `           // 40
        0,
        KEY_LSHIFT,   // LEFT SHIFT
        '>',
        'Z',
        'X',
        'C',
        'V',
        'B',
        'N',
        'M',                    // 50
        '\'',
        '.',
        0, // É
        KEY_RSHIFT,    // RIGHT SHIFT
        0,
        0,     // ALT left / right
        ' ',
        0,
        0,     // F1
        0,     // F2               // 60
        0,     // F3
        0,     // F4
        0,     // F5
        0,     // F6
        0,     // F7
        0,     // F8
        0,     // F9
        0,     // SCROLL LOCK
        0,     // PAUSE
        0,                     // 70
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,                  // 80
        0,
        0
    }
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static OS_RETURN_E _kbdAttach(const fdt_node_t* pkFdtNode)
{
    const uint32_t*  kpUintProp;
    const char*      kpStrProp;
    size_t           propLen;
    OS_RETURN_E      retCode;
    OS_RETURN_E      error;
    kbd_controler_t* pDrvCtrl;
    bool_t           isSemInit;
    bool_t           isInputBufferSet;

    isSemInit        = FALSE;
    isInputBufferSet = FALSE;

    /* Init structures */
    pDrvCtrl = kmalloc(sizeof(kbd_controler_t));
    if(pDrvCtrl == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pDrvCtrl, 0, sizeof(kbd_controler_t));
    pDrvCtrl->vfsDriver = VFS_DRIVER_INVALID;
    KERNEL_SPINLOCK_INIT(pDrvCtrl->lock);

    /* Get the keyboard CPU communication ports */
    kpUintProp = fdtGetProp(pkFdtNode, KBD_FDT_COMM_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->cpuCommPort = (uint16_t)FDTTOCPU32(*kpUintProp);
    pDrvCtrl->cpuDataPort = (uint16_t)FDTTOCPU32(*(kpUintProp + 1));

    /* Get IRQ lines */
    kpUintProp = fdtGetProp(pkFdtNode, KBD_FDT_INT_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Check that we are the only input port */
    if(spInputCtrl != NULL)
    {
        retCode = OS_ERR_ALREADY_EXIST;
        goto ATTACH_END;
    }

    /* Init buffer */
    pDrvCtrl->inputBufferStartCursor = 0;
    pDrvCtrl->inputBufferEndCursor   = 0;
    pDrvCtrl->pInputBuffer = kmalloc(KBD_INPUT_BUFFER_SIZE);
    if(pDrvCtrl->pInputBuffer == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    isInputBufferSet = TRUE;
    KERNEL_SPINLOCK_INIT(pDrvCtrl->inputBufferLock);

    retCode = semInit(&pDrvCtrl->inputBufferSem,
                      0,
                      SEMAPHORE_FLAG_QUEUING_PRIO | SEMAPHORE_FLAG_BINARY);
    if(retCode != OS_NO_ERR)
    {
        goto ATTACH_END;
    }
    isSemInit = TRUE;

    pDrvCtrl->irqNumber = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));

    /* Register the interrupt */
    retCode = interruptIRQRegister(pDrvCtrl->irqNumber, _kbdInterruptHandler);

    /* If we are already registered, error */
    if(retCode != OS_NO_ERR)
    {
        goto ATTACH_END;
    }

    /* Set typematic settings */
    while((_cpuInB(pDrvCtrl->cpuCommPort) & 2) != 0){}
    _cpuOutB(0xF3, pDrvCtrl->cpuDataPort);
    while((_cpuInB(pDrvCtrl->cpuCommPort) & 2) != 0){}
    _cpuOutB(0x20, pDrvCtrl->cpuDataPort);
    while((_cpuInB(pDrvCtrl->cpuCommPort) & 2) != 0){}
    _cpuInB(pDrvCtrl->cpuDataPort);

    /* Set the interrupt mask */
    interruptIRQSetMask(pDrvCtrl->irqNumber, TRUE);
    interruptIRQSetEOI(pDrvCtrl->irqNumber);

    /* Set the input driver */
    spInputCtrl = pDrvCtrl;

    /* Get the device path */
    kpStrProp = fdtGetProp(pkFdtNode, KBD_FDT_DEVICE_PROP, &propLen);
    if(kpStrProp == NULL || propLen  == 0)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Register the driver */
    pDrvCtrl->vfsDriver = vfsRegisterDriver(kpStrProp,
                                            pDrvCtrl,
                                            _kbdVfsOpen,
                                            _kbdVfsClose,
                                            _kbdVfsRead,
                                            NULL,
                                            NULL,
                                            NULL);
    if(pDrvCtrl->vfsDriver == VFS_DRIVER_INVALID)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

ATTACH_END:

    if(retCode != OS_NO_ERR)
    {
        if(isSemInit == TRUE)
        {
            semDestroy(&pDrvCtrl->inputBufferSem);
        }
        if(isInputBufferSet == TRUE)
        {
            kfree(pDrvCtrl->pInputBuffer);
        }
        if(pDrvCtrl != NULL)
        {
            if(pDrvCtrl->vfsDriver != VFS_DRIVER_INVALID)
            {
                error = vfsUnregisterDriver(&pDrvCtrl->vfsDriver);
                if(error != OS_NO_ERR)
                {
                    PANIC(error,
                          MODULE_NAME,
                          "Failed to unregister VFS driver");
                }
            }

            kfree(pDrvCtrl);
        }
    }

    return retCode;
}

static void _kbdInterruptHandler(kernel_thread_t* pCurrentThread)
{
    uint8_t     data;
    OS_RETURN_E error;
    size_t      availableSpace;

    (void)pCurrentThread;

    if(spInputCtrl == NULL)
    {
        return;
    }

    /* Check is we received a data */
    data = _manageKeycode(_cpuInB(spInputCtrl->cpuDataPort));

    /* Set EOI */
    interruptIRQSetEOI(spInputCtrl->irqNumber);

    if(data != 0)
    {
        /* Try to add the new data to the buffer */
        KERNEL_LOCK(spInputCtrl->inputBufferLock);

        if(spInputCtrl->inputBufferEndCursor >=
           spInputCtrl->inputBufferStartCursor)
        {
            availableSpace = KBD_INPUT_BUFFER_SIZE -
                             spInputCtrl->inputBufferEndCursor +
                             spInputCtrl->inputBufferStartCursor;
        }
        else
        {
            availableSpace = spInputCtrl->inputBufferStartCursor -
                             spInputCtrl->inputBufferEndCursor;
        }

        if(availableSpace > 0)
        {
            /* Read the data */
            spInputCtrl->pInputBuffer[spInputCtrl->inputBufferEndCursor] = data;

            spInputCtrl->inputBufferEndCursor =
                (spInputCtrl->inputBufferEndCursor + 1) %
                KBD_INPUT_BUFFER_SIZE;
        }

        KERNEL_UNLOCK(spInputCtrl->inputBufferLock);

        /* Post the semaphore */
        error = semPost(&spInputCtrl->inputBufferSem);
        KBD_ASSERT(error == OS_NO_ERR,
                   "Failed to post keyboard semaphore",
                   error);

    }
}

static ssize_t _kbdRead(void*        pDrvCtrl,
                        char*        pBuffer,
                        const size_t kBufferSize)
{
    OS_RETURN_E error;
    size_t      toRead;
    size_t      bytesToRead;
    size_t      usedSpace;
    size_t      i;

    if(pDrvCtrl != spInputCtrl || spInputCtrl == NULL || pBuffer == NULL)
    {
        return -1;
    }

    toRead = kBufferSize;
    while(toRead != 0)
    {
        /* Copy if we can */
        error = semWait(&spInputCtrl->inputBufferSem);
        KBD_ASSERT(error == OS_NO_ERR,
                   "Failed to wait keyboard semaphore",
                   error);

        KERNEL_LOCK(spInputCtrl->inputBufferLock);

        if(spInputCtrl->inputBufferEndCursor >=
           spInputCtrl->inputBufferStartCursor)
        {
            usedSpace = spInputCtrl->inputBufferEndCursor -
                        spInputCtrl->inputBufferStartCursor;
        }
        else
        {
            usedSpace = KBD_INPUT_BUFFER_SIZE -
                        spInputCtrl->inputBufferStartCursor +
                        spInputCtrl->inputBufferEndCursor;
        }

        /* Get what we can read */
        bytesToRead = MIN(toRead, usedSpace);

        /* Copy */
        for(i = 0; i < bytesToRead; ++i)
        {
            *pBuffer =
                spInputCtrl->pInputBuffer[spInputCtrl->inputBufferStartCursor];
            ++pBuffer;
            spInputCtrl->inputBufferStartCursor =
                (spInputCtrl->inputBufferStartCursor + 1) %
                KBD_INPUT_BUFFER_SIZE;
        }

        toRead -= bytesToRead;
        usedSpace -= bytesToRead;

        KERNEL_UNLOCK(spInputCtrl->inputBufferLock);

        /* If we can still read data, post the semaphore, we read what we had to
         * since we used
         * bytesToRead = MIN(toRead, spInputCtrl->inputBufferCursor)
         */
        if(usedSpace > 0)
        {
            error = semPost(&spInputCtrl->inputBufferSem);
            KBD_ASSERT(error == OS_NO_ERR,
                       "Failed to post keyboard semaphore",
                       error);
        }

    }

    return kBufferSize;
}

static char _manageKeycode(const int8_t kKey)
{
    char   retChar;
    bool_t mod;
    bool_t shifted;

    retChar = 0;

    /* Manage push of release */
    if(kKey >= 0)
    {
        mod = FALSE;

        /* Manage modifiers */
        switch(ksQwertyMap.regular[kKey])
        {
            case KEY_LSHIFT:
                spInputCtrl->flags |= KEY_LSHIFT;
                mod = TRUE;
                break;
            case KEY_RSHIFT:
                spInputCtrl->flags |= KEY_RSHIFT;
                mod = TRUE;
                break;
            default:
                break;
        }

        /* Manage only set characters */
        if(mod == FALSE &&
           (ksQwertyMap.regular[kKey] != 0 ||
            ksQwertyMap.shifted[kKey] != 0))
        {


            shifted = (spInputCtrl->flags & KEY_LSHIFT) != 0 ||
                      (spInputCtrl->flags & KEY_RSHIFT) != 0;
            retChar = (shifted > 0) ?
                        ksQwertyMap.shifted[kKey] :
                        ksQwertyMap.regular[kKey];
        }
    }
    else
    {
        /* Manage modifiers */
        switch(ksQwertyMap.shifted[kKey + 128])
        {
            case KEY_LSHIFT:
                spInputCtrl->flags &= ~KEY_LSHIFT;
                break;
            case KEY_RSHIFT:
                spInputCtrl->flags &= ~KEY_RSHIFT;
                break;
            default:
                break;
        }
    }

    return retChar;
}

static void* _kbdVfsOpen(void*       pDrvCtrl,
                         const char* kpPath,
                         int         flags,
                         int         mode)
{
    (void)pDrvCtrl;
    (void)mode;

    /* The path must be empty */
    if((*kpPath == '/' && *(kpPath + 1) != 0) || *kpPath != 0)
    {
        return (void*)-1;
    }

    /* The flags must be O_RDONLY */
    if(flags != O_RDONLY)
    {
        return (void*)-1;
    }

    /* We don't need a handle, return NULL */
    return NULL;
}

static int32_t _kbdVfsClose(void* pDrvCtrl, void* pHandle)
{
    (void)pDrvCtrl;

    if(pHandle == (void*)-1)
    {
        return -1;
    }

    /* Nothing to do */
    return 0;
}

static ssize_t _kbdVfsRead(void*  pDrvCtrl,
                           void*  pHandle,
                           void*  pBuffer,
                           size_t count)
{
    if(pHandle == (void*)-1)
    {
        return -1;
    }

    return _kbdRead(pDrvCtrl, pBuffer, count);
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86KeyboardDriver);

/************************************ EOF *************************************/