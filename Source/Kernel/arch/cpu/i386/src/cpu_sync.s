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
global spinlockAcquire
global spinlockRelease
global atomicIncrement32
global atomicDecrement32

;-------------------------------------------------------------------------------
; CODE
;-------------------------------------------------------------------------------

section .text
;-------------------------------------------------------------------------------
; Lock Spinlock
;
; Param:
;     Input: ESP + 4: Address of the lock

spinlockAcquire:
    mov  eax, [esp + 4]

__pauseSpinlockEntry:
    lock bts dword [eax], 0
    jc   __pauseSpinlockPause

    ret

__pauseSpinlockPause:
    pause
    test  dword [eax], 1
    jnz   __pauseSpinlockPause
    jmp   __pauseSpinlockEntry

;-------------------------------------------------------------------------------
; Unlock Spinlock
;
; Param:
;     Input: ESP + 4: Address of the lock

spinlockRelease:
    mov  eax, [esp + 4]
    mov  dword [eax], 0
    ret


;-------------------------------------------------------------------------------
; 32 Bits atomic increment
;
; Param:
;     Input: ESP + 4: u32_atomic_t value to increment
atomicIncrement32:
    mov  edx, [esp + 4]
    mov  eax, 1
    lock xadd dword [edx], eax
    ret

;-------------------------------------------------------------------------------
; 32 Bits atomic decrement
;
; Param:
;     Input: ESP + 4: u32_atomic_t value to decrement
atomicDecrement32:
    mov  edx, [esp + 4]
    mov  eax, -1
    lock xadd dword [edx], eax
    ret

;-------------------------------------------------------------------------------
; DATA
;-------------------------------------------------------------------------------

;************************************ EOF **************************************