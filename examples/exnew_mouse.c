#include <allegro.h>

int main(void)
{
	ALLEGRO_DISPLAY *display;
	ALLEGRO_BITMAP *cursor;
	ALLEGRO_MSESTATE msestate;
	ALLEGRO_KBDSTATE kbdstate;
	ALLEGRO_COLOR black;

	al_init();

	display = al_create_display(640, 480);

	al_hide_mouse_cursor();

	cursor = al_load_bitmap("cursor.tga");

	al_map_rgb(&black, 0, 0, 0);

	al_install_mouse();
	al_install_keyboard();

	do {
		al_get_mouse_state(&msestate);
		al_get_keyboard_state(&kbdstate);
		al_clear(&black);
		al_draw_bitmap(cursor, msestate.x, msestate.y, 0);
		al_flip_display();
		al_rest(5);
	} while (!al_key_down(&kbdstate, ALLEGRO_KEY_ESCAPE)
		&& !msestate.buttons);

	return 0;
}
END_OF_MAIN()

