#include "allegro5/allegro5.h"
#include "allegro5/allegro_shader.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/allegro_opengl.h"
#include <cstdio>

#if defined ALLEGRO_WINDOWS
   #define HLSL
   //#define GLSL
#else
   #define GLSL
#endif

/*
#include <Cg/cg.h>
#define CG
*/

#ifdef HLSL
#define TOP 0
#define BOT 1
#define vsource_name hlsl_vertex_source
#define psource_name hlsl_pixel_source
#define PLATFORM ALLEGRO_SHADER_HLSL
#elif defined GLSL
#define TOP 1
#define BOT 0
#define vsource_name glsl_vertex_source
#define psource_name glsl_pixel_source
#define PLATFORM ALLEGRO_SHADER_GLSL
#else
#define TOP 1
#define BOT 0
#define vsource_name cg_vertex_source
#define psource_name cg_pixel_source
#define PLATFORM ALLEGRO_SHADER_CG
#endif

#ifdef CG	
static const char *cg_vertex_source =
   "uniform float4x4 proj_matrix;\n"
   "uniform float4x4 view_matrix;\n"
   "void vs_main(\n"
   "  in float3 pos        : POSITION,\n"
   "  in float4 color      : COLOR0,\n"
   "  in float2 texcoord   : TEXCOORD0,\n"
   "  out float4 posO      : POSITION,\n"
   "  out float4 colorO    : COLOR0,\n"
   "  out float2 texcoordO : TEXCOORD0)\n"
   "{\n"
   "  posO = mul(mul(float4(pos, 1.0), view_matrix), proj_matrix);\n"
   "  colorO = color;\n"
   "  texcoordO = texcoord;\n"
   "}\n";

static const char *cg_pixel_source =
   "uniform sampler2D t;\n"
   "uniform float3 tint;\n"
   "void ps_main(\n"
   "  in float4 color    : COLOR0,\n"
   "  in float2 texcoord : TEXCOORD0,\n"
   "  out float4 colorO  : COLOR0)\n"
   "{\n"
   "  colorO = color * tex2D(t, texcoord);\n"
   "  colorO.r *= tint.r;\n"
   "  colorO.g *= tint.g;\n"
   "  colorO.b *= tint.b;\n"
   "}\n";
#endif

#ifdef HLSL
static const char *hlsl_vertex_source =
   "struct VS_INPUT\n"
   "{\n"
   "   float4 Position  : POSITION0;\n"
   "   float2 TexCoord  : TEXCOORD0;\n"
   "   float4 Color     : TEXCOORD1;\n"
   "};\n"
   "struct VS_OUTPUT\n"
   "{\n"
   "   float4 Position  : POSITION0;\n"
   "   float4 Color     : COLOR0;\n"
   "   float2 TexCoord  : TEXCOORD0;\n"
   "};\n"
   "\n"
   "float4x4 proj_matrix;\n"
   "float4x4 view_matrix;\n"
   "\n"
   "VS_OUTPUT vs_main(VS_INPUT Input)\n"
   "{\n"
   "   VS_OUTPUT Output;\n"
   "   Output.Position = mul(mul(Input.Position, view_matrix), proj_matrix);\n"
   "   Output.Color = Input.Color;\n"
   "   Output.TexCoord = Input.TexCoord;\n"
   "   return Output;\n"
   "}\n";

static const char *hlsl_pixel_source =
   "texture t;\n"
   "sampler2D s = sampler_state {\n"
   "   texture = <t>;\n"
   "};\n"
   "float3 tint;\n"
   "float4 ps_main(VS_OUTPUT Input) : COLOR0\n"
   "{\n"
   "   float4 pixel = tex2D(s, Input.TexCoord.xy);\n"
   "   pixel.r *= tint.r;\n"
   "   pixel.g *= tint.g;\n"
   "   pixel.b *= tint.b;\n"
   "   return pixel;\n"
   "}\n";
#endif

