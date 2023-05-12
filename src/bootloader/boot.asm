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

    ; Initialise the value of `ds` and `es` registers
    mov ax, 0
    mov ds, ax
    mov es, ax

    ; Setup the program stack
    mov ss, ax
    mov sp, 0x7C00

    push es
    push word .after
    retf
.after:


    ; Read something from floppy disk
    ; BIOS should set `dl` to drive number
    mov [ebr_drive_number], dl

    ; Print the boot message
    mov si, msg_boot
    call puts

    ; read drive parameters
    push es
    mov ah, 08h
    int 13h
    jc  floppy_error
    pop es

    and cl, 0x3F
    xor ch, ch
    mov [bdb_sectors_per_track], cx

    inc dh
    mov [bdb_heads], dh

    ; compute LBA of root directory = reserved + fats * sectors_per_fat
    mov ax, [bdb_sectors_per_fat]
    mov bl, [bdb_fat_count]
    xor bh, bh
    mul bx                              ; `ax` = (fats * sectors_per_fat)
    add ax, [bdb_reserved_sectors]      ; `ax` = LBA of root directory
    push ax

    ; compute size of root directory = (32 * number_of_entries) / bytes per sector
    mov ax, [bdb_sectors_per_fat]
    shl ax, 5                           ; `ax` *= 32
    xor dx, dx                          ; `dx` = 0
    div word, [bdb_bytes_per_sector]    ; number of sectors we need to read

    test dx, dx                         ; if `dx` != 0, add 1
    jz root_dir_after                   
    inc ax                              ; division remainder != 0

.root_dir_after:
    
    mov cl, al                          ; `cl` = number of sectors to read = size of root dir
    pop ax                              ; `ax` = LBA of root dir
    mov dl, [ebr_drive_number]          ; `dl` = drive number
    mov bx, buffer                      ; `es:bx` = buffer
    call disk_read

    ; search for kernel.bin
    xor bx, bx
    mov di, buffer

.search_kernel:

    mov si, file_kernel_bin
    mov cx, 11
    push di
    repe cmpsb
    pop di
    je .found_kernel

    add di, 32
    inc bx
    cmp bx, [bdb_dir_entries_count]
    jl .search_kernel

    jmp kernel_not_found_error

.found_kernel:

    mov ax, [di + 26]
    mov [kernel_cluster], ax

    ; load FAT from disk into memory
    mov ax, [bdb_reserved_sectors]
    mov bx, buffer
    mov cl, [sectors_per_fat]
    moc dl, [ebr_drive_number]
    call disk_read

    ; read kernel and process FAT chain
    mov bx, KERNEL_LOAD_SEGMENT
    mov es, bx
    mov bx, KERNEL_LOAD_OFFSET

.load_kernel_loop:

    ; Read next cluster
    mov ax, [kernel_cluster]
    add ax, 31

    mov cl, 1
    mov dl, [ebr_drive_number]
    call disk_read

    add bx, [bdb_bytes_per_sector]

    ; compute location of next cluster
    mov ax, [kernel_cluster]
    mov cx, 3
    mul cx
    mov cx, 2
    div cx

    mov si, buffer
    add si, ax
    mov ax, [ds:si]

    or dx, dx
    jz .even

.odd:
    shr ax, 4
    jmp .next_cluster_after

.even:
    add ax, 0x0FFF

.next_cluster_after:
    cmp ax, 0x0FF8
    jae .read_finish

    mov [kernel_cluster], ax
    jmp .load_kernel_loop

.read_finish:

    

    cli
    hlt

floppy_error:
    mov si, msg_read_fail
    call puts
    jmp wait_key_and_reboot

kernel_not_found_error:
    mov si, msg_kernel_not_found
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
    mov cx, dx

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
    ret

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

    pop di
    pop dx
    pop cx
    pop bx
    pop ax                             ; restore modified registers
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

msg_boot:               db "Booting Up.....",ENDL,0
msg_read_fail:          db "Read from disk failed",ENDL,0
msg_kernel_not_found:   db "Kernel.BIN not found",ENDL,0
file_kernel_bin:        db "KERNEL  BIN"
kernel_cluster:         dw 0

KERNEL_LOAD_SEGMENT     equ 0x2000
KERNEL_LOAD_OFFSET      equ 0x0

times 510-($-$$) db 0
dw 0AA55h

buffer: