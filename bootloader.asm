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

    ; 2. Загружаем ядро с дискеты в память (0x7E00)
    mov ah, 0x02     ; Функция BIOS: чтение секторов
    mov al, 9       ; Читаем 64 сектора (32 КБ), этого хватит с головой
    mov ch, 0        ; Цилиндр 0
    mov cl, 2        ; Начинаем со 2-го сектора (1-й занят загрузчиком)
    mov dh, 0        ; Головка 0
    mov dl, 0        ; Дисковод 0 (наша дискета)
    mov bx, 0x7E00   ; Адрес в памяти, куда грузим (ES:BX)
    int 0x13         ; Вызов BIOS
    jc disk_error    ; Если ошибка чтения (флаг переноса), прыгаем на ошибку

    ; 3. Загружаем таблицу GDT
    lgdt [gdt_descriptor]

    ; 4. Включаем 32-битный режим
    cli
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax

    ; 5. Прыжок в 32-бит
    jmp dword CODE_SEG:init_pm

disk_error:
    ; Если дискета не прочиталась, просто зависаем
    jmp $

; ========================================================
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

    ; ПЕРЕДАЕМ УПРАВЛЕНИЕ ЯДРУ (теперь оно точно там!)
    jmp 0x7E00

; ========================================================
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
