#ifndef  __IrDAStudy_H__
#define  __IrDAStudy_H__

#include <asm/irq.h>

#define DEVICE_NAME "IrDAStudy"
#define TIMER_REG_START   0x51000000
#define TIMER_REG_END     0x51000040

#define GPF0_REG_START    0x56000050  //GPF0��ӦEINT0
#define GPF0_REG_END      0x5600005c
#define EXTINT0           0x56000088

#define XINT_STUDY_IRQ    IRQ_EINT0   
#define TIMER0_IRQ        IRQ_TIMER0  //������irqs.h��,Ӳ���жϺ�10
#define TIMER4_IRQ         //Ӳ���жϺ�14
#define TIMER0_1_Pre      49//��ʱ��0 1��prescalerԤ��Ƶֵ
#define DIVIDER_MUX0      0//��Ƶֵ
#define MAX_CODE_WIDTH    5000000 //scales in us
#define CODE_MAX_LEN      256
#define CODE_MIN_LEN      64

#define INT_REG_START     0x4A000000
#define EXT_SPCPND        0x4A000000
#define INTMOD            0x4A000004
#define INTMASK           0x4A000008
#define PRIORITY          0x4A00000C
#define INTPND            0x4A000010
#define INTOFFSET         0x4A000014
#define SUBSRCPND         0x4A000018
#define INTSUBMSK         0x4A00001C

struct Timer_dev
{
	void __iomem *reg_base;
	int irq;
	unsigned int current_cap;
	unsigned int current_cnt;
	unsigned int freq;

	unsigned long *tcfg0;
	unsigned long *tcfg1;
	unsigned long *tcon;
	unsigned long *tcntb;
	unsigned long *tcmpb;
	unsigned long *tcnto;//ʵʱ�۲�ֵ
};

struct GPIO_dev
{
	void __iomem *port_base;
	void __iomem *xint_base;
	void __iomem *xint_mask_base;
	void __iomem *xint_pend_base;//���ܲ���Ҫӳ�䣬��irq���������ȫ�����ú���
	
	unsigned long *gpfcon;
	unsigned long *intmod;
	unsigned long *intmsk;
	unsigned long *extint0;
};
#endif
