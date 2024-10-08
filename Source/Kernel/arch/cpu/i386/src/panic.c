/*******************************************************************************
 * @file panic.c
 *
 * @see panic.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 31/03/2023
 *
 * @version 1.0
 *
 * @brief Panic feature of the kernel.
 *
 * @details Kernel panic functions. Displays the CPU registers, the faulty
 * instruction, the interrupt ID and cause for a kernel panic. For a process
 * panic the panic will kill the process.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <x86cpu.h>           /* CPU management */
#include <stdint.h>           /* Generic int types */
#include <string.h>           /* Memset */
#include <console.h>          /* Console service */
#include <stdbool.h>          /* Bool types */
#include <time_mgt.h>         /* Time management */
#include <critical.h>         /* Critical section */
#include <core_mgt.h>         /* Core manager */
#include <scheduler.h>        /* Kernel scheduler */
#include <ctrl_block.h>       /* Thread's control block */
#include <interrupts.h>       /* Interrupts manager */
#include <kerneloutput.h>     /* Kernel output methods */
#include <cpu_interrupt.h>    /* Interrupt management */

/* Configuration files */
#include <config.h>

/* Header file */
#include <panic.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief Defines the stack trace size */
#define STACK_TRACE_SIZE 8

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/

/**
 * @brief Prints the kernel panic screen header.
 *
 * @details Prints the kernel panic screen header. The header contains the
 * title, the interrupt number, the error code and the error string that caused
 * the panic.
 *
 * @param[in] kpVCpu The pointer to the VCPU state at the moment of the panic.
 */
static void _printHeader(const virtual_cpu_t* kpVCpu);

/**
 * @brief Prints the CPU state at the moment of the panic.
 *
 * @details Prints the CPU state at the moment of the panic. All CPU registers
 * are dumped.
 *
 * @param[in] kpVCpu The pointer to the VCPU state at the moment of the panic.
 */
static void _printCpuState(const virtual_cpu_t* kpVCpu);

/**
 * @brief Prints the CPU flags at the moment of the panic.
 *
 * @details Prints the CPU flags at the moment of the panic. The flags are
 * pretty printed for better reading.
 *
 * @param[in] kpVCpu The pointer to the VCPU state at the moment of the panic.
 */
static void _printCpuFlags(const virtual_cpu_t* kpVCpu);

/**
 * @brief Prints the stack frame rewind at the moment of the panic.
 *
 * @details Prints the stack frame rewind at the moment of the panic. The frames
 * will be unwinded and the symbols printed based on the information passed by
 * multiboot at initialization time.
 *
 * @param[in] lastEPB The last EBP in the stack to display.
 */
static void _printStackTrace(uintptr_t* lastEPB);

/**
 * @brief Kernel panic function used when the scheduler is not initialized.
 *
 * @details Kernel panic function used when the scheduler is not initialized.
 * This function does not rely on the currently executed thread to display
 * the panic information.
 */
static void _panicNoSched(void);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief Stores the current kernel panic error code. */
static uint32_t sPanicCode = 0;

/** @brief Stores the line at which the kernel panic was called. */
static uint32_t sPanicLine = 0;

/** @brief Stores the file from which the panic was called. */
static const char* skpPanicFile = NULL;

/** @brief Stores the module related to the panic. */
static const char* skpPanicModule = NULL;

/** @brief Stores the message related to the panic. */
static const char* skpPanicMsg = NULL;

/** @brief Panic lock */
static spinlock_t sLock = SPINLOCK_INIT_VALUE;

