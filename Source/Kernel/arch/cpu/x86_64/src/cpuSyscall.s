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
extern cpuSaveSyscallContext
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
cpuKernelSyscallRaise:
    push r12
    push r13
    push rbp

    ; Switch to kernel stack and save old stack
    mov r12, rsp
    mov rsp, rdx
    push r12

    ; Save if the current context shall be saved
    mov r12, rdi
    mov r13, rsi

    ; Save the cpu context in the case the syscall is blocking
    mov rdi, __cpuKernelSyscallReturn
    call cpuSaveSyscallContext

    ; Restore parameter context
    mov rdi, r13

__cpuKernelSyscallNoSave:
    ; Call the main system call handler with the right parameters
    call r12

__cpuKernelSyscallReturn:
    ; Switch to user stack
    pop rax
    mov rsp, rax

    ; Restore the saved stack context
    pop rbp
    pop r13
    pop r12

    ; Return
    ret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------

;************************************ EOF **************************************