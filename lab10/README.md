# 第九章 Course Projects

> 更喜岷山千里雪，三军过后尽开颜。

我们终于来到了本学期实验课的最后一次实验。在前面的 9 次实验中，同学们已经从零构建了一个具有中断处理、内存管理、进程管理、系统调用和文件系统等功能的原型操作系统。在本章中，同学们可以从以下项目中选出若干个自己喜欢的项目来实现，进一步探索操作系统的深层机制。

# 实验环境

| 项目 | Ubuntu AMD64 | 银河麒麟 ARM64 |
|------|-------------|---------------|
| **操作系统** | Ubuntu 22.04 x86_64 | 银河麒麟 V10 aarch64 |
| **C++ 编译器** | `g++ -m32` | `i686-elf-g++`（交叉编译） |
| **汇编器** | `nasm` | `nasm` |
| **链接器** | `ld -melf_i386` | `i686-elf-ld` |
| **QEMU** | `qemu-system-i386` | `qemu-system-x86_64`（用户态模拟） |
| **调试器** | `gdb` | QEMU Monitor（`make monitor`） |

> **ARM64 注意**：银河麒麟 ARM64 平台需要使用交叉编译工具链，Makefile 已自动检测 `uname -m` 并切换编译器。ARM64 平台额外需要 `-fno-exceptions -fno-rtti` 编译选项。

# 实验要求

> - DDL：2026.7.1 23:59:59
> - 提交的内容：将**若干个 Project 的代码**和**实验报告**放到**压缩包**中，命名为 "**lab10-姓名-学号**"，并交到课程网站上。
> - 不同的 Project 的代码需要放在不同的文件夹下（`project1/`、`project2/`、...）。

1. 实验不限语言，C/C++/Rust 都可以。
2. 实验不限平台，Windows、Linux（含银河麒麟 ARM64）和 MacOS 等都可以。
3. 实验不限 CPU，ARM/Intel/RISC-V 都可以。

# 评分说明

- **可以实现多个项目，项目的分数可以累加，上不封顶。** 超出 lab10 满分（100 分）的部分会被折算到实验课的加分部分，但不会超过实验课的最大加分限制。
- **lab10 不强求使用本教程提供的代码**，可以在你自己实现的操作系统、ucore 或 xv6 的代码上实现相同的功能。因此，下面要求中"自己的操作系统"指的是你自己实现的操作系统、ucore 或 xv6 等。
- ⚠️ **由于在 GitHub 上可以找到 ucore 或 xv6 的相关功能的实现代码，若直接抄袭，则相应实现部分判零分。**
- 每个 Project 均需在实验报告中详细分析实现思路和代码，自行提供测例截图展示正确性。

---

# Project 1 — 虚拟内存的完善（页换入换出）

**评分上限：100 分**

## 1.1 背景

通过分页机制，我们实现了内存的虚拟化，使得程序可以看到比实际物理内存更大的虚拟地址空间。虚拟内存的另一个重要组成部分是 **页换入换出机制（Page Swapping）**：

- **页换出（Page Out）**：当物理内存紧张时，操作系统根据页面置换算法将某些物理页写入磁盘（交换区），释放物理内存。
- **页换入（Page In）**：当程序访问已被换出的页时，触发 **缺页中断（Page Fault, INT 14）**，中断处理函数将该页从磁盘重新加载到物理内存，更新页表映射。

为了简便起见，前面的实验没有实现页换入换出机制。本项目要求同学们补全这一机制。

## 1.2 要求

1. **实现页换出机制**：当物理内存不足时，选择一个物理页写入磁盘交换区，释放该物理页，并将对应的页表项标记为"不在内存中"（PTE 的 P 位置 0，同时记录磁盘扇区号）。
2. **实现缺页中断处理（INT 14）**：当 CPU 访问一个 P=0 的页表项时触发缺页中断。在中断处理函数中：
   - 从 CR2 寄存器获取触发缺页的虚拟地址
   - 根据页表项中记录的磁盘扇区号，从磁盘读回数据
   - 分配新的物理页，将数据写入，更新页表
