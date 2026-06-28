/*******************************************************************************
 * @file cpuid.c
 *
 * @see cpuid.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 26/10/2025
 *
 * @version 1.0
 *
 * @brief x86_64 CPUID Driver.
 *
 * @details x86_64 CPUID Driver. Provides the definitions and functions to
 * manipulate the CPUID features.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/
/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <panic.h>   /* Kernel panic */
#include <kheap.h>   /* Kernel malloc */
#include <stdint.h>  /* Standard int definitions */
#include <string.h>  /* Memory manipulation */
#include <stdlib.h>  /* Standard lib functions */
#include <stdbool.h> /* Standard bool definitions */

/* Configuration files */
#include <config.h>

/* Header file */
#include <cpuid.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Current module name */
#define MODULE_NAME "CPUID"

/****************************************
 * CPUID Leaves
 ***************************************/

/** @brief Request vendor string. */
#define CPUID_GETVENDORSTRING          0x00000000
/** @brief Request capabled CPUID features. */
#define CPUID_GETFEATURES              0x00000001
/** @brief Cache information for Intel CPUs */
#define CPUID_GETCACHEINFO_INTEL       0x00000004
/** @brief Cache information for Intel CPUs */
#define CPUID_GETEXTBASE_FEATURES      0x00000007
/** @brief Request extended CPUID features. */
#define CPUID_GETEXTENDED_AVAILABLE    0x80000000
/** @brief Request Intel CPUID features. */
#define CPUID_GETEXTENTED_FEATURES     0x80000001
/** @brief Request CPU brand string. */
#define CPUID_GETBRANDSTRING_START     0x80000002
/** @brief Request Intel brand string extended. */
#define CPUID_GETBRANDSTRING_MID       0x80000003
/** @brief Request Intel brand string end. */
#define CPUID_GETBRANDSTRING_END       0x80000004
/** @brief Request AMD L1 Cache and TLB info. */
#define CPUID_GETL1CACHETLBINFO_AMD    0x80000005
/** @brief Request AMD L2, L3 Cache and TLB info. */
#define CPUID_GETL2CACHETLBINFO_AMD    0x80000006
/** @brief Request capacity and extended flags. */
#define CPUID_GETCAPACITY_EXT_FLAGS    0x80000008
/** @brief Cache information for AMD CPUs */
#define CPUID_GETCACHEINFO_AMD         0x8000001D
/** @brief Extended information for AMD CPUs */
#define CPUID_GETAMDEXTINFO            0x80000021

/****************************************
 * CPUID General Features 0x00000001
 ***************************************/

/** @brief Streaming SIMD Extensions 3 (SSE3). */
#define ECX_SSE3 (1U << 0)
/** @brief PCLMULQDQ instruction support. */
#define ECX_PCLMULQDQ (1U << 1)
/** @brief 64-bit DS save area. */
#define ECX_DTES64 (1U << 2)
/** @brief MONITOR/MWAIT support. */
#define ECX_MONITOR (1U << 3)
/** @brief CPL Qualified Debug Store. */
#define ECX_DSCPL (1U << 4)
/** @brief Virtual Machine Extensions. */
#define ECX_VMX (1U << 5)
/** @brief Safer Mode Extensions. */
#define ECX_SMX (1U << 6)
/** @brief Enhanced Intel SpeedStep. */
#define ECX_EST (1U << 7)
/** @brief Thermal Monitor 2. */
#define ECX_TM2 (1U << 8)
/** @brief Supplemental SSE3. */
#define ECX_SSSE3 (1U << 9)
/** @brief L1 Context ID. */
#define ECX_CNTXT_ID (1U << 10)
/** @brief Silicon Debug. */
#define ECX_SDBG (1U << 11)
/** @brief FMA extensions using YMM state. */
#define ECX_FMA (1U << 12)
/** @brief CMPXCHG16B instruction support. */
#define ECX_CX16 (1U << 13)
/** @brief xTPR Update Control. */
#define ECX_XTPR_UPDATE (1U << 14)
/** @brief Perfmon and Debug Capability. */
#define ECX_PDCM (1U << 15)
/** @brief Process-context identifiers. */
#define ECX_PCID (1U << 17)
/** @brief Direct Cache Access. */
#define ECX_DCA (1U << 18)
/** @brief SSE4.1. */
#define ECX_SSE4_1 (1U << 19)
/** @brief SSE4.2. */
#define ECX_SSE4_2 (1U << 20)
/** @brief X2APIC support. */
#define ECX_X2APIC (1U << 21)
/** @brief MOVBE instruction support. */
#define ECX_MOVBE (1U << 22)
/** @brief POPCNT instruction support. */
#define ECX_POPCNT (1U << 23)
/** @brief APIC timer one-shot operation. */
#define ECX_TSC_DEADLINE_TIMER (1U << 24)
/** @brief AES instructions. */
#define ECX_AES (1U << 25)
/** @brief XSAVE (and related instructions) support. */
#define ECX_XSAVE (1U << 26)
/** @brief XSAVE (and related instructions) are enabled by OS. */
#define ECX_OSXSAVE (1U << 27)
/** @brief AVX instructions support. */
#define ECX_AVX (1U << 28)
/** @brief Half-precision floating-point conversion support. */
#define ECX_F16C (1U << 29)
/** @brief RDRAND instruction support. */
#define ECX_RDRAND (1U << 30)
/** @brief System is running as guest; (para-)virtualized system. */
#define ECX_GUEST_STATUS (1U << 31)
/** @brief Floating-Point Unit on-chip (x87). */
#define EDX_FPU (1U << 0)
/** @brief Virtual-8086 Mode Extensions. */
#define EDX_VME (1U << 1)
/** @brief Debugging Extensions. */
#define EDX_DE (1U << 2)
/** @brief Page Size Extension. */
#define EDX_PSE (1U << 3)
/** @brief Time Stamp Counter. */
#define EDX_TSC (1U << 4)
/** @brief Model-Specific Registers (RDMSR and WRMSR support). */
#define EDX_MSR (1U << 5)
/** @brief Physical Address Extensions. */
#define EDX_PAE (1U << 6)
/** @brief Machine Check Exception. */
#define EDX_MCE (1U << 7)
/** @brief CMPXCHG8B instruction. */
#define EDX_CX8 (1U << 8)
/** @brief APIC on-chip. */
#define EDX_APIC (1U << 9)
/** @brief SYSENTER, SYSEXIT, and associated MSRs. */
#define EDX_SEP (1U << 11)
/** @brief Memory Type Range Registers. */
#define EDX_MTRR (1U << 12)
/** @brief Page Global Extensions. */
#define EDX_PGE (1U << 13)
/** @brief Machine Check Architecture. */
#define EDX_MCA (1U << 14)
/** @brief Conditional Move Instruction. */
#define EDX_CMOV (1U << 15)
/** @brief Page Attribute Table. */
#define EDX_PAT (1U << 16)
/** @brief Page Size Extension (36-bit). */
#define EDX_PSE36 (1U << 17)
/** @brief Processor Serial Number. */
#define EDX_PSN (1U << 18)
/** @brief CLFLUSH instruction. */
#define EDX_CLFLUSH (1U << 19)
/** @brief Debug Store. */
#define EDX_DS (1U << 21)
/** @brief Thermal monitor and clock control. */
#define EDX_ACPI (1U << 22)
/** @brief MMX instructions. */
#define EDX_MMX (1U << 23)
/** @brief FXSAVE and FXRSTOR instructions. */
#define EDX_FXSR (1U << 24)
/** @brief SSE instructions. */
#define EDX_SSE (1U << 25)
/** @brief SSE2 instructions. */
#define EDX_SSE2 (1U << 26)
/** @brief Self Snoop. */
#define EDX_SELFSNOOP (1U << 27)
/** @brief Hyper-threading. */
#define EDX_HTT (1U << 28)
/** @brief Thermal Monitor. */
#define EDX_TM (1U << 29)
/** @brief Legacy IA-64 (Itanium) support bit, now reserved. */
#define EDX_IA64 (1U << 30)
/** @brief Pending Break Enable. */
#define EDX_PBE (1U << 31)

/****************************************
 * CPUID Extended Features 0x00000007 Leaf 0
 ***************************************/

/** @brief FSBASE/GSBASE read/write. */
#define EBX_FSGSBASE (1U << 0)
/** @brief IA32_TSC_ADJUST MSR. */
#define EBX_TSC_ADJUST (1U << 1)
/** @brief Intel SGX (Software Guard Extensions). */
#define EBX_SGX (1U << 2)
/** @brief Bit manipulation extensions group 1. */
#define EBX_BMI1 (1U << 3)
/** @brief Hardware Lock Elision. */
#define EBX_HLE (1U << 4)
/** @brief AVX2 instruction set. */
#define EBX_AVX2 (1U << 5)
/** @brief FPU Data Pointer updated only on x87 exceptions. */
#define EBX_FDP_EXCPTN_ONLY (1U << 6)
/** @brief Supervisor Mode Execution Protection. */
#define EBX_SMEP (1U << 7)
/** @brief Bit manipulation extensions group 2. */
#define EBX_BMI2 (1U << 8)
/** @brief Enhanced REP MOVSB/STOSB. */
#define EBX_ERMS (1U << 9)
/** @brief INVPCID instruction (Invalidate Processor Context ID). */
#define EBX_INVPCID (1U << 10)
/** @brief Intel restricted transactional memory. */
#define EBX_RTM (1U << 11)
/** @brief Intel RDT-CMT / AMD Platform-QoS cache monitoring. */
#define EBX_PQM (1U << 12)
/** @brief Deprecated FPU CS/DS (stored as zero). */
#define EBX_ZERO_FCS_FDS (1U << 13)
/** @brief Intel memory protection extensions. */
#define EBX_MPX (1U << 14)
/** @brief Intel RDT / AMD Platform-QoS Enforcement. */
#define EBX_RDT_A (1U << 15)
/** @brief AVX-512 foundation instructions. */
#define EBX_AVX512F (1U << 16)
/** @brief AVX-512 double/quadword instructions. */
#define EBX_AVX512DQ (1U << 17)
/** @brief RDSEED instruction. */
#define EBX_RDSEED (1U << 18)
/** @brief ADCX/ADOX instructions. */
#define EBX_ADX (1U << 19)
/** @brief Supervisor mode access prevention. */
#define EBX_SMAP (1U << 20)
/** @brief AVX-512 integer fused multiply add. */
#define EBX_AVX512IFMA (1U << 21)
/** @brief CLFLUSHOPT instruction. */
#define EBX_CLFLUSHOPT (1U << 23)
/** @brief CLWB instruction. */
#define EBX_CLWB (1U << 24)
/** @brief Intel processor trace. */
#define EBX_INTEL_PT (1U << 25)
/** @brief AVX-512 prefetch instructions. */
#define EBX_AVX512PF (1U << 26)
/** @brief AVX-512 exponent/reciprocal instructions. */
#define EBX_AVX512ER (1U << 27)
/** @brief AVX-512 conflict detection instructions. */
#define EBX_AVX512CD (1U << 28)
/** @brief SHA/SHA256 instructions. */
#define EBX_SHA (1U << 29)
/** @brief AVX-512 byte/word instructions. */
#define EBX_AVX512BW (1U << 30)
/** @brief AVX-512 VL (128/256 vector length) extensions. */
#define EBX_AVX512VL (1U << 31)
/** @brief PREFETCHWT1 (Intel Xeon Phi only). */
#define ECX_PREFETCHWT1 (1U << 0)
/** @brief AVX-512 Vector byte manipulation instructions. */
#define ECX_AVX512VBMI (1U << 1)
/** @brief User mode instruction protection. */
#define ECX_UMIP (1U << 2)
/** @brief Protection keys for user-space. */
#define ECX_PKU (1U << 3)
/** @brief OS protection keys enable. */
#define ECX_OSPKE (1U << 4)
/** @brief WAITPKG instructions. */
#define ECX_WAITPKG (1U << 5)
/** @brief AVX-512 vector byte manipulation instructions group 2. */
#define ECX_AVX512_VBMI2 (1U << 6)
/** @brief CET shadow stack features. */
#define ECX_CET_SS (1U << 7)
/** @brief Galois field new instructions. */
#define ECX_GFNI (1U << 8)
/** @brief Vector AES instructions. */
#define ECX_VAES (1U << 9)
/** @brief VPCLMULQDQ 256-bit instruction. */
#define ECX_VPCLMULQDQ (1U << 10)
/** @brief Vector neural network instructions. */
#define ECX_AVX512_VNNI (1U << 11)
/** @brief AVX-512 bitwise algorithms. */
#define ECX_AVX512_BITALG (1U << 12)
/** @brief Intel total memory encryption. */
#define ECX_TME (1U << 13)
/** @brief AVX-512: POPCNT for vectors of DWORD/QWORD. */
#define ECX_AVX512_VPOPCNTDQ (1U << 14)
/** @brief 57-bit linear addresses (five-level paging). */
#define ECX_LA57 (1U << 16)
/** @brief RDPID instruction. */
#define ECX_RDPID (1U << 22)
/** @brief Intel key locker. */
#define ECX_KEY_LOCKER (1U << 23)
/** @brief OS bus-lock detection. */
#define ECX_BUS_LOCK_DETECT (1U << 24)
/** @brief CLDEMOTE instruction. */
#define ECX_CLDEMOTE (1U << 25)
/** @brief MOVDIRI instruction. */
#define ECX_MOVDIRI (1U << 27)
/** @brief MOVDIR64B instruction. */
#define ECX_MOVDIR64B (1U << 28)
/** @brief Enqueue stores (ENQCMD{,S}). */
#define ECX_ENQCMD (1U << 29)
/** @brief Intel SGX launch configuration. */
#define ECX_SGX_LC (1U << 30)
/** @brief Protection keys for supervisor-mode pages. */
#define ECX_PKS (1U << 31)
/** @brief Intel SGX attestation services. */
#define EDX_SGX_KEYS (1U << 1)
/** @brief AVX-512 neural network instructions. */
#define EDX_AVX512_4VNNIW (1U << 2)
/** @brief AVX-512 multiply accumulation single precision. */
#define EDX_AVX512_4FMAPS (1U << 3)
/** @brief Fast short REP MOV. */
#define EDX_FSRM (1U << 4)
/** @brief User interrupts. */
#define EDX_UINTR (1U << 5)
/** @brief VP2INTERSECT{D,Q} instructions. */
#define EDX_AVX512_VP2INTERSECT (1U << 8)
/** @brief SRBDS mitigation MSR. */
#define EDX_SRDBS_CTRL (1U << 9)
/** @brief VERW MD_CLEAR microcode. */
#define EDX_MD_CLEAR (1U << 10)
/** @brief XBEGIN (RTM transaction) always aborts. */
#define EDX_RTM_ALWAYS_ABORT (1U << 11)
/** @brief MSR TSX_FORCE_ABORT, RTM_ABORT bit. */
#define EDX_TSX_FORCE_ABORT (1U << 13)
/** @brief SERIALIZE instruction. */
#define EDX_SERIALIZE (1U << 14)
/** @brief The CPU is identified as a 'hybrid part'. */
#define EDX_HYBRID_CPU (1U << 15)
/** @brief TSX suspend/resume load address tracking. */
#define EDX_TSXLDTRK (1U << 16)
/** @brief PCONFIG instruction. */
#define EDX_PCONFIG (1U << 18)
/** @brief Intel architectural LBRs. */
#define EDX_ARCH_LBR (1U << 19)
/** @brief CET indirect branch tracking. */
#define EDX_CET_IBT (1U << 20)
/** @brief AMX-BF16: tile bfloat16. */
#define EDX_AMX_BF16 (1U << 22)
/** @brief AVX-512 FP16 instructions. */
#define EDX_AVX512_FP16 (1U << 23)
/** @brief AMX-TILE: tile architecture. */
#define EDX_AMX_TILE (1U << 24)
/** @brief AMX-INT8: tile 8-bit integer. */
#define EDX_AMX_INT8 (1U << 25)
/** @brief Speculation Control (IBRS/IBPB: indirect branch restrictions). */
#define EDX_SPEC_CTRL (1U << 26)
/** @brief Single thread indirect branch predictors. */
#define EDX_INTEL_STIBP (1U << 27)
/** @brief FLUSH L1D cache: IA32_FLUSH_CMD MSR. */
#define EDX_FLUSH_L1D (1U << 28)
/** @brief Intel IA32_ARCH_CAPABILITIES MSR. */
#define EDX_ARCH_CAPABILITIES (1U << 29)
/** @brief IA32_CORE_CAPABILITIES MSR. */
#define EDX_CORE_CAPABILITIES (1U << 30)
/** @brief Speculative store bypass disable. */
#define EDX_SPEC_CTRL_SSBD (1U << 31)