#ifdef GLSL
static const char *glsl_vertex_source =
   "attribute vec4 pos;\n"
   "attribute vec4 color;\n"
   "attribute vec2 texcoord;\n"
   "uniform mat4 proj_matrix;\n"
   "uniform mat4 view_matrix;\n"
   "varying vec4 varying_color;\n"
   "varying vec2 varying_texcoord;\n"
   "void main()\n"
   "{\n"
   "  varying_color = color;\n"
   "  varying_texcoord = texcoord;\n"
   "  gl_Position = proj_matrix * view_matrix * pos;\n"
   "}\n";

static const char *glsl_pixel_source =
   "uniform sampler2D t;\n"
   "uniform vec3 tint;\n"
   "varying vec4 varying_color;\n"
   "varying vec2 varying_texcoord;\n"
   "void main()\n"
   "{\n"
   "  vec4 tmp = varying_color * texture2D(t, varying_texcoord);\n"
   "  tmp.r *= tint.r;\n"
   "  tmp.g *= tint.g;\n"
   "  tmp.b *= tint.b;\n"
   "  gl_FragColor = tmp;\n"
   "}\n";
#endif

#if defined ALLEGRO_WINDOWS && !defined CG
#include <allegro5/allegro_direct3d.h>
#include <d3d9.h>
#include <d3dx9.h>

#define A5V_FVF (D3DFVF_XYZ | D3DFVF_TEX2 | D3DFVF_TEXCOORDSIZE2(0) | D3DFVF_TEXCOORDSIZE4(1))

void drawD3D(ALLEGRO_VERTEX *v, int start, int count)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   LPDIRECT3DDEVICE9 device = al_get_d3d_device(display);

   device->SetFVF(A5V_FVF);

   device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, count / 3,
      &v[start].x, sizeof(ALLEGRO_VERTEX));
}
#endif

void drawGL(ALLEGRO_VERTEX *v, int start, int count)
{
   (void)v;
   glDrawArrays(GL_TRIANGLES, start, count);
}

