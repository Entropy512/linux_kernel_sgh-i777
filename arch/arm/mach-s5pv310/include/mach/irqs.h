/* linux/arch/arm/mach-s5pv310/include/mach/irqs.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - IRQ definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H __FILE__

#include <plat/irqs.h>

/* PPI: Private Peripheral Interrupt */
#define IRQ_PPI(x)		S5P_IRQ(x+16)

#define IRQ_LOCALTIMER		IRQ_PPI(13)

/* SPI is Shared Peripheral Interrupt */
#define IRQ_SPI(x)		S5P_IRQ(x+32)

#ifdef CONFIG_USE_EXT_GIC

#define IRQ_EINT0               IRQ_SPI(16)
#define IRQ_EINT1               IRQ_SPI(17)
#define IRQ_EINT2               IRQ_SPI(18)
#define IRQ_EINT3               IRQ_SPI(19)
#define IRQ_EINT4               IRQ_SPI(20)
#define IRQ_EINT5               IRQ_SPI(21)
#define IRQ_EINT6               IRQ_SPI(22)
#define IRQ_EINT7               IRQ_SPI(23)
#define IRQ_EINT8               IRQ_SPI(24)
#define IRQ_EINT9               IRQ_SPI(25)
#define IRQ_EINT10              IRQ_SPI(26)
#define IRQ_EINT11              IRQ_SPI(27)
#define IRQ_EINT12              IRQ_SPI(28)
#define IRQ_EINT13              IRQ_SPI(29)
#define IRQ_EINT14              IRQ_SPI(30)
#define IRQ_EINT15              IRQ_SPI(31)
#define IRQ_EINT16_31           IRQ_SPI(32)
#define IRQ_MDMA0               IRQ_SPI(33)
#define IRQ_MDMA1               IRQ_SPI(34)
#define IRQ_PDMA0               IRQ_SPI(35)
#define IRQ_PDMA1               IRQ_SPI(36)
#define IRQ_TIMER0_VIC          IRQ_SPI(37)
#define IRQ_TIMER1_VIC          IRQ_SPI(38)
#define IRQ_TIMER2_VIC          IRQ_SPI(39)
#define IRQ_TIMER3_VIC          IRQ_SPI(40)
#define IRQ_TIMER4_VIC          IRQ_SPI(41)
#define IRQ_MCT_L0              IRQ_SPI(42)
#define IRQ_WDT                 IRQ_SPI(43)
#define IRQ_RTC_ALARM           IRQ_SPI(44)
#define IRQ_RTC_TIC             IRQ_SPI(45)
#define IRQ_GPIO_XB		IRQ_SPI(46)
#define IRQ_GPIO_XA		IRQ_SPI(47)
#define IRQ_MCT_L1              IRQ_SPI(48)

#define IRQ_UART0               IRQ_SPI(52)
#define IRQ_UART1               IRQ_SPI(53)
#define IRQ_UART2               IRQ_SPI(54)
#define IRQ_UART3               IRQ_SPI(55)
#define IRQ_UART4               IRQ_SPI(56)
#define IRQ_MCT_G0              IRQ_SPI(57)
#define IRQ_IIC                 IRQ_SPI(58)
#define IRQ_IIC1                IRQ_SPI(59)
#define IRQ_IIC2		IRQ_SPI(60)
#define IRQ_IIC3                IRQ_SPI(61)
#define IRQ_IIC4                IRQ_SPI(62)
#define IRQ_IIC5                IRQ_SPI(63)
#define IRQ_IIC6                IRQ_SPI(64)
#define IRQ_IIC7                IRQ_SPI(65)
#define IRQ_SPI0                IRQ_SPI(66)
#define IRQ_SPI1                IRQ_SPI(67)
#define IRQ_SPI2                IRQ_SPI(68)

#define IRQ_UHOST		IRQ_SPI(70)
#define IRQ_OTG		        IRQ_SPI(71)
#define IRQ_MODEM_IF            IRQ_SPI(72)
#define IRQ_HSMMC0              IRQ_SPI(73)
#define IRQ_HSMMC1              IRQ_SPI(74)
#define IRQ_HSMMC2              IRQ_SPI(75)
#define IRQ_HSMMC3              IRQ_SPI(76)
#define IRQ_MSHC                IRQ_SPI(77)
#define IRQ_MIPICSI0            IRQ_SPI(78)
#define IRQ_MIPIDSI0            IRQ_SPI(79)
#define IRQ_MIPICSI1            IRQ_SPI(80)
#define IRQ_MIPIDSI1            IRQ_SPI(81)

