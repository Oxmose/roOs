/*******************************************************************************
 * @file cpu.c
 *
 * @see cpu.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 30/03/2023
 *
 * @version 1.0
 *
 * @brief x64 CPU management functions
 *
 * @details x64 CPU manipulation functions. Wraps inline assembly calls for
 * ease of development.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <cpu.h>           /* Generic CPU API */
#include <panic.h>         /* Kernel Panic */
#include <stdint.h>        /* Generic int types */
#include <stddef.h>        /* Standard definition */
#include <string.h>        /* Memory manipulation */
#include <kerneloutput.h>  /* Kernel output */
#include <cpu_interrupt.h> /* Interrupt manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <x86cpu.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "CPU_X64"

/** @brief Kernel's 32 bits code segment descriptor. */
#define KERNEL_CS_32 0x08
/** @brief Kernel's 32 bits data segment descriptor. */
#define KERNEL_DS_32 0x10
/** @brief Kernel's 16 bits ode segment descriptor. */
#define KERNEL_CS_16 0x18
/** @brief Kernel's 16 bits data segment descriptor. */
#define KERNEL_DS_16 0x20
/** @brief Kernel's 64 bits ode segment descriptor. */
#define KERNEL_CS_64 0x28
/** @brief Kernel's 64 bits data segment descriptor. */
#define KERNEL_DS_64 0x30

/** @brief User's 32 bits code segment descriptor. */
#define USER_CS_32 0x38
/** @brief User's 32 bits data segment descriptor. */
#define USER_DS_32 0x40
/** @brief User's 64 bits code segment descriptor. */
#define USER_CS_64 0x48
/** @brief User's 64 bits data segment descriptor. */
#define USER_DS_64 0x50

/** @brief Kernel's TSS segment descriptor. */
#define TSS_SEGMENT  0x60

/** @brief Select the thread code segment. */
#define THREAD_KERNEL_CS KERNEL_CS_64
/** @brief Select the thread code segment. */
#define THREAD_KERNEL_DS KERNEL_DS_64

/** @brief Kernel's 64 bits code segment base address. */
#define KERNEL_CODE_SEGMENT_BASE_64  0x00000000
/** @brief Kernel's 64 bits code segment limit address. */
#define KERNEL_CODE_SEGMENT_LIMIT_64 0x000FFFFF
/** @brief Kernel's 64 bits data segment base address. */
#define KERNEL_DATA_SEGMENT_BASE_64  0x00000000
/** @brief Kernel's 64 bits data segment limit address. */
#define KERNEL_DATA_SEGMENT_LIMIT_64 0x000FFFFF

/** @brief Kernel's 32 bits code segment base address. */
#define KERNEL_CODE_SEGMENT_BASE_32  0x00000000
/** @brief Kernel's 32 bits code segment limit address. */
#define KERNEL_CODE_SEGMENT_LIMIT_32 0x000FFFFF
/** @brief Kernel's 32 bits data segment base address. */
#define KERNEL_DATA_SEGMENT_BASE_32  0x00000000
/** @brief Kernel's 32 bits data segment limit address. */
#define KERNEL_DATA_SEGMENT_LIMIT_32 0x000FFFFF

/** @brief Kernel's 16 bits code segment base address. */
#define KERNEL_CODE_SEGMENT_BASE_16  0x00000000
/** @brief Kernel's 16 bits code segment limit address. */
#define KERNEL_CODE_SEGMENT_LIMIT_16 0x000FFFFF
/** @brief Kernel's 16 bits data segment base address. */
#define KERNEL_DATA_SEGMENT_BASE_16  0x00000000
/** @brief Kernel's 16 bits data segment limit address. */
#define KERNEL_DATA_SEGMENT_LIMIT_16 0x000FFFFF

/** @brief User's 64 bits code segment base address. */
#define USER_CODE_SEGMENT_BASE_64  0x00000000
/** @brief User's 64 bits code segment limit address. */
#define USER_CODE_SEGMENT_LIMIT_64 0x000FFFFF
/** @brief User's 64 bits data segment base address. */
#define USER_DATA_SEGMENT_BASE_64  0x00000000
/** @brief User's 64 bits data segment limit address. */
#define USER_DATA_SEGMENT_LIMIT_64 0x000FFFFF

/** @brief User's 32 bits code segment base address. */
#define USER_CODE_SEGMENT_BASE_32  0x00000000
/** @brief User's 32 bits code segment limit address. */
#define USER_CODE_SEGMENT_LIMIT_32 0x000FFFFF
/** @brief User's 32 bits data segment base address. */
#define USER_DATA_SEGMENT_BASE_32  0x00000000
/** @brief User's 32 bits data segment limit address. */
#define USER_DATA_SEGMENT_LIMIT_32 0x000FFFFF

/***************************
 * GDT Flags
 **************************/

/** @brief GDT granularity flag: 4K block. */
#define GDT_FLAG_GRANULARITY_4K   0x800000
/** @brief GDT granularity flag: 1B block. */
#define GDT_FLAG_GRANULARITY_BYTE 0x000000
/** @brief GDT size flag: 16b protected mode. */
#define GDT_FLAG_16_BIT_SEGMENT   0x000000
/** @brief GDT size flag: 32b protected mode. */
#define GDT_FLAG_32_BIT_SEGMENT   0x400000
/** @brief GDT size flag: 64b protected mode. */
#define GDT_FLAG_64_BIT_SEGMENT   0x200000
/** @brief GDT AVL flag. */
#define GDT_FLAG_AVL              0x100000
/** @brief GDT segment present flag. */
#define GDT_FLAG_SEGMENT_PRESENT  0x008000
/** @brief GDT privilege level flag: Ring 0 (kernel). */
#define GDT_FLAG_PL0              0x000000
/** @brief GDT privilege level flag: Ring 1 (kernel-). */
#define GDT_FLAG_PL1              0x002000
/** @brief GDT privilege level flag: Ring 2 (kernel--). */
#define GDT_FLAG_PL2              0x004000
/** @brief GDT privilege level flag: Ring 3 (user). */
#define GDT_FLAG_PL3              0x006000
/** @brief GDT data type flag: code. */
#define GDT_FLAG_CODE_TYPE        0x001000
/** @brief GDT data type flag: data. */
#define GDT_FLAG_DATA_TYPE        0x001000
/** @brief GDT data type flag: system. */
#define GDT_FLAG_SYSTEM_TYPE      0x000000
/** @brief GDT TSS flag. */
#define GDT_FLAG_TSS              0x09

/** @brief GDT access byte flag: executable. */
#define GDT_TYPE_EXECUTABLE       0x8
/** @brief GDT access byte flag: growth direction up. */
#define GDT_TYPE_GROW_UP          0x4
/** @brief GDT access byte flag: growth direction down. */
#define GDT_TYPE_GROW_DOWN        0x0
/** @brief GDT access byte flag: conforming code. */
#define GDT_TYPE_CONFORMING       0x4
/** @brief GDT access byte flag: protected. */
#define GDT_TYPE_PROTECTED        0x0
/** @brief GDT access byte flag: readable. */
#define GDT_TYPE_READABLE         0x2
/** @brief GDT access byte flag: writable. */
#define GDT_TYPE_WRITABLE         0x2
/** @brief GDT access byte flag: accessed byte. */
#define GDT_TYPE_ACCESSED         0x1


/***************************
 * IDT Flags
 **************************/

/** @brief IDT flag: storage segment. */
#define IDT_FLAG_STORAGE_SEG 0x10
/** @brief IDT flag: privilege level, ring 0. */
#define IDT_FLAG_PL0         0x00
/** @brief IDT flag: privilege level, ring 1. */
#define IDT_FLAG_PL1         0x20
/** @brief IDT flag: privilege level, ring 2. */
#define IDT_FLAG_PL2         0x40
/** @brief IDT flag: privilege level, ring 3. */
#define IDT_FLAG_PL3         0x60
/** @brief IDT flag: interrupt present. */
#define IDT_FLAG_PRESENT     0x80

/** @brief IDT flag: interrupt type task gate. */
#define IDT_TYPE_TASK_GATE 0x05
/** @brief IDT flag: interrupt type interrupt gate. */
#define IDT_TYPE_INT_GATE  0x0E
/** @brief IDT flag: interrupt type trap gate. */
#define IDT_TYPE_TRAP_GATE 0x0F

/** @brief Number of entries in the kernel's GDT. */
#define GDT_ENTRY_COUNT (13 + MAX_CPU_COUNT)

/** @brief Request vendor string. */
#define CPUID_GETVENDORSTRING          0x00000000
/** @brief Request capabled CPUID features. */
#define CPUID_GETFEATURES              0x00000001
/** @brief Request TLB. */
#define CPUID_GETTLB                   0x00000002
/** @brief Request serial. */
#define CPUID_GETSERIAL                0x00000003
/** @brief Request extended CPUID features. */
#define CPUID_INTELEXTENDED_AVAILABLE  0x80000000
/** @brief Request Intel CPUID features. */
#define CPUID_INTELFEATURES            0x80000001
/** @brief Request Intel brand string. */
#define CPUID_INTELBRANDSTRING         0x80000002
/** @brief Request Intel brand string extended. */
#define CPUID_INTELBRANDSTRINGMORE     0x80000003
/** @brief Request Intel brand string end. */
#define CPUID_INTELBRANDSTRINGEND      0x80000004

/****************************
 * General Features
 ***************************/

/** @brief CPUID Streaming SIMD Extensions 3 flag. */
#define ECX_SSE3      (1U << 0)
/** @brief CPUID PCLMULQDQ Instruction flag. */
#define ECX_PCLMULQDQ (1U << 1)
/** @brief CPUID 64-Bit Debug Store Area flag. */
#define ECX_DTES64    (1U << 2)
/** @brief CPUID MONITOR/MWAIT flag. */
#define ECX_MONITOR   (1U << 3)
/** @brief CPUID CPL Qualified Debug Store flag. */
#define ECX_DS_CPL    (1U << 4)
/** @brief CPUID Virtual Machine Extensions flag. */
#define ECX_VMX       (1U << 5)
/** @brief CPUID Safer Mode Extensions flag. */
#define ECX_SMX       (1U << 6)
/** @brief CPUID Enhanced SpeedStep Technology flag. */
#define ECX_EST       (1U << 7)
/** @brief CPUID Thermal Monitor 2 flag. */
#define ECX_TM2       (1U << 8)
/** @brief CPUID Supplemental Streaming SIMD Extensions 3 flag. */
#define ECX_SSSE3     (1U << 9)
/** @brief CPUID L1 Context ID flag. */
#define ECX_CNXT_ID   (1U << 10)
/** @brief CPUID Fused Multiply Add flag. */
#define ECX_FMA       (1U << 12)
/** @brief CPUID CMPXCHG16B Instruction flag. */
#define ECX_CX16      (1U << 13)
/** @brief CPUID xTPR Update Control flag. */
#define ECX_XTPR      (1U << 14)
/** @brief CPUID Perf/Debug Capability MSR flag. */
#define ECX_PDCM      (1U << 15)
/** @brief CPUID Process-context Identifiers flag. */
#define ECX_PCID      (1U << 17)
/** @brief CPUID Direct Cache Access flag. */
#define ECX_DCA       (1U << 18)
/** @brief CPUID Streaming SIMD Extensions 4.1 flag. */
#define ECX_SSE41     (1U << 19)
/** @brief CPUID Streaming SIMD Extensions 4.2 flag. */
#define ECX_SSE42     (1U << 20)
/** @brief CPUID Extended xAPIC Support flag. */
#define ECX_X2APIC    (1U << 21)
/** @brief CPUID MOVBE Instruction flag. */
#define ECX_MOVBE     (1U << 22)
/** @brief CPUID POPCNT Instruction flag. */
#define ECX_POPCNT    (1U << 23)
/** @brief CPUID Local APIC supports TSC Deadline flag. */
#define ECX_TSC       (1U << 24)
/** @brief CPUID AESNI Instruction flag. */
#define ECX_AESNI     (1U << 25)
/** @brief CPUID XSAVE/XSTOR States flag. */
#define ECX_XSAVE     (1U << 26)
/** @brief CPUID OS Enabled Extended State Management flag. */
#define ECX_OSXSAVE   (1U << 27)
/** @brief CPUID AVX Instructions flag. */
#define ECX_AVX       (1U << 28)
/** @brief CPUID 16-bit Floating Point Instructions flag. */
#define ECX_F16C      (1U << 29)
/** @brief CPUID RDRAND Instruction flag. */
#define ECX_RDRAND    (1U << 30)
/** @brief CPUID Floating-Point Unit On-Chip flag. */
#define EDX_FPU       (1U << 0)
/** @brief CPUID Virtual 8086 Mode Extensions flag. */
#define EDX_VME       (1U << 1)
/** @brief CPUID Debugging Extensions flag. */
#define EDX_DE        (1U << 2)
/** @brief CPUID Page Size Extension flag. */
#define EDX_PSE       (1U << 3)
/** @brief CPUID Time Stamp Counter flag. */
#define EDX_TSC       (1U << 4)
/** @brief CPUID Model Specific Registers flag. */
#define EDX_MSR       (1U << 5)
/** @brief CPUID Physical Address Extension flag. */
#define EDX_PAE       (1U << 6)
/** @brief CPUID Machine-Check Exception flag. */
#define EDX_MCE       (1U << 7)
/** @brief CPUID CMPXCHG8 Instruction flag. */
#define EDX_CX8       (1U << 8)
/** @brief CPUID APIC On-Chip flag. */
#define EDX_APIC      (1U << 9)
/** @brief CPUID SYSENTER/SYSEXIT instructions flag. */
#define EDX_SEP       (1U << 11)
/** @brief CPUID Memory Type Range Registers flag. */
#define EDX_MTRR      (1U << 12)
/** @brief CPUID Page Global Bit flag. */
#define EDX_PGE       (1U << 13)
/** @brief CPUID Machine-Check Architecture flag. */
#define EDX_MCA       (1U << 14)
/** @brief CPUID Conditional Move Instruction flag. */
#define EDX_CMOV      (1U << 15)
/** @brief CPUID Page Attribute Table flag. */
#define EDX_PAT       (1U << 16)
/** @brief CPUID 36-bit Page Size Extension flag. */
#define EDX_PSE36     (1U << 17)
/** @brief CPUID Processor Serial Number flag. */
#define EDX_PSN       (1U << 18)
/** @brief CPUID CLFLUSH Instruction flag. */
#define EDX_CLFLUSH   (1U << 19)
/** @brief CPUID Debug Store flag. */
#define EDX_DS        (1U << 21)
/** @brief CPUID Thermal Monitor and Clock Facilities flag. */
#define EDX_ACPI      (1U << 22)
/** @brief CPUID MMX Technology flag. */
#define EDX_MMX       (1U << 23)
/** @brief CPUID FXSAVE and FXSTOR Instructions flag. */
#define EDX_FXSR      (1U << 24)
/** @brief CPUID Streaming SIMD Extensions flag. */
#define EDX_SSE       (1U << 25)
/** @brief CPUID Streaming SIMD Extensions 2 flag. */
#define EDX_SSE2      (1U << 26)
/** @brief CPUID Self Snoop flag. */
#define EDX_SS        (1U << 27)
/** @brief CPUID Multi-Threading flag. */
#define EDX_HTT       (1U << 28)
/** @brief CPUID Thermal Monitor flag. */
#define EDX_TM        (1U << 29)
/** @brief CPUID Pending Break Enable flag. */
#define EDX_PBE       (1U << 31)

