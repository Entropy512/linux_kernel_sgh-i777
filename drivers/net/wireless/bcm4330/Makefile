EXTRA_CFLAGS += -Dlinux -DLINUX -DBDC -DBCMDRIVER -DBCMDONGLEHOST -DDHDTHREAD \
    -DDHD_GPL -DDHD_SCHED -DBCMSDIO -DBCMLXSDMMC -DBCMPLATFORM_BUS -DDHD_BCMEVENTS \
    -DSHOW_EVENTS -DENABLE_DEEP_SLEEP

################ OPTIONAL FEATURES ############################################################
#TO ENALBLE OPTIONAL FEATURES UNCOMMENT THE CORRESPONDING FLAGS

# For Samsung Aries only.
EXTRA_CFLAGS += -DCUSTOMER_HW_SAMSUNG
EXTRA_CFLAGS += -DSYSLSI_SPECIFIC

# For U1
EXTRA_CFLAGS += -DUSE_WIFI_DIRECT -DDTIM_CNT1

# Print out CHIP REV
EXTRA_CFLAGS += -DCHECK_CHIP_REV

# For CID check
EXTRA_CFLAGS += -DUSE_CID_CHECK

# For MACID management
ifdef CONFIG_TARGET_LOCALE_KOR
EXTRA_CFLAGS += -DRDWR_MACADDR
EXTRA_CFLAGS += -DROAM_ENABLE
EXTRA_CFLAGS += -DOKC_SUPPORT
else
ifdef CONFIG_TARGET_LOCALE_NA
EXTRA_CFLAGS += -DRDWR_MACADDR
EXTRA_CFLAGS += -DUSE_HIDDEN_SSID
EXTRA_CFLAGS += -DUSE_NA_HOTSPOT
EXTRA_CFLAGS += -DDHD_DEBUG_MEM_DUMP
else
EXTRA_CFLAGS += -DWRITE_MACADDR
endif
endif
#EXTRA_CFLAGS += -DREAD_MACADDR

# SDIO_ISR_THREAD is mutually exclusive with OOB_INTR_ONLY
#EXTRA_CFLAGS += -DSDIO_ISR_THREAD

# For OOB only
EXTRA_CFLAGS += -DOOB_INTR_ONLY

# For HW_OOB
#EXTRA_CFLAGS += -DHW_OOB

# For SOFTAP
EXTRA_CFLAGS += -DSOFTAP

# For Debug
EXTRA_CFLAGS += -DDHD_DEBUG -DSRCBASE=\"$(src)/src\"

# HOST WAKEUP
EXTRA_CFLAGS += -DBCM_HOSTWAKE

#STATIC MEMORY ALLOCATION FEATURE
EXTRA_CFLAGS += -DDHD_USE_STATIC_BUF

#Disable PowerSave mode for OTA or certification test
#EXTRA_CFLAGS += -DBCMDISABLE_PM
EXTRA_CFLAGS += -DCONFIG_CONTROL_PM

# P2P ENABLE
#EXTRA_CFLAGS += -DWLP2P

# BTAMP
EXTRA_CFLAGS += -DWLBTAMP
EXTRA_CFLAGS += -DDHD_BTAMP_DBG

# COMBO SCAN
EXTRA_CFLAGS += -DCSCAN

# For Keepalive
EXTRA_CFLAGS += -DUSE_KEEP_ALIVE

# Packet Filter
EXTRA_CFLAGS += -DPKT_FILTER_SUPPORT

# Roam API
EXTRA_CFLAGS += -DROAM_API

#2.4G only scan for the initial broad scan
EXTRA_CFLAGS += -DUSE_INITIAL_2G_SCAN

#MobileAP fw_reload
EXTRA_CFLAGS += -DMOBILEAP_RELOAD

ifeq ($(CONFIG_TARGET_LOCALE_NAATT),y)
#Hidden SSID
EXTRA_CFLAGS += -DUSE_HIDDEN_SSID
endif


###############################################################################################

EXTRA_CFLAGS += -I$(src)/src/include/
EXTRA_CFLAGS += -I$(src)/src/dhd/sys/
EXTRA_CFLAGS += -I$(src)/src/dongle/
EXTRA_CFLAGS += -I$(src)/src/bcmsdio/sys/
EXTRA_CFLAGS += -I$(src)/src/wl/sys/
EXTRA_CFLAGS += -I$(src)/src/shared/

