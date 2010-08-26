#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>

#define MAX_BITMAPS  8
#define MAX_TRANS    8
#define MAX_FONTS    8

typedef struct {
   ALLEGRO_USTR   *name;
   ALLEGRO_BITMAP *bitmap[2];
} Bitmap;

typedef enum {
   SW = 0,
   HW = 1
} BmpType;

typedef struct {
   ALLEGRO_USTR   *name;
   ALLEGRO_TRANSFORM transform;
} Transform;

typedef struct {
   int            x;
   int            y;
   int            w;
   int            h;
   ALLEGRO_LOCKED_REGION *lr;
} LockRegion;

typedef struct {
   ALLEGRO_USTR   *name;
   ALLEGRO_FONT   *font;
} Font;

ALLEGRO_DISPLAY   *display;
ALLEGRO_BITMAP    *membuf;
Bitmap            bitmaps[MAX_BITMAPS];
LockRegion        lock_region;
Transform         transforms[MAX_TRANS];
Font              fonts[MAX_FONTS];
int               num_global_bitmaps;
float             delay = 0.0;
bool              save_outputs = false;
bool              quiet = false;
bool              verbose = false;
bool              no_exit_code = false;
int               total_tests = 0;
int               passed_tests = 0;
int               failed_tests = 0;

#define streq(a, b)  (0 == strcmp((a), (b)))

static void error(char const *msg, ...)
{
   va_list ap;

   va_start(ap, msg);
   fprintf(stderr, "test_driver: ");
   vfprintf(stderr, msg, ap);
   fprintf(stderr, "\n");
   va_end(ap);
   exit(EXIT_FAILURE);
}

static char const *bmp_type_to_string(BmpType bmp_type)
{
   switch (bmp_type) {
      case SW: return "sw";
      case HW: return "hw";
   }
   return "error";
}

static ALLEGRO_BITMAP *load_relative_bitmap(char const *filename)
{
   ALLEGRO_BITMAP *bmp;

   bmp = al_load_bitmap(filename);
   if (!bmp) {
      error("failed to load %s", filename);
   }
   return bmp;
}

static void load_bitmaps(ALLEGRO_CONFIG const *cfg, const char *section,
   BmpType bmp_type)
{
   int i = 0;
   void *iter;
   char const *key;
   char const *value;

   key = al_get_first_config_entry(cfg, section, &iter);
   while (key && i < MAX_BITMAPS) {
      value = al_get_config_value(cfg, section, key);

      bitmaps[i].name = al_ustr_new(key);
      bitmaps[i].bitmap[bmp_type] = load_relative_bitmap(value);

      key = al_get_next_config_entry(&iter);
      i++;
   }

   if (i == MAX_BITMAPS)
      error("bitmap limit reached");

   num_global_bitmaps = i;
}

static ALLEGRO_BITMAP **reserve_local_bitmap(const char *name, BmpType bmp_type)
{
   int i;

   for (i = num_global_bitmaps; i < MAX_BITMAPS; i++) {
      if (!bitmaps[i].name) {
         bitmaps[i].name = al_ustr_new(name);
         return &bitmaps[i].bitmap[bmp_type];
      }
   }

   error("bitmap limit reached");
   return NULL;
}

static void load_fonts(ALLEGRO_CONFIG const *cfg, const char *section)
{
   int i = 0;
   void *iter;
   char const *key;
   char const *value;

   key = al_get_first_config_entry(cfg, section, &iter);
   while (key && i < MAX_FONTS) {
      value = al_get_config_value(cfg, section, key);

      fonts[i].name = al_ustr_new(key);
      fonts[i].font = al_load_font(value, 24, 0);
      if (!fonts[i].font)
         error("failed to load font: %s", value);

      key = al_get_next_config_entry(&iter);
      i++;
   }

   if (i == MAX_FONTS)
      error("font limit reached");
}

static void set_target_reset(ALLEGRO_BITMAP *target)
{
   ALLEGRO_TRANSFORM ident;

   al_set_target_bitmap(target);
   al_clear_to_color(al_map_rgb(0, 0, 0));
   al_set_clipping_rectangle(0, 0,
      al_get_bitmap_width(target),
      al_get_bitmap_height(target));
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);
   al_identity_transform(&ident);
   al_use_transform(&ident);
}

static char const *resolve_var(ALLEGRO_CONFIG const *cfg, char const *section,
   char const *v)
{
   char const *vv = al_get_config_value(cfg, section, v);
   return (vv) ? vv : v;
}

static bool get_bool(char const *value)
{
   return streq(value, "true") ? true
      : streq(value, "false") ? false
      : atoi(value);
}

static ALLEGRO_COLOR get_color(char const *value)
{
   int r, g, b, a;

   if (sscanf(value, "#%02x%02x%02x%02x", &r, &g, &b, &a) == 4)
      return al_map_rgba(r, g, b, a);
   if (sscanf(value, "#%02x%02x%02x", &r, &g, &b) == 3)
      return al_map_rgb(r, g, b);
   return al_color_name(value);
}