3. **实现至少一种页面置换算法**：
   - **FIFO**（先进先出）：最早被加载的页最先被换出
   - **LRU**（最近最少使用）：最久未被访问的页最先被换出（可用近似 LRU：Clock 算法）
   - **LFU**（最不经常使用）：访问次数最少的页最先被换出
4. **自行设计测例**：创建一个进程，使其申请远超物理内存的虚拟内存，依次写入数据后再读取，验证数据的正确性。

## 1.3 磁盘读写参考

通过 I/O 端口读写硬盘的方法与 Lab3 和 Lab9 相同（ATA PIO 模式）。以下提供 C++ 封装的磁盘读写接口：

```cpp
#include "asm_utils.h"
#include "os_type.h"

#define SECTOR_SIZE 512

class Disk {
public:
    // 读取一个扇区（512 字节）
    // start: LBA 逻辑扇区号, buf: 目标缓冲区地址
    static void read(int start, void *buf);

    // 写入一个扇区（512 字节）
    // start: LBA 逻辑扇区号, buf: 源数据地址
    static void write(int start, void *buf);
};
```

> **注意**：磁盘读写只能在内核态使用。如需在用户态使用，请封装为系统调用。完整实现可参考 Lab9 的 `src/1` 中的磁盘驱动代码。建议使用 `hd.img` 的高扇区区域（如扇区号 2048 起）作为交换区，避免与文件系统冲突。

实现磁盘读写需要两个汇编函数用于按字（16 位）读写 I/O 端口：

```asm
; void asm_inw_port(int port, void *value);
asm_inw_port:
    push ebp
    mov ebp, esp
    push edx
    push ebx
    xor eax, eax
    mov edx, dword[ebp + 4 * 2]
    mov ebx, dword[ebp + 4 * 3]
    in ax, dx
    mov word[ebx], ax
    pop ebx
    pop edx
    pop ebp
    ret

; void asm_outw_port(int port, int value);
asm_outw_port:
    push ebp
    mov ebp, esp
    push edx
    mov edx, dword[ebp + 4 * 2]
    mov eax, dword[ebp + 4 * 3]
    out dx, ax
    pop edx
    pop ebp
    ret
```

## 1.4 提示

- 可以在页表项（PTE）的高位（Bit 9~11 为操作系统可用位）中记录额外信息。
- 交换区管理：使用一个 BitMap 跟踪磁盘交换区中哪些扇区已被占用。
- 刷新 TLB：修改页表后需要使用 `invlpg` 指令刷新 TLB 缓存。
- **银河麒麟 ARM64**：QEMU 模拟的 i386 虚拟机同样支持磁盘 I/O，无需特殊适配。

---

# Project 2 — malloc/free 的实现

**评分上限：100 分**

## 2.1 背景

在 Lab7 中，我们实现了以 **页（4KB）** 为粒度的动态内存分配和释放。但在实际程序中，我们需要的是以 **字节** 为粒度的内存管理，即 `malloc(size)` 和 `free(ptr)`。

本项目要求在页内存分配的基础上，实现以字节为粒度的动态内存管理。

## 2.2 方案选择

### 方案一：自行设计实现（100 分）

自行设计并实现 `malloc` 和 `free`。设计方案不限，常见思路包括：

- **Arena/Slab 分配器**：将页按固定大小划分为若干 arena（如 16/32/64/.../1024 字节），超出 1024 字节则直接分配整页。
- **Buddy System**（伙伴系统）：将内存按 2 的幂次大小划分和合并。
- **Free List**：使用空闲链表管理不同大小的内存块，支持合并相邻空闲块。

要求：
1. 实现 `void *malloc(int size)` 和 `void free(void *address)` 的系统调用。
2. 自行设计测例验证正确性（多次分配/释放、边界大小、碎片化场景）。
3. 在实验报告中详细分析内存管理的数据结构和算法。

### 方案二：参考实现并扩展（70 + 30 分）

