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
 *      OpenGL drawing routines
 *
 *      By Elias Pschernig.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_opengl.h"

ALLEGRO_DEBUG_CHANNEL("opengl")

bool _al_opengl_set_blender(ALLEGRO_DISPLAY *ogl_disp)
{
   int op, src_color, dst_color, op_alpha, src_alpha, dst_alpha;
   const int blend_modes[8] = {
      GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
      GL_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_SRC_COLOR,
      GL_ONE_MINUS_DST_COLOR
   };
   const int blend_equations[3] = {
      GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT
   };

   (void)ogl_disp;

   al_get_separate_blender(&op, &src_color, &dst_color,
      &op_alpha, &src_alpha, &dst_alpha);
   /* glBlendFuncSeparate was only included with OpenGL 1.4 */
   /* (And not in OpenGL ES) */
#if !defined ALLEGRO_GP2XWIZ
#if !defined ALLEGRO_IPHONE && !defined ALLEGRO_ANDROID
   if (ogl_disp->ogl_extras->ogl_info.version >= _ALLEGRO_OPENGL_VERSION_1_4) {
#else
   if (ogl_disp->ogl_extras->ogl_info.version >= _ALLEGRO_OPENGL_VERSION_2_0) {
#endif
      glEnable(GL_BLEND);
      glBlendFuncSeparate(blend_modes[src_color], blend_modes[dst_color],
         blend_modes[src_alpha], blend_modes[dst_alpha]);
      if (ogl_disp->ogl_extras->ogl_info.version >= _ALLEGRO_OPENGL_VERSION_2_0) {
         glBlendEquationSeparate(
            blend_equations[op],
            blend_equations[op_alpha]);
      }
      else {
         glBlendEquation(blend_equations[op]);
      }
   }
   else {
      if (src_color == src_alpha && dst_color == dst_alpha) {
         glEnable(GL_BLEND);
         glBlendFunc(blend_modes[src_color], blend_modes[dst_color]);
      }
      else {
         ALLEGRO_ERROR("Blender unsupported with this OpenGL version (%d %d %d %d %d %d)\n",
            op, src_color, dst_color, op_alpha, src_alpha, dst_alpha);
         return false;
      }
   }
#else
   glEnable(GL_BLEND);
   glBlendFunc(blend_modes[src_color], blend_modes[dst_color]);
#endif
   return true;
}

/* These functions make drawing calls use shaders or the fixed pipeline
 * based on what the user has set up. FIXME: OpenGL only right now.
 */

static void vert_ptr_on(ALLEGRO_DISPLAY *display, int n, GLint t, int stride, void *v)
{
/* Only use this shader stuff with GLES2+ or equivalent */
   if (display->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
#ifndef ALLEGRO_NO_GLES2
      if (display->ogl_extras->pos_loc >= 0) {
         glVertexAttribPointer(display->ogl_extras->pos_loc, n, t, false, stride, v);
         glEnableVertexAttribArray(display->ogl_extras->pos_loc);
      }
#endif
   }
   else {
      glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(n, t, stride, v);
   }
}

static void vert_ptr_off(ALLEGRO_DISPLAY *display)
{
   if (display->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
#ifndef ALLEGRO_NO_GLES2
      if (display->ogl_extras->pos_loc >= 0) {
         glDisableVertexAttribArray(display->ogl_extras->pos_loc);
      }
#endif
   }
   else {
      glDisableClientState(GL_VERTEX_ARRAY);
   }
}

static void color_ptr_on(ALLEGRO_DISPLAY *display, int n, GLint t, int stride, void *v)
{
   if (display->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
#ifndef ALLEGRO_NO_GLES2
      if (display->ogl_extras->color_loc >= 0) {
         glVertexAttribPointer(display->ogl_extras->color_loc, n, t, false, stride, v);
         glEnableVertexAttribArray(display->ogl_extras->color_loc);
      }
#endif
   }
   else {
      glEnableClientState(GL_COLOR_ARRAY);
      glColorPointer(n, t, stride, v);
   }
}

static void color_ptr_off(ALLEGRO_DISPLAY *display)
{
   if (display->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
#ifndef ALLEGRO_NO_GLES2
      if (display->ogl_extras->color_loc >= 0) {
         glDisableVertexAttribArray(display->ogl_extras->color_loc);
      }
#endif
   }
   else {
      glDisableClientState(GL_COLOR_ARRAY);
   }
}

static void tex_ptr_on(ALLEGRO_DISPLAY *display, int n, GLint t, int stride, void *v)
{
   if (display->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
#ifndef ALLEGRO_NO_GLES2
      if (display->ogl_extras->texcoord_loc >= 0) {
         glVertexAttribPointer(display->ogl_extras->texcoord_loc, n, t, false, stride, v);
         glEnableVertexAttribArray(display->ogl_extras->texcoord_loc);
      }
#endif
   }
   else {
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(n, t, stride, v);
   }
}

static void tex_ptr_off(ALLEGRO_DISPLAY *display)
{
   if (display->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
#ifndef ALLEGRO_NO_GLES2
      if (display->ogl_extras->texcoord_loc >= 0) {
         glDisableVertexAttribArray(display->ogl_extras->texcoord_loc);
      }
#endif
   }
   else {
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
   }
}

/* Dummy implementation of clear. */
static void ogl_clear(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color)
{
   ALLEGRO_DISPLAY *ogl_disp = (void *)d;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_target;
   float r, g, b, a;
   
   if (target->parent) target = target->parent;
   
   ogl_target = target->extra;

   if ((!ogl_target->is_backbuffer &&
      ogl_disp->ogl_extras->opengl_target != target) ||
      target->locked) {
      _al_clear_memory(color);
      return;
   }

   al_unmap_rgba_f(*color, &r, &g, &b, &a);

   glClearColor(r, g, b, a);
   glClear(GL_COLOR_BUFFER_BIT);
}


static void ogl_draw_pixel(ALLEGRO_DISPLAY *d, float x, float y,
   ALLEGRO_COLOR *color)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_target = target->extra;
   GLfloat vert[2];
   GLfloat color_array[4];

   if ((!ogl_target->is_backbuffer &&
      d->ogl_extras->opengl_target != target) ||
      target->locked || !_al_opengl_set_blender(d))  {
      _al_draw_pixel_memory(target, x, y, color);
      return;
   }

   vert[0] = x;
   vert[1] = y;

   color_array[0] = color->r;
   color_array[1] = color->g;
   color_array[2] = color->b;
   color_array[3] = color->a;

   vert_ptr_on(d, 2, GL_FLOAT, 2*sizeof(float), vert);
   color_ptr_on(d, 4, GL_FLOAT, 4*sizeof(float), color_array);

   if (!_al_opengl_set_blender(d)) {
      return;
   }

   glDrawArrays(GL_POINTS, 0, 1);

   vert_ptr_off(d);
   color_ptr_off(d);
}