/** @brief Panic delivered flag */
static bool sDelivered = false;

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _printHeader(const virtual_cpu_t* kpVCpu)
{
    const int_context_t* kpIntState;

    kpIntState = &kpVCpu->intContext;

    kprintfPanic("##############################    KERNEL PANIC    ##########"
                    "####################\n");
    switch(kpIntState->intId)
    {
        case 0:
            kprintfPanic("Division by zero                        ");
            break;
        case 1:
            kprintfPanic("Single-step interrupt                   ");
            break;
        case 2:
            kprintfPanic("Non maskable interrupt                  ");
            break;
        case 3:
            kprintfPanic("Breakpoint                              ");
            break;
        case 4:
            kprintfPanic("Overflow                                ");
            break;
        case 5:
            kprintfPanic("Bounds                                  ");
            break;
        case 6:
            kprintfPanic("Invalid Opcode                          ");
            break;
        case 7:
            kprintfPanic("Coprocessor not available               ");
            break;
        case 8:
            kprintfPanic("Double fault                            ");
            break;
        case 9:
            kprintfPanic("Coprocessor Segment Overrun             ");
            break;
        case 10:
            kprintfPanic("Invalid Task State Segment              ");
            break;
        case 11:
            kprintfPanic("Segment not present                     ");
            break;
        case 12:
            kprintfPanic("Stack Fault                             ");
            break;
        case 13:
            kprintfPanic("General protection fault                ");
            break;
        case 14:
            kprintfPanic("Page fault                              ");
            break;
        case 16:
            kprintfPanic("Math Fault                              ");
            break;
        case 17:
            kprintfPanic("Alignment Check                         ");
            break;
        case 18:
            kprintfPanic("Machine Check                           ");
            break;
        case 19:
            kprintfPanic("SIMD Floating-Point Exception           ");
            break;
        case 20:
            kprintfPanic("Virtualization Exception                ");
            break;
        case 21:
            kprintfPanic("Control Protection Exception            ");
            break;
        case PANIC_INT_LINE:
            kprintfPanic("Panic generated by the kernel           ");
            break;
        default:
            kprintfPanic("Unknown reason                          ");
    }

    kprintfPanic("          INT ID: 0x%02X                 \n",
                  kpIntState->intId);
    kprintfPanic("Instruction [EIP]: 0x%p                     Error code: "
                   "0x%X       \n",
                   kpIntState->eip,
                   kpIntState->errorCode);
    kprintfPanic("\n\n");
}

static void _printCpuState(const virtual_cpu_t* kpVCpu)
{
    const cpu_state_t*   cpuState;
    const int_context_t* intState;

    intState = &kpVCpu->intContext;
    cpuState = &kpVCpu->cpuState;

    uint32_t CR0;
    uint32_t CR2;
    uint32_t CR3;
    uint32_t CR4;

    __asm__ __volatile__ (
        "mov %%cr0, %%eax\n\t"
        "mov %%eax, %0\n\t"
        "mov %%cr2, %%eax\n\t"
        "mov %%eax, %1\n\t"
        "mov %%cr3, %%eax\n\t"
        "mov %%eax, %2\n\t"
        "mov %%cr4, %%eax\n\t"
        "mov %%eax, %3\n\t"
    : "=m" (CR0), "=m" (CR2), "=m" (CR3), "=m" (CR4)
    : /* no input */
    : "%eax"
    );

    kprintfPanic("EAX: 0x%p | EBX: 0x%p | ECX: 0x%p | EDX: 0x%p  \n",
                  cpuState->eax,
                  cpuState->ebx,
                  cpuState->ecx,
                  cpuState->edx);
    kprintfPanic("ESI: 0x%p | EDI: 0x%p | EBP: 0x%p | ESP: 0x%p  \n",
                  cpuState->esi,
                  cpuState->edi,
                  cpuState->ebp,
                  cpuState->esp);
    kprintfPanic("CR0: 0x%p | CR2: 0x%p | CR3: 0x%p | CR4: 0x%p  \n",
                  CR0,
                  CR2,
                  CR3,
                  CR4);
    kprintfPanic("CS: 0x%04X | DS: 0x%04X | SS: 0x%04X | ES: 0x%04X | "
                  "FS: 0x%04X | GS: 0x%04X\n",
                  intState->cs & 0xFFFF,
                  cpuState->ds & 0xFFFF,
                  cpuState->ss & 0xFFFF,
                  cpuState->es & 0xFFFF ,
                  cpuState->fs & 0xFFFF ,
                  cpuState->gs & 0xFFFF);
}

