org 0x7C00
bits 16

%define ENDL 0x0D,0x0A

;
; FAT12 header record
;

jmp short start
nop

bdb_oem:                            db 'MSWIN4.1'
bdb_bytes_per_sector:               dw 512
bdb_sectors_per_cluster:            db 1
bdb_reserved_sectors:               dw 1 
bdb_fat_count:                      db 2
bdb_dir_entries_count:              dw 0E0h
bdb_total_sectors:                  dw 2880
bdb_media_descriptor_type:          db 0F0h
bdb_sectors_per_fat:                dw 9
bdb_sectors_per_track:              dw 18
bdb_heads:                          dw 2
bdb_hidden_sectors:                 dd 0
bdb_large_sector_count:             dd 0

ebr_drive_number:                   db 0
                                    db 0
ebr_signature:                      db 0x29
ebr_volume_id:                      db 0x01,0x02,0x03,0x04
ebr_volume_label:                   db 'InfoSecIITR'
ebr_system_id:                      db 'FAT12   '

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

    ; Read something from floppy disk
    ; BIOS should set `dl` to drive number
    mov [ebr_drive_number], dl

    mov ax, 1                           ; LBA = 1, second sector from disk
    mov cl, 1                           ; 1 sector to read
    mov bx, 7E00h                       ; data should be after the bootloader
    call disk_read

    ; Print the boot message
    mov si, msg_boot
    call puts

    hlt

floppy_error:
    mov si, msg_read_fail
    call puts
    jmp wait_key_and_reboot

wait_key_and_reboot:
    mov ah, 0
    int 016h                            ; wait for key press

    jmp 0FFFFh:0                        ; jump to the beginning of bios.
                                        ; This will reboot the system 

    hlt

.halt:
    cli                                 ; disable interrupts
                                        ; THis way CPU cant get out of halt state
    hlt

; Converts an LBA address to CHS address
; Parameters:
;   - ax : LBA address
; Returns:
;   - cx [0-5] : sector number
;   - cx [6-15]: cyliinder
;   - dh       : head

lba_to_chs:

    push ax
    push dx

    xor dx, dx
    div word [bdb_sectors_per_track]    ; `ax` = lba / sectors per track
                                        ; `dx` = lba % sectors per track
    inc dx                              ; `dx` = lba % sectors per track + 1 = sector
    mv  cx, dx

    xor dx, dx
    div word [bdb_heads]                ; `ax` = (lba / sectors per track) / heads
                                        ; `dx` = (lba / sectors per track) % heads
    mov dh, dl
    mov ch, al
    shl ah, 06
    or  cl, ah

    pop ax
    mov dl, al
    pop ax


; Reads sectors from disk
; Parameters:
;   - ax : LBA address
;   - cl : number of sectors to read
;   - dl : drive number
;   - es:bx : memory address where to store read data

disk_read:

    push ax                             ; Save registers on the stack
    push bx
    push cx
    push dx
    push di

    push cx                             ; save `cl` -> number of sectors per thread
    call lba_to_chs
    pop ax                              ; number of sectors per thread
    
    mov ah, 02h
    mov di, 3                           ; loop count variable

; floppy being unreliable the call may not always be sucessful. 
; Hence operations are performed multiple times.
.retry:                                 
    pusha                               ; push all registers on stack
    stc                                 ; store carry flag
    int 013h                            ; if the call is sucessful carry flag is set to 0
    jnc .done                           ; jmp if call not succeful

    popa
    call disk_reset

    dec di
    test di, di
    jnz .retry

; If operation failed every time
.fail:
    jmp floppy_error

.done:
    popa

    push di
    push dx
    push cx
    push bx
    push ax                             ; restore modified registers
    ret
    
;
; Resets disk controller
; Parameters:
;   -dl : drive number
;
disk_reset:
    pusha
    mov ah, 0
    stc
    int 13h
    
    jc floppy_error
    popa
    ret

msg_boot: db "Hello World!",ENDL,0
msg_read_fail: db "Read from disk failed",ENDL,0

times 510-($-$$) db 0
dw 0AA55h