static void* ogl_prepare_vertex_cache(ALLEGRO_DISPLAY* disp, 
                                      int num_new_vertices)
{
   disp->num_cache_vertices += num_new_vertices;
   if (!disp->vertex_cache) {
      disp->vertex_cache = al_malloc(num_new_vertices * sizeof(ALLEGRO_OGL_BITMAP_VERTEX));
      
      disp->vertex_cache_size = num_new_vertices;
   } else if (disp->num_cache_vertices > disp->vertex_cache_size) {
      disp->vertex_cache = al_realloc(disp->vertex_cache, 
                              2 * disp->num_cache_vertices * sizeof(ALLEGRO_OGL_BITMAP_VERTEX));
                              
      disp->vertex_cache_size = 2 * disp->num_cache_vertices;
   }
   return (ALLEGRO_OGL_BITMAP_VERTEX*)disp->vertex_cache + 
         (disp->num_cache_vertices - num_new_vertices);
}

static void ogl_flush_vertex_cache(ALLEGRO_DISPLAY* disp)
{
   GLboolean on = false;
   GLuint current_texture;
   if (!disp->vertex_cache)
      return;
   if (disp->num_cache_vertices == 0)
      return;

   if (disp->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
#ifndef ALLEGRO_NO_GLES2
      if (disp->ogl_extras->use_tex_loc >= 0) {
         glUniform1i(disp->ogl_extras->use_tex_loc, 1);
      }
      if (disp->ogl_extras->use_tex_matrix_loc >= 0) {
         glUniform1i(disp->ogl_extras->use_tex_matrix_loc, 0);
      }
#endif
   }
   else {
      glGetBooleanv(GL_TEXTURE_2D, &on);
      if (!on) {
         glEnable(GL_TEXTURE_2D);
      }
   }
   
   glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&current_texture);
   if (current_texture != disp->cache_texture) {
      if (disp->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
#ifndef ALLEGRO_NO_GLES2
         /* Use texture unit 0 */
         glActiveTexture(GL_TEXTURE0);
         if (disp->ogl_extras->tex_loc >= 0)
            glUniform1i(disp->ogl_extras->tex_loc, 0);
#endif
      }
      glBindTexture(GL_TEXTURE_2D, disp->cache_texture);
   }

