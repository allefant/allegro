/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Windows OpenGL display driver
 *
 *      By Milan Mimica.
 *      Based on AllegroGL Windows display driver.
 *
 */

#include <process.h>

#include "allegro5/allegro5.h"
#include "allegro5/system_new.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/platform/aintwin.h"

#include "wgl.h"


#define PREFIX_I                "wgl-win INFO: "
#define PREFIX_W                "wgl-win WARNING: "
#define PREFIX_E                "wgl-win ERROR: "


static ALLEGRO_DISPLAY_INTERFACE *vt = 0;
static bool init_done = false;

/* Forward declarations: */
static void display_thread_proc(void *arg);


/*
 * These parameters cannot be gotten by the display thread because
 * they're thread local. We get them in the calling thread first.
 */
typedef struct new_display_parameters {
   ALLEGRO_DISPLAY_WGL *display;
   bool init_failed;
   bool initialized;
} new_display_parameters;


/* Logs a Win32 error/warning message in the log file.
 */
static void log_win32_msg(const char *prefix, const char *func,
                          const char *error_msg, DWORD err) {

   char *err_msg = NULL;
   BOOL free_msg = TRUE;

   /* Get the formatting error string from Windows. Note that only the
    * bottom 14 bits matter - the rest are reserved for various library
    * IDs and type of error.
    */
   if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                    | FORMAT_MESSAGE_FROM_SYSTEM
                    | FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, err & 0x3FFF,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     (LPTSTR) &err_msg, 0, NULL)) {
      err_msg = "(Unable to decode error code)  ";
      free_msg = FALSE;
   }

   /* Remove two trailing characters */
   if (err_msg && strlen(err_msg) > 1)
      *(err_msg + strlen(err_msg) - 2) = '\0';

   TRACE("%s%s(): %s %s (0x%08lx)\n", prefix, func, error_msg ? error_msg : "",
      err_msg ? err_msg : "(null)", (unsigned long)err);

   if (free_msg) {
      LocalFree(err_msg);
   }

   return;
}


/* Logs an error */
static void log_win32_error(const char *func, const char *error_msg, DWORD err) {
   log_win32_msg(PREFIX_E, func, error_msg, err);
}


/* Logs a warning */
static void log_win32_warning(const char *func, const char *error_msg, DWORD err) {
   log_win32_msg(PREFIX_W, func, error_msg, err);
}


/* Logs a note */
static void log_win32_note(const char *func, const char *error_msg, DWORD err) {
   log_win32_msg(PREFIX_I, func, error_msg, err);
}


static bool is_wgl_extension_supported(AL_CONST char *extension, HDC dc)
{
   ALLEGRO_GetExtensionsStringARB_t __wglGetExtensionsStringARB;
   int ret;

   if (!glGetString(GL_EXTENSIONS))
      return false;

   __wglGetExtensionsStringARB = (ALLEGRO_GetExtensionsStringARB_t)
                                  wglGetProcAddress("wglGetExtensionsStringARB");
   ret = _al_ogl_look_for_an_extension(extension,
         (const GLubyte*)__wglGetExtensionsStringARB(dc));

   return ret;
}


static HGLRC init_temp_context(HWND wnd) {
   PIXELFORMATDESCRIPTOR pfd;
   int pf;
   HDC dc;
   HGLRC glrc;

   dc = GetDC(wnd);

   memset(&pfd, 0, sizeof(pfd));
   pfd.nSize = sizeof(pfd);
   pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL
	           | PFD_DOUBLEBUFFER_DONTCARE | PFD_STEREO_DONTCARE;
   pfd.iPixelType = PFD_TYPE_RGBA;
   pfd.iLayerType = PFD_MAIN_PLANE;
   pfd.cColorBits = 32;

   pf = ChoosePixelFormat(dc, &pfd);
   if (!pf) {
      log_win32_error("init_pixel_format_extensions", "Unable to chose a temporary pixel format!",
                      GetLastError());
      return NULL;
   }

   memset(&pfd, 0, sizeof(pfd));
   if (!SetPixelFormat(dc, pf, &pfd)) {
      log_win32_error("init_pixel_format_extensions", "Unable to set a temporary pixel format!",
                      GetLastError());
      return NULL;
   }

   glrc = wglCreateContext(dc);
   if (!glrc) {
      log_win32_error("init_pixel_format_extensions", "Unable to create a render context!",
                      GetLastError());
      return NULL;
   }

   if (!wglMakeCurrent(dc, glrc)) {
      log_win32_error("init_pixel_format_extensions", "Unable to set the render context as current!",
                      GetLastError());
      wglDeleteContext(glrc);
      return NULL;
   }

   return glrc;
}


