#include "program.h"
#include "stdlib.h"
#include "interrupt.h"
#include "asm_utils.h"
#include "stdio.h"
#include "thread.h"
#include "os_modules.h"

const int PCB_SIZE = 4096;                   // PCB的大小，4KB。
char PCB_SET[PCB_SIZE * MAX_PROGRAM_AMOUNT]; // 存放PCB的数组，预留了MAX_PROGRAM_AMOUNT个PCB的大小空间。
bool PCB_SET_STATUS[MAX_PROGRAM_AMOUNT];     // PCB的分配状态，true表示已经分配，false表示未分配。

// ==================== 辅助函数 ====================

// 返回线程状态的字符串表示
const char *statusToString(ProgramStatus status)
{
    switch (status)
    {
    case CREATED:  return "CREATED";
    case RUNNING:  return "RUNNING";
    case READY:    return "READY  ";
    case BLOCKED:  return "BLOCKED";
    case DEAD:     return "DEAD   ";
    default:       return "UNKNOWN";
    }
}

// 返回调度算法的字符串表示
const char *algorithmToString(ScheduleAlgorithm algo)
{
    switch (algo)
    {
    case SCHEDULE_RR:       return "Round Robin";
    case SCHEDULE_PRIORITY: return "Priority";
    case SCHEDULE_FCFS:     return "FCFS";
    default:                return "Unknown";
    }
}

// ==================== ProgramManager 实现 ====================

ProgramManager::ProgramManager()
{
    initialize();
}

void ProgramManager::initialize()
{
    allPrograms.initialize();
    readyPrograms.initialize();
    running = nullptr;
    algorithm = SCHEDULE_RR; // 默认使用时间片轮转
    threadCounter = 0;

    for (int i = 0; i < MAX_PROGRAM_AMOUNT; ++i)
    {
        PCB_SET_STATUS[i] = false;
    }
}

void ProgramManager::setScheduleAlgorithm(ScheduleAlgorithm algo)
{
    algorithm = algo;
}

int ProgramManager::executeThread(ThreadFunction function, void *parameter, const char *name, int priority)
{
    // 关中断，防止创建线程的过程被打断
    bool status = interruptManager.getInterruptStatus();
    interruptManager.disableInterrupt();

    // 分配一页作为PCB
    PCB *thread = allocatePCB();

    if (!thread)
        return -1;

    // 初始化分配的页
    memset(thread, 0, PCB_SIZE);

    for (int i = 0; i < MAX_PROGRAM_NAME && name[i]; ++i)
    {
        thread->name[i] = name[i];
    }

    thread->status = ProgramStatus::READY;
    thread->priority = priority;
    thread->ticks = priority * 10;
    thread->ticksPassedBy = 0;
    thread->pid = ((int)thread - (int)PCB_SET) / PCB_SIZE;
    thread->arrivalOrder = threadCounter++; // 记录到达顺序

    // 线程栈
    thread->stack = (int *)((int)thread + PCB_SIZE);
    thread->stack -= 7;
    thread->stack[0] = 0;
    thread->stack[1] = 0;
    thread->stack[2] = 0;
    thread->stack[3] = 0;
    thread->stack[4] = (int)function;
    thread->stack[5] = (int)program_exit;
    thread->stack[6] = (int)parameter;

    allPrograms.push_back(&(thread->tagInAllList));
    readyPrograms.push_back(&(thread->tagInGeneralList));

    // 恢复中断
    interruptManager.setInterruptStatus(status);

    return thread->pid;
}

// ==================== 调度策略实现 ====================

void ProgramManager::schedule()
{
    bool status = interruptManager.getInterruptStatus();
    interruptManager.disableInterrupt();

    if (readyPrograms.size() == 0)
    {
        // 没有就绪线程，如果当前线程已死则释放
        if (running->status == ProgramStatus::DEAD)
        {
            releasePCB(running);
        }
        interruptManager.setInterruptStatus(status);
        return;
    }

    // 处理当前运行线程
    if (running->status == ProgramStatus::RUNNING)
    {
        running->status = ProgramStatus::READY;
        readyPrograms.push_back(&(running->tagInGeneralList));
    }
    else if (running->status == ProgramStatus::DEAD)
    {
        releasePCB(running);
    }

    // 根据调度算法选择下一个线程
    switch (algorithm)
    {
    case SCHEDULE_RR:
        scheduleRR();
        break;
    case SCHEDULE_PRIORITY:
        schedulePriority();
        break;
    case SCHEDULE_FCFS:
        scheduleFCFS();
        break;
    default:
        scheduleRR();
        break;
    }

    interruptManager.setInterruptStatus(status);
}

