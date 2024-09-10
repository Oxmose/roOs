/*******************************************************************************
 * @file vesa.c
 *
 * @see vesa.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 09/07/2024
 *
 * @version 2.0
 *
 * @brief VESA VBE 2 graphic driver.
 *
 * @details VESA VBE 2 graphic drivers. Allows the kernel to have a generic high
 * resolution output. The driver provides regular console output management and
 * generic screen drawing functions.
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
#include <time_mgt.h>      /* Time manager */
#include <drivermgr.h>     /* Driver manager service */
#include <scheduler.h>     /* Kernel scheduler */

/* Configuration files */
#include <config.h>

/* Header file */
#include <vesa.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Module's name */
#define MODULE_NAME "X86_VESA"

/** @brief FDT property for resolution */
#define VESA_FDT_RES_PROP "resolution"
/** @brief FDT property for color depth */
#define VESA_FDT_DEPTH_PROP "depth"
/** @brief FDT property for refresh rate */
#define VESA_FDT_REFRESH_PROP "refresh-rate"
/** @brief FDT property for device path */
#define VESA_FDT_DEVICE_PROP "device"

/** @brief Defines the height in pixel of a character for the VESA driver */
#define VESA_TEXT_CHAR_HEIGHT sVesaFontHeight
/** @brief Defines the width in pixel of a character for the VESA driver */
#define VESA_TEXT_CHAR_WIDTH sVesaFontWidth

/** @brief Defines the VESA BIOS call interrupt */
#define VESA_BIOS_CALL_INT 0x10

/** @brief Defines the VESA BIOS call get info function */
#define VESA_BIOS_CALL_GET_INFO_ID 0x4F00
/** @brief Defines the VESA BIOS call get mode info function */
#define VESA_BIOS_CALL_GET_MODE_ID 0x4F01
/** @brief Defines teh VESA BIOS call set mode function */
#define VESA_BIOS_CALL_SET_MODE 0x4F02

/** @brief Defines the BIOS call return value OK */
#define VESA_BIOS_CALL_RETURN_OK 0x004F

/** @brief Defines the OEM data size */
#define VESA_OEM_DATA_SIZE 256

/** @brief VESA mode attribute flag supported */
#define VESA_ATTRIBUTE_SUPPORTED 0x1
/** @brief VESA mode attribute flag linear frame buffer */
#define VESA_ATTRIBUTE_LINEAR_FB 0x90
/** @brief VESA memory model packed */
#define VESA_MEMORY_MODEL_PACKED 0x4
/** @brief VESA memory model direct color */
#define VESA_MEMORY_MODEL_DIRECTCOLOR 0x6

/** @brief VESA mode command: enable linear framebuffer. */
#define VESA_FLAG_LINEAR_FB_ENABLE 0x4000

/** @brief VESA display thread priority */
#define VESA_DISPLAY_THREAD_PRIO 0
/** @brief VESA display thread name */
#define VESA_DISPLAY_THREAD_NAME "vesaDisplay"
/** @brief VESA display thread stack size */
#define VESA_DISPLAY_THREAD_STACK_SIZE 0x1000
/** @brief VESA display thread affinity */
#define VESA_DISPLAY_THREAD_AFFINITY 0

/** @brief Tabulation size */
#define VESA_TAB_SIZE 4

/** @brief Cast a pointer to a vesa driver controler */
#define GET_CONTROLER(PTR) ((vesa_controler_t*)PTR)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Defines a VBE mode node containing a VBE mode information */
typedef struct vbe_mode_t
{
    /** @brief The mode resolution's width. */
    uint16_t width;
    /** @brief The mode resolution's height. */
    uint16_t height;
    /** @brief The mode's color depth. */
    uint16_t bpp;
    /** @brief The mode's id. */
    uint16_t id;
    /** @brief The number of bytes per logical line */
    uint16_t bytePerScanLine;

    /** @brief Start of the physical address of the mode's framebuffer. */
    void* framebuffer;

    /** @brief Link to the next node */
    struct vbe_mode_t* pNext;
} vbe_mode_t;

/** @brief VBE information structure, see the VBE standard for more information
 * about the contained data.
 */
typedef struct
{
    /** @brief The VBE signature. */
    char pSignature[4];
    /** @brief The VBE version. */
    uint16_t version;
    /** @brief The pointer to the OEM String. */
    uint32_t oem;
    /** @brief Capabilities of the graphics controller */
    uint32_t capabilities;
    /** @brief Pointer to the video mode list */
    uint32_t videoModes;
    /** @brief Number of memory blocks */
    uint16_t totalMemory;
    /** @brief VBE software revision */
    uint16_t softwareRev;
    /** @brief Pointer to the vendore name string */
    uint32_t vendor;
    /** @brief Pointer to the product name string */
    uint32_t productName;
    /** @brief Pointer to the product revision string */
    uint32_t productRev;
    /** @brief Reserved */
    uint8_t  reserved[222];
    /** @brief Data for OEM strings */
    uint8_t  oemData[VESA_OEM_DATA_SIZE];
} __attribute__((packed)) vbe_info_t;

/** @brief VBE mode information structure, see the VBE standard for more
 * information about the contained data.
 */
typedef struct
{
    /** @brief Mode attributes */
    uint16_t attributes;
    /** @brief Window A attributes */
    uint8_t  windowA;
    /** @brief Window B attributes */
    uint8_t  windowB;
    /** @brief Window granularity */
    uint16_t granularity;
    /** @brief Window size */
    uint16_t windowSize;
    /** @brief Window A start segment */
    uint16_t segmentA;
    /** @brief Window B start segment */
    uint16_t segmentB;
    /** @brief Pointer to window function */
    uint32_t winFuncPtr;
    /** @brief Bytes per scan line */
    uint16_t bytesPerScanLine;
    /** @brief Horizontal resolution in pixels or characters */
    uint16_t width;
    /** @brief Vertical resolution in pixels or characters */
    uint16_t height;
    /** @brief Character width in pixels */
    uint8_t  wChar;
    /** @brief Character height in pixels */
    uint8_t  yChar;
    /** @brief Number of memory planes */
    uint8_t  planes;
    /** @brief Color depth (Bits Per Pixel) */
    uint8_t  bpp;
    /** @brief Number of banks */
    uint8_t  banks;
    /** @brief Memory model type */
    uint8_t  memoryModel;
    /** @brief Bank size in KB */
    uint8_t  bankSize;
    /** @brief Number of images */
    uint8_t  imagePages;
    /** @brief Reserved */
    uint8_t  reserved0;
    /** @brief Size of direct color red mask in bits */
    uint8_t  redMask;
    /** @brief Bit position if LSB of red mask */
    uint8_t  redPosition;
    /** @brief Size of direct color green mask in bits */
    uint8_t  greenMask;
    /** @brief Bit position if LSB of green mask */
    uint8_t  greenPosition;
    /** @brief Size of direct color blue mask in bits */
    uint8_t  blueMask;
    /** @brief Bit position if LSB of blue mask */
    uint8_t  bluePosition;
    /** @brief Size of direct color reserved mask in bits */
    uint8_t  reservedMask;
    /** @brief Bit position if LSB of reserved mask */
    uint8_t  reservedPosition;
    /** @brief Direct color mode attributes */
    uint8_t  directColorAttributes;
    /** @brief Physical address of the framebuffer */
    uint32_t framebuffer;
    /** @brief Pointer to the start of the off screen memory */
    uint32_t offScreenMemOff;
    /** @brief Amount of off screen momory in 1K unit */
    uint16_t offScreenMemSize;
    /** @brief Reserved */
    uint8_t  reserved1[206];
} __attribute__((packed)) vbe_mode_info_t;

/** @brief Double buffering structure. */
typedef struct
{
    /** @brief Current mode frame buffer */
    void* pFramebuffer;

    /** @brief Current mode frame buffer mapping size */
    size_t hwFramebufferSize;

    /** @brief The back buffer pointer */
    void* pBack;

    /** @brief Current back buffer mapping size */
    size_t backBufferSize;
} double_buffer_t;

/** @brief x86 VESA driver controler. */
typedef struct
{
    /** @brief Current mode */
    vbe_mode_t* pCurrentMode;

    /** @brief Stores the curent screen's color scheme. */
    colorscheme_t screenScheme;

    /** @brief Stores the curent screen's cursor settings. */
    cursor_t screenCursor;

    /** @brief Stores the number of lines for the text mode */
    uint32_t lineCount;

    /** @brief Stores the number of columns for the text mode */
    uint32_t columnCount;

    /** @brief Contains the detected VESA information */
    vbe_mode_t* pVbeModes;

    /** @brief Contains the VBE information */
    vbe_info_t vbeInfo;

    /** @brief Stores the video buffer */
    double_buffer_t videoBuffer;

    /** @brief Display thread */
    kernel_thread_t* pDisplayThread;

    /** @brief Stores the VFS driver */
    vfs_driver_t vfsDriver;

    /** @brief Refresh rate */
    uint32_t refreshRate;
} vesa_controler_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the VESA to ensure correctness of execution.
 *
 * @details Assert macro used by the VESA to ensure correctness of execution.
 * Due to the critical nature of the VESA, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define VESA_ASSERT(COND, MSG, ERROR) {                     \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/**
 * @brief Unrolled action for the fast memcpy used by the VESA driver.
 *
 * @details Unrolled action for the fast memcpy used by the VESA driver. Used in
 * the Duff's device.
 *
 * @param[in] X The switch case.
 */
#define VESA_FAST_CPY_UNROLL_ACTION(X) {                        \
    case (X):                                                   \
        __asm__ __volatile__ ("movups (%0), %%xmm7\n\t"         \
                                "movntdq %%xmm7, (%1)\n\t"      \
                                :                               \
                                : "r"(pSrcPtr), "r"(pDstPtr)    \
                                : "memory");                    \
        pDstPtr += 16;                                          \
        pSrcPtr += 16;                                          \
}

