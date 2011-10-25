/*
 * drivers/usb/core/s3cotg_whitelist.h
 *
 * Copyright (C) 2004 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * This OTG Whitelist is the OTG "Targeted Peripheral List".  It should
 * mostly use of USB_DEVICE() or USB_DEVICE_VER() entries..
 *
 * YOU _SHOULD_ CHANGE THIS LIST TO MATCH YOUR PRODUCT AND ITS TESTING!
 */

static struct usb_device_id s3cotg_whitelist_table[] = {
	{ USB_DEVICE(0x1d6b, 0x0002), },	/* S3COTG HCD */
	{ USB_DEVICE(0x0525, 0xa4a1), },	/* Root hub for S3COTG */
	{ USB_DEVICE_INFO(0xef, 0x2, 0x1) },
#ifdef	CONFIG_USB_PRINTER		/* ignoring nonstatic linkage! */
	{ USB_DEVICE_INFO(0x7, 0x1, 0x1) },
	{ USB_DEVICE_INFO(0x7, 0x1, 0x2) },
	{ USB_DEVICE_INFO(0x7, 0x1, 0x3) },
#endif
#ifdef	CONFIG_USB_STORAGE
	{ USB_DEVICE_INFO(0x8, 0x6, 0x50) },
	{ USB_DEVICE_INFO(0x0, 0x0, 0x0) },
#endif
{ }	/* Terminating entry */
};

static int s3cotg_is_targeted(struct usb_device *dev)
{
	struct usb_device_id	*id = s3cotg_whitelist_table;

	/* otg whitelist is only applied to otg host bus.
	 * We have to find more generic method :( */
	if (dev->bus->busnum == 1)
		return 1;

	/* NOTE: can't use usb_match_id() since interface caches
	 * aren't set up yet. this is cut/paste from that code.
	 */
	for (id = s3cotg_whitelist_table; id->match_flags; id++) {
		if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
		    id->idVendor != le16_to_cpu(dev->descriptor.idVendor))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
		    id->idProduct != le16_to_cpu(dev->descriptor.idProduct))
			continue;

		/* No need to test id->bcdDevice_lo != 0, since 0 is never
		   greater than any unsigned number. */
		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
		    (id->bcdDevice_lo > le16_to_cpu(dev->descriptor.bcdDevice)))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
		    (id->bcdDevice_hi < le16_to_cpu(dev->descriptor.bcdDevice)))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
		    (id->bDeviceClass != dev->descriptor.bDeviceClass))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
		    (id->bDeviceSubClass != dev->descriptor.bDeviceSubClass))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
		    (id->bDeviceProtocol != dev->descriptor.bDeviceProtocol))
			continue;

		pr_info("device v%04x p%04x is supported. (Class = %x SubClass=%x Protocol=%x)\n",
			le16_to_cpu(dev->descriptor.idVendor),
			le16_to_cpu(dev->descriptor.idProduct),
			dev->descriptor.bDeviceClass,
			dev->descriptor.bDeviceSubClass,
			dev->descriptor.bDeviceProtocol);

		return 1;
	}

	/* add other match criteria here ... */

	/* OTG MESSAGE: report errors here, customize to match your product */
	pr_info("device v%04x p%04x is not supported. (Class = %x SubClass=%x Protocol=%x)\n",
		le16_to_cpu(dev->descriptor.idVendor),
		le16_to_cpu(dev->descriptor.idProduct),
		dev->descriptor.bDeviceClass,
		dev->descriptor.bDeviceSubClass,
		dev->descriptor.bDeviceProtocol);

	return 0;
}

