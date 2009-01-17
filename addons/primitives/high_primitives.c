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
 *      Some high level routines, provided for user's convinience.
 *
 *
 *      By Pavel Sountsov.
 *
 *
 *      Bezier spline plotter By Seymour Shlien.
 *
 *      Optimised version by Sven Sandberg.
 *
 *      I'm not sure wether or not we still use the Castelau Algorithm
 *      described in the book :o)
 *
 *      Interactive Computer Graphics
 *      by Peter Burger and Duncan Gillies
 *      Addison-Wesley Publishing Co 1989
 *      ISBN 0-201-17439-1
 *
 *      The 4 th order Bezier curve is a cubic curve passing
 *      through the first and fourth point. The curve does
 *      not pass through the middle two points. They are merely
 *      guide points which control the shape of the curve. The
 *      curve is tangent to the lines joining points 1 and 2
 *      and points 3 and 4.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/a5_primitives.h"

static ALLEGRO_VBUFFER* cache_buffer;
static float* cache_point_buffer;

static void verify_cache(void)
{
   if (!cache_buffer) {
      cache_buffer = al_create_vbuff(ALLEGRO_VBUFF_CACHE_SIZE, ALLEGRO_VBUFFER_SOFT | ALLEGRO_VBUFFER_WRITE | ALLEGRO_VBUFFER_READ);
      cache_point_buffer = malloc(2 * sizeof(float) * ALLEGRO_VBUFF_CACHE_SIZE);
   }
}

void al_draw_line_ex(float x1, float y1, float x2, float y2, ALLEGRO_COLOR color, float thickness)
{
   verify_cache();
   if (thickness > 0) {
      int ii;
      float len = hypotf(x2 - x1, y2 - y1);
      if (len == 0)
         return;
      float tx, ty;
      tx = 0.5f * thickness * (y2 - y1) / len;
      ty = 0.5f * thickness * -(x2 - x1) / len;
      
      if (!al_lock_vbuff_range(cache_buffer, 0, 4, ALLEGRO_VBUFFER_WRITE))
         return;
         
      al_set_vbuff_pos(cache_buffer, 0, x1 - tx, y1 - ty, 0);
      al_set_vbuff_pos(cache_buffer, 1, x1 + tx, y1 + ty, 0);
      al_set_vbuff_pos(cache_buffer, 2, x2 + tx, y2 + ty, 0);
      al_set_vbuff_pos(cache_buffer, 3, x2 - tx, y2 - ty, 0);
      
      for (ii = 0; ii < 4; ii++)
         al_set_vbuff_color(cache_buffer, ii, color);
         
      al_unlock_vbuff(cache_buffer);
      
      al_draw_prim(cache_buffer, 0, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN);
      
   } else {
      if (!al_lock_vbuff_range(cache_buffer, 0, 2, ALLEGRO_VBUFFER_WRITE))
         return;
         
      al_set_vbuff_pos(cache_buffer, 0, x1, y1, 0);
      al_set_vbuff_pos(cache_buffer, 1, x2, y2, 0);
      
      al_set_vbuff_color(cache_buffer, 0, color);
      al_set_vbuff_color(cache_buffer, 1, color);
      
      al_unlock_vbuff(cache_buffer);
      
      al_draw_prim(cache_buffer, 0, 0, 2, ALLEGRO_PRIM_LINE_LIST);
   }
}

