#ifndef _ALLEGRO_CONVERT_H
#define _ALLEGRO_CONVERT_H

#include "allegro5/allegro5.h"

/* Conversionf for most pixels */
#define ALLEGRO_SHIFT_CONVERT(p, \
		a_mask, a_lshift, a_rshift, \
		r_mask, r_lshift, r_rshift, \
		g_mask, g_lshift, g_rshift, \
		b_mask, b_lshift, b_rshift) \
	((((p & a_mask) >> a_rshift) << a_lshift) | \
	 (((p & r_mask) >> r_rshift) << r_lshift) | \
	 (((p & g_mask) >> g_rshift) << g_lshift) | \
	 (((p & b_mask) >> b_rshift) << b_lshift))

/* Converting 565 to 888 */
#define ALLEGRO_SHIFT_CONVERT_565(p, \
		r_mask, r_lshift, r_rshift, \
		g_mask, g_lshift, g_rshift, \
		b_mask, b_lshift, b_rshift) \
	((_rgb_scale_5[((p & r_mask) >> r_rshift)] << r_lshift) | \
	 (_rgb_scale_6[((p & g_mask) >> g_rshift)] << g_lshift) | \
	 (_rgb_scale_5[((p & b_mask) >> b_rshift)] << b_lshift))

/* Converting 555(1) to 888 */
#define ALLEGRO_SHIFT_CONVERT_555(p, \
		a_mask, a_lshift, a_rshift, \
		r_mask, r_lshift, r_rshift, \
		g_mask, g_lshift, g_rshift, \
		b_mask, b_lshift, b_rshift) \
	((_rgb_scale_1[((p & a_mask) >> a_rshift)] << a_lshift) | \
	 (_rgb_scale_5[((p & r_mask) >> r_rshift)] << r_lshift) | \
	 (_rgb_scale_5[((p & g_mask) >> g_rshift)] << g_lshift) | \
	 (_rgb_scale_5[((p & b_mask) >> b_rshift)] << b_lshift))

/* Converting 444 to 888 */
#define ALLEGRO_SHIFT_CONVERT_4444(p, \
		a_mask, a_lshift, a_rshift, \
		r_mask, r_lshift, r_rshift, \
		g_mask, g_lshift, g_rshift, \
		b_mask, b_lshift, b_rshift) \
	((_rgb_scale_4[((p & a_mask) >> a_rshift)] << a_lshift) | \
	 (_rgb_scale_4[((p & r_mask) >> r_rshift)] << r_lshift) | \
	 (_rgb_scale_4[((p & g_mask) >> g_rshift)] << g_lshift) | \
	 (_rgb_scale_4[((p & b_mask) >> b_rshift)] << b_lshift))

/* Left shift conversion */
#define ALLEGRO_LS_CONVERT(p, \
		a_mask, a_lshift, \
		r_mask, r_lshift, \
		g_mask, g_lshift, \
		b_mask, b_lshift) \
	ALLEGRO_SHIFT_CONVERT(p, \
		a_mask, a_lshift, 0, \
		r_mask, r_lshift, 0, \
		g_mask, g_lshift, 0, \
		b_mask, b_lshift, 0)


/* Right shift conversion */
#define ALLEGRO_RS_CONVERT(p, \
		a_mask, a_rshift, \
		r_mask, r_rshift, \
		g_mask, g_rshift, \
		b_mask, b_rshift) \
	ALLEGRO_SHIFT_CONVERT(p, \
		a_mask, 0, a_rshift, \
		r_mask, 0, r_rshift, \
		g_mask, 0, g_rshift, \
		b_mask, 0, b_rshift)


/* ARGB_8888 */

#define ALLEGRO_CONVERT_ARGB_8888_TO_ARGB_8888(p) (p)

#define ALLEGRO_CONVERT_ARGB_8888_TO_RGBA_8888(p) \
	(((p & 0x00FFFFFF) << 8) | \
	 ((p & 0xFF000000) >> 24))

#define ALLEGRO_CONVERT_ARGB_8888_TO_ARGB_4444(p) \
	ALLEGRO_RS_CONVERT(p, \
		0xF0000000, 16, \
		0x00F00000, 12, \
		0x0000F000, 8, \
		0x000000F0, 4)

#define ALLEGRO_CONVERT_ARGB_8888_TO_RGB_888(p) \
	(p & 0xFFFFFF)

#define ALLEGRO_CONVERT_ARGB_8888_TO_RGB_565(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x00F80000, 8, \
		0x0000FC00, 5, \
		0x000000F8, 3)

#define ALLEGRO_CONVERT_ARGB_8888_TO_RGB_555(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x00F80000, 9, \
		0x0000F800, 6, \
		0x000000F8, 3)

#define ALLEGRO_CONVERT_ARGB_8888_TO_RGBA_5551(p) \
	ALLEGRO_RS_CONVERT(p, \
		0x80000000, 31, \
		0x00F80000, 8, \
		0x0000F800, 5, \
		0x000000F8, 2)

#define ALLEGRO_CONVERT_ARGB_8888_TO_ARGB_1555(p) \
	ALLEGRO_RS_CONVERT(p, \
		0x80000000, 16, \
		0x00F80000, 9, \
		0x0000F800, 6, \
		0x000000F8, 3)

#define ALLEGRO_CONVERT_ARGB_8888_TO_ABGR_8888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xFF000000, 0, 0, \
		0x00FF0000, 0, 16, \
		0x0000FF00, 0, 0, \
		0x000000FF, 16, 0)

#define ALLEGRO_CONVERT_ARGB_8888_TO_XBGR_8888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xFF000000, 0, 0, \
		0x00FF0000, 0, 16, \
		0x0000FF00, 0, 0, \
		0x000000FF, 16, 0)

#define ALLEGRO_CONVERT_ARGB_8888_TO_BGR_888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x00FF0000, 0, 16, \
		0x0000FF00, 0, 0, \
		0x000000FF, 16, 0)

#define ALLEGRO_CONVERT_ARGB_8888_TO_BGR_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x00F80000, 0, 19, \
		0x0000FC00, 0, 5, \
		0x000000F8, 8, 0)

#define ALLEGRO_CONVERT_ARGB_8888_TO_BGR_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x00F80000, 0, 19, \
		0x0000F800, 0, 6, \
		0x000000F8, 7, 0)

#define ALLEGRO_CONVERT_ARGB_8888_TO_RGBX_8888(p) \
	(((p & 0x00FFFFFF) << 8) | 0xFF)

#define ALLEGRO_CONVERT_ARGB_8888_TO_XRGB_8888(p) \
	(p | 0xFF000000)

#define ALLEGRO_CONVERT_ARGB_8888_TO_RGBA_4444(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xF0000000, 0, 28, \
		0x00F00000, 0, 8, \
		0x0000F000, 0, 4, \
		0x000000F0, 0, 0) \

/* RGBA_8888 */

#define ALLEGRO_CONVERT_RGBA_8888_TO_RGBA_8888(p) (p)

#define ALLEGRO_CONVERT_RGBA_8888_TO_ARGB_8888(p) \
	(((p & 0xFFFFFF00) >> 8) | \
	 ((p & 0xFF) << 24))

#define ALLEGRO_CONVERT_RGBA_8888_TO_ARGB_4444(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x000000F0, 8, 0, \
		0xF0000000, 0, 20, \
		0x00F00000, 0, 16, \
		0x0000F000, 0, 12)

#define ALLEGRO_CONVERT_RGBA_8888_TO_RGB_888(p) \
	(p >> 8)

#define ALLEGRO_CONVERT_RGBA_8888_TO_RGB_565(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF8000000, 16, \
		0x00FC0000, 13, \
		0x0000F800, 11)

#define ALLEGRO_CONVERT_RGBA_8888_TO_RGB_555(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF8000000, 17, \
		0x00F80000, 14, \
		0x0000F800, 11)