static ALLEGRO_GetPixelFormatAttribivARB_t __wglGetPixelFormatAttribivARB = NULL;
static ALLEGRO_GetPixelFormatAttribivEXT_t __wglGetPixelFormatAttribivEXT = NULL;

static bool init_pixel_format_extensions()
{
   /* Load the ARB_p_f symbol - Note, we shouldn't use the extension
    * mechanism here, because it hasn't been initialized yet!
    */
   __wglGetPixelFormatAttribivARB =
      (ALLEGRO_GetPixelFormatAttribivARB_t)wglGetProcAddress("wglGetPixelFormatAttribivARB");
   __wglGetPixelFormatAttribivEXT = 
      (ALLEGRO_GetPixelFormatAttribivEXT_t)wglGetProcAddress("wglGetPixelFormatAttribivEXT");

   if (!__wglGetPixelFormatAttribivARB && !__wglGetPixelFormatAttribivEXT) {
      TRACE(PREFIX_E "init_pixel_format_extensions(): WGL_ARB/EXT_pf not supported!\n");
      return false;
   }

   return true;
}


static int get_pixel_formats_count_ext(HDC dc) {
   int attrib[1];
   int value[1];

   attrib[0] = WGL_NUMBER_PIXEL_FORMATS_ARB;
   if ((__wglGetPixelFormatAttribivARB(dc, 0, 0, 1, attrib, value) == GL_FALSE)
    && (__wglGetPixelFormatAttribivEXT(dc, 0, 0, 1, attrib, value) == GL_FALSE)) {
        log_win32_error("get_pixel_formats_count", "WGL_ARB/EXT_pixel_format use failed!",
                     GetLastError());
   }

   return value[0];
}


