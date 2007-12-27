/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      New bitmap routines.
 *
 *      By Elias Pschernig and Trent Gamblin.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Bitmap routines
 */


#include <string.h>
#include "allegro5.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"



/* Creates a memory bitmap. A memory bitmap can only be drawn to other memory
 * bitmaps, not to a display.
 */
static ALLEGRO_BITMAP *_al_create_memory_bitmap(int w, int h)
{
   ALLEGRO_BITMAP *bitmap;
   int format = al_get_new_bitmap_format();
   
   /* Pick an appropriate format if the user is vague */
   switch (format) {
      case ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA:
      case ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA:
         format = ALLEGRO_PIXEL_FORMAT_XRGB_8888;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA:
      case ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA:
         format = ALLEGRO_PIXEL_FORMAT_ARGB_8888;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA:
         format = ALLEGRO_PIXEL_FORMAT_RGB_555;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA:
         format = ALLEGRO_PIXEL_FORMAT_ARGB_1555;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA:
         format = ALLEGRO_PIXEL_FORMAT_RGB_565;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA:
         format = ALLEGRO_PIXEL_FORMAT_ARGB_4444;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA:
         format = ALLEGRO_PIXEL_FORMAT_RGB_888;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA:
         /* We don't support any 24 bit formats with alpha */
         return NULL;
      default:
         break;
   }

   bitmap = _AL_MALLOC(sizeof *bitmap);

   memset(bitmap, 0, sizeof *bitmap);

   bitmap->format = format;
   bitmap->flags = al_get_new_bitmap_flags();
   bitmap->w = w;
   bitmap->h = h;
   bitmap->display = NULL;
   bitmap->locked = false;
   bitmap->cl = bitmap->ct = 0;
   bitmap->cr = w-1;
   bitmap->cb = h-1;
   bitmap->parent = NULL;
   bitmap->xofs = bitmap->yofs = 0;
   // FIXME: Of course, we do need to handle all the possible different formats,
   // this will easily fill up its own file of 1000 lines, but for now,
   // RGBA with 8-bit per component is hardcoded.
   bitmap->memory = _AL_MALLOC(w * h * al_get_pixel_size(format));
   memset(bitmap->memory, 0, w * h * al_get_pixel_size(format));
   return bitmap;
}



static void _al_destroy_memory_bitmap(ALLEGRO_BITMAP *bmp)
{
   _AL_FREE(bmp->memory);
   _AL_FREE(bmp);
}



/* Function: al_create_bitmap
 *
 * Creates a new bitmap using the bitmap format and flags for the current
 * thread. Blitting between bitmaps of differing formats, or blitting between
 * memory bitmaps and display bitmaps may be slow.
 *
 * See Also: <al_set_new_bitmap_format>, <al_set_new_bitmap_flags>
 */
ALLEGRO_BITMAP *al_create_bitmap(int w, int h)
{
   ALLEGRO_BITMAP *bitmap;
   
   if (al_get_new_bitmap_flags() & ALLEGRO_MEMORY_BITMAP) {
      return _al_create_memory_bitmap(w, h);
   }

   /* Else it's a display bitmap */

   bitmap = _al_current_display->vt->create_bitmap(_al_current_display, w, h);

   bitmap->display = _al_current_display;
   bitmap->w = w;
   bitmap->h = h;
   bitmap->locked = false;
   bitmap->cl = bitmap->ct = 0;
   bitmap->cr = w-1;
   bitmap->cb = h-1;
   bitmap->parent = NULL;
   bitmap->xofs = bitmap->yofs = 0;

   if (!bitmap->memory) {
      bitmap->memory = _AL_MALLOC(w * h * al_get_pixel_size(bitmap->format));
      memset(bitmap->memory, 0, w * h * al_get_pixel_size(bitmap->format));
   }

   if (!bitmap->vt->upload_bitmap(bitmap, 0, 0, w, h)) {
      al_destroy_bitmap(bitmap);
      return NULL;
   }

   return bitmap;
}



/* Function: al_destroy_bitmap
 *
 * Destroys the given bitmap, freeing all resources used by it.
 * Does nothing if given the null pointer.
 */
