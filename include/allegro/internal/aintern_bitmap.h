#ifndef ALLEGRO_INTERNAL_BITMAP_NEW_H
#define ALLEGRO_INTERNAL_BITMAP_NEW_H

#include "allegro/display_new.h"
#include "allegro/bitmap_new.h"
#include "allegro/internal/aintern_color.h"

typedef struct ALLEGRO_BITMAP_INTERFACE ALLEGRO_BITMAP_INTERFACE;

struct BITMAP;

struct ALLEGRO_BITMAP
{
   ALLEGRO_BITMAP_INTERFACE *vt;
   ALLEGRO_DISPLAY *display;
   int format;
   int flags;
   int w, h;
   /* 
    * clip left, right, top, bottom
    * Clip anything outside of this
    */
   int cl, cr, ct, cb;
   /*
    * Locking info.
    *
    * locked - locked or not?
    * lock_x/y - top left of the locked region
    * lock_w/h - width and height of the locked region
    * lock_flags - flags the region was locked with
    * locked_region - a copy of the locked rectangle
    */
   bool locked;
   int lock_x;
   int lock_y;
   int lock_w;
   int lock_h;
   int lock_flags;
   ALLEGRO_LOCKED_REGION locked_region;

   /* Info for sub-bitmaps */
   ALLEGRO_BITMAP *parent;
   int xofs;
   int yofs;

   /* Masking color */
   ALLEGRO_COLOR mask_color;

   /* A memory copy of the bitmap data. May be NULL for an empty bitmap. */
   unsigned char *memory;

   /* TODO: Is it needed? */
   /*unsigned char *palette;*/
};

struct ALLEGRO_BITMAP_INTERFACE
{
   int id;
   void (*draw_bitmap)(struct ALLEGRO_BITMAP *bitmap, float x, float y, int flags);
   void (*draw_bitmap_region)(ALLEGRO_BITMAP *bitmap, float sx, float sy,
      float sw, float sh, float dx, float dy, int flags);
   void (*draw_scaled_bitmap)(ALLEGRO_BITMAP *bitmap, float sx, float sy,
      float sw, float sh, float dx, float dy, float dw, float dh, int flags);
   void (*draw_rotated_bitmap)(ALLEGRO_BITMAP *bitmap, float cx, float cy,
      float angle, float dx, float dy, int flags);
   void (*draw_rotated_scaled_bitmap)(ALLEGRO_BITMAP *bitmap, float cx, float cy,
      float angle, float dx, float dy, float xscale, float yscale,
      float flags);
   /* After the memory-copy of the bitmap has been modified, need to call this
    * to update the display-specific copy. E.g. with an OpenGL driver, this
    * might create/update a texture. Returns false on failure.
    */
   bool (*upload_bitmap)(ALLEGRO_BITMAP *bitmap, int x, int y, int width, int height);
   /* If the display version of the bitmap has been modified, use this to update
    * the memory copy accordingly. E.g. with an OpenGL driver, this might
    * read the contents of an associated texture.
    */

   void (*destroy_bitmap)(ALLEGRO_BITMAP *bitmap);

   ALLEGRO_LOCKED_REGION * (*lock_region)(ALLEGRO_BITMAP *bitmap,
   	int x, int y, int w, int h,
   	ALLEGRO_LOCKED_REGION *locked_region,
	int flags);

   void (*unlock_region)(ALLEGRO_BITMAP *bitmap);
};

void _al_blit_memory_bitmap(ALLEGRO_BITMAP *source, ALLEGRO_BITMAP *dest,
   int source_x, int source_y, int dest_x, int dest_y, int w, int h);
//ALLEGRO_BITMAP_INTERFACE *_al_bitmap_xdummy_driver(void);
ALLEGRO_BITMAP_INTERFACE *_al_bitmap_d3ddummy_driver(void);

/* Bitmap conversion */
void _al_convert_bitmap_data(
	void *src, int src_format, int src_pitch,
	void *dst, int dst_format, int dst_pitch,
	int sx, int sy, int dx, int dy,
	int width, int height);