/****************************
 * Extended Features
 ***************************/
/** @brief CPUID SYSCALL/SYSRET flag. */
#define EDX_SYSCALL   (1U << 11)
/** @brief CPUID Multiprocessor flag. */
#define EDX_MP        (1U << 19)
/** @brief CPUID Execute Disable Bit flag. */
#define EDX_XD        (1U << 20)
/** @brief CPUID MMX etended flag. */
#define EDX_MMX_EX    (1U << 22)
/** @brief CPUID FXSAVE/STOR available flag. */
#define EDX_FXSR     (1U << 24)
/** @brief CPUID FXSAVE/STOR optimized flag. */
#define EDX_FXSR_OPT  (1U << 25)
/** @brief CPUID 1 GB Pages flag. */
#define EDX_1GB_PAGE  (1U << 26)
/** @brief CPUID RDTSCP and IA32_TSC_AUX flag. */
#define EDX_RDTSCP    (1U << 27)
/** @brief CPUID 64-bit Architecture flag. */
#define EDX_64_BIT    (1U << 29)
/** @brief CPUID 3D Now etended flag. */
#define EDX_3DNOW_EX  (1U << 30)
/** @brief CPUID 3D Now flag. */
#define EDX_3DNOW     (1U << 31)
/** @brief CPUID LAHF Available in long mode flag */
#define ECX_LAHF_LM   (1U << 0)
/** @brief CPUID Hyperthreading not valid flag */
#define ECX_CMP_LEG   (1U << 1)
/** @brief CPUID Secure Virtual Machine flag */
#define ECX_SVM       (1U << 2)
/** @brief CPUID Extended API space flag */
#define ECX_EXTAPIC   (1U << 3)
/** @brief CPUID CR8 in protected mode flag */
#define ECX_CR8_LEG   (1U << 4)
/** @brief CPUID ABM available flag */
#define ECX_ABM       (1U << 5)
/** @brief CPUID SSE4A flag */
#define ECX_SSE4A     (1U << 6)
/** @brief CPUID Misaligne SSE mode flag */
#define ECX_MISASSE   (1U << 7)
/** @brief CPUID Prefetch flag */
#define ECX_PREFETCH  (1U << 8)
/** @brief CPUID OS Visible workaround flag */
#define ECX_OSVW      (1U << 9)
/** @brief CPUID Instruction based sampling flag */
#define ECX_IBS       (1U << 10)
/** @brief CPUID XIO intruction set flag */
#define ECX_XOP       (1U << 11)
/** @brief CPUID SKINIT instructions flag */
#define ECX_SKINIT    (1U << 12)
/** @brief CPUID watchdog timer flag */
#define ECX_WDT       (1U << 13)
/** @brief CPUID Light weight profiling flag */
#define ECX_LWP       (1U << 15)
/** @brief CPUID 4 operand fuxed multiply add flag */
#define ECX_FMA4      (1U << 16)
/** @brief CPUID Translation cache extension flag */
#define ECX_TCE       (1U << 17)
/** @brief CPUID NODE_ID MSR flag */
#define ECX_NODEIDMSR (1U << 19)
/** @brief CPUID Trailing bit manipulation flag */
#define ECX_TBM       (1U << 21)
/** @brief CPUID Topology extension flag */
#define ECX_TOPOEX    (1U << 22)
/** @brief CPUID Core performance counter extensions flag */
#define ECX_PERF_CORE (1U << 23)
/** @brief CPUID NB performance counter extensions flag */
#define ECX_PERF_NB   (1U << 24)
/** @brief CPUID Data breakpoint extensions flag */
#define ECX_DBX       (1U << 26)
/** @brief CPUID Performance TSC flag */
#define ECX_PERF_TSC  (1U << 27)
/** @brief CPUID L2I perf counter extensions flag */
#define ECX_PCX_L2I   (1U << 28)

/****************************
 * CPU Vendor signatures
 ***************************/

/** @brief CPUID Vendor signature AMD EBX. */
#define SIG_AMD_EBX 0x68747541
/** @brief CPUID Vendor signature AMD ECX. */
#define SIG_AMD_ECX 0x444d4163
/** @brief CPUID Vendor signature AMD EDX. */
#define SIG_AMD_EDX 0x69746e65

/** @brief CPUID Vendor signature Centaur EBX. */
#define SIG_CENTAUR_EBX   0x746e6543
/** @brief CPUID Vendor signature Centaur ECX. */
#define SIG_CENTAUR_ECX   0x736c7561
/** @brief CPUID Vendor signature Centaur EDX. */
#define SIG_CENTAUR_EDX   0x48727561

/** @brief CPUID Vendor signature Cyrix EBX. */
#define SIG_CYRIX_EBX 0x69727943
/** @brief CPUID Vendor signature Cyrix ECX. */
#define SIG_CYRIX_ECX 0x64616574
/** @brief CPUID Vendor signature Cyrix EDX. */
#define SIG_CYRIX_EDX 0x736e4978

/** @brief CPUID Vendor signature Intel EBX. */
#define SIG_INTEL_EBX 0x756e6547
/** @brief CPUID Vendor signature Intel ECX. */
#define SIG_INTEL_ECX 0x6c65746e
/** @brief CPUID Vendor signature Intel EDX. */
#define SIG_INTEL_EDX 0x49656e69

/** @brief CPUID Vendor signature TM1 EBX. */
#define SIG_TM1_EBX   0x6e617254
/** @brief CPUID Vendor signature TM1 ECX. */
#define SIG_TM1_ECX   0x55504361
/** @brief CPUID Vendor signature TM1 EDX. */
#define SIG_TM1_EDX   0x74656d73

/** @brief CPUID Vendor signature TM2 EBX. */
#define SIG_TM2_EBX   0x756e6547
/** @brief CPUID Vendor signature TM2 ECX. */
#define SIG_TM2_ECX   0x3638784d
/** @brief CPUID Vendor signature TM2 EDX. */
#define SIG_TM2_EDX   0x54656e69

/** @brief CPUID Vendor signature NSC EBX. */
#define SIG_NSC_EBX   0x646f6547
/** @brief CPUID Vendor signature NSC ECX. */
#define SIG_NSC_ECX   0x43534e20
/** @brief CPUID Vendor signature NSC EDX. */
#define SIG_NSC_EDX   0x79622065

/** @brief CPUID Vendor signature NextGen EBX. */
#define SIG_NEXGEN_EBX    0x4778654e
/** @brief CPUID Vendor signature NextGen ECX. */
#define SIG_NEXGEN_ECX    0x6e657669
/** @brief CPUID Vendor signature NextGen EDX. */
#define SIG_NEXGEN_EDX    0x72446e65

/** @brief CPUID Vendor signature Rise EBX. */
#define SIG_RISE_EBX  0x65736952
/** @brief CPUID Vendor signature Rise ECX. */
#define SIG_RISE_ECX  0x65736952
/** @brief CPUID Vendor signature Rise EDX. */
#define SIG_RISE_EDX  0x65736952

/** @brief CPUID Vendor signature SIS EBX. */
#define SIG_SIS_EBX   0x20536953
/** @brief CPUID Vendor signature SIS ECX. */
#define SIG_SIS_ECX   0x20536953
/** @brief CPUID Vendor signature SIS EDX. */
#define SIG_SIS_EDX   0x20536953

/** @brief CPUID Vendor signature UMC EBX. */
#define SIG_UMC_EBX   0x20434d55
/** @brief CPUID Vendor signature UMC ECX. */
#define SIG_UMC_ECX   0x20434d55
/** @brief CPUID Vendor signature UMC EDX. */
#define SIG_UMC_EDX   0x20434d55

/** @brief CPUID Vendor signature VIA EBX. */
#define SIG_VIA_EBX   0x20414956
/** @brief CPUID Vendor signature VIA ECX. */
#define SIG_VIA_ECX   0x20414956
/** @brief CPUID Vendor signature VIA EDX. */
#define SIG_VIA_EDX   0x20414956

/** @brief CPUID Vendor signature Vortex EBX. */
#define SIG_VORTEX_EBX    0x74726f56
/** @brief CPUID Vendor signature Vortex ECX. */
#define SIG_VORTEX_ECX    0x436f5320
/** @brief CPUID Vendor signature Vortex EDX. */
#define SIG_VORTEX_EDX    0x36387865

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/**
 * @brief CPU IDT entry. Describes an entry in the IDT.
 */
typedef struct
{
    /** @brief ISR low address. */
    uint16_t offLow;

    /** @brief Code segment selector. */
    uint16_t cSel;

    /** @brief Entry IST number. */
    uint8_t  ist;

    /** @brief Entry flags. */
    uint8_t  flags;

    /** @brief ISR middle address. */
    uint16_t offMid;

    /** @brief ISR high address. */
    uint32_t offHi;

    /** @brief Must be zero. */
    uint32_t reserved;
} cpu_idt_entry_t;

/**
 * @brief Define the GDT pointer, contains the  address and limit of the GDT.
 */
typedef struct
{
    /** @brief The GDT size. */
    uint16_t  size;

    /** @brief The GDT address. */
    uintptr_t base;
}__attribute__((packed)) gdt_ptr_t;

/**
 * @brief Define the IDT pointer, contains the  address and limit of the IDT.
 */
typedef struct
{
    /** @brief The IDT size. */
    uint16_t  size;

    /** @brief The IDT address. */
    uintptr_t base;
}__attribute__((packed)) idt_ptr_t;


/**
 * @brief CPU TSS abstraction structure. This is the representation the kernel
 * has of an intel's TSS entry.
 */
typedef struct cpu_tss_entry
{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t ioMapBase;
    uint16_t reserved3;
} __attribute__((__packed__)) cpu_tss_entry_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the CPU to ensure correctness of execution.
 *
 * @details Assert macro used by the CPU to ensure correctness of execution.
 * Due to the critical nature of the CPU, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define CPU_ASSERT(COND, MSG, ERROR) {                      \
    if((COND) == FALSE)                                     \
    {                                                       \
        PANIC(ERROR, "CPU", MSG, TRUE);                     \
    }                                                       \
}

/**
 * @brief Concatenates two string starting from a given index.
 *
 * @details Concatenates two string starting from a given index. The macro will
 * copy the string contained in the STR to BUFF starting with an offset of IDX.
 * IDX is updated to be equal to the position of the last chacacter copied
 * to the buffer.
 *
 * @param[out] BUFF The buffer used to receive the string.
 * @param[in, out] IDX The offset in BUFF to start copying the string to.
 * @param[in] STR The string to concatenate to BUFF.
 */
#define CONCAT_STR(BUFF, IDX, STR) {                        \
        strcpy(BUFF + IDX, STR);                            \
        IDX += strlen(STR);                                 \
}

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/** @brief Kernel stacks base symbol. */
extern int8_t _KERNEL_STACKS_BASE;

