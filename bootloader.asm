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

    mov si, msg_hello
    call print_string

hang:
    jmp hang

print_string:
    lodsb
    or al, al
    jz .done
    mov dx, 0x3F8
    out dx, al
    jmp print_string
.done:
    ret

msg_hello db 'DarkOS booting...', 13, 10, 0

times 510 - ($ - $$) db 0
dw 0xAA55
