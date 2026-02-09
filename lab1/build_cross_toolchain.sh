#!/bin/bash
set -e

echo "=== 从源码构建 x86 交叉编译工具链 ==="

CROSS_DIR="$HOME/lab1/cross-tools"
PREFIX="$CROSS_DIR/install"

mkdir -p "$CROSS_DIR"
cd "$CROSS_DIR"

# 1. 编译 binutils for i686-elf
echo "--- Step 1: binutils ---"
if [ ! -f binutils-2.36.tar.xz ]; then
    echo "下载 binutils..."
    wget -q https://mirrors.tuna.tsinghua.edu.cn/gnu/binutils/binutils-2.36.tar.xz
fi
if [ ! -d binutils-2.36 ]; then
    tar xf binutils-2.36.tar.xz
fi

mkdir -p build-binutils
cd build-binutils
if [ ! -f Makefile ]; then
    ../binutils-2.36/configure --target=i686-elf --prefix="$PREFIX" --disable-nls --disable-werror 2>&1 | tail -3
fi
make -j4 2>&1 | tail -5
make install 2>&1 | tail -3

echo "--- binutils 安装完成 ---"
ls "$PREFIX/bin/i686-elf-"* 2>/dev/null | head -10
cd "$CROSS_DIR"

# 2. 编译 GCC cross-compiler for i686-elf
echo "--- Step 2: gcc cross-compiler ---"
if [ ! -f gcc-12.2.0.tar.xz ]; then
    echo "下载 gcc..."
    wget -q https://mirrors.tuna.tsinghua.edu.cn/gnu/gcc/gcc-12.2.0/gcc-12.2.0.tar.xz
fi
if [ ! -d gcc-12.2.0 ]; then
    tar xf gcc-12.2.0.tar.xz
    cd gcc-12.2.0
    # 下载依赖
    ./contrib/download_prerequisites 2>&1 | tail -3
    cd ..
fi

export PATH="$PREFIX/bin:$PATH"
mkdir -p build-gcc
cd build-gcc
if [ ! -f Makefile ]; then
    ../gcc-12.2.0/configure --target=i686-elf --prefix="$PREFIX" \
        --disable-nls --enable-languages=c --without-headers \
        --disable-libssp --disable-libquadmath --disable-libstdcxx 2>&1 | tail -5
fi
make -j4 all-gcc 2>&1 | tail -10
make -j4 all-target-libgcc 2>&1 | tail -5
make install-gcc 2>&1 | tail -3
make install-target-libgcc 2>&1 | tail -3

echo "--- gcc 交叉编译器安装完成 ---"
ls "$PREFIX/bin/i686-elf-"* 2>/dev/null
"$PREFIX/bin/i686-elf-gcc" --version 2>/dev/null | head -1

echo "=== 工具链构建完成 ==="