void al_draw_triangle(float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR color, float thickness)
{
   verify_cache();
   if (thickness > 0) {
      if (!al_lock_vbuff_range(cache_buffer, 0, 8, ALLEGRO_VBUFFER_WRITE))
         return;
      int ii;
      
      float side1 = hypotf(x2 - x1, y2 - y1);
      float side2 = hypotf(x3 - x1, y3 - y1);
      float side3 = hypotf(x3 - x2, y3 - y2);
      
      float p = side1 + side2 + side3;
      float s = p / 2.0f;
      if (s < 0.00001f)
         return;
         
      float incircle_rad = sqrtf((s - side1) * (s - side2) * (s - side3) / s);
      
      if (incircle_rad < 0.00001f)
         return;
         
      float outer_frac = (incircle_rad + thickness / 2) / incircle_rad;
      float inner_frac = (incircle_rad - thickness / 2) / incircle_rad;
      
      float incenter_x = (side1 * x3 + side2 * x2 + side3 * x1) / p;
      float incenter_y = (side1 * y3 + side2 * y2 + side3 * y1) / p;
      
      float vert_x1 = x1 - incenter_x;
      float vert_y1 = y1 - incenter_y;
      float vert_x2 = x2 - incenter_x;
      float vert_y2 = y2 - incenter_y;
      float vert_x3 = x3 - incenter_x;
      float vert_y3 = y3 - incenter_y;
      
      al_set_vbuff_pos(cache_buffer, 0, incenter_x + vert_x1 * outer_frac, incenter_y + vert_y1 * outer_frac, 0);
      al_set_vbuff_pos(cache_buffer, 1, incenter_x + vert_x1 * inner_frac, incenter_y + vert_y1 * inner_frac, 0);
      
      al_set_vbuff_pos(cache_buffer, 2, incenter_x + vert_x2 * outer_frac, incenter_y + vert_y2 * outer_frac, 0);
      al_set_vbuff_pos(cache_buffer, 3, incenter_x + vert_x2 * inner_frac, incenter_y + vert_y2 * inner_frac, 0);
      
      al_set_vbuff_pos(cache_buffer, 4, incenter_x + vert_x3 * outer_frac, incenter_y + vert_y3 * outer_frac, 0);
      al_set_vbuff_pos(cache_buffer, 5, incenter_x + vert_x3 * inner_frac, incenter_y + vert_y3 * inner_frac, 0);
      
      al_set_vbuff_pos(cache_buffer, 6, incenter_x + vert_x1 * outer_frac, incenter_y + vert_y1 * outer_frac, 0);
      al_set_vbuff_pos(cache_buffer, 7, incenter_x + vert_x1 * inner_frac, incenter_y + vert_y1 * inner_frac, 0);
      
      for (ii = 0; ii < 8; ii++)
         al_set_vbuff_color(cache_buffer, ii, color);
         
      al_unlock_vbuff(cache_buffer);
      
      al_draw_prim(cache_buffer, 0, 0, 8, ALLEGRO_PRIM_TRIANGLE_STRIP);
      
   } else {
      if (!al_lock_vbuff_range(cache_buffer, 0, 3, ALLEGRO_VBUFFER_WRITE))
         return;
         
      al_set_vbuff_pos(cache_buffer, 0, x1, y1, 0);
      al_set_vbuff_pos(cache_buffer, 1, x2, y2, 0);
      al_set_vbuff_pos(cache_buffer, 2, x3, y3, 0);
      
      al_set_vbuff_color(cache_buffer, 0, color);
      al_set_vbuff_color(cache_buffer, 1, color);
      al_set_vbuff_color(cache_buffer, 2, color);
      
      al_unlock_vbuff(cache_buffer);
      
      al_draw_prim(cache_buffer, 0, 0, 3, ALLEGRO_PRIM_LINE_LOOP);
   }
}

void al_draw_filled_triangle(float x1, float y1, float x2, float y2, float x3, float y3, ALLEGRO_COLOR color)
{
   verify_cache();
   if (!al_lock_vbuff_range(cache_buffer, 0, 3, ALLEGRO_VBUFFER_WRITE))
      return;
      
   al_set_vbuff_pos(cache_buffer, 0, x1, y1, 0);
   al_set_vbuff_pos(cache_buffer, 1, x2, y2, 0);
   al_set_vbuff_pos(cache_buffer, 2, x3, y3, 0);
   
   al_set_vbuff_color(cache_buffer, 0, color);
   al_set_vbuff_color(cache_buffer, 1, color);
   al_set_vbuff_color(cache_buffer, 2, color);
   
   al_unlock_vbuff(cache_buffer);
   
   al_draw_prim(cache_buffer, 0, 0, 3, ALLEGRO_PRIM_TRIANGLE_LIST);
}