/****************************************
 * CPUID Extended Features 0x00000007 Leaf 1
 ***************************************/

/** @brief AVX-VNNI instructions. */
#define EAX_AVX_VNNI (1U << 4)
/** @brief AVX-512 bfloat16 instructions. */
#define EAX_AVX512_BF16 (1U << 5)
/** @brief Linear address space separation. */
#define EAX_LASS (1U << 6)
/** @brief CMPccXADD instructions. */
#define EAX_CMPCCXADD (1U << 7)
/** @brief ArchPerfmonExt: leaf 0x23. */
#define EAX_ARCH_PERFMON_EXT (1U << 8)
/** @brief Fast zero-length REP MOVSB. */
#define EAX_FZRM (1U << 10)
/** @brief Fast short REP STOSB. */
#define EAX_FSRS (1U << 11)
/** @brief Fast Short REP CMPSB/SCASB. */
#define EAX_FSRC (1U << 12)
/** @brief FRED: Flexible return and event delivery transitions. */
#define EAX_FRED (1U << 17)
/** @brief LKGS: Load 'kernel' (userspace) GS. */
#define EAX_LKGS (1U << 18)
/** @brief WRMSRNS instruction (WRMSR-non-serializing). */
#define EAX_WRMSRNS (1U << 19)
/** @brief NMI-source reporting with FRED event data. */
#define EAX_NMI_SRC (1U << 20)
/** @brief AMX-FP16: FP16 tile operations. */
#define EAX_AMX_FP16 (1U << 21)
/** @brief HRESET (Thread director history reset). */
#define EAX_HRESET  (1U << 22)
/** @brief Integer fused multiply add. */
#define EAX_AVX_IFMA (1U << 23)
/** @brief Linear address masking. */
#define EAX_LAM (1U << 26)
/** @brief RDMSRLIST/WRMSRLIST instructions. */
#define EAX_RD_WR_MSRLIST (1U << 27)
/** @brief Protected processor inventory number (PPIN{,_CTL} MSRs). */
#define EBX_INTEL_PPIN (1U << 0)
/** @brief AVX-VNNI-INT8 instructions. */
#define EDX_AVX_VNNI_INT8 (1U << 4)
/** @brief AVX-NE-CONVERT instructions. */
#define EDX_AVX_NE_CONVERT (1U << 5)
/** @brief AMX-COMPLEX instructions (starting from Granite Rapids). */
#define EDX_AMX_COMPLEX (1U << 8)
/** @brief PREFETCHIT0/1 instructions. */
#define EDX_PREFETCHIT_0_1 (1U << 14)
/** @brief CET supervisor shadow stacks safe to use. */
#define EDX_CET_SSS (1U << 18)

/****************************************
 * CPUID Extended Features 0x00000007 Leaf 1
 ***************************************/

/** @brief Intel predictive store forward disable. */
#define EDX_INTEL_PSFD (1U << 0)
/** @brief MSR bits IA32_SPEC_CTRL.IPRED_DIS_{U,S}. */
#define EDX_IPRED_CTRL (1U << 1)
/** @brief MSR bits IA32_SPEC_CTRL.RRSBA_DIS_{U,S}. */
#define EDX_RRSBA_CTRL (1U << 2)
/** @brief MSR bit  IA32_SPEC_CTRL.DDPD_U. */
#define EDX_DDP_CTRL (1U << 3)
/** @brief MSR bit  IA32_SPEC_CTRL.BHI_DIS_S. */
#define EDX_BHI_CTRL (1U << 4)
/** @brief MCDT mitigation not needed. */
#define EDX_MCDT_NO (1U << 5)
/** @brief UC-lock disable. */
#define EDX_UCLOCK_DISABLE (1U << 6)


/****************************************
 * CPUID Extended Features 0x80000001
 ***************************************/

/** @brief LAHF and SAHF in 64-bit mode. */
#define ECX_LAHF_LM (1U << 0)
/** @brief Multi-processing legacy mode (No HT). */
#define ECX_CMP_LEGACY (1U << 1)
/** @brief Secure Virtual Machine. */
#define ECX_SVM (1U << 2)
/** @brief Extended APIC space. */
#define ECX_EXTAPIC (1U << 3)
/** @brief LOCK MOV CR0 means MOV CR8. */
#define ECX_CR8_LEGACY (1U << 4)
/** @brief LZCNT advanced bit manipulation. */
#define ECX_LZCNT_ABM (1U << 5)
/** @brief SSE4A support. */
#define ECX_SSE4A (1U << 6)
/** @brief Misaligned SSE mode. */
#define ECX_MISALIGNED_SSE (1U << 7)
/** @brief 3DNow PREFETCH/PREFETCHW support. */
#define ECX_3DNOW_PREFETCH (1U << 8)
/** @brief OS visible workaround. */
#define ECX_OSVW (1U << 9)
/** @brief Instruction based sampling. */
#define ECX_IBS (1U << 10)
/** @brief XOP: extended operation (AVX instructions). */
#define ECX_XOP (1U << 11)
/** @brief SKINIT/STGI support. */
#define ECX_SKINIT (1U << 12)
/** @brief Watchdog timer support. */
#define ECX_WDT (1U << 13)
/** @brief Lightweight profiling. */
#define ECX_LWP (1U << 15)
/** @brief 4-operand FMA instruction. */
#define ECX_FMA4 (1U << 16)
/** @brief Translation cache extension. */
#define ECX_TCE (1U << 17)
/** @brief NodeId MSR (0xc001100c). */
#define ECX_NODEID_MSR (1U << 19)
/** @brief Trailing bit manipulations. */
#define ECX_TBM (1U << 21)
/** @brief Topology Extensions (leaf 0x8000001d). */
#define ECX_TOPOEXT (1U << 22)
/** @brief Core performance counter extensions. */
#define ECX_PERFCTR_CORE (1U << 23)
/** @brief NB/DF performance counter extensions. */
#define ECX_PERFCTR_NB (1U << 24)
/** @brief Data access breakpoint extension. */
#define ECX_DATA_BP_EXT (1U << 26)
/** @brief Performance time-stamp counter. */
#define ECX_PERF_TSC (1U << 27)
/** @brief LLC (L3) performance counter extensions. */
#define ECX_PERFCTR_LLC (1U << 28)
/** @brief MWAITX/MONITORX support. */
#define ECX_MWAITX (1U << 29)
/** @brief Breakpoint address mask extension (to bit 31). */
#define ECX_ADDR_MASK_EXT (1U << 30)
/** @brief Floating-Point Unit on-chip (x87). */
#define EDX_E_FPU (1U << 0)
/** @brief Virtual-8086 Mode Extensions. */
#define EDX_E_VME (1U << 1)
/** @brief Debugging Extensions. */
#define EDX_E_DE (1U << 2)
/** @brief Page Size Extension. */
#define EDX_E_PSE (1U << 3)
/** @brief Time Stamp Counter. */
#define EDX_E_TSC (1U << 4)
/** @brief Model-Specific Registers (RDMSR and WRMSR support). */
#define EDX_E_MSR (1U << 5)
/** @brief Physical Address Extensions. */
#define EDX_E_PAE (1U << 6)
/** @brief Machine Check Exception. */
#define EDX_E_MCE (1U << 7)
/** @brief CMPXCHG8B instruction. */
#define EDX_E_CX8 (1U << 8)
/** @brief APIC on-chip. */
#define EDX_E_APIC (1U << 9)
/** @brief SYSCALL and SYSRET instructions. */
#define EDX_SYSCALL (1U << 11)
/** @brief Memory Type Range Registers. */
#define EDX_E_MTRR (1U << 12)
/** @brief Page Global Extensions. */
#define EDX_E_PGE (1U << 13)
/** @brief Machine Check Architecture. */
#define EDX_E_MCA (1U << 14)
/** @brief Conditional Move Instruction. */
#define EDX_E_CMOV (1U << 15)
/** @brief Page Attribute Table. */
#define EDX_E_PAT (1U << 16)
/** @brief Page Size Extension (36-bit). */
#define EDX_E_PSE36 (1U << 17)
/** @brief Out-of-spec AMD Multiprocessing bit. */
#define EDX_OBSOLETE_MP_BIT (1U << 19)
/** @brief No-execute page protection. */
#define EDX_NX (1U << 20)
/** @brief AMD MMX extensions. */
#define EDX_MMXEXT (1U << 22)
/** @brief MMX instructions. */
#define EDX_E_MMX (1U << 23)
/** @brief FXSAVE and FXRSTOR instructions. */
#define EDX_E_FXSR (1U << 24)
/** @brief FXSAVE and FXRSTOR optimizations. */
#define EDX_FXSR_OPT (1U << 25)
/** @brief 1-GB large page support. */
#define EDX_PAGE1GB (1U << 26)
/** @brief RDTSCP instruction. */
#define EDX_RDTSCP (1U << 27)
/** @brief Long mode (x86-64, 64-bit support). */
#define EDX_LM (1U << 29)
/** @brief AMD 3DNow extensions. */
#define EDX_3DNOWEXT (1U << 30)
/** @brief 3DNow instructions. */
#define EDX_3DNOW (1U << 31)


/****************************************
 * CPUID Extended Features 0x80000008
 ***************************************/
/** @brief CLZERO instruction. */
#define EBX_CLZERO (1U << 0)
/** @brief Instruction retired counter MSR. */
#define EBX_INSN_RETIRED_PERF (1U << 1)
/** @brief XSAVE/XRSTOR always saves/restores FPU error pointers. */
#define EBX_XSAVE_ERR_PTR (1U << 2)
/** @brief INVLPGB broadcasts a TLB invalidate. */
#define EBX_INVLPGB (1U << 3)
/** @brief RDPRU (Read Processor Register at User level). */
#define EBX_RDPRU (1U << 4)
/** @brief Memory Bandwidth Allocation (AMD bit). */
#define EBX_MBA (1U << 6)
/** @brief MCOMMIT instruction. */
#define EBX_MCOMMIT (1U << 8)
/** @brief WBNOINVD instruction. */
#define EBX_WBNOINVD (1U << 9)
/** @brief Indirect Branch Prediction Barrier. */
#define EBX_IBPB (1U << 12)
/** @brief Interruptible WBINVD/WBNOINVD. */
#define EBX_WBINVD_INT (1U << 13)
/** @brief Indirect Branch Restricted Speculation. */
#define EBX_IBRS (1U << 14)
/** @brief Single Thread Indirect Branch Prediction mode. */
#define EBX_STIBP (1U << 15)
/** @brief IBRS always-on preferred. */
#define EBX_IBRS_ALWAYS_ON (1U << 16)
/** @brief STIBP always-on preferred. */
#define EBX_STIBP_ALWAYS_ON (1U << 17)
/** @brief IBRS is preferred over software solution. */
#define EBX_IBRS_FAST (1U << 18)
/** @brief IBRS provides same mode protection. */
#define EBX_IBRS_SAME_MODE (1U << 19)
/** @brief Long-Mode Segment Limit Enable unsupported. */
#define EBX_NO_EFER_LMSLE (1U << 20)
/** @brief INVLPGB RAX[5] bit can be set. */
#define EBX_TLB_FLUSH_NESTED (1U << 21)
/** @brief Protected Processor Inventory Number. */
#define EBX_AMD_PPIN (1U << 23)
/** @brief Speculative Store Bypass Disable. */
#define EBX_AMD_SSBD (1U << 24)
/** @brief virtualized SSBD (Speculative Store Bypass Disable). */
#define EBX_VIRT_SSBD (1U << 25)
/** @brief SSBD is not needed (fixed in hardware). */
#define EBX_AMD_SSB_NO (1U << 26)
/** @brief Collaborative Processor Performance Control. */
#define EBX_CPPC (1U << 27)
/** @brief Predictive Store Forward Disable. */
#define EBX_AMD_PSFD (1U << 28)
/** @brief CPU not affected by Branch Type Confusion. */
#define EBX_BTC_NO (1U << 29)
/** @brief IBPB clears RSB/RAS too. */
#define EBX_IBPB_RET (1U << 30)
/** @brief Branch Sampling. */
#define EBX_BRANCH_SAMPLING (1U << 31)

/****************************************
 * CPUID Extended Features 0x80000021
 ***************************************/
/** @brief No nested data breakpoints. */
#define EAX_NO_NESTED_DATA_BP (1U << 0)
/** @brief WRMSR to {FS,GS,KERNEL_GS}_BASE is non-serializing. */
#define EAX_FSGS_NON_SERIALIZING (1U << 1)
/** @brief LFENCE always serializing / synchronizes RDTSC. */
#define EAX_LFENCE_SERIALIZING (1U << 2)
/** @brief SMM paging configuration lock. */
#define EAX_SMM_PAGE_CFG_LOCK (1U << 3)
/** @brief Null selector clears base. */
#define EAX_NULL_SEL_CLR_BASE (1U << 6)
/** @brief EFER MSR Upper Address Ignore. */
#define EAX_UPPER_ADDR_IGNORE (1U << 7)
/** @brief EFER MSR Automatic IBRS. */
#define EAX_AUTO_IBRS (1U << 8)
/** @brief SMM_CTL MSR not available. */
#define EAX_NO_SMM_CTL_MSR (1U << 9)
/** @brief Fast Short Rep STOSB. */
#define EAX_E_FSRS (1U << 10)
/** @brief Fast Short Rep CMPSB. */
#define EAX_E_FSRC (1U << 11)
/** @brief Prefetch control MSR. */
#define EAX_PREFETCH_CTL_MSR (1U << 13)
/** @brief Reserves opcode space. */
#define EAX_OPCODE_RECLAIM (1U << 16)
/** @brief #GP when executing CPUID at CPL > 0. */
#define EAX_USER_CPUID_DISABLE (1U << 17)
/** @brief Enhanced Predictive Store Forwarding. */
#define EAX_EPSF (1U << 18)
/** @brief Workload-based heuristic feedback to OS. */
#define EAX_WL_FEEDBACK (1U << 22)
/** @brief Enhanced Return Address Predictor Security. */
#define EAX_ERAPS (1U << 24)
/** @brief Selective Branch Predictor Barrier. */
#define EAX_SBPB (1U << 27)
/** @brief Branch predictions flushed from CPU branch predictor. */
#define EAX_IBPB_BRTYPE (1U << 28)
/** @brief No SRSO vulnerability. */
#define EAX_SRSO_NO (1U << 29)
/** @brief No SRSO at user-kernel boundary. */
#define EAX_SRSO_UK_NO (1U << 30)
/** @brief MSR BP_CFG[BpSpecReduce] SRSO mitigation. */
#define EAX_SRSO_MSR_FIX (1U << 31)

/****************************************
 * CPUID Masks and offsets
 ***************************************/

/** @brief Base CPU family ID mask. */
#define EAX_FAMILY_ID 0xF00
/** @brief Base CPU family ID offset. */
#define EAX_FAMILY_ID_OFF 8

/** @brief Extended CPU family ID mask. */
#define EAX_FAMILY_ID_EXT 0xFF00000
/** @brief Extended CPU family ID offset. */
#define EAX_FAMILY_ID_EXT_OFF 20

/** @brief Base CPU model ID mask. */
#define EAX_MODEL_ID 0xF0
/** @brief Base CPU model ID offset. */
#define EAX_MODEL_ID_OFF 4

/** @brief Extended CPU model ID mask. */
#define EAX_MODEL_ID_EXT 0xF0000
/** @brief Extended CPU model ID offset. */
#define EAX_MODEL_ID_EXT_OFF 16

/** @brief CPU stepping ID mask. */
#define EAX_STEPPING_ID 0xF
/** @brief CPU stepping ID offset. */
#define EAX_STEPPING_ID_OFF 0

/** @brief CLFLUSH instruction cache line size mask. */
#define EBX_CFLUSH_SIZE 0xFF00
/** @brief CLFLUSH instruction cache line size offset. */
#define EBX_CFLUSH_SIZE_OFF 8

/** @brief Initial local APIC physical ID mask. */
#define EBX_INIT_LAPIC 0xFF000000
/** @brief Initial local APIC physical ID offset. */
#define EBX_INIT_LAPIC_OFF 24

/** @brief Cache type field mask. */
#define EAX_CACHE_TYPE 0x1F
/** @brief Cache type field offset. */
#define EAX_CACHE_TYPE_OFF 0

/** @brief Cache level (1-based) mask. */
#define EAX_CACHE_LEVEL 0xE0
/** @brief Cache level (1-based) offset. */
#define EAX_CACHE_LEVEL_OFF 5