///* Figure out what this is here for
#ifdef ALLEGRO_IPHONE
      if (!_al_opengl_set_blender(disp)) {
         return;
      }
#endif
//*/

   vert_ptr_on(disp, 2, GL_FLOAT, sizeof(ALLEGRO_OGL_BITMAP_VERTEX), (char *)(disp->vertex_cache) + offsetof(ALLEGRO_OGL_BITMAP_VERTEX, x));
   tex_ptr_on(disp, 2, GL_FLOAT, sizeof(ALLEGRO_OGL_BITMAP_VERTEX), (char*)(disp->vertex_cache) + offsetof(ALLEGRO_OGL_BITMAP_VERTEX, tx));
   color_ptr_on(disp, 4, GL_FLOAT, sizeof(ALLEGRO_OGL_BITMAP_VERTEX), (char*)(disp->vertex_cache) + offsetof(ALLEGRO_OGL_BITMAP_VERTEX, r));

   glDrawArrays(GL_TRIANGLES, 0, disp->num_cache_vertices);

   vert_ptr_off(disp);
   tex_ptr_off(disp);
   color_ptr_off(disp);
   
   disp->num_cache_vertices = 0;

   if (disp->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
#ifndef ALLEGRO_NO_GLES2
      if (disp->ogl_extras->use_tex_loc >= 0)
         glUniform1i(disp->ogl_extras->use_tex_loc, 0);
#endif
   }
   else {
      if (!on) {
         glDisable(GL_TEXTURE_2D);
      }
   }
}

static void ogl_update_transformation(ALLEGRO_DISPLAY* disp,
   ALLEGRO_BITMAP *target)
{
   ALLEGRO_TRANSFORM tmp;
   
   al_copy_transform(&tmp, &target->transform);

   if (target->parent) {
      /* Sub-bitmaps have an additional offset. */
      al_translate_transform(&tmp, target->xofs, target->yofs);
   }

   if (disp->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
#ifndef ALLEGRO_NO_GLES2
      GLuint program_object = disp->ogl_extras->program_object;
      GLint handle;
      
      // FIXME: In al_create_display we have no shader yet
      if (program_object == 0)
         return;

      al_copy_transform(&disp->view_transform, &tmp);

      al_compose_transform(&tmp, &disp->proj_transform);

      handle = glGetUniformLocation(program_object, "projview_matrix");
      if (handle >= 0) {
         glUniformMatrix4fv(handle, 1, GL_FALSE, (float *)tmp.m);
      }
#endif
   }
   else {
      glMatrixMode(GL_MODELVIEW);
      glLoadMatrixf((float *)tmp.m);
   }
}

static void ogl_set_projection(ALLEGRO_DISPLAY *d)
{
   if (d->flags & ALLEGRO_USE_PROGRAMMABLE_PIPELINE) {
#ifndef ALLEGRO_NO_GLES2
      GLuint program_object = d->ogl_extras->program_object;
      GLint handle;

      // FIXME: In al_create_display we have no shader yet
      if (program_object == 0)
         return;

      handle = glGetUniformLocation(program_object, "projview_matrix");
      if (handle >= 0) {
         ALLEGRO_TRANSFORM t;
         al_copy_transform(&t, &d->view_transform);
         al_compose_transform(&t, &d->proj_transform);
         glUniformMatrix4fv(handle, 1, false, (float *)t.m);
      }
#endif
   }
   else {
      glMatrixMode(GL_PROJECTION);
      glLoadMatrixf((float *)d->proj_transform.m);
      glMatrixMode(GL_MODELVIEW);
   }
}

/* Add drawing commands to the vtable. */
void _al_ogl_add_drawing_functions(ALLEGRO_DISPLAY_INTERFACE *vt)
{
   vt->clear = ogl_clear;
   vt->draw_pixel = ogl_draw_pixel;
   
   vt->flush_vertex_cache = ogl_flush_vertex_cache;
   vt->prepare_vertex_cache = ogl_prepare_vertex_cache;
   vt->update_transformation = ogl_update_transformation;
   vt->set_projection = ogl_set_projection;
}

/* vim: set sts=3 sw=3 et: */