#define IRQ_ONENAND_AUDI        IRQ_SPI(82)
#define IRQ_ROTATOR             IRQ_SPI(83)
#define IRQ_FIMC0               IRQ_SPI(84)
#define IRQ_FIMC1               IRQ_SPI(85)
#define IRQ_FIMC2               IRQ_SPI(86)
#define IRQ_FIMC3               IRQ_SPI(87)
#define IRQ_JPEG                IRQ_SPI(88)
#define IRQ_FIMG2D              IRQ_SPI(89)
#define IRQ_PCIE                IRQ_SPI(90)
#define IRQ_MIXER               IRQ_SPI(91)
#define IRQ_HDMI                IRQ_SPI(92)
#define IRQ_HDMI_I2C            IRQ_SPI(93)
#define IRQ_MFC                 IRQ_SPI(94)
#define IRQ_TVENC               IRQ_SPI(95)
#define IRQ_AUDIO_SS            IRQ_SPI(96)
#define IRQ_I2S0                IRQ_SPI(97)
#define IRQ_I2S1                IRQ_SPI(98)
#define IRQ_I2S2                IRQ_SPI(99)
#define IRQ_AC97                IRQ_SPI(100)

#define IRQ_SPDIF               IRQ_SPI(104)
#define IRQ_ADC0                IRQ_SPI(105)
#define IRQ_PEN0                IRQ_SPI(106)
#define IRQ_ADC1                IRQ_SPI(107)
#define IRQ_PEN1                IRQ_SPI(108)
#define IRQ_KEYPAD              IRQ_SPI(109)
#define IRQ_PMU                 IRQ_SPI(110)
#define IRQ_GPS                 IRQ_SPI(111)
#define IRQ_INTFEEDCTRL_SSS     IRQ_SPI(112)
#define IRQ_SLIMBUS             IRQ_SPI(113)
#define IRQ_CEC                 IRQ_SPI(114)
#define IRQ_TSI                 IRQ_SPI(115)
#define IRQ_SATA                IRQ_SPI(116)

#define IRQ_PPMMU0_3D    	IRQ_SPI(118)
#define IRQ_PPMMU1_3D    	IRQ_SPI(119)
#define IRQ_PPMMU2_3D    	IRQ_SPI(120)
#define IRQ_PPMMU3_3D    	IRQ_SPI(121)
#define IRQ_GPMMU_3D     	IRQ_SPI(122)

#define IRQ_PP0_3D         	IRQ_SPI(123)
#define IRQ_PP1_3D         	IRQ_SPI(124)
#define IRQ_PP2_3D         	IRQ_SPI(125)
#define IRQ_PP3_3D         	IRQ_SPI(126)
#define IRQ_GP_3D          	IRQ_SPI(127)
#define IRQ_PMU_3D        	IRQ_SPI(117)

#define MAX_IRQ_IN_COMBINER     8
#define COMBINER_GROUP(x)       ((x) * MAX_IRQ_IN_COMBINER + IRQ_SPI(128))
#define COMBINER_IRQ(x, y)      (COMBINER_GROUP(x) + y)

#define IRQ_PMU_0               COMBINER_IRQ(2, 2)
#define IRQ_TMU_TRIG0           COMBINER_IRQ(2, 4)

#define IRQ_PMU_1               COMBINER_IRQ(3, 2)
#define IRQ_TMU_TRIG1           COMBINER_IRQ(3, 4)

#define IRQ_SYSMMU_MDMA0_0      COMBINER_IRQ(4, 0)
#define IRQ_SYSMMU_SSS_0        COMBINER_IRQ(4, 1)
#define IRQ_SYSMMU_FIMC0_0      COMBINER_IRQ(4, 2)
#define IRQ_SYSMMU_FIMC1_0      COMBINER_IRQ(4, 3)
#define IRQ_SYSMMU_FIMC2_0      COMBINER_IRQ(4, 4)
#define IRQ_SYSMMU_FIMC3_0      COMBINER_IRQ(4, 5)
#define IRQ_SYSMMU_JPEG_0       COMBINER_IRQ(4, 6)
#define IRQ_SYSMMU_2D_0         COMBINER_IRQ(4, 7)