/**
 * @brief Unrolled action for the fast fill used by the VESA driver.
 *
 * @details Unrolled action for the fast fill used by the VESA driver. Used in
 * the Duff's device.
 *
 * @param[in] X The switch case.
 */
#define VESA_FAST_FILL_UNROLL_ACTION(X) {                       \
    case (X):                                                   \
        __asm__ __volatile__ ("movntdq %%xmm7, (%0)\n\t"        \
                              :                                 \
                              : "r"(pDestPtr)                   \
                              : "memory");                      \
                    pDestPtr += 16;                             \
}

/**
 * @brief Rounds closest the number given as argument.
 *
 * @details Rounds closest the number given as argument.
 *
 * @param[in] X The number to round up.
 */
#define ROUND_CLOSEST(X) (((X) - (double)((uint32_t)(X))) >= 0.5 ? \
                         ((uint32_t)(X) + 1) :                     \
                         ((uint32_t)(X)))
/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Attaches the VESA driver to the system.
 *
 * @details Attaches the VESA driver to the system. This function will use the
 * FDT to initialize the VESA hardware and retreive the VESA parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code is returned.
 */
static OS_RETURN_E _vesaDriverAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Get the VESA VBE information for this architecture.
 *
 * @details Get the VESA VBE information for this architecture. The function
 * will use a BIOS call to get the available modes and stores them in the
 * driver's controller.
 *
 * @param[out] pDrvCtrl The driver controller to populate.
 *
 * @return The success state or the error code is returned.
 */
static OS_RETURN_E _vesaGetVBEInfo(vesa_controler_t* pDrvCtrl);

/**
 * @brief Get the VESA modes for this architecture.
 *
 * @details Get the VESA modes information for this architecture. The
 * function will use a BIOS call to get the available modes and stores them in
 * the driver's controller.
 *
 * @param[out] pDrvCtrl The driver controller to populate.
 *
 * @return The success state or the error code is returned.
 */
static OS_RETURN_E _vesaGetAvailableModes(vesa_controler_t* pDrvCtrl);

/**
 * @brief Sets the VBE mode for the graphic card.
 *
 * @details Sets the VBE mode for the graphic card. The function will use a
 * BIOS call to set the new mode if found in the discovered modes.
 *
 * @param[in] kWidth The new mode resolution width.
 * @param[in] kHeight The new mode resolution height.
 * @param[in] kDepth The new mode color depth.
 * @param[in] kRate The new refresh rate.
 * @param[in] pDriverCtrl The driver controler to use.
 *
 * @return The success state or the error code is returned.
 */
static OS_RETURN_E _vesaSetGraphicMode(const uint32_t kWidth,
                                       const uint32_t kHeight,
                                       const uint32_t kDepth,
                                       const uint8_t  kRate,
                                       void*          pDriverCtrl);

/**
 * @brief VESA display routine used for the display thread.
 *
 * @details VESA display routine used for the display thread. This function
 * runs in an infinite loop and manages the buffers to display.
 *
 * @param[in, out] pDrvCtrl The pointer to the driver controller to use.
 *
 * @return The function should never return. In case of return, NULL is
 * returned.
 */
static void* _vesaDisplayRoutine(void* pDrvCtrl);

/**
 * @brief Prints a character to the selected coordinates.
 *
 * @details Prints a character to the selected coordinates by setting the memory
 * accordingly.
 *
 * @param[in, out] pDriverCtrl The VESA driver controler to use.
 * @param[in] kCharacter The character to display on the screem.
 */
static inline void _vesaPrintChar(vesa_controler_t* pDriverCtrl,
                                  const char        kCharacter);

/**
 * @brief Processes the character in parameters.
 *
 * @param[in, out] pDriverCtrl The VESA driver controler to use.
 * @details Check the character nature and code. Corresponding to the
 * character's code, an action is taken. A regular character will be printed
 * whereas \\n will create a line feed.
 *
 * @param[in] kCharacter The character to process.
 */
static void _vesaProcessChar(void* pDriverCtrl, const char kCharacter);

/**
 * @brief Clears the screen by printing null character character on black
 * background.
 *
 * @param[in, out] pDriverCtrl The VESA driver controler to use.
 */
static void _vesaClearFramebuffer(void* pDriverCtrl);

/**
 * @brief Saves the cursor attributes in the buffer given as parameter.
 *
 * @details Fills the buffer given as parrameter with the current cursor
 * settings.
 *
 * @param[in, out] pDriverCtrl The VESA driver controler to use.
 * @param[out] pBuffer The cursor buffer in which the current cursor
 * position is going to be saved.
 */
static void _vesaSaveCursor(void* pDriverCtrl, cursor_t* pBuffer);

/**
 * @brief Restores the cursor attributes from the buffer given as parameter.
 *
 * @details The function will restores the cursor attributes from the buffer
 * given as parameter.
 *
 * @param[in, out] pDriverCtrl The VESA driver controler to use.
 * @param[in] kpBuffer The cursor buffer containing the new
 * coordinates of the cursor.
 */
static void _vesaRestoreCursor(void* pDriverCtrl, const cursor_t* kpBuffer);


/**
 * @brief Scrolls in the desired direction of lines_count lines.
 *
 * @details The function will scroll of lines_count line in the desired
 * direction.
 *
 * @param[in, out] pCtrl The VESA driver controler to use.
 * @param[in] kDirection The direction to whoch the console
 * should be scrolled.
 * @param[in] kLines The number of lines to scroll.
 */
static void _vesaScrollSafe(vesa_controler_t*        pCtrl,
                            const SCROLL_DIRECTION_E kDirection,
                            const uint32_t           kLines);

/**
 * @brief Scrolls in the desired direction of lines_count lines.
 *
 * @details The function will scroll of lines_count line in the desired
 * direction.
 *
 * @param[in, out] pDriverCtrl The VESA driver controler to use.
 * @param[in] kDirection The direction to whoch the console
 * should be scrolled.
 * @param[in] kLines The number of lines to scroll.
 */
static void _vesaScroll(void*                    pDriverCtrl,
                        const SCROLL_DIRECTION_E kDirection,
                        const uint32_t           kLines);


/**
 * @brief Sets the color scheme of the screen.
 *
 * @details Replaces the curent color scheme used t output data with the new
 * one given as parameter.
 *
 * @param[in, out] pDriverCtrl The VESA driver controler to use.
 * @param[in] kpColorScheme The new color scheme to apply to
 * the screen console.
 */
static void _vesaSetScheme(void*                pDriverCtrl,
                           const colorscheme_t* kpColorScheme);


/**
 * @brief Saves the color scheme in the buffer given as parameter.
 *
 * @details Fills the buffer given as parameter with the current screen's
 * color scheme value.
 *
 * @param[in, out] pDriverCtrl The VESA driver controler to use.
 * @param[out] pBuffer The buffer that will receive the current
 * color scheme used by the screen console.
 */
static void _vesaSaveScheme(void* pDriverCtrl, colorscheme_t* pBuffer);

/**
 * @brief Places the cursor to the selected coordinates given as parameters.
 *
 * @details Places the screen cursor to the coordinated described with the
 * parameters. The function will check the boundaries or the position parameters
 * before setting the cursor position.
 *
 * @param[in, out] pDriverCtrl The VESA driver controler to use.
 * @param[in] kLine The line index where to place the cursor.
 * @param[in] column The column index where to place the cursor.
 */
static inline void _vesaPutCursorSafe(vesa_controler_t* pCtrl,
                                      const uint32_t    kLine,
                                      const uint32_t    kColumn);

/**
 * @brief Moves the cursor forward on the screen.
 *
 * @brief Moves the cursor forward on the screen. If the cursor reaches the
 * limit of the screen, it is not moved anymore.
 *
 * @param[in] pCtrl The VESA controller to use.
 * @param[in] kCount The number of times the cursor must be moved.
 */
static inline void _vesaCursorForward(vesa_controler_t* pCtrl,
                                      const uint32_t kCount);

/**
 * @brief Moves the cursor backward on the screen.
 *
 * @brief Moves the cursor backward on the screen. If the cursor reaches the
 * beginning of the screen, it is not moved anymore.
 *
 * @param[in] pCtrl The VESA controller to use.
 * @param[in] kCount The number of times the cursor must be moved.
 */