/**
 * @brief Assembly interrupt handler for line 0.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler0(void);
/**
 * @brief Assembly interrupt handler for line 1.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler1(void);
/**
 * @brief Assembly interrupt handler for line 2.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler2(void);
/**
 * @brief Assembly interrupt handler for line 3.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler3(void);
/**
 * @brief Assembly interrupt handler for line 4.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler4(void);
/**
 * @brief Assembly interrupt handler for line 5.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler5(void);
/**
 * @brief Assembly interrupt handler for line 6.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler6(void);
/**
 * @brief Assembly interrupt handler for line 7.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler7(void);
/**
 * @brief Assembly interrupt handler for line 8.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler8(void);
/**
 * @brief Assembly interrupt handler for line 9.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler9(void);
/**
 * @brief Assembly interrupt handler for line 10.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler10(void);
/**
 * @brief Assembly interrupt handler for line 11.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler11(void);
/**
 * @brief Assembly interrupt handler for line 12.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler12(void);
/**
 * @brief Assembly interrupt handler for line 13.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler13(void);
/**
 * @brief Assembly interrupt handler for line 14.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler14(void);
/**
 * @brief Assembly interrupt handler for line 15.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler15(void);
/**
 * @brief Assembly interrupt handler for line 16.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler16(void);
/**
 * @brief Assembly interrupt handler for line 17.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler17(void);
/**
 * @brief Assembly interrupt handler for line 18.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler18(void);
/**
 * @brief Assembly interrupt handler for line 19.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler19(void);
/**
 * @brief Assembly interrupt handler for line 20.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler20(void);
/**
 * @brief Assembly interrupt handler for line 21.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler21(void);
/**
 * @brief Assembly interrupt handler for line 22.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler22(void);
/**
 * @brief Assembly interrupt handler for line 23.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler23(void);
/**
 * @brief Assembly interrupt handler for line 24.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler24(void);
/**
 * @brief Assembly interrupt handler for line 25.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler25(void);
/**
 * @brief Assembly interrupt handler for line 26.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler26(void);
/**
 * @brief Assembly interrupt handler for line 27.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler27(void);
/**
 * @brief Assembly interrupt handler for line 28.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler28(void);
/**
 * @brief Assembly interrupt handler for line 29.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler29(void);
/**
 * @brief Assembly interrupt handler for line 30.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler30(void);
/**
 * @brief Assembly interrupt handler for line 31.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler31(void);
/**
 * @brief Assembly interrupt handler for line 32.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler32(void);
/**
 * @brief Assembly interrupt handler for line 33.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler33(void);
/**
 * @brief Assembly interrupt handler for line 34.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler34(void);
/**
 * @brief Assembly interrupt handler for line 35.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler35(void);
/**
 * @brief Assembly interrupt handler for line 36.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler36(void);
/**
 * @brief Assembly interrupt handler for line 37.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler37(void);
/**
 * @brief Assembly interrupt handler for line 38.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler38(void);
/**
 * @brief Assembly interrupt handler for line 39.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler39(void);
/**
 * @brief Assembly interrupt handler for line 40.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler40(void);
/**
 * @brief Assembly interrupt handler for line 41.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler41(void);
/**
 * @brief Assembly interrupt handler for line 42.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler42(void);
/**
 * @brief Assembly interrupt handler for line 43.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler43(void);
/**
 * @brief Assembly interrupt handler for line 44.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler44(void);
/**
 * @brief Assembly interrupt handler for line 45.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler45(void);
/**
 * @brief Assembly interrupt handler for line 46.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler46(void);
/**
 * @brief Assembly interrupt handler for line 47.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler47(void);
/**
 * @brief Assembly interrupt handler for line 48.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler48(void);
/**
 * @brief Assembly interrupt handler for line 49.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler49(void);
/**
 * @brief Assembly interrupt handler for line 50.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler50(void);
/**
 * @brief Assembly interrupt handler for line 51.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler51(void);
/**
 * @brief Assembly interrupt handler for line 52.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler52(void);
/**
 * @brief Assembly interrupt handler for line 53.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler53(void);
/**
 * @brief Assembly interrupt handler for line 54.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler54(void);
/**
 * @brief Assembly interrupt handler for line 55.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler55(void);
/**
 * @brief Assembly interrupt handler for line 56.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler56(void);
/**
 * @brief Assembly interrupt handler for line 57.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler57(void);
/**
 * @brief Assembly interrupt handler for line 58.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler58(void);
/**
 * @brief Assembly interrupt handler for line 59.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler59(void);
/**
 * @brief Assembly interrupt handler for line 60.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler60(void);
/**
 * @brief Assembly interrupt handler for line 61.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler61(void);
/**
 * @brief Assembly interrupt handler for line 62.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler62(void);
/**
 * @brief Assembly interrupt handler for line 63.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler63(void);
/**
 * @brief Assembly interrupt handler for line 64.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler64(void);
/**
 * @brief Assembly interrupt handler for line 65.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler65(void);
/**
 * @brief Assembly interrupt handler for line 66.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler66(void);
/**
 * @brief Assembly interrupt handler for line 67.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler67(void);
/**
 * @brief Assembly interrupt handler for line 68.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler68(void);
/**
 * @brief Assembly interrupt handler for line 69.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler69(void);
/**
 * @brief Assembly interrupt handler for line 70.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler70(void);
/**
 * @brief Assembly interrupt handler for line 71.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler71(void);
/**
 * @brief Assembly interrupt handler for line 72.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler72(void);
/**
 * @brief Assembly interrupt handler for line 73.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler73(void);
/**
 * @brief Assembly interrupt handler for line 74.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler74(void);
/**
 * @brief Assembly interrupt handler for line 75.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler75(void);
/**
 * @brief Assembly interrupt handler for line 76.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler76(void);
/**
 * @brief Assembly interrupt handler for line 77.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler77(void);
/**
 * @brief Assembly interrupt handler for line 78.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler78(void);
/**
 * @brief Assembly interrupt handler for line 79.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler79(void);
/**
 * @brief Assembly interrupt handler for line 80.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler80(void);
/**
 * @brief Assembly interrupt handler for line 81.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler81(void);
/**
 * @brief Assembly interrupt handler for line 82.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler82(void);
/**
 * @brief Assembly interrupt handler for line 83.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler83(void);
/**
 * @brief Assembly interrupt handler for line 84.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler84(void);
/**
 * @brief Assembly interrupt handler for line 85.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler85(void);
/**
 * @brief Assembly interrupt handler for line 86.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler86(void);
/**
 * @brief Assembly interrupt handler for line 87.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler87(void);
/**
 * @brief Assembly interrupt handler for line 88.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler88(void);
/**
 * @brief Assembly interrupt handler for line 89.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler89(void);
/**
 * @brief Assembly interrupt handler for line 90.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler90(void);
/**
 * @brief Assembly interrupt handler for line 91.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler91(void);
/**
 * @brief Assembly interrupt handler for line 92.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler92(void);
/**
 * @brief Assembly interrupt handler for line 93.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler93(void);
/**
 * @brief Assembly interrupt handler for line 94.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler94(void);
/**
 * @brief Assembly interrupt handler for line 95.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler95(void);
/**
 * @brief Assembly interrupt handler for line 96.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler96(void);
/**
 * @brief Assembly interrupt handler for line 97.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler97(void);
/**
 * @brief Assembly interrupt handler for line 98.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler98(void);
/**
 * @brief Assembly interrupt handler for line 99.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler99(void);
/**
 * @brief Assembly interrupt handler for line 100.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler100(void);
/**
 * @brief Assembly interrupt handler for line 101.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler101(void);
/**
 * @brief Assembly interrupt handler for line 102.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler102(void);
/**
 * @brief Assembly interrupt handler for line 103.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler103(void);
/**
 * @brief Assembly interrupt handler for line 104.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler104(void);
/**
 * @brief Assembly interrupt handler for line 105.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler105(void);
/**
 * @brief Assembly interrupt handler for line 106.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler106(void);
/**
 * @brief Assembly interrupt handler for line 107.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler107(void);
/**
 * @brief Assembly interrupt handler for line 108.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler108(void);
/**
 * @brief Assembly interrupt handler for line 109.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler109(void);
/**
 * @brief Assembly interrupt handler for line 110.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler110(void);
/**
 * @brief Assembly interrupt handler for line 111.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler111(void);
/**
 * @brief Assembly interrupt handler for line 112.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler112(void);
/**
 * @brief Assembly interrupt handler for line 113.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler113(void);
/**
 * @brief Assembly interrupt handler for line 114.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler114(void);
/**
 * @brief Assembly interrupt handler for line 115.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler115(void);
/**
 * @brief Assembly interrupt handler for line 116.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler116(void);
/**
 * @brief Assembly interrupt handler for line 117.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler117(void);
/**
 * @brief Assembly interrupt handler for line 118.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler118(void);
/**
 * @brief Assembly interrupt handler for line 119.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler119(void);
/**
 * @brief Assembly interrupt handler for line 120.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler120(void);
/**
 * @brief Assembly interrupt handler for line 121.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler121(void);
/**
 * @brief Assembly interrupt handler for line 122.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler122(void);
/**
 * @brief Assembly interrupt handler for line 123.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler123(void);
/**
 * @brief Assembly interrupt handler for line 124.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler124(void);
/**
 * @brief Assembly interrupt handler for line 125.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler125(void);
/**
 * @brief Assembly interrupt handler for line 126.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler126(void);
/**
 * @brief Assembly interrupt handler for line 127.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler127(void);
/**
 * @brief Assembly interrupt handler for line 128.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler128(void);
/**
 * @brief Assembly interrupt handler for line 129.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler129(void);
/**
 * @brief Assembly interrupt handler for line 130.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler130(void);
/**
 * @brief Assembly interrupt handler for line 131.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler131(void);
/**
 * @brief Assembly interrupt handler for line 132.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler132(void);
/**
 * @brief Assembly interrupt handler for line 133.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler133(void);
/**
 * @brief Assembly interrupt handler for line 134.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler134(void);
/**
 * @brief Assembly interrupt handler for line 135.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler135(void);
/**
 * @brief Assembly interrupt handler for line 136.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler136(void);
/**
 * @brief Assembly interrupt handler for line 137.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler137(void);
/**
 * @brief Assembly interrupt handler for line 138.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler138(void);
/**
 * @brief Assembly interrupt handler for line 139.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler139(void);
/**
 * @brief Assembly interrupt handler for line 140.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler140(void);
/**
 * @brief Assembly interrupt handler for line 141.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler141(void);
/**
 * @brief Assembly interrupt handler for line 142.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler142(void);
/**
 * @brief Assembly interrupt handler for line 143.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler143(void);
/**
 * @brief Assembly interrupt handler for line 144.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler144(void);
/**
 * @brief Assembly interrupt handler for line 145.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler145(void);
/**
 * @brief Assembly interrupt handler for line 146.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler146(void);
/**
 * @brief Assembly interrupt handler for line 147.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler147(void);
/**
 * @brief Assembly interrupt handler for line 148.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler148(void);
/**
 * @brief Assembly interrupt handler for line 149.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler149(void);
/**
 * @brief Assembly interrupt handler for line 150.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler150(void);
/**
 * @brief Assembly interrupt handler for line 151.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler151(void);
/**
 * @brief Assembly interrupt handler for line 152.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler152(void);
/**
 * @brief Assembly interrupt handler for line 153.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler153(void);
/**
 * @brief Assembly interrupt handler for line 154.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler154(void);
/**
 * @brief Assembly interrupt handler for line 155.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler155(void);
/**
 * @brief Assembly interrupt handler for line 156.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler156(void);
/**
 * @brief Assembly interrupt handler for line 157.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler157(void);
/**
 * @brief Assembly interrupt handler for line 158.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler158(void);
/**
 * @brief Assembly interrupt handler for line 159.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler159(void);
/**
 * @brief Assembly interrupt handler for line 160.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler160(void);
/**
 * @brief Assembly interrupt handler for line 161.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler161(void);
/**
 * @brief Assembly interrupt handler for line 162.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler162(void);
/**
 * @brief Assembly interrupt handler for line 163.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler163(void);
/**
 * @brief Assembly interrupt handler for line 164.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler164(void);
/**
 * @brief Assembly interrupt handler for line 165.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler165(void);
/**
 * @brief Assembly interrupt handler for line 166.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler166(void);
/**
 * @brief Assembly interrupt handler for line 167.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler167(void);
/**
 * @brief Assembly interrupt handler for line 168.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler168(void);
/**
 * @brief Assembly interrupt handler for line 169.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler169(void);
/**
 * @brief Assembly interrupt handler for line 170.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler170(void);
/**
 * @brief Assembly interrupt handler for line 171.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler171(void);
/**
 * @brief Assembly interrupt handler for line 172.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler172(void);
/**
 * @brief Assembly interrupt handler for line 173.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler173(void);
/**
 * @brief Assembly interrupt handler for line 174.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler174(void);
/**
 * @brief Assembly interrupt handler for line 175.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler175(void);
/**
 * @brief Assembly interrupt handler for line 176.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler176(void);
/**
 * @brief Assembly interrupt handler for line 177.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler177(void);
/**
 * @brief Assembly interrupt handler for line 178.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler178(void);
/**
 * @brief Assembly interrupt handler for line 179.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler179(void);
/**
 * @brief Assembly interrupt handler for line 180.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler180(void);
/**
 * @brief Assembly interrupt handler for line 181.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler181(void);
/**
 * @brief Assembly interrupt handler for line 182.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler182(void);
/**
 * @brief Assembly interrupt handler for line 183.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler183(void);
/**
 * @brief Assembly interrupt handler for line 184.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler184(void);
/**
 * @brief Assembly interrupt handler for line 185.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler185(void);
/**
 * @brief Assembly interrupt handler for line 186.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler186(void);
/**
 * @brief Assembly interrupt handler for line 187.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler187(void);
/**
 * @brief Assembly interrupt handler for line 188.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler188(void);
/**
 * @brief Assembly interrupt handler for line 189.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler189(void);
/**
 * @brief Assembly interrupt handler for line 190.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler190(void);
/**
 * @brief Assembly interrupt handler for line 191.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler191(void);
/**
 * @brief Assembly interrupt handler for line 192.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler192(void);
/**
 * @brief Assembly interrupt handler for line 193.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler193(void);
/**
 * @brief Assembly interrupt handler for line 194.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler194(void);
/**
 * @brief Assembly interrupt handler for line 195.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler195(void);
/**
 * @brief Assembly interrupt handler for line 196.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler196(void);
/**
 * @brief Assembly interrupt handler for line 197.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler197(void);
/**
 * @brief Assembly interrupt handler for line 198.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler198(void);
/**
 * @brief Assembly interrupt handler for line 199.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler199(void);
/**
 * @brief Assembly interrupt handler for line 200.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler200(void);
/**
 * @brief Assembly interrupt handler for line 201.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler201(void);
/**
 * @brief Assembly interrupt handler for line 202.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler202(void);
/**
 * @brief Assembly interrupt handler for line 203.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler203(void);
/**
 * @brief Assembly interrupt handler for line 204.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler204(void);
/**
 * @brief Assembly interrupt handler for line 205.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler205(void);
/**
 * @brief Assembly interrupt handler for line 206.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler206(void);
/**
 * @brief Assembly interrupt handler for line 207.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler207(void);
/**
 * @brief Assembly interrupt handler for line 208.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler208(void);
/**
 * @brief Assembly interrupt handler for line 209.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler209(void);
/**
 * @brief Assembly interrupt handler for line 210.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler210(void);
/**
 * @brief Assembly interrupt handler for line 211.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler211(void);
/**
 * @brief Assembly interrupt handler for line 212.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler212(void);
/**
 * @brief Assembly interrupt handler for line 213.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler213(void);
/**
 * @brief Assembly interrupt handler for line 214.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler214(void);
/**
 * @brief Assembly interrupt handler for line 215.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler215(void);
/**
 * @brief Assembly interrupt handler for line 216.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler216(void);
/**
 * @brief Assembly interrupt handler for line 217.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler217(void);
/**
 * @brief Assembly interrupt handler for line 218.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler218(void);
/**
 * @brief Assembly interrupt handler for line 219.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler219(void);
/**
 * @brief Assembly interrupt handler for line 220.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler220(void);
/**
 * @brief Assembly interrupt handler for line 221.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler221(void);
/**
 * @brief Assembly interrupt handler for line 222.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler222(void);
/**
 * @brief Assembly interrupt handler for line 223.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler223(void);
/**
 * @brief Assembly interrupt handler for line 224.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler224(void);
/**
 * @brief Assembly interrupt handler for line 225.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler225(void);
/**
 * @brief Assembly interrupt handler for line 226.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler226(void);
/**
 * @brief Assembly interrupt handler for line 227.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler227(void);
/**
 * @brief Assembly interrupt handler for line 228.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler228(void);
/**
 * @brief Assembly interrupt handler for line 229.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler229(void);
/**
 * @brief Assembly interrupt handler for line 230.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler230(void);
/**
 * @brief Assembly interrupt handler for line 231.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler231(void);
/**
 * @brief Assembly interrupt handler for line 232.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler232(void);
/**
 * @brief Assembly interrupt handler for line 233.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler233(void);
/**
 * @brief Assembly interrupt handler for line 234.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler234(void);
/**
 * @brief Assembly interrupt handler for line 235.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler235(void);
/**
 * @brief Assembly interrupt handler for line 236.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler236(void);
/**
 * @brief Assembly interrupt handler for line 237.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler237(void);
/**
 * @brief Assembly interrupt handler for line 238.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler238(void);
/**
 * @brief Assembly interrupt handler for line 239.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler239(void);
/**
 * @brief Assembly interrupt handler for line 240.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler240(void);
/**
 * @brief Assembly interrupt handler for line 241.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler241(void);
/**
 * @brief Assembly interrupt handler for line 242.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler242(void);
/**
 * @brief Assembly interrupt handler for line 243.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler243(void);
/**
 * @brief Assembly interrupt handler for line 244.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler244(void);
/**
 * @brief Assembly interrupt handler for line 245.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler245(void);
/**
 * @brief Assembly interrupt handler for line 246.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler246(void);
/**
 * @brief Assembly interrupt handler for line 247.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler247(void);
/**
 * @brief Assembly interrupt handler for line 248.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler248(void);
/**
 * @brief Assembly interrupt handler for line 249.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler249(void);
/**
 * @brief Assembly interrupt handler for line 250.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler250(void);
/**
 * @brief Assembly interrupt handler for line 251.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler251(void);
/**
 * @brief Assembly interrupt handler for line 252.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler252(void);
/**
 * @brief Assembly interrupt handler for line 253.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler253(void);
/**
 * @brief Assembly interrupt handler for line 254.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler254(void);
/**
 * @brief Assembly interrupt handler for line 255.
 * Saves the context and calls the generic interrupt handler
 */
