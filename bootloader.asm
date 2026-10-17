[BITS 16]
[ORG 0x7C00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Указываем на видеопамять (0xB8000)
    mov ax, 0xB800
    mov es, ax
    xor di, di          ; Начало видеопамяти
    mov si, msg_hello
    mov ah, 0x0A        ; Светло-зеленый текст на черном

.print_loop:
    lodsb               ; Берем символ
    test al, al         ; Проверяем на конец строки
    jz .hang            ; Если конец - зависаем
    mov [es:di], ax     ; Пишем символ и цвет в память
    add di, 2           ; Сдвигаем курсор
    jmp .print_loop

.hang:
    jmp $

msg_hello db 'DarkOS booting...', 0

times 510 - ($ - $$) db 0
dw 0xAA55
