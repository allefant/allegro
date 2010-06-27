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
 *      DirectX dummy shader support. Dummy shader doesn't do anything
 *      except making D3D happy when you pass it vertices with non-FVF
 *      memory layout.
 *
 *      By Pavel Sountsov.
 * 
 *      Thanks to Michał Cichoń for the pre-compiled shader code.
 *
 *      See readme.txt for copyright information.
 */

#include <string.h>
#include "allegro5/allegro_primitives.h"
#include "allegro5/internal/aintern_prim.h"
#include "allegro5/platform/alplatf.h"

#ifdef ALLEGRO_CFG_D3D

#include "allegro5/allegro_direct3d.h"
#include "allegro5/internal/aintern_prim_directx.h"

/*
 * The following pre-compiled shaders are generated using the
 * nshader.cpp program. See that file for instructions.
 */

/*
   POS3 TEX2 COL4 1

   //
   // Generated by Microsoft (R) HLSL Shader Compiler 9.23.949.2378
   //
   // Parameters:
   //
   //   float4x4 g_texture_proj;
   //   float4x4 g_world_view_proj;
   //
   //
   // Registers:
   //
   //   Name              Reg   Size
   //   ----------------- ----- ----
   //   g_world_view_proj c0       4
   //   g_texture_proj    c4       2
   //
   
   vs_1_1
   def c6, 1, 0, 0, 0
   dcl_position v0
   dcl_texcoord v1
   dcl_texcoord1 v2
   mad r0, v0.xyzx, c6.xxxy, c6.yyyx
   dp4 oPos.x, r0, c0
   dp4 oPos.y, r0, c1
   dp4 oPos.z, r0, c2
   dp4 oPos.w, r0, c3
   mad r0.xyz, v1.xyxw, c6.xxyw, c6.yyxw
   dp3 oT0.x, r0, c4.xyww
   dp3 oT0.y, r0, c5.xyww
   mov oD0, v2
   
   // approximately 9 instruction slots used
*/
static const uint8_t _al_vs_pos3_tex2_col4_1[] = {
   0x01, 0x01, 0xfe, 0xff, 0x51, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x80, 0x3f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0f, 0x90, 0x1f, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80,
   0x01, 0x00, 0x0f, 0x90, 0x1f, 0x00, 0x00, 0x00, 0x05, 0x00, 0x01, 0x80, 0x02, 0x00, 0x0f, 0x90,
   0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x00, 0x00, 0x24, 0x90, 0x06, 0x00, 0x40, 0xa0,
   0x06, 0x00, 0x15, 0xa0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0xe4, 0x80,
   0x00, 0x00, 0xe4, 0xa0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xc0, 0x00, 0x00, 0xe4, 0x80,
   0x01, 0x00, 0xe4, 0xa0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xc0, 0x00, 0x00, 0xe4, 0x80,
   0x02, 0x00, 0xe4, 0xa0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xc0, 0x00, 0x00, 0xe4, 0x80,
   0x03, 0x00, 0xe4, 0xa0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x01, 0x00, 0xc4, 0x90,
   0x06, 0x00, 0xd0, 0xa0, 0x06, 0x00, 0xc5, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xe0,
   0x00, 0x00, 0xe4, 0x80, 0x04, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xe0,
   0x00, 0x00, 0xe4, 0x80, 0x05, 0x00, 0xf4, 0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd0,
   0x02, 0x00, 0xe4, 0x90, 0xff, 0xff, 0x00, 0x00
};