void al_destroy_bitmap(ALLEGRO_BITMAP *bitmap)
{
   if (!bitmap) {
      return;
   }

   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_destroy_memory_bitmap(bitmap);
      return;
   }
   else if (bitmap->parent) {
      /* It's a sub-bitmap */
      _AL_FREE(bitmap);
      return;
   }

   /* Else it's a display bitmap */

   if (bitmap->locked)
      al_unlock_bitmap(bitmap);

   if (bitmap->vt)
      bitmap->vt->destroy_bitmap(bitmap);

   if (bitmap->memory)
      _AL_FREE(bitmap->memory);

   _AL_FREE(bitmap);
}



/* TODO
 * This will load a bitmap from a file into a memory bitmap, keeping it in the
 * same format as on disk. That is, a paletted image will be stored as indices
 * plus the palette, 16-bit color channels stay as 16-bit, and so on. If
 * a format is not supported by Allegro, the closest one will be used. (E.g. we
 * likely will throw away things like frame timings or gamma correction.)
 */
static ALLEGRO_BITMAP *_al_load_memory_bitmap(char const *filename)
{
   // TODO:
   // The idea is, load_bitmap returns a memory representation of the bitmap,
   // maybe in fixed RGBA or whatever format is set as default (e.g. for HDR
   // images, should not reduce to 8-bit channels).
   // Then, it is converted to a display specific bitmap, which can be used
   // for blitting to the current display.
   int flags = al_get_new_bitmap_flags();

   set_color_conversion(COLORCONV_NONE);
   // FIXME: should not use the 4.2 function here of course
   PALETTE pal;
   BITMAP *file_data = load_bitmap(filename, pal);
   select_palette(pal);
   if (!file_data) {
      return NULL;
   }

   if (flags & ALLEGRO_KEEP_BITMAP_FORMAT) {
      _al_push_new_bitmap_parameters();
      al_set_new_bitmap_format(_al_get_compat_bitmap_format(file_data));
   }

   ALLEGRO_BITMAP *bitmap = al_create_bitmap(file_data->w, file_data->h);

   if (flags & ALLEGRO_KEEP_BITMAP_FORMAT) {
      _al_pop_new_bitmap_parameters();
   }

   if (!bitmap)
      return NULL;

   _al_convert_compat_bitmap(
      file_data,
      bitmap->memory, bitmap->format,
      al_get_pixel_size(bitmap->format) * bitmap->w,
      0, 0, 0, 0,
      file_data->w, file_data->h);

   destroy_bitmap(file_data);
   return bitmap;
}



/* Function: al_load_bitmap
 *
 * Load a bitmap from a file using the bitmap format and flags of
 * the current thread.
 *
 * See Also: <al_set_new_bitmap_format>, <al_set_new_bitmap_flags>
 */
ALLEGRO_BITMAP *al_load_bitmap(char const *filename)
{
   ALLEGRO_BITMAP *bitmap;
  
   bitmap = _al_load_memory_bitmap(filename);

   /* If it's a display bitmap */
   if (!(al_get_new_bitmap_flags() & ALLEGRO_MEMORY_BITMAP) && bitmap) {
      bitmap->vt->upload_bitmap(bitmap, 0, 0, bitmap->w, bitmap->h);
   }

   return bitmap;
}



/* Function: al_draw_bitmap
 *
 * Draws an unscaled, unrotated bitmap at the given position
 * to the current target bitmap (see al_set_target_bitmap).
 * flags can be a combination of:
 *
 * > ALLEGRO_FLIP_HORIZONTAL
 * > ALLEGRO_FLIP_VERTICAL
 */
void al_draw_bitmap(ALLEGRO_BITMAP *bitmap, float dx, float dy, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   /* If one is a memory bitmap, do memory blit */
   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
         (dest->flags & ALLEGRO_MEMORY_BITMAP)) {
      if (_al_current_display->vt->draw_memory_bitmap_region)
         _al_current_display->vt->draw_memory_bitmap_region(_al_current_display,
	    bitmap, 0, 0, bitmap->w, bitmap->h, dx, dy, flags);
      else
         _al_draw_bitmap_memory(bitmap, dx, dy, flags);
   }
   else if (al_is_compatible_bitmap(bitmap))
      bitmap->vt->draw_bitmap(bitmap, dx, dy, flags);
}



/* Function: al_draw_bitmap_region
 *
 * Draws a region of the given bitmap to the target bitmap.
 *
 * sx - source x
 * sy - source y
 * sw - source width (width of region to blit)
 * sh - source height (height of region to blit)
 * dx - destination x
 * dy - destination y
 * flags - same as for al_draw_bitmap
 */