- **基础部分（70 分）**：参考下面的 Arena 分配器思路，复现、测试并详细分析代码。
- **扩展部分（30 分）**：在参考实现的基础上，完成以下扩展中的至少一项：
  - 加入线程/进程同步互斥机制，使 `malloc/free` 线程安全
  - 实现 `realloc(ptr, new_size)`：调整已分配内存的大小
  - 实现内存泄漏检测：记录每次分配的调用位置，在进程退出时报告未释放的内存

## 2.3 Arena 分配器参考方案

### 基本思想

固定大小的内存分配单元更容易管理。我们将可分配的大小设定为 2^N 的形式：16、32、64、128、256、512、1024 字节。当请求分配 size 字节时，找到满足 2^(N-1) < size <= 2^N 的最小 N。当 size > 1024 时，直接分配连续的整页。

### 数据结构

每种类型的 arena 都有一个空闲链表。一个页被划分为一个元信息头（`Arena`）加若干个等大的内存块：

```
+-------------------------------------------+
|  Arena（元信息）                            |
|  type: ARENA_64   counter: 62             |
+-------------------------------------------+
|  Block 0 (64B)  |  Block 1 (64B)  | ...  |
|  [prev|next|...]  [prev|next|...]         |
+-------------------------------------------+
               一个 4KB 页
```

空闲的内存块通过 `MemoryBlockListItem`（两个指针，占 8 字节）串成双向链表：

```cpp
enum ArenaType {
    ARENA_16, ARENA_32, ARENA_64, ARENA_128,
    ARENA_256, ARENA_512, ARENA_1024, ARENA_MORE
};

struct Arena {
    ArenaType type;   // Arena 的类型
    int counter;      // ARENA_MORE 时表示页数量，否则表示当前页中可分配的块数量
};

struct MemoryBlockListItem {
    MemoryBlockListItem *previous, *next;
};
```

### 分配流程

```
malloc(size)
  +-- size > 1024 -> 分配多个整页，在页头写入 Arena{ARENA_MORE, 页数}
  +-- size <= 1024 -> 查找对应类型的空闲链表
       +-- 链表非空 -> 取出第一个空闲块，counter--
       +-- 链表为空 -> 分配一个新页，划分为等大的块，串入链表
```

### 释放流程

```
free(address)
  +-- 根据 address & 0xFFFFF000 找到所在页的 Arena 头
  +-- ARENA_MORE -> 直接释放整页
  +-- 其他类型 -> 将块插回空闲链表头部，counter++
       +-- 如果 counter == 页内总块数 -> 整页所有块已归还，释放整页
```

### 完整实现参考

以下是 `ByteMemoryManager` 类的完整实现：

