#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/usb/ch9.h>

#include <plat/regs-otg.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>

#include <mach/regs-clock.h>

#if defined(CONFIG_SAMSUNG_PHONE_SVNET)
#include <linux/modemctl.h>
#endif
#define ETC6PUD		(S5PV310_VA_GPIO2 + 0x228)

enum usb_clk_type {
	USBOTG_CLK, USBHOST_CLK
};

static struct clk *usbotg_clk = NULL;
static struct clk *usbhost_clk = NULL;
static atomic_t otg_clk_count = ATOMIC_INIT(0);
static atomic_t host_clk_count = ATOMIC_INIT(0);

static void usb_clk_get(enum usb_clk_type clk_type)
{
	struct clk *usb_clk;

	if( clk_type == USBOTG_CLK) {
		if(usbotg_clk == NULL ) {
			usbotg_clk = clk_get(NULL, "usbotg");

			if (IS_ERR(usbotg_clk)) {
				printk(KERN_ERR"cannot get usbotg clock\n");
				usbotg_clk = NULL;
				return;
			}
		}
		usb_clk = usbotg_clk;
	} else if( clk_type == USBHOST_CLK) {
		if(usbhost_clk == NULL) {
			usbhost_clk = clk_get(NULL, "usbhost");

			if (IS_ERR(usbhost_clk)) {
				printk(KERN_ERR"cannot get usbhost clock\n");
				usbhost_clk = NULL;
				return;
			}
		}
		usb_clk = usbhost_clk;
	} else {
		printk(KERN_ERR"Undefine enum usb_clock\n");
		return ;
	}

	clk_enable(usb_clk);
	if(clk_type == USBOTG_CLK) {
		atomic_inc(&otg_clk_count);
	}
	else if(clk_type == USBHOST_CLK) {
		atomic_inc(&host_clk_count);
	}
}

static void usb_clk_put(enum usb_clk_type clk_type)
{
	struct clk *usb_clk;

	if( clk_type == USBOTG_CLK) {
		if(usbotg_clk != NULL ) {
			usb_clk = usbotg_clk;
		} else {
			printk(KERN_ERR"cannot put usbotg clock\n");
			return ;
		}
	} else if( clk_type == USBHOST_CLK) {
		if(usbhost_clk != NULL ) {
			usb_clk = usbhost_clk;
		} else {
			printk(KERN_ERR"cannot put usbhost clock\n");
			return ;
		}
	} else {
		printk(KERN_ERR"Undefine enum usb_clock\n");
		return ;
	}

	clk_disable(usb_clk);
	if(clk_type == USBOTG_CLK) {
		atomic_dec(&otg_clk_count);
	}
	else if(clk_type == USBHOST_CLK) {
		atomic_dec(&host_clk_count);
	}
}

int is_usb_host_phy_suspend(void)
{
	usb_clk_get(USBOTG_CLK);
	/*If USB Host PHY1 is suspended,  */
	if ((__raw_readl(S3C_USBOTG_PHYPWR) & (1<<6))) {
		unsigned long flags;
		local_irq_save(flags);
		if (__raw_readl(S5P_USBHOST_PHY_CONTROL) & 0x1) {
#if defined(CONFIG_SAMSUNG_PHONE_SVNET)
			mc_control_active_state(0);
			mc_control_pda_active(0);
#endif
			__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
				|(0x1<<7)|(0x1<<6), S3C_USBOTG_PHYPWR);
			__raw_writel(__raw_readl(S5P_USBHOST_PHY_CONTROL)
				&~(1<<0), S5P_USBHOST_PHY_CONTROL);
			usb_clk_put(USBHOST_CLK);
		}
		local_irq_restore(flags);
		usb_clk_put(USBOTG_CLK);
		return 1;
	}

	usb_clk_put(USBOTG_CLK);
	return 0;
}
EXPORT_SYMBOL(is_usb_host_phy_suspend);

