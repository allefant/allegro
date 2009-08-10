/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Compare software blending routines with hardware blending.
 */

#include <string>
#include "allegro5/allegro5.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_image.h"
#include <allegro5/allegro_primitives.h>

#include "nihgui.hpp"

ALLEGRO_BITMAP *allegro;
ALLEGRO_BITMAP *mysha;
ALLEGRO_BITMAP *allegro_bmp;
ALLEGRO_BITMAP *mysha_bmp;
ALLEGRO_BITMAP *target;
ALLEGRO_BITMAP *target_bmp;

class Prog {
private:
   Dialog d;
   Label memory_label;
   Label texture_label;
   Label source_label;
   Label destination_label;
   Label blending_label;
   List source_image;
   List destination_image;
   Label operation_label[4];
   List operations[4];
   VSlider r[3];
   VSlider g[3];
   VSlider b[3];
   VSlider a[3];

public:
   Prog(const Theme & theme, ALLEGRO_DISPLAY *display);
   void run();

private:
   void blending_test(bool memory);
   void draw_samples();
   void draw_bitmap(const std::string &, bool, bool);
};

Prog::Prog(const Theme & theme, ALLEGRO_DISPLAY *display) :
   d(Dialog(theme, display, 20, 40)),
   memory_label(Label("Memory")),
   texture_label(Label("Texture")),
   source_label(Label("Source", false)),
   destination_label(Label("Destination", false)),
   blending_label(Label("Blending", false)),
   destination_image(List(1))
{
   d.add(memory_label, 10, 0, 10, 2);
   d.add(texture_label, 0, 0, 10, 2);
   d.add(source_label, 1, 19, 6, 2);
   d.add(destination_label, 7, 19, 6, 2);
   d.add(blending_label, 13, 19, 6, 2);

   List *images[] = {&source_image, &destination_image};
   for (int i = 0; i < 2; i++) {
      List & image = *images[i];
      image.append_item("Mysha");
      image.append_item("Allegro");
      image.append_item("Color");
      d.add(image, 1 + i * 6, 21, 4, 4);
   }

   for (int i = 0; i < 4; i++) {
      operation_label[i] = Label(i % 2 == 0 ? "Color" : "Alpha", false);
      d.add(operation_label[i], 1 + i * 3, 25, 3, 2);
      List &l = operations[i];
      l.append_item("ONE");
      l.append_item("ZERO");
      l.append_item("ALPHA");
      l.append_item("INVERSE");
      d.add(l, 1 + i * 3, 27, 3, 6);
   }

   for (int i = 0; i < 3; i++) {
      r[i] = VSlider(255, 255);
      g[i] = VSlider(255, 255);
      b[i] = VSlider(255, 255);
      a[i] = VSlider(255, 255);
      d.add(r[i], 1 + i * 6, 33, 1, 6);
      d.add(g[i], 2 + i * 6, 33, 1, 6);
      d.add(b[i], 3 + i * 6, 33, 1, 6);
      d.add(a[i], 4 + i * 6, 33, 1, 6);
   }
}

void Prog::run()
{
   d.prepare();

   while (!d.is_quit_requested()) {
      if (d.is_draw_requested()) {
         al_clear_to_color(al_map_rgb(128, 128, 128));
         draw_samples();
         d.draw();
         al_flip_display();
      }

      d.run_step(true);
   }
}

int str_to_blend_mode(const std::string & str)
{
   if (str == "ZERO")
      return ALLEGRO_ZERO;
   if (str == "ONE")
      return ALLEGRO_ONE;
   if (str == "ALPHA")
      return ALLEGRO_ALPHA;
   if (str == "INVERSE")
      return ALLEGRO_INVERSE_ALPHA;

   ALLEGRO_ASSERT(false);
   return ALLEGRO_ONE;
}

void draw_background(int x, int y)
{
   ALLEGRO_COLOR c[] = {
      al_map_rgba(0x66, 0x66, 0x66, 0xff),
      al_map_rgba(0x99, 0x99, 0x99, 0xff)
   };

   for (int i = 0; i < 640 / 16; i++) {
      for (int j = 0; j < 200 / 16; j++) {
         al_draw_filled_rectangle(x + i * 16, y + j * 16,
            x + i * 16 + 16, y + j * 16 + 16,
            c[(i + j) & 1]);
      }
   }
}

