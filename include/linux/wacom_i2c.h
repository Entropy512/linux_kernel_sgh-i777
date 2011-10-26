#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <asm/gpio.h>
/*#include <plat/gpio-cfg.h>*/
#include <linux/irq.h>
#include <asm/irq.h>
#include <linux/delay.h>

#define NAMEBUF 12
#define WACNAME "WAC_I2C_EMR"
#define WACFLASH "WAC_I2C_FLASH"
#define LATEST_VERSION 0x15

/*Wacom Command*/
#define COM_COORD_NUM      10
#define COM_SAMPLERATE_40  0x33
#define COM_SAMPLERATE_80  0x32
#define COM_SAMPLERATE_133 0x31
#define COM_SURVEYSCAN     0x2B
#define COM_QUERY          0x2A
#define COM_FLASH          0xff

/*I2C address for digitizer and its boot loader*/
#define WACOM_I2C_ADDR 0x56
#define WACOM_I2C_BOOT 0x57

/*Information for input_dev*/
#define EMR 0
#define WACOM_PKGLEN_I2C_EMR 0

/*Enable/disable irq*/
#define ENABLE_IRQ 1
#define DISABLE_IRQ 0

/*Special keys*/
#define EPEN_TOOL_PEN		0x220
#define EPEN_TOOL_RUBBER	0x221
#define EPEN_STYLUS			0x22b
#define EPEN_STYLUS2		0x22c

//#define EMR_INT IRQ_EINT_GROUP(6, 2)	// group 6 : D2
//#define PDCT_INT GPIO_ACCESSORY_INT
//#define GPIO_PEN_SLP S5PV210_GPH2(3)

/*Parameters for wacom own features*/
struct wacom_features{
  int x_max;
  int y_max;
  int pressure_max;
  char comstat;
  char fw_version;
  u8 data[COM_COORD_NUM];
};

static struct wacom_features wacom_feature_EMR = {
 16128,
 8448,
 256,
 COM_QUERY,
 0x10,
 {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,},
};

struct wacom_g5_callbacks {
	int (*check_prox)(struct wacom_g5_callbacks *);
};

struct wacom_g5_platform_data {
	int x_invert;
	int y_invert;
	int xy_switch;
	void  (*init_platform_hw)(void);
	void  (*exit_platform_hw)(void);
	void  (*suspend_platform_hw)(void);
	void  (*resume_platform_hw)(void);
	void (*register_cb)(struct wacom_g5_callbacks *);
};

/*Parameters for i2c driver*/
struct wacom_i2c {
  struct i2c_client *client;
  struct input_dev *input_dev;
  struct mutex lock;
  int irq;
  int pen_pdct;
  int gpio;
  int irq_flag;
  int pen_prox;
  int pen_pressed;
  int side_pressed;
  int tool;
  const char name[NAMEBUF];
  struct wacom_features *wac_feature;
  struct wacom_g5_platform_data *wac_pdata;
  struct wacom_g5_callbacks callbacks;
  int (*power)(int on);
};

