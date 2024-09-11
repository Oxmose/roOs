/*******************************************************************************
 * @file vgatext.c
 *
 * @see vgatext.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 23/05/2024
 *
 * @version 2.0
 *
 * @brief VGA text mode driver.
 *
 * @details Allows the kernel to display text and general ASCII characters to be
 * displayed on the screen. Includes cursor management, screen colors management
 * and other fancy screen driver things.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <vfs.h>           /* Virtual File System*/
#include <ioctl.h>         /* IOCTL commands */
#include <panic.h>         /* Kernel panic */
#include <kheap.h>         /* Kernel heap */
#include <x86cpu.h>        /* CPU port manipulation */
#include <stdint.h>        /* Generic int types */
#include <string.h>        /* String manipualtion */
#include <kerror.h>        /* Kernel error */
#include <memory.h>        /* Memory manager */
#include <syslog.h>        /* Kernel Syslog */
#include <devtree.h>       /* Device tree */
#include <console.h>       /* Console driver manager */
#include <critical.h>      /* Kernel locks */
#include <drivermgr.h>     /* Driver manager service */

/* Configuration files */
#include <config.h>

/* Header file */
#include <vgatext.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Module's name */
#define MODULE_NAME "X86_VGA_TEXT"

/** @brief VGA cursor position command low. */
#define VGA_CONSOLE_CURSOR_COMM_LOW  0x0F
/** @brief VGA cursor position command high. */
#define VGA_CONSOLE_CURSOR_COMM_HIGH 0x0E

/** @brief FDT property for registers */
#define VGA_FDT_REG_PROP    "reg"
/** @brief FDT property for comm ports */
#define VGA_FDT_COMM_PROP   "comm"
/** @brief FDT property for resolution */
#define VGA_FDT_RES_PROP    "resolution"
/** @brief FDT property for device path */
#define VGA_FDT_DEVICE_PROP "device"

/** @brief Cast a pointer to a VGA driver controler */
#define GET_CONTROLER(PTR) ((vga_controler_t*)PTR)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 VGA driver controler. */
typedef struct
{
    /** @brief Screen line resolution. */
    uint8_t lineCount;

    /** @brief Screen column resolution. */
    uint8_t columnCount;

    /** @brief CPU command port. */
    uint16_t cpuCommPort;

    /** @brief CPU data port. */
    uint16_t cpuDataPort;

    /** @brief Stores the curent screen's color scheme. */
    colorscheme_t screenScheme;

    /** @brief Stores the curent screen's cursor settings. */
    cursor_t screenCursor;

    /** @brief VGA frame buffer address. */
    uint16_t* pFramebuffer;

    /** @brief Stores the VFS driver */
    vfs_driver_t vfsDriver;

    /** @brief Size in bytes of the framebuffer. */
    size_t framebufferSize;
} vga_controler_t;


