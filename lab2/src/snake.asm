[bits 16]
org 0x7e00

; 贪吃蛇主程序（不受512字节限制）

start:
    ; 设置显示模式
    mov ax, 0x0003
    int 0x10
    
    ; 初始化段寄存器
    xor ax, ax
    mov ds, ax
    mov es, ax
    
    ; 初始化蛇
    mov word [len], 5
    mov byte [dir], 3      ; 3=向上
    mov word [fx], 50
    mov word [fy], 12
    
    ; 初始化蛇身（在屏幕中下方）
    mov word [sx], 40
    mov word [sx+2], 20
    mov word [sx+4], 40
    mov word [sx+6], 21
    mov word [sx+8], 40
    mov word [sx+10], 22
    mov word [sx+12], 40
    mov word [sx+14], 23
    mov word [sx+16], 40
    mov word [sx+18], 24
    
    ; 设置显存段
    mov ax, 0xb800
    mov es, ax

game_loop:
    ; 清屏
    push es
    xor di, di
    mov cx, 2000
    mov ax, 0x0720
    rep stosw
    pop es
    
    ; 画四周黄色边框
    call draw_border
    
    ; 画食物（老鼠）
    mov ax, [fy]
    mov bx, 80
    mul bx
    add ax, [fx]
    shl ax, 1
    mov di, ax
    mov byte [es:di], 'm'
    mov byte [es:di+1], 0x0C  ; 红色
    
    ; 画蛇
    call draw_snake
    
    ; 检查输入
    call check_input
    
    ; 移动蛇
    call move_snake
    
    ; 检查碰撞
    call check_collision
    
    ; 延迟
    call delay
    
    jmp game_loop

; 画黄色边框
draw_border:
    push es
    push di
    push cx
    push ax
    
    ; 上边框
    xor di, di
    mov cx, 80
    mov ax, 0x0E3D      ; 黄色'='
.top:
    stosw
    loop .top
    
    ; 下边框
    mov di, 3840        ; 24行 * 80 * 2
    mov cx, 80
.bottom:
    stosw
    loop .bottom
    
    ; 左右边框
    mov cx, 23          ; 中间23行
    mov di, 160         ; 第1行
.sides:
    push cx
    mov word [es:di], 0x0E3D        ; 左边
    mov word [es:di+158], 0x0E3D    ; 右边
    add di, 160
    pop cx
    loop .sides
    
    pop ax
    pop cx
    pop di
    pop es
    ret

; 画蛇
draw_snake:
    push si
    push cx
    push ax
    push bx
    push di
    
    xor si, si
    mov cx, [len]
.loop:
    ; 计算位置
    mov ax, [sx+si+2]   ; y
    mov bx, 80
    mul bx
    add ax, [sx+si]     ; x
    shl ax, 1
    mov di, ax
    
    ; 画蛇
    cmp si, 0
    je .head
    ; 蛇身
    mov byte [es:di], '='
    mov byte [es:di+1], 0x02    ; 绿色
    jmp .next
.head:
    mov byte [es:di], '@'
    mov byte [es:di+1], 0x0A    ; 亮绿色
.next:
    add si, 4
    loop .loop
    
    pop di
    pop bx
    pop ax
    pop cx
    pop si
    ret

; 检查输入（非阻塞）
check_input:
    push ax
    mov ah, 1
    int 0x16
    jz .done
    
    mov ah, 0
    int 0x16
    
    cmp al, 'w'
    je .up
    cmp al, 's'
    je .down
    cmp al, 'a'
    je .left
    cmp al, 'd'
    je .right
    jmp .done
    
.up:
    cmp byte [dir], 1
    je .done
    mov byte [dir], 3
    jmp .done
.down:
    cmp byte [dir], 3
    je .done
    mov byte [dir], 1
    jmp .done
.left:
    cmp byte [dir], 0
    je .done
    mov byte [dir], 2
    jmp .done
.right:
    cmp byte [dir], 2
    je .done
    mov byte [dir], 0
    
.done:
    pop ax
    ret

; 移动蛇
move_snake:
    push si
    push cx
    push ax
    push bx
    
    ; 从尾部开始向前移动
    mov si, [len]
    dec si
    shl si, 2