#define IRQ_SYSMMU_ROTATOR_0    COMBINER_IRQ(5, 0)
#define IRQ_SYSMMU_MDMA1_0      COMBINER_IRQ(5, 1)
#define IRQ_SYSMMU_LCD0_M0_0    COMBINER_IRQ(5, 2)
#define IRQ_SYSMMU_LCD1_M1_0    COMBINER_IRQ(5, 3)
#define IRQ_SYSMMU_TV_M0_0      COMBINER_IRQ(5, 4)
#define IRQ_SYSMMU_MFC_M0_0     COMBINER_IRQ(5, 5)
#define IRQ_SYSMMU_MFC_M1_0     COMBINER_IRQ(5, 6)
#define IRQ_SYSMMU_PCIE_0       COMBINER_IRQ(5, 7)

#define IRQ_SYSMMU_MDMA0_1      COMBINER_IRQ(6, 0)
#define IRQ_SYSMMU_SSS_1        COMBINER_IRQ(6, 1)
#define IRQ_SYSMMU_FIMC0_1      COMBINER_IRQ(6, 2)
#define IRQ_SYSMMU_FIMC1_1      COMBINER_IRQ(6, 3)
#define IRQ_SYSMMU_FIMC2_1      COMBINER_IRQ(6, 4)
#define IRQ_SYSMMU_FIMC3_1      COMBINER_IRQ(6, 5)
#define IRQ_SYSMMU_JPEG_1       COMBINER_IRQ(6, 6)
#define IRQ_SYSMMU_2D_1         COMBINER_IRQ(6, 7)

#define IRQ_SYSMMU_ROTATOR_1    COMBINER_IRQ(7, 0)
#define IRQ_SYSMMU_MDMA1_1      COMBINER_IRQ(7, 1)
#define IRQ_SYSMMU_LCD0_M0_1    COMBINER_IRQ(7, 2)
#define IRQ_SYSMMU_LCD1_M1_1    COMBINER_IRQ(7, 3)
#define IRQ_SYSMMU_TV_M0_1      COMBINER_IRQ(7, 4)
#define IRQ_SYSMMU_MFC_M0_1     COMBINER_IRQ(7, 5)
#define IRQ_SYSMMU_MFC_M1_1     COMBINER_IRQ(7, 6)
#define IRQ_SYSMMU_PCIE_1       COMBINER_IRQ(7, 7)

#define IRQ_LCD0                COMBINER_IRQ(11, 0)
#define IRQ_LCD1                COMBINER_IRQ(11, 1)

/* UART interrupts, each UART has 4 intterupts per channel so
 * use the space between the ISA and S3C main interrupts. Note, these
 * are not in the same order as the S3C24XX series!
 */
#define IRQ_S5P_UART_BASE4      COMBINER_IRQ(MAX_COMBINER_NR,0)

#define UART_IRQ_RXD           (0)
#define UART_IRQ_ERR           (1)
#define UART_IRQ_TXD           (2)

#define IRQ_S5P_UART_RX4       (IRQ_S5P_UART_BASE4 + UART_IRQ_RXD)
#define IRQ_S5P_UART_TX4       (IRQ_S5P_UART_BASE4 + UART_IRQ_TXD)
#define IRQ_S5P_UART_ERR4      (IRQ_S5P_UART_BASE4 + UART_IRQ_ERR)

/* S3C compatibilty defines */
#define IRQ_S3CUART_RX4         IRQ_S5P_UART_RX4

#define IRQ_EINT_BASE           IRQ_S5P_UART_BASE4 + 4

#define EINT_NUMBER(x)          ((x) + IRQ_EINT_BASE)

#define S5P_EINT_BASE1          EINT_NUMBER(0)
#define S5P_EINT_BASE2          EINT_NUMBER(16)

#define IRQ_TVOUT_HPD           EINT_NUMBER(31)

