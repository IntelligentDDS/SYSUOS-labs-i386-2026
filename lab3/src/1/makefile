# 自动检测 QEMU 命令（兼容麒麟OS等仅有 qemu-system-x86_64 的环境）
QEMU := $(shell command -v qemu-system-i386 2>/dev/null || command -v qemu-system-x86_64 2>/dev/null)

build:
	nasm -f bin mbr.asm -o mbr.bin
	nasm -f bin bootloader.asm -o bootloader.bin
	dd if=mbr.bin of=hd.img bs=512 count=1 seek=0 conv=notrunc
	dd if=bootloader.bin of=hd.img bs=512 count=5 seek=1 conv=notrunc
run:
	$(QEMU) -hda hd.img -serial null -parallel stdio 
clean:
	rm -fr *.bin