#define ALLEGRO_CONVERT_RGBA_8888_TO_RGBA_5551(p) \
	ALLEGRO_RS_CONVERT(p, \
		0xFF, 7, \
		0xF8000000, 16, \
		0x00F80000, 13, \
		0x0000F800, 10)

#define ALLEGRO_CONVERT_RGBA_8888_TO_ARGB_1555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x80, 12, 0, \
		0xF8000000, 0, 17, \
		0x00F80000, 0, 14, \
		0x0000F800, 0, 11)

#define ALLEGRO_CONVERT_RGBA_8888_TO_ABGR_8888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xFF, 24, 0, \
		0xFF000000, 0, 24, \
		0x00FF0000, 0, 8, \
		0x0000FF00, 8, 0)

#define ALLEGRO_CONVERT_RGBA_8888_TO_XBGR_8888(p) \
	(ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF000000, 0, 24, \
		0x00FF0000, 0, 8, \
		0x0000FF00, 8, 0) | 0xFF000000)

#define ALLEGRO_CONVERT_RGBA_8888_TO_BGR_888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF000000, 0, 24, \
		0x00FF0000, 0, 8, \
		0x0000FF00, 8, 0)

#define ALLEGRO_CONVERT_RGBA_8888_TO_BGR_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF8000000, 0, 27, \
		0x00FC0000, 0, 13, \
		0x0000F800, 0, 0)

#define ALLEGRO_CONVERT_RGBA_8888_TO_BGR_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF8000000, 0, 27, \
		0x00F80000, 0, 14, \
		0x0000F800, 0, 1)

#define ALLEGRO_CONVERT_RGBA_8888_TO_RGBX_8888(p) \
	(p)

#define ALLEGRO_CONVERT_RGBA_8888_TO_XRGB_8888(p) \
	((p >> 8) | 0xFF000000)

#define ALLEGRO_CONVERT_RGBA_8888_TO_RGBA_4444(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x000000F0, 8, 0, \
		0xF0000000, 0, 28, \
		0x00F00000, 0, 16, \
		0x0000F000, 0, 4)

/* ARGB_4444 */

#define ALLEGRO_CONVERT_ARGB_4444_TO_ARGB_4444(p) (p)

#define ALLEGRO_CONVERT_ARGB_4444_TO_ARGB_8888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0xF000, 24, 12, \
		0x0F00, 16, 8, \
		0x00F0, 8, 4, \
		0x000F, 0, 0)

#define ALLEGRO_CONVERT_ARGB_4444_TO_RGBA_8888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0xF000, 0, 12, \
		0x0F00, 24, 8, \
		0x00F0, 16, 4, \
		0x000F, 8, 0)

#define ALLEGRO_CONVERT_ARGB_4444_TO_RGB_888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0, 0, 0, \
		0x0F00, 16, 8, \
		0x00F0, 8, 4, \
		0x000F, 0, 0)

#define ALLEGRO_CONVERT_ARGB_4444_TO_RGB_565(p) \
	ALLEGRO_LS_CONVERT(p, \
		0, 0, \
		0x0F00, 4, \
		0x00F0, 3, \
		0x000F, 1)

#define ALLEGRO_CONVERT_ARGB_4444_TO_RGB_555(p) \
	ALLEGRO_LS_CONVERT(p, \
		0, 0, \
		0x0F00, 3, \
		0x00F0, 2, \
		0x000F, 1)

#define ALLEGRO_CONVERT_ARGB_4444_TO_RGBA_5551(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x8000, 0, 15, \
		0x0F00, 4, 0, \
		0x00F0, 3, 0, \
		0x000F, 2, 0)

#define ALLEGRO_CONVERT_ARGB_4444_TO_ARGB_1555(p) \
	ALLEGRO_LS_CONVERT(p, \
		0x8000, 0, /* alpha can stay */ \
		0x0F00, 3, \
		0x00F0, 2, \
		0x000F, 1)

#define ALLEGRO_CONVERT_ARGB_4444_TO_ABGR_8888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0xF000, 24, 12, \
		0x0F00, 0, 8, \
		0x00F0, 8, 4, \
		0x000F, 16, 0)

#define ALLEGRO_CONVERT_ARGB_4444_TO_XBGR_8888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0xFF000000, 0, 0, \
		0x0F00, 0, 8, \
		0x00F0, 8, 4, \
		0x000F, 16, 0)

#define ALLEGRO_CONVERT_ARGB_4444_TO_BGR_888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0, 0, 0, \
		0x0F00, 0, 8, \
		0x00F0, 8, 4, \
		0x000F, 16, 0)

#define ALLEGRO_CONVERT_ARGB_4444_TO_BGR_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0F00, 0, 7, \
		0x00F0, 3, 0, \
		0x000F, 12, 0)

#define ALLEGRO_CONVERT_ARGB_4444_TO_BGR_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0F00, 0, 7, \
		0x00F0, 2, 0, \
		0x000F, 11, 0)

#define ALLEGRO_CONVERT_ARGB_4444_TO_RGBX_8888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0xFF, 0, 0, \
		0x0F00, 24, 8, \
		0x00F0, 16, 4, \
		0x000F, 8, 0)

#define ALLEGRO_CONVERT_ARGB_4444_TO_XRGB_8888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0xFF000000, 0, 0, \
		0x0F00, 16, 8, \
		0x00F0, 8, 4, \
		0x000F, 0, 0)

#define ALLEGRO_CONVERT_ARGB_4444_TO_RGBA_4444(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xF000, 0, 12, \
		0x0F00, 4, 0, \
		0x00F0, 4, 0, \
		0x000F, 4, 0)

/* RGB_888 */

#define ALLEGRO_CONVERT_RGB_888_TO_RGB_888(p) (p)

#define ALLEGRO_CONVERT_RGB_888_TO_ARGB_8888(p) \
	(p | 0xFF000000)

#define ALLEGRO_CONVERT_RGB_888_TO_RGBA_8888(p) \
	((p << 8) | 0xFF)

#define ALLEGRO_CONVERT_RGB_888_TO_ARGB_4444(p) \
	(0xF000 | \
		ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF00000, 12, \
		0x00F000, 8, \
		0x0000F0, 4))

#define ALLEGRO_CONVERT_RGB_888_TO_RGB_565(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF80000, 8, \
		0x00FC00, 5, \
		0x0000F8, 3)

#define ALLEGRO_CONVERT_RGB_888_TO_RGB_555(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF80000, 9, \
		0x00F800, 6, \
		0x0000F8, 3)

#define ALLEGRO_CONVERT_RGB_888_TO_RGBA_5551(p) \
	(1 | \
		ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF80000, 8, \
		0x00F800, 5, \
		0x0000F8, 2))

#define ALLEGRO_CONVERT_RGB_888_TO_ARGB_1555(p) \
	(0x8000 | \
		ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF80000, 9, \
		0x00F800, 6, \
		0x0000F8, 3))

#define ALLEGRO_CONVERT_RGB_888_TO_ABGR_8888(p) \
	(0xFF000000 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF0000, 0, 16, \
		0x00FF00, 0, 0, \
		0x0000FF, 16, 0))

#define ALLEGRO_CONVERT_RGB_888_TO_XBGR_8888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xFF000000, 0, 0, \
		0xFF0000, 0, 16, \
		0x00FF00, 0, 0, \
		0x0000FF, 16, 0)

#define ALLEGRO_CONVERT_RGB_888_TO_BGR_888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF0000, 0, 16, \
		0x00FF00, 0, 0, \
		0x0000FF, 16, 0)

#define ALLEGRO_CONVERT_RGB_888_TO_BGR_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF80000, 0, 19, \
		0x00FC00, 0, 5, \
		0x0000F8, 8, 0)