/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the VGA to ensure correctness of execution.
 *
 * @details Assert macro used by the VGA to ensure correctness of execution.
 * Due to the critical nature of the VGA, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define VGA_ASSERT(COND, MSG, ERROR) {                      \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/**
 * @brief Get the VGA frame buffer virtual address.
 *
 * @details Get the VGA frame buffer virtual address correponding to a
 * certain region of the buffer given the parameters.
 *
 * @param[in] LINE The frame buffer line.
 * @param[in] COLUMN The frame buffer column.
 *
 * @return The frame buffer virtual address is get correponding to a
 * certain region of the buffer given the parameters.
 */
#define GET_FRAME_BUFFER_AT(CTRL, LINE, COL)                           \
        ((CTRL->pFramebuffer) + ((COL) + (LINE) * CTRL->columnCount))

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Attaches the VGA driver to the system.
 *
 * @details Attaches the VGA driver to the system. This function will use the
 * FDT to initialize the VGA hardware and retreive the VGA parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _vgaConsoleAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Prints a character to the selected coordinates.
 *
 * @details Prints a character to the selected coordinates by setting the memory
 * accordingly.
 *
 * @param[in, out] pDriverCtrl The VGA driver controler to use.
 * @param[in] kLine The line index where to write the character.
 * @param[in] kColumn The colums index where to write the character.
 * @param[in] kCharacter The character to display on the screem.
 */
static inline void _vgaPrintChar(void*          pDriverCtrl,
                                 const uint32_t kLine,
                                 const uint32_t kColumn,
                                 const char     kCharacter);

/**
 * @brief Processes the character in parameters.
 *
 * @param[in, out] pDriverCtrl The VGA driver controler to use.
 * @details Check the character nature and code. Corresponding to the
 * character's code, an action is taken. A regular character will be printed
 * whereas \\n will create a line feed.
 *
 * @param[in] kCharacter The character to process.
 */
static void _vgaProcessChar(void* pDriverCtrl, const char kCharacter);

/**
 * @brief Clears the screen by printing null character character on black
 * background.
 *
 * @param[in, out] pDriverCtrl The VGA driver controler to use.
 */
static void _vgaClearFramebuffer(void* pDriverCtrl);

/**
 * @brief Saves the cursor attributes in the buffer given as parameter.
 *
 * @details Fills the buffer given as parrameter with the current cursor
 * settings.
 *
 * @param[in, out] pDriverCtrl The VGA driver controler to use.
 * @param[out] pBuffer The cursor buffer in which the current cursor
 * position is going to be saved.
 */
static void _vgaSaveCursor(void* pDriverCtrl, cursor_t* pBuffer);

/**
 * @brief Restores the cursor attributes from the buffer given as parameter.
 *
 * @details The function will restores the cursor attributes from the buffer
 * given as parameter.
 *
 * @param[in, out] pDriverCtrl The VGA driver controler to use.
 * @param[in] kpBuffer The cursor buffer containing the new
 * coordinates of the cursor.
 */
static void _vgaRestoreCursor(void* pDriverCtrl, const cursor_t* kpBuffer);


/**
 * @brief Scrolls in the desired direction of lines_count lines.
 *
 * @details The function will scroll of lines_count line in the desired
 * direction.
 *
 * @param[in, out] pCtrl The VGA driver controler to use.
 * @param[in] kDirection The direction to whoch the console
 * should be scrolled.
 * @param[in] kLines The number of lines to scroll.
 */
static void _vgaScrollSafe(vga_controler_t*         pCtrl,
                           const SCROLL_DIRECTION_E kDirection,
                           const uint32_t           kLines);

/**
 * @brief Scrolls in the desired direction of lines_count lines.
 *
 * @details The function will scroll of lines_count line in the desired
 * direction.
 *
 * @param[in, out] pDriverCtrl The VGA driver controler to use.
 * @param[in] kDirection The direction to whoch the console
 * should be scrolled.
 * @param[in] kLines The number of lines to scroll.
 */
static void _vgaScroll(void*                    pDriverCtrl,
                       const SCROLL_DIRECTION_E kDirection,
                       const uint32_t           kLines);


/**
 * @brief Sets the color scheme of the screen.
 *
 * @details Replaces the curent color scheme used t output data with the new
 * one given as parameter.
 *
 * @param[in, out] pDriverCtrl The VGA driver controler to use.
 * @param[in] kpColorScheme The new color scheme to apply to
 * the screen console.
 */
static void _vgaSetScheme(void*                pDriverCtrl,
                          const colorscheme_t* kpColorScheme);


/**
 * @brief Saves the color scheme in the buffer given as parameter.
 *
 * @details Fills the buffer given as parameter with the current screen's
 * color scheme value.
 *
 * @param[in, out] pDriverCtrl The VGA driver controler to use.
 * @param[out] pBuffer The buffer that will receive the current
 * color scheme used by the screen console.
 */
static void _vgaSaveScheme(void* pDriverCtrl, colorscheme_t* pBuffer);

/**
 * @brief Places the cursor to the selected coordinates given as parameters.
 *
 * @details Places the screen cursor to the coordinated described with the
 * parameters. The function will check the boundaries or the position parameters
 * before setting the cursor position.
 *
 * @param[in, out] pDriverCtrl The VGA driver controler to use.
 * @param[in] kLine The line index where to place the cursor.
 * @param[in] column The column index where to place the cursor.
 */
static inline void _vgaPutCursorSafe(vga_controler_t* pCtrl,
                                     const uint32_t   kLine,
                                     const uint32_t   kColumn);

/**
 * @brief VGA VFS open hook.
 *
 * @details VGA VFS open hook. This function returns a handle to control the
 * VGA driver through VFS.
 *
 * @param[in, out] pDrvCtrl The VGA driver that was registered in the VFS.
 * @param[in] kpPath The path in the VGA driver mount point.
 * @param[in] flags The open flags, must be O_RDWR.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _vgaVfsOpen(void*       pDrvCtrl,
                         const char* kpPath,
                         int         flags,
                         int         mode);

/**
 * @brief VGA VFS close hook.
 *
 * @details VGA VFS close hook. This function closes a handle that was created
 * when calling the open function.
 *
 * @param[in, out] pDrvCtrl The VGA driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _vgaVfsClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief VGA VFS write hook.
 *
 * @details VGA VFS write hook. This function writes a string to the VGA
 * framebuffer.
 *
 * @param[in, out] pDrvCtrl The VGA driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _vgaVfsWrite(void*       pDrvCtrl,
                            void*       pHandle,
                            const void* kpBuffer,
                            size_t      count);

/**
 * @brief VGA VFS IOCTL hook.
 *
 * @details VGA VFS IOCTL hook. This function performs the IOCTL for the VGA
 * driver.
 *
 * @param[in, out] pDrvCtrl The VGA driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _vgaVfsIOCTL(void*    pDriverData,
                            void*    pHandle,
                            uint32_t operation,
                            void*    pArgs);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief VGA driver instance. */
static driver_t sX86VGADriver = {
    .pName         = "X86 VGA driver",
    .pDescription  = "X86 VGA driver for roOs",
    .pCompatible   = "x86,x86-vga-text",
    .pVersion      = "2.0",
    .pDriverAttach = _vgaConsoleAttach
};


/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _vgaConsoleAttach(const fdt_node_t* pkFdtNode)
{
    const uintptr_t* ptrProp;
    const uint32_t*  kpUintProp;
    const char*      kpStrProp;
    size_t           propLen;
    OS_RETURN_E      retCode;
    OS_RETURN_E      error;
    vga_controler_t* pDrvCtrl;
    colorscheme_t    initScheme;
    uintptr_t        frameBufferPhysAddr;
    size_t           frameBufferPhysSize;
    void*            mappedFrameBufferAddr;

    /* Init structures */
    pDrvCtrl = kmalloc(sizeof(vga_controler_t));
    if(pDrvCtrl == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pDrvCtrl, 0, sizeof(vga_controler_t));
    pDrvCtrl->vfsDriver = VFS_DRIVER_INVALID;

    /* Get the VGA framebuffer address */
    ptrProp = fdtGetProp(pkFdtNode, VGA_FDT_REG_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
#ifdef ARCH_32_BITS
    pDrvCtrl->pFramebuffer    = (uint16_t*)FDTTOCPU32(*ptrProp);
    pDrvCtrl->framebufferSize = (size_t)FDTTOCPU32(*(ptrProp + 1));
#elif defined(ARCH_64_BITS)
    pDrvCtrl->pFramebuffer    = (uint16_t*)FDTTOCPU64(*ptrProp);
    pDrvCtrl->framebufferSize = (size_t)FDTTOCPU64(*(ptrProp + 1));
#else
    #error "Invalid architecture"
#endif

    /* Align and map framebuffer */
    frameBufferPhysAddr = (uintptr_t)pDrvCtrl->pFramebuffer & ~PAGE_SIZE_MASK;
    frameBufferPhysSize = pDrvCtrl->framebufferSize;

    frameBufferPhysSize += (uintptr_t)pDrvCtrl->pFramebuffer -
                           frameBufferPhysAddr;
    frameBufferPhysSize = (frameBufferPhysSize + PAGE_SIZE_MASK) &
                          ~PAGE_SIZE_MASK;

    mappedFrameBufferAddr = memoryKernelMap((void*)frameBufferPhysAddr,
                                            frameBufferPhysSize,
                                            MEMMGR_MAP_HARDWARE |
                                            MEMMGR_MAP_KERNEL   |
                                            MEMMGR_MAP_RW,
                                            &retCode);

#if VGA_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Framebuffer: 0x%p: 0x%p (0x%p) | Size: 0x%p (0x%p)",
           mappedFrameBufferAddr,
           pDrvCtrl->pFramebuffer,
           frameBufferPhysAddr,
           pDrvCtrl->framebufferSize,
           frameBufferPhysSize);
#endif

    if(mappedFrameBufferAddr == NULL || retCode != OS_NO_ERR)
    {
        goto ATTACH_END;
    }

    /* Update framebuffer address but not size even if we mapped more */
    pDrvCtrl->pFramebuffer = mappedFrameBufferAddr;

    /* Get the VGA CPU communication ports */
    kpUintProp = fdtGetProp(pkFdtNode, VGA_FDT_COMM_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->cpuCommPort = (uint16_t)FDTTOCPU32(*kpUintProp);
    pDrvCtrl->cpuDataPort = (uint16_t)FDTTOCPU32(*(kpUintProp + 1));

#if VGA_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "COMM: 0x%x | DATA: 0x%x",
           pDrvCtrl->cpuCommPort,
           pDrvCtrl->cpuDataPort);
#endif

    /* Get the resolution */
    kpUintProp = fdtGetProp(pkFdtNode, VGA_FDT_RES_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->columnCount = (uint8_t)FDTTOCPU32(*kpUintProp);
    pDrvCtrl->lineCount   = (uint8_t)FDTTOCPU32(*(kpUintProp + 1));

#if VGA_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Resolution: %dx%d",
           pDrvCtrl->columnCount, pDrvCtrl->lineCount);
#endif

    /* Set initial scheme */
    initScheme.background = BG_BLACK;
    initScheme.foreground = FG_WHITE;
    _vgaSetScheme(pDrvCtrl, &initScheme);

    /* Get the device path */
    kpStrProp = fdtGetProp(pkFdtNode, VGA_FDT_DEVICE_PROP, &propLen);
    if(kpStrProp == NULL || propLen  == 0)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Register the driver */
    pDrvCtrl->vfsDriver = vfsRegisterDriver(kpStrProp,
                                            pDrvCtrl,
                                            _vgaVfsOpen,
                                            _vgaVfsClose,
                                            NULL,
                                            _vgaVfsWrite,
                                            NULL,
                                            _vgaVfsIOCTL);
    if(pDrvCtrl->vfsDriver == VFS_DRIVER_INVALID)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

#if VGA_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "VGA driver initialized");
#endif

ATTACH_END:
    if(retCode != OS_NO_ERR)
    {
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

static inline void _vgaPutCursorSafe(vga_controler_t* pCtrl,
                                     const uint32_t   kLine,
                                     const uint32_t   kColumn)
{
    uint16_t cursorPosition;

    /* Checks the values of line and column */
    if(kLine > pCtrl->lineCount ||
       kColumn > pCtrl->columnCount)
    {
        return;
    }

    /* Set new cursor position */
    pCtrl->screenCursor.x = kColumn;
    pCtrl->screenCursor.y = kLine;

    /* Display new position on screen */
    cursorPosition = kColumn + kLine * pCtrl->columnCount;

    /* Send low part to the screen */
    _cpuOutB(VGA_CONSOLE_CURSOR_COMM_LOW, pCtrl->cpuCommPort);
    _cpuOutB((int8_t)(cursorPosition & 0x00FF), pCtrl->cpuDataPort);

    /* Send high part to the screen */
    _cpuOutB(VGA_CONSOLE_CURSOR_COMM_HIGH, pCtrl->cpuCommPort);
    _cpuOutB((int8_t)((cursorPosition & 0xFF00) >> 8), pCtrl->cpuDataPort);
}


static inline void _vgaPrintChar(void*          pDriverCtrl,
                                 const uint32_t kLine,
                                 const uint32_t kColumn,
                                 const char     kCharacter)
{
    uint16_t*        pScreenMem;
    vga_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    if((uint8_t)kLine  > pCtrl->lineCount - 1 ||
       (uint8_t)kColumn > pCtrl->columnCount - 1)
    {
        return;
    }

    /* Get address to inject */
    pScreenMem = GET_FRAME_BUFFER_AT(pCtrl, kLine, kColumn);

    /* Inject the character with the current colorscheme */
    *pScreenMem = kCharacter |
                  ((pCtrl->screenScheme.background << 8) & 0xF000) |
                  ((pCtrl->screenScheme.foreground << 8) & 0x0F00);
}

static void _vgaProcessChar(void* pDriverCtrl, const char kCharacter)
{
    vga_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    /* If character is a normal ASCII character */
    if(kCharacter > 31 && kCharacter < 127)
    {
        /* Manage end of line cursor position */
        if((uint8_t)pCtrl->screenCursor.x > pCtrl->columnCount - 1)
        {
            _vgaPutCursorSafe(pDriverCtrl, pCtrl->screenCursor.y + 1, 0);
        }

        /* Manage end of screen cursor position */
        if((uint8_t)pCtrl->screenCursor.y >= pCtrl->lineCount)
        {
            _vgaScrollSafe(pDriverCtrl, SCROLL_DOWN, 1);

        }
        else
        {
            /* Move cursor */
            _vgaPutCursorSafe(pDriverCtrl,
                              pCtrl->screenCursor.y,
                              pCtrl->screenCursor.x);
        }

        /* Display character and move cursor */
        _vgaPrintChar(pDriverCtrl,
                      pCtrl->screenCursor.y,
                      pCtrl->screenCursor.x++,
                      kCharacter);
    }
    else
    {
        /* Manage special ACSII characters*/
        switch(kCharacter)
        {
            /* Backspace */
            case '\b':
                if(pCtrl->screenCursor.x > 0)
                {
                    _vgaPutCursorSafe(pDriverCtrl,
                                      pCtrl->screenCursor.y,
                                      pCtrl->screenCursor.x - 1);
                }
                else if(pCtrl->screenCursor.y > 0)
                {
                    _vgaPutCursorSafe(pDriverCtrl,
                                      pCtrl->screenCursor.y - 1,
                                      pCtrl->columnCount - 1);
                }
                break;
            /* Tab */
            case '\t':
                if((uint8_t)pCtrl->screenCursor.x + 8 <
                   pCtrl->columnCount - 1)
                {
                    _vgaPutCursorSafe(pDriverCtrl,
                                      pCtrl->screenCursor.y,
                                      pCtrl->screenCursor.x  +
                                      (8 - pCtrl->screenCursor.x % 8));
                }
                else
                {
                    _vgaPutCursorSafe(pDriverCtrl,
                                      pCtrl->screenCursor.y,
                                      pCtrl->columnCount - 1);
                }
                break;
            /* Line feed */
            case '\n':
                if((uint8_t)pCtrl->screenCursor.y < pCtrl->lineCount - 1)
                {
                    _vgaPutCursorSafe(pDriverCtrl,
                                      pCtrl->screenCursor.y + 1,
                                      0);
                }
                else
                {
                    _vgaScrollSafe(pDriverCtrl, SCROLL_DOWN, 1);
                }
                break;
            /* Clear screen */
            case '\f':
                /* Clear all screen */
                memset(pCtrl->pFramebuffer, 0, pCtrl->framebufferSize);
                break;
            /* Line return */
            case '\r':
                _vgaPutCursorSafe(pDriverCtrl, pCtrl->screenCursor.y, 0);
                break;
            /* Undefined */
            default:
                break;
        }
    }
}

static void _vgaClearFramebuffer(void* pDriverCtrl)
{
    vga_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    /* Clear all screen */
    memset(pCtrl->pFramebuffer, 0, pCtrl->framebufferSize);
}

static void _vgaSaveCursor(void* pDriverCtrl, cursor_t* pBuffer)
{
    vga_controler_t* pCtrl;

    if(pBuffer != NULL)
    {
        pCtrl = GET_CONTROLER(pDriverCtrl);

        /* Save cursor attributes */
        pBuffer->x = pCtrl->screenCursor.x;
        pBuffer->y = pCtrl->screenCursor.y;
    }
}

static void _vgaRestoreCursor(void* pDriverCtrl, const cursor_t* kpBuffer)
{
    vga_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    if(kpBuffer->x >= pCtrl->columnCount || kpBuffer->y >= pCtrl->lineCount)
    {
        return;
    }
    /* Restore cursor attributes */
    _vgaPutCursorSafe(pDriverCtrl, kpBuffer->y, kpBuffer->x);
}

static void _vgaScrollSafe(vga_controler_t*         pCtrl,
                           const SCROLL_DIRECTION_E kDirection,
                           const uint32_t           kLines)
{
    uint8_t toScroll;
    uint8_t i;
    uint8_t j;

    if(pCtrl->lineCount < kLines)
    {
        toScroll = pCtrl->lineCount;
    }
    else
    {
        toScroll = kLines;
    }

    /* Select scroll direction */
    if(kDirection == SCROLL_DOWN)
    {
        /* For each line scroll we want */
        for(j = 0; j < toScroll; ++j)
        {
            /* Copy all the lines to the above one */
            for(i = 0; i < pCtrl->lineCount - 1; ++i)
            {
                memmove(GET_FRAME_BUFFER_AT(pCtrl, i, 0),
                        GET_FRAME_BUFFER_AT(pCtrl, i + 1, 0),
                        sizeof(uint16_t) * pCtrl->columnCount);
            }
        }
        /* Clear last line */
        for(i = 0; i < pCtrl->columnCount; ++i)
        {
            _vgaPrintChar(pCtrl, pCtrl->lineCount - 1, i, ' ');
        }

        /* Replace cursor */
        _vgaPutCursorSafe(pCtrl, pCtrl->lineCount - toScroll, 0);
    }
}

static void _vgaScroll(void*                    pDriverCtrl,
                       const SCROLL_DIRECTION_E kDirection,
                       const uint32_t           kLines)
{
    vga_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    _vgaScrollSafe(pCtrl, kDirection, kLines);
}

static void _vgaSetScheme(void* pDriverCtrl, const colorscheme_t* kpColorScheme)
{
    vga_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    pCtrl->screenScheme.foreground = kpColorScheme->foreground;
    pCtrl->screenScheme.background = kpColorScheme->background;
}

static void _vgaSaveScheme(void* pDriverCtrl, colorscheme_t* pBuffer)
{
    vga_controler_t* pCtrl;


    if(pBuffer != NULL)
    {
        pCtrl = GET_CONTROLER(pDriverCtrl);

        /* Save color scheme into buffer */
        pBuffer->foreground = pCtrl->screenScheme.foreground;
        pBuffer->background = pCtrl->screenScheme.background;
    }
}

static void* _vgaVfsOpen(void*       pDrvCtrl,
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

    /* The flags must be O_RDWR */
    if(flags != O_RDWR)
    {
        return (void*)-1;
    }

    /* We don't need a handle, return NULL */
    return NULL;
}

static int32_t _vgaVfsClose(void* pDrvCtrl, void* pHandle)
{
    (void)pDrvCtrl;
    if(pHandle == (void*)-1)
    {
        return -1;
    }

    /* Nothing to do */
    return 0;
}

static ssize_t _vgaVfsWrite(void*       pDrvCtrl,
                            void*       pHandle,
                            const void* kpBuffer,
                            size_t      count)
{
    const char* pCursor;
    size_t      coutSave;

    if(pHandle == (void*)-1)
    {
        return -1;
    }

    pCursor = (char*)kpBuffer;

    /* Output each character of the string */
    coutSave = count;
    while(pCursor != NULL && *pCursor != 0 && count > 0)
    {
        _vgaProcessChar(pDrvCtrl, *pCursor);
        ++pCursor;
        --count;
    }

    return coutSave - count;
}

static ssize_t _vgaVfsIOCTL(void*    pDriverData,
                            void*    pHandle,
                            uint32_t operation,
                            void*    pArgs)
{
    int32_t                   retVal;
    cons_ioctl_args_scroll_t* pScrollArgs;

    if(pHandle == (void*)-1)
    {
        return -1;
    }

    /* Switch on the operation */
    retVal = 0;
    switch(operation)
    {
        case VFS_IOCTL_CONS_RESTORE_CURSOR:
            _vgaRestoreCursor(pDriverData, pArgs);
            break;
        case VFS_IOCTL_CONS_SAVE_CURSOR:
            _vgaSaveCursor(pDriverData, pArgs);
            break;
        case VFS_IOCTL_CONS_SCROLL:
            pScrollArgs = pArgs;
            _vgaScroll(pDriverData,
                       pScrollArgs->direction,
                       pScrollArgs->lineCount);
            break;
        case VFS_IOCTL_CONS_SET_COLORSCHEME:
            _vgaSetScheme(pDriverData, pArgs);
            break;
        case VFS_IOCTL_CONS_SAVE_COLORSCHEME:
            _vgaSaveScheme(pDriverData, pArgs);
            break;
        case VFS_IOCTL_CONS_CLEAR:
            _vgaClearFramebuffer(pDriverData);
            break;
        case VFS_IOCTL_CONS_FLUSH:
            break;
        default:
            retVal = -1;
    }

    return retVal;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86VGADriver);

/************************************ EOF *************************************/