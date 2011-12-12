#include <linux/wacom_i2c.h>

#include "wacom_i2c_flash.h"

extern unsigned char onEmrProx;



int wacom_i2c_test(struct wacom_i2c *wac_i2c)
{
	int ret, i;
	char buf, test[10];
	buf = COM_QUERY;

	ret = i2c_master_send(wac_i2c->client, &buf, sizeof(buf));
	if (ret > 0)
		printk(KERN_INFO "buf:%d, sent:%d\n", buf, ret);
	else {
		printk(KERN_ERR "Digitizer is not active\n");
		return -1;
	}

	ret = i2c_master_recv(wac_i2c->client, test, sizeof(test));
	if (ret >= 0) {
		for (i = 0; i < 8; i++)
			printk(KERN_INFO "%d\n", test[i]);
	} else {
		printk(KERN_ERR "Digitizer does not reply\n");
		return -1;
	}

	return 0;
}

int wacom_i2c_master_send(struct i2c_client *client,
			const char *buf, int count,
			unsigned short addr)
{
	int ret;
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;

	msg.addr = addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (char *)buf;

	ret = i2c_transfer(adap, &msg, 1);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

int wacom_i2c_master_recv(struct i2c_client *client, char *buf,
			int count, unsigned short addr)
{
	struct i2c_adapter *adap = client->adapter;
	struct i2c_msg msg;
	int ret;

	msg.addr = addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = buf;

	ret = i2c_transfer(adap, &msg, 1);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

int wacom_i2c_query(struct wacom_i2c *wac_i2c)
{
	struct wacom_features *wac_feature = wac_i2c->wac_feature;
	struct i2c_msg msg[2];
	int ret;
	char buf;
	u8 *data;

	buf = COM_QUERY;
	data = wac_feature->data;

	msg[0].addr  = wac_i2c->client->addr;
	msg[0].flags = 0x00;
	msg[0].len   = 2;
	msg[0].buf   = (u8 *) &buf;


	msg[1].addr  = wac_i2c->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len   = COM_COORD_NUM;
	msg[1].buf   = (u8 *) data;

	ret = i2c_transfer(wac_i2c->client->adapter, msg, 2);
	printk(KERN_INFO "[E-PEN]:%s:ret of i2c_transfer=%d\n", __func__, ret);

	if  (ret == 2)
		printk(KERN_INFO "[E-PEN]:mesg=%d\n", ret);
	else {
		printk(KERN_ERR "[E-PEN]:Digitizer is not active(%d)\n", ret);
//		return -1;
	}

	wac_feature->x_max = ((u16)data[1]<<8)+(u16)data[2];
	wac_feature->y_max = ((u16)data[3]<<8)+(u16)data[4];
	wac_feature->pressure_max = (u16)data[6]+((u16)data[5]<<8);
	wac_feature->fw_version = ((u16)data[7]<<8)+(u16)data[8];

	printk(KERN_NOTICE "[E-PEN]: x_max=0x%X\n", wac_feature->x_max);
	printk(KERN_NOTICE "[E-PEN]: y_max=0x%X\n", wac_feature->y_max);
	printk(KERN_NOTICE "[E-PEN]: pressure_max=0x%X\n",
		wac_feature->pressure_max);
	printk(KERN_NOTICE "[E-PEN]: fw_version=0x%X\n",
		wac_feature->fw_version);

	if (ret >= 0) {

		if (wac_feature->fw_version !=  Firmware_version_of_file) {
			printk(KERN_WARNING "[E-PEN]: Need to flash wacom digitizer;"
				"entering flashing\n");
			ret = wacom_i2c_flash(wac_i2c);
			if (ret < 0) {
				printk(KERN_ERR "[E-PEN]:Flashing wacom digitizer failed\n");
				return -1;
			}
			if (ret == 0) {
				msleep(1000);

				ret = i2c_master_send(wac_i2c->client,
						&buf, sizeof(buf));
				msleep(1);

				do {
					ret = i2c_master_recv(wac_i2c->client,
							data, COM_COORD_NUM);
					if (ret < 1) {
						printk(KERN_ERR "[E-PEN]:failed to get wacom features\n");
						return -1;
					}
				} while (data[0] != 0x0f);
				if (ret >= 0)
					wac_feature->fw_version = ((u16)data[7]<<8)+(u16)data[8];
			}
		}
	} else {
		ret = wacom_i2c_flash(wac_i2c);
		printk(KERN_NOTICE "[E-PEN]: flashed.\n");
		if (ret < 0) {
			printk(KERN_ERR "Flashing wacom digitizer failed\n");
			return -1;
		}
		msleep(1000);
		ret = i2c_master_send(wac_i2c->client, &buf, sizeof(buf));
		msleep(1);

		do {
			ret = i2c_master_recv(wac_i2c->client, data, COM_COORD_NUM);
			if (ret < 1) {
				printk(KERN_ERR "failed to get wacom features\n");
				return -1;
			}
		} while (data[0] != 0x0f);
		if (ret >= 0)
			wac_feature->fw_version = ((u16)data[7]<<8)+(u16)data[8];
	}

	return 0;
}

int wacom_i2c_coord(struct wacom_i2c *wac_i2c)
{
	bool prox = false;
	int ret = 0;
	u8 *data;
	int rubber, stylus;
	static u16 x, y, pressure;
	static u16 tmp;

	data = wac_i2c->wac_feature->data;
	ret = i2c_master_recv(wac_i2c->client, data, COM_COORD_NUM);

	if (ret >= 0) {
	
		pr_debug("[E-PEN] %x, %x, %x, %x, %x, %x, %x\n",
			data[0], data[1], data[2], data[3], data[4], data[5], data[6]);

		if (data[0]&0x80) { 
			/* enable emr device */

			if (!wac_i2c->pen_prox) {

				wac_i2c->pen_prox = 1;

				if(data[0] & 0x40)
					wac_i2c->tool=EPEN_TOOL_RUBBER;
				else
					wac_i2c->tool=EPEN_TOOL_PEN;
				pr_debug("[E-PEN] is in(%d)\n", wac_i2c->tool);
			}

			prox = !!(data[0]&0x10);
			stylus = !!(data[0]&0x20);
			rubber = !!(data[0]&0x40);

			x = ((u16)data[1]<<8)+(u16)data[2];
			y = ((u16)data[3]<<8)+(u16)data[4];
			pressure = ((u16)data[5]<<8)+(u16)data[6];

			if (wac_i2c->wac_pdata->x_invert)
				x = wac_i2c->wac_feature->x_max - x;
			if (wac_i2c->wac_pdata->y_invert)
				y = wac_i2c->wac_feature->y_max - y;
			
			if (wac_i2c->wac_pdata->xy_switch) {
				tmp = x;
				x = y;
				y = tmp;
			}

			input_report_abs(wac_i2c->input_dev, ABS_X, x);
			input_report_abs(wac_i2c->input_dev, ABS_Y, y);
			input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, pressure);
			input_report_key(wac_i2c->input_dev, EPEN_STYLUS, stylus);
			input_report_key(wac_i2c->input_dev, BTN_TOUCH, prox);
			input_report_key(wac_i2c->input_dev, wac_i2c->tool, 1);
			input_sync(wac_i2c->input_dev);

			if(prox && !wac_i2c->pen_pressed)
				pr_info("[E-PEN] is pressed(%d,%d)(%d)\n", x, y, wac_i2c->tool);
			else if(!prox && wac_i2c->pen_pressed)
				pr_info("[E-PEN] is released(%d,%d)(%d)\n", x, y, wac_i2c->tool);
			wac_i2c->pen_pressed = prox;

			if(stylus && !wac_i2c->side_pressed)
				pr_info("[E-PEN] side on(%d,%d)(%d)\n", x, y, wac_i2c->tool);
			else if(!stylus && wac_i2c->side_pressed)
				pr_info("[E-PEN] side off(%d,%d)(%d)\n", x, y, wac_i2c->tool);
			wac_i2c->side_pressed = stylus;

		} else {
			/* enable emr device */

			if (wac_i2c->pen_prox) {
				/* input_report_abs(wac->input_dev, ABS_X, x); */
				/* input_report_abs(wac->input_dev, ABS_Y, y); */
				input_report_abs(wac_i2c->input_dev, ABS_PRESSURE, 0);
				input_report_key(wac_i2c->input_dev, EPEN_STYLUS, 0);
				input_report_key(wac_i2c->input_dev, BTN_TOUCH, 0);
				input_report_key(wac_i2c->input_dev, wac_i2c->tool, 0);
				input_sync(wac_i2c->input_dev);

				if (wac_i2c->pen_pressed || wac_i2c->side_pressed)
					pr_info("[E-PEN] is out(%d)\n", wac_i2c->tool);
				else
					pr_debug("[E-PEN] is out(%d)\n", wac_i2c->tool);
				/* check_emr_device(false); */
				/* unset dvfs level */
			}
			wac_i2c->pen_prox = 0;
			wac_i2c->pen_pressed = 0;
			wac_i2c->side_pressed = 0;
		}
	} else {
		printk(KERN_ERR "[E-PEN]: failed to read i2c\n");
		return -1;
	}

	return 0;
}