static inline void _vesaCursorBackward(vesa_controler_t* pCtrl,
                                       const uint32_t kCount);

/**
 * @brief Prints the cursor on the screen.
 *
 * @details Prints the cursor on the screen at its current position in the
 * controller.
 *
 * @param[in] pCtrl The VESA controller to use.
 */
static inline void _vesaPrintCursor(vesa_controler_t* pCtrl);

/**
 * @brief Clears the cursor on the screen.
 *
 * @details Clears the cursor on the screen at its current position in the
 * controller.
 *
 * @param[in] pCtrl The VESA controller to use.
 */
static inline void _vesaClearCursor(vesa_controler_t* pCtrl);

/**
 * @brief Prints a pixel to the buffer.
 *
 * @details Prints a pixel to the buffer. This function manages the
 * transformation from the 32bits color pixel to the color depth used by
 * the driver.
 *
 * @param[in] pCtrl The VESA controller do use.
 * @param[in] kX The position on the horizontal axis where to put the pixel.
 * @param[in] kY The position on the vertical axis where to put the pixel.
 * @param[in] kRGBPixel The 32bits color pixel to print.
 */
static inline void _vesaPutPixel(vesa_controler_t* pCtrl,
                                 const uint32_t    kX,
                                 const uint32_t    kY,
                                 const uint32_t    kRGBPixel);

/**
 * @brief Draws a 32bits color pixel to the graphics controller.
 *
 * @details Draws a 32bits color pixel to the graphics controller. The driver
 * handles the overflow and might not print the pixel if the parameters are
 * invalid.
 *
 * @param[in] pDriverCtrl The VESA controller do use.
 * @param[in] kX The horizontal position where to draw the pixel.
 * @param[in] kY The vertical position where to draw the pixel.
 * @param[in] kRGBPixel The 32bits color pixel to draw.
 *
 * @return The success or error status is returned.
 */
static OS_RETURN_E _vesaDrawPixel(void*          pCtrl,
                                  const uint32_t kX,
                                  const uint32_t kY,
                                  const uint32_t kRGBPixel);

/**
 * @brief Draws a 32bits color rectangle to the graphics controller.
 *
 * @details Draws a 32bits color rectangle to the graphics controller. The
 * driver handles the overflow and might not print the rectangle if the
 * parameters are invalid.
 *
 * @param[in] pDriverCtrl The VESA controller do use.
 * @param[in] kpRect The rectangle to draw.
 *
 * @return The success or error status is returned.
 */
static OS_RETURN_E _vesaDrawRectangle(void*               pCtrl,
                                      const graph_rect_t* kpRect);

/**
 * @brief Draws a 32bits color line to the graphics controller.
 *
 * @details Draws a 32bits color line to the graphics controller. The
 * driver handles the overflow and might not print the line if the
 * parameters are invalid.
 *
 * @param[in] pDriverCtrl The VESA controller do use.
 * @param[in] kpLine The line to draw.
 *
 * @return The success or error status is returned.
 */
static OS_RETURN_E _vesaDrawLine(void*               pCtrl,
                                 const graph_line_t* kpLine);

/**
 * @brief Draws a 32bits color bitmap to the graphics controller.
 *
 * @details Draws a 32bits color bitmap to the graphics controller. The driver
 * handles the overflow and might not print the bitmap if the parameters are
 * invalid.
 *
 * @param[in] pDriverCtrl The VESA controller do use.
 * @param[in] kpBitmap The bitmap to draw.
 *
 * @return The success or error status is returned.
 */
static OS_RETURN_E _vesaDrawBitmap(void*                 pCtrl,
                                   const graph_bitmap_t* kpBitmap);

/**
 * @brief Flushes the VESA back buffer to the VESA framebuffer
 *
 * @details Flushes the VESA back buffer to the VESA framebuffer. No concurrency
 * management is done with the VESA display thread. This function is to use
 * when the thread is not running.
 *
 * @param[in] pDriverCtrl The VESA controller do use.
 *
 * @warning No concurrency management is done with the VESA display thread. This
 * function is to use when the thread is not running.
 */
static void _vesaFlush(void* pDriverCtrl);

/**
 * @brief VESA VFS open hook.
 *
 * @details VESA VFS open hook. This function returns a handle to control the
 * VESA driver through VFS.
 *
 * @param[in, out] pDrvCtrl The VESA driver that was registered in the VFS.
 * @param[in] kpPath The path in the VESA driver mount point.
 * @param[in] flags The open flags, must be O_RDWR.
 * @param[in] mode Unused.
 *
 * @return The function returns an internal handle used by the driver during
 * file operations.
 */
static void* _vesaVfsOpen(void*       pDrvCtrl,
                          const char* kpPath,
                          int         flags,
                          int         mode);

/**
 * @brief VESA VFS close hook.
 *
 * @details VESA VFS close hook. This function closes a handle that was created
 * when calling the open function.
 *
 * @param[in, out] pDrvCtrl The VESA driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static int32_t _vesaVfsClose(void* pDrvCtrl, void* pHandle);

/**
 * @brief VESA VFS write hook.
 *
 * @details VESA VFS write hook. This function writes a string to the VESA
 * framebuffer.
 *
 * @param[in, out] pDrvCtrl The VESA driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] kpBuffer The buffer that contains the string to write.
 * @param[in] count The number of bytes of the string to write.
 *
 * @return The function returns the number of bytes written or -1 on error;
 */
static ssize_t _vesaVfsWrite(void*       pDrvCtrl,
                             void*       pHandle,
                             const void* kpBuffer,
                             size_t      count);

/**
 * @brief VESA VFS IOCTL hook.
 *
 * @details VESA VFS IOCTL hook. This function performs the IOCTL for the VESA
 * driver.
 *
 * @param[in, out] pDrvCtrl The VESA driver that was registered in the VFS.
 * @param[in] pHandle The handle that was created when calling the open
 * function.
 * @param[in] operation The operation to perform.
 * @param[in, out] pArgs The arguments for the IOCTL operation.
 *
 * @return The function returns 0 on success and -1 on error;
 */
static ssize_t _vesaVfsIOCTL(void*    pDriverData,
                             void*    pHandle,
                             uint32_t operation,
                             void*    pArgs);

/**
 * @brief VESA fast fill function.
 *
 * @details VESA fast fill function. Using SSE instructions to speedup the
 * filling of the back buffer.
 *
 * @param[in] bufferAddr The start address of the region of the buffer to fill.
 * @param[in] kPixel The color pixel to fill.
 * @param[in] pixelCount The amount of pixel the fill.
 */
static inline void _vesaFastFill(uintptr_t      bufferAddr,
                                 const uint32_t kPixel,
                                 uint32_t       pixelCount);

/**
 * @brief VESA fast fill function.
 *
 * @details VESA fast fill function. Using SSE instructions to speedup the
 * filling of the back buffer.
 *
 * @param[out] pDest The start address of the region of the buffer to fill.
 * @param[in] kpSrc The start address of the source region to copy.
 * @param[in] sizeThe The size in bytes to copy.
 */
static inline void _vesaFastMemcpy(void*       pDest,
                                   const void* kpSrc,
                                   size_t      size);

/**
 * @brief VESA fast copy from the back buffer to the framebuffer.
 *
 * @details VESA fast copy from the back buffer to the framebuffer. Using SSE
 * instruction to speedup the copy.
 *
 * @param[in] pCtrl The controller to use for the copy.
 * @param[in] frameBufferAddr The start address of the framebuffer to copy.
 * @param[in] videoBufferAddr The start address of the back buffer to copy.
 * @param[in] lineCount The number of lines of pixel to copy.
 */
static inline void _vesaFastToFramebuffer(vesa_controler_t* pCtrl,
                                          uintptr_t         frameBufferAddr,
                                          uintptr_t         videoBufferAddr,
                                          size_t            lineCount);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief VESA driver instance. */
static driver_t sX86VESADriver = {
    .pName         = "X86 VESA driver",
    .pDescription  = "X86 VESA driver for roOs",
    .pCompatible   = "x86,x86-vesa",
    .pVersion      = "2.0",
    .pDriverAttach = _vesaDriverAttach
};

