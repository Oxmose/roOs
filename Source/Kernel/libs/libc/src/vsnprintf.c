/*******************************************************************************
 * @file vsnprintf.c
 *
 * @see stdlib.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 21/07/2024
 *
 * @version 1.0
 *
 * @brief vsnprintf function. To be used with stdlib.h header.
 *
 * @details vsnprintf function. To be used with stdlib.h header.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <stddef.h>   /* Standard definitions */
#include <string.h>   /* String manipulation */
#include <stdbool.h>  /* Bool types */

/* Configuration files */
#include <config.h>

/* Header file */
#include <stdlib.h>

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

/**
 * @brief Adds a padding sequence before a formated input.
 */
#define PAD_SEQ                                                         \
{                                                                       \
    strSize = strlen(tmpSeq);                                           \
                                                                        \
    while(paddingMod > strSize)                                         \
    {                                                                   \
        _toBufferChar(pBuffer, &bufferPos, kSize, padCharMod);          \
        --paddingMod;                                                   \
    }                                                                   \
}

/**
 * @brief Get a sequence value argument for floats.
 */
#define GET_SEQ_VAL_DOUBLE(VAL, ARGS, LENGTH_MOD)   \
{                                                   \
    VAL = __builtin_va_arg(ARGS, double);           \
}

/**
 * @brief Get a sequence value argument for integers.
 */
#define GET_SEQ_VAL(VAL, ARGS, LENGTH_MOD, EXTEND)                  \
{                                                                   \
                                                                    \
    /* Harmonize length */                                          \
    if(LENGTH_MOD > 8)                                              \
    {                                                               \
        LENGTH_MOD = 8;                                             \
    }                                                               \
                                                                    \
    switch(LENGTH_MOD)                                              \
    {                                                               \
        case 1:                                                     \
            VAL = (__builtin_va_arg(ARGS, uint32_t) & 0xFF);        \
            if(((uint64_t)VAL & 0x80) != 0 && EXTEND == true)       \
            {                                                       \
                VAL |= 0xFFFFFFFFFFFFFF00;                          \
            }                                                       \
            break;                                                  \
        case 2:                                                     \
            VAL = (__builtin_va_arg(ARGS, uint32_t) & 0xFFFF);      \
            if(((uint64_t)VAL & 0x8000) != 0 && EXTEND == true)     \
            {                                                       \
                VAL |= 0xFFFFFFFFFFFF0000;                          \
            }                                                       \
            break;                                                  \
        case 4:                                                     \
            VAL = __builtin_va_arg(ARGS, uint32_t);                 \
            if(((uint64_t)VAL & 0x80000000) != 0 && EXTEND == true) \
            {                                                       \
                VAL |= 0xFFFFFFFF00000000;                          \
            }                                                       \
            break;                                                  \
        case 8:                                                     \
            VAL = __builtin_va_arg(ARGS, uint64_t);                 \
            break;                                                  \
        default:                                                    \
            VAL = __builtin_va_arg(ARGS, uint32_t);                 \
            if(((uint64_t)VAL & 0x80000000) != 0 && EXTEND == true) \
            {                                                       \
                VAL |= 0xFFFFFFFF00000000;                          \
            }                                                       \
    }                                                               \
}

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

/**
 * @brief Converts a string to upper case characters.
 *
 * @details Transforms all lowercase character of a NULL terminated string to
 * uppercase characters.
 *
 * @param[in, out] pString The string to tranform.
 */
static inline void _toUpper(char* pString);

/**
 * @brief Converts a string to upper case characters.
 *
 * @details Transforms all uppercase character of a NULL terminated string to
 * lowercase characters.
 *
 * @param[in, out] pString The string to tranform.
 */
static inline void _toLower(char* pString);

/**
 * @brief Copies a character to the buffer.
 *
 * @details Copies a character to the buffer. The offset of  the buffer is
 * updated in the function and the function does not copy more than the size
 * of the buffer.
 *
 * @param[out] pBuffer The buffer to fill.
 * @param[in, out] pOffset The pointer to the buffer offset. It is updated by
 * the copy.
 * @param[in] kSize The maximal size of the buffer.
 * @param[in] kChar The character to copy.
 */
static inline void _toBufferChar(char*        pBuffer,
                                 size_t*      pOffset,
                                 const size_t kSize,
                                 const char   kChar);

/**
 * @brief Copies a string to the buffer.
 *
 * @details Copies a string to the buffer. The offset of  the buffer is
 * updated in the function and the function does not copy more than the size
 * of the buffer.
 *
 * @param[out] pBuffer The buffer to fill.
 * @param[in, out] pOffset The pointer to the buffer offset. It is updated by
 * the copy.
 * @param[in] kSize The maximal size of the buffer.
 * @param[in] kStr The string to copy.
 */
