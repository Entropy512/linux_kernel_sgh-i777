#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

extern struct device *gps_dev;

static unsigned int gps_uart_on_table[][4] = {
	{GPIO_GPS_RXD, GPIO_GPS_RXD_AF, 2, S3C_GPIO_PULL_UP},
	{GPIO_GPS_TXD, GPIO_GPS_TXD_AF, 2, S3C_GPIO_PULL_NONE},
//#if defined(CONFIG_MACH_C1_REV00)
	{GPIO_GPS_CTS, GPIO_GPS_CTS_AF, 2, S3C_GPIO_PULL_NONE},
	{GPIO_GPS_RTS, GPIO_GPS_RTS_AF, 2, S3C_GPIO_PULL_NONE},
//#endif
};

void gps_uart_cfg_gpio(int array_size, unsigned int (*gpio_table)[4])
{
	u32 i, gpio;

	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
		s3c_gpio_setpull(gpio, gpio_table[i][3]);
		if (gpio_table[i][2] != 2)
			gpio_set_value(gpio, gpio_table[i][2]);
	}
}

static void __init gps_gpio_init(void)
{
	gpio_request(GPIO_GPS_nRST, "GPS_nRST");
	s3c_gpio_setpull(GPIO_GPS_nRST, S3C_GPIO_PULL_UP);
	s3c_gpio_cfgpin(GPIO_GPS_nRST, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_GPS_nRST, 1);
#ifdef CONFIG_TARGET_LOCALE_NTT
	if (system_rev >= 11) {
		gpio_request(GPIO_GPS_PWR_EN, "GPS_PWR_EN");
		s3c_gpio_setpull(GPIO_GPS_PWR_EN, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_GPS_PWR_EN, S3C_GPIO_OUTPUT);
		gpio_direction_output(GPIO_GPS_PWR_EN, 0);

		gpio_export(GPIO_GPS_nRST, 1);
		gpio_export(GPIO_GPS_PWR_EN, 1);
	}
	else {
		gpio_request(GPIO_GPS_PWR_EN_SPI, "GPS_PWR_EN");
		s3c_gpio_setpull(GPIO_GPS_PWR_EN_SPI, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_GPS_PWR_EN_SPI, S3C_GPIO_OUTPUT);
		gpio_direction_output(GPIO_GPS_PWR_EN_SPI, 0);

		gpio_export(GPIO_GPS_nRST, 1);
		gpio_export(GPIO_GPS_PWR_EN_SPI, 1);
	}
#else
	gpio_request(GPIO_GPS_PWR_EN, "GPS_PWR_EN");
	s3c_gpio_setpull(GPIO_GPS_PWR_EN, S3C_GPIO_PULL_NONE);
	s3c_gpio_cfgpin(GPIO_GPS_PWR_EN, S3C_GPIO_OUTPUT);
	gpio_direction_output(GPIO_GPS_PWR_EN, 0);

	gpio_export(GPIO_GPS_nRST, 1);
	gpio_export(GPIO_GPS_PWR_EN, 1);
#endif

#ifdef CONFIG_TARGET_LOCALE_KOR
	if(system_rev >= 7) {
       gpio_request(GPIO_GPS_RTS, "GPS_RTS");
       s3c_gpio_setpull(GPIO_GPS_RTS, S3C_GPIO_PULL_UP);
       s3c_gpio_cfgpin(GPIO_GPS_RTS, S3C_GPIO_OUTPUT);
       gpio_direction_output(GPIO_GPS_RTS, 1);

       gpio_request(GPIO_GPS_CTS, "GPS_CTS");
       s3c_gpio_setpull(GPIO_GPS_CTS, S3C_GPIO_PULL_UP);
       s3c_gpio_cfgpin(GPIO_GPS_CTS, S3C_GPIO_OUTPUT);
       gpio_direction_output(GPIO_GPS_CTS, 1);
    }
#endif

	BUG_ON(!gps_dev);
	gpio_export_link(gps_dev, "GPS_nRST", GPIO_GPS_nRST);
#ifdef CONFIG_TARGET_LOCALE_NTT
	if (system_rev >= 11)
		gpio_export_link(gps_dev, "GPS_PWR_EN", GPIO_GPS_PWR_EN);
	else
		gpio_export_link(gps_dev, "GPS_PWR_EN", GPIO_GPS_PWR_EN_SPI);
#else
	gpio_export_link(gps_dev, "GPS_PWR_EN", GPIO_GPS_PWR_EN);
#endif

	gps_uart_cfg_gpio(ARRAY_SIZE(gps_uart_on_table), gps_uart_on_table);
}

arch_initcall(gps_gpio_init);
