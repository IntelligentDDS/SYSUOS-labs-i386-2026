# 第八章 简单文件系统

> 文件系统是操作系统中最直观可见的部分之一。

# 实验环境

| 项目 | Ubuntu AMD64 | 银河麒麟 ARM64 |
|------|-------------|---------------|
| **操作系统** | Ubuntu 22.04 x86_64 | 银河麒麟 V10 aarch64 |
| **C++ 编译器** | `g++ -m32` | `i686-elf-g++`（交叉编译） |
| **汇编器** | `nasm` | `nasm` |
| **链接器** | `ld -melf_i386` | `i686-elf-ld` |
| **QEMU** | `qemu-system-i386` | `qemu-system-x86_64`（用户态模拟） |
| **调试器** | `gdb` | `gdb` |

> **ARM64 注意**：银河麒麟 ARM64 平台需要使用交叉编译工具链，Makefile 已自动检测 `uname -m` 并切换编译器。ARM64 平台额外需要 `-fno-exceptions -fno-rtti` 编译选项。

# QEMU Monitor 调试

示例代码提供 `make monitor` 目标，可在无图形界面的环境下启动 QEMU 并通过 telnet 进行调试。

```shell
cd build
make clean && make image && make build
make monitor          # 后台启动 QEMU，监听 telnet 端口
telnet 127.0.0.1 45491 # 连接 QEMU Monitor
```

| 示例目录 | Monitor 端口 | 说明 |
|---------|-------------|------|
| `src/1` | 45491 | FAT16 简单文件系统 |

常用 QEMU Monitor 命令：

```
info registers    # 查看 CPU 寄存器（含 CR0/CR3/CS 等）
info mem          # 查看虚拟地址映射
xp /Nwx PADDR    # 查看物理地址处的内容（N 个 word）
x /Nwx VADDR     # 查看虚拟地址处的内容
info cpus         # 查看 CPU 状态
info block        # 查看块设备（磁盘）信息
```

# 实验概述

在前面的章节中，我们已经实现了一个具有中断处理、内存管理、进程管理和系统调用等功能的原型操作系统。然而，我们的操作系统还缺少一个非常重要的组件——文件系统。文件系统是操作系统中负责管理持久化存储数据的子系统，它为用户和程序提供了一种方便的方式来组织、存储和访问磁盘上的数据。

在本章中，我们将在前面实验的基础上，实现一个简化的 FAT16 文件系统。我们首先从磁盘驱动开始，实现 ATA PIO 模式的磁盘读写驱动程序。然后，在磁盘驱动之上构建一个简化的 FAT16 文件系统，包括文件分配表（FAT）、目录项管理、文件的创建/读取/写入/删除等操作。最后，我们将文件系统集成到 Shell 中进行演示。

本章涉及的关键概念包括：

+ **磁盘 I/O**：通过 ATA PIO 模式直接控制磁盘的读写。
+ **文件分配表（FAT）**：用链式结构管理磁盘上的数据块（簇）分配。
+ **目录条目**：记录文件名、大小、属性和起始簇号等元数据。
+ **文件操作**：实现 format、create、read、write、delete 和 list 等基本文件操作。

> 代码放置在 `src/1` 下。

# 磁盘驱动

要实现文件系统，首先需要解决的问题是如何与磁盘进行通信。我们需要能够从磁盘中读取数据，以及将数据写入磁盘。在我们的实验环境中，QEMU 模拟了一个 ATA（Advanced Technology Attachment）硬盘，我们通过 PIO（Programmed I/O）模式来访问它。

## ATA PIO 模式简介

ATA（Advanced Technology Attachment），也称为 IDE（Integrated Drive Electronics），是一种常见的硬盘接口标准。在早期的 PC 中，ATA 接口是最主要的硬盘连接方式。

PIO（Programmed I/O，程序化输入/输出）是 ATA 接口最基本的数据传输模式。在 PIO 模式下，CPU 通过 I/O 端口直接控制数据传输的每一个步骤——选择扇区、发送命令、等待就绪、逐字传输数据。虽然 PIO 模式效率不高（CPU 需要全程参与数据传输），但它的实现非常简单，非常适合我们的教学操作系统。

> 在实际的操作系统中，通常使用 DMA（Direct Memory Access）模式来进行磁盘数据传输，DMA 模式允许磁盘控制器直接与内存交换数据，不需要 CPU 参与，从而大大提高了效率。但 DMA 的实现较为复杂，因此我们在本实验中使用 PIO 模式。

### ATA 端口

ATA 的主通道使用 I/O 端口 `0x1F0`~`0x1F7`，每个端口有不同的功能，如下表所示。

| 端口 | 读取时 | 写入时 | 说明 |
|------|-------|-------|------|
| `0x1F0` | 数据寄存器 | 数据寄存器 | 16 位数据传输端口 |
| `0x1F1` | 错误寄存器 | Features | 读取错误信息 / 写入特性 |
| `0x1F2` | 扇区计数 | 扇区计数 | 要读/写的扇区数量 |
| `0x1F3` | LBA 低 8 位 | LBA 低 8 位 | LBA 地址 [7:0] |
| `0x1F4` | LBA 中 8 位 | LBA 中 8 位 | LBA 地址 [15:8] |
| `0x1F5` | LBA 高 8 位 | LBA 高 8 位 | LBA 地址 [23:16] |
| `0x1F6` | 驱动器/磁头 | 驱动器/磁头 | LBA [27:24] + 驱动器选择 + LBA 模式位 |
| `0x1F7` | 状态寄存器 | 命令寄存器 | 读取状态 / 发送命令 |

其中，`0x1F6` 端口的格式较为特殊，其各位含义如下：

```
位 7   : 1 (固定)
位 6   : 1 = LBA 模式, 0 = CHS 模式
位 5   : 1 (固定)
位 4   : 0 = 主盘(Master), 1 = 从盘(Slave)
位 3~0 : LBA 地址的高 4 位 [27:24]
```

我们使用 LBA（Logical Block Addressing，逻辑块寻址）模式来访问磁盘。在 LBA 模式下，磁盘的每个扇区都有一个唯一的逻辑编号，从 0 开始递增。这比传统的 CHS（Cylinder-Head-Sector，柱面-磁头-扇区）寻址方式要简单得多。对于主盘的 LBA 模式，`0x1F6` 端口的值通常设置为 `0xE0 | (lba的高4位)`。

### 状态寄存器

状态寄存器（端口 `0x1F7` 读取）的各位含义如下：

| 位 | 名称 | 值 | 说明 |
|----|------|-----|------|
| 7 | BSY | `0x80` | 磁盘忙，正在处理命令 |
| 6 | DRDY | `0x40` | 磁盘就绪，可以接受命令 |
| 5 | DF | `0x20` | 驱动器故障 |
| 4 | SRV | `0x10` | 服务请求 |
| 3 | DRQ | `0x08` | 数据请求，数据已准备好可传输 |
| 2 | CORR | `0x04` | 数据已校正 |
| 1 | IDX | `0x02` | 索引标记 |
| 0 | ERR | `0x01` | 有错误发生，具体错误见错误寄存器 |

我们在编程时主要关注以下几个状态位：