/** @brief VGA color to RGB translation table. */
static const uint32_t vgaColorTable[16] = {
    0xFF000000,
    0xFF0000AA,
    0xFF00AA00,
    0xFF00AAAA,
    0xFFAA0000,
    0xFFAA00AA,
    0xFFAA5500,
    0xFFAAAAAA,
    0xFF555555,
    0xFF5555FF,
    0xFF55FF55,
    0xFF55FFFF,
    0xFFFF5555,
    0xFFFF55FF,
    0xFFFFFF55,
    0xFFFFFFFF
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/
static inline void _vesaFastFill(uintptr_t      bufferAddr,
                                 const uint32_t kPixel,
                                 uint32_t       pixelCount)
{
    uint32_t replicateValue[4] __attribute__((aligned(16)));

    /* Compute the replicated value */
    replicateValue[0] = kPixel;
    replicateValue[1] = kPixel;
    replicateValue[2] = kPixel;
    replicateValue[3] = kPixel;

    register size_t sseSize;
    register size_t n;
    register char*  pDestPtr;

    pDestPtr = (char*)bufferAddr;

    /* First unaligned */
    while(((uintptr_t)pDestPtr & 0xF) != 0 && pixelCount > 0)
    {
        *(uint32_t*)pDestPtr = *(uint32_t*)replicateValue;
        pDestPtr += sizeof(uint32_t);
        --pixelCount;
    }

    sseSize = pixelCount / 4;

    if(sseSize > 0)
    {
        /* Aligned */
        __asm__ __volatile__ ("movups (%0), %%xmm7\n\t"
                              :
                              : "r"(replicateValue)
                              : "memory");

        pixelCount -= sseSize * 4;
        n = (sseSize + 15) / 16;
        switch (sseSize % 16)
        {
            case 0:
                do {
                    __asm__ __volatile__ ("movntdq %%xmm7, (%0)\n\t"
                              :
                              : "r"(pDestPtr)
                              : "memory");
                    pDestPtr += 16;
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(15)
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(14)
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(13)
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(12)
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(11)
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(10)
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(9)
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(8)
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(7)
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(6)
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(5)
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(4)
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(3)
            __attribute__ ((fallthrough));
            VESA_FAST_FILL_UNROLL_ACTION(2)
            __attribute__ ((fallthrough));
            case 1:
                    __asm__ __volatile__ ("movntdq %%xmm7, (%0)\n\t"
                              :
                              : "r"(pDestPtr)
                              : "memory");
                     pDestPtr += 16;
                } while (--n > 0);
        }
    }

    /* Last unaligned */
    while(pixelCount > 0)
    {
        *(uint32_t*)pDestPtr = *(uint32_t*)replicateValue;
        pDestPtr += sizeof(uint32_t);
        --pixelCount;
    }
}

static inline void _vesaFastMemcpy(void*       pDest,
                                   const void* kpSrc,
                                   size_t      size)
{
    register const char* pSrcPtr;
    register char*       pDstPtr;
    register size_t      sseSize;
    register size_t      n;

    pSrcPtr = kpSrc;
    pDstPtr = pDest;

    /* If not the same alignement, we will never be able to align, use memcpy */
    if(((uintptr_t)pSrcPtr & 0xF) != ((uintptr_t)pDstPtr & 0xF) || size <= 20)
    {
        memcpy(pDstPtr, pSrcPtr, size);
        return;
    }

    /* First unaligned */
    while(((uintptr_t)pSrcPtr & 0xF) != 0 && size > 0)
    {
        *(uint32_t*)pDstPtr = *(uint32_t*)pSrcPtr;
        pSrcPtr += 4;
        pDstPtr += 4;
        size -= 4;
    }

    sseSize = size / (sizeof(uint64_t) * 2);

    if(sseSize > 0)
    {
        size -= sseSize * (sizeof(uint64_t) * 2);
        n = (sseSize + 31) / 32;
        switch (sseSize % 32)
        {
            case 0:
                do {
                    __asm__ __volatile__ ("movups (%0), %%xmm7\n\t"
                                          "movntdq %%xmm7, (%1)\n\t"
                                          :
                                          :"r"(pSrcPtr), "r"(pDstPtr)
                                          : "memory");
                    pDstPtr += 16;
                    pSrcPtr += 16;
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(31)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(30)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(29)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(28)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(27)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(26)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(25)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(24)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(23)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(22)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(21)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(20)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(19)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(18)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(17)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(16)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(15)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(14)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(13)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(12)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(11)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(10)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(9)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(8)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(7)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(6)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(5)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(4)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(3)
            __attribute__ ((fallthrough));
            VESA_FAST_CPY_UNROLL_ACTION(2)
            __attribute__ ((fallthrough));
            case 1:
                    __asm__ __volatile__ ("movups (%0), %%xmm7\n\t"
                                          "movntdq %%xmm7, (%1)\n\t"
                                          :
                                          :"r"(pSrcPtr), "r"(pDstPtr)
                                          : "memory");
                    pDstPtr += 16;
                    pSrcPtr += 16;
                } while (--n > 0);
        }
    }

    /* Last unaligned */
    while(size > 0)
    {
        *(uint32_t*)pDstPtr = *(uint32_t*)pSrcPtr;
        pSrcPtr += 4;
        pDstPtr += 4;
        size -= 4;
    }
}

static inline void _vesaFastToFramebuffer(vesa_controler_t* pCtrl,
                                          uintptr_t         frameBufferAddr,
                                          uintptr_t         videoBufferAddr,
                                          size_t            lineCount)
{
    size_t   i;
    size_t   lineStride;
    size_t   scanLineSize;
    uint32_t pixel;
    uint8_t  bpp;

    bpp = pCtrl->pCurrentMode->bpp;
    lineStride = pCtrl->pCurrentMode->width * sizeof(uint32_t);
    scanLineSize = pCtrl->pCurrentMode->bytePerScanLine;
    while(lineCount > 0)
    {
        switch(bpp)
        {
            case 32:
                _vesaFastMemcpy((void*)frameBufferAddr,
                                (void*)videoBufferAddr,
                                lineStride);
                break;
            case 24:
                for(i = 0; i < pCtrl->pCurrentMode->width; ++i)
                {
                    pixel = *(((uint32_t*)videoBufferAddr) + i);

                    *(((uint16_t*)frameBufferAddr) + i) = (uint16_t)pixel;
                    *(((uint8_t*)frameBufferAddr) + i * 2 + 1) =
                                                        (uint8_t)(pixel >> 16);
                }
                break;
            case 16:
                for(i = 0; i < pCtrl->pCurrentMode->width; ++i)
                {
                    pixel = *(((uint32_t*)videoBufferAddr) + i);
                    pixel = (pixel & 0xFF) >> 3         |
                            ((pixel >> 8) & 0xFF) >> 3  |
                            ((pixel >> 16) & 0xFF) >> 3;

                    *(((uint16_t*)frameBufferAddr) + i) = (uint16_t)pixel;
                }
                break;
            case 8:
                for(i = 0; i < pCtrl->pCurrentMode->width; ++i)
                {
                    pixel = *(((uint32_t*)videoBufferAddr) + i);
                    pixel = (pixel & 0xFF) >> 6         |
                            ((pixel >> 8) & 0xFF) >> 5  |
                            ((pixel >> 16) & 0xFF) >> 5;

                    *(((uint8_t*)frameBufferAddr) + i) = (uint8_t)pixel;
                }
                break;
            default:
                /* Do nothing, we do not support this mode */
                break;
        }
        frameBufferAddr += scanLineSize;
        videoBufferAddr += lineStride;

        --lineCount;
    }
}

static OS_RETURN_E _vesaDriverAttach(const fdt_node_t* pkFdtNode)
{
    const uint32_t*   kpUintProp;
    const char*       kpStrProp;
    size_t            propLen;
    OS_RETURN_E       retCode;
    OS_RETURN_E       error;
    vesa_controler_t* pDrvCtrl;
    colorscheme_t     initScheme;
    vbe_mode_t*       pModeNode;
    vbe_mode_t*       pSaveNode;
    uint32_t          width;
    uint32_t          height;
    uint32_t          depth;

    /* Init structures */
    pDrvCtrl = kmalloc(sizeof(vesa_controler_t));
    if(pDrvCtrl == NULL)
    {
        retCode = OS_ERR_NO_MORE_MEMORY;
        goto ATTACH_END;
    }
    memset(pDrvCtrl, 0, sizeof(vesa_controler_t));
    pDrvCtrl->vfsDriver = VFS_DRIVER_INVALID;

    /* Get the resolution */
    kpUintProp = fdtGetProp(pkFdtNode, VESA_FDT_RES_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    width  = FDTTOCPU32(*kpUintProp);
    height = FDTTOCPU32(*(kpUintProp + 1));

    /* Get the color depth */
    kpUintProp = fdtGetProp(pkFdtNode, VESA_FDT_DEPTH_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    depth  = FDTTOCPU32(*kpUintProp);

    kpUintProp = fdtGetProp(pkFdtNode, VESA_FDT_REFRESH_PROP, &propLen);
    if(kpUintProp == NULL || propLen != sizeof(uint32_t))
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    pDrvCtrl->refreshRate  = FDTTOCPU32(*kpUintProp);

#if VESA_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Resolution: %dx%d | Depth: %d | Rate %dHz",
           width,
           height,
           depth,
           pDrvCtrl->refreshRate);
#endif

    /* Get the VESA modes */
    retCode = _vesaGetVBEInfo(pDrvCtrl);
    if(retCode != OS_NO_ERR)
    {
        goto ATTACH_END;
    }
    retCode = _vesaGetAvailableModes(pDrvCtrl);
    if(retCode != OS_NO_ERR)
    {
        goto ATTACH_END;
    }

    /* Apply current mode */
    retCode = _vesaSetGraphicMode(width,
                                  height,
                                  depth,
                                  pDrvCtrl->refreshRate,
                                  pDrvCtrl);
    if(retCode != OS_NO_ERR)
    {
        goto ATTACH_END;
    }

    /* Create the display thread */
    retCode = schedCreateKernelThread(&pDrvCtrl->pDisplayThread,
                                      VESA_DISPLAY_THREAD_PRIO,
                                      VESA_DISPLAY_THREAD_NAME,
                                      VESA_DISPLAY_THREAD_STACK_SIZE,
                                      VESA_DISPLAY_THREAD_AFFINITY,
                                      _vesaDisplayRoutine,
                                      (void*)pDrvCtrl);
    if(retCode != OS_NO_ERR)
    {
        goto ATTACH_END;
    }
    /* Set initial scheme */
    initScheme.background = BG_BLACK;
    initScheme.foreground = FG_WHITE;
    _vesaSetScheme(pDrvCtrl, &initScheme);

    /* Get the device path */
    kpStrProp = fdtGetProp(pkFdtNode, VESA_FDT_DEVICE_PROP, &propLen);
    if(kpStrProp == NULL || propLen  == 0)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

    /* Register the driver */
    pDrvCtrl->vfsDriver = vfsRegisterDriver(kpStrProp,
                                            pDrvCtrl,
                                            _vesaVfsOpen,
                                            _vesaVfsClose,
                                            NULL,
                                            _vesaVfsWrite,
                                            NULL,
                                            _vesaVfsIOCTL);
    if(pDrvCtrl->vfsDriver == VFS_DRIVER_INVALID)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

#if VESA_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "VESA driver initialized");
#endif

ATTACH_END:
    if(retCode != OS_NO_ERR)
    {
        if(pDrvCtrl != NULL)
        {
            /* Free the modes */
            pModeNode = pDrvCtrl->pVbeModes;
            while(pModeNode != NULL)
            {
                pSaveNode = pModeNode;
                pModeNode = pModeNode->pNext;
                kfree(pSaveNode);
            }

            /* Free the buffers if needed */
            if(pDrvCtrl->videoBuffer.pFramebuffer != NULL)
            {
                pDrvCtrl->videoBuffer.pFramebuffer =
                    (void*)((uintptr_t)pDrvCtrl->videoBuffer.pFramebuffer &
                            ~PAGE_SIZE_MASK);
                error = memoryKernelUnmap(pDrvCtrl->videoBuffer.pFramebuffer,
                                    pDrvCtrl->videoBuffer.hwFramebufferSize);
                if(error != OS_NO_ERR)
                {
                    PANIC(error, MODULE_NAME, "Failed to unmap memory");
                }
            }
            if(pDrvCtrl->videoBuffer.pBack != NULL)
            {
                error = memoryKernelUnmap(pDrvCtrl->videoBuffer.pBack,
                                          pDrvCtrl->videoBuffer.backBufferSize);
                if(error != OS_NO_ERR)
                {
                    PANIC(error, MODULE_NAME, "Failed to unmap memory");
                }
            }
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

static OS_RETURN_E _vesaGetVBEInfo(vesa_controler_t* pDrvCtrl)
{
    bios_int_regs_t biosRegs;
    size_t          toMap;
    uintptr_t       addrToMap;
    uint32_t        offset;
    char*           oemData;
    OS_RETURN_E     error;
    uint32_t        initLoc;

    /* Make the BIOS call to get the VBE information */
    biosRegs.ax = VESA_BIOS_CALL_GET_INFO_ID;
    biosRegs.bx = 0;
    biosRegs.cx = 0;
    biosRegs.dx = 0;
    memset(&pDrvCtrl->vbeInfo, 0, sizeof(vbe_info_t));
    memcpy(pDrvCtrl->vbeInfo.pSignature, "VBE2", 4);
    cpuBiosCall(&biosRegs,
                VESA_BIOS_CALL_INT,
                &pDrvCtrl->vbeInfo,
                sizeof(vbe_info_t),
                &initLoc);

    /* Check return value */
    if(biosRegs.ax != VESA_BIOS_CALL_RETURN_OK ||
       strncmp(pDrvCtrl->vbeInfo.pSignature, "VESA", 4) != 0)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "VESA Bios call failed: 0x%x",
               biosRegs.ax);
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Check compatibility */
    if(pDrvCtrl->vbeInfo.version < 0x200)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "VESA VBE Version incompatible: 0x%x",
               pDrvCtrl->vbeInfo.version);
        return OS_ERR_INCORRECT_VALUE;
    }

    /* Get the address to map */
    toMap     = KERNEL_PAGE_SIZE;
    addrToMap = ((pDrvCtrl->vbeInfo.oem >> 16) << 4) |
                (pDrvCtrl->vbeInfo.oem & 0xFFFF);
    if(((addrToMap + VESA_OEM_DATA_SIZE) & ~PAGE_SIZE_MASK) !=
       (addrToMap & ~PAGE_SIZE_MASK))
    {
        toMap += KERNEL_PAGE_SIZE;
    }
    addrToMap = addrToMap & ~PAGE_SIZE_MASK;

    oemData = memoryKernelMap((void*)addrToMap,
                              toMap,
                              MEMMGR_MAP_RO       |
                              MEMMGR_MAP_KERNEL   |
                              MEMMGR_MAP_HARDWARE,
                              &error);
    if(error != OS_NO_ERR || oemData == NULL)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to map OEM data",
               error);
        return error;
    }

    /* Copy the data to the OEM data */
    offset = pDrvCtrl->vbeInfo.oem & PAGE_SIZE_MASK;
    memcpy(pDrvCtrl->vbeInfo.oemData, oemData + offset, VESA_OEM_DATA_SIZE);

    /* Unmap memory */
    error = memoryKernelUnmap(oemData, toMap);
    if(error != OS_NO_ERR)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to unmap OEM data",
               error);
        return error;
    }

    /* Update pointer to offsets */
    pDrvCtrl->vbeInfo.productRev -= pDrvCtrl->vbeInfo.oem;
    pDrvCtrl->vbeInfo.productName -= pDrvCtrl->vbeInfo.oem;
    pDrvCtrl->vbeInfo.vendor -= pDrvCtrl->vbeInfo.oem;
    pDrvCtrl->vbeInfo.oem = 0;
    pDrvCtrl->vbeInfo.videoModes = pDrvCtrl->vbeInfo.videoModes - initLoc;

