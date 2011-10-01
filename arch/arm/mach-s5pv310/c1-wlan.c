#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/skbuff.h>

#include <plat/devs.h>
#include <plat/sdhci.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>

#define GPIO_LEVEL_LOW          0
#define GPIO_LEVEL_HIGH         1
#define GPIO_LEVEL_NONE         2

#ifdef CONFIG_MACH_C1_NA_SPR_REV02
#define SAMSUNG_GAUDI_WLAN_HSIC
#else
#define SAMSUNG_C1_WLAN
#endif

#ifdef SAMSUNG_C1_WLAN

static unsigned int wlan_on_gpio_table[][4] = {
        {GPIO_WLAN_EN , GPIO_WLAN_EN_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_HOST_WAKE, GPIO_WLAN_HOST_WAKE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
};

static unsigned int wlan_off_gpio_table[][4] = {
        {GPIO_WLAN_EN , GPIO_WLAN_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_HOST_WAKE, 0 , GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
};

static unsigned int wlan_sdio_on_table[][4] = {
        {GPIO_WLAN_SDIO_CLK, GPIO_WLAN_SDIO_CLK_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_CMD, GPIO_WLAN_SDIO_CMD_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D0, GPIO_WLAN_SDIO_D0_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D1, GPIO_WLAN_SDIO_D1_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D2, GPIO_WLAN_SDIO_D2_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D3, GPIO_WLAN_SDIO_D3_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
};

static unsigned int wlan_sdio_off_table[][4] = {
        {GPIO_WLAN_SDIO_CLK, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_SDIO_D3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
};


void s3c_config_gpio_alive_table(int array_size, unsigned int (*gpio_table)[4])
{
	u32 i, gpio;
	printk("gpio_table = [%d] \r\n" , array_size);
	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
		s3c_gpio_setpull(gpio, gpio_table[i][3]);
		if (gpio_table[i][2] != GPIO_LEVEL_NONE)
		    gpio_set_value(gpio, gpio_table[i][2]);
	}
}

void wlan_setup_power(int on, int flag)
{
	printk("------------------------------------------------");
	printk("------------------------------------------------");
	printk("------------------------------------------------");

	printk(/*KERN_INFO*/ "%s %s", __func__, on ? "on" : "down");
	if (flag != 1) {
		printk(/*KERN_DEBUG*/ "(on=%d, flag=%d)\n", on, flag);
#if 1
		if (on)
			gpio_set_value(GPIO_WLAN_EN, GPIO_LEVEL_HIGH);
		else
			gpio_set_value(GPIO_WLAN_EN, GPIO_LEVEL_LOW);
#endif
	return;
	}
	printk(/*KERN_INFO*/ " --enter\n");

	if (on) {
		s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_on_gpio_table), wlan_on_gpio_table);
		s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_sdio_on_table), wlan_sdio_on_table);

		udelay(200);

		gpio_set_value(GPIO_WLAN_EN, GPIO_LEVEL_HIGH);
		//s3c_gpio_slp_cfgpin(GPIO_WLAN_EN, S3C_GPIO_SLP_OUT1);

		printk(KERN_DEBUG "WLAN: GPIO_WLAN_EN = %d \n", gpio_get_value(GPIO_WLAN_EN));
	}
	else {
		gpio_set_value(GPIO_WLAN_EN, GPIO_LEVEL_LOW);
		//s3c_gpio_slp_cfgpin(GPIO_WLAN_EN, S3C_GPIO_SLP_OUT0);
		s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_off_gpio_table), wlan_off_gpio_table);

		printk(KERN_DEBUG "WLAN: GPIO_WLAN_EN = %d \n", gpio_get_value(GPIO_WLAN_EN));

		s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_sdio_off_table), wlan_sdio_off_table);
	}

	/* mmc_rescan*/
#if defined(CONFIG_MACH_C1_NA_SPR_REV05) || defined(CONFIG_MACH_C1_NA_SPR_EPIC2_REV00)
	sdhci_s3c_force_presence_change(&s3c_device_hsmmc2);
#else
	sdhci_s3c_force_presence_change(&s3c_device_hsmmc3);
#endif

}
EXPORT_SYMBOL(wlan_setup_power);

#endif

#ifdef SAMSUNG_GAUDI_WLAN_HSIC
void usb_host_hsic_power_init();
void usb_host_hsic_power_off();

