# lab1 编译内核/利用已有内核构建OS  

<center>
    任课老师：陈鹏飞
</center>

# 实验要求

> + **DDL：2026-03-10 23:59**
> + 提交的内容：将**实验报告**转换成pdf格式，并命名为"lab1-学号-姓名.pdf"，然后提交到课程网站的"实验作业一"处。

1. **独立完成**实验8个部份：**环境配置**、**编译Linux内核**、**Qemu启动内核并开启远程调试**、**制作Initramfs**、**编译并启动Busybox**、**完成Linux 0.11内核的编译启动和调试**、**认识麒麟操作系统（Kylin OS）** 和 **探索麒麟OS内核**。
2. 编写实验报告，结合实验过程来谈谈你完成实验的思路和结果，最后需要提供实验各个部份的程序运行截屏来证明你完成了实验。
3. 实验不限语言， C/C++/Rust都可以。
4. 实验不限平台， Windows、Linux和MacOS等都可以。
5. 实验不限CPU， ARM/Intel/Risc-V都可以。
6. 本次实验支持**Ubuntu**和**麒麟操作系统（Kylin OS）**环境，支持**x86_64（AMD64）**和**ARM64（AArch64）**两种CPU架构。第一至六部分为核心实验（编译i386内核），ARM64架构用户请参考附录G的替代方案。第七、八部分为麒麟OS扩展实验，在麒麟操作系统中完成，支持所有架构。

# 实验概述

> 在本次实验中，同学们会熟悉现有Linux内核的编译过程和启动过程，并在自行编译内核的基础上构建简单应用并启动。同时，同学们会利用精简的Busybox工具集构建简单的OS，熟悉现代操作系统的构建过程。此外，同学们会熟悉编译环境、相关工具集，并能够实现内核远程调试。最后，同学们将在国产麒麟操作系统（Kylin OS）中进行实验，探索不同发行版和不同CPU架构下的操作系统异同，加深对操作系统内核的理解。

1. 搭建OS内核开发环境包括：代码编辑环境、编译环境、运行环境、调试环境等。
2. 下载并编译i386（32位）内核，并利用qemu启动内核。
3. 熟悉制作initramfs的方法。
4. 编写简单应用程序随内核启动运行。
5. 编译i386版本的Busybox，随内核启动，构建简单的OS。
6. 开启远程调试功能，进行调试跟踪代码运行。
7. 完成Linux 0.11内核的编译、启动和调试。
8. 在麒麟操作系统中进行实验，认识国产OS生态与不同CPU架构的差异。
9. 探索麒麟OS内核机制，包括/proc文件系统、内核模块、系统调用等。
10. 撰写实验报告。

---

