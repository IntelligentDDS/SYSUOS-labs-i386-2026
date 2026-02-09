[bits 16]
org 0x7c00

; 简单的MBR，只负责加载后续扇区的snake程序
xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7c00

; 从软盘读取4个扇区（2KB）到0x7e00
mov ax, 0x0201      ; ah=2 (read), al=1 (1 sector)
mov ch, 0           ; cylinder 0
mov dh, 0           ; head 0
mov cl, 2           ; sector 2 (第2个扇区)
mov bx, 0x7e00      ; 目标地址
int 0x13

mov ax, 0x0203      ; 再读3个扇区
mov cl, 3
mov bx, 0x8000
int 0x13

; 跳转到snake程序
jmp 0x0000:0x7e00

times 510-($-$$) db 0
db 0x55, 0xaa