static unsigned int wlan_on_gpio_table[][4] = {
        {GPIO_WLAN_EN , GPIO_WLAN_EN_AF, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_HOST_WAKE, GPIO_WLAN_HOST_WAKE_AF, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
};

static unsigned int wlan_off_gpio_table[][4] = {
        {GPIO_WLAN_EN , GPIO_WLAN_EN_AF, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
        {GPIO_WLAN_HOST_WAKE, 0 , GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
};


void s3c_config_gpio_alive_table(int array_size, unsigned int (*gpio_table)[4])
{
	u32 i, gpio;
	printk("gpio_table = [%d] \r\n" , array_size);
	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
		s3c_gpio_setpull(gpio, gpio_table[i][3]);
		if (gpio_table[i][2] != GPIO_LEVEL_NONE)
		    gpio_set_value(gpio, gpio_table[i][2]);
	}
}

void wlan_setup_power(int on, int flag)
{
	printk("------------------------------------------------");
	printk("------------------------------------------------");
	printk("------------------------------------------------");

	printk(/*KERN_INFO*/ "%s %s", __func__, on ? "on" : "down");
	if (flag != 1) {
		printk(/*KERN_DEBUG*/ "(on=%d, flag=%d)\n", on, flag);
#if 1
		if (on)
		{
			gpio_set_value(GPIO_WLAN_EN, GPIO_LEVEL_HIGH);
		}
		else
			gpio_set_value(GPIO_WLAN_EN, GPIO_LEVEL_LOW);
#endif
	return;
	}
	printk(/*KERN_INFO*/ " --enter\n");

	if (on) {
		usb_host_hsic_power_init();
		udelay(100);
		s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_on_gpio_table), wlan_on_gpio_table);

		udelay(200);

		gpio_set_value(GPIO_WLAN_EN, GPIO_LEVEL_HIGH);

		printk(KERN_DEBUG "WLAN: GPIO_WLAN_EN = %d \n", gpio_get_value(GPIO_WLAN_EN));
	}
	else {
		usb_host_hsic_power_off();
		gpio_set_value(GPIO_WLAN_EN, GPIO_LEVEL_LOW);
		//s3c_gpio_slp_cfgpin(GPIO_WLAN_EN, S3C_GPIO_SLP_OUT0);
		s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_off_gpio_table), wlan_off_gpio_table);

		printk(KERN_DEBUG "WLAN: GPIO_WLAN_EN = %d \n", gpio_get_value(GPIO_WLAN_EN));
	}

}
EXPORT_SYMBOL(wlan_setup_power);
#endif

#define WLAN_STATIC_BUF	// this feature for using static buffer on wifi driver /Kernel/drivers/net/wireless/bcm4330
#ifdef WLAN_STATIC_BUF

#define PREALLOC_WLAN_SEC_NUM		4
#define PREALLOC_WLAN_BUF_NUM		160
#define PREALLOC_WLAN_SECTION_HEADER	24


#define WLAN_SECTION_SIZE_0	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_1	(PREALLOC_WLAN_BUF_NUM * 128)
#define WLAN_SECTION_SIZE_2	(PREALLOC_WLAN_BUF_NUM * 512)
#define WLAN_SECTION_SIZE_3	(PREALLOC_WLAN_BUF_NUM * 1024)

#define WLAN_SKB_BUF_NUM	17

static struct sk_buff *wlan_static_skb[WLAN_SKB_BUF_NUM];

#endif /* WLAN_STATIC_BUF */

#ifdef WLAN_STATIC_BUF

struct wifi_mem_prealloc {
	void *mem_ptr;
	unsigned long size;
};

static struct wifi_mem_prealloc wifi_mem_array[PREALLOC_WLAN_SEC_NUM] = {
	{NULL, (WLAN_SECTION_SIZE_0 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_1 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_2 + PREALLOC_WLAN_SECTION_HEADER)},
	{NULL, (WLAN_SECTION_SIZE_3 + PREALLOC_WLAN_SECTION_HEADER)}
};

void *wlan_mem_prealloc(int section, unsigned long size)
{
	if (section == PREALLOC_WLAN_SEC_NUM)
		return wlan_static_skb;

	if ((section < 0) || (section > PREALLOC_WLAN_SEC_NUM))
		return NULL;

	if (wifi_mem_array[section].size < size)
		return NULL;

	return wifi_mem_array[section].mem_ptr;
}
EXPORT_SYMBOL(wlan_mem_prealloc);

#define DHD_SKB_HDRSIZE			336
#define DHD_SKB_1PAGE_BUFSIZE	((PAGE_SIZE*1)-DHD_SKB_HDRSIZE)
#define DHD_SKB_2PAGE_BUFSIZE	((PAGE_SIZE*2)-DHD_SKB_HDRSIZE)
#define DHD_SKB_4PAGE_BUFSIZE	((PAGE_SIZE*4)-DHD_SKB_HDRSIZE)

static int __init init_wifi_mem(void)
{
	int i;
	int j;

	//init GPIO
    #ifndef SAMSUNG_GAUDI_WLAN_HSIC
	s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_sdio_off_table), wlan_sdio_off_table);
	#endif
	s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_off_gpio_table), wlan_off_gpio_table);

	for (i = 0; i < 8; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_1PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	for (; i < 16; i++) {
		wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_2PAGE_BUFSIZE);
		if (!wlan_static_skb[i])
			goto err_skb_alloc;
	}

	wlan_static_skb[i] = dev_alloc_skb(DHD_SKB_4PAGE_BUFSIZE);
	if (!wlan_static_skb[i])
		goto err_skb_alloc;

	for (i = 0 ; i < PREALLOC_WLAN_SEC_NUM ; i++) {
		wifi_mem_array[i].mem_ptr =
				kmalloc(wifi_mem_array[i].size, GFP_KERNEL);

		if (!wifi_mem_array[i].mem_ptr)
			goto err_mem_alloc;
	}
	printk("%s: WIFI MEM Allocated\n", __FUNCTION__);
	return 0;

 err_mem_alloc:
	pr_err("Failed to mem_alloc for WLAN\n");
	for (j = 0 ; j < i ; j++)
		kfree(wifi_mem_array[j].mem_ptr);

	i = WLAN_SKB_BUF_NUM;

 err_skb_alloc:
	pr_err("Failed to skb_alloc for WLAN\n");
	for (j = 0 ; j < i ; j++)
		dev_kfree_skb(wlan_static_skb[j]);

	return -ENOMEM;
}

module_init(init_wifi_mem);

#endif	/* WLAN_STATIC_BUF */
