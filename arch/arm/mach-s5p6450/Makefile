# arch/arm/mach-s5p6450/Makefile
#
# Copyright (c) 2009 Samsung Electronics Co., Ltd.
# 		http://www.samsung.com/
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:=
obj-				:=

# Core support for S5P6450 system

obj-$(CONFIG_CPU_S5P6450)	+= cpu.o init.o clock.o dma.o
obj-$(CONFIG_CPU_S5P6450)	+= setup-i2c0.o gpio.o irq-eint.o time.o
obj-$(CONFIG_CPU_S5P6450)	+= bootmem-smdk6450.o

# machine support

obj-$(CONFIG_MACH_SMDK6450)	+= mach-smdk6450.o button-smdk6450.o

# device support

obj-$(CONFIG_FB_S3C_V2)			+= dev-fb.o
obj-$(CONFIG_S3C64XX_DEV_SPI)		+= dev-spi.o

obj-$(CONFIG_S5P6450_SETUP_FB_S3C)	+= setup-fb.o
obj-$(CONFIG_S5P6450_SETUP_I2C1)	+= setup-i2c1.o
obj-$(CONFIG_S5P6450_SETUP_SDHCI)	+= setup-sdhci.o
obj-$(CONFIG_S5P6450_SETUP_SDHCI_GPIO)	+= setup-sdhci-gpio.o
obj-$(CONFIG_S5P6450_SETUP_MSHCI)	+= setup-mshci.o
obj-$(CONFIG_S5P6450_SETUP_MSHCI_GPIO)	+= setup-mshci-gpio.o
obj-$(CONFIG_CPU_FREQ_S5P6450)	+= cpu-freq.o

obj-$(CONFIG_S5P6450_SETUP_FIMC0)	+= setup-fimc0.o
# PM

obj-$(CONFIG_PM)        += pm.o
obj-$(CONFIG_PM)        += sleep.o
obj-$(CONFIG_PM)        += irq-pm.o

obj-y			+= dev-audio.o