+ **BSY（`0x80`）**：当此位为 1 时，表示磁盘正在忙碌，此时不应对其他寄存器进行读写。我们在发送命令前需要轮询等待此位清零。
+ **DRQ（`0x08`）**：当此位为 1 时，表示磁盘已准备好数据，可以进行数据传输。在发送 READ 命令后，我们需要等待此位置 1 再读取数据。
+ **ERR（`0x01`）**：当此位为 1 时，表示上一条命令执行出错。

### ATA 命令

常用的 ATA 命令写入到端口 `0x1F7` 中：

| 命令 | 值 | 说明 |
|------|-----|------|
| READ SECTORS | `0x20` | 读取扇区 |
| WRITE SECTORS | `0x30` | 写入扇区 |
| IDENTIFY | `0xEC` | 识别设备 |

在我们的实现中，只使用 `0x20`（READ）和 `0x30`（WRITE）两个命令。

在头文件 `include/disk.h` 中，我们定义了上述 ATA 端口、命令和状态位的宏，如下所示。

```cpp
// ATA PIO 端口定义
#define ATA_DATA       0x1F0
#define ATA_ERROR      0x1F1
#define ATA_SECT_CNT   0x1F2
#define ATA_LBA_LOW    0x1F3
#define ATA_LBA_MID    0x1F4
#define ATA_LBA_HIGH   0x1F5
#define ATA_DRIVE_HEAD 0x1F6
#define ATA_STATUS     0x1F7
#define ATA_COMMAND    0x1F7

// ATA 命令
#define ATA_CMD_READ   0x20
#define ATA_CMD_WRITE  0x30

// ATA 状态位
#define ATA_STATUS_BSY  0x80
#define ATA_STATUS_DRDY 0x40
#define ATA_STATUS_DRQ  0x08
#define ATA_STATUS_ERR  0x01
```

## 16位端口 I/O

在之前的实验中，我们已经实现了 8 位端口 I/O 函数 `asm_in_port` 和 `asm_out_port`，分别用于从端口读取一个字节和向端口写入一个字节。但是，ATA 数据寄存器（端口 `0x1F0`）是一个 16 位的端口——每次数据传输需要读写 16 位（2 字节）的数据。因此，我们需要新增 16 位端口 I/O 函数。

我们在 `src/utils/asm_utils.asm` 中添加两个新的汇编函数，如下所示。

```asm
; void asm_out_port_word(uint16 port, uint16 value)
asm_out_port_word:
    push ebp
    mov ebp, esp
    push edx
    push eax
    mov edx, [ebp + 4 * 2] ; port
    mov eax, [ebp + 4 * 3] ; value
    out dx, ax              ; 16-bit output
    pop eax
    pop edx
    pop ebp
    ret

; void asm_in_port_word(uint16 port, uint16 *value)
asm_in_port_word:
    push ebp
    mov ebp, esp
    push edx
    push eax
    push ebx
    xor eax, eax
    mov edx, [ebp + 4 * 2] ; port
    mov ebx, [ebp + 4 * 3] ; *value
    in ax, dx               ; 16-bit input
    mov [ebx], ax
    pop ebx
    pop eax
    pop edx
    pop ebp
    ret
```

`asm_out_port_word` 的实现和 `asm_out_port` 类似，区别在于使用了 `out dx, ax` 指令（而非 `out dx, al`），这样就能一次输出 16 位数据。同理，`asm_in_port_word` 使用了 `in ax, dx` 指令来一次读入 16 位数据，并通过指针参数 `value` 将结果写回。

在 `include/asm_utils.h` 中声明这两个函数：

```cpp
extern "C" void asm_out_port_word(uint16 port, uint16 value);
extern "C" void asm_in_port_word(uint16 port, uint16 *value);
```

> 为什么 ATA 数据端口是 16 位的？因为 ATA 规范定义数据传输的最小单位是 16 位字。一个扇区（512 字节）包含 256 个 16 位字（$512 / 2 = 256$）。因此，读/写一个扇区需要进行 256 次 16 位端口 I/O 操作。

## DiskDriver 类

有了端口 I/O 的基础设施后，我们就可以实现磁盘驱动了。我们将磁盘驱动封装成 `DiskDriver` 类，声明在 `include/disk.h` 中，如下所示。

```cpp
class DiskDriver
{
public:
    DiskDriver();
    void initialize();

    // 从指定 LBA 扇区开始，读取 count 个扇区到 buffer
    bool readSectors(uint32 lba, uint32 count, void *buffer);

    // 从指定 LBA 扇区开始，将 buffer 写入 count 个扇区
    bool writeSectors(uint32 lba, uint32 count, const void *buffer);

private:
    // 等待磁盘就绪
    void waitReady();
    // 选择扇区 (LBA 模式)
    void selectSector(uint32 lba, uint8 count);
};
```

`DiskDriver` 类对外提供两个主要接口：

+ `readSectors(lba, count, buffer)`：从 LBA 扇区 `lba` 开始，连续读取 `count` 个扇区的数据到 `buffer`。
+ `writeSectors(lba, count, buffer)`：从 LBA 扇区 `lba` 开始，将 `buffer` 中的数据写入连续 `count` 个扇区。

实现放在 `src/kernel/disk.cpp` 中。我们首先来看两个辅助函数。

### 等待磁盘就绪 (waitReady)

在对磁盘发送任何命令之前，我们需要先确保磁盘不在忙碌状态。`waitReady` 函数通过轮询状态寄存器中的 BSY 位来等待磁盘就绪。

```cpp
void DiskDriver::waitReady()
{
    uint8 status;
    do
    {
        asm_in_port(ATA_STATUS, &status);
    } while (status & ATA_STATUS_BSY);
}
```

这段代码不断地从端口 `0x1F7` 读取状态寄存器的值，直到 BSY 位（`0x80`）为 0，表示磁盘已经空闲。

### 选择扇区 (selectSector)

`selectSector` 函数负责设置要操作的扇区地址和数量。

```cpp
void DiskDriver::selectSector(uint32 lba, uint8 count)
{
    waitReady();
    asm_out_port(ATA_SECT_CNT, count);                         // 扇区数量
    asm_out_port(ATA_LBA_LOW, (uint8)(lba & 0xFF));            // LBA [7:0]
    asm_out_port(ATA_LBA_MID, (uint8)((lba >> 8) & 0xFF));    // LBA [15:8]
    asm_out_port(ATA_LBA_HIGH, (uint8)((lba >> 16) & 0xFF));  // LBA [23:16]
    // LBA 模式, 主盘, 高4位LBA
    asm_out_port(ATA_DRIVE_HEAD, (uint8)(0xE0 | ((lba >> 24) & 0x0F)));
}
```

这个函数的步骤是：
1. 等待磁盘就绪。
2. 向端口 `0x1F2` 写入要读/写的扇区数量。
3. 向端口 `0x1F3`~`0x1F5` 分别写入 LBA 地址的低 8 位、中 8 位和高 8 位。
4. 向端口 `0x1F6` 写入驱动器/磁头寄存器的值：`0xE0`（LBA 模式 + 主盘）和 LBA 的最高 4 位。

### 读取扇区 (readSectors)

