.PHONY: all clean run

# 自动检测 QEMU：优先使用 qemu-system-i386，若不存在则使用 qemu-system-x86_64
QEMU := $(shell command -v qemu-system-i386 2>/dev/null || command -v qemu-system-x86_64 2>/dev/null)

all: snake.bin snake.img

snake.bin: snake.asm
	nasm -f bin snake.asm -o snake.bin

snake.img: snake.bin
	dd if=/dev/zero of=snake.img bs=512 count=2880
	dd if=snake.bin of=snake.img bs=512 count=1 conv=notrunc

run: snake.img
	$(QEMU) -drive file=snake.img,format=raw,if=floppy

clean:
	rm -f snake.bin snake.img
