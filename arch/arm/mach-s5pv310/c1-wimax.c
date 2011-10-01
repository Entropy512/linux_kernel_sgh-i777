#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/skbuff.h>
#include <linux/wimax/samsung/wimax732.h>

#include <plat/devs.h>
#include <plat/sdhci.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/gpio.h>

#if defined(CONFIG_WIMAX_CMC) && defined(CONFIG_TARGET_LOCALE_NA)
#define dump_debug(args...)				\
	{						\
		printk(KERN_ALERT"\x1b[1;33m[WiMAX] "); \
		printk(args);				\
		printk("\x1b[0m\n");			\
	}

extern int wimax_pmic_set_voltage();

extern int s3c_bat_use_wimax(int onoff);

static struct wimax_cfg wimax_config;

void wimax_on_pin_conf(int onoff)
{
	int gpio;
	if (onoff) {

		for (gpio = S5PV310_GPK3(0); gpio < S5PV310_GPK3(2); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}

		for (gpio = S5PV310_GPK3(3); gpio <= S5PV310_GPK3(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}

	} else {
		for (gpio = S5PV310_GPK3(0); gpio < S5PV310_GPK3(2); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_DOWN);
		}
		for (gpio = S5PV310_GPK3(3); gpio <= S5PV310_GPK3(6); gpio++) {
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}

	}

}

static void store_uart_path(void)
{
	wimax_config.uart_sel = gpio_get_value(GPIO_UART_SEL);
	wimax_config.uart_sel1 = gpio_get_value(GPIO_UART_SEL1);
}

static void wimax_deinit_gpios(void);
static void wimax_wakeup_assert(int enable)
{
	gpio_set_value(GPIO_WIMAX_WAKEUP, !enable);
}

static int get_wimax_sleep_mode(void)
{
	return gpio_get_value(GPIO_WIMAX_IF_MODE1);
}

static int is_wimax_active(void)
{
	return gpio_get_value(GPIO_WIMAX_CON0);
}

static void signal_ap_active(int enable)
{
	gpio_set_value(GPIO_WIMAX_CON1, enable);
}

static void switch_eeprom_wimax(void)
{
	gpio_set_value(GPIO_WIMAX_I2C_CON, GPIO_LEVEL_LOW);
	msleep(10);
}

static void switch_usb_ap(void)
{
	gpio_set_value(GPIO_WIMAX_USB_EN, GPIO_LEVEL_LOW);
#if defined(CONFIG_MACH_C1_REV02)
	gpio_set_value(USB_SEL, GPIO_LEVEL_LOW);
#endif				/* CONFIG_MACH_C1_REV02 */
	msleep(10);
}

static void switch_usb_wimax(void)
{
	gpio_set_value(GPIO_WIMAX_USB_EN, GPIO_LEVEL_HIGH);
#if defined(CONFIG_MACH_C1_REV02)
	gpio_set_value(USB_SEL, GPIO_LEVEL_HIGH);
#endif				/* CONFIG_MACH_C1_REV02 */
	msleep(10);
}

static void switch_uart_wimax(void)
{
	gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);
	gpio_set_value(GPIO_UART_SEL1, GPIO_LEVEL_HIGH);
}

static void wimax_init_gpios(void)
{
	s3c_gpio_cfgpin(GPIO_WIMAX_INT, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_WIMAX_INT, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_WIMAX_CON0, S3C_GPIO_INPUT);
	s3c_gpio_setpull(GPIO_WIMAX_CON0, S3C_GPIO_PULL_UP);
	gpio_set_value(GPIO_WIMAX_IF_MODE1, GPIO_LEVEL_HIGH);	/* default idle */
	gpio_set_value(GPIO_WIMAX_CON2, GPIO_LEVEL_HIGH);	/* active low interrupt */
	gpio_set_value(GPIO_WIMAX_CON1, GPIO_LEVEL_HIGH);
}

