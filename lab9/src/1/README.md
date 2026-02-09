# Lab9 示例代码 - FAT16 文件系统

在 Lab8 的原型操作系统基础上，实现一个简化的 FAT16 文件系统。

## 新增文件说明

| 文件 | 说明 |
|------|------|
| `include/disk.h` | ATA 磁盘驱动头文件 |
| `include/fat16.h` | FAT16 文件系统数据结构和接口 |
| `src/kernel/disk.cpp` | ATA PIO 模式磁盘驱动实现 |
| `src/kernel/fat16.cpp` | FAT16 文件系统核心实现 |
| `src/kernel/shell.cpp` | Shell 文件系统演示 |

## 构建与运行

```bash
cd build

# 首次运行：创建 10MB 磁盘镜像
make image

# 编译并写入磁盘镜像
make

# 运行 (图形界面)
make run

# 运行 (无头模式，通过 telnet 查看)
make monitor
# 另一终端: telnet 127.0.0.1 45491
```

## 磁盘布局

```
扇区 0        : MBR
扇区 1-5      : Bootloader
扇区 6-205    : Kernel
...
扇区 500      : FAT16 超级块
扇区 501-532  : FAT 表
扇区 533-564  : 根目录区
扇区 565+     : 数据区
```

## 支持平台

- Ubuntu AMD64 (本地编译)
- 银河麒麟 ARM64 (交叉编译)

Makefile 自动检测平台，无需手动配置。
