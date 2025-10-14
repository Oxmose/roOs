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
%define VCPU_OFF_SYSCALL_RSP 0x2E8
%define VCPU_OFF_KERNEL_STACK_END 0x2F0

;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------
extern syscallHandle
extern schedGetCurrentThread
extern cpuSwitchKernelSyscallContext
extern cpuRestoreKernelSyscallContext

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global cpuSystemCallInit
global cpuUserSyscallHandler
global cpuKernelSyscallRaise

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .text
;-------------------------------------------------------------------------------
; Initilializes the CPU registers for the system calls.
;
; Param:
;    Input: rdi: The system call main handler function for user system calls.
;           rsi: The system call kernel selector
;           rdx: the system call user selector
;
cpuSystemCallInit:
    ; Setup segments
    mov rax, rsi
    and rax, 0xFFFF
    and rdx, 0xFFFF
    or  rdx, 3
    shl rdx, 16
    or  rdx, rax
    xor rax, rax
    mov rcx, 0xC0000081
    wrmsr

    ; Setup the entry RIP
    mov rax, rdi
    mov rdx, rdi
    shr rdx, 32
    mov rcx, 0xC0000082
    wrmsr

    ; Setup the flags
    mov rax, 0
    mov rdx, 0
    mov rcx, 0xC0000084
    wrmsr

    ret

;-------------------------------------------------------------------------------
; Handles a user system call
;
; Param:
;     Input: rdi: The systen call ID
;            rsi: A pointer to the system call parameters.
cpuUserSyscallHandler:
    cli

    ; Save the user stack
    push r11
    push rcx
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    ; Get the current thread handle and VCPU
    push rdi
    push rsi
    call schedGetCurrentThread
    pop rsi
    pop rdi
    mov r12, rax
    mov r13, rax
    mov r12, [r12]

    ; Update to the kernel stack
    mov  rbx, rsp
    mov  rsp, [r12 + VCPU_OFF_KERNEL_STACK_END]
    push rbx
    mov  rbx, __cpuUserSyscallReturn
    push rbx
    mov  [r12 + VCPU_OFF_SYSCALL_RSP], rsp

    ; Call the main C handler
    sti
    call syscallHandle

    ; Discard the scheduler return that was pushed in case the call generated a
    ; scheduling.
    add rsp, 8

    ; Get the current thread handle and VCPU
    push rdi
    push rsi
    call schedGetCurrentThread
    pop rsi
    pop rdi
    mov rax, [rax]

    ; Clear the syscall stack registration
    xor rbx, rbx
    mov [rax + VCPU_OFF_SYSCALL_RSP], rbx


__cpuUserSyscallReturn:
    cli

    ; Restore the user stack
    pop rax
    mov rsp, rax

    ; Restore the user stack
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop rcx
    pop r11

    ; Return from syscall
    o64 sysret


;-------------------------------------------------------------------------------
; Raise a kernel space system call.
;
; Param:
;     Input: rdi: The system call handler function address.
;            rsi: A pointer to the system call parameters.
;            rdx: The current thread
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
    mov r14, rdx

    ; Save the cpu context in the case the syscall is blocking
    mov rdi, __cpuKernelSyscallReturn
    mov rsi, rdx
    call cpuSwitchKernelSyscallContext

    ; Restore parameter context
    mov rdi, r13

    ; Call the main system call handler with the right parameters
    call r12

    ; If we returned, restore the context
    mov rdi, r14
    call cpuRestoreKernelSyscallContext

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