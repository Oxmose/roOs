/*******************************************************************************
 * @file x86cpu.h
 *
 * @see cpu.c
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief i386 CPU management functions
 *
 * @details i386 CPU manipulation functions. Wraps inline assembly calls for
 * ease of development.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __I386_X86_CPU_H_
#define __I386_X86_CPU_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <cpu.h>          /* Generic CPU definitions */
#include <stdint.h>       /* Generic int types */
#include <stddef.h>       /* Standard definition */
#include <kerror.h>       /* Kernel error */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief CPU flags interrupt enabled flag. */
#define CPU_EFLAGS_IF 0x000000200

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Holds the CPU register values */
typedef struct
{
    /** @brief CPU's esp register. */
    uint32_t esp;
    /** @brief CPU's ebp register. */
    uint32_t ebp;
    /** @brief CPU's edi register. */
    uint32_t edi;
    /** @brief CPU's esi register. */
    uint32_t esi;
    /** @brief CPU's edx register. */
    uint32_t edx;
    /** @brief CPU's ecx register. */
    uint32_t ecx;
    /** @brief CPU's ebx register. */
    uint32_t ebx;
    /** @brief CPU's eax register. */
    uint32_t eax;

    /** @brief CPU's ss register. */
    uint32_t ss;
    /** @brief CPU's gs register. */
    uint32_t gs;
    /** @brief CPU's fs register. */
    uint32_t fs;
    /** @brief CPU's es register. */
    uint32_t es;
    /** @brief CPU's ds register. */
    uint32_t ds;
} __attribute__((packed)) cpu_state_t;

/** @brief Holds the interrupt context */
typedef struct
{
    /** @brief Interrupt's index */
    uint32_t intId;
    /** @brief Interrupt's error code. */
    uint32_t errorCode;
    /** @brief EIP of the faulting instruction. */
    uint32_t eip;
    /** @brief CS before the interrupt. */
    uint32_t cs;
    /** @brief EFLAGS before the interrupt. */
    uint32_t eflags;
} __attribute__((packed)) int_context_t;

/**
 * @brief Defines the virtual CPU context for the i386 CPU.
 */
typedef struct
{
    /** @brief VCPU interupt context */
    int_context_t intContext;

    /** @brief Virtual CPU context */
    cpu_state_t vCpu;
} virtual_cpu_t;

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
 * @brief Returns the highest support CPUID feature request ID.
 *
 * @details Returns the highest supported input value for CPUID instruction.
 * kExt can be either 0x0 or 0x80000000 to return highest supported value for
 * basic or extended CPUID information.  Function returns 0 if CPUID
 * is not supported or whatever CPUID returns in eax register.  If sig
 * pointer is non-null, then first four bytes of the SIG
 * (as found in ebx register) are returned in location pointed by sig.
 *
 * @param[in] kExt The opperation code for the CPUID instruction.
 * @return The highest supported input value for CPUID instruction.
 */
static inline uint32_t _cpuGetCPUIDMax(const uint32_t kExt)
{
    uint32_t regs[4];

    /* Host supports CPUID. Return highest supported CPUID input value. */
    __asm__ __volatile__("cpuid":
                         "=a"(*regs),
                         "=b"(*(regs+1)),
                         "=c"(*(regs+2)),
                         "=d"(*(regs+3)):
                         "a"(kExt));

    return regs[0];
}

/**
 * @brief Returns the CPUID data for a requested leaf.
 *
 * @details Returns CPUID data for requested CPUID leaf, as found in returned
 * eax, ebx, ecx and edx registers.  The function checks if CPUID is
 * supported and returns 1 for valid CPUID information or 0 for
 * unsupported CPUID leaf. All pointers are required to be non-null.
 *
 * @param[in] kCode The opperation code for the CPUID instruction.
 * @param[out] regs The register used to store the CPUID instruction return.
 * @return 1 in case of succes, 0 otherwise.
 */
static inline int32_t _cpuCPUID(const uint32_t kCode, uint32_t pRegs[4])
{
    uint32_t ext;
    uint32_t maxLevel;

    ext      = kCode & 0x80000000;
    maxLevel = _cpuGetCPUIDMax(ext);

    if (maxLevel == 0 || maxLevel < kCode)
    {
        return 0;
    }
    __asm__ __volatile__("cpuid":
                         "=a"(*pRegs),
                         "=b"(*(pRegs+1)),
                         "=c"(*(pRegs+2)),
                         "=d"(*(pRegs+3)):
                         "a"(kCode));
    return 1;
}