static ALLEGRO_BITMAP *get_bitmap(char const *value, BmpType bmp_type,
   ALLEGRO_BITMAP *target)
{
   int i;

   for (i = 0; i < MAX_BITMAPS; i++) {
      if (bitmaps[i].name && streq(al_cstr(bitmaps[i].name), value))
         return bitmaps[i].bitmap[bmp_type];
   }

   if (streq(value, "target"))
      return target;

   error("undefined bitmap: %s", value);
   return NULL;
}

static int get_draw_bitmap_flag(char const *value)
{
   if (streq(value, "ALLEGRO_FLIP_HORIZONTAL"))
      return ALLEGRO_FLIP_HORIZONTAL;
   if (streq(value, "ALLEGRO_FLIP_VERTICAL"))
      return ALLEGRO_FLIP_VERTICAL;
   if (streq(value, "ALLEGRO_FLIP_VERTICAL|ALLEGRO_FLIP_HORIZONTAL"))
      return ALLEGRO_FLIP_VERTICAL|ALLEGRO_FLIP_HORIZONTAL;
   if (streq(value, "ALLEGRO_FLIP_HORIZONTAL|ALLEGRO_FLIP_VERTICAL"))
      return ALLEGRO_FLIP_HORIZONTAL|ALLEGRO_FLIP_VERTICAL;
   return atoi(value);
}

static int get_blender_op(char const *value)
{
   return streq(value, "ALLEGRO_ADD") ? ALLEGRO_ADD
      : streq(value, "ALLEGRO_DEST_MINUS_SRC") ? ALLEGRO_DEST_MINUS_SRC
      : streq(value, "ALLEGRO_SRC_MINUS_DEST") ? ALLEGRO_SRC_MINUS_DEST
      : atoi(value);
}

static int get_blend_factor(char const *value)
{
   return streq(value, "ALLEGRO_ZERO") ? ALLEGRO_ZERO
      : streq(value, "ALLEGRO_ONE") ? ALLEGRO_ONE
      : streq(value, "ALLEGRO_ALPHA") ? ALLEGRO_ALPHA
      : streq(value, "ALLEGRO_INVERSE_ALPHA") ? ALLEGRO_INVERSE_ALPHA
      : atoi(value);
}

static ALLEGRO_TRANSFORM *get_transform(const char *name)
{
   int i;

   for (i = 0; i < MAX_TRANS; i++) {
      if (!transforms[i].name) {
         transforms[i].name = al_ustr_new(name);
         al_identity_transform(&transforms[i].transform);
         return &transforms[i].transform;
      }

      if (transforms[i].name && streq(al_cstr(transforms[i].name), name))
         return &transforms[i].transform;
   }

   error("transforms limit reached");
   return NULL;
}

static int get_pixel_format(char const *v)
{
   int format = streq(v, "ALLEGRO_PIXEL_FORMAT_ANY") ? ALLEGRO_PIXEL_FORMAT_ANY
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA") ? ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ARGB_8888") ? ALLEGRO_PIXEL_FORMAT_ARGB_8888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGBA_8888") ? ALLEGRO_PIXEL_FORMAT_RGBA_8888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ARGB_4444") ? ALLEGRO_PIXEL_FORMAT_ARGB_4444
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGB_888") ? ALLEGRO_PIXEL_FORMAT_RGB_888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGB_565") ? ALLEGRO_PIXEL_FORMAT_RGB_565
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGB_555") ? ALLEGRO_PIXEL_FORMAT_RGB_555
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGBA_5551") ? ALLEGRO_PIXEL_FORMAT_RGBA_5551
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ARGB_1555") ? ALLEGRO_PIXEL_FORMAT_ARGB_1555
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ABGR_8888") ? ALLEGRO_PIXEL_FORMAT_ABGR_8888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_XBGR_8888") ? ALLEGRO_PIXEL_FORMAT_XBGR_8888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_BGR_888") ? ALLEGRO_PIXEL_FORMAT_BGR_888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_BGR_565") ? ALLEGRO_PIXEL_FORMAT_BGR_565
      : streq(v, "ALLEGRO_PIXEL_FORMAT_BGR_555") ? ALLEGRO_PIXEL_FORMAT_BGR_555
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGBX_8888") ? ALLEGRO_PIXEL_FORMAT_RGBX_8888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_XRGB_8888") ? ALLEGRO_PIXEL_FORMAT_XRGB_8888
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ABGR_F32") ? ALLEGRO_PIXEL_FORMAT_ABGR_F32
      : streq(v, "ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE") ? ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE
      : streq(v, "ALLEGRO_PIXEL_FORMAT_RGBA_4444") ? ALLEGRO_PIXEL_FORMAT_RGBA_4444
      : -1;
   if (format == -1)
      error("invalid format: %s", v);
   return format;
}