```cpp
bool DiskDriver::readSectors(uint32 lba, uint32 count, void *buffer)
{
    uint16 *buf = (uint16 *)buffer;

    for (uint32 s = 0; s < count; ++s)
    {
        selectSector(lba + s, 1);
        asm_out_port(ATA_COMMAND, ATA_CMD_READ);

        // 等待数据就绪
        uint8 status;
        do
        {
            asm_in_port(ATA_STATUS, &status);
        } while (!(status & ATA_STATUS_DRQ));

        // 每个扇区 256 个 16-bit 字
        for (int i = 0; i < 256; ++i)
        {
            asm_in_port_word(ATA_DATA, &buf[s * 256 + i]);
        }
    }

    return true;
}
```

读取扇区的过程如下：

1. **循环处理每个扇区**：虽然 ATA 支持一次性读取多个扇区，但为了简单起见，我们每次只读取一个扇区。
2. **选择扇区**：调用 `selectSector` 设置 LBA 地址和扇区计数为 1。
3. **发送 READ 命令**：向命令寄存器（端口 `0x1F7`）写入 `0x20`（READ SECTORS）。
4. **等待 DRQ**：轮询状态寄存器，等待 DRQ 位（`0x08`）置 1，表示磁盘已将数据准备好。
5. **传输数据**：从数据端口 `0x1F0` 连续读取 256 个 16 位字（共 512 字节，即一个扇区的大小），存入 `buffer`。

### 写入扇区 (writeSectors)

```cpp
bool DiskDriver::writeSectors(uint32 lba, uint32 count, const void *buffer)
{
    const uint16 *buf = (const uint16 *)buffer;

    for (uint32 s = 0; s < count; ++s)
    {
        selectSector(lba + s, 1);
        asm_out_port(ATA_COMMAND, ATA_CMD_WRITE);

        // 等待数据就绪
        uint8 status;
        do
        {
            asm_in_port(ATA_STATUS, &status);
        } while (!(status & ATA_STATUS_DRQ));

        // 每个扇区 256 个 16-bit 字
        for (int i = 0; i < 256; ++i)
        {
            asm_out_port_word(ATA_DATA, buf[s * 256 + i]);
        }
    }

    return true;
}
```

写入扇区的过程和读取类似，区别在于：

+ 发送的命令是 `0x30`（WRITE SECTORS）。
+ 数据方向相反：从 `buffer` 中取出 16 位字，通过 `asm_out_port_word` 写入到数据端口 `0x1F0`。

至此，我们的磁盘驱动就完成了。它提供了 `readSectors` 和 `writeSectors` 两个接口，可以对磁盘上任意 LBA 扇区进行读写操作。接下来，我们将在磁盘驱动之上构建文件系统。

# FAT16 文件系统

## 文件系统概述

文件系统（File System）是操作系统中用于管理持久化存储设备上数据的一套机制。没有文件系统时，磁盘上的数据只是一串无意义的二进制序列；有了文件系统，我们才能以"文件"和"目录"的形式来组织和访问数据。

文件系统需要解决以下几个核心问题：

1. **空间管理**：如何跟踪哪些磁盘块已被使用，哪些是空闲的？
2. **文件组织**：一个文件可能占用多个不连续的磁盘块，如何将它们关联起来？
3. **元数据管理**：如何存储文件名、大小、创建时间等信息？
4. **文件操作**：如何实现创建、读取、写入、删除等操作？

FAT（File Allocation Table，文件分配表）文件系统是由微软开发的一种简单而经典的文件系统。FAT 文件系统经历了多个版本的演进：

+ **FAT12**：最早的版本，每个 FAT 表条目占 12 位，最多支持约 4084 个簇，主要用于软盘。
+ **FAT16**：每个 FAT 表条目占 16 位，最多支持 65536 个簇，用于早期的硬盘和 U 盘。
+ **FAT32**：每个 FAT 表条目占 32 位（实际使用 28 位），支持更大的分区，至今仍广泛用于 U 盘和 SD 卡。

在本实验中，我们实现一个**简化的 FAT16** 文件系统。与标准 FAT16 相比，我们做了以下简化：

+ 只支持单级根目录（无子目录）。
+ 每个簇固定为 1 个扇区（512 字节）。
+ 不支持长文件名（只使用 8.3 格式）。
+ 没有完整的 BPB（BIOS Parameter Block），使用简化的超级块。

这些简化使得我们可以专注于理解文件系统的核心原理，而不必被复杂的标准细节所困扰。

## 磁盘布局

我们的文件系统位于磁盘上从扇区 500 开始的区域，整体布局如下：

```
扇区 0        : MBR（主引导记录）
扇区 1~5      : Bootloader
扇区 6~205    : Kernel
  ...
扇区 500      : SuperBlock（超级块）
扇区 501~532  : FAT 表（32 个扇区，8192 个条目）
扇区 533~564  : 根目录区（32 个扇区，512 个条目）
扇区 565+     : 数据区（文件内容存放区）
```

> 为什么文件系统从扇区 500 开始？因为磁盘的前面部分已经被 MBR（扇区 0）、Bootloader（扇区 1~5）和 Kernel（扇区 6~205）占用了。我们将文件系统放在扇区 500 开始的位置，留出足够的间隔以避免与操作系统的引导区和内核区域冲突。这些常量在 `include/fat16.h` 中定义：

```cpp
#define FS_START_SECTOR   500     // 文件系统起始扇区
#define FAT_SECTORS       32      // FAT 表占用的扇区数
#define ROOT_DIR_SECTORS  32      // 根目录区占用的扇区数
#define MAX_ROOT_ENTRIES  512     // 最大根目录条目数 (32×16)
#define ENTRIES_PER_SECTOR 16     // 每扇区目录条目数 (512/32)
#define FAT_ENTRIES       8192    // FAT 表条目总数 (32×256)
#define DATA_START_OFFSET 65      // 数据区相对 FS_START_SECTOR 的偏移
#define SECTOR_SIZE       512     // 每扇区字节数
```

其中，`DATA_START_OFFSET = 1(超级块) + 32(FAT表) + 32(根目录区) = 65`。

## 超级块 (SuperBlock)

超级块是文件系统的"身份证"，位于文件系统的第一个扇区（扇区 500）。它记录了文件系统的基本参数，操作系统在挂载文件系统时首先读取超级块来了解文件系统的布局信息。

```cpp
struct SuperBlock
{
    uint8  magic[4];          // "SF16" 魔数
    uint16 bytesPerSector;    // 每扇区字节数 (512)
    uint8  sectorsPerCluster; // 每簇扇区数 (1)
    uint16 fatSectors;        // FAT 表占用的扇区数
    uint16 rootDirSectors;    // 根目录区占用的扇区数
    uint16 maxRootEntries;    // 最大根目录条目数
    uint32 totalSectors;      // 数据区总扇区数
    uint8  reserved[495];     // 填充到 512 字节
} __attribute__((packed));
```

各字段说明：

+ `magic[4]`：魔数（Magic Number），设置为 `"SF16"`（Simple FAT16 的缩写）。挂载文件系统时，先检查魔数是否正确，以判断磁盘上是否存在有效的文件系统。
+ `bytesPerSector`：每个扇区的字节数，固定为 512。
+ `sectorsPerCluster`：每个簇包含的扇区数，我们设置为 1，即一个簇等于一个扇区。
+ `fatSectors`：FAT 表占用的扇区数（32）。
+ `rootDirSectors`：根目录区占用的扇区数（32）。
+ `maxRootEntries`：根目录中最多可以存放的文件条目数（512）。
+ `totalSectors`：数据区的总扇区数。
+ `reserved`：填充字段，使超级块的总大小恰好为 512 字节（一个扇区的大小）。