extern void __intHandler255(void);

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief CPU GDT space in memory. */
static uint64_t sGDT[GDT_ENTRY_COUNT]        __attribute__((aligned(8)));
/** @brief Kernel GDT structure */
static gdt_ptr_t sGDTPtr                     __attribute__((aligned(8)));

/** @brief CPU IDT space in memory. */
static cpu_idt_entry_t sIDT[IDT_ENTRY_COUNT] __attribute__((aligned(8)));
/** @brief Kernel IDT structure */
static idt_ptr_t sIDTPtr                     __attribute__((aligned(8)));

/** @brief CPU TSS structures */
static cpu_tss_entry_t sTSS[MAX_CPU_COUNT]   __attribute__((aligned(8)));

/** @brief Stores the CPU interrupt handlers entry point */
static uintptr_t sIntHandlerTable[IDT_ENTRY_COUNT] = {
    (uintptr_t)__intHandler0,
    (uintptr_t)__intHandler1,
    (uintptr_t)__intHandler2,
    (uintptr_t)__intHandler3,
    (uintptr_t)__intHandler4,
    (uintptr_t)__intHandler5,
    (uintptr_t)__intHandler6,
    (uintptr_t)__intHandler7,
    (uintptr_t)__intHandler8,
    (uintptr_t)__intHandler9,
    (uintptr_t)__intHandler10,
    (uintptr_t)__intHandler11,
    (uintptr_t)__intHandler12,
    (uintptr_t)__intHandler13,
    (uintptr_t)__intHandler14,
    (uintptr_t)__intHandler15,
    (uintptr_t)__intHandler16,
    (uintptr_t)__intHandler17,
    (uintptr_t)__intHandler18,
    (uintptr_t)__intHandler19,
    (uintptr_t)__intHandler20,
    (uintptr_t)__intHandler21,
    (uintptr_t)__intHandler22,
    (uintptr_t)__intHandler23,
    (uintptr_t)__intHandler24,
    (uintptr_t)__intHandler25,
    (uintptr_t)__intHandler26,
    (uintptr_t)__intHandler27,
    (uintptr_t)__intHandler28,
    (uintptr_t)__intHandler29,
    (uintptr_t)__intHandler30,
    (uintptr_t)__intHandler31,
    (uintptr_t)__intHandler32,
    (uintptr_t)__intHandler33,
    (uintptr_t)__intHandler34,
    (uintptr_t)__intHandler35,
    (uintptr_t)__intHandler36,
    (uintptr_t)__intHandler37,
    (uintptr_t)__intHandler38,
    (uintptr_t)__intHandler39,
    (uintptr_t)__intHandler40,
    (uintptr_t)__intHandler41,
    (uintptr_t)__intHandler42,
    (uintptr_t)__intHandler43,
    (uintptr_t)__intHandler44,
    (uintptr_t)__intHandler45,
    (uintptr_t)__intHandler46,
    (uintptr_t)__intHandler47,
    (uintptr_t)__intHandler48,
    (uintptr_t)__intHandler49,
    (uintptr_t)__intHandler50,
    (uintptr_t)__intHandler51,
    (uintptr_t)__intHandler52,
    (uintptr_t)__intHandler53,
    (uintptr_t)__intHandler54,
    (uintptr_t)__intHandler55,
    (uintptr_t)__intHandler56,
    (uintptr_t)__intHandler57,
    (uintptr_t)__intHandler58,
    (uintptr_t)__intHandler59,
    (uintptr_t)__intHandler60,
    (uintptr_t)__intHandler61,
    (uintptr_t)__intHandler62,
    (uintptr_t)__intHandler63,
    (uintptr_t)__intHandler64,
    (uintptr_t)__intHandler65,
    (uintptr_t)__intHandler66,
    (uintptr_t)__intHandler67,
    (uintptr_t)__intHandler68,
    (uintptr_t)__intHandler69,
    (uintptr_t)__intHandler70,
    (uintptr_t)__intHandler71,
    (uintptr_t)__intHandler72,
    (uintptr_t)__intHandler73,
    (uintptr_t)__intHandler74,
    (uintptr_t)__intHandler75,
    (uintptr_t)__intHandler76,
    (uintptr_t)__intHandler77,
    (uintptr_t)__intHandler78,
    (uintptr_t)__intHandler79,
    (uintptr_t)__intHandler80,
    (uintptr_t)__intHandler81,
    (uintptr_t)__intHandler82,
    (uintptr_t)__intHandler83,
    (uintptr_t)__intHandler84,
    (uintptr_t)__intHandler85,
    (uintptr_t)__intHandler86,
    (uintptr_t)__intHandler87,
    (uintptr_t)__intHandler88,
    (uintptr_t)__intHandler89,
    (uintptr_t)__intHandler90,
    (uintptr_t)__intHandler91,
    (uintptr_t)__intHandler92,
    (uintptr_t)__intHandler93,
    (uintptr_t)__intHandler94,
    (uintptr_t)__intHandler95,
    (uintptr_t)__intHandler96,
    (uintptr_t)__intHandler97,
    (uintptr_t)__intHandler98,
    (uintptr_t)__intHandler99,
    (uintptr_t)__intHandler100,
    (uintptr_t)__intHandler101,
    (uintptr_t)__intHandler102,
    (uintptr_t)__intHandler103,
    (uintptr_t)__intHandler104,
    (uintptr_t)__intHandler105,
    (uintptr_t)__intHandler106,
    (uintptr_t)__intHandler107,
    (uintptr_t)__intHandler108,
    (uintptr_t)__intHandler109,
    (uintptr_t)__intHandler110,
    (uintptr_t)__intHandler111,
    (uintptr_t)__intHandler112,
    (uintptr_t)__intHandler113,
    (uintptr_t)__intHandler114,
    (uintptr_t)__intHandler115,
    (uintptr_t)__intHandler116,
    (uintptr_t)__intHandler117,
    (uintptr_t)__intHandler118,
    (uintptr_t)__intHandler119,
    (uintptr_t)__intHandler120,
    (uintptr_t)__intHandler121,
    (uintptr_t)__intHandler122,
    (uintptr_t)__intHandler123,
    (uintptr_t)__intHandler124,
    (uintptr_t)__intHandler125,
    (uintptr_t)__intHandler126,
    (uintptr_t)__intHandler127,
    (uintptr_t)__intHandler128,
    (uintptr_t)__intHandler129,
    (uintptr_t)__intHandler130,
    (uintptr_t)__intHandler131,
    (uintptr_t)__intHandler132,
    (uintptr_t)__intHandler133,
    (uintptr_t)__intHandler134,
    (uintptr_t)__intHandler135,
    (uintptr_t)__intHandler136,
    (uintptr_t)__intHandler137,
    (uintptr_t)__intHandler138,
    (uintptr_t)__intHandler139,
    (uintptr_t)__intHandler140,
    (uintptr_t)__intHandler141,
    (uintptr_t)__intHandler142,
    (uintptr_t)__intHandler143,
    (uintptr_t)__intHandler144,
    (uintptr_t)__intHandler145,
    (uintptr_t)__intHandler146,
    (uintptr_t)__intHandler147,
    (uintptr_t)__intHandler148,
    (uintptr_t)__intHandler149,
    (uintptr_t)__intHandler150,
    (uintptr_t)__intHandler151,
    (uintptr_t)__intHandler152,
    (uintptr_t)__intHandler153,
    (uintptr_t)__intHandler154,
    (uintptr_t)__intHandler155,
    (uintptr_t)__intHandler156,
    (uintptr_t)__intHandler157,
    (uintptr_t)__intHandler158,
    (uintptr_t)__intHandler159,
    (uintptr_t)__intHandler160,
    (uintptr_t)__intHandler161,
    (uintptr_t)__intHandler162,
    (uintptr_t)__intHandler163,
    (uintptr_t)__intHandler164,
    (uintptr_t)__intHandler165,
    (uintptr_t)__intHandler166,
    (uintptr_t)__intHandler167,
    (uintptr_t)__intHandler168,
    (uintptr_t)__intHandler169,
    (uintptr_t)__intHandler170,
    (uintptr_t)__intHandler171,
    (uintptr_t)__intHandler172,
    (uintptr_t)__intHandler173,
    (uintptr_t)__intHandler174,
    (uintptr_t)__intHandler175,
    (uintptr_t)__intHandler176,
    (uintptr_t)__intHandler177,
    (uintptr_t)__intHandler178,
    (uintptr_t)__intHandler179,
    (uintptr_t)__intHandler180,
    (uintptr_t)__intHandler181,
    (uintptr_t)__intHandler182,
    (uintptr_t)__intHandler183,
    (uintptr_t)__intHandler184,
    (uintptr_t)__intHandler185,
    (uintptr_t)__intHandler186,
    (uintptr_t)__intHandler187,
    (uintptr_t)__intHandler188,
    (uintptr_t)__intHandler189,
    (uintptr_t)__intHandler190,
    (uintptr_t)__intHandler191,
    (uintptr_t)__intHandler192,
    (uintptr_t)__intHandler193,
    (uintptr_t)__intHandler194,
    (uintptr_t)__intHandler195,
    (uintptr_t)__intHandler196,
    (uintptr_t)__intHandler197,
    (uintptr_t)__intHandler198,
    (uintptr_t)__intHandler199,
    (uintptr_t)__intHandler200,
    (uintptr_t)__intHandler201,
    (uintptr_t)__intHandler202,
    (uintptr_t)__intHandler203,
    (uintptr_t)__intHandler204,
    (uintptr_t)__intHandler205,
    (uintptr_t)__intHandler206,
    (uintptr_t)__intHandler207,
    (uintptr_t)__intHandler208,
    (uintptr_t)__intHandler209,
    (uintptr_t)__intHandler210,
    (uintptr_t)__intHandler211,
    (uintptr_t)__intHandler212,
    (uintptr_t)__intHandler213,
    (uintptr_t)__intHandler214,
    (uintptr_t)__intHandler215,
    (uintptr_t)__intHandler216,
    (uintptr_t)__intHandler217,
    (uintptr_t)__intHandler218,
    (uintptr_t)__intHandler219,
    (uintptr_t)__intHandler220,
    (uintptr_t)__intHandler221,
    (uintptr_t)__intHandler222,
    (uintptr_t)__intHandler223,
    (uintptr_t)__intHandler224,
    (uintptr_t)__intHandler225,
    (uintptr_t)__intHandler226,
    (uintptr_t)__intHandler227,
    (uintptr_t)__intHandler228,
    (uintptr_t)__intHandler229,
    (uintptr_t)__intHandler230,
    (uintptr_t)__intHandler231,
    (uintptr_t)__intHandler232,
    (uintptr_t)__intHandler233,
    (uintptr_t)__intHandler234,
    (uintptr_t)__intHandler235,
    (uintptr_t)__intHandler236,
    (uintptr_t)__intHandler237,
    (uintptr_t)__intHandler238,
    (uintptr_t)__intHandler239,
    (uintptr_t)__intHandler240,
    (uintptr_t)__intHandler241,
    (uintptr_t)__intHandler242,
    (uintptr_t)__intHandler243,
    (uintptr_t)__intHandler244,
    (uintptr_t)__intHandler245,
    (uintptr_t)__intHandler246,
    (uintptr_t)__intHandler247,
    (uintptr_t)__intHandler248,
    (uintptr_t)__intHandler249,
    (uintptr_t)__intHandler250,
    (uintptr_t)__intHandler251,
    (uintptr_t)__intHandler252,
    (uintptr_t)__intHandler253,
    (uintptr_t)__intHandler254,
    (uintptr_t)__intHandler255
};

/** @brief Defines the CPU interrupt configuration */
const cpu_interrupt_config_t ksInterruptConfig = {
    .minExceptionLine        = MIN_EXCEPTION_LINE,
    .maxExceptionLine        = MAX_EXCEPTION_LINE,
    .minInterruptLine        = MIN_INTERRUPT_LINE,
    .maxInterruptLine        = MAX_INTERRUPT_LINE,
    .totalInterruptLineCount = INT_ENTRY_COUNT,
    .panicInterruptLine      = PANIC_INT_LINE,
    .spuriousInterruptLine   = SPURIOUS_INT_LINE
};

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Setups the kernel's GDT in memory and loads it in the GDT register.
 *
 * @details Setups a GDT for the kernel. Fills the entries in the GDT table and
 * load the new GDT in the CPU's GDT register.
 * Once done, the function sets the segment registers (CS, DS, ES, FS, GS, SS)
 * of the CPU according to the kernel's settings.
 */
static void _setupGDT(void);

/**
 * @brief Setups the generic kernel's IDT in memory and loads it in the IDT
 * register.
 *
 * @details Setups a simple IDT for the kernel. Fills the entries in the IDT
 * table by adding basic support to the x86 exception (interrutps 0 to 32).
 * The rest of the interrupts are not set.
 */
static void _setupIDT(void);

/**
 *  @brief Setups the main CPU TSS for the kernel.
 *
 * @details Initializes the main CPU's TSS with kernel settings in memory and
 * loads it in the TSS register.
 */
static void _setupTSS(void);

/**
 * @brief Formats a GDT entry.
 *
 * @details Formats data given as parameter into a standard GDT entry.
 * The result is directly written in the memory pointed by the entry parameter.
 *
 * @param[out] pEntry The pointer to the entry structure to format.
 * @param[in] kBase  The base address of the segment for the GDT entry.
 * @param[in] kLimit The limit address of the segment for the GDT entry.
 * @param[in] kType  The type of segment for the GDT entry.
 * @param[in] kFlags The flags to be set for the GDT entry.
 */