# 第一部分：环境配置
> 下载 GCC 源码：`wget https://ftp.gnu.org/gnu/gcc/gcc-12.2.0/gcc-12.2.0.tar.gz -P lab1/`
> 本实验支持在**Ubuntu**和**麒麟操作系统（Kylin OS）**上完成，请根据你的操作系统选择对应的配置步骤。你可以通过`cat /etc/os-release`和`uname -m`确认当前操作系统和CPU架构。
>
> - **Ubuntu用户：** 若在非Linux环境下，首先下载安装Virtualbox，用于启动虚拟机。如果本身是Linux环境则不需要这个步骤。建议安装Ubuntu 22.04桌面版（校内可以使用[matrix下载源](https://mirrors.matrix.moe/ubuntu-releases/22.04/)进行下载），并将下载源换成清华下载源。
> - **麒麟OS用户：** 如果你的实验环境已预装麒麟操作系统，可直接在麒麟OS中进行实验。麒麟OS使用`yum`或`dnf`（两者用法基本一致）作为包管理器，而非Ubuntu的`apt`。

有部分同学使用arm架构的Mac做本实验，可能会遇到一些兼容性问题，可以参考此文章：[ARM ubuntu配置OS实验环境](https://www.cnblogs.com/foreeest/p/18228877)

按如下步骤配置环境。

## 换源

### Ubuntu用户

由于ubuntu的下载源默认是国外的，为了提高下载速度，我们需要将下载源更换为国内源。我们首先备份原先的下载源。

```shell
sudo mv /etc/apt/sources.list /etc/apt/sources.list.backup
```

> 对于ubuntu22.04，可能会遇到用户没有权限的问题，可以参考[该链接](https://blog.csdn.net/weixin_37787043/article/details/123045557)进行配置

然后找到清华的ubuntu下载源[https://mirrors.tuna.tsinghua.edu.cn/help/ubuntu]。注意，选择你ubuntu的版本对应的下载源。

![清华下载源](images/清华下载源.PNG)

然后使用`gedit`打开下载源保存的文件`/etc/apt/sources.list`

```shell
sudo gedit /etc/apt/sources.list
```

将下载源复制进`/etc/apt/sources.list`后保存退出。

更新`apt`，检查是否更换为清华的下载源。

```shell
sudo apt update
```

### 麒麟OS用户

麒麟OS的软件源由麒麟官方提供，通常已预配置好，不需要更换。软件源配置文件位于`/etc/yum.repos.d/`目录下：

```shell
ls /etc/yum.repos.d/
```

更新软件源缓存：

```shell
sudo yum makecache
```

## 配置C/C++环境

**Ubuntu：**

```shell
sudo apt install binutils
sudo apt install gcc
```

**麒麟OS：**

```shell
sudo yum install binutils
sudo yum install gcc
```

查看`gcc`是否安装。

```shell
gcc -v
```

若输出gcc的版本号则表明安装成功。

## 安装其他工具

**Ubuntu：**

```shell
sudo apt install nasm
sudo apt install qemu
sudo apt install cmake
sudo apt install libncurses5-dev
sudo apt install bison
sudo apt install flex
sudo apt install libssl-dev
sudo apt install libc6-dev-i386
```

**麒麟OS：**

```shell
sudo yum install nasm
sudo yum install qemu-system-x86
sudo yum install cmake
sudo yum install ncurses-devel
sudo yum install bison
sudo yum install flex
sudo yum install openssl-devel
sudo yum install bc
```

> 在x86_64架构的麒麟OS上，还需要安装i386（32位）开发库：
> ```shell
> sudo yum install glibc-devel.i686 libgcc.i686
> ```
> 在ARM64架构的麒麟OS上，请参阅第二部分开头的“多架构说明”。

> **说明：** Ubuntu上的`libc6-dev-i386`和麒麟OS上的`glibc-devel.i686`作用相同，都是编译32位（i386）程序所必需的库。不同Linux发行版的包名有所不同，这体现了Debian（deb）和Red Hat（rpm）两大包管理体系的差异。

安装vscode以及在vscode中安装汇编、 C/C++插件。vscode将作为一个有力的代码编辑器。

# 第二部分：编译Linux内核

> **多架构说明（请根据你的环境选择）：**
> - **x86_64（AMD64）环境**（Ubuntu或麒麟OS均适用）：直接按照以下步骤编译i386内核即可。麒麟OS用户仅需将`apt`命令替换为`yum`（参见第一部分）。
> - **ARM64（AArch64）环境**（Ubuntu ARM64或麒麟OS ARM64均适用）：由于i386内核需要x86编译工具链，ARM64系统上无法直接使用`gcc -m32`。建议选择以下方案之一：
>   1. 在x86_64虚拟机或另一台x86_64机器上完成第二至五部分实验。
>   2. 将第二至五部分的编译目标替换为ARM64架构内核（去掉`-m32`参数，QEMU使用`qemu-system-aarch64`），具体差异请参考附录G.2。
>   3. 第六部分（Linux 0.11）可通过构建i686交叉编译器在ARM64上直接编译，详见附录G.1。

## 下载内核

我们先在当前用户目录下创建文件夹`lab1`并进入。

```shell
mkdir ~/lab1
cd ~/lab1
```

到 <https://www.kernel.org/> 下载内核5.10到文件夹`~/lab1`。
[linux-5.10.19.tar.xz下载地址](https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.10.19.tar.xz)
<img src="images/下载linux内核.PNG" alt="下载linux内核" style="zoom:50%;" />

> **提示：** 如果下载较慢，可以使用清华镜像源加速：
> ```shell
> wget https://mirrors.tuna.tsinghua.edu.cn/kernel/v5.x/linux-5.10.19.tar.xz
> ```

解压并进入。

```shell
xz -d linux-5.10.19.tar.xz
tar -xvf linux-5.10.19.tar
cd linux-5.10.19
```

## 编译内核

将内核编译成i386 32位版本。

```shell
make i386_defconfig
make menuconfig
```

在打开的图像界面中依次选择`Kernel hacking`、`Compile-time checks and compiler options`，最后在`[ ] Compile the kernel with debug info`输入`Y`勾选，保存退出。

编译内核，这一步较慢。

```shell
make -j8
```

> **注意：** 如果你的机器内存较少（如2GB），可以减少并行任务数：`make -j4`或`make -j2`。

检查Linux压缩镜像`linux-5.10.19/arch/x86/boot/bzImage`和符号表`linux-5.10.19/vmlinux`是否已经生成。

```shell
ls -lh arch/x86/boot/bzImage
ls -lh vmlinux
```

> **知识点：**
> - `bzImage`是压缩的Linux内核镜像，"bz"代表"big zImage"，用于引导启动。
> - `vmlinux`是未压缩的内核ELF文件，包含调试符号表，供GDB调试使用。
> - `i386_defconfig`使用了x86 32位的默认配置，这意味着编译出的内核运行在32位保护模式下。

# 第三部分：启动内核并调试  

> qemu和gdb是常用的程序调试工具，如果你不知道qemu是什么，请参考[https://baike.baidu.com/item/QEMU/1311178?fr=aladdin]；同样地，如果你不知道gdb是什么，请参考[https://baike.baidu.com/item/gdb/10869514?fr=aladdin]。

下面的过程在文件夹`~/lab1`下进行。

```shell
cd ~/lab1
```

## 启动qemu

使用`qemu`启动内核并开启远程调试。

```shell
qemu-system-i386 -kernel linux-5.10.19/arch/x86/boot/bzImage -s -S -append "console=ttyS0" -nographic
```

> **参数说明：**
> - `-kernel`：指定内核镜像文件。
> - `-s`：开启GDB调试服务器，监听TCP端口1234。
> - `-S`：启动后暂停CPU，等待GDB连接。
> - `-append "console=ttyS0"`：内核启动参数，将控制台输出重定向到串口`ttyS0`。
> - `-nographic`：不使用图形界面，串口输出到终端。
>
> **提示：** 如果你的系统上没有`qemu-system-i386`命令（如某些麒麟OS版本），可以使用`qemu-system-x86_64`代替，它完全兼容32位内核。ARM64用户若编译了ARM64内核（附录G.2），则需要使用`qemu-system-aarch64`启动，参数有所不同，详见附录G.2。

此时，同学们会发现qemu（终端）并未输出任何信息。这是因为我们开启了gdb调试，而qemu在等待gdb输入的指令后才能继续执行。接下来我们启动gdb，通过gdb来告诉qemu应该怎么做。

## gdb调试

在另外一个Terminal下启动gdb，注意，不要关闭qemu所在的Terminal。

```shell
gdb
```

在gdb下，加载符号表

```shell
file linux-5.10.19/vmlinux
```

在gdb下，连接已经启动的qemu进行调试。

```shell
target remote:1234
```

在gdb下，为start_kernel函数设置断点。

```shell
break start_kernel
```

在gdb下，输入`c`运行。

```
c
```

在继续执行后，最终qemu的输出如下，在qemu虚拟机里运行的Linux系统能成功启动，并且最终以Kernel panic宣告结束。看到call trace打出来的是在initrd_load的时候出错，原因很简单，因为启动系统的时候只指定了bzImage，没有指定initrd文件，系统无法mount上initrd (init ram disk) 及其initramfs文件系统。

> **思考题：**
> 1. `start_kernel`函数位于哪个源文件中？请用GDB的`list`命令查看。
> 2. 使用`info registers`查看寄存器状态。在i386模式下，你能看到哪些寄存器（EAX、EBX、ESP等）？
> 3. 尝试在`start_kernel`中单步执行几步（`n`），观察内核初始化的前几个函数调用。

# 第四部分：制作Initramfs  

下面的过程在文件夹`~/lab1`下进行。

```shell
cd ~/lab1
```

## Hello World

在前面调试内核中，我们已经准备了一个Linux启动环境，但是缺少initramfs。我们可以做一个最简单的Hello World initramfs，来直观地理解initramfs，Hello World程序如下。

```c
#include <stdio.h>

void main()
{
    printf("lab1: Hello World\n");
    fflush(stdout);
    /* 让程序打印完后继续维持在用户态 */
    while(1);
}
```

上述文件保存在`~/lab1/helloworld.c`中，然后将上面代码编译成32位可执行文件。

```shell
gcc -o helloworld -m32 -static helloworld.c
```

> **说明：**
> - `-m32`：生成32位可执行文件，与i386内核匹配。
> - `-static`：静态链接，将所有库函数编译进可执行文件中。这是必须的，因为initramfs中没有动态链接库。
>
> 编译完成后，可以使用`file helloworld`命令验证，输出应包含`ELF 32-bit`和`statically linked`。
>
> **麒麟OS用户提示：** 在x86_64麒麟OS上编译32位程序需要安装：`sudo yum install glibc-devel.i686 libgcc.i686`（参见第一部分）。ARM64用户请去掉`-m32`参数，直接编译为64位程序（详见附录G.2）。

## 加载initramfs

用cpio打包initramfs。

```shell
echo helloworld | cpio -o --format=newc > hwinitramfs
```

启动内核，并加载initramfs。

```shell
qemu-system-i386 -kernel linux-5.10.19/arch/x86/boot/bzImage -initrd hwinitramfs -s -S -append "console=ttyS0 rdinit=helloworld" -nographic
```

> **提示：** 没有`qemu-system-i386`的用户可用`qemu-system-x86_64`代替（参见第三部分说明）。

重复上面的gdb的调试过程，可以看到gdb中输出了`lab1: Hello World\n`

> **知识点：** initramfs（Initial RAM Filesystem）是Linux内核启动时加载到内存中的临时根文件系统。内核完成自身初始化后，会解压initramfs并执行其中的init程序（由`rdinit=`参数指定）。这是从内核态过渡到用户态的关键步骤。

# 第五部分：编译并启动Busybox  

下面的过程在文件夹`~/lab1`下进行。

```shell
cd ~/lab1
```

## 下载并解压

从Busybox官网下载源码到`~/lab1`，然后解压。

```shell
wget https://busybox.net/downloads/busybox-1.36.1.tar.bz2
tar -xjf busybox-1.36.1.tar.bz2
```

> **说明：** 原来使用的busybox-1.33.0版本也可以使用。如果从课程网站下载了busybox-1.33.0，请将后续命令中的版本号替换为1.33.0。如果官网下载较慢，可以尝试搜索国内镜像源，或访问 https://busybox.net/downloads/ 选择其他可用版本。

## 编译busybox

```shell
cd busybox-1.36.1
make defconfig
make menuconfig
```

进入`Settings`，然后在`Build static binary (no shared libs)`处输入`Y`勾选，然后分别设置`() Additional CFLAGS`和`() Additional LDFLAGS`为`(-m32 -march=i386) Additional CFLAGS`和`(-m32) Additional LDFLAGS`。

![配置busybox](images/配置busybox.PNG)

> **说明：** 设置`-m32`和`-march=i386`是为了将Busybox编译成32位版本，与我们编译的i386内核匹配。麒麟OS x86_64用户操作方式完全相同；ARM64用户不需要设置`-m32`和`-march=i386`参数，详见附录G.2。

保存退出，然后编译。

```shell
make -j8
make install
```

## 制作Initramfs 

将安装在_install目录下的文件和目录取出放在`~/lab1/mybusybox`处。

```shell
cd ~/lab1
mkdir mybusybox
mkdir -pv mybusybox/{bin,sbin,etc,proc,sys,usr/{bin,sbin}}
cp -av busybox-1.36.1/_install/* mybusybox/
cd mybusybox
```

initramfs需要一个init程序，可以写一个简单的shell脚本作为init。用编辑器打开文件`init`，复制入如下内容，保存退出。

```shell
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
echo -e "\nBoot took $(cut -d' ' -f1 /proc/uptime) seconds\n"
exec /bin/sh
```

加上执行权限。

```shell
chmod u+x init
```

将mybusybox下面的内容打包归档成cpio文件，以供Linux内核做initramfs启动执行。

```shell
find . -print0 | cpio --null -ov --format=newc | gzip -9 > ~/lab1/initramfs-busybox-x86.cpio.gz
```

## 加载busybox

```shell
cd ~/lab1
qemu-system-i386 -kernel linux-5.10.19/arch/x86/boot/bzImage -initrd initramfs-busybox-x86.cpio.gz -nographic -append "console=ttyS0" -m size=2048
```

> **提示：** 没有`qemu-system-i386`的用户，可将命令中的`qemu-system-i386`替换为`qemu-system-x86_64`。

然后使用`ls`命令即可看到当前文件夹。

> **恭喜！** 你已经成功构建了一个基于自编译i386内核和Busybox的迷你操作系统！尝试在Busybox shell中执行以下命令：
>
> ```shell
> uname -a          # 查看内核信息
> cat /proc/cpuinfo # 查看CPU信息
> cat /proc/meminfo # 查看内存信息
> ps                # 查看进程列表
> ```

退出QEMU：按`Ctrl+A`然后按`X`。

# 第六部分：完成Linux 0.11内核的编译、启动和调试

> 该部分实验任务与前5个实验步骤类似，故该部分说明较简略，可回顾前面的实验步骤进行实验。
>
> **多环境说明：**
> - **x86_64环境**（Ubuntu或麒麟OS）：直接按照以下步骤操作。麒麟OS上若没有`qemu-system-i386`，可使用`qemu-system-x86_64`代替（完全兼容32位）。
> - **ARM64环境**（Ubuntu ARM64或麒麟OS ARM64）：Linux 0.11是32位x86操作系统，在ARM64上编译需要构建i686交叉编译工具链。完整步骤请参考附录G.1，已在麒麟V11 ARM64上验证通过。

## 下载Linux 0.11内核代码

注意从课程提供的地址（因为文件较大，故传到了微信群）下载，否则linux 0.11内核不能编译，因为早期的linux内核需要低版本的gcc编译器；

## 编译32位版本的linux 0.11内核

找到Makefile，查看里面的内容，通过make工具进行编译；另外，因为需要调试，所以需要在gcc编译命令中添加 -g 参数，产生内核的符号表；编译32位版本的内核，需要添加 -m32 参数；

## 使用QEMU加载、启动内核

```shell
qemu-system-i386 -m 16 -boot a -fda Image -hda hdc-0.11.img -s -S
```

> **提示：** 如果你的系统上没有`qemu-system-i386`（如某些麒麟OS版本），可使用：
> ```shell
> qemu-system-x86_64 -m 16 -boot a -fda Image -hda hdc-0.11.img -s -S
> ```
> ARM64用户请参考附录G.1中第六步。

此时会打开一个黑色窗口，里面并不会有任何东西，这跟我们前面编译Linux5.10.19是类似的，因为我们开启了gdb调试，而qemu在等待gdb输入的指令后才能继续执行。所以接下来我们需要使用gdb进行调试。

## 利用gdb进行调试远程调试

启动gdb； 加载linux 0.11的符号表（一般位于tools/system）； `target remote:1234`远程连接qemu调试； 设置源码目录：`directory linux0.11的源码路径`（请自行替换）; 设置汇编代码的形式：`set disassembly-flavor intel` 
在关键位置设置断点如在地址0x7c00、内核入口函数（main）等（可以参考linux 0.11 源码中的bootsect.S中的一些关键地址和寄存器）； 观察 0x7DFE和0x7DFF地址的内容。（该部分可以请学习一下gdb的基础操作） 
熟悉linux 0.11操作系统与主机操作系统之间的文件交换。

关闭qemu模拟器；

找到hdc-0.11.img硬盘镜像文件，这是linux 0.11操作系统启动后的根文件系统，相当于在 qemu 虚拟机里装载的硬盘；

先用fdisk命令查看磁盘的分区情况以及文件类型(minix):

```shell
fdisk hdc-0.11.img
```

创建本地挂载目录
```shell
mkdir hdc
```

显式磁盘空间
```shell
df -h
```

挂载linux 0.11硬盘镜像

```shell
sudo mount -t minix -o loop,offset=512 hdc-0.11.img hdc
```

> （注意是hdc的完整路径）

查看是否挂载成功

```shell
df -h
```

> （在Ubuntu或麒麟OS中执行，查看是否出现挂载的hdc路径）

查看挂载后的hdc目录结构

```shell
ll hdc
```

## 在hdc中创建文件

进入hdc的usr目录 
```shell
cd hdc/usr
sudo touch hello.txt
sudo vim hello.txt
```

> （编辑文件，写入一些内容）

卸载文件系统hdc 
```shell
sudo umount /dev/loop设备号
df -h
```

> （查看具体的loop设备；查看是否已经卸载）

重新用qemu启动linux 0.11

观察/usr目录下是否有hello.txt文件

---

# 第七部分：认识麒麟操作系统（扩展实验）

> 麒麟操作系统（Kylin OS）是我国自主研发的国产操作系统，广泛应用于政府、军工、金融等关键领域。本部分实验在麒麟操作系统中完成，同学们将直接在麒麟OS环境下操作，认识不同Linux发行版和不同CPU架构之间的差异。
>
> 麒麟操作系统支持多种CPU架构，包括x86_64（AMD64）和ARM64（AArch64）等。本实验内容适用于所有架构。
>
> **提示：** 如果你的麒麟OS环境需要通过SSH远程登录，请使用`ssh`命令连接后在远程终端中执行以下操作。如果已直接运行在麒麟OS中，可在本地终端中直接操作。

## 7.1 了解系统基本信息

**查看操作系统版本：**

```shell
cat /etc/os-release
```

你会看到类似如下输出，表明这是麒麟Linux高级服务器版V11：

```
NAME="Kylin Linux Advanced Server"
VERSION="V11 (Swan25)"
ID="kylin"
VERSION_ID="V11"
PRETTY_NAME="Kylin Linux Advanced Server V11 (Swan25)"
```

**查看内核版本和CPU架构：**

```shell
uname -a
```

输出中会显示当前CPU架构：`x86_64`表示AMD64架构，`aarch64`表示ARM64架构。

**查看CPU信息：**

```shell
cat /proc/cpuinfo
```

> 不同架构下`/proc/cpuinfo`的输出格式有所不同。x86_64架构会显示`model name`、`cpu MHz`等字段；ARM64架构则会显示`CPU implementer`、`CPU architecture`等字段。

**查看内存和磁盘信息：**

```shell
free -h
df -h
```

> **思考题：** 请在实验报告中记录以下信息，并与第一至六部分的实验环境进行对比：
> - 操作系统名称和版本（Ubuntu vs Kylin），两者有何异同？
> - CPU架构信息（通过`uname -m`和`/proc/cpuinfo`获取），不同架构的输出有何差异？
> - 内核版本的差异
> - 如果你的环境涉及不同架构，对比x86和ARM64的寄存器差异：x86有EAX/EBX/ECX等，ARM64有X0-X30等

## 7.2 包管理器对比：yum/dnf vs apt

麒麟操作系统基于RPM包管理体系，使用`yum`或`dnf`作为包管理器，而非Ubuntu使用的`apt`。以下是常用命令对比：

| 功能 | Ubuntu (apt) | 麒麟OS (yum/dnf) |
|------|-------------|-------------------|
| 更新软件源 | `sudo apt update` | `sudo yum makecache` |
| 安装软件 | `sudo apt install 包名` | `sudo yum install 包名` |
| 卸载软件 | `sudo apt remove 包名` | `sudo yum remove 包名` |
| 搜索软件 | `apt search 关键字` | `yum search 关键字` |
| 查看已安装 | `dpkg -l` | `rpm -qa` |
| 查看包信息 | `apt show 包名` | `yum info 包名` |

尝试使用`yum`搜索和查看一些软件包：

```shell
yum search gcc
yum info gcc
rpm -qa | grep gcc
```

**查看系统已配置的软件源：**

```shell
cat /etc/yum.repos.d/*.repo
```

你会看到麒麟操作系统使用的是麒麟官方软件源。

## 7.3 了解CPU架构

通过以下命令深入了解当前系统的CPU架构：

```shell
# 查看CPU架构
uname -m

# 查看CPU详细信息
lscpu

# 查看GCC默认目标架构
gcc -dumpmachine
```

### x86_64与ARM64架构对比

x86_64（Intel/AMD设计）和ARM64（ARM公司设计，又称AArch64）是两种主流的CPU架构。以下是主要区别对比：

| 特性 | x86/x86_64 (CISC) | ARM64 (RISC) |
|------|---------------------|--------------|
| 指令集类型 | 复杂指令集（CISC） | 精简指令集（RISC） |
| 通用寄存器数量 | x86: 8个32位; x86_64: 16个64位 | 31个64位（X0-X30） |
| 指令长度 | 可变长度（1-15字节） | 固定长度（4字节） |
| 功耗 | 较高 | 较低 |
| 典型应用 | PC、服务器 | 手机、嵌入式、ARM服务器 |
| Linux内核镜像 | bzImage（压缩） | Image（未压缩） |
| QEMU命令 | `qemu-system-i386/x86_64` | `qemu-system-aarch64` |
| 串口设备名 | `ttyS0` | `ttyAMA0` |

在前面第二至五部分你做的i386实验和本部分的对比，是理解不同架构操作系统差异的极好切入点。

> **思考题：** 为什么ARM架构在手机和嵌入式领域占据主导地位，而x86在桌面和服务器领域更为流行？RISC与CISC两种设计哲学各有什么优缺点？

# 第八部分：探索麒麟OS内核（扩展实验）

> 在前面的实验中，我们编译和启动了i386内核。本部分将在麒麟操作系统中直接探索正在运行的内核，通过实际操作来加深对操作系统内核机制的理解。
>
> **本部分所有操作在麒麟操作系统的终端中执行。**

## 8.1 探索/proc虚拟文件系统

`/proc`是Linux提供的一个虚拟文件系统，它不存在于磁盘上，而是由内核在内存中动态生成的。通过`/proc`，我们可以查看和修改内核的运行时信息。

**查看内核版本：**

```shell
cat /proc/version
```

**查看系统运行时间：**

```shell
cat /proc/uptime
```

> 第一个数字是系统运行的总秒数，第二个数字是所有CPU空闲时间的总和。

**查看内存使用详情：**

```shell
cat /proc/meminfo
```

**查看当前进程的信息：**

```shell
cat /proc/self/status
```

> `/proc/self`是一个特殊的符号链接，总是指向当前进程的`/proc/[pid]`目录。

**查看系统支持的文件系统：**

```shell
cat /proc/filesystems
```

**查看内核启动命令行参数：**

```shell
cat /proc/cmdline
```

> **思考题：**
> 1. `/proc`目录下的数字目录（如`/proc/1`、`/proc/2`等）分别代表什么？请查看`/proc/1/status`，这是哪个进程？
> 2. 对比麒麟OS上`/proc/cpuinfo`的输出和你本地x86环境上的输出，有哪些字段不同？
> 3. 找出当前系统中占用内存最多的前5个进程（提示：`ps aux --sort=-%mem | head -6`）。

## 8.2 分析系统启动日志

`dmesg`命令可以查看内核的环形缓冲区日志，其中包含了系统启动过程中内核输出的所有信息。

> **注意：** 麒麟OS默认限制了普通用户访问`dmesg`（`kernel.dmesg_restrict=1`）。可以使用以下两种方式查看内核日志：
>
> - **方式一（推荐）：** 使用`journalctl -k`，普通用户即可执行。
> - **方式二：** 使用`sudo dmesg`（需要输入密码）。

```shell
# 方式一：使用 journalctl 查看内核日志（推荐）
journalctl -k --no-pager | head -50

# 方式二：使用 sudo dmesg
sudo dmesg | head -50
```

观察内核启动的关键阶段：

```shell
# 查看与启动相关的信息
journalctl -k --no-pager | grep -i "boot"

# 查看CPU初始化信息
journalctl -k --no-pager | grep -i "cpu"

# 查看内存初始化信息
journalctl -k --no-pager | grep -i "memory"

# 查看设备驱动加载信息
journalctl -k --no-pager | grep -i "driver"
```

> **思考题：** 从`dmesg`日志中，你能识别出内核启动的哪些阶段？请列举至少3个关键阶段并简要说明。与你在第三部分中用GDB调试i386内核时看到的`start_kernel`函数有什么联系？

## 8.3 理解内核模块

Linux内核采用模块化设计，部分功能可以编译为可加载模块（`.ko`文件），在运行时动态加载和卸载。这是一种灵活的内核扩展机制。

**查看已加载的内核模块：**

```shell
lsmod
```

**查看某个模块的详细信息：**

```shell
modinfo ext4
```

> `ext4`是Linux最常用的文件系统模块。观察输出中的`description`、`author`、`depends`等字段。

### 编写一个简单的Hello World内核模块

创建工作目录：

```shell
mkdir -p ~/lab1/mymodule
cd ~/lab1/mymodule
```

使用`vi`创建内核模块源文件`hello_module.c`：

```c
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
```

创建Makefile（**注意：缩进必须使用Tab字符，不能使用空格**）：

```makefile
obj-m += hello_module.o

KDIR := /lib/modules/$(shell uname -r)/build

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean
```

编译模块：

```shell
make
```

编译成功后会生成`hello_module.ko`文件。

> **提示：** 编译时可能会出现“`warning: the compiler differs from the one used to build the kernel`”警告，这是因为当前GCC小版本与编译内核时使用的版本略有不同，通常不影响使用，可以忽略。

> **知识点：** 使用`file hello_module.ko`查看文件类型。在x86_64系统上，你会看到`ELF 64-bit ... x86-64`；在ARM64系统上则是`ELF 64-bit ... aarch64`。这说明内核模块必须与当前运行内核的架构一致。

**加载模块：**

```shell
sudo insmod hello_module.ko
```

> **注意：** `insmod`和`rmmod`需要root权限，使用`sudo`时需要输入你的用户密码。

**查看模块输出（内核模块使用`printk`而非`printf`，输出在内核日志中）：**

```shell
# 使用 journalctl 查看内核日志（推荐）
journalctl -k --no-pager | tail -5

# 或使用 sudo dmesg
sudo dmesg | tail -5
```

你应该能看到：`Hello! This module is running on Kylin OS.`

**查看模块是否已加载：**

```shell
lsmod | grep hello
```

**卸载模块：**

```shell
sudo rmmod hello_module
journalctl -k --no-pager | tail -3
```

你应该能看到：`Goodbye! Module unloaded from Kylin OS.`

> **思考题：**
> 1. 用户态程序使用`printf`输出到标准输出（终端），内核模块使用什么函数？输出到哪里？为什么不能在内核中使用`printf`？
> 2. `MODULE_LICENSE("GPL")`这一行的作用是什么？如果去掉会怎样？（提示：尝试去掉后编译加载，观察`dmesg`的输出）
> 3. `__init`和`__exit`宏的作用是什么？（提示：与内存优化有关，`__init`标记的函数在执行完后其内存会被释放）

## 8.4 观察系统调用

系统调用是用户态程序请求内核服务的接口，是用户态与内核态的桥梁。我们可以使用`strace`工具来观察一个程序执行了哪些系统调用。

安装strace（如果未安装）：

```shell
sudo yum install -y strace
```

观察`ls`命令的系统调用：

```shell
strace ls /tmp 2>&1 | head -30
```

观察`cat`命令读取文件时的系统调用：

```shell
strace cat /proc/version 2>&1 | head -30
```

统计一个程序使用的系统调用种类和次数：

```shell
strace -c ls /tmp
```

> **思考题：**
> 1. `ls`命令主要使用了哪些系统调用？`openat`、`getdents64`、`write`分别完成什么功能？
> 2. 编写一个简单的Hello World C程序（在麒麟OS上用`gcc -o hello hello.c -static`编译），用`strace ./hello`观察它执行了哪些系统调用。一个简单的Hello World程序背后到底调用了多少个系统调用？
> 3. 如果你有不同架构的环境（如x86_64和ARM64），对比两种环境上用`strace`观察同一个程序的输出，系统调用的名称是否完全一致？（提示：绝大部分系统调用名称在各架构上是一致的，但底层的系统调用号不同）

## 8.5 使用QEMU体验跨架构运行（选做）

QEMU不仅支持运行同架构的虚拟机，还支持跨架构模拟运行。你可以在麒麟OS上体验运行不同架构的系统。

**查看QEMU支持的架构：**

```shell
ls /usr/bin/qemu-system-*
```

**如果你的麒麟OS是x86_64架构：**

你已经在第二至五部分中使用`qemu-system-i386`运行了i386内核。现在可以尝试用QEMU模拟ARM64架构：

```shell
# 安装ARM64的QEMU（如果未安装）
sudo yum install qemu-system-aarch64

# 用QEMU启动一个ARM64系统（需要ARM64内核镜像）
# qemu-system-aarch64 -M virt -cpu cortex-a57 -m 512 -kernel <arm64-kernel-image> -nographic -append "console=ttyAMA0"
```

**如果你的麒麟OS是ARM64架构：**

如果你在第二至五部分的实验中编译了i386内核和Busybox initramfs（在x86_64环境中完成），可以将它们复制到本机，然后用QEMU运行：

```shell
# 注意：麒麟OS ARM64版上没有 qemu-system-i386，但可以使用 qemu-system-x86_64 运行 i386 内核
qemu-system-x86_64 -kernel bzImage -initrd initramfs-busybox-x86.cpio.gz -nographic -append "console=ttyS0" -m size=512
```

> 如果成功启动，你会看到与x86环境相同的Busybox系统。这证明了QEMU的强大之处——它可以在不同架构的硬件上**模拟**另一种CPU来运行对应内核！
>
> **思考题：** 跨架构模拟运行内核时，启动速度与在原生架构上运行相比如何？为什么会有差异？（提示：跨架构"模拟"需要逐条翻译指令，而在同架构上可以利用KVM硬件虚拟化加速）

---

# 附录

## A. 常用QEMU操作

| 操作 | 快捷键/命令 |
|------|------------|
| 退出QEMU | `Ctrl+A` 然后 `X` |
| 切换到QEMU监视器 | `Ctrl+A` 然后 `C` |
| 帮助 | `Ctrl+A` 然后 `H` |

## B. 常用GDB命令

| 命令 | 缩写 | 功能 |
|------|------|------|
| `break <函数名/地址>` | `b` | 设置断点 |
| `continue` | `c` | 继续运行 |
| `next` | `n` | 单步执行（不进入函数） |
| `step` | `s` | 单步执行（进入函数） |
| `print <变量>` | `p` | 打印变量值 |
| `info registers` | `i r` | 查看寄存器 |
| `backtrace` | `bt` | 查看调用栈 |
| `list` | `l` | 查看源代码 |
| `info breakpoints` | `i b` | 查看所有断点 |
| `delete <编号>` | `d` | 删除断点 |
| `quit` | `q` | 退出GDB |

## C. 麒麟OS常用yum命令

```shell
sudo yum install <包名>          # 安装软件包
sudo yum remove <包名>           # 卸载软件包
sudo yum update                   # 更新所有软件包
sudo yum search <关键字>         # 搜索软件包
yum info <包名>                   # 查看软件包信息
rpm -qa                           # 查看所有已安装的RPM包
rpm -qa | grep <关键字>          # 搜索已安装的包
```

## D. SSH连接与文件传输技巧（适用于远程环境）

如果你的麒麟OS环境需要通过SSH远程登录，以下技巧可以提高效率。如果你直接在麒麟OS本机操作，可以跳过本节。

如果频繁SSH登录觉得麻烦，可以配置SSH免密登录：

```shell
# 在你的本地电脑上生成SSH密钥（如果还没有）
ssh-keygen -t ed25519

# 将公钥复制到麒麟OS服务器
ssh-copy-id -p 722 你的用户名@服务器地址
```

配置完成后，后续登录将不再需要输入密码。

也可以在本地`~/.ssh/config`中配置快捷方式：

```
Host kylin
    HostName 服务器地址
    Port 722
    User 你的用户名
```

配置后只需输入`ssh kylin`即可登录。

使用`scp`在本地和麒麟服务器之间传输文件：

```shell
# 上传文件到服务器
scp -P 722 本地文件路径 你的用户名@服务器地址:远程路径

# 从服务器下载文件
scp -P 722 你的用户名@服务器地址:远程文件路径 本地路径
```

## E. 校园网环境加速

部分软件在校园网下下载可能缓慢，建议使用镜像源。此处推荐两个：

- [Matrix 镜像源](https://mirrors.matrix.moe)（此镜像源位于校内）
- [清华 Tuna 镜像源](https://mirrors.tuna.tsinghua.edu.cn)

例如，

- <https://mirrors.matrix.moe/kernel/v5.x/> 的 `linux-5.10.19.tar.xz` 等压缩文件提供了 Linux Kernel 源代码的下载。
- <https://mirrors.tuna.tsinghua.edu.cn/kernel/> 也提供了Linux Kernel源代码的镜像下载。
- 参照[帮助文档](https://mirrors.matrix.moe/docs/ubuntu)可以设置 Ubuntu 软件源更新使用 Matrix 镜像源。另外，Matrix 镜像源也提供 Arch Linux、Debian、CentOS、Fedora 等发行版的软件源镜像。

## F. 问题排查

**Q: QEMU启动后没有任何输出？**
A: 如果使用了`-S`参数，这是正常的，QEMU在等待GDB连接。如果没有使用`-S`，请检查`-append`参数中的串口设备名是否正确（i386使用`ttyS0`，ARM64使用`ttyAMA0`）。

**Q: 编译i386内核报错找不到头文件？**
A: 确保已安装`libc6-dev-i386`：`sudo apt install libc6-dev-i386`

**Q: 编译helloworld时`-m32`报错？**
A: 确保已安装32位开发库：`sudo apt install gcc-multilib`

**Q: 内核编译报错`openssl/opensslv.h: No such file`？**
A: Ubuntu上安装：`sudo apt install libssl-dev`；麒麟OS上安装：`sudo yum install openssl-devel`

**Q: `make menuconfig`报错？**
A: Ubuntu上安装：`sudo apt install libncurses5-dev`；麒麟OS上安装：`sudo yum install ncurses-devel`

**Q: 内核模块编译报错找不到build目录？（麒麟OS上）**
A: 安装kernel-devel包：`sudo yum install kernel-devel-$(uname -r)`

**Q: 编译内核时内存不足（OOM Killed）？**
A: 减少并行编译任务数，使用`make -j2`甚至`make`（单线程编译）。

**Q: GDB连接QEMU后无法设置断点？**
A: 确保内核编译时开启了debug info选项。使用`file vmlinux`命令检查，输出应显示`with debug_info`。

**Q: SSH连接麒麟服务器被拒绝？**
A: 如果使用SSH远程连接，请确认端口号和用户名是否正确。如果多次输错密码可能被临时封禁，请稍等几分钟再试。

**Q: 麒麟OS上找不到某个软件包？**
A: 麒麟OS使用`yum`包管理器。使用`yum search 关键字`搜索。注意麒麟OS的包名可能与Ubuntu不同（如`libssl-dev`对应`openssl-devel`，`libncurses5-dev`对应`ncurses-devel`）。

**Q: ARM64架构上如何完成i386内核编译实验？**
A: ARM64架构需要交叉编译。第六部分（Linux 0.11）可通过构建i686交叉编译器完成（详见附录G.1）；第二至五部分建议将编译目标改为ARM64架构内核（详见附录G.2），或在x86_64虚拟机中完成。

**Q: ARM64上编译Linux 0.11报错？**
A: ARM64需要构建i686-elf交叉编译器，不能直接使用`gcc -m32`。详见附录G.1的完整步骤。

## G. ARM64架构用户实验指南

如果你的操作系统（Ubuntu或麒麟OS）运行在ARM64（AArch64）架构上，i386/x86相关的编译实验需要做调整。以下指南同时适用于Ubuntu ARM64和麒麟OS ARM64环境。

### G.1 在ARM64上编译和启动Linux 0.11（替代第六部分）

ARM64架构不支持`gcc -m32`，需要构建一套i686交叉编译工具链。以下步骤已在麒麟V11 ARM64上验证通过。

#### 第一步：安装构建依赖

**麒麟OS：**

```shell
sudo yum install -y gcc gcc-c++ make bison flex texinfo \
    gmp-devel mpfr-devel libmpc-devel wget
```

**Ubuntu：**

```shell
sudo apt install -y gcc g++ make bison flex texinfo \
    libgmp-dev libmpfr-dev libmpc-dev wget
```

#### 第二步：构建binutils（约2分钟）

```shell
mkdir -p ~/lab1/cross-tools && cd ~/lab1/cross-tools
# 下载binutils源码（使用国内镜像加速）
wget https://mirrors.tuna.tsinghua.edu.cn/gnu/binutils/binutils-2.36.tar.xz
tar xf binutils-2.36.tar.xz

# 配置并编译
export PREFIX=$HOME/lab1/cross-tools/install
export TARGET=i686-elf
mkdir -p build-binutils && cd build-binutils
../binutils-2.36/configure --target=$TARGET --prefix=$PREFIX \
    --with-sysroot --disable-nls --disable-werror
make -j$(nproc)
make install
```

#### 第三步：构建GCC交叉编译器（约10分钟）

```shell
cd ~/lab1/cross-tools
# 下载GCC源码
wget https://mirrors.tuna.tsinghua.edu.cn/gnu/gcc/gcc-12.2.0/gcc-12.2.0.tar.xz
tar xf gcc-12.2.0.tar.xz

# 配置并编译（C-only，freestanding模式，不需要C库）
export PATH="$PREFIX/bin:$PATH"
mkdir -p build-gcc && cd build-gcc
../gcc-12.2.0/configure --target=$TARGET --prefix=$PREFIX \
    --disable-nls --enable-languages=c --without-headers \
    --disable-hosted-libstdcxx --disable-libssp --disable-libquadmath \
    --disable-libgomp --disable-libatomic --disable-threads --disable-shared
make -j$(nproc) all-gcc
make install-gcc
make -j$(nproc) all-target-libgcc
make install-target-libgcc
```

#### 第四步：创建符号链接

Linux 0.11教学版的Makefile使用`i386-elf-*`前缀（macOS配置），需创建符号链接：

```shell
cd $PREFIX/bin
for tool in as ld gcc cpp ar strip objcopy objdump nm ranlib readelf; do
    ln -sf i686-elf-$tool i386-elf-$tool
done
```

#### 第五步：编译Linux 0.11

```shell
export PATH=$HOME/lab1/cross-tools/install/bin:$PATH
cd ~/lab1/linux-0.11
make clean
make UNAME=Darwin   # 使用Darwin配置（i386-elf-*交叉编译器）
```

> 编译会产生一些inline函数的警告，这是正常的（老代码与新编译器的兼容性问题），不影响运行。

验证编译结果：

```shell
file Image              # 应显示：DOS/MBR boot sector
file tools/system       # 应显示：ELF 32-bit LSB executable, Intel 80386
```

#### 第六步：使用QEMU启动Linux 0.11

ARM64上没有`qemu-system-i386`，使用`qemu-system-x86_64`代替（完全兼容32位）：

```shell
# 普通启动（-s -S 用于GDB调试）
qemu-system-x86_64 -m 16 -boot a -fda Image -hda hdc-0.11.img -s -S
```

> **注意：** ARM64上的GDB不支持x86调试目标。如需GDB远程调试，需安装`gdb-multiarch`：
>
> ```shell
> sudo yum install gdb-multiarch   # 如果仓库中有
> # 或从源码编译GDB并启用i386支持
> ```
>
> 使用gdb-multiarch连接：
>
> ```shell
> gdb-multiarch tools/system
> (gdb) target remote :1234
> (gdb) break main
> (gdb) continue
> ```

### G.2 编译ARM64内核（替代第二至五部分）

另一种方案是将编译目标从i386改为ARM64，实验流程和原理完全一致。

#### 编译ARM64内核（替代第二部分）

```shell
cd ~/lab1/linux-5.10.19
make defconfig             # 使用当前架构的默认配置（而非i386_defconfig）
make menuconfig            # 同样开启debug info
make -j$(nproc)
```

编译完成后，内核镜像位于`arch/arm64/boot/Image`（而非`arch/x86/boot/bzImage`），符号表仍为`vmlinux`。

#### 使用QEMU启动ARM64内核（替代第三部分）

```shell
qemu-system-aarch64 -M virt -cpu cortex-a57 -m 1024 \
    -kernel linux-5.10.19/arch/arm64/boot/Image \
    -append "console=ttyAMA0" \
    -nographic -s -S
```

> 注意：ARM64使用`ttyAMA0`作为串口设备名（而非i386的`ttyS0`），QEMU使用`-M virt`指定虚拟机器类型。

#### GDB调试（替代第三部分）

```shell
gdb-multiarch linux-5.10.19/vmlinux   # 可能需要安装gdb-multiarch
# 或直接使用
gdb linux-5.10.19/vmlinux
```

在GDB中连接QEMU的步骤与i386完全相同：`target remote:1234`，`break start_kernel`，`c`。

#### 编译helloworld和Busybox（替代第四、五部分）

去掉所有`-m32`和`-march=i386`参数，直接编译即可（编译出的是ARM64版本）：

```shell
# helloworld
gcc -o helloworld -static helloworld.c

# Busybox menuconfig中不需要设置-m32相关的CFLAGS和LDFLAGS
# 只需开启Build static binary即可
```

打包initramfs的方式完全一致，QEMU启动时使用`qemu-system-aarch64`即可。

