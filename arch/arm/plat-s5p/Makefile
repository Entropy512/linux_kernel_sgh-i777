# arch/arm/plat-s5p/Makefile
#
# Copyright (c) 2009 Samsung Electronics Co., Ltd.
# 		http://www.samsung.com/
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:= dummy.o
obj-				:=

# Core files

obj-y				+= dev-uart.o
obj-y				+= cpu.o
obj-y				+= clock.o
obj-y				+= devs.o
obj-y				+= irq.o
obj-$(CONFIG_S5P_MEM_BOOTMEM)	+= bootmem.o
obj-$(CONFIG_S5P_EXT_INT)	+= irq-eint.o
obj-$(CONFIG_PM)		+= pm.o
obj-$(CONFIG_PM)		+= irq-pm.o

obj-$(CONFIG_PM)		+= pm.o

obj-$(CONFIG_S5P_DEV_MFC)	+= dev-mfc.o

obj-$(CONFIG_S5P_SETUP_MFC)	+= setup-mfc.o
obj-$(CONFIG_S5P_SYSMMU)	+= sysmmu.o
obj-$(CONFIG_VCM_MMU)		+= s5p-vcm.o
obj-$(CONFIG_DEV_THERMAL)	+= dev-tmu.o

obj-$(CONFIG_FB_S3C_MIPI_LCD)	+= dev-dsim.o
obj-$(CONFIG_FB_S3C_MIPI_LCD)	+= setup-dsim.o