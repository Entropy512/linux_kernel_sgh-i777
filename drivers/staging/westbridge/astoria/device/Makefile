#
# Makefile for the kernel westbridge device driver
#




ifneq ($(CONFIG_WESTBRIDGE_DEBUG),y)
	EXTRA_CFLAGS		+= -DWESTBRIDGE_NDEBUG
endif

#EXTRA_CFLAGS		+= -DWESTBRIDGE_ANTIOCH_CSP
#EXTRA_CFLAGS		+= -DWESTBRIDGE_ANTIOCH 
EXTRA_CFLAGS		+= -DWESTBRIDGE_ASTORIA
EXTRA_CFLAGS		+= -D__USE_CYAS_AUTO_SUSPEND__

obj-$(CONFIG_WESTBRIDGE_DEVICE_DRIVER)  += cyasdev.o

ifeq ($(CONFIG_MACH_OMAP3_WESTBRIDGE_AST_PNAND_HAL),y)
#moved for staging compatbility
#cyasdev-y		:= ../../../arch/arm/mach-omap2/cyashalomap_kernel.o cyasdevice.o
cyasdev-y		:= ../arch/arm/mach-omap2/cyashalomap_pnand.o cyasdevice.o \
			   ../api/src/cyasdma.o ../api/src/cyasintr.o ../api/src/cyaslep2pep.o \
			   ../api/src/cyaslowlevel.o ../api/src/cyasmisc.o ../api/src/cyasmtp.o \
			   ../api/src/cyasstorage.o ../api/src/cyasusb.o  ./cyasusbinit.o \
			   ../block/cyasblkdev_block.o ../block/cyasblkdev_queue.o ../gadget/cyasgadget.o
endif

ifeq ($(CONFIG_MACH_OMAP3_WESTBRIDGE_AST_CRAM_HAL),y)
cyasdev-y		:= ../arch/arm/mach-omap2/cyashalomap_cram.o cyasdevice.o \
			   ../api/src/cyasdma.o ../api/src/cyasintr.o ../api/src/cyaslep2pep.o \
			   ../api/src/cyaslowlevel.o ../api/src/cyasmisc.o ../api/src/cyasmtp.o \
			   ../api/src/cyasstorage.o ../api/src/cyasusb.o  ./cyasusbinit.o \
			   ../block/cyasblkdev_block.o ../block/cyasblkdev_queue.o ../gadget/cyasgadget.o
endif

ifeq ($(CONFIG_MACH_QC_WESTBRIDGE_AST_HAL),y)
#moved for staging compatbility
#cyasdev-y		:= ../../../arch/arm/mach-omap2/cyashalomap_kernel.o cyasdevice.o
cyasdev-y		:= ../arch/arm/mach-qc/cyashalqc_kernel.o cyasdevice.o \
			   ../api/src/cyasdma.o ../api/src/cyasintr.o ../api/src/cyaslep2pep.o \
			   ../api/src/cyaslowlevel.o ../api/src/cyasmisc.o ../api/src/cyasmtp.o \
			   ../api/src/cyasstorage.o ../api/src/cyasusb.o  ./cyasusbinit.o \
			   ../block/cyasblkdev_block.o ../block/cyasblkdev_queue.o ../gadget/cyasgadget.o cyasdiagnostics.o

endif

ifeq ($(CONFIG_MACH_C110_WESTBRIDGE_AST_CRAM_HAL),y)
cyasdev-y		:= ../arch/arm/mach-c110/cyashalc110_cram.o cyasdevice.o \
			   ../api/src/cyasdma.o ../api/src/cyasintr.o ../api/src/cyaslep2pep.o \
			   ../api/src/cyaslowlevel.o ../api/src/cyasmisc.o ../api/src/cyasmtp.o \
			   ../api/src/cyasstorage.o ../api/src/cyasusb.o  ./cyasusbinit.o \
			   ../block/cyasblkdev_block.o ../block/cyasblkdev_queue.o ../gadget/cyasgadget.o
endif

ifeq ($(CONFIG_MACH_C110_WESTBRIDGE_AST_PNAND_HAL),y)
cyasdev-y		:= ../arch/arm/mach-c110/cyashalc110_pnand.o cyasdevice.o \
			   ../api/src/cyasdma.o ../api/src/cyasintr.o ../api/src/cyaslep2pep.o \
			   ../api/src/cyaslowlevel.o ../api/src/cyasmisc.o \
			   ../api/src/cyasstorage.o  \
			   ../block/cyasblkdev_block.o ../block/cyasblkdev_queue.o cyasdiagnostics.o
endif