/** @brief Ways of associativity (0-based) mask. */
#define EBX_CACHE_WAYS 0xFFC00000
/** @brief Ways of associativity (0-based) offset. */
#define EBX_CACHE_WAYS_OFF 22

/** @brief Physical line partitions (0-based) mask. */
#define EBX_CACHE_PARTS 0x003FF000
/** @brief Physical line partitions (0-based) offset. */
#define EBX_CACHE_PARTS_OFF 12

/** @brief System coherency line size (0-based) mask. */
#define EBX_CACHE_LINESIZE 0xFFF
/** @brief System coherency line size (0-based) offset. */
#define EBX_CACHE_LINESIZE_OFF 0

/** @brief Cache number of sets (0-based) mask. */
#define ECX_CACHE_SETS 0xFFFFFFFF
/** @brief Cache number of sets (0-based) offset. */
#define ECX_CACHE_SETS_OFF 0

/** @brief L1 icache line size, in bytes mask. */
#define EDX_AMD_ICACHE_LINESIZE 0xFF
/** @brief L1 icache line size, in bytes iffset. */
#define EDX_AMD_ICACHE_LINESIZE_OFF 0

/** @brief L1 icache associativity mask. */
#define EDX_AMD_ICACHE_WAYS 0xFF0000
/** @brief L1 icache associativity offset. */
#define EDX_AMD_ICACHE_WAYS_OFF 16

/** @brief L1 icache size, in KB mask. */
#define EDX_AMD_ICACHE_SZE 0xFF000000
/** @brief L1 icache size, in KB offset. */
#define EDX_AMD_ICACHE_SZE_OFF 24

/** @brief icache number of lines per tag mask. */
#define EDX_ICACHE_PARTS 0xFF00
/** @brief icache number of lines per tag offset. */
#define EDX_ICACHE_PARTS_OFF 8

/** @brief L1 dcache line size, in bytes mask. */
#define ECX_AMD_DCACHE_LINESIZE 0xFF
/** @brief L1 dcache line size, in bytes iffset. */
#define ECX_AMD_DCACHE_LINESIZE_OFF 0

/** @brief L1 dcache associativity mask. */
#define ECX_AMD_DCACHE_WAYS 0xFF0000
/** @brief L1 dcache associativity offset. */
#define ECX_AMD_DCACHE_WAYS_OFF 16

/** @brief L1 dcache size, in KB mask. */
#define ECX_AMD_DCACHE_SZE 0xFF000000
/** @brief L1 dcache size, in KB offset. */
#define ECX_AMD_DCACHE_SZE_OFF 24

/** @brief L1 dcache number of lines per tag mask. */
#define ECX_DCACHE_PARTS 0xFF00
/** @brief L1 dcache number of lines per tag offset. */
#define ECX_DCACHE_PARTS_OFF 8

/** @brief L2 cache line size, in bytes mask. */
#define ECX_AMD_L2CACHE_LINESIZE 0xFF
/** @brief L2 cache line size, in bytes offset. */
#define ECX_AMD_L2CACHE_LINESIZE_OFF 0

/** @brief L2 cache associativity mask. */
#define ECX_AMD_L2CACHE_WAYS 0xF000
/** @brief L2 cache associativity offset. */
#define ECX_AMD_L2CACHE_WAYS_OFF 12

/** @brief L2 cache size, in KB mask. */
#define ECX_AMD_L2CACHE_SZE 0xFFFF0000
/** @brief L2 cache size, in KB offset. */
#define ECX_AMD_L2CACHE_SZE_OFF 16

/** @brief L2 cache number of lines per tag mask. */
#define ECX_L2CACHE_PARTS 0xF00
/** @brief L2 cache number of lines per tag offset. */
#define ECX_L2CACHE_PARTS_OFF 8


/** @brief L3 cache line size, in bytes mask. */
#define EDX_AMD_L3CACHE_LINESIZE 0xFF
/** @brief L3 cache line size, in bytes offset. */
#define EDX_AMD_L3CACHE_LINESIZE_OFF 0

/** @brief L3 cache associativity tag mask. */
#define EDX_AMD_L3CACHE_WAYS 0xF000
/** @brief L3 cache associativity tag offset. */
#define EDX_AMD_L3CACHE_WAYS_OFF 12

/** @brief L3 cache size, in KB mask. */
#define EDX_AMD_L3CACHE_SZE 0xFFFC0000
/** @brief L3 cache size, in KB offset. */
#define EDX_AMD_L3CACHE_SZE_OFF 18

/** @brief L3 cache number of lines per tag mask. */
#define EDX_L3CACHE_PARTS 0xF00
/** @brief L3 cache number of lines per tag offset. */
#define EDX_L3CACHE_PARTS_OFF 8

/****************************************
 * CPUID Registers
 ***************************************/
/** @brief CPUID Register: EAX */
#define EAX_REG 0
/** @brief CPUID Register: EBX */
#define EBX_REG 1
/** @brief CPUID Register: ECX */
#define ECX_REG 2
/** @brief CPUID Register: EDX */
#define EDX_REG 3

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

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
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/**
 * @brief Gets the CPU family ID value.
 *
 * @details Gets the CPU family ID value. This value is computed based on the
 * field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU family ID value is returned.
 */
#define CPUID_FAMILY_ID(REGS) \
    ((REGS[EAX_REG] & EAX_FAMILY_ID) >> EAX_FAMILY_ID_OFF)

/**
 * @brief Gets the CPU extended family ID value.
 *
 * @details Gets the CPU extended family ID value. This value is computed based
 * on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU extended family ID value is returned.
 */
#define CPUID_FAMILY_ID_EXT(REGS) \
    ((REGS[EAX_REG] & EAX_FAMILY_ID_EXT) >> EAX_FAMILY_ID_EXT_OFF)

/**
 * @brief Gets the CPU model ID value.
 *
 * @details Gets the CPU model ID value. This value is computed based on the
 * field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU model ID value is returned.
 */
#define CPUID_MODEL_ID(REGS) \
    ((REGS[EAX_REG] & EAX_MODEL_ID) >> EAX_MODEL_ID_OFF)

/**
 * @brief Gets the CPU extended model ID value.
 *
 * @details Gets the CPU extended model ID value. This value is computed based
 * on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU extended model ID value is returned.
 */
#define CPUID_MODEL_ID_EXT(REGS) \
    (((REGS[EAX_REG] & EAX_MODEL_ID_EXT) >> EAX_MODEL_ID_EXT_OFF) << 4)

/**
 * @brief Gets the CPU stepping ID value.
 *
 * @details Gets the CPU stepping ID value. This value is computed based on the
 * field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU stepping ID value is returned.
 */
#define CPUID_STEPPING_ID(REGS) \
    ((REGS[EAX_REG] & EAX_STEPPING_ID) >> EAX_STEPPING_ID_OFF)

/**
 * @brief Gets the CPU cache CFLUSH instruction line size.
 *
 * @details Gets the CPU cache CFLUSH instruction line size. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache CFLUSH instruction line size is returned.
 */
#define CPUID_CFLUSH_SIZE(REGS) \
    ((REGS[EBX_REG] & EBX_CFLUSH_SIZE) >> EBX_CFLUSH_SIZE_OFF)

/**
 * @brief Gets the CPU LAPIC ID.
 *
 * @details Gets the CPU LAPIC ID. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU LAPIC ID is returned.
 */
#define CPUID_INIT_LAPIC(REGS) \
    ((REGS[EBX_REG] & EBX_INIT_LAPIC) >> EBX_INIT_LAPIC_OFF)

/**
 * @brief Gets the CPU cache type.
 *
 * @details Gets the CPU cache type. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache type is returned.
 */
#define CPUID_CACHE_TYPE(REGS) \
    ((REGS[EAX_REG] & EAX_CACHE_TYPE) >> EAX_CACHE_TYPE_OFF)

/**
 * @brief Gets the CPU cache level.
 *
 * @details Gets the CPU cache level. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache level is returned.
 */
#define CPUID_CACHE_LEVEL(REGS) \
    ((REGS[EAX_REG] & EAX_CACHE_LEVEL) >> EAX_CACHE_LEVEL_OFF)

/**
 * @brief Gets the CPU cache ways.
 *
 * @details Gets the CPU cache ways. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache ways is returned.
 */
#define CPUID_CACHE_WAYS(REGS) \
    ((REGS[EBX_REG] & EBX_CACHE_WAYS) >> EBX_CACHE_WAYS_OFF)

/**
 * @brief Gets the CPU cache partitions.
 *
 * @details Gets the CPU cache partitions. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache partitions is returned.
 */
#define CPUID_CACHE_PARTS(REGS) \
    ((REGS[EBX_REG] & EBX_CACHE_PARTS) >> EBX_CACHE_PARTS_OFF)

/**
 * @brief Gets the CPU cache line size.
 *
 * @details Gets the CPU cache line size. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache line size is returned.
 */
#define CPUID_CACHE_LINESIZE(REGS) \
    ((REGS[EBX_REG] & EBX_CACHE_LINESIZE) >> EBX_CACHE_LINESIZE_OFF)

/**
 * @brief Gets the CPU cache sets.
 *
 * @details Gets the CPU cache setse. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache sets is returned.
 */
#define CPUID_CACHE_SETS(REGS) \
    ((REGS[ECX_REG] & ECX_CACHE_SETS) >> ECX_CACHE_SETS_OFF)

/**
 * @brief Calculates the total cache size.
 *
 * @details Calculates the total cache size. This value is based on previously
 * gathered data bout the cache.
 *
 * @param[in] CPU_INFO The CPU information structure to use.
 *
 * @return The total CPU cache size returned.
 */
#define CPUID_CACHE_SIZE(CPU_INFO)              \
    ((CPU_INFO->ways) * (CPU_INFO->parts) *     \
     (CPU_INFO->sets) * (CPU_INFO->lineSize))

/**
 * @brief Gets the AMD CPU cache line size for L1 Instruction.
 *
 * @details Gets the AMD CPU cache line size for L1 Instruction. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache line size is returned.
 */
#define CPUID_AMD_ICACHE_LINESIZE(REGS) \
    ((REGS[EDX_REG] & EDX_AMD_ICACHE_LINESIZE) >> EDX_AMD_ICACHE_LINESIZE_OFF)

/**
 * @brief Gets the AMD CPU cache ways for L1 Instruction.
 *
 * @details Gets the CPU cache ways for L1 Instruction. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache ways is returned.
 */
#define CPUID_AMD_ICACHE_WAYS(REGS) \
    ((REGS[EDX_REG] & EDX_AMD_ICACHE_WAYS) >> EDX_AMD_ICACHE_WAYS_OFF)

/**
 * @brief Gets the AMD CPU cache partitions for L1 Instructions.
 *
 * @details Gets the AMDCPU cache partitions for L1 Instructions. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache partitions is returned.
 */
#define CPUID_AMD_ICACHE_PARTS(REGS) \
    ((REGS[EDX_REG] & EDX_ICACHE_PARTS) >> EDX_ICACHE_PARTS_OFF)

/**
 * @brief Gets the AMD CPU cache sets for L1 Instruction.
 *
 * @details Gets the CPU cache sets for L1 Instruction. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache sets is returned.
 */
#define CPUID_AMD_ICACHE_SETS(REGS)                                            \
    ((((REGS[EDX_REG] & EDX_AMD_ICACHE_SZE) >> EDX_AMD_ICACHE_SZE_OFF) * 1024) \
     / (CPUID_AMD_ICACHE_WAYS(REGS) * CPUID_AMD_ICACHE_LINESIZE(REGS) *        \
        CPUID_AMD_ICACHE_PARTS(REGS)))

/**
 * @brief Gets the AMD CPU cache line size for L1 Data.
 *
 * @details Gets the AMD CPU cache line size for L1 Data. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache line size is returned.
 */
#define CPUID_AMD_DCACHE_LINESIZE(REGS) \
    ((REGS[ECX_REG] & ECX_AMD_DCACHE_LINESIZE) >> ECX_AMD_DCACHE_LINESIZE_OFF)

    /**
 * @brief Gets the AMD CPU cache ways for L1 Data.
 *
 * @details Gets the CPU cache ways for L1 Data. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache ways is returned.
 */
#define CPUID_AMD_DCACHE_WAYS(REGS) \
    ((REGS[ECX_REG] & ECX_AMD_DCACHE_WAYS) >> ECX_AMD_DCACHE_WAYS_OFF)

/**
 * @brief Gets the AMD CPU cache partitions for L1 Data.
 *
 * @details Gets the AMDCPU cache partitions for L1 Data. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache partitions is returned.
 */
#define CPUID_AMD_DCACHE_PARTS(REGS) \
    ((REGS[ECX_REG] & ECX_DCACHE_PARTS) >> ECX_DCACHE_PARTS_OFF)

/**
 * @brief Gets the AMD CPU cache sets for L1 Data.
 *
 * @details Gets the CPU cache sets for L1 Data. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache sets is returned.
 */
#define CPUID_AMD_DCACHE_SETS(REGS)                                            \
    ((((REGS[ECX_REG] & ECX_AMD_DCACHE_SZE) >> ECX_AMD_DCACHE_SZE_OFF) * 1024) \
     / (CPUID_AMD_DCACHE_WAYS(REGS) * CPUID_AMD_DCACHE_LINESIZE(REGS) *        \
        CPUID_AMD_DCACHE_PARTS(REGS)))

/**
 * @brief Gets the AMD CPU cache line size for L2.
 *
 * @details Gets the AMD CPU cache line size for L2. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache line size is returned.
 */
#define CPUID_AMD_L2CACHE_LINESIZE(REGS) \
    ((REGS[ECX_REG] & ECX_AMD_L2CACHE_LINESIZE) >> ECX_AMD_L2CACHE_LINESIZE_OFF)

/**
 * @brief Gets the AMD CPU cache ways for L2.
 *
 * @details Gets the CPU cache ways for L2. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache ways is returned.
 */
#define CPUID_AMD_L2CACHE_WAYS(REGS) \
    ((REGS[ECX_REG] & ECX_AMD_L2CACHE_WAYS) >> ECX_AMD_L2CACHE_WAYS_OFF)

/**
 * @brief Gets the AMD CPU cache partitions for L2.
 *
 * @details Gets the AMDCPU cache partitions for L2. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache partitions is returned.
 */
#define CPUID_AMD_L2CACHE_PARTS(REGS) \
    ((REGS[ECX_REG] & ECX_L2CACHE_PARTS) >> ECX_L2CACHE_PARTS_OFF)

/**
 * @brief Gets the AMD CPU cache sets for L2.
 *
 * @details Gets the CPU cache sets for L2. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache sets is returned.
 */
#define CPUID_AMD_L2CACHE_SETS(REGS)                                         \
    ((((REGS[ECX_REG] & ECX_AMD_L2CACHE_SZE) >> ECX_AMD_L2CACHE_SZE_OFF)     \
    * 1024)                                                                  \
     / (CPUID_AMD_L2CACHE_WAYS(REGS) * CPUID_AMD_L2CACHE_LINESIZE(REGS)*     \
        CPUID_AMD_L2CACHE_PARTS(REGS)))

/**
 * @brief Gets the AMD CPU cache line size for L3.
 *
 * @details Gets the AMD CPU cache line size for L3. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache line size is returned.
 */
#define CPUID_AMD_L3CACHE_LINESIZE(REGS) \
    ((REGS[EDX_REG] & EDX_AMD_L3CACHE_LINESIZE) >> EDX_AMD_L3CACHE_LINESIZE_OFF)

/**
 * @brief Gets the AMD CPU cache ways for L3.
 *
 * @details Gets the CPU cache ways for L3. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache ways is returned.
 */
#define CPUID_AMD_L3CACHE_WAYS(REGS) \
    ((REGS[EDX_REG] & EDX_AMD_L3CACHE_WAYS) >> EDX_AMD_L3CACHE_WAYS_OFF)

/**
 * @brief Gets the AMD CPU cache partitions for L3.
 *
 * @details Gets the AMDCPU cache partitions for L3. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache partitions is returned.
 */
#define CPUID_AMD_L3CACHE_PARTS(REGS) \
    ((REGS[EDX_REG] & EDX_L3CACHE_PARTS) >> EDX_L3CACHE_PARTS_OFF)

/**
 * @brief Gets the AMD CPU cache sets for L3.
 *
 * @details Gets the CPU cache sets for L3. This value is
 * computed based on the field offset and mask.
 *
 * @param[in] REGS The CPUID registers containing the data.
 *
 * @return The CPU cache sets is returned.
 */
