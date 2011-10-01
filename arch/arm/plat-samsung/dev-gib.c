
#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>

#include <mach/irqs.h>
#include <mach/map.h>

#include <plat/regs-iic.h>
#include <plat/gib.h>
#include <plat/devs.h>
#include <plat/cpu.h>

static struct resource s3c_gib_resource[] = {
	[0] = {
		.start = S5P_PA_GPS,
		.end   = S5P_PA_GPS + S5P_SZ_GPS,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_GPS,
		.end   = IRQ_GPS,
		.flags = IORESOURCE_IRQ,
	},
};


static u64 s3c_device_gib_dmamask = 0xffffffffUL;

struct platform_device s3c_device_gib = {
	.name		  = "s3c-gib",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(s3c_gib_resource),
	.resource	  = s3c_gib_resource,
	.dev              = {
                .dma_mask = &s3c_device_gib_dmamask,
                .coherent_dma_mask = 0xffffffffUL
        }
};
EXPORT_SYMBOL(s3c_device_gib);


static struct s3c_gib_platdata default_gib_data __initdata = {
	.flags		= 0,
	.slave_addr	= 0x10,
	.frequency	= 100*1000,
	.sda_delay	= 100,
};

void __init s3c_gib_set_platdata(struct s3c_gib_platdata *pd)
{
	struct s3c_gib_platdata *npd;

	if (!pd)
		pd = &default_gib_data;

	npd = kmemdup(pd, sizeof(struct s3c_gib_platdata), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
        s3c_device_gib.dev.platform_data = npd;
}



