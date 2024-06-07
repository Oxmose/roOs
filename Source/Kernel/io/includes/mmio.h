/*******************************************************************************
 * @file mmio.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 2.0
 *
 * @brief Kernel's memory mapped IO functions.
 *
 * @details Kernel's memory mapped IO functions. Memory mapped IOs, avoids
 * compilers to reorganize memory access.
 * So instead of doing : *addr = kValue, do
 * mmioWrite(pAddr, kValue)
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __IO_MMIO_H_
#define __IO_MMIO_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h> /* Generic types */

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
 * FUNCTIONS
 ******************************************************************************/

/**
 * @brief Memory mapped IO byte write access.
 *
 * @details Memory mapped IOs byte write access. Avoids compilers to reorganize
 * memory access, instead of doing : *addr = kValue, do
 * mmioWrite(pAddr, kValue) to avoid instruction reorganization.
 *
 * @param[in] pAddr The address of the IO to write.
 * @param[in] kValue The value to write to the IO.
 */
inline static void _mmioWrite8(void* volatile pAddr, const uint8_t kValue)
{
    *(volatile uint8_t*)(pAddr) = kValue;
}

/**
 * @brief Memory mapped IO half word write access.
 *
 * @details Memory mapped IOs half word write access. Avoids compilers to
 * reorganize memory access, instead of doing : *addr = kValue, do
 * mmioWrite(pAddr, kValue) to avoid instruction reorganization.
 *
 * @param[in] pAddr The address of the IO to write.
 * @param[in] kValue The value to write to the IO.
 */
inline static void _mmioWrite16(void* volatile pAddr, const uint16_t kValue)
{
    *(volatile uint16_t*)(pAddr) = kValue;
}

/**
 * @brief Memory mapped IO word write access.
 *
 * @details Memory mapped IOs word write access. Avoids compilers to reorganize
 * memory access, instead of doing : *addr = kValue, do
 * mmioWrite(pAddr, kValue) to avoid instruction reorganization.
 *
 * @param[in] pAddr The address of the IO to write.
 * @param[in] kValue The value to write to the IO.
 */
inline static void _mmioWrite32(void* volatile pAddr, const uint32_t kValue)
{
    *(volatile uint32_t*)(pAddr) = kValue;
}

/**
 * @brief Memory mapped IO double word write access.
 *
 * @details Memory mapped IOs double word write access. Avoids compilers to
 * reorganize memory access, instead of doing : *addr = kValue, do
 * mmioWrite(pAddr, kValue) to avoid instruction reorganization.
 *
 * @param[in] pAddr The address of the IO to write.
 * @param[in] kValue The value to write to the IO.
 */
inline static void _mmioWrite64(void* volatile pAddr, const uint64_t kValue)
{
    *(volatile uint64_t*)(pAddr) = kValue;
}

/**
 * @brief Memory mapped IO byte read access.
 *
 * @details Memory mapped IOs byte read access. Avoids compilers to reorganize
 * memory access, instead of doing : kValue = *addr, do
 * value = mmioRead(pAddr) to avoid instruction reorganization.
 *
 * @param[in] pAddr The address of the IO to read.
 */
inline static uint8_t _mmioRead8(const volatile void* pAddr)
{
    return *(volatile uint8_t*)(pAddr);
}

/**
 * @brief Memory mapped IO half word read access.
 *
 * @details Memory mapped IOs half word read access. Avoids compilers to
 * reorganize memory access, instead of doing : kValue = *addr, do
 * value = mmioRead(pAddr) to avoid instruction reorganization.
 *
 * @param[in] pAddr The address of the IO to read.
 */
inline static uint16_t _mmioRead16(const volatile void* pAddr)
{
    return *(volatile uint16_t*)(pAddr);
}

/**
 * @brief Memory mapped IO word read access.
 *
 * @details Memory mapped IOs word read access. Avoids compilers to reorganize
 * memory access, instead of doing : kValue = *addr, do
 * value = mmioRead(pAddr) to avoid instruction reorganization.
 *
 * @param[in] pAddr The address of the IO to read.
 */
inline static uint32_t _mmioRead32(const volatile void* pAddr)
{
    return *(volatile uint32_t*)(pAddr);
}

/**
 * @brief Memory mapped IO double word read access.
 *
 * @details Memory mapped IOs double word read access. Avoids compilers to
 * reorganize memory access, instead of doing : kValue = *addr, do
 * value = mmioRead(pAddr) to avoid instruction reorganization.
 *
 * @param[in] pAddr The address of the IO to read.
 */
inline static uint64_t _mmioRead64(const volatile void* pAddr)
{
    return *(volatile uint64_t*)(pAddr);
}

#endif /* #ifndef __IO_MMIO_H_ */

/************************************ EOF *************************************/