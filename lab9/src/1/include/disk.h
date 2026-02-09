#ifndef DISK_H
#define DISK_H

#include "os_type.h"

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
    // 选择扇区 (LBA模式)
    void selectSector(uint32 lba, uint8 count);
};

#endif