static inline void _toBufferString(char*        pBuffer,
                                   size_t*      pOffset,
                                   const size_t kSize,
                                   const char*  kStr);

/**
 * @brief Formats a string to a buffer in memory.
 *
 * @details Formats a string to a buffer in memory. The buffer is not filled
 * more than its size.
 *
 * @param[out] pBuffer The buffer to fill.
 * @param[in] kSize The maximal size of the buffer.
 * @param[in] kFmt The format string to use.
 * @param[in] args Additional parameters for the format.
 */
static size_t _formatArgs(char*             pBuffer,
                          const size_t      kSize,
                          const char*       kpFmt,
                          __builtin_va_list args);

/**
 * @brief Converts a double or float value to a string.
 *
 * @details Converts a double or float value to a string.
 *
 * @param[in] value The value to convert.
 * @param[out] pBuffer The buffer that receives the converted value.
 * @param[in, out] pSignificant The number of decimal after the decimal point.
 * This value is updated to substract the number of decimal actually set.
 */
static inline void _floatToStr(double   value,
                               char*    pBuffer,
                               uint8_t* pSignificant);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static inline void _toUpper(char* pString)
{
    /* For each character of the string */
    while(*pString != 0)
    {
        /* If the character is lowercase, makes it uppercase */
        if(*pString > 96 && *pString < 123)
        {
            *pString = *pString - 32;
        }
        ++pString;
    }
}

static inline void _toLower(char* pString)
{
    /* For each character of the string */
    while(*pString != 0)
    {
        /* If the character is uppercase, makes it lowercase */
        if(*pString > 64 && *pString < 91)
        {
            *pString = *pString + 32;
        }
        ++pString;
    }
}

static inline void _toBufferChar(char*        pBuffer,
                                 size_t*      pOffset,
                                 const size_t kSize,
                                 const char   kChar)
{
    if(kSize - 1 > *pOffset)
    {
        pBuffer[*pOffset] = kChar;
        *pOffset += 1;
    }
}

static inline void _toBufferString(char*        pBuffer,
                                   size_t*      pOffset,
                                   const size_t kSize,
                                   const char*  kStr)
{
    while(kSize - 1 > *pOffset && *kStr != 0)
    {
        pBuffer[*pOffset] = *kStr;
        *pOffset += 1;
        ++kStr;
    }
}

#pragma GCC diagnostic ignored "-Wstringop-overflow"
static inline void _floatToStr(double   value,
                               char*    pBuffer,
                               uint8_t* pSignificant)
{
    uint64_t intValue;
    bool     isNeg;

    isNeg = (value < 0);
    if(isNeg)
    {
        *pBuffer = '-';
        ++pBuffer;
        value = -value;
    }

    /* Setup the integer part */
    intValue = (uint64_t)value;
    uitoa(intValue, pBuffer, 10);
    pBuffer += strlen(pBuffer) + (isNeg ? 1 : 0);
    value -= intValue;

    if(*pSignificant == 0)
    {
        *pSignificant = 10;
    }

    *pBuffer = '.';
    ++pBuffer;

    /* Now setup the decimal part */
    while(*pSignificant > 0)
    {
        intValue = (uint64_t)(value * 10.0);
        *pBuffer = ((char)intValue) + '0';
        ++pBuffer;
        --*pSignificant;
    }

    pBuffer = 0;
}

