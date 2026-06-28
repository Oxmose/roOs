/*******************************************************************************
 * @file cpuid.h
 *
 * @see cpuid.c
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

#ifndef __X8664_CPUID_H_
#define __X8664_CPUID_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdint.h>  /* Standard integer definitions */
#include <stddef.h>  /* Standard definitions */
#include <stdbool.h> /* Standard bool */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the vendor string size */
#define CPU_VENDOR_STR_SIZE 12
/** @brief Defines the cpu name string size */
#define CPU_NAME_SIZE 48
/** @brief Defines the addressing string size */
#define CPU_ADDRESSING_SIZE 32

/** @brief CPUID Vendor String for Intel processors  */
#define CPUID_VENDOR_INTEL "GenuineIntel"
/** @brief CPUID Vendor String for AMD processors  */
#define CPUID_VENDOR_AMD "AuthenticAMD"
/** @brief CPUID Vendor String for K5 AMD processors  */
#define CPUID_VENDOR_AMD_K5 "AMDisbetter!"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/
/** @brief Defines the flags available for the CPU */
typedef struct
{
    /** @brief Streaming SIMD Extensions 3 (SSE3). */
    bool sse3 : 1;
    /** @brief PCLMULQDQ instruction support. */
    bool pclmulqdq : 1;
    /** @brief 64-bit DS save area. */
    bool dtes64 : 1;
    /** @brief MONITOR/MWAIT support. */
    bool monitor : 1;
    /** @brief CPL Qualified Debug Store. */
    bool dscpl : 1;
    /** @brief Virtual Machine Extensions. */
    bool vmx : 1;
    /** @brief Safer Mode Extensions. */
    bool smx : 1;
    /** @brief Enhanced Intel SpeedStep. */
    bool est : 1;
    /** @brief Thermal Monitor 2. */
    bool tm2 : 1;
    /** @brief Supplemental SSE3. */
    bool ssse3 : 1;
    /** @brief L1 Context ID. */
    bool cntxt_id : 1;
    /** @brief Silicon Debug. */
    bool sdbg : 1;
    /** @brief FMA extensions using YMM state. */
    bool fma : 1;
    /** @brief CMPXCHG16B instruction support. */
    bool cx16 : 1;
    /** @brief xTPR Update Control. */
    bool xtpr_update : 1;
    /** @brief Perfmon and Debug Capability. */
    bool pdcm : 1;
    /** @brief Process-context identifiers. */
    bool pcid : 1;
    /** @brief Direct Cache Access. */
    bool dca : 1;
    /** @brief SSE4.1. */
    bool sse4_1 : 1;
    /** @brief SSE4.2. */
    bool sse4_2 : 1;
    /** @brief X2APIC support. */
    bool x2apic : 1;
    /** @brief MOVBE instruction support. */
    bool movbe : 1;
    /** @brief POPCNT instruction support. */
    bool popcnt : 1;
    /** @brief APIC timer one-shot operation. */
    bool tsc_deadline_timer : 1;
    /** @brief AES instructions. */
    bool aes : 1;
    /** @brief XSAVE (and related instructions) support. */
    bool xsave : 1;
    /** @brief XSAVE (and related instructions) are enabled by OS. */
    bool osxsave : 1;
    /** @brief AVX instructions support. */
    bool avx : 1;
    /** @brief Half-precision floating-point conversion support. */
    bool f16c : 1;
    /** @brief RDRAND instruction support. */
    bool rdrand : 1;
    /** @brief System is running as guest; (para-)virtualized system. */
    bool guest_status : 1;
    /** @brief Floating-Point Unit on-chip (x87). */
    bool fpu : 1;
    /** @brief Virtual-8086 Mode Extensions. */
    bool vme : 1;
    /** @brief Debugging Extensions. */
    bool de : 1;
    /** @brief Page Size Extension. */
    bool pse : 1;
    /** @brief Time Stamp Counter. */
    bool tsc : 1;
    /** @brief Model-Specific Registers (RDMSR and WRMSR support). */
    bool msr : 1;
    /** @brief Physical Address Extensions. */
    bool pae : 1;
    /** @brief Machine Check Exception. */
    bool mce : 1;
    /** @brief CMPXCHG8B instruction. */
    bool cx8 : 1;
    /** @brief APIC on-chip. */
    bool apic : 1;
    /** @brief SYSENTER, SYSEXIT, and associated MSRs. */
    bool sep : 1;
    /** @brief Memory Type Range Registers. */
    bool mtrr : 1;
    /** @brief Page Global Extensions. */
    bool pge : 1;
    /** @brief Machine Check Architecture. */
    bool mca : 1;
    /** @brief Conditional Move Instruction. */
    bool cmov : 1;
    /** @brief Page Attribute Table. */
    bool pat : 1;
    /** @brief Page Size Extension (36-bit). */
    bool pse36 : 1;
    /** @brief Processor Serial Number. */
    bool psn : 1;
    /** @brief CLFLUSH instruction. */
    bool clflush : 1;
    /** @brief Debug Store. */
    bool ds : 1;
    /** @brief Thermal monitor and clock control. */
    bool acpi : 1;
    /** @brief MMX instructions. */
    bool mmx : 1;
    /** @brief FXSAVE and FXRSTOR instructions. */
    bool fxsr : 1;
    /** @brief SSE instructions. */
    bool sse : 1;
    /** @brief SSE2 instructions. */
    bool sse2 : 1;
    /** @brief Self Snoop. */
    bool selfsnoop : 1;
    /** @brief Hyper-threading. */
    bool htt : 1;
    /** @brief Thermal Monitor. */
    bool tm : 1;
    /** @brief Legacy IA-64 (Itanium) support bit, now reserved. */
    bool ia64 : 1;
    /** @brief Pending Break Enable. */
    bool pbe : 1;
    /** @brief FSBASE/GSBASE read/write. */
    bool fsgsbase : 1;
    /** @brief IA32_TSC_ADJUST MSR. */
    bool tsc_adjust : 1;
    /** @brief Intel SGX (Software Guard Extensions). */
    bool sgx : 1;
    /** @brief Bit manipulation extensions group 1. */
    bool bmi1 : 1;
    /** @brief Hardware Lock Elision. */
    bool hle : 1;
    /** @brief AVX2 instruction set. */
    bool avx2 : 1;
    /** @brief FPU Data Pointer updated only on x87 exceptions. */
    bool fdp_excptn_only : 1;
    /** @brief Supervisor Mode Execution Protection. */
    bool smep : 1;
    /** @brief Bit manipulation extensions group 2. */
    bool bmi2 : 1;
    /** @brief Enhanced REP MOVSB/STOSB. */
    bool erms : 1;
    /** @brief INVPCID instruction (Invalidate Processor Context ID). */
    bool invpcid : 1;
    /** @brief Intel restricted transactional memory. */
    bool rtm : 1;
    /** @brief Intel RDT-CMT / AMD Platform-QoS cache monitoring. */
    bool pqm : 1;
    /** @brief Deprecated FPU CS/DS (stored as zero). */
    bool zero_fcs_fds : 1;
    /** @brief Intel memory protection extensions. */
    bool mpx : 1;
    /** @brief Intel RDT / AMD Platform-QoS Enforcement. */
    bool rdt_a : 1;
    /** @brief AVX-512 foundation instructions. */
    bool avx512f : 1;
    /** @brief AVX-512 double/quadword instructions. */
    bool avx512dq : 1;
    /** @brief RDSEED instruction. */
    bool rdseed : 1;
    /** @brief ADCX/ADOX instructions. */
    bool adx : 1;
    /** @brief Supervisor mode access prevention. */
    bool smap : 1;
    /** @brief AVX-512 integer fused multiply add. */
    bool avx512ifma : 1;
    /** @brief CLFLUSHOPT instruction. */
    bool clflushopt : 1;
    /** @brief CLWB instruction. */
    bool clwb : 1;
    /** @brief Intel processor trace. */
    bool intel_pt : 1;
    /** @brief AVX-512 prefetch instructions. */
    bool avx512pf : 1;
    /** @brief AVX-512 exponent/reciprocal instructions. */
    bool avx512er : 1;
    /** @brief AVX-512 conflict detection instructions. */
    bool avx512cd : 1;
    /** @brief SHA/SHA256 instructions. */
    bool sha : 1;
    /** @brief AVX-512 byte/word instructions. */
    bool avx512bw : 1;
    /** @brief AVX-512 VL (128/256 vector length) extensions. */
    bool avx512vl : 1;
    /** @brief PREFETCHWT1 (Intel Xeon Phi only). */
    bool prefetchwt1 : 1;
    /** @brief AVX-512 Vector byte manipulation instructions. */
    bool avx512vbmi : 1;
    /** @brief User mode instruction protection. */
    bool umip : 1;
    /** @brief Protection keys for user-space. */
    bool pku : 1;
    /** @brief OS protection keys enable. */
    bool ospke : 1;
    /** @brief WAITPKG instructions. */
    bool waitpkg : 1;
    /** @brief AVX-512 vector byte manipulation instructions group 2. */
    bool avx512_vbmi2 : 1;
    /** @brief CET shadow stack features. */
    bool cet_ss : 1;
    /** @brief Galois field new instructions. */
    bool gfni : 1;
    /** @brief Vector AES instructions. */
    bool vaes : 1;
    /** @brief VPCLMULQDQ 256-bit instruction. */
    bool vpclmulqdq : 1;
    /** @brief Vector neural network instructions. */
    bool avx512_vnni : 1;
    /** @brief AVX-512 bitwise algorithms. */
    bool avx512_bitalg : 1;
    /** @brief Intel total memory encryption. */
    bool tme : 1;
    /** @brief AVX-512: POPCNT for vectors of DWORD/QWORD. */
    bool avx512_vpopcntdq : 1;
    /** @brief 57-bit linear addresses (five-level paging). */
    bool la57 : 1;
    /** @brief RDPID instruction. */
    bool rdpid : 1;
    /** @brief Intel key locker. */
    bool key_locker : 1;
    /** @brief OS bus-lock detection. */
    bool bus_lock_detect : 1;
    /** @brief CLDEMOTE instruction. */
    bool cldemote : 1;
    /** @brief MOVDIRI instruction. */
    bool movdiri : 1;
    /** @brief MOVDIR64B instruction. */
    bool movdir64b : 1;
    /** @brief Enqueue stores (ENQCMD{,S}). */
    bool enqcmd : 1;
    /** @brief Intel SGX launch configuration. */
    bool sgx_lc : 1;
    /** @brief Protection keys for supervisor-mode pages. */
    bool pks : 1;
    /** @brief Intel SGX attestation services. */
    bool sgx_keys : 1;
    /** @brief AVX-512 neural network instructions. */
    bool avx512_4vnniw : 1;
    /** @brief AVX-512 multiply accumulation single precision. */
    bool avx512_4fmaps : 1;
    /** @brief Fast short REP MOV. */
    bool fsrm : 1;
    /** @brief User interrupts. */
    bool uintr : 1;
    /** @brief VP2INTERSECT{D,Q} instructions. */
    bool avx512_vp2intersect : 1;
    /** @brief SRBDS mitigation MSR. */
    bool srdbs_ctrl : 1;
    /** @brief VERW MD_CLEAR microcode. */
    bool md_clear : 1;
    /** @brief XBEGIN (RTM transaction) always aborts. */
    bool rtm_always_abort : 1;
    /** @brief MSR TSX_FORCE_ABORT, RTM_ABORT bit. */
    bool tsx_force_abort : 1;
    /** @brief SERIALIZE instruction. */
    bool serialize : 1;
    /** @brief The CPU is identified as a 'hybrid part'. */
    bool hybrid_cpu : 1;
    /** @brief TSX suspend/resume load address tracking. */
    bool tsxldtrk : 1;
    /** @brief PCONFIG instruction. */
    bool pconfig : 1;
    /** @brief Intel architectural LBRs. */
    bool arch_lbr : 1;
    /** @brief CET indirect branch tracking. */
    bool cet_ibt : 1;
    /** @brief AMX-BF16: tile bfloat16. */
    bool amx_bf16 : 1;
    /** @brief AVX-512 FP16 instructions. */
    bool avx512_fp16 : 1;
    /** @brief AMX-TILE: tile architecture. */
    bool amx_tile : 1;
    /** @brief AMX-INT8: tile 8-bit integer. */
    bool amx_int8 : 1;
    /** @brief Speculation Control (indirect branch restrictions). */
    bool spec_ctrl : 1;
    /** @brief Single thread indirect branch predictors. */
    bool intel_stibp : 1;
    /** @brief FLUSH L1D cache: IA32_FLUSH_CMD MSR. */
    bool flush_l1d : 1;
    /** @brief Intel IA32_ARCH_CAPABILITIES MSR. */
    bool arch_capabilities : 1;
    /** @brief IA32_CORE_CAPABILITIES MSR. */
    bool core_capabilities : 1;
    /** @brief Speculative store bypass disable. */
    bool spec_ctrl_ssbd : 1;
    /** @brief AVX-VNNI instructions. */
    bool avx_vnni : 1;
    /** @brief AVX-512 bfloat16 instructions. */
    bool avx512_bf16 : 1;
    /** @brief Linear address space separation. */
    bool lass : 1;
    /** @brief CMPccXADD instructions. */
    bool cmpccxadd : 1;
    /** @brief ArchPerfmonExt: leaf 0x23. */
    bool arch_perfmon_ext : 1;
    /** @brief Fast zero-length REP MOVSB. */
    bool fzrm : 1;
    /** @brief Fast short REP STOSB. */
    bool fsrs : 1;
    /** @brief Fast Short REP CMPSB/SCASB. */
    bool fsrc : 1;
    /** @brief FRED: Flexible return and event delivery transitions. */
    bool fred : 1;
    /** @brief LKGS: Load 'kernel' (userspace) GS. */
    bool lkgs : 1;
    /** @brief WRMSRNS instruction (WRMSR-non-serializing). */
    bool wrmsrns : 1;
    /** @brief NMI-source reporting with FRED event data. */
    bool nmi_src : 1;
    /** @brief AMX-FP16: FP16 tile operations. */
    bool amx_fp16 : 1;
    /** @brief HRESET (Thread director history reset). */
    bool hreset  : 1;
    /** @brief Integer fused multiply add. */
    bool avx_ifma : 1;
    /** @brief Linear address masking. */
    bool lam : 1;
    /** @brief RDMSRLIST/WRMSRLIST instructions. */
    bool rd_wr_msrlist : 1;
    /** @brief Protected processor inventory number (PPIN{,_CTL} MSRs). */
    bool intel_ppin : 1;
    /** @brief AVX-VNNI-INT8 instructions. */
    bool avx_vnni_int8 : 1;
    /** @brief AVX-NE-CONVERT instructions. */
    bool avx_ne_convert : 1;
    /** @brief AMX-COMPLEX instructions (starting from Granite Rapids). */
    bool amx_complex : 1;
    /** @brief PREFETCHIT0/1 instructions. */
    bool prefetchit_0_1 : 1;
    /** @brief CET supervisor shadow stacks safe to use. */
    bool cet_sss : 1;
    /** @brief Intel predictive store forward disable. */
    bool intel_psfd : 1;
    /** @brief MSR bits IA32_SPEC_CTRL.IPRED_DIS_{U,S}. */
    bool ipred_ctrl : 1;
    /** @brief MSR bits IA32_SPEC_CTRL.RRSBA_DIS_{U,S}. */
    bool rrsba_ctrl : 1;
    /** @brief MSR bit  IA32_SPEC_CTRL.DDPD_U. */
    bool ddp_ctrl : 1;
    /** @brief MSR bit  IA32_SPEC_CTRL.BHI_DIS_S. */
    bool bhi_ctrl : 1;
    /** @brief MCDT mitigation not needed. */
    bool mcdt_no : 1;
    /** @brief UC-lock disable. */
    bool uclock_disable : 1;
    /** @brief LAHF and SAHF in 64-bit mode. */
	bool lahf_lm : 1;
	/** @brief Multi-processing legacy mode (No HT). */
	bool cmp_legacy : 1;
	/** @brief Secure Virtual Machine. */
	bool svm : 1;
	/** @brief Extended APIC space. */
	bool extapic : 1;
	/** @brief LOCK MOV CR0 means MOV CR8. */
	bool cr8_legacy : 1;
	/** @brief LZCNT advanced bit manipulation. */
	bool lzcnt_abm : 1;
	/** @brief SSE4A support. */
	bool sse4a : 1;
	/** @brief Misaligned SSE mode. */
	bool misaligned_sse : 1;
	/** @brief 3DNow PREFETCH/PREFETCHW support. */
	bool f3dnow_prefetch : 1;
	/** @brief OS visible workaround. */
	bool osvw : 1;
	/** @brief Instruction based sampling. */
	bool ibs : 1;
	/** @brief XOP: extended operation (AVX instructions). */
	bool xop : 1;
	/** @brief SKINIT/STGI support. */
	bool skinit : 1;
	/** @brief Watchdog timer support. */
	bool wdt : 1;
	/** @brief Lightweight profiling. */
	bool lwp : 1;
	/** @brief 4-operand FMA instruction. */
	bool fma4 : 1;
	/** @brief Translation cache extension. */
	bool tce : 1;
	/** @brief NodeId MSR (0xc001100c). */
	bool nodeid_msr : 1;
	/** @brief Trailing bit manipulations. */
	bool tbm : 1;
	/** @brief Topology Extensions (leaf 0x8000001d). */
	bool topoext : 1;
	/** @brief Core performance counter extensions. */
	bool perfctr_core : 1;
	/** @brief NB/DF performance counter extensions. */
	bool perfctr_nb : 1;
	/** @brief Data access breakpoint extension. */
	bool data_bp_ext : 1;
	/** @brief Performance time-stamp counter. */
	bool perf_tsc : 1;
	/** @brief LLC (L3) performance counter extensions. */
	bool perfctr_llc : 1;
	/** @brief MWAITX/MONITORX support. */
	bool mwaitx : 1;
	/** @brief Breakpoint address mask extension (to bit 31). */
	bool addr_mask_ext : 1;
	/** @brief Floating-Point Unit on-chip (x87). */
	bool e_fpu : 1;
	/** @brief Virtual-8086 Mode Extensions. */
	bool e_vme : 1;
	/** @brief Debugging Extensions. */
	bool e_de : 1;
	/** @brief Page Size Extension. */
	bool e_pse : 1;
	/** @brief Time Stamp Counter. */
	bool e_tsc : 1;
	/** @brief Model-Specific Registers (RDMSR and WRMSR support). */
	bool e_msr : 1;
	/** @brief Physical Address Extensions. */
	bool e_pae : 1;
	/** @brief Machine Check Exception. */
	bool e_mce : 1;
	/** @brief CMPXCHG8B instruction. */
	bool e_cx8 : 1;
	/** @brief APIC on-chip. */
	bool e_apic : 1;
	/** @brief SYSCALL and SYSRET instructions. */
	bool syscall : 1;
	/** @brief Memory Type Range Registers. */
	bool e_mtrr : 1;
	/** @brief Page Global Extensions. */
	bool e_pge : 1;
	/** @brief Machine Check Architecture. */
	bool e_mca : 1;
	/** @brief Conditional Move Instruction. */
	bool e_cmov : 1;
	/** @brief Page Attribute Table. */
	bool e_pat : 1;
	/** @brief Page Size Extension (36-bit). */
	bool e_pse36 : 1;
	/** @brief Out-of-spec AMD Multiprocessing bit. */
	bool obsolete_mp_bit : 1;
	/** @brief No-execute page protection. */
	bool nx : 1;
	/** @brief AMD MMX extensions. */
	bool mmxext : 1;
	/** @brief MMX instructions. */
	bool e_mmx : 1;
	/** @brief FXSAVE and FXRSTOR instructions. */
	bool e_fxsr : 1;
	/** @brief FXSAVE and FXRSTOR optimizations. */
	bool fxsr_opt : 1;
	/** @brief 1-GB large page support. */
	bool page1gb : 1;
	/** @brief RDTSCP instruction. */
	bool rdtscp : 1;
	/** @brief Long mode (x86-64, 64-bit support). */
	bool lm : 1;
	/** @brief AMD 3DNow extensions. */
	bool f3dnowext : 1;
	/** @brief 3DNow instructions. */
	bool f3dnow : 1;
    /** @brief CLZERO instruction. */
	bool clzero : 1;
	/** @brief Instruction retired counter MSR. */
	bool insn_retired_perf : 1;
	/** @brief XSAVE/XRSTOR always saves/restores FPU error pointers. */
	bool xsave_err_ptr : 1;
	/** @brief INVLPGB broadcasts a TLB invalidate. */
	bool invlpgb : 1;
	/** @brief RDPRU (Read Processor Register at User level). */
	bool rdpru : 1;
	/** @brief Memory Bandwidth Allocation (AMD bit). */
	bool mba : 1;
	/** @brief MCOMMIT instruction. */
	bool mcommit : 1;
	/** @brief WBNOINVD instruction. */
	bool wbnoinvd : 1;
	/** @brief Indirect Branch Prediction Barrier. */
	bool ibpb : 1;
	/** @brief Interruptible WBINVD/WBNOINVD. */
	bool wbinvd_int : 1;
	/** @brief Indirect Branch Restricted Speculation. */
	bool ibrs : 1;
	/** @brief Single Thread Indirect Branch Prediction mode. */
	bool stibp : 1;
	/** @brief IBRS always-on preferred. */
	bool ibrs_always_on : 1;
	/** @brief STIBP always-on preferred. */
	bool stibp_always_on : 1;
	/** @brief IBRS is preferred over software solution. */
	bool ibrs_fast : 1;
	/** @brief IBRS provides same mode protection. */
	bool ibrs_same_mode : 1;
	/** @brief Long-Mode Segment Limit Enable unsupported. */
	bool no_efer_lmsle : 1;
	/** @brief INVLPGB RAX[5] bit can be set. */
	bool tlb_flush_nested : 1;
	/** @brief Protected Processor Inventory Number. */
	bool amd_ppin : 1;
	/** @brief Speculative Store Bypass Disable. */
	bool amd_ssbd : 1;
	/** @brief virtualized SSBD (Speculative Store Bypass Disable). */
	bool virt_ssbd : 1;
	/** @brief SSBD is not needed (fixed in hardware). */
	bool amd_ssb_no : 1;
	/** @brief Collaborative Processor Performance Control. */
	bool cppc : 1;
	/** @brief Predictive Store Forward Disable. */
	bool amd_psfd : 1;
	/** @brief CPU not affected by Branch Type Confusion. */
	bool btc_no : 1;
	/** @brief IBPB clears RSB/RAS too. */
	bool ibpb_ret : 1;
	/** @brief Branch Sampling. */
	bool branch_sampling : 1;
    /** @brief No nested data breakpoints. */
	bool no_nested_data_bp : 1;
	/** @brief WRMSR to {FS,GS,KERNEL_GS}_BASE is non-serializing. */
	bool fsgs_non_serializing : 1;
	/** @brief LFENCE always serializing / synchronizes RDTSC. */
	bool lfence_serializing : 1;
	/** @brief SMM paging configuration lock. */
	bool smm_page_cfg_lock : 1;
	/** @brief Null selector clears base. */
	bool null_sel_clr_base : 1;
	/** @brief EFER MSR Upper Address Ignore. */
	bool upper_addr_ignore : 1;
	/** @brief EFER MSR Automatic IBRS. */
	bool auto_ibrs : 1;
	/** @brief SMM_CTL MSR not available. */
	bool no_smm_ctl_msr : 1;
	/** @brief Fast Short Rep STOSB. */
	bool e_fsrs : 1;
	/** @brief Fast Short Rep CMPSB. */
	bool e_fsrc : 1;
	/** @brief Prefetch control MSR. */
	bool prefetch_ctl_msr : 1;
	/** @brief Reserves opcode space. */
	bool opcode_reclaim : 1;
	/** @brief #GP when executing CPUID at CPL > 0. */
	bool user_cpuid_disable : 1;
	/** @brief Enhanced Predictive Store Forwarding. */
	bool epsf : 1;
	/** @brief Workload-based heuristic feedback to OS. */
	bool wl_feedback : 1;
	/** @brief Enhanced Return Address Predictor Security. */
	bool eraps : 1;
	/** @brief Selective Branch Predictor Barrier. */
	bool sbpb : 1;
	/** @brief Branch predictions flushed from CPU branch predictor. */
	bool ibpb_brtype : 1;
	/** @brief No SRSO vulnerability. */
	bool srso_no : 1;
	/** @brief No SRSO at user-kernel boundary. */
	bool srso_uk_no : 1;
	/** @brief MSR BP_CFG[BpSpecReduce] SRSO mitigation. */
	bool srso_msr_fix : 1;
} cpu_flags_info_t;