```cpp
class ByteMemoryManager {
private:
    static const int MEM_BLOCK_TYPES = 7;
    static const int minSize = 16;
    int arenaSize[MEM_BLOCK_TYPES];
    MemoryBlockListItem *arenas[MEM_BLOCK_TYPES];

public:
    ByteMemoryManager() { initialize(); }

    void initialize() {
        int size = minSize;
        for (int i = 0; i < MEM_BLOCK_TYPES; ++i) {
            arenas[i] = nullptr;
            arenaSize[i] = size;
            size <<= 1;
        }
    }

    void *allocate(int size) {
        int index = 0;
        while (index < MEM_BLOCK_TYPES && arenaSize[index] < size) ++index;

        PCB *pcb = programManager.running;
        AddressPoolType poolType = pcb->pageDirectoryAddress
            ? AddressPoolType::USER : AddressPoolType::KERNEL;
        void *ans = nullptr;

        if (index == MEM_BLOCK_TYPES) {
            int pageAmount = (size + sizeof(Arena) + PAGE_SIZE - 1) / PAGE_SIZE;
            ans = memoryManager.allocatePages(poolType, pageAmount);
            if (ans) {
                Arena *arena = (Arena *)ans;
                arena->type = ArenaType::ARENA_MORE;
                arena->counter = pageAmount;
            }
        } else {
            if (!arenas[index] && !getNewArena(poolType, index))
                return nullptr;
            ans = arenas[index];
            arenas[index] = ((MemoryBlockListItem *)ans)->next;
            if (arenas[index]) arenas[index]->previous = nullptr;
            Arena *arena = (Arena *)((int)ans & 0xfffff000);
            --(arena->counter);
        }
        return ans;
    }

    void release(void *address) {
        Arena *arena = (Arena *)((int)address & 0xfffff000);
        if (arena->type == ARENA_MORE) {
            memoryManager.releasePages((int)arena, arena->counter);
        } else {
            MemoryBlockListItem *item = (MemoryBlockListItem *)address;
            item->next = arenas[arena->type];
            item->previous = nullptr;
            if (item->next) item->next->previous = item;
            arenas[arena->type] = item;
            ++(arena->counter);

            int amount = (PAGE_SIZE - sizeof(Arena)) / arenaSize[arena->type];
            if (arena->counter == amount) {
                MemoryBlockListItem *p = arenas[arena->type];
                while (p) {
                    if ((int)arena == ((int)p & 0xfffff000)) {
                        if (p->previous) p->previous->next = p->next;
                        else arenas[arena->type] = p->next;
                        if (p->next) p->next->previous = p->previous;
                    }
                    p = p->next;
                }
                memoryManager.releasePages((int)arena, 1);
            }
        }
    }

private:
    bool getNewArena(AddressPoolType type, int index) {
        void *ptr = memoryManager.allocatePages(type, 1);
        if (!ptr) return false;

        int times = (PAGE_SIZE - sizeof(Arena)) / arenaSize[index];
        int address = (int)ptr + sizeof(Arena);

        Arena *arena = (Arena *)ptr;
        arena->type = (ArenaType)index;
        arena->counter = times;

        MemoryBlockListItem *prev = (MemoryBlockListItem *)address;
        arenas[index] = prev;
        prev->previous = nullptr;
        prev->next = nullptr;
        --times;

        while (times) {
            address += arenaSize[index];
            MemoryBlockListItem *cur = (MemoryBlockListItem *)address;
            prev->next = cur;
            cur->previous = prev;
            cur->next = nullptr;
            prev = cur;
            --times;
        }
        return true;
    }
};
```

### 系统调用封装

由于进程有独立的内存空间，需要在 PCB 中加入 `ByteMemoryManager`：

```cpp
struct PCB {
    // ...
    ByteMemoryManager byteMemoryManager;
    // ...
};
```

系统调用处理函数：

```cpp
void *syscall_malloc(int size) {
    PCB *pcb = programManager.running;
    if (pcb->pageDirectoryAddress)
        return pcb->byteMemoryManager.allocate(size);
    else
        return kernelByteMemoryManager.allocate(size);
}

void syscall_free(void *address) {
    PCB *pcb = programManager.running;
    if (pcb->pageDirectoryAddress)
        pcb->byteMemoryManager.release(address);
    else
        kernelByteMemoryManager.release(address);
}
```

其中 `kernelByteMemoryManager` 是一个全局的 `ByteMemoryManager` 实例，供所有内核线程共享。

---

# Project 3 — 文件系统的实现

**评分上限：100 分**

## 3.1 背景

在 Lab9 中我们已经实现了简化的 FAT16 文件系统。本项目要求同学们从更底层开始，**设计并实现一个自己的文件系统**，或者在 Lab9 的基础上进行大幅扩展。

## 3.2 方案选择

### 方案一：分步骤实现

- **模拟文件系统（85 分）**：在 Ubuntu / 银河麒麟的用户空间中，用一个固定大小的文件模拟磁盘（如 `dd if=/dev/zero of=disk.img bs=1M count=16`），实现文件系统的基本功能。
- **迁移到操作系统（15 分）**：将模拟文件系统迁移到自己的 OS 上，通过 ATA PIO 读写真实的 `hd.img`。

