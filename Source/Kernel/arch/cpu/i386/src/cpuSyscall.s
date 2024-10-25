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
extern cpuRestoreKernelSyscallContext

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
;     Input: esp + 12: The system call handler function address.
;            esp + 8: A pointer to the system call parameters.
;            esp + 4: The current thread
cpuKernelSyscallRaise:
    push ebx
    push esi
    push edi
    push ebp

    ; Save the cpu context in the case the syscall is blocking
    mov eax, __cpuKernelSyscallReturn
    push eax
    mov eax, [esp + 20]
    push eax
    call cpuSwitchKernelSyscallContext
    add eax, 8

    ; Call the main system call handler with the right parameters
    mov eax, [esp + 24]
    push eax
    mov eax, [esp + 32]
    call eax

    ; If we returned, restore the context
    add esp, 4
    mov eax, [esp + 20]
    push eax
    call cpuRestoreKernelSyscallContext

__cpuKernelSyscallReturn:
    ; Restore the saved stack context
    pop ebp
    pop edi
    pop esi
    pop ebx

    ; Return
    ret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------

;************************************ EOF **************************************