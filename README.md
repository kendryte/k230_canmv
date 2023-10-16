# CANMV

## 编译

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

## 烧录

linux下直接使用dd命令进行烧录，windows下使用烧录工具进行烧录

## 获取更新

```sh
git pull
# 更新后如果编译报错，可使用此命令清除之前编译生成的文件(可选)
make clean
make prepare_sourcecode
make
```
