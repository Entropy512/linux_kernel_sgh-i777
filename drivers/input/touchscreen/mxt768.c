/*
 *  Copyright (C) 2010, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/i2c/mxt224_u1.h>
#include <asm/unaligned.h>
#include "mxt768e.h"

#define OBJECT_TABLE_START_ADDRESS	7
#define OBJECT_TABLE_ELEMENT_SIZE	6

#define CMD_RESET_OFFSET		0
#define CMD_BACKUP_OFFSET		1

#define DETECT_MSG_MASK			0x80
#define PRESS_MSG_MASK			0x40
#define RELEASE_MSG_MASK		0x20
#define MOVE_MSG_MASK			0x10
#define SUPPRESS_MSG_MASK		0x02

#define ID_BLOCK_SIZE			7

struct object_t {
	u8 object_type;
	u16 i2c_address;
	u8 size;
	u8 instances;
	u8 num_report_ids;
} __packed;

struct finger_info {
	s16 x;
	s16 y;
	s16 z;
	u16 w;
	int16_t component;
};

struct mxt224_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct early_suspend early_suspend;
	u32 finger_mask;
	int gpio_read_done;
	struct object_t *objects;
	u8 objects_len;
	u8 tsp_version;
	const u8 *power_cfg;
	u8 finger_type;
	u16 msg_proc;
	u16 cmd_proc;
	u16 msg_object_size;
	u32 x_dropbits:2;
	u32 y_dropbits:2;
	void (*power_on)(void);
	void (*power_off)(void);
	void (*register_cb)(void*);
	void (*read_ta_status)(void*);	
	int num_fingers;
	struct finger_info fingers[];
};

struct mxt224_data *copy_data;
int touch_is_pressed = 0;
extern struct class *sec_class;

static int mxt224_enabled = 0;
static bool g_debug_switch = 1;
static u8 tsp_version_disp = 0;
static int threshold=0;

static int read_mem(struct mxt224_data *data, u16 reg, u8 len, u8 *buf)
{
	int ret;
	u16 le_reg = cpu_to_le16(reg);
	struct i2c_msg msg[2] = {
		{
			.addr = data->client->addr,
			.flags = 0,
			.len = 2,
			.buf = (u8 *)&le_reg,
		},
		{
			.addr = data->client->addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = buf,
		},
	};

	ret = i2c_transfer(data->client->adapter, msg, 2);
	if (ret < 0)
		return ret;

	return ret == 2 ? 0 : -EIO;
}

static int write_mem(struct mxt224_data *data, u16 reg, u8 len, const u8 *buf)
{
	int ret;
	u8 tmp[len + 2];

	put_unaligned_le16(cpu_to_le16(reg), tmp);
	memcpy(tmp + 2, buf, len);

	ret = i2c_master_send(data->client, tmp, sizeof(tmp));
	if (ret < 0)
		return ret;

	return ret == sizeof(tmp) ? 0 : -EIO;
}

static int __devinit mxt224_reset(struct mxt224_data *data)
{
	u8 buf = 1u;
	return write_mem(data, data->cmd_proc + CMD_RESET_OFFSET, 1, &buf);
}

static int __devinit mxt224_backup(struct mxt224_data *data)
{
	u8 buf = 0x55u;
	return write_mem(data, data->cmd_proc + CMD_BACKUP_OFFSET, 1, &buf);
}

static int get_object_info(struct mxt224_data *data, u8 object_type, u16 *size,
				u16 *address)
{
	int i;

	for (i = 0; i < data->objects_len; i++) {
		if (data->objects[i].object_type == object_type) {
			*size = data->objects[i].size + 1;
			*address = data->objects[i].i2c_address;
			return 0;
		}
	}

	return -ENODEV;
}

static int write_config(struct mxt224_data *data, u8 type, const u8 *cfg)
{
	int ret;
	u16 address;
	u16 size;

	ret = get_object_info(data, type, &size, &address);

	if (ret)
		return ret;

	return write_mem(data, address, size, cfg);
}


static u32 __devinit crc24(u32 crc, u8 byte1, u8 byte2)
{
	static const u32 crcpoly = 0x80001B;
	u32 res;
	u16 data_word;

	data_word = (((u16)byte2) << 8) | byte1;
	res = (crc << 1) ^ (u32)data_word;

	if (res & 0x1000000)
		res ^= crcpoly;

	return res;
}

static int __devinit calculate_infoblock_crc(struct mxt224_data *data,
							u32 *crc_pointer)
{
	u32 crc = 0;
	u8 mem[7 + data->objects_len * 6];
	int status;
	int i;

	status = read_mem(data, 0, sizeof(mem), mem);

	if (status)
		return status;

	for (i = 0; i < sizeof(mem) - 1; i += 2)
		crc = crc24(crc, mem[i], mem[i + 1]);

	*crc_pointer = crc24(crc, mem[i], 0) & 0x00FFFFFF;

	return 0;
}

static void mxt224_ta_probe(int ta_status)
{
	u16 obj_address=0;
	u16 size_one;
	int ret;
	u8 value;
	int active_interval=0;
	u8 val=0;
	unsigned int register_address=7;

	if(!mxt224_enabled) {
		printk(KERN_ERR"mxt224_enabled is 0\n");
		return;
	}

	if(ta_status) {
		threshold=70;
		active_interval=255;
		}
	else {
 		 threshold=40;
		 active_interval=18;
		}
	value=(u8)threshold;
	ret = get_object_info(copy_data, TOUCH_MULTITOUCHSCREEN_T9, &size_one, &obj_address);
	size_one=1;
	write_mem(copy_data, obj_address+(u16)register_address, size_one, &value);
	read_mem(copy_data, obj_address+(u16)register_address, (u8)size_one, &val);
	printk(KERN_ERR"T%d Byte%d is %d\n",9,register_address,val);
	
	value=(u8)active_interval;
	register_address=1;
	ret = get_object_info(copy_data, GEN_POWERCONFIG_T7, &size_one, &obj_address);
	size_one=1;
	write_mem(copy_data, obj_address+(u16)register_address, size_one, &value);
	read_mem(copy_data, obj_address+(u16)register_address, (u8)size_one, &val);
	printk(KERN_ERR"T%d Byte%d is %d\n",7,register_address,val);
	
	
};

static int __devinit mxt224_init_touch_driver(struct mxt224_data *data)
{
	struct object_t *object_table;
	u32 read_crc = 0;
	u32 calc_crc;
	u16 crc_address;
	u16 dummy;
	int i;
	u8 id[ID_BLOCK_SIZE];
	int ret;
	u8 type_count = 0;
	u8 tmp;

	ret = read_mem(data, 0, sizeof(id), id);
	if (ret)
		return ret;

	dev_info(&data->client->dev, "family = %#02x, variant = %#02x, version "
			"= %#02x, build = %d\n", id[0], id[1], id[2], id[3]);
	dev_dbg(&data->client->dev, "matrix X size = %d\n", id[4]);
	dev_dbg(&data->client->dev, "matrix Y size = %d\n", id[5]);

	data->tsp_version = id[2];
	data->objects_len = id[6];

	tsp_version_disp = data->tsp_version;

	object_table = kmalloc(data->objects_len * sizeof(*object_table),
				GFP_KERNEL);
	if (!object_table)
		return -ENOMEM;

	ret = read_mem(data, OBJECT_TABLE_START_ADDRESS,
			data->objects_len * sizeof(*object_table),
			(u8 *)object_table);
	if (ret)
		goto err;

	for (i = 0; i < data->objects_len; i++) {
		object_table[i].i2c_address =
				le16_to_cpu(object_table[i].i2c_address);
		tmp = 0;
		if (object_table[i].num_report_ids) {
			tmp = type_count + 1;
			type_count += object_table[i].num_report_ids *
						(object_table[i].instances + 1);
		}
		switch (object_table[i].object_type) {
		case TOUCH_MULTITOUCHSCREEN_T9:
			data->finger_type = tmp;
			dev_dbg(&data->client->dev, "Finger type = %d\n",
						data->finger_type);
			break;
		case GEN_MESSAGEPROCESSOR_T5:
			data->msg_object_size = object_table[i].size + 1;
			dev_dbg(&data->client->dev, "Message object size = "
						"%d\n", data->msg_object_size);
			break;
		}
	}

	data->objects = object_table;

	/* Verify CRC */
	crc_address = OBJECT_TABLE_START_ADDRESS +
			data->objects_len * OBJECT_TABLE_ELEMENT_SIZE;

