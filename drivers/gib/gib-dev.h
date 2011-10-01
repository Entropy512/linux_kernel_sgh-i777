/* ------------------------------------------------------------------------- */
/* 									     */
/* gib-dev.h - definitions for the GPS interface			     */
/* 									     */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2010 Samsung Electronics Co. ltd.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.		     */
/* ------------------------------------------------------------------------- */

#ifndef _LINUX_GIB_H
#define _LINUX_GIB_H

#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>	/* for struct device */
#include <linux/semaphore.h>

/* --- General options ------------------------------------------------	*/

struct gib_dev;

/*
 * A driver is capable of handling one or more physical devices present on
 * GIB adapters. This information is used to inform the driver of adapter
 * events.
 */

struct gib_dev {
	int 			minor;
	int 			ubp_debug_flag;
	struct gib_reset 	*g_reset;
	struct gib_udp_debug 	*g_udp_debug;
	struct device		dev;		/* the adapter device 			*/
};

struct gib_reset {
	int (*core_reset)(unsigned int);
};

struct gib_udp_debug {
	int (*ubp_debug)( unsigned int);
};

extern int gib_attach_gibdev(struct gib_dev *);
extern int gib_detach_gibdev(struct gib_dev *);


#define SET_BB_RESET_LOW	0x0
#define SET_BB_RESET_HIGH	0x1
#define SET_UBP_DEBUG_MODE 0x80
#define SET_UBP_NO_DEBUG_MODE 0x81

#define GIB_MAJOR	210		/* Device major number		*/

#endif /* _LINUX_GIB_H */