static void _printCpuFlags(const virtual_cpu_t* kpVCpu)
{
    const int_context_t* intState;

    intState = &kpVCpu->intContext;

    int8_t cf_f = (intState->eflags & 0x1);
    int8_t pf_f = (intState->eflags & 0x4) >> 2;
    int8_t af_f = (intState->eflags & 0x10) >> 4;
    int8_t zf_f = (intState->eflags & 0x40) >> 6;
    int8_t sf_f = (intState->eflags & 0x80) >> 7;
    int8_t tf_f = (intState->eflags & 0x100) >> 8;
    int8_t if_f = (intState->eflags & 0x200) >> 9;
    int8_t df_f = (intState->eflags & 0x400) >> 10;
    int8_t of_f = (intState->eflags & 0x800) >> 11;
    int8_t nf_f = (intState->eflags & 0x4000) >> 14;
    int8_t rf_f = (intState->eflags & 0x10000) >> 16;
    int8_t vm_f = (intState->eflags & 0x20000) >> 17;
    int8_t ac_f = (intState->eflags & 0x40000) >> 18;
    int8_t id_f = (intState->eflags & 0x200000) >> 21;
    int8_t iopl0_f = (intState->eflags & 0x1000) >> 12;
    int8_t iopl1_f = (intState->eflags & 0x2000) >> 13;
    int8_t vif_f = (intState->eflags & 0x8000) >> 19;
    int8_t vip_f = (intState->eflags & 0x100000) >> 20;

    kprintfPanic("EFLAGS: 0x%p | ", intState->eflags);

    if(cf_f != 0)
    {
        kprintfPanic("CF ");
    }
    if(pf_f != 0)
    {
        kprintfPanic("PF ");
    }
    if(af_f != 0)
    {
        kprintfPanic("AF ");
    }
    if(zf_f != 0)
    {
        kprintfPanic("ZF ");
    }
    if(sf_f != 0)
    {
        kprintfPanic("SF ");
    }
    if(tf_f != 0)
    {
        kprintfPanic("TF ");
    }
    if(if_f != 0)
    {
        kprintfPanic("IF ");
    }
    if(df_f != 0)
    {
        kprintfPanic("DF ");
    }
    if(of_f != 0)
    {
        kprintfPanic("OF ");
    }
    if(nf_f != 0)
    {
        kprintfPanic("NT ");
    }
    if(rf_f != 0)
    {
        kprintfPanic("RF ");
    }
    if(vm_f != 0)
    {
        kprintfPanic("VM ");
    }
    if(ac_f != 0)
    {
        kprintfPanic("AC ");
    }
    if(vif_f != 0)
    {
        kprintfPanic("VF ");
    }
    if(vip_f != 0)
    {
        kprintfPanic("VP ");
    }
    if(id_f != 0)
    {
        kprintfPanic("ID ");
    }
    if((iopl0_f | iopl1_f) != 0)
    {
        kprintfPanic("IO: %d ", (iopl0_f | iopl1_f << 1));
    }
    kprintfPanic("\n");
}

static void _printStackTrace(uintptr_t* lastEBP)
{
    size_t    i;
    uintptr_t callAddr;

    /* Get the return address */
    kprintfPanic("Last EBP: 0x%p\n", lastEBP);

    for(i = 0; i < STACK_TRACE_SIZE && lastEBP != NULL; ++i)
    {
        callAddr = *(lastEBP + 1);

        if(callAddr == 0x0) break;

        if(i != 0 && i % 4 == 0)
        {
            kprintfPanic("\n");
        }
        else if(i != 0)
        {
            kprintfPanic(" | ");
        }

        kprintfPanic("[%u] 0x%p", i, callAddr);
        lastEBP  = (uintptr_t*)*lastEBP;
    }
}

static void _panicNoSched(void)
{
    colorscheme_t  consoleScheme;
    cursor_t       consoleCursor;
    uint8_t        cpuId;
    time_t         currTime;
    uint64_t       uptime;
    uintptr_t*     lastEBP;

    interruptDisable();

    cpuId = cpuGetId();

    consoleScheme.background = BG_BLACK;
    consoleScheme.foreground = FG_CYAN;
    consoleScheme.vgaColor   = true;

    consoleSetColorScheme(&consoleScheme);

    /* Clear screen */
    consoleClear();
    consoleCursor.x = 0;
    consoleCursor.y = 0;
    consoleRestoreCursor(&consoleCursor);

    kprintfPanic("##############################    KERNEL PANIC    ##########"
                    "####################\n");

    kprintfPanic("\nPanic called before scheduler was initialized. Error %d\n",
            sPanicCode);
    uptime = timeGetUptime();
    currTime = timeGetDayTime();
    kprintfPanic("\n--------------------------------- INFORMATION ------------"
                    "----------------------\n");
    kprintfPanic("Core ID: %u | Time: %02u:%02u:%02u | "
            "Core uptime: [%llu.%llu.%llu.%llu]\n",
            cpuId,
            currTime.hours,
            currTime.minutes,
            currTime.seconds,
            uptime / 1000000000,
            (uptime / 1000000) % 1000,
            (uptime / 1000) % 1000,
            uptime % 1000);

    if(skpPanicFile != NULL)
    {
        kprintfPanic("File: %s at line %d\n", skpPanicFile, sPanicLine);
    }

    if(skpPanicModule != NULL && strlen(skpPanicModule) != 0)
    {
        kprintfPanic("[%s] | ", skpPanicModule);
    }
    if(skpPanicMsg != NULL)
    {
        kprintfPanic("%s (%d)\n\n", skpPanicMsg, sPanicCode);
    }

    /* Get rbp */
    __asm__ __volatile__ ("mov %%ebp, %0\n\t" : "=m" (lastEBP));

    _printStackTrace(lastEBP);

    /* Hide cursor */
    consoleScheme.background = BG_BLACK;
    consoleScheme.foreground = FG_BLACK;
    consoleScheme.vgaColor   = true;

    consoleSetColorScheme(&consoleScheme);

    /* We will never return from interrupt */
    while(1)
    {
        interruptDisable();
        cpuHalt();
    }
}