int main(int argc, char **argv)
{
   ALLEGRO_DISPLAY *display;
   ALLEGRO_BITMAP *bmp;
   ALLEGRO_SHADER *shader;
   float *tints;

   (void)argc;
   (void)argv;

   al_init();
   al_install_keyboard();
   al_init_image_addon();

   al_set_new_display_flags(ALLEGRO_USE_PROGRAMMABLE_PIPELINE);

   display = al_create_display(640, 480);
   bmp = al_load_bitmap("data/mysha.pcx");

   shader = al_create_shader(PLATFORM);

   al_attach_shader_source(
      shader,
      ALLEGRO_VERTEX_SHADER,
      vsource_name
   );
   al_attach_shader_source(
      shader,
      ALLEGRO_PIXEL_SHADER,
      psource_name 
   );

   al_link_shader(shader);

#ifdef GLSL
   al_set_opengl_program_object(display, al_get_opengl_program_object(shader));
#endif
      
   ALLEGRO_VERTEX *v;
   v = new ALLEGRO_VERTEX[6*4];

   v[0].x = 0;
   v[0].y = 160;
   v[0].z = 0;
   v[0].u = 0;
   v[0].v = BOT;
   v[0].color = al_map_rgb(255, 255, 255);
   v[1].x = 0;
   v[1].y = 0;
   v[1].z = 0;
   v[1].u = 0;
   v[1].v = TOP;
   v[1].color = al_map_rgb(255, 255, 255);
   v[2].x = 240;
   v[2].y = 0;
   v[2].z = 0;
   v[2].u = 1;
   v[2].v = TOP;
   v[2].color = al_map_rgb(255, 255, 255);

   v[3].x = 0;
   v[3].y = 160;
   v[3].z = 0;
   v[3].u = 0;
   v[3].v = BOT;
   v[3].color = al_map_rgb(255, 255, 255);
   v[4].x = 240;
   v[4].y = 0;
   v[4].z = 0;
   v[4].u = 1;
   v[4].v = TOP;
   v[4].color = al_map_rgb(255, 255, 255);
   v[5].x = 240;
   v[5].y = 160;
   v[5].z = 0;
   v[5].u = 1;
   v[5].v = BOT;
   v[5].color = al_map_rgb(255, 255, 0);

   for (int i = 0; i < 6; i++) {
      v[i+6] = v[i];
      v[i+6].x += 300;
      v[i+12] = v[i];
      v[i+12].y += 200;
      v[i+18] = v[i];
   }

   tints = new float[3*4];

   tints[0] = 4.0;
   tints[1] = 0.0;
   tints[2] = 1.0;

   tints[3] = 0.0;
   tints[4] = 4.0;
   tints[5] = 1.0;

   tints[6] = 1.0;
   tints[7] = 0.0;
   tints[8] = 4.0;

   tints[9] = 4.0;
   tints[10] = 4.0;
   tints[11] = 1.0;

   al_set_shader_vertex_array(shader, &v[0].x, sizeof(ALLEGRO_VERTEX));
   al_set_shader_color_array(shader, (unsigned char *)&v[0].color, sizeof(ALLEGRO_VERTEX));
   al_set_shader_texcoord_array(shader, &v[0].u, sizeof(ALLEGRO_VERTEX));

   while (1) {
      ALLEGRO_KEYBOARD_STATE s;
      al_get_keyboard_state(&s);
      if (al_key_down(&s, ALLEGRO_KEY_ESCAPE))
         break;
      al_set_target_bitmap(al_get_backbuffer(display));
      al_clear_to_color(
         al_map_rgb(140, 40, 40)
      );

      al_set_shader_sampler(shader, "t", bmp, 0);
      al_set_shader_float_vector(shader, "tint", 3, &tints[0]);
      al_use_shader(shader, true);
      if (al_get_display_flags(display) & ALLEGRO_OPENGL)
      	drawGL(v, 0, 6);
      #if defined ALLEGRO_WINDOWS && !defined CG
      else
         drawD3D(v, 0, 6);
      #endif
      al_use_shader(shader, false);

      al_set_shader_sampler(shader, "t", bmp, 0);
      al_set_shader_float_vector(shader, "tint", 3, &tints[3]);
      al_use_shader(shader, true);
      if (al_get_display_flags(display) & ALLEGRO_OPENGL)
      	drawGL(v, 6, 6);
      #if defined ALLEGRO_WINDOWS && !defined CG
      else
         drawD3D(v, 6, 6);
      #endif
      al_use_shader(shader, false);

      al_set_shader_sampler(shader, "t", bmp, 0);
      al_set_shader_float_vector(shader, "tint", 3, &tints[6]);
      al_use_shader(shader, true);
      if (al_get_display_flags(display) & ALLEGRO_OPENGL)
      	drawGL(v, 12, 6);
      #if defined ALLEGRO_WINDOWS && !defined CG
      else
         drawD3D(v, 12, 6);
      #endif
      al_use_shader(shader, false);

      /* Draw the last one transformed */

      ALLEGRO_TRANSFORM trans, backup;
      al_copy_transform(&backup, al_get_current_transform());
      al_identity_transform(&trans);
      al_translate_transform(&trans, 300, 200);
      al_use_transform(&trans);

      al_set_shader_sampler(shader, "t", bmp, 0);
      al_set_shader_float_vector(shader, "tint", 3, &tints[9]);
      al_use_shader(shader, true);
      if (al_get_display_flags(display) & ALLEGRO_OPENGL)
      	drawGL(v, 18, 6);
      #if defined ALLEGRO_WINDOWS && !defined CG
      else
         drawD3D(v, 18, 6);
      #endif
      al_use_shader(shader, false);

      al_use_transform(&backup);

      al_flip_display();
      al_rest(0.01);
   }

   al_destroy_shader(shader);

   return 0;
}
