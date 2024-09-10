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
 * @brief x86_64 CPU management functions
 *
 * @details x86_64 CPU manipulation functions. Wraps inline assembly calls for
 * ease of development.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __X8664_X86_CPU_H_
#define __X8664_X86_CPU_H_

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

/** @brief FX data region size, increased with padding for alignement */
#define FXDATA_REGION_SIZE 528

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Holds the CPU register values */
typedef struct
{
    /** @brief CPU's rsp register. */
    uint64_t rsp;
    /** @brief CPU's rbp register. */
    uint64_t rbp;
    /** @brief CPU's rdi register. */
    uint64_t rdi;
    /** @brief CPU's rsi register. */
    uint64_t rsi;
    /** @brief CPU's rdx register. */
    uint64_t rdx;
    /** @brief CPU's rcx register. */
    uint64_t rcx;
    /** @brief CPU's rbx register. */
    uint64_t rbx;
    /** @brief CPU's rax register. */
    uint64_t rax;

    /** @brief CPU's r8 register. */
    uint64_t r8;
    /** @brief CPU's r9 register. */
    uint64_t r9;
    /** @brief CPU's r10 register. */
    uint64_t r10;
    /** @brief CPU's r11 register. */
    uint64_t r11;
    /** @brief CPU's r12 register. */
    uint64_t r12;
    /** @brief CPU's r13 register. */
    uint64_t r13;
    /** @brief CPU's r14 register. */
    uint64_t r14;
    /** @brief CPU's r15 register. */
    uint64_t r15;

    /** @brief CPU's ss register. */
    uint64_t ss;
    /** @brief CPU's gs register. */
    uint64_t gs;
    /** @brief CPU's fs register. */
    uint64_t fs;
    /** @brief CPU's es register. */
    uint64_t es;
    /** @brief CPU's ds register. */
    uint64_t ds;
} __attribute__((packed)) cpu_state_t;

/** @brief Holds the interrupt context */
typedef struct
{
    /** @brief Interrupt's index */
    uint64_t intId;
    /** @brief Interrupt's error code. */
    uint64_t errorCode;
    /** @brief RIP of the faulting instruction. */
    uint64_t rip;
    /** @brief CS before the interrupt. */
    uint64_t cs;
    /** @brief RFLAGS before the interrupt. */
    uint64_t rflags;
} __attribute__((packed)) int_context_t;

/**
 * @brief Defines the virtual CPU context for the x86_64 CPU.
 */
typedef struct
{
    /** @brief VCPU interupt context */
    int_context_t intContext;

    /** @brief Virtual CPU context */
    cpu_state_t cpuState;

    /** @brief FXSAVE / FXRSTOR data region */
    uint8_t fxData[FXDATA_REGION_SIZE];

    /** @brief Last context save status */
    uint64_t isContextSaved;
} __attribute__((packed)) virtual_cpu_t;

/** @brief Defines the memory layout of the FXData region */
typedef struct
{
    /** @brief FPU Control Word */
    uint16_t fcw;
    /** @brief FPU Status Word */
    uint16_t fsw;
    /** @brief FPU Tag Word */
    uint16_t ftw;
    /** @brief FPU Final Opcode */
    uint16_t fop;
    /** @brief FPU Intruction Pointer */
    uint32_t fip;
    /** @brief FPU Control Status */
    uint16_t fcs;
    /** @brief FPU Reserved */
    uint16_t reserved0;
    /** @brief FPU Data Pointer */
    uint32_t fdp;
    /** @brief FPU Data Pointer Selector */
    uint16_t fds;
    /** @brief FPU Reserved */
    uint16_t reserved1;
    /** @brief MXCSR Register */
    uint32_t mxcsr;
    /** @brief MXCSR Mask Register */
    uint32_t mxcsrMask;

    /** @brief Other SSE/FPU Work Register */
    uint8_t registers[];
} fxdata_layout_t;

/** @brief BIOS call CPU abstraction. Used to store the CPU registers value. */
typedef struct
{
    /** @brief CPU ax register */
    uint16_t ax;
    /** @brief CPU bx register */
    uint16_t bx;
    /** @brief CPU cx register */
    uint16_t cx;
    /** @brief CPU ax register */
    uint16_t dx;
    /** @brief CPU flags register */
    uint16_t flags;
    /** @brief Padding */
    uint8_t pad[6];
} __attribute__((__packed__)) bios_int_regs_t;


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
static inline uint64_t _cpuSaveFlags(void)
{
    uint64_t flags;

    __asm__ __volatile__(
        "pushfq\n\t"
        "pop %0\n\t"
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
static inline void _cpuRestoreFlags(const uint64_t kFlags)
{
    __asm__ __volatile__(
        "push %0\n\t"
        "popfq\n\t"
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

/**
 * @brief Issue a bios interrupt.
 *
 * @details Switch the CPU to real mode and raise an interrupt. This interrupt
 * should be handled by the BIOS IVT. This function is not thread safe.
 *
 * @param[in, out] pRegs The array containing the registers values for the call.
 * @param[in] kIntNumber The interrupt number to raise.
 * @param[out] pBuffer The buffer used to receive the generated data is any.
 * @param[in] kBufferSize The size of the data buffer.
 * @param[out] pInitialDataLocation In the case data were to be copied in the
 * buffer, this parameter is set with the initial location of the data before
 * the copy.
 */
void cpuBiosCall(bios_int_regs_t* pRegs,
                 const uint8_t    kIntNumber,
                 void*            pBuffer,
                 const size_t     kBufferSize,
                 uint32_t*        pInitialDataLocation);

/**
 * @brief CPU Redirection handler entry point.
 *
 * @details CPU Redirection handler entry point. This function is put on the
 * RIP of a thread that needs execution redirection. In this function, the
 * stack is used (at RSP) to get the address of the function to be called by
 * the handler.
 *
 * @warning Never call this function directly, this function is only used to be
 * put in the thread's VCPU along with the stack modification that it requires.
 */
void cpuSignalHandler(void);

#endif /* #ifndef __X8664_X86_CPU_H_ */

/************************************ EOF *************************************/