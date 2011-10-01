/* linux/arch/arm/mach-s5pv210/include/mach/regs-hdmi.h
 *
 * Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/
 *
 * Hdmi register header file for Samsung TVOut driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_REGS_HDMI_H

#include <mach/map.h>

#define S5P_HDMI_I2C_PHY_BASE(x) 	(x)

#define HDMI_I2C_CON				S5P_HDMI_I2C_PHY_BASE(0x0000)
#define HDMI_I2C_STAT				S5P_HDMI_I2C_PHY_BASE(0x0004)
#define HDMI_I2C_ADD				S5P_HDMI_I2C_PHY_BASE(0x0008)
#define HDMI_I2C_DS				S5P_HDMI_I2C_PHY_BASE(0x000c)
#define HDMI_I2C_LC				S5P_HDMI_I2C_PHY_BASE(0x0010)

/*
 * Register part
*/
#define S5P_HDMI_CTRL_BASE(x) 			(x)
#define S5P_HDMI_BASE(x) 			((x) + 0x00010000)
#define S5P_HDMI_SPDIF_BASE(x) 			((x) + 0x00030000)
#define S5P_HDMI_I2S_BASE(x) 			((x) + 0x00040000)
#define S5P_HDMI_TG_BASE(x) 			((x) + 0x00050000)
#define S5P_HDMI_EFUSE_BASE(x)			((x) + 0x00060000)


#define S5P_HDMI_INTC_CON			S5P_HDMI_CTRL_BASE(0x0000)	/* Interrupt Control Register */
#define S5P_HDMI_INTC_FLAG			S5P_HDMI_CTRL_BASE(0x0004)	/* Interrupt Flag Register */
#define S5P_HDMI_HDCP_KEY_LOAD			S5P_HDMI_CTRL_BASE(0x0008)	/* HDCP KEY Status */
#define S5P_HDMI_HPD_STATUS			S5P_HDMI_CTRL_BASE(0x000C)	/* HPD signal */
#define S5P_HDMI_AUDIO_CLKSEL			S5P_HDMI_CTRL_BASE(0x0010)	/* Audio system clock selection */
#define S5P_HDMI_PHY_RSTOUT			S5P_HDMI_CTRL_BASE(0x0014)	/* HDMI PHY Reset Out */
#define S5P_HDMI_PHY_VPLL			S5P_HDMI_CTRL_BASE(0x0018)	/* HDMI PHY VPLL Monitor */
#define S5P_HDMI_PHY_CMU			S5P_HDMI_CTRL_BASE(0x001C)	/* HDMI PHY CMU Monitor */
#define S5P_HDMI_CORE_RSTOUT			S5P_HDMI_CTRL_BASE(0x0020)	/* HDMI TX Core S/W reset */
			
#define S5P_HDMI_CON_0				S5P_HDMI_BASE(0x0000)		/* HDMI System Control Register 0 0x00 */
#define S5P_HDMI_CON_1				S5P_HDMI_BASE(0x0004)		/* HDMI System Control Register 1 0x00 */
#define S5P_HDMI_CON_2				S5P_HDMI_BASE(0x0008)		/* HDMI System Control Register 2. 0x00 */
#define S5P_HDMI_SYS_STATUS			S5P_HDMI_BASE(0x0010)		/* HDMI System Status Register 0x00 */
#define S5P_HDMI_PHY_STATUS			S5P_HDMI_BASE(0x0014)		/* HDMI Phy Status */
#define S5P_HDMI_STATUS_EN			S5P_HDMI_BASE(0x0020)		/* HDMI System Status Enable Register 0x00 */
#define S5P_HDMI_HPD				S5P_HDMI_BASE(0x0030)		/* Hot Plug Detection Control Register 0x00 */
#define S5P_HDMI_MODE_SEL			S5P_HDMI_BASE(0x0040)		/* HDMI/DVI Mode Selection 0x00 */
#define S5P_HDMI_ENC_EN				S5P_HDMI_BASE(0x0044)		/* HDCP Encryption Enable Register 0x00 */
			
#define S5P_HDMI_BLUE_SCREEN_0			S5P_HDMI_BASE(0x0050)		/* Pixel Values for Blue Screen 0x00 */
#define S5P_HDMI_BLUE_SCREEN_1			S5P_HDMI_BASE(0x0054)		/* Pixel Values for Blue Screen 0x00 */
#define S5P_HDMI_BLUE_SCREEN_2			S5P_HDMI_BASE(0x0058)		/* Pixel Values for Blue Screen 0x00 */
			
#define S5P_HDMI_YMAX				S5P_HDMI_BASE(0x0060)		/* Maximum Y (or R,G,B) Pixel Value 0xEB */
#define S5P_HDMI_YMIN				S5P_HDMI_BASE(0x0064)		/* Minimum Y (or R,G,B) Pixel Value 0x10 */
#define S5P_HDMI_CMAX				S5P_HDMI_BASE(0x0068)		/* Maximum Cb/ Cr Pixel Value 0xF0 */
#define S5P_HDMI_CMIN				S5P_HDMI_BASE(0x006C)		/* Minimum Cb/ Cr Pixel Value 0x10 */
			
#define S5P_HDMI_H_BLANK_0			S5P_HDMI_BASE(0x00A0)		/* Horizontal Blanking Setting 0x00 */
#define S5P_HDMI_H_BLANK_1			S5P_HDMI_BASE(0x00A4)		/* Horizontal Blanking Setting 0x00 */
#define S5P_HDMI_V_BLANK_0			S5P_HDMI_BASE(0x00B0)		/* Vertical Blanking Setting 0x00 */
#define S5P_HDMI_V_BLANK_1			S5P_HDMI_BASE(0x00B4)		/* Vertical Blanking Setting 0x00 */
#define S5P_HDMI_V_BLANK_2			S5P_HDMI_BASE(0x00B8)		/* Vertical Blanking Setting 0x00 */
#define S5P_HDMI_H_V_LINE_0			S5P_HDMI_BASE(0x00C0)		/* Hori. Line and Ver. Line 0x00 */
#define S5P_HDMI_H_V_LINE_1			S5P_HDMI_BASE(0x00C4)		/* Hori. Line and Ver. Line 0x00 */
#define S5P_HDMI_H_V_LINE_2			S5P_HDMI_BASE(0x00C8)		/* Hori. Line and Ver. Line 0x00 */
			
#define S5P_HDMI_SYNC_MODE			S5P_HDMI_BASE(0x00E4)		/* Vertical Sync Polarity Control Register 0x00 */
#define S5P_HDMI_INT_PRO_MODE			S5P_HDMI_BASE(0x00E8)		/* Interlace/ Progressive Control Register 0x00 */
			
#define S5P_HDMI_V_BLANK_F_0			S5P_HDMI_BASE(0x0110)		/* Vertical Blanking Setting for Bottom Field 0x00 */
#define S5P_HDMI_V_BLANK_F_1			S5P_HDMI_BASE(0x0114)		/* Vertical Blanking Setting for Bottom Field 0x00 */
#define S5P_HDMI_V_BLANK_F_2			S5P_HDMI_BASE(0x0118)		/* Vertical Blanking Setting for Bottom Field 0x00 */
#define S5P_HDMI_H_SYNC_GEN_0			S5P_HDMI_BASE(0x0120)		/* Horizontal Sync Generation Setting 0x00 */
#define S5P_HDMI_H_SYNC_GEN_1			S5P_HDMI_BASE(0x0124)		/* Horizontal Sync Generation Setting 0x00 */
#define S5P_HDMI_H_SYNC_GEN_2			S5P_HDMI_BASE(0x0128)		/* Horizontal Sync Generation Setting 0x00 */
#define S5P_HDMI_V_SYNC_GEN_1_0			S5P_HDMI_BASE(0x0130)		/* Vertical Sync Generation for Top Field or Frame. 0x01 */
#define S5P_HDMI_V_SYNC_GEN_1_1			S5P_HDMI_BASE(0x0134)		/* Vertical Sync Generation for Top Field or Frame. 0x10 */
#define S5P_HDMI_V_SYNC_GEN_1_2			S5P_HDMI_BASE(0x0138)		/* Vertical Sync Generation for Top Field or Frame. 0x00 */
#define S5P_HDMI_V_SYNC_GEN_2_0			S5P_HDMI_BASE(0x0140)		/* Vertical Sync Generation for Bottom field ? Vertical position. 0x01 */
#define S5P_HDMI_V_SYNC_GEN_2_1			S5P_HDMI_BASE(0x0144)		/* Vertical Sync Generation for Bottom field ? Vertical position. 0x10 */
#define S5P_HDMI_V_SYNC_GEN_2_2			S5P_HDMI_BASE(0x0148)		/* Vertical Sync Generation for Bottom field ? Vertical position. 0x00 */
#define S5P_HDMI_V_SYNC_GEN_3_0			S5P_HDMI_BASE(0x0150)		/* Vertical Sync Generation for Bottom field ? Horizontal position. 0x01 */
#define S5P_HDMI_V_SYNC_GEN_3_1			S5P_HDMI_BASE(0x0154)		/* Vertical Sync Generation for Bottom field ? Horizontal position. 0x10 */
#define S5P_HDMI_V_SYNC_GEN_3_2			S5P_HDMI_BASE(0x0158)		/* Vertical Sync Generation for Bottom field ? Horizontal position. 0x00 */
			
#define S5P_HDMI_ASP_CON			S5P_HDMI_BASE(0x0160)		/* ASP Packet Control Register 0x00 */
#define S5P_HDMI_ASP_SP_FLAT			S5P_HDMI_BASE(0x0164)		/* ASP Packet sp_flat Bit Control 0x00 */
#define S5P_HDMI_ASP_CHCFG0			S5P_HDMI_BASE(0x0170)		/* ASP Audio Channel Configuration 0x04 */
#define S5P_HDMI_ASP_CHCFG1			S5P_HDMI_BASE(0x0174)		/* ASP Audio Channel Configuration 0x1A */
#define S5P_HDMI_ASP_CHCFG2			S5P_HDMI_BASE(0x0178)		/* ASP Audio Channel Configuration 0x2C */
#define S5P_HDMI_ASP_CHCFG3			S5P_HDMI_BASE(0x017C)		/* ASP Audio Channel Configuration 0x3E */
			
#define S5P_HDMI_ACR_CON			S5P_HDMI_BASE(0x0180)		/* ACR Packet Control Register 0x00 */
#define S5P_HDMI_ACR_MCTS0			S5P_HDMI_BASE(0x0184)		/* Measured CTS Value 0x01 */
#define S5P_HDMI_ACR_MCTS1			S5P_HDMI_BASE(0x0188)		/* Measured CTS Value 0x00 */
#define S5P_HDMI_ACR_MCTS2			S5P_HDMI_BASE(0x018C)		/* Measured CTS Value 0x00 */
#define S5P_HDMI_ACR_CTS0			S5P_HDMI_BASE(0x0190)		/* CTS Value for Fixed CTS Transmission Mode. 0xE8 */
#define S5P_HDMI_ACR_CTS1			S5P_HDMI_BASE(0x0194)		/* CTS Value for Fixed CTS Transmission Mode. 0x03 */
#define S5P_HDMI_ACR_CTS2			S5P_HDMI_BASE(0x0198)		/* CTS Value for Fixed CTS Transmission Mode. 0x00 */
#define S5P_HDMI_ACR_N0				S5P_HDMI_BASE(0x01A0)		/* N Value for ACR Packet. 0xE8 */
#define S5P_HDMI_ACR_N1				S5P_HDMI_BASE(0x01A4)		/* N Value for ACR Packet. 0x03 */
#define S5P_HDMI_ACR_N2				S5P_HDMI_BASE(0x01A8)		/* N Value for ACR Packet. 0x00 */
#define S5P_HDMI_ACR_LSB2			S5P_HDMI_BASE(0x01B0)		/* Altenate LSB for Fixed CTS Transmission Mode 0x00 */
#define S5P_HDMI_ACR_TXCNT			S5P_HDMI_BASE(0x01B4)		/* Number of ACR Packet Transmission per frame 0x1F */
#define S5P_HDMI_ACR_TXINTERVAL			S5P_HDMI_BASE(0x01B8)		/* Interval for ACR Packet Transmission 0x63 */
#define S5P_HDMI_ACR_CTS_OFFSET			S5P_HDMI_BASE(0x01BC)		/* CTS Offset for Measured CTS mode. 0x00 */
			
#define S5P_HDMI_GCP_CON			S5P_HDMI_BASE(0x01C0)		/* ACR Packet Control register 0x00 */
#define S5P_HDMI_GCP_BYTE1			S5P_HDMI_BASE(0x01D0)		/* GCP Packet Body 0x00 */
#define S5P_HDMI_GCP_BYTE2			S5P_HDMI_BASE(0x01D4)		/* GCP Packet Body 0x01 */
#define S5P_HDMI_GCP_BYTE3			S5P_HDMI_BASE(0x01D8)		/* GCP Packet Body 0x02 */
			
#define S5P_HDMI_ACP_CON			S5P_HDMI_BASE(0x01E0)		/* ACP Packet Control register 0x00 */
#define S5P_HDMI_ACP_TYPE			S5P_HDMI_BASE(0x01E4)		/* ACP Packet Header 0x00 */
			
