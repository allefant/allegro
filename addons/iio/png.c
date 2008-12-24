/* loadpng, Allegro wrapper routines for libpng
 * by Peter Wang (tjaden@users.sf.net).
 */


#include <png.h>

#include "allegro5/allegro5.h"
#include "allegro5/fshook.h"

#include "iio.h"


double _png_screen_gamma = -1.0;
int _png_compression_level = Z_BEST_COMPRESSION;



/* get_gamma:
 *  Get screen gamma value one of three ways.
 */
static double get_gamma(void)
{
   if (_png_screen_gamma == -1.0) {
      /* Use the environment variable if available.
       * 2.2 is a good guess for PC monitors.
       * 1.1 is good for my laptop.
       */
      AL_CONST char *gamma_str = getenv("SCREEN_GAMMA");
      return (gamma_str) ? atof(gamma_str) : 2.2;
   }

   return _png_screen_gamma;
}



/*****************************************************************************
 * Loading routines
 ****************************************************************************/



/* read_data:
 *  Custom read function to use Allegro packfile routines,
 *  rather than C streams (so we can read from datafiles!)
 */
static void read_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
   ALLEGRO_FS_ENTRY *f = (ALLEGRO_FS_ENTRY *)png_get_io_ptr(png_ptr);
   if ((png_uint_32) al_fs_entry_read(data, length, f) != length)
      png_error(png_ptr, "read error (loadpng calling al_fs_entry_read)");
}



/* check_if_png:
 *  Check if input file is really PNG format.
 */
#define PNG_BYTES_TO_CHECK 4

static int check_if_png(ALLEGRO_FS_ENTRY *fp)
{
   unsigned char buf[PNG_BYTES_TO_CHECK];

   ASSERT(fp);

   if (al_fs_entry_read(buf, PNG_BYTES_TO_CHECK, fp) != PNG_BYTES_TO_CHECK)
      return 0;

   return (png_sig_cmp(buf, (png_size_t) 0, PNG_BYTES_TO_CHECK) == 0);
}



/* really_load_png:
 *  Worker routine, used by load_png and load_memory_png.
 */
