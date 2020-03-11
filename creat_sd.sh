#!/bin/bash
#  sd卡最后部分内容布局(假定一个扇区大小为512个字节)：
# =-=================================================================================================================
#   BL2(u-boot.bin)   BL1(u-boot-spl-16k.bin,16个扇区) Signature(1个扇区） Reserved(1个或者1025个扇区，跟sd卡大小相关）
# =================================================================================================================== 
# 其中，Reserved扇区在SD卡小于等于1G时为1个字节，在4G及以上情况下为1025个字节）(我仅仅测试了1G和4G容量sd卡，其他大小未测试)
#
# 使用当前脚本时需要注意需要配置SD_DEV,SD_IS_1G,UBOOT_SECTORS,UBOOT_ROOT这几个变量的值

SD_DEV=/dev/sdb   #sd卡分区名称
SD_IS_1G=n        #sd卡是否1G（只能是y或者n）
UBOOT_SECTORS=508 #uboot所占大小，通常在开发板配置文件（请查看CONFIG_SYS_MMC_U_BOOT_SIZE的配置，转换成扇区数)
UBOOT_ROOT=.      #uboot根目录路径


# Disk /dev/sdb: 3.7 GiB, 3965190144 bytes, 7744512 sectors
# Units: sectors of 1 * 512 = 512 bytes
# Sector size (logical/physical): 512 bytes / 512 bytes
# I/O size (minimum/optimal): 512 bytes / 512 bytes
# Disklabel type: dos
# Disk identifier: 0x4c0d5923

# Device     Boot Start     End Sectors  Size Id Type
# /dev/sdb1         128 7477375 7477248  3.6G  c W95 FAT32 (LBA)

TOTAL_SECTORS=`sudo fdisk -l $SD_DEV | grep ',.*sectors' | tr -d '[:lower:][:upper:][:punct:]'| awk '{print $3}'`
case $SD_IS_1G in 
    "n") IROM_SECTORS=`expr $TOTAL_SECTORS - 1024` ;; #如果sd卡大于1G，那么IROM能识别的扇区数需要减去1024. 
    "y") IROM_SECTORS=$TOTAL_SECTORS ;; #如果sd卡小于等于1G,那么IROM能识别的扇区数等于sd卡的扇区数目
     *) IROM_SECTORS=$TOTAL_SECTORS ;; #如果sd卡小于等于1G,那么IROM能识别的扇区数等于sd卡的扇区数目
esac
LAST1M_OFF=`expr $TOTAL_SECTORS - 2000` #最后1M字节开始地址
SPL_START=`expr $IROM_SECTORS - 18`  #u-boot-spl-16k.bin文件的开始地址
#SPL_START=`expr $IROM_SECTORS - 1042` #SDHC偏移
UBOOT_START=`expr $SPL_START - $UBOOT_SECTORS` #u-boot.bin文件的开始地址
echo $TOTAL_SECTORS  $IROM_SECTORS $LAST1M_OFF $SPL_START $UBOOT_START

sudo dd if=/dev/zero of=$SD_DEV bs=512 seek=$LAST1M_OFF count=2000
sudo dd if=$UBOOT_ROOT/nand_spl/u-boot-spl-16k.bin of=$SD_DEV bs=512 seek=$SPL_START conv=fdatasync
sudo dd if=$UBOOT_ROOT/u-boot.bin of=$SD_DEV bs=512 seek=$UBOOT_START conv=fdatasync