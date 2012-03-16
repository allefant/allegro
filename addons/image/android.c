#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_android.h"

ALLEGRO_BITMAP *_al_load_android_bitmap_f(ALLEGRO_FILE *fp, int flags)
{
   return _al_android_load_image_f(fp, flags);   
}

ALLEGRO_BITMAP *_al_load_android_bitmap(const char *filename, int flags)
{
   ALLEGRO_BITMAP *bmp = NULL;

   // NOTE: reads from assets. asset handling must be done better.
   return _al_android_load_image(filename, flags);
   
   return bmp;
}

