# CANMV

## 编译环境

### docker环境

```sh
docker pull ai.b-bug.org:5000/k230_sdk:latest
cd /path/to/k230_canmv
docker run -u root -it -v $(pwd):$(pwd) -v $(pwd)/k230_sdk/toolchain:/opt/toolchain -w $(pwd) ai.b-bug.org:5000/k230_sdk /bin/bash
```

### 本地环境

参考K230_SDK本地编译环境搭建

#### 安装genimage

```sh
sudo apt-get install libconfuse-dev
wget https://github.com/pengutronix/genimage/releases/download/v16/genimage-16.tar.xz
tar -xf genimage-16.tar.xz
cd genimage-16
./configure
make -j
sudo make install
```

## 编译流程

```sh
git clone k230_canmv
cd k230_canmv
make prepare_sourcecode
# 启动docker环境(可选)
make
# 默认使用EVB板卡，如果需要使用其他板卡，请使用 make CONF=k230_xx_defconfig，支持的板卡在configs目录下
```

编译完成后会在`output/k230_xx_defconfig/images`目录下生成`sysimage-sdcard.img`镜像

## 烧录

linux下直接使用dd命令进行烧录，windows下使用烧录工具进行烧录

## 启动micropython

在rt-smart命令行下输入`./sdcard/app/micropython`， micropython REPL会在`ttyACMx`上开始交互

## 项目结构

```sh
k230_canmv/
├── configs
├── k230_sdk
├── k230_sdk_overlay
├── micropython
├── micropython_port
└── scripts
```

目录介绍:

1. `configs`: 各种板级配置
1. `k230_sdk`: k230_sdk源码
1. `k230_sdk_overlay`: 基于k230源码的修改
1. `micropython`: micropython源码
1. `micropython_port`: k230 micropython 移植
1. `scripts`: 各种脚本

其中`k230_sdk`, `micropython`是git submodule, 子项目地址为:

- `k230_sdk`: <https://github.com/kendryte/k230_sdk.git>
- `micropython`: <https://github.com/micropython/micropython.git>

`k230_sdk_overlay`中的目录结构与`k230_sdk`相同, 编译时会将`k230_sdk_overlay`同步到`k230_sdk`

## micropython模块添加

`micropython_port`目录大体如下:

```sh
micropython_port/
├── boards
│   └── k230_evb
│       └── mpconfigboard.h
├── core
│   ├── coverage.c
│   ├── coveragecpp.cpp
│   ├── gccollect.c
│   ├── input.c
│   ├── main.c
│   ├── modmachine.c
│   ├── modos.c
│   ├── modtermios.c
│   ├── modtime.c
│   ├── mphalport.c
│   └── mpthreadport.c
├── include
│   ├── core
│   │   ├── input.h
│   │   ├── mpconfigport.h
│   │   └── mphalport.h
│   ├── kpu
│   ├── machine
│   ├── maix
│   ├── mpp
│   └── omv
├── Kconfig
├── kpu
├── machine
├── maix
├── mpp
└── omv
```

目录介绍:

1. `boards`: 各种板级配置
1. `core`: micropython core模块
1. `machine`: machine模块, 包含GPIO, SPI, IIC, UART, PWM, WDT等
1. `kpu`: k230 kpu模块, 包含KPU, AI2D
1. `mpp`: k230 mpp模块, 包含VO, VI, AI, AO, MMZ, VPU, DPU等
1. `maix`: k230 其他模块, 包含IOMUX, PM等
1. `omv`: openmv模块
1. `include`: 各模块头文件
