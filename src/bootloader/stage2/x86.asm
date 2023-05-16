bits 16

section _TEXT class=CODE

global _x86_Video_WriteCharTeletype

;
; Parameters:
;   @characters:
;   @page
;
_x86_Video_WriteCharTeletype:

    ; make the call frame
    push bp
    mov bp, sp

    push bx

    ; [bp + 0] - old call frame
    ; [bp + 2] - return address (small memory model => 2 bytes)
    ; [bp + 4] - first argument (character)
    ; [bp + 6] - second argument (page)

    mov ah, 0xE
    mov al, [bp + 4]
    mov bh, [bp + 6]

    int 10h

    ; restore call frame
    pop bx
    mov bp, sp
    pop bp

    ret