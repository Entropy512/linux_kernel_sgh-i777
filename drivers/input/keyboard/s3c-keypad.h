/* linux/drivers/input/keyboard/s3c-keypad.h
 *
 * Driver header for Samsung SoC keypad.
 *
 * Kim Kyoungil, Copyright (c) 2006-2009 Samsung Electronics
 *      http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef _S3C_KEYPAD_H_
#define _S3C_KEYPAD_H_

static void __iomem *key_base;

#define KEYPAD_COLUMNS	8
#define KEYPAD_ROWS	14
#define MAX_KEYPAD_NR	112 /* 8*14 */
#define MAX_KEYMASK_NR	56

#if 0
int keypad_keycode[] = {
	1,				2,		KEY_1,	KEY_Q,	KEY_A,	6,			7,			KEY_LEFT,		64, 65, 66, 67, 68, 69,
	9,				10,		KEY_2,	KEY_W,	KEY_S,	KEY_Z,    	KEY_RIGHT,	16,				70, 71, 72, 73, 74, 75,
	17,				18,		KEY_3,	KEY_E,	KEY_D,	KEY_X,		23,			KEY_UP,			76, 77, 78, 79, 80, 81,
	25,				26,		KEY_4,	KEY_R,	KEY_F,	KEY_C,		31,			32,				82, 83, 84, 85, 86, 87,
	33,				KEY_O,	KEY_5,	KEY_T,	KEY_G,	KEY_V,		KEY_DOWN,	KEY_BACKSPACE,	88, 89, 90, 91, 92, 93,
	KEY_P,    		KEY_0,	KEY_6,	KEY_Y,	KEY_H,	KEY_SPACE,	47,			 48,				94, 95, 96, 97, 98, 99,
	KEY_M,			KEY_L,	KEY_7,	KEY_U,	KEY_J,	KEY_N,    	55,	 		KEY_ENTER,		100, 101, 102, 103, 104, 105,
	KEY_LEFTSHIFT,	KEY_9,	KEY_8,	KEY_I,	KEY_K,	KEY_B,    	63,			KEY_COMMA,		106, 107, 108, 109, 110, 111,
};
#endif

int keypad_keycode[] = {
	1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
	15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,
	29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,
	KEY_UP,  KEY_END,  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,
	KEY_REPLY,  KEY_BACK,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,
	KEY_DOWN,  KEY_VOLUMEUP,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,
	KEY_RIGHT,  KEY_MENU,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,
	KEY_LEFT,  KEY_VOLUMEDOWN, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, KEY_POWER,
};

#if defined(CONFIG_CPU_S3C6410)
#define KEYPAD_DELAY		(50)
#elif defined(CONFIG_CPU_S5PC100)
#define KEYPAD_DELAY		(600)
#elif defined(CONFIG_CPU_S5PV210) || defined(CONFIG_CPU_S5PV310)
#define KEYPAD_DELAY		(900)
#elif defined(CONFIG_CPU_S5P6442)
#define KEYPAD_DELAY		(50)
#endif

#define	KEYIFCOL_CLEAR		(readl(key_base+S3C_KEYIFCOL) & ~0xffff)
#define	KEYIFCON_CLEAR		(readl(key_base+S3C_KEYIFCON) & ~0x1f)
#define	KEYIFFC_DIV		(readl(key_base+S3C_KEYIFFC) | 0x1)

struct s3c_keypad {
	struct input_dev *dev;
	int nr_rows;
	int no_cols;
	int total_keys;
	int keycodes[MAX_KEYPAD_NR];
};

extern void s3c_setup_keypad_cfg_gpio(int rows, int columns);
#endif				/* _S3C_KEYIF_H_ */