static bool decode_pixel_format_attrib(OGL_PIXEL_FORMAT *pf, int num_attribs,
                                      const int *attrib, const int *value) {
   int i;

   TRACE(PREFIX_I "Decoding: \n");

   pf->samples = 0;
   pf->sample_buffers = 0;
   pf->float_depth = 0;
   pf->float_color = 0;

   for (i = 0; i < num_attribs; i++) {
      /* Not interested if it doesn't support OpenGL or window drawing or RGBA. */
      if (attrib[i] == WGL_SUPPORT_OPENGL_ARB && value[i] == 0) {	
         TRACE(PREFIX_I "OpenGL Unsupported\n");
         return false;
      }
      else if (attrib[i] == WGL_DRAW_TO_WINDOW_ARB && value[i] == 0) {	
         TRACE(PREFIX_I "Can't draw to window\n");
         return false;
      }
      else if (attrib[i] == WGL_PIXEL_TYPE_ARB
         && (value[i] != WGL_TYPE_RGBA_ARB && value[i] != WGL_TYPE_RGBA_FLOAT_ARB)) {
         TRACE(PREFIX_I "Not RGBA mode\n");
         return false;
      }
      /* hardware acceleration */
      else if (attrib[i] == WGL_ACCELERATION_ARB) {
         pf->rmethod = (value[i] == WGL_NO_ACCELERATION_ARB) ? 0 : 1;
      }
      /* Depths of colour buffers */
      else if (attrib[i] == WGL_RED_BITS_ARB) {
         pf->r_size = value[i];
      }
      else if (attrib[i] == WGL_GREEN_BITS_ARB) {
         pf->g_size = value[i];
      }
      else if (attrib[i] == WGL_BLUE_BITS_ARB) {
         pf->b_size = value[i];
      }
      else if (attrib[i] == WGL_ALPHA_BITS_ARB) {
         pf->a_size = value[i];
      }
      /* Shift of color components */
      else if (attrib[i] == WGL_RED_SHIFT_ARB) {
         pf->r_shift = value[i];
      }
      else if (attrib[i] == WGL_GREEN_SHIFT_ARB) {
         pf->g_shift = value[i];
      }
      else if (attrib[i] == WGL_BLUE_SHIFT_ARB) {
         pf->b_shift = value[i];
      }
      else if (attrib[i] == WGL_ALPHA_SHIFT_ARB) {
         pf->a_shift = value[i];
      }
      /* Miscellaneous settings */
      else if (attrib[i] == WGL_DOUBLE_BUFFER_ARB) {
         pf->doublebuffered = value[i];
      }

      /* XXX: enable if needed, unused currently */
#if 0
      else if (attrib[i] == WGL_STEREO_ARB) {
         pf->stereo = value[i];
      }
      else if (attrib[i] == WGL_AUX_BUFFERS_ARB) {
         pf->aux_buffers = value[i];
      }
      else if (attrib[i] == WGL_STENCIL_BITS_ARB) {
         pf->stencil_size = value[i];
      }
      /* Depths of accumulation buffer */
      else if (attrib[i] == WGL_ACCUM_RED_BITS_ARB) {
         pf->accum_size.rgba.r = value[i];
      }
      else if (attrib[i] == WGL_ACCUM_GREEN_BITS_ARB) {
         pf->accum_size.rgba.g = value[i];
      }
      else if (attrib[i] == WGL_ACCUM_BLUE_BITS_ARB) {
         pf->accum_size.rgba.b = value[i];
      }
      else if (attrib[i] == WGL_ACCUM_ALPHA_BITS_ARB) {
         pf->accum_size.rgba.a = value[i];
      }
#endif // #if 0

      else if (attrib[i] == WGL_DEPTH_BITS_ARB) {
         pf->depth_size = value[i];
      }
      /* Multisampling bits */
      else if (attrib[i] == WGL_SAMPLE_BUFFERS_ARB) {
         pf->sample_buffers = value[i];
      }
      else if (attrib[i] == WGL_SAMPLES_ARB) {
         pf->samples = value[i];
      }
      /* Float color */
      if (attrib[i] == WGL_PIXEL_TYPE_ARB && value[i] == WGL_TYPE_RGBA_FLOAT_ARB) {
         pf->float_color = TRUE;
      }
      /* Float depth */
      else if (attrib[i] == WGL_DEPTH_FLOAT_EXT) {
         pf->float_depth = value[i];
      }
   }

   /* Setting some things based on what we've read out of the PFD. */

   /* FIXME: complete this with all formats */
   /* FIXME: how to detect XRGB formats? */
   /* XXX REVEIW: someone check this!!! */
   if (pf->r_size == 8 && pf->g_size == 8 && pf->b_size == 8) {
      if (pf->a_size == 8) {
         if (pf->a_shift == 0 && pf->b_shift == 8 && pf->g_shift == 16 && pf->r_shift == 24) {
            pf->format = ALLEGRO_PIXEL_FORMAT_RGBA_8888;
         }
         else if (pf->a_shift == 24 && pf->r_shift == 0 && pf->g_shift == 8 && pf->b_shift == 16) {
            pf->format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;
         }
         else if (pf->a_shift == 24 && pf->r_shift == 16 && pf->g_shift == 8 && pf->b_shift == 0) {
            pf->format = ALLEGRO_PIXEL_FORMAT_ARGB_8888;
         }
      }
      else if (pf->a_size == 0) {
         if (pf->b_shift == 0 && pf->g_shift == 8 && pf->r_shift == 16) {
            pf->format = ALLEGRO_PIXEL_FORMAT_RGB_888;
         }
         else if (pf->r_shift == 0 && pf->g_shift == 8 && pf->b_shift == 16) {
            pf->format = ALLEGRO_PIXEL_FORMAT_BGR_888;
         }
      }
   }
   else if (pf->r_size == 5 && pf->g_size == 6 && pf->b_size == 5) {
      if (pf->r_shift == 0 && pf->g_shift == 5 && pf->b_shift == 11) {
         pf->format = ALLEGRO_PIXEL_FORMAT_BGR_565;
      }
      else if (pf->b_shift == 0 && pf->g_shift == 5 && pf->r_shift == 11) {
         pf->format = ALLEGRO_PIXEL_FORMAT_RGB_565;
      }
   }


   /*pf->colour_depth = 0;
   if (pf->pixel_size.rgba.r == 5 && pf->pixel_size.rgba.b == 5) {
      if (pf->pixel_size.rgba.g == 5)
         pf->colour_depth = 15;
      if (pf->pixel_size.rgba.g == 6)
         pf->colour_depth = 16;
   }
   if (pf->pixel_size.rgba.r == 8 && pf->pixel_size.rgba.g == 8 && pf->pixel_size.rgba.b == 8) {
      if (pf->pixel_size.rgba.a == 8)
         pf->colour_depth = 32;
      else
         pf->colour_depth = 24;
   }*/

   /* FIXME: this is suppsed to tell if the pixel format is compatible with allegro's
    * color format, but this code was originally for 4.2.
    */
   /*
   pf->allegro_format =
         (pf->colour_depth != 0)
      && (pf->g_shift == pf->pixel_size.rgba.b)
      && (pf->r_shift * pf->b_shift == 0)
      && (pf->r_shift + pf->b_shift == pf->pixel_size.rgba.b + pf->pixel_size.rgba.g);
   */

   return true;
}


