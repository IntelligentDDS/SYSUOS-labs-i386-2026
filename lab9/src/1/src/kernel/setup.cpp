#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"
#include "memory.h"
#include "syscall.h"
#include "tss.h"
#include "shell.h"
#include "disk.h"
#include "fat16.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;
// 内存管理器
MemoryManager memoryManager;
// 系统调用
SystemService systemService;
// Task State Segment
TSS tss;
// 磁盘驱动
DiskDriver diskDriver;
// FAT16 文件系统
FAT16 fat16;

int syscall_0(int first, int second, int third, int forth, int fifth)
{
    printf("system call 0: %d, %d, %d, %d, %d\n",
           first, second, third, forth, fifth);
    return first + second + third + forth + fifth;
}

void first_process()
{
    int pid = fork();

    if (pid == -1)
    {
        printf("error\n");
        asm_halt();
    }
    else
    {
        if (pid)
        {
            while ((pid = wait(nullptr)) != -1)
            {
            }
            asm_halt();
        }
        else
        {
            Shell shell;
            shell.initialize(&diskDriver, &fat16);
            shell.run();
        }
    }
}

void first_thread(void *arg)
{
    // 初始化磁盘驱动
    diskDriver.initialize();

    printf("start process\n");
    programManager.executeProcess((const char *)first_process, 1);
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

    // 初始化系统调用
    systemService.initialize();
    systemService.setSystemCall(0, (int)syscall_0);
    systemService.setSystemCall(1, (int)syscall_write);
    systemService.setSystemCall(2, (int)syscall_fork);
    systemService.setSystemCall(3, (int)syscall_exit);
    systemService.setSystemCall(4, (int)syscall_wait);
    systemService.setSystemCall(5, (int)syscall_move_cursor);

    // 内存管理器
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
    firstThread->status = ProgramStatus::RUNNING;
    programManager.readyPrograms.pop_front();
    programManager.running = firstThread;
    asm_switch_thread(0, firstThread);

    asm_halt();
}
