/* hello_module.c - 一个简单的Hello World内核模块 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
MODULE_DESCRIPTION("A simple Hello World kernel module for Kylin OS");
MODULE_VERSION("1.0");

/* 模块加载时执行的函数 */
static int __init hello_init(void)
{
    printk(KERN_INFO "Hello! This module is running on Kylin OS.\n");
    return 0;
}

/* 模块卸载时执行的函数 */
static void __exit hello_exit(void)
{
    printk(KERN_INFO "Goodbye! Module unloaded from Kylin OS.\n");
}

module_init(hello_init);
module_exit(hello_exit);
