#
# Makefile for the s3c framebuffer driver
#
ifeq ($(CONFIG_FB_S3C),y)
obj-y				+= s3cfb.o
s3cfb-y				:= s3cfb_main.o s3cfb_ops.o
obj-$(CONFIG_ARCH_S5PV210)	+= s3cfb_fimd6x.o
obj-$(CONFIG_ARCH_S5PV310)	+= s3cfb_fimd6x.o

obj-$(CONFIG_FB_S3C_LTE480WV)	+= s3cfb_lte480wv.o
obj-$(CONFIG_FB_S3C_HT101HD1)	+= s3cfb_ht101hd1.o
obj-$(CONFIG_FB_S3C_WA101S)	+= s3cfb_wa101s.o
obj-$(CONFIG_FB_S3C_AMS369FG06)	+= s3cfb_ams369fg06.o
obj-$(CONFIG_FB_S3C_TL2796)	+= s3cfb_tl2796.o
obj-$(CONFIG_FB_S3C_MDNIE)      += s3cfb_mdnie.o s3cfb_ielcd.o
obj-$(CONFIG_FB_S3C_LD9040)	+= ld9040.o
obj-$(CONFIG_FB_S3C_S6E63M0)	+= s3cfb_s6e63m0.o
obj-$(CONFIG_FB_S3C_S6E8AA0)	+= s3cfb_s6e8aa0.o
obj-$(CONFIG_FB_S3C_S6D6AA0)	+= s3cfb_s6d6aa0.o
obj-$(CONFIG_FB_S3C_MIPI_LCD)   += s5p_dsim_lowlevel.o
obj-$(CONFIG_FB_S3C_MIPI_LCD)   += s5p-dsim.o
endif


ifeq ($(CONFIG_FB_S3C_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

# version 2 rules
ifeq ($(CONFIG_FB_S3C_V2),y)
obj-y					+= s3cfb2.o
obj-$(CONFIG_PLAT_S3C64XX)		+= s3cfb2_fimd4x.o
obj-$(CONFIG_PLAT_S5P64XX)		+= s3cfb2_fimd5x.o
obj-$(CONFIG_CPU_S5P6450)              += s3cfb2_fimd5x.o
obj-$(CONFIG_CPU_S5PC100)		+= s3cfb2_fimd5x.o
obj-$(CONFIG_CPU_S5PC110)		+= s3cfb2_fimd6x.o
obj-$(CONFIG_FB_S3C_V2_LTE480WV)	+= s3cfb2_lte480wv.o
obj-$(CONFIG_FB_S3C_V2_TL2796)		+= s3cfb2_tl2796.o
endif

ifeq ($(CONFIG_FB_S3C_V2_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

ifeq ($(CONFIG_SUPPORT_UMP),y)
EXTRA_CFLAGS += -DMALI_USE_UNIFIED_MEMORY_PROVIDER=1 -Idrivers/media/video/samsung/ump/include
else
EXTRA_CFLAGS += -DMALI_USE_UNIFIED_MEMORY_PROVIDER=0
endif
