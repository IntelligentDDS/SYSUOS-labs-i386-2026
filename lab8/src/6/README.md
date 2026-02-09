# src/6 — Wait 的实现

在`build`文件夹下执行下面的命令。

## 编译
```shell
make clean && make build
```

## 运行
```shell
make run
```

## 无图形界面运行（QEMU Monitor）
```shell
make monitor
# 另开终端: telnet 127.0.0.1 45486
```

## 调试
```shell
make debug
```

> **银河麒麟 ARM64**：Makefile 已自动检测平台，无需手动修改。