static void hw_set_wimax_mode(void)
{
	switch (wimax_config.wimax_mode) {
	case SDIO_MODE:
		pr_debug("SDIO MODE");
		gpio_set_value(GPIO_WIMAX_WAKEUP, GPIO_LEVEL_HIGH);
		gpio_set_value(GPIO_WIMAX_IF_MODE0, GPIO_LEVEL_HIGH);
		break;
	case WTM_MODE:
	case AUTH_MODE:
		pr_debug("WTM_MODE || AUTH_MODE");
		gpio_set_value(GPIO_WIMAX_WAKEUP, GPIO_LEVEL_LOW);
		gpio_set_value(GPIO_WIMAX_IF_MODE0, GPIO_LEVEL_LOW);
		break;
	case DM_MODE:
		pr_debug("DM MODE");
		gpio_set_value(GPIO_WIMAX_WAKEUP, GPIO_LEVEL_HIGH);
		gpio_set_value(GPIO_WIMAX_IF_MODE0, GPIO_LEVEL_LOW);
		break;
	case USB_MODE:
	case USIM_RELAY_MODE:
		pr_debug("USB MODE");
		gpio_set_value(GPIO_WIMAX_WAKEUP, GPIO_LEVEL_LOW);
		gpio_set_value(GPIO_WIMAX_IF_MODE0, GPIO_LEVEL_HIGH);
		break;
	}
}

static void wimax_hsmmc_presence_check(void)
{
	sdhci_s3c_force_presence_change(&s3c_device_hsmmc3);
}

static void display_gpios(void)
{
	int val = 0;
	val = gpio_get_value(GPIO_WIMAX_EN);
	dump_debug("WIMAX_EN: %d", val);
	val = gpio_get_value(GPIO_WIMAX_RESET_N);
	dump_debug("WIMAX_RESET: %d", val);
	val = gpio_get_value(GPIO_WIMAX_CON0);
	dump_debug("WIMAX_CON0: %d", val);
	val = gpio_get_value(GPIO_WIMAX_CON1);
	dump_debug("WIMAX_CON1: %d", val);
	val = gpio_get_value(GPIO_WIMAX_CON2);
	dump_debug("WIMAX_CON2: %d", val);
	val = gpio_get_value(GPIO_WIMAX_WAKEUP);
	dump_debug("WIMAX_WAKEUP: %d", val);
	val = gpio_get_value(GPIO_WIMAX_INT);
	dump_debug("WIMAX_INT: %d", val);
	val = gpio_get_value(GPIO_WIMAX_IF_MODE0);
	dump_debug("WIMAX_IF_MODE0: %d", val);
	val = gpio_get_value(GPIO_WIMAX_IF_MODE1);
	dump_debug("WIMAX_IF_MODE1: %d", val);
	val = gpio_get_value(GPIO_WIMAX_USB_EN);
	dump_debug("WIMAX_USB_EN: %d", val);
	val = gpio_get_value(GPIO_WIMAX_I2C_CON);
	dump_debug("I2C_SEL: %d", val);
#if defined(CONFIG_MACH_C1_REV02)
	val = gpio_get_value(USB_SEL);
	dump_debug("WIMAX_USB_SEL: %d", val);
#endif				/* CONFIG_MACH_C1_REV02 */
	val = gpio_get_value(GPIO_UART_SEL);
	dump_debug("UART_SEL1: %d", val);
}

