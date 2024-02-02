<img height=230 src="images/CanMV_logo_800x260.png">

**CanMV, 让 AIOT 更简单～**

CanMV 的目的是让 AIOT 编程更简单， 基于 [Micropython](http://www.micropython.org) 语法, 运行在[Canaan](https://www.canaan-creative.com/)强大的嵌入式AI SOC系列上。目前它在K230上运行。

## 镜像下载

1. **[main branch](https://github.com/kendryte/k230_canmv/tree/main)**: Github默认分支，作为release分支，编译release镜像自动发布至[Release](https://github.com/kendryte/k230_canmv/releases)页面
2. **[dev branch](https://github.com/kendryte/k230_canmv/tree/dev)**: Github开发分支，仅供测试，镜像可通过[GitHub Actions](https://github.com/kendryte/k230_canmv/actions)页面下载`dev`分支中不同commit id对应的artifacts产物，*默认缓存90天*
3. 预编译release镜像：请访问[嘉楠开发者社区](https://developer.canaan-creative.com/resource), 然后在`K230/Images`分类中，下载镜像文件名包含`micropython`的文件，并烧录至SD卡中。(镜像文件名格式：`CanMV-K230_micropython_*.img.gz`)

> 下载的镜像默认为`.gz`压缩格式，需先解压缩，然后再烧录。
> micropython镜像与K230 SDK镜像所支持的功能并不相同，请勿下载K230 SDK镜像来使用micropython

## 快速开始

### 自行编译镜像

```sh
git clone k230_canmv
cd k230_canmv
make prepare_sourcecode
# 生成docker镜像（第一次编译需要，已经生成docker镜像后跳过此步骤，可选）
docker build -f k230_sdk/tools/docker/Dockerfile -t k230_docker k230_sdk/tools/docker
# 启动docker环境(可选)
docker run -u root -it -v $(pwd):$(pwd) -v $(pwd)/k230_sdk/toolchain:/opt/toolchain -w $(pwd) k230_docker /bin/bash
# 默认使用canmv板卡，如果需要使用其他板卡，请使用 make CONF=k230_xx_defconfig，支持的板卡在configs目录下
make
```

编译完成后会在`output/k230_xx_defconfig/images`目录下生成`sysimage-sdcard.img`镜像

### 烧录

linux下直接使用dd命令进行烧录，windows下使用烧录工具进行烧录，可参考[K230 SDK烧录镜像文件](https://github.com/kendryte/k230_sdk?tab=readme-ov-file#%E7%83%A7%E5%BD%95%E9%95%9C%E5%83%8F%E6%96%87%E4%BB%B6)

### 获取更新

```sh
git pull
# 更新后如果编译报错，可使用此命令清除之前编译生成的文件(可选)
make clean
make prepare_sourcecode
make
```

详情流程建议参考[K230 CanMV 使用说明](https://github.com/kendryte/k230_canmv_docs/blob/main/zh/userguide/K230_CanMV%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E.md) 或者 [K230 CanMV用户指南](https://developer.canaan-creative.com/k230_canmv/dev/zh/userguide/userguide.html)

## 贡献指南

如果您对本项目感兴趣，想要反馈问题或提交代码，请参考[CONTRIBUTING](.github/CONTRIBUTING.md)

## 联系我们

北京嘉楠捷思信息技术有限公司

网址:[www.canaan-creative.com](https://www.canaan-creative.com/)

商务垂询:[salesAI@canaan-creative.com](salesAI@canaan-creative.com)