void al_draw_rectangle_ex(float x1, float y1, float x2, float y2, ALLEGRO_COLOR color, float thickness)
{
   verify_cache();
   int ii;
   if (thickness > 0) {
      if (!al_lock_vbuff_range(cache_buffer, 0, 10, ALLEGRO_VBUFFER_WRITE))
         return;
         
      float t = thickness / 2;
      
      al_set_vbuff_pos(cache_buffer, 0, x1 - t, y1 - t, 0);
      al_set_vbuff_pos(cache_buffer, 1, x1 + t, y1 + t, 0);
      al_set_vbuff_pos(cache_buffer, 2, x2 + t, y1 - t, 0);
      al_set_vbuff_pos(cache_buffer, 3, x2 - t, y1 + t, 0);
      al_set_vbuff_pos(cache_buffer, 4, x2 + t, y2 + t, 0);
      al_set_vbuff_pos(cache_buffer, 5, x2 - t, y2 - t, 0);
      al_set_vbuff_pos(cache_buffer, 6, x1 - t, y2 + t, 0);
      al_set_vbuff_pos(cache_buffer, 7, x1 + t, y2 - t, 0);
      al_set_vbuff_pos(cache_buffer, 8, x1 - t, y1 - t, 0);
      al_set_vbuff_pos(cache_buffer, 9, x1 + t, y1 + t, 0);
      
      for (ii = 0; ii < 10; ii++)
         al_set_vbuff_color(cache_buffer, ii, color);
         
      al_unlock_vbuff(cache_buffer);
      
      al_draw_prim(cache_buffer, 0, 0, 10, ALLEGRO_PRIM_TRIANGLE_STRIP);
   } else {
      if (!al_lock_vbuff_range(cache_buffer, 0, 4, ALLEGRO_VBUFFER_WRITE))
         return;
         
      al_set_vbuff_pos(cache_buffer, 0, x1, y1, 0);
      al_set_vbuff_pos(cache_buffer, 1, x2, y1, 0);
      al_set_vbuff_pos(cache_buffer, 2, x2, y2, 0);
      al_set_vbuff_pos(cache_buffer, 3, x1, y2, 0);
      
      for (ii = 0; ii < 4; ii++)
         al_set_vbuff_color(cache_buffer, ii, color);
         
      al_unlock_vbuff(cache_buffer);
      
      al_draw_prim(cache_buffer, 0, 0, 4, ALLEGRO_PRIM_LINE_LOOP);
   }
}

void al_draw_filled_rectangle(float x1, float y1, float x2, float y2, ALLEGRO_COLOR color)
{
   verify_cache();
   if (!al_lock_vbuff_range(cache_buffer, 0, 4, ALLEGRO_VBUFFER_WRITE))
      return;
      
   al_set_vbuff_pos(cache_buffer, 0, x1, y1, 0);
   al_set_vbuff_pos(cache_buffer, 1, x2, y1, 0);
   al_set_vbuff_pos(cache_buffer, 2, x2, y2, 0);
   al_set_vbuff_pos(cache_buffer, 3, x1, y2, 0);
   
   al_set_vbuff_color(cache_buffer, 0, color);
   al_set_vbuff_color(cache_buffer, 1, color);
   al_set_vbuff_color(cache_buffer, 2, color);
   al_set_vbuff_color(cache_buffer, 3, color);
   
   al_unlock_vbuff(cache_buffer);
   
   al_draw_prim(cache_buffer, 0, 0, 4, ALLEGRO_PRIM_TRIANGLE_FAN);
}