#define S5P_HDMI_ACP_DATA0			S5P_HDMI_BASE(0x0200)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA1			S5P_HDMI_BASE(0x0204)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA2			S5P_HDMI_BASE(0x0208)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA3			S5P_HDMI_BASE(0x020c)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA4			S5P_HDMI_BASE(0x0210)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA5			S5P_HDMI_BASE(0x0214)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA6			S5P_HDMI_BASE(0x0218)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA7			S5P_HDMI_BASE(0x021c)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA8			S5P_HDMI_BASE(0x0220)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA9			S5P_HDMI_BASE(0x0224)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA10			S5P_HDMI_BASE(0x0228)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA11			S5P_HDMI_BASE(0x022c)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA12			S5P_HDMI_BASE(0x0230)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA13			S5P_HDMI_BASE(0x0234)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA14			S5P_HDMI_BASE(0x0238)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA15			S5P_HDMI_BASE(0x023c)		/* ACP Packet Body 0x00 */
#define S5P_HDMI_ACP_DATA16			S5P_HDMI_BASE(0x0240)		/* ACP Packet Body 0x00 */
			
#define S5P_HDMI_ISRC_CON			S5P_HDMI_BASE(0x0250)		/* ACR Packet Control Register 0x00 */
#define S5P_HDMI_ISRC1_HEADER1			S5P_HDMI_BASE(0x0264)		/* ISCR1 Packet Header 0x00 */
			
#define S5P_HDMI_ISRC1_DATA0			S5P_HDMI_BASE(0x0270)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA1			S5P_HDMI_BASE(0x0274)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA2			S5P_HDMI_BASE(0x0278)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA3			S5P_HDMI_BASE(0x027c)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA4			S5P_HDMI_BASE(0x0280)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA5			S5P_HDMI_BASE(0x0284)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA6			S5P_HDMI_BASE(0x0288)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA7			S5P_HDMI_BASE(0x028c)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA8			S5P_HDMI_BASE(0x0290)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA9			S5P_HDMI_BASE(0x0294)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA10			S5P_HDMI_BASE(0x0298)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA11			S5P_HDMI_BASE(0x029c)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA12			S5P_HDMI_BASE(0x02a0)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA13			S5P_HDMI_BASE(0x02a4)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA14			S5P_HDMI_BASE(0x02a8)		/* ISRC1 Packet Body 0x00 */
#define S5P_HDMI_ISRC1_DATA15			S5P_HDMI_BASE(0x02ac)		/* ISRC1 Packet Body 0x00 */
			
#define S5P_HDMI_ISRC2_DATA0			S5P_HDMI_BASE(0x02b0)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA1			S5P_HDMI_BASE(0x02b4)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA2			S5P_HDMI_BASE(0x02b8)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA3			S5P_HDMI_BASE(0x02bc)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA4			S5P_HDMI_BASE(0x02c0)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA5			S5P_HDMI_BASE(0x02c4)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA6			S5P_HDMI_BASE(0x02c8)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA7			S5P_HDMI_BASE(0x02cc)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA8			S5P_HDMI_BASE(0x02d0)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA9			S5P_HDMI_BASE(0x02d4)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA10			S5P_HDMI_BASE(0x02d8)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA11			S5P_HDMI_BASE(0x02dc)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA12			S5P_HDMI_BASE(0x02e0)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA13			S5P_HDMI_BASE(0x02e4)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA14			S5P_HDMI_BASE(0x02e8)		/* ISRC2 Packet Body 0x00 */
#define S5P_HDMI_ISRC2_DATA15			S5P_HDMI_BASE(0x02ec)		/* ISRC2 Packet Body 0x00 */
			
#define S5P_HDMI_AVI_CON			S5P_HDMI_BASE(0x0300)		/* AVI Packet Control Register 0x00 */
#define S5P_HDMI_AVI_CHECK_SUM			S5P_HDMI_BASE(0x0310)		/* AVI Packet Checksum 0x00 */
			
#define S5P_HDMI_AVI_BYTE1			S5P_HDMI_BASE(0x0320)		/* AVI Packet Body 0x00 */
#define S5P_HDMI_AVI_BYTE2			S5P_HDMI_BASE(0x0324)		/* AVI Packet Body 0x00 */
#define S5P_HDMI_AVI_BYTE3			S5P_HDMI_BASE(0x0328)		/* AVI Packet Body 0x00 */
#define S5P_HDMI_AVI_BYTE4			S5P_HDMI_BASE(0x032c)		/* AVI Packet Body 0x00 */
#define S5P_HDMI_AVI_BYTE5			S5P_HDMI_BASE(0x0330)		/* AVI Packet Body 0x00 */
#define S5P_HDMI_AVI_BYTE6			S5P_HDMI_BASE(0x0334)		/* AVI Packet Body 0x00 */
#define S5P_HDMI_AVI_BYTE7			S5P_HDMI_BASE(0x0338)		/* AVI Packet Body 0x00 */
#define S5P_HDMI_AVI_BYTE8			S5P_HDMI_BASE(0x033c)		/* AVI Packet Body 0x00 */
#define S5P_HDMI_AVI_BYTE9			S5P_HDMI_BASE(0x0340)		/* AVI Packet Body 0x00 */
#define S5P_HDMI_AVI_BYTE10			S5P_HDMI_BASE(0x0344)		/* AVI Packet Body 0x00 */
#define S5P_HDMI_AVI_BYTE11			S5P_HDMI_BASE(0x0348)		/* AVI Packet Body 0x00 */
#define S5P_HDMI_AVI_BYTE12			S5P_HDMI_BASE(0x034c)		/* AVI Packet Body 0x00 */
#define S5P_HDMI_AVI_BYTE13			S5P_HDMI_BASE(0x0350)		/* AVI Packet Body 0x00  */
			
#define S5P_HDMI_AUI_CON			S5P_HDMI_BASE(0x0360)		/* AUI Packet Control Register 0x00 */
#define S5P_HDMI_AUI_CHECK_SUM			S5P_HDMI_BASE(0x0370)		/* AUI Packet Checksum 0x00 */
			
#define S5P_HDMI_AUI_BYTE1			S5P_HDMI_BASE(0x0380)		/* AUI Packet Body 0x00 */
#define S5P_HDMI_AUI_BYTE2			S5P_HDMI_BASE(0x0384)		/* AUI Packet Body 0x00 */
#define S5P_HDMI_AUI_BYTE3			S5P_HDMI_BASE(0x0388)		/* AUI Packet Body 0x00 */
#define S5P_HDMI_AUI_BYTE4			S5P_HDMI_BASE(0x038c)		/* AUI Packet Body 0x00 */
#define S5P_HDMI_AUI_BYTE5			S5P_HDMI_BASE(0x0390)		/* AUI Packet Body 0x00 */
			
#define S5P_HDMI_MPG_CON			S5P_HDMI_BASE(0x03A0)		/* ACR Packet Control Register 0x00 */
#define S5P_HDMI_MPG_CHECK_SUM			S5P_HDMI_BASE(0x03B0)		/* MPG Packet Checksum 0x00 */
			
#define S5P_HDMI_MPEG_BYTE1			S5P_HDMI_BASE(0x03c0)		/* MPEG Packet Body 0x00 */
#define S5P_HDMI_MPEG_BYTE2			S5P_HDMI_BASE(0x03c4)		/* MPEG Packet Body 0x00 */
#define S5P_HDMI_MPEG_BYTE3			S5P_HDMI_BASE(0x03c8)		/* MPEG Packet Body 0x00 */
#define S5P_HDMI_MPEG_BYTE4			S5P_HDMI_BASE(0x03cc)		/* MPEG Packet Body 0x00 */
#define S5P_HDMI_MPEG_BYTE5			S5P_HDMI_BASE(0x03d0)		/* MPEG Packet Body 0x00 */
			
#define S5P_HDMI_SPD_CON			S5P_HDMI_BASE(0x0400)		/* SPD Packet Control Register 0x00 */
#define S5P_HDMI_SPD_HEADER0			S5P_HDMI_BASE(0x0410)		/* SPD Packet Header 0x00 */
#define S5P_HDMI_SPD_HEADER1			S5P_HDMI_BASE(0x0414)		/* SPD Packet Header 0x00 */
#define S5P_HDMI_SPD_HEADER2			S5P_HDMI_BASE(0x0418)		/* SPD Packet Header 0x00 */
			
#define S5P_HDMI_SPD_DATA0			S5P_HDMI_BASE(0x0420)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA1			S5P_HDMI_BASE(0x0424)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA2			S5P_HDMI_BASE(0x0428)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA3			S5P_HDMI_BASE(0x042c)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA4			S5P_HDMI_BASE(0x0430)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA5			S5P_HDMI_BASE(0x0434)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA6			S5P_HDMI_BASE(0x0438)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA7			S5P_HDMI_BASE(0x043c)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA8			S5P_HDMI_BASE(0x0440)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA9			S5P_HDMI_BASE(0x0444)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA10			S5P_HDMI_BASE(0x0448)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA11			S5P_HDMI_BASE(0x044c)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA12			S5P_HDMI_BASE(0x0450)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA13			S5P_HDMI_BASE(0x0454)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA14			S5P_HDMI_BASE(0x0458)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA15			S5P_HDMI_BASE(0x045c)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA16			S5P_HDMI_BASE(0x0460)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA17			S5P_HDMI_BASE(0x0464)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA18			S5P_HDMI_BASE(0x0468)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA19			S5P_HDMI_BASE(0x046c)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA20			S5P_HDMI_BASE(0x0470)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA21			S5P_HDMI_BASE(0x0474)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA22			S5P_HDMI_BASE(0x0478)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA23			S5P_HDMI_BASE(0x048c)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA24			S5P_HDMI_BASE(0x0480)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA25			S5P_HDMI_BASE(0x0484)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA26			S5P_HDMI_BASE(0x0488)		/* SPD Packet Body 0x00 */
#define S5P_HDMI_SPD_DATA27			S5P_HDMI_BASE(0x048c)		/* SPD Packet Body 0x00 */
			
#define S5P_HDMI_HDCP_RX_SHA1_0_0		S5P_HDMI_BASE(0x0600)		/* SHA-1 Value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_0_1		S5P_HDMI_BASE(0x0604)		/* SHA-1 Value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_0_2		S5P_HDMI_BASE(0x0608)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_0_3		S5P_HDMI_BASE(0x060C)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_1_0		S5P_HDMI_BASE(0x0610)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_1_1		S5P_HDMI_BASE(0x0614)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_1_2		S5P_HDMI_BASE(0x0618)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_1_3		S5P_HDMI_BASE(0x061C)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_2_0		S5P_HDMI_BASE(0x0620)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_2_1		S5P_HDMI_BASE(0x0624)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_2_2		S5P_HDMI_BASE(0x0628)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_2_3		S5P_HDMI_BASE(0x062C)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_3_0		S5P_HDMI_BASE(0x0630)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_3_1		S5P_HDMI_BASE(0x0634)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_3_2		S5P_HDMI_BASE(0x0638)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_3_3		S5P_HDMI_BASE(0x063C)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_4_0		S5P_HDMI_BASE(0x0640)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_4_1		S5P_HDMI_BASE(0x0644)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_4_2		S5P_HDMI_BASE(0x0648)		/* SHA-1 value from Repeater 0x00 */
#define S5P_HDMI_HDCP_RX_SHA1_4_3		S5P_HDMI_BASE(0x064C)		/* SHA-1 value from Repeater 0x00 */
			
#define S5P_HDMI_HDCP_RX_KSV_0_0		S5P_HDMI_BASE(0x0650)		/* Receiver¡¯s KSV 0 0x00 */
#define S5P_HDMI_HDCP_RX_KSV_0_1		S5P_HDMI_BASE(0x0654)		/* Receiver¡¯s KSV 0 0x00 */
#define S5P_HDMI_HDCP_RX_KSV_0_2		S5P_HDMI_BASE(0x0658)		/* Receiver¡¯s KSV 0 0x00 */
#define S5P_HDMI_HDCP_RX_KSV_0_3		S5P_HDMI_BASE(0x065C)		/* Receiver¡¯s KSV 0 0x00 */
#define S5P_HDMI_HDCP_RX_KSV_0_4		S5P_HDMI_BASE(0x0660)		/* Receiver¡¯s KSV 1 0x00 */
			
#define S5P_HDMI_HDCP_KSV_LIST_CON		S5P_HDMI_BASE(0x0664)		/* Receiver¡¯s KSV 1 0x00 */
#define S5P_HDMI_HDCP_SHA_RESULT		S5P_HDMI_BASE(0x0670)		/* 2nd authentication status 0x00 */
#define S5P_HDMI_HDCP_CTRL1			S5P_HDMI_BASE(0x0680)		/* HDCP Control 0x00 */
#define S5P_HDMI_HDCP_CTRL2			S5P_HDMI_BASE(0x0684)		/* HDCP Control 0x00 */
#define S5P_HDMI_HDCP_CHECK_RESULT		S5P_HDMI_BASE(0x0690)		/* HDCP Ri, Pj, V result 0x00 */
			
#define S5P_HDMI_HDCP_BKSV_0_0			S5P_HDMI_BASE(0x06A0)		/* Receiver¡¯s BKSV 0x00 */
#define S5P_HDMI_HDCP_BKSV_0_1			S5P_HDMI_BASE(0x06A4)		/* Receiver¡¯s BKSV 0x00 */
#define S5P_HDMI_HDCP_BKSV_0_2			S5P_HDMI_BASE(0x06A8)		/* Receiver¡¯s BKSV 0x00 */
#define S5P_HDMI_HDCP_BKSV_0_3			S5P_HDMI_BASE(0x06AC)		/* Receiver¡¯s BKSV 0x00 */
#define S5P_HDMI_HDCP_BKSV_1			S5P_HDMI_BASE(0x06B0)		/* Receiver¡¯s BKSV 0x00 */
			