static ALLEGRO_BITMAP *really_load_png(png_structp png_ptr, png_infop info_ptr)
{
   ALLEGRO_BITMAP *bmp;
   png_uint_32 width, height, rowbytes;
   int bit_depth, color_type, interlace_type;
   double image_gamma, screen_gamma;
   int intent;
   int bpp;
   int tRNS_to_alpha = FALSE;
   int number_passes, pass;
   PalEntry pal[256];
   ALLEGRO_LOCKED_REGION lock;
   ALLEGRO_STATE backup;
   unsigned char *buf;

   ASSERT(png_ptr && info_ptr);

   /* The call to png_read_info() gives us all of the information from the
    * PNG file before the first IDAT (image data chunk).
    */
   png_read_info(png_ptr, info_ptr);

   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                &color_type, &interlace_type, NULL, NULL);

   /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
    * byte into separate bytes (useful for paletted and grayscale images).
    */
   png_set_packing(png_ptr);

   /* Expand grayscale images to the full 8 bits from 1, 2, or 4 bits/pixel */
   if ((color_type == PNG_COLOR_TYPE_GRAY) && (bit_depth < 8))
      png_set_expand(png_ptr);

   /* Adds a full alpha channel if there is transparency information
    * in a tRNS chunk. */
   if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
      png_set_tRNS_to_alpha(png_ptr);
      tRNS_to_alpha = TRUE;
   }

   /* Convert 16-bits per colour component to 8-bits per colour component. */
   if (bit_depth == 16)
      png_set_strip_16(png_ptr);

   /* Convert grayscale to RGB triplets */
   if ((color_type == PNG_COLOR_TYPE_GRAY) ||
       (color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
      png_set_gray_to_rgb(png_ptr);

   /* Optionally, tell libpng to handle the gamma correction for us. */
   if (_png_screen_gamma != 0.0) {
      screen_gamma = get_gamma();

      if (png_get_sRGB(png_ptr, info_ptr, &intent))
         png_set_gamma(png_ptr, screen_gamma, 0.45455);
      else {
         if (png_get_gAMA(png_ptr, info_ptr, &image_gamma))
            png_set_gamma(png_ptr, screen_gamma, image_gamma);
         else
            png_set_gamma(png_ptr, screen_gamma, 0.45455);
      }
   }

   /* Turn on interlace handling. */
   number_passes = png_set_interlace_handling(png_ptr);

   /* Call to gamma correct and add the background to the palette
    * and update info structure.
    */
   png_read_update_info(png_ptr, info_ptr);

   /* Palettes. */
   if (color_type & PNG_COLOR_MASK_PALETTE) {
      int num_palette, i;
      png_colorp palette;

      if (png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette)) {
         /* We don't actually dither, we just copy the palette. */
         for (i = 0; ((i < num_palette) && (i < 256)); i++) {
            pal[i].r = palette[i].red;  /* 256 -> 64 */
            pal[i].g = palette[i].green;
            pal[i].b = palette[i].blue;
         }

         for (; i < 256; i++)
            pal[i].r = pal[i].g = pal[i].b = 0;
      }
   }

   rowbytes = png_get_rowbytes(png_ptr, info_ptr);

   /* Allocate the memory to hold the image using the fields of info_ptr. */
   bpp = rowbytes * 8 / width;

   /* Allegro cannot handle less than 8 bpp. */
   if (bpp < 8)
      bpp = 8;


   /* Maybe flip RGB to BGR. (FIXME: removed) */
   if ((bpp == 24) || (bpp == 32)) {
#ifdef ALLEGRO_BIG_ENDIAN
      png_set_swap_alpha(png_ptr);
#endif
   }

   bmp = al_create_bitmap(width, height);

   buf = malloc(((bpp + 7) / 8) * width);

   al_lock_bitmap(bmp, &lock, ALLEGRO_LOCK_WRITEONLY);
   al_store_state(&backup, ALLEGRO_STATE_TARGET_BITMAP);
   al_set_target_bitmap(bmp);

   /* Read the image, one line at a line (easier to debug!) */
   for (pass = 0; pass < number_passes; pass++) {
      png_uint_32 y;
      unsigned int i;
      unsigned char *ptr;
      for (y = 0; y < height; y++) {
         png_read_row(png_ptr, buf, NULL);
         ptr = buf;
         if (bpp == 8 && (color_type & PNG_COLOR_MASK_PALETTE)) {
            for (i = 0; i < width; i++) {
               int pix = ptr[0];
               ALLEGRO_COLOR c;
               ptr++;
               c = al_map_rgb(pal[pix].r, pal[pix].g, pal[pix].b);
               al_put_pixel(i, y, c);
            }
         }
         else if (bpp == 8) {
            for (i = 0; i < width; i++) {
               int pix = ptr[0];
               ALLEGRO_COLOR c;
               ptr++;
               c = al_map_rgb(pix, pix, pix);
               al_put_pixel(i, y, c);
            }
         }
         else if (bpp == 24) {
            for (i = 0; i < width; i++) {
               uint32_t pix = READ3BYTES(ptr);
               ALLEGRO_COLOR c;
               ptr += 3;
               c = al_map_rgb(pix & 0xff,
                              (pix >> 8) & 0xff, (pix >> 16) & 0xff);
               al_put_pixel(i, y, c);
            }
         }
         else {
            for (i = 0; i < width; i++) {
               uint32_t pix = bmp_read32(ptr);
               ALLEGRO_COLOR c;
               ptr += 4;
               c = al_map_rgba(pix & 0xff,
                               (pix >> 8) & 0xff,
                               (pix >> 16) & 0xff, (pix >> 24) & 0xff);
               al_put_pixel(i, y, c);
            }
         }
      }
   }

   al_unlock_bitmap(bmp);
   al_restore_state(&backup);

   free(buf);

   /* Read rest of file, and get additional chunks in info_ptr. */
   png_read_end(png_ptr, info_ptr);

   return bmp;
}


/* load_png_pf:
 *  Load a PNG file from disk, doing colour coversion if required.
 */
static ALLEGRO_BITMAP *load_png_pf(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_BITMAP *bmp;
   png_structp png_ptr;
   png_infop info_ptr;

   ASSERT(fp);

   if (!check_if_png(fp)) {
      return NULL;
   }

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also supply the
    * the compiler header file version, so that we know if the application
    * was compiled with a compatible version of the library.
    */
   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                    (void *)NULL, NULL, NULL);
   if (!png_ptr) {
      return NULL;
   }

   /* Allocate/initialize the memory for image information. */
   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr) {
      png_destroy_read_struct(&png_ptr, (png_infopp) NULL, (png_infopp) NULL);
      return NULL;
   }

   /* Set error handling if you are using the setjmp/longjmp method (this is
    * the normal method of doing things with libpng).  REQUIRED unless you
    * set up your own error handlers in the png_create_read_struct() earlier.
    */
   if (setjmp(png_ptr->jmpbuf)) {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
      /* If we get here, we had a problem reading the file */
      return NULL;
   }

   /* Use Allegro packfile routines. */
   png_set_read_fn(png_ptr, fp, (png_rw_ptr) read_data);

   /* We have already read some of the signature. */
   png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

   /* Really load the image now. */
   bmp = really_load_png(png_ptr, info_ptr);

   /* Clean up after the read, and free any memory allocated. */
   png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);

   return bmp;
}





/* Function: iio_load_png
 * Create a new ALLEGRO_BITMAP from a PNG file. The bitmap is created with
 * <al_create_bitmap>.
 * See Also: <al_iio_load>.
 */
