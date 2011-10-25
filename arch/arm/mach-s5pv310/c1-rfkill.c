/*
 * Copyright (C) 2010 Samsung Electronics Co., Ltd.
 *
 * Copyright (C) 2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Modified for Crespo on August, 2010 By Samsung Electronics Co.
 * This is modified operate according to each status.
 *
 */

/* Control bluetooth power for Crespo platform */

#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/rfkill.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/wakelock.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <mach/gpio.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>

#define BT_SLEEP_ENABLE

#define IRQ_BT_HOST_WAKE      IRQ_EINT(22)

#ifndef	GPIO_LEVEL_LOW
#define GPIO_LEVEL_LOW		0
#define GPIO_LEVEL_HIGH		1
#endif

static struct wake_lock rfkill_wake_lock;
static struct rfkill *bt_rfk;
static const char bt_name[] = "bcm4330";

#ifdef BT_SLEEP_ENABLE
static struct wake_lock bt_wake_lock;
static struct rfkill *bt_sleep_rfk;
#endif /* BT_SLEEP_ENABLE */

volatile int bt_is_running = 0;
EXPORT_SYMBOL(bt_is_running);

extern int s3c_gpio_slp_cfgpin(unsigned int pin, unsigned int config);
extern int s3c_gpio_slp_setpull_updown(unsigned int pin, unsigned int config);

static unsigned int bt_uart_on_table[][4] = {
	{S5PV310_GPA0(0), 2, 2, S3C_GPIO_PULL_NONE},
	{S5PV310_GPA0(1), 2, 2, S3C_GPIO_PULL_NONE},
	{S5PV310_GPA0(2), 2, 2, S3C_GPIO_PULL_NONE},
	{S5PV310_GPA0(3), 2, 2, S3C_GPIO_PULL_NONE},
};

/*
static unsigned int bt_uart_off_table[][4] = {
	{S5PV310_GPA0(0), 1, 2, S3C_GPIO_PULL_NONE},
	{S5PV310_GPA0(1), 1, 2, S3C_GPIO_PULL_NONE},
	{S5PV310_GPA0(2), 1, 2, S3C_GPIO_PULL_NONE},
	{S5PV310_GPA0(3), 1, 2, S3C_GPIO_PULL_NONE},
};
*/

void bt_config_gpio_table(int array_size, unsigned int (*gpio_table)[4])
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