#define CPUID_AMD_L3CACHE_SETS(REGS)                                      \
    ((((REGS[EDX_REG] & EDX_AMD_L3CACHE_SZE) >> EDX_AMD_L3CACHE_SZE_OFF)  \
      * 512 * 1024)                                                       \
     / (CPUID_AMD_L3CACHE_WAYS(REGS) * CPUID_AMD_L3CACHE_LINESIZE(REGS) * \
        CPUID_AMD_L3CACHE_PARTS(REGS)))


/**
 * @brief Prints the name of the flag if the flag is enabled.
 *
 * @param[in] FLAG The flag to test.
 * @param[in] STR The string of the flag.
 */
#define PRINT_FLAG_INFO(FLAG, STR) {                                \
    if(FLAG == true)                                                \
    {                                                               \
        offset = snprintf(pBuffer, length, "%s ", STR);             \
        pBuffer += offset;                                          \
        length -= offset;                                           \
        mainOffset += offset;                                       \
        if(length == 0)                                             \
        {                                                           \
            return mainOffset;                                      \
        }                                                           \
    }                                                               \
}
/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
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
static uint32_t _getCPUIDMax(const uint32_t kExt);

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
static int32_t _performCPUID(const uint32_t kCode, uint32_t pRegs[4]);

/**
 * @brief Retrieves the CPU vendor information.
 *
 * @details Retrieves the CPU vendor information. This function uses the CPUID
 * functions to gather the CPU information.
 *
 * @param[out] pCpuInf The CPU information structure to fill with the
 * information.
 */
static void _getVendorInfo(cpu_info_t* pCpuInf);

/**
 * @brief Retrieves the CPU name information.
 *
 * @details Retrieves the CPU name information. This function uses the CPUID
 * functions to gather the CPU information.
 *
 * @param[out] pCpuInf The CPU information structure to fill with the
 * information.
 */
static void _getCPUName(cpu_info_t* pCpuInf);

/**
 * @brief Retrieves the CPU feature information.
 *
 * @details Retrieves the CPU feature information. This function uses the CPUID
 * functions to gather the CPU information.
 *
 * @param[out] pCpuInf The CPU information structure to fill with the
 * information.
 */
static void _getGeneralFeatures(cpu_info_t* pCpuInf);

/**
 * @brief Retrieves the CPU general information.
 *
 * @details Retrieves the CPU general information. This function uses the CPUID
 * functions to gather the CPU information.
 *
 * @param[out] pCpuInf The CPU information structure to fill with the
 * information.
 */
static void _getGeneralInformation(cpu_info_t* pCpuInf);

/**
 * @brief Retrieves the CPU memory information.
 *
 * @details Retrieves the CPU memory information. This function uses the CPUID
 * functions to gather the CPU information.
 *
 * @param[out] pCpuInf The CPU information structure to fill with the
 * information.
 */
static void _getMemoryInformation(cpu_info_t* pCpuInf);

/**
 * @brief Retrieves the CPU TLB information.
 *
 * @details Retrieves the CPU TLB information. This function uses the CPUID
 * functions to gather the CPU information.
 *
 * @param[out] pCpuInf The CPU information structure to fill with the
 * information.
 */
static void _getTLBInformation(cpu_info_t* pCpuInf);

/**
 * @brief Retrieves the CPU cache information.
 *
 * @details Retrieves the CPU cache information. This function uses the CPUID
 * functions to gather the CPU information.
 *
 * @param[out] pCpuInf The CPU information structure to fill with the
 * information.
 */
static void _getCacheInformation(cpu_info_t* pCpuInf);


/**
 * @brief Retrieves the CPU cache information.
 *
 * @details Retrieves the CPU cache information. This function uses the CPUID
 * functions unified for AMD and Intel.
 *
 * @param[out] pCpuInf The CPU information structure to fill with the
 * information.
 */
static void _getCacheInformationUnified(cpu_info_t*    pCpuInf,
                                        const uint32_t kLeaf);

/**
 * @brief Returns the cache associativity of an AMD processor.
 *
 * @details Returns the cache associativity of an AMD processor.
 *
 * @param[in] kBase The associativity as read from the CPUID instruction.
 *
 * @return Returns the cache associativity of an AMD processor.
 */
static uint16_t _getCacheAssocAmd(const uint32_t kBase);

/**
 * @brief Retrieves the CPU cache information.
 *
 * @details Retrieves the CPU cache information. This function uses the CPUID
 * functions specific to AMD.
 *
 * @param[out] pCpuInf The CPU information structure to fill with the
 * information.
 */
static void _getCacheInformationAmd(cpu_info_t* pCpuInf);

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

static uint32_t _getCPUIDMax(const uint32_t kExt)
{
    uint32_t regs[4];

    /* Host supports CPUID. Return highest supported CPUID input value. */
    __asm__ __volatile__("cpuid":
                         "=a"(*regs),
                         "=b"(*(regs+1)),
                         "=c"(*(regs+2)),
                         "=d"(*(regs+3)):
                         "a"(kExt));

    return regs[EAX_REG];
}

static int32_t _performCPUID(const uint32_t kCode, uint32_t pRegs[4])
{
    uint32_t ext;
    uint32_t maxLevel;

    ext      = kCode & 0x80000000;
    maxLevel = _getCPUIDMax(ext);

    if (maxLevel == 0 || maxLevel < kCode)
    {
        return 0;
    }
    __asm__ __volatile__("cpuid":
                         "=a"(*pRegs),
                         "=b"(*(pRegs + 1)),
                         "=c"(*(pRegs + 2)),
                         "=d"(*(pRegs + 3)):
                         "a"(kCode));
    return 1;
}

static void _getVendorInfo(cpu_info_t* pCpuInf)
{
    int32_t  ret;
    uint8_t  i;
    uint32_t regs[4];

    ret =_performCPUID(CPUID_GETVENDORSTRING, regs);
    if(ret == 1)
    {
        for(i = 0; i < 4; ++i)
        {
            pCpuInf->pVendor[i] = (char)((regs[EBX_REG] >> (i * 8)) & 0xFF);
        }
        for(i = 0; i < 4; ++i)
        {
            pCpuInf->pVendor[4 + i] = (char)((regs[EDX_REG] >> (i * 8)) & 0xFF);
        }
        for(i = 0; i < 4; ++i)
        {
            pCpuInf->pVendor[8 + i] = (char)((regs[ECX_REG] >> (i * 8)) & 0xFF);
        }
    }
    else
    {
        memcpy(pCpuInf->pVendor, "UNKNOWN    ", CPU_VENDOR_STR_SIZE);
    }

    if(strncmp(pCpuInf->pVendor,
               CPUID_VENDOR_INTEL,
               CPU_VENDOR_STR_SIZE) == 0)
    {
        pCpuInf->type = CPU_INTEL;
    }
    else if(strncmp(pCpuInf->pVendor,
                    CPUID_VENDOR_AMD,
                    CPU_VENDOR_STR_SIZE) == 0)
    {
        pCpuInf->type = CPU_AMD;
    }
    else if(strncmp(pCpuInf->pVendor,
                    CPUID_VENDOR_AMD_K5,
                    CPU_VENDOR_STR_SIZE) == 0)
    {
        pCpuInf->type = CPU_AMD;
    }
    else
    {
        CPU_ASSERT(false, "CPU is not supported.", OS_ERR_NOT_SUPPORTED);
    }
}

static void _getCPUName(cpu_info_t* pCpuInf)
{
    int32_t  ret;
    uint32_t regs[4];

    strncpy(pCpuInf->pName, "UNKNOWN", 8);
    ret = _performCPUID(CPUID_GETBRANDSTRING_START, regs);
    if(ret == 1)
    {
        memcpy(pCpuInf->pName, (uint8_t*)regs, 16);
    }
    ret = _performCPUID(CPUID_GETBRANDSTRING_MID, regs);
    if(ret == 1)
    {
        memcpy(pCpuInf->pName + 16, (uint8_t*)regs, 16);
    }
    ret = _performCPUID(CPUID_GETBRANDSTRING_END, regs);
    if(ret == 1)
    {
        memcpy(pCpuInf->pName + 32, (uint8_t*)regs, 16);
    }
}

static void _getGeneralInformation(cpu_info_t* pCpuInf)
{
    int32_t  ret;
    uint32_t regs[4];

    /* Get basic CPU information */
    ret = _performCPUID(CPUID_GETFEATURES, regs);
    if(ret == 1)
    {
        pCpuInf->family = CPUID_FAMILY_ID(regs);
        pCpuInf->model = CPUID_MODEL_ID(regs);
        if(pCpuInf->family == 0xF)
        {
            pCpuInf->family += CPUID_FAMILY_ID_EXT(regs);
            pCpuInf->model += CPUID_MODEL_ID_EXT(regs);
        }
        else if(pCpuInf->family == 0x6)
        {
            pCpuInf->model += CPUID_MODEL_ID_EXT(regs);
        }

        pCpuInf->stepping = CPUID_STEPPING_ID(regs);
        pCpuInf->clFlushSize = CPUID_CFLUSH_SIZE(regs) * 8;
        pCpuInf->initialApicId = CPUID_INIT_LAPIC(regs);

    }
    else
    {
        pCpuInf->family = -1;
        pCpuInf->model = -1;
        pCpuInf->stepping = -1;
        pCpuInf->clFlushSize = -1;
        pCpuInf->initialApicId = -1;
    }

    /* Get the max CPUID */
    ret = _performCPUID(CPUID_GETEXTENDED_AVAILABLE, regs);
    if(ret == 1)
    {
        pCpuInf->cpuIdLevel = regs[EAX_REG];
    }
    else
    {
        pCpuInf->cpuIdLevel = 0;
    }
}

