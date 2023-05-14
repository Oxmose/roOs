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
#include <cpu_interrupt.h>        /* Interrupt management */
#include <vga_console.h>          /* VGA console driver*/
#include <kernel_output.h>        /* Kernel output methods */
#include <stdint.h>               /* Generic int types */
#include <cpu.h>                  /* CPU management */
#include <string.h>               /* Memset */
#include <ctrl_block.h>           /* Thread's control block */
#include <interrupts.h>           /* Interrupts manager */

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
static uint32_t panic_code = 0;

/** @brief Stores the line at which the kernel panic was called. */
static uint32_t panic_line;

/** @brief Stores the file from which the panic was called. */
static const char* panic_file;

/** @brief Stores the module related to the panic. */
static const char* panic_module;

/** @brief Stores the message related to the panic. */
static const char* panic_msg;

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
 * @param[in] vcpu The pointer to the VCPU state at the moment of the panic.
 */
static void _print_panic_header(const virtual_cpu_t* vcpu);

/**
 * @brief Prints the CPU state at the moment of the panic.
 *
 * @details Prints the CPU state at the moment of the panic. All CPU registers
 * are dumped.
 *
 * @param[in] vcpu The pointer to the VCPU state at the moment of the panic.
 */
static void _print_cpu_state(const virtual_cpu_t* vcpu);

/**
 * @brief Prints the CPU flags at the moment of the panic.
 *
 * @details Prints the CPU flags at the moment of the panic. The flags are
 * pretty printed for better reading.
 *
 * @param[in] vcpu The pointer to the VCPU state at the moment of the panic.
 */
static void _print_cpu_flags(const virtual_cpu_t* vcpu);

/**
 * @brief Prints the stack frame rewind at the moment of the panic.
 *
 * @details Prints the stack frame rewind at the moment of the panic. The frames
 * will be unwinded and the symbols printed based on the information passed by
 * multiboot at initialization time.
 */
static void _print_stack_trace(void);

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static void _print_panic_header(const virtual_cpu_t* vcpu)
{
    const int_context_t* int_state;

    int_state = &vcpu->int_context;

    kernel_printf("##############################    KERNEL PANIC    ##########"
                    "####################\n");
    switch(int_state->int_id)
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
                  int_state->int_id);
    kernel_printf("Instruction [EIP]: 0x%p             Error code: "
                    "0x%X       \n", int_state->rip, int_state->error_code);
    kernel_printf("                                                            "
                    "                   \n");
}

static void _print_cpu_state(const virtual_cpu_t* vcpu)
{
    const cpu_state_t*   cpu_state;
    const int_context_t* int_state;

    int_state = &vcpu->int_context;
    cpu_state = &vcpu->vcpu;

    uint64_t CR0;
    uint64_t CR2;
    uint64_t CR3;
    uint64_t CR4;

    __asm__ __volatile__ (
        "mov %%cr0, %%rax\n\t"
        "mov %%rax, %0\n\t"
        "mov %%cr2, %%rax\n\t"
        "mov %%rax, %1\n\t"
        "mov %%cr3, %%rax\n\t"
        "mov %%rax, %2\n\t"
        "mov %%cr4, %%rax\n\t"
        "mov %%rax, %3\n\t"
    : "=m" (CR0), "=m" (CR2), "=m" (CR3), "=m" (CR4)
    : /* no input */
    : "%rax"
    );

    kernel_printf("RAX: 0x%p | RBX: 0x%p | RCX: 0x%p\n",
                  cpu_state->rax, cpu_state->rbx, cpu_state->rcx);
    kernel_printf("RDX: 0x%p | RSI: 0x%p | RDI: 0x%p \n",
                  cpu_state->rdx, cpu_state->rsi, cpu_state->rdi);
    kernel_printf("RBP: 0x%p | RSP: 0x%p | R8:  0x%p\n",
                  cpu_state->rbp, cpu_state->rsp, cpu_state->r8);
    kernel_printf("R9:  0x%p | R10: 0x%p | R11: 0x%p\n",
                  cpu_state->r9, cpu_state->r10, cpu_state->r11);
    kernel_printf("R12: 0x%p | R13: 0x%p | R14: 0x%p\n",
                  cpu_state->r12, cpu_state->r13, cpu_state->r14);
    kernel_printf("R15: 0x%p\n", cpu_state->r15);
    kernel_printf("CR0: 0x%p | CR2: 0x%p | CR3: 0x%p\nCR4: "
                  "0x%p\n", CR0, CR2, CR3, CR4);
    kernel_printf("CS: 0x%04X | DS: 0x%04X | SS: 0x%04X | ES: 0x%04X | "
                  "FS: 0x%04X | GS: 0x%04X\n",
                    int_state->cs & 0xFFFF,
                    cpu_state->ds & 0xFFFF,
                    cpu_state->ss & 0xFFFF,
                    cpu_state->es & 0xFFFF ,
                    cpu_state->fs & 0xFFFF ,
                    cpu_state->gs & 0xFFFF);
}