/* Initializes OTG Phy. */
void otg_phy_init(void)
{
	usb_clk_get(USBOTG_CLK);

	__raw_writel(__raw_readl(S5P_USBOTG_PHY_CONTROL)
		|(0x1<<0), S5P_USBOTG_PHY_CONTROL);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
		&~(0x7<<3)&~(0x1<<0)), S3C_USBOTG_PHYPWR);
	__raw_writel((0x3<<0), S3C_USBOTG_PHYCLK); /* PLL 24Mhz */
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON)
		&~(0x3<<1))|(0x1<<0), S3C_USBOTG_RSTCON);
	udelay(10);
	__raw_writel(__raw_readl(S3C_USBOTG_RSTCON)
		&~(0x7<<0), S3C_USBOTG_RSTCON);
	udelay(10);
#if defined (CONFIG_TARGET_LOCALE_NTT)
	/* Enables HS Transmitter pre-emphasis [20] */
	__raw_writel(__raw_readl(S3C_USBOTG_PHYTUNE) |(0x1<<20), S3C_USBOTG_PHYTUNE);
	udelay(10);
	/* HD DC Voltage Level Adjustment [3:0] (1000 : +10%) */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYTUNE) & ~(0xf)) | (0x1<<3), S3C_USBOTG_PHYTUNE);
	/* HD DC Voltage Level Adjustment [3:0] (1111 : +24%) */
	/*__raw_writel((__raw_readl(S3C_USBOTG_PHYTUNE) & ~(0xf)) | (0xf), S3C_USBOTG_PHYTUNE);*/
	udelay(10);
#elif defined (CONFIG_TARGET_LOCALE_KOR)
	/* Enables HS Transmitter pre-emphasis [20] */
	__raw_writel(__raw_readl(S3C_USBOTG_PHYTUNE) |(0x1<<20), S3C_USBOTG_PHYTUNE);
	udelay(10);

	/* HD DC Voltage Level Adjustment [3:0] (1011 : +16%) */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYTUNE) & ~(0xf)) | (0xb), S3C_USBOTG_PHYTUNE);
#endif
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
//struct usb_ctrlrequest usb_ctrl __cacheline_aligned;

/* OTG PHY Power Off */
void otg_phy_off(void)
{
	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
		|(0x3<<3), S3C_USBOTG_PHYPWR);
	__raw_writel(__raw_readl(S5P_USBOTG_PHY_CONTROL)
		&~(1<<0), S5P_USBOTG_PHY_CONTROL);

	usb_clk_put(USBOTG_CLK);
}
EXPORT_SYMBOL(otg_phy_off);

void usb_host_phy_init(void)
{
#if defined(CONFIG_SAMSUNG_PHONE_SVNET)
	mc_control_pda_active(1);
	if (mc_is_modem_on() && !mc_is_host_wakeup()) {
		mc_control_slave_wakeup(1);
	}
#endif
	/*  Must be enable usbhost & usbotg clk  */
	usb_clk_get(USBHOST_CLK);
	usb_clk_get(USBOTG_CLK);

	if (__raw_readl(S5P_USBHOST_PHY_CONTROL) & (0x1<<0)) {
		printk(KERN_ERR"[usb_host_phy_init]Already power on PHY\n");
		return;
	}

#ifdef CONFIG_CPU_S5PV310_EVT1
	 /*
	 // set XuhostOVERCUR to in-active by controlling ET6PUD[15:14]
	 //  0x0 : pull-up/down disabled
	 //  0x1 : pull-down enabled
	 //  0x2 : reserved
	 //  0x3 : pull-up enabled
	 */
	__raw_writel((__raw_readl(ETC6PUD) & ~(0x3 << 14)) | (0x3 << 14), ETC6PUD); //pull-up
#else
	__raw_writel((__raw_readl(ETC6PUD) & ~(0x3 << 14)) | (0x1 << 14), ETC6PUD); // pull-down
#endif

	__raw_writel(__raw_readl(S5P_USBHOST_PHY_CONTROL)
		|(0x1<<0), S5P_USBHOST_PHY_CONTROL);

	/* floating prevention logic : disable */
	__raw_writel((__raw_readl(S3C_USBOTG_PHY1CON) | (0x1<<0)),
		S3C_USBOTG_PHY1CON);

	/* set hsic phy0,1 to normal */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR) & ~(0xf<<9)),
		S3C_USBOTG_PHYPWR);

	/* phy-power on */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR) & ~(0x7<<6)),
		S3C_USBOTG_PHYPWR);

	/* set clock source for PLL (24MHz) */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK) | (0x1<<7) | (0x3<<0)),
		S3C_USBOTG_PHYCLK);

	/* reset all ports of both PHY and Link */
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON) | (0x1<<6) | (0x7<<3)),
		S3C_USBOTG_RSTCON);
	udelay(10);
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON) & ~(0x1<<6) & ~(0x7<<3)),
		S3C_USBOTG_RSTCON);
	udelay(50);

	usb_clk_put(USBOTG_CLK);
}
EXPORT_SYMBOL(usb_host_phy_init);