### 方案二：一步到位（100 分）

直接在自己的操作系统上编写文件系统。

### 功能要求

无论选择哪种方案，需要实现以下基本功能：

| 功能 | 说明 |
|------|------|
| `open(path, flags)` | 打开文件，返回文件描述符 |
| `read(fd, buf, size)` | 读取文件内容 |
| `write(fd, buf, size)` | 写入文件内容 |
| `close(fd)` | 关闭文件 |
| `mkdir(path)` | 创建目录 |
| `ls(path)` | 列出目录内容 |
| `cd(path)` | 切换当前目录 |
| `pwd()` | 显示当前路径 |
| `cat(file)` | 显示文件内容 |
| `rm(file)` | 删除文件 |

> 不需要实现文本编辑器，不需要实现键盘输入功能（方案一中在主机终端交互即可）。

## 3.3 可参考的文件系统

- **FAT12/FAT16/FAT32**：最简单，链式分配
- **ext2**：基于 inode 的经典 Unix 文件系统，支持符号链接、权限
- **自定义**：设计自己的磁盘布局和分配策略

---

# Project 4 — Shell 的实现

**评分上限：100 分**

## 4.1 背景

虽然我们的操作系统已经具备了中断处理、进程管理、文件系统等核心功能，但还缺少一个用户交互界面。Shell 是操作系统中最基本的用户界面，用于接收和解析用户命令。

## 4.2 方案选择

### 方案一：分步骤实现

- **模拟 Shell（85 分）**：在 Ubuntu / 银河麒麟下实现 Shell 的基本功能，处理键盘输入、命令解析、命令执行。
- **迁移到操作系统（15 分）**：将 Shell 迁移到自己的 OS 上，通过键盘中断（IRQ 1）接收输入。

### 方案二：一步到位（100 分）

直接在自己的操作系统上编写 Shell。

### 功能要求

Shell 应支持以下功能：

1. **键盘输入处理**：通过 8042 PS/2 键盘控制器（端口 `0x60`/`0x64`）接收扫描码，转换为 ASCII 字符。
2. **命令行编辑**：支持退格删除、回车提交。
3. **内置命令**：

   | 命令 | 功能 |
   |------|------|
   | `help` | 显示可用命令列表 |
   | `clear` | 清屏 |
   | `echo <text>` | 打印文本 |
   | `ps` | 显示所有进程/线程的 PID、状态 |
   | `kill <pid>` | 终止指定进程 |

4. **文件系统命令**（需要 Project 3 或 Lab9 的文件系统支持）：`ls`、`cd`、`pwd`、`cat`、`touch`、`rm`、`mkdir`
5. **程序加载与执行**（加分项）：从文件系统中加载 ELF 可执行文件，创建进程运行。

## 4.3 键盘驱动提示

```
IRQ 1 -> 中断处理函数
  +-- 从端口 0x60 读取扫描码（scan code）
  +-- 查扫描码表，转换为 ASCII 字符
  +-- 处理特殊键（Shift、Ctrl、CapsLock、退格、回车）
  +-- 将字符写入输入缓冲区（环形缓冲区）
```

> **银河麒麟 ARM64**：QEMU 模拟的 i386 虚拟机同样支持 PS/2 键盘中断，无需特殊适配。在 `make run`（SDL 窗口）模式下即可使用键盘输入。

---

# Project 5 — Copy-On-Write（写时复制）

**评分上限：100 分**

## 5.1 背景

在 Lab8 的 `fork` 实现中，我们直接复制了父进程的所有物理页到子进程。这种做法虽然简单，但存在以下问题：

- **内存浪费**：父进程的只读数据（如代码段）被不必要地复制了一份
- **性能开销**：如果子进程立即调用 `exec` 加载新程序，之前复制的内存全部白费
- **延迟大**：大进程的 fork 需要复制大量内存，延迟很高

**Copy-On-Write（COW）** 是现代操作系统解决这些问题的标准方案。

## 5.2 要求

在本学期自己的实验基础上，实现基于 COW 机制的 `fork` 函数：

