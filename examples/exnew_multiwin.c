#include "allegro5/allegro5.h"

const int W = 640;
const int H = 400;

int main(void)
{
   ALLEGRO_DISPLAY *display[2];
   ALLEGRO_KEYBOARD *keyboard;
   ALLEGRO_EVENT event;
   ALLEGRO_EVENT_QUEUE *events;
   ALLEGRO_BITMAP *pictures[2];
   bool quit = false;
   int i;

   al_init();

   events = al_create_event_queue();

   al_set_new_display_flags(ALLEGRO_WINDOWED|ALLEGRO_RESIZABLE);

   /* Create two windows. */
   display[0] = al_create_display(W, H);
   display[1] = al_create_display(W, H);

   /* This is only needed since we want to receive resize events. */
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)display[0]);
   al_register_event_source(events, (ALLEGRO_EVENT_SOURCE *)display[1]);

   pictures[0] = al_load_bitmap("mysha.pcx");
   if (!pictures[0]) {
      allegro_message("failed to load mysha.pcx");
      return 1;
   }

   pictures[1] = al_load_bitmap("allegro.pcx");
   if (!pictures[1]) {
      allegro_message("failed to load allegro.pcx");
      return 1;
   }

   while (!quit) {
      /* read input */
      while (!al_event_queue_is_empty(events)) {
         al_get_next_event(events, &event);
         if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            ALLEGRO_KEYBOARD_EVENT *key = &event.keyboard;
            if (key->keycode == ALLEGRO_KEY_ESCAPE) {
               quit = 1;
            }
         }
         if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
            ALLEGRO_DISPLAY_EVENT *display = &event.display;
            al_acknowledge_resize(display->source);
         }
         if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            int i;
            for (i = 0; i < 2; i++) {
               if (display[i] == event.display.source)
                  display[i] = 0;
            }
            al_destroy_display(event.display.source);
            for (i = 0; i < 2; i++) {
               if (display[i])
                  goto not_done;
            }
            quit = 1;
         not_done:
            ;
         }
      }

      for (i = 0; i < 2; i++) {
         if (!display[i])
            continue;

         al_set_current_display(display[i]);
         al_draw_scaled_bitmap(pictures[i], 0, 0,
            al_get_bitmap_width(pictures[i]),
            al_get_bitmap_height(pictures[i]),
            0, 0,
            al_get_display_width(),
            al_get_display_height(), 0);

         al_flip_display();
      }
   }

   al_destroy_bitmap(pictures[0]);
   al_destroy_bitmap(pictures[1]);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
