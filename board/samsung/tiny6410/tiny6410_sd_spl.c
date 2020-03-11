#include <common.h>

#define HM_CONTROL4     *((volatile unsigned int*)(0x8C))


#define HSMMC_CHANNEL		1

#define	TCM_BASE		0x0C004000
#define BL2_BASE		CONFIG_SYS_PHY_UBOOT_BASE

#define CFG_ENV_SIZE    CONFIG_ENV_SIZE
#define CFG_UBOOT_SIZE  CONFIG_SYS_NAND_U_BOOT_SIZE

/* size information */
// #if defined(CONFIG_S3C6400)
// #define SS_SIZE			(4 * 1024)
// #define eFUSE_SIZE		(2 * 1024)	// 1.5k eFuse, 0.5k reserved
// #else
#define SS_SIZE			(8 * 1024)
#define eFUSE_SIZE		(1 * 1024)	// 0.5k eFuse, 0.5k reserved`
// #define eFUSE_SIZE		(513 * 1024)	// 0.5k eFuse, 512.5k reserved`     for SDHC
// #endif

/* movinand definitions */
#define MOVI_BLKSIZE		512

#ifdef CONFIG_BOOT_MOVINAND
#define MOVI_TOTAL_BLKCNT	*((volatile unsigned int*)(TCM_BASE - 0x4))
#define MOVI_HIGH_CAPACITY	*((volatile unsigned int*)(TCM_BASE - 0x8))
#else
#define MOVI_TOTAL_BLKCNT	7864320 // 7864320 // 3995648 // 1003520 /* static movinand total block count: for writing to movinand when nand boot */
#define MOVI_HIGH_CAPACITY	0
#endif

#define MOVI_LAST_BLKPOS	(MOVI_TOTAL_BLKCNT - (eFUSE_SIZE / MOVI_BLKSIZE))
#define MOVI_BL1_BLKCNT		(SS_SIZE / MOVI_BLKSIZE)
#define MOVI_ENV_BLKCNT		0   // sd卡中就不设置环境变量区了 (CFG_ENV_SIZE / MOVI_BLKSIZE)
#define MOVI_BL2_BLKCNT		(CFG_UBOOT_SIZE / MOVI_BLKSIZE)
// #define MOVI_ZIMAGE_BLKCNT	((PART_ROOTFS_OFFSET - PART_ZIMAGE_OFFSET) / MOVI_BLKSIZE)
#define MOVI_BL2_POS		(MOVI_LAST_BLKPOS - MOVI_BL1_BLKCNT - MOVI_BL2_BLKCNT - MOVI_ENV_BLKCNT)

#define MOVI_INIT_REQUIRED	0

#define CopyMovitoMem(a,b,c,d,e)	(((int(*)(int, uint, ushort, uint *, int))(*((uint *)(TCM_BASE + 0x8))))(a,b,c,d,e))

void movi_bl2_copy(void)
{
    HM_CONTROL4 = HM_CONTROL4 | (0x3 << 16);
    led_test(0x30);
	CopyMovitoMem(HSMMC_CHANNEL, 0, MOVI_BL2_BLKCNT, (uint *)BL2_BASE, MOVI_INIT_REQUIRED);
}

void sd_boot(void)
{
    __attribute__((noreturn)) void (*uboot)(void);
    led_test(0xc0);
    movi_bl2_copy();
	/*
	 * Jump to U-Boot image
	 */
    led_test(0xf0);
	uboot = (void *)BL2_BASE;
	(*uboot)();
}

void board_init_f(unsigned long bootflag)
{
	
}