#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"
#include "memory.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;
// 内存管理器
MemoryManager memoryManager;

void first_thread(void *arg)
{
    // 第1个线程不可以返回

    printf("=== Memory Management Test ===\n\n");

    // 显示初始内存状态
    memoryManager.printMemoryStatus();

    // 测试1: 分配多组虚拟页
    printf("\n[Test 1] Allocate pages\n");
    char *p1 = (char *)memoryManager.allocatePages(AddressPoolType::KERNEL, 100);
    char *p2 = (char *)memoryManager.allocatePages(AddressPoolType::KERNEL, 10);
    char *p3 = (char *)memoryManager.allocatePages(AddressPoolType::KERNEL, 100);
    printf("p1(100 pages)=0x%x\n", p1);
    printf("p2( 10 pages)=0x%x\n", p2);
    printf("p3(100 pages)=0x%x\n", p3);

    // 测试2: 虚拟地址到物理地址验证
    printf("\n[Test 2] Vaddr -> Paddr verify\n");
    int vaddr = (int)p1;
    int paddr = memoryManager.vaddr2paddr(vaddr);
    printf("vaddr=0x%x -> paddr=0x%x\n", vaddr, paddr);

    // 写入标记值并提示 QEMU Monitor 验证
    *((int *)p1) = 0x12345678;
    printf("wrote 0x12345678 to vaddr 0x%x\n", vaddr);
    printf("use 'xp /1wx 0x%x' in QEMU Monitor to verify\n", paddr);

    // 测试3: 释放中间块后重新分配
    printf("\n[Test 3] Release p2, reallocate\n");
    memoryManager.releasePages(AddressPoolType::KERNEL, (int)p2, 10);
    printf("released p2(10 pages)\n");

    // 分配大于释放的块 -> 不会复用p2的空间
    char *p4 = (char *)memoryManager.allocatePages(AddressPoolType::KERNEL, 100);
    printf("p4(100 pages)=0x%x\n", p4);

    // 分配等于释放的块 -> 应复用p2的空间
    char *p5 = (char *)memoryManager.allocatePages(AddressPoolType::KERNEL, 10);
    printf("p5( 10 pages)=0x%x (should reuse p2)\n", p5);

    // 显示最终内存状态
    printf("\n");
    memoryManager.printMemoryStatus();

    asm_halt();
}

extern "C" void setup_kernel()
{

    // 中断管理器
    interruptManager.initialize();
    interruptManager.enableTimeInterrupt();
    interruptManager.setTimeInterrupt((void *)asm_time_interrupt_handler);

    // 输出管理器
    stdio.initialize();

    // 进程/线程管理器
    programManager.initialize();

    // 内存管理器
    memoryManager.openPageMechanism();
    memoryManager.initialize();

    // 创建第一个线程
    int pid = programManager.executeThread(first_thread, nullptr, "first thread", 1);
    if (pid == -1)
    {
        printf("can not execute thread\n");
        asm_halt();
    }

    ListItem *item = programManager.readyPrograms.front();
    PCB *firstThread = ListItem2PCB(item, tagInGeneralList);
    firstThread->status = RUNNING;
    programManager.readyPrograms.pop_front();
    programManager.running = firstThread;
    asm_switch_thread(0, firstThread);

    asm_halt();
}