void al_calculate_arc(ALLEGRO_VBUFFER* vbuff, float cx, float cy, float rx, float ry, float start_theta, float delta_theta, float thickness, int start, int num_segments)
{
   ASSERT(vbuff);
   
   int need_unlock = 0;
   float theta;
   float tangetial_factor;
   float radial_factor;
   float x, y;
   int ii;
   
   if (thickness > 0.0f) {
      theta = delta_theta / ((float)(num_segments) - 1);
      tangetial_factor = tanf(theta);
      radial_factor = cosf(theta);
      x = cosf(start_theta);
      y = sinf(start_theta);
      
      if (al_vbuff_is_locked(vbuff)) {
         if (!al_vbuff_range_is_locked(vbuff, start, start + 2 * num_segments))
            return;
      } else {
         if (!al_lock_vbuff_range(vbuff, start, start + 2 * num_segments, ALLEGRO_VBUFFER_WRITE))
            return;
         need_unlock = 1;
      }
      
      
      if (rx == ry) {
         /*
         The circle case is particularly simple
         */
         float r1 = rx - thickness / 2.0f;
         float r2 = rx + thickness / 2.0f;
         for (ii = 0; ii < num_segments; ii ++) {
            al_set_vbuff_pos(vbuff, 2 * ii + start, r1 * x + cx, r1 * y + cy, 0);
            al_set_vbuff_pos(vbuff, 2 * ii + 1 + start, r2 * x + cx, r2 * y + cy, 0);
            
            float tx = -y;
            float ty = x;
            
            x += tx * tangetial_factor;
            y += ty * tangetial_factor;
            
            x *= radial_factor;
            y *= radial_factor;
         }
      } else {
         if (rx != 0 && !ry == 0) {
            for (ii = 0; ii < num_segments; ii++) {
               float denom = hypotf(ry * x, rx * y);
               float nx = thickness / 2 * ry * x / denom;
               float ny = thickness / 2 * rx * y / denom;
               
               al_set_vbuff_pos(vbuff, 2 * ii + start, rx * x + cx + nx, ry * y + cy + ny, 0);
               al_set_vbuff_pos(vbuff, 2 * ii + 1 + start, rx * x + cx - nx, ry * y + cy - ny, 0);
               
               float tx = -y;
               float ty = x;
               
               x += tx * tangetial_factor;
               y += ty * tangetial_factor;
               
               x *= radial_factor;
               y *= radial_factor;
            }
         }
      }
   } else {
      theta = delta_theta / ((float)num_segments - 1);
      tangetial_factor = tanf(theta);
      radial_factor = cosf(theta);
      x = cosf(start_theta);
      y = sinf(start_theta);
      
      if (al_vbuff_is_locked(vbuff)) {
         if (!al_vbuff_range_is_locked(vbuff, start, start + num_segments))
            return;
      } else {
         if (!al_lock_vbuff_range(vbuff, start, start + num_segments, ALLEGRO_VBUFFER_WRITE))
            return;
         need_unlock = 1;
      }
      
      for (ii = 0; ii < num_segments; ii++) {
         al_set_vbuff_pos(vbuff, ii + start, rx * x + cx, ry * y + cy, 0);
         
         float tx = -y;
         float ty = x;
         
         x += tx * tangetial_factor;
         y += ty * tangetial_factor;
         
         x *= radial_factor;
         y *= radial_factor;
      }
   }
   
   if (need_unlock)
      al_unlock_vbuff(vbuff);
}

void al_draw_ellipse(float cx, float cy, float rx, float ry, ALLEGRO_COLOR color, float thickness)
{
   verify_cache();
   if (thickness > 0) {
      int num_segments = ALLEGRO_PRIM_QUALITY * sqrtf((rx + ry) / 2.0f);
      int ii;
      
      if (2 * num_segments >= ALLEGRO_VBUFF_CACHE_SIZE) {
         num_segments = (ALLEGRO_VBUFF_CACHE_SIZE - 1) / 2;
      }
      
      al_calculate_arc(cache_buffer, cx, cy, rx, ry, 0, AL_PI * 2, thickness, 0, num_segments);
      for (ii = 0; ii < 2 * num_segments; ii++)
         al_set_vbuff_color(cache_buffer, ii, color);
         
      al_draw_prim(cache_buffer, 0, 0, 2 * num_segments, ALLEGRO_PRIM_TRIANGLE_STRIP);
   } else {
      int num_segments = ALLEGRO_PRIM_QUALITY * sqrtf((rx + ry) / 2.0f);
      int ii;
      
      if (num_segments >= ALLEGRO_VBUFF_CACHE_SIZE) {
         num_segments = ALLEGRO_VBUFF_CACHE_SIZE - 1;
      }
      
      al_calculate_arc(cache_buffer, cx, cy, rx, ry, 0, AL_PI * 2, 0, 0, num_segments);
      for (ii = 0; ii < num_segments; ii++)
         al_set_vbuff_color(cache_buffer, ii, color);
         
      al_draw_prim(cache_buffer, 0, 0, num_segments - 1, ALLEGRO_PRIM_LINE_LOOP);
   }
}

