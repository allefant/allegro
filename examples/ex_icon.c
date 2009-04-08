#include <allegro5/allegro5.h>
#include "allegro5/a5_iio.h"

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *icon1;
   ALLEGRO_BITMAP *icon2;
   int i;

   if (!al_init()) {
      TRACE("Could not init Allegro.\n");
      return 1;
   }

   al_install_mouse();
   al_init_iio_addon();

   display = al_create_display(320, 200);
   if (!display) {
      TRACE("Error creating display\n");
      return 1;
   }

   /* First icon: Read from file. */
   icon1 = al_load_image("data/icon.tga");
   if (!icon1) {
      TRACE("icon.tga not found\n");
      return 1;
   }

   /* Second icon: Create it. */
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   icon2 = al_create_bitmap(16, 16);
   al_set_target_bitmap(icon2);
   for (i = 0; i < 256; i++) {
      int u = i % 16;
      int v = i / 16;
      al_put_pixel(u, v, al_map_rgb_f(u / 15.0, v / 15.0, 1));
   }
   al_set_target_bitmap(al_get_backbuffer());

   al_set_window_title("<-- Changing icon example");

   for (i = 0; i < 8; i++) {
      al_set_display_icon((i & 1) ? icon2 : icon1);
      al_flip_display();
      al_rest(1.0);
   }

   return 0;
}
END_OF_MAIN()