static OGL_PIXEL_FORMAT* read_pixel_format_ext(int fmt, HDC dc) {
   OGL_PIXEL_FORMAT *pf = NULL;

   /* Note: Even though we use te ARB suffix, all those enums are compatible
    * with EXT_pixel_format.
    */
   int attrib[] = {
      WGL_SUPPORT_OPENGL_ARB,
      WGL_DRAW_TO_WINDOW_ARB,
      WGL_PIXEL_TYPE_ARB,
      WGL_ACCELERATION_ARB,
      WGL_DOUBLE_BUFFER_ARB,
      WGL_DEPTH_BITS_ARB,
      WGL_COLOR_BITS_ARB,
      WGL_RED_BITS_ARB,
      WGL_GREEN_BITS_ARB,
      WGL_BLUE_BITS_ARB,
      WGL_ALPHA_BITS_ARB,
      WGL_RED_SHIFT_ARB,
      WGL_GREEN_SHIFT_ARB,
      WGL_BLUE_SHIFT_ARB,
      WGL_ALPHA_SHIFT_ARB,
      WGL_STENCIL_BITS_ARB,
      WGL_STEREO_ARB,
      WGL_ACCUM_BITS_ARB,
      WGL_ACCUM_RED_BITS_ARB,
      WGL_ACCUM_GREEN_BITS_ARB,
      WGL_ACCUM_BLUE_BITS_ARB,
      WGL_ACCUM_ALPHA_BITS_ARB,
      WGL_AUX_BUFFERS_ARB,

      /* The following are used by extensions that add to WGL_pixel_format.
       * If WGL_p_f isn't supported though, we can't use the (then invalid)
       * enums. We can't use any magic number either, so we settle for 
       * replicating one. The pixel format decoder
       * (decode_pixel_format_attrib()) doesn't care about duplicates.
       */
      WGL_AUX_BUFFERS_ARB, /* placeholder for WGL_SAMPLE_BUFFERS_ARB */
      WGL_AUX_BUFFERS_ARB, /* placeholder for WGL_SAMPLES_ARB        */
      WGL_AUX_BUFFERS_ARB, /* placeholder for WGL_DEPTH_FLOAT_EXT    */
   };

   const int num_attribs = sizeof(attrib) / sizeof(attrib[0]);
   int *value = (int*)malloc(sizeof(int) * num_attribs);
   int ret;

   if (!value)
      return NULL;

   /* If multisampling is supported, query for it. */
   if (is_wgl_extension_supported("WGL_ARB_multisample", dc)) {
      attrib[num_attribs - 3] = WGL_SAMPLE_BUFFERS_ARB;
      attrib[num_attribs - 2] = WGL_SAMPLES_ARB;
   }
   if (is_wgl_extension_supported("WGL_EXT_depth_float", dc)) {
      attrib[num_attribs - 1] = WGL_DEPTH_FLOAT_EXT;
   }

   /* Get the pf attributes */
   if (__wglGetPixelFormatAttribivARB) {
      ret = __wglGetPixelFormatAttribivARB(dc, fmt, 0, num_attribs, attrib, value);
   }
   else if (__wglGetPixelFormatAttribivEXT) {
      ret = __wglGetPixelFormatAttribivEXT(dc, fmt, 0, num_attribs, attrib, value);
   }
   else {
      log_win32_error("describe_pixel_format_new", "wglGetPixelFormatAttrib failed!",
                      GetLastError());
      free(value);
      return NULL;
   }

   pf = malloc(sizeof *pf);
   if (!decode_pixel_format_attrib(pf, num_attribs, attrib, value)) {
      free(pf);
      pf = NULL;
   }

   free(value);

   return pf;
}