// 内部切换函数：切换到 next 线程
void ProgramManager::switchTo(PCB *next)
{
    PCB *cur = running;
    next->status = ProgramStatus::RUNNING;
    running = next;
    readyPrograms.erase(&(next->tagInGeneralList));
    asm_switch_thread(cur, next);
}

// 时间片轮转调度：FIFO从就绪队列取第一个
void ProgramManager::scheduleRR()
{
    ListItem *item = readyPrograms.front();
    PCB *next = ListItem2PCB(item, tagInGeneralList);

    // 重置时间片
    next->ticks = next->priority * 10;

    switchTo(next);
}

// 优先级调度：选择优先级最高（数值最大）的线程
void ProgramManager::schedulePriority()
{
    ListItem *bestItem = readyPrograms.front();
    PCB *bestPCB = ListItem2PCB(bestItem, tagInGeneralList);

    // 遍历就绪队列，找优先级最高的
    int count = readyPrograms.size();
    ListItem *current = readyPrograms.head.next;
    for (int i = 0; i < count; ++i)
    {
        PCB *pcb = ListItem2PCB(current, tagInGeneralList);
        if (pcb->priority > bestPCB->priority)
        {
            bestItem = current;
            bestPCB = pcb;
        }
        current = current->next;
    }

    // 重置时间片
    bestPCB->ticks = bestPCB->priority * 10;

    switchTo(bestPCB);
}

// 先来先服务调度：选择到达顺序最小（最先创建）的线程
void ProgramManager::scheduleFCFS()
{
    ListItem *bestItem = readyPrograms.front();
    PCB *bestPCB = ListItem2PCB(bestItem, tagInGeneralList);

    // 遍历就绪队列，找到达顺序最小的
    int count = readyPrograms.size();
    ListItem *current = readyPrograms.head.next;
    for (int i = 0; i < count; ++i)
    {
        PCB *pcb = ListItem2PCB(current, tagInGeneralList);
        if (pcb->arrivalOrder < bestPCB->arrivalOrder)
        {
            bestItem = current;
            bestPCB = pcb;
        }
        current = current->next;
    }

    // FCFS: 给予足够大的时间片（非抢占）
    bestPCB->ticks = 1000;

    switchTo(bestPCB);
}

// ==================== 线程主动让出 CPU ====================

void ProgramManager::thread_yield()
{
    bool status = interruptManager.getInterruptStatus();
    interruptManager.disableInterrupt();

    if (readyPrograms.size() == 0)
    {
        interruptManager.setInterruptStatus(status);
        return;
    }

    // 当前线程放回就绪队列
    running->status = ProgramStatus::READY;
    running->ticks = running->priority * 10;
    readyPrograms.push_back(&(running->tagInGeneralList));

    // 根据调度算法选择下一个
    switch (algorithm)
    {
    case SCHEDULE_RR:
        scheduleRR();
        break;
    case SCHEDULE_PRIORITY:
        schedulePriority();
        break;
    case SCHEDULE_FCFS:
        scheduleFCFS();
        break;
    default:
        scheduleRR();
        break;
    }

    interruptManager.setInterruptStatus(status);
}

// ==================== 线程状态显示 ====================

