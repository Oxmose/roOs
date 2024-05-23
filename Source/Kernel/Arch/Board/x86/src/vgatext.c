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

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief x86 VGA Text driver controler. */
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
#define GET_FRAME_BUFFER_AT(LINE, COL)                                   \
        ((sDrvCtrl.pFramebuffer) + ((COL) + (LINE) * sDrvCtrl.columnCount))

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

static OS_RETURN_E _vgaConsoleAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Prints a character to the selected coordinates.
 *
 * @details Prints a character to the selected coordinates by setting the memory
 * accordingly.
 *
 * @param[in] uint32_t kLine The - line index where to write the character.
 * @param[in] uint32_t kColumn - The colums index where to write the character.
 * @param[in] char kCharacter - The character to display on the screem.
 */
inline static void _vgaPrintChar(const uint32_t kLine,
                                 const uint32_t kColumn,
                                 const char     kCharacter);

/**
 * @brief Processes the character in parameters.
 *
 * @details Check the character nature and code. Corresponding to the
 * character's code, an action is taken. A regular character will be printed
 * whereas \\n will create a line feed.
 *
 * @param[in] char kCharacter - The character to process.
 */
static void _vgaProcessChar(const char kCharacter);

/**
 * @brief Clears the screen by printing null character character on black
 * background.
 */
static void _vgaClearFramebuffer(void);

/**
 * @brief Places the cursor to the selected coordinates given as parameters.
 *
 * @details Places the screen cursor to the coordinated described with the
 * parameters. The function will check the boundaries or the position parameters
 * before setting the cursor position.
 *
 * @param[in] uint32_t kLine - The line index where to place the cursor.
 * @param[in] uint32_t column - The column index where to place the cursor.
 */
static void _vgaPutCursor(const uint32_t kLine, const uint32_t kColumn);

/**
 * @brief Saves the cursor attributes in the buffer given as parameter.
 *
 * @details Fills the buffer given as parrameter with the current cursor
 * settings.
 *
 * @param[out] cursor_t* pBuffer - The cursor buffer in which the current cursor
 * position is going to be saved.
 */
static void _vgaSaveCursor(cursor_t* pBuffer);

/**
 * @brief Restores the cursor attributes from the buffer given as parameter.
 *
 * @details The function will restores the cursor attributes from the buffer
 * given as parameter.
 *
 * @param[in] cursor_t* kpBuffer - The cursor buffer containing the new
 * coordinates of the cursor.
 */
static void _vgaResotreCursor(const cursor_t* kpBuffer);

/**
 * @brief Scrolls in the desired direction of lines_count lines.
 *
 * @details The function will scroll of lines_count line in the desired
 * direction.
 *
 * @param[in] SCROLL_DIRECTION_E kDirection - The direction to whoch the console
 * should be scrolled.
 * @param[in] uint32_t kLines - The number of lines to scroll.
 */
static void _vgaScroll(const SCROLL_DIRECTION_E kDirection,
                       const uint32_t           kLines);


/**
 * @brief Sets the color scheme of the screen.
 *
 * @details Replaces the curent color scheme used t output data with the new
 * one given as parameter.
 *
 * @param[in] colorscheme_t* kpColorScheme - The new color scheme to apply to
 * the screen console.
 */
static void _vgaSetScheme(const colorscheme_t* kpColorScheme);


/**
 * @brief Saves the color scheme in the buffer given as parameter.
 *
 * @details Fills the buffer given as parameter with the current screen's
 * color scheme value.
 *
 * @param[out] colorscheme_t* pBuffer - The buffer that will receive the current
 * color scheme used by the screen console.
 */
static void _vgaSaveScheme(colorscheme_t* pBuffer);

/**
 * ­@brief Put a string to screen.
 *
 * @details The function will display the string given as parameter to the
 * screen.
 *
 * @param[in] char* kpString - The string to display on the screen.
 *
 * @warning kpString must be NULL terminated.
 */
static void _vgaPutString(const char* kpString);

/**
 * ­@brief Put a character to screen.
 *
 * @details The function will display the character given as parameter to the
 * screen.
 *
 * @param[in] char kCharacter - The char to display on the screen.
 */
static void _vgaPutChar(const char kCharacter);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/** @brief VGA text driver instance. */
driver_t x86VGADriver = {
    .pName         = "X86 VGA Text Driver",
    .pDescription  = "X86 VGA Text Driver for UTK",
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
    .pPutChar         = _vgaPutChar
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
        _vgaClearFramebuffer();
        retCode = consoleSetDriver(&sVGAConsoleDriver);
        if(retCode != OS_NO_ERR)
        {
            KERNEL_ERROR("Failed to set VGA driver as console driver.\n");
            goto ATTACH_END;
        }
    }

    KERNEL_DEBUG(VGA_DEBUG_ENABLED, MODULE_NAME, "VGA text driver initialized");
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