static bool change_display_mode(ALLEGRO_DISPLAY *d) {
   DEVMODE dm;
   DEVMODE fallback_dm;
   int i, modeswitch, result;
   int fallback_dm_valid = 0;

   memset(&fallback_dm, 0, sizeof(fallback_dm));
   memset(&dm, 0, sizeof(dm));
   dm.dmSize = sizeof(DEVMODE);

   i = 0;
   do {
      modeswitch = EnumDisplaySettings(NULL, i, &dm);
      if (!modeswitch)
         break;

      if ((dm.dmPelsWidth  == (unsigned) d->w)
       && (dm.dmPelsHeight == (unsigned) d->h)
       && (dm.dmBitsPerPel == 32) /* FIXME */
       && (dm.dmDisplayFrequency != (unsigned) d->refresh_rate)) {
         /* Keep it as fallback if refresh rate request could not
          * be satisfied. Try to get as close to 60Hz as possible though,
          * it's a bit better for a fallback than just blindly picking
          * something like 47Hz or 200Hz.
          */
         if (!fallback_dm_valid) {
            fallback_dm = dm;
            fallback_dm_valid = 1;
         }
         else if (dm.dmDisplayFrequency >= 60) {
            if (dm.dmDisplayFrequency < fallback_dm.dmDisplayFrequency) {
                fallback_dm = dm;
            }
         }
      }
      i++;
   }
   while ((dm.dmPelsWidth  != (unsigned) d->w)
       || (dm.dmPelsHeight != (unsigned) d->h)
       || (dm.dmBitsPerPel != 32) /* FIXME */
       || (dm.dmDisplayFrequency != (unsigned) d->refresh_rate));

   if (!modeswitch && !fallback_dm_valid) {
      TRACE(PREFIX_E "change_display_mode: Mode not found.");
      return false;
   }

   if (!modeswitch && fallback_dm_valid)
      dm = fallback_dm;

   dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;
   result = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);

   if (result != DISP_CHANGE_SUCCESSFUL) {
      log_win32_error("change_display_mode", "Unable to set mode!",
                      GetLastError());
      return false;
   }

   TRACE(PREFIX_I "change_display_mode: Mode seccessfuly set.");
   return true;
}


static OGL_PIXEL_FORMAT** get_available_pixel_formats_ext(int *count) {
   HWND testwnd = NULL;
   HDC testdc   = NULL;
   HGLRC testrc = NULL;
   HGLRC old_rc = NULL;
   HDC old_dc   = NULL;
   OGL_PIXEL_FORMAT **pf_list = NULL;
   int maxindex;
   int i;

   /* We need to create a dummy window with a pixel format to get the
    * list of valid PFDs
    */
   testwnd = _al_win_create_hidden_window();
   if (!testwnd)
      return false;

   old_rc = wglGetCurrentContext();
   old_dc = wglGetCurrentDC();

   testdc = GetDC(testwnd);
   testrc = init_temp_context(testwnd);
   if (!testrc)
      goto bail;

   if (!init_pixel_format_extensions())
      goto bail;

   maxindex = get_pixel_formats_count_ext(testdc);
   if (maxindex < 1)
      goto bail;
   *count = maxindex;
   TRACE(PREFIX_I "get_available_pixel_formats_ext(): Got %i visuals.\n", maxindex);

   pf_list = malloc(maxindex * sizeof(*pf_list));
   if (!pf_list)
      goto bail;

   for (i = 0; i < maxindex; i++) {
      TRACE(PREFIX_I "Reading visual no. %i...\n", i);
      pf_list[i] = read_pixel_format_ext(i, testdc);
   }

bail:
   wglMakeCurrent(NULL, NULL);
   if (testrc) {
      wglDeleteContext(testrc);
   }

   wglMakeCurrent(old_dc, old_rc);

   __wglGetPixelFormatAttribivARB = NULL;
   __wglGetPixelFormatAttribivEXT = NULL;

   if (testwnd) {
      ReleaseDC(testwnd, testdc);
      DestroyWindow(testwnd);
   }

   return pf_list;
}


static bool try_to_set_pixel_format(int i) {
   HWND testwnd;
   HDC testdc;
   PIXELFORMATDESCRIPTOR pfd;

   /* Recreate our test windows */
   testwnd = _al_win_create_hidden_window();
   if (!testwnd)
      return false;
   testdc = GetDC(testwnd);
		
   if (SetPixelFormat(testdc, i, &pfd)) {
      HGLRC rc = wglCreateContext(testdc);
      if (!rc) {
         TRACE(PREFIX_I "try_to_set_pixel_format(): Unable to create RC!\n");
      }
      else {
         if (wglMakeCurrent(testdc, rc)) {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(rc);
            ReleaseDC(testwnd, testdc);
            DestroyWindow(testwnd);

            return true;
         }
         else {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(rc);
            log_win32_warning("try_to_set_pixel_format", "Couldn't make the temporary render context "
                              "current for the this pixel format.", GetLastError());
         }
      }
   }
   else {
      log_win32_note("try_to_set_pixel_format", "Unable to set pixel format!", GetLastError());
   }

   ReleaseDC(testwnd, testdc);
   DestroyWindow(testwnd);
   testdc = NULL;
   testwnd = NULL;

   return false;
}


