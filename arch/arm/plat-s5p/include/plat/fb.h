/* linux/arch/arm/plat-s5p/include/plat/fb.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Core file for Samsung Display Controller (FIMD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_PLAT_FB_H
#define __ASM_PLAT_FB_H __FILE__

#define FB_SWAP_WORD	(1 << 24)
#define FB_SWAP_HWORD	(1 << 16)
#define FB_SWAP_BYTE	(1 << 8)
#define FB_SWAP_BIT	(1 << 0)

struct platform_device;
struct clk;

#ifdef CONFIG_FB_S3C_MIPI_LCD
/* enumerates display mode. */
enum {
	SINGLE_LCD_MODE = 1,
	DUAL_LCD_MODE = 2,
};

/* enumerates interface mode. */
enum {
	FIMD_RGB_INTERFACE = 1,
	FIMD_CPU_INTERFACE = 2,
};
#endif

struct s3c_platform_fb {
	int		hw_ver;
	char		clk_name[16];
	int		nr_wins;
	int		nr_buffers[5];
	int		default_win;
	int		swap;
	void		*lcd;
#ifdef CONFIG_FB_S3C_MIPI_LCD
	unsigned int	sub_lcd_enabled;
	unsigned int	machine_is_cypress;
	unsigned int	machine_is_p1p2;
	unsigned int	mdnie_is_enabled;
	unsigned int	mipi_is_enabled;
	unsigned int	interface_mode;

	void		*single_lcd;
	void		*dual_lcd;

	void		(*set_display_path)(unsigned int mode);
	int		(*reset_lcd)(void);

	/* variables and interface for mDNIe */
	char		mdnie_clk_name[20];
	void		*mdnie_clk;
	unsigned int	mdnie_phy_base;
	unsigned int	ielcd_phy_base;
	void __iomem	*mdnie_mmio_base;
	void __iomem	*ielcd_mmio_base;
	unsigned char	mdnie_mode;

	void		(*set_mdnie_clock)(void *mdnie_clk, unsigned char enable);
	void		(*init_mdnie)(unsigned int mdnie_base,
				unsigned int hsize, unsigned int vsize);
	void		(*mdnie_set_mode)(unsigned int mdnie_base, unsigned char mode);

	void		(*start_ielcd_logic)(unsigned int ielcd_base);
	void		(*init_ielcd)(unsigned int ielcd_base, void *l, void *c);
#endif
	void		(*cfg_gpio)(struct platform_device *dev);
	void		(*cfg_gpio_sleep)(struct platform_device *dev);	
	int		(*backlight_on)(struct platform_device *dev);
	int		(*backlight_off)(struct platform_device *dev);
	int		(*lcd_on)(struct platform_device *dev);
	int		(*lcd_off)(struct platform_device *dev);
	int		(*clk_on)(struct platform_device *pdev, struct clk **s3cfb_clk);
	int		(*clk_off)(struct platform_device *pdev, struct clk **clk);
};

extern void s3cfb_set_platdata(struct s3c_platform_fb *fimd);

/* defined by architecture to configure gpio */
extern void s3cfb_cfg_gpio(struct platform_device *pdev);
extern void s3cfb_cfg_gpio_sleep(struct platform_device *pdev);
extern int s3cfb_backlight_on(struct platform_device *pdev);
extern int s3cfb_backlight_off(struct platform_device *pdev);
extern int s3cfb_lcd_on(struct platform_device *pdev);
extern int s3cfb_lcd_off(struct platform_device *pdev);
extern int s3cfb_clk_on(struct platform_device *pdev, struct clk **s3cfb_clk);
extern int s3cfb_clk_off(struct platform_device *pdev, struct clk **clk);
extern void s3cfb_get_clk_name(char *clk_name);
#endif /* __ASM_PLAT_FB_H */