void usb_host_phy_off(void)
{
	if ((__raw_readl(S5P_USBHOST_PHY_CONTROL) & (0x1<<0))) {
		usb_clk_get(USBOTG_CLK);

		__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
			|(0x1<<7)|(0x1<<6), S3C_USBOTG_PHYPWR);
		__raw_writel(__raw_readl(S5P_USBHOST_PHY_CONTROL)
			&~(1<<0), S5P_USBHOST_PHY_CONTROL);

		usb_clk_put(USBOTG_CLK);
		usb_clk_put(USBHOST_CLK);
	}
}
EXPORT_SYMBOL(usb_host_phy_off);

/* For L2 suspend */
void usb_host_phy_suspend(void)
{
	unsigned int reg = __raw_readl(S3C_USBOTG_PHYPWR);

	usb_clk_get(USBOTG_CLK);
	/* if OHCI isn't used, 7 bit clear */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK) & ~(0x1<<7)),
		S3C_USBOTG_PHYCLK);

	reg |= (0x1<<6);  /* phy port0 suspend */
	reg |= (0x1<<9);  /* phy hsic port0 suspend */
	reg |= (0x1<<11); /* phy hsic port1 suspend */
	__raw_writel(reg, S3C_USBOTG_PHYPWR);

	usb_clk_put(USBOTG_CLK);
}
EXPORT_SYMBOL(usb_host_phy_suspend);

int usb_host_phy_resume(void)
{
	int ret = 0;
	usb_clk_get(USBOTG_CLK);

	if (!(__raw_readl(S5P_USBHOST_PHY_CONTROL) & 0x1)) {
		usb_clk_get(USBHOST_CLK);
#if defined(CONFIG_SAMSUNG_PHONE_SVNET)
		mc_control_pda_active(1);
#endif
		printk(KERN_DEBUG "Host USB : Reset-resume\n");
		__raw_writel(__raw_readl(S5P_USBHOST_PHY_CONTROL)
			|(0x1<<0), S5P_USBHOST_PHY_CONTROL);

		/* floating prevention logic : disable */
		__raw_writel((__raw_readl(S3C_USBOTG_PHY1CON) | (0x1<<0)),
			S3C_USBOTG_PHY1CON);

		/* set hsic phy0,1 to normal */
		__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR) & ~(0xf<<9)),
			S3C_USBOTG_PHYPWR);

		/* phy-power on */
		__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR) & ~(0x7<<6)),
			S3C_USBOTG_PHYPWR);

		/* set clock source for PLL (24MHz) */
		__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK) | (0x1<<7) | (0x3<<0)),
			S3C_USBOTG_PHYCLK);

		/* reset all ports of both PHY and Link */
		__raw_writel((__raw_readl(S3C_USBOTG_RSTCON) | (0x1<<6) | (0x7<<3)),
			S3C_USBOTG_RSTCON);
		udelay(10);
		__raw_writel((__raw_readl(S3C_USBOTG_RSTCON) & ~(0x1<<6) & ~(0x7<<3)),
			S3C_USBOTG_RSTCON);
		udelay(50);
		ret = 1;
	} else {
		printk(KERN_DEBUG "Host USB : Resume\n");
		__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
			& ~((0x1<<6)|(0x1<<9)|(0x1<<11)), S3C_USBOTG_PHYPWR);
		/* if OHCI 48mhz, 7 bit set */
		__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK) | (0x1<<7)),
			S3C_USBOTG_PHYCLK);
		ret = 0;
	}
	usb_clk_put(USBOTG_CLK);

	return ret;
}
EXPORT_SYMBOL(usb_host_phy_resume);

