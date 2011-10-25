/*
 * Driver for S5K6AAFX (VGA camera) from SAMSUNG ELECTRONICS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */
#define DEFAULT_WIDTH		640
#define DEFAULT_HEIGHT		480

struct s5k6aafx_platform_data {
	unsigned int default_width;
	unsigned int default_height;
	unsigned int pixelformat;
#if defined (CONFIG_VIDEO_S5K6AAFX_MIPI)
	int freq;	/* MCLK in KHz */
	/* This SoC supports Parallel & CSI-2 */
	int is_mipi;
#endif
};