`__attribute__((packed))` 告诉编译器不要在结构体成员之间插入任何填充字节（padding），确保结构体的内存布局与磁盘上的数据格式完全一致。

## FAT 表

FAT 表（File Allocation Table，文件分配表）是 FAT 文件系统的核心数据结构。它是一个数组，每个元素（称为一个"条目"）对应磁盘数据区中的一个簇。FAT 表通过链表的方式记录每个文件所占用的簇的顺序。

### FAT 表的基本结构

在 FAT16 中，每个 FAT 表条目占 2 个字节（16 位）。我们的 FAT 表占用 32 个扇区，每个扇区包含 $512 / 2 = 256$ 个条目，因此 FAT 表共有 $32 \times 256 = 8192$ 个条目。

每个 FAT 表条目的值表示如下含义：

| 条目值 | 含义 |
|--------|------|
| `0x0000` | 空闲簇（可以被分配） |
| `0x0002`~`0xFFEF` | 下一个簇号（簇链的下一个节点） |
| `0xFFF7` | 坏簇（该簇有物理损坏，不可使用） |
| `0xFFF8`~`0xFFFF` | 文件结束标记（EOF，簇链的末尾） |

特殊规则：
+ **条目 0 和条目 1 是保留的**，不对应实际的数据簇。条目 0 通常设为 `0xFFF8`，条目 1 设为 `0xFFFF`。
+ **有效的簇号从 2 开始**，簇 2 对应数据区的第一个扇区。

在代码中，我们定义了 FAT 相关的常量：

```cpp
#define FAT16_FREE        0x0000  // 空闲簇
#define FAT16_EOF         0xFFF8  // 文件结束标记
#define FAT16_BAD         0xFFF7  // 坏簇
```

### 簇链（Cluster Chain）

FAT 文件系统使用**链式结构**来记录一个文件占用了哪些簇。具体来说：

+ 目录条目中记录了文件的**第一个簇号**（`firstCluster`）。
+ `FAT[firstCluster]` 的值是文件的第二个簇号。
+ `FAT[第二个簇号]` 的值是文件的第三个簇号。
+ ……以此类推，直到某个条目的值为 `0xFFF8`~`0xFFFF`（EOF），表示文件结束。

举例说明：假设文件 "hello.txt" 占用了 2 个簇（2 个扇区），起始簇号为 5。

```
目录条目: firstCluster = 5

FAT 表:
  FAT[0] = 0xFFF8  (保留)
  FAT[1] = 0xFFFF  (保留)
  FAT[2] = 0x0000  (空闲)
  FAT[3] = 0x0000  (空闲)
  FAT[4] = 0x0000  (空闲)
  FAT[5] = 8       ← 簇 5 → 下一个簇是 8
  FAT[6] = 0x0000  (空闲)
  FAT[7] = 0x0000  (空闲)
  FAT[8] = 0xFFF8  ← 簇 8 → EOF（文件结束）

簇链: 5 → 8 → EOF
```

读取这个文件时，先从目录条目获取 `firstCluster = 5`，读取簇 5 的数据；然后查 `FAT[5] = 8`，读取簇 8 的数据；再查 `FAT[8] = 0xFFF8`，发现是 EOF，文件读取结束。

> FAT 表的这种链式结构非常直观，但有一个缺点：要读取文件中间某个位置的数据，必须从头遍历簇链。这在实际系统中可能导致效率问题，但对于我们的简化文件系统来说已经足够。

## 目录条目 (Directory Entry)

目录条目用于存储文件的元数据信息。每个目录条目固定占 32 字节，结构如下：

```cpp
struct DirEntry
{
    char   filename[8];       // 文件名（空格填充）
    char   extension[3];      // 扩展名（空格填充）
    uint8  attributes;        // 文件属性
    uint8  reserved[10];      // 保留字段
    uint16 firstCluster;      // 起始簇号
    uint32 fileSize;          // 文件大小（字节）
} __attribute__((packed));
```

### 8.3 文件名格式

FAT 文件系统使用所谓的"8.3"文件名格式：文件名最多 8 个字符，扩展名最多 3 个字符。文件名和扩展名分别存储在 `filename` 和 `extension` 字段中。

规则如下：
+ 文件名和扩展名全部转换为**大写字母**。
+ 如果文件名不足 8 个字符，右边用**空格**（`0x20`）填充。
+ 如果扩展名不足 3 个字符，右边用空格填充。
+ 文件名和扩展名之间的点号（`.`）不存储，由格式隐含。

例如：

| 用户输入 | filename (8字节) | extension (3字节) |
|---------|-----------------|-------------------|
| `hello.txt` | `HELLO   ` | `TXT` |
| `data.bin` | `DATA    ` | `BIN` |
| `readme` | `README  ` | `   ` |
| `a.c` | `A       ` | `C  ` |

### 文件属性

`attributes` 字段是一个位图，标记文件的各种属性：

| 属性 | 值 | 说明 |
|------|-----|------|
| `ATTR_READONLY` | `0x01` | 只读 |
| `ATTR_HIDDEN` | `0x02` | 隐藏 |
| `ATTR_SYSTEM` | `0x04` | 系统文件 |
| `ATTR_DIRECTORY` | `0x10` | 目录 |
| `ATTR_ARCHIVE` | `0x20` | 归档（普通文件） |

在我们的简化实现中，创建的文件统一设置为 `ATTR_ARCHIVE`。

### 特殊标记

目录条目的第一个字节有两个特殊含义：

+ **`0x00`**：表示该条目为空，并且**之后的所有条目也都是空的**。在遍历目录时，遇到 `0x00` 就可以停止搜索。
+ **`0xE5`**：表示该条目对应的文件**已被删除**。删除文件时，我们只需将条目的第一个字节设为 `0xE5`，而不需要清除整个条目。这样被删除的文件空间可以被后续的新文件复用。

### 每扇区的目录条目数

由于每个目录条目占 32 字节，每个扇区 512 字节，所以每个扇区可以容纳 $512 / 32 = 16$ 个目录条目。根目录区共 32 个扇区，因此最多可以存放 $32 \times 16 = 512$ 个文件。

## 簇与扇区的转换

在文件系统中，数据区的每个簇都有一个簇号。由于簇号从 2 开始，而数据区从文件系统起始扇区偏移 `DATA_START_OFFSET = 65` 个扇区的位置开始，因此簇号到扇区号的转换公式为：

$$\text{sector} = \text{FS\_START\_SECTOR} + \text{DATA\_START\_OFFSET} + (\text{cluster} - 2)$$

在代码中实现如下：

```cpp
uint32 FAT16::clusterToSector(uint16 cluster)
{
    // 簇号从 2 开始，簇 2 对应数据区第一个扇区
    return FS_START_SECTOR + DATA_START_OFFSET + (cluster - 2);
}
```

例如：簇 2 对应扇区 $500 + 65 + 0 = 565$，簇 3 对应扇区 566，以此类推。

# 文件操作实现

了解了 FAT16 文件系统的整体结构后，我们来看具体的文件操作是如何实现的。文件系统的核心代码放在 `src/kernel/fat16.cpp` 中。

## 辅助函数

在介绍主要的文件操作之前，我们先来看几个辅助函数。

### 文件名格式转换

