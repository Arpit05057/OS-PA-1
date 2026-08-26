bits 16
org 0x7c00

start:
    mov si, message

print:
    lodsb
    cmp al, 0
    je hang

    mov ah, 0x0e
    int 0x10

    jmp print

hang:
    cli
    hlt
    jmp hang

message:
    db "Hello from my bootloader!", 0

times 510-($-$$) db 0
dw 0xaa55