void kernelPanicHandler(kernel_thread_t* pCurrThread)
{
    colorscheme_t  consoleScheme;
    cursor_t       consoleCursor;
    virtual_cpu_t* pThreadVCpu;
    uint8_t        cpuId;
    time_t         currTime;
    uint64_t       uptime;
    ipi_params_t   ipiParams;

    interruptDisable();

    spinlockAcquire(&sLock);

    if(sDelivered == true)
    {
        spinlockRelease(&sLock);
        goto PANIC_END;
    }

    sDelivered = true;
    spinlockRelease(&sLock);

    cpuId = cpuGetId();

    /* Send IPI to all other cores and tell that the panic was delivered */
    ipiParams.function = IPI_FUNC_PANIC;
    cpuMgtSendIpi(CPU_IPI_BROADCAST_TO_OTHER, &ipiParams, false);

    pThreadVCpu = pCurrThread->pVCpu;

    consoleScheme.background = BG_BLACK;
    consoleScheme.foreground = FG_CYAN;
    consoleScheme.vgaColor   = true;

    consoleSetColorScheme(&consoleScheme);

    /* Clear screen */
    consoleClear();
    consoleCursor.x = 0;
    consoleCursor.y = 0;
    consoleRestoreCursor(&consoleCursor);

    _printHeader(pThreadVCpu);
    _printCpuState(pThreadVCpu);
    _printCpuFlags(pThreadVCpu);

    uptime = timeGetUptime();
    currTime = timeGetDayTime();
    kprintfPanic("\n--------------------------------- INFORMATION ------------"
                    "----------------------\n");
    kprintfPanic("Core ID: %u | Time: %02u:%02u:%02u | "
            "Core uptime: [%llu.%llu.%llu.%llu]\n"
            "Thread: %s (%u, state: %d) | Process: %s (%u)\n",
            cpuId,
            currTime.hours,
            currTime.minutes,
            currTime.seconds,
            uptime / 1000000000,
            (uptime / 1000000) % 1000,
            (uptime / 1000) % 1000,
            uptime % 1000,
            pCurrThread->pName,
            pCurrThread->tid,
            pCurrThread->currentState,
            pCurrThread->pProcess->pName,
            pCurrThread->pProcess->pid);

    if(skpPanicFile != NULL)
    {
        kprintfPanic("File: %s at line %d\n", skpPanicFile, sPanicLine);
    }

    if(skpPanicModule != NULL && strlen(skpPanicModule) != 0)
    {
        kprintfPanic("[%s] | ", skpPanicModule);
    }
    if(skpPanicMsg != NULL)
    {
        kprintfPanic("%s (%d)\n\n", skpPanicMsg, sPanicCode);
    }

    _printStackTrace((uintptr_t*)pThreadVCpu->cpuState.ebp);

    /* Hide cursor */
    consoleScheme.background = BG_BLACK;
    consoleScheme.foreground = FG_BLACK;
    consoleScheme.vgaColor   = true;

    consoleSetColorScheme(&consoleScheme);

    TEST_POINT_ASSERT_RCODE(TEST_PANIC_SUCCESS_ID,
                            true,
                            OS_NO_ERR,
                            OS_NO_ERR,
                            TEST_PANIC_ENABLED);

#if TEST_PANIC_ENABLED
    TEST_FRAMEWORK_END();
#endif

PANIC_END:
    /* We will never return from interrupt */
    while(1)
    {
        interruptDisable();
        cpuHalt();
    }
}

void kernelPanic(const uint32_t kErrorCode,
                 const char*    kpModule,
                 const char*    kpMsg,
                 const char*    kpFile,
                 const size_t   kLine)
{

    /* We don't need interrupt anymore */
    interruptDisable();

    /* Set the parameters */
    sPanicCode     = kErrorCode;
    skpPanicModule = kpModule;
    skpPanicMsg    = kpMsg;
    skpPanicFile   = kpFile;
    sPanicLine     = kLine;

    if(schedIsRunning() == false)
    {
        _panicNoSched();
    }
    else
    {
        /* Call the panic formater */
        cpuRaiseInterrupt(PANIC_INT_LINE);
    }

    /* We should never get here, but just in case */
    while(1)
    {
        interruptDisable();
        cpuHalt();
    }
}

/************************************ EOF *************************************/