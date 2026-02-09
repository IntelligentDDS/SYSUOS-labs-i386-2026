# 自动检测 QEMU 命令（兼容麒麟OS等仅有 qemu-system-x86_64 的环境）
QEMU := $(shell command -v qemu-system-i386 2>/dev/null || command -v qemu-system-x86_64 2>/dev/null)

# 自动检测链接器（优先使用系统 ld，ARM64 上使用 i686-elf-ld 交叉工具链）
LD := $(shell command -v ld 2>/dev/null)
LD_SUPPORTS_ELF32 := $(shell ld -melf_i386 --version 2>/dev/null && echo yes || echo no)
ifeq ($(LD_SUPPORTS_ELF32),no)
    LD := $(shell command -v i686-elf-ld 2>/dev/null || echo ld)
endif

run:
	@$(QEMU) -hda hd.img -serial null -parallel stdio 
debug:
	@$(QEMU) -s -S -hda hd.img -serial null -parallel stdio &
	@sleep 1
	@gnome-terminal -- gdb -q -x gdbinit 2>/dev/null || xterm -e "gdb -q -x gdbinit" 2>/dev/null || echo "请在另一个终端中运行: gdb -q -x gdbinit"

# QEMU Monitor 调试（适用于 ARM64 等 GDB 不支持 x86 的环境）
monitor:
	@$(QEMU) -drive file=hd.img,format=raw -display none \
		-monitor telnet:127.0.0.1:4444,server,nowait &
	@sleep 2
	@echo "QEMU Monitor 已启动，请运行: ncat 127.0.0.1 4444"
	@echo "常用命令: info registers, x /10i <addr>, xp /10xb <addr>, stop, cont, quit"

build:
	@nasm -g -f bin mbr.asm -o mbr.bin
	@dd if=mbr.bin of=hd.img bs=512 count=1 seek=0 conv=notrunc
	@nasm -g -f bin bootloader.asm -o bootloader.bin
	@dd if=bootloader.bin of=hd.img bs=512 count=5 seek=1 conv=notrunc

symbol:
	@nasm -g -f elf32 mbr.asm -o mbr.o
	@${LD} -o mbr.symbol -melf_i386 -N mbr.o -Ttext 0x7c00
	@sed '/^org/d' bootloader.asm > bootloader_noorg.asm
	@nasm -g -f elf32 bootloader_noorg.asm -o bootloader.o
	@${LD} -o bootloader.symbol -melf_i386 -N bootloader.o -Ttext 0x7e00
	@rm -f bootloader_noorg.asm

clean:
	@rm -fr *.bin *.o *.symbol