static void _getGeneralFeatures(cpu_info_t* pCpuInf)
{
    int32_t  ret;
    uint32_t regs[4];
    uint32_t maxLeaves;

    ret = _performCPUID(CPUID_GETFEATURES, regs);
    if(ret == 1)
    {
        pCpuInf->flags.sse3 = ((regs[ECX_REG] & ECX_SSE3) == ECX_SSE3);
        pCpuInf->flags.pclmulqdq =
            ((regs[ECX_REG] & ECX_PCLMULQDQ) == ECX_PCLMULQDQ);
        pCpuInf->flags.dtes64 = ((regs[ECX_REG] & ECX_DTES64) == ECX_DTES64);
        pCpuInf->flags.monitor = ((regs[ECX_REG] & ECX_MONITOR) == ECX_MONITOR);
        pCpuInf->flags.dscpl = ((regs[ECX_REG] & ECX_DSCPL) == ECX_DSCPL);
        pCpuInf->flags.vmx = ((regs[ECX_REG] & ECX_VMX) == ECX_VMX);
        pCpuInf->flags.smx = ((regs[ECX_REG] & ECX_SMX) == ECX_SMX);
        pCpuInf->flags.est = ((regs[ECX_REG] & ECX_EST) == ECX_EST);
        pCpuInf->flags.tm2 = ((regs[ECX_REG] & ECX_TM2) == ECX_TM2);
        pCpuInf->flags.ssse3 = ((regs[ECX_REG] & ECX_SSSE3) == ECX_SSSE3);
        pCpuInf->flags.cntxt_id =
            ((regs[ECX_REG] & ECX_CNTXT_ID) == ECX_CNTXT_ID);
        pCpuInf->flags.sdbg = ((regs[ECX_REG] & ECX_SDBG) == ECX_SDBG);
        pCpuInf->flags.fma = ((regs[ECX_REG] & ECX_FMA) == ECX_FMA);
        pCpuInf->flags.cx16 = ((regs[ECX_REG] & ECX_CX16) == ECX_CX16);
        pCpuInf->flags.xtpr_update =
            ((regs[ECX_REG] & ECX_XTPR_UPDATE) == ECX_XTPR_UPDATE);
        pCpuInf->flags.pdcm = ((regs[ECX_REG] & ECX_PDCM) == ECX_PDCM);
        pCpuInf->flags.pcid = ((regs[ECX_REG] & ECX_PCID) == ECX_PCID);
        pCpuInf->flags.dca = ((regs[ECX_REG] & ECX_DCA) == ECX_DCA);
        pCpuInf->flags.sse4_1 = ((regs[ECX_REG] & ECX_SSE4_1) == ECX_SSE4_1);
        pCpuInf->flags.sse4_2 = ((regs[ECX_REG] & ECX_SSE4_2) == ECX_SSE4_2);
        pCpuInf->flags.x2apic = ((regs[ECX_REG] & ECX_X2APIC) == ECX_X2APIC);
        pCpuInf->flags.movbe = ((regs[ECX_REG] & ECX_MOVBE) == ECX_MOVBE);
        pCpuInf->flags.popcnt = ((regs[ECX_REG] & ECX_POPCNT) == ECX_POPCNT);
        pCpuInf->flags.tsc_deadline_timer =
            ((regs[ECX_REG] & ECX_TSC_DEADLINE_TIMER) ==
              ECX_TSC_DEADLINE_TIMER);
        pCpuInf->flags.aes = ((regs[ECX_REG] & ECX_AES) == ECX_AES);
        pCpuInf->flags.xsave = ((regs[ECX_REG] & ECX_XSAVE) == ECX_XSAVE);
        pCpuInf->flags.osxsave = ((regs[ECX_REG] & ECX_OSXSAVE) == ECX_OSXSAVE);
        pCpuInf->flags.avx = ((regs[ECX_REG] & ECX_AVX) == ECX_AVX);
        pCpuInf->flags.f16c = ((regs[ECX_REG] & ECX_F16C) == ECX_F16C);
        pCpuInf->flags.rdrand = ((regs[ECX_REG] & ECX_RDRAND) == ECX_RDRAND);
        pCpuInf->flags.guest_status =
            ((regs[ECX_REG] & ECX_GUEST_STATUS) == ECX_GUEST_STATUS);
        pCpuInf->flags.fpu = ((regs[EDX_REG] & EDX_FPU) == EDX_FPU);
        pCpuInf->flags.vme = ((regs[EDX_REG] & EDX_VME) == EDX_VME);
        pCpuInf->flags.de = ((regs[EDX_REG] & EDX_DE) == EDX_DE);
        pCpuInf->flags.pse = ((regs[EDX_REG] & EDX_PSE) == EDX_PSE);
        pCpuInf->flags.tsc = ((regs[EDX_REG] & EDX_TSC) == EDX_TSC);
        pCpuInf->flags.msr = ((regs[EDX_REG] & EDX_MSR) == EDX_MSR);
        pCpuInf->flags.pae = ((regs[EDX_REG] & EDX_PAE) == EDX_PAE);
        pCpuInf->flags.mce = ((regs[EDX_REG] & EDX_MCE) == EDX_MCE);
        pCpuInf->flags.cx8 = ((regs[EDX_REG] & EDX_CX8) == EDX_CX8);
        pCpuInf->flags.apic = ((regs[EDX_REG] & EDX_APIC) == EDX_APIC);
        pCpuInf->flags.sep = ((regs[EDX_REG] & EDX_SEP) == EDX_SEP);
        pCpuInf->flags.mtrr = ((regs[EDX_REG] & EDX_MTRR) == EDX_MTRR);
        pCpuInf->flags.pge = ((regs[EDX_REG] & EDX_PGE) == EDX_PGE);
        pCpuInf->flags.mca = ((regs[EDX_REG] & EDX_MCA) == EDX_MCA);
        pCpuInf->flags.cmov = ((regs[EDX_REG] & EDX_CMOV) == EDX_CMOV);
        pCpuInf->flags.pat = ((regs[EDX_REG] & EDX_PAT) == EDX_PAT);
        pCpuInf->flags.pse36 = ((regs[EDX_REG] & EDX_PSE36) == EDX_PSE36);
        pCpuInf->flags.psn = ((regs[EDX_REG] & EDX_PSN) == EDX_PSN);
        pCpuInf->flags.clflush = ((regs[EDX_REG] & EDX_CLFLUSH) == EDX_CLFLUSH);
        pCpuInf->flags.ds = ((regs[EDX_REG] & EDX_DS) == EDX_DS);
        pCpuInf->flags.acpi = ((regs[EDX_REG] & EDX_ACPI) == EDX_ACPI);
        pCpuInf->flags.mmx = ((regs[EDX_REG] & EDX_MMX) == EDX_MMX);
        pCpuInf->flags.fxsr = ((regs[EDX_REG] & EDX_FXSR) == EDX_FXSR);
        pCpuInf->flags.sse = ((regs[EDX_REG] & EDX_SSE) == EDX_SSE);
        pCpuInf->flags.sse2 = ((regs[EDX_REG] & EDX_SSE2) == EDX_SSE2);
        pCpuInf->flags.selfsnoop =
            ((regs[EDX_REG] & EDX_SELFSNOOP) == EDX_SELFSNOOP);
        pCpuInf->flags.htt = ((regs[EDX_REG] & EDX_HTT) == EDX_HTT);
        pCpuInf->flags.tm = ((regs[EDX_REG] & EDX_TM) == EDX_TM);
        pCpuInf->flags.ia64 = ((regs[EDX_REG] & EDX_IA64) == EDX_IA64);
        pCpuInf->flags.pbe = ((regs[EDX_REG] & EDX_PBE) == EDX_PBE);
    }

    regs[ECX_REG] = 0;
    maxLeaves = 0;
    ret = _performCPUID(CPUID_GETEXTBASE_FEATURES, regs);
    if(ret == 1)
    {
        maxLeaves = regs[EAX_REG];
        pCpuInf->flags.fsgsbase =
            ((regs[EBX_REG] & EBX_FSGSBASE) == EBX_FSGSBASE);
        pCpuInf->flags.tsc_adjust =
            ((regs[EBX_REG] & EBX_TSC_ADJUST) == EBX_TSC_ADJUST);
        pCpuInf->flags.sgx = ((regs[EBX_REG] & EBX_SGX) == EBX_SGX);
        pCpuInf->flags.bmi1 = ((regs[EBX_REG] & EBX_BMI1) == EBX_BMI1);
        pCpuInf->flags.hle = ((regs[EBX_REG] & EBX_HLE) == EBX_HLE);
        pCpuInf->flags.avx2 = ((regs[EBX_REG] & EBX_AVX2) == EBX_AVX2);
        pCpuInf->flags.fdp_excptn_only =
            ((regs[EBX_REG] & EBX_FDP_EXCPTN_ONLY) == EBX_FDP_EXCPTN_ONLY);
        pCpuInf->flags.smep = ((regs[EBX_REG] & EBX_SMEP) == EBX_SMEP);
        pCpuInf->flags.bmi2 = ((regs[EBX_REG] & EBX_BMI2) == EBX_BMI2);
        pCpuInf->flags.erms = ((regs[EBX_REG] & EBX_ERMS) == EBX_ERMS);
        pCpuInf->flags.invpcid = ((regs[EBX_REG] & EBX_INVPCID) == EBX_INVPCID);
        pCpuInf->flags.rtm = ((regs[EBX_REG] & EBX_RTM) == EBX_RTM);
        pCpuInf->flags.pqm = ((regs[EBX_REG] & EBX_PQM) == EBX_PQM);
        pCpuInf->flags.zero_fcs_fds =
            ((regs[EBX_REG] & EBX_ZERO_FCS_FDS) == EBX_ZERO_FCS_FDS);
        pCpuInf->flags.mpx = ((regs[EBX_REG] & EBX_MPX) == EBX_MPX);
        pCpuInf->flags.rdt_a = ((regs[EBX_REG] & EBX_RDT_A) == EBX_RDT_A);
        pCpuInf->flags.avx512f = ((regs[EBX_REG] & EBX_AVX512F) == EBX_AVX512F);
        pCpuInf->flags.avx512dq =
            ((regs[EBX_REG] & EBX_AVX512DQ) == EBX_AVX512DQ);
        pCpuInf->flags.rdseed = ((regs[EBX_REG] & EBX_RDSEED) == EBX_RDSEED);
        pCpuInf->flags.adx = ((regs[EBX_REG] & EBX_ADX) == EBX_ADX);
        pCpuInf->flags.smap = ((regs[EBX_REG] & EBX_SMAP) == EBX_SMAP);
        pCpuInf->flags.avx512ifma =
            ((regs[EBX_REG] & EBX_AVX512IFMA) == EBX_AVX512IFMA);
        pCpuInf->flags.clflushopt =
            ((regs[EBX_REG] & EBX_CLFLUSHOPT) == EBX_CLFLUSHOPT);
        pCpuInf->flags.clwb =
            ((regs[EBX_REG] & EBX_CLWB) == EBX_CLWB);
        pCpuInf->flags.intel_pt =
            ((regs[EBX_REG] & EBX_INTEL_PT) == EBX_INTEL_PT);
        pCpuInf->flags.avx512pf =
            ((regs[EBX_REG] & EBX_AVX512PF) == EBX_AVX512PF);
        pCpuInf->flags.avx512er =
            ((regs[EBX_REG] & EBX_AVX512ER) == EBX_AVX512ER);
        pCpuInf->flags.avx512cd =
            ((regs[EBX_REG] & EBX_AVX512CD) == EBX_AVX512CD);
        pCpuInf->flags.sha = ((regs[EBX_REG] & EBX_SHA) == EBX_SHA);
        pCpuInf->flags.avx512bw =
            ((regs[EBX_REG] & EBX_AVX512BW) == EBX_AVX512BW);
        pCpuInf->flags.avx512vl =
            ((regs[EBX_REG] & EBX_AVX512VL) == EBX_AVX512VL);
        pCpuInf->flags.prefetchwt1 =
            ((regs[ECX_REG] & ECX_PREFETCHWT1) == ECX_PREFETCHWT1);
        pCpuInf->flags.avx512vbmi =
            ((regs[ECX_REG] & ECX_AVX512VBMI) == ECX_AVX512VBMI);
        pCpuInf->flags.umip = ((regs[ECX_REG] & ECX_UMIP) == ECX_UMIP);
        pCpuInf->flags.pku = ((regs[ECX_REG] & ECX_PKU) == ECX_PKU);
        pCpuInf->flags.ospke = ((regs[ECX_REG] & ECX_OSPKE) == ECX_OSPKE);
        pCpuInf->flags.waitpkg = ((regs[ECX_REG] & ECX_WAITPKG) == ECX_WAITPKG);
        pCpuInf->flags.avx512_vbmi2 =
            ((regs[ECX_REG] & ECX_AVX512_VBMI2) == ECX_AVX512_VBMI2);
        pCpuInf->flags.cet_ss = ((regs[ECX_REG] & ECX_CET_SS) == ECX_CET_SS);
        pCpuInf->flags.gfni = ((regs[ECX_REG] & ECX_GFNI) == ECX_GFNI);
        pCpuInf->flags.vaes = ((regs[ECX_REG] & ECX_VAES) == ECX_VAES);
        pCpuInf->flags.vpclmulqdq =
            ((regs[ECX_REG] & ECX_VPCLMULQDQ) == ECX_VPCLMULQDQ);
        pCpuInf->flags.avx512_vnni =
            ((regs[ECX_REG] & ECX_AVX512_VNNI) == ECX_AVX512_VNNI);
        pCpuInf->flags.avx512_bitalg =
            ((regs[ECX_REG] & ECX_AVX512_BITALG) == ECX_AVX512_BITALG);
        pCpuInf->flags.tme = ((regs[ECX_REG] & ECX_TME) == ECX_TME);
        pCpuInf->flags.avx512_vpopcntdq =
            ((regs[ECX_REG] & ECX_AVX512_VPOPCNTDQ) == ECX_AVX512_VPOPCNTDQ);
        pCpuInf->flags.la57 = ((regs[ECX_REG] & ECX_LA57) == ECX_LA57);
        pCpuInf->flags.rdpid = ((regs[ECX_REG] & ECX_RDPID) == ECX_RDPID);
        pCpuInf->flags.key_locker =
            ((regs[ECX_REG] & ECX_KEY_LOCKER) == ECX_KEY_LOCKER);
        pCpuInf->flags.bus_lock_detect =
            ((regs[ECX_REG] & ECX_BUS_LOCK_DETECT) == ECX_BUS_LOCK_DETECT);
        pCpuInf->flags.cldemote =
            ((regs[ECX_REG] & ECX_CLDEMOTE) == ECX_CLDEMOTE);
        pCpuInf->flags.movdiri = ((regs[ECX_REG] & ECX_MOVDIRI) == ECX_MOVDIRI);
        pCpuInf->flags.movdir64b =
            ((regs[ECX_REG] & ECX_MOVDIR64B) == ECX_MOVDIR64B);
        pCpuInf->flags.enqcmd = ((regs[ECX_REG] & ECX_ENQCMD) == ECX_ENQCMD);
        pCpuInf->flags.sgx_lc = ((regs[ECX_REG] & ECX_SGX_LC) == ECX_SGX_LC);
        pCpuInf->flags.pks = ((regs[ECX_REG] & ECX_PKS) == ECX_PKS);
        pCpuInf->flags.sgx_keys =
            ((regs[EDX_REG] & EDX_SGX_KEYS) == EDX_SGX_KEYS);
        pCpuInf->flags.avx512_4vnniw =
            ((regs[EDX_REG] & EDX_AVX512_4VNNIW) == EDX_AVX512_4VNNIW);
        pCpuInf->flags.avx512_4fmaps =
            ((regs[EDX_REG] & EDX_AVX512_4FMAPS) == EDX_AVX512_4FMAPS);
        pCpuInf->flags.fsrm = ((regs[EDX_REG] & EDX_FSRM) == EDX_FSRM);
        pCpuInf->flags.uintr = ((regs[EDX_REG] & EDX_UINTR) == EDX_UINTR);
        pCpuInf->flags.avx512_vp2intersect =
            ((regs[EDX_REG] & EDX_AVX512_VP2INTERSECT) ==
             EDX_AVX512_VP2INTERSECT);
        pCpuInf->flags.srdbs_ctrl =
            ((regs[EDX_REG] & EDX_SRDBS_CTRL) == EDX_SRDBS_CTRL);
        pCpuInf->flags.md_clear =
            ((regs[EDX_REG] & EDX_MD_CLEAR) == EDX_MD_CLEAR);
        pCpuInf->flags.rtm_always_abort =
            ((regs[EDX_REG] & EDX_RTM_ALWAYS_ABORT) == EDX_RTM_ALWAYS_ABORT);
        pCpuInf->flags.tsx_force_abort =
            ((regs[EDX_REG] & EDX_TSX_FORCE_ABORT) == EDX_TSX_FORCE_ABORT);
        pCpuInf->flags.serialize =
            ((regs[EDX_REG] & EDX_SERIALIZE) == EDX_SERIALIZE);
        pCpuInf->flags.hybrid_cpu =
            ((regs[EDX_REG] & EDX_HYBRID_CPU) == EDX_HYBRID_CPU);
        pCpuInf->flags.tsxldtrk =
            ((regs[EDX_REG] & EDX_TSXLDTRK) == EDX_TSXLDTRK);
        pCpuInf->flags.pconfig = ((regs[EDX_REG] & EDX_PCONFIG) == EDX_PCONFIG);
        pCpuInf->flags.arch_lbr =
            ((regs[EDX_REG] & EDX_ARCH_LBR) == EDX_ARCH_LBR);
        pCpuInf->flags.cet_ibt = ((regs[EDX_REG] & EDX_CET_IBT) == EDX_CET_IBT);
        pCpuInf->flags.amx_bf16 =
            ((regs[EDX_REG] & EDX_AMX_BF16) == EDX_AMX_BF16);
        pCpuInf->flags.avx512_fp16 =
            ((regs[EDX_REG] & EDX_AVX512_FP16) == EDX_AVX512_FP16);
        pCpuInf->flags.amx_tile =
            ((regs[EDX_REG] & EDX_AMX_TILE) == EDX_AMX_TILE);
        pCpuInf->flags.amx_int8 =
            ((regs[EDX_REG] & EDX_AMX_INT8) == EDX_AMX_INT8);
        pCpuInf->flags.spec_ctrl =
            ((regs[EDX_REG] & EDX_SPEC_CTRL) == EDX_SPEC_CTRL);
        pCpuInf->flags.intel_stibp =
            ((regs[EDX_REG] & EDX_INTEL_STIBP) == EDX_INTEL_STIBP);
        pCpuInf->flags.flush_l1d =
            ((regs[EDX_REG] & EDX_FLUSH_L1D) == EDX_FLUSH_L1D);
        pCpuInf->flags.arch_capabilities =
            ((regs[EDX_REG] & EDX_ARCH_CAPABILITIES) == EDX_ARCH_CAPABILITIES);
        pCpuInf->flags.core_capabilities =
            ((regs[EDX_REG] & EDX_CORE_CAPABILITIES) == EDX_CORE_CAPABILITIES);
        pCpuInf->flags.spec_ctrl_ssbd =
            ((regs[EDX_REG] & EDX_SPEC_CTRL_SSBD) == EDX_SPEC_CTRL_SSBD);
    }

    regs[ECX_REG] = 1;
    ret = _performCPUID(CPUID_GETEXTBASE_FEATURES, regs);
    if(maxLeaves > 1 && ret == 1)
    {
        pCpuInf->flags.avx_vnni =
            ((regs[EAX_REG] & EAX_AVX_VNNI) == EAX_AVX_VNNI);
        pCpuInf->flags.avx512_bf16 =
            ((regs[EAX_REG] & EAX_AVX512_BF16) == EAX_AVX512_BF16);
        pCpuInf->flags.lass = ((regs[EAX_REG] & EAX_LASS) == EAX_LASS);
        pCpuInf->flags.cmpccxadd =
            ((regs[EAX_REG] & EAX_CMPCCXADD) == EAX_CMPCCXADD);
        pCpuInf->flags.arch_perfmon_ext =
            ((regs[EAX_REG] & EAX_ARCH_PERFMON_EXT) == EAX_ARCH_PERFMON_EXT);
        pCpuInf->flags.fzrm = ((regs[EAX_REG] & EAX_FZRM) == EAX_FZRM);
        pCpuInf->flags.fsrs = ((regs[EAX_REG] & EAX_FSRS) == EAX_FSRS);
        pCpuInf->flags.fsrc = ((regs[EAX_REG] & EAX_FSRC) == EAX_FSRC);
        pCpuInf->flags.fred = ((regs[EAX_REG] & EAX_FRED) == EAX_FRED);
        pCpuInf->flags.lkgs = ((regs[EAX_REG] & EAX_LKGS) == EAX_LKGS);
        pCpuInf->flags.wrmsrns = ((regs[EAX_REG] & EAX_WRMSRNS) == EAX_WRMSRNS);
        pCpuInf->flags.nmi_src = ((regs[EAX_REG] & EAX_NMI_SRC) == EAX_NMI_SRC);
        pCpuInf->flags.amx_fp16 =
            ((regs[EAX_REG] & EAX_AMX_FP16) == EAX_AMX_FP16);
        pCpuInf->flags.hreset  = ((regs[EAX_REG] & EAX_HRESET ) == EAX_HRESET );
        pCpuInf->flags.avx_ifma =
            ((regs[EAX_REG] & EAX_AVX_IFMA) == EAX_AVX_IFMA);
        pCpuInf->flags.lam = ((regs[EAX_REG] & EAX_LAM) == EAX_LAM);
        pCpuInf->flags.rd_wr_msrlist =
            ((regs[EAX_REG] & EAX_RD_WR_MSRLIST) == EAX_RD_WR_MSRLIST);
        pCpuInf->flags.intel_ppin =
            ((regs[EBX_REG] & EBX_INTEL_PPIN) == EBX_INTEL_PPIN);
        pCpuInf->flags.avx_vnni_int8 =
            ((regs[EDX_REG] & EDX_AVX_VNNI_INT8) == EDX_AVX_VNNI_INT8);
        pCpuInf->flags.avx_ne_convert =
            ((regs[EDX_REG] & EDX_AVX_NE_CONVERT) == EDX_AVX_NE_CONVERT);
        pCpuInf->flags.amx_complex =
            ((regs[EDX_REG] & EDX_AMX_COMPLEX) == EDX_AMX_COMPLEX);
        pCpuInf->flags.prefetchit_0_1 =
            ((regs[EDX_REG] & EDX_PREFETCHIT_0_1) == EDX_PREFETCHIT_0_1);
        pCpuInf->flags.cet_sss = ((regs[EDX_REG] & EDX_CET_SSS) == EDX_CET_SSS);
    }

    regs[ECX_REG] = 2;
    ret = _performCPUID(CPUID_GETEXTBASE_FEATURES, regs);
    if(maxLeaves > 2 && ret == 1)
    {
        pCpuInf->flags.intel_psfd =
            ((regs[EDX_REG] & EDX_INTEL_PSFD) == EDX_INTEL_PSFD);
        pCpuInf->flags.ipred_ctrl =
            ((regs[EDX_REG] & EDX_IPRED_CTRL) == EDX_IPRED_CTRL);
        pCpuInf->flags.rrsba_ctrl =
            ((regs[EDX_REG] & EDX_RRSBA_CTRL) == EDX_RRSBA_CTRL);
        pCpuInf->flags.ddp_ctrl =
            ((regs[EDX_REG] & EDX_DDP_CTRL) == EDX_DDP_CTRL);
        pCpuInf->flags.bhi_ctrl =
            ((regs[EDX_REG] & EDX_BHI_CTRL) == EDX_BHI_CTRL);
        pCpuInf->flags.mcdt_no = ((regs[EDX_REG] & EDX_MCDT_NO) == EDX_MCDT_NO);
        pCpuInf->flags.uclock_disable =
            ((regs[EDX_REG] & EDX_UCLOCK_DISABLE) == EDX_UCLOCK_DISABLE);
    }

    ret = _performCPUID(CPUID_GETEXTENTED_FEATURES, regs);
    if(ret == 1)
    {
        pCpuInf->flags.lahf_lm = ((regs[ECX_REG] & ECX_LAHF_LM) == ECX_LAHF_LM);
        pCpuInf->flags.cmp_legacy =
            ((regs[ECX_REG] & ECX_CMP_LEGACY) == ECX_CMP_LEGACY);
        pCpuInf->flags.svm = ((regs[ECX_REG] & ECX_SVM) == ECX_SVM);
        pCpuInf->flags.extapic = ((regs[ECX_REG] & ECX_EXTAPIC) == ECX_EXTAPIC);
        pCpuInf->flags.cr8_legacy =
            ((regs[ECX_REG] & ECX_CR8_LEGACY) == ECX_CR8_LEGACY);
        pCpuInf->flags.lzcnt_abm =
            ((regs[ECX_REG] & ECX_LZCNT_ABM) == ECX_LZCNT_ABM);
        pCpuInf->flags.sse4a = ((regs[ECX_REG] & ECX_SSE4A) == ECX_SSE4A);
        pCpuInf->flags.misaligned_sse =
            ((regs[ECX_REG] & ECX_MISALIGNED_SSE) == ECX_MISALIGNED_SSE);
        pCpuInf->flags.f3dnow_prefetch =
            ((regs[ECX_REG] & ECX_3DNOW_PREFETCH) == ECX_3DNOW_PREFETCH);
        pCpuInf->flags.osvw = ((regs[ECX_REG] & ECX_OSVW) == ECX_OSVW);
        pCpuInf->flags.ibs = ((regs[ECX_REG] & ECX_IBS) == ECX_IBS);
        pCpuInf->flags.xop = ((regs[ECX_REG] & ECX_XOP) == ECX_XOP);
        pCpuInf->flags.skinit = ((regs[ECX_REG] & ECX_SKINIT) == ECX_SKINIT);
        pCpuInf->flags.wdt = ((regs[ECX_REG] & ECX_WDT) == ECX_WDT);
        pCpuInf->flags.lwp = ((regs[ECX_REG] & ECX_LWP) == ECX_LWP);
        pCpuInf->flags.fma4 = ((regs[ECX_REG] & ECX_FMA4) == ECX_FMA4);
        pCpuInf->flags.tce = ((regs[ECX_REG] & ECX_TCE) == ECX_TCE);
        pCpuInf->flags.nodeid_msr =
            ((regs[ECX_REG] & ECX_NODEID_MSR) == ECX_NODEID_MSR);
        pCpuInf->flags.tbm = ((regs[ECX_REG] & ECX_TBM) == ECX_TBM);
        pCpuInf->flags.topoext = ((regs[ECX_REG] & ECX_TOPOEXT) == ECX_TOPOEXT);
        pCpuInf->flags.perfctr_core =
            ((regs[ECX_REG] & ECX_PERFCTR_CORE) == ECX_PERFCTR_CORE);
        pCpuInf->flags.perfctr_nb =
            ((regs[ECX_REG] & ECX_PERFCTR_NB) == ECX_PERFCTR_NB);
        pCpuInf->flags.data_bp_ext =
            ((regs[ECX_REG] & ECX_DATA_BP_EXT) == ECX_DATA_BP_EXT);
        pCpuInf->flags.perf_tsc =
            ((regs[ECX_REG] & ECX_PERF_TSC) == ECX_PERF_TSC);
        pCpuInf->flags.perfctr_llc =
            ((regs[ECX_REG] & ECX_PERFCTR_LLC) == ECX_PERFCTR_LLC);
        pCpuInf->flags.mwaitx = ((regs[ECX_REG] & ECX_MWAITX) == ECX_MWAITX);
        pCpuInf->flags.addr_mask_ext =
            ((regs[ECX_REG] & ECX_ADDR_MASK_EXT) == ECX_ADDR_MASK_EXT);
        pCpuInf->flags.e_fpu = ((regs[EDX_REG] & EDX_E_FPU) == EDX_E_FPU);
        pCpuInf->flags.e_vme = ((regs[EDX_REG] & EDX_E_VME) == EDX_E_VME);
        pCpuInf->flags.e_de = ((regs[EDX_REG] & EDX_E_DE) == EDX_E_DE);
        pCpuInf->flags.e_pse = ((regs[EDX_REG] & EDX_E_PSE) == EDX_E_PSE);
        pCpuInf->flags.e_tsc = ((regs[EDX_REG] & EDX_E_TSC) == EDX_E_TSC);
        pCpuInf->flags.e_msr = ((regs[EDX_REG] & EDX_E_MSR) == EDX_E_MSR);
        pCpuInf->flags.e_pae = ((regs[EDX_REG] & EDX_E_PAE) == EDX_E_PAE);
        pCpuInf->flags.e_mce = ((regs[EDX_REG] & EDX_E_MCE) == EDX_E_MCE);
        pCpuInf->flags.e_cx8 = ((regs[EDX_REG] & EDX_E_CX8) == EDX_E_CX8);
        pCpuInf->flags.e_apic = ((regs[EDX_REG] & EDX_E_APIC) == EDX_E_APIC);
        pCpuInf->flags.syscall = ((regs[EDX_REG] & EDX_SYSCALL) == EDX_SYSCALL);
        pCpuInf->flags.e_mtrr = ((regs[EDX_REG] & EDX_E_MTRR) == EDX_E_MTRR);
        pCpuInf->flags.e_pge = ((regs[EDX_REG] & EDX_E_PGE) == EDX_E_PGE);
        pCpuInf->flags.e_mca = ((regs[EDX_REG] & EDX_E_MCA) == EDX_E_MCA);
        pCpuInf->flags.e_cmov = ((regs[EDX_REG] & EDX_E_CMOV) == EDX_E_CMOV);
        pCpuInf->flags.e_pat = ((regs[EDX_REG] & EDX_E_PAT) == EDX_E_PAT);
        pCpuInf->flags.e_pse36 = ((regs[EDX_REG] & EDX_E_PSE36) == EDX_E_PSE36);
        pCpuInf->flags.obsolete_mp_bit =
            ((regs[EDX_REG] & EDX_OBSOLETE_MP_BIT) == EDX_OBSOLETE_MP_BIT);
        pCpuInf->flags.nx = ((regs[EDX_REG] & EDX_NX) == EDX_NX);
        pCpuInf->flags.mmxext = ((regs[EDX_REG] & EDX_MMXEXT) == EDX_MMXEXT);
        pCpuInf->flags.e_mmx = ((regs[EDX_REG] & EDX_E_MMX) == EDX_E_MMX);
        pCpuInf->flags.e_fxsr = ((regs[EDX_REG] & EDX_E_FXSR) == EDX_E_FXSR);
        pCpuInf->flags.fxsr_opt =
            ((regs[EDX_REG] & EDX_FXSR_OPT) == EDX_FXSR_OPT);
        pCpuInf->flags.page1gb = ((regs[EDX_REG] & EDX_PAGE1GB) == EDX_PAGE1GB);
        pCpuInf->flags.rdtscp = ((regs[EDX_REG] & EDX_RDTSCP) == EDX_RDTSCP);
        pCpuInf->flags.lm = ((regs[EDX_REG] & EDX_LM) == EDX_LM);
        pCpuInf->flags.f3dnowext =
            ((regs[EDX_REG] & EDX_3DNOWEXT) == EDX_3DNOWEXT);
        pCpuInf->flags.f3dnow = ((regs[EDX_REG] & EDX_3DNOW) == EDX_3DNOW);
    }

    ret = _performCPUID(CPUID_GETCAPACITY_EXT_FLAGS, regs);
    if(ret == 1)
    {
        pCpuInf->flags.clzero = ((regs[EBX_REG] & EBX_CLZERO) == EBX_CLZERO);
        pCpuInf->flags.insn_retired_perf =
            ((regs[EBX_REG] & EBX_INSN_RETIRED_PERF) == EBX_INSN_RETIRED_PERF);
        pCpuInf->flags.xsave_err_ptr =
            ((regs[EBX_REG] & EBX_XSAVE_ERR_PTR) == EBX_XSAVE_ERR_PTR);
        pCpuInf->flags.invlpgb = ((regs[EBX_REG] & EBX_INVLPGB) == EBX_INVLPGB);
        pCpuInf->flags.rdpru = ((regs[EBX_REG] & EBX_RDPRU) == EBX_RDPRU);
        pCpuInf->flags.mba = ((regs[EBX_REG] & EBX_MBA) == EBX_MBA);
        pCpuInf->flags.mcommit = ((regs[EBX_REG] & EBX_MCOMMIT) == EBX_MCOMMIT);
        pCpuInf->flags.wbnoinvd =
            ((regs[EBX_REG] & EBX_WBNOINVD) == EBX_WBNOINVD);
        pCpuInf->flags.ibpb = ((regs[EBX_REG] & EBX_IBPB) == EBX_IBPB);
        pCpuInf->flags.wbinvd_int =
            ((regs[EBX_REG] & EBX_WBINVD_INT) == EBX_WBINVD_INT);
        pCpuInf->flags.ibrs = ((regs[EBX_REG] & EBX_IBRS) == EBX_IBRS);
        pCpuInf->flags.stibp = ((regs[EBX_REG] & EBX_STIBP) == EBX_STIBP);
        pCpuInf->flags.ibrs_always_on =
            ((regs[EBX_REG] & EBX_IBRS_ALWAYS_ON) == EBX_IBRS_ALWAYS_ON);
        pCpuInf->flags.stibp_always_on =
            ((regs[EBX_REG] & EBX_STIBP_ALWAYS_ON) == EBX_STIBP_ALWAYS_ON);
        pCpuInf->flags.ibrs_fast =
            ((regs[EBX_REG] & EBX_IBRS_FAST) == EBX_IBRS_FAST);
        pCpuInf->flags.ibrs_same_mode =
            ((regs[EBX_REG] & EBX_IBRS_SAME_MODE) == EBX_IBRS_SAME_MODE);
        pCpuInf->flags.no_efer_lmsle =
            ((regs[EBX_REG] & EBX_NO_EFER_LMSLE) == EBX_NO_EFER_LMSLE);
        pCpuInf->flags.tlb_flush_nested =
            ((regs[EBX_REG] & EBX_TLB_FLUSH_NESTED) == EBX_TLB_FLUSH_NESTED);
        pCpuInf->flags.amd_ppin =
            ((regs[EBX_REG] & EBX_AMD_PPIN) == EBX_AMD_PPIN);
        pCpuInf->flags.amd_ssbd =
            ((regs[EBX_REG] & EBX_AMD_SSBD) == EBX_AMD_SSBD);
        pCpuInf->flags.virt_ssbd =
            ((regs[EBX_REG] & EBX_VIRT_SSBD) == EBX_VIRT_SSBD);
        pCpuInf->flags.amd_ssb_no =
            ((regs[EBX_REG] & EBX_AMD_SSB_NO) == EBX_AMD_SSB_NO);
        pCpuInf->flags.cppc = ((regs[EBX_REG] & EBX_CPPC) == EBX_CPPC);
        pCpuInf->flags.amd_psfd =
            ((regs[EBX_REG] & EBX_AMD_PSFD) == EBX_AMD_PSFD);
        pCpuInf->flags.btc_no = ((regs[EBX_REG] & EBX_BTC_NO) == EBX_BTC_NO);
        pCpuInf->flags.ibpb_ret =
            ((regs[EBX_REG] & EBX_IBPB_RET) == EBX_IBPB_RET);
        pCpuInf->flags.branch_sampling =
            ((regs[EBX_REG] & EBX_BRANCH_SAMPLING) == EBX_BRANCH_SAMPLING);
    }

    ret = _performCPUID(CPUID_GETAMDEXTINFO, regs);
    if(ret == 1)
    {
        pCpuInf->flags.no_nested_data_bp =
            ((regs[EAX_REG] & EAX_NO_NESTED_DATA_BP) == EAX_NO_NESTED_DATA_BP);
        pCpuInf->flags.fsgs_non_serializing =
            ((regs[EAX_REG] & EAX_FSGS_NON_SERIALIZING) ==
            EAX_FSGS_NON_SERIALIZING);
        pCpuInf->flags.lfence_serializing =
            ((regs[EAX_REG] & EAX_LFENCE_SERIALIZING) ==
            EAX_LFENCE_SERIALIZING);
        pCpuInf->flags.smm_page_cfg_lock =
            ((regs[EAX_REG] & EAX_SMM_PAGE_CFG_LOCK) == EAX_SMM_PAGE_CFG_LOCK);
        pCpuInf->flags.null_sel_clr_base =
            ((regs[EAX_REG] & EAX_NULL_SEL_CLR_BASE) == EAX_NULL_SEL_CLR_BASE);
        pCpuInf->flags.upper_addr_ignore =
            ((regs[EAX_REG] & EAX_UPPER_ADDR_IGNORE) == EAX_UPPER_ADDR_IGNORE);
        pCpuInf->flags.auto_ibrs =
            ((regs[EAX_REG] & EAX_AUTO_IBRS) == EAX_AUTO_IBRS);
        pCpuInf->flags.no_smm_ctl_msr =
            ((regs[EAX_REG] & EAX_NO_SMM_CTL_MSR) == EAX_NO_SMM_CTL_MSR);
        pCpuInf->flags.e_fsrs = ((regs[EAX_REG] & EAX_E_FSRS) == EAX_E_FSRS);
        pCpuInf->flags.e_fsrc = ((regs[EAX_REG] & EAX_E_FSRC) == EAX_E_FSRC);
        pCpuInf->flags.prefetch_ctl_msr =
            ((regs[EAX_REG] & EAX_PREFETCH_CTL_MSR) == EAX_PREFETCH_CTL_MSR);
        pCpuInf->flags.opcode_reclaim =
            ((regs[EAX_REG] & EAX_OPCODE_RECLAIM) == EAX_OPCODE_RECLAIM);
        pCpuInf->flags.user_cpuid_disable =
            ((regs[EAX_REG] & EAX_USER_CPUID_DISABLE) ==
            EAX_USER_CPUID_DISABLE);
        pCpuInf->flags.epsf = ((regs[EAX_REG] & EAX_EPSF) == EAX_EPSF);
        pCpuInf->flags.wl_feedback =
            ((regs[EAX_REG] & EAX_WL_FEEDBACK) == EAX_WL_FEEDBACK);
        pCpuInf->flags.eraps = ((regs[EAX_REG] & EAX_ERAPS) == EAX_ERAPS);
        pCpuInf->flags.sbpb = ((regs[EAX_REG] & EAX_SBPB) == EAX_SBPB);
        pCpuInf->flags.ibpb_brtype =
            ((regs[EAX_REG] & EAX_IBPB_BRTYPE) == EAX_IBPB_BRTYPE);
        pCpuInf->flags.srso_no = ((regs[EAX_REG] & EAX_SRSO_NO) == EAX_SRSO_NO);
        pCpuInf->flags.srso_uk_no =
            ((regs[EAX_REG] & EAX_SRSO_UK_NO) == EAX_SRSO_UK_NO);
        pCpuInf->flags.srso_msr_fix =
            ((regs[EAX_REG] & EAX_SRSO_MSR_FIX) == EAX_SRSO_MSR_FIX);
    }
}