1. **fork 时不复制物理页**：父子进程共享相同的物理页，所有共享页的页表项标记为只读（R/W 位 = 0）。
2. **维护引用计数**：为每个物理页维护一个引用计数（`ref_count`），`fork` 时 `ref_count++`。
3. **写时复制**：当父/子进程写入一个共享页时，触发 **页保护异常（Page Fault, INT 14）**：
   - 从 CR2 获取触发异常的虚拟地址
   - 检查 error code 确认是写保护异常（Bit 1 = 1）
   - 分配一个新物理页，复制原页内容
   - 更新页表指向新页，设置 R/W = 1
   - 原物理页 `ref_count--`，如果 `ref_count == 1`，恢复其 R/W = 1
4. **测例验证**：
   - 父进程 fork 后，父子进程各自修改同一个全局变量，验证修改互不影响
   - 对比 COW fork 和普通 fork 的性能（分配物理页的次数）

## 5.3 提示

```
fork() with COW:
  +-- 分配新的页目录表
  +-- 复制父进程的页表项（不分配新物理页）
  +-- 将父子进程的所有用户空间页标记为只读（R/W = 0）
  +-- 为每个共享物理页 ref_count++
  +-- 刷新 TLB（invlpg 或重载 CR3）

Page Fault Handler (COW):
  +-- 从 CR2 获取故障虚拟地址
  +-- 检查 error code: bit0=1(P), bit1=1(Write), bit2=1(User)
  +-- 查页表找到物理页，检查 ref_count
  |   +-- ref_count == 1 -> 恢复 R/W = 1（最后一个引用者）
  |   +-- ref_count > 1 -> 分配新页，复制数据，ref_count--
  +-- 刷新 TLB
```

---

# Project 6 — 进程间通信（IPC）

**评分上限：100 分**

## 6.1 背景

进程间通信（Inter-Process Communication, IPC）是操作系统的重要功能。在前面的实验中，我们的进程之间只能通过信号量进行简单的同步。本项目要求实现更丰富的 IPC 机制。

## 6.2 要求

实现以下 IPC 机制中的 **至少两种**：

### 管道（Pipe）— 40 分

```cpp
int pipe(int fd[2]);  // fd[0] 读端，fd[1] 写端
int write(int fd, void *buf, int size);
int read(int fd, void *buf, int size);
```

- 创建一个内核缓冲区（如 4KB 环形缓冲区）
- 写端写入数据，读端读取数据
- 缓冲区满时 `write` 阻塞，缓冲区空时 `read` 阻塞
- 支持父子进程间的单向通信

### 共享内存（Shared Memory）— 30 分

```cpp
int shmget(int key, int size);  // 创建/获取共享内存段
void *shmat(int shmid);         // 将共享内存映射到进程地址空间
int shmdt(void *addr);          // 解除映射
```

- 多个进程可以将同一块物理内存映射到各自的虚拟地址空间
- 需要配合信号量/互斥锁使用，防止数据竞争

### 消息队列（Message Queue）— 30 分

```cpp
int msgget(int key);                         // 创建/获取消息队列
int msgsnd(int msqid, void *msg, int size);  // 发送消息
int msgrcv(int msqid, void *msg, int size);  // 接收消息
```

- 消息队列存储在内核空间
- 支持多个进程向同一队列发送/接收消息
- 队列满时 `msgsnd` 阻塞，队列空时 `msgrcv` 阻塞

### 信号（Signal）— 30 分

```cpp
int signal(int signum, void (*handler)(int));  // 注册信号处理函数
int kill(int pid, int signum);                 // 向进程发送信号
```

- 实现至少 3 种信号：`SIGKILL`（终止）、`SIGSTOP`（暂停）、`SIGUSR1`（用户自定义）
- 进程可以注册自定义信号处理函数
- 信号在进程从内核态返回用户态时被递送

## 6.3 测例要求

