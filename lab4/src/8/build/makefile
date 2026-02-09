# ============================================================
# 自动检测编译环境
# Ubuntu AMD64: 使用 g++ -m32 和 ld -melf_i386
# 银河麒麟 ARM64: 使用 i686-elf-g++ 和 i686-elf-ld 交叉编译
# ============================================================

ARCH := $(shell uname -m)
ifeq ($(ARCH),x86_64)
    C_COMPILER = gcc
    CXX_COMPILER = g++
    CXX_COMPILER_FLAGS = -g -Wall -march=i386 -std=c++11 -m32 -nostdlib -fno-builtin -ffreestanding -fno-pic
    LINKER = ld
    OBJCOPY = objcopy
    QEMU = $(shell which qemu-system-i386 2>/dev/null || echo qemu-system-x86_64)
else
    CROSS_PREFIX = $(HOME)/lab1/cross-tools/install/bin/i686-elf-
    C_COMPILER = $(CROSS_PREFIX)gcc
    CXX_COMPILER = $(CROSS_PREFIX)g++
    CXX_COMPILER_FLAGS = -g -Wall -march=i386 -std=c++11 -m32 -nostdlib -fno-builtin -ffreestanding -fno-pic -fno-exceptions -fno-rtti
    LINKER = $(CROSS_PREFIX)ld
    OBJCOPY = $(CROSS_PREFIX)objcopy
    QEMU = qemu-system-x86_64
endif

ASM_COMPILER = nasm

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
	$(ASM_COMPILER) -o entry.obj -g -f elf32 $(SRCDIR)/boot/entry.asm
	
kernel.bin : kernel.o
	$(OBJCOPY) -O binary kernel.o kernel.bin
	
kernel.o : entry.obj $(OBJ)
	$(LINKER) -o kernel.o -melf_i386 -N entry.obj $(OBJ) -e enter_kernel -Ttext 0x00020000
	
$(CXX_OBJ): $(CXX_SOURCE)
	$(CXX_COMPILER) $(CXX_COMPILER_FLAGS) -I$(INCLUDE_PATH) -c $(CXX_SOURCE)
	
asm_utils.o : $(SRCDIR)/utils/asm_utils.asm
	$(ASM_COMPILER) -o asm_utils.o -g -f elf32 $(SRCDIR)/utils/asm_utils.asm

clean:
	rm -f *.o* *.bin 
	
run:
	$(QEMU) -drive file=$(RUNDIR)/hd.img,format=raw -serial null -parallel stdio -no-reboot

debug: 
	$(QEMU) -S -s -parallel stdio -drive file=$(RUNDIR)/hd.img,format=raw -serial null&
	@sleep 1
ifeq ($(ARCH),x86_64)
	gnome-terminal -e "gdb -q -tui -x $(RUNDIR)/gdbinit" 2>/dev/null || gdb -q -tui -x $(RUNDIR)/gdbinit
else
	gdb-multiarch -q -tui -x $(RUNDIR)/gdbinit 2>/dev/null || echo "请手动启动 gdb: gdb-multiarch -q -tui -x $(RUNDIR)/gdbinit"
endif

monitor:
	$(QEMU) -drive file=$(RUNDIR)/hd.img,format=raw -serial null -parallel stdio -no-reboot -monitor stdio

test:
	echo $(CXX_OBJ)