#ifdef CONFIG_SAMSUNG_IRQ_GPIO
/* GPIO interrupts */
#define IRQ_GPIO_BASE           EINT_NUMBER(32)
#define IRQ_GPIO1_NR_GROUPS     16
#define IRQ_GPIO2_NR_GROUPS     9
#define IRQ_GPIO_GROUP(x)       (IRQ_GPIO_BASE + ((x) * 8))
#define IRQ_GPIO_END            IRQ_GPIO_GROUP(IRQ_GPIO1_NR_GROUPS + IRQ_GPIO2_NR_GROUPS)
#else
#define IRQ_GPIO_END            EINT_NUMBER(32)
#endif

#define IRQ_BOARD_START         IRQ_GPIO_END
#define IRQ_NR_BOARD            40

/* Set the default NR_IRQS */
#define NR_IRQS                 (IRQ_GPIO_END + IRQ_NR_BOARD)

#define MAX_COMBINER_NR         16

/* 2010.08.28 by icarus : should be fixed */
#ifdef CONFIG_TOUCH_LCD_A  /* defined(CONFIG_S3C_DEV_ADC1) */
#define	IRQ_ADC		IRQ_ADC1
#define	IRQ_TC		IRQ_PEN1
#else
#define	IRQ_ADC		IRQ_ADC0
#define	IRQ_TC		IRQ_PEN0
#endif

#else /* CONFIG_USE_EXT_INT */

#ifdef CONFIG_CPU_S5PV310_EVT1
#define IRQ_MCT1		IRQ_SPI(35)
#endif

#define IRQ_EINT0		IRQ_SPI(40)
#define IRQ_EINT1		IRQ_SPI(41)
#define IRQ_EINT2		IRQ_SPI(42)
#define IRQ_EINT3		IRQ_SPI(43)
#define IRQ_OTG			IRQ_SPI(44)
#define IRQ_UHOST		IRQ_SPI(45)
#define IRQ_MODEM_IF		IRQ_SPI(46)
#define IRQ_ROTATOR		IRQ_SPI(47)
#define IRQ_JPEG		IRQ_SPI(48)
#define IRQ_FIMG2D		IRQ_SPI(49)
#define IRQ_PCIE		IRQ_SPI(50)

#ifdef CONFIG_CPU_S5PV310_EVT1
#define IRQ_MCT0		IRQ_SPI(51)
#else
#define IRQ_SYSTEM_TIMER	IRQ_SPI(51)
#endif

#define IRQ_MFC			IRQ_SPI(52)

#ifndef CONFIG_CPU_S5PV310_EVT1
#define IRQ_WDT			IRQ_SPI(53)
#endif

#define IRQ_AUDIO_SS		IRQ_SPI(54)
#define IRQ_AC97		IRQ_SPI(55)
#define IRQ_SPDIF		IRQ_SPI(56)
#define IRQ_KEYPAD		IRQ_SPI(57)
#define IRQ_INTFEEDCTRL_SSS	IRQ_SPI(58)
#define IRQ_SLIMBUS		IRQ_SPI(59)
#define IRQ_PMU			IRQ_SPI(60)
#define IRQ_TSI			IRQ_SPI(61)
#define IRQ_SATA		IRQ_SPI(62)
#define IRQ_GPS			IRQ_SPI(63)

#define MAX_IRQ_IN_COMBINER	8
#define COMBINER_GROUP(x)	((x) * MAX_IRQ_IN_COMBINER + IRQ_SPI(64))
#define COMBINER_IRQ(x, y)	(COMBINER_GROUP(x) + y)

#define IRQ_PMU_0               COMBINER_IRQ(2, 2)
#define IRQ_TMU_TRIG0		COMBINER_IRQ(2, 4)

#define IRQ_PMU_1               COMBINER_IRQ(3, 2)
#define IRQ_TMU_TRIG1		COMBINER_IRQ(3, 4)

#define IRQ_SYSMMU_MDMA0_0	COMBINER_IRQ(4, 0)
#define IRQ_SYSMMU_SSS_0	COMBINER_IRQ(4, 1)
#define IRQ_SYSMMU_FIMC0_0	COMBINER_IRQ(4, 2)
#define IRQ_SYSMMU_FIMC1_0	COMBINER_IRQ(4, 3)
#define IRQ_SYSMMU_FIMC2_0	COMBINER_IRQ(4, 4)
#define IRQ_SYSMMU_FIMC3_0	COMBINER_IRQ(4, 5)
#define IRQ_SYSMMU_JPEG_0	COMBINER_IRQ(4, 6)
#define IRQ_SYSMMU_2D_0		COMBINER_IRQ(4, 7)

