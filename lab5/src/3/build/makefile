# ==================== 平台自动检测 ====================
ARCH := $(shell uname -m)

ifeq ($(ARCH),x86_64)
    # Ubuntu AMD64：使用原生编译器
    ASM_COMPILER = nasm
    C_COMPILER = gcc
    CXX_COMPILER = g++
    CXX_FLAGS = -g -Wall -march=i386 -m32 -nostdlib -fno-builtin -ffreestanding -fno-pic
    LINKER = ld
    OBJCOPY = objcopy
    QEMU = qemu-system-i386
    QEMU_FLAGS = -hda
    GDB = gdb
else
    # 银河麒麟 ARM64 / 其他架构：使用交叉编译器
    CROSS_PREFIX = $(HOME)/lab1/cross-tools/install/bin/i686-elf-
    ASM_COMPILER = nasm
    C_COMPILER = $(CROSS_PREFIX)gcc
    CXX_COMPILER = $(CROSS_PREFIX)g++
    CXX_FLAGS = -g -Wall -nostdlib -fno-builtin -ffreestanding -fno-pic -fno-exceptions -fno-rtti
    LINKER = $(CROSS_PREFIX)ld
    OBJCOPY = $(CROSS_PREFIX)objcopy
    QEMU = qemu-system-x86_64
    QEMU_FLAGS = -drive file=
    QEMU_FLAGS_SUFFIX = ,format=raw
    GDB = gdb-multiarch
endif

SRCDIR = ../src
RUNDIR = ../run
BUILDDIR = build
INCLUDE_PATH = ../include

KERNEL_SOURCE = $(wildcard $(SRCDIR)/kernel/*.cpp)
CXX_SOURCE += $(KERNEL_SOURCE)
CXX_OBJ += $(KERNEL_SOURCE:$(SRCDIR)/kernel/%.cpp=%.o)

UTILS_SOURCE = $(wildcard $(SRCDIR)/utils/*.cpp)
CXX_SOURCE += $(UTILS_SOURCE)
CXX_OBJ += $(UTILS_SOURCE:$(SRCDIR)/utils/%.cpp=%.o)

ASM_SOURCE += $(wildcard $(SRCDIR)/utils/*.asm)
ASM_OBJ += $(ASM_SOURCE:$(SRCDIR)/utils/%.asm=%.o)

OBJ += $(CXX_OBJ)
OBJ += $(ASM_OBJ)

build : mbr.bin bootloader.bin kernel.bin kernel.o
	dd if=mbr.bin of=$(RUNDIR)/hd.img bs=512 count=1 seek=0 conv=notrunc
	dd if=bootloader.bin of=$(RUNDIR)/hd.img bs=512 count=5 seek=1 conv=notrunc
	dd if=kernel.bin of=$(RUNDIR)/hd.img bs=512 count=145 seek=6 conv=notrunc
# nasm的include path有一个尾随/

mbr.bin : $(SRCDIR)/boot/mbr.asm
	$(ASM_COMPILER) -o mbr.bin -f bin -I$(INCLUDE_PATH)/ $(SRCDIR)/boot/mbr.asm

bootloader.bin : $(SRCDIR)/boot/bootloader.asm
	$(ASM_COMPILER) -o bootloader.bin -f bin -I$(INCLUDE_PATH)/ $(SRCDIR)/boot/bootloader.asm

entry.obj : $(SRCDIR)/boot/entry.asm
	$(ASM_COMPILER) -o entry.obj -f elf32 $(SRCDIR)/boot/entry.asm

kernel.bin : kernel.o
	$(OBJCOPY) -O binary kernel.o kernel.bin

kernel.o : entry.obj $(OBJ)
	$(LINKER) -o kernel.o -melf_i386 -N entry.obj $(OBJ) -e enter_kernel -Ttext 0x00020000

$(CXX_OBJ): $(CXX_SOURCE)
	$(CXX_COMPILER) $(CXX_FLAGS) -I$(INCLUDE_PATH) -c $(CXX_SOURCE)

asm_utils.o : $(SRCDIR)/utils/asm_utils.asm
	$(ASM_COMPILER) -o asm_utils.o -f elf32 $(SRCDIR)/utils/asm_utils.asm

clean:
	rm -f *.o* *.bin

ifeq ($(ARCH),x86_64)
run:
	$(QEMU) $(QEMU_FLAGS) $(RUNDIR)/hd.img -serial null -parallel stdio -no-reboot
debug:
	$(QEMU) -S -s -parallel stdio $(QEMU_FLAGS) $(RUNDIR)/hd.img -serial null &
	@sleep 1
	gnome-terminal -e "$(GDB) -q -tui -x $(RUNDIR)/gdbinit"
else
run:
	$(QEMU) $(QEMU_FLAGS)$(RUNDIR)/hd.img$(QEMU_FLAGS_SUFFIX) -serial null -parallel stdio -no-reboot
monitor:
	$(QEMU) $(QEMU_FLAGS)$(RUNDIR)/hd.img$(QEMU_FLAGS_SUFFIX) -serial null -parallel stdio -no-reboot -monitor telnet:127.0.0.1:45454,server,nowait &
	@sleep 1
	@echo "QEMU Monitor 已启动，使用 telnet 127.0.0.1 45454 连接"
	@echo "常用命令: info registers, xp /20bx 0xb8000, quit"
debug:
	$(QEMU) -S -s $(QEMU_FLAGS)$(RUNDIR)/hd.img$(QEMU_FLAGS_SUFFIX) -serial null -parallel stdio -no-reboot &
	@sleep 1
	$(GDB) -q -tui -x $(RUNDIR)/gdbinit
endif

test:
	echo $(CXX_OBJ)