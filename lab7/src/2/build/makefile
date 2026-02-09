# ============================================================
# 自动检测平台：Ubuntu AMD64 / 银河麒麟 ARM64
# ============================================================
ARCH := $(shell uname -m)

ifeq ($(ARCH),aarch64)
    # ---- 银河麒麟 ARM64 (交叉编译) ----
    CROSS_PREFIX = $(HOME)/lab1/cross-tools/install/bin/i686-elf-
    ASM_COMPILER = nasm
    C_COMPILER   = $(CROSS_PREFIX)gcc
    CXX_COMPILER = $(CROSS_PREFIX)g++
    LINKER       = $(CROSS_PREFIX)ld
    OBJCOPY      = $(CROSS_PREFIX)objcopy
    CXX_COMPILER_FLAGS = -g -Wall -march=i386 -std=c++11 -m32 -nostdlib \
                         -fno-builtin -ffreestanding -fno-pic -fno-exceptions -fno-rtti
    LINKER_FLAGS = -N
    QEMU = qemu-system-x86_64 -drive file=$(RUNDIR)/hd.img,format=raw
    GDB  = gdb-multiarch
else
    # ---- Ubuntu AMD64 (本地编译) ----
    ASM_COMPILER = nasm
    C_COMPILER   = gcc
    CXX_COMPILER = g++
    LINKER       = ld
    OBJCOPY      = objcopy
    CXX_COMPILER_FLAGS = -g -Wall -march=i386 -std=c++11 -m32 -nostdlib \
                         -fno-builtin -ffreestanding -fno-pic
    LINKER_FLAGS = -N -melf_i386
    QEMU = qemu-system-i386 -hda $(RUNDIR)/hd.img
    GDB  = gdb
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

mbr.bin : $(SRCDIR)/boot/mbr.asm
	$(ASM_COMPILER) -o mbr.bin -f bin -I$(INCLUDE_PATH)/ $(SRCDIR)/boot/mbr.asm

bootloader.bin : $(SRCDIR)/boot/bootloader.asm
	$(ASM_COMPILER) -o bootloader.bin -f bin -I$(INCLUDE_PATH)/ $(SRCDIR)/boot/bootloader.asm

entry.obj : $(SRCDIR)/boot/entry.asm
	$(ASM_COMPILER) -o entry.obj -g -f elf32 $(SRCDIR)/boot/entry.asm

kernel.bin : kernel.o
	$(OBJCOPY) -O binary kernel.o kernel.bin

kernel.o : entry.obj $(OBJ)
	$(LINKER) -o kernel.o $(LINKER_FLAGS) entry.obj $(OBJ) -e enter_kernel -Ttext 0x00020000

$(CXX_OBJ): $(CXX_SOURCE)
	$(CXX_COMPILER) $(CXX_COMPILER_FLAGS) -I$(INCLUDE_PATH) -c $(CXX_SOURCE)

asm_utils.o : $(SRCDIR)/utils/asm_utils.asm
	$(ASM_COMPILER) -o asm_utils.o -g -f elf32 $(SRCDIR)/utils/asm_utils.asm

clean:
	rm -f *.o* *.bin

run:
	$(QEMU) -serial null -parallel stdio -no-reboot

monitor:
	$(QEMU) -serial null -no-reboot -display none \
	    -monitor telnet:127.0.0.1:45472,server,nowait -daemonize
	@echo "QEMU Monitor: telnet 127.0.0.1 45472"

debug:
	$(QEMU) -S -s -parallel stdio -serial null &
	@sleep 1
ifeq ($(ARCH),aarch64)
	$(GDB) -q -tui -x $(RUNDIR)/gdbinit
else
	gnome-terminal -e "$(GDB) -q -tui -x $(RUNDIR)/gdbinit"
endif

test:
	echo $(CXX_OBJ)