/** @brief Enumartion of the supported CPU types. */
typedef enum
{
    /** @brief Intel CPUs */
    CPU_INTEL,
    /** @brief AMD CPUs */
    CPU_AMD,
    /** @brief Unknown */
    CPU_UNKNOWN
} CPU_TYPE_E;

/** Defines the supported types of cache */
typedef enum
{
    /** @brief Unknown */
    CACHE_UNKNOWN = 0,
    /** @brief Data cache */
    CACHE_DATA = 1,
    /** @brief Instruction cache */
    CACHE_INSTRUCTION = 2,
    /** @brief Unified cache */
    CACHE_UNIFIED = 3,
} CPU_CACHE_TYPE_E;

/** Defines the supported types of TLB */
typedef enum
{
    /** @brief Unknown */
    TLB_UNKNOWN = 0,
    /** @brief Data TLB */
    TLB_DATA = 1,
    /** @brief Instruction TLB */
    TLB_INSTRUCTIONS = 2,
    /** @brief Unified TLB */
    TLB_UNIFIED = 3,
} CPU_TLB_TYPE_E;

/** Defines the supported Size of TLB */
typedef enum
{
    /** @brief Unknown */
    TLB_SIZE_UNKNOWN = 0,
    /** @brief 4KB TLB */
    TLB_4K = 1,
    /** @brief 2MB/4MB TLB */
    TLB_2MB_4MB = 2,
    /** @brief 1GB TLB */
    TLB_1G = 3,
} CPU_TLB_SIZE_E;

