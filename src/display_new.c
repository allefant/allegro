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
 *      New display driver.
 *
 *      By Elias Pschernig.
 *
 *      Modified by Trent Gamblin.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Display routines
 */



#include "allegro.h"
#include "allegro/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro/internal/aintern_system.h"
#include "allegro/internal/aintern_display.h"
#include "allegro/internal/aintern_bitmap.h"



// FIXME: The system driver must be used to get drivers!
extern ALLEGRO_DISPLAY_INTERFACE *_al_glx_vt(void);



/* Function: al_create_display
 *
 * Create a display, or window, with the specified dimensions.
 * The parameters of the display are determined by the last calls to
 * al_set_new_display_*. Default parameters are used if none are set
 * explicitly.
 * Creating a new display will automatically make it the active one, with the
 * backbuffer selected for drawing. 
 */
ALLEGRO_DISPLAY *al_create_display(int w, int h)
{
   // FIXME: We need to ask the system driver for a list of possible display
   // drivers here, then select a suitable one according to configuration
   // variables like "display/driver" and according to flags (e.g. OpenGL
   // requested or not).

   // Right now, the X11 driver is hardcoded.

   ALLEGRO_SYSTEM *system = al_system_driver();
   ALLEGRO_DISPLAY_INTERFACE *driver = system->vt->get_display_driver();
   ALLEGRO_DISPLAY *display = driver->create_display(w, h);
   if (!display)
      return NULL;

   ALLEGRO_COLOR black;
   al_set_current_display(display);
   al_set_target_bitmap(al_get_backbuffer());
   __al_map_rgba(display->format, &black, 0, 0, 0, 0);
   al_clear(&black);
   al_flip_display();

   return display;
}



/* Function: al_destroy_display
 *
 * Destroy a display.
 */
void al_destroy_display(ALLEGRO_DISPLAY *display)
{
   display->vt->destroy_display(display);
}



/* Function: al_get_backbuffer
 *
 * Return a special bitmap representing the back-buffer of the
 * current display.
 */
ALLEGRO_BITMAP *al_get_backbuffer(void)
{
   return _al_current_display->vt->get_backbuffer(_al_current_display);
}



/* Function: al_get_frontbuffer
 *
 * Return a special bitmap representing the front-buffer of
 * the current display. This may not be supported by the driver;
 * returns NULL in that case.
 */
ALLEGRO_BITMAP *al_get_frontbuffer(void)
{
   return _al_current_display->vt->get_frontbuffer(_al_current_display);
}



/* Function: al_flip_display
 *
 * Copies or updates the front and back buffers so that what has
 * been drawn previously on the currently selected display becomes
 * visible on screen. Pointers to the special back and front buffer
 * bitmaps remain valid and retain their semantics as back and front
 * buffers respectively, although their contents may have changed.
 */
void al_flip_display(void)
{
   _al_current_display->vt->flip_display(_al_current_display);
}



/* Function: al_update_display_region
 *
 * Update the the front buffer from the backbuffer in the
 * specified region. This does not flip the whole buffer
 * and preserves the contents of the front buffer outside of
 * the given rectangle. This may not be supported by all drivers,
 * in which case it returns false.
 */
bool al_update_display_region(int x, int y,
	int width, int height)
{
   return _al_current_display->vt->update_display_region(
      _al_current_display, x, y, width, height);
}



/* Function: al_acknowledge_resize
 *
 * When the user receives a resize event from a resizable display,
 * if they wish the display to be resized they must call this
 * function to let the graphics driver know that it can now resize
 * the display. Returns true on success.
 *
 * Adjusts the clipping rectangle to the full size of the backbuffer.
 */
bool al_acknowledge_resize(void)
{
   if (!(_al_current_display->flags & ALLEGRO_FULLSCREEN)) {
      if (_al_current_display->vt->acknowledge_resize)
         return _al_current_display->vt->acknowledge_resize(_al_current_display);
   }
   return false;
}



/* Function: al_resize_display
 *
 * Resize the current display. Returns false on error,
 * true on success. This works on both fullscreen and windowed
 * displays, regardless of the ALLEGRO_RESIZABLE flag.
 *
 * Adjusts the clipping rectangle to the full size of the backbuffer.
 */
bool al_resize_display(int width, int height)
{
   if (_al_current_display->vt->resize_display) {
      return _al_current_display->vt->resize_display(_al_current_display,
         width, height);
   }
   return false;
}



/* Function: al_clear
 *
 * Clear a complete display, but confined by the clipping rectangle.
 */
void al_clear(ALLEGRO_COLOR *color)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   if (target->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_clear_memory(color);
   }
   else {
      _al_current_display->vt->clear(_al_current_display, color);
   }
}



/* Function: al_draw_line
 *
 * Draws a line to the current target.
 */
void al_draw_line(float fx, float fy, float tx, float ty,
   ALLEGRO_COLOR* color)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   if (target->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_draw_line_memory(fx, fy, tx, ty, color);
   }
   else {
      _al_current_display->vt->draw_line(_al_current_display,
         fx, fy, tx, ty, color);
   }
}



