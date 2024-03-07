#!/bin/bash
set -e;

source ${K230_SDK_ROOT}/board/common/gen_image_script/gen_image_comm_func.sh


env_dir="${K230_SDK_ROOT}/board/common/env"

GENIMAGE_CFG_DIR="${K230_SDK_ROOT}/board/common/gen_image_cfg"
GENIMAGE_CFG_SD="${GENIMAGE_CFG_DIR}/genimage-sdcard.cfg"
GENIMAGE_CFG_SD_AES="${GENIMAGE_CFG_DIR}/genimage-sdcard_aes.cfg"
GENIMAGE_CFG_SD_SM="${GENIMAGE_CFG_DIR}/genimage-sdcard_sm.cfg"
#GENIMAGE_CFG_SD_DDR4="${GENIMAGE_CFG_DIR}/genimage-sdcard_ddr4.cfg"
GENIMAGE_CFG_SPI_NOR="${GENIMAGE_CFG_DIR}/genimage-spinor.cfg"
GENIMAGE_CFG_SPI_NAND="${GENIMAGE_CFG_DIR}/genimage-spinand.cfg"
GENIMAGE_CFG_SD_REMOTE="${GENIMAGE_CFG_DIR}/genimage-sdcard_remote.cfg"

cfg_data_file_path="${GENIMAGE_CFG_DIR}/data"
quick_boot_cfg_data_file="${GENIMAGE_CFG_DIR}/data/quick_boot.bin"
face_database_data_file="${GENIMAGE_CFG_DIR}/data/face_data.bin"
sensor_cfg_data_file="${GENIMAGE_CFG_DIR}/data/sensor_cfg.bin"
ai_mode_data_file="${BUILD_DIR}/images/big-core/ai_mode.bin" #"${GENIMAGE_CFG_DIR}/data/ai_mode.bin"
speckle_data_file="${GENIMAGE_CFG_DIR}/data/speckle.bin"
rtapp_data_file="${BUILD_DIR}/images/big-core/fastboot_app.elf"
shrink_rootfs()
{
	#�ü�С��rootfs
	local KERNELRELEASE="$(cat ${LINUX_BUILD_DIR}/include/config/kernel.release 2> /dev/null)"
	local lib_mod="lib/modules/${KERNELRELEASE}/"
	cd ${BUILD_DIR}/images/little-core/rootfs/; 
	#rm -rf lib/modules/;
	rm -rf ${lib_mod}/kernel/drivers/mtd  ${lib_mod}/kernel/drivers/usb ${lib_mod}/kernel/fs/efivarfs
	rm -rf  mnt/backchannel_client
	#rm -rf  #mnt/k_ipcm.ko
	rm -rf  mnt/libsharefs.a
	rm -rf  mnt/libsharefs.so
	#rm -rf  mnt/sharefs
	rm -rf  mnt/camera
	rm -rf  mnt/canaan-camera.sh
	rm -rf  mnt/comm_client
	rm -rf  mnt/doorbell.g711u
	rm -rf  mnt/peephole_dev
	rm -rf  mnt/red.jpg
	rm -rf  mnt/rpc_client
	rm -rf  mnt/rpc_server
	rm -rf  mnt/rtsp_demo
	rm -rf  mnt/rtsp_server
	rm -rf  mnt/sample_audio
	rm -rf  mnt/sample_kplayer
	rm -rf  mnt/sample_reader
	rm -rf  mnt/sample_sender
	rm -rf  mnt/sample_vdec
	rm -rf  mnt/sample_vdss
	rm -rf  mnt/sample_venc
	rm -rf  mnt/sample_vicap
	rm -rf  mnt/sample_vicap_dump_lite
	rm -rf  mnt/sample_virtual_vio
	rm -rf  mnt/sample_vo
	rm -rf  mnt/wsd_test


	rm -rf lib/libstdc++*;
	rm -rf usr/bin/fio;
	rm -rf usr/bin/usb_test;
	rm -rf usr/bin/hid_gadget_test;
	rm -rf usr/bin/gadget*;
	rm -rf usr/bin/otp_test_demo;
	rm -rf usr/bin/iotwifi*;
	rm -rf usr/bin/i2c-tools.sh;
	rm -rf app/;
	rm -rf lib/tuning-server;	
	rm -rf usr/bin/stress-ng  bin/bash usr/sbin/sshd usr/bin/trace-cmd usr/bin/lvgl_demo_widgets;
	rm -rf    etc/ssh/moduli  usr/bin/ssh-keygen \
		usr/libexec/ssh-keysign  usr/bin/ssh-keyscan  usr/bin/ssh-add usr/bin/ssh-agent usr/libexec/ssh-pkcs11-helper\
		   usr/lib/libvg_lite_util.so  usr/bin/par_ops usr/bin/sftp  usr/libexec/lzo/examples/lzotest;
	#find . -name *.ko | xargs rm -rf ;	
	fakeroot -- ${K230_SDK_ROOT}/tools/mkcpio-rootfs.sh;
	cd ../;  tar -zcf rootfs-final.tar.gz rootfs;

	#�ü����rootfs;
	cd ${BUILD_DIR}/images/big-core/root/bin/;
	find . -type f  -not -name init.sh  -not -name  fastboot_app.elf  test.kmodel  | xargs rm -rf ; 

	if [ -f "${K230_SDK_ROOT}/src/big/mpp/userapps/src/vicap/src/isp/sdk/t_frameworks/t_database_c/calibration_data/sensor_cfg.bin" ]; then
        mkdir -p ${cfg_data_file_path};
		cp ${K230_SDK_ROOT}/src/big/mpp/userapps/src/vicap/src/isp/sdk/t_frameworks/t_database_c/calibration_data/sensor_cfg.bin ${cfg_data_file_path}/sensor_cfg.bin
	fi

}

