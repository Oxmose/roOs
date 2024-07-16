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
 * @brief Generic CPU management functions
 *
 * @details Generic CPU manipulation functions. The underlying platform should
 * implement those calls.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __CPU_H_
#define __CPU_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include <stdint.h>       /* Generic int types */
#include <stddef.h>       /* Standard definitions */
#include <kerror.h>       /* Kernel error */
#include <ctrl_block.h>   /* Kernel control blocks */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/


/** @brief CPU IPI send flag: send to all CPUs but the calling one. */
#define CPU_IPI_BROADCAST_TO_OTHER 0x100

/** @brief CPU IPI send flag: send to all CPUs including the calling one. */
#define CPU_IPI_BROADCAST_TO_ALL 0x300

/** @brief CPU IPI send flag: send to a specific CPU using its id. */
#define CPU_IPI_SEND_TO(X) ((X) & 0xFF)

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief Structure that contains the CPU interrupts fonfiguration. */
typedef struct
{
    /** @brief Minimal exception line */
    uint32_t minExceptionLine;
    /** @brief Maximal exception line */
    uint32_t maxExceptionLine;
    /** @brief Minimal interrupt line */
    uint32_t minInterruptLine;
    /** @brief Maximal interrupt line */
    uint32_t maxInterruptLine;
    /** @brief Total interrupt lines, including interrupts and exceptions */
    uint32_t totalInterruptLineCount;
    /** @brief Kernel panic interrupt line id */
    uint32_t panicInterruptLine;
    /** @brief Kernel scheduling interrupt line */
    uint32_t schedulerInterruptLine;
    /** @brief Spurious interrupts line id */
    uint32_t spuriousInterruptLine;
    /** @brief IPI interrupt line id */
    uint32_t ipiInterruptLine;
} cpu_interrupt_config_t;

/** @brief Defines the IPI functions */
typedef enum
{
    /** @brief Panic function */
    IPI_FUNC_PANIC = 0,
    /** @brief TLB invalidation function */
    IPI_FUNC_TLB_INVAL = 1,
    /** @brief Scheduler call function */
    IPI_FUNC_SCHEDULE = 2,
} IPI_FUNCTION_E;

/** @brief Defines the IPI parameters structure. */
typedef struct
{
    /** @brief IPI function to be used. */
    IPI_FUNCTION_E function;
    /** @brief Data for the function to be used */
    void* pData;
} ipi_params_t;

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
 * @brief Returns the saved interrupt state.
 *
 * @details Returns the saved interrupt state based on the virtual CPU.
 *
 * @param[in] kpVCpu The virtual CPU to extract the information from.
 *
 * @return The current saved interrupt state: 1 if enabled, 0 otherwise.
 */
uint32_t cpuGetContextIntState(const void* kpVCpu);

/**
 * @brief Returns the last interrupt registered for the virtual CPU.
 *
 * @details Returns the last interrupt registered for the virtual CPU.
 *
 * @param[in] kpVCpu The virtual CPU to extract the information from.
 *
 * @return The current saved interrupt number.
 */
uint32_t cpuGetContextIntNumber(const void* kpVCpu);

/**
 * @brief Returns the last instruction pointer registered for the virtual CPU.
 *
 * @details Returns the last instruction pointer registered for the virtual CPU.
 *
 * @param[in] kpVCpu The virtual CPU to extract the information from.
 *
 * @return The current saved instruction pointer.
 */
uintptr_t cpuGetContextIP(const void* kpVCpu);

/**
 * @brief Returns the CPU's interrupt configuration.
 *
 * @details Returns the CPU's interrupt configuration. The configuration is
 * initialized during the CPU initialization.
 *
 * @return The CPU's interrupt configuration is returned.
 */
const cpu_interrupt_config_t* cpuGetInterruptConfig(void);

/**
 * @brief Initializes the CPU.
 *
 * @details Initializes the CPU registers and relevant structures.
 */
void cpuInit(void);

/**
 * @brief Checks the architecture's feature and requirements for roOs.
 *
 * @details Checks the architecture's feature and requirements for roOs. If a
 * requirement is not met, a kernel panic is raised.
 */
void cpuValidateArchitecture(void);

/**
 * @brief Returns the CPU current interrupt state.
 *
 * @details Returns the current CPU eflags interrupt enable value.
 *
 * @return The CPU current interrupt state: 1 if enabled, 0 otherwise.
 */
uint32_t cpuGetIntState(void);

/** @brief Clears interupt bit which results in disabling interrupts. */
void cpuClearInterrupt(void);

/** @brief Sets interrupt bit which results in enabling interupts. */
void cpuSetInterrupt(void);

/** @brief Halts the CPU for lower energy consuption. */
void cpuHalt(void);

