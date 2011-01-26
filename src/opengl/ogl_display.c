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
 *      OpenGL routines common to all OpenGL drivers.
 *
 *      By Elias Pschernig and Milan Mimica.
 *
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_pixels.h"

#ifdef ALLEGRO_GP2XWIZ
#include "allegro5/internal/aintern_gp2xwiz.h"
#endif

#ifdef ALLEGRO_IPHONE
#include "allegro5/internal/aintern_iphone.h"
#endif

ALLEGRO_DEBUG_CHANNEL("opengl")

static void setup_fbo(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap;
   
   if (bitmap->parent) bitmap = bitmap->parent;
   ogl_bitmap = (void *)bitmap;

#if !defined ALLEGRO_GP2XWIZ
   if (!ogl_bitmap->is_backbuffer) {
       
      // FIXME: ...
      #ifdef ALLEGRO_IPHONE
      #define glGenFramebuffersEXT glGenFramebuffersOES
      #define glBindFramebufferEXT glBindFramebufferOES
      #define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER_OES
      #define GL_COLOR_ATTACHMENT0_EXT GL_COLOR_ATTACHMENT0_OES
      #define glCheckFramebufferStatusEXT glCheckFramebufferStatusOES
      #define glFramebufferTexture2DEXT glFramebufferTexture2DOES
      #define GL_FRAMEBUFFER_COMPLETE_EXT GL_FRAMEBUFFER_COMPLETE_OES
      #define glDeleteFramebuffersEXT glDeleteFramebuffersOES
      #define glOrtho glOrthof
      #endif

      /* When a bitmap is set as target bitmap, we try to create an FBO for it.
       */
      if (ogl_bitmap->fbo == 0 && !(bitmap->flags & ALLEGRO_FORCE_LOCKING)) {
      
   /* FIXME This is quite a hack but I don't know how the Allegro extension
    * manager works to fix this properly (getting extensions properly reported
    * on iphone.
    */
#ifdef ALLEGRO_IPHONE
         if (true) {
#else
         if (al_get_opengl_extension_list()->ALLEGRO_GL_EXT_framebuffer_object ||
            al_get_opengl_extension_list()->ALLEGRO_GL_OES_framebuffer_object) {
#endif
            glGenFramebuffersEXT(1, &ogl_bitmap->fbo);
         }
      }

      if (ogl_bitmap->fbo) {
         /* Bind to the FBO. */
#ifndef ALLEGRO_IPHONE
         ASSERT(display->ogl_extras->extension_list->ALLEGRO_GL_EXT_framebuffer_object ||
         display->ogl_extras->extension_list->ALLEGRO_GL_OES_framebuffer_object);
#endif
         glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, ogl_bitmap->fbo);

         /* Attach the texture. */
         glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
            GL_TEXTURE_2D, ogl_bitmap->texture, 0);
         if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) !=
            GL_FRAMEBUFFER_COMPLETE_EXT) {
            /* For some reason, we cannot use the FBO with this
             * texture. So no reason to keep re-trying, output a log
             * message and switch to (extremely slow) software mode.
             */
            ALLEGRO_ERROR("Could not use FBO for bitmap with format %s.\n",
               _al_format_name(bitmap->format));
            ALLEGRO_ERROR("*** SWITCHING TO SOFTWARE MODE ***\n");
            glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
            glDeleteFramebuffersEXT(1, &ogl_bitmap->fbo);
            ogl_bitmap->fbo = 0;
         }
         else {
            display->ogl_extras->opengl_target = ogl_bitmap;

            glViewport(0, 0, bitmap->w, bitmap->h);

            al_identity_transform(&display->proj_transform);
            al_ortho_transform(&display->proj_transform, 0, bitmap->w, bitmap->h, 0, -1, 1);

            display->vt->set_projection(display);
         }
      }
   }
   else {
      display->ogl_extras->opengl_target = ogl_bitmap;

#ifndef ALLEGRO_IPHONE
      if (display->ogl_extras->extension_list->ALLEGRO_GL_EXT_framebuffer_object ||
          display->ogl_extras->extension_list->ALLEGRO_GL_OES_framebuffer_object) {
         glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
      }      

      glViewport(0, 0, display->w, display->h);
   
      al_identity_transform(&display->proj_transform);
      /* We use upside down coordinates compared to OpenGL, so the bottommost
       * coordinate is display->h not 0.
       */
      al_ortho_transform(&display->proj_transform, 0, display->w, display->h, 0, -1, 1);

      display->vt->set_projection(display);
#else
      glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
      _al_iphone_setup_opengl_view(display);
#endif
      if (display->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
         GLint handle;
         GLuint program_object = display->ogl_extras->program_object;
         handle = glGetUniformLocation(program_object, "proj_matrix");
         if (handle >= 0) {
            glUniformMatrix4fv(handle, 1, false, (float *)display->proj_transform.m);
         }
         handle = glGetUniformLocation(program_object, "view_matrix");
         if (handle >= 0) {
            glUniformMatrix4fv(handle, 1, false, (float *)display->view_transform.m);
         }
      }
   }
