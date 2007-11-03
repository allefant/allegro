#include "allegro.h"

int main(void)
{
    ALLEGRO_DISPLAY *display;
    ALLEGRO_BITMAP *bitmap;
    ALLEGRO_LOCKED_REGION locked;
    int i;
    al_init();
    display = al_create_display(100, 100);
    al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA);
    bitmap = al_create_bitmap(100, 100);
    al_lock_bitmap(bitmap, &locked, 0);
    for (i = 0; i < 100 * locked.pitch; i++)
        ((char *)locked.data)[i] = (i / locked.pitch) * 255 / 99;
    al_unlock_bitmap(bitmap);
    al_draw_bitmap(bitmap, 0, 0, 0);
    al_flip_display();
    al_rest(5000);
    return 0;
}
END_OF_MAIN()
