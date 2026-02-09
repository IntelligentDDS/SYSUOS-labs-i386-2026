#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;

// ==================== 线程输出辅助函数 ====================
// 线程输出区从第9行开始（前9行是状态监控面板）
// 每个线程分配固定的行，避免输出交错

// 在指定行输出信息（直接写VGA显存）
void thread_print_at(int row, const char *str, uint8 color)
{
    uint8 *screen = (uint8 *)0xb8000;
    int pos = row * 80;
    // 先清空该行
    for (int i = 0; i < 80; ++i)
    {
        screen[2 * (pos + i)] = ' ';
        screen[2 * (pos + i) + 1] = color;
    }
    // 写入字符串
    for (int i = 0; str[i] && i < 80; ++i)
    {
        screen[2 * (pos + i)] = str[i];
        screen[2 * (pos + i) + 1] = color;
    }
}

// 整数转字符串（简易版）
void int_to_str(int val, char *buf)
{
    int i = 0;
    bool neg = false;
    if (val < 0) { neg = true; val = -val; }
    if (val == 0) { buf[i++] = '0'; }
    else {
        char tmp[12];
        int j = 0;
        while (val > 0) { tmp[j++] = '0' + (val % 10); val /= 10; }
        if (neg) tmp[j++] = '-';
        for (int k = j - 1; k >= 0; --k) buf[i++] = tmp[k];
    }
    buf[i] = '\0';
}

// ==================== 示例线程函数 ====================

// 线程A：计数器，优先级高 (priority=3)
void thread_counter_A(void *arg)
{
    int count = 0;
    char buf[80];
    while (count < 50)
    {
        // 构造输出字符串
        char num[12];
        int_to_str(count, num);

        // 手动拼接 "Thread A [pri=3]: count = xxx"
        int idx = 0;
        const char *prefix = "  Thread A [pri=3]: count = ";
        for (int i = 0; prefix[i]; ++i) buf[idx++] = prefix[i];
        for (int i = 0; num[i]; ++i) buf[idx++] = num[i];
        buf[idx] = '\0';

        thread_print_at(10, buf, 0x0b); // 亮青色
        count++;

        // 忙等待，模拟计算
        for (volatile int i = 0; i < 100000; ++i);

        // 每10次主动让出CPU
        if (count % 10 == 0)
            programManager.thread_yield();
    }
}

// 线程B：计数器，优先级中 (priority=2)
void thread_counter_B(void *arg)
{
    int count = 0;
    char buf[80];
    while (count < 50)
    {
        char num[12];
        int_to_str(count, num);

        int idx = 0;
        const char *prefix = "  Thread B [pri=2]: count = ";
        for (int i = 0; prefix[i]; ++i) buf[idx++] = prefix[i];
        for (int i = 0; num[i]; ++i) buf[idx++] = num[i];
        buf[idx] = '\0';

        thread_print_at(12, buf, 0x0e); // 黄色
        count++;

        for (volatile int i = 0; i < 100000; ++i);

        if (count % 10 == 0)
            programManager.thread_yield();
    }
}

// 线程C：计数器，优先级低 (priority=1)
void thread_counter_C(void *arg)
{
    int count = 0;
    char buf[80];
    while (count < 50)
    {
        char num[12];
        int_to_str(count, num);

        int idx = 0;
        const char *prefix = "  Thread C [pri=1]: count = ";
        for (int i = 0; prefix[i]; ++i) buf[idx++] = prefix[i];
        for (int i = 0; num[i]; ++i) buf[idx++] = num[i];
        buf[idx] = '\0';

        thread_print_at(14, buf, 0x0d); // 亮紫色
        count++;

        for (volatile int i = 0; i < 100000; ++i);

        if (count % 10 == 0)
            programManager.thread_yield();
    }
}

// ==================== 第一个线程（主线程，不可退出）====================

void first_thread(void *arg)
{
    // 在输出区打印调度信息
    thread_print_at(9, "  === Thread Output Area ===", 0x0f);

    // 创建3个工作线程，优先级不同
    programManager.executeThread(thread_counter_A, nullptr, "counter_A", 3);
    programManager.executeThread(thread_counter_B, nullptr, "counter_B", 2);
    programManager.executeThread(thread_counter_C, nullptr, "counter_C", 1);

    // 主线程：周期性刷新状态面板
    int tick = 0;
    char buf[80];
    while (1)
    {
        // 刷新线程状态监控面板
        programManager.printThreadStatus();

        // 在第16行显示主线程的心跳
        char num[12];
        int_to_str(tick, num);
        int idx = 0;
        const char *prefix = "  Main thread heartbeat: ";
        for (int i = 0; prefix[i]; ++i) buf[idx++] = prefix[i];
        for (int i = 0; num[i]; ++i) buf[idx++] = num[i];
        buf[idx] = '\0';
        thread_print_at(16, buf, 0x07); // 灰色

        tick++;

        // 让出CPU给其他线程
        programManager.thread_yield();
    }
}

// ==================== 内核入口 ====================

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

    // ★ 设置调度算法 ★
    // 可选: SCHEDULE_RR, SCHEDULE_PRIORITY, SCHEDULE_FCFS
    programManager.setScheduleAlgorithm(SCHEDULE_PRIORITY);

    // 创建第一个线程（主线程）
    int pid = programManager.executeThread(first_thread, nullptr, "main", 1);
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
