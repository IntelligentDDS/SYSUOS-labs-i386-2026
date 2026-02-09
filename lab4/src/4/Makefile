# ============================================================
# 自动检测编译环境
# Ubuntu AMD64: 使用 gcc/g++ -m32 进行32位编译，可直接运行
# 银河麒麟 ARM64: 使用 i686-elf-gcc/g++ 交叉编译，
#                 需通过 objdump 或 qemu-i386 验证
# ============================================================

ARCH := $(shell uname -m)
ifeq ($(ARCH),x86_64)
    CC = gcc
    CXX = g++
    CC_FLAGS = -m32
    CXX_FLAGS = -m32
    LINK_FLAGS = -m32
else
    # ARM64 (aarch64) 使用交叉编译器
    # 注意：交叉编译器为 freestanding 环境，不支持 <stdio.h>/<iostream>
    # 需使用不依赖标准库的版本（见 README 说明）
    CROSS_PREFIX = $(HOME)/lab1/cross-tools/install/bin/i686-elf-
    CC = $(CROSS_PREFIX)gcc
    CXX = $(CROSS_PREFIX)g++
    CC_FLAGS =
    CXX_FLAGS = -fno-exceptions -fno-rtti
    LINK_FLAGS = -fno-exceptions -fno-rtti -nostdlib -lgcc
endif

ASM_COMPILER = nasm

main.out: main.o c_func.o cpp_func.o asm_func.o
	$(CXX) -o main.out main.o c_func.o cpp_func.o asm_func.o $(LINK_FLAGS)

c_func.o: c_func.c
	$(CC) -o c_func.o $(CC_FLAGS) -c c_func.c

cpp_func.o: cpp_func.cpp
	$(CXX) -o cpp_func.o $(CXX_FLAGS) -c cpp_func.cpp

main.o: main.cpp
	$(CXX) -o main.o $(CXX_FLAGS) -c main.cpp

asm_func.o: asm_utils.asm
	$(ASM_COMPILER) -o asm_func.o -f elf32 asm_utils.asm

clean:
	rm -f *.o *.out