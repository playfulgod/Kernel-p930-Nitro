/* drivers/video/msm/logo.c
 *
 * Show Logo in RLE 565 format
 *
 * Copyright (C) 2008 Google Incorporated
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fb.h>
#include <linux/vt_kern.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>

#include <linux/irq.h>
#include <asm/system.h>

/* neo.kang@lge.com 2011-09-08, for hidden reset */
#if defined(CONFIG_LGE_HIDDEN_RESET_PATCH)
#include "../../../lge/include/board_lge.h"
#endif

#define fb_width(fb)	((fb)->var.xres)
#define fb_height(fb)	((fb)->var.yres)
#define fb_size(fb)	((fb)->var.xres * (fb)->var.yres * 2)

static void memset16(void *_ptr, unsigned short val, unsigned count)
{
	unsigned short *ptr = _ptr;
	count >>= 1;
	while (count--)
		*ptr++ = val;
}
#ifdef CONFIG_LGE_I_DISP_BOOTLOGO
static void memset32(void *_ptr, unsigned short val, unsigned count)
{
	char *ptr = _ptr;
    char r = val & 0x001f;
    char g = (val & 0x07e0)>>5;
    char b = (val & 0xf800)>>11;
	count >>= 1;
	while (count--){
		*ptr++ = b<<3 | b>>2;
		*ptr++ = g<<2 | g>>4;
		*ptr++ = r<<3 | r>>2;
		*ptr++ = 0xff;
    }
}
#endif

/* 565RLE image format: [count(2 bytes), rle(2 bytes)] */
int load_565rle_image(char *filename)
{
	struct fb_info *info;
	int fd, count, err = 0;
	unsigned max;
#ifdef CONFIG_LGE_I_DISP_BOOTLOGO
	unsigned short *data, *ptr;
    char *bits;
#else
	unsigned short *data, *bits, *ptr;
#endif
/* neo.kang@lge.com 2011-09-08, for hidden reset */
#if defined(CONFIG_LGE_HIDDEN_RESET_PATCH)
	int i;
	unsigned char *src;
#endif

	info = registered_fb[0];
	if (!info) {
		printk(KERN_WARNING "%s: Can not access framebuffer\n",
			__func__);
		return -ENODEV;
	}

/* neo.kang@lge.com 2011-09-08, for hidden reset */
#if defined(CONFIG_LGE_HIDDEN_RESET_PATCH)
	bits = (unsigned char *)info->screen_base;
	src = (unsigned char *)lge_get_fb_copy_virt_addr();

	if( on_hidden_reset == 1 ) {
		for( i=0; i<1280*736*4; i++)
			*bits++ = *src++;

		printk("%s: %x\n", __func__, (unsigned int)info->screen_base);
		return err;
	}
#endif

	fd = sys_open(filename, O_RDONLY, 0);
	if (fd < 0) {
		printk(KERN_WARNING "%s: Can not open %s\n",
			__func__, filename);
		return -ENOENT;
	}
	count = sys_lseek(fd, (off_t)0, 2);
	if (count <= 0) {
		err = -EIO;
		goto err_logo_close_file;
	}
	sys_lseek(fd, (off_t)0, 0);
	data = kmalloc(count, GFP_KERNEL);
	if (!data) {
		printk(KERN_WARNING "%s: Can not alloc data\n", __func__);
		err = -ENOMEM;
		goto err_logo_close_file;
	}
	if (sys_read(fd, (char *)data, count) != count) {
		err = -EIO;
		goto err_logo_free_data;
	}

	max = fb_width(info) * fb_height(info);
	ptr = data;
#ifdef CONFIG_LGE_I_DISP_BOOTLOGO
	bits = (char *)(info->screen_base);
#else
	bits = (unsigned short *)(info->screen_base);
#endif
	while (count > 3) {
		unsigned n = ptr[0];
		if (n > max)
			break;
#ifdef CONFIG_LGE_I_DISP_BOOTLOGO
        if(info->var.bits_per_pixel/8 == 4){
            memset32(bits, ptr[1], n << 1);
        }
        else{
            memset16(bits, ptr[1], n << 1);
        }
        bits += info->var.bits_per_pixel/8*n;
#else
		memset16(bits, ptr[1], n << 1);
		bits += n;
#endif

		max -= n;
		ptr += 2;
		count -= 4;
	}

err_logo_free_data:
	kfree(data);
err_logo_close_file:
	sys_close(fd);
	return err;
}
EXPORT_SYMBOL(load_565rle_image);
