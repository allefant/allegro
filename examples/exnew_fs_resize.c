#include "allegro5/allegro5.h"

void redraw(ALLEGRO_BITMAP *picture)
{
   ALLEGRO_COLOR color;
   color = al_map_rgb(rand()%255, rand()%255, rand()%255);
   al_clear(color);
   color = al_map_rgb(255, 0, 0);
   al_draw_line(0, 0, 640, 480, color);
   al_draw_line(0, 480, 640, 0, color);
   al_draw_bitmap(picture, 0, 0, 0);
   al_flip_display();
}

int main(void)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *picture;

   al_init();

   al_set_new_display_flags(ALLEGRO_FULLSCREEN);
   display = al_create_display(640, 480);

   picture = al_load_bitmap("mysha.pcx");
   if (!picture) {
   	allegro_message("mysha.pcx not found");
   }

   redraw(picture);
   al_rest(2.5);
   al_resize_display(800, 600);
   redraw(picture);
   al_rest(2.5);

   return 0;
}
END_OF_MAIN()