`toFAT16Name` 函数将用户输入的文件名（如 `"hello.txt"`）转换为 FAT 的 8.3 格式：

```cpp
void FAT16::toFAT16Name(const char *name, char *fat16Name)
{
    // 初始化为空格
    for (int i = 0; i < 11; ++i)
        fat16Name[i] = ' ';

    int i = 0, j = 0;

    // 复制文件名部分 (最多8字符)
    while (name[i] && name[i] != '.' && j < 8)
    {
        char c = name[i];
        if (c >= 'a' && c <= 'z')
            c = c - 'a' + 'A';  // 转大写
        fat16Name[j] = c;
        ++i; ++j;
    }

    // 跳过多余的文件名字符
    while (name[i] && name[i] != '.')
        ++i;

    // 复制扩展名部分 (最多3字符)
    if (name[i] == '.')
    {
        ++i;
        j = 8;
        while (name[i] && j < 11)
        {
            char c = name[i];
            if (c >= 'a' && c <= 'z')
                c = c - 'a' + 'A';
            fat16Name[j] = c;
            ++i; ++j;
        }
    }
}
```

反向转换函数 `fromFAT16Name` 则将 8.3 格式的文件名还原为可读格式（小写，去尾部空格，加点号）。

### FAT 表操作

FAT 表在文件系统挂载时被整体加载到内存中（`fatTable` 数组），修改后通过 `syncFAT` 写回磁盘：

```cpp
void FAT16::loadFAT()
{
    // FAT 表从 FS_START_SECTOR + 1 开始
    disk->readSectors(FS_START_SECTOR + 1, FAT_SECTORS, (void *)fatTable);
}

void FAT16::syncFAT()
{
    disk->writeSectors(FS_START_SECTOR + 1, FAT_SECTORS, (const void *)fatTable);
}
```

分配空闲簇和释放簇链的函数：

```cpp
uint16 FAT16::allocateCluster()
{
    // 从簇 2 开始搜索（0 和 1 为保留簇）
    for (int i = 2; i < FAT_ENTRIES; ++i)
    {
        if (fatTable[i] == FAT16_FREE)
        {
            fatTable[i] = FAT16_EOF;  // 标记为已占用（末尾）
            return (uint16)i;
        }
    }
    return 0; // 无空闲簇
}

void FAT16::freeClusterChain(uint16 startCluster)
{
    uint16 current = startCluster;
    while (current >= 2 && current < FAT16_BAD)
    {
        uint16 next = fatTable[current];
        fatTable[current] = FAT16_FREE;  // 释放当前簇
        current = next;                  // 移到下一个簇
    }
}
```

`allocateCluster` 从簇号 2 开始线性搜索 FAT 表，找到第一个空闲簇（值为 `0x0000`），将其标记为 EOF（`0xFFF8`），然后返回簇号。

`freeClusterChain` 从给定的起始簇开始，沿着簇链逐一将每个簇释放（设为 `0x0000`），直到遇到 EOF 或无效值。

### 目录条目操作

```cpp
void FAT16::readDirEntry(int index, DirEntry *entry)
{
    uint32 rootStart = FS_START_SECTOR + 1 + FAT_SECTORS;
    uint32 sectorIndex = index / ENTRIES_PER_SECTOR;
    uint32 entryOffset = index % ENTRIES_PER_SECTOR;

    uint8 sectorBuf[SECTOR_SIZE];
    disk->readSectors(rootStart + sectorIndex, 1, sectorBuf);
    memcpy(sectorBuf + entryOffset * 32, entry, 32);
}

void FAT16::writeDirEntry(int index, const DirEntry *entry)
{
    uint32 rootStart = FS_START_SECTOR + 1 + FAT_SECTORS;
    uint32 sectorIndex = index / ENTRIES_PER_SECTOR;
    uint32 entryOffset = index % ENTRIES_PER_SECTOR;

    uint8 sectorBuf[SECTOR_SIZE];
    disk->readSectors(rootStart + sectorIndex, 1, sectorBuf);
    memcpy((void *)entry, sectorBuf + entryOffset * 32, 32);
    disk->writeSectors(rootStart + sectorIndex, 1, sectorBuf);
}
```

由于目录条目存储在磁盘上，且每个扇区包含 16 个目录条目，因此读写单个目录条目时，需要：

1. 计算条目所在的扇区号：`sectorIndex = index / 16`。
2. 计算条目在扇区内的偏移：`entryOffset = index % 16`。
3. 读取整个扇区到缓冲区。
4. 从缓冲区中复制出/写入目标条目（偏移 `entryOffset * 32` 字节处）。
5. 写入时，还需要将修改后的扇区整体写回磁盘。

## 格式化 (format)

格式化是创建文件系统的过程。`format` 函数会在磁盘上初始化超级块、FAT 表和根目录区，如下所示。

```cpp
bool FAT16::format(DiskDriver *drv)
{
    this->disk = drv;

    printf("Formatting FAT16 filesystem...\n");

    // 1. 写入超级块
    SuperBlock sb;
    memset(&sb, 0, sizeof(SuperBlock));
    sb.magic[0] = 'S';
    sb.magic[1] = 'F';
    sb.magic[2] = '1';
    sb.magic[3] = '6';
    sb.bytesPerSector = SECTOR_SIZE;
    sb.sectorsPerCluster = 1;
    sb.fatSectors = FAT_SECTORS;
    sb.rootDirSectors = ROOT_DIR_SECTORS;
    sb.maxRootEntries = MAX_ROOT_ENTRIES;
    sb.totalSectors = FAT_ENTRIES - 2;
    disk->writeSectors(FS_START_SECTOR, 1, &sb);

    // 2. 清空 FAT 表
    memset(fatTable, 0, sizeof(fatTable));
    fatTable[0] = 0xFFF8; // 保留
    fatTable[1] = 0xFFFF; // 保留
    syncFAT();

    // 3. 清空根目录区
    uint8 zeroBuf[SECTOR_SIZE];
    memset(zeroBuf, 0, SECTOR_SIZE);
    uint32 rootStart = FS_START_SECTOR + 1 + FAT_SECTORS;
    for (int i = 0; i < ROOT_DIR_SECTORS; ++i)
    {
        disk->writeSectors(rootStart + i, 1, zeroBuf);
    }

    mounted = true;
    printf("Format complete. FAT16 filesystem ready.\n");
    return true;
}
```

格式化的三个步骤：

1. **写入超级块**：填充超级块的各个字段，设置魔数为 `"SF16"`，然后写入扇区 500。
2. **初始化 FAT 表**：将整个 FAT 表清零，然后将保留条目 0 设为 `0xFFF8`、条目 1 设为 `0xFFFF`，最后将 FAT 表写入磁盘（扇区 501~532）。
3. **清空根目录区**：将根目录区的 32 个扇区全部写入零，表示没有任何文件。

格式化完成后，磁盘上就有了一个空的 FAT16 文件系统。

## 挂载文件系统 (initialize)

挂载是在格式化之后或系统重启后"加载"文件系统的过程。