inline static void _vgaPrintChar(const uint32_t kLine,
                                 const uint32_t kColumn,
                                 const char     kCharacter)
{
    uint16_t* screenMem;

    if((uint8_t)kLine  > sDrvCtrl.lineCount - 1 ||
       (uint8_t)kColumn > sDrvCtrl.columnCount - 1)
    {
        return;
    }

    /* Get address to inject */
    screenMem = GET_FRAME_BUFFER_AT(kLine, kColumn);

    /* Inject the character with the current colorscheme */
    *screenMem = kCharacter |
                  ((sDrvCtrl.screenScheme.background << 8) & 0xF000) |
                  ((sDrvCtrl.screenScheme.foreground << 8) & 0x0F00);
}

static void _vgaProcessChar(const char kCharacter)
{
    /* If character is a normal ASCII character */
    if(kCharacter > 31 && kCharacter < 127)
    {
         /* Manage end of line cursor position */
        if((uint8_t)sDrvCtrl.screenCursor.x > sDrvCtrl.columnCount - 1)
        {
            _vgaPutCursor(sDrvCtrl.screenCursor.y + 1, 0);
            sDrvCtrl.pLastColumns[sDrvCtrl.screenCursor.y] =
                sDrvCtrl.screenCursor.x;
        }

        /* Manage end of screen cursor position */
        if((uint8_t)sDrvCtrl.screenCursor.y >= sDrvCtrl.lineCount)
        {
            _vgaScroll(SCROLL_DOWN, 1);

        }
        else
        {
            /* Move cursor */
            _vgaPutCursor(sDrvCtrl.screenCursor.y, sDrvCtrl.screenCursor.x);
            sDrvCtrl.pLastColumns[sDrvCtrl.screenCursor.y] =
                sDrvCtrl.screenCursor.x;
        }

        /* Display character and move cursor */
        _vgaPrintChar(sDrvCtrl.screenCursor.y,
                      sDrvCtrl.screenCursor.x++,
                      kCharacter);
    }
    else
    {
        /* Manage special ACSII characters*/
        switch(kCharacter)
        {
            /* Backspace */
            case '\b':
                if(sDrvCtrl.lastPrintedCursor.y == sDrvCtrl.screenCursor.y)
                {
                    if(sDrvCtrl.screenCursor.x > sDrvCtrl.lastPrintedCursor.x)
                    {
                        _vgaPutCursor(sDrvCtrl.screenCursor.y,
                                      sDrvCtrl.screenCursor.x - 1);
                        sDrvCtrl.pLastColumns[sDrvCtrl.screenCursor.y] =
                            sDrvCtrl.screenCursor.x;
                        _vgaPrintChar(sDrvCtrl.screenCursor.y,
                                      sDrvCtrl.screenCursor.x, ' ');
                    }
                }
                else if(sDrvCtrl.lastPrintedCursor.y < sDrvCtrl.screenCursor.y)
                {
                    if(sDrvCtrl.screenCursor.x > 0)
                    {
                        _vgaPutCursor(sDrvCtrl.screenCursor.y,
                                      sDrvCtrl.screenCursor.x - 1);
                            sDrvCtrl.pLastColumns[sDrvCtrl.screenCursor.y] =
                                sDrvCtrl.screenCursor.x;
                        _vgaPrintChar(sDrvCtrl.screenCursor.y,
                                      sDrvCtrl.screenCursor.x, ' ');
                    }
                    else
                    {
                        if(sDrvCtrl.pLastColumns[sDrvCtrl.screenCursor.y - 1] >=
                           sDrvCtrl.columnCount)
                        {
                            sDrvCtrl.pLastColumns[sDrvCtrl.screenCursor.y - 1] =
                               sDrvCtrl.columnCount - 1;
                        }

                        _vgaPutCursor(sDrvCtrl.screenCursor.y - 1,
                            sDrvCtrl.pLastColumns[sDrvCtrl.screenCursor.y - 1]);
                        _vgaPrintChar(sDrvCtrl.screenCursor.y,
                                      sDrvCtrl.screenCursor.x, ' ');
                    }
                }
                break;
            /* Tab */
            case '\t':
                if((uint8_t)sDrvCtrl.screenCursor.x + 8 <
                   sDrvCtrl.columnCount - 1)
                {
                    _vgaPutCursor(sDrvCtrl.screenCursor.y,
                                  sDrvCtrl.screenCursor.x  +
                                  (8 - sDrvCtrl.screenCursor.x % 8));
                }
                else
                {
                    _vgaPutCursor(sDrvCtrl.screenCursor.y,
                                  sDrvCtrl.columnCount - 1);
                }
                sDrvCtrl.pLastColumns[sDrvCtrl.screenCursor.y] =
                    sDrvCtrl.screenCursor.x;
                break;
            /* Line feed */
            case '\n':
                if((uint8_t)sDrvCtrl.screenCursor.y < sDrvCtrl.lineCount - 1)
                {
                    _vgaPutCursor(sDrvCtrl.screenCursor.y + 1, 0);
                    sDrvCtrl.pLastColumns[sDrvCtrl.screenCursor.y] =
                        sDrvCtrl.screenCursor.x;
                }
                else
                {
                    _vgaScroll(SCROLL_DOWN, 1);
                }
                break;
            /* Clear screen */
            case '\f':
                _vgaClearFramebuffer();
                break;
            /* Line return */
            case '\r':
                _vgaPutCursor(sDrvCtrl.screenCursor.y, 0);
                sDrvCtrl.pLastColumns[sDrvCtrl.screenCursor.y] =
                    sDrvCtrl.screenCursor.x;
                break;
            /* Undefined */
            default:
                break;
        }
    }
}