#define ALLEGRO_CONVERT_RGB_888_TO_BGR_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF80000, 0, 19, \
		0x00F800, 0, 6, \
		0x0000F8, 7, 0)

#define ALLEGRO_CONVERT_RGB_888_TO_RGBX_8888(p) \
	((p << 8) | 0xFF)

#define ALLEGRO_CONVERT_RGB_888_TO_XRGB_8888(p) \
	((p) | 0xFF000000)

#define ALLEGRO_CONVERT_RGB_888_TO_RGBA_4444(p) \
	(0xF | \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x00F00000, 0, 8, \
		0x0000F000, 0, 4, \
		0x000000F0, 0, 0))
		

/* RGB_565 */

#define ALLEGRO_CONVERT_RGB_565_TO_RGB_565(p) (p)

#define ALLEGRO_CONVERT_RGB_565_TO_ARGB_8888(p) \
	(0xFF000000 | \
		ALLEGRO_SHIFT_CONVERT_565(p, \
		0xF800, 16, 11, \
		0x07E0, 8, 5, \
		0x001F, 0, 0))

#define ALLEGRO_CONVERT_RGB_565_TO_RGBA_8888(p) \
	(0xFF | \
		ALLEGRO_SHIFT_CONVERT_565(p, \
		0xF800, 24, 11, \
		0x07E0, 16, 5, \
		0x001F, 8, 0))

#define ALLEGRO_CONVERT_RGB_565_TO_ARGB_4444(p) \
	(0xF000 | \
		ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF000, 4, \
		0x0780, 3, \
		0x001E, 1))

#define ALLEGRO_CONVERT_RGB_565_TO_RGB_888(p) \
	ALLEGRO_SHIFT_CONVERT_565(p, \
		0xF800, 16, 11, \
		0x07E0, 8, 5, \
		0x001F, 0, 0)

#define ALLEGRO_CONVERT_RGB_565_TO_RGB_555(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF800, 1, \
		0x07E0, 1, \
		0x001F, 0)

#define ALLEGRO_CONVERT_RGB_565_TO_RGBA_5551(p) \
	(1 | \
		ALLEGRO_LS_CONVERT(p, \
		0, 0, \
		0xF800, 0, \
		0x07E0, 0, \
		0x001F, 1))

#define ALLEGRO_CONVERT_RGB_565_TO_ARGB_1555(p) \
	(0x8000 | \
		ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF800, 1, \
		0x07E0, 1, \
		0x001F, 0))

#define ALLEGRO_CONVERT_RGB_565_TO_ABGR_8888(p) \
	(0xFF000000 | \
		ALLEGRO_SHIFT_CONVERT_565(p, \
		0xF800, 0, 11, \
		0x07E0, 8, 5, \
		0x001F, 16, 0))

#define ALLEGRO_CONVERT_RGB_565_TO_XBGR_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_565(p, \
		0xF800, 0, 11, \
		0x07E0, 8, 5, \
		0x001F, 16, 0) | 0xFF000000)

#define ALLEGRO_CONVERT_RGB_565_TO_BGR_888(p) \
	ALLEGRO_SHIFT_CONVERT_565(p, \
		0xF800, 0, 11, \
		0x07E0, 8, 5, \
		0x001F, 16, 0)

#define ALLEGRO_CONVERT_RGB_565_TO_BGR_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF800, 0, 11, \
		0x07E0, 0, 0, \
		0x001F, 11, 0)

#define ALLEGRO_CONVERT_RGB_565_TO_BGR_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF800, 0, 11, \
		0x07E0, 0, 1, \
		0x001F, 10, 0)

#define ALLEGRO_CONVERT_RGB_565_TO_RGBX_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_565(p, \
		0xF800, 24, 11, \
		0x07E0, 16, 5, \
		0x001F, 8, 0) | 0xFF)

#define ALLEGRO_CONVERT_RGB_565_TO_XRGB_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_565(p, \
		0xF800, 16, 11, \
		0x07E0, 8, 5, \
		0x001F, 0, 0) | 0xFF000000)


#define ALLEGRO_CONVERT_RGB_565_TO_RGBA_4444(p) \
	(0xF | \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF000, 0, 0, \
		0x0780, 1, 0, \
		0x001E, 3, 0))

/* RGB_555 */

#define ALLEGRO_CONVERT_RGB_555_TO_RGB_555(p) (p)

#define ALLEGRO_CONVERT_RGB_555_TO_ARGB_8888(p) \
	(0xFF000000 | \
		ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 16, 10, \
		0x03E0, 8, 5, \
		0x001F, 0, 0))

#define ALLEGRO_CONVERT_RGB_555_TO_RGBA_8888(p) \
	(0xFF | \
		ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 24, 10, \
		0x03E0, 16, 5, \
		0x001F, 8, 0))

#define ALLEGRO_CONVERT_RGB_555_TO_ARGB_4444(p) \
	(0xF000 | \
		ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x7800, 3, \
		0x03C0, 2, \
		0x001E, 1))

#define ALLEGRO_CONVERT_RGB_555_TO_RGB_888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 16, 10, \
		0x03E0, 8, 5, \
		0x001F, 0, 0)

#define ALLEGRO_CONVERT_RGB_555_TO_RGB_565(p) \
	ALLEGRO_LS_CONVERT(p, \
		0, 0, \
		0x7C00, 1, \
		0x03E0, 1, \
		0x001F, 0)

#define ALLEGRO_CONVERT_RGB_555_TO_RGBA_5551(p) \
	((p << 1) | 1)

#define ALLEGRO_CONVERT_RGB_555_TO_ARGB_1555(p) \
	(p | 0x8000)

#define ALLEGRO_CONVERT_RGB_555_TO_ABGR_8888(p) \
	(0xFF000000 | \
		ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 0, 10, \
		0x03E0, 8, 5, \
		0x001F, 16, 0))

#define ALLEGRO_CONVERT_RGB_555_TO_XBGR_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 0, 10, \
		0x03E0, 8, 5, \
		0x001F, 16, 0) | 0xFF000000)

#define ALLEGRO_CONVERT_RGB_555_TO_BGR_888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 0, 10, \
		0x03E0, 8, 5, \
		0x001F, 16, 0)

#define ALLEGRO_CONVERT_RGB_555_TO_BGR_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 0, 10, \
		0x03E0, 1, 0, \
		0x001F, 11, 0)

#define ALLEGRO_CONVERT_RGB_555_TO_BGR_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 0, 10, \
		0x03E0, 0, 0, \
		0x001F, 10, 0)

#define ALLEGRO_CONVERT_RGB_555_TO_RGBX_8888(p) \
	((ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 24, 10, \
		0x03E0, 16, 5, \
		0x001F, 8, 0) | 0xFF) | 0xFF)

#define ALLEGRO_CONVERT_RGB_555_TO_XRGB_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 16, 10, \
		0x03E0, 8, 5, \
		0x001F, 0, 0) | 0xFF000000)

#define ALLEGRO_CONVERT_RGB_555_TO_RGBA_4444(p) \
	(0xF | \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7800, 1, 0, \
		0x03C0, 2, 0, \
		0x001E, 3, 0))

/* RGBA_5551 */

#define ALLEGRO_CONVERT_RGBA_5551_TO_RGBA_5551(p) (p)

#define ALLEGRO_CONVERT_RGBA_5551_TO_ARGB_8888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0x1, 24, 0, \
		0xF800, 16, 11, \
		0x07C0, 8, 6, \
		0x003E, 0, 1)

#define ALLEGRO_CONVERT_RGBA_5551_TO_RGBA_8888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0x1, 0, 0, \
		0xF800, 24, 11, \
		0x07C0, 16, 6, \
		0x003E, 8, 1)

