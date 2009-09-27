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
 *      OpenGL implementation of some of the primitive routines.
 *
 *
 *      By Pavel Sountsov.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro5.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_prim_opengl.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include "allegro5/platform/alplatf.h"
#include "allegro5/internal/aintern_prim.h"

#ifdef ALLEGRO_CFG_OPENGL

#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_opengl.h"

static void setup_blending(void)
{
   const int blend_modes[4] = {
      GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA
   };
   int src_color, dst_color, src_alpha, dst_alpha;
   ALLEGRO_DISPLAY *d = al_get_current_display();
   (void)d;

   al_get_separate_blender(&src_color, &dst_color, &src_alpha,
      &dst_alpha, NULL);
#if !defined ALLEGRO_GP2XWIZ && !defined ALLEGRO_IPHONE
   if (d->ogl_extras->ogl_info.version >= 1.4) {
      glEnable(GL_BLEND);
      glBlendFuncSeparate(blend_modes[src_color],
         blend_modes[dst_color], blend_modes[src_alpha],
         blend_modes[dst_alpha]);
   }
   else {
      if (src_color == src_alpha && dst_color == dst_alpha) {
#endif
         glEnable(GL_BLEND);
         glBlendFunc(blend_modes[src_color], blend_modes[dst_color]);
#if !defined ALLEGRO_GP2XWIZ && !defined ALLEGRO_IPHONE
      }
   }
#endif
}

static void setup_state(const char* vtxs, const ALLEGRO_VERTEX_DECL* decl, ALLEGRO_BITMAP* texture)
{
   if(decl) {
      ALLEGRO_VERTEX_ELEMENT* e;
      e = &decl->elements[ALLEGRO_PRIM_POSITION];
      if(e->attribute) {
         int ncoord = 0;
         GLenum type = 0;

         if(!glIsEnabled(GL_VERTEX_ARRAY))
            glEnableClientState(GL_VERTEX_ARRAY);

         switch(e->storage) {
            case ALLEGRO_PRIM_FLOAT_2:
               ncoord = 2;
               type = GL_FLOAT;
            break;
            case ALLEGRO_PRIM_FLOAT_3:
               ncoord = 3;
               type = GL_FLOAT;
            break;
            case ALLEGRO_PRIM_SHORT_2:
               ncoord = 2;
               type = GL_SHORT;
            break;
         }
         glVertexPointer(ncoord, type, decl->stride, vtxs + e->offset);
      } else {
         glDisableClientState(GL_VERTEX_ARRAY);
      }

      e = &decl->elements[ALLEGRO_PRIM_TEX_COORD];
      if(!e->attribute)
         e = &decl->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL];
      if(texture && e->attribute) {
         GLenum type = 0;

         if(!glIsEnabled(GL_TEXTURE_COORD_ARRAY))
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);

         switch(e->storage) {
            case ALLEGRO_PRIM_FLOAT_2:
            case ALLEGRO_PRIM_FLOAT_3:
               type = GL_FLOAT;
            break;
            case ALLEGRO_PRIM_SHORT_2:
               type = GL_SHORT;
            break;
         }
         glTexCoordPointer(2, type, decl->stride, vtxs + e->offset);
      } else {
         glDisableClientState(GL_TEXTURE_COORD_ARRAY);
      }

      e = &decl->elements[ALLEGRO_PRIM_COLOR_ATTR];
      if(e->attribute) {
         if(!glIsEnabled(GL_COLOR_ARRAY))
            glEnableClientState(GL_COLOR_ARRAY);

         glColorPointer(4, GL_FLOAT, decl->stride, vtxs + e->offset + offsetof(ALLEGRO_PRIM_COLOR, r));
      } else {
         glDisableClientState(GL_COLOR_ARRAY);
         glColor4f(1, 1, 1, 1);
      }
   } else {
      const ALLEGRO_VERTEX* vtx = (const ALLEGRO_VERTEX*)vtxs;
   
      if(!glIsEnabled(GL_COLOR_ARRAY))
         glEnableClientState(GL_COLOR_ARRAY);
      if(!glIsEnabled(GL_VERTEX_ARRAY))
         glEnableClientState(GL_VERTEX_ARRAY);
      if (!glIsEnabled(GL_TEXTURE_COORD_ARRAY))
         glEnableClientState(GL_TEXTURE_COORD_ARRAY);

      glVertexPointer(2, GL_FLOAT, sizeof(ALLEGRO_VERTEX), &vtx[0].x);
      glColorPointer(4, GL_FLOAT, sizeof(ALLEGRO_VERTEX), &vtx[0].color.r);
      glTexCoordPointer(2, GL_FLOAT, sizeof(ALLEGRO_VERTEX), &vtx[0].u);
   }

   if (texture) {
      ALLEGRO_BITMAP_OGL *ogl_bitmap = (void *)texture; 
      GLuint current_texture;
      float t = (float)al_get_bitmap_height(texture) / ogl_bitmap->true_h;
      float mat[4][4] = {
         {1,  0, 0, 0},
         {0, -1, 0, 0},
         {0,  0, 1, 0},
         {0,  t, 0, 1}
      };
      (void)current_texture;

      if(decl) {
         if(decl->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL].attribute) {
            mat[0][0] = 1.0f / ogl_bitmap->true_w;
            mat[1][1] = -1.0f / ogl_bitmap->true_h;
         } else {
            mat[0][0] = (float)al_get_bitmap_width(texture) / ogl_bitmap->true_w;
            mat[1][1] = -(float)al_get_bitmap_height(texture) / ogl_bitmap->true_h;
         }
      } else {
         mat[0][0] = 1.0f / ogl_bitmap->true_w;
         mat[1][1] = -1.0f / ogl_bitmap->true_h;
      }

      glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&current_texture);
      if (current_texture != ogl_bitmap->texture) {
         glBindTexture(GL_TEXTURE_2D, ogl_bitmap->texture);
      }

      glMatrixMode(GL_TEXTURE);
      glLoadMatrixf(mat[0]);
      glMatrixMode(GL_MODELVIEW);
   } else {
      glBindTexture(GL_TEXTURE_2D, 0);
   }
}