#���ɿ���uboot������linux�汾�ļ�
gen_linux_bin_ramdisk ()
{
	local OPENSBI_KERNEL_DTB_MAX_SIZE="0x2100000"
	local mkimage="${UBOOT_BUILD_DIR}/tools/mkimage"
	local LINUX_SRC_PATH="src/little/linux"
	local LINUX_DTS_PATH="src/little/linux/arch/riscv/boot/dts/kendryte/${CONFIG_LINUX_DTB}.dts"

	local OPENSBI_LINUX_BASE="$( printf '0x%x\n' $[${CONFIG_MEM_LINUX_SYS_BASE}+0])"
	local RAMDISK_ADDR="$( printf '0x%x\n' $[${OPENSBI_LINUX_BASE}+${OPENSBI_KERNEL_DTB_MAX_SIZE}])"

	
	cd  "${BUILD_DIR}/images/little-core/" ; 
	cpp -nostdinc -I ${K230_SDK_ROOT}/${LINUX_SRC_PATH}/include -I ${K230_SDK_ROOT}/${LINUX_SRC_PATH}/arch  -undef -x assembler-with-cpp ${K230_SDK_ROOT}/${LINUX_DTS_PATH}  hw/k230.dts.txt
	

	#ROOTFS_BASE=`cat hw/k230.dts.txt | grep initrd-start | awk -F " " '{print $4}' | awk -F ">" '{print $1}'`
	ROOTFS_BASE="${RAMDISK_ADDR}"
	ROOTFS_SIZE=`ls -lt rootfs-final.cpio.gz | awk '{print $5}'`
	((ROOTFS_END= $ROOTFS_BASE + $ROOTFS_SIZE))
	ROOTFS_END=`printf "0x%x" $ROOTFS_END`
	sed -i "s/linux,initrd-end = <0x0 .*/linux,initrd-end = <0x0 $ROOTFS_END>;/g" hw/k230.dts.txt
	#linux,initrd-start = <0x0 0xa100000>;
	sed -i "s/linux,initrd-start = <0x0 .*/linux,initrd-start = <0x0  $ROOTFS_BASE>;/g" hw/k230.dts.txt

	${LINUX_BUILD_DIR}/scripts/dtc/dtc -I dts -O dtb hw/k230.dts.txt  >k230.dtb;		
	k230_gzip fw_payload.bin;
	cp rootfs-final.cpio.gz rd;
	${mkimage} -A riscv -O linux -T multi -C gzip -a ${CONFIG_MEM_LINUX_SYS_BASE} -e ${CONFIG_MEM_LINUX_SYS_BASE} -n linux -d fw_payload.bin.gz:rd:k230.dtb  ulinux.bin;

	add_firmHead  ulinux.bin 
	mv fn_ulinux.bin  linux_system.bin
	[ -f fa_ulinux.bin ] && mv fa_ulinux.bin  linux_system_aes.bin
	[ -f fs_ulinux.bin ] && mv fs_ulinux.bin  linux_system_sm.bin
	rm -rf rd;
}
copye_file_to_images;
if [ "${CONFIG_SUPPORT_LINUX}" = "y" ]; then
	gen_version;
	add_dev_firmware;
	shrink_rootfs;
	gen_linux_bin_ramdisk;
	#gen_final_ext2;
fi
[ "${CONFIG_SUPPORT_RTSMART}" = "y" ] &&  gen_rtt_bin;

gen_uboot_bin;
gen_env_bin;
copy_app;

# if [ "${CONFIG_REMOTE_TEST_PLATFORM}" = "y" ] ; then
# 	gen_image ${GENIMAGE_CFG_SD_REMOTE}   sysimage-sdcard.img
# else
# 	gen_image ${GENIMAGE_CFG_SD}   sysimage-sdcard.img
# fi

# if [ "${CONFIG_GEN_SECURITY_IMG}" = "y" ] ; then
# 	gen_image  ${GENIMAGE_CFG_SD_AES}  sysimage-sdcard_aes.img
# 	gen_image ${GENIMAGE_CFG_SD_SM}  sysimage-sdcard_sm.img
# fi

# if [ "${CONFIG_SPI_NOR}" = "y" ]; then
# 	cd  ${BUILD_DIR}/;rm -rf images_bak;cp images images_bak -r;
# 	shrink_rootfs_common
# 	gen_image_spinor
# 	cd  ${BUILD_DIR}/;rm -rf images_spinor;mv  images images_spinor; mv  images_bak images; cp images_spinor/sysimage-spinor32m*.img images;
# fi
# [ "${CONFIG_SPI_NAND}" = "y" ] && gen_image_spinand;


gen_image ${GENIMAGE_CFG_SD}   sysimage-sdcard.img
cd  ${BUILD_DIR}/images/
rm -rf  sysimage-sdcard_aes.img  sysimage-sdcard_sm.img  *.vfat
cp sysimage-sdcard.img*  ${K230_CANMV_BUILD_DIR}/images/
cp  -P CanMV-K230_micropython*  ${K230_CANMV_BUILD_DIR}/images/