#if VESA_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "VESA Get Info Table (initial location 0x%x):\n"
           "\tSignature: %c%c%c%c\n"
           "\tVersion: 0x%x\n"
           "\tOEM: %s\n"
           "\tCapabilities: 0x%x\n"
           "\tVideo Modes Ptr: 0x%p\n"
           "\tTotal Memory: 0x%x\n"
           "\tSoftware Rev.: %d\n"
           "\tVendor: %s\n"
           "\tProduct Name: %s\n"
           "\tProvuct Rev.: %s",
           initLoc,
           pDrvCtrl->vbeInfo.pSignature[0],
           pDrvCtrl->vbeInfo.pSignature[1],
           pDrvCtrl->vbeInfo.pSignature[2],
           pDrvCtrl->vbeInfo.pSignature[3],
           pDrvCtrl->vbeInfo.version,
           &pDrvCtrl->vbeInfo.oemData[pDrvCtrl->vbeInfo.oem],
           pDrvCtrl->vbeInfo.capabilities,
           pDrvCtrl->vbeInfo.videoModes,
           pDrvCtrl->vbeInfo.totalMemory,
           pDrvCtrl->vbeInfo.softwareRev,
           &pDrvCtrl->vbeInfo.oemData[pDrvCtrl->vbeInfo.vendor],
           &pDrvCtrl->vbeInfo.oemData[pDrvCtrl->vbeInfo.productName],
           &pDrvCtrl->vbeInfo.oemData[pDrvCtrl->vbeInfo.productRev]);
#endif

    return OS_NO_ERR;
}

