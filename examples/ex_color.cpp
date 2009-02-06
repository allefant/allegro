/*
 *    Example program for the Allegro library, by Elias Pschernig.
 *
 *    Demonstrates some of the conversion functions in the color addon.
 */

#include <string>
#include "allegro5/allegro5.h"
#include "allegro5/a5_ttf.h"
#include "allegro5/a5_color.h"

#include "nihgui.hpp"

#define SLIDERS_COUNT 16
char const *names[] = {"R", "G", "B", "H", "S", "V", "H", "S", "L",
    "Y", "U", "V", "C", "M", "Y", "K"};

class Prog {
private:
   Dialog d;
   VSlider sliders[SLIDERS_COUNT];
   Label labels[SLIDERS_COUNT];
   Label labels2[SLIDERS_COUNT];
   int previous[SLIDERS_COUNT];

public:
   Prog(const Theme & theme, ALLEGRO_DISPLAY *display);
   void run();   
};

Prog::Prog(const Theme & theme, ALLEGRO_DISPLAY *display) :
   d(Dialog(theme, display, 640, 480))
{
   for (int i = 0; i < SLIDERS_COUNT; i++) {
      int j = i < 12 ? i / 3 : 4;
      sliders[i] = VSlider(1000, 1000);
      d.add(sliders[i], 8 + i * 32 + j * 16, 8, 15, 256);
      labels[i].set_text(names[i]);
      d.add(labels[i], i * 32 + j * 16, 8 + 256, 32, 20);
      d.add(labels2[i], i * 32 + j * 16, 8 + 276, 32, 20);
      previous[i] = 0;
   }
}

namespace {
float clamp(float x)
{
    if (x < 0) return 0;
    if (x > 1) return 1;
    return x;
}
};

void Prog::run()
{
   d.prepare();

   while (!d.is_quit_requested()) {
      if (d.is_draw_requested()) {
         al_clear(al_map_rgb(128, 128, 128));
         float v[SLIDERS_COUNT];
         int keep = -1;
         for (int i = 0; i < SLIDERS_COUNT; i++) {
            int x = sliders[i].get_cur_value();
            v[i] = x / 1000.0;
            if (previous[i] != x) {
               keep = i;
            }
         }

         if (keep != -1) {
            int space = keep < 12 ? keep / 3 : 4;
            switch (space) {
               case 0:
                  al_color_rgb_to_hsv(v[0], v[1], v[2], v + 3, v + 4, v + 5);
                  al_color_rgb_to_hsl(v[0], v[1], v[2], v + 6, v + 7, v + 8);
                  al_color_rgb_to_cmyk(v[0], v[1], v[2], v + 12, v + 13, v + 14, v + 15);
                  al_color_rgb_to_yuv(v[0], v[1], v[2], v + 9, v + 10, v + 11);
                  v[3] /= 360;
                  v[6] /= 360;
                  break;
               case 1:
                  al_color_hsv_to_rgb(v[3] * 360, v[4], v[5], v + 0, v + 1, v + 2);
                  al_color_rgb_to_hsl(v[0], v[1], v[2], v + 6, v + 7, v + 8);
                  al_color_rgb_to_cmyk(v[0], v[1], v[2], v + 12, v + 13, v + 14, v + 15);
                  al_color_rgb_to_yuv(v[0], v[1], v[2], v + 9, v + 10, v + 11);
                  v[6] /= 360;
                  break;
               case 2:
                  al_color_hsl_to_rgb(v[6] * 360, v[7], v[8], v + 0, v + 1, v + 2);
                  al_color_rgb_to_hsv(v[0], v[1], v[2], v + 3, v + 4, v + 5);
                  al_color_rgb_to_cmyk(v[0], v[1], v[2], v + 12, v + 13, v + 14, v + 15);
                  al_color_rgb_to_yuv(v[0], v[1], v[2], v + 9, v + 10, v + 11);
                  v[3] /= 360;
                  break;
               case 3:
                  al_color_yuv_to_rgb(v[9], v[10], v[11], v + 0, v + 1, v + 2);
                  v[0] = clamp(v[0]);
                  v[1] = clamp(v[1]);
                  v[2] = clamp(v[2]);
                  al_color_rgb_to_yuv(v[0], v[1], v[2], v + 9, v + 10, v + 11);
                  al_color_rgb_to_hsv(v[0], v[1], v[2], v + 3, v + 4, v + 5);
                  al_color_rgb_to_hsl(v[0], v[1], v[2], v + 6, v + 7, v + 8);
                  al_color_rgb_to_cmyk(v[0], v[1], v[2], v + 12, v + 13, v + 14, v + 15);
                  v[3] /= 360;
                  v[6] /= 360;
                  break;
               case 4:
                  al_color_cmyk_to_rgb(v[12], v[13], v[14], v[15], v + 0, v + 1, v + 2);
                  al_color_rgb_to_hsv(v[0], v[1], v[2], v + 3, v + 4, v + 5);
                  al_color_rgb_to_hsl(v[0], v[1], v[2], v + 6, v + 7, v + 8);
                  al_color_rgb_to_yuv(v[0], v[1], v[2], v + 9, v + 10, v + 11);
                  v[3] /= 360;
                  v[6] /= 360;
                  break;
            }
         }
         
         for (int i = 0; i < SLIDERS_COUNT; i++) {
            sliders[i].set_cur_value(v[i] * 1000);
            previous[i] = sliders[i].get_cur_value();
            char c[100];
            sprintf(c, "%d", (int)(v[i] * 100));
            labels2[i].set_text(c);
         }
         
         d.draw();
         
         al_draw_rectangle(0, 400, 640, 480,
            al_map_rgb_f(v[0], v[1], v[2]), ALLEGRO_FILLED);
         char const *name = al_color_rgb_to_name(v[0], v[1], v[2]);
         char html[8];
         al_color_rgb_to_html(v[0], v[1], v[2], html);
         ALLEGRO_STATE state;
         al_store_state(&state, ALLEGRO_STATE_ALL);
         al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
            al_map_rgb(0, 0, 0));
         al_font_textout(d.get_theme().font, 0, 380, name, -1);
         al_font_textout(d.get_theme().font, 0, 360, html, -1);
         al_restore_state(&state);

         al_flip_display();
      }

      d.run_step(true);
   }
}

int main()
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_FONT *font;

   if (!al_init()) {
      TRACE("Could not init Allegro\n");
      return 1;
   }
   al_install_keyboard();
   al_install_mouse();

   al_font_init();
   al_iio_init();

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Unable to create display\n");
      return 1;
   }
   font = al_ttf_load_font("data/DejaVuSans.ttf", 12, 0);
   if (!font) {
      TRACE("Failed to load data/DejaVuSans.ttf\n");
      return 1;
   }

   al_show_mouse_cursor();

   Theme theme(font);
   Prog prog(theme, display);
   prog.run();

   al_font_destroy_font(font);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