/* For LPM(L1) suspend */
void usb_host_phy_sleep(void)
{
	unsigned int reg = __raw_readl(S3C_USBOTG_PHYPWR);

	usb_clk_get(USBOTG_CLK);

	/* if OHCI isn't used, 7 bit clear */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK) & ~(0x1<<7)),
		S3C_USBOTG_PHYCLK);

	reg |= (0x1<<8);  /* phy port0 sleep */
	reg |= (0x1<<10); /* phy hsic port0 sleep */
	reg |= (0x1<<12); /* phy hsic port1 sleep */
	__raw_writel(reg, S3C_USBOTG_PHYPWR);

	usb_clk_put(USBOTG_CLK);
}
EXPORT_SYMBOL(usb_host_phy_sleep);

void usb_host_phy_wakeup(void)
{
	usb_clk_get(USBOTG_CLK);

	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
		& ~((0x1<<8)|(0x1<<10)|(0x1<<12)), S3C_USBOTG_PHYPWR);
	/* if OHCI 48mhz, 7 bit set */
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK) | (0x1<<7)),
		S3C_USBOTG_PHYCLK);

	usb_clk_put(USBOTG_CLK);
}
EXPORT_SYMBOL(usb_host_phy_wakeup);


/* soonyong.cho : We received below code from MCCI. This function can enable otg host. */
#ifdef CONFIG_USB_S3C_OTG_HOST
/* Initializes OTG Phy */
void otg_host_phy_init(void)
{
	usb_clk_get(USBOTG_CLK);

    __raw_writel(__raw_readl(S5P_USBOTG_PHY_CONTROL)
		|(0x1<<0), S5P_USBOTG_PHY_CONTROL);
    __raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
        &~(0x7<<3)&~(0x1<<0)), S3C_USBOTG_PHYPWR);
    __raw_writel((__raw_readl(S3C_USBOTG_PHYCLK)
        &~(0x1<<4))|(0x7<<0), S3C_USBOTG_PHYCLK);

    __raw_writel((__raw_readl(S3C_USBOTG_RSTCON)
        &~(0x3<<1))|(0x1<<0), S3C_USBOTG_RSTCON);
    udelay(10);
    __raw_writel((__raw_readl(S3C_USBOTG_RSTCON)
        &~(0x7<<0)), S3C_USBOTG_RSTCON);
    udelay(10);

    __raw_writel((__raw_readl(S3C_UDC_OTG_GUSBCFG)
        |(0x3<<8)), S3C_UDC_OTG_GUSBCFG);

    printk("otg_host_phy_int : USBPHYCTL=0x%x,PHYPWR=0x%x,PHYCLK=0x%x,PHYTUNE=0x%x,USBCFG=0x%x\n",
        readl(S5P_USBOTG_PHY_CONTROL),
        readl(S3C_USBOTG_PHYPWR),
        readl(S3C_USBOTG_PHYCLK),
	readl(S3C_USBOTG_PHYTUNE),
        readl(S3C_UDC_OTG_GUSBCFG)
        );
}
EXPORT_SYMBOL(otg_host_phy_init);
#endif