#ifdef __BIG_ENDIAN
#error The following code will likely break on a big endian machine
#endif
	ret = read_mem(data, crc_address, 3, (u8 *)&read_crc);
	if (ret)
		goto err;

	read_crc = le32_to_cpu(read_crc);

	ret = calculate_infoblock_crc(data, &calc_crc);
	if (ret)
		goto err;

	if (read_crc != calc_crc) {
		dev_err(&data->client->dev, "CRC error\n");
		ret = -EFAULT;
		goto err;
	}

	ret = get_object_info(data, GEN_MESSAGEPROCESSOR_T5, &dummy,
					&data->msg_proc);
	if (ret)
		goto err;

	ret = get_object_info(data, GEN_COMMANDPROCESSOR_T6, &dummy,
					&data->cmd_proc);
	if (ret)
		goto err;

	return 0;

err:
	kfree(object_table);
	return ret;
}

static void report_input_data(struct mxt224_data *data)
{
	int i;

	for (i = 0; i < data->num_fingers; i++) {
		if (data->fingers[i].z == -1)
			continue;

		input_report_abs(data->input_dev, ABS_MT_POSITION_X,
					data->fingers[i].x);
		input_report_abs(data->input_dev, ABS_MT_POSITION_Y,
					data->fingers[i].y);
		input_report_abs(data->input_dev, ABS_MT_TOUCH_MAJOR,
					data->fingers[i].z);
		input_report_abs(data->input_dev, ABS_MT_WIDTH_MAJOR,
					data->fingers[i].w);
		input_report_abs(data->input_dev, ABS_MT_TRACKING_ID, i);

		#ifdef _SUPPORT_SHAPE_TOUCH_
		input_report_abs(data->input_dev, ABS_MT_COMPONENT, data->fingers[i].component);
		//printk(KERN_ERR"the component data is %d\n",data->fingers[i].component);
		#endif											

		input_mt_sync(data->input_dev);
		
		if(g_debug_switch) { 
			printk(KERN_ERR"ID-%d, X-%d ,Y-%d\n",i , data->fingers[i].x, data->fingers[i].y);
		}
		
		if (data->fingers[i].z == 0)
			data->fingers[i].z = -1;
	}
	data->finger_mask = 0;

	input_sync(data->input_dev);
}