#define S5P_HDMI_HDCP_AKSV_0_0			S5P_HDMI_BASE(0x06C0)		/* Transmitter¡¯s AKSV 0x00 */
#define S5P_HDMI_HDCP_AKSV_0_1			S5P_HDMI_BASE(0x06C4)		/* Transmitter¡¯s AKSV 0x00 */
#define S5P_HDMI_HDCP_AKSV_0_2			S5P_HDMI_BASE(0x06C8)		/* Transmitter¡¯s AKSV 0x00 */
#define S5P_HDMI_HDCP_AKSV_0_3			S5P_HDMI_BASE(0x06CC)		/* Transmitter¡¯s AKSV 0x00 */
#define S5P_HDMI_HDCP_AKSV_1			S5P_HDMI_BASE(0x06D0)		/* Transmitter¡¯s AKSV 0x00 */
			
#define S5P_HDMI_HDCP_An_0_0			S5P_HDMI_BASE(0x06E0)		/* Transmitter¡¯s An 0x00 */
#define S5P_HDMI_HDCP_An_0_1			S5P_HDMI_BASE(0x06E4)		/* Transmitter¡¯s An 0x00 */
#define S5P_HDMI_HDCP_An_0_2			S5P_HDMI_BASE(0x06E8)		/* Transmitter¡¯s An 0x00 */
#define S5P_HDMI_HDCP_An_0_3			S5P_HDMI_BASE(0x06EC)		/* Transmitter¡¯s An 0x00 */
#define S5P_HDMI_HDCP_An_1_0			S5P_HDMI_BASE(0x06F0)		/* Transmitter¡¯s An 0x00 */
#define S5P_HDMI_HDCP_An_1_1			S5P_HDMI_BASE(0x06F4)		/* Transmitter¡¯s An 0x00 */
#define S5P_HDMI_HDCP_An_1_2			S5P_HDMI_BASE(0x06F8)		/* Transmitter¡¯s An 0x00 */
#define S5P_HDMI_HDCP_An_1_3			S5P_HDMI_BASE(0x06FC)		/* Transmitter¡¯s An 0x00 */
			
#define S5P_HDMI_HDCP_BCAPS			S5P_HDMI_BASE(0x0700)		/* Receiver¡¯s BCAPS 0x00 */
#define S5P_HDMI_HDCP_BSTATUS_0			S5P_HDMI_BASE(0x0710)		/* Receiver¡¯s BSTATUS 0x00 */
#define S5P_HDMI_HDCP_BSTATUS_1			S5P_HDMI_BASE(0x0714)		/* Receiver¡¯s BSTATUS 0x00 */
#define S5P_HDMI_HDCP_Ri_0			S5P_HDMI_BASE(0x0740)		/* Transmitter¡¯s Ri 0x00 */
#define S5P_HDMI_HDCP_Ri_1			S5P_HDMI_BASE(0x0744)		/* Transmitter¡¯s Ri 0x00 */
			
#define S5P_HDMI_HDCP_I2C_INT			S5P_HDMI_BASE(0x0780)		/* HDCP I2C interrupt status */
#define S5P_HDMI_HDCP_AN_INT			S5P_HDMI_BASE(0x0790)		/* HDCP An interrupt status */
#define S5P_HDMI_HDCP_WDT_INT			S5P_HDMI_BASE(0x07a0)		/* HDCP Watchdog interrupt status */
#define S5P_HDMI_HDCP_RI_INT			S5P_HDMI_BASE(0x07b0)		/* HDCP RI interrupt status */
			
#define S5P_HDMI_HDCP_RI_COMPARE_0		S5P_HDMI_BASE(0x07d0)		/* HDCP Ri Interrupt Frame number index register 0 */
#define S5P_HDMI_HDCP_RI_COMPARE_1		S5P_HDMI_BASE(0x07d4)		/* HDCP Ri Interrupt Frame number index register 1 */
#define S5P_HDMI_HDCP_FRAME_COUNT		S5P_HDMI_BASE(0x07e0)		/* Current value of the frame count index in the hardware */
			
#define S5P_HDMI_GAMUT_CON			S5P_HDMI_BASE(0x0500)		/* Gamut Metadata packet transmission control register */
#define S5P_HDMI_GAMUT_HEADER0			S5P_HDMI_BASE(0x0504)		/* Gamut metadata packet header */
#define S5P_HDMI_GAMUT_HEADER1			S5P_HDMI_BASE(0x0508)		/* Gamut metadata packet header */
#define S5P_HDMI_GAMUT_HEADER2			S5P_HDMI_BASE(0x050c)		/* Gamut metadata packet header */
#define S5P_HDMI_GAMUT_DATA00			S5P_HDMI_BASE(0x0510)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA01			S5P_HDMI_BASE(0x0514)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA02			S5P_HDMI_BASE(0x0518)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA03			S5P_HDMI_BASE(0x051c)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA04			S5P_HDMI_BASE(0x0520)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA05			S5P_HDMI_BASE(0x0524)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA06			S5P_HDMI_BASE(0x0528)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA07			S5P_HDMI_BASE(0x052c)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA08			S5P_HDMI_BASE(0x0530)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA09			S5P_HDMI_BASE(0x0534)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA10			S5P_HDMI_BASE(0x0538)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA11			S5P_HDMI_BASE(0x053c)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA12			S5P_HDMI_BASE(0x0540)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA13			S5P_HDMI_BASE(0x0544)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA14			S5P_HDMI_BASE(0x0548)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA15			S5P_HDMI_BASE(0x054c)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA16			S5P_HDMI_BASE(0x0550)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA17			S5P_HDMI_BASE(0x0554)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA18			S5P_HDMI_BASE(0x0558)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA19			S5P_HDMI_BASE(0x055c)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA20			S5P_HDMI_BASE(0x0560)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA21			S5P_HDMI_BASE(0x0564)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA22			S5P_HDMI_BASE(0x0568)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA23			S5P_HDMI_BASE(0x056c)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA24			S5P_HDMI_BASE(0x0570)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA25			S5P_HDMI_BASE(0x0574)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA26			S5P_HDMI_BASE(0x0578)		/* Gamut Metadata packet body data */
#define S5P_HDMI_GAMUT_DATA27			S5P_HDMI_BASE(0x057c)		/* Gamut Metadata packet body data */

#define	S5P_HDMI_DC_CONTROL			S5P_HDMI_BASE(0x05C0)		/* Gamut Metadata packet body data */
#define S5P_HDMI_VIDEO_PATTERN_GEN		S5P_HDMI_BASE(0x05C4)		/* Gamut Metadata packet body data */
#define S5P_HDMI_HPD_GEN			S5P_HDMI_BASE(0x05C8)		/* Gamut Metadata packet body data */


#define S5P_HDMI_SPDIFIN_CLK_CTRL		S5P_HDMI_SPDIF_BASE(0x0000)	/* SPDIFIN_CLK_CTRL [1:0] 0x02 */
#define S5P_HDMI_SPDIFIN_OP_CTRL		S5P_HDMI_SPDIF_BASE(0x0004)	/* SPDIFIN_OP_CTRL [1:0] 0x00 */
#define S5P_HDMI_SPDIFIN_IRQ_MASK		S5P_HDMI_SPDIF_BASE(0x0008)	/* SPDIFIN_IRQ_MASK[7:0] 0x00 */
#define S5P_HDMI_SPDIFIN_IRQ_STATUS		S5P_HDMI_SPDIF_BASE(0x000C)	/* SPDIFIN_IRQ_STATUS [7:0] 0x00 */
#define S5P_HDMI_SPDIFIN_CONFIG_1		S5P_HDMI_SPDIF_BASE(0x0010)	/* SPDIFIN_CONFIG [7:0] 0x00 */
#define S5P_HDMI_SPDIFIN_CONFIG_2		S5P_HDMI_SPDIF_BASE(0x0014)	/* SPDIFIN_CONFIG [11:8] 0x00 */
#define S5P_HDMI_SPDIFIN_USER_VALUE_1		S5P_HDMI_SPDIF_BASE(0x0020)	/* SPDIFIN_USER_VALUE [7:0] 0x00 */
#define S5P_HDMI_SPDIFIN_USER_VALUE_2		S5P_HDMI_SPDIF_BASE(0x0024)	/* SPDIFIN_USER_VALUE [15:8] 0x00 */
#define S5P_HDMI_SPDIFIN_USER_VALUE_3		S5P_HDMI_SPDIF_BASE(0x0028)	/* SPDIFIN_USER_VALUE [23:16] 0x00 */
#define S5P_HDMI_SPDIFIN_USER_VALUE_4		S5P_HDMI_SPDIF_BASE(0x002C)	/* SPDIFIN_USER_VALUE [31:24] 0x00 */
#define S5P_HDMI_SPDIFIN_CH_STATUS_0_1		S5P_HDMI_SPDIF_BASE(0x0030)	/* SPDIFIN_CH_STATUS_0 [7:0] 0x00 */
#define S5P_HDMI_SPDIFIN_CH_STATUS_0_2		S5P_HDMI_SPDIF_BASE(0x0034)	/* SPDIFIN_CH_STATUS_0 [15:8] 0x00 */
#define S5P_HDMI_SPDIFIN_CH_STATUS_0_3		S5P_HDMI_SPDIF_BASE(0x0038)	/* SPDIFIN_CH_STATUS_0 [23:16] 0x00 */
#define S5P_HDMI_SPDIFIN_CH_STATUS_0_4		S5P_HDMI_SPDIF_BASE(0x003C)	/* SPDIFIN_CH_STATUS_0 [31:24] 0x00 */
#define S5P_HDMI_SPDIFIN_CH_STATUS_1		S5P_HDMI_SPDIF_BASE(0x0040)	/* SPDIFIN_CH_STATUS_1 0x00 */
#define S5P_HDMI_SPDIFIN_FRAME_PERIOD_1		S5P_HDMI_SPDIF_BASE(0x0048)	/* SPDIF_FRAME_PERIOD [7:0] 0x00 */
#define S5P_HDMI_SPDIFIN_FRAME_PERIOD_2		S5P_HDMI_SPDIF_BASE(0x004C)	/* SPDIF_FRAME_PERIOD [15:8] 0x00 */
#define S5P_HDMI_SPDIFIN_Pc_INFO_1		S5P_HDMI_SPDIF_BASE(0x0050)	/* SPDIFIN_Pc_INFO [7:0] 0x00 */
#define S5P_HDMI_SPDIFIN_Pc_INFO_2		S5P_HDMI_SPDIF_BASE(0x0054)	/* SPDIFIN_Pc_INFO [15:8] 0x00 */
#define S5P_HDMI_SPDIFIN_Pd_INFO_1		S5P_HDMI_SPDIF_BASE(0x0058)	/* SPDIFIN_Pd_INFO [7:0] 0x00 */
#define S5P_HDMI_SPDIFIN_Pd_INFO_2		S5P_HDMI_SPDIF_BASE(0x005C)	/* SPDIFIN_Pd_INFO [15:8] 0x00 */
#define S5P_HDMI_SPDIFIN_DATA_BUF_0_1		S5P_HDMI_SPDIF_BASE(0x0060)	/* SPDIFIN_DATA_BUF_0 [7:0] 0x00 */
#define S5P_HDMI_SPDIFIN_DATA_BUF_0_2		S5P_HDMI_SPDIF_BASE(0x0064)	/* SPDIFIN_DATA_BUF_0 [15:8] 0x00 */
#define S5P_HDMI_SPDIFIN_DATA_BUF_0_3		S5P_HDMI_SPDIF_BASE(0x0068)	/* SPDIFIN_DATA_BUF_0 [23:16] 0x00 */
#define S5P_HDMI_SPDIFIN_USER_BUF_0		S5P_HDMI_SPDIF_BASE(0x006C)	/* SPDIFIN_DATA_BUF_0 [31:28] 0x00 */
#define S5P_HDMI_SPDIFIN_DATA_BUF_1_1		S5P_HDMI_SPDIF_BASE(0x0070)	/* SPDIFIN_DATA_BUF_1 [7:0] 0x00 */
#define S5P_HDMI_SPDIFIN_DATA_BUF_1_2		S5P_HDMI_SPDIF_BASE(0x0074)	/* SPDIFIN_DATA_BUF_1 [15:8] 0x00 */
#define S5P_HDMI_SPDIFIN_DATA_BUF_1_3		S5P_HDMI_SPDIF_BASE(0x0078)	/* SPDIFIN_DATA_BUF_1 [23:16] 0x00 */
#define S5P_HDMI_SPDIFIN_USER_BUF_1		S5P_HDMI_SPDIF_BASE(0x007C)	/* SPDIFIN_DATA_BUF_1 [31:28] 0x00 */