int _al_draw_prim_opengl(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, int start, int end, int type)
{   
#ifdef ALLEGRO_CFG_OPENGL

   int num_primitives = 0;
   ALLEGRO_DISPLAY *ogl_disp = al_get_current_display();
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target = (void *)target;
   const void* vtx;
   int stride = decl ? decl->stride : (int)sizeof(ALLEGRO_VERTEX);
   int num_vtx;
  
   if ((!ogl_target->is_backbuffer && ogl_disp->ogl_extras->opengl_target != ogl_target) || al_is_bitmap_locked(target)) {
      return _al_draw_prim_soft(texture, vtxs, decl, start, end, type);
   }
   
   vtx = (const char*)vtxs + start * stride;
   num_vtx = end - start;
   
   if(texture) {
      glEnable(GL_TEXTURE_2D);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   }

   setup_blending();
   setup_state(vtx, decl, texture);

   switch (type) {
      case ALLEGRO_PRIM_LINE_LIST: {
         glDrawArrays(GL_LINES, 0, num_vtx);
         num_primitives = num_vtx / 2;
         break;
      };
      case ALLEGRO_PRIM_LINE_STRIP: {
         glDrawArrays(GL_LINE_STRIP, 0, num_vtx);
         num_primitives = num_vtx - 1;
         break;
      };
      case ALLEGRO_PRIM_LINE_LOOP: {
         glDrawArrays(GL_LINE_LOOP, 0, num_vtx);
         num_primitives = num_vtx;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_LIST: {
         glDrawArrays(GL_TRIANGLES, 0, num_vtx);
         num_primitives = num_vtx / 3;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_STRIP: {
         glDrawArrays(GL_TRIANGLE_STRIP, 0, num_vtx);
         num_primitives = num_vtx - 2;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_FAN: {
         glDrawArrays(GL_TRIANGLE_FAN, 0, num_vtx);
         num_primitives = num_vtx - 2;
         break;
      };
      case ALLEGRO_PRIM_POINT_LIST: {
         glDrawArrays(GL_POINTS, 0, num_vtx);
         num_primitives = num_vtx;
         break;
      };
   }

   if(texture) {
      glDisable(GL_TEXTURE_2D);
      glMatrixMode(GL_TEXTURE);
      glLoadIdentity();
      glMatrixMode(GL_MODELVIEW);
   }

   return num_primitives;
#else
   (void)texture;
   (void)vtxs;
   (void)start;
   (void)end;
   (void)type;
   (void)decl;

   return 0;
#endif
}

int _al_draw_prim_indexed_opengl(ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, const int* indices, int num_vtx, int type)
{   
#ifdef ALLEGRO_CFG_OPENGL

   int num_primitives = 0;
   ALLEGRO_DISPLAY *ogl_disp = al_get_current_display();
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_OGL *ogl_target = (void *)target;
   const void* vtx;
   const void* idx = indices;
   GLenum idx_size = GL_UNSIGNED_INT;

#if defined ALLEGRO_GP2XWIZ || defined ALLEGRO_IPHONE
   GLushort ind[num_vtx];
   int ii;
#endif

   if ((!ogl_target->is_backbuffer && ogl_disp->ogl_extras->opengl_target != ogl_target) || al_is_bitmap_locked(target)) {
      return _al_draw_prim_indexed_soft(texture, decl, vtxs, indices, num_vtx, type);
   }
   
   vtx = vtxs;

   if(texture) {
      glEnable(GL_TEXTURE_2D);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   }
   
   setup_blending();
   setup_state(vtx, decl, texture);
  
#if defined ALLEGRO_GP2XWIZ || defined ALLEGRO_IPHONE
   for (ii = 0; ii < num_vtx; ii++) {
      ind[ii] = (GLushort)indices[ii];
   }
   idx = ind;
   idx_size = GL_UNSIGNED_SHORT;
#endif

   switch (type) {
      case ALLEGRO_PRIM_LINE_LIST: {
         glDrawElements(GL_LINES, num_vtx, idx_size, idx);
         num_primitives = num_vtx / 2;
         break;
      };
      case ALLEGRO_PRIM_LINE_STRIP: {
         glDrawElements(GL_LINE_STRIP, num_vtx, idx_size, idx);
         num_primitives = num_vtx - 1;
         break;
      };
      case ALLEGRO_PRIM_LINE_LOOP: {
         glDrawElements(GL_LINE_LOOP, num_vtx, idx_size, idx);
         num_primitives = num_vtx;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_LIST: {
         glDrawElements(GL_TRIANGLES, num_vtx, idx_size, idx);
         num_primitives = num_vtx / 3;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_STRIP: {
         glDrawElements(GL_TRIANGLE_STRIP, num_vtx, idx_size, idx);
         num_primitives = num_vtx - 2;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_FAN: {
         glDrawElements(GL_TRIANGLE_FAN, num_vtx, idx_size, idx);
         num_primitives = num_vtx - 2;
         break;
      };
      case ALLEGRO_PRIM_POINT_LIST: {
         glDrawElements(GL_POINTS, num_vtx, idx_size, idx);
         num_primitives = num_vtx;
         break;
      };
   }

   if(texture) {
      glDisable(GL_TEXTURE_2D);
      glMatrixMode(GL_TEXTURE);
      glLoadIdentity();
      glMatrixMode(GL_MODELVIEW);
   }

   return num_primitives;
#else
   (void)texture;
   (void)vtxs;
   (void)start;
   (void)end;
   (void)type;
   (void)decl;

   return 0;
#endif
}

#endif

void _al_use_transform_opengl(const ALLEGRO_TRANSFORM* trans)
{
#ifdef ALLEGRO_CFG_OPENGL
   glMatrixMode(GL_MODELVIEW);
   glLoadMatrixf((float*)(trans->m));
#endif
}