EXTRA_LDFLAGS += --strip-debug

KBUILD_CFLAGS += -I$(LINUXDIR)/include -I$(shell pwd)

#obj-$(CONFIG_BROADCOM_WIFI)-m	+= dhd.o
obj-m	+= dhd.o

dhd-y := src/dhd/sys/dhd_linux.o \
         src/dhd/sys/dhd_common.o \
         src/dhd/sys/dhd_cdc.o \
         src/dhd/sys/dhd_linux_sched.o\
         src/dhd/sys/dhd_sdio.o \
         src/dhd/sys/dhd_custom_gpio.o \
         src/dhd/sys/dhd_custom_sec.o \
         src/dhd/sys/dhd_bta.o \
         src/shared/aiutils.o \
         src/shared/bcmutils.o \
         src/shared/bcmwifi.o \
         src/shared/hndpmu.o \
         src/shared/linux_osl.o \
         src/shared/sbutils.o \
         src/shared/siutils.o \
         src/shared/bcmevent.o \
         src/bcmsdio/sys/bcmsdh_sdmmc.o \
         src/bcmsdio/sys/bcmsdh.o \
         src/bcmsdio/sys/bcmsdh_linux.o \
         src/bcmsdio/sys/bcmsdh_sdmmc_linux.o \
         src/wl/sys/wl_iw.o
         
all:
	@echo "$(MAKE) --no-print-directory -C $(KDIR) SUBDIRS=$(CURDIR) modules"
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules

clean: 
	rm -rf *.o *.ko *.mod.c *~ .*.cmd Module.symvers modules.order .tmp_versions	\
	 	 src/dhd/sys/dhd_linux.o \
         src/dhd/sys/dhd_common.o \
         src/dhd/sys/dhd_cdc.o \
         src/dhd/sys/dhd_linux_sched.o\
         src/dhd/sys/dhd_sdio.o \
         src/dhd/sys/dhd_custom_gpio.o \
         src/dhd/sys/dhd_custom_sec.o \
         src/dhd/sys/dhd_bta.o \
         src/shared/aiutils.o \
         src/shared/bcmutils.o \
         src/shared/bcmwifi.o \
         src/shared/hndpmu.o \
         src/shared/linux_osl.o \
         src/shared/sbutils.o \
         src/shared/siutils.o \
         src/shared/bcmevent.o \
         src/bcmsdio/sys/bcmsdh_sdmmc.o \
         src/bcmsdio/sys/bcmsdh.o \
         src/bcmsdio/sys/bcmsdh_linux.o \
         src/bcmsdio/sys/bcmsdh_sdmmc_linux.o \
         src/wl/sys/wl_iw.o \
         src/dhd/sys/.dhd_linux.o.cmd \
         src/dhd/sys/.dhd_common.o.cmd \
         src/dhd/sys/.dhd_cdc.o.cmd \
         src/dhd/sys/.dhd_linux_sched.o.cmd \
         src/dhd/sys/.dhd_sdio.o.cmd \
         src/dhd/sys/.dhd_custom_gpio.o.cmd \
         src/dhd/sys/.dhd_custom_sec.o.cmd \
         src/dhd/sys/.dhd_bta.o.cmd \
         src/shared/.aiutils.o.cmd \
         src/shared/.bcmutils.o.cmd \
         src/shared/.bcmwifi.o.cmd \
         src/shared/.hndpmu.o.cmd \
         src/shared/.linux_osl.o.cmd \
         src/shared/.sbutils.o.cmd \
         src/shared/.siutils.o.cmd \
         src/shared/.bcmevent.o.cmd \
         src/bcmsdio/sys/.bcmsdh_sdmmc.o.cmd \
         src/bcmsdio/sys/.bcmsdh.o.cmd \
         src/bcmsdio/sys/.bcmsdh_linux.o.cmd \
         src/bcmsdio/sys/.bcmsdh_sdmmc_linux.o.cmd \
         src/wl/sys/.wl_iw.o.cmd

install:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules_install