void al_draw_filled_ellipse(float cx, float cy, float rx, float ry, ALLEGRO_COLOR color)
{
   verify_cache();
   
   int num_segments = ALLEGRO_PRIM_QUALITY * sqrtf((rx + ry) / 2.0f);
   int ii;
   
   if (num_segments >= ALLEGRO_VBUFF_CACHE_SIZE) {
      num_segments = ALLEGRO_VBUFF_CACHE_SIZE - 1;
   }
   
   if (!al_lock_vbuff_range(cache_buffer, 0, num_segments + 1, ALLEGRO_VBUFFER_WRITE))
      return;
      
   al_calculate_arc(cache_buffer, cx, cy, rx, ry, 0, AL_PI * 2, 0, 1, num_segments);
   al_set_vbuff_pos(cache_buffer, 0, cx, cy, 0);
   
   for (ii = 0; ii < num_segments + 1; ii++)
      al_set_vbuff_color(cache_buffer, ii, color);
      
   al_unlock_vbuff(cache_buffer);
   
   al_draw_prim(cache_buffer, 0, 0, num_segments + 1, ALLEGRO_PRIM_TRIANGLE_FAN);
}

void al_draw_circle(float cx, float cy, float r, ALLEGRO_COLOR color, float thickness)
{
   al_draw_ellipse(cx, cy, r, r, color, thickness);
}

void al_draw_filled_circle(float cx, float cy, float r, ALLEGRO_COLOR color)
{
   al_draw_filled_ellipse(cx, cy, r, r, color);
}

void al_draw_arc(float cx, float cy, float r, float start_theta, float delta_theta, ALLEGRO_COLOR color, float thickness)
{
   verify_cache();
   if (thickness > 0) {
      int num_segments = delta_theta / (2 * AL_PI) * ALLEGRO_PRIM_QUALITY * sqrtf(r);
      int ii;
      
      if (2 * num_segments >= ALLEGRO_VBUFF_CACHE_SIZE) {
         num_segments = (ALLEGRO_VBUFF_CACHE_SIZE - 1) / 2;
      }
      
      al_calculate_arc(cache_buffer, cx, cy, r, r, start_theta, delta_theta, thickness, 0, num_segments);
      
      for (ii = 0; ii < 2 *   num_segments; ii++) {
         al_set_vbuff_color(cache_buffer, ii, color);
      }
      
      al_draw_prim(cache_buffer, 0, 0, 2 * num_segments, ALLEGRO_PRIM_TRIANGLE_STRIP);
   } else {
      int num_segments = delta_theta / (2 * AL_PI) * ALLEGRO_PRIM_QUALITY * sqrtf(r);
      int ii;
      
      if (num_segments >= ALLEGRO_VBUFF_CACHE_SIZE) {
         num_segments = ALLEGRO_VBUFF_CACHE_SIZE - 1;
      }
      
      al_calculate_arc(cache_buffer, cx, cy, r, r, start_theta, delta_theta, 0, 0, num_segments);
      
      for (ii = 0; ii < num_segments; ii++) {
         al_set_vbuff_color(cache_buffer, ii, color);
      }
      
      al_draw_prim(cache_buffer, 0, 0, num_segments, ALLEGRO_PRIM_LINE_STRIP);
   }
}