static int gpio_wimax_power(int enable)
{
	if (!enable)
		goto wimax_power_off;
	if (gpio_get_value(GPIO_WIMAX_EN)) {
		dump_debug("Already Wimax powered ON");
		return WIMAX_ALREADY_POWER_ON;
	}
	while (!wimax_config.card_removed)
		msleep(100);
	dump_debug("Wimax power ON");
	if (wimax_config.wimax_mode != SDIO_MODE) {
		switch_usb_wimax();
		s3c_bat_use_wimax(1);
	}
	if (wimax_config.wimax_mode == WTM_MODE) {
		store_uart_path();
		switch_uart_wimax();
	}
	switch_eeprom_wimax();
	wimax_on_pin_conf(1);
	gpio_set_value(GPIO_WIMAX_EN, GPIO_LEVEL_HIGH);
	wimax_init_gpios();
	wimax_pmic_set_voltage();
	dump_debug("RESET");
	gpio_set_value(GPIO_WIMAX_RESET_N, GPIO_LEVEL_LOW);
	mdelay(3);
	gpio_set_value(GPIO_WIMAX_RESET_N, GPIO_LEVEL_HIGH);
	msleep(400);
	wimax_hsmmc_presence_check();
	return WIMAX_POWER_SUCCESS;
 wimax_power_off:
	if (!gpio_get_value(GPIO_WIMAX_EN)) {
		dump_debug("Already Wimax powered OFF");
		return WIMAX_ALREADY_POWER_OFF;	/* already power off */
	}
	while (!wimax_config.powerup_done) {
		msleep(500);
		dump_debug("Wimax waiting for power Off ");
	}
	msleep(500);
	wimax_deinit_gpios();
	dump_debug("Wimax power OFF");
	wimax_hsmmc_presence_check();
	msleep(500);

	if (wimax_config.wimax_mode != SDIO_MODE) {
		s3c_bat_use_wimax(0);
	}
	wimax_on_pin_conf(0);

	return WIMAX_POWER_SUCCESS;
}

static void wimax_deinit_gpios(void)
{
	s3c_gpio_cfgpin(GPIO_WIMAX_DBGEN_28V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_DBGEN_28V, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_DBGEN_28V, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_I2C_CON, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_I2C_CON, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_I2C_CON, GPIO_LEVEL_HIGH);

	s3c_gpio_cfgpin(GPIO_WIMAX_WAKEUP, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_WAKEUP, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_WAKEUP, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_IF_MODE0, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_IF_MODE0, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_IF_MODE0, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_CON0, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_CON0, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_CON0, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_IF_MODE1, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_IF_MODE1, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_IF_MODE1, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_CON2, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_CON2, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_CON2, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_CON1, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_CON1, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_CON1, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_RESET_N, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_RESET_N, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_RESET_N, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_EN, GPIO_LEVEL_LOW);
	s3c_gpio_cfgpin(GPIO_WIMAX_USB_EN, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_USB_EN, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_USB_EN, GPIO_LEVEL_LOW);
#if defined(CONFIG_MACH_C1_REV02)
	s3c_gpio_cfgpin(USB_SEL, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(USB_SEL, S3C_GPIO_PULL_NONE);
#endif				/* CONFIG_MACH_C1_REV02 */
	s3c_gpio_cfgpin(GPIO_UART_SEL1, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_UART_SEL1, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_UART_SEL, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);

	s3c_gpio_cfgpin(GPIO_CMC_SCL_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_CMC_SCL_18V, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_CMC_SCL_18V, GPIO_LEVEL_LOW);

	s3c_gpio_cfgpin(GPIO_CMC_SDA_18V, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_CMC_SDA_18V, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_CMC_SDA_18V, GPIO_LEVEL_LOW);

	s3c_gpio_cfgpin(GPIO_WIMAX_INT, S3C_GPIO_OUTPUT);
	s3c_gpio_setpull(GPIO_WIMAX_INT, S3C_GPIO_PULL_NONE);
	gpio_set_value(GPIO_WIMAX_INT, GPIO_LEVEL_LOW);

	switch_usb_ap();
}

static void restore_uart_path(void)
{
	gpio_set_value(GPIO_UART_SEL, wimax_config.uart_sel);
	gpio_set_value(GPIO_UART_SEL1, wimax_config.uart_sel1);
}

static void switch_uart_ap(void)
{
	gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);
}

static struct wimax732_platform_data wimax732_pdata = {
	.power = gpio_wimax_power,
	.set_mode = hw_set_wimax_mode,
	.signal_ap_active = signal_ap_active,
	.get_sleep_mode = get_wimax_sleep_mode,
	.is_modem_awake = is_wimax_active,
	.wakeup_assert = wimax_wakeup_assert,
	.uart_wimax = switch_uart_wimax,
	.uart_ap = switch_uart_ap,
	.gpio_display = display_gpios,
	.restore_uart_path = restore_uart_path,
	.g_cfg = &wimax_config,
};

struct platform_device s3c_device_cmc732 = {
	.name = "wimax732_driver",
	.id = 1,
	.dev.platform_data = &wimax732_pdata,
};

#endif
