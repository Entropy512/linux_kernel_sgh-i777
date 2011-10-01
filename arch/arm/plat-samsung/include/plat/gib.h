#ifndef __PLAT_S3C_GIB_H
#define __PLAT_S3C_GIB_H __FILE__

#define S3C_GIB_MAX_WIN	(5)


struct s3c_gib_platdata {
	int		bus_num;
	unsigned int	flags;
	unsigned int	slave_addr;
	unsigned long	frequency;
	unsigned int	sda_delay;

	void	(*cfg_gpio)(struct platform_device *dev);
};

extern void s3c_gib_set_platdata(struct s3c_gib_platdata *pd);

#endif /* __PLAT_S3C_GIB_H */
