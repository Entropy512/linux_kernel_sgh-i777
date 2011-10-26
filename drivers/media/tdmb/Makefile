# 
# Makefile for TDMB Device Drivers.
#

EXTRA_CFLAGS += -Idrivers/media/tdmb
ifeq ($(CONFIG_TDMB_T3900),y)
EXTRA_CFLAGS += -Idrivers/media/tdmb/t3900
endif
ifeq ($(CONFIG_TDMB_T3700),y)
EXTRA_CFLAGS += -Idrivers/media/tdmb/t3700
endif
ifeq ($(CONFIG_TDMB_FC8050),y)
EXTRA_CFLAGS += -Idrivers/media/tdmb/fc8050
endif

EXTRA_CFLAGS += -v

obj-$(CONFIG_TDMB) += tdmb_module.o

tdmb_module-y := tdmb.o tdmb_drv.o
ifeq ($(CONFIG_TDMB_T3900),y)
tdmb_module-y += t3900/INC_FICDEC.o t3900/INC_INTERFACE.o t3900/INC_PROCESS.o t3900/INC_RF_CTRL500.o
endif
ifeq ($(CONFIG_TDMB_T3700),y)
tdmb_module-y += t3700/INC_DAB_DLS.o t3700/INC_FICDEC.o t3700/INC_INTERFACE.o t3700/INC_PROCESS.o t3700/INC_RF_CTRL500.o
endif
ifeq ($(CONFIG_TDMB_FC8050),y)
tdmb_module-y += fc8050/bbm.o fc8050/DMBDrv_wrap_FC8050.o fc8050/fc8050_bb.o fc8050/fc8050_hpi.o fc8050/fc8050_i2c.o fc8050/fc8050_isr.o fc8050/fc8050_ppi.o 
tdmb_module-y += fc8050/fc8050_spi.o fc8050/fc8050_tun.o fc8050/fci_hal.o fc8050/fci_i2c.o fc8050/fci_oal.o fc8050/fci_tun.o fc8050/fic.o fc8050/ficdecoder.o
endif

