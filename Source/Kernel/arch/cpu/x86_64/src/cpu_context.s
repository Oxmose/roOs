;-------------------------------------------------------------------------------
;
; File: cpu_context.s
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 16/06/2024
;
; Version: 1.0
;
; CPU context manager for i386
; Allows saving and restoring the CPU context.
;
;
;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
; ARCH
;-------------------------------------------------------------------------------
[bits 64]

;-------------------------------------------------------------------------------
; DEFINES
;-------------------------------------------------------------------------------

; The following define shall correspond to the first TSS entry in the GDT
%define FIRST_GDT_TSS_SEGMENT 0x30

; The following defines shall correspond to the virtual CPU context
%define VCPU_OFF_INT 0x00
%define VCPU_OFF_ERR 0x08
%define VCPU_OFF_RIP 0x10
%define VCPU_OFF_CS  0x18
%define VCPU_OFF_FLG 0x20
%define VCPU_OFF_RSP 0x28
%define VCPU_OFF_RBP 0x30
%define VCPU_OFF_RDI 0x38
%define VCPU_OFF_RSI 0x40
%define VCPU_OFF_RDX 0x48
%define VCPU_OFF_RCX 0x50
%define VCPU_OFF_RBX 0x58
%define VCPU_OFF_RAX 0x60
%define VCPU_OFF_R8  0x68
%define VCPU_OFF_R9  0x70
%define VCPU_OFF_R10 0x78
%define VCPU_OFF_R11 0x80
%define VCPU_OFF_R12 0x88
%define VCPU_OFF_R13 0x90
%define VCPU_OFF_R14 0x98
%define VCPU_OFF_R15 0xA0
%define VCPU_OFF_SS  0xA8
%define VCPU_OFF_GS  0xB0
%define VCPU_OFF_FS  0xB8
%define VCPU_OFF_ES  0xC0
%define VCPU_OFF_DS  0xC8
%define VCPU_OFF_FXD 0xD0

%define VCPU_OFF_SAVED 0x2E0


;-------------------------------------------------------------------------------
; MACRO DEFINE
;-------------------------------------------------------------------------------

; None

;-------------------------------------------------------------------------------
; EXTERN DATA
;-------------------------------------------------------------------------------
extern pCurrentThreadsPtr

;-------------------------------------------------------------------------------
; EXTERN FUNCTIONS
;-------------------------------------------------------------------------------
extern kernelPanic

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global cpuSaveContext
global cpuRestoreContext
global cpuGetId

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .text
cpuSaveContext:
    ; Save a bit of context
    push rax
    push rbx
    push rdx

    ; Get the current thread handle and VCPU
    mov rbx, 8
    call cpuGetId
    mul rbx
    mov rbx, pCurrentThreadsPtr
    add rax, rbx
    mov rax, [rax]
    mov rax, [rax]

    ; Restore RDX used with mul
    pop rdx

    ; Save the interrupt context
    mov rbx, [rsp + 24]               ; Int id
    mov [rax + VCPU_OFF_INT], rbx
    mov rbx, [rsp + 32]               ; Int code
    mov [rax + VCPU_OFF_ERR], rbx
    mov rbx, [rsp + 40]               ; RIP
    mov [rax + VCPU_OFF_RIP], rbx
    mov rbx, [rsp + 48]               ; CS
    mov [rax + VCPU_OFF_CS], rbx
    mov rbx, [rsp + 56]               ; RFLAGS
    mov [rax + VCPU_OFF_FLG], rbx
    mov rbx, [rsp + 64]               ; RSP
    mov [rax + VCPU_OFF_RSP], rbx
    mov rbx, [rsp + 72]               ; SS
    mov [rax + VCPU_OFF_SS], rbx

    mov [rax + VCPU_OFF_RBP], rbp
    mov [rax + VCPU_OFF_RDI], rdi
    mov [rax + VCPU_OFF_RSI], rsi
    mov [rax + VCPU_OFF_RDX], rdx
    mov [rax + VCPU_OFF_RCX], rcx
    pop rbx                         ; restore prelude rbx
    mov [rax + VCPU_OFF_RBX], rbx
    pop rbx                         ; restore prelude rax
    mov [rax + VCPU_OFF_RAX], rbx

    mov [rax + VCPU_OFF_R8],  r8
    mov [rax + VCPU_OFF_R9],  r9
    mov [rax + VCPU_OFF_R10], r10
    mov [rax + VCPU_OFF_R11], r11
    mov [rax + VCPU_OFF_R12], r12
    mov [rax + VCPU_OFF_R13], r13
    mov [rax + VCPU_OFF_R14], r14
    mov [rax + VCPU_OFF_R15], r15

    mov [rax + VCPU_OFF_GS], gs
    mov [rax + VCPU_OFF_FS], fs
    mov [rax + VCPU_OFF_ES], es
    mov [rax + VCPU_OFF_DS], ds

    ; Save the FxData
    mov rbx, rax
    add rbx, VCPU_OFF_FXD
    add rbx, 0xF                   ; ALIGN Region
    and rbx, 0xFFFFFFFFFFFFFFF0    ; ALIGN Region
    fxsave [rbx]

    ; Set the last context as saved
    mov rbx, 1
    mov [rax + VCPU_OFF_SAVED], rbx

    ret