static OS_RETURN_E _vesaGetAvailableModes(vesa_controler_t* pDrvCtrl)
{
    uint16_t*       pModeId;
    uint16_t        i;
    bios_int_regs_t biosRegs;
    vbe_mode_info_t modeInfo;
    uint32_t        initLoc;
    vbe_mode_t*     pNewMode;
    vbe_mode_t*     pSaveNode;
    OS_RETURN_E     error;

    /* Get the first mode ID in the structure */
    pModeId = (uint16_t*)((uintptr_t)&pDrvCtrl->vbeInfo +
                          pDrvCtrl->vbeInfo.videoModes);

    error = OS_NO_ERR;

    /* Loop through the modes */
    for(i = 0;
        (void*)&pModeId[i] < (void*)pDrvCtrl->vbeInfo.oemData &&
        pModeId[i] != 0xFFFF;
        ++i)
    {
#if VESA_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "Processing Video Mode 0x%x",
               pModeId[i]);
#endif

        /* Prepare BIOS call */
        biosRegs.ax = VESA_BIOS_CALL_GET_MODE_ID;
        biosRegs.bx = 0;
        biosRegs.cx = pModeId[i];
        biosRegs.dx = 0;
        memset(&modeInfo, 0, sizeof(vbe_mode_info_t));
        cpuBiosCall(&biosRegs,
                    VESA_BIOS_CALL_INT,
                    &modeInfo,
                    sizeof(vbe_mode_info_t),
                    &initLoc);

        /* Check return value */
        if(biosRegs.ax != VESA_BIOS_CALL_RETURN_OK)
        {
            syslog(SYSLOG_LEVEL_ERROR,
                   MODULE_NAME,
                   "Failed to get VESA mode information, error %x",
                   biosRegs.ax);
            error = OS_ERR_INCORRECT_VALUE;
            goto MODE_DISCOVERY_END;
        }

        /* Check support */
        if((modeInfo.attributes & VESA_ATTRIBUTE_SUPPORTED) !=
           VESA_ATTRIBUTE_SUPPORTED)
        {

#if VESA_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Not supported, skipping");
#endif

            continue;
        }

        /* We only support linear buffer now */
        if((modeInfo.attributes & VESA_ATTRIBUTE_LINEAR_FB) !=
           VESA_ATTRIBUTE_LINEAR_FB)
        {

#if VESA_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Not linear, skipping");
#endif

            continue;
        }

        /* We only support direct color mode or packed */
        if(modeInfo.memoryModel != VESA_MEMORY_MODEL_PACKED &&
           modeInfo.memoryModel != VESA_MEMORY_MODEL_DIRECTCOLOR)
        {

#if VESA_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Unsupported memory model skipping");
#endif

            continue;
        }

        /* Allocate the new mode */
        pNewMode = kmalloc(sizeof(vbe_mode_t));
        if(pNewMode == NULL)
        {
            error = OS_ERR_NO_MORE_MEMORY;
            goto MODE_DISCOVERY_END;
        }

        /* Setup the mode */
        pNewMode->id              = pModeId[i];
        pNewMode->width           = modeInfo.width;
        pNewMode->height          = modeInfo.height;
        pNewMode->bpp             = modeInfo.bpp;
        pNewMode->bytePerScanLine = modeInfo.bytesPerScanLine;

        pNewMode->framebuffer = (void*)(uintptr_t)modeInfo.framebuffer;

#if VESA_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "==========> Supported mode 0x%p:\n"
               "\tResolution: %dx%d\n"
               "\tColor depth: %dbpp\n"
               "\tFramebuffer: 0x%p",
               pNewMode->id,
               pNewMode->width,
               pNewMode->height,
               pNewMode->bpp,
               pNewMode->framebuffer);
#endif

        /* Link node */
        pNewMode->pNext = pDrvCtrl->pVbeModes;
        pDrvCtrl->pVbeModes = pNewMode;
    }

MODE_DISCOVERY_END:
    if(error != OS_NO_ERR)
    {
        /* Free the modes */
        pNewMode = pDrvCtrl->pVbeModes;
        while(pNewMode != NULL)
        {
            pSaveNode = pNewMode;
            pNewMode = pNewMode->pNext;
            kfree(pSaveNode);
        }
        pDrvCtrl->pVbeModes = NULL;
    }
    return error;
}

