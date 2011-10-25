obj-$(CONFIG_VIDEO_FIMC)	+= fimc_dev.o fimc_v4l2.o fimc_capture.o fimc_output.o fimc_overlay.o fimc_regs.o
obj-$(CONFIG_VIDEO_FIMC_MIPI)	+= csis.o
obj-$(CONFIG_CPU_S5PV210)	+= ipc.o

ifeq ($(CONFIG_CPU_S5PV210),y)
EXTRA_CFLAGS += -DCONFIG_MIPI_CSI_ADV_FEATURE
endif

ifeq ($(CONFIG_CPU_S5PV310),y)
EXTRA_CFLAGS += -DCONFIG_MIPI_CSI_ADV_FEATURE
endif

EXTRA_CFLAGS += -Idrivers/media/video

ifeq ($(CONFIG_VIDEO_FIMC_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif
ifeq ($(CONFIG_VIDEO_FIMC_UMP_VCM_CMA),y)
EXTRA_CFLAGS += -Idrivers/media/video/samsung/ump/include -Idrivers/media/video/samsung/ump/common
endif

ifeq ($(CONFIG_S5P_SYSMMU_FIMC0),y)
EXTRA_CFLAGS += -DSYSMMU_FIMC
endif
ifeq ($(CONFIG_S5P_SYSMMU_FIMC1),y)
EXTRA_CFLAGS += -DSYSMMU_FIMC
endif
ifeq ($(CONFIG_S5P_SYSMMU_FIMC2),y)
EXTRA_CFLAGS += -DSYSMMU_FIMC
endif
ifeq ($(CONFIG_S5P_SYSMMU_FIMC3),y)
EXTRA_CFLAGS += -DSYSMMU_FIMC
endif