static bool select_pixel_format(ALLEGRO_DISPLAY_WGL *d) {
   HWND testwnd = NULL;
   HDC testdc   = NULL;
   HGLRC testrc = NULL;
   OGL_PIXEL_FORMAT **pf_list = NULL;
   enum ALLEGRO_PIXEL_FORMAT format = ((ALLEGRO_DISPLAY *) d)->format;
   int maxindex = 0;
   int i;

   /* There are two ways to describe pixel formats: the old way, using DescribePixelFormat,
    * and the new way, using wglGetPixelFormatAttribiv extension. The driver
    * currently uses only the later, which is more advanced but may fail on older/broken
    * GFX drivers. FIXME: Should fallback to the old way in case something goes wrong. */

   pf_list = get_available_pixel_formats_ext(&maxindex);

   for (i = 0; i < maxindex; i++) {
      OGL_PIXEL_FORMAT *pf = pf_list[i];
      /* TODO: implement a choice system (scoring?) */
      if (pf && pf->doublebuffered == true && pf->format == format) {
         if (try_to_set_pixel_format(i)) {
            PIXELFORMATDESCRIPTOR pdf;
            TRACE(PREFIX_I "select_pixel_format(): Chose visual no. %i\n.", i);
            /* TODO: read info out of pdf. Print it too.*/
            SetPixelFormat(d->dc, i, &pdf);
            break;
         }
      }
   }

   for (i = 0; i < maxindex; i++)
      free(pf_list[i]);

   if (pf_list)
      free(pf_list);

   return true;
}


static bool create_display_internals(ALLEGRO_DISPLAY_WGL *wgl_disp) {
   ALLEGRO_DISPLAY *disp = (void*)wgl_disp;
   static new_display_parameters ndp;

   ndp.display = wgl_disp;
   ndp.init_failed = false;
   ndp.initialized = false;
   _beginthread(display_thread_proc, 0, &ndp);

   while (!ndp.initialized && !ndp.init_failed)
      al_rest(0.001);

   if (ndp.init_failed) {
      TRACE(PREFIX_E "Faild to create display.\n");
      return false;
   }

   return true;
}


static ALLEGRO_DISPLAY* wgl_create_display(int w, int h) {
   ALLEGRO_SYSTEM_WIN *system = (ALLEGRO_SYSTEM_WIN *)al_system_driver();
   ALLEGRO_DISPLAY_WGL **add;
   ALLEGRO_DISPLAY_WGL *wgl_display = _AL_MALLOC(sizeof *wgl_display);
   ALLEGRO_DISPLAY_OGL *ogl_display = (void*)wgl_display;
   ALLEGRO_DISPLAY     *display     = (void*)ogl_display;

   memset(display, 0, sizeof *wgl_display);
   display->w = w;
   display->h = h;
   //FIXME
   display->format = ALLEGRO_PIXEL_FORMAT_RGB_888;//al_get_new_display_format();
   display->refresh_rate = al_get_new_display_refresh_rate();
   display->flags = al_get_new_display_flags();
   display->vt = vt;

   if (!create_display_internals(wgl_display)) {
      return NULL;
   }

   win_grab_input();

   ogl_display->backbuffer = _al_ogl_create_backbuffer(display);

   /* Init will finish in set_current_display() */
   init_done = false;

   /* Add ourself to the list of displays. */
   add = _al_vector_alloc_back(&system->system.displays);
   *add = wgl_display;

   /* Each display is an event source. */
   _al_event_source_init(&display->es);

   return display;
}


static void wgl_destroy_display(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_SYSTEM_WIN *system = (ALLEGRO_SYSTEM_WIN *)al_system_driver();
   ALLEGRO_DISPLAY_OGL *ogl_disp = (ALLEGRO_DISPLAY_OGL *)display;
   ALLEGRO_DISPLAY_WGL *wgl_disp = (ALLEGRO_DISPLAY_WGL *)ogl_disp;

   /* REVIEW: can al_destroy_bitmap() handle backbuffers? */
   if (ogl_disp->backbuffer)
      al_destroy_bitmap((ALLEGRO_BITMAP*)ogl_disp->backbuffer);
   ogl_disp->backbuffer = NULL;

   _al_ogl_unmanage_extensions(ogl_disp);

   wgl_disp->end_thread = true;
   while (!wgl_disp->thread_ended)
      al_rest(0.001);

   _al_vector_find_and_delete(&system->system.displays, &display);

   _AL_FREE(wgl_disp);
}


