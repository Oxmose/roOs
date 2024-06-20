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

; None

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

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .text
cpuSaveContext:
        ; At entry, RIP was pushed by the call to this function

        ; Save a bit of context
        push rax
        push rbx
        push rdx

        ; Get the current thread handle and VCPU
        mov rax, 8
        mov rbx, gs ; GS stores the CPU id
        mul rbx
        mov rbx, pCurrentThreadsPtr
        add rax, rbx
        mov rax, [rax]
        mov rax, [rax]

        ; Restore RDX used with mul
        pop rdx

        ; Save the interrupt context
        mov rbx, [rsp+24]  ; Int id
        mov [rax], rbx
        mov rbx, [rsp+32]  ; Int code
        mov [rax+8], rbx
        mov rbx, [rsp+40]  ; RIP
        mov [rax+16], rbx
        mov rbx, [rsp+48]  ; CS
        mov [rax+24], rbx
        mov rbx, [rsp+56]  ; RFLAGS
        mov [rax+32], rbx
        mov rbx, [rsp+64]  ; RSP
        mov [rax+40], rbx
        mov rbx, [rsp+72]  ; SS
        mov [rax+168], rbx

        mov [rax+48], rbp
        mov [rax+56], rdi
        mov [rax+64], rsi
        mov [rax+72], rdx
        mov [rax+80], rcx

        pop rbx             ; restore prelude rbx
        mov [rax+88], rbx
        pop rbx             ; restore prelude rax
        mov [rax+96], rbx

        mov [rax+104], r8
        mov [rax+112], r9
        mov [rax+120], r10
        mov [rax+128], r11
        mov [rax+136], r12
        mov [rax+144], r13
        mov [rax+152], r14
        mov [rax+160], r15

        mov [rax+176], gs
        mov [rax+184], fs
        mov [rax+192], es
        mov [rax+200], ds

        ret

cpuRestoreContext:
        ; The current thread is sent as parameter, load the VCPU
        mov rax, [rdi]

        ; Restore registers
        mov ss, [rax+168]
        ; mov gs, [rax+176]
        mov fs, [rax+184]
        mov es, [rax+192]
        mov ds, [rax+200]

        mov r8,  [rax+104]
        mov r9,  [rax+112]
        mov r10, [rax+120]
        mov r11, [rax+128]
        mov r12, [rax+136]
        mov r13, [rax+144]
        mov r14, [rax+152]
        mov r15, [rax+160]

        mov rbp, [rax+48]
        mov rdi, [rax+56]
        mov rsi, [rax+64]
        mov rdx, [rax+72]
        mov rcx, [rax+80]

        ; Restore stack pointer pre-int and make space of IRET pop data
        mov rsp, [rax+40]
        sub rsp, 40

        ; Restore the interrupt context
        mov rbx, [rax+16]  ; RIP
        mov [rsp], rbx
        mov rbx, [rax+24]  ; CS
        mov [rsp+8], rbx
        mov rbx, [rax+32]  ; RFLAGS
        mov [rsp+16], rbx

        ; Retore base RSP
        mov rbx, rsp
        add rbx, 40
        mov [rsp+24], rbx  ; RSP
        mov [rsp+32], ss   ; SS

        mov rbx, [rax+88]
        mov rax, [rax+96]

        ; Return from interrupt
        iretq

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------