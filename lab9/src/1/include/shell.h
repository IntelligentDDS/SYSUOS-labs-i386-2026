#ifndef SHELL_H
#define SHELL_H

#include "fat16.h"

class Shell
{
public:
    Shell();
    void initialize(DiskDriver *disk, FAT16 *fs);
    void run();
private:
    void printLogo();
    void runDemo();
    DiskDriver *disk;
    FAT16 *fs;
};

#endif