/*
   POS3 TEX2 COL0

   //
   // Generated by Microsoft (R) HLSL Shader Compiler 9.23.949.2378
   //
   // Parameters:
   //
   //   float4x4 g_texture_proj;
   //   float4x4 g_world_view_proj;
   //
   //
   // Registers:
   //
   //   Name              Reg   Size
   //   ----------------- ----- ----
   //   g_world_view_proj c0       4
   //   g_texture_proj    c4       2
   //
   
   vs_1_1
   def c6, 1, 0, 0, 0
   dcl_position v0
   dcl_texcoord v1
   mad r0, v0.xyzx, c6.xxxy, c6.yyyx
   dp4 oPos.x, r0, c0
   dp4 oPos.y, r0, c1
   dp4 oPos.z, r0, c2
   dp4 oPos.w, r0, c3
   mad r0.xyz, v1.xyxw, c6.xxyw, c6.yyxw
   dp3 oT0.x, r0, c4.xyww
   dp3 oT0.y, r0, c5.xyww
   mov oD0, c6.x
   
   // approximately 9 instruction slots used
*/
static const uint8_t _al_vs_pos3_tex2_col0[] = {
   0x01, 0x01, 0xfe, 0xff, 0x51, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x80, 0x3f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0f, 0x90, 0x1f, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80,
   0x01, 0x00, 0x0f, 0x90, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x00, 0x00, 0x24, 0x90,
   0x06, 0x00, 0x40, 0xa0, 0x06, 0x00, 0x15, 0xa0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x00, 0x00, 0xe4, 0xa0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x01, 0x00, 0xe4, 0xa0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x02, 0x00, 0xe4, 0xa0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x03, 0x00, 0xe4, 0xa0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80,
   0x01, 0x00, 0xc4, 0x90, 0x06, 0x00, 0xd0, 0xa0, 0x06, 0x00, 0xc5, 0xa0, 0x08, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0xe4, 0x80, 0x04, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x02, 0xe0, 0x00, 0x00, 0xe4, 0x80, 0x05, 0x00, 0xf4, 0xa0, 0x01, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x0f, 0xd0, 0x06, 0x00, 0x00, 0xa0, 0xff, 0xff, 0x00, 0x00
};

/*
   POS3 TEX0 COL4 0

   //
   // Generated by Microsoft (R) HLSL Shader Compiler 9.23.949.2378
   //
   // Parameters:
   //
   //   float4x4 g_world_view_proj;
   //
   //
   // Registers:
   //
   //   Name              Reg   Size
   //   ----------------- ----- ----
   //   g_world_view_proj c0       4
   //
   
   vs_1_1
   def c4, 1, 0, 0, 0
   dcl_position v0
   dcl_texcoord v1
   mad r0, v0.xyzx, c4.xxxy, c4.yyyx
   dp4 oPos.x, r0, c0
   dp4 oPos.y, r0, c1
   dp4 oPos.z, r0, c2
   dp4 oPos.w, r0, c3
   mov oD0, v1
   
   // approximately 6 instruction slots used
*/
static const uint8_t _al_vs_pos3_tex0_col4_0[] = {
   0x01, 0x01, 0xfe, 0xff, 0x51, 0x00, 0x00, 0x00, 0x04, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x80, 0x3f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0f, 0x90, 0x1f, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80,
   0x01, 0x00, 0x0f, 0x90, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x80, 0x00, 0x00, 0x24, 0x90,
   0x04, 0x00, 0x40, 0xa0, 0x04, 0x00, 0x15, 0xa0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x00, 0x00, 0xe4, 0xa0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x01, 0x00, 0xe4, 0xa0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x02, 0x00, 0xe4, 0xa0, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x03, 0x00, 0xe4, 0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd0,
   0x01, 0x00, 0xe4, 0x90, 0xff, 0xff, 0x00, 0x00
};