#define ALLEGRO_CONVERT_RGBA_5551_TO_ARGB_4444(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x1, 15, 0, \
		0xF000, 0, 4, \
		0x0780, 0, 3, \
		0x003C, 0, 2)

#define ALLEGRO_CONVERT_RGBA_5551_TO_RGB_888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0xF800, 16, 11, \
		0x07C0, 8, 6, \
		0x003E, 0, 1)

#define ALLEGRO_CONVERT_RGBA_5551_TO_RGB_565(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF800, 0, \
		0x07C0, 0, \
		0x003E, 1)

#define ALLEGRO_CONVERT_RGBA_5551_TO_RGB_555(p) \
	(p >> 1)

#define ALLEGRO_CONVERT_RGBA_5551_TO_ARGB_1555(p) \
	(((p & 1) << 15) | (p >> 1))

#define ALLEGRO_CONVERT_RGBA_5551_TO_ABGR_8888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0x1, 24, 0, \
		0xF800, 0, 11, \
		0x07C0, 8, 6, \
		0x003E, 16, 1)

#define ALLEGRO_CONVERT_RGBA_5551_TO_XBGR_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0xF800, 0, 11, \
		0x07C0, 8, 6, \
		0x003E, 16, 1) | 0xFF000000)

#define ALLEGRO_CONVERT_RGBA_5551_TO_BGR_888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0xF800, 0, 11, \
		0x07C0, 8, 6, \
		0x003E, 16, 1)

#define ALLEGRO_CONVERT_RGBA_5551_TO_BGR_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF800, 0, 11, \
		0x07C0, 0, 0, \
		0x003E, 10, 0)

#define ALLEGRO_CONVERT_RGBA_5551_TO_BGR_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF800, 0, 11, \
		0x07C0, 0, 1, \
		0x003E, 9, 0)

#define ALLEGRO_CONVERT_RGBA_5551_TO_RGBX_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0xF800, 24, 11, \
		0x07C0, 16, 6, \
		0x003E, 8, 1) | 0xFF)

#define ALLEGRO_CONVERT_RGBA_5551_TO_XRGB_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0xF800, 16, 11, \
		0x07C0, 8, 6, \
		0x003E, 0, 1) | 0xFF000000)

#define ALLEGRO_CONVERT_RGBA_5551_TO_RGBA_4444(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x0001, 3, 0, \
		0xF000, 0, 0, \
		0x0780, 1, 0, \
		0x003C, 2, 0)

/* ARGB_1555 */

#define ALLEGRO_CONVERT_ARGB_1555_TO_ARGB_1555(p) (p)

#define ALLEGRO_CONVERT_ARGB_1555_TO_ARGB_8888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0x8000, 24, 15, \
		0x7C00, 16, 10, \
		0x03E0, 8, 5, \
		0x001F, 0, 0)

#define ALLEGRO_CONVERT_ARGB_1555_TO_RGBA_8888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0x8000, 0, 15, \
		0x7C00, 24, 10, \
		0x03E0, 16, 5, \
		0x001F, 8, 0)

#define ALLEGRO_CONVERT_ARGB_1555_TO_ARGB_4444(p) \
	ALLEGRO_RS_CONVERT(p, \
		0x8000, 0, \
		0x7800, 3, \
		0x03C0, 2, \
		0x001E, 1)

#define ALLEGRO_CONVERT_ARGB_1555_TO_RGB_888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 16, 10, \
		0x03E0, 8, 5, \
		0x001F, 0, 0)

#define ALLEGRO_CONVERT_ARGB_1555_TO_RGB_565(p) \
	ALLEGRO_LS_CONVERT(p, \
		0, 0, \
		0x7C00, 1, \
		0x03E0, 1, \
		0x001F, 0)

#define ALLEGRO_CONVERT_ARGB_1555_TO_RGB_555(p) \
	(p & 0x7FFF)

#define ALLEGRO_CONVERT_ARGB_1555_TO_RGBA_5551(p) \
	((p << 1) | (p >> 15))

#define ALLEGRO_CONVERT_ARGB_1555_TO_ABGR_8888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0x8000, 24, 15, \
		0x7C00, 0, 10, \
		0x03E0, 8, 5, \
		0x001F, 16, 0)

#define ALLEGRO_CONVERT_ARGB_1555_TO_XBGR_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 0, 10, \
		0x03E0, 8, 5, \
		0x001F, 16, 0) | 0xFF000000)

#define ALLEGRO_CONVERT_ARGB_1555_TO_BGR_888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 0, 10, \
		0x03E0, 8, 5, \
		0x001F, 16, 0)

#define ALLEGRO_CONVERT_ARGB_1555_TO_BGR_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 0, 10, \
		0x03E0, 1, 0, \
		0x001F, 11, 0)

#define ALLEGRO_CONVERT_ARGB_1555_TO_BGR_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x7C00, 0, 10, \
		0x03E0, 0, 0, \
		0x001F, 10, 0)

#define ALLEGRO_CONVERT_ARGB_1555_TO_RGBX_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 24, 10, \
		0x03E0, 16, 5, \
		0x001F, 8, 0) | 0xFF)

#define ALLEGRO_CONVERT_ARGB_1555_TO_XRGB_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x7C00, 16, 10, \
		0x03E0, 8, 5, \
		0x001F, 0, 0) | 0xFF000000)

#define ALLEGRO_CONVERT_ARGB_1555_TO_RGBA_4444(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x8000, 0, 12, \
		0x7800, 1, 0, \
		0x03C0, 2, 0, \
		0x001E, 3, 0)

/* ABGR_8888 */

#define ALLEGRO_CONVERT_ABGR_8888_TO_ABGR_8888(p) (p)

#define ALLEGRO_CONVERT_ABGR_8888_TO_ARGB_8888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xFF000000, 0, 0, \
		0x000000FF, 16, 0, \
		0x0000FF00, 0, 0, \
		0x00FF0000, 0, 16)

#define ALLEGRO_CONVERT_ABGR_8888_TO_RGBA_8888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xFF000000, 0, 24, \
		0x000000FF, 24, 0, \
		0x0000FF00, 8, 0, \
		0x00FF0000, 0, 8)

#define ALLEGRO_CONVERT_ABGR_8888_TO_ARGB_4444(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xF0000000, 0, 16, \
		0x000000F0, 4, 0, \
		0x0000F000, 0, 8, \
		0x00F00000, 0, 20)

#define ALLEGRO_CONVERT_ABGR_8888_TO_RGB_888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000FF, 16, 0, \
		0x0000FF00, 0, 0, \
		0x00FF0000, 0, 16)

#define ALLEGRO_CONVERT_ABGR_8888_TO_RGB_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000F8, 8, 0, \
		0x0000FC00, 0, 5, \
		0x00F80000, 0, 19)

#define ALLEGRO_CONVERT_ABGR_8888_TO_RGB_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000F8, 7, 0, \
		0x0000F800, 0, 6, \
		0x00F80000, 0, 19)

#define ALLEGRO_CONVERT_ABGR_8888_TO_RGBA_5551(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x80000000, 0, 31, \
		0x000000F8, 8, 0, \
		0x0000F800, 0, 5, \
		0x00F80000, 0, 18)

#define ALLEGRO_CONVERT_ABGR_8888_TO_ARGB_1555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x80000000, 0, 16, \
		0x000000F8, 7, 0, \
		0x0000F800, 0, 6, \
		0x00F80000, 0, 19)

#define ALLEGRO_CONVERT_ABGR_8888_TO_XBGR_8888(p) \
	(p)

#define ALLEGRO_CONVERT_ABGR_8888_TO_BGR_888(p) \
	(p & 0xFFFFFF)

#define ALLEGRO_CONVERT_ABGR_8888_TO_BGR_565(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x000000F8, 3, \
		0x0000FC00, 5, \
		0x00F80000, 8)