static irqreturn_t mxt224_irq_thread(int irq, void *ptr)
{
	struct mxt224_data *data = ptr;
	int id;
	u8 msg[data->msg_object_size];

	do {
		if (read_mem(data, data->msg_proc, sizeof(msg), msg))
			return IRQ_HANDLED;

		id = msg[0] - data->finger_type;

		if( ( msg[1] & 0x80 ) && ( msg[1] & 0x40 ) ) {
			touch_is_pressed = 1;
		} else if ( ( msg[1] & 0x20 ) && ( id == 0 ) ) {
			touch_is_pressed = 0;
		} else if ( ( msg[0] == 14 ) && ( ( msg[1]&0x01) == 0x00 ) ) {
			touch_is_pressed = 0;
		}

		/* If not a touch event, then keep going */
		if (id < 0 || id >= data->num_fingers)
			continue;

		if (data->finger_mask & (1U << id))
			report_input_data(data);

		if (msg[1] & RELEASE_MSG_MASK) {
			data->fingers[id].z = 0;
			data->fingers[id].w = msg[5];
			data->finger_mask |= 1U << id;
		} else if ((msg[1] & DETECT_MSG_MASK) && (msg[1] &
				(PRESS_MSG_MASK | MOVE_MSG_MASK))) {
			data->fingers[id].z = 40;
			data->fingers[id].w = msg[5];
			data->fingers[id].x = ((msg[2] << 4) | (msg[4] >> 4)) >>
							data->x_dropbits;
			data->fingers[id].y = ((msg[3] << 4) |
					(msg[4] & 0xF)) >> data->y_dropbits;
			data->finger_mask |= 1U << id;

#ifdef _SUPPORT_SHAPE_TOUCH_
			data->fingers[id].component= msg[7];
#endif

		} else if ((msg[1] & SUPPRESS_MSG_MASK) &&
			   (data->fingers[id].z != -1)) {
			data->fingers[id].z = 0;
			data->fingers[id].w = msg[5];
			data->finger_mask |= 1U << id;
		} else {
			dev_dbg(&data->client->dev, "Unknown state %#02x %#02x"
						"\n", msg[0], msg[1]);
			continue;
		}
	} while (!gpio_get_value(data->gpio_read_done));

	if (data->finger_mask)
		report_input_data(data);

	return IRQ_HANDLED;
}

static int mxt224_internal_suspend(struct mxt224_data *data)
{
	static const u8 sleep_power_cfg[3];
	int ret;
	int i;

	ret = write_config(data, GEN_POWERCONFIG_T7, sleep_power_cfg);
	if (ret)
		return ret;


	for (i = 0; i < data->num_fingers; i++) {
		if (data->fingers[i].z == -1)
			continue;
		data->fingers[i].z = 0;
	}
	report_input_data(data);

	data->power_off();

	return 0;
}

static int mxt224_internal_resume(struct mxt224_data *data)
{
	int ret;
	int i;

	data->power_on();

	i = 0;
	do {
		ret = write_config(data, GEN_POWERCONFIG_T7, data->power_cfg);
		msleep(20);
		i++;
	} while (ret && i < 10);

	return ret;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
#define mxt224_suspend	NULL
#define mxt224_resume	NULL

static void mxt224_early_suspend(struct early_suspend *h)
{
	struct mxt224_data *data = container_of(h, struct mxt224_data,
								early_suspend);
	mxt224_enabled = 0;
	touch_is_pressed = 0;
	disable_irq(data->client->irq);
	mxt224_internal_suspend(data);
}

static void mxt224_late_resume(struct early_suspend *h)
{
	bool ta_status=0;
	struct mxt224_data *data = container_of(h, struct mxt224_data,
								early_suspend);
	mxt224_internal_resume(data);
	enable_irq(data->client->irq);

	mxt224_enabled = 1;	

	if(data->read_ta_status) {
		data->read_ta_status(&ta_status);
		printk(KERN_ERR"## ta_status is %d", ta_status);
		mxt224_ta_probe(ta_status);
	}
}
#else
static int mxt224_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt224_data *data = i2c_get_clientdata(client);

	mxt224_enabled = 0;
	touch_is_pressed = 0;
	return mxt224_internal_suspend(data);
}

static int mxt224_resume(struct device *dev)
{
	int ret = 0;
	bool ta_status=0;
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt224_data *data = i2c_get_clientdata(client);

	ret = mxt224_internal_resume(data);

	mxt224_enabled = 1;	

	if(data->read_ta_status) {
		data->read_ta_status(&ta_status);
		printk(KERN_ERR"## ta_status is %d", ta_status);
		mxt224_ta_probe(ta_status);
	}
	return ret;
}
#endif

void Mxt224_force_released(void)
{
    struct mxt224_data*data = copy_data;
	int i;

	if(!mxt224_enabled) {
		printk(KERN_ERR"mxt224_enabled is 0\n");
		return;
	}

	for (i = 0; i < data->num_fingers; i++) {
		if (data->fingers[i].z == -1)
			continue;
		data->fingers[i].z = 0;
	}
	report_input_data(data);

	touch_is_pressed = 0;	
};
EXPORT_SYMBOL(Mxt224_force_released);