#define IRQ_SYSMMU_ROTATOR_0	COMBINER_IRQ(5, 0)
#define IRQ_SYSMMU_MDMA1_0	COMBINER_IRQ(5, 1)
#define IRQ_SYSMMU_LCD0_M0_0	COMBINER_IRQ(5, 2)
#define IRQ_SYSMMU_LCD1_M1_0	COMBINER_IRQ(5, 3)
#define IRQ_SYSMMU_TV_M0_0	COMBINER_IRQ(5, 4)
#define IRQ_SYSMMU_MFC_M0_0	COMBINER_IRQ(5, 5)
#define IRQ_SYSMMU_MFC_M1_0	COMBINER_IRQ(5, 6)
#define IRQ_SYSMMU_PCIE_0	COMBINER_IRQ(5, 7)

#define IRQ_SYSMMU_MDMA0_1	COMBINER_IRQ(6, 0)
#define IRQ_SYSMMU_SSS_1	COMBINER_IRQ(6, 1)
#define IRQ_SYSMMU_FIMC0_1	COMBINER_IRQ(6, 2)
#define IRQ_SYSMMU_FIMC1_1	COMBINER_IRQ(6, 3)
#define IRQ_SYSMMU_FIMC2_1	COMBINER_IRQ(6, 4)
#define IRQ_SYSMMU_FIMC3_1	COMBINER_IRQ(6, 5)
#define IRQ_SYSMMU_JPEG_1	COMBINER_IRQ(6, 6)
#define IRQ_SYSMMU_2D_1		COMBINER_IRQ(6, 7)

#define IRQ_SYSMMU_ROTATOR_1	COMBINER_IRQ(7, 0)
#define IRQ_SYSMMU_MDMA1_1	COMBINER_IRQ(7, 1)
#define IRQ_SYSMMU_LCD0_M0_1	COMBINER_IRQ(7, 2)
#define IRQ_SYSMMU_LCD1_M1_1	COMBINER_IRQ(7, 3)
#define IRQ_SYSMMU_TV_M0_1	COMBINER_IRQ(7, 4)
#define IRQ_SYSMMU_MFC_M0_1	COMBINER_IRQ(7, 5)
#define IRQ_SYSMMU_MFC_M1_1	COMBINER_IRQ(7, 6)
#define IRQ_SYSMMU_PCIE_1	COMBINER_IRQ(7, 7)

#define IRQ_LCD0		COMBINER_IRQ(11, 0)
#define IRQ_LCD1		COMBINER_IRQ(11, 1)

#define IRQ_PPMMU0_3D    	COMBINER_IRQ(13, 0)
#define IRQ_PPMMU1_3D    	COMBINER_IRQ(13, 1)
#define IRQ_PPMMU2_3D    	COMBINER_IRQ(13, 2)
#define IRQ_PPMMU3_3D    	COMBINER_IRQ(13, 3)
#define IRQ_GPMMU_3D     	COMBINER_IRQ(13, 4)

#define IRQ_PP0_3D         	COMBINER_IRQ(14, 0)
#define IRQ_PP1_3D         	COMBINER_IRQ(14, 1)
#define IRQ_PP2_3D         	COMBINER_IRQ(14, 2)
#define IRQ_PP3_3D         	COMBINER_IRQ(14, 3)
#define IRQ_GP_3D          	COMBINER_IRQ(14, 4)
#define IRQ_PMU_3D         	COMBINER_IRQ(14, 5)

#define IRQ_HDMI		COMBINER_IRQ(16, 0)
#define IRQ_HDMI_I2C		COMBINER_IRQ(16, 1)
#define IRQ_CEC			COMBINER_IRQ(16, 2)

#define IRQ_I2S0		COMBINER_IRQ(17, 0)
#define IRQ_I2S1		COMBINER_IRQ(17, 1)
#define IRQ_I2S2		COMBINER_IRQ(17, 2)