static OS_RETURN_E _vesaSetGraphicMode(const uint32_t kWidth,
                                       const uint32_t kHeight,
                                       const uint32_t kDepth,
                                       const uint8_t  kRate,
                                       void*          pDriverCtrl)
{
    vbe_mode_t*       pMode;
    vesa_controler_t* pCtrl;
    bios_int_regs_t   biosRegs;
    OS_RETURN_E       error;
    OS_RETURN_E       retCode;
    size_t            newBufferSize;
    size_t            newBackBufferSize;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    /* Try to find a match for the modes */
    pMode = pCtrl->pVbeModes;
    while(pMode != NULL)
    {
        /* Compare and stop on success */
        if(pMode->width == kWidth &&
           pMode->height == kHeight &&
           pMode->bpp == kDepth)
        {
            break;
        }

        pMode = pMode->pNext;
    }

    /* Check if we found the mode */
    if(pMode == NULL)
    {
        syslog(SYSLOG_LEVEL_ERROR, MODULE_NAME, "VESA mode not supported");
        return OS_ERR_NOT_SUPPORTED;
    }

    /* Check if we have something to do */
    if(pMode == pCtrl->pCurrentMode)
    {
        /* Just update the rate */
        pCtrl->refreshRate = kRate;
        return OS_NO_ERR;
    }

    /* Setup the new framebuffer */
    newBufferSize = kHeight * pMode->bytePerScanLine;
    newBufferSize = (newBufferSize + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;
    pCtrl->videoBuffer.hwFramebufferSize = newBufferSize;
    pCtrl->videoBuffer.pFramebuffer = memoryKernelMap(pMode->framebuffer,
                                                      newBufferSize,
                                                      MEMMGR_MAP_HARDWARE |
                                                      MEMMGR_MAP_KERNEL   |
                                                      MEMMGR_MAP_RW |
                                                      MEMMGR_MAP_WRITE_COMBINING,
                                                      &error);
    pCtrl->videoBuffer.pFramebuffer =
                            (void*)((uintptr_t)pCtrl->videoBuffer.pFramebuffer +
                            ((uintptr_t)pMode->framebuffer &
                            PAGE_SIZE_MASK));
    if(error != OS_NO_ERR || pCtrl->videoBuffer.pFramebuffer == NULL)
    {
        return error;
    }

    /* Setup the new buffers */
    newBackBufferSize = kHeight * kWidth * sizeof(uint32_t);
    newBackBufferSize = (newBackBufferSize + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;
    pCtrl->videoBuffer.backBufferSize = newBackBufferSize;
    pCtrl->videoBuffer.pBack = memoryKernelAllocate(newBackBufferSize,
                                                    MEMMGR_MAP_KERNEL |
                                                    MEMMGR_MAP_RW,
                                                    &error);
    if(error != OS_NO_ERR || pCtrl->videoBuffer.pBack == NULL)
    {
        /* Set back old buffers */
        pCtrl->videoBuffer.pFramebuffer = (void*)
                              ((uintptr_t)pCtrl->videoBuffer.pFramebuffer &
                               ~PAGE_SIZE_MASK);
        retCode = memoryKernelUnmap(pCtrl->videoBuffer.pFramebuffer,
                                    newBufferSize);
        if(retCode != OS_NO_ERR)
        {
            PANIC(retCode, MODULE_NAME, "Failed to unmap memory");
        }

        return error;
    }

    /* Now set the mode */
    biosRegs.ax = VESA_BIOS_CALL_SET_MODE;
    biosRegs.bx = pMode->id | VESA_FLAG_LINEAR_FB_ENABLE;
    biosRegs.cx = 0;
    biosRegs.dx = 0;
    cpuBiosCall(&biosRegs, VESA_BIOS_CALL_INT, NULL, 0, NULL);

    /* Check the result */
    if(biosRegs.ax != VESA_BIOS_CALL_RETURN_OK)
    {
        /* Set back old buffers */
        pCtrl->videoBuffer.pFramebuffer = (void*)
                              ((uintptr_t)pCtrl->videoBuffer.pFramebuffer &
                               ~PAGE_SIZE_MASK);
        error = memoryKernelUnmap(pCtrl->videoBuffer.pFramebuffer,
                                  newBufferSize);
        if(error != OS_NO_ERR)
        {
            PANIC(error, MODULE_NAME, "Failed to unmap memory");
        }
        error = memoryKernelUnmap(pCtrl->videoBuffer.pBack, newBackBufferSize);
        if(error != OS_NO_ERR)
        {
            PANIC(error, MODULE_NAME, "Failed to unmap memory");
        }
        return OS_ERR_INCORRECT_VALUE;
    }

    memset(pCtrl->videoBuffer.pBack, 0, newBackBufferSize);

#if VESA_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Updated VESA mode to %dx%x %dbpp | Refresh rate %dHz",
           kWidth,
           kHeight,
           kDepth,
           pCtrl->refreshRate);
#endif

    /* Update the screen value */
    pCtrl->lineCount    = kHeight / VESA_TEXT_CHAR_HEIGHT;
    pCtrl->columnCount  = kWidth / VESA_TEXT_CHAR_WIDTH;
    pCtrl->pCurrentMode = pMode;

    return OS_NO_ERR;
}

static void* _vesaDisplayRoutine(void* pDrvCtrl)
{
    vesa_controler_t* pCtrl;
    uint64_t          startTime;
    uint64_t          elapsed;
    uint64_t          period;

    pCtrl = GET_CONTROLER(pDrvCtrl);

    while(TRUE)
    {
        startTime = timeGetUptime();

        _vesaFlush(pDrvCtrl);

        /* Manage refresh rate */
        period  = 1000000000ULL / pCtrl->refreshRate;
        elapsed = timeGetUptime() - startTime;

        if(period > elapsed)
        {
            schedSleep(period - elapsed);
        }
    }

    return NULL;
}

static inline void _vesaPutPixel(vesa_controler_t* pCtrl,
                                 const uint32_t    kX,
                                 const uint32_t    kY,
                                 const uint32_t    kRGBPixel)
{
    void*  pBufferMem;
    size_t offset;

    /* Calculate the position based on the BPP and screen resolution */
    offset = kY * sizeof(uint32_t) *
             pCtrl->pCurrentMode->width +
             kX * sizeof(uint32_t);

    /* Check that we are in the bounds */
    offset = offset % pCtrl->videoBuffer.backBufferSize;
    pBufferMem = (void*)((uintptr_t)pCtrl->videoBuffer.pBack + offset);
    *(uint32_t*)pBufferMem = kRGBPixel;
}

static inline void _vesaPrintChar(vesa_controler_t* pCtrl,
                                  const char        kCharacter)
{
    uint32_t          x;
    uint32_t          y;
    uint32_t          cx;
    uint32_t          cy;
    const uint8_t*    pGlyph;
    uint32_t          pixel;
    uint32_t          mask[8] = {1, 2, 4, 8, 16, 32, 64, 128};

    x = pCtrl->screenCursor.x * VESA_TEXT_CHAR_WIDTH;
    y = pCtrl->screenCursor.y * VESA_TEXT_CHAR_HEIGHT;

    pGlyph = sVesaFontBitmap + (kCharacter - 31) * 16;
    for(cy = 0; cy < VESA_TEXT_CHAR_HEIGHT; ++cy)
    {
        for(cx = 0; cx < VESA_TEXT_CHAR_WIDTH; ++cx)
        {
            pixel = pGlyph[cy] & mask[cx] ?
                    vgaColorTable[pCtrl->screenScheme.foreground] :
                    vgaColorTable[pCtrl->screenScheme.background];
            _vesaPutPixel(pCtrl,
                          x + VESA_TEXT_CHAR_WIDTH - cx,
                          y + cy,
                          pixel);
        }
    }
}

static void _vesaProcessChar(void* pDriverCtrl, const char kCharacter)
{
    vesa_controler_t* pCtrl;
    uint32_t          i;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    _vesaClearCursor(pCtrl);

    /* If character is a normal ASCII character */
    if(kCharacter > 31 && kCharacter < 127)
    {
        /* Display character and move cursor */
        _vesaPrintChar(pCtrl, kCharacter);

        /* Scroll if we reached the end of the line */
        if(pCtrl->screenCursor.y >= pCtrl->lineCount)
        {
            _vesaScrollSafe(pCtrl, SCROLL_DOWN, 1);
        }
        else
        {
            /* Manage cursor position */
            _vesaCursorForward(pCtrl, 1);
        }
    }
    else
    {
        /* Manage special ACSII characters*/
        switch(kCharacter)
        {
            /* Backspace */
            case '\b':
                _vesaCursorBackward(pCtrl, 1);
                break;
            /* Tab */
            case '\t':
                for(i = 0; i < VESA_TAB_SIZE; ++i)
                {
                    /* Display character and move cursor */
                    _vesaPrintChar(pCtrl, ' ');

                    /* Scroll if we reached the end of the line */
                    if((uint8_t)pCtrl->screenCursor.y >= pCtrl->lineCount)
                    {
                        _vesaScrollSafe(pCtrl, SCROLL_DOWN, 1);
                    }
                    else
                    {
                        /* Manage cursor position */
                        _vesaCursorForward(pCtrl, 1);
                    }
                }
                break;
            /* Line feed */
            case '\n':
                if((uint8_t)pCtrl->screenCursor.y < pCtrl->lineCount - 1)
                {
                    _vesaPutCursorSafe(pCtrl,
                                       pCtrl->screenCursor.y + 1,
                                       0);
                }
                else
                {
                    _vesaScrollSafe(pCtrl, SCROLL_DOWN, 1);
                }
                break;
            /* Clear screen */
            case '\f':
                /* Clear all screen */
                _vesaClearFramebuffer(pCtrl);
                break;
            /* Line return */
            case '\r':
                _vesaPutCursorSafe(pCtrl, pCtrl->screenCursor.y, 0);
                break;
            /* Undefined */
            default:
                break;
        }
    }

    _vesaPrintCursor(pCtrl);
}

static void _vesaClearFramebuffer(void* pDriverCtrl)
{
    vesa_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    /* Clear all screen */
    _vesaFastFill((uintptr_t)pCtrl->videoBuffer.pBack,
                  0,
                  pCtrl->pCurrentMode->width * pCtrl->pCurrentMode->height);
}

static void _vesaSaveCursor(void* pDriverCtrl, cursor_t* pBuffer)
{
    vesa_controler_t* pCtrl;

    if(pBuffer != NULL)
    {
        pCtrl = GET_CONTROLER(pDriverCtrl);

        /* Save cursor attributes */
        pBuffer->x = pCtrl->screenCursor.x;
        pBuffer->y = pCtrl->screenCursor.y;
    }
}

static void _vesaRestoreCursor(void* pDriverCtrl, const cursor_t* kpBuffer)
{
    vesa_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    if(kpBuffer->x >= pCtrl->columnCount || kpBuffer->y >= pCtrl->lineCount)
    {
        return;
    }

    /* Restore cursor attributes */
    _vesaPutCursorSafe(pDriverCtrl, kpBuffer->y, kpBuffer->x);
}

static void _vesaScrollSafe(vesa_controler_t*        pCtrl,
                            const SCROLL_DIRECTION_E kDirection,
                            const uint32_t           kLines)
{
    uint32_t  toScroll;
    size_t    buffOffset;
    char*     pSource;
    char*     pDestination;

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
        pSource = pCtrl->videoBuffer.pBack;
        pDestination = pCtrl->videoBuffer.pBack;

        buffOffset = VESA_TEXT_CHAR_HEIGHT *
                     pCtrl->pCurrentMode->width *
                     toScroll *
                     sizeof(uint32_t);
        pSource += buffOffset;
        _vesaFastMemcpy(pDestination,
                        pSource,
                        pCtrl->videoBuffer.backBufferSize - buffOffset);

        pDestination += pCtrl->videoBuffer.backBufferSize - buffOffset;
        /* Clear read */
        _vesaFastFill((uintptr_t)pDestination,
                      0,
                      buffOffset / sizeof(uint32_t));

        /* Replace cursor */
        _vesaPutCursorSafe(pCtrl, pCtrl->lineCount - toScroll, 0);
    }
}

static void _vesaScroll(void*                    pDriverCtrl,
                        const SCROLL_DIRECTION_E kDirection,
                        const uint32_t           kLines)
{
    vesa_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    _vesaScrollSafe(pCtrl, kDirection, kLines);
}

static void _vesaSetScheme(void*                pDriverCtrl,
                           const colorscheme_t* kpColorScheme)
{
    vesa_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    pCtrl->screenScheme.foreground = kpColorScheme->foreground;
    pCtrl->screenScheme.background = kpColorScheme->background;
}

static void _vesaSaveScheme(void* pDriverCtrl, colorscheme_t* pBuffer)
{
    vesa_controler_t* pCtrl;


    if(pBuffer != NULL)
    {
        pCtrl = GET_CONTROLER(pDriverCtrl);

        /* Save color scheme into buffer */
        pBuffer->foreground = pCtrl->screenScheme.foreground;
        pBuffer->background = pCtrl->screenScheme.background;
    }
}

static inline void _vesaPutCursorSafe(vesa_controler_t* pCtrl,
                                      const uint32_t    kLine,
                                      const uint32_t    kColumn)
{
    /* Checks the values of line and column */
    if(kLine > pCtrl->lineCount ||
       kColumn > pCtrl->columnCount)
    {
        return;
    }

    /* Set new cursor position */
    pCtrl->screenCursor.x = kColumn;
    pCtrl->screenCursor.y = kLine;
}

static inline void _vesaCursorForward(vesa_controler_t* pCtrl,
                                      const uint32_t kCount)
{
    uint32_t i;

    for(i = 0; i < kCount; ++i)
    {
        /* Check if we can move backward on the same line */
        if(pCtrl->screenCursor.x < pCtrl->columnCount - 1)
        {
            ++pCtrl->screenCursor.x;
        }
        else if(pCtrl->screenCursor.y < pCtrl->lineCount - 1)
        {
            ++pCtrl->screenCursor.y;
            pCtrl->screenCursor.x = 0;
        }
        else
        {
            _vesaScrollSafe(pCtrl, SCROLL_DOWN, 1);
            pCtrl->screenCursor.x = 0;
        }
    }
}

static inline void _vesaCursorBackward(vesa_controler_t* pCtrl,
                                       const uint32_t kCount)
{
    uint32_t i;

    for(i = 0; i < kCount; ++i)
    {
        /* Check if we can move backward on the same line */
        if(pCtrl->screenCursor.x > 0)
        {
            --pCtrl->screenCursor.x;
        }
        else if(pCtrl->screenCursor.y > 0)
        {
            --pCtrl->screenCursor.y;
            pCtrl->screenCursor.x = pCtrl->columnCount - 1;
        }
    }
}

static inline void _vesaPrintCursor(vesa_controler_t* pCtrl)
{
    _vesaPrintChar(pCtrl, '_');
}

static inline void _vesaClearCursor(vesa_controler_t* pCtrl)
{
    _vesaPrintChar(pCtrl, ' ');
}