/**
 * @brief Returns the CPU identifier of the calling code.
 *
 * @details Returns the CPU identifier of the calling code.
 *
 * @return The CPU identifier of the calling core is returned.
 */
uint8_t cpuGetId(void);

/**
 * @brief Creates and allocates a kernel stack.
 *
 * @details Creates and allocates a kernel stack. The stack will be mapped
 * to the kernel's virtual memory. The function aligns the end pointer of the
 * stack based on the CPU requirements and returns this end pointer.
 *
 * @param[in] kStackSize The stack size in bytes.
 *
 * @return The end address (high address) with CPU required alignement is
 * returned.
 */
uintptr_t cpuCreateKernelStack(const size_t kStackSize);

/**
 * @brief Destroys and deallocates a kernel stack.
 *
 * @details Destroys and deallocates a kernel stack. The stack will be unmapped
 * from the kernel's virtual memory.
 *
 * @param[in] kStackEndAddr The stack end address to destroy.
 * @param[in] kStackSize The stack size in bytes.
 */
void cpuDestroyKernelStack(const uintptr_t kStackEndAddr,
                           const size_t kStackSize);

/**
 * @brief Creates a thread's virtual CPU.
 *
 * @details Initializes the thread's virtual CPU structure later used by the
 * CPU. This structure is private to the architecture and should not be used
 * elsewhere than the CPU module.
 *
 * @param[in] kpEntryPoint The thread's entry point used to initialize the vCPU.
 * @param[out] pThread The thread's structure to update.
 *
 * @return The address of the newly created VCPU is returned.
 */
uintptr_t cpuCreateVirtualCPU(void             (*kEntryPoint)(void),
                              kernel_thread_t* pThread);

/**
 * @brief Detroys a thread's virtual CPU.
 *
 * @details Detroys the thread's virtual CPU structure.
 *
 * @param[in] kpEntryPoint The thread's entry point used to initialize the vCPU.
 * @param[out] pThread The thread's structure to update.
 */
void cpuDestroyVirtualCPU(const uintptr_t kVCpuAddress);

/**
 * @brief Restores the CPU context of a thread.
 *
 * @details Restores the CPU context of a thread. This function will load the
 * thread's vCPU and replace the current context.
 *
 * @param[in] kpThread The thread of which the CPU should restore the context.
 */
void cpuRestoreContext(const kernel_thread_t* kpThread);

/**
 * @brief Redirects the execution flow of a thread to a signal.
 *
 * @details Redirects the execution flow of a thread to a signal. The actual
 * execution of the new flow is done only on the next context restore.
 *
 * @param[in] pThread The thread for which the redirection is done.
 * @param[in] instructionAddr The address to which the execution flow is done.
 *
 * @warning This function shall only be called in the scheduler it is restoring
 * the thread's context.
 */
void cpuRequestSignal(kernel_thread_t* pThread, void* instructionAddr);

/**
 * @brief Registers the CPU exceptions.
 *
 * @details Registers the CPU exceptions. Since CPUs can have different
 * exceptions, its is up to the CPU to manage its own.
 *
 * @return The success or error state is returned.
 */
OS_RETURN_E cpuRegisterExceptions(void);

/**
 * @brief Manages exception occuring during thread execution.
 *
 * @details Manages exception occuring during thread execution. Exception are
 * signaled and then handled once the signal handler is executing.
 *
 * @param[in] pThread The thread to manage the exceptions of.
 */
void cpuManageThreadException(kernel_thread_t* pThread);

/**
 * @brief Sends an IPI to the cores.
 *
 * @details Sends an IPI to the cores. The flags define the nature of the
 * IPI, if it should be broadcasted, including the calling core, etc.
 *
 * @param[in] kFlags The flags to use, see IPI flags definition.
 * @param[in] pParams The IPI parameters to use.
 * @param[in] kAllocateParam Tells if the parameters should be duplicated, in
 * the advent the IPI is received once the current parameters structure was
 * destroyed.
 */
void cpuMgtSendIpi(const uint32_t kFlags,
                   ipi_params_t*  pParams,
                   const bool_t   kAllocateParam);


/**
 * @brief Tells if the VCPU had is latest context saved.
 *
 * @details Tells if the VCPU had is latest context saved. This is refering
 * to the VCPU's context out of interrupts.
 *
 * @param[in] kpVCpu The VCPU to check.
 *
 * @return Returns TRUE is the last VCPU's context was saved, FALSE otherwise.
 */
bool_t cpuIsVCPUSaved(const void* kpVCpu);

/**
 * @brief Prints the virtual CPU core dump.
 *
 * @details Prints the virtual CPU core dump. This will print the stack
 * trace to the console.
 *
 * @param[in] kpVCpu The VCPU to use.
 */
void cpuCoreDump(const void* kpVCpu);
#endif /* #ifndef __CPU_H_ */

/************************************ EOF *************************************/