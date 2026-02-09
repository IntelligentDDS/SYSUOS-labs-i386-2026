#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;

// ==================== 芝士汉堡问题（信号量互斥） ====================

Semaphore semaphore;
int cheese_burger;

void a_mother(void *arg)
{
    semaphore.P();
    int delay = 0;

    printf("mother: start to make cheese burger, there are %d cheese burger now\n", cheese_burger);
    // make 10 cheese_burger
    cheese_burger += 10;

    printf("mother: oh, I have to hang clothes out.\n");
    // hanging clothes out
    delay = 0xfffffff;
    while (delay)
        --delay;
    // done

    printf("mother: Oh, Jesus! There are %d cheese burgers\n", cheese_burger);
    semaphore.V();
}

void a_naughty_boy(void *arg)
{
    semaphore.P();
    printf("boy   : Look what I found!\n");
    // eat all cheese_burgers out secretly
    cheese_burger -= 10;
    // run away as fast as possible
    semaphore.V();
}

// ==================== 生产者-消费者问题（有界缓冲区） ====================

const int BUFFER_SIZE = 5;         // 缓冲区大小
int buffer[BUFFER_SIZE];           // 有界缓冲区
int in_idx = 0;                    // 生产者写入位置
int out_idx = 0;                   // 消费者读取位置

Semaphore sem_empty;               // 空槽位数量（初始=BUFFER_SIZE）
Semaphore sem_full;                // 已填充槽位数量（初始=0）
Semaphore sem_mutex;               // 互斥信号量（初始=1）

int produced_count = 0;            // 已生产总数
int consumed_count = 0;            // 已消费总数

// 简单延时函数
void delay(int count)
{
    volatile int d = count;
    while (d) --d;
}

// 生产者线程
void producer(void *arg)
{
    int id = (int)arg;
    for (int i = 0; i < 3; ++i)
    {
        int item = id * 10 + i;  // 产品编号

        sem_empty.P();           // 等待空槽位
        sem_mutex.P();           // 进入临界区

        buffer[in_idx] = item;
        printf("[Producer %d] put item %d into buffer[%d]\n", id, item, in_idx);
        in_idx = (in_idx + 1) % BUFFER_SIZE;
        ++produced_count;

        sem_mutex.V();           // 离开临界区
        sem_full.V();            // 增加已填充槽位

        delay(0x3fffff);         // 模拟生产耗时
    }
    printf("[Producer %d] done.\n", id);
}

// 消费者线程
void consumer(void *arg)
{
    int id = (int)arg;
    for (int i = 0; i < 3; ++i)
    {
        sem_full.P();            // 等待已填充槽位
        sem_mutex.P();           // 进入临界区

        int item = buffer[out_idx];
        printf("[Consumer %d] got item %d from buffer[%d]\n", id, item, out_idx);
        out_idx = (out_idx + 1) % BUFFER_SIZE;
        ++consumed_count;

        sem_mutex.V();           // 离开临界区
        sem_empty.V();           // 增加空槽位

        delay(0x5fffff);         // 模拟消费耗时
    }
    printf("[Consumer %d] done.\n", id);
}

// ==================== 主线程 ====================

void first_thread(void *arg)
{
    // 第1个线程不可以返回
    stdio.moveCursor(0);
    for (int i = 0; i < 25 * 80; ++i)
    {
        stdio.print(' ');
    }
    stdio.moveCursor(0);

    // ---- Part 1: 芝士汉堡问题 ----
    printf("===== Part 1: Cheese Burger (Semaphore Mutex) =====\n");
    cheese_burger = 0;
    semaphore.initialize(1);

    programManager.executeThread(a_mother, nullptr, "mother", 1);
    programManager.executeThread(a_naughty_boy, nullptr, "boy", 1);

    // 等待芝士汉堡问题的两个线程执行完毕
    delay(0x3fffffff);

    // ---- Part 2: 生产者-消费者问题 ----
    printf("\n===== Part 2: Producer-Consumer (Bounded Buffer) =====\n");
    printf("Buffer size = %d, 2 producers x 3 items, 2 consumers x 3 items\n\n", BUFFER_SIZE);

    // 初始化信号量
    sem_empty.initialize(BUFFER_SIZE);  // 5 个空槽位
    sem_full.initialize(0);             // 0 个已填充槽位
    sem_mutex.initialize(1);            // 互斥

    // 初始化缓冲区
    in_idx = 0;
    out_idx = 0;
    produced_count = 0;
    consumed_count = 0;
    for (int i = 0; i < BUFFER_SIZE; ++i)
        buffer[i] = -1;

    // 创建2个生产者和2个消费者
    programManager.executeThread(producer, (void *)1, "producer1", 1);
    programManager.executeThread(producer, (void *)2, "producer2", 1);
    programManager.executeThread(consumer, (void *)1, "consumer1", 1);
    programManager.executeThread(consumer, (void *)2, "consumer2", 1);

    asm_halt();
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