```cpp
bool FAT16::initialize(DiskDriver *drv)
{
    this->disk = drv;

    // 读取超级块，验证魔数
    SuperBlock sb;
    disk->readSectors(FS_START_SECTOR, 1, &sb);

    if (sb.magic[0] != 'S' || sb.magic[1] != 'F' ||
        sb.magic[2] != '1' || sb.magic[3] != '6')
    {
        printf("No FAT16 filesystem found. Use 'format' to create one.\n");
        return false;
    }

    // 加载 FAT 表到内存
    loadFAT();
    mounted = true;
    printf("FAT16 filesystem mounted.\n");
    return true;
}
```

挂载时，先读取扇区 500 处的超级块，检查魔数是否为 `"SF16"`。如果魔数不匹配，说明磁盘上还没有有效的文件系统。如果验证通过，则将 FAT 表从磁盘加载到内存中的 `fatTable` 数组，以便后续的文件操作能够快速查询和修改 FAT 表。

## 创建文件 (createFile)

```cpp
bool FAT16::createFile(const char *name)
{
    if (!mounted) { ... }

    // 检查文件是否已存在
    if (findEntry(name) >= 0)
    {
        printf("Error: file '%s' already exists\n", name);
        return false;
    }

    // 找到空闲目录条目
    int idx = findFreeEntry();
    if (idx < 0)
    {
        printf("Error: root directory full\n");
        return false;
    }

    // 创建目录条目
    DirEntry entry;
    memset(&entry, 0, sizeof(DirEntry));

    char fat16Name[11];
    toFAT16Name(name, fat16Name);
    memcpy(fat16Name, entry.filename, 8);
    memcpy(fat16Name + 8, entry.extension, 3);

    entry.attributes = ATTR_ARCHIVE;
    entry.firstCluster = 0; // 空文件没有簇
    entry.fileSize = 0;

    writeDirEntry(idx, &entry);
    return true;
}
```

创建文件的步骤：

1. **检查重复**：调用 `findEntry` 在根目录中搜索是否已存在同名文件。
2. **找空闲位置**：调用 `findFreeEntry` 找到一个空闲的目录条目（第一个字节为 `0x00` 或 `0xE5`）。
3. **初始化条目**：将文件名转换为 8.3 格式，设置属性为 `ATTR_ARCHIVE`（普通文件），起始簇号设为 0（空文件不占用任何簇），文件大小设为 0。
4. **写入磁盘**：将新的目录条目写回磁盘。

## 写入文件 (writeFile)

写入文件是最复杂的操作之一，因为它涉及目录条目修改、FAT 表操作和数据写入。我们的实现采用**覆盖写**策略——每次写入都会先释放旧数据，再写入新数据。

```cpp
bool FAT16::writeFile(const char *name, const char *data, uint32 size)
{
    if (!mounted) { ... }

    int idx = findEntry(name);
    DirEntry entry;

    if (idx < 0)
    {
        // 文件不存在，先创建
        idx = findFreeEntry();
        if (idx < 0) { ... }
        memset(&entry, 0, sizeof(DirEntry));
        char fat16Name[11];
        toFAT16Name(name, fat16Name);
        memcpy(fat16Name, entry.filename, 8);
        memcpy(fat16Name + 8, entry.extension, 3);
        entry.attributes = ATTR_ARCHIVE;
    }
    else
    {
        readDirEntry(idx, &entry);
        // 释放旧的簇链
        if (entry.firstCluster >= 2)
        {
            freeClusterChain(entry.firstCluster);
        }
    }

    if (size == 0)
    {
        entry.firstCluster = 0;
        entry.fileSize = 0;
        writeDirEntry(idx, &entry);
        syncFAT();
        return true;
    }

    // 计算需要的簇数
    uint32 clustersNeeded = (size + SECTOR_SIZE - 1) / SECTOR_SIZE;

    // 分配簇链
    uint16 firstCluster = 0;
    uint16 prevCluster = 0;

    for (uint32 i = 0; i < clustersNeeded; ++i)
    {
        uint16 cluster = allocateCluster();
        if (cluster == 0) { /* 磁盘满，回滚 */ ... }

        if (i == 0)
            firstCluster = cluster;
        else
            fatTable[prevCluster] = cluster;

        // 将数据写入该簇对应的扇区
        uint8 sectorBuf[SECTOR_SIZE];
        memset(sectorBuf, 0, SECTOR_SIZE);
        uint32 copySize = size - i * SECTOR_SIZE;
        if (copySize > SECTOR_SIZE) copySize = SECTOR_SIZE;
        memcpy((void *)(data + i * SECTOR_SIZE), sectorBuf, copySize);
        disk->writeSectors(clusterToSector(cluster), 1, sectorBuf);

        prevCluster = cluster;
    }

    // 更新目录条目
    entry.firstCluster = firstCluster;
    entry.fileSize = size;
    writeDirEntry(idx, &entry);
    syncFAT();

    return true;
}
```

写入文件的详细步骤：

1. **查找或创建文件**：如果文件已存在，读取其目录条目并释放旧的簇链；如果文件不存在，创建一个新的目录条目。
2. **计算需要的簇数**：$\lceil \text{size} / 512 \rceil$，即将数据大小向上取整到扇区大小。
3. **分配簇链**：循环分配簇，每分配一个新簇就将前一个簇的 FAT 条目指向新簇（构建链表）。第一个分配的簇记录为 `firstCluster`。
4. **写入数据**：将数据按扇区大小分块，逐一写入到对应簇的扇区中。最后一个扇区如果数据不足 512 字节，则用零填充。
5. **更新元数据**：更新目录条目中的 `firstCluster` 和 `fileSize`，然后将目录条目和 FAT 表写回磁盘。

## 读取文件 (readFile)

```cpp
int FAT16::readFile(const char *name, char *buffer, uint32 maxSize)
{
    if (!mounted) { ... }

    int idx = findEntry(name);
    if (idx < 0)
    {
        printf("Error: file '%s' not found\n", name);
        return -1;
    }

    DirEntry entry;
    readDirEntry(idx, &entry);

    if (entry.fileSize == 0)
        return 0;

    uint32 readSize = entry.fileSize;
    if (readSize > maxSize)
        readSize = maxSize;

    uint16 cluster = entry.firstCluster;
    uint32 bytesRead = 0;
    uint8 sectorBuf[SECTOR_SIZE];

    while (cluster >= 2 && cluster < FAT16_BAD && bytesRead < readSize)
    {
        disk->readSectors(clusterToSector(cluster), 1, sectorBuf);

        uint32 copySize = readSize - bytesRead;
        if (copySize > SECTOR_SIZE)
            copySize = SECTOR_SIZE;

        memcpy(sectorBuf, buffer + bytesRead, copySize);
        bytesRead += copySize;

        cluster = fatTable[cluster]; // 沿簇链前进
    }

    return (int)bytesRead;
}
```

读取文件的步骤：

1. **查找文件**：在根目录中找到对应的目录条目。
2. **获取元数据**：从目录条目中获取文件大小和起始簇号。
3. **遍历簇链**：从起始簇开始，读取每个簇的数据到扇区缓冲区，再从缓冲区中复制所需的字节数到 `buffer`。然后通过 `fatTable[cluster]` 获取下一个簇号，继续读取。
4. **返回字节数**：当遇到 EOF 标记或已读取足够数据时停止，返回实际读取的字节数。

## 删除文件 (deleteFile)

