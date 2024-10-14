;-------------------------------------------------------------------------------
;
; File: cpuSyscall.S
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 12/10/2023
;
; Version: 1.0
;
; CPU system call manager
;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
; ARCH
;-------------------------------------------------------------------------------
[bits 64]

;-------------------------------------------------------------------------------
; DEFINES
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------
extern cpuSwitchKernelSyscallContext
extern cpuRestoreSyscallContext

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global cpuKernelSyscallRaise

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .text
;-------------------------------------------------------------------------------
; Raise a kernel space system call.
;
; Param:
;     Input: rdi: The system call handler function address.
;            rsi: A pointer to the system call parameters.
;            rdx: The kernel stack to use for the call.
;            rcx: The current thread
cpuKernelSyscallRaise:
    push rbx
    push rdx
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov r12, rdi
    mov r13, rsi
    mov r14, rcx

    ; Save the cpu context in the case the syscall is blocking
    mov rdi, __cpuKernelSyscallReturn
    mov rsi, rcx
    call cpuSwitchKernelSyscallContext

    ; Restore parameter context
    mov rdi, r13

__cpuKernelSyscallNoSave:
    ; Call the main system call handler with the right parameters
    call r12

    ; If we returned, restore the context
    mov rdi, r14
    call cpuRestoreSyscallContext

__cpuKernelSyscallReturn:
    ; Restore the saved stack context
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rdx
    pop rbx

    ; Return
    ret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------

;************************************ EOF **************************************