static int get_lock_bitmap_flags(char const *v)
{
   return streq(v, "ALLEGRO_LOCK_READWRITE") ? ALLEGRO_LOCK_READWRITE
      : streq(v, "ALLEGRO_LOCK_READONLY") ? ALLEGRO_LOCK_READONLY
      : streq(v, "ALLEGRO_LOCK_WRITEONLY") ? ALLEGRO_LOCK_WRITEONLY
      : atoi(v);
}

static void fill_lock_region(LockRegion *lr, float alphafactor, bool blended)
{
   int x, y;
   float r, g, b, a;
   ALLEGRO_COLOR c;

   for (y = 0; y < lr->h; y++) {
      for (x = 0; x < lr->w; x++) {
         r = (float)x / lr->w;
         b = (float)y / lr->h;
         g = r*b;
         a = r * alphafactor;
         c = al_map_rgba_f(r, g, b, a);
         if (blended)
            al_put_blended_pixel(lr->x + x, lr->y + y, c);
         else
            al_put_pixel(lr->x + x, lr->y + y, c);
      }
   }
}

static ALLEGRO_FONT *get_font(char const *name)
{
   int i;

   for (i = 0; i < MAX_FONTS; i++) {
      if (fonts[i].name && streq(al_cstr(fonts[i].name), name))
         return fonts[i].font;
   }

   error("undefined font: %s", name);
   return NULL;
}

static int get_font_align(char const *value)
{
   return streq(value, "ALLEGRO_ALIGN_LEFT") ? ALLEGRO_ALIGN_LEFT
      : streq(value, "ALLEGRO_ALIGN_CENTRE") ? ALLEGRO_ALIGN_CENTRE
      : streq(value, "ALLEGRO_ALIGN_RIGHT") ? ALLEGRO_ALIGN_RIGHT
      : atoi(value);
}

/* FNV-1a algorithm, parameters from:
 * http://www.isthe.com/chongo/tech/comp/fnv/index.html
 */
#define FNV_OFFSET_BASIS   2166136261UL
#define FNV_PRIME          16777619

static uint32_t hash_bitmap(ALLEGRO_BITMAP *bmp)
{
   ALLEGRO_LOCKED_REGION *lr;
   int x, y, w, h;
   uint32_t hash;

   w = al_get_bitmap_width(bmp);
   h = al_get_bitmap_height(bmp);
   hash = FNV_OFFSET_BASIS;

   lr = al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE,
      ALLEGRO_LOCK_READONLY);

   for (y = 0; y < h; y++) {
      /* Oops, I unintentially committed the first version of this with signed
       * chars and computing in BGRA order, so leave it like that so we don't
       * have to update a bunch of old hashes.
       */
      signed char const *data = ((signed char const *)lr->data) + y*lr->pitch;
      for (x = 0; x < w; x++) {
         hash ^= data[x*4 + 3]; hash *= FNV_PRIME;
         hash ^= data[x*4 + 2]; hash *= FNV_PRIME;
         hash ^= data[x*4 + 1]; hash *= FNV_PRIME;
         hash ^= data[x*4 + 0]; hash *= FNV_PRIME;
      }
   }

   al_unlock_bitmap(bmp);

   return hash;
}

/* Image "signature" I just made up:
 * We take the average intensity of 7x7 patches centred at 9x9 grid points on
 * the image.  Each of these values is reduced down to 6 bits so it can be
 * represented by one printable character in base64 encoding.  This gives a
 * reasonable signature length.
 */

#define SIG_GRID  9
#define SIG_LEN   (SIG_GRID * SIG_GRID)
#define SIG_LENZ  (SIG_LEN + 1)

static int patch_intensity(ALLEGRO_BITMAP *bmp, int cx, int cy)
{
   float sum = 0.0;
   int x, y;

   for (y = -3; y <= 3; y++) {
      for (x = -3; x <= 3; x++) {
         ALLEGRO_COLOR c = al_get_pixel(bmp, cx + x, cy + y);
         sum += c.r + c.g + c.b;
      }
   }

   return 255 * sum/(7*7*3);
}

static char const base64[64] =
   "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+/";

static int base64_decode(char c)
{
   if (c >= '0' && c <= '9') return c - '0';
   if (c >= 'A' && c <= 'Z') return 10 + (c - 'A');
   if (c >= 'a' && c <= 'z') return 36 + (c - 'a');
   if (c == '+') return 62;
   if (c == '/') return 63;
   error("invalid base64 character: %c", c);
   return -1;
}