/** @brief Stores the CPU cache information */
typedef struct cpu_cache_info_t
{
    /** @brief Size in bytes */
    uint32_t size;
    /** @brief Number of ways */
    uint16_t ways;
    /** @brief Number of partitions */
    uint16_t parts;
    /** @brief Number of sets */
    uint32_t sets;
    /** @brief Line size in bytes */
    uint16_t lineSize;
    /** @brief Cache level */
    uint8_t level;
    /** @brief Cache type */
    CPU_CACHE_TYPE_E type;

    /** @brief Next link */
    struct cpu_cache_info_t* pNext;
} cpu_cache_info_t;

/** @brief Stores the CPU TLB information */
typedef struct cpu_tlb_info_t
{
    /** @brief Number of entries */
    uint32_t nbEntries;
    /** @brief Number of ways */
    uint16_t ways;
    /** @brief Number of sets */
    uint16_t sets;
    /** @brief Cache level */
    uint8_t level;
    /** @brief TLB type */
    CPU_TLB_TYPE_E type;
    /** @brief TLB entry size */
    CPU_TLB_SIZE_E size;

    /** @brief Next link */
    struct cpu_tlb_info_t* pNext;
} cpu_tlb_info_t;

/** @brief Structure containing the CPU information. */
typedef struct cpu_info_t
{
    /** @brief CPU type */
    CPU_TYPE_E type;
    /** @brief CPU identifier */
    uint16_t id;
    /** @brief CPU vendor string */
    char     pVendor[CPU_VENDOR_STR_SIZE + 1];
    /** @brief CPU model family */
    uint32_t family;
    /** @brief CPU model */
    uint32_t model;
    /** @brief CPU model name */
    char     pName[CPU_NAME_SIZE + 1];
    /** @brief CPU stepping  */
    uint32_t stepping;
    /** @brief CPU microcode version */
    uint32_t microcode;
    /** @brief CPU frequency in hertz */
    uint64_t frequencyHz;
    /** @brief CPU cache info */
    cpu_cache_info_t* pCaches;
    /** @brief CPU TLB info */
    cpu_tlb_info_t* pTLBs;
    /** @brief CPU physical ID */
    uint16_t physicalId;
    /** @brief Number of CPU siblings */
    uint16_t siblings;
    /** @brief CPU code ID */
    uint16_t coreId;
    /** @brief Number of Cores in the CPU */
    uint16_t cpuCores;
    /** @brief CPU APIC identifier */
    uint16_t apicId;
    /** @brief CPU initial APIC identifier */
    uint16_t initialApicId;
    /** @brief Tells if the CPU has an FPU */
    bool     fpu;
    /** @brief CPU CPUID feature level */
    uint32_t cpuIdLevel;
    /** CPU Write Protect status */
    bool     wp;
    /** @brief CPU crude MIPS */
    uint64_t bogoMips;
    /** @brief CFLUSH operation size */
    uint32_t clFlushSize;
    /** @brief CPU physical addressing width */
    uint8_t physAddressWidth;
    /** @brief CPU virtual addressing width */
    uint8_t virtAddressWidth;
    /** @brief CPU flags */
    cpu_flags_info_t flags;

    /** @brief Next structure in the list */
    struct cpu_info_t* pNext;
} cpu_info_t;

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
 * @brief Analyzes the CPU capabilities and fills the CPU information structure.
 *
 * @details Analyzes the CPU capabilities and fills the CPU information
 * structure. All detected flag features are reported and other CPU information
 * when available. For unavailable information, the value is left unchanged.
 *
 * @param[out] pCpuInfo The CPU information to fill.
 *  */
void cpuidAnalyzeCPU(cpu_info_t* pCpuInfo);

/**
 * @brief Prints the CPU flags in a buffer string.
 *
 * @details Prints the CPU flags in a buffer string. The returned offset of the
 * next free byte in the string, as in snprintf. It only prints the flags that
 * are enabled in the flag structure.
 *
 * @param[out] pBuffer The buffer to fill.
 * @param[in] length The maximun length of the buffer.
 * @param[in] kpInfo The flags to use for the conversion to string.
 *
 * @return Returns the offset of the next free byte in the string.
 */
size_t cpuIdGetFlagsString(char*                   pBuffer,
                           size_t                  length,
                           const cpu_flags_info_t* kpInfo);

#endif /* #ifndef __X8664_CPUID_H_ */

/************************************ EOF *************************************/