/* Function: al_draw_rectangle
 *
 * Draws a rectangle to the current target.
 * flags can be:
 *
 * > ALLEGRO_FILLED
 * > ALLEGRO_OUTLINED
 * 
 * Outlined is the default.
 */
void al_draw_rectangle(float tlx, float tly, float brx, float bry,
   ALLEGRO_COLOR *color, int flags)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   if (target->flags & ALLEGRO_MEMORY_BITMAP) {
      _al_draw_rectangle_memory(tlx, tly, brx, bry, color, flags);
   }
   else {
      _al_current_display->vt->draw_rectangle(_al_current_display,
         tlx, tly, brx, bry, color, flags);
   }
}



/* Function: al_is_compatible_bitmap
 *
 * D3D and OpenGL allow sharing a texture in a way so it can be used for
 * multiple windows. Each ALLEGRO_BITMAP created with al_create_bitmap
 * however is usually tied to a single ALLEGRO_DISPLAY. This function can
 * be used to know if the bitmap is compatible with the current display,
 * even if it is another display than the one it was created with. It
 * returns true if the bitmap is compatible (things like a cached texture
 * version can be used) and false otherwise (blitting in the current
 * display will be slow). The only time this function is useful is if you
 * are using multiple windows and need accelerated blitting of the same
 * bitmaps to both. 
 */
bool al_is_compatible_bitmap(ALLEGRO_BITMAP *bitmap)
{
   return _al_current_display->vt->is_compatible_bitmap(
      _al_current_display, bitmap);
}



/* Function: al_get_display_width
 *
 * Gets the width of the current display. This is like SCREEN_W in Allegro 4.x.
 */
int al_get_display_width(void)
{
   return _al_current_display->w;
}



/* Function: al_get_display_height
 *
 * Gets the height of the current display. This is like SCREEN_H in Allegro 4.x.
 */
int al_get_display_height(void)
{
   return _al_current_display->h;
}



/* Function: al_get_display_format
 *
 * Gets the pixel format of the current display.
 */
int al_get_display_format(void)
{
   return _al_current_display->format;
}



/* Function: al_get_display_refresh_rate
 *
 * Gets the refresh rate of the current display.
 */
int al_get_display_refresh_rate(void)
{
   return _al_current_display->refresh_rate;
}



/* Function: al_get_display_flags
 *
 * Gets the flags of the current display.
 */
int al_get_display_flags(void)
{
   return _al_current_display->flags;
}



/* Function: al_get_num_display_modes
 *
 * Get the number of available fullscreen display modes
 * for the current set of display parameters. This will
 * use the values set with al_set_new_display_format,
 * al_set_new_display_refresh_rate, and al_set_new_display_flags
 * to find the number of modes that match. Settings the new
 * display parameters to zero will give a list of all modes
 * for the default driver.
 */
int al_get_num_display_modes(void)
{
   ALLEGRO_SYSTEM *system = al_system_driver();
   return system->vt->get_num_display_modes();
}



/* Function: al_get_display_mode
 *
 * Retrieves a display mode. Display parameters should not be
 * changed between a call of al_get_num_display_modes and
 * al_get_display_mode. index must be between 0 and the number
 * returned from al_get_num_display_modes-1. mode must be an
 * allocated ALLEGRO_DISPLAY_MODE structure. This function will
 * return NULL on failure, and the mode parameter that was passed
 * in on success.
 */
ALLEGRO_DISPLAY_MODE *al_get_display_mode(int index, ALLEGRO_DISPLAY_MODE *mode)
{
   ALLEGRO_SYSTEM *system = al_system_driver();
   return system->vt->get_display_mode(index, mode);
}



/* Function: al_wait_for_vsync
 *
 * Wait for the beginning of a vertical retrace. Some
 * driver/card/monitor combinations may not be capable
 * of this. Returns false if not possible, true if successful.
 */
bool al_wait_for_vsync(void)
{
   if (_al_current_display->vt && _al_current_display->vt->wait_for_vsync)
      return _al_current_display->vt->wait_for_vsync(_al_current_display);
   else
      return false;
}



/* Function: al_set_clipping_rectangle
 *
 * Set the region of the target bitmap or display that
 * pixels get clipped to. The default is to clip pixels
 * to the entire bitmap.
 */
void al_set_clipping_rectangle(int x, int y, int width, int height)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();

   if (x < 0) {
      width += x;
      x = 0;
   }
   if (y < 0) {
      height += y;
      y = 0;
   }
   if (x+width >= bitmap->w) {
      width = bitmap->w - x - 1;
   }
   if (y+height >= bitmap->h) {
      height = bitmap->h - y - 1;
   }

   bitmap->cl = x;
   bitmap->ct = y;
   bitmap->cr = x + width;
   bitmap->cb = y + height;
}



/* Function: al_get_clipping_rectangle
 *
 * Gets the clipping rectangle of the target bitmap.
 */
void al_get_clipping_rectangle(int *x, int *y, int *w, int *h)
{
   ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();

   if (x) *x = bitmap->cl;
   if (y) *y = bitmap->ct;
   if (w) *w = bitmap->cr - bitmap->cl + 1;
   if (h) *h = bitmap->cb - bitmap->ct + 1;
}
