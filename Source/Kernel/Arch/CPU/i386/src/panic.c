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
#include <cpu.h>              /* CPU management */
#include <stdint.h>           /* Generic int types */
#include <string.h>           /* Memset */
#include <console.h>          /* Console service */
#include <vgatext.h>          /* VGA console driver*/
#include <time_mgt.h>         /* Time management */
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
#define STACK_TRACE_SIZE 6

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

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
/** @brief Stores the current kernel panic error code. */
static uint32_t sPanicCode = 0;

/** @brief Stores the line at which the kernel panic was called. */
static uint32_t sPanicLine;

/** @brief Stores the file from which the panic was called. */
static const char* skpPanicFile;

/** @brief Stores the module related to the panic. */
static const char* skpPanicModule;

/** @brief Stores the message related to the panic. */
static const char* skpPanicMsg;

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
 */
static void _printStackTrace(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _printHeader(const virtual_cpu_t* kpVCpu)
{
    const int_context_t* kpIntState;

    kpIntState = &kpVCpu->intContext;

    kernel_printf("##############################    KERNEL PANIC    ##########"
                    "####################\n");
    switch(kpIntState->intId)
    {
        case 0:
            kernel_printf("Division by zero                        ");
            break;
        case 1:
            kernel_printf("Single-step interrupt                   ");
            break;
        case 2:
            kernel_printf("Non maskable interrupt                  ");
            break;
        case 3:
            kernel_printf("Breakpoint                              ");
            break;
        case 4:
            kernel_printf("Overflow                                ");
            break;
        case 5:
            kernel_printf("Bounds                                  ");
            break;
        case 6:
            kernel_printf("Invalid Opcode                          ");
            break;
        case 7:
            kernel_printf("Coprocessor not available               ");
            break;
        case 8:
            kernel_printf("Double fault                            ");
            break;
        case 9:
            kernel_printf("Coprocessor Segment Overrun             ");
            break;
        case 10:
            kernel_printf("Invalid Task State Segment              ");
            break;
        case 11:
            kernel_printf("Segment not present                     ");
            break;
        case 12:
            kernel_printf("Stack Fault                             ");
            break;
        case 13:
            kernel_printf("General protection fault                ");
            break;
        case 14:
            kernel_printf("Page fault                              ");
            break;
        case 16:
            kernel_printf("Math Fault                              ");
            break;
        case 17:
            kernel_printf("Alignment Check                         ");
            break;
        case 18:
            kernel_printf("Machine Check                           ");
            break;
        case 19:
            kernel_printf("SIMD Floating-Point Exception           ");
            break;
        case 20:
            kernel_printf("Virtualization Exception                ");
            break;
        case 21:
            kernel_printf("Control Protection Exception            ");
            break;
        case PANIC_INT_LINE:
            kernel_printf("Panic generated by the kernel           ");
            break;
        default:
            kernel_printf("Unknown reason                          ");
    }

    kernel_printf("          INT ID: 0x%02X                 \n",
                  kpIntState->intId);
    kernel_printf("Instruction [EIP]: 0x%p                     Error code: "
                    "0x%X       \n", 
                    kpIntState->eip, 
                    kpIntState->errorCode);
    kernel_printf("\n\n");
}

static void _printCpuState(const virtual_cpu_t* kpVCpu)
{
    const cpu_state_t*   cpuState;
    const int_context_t* intState;

    intState = &kpVCpu->intContext;
    cpuState = &kpVCpu->vCpu;

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

    kernel_printf("EAX: 0x%p | EBX: 0x%p | ECX: 0x%p | EDX: 0x%p  \n", 
                  cpuState->eax, 
                  cpuState->ebx,
                  cpuState->ecx,  
                  cpuState->edx);
    kernel_printf("ESI: 0x%p | EDI: 0x%p | EBP: 0x%p | ESP: 0x%p  \n", 
                  cpuState->esi, 
                  cpuState->edi,
                  cpuState->ebp, 
                  cpuState->esp);
    kernel_printf("CR0: 0x%p | CR2: 0x%p | CR3: 0x%p | CR4: 0x%p  \n", 
                  CR0, 
                  CR2, 
                  CR3, 
                  CR4);
    kernel_printf("CS: 0x%04X | DS: 0x%04X | SS: 0x%04X | ES: 0x%04X | "
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

    kernel_printf("EFLAGS: 0x%p | ", intState->eflags);

    if(cf_f != 0)
    {
        kernel_printf("CF ");
    }
    if(pf_f != 0)
    {
        kernel_printf("PF ");
    }
    if(af_f != 0)
    {
        kernel_printf("AF ");
    }
    if(zf_f != 0)
    {
        kernel_printf("ZF ");
    }
    if(sf_f != 0)
    {
        kernel_printf("SF ");
    }
    if(tf_f != 0)
    {
        kernel_printf("TF ");
    }
    if(if_f != 0)
    {
        kernel_printf("IF ");
    }
    if(df_f != 0)
    {
        kernel_printf("DF ");
    }
    if(of_f != 0)
    {
        kernel_printf("OF ");
    }
    if(nf_f != 0)
    {
        kernel_printf("NT ");
    }
    if(rf_f != 0)
    {
        kernel_printf("RF ");
    }
    if(vm_f != 0)
    {
        kernel_printf("VM ");
    }
    if(ac_f != 0)
    {
        kernel_printf("AC ");
    }
    if(vif_f != 0)
    {
        kernel_printf("VF ");
    }
    if(vip_f != 0)
    {
        kernel_printf("VP ");
    }
    if(id_f != 0)
    {
        kernel_printf("ID ");
    }
    if((iopl0_f | iopl1_f) != 0)
    {
        kernel_printf("IO: %d ", (iopl0_f | iopl1_f << 1));
    }
    kernel_printf("\n");
}

static void _printStackTrace(void)
{
    size_t        i;
    uintptr_t*    callAddr;
    uintptr_t*    lastEBP;
    char*         symbol;

    /* Get ebp */
    __asm__ __volatile__ ("mov %%ebp, %0\n\t" : "=m" (lastEBP));

    /* Get the return address */
    callAddr = *(uintptr_t**)(lastEBP + 1);
    for(i = 0; i < STACK_TRACE_SIZE && callAddr != NULL; ++i)
    {
        /* Get the associated symbol */
        symbol = NULL;

        kernel_printf("[%u] 0x%p in %s", i, callAddr,
                      symbol == NULL ? "[NO_SYMBOL]" : symbol);
        if(i % 2 == 0)
        {
            kernel_printf(" | ");
        }
        else
        {
            kernel_printf("\n");
        }

        lastEBP  = (uintptr_t*)*lastEBP;
        callAddr = *(uintptr_t**)(lastEBP + 1);
    }


}

void kernelPanicHandler(kernel_thread_t* pCurrThread)
{
    colorscheme_t consoleScheme;
    cursor_t      consoleCursor;

    uint32_t cpuId;

    uint32_t time;
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PANIC_HANDLER_START, 1, sPanicCode);

    kernel_interrupt_disable();

    time    = 0; // TODO: Get RTC time.
    hours   = time / 3600;
    minutes = (time / 60) % 60;
    seconds = time % 60;

    cpuId = 0; // TODO: Get CPU id

    consoleScheme.background = BG_BLACK;
    consoleScheme.foreground = FG_CYAN;
    consoleScheme.vgaColor   = TRUE;

    consoleSetColorScheme(&consoleScheme);

    /* Clear screen */
    consoleClear();
    consoleCursor.x = 0;
    consoleCursor.y = 0;
    consoleRestoreCursor(&consoleCursor);

    _printHeader(&pCurrThread->v_cpu);
    _printCpuState(&pCurrThread->v_cpu);
    _printCpuFlags(&pCurrThread->v_cpu);

    kernel_printf("\n--------------------------------- INFORMATION ------------"
                    "----------------------\n");
    kernel_printf("Core ID: %u | Time: %02u:%02u:%02u [%llu]\n"
                  "Thread: %s (%u) | Process: %s (%u)\n",
                  cpuId,
                  hours, 
                  minutes, 
                  seconds, 
                  time_get_current_uptime(),
                  pCurrThread->name,
                  pCurrThread->tid,
                  "NO_PROCESS", // TODO: Process name
                  0); // TODO: Process ID

    kernel_printf("File: %s at line %d\n", skpPanicFile, sPanicLine);

    if(strlen(skpPanicModule) != 0)
    {
        kernel_printf("[%s] | ", skpPanicModule);
    }
    kernel_printf("%s (%d)\n\n", skpPanicMsg, sPanicCode);

    _printStackTrace();

    /* Hide cursor */
    consoleScheme.background = BG_BLACK;
    consoleScheme.foreground = FG_BLACK;
    consoleScheme.vgaColor   = TRUE;

    consoleSetColorScheme(&consoleScheme);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PANIC_HANDLER_END, 1, sPanicCode);

    TEST_POINT_ASSERT_RCODE(TEST_PANIC_SUCCESS_ID,
                            TRUE,
                            OS_NO_ERR,
                            OS_NO_ERR,
                            TEST_PANIC_ENABLED);

#if TEST_PANIC_ENABLED
    TEST_FRAMEWORK_END();
#endif

    /* We will never return from interrupt */
    while(1)
    {
        kernel_interrupt_disable();
        _cpuHalt();
    }
}

void kernelPanic(const uint32_t kErrorCode,
                 const char*    kpModule,
                 const char*    kpMsg,
                 const char*    kpFile,
                 const size_t   kLine)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_PANIC, 1, kErrorCode);

    /* We don't need interrupt anymore */
    kernel_interrupt_disable();

    /* Set the parameters */
    sPanicCode     = kErrorCode;
    skpPanicModule = kpModule;
    skpPanicMsg    = kpMsg;
    skpPanicFile   = kpFile;
    sPanicLine     = kLine;

    /* Call the panic formater */
    cpuRaiseInterrupt(PANIC_INT_LINE);

    /* We should never get here, but just in case */
    while(1)
    {
        kernel_interrupt_disable();
        _cpuHalt();
    }
}

/************************************ EOF *************************************/