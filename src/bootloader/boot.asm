org 0x7C00
bits 16

%define ENDL 0x0D,0x0A

start:
    jmp main

; Print string to the screen
; Params:
;   - ds:si points to a string

puts:
    push si
    push ax

.loop:
    lodsb       ; Load byte into the `al` register
    or al, al   ; Check to see if value is 0. If it is 0 then `flags` register is set
    jz .done

    mov ah, 0x0e
    mov bh, 0x0
    int 0x10


    jmp .loop

.done:
    pop ax
    pop si
    ret

main:
    ; Initialise the value of `ds` and `es` registers
    mov ax, 0
    mov ds, ax
    mov es, ax

    ; Setup the program stack
    mov ss, ax
    mov sp, 0x7C00
    mov si, msg_boot
    call puts

    hlt

.halt:
    jmp .halt

msg_boot: db "Hello World!",ENDL,0

times 510-($-$$) db 0
dw 0AA55h