- 创建多个进程，演示通过 IPC 机制交换数据
- 验证阻塞/唤醒机制的正确性
- 截图展示通信过程和数据一致性

---

# Project 7 — 进程调度算法

**评分上限：80 分**

## 7.1 背景

在前面的实验中，我们使用了简单的时间片轮转（Round Robin）调度算法。本项目要求实现更复杂的调度算法，并对比分析不同算法的行为。

## 7.2 要求

实现以下调度算法中的 **至少两种**（每种 30 分）并进行对比分析（20 分）：

### 优先级调度（Priority Scheduling）

- 每个进程有一个优先级值（0 = 最高，数值越大优先级越低）
- 调度器始终选择最高优先级的就绪进程
- 需要处理 **优先级反转** 问题（可选：实现优先级继承协议）

### 多级反馈队列（MLFQ）

- 设置 2~4 个优先级队列，每个队列有不同的时间片（高优先级 = 短时间片）
- 新进程进入最高优先级队列
- 时间片用完则降级到下一级队列
- 低优先级队列中等待过久的进程提升优先级（防止饥饿）

### 完全公平调度（CFS, Completely Fair Scheduler）

- 每个进程维护一个 `vruntime`（虚拟运行时间）
- 调度器选择 `vruntime` 最小的进程
- 使用红黑树或有序链表管理就绪进程
- 高优先级进程的 `vruntime` 增长更慢

### 对比分析（20 分）

- 设计至少 3 个测试场景（CPU 密集型、I/O 密集型、混合型）
- 记录每个进程的等待时间、周转时间、CPU 利用率
- 在实验报告中对比不同算法的表现，分析各自的优缺点

---

# Project 8 — 网络协议栈（TCP/IP）

**评分上限：100 分**

## 8.1 背景

网络是现代操作系统不可或缺的功能。本项目要求在自己的操作系统上实现一个简化的网络协议栈。由于完整的 TCP/IP 协议栈非常复杂，这里允许分层实现。

## 8.2 分层实现与评分

### 第一层：网卡驱动（30 分）

实现 QEMU 虚拟网卡的驱动程序：
- **推荐网卡**：RTL8139（最简单）或 E1000（更接近真实硬件）
- 通过 PCI 配置空间发现网卡设备
- 实现数据包的发送和接收
- QEMU 启动参数：`-net nic,model=rtl8139 -net user`

### 第二层：以太网 + ARP + IP（30 分）

- 解析以太网帧头（目的 MAC、源 MAC、类型）
- 实现 ARP 协议（IP 地址 -> MAC 地址映射）
- 实现 IP 协议（发送/接收 IP 数据报，支持 ICMP Echo，即 `ping`）

### 第三层：UDP / TCP（40 分）

- **UDP（15 分）**：实现无连接的数据报通信
- **TCP（25 分）**：实现面向连接的可靠传输
  - 三次握手 / 四次挥手
  - 序列号与确认号
  - 简单的重传机制

## 8.3 提示

- QEMU 的 `-net user` 模式提供了一个内置的 DHCP/DNS 服务器和 NAT 网关
- 可以使用主机的 Wireshark 抓包调试
- 如果完整 TCP 过于困难，只实现网卡驱动 + IP + ICMP 也有 60 分
- **银河麒麟 ARM64**：QEMU 同样支持虚拟网卡，使用相同的参数

---

# Project 9 — 图形界面（GUI）

**评分上限：80 分**

## 9.1 背景

目前我们的操作系统只有文本模式界面。本项目要求实现一个简单的图形用户界面，让操作系统具备基本的图形显示和鼠标/键盘交互能力。

## 9.2 分层实现与评分

### 第一层：VBE 图形模式（30 分）

- 在 bootloader 中使用 BIOS VBE（VESA BIOS Extensions）中断切换到图形模式
- 推荐分辨率：800x600 或 1024x768，32 位色深
- 实现基本绘图函数：`drawPixel`、`drawLine`、`drawRect`、`fillRect`、`drawChar`
- QEMU 参数：默认即支持 VBE