static size_t _formatArgs(char*             pBuffer,
                          const size_t      kSize,
                          const char*       kpFmt,
                          __builtin_va_list args)
{
    size_t   pos;
    size_t   strLength;
    uint64_t seqVal;
    double   seqValFloat;
    size_t   strSize;
    uint8_t  modifier;
    uint8_t  lengthMod;
    uint8_t  paddingMod;
    bool     upperMod;
    char     padCharMod;
    char     tmpSeq[128];
    char*    pArgsValue;
    size_t   bufferPos;

    bufferPos  = 0;
    modifier   = 0;
    lengthMod  = 4;
    paddingMod = 0;
    upperMod   = false;
    padCharMod = ' ';
    strLength  = strlen(kpFmt);

    for(pos = 0; pos < strLength; ++pos)
    {
        if(kpFmt[pos] == '%')
        {
            /* If we encouter this character in a modifier sequence, it was
             * just an escape one.
             */
            modifier = !modifier;
            if(modifier)
            {
                continue;
            }
            else
            {
                _toBufferChar(pBuffer, &bufferPos, kSize, kpFmt[pos]);
            }
        }
        else if(modifier)
        {
            switch(kpFmt[pos])
            {
                /* Length mods */
                case 'h':
                    lengthMod /= 2;
                    continue;
                case 'l':
                    lengthMod *= 2;
                    continue;

                /* Specifier mods */
                case 's':
                    pArgsValue = __builtin_va_arg(args, char*);
                    _toBufferString(pBuffer, &bufferPos, kSize, pArgsValue);

                    break;
                case 'd':
                case 'i':
                    GET_SEQ_VAL(seqVal, args, lengthMod, true);
                    memset(tmpSeq, 0, sizeof(tmpSeq));
                    itoa(seqVal, tmpSeq, 10);
                    PAD_SEQ
                    _toBufferString(pBuffer, &bufferPos, kSize, tmpSeq);
                    break;
                case 'u':
                    GET_SEQ_VAL(seqVal, args, lengthMod, false);
                    memset(tmpSeq, 0, sizeof(tmpSeq));
                    uitoa(seqVal, tmpSeq, 10);
                    PAD_SEQ
                    _toBufferString(pBuffer, &bufferPos, kSize, tmpSeq);
                    break;
                case 'X':
                    upperMod = true;
                    __attribute__ ((fallthrough));
                case 'x':
                    GET_SEQ_VAL(seqVal, args, lengthMod, false);
                    memset(tmpSeq, 0, sizeof(tmpSeq));
                    uitoa(seqVal, tmpSeq, 16);
                    PAD_SEQ
                    if(upperMod == true)
                    {
                        _toUpper(tmpSeq);
                    }
                    else
                    {
                        _toLower(tmpSeq);
                    }
                    _toBufferString(pBuffer, &bufferPos, kSize, tmpSeq);
                    break;
                case 'f':
                    GET_SEQ_VAL_DOUBLE(seqValFloat, args, lengthMod);
                    memset(tmpSeq, 0, sizeof(tmpSeq));
                    _floatToStr(seqValFloat, tmpSeq, &paddingMod);
                    _toBufferString(pBuffer, &bufferPos, kSize, tmpSeq);
                    break;
                case 'P':
                    upperMod = true;
                    __attribute__ ((fallthrough));
                case 'p':
                    paddingMod  = 2 * sizeof(uintptr_t);
                    padCharMod = '0';
                    lengthMod = sizeof(uintptr_t);
                    GET_SEQ_VAL(seqVal, args, lengthMod, false);
                    memset(tmpSeq, 0, sizeof(tmpSeq));
                    uitoa(seqVal, tmpSeq, 16);
                    PAD_SEQ
                    if(upperMod == true)
                    {
                        _toUpper(tmpSeq);
                    }
                    else
                    {
                        _toLower(tmpSeq);
                    }
                    _toBufferString(pBuffer, &bufferPos, kSize, tmpSeq);
                    break;
                case 'c':
                    lengthMod = sizeof(char);
                    GET_SEQ_VAL(tmpSeq[0], args, lengthMod, false);
                    _toBufferChar(pBuffer, &bufferPos, kSize, tmpSeq[0]);
                    break;

                /* Padding mods */
                case '0':
                    if(paddingMod == 0)
                    {
                        padCharMod = '0';
                    }
                    else
                    {
                        paddingMod *= 10;
                    }
                    continue;
                case '1':
                    paddingMod = paddingMod * 10 + 1;
                    continue;
                case '2':
                    paddingMod = paddingMod * 10 + 2;
                    continue;
                case '3':
                    paddingMod = paddingMod * 10 + 3;
                    continue;
                case '4':
                    paddingMod = paddingMod * 10 + 4;
                    continue;
                case '5':
                    paddingMod = paddingMod * 10 + 5;
                    continue;
                case '6':
                    paddingMod = paddingMod * 10 + 6;
                    continue;
                case '7':
                    paddingMod = paddingMod * 10 + 7;
                    continue;
                case '8':
                    paddingMod = paddingMod * 10 + 8;
                    continue;
                case '9':
                    paddingMod = paddingMod * 10 + 9;
                    continue;
                default:
                    continue;
            }
        }
        else
        {
            _toBufferChar(pBuffer, &bufferPos, kSize, kpFmt[pos]);
        }

        /* Reinit mods */
        lengthMod  = 4;
        paddingMod = 0;
        upperMod   = false;
        padCharMod = ' ';
        modifier   = 0;
    }

    pBuffer[bufferPos] = 0;

    return bufferPos;
}

int vsnprintf(char*             pBuffer,
              size_t            size,
              const char*       kpFmt,
              __builtin_va_list args)
{
    return (int)_formatArgs(pBuffer, size, kpFmt, args);
}

/************************************ EOF *************************************/