static void _print_cpu_flags(const virtual_cpu_t* vcpu)
{
    const int_context_t* int_state;

    int_state = &vcpu->int_context;

    int8_t cf_f = (int_state->rflags & 0x1);
    int8_t pf_f = (int_state->rflags & 0x4) >> 2;
    int8_t af_f = (int_state->rflags & 0x10) >> 4;
    int8_t zf_f = (int_state->rflags & 0x40) >> 6;
    int8_t sf_f = (int_state->rflags & 0x80) >> 7;
    int8_t tf_f = (int_state->rflags & 0x100) >> 8;
    int8_t if_f = (int_state->rflags & 0x200) >> 9;
    int8_t df_f = (int_state->rflags & 0x400) >> 10;
    int8_t of_f = (int_state->rflags & 0x800) >> 11;
    int8_t nf_f = (int_state->rflags & 0x4000) >> 14;
    int8_t rf_f = (int_state->rflags & 0x10000) >> 16;
    int8_t vm_f = (int_state->rflags & 0x20000) >> 17;
    int8_t ac_f = (int_state->rflags & 0x40000) >> 18;
    int8_t id_f = (int_state->rflags & 0x200000) >> 21;
    int8_t iopl0_f = (int_state->rflags & 0x1000) >> 12;
    int8_t iopl1_f = (int_state->rflags & 0x2000) >> 13;
    int8_t vif_f = (int_state->rflags & 0x8000) >> 19;
    int8_t vip_f = (int_state->rflags & 0x100000) >> 20;

    kernel_printf("RFLAGS: 0x%p | ", int_state->rflags);

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

static void _print_stack_trace(void)
{
    size_t        i;
    uintptr_t*    call_addr;
    uintptr_t*    last_rbp;
    char*         symbol;

    /* Get ebp */
    __asm__ __volatile__ ("mov %%rbp, %0\n\t" : "=m" (last_rbp));

    /* Get the return address */
    call_addr = *(uintptr_t**)(last_rbp + 1);
    for(i = 0; i < STACK_TRACE_SIZE && call_addr != NULL; ++i)
    {
        /* Get the associated symbol */
        symbol = NULL;

        kernel_printf("[%u] 0x%p in %s", i, call_addr,
                      symbol == NULL ? "[NO_SYMBOL]" : symbol);
        if(i % 2 == 0)
        {
            kernel_printf(" | ");
        }
        else
        {
            kernel_printf("\n");
        }
        last_rbp  = (uintptr_t*)*last_rbp;
        call_addr = *(uintptr_t**)(last_rbp + 1);
    }
}

void panic_handler(kernel_thread_t* curr_thread)
{
    colorscheme_t panic_scheme;
    cursor_t      panic_cursor;

    uint32_t cpu_id;

    uint32_t time;
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PANIC_HANDLER_START, 1, panic_code);

    time    = 0;
    hours   = time / 3600;
    minutes = (time / 60) % 60;
    seconds = time % 60;

    cpu_id = 0;

    panic_scheme.background = BG_BLACK;
    panic_scheme.foreground = FG_CYAN;
    panic_scheme.vga_color  = TRUE;

    console_set_color_scheme(panic_scheme);

    /* Clear screen */
    console_clear_screen();
    panic_cursor.x = 0;
    panic_cursor.y = 0;
    console_restore_cursor(panic_cursor);

    _print_panic_header(&curr_thread->v_cpu);
    _print_cpu_state(&curr_thread->v_cpu);
    _print_cpu_flags(&curr_thread->v_cpu);

    kernel_printf("\n--------------------------------- INFORMATION ------------"
                    "----------------------\n");
    kernel_printf("Core ID: %u | Time: %02u:%02u:%02u\n"
                  "Thread: %s (%u) | Process: %s (%u)\n", cpu_id,
                  hours, minutes, seconds,
                  curr_thread->name,
                  curr_thread->tid,
                  "NO_PROCESS",
                  0);

    kernel_printf("File: %s at line %d\n", panic_file, panic_line);

    if(strlen(panic_module) != 0)
    {
        kernel_printf("[%s] | ", panic_module);
    }
    kernel_printf("%s (%d)\n\n", panic_msg, panic_code);

    _print_stack_trace();

    /* Hide cursor */
    panic_scheme.background = BG_BLACK;
    panic_scheme.foreground = FG_BLACK;
    panic_scheme.vga_color  = TRUE;

    console_set_color_scheme(panic_scheme);

    KERNEL_TRACE_EVENT(EVENT_KERNEL_PANIC_HANDLER_END, 1, panic_code);

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
        _cpu_hlt();
    }
}

void kernel_panic(const uint32_t error_code,
                  const char* module,
                  const char* msg,
                  const char* file,
                  const size_t line)
{
    KERNEL_TRACE_EVENT(EVENT_KERNEL_PANIC, 1, error_code);

    /* We don't need interrupt anymore */
    kernel_interrupt_disable();

    /* Set the parameters */
    panic_code   = error_code;
    panic_module = module;
    panic_msg    = msg;
    panic_file   = file;
    panic_line   = line;

    /* Call the panic formater */
    cpu_raise_interrupt(PANIC_INT_LINE);

    /* We should never get here, but just in case */
    while(1)
    {
        kernel_interrupt_disable();
        _cpu_hlt();
    }
}

/************************************ EOF *************************************/