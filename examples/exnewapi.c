/* This is a temporary example to get a feeling for the new API, which was very
 * much in flux yet when this example was created.
 *
 * Right now, it moves a red rectangle across the screen with a speed of
 * 100 pixels / second, limits the FPS to 100 Hz to save CPU, and prints the
 * current FPS in the top right corner.
 */
#include "allegro.h"
#include "allegro/platform/ald3d.h"
#include "allegro/internal/aintern_bitmap.h"
#include <winalleg.h>

#include "d3d.h"

int main(void)
{
   AL_DISPLAY *display[3];
   AL_KEYBOARD *keyboard;
   AL_EVENT event;
   AL_EVENT_QUEUE *events;
   int quit = 0;
   int ticks = 0, last_rendered = 0, start_ticks;
   int fps_accumulator = 0, fps_time = 0;
   double fps = 0;
   int FPS = 100;
   int x = 0, y = 0;
   int dx = 1;
   int w = 640, h = 480;
   AL_BITMAP *picture;
   AL_BITMAP *mask;

	allegro_init();
	set_color_depth(32);
   al_init();

   events = al_create_event_queue();

   /* Create three windows. */
   display[0] = al_create_display(w, h, AL_WINDOWED | AL_OPENGL | AL_RESIZABLE);
   display[1] = al_create_display(w, h, AL_WINDOWED | AL_OPENGL | AL_RESIZABLE);
   display[2] = al_create_display(w, h, AL_WINDOWED | AL_OPENGL);

   /* This is only needed since we want to receive resize events. */
   al_register_event_source(events, (AL_EVENT_SOURCE *)display[0]);
   al_register_event_source(events, (AL_EVENT_SOURCE *)display[1]);
   al_register_event_source(events, (AL_EVENT_SOURCE *)display[2]);

   /* Apparently, need to think a bit more about memory/display bitmaps.. should
    * only need to load it once (as memory bitmap), then make available on all
    * displays.
    */
   al_make_display_current(display[2]);
   picture = al_load_bitmap("mysha.tga", 0);
   mask = al_load_bitmap("mask.pcx", 0);

   al_install_keyboard();
   al_register_event_source(events, (AL_EVENT_SOURCE *)al_get_keyboard());

   start_ticks = al_current_time();

   al_set_light_color(picture, &al_color(1.0f, 1.0f, 0.0f, 0.5f));

   while (!quit) {
      /* read input */
      while (!al_event_queue_is_empty(events)) {
         al_get_next_event(events, &event);
         if (event.type == AL_EVENT_KEY_DOWN)
         {
            AL_KEYBOARD_EVENT *key = &event.keyboard;
            if (key->keycode == AL_KEY_ESCAPE) {
               quit = 1;
	    }
         }
         if (event.type == AL_EVENT_DISPLAY_RESIZE) {
            AL_DISPLAY_EVENT *display = &event.display;
            w = display->width;
            h = display->height;
            al_make_display_current(display->source);
            al_acknowledge_resize();
         }
         if (event.type == AL_EVENT_DISPLAY_CLOSE)
         {
            quit = 1;
         }
      }

      /* handle game ticks */
      while (ticks * 1000 < (al_current_time() - start_ticks) * FPS) {
          x += dx;
          if (x == 0) dx = 1;
          if (x == w - 40) dx = -1;
          ticks++;
      }

      /* render */
      if (ticks > last_rendered) {
         int i;
         AL_COLOR colors[3] = {
            al_color(1, 1, 0, 1.0),
            al_color(0, 1, 0, 0.5),
            al_color(0, 0, 1, 0.5)};
         for (i = 0; i < 3; i++) {
            al_make_display_current(display[i]);
            al_clear(al_color(1.0f, 1.0f, 1.0f, 1.0));
	    if (i == 0) {
	    	al_line(50, 50, 150, 150, colors[0]);
	    }
            else if (i == 2) {
	    	al_blit_scaled(0, picture, 0, 0, picture->w, picture->h,
			0, 0, 0, 640, 479);
	    	al_blit(0, mask, 0, 0, 0);
		al_blit(AL_BLEND|AL_MASK_SOURCE|AL_MASK_INV_DEST, picture, 0, 0, 0);
	    }
            al_filled_rectangle(x, y, x + 40, y + 40, colors[i]);
            al_flip();
         }
         last_rendered = ticks;
         {
            int d = al_current_time() - fps_time;
            fps_accumulator++;
            if (d >= 1000) {
               fps_time += d;
               fps = 1000.0 * fps_accumulator / d;
               fps_accumulator = 0;
            }
         }
      }
      else {
      	int r = start_ticks + 1000 * ticks / FPS - al_current_time();
	al_rest(r < 0 ? 0 : r);
      }
   }

   return 0;
}
END_OF_MAIN();