#define S5P_HDMI_I2S_CLK_CON			S5P_HDMI_I2S_BASE(0x0000)	/* I2S Clock Enable Register0x00  */
#define S5P_HDMI_I2S_CON_1			S5P_HDMI_I2S_BASE(0x0004)	/* I2S Control Register 10x00  */
#define S5P_HDMI_I2S_CON_2			S5P_HDMI_I2S_BASE(0x0008)	/* I2S Control Register 20x00  */
#define S5P_HDMI_I2S_PIN_SEL_0			S5P_HDMI_I2S_BASE(0x000C)	/* I2S Input Pin Selection Register 0 0x77  */
#define S5P_HDMI_I2S_PIN_SEL_1			S5P_HDMI_I2S_BASE(0x0010)	/* I2S Input Pin Selection Register 1 0x77  */
#define S5P_HDMI_I2S_PIN_SEL_2			S5P_HDMI_I2S_BASE(0x0014)	/* I2S Input Pin Selection Register 2 0x77  */
#define S5P_HDMI_I2S_PIN_SEL_3			S5P_HDMI_I2S_BASE(0x0018)	/* I2S Input Pin Selection Register 30x07  */
#define S5P_HDMI_I2S_DSD_CON			S5P_HDMI_I2S_BASE(0x001C)	/* I2S DSD Control Register0x02  */
#define S5P_HDMI_I2S_MUX_CON			S5P_HDMI_I2S_BASE(0x0020)	/* I2S In/Mux Control Register 0x60  */
#define S5P_HDMI_I2S_CH_ST_CON			S5P_HDMI_I2S_BASE(0x0024)	/* I2S Channel Status Control Register0x00  */
#define S5P_HDMI_I2S_CH_ST_0			S5P_HDMI_I2S_BASE(0x0028)	/* I2S Channel Status Block 00x00  */
#define S5P_HDMI_I2S_CH_ST_1			S5P_HDMI_I2S_BASE(0x002C)	/* I2S Channel Status Block 10x00  */
#define S5P_HDMI_I2S_CH_ST_2			S5P_HDMI_I2S_BASE(0x0030)	/* I2S Channel Status Block 20x00  */
#define S5P_HDMI_I2S_CH_ST_3			S5P_HDMI_I2S_BASE(0x0034)	/* I2S Channel Status Block 30x00  */
#define S5P_HDMI_I2S_CH_ST_4			S5P_HDMI_I2S_BASE(0x0038)	/* I2S Channel Status Block 40x00  */
#define S5P_HDMI_I2S_CH_ST_SH_0			S5P_HDMI_I2S_BASE(0x003C)	/* I2S Channel Status Block Shadow Register 00x00  */
#define S5P_HDMI_I2S_CH_ST_SH_1			S5P_HDMI_I2S_BASE(0x0040)	/* I2S Channel Status Block Shadow Register 10x00  */
#define S5P_HDMI_I2S_CH_ST_SH_2			S5P_HDMI_I2S_BASE(0x0044)	/* I2S Channel Status Block Shadow Register 20x00  */
#define S5P_HDMI_I2S_CH_ST_SH_3			S5P_HDMI_I2S_BASE(0x0048)	/* I2S Channel Status Block Shadow Register 30x00  */
#define S5P_HDMI_I2S_CH_ST_SH_4			S5P_HDMI_I2S_BASE(0x004C)	/* I2S Channel Status Block Shadow Register 40x00  */
#define S5P_HDMI_I2S_VD_DATA			S5P_HDMI_I2S_BASE(0x0050)	/* I2S Audio Sample Validity Register0x00  */
#define S5P_HDMI_I2S_MUX_CH			S5P_HDMI_I2S_BASE(0x0054)	/* I2S Channel Enable Register0x03  */
#define S5P_HDMI_I2S_MUX_CUV			S5P_HDMI_I2S_BASE(0x0058)	/* I2S CUV Enable Register0x03  */
#define S5P_HDMI_I2S_IRQ_MASK			S5P_HDMI_I2S_BASE(0x005C)	/* I2S Interrupt Request Mask Register0x03  */
#define S5P_HDMI_I2S_IRQ_STATUS			S5P_HDMI_I2S_BASE(0x0060)	/* I2S Interrupt Request Status Register0x00  */
#define S5P_HDMI_I2S_CH0_L_0			S5P_HDMI_I2S_BASE(0x0064)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_L_1			S5P_HDMI_I2S_BASE(0x0068)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_L_2			S5P_HDMI_I2S_BASE(0x006C)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_L_3			S5P_HDMI_I2S_BASE(0x0070)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_R_0			S5P_HDMI_I2S_BASE(0x0074)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_R_1			S5P_HDMI_I2S_BASE(0x0078)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_R_2			S5P_HDMI_I2S_BASE(0x007C)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH0_R_3			S5P_HDMI_I2S_BASE(0x0080)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_L_0			S5P_HDMI_I2S_BASE(0x0084)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_L_1			S5P_HDMI_I2S_BASE(0x0088)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_L_2			S5P_HDMI_I2S_BASE(0x008C)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_L_3			S5P_HDMI_I2S_BASE(0x0090)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_R_0			S5P_HDMI_I2S_BASE(0x0094)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_R_1			S5P_HDMI_I2S_BASE(0x0098)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_R_2			S5P_HDMI_I2S_BASE(0x009C)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH1_R_3			S5P_HDMI_I2S_BASE(0x00A0)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_L_0			S5P_HDMI_I2S_BASE(0x00A4)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_L_1			S5P_HDMI_I2S_BASE(0x00A8)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_L_2			S5P_HDMI_I2S_BASE(0x00AC)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_L_3			S5P_HDMI_I2S_BASE(0x00B0)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_R_0			S5P_HDMI_I2S_BASE(0x00B4)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_R_1			S5P_HDMI_I2S_BASE(0x00B8)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH2_R_2			S5P_HDMI_I2S_BASE(0x00BC)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_Ch2_R_3			S5P_HDMI_I2S_BASE(0x00C0)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH3_L_0			S5P_HDMI_I2S_BASE(0x00C4)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH3_L_1			S5P_HDMI_I2S_BASE(0x00C8)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH3_L_2			S5P_HDMI_I2S_BASE(0x00CC)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH3_R_0			S5P_HDMI_I2S_BASE(0x00D0)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH3_R_1			S5P_HDMI_I2S_BASE(0x00D4)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CH3_R_2			S5P_HDMI_I2S_BASE(0x00D8)	/* I2S PCM Output Data Register0x00  */
#define S5P_HDMI_I2S_CUV_L_R			S5P_HDMI_I2S_BASE(0x00DC)	/* I2S CUV Output Data Register0x00  */


#define S5P_HDMI_TG_CMD				S5P_HDMI_TG_BASE(0x0000)	/* Command Register 0x00 */
#define S5P_HDMI_TG_H_FSZ_L			S5P_HDMI_TG_BASE(0x0018)	/* Horizontal Full Size 0x72 */
#define S5P_HDMI_TG_H_FSZ_H			S5P_HDMI_TG_BASE(0x001C)	/* Horizontal Full Size 0x06 */
#define S5P_HDMI_TG_HACT_ST_L			S5P_HDMI_TG_BASE(0x0020)	/* Horizontal Active Start 0x05 */
#define S5P_HDMI_TG_HACT_ST_H			S5P_HDMI_TG_BASE(0x0024)	/* Horizontal Active Start 0x01 */
#define S5P_HDMI_TG_HACT_SZ_L			S5P_HDMI_TG_BASE(0x0028)	/* Horizontal Active Size 0x00 */
#define S5P_HDMI_TG_HACT_SZ_H			S5P_HDMI_TG_BASE(0x002C)	/* Horizontal Active Size 0x05 */
#define S5P_HDMI_TG_V_FSZ_L			S5P_HDMI_TG_BASE(0x0030)	/* Vertical Full Line Size 0xEE */
#define S5P_HDMI_TG_V_FSZ_H			S5P_HDMI_TG_BASE(0x0034)	/* Vertical Full Line Size 0x02 */
#define S5P_HDMI_TG_VSYNC_L			S5P_HDMI_TG_BASE(0x0038)	/* Vertical Sync Position 0x01 */
#define S5P_HDMI_TG_VSYNC_H			S5P_HDMI_TG_BASE(0x003C)	/* Vertical Sync Position 0x00 */
#define S5P_HDMI_TG_VSYNC2_L			S5P_HDMI_TG_BASE(0x0040)	/* Vertical Sync Position for Bottom Field 0x33 */
#define S5P_HDMI_TG_VSYNC2_H			S5P_HDMI_TG_BASE(0x0044)	/* Vertical Sync Position for Bottom Field 0x02 */
#define S5P_HDMI_TG_VACT_ST_L			S5P_HDMI_TG_BASE(0x0048)	/* Vertical Sync Active Start Position 0x1a */
#define S5P_HDMI_TG_VACT_ST_H			S5P_HDMI_TG_BASE(0x004C)	/* Vertical Sync Active Start Position 0x00 */
#define S5P_HDMI_TG_VACT_SZ_L			S5P_HDMI_TG_BASE(0x0050)	/* Vertical Active Size 0xd0 */
#define S5P_HDMI_TG_VACT_SZ_H			S5P_HDMI_TG_BASE(0x0054)	/* Vertical Active Size 0x02 */
#define S5P_HDMI_TG_FIELD_CHG_L			S5P_HDMI_TG_BASE(0x0058)	/* Field Change Position 0x33 */
#define S5P_HDMI_TG_FIELD_CHG_H			S5P_HDMI_TG_BASE(0x005C)	/* Field Change Position 0x02 */
#define S5P_HDMI_TG_VACT_ST2_L			S5P_HDMI_TG_BASE(0x0060)	/* Vertical Sync Active Start Position for Bottom Field 0x48 */
#define S5P_HDMI_TG_VACT_ST2_H			S5P_HDMI_TG_BASE(0x0064)	/* Vertical Sync Active Start Position for Bottom Field 0x02 */
			
#define S5P_HDMI_TG_VSYNC_TOP_HDMI_L		S5P_HDMI_TG_BASE(0x0078)	/* HDMI Vsync Positon for Top Field 0x01 */
#define S5P_HDMI_TG_VSYNC_TOP_HDMI_H		S5P_HDMI_TG_BASE(0x007C)	/* HDMI Vsync Positon for Top Field 0x00 */
#define S5P_HDMI_TG_VSYNC_BOT_HDMI_L		S5P_HDMI_TG_BASE(0x0080)	/* HDMI Vsync Positon for Bottom Field 0x33 */
#define S5P_HDMI_TG_VSYNC_BOT_HDMI_H		S5P_HDMI_TG_BASE(0x0084)	/* HDMI Vsync Positon for Bottom Field 0x02 */
#define S5P_HDMI_TG_FIELD_TOP_HDMI_L		S5P_HDMI_TG_BASE(0x0088)	/* HDMI Top Field Start Position 0x01 */
#define S5P_HDMI_TG_FIELD_TOP_HDMI_H		S5P_HDMI_TG_BASE(0x008C)	/* HDMI Top Field Start Position 0x00 */
#define S5P_HDMI_TG_FIELD_BOT_HDMI_L		S5P_HDMI_TG_BASE(0x0090)	/* HDMI Bottom Field Start Position 0x33 */
#define S5P_HDMI_TG_FIELD_BOT_HDMI_H		S5P_HDMI_TG_BASE(0x0094)	/* HDMI Bottom Field Start Position 0x02 */

#define S5P_HDMI_EFUSE_CTRL			S5P_HDMI_EFUSE_BASE(0x0000)
#define S5P_HDMI_EFUSE_STATUS			S5P_HDMI_EFUSE_BASE(0x0004)
#define S5P_HDMI_EFUSE_ADDR_WIDTH		S5P_HDMI_EFUSE_BASE(0x0008)
#define S5P_HDMI_EFUSE_SIGDEV_ASSERT		S5P_HDMI_EFUSE_BASE(0x000c)
#define S5P_HDMI_EFUSE_SIGDEV_DEASSERT		S5P_HDMI_EFUSE_BASE(0x0010)
#define S5P_HDMI_EFUSE_PRCHG_ASSERT		S5P_HDMI_EFUSE_BASE(0x0014)
#define S5P_HDMI_EFUSE_PRCHG_DEASSERT		S5P_HDMI_EFUSE_BASE(0x0018)
#define S5P_HDMI_EFUSE_FSET_ASSERT		S5P_HDMI_EFUSE_BASE(0x001c)
#define S5P_HDMI_EFUSE_FSET_DEASSERT		S5P_HDMI_EFUSE_BASE(0x0020)
#define S5P_HDMI_EFUSE_SENSING			S5P_HDMI_EFUSE_BASE(0x0024)
#define S5P_HDMI_EFUSE_SCK_ASSERT		S5P_HDMI_EFUSE_BASE(0x0028)
#define S5P_HDMI_EFUSE_SCK_DEASSERT		S5P_HDMI_EFUSE_BASE(0x002c)
#define S5P_HDMI_EFUSE_SDOUT_OFFSET		S5P_HDMI_EFUSE_BASE(0x0030)
#define S5P_HDMI_EFUSE_READ_OFFSET		S5P_HDMI_EFUSE_BASE(0x0034)
			
			


/*
 * Bit definition part
 */

/* Control Register */

/* INTC_CON */
#define S5P_HDMI_INTC_ACT_HI			(1 << 7)
#define S5P_HDMI_INTC_ACT_LOW			(0 << 7)
#define S5P_HDMI_INTC_EN_GLOBAL			(1 << 6)
#define S5P_HDMI_INTC_DIS_GLOBAL		(0 << 6)
#define S5P_HDMI_INTC_EN_I2S			(1 << 5)
#define S5P_HDMI_INTC_DIS_I2S			(0 << 5)
#define S5P_HDMI_INTC_EN_CEC			(1 << 4)
#define S5P_HDMI_INTC_DIS_CEC			(0 << 4)
#define S5P_HDMI_INTC_EN_HPD_PLUG		(1 << 3)
#define S5P_HDMI_INTC_DIS_HPD_PLUG		(0 << 3)
#define S5P_HDMI_INTC_EN_HPD_UNPLUG		(1 << 2)
#define S5P_HDMI_INTC_DIS_HPD_UNPLUG		(0 << 2)
#define S5P_HDMI_INTC_EN_SPDIF			(1 << 1)
#define S5P_HDMI_INTC_DIS_SPDIF			(0 << 1)
#define S5P_HDMI_INTC_EN_HDCP			(1 << 0)
#define S5P_HDMI_INTC_DIS_HDCP			(0 << 0)

/* INTC_FLAG */
#define S5P_HDMI_INTC_FLAG_I2S			(1 << 5)
#define S5P_HDMI_INTC_FLAG_CEC			(1 << 4)
#define S5P_HDMI_INTC_FLAG_HPD_PLUG		(1 << 3)
#define S5P_HDMI_INTC_FLAG_HPD_UNPLUG		(1 << 2)
#define S5P_HDMI_INTC_FLAG_SPDIF		(1 << 1)
#define S5P_HDMI_INTC_FLAG_HDCP			(1 << 0)