```cpp
bool FAT16::deleteFile(const char *name)
{
    if (!mounted) { ... }

    int idx = findEntry(name);
    if (idx < 0)
    {
        printf("Error: file '%s' not found\n", name);
        return false;
    }

    DirEntry entry;
    readDirEntry(idx, &entry);

    // 释放簇链
    if (entry.firstCluster >= 2)
    {
        freeClusterChain(entry.firstCluster);
        syncFAT();
    }

    // 标记目录条目为已删除
    entry.filename[0] = (char)ATTR_DELETED;
    writeDirEntry(idx, &entry);

    return true;
}
```

删除文件的步骤非常简洁：

1. **查找文件**：在根目录中找到对应的目录条目。
2. **释放簇链**：调用 `freeClusterChain` 将文件占用的所有簇释放（FAT 表中对应条目设为 `0x0000`），然后将 FAT 表写回磁盘。
3. **标记删除**：将目录条目的第一个字节设为 `0xE5`，表示该条目已被删除。

> 注意，删除文件并不会真正清除磁盘上的数据内容，只是释放了 FAT 表中的簇和标记了目录条目。这也是为什么在很多操作系统中，删除的文件有时可以被恢复的原因。

## 列出文件 (listFiles)

```cpp
void FAT16::listFiles()
{
    if (!mounted) { ... }

    printf("%-14s %8s  %s\n", "FILENAME", "SIZE", "CLUSTER");
    printf("-------------------------------\n");

    int count = 0;
    DirEntry entry;
    for (int i = 0; i < MAX_ROOT_ENTRIES; ++i)
    {
        readDirEntry(i, &entry);

        if ((uint8)entry.filename[0] == 0x00)
            break;     // 后面没有更多条目
        if ((uint8)entry.filename[0] == ATTR_DELETED)
            continue;  // 跳过已删除条目

        char name[16];
        fromFAT16Name(entry.filename, name);
        printf("%-14s %8d  %d\n", name, entry.fileSize, entry.firstCluster);
        count++;
    }

    printf("-------------------------------\n");
    printf("Total: %d file(s)\n", count);
}
```

列出文件的逻辑：遍历根目录的所有条目，跳过已删除的（`0xE5`），遇到空条目（`0x00`）则停止。对于有效的条目，将 8.3 格式的文件名转换为可读格式，然后输出文件名、大小和起始簇号。

## 查看文件系统信息 (showInfo)

```cpp
void FAT16::showInfo()
{
    if (!mounted) { ... }

    int usedClusters = 0;
    int freeClusters = 0;
    for (int i = 2; i < FAT_ENTRIES; ++i)
    {
        if (fatTable[i] == FAT16_FREE)
            freeClusters++;
        else
            usedClusters++;
    }

    printf("=== FAT16 Filesystem Info ===\n");
    printf("  Start sector  : %d\n", FS_START_SECTOR);
    printf("  Cluster size  : %d bytes\n", SECTOR_SIZE);
    printf("  Total clusters: %d\n", FAT_ENTRIES - 2);
    printf("  Used clusters : %d (%d bytes)\n", usedClusters, usedClusters * SECTOR_SIZE);
    printf("  Free clusters : %d (%d bytes)\n", freeClusters, freeClusters * SECTOR_SIZE);
    printf("  Max files     : %d\n", MAX_ROOT_ENTRIES);
}
```

这个函数统计 FAT 表中已使用和空闲的簇数量，然后输出文件系统的汇总信息。

# Shell 集成

文件系统实现完成后，我们需要将其集成到操作系统中进行演示。我们在 `Shell` 类中添加了一个 `runDemo` 方法，按照顺序执行一系列文件操作来展示文件系统的功能。

## Shell 类

Shell 类声明在 `include/shell.h` 中：

```cpp
class Shell
{
public:
    Shell();
    void initialize(DiskDriver *disk, FAT16 *fs);
    void run();
private:
    void printLogo();
    void runDemo();
    DiskDriver *disk;
    FAT16 *fs;
};
```

## 演示流程

`runDemo` 方法按照以下步骤演示文件系统的各项功能：

| 步骤 | 操作 | 说明 |
|------|------|------|
| Step 1 | `fs->format(disk)` | 格式化文件系统 |
| Step 2 | `fs->showInfo()` | 显示文件系统信息 |
| Step 3 | `fs->createFile(...)` | 创建 3 个空文件 |
| Step 4 | `fs->listFiles()` | 列出所有文件（空文件） |
| Step 5 | `fs->writeFile(...)` | 向文件写入内容 |
| Step 6 | `fs->listFiles()` | 列出文件（写入后，显示大小变化） |
| Step 7 | `fs->readFile(...)` | 读取文件内容并显示 |
| Step 8 | `fs->deleteFile(...)` | 删除一个文件 |
| Step 9 | `fs->listFiles()` | 列出文件（删除后） |
| Step 10 | `fs->readFile(...)` | 尝试读取已删除的文件（预期失败） |
| Step 11 | `fs->writeFile(...)` | 覆盖写已有文件 |
| Step 12 | `fs->showInfo()` | 显示最终的文件系统信息 |

## 内核入口

在 `src/kernel/setup.cpp` 中，我们声明了全局的磁盘驱动和文件系统对象：

```cpp
// 磁盘驱动
DiskDriver diskDriver;
// FAT16 文件系统
FAT16 fat16;
```

在第一个内核线程 `first_thread` 中初始化磁盘驱动：

```cpp
void first_thread(void *arg)
{
    // 初始化磁盘驱动
    diskDriver.initialize();

    printf("start process\n");
    programManager.executeProcess((const char *)first_process, 1);
    asm_halt();
}
```

然后在第一个用户进程 `first_process` 中 fork 出子进程来运行 Shell：

```cpp
void first_process()
{
    int pid = fork();

    if (pid == -1) { ... }
    else
    {
        if (pid) // 父进程
        {
            while ((pid = wait(nullptr)) != -1) { }
            asm_halt();
        }
        else // 子进程
        {
            Shell shell;
            shell.initialize(&diskDriver, &fat16);
            shell.run();
        }
    }
}
```

子进程创建 `Shell` 对象，传入磁盘驱动和文件系统对象的指针，然后调用 `shell.run()` 开始文件系统演示。

# 编译和运行

> 代码放置在 `src/1` 下。

## 创建磁盘镜像

由于我们的文件系统需要使用磁盘空间来存储数据，因此需要先创建一个足够大的磁盘镜像文件。

```shell
cd lab9/src/1/build

# 创建 10MB 磁盘镜像（20160 个扇区）
make image
```

`make image` 实际执行的命令是：

```shell
dd if=/dev/zero of=../run/hd.img bs=512 count=20160
```

这个命令使用 `dd` 工具创建一个全零的文件，大小为 $512 \times 20160 = 10{,}321{,}920$ 字节（约 10MB）。这个文件将作为 QEMU 的虚拟硬盘使用。

> 为什么需要 10MB？我们的文件系统从扇区 500 开始，FAT 表支持 8192 个簇，每个簇 512 字节，数据区约 $8190 \times 512 \approx 4\text{MB}$。加上前面的 MBR、Bootloader、内核等区域，10MB 的磁盘镜像已经足够使用。

## 编译和写入

```shell
# 编译内核和引导程序，写入磁盘镜像
make build
```

`make build` 会：
1. 编译 MBR 并写入磁盘镜像的第 0 扇区。
2. 编译 Bootloader 并写入第 1~5 扇区。
3. 编译 Kernel 并写入第 6~150 扇区。