static ssize_t mxt224_debug_setting(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	g_debug_switch = !g_debug_switch;
	return 0;
}

static ssize_t qt602240_object_setting(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	  struct mxt224_data*data = dev_get_drvdata(dev);
	  unsigned int object_type;
	  unsigned int object_register;
	  unsigned int register_value;
	  u8 value;
	  u8 val;
	  int ret;
	  u16 address;
	  u16 size;
	  sscanf(buf,"%u%u%u",&object_type,&object_register,&register_value);
	  printk(KERN_ERR"object type T%d\n",object_type);
	  printk(KERN_ERR"object register ->Byte%d\n",object_register);
	  printk(KERN_ERR"register value %d\n",register_value);
	  ret = get_object_info(data, (u8)object_type, &size, &address);
	  if(ret)
	  	{
	  	printk(KERN_ERR"fail to get object_info\n");
	    return count;
	  	}
      size=1;
	  value=(u8)register_value;
      write_mem(data, address+(u16)object_register, size, &value);
      read_mem(data, address+(u16)object_register, (u8)size, &val);

	  printk(KERN_ERR"T%d Byte%d is %d\n",object_type,object_register,val);
	  /*test program*/
	  mxt224_ta_probe(1);
	  return count;

}

static ssize_t qt602240_object_show(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mxt224_data*data = dev_get_drvdata(dev);
	unsigned int object_type;
	u8 val;
	int ret;
	u16 address;
	u16 size;
	u16 i;
	sscanf(buf,"%u",&object_type);
	printk(KERN_ERR"object type T%d\n",object_type);
	ret = get_object_info(data, (u8)object_type, &size, &address);
	  if(ret)
	  	{
	  	printk(KERN_ERR"fail to get object_info\n");
	    return count;
	  	}
    for (i=0;i<size;i++)
    	{
    	 read_mem(data, address+i, 1, &val);
		 printk(KERN_ERR"Byte %u --> %u\n",i,val);
    	}
    return count;
}

#define ENABLE_NOISE_TEST_MODE 1
#ifdef ENABLE_NOISE_TEST_MODE
struct device *qt602240_noise_test;
/*
	botton_right, botton_left, center, top_right, top_left
*/
unsigned char test_node[5] = {12, 20, 104, 188, 196};

void diagnostic_chip(u8 mode)
{
    int error;
    u16 t6_address=0;
    u16 size_one;
    int ret;
    u8 value;
    u16 t37_address=0;

    ret = get_object_info(copy_data, GEN_COMMANDPROCESSOR_T6, &size_one, &t6_address);

    size_one=1;
    error = write_mem(copy_data, t6_address+5, (u8)size_one, &mode);
		//qt602240_write_object(p_qt602240_data, QT602240_GEN_COMMAND,
        	//QT602240_COMMAND_DIAGNOSTIC, mode);
    if (error < 0)
    {
        printk(KERN_ERR "[TSP] error %s: write_object\n", __func__);
    }
	else
	{   
	    get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size_one, &t37_address);
	    size_one=1;
	    //printk(KERN_ERR"diagnostic_chip setting success\n");
	    read_mem(copy_data, t37_address, (u8)size_one, &value);
		//printk(KERN_ERR"dianostic_chip mode is %d\n",value);
	    
	}
}

void read_dbg_data(uint8_t dbg_mode , uint8_t node, uint16_t * dbg_data)
{
	u8 read_page, read_point;
	u8 data_buffer[2] = { 0 };
	int i,ret;
	u16 size;
	u16 object_address=0;

	read_page = node / 64;
	node %= 64;
	read_point = (node *2) + 2;

	/* Page Num Clear */
	diagnostic_chip(QT_CTE_MODE);
	msleep(20);

	diagnostic_chip(dbg_mode);
	msleep(20);

	ret = get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size, &object_address);
#if 0
	for(i = 0; i < 5; i++) {
		//qt602240_read_diagnostic(0, data_buffer, 1);
		if(data_buffer[0] == dbg_mode) {
			break;
		}
		msleep(20);
	}
#else
	msleep(20);
#endif
	printk(KERN_DEBUG "[TSP] page clear \n");

	for(i = 1; i <= read_page; i++) {
		diagnostic_chip(QT_PAGE_UP);
		msleep(20);
		//qt602240_read_diagnostic(1, data_buffer, 1);
		read_mem(copy_data, object_address+1, 1, data_buffer);
		if(data_buffer[0] != i) {
			if(data_buffer[0] >= 0x4)
				break;
			i--;
		}
	}

	//qt602240_read_diagnostic(read_point, data_buffer, 2);
	read_mem(copy_data, object_address+(u16)read_point, 2, data_buffer);
	*dbg_data= ((uint16_t)data_buffer[1]<<8)+ (uint16_t)data_buffer[0];
}