void _al_convert_compat_bitmap(
	BITMAP *src,
	void *dst, int dst_format, int dst_pitch,
	int sx, int sy, int dx, int dy,
	int width, int height);
int _al_get_pixel_value(int src_format, ALLEGRO_COLOR *src_color);
int _al_get_compat_bitmap_format(BITMAP *bmp);
bool _al_format_has_alpha(int format);
bool _al_pixel_format_is_real(int format);

void _al_push_new_bitmap_parameters();
void _al_pop_new_bitmap_parameters();

/* Memory bitmap blitting */
void _al_draw_bitmap_region_memory(ALLEGRO_BITMAP *bitmap,
   int sx, int sy, int sw, int sh,
   int dx, int dy, int flags);
void _al_draw_bitmap_memory(ALLEGRO_BITMAP *bitmap,
   int dx, int dy, int flags);
void _al_draw_scaled_bitmap_memory(ALLEGRO_BITMAP *bitmap,
   int sx, int sy, int sw, int sh, int dx, int dy, int dw, int dh, int flags);
void _al_draw_rotated_bitmap_memory(ALLEGRO_BITMAP *bitmap,
   int center_x, int center_y, int dx, int dy, float angle, int flags);
void _al_draw_rotated_bitmap_memory(ALLEGRO_BITMAP *bitmap,
   int center_x, int center_y, int dx, int dy,
   float angle, int flags);
void _al_draw_rotated_scaled_bitmap_memory(ALLEGRO_BITMAP *bitmap,
   int center_x, int center_y, int dx, int dy,
   float xscale, float yscale, float angle, int flags);


/* For blending memory bitmaps */
typedef void (*ALLEGRO_MEMORY_BLENDER)(
   int src_format,
   ALLEGRO_COLOR *src_color,
   int dest_format,
   ALLEGRO_COLOR *dest_color,
   ALLEGRO_COLOR *result);

/* Blending stuff (should this be somewhere else? */
ALLEGRO_INDEPENDANT_COLOR *_al_get_blend_color();
void _al_set_memory_blender(int src, int dst, ALLEGRO_COLOR *color);
ALLEGRO_MEMORY_BLENDER _al_get_memory_blender();
void _al_blender_zero_zero(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_zero_one(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_zero_alpha(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_zero_inverse_alpha(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_one_zero(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_one_one(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_one_alpha(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_one_inverse_alpha(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_alpha_zero(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_alpha_one(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_alpha_alpha(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_alpha_inverse_alpha(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_inverse_alpha_zero(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_inverse_alpha_one(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_inverse_alpha_alpha(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);
void _al_blender_inverse_alpha_inverse_alpha(int src_format, ALLEGRO_COLOR *src_color, int dst_format, ALLEGRO_COLOR *dst_color, ALLEGRO_COLOR *result);

/* Pixel mapping */
ALLEGRO_COLOR *_al_map_rgb(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	unsigned char r, unsigned char g, unsigned char b);
ALLEGRO_COLOR *_al_map_rgba(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a);
ALLEGRO_COLOR *_al_map_rgb_f(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	float r, float g, float b);
ALLEGRO_COLOR *_al_map_rgba_f(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	float r, float g, float b, float a);
ALLEGRO_COLOR *_al_map_rgb_i(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	int r, int g, int b);
ALLEGRO_COLOR *_al_map_rgba_i(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	int r, int g, int b, int a);

/* Pixel unmapping */
void _al_unmap_rgb(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	unsigned char *r, unsigned char *g, unsigned char *b);
void _al_unmap_rgba(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	unsigned char *r, unsigned char *g, unsigned char *b, unsigned char *a);
void _al_unmap_rgb_f(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	float *r, float *g, float *b);
void _al_unmap_rgba_f(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	float *r, float *g, float *b, float *a);
void _al_unmap_rgb_i(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	int *r, int *g, int *b);
void _al_unmap_rgba_i(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *color,
	int *r, int *g, int *b, int *a);


#endif