/*
   POS3 TEX0 COL0

   //
   // Generated by Microsoft (R) HLSL Shader Compiler 9.23.949.2378
   //
   // Parameters:
   //
   //   float4x4 g_world_view_proj;
   //
   //
   // Registers:
   //
   //   Name              Reg   Size
   //   ----------------- ----- ----
   //   g_world_view_proj c0       4
   //
   
   vs_1_1
   def c4, 1, 0, 0, 0
   dcl_position v0
   mad r0, v0.xyzx, c4.xxxy, c4.yyyx
   dp4 oPos.x, r0, c0
   dp4 oPos.y, r0, c1
   dp4 oPos.z, r0, c2
   dp4 oPos.w, r0, c3
   mov oD0, c4.x
   
   // approximately 6 instruction slots used
*/
static const uint8_t _al_vs_pos3_tex0_col0[] = {
   0x01, 0x01, 0xfe, 0xff, 0x51, 0x00, 0x00, 0x00, 0x04, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x80, 0x3f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0f, 0x90, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x80,
   0x00, 0x00, 0x24, 0x90, 0x04, 0x00, 0x40, 0xa0, 0x04, 0x00, 0x15, 0xa0, 0x09, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0xe4, 0x80, 0x00, 0x00, 0xe4, 0xa0, 0x09, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x02, 0xc0, 0x00, 0x00, 0xe4, 0x80, 0x01, 0x00, 0xe4, 0xa0, 0x09, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x04, 0xc0, 0x00, 0x00, 0xe4, 0x80, 0x02, 0x00, 0xe4, 0xa0, 0x09, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x08, 0xc0, 0x00, 0x00, 0xe4, 0x80, 0x03, 0x00, 0xe4, 0xa0, 0x01, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x0f, 0xd0, 0x04, 0x00, 0x00, 0xa0, 0xff, 0xff, 0x00, 0x00
};

/*
   POS2 TEX2 COL4 1

   //
   // Generated by Microsoft (R) HLSL Shader Compiler 9.23.949.2378
   //
   // Parameters:
   //
   //   float4x4 g_texture_proj;
   //   float4x4 g_world_view_proj;
   //
   //
   // Registers:
   //
   //   Name              Reg   Size
   //   ----------------- ----- ----
   //   g_world_view_proj c0       4
   //   g_texture_proj    c4       2
   //
   
   vs_1_1
   def c6, 1, 0, 0, 0
   dcl_position v0
   dcl_texcoord v1
   dcl_texcoord1 v2
   mad r0.xyz, v0.xyxw, c6.xxyw, c6.yyxw
   dp3 oPos.x, r0, c0.xyww
   dp3 oPos.y, r0, c1.xyww
   dp3 oPos.z, r0, c2.xyww
   dp3 oPos.w, r0, c3.xyww
   mad r0.xyz, v1.xyxw, c6.xxyw, c6.yyxw
   dp3 oT0.x, r0, c4.xyww
   dp3 oT0.y, r0, c5.xyww
   mov oD0, v2
   
   // approximately 9 instruction slots used
*/
static const uint8_t _al_vs_pos2_tex2_col4_1[] = {
   0x01, 0x01, 0xfe, 0xff, 0x51, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x80, 0x3f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0f, 0x90, 0x1f, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80,
   0x01, 0x00, 0x0f, 0x90, 0x1f, 0x00, 0x00, 0x00, 0x05, 0x00, 0x01, 0x80, 0x02, 0x00, 0x0f, 0x90,
   0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0xc4, 0x90, 0x06, 0x00, 0xd0, 0xa0,
   0x06, 0x00, 0xc5, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0xe4, 0x80,
   0x00, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xc0, 0x00, 0x00, 0xe4, 0x80,
   0x01, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xc0, 0x00, 0x00, 0xe4, 0x80,
   0x02, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xc0, 0x00, 0x00, 0xe4, 0x80,
   0x03, 0x00, 0xf4, 0xa0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x01, 0x00, 0xc4, 0x90,
   0x06, 0x00, 0xd0, 0xa0, 0x06, 0x00, 0xc5, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xe0,
   0x00, 0x00, 0xe4, 0x80, 0x04, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xe0,
   0x00, 0x00, 0xe4, 0x80, 0x05, 0x00, 0xf4, 0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd0,
   0x02, 0x00, 0xe4, 0x90, 0xff, 0xff, 0x00, 0x00
};