static void compute_signature(ALLEGRO_BITMAP *bmp, char sig[SIG_LENZ])
{
   int w = al_get_bitmap_width(bmp);
   int h = al_get_bitmap_height(bmp);
   int x, y;
   int n = 0;

   al_lock_bitmap(bmp, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READONLY);

   for (y = 0; y < SIG_GRID; y++) {
      for (x = 0; x < SIG_GRID; x++) {
         int cx = (1 + x) * w/(1 + SIG_GRID);
         int cy = (1 + y) * h/(1 + SIG_GRID);
         int level = patch_intensity(bmp, cx, cy);
         sig[n] = base64[level >> 2];
         n++;
      }
   }

   sig[n] = '\0';

   al_unlock_bitmap(bmp);
}

static bool similar_signatures(char const sig1[SIG_LEN], char const sig2[SIG_LEN])
{
   int correct = 0;
   int i;

   for (i = 0; i < SIG_LEN; i++) {
      int q1 = base64_decode(sig1[i]);
      int q2 = base64_decode(sig2[i]);

      /* A difference of one quantisation level could be because two values
       * which were originally close by straddled a quantisation boundary.
       * A difference of two quantisation levels is a significant deviation.
       */
      if (abs(q1 - q2) > 1)
         return false;

      correct++;
   }

   return ((float)correct / SIG_LEN) > 0.95;
}

static void check_hash(ALLEGRO_CONFIG const *cfg, char const *testname,
   ALLEGRO_BITMAP *bmp, BmpType bmp_type)
{
   char const *bt = bmp_type_to_string(bmp_type);
   char hash[16];
   char sig[SIG_LENZ];
   char const *exp;
   char const *sigexp;

   exp = al_get_config_value(cfg, testname, "hash");
   sigexp = al_get_config_value(cfg, testname, "sig");

   if (exp && streq(exp, "off")) {
      printf("OK   %s [%s] - hash check off\n", testname, bt);
      passed_tests++;
      return;
   }

   sprintf(hash, "%08x", hash_bitmap(bmp));
   compute_signature(bmp, sig);

   if (verbose) {
      printf("hash=%s\n", hash);
      printf("sig=%s\n", sig);
   }

   if (!exp && !sigexp) {
      printf("NEW  %s [%s] - hash=%s; sig=%s\n",
         testname, bt, hash, sig);
      return;
   }

   if (exp && streq(hash, exp)) {
      printf("OK   %s [%s]\n", testname, bt);
      passed_tests++;
      return;
   }

   if (sigexp && strlen(sigexp) != SIG_LEN) {
      printf("WARNING: ignoring bad signature: %s\n", sigexp);
      sigexp = NULL;
   }

   if (sigexp && similar_signatures(sig, sigexp)) {
      printf("OK   %s [%s] - by signature\n", testname, bt);
      passed_tests++;
      return;
   }

   printf("FAIL %s [%s] - hash=%s\n", testname, bt, hash);
   failed_tests++;
}

static double bitmap_dissimilarity(ALLEGRO_BITMAP *bmp1, ALLEGRO_BITMAP *bmp2)
{
   ALLEGRO_LOCKED_REGION *lr1;
   ALLEGRO_LOCKED_REGION *lr2;
   int x, y, w, h;
   double sqerr = 0.0;

   lr1 = al_lock_bitmap(bmp1, ALLEGRO_PIXEL_FORMAT_RGBA_8888,
      ALLEGRO_LOCK_READONLY);
   lr2 = al_lock_bitmap(bmp2, ALLEGRO_PIXEL_FORMAT_RGBA_8888,
      ALLEGRO_LOCK_READONLY);

   w = al_get_bitmap_width(bmp1);
   h = al_get_bitmap_height(bmp1);

   for (y = 0; y < h; y++) {
      char const *data1 = ((char const *)lr1->data) + y*lr1->pitch;
      char const *data2 = ((char const *)lr2->data) + y*lr2->pitch;

      for (x = 0; x < w*4; x++) {
         double err = (double)data1[x] - (double)data2[x];
         sqerr += err*err;
      }
   }

   al_unlock_bitmap(bmp1);
   al_unlock_bitmap(bmp2);

   return sqrt(sqerr / (w*h*4.0));
}

static void check_similarity(char const *testname,
   ALLEGRO_BITMAP *bmp1, ALLEGRO_BITMAP *bmp2, BmpType bmp_type, bool reliable)
{
   char const *bt = bmp_type_to_string(bmp_type);
   double rms = bitmap_dissimilarity(bmp1, bmp2);

   /* This cutoff is "empirically determined" only. */
   if (rms <= 17.5) {
      if (reliable)
         printf("OK   %s [%s]\n", testname, bt);
      else
         printf("OK?  %s [%s]\n", testname, bt);
      passed_tests++;
   }
   else {
      printf("FAIL %s [%s] - RMS error is %g\n", testname, bt, rms);
      failed_tests++;
   }
}

