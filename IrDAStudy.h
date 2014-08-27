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
#define TIMER_IRQ         IRQ_TIMER0  //������irqs.h��,Ӳ���жϺ�10
#define TIMER0_1_Pre      24//��ʱ��0 1��prescalerԤ��Ƶֵ
#define TIMER2_3_4_Pre    24//��ʱ��2 3 4��Ԥ��Ƶֵ
#define DIVIDER_MUX0      0//��Ƶֵ
#define MAX_CODE_WIDTH    50000 // 2us/count,16bit timer
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
	u32 freq;

	u32 *tcfg0;
	u32 *tcfg1;
	u32 *tcon;
	u32 *tcntb;
	u32 *tcmpb;
	u32 *tcnto;//ʵʱ�۲�ֵ
};

struct GPIO_dev
{
	void __iomem *port_base;
	void __iomem *xint_base;
	void __iomem *xint_mask_base;
	void __iomem *xint_pend_base;//���ܲ���Ҫӳ�䣬��irq���������ȫ�����ú���
	
	u32 *gpfcon;
	u32 *intmod;
	u32 *intmsk;
	u32 *extint0;
};
#endif