#define MAX_VALUE 7000
#if 0
#define MIN_VALUE 14500
#else
#define MIN_VALUE 16500
#endif
int read_all_data(uint16_t dbg_mode)
{
	u8 read_page,read_point;
	u16 max_value = MAX_VALUE, min_value = MIN_VALUE;
	uint16_t qt_refrence; 
	u16 object_address = 0;
	u8 data_buffer[2] = { 0 };	
	u8 node = 0;
	int state = 0;
	int i, ret;
	u16 size;

	/* Page Num Clear */
	diagnostic_chip(QT_CTE_MODE);
	msleep(20);

	diagnostic_chip(dbg_mode);
	msleep(20);

	ret = get_object_info(copy_data, DEBUG_DIAGNOSTIC_T37, &size, &object_address);	

#if 0		
	for(i = 0; i < 5; i++) {
		if(data_buffer[0] == dbg_mode) {
			break;
		}
		msleep(20);
	}
#else
	msleep(20);
#endif

	for(read_page=0; read_page<4; read_page++) {
 		for(node = 0; node < 64; node++) {
			read_point = (node *2) + 2;
			read_mem(copy_data, object_address+(u16)read_point, 2, data_buffer);
			qt_refrence = ((uint16_t)data_buffer[1]<<8)+ (uint16_t)data_buffer[0];
			if((qt_refrence > MIN_VALUE)||(qt_refrence < MAX_VALUE)) {
				state = 1;
				printk(KERN_ERR"page %d, node %d \n",read_page,node);
				break;
			}

			if(data_buffer[0] != 0) {
				if(qt_refrence > max_value)
					max_value = qt_refrence;
				if(qt_refrence < min_value)
					min_value = qt_refrence;
			}

			// all node => 19 * 11 = 209 => (3page * 64) + 17
			if((read_page == 3) && (node == 16)) 
				break;
		}
		diagnostic_chip(QT_PAGE_UP);		
		msleep(10); 
	}

	if((max_value - min_value) > 4500) {
		printk(KERN_ERR"diff = %d \n",(max_value - min_value));
		state = 1;
	}

	return (state);
}

static ssize_t set_refer0_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint16_t qt_refrence=0;
    read_dbg_data(QT_REFERENCE_MODE, test_node[0],&qt_refrence);
    return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer1_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint16_t qt_refrence=0;
    read_dbg_data(QT_REFERENCE_MODE, test_node[1],&qt_refrence);
    return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer2_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint16_t qt_refrence=0;
    read_dbg_data(QT_REFERENCE_MODE, test_node[2],&qt_refrence);
    return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer3_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint16_t qt_refrence=0;
    read_dbg_data(QT_REFERENCE_MODE, test_node[3],&qt_refrence);
    return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_refer4_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint16_t qt_refrence=0;
    read_dbg_data(QT_REFERENCE_MODE, test_node[4],&qt_refrence);
    return sprintf(buf, "%u\n", qt_refrence);
}

static ssize_t set_delta0_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint16_t qt_delta=0;
    read_dbg_data(QT_DELTA_MODE, test_node[0],&qt_delta);
    if(qt_delta < 32767)
        return sprintf(buf, "%u\n", qt_delta);
    else
        qt_delta = 65535 - qt_delta;

    return sprintf(buf, "-%u\n", qt_delta);
}

static ssize_t set_delta1_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint16_t qt_delta=0;
    read_dbg_data(QT_DELTA_MODE, test_node[1],&qt_delta);
    if(qt_delta < 32767)
        return sprintf(buf, "%u\n", qt_delta);
    else
        qt_delta = 65535 - qt_delta;

    return sprintf(buf, "-%u\n", qt_delta);
}

static ssize_t set_delta2_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint16_t qt_delta=0;
    read_dbg_data(QT_DELTA_MODE, test_node[2],&qt_delta);
    if(qt_delta < 32767)
        return sprintf(buf, "%u\n", qt_delta);
    else
        qt_delta = 65535 - qt_delta;

    return sprintf(buf, "-%u\n", qt_delta);
}

static ssize_t set_delta3_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint16_t qt_delta=0;
    read_dbg_data(QT_DELTA_MODE, test_node[3],&qt_delta);
    if(qt_delta < 32767)
        return sprintf(buf, "%u\n", qt_delta);
    else
        qt_delta = 65535 - qt_delta;

    return sprintf(buf, "-%u\n", qt_delta);
}

static ssize_t set_delta4_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    uint16_t qt_delta=0;
    read_dbg_data(QT_DELTA_MODE, test_node[4],&qt_delta);
    if(qt_delta < 32767)
        return sprintf(buf, "%u\n", qt_delta);
    else
        qt_delta = 65535 - qt_delta;

    return sprintf(buf, "-%u\n", qt_delta);
}

static ssize_t set_threshold_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	
	return sprintf(buf, "%u\n", threshold);
}

static ssize_t set_all_refer_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int status = 0;
	
	status = read_all_data(QT_REFERENCE_MODE);

	return sprintf(buf, "%u\n", status);	
}

static ssize_t set_firm_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{

    return sprintf(buf, "%#02x\n", tsp_version_disp);    

}

static DEVICE_ATTR(set_refer0, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_refer0_mode_show, NULL);
static DEVICE_ATTR(set_delta0, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_delta0_mode_show, NULL);
static DEVICE_ATTR(set_refer1, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_refer1_mode_show, NULL);
static DEVICE_ATTR(set_delta1, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_delta1_mode_show, NULL);
static DEVICE_ATTR(set_refer2, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_refer2_mode_show, NULL);
static DEVICE_ATTR(set_delta2, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_delta2_mode_show, NULL);
static DEVICE_ATTR(set_refer3, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_refer3_mode_show, NULL);
static DEVICE_ATTR(set_delta3, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_delta3_mode_show, NULL);
static DEVICE_ATTR(set_refer4, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_refer4_mode_show, NULL);
static DEVICE_ATTR(set_delta4, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_delta4_mode_show, NULL);
static DEVICE_ATTR(set_all_refer, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_all_refer_mode_show, NULL);
static DEVICE_ATTR(set_threshould, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_threshold_mode_show, NULL);
static DEVICE_ATTR(set_firm_version, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, set_firm_version_show, NULL);
#endif /* ENABLE_NOISE_TEST_MODE */

