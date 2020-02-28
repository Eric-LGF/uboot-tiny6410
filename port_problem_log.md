# 移植过程中，出现的一些问题记录

## 2020/02/28 - 串口不输出问题

问题描述：串口通信会一直卡在串口发送完成标志判断的while循环中

```
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