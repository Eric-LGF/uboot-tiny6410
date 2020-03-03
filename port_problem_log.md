# 移植过程中，出现的一些问题记录

## 2020/03/01 - 解决raise: Signal # 8 caught问题

### 问题描述：

上个版本临时解决了raise: Signal # 8 caught问题，但是这种方法会使得但autoboot倒计时的速度会加快，这个版本将解决这个问题

### 问题解决：

该问题解决参考了[网址](https://blog.csdn.net/zhaocj/article/details/6667758),同时对比了本工程文件(arch/arm/cpu/arm920t/s3c24x0/timer.c)

本次主要在arch/arm/cpu/arm176/s3c64xx/timer.c进行了一些变量替换,如下：

- 添加全局数据变量定义DECLARE_GLOBAL_DATA_PTR
- timer_load_val替换为gd-tbu
- lastdec替换为gd->lastinc
- timestamp替换为gd->tbl


## 2020/02/29 - raise: Signal # 8 caught问题

### 问题描述:

系统运行过程中会不断的打印“raise: Signal # 8 caught”

系统日志：
```
U-Boot 2013.01-00008-ga1e7b04d3d-dirty (Feb 29 2020 - 18:53:03) for TINY6410


CPU:     S3C6400@532MHz
         Fclk = 532MHz, Hclk = 133MHz, Pclk = 66MHz (ASYNC Mode) 
Board:   TINY6400
DRAM:  128 MiB
WARNING: Caches not enabled
Flash: 0 Bytes
NAND:  raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
256 MiB
*** Warning - bad CRC, using default environment

In:    serial
Out:   serial
Err:   serial
Net:   CS8900-0
Hit any key to stop autoboot:  0 

NAND read: device 0 offset 0x60000, size 0x1c0000
raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
raise: Signal # 8 caught
```

### 问题解决

参考这个[网址](https://www.cnblogs.com/lucky-tom/p/3527299.html)，可暂时解决这个问题

但autoboot倒计时的速度会加快，这里先将倒计时更改为30
（修改include/configs/tiny6410.h中的宏CONFIG_BOOTDELAY）


## 2020/02/29 - flash初始化失败

### 问题描述：

系统日志：
```
U-Boot 2013.01-00007-g3250eff2a0-dirty (Feb 28 2020 - 15:19:35) for TINY6410

U-Boot code: 50000000 -> 5003DB08  BSS: -> 50082164

CPU:     S3C6400@532MHz
         Fclk = 532MHz, Hclk = 133MHz, Pclk = 66MHz (ASYNC Mode) 
Board:   TINY6400
monitor len: 00082164
ramsize: 08000000
TLB table from 57ff0000 to 57ff4000
Top of RAM usable for U-Boot at: 57ff0000
Reserving 520k for U-Boot at: 57f6d000
Reserving 1056k for malloc() at: 57e65000
Reserving 32 Bytes for Board Info at: 57e64fe0
Reserving 128 Bytes for Global Data at: 57e64f60
New Stack Pointer is: 57e64f50
RAM Configuration:
Bank #0: 50000000 128 MiB
relocation Offset is: 07f6d000
WARNING: Caches not enabled
monitor flash len: 00044648
Now running in RAM - U-Boot at: 57f6d000
Flash: fwc addr 10000000 cmd f0 00f0 16bit x 16 bit
fwc addr 1000aaaa cmd aa 00aa 16bit x 16 bit
fwc addr 10005554 cmd 55 0055 16bit x 16 bit
fwc addr 1000aaaa cmd 90 0090 16bit x 16 bit
fwc addr 10000000 cmd f0 00f0 16bit x 16 bit
JEDEC PROBE: ID 0 0 0
fwc addr 10000000 cmd ff 00ff 16bit x 16 bit
fwc addr 10000000 cmd 90 0090 16bit x 16 bit
fwc addr 10000000 cmd ff 00ff 16bit x 16 bit
JEDEC PROBE: ID 0 0 0
*** failed ***
### ERROR ### Please RESET the board ###
```

### 问题定位

查看代码，在flash_init()调用中返回flash_size=0，导致后续的代码判断进行系统报错与挂起。

### 问题解决

根据代码可以定义宏CONFIG_SYS_NO_FLASH，去除norflash的相关代码，但由于uboot其他地方牵扯到该部分，造成报错，因此目前先保留norflash代码，并在board_init_r函数中修改flash_init()返回值判断，使得flash_size等于0也不进行报错。
```c
flash_size = flash_init();
	if (flash_size >= 0) {			/* flash_size等于0时，也认为系统是正常的 */
# ifdef CONFIG_SYS_FLASH_CHECKSUM
		print_size(flash_size, "");
		/*
		 * Compute and print flash CRC if flashchecksum is set to 'y'
		 *
		 * NOTE: Maybe we should add some WATCHDOG_RESET()? XXX
		 */
		if (getenv_yesno("flashchecksum") == 1) {
			printf("  CRC: %08X", crc32(0,
				(const unsigned char *) CONFIG_SYS_FLASH_BASE,
				flash_size));
		}
		putc('\n');
# else	/* !CONFIG_SYS_FLASH_CHECKSUM */
		print_size(flash_size, "\n");
# endif /* CONFIG_SYS_FLASH_CHECKSUM */
	} else {
		puts(failed);
		hang();
	}
```


## 2020/02/28 - 串口不输出问题

问题描述：串口通信会一直卡在串口发送完成标志判断的while循环中

```c
static void s3c64xx_serial_putc(const char c)
{
	s3c64xx_uart *const uart = s3c64xx_get_base_uart(UART_NR);

#ifdef CONFIG_MODEM_SUPPORT
	if (be_quiet)
		return;
#endif

	/* wait for room in the tx FIFO */
	while (!(uart->UTRSTAT & 0x2));		/* 发送完成标志一直无法置位 */

	uart->UTXH = c;

	/* If \n, also do \r */
	if (c == '\n')
		serial_putc('\r');
}
```

问题原因：
在S3C6410数据手册中，关于串口波特率时钟描述有如下一段，

- 当串口波特率时钟从EXT_UCLK0切换到PCLK时，时钟设置需设置为2‘b00
- 当串口波特率时钟从EXT_UCLK1切换到PCLK时，时钟设置需设置为2‘b10

```
 When you want to change EXT_UCLK0 to PCLK for UART baudrate , clock selection field must be set to 2’b00. 
 But, when you want to change EXT_UCLK1 to PCLK for UART baudrate , clock selection field must be set to 2’b10.
```
因为之前的uboot所使用的时钟为EXT_UCLK1,因此在本uboot中对UCON寄存器的设置，时钟选择需设置为2’b10

问题解决：

对串口初始化代码进行如下修改：
```c
static int s3c64xx_serial_init(void)
{
	s3c64xx_uart *const uart = s3c64xx_get_base_uart(UART_NR);

	/* reset and enable FIFOs, set triggers to the maximum */
	uart->UFCON = 0xff;
	uart->UMCON = 0;
	/* 8N1 */
	uart->ULCON = 3;
	/* No interrupts, no DMA, pure polling */
	if (uart->UCON & 0xC00 == 0xC00)	/*判断当前是否为EXT_UCLK1*/
		uart->UCON = (2<<10) | 0x5;
	else
		uart->UCON = 5;

	serial_setbrg();

	return 0;
}
```

## 2020/02/26 - 使用LED调试，发现系统运行不正常

使用LED调试，发现系统运行到system_clock_init（lowlevel_init.S）子过程调用后，无返回

对比友善之臂提供的uboot源码，发现system_clock_init中副本代码由于宏定义，未被执行

（对比代码，还有几处不一致，但目前先调通代码，使得串口交互能够正常工作）

```c
system_clock_init:

    /*... 此处代码省略 */

	/*
	 * This was unconditional in original Samsung sources, but it doesn't
	 * seem to make much sense on S3C6400.
	 */
     
#ifndef CONFIG_S3C6400  /* 此行删除 */    
	ldr	r1, [r0, #OTHERS_OFFSET]
	bic	r1, r1, #0xC0
	orr	r1, r1, #0x40
	str	r1, [r0, #OTHERS_OFFSET]

wait_for_async:
	ldr	r1, [r0, #OTHERS_OFFSET]
	and	r1, r1, #0xf00
	cmp	r1, #0x0
	bne	wait_for_async
#endif  /* 此行删除 */

/*  工程移植中为了减少工作，并未重新定义S3C6410，
 *  tiny6410.h文件定义的宏仍然是CONFIG_S3C6400，
 *  因此一下代码无法得到执行，需删掉此处的宏定义条件 
 */

    /*... 此处代码省略 */

```