static DEVICE_ATTR(object_show, 0664,NULL, qt602240_object_show);
static DEVICE_ATTR(object_write, 0664, NULL, qt602240_object_setting);
static DEVICE_ATTR(dbg_switch, 0664, NULL, mxt224_debug_setting);


static struct attribute *qt602240_attrs[] = {
	&dev_attr_object_show.attr,
	&dev_attr_object_write.attr,
	&dev_attr_dbg_switch,
	NULL
};

static const struct attribute_group qt602240_attr_group = {
	.attrs = qt602240_attrs,
};


#ifdef QT_FIRMUP_ENABLE	
#define I2C_M_WR 0 /* for i2c */
#define I2C_MAX_SEND_LENGTH     300

int boot_qt602240_i2c_write(struct mxt224_data *data, u16 reg, u8 *read_val, unsigned int len)
{
	struct i2c_msg wmsg;

	unsigned char data_buf[I2C_MAX_SEND_LENGTH];
	int ret,i;

	if(len+2 > I2C_MAX_SEND_LENGTH)
	{
		printk("[TSP][ERROR] %s() data length error\n", __FUNCTION__);
		return -ENODEV;
	}

	wmsg.addr = 0x26;
	wmsg.flags = I2C_M_WR;
	wmsg.len = len;
	wmsg.buf = data_buf;


	for (i = 0; i < len; i++)
	{
		data_buf[i] = *(read_val+i);
	}

	/* printk("%s, %s\n",__func__,wbuf); */
	ret = i2c_transfer(data->client->adapter, &wmsg, 1);
	return ret;
}

int boot_write_mem(struct mxt224_data *data, u16 start, u16 size, u8 *mem)
{
	int ret;

	ret = boot_qt602240_i2c_write(data, start,mem,size);
	if(ret < 0){
		printk("boot write mem fail: %d \n",ret);
		return ret;
	}
	else
		return ret;
}

int boot_read_mem(struct mxt224_data *data, u16 start, u8 size, u8 *mem)
{
	struct i2c_msg rmsg;
	int ret;

	rmsg.addr = 0x26;
	rmsg.flags = I2C_M_RD;
	rmsg.len = size;
	rmsg.buf = mem;
	ret = i2c_transfer(data->client->adapter, &rmsg, 1);
	
	return ret;
}

int boot_unlock(struct mxt224_data *data)
{

	int ret;
	unsigned char data_buf[2];

	data_buf[0] = 0xDC;
	data_buf[1] = 0xAA;
	
	ret = boot_qt602240_i2c_write(data, 0,data_buf,2);
	if(ret < 0) {
		printk("%s : i2c write failed\n",__func__);
		return(WRITE_MEM_FAILED);
	} 

	return(WRITE_MEM_OK);

}

int QT_Boot_no_reset(struct mxt224_data *data)
{
	unsigned char boot_status;
	unsigned char boot_ver;
	unsigned char retry_cnt;
	unsigned short character_position;
	unsigned short frame_size = 0;
	unsigned short next_frame;
	unsigned short crc_error_count;
	unsigned char size1,size2;
	unsigned short j,read_status_flag;
	unsigned char  *firmware_data ;

	firmware_data = MXT768_firmware;

	crc_error_count = 0;
	character_position = 0;
	next_frame = 0;

		for(retry_cnt = 0; retry_cnt < 30; retry_cnt++)
		{
			
			if( (boot_read_mem(data, 0,1,&boot_status) == READ_MEM_OK) && (boot_status & 0xC0) == 0xC0) 
			{
				boot_ver = boot_status & 0x3F;
				crc_error_count = 0;
				character_position = 0;
				next_frame = 0;

				while(1)
				{ 
					for(j = 0; j<5; j++)
					{
						mdelay(60);
						if(boot_read_mem(data, 0,1,&boot_status) == READ_MEM_OK)
						{
							read_status_flag = 1;
							break;
						}
						else 
							read_status_flag = 0;
					}
					
					if(read_status_flag==1)	
					{
						retry_cnt  = 0;
						printk("TSP boot status is %x		stage 2 \n", boot_status);
						if((boot_status & QT_WAITING_BOOTLOAD_COMMAND) == QT_WAITING_BOOTLOAD_COMMAND)
						{
							if(boot_unlock(data) == WRITE_MEM_OK)
							{
								mdelay(10);
			
								printk("Unlock OK\n");
			
							}
							else
							{
								printk("Unlock fail\n");
							}
						}
						else if((boot_status & 0xC0) == QT_WAITING_FRAME_DATA)
						{
							 /* Add 2 to frame size, as the CRC bytes are not included */
							size1 =  *(firmware_data+character_position);
							size2 =  *(firmware_data+character_position+1)+2;
							frame_size = (size1<<8) + size2;
			
							printk("Frame size:%d\n", frame_size);
							printk("Firmware pos:%d\n", character_position);
							/* Exit if frame data size is zero */
							if( 0 == frame_size )
							{
								if(QT_i2c_address == I2C_BOOT_ADDR_0)
								{
									QT_i2c_address = I2C_APPL_ADDR_0;
								}
								printk("0 == frame_size\n");
								return 1;
							}
							next_frame = 1;
							boot_write_mem(data, 0,frame_size, (firmware_data +character_position));
							mdelay(10);
							printk(".");
			
						}
						else if(boot_status == QT_FRAME_CRC_CHECK)
						{
							printk("CRC Check\n");
						}
						else if(boot_status == QT_FRAME_CRC_PASS)
						{
							if( next_frame == 1)
							{
								printk("CRC Ok\n");
								character_position += frame_size;
								next_frame = 0;
							}
							else {
								printk("next_frame != 1\n");
							}
						}
						else if(boot_status  == QT_FRAME_CRC_FAIL)
						{
							printk("CRC Fail\n");
							crc_error_count++;
						}
						if(crc_error_count > 10)
						{
							return QT_FRAME_CRC_FAIL;
						}
			
					}
					else
					{
						return 0;
					}
				}
			}
			else
			{
				printk("[TSP] read_boot_state() or (boot_status & 0xC0) == 0xC0) is fail!!!\n");
			}
		}
		
		printk("QT_Boot_no_reset end \n");
		return 0;

}