cpuRestoreContext:
    ; The current thread is sent as parameter, load the VCPU
    mov rax, [rdi]

    ; Set the last context as not been saved
    mov rbx, 0
    mov [rax + VCPU_OFF_SAVED], rbx

    ; Restore the FxData
    mov rbx, rax
    add rbx, VCPU_OFF_FXD
    add rbx, 0xF                   ; ALIGN Region
    and rbx, 0xFFFFFFFFFFFFFFF0    ; ALIGN Region
    fxrstor [rbx]

    ; Restore registers
    mov gs, [rax + VCPU_OFF_GS]
    mov fs, [rax + VCPU_OFF_FS]
    mov es, [rax + VCPU_OFF_ES]
    mov ds, [rax + VCPU_OFF_DS]

    mov r8,  [rax + VCPU_OFF_R8]
    mov r9,  [rax + VCPU_OFF_R9]
    mov r10, [rax + VCPU_OFF_R10]
    mov r11, [rax + VCPU_OFF_R11]
    mov r12, [rax + VCPU_OFF_R12]
    mov r13, [rax + VCPU_OFF_R13]
    mov r14, [rax + VCPU_OFF_R14]
    mov r15, [rax + VCPU_OFF_R15]

    mov rbp, [rax + VCPU_OFF_RBP]
    mov rdi, [rax + VCPU_OFF_RDI]
    mov rsi, [rax + VCPU_OFF_RSI]
    mov rdx, [rax + VCPU_OFF_RDX]
    mov rcx, [rax + VCPU_OFF_RCX]

    ; Restore stack pointer pre-int and make space of IRET pop data
    mov rsp, [rax + VCPU_OFF_RSP]
    sub rsp, 40

    ; Restore the interrupt context
    mov rbx, [rax + VCPU_OFF_RIP]  ; RIP
    mov [rsp], rbx
    mov rbx, [rax + VCPU_OFF_CS]  ; CS
    mov [rsp + 8], rbx
    mov rbx, [rax + VCPU_OFF_FLG]  ; RFLAGS
    mov [rsp + 16], rbx

    ; Retore base RSP
    mov rbx, rsp
    add rbx, 40
    mov [rsp + 24], rbx  ; RSP
    mov rbx, [rax + VCPU_OFF_SS]  ; SS
    mov [rsp + 32], rbx

    ; Restore RBX and RAX
    mov rbx, [rax + VCPU_OFF_RBX]
    mov rax, [rax + VCPU_OFF_RAX]

    ; Return from interrupt
    iretq

cpuGetId:
    ; Get the TSS segment
    str rax
    sub rax, FIRST_GDT_TSS_SEGMENT
    shr rax, 4
    ret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------