static void _formatGDTEntry(uint64_t*      pEntry,
                            const uint32_t kBase,
                            const uint32_t kLimit,
                            const uint8_t  kType,
                            const uint32_t kFlags);

/**
 * @brief Formats an IDT entry.
 *
 * @details Formats data given as parameter into a standard IDT entry.
 * The result is directly written in the memory pointed by the entry parameter.
 *
 * @param[out] pEntry The pointer to the entry structure to format.
 * @param[in] kandler The handler function for the IDT entry.
 * @param[in] kType  The type of segment for the IDT entry.
 * @param[in] kFlags The flags to be set for the IDT entry.
 */
static void _formatIDTEntry(cpu_idt_entry_t* pEntry,
                            const uintptr_t  kHandler,
                            const uint8_t    kType,
                            const uint32_t   kFlags);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _formatGDTEntry(uint64_t*      pEntry,
                            const uint32_t kBase,
                            const uint32_t kLimit,
                            const uint8_t  kType,
                            const uint32_t kFlags)
{
    uint32_t lo_part = 0;
    uint32_t hi_part = 0;

    /*
     * Low part[31;0] = Base[15;0] Limit[15;0]
     */
    lo_part = ((kBase & 0xFFFF) << 16) | (kLimit & 0xFFFF);

    /*
     * High part[7;0] = Base[23;16]
     */
    hi_part = (kBase >> 16) & 0xFF;
    /*
     * High part[11;8] = Type[3;0]
     */
    hi_part |= (kType & 0xF) << 8;
    /*
     * High part[15;12] = Seg_Present[1;0]Privilege[2;0]Descriptor_Type[1;0]
     * High part[23;20] = Granularity[1;0]Op_Size[1;0]L[1;0]AVL[1;0]
     */
    hi_part |= kFlags & 0x00F0F000;

    /*
     * High part[19;16] = Limit[19;16]
     */
    hi_part |= kLimit & 0xF0000;
    /*
     * High part[31;24] = Base[31;24]
     */
    hi_part |= kBase & 0xFF000000;

    /* Set the value of the entry */
    *pEntry = lo_part | (((uint64_t) hi_part) << 32);
}

static void _formatIDTEntry(cpu_idt_entry_t* pEntry,
                            const uintptr_t  kHandler,
                            const uint8_t    kType,
                            const uint32_t   kFlags)
{
    /* Set offset */
    pEntry->offLow = kHandler & 0x000000000000FFFF;
    pEntry->offMid = (kHandler >> 16) & 0x000000000000FFFF;
    pEntry->offHi  = (kHandler >> 32) & 0x00000000FFFFFFFF;

    /* Set selector and flags */
    pEntry->cSel  = KERNEL_CS_64;
    pEntry->flags = (kFlags & 0xF0) | (kType & 0x0F);

    /* Zeroise */
    pEntry->ist      = 0;
    pEntry->reserved = 0;
}

static void _setupGDT(void)
{
    uint32_t i;

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED, TRACE_X86_CPU_SETUP_GDT_ENTRY, 0);

    KERNEL_DEBUG(CPU_DEBUG_ENABLED, MODULE_NAME, "Setting GDT");

    /************************************
     * KERNEL GDT ENTRIES
     ***********************************/

    /* Set the kernel 64 bits code descriptor */
    uint32_t kernelCode64SegFlags = GDT_FLAG_64_BIT_SEGMENT |
                                    GDT_FLAG_PL0 |
                                    GDT_FLAG_SEGMENT_PRESENT |
                                    GDT_FLAG_CODE_TYPE;

    uint32_t kernelCode64SegType =  GDT_TYPE_EXECUTABLE |
                                    GDT_TYPE_READABLE |
                                    GDT_TYPE_PROTECTED;

    /* Set the kernel 64 bits data descriptor */
    uint32_t kernelData64SegFlags = GDT_FLAG_64_BIT_SEGMENT |
                                    GDT_FLAG_PL0 |
                                    GDT_FLAG_SEGMENT_PRESENT |
                                    GDT_FLAG_DATA_TYPE;

    uint32_t kernelData64SegType =  GDT_TYPE_WRITABLE | GDT_TYPE_GROW_DOWN;

    /* Set the kernel code descriptor */
    uint32_t kernelCode32SegFlags = GDT_FLAG_GRANULARITY_4K |
                                    GDT_FLAG_32_BIT_SEGMENT |
                                    GDT_FLAG_PL0 |
                                    GDT_FLAG_SEGMENT_PRESENT |
                                    GDT_FLAG_CODE_TYPE;

    uint32_t kernelCode32SegType =  GDT_TYPE_EXECUTABLE |
                                    GDT_TYPE_READABLE |
                                    GDT_TYPE_PROTECTED;

    /* Set the kernel data descriptor */
    uint32_t kernelData32SegFlags = GDT_FLAG_GRANULARITY_4K |
                                    GDT_FLAG_32_BIT_SEGMENT |
                                    GDT_FLAG_PL0 |
                                    GDT_FLAG_SEGMENT_PRESENT |
                                    GDT_FLAG_DATA_TYPE;

    uint32_t kernelData32SegType =  GDT_TYPE_WRITABLE | GDT_TYPE_GROW_DOWN;

    /* Set the kernel 16 bits code descriptor */
    uint32_t kernelCode16SegFlags = GDT_FLAG_GRANULARITY_4K |
                                    GDT_FLAG_16_BIT_SEGMENT |
                                    GDT_FLAG_PL0 |
                                    GDT_FLAG_SEGMENT_PRESENT |
                                    GDT_FLAG_CODE_TYPE;

    uint32_t kernelCode16SegType =  GDT_TYPE_EXECUTABLE |
                                    GDT_TYPE_READABLE |
                                    GDT_TYPE_PROTECTED;

    /* Set the kernel 16 bits data descriptor */
    uint32_t kernelData16SegFlags = GDT_FLAG_GRANULARITY_4K |
                                    GDT_FLAG_16_BIT_SEGMENT |
                                    GDT_FLAG_PL0 |
                                    GDT_FLAG_SEGMENT_PRESENT |
                                    GDT_FLAG_DATA_TYPE;

    uint32_t kernelData16SegType  = GDT_TYPE_WRITABLE | GDT_TYPE_GROW_DOWN;

    /* Set the user 32 bits code descriptor */
    uint32_t userCode32SegFlags = GDT_FLAG_GRANULARITY_4K |
                                  GDT_FLAG_32_BIT_SEGMENT |
                                  GDT_FLAG_PL3 |
                                  GDT_FLAG_SEGMENT_PRESENT |
                                  GDT_FLAG_CODE_TYPE;

    uint32_t userCode32SegType  = GDT_TYPE_EXECUTABLE |
                                  GDT_TYPE_READABLE |
                                  GDT_TYPE_PROTECTED;

    /* Set the user 32 bits data descriptor */
    uint32_t userData32SegFlags = GDT_FLAG_GRANULARITY_4K |
                                  GDT_FLAG_32_BIT_SEGMENT |
                                  GDT_FLAG_PL3 |
                                  GDT_FLAG_SEGMENT_PRESENT |
                                  GDT_FLAG_DATA_TYPE;

    uint32_t userData32SegType  =  GDT_TYPE_WRITABLE | GDT_TYPE_GROW_DOWN;

    /* Set the user 64 bits code descriptor */
    uint32_t userCode64SegFlags = GDT_FLAG_64_BIT_SEGMENT |
                                  GDT_FLAG_PL3 |
                                  GDT_FLAG_SEGMENT_PRESENT |
                                  GDT_FLAG_CODE_TYPE;

    uint32_t userCode64SegType  = GDT_TYPE_EXECUTABLE |
                                  GDT_TYPE_READABLE |
                                  GDT_TYPE_PROTECTED;

    /* Set the user 64 bits data descriptor */
    uint32_t userData64SegFlags = GDT_FLAG_64_BIT_SEGMENT |
                                  GDT_FLAG_PL3 |
                                  GDT_FLAG_SEGMENT_PRESENT |
                                  GDT_FLAG_DATA_TYPE;

    uint32_t userData64SegType  = GDT_TYPE_WRITABLE | GDT_TYPE_GROW_DOWN;

    /************************************
     * TSS ENTRY
     ***********************************/

    uint32_t tss_seg_flags = GDT_FLAG_GRANULARITY_4K |
                             GDT_FLAG_64_BIT_SEGMENT |
                             GDT_FLAG_SEGMENT_PRESENT |
                             GDT_FLAG_PL0 |
                             GDT_FLAG_TSS;

    uint32_t tss_seg_type = GDT_TYPE_ACCESSED |
                            GDT_TYPE_EXECUTABLE;

    /* Blank the GDT, set the NULL descriptor */
    memset(sGDT, 0, sizeof(uint64_t) * GDT_ENTRY_COUNT);

    /* Load the segments */
    _formatGDTEntry(&sGDT[KERNEL_CS_64 / 8],
                    KERNEL_CODE_SEGMENT_BASE_64,
                    KERNEL_CODE_SEGMENT_LIMIT_64,
                    kernelCode64SegType,
                    kernelCode64SegFlags);

    _formatGDTEntry(&sGDT[KERNEL_DS_64 / 8],
                    KERNEL_DATA_SEGMENT_BASE_64,
                    KERNEL_DATA_SEGMENT_LIMIT_64,
                    kernelData64SegType,
                    kernelData64SegFlags);

    _formatGDTEntry(&sGDT[KERNEL_CS_32 / 8],
                    KERNEL_CODE_SEGMENT_BASE_32,
                    KERNEL_CODE_SEGMENT_LIMIT_32,
                    kernelCode32SegType,
                    kernelCode32SegFlags);

    _formatGDTEntry(&sGDT[KERNEL_DS_32 / 8],
                    KERNEL_DATA_SEGMENT_BASE_32,
                    KERNEL_DATA_SEGMENT_LIMIT_32,
                    kernelData32SegType,
                    kernelData32SegFlags);

    _formatGDTEntry(&sGDT[KERNEL_CS_16 / 8],
                    KERNEL_CODE_SEGMENT_BASE_16,
                    KERNEL_CODE_SEGMENT_LIMIT_16,
                    kernelCode16SegType,
                    kernelCode16SegFlags);

    _formatGDTEntry(&sGDT[KERNEL_DS_16 / 8],
                    KERNEL_DATA_SEGMENT_BASE_16,
                    KERNEL_DATA_SEGMENT_LIMIT_16,
                    kernelData16SegType,
                    kernelData16SegFlags);

    _formatGDTEntry(&sGDT[USER_CS_64 / 8],
                    USER_CODE_SEGMENT_BASE_64,
                    USER_CODE_SEGMENT_LIMIT_64,
                    userCode64SegType,
                    userCode64SegFlags);

    _formatGDTEntry(&sGDT[USER_DS_64 / 8],
                    USER_DATA_SEGMENT_BASE_64,
                    USER_DATA_SEGMENT_LIMIT_64,
                    userData64SegType,
                    userData64SegFlags);

    _formatGDTEntry(&sGDT[USER_CS_32 / 8],
                    USER_CODE_SEGMENT_BASE_32,
                    USER_CODE_SEGMENT_LIMIT_32,
                    userCode32SegType,
                    userCode32SegFlags);

    _formatGDTEntry(&sGDT[USER_DS_32 / 8],
                    USER_DATA_SEGMENT_BASE_32,
                    USER_DATA_SEGMENT_LIMIT_32,
                    userData32SegType,
                    userData32SegFlags);

    for(i = 0; i < MAX_CPU_COUNT; ++i)
    {
        _formatGDTEntry(&sGDT[(TSS_SEGMENT + i * 0x08) / 8],
                        (uintptr_t)&sTSS[i],
                        ((uintptr_t)(&sTSS[i])) + sizeof(cpu_tss_entry_t),
                        tss_seg_type,
                        tss_seg_flags);
    }

    /* Set the GDT descriptor */
    sGDTPtr.size = ((sizeof(uint64_t) * GDT_ENTRY_COUNT) - 1);
    sGDTPtr.base = (uintptr_t)&sGDT;

    /* Load the GDT */
    __asm__ __volatile__("lgdt %0" :: "m" (sGDTPtr.size),
                                      "m" (sGDTPtr.base));

    /* Load segment selectors with a far jump for CS*/
    __asm__ __volatile__("movw %w0,%%ds\n\t"
                         "movw %w0,%%es\n\t"
                         "movw %w0,%%fs\n\t"
                         "movw %w0,%%gs\n\t"
                         "movw %w0,%%ss\n\t" :: "r" (KERNEL_DS_64));

    __asm__ __volatile__("mov %0, %%rax\n\t"
                         "push %%rax\n\t"
                         "movabs $new_gdt_seg_, %%rax\n\t"
                         "push %%rax\n\t"
                         "lretq\n\t"
                         "new_gdt_seg_: \n\t" :: "i" (KERNEL_CS_64) : "rax");

    KERNEL_SUCCESS("GDT Initialized at 0x%P\n", sGDTPtr.base);

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED, TRACE_X86_CPU_SETUP_GDT_EXIT, 0);
}

static void _setupIDT(void)
{
    uint32_t i;

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED, TRACE_X86_CPU_SETUP_IDT_ENTRY, 0);

    KERNEL_DEBUG(CPU_DEBUG_ENABLED, MODULE_NAME, "Setting IDT");

    /* Blank the IDT */
    memset(sIDT, 0, sizeof(cpu_idt_entry_t) * IDT_ENTRY_COUNT);

    /* Set interrupt handlers for each interrupt
     * This allows to redirect all interrupts to a global handler in C
     */
    for(i = 0; i < IDT_ENTRY_COUNT; ++i)
    {
        _formatIDTEntry(&sIDT[i],
                        sIntHandlerTable[i],
                        IDT_TYPE_INT_GATE, IDT_FLAG_PRESENT | IDT_FLAG_PL0);
    }

    /* Set the GDT descriptor */
    sIDTPtr.size = ((sizeof(cpu_idt_entry_t) * IDT_ENTRY_COUNT) - 1);
    sIDTPtr.base = (uintptr_t)&sIDT;

    /* Load the GDT */
    __asm__ __volatile__("lidt %0" :: "m" (sIDTPtr.size),
                                      "m" (sIDTPtr.base));

    KERNEL_SUCCESS("IDT Initialized at 0x%P\n", sIDTPtr.base);

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED, TRACE_X86_CPU_SETUP_IDT_EXIT, 0);
}