/*
   POS2 TEX2 COL0

   //
   // Generated by Microsoft (R) HLSL Shader Compiler 9.23.949.2378
   //
   // Parameters:
   //
   //   float4x4 g_texture_proj;
   //   float4x4 g_world_view_proj;
   //
   //
   // Registers:
   //
   //   Name              Reg   Size
   //   ----------------- ----- ----
   //   g_world_view_proj c0       4
   //   g_texture_proj    c4       2
   //
   
   vs_1_1
   def c6, 1, 0, 0, 0
   dcl_position v0
   dcl_texcoord v1
   mad r0.xyz, v0.xyxw, c6.xxyw, c6.yyxw
   dp3 oPos.x, r0, c0.xyww
   dp3 oPos.y, r0, c1.xyww
   dp3 oPos.z, r0, c2.xyww
   dp3 oPos.w, r0, c3.xyww
   mad r0.xyz, v1.xyxw, c6.xxyw, c6.yyxw
   dp3 oT0.x, r0, c4.xyww
   dp3 oT0.y, r0, c5.xyww
   mov oD0, c6.x
   
   // approximately 9 instruction slots used
*/
static const uint8_t _al_vs_pos2_tex2_col0[] = {
   0x01, 0x01, 0xfe, 0xff, 0x51, 0x00, 0x00, 0x00, 0x06, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x80, 0x3f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0f, 0x90, 0x1f, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80,
   0x01, 0x00, 0x0f, 0x90, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0xc4, 0x90,
   0x06, 0x00, 0xd0, 0xa0, 0x06, 0x00, 0xc5, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x00, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x01, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x02, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x03, 0x00, 0xf4, 0xa0, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80,
   0x01, 0x00, 0xc4, 0x90, 0x06, 0x00, 0xd0, 0xa0, 0x06, 0x00, 0xc5, 0xa0, 0x08, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0xe4, 0x80, 0x04, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x02, 0xe0, 0x00, 0x00, 0xe4, 0x80, 0x05, 0x00, 0xf4, 0xa0, 0x01, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x0f, 0xd0, 0x06, 0x00, 0x00, 0xa0, 0xff, 0xff, 0x00, 0x00
};

/*
   POS2 TEX0 COL4 0

   //
   // Generated by Microsoft (R) HLSL Shader Compiler 9.23.949.2378
   //
   // Parameters:
   //
   //   float4x4 g_world_view_proj;
   //
   //
   // Registers:
   //
   //   Name              Reg   Size
   //   ----------------- ----- ----
   //   g_world_view_proj c0       4
   //
   
   vs_1_1
   def c4, 1, 0, 0, 0
   dcl_position v0
   dcl_texcoord v1
   mad r0.xyz, v0.xyxw, c4.xxyw, c4.yyxw
   dp3 oPos.x, r0, c0.xyww
   dp3 oPos.y, r0, c1.xyww
   dp3 oPos.z, r0, c2.xyww
   dp3 oPos.w, r0, c3.xyww
   mov oD0, v1
   
   // approximately 6 instruction slots used
*/
static const uint8_t _al_vs_pos2_tex0_col4_0[] = {
   0x01, 0x01, 0xfe, 0xff, 0x51, 0x00, 0x00, 0x00, 0x04, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x80, 0x3f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0f, 0x90, 0x1f, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x80,
   0x01, 0x00, 0x0f, 0x90, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0xc4, 0x90,
   0x04, 0x00, 0xd0, 0xa0, 0x04, 0x00, 0xc5, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x00, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x01, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x02, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xc0,
   0x00, 0x00, 0xe4, 0x80, 0x03, 0x00, 0xf4, 0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd0,
   0x01, 0x00, 0xe4, 0x90, 0xff, 0xff, 0x00, 0x00
};

