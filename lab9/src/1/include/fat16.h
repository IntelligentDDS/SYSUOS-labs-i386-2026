#ifndef FAT16_H
#define FAT16_H

#include "os_type.h"
#include "disk.h"

// ============================================================
// 简化 FAT16 文件系统
// ============================================================
//
// 磁盘布局 (从 FS_START_SECTOR 开始):
//   扇区 0       : 超级块 (Superblock / BPB)
//   扇区 1~32    : FAT 表 (每个条目2字节，共 32*256 = 8192 条目)
//   扇区 33~64   : 根目录区 (每个条目32字节，共 32*16 = 512 条目)
//   扇区 65+     : 数据区 (簇大小 = 1扇区 = 512字节)
//
// FAT 表条目值:
//   0x0000 : 空闲簇
//   0xFFF8~0xFFFF : 文件结束 (EOF)
//   其他   : 下一个簇号
//
// ============================================================

// 文件系统起始扇区（避开 MBR + bootloader + kernel 区域）
#define FS_START_SECTOR   500

// FAT16 布局常量
#define FAT_SECTORS       32      // FAT 表占用的扇区数
#define ROOT_DIR_SECTORS  32      // 根目录区占用的扇区数
#define MAX_ROOT_ENTRIES  512     // 最大根目录条目数 (32*16)
#define ENTRIES_PER_SECTOR 16     // 每扇区目录条目数 (512/32)
#define FAT_ENTRIES       8192    // FAT 表条目总数 (32*256)
#define DATA_START_OFFSET 65      // 数据区相对 FS_START_SECTOR 的偏移
#define SECTOR_SIZE       512     // 每扇区字节数

#define FAT16_FREE        0x0000
#define FAT16_EOF         0xFFF8
#define FAT16_BAD         0xFFF7

#define MAX_FILENAME      11      // 8.3 格式文件名长度

// 文件属性
#define ATTR_NONE         0x00
#define ATTR_READONLY     0x01
#define ATTR_HIDDEN       0x02
#define ATTR_SYSTEM       0x04
#define ATTR_DIRECTORY    0x10
#define ATTR_ARCHIVE      0x20
#define ATTR_DELETED      0xE5    // 已删除标记（首字节）

// 超级块 (简化版 BPB)
struct SuperBlock
{
    uint8  magic[4];          // "SF16" 魔数
    uint16 bytesPerSector;    // 每扇区字节数 (512)
    uint8  sectorsPerCluster; // 每簇扇区数 (1)
    uint16 fatSectors;        // FAT 表扇区数
    uint16 rootDirSectors;    // 根目录区扇区数
    uint16 maxRootEntries;    // 最大根目录条目数
    uint32 totalSectors;      // 数据区总扇区数
    uint8  reserved[495];     // 填充到 512 字节
} __attribute__((packed));

// 目录条目 (32 字节)
struct DirEntry
{
    char   filename[8];       // 文件名 (空格填充)
    char   extension[3];      // 扩展名 (空格填充)
    uint8  attributes;        // 文件属性
    uint8  reserved[14];      // 保留字段 (含创建/访问时间等, 共14字节)
    uint16 firstCluster;      // 起始簇号
    uint32 fileSize;          // 文件大小 (字节)
} __attribute__((packed));

class FAT16
{
public:
    FAT16();

    // 初始化文件系统 (挂载)
    bool initialize(DiskDriver *disk);

    // 格式化磁盘，创建空文件系统
    bool format(DiskDriver *disk);

    // 创建文件 (空文件)
    bool createFile(const char *name);

    // 写入文件内容 (覆盖写)
    bool writeFile(const char *name, const char *data, uint32 size);

    // 读取文件内容到 buffer，返回实际读取字节数
    int readFile(const char *name, char *buffer, uint32 maxSize);

    // 删除文件
    bool deleteFile(const char *name);

    // 列出根目录所有文件
    void listFiles();

    // 查看文件系统信息
    void showInfo();

private:
    DiskDriver *disk;
    bool mounted;

    // 缓存: FAT 表 (内存中维护一份)
    uint16 fatTable[FAT_ENTRIES];

    // 将文件名转换为 8.3 格式
    void toFAT16Name(const char *name, char *fat16Name);

    // 从 8.3 格式还原为可读文件名
    void fromFAT16Name(const char *fat16Name, char *name);

    // 查找目录条目，返回条目在根目录区的索引，-1 表示未找到
    int findEntry(const char *name);

    // 找到一个空闲目录条目，返回索引
    int findFreeEntry();

    // 在 FAT 表中分配一个空闲簇
    uint16 allocateCluster();

    // 释放簇链
    void freeClusterChain(uint16 startCluster);

    // 将 FAT 表写回磁盘
    void syncFAT();

    // 从磁盘读取 FAT 表
    void loadFAT();

    // 读取一个目录条目
    void readDirEntry(int index, DirEntry *entry);

    // 写入一个目录条目
    void writeDirEntry(int index, const DirEntry *entry);

    // 簇号转扇区号
    uint32 clusterToSector(uint16 cluster);
};

#endif