static void _setupTSS(void)
{
    uint32_t i;

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED, TRACE_X86_CPU_SETUP_TSS_ENTRY, 0);

    KERNEL_DEBUG(CPU_DEBUG_ENABLED, MODULE_NAME, "Setting TSS");

    /* Blank the TSS */
    memset(sTSS, 0, sizeof(cpu_tss_entry_t) * MAX_CPU_COUNT);

    /* Set basic values */
    for(i = 0; i < MAX_CPU_COUNT; ++i)
    {
        sTSS[i].rsp0 = ((uintptr_t)&_KERNEL_STACKS_BASE) +
                        KERNEL_STACK_SIZE * (i + 1) - sizeof(uint32_t);
        sTSS[i].ioMapBase = sizeof(cpu_tss_entry_t);
    }

    /* TODO: Load TSS */
    //__asm__ __volatile__("ltr %0" : : "rm" ((uint16_t)(TSS_SEGMENT)));

    KERNEL_SUCCESS("TSS Initialized at 0x%P\n", sTSS);

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED, TRACE_X86_CPU_SETUP_TSS_EXIT, 0);
}

void cpuInit(void)
{
    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED, TRACE_X86_CPU_SETUP_CPU_ENTRY, 0);

    /* Init the GDT, IDT and TSS */
    _setupGDT();
    _setupIDT();
    _setupTSS();

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED, TRACE_X86_CPU_SETUP_CPU_EXIT, 0);
}

OS_RETURN_E cpuRaiseInterrupt(const uint32_t kInterruptLine)
{
    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                       TRACE_X86_CPU_RAISE_INT_ENTRY,
                       1,
                       kInterruptLine);

    KERNEL_DEBUG(CPU_DEBUG_ENABLED,
                 MODULE_NAME,
                 "Requesting interrupt raise %d",
                 kInterruptLine);

    if(kInterruptLine > MAX_INTERRUPT_LINE)
    {
        KERNEL_ERROR("Requested an invalid CPU interrupt raise: %d\n",
                     kInterruptLine);
        KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                           TRACE_X86_CPU_RAISE_INT_EXIT,
                           2,
                           kInterruptLine,
                           OS_ERR_UNAUTHORIZED_ACTION);

        return OS_ERR_UNAUTHORIZED_ACTION;
    }

    switch(kInterruptLine)
    {
        case 0:
            __asm__ __volatile__("int %0" :: "i" (0));
            break;
        case 1:
            __asm__ __volatile__("int %0" :: "i" (1));
            break;
        case 2:
            __asm__ __volatile__("int %0" :: "i" (2));
            break;
        case 3:
            __asm__ __volatile__("int %0" :: "i" (3));
            break;
        case 4:
            __asm__ __volatile__("int %0" :: "i" (4));
            break;
        case 5:
            __asm__ __volatile__("int %0" :: "i" (5));
            break;
        case 6:
            __asm__ __volatile__("int %0" :: "i" (6));
            break;
        case 7:
            __asm__ __volatile__("int %0" :: "i" (7));
            break;
        case 8:
            __asm__ __volatile__("int %0" :: "i" (8));
            break;
        case 9:
            __asm__ __volatile__("int %0" :: "i" (9));
            break;
        case 10:
            __asm__ __volatile__("int %0" :: "i" (10));
            break;
        case 11:
            __asm__ __volatile__("int %0" :: "i" (11));
            break;
        case 12:
            __asm__ __volatile__("int %0" :: "i" (12));
            break;
        case 13:
            __asm__ __volatile__("int %0" :: "i" (13));
            break;
        case 14:
            __asm__ __volatile__("int %0" :: "i" (14));
            break;
        case 15:
            __asm__ __volatile__("int %0" :: "i" (15));
            break;
        case 16:
            __asm__ __volatile__("int %0" :: "i" (16));
            break;
        case 17:
            __asm__ __volatile__("int %0" :: "i" (17));
            break;
        case 18:
            __asm__ __volatile__("int %0" :: "i" (18));
            break;
        case 19:
            __asm__ __volatile__("int %0" :: "i" (19));
            break;
        case 20:
            __asm__ __volatile__("int %0" :: "i" (20));
            break;
        case 21:
            __asm__ __volatile__("int %0" :: "i" (21));
            break;
        case 22:
            __asm__ __volatile__("int %0" :: "i" (22));
            break;
        case 23:
            __asm__ __volatile__("int %0" :: "i" (23));
            break;
        case 24:
            __asm__ __volatile__("int %0" :: "i" (24));
            break;
        case 25:
            __asm__ __volatile__("int %0" :: "i" (25));
            break;
        case 26:
            __asm__ __volatile__("int %0" :: "i" (26));
            break;
        case 27:
            __asm__ __volatile__("int %0" :: "i" (27));
            break;
        case 28:
            __asm__ __volatile__("int %0" :: "i" (28));
            break;
        case 29:
            __asm__ __volatile__("int %0" :: "i" (29));
            break;
        case 30:
            __asm__ __volatile__("int %0" :: "i" (30));
            break;
        case 31:
            __asm__ __volatile__("int %0" :: "i" (31));
            break;
        case 32:
            __asm__ __volatile__("int %0" :: "i" (32));
            break;
        case 33:
            __asm__ __volatile__("int %0" :: "i" (33));
            break;
        case 34:
            __asm__ __volatile__("int %0" :: "i" (34));
            break;
        case 35:
            __asm__ __volatile__("int %0" :: "i" (35));
            break;
        case 36:
            __asm__ __volatile__("int %0" :: "i" (36));
            break;
        case 37:
            __asm__ __volatile__("int %0" :: "i" (37));
            break;
        case 38:
            __asm__ __volatile__("int %0" :: "i" (38));
            break;
        case 39:
            __asm__ __volatile__("int %0" :: "i" (39));
            break;
        case 40:
            __asm__ __volatile__("int %0" :: "i" (40));
            break;
        case 41:
            __asm__ __volatile__("int %0" :: "i" (41));
            break;
        case 42:
            __asm__ __volatile__("int %0" :: "i" (42));
            break;
        case 43:
            __asm__ __volatile__("int %0" :: "i" (43));
            break;
        case 44:
            __asm__ __volatile__("int %0" :: "i" (44));
            break;
        case 45:
            __asm__ __volatile__("int %0" :: "i" (45));
            break;
        case 46:
            __asm__ __volatile__("int %0" :: "i" (46));
            break;
        case 47:
            __asm__ __volatile__("int %0" :: "i" (47));
            break;
        case 48:
            __asm__ __volatile__("int %0" :: "i" (48));
            break;
        case 49:
            __asm__ __volatile__("int %0" :: "i" (49));
            break;
        case 50:
            __asm__ __volatile__("int %0" :: "i" (50));
            break;
        case 51:
            __asm__ __volatile__("int %0" :: "i" (51));
            break;
        case 52:
            __asm__ __volatile__("int %0" :: "i" (52));
            break;
        case 53:
            __asm__ __volatile__("int %0" :: "i" (53));
            break;
        case 54:
            __asm__ __volatile__("int %0" :: "i" (54));
            break;
        case 55:
            __asm__ __volatile__("int %0" :: "i" (55));
            break;
        case 56:
            __asm__ __volatile__("int %0" :: "i" (56));
            break;
        case 57:
            __asm__ __volatile__("int %0" :: "i" (57));
            break;
        case 58:
            __asm__ __volatile__("int %0" :: "i" (58));
            break;
        case 59:
            __asm__ __volatile__("int %0" :: "i" (59));
            break;
        case 60:
            __asm__ __volatile__("int %0" :: "i" (60));
            break;
        case 61:
            __asm__ __volatile__("int %0" :: "i" (61));
            break;
        case 62:
            __asm__ __volatile__("int %0" :: "i" (62));
            break;
        case 63:
            __asm__ __volatile__("int %0" :: "i" (63));
            break;
        case 64:
            __asm__ __volatile__("int %0" :: "i" (64));
            break;
        case 65:
            __asm__ __volatile__("int %0" :: "i" (65));
            break;
        case 66:
            __asm__ __volatile__("int %0" :: "i" (66));
            break;
        case 67:
            __asm__ __volatile__("int %0" :: "i" (67));
            break;
        case 68:
            __asm__ __volatile__("int %0" :: "i" (68));
            break;
        case 69:
            __asm__ __volatile__("int %0" :: "i" (69));
            break;
        case 70:
            __asm__ __volatile__("int %0" :: "i" (70));
            break;
        case 71:
            __asm__ __volatile__("int %0" :: "i" (71));
            break;
        case 72:
            __asm__ __volatile__("int %0" :: "i" (72));
            break;
        case 73:
            __asm__ __volatile__("int %0" :: "i" (73));
            break;
        case 74:
            __asm__ __volatile__("int %0" :: "i" (74));
            break;
        case 75:
            __asm__ __volatile__("int %0" :: "i" (75));
            break;
        case 76:
            __asm__ __volatile__("int %0" :: "i" (76));
            break;
        case 77:
            __asm__ __volatile__("int %0" :: "i" (77));
            break;
        case 78:
            __asm__ __volatile__("int %0" :: "i" (78));
            break;
        case 79:
            __asm__ __volatile__("int %0" :: "i" (79));
            break;
        case 80:
            __asm__ __volatile__("int %0" :: "i" (80));
            break;
        case 81:
            __asm__ __volatile__("int %0" :: "i" (81));
            break;
        case 82:
            __asm__ __volatile__("int %0" :: "i" (82));
            break;
        case 83:
            __asm__ __volatile__("int %0" :: "i" (83));
            break;
        case 84:
            __asm__ __volatile__("int %0" :: "i" (84));
            break;
        case 85:
            __asm__ __volatile__("int %0" :: "i" (85));
            break;
        case 86:
            __asm__ __volatile__("int %0" :: "i" (86));
            break;
        case 87:
            __asm__ __volatile__("int %0" :: "i" (87));
            break;
        case 88:
            __asm__ __volatile__("int %0" :: "i" (88));
            break;
        case 89:
            __asm__ __volatile__("int %0" :: "i" (89));
            break;
        case 90:
            __asm__ __volatile__("int %0" :: "i" (90));
            break;
        case 91:
            __asm__ __volatile__("int %0" :: "i" (91));
            break;
        case 92:
            __asm__ __volatile__("int %0" :: "i" (92));
            break;
        case 93:
            __asm__ __volatile__("int %0" :: "i" (93));
            break;
        case 94:
            __asm__ __volatile__("int %0" :: "i" (94));
            break;
        case 95:
            __asm__ __volatile__("int %0" :: "i" (95));
            break;
        case 96:
            __asm__ __volatile__("int %0" :: "i" (96));
            break;
        case 97:
            __asm__ __volatile__("int %0" :: "i" (97));
            break;
        case 98:
            __asm__ __volatile__("int %0" :: "i" (98));
            break;
        case 99:
            __asm__ __volatile__("int %0" :: "i" (99));
            break;
        case 100:
            __asm__ __volatile__("int %0" :: "i" (100));
            break;
        case 101:
            __asm__ __volatile__("int %0" :: "i" (101));
            break;
        case 102:
            __asm__ __volatile__("int %0" :: "i" (102));
            break;
        case 103:
            __asm__ __volatile__("int %0" :: "i" (103));
            break;
        case 104:
            __asm__ __volatile__("int %0" :: "i" (104));
            break;
        case 105:
            __asm__ __volatile__("int %0" :: "i" (105));
            break;
        case 106:
            __asm__ __volatile__("int %0" :: "i" (106));
            break;
        case 107:
            __asm__ __volatile__("int %0" :: "i" (107));
            break;
        case 108:
            __asm__ __volatile__("int %0" :: "i" (108));
            break;
        case 109:
            __asm__ __volatile__("int %0" :: "i" (109));
            break;
        case 110:
            __asm__ __volatile__("int %0" :: "i" (110));
            break;
        case 111:
            __asm__ __volatile__("int %0" :: "i" (111));
            break;
        case 112:
            __asm__ __volatile__("int %0" :: "i" (112));
            break;
        case 113:
            __asm__ __volatile__("int %0" :: "i" (113));
            break;
        case 114:
            __asm__ __volatile__("int %0" :: "i" (114));
            break;
        case 115:
            __asm__ __volatile__("int %0" :: "i" (115));
            break;
        case 116:
            __asm__ __volatile__("int %0" :: "i" (116));
            break;
        case 117:
            __asm__ __volatile__("int %0" :: "i" (117));
            break;
        case 118:
            __asm__ __volatile__("int %0" :: "i" (118));
            break;
        case 119:
            __asm__ __volatile__("int %0" :: "i" (119));
            break;
        case 120:
            __asm__ __volatile__("int %0" :: "i" (120));
            break;
        case 121:
            __asm__ __volatile__("int %0" :: "i" (121));
            break;
        case 122:
            __asm__ __volatile__("int %0" :: "i" (122));
            break;
        case 123:
            __asm__ __volatile__("int %0" :: "i" (123));
            break;
        case 124:
            __asm__ __volatile__("int %0" :: "i" (124));
            break;
        case 125:
            __asm__ __volatile__("int %0" :: "i" (125));
            break;
        case 126:
            __asm__ __volatile__("int %0" :: "i" (126));
            break;
        case 127:
            __asm__ __volatile__("int %0" :: "i" (127));
            break;
        case 128:
            __asm__ __volatile__("int %0" :: "i" (128));
            break;
        case 129:
            __asm__ __volatile__("int %0" :: "i" (129));
            break;
        case 130:
            __asm__ __volatile__("int %0" :: "i" (130));
            break;
        case 131:
            __asm__ __volatile__("int %0" :: "i" (131));
            break;
        case 132:
            __asm__ __volatile__("int %0" :: "i" (132));
            break;
        case 133:
            __asm__ __volatile__("int %0" :: "i" (133));
            break;
        case 134:
            __asm__ __volatile__("int %0" :: "i" (134));
            break;
        case 135:
            __asm__ __volatile__("int %0" :: "i" (135));
            break;
        case 136:
            __asm__ __volatile__("int %0" :: "i" (136));
            break;
        case 137:
            __asm__ __volatile__("int %0" :: "i" (137));
            break;
        case 138:
            __asm__ __volatile__("int %0" :: "i" (138));
            break;
        case 139:
            __asm__ __volatile__("int %0" :: "i" (139));
            break;
        case 140:
            __asm__ __volatile__("int %0" :: "i" (140));
            break;
        case 141:
            __asm__ __volatile__("int %0" :: "i" (141));
            break;
        case 142:
            __asm__ __volatile__("int %0" :: "i" (142));
            break;
        case 143:
            __asm__ __volatile__("int %0" :: "i" (143));
            break;
        case 144:
            __asm__ __volatile__("int %0" :: "i" (144));
            break;
        case 145:
            __asm__ __volatile__("int %0" :: "i" (145));
            break;
        case 146:
            __asm__ __volatile__("int %0" :: "i" (146));
            break;
        case 147:
            __asm__ __volatile__("int %0" :: "i" (147));
            break;
        case 148:
            __asm__ __volatile__("int %0" :: "i" (148));
            break;
        case 149:
            __asm__ __volatile__("int %0" :: "i" (149));
            break;
        case 150:
            __asm__ __volatile__("int %0" :: "i" (150));
            break;
        case 151:
            __asm__ __volatile__("int %0" :: "i" (151));
            break;
        case 152:
            __asm__ __volatile__("int %0" :: "i" (152));
            break;
        case 153:
            __asm__ __volatile__("int %0" :: "i" (153));
            break;
        case 154:
            __asm__ __volatile__("int %0" :: "i" (154));
            break;
        case 155:
            __asm__ __volatile__("int %0" :: "i" (155));
            break;
        case 156:
            __asm__ __volatile__("int %0" :: "i" (156));
            break;
        case 157:
            __asm__ __volatile__("int %0" :: "i" (157));
            break;
        case 158:
            __asm__ __volatile__("int %0" :: "i" (158));
            break;
        case 159:
            __asm__ __volatile__("int %0" :: "i" (159));
            break;
        case 160:
            __asm__ __volatile__("int %0" :: "i" (160));
            break;
        case 161:
            __asm__ __volatile__("int %0" :: "i" (161));
            break;
        case 162:
            __asm__ __volatile__("int %0" :: "i" (162));
            break;
        case 163:
            __asm__ __volatile__("int %0" :: "i" (163));
            break;
        case 164:
            __asm__ __volatile__("int %0" :: "i" (164));
            break;
        case 165:
            __asm__ __volatile__("int %0" :: "i" (165));
            break;
        case 166:
            __asm__ __volatile__("int %0" :: "i" (166));
            break;
        case 167:
            __asm__ __volatile__("int %0" :: "i" (167));
            break;
        case 168:
            __asm__ __volatile__("int %0" :: "i" (168));
            break;
        case 169:
            __asm__ __volatile__("int %0" :: "i" (169));
            break;
        case 170:
            __asm__ __volatile__("int %0" :: "i" (170));
            break;
        case 171:
            __asm__ __volatile__("int %0" :: "i" (171));
            break;
        case 172:
            __asm__ __volatile__("int %0" :: "i" (172));
            break;
        case 173:
            __asm__ __volatile__("int %0" :: "i" (173));
            break;
        case 174:
            __asm__ __volatile__("int %0" :: "i" (174));
            break;
        case 175:
            __asm__ __volatile__("int %0" :: "i" (175));
            break;
        case 176:
            __asm__ __volatile__("int %0" :: "i" (176));
            break;
        case 177:
            __asm__ __volatile__("int %0" :: "i" (177));
            break;
        case 178:
            __asm__ __volatile__("int %0" :: "i" (178));
            break;
        case 179:
            __asm__ __volatile__("int %0" :: "i" (179));
            break;
        case 180:
            __asm__ __volatile__("int %0" :: "i" (180));
            break;
        case 181:
            __asm__ __volatile__("int %0" :: "i" (181));
            break;
        case 182:
            __asm__ __volatile__("int %0" :: "i" (182));
            break;
        case 183:
            __asm__ __volatile__("int %0" :: "i" (183));
            break;
        case 184:
            __asm__ __volatile__("int %0" :: "i" (184));
            break;
        case 185:
            __asm__ __volatile__("int %0" :: "i" (185));
            break;
        case 186:
            __asm__ __volatile__("int %0" :: "i" (186));
            break;
        case 187:
            __asm__ __volatile__("int %0" :: "i" (187));
            break;
        case 188:
            __asm__ __volatile__("int %0" :: "i" (188));
            break;
        case 189:
            __asm__ __volatile__("int %0" :: "i" (189));
            break;
        case 190:
            __asm__ __volatile__("int %0" :: "i" (190));
            break;
        case 191:
            __asm__ __volatile__("int %0" :: "i" (191));
            break;
        case 192:
            __asm__ __volatile__("int %0" :: "i" (192));
            break;
        case 193:
            __asm__ __volatile__("int %0" :: "i" (193));
            break;
        case 194:
            __asm__ __volatile__("int %0" :: "i" (194));
            break;
        case 195:
            __asm__ __volatile__("int %0" :: "i" (195));
            break;
        case 196:
            __asm__ __volatile__("int %0" :: "i" (196));
            break;
        case 197:
            __asm__ __volatile__("int %0" :: "i" (197));
            break;
        case 198:
            __asm__ __volatile__("int %0" :: "i" (198));
            break;
        case 199:
            __asm__ __volatile__("int %0" :: "i" (199));
            break;
        case 200:
            __asm__ __volatile__("int %0" :: "i" (200));
            break;
        case 201:
            __asm__ __volatile__("int %0" :: "i" (201));
            break;
        case 202:
            __asm__ __volatile__("int %0" :: "i" (202));
            break;
        case 203:
            __asm__ __volatile__("int %0" :: "i" (203));
            break;
        case 204:
            __asm__ __volatile__("int %0" :: "i" (204));
            break;
        case 205:
            __asm__ __volatile__("int %0" :: "i" (205));
            break;
        case 206:
            __asm__ __volatile__("int %0" :: "i" (206));
            break;
        case 207:
            __asm__ __volatile__("int %0" :: "i" (207));
            break;
        case 208:
            __asm__ __volatile__("int %0" :: "i" (208));
            break;
        case 209:
            __asm__ __volatile__("int %0" :: "i" (209));
            break;
        case 210:
            __asm__ __volatile__("int %0" :: "i" (210));
            break;
        case 211:
            __asm__ __volatile__("int %0" :: "i" (211));
            break;
        case 212:
            __asm__ __volatile__("int %0" :: "i" (212));
            break;
        case 213:
            __asm__ __volatile__("int %0" :: "i" (213));
            break;
        case 214:
            __asm__ __volatile__("int %0" :: "i" (214));
            break;
        case 215:
            __asm__ __volatile__("int %0" :: "i" (215));
            break;
        case 216:
            __asm__ __volatile__("int %0" :: "i" (216));
            break;
        case 217:
            __asm__ __volatile__("int %0" :: "i" (217));
            break;
        case 218:
            __asm__ __volatile__("int %0" :: "i" (218));
            break;
        case 219:
            __asm__ __volatile__("int %0" :: "i" (219));
            break;
        case 220:
            __asm__ __volatile__("int %0" :: "i" (220));
            break;
        case 221:
            __asm__ __volatile__("int %0" :: "i" (221));
            break;
        case 222:
            __asm__ __volatile__("int %0" :: "i" (222));
            break;
        case 223:
            __asm__ __volatile__("int %0" :: "i" (223));
            break;
        case 224:
            __asm__ __volatile__("int %0" :: "i" (224));
            break;
        case 225:
            __asm__ __volatile__("int %0" :: "i" (225));
            break;
        case 226:
            __asm__ __volatile__("int %0" :: "i" (226));
            break;
        case 227:
            __asm__ __volatile__("int %0" :: "i" (227));
            break;
        case 228:
            __asm__ __volatile__("int %0" :: "i" (228));
            break;
        case 229:
            __asm__ __volatile__("int %0" :: "i" (229));
            break;
        case 230:
            __asm__ __volatile__("int %0" :: "i" (230));
            break;
        case 231:
            __asm__ __volatile__("int %0" :: "i" (231));
            break;
        case 232:
            __asm__ __volatile__("int %0" :: "i" (232));
            break;
        case 233:
            __asm__ __volatile__("int %0" :: "i" (233));
            break;
        case 234:
            __asm__ __volatile__("int %0" :: "i" (234));
            break;
        case 235:
            __asm__ __volatile__("int %0" :: "i" (235));
            break;
        case 236:
            __asm__ __volatile__("int %0" :: "i" (236));
            break;
        case 237:
            __asm__ __volatile__("int %0" :: "i" (237));
            break;
        case 238:
            __asm__ __volatile__("int %0" :: "i" (238));
            break;
        case 239:
            __asm__ __volatile__("int %0" :: "i" (239));
            break;
        case 240:
            __asm__ __volatile__("int %0" :: "i" (240));
            break;
        case 241:
            __asm__ __volatile__("int %0" :: "i" (241));
            break;
        case 242:
            __asm__ __volatile__("int %0" :: "i" (242));
            break;
        case 243:
            __asm__ __volatile__("int %0" :: "i" (243));
            break;
        case 244:
            __asm__ __volatile__("int %0" :: "i" (244));
            break;
        case 245:
            __asm__ __volatile__("int %0" :: "i" (245));
            break;
        case 246:
            __asm__ __volatile__("int %0" :: "i" (246));
            break;
        case 247:
            __asm__ __volatile__("int %0" :: "i" (247));
            break;
        case 248:
            __asm__ __volatile__("int %0" :: "i" (248));
            break;
        case 249:
            __asm__ __volatile__("int %0" :: "i" (249));
            break;
        case 250:
            __asm__ __volatile__("int %0" :: "i" (250));
            break;
        case 251:
            __asm__ __volatile__("int %0" :: "i" (251));
            break;
        case 252:
            __asm__ __volatile__("int %0" :: "i" (252));
            break;
        case 253:
            __asm__ __volatile__("int %0" :: "i" (253));
            break;
        case 254:
            __asm__ __volatile__("int %0" :: "i" (254));
            break;
        case 255:
            __asm__ __volatile__("int %0" :: "i" (255));
            break;
    }

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                       TRACE_X86_CPU_RAISE_INT_EXIT,
                       2,
                       kInterruptLine,
                       OS_NO_ERR);
    return OS_NO_ERR;
}