#define ALLEGRO_CONVERT_ABGR_8888_TO_BGR_555(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x000000F8, 3, \
		0x0000F800, 6, \
		0x00F80000, 9)

#define ALLEGRO_CONVERT_ABGR_8888_TO_RGBX_8888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xFF, 0, 0, \
		0x000000FF, 24, 0, \
		0x0000FF00, 8, 0, \
		0x00FF0000, 0, 8)

#define ALLEGRO_CONVERT_ABGR_8888_TO_XRGB_8888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xFF000000, 0, 0, \
		0x000000FF, 16, 0, \
		0x0000FF00, 0, 0, \
		0x00FF0000, 0, 16)

#define ALLEGRO_CONVERT_ABGR_8888_TO_RGBA_4444(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xF0000000, 0, 28, \
		0x000000F0, 8, 0, \
		0x0000F000, 0, 4, \
		0x00F00000, 0, 16)

/* XBGR_8888 */

#define ALLEGRO_CONVERT_XBGR_8888_TO_XBGR_8888(p) (p)

#define ALLEGRO_CONVERT_XBGR_8888_TO_ARGB_8888(p) \
	(0xFF000000 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000FF, 16, 0, \
		0x0000FF00, 0, 0, \
		0x00FF0000, 0, 16))
		

#define ALLEGRO_CONVERT_XBGR_8888_TO_RGBA_8888(p) \
	(0xFF | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000FF, 24, 0, \
		0x0000FF00, 8, 0, \
		0x00FF0000, 0, 8))

#define ALLEGRO_CONVERT_XBGR_8888_TO_ARGB_4444(p) \
	(0xF000 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000F0, 4, 0, \
		0x0000F000, 0, 8, \
		0x00F00000, 0, 20))

#define ALLEGRO_CONVERT_XBGR_8888_TO_RGB_888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000FF, 16, 0, \
		0x0000FF00, 0, 0, \
		0x00FF0000, 0, 16)

#define ALLEGRO_CONVERT_XBGR_8888_TO_RGB_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000F8, 8, 0, \
		0x0000FC00, 0, 5, \
		0x00F80000, 0, 19)

#define ALLEGRO_CONVERT_XBGR_8888_TO_RGB_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x000000F8, 7, 0, \
		0x0000F800, 0, 6, \
		0x00F80000, 0, 19)

#define ALLEGRO_CONVERT_XBGR_8888_TO_RGBA_5551(p) \
	(1 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F8, 8, 0, \
		0x00F800, 0, 5, \
		0xF80000, 0, 18))
		

#define ALLEGRO_CONVERT_XBGR_8888_TO_ARGB_1555(p) \
	(0x8000 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F8, 7, 0, \
		0x00F800, 0, 6, \
		0xF80000, 0, 19))

#define ALLEGRO_CONVERT_XBGR_8888_TO_ABGR_8888(p) \
	(0xFF000000 | p)

#define ALLEGRO_CONVERT_XBGR_8888_TO_BGR_888(p) \
	(p & 0xFFFFFF)

#define ALLEGRO_CONVERT_XBGR_8888_TO_BGR_565(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x000000F8, 3, \
		0x0000FC00, 5, \
		0x00F80000, 8)

#define ALLEGRO_CONVERT_XBGR_8888_TO_BGR_555(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x000000F8, 3, \
		0x0000F800, 6, \
		0x00F80000, 9)

#define ALLEGRO_CONVERT_XBGR_8888_TO_RGBX_8888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xFF, 0, 0, \
		0x000000FF, 24, 0, \
		0x0000FF00, 8, 0, \
		0x00FF0000, 0, 8)

#define ALLEGRO_CONVERT_XBGR_8888_TO_XRGB_8888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xFF000000, 0, 0, \
		0x000000FF, 16, 0, \
		0x0000FF00, 0, 0, \
		0x00FF0000, 0, 16)

#define ALLEGRO_CONVERT_XBGR_8888_TO_RGBA_4444(p) \
	(0xF | \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F0, 8, 0, \
		0x00F000, 0, 4, \
		0xF00000, 0, 16))

/* BGR_888 */

#define ALLEGRO_CONVERT_BGR_888_TO_ARGB_8888(p) \
	(0xFF000000 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000FF, 16, 0, \
		0x00FF00, 0, 0, \
		0xFF0000, 0, 16))
		

#define ALLEGRO_CONVERT_BGR_888_TO_RGBA_8888(p) \
	(0xFF | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000FF, 24, 0, \
		0x00FF00, 8, 0, \
		0xFF0000, 0, 8))

#define ALLEGRO_CONVERT_BGR_888_TO_ARGB_4444(p) \
	(0xF000 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F0, 4, 0, \
		0x00F000, 0, 8, \
		0xF00000, 0, 20))

#define ALLEGRO_CONVERT_BGR_888_TO_RGB_888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000FF, 16, 0, \
		0x00FF00, 0, 0, \
		0xFF0000, 0, 16)

#define ALLEGRO_CONVERT_BGR_888_TO_RGB_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F8, 8, 0, \
		0x00FC00, 0, 5, \
		0xF80000, 0, 19)

#define ALLEGRO_CONVERT_BGR_888_TO_RGB_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F8, 7, 0, \
		0x00F800, 0, 6, \
		0xF80000, 0, 19)

#define ALLEGRO_CONVERT_BGR_888_TO_RGBA_5551(p) \
	(1 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F8, 8, 0, \
		0x00F800, 0, 5, \
		0xF80000, 0, 18))

#define ALLEGRO_CONVERT_BGR_888_TO_ARGB_1555(p) \
	(0x8000 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F8, 7, 0, \
		0x00F800, 0, 6, \
		0xF80000, 0, 19))

#define ALLEGRO_CONVERT_BGR_888_TO_ABGR_8888(p) \
	(0xFF000000 | p)

#define ALLEGRO_CONVERT_BGR_888_TO_XBGR_8888(p) \
	((p) | 0xFF000000)

#define ALLEGRO_CONVERT_BGR_888_TO_BGR_888(p) \
	(p)

#define ALLEGRO_CONVERT_BGR_888_TO_BGR_565(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x0000F8, 3, \
		0x00FC00, 5, \
		0xF80000, 8)

#define ALLEGRO_CONVERT_BGR_888_TO_BGR_555(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x0000F8, 3, \
		0x00F800, 6, \
		0xF80000, 9)

#define ALLEGRO_CONVERT_BGR_888_TO_RGBX_8888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xFF, 0, 0, \
		0x0000FF, 24, 0, \
		0x00FF00, 8, 0, \
		0xFF0000, 0, 8)

#define ALLEGRO_CONVERT_BGR_888_TO_XRGB_8888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xFF000000, 0, 0, \
		0x000000FF, 16, 0, \
		0x0000FF00, 0, 0, \
		0x00FF0000, 0, 16)

#define ALLEGRO_CONVERT_BGR_888_TO_RGBA_4444(p) \
	(0xF | \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x0000F0, 0, 16, \
		0x00F000, 4, 0, \
		0xF00000, 0, 16))

/* BGR_565 */

#define ALLEGRO_CONVERT_BGR_565_TO_ARGB_8888(p) \
	(0xFF000000 | \
		ALLEGRO_SHIFT_CONVERT_565(p, \
		0x001F, 16, 0, \
		0x07E0, 8, 5, \
		0xF800, 0, 11))

#define ALLEGRO_CONVERT_BGR_565_TO_RGBA_8888(p) \
	(0xFF | \
		ALLEGRO_SHIFT_CONVERT_565(p, \
		0x001F, 24, 0, \
		0x07E0, 16, 5, \
		0xF800, 8, 11))


#define ALLEGRO_CONVERT_BGR_565_TO_ARGB_4444(p) \
	(0xF000 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001E, 7, 0, \
		0x0780, 0, 3, \
		0xF000, 0, 12))

