;-------------------------------------------------------------------------------
;
; File: cpu_sync.S
;
; Author: Alexy Torres Aurora Dugo
;
; Date: 25/04/2023
;
; Version: 1.0
;
; CPU synchronization functions
;-------------------------------------------------------------------------------
;-------------------------------------------------------------------------------
; ARCH
;-------------------------------------------------------------------------------
[bits 32]

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

;-------------------------------------------------------------------------------
; EXPORTED FUNCTIONS
;-------------------------------------------------------------------------------
global cpu_lock_spinlock
global cpu_unlock_spinlock

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .text
;-------------------------------------------------------------------------------
; Lock Spinlock
;
; Param:
;     Input: ESP + 4: Address of the lock

cpu_lock_spinlock:
    push eax
    mov  eax, [esp + 8]

__pause_spinlock_entry:
    lock bts dword [eax], 0
    jc   __pause_spinlock_pause

    pop eax
    ret

__pause_spinlock_pause:
    pause
    test  dword [eax], 1
    jnz   __pause_spinlock_pause
    jmp   __pause_spinlock_entry

;-------------------------------------------------------------------------------
; Unlock Spinlock
;
; Param:
;     Input: ESP + 4: Address of the lock

cpu_unlock_spinlock:
    push eax
    mov  eax, [esp + 8]
    mov  dword [eax], 0
    pop  eax
    ret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------