void Prog::draw_bitmap(const std::string & str, bool memory,
   bool destination)
{
   int i = destination ? 1 : 0;
   int rv = r[i].get_cur_value();
   int gv = g[i].get_cur_value();
   int bv = b[i].get_cur_value();
   int av = a[i].get_cur_value();
   ALLEGRO_COLOR color = al_map_rgba(rv, gv, bv, av);

   if (str == "Mysha")
      al_draw_bitmap(memory ? mysha_bmp : mysha, 0, 0, 0);
   else if (str == "Allegro")
      al_draw_bitmap(memory ? allegro_bmp : allegro, 0, 0, 0);
   else if (str == "Color")
      al_draw_filled_rectangle(0, 0, 320, 200, color);
}

void Prog::blending_test(bool memory)
{
   ALLEGRO_COLOR opaque_white = al_map_rgba_f(1, 1, 1, 1);
   int src = str_to_blend_mode(operations[0].get_selected_item_text());
   int asrc = str_to_blend_mode(operations[1].get_selected_item_text());
   int dst = str_to_blend_mode(operations[2].get_selected_item_text());
   int adst = str_to_blend_mode(operations[3].get_selected_item_text());
   int rv = r[2].get_cur_value();
   int gv = g[2].get_cur_value();
   int bv = b[2].get_cur_value();
   int av = a[2].get_cur_value();

   /* Initialize with destination. */
   al_clear_to_color(opaque_white); // Just in case.
   al_set_blender(ALLEGRO_ONE, ALLEGRO_ZERO, opaque_white);
   draw_bitmap(destination_image.get_selected_item_text(), memory, true);

   /* Now draw the blended source over it. */
   al_set_separate_blender(src, dst, asrc, adst,
      al_map_rgba(rv, gv, bv, av));
   draw_bitmap(source_image.get_selected_item_text(), memory, false);
}

void Prog::draw_samples()
{
   ALLEGRO_STATE state;
   al_store_state(&state, ALLEGRO_STATE_ALL);
      
   /* Draw a background, in case our target bitmap will end up with
    * alpha in it.
    */
   draw_background(0, 20);
   
   /* Test standard blending. */
   al_set_target_bitmap(target);
   blending_test(false);

   /* Test memory blending. */
   al_set_target_bitmap(target_bmp);
   blending_test(true);

   /* Display results. */
   al_set_target_bitmap(al_get_backbuffer());
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA,
      al_map_rgba(255, 255, 255, 255));
   al_draw_bitmap(target, 0, 20, 0);
   al_draw_bitmap(target_bmp, 320, 20, 0);
 
   al_restore_state(&state);
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

   al_init_font_addon();
   al_init_image_addon();

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Unable to create display\n");
      return 1;
   }
   font = al_load_font("data/fixed_font.tga", 0, 0);
   if (!font) {
      TRACE("Failed to load data/fixed_font.tga\n");
      return 1;
   }
   allegro = al_load_bitmap("data/allegro.pcx");
   if (!allegro) {
      TRACE("Failed to load data/allegro.pcx\n");
      return 1;
   }
   mysha = al_load_bitmap("data/mysha.pcx");
   if (!mysha) {
      TRACE("Failed to load data/mysha.pcx\n");
      return 1;
   }
   
   target = al_create_bitmap(320, 200);

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   allegro_bmp = al_clone_bitmap(allegro);
   mysha_bmp = al_clone_bitmap(mysha);
   target_bmp = al_clone_bitmap(target);

   al_show_mouse_cursor();

   /* Don't remove these braces. */
   {
      Theme theme(font);
      Prog prog(theme, display);
      prog.run();
   }

   al_destroy_bitmap(allegro);
   al_destroy_bitmap(allegro_bmp);
   al_destroy_bitmap(mysha);
   al_destroy_bitmap(mysha_bmp);
   al_destroy_bitmap(target);
   al_destroy_bitmap(target_bmp);

   al_destroy_font(font);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