static void _getMemoryInformation(cpu_info_t* pCpuInf)
{
    int32_t  ret;
    uint32_t regs[4];

    ret = _performCPUID(CPUID_GETCAPACITY_EXT_FLAGS, regs);
    if(ret == 1)
    {
        pCpuInf->physAddressWidth = regs[EAX_REG] & 0xFF;
        pCpuInf->virtAddressWidth = (regs[EAX_REG] >> 8) & 0xFF;
    }
    else
    {
        pCpuInf->physAddressWidth = 0;
        pCpuInf->virtAddressWidth = 0;
    }
}

static void _getCacheInformationUnified(cpu_info_t*    pCpuInf,
                                        const uint32_t kLeaf)
{
    uint32_t          i;
    int32_t           ret;
    CPU_CACHE_TYPE_E  cacheType;
    cpu_cache_info_t* pNewCacheInfo;
    uint32_t          regs[4];

    /* Main leaf */
    i = 0;
    while(true)
    {
        regs[ECX_REG] = i;
        ret = _performCPUID(kLeaf, regs);
        if(ret == 1)
        {
            cacheType = CPUID_CACHE_TYPE(regs);

            /* Unknown means no more cache */
            if(cacheType == CACHE_UNKNOWN)
            {
                break;
            }

            /* Allocate the structure and fill the information */
            pNewCacheInfo = kmalloc(sizeof(cpu_cache_info_t));
            CPU_ASSERT(pNewCacheInfo != NULL,
                       "Failed to allocate CPU info structure.",
                       OS_ERR_NO_MORE_MEMORY);

            pNewCacheInfo->ways = CPUID_CACHE_WAYS(regs) + 1;
            pNewCacheInfo->parts = CPUID_CACHE_PARTS(regs) + 1;
            pNewCacheInfo->sets = CPUID_CACHE_SETS(regs) + 1;
            pNewCacheInfo->lineSize = CPUID_CACHE_LINESIZE(regs) + 1;
            pNewCacheInfo->level = CPUID_CACHE_LEVEL(regs);
            pNewCacheInfo->size = CPUID_CACHE_SIZE(pNewCacheInfo);
            pNewCacheInfo->type = cacheType;

            /* Link the new node */
            pNewCacheInfo->pNext = pCpuInf->pCaches;
            pCpuInf->pCaches = pNewCacheInfo;
        }
        else
        {
            break;
        }
        ++i;
    }
}