/**
 * @brief Returns the current CPU flags.
 *
 * @return The current CPU flags.
 */
static inline uint32_t _cpuSaveFlags(void)
{
    uint32_t flags;

    __asm__ __volatile__(
        "pushfl\n\t"
        "popl %0\n\t"
        : "=g" (flags)
        :
        : "memory"
    );

    return flags;
}

/**
 * @brief Restores CPU flags
 *
 * @param[in] kFlags The flags to be restored.
 */
static inline void _cpuRestoreFlags(const uint32_t kFlags)
{
    __asm__ __volatile__(
        "pushl %0\n\t"
        "popfl\n\t"
        :
        : "g" (kFlags)
        : "memory"
    );
}

/**
 * @brief Writes byte on port.
 *
 * @param[in] kValue The value to send to the port.
 * @param[in] kPort The port to which the value has to be written.
 */
static inline void _cpuOutB(const uint8_t kValue, const uint16_t kPort)
{
    __asm__ __volatile__("outb %0, %1" : : "a" (kValue), "Nd" (kPort));
}

/**
 * @brief Writes word on port.
 *
 * @param[in] kValue The value to send to the port.
 * @param[in] kPort The port to which the value has to be written.
 */
static inline void _cpuOutW(const uint16_t kValue, const uint16_t kPort)
{
    __asm__ __volatile__("outw %0, %1" : : "a" (kValue), "Nd" (kPort));
}

/**
 * @brief Writes long on port.
 *
 * @param[in] kValue The value to send to the port.
 * @param[in] kPort The port to which the value has to be written.
 */
static inline void _cpuOutL(const uint32_t kValue, const uint16_t kPort)
{
    __asm__ __volatile__("outl %0, %1" : : "a" (kValue), "Nd" (kPort));
}

/**
 * @brief Reads byte on port.
 *
 * @return The value read from the port.
 *
 * @param[in] kPort The port to which the value has to be read.
 */
static inline uint8_t _cpuInB(const uint16_t kPort)
{
    uint8_t rega;
    __asm__ __volatile__("inb %1,%0" : "=a" (rega) : "Nd" (kPort));
    return rega;
}

/**
 * @brief Reads word on port.
 *
 * @return The value read from the port.
 *
 * @param[in] kPort The port to which the value has to be read.
 */
static inline uint16_t _cpuInW(const uint16_t kPort)
{
    uint16_t rega;
    __asm__ __volatile__("inw %1,%0" : "=a" (rega) : "Nd" (kPort));
    return rega;
}

/**
 * @brief Reads long on port.
 *
 * @return The value read from the port.
 *
 * @param[in] kPort The port to which the value has to be read.
 */
static inline uint32_t _cpuInL(const uint16_t kPort)
{
    uint32_t rega;
    __asm__ __volatile__("inl %1,%0" : "=a" (rega) : "Nd" (kPort));
    return rega;
}

/**
 * @brief Entry C function for secondary cores.
 *
 * @details Entry C function for secondary cores. This function is called by the
 * secondary cores after initializing their state in the secondary core
 * startup function.
 *
 * @param[in] kCpuId The booted CPU identifier that call the function.
 *
 * @warning This function should never be called by the user, only the assembly
 * startup should call it.
 */
void cpuApInit(const uint8_t kCpuId);

/**
 * @brief Sets the new page directory for the calling CPU.
 *
 * @details Sets the new page directory for the calling CPU. The page directory
 * address passed as parameter must be a physical address.
 *
 * @param[in] kNewPgDir The physical address of the new page directory.
 */
void cpuSetPageDirectory(const uintptr_t kNewPgDir);

/**
 * @brief Invalidates a page in the TLB that contains the virtual address given
 * as parameter.
 *
 * @details Invalidates a page in the TLB that contains the virtual address
 * given as parameter. This macro uses inline assembly to invocate the INVLPG
 * instruction.
 *
 * @param[in] kVirtAddress The virtual address contained in the page to
 * invalidate.
 */
void cpuInvalidateTlbEntry(const uintptr_t kVirtAddress);

#endif /* #ifndef __I386_X86_CPU_H_ */

/************************************ EOF *************************************/