ALLEGRO_BITMAP *iio_load_png(AL_CONST char *filename)
{
   ALLEGRO_FS_ENTRY *fp;
   ALLEGRO_BITMAP *bmp;

   ASSERT(filename);

   fp = al_fs_entry_open(filename, "rb");
   if (!fp)
      return NULL;

   bmp = load_png_pf(fp);

   al_fs_entry_close(fp);

   return bmp;
}




/*****************************************************************************
 * Saving routines
 ****************************************************************************/



/* write_data:
 *  Custom write function to use Allegro packfile routines,
 *  rather than C streams.
 */
static void write_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
   ALLEGRO_FS_ENTRY *f = (ALLEGRO_FS_ENTRY *)png_get_io_ptr(png_ptr);
   if ((png_uint_32) al_fs_entry_write(data, length, f) != length)
      png_error(png_ptr, "write error (loadpng calling al_fs_entry_write)");
}

/* Don't think Allegro has any problem with buffering
 * (rather, Allegro provides no way to flush packfiles).
 */
static void flush_data(png_structp png_ptr)
{
   (void)png_ptr;
}


/* save_rgba:
 *  Core save routine for 32 bpp images.
 */
static int save_rgba(png_structp png_ptr, ALLEGRO_BITMAP *bmp)
{
   const int bmp_w = al_get_bitmap_width(bmp);
   const int bmp_h = al_get_bitmap_height(bmp);
   unsigned char *rowdata;
   int x, y;

   rowdata = (unsigned char *)malloc(bmp_w * 4);
   if (!rowdata)
      return 0;

   for (y = 0; y < bmp_h; y++) {
      unsigned char *p = rowdata;

      for (x = 0; x < bmp_w; x++) {
         ALLEGRO_COLOR c = al_get_pixel(bmp, x, y);
         unsigned char r, g, b, a;
         al_unmap_rgba(c, &r, &g, &b, &a);
         *p++ = r;
         *p++ = g;
         *p++ = b;
         *p++ = a;
      }

      png_write_row(png_ptr, rowdata);
   }

   free(rowdata);

   return 1;
}



/* save_png:
 *  Writes a non-interlaced, no-frills PNG, taking the usual save_xyz
 *  parameters.  Returns non-zero on error.
 */
static int really_save_png(ALLEGRO_FS_ENTRY *fp, ALLEGRO_BITMAP *bmp)
{
   png_structp png_ptr = NULL;
   png_infop info_ptr = NULL;
   int depth;
   int colour_type;

   depth = 32;

   /* Create and initialize the png_struct with the
    * desired error handler functions.
    */
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                     (void *)NULL, NULL, NULL);
   if (!png_ptr)
      goto Error;

   /* Allocate/initialize the image information data. */
   info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr)
      goto Error;

   /* Set error handling. */
   if (setjmp(png_ptr->jmpbuf)) {
      /* If we get here, we had a problem reading the file. */
      goto Error;
   }

   /* Use packfile routines. */
   png_set_write_fn(png_ptr, fp, (png_rw_ptr) write_data, flush_data);

   /* Set the image information here.  Width and height are up to 2^31,
    * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
    * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
    * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
    * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
    * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
    * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE.
    */
   colour_type = PNG_COLOR_TYPE_RGB_ALPHA;

   /* Set compression level. */
   png_set_compression_level(png_ptr, _png_compression_level);

   png_set_IHDR(png_ptr, info_ptr,
                al_get_bitmap_width(bmp), al_get_bitmap_height(bmp),
                8, colour_type,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
                PNG_FILTER_TYPE_BASE);

   /* Optionally write comments into the image ... Nah. */

   /* Write the file header information. */
   png_write_info(png_ptr, info_ptr);

   /* Once we write out the header, the compression type on the text
    * chunks gets changed to PNG_TEXT_COMPRESSION_NONE_WR or
    * PNG_TEXT_COMPRESSION_zTXt_WR, so it doesn't get written out again
    * at the end.
    */
   if (!save_rgba(png_ptr, bmp))
      goto Error;

   png_write_end(png_ptr, info_ptr);

   png_destroy_write_struct(&png_ptr, &info_ptr);

   return 0;

 Error:

   if (png_ptr) {
      if (info_ptr)
         png_destroy_write_struct(&png_ptr, &info_ptr);
      else
         png_destroy_write_struct(&png_ptr, NULL);
   }

   return -1;
}


/* Function: iio_save_png
 * Save an ALLEGRO_BITMAP as a PNG file. 
 * See Also: <al_iio_save>.
 */
int iio_save_png(AL_CONST char *filename, ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_FS_ENTRY *fp;
   int result;

   ASSERT(filename);
   ASSERT(bmp);

   fp = al_fs_entry_open(filename, "wb");
   if (!fp) {
      TRACE("Unable to open file %s for writing\n", filename);
      return -1;
   }

   result = really_save_png(fp, bmp);
   al_fs_entry_close(fp);

   return result;
}

/* vim: set sts=3 sw=3 et: */