void bt_uart_rts_ctrl(int flag)
{
	if(!gpio_get_value(GPIO_BT_nRST))
		return;

	if(flag) {
		// BT RTS Set to HIGH
		s3c_gpio_cfgpin(S5PV310_GPA0(3), S3C_GPIO_OUTPUT);
		s3c_gpio_setpull(S5PV310_GPA0(3), S3C_GPIO_PULL_NONE);
		gpio_set_value(S5PV310_GPA0(3), 1);

                s3c_gpio_slp_cfgpin(S5PV310_GPA0(3), S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(S5PV310_GPA0(3), S3C_GPIO_PULL_NONE);
	}
	else {
		// BT RTS Set to LOW
		s3c_gpio_cfgpin(S5PV310_GPA0(3), S3C_GPIO_OUTPUT);
		gpio_set_value(S5PV310_GPA0(3), 0);

		s3c_gpio_cfgpin(S5PV310_GPA0(3), S3C_GPIO_SFN(2));
		s3c_gpio_setpull(S5PV310_GPA0(3), S3C_GPIO_PULL_NONE);
	}
}
EXPORT_SYMBOL(bt_uart_rts_ctrl);

static int bluetooth_set_power(void *data, enum rfkill_user_states state)
{
	int ret = 0;
	int irq;
	/* BT Host Wake IRQ */
	irq = IRQ_BT_HOST_WAKE;

	switch (state) {

	case RFKILL_USER_STATE_UNBLOCKED:
		pr_debug("[BT] Device Powering ON\n");

		bt_config_gpio_table(ARRAY_SIZE(bt_uart_on_table), bt_uart_on_table);

		if (gpio_is_valid(GPIO_BT_EN))
			gpio_direction_output(GPIO_BT_EN, GPIO_LEVEL_HIGH);

		if (gpio_is_valid(GPIO_BT_nRST))
			gpio_direction_output(GPIO_BT_nRST, GPIO_LEVEL_LOW);

		pr_debug("[BT] GPIO_BT_nRST = %d\n",
				gpio_get_value(GPIO_BT_nRST));

		/* Set GPIO_BT_WLAN_REG_ON high */
		s3c_gpio_setpull(GPIO_BT_EN, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BT_EN, GPIO_LEVEL_HIGH);

		s3c_gpio_slp_cfgpin(GPIO_BT_EN, S3C_GPIO_SLP_OUT1);
		s3c_gpio_slp_setpull_updown(GPIO_BT_EN, S3C_GPIO_PULL_NONE);

		pr_debug("[BT] GPIO_BT_EN = %d\n",
				gpio_get_value(GPIO_BT_EN));
		/*
		 * FIXME sleep should be enabled disabled since the device is
		 * not booting if its enabled
		 */
		/*
		 * 100msec, delay between reg_on & rst.
		 * (bcm4329 powerup sequence)
		 */
		//msleep(100);

		/* Set GPIO_BT_nRST high */
		s3c_gpio_setpull(GPIO_BT_nRST, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BT_nRST, GPIO_LEVEL_HIGH);

		s3c_gpio_slp_cfgpin(GPIO_BT_nRST, S3C_GPIO_SLP_OUT1);
		s3c_gpio_slp_setpull_updown(GPIO_BT_nRST, S3C_GPIO_PULL_NONE);

		pr_debug("[BT] GPIO_BT_nRST = %d\n",
				gpio_get_value(GPIO_BT_nRST));

		/*
		 * 50msec, delay after bt rst
		 * (bcm4329 powerup sequence)
		 */
		msleep(50);

		ret = enable_irq_wake(irq);
		if (ret < 0)
			pr_err("[BT] set wakeup src failed\n");

		enable_irq(irq);
		break;

	case RFKILL_USER_STATE_SOFT_BLOCKED:
		pr_debug("[BT] Device Powering OFF\n");

		bt_is_running = 0;

		ret = disable_irq_wake(irq);
		if (ret < 0)
			pr_err("[BT] unset wakeup src failed\n");

		disable_irq(irq);
		wake_unlock(&rfkill_wake_lock);

		s3c_gpio_setpull(GPIO_BT_nRST, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BT_nRST, GPIO_LEVEL_LOW);

		s3c_gpio_slp_cfgpin(GPIO_BT_nRST, S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(GPIO_BT_nRST, S3C_GPIO_PULL_NONE);

		pr_debug("[BT] GPIO_BT_nRST = %d\n",
				gpio_get_value(GPIO_BT_nRST));

		s3c_gpio_setpull(GPIO_BT_EN, S3C_GPIO_PULL_NONE);
		gpio_set_value(GPIO_BT_EN, GPIO_LEVEL_LOW);

		s3c_gpio_slp_cfgpin(GPIO_BT_EN, S3C_GPIO_SLP_OUT0);
		s3c_gpio_slp_setpull_updown(GPIO_BT_EN, S3C_GPIO_PULL_NONE);

		pr_debug("[BT] GPIO_BT_EN = %d\n",
				gpio_get_value(GPIO_BT_EN));

		break;

	default:
		pr_err("[BT] Bad bluetooth rfkill state %d\n", state);
	}

	return 0;
}

irqreturn_t bt_host_wake_irq_handler(int irq, void *dev_id)
{
	pr_debug("[BT] bt_host_wake_irq_handler start\n");

	bt_is_running = 1;

	wake_lock_timeout(&rfkill_wake_lock, 5*HZ);

	return IRQ_HANDLED;
}

static int bt_rfkill_set_block(void *data, bool blocked)
{
	unsigned int ret = 0;

	ret = bluetooth_set_power(data, blocked ?
			RFKILL_USER_STATE_SOFT_BLOCKED :
			RFKILL_USER_STATE_UNBLOCKED);

	return ret;
}

static const struct rfkill_ops bt_rfkill_ops = {
	.set_block = bt_rfkill_set_block,
};

#ifdef BT_SLEEP_ENABLE
static int bluetooth_set_sleep(void *data, enum rfkill_user_states state)
{
	switch (state) {

		case RFKILL_USER_STATE_UNBLOCKED:
			bt_is_running = 0;
			gpio_set_value(GPIO_BT_WAKE, 0);
			pr_debug("[BT] GPIO_BT_WAKE = %d\n", gpio_get_value(GPIO_BT_WAKE) );
			wake_unlock(&bt_wake_lock);
			break;

		case RFKILL_USER_STATE_SOFT_BLOCKED:
			bt_is_running = 1;
			gpio_set_value(GPIO_BT_WAKE, 1);
			pr_debug("[BT] GPIO_BT_WAKE = %d\n", gpio_get_value(GPIO_BT_WAKE) );
			wake_lock(&bt_wake_lock);
			break;

		default:
			pr_err("[BT] bad bluetooth rfkill state %d\n", state);
	}
	return 0;
}

static int btsleep_rfkill_set_block(void *data, bool blocked)
{
	int ret =0;

	ret = bluetooth_set_sleep(data, blocked?
			RFKILL_USER_STATE_SOFT_BLOCKED :
			RFKILL_USER_STATE_UNBLOCKED);

	return ret;
}

static const struct rfkill_ops btsleep_rfkill_ops = {
	.set_block = btsleep_rfkill_set_block,
};
#endif

static int __init crespo_rfkill_probe(struct platform_device *pdev)
{
	int irq;
	int ret;

	/* Initialize wake locks */
	wake_lock_init(&rfkill_wake_lock, WAKE_LOCK_SUSPEND, "bt_host_wake");

	ret = gpio_request(GPIO_BT_EN, "GPB");
	if (ret < 0) {
		pr_err("[BT] Failed to request GPIO_BT_EN!\n");
		goto err_req_gpio_bt_en;
	}

	ret = gpio_request(GPIO_BT_nRST, "GPB");
	if (ret < 0) {
		pr_err("[BT] Failed to request GPIO_BT_nRST!\n");
		goto err_req_gpio_bt_nrst;
	}

	/* BT Host Wake IRQ */
	irq = IRQ_BT_HOST_WAKE;

	ret = request_irq(irq, bt_host_wake_irq_handler,
			IRQF_TRIGGER_RISING,
			"bt_host_wake_irq_handler", NULL);

	if (ret < 0) {
		pr_err("[BT] Request_irq failed\n");
		goto err_req_irq;
	}

	disable_irq(irq);

	bt_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			&bt_rfkill_ops, NULL);

	if (!bt_rfk) {
		pr_err("[BT] bt_rfk : rfkill_alloc is failed\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	rfkill_init_sw_state(bt_rfk, 0);

	pr_debug("[BT] rfkill_register(bt_rfk)\n");

	ret = rfkill_register(bt_rfk);
	if (ret) {
		pr_err("********ERROR IN REGISTERING THE bt_rfk********\n");
		goto err_register;
	}

	rfkill_set_sw_state(bt_rfk, 1);
	//bluetooth_set_power(NULL, RFKILL_USER_STATE_SOFT_BLOCKED);

#ifdef BT_SLEEP_ENABLE
	wake_lock_init(&bt_wake_lock, WAKE_LOCK_SUSPEND, "bt_wake");

	ret = gpio_request(GPIO_BT_WAKE, "gpio_bt_wake");
	if (ret < 0) {
		pr_err("[BT] Failed to request GPIO_BT_WAKE\n");
		goto err_req_gpio_bt_wake;
	}

	gpio_direction_output(GPIO_BT_WAKE, GPIO_LEVEL_LOW);

	bt_sleep_rfk = rfkill_alloc(bt_name, &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			&btsleep_rfkill_ops, NULL);

	if (!bt_sleep_rfk) {
		pr_err("[BT] bt_sleep_rfk : rfkill_alloc is failed\n");
		ret = -ENOMEM;
		goto err_sleep_alloc;
	}

	rfkill_set_sw_state(bt_sleep_rfk, 1);

	pr_debug("[BT] rfkill_register(bt_sleep_rfk)\n");

	ret = rfkill_register(bt_sleep_rfk);
	if (ret) {
		pr_err("********ERROR IN REGISTERING THE bt_sleep_rfk********\n");
		goto err_sleep_register;
	}
#endif

	return ret;

#ifdef BT_SLEEP_ENABLE
err_sleep_register:
	rfkill_destroy(bt_sleep_rfk);

err_sleep_alloc:
	gpio_free(GPIO_BT_WAKE);

err_req_gpio_bt_wake:
	rfkill_unregister(bt_rfk);
#endif

 err_register:
	rfkill_destroy(bt_rfk);

 err_alloc:
	free_irq(irq, NULL);

 err_req_irq:
	gpio_free(GPIO_BT_nRST);

 err_req_gpio_bt_nrst:
	gpio_free(GPIO_BT_EN);

 err_req_gpio_bt_en:
	return ret;
}

static struct platform_driver crespo_device_rfkill = {
	.probe = crespo_rfkill_probe,
	.driver = {
		.name = "bt_rfkill",
		.owner = THIS_MODULE,
	},
};

static int __init crespo_rfkill_init(void)
{
	int rc = 0;
	rc = platform_driver_register(&crespo_device_rfkill);

	bt_is_running = 0;

	return rc;
}

module_init(crespo_rfkill_init);
MODULE_DESCRIPTION("crespo rfkill");
MODULE_LICENSE("GPL");