/* HDCP_KEY_LOAD_DONE */
#define S5P_HDMI_HDCP_KEY_LOAD_DONE		(1 << 0)

/* HPD_STATUS */
#define S5P_HDMI_HPD_PLUGED			(1 << 0)

/* AUDIO_CLKSEL */
#define S5P_HDMI_AUDIO_SPDIF_CLK		(1 << 0)
#define S5P_HDMI_AUDIO_PCLK			(0 << 0)

/* HDMI_PHY_RSTOUT */
#define S5P_HDMI_PHY_SW_RSTOUT			(1 << 0)

/* HDMI_PHY_VPLL */
#define S5P_HDMI_PHY_VPLL_LOCK			(1 << 7)
#define S5P_HDMI_PHY_VPLL_CODE_MASK		(0x7 << 0)

/* HDMI_PHY_CMU */
#define S5P_HDMI_PHY_CMU_LOCK			(1 << 7)
#define S5P_HDMI_PHY_CMU_CODE_MASK		(0x7 << 0)

/* HDMI_CORE_RSTOUT */
#define S5P_HDMI_CORE_SW_RSTOUT			(1 << 0)


/* Core Register */

/* HDMI_CON_0 */
#define S5P_HDMI_BLUE_SCR_EN			(1 << 5)
#define S5P_HDMI_BLUE_SCR_DIS			(0 << 5)
#define S5P_HDMI_ENC_OPTION			(1 << 4)
#define S5P_HDMI_ASP_EN				(1 << 2)
#define S5P_HDMI_ASP_DIS			(0 << 2)
#define S5P_HDMI_PWDN_ENB_NORMAL		(1 << 1)
#define S5P_HDMI_PWDN_ENB_PD			(0 << 1)
#define S5P_HDMI_EN				(1 << 0)
#define S5P_HDMI_DIS				(~(1 << 0))

/* HDMI_CON_1 */
#define S5P_HDMI_PX_LMT_CTRL_BYPASS		(0 << 5)
#define S5P_HDMI_PX_LMT_CTRL_RGB		(1 << 5)
#define S5P_HDMI_PX_LMT_CTRL_YPBPR		(2 << 5)
#define S5P_HDMI_PX_LMT_CTRL_RESERVED		(3 << 5)
#define S5P_HDMI_CON_PXL_REP_RATIO_MASK		(1 << 1 | 1 << 0)		/*Not support in S5PV210 */
#define S5P_HDMI_DOUBLE_PIXEL_REPETITION	(0x01)				/*Not support in S5PV210 */

/* HDMI_CON_2 */
#define S5P_HDMI_VID_PREAMBLE_EN		(0 << 5)
#define S5P_HDMI_VID_PREAMBLE_DIS		(1 << 5)
#define S5P_HDMI_GUARD_BAND_EN			(0 << 1)
#define S5P_HDMI_GUARD_BAND_DIS			(1 << 1)

/* STATUS */
#define S5P_HDMI_AUTHEN_ACK_AUTH		(1 << 7)
#define S5P_HDMI_AUTHEN_ACK_NOT			(0 << 7)
#define S5P_HDMI_AUD_FIFO_OVF_FULL		(1 << 6)
#define S5P_HDMI_AUD_FIFO_OVF_NOT		(0 << 6)
#define S5P_HDMI_UPDATE_RI_INT_OCC		(1 << 4)
#define S5P_HDMI_UPDATE_RI_INT_NOT		(0 << 4)
#define S5P_HDMI_UPDATE_RI_INT_CLEAR		(1 << 4)
#define S5P_HDMI_UPDATE_PJ_INT_OCC		(1 << 3)
#define S5P_HDMI_UPDATE_PJ_INT_NOT		(0 << 3)
#define S5P_HDMI_UPDATE_PJ_INT_CLEAR		(1 << 3)
#define S5P_HDMI_WRITE_INT_OCC			(1 << 2)
#define S5P_HDMI_WRITE_INT_NOT			(0 << 2)
#define S5P_HDMI_WRITE_INT_CLEAR		(1 << 2)
#define S5P_HDMI_WATCHDOG_INT_OCC		(1 << 1)
#define S5P_HDMI_WATCHDOG_INT_NOT		(0 << 1)
#define S5P_HDMI_WATCHDOG_INT_CLEAR		(1 << 1)
#define S5P_HDMI_WTFORACTIVERX_INT_OCC		(1)
#define S5P_HDMI_WTFORACTIVERX_INT_NOT		(0)
#define S5P_HDMI_WTFORACTIVERX_INT_CLEAR	(1)

/* PHY_STATUS */
#define S5P_HDMI_PHY_STATUS_READY		(1)

/* STATUS_EN */
#define S5P_HDMI_AUD_FIFO_OVF_EN		(1 << 6)
#define S5P_HDMI_AUD_FIFO_OVF_DIS		(0 << 6)
#define S5P_HDMI_UPDATE_RI_INT_EN		(1 << 4)
#define S5P_HDMI_UPDATE_RI_INT_DIS		(0 << 4)
#define S5P_HDMI_UPDATE_PJ_INT_EN		(1 << 3)
#define S5P_HDMI_UPDATE_PJ_INT_DIS		(0 << 3)
#define S5P_HDMI_WRITE_INT_EN			(1 << 2)
#define S5P_HDMI_WRITE_INT_DIS			(0 << 2)
#define S5P_HDMI_WATCHDOG_INT_EN		(1 << 1)
#define S5P_HDMI_WATCHDOG_INT_DIS		(0 << 1)
#define S5P_HDMI_WTFORACTIVERX_INT_EN		(1)
#define S5P_HDMI_WTFORACTIVERX_INT_DIS		(0)
#define S5P_HDMI_INT_EN_ALL 			(S5P_HDMI_UPDATE_RI_INT_EN|\
						S5P_HDMI_UPDATE_PJ_INT_DIS|\
						S5P_HDMI_WRITE_INT_EN|\
						S5P_HDMI_WATCHDOG_INT_EN|\
						S5P_HDMI_WTFORACTIVERX_INT_EN)
#define S5P_HDMI_INT_DIS_ALL			(~0x1F)	

/* HPD */
#define S5P_HDMI_SW_HPD_PLUGGED			(1 << 1)
#define S5P_HDMI_SW_HPD_UNPLUGGED		(0 << 1)
#define S5P_HDMI_HPD_SEL_I_HPD			(1)
#define S5P_HDMI_HPD_SEL_SW_HPD			(0)

/* MODE_SEL */
#define S5P_HDMI_MODE_EN			(1 << 1)
#define S5P_HDMI_MODE_DIS			(0 << 1)
#define S5P_HDMI_DVI_MODE_EN			(1)
#define S5P_HDMI_DVI_MODE_DIS			(0)

/* ENC_EN */
#define S5P_HDMI_HDCP_ENC_ENABLE		(1)
#define S5P_HDMI_HDCP_ENC_DISABLE		(0)


/* Video Related Register */

/* BLUESCREEN_0/1/2 */
#define S5P_HDMI_SET_BLUESCREEN_0(x)		((x) & 0xFF)
#define S5P_HDMI_SET_BLUESCREEN_1(x)		((x) & 0xFF)
#define S5P_HDMI_SET_BLUESCREEN_2(x)		((x) & 0xFF)

/* HDMI_YMAX/YMIN/CMAX/CMIN */
#define S5P_HDMI_SET_YMAX(x)			((x) & 0xFF)
#define S5P_HDMI_SET_YMIN(x)			((x) & 0xFF)
#define S5P_HDMI_SET_CMAX(x)			((x) & 0xFF)
#define S5P_HDMI_SET_CMIN(x)			((x) & 0xFF)

/* H_BLANK_0/1 */
#define S5P_HDMI_SET_H_BLANK_0(x)		((x) & 0xFF)
#define S5P_HDMI_SET_H_BLANK_1(x)		(((x) >> 8) & 0x3FF)

/* V_BLANK_0/1/2 */
#define S5P_HDMI_SET_V_BLANK_0(x)		((x) & 0xFF)
#define S5P_HDMI_SET_V_BLANK_1(x)		(((x) >> 8) & 0xFF)
#define S5P_HDMI_SET_V_BLANK_2(x)		(((x) >> 16) & 0xFF)

/* H_V_LINE_0/1/2 */
#define S5P_HDMI_SET_H_V_LINE_0(x)		((x) & 0xFF)
#define S5P_HDMI_SET_H_V_LINE_1(x)		(((x) >> 8) & 0xFF)
#define S5P_HDMI_SET_H_V_LINE_2(x)		(((x) >> 16) & 0xFF)

/* VSYNC_POL */
#define S5P_HDMI_V_SYNC_POL_ACT_LOW		(1)
#define S5P_HDMI_V_SYNC_POL_ACT_HIGH		(0)

/* INT_PRO_MODE */
#define S5P_HDMI_INTERLACE_MODE			(1)
#define S5P_HDMI_PROGRESSIVE_MODE		(0)

/* V_BLANK_F_0/1/2 */
#define S5P_HDMI_SET_V_BLANK_F_0(x)		((x) & 0xFF)
#define S5P_HDMI_SET_V_BLANK_F_1(x)		(((x) >> 8) & 0xFF)
#define S5P_HDMI_SET_V_BLANK_F_2(x)		(((x) >> 16) & 0xFF)


/* H_SYNC_GEN_0/1/2 */
#define S5P_HDMI_SET_H_SYNC_GEN_0(x)		((x) & 0xFF)
#define S5P_HDMI_SET_H_SYNC_GEN_1(x)		(((x) >> 8) & 0xFF)
#define S5P_HDMI_SET_H_SYNC_GEN_2(x)		(((x) >> 16) & 0xFF)


/* V_SYNC_GEN1_0/1/2 */
#define S5P_HDMI_SET_V_SYNC_GEN1_0(x)		((x) & 0xFF)
#define S5P_HDMI_SET_V_SYNC_GEN1_1(x)		(((x) >> 8) & 0xFF)
#define S5P_HDMI_SET_V_SYNC_GEN1_2(x)		(((x) >> 16) & 0xFF)

/* V_SYNC_GEN2_0/1/2 */
#define S5P_HDMI_SET_V_SYNC_GEN2_0(x)		((x) & 0xFF)
#define S5P_HDMI_SET_V_SYNC_GEN2_1(x)		(((x) >> 8) & 0xFF)
#define S5P_HDMI_SET_V_SYNC_GEN2_2(x)		(((x) >> 16) & 0xFF)

/* V_SYNC_GEN3_0/1/2 */
#define S5P_HDMI_SET_V_SYNC_GEN3_0(x)		((x) & 0xFF)
#define S5P_HDMI_SET_V_SYNC_GEN3_1(x)		(((x) >> 8) & 0xFF)
#define S5P_HDMI_SET_V_SYNC_GEN3_2(x)		(((x) >> 16) & 0xFF)


/* Audio Related Packet Register */

/* ASP_CON */
#define S5P_HDMI_AUD_DST_DOUBLE			(1 << 7)
#define S5P_HDMI_AUD_NO_DST_DOUBLE		(0 << 7)
#define S5P_HDMI_AUD_TYPE_SAMPLE		(0 << 5)
#define S5P_HDMI_AUD_TYPE_ONE_BIT		(1 << 5)
#define S5P_HDMI_AUD_TYPE_HBR			(2 << 5)
#define S5P_HDMI_AUD_TYPE_DST			(3 << 5)
#define S5P_HDMI_AUD_MODE_TWO_CH		(0 << 4)
#define S5P_HDMI_AUD_MODE_MULTI_CH		(1 << 4)
#define S5P_HDMI_AUD_SP_AUD3_EN			(1 << 3)
#define S5P_HDMI_AUD_SP_AUD2_EN			(1 << 2)
#define S5P_HDMI_AUD_SP_AUD1_EN			(1 << 1)
#define S5P_HDMI_AUD_SP_AUD0_EN			(1 << 0)
#define S5P_HDMI_AUD_SP_ALL_DIS			(0 << 0)

#define S5P_HDMI_AUD_SET_SP_PRE(x)		((x) & 0xF)

/* ASP_SP_FLAT */
#define S5P_HDMI_ASP_SP_FLAT_AUD_SAMPLE		(0)

