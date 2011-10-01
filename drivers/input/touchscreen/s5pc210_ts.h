/*
 *
 * S5PV310_TS_H_ Header file
 *
 */
#ifndef	_S5PV310_TS_H_
#define	_S5PV310_TS_H_

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#define S5PV310_TS_DEVICE_NAME 	"s5pc210_ts"

#define	TOUCH_PRESS             1
#define	TOUCH_RELEASE           0

/* Touch Configuration */

#define GPX3DAT (S5PV310_VA_GPIO2 + 0xC64)
#define EINT_43CON (S5PV310_VA_GPIO2 + 0xE0C)

/* Touch Interrupt define */
#define	S5PV310_TS_IRQ          gpio_to_irq(S5PV310_GPX3(5))

#define	TS_ABS_MIN_X            0
#define	TS_ABS_MIN_Y            0
#define	TS_ABS_MAX_X		1366
#define	TS_ABS_MAX_Y		766

#define	TS_X_THRESHOLD		1
#define	TS_Y_THRESHOLD		1

#define	TS_ATTB			(S5PV310_GPX3(5))

/* Interrupt Check port */
#define	GET_INT_STATUS()	(((*(unsigned long *)GPX3DAT) & 0x01) ? 1 : 0)

/* touch register */
#define	MODULE_CALIBRATION	0x37
#define	MODULE_POWERMODE	0x14
#define	MODULE_INTMODE		0x15
#define	MODULE_INTWIDTH		0x16

#define	PERIOD_10MS		(HZ/100) /* 10ms */
#define	PERIOD_20MS		(HZ/50)	 /* 20ms */
#define	PERIOD_50MS		(HZ/20)	 /* 50ms */

#define	TOUCH_STATE_BOOT	0
#define	TOUCH_STATE_RESUME	1

/* Touch hold event */
#define	SW_TOUCH_HOLD		0x09

#if defined(CONFIG_TOUCHSCREEN_S5PV310_MT)

/* multi-touch data process struct */
typedef struct	touch_process_data__t	{
    unsigned 	char	finger_cnt;
    unsigned 	int		x1;
    unsigned 	int		y1;
    unsigned 	int		x2;
    unsigned 	int		y2;
} touch_process_data_t;

#endif

typedef struct s5pv310_ts__t {

    struct input_dev *driver;

    /* seqlock_t */
    seqlock_t				lock;
    unsigned int			seq;

    /* timer */
    struct  timer_list		penup_timer;

    /* data store */
    unsigned int			status;
    unsigned int			x;
    unsigned int			y;

    /* i2c read buffer */
    unsigned char			rd[10];

    /* sysfs used */
    unsigned char			hold_status;

    unsigned char			sampling_rate;

    /* x data threshold (0-10) : default 3 */
    unsigned char			threshold_x;
    /* y data threshold (0-10) : default 3 */
    unsigned char			threshold_y;
    /* touch sensitivity (0-255) : default 0x14 */
    unsigned char			sensitivity;

#ifdef CONFIG_TOUCHSCREEN_S5PV310_MT

#ifdef CONFIG_HAS_EARLYSUSPEND
    struct	early_suspend		power;
#endif/* CONFIG_HAS)EARLYSUSPEND */
#endif/* CONFIG_TOUCHSCREEN_S5PV310_MT */

} s5pv310_ts_t;

extern s5pv310_ts_t s5pv310_ts;
#endif		/* _HKC100_TOUCH_H_ */
