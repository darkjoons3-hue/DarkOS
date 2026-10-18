[BITS 16]
[ORG 0x7C00]

start:
    ; Инициализация COM1 (для отладки)
    mov dx, 0x3FB
    mov al, 0x80
    out dx, al
    mov dx, 0x3F8
    mov al, 0x03
    out dx, al
    mov dx, 0x3F9
    mov al, 0x00
    out dx, al
    mov dx, 0x3FB
    mov al, 0x03
    out dx, al
    mov dx, 0x3FA
    mov al, 0xC7
    out dx, al
    mov dx, 0x3FC
    mov al, 0x0B
    out dx, al

    ; Выводим 'S'
    mov dx, 0x3F8
    mov al, 'S'
    out dx, al

    ; Пытаемся прочитать ядро
    mov ah, 0x02
    mov al, 2        ; 2 сектора
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0        ; <--- ВОЗВРАЩАЕМ ДИСКОВОД 0 (наша дискета)
    mov bx, 0x7E00
    int 0x13
    jc disk_error

    ; Выводим 'R'
    mov dx, 0x3F8
    mov al, 'R'
    out dx, al

    ; Переключаемся в 32-бит
    lgdt [gdt_descriptor]
    cli
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp dword CODE_SEG:init_pm

disk_error:
    ; Выводим 'E'
    mov dx, 0x3F8
    mov al, 'E'
    out dx, al
    jmp $

[BITS 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebp, 0x90000
    mov esp, ebp

    ; Выводим 'K'
    mov dx, 0x3F8
    mov al, 'K'
    out dx, al

    ; Прыгаем в ядро
    jmp 0x7E00

gdt_start:
    dq 0x0000000000000000
gdt_code:
    dq 0x00CF9A000000FFFF
gdt_data:
    dq 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

times 510 - ($ - $$) db 0
dw 0xAA55
