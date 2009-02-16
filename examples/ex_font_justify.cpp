/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Test text justification routines.
 */

#include <string>
#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_ttf.h"
#include <allegro5/a5_primitives.h>
#include "nihgui.hpp"

ALLEGRO_FONT *font;
ALLEGRO_FONT *font_gui;

class Prog {
private:
   Dialog d;
   Label text_label;
   Label width_label;
   Label diff_label;
   TextEntry text_entry;
   HSlider width_slider;
   HSlider diff_slider;

public:
   Prog(const Theme & theme);
   void run();
   void draw_text();
};

Prog::Prog(const Theme & theme) :
   d(Dialog(theme, al_get_current_display(), 10, 20)),
   text_label(Label("Text")),
   width_label(Label("Width")),
   diff_label(Label("Diff")),
   text_entry(TextEntry("Lorem ipsum dolor sit amet")),
   width_slider(HSlider(400, al_get_display_width())),
   diff_slider(HSlider(100, al_get_display_width()))
{
   d.add(text_label, 0, 10, 1, 1);
   d.add(text_entry, 1, 10, 8, 1);

   d.add(width_label,  0, 12, 1, 1);
   d.add(width_slider, 1, 12, 8, 1);

   d.add(diff_label,  0, 14, 1, 1);
   d.add(diff_slider, 1, 14, 8, 1);
}

void Prog::run()
{
   d.prepare();

   while (!d.is_quit_requested()) {
      if (d.is_draw_requested()) {
         al_clear(al_map_rgb(128, 128, 128));
         draw_text();
         d.draw();
         al_flip_display();
      }

      d.run_step(true);
   }
}

void Prog::draw_text()
{
   const int cx = al_get_display_width() / 2;
   const int x1 = cx - width_slider.get_cur_value() / 2;
   const int x2 = cx + width_slider.get_cur_value() / 2;
   const int diff = diff_slider.get_cur_value();
   const int th = al_font_text_height(font);

   al_font_textout_justify(font, x1, x2, 50, diff,
      text_entry.get_text().c_str());

   al_draw_rectangle(x1, 50, x2, 50 + th, al_map_rgb(0, 0, 255), 0);

   al_draw_line(cx - diff / 2, 53 + th, cx + diff / 2, 53 + th,
      al_map_rgb(0, 255, 0), 0);
}

int main()
{
   ALLEGRO_DISPLAY *display;

   if (!al_init()) {
      TRACE("Could not init Allegro\n");
      return 1;
   }
   al_install_keyboard();
   al_install_mouse();

   al_font_init();

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Unable to create display\n");
      return 1;
   }

   /* Test TTF fonts or bitmap fonts. */
#if 1
   font = al_ttf_load_font("data/DejaVuSans.ttf", 24, 0);
   if (!font) {
      TRACE("Failed to load data/DejaVuSans.ttf\n");
      return 1;
   }
#else
   font = al_font_load_font("data/font.tga", 0);
   if (!font) {
      TRACE("Failed to load data/font.tga\n");
      return 1;
   }
#endif

   font_gui = al_font_load_font("data/fixed_font.tga", 0);
   if (!font_gui) {
      TRACE("Failed to load data/DejaVuSans.ttf\n");
      return 1;
   }

   al_show_mouse_cursor();

   /* Don't remove these braces. */
   {
      Theme theme(font_gui);
      Prog prog(theme);
      prog.run();
   }

   al_font_destroy_font(font);
   al_font_destroy_font(font_gui);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
