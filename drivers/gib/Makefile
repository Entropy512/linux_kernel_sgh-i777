#
# Makefile for kernel GIB drivers.
#

ifeq ($(CONFIG_GIB_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

# GIB protocol drivers (device/link on bus)
ifeq ($(CONFIG_MACH_SMDK6450),y)
obj-$(CONFIG_GIB_S5P64XX)	+= gib-dev.o s3c-gib.o
endif


ifeq ($(CONFIG_MACH_SMDKV310),y)
obj-$(CONFIG_GIB_S5PV3XX)	+= gib-dev.o s5p-gib.o
endif

ifeq ($(CONFIG_MACH_C1),y)
obj-$(CONFIG_GIB_S5PV3XX)	+= gib-dev.o s5p-gib.o
endif