#else

   ALLEGRO_DISPLAY_GP2XWIZ_OGL *wiz_disp = (ALLEGRO_DISPLAY_GP2XWIZ_OGL *)display;
   display->ogl_extras->opengl_target = ogl_bitmap;

   if (!ogl_bitmap->is_backbuffer) {
      /* FIXME: implement */
   }
   else {
      eglMakeCurrent(wiz_disp->egl_display, wiz_disp->egl_surface, wiz_disp->egl_surface, wiz_disp->egl_context); 

      glViewport(0, 0, display->w, display->h);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      /* We use upside down coordinates compared to OpenGL, so the bottommost
       * coordinate is display->h not 0.
       */
      glOrtho(0, display->w, display->h, 0, -1, 1);
   }
#endif
}

void _al_ogl_set_target_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)bitmap;
   if (bitmap->parent)
      ogl_bitmap = (void *)bitmap->parent;

   if (!bitmap->locked) {
      setup_fbo(display, bitmap);

      if (display->ogl_extras->opengl_target == ogl_bitmap) {
         _al_ogl_setup_bitmap_clipping(bitmap);
      }
   }
}


/* Function: al_set_current_opengl_context
 */
void al_set_current_opengl_context(ALLEGRO_DISPLAY *display)
{
   ASSERT(display);

   if (!(display->flags & ALLEGRO_OPENGL))
      return;

   if (display) {
      ALLEGRO_BITMAP *bmp = al_get_target_bitmap();
      if (bmp && bmp->display && bmp->display != display) {
         al_set_target_bitmap(NULL);
      }
   }

   _al_set_current_display_only(display);
}


void _al_ogl_setup_bitmap_clipping(const ALLEGRO_BITMAP *bitmap)
{
   int x_1, y_1, x_2, y_2, h;
   bool use_scissor = true;

   x_1 = bitmap->cl;
   y_1 = bitmap->ct;
   x_2 = bitmap->cr_excl;
   y_2 = bitmap->cb_excl;
   h = bitmap->h;

   /* Drawing onto the sub bitmap is handled by clipping the parent. */
   if (bitmap->parent) {
      x_1 += bitmap->xofs;
      y_1 += bitmap->yofs;
      x_2 += bitmap->xofs;
      y_2 += bitmap->yofs;
      h = bitmap->parent->h;
   }

   if (x_1 == 0 &&  y_1 == 0 && x_2 == bitmap->w && y_2 == bitmap->h) {
      if (bitmap->parent) {
         /* Can only disable scissor if the sub-bitmap covers the
          * complete parent.
          */
         if (bitmap->xofs == 0 && bitmap->yofs == 0 && bitmap->w ==
            bitmap->parent->w && bitmap->h == bitmap->parent->h) {
               use_scissor = false;
            }
      }
      else
         use_scissor = false;
   }
   if (!use_scissor) {
      glDisable(GL_SCISSOR_TEST);
   }
   else {
      glEnable(GL_SCISSOR_TEST);
       
      #ifdef ALLEGRO_IPHONE
      _al_iphone_clip(bitmap, x_1, y_1, x_2, y_2);
      #else
      /* OpenGL is upside down, so must adjust y_2 to the height. */
      glScissor(x_1, h - y_2, x_2 - x_1, y_2 - y_1);
      #endif
   }
}


ALLEGRO_BITMAP *_al_ogl_get_backbuffer(ALLEGRO_DISPLAY *d)
{
   return (ALLEGRO_BITMAP *)d->ogl_extras->backbuffer;
}


