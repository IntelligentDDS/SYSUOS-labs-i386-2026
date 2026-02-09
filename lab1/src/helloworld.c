/* helloworld.c - Lab1 initramfs Hello World程序
 * 
 * 编译方法（本地x86_64环境）：
 *   gcc -o helloworld -m32 -static helloworld.c
 */
#include <stdio.h>

void main()
{
    printf("lab1: Hello World\n");
    fflush(stdout);
    /* 让程序打印完后继续维持在用户态 */
    while(1);
}