void QT_reprogram(struct mxt224_data *data)
{
	uint8_t version, build;
	unsigned char rxdata;

	printk("QT_reprogram check\n");

	if(boot_read_mem(data, 0,1,&rxdata) == READ_MEM_OK)
	{
		printk("Enter to new firmware : boot mode\n");
	        if(!QT_Boot_no_reset(data)) {
				data->power_off();
				mdelay(300);
				data->power_on();
	        }
	           
	        printk("Reprogram done : boot mode\n");
	}
#if 0
	quantum_touch_probe();  /* find and initialise QT device */

	get_version(&version);
	get_build_number(&build);

	if((version != 0x14)&&(version < 0x16)||((version == 0x16)&&(build == 0xAA)))
	{
	        printk("Enter to new firmware : ADDR = Other Version\n");
	        if(!QT_Boot())
	        {
	            TSP_Restart();
	            quantum_touch_probe();
	        }
	        printk("Reprogram done : ADDR = Other Version\n");
	}
#endif
}
#endif

static int __devinit mxt224_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	struct mxt224_platform_data *pdata = client->dev.platform_data;
	struct mxt224_data *data;
	struct input_dev *input_dev;
	int ret;
	int i;

	touch_is_pressed = 0;

	if (!pdata) {
		dev_err(&client->dev, "missing platform data\n");
		return -ENODEV;
	}

	if (pdata->max_finger_touches <= 0)
		return -EINVAL;

	data = kzalloc(sizeof(*data) + pdata->max_finger_touches *
					sizeof(*data->fingers), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->num_fingers = pdata->max_finger_touches;
	data->power_on = pdata->power_on;
	data->power_off = pdata->power_off;
	data->register_cb = pdata->register_cb;
	data->read_ta_status = pdata->read_ta_status;	

	data->client = client;
	i2c_set_clientdata(client, data);

	input_dev = input_allocate_device();
	if (!input_dev) {
		ret = -ENOMEM;
		dev_err(&client->dev, "input device allocation failed\n");
		goto err_alloc_dev;
	}
	data->input_dev = input_dev;
	input_set_drvdata(input_dev, data);
	input_dev->name = "sec_touchscreen";

	set_bit(EV_ABS, input_dev->evbit);

	input_set_abs_params(input_dev, ABS_MT_POSITION_X, pdata->min_x,
			pdata->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, pdata->min_y,
			pdata->max_y, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, pdata->min_z,
			pdata->max_z, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, pdata->min_w,
			pdata->max_w, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TRACKING_ID, 0,
			data->num_fingers - 1, 0, 0);

#ifdef _SUPPORT_SHAPE_TOUCH_
	input_set_abs_params(input_dev, ABS_MT_COMPONENT, 0, 255, 0, 0);
#endif
	ret = input_register_device(input_dev);
	if (ret) {
		input_free_device(input_dev);
		goto err_reg_dev;
	}

	data->gpio_read_done = pdata->gpio_read_done;

	data->power_on();

#ifdef QT_FIRMUP_ENABLE
	{
		QT_i2c_address = 0x26;
		QT_reprogram(data);
	}	
#endif

	ret = mxt224_init_touch_driver(data);

	copy_data=data;

	data->register_cb(mxt224_ta_probe);

	if (ret) {
		dev_err(&client->dev, "chip initialization failed\n");
		goto err_init_drv;
	}

	for (i = 0; pdata->config[i][0] != RESERVED_T255; i++) {
		ret = write_config(data, pdata->config[i][0],
							pdata->config[i] + 1);
		if (ret)
			goto err_config;

		if (pdata->config[i][0] == GEN_POWERCONFIG_T7)
			data->power_cfg = pdata->config[i] + 1;

		if (pdata->config[i][0] == TOUCH_MULTITOUCHSCREEN_T9) {
			/* Are x and y inverted? */
			if (pdata->config[i][10] & 0x1) {
				data->x_dropbits =
					(!(pdata->config[i][22] & 0xC)) << 1;
				data->y_dropbits =
					(!(pdata->config[i][20] & 0xC)) << 1;
			} else {
				data->x_dropbits =
					(!(pdata->config[i][20] & 0xC)) << 1;
				data->y_dropbits =
					(!(pdata->config[i][22] & 0xC)) << 1;
			}
		}
	}

	ret = mxt224_backup(data);
	if (ret)
		goto err_backup;

	/* reset the touch IC. */
	ret = mxt224_reset(data);
	if (ret)
		goto err_reset;

	msleep(60);

	for (i = 0; i < data->num_fingers; i++)
		data->fingers[i].z = -1;

	ret = request_threaded_irq(client->irq, NULL, mxt224_irq_thread,
		IRQF_TRIGGER_LOW | IRQF_ONESHOT, "mxt224_ts", data);
	if (ret < 0)
		goto err_irq;

	ret = sysfs_create_group(&client->dev.kobj, &qt602240_attr_group);
	if(ret)
		printk(KERN_ERR"sysfs_create_group()is falled\n");

#ifdef ENABLE_NOISE_TEST_MODE
		qt602240_noise_test = device_create(sec_class, NULL, 0, NULL, "qt602240_noise_test");
	
		if (IS_ERR(qt602240_noise_test))
		{
			printk(KERN_ERR "Failed to create device(qt602240_noise_test)!\n");
		}
	
		if (device_create_file(qt602240_noise_test, &dev_attr_set_refer0)< 0)
		{
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_refer0.attr.name);
		}
	
		if (device_create_file(qt602240_noise_test, &dev_attr_set_delta0) < 0)
		{
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_delta0.attr.name);
		}
	
		if (device_create_file(qt602240_noise_test, &dev_attr_set_refer1)< 0)
		{
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_refer1.attr.name);
		}
	
		if (device_create_file(qt602240_noise_test, &dev_attr_set_delta1) < 0)
		{
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_delta1.attr.name);
		}
	
		if (device_create_file(qt602240_noise_test, &dev_attr_set_refer2)< 0)
		{
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_refer2.attr.name);
		}
	
		if (device_create_file(qt602240_noise_test, &dev_attr_set_delta2) < 0)
		{
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_delta2.attr.name);
		}
	
		if (device_create_file(qt602240_noise_test, &dev_attr_set_refer3)< 0)
		{
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_refer3.attr.name);
		}
	
		if (device_create_file(qt602240_noise_test, &dev_attr_set_delta3) < 0)
		{
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_delta3.attr.name);
		}
	
		if (device_create_file(qt602240_noise_test, &dev_attr_set_refer4)< 0)
		{
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_refer4.attr.name);
		}
	
		if (device_create_file(qt602240_noise_test, &dev_attr_set_delta4) < 0)
		{
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_delta4.attr.name);
		}

		if (device_create_file(qt602240_noise_test, &dev_attr_set_all_refer)< 0)
		{
			printk("Failed to create device file(%s)!\n", dev_attr_set_all_refer.attr.name);		
		}			

		if (device_create_file(qt602240_noise_test, &dev_attr_set_threshould) < 0)
		{
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_threshould.attr.name);
		}

		if (device_create_file(qt602240_noise_test, &dev_attr_set_firm_version) < 0)
		{
			printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_set_firm_version.attr.name);
		}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = mxt224_early_suspend;
	data->early_suspend.resume = mxt224_late_resume;
	register_early_suspend(&data->early_suspend);