static OS_RETURN_E _vesaDrawPixel(void*          pCtrl,
                                  const uint32_t kX,
                                  const uint32_t kY,
                                  const uint32_t kRGBPixel)
{
    vesa_controler_t* pControler;

    pControler = GET_CONTROLER(pCtrl);

    _vesaPutPixel(pControler, kX, kY, kRGBPixel);

    return OS_NO_ERR;
}

static OS_RETURN_E _vesaDrawRectangle(void*               pCtrl,
                                      const graph_rect_t* kpRect)
{
    uint32_t          xEnd;
    uint32_t          yEnd;
    uint32_t          y;
    vesa_controler_t* pControler;
    uintptr_t         pStartBuffer;
    uint16_t          lineSize;
    uint32_t          offset;

    pControler = GET_CONTROLER(pCtrl);

    /* Compute the maximal size based on the screen settings */
    xEnd = MIN(pControler->pCurrentMode->width, kpRect->width + kpRect->x);
    yEnd = MIN(pControler->pCurrentMode->height, kpRect->height + kpRect->y);

    pStartBuffer = (uintptr_t)pControler->videoBuffer.pBack;
    lineSize     = pControler->pCurrentMode->width * sizeof(uint32_t);

    /* Fill the buffer lines by lines */
    offset = kpRect->y * lineSize + kpRect->x * sizeof(uint32_t);
    pStartBuffer += offset;

    for(y = kpRect->y; y < yEnd; ++y)
    {
        _vesaFastFill(pStartBuffer,
                      kpRect->color,
                      xEnd - kpRect->x - 1);
        pStartBuffer += lineSize;
    }

    return OS_NO_ERR;
}

static OS_RETURN_E _vesaDrawLine(void*               pCtrl,
                                 const graph_line_t* kpLine)
{
    uint32_t     distance;
    uint32_t     width;
    uint32_t     height;
    double       xFactor;
    double       yFactor;
    double       currX;
    double       currY;


    graph_rect_t rect;

    width = ABS((int32_t)(kpLine->xEnd - kpLine->xStart));
    height = ABS((int32_t)(kpLine->yEnd - kpLine->yStart));

    /* Check if straight */
    if(kpLine->xStart == kpLine->xEnd)
    {
        rect.color  = kpLine->color;
        rect.height = height;
        rect.width  = 1;
        rect.x      = MIN(kpLine->xStart, kpLine->xEnd);
        rect.y      = MIN(kpLine->yStart, kpLine->yEnd);

        _vesaDrawRectangle(pCtrl, &rect);
    }
    else if(kpLine->yStart == kpLine->yEnd)
    {
        rect.color  = kpLine->color;
        rect.height = 1;
        rect.width  = width;
        rect.x      = MIN(kpLine->xStart, kpLine->xEnd);
        rect.y      = MIN(kpLine->yStart, kpLine->yEnd);

        _vesaDrawRectangle(pCtrl, &rect);
    }
    else
    {
        distance = MAX(width, height);
        if(kpLine->xStart < kpLine->xEnd)
        {
            xFactor  = (double)width / (double)distance;
        }
        else
        {
            xFactor  = -(double)width / (double)distance;
        }
        if(kpLine->yStart < kpLine->yEnd)
        {
            yFactor  = (double)height / (double)distance;
        }
        else
        {
            yFactor  = -(double)height / (double)distance;
        }
        currY    = (double)kpLine->yStart;
        currX    = (double)kpLine->xStart;
        while(distance > 0)
        {
            _vesaPutPixel(pCtrl,
                          ROUND_CLOSEST(currX),
                          ROUND_CLOSEST(currY),
                          kpLine->color);
            currX += xFactor;
            currY += yFactor;
            --distance;
        }
    }

    return OS_NO_ERR;
}

static OS_RETURN_E _vesaDrawBitmap(void*                 pCtrl,
                                   const graph_bitmap_t* kpBitmap)
{
    uint32_t          maxCpy;
    uint32_t          yEnd;
    uint32_t          y;
    vesa_controler_t* pControler;
    uintptr_t         pStartBuffer;
    uintptr_t         pStartBitmap;
    uint16_t          lineSize;
    uint16_t          imageLineSize;
    uint32_t          offset;

    pControler = GET_CONTROLER(pCtrl);

    /* Compute the maximal size based on the screen settings */
    maxCpy = MIN(kpBitmap->x + kpBitmap->width,
                 pControler->pCurrentMode->width);
    maxCpy = (maxCpy - kpBitmap->x) * sizeof(uint32_t);
    yEnd   = MIN(pControler->pCurrentMode->height,
                 kpBitmap->height + kpBitmap->y);

    pStartBuffer = (uintptr_t)pControler->videoBuffer.pBack;
    pStartBitmap = (uintptr_t)kpBitmap->kpData;
    lineSize     = pControler->pCurrentMode->width * sizeof(uint32_t);

    imageLineSize = kpBitmap->width * sizeof(uint32_t);

    /* Fill the buffer lines by lines */
    offset = kpBitmap->y * lineSize + kpBitmap->x * sizeof(uint32_t);
    pStartBuffer += offset;

    for(y = kpBitmap->y; y < yEnd; ++y)
    {
        _vesaFastMemcpy((void*)pStartBuffer, (void*)pStartBitmap, maxCpy);
        pStartBuffer += lineSize;
        pStartBitmap += imageLineSize;
    }

    return OS_NO_ERR;
}

static void _vesaFlush(void* pDriverCtrl)
{
    vesa_controler_t* pCtrl;

    pCtrl = GET_CONTROLER(pDriverCtrl);

    /* Print the first past of the video buffer based on scroll offset */
    _vesaFastToFramebuffer(pCtrl,
                           (uintptr_t)pCtrl->videoBuffer.pFramebuffer,
                           (uintptr_t)pCtrl->videoBuffer.pBack,
                           pCtrl->pCurrentMode->height);
}

static void* _vesaVfsOpen(void*       pDrvCtrl,
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

static int32_t _vesaVfsClose(void* pDrvCtrl, void* pHandle)
{
    (void)pDrvCtrl;
    (void)pHandle;

    /* Nothing to do */
    return 0;
}

static ssize_t _vesaVfsWrite(void*       pDrvCtrl,
                             void*       pHandle,
                             const void* kpBuffer,
                             size_t      count)
{
    const char* pCursor;
    size_t      coutSave;

    (void)pHandle;

    pCursor = (char*)kpBuffer;

    /* Output each character of the string */
    coutSave = count;
    while(pCursor != NULL && *pCursor != 0 && count > 0)
    {
        _vesaProcessChar(pDrvCtrl, *pCursor);
        ++pCursor;
        --count;
    }

    return coutSave - count;
}

static ssize_t _vesaVfsIOCTL(void*    pDriverData,
                             void*    pHandle,
                             uint32_t operation,
                             void*    pArgs)
{
    int32_t                     retVal;
    cons_ioctl_args_scroll_t*   pScrollArgs;
    graph_ioctl_args_drawpixel* pDrawPixelArgs;

    (void)pHandle;

    /* Switch on the operation */
    retVal = 0;
    switch(operation)
    {
        case VFS_IOCTL_CONS_RESTORE_CURSOR:
            _vesaRestoreCursor(pDriverData, pArgs);
            break;
        case VFS_IOCTL_CONS_SAVE_CURSOR:
            _vesaSaveCursor(pDriverData, pArgs);
            break;
        case VFS_IOCTL_CONS_SCROLL:
            pScrollArgs = pArgs;
            _vesaScroll(pDriverData,
                        pScrollArgs->direction,
                        pScrollArgs->lineCount);
            break;
        case VFS_IOCTL_CONS_SET_COLORSCHEME:
            _vesaSetScheme(pDriverData, pArgs);
            break;
        case VFS_IOCTL_CONS_SAVE_COLORSCHEME:
            _vesaSaveScheme(pDriverData, pArgs);
            break;
        case VFS_IOCTL_CONS_CLEAR:
            _vesaClearFramebuffer(pDriverData);
            break;
        case VFS_IOCTL_CONS_FLUSH:
            _vesaFlush(pDriverData);
            break;
        case VFS_IOCTL_GRAPH_DRAWPIXEL:
            pDrawPixelArgs = pArgs;
            retVal = (int32_t)_vesaDrawPixel(pDriverData,
                                             pDrawPixelArgs->x,
                                             pDrawPixelArgs->y,
                                             pDrawPixelArgs->rgbPixel);
            break;
        case VFS_IOCTL_GRAPH_DRAWRECT:
            retVal = (int32_t)_vesaDrawRectangle(pDriverData, pArgs);
            break;
        case VFS_IOCTL_GRAPH_DRAWLINE:
            retVal = (int32_t)_vesaDrawLine(pDriverData, pArgs);
            break;
        case VFS_IOCTL_GRAPH_DRAWBITMAP:
            retVal = (int32_t)_vesaDrawBitmap(pDriverData, pArgs);
            break;
        default:
            retVal = -1;
    }

    return retVal;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86VESADriver);

/************************************ EOF *************************************/