### 第二层：窗口系统（30 分）

- 实现窗口管理器：创建窗口、移动窗口、窗口层叠（Z-order）
- 实现双缓冲（避免闪烁）
- 实现鼠标驱动（PS/2 鼠标，IRQ 12，端口 `0x60`/`0x64`）
- 鼠标光标的绘制和移动

### 第三层：GUI 应用（20 分）

- 实现至少一个 GUI 应用，例如：
  - 简易文本编辑器（在窗口中显示和编辑文本）
  - 简易绘图程序（鼠标画线、画矩形）
  - 任务管理器（图形化显示进程列表）
  - 简易计算器

## 9.3 提示

- VBE 模式下，显存被线性映射到物理地址（如 `0xFD000000`），直接写入像素数据即可
- 字体可以使用 8x16 的点阵字体（bitmap font），不需要实现矢量字体
- PS/2 鼠标协议：每次中断发送 3 字节数据（按键状态、X 偏移、Y 偏移）
- **银河麒麟 ARM64**：QEMU 在 SDL 模式下支持鼠标输入

---

# Project 10 — 另辟蹊径

**评分上限：100 分**

考虑到同学们可能有其他的想法，欢迎天马行空。以下提供一些探索方向供参考：

### 方向一：平台移植

- **Rust 重构**：使用 Rust 语言重写部分或全部操作系统模块（推荐从内存管理开始）
- **ARM64 原生移植**：不再交叉编译 x86，而是直接为 AArch64 架构编写操作系统（使用 `qemu-system-aarch64`）
- **RISC-V 移植**：为 RISC-V 架构编写操作系统（使用 `qemu-system-riscv64`）
- **UEFI 启动**：将操作系统从 Legacy BIOS 启动改为 UEFI 启动

### 方向二：功能增强

- **多核支持（SMP）**：初始化 APIC，启动多个 CPU 核心，实现多核调度
- **动态链接器**：实现 ELF 动态链接，支持共享库（`.so`）的加载
- **内存映射文件（mmap）**：将文件映射到进程的虚拟地址空间
- **用户态线程库（pthread）**：实现 `pthread_create`、`pthread_join`、`pthread_mutex` 等

### 方向三：真实硬件运行

- 将操作系统烧录到 U 盘，在真实的 x86 电脑上启动
- 在树莓派（ARM64）上运行自己的操作系统
- 在 RISC-V 开发板上运行

### 方向四：64 位操作系统

- 从 32 位保护模式升级到 64 位长模式（Long Mode）
- 实现 64 位分页（4 级页表：PML4 -> PDPT -> PD -> PT）
- 使用 `syscall`/`sysret` 指令代替 `int 0x80`

### 评分标准

- 根据项目的难度、完成度、创新性综合评分
- 需要在实验报告中清晰说明项目的目标、设计方案、实现过程和测试结果
- 鼓励有创意的想法，即使没有完全实现也会根据思路和部分成果给分

---

# 附录：QEMU 常用调试命令

在所有 Project 中，都可以使用 QEMU Monitor 进行调试。通过 `make monitor` 启动 QEMU 后，使用 telnet 连接：

```shell
telnet 127.0.0.1 <端口号>
```

| 命令 | 功能 |
|------|------|
| `info registers` | 查看所有 CPU 寄存器（含 CR0/CR2/CR3/CS 等） |
| `info mem` | 查看虚拟地址映射 |
| `info tlb` | 查看 TLB 缓存 |
| `xp /Nwx PADDR` | 查看物理地址处的 N 个 word |
| `x /Nwx VADDR` | 查看虚拟地址处的 N 个 word |
| `info cpus` | 查看 CPU 状态 |
| `info block` | 查看块设备（磁盘）信息 |
| `info network` | 查看网络设备信息 |
| `stop` / `cont` | 暂停 / 继续虚拟机 |
| `quit` | 退出 QEMU |

> **Ubuntu AMD64** 和 **银河麒麟 ARM64** 都支持上述所有调试命令，操作方法一致。