#define IRQ_PCM0		COMBINER_IRQ(18, 0)
#define IRQ_PCM1		COMBINER_IRQ(18, 1)
#define IRQ_PCM2		COMBINER_IRQ(18, 2)

#define IRQ_ADC0		COMBINER_IRQ(19, 0)
#define IRQ_PEN0		COMBINER_IRQ(19, 1)
#define IRQ_ADC1		COMBINER_IRQ(19, 2)
#define IRQ_PEN1		COMBINER_IRQ(19, 3)

#define IRQ_MDMA0		COMBINER_IRQ(20, 0)
#define IRQ_MDMA1		COMBINER_IRQ(20, 1)

#define IRQ_PDMA0		COMBINER_IRQ(21, 0)
#define IRQ_PDMA1		COMBINER_IRQ(21, 1)

#define IRQ_TIMER0_VIC		COMBINER_IRQ(22, 0)
#define IRQ_TIMER1_VIC		COMBINER_IRQ(22, 1)
#define IRQ_TIMER2_VIC		COMBINER_IRQ(22, 2)
#define IRQ_TIMER3_VIC		COMBINER_IRQ(22, 3)
#define IRQ_TIMER4_VIC		COMBINER_IRQ(22, 4)

#define IRQ_RTC_ALARM		COMBINER_IRQ(23, 0)
#define IRQ_RTC_TIC		COMBINER_IRQ(23, 1)

#define IRQ_GPIO_XB		COMBINER_IRQ(24, 0)
#define IRQ_GPIO_XA		COMBINER_IRQ(24, 1)

#define IRQ_UART0		COMBINER_IRQ(26, 0)
#define IRQ_UART1		COMBINER_IRQ(26, 1)
#define IRQ_UART2		COMBINER_IRQ(26, 2)
#define IRQ_UART3		COMBINER_IRQ(26, 3)
#define IRQ_UART4		COMBINER_IRQ(26, 4)

#define IRQ_IIC			COMBINER_IRQ(27, 0)
#define IRQ_IIC1		COMBINER_IRQ(27, 1)
#define IRQ_IIC2		COMBINER_IRQ(27, 2)
#define IRQ_IIC3		COMBINER_IRQ(27, 3)
#define IRQ_IIC4		COMBINER_IRQ(27, 4)
#define IRQ_IIC5		COMBINER_IRQ(27, 5)
#define IRQ_IIC6		COMBINER_IRQ(27, 6)
#define IRQ_IIC7		COMBINER_IRQ(27, 7)

#define IRQ_MSHC                COMBINER_IRQ(29,4)
#define IRQ_SPI0		COMBINER_IRQ(28, 0)
#define IRQ_SPI1		COMBINER_IRQ(28, 1)
#define IRQ_SPI2		COMBINER_IRQ(28, 2)

#define IRQ_HSMMC0		COMBINER_IRQ(29, 0)
#define IRQ_HSMMC1		COMBINER_IRQ(29, 1)
#define IRQ_HSMMC2		COMBINER_IRQ(29, 2)
#define IRQ_HSMMC3		COMBINER_IRQ(29, 3)

#define IRQ_MIPICSI0		COMBINER_IRQ(30, 0)
#define IRQ_MIPICSI1		COMBINER_IRQ(30, 1)

#define IRQ_MIPIDSI0		COMBINER_IRQ(31, 0)
#define IRQ_MIPIDSI1		COMBINER_IRQ(31, 1)

#define IRQ_FIMC0		COMBINER_IRQ(32, 0)
#define IRQ_FIMC1		COMBINER_IRQ(32, 1)
#define IRQ_FIMC2		COMBINER_IRQ(33, 0)
#define IRQ_FIMC3		COMBINER_IRQ(33, 1)

#define IRQ_ONENAND_AUDI	COMBINER_IRQ(34, 0)

#ifdef CONFIG_CPU_S5PV310_EVT1
#define IRQ_MCT_L1		COMBINER_IRQ(35, 3)
#endif

#define IRQ_MIXER		COMBINER_IRQ(36, 0)
#define IRQ_TVENC		COMBINER_IRQ(36, 1)

