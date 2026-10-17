[BITS 16]
[ORG 0x7C00]

start:
    ; 1. Инициализация
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; 2. Загружаем таблицу GDT (Global Descriptor Table)
    ; Это "паспорт", который разрешает процессору перейти в 32-битный режим
    lgdt [gdt_descriptor]

    ; 3. Включаем 32-битный режим (Protected Mode)
    mov eax, cr0
    or eax, 0x1        ; Устанавливаем первый бит в 1
    mov cr0, eax

    ; 4. Совершаем дальний прыжок (Far Jump), чтобы сбросить конвейер процессора
    ; и перейти к 32-битному коду
    jmp CODE_SEG:init_pm

; ========================================================
; НАЧАЛО 32-БИТНОГО КОДА
; ========================================================
[BITS 32]

init_pm:
    ; Настраиваем сегментные регистры для 32-битного режима
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Настраиваем стек
    mov ebp, 0x90000
    mov esp, ebp

    ; Выводим текст в видеопамять (0xB8000)
    mov ebx, 0xB8000    ; Адрес видеопамяти
    mov ecx, msg_pm     ; Наш текст
    mov ah, 0x0A        ; Светло-зеленый на черном

print_loop_32:
    mov al, [ecx]
    test al, al
    jz hang
    mov [ebx], ax       ; Записываем символ
    add ebx, 2          ; Следующая ячейка (2 байта)
    inc ecx             ; Следующий символ
    jmp print_loop_32

hang:
    jmp $               ; Бесконечный цикл

; ========================================================
; ДАННЫЕ И GDT
; ========================================================
msg_pm db 'DarkOS: 32-bit Protected Mode Activated!', 0

; Описание GDT (стандартные сегменты)
gdt_start:
    dq 0x0000000000000000 ; Нулевой дескриптор (обязателен)
gdt_code:
    dq 0x00CF9A000000FFFF ; Дескриптор кода (Code Segment)
gdt_data:
    dq 0x00CF92000000FFFF ; Дескриптор данных (Data Segment)
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1 ; Размер GDT
    dd gdt_start               ; Адрес GDT

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

; --- МАГИЯ ЗАГРУЗОЧНОГО СЕКТОРА ---
times 510 - ($ - $$) db 0
dw 0xAA55

; --- РАСКАЧИВАЕМ ДО 1.44 МБ ДЛЯ VECTRAS ---
times 1474560 - ($ - $$) db 0
