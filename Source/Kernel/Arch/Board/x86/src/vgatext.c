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
#include <cpu.h>           /* CPU port manipulation */
#include <panic.h>         /* Kernel panic */
#include <kheap.h>         /* Kernel heap */
#include <stdint.h>        /* Generic int types */
#include <string.h>        /* String manipualtion */
#include <kerror.h>        /* Kernel error */
#include <devtree.h>       /* Device tree */
#include <console.h>       /* Console driver manager */
#include <drivermgr.h>     /* Driver manager service */
#include <kerneloutput.h>  /* Kernel output manager */

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
/** @brief FDT property for console output set */
#define VGA_FDT_IS_CON_PROP "is-console"

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

    /** @brief Curent screen's cursor settings of the last printed character. */
    cursor_t lastPrintedCursor;

    /** @brief Column index of the last printed character for each line. */
    uint8_t* pLastColumns;

    /** @brief VGA frame buffer address. */
    uint16_t* pFramebuffer;

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
        PANIC(ERROR, MODULE_NAME, MSG, TRUE);               \
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
inline static void _vgaPrintChar(void*          pDriverCtrl,
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
static void _vgaPutCursor(void*          pDriverCtrl,
                          const uint32_t kLine,
                          const uint32_t kColumn);

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
static void _vgaResotreCursor(void* pDriverCtrl, const cursor_t* kpBuffer);

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
 * ­@brief Put a string to screen.
 *
 * @details The function will display the string given as parameter to the
 * screen.
 *
 * @param[in, out] pDriverCtrl The VGA driver controler to use.
 * @param[in] kpString The string to display on the screen.
 *
 * @warning kpString must be NULL terminated.
 */
static void _vgaPutString(void* pDriverCtrl, const char* kpString);

/**
 * ­@brief Put a character to screen.
 *
 * @details The function will display the character given as parameter to the
 * screen.
 *
 * @param[in, out] pDriverCtrl The VGA driver controler to use.
 * @param[in] kCharacter The char to display on the screen.
 */
static void _vgaPutChar(void* pDriverCtrl, const char kCharacter);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/** @brief VGA driver instance. */
driver_t x86VGADriver = {
    .pName         = "X86 VGA driver",
    .pDescription  = "X86 VGA driver for UTK",
    .pCompatible   = "x86,x86-vga-text",
    .pVersion      = "2.0",
    .pDriverAttach = _vgaConsoleAttach
};

/************************** Static global variables ***************************/

/** @brief VGA driver controler instance. */
static vga_controler_t sDrvCtrl = {
    .lineCount         = 0,
    .columnCount       = 0,
    .cpuCommPort       = 0,
    .cpuDataPort       = 0,
    .screenScheme      = {.background = BG_BLACK, .foreground = FG_WHITE},
    .screenCursor      = {.x = 0, .y = 0},
    .lastPrintedCursor = {.x = 0, .y = 0},
    .pLastColumns      = NULL,
    .pFramebuffer      = NULL,
    .framebufferSize   = 0
};

/** @brief VGA console driver */
static console_driver_t sVGAConsoleDriver = {
    .pClear           = _vgaClearFramebuffer,
    .pPutCursor       = _vgaPutCursor,
    .pSaveCursor      = _vgaSaveCursor,
    .pRestoreCursor   = _vgaResotreCursor,
    .pScroll          = _vgaScroll,
    .pSetColorScheme  = _vgaSetScheme,
    .pSaveColorScheme = _vgaSaveScheme,
    .pPutString       = _vgaPutString,
    .pPutChar         = _vgaPutChar,
    .pDriverCtrl      = (void*)&sDrvCtrl
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _vgaConsoleAttach(const fdt_node_t* pkFdtNode)
{
    const uintptr_t* ptrProp;
    size_t           propLen;
    OS_RETURN_E      retCode;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_VGA_INIT_START, 0);

    /* Get the VGA framebuffer address */
    ptrProp = fdtGetProp(pkFdtNode, VGA_FDT_REG_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t) * 2)
    {
        KERNEL_ERROR("Failed to retreive the framebuffer from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.pFramebuffer    = (uint16_t*)FDTTOCPU32(*ptrProp);
    sDrvCtrl.framebufferSize = (size_t)FDTTOCPU32(*(ptrProp + 1));
    KERNEL_DEBUG(VGA_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Framebuffer: 0x%p | Size: 0x%p",
                 sDrvCtrl.pFramebuffer,
                 sDrvCtrl.framebufferSize);

    /* Get the VGA CPU communication ports */
    ptrProp = fdtGetProp(pkFdtNode, VGA_FDT_COMM_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t) * 2)
    {
        KERNEL_ERROR("Failed to retreive the CPU comm from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.cpuCommPort = (uint16_t)FDTTOCPU32(*ptrProp);
    sDrvCtrl.cpuDataPort = (uint16_t)FDTTOCPU32(*(ptrProp + 1));

    KERNEL_DEBUG(VGA_DEBUG_ENABLED,
                 MODULE_NAME,
                 "COMM: 0x%x | DATA: 0x%x",
                 sDrvCtrl.cpuCommPort,
                 sDrvCtrl.cpuDataPort);

    /* Get the resolution */
    ptrProp = fdtGetProp(pkFdtNode, VGA_FDT_RES_PROP, &propLen);
    if(ptrProp == NULL || propLen != sizeof(uintptr_t) * 2)
    {
        KERNEL_ERROR("Failed to retreive the resolution from FDT.\n");
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    sDrvCtrl.columnCount = (uint8_t)FDTTOCPU32(*ptrProp);
    sDrvCtrl.lineCount   = (uint8_t)FDTTOCPU32(*(ptrProp + 1));

    KERNEL_DEBUG(VGA_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Resolution: %dx%d",
                 sDrvCtrl.columnCount, sDrvCtrl.lineCount);

    /* Init last columns manager */
    sDrvCtrl.pLastColumns =
        (uint8_t*)kmalloc(sDrvCtrl.lineCount * sizeof(uint8_t));
    if(sDrvCtrl.pLastColumns == NULL)
    {
        KERNEL_ERROR("Failed allocate last colums manager.\n");
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(sDrvCtrl.pLastColumns, 0, sDrvCtrl.lineCount * sizeof(uint8_t));

    /* Clear screen and set as output if needed */
    if(fdtGetProp(pkFdtNode, VGA_FDT_IS_CON_PROP, &propLen) != NULL)
    {
        _vgaClearFramebuffer(&sDrvCtrl);
        retCode = consoleSetDriver(&sVGAConsoleDriver);
        if(retCode != OS_NO_ERR)
        {
            KERNEL_ERROR("Failed to set VGA driver as console driver.\n");
            goto ATTACH_END;
        }
    }

    KERNEL_DEBUG(VGA_DEBUG_ENABLED, MODULE_NAME, "VGA driver initialized");
    retCode = OS_NO_ERR;

ATTACH_END:
#ifdef ARCH_64_BITS
    KERNEL_TRACE_EVENT(EVENT_KERNEL_VGA_INIT_END, 3,
                       (uintptr_t)sDrvCtrl.pFramebuffer & 0xFFFFFFFF,
                       (uintptr_t)sDrvCtrl.pFramebuffer >> 32,
                       sDrvCtrl.framebufferSize);
#else
    KERNEL_TRACE_EVENT(EVENT_KERNEL_VGA_INIT_END, 3,
                       (uintptr_t)sDrvCtrl.pFramebuffer & 0xFFFFFFFF,
                       0,
                       sDrvCtrl.framebufferSize);
#endif
    return retCode;
}


inline static void _vgaPrintChar(void* pDriverCtrl,
                                 const uint32_t kLine,
                                 const uint32_t kColumn,
                                 const char     kCharacter)
{
    uint16_t*        screenMem;
    vga_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    if((uint8_t)kLine  > pCtrl->lineCount - 1 ||
       (uint8_t)kColumn > pCtrl->columnCount - 1)
    {
        return;
    }

    /* Get address to inject */
    screenMem = GET_FRAME_BUFFER_AT(pCtrl, kLine, kColumn);

    /* Inject the character with the current colorscheme */
    *screenMem = kCharacter |
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
            _vgaPutCursor(pDriverCtrl, pCtrl->screenCursor.y + 1, 0);
            pCtrl->pLastColumns[pCtrl->screenCursor.y] =
                pCtrl->screenCursor.x;
        }

        /* Manage end of screen cursor position */
        if((uint8_t)pCtrl->screenCursor.y >= pCtrl->lineCount)
        {
            _vgaScroll(pDriverCtrl, SCROLL_DOWN, 1);

        }
        else
        {
            /* Move cursor */
            _vgaPutCursor(pDriverCtrl,
                          pCtrl->screenCursor.y,
                          pCtrl->screenCursor.x);
            pCtrl->pLastColumns[pCtrl->screenCursor.y] =
                pCtrl->screenCursor.x;
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
                if(pCtrl->lastPrintedCursor.y == pCtrl->screenCursor.y)
                {
                    if(pCtrl->screenCursor.x > pCtrl->lastPrintedCursor.x)
                    {
                        _vgaPutCursor(pDriverCtrl,
                                      pCtrl->screenCursor.y,
                                      pCtrl->screenCursor.x - 1);
                        pCtrl->pLastColumns[pCtrl->screenCursor.y] =
                            pCtrl->screenCursor.x;
                        _vgaPrintChar(pDriverCtrl,
                                      pCtrl->screenCursor.y,
                                      pCtrl->screenCursor.x, ' ');
                    }
                }
                else if(pCtrl->lastPrintedCursor.y < pCtrl->screenCursor.y)
                {
                    if(pCtrl->screenCursor.x > 0)
                    {
                        _vgaPutCursor(pDriverCtrl,
                                      pCtrl->screenCursor.y,
                                      pCtrl->screenCursor.x - 1);
                            pCtrl->pLastColumns[pCtrl->screenCursor.y] =
                                pCtrl->screenCursor.x;
                        _vgaPrintChar(pDriverCtrl,
                                      pCtrl->screenCursor.y,
                                      pCtrl->screenCursor.x, ' ');
                    }
                    else
                    {
                        if(pCtrl->pLastColumns[pCtrl->screenCursor.y - 1] >=
                           pCtrl->columnCount)
                        {
                            pCtrl->pLastColumns[pCtrl->screenCursor.y - 1] =
                               pCtrl->columnCount - 1;
                        }

                        _vgaPutCursor(pDriverCtrl,
                                      pCtrl->screenCursor.y - 1,
                            pCtrl->pLastColumns[pCtrl->screenCursor.y - 1]);
                        _vgaPrintChar(pDriverCtrl,
                                      pCtrl->screenCursor.y,
                                      pCtrl->screenCursor.x, ' ');
                    }
                }
                break;
            /* Tab */
            case '\t':
                if((uint8_t)pCtrl->screenCursor.x + 8 <
                   pCtrl->columnCount - 1)
                {
                    _vgaPutCursor(pDriverCtrl,
                                  pCtrl->screenCursor.y,
                                  pCtrl->screenCursor.x  +
                                  (8 - pCtrl->screenCursor.x % 8));
                }
                else
                {
                    _vgaPutCursor(pDriverCtrl,
                                  pCtrl->screenCursor.y,
                                  pCtrl->columnCount - 1);
                }
                pCtrl->pLastColumns[pCtrl->screenCursor.y] =
                    pCtrl->screenCursor.x;
                break;
            /* Line feed */
            case '\n':
                if((uint8_t)pCtrl->screenCursor.y < pCtrl->lineCount - 1)
                {
                    _vgaPutCursor(pDriverCtrl, pCtrl->screenCursor.y + 1, 0);
                    pCtrl->pLastColumns[pCtrl->screenCursor.y] =
                        pCtrl->screenCursor.x;
                }
                else
                {
                    _vgaScroll(pDriverCtrl, SCROLL_DOWN, 1);
                }
                break;
            /* Clear screen */
            case '\f':
                _vgaClearFramebuffer(pDriverCtrl);
                break;
            /* Line return */
            case '\r':
                _vgaPutCursor(pDriverCtrl, pCtrl->screenCursor.y, 0);
                pCtrl->pLastColumns[pCtrl->screenCursor.y] =
                    pCtrl->screenCursor.x;
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

static void _vgaPutCursor(void* pDriverCtrl,
                          const uint32_t kLine,
                          const uint32_t kColumn)
{
    int16_t          cursorPosition;
    vga_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

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

static void _vgaSaveCursor(void* pDriverCtrl, cursor_t* pBuffer)
{
    vga_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    if(pBuffer != NULL)
    {
        /* Save cursor attributes */
        pBuffer->x = pCtrl->screenCursor.x;
        pBuffer->y = pCtrl->screenCursor.y;
    }
}

static void _vgaResotreCursor(void* pDriverCtrl, const cursor_t* kpBuffer)
{
    vga_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    if(kpBuffer->x >= pCtrl->columnCount || kpBuffer->y >= pCtrl->lineCount)
    {
        return;
    }
    /* Restore cursor attributes */
    _vgaPutCursor(pDriverCtrl, kpBuffer->y, kpBuffer->x);
}

static void _vgaScroll(void* pDriverCtrl,
                       const SCROLL_DIRECTION_E kDirection,
                       const uint32_t kLines)
{
    uint8_t          toScroll;
    uint8_t          i;
    uint8_t          j;
    vga_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

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
                pCtrl->pLastColumns[i] = pCtrl->pLastColumns[i+1];
            }
            pCtrl->pLastColumns[pCtrl->lineCount - 1] = 0;
        }
        /* Clear last line */
        for(i = 0; i < pCtrl->columnCount; ++i)
        {
            _vgaPrintChar(pDriverCtrl, pCtrl->lineCount - 1, i, ' ');
        }
    }

    /* Replace cursor */
    _vgaPutCursor(pDriverCtrl, pCtrl->lineCount - toScroll, 0);

    if(toScroll <= pCtrl->lastPrintedCursor.y)
    {
        pCtrl->lastPrintedCursor.y -= toScroll;
    }
    else
    {
        pCtrl->lastPrintedCursor.x = 0;
        pCtrl->lastPrintedCursor.y = 0;
    }
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

    pCtrl = GET_CONTROLER(pDriverCtrl);

    if(pBuffer != NULL)
    {
        /* Save color scheme into buffer */
        pBuffer->foreground = pCtrl->screenScheme.foreground;
        pBuffer->background = pCtrl->screenScheme.background;
    }
}

static void _vgaPutString(void* pDriverCtrl, const char* kpString)
{
    size_t i;
    size_t stringLen;

    stringLen = strlen(kpString);

    /* Output each character of the string */
    for(i = 0; i < stringLen; ++i)
    {
        _vgaPutChar(pDriverCtrl, kpString[i]);
    }
}

static void _vgaPutChar(void* pDriverCtrl, const char kCharacter)
{
    vga_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    _vgaProcessChar(pDriverCtrl, kCharacter);
    pCtrl->lastPrintedCursor = pCtrl->screenCursor;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG(x86VGADriver);

/************************************ EOF *************************************/