// 在屏幕顶部 (行 0~7) 显示所有线程的状态表
// 使用直接写VGA显存的方式，避免干扰printf的光标
void ProgramManager::printThreadStatus()
{
    // VGA 显存基地址
    uint8 *screen = (uint8 *)0xb8000;

    // 颜色定义
    const uint8 TITLE_COLOR = 0x1f;  // 蓝底白字 (标题)
    const uint8 HEADER_COLOR = 0x1e; // 蓝底黄字 (表头)
    const uint8 DATA_COLOR = 0x0f;   // 黑底亮白字 (数据)
    const uint8 RUN_COLOR = 0x0a;    // 黑底亮绿字 (RUNNING)
    const uint8 READY_COLOR = 0x0e;  // 黑底黄字 (READY)
    const uint8 DEAD_COLOR = 0x0c;   // 黑底亮红字 (DEAD)
    const uint8 SEP_COLOR = 0x08;    // 暗灰色 (分隔线)

    // 辅助lambda：在指定位置写字符串
    // row: 0~24, col: 0~79
    auto writeStr = [&](int row, int col, const char *str, uint8 color) {
        int pos = row * 80 + col;
        for (int i = 0; str[i] && (col + i) < 80; ++i)
        {
            screen[2 * (pos + i)] = str[i];
            screen[2 * (pos + i) + 1] = color;
        }
    };

    // 辅助：在指定位置写整数
    auto writeInt = [&](int row, int col, int val, uint8 color) {
        char buf[12];
        int i = 0;
        bool neg = false;
        if (val < 0) { neg = true; val = -val; }
        if (val == 0) { buf[i++] = '0'; }
        else {
            while (val > 0) { buf[i++] = '0' + (val % 10); val /= 10; }
        }
        if (neg) buf[i++] = '-';

        // 反转
        int pos = row * 80 + col;
        for (int j = i - 1; j >= 0; --j)
        {
            screen[2 * pos] = buf[j];
            screen[2 * pos + 1] = color;
            pos++;
        }
    };

    // 辅助：清空一行
    auto clearRow = [&](int row, uint8 color) {
        int pos = row * 80;
        for (int i = 0; i < 80; ++i)
        {
            screen[2 * (pos + i)] = ' ';
            screen[2 * (pos + i) + 1] = color;
        }
    };

    // 辅助：画分隔线
    auto drawSep = [&](int row) {
        int pos = row * 80;
        for (int i = 0; i < 80; ++i)
        {
            screen[2 * (pos + i)] = '-';
            screen[2 * (pos + i) + 1] = SEP_COLOR;
        }
    };

    // === 第0行：标题 ===
    clearRow(0, TITLE_COLOR);
    writeStr(0, 1, "[ Thread Status Monitor ]  Algorithm: ", TITLE_COLOR);
    writeStr(0, 39, algorithmToString(algorithm), TITLE_COLOR);

    // === 第1行：表头 ===
    clearRow(1, HEADER_COLOR);
    //       PID  Name             Status   Priority  Ticks  Elapsed  Arrival
    writeStr(1, 1, "PID", HEADER_COLOR);
    writeStr(1, 6, "Name", HEADER_COLOR);
    writeStr(1, 23, "Status", HEADER_COLOR);
    writeStr(1, 32, "Pri", HEADER_COLOR);
    writeStr(1, 37, "Ticks", HEADER_COLOR);
    writeStr(1, 44, "Elapsed", HEADER_COLOR);
    writeStr(1, 53, "Arrival", HEADER_COLOR);

    // === 第2行：分隔线 ===
    drawSep(2);

    // === 第3~7行：线程数据（最多显示5个线程） ===
    int row = 3;
    int maxRows = 7; // 行3~7，共5行数据

    // 遍历allPrograms链表
    int count = allPrograms.size();
    ListItem *item = allPrograms.head.next;
    for (int i = 0; i < count && row <= maxRows; ++i)
    {
        PCB *pcb = ListItem2PCB(item, tagInAllList);
        clearRow(row, 0x00);

        // PID
        writeInt(row, 1, pcb->pid, DATA_COLOR);

        // Name
        writeStr(row, 6, pcb->name, DATA_COLOR);

        // Status（带颜色区分）
        uint8 scolor = DATA_COLOR;
        if (pcb->status == RUNNING)      scolor = RUN_COLOR;
        else if (pcb->status == READY)   scolor = READY_COLOR;
        else if (pcb->status == DEAD)    scolor = DEAD_COLOR;
        writeStr(row, 23, statusToString(pcb->status), scolor);

        // Priority
        writeInt(row, 32, pcb->priority, DATA_COLOR);

        // Ticks
        writeInt(row, 37, pcb->ticks, DATA_COLOR);

        // Elapsed (ticksPassedBy)
        writeInt(row, 44, pcb->ticksPassedBy, DATA_COLOR);

        // Arrival order
        writeInt(row, 53, pcb->arrivalOrder, DATA_COLOR);

        item = item->next;
        row++;
    }

    // 清空剩余行
    for (; row <= maxRows; ++row)
    {
        clearRow(row, 0x00);
    }

    // === 第8行：分隔线（状态区与输出区的边界）===
    drawSep(8);
}

// ==================== 线程退出 ====================

void program_exit()
{
    PCB *thread = programManager.running;
    thread->status = ProgramStatus::DEAD;

    if (thread->pid)
    {
        programManager.schedule();
    }
    else
    {
        interruptManager.disableInterrupt();
        printf("halt\n");
        asm_halt();
    }
}

// ==================== PCB 分配与释放 ====================

PCB *ProgramManager::allocatePCB()
{
    for (int i = 0; i < MAX_PROGRAM_AMOUNT; ++i)
    {
        if (!PCB_SET_STATUS[i])
        {
            PCB_SET_STATUS[i] = true;
            return (PCB *)((int)PCB_SET + PCB_SIZE * i);
        }
    }

    return nullptr;
}

void ProgramManager::releasePCB(PCB *program)
{
    int index = ((int)program - (int)PCB_SET) / PCB_SIZE;
    PCB_SET_STATUS[index] = false;
}