static uint16_t _getCacheAssocAmd(const uint32_t kBase)
{
    switch(kBase)
    {
        case 0:
            return 0;
        case 1:
            return 1;
        case 2:
            return 2;
        case 3:
            return 3;
        case 4:
            return 4;
        case 5:
            return 6;
        case 6:
            return 8;
        case 8:
            return 16;
        case 9:
            return 0;
        case 10:
            return 32;
        case 11:
            return 48;
        case 12:
            return 64;
        case 13:
            return 96;
        case 14:
            return 128;
        case 15:
            return 1;
        default:
            return 0;
    }
}

static void _getCacheInformationAmd(cpu_info_t* pCpuInf)
{
    int32_t           ret;
    cpu_cache_info_t* pNewCacheInfo;
    uint32_t          regs[4];

    /* Get the L1 info */
    ret = _performCPUID(CPUID_GETL1CACHETLBINFO_AMD, regs);
    if(ret == 1)
    {
        /* Allocate the structure and fill the information */
        pNewCacheInfo = kmalloc(sizeof(cpu_cache_info_t));
        CPU_ASSERT(pNewCacheInfo != NULL,
                    "Failed to allocate CPU info structure.",
                    OS_ERR_NO_MORE_MEMORY);

        pNewCacheInfo->ways = CPUID_AMD_ICACHE_WAYS(regs);
        pNewCacheInfo->parts = CPUID_AMD_ICACHE_PARTS(regs);
        pNewCacheInfo->sets = CPUID_AMD_ICACHE_SETS(regs);
        pNewCacheInfo->lineSize = CPUID_AMD_ICACHE_LINESIZE(regs);
        pNewCacheInfo->level = 0;
        pNewCacheInfo->size = CPUID_CACHE_SIZE(pNewCacheInfo);
        pNewCacheInfo->type = CACHE_INSTRUCTION;

        /* Link the new node */
        pNewCacheInfo->pNext = pCpuInf->pCaches;
        pCpuInf->pCaches = pNewCacheInfo;

        /* Allocate the structure and fill the information */
        pNewCacheInfo = kmalloc(sizeof(cpu_cache_info_t));
        CPU_ASSERT(pNewCacheInfo != NULL,
                    "Failed to allocate CPU info structure.",
                    OS_ERR_NO_MORE_MEMORY);

        pNewCacheInfo->ways = CPUID_AMD_DCACHE_WAYS(regs);
        pNewCacheInfo->parts = CPUID_AMD_DCACHE_PARTS(regs);
        pNewCacheInfo->sets = CPUID_AMD_DCACHE_SETS(regs);
        pNewCacheInfo->lineSize = CPUID_AMD_DCACHE_LINESIZE(regs);
        pNewCacheInfo->level = 0;
        pNewCacheInfo->size = CPUID_CACHE_SIZE(pNewCacheInfo);
        pNewCacheInfo->type = CACHE_DATA;

        /* Link the new node */
        pNewCacheInfo->pNext = pCpuInf->pCaches;
        pCpuInf->pCaches = pNewCacheInfo;
    }

    /* Get the L2 and L3 info */
    ret = _performCPUID(CPUID_GETL2CACHETLBINFO_AMD, regs);
    if(ret == 1)
    {
        /* Allocate the structure and fill the information */
        pNewCacheInfo = kmalloc(sizeof(cpu_cache_info_t));
        CPU_ASSERT(pNewCacheInfo != NULL,
                    "Failed to allocate CPU info structure.",
                    OS_ERR_NO_MORE_MEMORY);

        pNewCacheInfo->ways = _getCacheAssocAmd(CPUID_AMD_L2CACHE_WAYS(regs));
        pNewCacheInfo->parts = CPUID_AMD_L2CACHE_PARTS(regs);
        pNewCacheInfo->sets = CPUID_AMD_L2CACHE_SETS(regs);
        pNewCacheInfo->lineSize = CPUID_AMD_L2CACHE_LINESIZE(regs);
        pNewCacheInfo->level = 1;
        pNewCacheInfo->size = CPUID_CACHE_SIZE(pNewCacheInfo);
        pNewCacheInfo->type = CACHE_UNIFIED;

        /* Link the new node */
        pNewCacheInfo->pNext = pCpuInf->pCaches;
        pCpuInf->pCaches = pNewCacheInfo;

        /* Allocate the structure and fill the information */
        pNewCacheInfo = kmalloc(sizeof(cpu_cache_info_t));
        CPU_ASSERT(pNewCacheInfo != NULL,
                    "Failed to allocate CPU info structure.",
                    OS_ERR_NO_MORE_MEMORY);

        pNewCacheInfo->ways = _getCacheAssocAmd(CPUID_AMD_L3CACHE_WAYS(regs));
        pNewCacheInfo->parts = CPUID_AMD_L3CACHE_PARTS(regs);
        pNewCacheInfo->sets = CPUID_AMD_L3CACHE_SETS(regs);
        pNewCacheInfo->lineSize = CPUID_AMD_L3CACHE_LINESIZE(regs);
        pNewCacheInfo->level = 2;
        pNewCacheInfo->size = CPUID_CACHE_SIZE(pNewCacheInfo);
        pNewCacheInfo->type = CACHE_UNIFIED;

        /* Link the new node */
        pNewCacheInfo->pNext = pCpuInf->pCaches;
        pCpuInf->pCaches = pNewCacheInfo;
    }
}

static void _getCacheInformation(cpu_info_t* pCpuInf)
{
    /* Get cache information */
    if(pCpuInf->type == CPU_INTEL)
    {
        _getCacheInformationUnified(pCpuInf, CPUID_GETCACHEINFO_INTEL);
    }
    else if(pCpuInf->type == CPU_AMD)
    {
        if(pCpuInf->cpuIdLevel >= CPUID_GETCACHEINFO_AMD &&
           pCpuInf->flags.topoext == true)
        {
            _getCacheInformationUnified(pCpuInf, CPUID_GETCACHEINFO_AMD);
        }
        else
        {
            /* Fallback L1 cache info AMD */
            _getCacheInformationAmd(pCpuInf);
        }
    }
}

static void _getTLBInformation(cpu_info_t* pCpuInf)
{
    #if 0
    if(pCpuInf->type == CPU_INTEL)
    {
        _getIntelTLBInformation(pCpuInf);
    }
    else if(pCpuInf->type == CPU_AMD)
    {
        _getAMDTLBInformation(pCpuInf);
    }
    #else
    (void)pCpuInf;
    #endif
}

void cpuidAnalyzeCPU(cpu_info_t* pCpuInfo)
{
    /* Get the vendor information */
    _getVendorInfo(pCpuInfo);
    _getCPUName(pCpuInfo);

    /* Get general features */
    _getGeneralInformation(pCpuInfo);
    _getGeneralFeatures(pCpuInfo);

    /* Get the memory, TLB and cache information */
    _getMemoryInformation(pCpuInfo);
    _getCacheInformation(pCpuInfo);
    _getTLBInformation(pCpuInfo);

    /* Fill in direct information */
    pCpuInfo->fpu = pCpuInfo->flags.fpu;

    /* TODO */
    pCpuInfo->microcode = 0;
    pCpuInfo->frequencyHz = 0;
    pCpuInfo->physicalId = 0;
    pCpuInfo->siblings = 0;
    pCpuInfo->coreId = 0;
    pCpuInfo->cpuCores = 0;
    pCpuInfo->apicId = 0;
    pCpuInfo->bogoMips = 0;

}

