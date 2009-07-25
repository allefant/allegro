/* Image conversion example */

#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/a5_iio.h"


int main(int argc, char **argv)
{
   ALLEGRO_BITMAP *bitmap;

   if (argc < 3) {
      fprintf(stderr, "Usage: exnew_convert <infile> <outfile>\n");
      fprintf(stderr, "\tPossible file types: BMP PCX PNG TGA\n");
      return 1;
   }

   if (!al_init()) {
      fprintf(stderr, "Could not init Allegro.\n");
      return 1;
   }

   al_init_iio_addon();

   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ARGB_8888);
   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);

   bitmap = al_load_bitmap(argv[1]);
   if (!bitmap) {
      fprintf(stderr, "Error loading input file\n");
      return 1;
   }

   if (!al_save_bitmap(argv[2], bitmap)) {
      fprintf(stderr, "Error saving bitmap\n");
      return 1;
   }

   al_destroy_bitmap(bitmap);

   return 0;
}
END_OF_MAIN()