static void wgl_set_current_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_WGL *wgl_disp = (ALLEGRO_DISPLAY_WGL *)d;
   ALLEGRO_DISPLAY_OGL *ogl_disp = (ALLEGRO_DISPLAY_OGL *)d;

   /* make the context the current one */
   if (!wglMakeCurrent(wgl_disp->dc, wgl_disp->glrc)) {
      log_win32_error("wgl_set_current_display", "Unable to make the context current!",
                       GetLastError());
      return;
   }

   if (!init_done) {
      /* Print out OpenGL version info */
      TRACE(PREFIX_I "OpenGL Version: %s\n", (const char*)glGetString(GL_VERSION));
      TRACE(PREFIX_I "Vendor: %s\n", (const char*)glGetString(GL_VENDOR));
      TRACE(PREFIX_I "Renderer: %s\n\n", (const char*)glGetString(GL_RENDERER));

      _al_ogl_manage_extensions(ogl_disp);

      init_done = true;
   }

   _al_ogl_set_extensions(ogl_disp->extension_api);
}


static void set_window_style(ALLEGRO_DISPLAY_WGL *wgl_disp)
{
   ALLEGRO_DISPLAY *disp = (ALLEGRO_DISPLAY*)wgl_disp;
   RECT rect;
   DWORD style;
   DWORD exstyle;
   bool fullscreen = disp->flags & ALLEGRO_FULLSCREEN;
   /* TODO: make this configurable */
   int x = 100;
   int y = 100;

   if (fullscreen) {
      style = WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
      exstyle = WS_EX_APPWINDOW | WS_EX_TOPMOST;
   }
   else {
      style = WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_CLIPCHILDREN
            | WS_CLIPSIBLINGS;
      exstyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
   }


   SetWindowLong(wgl_disp->window, GWL_STYLE, style);
   SetWindowLong(wgl_disp->window, GWL_EXSTYLE, exstyle);

   if (!fullscreen) {
      rect.left = x;
      rect.right = x + disp->w;
      rect.top = y;
      rect.bottom = y + disp->h;
   }
   else {
      rect.left = 0;
      rect.right = disp->w;
      rect.top  = 0;
      rect.bottom = disp->h;
   }

   if (!fullscreen) {
      AdjustWindowRectEx(&rect, style, FALSE, exstyle);
   }

   SetWindowPos(wgl_disp->window, 0, rect.left, rect.top,
         rect.right - rect.left, rect.bottom - rect.top,
         SWP_NOZORDER | SWP_FRAMECHANGED);
}


/*
 * The window must be created in the same thread that
 * runs the message loop.
 */
static void display_thread_proc(void *arg)
{
   new_display_parameters *ndp = arg;
   ALLEGRO_DISPLAY *disp = (ALLEGRO_DISPLAY*)ndp->display;
   ALLEGRO_DISPLAY_WGL *wgl_disp = (void*)disp;
   DWORD result;
   MSG msg;

   wgl_disp->window = _al_win_create_window(disp, disp->w, disp->h, disp->flags);

   if (!wgl_disp->window) {
      ndp->init_failed = true;
      return;
   }

   set_window_style(wgl_disp);

   /* get the device context of our window */
   wgl_disp->dc = GetDC(wgl_disp->window);

   if (disp->flags & ALLEGRO_FULLSCREEN) {
      if (!change_display_mode(disp)) {
         wgl_disp->thread_ended = true;
         wgl_destroy_display(disp);
         ndp->init_failed = true;
         return;
      }
   }

   if (!select_pixel_format(wgl_disp)) {
      wgl_disp->thread_ended = true;
      wgl_destroy_display(disp);
      ndp->init_failed = true;
      return;
   }

   /* create an OpenGL context */
   wgl_disp->glrc = wglCreateContext(wgl_disp->dc);
   if (!wgl_disp->glrc) {
      log_win32_error("wgl_disp_display_thread_proc", "Unable to create a render context!",
                      GetLastError());
      wgl_disp->thread_ended = true;
      wgl_destroy_display(disp);
      ndp->init_failed = true;
      return;
   }

   /* <rohannessian> Win98/2k/XP's window forground rules don't let us
   * make our window the topmost window on launch. This causes issues on
   * full-screen apps, as DInput loses input focus on them.
   * We use this trick to force the window to be topmost, when switching
   * to full-screen only. Note that this only works for Win98 and greater.
   * Win95 will ignore our SystemParametersInfo() calls.
   *
   * See http://support.microsoft.com:80/support/kb/articles/Q97/9/25.asp
   * for details.
   */
   {
      HWND wnd = wgl_disp->window;
      DWORD lock_time;

#define SPI_GETFOREGROUNDLOCKTIMEOUT 0x2000
#define SPI_SETFOREGROUNDLOCKTIMEOUT 0x2001
      if (disp->flags & ALLEGRO_FULLSCREEN) {
            SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT,
                                 0, (LPVOID)&lock_time, 0);
            SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,
                                 0, (LPVOID)0,
                                 SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
      }

      ShowWindow(wnd, SW_SHOWNORMAL);
      SetForegroundWindow(wnd);
      /* In some rare cases, it doesn't seem to work without the loop. And we
       * absolutely need this to succeed, else we trap the user in a
       * fullscreen window without input.
       */
      while (GetForegroundWindow() != wnd) {
         al_rest(0.001);
         SetForegroundWindow(wnd);
      }
      UpdateWindow(wnd);

      if (disp->flags & ALLEGRO_FULLSCREEN) {
         SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT,
                              0, (LPVOID)lock_time,
                              SPIF_SENDWININICHANGE | SPIF_UPDATEINIFILE);
      }
