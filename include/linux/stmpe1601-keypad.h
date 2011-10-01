/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License, version 2
 * Author: Rabin Vincent <rabin.vincent@stericsson.com> for ST-Ericsson
 */

#ifndef __STMPE_H
#define __STMPE_H


/*
 * STMPE1601
 */

/* System Regster */
#define STMPE1601_REG_CHIP_ID                    0x80
#define STMPE1601_REG_VERSION_ID                 0x81
#define STMPE1601_REG_SYS_CTRL                   0x02
#define STMPE1601_REG_SYS_CTRL2                  0x03

/* Interrupt Regster */
#define STMPE1601_REG_INT_CTRL_MSB               0x10
#define STMPE1601_REG_INT_CTRL_LSB               0x11
#define STMPE1601_REG_INT_EN_MASK_MSB            0x12
#define STMPE1601_REG_INT_EN_MASK_LSB            0x13
#define STMPE1601_REG_INT_STA_MSB                0x14
#define STMPE1601_REG_INT_STA_LSB                0x15
#define STMPE1601_REG_INT_EN_GPIO_MASK_MSB       0x16
#define STMPE1601_REG_INT_EN_GPIO_MASK_LSB       0x17
#define STMPE1601_REG_INT_STA_GPIO_MSB           0x18
#define STMPE1601_REG_INT_STA_GPIO_LSB           0x19

/* GPIO Controller Regster */
#define STMPE1601_GPIO_SET_MSB                   0x82
#define STMPE1601_GPIO_SET_LSB                   0x83
#define STMPE1601_GPIO_CLR_MSB                   0x84
#define STMPE1601_GPIO_CLR_LSB                   0x85
#define STMPE1601_GPIO_MP_MSB                    0x86
#define STMPE1601_GPIO_MP_LSB                    0x87
#define STMPE1601_GPIO_SET_DIR_MSB               0x88
#define STMPE1601_GPIO_SET_DIR_LSB               0x89
#define STMPE1601_GPIO_ED_MSB                    0x8a
#define STMPE1601_GPIO_ED_LSB                    0x8b
#define STMPE1601_GPIO_RE_MSB                    0x8c
#define STMPE1601_GPIO_RE_LSB                    0x8d
#define STMPE1601_GPIO_FE_MSB                    0x8e
#define STMPE1601_GPIO_FE_LSB                    0x8f
#define STMPE1601_GPIO_PULL_UP_MSB               0x90
#define STMPE1601_GPIO_PULL_UP_LSB               0x91
#define STMPE1601_GPIO_AF_U_MSB                  0x92
#define STMPE1601_GPIO_AF_U_LSB                  0x93
#define STMPE1601_GPIO_AF_L_MSB                  0x94
#define STMPE1601_GPIO_AF_L_LSB                  0x95
#define STMPE1601_GPIO_LT_EN                     0x96
#define STMPE1601_GPIO_LT_DIR                    0x97

/* Keypad Controller Regster */
#define STMPE1601_REG_KPC_COL                    0x60
#define STMPE1601_REG_KPC_ROW_MSB                0x61
#define STMPE1601_REG_KPC_ROW_LSB                0x62
#define STMPE1601_REG_KPC_CTRL_MSB               0x63
#define STMPE1601_REG_KPC_CTRL_LSB               0x64
#define STMPE1601_REG_KPC_COMBI_KEY_0            0x65
#define STMPE1601_REG_KPC_COMBI_KEY_1            0x66
#define STMPE1601_REG_KPC_COMBI_KEY_2            0x67
#define STMPE1601_REG_KPC_DATA_BYTE0             0x68
#define STMPE1601_REG_KPC_DATA_BYTE1             0x69
#define STMPE1601_REG_KPC_DATA_BYTE2             0x6A
#define STMPE1601_REG_KPC_DATA_BYTE3             0x6B
#define STMPE1601_REG_KPC_DATA_BYTE4             0x6C

/* Defines */
/* Macro */
#define STMPE1601_SYS_CTRL_EN_GPIO              (1 << 3)
#define STMPE1601_SYS_CTRL_EN_KPC               (1 << 1)
#define STMPE1601_SYSCON_EN_SPWM                (1 << 0)

#define STMPE1601_AHRE_INT                      (1 << 2)
#define STMPE1601_EDGE_INT                      (1 << 1)
#define STMPE1601_GIM_SET                       (1 << 0)
#define STPME1601_KPC_INT_EN                    (1 << 1)
#define STPME1601_KPC_FIFO_OVR_EN               (1 << 2)
#define STPME1601_AUTOSLEEP_EN                  (1 << 3)
#define STMPE1601_SCAN_EN                       (1 << 0)

#define STMPE1601_GPIO_ALT_Fn_KPAD              0x55
#define STMPE1601_COL_SCAN_EN_WRD               0xFF
#define STMPE1601_ROW_SCAN_EN_WRD               0xFF
#define STMPE1601_ROW_SCAN_EN_WRD               0xFF

#define KEYPAD_LED_TIMEOUT                      5 //in sec
#define STMPE1601_KPAD_MAX_DEBOUNCE             127 //in msec
#define STMPE1601_KPAD_MAX_SCAN_COUNT           15
#define STMPE1601_KPAD_MAX_ROWS                 8
#define STMPE1601_KPAD_MAX_COLS                 8
#define STMPE1601_KPAD_KEYMAP_SIZE   (STMPE1601_KPAD_MAX_ROWS * STMPE1601_KPAD_MAX_COLS)

#define STMPE1601_CHIP_ID                       0x02
#define STMPE1601_KPAD_ROW_SHIFT                3
#define STMPE1601_KPC_DATA_NOKEY_MASK           0x78

#define STMPE1601_KPC_DATA_LEN                  5
#define STMPE1601_KPC_DATA_UP                  (0x1 << 7)
#define STMPE1601_KPC_DATA_ROW                 (0xf << 3)
#define STMPE1601_KPC_DATA_COL                 (0x7 << 0)
#define STMPE1601_KPC_HYBER_EN                 (1 << 5)

/* GPIO from AP */
#define STMPE1601_KPAD_INT		GPIO_KEY_INT
#define KEYPAD_LED_CTRL_SW		GPIO_KEYPAD_LED_EN
#define KEYPAD_SLIDE_SW			GPIO_HALL_SW

/* Standard Defn */
#define GPIO_LEVEL_LOW		0
#define GPIO_LEVEL_HIGH		1


enum stmpe1601_ctrls {
	STMPE_CTRL_PWM     = 1 << 0,
	STMPE_CTRL_KEYPAD  = 1 << 1,
	STMPE_CTRL_GPIO    = 1 << 2,
	STMPE_CTRL_ALL     = 1 << 3,
};

struct stmpe1601_kpad {
	struct device *dev;
	struct i2c_client *i2c;
	struct input_dev *input;
	u8 partnum;
	unsigned short keycode[64];
	const struct stmpe1601_kpad_platform_data *plat;
	struct workqueue_struct *kpad_workqueue;
	struct hrtimer timer;
};

struct stmpe1601_kpad_platform_data {
	unsigned int rows;
	unsigned int cols;
	unsigned int gpio_irq;
	unsigned int irq_trigger;
	struct matrix_keymap_data *keymap;
	unsigned int repeat;
	unsigned int debounce_ms;
	unsigned int scan_count;
	unsigned int autosleep_timeout;
#ifdef CONFIG_MACH_C1_NA_SPR_EPIC2_REV00
	int slide_gpio;
	int slide_irq;
#endif
};
#endif
