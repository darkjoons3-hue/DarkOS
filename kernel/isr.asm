[BITS 32]
global keyboard_handler_asm
extern keyboard_handler

keyboard_handler_asm:
    pusha                   ; Сохраняем регистры

    ; Сохраняем старый указатель стека в EBP
    mov ebp, esp
    
    ; Выравниваем стек для C-функции
    and esp, 0xFFFFFFF0
    call keyboard_handler
    
    ; Восстанавливаем стек
    mov esp, ebp

    popa                    ; Восстанавливаем регистры
    iret                    ; Возвращаемся из прерывания