void cpuValidateArchitecture(void)
{
    /* eax, ebx, ecx, edx */
    volatile int32_t regs[4];
    volatile int32_t regsExt[4];
    uint32_t         ret;

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                       TRACE_X86_CPU_VALIDATE_ARCH_ENTRY,
                       0);

#if KERNEL_LOG_LEVEL >= INFO_LOG_LEVEL
    uint32_t outputBuffIndex;
    char     outputBuff[512];
    char     vendorString[26] = "CPU Vendor:             \n\0";

    outputBuffIndex = 0;
#endif

    KERNEL_DEBUG(CPU_DEBUG_ENABLED, "CPU", "Detecting cpu capabilities");

    ret = _cpuCPUID(CPUID_GETVENDORSTRING, (uint32_t*)regs);

    CPU_ASSERT(ret != 0,
               "CPU does not support CPUID",
               OS_ERR_NOT_SUPPORTED);

    /* Check if CPUID return more that one available function */

#if KERNEL_LOG_LEVEL >= INFO_LOG_LEVEL
    for(int8_t j = 0; j < 4; ++j)
    {
        vendorString[12 + j] = (char)((regs[1] >> (j * 8)) & 0xFF);
    }
    for(int8_t j = 0; j < 4; ++j)
    {
        vendorString[16 + j] = (char)((regs[3] >> (j * 8)) & 0xFF);
    }
    for(int8_t j = 0; j < 4; ++j)
    {
        vendorString[20 + j] = (char)((regs[2] >> (j * 8)) & 0xFF);
    }

    KERNEL_INFO(vendorString);
#endif

    /* Get CPUID features */
    _cpuCPUID(CPUID_GETFEATURES, (uint32_t*)regs);