.loop:
    cmp si, 0
    jle .move_head
    mov ax, [sx+si-4]
    mov [sx+si], ax
    mov ax, [sx+si-2]
    mov [sx+si+2], ax
    sub si, 4
    jmp .loop
    
.move_head:
    ; 根据方向移动头部
    mov al, [dir]
    cmp al, 0
    je .right
    cmp al, 1
    je .down
    cmp al, 2
    je .left
    jmp .up
    
.right:
    inc word [sx]
    jmp .check_food
.left:
    dec word [sx]
    jmp .check_food
.down:
    inc word [sx+2]
    jmp .check_food
.up:
    dec word [sx+2]
    
.check_food:
    ; 检查是否吃到食物
    mov ax, [sx]
    cmp ax, [fx]
    jne .done
    mov ax, [sx+2]
    cmp ax, [fy]
    jne .done
    
    ; 吃到食物，增长
    cmp word [len], 20
    jge .done
    inc word [len]
    
    ; 生成新食物
    call gen_food
    
.done:
    pop bx
    pop ax
    pop cx
    pop si
    ret

; 生成新食物
gen_food:
    push ax
    push dx
    
    ; 使用时钟作为随机数
    mov ah, 0
    int 0x1a
    
    mov ax, dx
    xor dx, dx
    mov bx, 76          ; 2到77之间（避开边框）
    div bx
    add dl, 2
    mov [fx], dl
    
    mov ax, cx
    xor dx, dx
    mov bx, 22          ; 2到23之间
    div bx
    add dl, 2
    mov [fy], dl
    
    pop dx
    pop ax
    ret

; 检查碰撞
check_collision:
    push ax
    
    ; 检查墙壁
    mov ax, [sx]
    cmp ax, 1
    jle game_over
    cmp ax, 78
    jge game_over
    
    mov ax, [sx+2]
    cmp ax, 1
    jle game_over
    cmp ax, 23
    jge game_over
    
    ; 检查自身碰撞
    mov si, 4
    mov cx, [len]
    dec cx
.loop:
    cmp cx, 0
    jle .done
    mov ax, [sx]
    cmp ax, [sx+si]
    jne .next
    mov ax, [sx+2]
    cmp ax, [sx+si+2]
    je game_over
.next:
    add si, 4
    dec cx
    jmp .loop
    
.done:
    pop ax
    ret

; 延迟函数
delay:
    push cx
    push dx
    mov cx, 0xFFFF
.outer:
    push cx
    mov cx, 0x3200
.inner:
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    loop .inner
    pop cx
    loop .outer
    pop dx
    pop cx
    ret

; 游戏结束
game_over:
    ; 清屏
    mov ax, 0xb800
    mov es, ax
    xor di, di
    mov cx, 2000
    mov ax, 0x0720
    rep stosw
    
    ; 显示 "This is SYSU OS!" (居中，第12行)
    mov di, 1860        ; 第12行中间位置 (12*80*2 + 30*2 = 1860)
    mov si, game_over_msg
    mov ah, 0x0A        ; 亮绿色
.msg_loop:
    lodsb
    test al, al
    jz .show_restart
    stosw
    jmp .msg_loop
    
.show_restart:
    ; 显示提示信息 (第14行)
    mov di, 2188        ; 第14行中间 (14*80*2 + 26*2)
    mov si, restart_msg
    mov ah, 0x0E        ; 黄色
.restart_loop:
    lodsb
    test al, al
    jz .key_wait
    stosw
    jmp .restart_loop
    
.key_wait:
    mov ah, 0
    int 0x16
    jmp start           ; 重新开始游戏

; 数据区
dir: db 0               ; 方向：0=右，1=下，2=左，3=上
len: dw 0               ; 蛇长度
fx: dw 0                ; 食物x
fy: dw 0                ; 食物y
sx: times 80 dw 0       ; 蛇身坐标（x,y对）

game_over_msg: db 'This is SYSU OS!', 0
restart_msg: db 'Press any key to restart...', 0

times 2048-($-$$) db 0  ; 填充到2KB（4个扇区）