/*******************************************************************************
 * @file cpu.h
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

#ifndef __X86_64_CPU_H_
#define __X86_64_CPU_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h>       /* Generic int types */
#include <stddef.h>       /* Standard definition */
#include <kerror.h>       /* Kernel error */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief CPU flags interrupt enabled flag. */
#define CPU_RFLAGS_IF 0x000000200

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
 * ext canbe either 0x0 or 0x80000000 to return highest supported value for
 * basic or extended CPUID information.  Function returns 0 if CPUID
 * is not supported or whatever CPUID returns in eax register.  If sig
 * pointer is non-null, then first four bytes of the SIG
 * (as found in ebx register) are returned in location pointed by sig.
 *
 * @param[in] ext The opperation code for the CPUID instruction.
 * @return The highest supported input value for CPUID instruction.
 */
inline static uint32_t _cpuGetCPUIDMax (const uint32_t kExt)
{
    uint32_t regs[4];

    /* Host supports CPUID. Return highest supported CPUID input value. */
    __asm__ __volatile__("cpuid":"=a"(*regs),"=b"(*(regs+1)),
                         "=c"(*(regs+2)),"=d"(*(regs+3)):"a"(kExt));

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
inline static int32_t _cpuCPUID(const uint32_t kCode, uint32_t regs[4])
{
    uint32_t ext      = kCode & 0x80000000;
    uint32_t maxlevel = _cpuGetCPUIDMax(ext);

    if (maxlevel == 0 || maxlevel < kCode)
    {
        return 0;
    }
    __asm__ __volatile__("cpuid":"=a"(*regs),"=b"(*(regs+1)),
                         "=c"(*(regs+2)),"=d"(*(regs+3)):"a"(kCode));
    return 1;
}

/** @brief Clears interupt bit which results in disabling interrupts. */
inline static void _cpuClearInterrupt(void)
{

    __asm__ __volatile__("cli":::"memory");
}

/** @brief Sets interrupt bit which results in enabling interupts. */
inline static void _cpuSetInterrupt(void)
{

    __asm__ __volatile__("sti":::"memory");
}

/** @brief Halts the CPU for lower energy consuption. */
inline static void _cpuHalt(void)
{

    __asm__ __volatile__ ("hlt":::"memory");
}

/**
 * @brief Returns the current CPU flags.
 *
 * @return The current CPU flags.
 */
inline static uint64_t _cpuSaveFlags(void)
{
    uint64_t flags;

    __asm__ __volatile__(
        "pushfq\n\t"
        "pop    %0\n\t"
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
inline static void _cpuRestoreFlags(const uint64_t kFlags)
{
    __asm__ __volatile__(
        "push    %0\n\t"
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
inline static void _cpuOutB(const uint8_t kValue, const uint16_t kPort)
{
    __asm__ __volatile__("outb %0, %1" : : "a" (kValue), "Nd" (kPort));
}

/**
 * @brief Writes word on port.
 *
 * @param[in] kValue The value to send to the port.
 * @param[in] kPort The port to which the value has to be written.
 */
inline static void _cpuOutW(const uint16_t kValue, const uint16_t kPort)
{
    __asm__ __volatile__("outw %0, %1" : : "a" (kValue), "Nd" (kPort));
}

/**
 * @brief Writes long on port.
 *
 * @param[in] kValue The value to send to the port.
 * @param[in] kPort The port to which the value has to be written.
 */
inline static void _cpuOutL(const uint32_t kValue, const uint16_t kPort)
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
inline static uint8_t _cpuInB(const uint16_t kPort)
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
inline static uint16_t _cpuInW(const uint16_t kPort)
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
inline static uint32_t _cpuInL(const uint16_t kPort)
{
    uint32_t rega;
    __asm__ __volatile__("inl %1,%0" : "=a" (rega) : "Nd" (kPort));
    return rega;
}


/**
 * @brief Returns the saved interrupt state.
 *
 * @details Returns the saved interrupt state based on the stack state.
 *
 * @param[in] kpVCpu The current thread's virtual CPU.
 *
 * @return The current savec interrupt state: 1 if enabled, 0 otherwise.
 */
inline static uint32_t _cpuGetContextIntState(const virtual_cpu_t* kpVCpu)
{
    return kpVCpu->intContext.rflags & CPU_RFLAGS_IF;
}

/**
 * @brief Returns the CPU current interrupt state.
 *
 * @details Returns the current CPU eflags interrupt enable value.
 *
 * @return The CPU current interrupt state: 1 if enabled, 0 otherwise.
 */
inline static uint32_t _cpuGeIntState(void)
{
    return ((_cpuSaveFlags() & CPU_RFLAGS_IF) != 0);
}

/**
 * @brief Initializes the CPU.
 *
 * @details Initializes the CPU registers and relevant structures.
 */
void cpuInit(void);

/**
 * @brief Raises CPU interrupt.
 *
 * @details Raises a software CPU interrupt on the desired line.
 *
 * @param[in] kInterruptLine The line on which the interrupt should be raised.
 *
 * @return OS_NO_ERR shoudl be return in case of success.
 * - OS_ERR_UNAUTHORIZED_ACTION Is returned if the interrupt line is not
 * correct.
 */
OS_RETURN_E cpuRaiseInterrupt(const uint32_t kInterruptLine);

/**
 * @brief Checks the architecture's feature and requirements for UTK.
 *
 * @details Checks the architecture's feature and requirements for UTK. If a
 * requirement is not met, a kernel panic is raised.
 */
void cpuValidateArchitecture(void);


#endif /* #ifndef __X86_64_CPU_H_ */

/************************************ EOF *************************************/