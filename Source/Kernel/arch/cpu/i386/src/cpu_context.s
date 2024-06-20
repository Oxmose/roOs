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
[bits 32]

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
        ; At entry, EIP was pushed by the call to this function

        ; Save a bit of context
        push eax
        push ebx
        push edx

        ; Get the current thread handle
        mov eax, 4
        mov ebx, gs ; GS stores the CPU id
        mul ebx
        mov ebx, pCurrentThreadsPtr
        add eax, ebx
        mov eax, [eax]
        mov eax, [eax]

        ; Restore EDX used with mul
        pop edx

        ; Save the interrupt context
        mov ebx, [esp+12]  ; Int id
        mov [eax], ebx
        mov ebx, [esp+16]  ; Int code
        mov [eax+4], ebx
        mov ebx, [esp+20]  ; EIP
        mov [eax+8], ebx
        mov ebx, [esp+24]  ; CS
        mov [eax+12], ebx
        mov ebx, [esp+28]  ; EFLAGS
        mov [eax+16], ebx

        ; Save CPU state before calling interrupt
        mov ebx, esp       ; ESP before int save
        add ebx, 20
        mov [eax+20], ebx

        mov [eax+24], ebp
        mov [eax+28], edi
        mov [eax+32], esi
        mov [eax+36], edx
        mov [eax+40], ecx

        pop ebx             ; restore prelude ebx
        mov [eax+44], ebx
        pop ebx             ; restore prelude eax
        mov [eax+48], ebx

        mov [eax+52], ss
        mov [eax+56], gs
        mov [eax+60], fs
        mov [eax+64], es
        mov [eax+68], ds

        ret

cpuRestoreContext:
        ; The current thread is sent as parameter
        mov eax, [esp + 4]

        ; Load the VCPU
        mov eax, [eax]

        ; Restore registers
        mov ds,  [eax+68]
        mov es,  [eax+64]
        mov fs,  [eax+60]
        ; mov gs,  [eax+56] GS should not be changed, it contains the CPU ID
        mov ss,  [eax+52]
        mov ecx, [eax+40]
        mov edx, [eax+36]
        mov esi, [eax+32]
        mov edi, [eax+28]
        mov ebp, [eax+24]
        mov esp, [eax+20]

        ; Restore the interrupt context
        mov ebx, [eax+8]  ; EIP
        mov [esp], ebx
        mov ebx, [eax+12]  ; CS
        mov [esp+4], ebx
        mov ebx, [eax+16]  ; EFLAGS
        mov [esp+8], ebx

        mov ebx, [eax+44]
        mov eax, [eax+48]

        ; Return from interrupt
        iret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------