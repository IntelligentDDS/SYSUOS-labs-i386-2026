[bits 16]
org 0x7c00

xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7c00

mov ax, 0x0003
int 0x10

mov word [sx], 40
mov word [sy], 12
mov byte [d], 0
mov word [fx], 50
mov word [fy], 15

loop:
call cls
mov ax, [sx]
mov bx, [sy]
mov dl, 'O'
call dc
mov ax, [fx]
mov bx, [fy]
mov dl, '*'
call dc

call ci
call ms
call ce
call delay
jmp loop

cls:
mov ax, 0xb800
mov es, ax
xor di, di
mov cx, 2000
mov ax, 0x0720
rep stosw
ret

dc:
push ax
push bx
push bp
mov bp, sp
mov cx, 80
mov ax, [bp+2]  ; y
mul cx          ; ax = y * 80
add ax, [bp+4]  ; add x
shl ax, 1
mov di, ax
mov ax, 0xb800
mov es, ax
mov al, dl
mov ah, 0x0A
stosw
pop bp
pop bx
pop ax
ret

ci:
mov ah, 1
int 0x16
jz nk
mov ah, 0
int 0x16
cmp al, 'w'
je u
cmp al, 's'
je dn
cmp al, 'a'
je l
cmp al, 'd'
je r
jmp nk
u: mov byte [d], 3
jmp nk
dn: mov byte [d], 1
jmp nk
l: mov byte [d], 2
jmp nk
r: mov byte [d], 0
jmp nk
nk: ret

ms:
mov al, [d]
cmp al, 0
je mr
cmp al, 1
je md
cmp al, 2
je ml
cmp al, 3
je mu
mr: inc word [sx]
jmp mdn
md: inc word [sy]
jmp mdn
ml: dec word [sx]
jmp mdn
mu: dec word [sy]
jmp mdn
mdn:
mov ax, [sx]
cmp ax, 80
jge go
cmp ax, 0
jl go
mov ax, [sy]
cmp ax, 25
jge go
cmp ax, 0
jl go
ret
go: jmp $

ce:
mov ax, [sx]
cmp ax, [fx]
jne ne
mov ax, [sy]
cmp ax, [fy]
jne ne
mov ax, [sx]
add ax, 10
cmp ax, 80
jl ox
sub ax, 20
ox: mov [fx], ax
mov ax, [sy]
add ax, 5
cmp ax, 25
jl oy
sub ax, 10
oy: mov [fy], ax
ne: ret

delay:
mov cx, 0xFFFF
dloop: loop dloop
ret

sx: dw 0
sy: dw 0
d: db 0
fx: dw 0
fy: dw 0

times 510 - ($ - $$) db 0
dw 0xAA55