static void do_test(ALLEGRO_CONFIG const *cfg, char const *testname,
   ALLEGRO_BITMAP *target, int bmp_type, bool reliable)
{
#define MAXBUF    80
#define PAT       " %80[-A-Za-z0-9_.$|#] "
#define PAT1      PAT
#define PAT2      PAT1 "," PAT1
#define PAT3      PAT2 "," PAT1
#define PAT4      PAT3 "," PAT1
#define PAT5      PAT4 "," PAT1
#define PAT6      PAT5 "," PAT1
#define PAT7      PAT6 "," PAT1
#define PAT8      PAT7 "," PAT1
#define PAT9      PAT8 "," PAT1
#define PAT10     PAT9 "," PAT1
#define PAT11     PAT10 "," PAT1
#define ARGS1     arg[0]
#define ARGS2     ARGS1, arg[1]
#define ARGS3     ARGS2, arg[2]
#define ARGS4     ARGS3, arg[3]
#define ARGS5     ARGS4, arg[4]
#define ARGS6     ARGS5, arg[5]
#define ARGS7     ARGS6, arg[6]
#define ARGS8     ARGS7, arg[7]
#define ARGS9     ARGS8, arg[8]
#define ARGS10    ARGS9, arg[9]
#define ARGS11    ARGS10, arg[10]
#define V(a)      resolve_var(cfg, testname, arg[(a)])
#define I(a)      atoi(V(a))
#define F(a)      atof(V(a))
#define C(a)      get_color(V(a))
#define B(a)      get_bitmap(V(a), bmp_type, target)
#define SCAN(fn, arity) \
      (sscanf(stmt, fn " (" PAT##arity " )", ARGS##arity) == arity)
#define SCANLVAL(fn, arity) \
      (sscanf(stmt, PAT " = " fn " (" PAT##arity " )", lval, ARGS##arity) \
         == 1 + arity)

   int op;
   char const *stmt;
   char buf[MAXBUF];
   char arg[11][MAXBUF];
   char lval[MAXBUF];
   int i;

   if (verbose) {
      /* So in case it segfaults, we know which test to re-run. */
      printf("\nRunning %s [%s].\n", testname, bmp_type_to_string(bmp_type));
      fflush(stdout);
   }

   set_target_reset(target);

   for (op = 0; ; op++) {
      sprintf(buf, "op%d", op);
      stmt = al_get_config_value(cfg, testname, buf);
      if (!stmt) {
         /* Check for a common mistake. */
         sprintf(buf, "op%d", op+1);
         stmt = al_get_config_value(cfg, testname, buf);
         if (!stmt)
            break;
         printf("WARNING: op%d skipped, continuing at op%d\n", op, op+1);
         op++;
      }

      if (verbose)
         printf("# %s\n", stmt);

      if (streq(stmt, ""))
         continue;

      if (SCAN("al_set_target_bitmap", 1)) {
         al_set_target_bitmap(B(0));
         continue;
      }

      if (SCAN("al_set_clipping_rectangle", 4)) {
         al_set_clipping_rectangle(I(0), I(1), I(2), I(3));
         continue;
      }

      if (SCAN("al_set_blender", 3)) {
         al_set_blender(
            get_blender_op(V(0)),
            get_blend_factor(V(1)),
            get_blend_factor(V(2)));
         continue;
      }

      if (SCAN("al_set_separate_blender", 6)) {
         al_set_separate_blender(
            get_blender_op(V(0)),
            get_blend_factor(V(1)),
            get_blend_factor(V(2)),
            get_blender_op(V(3)),
            get_blend_factor(V(4)),
            get_blend_factor(V(5)));
         continue;
      }

      if (SCAN("al_clear_to_color", 1)) {
         al_clear_to_color(C(0));
         continue;
      }

      if (SCAN("al_draw_bitmap", 4)) {
         al_draw_bitmap(B(0), F(1), F(2),
            get_draw_bitmap_flag(V(3)));
         continue;
      }

      if (SCAN("al_draw_tinted_bitmap", 5)) {
         al_draw_tinted_bitmap(B(0), C(1), F(2), F(3),
            get_draw_bitmap_flag(V(4)));
         continue;
      }

      if (SCAN("al_draw_bitmap_region", 8)) {
         al_draw_bitmap_region(B(0),
            F(1), F(2), F(3), F(4), F(5), F(6),
            get_draw_bitmap_flag(V(7)));
         continue;
      }

      if (SCAN("al_draw_tinted_bitmap_region", 9)) {
         al_draw_tinted_bitmap_region(B(0), C(1),
            F(2), F(3), F(4), F(5), F(6), F(7),
            get_draw_bitmap_flag(V(8)));
         continue;
      }

      if (SCAN("al_draw_rotated_bitmap", 7)) {
         al_draw_rotated_bitmap(B(0), F(1), F(2), F(3), F(4), F(5),
            get_draw_bitmap_flag(V(6)));
         continue;
      }

      if (SCAN("al_draw_tinted_rotated_bitmap", 8)) {
         al_draw_tinted_rotated_bitmap(B(0), C(1),
            F(2), F(3), F(4), F(5), F(6),
            get_draw_bitmap_flag(V(7)));
         continue;
      }

      if (SCAN("al_draw_scaled_bitmap", 10)) {
         al_draw_scaled_bitmap(B(0),
            F(1), F(2), F(3), F(4), F(5), F(6), F(7), F(8),
            get_draw_bitmap_flag(V(9)));
         continue;
      }

      if (SCAN("al_draw_tinted_scaled_bitmap", 11)) {
         al_draw_tinted_scaled_bitmap(B(0), C(1),
            F(2), F(3), F(4), F(5), F(6), F(7), F(8), F(9),
            get_draw_bitmap_flag(V(10)));
         continue;
      }

      if (SCAN("al_draw_scaled_rotated_bitmap", 9)) {
         al_draw_scaled_rotated_bitmap(B(0),
            F(1), F(2), F(3), F(4), F(5), F(6), F(7),
            get_draw_bitmap_flag(V(8)));
         continue;
      }

      if (SCAN("al_draw_tinted_scaled_rotated_bitmap", 10)) {
         al_draw_tinted_scaled_rotated_bitmap(B(0), C(1),
            F(2), F(3), F(4), F(5), F(6), F(7), F(8),
            get_draw_bitmap_flag(V(9)));
         continue;
      }

      if (SCAN("al_draw_pixel", 3)) {
         al_draw_pixel(I(0), I(1), C(2));
         continue;
      }

      if (SCAN("al_put_pixel", 3)) {
         al_put_pixel(I(0), I(1), C(2));
         continue;
      }

      if (SCAN("al_put_blended_pixel", 3)) {
         al_put_blended_pixel(I(0), I(1), C(2));
         continue;
      }

      if (SCANLVAL("al_create_bitmap", 2)) {
         ALLEGRO_BITMAP **bmp = reserve_local_bitmap(lval, bmp_type);
         (*bmp) = al_create_bitmap(I(0), I(1));
         continue;
      }

      if (SCANLVAL("al_create_sub_bitmap", 5)) {
         ALLEGRO_BITMAP **bmp = reserve_local_bitmap(lval, bmp_type);
         (*bmp) = al_create_sub_bitmap(B(0), I(1), I(2), I(3), I(4));
         continue;
      }

      if (SCANLVAL("al_load_bitmap", 1)) {
         ALLEGRO_BITMAP **bmp = reserve_local_bitmap(lval, bmp_type);
         (*bmp) = load_relative_bitmap(V(0));
         continue;
      }

      if (SCAN("al_hold_bitmap_drawing", 1)) {
         al_hold_bitmap_drawing(get_bool(V(0)));
         continue;
      }

      /* Transformations */
      if (SCAN("al_copy_transform", 2)) {
         al_copy_transform(get_transform(V(0)), get_transform(V(1)));
         continue;
      }
      if (SCAN("al_use_transform", 1)) {
         al_use_transform(get_transform(V(0)));
         continue;
      }
      if (SCAN("al_build_transform", 6)) {
         al_build_transform(get_transform(V(0)), F(1), F(2), F(3), F(4), F(5));
         continue;
      }
      if (SCAN("al_translate_transform", 3)) {
         al_translate_transform(get_transform(V(0)), F(1), F(2));
         continue;
      }
      if (SCAN("al_scale_transform", 3)) {
         al_scale_transform(get_transform(V(0)), F(1), F(2));
         continue;
      }
      if (SCAN("al_rotate_transform", 2)) {
         al_rotate_transform(get_transform(V(0)), F(1));
         continue;
      }
      if (SCAN("al_compose_transform", 2)) {
         al_compose_transform(get_transform(V(0)), get_transform(V(1)));
         continue;
      }

      /* Locking */
      if (SCAN("al_lock_bitmap", 3)) {
         ALLEGRO_BITMAP *bmp = B(0);
         lock_region.x = 0;
         lock_region.y = 0;
         lock_region.w = al_get_bitmap_width(bmp);
         lock_region.h = al_get_bitmap_height(bmp);
         lock_region.lr = al_lock_bitmap(bmp,
            get_pixel_format(V(1)),
            get_lock_bitmap_flags(V(2)));
         continue;
      }
      if (SCAN("al_lock_bitmap_region", 7)) {
         ALLEGRO_BITMAP *bmp = B(0);
         lock_region.x = I(1);
         lock_region.y = I(2);
         lock_region.w = I(3);
         lock_region.h = I(4);
         lock_region.lr = al_lock_bitmap_region(bmp,
            lock_region.x, lock_region.y,
            lock_region.w, lock_region.h,
            get_pixel_format(V(5)),
            get_lock_bitmap_flags(V(6)));
         continue;
      }
      if (SCAN("al_unlock_bitmap", 1)) {
         al_unlock_bitmap(B(0));
         lock_region.lr = NULL;
         continue;
      }
      if (SCAN("fill_lock_region", 2)) {
         fill_lock_region(&lock_region, F(0), get_bool(V(1)));
         continue;
      }

      /* Fonts */
      if (SCAN("al_draw_text", 6)) {
         al_draw_text(get_font(V(0)), C(1), F(2), F(3), get_font_align(V(4)),
            V(5));
         continue;
      }

      if (SCAN("al_draw_justified_text", 8)) {
         al_draw_justified_text(get_font(V(0)), C(1), F(2), F(3), F(4), F(5),
            get_font_align(V(6)), V(7));
         continue;
      }

      /* Primitives */
      if (SCAN("al_draw_line", 6)) {
         al_draw_line(F(0), F(1), F(2), F(3), C(4), F(5));
         continue;
      }
      if (SCAN("al_draw_triangle", 8)) {
         al_draw_triangle(F(0), F(1), F(2), F(3), F(4), F(5),
            C(6), F(7));
         continue;
      }
      if (SCAN("al_draw_filled_triangle", 7)) {
         al_draw_filled_triangle(F(0), F(1), F(2), F(3), F(4), F(5), C(6));
         continue;
      }
      if (SCAN("al_draw_rectangle", 6)) {
         al_draw_rectangle(F(0), F(1), F(2), F(3), C(4), F(5));
         continue;
      }
      if (SCAN("al_draw_filled_rectangle", 5)) {
         al_draw_filled_rectangle(F(0), F(1), F(2), F(3), C(4));
         continue;
      }
      if (SCAN("al_draw_rounded_rectangle", 8)) {
         al_draw_rounded_rectangle(F(0), F(1), F(2), F(3), F(4), F(5), C(6),
            F(7));
         continue;
      }
      if (SCAN("al_draw_filled_rounded_rectangle", 7)) {
         al_draw_filled_rounded_rectangle(F(0), F(1), F(2), F(3), F(4), F(5),
            C(6));
         continue;
      }
      if (SCAN("al_draw_ellipse", 6)) {
         al_draw_ellipse(F(0), F(1), F(2), F(3), C(4), F(5));
         continue;
      }
      if (SCAN("al_draw_filled_ellipse", 5)) {
         al_draw_filled_ellipse(F(0), F(1), F(2), F(3), C(4));
         continue;
      }
      if (SCAN("al_draw_circle", 5)) {
         al_draw_circle(F(0), F(1), F(2), C(3), F(4));
         continue;
      }
      if (SCAN("al_draw_filled_circle", 4)) {
         al_draw_filled_circle(F(0), F(1), F(2), C(3));
         continue;
      }
      if (SCAN("al_draw_arc", 7)) {
         al_draw_arc(F(0), F(1), F(2), F(3), F(4), C(5), F(6));
         continue;
      }
      if (SCAN("al_draw_spline", 3)) {
         float pt[8];
         if (sscanf(V(0), "%f, %f, %f, %f, %f, %f, %f, %f",
               pt+0, pt+1, pt+2, pt+3, pt+4, pt+5, pt+6, pt+7) == 8) {
            al_draw_spline(pt, C(1), F(2));
            continue;
         }
      }

      error("statement didn't scan: %s", stmt);
   }

   if (bmp_type == SW)
      check_hash(cfg, testname, target, bmp_type);
   else
      check_similarity(testname, target, membuf, bmp_type, reliable);

   total_tests++;

   if (save_outputs) {
      ALLEGRO_USTR *filename = al_ustr_newf("%s.png", testname);
      al_save_bitmap(al_cstr(filename), target);
      al_ustr_free(filename);
   }

   if (!quiet) {
      if (target != al_get_backbuffer(display)) {
         set_target_reset(al_get_backbuffer(display));
         al_draw_bitmap(target, 0, 0, 0);
      }

      al_flip_display();
      al_rest(delay);
   }

   /* Destroy local bitmaps. */
   for (i = num_global_bitmaps; i < MAX_BITMAPS; i++) {
      if (bitmaps[i].name) {
         al_ustr_free(bitmaps[i].name);
         bitmaps[i].name = NULL;
         al_destroy_bitmap(bitmaps[i].bitmap[bmp_type]);
         bitmaps[i].bitmap[bmp_type] = NULL;
      }
   }

   /* Free transform names. */
   for (i = 0; i < MAX_TRANS; i++) {
      al_ustr_free(transforms[i].name);
      transforms[i].name = NULL;
   }

#undef B
#undef C
#undef F
#undef I
#undef SCAN
}

static void sw_hw_test(ALLEGRO_CONFIG const *cfg, char const *testname)
{
   int old_failed_tests = failed_tests;
   bool reliable;

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   do_test(cfg, testname, membuf, SW, true);

   reliable = (failed_tests == old_failed_tests);

   al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
   do_test(cfg, testname, al_get_backbuffer(display), HW, reliable);
}

static bool section_exists(ALLEGRO_CONFIG const *cfg, char const *section)
{
   void *iter;

   return al_get_first_config_entry(cfg, section, &iter) != NULL;
}

static void merge_config_sections(
   ALLEGRO_CONFIG *targ_cfg, char const *targ_section,
   ALLEGRO_CONFIG const *src_cfg, char const *src_section)
{
   char const *key;
   char const *value;
   void *iter;

   value = al_get_config_value(src_cfg, src_section, "extend");
   if (value) {
      if (streq(value, src_section)) {
         error("section cannot extend itself: %s "
            "(did you forget to rename a section?)", src_section);
      }
      merge_config_sections(targ_cfg, targ_section, src_cfg, value);
   }

   key = al_get_first_config_entry(src_cfg, src_section, &iter);
   if (!key) {
      error("missing section: %s", src_section);
   }
   for (; key != NULL; key = al_get_next_config_entry(&iter)) {
      value = al_get_config_value(src_cfg, src_section, key);
      al_set_config_value(targ_cfg, targ_section, key, value);
   }
}

static void run_test(ALLEGRO_CONFIG const *cfg, char const *section)
{
   char const *extend;
   ALLEGRO_CONFIG *cfg2;

   if (!section_exists(cfg, section)) {
      error("section not found: %s", section);
   }

   extend = al_get_config_value(cfg, section, "extend");
   if (!extend) {
      sw_hw_test(cfg, section);
   }
   else {
      cfg2 = al_create_config();
      merge_config_sections(cfg2, section, cfg, section);
      sw_hw_test(cfg2, section);
      al_destroy_config(cfg2);
   }
}

static void run_matching_tests(ALLEGRO_CONFIG const *cfg, const char *prefix)
{
   void *iter;
   char const *section;

   for (section = al_get_first_config_section(cfg, &iter);
         section != NULL;
         section = al_get_next_config_section(&iter)) {
      if (0 == strncmp(section, prefix, strlen(prefix))) {
         run_test(cfg, section);
      }
   }
}

static void partial_tests(ALLEGRO_CONFIG const *cfg,
   int argc, char const *argv[])
{
   ALLEGRO_USTR *name = al_ustr_new("");

   for (; argc > 0; argc--, argv++) {
      /* Automatically prepend "test" for convenience. */
      if (0 == strncmp(argv[0], "test ", 5)) {
         al_ustr_assign_cstr(name, argv[0]);
      }
      else {
         al_ustr_truncate(name, 0);
         al_ustr_appendf(name, "test %s", argv[0]);
      }

      /* Star suffix means run all matching tests. */
      if (al_ustr_has_suffix_cstr(name, "*")) {
         al_ustr_truncate(name, al_ustr_size(name) - 1);
         run_matching_tests(cfg, al_cstr(name));
      }
      else {
         run_test(cfg, al_cstr(name));
      }
   }

   al_ustr_free(name);
}

int main(int argc, char const *argv[])
{
   ALLEGRO_CONFIG *cfg;

   if (argc == 1) {
      error("requires config file argument");
   }
   argc--;
   argv++;

   if (!al_init()) {
      error("failed to initialise Allegro");
   }
   al_init_image_addon();
   al_init_font_addon();
   al_init_ttf_addon();
   al_init_primitives_addon();

   for (; argc > 0; argc--, argv++) {
      char const *opt = argv[0];
      if (streq(opt, "-d") || streq(opt, "--delay")) {
         delay = 1.0;
      }
      else if (streq(opt, "-s") || streq(opt, "--save")) {
         save_outputs = true;
      }
      else if (streq(opt, "-q") || streq(opt, "--quiet")) {
         quiet = true;
      }
      else if (streq(opt, "-v") || streq(opt, "--verbose")) {
         verbose = true;
      }
      else if (streq(opt, "-x") || streq(opt, "--no-exit-code")) {
         no_exit_code = true;
      }
      else {
         break;
      }
   }

   cfg = al_load_config_file(argv[0]);
   if (!cfg)
      error("failed to load config file %s", argv[0]);
   argc--;
   argv++;

   display = al_create_display(640, 480);
   if (!display) {
      error("failed to create display");
   }

   al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
   membuf = al_create_bitmap(
      al_get_display_width(display),
      al_get_display_height(display));
   load_bitmaps(cfg, "bitmaps", SW);

   al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
   load_bitmaps(cfg, "bitmaps", HW);
   load_fonts(cfg, "fonts");

   if (argc == 0)
      run_matching_tests(cfg, "test ");
   else
      partial_tests(cfg, argc, argv);

   al_destroy_config(cfg);

   printf("\n");
   printf("total tests:  %d\n", total_tests);
   printf("passed tests: %d\n", passed_tests);
   printf("failed tests: %d\n", failed_tests);

   if (no_exit_code)
      return 0;
   else
      return !!failed_tests;
}

/* vim: set sts=3 sw=3 et: */
