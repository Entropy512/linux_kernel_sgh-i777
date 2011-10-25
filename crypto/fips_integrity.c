/*
 * Integrity check code for crypto module.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 */
#include <crypto/hash.h>
#include <crypto/sha.h>
#include <linux/err.h>
#include <linux/scatterlist.h>

#include "internal.h"

static bool integrity_checked = false;

#define ZIMAGE_START (0x60000000)

void do_integrity_check(void) {
	u8* rbuf=__va(ZIMAGE_START);
	u32 len;
	u8 hmac[SHA256_DIGEST_SIZE];
	struct hash_desc desc;
	struct scatterlist sg;
	u8* key="12345678";

	printk(KERN_INFO "do kernel integrity check\n");

	if (integrity_checked || in_fips_err()) return;

	if ( *((u32*) &rbuf[36]) != 0x016F2818) {
		printk(KERN_ERR "integ: invalid zImage magic number.");
		set_in_fips_err();
		goto err;
	}

	len = *(u32*)&rbuf[44] - *(u32*)&rbuf[40];

	if (len < 0) {
                printk(KERN_ERR "integ: invalid zImage calculated len");
                set_in_fips_err();
                goto err;
	}

	desc.tfm = crypto_alloc_hash("hmac(sha256)",0,0);

	if (IS_ERR(desc.tfm)) {
		printk(KERN_ERR "integ: failed to allocate tfm %ld\n",PTR_ERR(desc.tfm));
		set_in_fips_err();
		goto err;	
	}

	sg_init_one(&sg, rbuf, len);
	crypto_hash_setkey(desc.tfm,key,strlen(key));
	crypto_hash_digest(&desc,&sg,len,hmac);	

	if (!strncmp(hmac,&rbuf[len],SHA256_DIGEST_SIZE)) {
		printk(KERN_INFO "integrity check passed");
	} else {
		printk(KERN_ERR "integrity check failed");
		set_in_fips_err();
	}

err:
	integrity_checked=true;
	crypto_free_hash(desc.tfm);

	return;
}