#endif

	mxt224_enabled = 1;

	return 0;

err_irq:
err_reset:
err_backup:
err_config:
	kfree(data->objects);
err_init_drv:
	gpio_free(data->gpio_read_done);
//err_gpio_req:
	//data->power_off();
	//input_unregister_device(input_dev);
err_reg_dev:
err_alloc_dev:
	kfree(data);
	return ret;
}

static int __devexit mxt224_remove(struct i2c_client *client)
{
	struct mxt224_data *data = i2c_get_clientdata(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&data->early_suspend);
#endif
	free_irq(client->irq, data);
	kfree(data->objects);
	gpio_free(data->gpio_read_done);
	data->power_off();
	input_unregister_device(data->input_dev);
	kfree(data);

	return 0;
}

static struct i2c_device_id mxt224_idtable[] = {
	{MXT224_DEV_NAME, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, mxt224_idtable);

static const struct dev_pm_ops mxt224_pm_ops = {
	.suspend = mxt224_suspend,
	.resume = mxt224_resume,
};

static struct i2c_driver mxt224_i2c_driver = {
	.id_table = mxt224_idtable,
	.probe = mxt224_probe,
	.remove = __devexit_p(mxt224_remove),
	.driver = {
		.owner	= THIS_MODULE,
		.name	= MXT224_DEV_NAME,
		.pm	= &mxt224_pm_ops,
	},
};

static int __init mxt224_init(void)
{
	return i2c_add_driver(&mxt224_i2c_driver);
}

static void __exit mxt224_exit(void)
{
	i2c_del_driver(&mxt224_i2c_driver);
}
module_init(mxt224_init);
module_exit(mxt224_exit);

MODULE_DESCRIPTION("Atmel MaXTouch 224 driver");
MODULE_AUTHOR("Rom Lemarchand <rlemarchand@sta.samsung.com>");
MODULE_LICENSE("GPL");