#define ALLEGRO_CONVERT_BGR_565_TO_RGB_888(p) \
	ALLEGRO_SHIFT_CONVERT_565(p, \
		0x001F, 16, 0, \
		0x07E0, 8, 5, \
		0xF800, 0, 11)

#define ALLEGRO_CONVERT_BGR_565_TO_RGB_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 11, 0, \
		0x07E0, 0, 0, \
		0xF800, 0, 11)

#define ALLEGRO_CONVERT_BGR_565_TO_RGB_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 10, 0, \
		0x07C0, 0, 1, \
		0xF800, 0, 11)

#define ALLEGRO_CONVERT_BGR_565_TO_RGBA_5551(p) \
	(1 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 11, 0, \
		0x07C0, 0, 0, \
		0xF800, 0, 10))

#define ALLEGRO_CONVERT_BGR_565_TO_ARGB_1555(p) \
	(0x8000 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 10, 0, \
		0x07C0, 0, 1, \
		0xF800, 0, 11))

#define ALLEGRO_CONVERT_BGR_565_TO_ABGR_8888(p) \
	(0xFF000000 | \
		ALLEGRO_SHIFT_CONVERT_565(p, \
		0x001F, 0, 0, \
		0x07E0, 8, 5, \
		0xF800, 16, 11))

#define ALLEGRO_CONVERT_BGR_565_TO_XBGR_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_565(p, \
		0x001F, 0, 0, \
		0x07E0, 8, 5, \
		0xF800, 16, 11) | 0xFF000000)

#define ALLEGRO_CONVERT_BGR_565_TO_BGR_888(p) \
	ALLEGRO_SHIFT_CONVERT_565(p, \
		0x001F, 0, 0, \
		0x07E0, 8, 5, \
		0xF800, 16, 11)

#define ALLEGRO_CONVERT_BGR_565_TO_BGR_565(p) \
	(p)

#define ALLEGRO_CONVERT_BGR_565_TO_BGR_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 0, 0, \
		0x07C0, 0, 1, \
		0xF800, 0, 1)

#define ALLEGRO_CONVERT_BGR_565_TO_RGBX_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_565(p, \
		0x001F, 24, 0, \
		0x07E0, 16, 5, \
		0xF800, 8, 11) | 0xFF)

#define ALLEGRO_CONVERT_BGR_565_TO_XRGB_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_565(p, \
		0x001F, 16, 0, \
		0x07E0, 8, 5, \
		0xF800, 0, 11) | 0xFF000000)


#define ALLEGRO_CONVERT_BGR_565_TO_RGBA_4444(p) \
	(0xF | \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001E, 11, 0, \
		0x0780, 1, 0, \
		0xF000, 0, 8))

/* BGR_555 */

#define ALLEGRO_CONVERT_BGR_555_TO_ARGB_8888(p) \
	(0xFF000000 | \
		ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 16, 0, \
		0x03E0, 8, 5, \
		0x7C00, 0, 10))

#define ALLEGRO_CONVERT_BGR_555_TO_RGBA_8888(p) \
	(0xFF | \
		ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 24, 0, \
		0x03E0, 16, 5, \
		0x7C00, 8, 10))

#define ALLEGRO_CONVERT_BGR_555_TO_ARGB_4444(p) \
	(0xF000 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001E, 7, 0, \
		0x03C0, 0, 2, \
		0x7800, 0, 11))

#define ALLEGRO_CONVERT_BGR_555_TO_RGB_888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 16, 0, \
		0x03E0, 8, 5, \
		0x7C00, 0, 10)

#define ALLEGRO_CONVERT_BGR_555_TO_RGB_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 11, 0, \
		0x03E0, 1, 0, \
		0x7C00, 0, 10)

#define ALLEGRO_CONVERT_BGR_555_TO_RGB_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 10, 0, \
		0x03E0, 0, 0, \
		0x7C00, 0, 10)

#define ALLEGRO_CONVERT_BGR_555_TO_RGBA_5551(p) \
	(1 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 11, 0, \
		0x03E0, 1, 0, \
		0x7C00, 0, 9))

#define ALLEGRO_CONVERT_BGR_555_TO_ARGB_1555(p) \
	(0x8000 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001F, 10, 0, \
		0x03E0, 0, 0, \
		0x7C00, 0, 10))

#define ALLEGRO_CONVERT_BGR_555_TO_ABGR_8888(p) \
	(0xFF000000 | \
		ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 0, 0, \
		0x03E0, 8, 5, \
		0x7C00, 16, 10))

#define ALLEGRO_CONVERT_BGR_555_TO_XBGR_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 0, 0, \
		0x03E0, 8, 5, \
		0x7C00, 16, 10) | 0xFF000000)

#define ALLEGRO_CONVERT_BGR_555_TO_BGR_888(p) \
	ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 0, 0, \
		0x03E0, 8, 5, \
		0x7C00, 16, 10)

#define ALLEGRO_CONVERT_BGR_555_TO_BGR_565(p) \
	ALLEGRO_LS_CONVERT(p, \
		0, 0, \
		0x001F, 0, \
		0x03E0, 1, \
		0x7C00, 1)

#define ALLEGRO_CONVERT_BGR_555_TO_BGR_555(p) \
	(p)

#define ALLEGRO_CONVERT_BGR_555_TO_RGBX_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 24, 0, \
		0x03E0, 16, 5, \
		0x7C00, 8, 10) | 0xFF)

#define ALLEGRO_CONVERT_BGR_555_TO_XRGB_8888(p) \
	(ALLEGRO_SHIFT_CONVERT_555(p, \
		0, 0, 0, \
		0x001F, 16, 0, \
		0x03E0, 8, 5, \
		0x7C00, 0, 10) | 0xFF000000)

#define ALLEGRO_CONVERT_BGR_555_TO_RGBA_4444(p) \
	(0xF | \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x001E, 4, 0, \
		0x03C0, 2, 0, \
		0x7800, 0, 7))


/* RGBX_8888 */

#define ALLEGRO_CONVERT_RGBX_8888_TO_ARGB_8888(p) \
	(0xFF000000 | (p >> 8))

#define ALLEGRO_CONVERT_RGBX_8888_TO_RGBA_8888(p) \
	(0xFF | p)

#define ALLEGRO_CONVERT_RGBX_8888_TO_ARGB_4444(p) \
	(0xF000 | \
		ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF0000000, 20, \
		0x00F00000, 16, \
		0x0000F000, 12))

#define ALLEGRO_CONVERT_RGBX_8888_TO_RGB_888(p) \
	(p >> 8)

#define ALLEGRO_CONVERT_RGBX_8888_TO_RGB_565(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF8000000, 16, \
		0x00FC0000, 13, \
		0x0000F800, 11)

#define ALLEGRO_CONVERT_RGBX_8888_TO_RGB_555(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF8000000, 17, \
		0x00F80000, 14, \
		0x0000F800, 11)

#define ALLEGRO_CONVERT_RGBX_8888_TO_RGBA_5551(p) \
	(1 | \
		ALLEGRO_RS_CONVERT(p, \
			0, 0, \
			0xF8000000, 16, \
			0x00F80000, 13, \
			0x0000F800, 10))

#define ALLEGRO_CONVERT_RGBX_8888_TO_ARGB_1555(p) \
	(0x8000 | \
		ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0xF8000000, 17, \
		0x00F80000, 14, \
		0x0000F800, 11))

#define ALLEGRO_CONVERT_RGBX_8888_TO_ABGR_8888(p) \
	(0xFF000000 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF000000, 0, 24, \
		0x00FF0000, 0, 8, \
		0x0000FF00, 8, 0))

#define ALLEGRO_CONVERT_RGBX_8888_TO_XBGR_8888(p) \
	(ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF000000, 0, 24, \
		0x00FF0000, 0, 8, \
		0x0000FF00, 8, 0) | 0xFF000000)