#undef SPI_GETFOREGROUNDLOCKTIMEOUT
#undef SPI_SETFOREGROUNDLOCKTIMEOUT
   }


   wgl_disp->thread_ended = false;
   ndp->initialized = true;

   for (;;) {
      if (wgl_disp->end_thread) {
         break;
      }
      /* FIXME: How long should we wait? */
      result = MsgWaitForMultipleObjects(_win_input_events, _win_input_event_id, FALSE, 5/*INFINITE*/, QS_ALLINPUT);
      if (result < (DWORD) WAIT_OBJECT_0 + _win_input_events) {
         /* one of the registered events is in signaled state */
         (*_win_input_event_handler[result - WAIT_OBJECT_0])();
      }
      else if (result == (DWORD) WAIT_OBJECT_0 + _win_input_events) {
         /* messages are waiting in the queue */
         while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
            if (GetMessage(&msg, NULL, 0, 0)) {
               DispatchMessage(&msg);
            }
            else {
               goto End;
            }
         }
      }
   }

End:
   if (wgl_disp->glrc) {
      wglDeleteContext(wgl_disp->glrc);
   }
   if (wgl_disp->dc) {
      ReleaseDC(wgl_disp->window, wgl_disp->dc);
   }

   if (disp->flags & ALLEGRO_FULLSCREEN) {
      ChangeDisplaySettings(NULL, 0);
   }

   _al_win_delete_thread_handle(GetCurrentThreadId());

   TRACE("wgl display thread exits\n");
   wgl_disp->thread_ended = true;
}


static void wgl_flip_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_WGL* disp = (ALLEGRO_DISPLAY_WGL*)d;
   glFlush();
   SwapBuffers(disp->dc);
}


static bool wgl_update_display_region(ALLEGRO_DISPLAY *d,
                                      int x, int y, int width, int height)
{
   return true;
}


static bool wgl_acknowledge_resize(ALLEGRO_DISPLAY *d)
{
   /* hm... */
   return true;
}


static bool wgl_resize_display(ALLEGRO_DISPLAY *d, int width, int height)
{
   /* hm#2 .... */
   return true;
}


static bool wgl_is_compatible_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   return true;
}


static bool wgl_wait_for_vsync(ALLEGRO_DISPLAY *display)
{
   return true;
}


static bool wgl_show_cursor(ALLEGRO_DISPLAY *display)
{
   return true;
}


static bool wgl_hide_cursor(ALLEGRO_DISPLAY *display)
{
   return true;
}


/* Obtain a reference to this driver. */
ALLEGRO_DISPLAY_INTERFACE *_al_display_wgl_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->create_display = wgl_create_display;
   vt->destroy_display = wgl_destroy_display;
   vt->resize_display = wgl_resize_display;
   vt->set_current_display = wgl_set_current_display;
   vt->flip_display = wgl_flip_display;
   vt->update_display_region = wgl_update_display_region;
   vt->acknowledge_resize = wgl_acknowledge_resize;
   vt->create_bitmap = _al_ogl_create_bitmap;
   vt->get_backbuffer = _al_ogl_get_backbuffer;
   vt->get_frontbuffer = _al_ogl_get_backbuffer;
   vt->set_target_bitmap = _al_ogl_set_target_bitmap;
   vt->is_compatible_bitmap = wgl_is_compatible_bitmap;
   vt->switch_out = NULL;
   vt->switch_in = NULL;
   vt->upload_compat_screen = NULL;
   vt->show_cursor = wgl_show_cursor;
   vt->hide_cursor = wgl_hide_cursor;
   vt->set_icon = _al_win_set_display_icon;
   _al_ogl_add_drawing_functions(vt);

   return vt;
}

int _al_wgl_get_num_display_modes(int format, int refresh_rate, int flags)
{
   return 0;
}


ALLEGRO_DISPLAY_MODE *_al_wgl_get_display_mode(int index, int format,
   int refresh_rate, int flags, ALLEGRO_DISPLAY_MODE *mode)
{
   return NULL;
}