void al_calculate_spline(ALLEGRO_VBUFFER* vbuff, float points[8], float thickness, int start, int num_segments)
{
   ASSERT(vbuff);
   ASSERT(points);
   
   /* Derivatives of x(t) and y(t). */
   float x, dx, ddx, dddx;
   float y, dy, ddy, dddy;
   int ii = 0;
   
   /* Temp variables used in the setup. */
   float dt, dt2, dt3;
   float xdt2_term, xdt3_term;
   float ydt2_term, ydt3_term;
   
   dt = 1.0 / (num_segments - 1);
   dt2 = (dt * dt);
   dt3 = (dt2 * dt);
   
   /* x coordinates. */
   xdt2_term = 3 * (points[4] - 2 * points[2] + points[0]);
   xdt3_term = points[6] + 3 * (-points[4] + points[2]) - points[0];
   
   xdt2_term = dt2 * xdt2_term;
   xdt3_term = dt3 * xdt3_term;
   
   dddx = 6 * xdt3_term;
   ddx = -6 * xdt3_term + 2 * xdt2_term;
   dx = xdt3_term - xdt2_term + 3 * dt * (points[2] - points[0]);
   x = points[0];
   
   /* y coordinates. */
   ydt2_term = 3 * (points[5] - 2 * points[3] + points[1]);
   ydt3_term = points[7] + 3 * (-points[5] + points[3]) - points[1];
   
   ydt2_term = dt2 * ydt2_term;
   ydt3_term = dt3 * ydt3_term;
   
   dddy = 6 * ydt3_term;
   ddy = -6 * ydt3_term + 2 * ydt2_term;
   dy = ydt3_term - ydt2_term + dt * 3 * (points[3] - points[1]);
   y = points[1];
   
   cache_point_buffer[2 * ii] = x;
   cache_point_buffer[2 * ii + 1] = y;
   
   for (ii = 1; ii < num_segments; ii++) {
      ddx += dddx;
      dx += ddx;
      x += dx;
      
      ddy += dddy;
      dy += ddy;
      y += dy;
      
      cache_point_buffer[2 * ii] = x;
      cache_point_buffer[2 * ii + 1] = y;
   }
   al_calculate_ribbon(vbuff, cache_point_buffer, thickness, start, num_segments);
}

void al_draw_spline(float points[8], ALLEGRO_COLOR color, float thickness)
{
   int ii;
   int num_segments = (int)(sqrtf(hypotf(points[2] - points[0], points[3] - points[1]) +
                                  hypotf(points[4] - points[2], points[5] - points[3]) +
                                  hypotf(points[6] - points[4], points[7] - points[5])) *
                            1.2 * ALLEGRO_PRIM_QUALITY / 10);
                            
   verify_cache();
   
   if (thickness > 0) {
      if (2 * num_segments >= ALLEGRO_VBUFF_CACHE_SIZE) {
         num_segments = (ALLEGRO_VBUFF_CACHE_SIZE - 1) / 2;
      }
      
      if (!al_lock_vbuff_range(cache_buffer, 0, 2 * num_segments, ALLEGRO_VBUFFER_WRITE))
         return;
         
      al_calculate_spline(cache_buffer, points, thickness, 0, num_segments);
      
      for (ii = 0; ii < 2 * num_segments; ii++) {
         al_set_vbuff_color(cache_buffer, ii, color);
      }
      
      al_unlock_vbuff(cache_buffer);
      
      al_draw_prim(cache_buffer, 0, 0, 2 * num_segments, ALLEGRO_PRIM_TRIANGLE_STRIP);
   } else {
      if (num_segments >= ALLEGRO_VBUFF_CACHE_SIZE) {
         num_segments = ALLEGRO_VBUFF_CACHE_SIZE - 1;
      }
      
      if (!al_lock_vbuff_range(cache_buffer, 0, num_segments, ALLEGRO_VBUFFER_WRITE))
         return;
         
      al_calculate_spline(cache_buffer, points, thickness, 0, num_segments);
      
      for (ii = 0; ii < num_segments; ii++) {
         al_set_vbuff_color(cache_buffer, ii, color);
      }
      
      al_unlock_vbuff(cache_buffer);
      
      al_draw_prim(cache_buffer, 0, 0, num_segments, ALLEGRO_PRIM_LINE_STRIP);
   }
}