/*
   POS2 TEX0 COL0

   //
   // Generated by Microsoft (R) HLSL Shader Compiler 9.23.949.2378
   //
   // Parameters:
   //
   //   float4x4 g_world_view_proj;
   //
   //
   // Registers:
   //
   //   Name              Reg   Size
   //   ----------------- ----- ----
   //   g_world_view_proj c0       4
   //
   
   vs_1_1
   def c4, 1, 0, 0, 0
   dcl_position v0
   mad r0.xyz, v0.xyxw, c4.xxyw, c4.yyxw
   dp3 oPos.x, r0, c0.xyww
   dp3 oPos.y, r0, c1.xyww
   dp3 oPos.z, r0, c2.xyww
   dp3 oPos.w, r0, c3.xyww
   mov oD0, c4.x
   
   // approximately 6 instruction slots used
*/
static const uint8_t _al_vs_pos2_tex0_col0[] = {
   0x01, 0x01, 0xfe, 0xff, 0x51, 0x00, 0x00, 0x00, 0x04, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x80, 0x3f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0f, 0x90, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80,
   0x00, 0x00, 0xc4, 0x90, 0x04, 0x00, 0xd0, 0xa0, 0x04, 0x00, 0xc5, 0xa0, 0x08, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x01, 0xc0, 0x00, 0x00, 0xe4, 0x80, 0x00, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x02, 0xc0, 0x00, 0x00, 0xe4, 0x80, 0x01, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x04, 0xc0, 0x00, 0x00, 0xe4, 0x80, 0x02, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x08, 0xc0, 0x00, 0x00, 0xe4, 0x80, 0x03, 0x00, 0xf4, 0xa0, 0x01, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x0f, 0xd0, 0x04, 0x00, 0x00, 0xa0, 0xff, 0xff, 0x00, 0x00
};

/*
   POS0 TEX2 COL4 1

   //
   // Generated by Microsoft (R) HLSL Shader Compiler 9.23.949.2378
   //
   // Parameters:
   //
   //   float4x4 g_texture_proj;
   //
   //
   // Registers:
   //
   //   Name           Reg   Size
   //   -------------- ----- ----
   //   g_texture_proj c4       2
   //
   
   vs_1_1
   def c0, 1, 0, 0, 0
   dcl_texcoord v0
   dcl_texcoord1 v1
   mad r0.xyz, v0.xyxw, c0.xxyw, c0.yyxw
   dp3 oT0.x, r0, c4.xyww
   dp3 oT0.y, r0, c5.xyww
   mov oPos, c0.yyyx
   mov oD0, v1
   
   // approximately 5 instruction slots used
*/
static const uint8_t _al_vs_pos0_tex2_col4_1[] = {
   0x01, 0x01, 0xfe, 0xff, 0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x80, 0x3f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
   0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0f, 0x90, 0x1f, 0x00, 0x00, 0x00, 0x05, 0x00, 0x01, 0x80,
   0x01, 0x00, 0x0f, 0x90, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0xc4, 0x90,
   0x00, 0x00, 0xd0, 0xa0, 0x00, 0x00, 0xc5, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xe0,
   0x00, 0x00, 0xe4, 0x80, 0x04, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xe0,
   0x00, 0x00, 0xe4, 0x80, 0x05, 0x00, 0xf4, 0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0,
   0x00, 0x00, 0x15, 0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd0, 0x01, 0x00, 0xe4, 0x90,
   0xff, 0xff, 0x00, 0x00
};