static void _vgaClearFramebuffer(void)
{
    /* Clear all screen */
    memset(sDrvCtrl.pFramebuffer, 0, sDrvCtrl.framebufferSize);
}

static void _vgaPutCursor(const uint32_t kLine, const uint32_t kColumn)
{
    int16_t  cursorPosition;

    /* Checks the values of line and column */
    if(kLine > sDrvCtrl.columnCount ||
       kColumn > sDrvCtrl.lineCount)
    {
        return;
    }

    /* Set new cursor position */
    sDrvCtrl.screenCursor.x = kColumn;
    sDrvCtrl.screenCursor.y = kLine;

    /* Display new position on screen */
    cursorPosition = kColumn + kLine * sDrvCtrl.columnCount;

    /* Send low part to the screen */
    _cpu_outb(VGA_CONSOLE_CURSOR_COMM_LOW, sDrvCtrl.cpuCommPort);
    _cpu_outb((int8_t)(cursorPosition & 0x00FF), sDrvCtrl.cpuDataPort);

    /* Send high part to the screen */
    _cpu_outb(VGA_CONSOLE_CURSOR_COMM_HIGH, sDrvCtrl.cpuCommPort);
    _cpu_outb((int8_t)((cursorPosition & 0xFF00) >> 8), sDrvCtrl.cpuDataPort);
}

static void _vgaSaveCursor(cursor_t* pBuffer)
{
    if(pBuffer != NULL)
    {
        /* Save cursor attributes */
        pBuffer->x = sDrvCtrl.screenCursor.x;
        pBuffer->y = sDrvCtrl.screenCursor.y;
    }
}

static void _vgaResotreCursor(const cursor_t* kpBuffer)
{
    if(kpBuffer->x >= sDrvCtrl.columnCount || kpBuffer->y >= sDrvCtrl.lineCount)
    {
        return;
    }
    /* Restore cursor attributes */
    _vgaPutCursor(kpBuffer->y, kpBuffer->x);
}

static void _vgaScroll(const SCROLL_DIRECTION_E kDirection,
                       const uint32_t kLines)
{
    uint8_t toScroll;
    uint8_t i;
    uint8_t j;

    if(sDrvCtrl.lineCount < kLines)
    {
        toScroll = sDrvCtrl.lineCount;
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
            for(i = 0; i < sDrvCtrl.lineCount - 1; ++i)
            {
                memmove(GET_FRAME_BUFFER_AT(i, 0),
                        GET_FRAME_BUFFER_AT(i + 1, 0),
                        sizeof(uint16_t) * sDrvCtrl.columnCount);
                sDrvCtrl.pLastColumns[i] = sDrvCtrl.pLastColumns[i+1];
            }
            sDrvCtrl.pLastColumns[sDrvCtrl.lineCount - 1] = 0;
        }
        /* Clear last line */
        for(i = 0; i < sDrvCtrl.columnCount; ++i)
        {
            _vgaPrintChar(sDrvCtrl.lineCount - 1, i, ' ');
        }
    }

    /* Replace cursor */
    _vgaPutCursor(sDrvCtrl.lineCount - toScroll, 0);

    if(toScroll <= sDrvCtrl.lastPrintedCursor.y)
    {
        sDrvCtrl.lastPrintedCursor.y -= toScroll;
    }
    else
    {
        sDrvCtrl.lastPrintedCursor.x = 0;
        sDrvCtrl.lastPrintedCursor.y = 0;
    }
}

static void _vgaSetScheme(const colorscheme_t* kpColorScheme)
{
    sDrvCtrl.screenScheme.foreground = kpColorScheme->foreground;
    sDrvCtrl.screenScheme.background = kpColorScheme->background;
}

void _vgaSaveScheme(colorscheme_t* pBuffer)
{
    if(pBuffer != NULL)
    {
        /* Save color scheme into buffer */
        pBuffer->foreground = sDrvCtrl.screenScheme.foreground;
        pBuffer->background = sDrvCtrl.screenScheme.background;
    }
}

void _vgaPutString(const char* kpString)
{
    size_t i;

    /* Output each character of the string */
    for(i = 0; i < strlen(kpString); ++i)
    {
        _vgaPutChar(kpString[i]);
    }
}

void _vgaPutChar(const char kCharacter)
{
    _vgaProcessChar(kCharacter);
    sDrvCtrl.lastPrintedCursor = sDrvCtrl.screenCursor;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG(x86VGADriver);

/************************************ EOF *************************************/