# 移植过程中，出现的一些问题记录

## 2020/02/26 - 使用LED调试，发现系统运行不正常

使用LED调试，发现系统运行到system_clock_init（lowlevel_init.S）子过程调用后，无返回

对比友善之臂提供的uboot源码，发现system_clock_init中副本代码由于宏定义，未被执行

（对比代码，还有几处不一致，但目前先调通代码，使得串口交互能够正常工作）

```
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