/*
   POS0 TEX2 COL0

   //
   // Generated by Microsoft (R) HLSL Shader Compiler 9.23.949.2378
   //
   // Parameters:
   //
   //   float4x4 g_texture_proj;
   //
   //
   // Registers:
   //
   //   Name           Reg   Size
   //   -------------- ----- ----
   //   g_texture_proj c4       2
   //
   
   vs_1_1
   def c0, 1, 0, 0, 0
   dcl_texcoord v0
   mad r0.xyz, v0.xyxw, c0.xxyw, c0.yyxw
   dp3 oT0.x, r0, c4.xyww
   dp3 oT0.y, r0, c5.xyww
   mov oPos, c0.yyyx
   mov oD0, c0.x
   
   // approximately 5 instruction slots used
*/
static const uint8_t _al_vs_pos0_tex2_col0[] = {
   0x01, 0x01, 0xfe, 0xff, 0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x80, 0x3f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
   0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0f, 0x90, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x80,
   0x00, 0x00, 0xc4, 0x90, 0x00, 0x00, 0xd0, 0xa0, 0x00, 0x00, 0xc5, 0xa0, 0x08, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x01, 0xe0, 0x00, 0x00, 0xe4, 0x80, 0x04, 0x00, 0xf4, 0xa0, 0x08, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x02, 0xe0, 0x00, 0x00, 0xe4, 0x80, 0x05, 0x00, 0xf4, 0xa0, 0x01, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x0f, 0xc0, 0x00, 0x00, 0x15, 0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd0,
   0x00, 0x00, 0x00, 0xa0, 0xff, 0xff, 0x00, 0x00
};

/*
   POS0 TEX0 COL4 0

   //
   // Generated by Microsoft (R) HLSL Shader Compiler 9.23.949.2378
   vs_1_1
   def c0, 0, 1, 0, 0
   dcl_texcoord v0
   mov oPos, c0.xxxy
   mov oD0, v0
   
   // approximately 2 instruction slots used
*/
static const uint8_t _al_vs_pos0_tex0_col4_0[] = {
   0x01, 0x01, 0xfe, 0xff, 0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
   0x05, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0f, 0x90, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xc0,
   0x00, 0x00, 0x40, 0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd0, 0x00, 0x00, 0xe4, 0x90,
   0xff, 0xff, 0x00, 0x00
};

/*
   POS0 TEX0 COL0

   //
   // Generated by Microsoft (R) HLSL Shader Compiler 9.23.949.2378
   vs_1_1
   def c0, 0, 1, 0, 0
   mov oPos, c0.xxxy
   mov oD0, c0.y
   
   // approximately 2 instruction slots used
*/
static const uint8_t _al_vs_pos0_tex0_col0[] = {
   0x01, 0x01, 0xfe, 0xff, 0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xa0, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x80, 0x3f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x0f, 0xc0, 0x00, 0x00, 0x40, 0xa0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xd0,
   0x00, 0x00, 0x55, 0xa0, 0xff, 0xff, 0x00, 0x00
};

void _al_create_shader(void* dev, ALLEGRO_VERTEX_DECL* decl)
{
   LPDIRECT3DDEVICE9 device = dev;
   LPDIRECT3DVERTEXSHADER9 ret = 0;

   ALLEGRO_VERTEX_ELEMENT* e;

   int position = 0;
   int texture = 0;
   int color = 0;

   const uint8_t* shader = NULL;
   
   /*position, texture, color*/
   const uint8_t* shaders[3][2][2] = 
   {
      {
         {_al_vs_pos0_tex0_col0, _al_vs_pos0_tex0_col4_0},
         {_al_vs_pos0_tex2_col0, _al_vs_pos0_tex2_col4_1}
      },
      {
         {_al_vs_pos2_tex0_col0, _al_vs_pos2_tex0_col4_0},
         {_al_vs_pos2_tex2_col0, _al_vs_pos2_tex2_col4_1}
      },
      {
         {_al_vs_pos3_tex0_col0, _al_vs_pos3_tex0_col4_0},
         {_al_vs_pos3_tex2_col0, _al_vs_pos3_tex2_col4_1}
      }
   };

   e = &decl->elements[ALLEGRO_PRIM_POSITION];
   if (e->attribute) {
      switch(e->storage) {
         case ALLEGRO_PRIM_SHORT_2:
         case ALLEGRO_PRIM_FLOAT_2:
            position = 1;
         break;
         case ALLEGRO_PRIM_FLOAT_3:
            position = 2;
         break;
      }
   }

   e = &decl->elements[ALLEGRO_PRIM_TEX_COORD];
   if(!e->attribute)
      e = &decl->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL];
   if(e->attribute) {
      texture = 1;
   }

   e = &decl->elements[ALLEGRO_PRIM_COLOR_ATTR];
   if(e->attribute) {
      color = 1;
   }
      
   shader = shaders[position][texture][color];

   IDirect3DDevice9_CreateVertexShader(device, (const DWORD*)shader, &ret);

   decl->d3d_dummy_shader = ret;
}

