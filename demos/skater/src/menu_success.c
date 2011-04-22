#include <allegro5/allegro.h>
#include "defines.h"
#include "global.h"
#include "menu.h"
#include "menus.h"
#include "music.h"

static int _id = DEMO_STATE_SUCCESS;

static int id(void)
{
   return _id;
}


static DEMO_MENU menu[] = {
   {demo_text_proc, "Well done! Ted's stock is saved!", 0, 0, 0, 0},
   {demo_text_proc, " ", 0, 0, 0, 0},
   {demo_text_proc, "This demo has shown only a fraction", 0, 0, 0,
    0},
   {demo_text_proc, "of Allegro's capabilities.", 0, 0, 0, 0},
   {demo_text_proc, " ", 0, 0, 0, 0},
   {demo_text_proc, "Now it's up to you to show the world the rest!",
    0, 0,
    0, 0},
   {demo_text_proc, "Get coding!", 0, 0, 0, 0},
   {demo_text_proc, " ", 0, 0, 0, 0},
   {demo_button_proc, "Back", DEMO_MENU_SELECTABLE,
    DEMO_STATE_MAIN_MENU,
    0, 0},
   {0, 0, 0, 0, 0, 0}
};


static void init(void)
{
   init_demo_menu(menu, false);
   disable_continue_game();
   play_music(DEMO_MIDI_SUCCESS, false);
}


static int update(void)
{
   int ret = update_demo_menu(menu);

   switch (ret) {
      case DEMO_MENU_CONTINUE:
         return id();

      case DEMO_MENU_BACK:
         return DEMO_STATE_MAIN_MENU;

      default:
         return ret;
   };
}


static void draw(void)
{
   draw_demo_menu(menu);
}


void create_success_menu(GAMESTATE * state)
{
   state->id = id;
   state->init = init;
   state->update = update;
   state->draw = draw;
}