void al_calculate_ribbon(ALLEGRO_VBUFFER* vbuff, float *points, float thickness, int start, int num_segments)
{
   ASSERT(vbuff);
   ASSERT(points);
   ASSERT(num_segments >= 2);
   int need_unlock = 0;
   
   if (thickness > 0) {
      if (al_vbuff_is_locked(vbuff)) {
         if (!al_vbuff_range_is_locked(vbuff, start, start + 2 * num_segments))
            return;
      } else {
         if (!al_lock_vbuff_range(vbuff, start, start + 2 * num_segments, ALLEGRO_VBUFFER_WRITE))
            return;
         need_unlock = 1;
      }
      
      int ii = 0;
      float x, y;
      
      float cur_dir_x;
      float cur_dir_y;
      float prev_dir_x;
      float prev_dir_y;
      float t = thickness / 2;
      float tx, ty;
      
      for (ii = 0; ii < 2 * num_segments - 2; ii += 2) {
         x = points[ii];
         y = points[ii + 1];
         
         cur_dir_x = points[ii + 2] - x;
         cur_dir_y = points[ii + 3] - y;
         
         const float dir_len = hypotf(cur_dir_x, cur_dir_y);
         
         cur_dir_x /= dir_len;
         cur_dir_y /= dir_len;
         
         if (ii == 0) {
            tx = -t * cur_dir_y;
            ty = t * cur_dir_x;
            
            al_set_vbuff_pos(vbuff, start + ii, x + tx, y + ty, 0);
            al_set_vbuff_pos(vbuff, start + ii + 1, x - tx, y - ty, 0);
         } else {
         
            tx = cur_dir_x - prev_dir_x;
            ty = cur_dir_y - prev_dir_y;
            const float norm_len = hypotf(tx, ty);
            tx /= norm_len;
            ty /= norm_len;
            
            float cosine = tx * (-cur_dir_y) + ty * (cur_dir_x);
            const float new_norm_len = t / cosine;
            
            tx *= new_norm_len;
            ty *= new_norm_len;
         }
         
         al_set_vbuff_pos(vbuff, start + ii, x + tx, y + ty, 0);
         al_set_vbuff_pos(vbuff, start + ii + 1, x - tx, y - ty, 0);
         
         
         prev_dir_x = cur_dir_x;
         prev_dir_y = cur_dir_y;
      }
      tx = -t * prev_dir_y;
      ty = t * prev_dir_x;
      
      x = points[ii];
      y = points[ii + 1];
      
      al_set_vbuff_pos(vbuff, start + ii, x + tx, y + ty, 0);
      al_set_vbuff_pos(vbuff, start + ii + 1, x - tx, y - ty, 0);
   } else {
      if (al_vbuff_is_locked(vbuff)) {
         if (!al_vbuff_range_is_locked(vbuff, start, start + num_segments))
            return;
      } else {
         if (!al_lock_vbuff_range(vbuff, start, start + num_segments, ALLEGRO_VBUFFER_WRITE))
            return;
         need_unlock = 1;
      }
      int ii;
      for (ii = 0; ii < num_segments; ii++) {
         al_set_vbuff_pos(vbuff, start + ii, points[2 * ii], points[2 * ii + 1], 0);
      }
   }
   
   if (need_unlock)
      al_unlock_vbuff(vbuff);
}

void al_draw_ribbon(float *points, ALLEGRO_COLOR color, float thickness, int num_segments)
{
   int ii;
   verify_cache();
   
   if (thickness > 0) {
      if (!al_lock_vbuff_range(cache_buffer, 0, 2 * num_segments, ALLEGRO_VBUFFER_WRITE))
         return;
         
      al_calculate_ribbon(cache_buffer, points, thickness, 0, num_segments);
      
      for (ii = 0; ii < 2 * num_segments; ii++) {
         al_set_vbuff_color(cache_buffer, ii, color);
      }
      
      al_unlock_vbuff(cache_buffer);
      
      al_draw_prim(cache_buffer, 0, 0, 2 * num_segments, ALLEGRO_PRIM_TRIANGLE_STRIP);
   } else {
      if (!al_lock_vbuff_range(cache_buffer, 0, num_segments, ALLEGRO_VBUFFER_WRITE))
         return;
         
      al_calculate_ribbon(cache_buffer, points, thickness, 0, num_segments);
      
      for (ii = 0; ii < num_segments; ii++) {
         al_set_vbuff_color(cache_buffer, ii, color);
      }
      
      al_unlock_vbuff(cache_buffer);
      
      al_draw_prim(cache_buffer, 0, 0, num_segments, ALLEGRO_PRIM_LINE_STRIP);
   }
}