## 运行

```shell
# 有图形界面运行
make run

# 无图形界面运行（通过 telnet 查看 QEMU Monitor）
make monitor
```

如果使用 `make monitor`，在另一个终端中通过 telnet 连接：

```shell
telnet 127.0.0.1 45491
```

## Makefile 说明

Makefile 的关键部分如下：

```makefile
ARCH := $(shell uname -m)

ifeq ($(ARCH),aarch64)
    # 银河麒麟 ARM64 (交叉编译)
    CROSS_PREFIX = $(HOME)/lab1/cross-tools/install/bin/i686-elf-
    CXX_COMPILER = $(CROSS_PREFIX)g++
    CXX_COMPILER_FLAGS = ... -fno-exceptions -fno-rtti
    QEMU = qemu-system-x86_64 -drive file=$(RUNDIR)/hd.img,format=raw -m 32
else
    # Ubuntu AMD64 (本地编译)
    CXX_COMPILER = g++
    CXX_COMPILER_FLAGS = ... -m32
    QEMU = qemu-system-i386 -hda $(RUNDIR)/hd.img
endif
```

Makefile 通过 `uname -m` 自动检测当前平台架构，在 ARM64（银河麒麟）平台上自动切换到交叉编译工具链。ARM64 平台需要额外的 `-fno-exceptions -fno-rtti` 编译选项，因为交叉编译工具链中可能缺少 C++ 运行时支持库。

# 运行效果

编译运行后，预期输出类似如下内容：

```
 ____  _   _ __  __ __  __ _____ ____
/ ___|| | | |  \/  |  \/  | ____|  _ \
\___ \| | | | |\/| | |\/| |  _| | |_) |
 ___) | |_| |  |  | |  |  | |___|  _ <
|____/ \___/|_|  |_|_|  |_|_____|_| \_\

SUMMER OS - FAT16 File System Lab

==============================================
      FAT16 File System Demo
==============================================

[Step 1] Formatting file system...
Formatting FAT16 filesystem...
Format complete. FAT16 filesystem ready.

[Step 2] File system info:
=== FAT16 Filesystem Info ===
  Start sector  : 500
  Cluster size  : 512 bytes
  Total clusters: 8190
  Used clusters : 0 (0 bytes)
  Free clusters : 8190 (4193280 bytes)
  Max files     : 512

[Step 3] Creating files...
  Created: hello.txt
  Created: data.bin
  Created: readme

[Step 4] Listing files:
FILENAME          SIZE  CLUSTER
-------------------------------
hello.txt            0  0
data.bin             0  0
readme               0  0
-------------------------------
Total: 3 file(s)

[Step 5] Writing to files...
  Wrote 19 bytes to hello.txt
  Wrote 41 bytes to readme
  Wrote 29 bytes to data.bin

[Step 6] Listing files after write:
FILENAME          SIZE  CLUSTER
-------------------------------
hello.txt           19  2
data.bin            29  4
readme              41  3
-------------------------------
Total: 3 file(s)

[Step 7] Reading files...
  hello.txt (19 bytes): Hello, FAT16 World!
  readme (41 bytes): This is a README file for our simple OS.

[Step 8] Deleting data.bin...
  Deleted: data.bin

[Step 9] Listing files after delete:
FILENAME          SIZE  CLUSTER
-------------------------------
hello.txt           19  2
readme              41  3
-------------------------------
Total: 2 file(s)

[Step 10] Try reading deleted file...
Error: file 'data.bin' not found

[Step 11] Overwriting hello.txt...
  hello.txt now: Content has been updated!

[Step 12] Final file system info:
=== FAT16 Filesystem Info ===
  Start sector  : 500
  Cluster size  : 512 bytes
  Total clusters: 8190
  Used clusters : 2 (1024 bytes)
  Free clusters : 8188 (4192256 bytes)
  Max files     : 512

==============================================
      Demo Complete!
==============================================
```

从输出中我们可以看到完整的文件系统操作流程：

1. 格式化后文件系统为空，所有 8190 个簇都是空闲的。
2. 创建 3 个空文件，此时文件大小和起始簇号都为 0。
3. 写入数据后，每个文件被分配了簇，文件大小也更新了。
4. 读取文件可以正确返回之前写入的内容。
5. 删除文件后，该文件从目录列表中消失，尝试读取会报错。
6. 覆盖写操作成功更新了文件内容。
7. 最终文件系统信息显示只有 2 个簇被占用（对应 hello.txt 和 readme 各 1 个簇）。

# 新增文件说明

本章相较于 Lab8 新增了以下文件：

| 文件 | 说明 |
|------|------|
| `include/disk.h` | ATA 磁盘驱动头文件，定义端口、命令和状态位宏 |
| `include/fat16.h` | FAT16 文件系统头文件，定义数据结构和接口 |
| `include/shell.h` | Shell 头文件 |
| `src/kernel/disk.cpp` | ATA PIO 模式磁盘驱动实现 |
| `src/kernel/fat16.cpp` | FAT16 文件系统核心实现 |
| `src/kernel/shell.cpp` | Shell 文件系统演示 |

在 `src/utils/asm_utils.asm` 中新增了 `asm_in_port_word` 和 `asm_out_port_word` 两个 16 位端口 I/O 函数。

# 总结

在本章中，我们在前几章构建的原型操作系统基础上，实现了一个简化的 FAT16 文件系统。让我们回顾一下所学到的关键内容：

1. **ATA PIO 磁盘驱动**：我们了解了 ATA 硬盘接口的基本概念和 PIO 模式的工作原理。通过 I/O 端口 `0x1F0`~`0x1F7`，我们可以向磁盘发送读/写命令，并以 16 位字为单位传输数据。PIO 模式虽然效率不高（CPU 全程参与数据传输），但实现简单，适合教学目的。

2. **FAT16 文件系统结构**：我们学习了 FAT 文件系统的核心数据结构——超级块（记录文件系统参数）、FAT 表（用链式结构管理簇的分配）和目录条目（记录文件的元数据）。理解这三个数据结构的布局和作用，是理解 FAT 文件系统的关键。

3. **文件操作实现**：我们实现了文件系统的基本操作——格式化（初始化磁盘布局）、创建文件（分配目录条目）、写入文件（分配簇链+写入数据）、读取文件（遍历簇链+读取数据）、删除文件（释放簇链+标记删除）以及列出文件（遍历目录条目）。这些操作覆盖了文件系统的核心功能。

4. **与操作系统内核的集成**：我们将文件系统集成到了已有的操作系统中，通过内核线程初始化磁盘驱动，在用户进程中创建 Shell 来执行文件系统演示。这体现了操作系统各子系统之间的协作关系——进程管理、内存管理、中断处理和文件系统共同构成了一个完整的操作系统。

虽然我们的 FAT16 实现做了很多简化（如单级目录、固定簇大小、覆盖写等），但它已经涵盖了文件系统最核心的概念。理解了这些基础知识后，同学们可以进一步学习更复杂的文件系统，如 ext4、NTFS 等。

> 磁盘是计算机系统中唯一的持久化存储设备（不考虑 NVM 等新技术），文件系统则是连接用户和磁盘的桥梁。从简单的 FAT 到复杂的日志文件系统、分布式文件系统，文件系统的设计始终围绕着**可靠性**、**性能**和**易用性**这三个目标展开。