/* ASP_CHCFG0/1/2/3 */
#define S5P_HDMI_SPK3R_SEL_I_PCM0L		(0 << 27)
#define S5P_HDMI_SPK3R_SEL_I_PCM0R		(1 << 27)
#define S5P_HDMI_SPK3R_SEL_I_PCM1L		(2 << 27)
#define S5P_HDMI_SPK3R_SEL_I_PCM1R		(3 << 27)
#define S5P_HDMI_SPK3R_SEL_I_PCM2L		(4 << 27)
#define S5P_HDMI_SPK3R_SEL_I_PCM2R		(5 << 27)
#define S5P_HDMI_SPK3R_SEL_I_PCM3L		(6 << 27)
#define S5P_HDMI_SPK3R_SEL_I_PCM3R		(7 << 27)
#define S5P_HDMI_SPK3L_SEL_I_PCM0L		(0 << 24)
#define S5P_HDMI_SPK3L_SEL_I_PCM0R		(1 << 24)
#define S5P_HDMI_SPK3L_SEL_I_PCM1L		(2 << 24)
#define S5P_HDMI_SPK3L_SEL_I_PCM1R		(3 << 24)
#define S5P_HDMI_SPK3L_SEL_I_PCM2L		(4 << 24)
#define S5P_HDMI_SPK3L_SEL_I_PCM2R		(5 << 24)
#define S5P_HDMI_SPK3L_SEL_I_PCM3L		(6 << 24)
#define S5P_HDMI_SPK3L_SEL_I_PCM3R		(7 << 24)
#define S5P_HDMI_SPK2R_SEL_I_PCM0L		(0 << 19)
#define S5P_HDMI_SPK2R_SEL_I_PCM0R		(1 << 19)
#define S5P_HDMI_SPK2R_SEL_I_PCM1L		(2 << 19)
#define S5P_HDMI_SPK2R_SEL_I_PCM1R		(3 << 19)
#define S5P_HDMI_SPK2R_SEL_I_PCM2L		(4 << 19)
#define S5P_HDMI_SPK2R_SEL_I_PCM2R		(5 << 19)
#define S5P_HDMI_SPK2R_SEL_I_PCM3L		(6 << 19)
#define S5P_HDMI_SPK2R_SEL_I_PCM3R		(7 << 19)
#define S5P_HDMI_SPK2L_SEL_I_PCM0L		(0 << 16)
#define S5P_HDMI_SPK2L_SEL_I_PCM0R		(1 << 16)
#define S5P_HDMI_SPK2L_SEL_I_PCM1L		(2 << 16)
#define S5P_HDMI_SPK2L_SEL_I_PCM1R		(3 << 16)
#define S5P_HDMI_SPK2L_SEL_I_PCM2L		(4 << 16)
#define S5P_HDMI_SPK2L_SEL_I_PCM2R		(5 << 16)
#define S5P_HDMI_SPK2L_SEL_I_PCM3L		(6 << 16)
#define S5P_HDMI_SPK2L_SEL_I_PCM3R		(7 << 16)
#define S5P_HDMI_SPK1R_SEL_I_PCM0L		(0 << 11)
#define S5P_HDMI_SPK1R_SEL_I_PCM0R		(1 << 11)
#define S5P_HDMI_SPK1R_SEL_I_PCM1L		(2 << 11)
#define S5P_HDMI_SPK1R_SEL_I_PCM1R		(3 << 11)
#define S5P_HDMI_SPK1R_SEL_I_PCM2L		(4 << 11)
#define S5P_HDMI_SPK1R_SEL_I_PCM2R		(5 << 11)
#define S5P_HDMI_SPK1R_SEL_I_PCM3L		(6 << 11)
#define S5P_HDMI_SPK1R_SEL_I_PCM3R		(7 << 11)
#define S5P_HDMI_SPK1L_SEL_I_PCM0L		(0 << 8)
#define S5P_HDMI_SPK1L_SEL_I_PCM0R		(1 << 8)
#define S5P_HDMI_SPK1L_SEL_I_PCM1L		(2 << 8)
#define S5P_HDMI_SPK1L_SEL_I_PCM1R		(3 << 8)
#define S5P_HDMI_SPK1L_SEL_I_PCM2L		(4 << 8)
#define S5P_HDMI_SPK1L_SEL_I_PCM2R		(5 << 8)
#define S5P_HDMI_SPK1L_SEL_I_PCM3L		(6 << 8)
#define S5P_HDMI_SPK1L_SEL_I_PCM3R		(7 << 8)
#define S5P_HDMI_SPK0R_SEL_I_PCM0L		(0 << 3)
#define S5P_HDMI_SPK0R_SEL_I_PCM0R		(1 << 3)
#define S5P_HDMI_SPK0R_SEL_I_PCM1L		(2 << 3)
#define S5P_HDMI_SPK0R_SEL_I_PCM1R		(3 << 3)
#define S5P_HDMI_SPK0R_SEL_I_PCM2L		(4 << 3)
#define S5P_HDMI_SPK0R_SEL_I_PCM2R		(5 << 3)
#define S5P_HDMI_SPK0R_SEL_I_PCM3L		(6 << 3)
#define S5P_HDMI_SPK0R_SEL_I_PCM3R		(7 << 3)
#define S5P_HDMI_SPK0L_SEL_I_PCM0L		(0)
#define S5P_HDMI_SPK0L_SEL_I_PCM0R		(1)
#define S5P_HDMI_SPK0L_SEL_I_PCM1L		(2)
#define S5P_HDMI_SPK0L_SEL_I_PCM1R		(3)
#define S5P_HDMI_SPK0L_SEL_I_PCM2L		(4)
#define S5P_HDMI_SPK0L_SEL_I_PCM2R		(5)
#define S5P_HDMI_SPK0L_SEL_I_PCM3L		(6)
#define S5P_HDMI_SPK0L_SEL_I_PCM3R		(7)

/* ACR_CON */
#define S5P_HDMI_ALT_CTS_RATE_CTS_1		(0 << 3)
#define S5P_HDMI_ALT_CTS_RATE_CTS_11		(1 << 3)
#define S5P_HDMI_ALT_CTS_RATE_CTS_21		(2 << 3)
#define S5P_HDMI_ALT_CTS_RATE_CTS_31		(3 << 3)
#define S5P_HDMI_ACR_TX_MODE_NO_TX		(0)
#define S5P_HDMI_ACR_TX_MODE_TX_ONCE		(1)
#define S5P_HDMI_ACR_TX_MODE_TXCNT_VBI		(2)
#define S5P_HDMI_ACR_TX_MODE_TX_VPC		(3)
#define S5P_HDMI_ACR_TX_MODE_MESURE_CTS		(4)

/* ACR_MCTS0/1/2 */
#define S5P_HDMI_SET_ACR_MCTS_0(x)		((x) & 0xFF)
#define S5P_HDMI_SET_ACR_MCTS_1(x)		(((x) >> 8) & 0xFF)
#define S5P_HDMI_SET_ACR_MCTS_2(x)		(((x) >> 16) & 0xFF)

/* ACR_CTS0/1/2 */
#define S5P_HDMI_SET_ACR_CTS_0(x)		((x) & 0xFF)
#define S5P_HDMI_SET_ACR_CTS_1(x)		(((x) >> 8) & 0xFF)
#define S5P_HDMI_SET_ACR_CTS_2(x)		(((x) >> 16) & 0xFF)

/* ACR_N0/1/2 */
#define S5P_HDMI_SET_ACR_N_0(x)			((x) & 0xFF)
#define S5P_HDMI_SET_ACR_N_1(x)			(((x) >> 8) & 0xFF)
#define S5P_HDMI_SET_ACR_N_2(x)			(((x) >> 16) & 0xFF)


/* ACR_LSB2 */
#define S5P_HDMI_ACR_LSB2_MASK			(0xFF)

/* ACR_TXCNT */
#define S5P_HDMI_ACR_TXCNT_MASK			(0x1F)

/* ACR_TXINTERNAL */
#define S5P_HDMI_ACR_TX_INTERNAL_MASK		(0xFF)

/* ACR_CTS_OFFSET */
#define S5P_HDMI_ACR_CTS_OFFSET_MASK		(0xFF)

/* GCP_CON */
#define S5P_HDMI_GCP_CON_EN_1ST_VSYNC		(1 << 3)
#define S5P_HDMI_GCP_CON_EN_2ST_VSYNC		(1 << 2)
#define S5P_HDMI_GCP_CON_TRANS_EVERY_VSYNC	(2)
#define S5P_HDMI_GCP_CON_NO_TRAN		(0)
#define S5P_HDMI_GCP_CON_TRANS_ONCE		(1)
#define S5P_HDMI_GCP_CON_TRANS_EVERY_VSYNC	(2)

/* GCP_BYTE1 */
#define S5P_HDMI_GCP_BYTE1_MASK			(0xFF)

/* GCP_BYTE2 */
#define S5P_HDMI_GCP_BYTE2_PP_MASK		(0xF << 4)
#define S5P_HDMI_GCP_BYTE2_CD_24BPP		(1 << 2)
#define S5P_HDMI_GCP_BYTE2_CD_30BPP		(1 << 0 | 1 << 2)		/*Not support in S5PV210 */ 
#define S5P_HDMI_GCP_BYTE2_CD_36BPP		(1 << 1 | 1 << 2)		/*Not support in S5PV210 */
#define S5P_HDMI_GCP_BYTE2_CD_48BPP		(1 << 0 | 1 << 1 | 1 << 2)	/*Not support in S5PV210 */


/* GCP_BYTE3 */
#define S5P_HDMI_GCP_BYTE3_MASK			(0xFF)


/* ACP Packet Register */

/* ACP_CON */
#define S5P_HDMI_ACP_FR_RATE_MASK		(0x1F << 3)
#define S5P_HDMI_ACP_CON_NO_TRAN		(0)
#define S5P_HDMI_ACP_CON_TRANS_ONCE		(1)
#define S5P_HDMI_ACP_CON_TRANS_EVERY_VSYNC	(2)

/* ACP_TYPE */
#define S5P_HDMI_ACP_TYPE_MASK			(0xFF)

/* ACP_DATA00~16 */
#define S5P_HDMI_ACP_DATA_MASK			(0xFF)


/* ISRC1/2 Packet Register */

/* ISRC_CON */
#define S5P_HDMI_ISRC_FR_RATE_MASK		(0x1F << 3)
#define S5P_HDMI_ISRC_EN			(1 << 2)
#define S5P_HDMI_ISRC_DIS			(0 << 2)


/* ISRC1_HEADER1 */
#define S5P_HDMI_ISRC1_HEADER_MASK		(0xFF)

/* ISRC1_DATA 00~15 */
#define S5P_HDMI_ISRC1_DATA_MASK		(0xFF)

/* ISRC2_DATA 00~15 */
#define S5P_HDMI_ISRC2_DATA_MASK		(0xFF)


/* AVI InfoFrame Register */

/* AVI_CON */

/* AVI_CHECK_SUM */
#define S5P_HDMI_SET_AVI_CHECK_SUM(x)		((x) & 0xFF)

/* AVI_DATA01~13 */
#define S5P_HDMI_SET_AVI_DATA(x)		((x) & 0xFF)
#define S5P_HDMI_AVI_PIXEL_REPETITION_DOUBLE	(1<<0)
#define S5P_HDMI_AVI_PICTURE_ASPECT_4_3		(1<<4)                            
#define S5P_HDMI_AVI_PICTURE_ASPECT_16_9	(1<<5)


/* Audio InfoFrame Register */

/* AUI_CON */

/* AUI_CHECK_SUM */
#define S5P_HDMI_SET_AUI_CHECK_SUM(x)		((x) & 0xFF)

/* AUI_DATA1~5 */
#define S5P_HDMI_SET_AUI_DATA(x)		((x) & 0xFF)


/* MPEG Source InfoFrame registers */

/* MPG_CON */

/* HDMI_MPG_CHECK_SUM */
#define S5P_HDMI_SET_MPG_CHECK_SUM(x)		((x) & 0xFF)

/* MPG_DATA1~5 */
#define S5P_HDMI_SET_MPG_DATA(x)		((x) & 0xFF)


/* Source Product Descriptor Infoframe registers */

/* SPD_CON */

/* SPD_HEADER0/1/2 */
#define S5P_HDMI_SET_SPD_HEADER(x)		((x) & 0xFF)


/* SPD_DATA0~27 */
#define S5P_HDMI_SET_SPD_DATA(x)		((x) & 0xFF)


/* HDCP Register */

/* HDCP_SHA1_00~19 */
#define S5P_HDMI_SET_HDCP_SHA1(x)		((x) & 0xFF)

/* HDCP_KSV_LIST_0~4 */

/* HDCP_KSV_LIST_CON */
#define S5P_HDMI_HDCP_KSV_WRITE_DONE		(0x1 << 3)
#define S5P_HDMI_HDCP_KSV_LIST_EMPTY		(0x1 << 2)
#define S5P_HDMI_HDCP_KSV_END			(0x1 << 1)
#define S5P_HDMI_HDCP_KSV_READ			(0x1 << 0)

/* HDCP_CTRL1 */
#define S5P_HDMI_HDCP_EN_PJ_EN			(1 << 4)
#define S5P_HDMI_HDCP_EN_PJ_DIS			(~(1 << 4))
#define S5P_HDMI_HDCP_SET_REPEATER_TIMEOUT	(1 << 2)
#define S5P_HDMI_HDCP_CLEAR_REPEATER_TIMEOUT	(~(1 << 2))
#define S5P_HDMI_HDCP_CP_DESIRED_EN		(1 << 1)
#define S5P_HDMI_HDCP_CP_DESIRED_DIS		(~(1 << 1))
#define S5P_HDMI_HDCP_ENABLE_1_1_FEATURE_EN	(1)
#define S5P_HDMI_HDCP_ENABLE_1_1_FEATURE_DIS	(~(1))

/* HDCP_CHECK_RESULT */
#define S5P_HDMI_HDCP_Pi_MATCH_RESULT_Y		((0x1 << 3) | (0x1 << 2))
#define S5P_HDMI_HDCP_Pi_MATCH_RESULT_N		((0x1 << 3) | (0x0 << 2))
#define S5P_HDMI_HDCP_Ri_MATCH_RESULT_Y		((0x1 << 1) | (0x1 << 0))
#define S5P_HDMI_HDCP_Ri_MATCH_RESULT_N		((0x1 << 1) | (0x0 << 0))
#define S5P_HDMI_HDCP_CLR_ALL_RESULTS		(0)

/* HDCP_BKSV0~4 */
/* HDCP_AKSV0~4 */

/* HDCP_BCAPS */
#define S5P_HDMI_HDCP_BCAPS_REPEATER		(1 << 6)
#define S5P_HDMI_HDCP_BCAPS_READY		(1 << 5)
#define S5P_HDMI_HDCP_BCAPS_FAST		(1 << 4)
#define S5P_HDMI_HDCP_BCAPS_1_1_FEATURES	(1 << 1)
#define S5P_HDMI_HDCP_BCAPS_FAST_REAUTH		(1)

/* HDCP_BSTATUS_0/1 */
/* HDCP_Ri_0/1 */
/* HDCP_I2C_INT */
/* HDCP_AN_INT */
/* HDCP_WATCHDOG_INT */
/* HDCP_RI_INT/1 */
/* HDCP_Ri_Compare_0 */
/* HDCP_Ri_Compare_1 */
/* HDCP_Frame_Count */