#define IRQ_EINT15		COMBINER_IRQ(38,7)
#define IRQ_EINT14		COMBINER_IRQ(38,6)
#define IRQ_EINT13		COMBINER_IRQ(38,5)
#define IRQ_EINT12		COMBINER_IRQ(38,4)
#define IRQ_EINT11		COMBINER_IRQ(38,3)
#define IRQ_EINT10		COMBINER_IRQ(38,2)
#define IRQ_EINT9		COMBINER_IRQ(38,1)
#define IRQ_EINT8		COMBINER_IRQ(38,0)

#define IRQ_EINT7		COMBINER_IRQ(37,3)
#define IRQ_EINT6		COMBINER_IRQ(37,2)
#define IRQ_EINT5		COMBINER_IRQ(37,1)
#define IRQ_EINT4		COMBINER_IRQ(37,0)

#define IRQ_EINT16_31		COMBINER_IRQ(39,0)

#ifdef CONFIG_CPU_S5PV310_EVT1
#define IRQ_MCT_L0		COMBINER_IRQ(51, 0)

#define IRQ_WDT			COMBINER_IRQ(53, 0)
#endif

/* UART interrupts, each UART has 4 intterupts per channel so
 *  * use the space between the ISA and S3C main interrupts. Note, these
 *   * are not in the same order as the S3C24XX series! */
#define IRQ_S5P_UART_BASE4	COMBINER_IRQ(MAX_COMBINER_NR,0)

#define UART_IRQ_RXD           (0)
#define UART_IRQ_ERR           (1)
#define UART_IRQ_TXD           (2)

#define IRQ_S5P_UART_RX4       (IRQ_S5P_UART_BASE4 + UART_IRQ_RXD)
#define IRQ_S5P_UART_TX4       (IRQ_S5P_UART_BASE4 + UART_IRQ_TXD)
#define IRQ_S5P_UART_ERR4      (IRQ_S5P_UART_BASE4 + UART_IRQ_ERR)

/* S3C compatibilty defines */
#define IRQ_S3CUART_RX4                IRQ_S5P_UART_RX4

#define IRQ_EINT_BASE		IRQ_S5P_UART_BASE4 + 4

#define EINT_NUMBER(x)		((x) + IRQ_EINT_BASE)

#define S5P_EINT_BASE1		EINT_NUMBER(0)
#define S5P_EINT_BASE2		EINT_NUMBER(16)

#define IRQ_TVOUT_HPD           EINT_NUMBER(31)

#ifdef CONFIG_SAMSUNG_IRQ_GPIO
/* GPIO interrupts */
#define IRQ_GPIO_BASE           EINT_NUMBER(32)
#define IRQ_GPIO1_NR_GROUPS     16
#define IRQ_GPIO2_NR_GROUPS     9
#define IRQ_GPIO_GROUP(x)       (IRQ_GPIO_BASE + ((x) * 8))
#define IRQ_GPIO_END            IRQ_GPIO_GROUP(IRQ_GPIO1_NR_GROUPS + IRQ_GPIO2_NR_GROUPS)
#else
#define IRQ_GPIO_END            EINT_NUMBER(32)
#endif

#define IRQ_BOARD_START         IRQ_GPIO_END
#define IRQ_NR_BOARD            40

/* Set the default NR_IRQS */
#define NR_IRQS                 (IRQ_GPIO_END + IRQ_NR_BOARD)

#ifdef CONFIG_CPU_S5PV310_EVT1
#define MAX_COMBINER_NR		54
#else
#define MAX_COMBINER_NR		40
#endif

#define MAX_EXTCOMBINER_NR	32

/* 2010.08.28 by icarus : should be fixed */
#ifdef CONFIG_TOUCH_LCD_A  /* defined(CONFIG_S3C_DEV_ADC1) */
#define	IRQ_ADC		IRQ_ADC1
#define	IRQ_TC		IRQ_PEN1
#else
#define	IRQ_ADC		IRQ_ADC0
#define	IRQ_TC		IRQ_PEN0
#endif

#define MAX_IRQ_IN_COMBINER	8
#define COMBINER_GROUP(x)	((x) * MAX_IRQ_IN_COMBINER + IRQ_SPI(64))
#define COMBINER_IRQ(x, y)	(COMBINER_GROUP(x) + y)

#endif /* CONFIG_USE_EXT_INT */

#endif /* __ASM_ARCH_IRQS_H */
