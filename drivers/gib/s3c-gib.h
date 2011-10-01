/* ------------------------------------------------------------------------- */
/* 									     */
/* s3c-gib.h - definitions of s3c24xx specific gib interface	     */
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

#ifndef _S3C24XX_GIB_H
#define _S3C24XX_GIB_H


struct s3c_gib {
	int 			nr;
	void __iomem		*regs;
	struct device		*dev;
	struct gib_dev		gibdev;
};



#endif /* _S3C24XX_GIB_H */