/* Gamut Metadata Packet Register */

/* GAMUT_CON */
/* GAMUT_HEADER0 */
/* GAMUT_HEADER1 */
/* GAMUT_HEADER2 */
/* GAMUT_METADATA0~27 */


/* Video Mode Register */

/* VIDEO_PATTERN_GEN */
/* HPD_GEN */
/* HDCP_Ri_Compare_0 */
/* HDCP_Ri_Compare_0 */
/* HDCP_Ri_Compare_0 */
/* HDCP_Ri_Compare_0 */
/* HDCP_Ri_Compare_0 */
/* HDCP_Ri_Compare_0 */
/* HDCP_Ri_Compare_0 */
/* HDCP_Ri_Compare_0 */
/* HDCP_Ri_Compare_0 */
/* HDCP_Ri_Compare_0 */


/* SPDIF Register */

/* SPDIFIN_CLK_CTRL */
#define S5P_HDMI_SPDIFIN_READY_CLK_DOWN		(1 << 1)
#define S5P_HDMI_SPDIFIN_CLK_ON			(1)

/* SPDIFIN_OP_CTRL */
#define S5P_HDMI_SPDIFIN_SW_RESET		(0)
#define S5P_HDMI_SPDIFIN_STATUS_CHECK_MODE	(1)
#define S5P_HDMI_SPDIFIN_STATUS_CHK_OP_MODE	(3)

/* SPDIFIN_IRQ_MASK */

/* SPDIFIN_IRQ_STATUS */
#define S5P_HDMI_SPDIFIN_IRQ_OVERFLOW_EN			(1 << 7)
#define S5P_HDMI_SPDIFIN_IRQ_ABNORMAL_PD_EN			(1 << 6)
#define S5P_HDMI_SPDIFIN_IRQ_SH_NOT_DETECTED_RIGHTTIME_EN 	(1 << 5)
#define S5P_HDMI_SPDIFIN_IRQ_SH_DETECTED_EN			(1 << 4)
#define S5P_HDMI_SPDIFIN_IRQ_SH_NOT_DETECTED_EN			(1 << 3)
#define S5P_HDMI_SPDIFIN_IRQ_WRONG_PREAMBLE_EN			(1 << 2)
#define S5P_HDMI_SPDIFIN_IRQ_CH_STATUS_RECOVERED_EN		(1 << 1)
#define S5P_HDMI_SPDIFIN_IRQ_WRONG_SIG_EN			(1 << 0)

/* SPDIFIN_CONFIG_1 */
#define S5P_HDMI_SPDIFIN_CFG_FILTER_3_SAMPLE			(0 << 6)
#define S5P_HDMI_SPDIFIN_CFG_FILTER_2_SAMPLE			(1 << 6)
#define S5P_HDMI_SPDIFIN_CFG_LINEAR_PCM_TYPE			(0 << 5)
#define S5P_HDMI_SPDIFIN_CFG_NO_LINEAR_PCM_TYPE			(1 << 5)
#define S5P_HDMI_SPDIFIN_CFG_PCPD_AUTO_SET			(0 << 4)
#define S5P_HDMI_SPDIFIN_CFG_PCPD_MANUAL_SET			(1 << 4)
#define S5P_HDMI_SPDIFIN_CFG_WORD_LENGTH_A_SET			(0 << 3)
#define S5P_HDMI_SPDIFIN_CFG_WORD_LENGTH_M_SET			(1 << 3)
#define S5P_HDMI_SPDIFIN_CFG_U_V_C_P_NEGLECT			(0 << 2)
#define S5P_HDMI_SPDIFIN_CFG_U_V_C_P_REPORT			(1 << 2)
#define S5P_HDMI_SPDIFIN_CFG_BURST_SIZE_1			(0 << 1)
#define S5P_HDMI_SPDIFIN_CFG_BURST_SIZE_2			(1 << 1)
#define S5P_HDMI_SPDIFIN_CFG_DATA_ALIGN_16BIT			(0 << 0)
#define S5P_HDMI_SPDIFIN_CFG_DATA_ALIGN_32BIT			(1 << 0)

/* SPDIFIN_CONFIG_2 */
#define S5P_HDMI_SPDIFIN_CFG2_NO_CLK_DIV			(0)

/* SPDIFIN_USER_VALUE_1 */
/* SPDIFIN_USER_VALUE_2 */
/* SPDIFIN_USER_VALUE_3 */
/* SPDIFIN_USER_VALUE_4 */
/* SPDIFIN_CH_STATUS_0_1 */
/* SPDIFIN_CH_STATUS_0_2 */
/* SPDIFIN_CH_STATUS_0_3 */
/* SPDIFIN_CH_STATUS_0_4 */
/* SPDIFIN_CH_STATUS_1 */
/* SPDIFIN_FRAME_PERIOD_1 */
/* SPDIFIN_FRAME_PERIOD_2 */
/* SPDIFIN_PC_INFO_1 */
/* SPDIFIN_PC_INFO_2 */
/* SPDIFIN_PD_INFO_1 */
/* SPDIFIN_PD_INFO_2 */
/* SPDIFIN_DATA_BUF_0_1 */
/* SPDIFIN_DATA_BUF_0_2 */
/* SPDIFIN_DATA_BUF_0_3 */
/* SPDIFIN_USER_BUF_0 */
/* SPDIFIN_USER_BUF_1_1 */
/* SPDIFIN_USER_BUF_1_2 */
/* SPDIFIN_USER_BUF_1_3 */
/* SPDIFIN_USER_BUF_1 */


/* I2S Register */

/* I2S_CLK_CON */
#define S5P_HDMI_I2S_CLK_DIS			(0)
#define S5P_HDMI_I2S_CLK_EN			(1)

/* I2S_CON_1 */
#define S5P_HDMI_I2S_SCLK_FALLING_EDGE		(0 << 1)
#define S5P_HDMI_I2S_SCLK_RISING_EDGE		(1 << 1)
#define S5P_HDMI_I2S_L_CH_LOW_POL		(0)
#define S5P_HDMI_I2S_L_CH_HIGH_POL		(1)

/* I2S_CON_2 */
#define S5P_HDMI_I2S_MSB_FIRST_MODE		(0 << 6)
#define S5P_HDMI_I2S_LSB_FIRST_MODE		(1 << 6)
#define S5P_HDMI_I2S_BIT_CH_32FS		(0 << 4)
#define S5P_HDMI_I2S_BIT_CH_48FS		(1 << 4)
#define S5P_HDMI_I2S_BIT_CH_RESERVED		(2 << 4)
#define S5P_HDMI_I2S_SDATA_16BIT		(1 << 2)
#define S5P_HDMI_I2S_SDATA_20BIT		(2 << 2)
#define S5P_HDMI_I2S_SDATA_24BIT		(3 << 2)
#define S5P_HDMI_I2S_BASIC_FORMAT		(0)
#define S5P_HDMI_I2S_L_JUST_FORMAT		(2)
#define S5P_HDMI_I2S_R_JUST_FORMAT		(3)
#define S5P_HDMI_I2S_CON_2_CLR			~(0xFF)
#define S5P_HDMI_I2S_SET_BIT_CH(x)		(((x) & 0x7) << 4)
#define S5P_HDMI_I2S_SET_SDATA_BIT(x)		(((x) & 0x7) << 2)

/* I2S_PIN_SEL_0 */
#define S5P_HDMI_I2S_SEL_SCLK(x)		(((x) & 0x7) << 4)
#define S5P_HDMI_I2S_SEL_SCLK_DEFAULT_1		(0x7 << 4)
#define S5P_HDMI_I2S_SEL_LRCK(x)		((x) & 0x7)
#define S5P_HDMI_I2S_SEL_LRCK_DEFAULT_0		(0x7)

/* I2S_PIN_SEL_1 */
#define S5P_HDMI_I2S_SEL_SDATA1(x)		(((x) & 0x7) << 4)
#define S5P_HDMI_I2S_SEL_SDATA1_DEFAULT_3	(0x7 << 4)
#define S5P_HDMI_I2S_SEL_SDATA2(x)		((x) & 0x7)
#define S5P_HDMI_I2S_SEL_SDATA2_DEFAULT_2	(0x7)

/* I2S_PIN_SEL_2 */
#define S5P_HDMI_I2S_SEL_SDATA3(x)		(((x) & 0x7) << 4)
#define S5P_HDMI_I2S_SEL_SDATA3_DEFAULT_5	(0x7 << 4)
#define S5P_HDMI_I2S_SEL_SDATA2(x)		((x) & 0x7)
#define S5P_HDMI_I2S_SEL_SDATA2_DEFAULT_4	(0x7)

/* I2S_PIN_SEL_3 */
#define S5P_HDMI_I2S_SEL_DSD(x)			((x) & 0x7)
#define S5P_HDMI_I2S_SEL_DSD_DEFAULT_6		(0x7)

/* I2S_DSD_CON */
#define S5P_HDMI_I2S_DSD_CLK_RI_EDGE		(1 << 1)
#define S5P_HDMI_I2S_DSD_CLK_FA_EDGE		(0 << 1)
#define S5P_HDMI_I2S_DSD_ENABLE			(1)
#define S5P_HDMI_I2S_DSD_DISABLE		(0)

/* I2S_MUX_CON */
#define S5P_HDMI_I2S_NOISE_FILTER_ZERO		(0 << 5)
#define S5P_HDMI_I2S_NOISE_FILTER_2_STAGE	(1 << 5)
#define S5P_HDMI_I2S_NOISE_FILTER_3_STAGE	(2 << 5)
#define S5P_HDMI_I2S_NOISE_FILTER_4_STAGE	(3 << 5)
#define S5P_HDMI_I2S_NOISE_FILTER_5_STAGE	(4 << 5)
#define S5P_HDMI_I2S_IN_DISABLE			(1 << 4)
#define S5P_HDMI_I2S_IN_ENABLE			(0 << 4)
#define S5P_HDMI_I2S_AUD_SPDIF			(0 << 2)
#define S5P_HDMI_I2S_AUD_I2S			(1 << 2)
#define S5P_HDMI_I2S_AUD_DSD			(2 << 2)
#define S5P_HDMI_I2S_CUV_SPDIF_ENABLE		(0 << 1)
#define S5P_HDMI_I2S_CUV_I2S_ENABLE		(1 << 1)
#define S5P_HDMI_I2S_MUX_DISABLE		(0)
#define S5P_HDMI_I2S_MUX_ENABLE			(1)
#define S5P_HDMI_I2S_MUX_CON_CLR		~(0xFF)

/* I2S_CH_ST_CON */
#define S5P_HDMI_I2S_CH_STATUS_RELOAD		(1)
#define S5P_HDMI_I2S_CH_ST_CON_CLR		~(1)

/* I2S_CH_ST_0 / I2S_CH_ST_SH_0 */
#define S5P_HDMI_I2S_CH_STATUS_MODE_0		(0 << 6)
#define S5P_HDMI_I2S_2AUD_CH_WITHOUT_PREEMPH	(0 << 3)
#define S5P_HDMI_I2S_2AUD_CH_WITH_PREEMPH	(1 << 3)
#define S5P_HDMI_I2S_DEFAULT_EMPHASIS		(0 << 3)
#define S5P_HDMI_I2S_COPYRIGHT			(0 << 2)
#define S5P_HDMI_I2S_NO_COPYRIGHT		(1 << 2)
#define S5P_HDMI_I2S_LINEAR_PCM			(0 << 1)
#define S5P_HDMI_I2S_NO_LINEAR_PCM		(1 << 1)
#define S5P_HDMI_I2S_CONSUMER_FORMAT		(0)
#define S5P_HDMI_I2S_PROF_FORMAT		(1)
#define S5P_HDMI_I2S_CH_ST_0_CLR		~(0xFF)

/* I2S_CH_ST_1 / I2S_CH_ST_SH_1 */
#define S5P_HDMI_I2S_CD_PLAYER			(0x00)
#define S5P_HDMI_I2S_DAT_PLAYER			(0x03)
#define S5P_HDMI_I2S_DCC_PLAYER			(0x43)
#define S5P_HDMI_I2S_MINI_DISC_PLAYER		(0x49)

/* I2S_CH_ST_2 / I2S_CH_ST_SH_2 */
#define S5P_HDMI_I2S_CHANNEL_NUM_MASK		(0xF << 4)
#define S5P_HDMI_I2S_SOURCE_NUM_MASK		(0xF)
#define S5P_HDMI_I2S_SET_CHANNEL_NUM(x)		((x) & (0xF) << 4)
#define S5P_HDMI_I2S_SET_SOURCE_NUM(x)		((x) & (0xF))

/* I2S_CH_ST_3 / I2S_CH_ST_SH_3 */
#define S5P_HDMI_I2S_CLK_ACCUR_LEVEL_1		(1 << 4)
#define S5P_HDMI_I2S_CLK_ACCUR_LEVEL_2		(0 << 4)
#define S5P_HDMI_I2S_CLK_ACCUR_LEVEL_3		(2 << 4)
#define S5P_HDMI_I2S_SAMPLING_FREQ_44_1		(0x0)
#define S5P_HDMI_I2S_SAMPLING_FREQ_48		(0x2)
#define S5P_HDMI_I2S_SAMPLING_FREQ_32		(0x3)
#define S5P_HDMI_I2S_SAMPLING_FREQ_96		(0xA)
#define S5P_HDMI_I2S_SET_SAMPLING_FREQ(x)	((x) & (0xF))

