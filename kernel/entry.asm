[BITS 32]
global _start
extern main

_start:
    ; Принудительно выравниваем стек по границе 16 байт
    mov esp, 0x90000
    and esp, 0xFFFFFFF0
    call main
    jmp $
