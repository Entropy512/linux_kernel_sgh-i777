#
# Makefile for the OneNAND MTD
#

# Core functionality.
obj-$(CONFIG_MTD_ONENAND)		+= onenand.o

# Board specific.
obj-$(CONFIG_MTD_ONENAND_GENERIC)	+= generic.o
obj-$(CONFIG_MTD_ONENAND_OMAP2)		+= omap2.o
obj-$(CONFIG_MTD_ONENAND_SAMSUNG)       += samsung.o

# Simulator
obj-$(CONFIG_MTD_ONENAND_SIM)		+= onenand_sim.o

ifeq ($(CONFIG_MTD_ONENAND_SAMSUNG_AUDI),y)
onenand-objs = samsung_audi.o
else
onenand-objs = onenand_base.o
endif
onenand-objs += onenand_bbt.o
