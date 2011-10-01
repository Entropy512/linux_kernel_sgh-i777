/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define DEFAULT_FMT		V4L2_PIX_FMT_UYVY	/* YUV422 */
#define DEFAULT_PREVIEW_WIDTH		640
#define DEFAULT_PREVIEW_HEIGHT		480
#define DEFAULT_CAPTURE_WIDTH		1280
#define DEFAULT_CAPTURE_HEIGHT		960

struct sr130pc10_platform_data {
	unsigned int default_width;
	unsigned int default_height;
	unsigned int pixelformat;
	int freq;	/* MCLK in KHz */
	/* This SoC supports Parallel & CSI-2 */
	int is_mipi;
};