void al_draw_bitmap_region(ALLEGRO_BITMAP *bitmap, float sx, float sy,
	float sw, float sh, float dx, float dy, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   /* If one is a memory bitmap, do memory blit */
   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
       (dest->flags & ALLEGRO_MEMORY_BITMAP))
   {
      if (_al_current_display &&
          _al_current_display->vt->draw_memory_bitmap_region)
      {
         _al_current_display->vt->draw_memory_bitmap_region(_al_current_display,
	    bitmap, sx, sy, sw, sh, dx, dy, flags);
      }
      else {
         _al_draw_bitmap_region_memory(bitmap, sx, sy, sw, sh, dx, dy, flags);
      }
   }
   else if (al_is_compatible_bitmap(bitmap)) {
      bitmap->vt->draw_bitmap_region(bitmap, sx, sy, sw, sh, dx, dy, flags);
   }
}



/* Function: al_draw_scaled_bitmap
 *
 * Draws a scaled version of the given bitmap to the target bitmap.
 *
 * sx - source x
 * sy - source y
 * sw - source width
 * sh - source height
 * dx - destination width
 * dy - destination height
 * flags - same as for al_draw_bitmap
 */
void al_draw_scaled_bitmap(ALLEGRO_BITMAP *bitmap, float sx, float sy,
	float sw, float sh, float dx, float dy, float dw, float dh, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
       (dest->flags & ALLEGRO_MEMORY_BITMAP))
   {
      _al_draw_scaled_bitmap_memory(bitmap, sx, sy, sw, sh,
         dx, dy, dw, dh, flags);
   }
   else if (al_is_compatible_bitmap(bitmap)) {
      bitmap->vt->draw_scaled_bitmap(bitmap, sx, sy, sw, sh,
         dx, dy, dw, dh, flags);
   }
}



/* Function: al_draw_rotated_bitmap
 *
 * Draws a rotated version of the given bitmap to the target bitmap.
 * The bitmap is rotated by 'angle' radians counter clockwise.
 * The point center_x, center_y will be drawn at dx, dy with the bitmap rotated
 * around that point.
 */
void al_draw_rotated_bitmap(ALLEGRO_BITMAP *bitmap, float cx, float cy,
	float dx, float dy, float angle, int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   /* If one is a memory bitmap, do memory blit */
   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
       (dest->flags & ALLEGRO_MEMORY_BITMAP))
   {
      _al_draw_rotated_bitmap_memory(bitmap, cx, cy,
         dx, dy, angle, flags);
   }
   else if (al_is_compatible_bitmap(bitmap)) {
      bitmap->vt->draw_rotated_bitmap(bitmap, cx, cy, dx, dy, angle, flags);
   }
}



/* Function: al_draw_rotated_scaled_bitmap
 *
 * Like al_draw_rotated_bitmap, but can also scale the bitmap.
 * center_x and center_y are in source bitmap coordinates.
 *
 * xscale - how much to scale on the x-axis (e.g. 2 for twice the size)
 * yscale - how much to scale on the y-axis
 */
void al_draw_rotated_scaled_bitmap(ALLEGRO_BITMAP *bitmap, float cx, float cy,
	float dx, float dy, float xscale, float yscale, float angle,
	int flags)
{
   ALLEGRO_BITMAP *dest = al_get_target_bitmap();

   /* If one is a memory bitmap, do memory blit */
   if ((bitmap->flags & ALLEGRO_MEMORY_BITMAP) ||
       (dest->flags & ALLEGRO_MEMORY_BITMAP))
   {
      _al_draw_rotated_scaled_bitmap_memory(bitmap, cx, cy,
         dx, dy, xscale, yscale, angle, flags);
   }
   else if (al_is_compatible_bitmap(bitmap)) {
      bitmap->vt->draw_rotated_scaled_bitmap(bitmap, cx, cy,
         dx, dy, xscale, yscale, angle, flags);
   }
}



/* Function: al_lock_bitmap_region
 *
 * Like al_lock_bitmap, but only locks a specific area of the bitmap.
 * If the bitmap is a display bitmap, only that area of the texture will
 * be updated when it is unlocked. Locking only the region you indend to
 * modify will be faster than locking the whole bitmap.
 *
 * See Also: <al_lock_bitmap>
 */
