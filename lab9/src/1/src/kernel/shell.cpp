#include "shell.h"
#include "asm_utils.h"
#include "syscall.h"
#include "stdio.h"
#include "stdlib.h"

Shell::Shell()
{
    disk = nullptr;
    fs = nullptr;
}

void Shell::initialize(DiskDriver *drv, FAT16 *filesystem)
{
    this->disk = drv;
    this->fs = filesystem;
}

void Shell::printLogo()
{
    move_cursor(0, 19);
    printf(" ____  _   _ __  __ __  __ _____ ____\n");
    move_cursor(1, 19);
    printf("/ ___|| | | |  \\/  |  \\/  | ____|  _ \\\n");
    move_cursor(2, 19);
    printf("\\___ \\| | | | |\\/| | |\\/| |  _| | |_) |\n");
    move_cursor(3, 19);
    printf(" ___) | |_| | |  | | |  | | |___|  _ <\n");
    move_cursor(4, 19);
    printf("|____/ \\___/|_|  |_|_|  |_|_____|_| \\_\\\n");
}

void Shell::runDemo()
{
    char readBuf[1024];

    printf("\n==============================================\n");
    printf("      FAT16 File System Demo\n");
    printf("==============================================\n\n");

    // ---- 步骤1: 格式化文件系统 ----
    printf("[Step 1] Formatting file system...\n");
    fs->format(disk);
    printf("\n");

    // ---- 步骤2: 查看文件系统信息 ----
    printf("[Step 2] File system info:\n");
    fs->showInfo();
    printf("\n");

    // ---- 步骤3: 创建文件 ----
    printf("[Step 3] Creating files...\n");
    fs->createFile("hello.txt");
    printf("  Created: hello.txt\n");
    fs->createFile("data.bin");
    printf("  Created: data.bin\n");
    fs->createFile("readme");
    printf("  Created: readme\n");
    printf("\n");

    // ---- 步骤4: 列出目录 ----
    printf("[Step 4] Listing files:\n");
    fs->listFiles();
    printf("\n");

    // ---- 步骤5: 写入文件 ----
    printf("[Step 5] Writing to files...\n");
    const char *msg1 = "Hello, FAT16 World!";
    fs->writeFile("hello.txt", msg1, strlen(msg1));
    printf("  Wrote %d bytes to hello.txt\n", strlen(msg1));

    const char *msg2 = "This is a README file for our simple OS.";
    fs->writeFile("readme", msg2, strlen(msg2));
    printf("  Wrote %d bytes to readme\n", strlen(msg2));

    const char *msg3 = "Binary data: ABCDEFGH12345678";
    fs->writeFile("data.bin", msg3, strlen(msg3));
    printf("  Wrote %d bytes to data.bin\n", strlen(msg3));
    printf("\n");

    // ---- 步骤6: 列出目录 (更新后) ----
    printf("[Step 6] Listing files after write:\n");
    fs->listFiles();
    printf("\n");

    // ---- 步骤7: 读取文件 ----
    printf("[Step 7] Reading files...\n");

    memset(readBuf, 0, sizeof(readBuf));
    int n = fs->readFile("hello.txt", readBuf, sizeof(readBuf) - 1);
    if (n > 0)
    {
        readBuf[n] = '\0';
        printf("  hello.txt (%d bytes): %s\n", n, readBuf);
    }

    memset(readBuf, 0, sizeof(readBuf));
    n = fs->readFile("readme", readBuf, sizeof(readBuf) - 1);
    if (n > 0)
    {
        readBuf[n] = '\0';
        printf("  readme (%d bytes): %s\n", n, readBuf);
    }
    printf("\n");

    // ---- 步骤8: 删除文件 ----
    printf("[Step 8] Deleting data.bin...\n");
    fs->deleteFile("data.bin");
    printf("  Deleted: data.bin\n");
    printf("\n");

    // ---- 步骤9: 列出目录 (删除后) ----
    printf("[Step 9] Listing files after delete:\n");
    fs->listFiles();
    printf("\n");

    // ---- 步骤10: 尝试读取已删除文件 ----
    printf("[Step 10] Try reading deleted file...\n");
    n = fs->readFile("data.bin", readBuf, sizeof(readBuf) - 1);
    printf("\n");

    // ---- 步骤11: 覆盖写已有文件 ----
    printf("[Step 11] Overwriting hello.txt...\n");
    const char *msg4 = "Content has been updated!";
    fs->writeFile("hello.txt", msg4, strlen(msg4));
    memset(readBuf, 0, sizeof(readBuf));
    n = fs->readFile("hello.txt", readBuf, sizeof(readBuf) - 1);
    if (n > 0)
    {
        readBuf[n] = '\0';
        printf("  hello.txt now: %s\n", readBuf);
    }
    printf("\n");

    // ---- 步骤12: 最终文件系统信息 ----
    printf("[Step 12] Final file system info:\n");
    fs->showInfo();
    printf("\n");

    printf("==============================================\n");
    printf("      Demo Complete!\n");
    printf("==============================================\n");
}

void Shell::run()
{
    // 清屏
    move_cursor(0, 0);
    for (int i = 0; i < 25; ++i)
    {
        for (int j = 0; j < 80; ++j)
        {
            printf(" ");
        }
    }
    move_cursor(0, 0);

    printLogo();

    move_cursor(6, 20);
    printf("SUMMER OS - FAT16 File System Lab\n\n");

    // 运行文件系统演示
    runDemo();

    asm_halt();
}