size_t cpuIdGetFlagsString(char*                   pBuffer,
                           size_t                  length,
                           const cpu_flags_info_t* kpInfo)
{
    size_t mainOffset;
    size_t offset;
    mainOffset = 0;
    PRINT_FLAG_INFO(kpInfo->sse3, "sse3");
    PRINT_FLAG_INFO(kpInfo->pclmulqdq, "pclmulqdq");
    PRINT_FLAG_INFO(kpInfo->dtes64, "dtes64");
    PRINT_FLAG_INFO(kpInfo->monitor, "monitor");
    PRINT_FLAG_INFO(kpInfo->dscpl, "dscpl");
    PRINT_FLAG_INFO(kpInfo->vmx, "vmx");
    PRINT_FLAG_INFO(kpInfo->smx, "smx");
    PRINT_FLAG_INFO(kpInfo->est, "est");
    PRINT_FLAG_INFO(kpInfo->tm2, "tm2");
    PRINT_FLAG_INFO(kpInfo->ssse3, "ssse3");
    PRINT_FLAG_INFO(kpInfo->cntxt_id, "cntxt_id");
    PRINT_FLAG_INFO(kpInfo->sdbg, "sdbg");
    PRINT_FLAG_INFO(kpInfo->fma, "fma");
    PRINT_FLAG_INFO(kpInfo->cx16, "cx16");
    PRINT_FLAG_INFO(kpInfo->xtpr_update, "xtpr_update");
    PRINT_FLAG_INFO(kpInfo->pdcm, "pdcm");
    PRINT_FLAG_INFO(kpInfo->pcid, "pcid");
    PRINT_FLAG_INFO(kpInfo->dca, "dca");
    PRINT_FLAG_INFO(kpInfo->sse4_1, "sse4_1");
    PRINT_FLAG_INFO(kpInfo->sse4_2, "sse4_2");
    PRINT_FLAG_INFO(kpInfo->x2apic, "x2apic");
    PRINT_FLAG_INFO(kpInfo->movbe, "movbe");
    PRINT_FLAG_INFO(kpInfo->popcnt, "popcnt");
    PRINT_FLAG_INFO(kpInfo->tsc_deadline_timer, "tsc_deadline_timer");
    PRINT_FLAG_INFO(kpInfo->aes, "aes");
    PRINT_FLAG_INFO(kpInfo->xsave, "xsave");
    PRINT_FLAG_INFO(kpInfo->osxsave, "osxsave");
    PRINT_FLAG_INFO(kpInfo->avx, "avx");
    PRINT_FLAG_INFO(kpInfo->f16c, "f16c");
    PRINT_FLAG_INFO(kpInfo->rdrand, "rdrand");
    PRINT_FLAG_INFO(kpInfo->guest_status, "guest_status");
    PRINT_FLAG_INFO(kpInfo->fpu, "fpu");
    PRINT_FLAG_INFO(kpInfo->vme, "vme");
    PRINT_FLAG_INFO(kpInfo->de, "de");
    PRINT_FLAG_INFO(kpInfo->pse, "pse");
    PRINT_FLAG_INFO(kpInfo->tsc, "tsc");
    PRINT_FLAG_INFO(kpInfo->msr, "msr");
    PRINT_FLAG_INFO(kpInfo->pae, "pae");
    PRINT_FLAG_INFO(kpInfo->mce, "mce");
    PRINT_FLAG_INFO(kpInfo->cx8, "cx8");
    PRINT_FLAG_INFO(kpInfo->apic, "apic");
    PRINT_FLAG_INFO(kpInfo->sep, "sep");
    PRINT_FLAG_INFO(kpInfo->mtrr, "mtrr");
    PRINT_FLAG_INFO(kpInfo->pge, "pge");
    PRINT_FLAG_INFO(kpInfo->mca, "mca");
    PRINT_FLAG_INFO(kpInfo->cmov, "cmov");
    PRINT_FLAG_INFO(kpInfo->pat, "pat");
    PRINT_FLAG_INFO(kpInfo->pse36, "pse36");
    PRINT_FLAG_INFO(kpInfo->psn, "psn");
    PRINT_FLAG_INFO(kpInfo->clflush, "clflush");
    PRINT_FLAG_INFO(kpInfo->ds, "ds");
    PRINT_FLAG_INFO(kpInfo->acpi, "acpi");
    PRINT_FLAG_INFO(kpInfo->mmx, "mmx");
    PRINT_FLAG_INFO(kpInfo->fxsr, "fxsr");
    PRINT_FLAG_INFO(kpInfo->sse, "sse");
    PRINT_FLAG_INFO(kpInfo->sse2, "sse2");
    PRINT_FLAG_INFO(kpInfo->selfsnoop, "selfsnoop");
    PRINT_FLAG_INFO(kpInfo->htt, "htt");
    PRINT_FLAG_INFO(kpInfo->tm, "tm");
    PRINT_FLAG_INFO(kpInfo->ia64, "ia64");
    PRINT_FLAG_INFO(kpInfo->pbe, "pbe");
    PRINT_FLAG_INFO(kpInfo->fsgsbase, "fsgsbase");
    PRINT_FLAG_INFO(kpInfo->tsc_adjust, "tsc_adjust");
    PRINT_FLAG_INFO(kpInfo->sgx, "sgx");
    PRINT_FLAG_INFO(kpInfo->bmi1, "bmi1");
    PRINT_FLAG_INFO(kpInfo->hle, "hle");
    PRINT_FLAG_INFO(kpInfo->avx2, "avx2");
    PRINT_FLAG_INFO(kpInfo->fdp_excptn_only, "fdp_excptn_only");
    PRINT_FLAG_INFO(kpInfo->smep, "smep");
    PRINT_FLAG_INFO(kpInfo->bmi2, "bmi2");
    PRINT_FLAG_INFO(kpInfo->erms, "erms");
    PRINT_FLAG_INFO(kpInfo->invpcid, "invpcid");
    PRINT_FLAG_INFO(kpInfo->rtm, "rtm");
    PRINT_FLAG_INFO(kpInfo->pqm, "pqm");
    PRINT_FLAG_INFO(kpInfo->zero_fcs_fds, "zero_fcs_fds");
    PRINT_FLAG_INFO(kpInfo->mpx, "mpx");
    PRINT_FLAG_INFO(kpInfo->rdt_a, "rdt_a");
    PRINT_FLAG_INFO(kpInfo->avx512f, "avx512f");
    PRINT_FLAG_INFO(kpInfo->avx512dq, "avx512dq");
    PRINT_FLAG_INFO(kpInfo->rdseed, "rdseed");
    PRINT_FLAG_INFO(kpInfo->adx, "adx");
    PRINT_FLAG_INFO(kpInfo->smap, "smap");
    PRINT_FLAG_INFO(kpInfo->avx512ifma, "avx512ifma");
    PRINT_FLAG_INFO(kpInfo->clflushopt, "clflushopt");
    PRINT_FLAG_INFO(kpInfo->clwb, "clwb");
    PRINT_FLAG_INFO(kpInfo->intel_pt, "intel_pt");
    PRINT_FLAG_INFO(kpInfo->avx512pf, "avx512pf");
    PRINT_FLAG_INFO(kpInfo->avx512er, "avx512er");
    PRINT_FLAG_INFO(kpInfo->avx512cd, "avx512cd");
    PRINT_FLAG_INFO(kpInfo->sha, "sha");
    PRINT_FLAG_INFO(kpInfo->avx512bw, "avx512bw");
    PRINT_FLAG_INFO(kpInfo->avx512vl, "avx512vl");
    PRINT_FLAG_INFO(kpInfo->prefetchwt1, "prefetchwt1");
    PRINT_FLAG_INFO(kpInfo->avx512vbmi, "avx512vbmi");
    PRINT_FLAG_INFO(kpInfo->umip, "umip");
    PRINT_FLAG_INFO(kpInfo->pku, "pku");
    PRINT_FLAG_INFO(kpInfo->ospke, "ospke");
    PRINT_FLAG_INFO(kpInfo->waitpkg, "waitpkg");
    PRINT_FLAG_INFO(kpInfo->avx512_vbmi2, "avx512_vbmi2");
    PRINT_FLAG_INFO(kpInfo->cet_ss, "cet_ss");
    PRINT_FLAG_INFO(kpInfo->gfni, "gfni");
    PRINT_FLAG_INFO(kpInfo->vaes, "vaes");
    PRINT_FLAG_INFO(kpInfo->vpclmulqdq, "vpclmulqdq");
    PRINT_FLAG_INFO(kpInfo->avx512_vnni, "avx512_vnni");
    PRINT_FLAG_INFO(kpInfo->avx512_bitalg, "avx512_bitalg");
    PRINT_FLAG_INFO(kpInfo->tme, "tme");
    PRINT_FLAG_INFO(kpInfo->avx512_vpopcntdq, "avx512_vpopcntdq");
    PRINT_FLAG_INFO(kpInfo->la57, "la57");
    PRINT_FLAG_INFO(kpInfo->rdpid, "rdpid");
    PRINT_FLAG_INFO(kpInfo->key_locker, "key_locker");
    PRINT_FLAG_INFO(kpInfo->bus_lock_detect, "bus_lock_detect");
    PRINT_FLAG_INFO(kpInfo->cldemote, "cldemote");
    PRINT_FLAG_INFO(kpInfo->movdiri, "movdiri");
    PRINT_FLAG_INFO(kpInfo->movdir64b, "movdir64b");
    PRINT_FLAG_INFO(kpInfo->enqcmd, "enqcmd");
    PRINT_FLAG_INFO(kpInfo->sgx_lc, "sgx_lc");
    PRINT_FLAG_INFO(kpInfo->pks, "pks");
    PRINT_FLAG_INFO(kpInfo->sgx_keys, "sgx_keys");
    PRINT_FLAG_INFO(kpInfo->avx512_4vnniw, "avx512_4vnniw");
    PRINT_FLAG_INFO(kpInfo->avx512_4fmaps, "avx512_4fmaps");
    PRINT_FLAG_INFO(kpInfo->fsrm, "fsrm");
    PRINT_FLAG_INFO(kpInfo->uintr, "uintr");
    PRINT_FLAG_INFO(kpInfo->avx512_vp2intersect, "avx512_vp2intersect");
    PRINT_FLAG_INFO(kpInfo->srdbs_ctrl, "srdbs_ctrl");
    PRINT_FLAG_INFO(kpInfo->md_clear, "md_clear");
    PRINT_FLAG_INFO(kpInfo->rtm_always_abort, "rtm_always_abort");
    PRINT_FLAG_INFO(kpInfo->tsx_force_abort, "tsx_force_abort");
    PRINT_FLAG_INFO(kpInfo->serialize, "serialize");
    PRINT_FLAG_INFO(kpInfo->hybrid_cpu, "hybrid_cpu");
    PRINT_FLAG_INFO(kpInfo->tsxldtrk, "tsxldtrk");
    PRINT_FLAG_INFO(kpInfo->pconfig, "pconfig");
    PRINT_FLAG_INFO(kpInfo->arch_lbr, "arch_lbr");
    PRINT_FLAG_INFO(kpInfo->cet_ibt, "cet_ibt");
    PRINT_FLAG_INFO(kpInfo->amx_bf16, "amx_bf16");
    PRINT_FLAG_INFO(kpInfo->avx512_fp16, "avx512_fp16");
    PRINT_FLAG_INFO(kpInfo->amx_tile, "amx_tile");
    PRINT_FLAG_INFO(kpInfo->amx_int8, "amx_int8");
    PRINT_FLAG_INFO(kpInfo->spec_ctrl, "spec_ctrl");
    PRINT_FLAG_INFO(kpInfo->intel_stibp, "intel_stibp");
    PRINT_FLAG_INFO(kpInfo->flush_l1d, "flush_l1d");
    PRINT_FLAG_INFO(kpInfo->arch_capabilities, "arch_capabilities");
    PRINT_FLAG_INFO(kpInfo->core_capabilities, "core_capabilities");
    PRINT_FLAG_INFO(kpInfo->spec_ctrl_ssbd, "spec_ctrl_ssbd");
    PRINT_FLAG_INFO(kpInfo->avx_vnni, "avx_vnni");
    PRINT_FLAG_INFO(kpInfo->avx512_bf16, "avx512_bf16");
    PRINT_FLAG_INFO(kpInfo->lass, "lass");
    PRINT_FLAG_INFO(kpInfo->cmpccxadd, "cmpccxadd");
    PRINT_FLAG_INFO(kpInfo->arch_perfmon_ext, "arch_perfmon_ext");
    PRINT_FLAG_INFO(kpInfo->fzrm, "fzrm");
    PRINT_FLAG_INFO(kpInfo->fsrs, "fsrs");
    PRINT_FLAG_INFO(kpInfo->fsrc, "fsrc");
    PRINT_FLAG_INFO(kpInfo->fred, "fred");
    PRINT_FLAG_INFO(kpInfo->lkgs, "lkgs");
    PRINT_FLAG_INFO(kpInfo->wrmsrns, "wrmsrns");
    PRINT_FLAG_INFO(kpInfo->nmi_src, "nmi_src");
    PRINT_FLAG_INFO(kpInfo->amx_fp16, "amx_fp16");
    PRINT_FLAG_INFO(kpInfo->hreset , "hreset ");
    PRINT_FLAG_INFO(kpInfo->avx_ifma, "avx_ifma");
    PRINT_FLAG_INFO(kpInfo->lam, "lam");
    PRINT_FLAG_INFO(kpInfo->rd_wr_msrlist, "rd_wr_msrlist");
    PRINT_FLAG_INFO(kpInfo->intel_ppin, "intel_ppin");
    PRINT_FLAG_INFO(kpInfo->avx_vnni_int8, "avx_vnni_int8");
    PRINT_FLAG_INFO(kpInfo->avx_ne_convert, "avx_ne_convert");
    PRINT_FLAG_INFO(kpInfo->amx_complex, "amx_complex");
    PRINT_FLAG_INFO(kpInfo->prefetchit_0_1, "prefetchit_0_1");
    PRINT_FLAG_INFO(kpInfo->cet_sss, "cet_sss");
    PRINT_FLAG_INFO(kpInfo->intel_psfd, "intel_psfd");
    PRINT_FLAG_INFO(kpInfo->ipred_ctrl, "ipred_ctrl");
    PRINT_FLAG_INFO(kpInfo->rrsba_ctrl, "rrsba_ctrl");
    PRINT_FLAG_INFO(kpInfo->ddp_ctrl, "ddp_ctrl");
    PRINT_FLAG_INFO(kpInfo->bhi_ctrl, "bhi_ctrl");
    PRINT_FLAG_INFO(kpInfo->mcdt_no, "mcdt_no");
    PRINT_FLAG_INFO(kpInfo->uclock_disable, "uclock_disable");
    PRINT_FLAG_INFO(kpInfo->lahf_lm, "lahf_lm");
    PRINT_FLAG_INFO(kpInfo->cmp_legacy, "cmp_legacy");
    PRINT_FLAG_INFO(kpInfo->svm, "svm");
    PRINT_FLAG_INFO(kpInfo->extapic, "extapic");
    PRINT_FLAG_INFO(kpInfo->cr8_legacy, "cr8_legacy");
    PRINT_FLAG_INFO(kpInfo->lzcnt_abm, "lzcnt_abm");
    PRINT_FLAG_INFO(kpInfo->sse4a, "sse4a");
    PRINT_FLAG_INFO(kpInfo->misaligned_sse, "misaligned_sse");
    PRINT_FLAG_INFO(kpInfo->f3dnow_prefetch, "f3dnow_prefetch");
    PRINT_FLAG_INFO(kpInfo->osvw, "osvw");
    PRINT_FLAG_INFO(kpInfo->ibs, "ibs");
    PRINT_FLAG_INFO(kpInfo->xop, "xop");
    PRINT_FLAG_INFO(kpInfo->skinit, "skinit");
    PRINT_FLAG_INFO(kpInfo->wdt, "wdt");
    PRINT_FLAG_INFO(kpInfo->lwp, "lwp");
    PRINT_FLAG_INFO(kpInfo->fma4, "fma4");
    PRINT_FLAG_INFO(kpInfo->tce, "tce");
    PRINT_FLAG_INFO(kpInfo->nodeid_msr, "nodeid_msr");
    PRINT_FLAG_INFO(kpInfo->tbm, "tbm");
    PRINT_FLAG_INFO(kpInfo->topoext, "topoext");
    PRINT_FLAG_INFO(kpInfo->perfctr_core, "perfctr_core");
    PRINT_FLAG_INFO(kpInfo->perfctr_nb, "perfctr_nb");
    PRINT_FLAG_INFO(kpInfo->data_bp_ext, "data_bp_ext");
    PRINT_FLAG_INFO(kpInfo->perf_tsc, "perf_tsc");
    PRINT_FLAG_INFO(kpInfo->perfctr_llc, "perfctr_llc");
    PRINT_FLAG_INFO(kpInfo->mwaitx, "mwaitx");
    PRINT_FLAG_INFO(kpInfo->addr_mask_ext, "addr_mask_ext");
    PRINT_FLAG_INFO(kpInfo->e_fpu, "e_fpu");
    PRINT_FLAG_INFO(kpInfo->e_vme, "e_vme");
    PRINT_FLAG_INFO(kpInfo->e_de, "e_de");
    PRINT_FLAG_INFO(kpInfo->e_pse, "e_pse");
    PRINT_FLAG_INFO(kpInfo->e_tsc, "e_tsc");
    PRINT_FLAG_INFO(kpInfo->e_msr, "e_msr");
    PRINT_FLAG_INFO(kpInfo->e_pae, "e_pae");
    PRINT_FLAG_INFO(kpInfo->e_mce, "e_mce");
    PRINT_FLAG_INFO(kpInfo->e_cx8, "e_cx8");
    PRINT_FLAG_INFO(kpInfo->e_apic, "e_apic");
    PRINT_FLAG_INFO(kpInfo->syscall, "syscall");
    PRINT_FLAG_INFO(kpInfo->e_mtrr, "e_mtrr");
    PRINT_FLAG_INFO(kpInfo->e_pge, "e_pge");
    PRINT_FLAG_INFO(kpInfo->e_mca, "e_mca");
    PRINT_FLAG_INFO(kpInfo->e_cmov, "e_cmov");
    PRINT_FLAG_INFO(kpInfo->e_pat, "e_pat");
    PRINT_FLAG_INFO(kpInfo->e_pse36, "e_pse36");
    PRINT_FLAG_INFO(kpInfo->obsolete_mp_bit, "obsolete_mp_bit");
    PRINT_FLAG_INFO(kpInfo->nx, "nx");
    PRINT_FLAG_INFO(kpInfo->mmxext, "mmxext");
    PRINT_FLAG_INFO(kpInfo->e_mmx, "e_mmx");
    PRINT_FLAG_INFO(kpInfo->e_fxsr, "e_fxsr");
    PRINT_FLAG_INFO(kpInfo->fxsr_opt, "fxsr_opt");
    PRINT_FLAG_INFO(kpInfo->page1gb, "page1gb");
    PRINT_FLAG_INFO(kpInfo->rdtscp, "rdtscp");
    PRINT_FLAG_INFO(kpInfo->lm, "lm");
    PRINT_FLAG_INFO(kpInfo->f3dnowext, "f3dnowext");
    PRINT_FLAG_INFO(kpInfo->f3dnow, "f3dnow");
    PRINT_FLAG_INFO(kpInfo->clzero, "clzero");
    PRINT_FLAG_INFO(kpInfo->insn_retired_perf, "insn_retired_perf");
    PRINT_FLAG_INFO(kpInfo->xsave_err_ptr, "xsave_err_ptr");
    PRINT_FLAG_INFO(kpInfo->invlpgb, "invlpgb");
    PRINT_FLAG_INFO(kpInfo->rdpru, "rdpru");
    PRINT_FLAG_INFO(kpInfo->mba, "mba");
    PRINT_FLAG_INFO(kpInfo->mcommit, "mcommit");
    PRINT_FLAG_INFO(kpInfo->wbnoinvd, "wbnoinvd");
    PRINT_FLAG_INFO(kpInfo->ibpb, "ibpb");
    PRINT_FLAG_INFO(kpInfo->wbinvd_int, "wbinvd_int");
    PRINT_FLAG_INFO(kpInfo->ibrs, "ibrs");
    PRINT_FLAG_INFO(kpInfo->stibp, "stibp");
    PRINT_FLAG_INFO(kpInfo->ibrs_always_on, "ibrs_always_on");
    PRINT_FLAG_INFO(kpInfo->stibp_always_on, "stibp_always_on");
    PRINT_FLAG_INFO(kpInfo->ibrs_fast, "ibrs_fast");
    PRINT_FLAG_INFO(kpInfo->ibrs_same_mode, "ibrs_same_mode");
    PRINT_FLAG_INFO(kpInfo->no_efer_lmsle, "no_efer_lmsle");
    PRINT_FLAG_INFO(kpInfo->tlb_flush_nested, "tlb_flush_nested");
    PRINT_FLAG_INFO(kpInfo->amd_ppin, "amd_ppin");
    PRINT_FLAG_INFO(kpInfo->amd_ssbd, "amd_ssbd");
    PRINT_FLAG_INFO(kpInfo->virt_ssbd, "virt_ssbd");
    PRINT_FLAG_INFO(kpInfo->amd_ssb_no, "amd_ssb_no");
    PRINT_FLAG_INFO(kpInfo->cppc, "cppc");
    PRINT_FLAG_INFO(kpInfo->amd_psfd, "amd_psfd");
    PRINT_FLAG_INFO(kpInfo->btc_no, "btc_no");
    PRINT_FLAG_INFO(kpInfo->ibpb_ret, "ibpb_ret");
    PRINT_FLAG_INFO(kpInfo->branch_sampling, "branch_sampling");
    PRINT_FLAG_INFO(kpInfo->no_nested_data_bp, "no_nested_data_bp");
    PRINT_FLAG_INFO(kpInfo->fsgs_non_serializing, "fsgs_non_serializing");
    PRINT_FLAG_INFO(kpInfo->lfence_serializing, "lfence_serializing");
    PRINT_FLAG_INFO(kpInfo->smm_page_cfg_lock, "smm_page_cfg_lock");
    PRINT_FLAG_INFO(kpInfo->null_sel_clr_base, "null_sel_clr_base");
    PRINT_FLAG_INFO(kpInfo->upper_addr_ignore, "upper_addr_ignore");
    PRINT_FLAG_INFO(kpInfo->auto_ibrs, "auto_ibrs");
    PRINT_FLAG_INFO(kpInfo->no_smm_ctl_msr, "no_smm_ctl_msr");
    PRINT_FLAG_INFO(kpInfo->e_fsrs, "e_fsrs");
    PRINT_FLAG_INFO(kpInfo->e_fsrc, "e_fsrc");
    PRINT_FLAG_INFO(kpInfo->prefetch_ctl_msr, "prefetch_ctl_msr");
    PRINT_FLAG_INFO(kpInfo->opcode_reclaim, "opcode_reclaim");
    PRINT_FLAG_INFO(kpInfo->user_cpuid_disable, "user_cpuid_disable");
    PRINT_FLAG_INFO(kpInfo->epsf, "epsf");
    PRINT_FLAG_INFO(kpInfo->wl_feedback, "wl_feedback");
    PRINT_FLAG_INFO(kpInfo->eraps, "eraps");
    PRINT_FLAG_INFO(kpInfo->sbpb, "sbpb");
    PRINT_FLAG_INFO(kpInfo->ibpb_brtype, "ibpb_brtype");
    PRINT_FLAG_INFO(kpInfo->srso_no, "srso_no");
    PRINT_FLAG_INFO(kpInfo->srso_uk_no, "srso_uk_no");
    PRINT_FLAG_INFO(kpInfo->srso_msr_fix, "srso_msr_fix");
    return mainOffset;
}
/************************************ EOF *************************************/