ALLEGRO_LOCKED_REGION *al_lock_bitmap_region(ALLEGRO_BITMAP *bitmap,
	int x, int y,
	int width, int height,
	ALLEGRO_LOCKED_REGION *locked_region,
	int flags)
{
   /* For sub-bitmaps */
   if (bitmap->parent) {
      x += bitmap->xofs;
      y += bitmap->yofs;
      bitmap = bitmap->parent;
   }

   if (bitmap->locked)
      return NULL;

   bitmap->locked = true;
   bitmap->lock_x = x;
   bitmap->lock_y = y;
   bitmap->lock_w = width;
   bitmap->lock_h = height;
   bitmap->lock_flags = flags;

   if (bitmap->flags & ALLEGRO_MEMORY_BITMAP ||
       bitmap->flags & ALLEGRO_SYNC_MEMORY_COPY)
   {
      locked_region->data = bitmap->memory
         + (bitmap->w * y + x) * al_get_pixel_size(bitmap->format);
      locked_region->format = bitmap->format;
      locked_region->pitch = bitmap->w*al_get_pixel_size(bitmap->format);
   }
   else {
      locked_region = bitmap->vt->lock_region(bitmap,
         x, y, width, height,
         locked_region,
         flags);
   }

   if (locked_region) {
      memcpy(&bitmap->locked_region, locked_region,
         sizeof(ALLEGRO_LOCKED_REGION));
   }
   else {
      bitmap->locked = false;
   }

   return locked_region;
}




/* Function: al_lock_bitmap
 *
 * Lock an entire bitmap for reading or writing. If the bitmap is a
 * display bitmap it will be updated from system memory after the bitmap
 * is unlocked (unless locked read only). locked_region must point to an
 * already allocated ALLEGRO_LOCKED_REGION structure. Returns false if the
 * bitmap cannot be locked, e.g. the bitmap was locked previously and not
 * unlocked.
 *
 * Flags are:
 * > ALLEGRO_LOCK_READONLY - The locked region will not be written to.
 * >                         This can be faster if the bitmap is a video
 * >                         texture, as it can be discarded after the lock
 * >                         instead of uploaded back to the card. 
 */
ALLEGRO_LOCKED_REGION *al_lock_bitmap(ALLEGRO_BITMAP *bitmap,
   ALLEGRO_LOCKED_REGION *locked_region,
   int flags)
{
   return al_lock_bitmap_region(bitmap, 0, 0, bitmap->w, bitmap->h,
      locked_region, flags);
}



/* Function: al_unlock_bitmap
 *
 * Unlock a previously locked bitmap or bitmap region. If the bitmap
 * is a display bitmap, the texture will be updated to match the system
 * memory copy (unless it was locked read only).
 */
void al_unlock_bitmap(ALLEGRO_BITMAP *bitmap)
{
   /* For sub-bitmaps */
   if (bitmap->parent) {
      bitmap = bitmap->parent;
   }

   if (bitmap->flags & ALLEGRO_SYNC_MEMORY_COPY &&
       !(bitmap->flags & ALLEGRO_MEMORY_BITMAP))
   {
      bitmap->vt->upload_bitmap(bitmap,
      bitmap->lock_x,
      bitmap->lock_y,
      bitmap->lock_w,
      bitmap->lock_h);
   }
   else if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP)) {
      bitmap->vt->unlock_region(bitmap);
   }

   bitmap->locked = false;
}



/* Function: al_convert_mask_to_alpha
 *
 * Convert the given mask color to an alpha channel in the bitmap.
 * Can be used to convert older 4.2-style bitmaps with magic pink
 * to alpha-ready bitmaps.
 */
void al_convert_mask_to_alpha(ALLEGRO_BITMAP *bitmap, ALLEGRO_COLOR *mask_color)
{
   ALLEGRO_LOCKED_REGION lr;
   int x, y;
   ALLEGRO_COLOR pixel;
   ALLEGRO_COLOR alpha_pixel;

   if (!al_lock_bitmap(bitmap, &lr, 0)) {
      TRACE("al_convert_mask_to_alpha: Couldn't lock bitmap.\n");
      return;
   }

   _al_push_target_bitmap();
   al_set_target_bitmap(bitmap);

   _al_map_rgba(bitmap, &alpha_pixel, 0, 0, 0, 0);

   for (y = 0; y < bitmap->h; y++) {
      for (x = 0; x < bitmap->w; x++) {
         al_get_pixel(bitmap, x, y, &pixel);
         if (memcmp(&pixel, mask_color, sizeof(ALLEGRO_COLOR)) == 0) {
            al_put_pixel(x, y, &alpha_pixel);
         }
      }
   }

   _al_pop_target_bitmap();

   al_unlock_bitmap(bitmap);
}



