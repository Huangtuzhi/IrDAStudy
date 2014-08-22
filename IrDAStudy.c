#include <asm/io.h>
#include <mach/irqs.h>
//#include <asm/irq.h>
#include <asm/uaccess.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <mach/regs-clock.h>
#include <plat/regs-timer.h>
#include "IrDAStudy.h"

#define DEBUG
#ifdef  DEBUG
#define DPRINTK(x...) {printk(__FUNCTION__"(%d): ",__LINE__);printk(##x);}
#else
#define DPRINTK(x...) (void)(0)
#endif

static DECLARE_WAIT_QUEUE_HEAD(IrDA_Study_Queue);
struct Timer_dev Study_Timer;
struct GPIO_dev Study_GPIO;

static void timer_setup(struct Timer_dev *dev,unsigned int us);//设置定时器
static void GPIO_setup(struct GPIO_dev *dev);//设置外部中断触发
static void timer_on(struct Timer_dev *dev);
static void timer_off(struct Timer_dev *dev);
static unsigned int read_timer_cnt(struct Timer_dev *dev);//读取timer计数值

static irqreturn_t gpio_study_irq_handler(int irq, void *wbuf);//GPIO外部触发学习中断
static irqreturn_t timer_irq_handler(int irq, void *dev_id);//定时器超时中断

unsigned int IrDA_cnt;

//timer设置有问题
static void timer_setup(struct Timer_dev *dev,unsigned int us)
{
	unsigned int temp;
	dev->tcfg0=(volatile unsigned long*)(dev->reg_base+0x00);
	dev->tcfg1=(volatile unsigned long*)(dev->reg_base+0x04);
	dev->tcon =(volatile unsigned long*)(dev->reg_base+0x08);
	dev->tcntb=(volatile unsigned long*)(dev->reg_base+0x0c);
	dev->tcmpb=(volatile unsigned long*)(dev->reg_base+0x10);
	dev->tcnto=(volatile unsigned long*)(dev->reg_base+0x14);
  

	temp=ioread32(dev->tcfg0);
	iowrite32((temp&0xFFFFFF00)|((TIMER0_1_Pre&0xFF)<<0),dev->tcfg0);//设置定时器0 1的预分频值

	temp=ioread32(dev->tcfg1);
	iowrite32((temp&0xFFFFFFF0)|((DIVIDER_MUX0&0x00)<<0),dev->tcfg1);//设置分频值
	dev->freq=50000000/(TIMER0_1_Pre+1);
	printk("timer freq is %d",dev->freq);
	
	temp=ioread32(dev->tcon);
	iowrite32((temp&0xFFFFFFF0)|0x0c,dev->tcon);//设置自动加载模式
	iowrite32(us,dev->tcntb);//写计数器装载值
	dev->current_cnt=ioread32(dev->tcntb);//读取当前记数值
	iowrite32(0,dev->tcmpb); // 初始化计数器比较值，减法计数
	dev->current_cap=ioread32(dev->tcmpb);//读取比较值

	temp=ioread32(dev->tcon);
	iowrite32(temp|0x02,dev->tcon);//更新状态寄存器
	temp=ioread32(dev->tcon);
	iowrite32(temp&(~0x02),dev->tcon);//更新完寄存器关闭
}


static void timer_on(struct Timer_dev *dev)
{
	unsigned int temp;
	temp=ioread32(dev->tcon);
	iowrite32(temp|0x01,dev->tcon);//开始计数
}

static void timer_off(struct Timer_dev *dev)
{
	unsigned int temp;
	temp=ioread32(dev->tcon);
	iowrite32(temp&(~0x01),dev->tcon);//停止计数
}

static unsigned int read_timer_cnt(struct Timer_dev *dev)
{
	return ioread32(dev->tcnto);//读取拷贝的观测值
}

static void GPIO_setup(struct GPIO_dev *dev)
{   //GPIO的中断触发方式在软件里设置好了
    unsigned int temp;
	dev->gpfcon=(volatile unsigned long*)(dev->port_base+0x00);//这里映射关系没有对应过来导致oops
	dev->intmod=(volatile unsigned long*)(dev->xint_base+0x04);
	dev->intmsk=(volatile unsigned long*)(dev->xint_base+0x08);
	dev->extint0=(volatile unsigned long*)(dev->port_base+0x38);

	temp=ioread32(dev->gpfcon);
	iowrite32((temp&(~0x02))|(0x02),dev->gpfcon);//配置GPF0为EINT0模式
	temp=ioread32(dev->extint0);
	iowrite32((temp&(~0x06))|(0x06),dev->extint0);//配置GPIO口的EINT0为上下降沿触发模式
	
// 这里的FIQ模式不能这么设置，会导致开发板死机
//	temp=ioread32(dev->intmod);
//	iowrite32((temp&(~(0x3)))|(0x3),dev->intmod);//设置为FIQ模式
	temp=ioread32(dev->intmsk);
	iowrite32(temp&(~(0x01)),dev->intmsk);//配置GPF0为EINT0模式
}