#if KERNEL_LOG_LEVEL >= INFO_LOG_LEVEL
    memset(outputBuff, 0, 512 * sizeof(char));
    strncpy(outputBuff, "CPU Features: ", 14);
    outputBuffIndex = 14;

    if((regs[2] & ECX_SSE3) == ECX_SSE3)
    { CONCAT_STR(outputBuff, outputBuffIndex, "SSE3 - "); }
    if((regs[2] & ECX_PCLMULQDQ) == ECX_PCLMULQDQ)
    { CONCAT_STR(outputBuff, outputBuffIndex, "PCLMULQDQ - "); }
    if((regs[2] & ECX_DTES64) == ECX_DTES64)
    { CONCAT_STR(outputBuff, outputBuffIndex, "DTES64 - "); }
    if((regs[2] & ECX_MONITOR) == ECX_MONITOR)
    { CONCAT_STR(outputBuff, outputBuffIndex, "MONITOR - "); }
    if((regs[2] & ECX_DS_CPL) == ECX_DS_CPL)
    { CONCAT_STR(outputBuff, outputBuffIndex, "DS_CPL - "); }
    if((regs[2] & ECX_VMX) == ECX_VMX)
    { CONCAT_STR(outputBuff, outputBuffIndex, "VMX - "); }
    if((regs[2] & ECX_SMX) == ECX_SMX)
    { CONCAT_STR(outputBuff, outputBuffIndex, "SMX - "); }
    if((regs[2] & ECX_EST) == ECX_EST)
    { CONCAT_STR(outputBuff, outputBuffIndex, "EST - "); }
    if((regs[2] & ECX_TM2) == ECX_TM2)
    { CONCAT_STR(outputBuff, outputBuffIndex, "TM2 - "); }
    if((regs[2] & ECX_SSSE3) == ECX_SSSE3)
    { CONCAT_STR(outputBuff, outputBuffIndex, "SSSE3 - "); }
    if((regs[2] & ECX_CNXT_ID) == ECX_CNXT_ID)
    { CONCAT_STR(outputBuff, outputBuffIndex, "CNXT_ID - "); }
    if((regs[2] & ECX_FMA) == ECX_FMA)
    { CONCAT_STR(outputBuff, outputBuffIndex, "FMA - "); }
    if((regs[2] & ECX_CX16) == ECX_CX16)
    { CONCAT_STR(outputBuff, outputBuffIndex, "CX16 - "); }
    if((regs[2] & ECX_XTPR) == ECX_XTPR)
    { CONCAT_STR(outputBuff, outputBuffIndex, "XTPR - "); }
    if((regs[2] & ECX_PDCM) == ECX_PDCM)
    { CONCAT_STR(outputBuff, outputBuffIndex, "PDCM - "); }
    if((regs[2] & ECX_PCID) == ECX_PCID)
    { CONCAT_STR(outputBuff, outputBuffIndex, "PCID - "); }
    if((regs[2] & ECX_DCA) == ECX_DCA)
    { CONCAT_STR(outputBuff, outputBuffIndex, "DCA - "); }
    if((regs[2] & ECX_SSE41) == ECX_SSE41)
    { CONCAT_STR(outputBuff, outputBuffIndex, "SSE41 - "); }
    if((regs[2] & ECX_SSE42) == ECX_SSE42)
    { CONCAT_STR(outputBuff, outputBuffIndex, "SSE42 - "); }
    if((regs[2] & ECX_X2APIC) == ECX_X2APIC)
    { CONCAT_STR(outputBuff, outputBuffIndex, "X2APIC - "); }
    if((regs[2] & ECX_MOVBE) == ECX_MOVBE)
    { CONCAT_STR(outputBuff, outputBuffIndex, "MOVBE - "); }
    if((regs[2] & ECX_POPCNT) == ECX_POPCNT)
    { CONCAT_STR(outputBuff, outputBuffIndex, "POPCNT - "); }
    if((regs[2] & ECX_TSC) == ECX_TSC)
    { CONCAT_STR(outputBuff, outputBuffIndex, "TSC - "); }
    if((regs[2] & ECX_AESNI) == ECX_AESNI)
    { CONCAT_STR(outputBuff, outputBuffIndex, "AESNI - "); }
    if((regs[2] & ECX_XSAVE) == ECX_XSAVE)
    { CONCAT_STR(outputBuff, outputBuffIndex, "XSAVE - "); }
    if((regs[2] & ECX_OSXSAVE) == ECX_OSXSAVE)
    { CONCAT_STR(outputBuff, outputBuffIndex, "OSXSAVE - "); }
    if((regs[2] & ECX_AVX) == ECX_AVX)
    { CONCAT_STR(outputBuff, outputBuffIndex, "AVX - "); }
    if((regs[2] & ECX_F16C) == ECX_F16C)
    { CONCAT_STR(outputBuff, outputBuffIndex, "F16C - "); }
    if((regs[2] & ECX_RDRAND) == ECX_RDRAND)
    { CONCAT_STR(outputBuff, outputBuffIndex, "RDRAND - "); }
    if((regs[3] & EDX_FPU) == EDX_FPU)
    { CONCAT_STR(outputBuff, outputBuffIndex, "FPU - "); }
    if((regs[3] & EDX_VME) == EDX_VME)
    { CONCAT_STR(outputBuff, outputBuffIndex, "VME - "); }
    if((regs[3] & EDX_DE) == EDX_DE)
    { CONCAT_STR(outputBuff, outputBuffIndex, "DE - "); }
    if((regs[3] & EDX_PSE) == EDX_PSE)
    { CONCAT_STR(outputBuff, outputBuffIndex, "PSE - "); }
    if((regs[3] & EDX_TSC) == EDX_TSC)
    { CONCAT_STR(outputBuff, outputBuffIndex, "TSC - "); }
    if((regs[3] & EDX_MSR) == EDX_MSR)
    { CONCAT_STR(outputBuff, outputBuffIndex, "MSR - "); }
    if((regs[3] & EDX_PAE) == EDX_PAE)
    { CONCAT_STR(outputBuff, outputBuffIndex, "PAE - "); }
    if((regs[3] & EDX_MCE) == EDX_MCE)
    { CONCAT_STR(outputBuff, outputBuffIndex, "MCE - "); }
    if((regs[3] & EDX_CX8) == EDX_CX8)
    { CONCAT_STR(outputBuff, outputBuffIndex, "CX8 - "); }
    if((regs[3] & EDX_APIC) == EDX_APIC)
    { CONCAT_STR(outputBuff, outputBuffIndex, "APIC - "); }
    if((regs[3] & EDX_SEP) == EDX_SEP)
    { CONCAT_STR(outputBuff, outputBuffIndex, "SEP - "); }
    if((regs[3] & EDX_MTRR) == EDX_MTRR)
    { CONCAT_STR(outputBuff, outputBuffIndex, "MTRR - "); }
    if((regs[3] & EDX_PGE) == EDX_PGE)
    { CONCAT_STR(outputBuff, outputBuffIndex, "PGE - "); }
    if((regs[3] & EDX_MCA) == EDX_MCA)
    { CONCAT_STR(outputBuff, outputBuffIndex, "MCA - "); }
    if((regs[3] & EDX_CMOV) == EDX_CMOV)
    { CONCAT_STR(outputBuff, outputBuffIndex, "CMOV - "); }
    if((regs[3] & EDX_PAT) == EDX_PAT)
    { CONCAT_STR(outputBuff, outputBuffIndex, "PAT - "); }
    if((regs[3] & EDX_PSE36) == EDX_PSE36)
    { CONCAT_STR(outputBuff, outputBuffIndex, "PSE36 - "); }
    if((regs[3] & EDX_PSN) == EDX_PSN)
    { CONCAT_STR(outputBuff, outputBuffIndex, "PSN - "); }
    if((regs[3] & EDX_CLFLUSH) == EDX_CLFLUSH)
    { CONCAT_STR(outputBuff, outputBuffIndex, "CLFLUSH - "); }
    if((regs[3] & EDX_DS) == EDX_DS)
    { CONCAT_STR(outputBuff, outputBuffIndex, "DS - "); }
    if((regs[3] & EDX_ACPI) == EDX_ACPI)
    { CONCAT_STR(outputBuff, outputBuffIndex, "ACPI - "); }
    if((regs[3] & EDX_MMX) == EDX_MMX)
    { CONCAT_STR(outputBuff, outputBuffIndex, "MMX - "); }
    if((regs[3] & EDX_FXSR) == EDX_FXSR)
    { CONCAT_STR(outputBuff, outputBuffIndex, "FXSR - "); }
    if((regs[3] & EDX_SSE) == EDX_SSE)
    { CONCAT_STR(outputBuff, outputBuffIndex, "SSE - "); }
    if((regs[3] & EDX_SSE2) == EDX_SSE2)
    { CONCAT_STR(outputBuff, outputBuffIndex, "SSE2 - "); }
    if((regs[3] & EDX_SS) == EDX_SS)
    { CONCAT_STR(outputBuff, outputBuffIndex, "SS - "); }
    if((regs[3] & EDX_HTT) == EDX_HTT)
    { CONCAT_STR(outputBuff, outputBuffIndex, "HTT - "); }
    if((regs[3] & EDX_TM) == EDX_TM)
    { CONCAT_STR(outputBuff, outputBuffIndex, "TM - "); }
    if((regs[3] & EDX_PBE) == EDX_PBE)
    { CONCAT_STR(outputBuff, outputBuffIndex, "PBE - "); }

    /* Check for extended features */
    _cpuCPUID(CPUID_INTELEXTENDED_AVAILABLE, (uint32_t*)regsExt);
    if((uint32_t)regsExt[0] >= (uint32_t)CPUID_INTELFEATURES)
    {
        _cpuCPUID(CPUID_INTELFEATURES, (uint32_t*)regsExt);

        if((regsExt[3] & EDX_SYSCALL) == EDX_SYSCALL)
        { CONCAT_STR(outputBuff, outputBuffIndex, "SYSCALL - "); }
        if((regsExt[3] & EDX_MP) == EDX_MP)
        { CONCAT_STR(outputBuff, outputBuffIndex, "MP - "); }
        if((regsExt[3] & EDX_XD) == EDX_XD)
        { CONCAT_STR(outputBuff, outputBuffIndex, "XD - "); }
        if((regsExt[3] & EDX_MMX_EX) == EDX_MMX_EX)
        { CONCAT_STR(outputBuff, outputBuffIndex, "MMX_EX - "); }
        if((regsExt[3] & EDX_FXSR) == EDX_FXSR)
        { CONCAT_STR(outputBuff, outputBuffIndex, "FXSR - "); }
        if((regsExt[3] & EDX_FXSR_OPT) == EDX_FXSR_OPT)
        { CONCAT_STR(outputBuff, outputBuffIndex, "FXSR_OPT - "); }
        if((regsExt[3] & EDX_1GB_PAGE) == EDX_1GB_PAGE)
        { CONCAT_STR(outputBuff, outputBuffIndex, "1GB_PAGE - "); }
        if((regsExt[3] & EDX_RDTSCP) == EDX_RDTSCP)
        { CONCAT_STR(outputBuff, outputBuffIndex, "RDTSCP - "); }
        if((regsExt[3] & EDX_64_BIT) == EDX_64_BIT)
        { CONCAT_STR(outputBuff, outputBuffIndex, "X64 - "); }
        if((regsExt[3] & EDX_3DNOW_EX) == EDX_3DNOW_EX)
        { CONCAT_STR(outputBuff, outputBuffIndex, "3DNOW_EX - "); }
        if((regsExt[3] & EDX_3DNOW) == EDX_3DNOW)
        { CONCAT_STR(outputBuff, outputBuffIndex, "3DNOW - "); }
        if((regsExt[2] & ECX_LAHF_LM) == ECX_LAHF_LM)
        { CONCAT_STR(outputBuff, outputBuffIndex, "LAHF_LM - "); }
        if((regsExt[2] & ECX_CMP_LEG) == ECX_CMP_LEG)
        { CONCAT_STR(outputBuff, outputBuffIndex, "CMP_LEG - "); }
        if((regsExt[2] & ECX_SVM) == ECX_SVM)
        { CONCAT_STR(outputBuff, outputBuffIndex, "SVM - "); }
        if((regsExt[2] & ECX_EXTAPIC) == ECX_EXTAPIC)
        { CONCAT_STR(outputBuff, outputBuffIndex, "EXTAPIC - "); }
        if((regsExt[2] & ECX_CR8_LEG) == ECX_CR8_LEG)
        { CONCAT_STR(outputBuff, outputBuffIndex, "CR8_LEG - "); }
        if((regsExt[2] & ECX_ABM) == ECX_ABM)
        { CONCAT_STR(outputBuff, outputBuffIndex, "ABM - "); }
        if((regsExt[2] & ECX_SSE4A) == ECX_SSE4A)
        { CONCAT_STR(outputBuff, outputBuffIndex, "SSE4A - "); }
        if((regsExt[2] & ECX_MISASSE) == ECX_MISASSE)
        { CONCAT_STR(outputBuff, outputBuffIndex, "MISALIGNED_SSE - "); }
        if((regsExt[2] & ECX_PREFETCH) == ECX_PREFETCH)
        { CONCAT_STR(outputBuff, outputBuffIndex, "PREFETCH - "); }
        if((regsExt[2] & ECX_OSVW) == ECX_OSVW)
        { CONCAT_STR(outputBuff, outputBuffIndex, "OSVW - "); }
        if((regsExt[2] & ECX_IBS) == ECX_IBS)
        { CONCAT_STR(outputBuff, outputBuffIndex, "IBS - "); }
        if((regsExt[2] & ECX_XOP) == ECX_XOP)
        { CONCAT_STR(outputBuff, outputBuffIndex, "XOP - "); }
        if((regsExt[2] & ECX_SKINIT) == ECX_SKINIT)
        { CONCAT_STR(outputBuff, outputBuffIndex, "SKINIT - "); }
        if((regsExt[2] & ECX_WDT) == ECX_WDT)
        { CONCAT_STR(outputBuff, outputBuffIndex, "WDT - "); }
        if((regsExt[2] & ECX_LWP) == ECX_LWP)
        { CONCAT_STR(outputBuff, outputBuffIndex, "LWP - "); }
        if((regsExt[2] & ECX_FMA4) == ECX_FMA4)
        { CONCAT_STR(outputBuff, outputBuffIndex, "FMA4 - "); }
        if((regsExt[2] & ECX_TCE) == ECX_TCE)
        { CONCAT_STR(outputBuff, outputBuffIndex, "TCE - "); }
        if((regsExt[2] & ECX_NODEIDMSR) == ECX_NODEIDMSR)
        { CONCAT_STR(outputBuff, outputBuffIndex, "NODE_ID_MSR - "); }
        if((regsExt[2] & ECX_TBM) == ECX_TBM)
        { CONCAT_STR(outputBuff, outputBuffIndex, "TMB - "); }
        if((regsExt[2] & ECX_TOPOEX) == ECX_TOPOEX)
        { CONCAT_STR(outputBuff, outputBuffIndex, "TOPOEX - "); }
        if((regsExt[2] & ECX_PERF_CORE) == ECX_PERF_CORE)
        { CONCAT_STR(outputBuff, outputBuffIndex, "PERF_CORE - "); }
        if((regsExt[2] & ECX_PERF_NB) == ECX_PERF_NB)
        { CONCAT_STR(outputBuff, outputBuffIndex, "PERF_NB - "); }
        if((regsExt[2] & ECX_DBX) == ECX_DBX)
        { CONCAT_STR(outputBuff, outputBuffIndex, "DBX - "); }
        if((regsExt[2] & ECX_PERF_TSC) == ECX_PERF_TSC)
        { CONCAT_STR(outputBuff, outputBuffIndex, "TSC - "); }
        if((regsExt[2] & ECX_PCX_L2I) == ECX_PCX_L2I)
        { CONCAT_STR(outputBuff, outputBuffIndex, "PCX_L2I - "); }
    }

    outputBuff[outputBuffIndex - 2] = '\n';
    outputBuff[outputBuffIndex - 1] = 0;
    KERNEL_INFO(outputBuff);
#else
    (void)regsExt;
#endif

    /* Validate features */
    CPU_ASSERT((regs[3] & EDX_SEP) == EDX_SEP,
               "CPU does not support SYSENTER",
               OS_ERR_NOT_SUPPORTED);
    CPU_ASSERT((regs[3] & EDX_FPU) == EDX_FPU,
               "CPU does not support FPU",
               OS_ERR_NOT_SUPPORTED);
    CPU_ASSERT((regs[3] & EDX_TSC) == EDX_TSC,
               "CPU does not support TSC",
               OS_ERR_NOT_SUPPORTED);
    CPU_ASSERT((regs[3] & EDX_APIC) == EDX_APIC,
               "CPU does not support APIC",
               OS_ERR_NOT_SUPPORTED);
    CPU_ASSERT((regs[3] & EDX_FXSR) == EDX_FXSR,
               "CPU does not support FX instructions",
               OS_ERR_NOT_SUPPORTED);
    CPU_ASSERT((regs[3] & EDX_SSE) == EDX_SSE,
               "CPU does not support SSE",
               OS_ERR_NOT_SUPPORTED);
    CPU_ASSERT((regs[3] & EDX_SSE2) == EDX_SSE2,
               "CPU does not support SSE2",
               OS_ERR_NOT_SUPPORTED);

    KERNEL_TRACE_EVENT(TRACE_X86_CPU_ENABLED,
                       TRACE_X86_CPU_VALIDATE_ARCH_EXIT,
                       0);
}

uint32_t cpuGetContextIntState(const void* kpVCpu)
{
    return ((virtual_cpu_t*)kpVCpu)->intContext.rflags & CPU_RFLAGS_IF;
}

uint32_t cpuGetContextIntNumber(const void* kpVCpu)
{
    return  ((virtual_cpu_t*)kpVCpu)->intContext.intId;
}

const cpu_interrupt_config_t* cpuGetInterruptConfig(void)
{
    return &ksInterruptConfig;
}

uint32_t cpuGeIntState(void)
{
    return ((_cpuSaveFlags() & CPU_RFLAGS_IF) != 0);
}

void cpuClearInterrupt(void)
{
    __asm__ __volatile__("cli":::"memory");
}

void cpuSetInterrupt(void)
{
    __asm__ __volatile__("sti":::"memory");
}

void cpuHalt(void)
{
    __asm__ __volatile__ ("hlt":::"memory");
}

/************************************ EOF *************************************/