#define ALLEGRO_CONVERT_RGBX_8888_TO_BGR_888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xFF000000, 0, 24, \
		0x00FF0000, 0, 8, \
		0x0000FF00, 8, 0)

#define ALLEGRO_CONVERT_RGBX_8888_TO_BGR_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF8000000, 0, 27, \
		0x00FC0000, 0, 13, \
		0x0000F800, 0, 0)

#define ALLEGRO_CONVERT_RGBX_8888_TO_BGR_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF8000000, 0, 27, \
		0x00F80000, 0, 14, \
		0x0000F800, 0, 1)

#define ALLEGRO_CONVERT_RGBX_8888_TO_RGBX_8888(p) \
	(p)

#define ALLEGRO_CONVERT_RGBX_8888_TO_XRGB_8888(p) \
	((p >> 8) | 0xFF000000)

#define ALLEGRO_CONVERT_RGBX_8888_TO_RGBA_4444(p) \
	(0xF | \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF0000000, 0, 8, \
		0x00F00000, 0, 12, \
		0x0000F000, 0, 8))

/* XRGB_8888 */

#define ALLEGRO_CONVERT_XRGB_8888_TO_ARGB_8888(p) \
	(p | 0xFF000000)

#define ALLEGRO_CONVERT_XRGB_8888_TO_RGBA_8888(p) \
	((p << 8) | 0xFF)

#define ALLEGRO_CONVERT_XRGB_8888_TO_ARGB_4444(p) \
	(0xF000 | \
		ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x00F00000, 12, \
		0x0000F000, 8, \
		0x000000F0, 4))

#define ALLEGRO_CONVERT_XRGB_8888_TO_RGB_888(p) \
	(p)

#define ALLEGRO_CONVERT_XRGB_8888_TO_RGB_565(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x00F80000, 8, \
		0x0000FC00, 5, \
		0x000000F8, 3)

#define ALLEGRO_CONVERT_XRGB_8888_TO_RGB_555(p) \
	ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x00F80000, 9, \
		0x0000F800, 6, \
		0x000000F8, 3)

#define ALLEGRO_CONVERT_XRGB_8888_TO_RGBA_5551(p) \
	(1 | \
		ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x00F80000, 8, \
		0x0000F800, 5, \
		0x000000F8, 2))

#define ALLEGRO_CONVERT_XRGB_8888_TO_ARGB_1555(p) \
	(0x8000 | \
		ALLEGRO_RS_CONVERT(p, \
		0, 0, \
		0x00F80000, 9, \
		0x0000F800, 6, \
		0x000000F8, 3))

#define ALLEGRO_CONVERT_XRGB_8888_TO_ABGR_8888(p) \
	(0xFF000000 | \
		ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x00FF0000, 0, 16, \
		0x0000FF00, 0, 0, \
		0x000000FF, 16, 0))

#define ALLEGRO_CONVERT_XRGB_8888_TO_XBGR_8888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0xFF000000, 0, 0, \
		0x00FF0000, 0, 16, \
		0x0000FF00, 0, 0, \
		0x000000FF, 16, 0)

#define ALLEGRO_CONVERT_XRGB_8888_TO_BGR_888(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x00FF0000, 0, 16, \
		0x0000FF00, 0, 0, \
		0x000000FF, 16, 0)

#define ALLEGRO_CONVERT_XRGB_8888_TO_BGR_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x00F80000, 0, 19, \
		0x0000FC00, 0, 5, \
		0x000000F8, 8, 0)

#define ALLEGRO_CONVERT_XRGB_8888_TO_BGR_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0x00F80000, 0, 19, \
		0x0000F800, 0, 6, \
		0x000000F8, 7, 0)

#define ALLEGRO_CONVERT_XRGB_8888_TO_RGBX_8888(p) \
	((p << 8) | 0xFF)

#define ALLEGRO_CONVERT_XRGB_8888_TO_XRGB_8888(p) \
	(p)

#define ALLEGRO_CONVERT_XRGB_8888_TO_RGBA_4444(p) \
	(0xF | \
	ALLEGRO_SHIFT_CONVERT(p, \
		0, 0, 0, \
		0xF00000, 0, 8, \
		0x00F000, 0, 4, \
		0x0000F0, 0, 0))

/* ABGR_F32 */

#define ALLEGRO_CONVERT_ARGB_8888_TO_ABGR_F32(p) al_map_rgba( \
   (p >> 16) & 255, (p >> 8) & 255, p & 255, (p >> 24) & 255)

#define ALLEGRO_CONVERT_RGBA_8888_TO_ABGR_F32(p) al_map_rgba( \
   (p >> 24) & 255, (p >> 16) & 255, (p >> 8) & 255, p & 255)

#define ALLEGRO_CONVERT_ARGB_4444_TO_ABGR_F32(p) al_map_rgba( \
   (p & 0x0f00) >> 4, p & 0x00f0, (p & 0x0004) << 4, (p & 0xf000) >> 8)

#define ALLEGRO_CONVERT_RGB_888_TO_ABGR_F32(p) al_map_rgb( \
   (p >> 16) & 255, (p >> 8) & 255, p & 255)

#define ALLEGRO_CONVERT_RGB_565_TO_ABGR_F32(p) al_map_rgb( \
   (p & 0xf800) >> 8, (p & 0x07e0) >> 3, (p & 0x1f) << 3)

#define ALLEGRO_CONVERT_RGB_555_TO_ABGR_F32(p) al_map_rgb( \
   (p & 0x7c00) >> 7, (p & 0x03e0) >> 2, (p & 0x1f) << 3)

#define ALLEGRO_CONVERT_RGBA_5551_TO_ABGR_F32(p) al_map_rgba( \
   (p & 0xf800) >> 8, (p & 0x07c0) >> 3, (p & 0x3e) << 2, (p & 1) * 255)

#define ALLEGRO_CONVERT_ARGB_1555_TO_ABGR_F32(p) al_map_rgba( \
   (p & 0x7c00) >> 7, (p & 0x03e0) >> 2, (p & 0x1f) << 3, ((p & 0x8000) >> 15) * 255)

#define ALLEGRO_CONVERT_ABGR_8888_TO_ABGR_F32(p) al_map_rgba( \
   p & 255, (p >> 8) & 255, (p >> 16) & 255, (p >> 24) & 255)

#define ALLEGRO_CONVERT_XBGR_8888_TO_ABGR_F32(p) al_map_rgb( \
   p & 255, (p >> 8) & 255, (p >> 16) & 255)

#define ALLEGRO_CONVERT_BGR_888_TO_ABGR_F32(p) al_map_rgb( \
   p & 255, (p >> 8) & 255, (p >> 16) & 255)

#define ALLEGRO_CONVERT_BGR_565_TO_ABGR_F32(p) al_map_rgb( \
   (p & 0x1f) << 3, (p & 0x07e0) >> 3, (p & 0xf800) >> 8)

#define ALLEGRO_CONVERT_BGR_555_TO_ABGR_F32(p) al_map_rgb( \
   (p & 0x1f) << 3, (p & 0x03e0) >> 2, (p & 0x7c00) >> 7)

#define ALLEGRO_CONVERT_RGBX_8888_TO_ABGR_F32(p) al_map_rgb( \
   (p & 0xf800) >> 8, (p & 0x07c0) >> 3, (p & 0x3e) << 2)

#define ALLEGRO_CONVERT_XRGB_8888_TO_ABGR_F32(p) al_map_rgb( \
   (p & 0x7c00) >> 7, (p & 0x03e0) >> 2, (p & 0x1f) << 3)