static irqreturn_t gpio_study_irq_handler(int irq, void *wbuf)
{
	unsigned int *buf=(unsigned int *)wbuf;
	if(0==IrDA_cnt){
		timer_setup(&Study_Timer,MAX_CODE_WIDTH);
		timer_on(&Study_Timer);
		IrDA_cnt++;
		return IRQ_RETVAL(IRQ_HANDLED);
	}
	timer_off(&Study_Timer);//这句应该在if之后，前面还没初始化结构体，不能关闭。
	*(buf+IrDA_cnt-1)=MAX_CODE_WIDTH-read_timer_cnt(&Study_Timer);
	iowrite32(MAX_CODE_WIDTH,Study_Timer.tcntb);//重新填充计数值
	timer_on(&Study_Timer);
	IrDA_cnt++;
	if(IrDA_cnt > CODE_MAX_LEN)
	{//学习完成
		wake_up_interruptible(&IrDA_Study_Queue);//注意是队列
	}	
	return IRQ_RETVAL(IRQ_HANDLED);
}


static irqreturn_t timer_irq_handler(int irq, void *dev_id)
{  
	printk("The last code is studied and timeout,having studied %d\n",IrDA_cnt);
    /*
	if(IrDA_cnt<CODE_MIN_LEN){
		IrDA_cnt=0;//当学习到的长度不满足要求时
	}
	*/
	wake_up_interruptible(&IrDA_Study_Queue);
	timer_off(&Study_Timer);
	return IRQ_RETVAL(IRQ_HANDLED);
}


static ssize_t IrDA_read(struct file *filp, char __user *buffer, size_t count, loff_t *ppos)
{
	unsigned int wbuf[CODE_MAX_LEN];
	unsigned temp;
	int timeout;
	GPIO_setup(&Study_GPIO);//初始化GPIO口和EINT0,完全可以不要，下面的申请完成了全部的GPIO设置
	temp=request_irq(XINT_STUDY_IRQ,gpio_study_irq_handler,IRQF_DISABLED|IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING,DEVICE_NAME,wbuf);//DISABLED表示关闭中断，不接受其他中断。
	if(temp<0)
	{
		printk("request for xint irq fail, error id %d\n",temp);
		return -1;
	}

	temp=request_irq(TIMER0_IRQ,timer_irq_handler,IRQF_DISABLED,DEVICE_NAME,NULL);
	if(temp<0)
	{
		printk("request for timer irq fail, error id %d\n",temp);
		return -1;

	}
	printk("Press remote button to study\n");
	IrDA_cnt=0;
	timeout=interruptible_sleep_on_timeout(&IrDA_Study_Queue,2000);//当前进程进入睡眠，加入IrDA_Study队列，可以被wake_up唤醒|| 等待6S之后执行以下程序
//wake_up在下面两种情况中被调用：学习的长度达到阈值 | 学习过程中定时器到达设定阈值，说明按键结束，没有信号触发来重设装载值
	printk("sleep time:%d\n",timeout);
	temp=copy_to_user(buffer,wbuf,(IrDA_cnt ? 4*(IrDA_cnt-1):0) );//wbuf---->用户空间buffer
	if(timeout==0) 
	{
		printk("No IrDA signal,timeout error\n");//没有按键学习，等待6s后返回。
		return 0;
	}
	timer_off(&Study_Timer);
	free_irq(XINT_STUDY_IRQ,wbuf);
	free_irq(TIMER0_IRQ,NULL);
	return IrDA_cnt-1;
}

static int IrDA_open(struct inode *inode, struct file *filp)
{
	printk("-------------------------------\n");
	printk("IrDA Device opened\n");
	return 0;
}

static int  IrDA_release(struct inode *inode, struct file *filp)
{
	printk("IrDA code study finish, Device close\n");
	return 0;
}
	
static struct file_operations dev_fops={
	.owner=THIS_MODULE,
	.open=IrDA_open,
	.read=IrDA_read,
	.release=IrDA_release
};

static struct miscdevice IrDA_misc={
	.minor=MISC_DYNAMIC_MINOR,
	.name =DEVICE_NAME,
	.fops=&dev_fops
};

static int __init dev_init(void)
{
	int ret;
	
	//映射物理地址到操作系统地址空间
	Study_Timer.reg_base=ioremap(TIMER_REG_START,0x40);
	if(Study_Timer.reg_base == NULL){
		printk(KERN_ERR "Failed to remap remap register block of timer \n");
		return -ENOMEM;
	}

	Study_GPIO.xint_base=ioremap(INT_REG_START,0x1c);//逗号全角 半角 注意
	if(Study_GPIO.xint_base == NULL){
		printk(KERN_ERR "Failed to remap remap register block of GPIO XINT\n");
		return -ENOMEM;
	}

	Study_GPIO.port_base=ioremap(GPF0_REG_START,0x40);
	if(Study_GPIO.port_base == NULL){
		printk(KERN_ERR "Failed to remap remap register block of GPIO Port \n");
		return -ENOMEM;
	}
	
	ret = misc_register(&IrDA_misc);
	printk(DEVICE_NAME "\t initialized\n");
	return ret;
}


static void __exit dev_exit(void)
{
	printk("To __exit dev_exoit\n");
	iounmap(Study_Timer.reg_base);
	iounmap(Study_GPIO.xint_base);
	iounmap(Study_GPIO.port_base);
    misc_deregister(&IrDA_misc);
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huang Yi");
