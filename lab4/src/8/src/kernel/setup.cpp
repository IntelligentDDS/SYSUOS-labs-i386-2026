#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "memory.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 内存管理器
MemoryManager memoryManager;


extern "C" void setup_kernel()
{

    // 中断管理器
    interruptManager.initialize();

    // 输出管理器
    stdio.initialize();

    // 内存管理器
    memoryManager.openPageMechanism();

    // 除零错误
    // int t = 1 / 0;

    // 段错误触发
    *(int*)0x100000 = 1;

    asm_halt();
}