void* _al_create_default_shader(void* dev)
{
   LPDIRECT3DDEVICE9 device = dev;
   LPDIRECT3DVERTEXSHADER9 shader;
   IDirect3DDevice9_CreateVertexShader(device, (const DWORD*)_al_vs_pos3_tex2_col4_1, &shader);
   return shader;
}

static void _al_swap(float* l, float* r)
{
  float temp = *r;
  *r = *l;
  *l = temp;
}

static void _al_transpose_d3d_matrix(D3DMATRIX* m)
{
   int i, j;
   float* m_ptr = (float*)m;

   for (i = 1; i < 4; i++) {
      for (j = 0; j < i; j++) {
         _al_swap(m_ptr + (i * 4 + j), m_ptr + (j * 4 + i));
      }
   }
}

static void _al_multiply_d3d_matrix(D3DMATRIX* result, const D3DMATRIX* a, const D3DMATRIX* b)
{
   int i, j, k;
   float* r_ptr = (float*)result;
   float* a_ptr = (float*)a;
   float* b_ptr = (float*)b;

   for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
         r_ptr[i * 4 + j] = 0.0f;
         for (k = 0; k < 4; k++) {
            r_ptr[i * 4 + j] += a_ptr[i * 4 + k] * b_ptr[k * 4 + j];
         }
      }
   }
}

static void _al_multiply_transpose_d3d_matrix(D3DMATRIX* result, const D3DMATRIX* a, const D3DMATRIX* b)
{
   int i, j, k;
   float* r_ptr = (float*)result;
   float* a_ptr = (float*)a;
   float* b_ptr = (float*)b;

   for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
         r_ptr[i + 4 * j] = 0.0f;
         for (k = 0; k < 4; k++) {
            r_ptr[i + 4 * j] += a_ptr[i * 4 + k] * b_ptr[k * 4 + j];
         }
      }
   }
}

/* note: passed matrix may be modified by this function */
void _al_set_texture_matrix(void* dev, float* mat)
{
   _al_transpose_d3d_matrix((D3DMATRIX*)mat);
   IDirect3DDevice9_SetVertexShaderConstantF((IDirect3DDevice9*)dev, 4, mat, 4);
}

static void setup_transforms(IDirect3DDevice9* device)
{
   D3DMATRIX matWorld, matView, matProj, matWorldView, matWorldViewProj;
   IDirect3DDevice9_GetTransform(device, D3DTS_WORLD, &matWorld);
   IDirect3DDevice9_GetTransform(device, D3DTS_VIEW, &matView);
   IDirect3DDevice9_GetTransform(device, D3DTS_PROJECTION, &matProj);
   _al_multiply_d3d_matrix(&matWorldView, &matWorld, &matView);
   _al_multiply_transpose_d3d_matrix(&matWorldViewProj, &matWorldView, &matProj);
   IDirect3DDevice9_SetVertexShaderConstantF(device, 0, (float*)&matWorldViewProj, 4);
}

void _al_setup_shader(void* dev, const ALLEGRO_VERTEX_DECL* decl)
{
   IDirect3DDevice9* device = (IDirect3DDevice9*)dev;
   setup_transforms(device);
   IDirect3DDevice9_SetVertexShader(device, (IDirect3DVertexShader9*)decl->d3d_dummy_shader);
}

void _al_setup_default_shader(void* dev, void* shader)
{
   IDirect3DDevice9* device = (IDirect3DDevice9*)dev;
   setup_transforms(device);
   IDirect3DDevice9_SetVertexShader(device, shader);
}

#endif /* ALLEGRO_CFG_D3D */

/* vim: set sts=3 sw=3 et: */