#define ALLEGRO_CONVERT_RGBA_4444_TO_ABGR_F32(p) al_map_rgba( \
  _rgb_scale_4[(p & 0xF000) >> 12], _rgb_scale_4[(p & 0x0F00) >> 8], _rgb_scale_4[(p & 0x00F0) >> 4], _rgb_scale_4[(p & 0xF)])

#define ALLEGRO_CONVERT_ABGR_F32_TO_ABGR_F32(p) (p)

#define ALLEGRO_C3_CONVERT(c, rm, rp, gm, gp, bm, bp) ( \
   ((uint32_t)(c.r * rm) << rp) + \
   ((uint32_t)(c.g * gm) << gp) + \
   ((uint32_t)(c.b * bm) << bp))

#define ALLEGRO_C4_CONVERT(c, rm, rp, gm, gp, bm, bp, am, ap) ( \
   ((uint32_t)(c.r * rm) << rp) + \
   ((uint32_t)(c.g * gm) << gp) + \
   ((uint32_t)(c.b * bm) << bp) + \
   ((uint32_t)(c.a * am) << ap))

#define ALLEGRO_CONVERT_ABGR_F32_TO_ARGB_8888(c) \
   ALLEGRO_C4_CONVERT(c, 255, 16, 255, 8, 255, 0, 255, 24)

#define ALLEGRO_CONVERT_ABGR_F32_TO_RGBA_8888(c) \
   ALLEGRO_C4_CONVERT(c, 255, 0, 255, 8, 255, 16, 255, 24)

#define ALLEGRO_CONVERT_ABGR_F32_TO_ARGB_4444(c) \
   ALLEGRO_C4_CONVERT(c, 15, 8, 15, 4, 15, 0, 15, 12)
   
#define ALLEGRO_CONVERT_ABGR_F32_TO_RGB_888(c) \
   ALLEGRO_C3_CONVERT(c, 255, 16, 255, 8, 255, 0)

#define ALLEGRO_CONVERT_ABGR_F32_TO_RGB_565(c) \
   ALLEGRO_C3_CONVERT(c, 31, 11, 63, 5, 31, 0)

#define ALLEGRO_CONVERT_ABGR_F32_TO_RGB_555(c) \
   ALLEGRO_C3_CONVERT(c, 31, 10, 31, 5, 31, 0)

#define ALLEGRO_CONVERT_ABGR_F32_TO_RGBA_5551(c) \
   ALLEGRO_C4_CONVERT(c, 31, 11, 31, 6, 31, 1, 1, 0)

#define ALLEGRO_CONVERT_ABGR_F32_TO_ARGB_1555(c) \
   ALLEGRO_C4_CONVERT(c, 31, 10, 31, 5, 31, 0, 1, 15)

#define ALLEGRO_CONVERT_ABGR_F32_TO_ABGR_8888(c) \
   ALLEGRO_C4_CONVERT(c, 255, 0, 255, 8, 255, 16, 255, 24)

#define ALLEGRO_CONVERT_ABGR_F32_TO_XBGR_8888(c) \
   ALLEGRO_C3_CONVERT(c, 255, 0, 255, 8, 255, 16)

#define ALLEGRO_CONVERT_ABGR_F32_TO_BGR_888(c) \
   ALLEGRO_C3_CONVERT(c, 255, 0, 255, 8, 255, 16)

#define ALLEGRO_CONVERT_ABGR_F32_TO_BGR_565(c) \
   ALLEGRO_C3_CONVERT(c, 31, 0, 63, 6, 31, 11)

#define ALLEGRO_CONVERT_ABGR_F32_TO_BGR_555(c) \
   ALLEGRO_C3_CONVERT(c, 31, 0, 31, 5, 31, 10)

#define ALLEGRO_CONVERT_ABGR_F32_TO_RGBX_8888(c) \
   ALLEGRO_C3_CONVERT(c, 255, 24, 255, 16, 255, 8)

#define ALLEGRO_CONVERT_ABGR_F32_TO_XRGB_8888(c) \
   ALLEGRO_C3_CONVERT(c, 255, 16, 255, 8, 255, 0)

#define ALLEGRO_CONVERT_ABGR_F32_TO_RGBA_4444(c) \
   ALLEGRO_C4_CONVERT(c, 15, 12, 15, 8, 15, 4, 15, 0)

/* RGBA_4444 */

#define ALLEGRO_CONVERT_RGBA_4444_TO_RGBA_4444(p) (p)

#define ALLEGRO_CONVERT_RGBA_4444_TO_ARGB_8888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0x000F, 28, 0, \
		0xF000, 20, 12, \
		0x0F00, 8, 8, \
		0x00F0, 4, 4)

#define ALLEGRO_CONVERT_RGBA_4444_TO_RGBA_8888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0x000F, 4, 0, \
		0xF000, 28, 12, \
		0x0F00, 20, 8, \
		0x00F0, 12, 4)

#define ALLEGRO_CONVERT_RGBA_4444_TO_RGB_888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0x000F, 28, 0, \
		0xF000, 20, 12, \
		0x0F00, 12, 8, \
		0x00F0, 4, 4)

#define ALLEGRO_CONVERT_RGBA_4444_TO_RGB_565(p) \
	((p & 0xF000) | ((p & 0x0F00) >> 1) | ((p & 0x00F0) >> 3))

#define ALLEGRO_CONVERT_RGBA_4444_TO_RGB_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x0000, 0, 0, \
		0xF000, 0, 1, \
		0x0F00, 0, 2, \
		0x00F0, 0, 3)

#define ALLEGRO_CONVERT_RGBA_4444_TO_RGBA_5551(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x8000, 0, 3, \
		0xF000, 0, 0, \
		0x0F00, 0, 1, \
		0x00F0, 0, 2)

#define ALLEGRO_CONVERT_RGBA_4444_TO_ARGB_1555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x8000, 12, 0, \
		0xF000, 0, 1, \
		0x0F00, 0, 2, \
		0x00F0, 0, 3)

#define ALLEGRO_CONVERT_RGBA_4444_TO_ABGR_8888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0x000F, 28, 0, \
		0xF000, 4, 12, \
		0x0F00, 12, 8, \
		0x00F0, 20, 4)

#define ALLEGRO_CONVERT_RGBA_4444_TO_XBGR_8888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0x000F, 28, 0, \
		0xF000, 4, 12, \
		0x0F00, 12, 8, \
		0x00F0, 20, 4)

#define ALLEGRO_CONVERT_RGBA_4444_TO_BGR_888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0x000F, 28, 0, \
		0xF000, 4, 12, \
		0x0F00, 12, 8, \
		0x00F0, 20, 4)

#define ALLEGRO_CONVERT_RGBA_4444_TO_BGR_565(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x0000, 0, 0, \
		0xF000, 0, 11, \
		0x0F00, 0, 1, \
		0x00F0, 8, 0)

#define ALLEGRO_CONVERT_RGBA_4444_TO_BGR_555(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x0000, 0, 0, \
		0xF000, 0, 11, \
		0x0F00, 0, 2, \
		0x00F0, 7, 0)

#define ALLEGRO_CONVERT_RGBA_4444_TO_RGBX_8888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0x000F, 4, 0, \
		0xF000, 28, 12, \
		0x0F00, 20, 8, \
		0x00F0, 12, 4)

#define ALLEGRO_CONVERT_RGBA_4444_TO_XRGB_8888(p) \
	ALLEGRO_SHIFT_CONVERT_4444(p, \
		0x000F, 28, 0, \
		0xF000, 20, 12, \
		0x0F00, 12, 8, \
		0x00F0, 4, 4)

#define ALLEGRO_CONVERT_RGBA_4444_TO_ARGB_4444(p) \
	ALLEGRO_SHIFT_CONVERT(p, \
		0x000F, 12, 0, \
		0xF000, 0, 4, \
		0x0F00, 0, 4, \
		0x00F0, 0, 4)


#endif