/* Function: al_get_bitmap_width
 *
 * Returns the width of a bitmap in pixels.
 */
int al_get_bitmap_width(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->w;
}



/* Function: al_get_bitmap_height
 * 
 * Returns the height of a bitmap in pixels.
 */
int al_get_bitmap_height(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->h;
}



/* Function: al_get_bitmap_format
 *
 * Returns the pixel format of a bitmap.
 */
int al_get_bitmap_format(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->format;
}



/* Function: al_get_bitmap_flags
 *
 * Return the flags user to create the bitmap.
 */
int al_get_bitmap_flags(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->flags;
}



/* Function: al_create_sub_bitmap
 *
 * Creates a sub-bitmap of the parent, at the specified coordinates and of the
 * specified size. A sub-bitmap is a bitmap that shares drawing memory with a
 * pre-existing (parent) bitmap, but possibly with a different size and
 * clipping settings.
 *
 * If the sub-bitmap does not lie completely inside the parent bitmap, then
 * it is automatically clipped so that it does.
 *
 * The parent bitmap's clipping rectangles are ignored.
 *
 * If a sub-bitmap was not or cannot be created then NULL is returned.
 *
 * Note that destroying parents of sub-bitmaps will not destroy the
 * sub-bitmaps; instead the sub-bitmaps become invalid and should no
 * longer be used.
 */
ALLEGRO_BITMAP *al_create_sub_bitmap(ALLEGRO_BITMAP *parent,
   int x, int y, int w, int h)
{
   ALLEGRO_BITMAP *bitmap;

   if (parent->display && parent->display->vt &&
         parent->display->vt->create_sub_bitmap)
   {
      bitmap = parent->display->vt->create_sub_bitmap(
         parent->display, parent, x, y, w, h);
   }
   else {
      bitmap = _AL_MALLOC(sizeof *bitmap);
      memset(bitmap, 0, sizeof *bitmap);
   }

   bitmap->format = parent->format;
   bitmap->flags = parent->flags;

   /* Clip */
   if (x < 0) {
      w += x;
      x = 0;
   }
   if (y < 0) {
      h += y;
      y = 0;
   }
   if (x+w > parent->w) {
      w = parent->w - x;
   }
   if (y+h > parent->h) {
      h = parent->h - y;
   }

   bitmap->w = w;
   bitmap->h = h;
   bitmap->display = parent->display;
   bitmap->locked = false;
   bitmap->cl = bitmap->ct = 0;
   bitmap->cr = w-1;
   bitmap->cb = h-1;
   bitmap->parent = parent;
   bitmap->xofs = x;
   bitmap->yofs = y;
   bitmap->memory = NULL;

   return bitmap;
}



/* Function: al_clone_bitmap
 *
 * Clone a bitmap "exactly", formats can be different.
 *
 * XXX is this supposed to be public? --pw
 */
ALLEGRO_BITMAP *al_clone_bitmap(ALLEGRO_BITMAP *bitmap)
{
   
   ALLEGRO_BITMAP *clone = al_create_bitmap(bitmap->w, bitmap->h);
   ALLEGRO_LOCKED_REGION dst_region;
   ALLEGRO_LOCKED_REGION src_region;

   if (!clone)
      return NULL;

   if (!al_lock_bitmap(bitmap, &src_region, ALLEGRO_LOCK_READONLY))
      return NULL;

   if (!al_lock_bitmap(clone, &dst_region, 0)) {
      al_unlock_bitmap(bitmap);
      return NULL;
   }

   _al_convert_bitmap_data(
	src_region.data, src_region.format, src_region.pitch,
        dst_region.data, dst_region.format, dst_region.pitch,
        0, 0, 0, 0, bitmap->w, bitmap->h);

   al_unlock_bitmap(bitmap);
   al_unlock_bitmap(clone);

   return clone;
}


/* Function: al_bitmap_is_locked
 *
 * Returns whether or not a bitmap is already locked.
 */
bool al_is_bitmap_locked(ALLEGRO_BITMAP *bitmap)
{
   return bitmap->locked;
}

/* vim: set ts=8 sts=3 sw=3 et: */
