/*
 * Copyright (C) 2010 Samsung Electronics
 * Minkyu Kang <mk7.kang@samsung.com>
 * Wonguk Jeong <wonguk.jeong@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef _FSA9480_H_
#define _FSA9480_H_

#define FSA9480_ATTACHED	1
#define FSA9480_DETACHED	0

struct fsa9480_platform_data {
	void (*cfg_gpio) (void);
	void (*usb_cb) (u8 attached);
	void (*uart_cb) (u8 attached);
	void (*charger_cb) (u8 attached);
	void (*jig_cb) (u8 attached);
	void (*reset_cb) (void);
};

void fsa9480_set_switch(const char *buf);
ssize_t fsa9480_get_switch(char *buf);

#endif /* _FSA9480_H_ */