/* I2S_CH_ST_4 / I2S_CH_ST_SH_4 */
#define S5P_HDMI_I2S_ORG_SAMPLING_FREQ_44_1	(0xF << 4)
#define S5P_HDMI_I2S_ORG_SAMPLING_FREQ_88_2	(0x7 << 4)
#define S5P_HDMI_I2S_ORG_SAMPLING_FREQ_22_05	(0xB << 4)
#define S5P_HDMI_I2S_ORG_SAMPLING_FREQ_176_4	(0x3 << 4)
#define S5P_HDMI_I2S_WORD_LENGTH_NOT_DEFINE	(0x0 << 1)
#define S5P_HDMI_I2S_WORD_LENGTH_MAX24_20BITS	(0x1 << 1)
#define S5P_HDMI_I2S_WORD_LENGTH_MAX24_22BITS	(0x2 << 1)
#define S5P_HDMI_I2S_WORD_LENGTH_MAX24_23BITS	(0x4 << 1)
#define S5P_HDMI_I2S_WORD_LENGTH_MAX24_24BITS	(0x5 << 1)
#define S5P_HDMI_I2S_WORD_LENGTH_MAX24_21BITS	(0x6 << 1)
#define S5P_HDMI_I2S_WORD_LENGTH_MAX20_16BITS	(0x1 << 1)
#define S5P_HDMI_I2S_WORD_LENGTH_MAX20_18BITS	(0x2 << 1)
#define S5P_HDMI_I2S_WORD_LENGTH_MAX20_19BITS	(0x4 << 1)
#define S5P_HDMI_I2S_WORD_LENGTH_MAX20_20BITS	(0x5 << 1)
#define S5P_HDMI_I2S_WORD_LENGTH_MAX20_17BITS	(0x6 << 1)
#define S5P_HDMI_I2S_WORD_LENGTH_MAX_24BITS	(1)
#define S5P_HDMI_I2S_WORD_LENGTH_MAX_20BITS	(0)

/* I2S_VD_DATA */
#define S5P_HDMI_I2S_VD_AUD_SAMPLE_RELIABLE	(0)
#define S5P_HDMI_I2S_VD_AUD_SAMPLE_UNRELIABLE	(1)

/* I2S_MUX_CH */
#define S5P_HDMI_I2S_CH3_R_EN			(1 << 7)
#define S5P_HDMI_I2S_CH3_L_EN			(1 << 6)
#define S5P_HDMI_I2S_CH3_EN			(3 << 6)
#define S5P_HDMI_I2S_CH2_R_EN			(1 << 5)
#define S5P_HDMI_I2S_CH2_L_EN			(1 << 4)
#define S5P_HDMI_I2S_CH2_EN			(3 << 4)
#define S5P_HDMI_I2S_CH1_R_EN			(1 << 3)
#define S5P_HDMI_I2S_CH1_L_EN			(1 << 2)
#define S5P_HDMI_I2S_CH1_EN			(3 << 2)
#define S5P_HDMI_I2S_CH0_R_EN			(1 << 1)
#define S5P_HDMI_I2S_CH0_L_EN			(1)
#define S5P_HDMI_I2S_CH0_EN			(3)
#define S5P_HDMI_I2S_CH_ALL_EN			(0xFF)
#define S5P_HDMI_I2S_MUX_CH_CLR			~S5P_HDMI_I2S_CH_ALL_EN

/* I2S_MUX_CUV */
#define S5P_HDMI_I2S_CUV_R_EN			(1 << 1)
#define S5P_HDMI_I2S_CUV_L_EN			(1)
#define S5P_HDMI_I2S_CUV_RL_EN			(0x03)

/* I2S_IRQ_MASK */
#define S5P_HDMI_I2S_INT2_DIS			(0 << 1)
#define S5P_HDMI_I2S_INT2_EN			(1 << 1)

/* I2S_IRQ_STATUS */
#define S5P_HDMI_I2S_INT2_STATUS		(1 << 1)

/* I2S_CH0_L_0 */
/* I2S_CH0_L_1 */
/* I2S_CH0_L_2 */
/* I2S_CH0_L_3 */
/* I2S_CH0_R_0 */
/* I2S_CH0_R_1 */
/* I2S_CH0_R_2 */
/* I2S_CH0_R_3 */
/* I2S_CH1_L_0 */
/* I2S_CH1_L_1 */
/* I2S_CH1_L_2 */
/* I2S_CH1_L_3 */
/* I2S_CH1_R_0 */
/* I2S_CH1_R_1 */
/* I2S_CH1_R_2 */
/* I2S_CH1_R_3 */
/* I2S_CH2_L_0 */
/* I2S_CH2_L_1 */
/* I2S_CH2_L_2 */
/* I2S_CH2_L_3 */
/* I2S_CH2_R_0 */
/* I2S_CH2_R_1 */
/* I2S_CH2_R_2 */
/* I2S_Ch2_R_3 */
/* I2S_CH3_L_0 */
/* I2S_CH3_L_1 */
/* I2S_CH3_L_2 */
/* I2S_CH3_R_0 */
/* I2S_CH3_R_1 */
/* I2S_CH3_R_2 */

/* I2S_CUV_L_R */
#define S5P_HDMI_I2S_CUV_R_DATA_MASK		(0x7 << 4)
#define S5P_HDMI_I2S_CUV_L_DATA_MASK		(0x7)


/* Timing Generator Register */
/* TG_CMD */
#define S5P_HDMI_GETSYNC_TYPE_EN		(1 << 4)
#define S5P_HDMI_GETSYNC_TYPE_DIS		(~S5P_HDMI_GETSYNC_TYPE_EN)
#define S5P_HDMI_GETSYNC_EN			(1 << 3)
#define S5P_HDMI_GETSYNC_DIS			(~S5P_HDMI_GETSYNC_EN)
#define S5P_HDMI_FIELD_EN			(1 << 1)
#define S5P_HDMI_FIELD_DIS			(~S5P_HDMI_FIELD_EN)
#define S5P_HDMI_TG_EN				(1)
#define S5P_HDMI_TG_DIS				(~S5P_HDMI_TG_EN)

/* TG_CFG */
/* TG_CB_SZ */
/* TG_INDELAY_L */
/* TG_INDELAY_H */
/* TG_POL_CTRL */

/* TG_H_FSZ_L */
#define S5P_HDMI_SET_TG_H_FSZ_L(x)		((x) & 0xFF)

/* TG_H_FSZ_H */
#define S5P_HDMI_SET_TG_H_FSZ_H(x)		(((x) >> 8) & 0x1F)

/* TG_HACT_ST_L */
#define S5P_HDMI_SET_TG_HACT_ST_L(x)		((x) & 0xFF)

/* TG_HACT_ST_H */
#define S5P_HDMI_SET_TG_HACT_ST_H(x)		(((x) >> 8) & 0xF)

/* TG_HACT_SZ_L */
#define S5P_HDMI_SET_TG_HACT_SZ_L(x)		((x) & 0xFF)

/* TG_HACT_SZ_H */
#define S5P_HDMI_SET_TG_HACT_SZ_H(x)		(((x) >> 8) & 0xF)

/* TG_V_FSZ_L */
#define S5P_HDMI_SET_TG_V_FSZ_L(x)		((x) & 0xFF)

/* TG_V_FSZ_H */
#define S5P_HDMI_SET_TG_V_FSZ_H(x)		(((x) >> 8) & 0x7)

/* TG_VSYNC_L */
#define S5P_HDMI_SET_TG_VSYNC_L(x)		((x) & 0xFF)

/* TG_VSYNC_H */
#define S5P_HDMI_SET_TG_VSYNC_H(x)		(((x) >> 8) & 0x7)

/* TG_VSYNC2_L */
#define S5P_HDMI_SET_TG_VSYNC2_L(x)		((x) & 0xFF)

/* TG_VSYNC2_H */
#define S5P_HDMI_SET_TG_VSYNC2_H(x)		(((x) >> 8) & 0x7)

/* TG_VACT_ST_L */
#define S5P_HDMI_SET_TG_VACT_ST_L(x)		((x) & 0xFF)

/* TG_VACT_ST_H */
#define S5P_HDMI_SET_TG_VACT_ST_H(x)		(((x) >> 8) & 0x7)

/* TG_VACT_SZ_L */
#define S5P_HDMI_SET_TG_VACT_SZ_L(x)		((x) & 0xFF)

/* TG_VACT_SZ_H */
#define S5P_HDMI_SET_TG_VACT_SZ_H(x)		(((x) >> 8) & 0x7)

/* TG_FIELD_CHG_L */
#define S5P_HDMI_SET_TG_FIELD_CHG_L(x)		((x) & 0xFF)

/* TG_FIELD_CHG_H */
#define S5P_HDMI_SET_TG_FIELD_CHG_H(x)		(((x) >> 8) & 0x7)

/* TG_VACT_ST2_L */
#define S5P_HDMI_SET_TG_VACT_ST2_L(x)		((x) & 0xFF)

/* TG_VACT_ST2_H */
#define S5P_HDMI_SET_TG_VACT_ST2_H(x)		(((x) >> 8) & 0x7)

/* TG_VACT_SC_ST_L */
/* TG_VACT_SC_ST_H */
/* TG_VACT_SC_SZ_L */
/* TG_VACT_SC_SZ_H */

/* TG_VSYNC_TOP_HDMI_L */
#define S5P_HDMI_SET_TG_VSYNC_TOP_HDMI_L(x)	((x) & 0xFF)

/* TG_VSYNC_TOP_HDMI_H */
#define S5P_HDMI_SET_TG_VSYNC_TOP_HDMI_H(x)	(((x) >> 8) & 0x7)

/* TG_VSYNC_BOT_HDMI_L */
#define S5P_HDMI_SET_TG_VSYNC_BOT_HDMI_L(x)	((x) & 0xFF)

/* TG_VSYNC_BOT_HDMI_H */
#define S5P_HDMI_SET_TG_VSYNC_BOT_HDMI_H(x)	(((x) >> 8) & 0x7)

/* TG_FIELD_TOP_HDMI_L */
#define S5P_HDMI_SET_TG_FIELD_TOP_HDMI_L(x)	((x) & 0xFF)

/* TG_FIELD_TOP_HDMI_H */
#define S5P_HDMI_SET_TG_FIELD_TOP_HDMI_H(x)	(((x) >> 8) & 0x7)

/* TG_FIELD_BOT_HDMI_L */
#define S5P_HDMI_SET_TG_FIELD_BOT_HDMI_L(x)	((x) & 0xFF)

/* TG_FIELD_BOT_HDMI_H */
#define S5P_HDMI_SET_TG_FIELD_BOT_HDMI_H(x)	(((x) >> 8) & 0x7)

/* TG_HSYNC_HDOUT_ST_L */
/* TG_HSYNC_HDOUT_ST_H */
/* TG_HSYNC_HDOUT_END_L */
/* TG_HSYNC_HDOUT_END_H */
/* TG_VSYNC_HDOUT_ST_L */
/* TG_VSYNC_HDOUT_ST_H */
/* TG_VSYNC_HDOUT_END_L */
/* TG_VSYNC_HDOUT_END_H */
/* TG_VSYNC_HDOUT_DLY_L */
/* TG_VSYNC_HDOUT_DLY_H */
/* TG_BT_ERR_RANGE */
/* TG_BT_ERR_RESULT */
/* TG_COR_THR */
/* TG_COR_NUM */
/* TG_BT_CON */
/* TG_BT_H_FSZ_L */
/* TG_BT_H_FSZ_H */
/* TG_BT_HSYNC_ST */
/* TG_BT_HSYNC_SZ */
/* TG_BT_FSZ_L */
/* TG_BT_FSZ_H */
/* TG_BT_VACT_T_ST_L */
/* TG_BT_VACT_T_ST_H */
/* TG_BT_VACT_B_ST_L */
/* TG_BT_VACT_B_ST_H */
/* TG_BT_VACT_SZ_L */
/* TG_BT_VACT_SZ_H */
/* TG_BT_VSYNC_SZ */


/* HDCP E-FUSE Control Register */
/* HDCP_E_FUSE_CTRL */
#define S5P_HDMI_EFUSE_CTRL_HDCP_KEY_READ	(1)

/* HDCP_E_FUSE_STATUS */
#define S5P_HDMI_EFUSE_ECC_FAIL			(1 << 2)
#define S5P_HDMI_EFUSE_ECC_BUSY			(1 << 1)
#define S5P_HDMI_EFUSE_ECC_DONE			(1)

/* EFUSE_ADDR_WIDTH */
/* EFUSE_SIGDEV_ASSERT */
/* EFUSE_SIGDEV_DE-ASSERT */
/* EFUSE_PRCHG_ASSERT */
/* EFUSE_PRCHG_DE-ASSERT */
/* EFUSE_FSET_ASSERT */
/* EFUSE_FSET_DE-ASSERT */
/* EFUSE_SENSING */
/* EFUSE_SCK_ASSERT */
/* EFUSE_SCK_DEASSERT */
/* EFUSE_SDOUT_OFFSET */
/* EFUSE_READ_OFFSET */

/* HDCP_SHA_RESULT */								/* Not support in s5pv210 */
#define S5P_HDMI_HDCP_SHA_VALID_NO_RD		(0 << 1)			/* Not support in s5pv210 */
#define S5P_HDMI_HDCP_SHA_VALID_RD		(1 << 1)			/* Not support in s5pv210 */
#define S5P_HDMI_HDCP_SHA_VALID			(1)				/* Not support in s5pv210 */
#define S5P_HDMI_HDCP_SHA_NO_VALID		(0)				/* Not support in s5pv210 */

/* DC_CONTRAL */
#define S5P_HDMI_DC_CTL_12			(1 << 1)			/*Not support in S5PV210 */
#define S5P_HDMI_DC_CTL_8 			(0)				/*Not support in S5PV210 */
#define S5P_HDMI_DC_CTL_10			(1)				/*Not support in S5PV210 */
#endif // __ASM_ARCH_REGS_HDMI_H