bool _al_ogl_resize_backbuffer(ALLEGRO_BITMAP_OGL *b, int w, int h)
{
   int pitch;
         
   pitch = w * al_get_pixel_size(b->bitmap.format);

   b->bitmap.w = w;
   b->bitmap.h = h;
   b->bitmap.pitch = pitch;
   b->bitmap.cl = 0;
   b->bitmap.ct = 0;
   b->bitmap.cr_excl = w;
   b->bitmap.cb_excl = h;

   /* There is no texture associated with the backbuffer so no need to care
    * about texture size limitations. */
   b->true_w = w;
   b->true_h = h;

   /* This code below appears to be unneccessary on platforms other than
    * OS X.
    */
#ifdef ALLEGRO_MACOSX
   b->bitmap.display->vt->set_projection(b->bitmap.display);
#endif

#if !defined(ALLEGRO_IPHONE) && !defined(ALLEGRO_GP2XWIZ)
   b->bitmap.memory = NULL;
#else
   /* iPhone/Wiz ports still expect the buffer to be present. */
   {
      /* FIXME: lazily manage memory */
      size_t bytes = pitch * h;
      al_free(b->bitmap.memory);
      b->bitmap.memory = al_malloc(bytes);
      memset(b->bitmap.memory, 0, bytes);
   }
#endif

   return true;
}


ALLEGRO_BITMAP_OGL* _al_ogl_create_backbuffer(ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_BITMAP_OGL *ogl_backbuffer;
   ALLEGRO_BITMAP *backbuffer;
   ALLEGRO_STATE backup;
   int format;

   ALLEGRO_DEBUG("Creating backbuffer\n");

   al_store_state(&backup, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);

   // FIXME: _al_deduce_color_format would work fine if the display paramerers
   // are filled in, for WIZ and IPOD
#ifdef ALLEGRO_GP2XWIZ
   format = ALLEGRO_PIXEL_FORMAT_RGB_565; /* Only support display format */
#elif defined ALLEGRO_IPHONE
   format = ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE;
   // TODO: This one is also supported
   //format = ALLEGRO_PIXEL_FORMAT_RGB_565;
#else
   format = _al_deduce_color_format(&disp->extra_settings);
   /* Eww. No OpenGL hardware in the world does that - let's just
    * switch to some default.
    */
   if (al_get_pixel_size(format) == 3) {
      /* Or should we use RGBA? Maybe only if not Nvidia cards? */
      format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;
   }
#endif
   ALLEGRO_TRACE_CHANNEL_LEVEL("display", 1)("Deduced format %s for backbuffer.\n",
      _al_format_name(format));

   /* Now that the display backbuffer has a format, update extra_settings so
    * the user can query it back.
    */
   _al_set_color_components(format, &disp->extra_settings, ALLEGRO_REQUIRE);
   disp->backbuffer_format = format;

   ALLEGRO_DEBUG("Creating backbuffer bitmap\n");
   al_set_new_bitmap_format(format);
   al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
   backbuffer = _al_ogl_create_bitmap(disp, disp->w, disp->h);
   al_restore_state(&backup);

   if (!backbuffer) {
      ALLEGRO_DEBUG("Backbuffer bitmap creation failed.\n");
      return NULL;
   }
   
   ALLEGRO_TRACE_CHANNEL_LEVEL("display", 1)(
      "Created backbuffer bitmap (actual format: %s)\n",
      _al_format_name(backbuffer->format));

   ogl_backbuffer = (ALLEGRO_BITMAP_OGL*)backbuffer;
   ogl_backbuffer->is_backbuffer = 1;
   backbuffer->display = disp;

   al_identity_transform(&disp->view_transform);

   return ogl_backbuffer;
}


void _al_ogl_destroy_backbuffer(ALLEGRO_BITMAP_OGL *b)
{
   al_destroy_bitmap((ALLEGRO_BITMAP *)b);
}

void al_set_opengl_program_object(ALLEGRO_DISPLAY *display, GLuint program_object)
{
   GLint handle;
   
   display->ogl_extras->program_object = program_object;

   glUseProgram(program_object);
      
   handle = glGetUniformLocation(program_object, "view_matrix");
   if (handle >= 0) {
      glUniformMatrix4fv(handle, 1, false, (float *)display->view_transform.m);
   }
   handle = glGetUniformLocation(program_object, "proj_matrix");
   if (handle >= 0) {
      glUniformMatrix4fv(handle, 1, false, (float *)display->proj_transform.m);
   }

   display->ogl_extras->pos_loc = glGetAttribLocation(program_object, "pos");
   display->ogl_extras->color_loc = glGetAttribLocation(program_object, "color");
   display->ogl_extras->texcoord_loc = glGetAttribLocation(program_object, "texcoord");
   display->ogl_extras->use_tex_loc = glGetUniformLocation(program_object, "use_tex");
   display->ogl_extras->tex_loc = glGetUniformLocation(program_object, "tex");
   display->ogl_extras->use_tex_matrix_loc = glGetUniformLocation(program_object, "use_tex_matrix");
   display->ogl_extras->tex_matrix_loc = glGetUniformLocation(program_object, "tex_matrix");
}

/* vi: set sts=3 sw=3 et: */
