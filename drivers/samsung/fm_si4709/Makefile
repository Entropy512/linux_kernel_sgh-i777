##############################################################################
# COPYRIGHT(C)	: Samsung Electronics Co.Ltd, 2006-2011 ALL RIGHTS RESERVED
# AUTHOR		: Varun Mahajan <m.varun@samsung.com>
##############################################################################
# VERSION&DATE	: Version 0.1
##############################################################################

ifneq ($(KERNELRELEASE), )

MOD_NAME = Si4709_driver
obj-$(CONFIG_FM_SI4709) := $(MOD_NAME).o

EXTRA_CFLAGS := -I$(PRJROOT)/modules/include

$(MOD_NAME)-y := Si4709_dev.o \
				Si4709_i2c_drv.o \
				Si4709_main.o 

else

ifndef KDIR
	KDIR :=$(PRJROOT)/linux-2.6.29
endif 

all:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules

clean:
	rm -f *.o *.ko *.mod.c *~ .*.cmd

install:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules_install

endif
