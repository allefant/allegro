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
 *      Direct3D display driver
 *
 *      By Trent Gamblin.
 *
 */

#include <string.h>
#include <stdio.h>
#include <process.h>

#include "allegro.h"
#include "allegro/system_new.h"
#include "allegro/internal/aintern.h"
#include "allegro/internal/aintern_system.h"
#include "allegro/internal/aintern_display.h"
#include "allegro/internal/aintern_bitmap.h"
#include "allegro/internal/aintern_vector.h"
#include "allegro/platform/aintwin.h"
#include "allegro/internal/aintern_thread.h"

#include "wddraw.h"
#include "d3d.h"

static ALLEGRO_DISPLAY_INTERFACE *vt = 0;

static LPDIRECT3D9 _al_d3d = 0;
LPDIRECT3DDEVICE9 _al_d3d_device = 0;

static D3DPRESENT_PARAMETERS d3d_pp;

/* FIXME: This can probably go, it's kept in the system driver too */
static _AL_VECTOR d3d_created_displays = _AL_VECTOR_INITIALIZER(ALLEGRO_DISPLAY_D3D *);

static HWND d3d_hidden_window;

static float d3d_ortho_w;
static float d3d_ortho_h;

static bool d3d_already_fullscreen = false;
static bool _al_d3d_device_lost = false;
static LPDIRECT3DSURFACE9 d3d_current_texture_render_target = NULL;

static ALLEGRO_DISPLAY *d3d_target_display_before_device_lost = NULL;
static ALLEGRO_BITMAP *d3d_target_bitmap_before_device_lost = NULL;

static ALLEGRO_DISPLAY_D3D *d3d_fullscreen_display;

static _AL_MUTEX d3d_device_mutex;

static bool d3d_can_wait_for_vsync;

static bool render_to_texture_supported;

/*
 * These parameters cannot be gotten by the display thread because
 * they're thread local. We get them in the calling thread first.
 */
typedef struct new_display_parameters {
   ALLEGRO_DISPLAY_D3D *display;
   int format;
   int refresh_rate;
   int flags;
   int width;
   int height;
   bool is_resize;
} new_display_parameters;

static bool d3d_bitmaps_prepared_for_reset = false;

static int allegro_formats[] = {
   ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ARGB_8888,
   ALLEGRO_PIXEL_FORMAT_ARGB_4444,
   ALLEGRO_PIXEL_FORMAT_RGB_565,
   ALLEGRO_PIXEL_FORMAT_ARGB_1555,
   ALLEGRO_PIXEL_FORMAT_XRGB_8888,
   -1
};

/* Mapping of Allegro formats to D3D formats */
static int d3d_formats[] = {
   ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA,
   ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA,
   D3DFMT_A8R8G8B8,
   D3DFMT_A4R4G4B4,
   D3DFMT_R5G6B5,
   D3DFMT_A1R5G5B5,
   D3DFMT_X8R8G8B8,
   -1
};

/* Dummy graphics driver for compatibility */
GFX_DRIVER _al_d3d_dummy_gfx_driver = {
   0,
   "D3D Compatibility GFX driver",
   "D3D Compatibility GFX driver",
   "D3D Compatibility GFX driver",
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   0, 0,
   0,
   0,
   0,
   0,
   0,
   0,
   0,
   _al_win_directx_create_mouse_cursor,
   _al_win_directx_destroy_mouse_cursor,
   _al_win_directx_set_mouse_cursor,
   _al_win_directx_set_system_mouse_cursor,
   _al_win_directx_show_mouse_cursor,
   _al_win_directx_hide_mouse_cursor
};


void _al_d3d_lock_device()
{
   _al_mutex_lock(&d3d_device_mutex);
}

void _al_d3d_unlock_device()
{
   _al_mutex_unlock(&d3d_device_mutex);
}


int _al_format_to_d3d(int format)
{
   int i;
   D3DDISPLAYMODE d3d_dm;

   for (i = 0; allegro_formats[i] >= 0; i++) {
      if (!_al_pixel_format_is_real(allegro_formats[i]))
         continue;
      if (allegro_formats[i] == format) {
         return d3d_formats[i];
      }
   }

   /* If not found or ALLEGRO_PIXEL_FORMAT_ANY_*, return desktop format */
   
   IDirect3D9_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);

   return d3d_dm.Format;
}

int _al_d3d_format_to_allegro(int d3d_fmt)
{
   int i;

   for (i = 0; d3d_formats[i] >= 0; i++) {
      if (!_al_pixel_format_is_real(allegro_formats[i]))
         continue;
      if (d3d_formats[i] == d3d_fmt) {
         return allegro_formats[i];
      }
   }

   return -1;
}

static int d3d_al_color_to_d3d(int format, ALLEGRO_COLOR *color)
{
   unsigned char r, g, b, a;
   __al_unmap_rgba(format, color, &r, &g, &b, &a);
   return D3DCOLOR_ARGB(a, r, g, b);
}

static bool d3d_format_is_valid(int format)
{
   int i;

   for (i = 0; allegro_formats[i] >= 0; i++) {
      if (allegro_formats[i] == format)
         return true;
   }

   return false;
}


static bool d3d_parameters_are_valid(int format, int refresh_rate, int flags)
{
   if (!d3d_format_is_valid(format))
      return false;
   
   return true;
}

static void d3d_reset_state()
{
   if (_al_d3d_is_device_lost()) return;

   IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ZENABLE, D3DZB_FALSE);
   IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ZWRITEENABLE, FALSE);
   IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_LIGHTING, FALSE);
   IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_CULLMODE, D3DCULL_NONE);
   IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ALPHABLENDENABLE, TRUE);
}

void _al_d3d_get_current_ortho_projection_parameters(float *w, float *h)
{
   *w = d3d_ortho_w;
   *h = d3d_ortho_h;
}

static void d3d_get_ortho_matrix(float w, float h, D3DMATRIX *matrix)
{
   float left = 0.0f;
   float right = w;
   float top = 0.0f;
   float bottom = h;
   float neer = -1.0f;
   float farr = 1.0f;

   matrix->m[1][0] = 0.0f;
   matrix->m[2][0] = 0.0f;
   matrix->m[0][1] = 0.0f;
   matrix->m[2][1] = 0.0f;
   matrix->m[0][2] = 0.0f;
   matrix->m[1][2] = 0.0f;
   matrix->m[0][3] = 0.0f;
   matrix->m[1][3] = 0.0f;
   matrix->m[2][3] = 0.0f;

   matrix->m[0][0] = 2.0f / (right - left);
   matrix->m[1][1] = 2.0f / (top - bottom);
   matrix->m[2][2] = 2.0f / (farr - neer);

   matrix->m[3][0] = -((right+left)/(right-left));
   matrix->m[3][1] = -((top+bottom)/(top-bottom));
   matrix->m[3][2] = -((farr+neer)/(farr-neer));
   matrix->m[3][3] = 1.0f;
}

static void d3d_get_identity_matrix(D3DMATRIX *matrix)
{
   int i, j;
   int one = 0;

   for (i = 0; i < 4; i++) {
      for (j = 0; j < 4; j++) {
         if (j == one)
            matrix->m[j][i] = 1.0f;
         else
            matrix->m[j][i] = 0.0f;
      }
      one++;
   }
}

static void _al_d3d_set_ortho_projection(float w, float h)
{
   D3DMATRIX matOrtho;
   D3DMATRIX matIdentity;

   if (_al_d3d_is_device_lost()) return;

   d3d_ortho_w = w;
   d3d_ortho_h = h;

   d3d_get_identity_matrix(&matIdentity);
   d3d_get_ortho_matrix(w, h, &matOrtho);

   _al_d3d_lock_device();

   IDirect3DDevice9_SetTransform(_al_d3d_device, D3DTS_PROJECTION, &matOrtho);
   IDirect3DDevice9_SetTransform(_al_d3d_device, D3DTS_WORLD, &matIdentity);
   IDirect3DDevice9_SetTransform(_al_d3d_device, D3DTS_VIEW, &matIdentity);

   _al_d3d_unlock_device();
}

static bool d3d_display_mode_matches(D3DDISPLAYMODE *dm, int w, int h, int format, int refresh_rate)
{
   if ((dm->Width == (unsigned int)w) &&
       (dm->Height == (unsigned int)h) &&
       ((!refresh_rate) || (dm->RefreshRate == (unsigned int)refresh_rate)) &&
       (dm->Format == (unsigned int)_al_format_to_d3d(format))) {
          return true;
   }
   return false;
}

static int d3d_get_max_refresh_rate(int w, int h, int format)
{
   UINT num_modes = IDirect3D9_GetAdapterModeCount(_al_d3d, D3DADAPTER_DEFAULT, _al_format_to_d3d(format));
   UINT i;
   D3DDISPLAYMODE display_mode;
   UINT max = 0;

   for (i = 0; i < num_modes; i++) {
      if (IDirect3D9_EnumAdapterModes(_al_d3d, D3DADAPTER_DEFAULT, _al_format_to_d3d(format), i, &display_mode) != D3D_OK) {
         TRACE("d3d_get_max_refresh_rate: EnumAdapterModes failed.\n");
         return -1;
      }
      if (d3d_display_mode_matches(&display_mode, w, h, format, 0)) {
         if (display_mode.RefreshRate > max) {
            max = display_mode.RefreshRate;
         }
      }
   }

   return max;
}

static bool d3d_check_mode(int w, int h, int format, int refresh_rate)
{
   UINT num_modes = IDirect3D9_GetAdapterModeCount(_al_d3d, D3DADAPTER_DEFAULT, _al_format_to_d3d(format));
   UINT i;
   D3DDISPLAYMODE display_mode;

   for (i = 0; i < num_modes; i++) {
      if (IDirect3D9_EnumAdapterModes(_al_d3d, D3DADAPTER_DEFAULT, _al_format_to_d3d(format), i, &display_mode) != D3D_OK) {
         TRACE("d3d_check_mode: EnumAdapterModes failed.\n");
         return false;
      }
      if (d3d_display_mode_matches(&display_mode, w, h, format, refresh_rate)) {
         return true;
      }
   }

   return false;
}

/* FIXME: release swap chain too? */
static bool d3d_create_hidden_device()
{
   D3DDISPLAYMODE d3d_dm;
   LPDIRECT3DSURFACE9 render_target;
   int ret;

   d3d_hidden_window = _al_win_create_hidden_window();
   if (d3d_hidden_window == 0) {
      TRACE("Failed to create hidden window.\n");
      return 0;
   }

   IDirect3D9_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);

   ZeroMemory(&d3d_pp, sizeof(d3d_pp));
   d3d_pp.BackBufferFormat = d3d_dm.Format;
   d3d_pp.BackBufferWidth = 100;
   d3d_pp.BackBufferHeight = 100;
   d3d_pp.BackBufferCount = 1;
   d3d_pp.Windowed = 1;
   d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
   d3d_pp.hDeviceWindow = d3d_hidden_window;

   /* FIXME: try hardware vertex processing first? */
   if ((ret = IDirect3D9_CreateDevice(_al_d3d, D3DADAPTER_DEFAULT,
         D3DDEVTYPE_HAL, d3d_hidden_window,
         D3DCREATE_HARDWARE_VERTEXPROCESSING,
         &d3d_pp, &_al_d3d_device)) != D3D_OK) {
      if ((ret = IDirect3D9_CreateDevice(_al_d3d, D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL, d3d_hidden_window,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &d3d_pp, &_al_d3d_device)) != D3D_OK) {
         switch (ret) {
            case D3DERR_INVALIDCALL:
               TRACE("D3DERR_INVALIDCALL in create_device.\n");
               break;
            case D3DERR_NOTAVAILABLE:
               TRACE("D3DERR_NOTAVAILABLE in create_device.\n");
               break;
            case D3DERR_OUTOFVIDEOMEMORY:
               TRACE("D3DERR_OUTOFVIDEOMEMORY in create_device.\n");
               break;
            default:
               TRACE("Direct3D Device creation failed.\n");
               break;
         }
         return 0;
      }
   }

   /* We won't be using this surface, so release it immediately */

   IDirect3DDevice9_GetRenderTarget(_al_d3d_device, 0, &render_target);
   IDirect3DSurface9_Release(render_target);

   TRACE("Direct3D device created.\n");

   return 1;
}

static bool d3d_create_fullscreen_device(ALLEGRO_DISPLAY_D3D *d,
   int format, int refresh_rate, int flags)
{
   int ret;

   if (!d3d_check_mode(d->display.w, d->display.h, format, refresh_rate)) {
      TRACE("d3d_create_fullscreen_device: Mode not supported.\n");
      return 0;
   }

   ZeroMemory(&d3d_pp, sizeof(d3d_pp));
   d3d_pp.BackBufferFormat = _al_format_to_d3d(format);
   d3d_pp.BackBufferWidth = d->display.w;
   d3d_pp.BackBufferHeight = d->display.h;
   d3d_pp.BackBufferCount = 1;
   d3d_pp.Windowed = 0;
   d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
   d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
   if (flags & ALLEGRO_SINGLEBUFFER) {
      d3d_pp.SwapEffect = D3DSWAPEFFECT_COPY;
   }
   else {
      d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
   }
   d3d_pp.hDeviceWindow = d->window;
   if (refresh_rate) {
      d3d_pp.FullScreen_RefreshRateInHz = refresh_rate;
   }
   else {
      d3d_pp.FullScreen_RefreshRateInHz =
         d3d_get_max_refresh_rate(d->display.w, d->display.h, format);
   }

   /* FIXME: try hardware vertex processing first? */
   if ((ret = IDirect3D9_CreateDevice(_al_d3d, D3DADAPTER_DEFAULT,
         D3DDEVTYPE_HAL, d->window,
         D3DCREATE_HARDWARE_VERTEXPROCESSING,
         &d3d_pp, &_al_d3d_device)) != D3D_OK) {
      if ((ret = IDirect3D9_CreateDevice(_al_d3d, D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL, d->window,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING,
            &d3d_pp, &_al_d3d_device)) != D3D_OK) {
         switch (ret) {
            case D3DERR_INVALIDCALL:
               TRACE("D3DERR_INVALIDCALL in create_device.\n");
               break;
            case D3DERR_NOTAVAILABLE:
               TRACE("D3DERR_NOTAVAILABLE in create_device.\n");
               break;
            case D3DERR_OUTOFVIDEOMEMORY:
               TRACE("D3DERR_OUTOFVIDEOMEMORY in create_device.\n");
               break;
            default:
               TRACE("Direct3D Device creation failed.\n");
               break;
         }
         return 0;
      }
   }

   IDirect3DDevice9_GetRenderTarget(_al_d3d_device, 0, &d->render_target);
   IDirect3DDevice9_GetSwapChain(_al_d3d_device, 0, &d->swap_chain);

   d3d_reset_state();

   TRACE("Fullscreen Direct3D device created.\n");

   return 1;
}

static void d3d_destroy_device()
{
   IDirect3DDevice9_Release(_al_d3d_device);
   _al_d3d_device = NULL;
}

static void d3d_destroy_hidden_device()
{
   d3d_destroy_device();
   DestroyWindow(d3d_hidden_window);
   d3d_hidden_window = 0;
}

bool _al_d3d_render_to_texture_supported()
{
   return render_to_texture_supported;
}

bool _al_d3d_init_display()
{
   if ((_al_d3d = Direct3DCreate9(D3D9b_SDK_VERSION)) == NULL) {
      TRACE("Direct3DCreate9 failed.\n");
      return false;
   }

   D3DDISPLAYMODE d3d_dm;

   IDirect3D9_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);

   if (IDirect3D9_CheckDeviceFormat(_al_d3d, D3DADAPTER_DEFAULT,
         D3DDEVTYPE_HAL, d3d_dm.Format, D3DUSAGE_RENDERTARGET,
         D3DRTYPE_TEXTURE, d3d_dm.Format) != D3D_OK)
      render_to_texture_supported = false;
   else
      render_to_texture_supported = true;

   _al_mutex_init(&d3d_device_mutex);

   return true;
}

static bool d3d_create_swap_chain(ALLEGRO_DISPLAY_D3D *d,
   int format, int refresh_rate, int flags)
{
   ZeroMemory(&d3d_pp, sizeof(d3d_pp));
   d3d_pp.BackBufferFormat = _al_format_to_d3d(format);
   d3d_pp.BackBufferWidth = d->display.w;
   d3d_pp.BackBufferHeight = d->display.h;
   d3d_pp.BackBufferCount = 1;
   d3d_pp.Windowed = 1;
   d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
   d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
   if (flags & ALLEGRO_SINGLEBUFFER) {
      d3d_pp.SwapEffect = D3DSWAPEFFECT_COPY;
   }
   else {
      d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
   }
   d3d_pp.hDeviceWindow = d->window;

   HRESULT hr;

   if ((hr = IDirect3DDevice9_CreateAdditionalSwapChain(_al_d3d_device, &d3d_pp, &d->swap_chain)) != D3D_OK) {
      TRACE("d3d_create_swap_chain: CreateAdditionalSwapChain failed.\n");
      return 0;
   }

   if (IDirect3DSwapChain9_GetBackBuffer(d->swap_chain, 0, D3DBACKBUFFER_TYPE_MONO, &d->render_target) != D3D_OK) {
      IDirect3DSwapChain9_Release(d->swap_chain);
      TRACE("d3d_create_swap_chain: GetBackBuffer failed.\n");
      return 0;
   }

   d3d_reset_state();

   return 1;
}

static void d3d_destroy_display_internals(ALLEGRO_DISPLAY_D3D *display)
{
   unsigned int i;

   if (d3d_current_texture_render_target) {
      IDirect3DSurface9_Release(d3d_current_texture_render_target);
   }
   if (display->render_target)
      IDirect3DSurface9_Release(display->render_target);
   if (display->swap_chain)
      IDirect3DSwapChain9_Release(display->swap_chain);

   if (!(display->display.flags & ALLEGRO_FULLSCREEN)) {
      if (d3d_created_displays._size <= 1) {
         d3d_destroy_hidden_device();
      }
   }

   display->end_thread = true;
   while (!display->thread_ended)
      al_rest(1);
   
   d3d_already_fullscreen = false;

   _al_win_ungrab_input();

   PostMessage(display->window, _al_win_msg_suicide, 0, 0);
   
   _al_event_source_free(&display->display.es);
}

static void d3d_destroy_display(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_SYSTEM_WIN *system = (ALLEGRO_SYSTEM_WIN *)al_system_driver();

   d3d_destroy_display_internals((ALLEGRO_DISPLAY_D3D *)display);

   _al_vector_find_and_delete(&system->system.displays, &display);

   _al_vector_find_and_delete(&d3d_created_displays, &display);

   if (d3d_created_displays._size > 0) {
      ALLEGRO_DISPLAY_D3D **dptr = _al_vector_ref(&d3d_created_displays, 0);
      ALLEGRO_DISPLAY_D3D *d = *dptr;
      _al_win_wnd = d->window;
      win_grab_input();
   }
   else {
      gfx_driver = 0;
   }

   _AL_FREE(display);
}

void _al_d3d_prepare_for_reset()
{
   unsigned int i;

   if (!d3d_bitmaps_prepared_for_reset) {
      _al_d3d_prepare_bitmaps_for_reset();
      d3d_bitmaps_prepared_for_reset = true;
   }
   _al_d3d_release_default_pool_textures();

   if (d3d_current_texture_render_target) {
      IDirect3DSurface9_Release(d3d_current_texture_render_target);
      d3d_current_texture_render_target = NULL;
   }

   for (i = 0; i < d3d_created_displays._size; i++) {
      ALLEGRO_DISPLAY_D3D **dptr = _al_vector_ref(&d3d_created_displays, i);
      ALLEGRO_DISPLAY_D3D *disp = *dptr;

      IDirect3DSurface9_Release(disp->render_target);
      IDirect3DSwapChain9_Release(disp->swap_chain);
   }
}

static bool _al_d3d_reset_device()
{
   if (_al_d3d_device) {
      if (d3d_already_fullscreen) {
         unsigned int i;
         HRESULT hr;
      
         _al_d3d_lock_device();

         _al_d3d_prepare_for_reset();

         ZeroMemory(&d3d_pp, sizeof(d3d_pp));
         d3d_pp.BackBufferFormat = _al_format_to_d3d(d3d_fullscreen_display->display.format);
         d3d_pp.BackBufferWidth = d3d_fullscreen_display->display.w;
         d3d_pp.BackBufferHeight = d3d_fullscreen_display->display.h;
         d3d_pp.BackBufferCount = 1;
         d3d_pp.Windowed = 0;
         d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
         d3d_pp.hDeviceWindow = d3d_fullscreen_display->window;
         d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
         d3d_pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
         if (d3d_fullscreen_display->display.flags & ALLEGRO_SINGLEBUFFER) {
            d3d_pp.SwapEffect = D3DSWAPEFFECT_COPY;
         }
         else {
            d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
         }
         if (d3d_fullscreen_display->display.refresh_rate) {
            d3d_pp.FullScreen_RefreshRateInHz =
               d3d_fullscreen_display->display.refresh_rate;
         }
         else {
            d3d_pp.FullScreen_RefreshRateInHz =
               d3d_get_max_refresh_rate(d3d_fullscreen_display->display.w,
                  d3d_fullscreen_display->display.h,
                  d3d_fullscreen_display->display.format);
         }

         while ((hr = IDirect3DDevice9_Reset(_al_d3d_device, &d3d_pp)) != D3D_OK) {
            /* FIXME: Should we try forever? */
            TRACE("_al_d3d_reset_device: Reset failed. Trying again.\n");
            al_rest(10);
         }

         IDirect3DDevice9_GetRenderTarget(_al_d3d_device, 0, &d3d_fullscreen_display->render_target);
         IDirect3DDevice9_GetSwapChain(_al_d3d_device, 0, &d3d_fullscreen_display->swap_chain);

         _al_d3d_unlock_device();
      }
      else {
         unsigned int i;
         HRESULT hr;
         LPDIRECT3DSURFACE9 render_target;
         D3DDISPLAYMODE d3d_dm;
         
         _al_d3d_lock_device();

         _al_d3d_prepare_for_reset();

         IDirect3D9_GetAdapterDisplayMode(_al_d3d, D3DADAPTER_DEFAULT, &d3d_dm);

         ZeroMemory(&d3d_pp, sizeof(d3d_pp));
         d3d_pp.BackBufferFormat = d3d_dm.Format;
         d3d_pp.BackBufferWidth = 100;
         d3d_pp.BackBufferHeight = 100;
         d3d_pp.BackBufferCount = 1;
         d3d_pp.Windowed = 1;
         d3d_pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
         d3d_pp.hDeviceWindow = d3d_hidden_window;
         d3d_pp.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;

         /* Try to reset a few times */
         for (i = 0; i < 5; i++) {
            if (IDirect3DDevice9_Reset(_al_d3d_device, &d3d_pp) == D3D_OK) {
               break;
            }
            al_rest(100);
         }
         if (i == 5) {
            _al_d3d_unlock_device();
            return 0;
         }

         /* We don't need this render target */
         IDirect3DDevice9_GetRenderTarget(_al_d3d_device, 0, &render_target);
         IDirect3DSurface9_Release(render_target);

         for (i = 0; i < d3d_created_displays._size; i++) {
            ALLEGRO_DISPLAY_D3D **dptr = _al_vector_ref(&d3d_created_displays, i);
            ALLEGRO_DISPLAY_D3D *d = *dptr;
            ALLEGRO_DISPLAY *al_display = (ALLEGRO_DISPLAY *)d;
            int flags = 0;

            d3d_create_swap_chain(d, al_display->format, al_display->refresh_rate, al_display->flags);
         }
         
         _al_d3d_unlock_device();

      }
   }

   _al_d3d_refresh_texture_memory();

   al_set_current_display(d3d_target_display_before_device_lost);
   al_set_target_bitmap(d3d_target_bitmap_before_device_lost);

   _al_d3d_lock_device();

   d3d_reset_state();

   _al_d3d_unlock_device();
   
   d3d_bitmaps_prepared_for_reset = false;

   return 1;
}

static int d3d_choose_format(int fake)
{
   /* Pick an appropriate format if the user is vague */
   switch (fake) {
      case ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_RGB_565;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA:
      case ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_ARGB_8888;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_XRGB_8888;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_15_WITH_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_ARGB_1555;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_RGB_565;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA:
         fake = ALLEGRO_PIXEL_FORMAT_ARGB_4444;
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_15_NO_ALPHA:
      case ALLEGRO_PIXEL_FORMAT_ANY_24_WITH_ALPHA:
      case ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA:
         fake = -1;
         break;
      default:
         break;
   }

   return fake;
}

/*
 * The window and swap chain must be created in the same
 * thread that runs the message loop. It also must be
 * reset from the same thread.
 */
static void d3d_display_thread_proc(void *arg)
{
   ALLEGRO_DISPLAY_D3D *d;
   DWORD result;
   MSG msg;
   HRESULT hr;
   bool lost_event_generated = false;
   new_display_parameters *params = arg;
   D3DCAPS9 caps;

   d = params->display;

   /*
    * Direct3D will only allow 1 fullscreen swap chain, and you can't
    * combine fullscreen and windowed swap chains.
    */
   if (!params->is_resize && (d3d_already_fullscreen ||
      ((d3d_created_displays._size > 0) && params->flags & ALLEGRO_FULLSCREEN))) {
      params->display->init_failed = true;
      return;
   }

   if (!_al_pixel_format_is_real(params->format)) {
      int f = d3d_choose_format(params->format);
      if (f < 0) {
         d->init_failed = true;
         return;
      }
      params->format = f;

   }

   if (!d3d_parameters_are_valid(params->format, params->refresh_rate, params->flags)) {
      TRACE("d3d_display_thread_proc: Invalid parameters.\n");
      params->display->init_failed = true;
      return;
   }

   d->display.w = params->width;
   d->display.h = params->height;
   d->display.vt = vt;

   d->window = _al_win_create_window((ALLEGRO_DISPLAY *)d, params->width,
      params->height, params->flags);
   
   if (!d->window) {
      params->display->init_failed = true;
      return;
   }

   d->display.format = params->format;
   d->display.refresh_rate = params->refresh_rate;
   d->display.flags = params->flags;

   if (!(params->flags & ALLEGRO_FULLSCREEN)) {
      if (!d3d_create_swap_chain(d, params->format, params->refresh_rate, params->flags)) {
         d->thread_ended = true;
         d3d_destroy_display((ALLEGRO_DISPLAY *)d);
         params->display->init_failed = true;
         return;
      }
   }
   else {
      if (!d3d_create_fullscreen_device(d, params->format, params->refresh_rate, params->flags)) {
         d->thread_ended = true;
         d3d_destroy_display((ALLEGRO_DISPLAY *)d);
         params->display->init_failed = true;
         return;
      }
   }

   IDirect3DDevice9_GetDeviceCaps(_al_d3d_device, &caps);
   d3d_can_wait_for_vsync = caps.Caps & D3DCAPS_READ_SCANLINE;

   d->thread_ended = false;

   params->display->initialized = true;

   for (;;) {
      if (d->end_thread) {
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
         if (_al_d3d_device) {
            _al_d3d_lock_device();
            hr = IDirect3DDevice9_TestCooperativeLevel(_al_d3d_device);

            if (hr == D3D_OK) {
               _al_d3d_device_lost = false;
            }
            else if (hr == D3DERR_DEVICELOST) {
               /* device remains lost */
               if (!lost_event_generated) {
                  _al_event_source_lock(&d->display.es);
                  if (_al_event_source_needs_to_generate_event(&d->display.es)) {
                     AL_EVENT *event = _al_event_source_get_unused_event(&d->display.es);
                     if (event) {
                        event->display.type = AL_EVENT_DISPLAY_LOST;
                        event->display.timestamp = al_current_time();
                        _al_event_source_emit_event(&d->display.es, event);
                     }
                  }
                  _al_event_source_unlock(&d->display.es);
                  lost_event_generated = true;
               }
            }
            else if (hr == D3DERR_DEVICENOTRESET) {
               if (_al_d3d_reset_device()) {
                  _al_d3d_device_lost = false;
                  _al_event_source_lock(&d->display.es);
                  if (_al_event_source_needs_to_generate_event(&d->display.es)) {
                     AL_EVENT *event = _al_event_source_get_unused_event(&d->display.es);
                     if (event) {
                        event->display.type = AL_EVENT_DISPLAY_FOUND;
                        event->display.timestamp = al_current_time();
                        _al_event_source_emit_event(&d->display.es, event);
                     }
                  }
                  _al_event_source_unlock(&d->display.es);
                  lost_event_generated = false;
               }
            }
            _al_d3d_unlock_device();
         }
      }
   }

End:
   if (d->display.flags & ALLEGRO_FULLSCREEN) {
      _al_d3d_release_bitmap_textures();
      if (IDirect3DDevice9_Release(_al_d3d_device) != D3D_OK) {
         TRACE("Releasing fullscreen D3D device failed.\n");
      }
   }

   _al_win_delete_thread_handle(GetCurrentThreadId());

   d->thread_ended = true;

   TRACE("d3d display thread exits\n");
}

static bool d3d_create_display_internals(ALLEGRO_DISPLAY_D3D *display, bool is_resize)
{
   static new_display_parameters params;

   _al_d3d_lock_device();

   params.width = display->display.w;
   params.height = display->display.h;
   params.display = display;
   params.format = display->display.format;
   params.refresh_rate = display->display.refresh_rate;
   params.flags = display->display.flags;
   params.is_resize = is_resize;

   if (d3d_created_displays._size == 0 && !(params.flags & ALLEGRO_FULLSCREEN)) {
      if (!d3d_create_hidden_device()) {
         _al_d3d_unlock_device();
         return false;
      }
   }

   _beginthread(d3d_display_thread_proc, 0, &params);

   while (!params.display->initialized && !params.display->init_failed)
      al_rest(1);

   if (params.display->init_failed) {
      _al_d3d_unlock_device();
      return false;
   }

   win_grab_input();

   if (params.flags & ALLEGRO_FULLSCREEN) {
      d3d_already_fullscreen = true;
      d3d_fullscreen_display = params.display;
   }

   display->backbuffer_bmp.is_backbuffer = true;
   display->backbuffer_bmp.bitmap.display = (ALLEGRO_DISPLAY *)display;
   display->backbuffer_bmp.bitmap.format = display->display.format;
   display->backbuffer_bmp.bitmap.flags = 0;
   display->backbuffer_bmp.bitmap.w = display->display.w;
   display->backbuffer_bmp.bitmap.h = display->display.h;
   display->backbuffer_bmp.bitmap.cl = 0;
   display->backbuffer_bmp.bitmap.ct = 0;
   display->backbuffer_bmp.bitmap.cr = display->display.w-1;
   display->backbuffer_bmp.bitmap.cb = display->display.h-1;
   display->backbuffer_bmp.bitmap.vt = (ALLEGRO_BITMAP_INTERFACE *)_al_bitmap_d3d_driver();

   /* Alpha blending is the default */
   if (d3d_created_displays._size == 1) {
      IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_ALPHABLENDENABLE, TRUE);
      IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
   }

   _al_d3d_unlock_device();

   return true;
}


/* Create a new X11 dummy display, which maps directly to a GLX window. */
static ALLEGRO_DISPLAY *d3d_create_display(int w, int h)
{
   ALLEGRO_SYSTEM_WIN *system = (ALLEGRO_SYSTEM_WIN *)al_system_driver();
   ALLEGRO_DISPLAY_D3D *display = _AL_MALLOC(sizeof *display);
   ALLEGRO_DISPLAY_D3D **add;

   memset(display, 0, sizeof *display);
   
   display->display.w = w;
   display->display.h = h;
   display->display.format = al_get_new_display_format();
   display->display.refresh_rate = al_get_new_display_refresh_rate();
   display->display.flags = al_get_new_display_flags();

   if (!d3d_create_display_internals(display, false)) {
      TRACE("d3d_create_display failed.\n");
      _AL_FREE(display);
      return NULL;
   }

   /* Add ourself to the list of displays. */
   add = _al_vector_alloc_back(&system->system.displays);
   *add = display;

   /* Each display is an event source. */
   _al_event_source_init(&display->display.es);

   /* Keep track of the displays created */
   add = _al_vector_alloc_back(&d3d_created_displays);
   *add = display;

   /* Set up a dummy gfx_driver */
   gfx_driver = &_al_d3d_dummy_gfx_driver;
   gfx_driver->w = w;
   gfx_driver->h = h;
   gfx_driver->windowed = (display->display.flags & ALLEGRO_FULLSCREEN) ? 0 : 1;

   /* Setup the mouse */
   display->mouse_range_x1 = 0;
   display->mouse_range_y1 = 0;
   display->mouse_range_x2 = w;
   display->mouse_range_y2 = h;
   if (al_is_mouse_installed()) {
      al_set_mouse_xy(w/2, h/2);
      al_set_mouse_range(0, 0, w, h);
   }

   return (ALLEGRO_DISPLAY *)display;
}

/* FIXME: this will have to return a success/failure */
static void d3d_set_current_display(ALLEGRO_DISPLAY *d)
{
   /* Don't need to do anything */
}


static int d3d_al_blender_to_d3d(int al_mode)
{
   int num_modes = 4;

   int allegro_modes[] = {
      ALLEGRO_ZERO,
      ALLEGRO_ONE,
      ALLEGRO_ALPHA,
      ALLEGRO_INVERSE_ALPHA
   };

   int d3d_modes[] = {
      D3DBLEND_ZERO,
      D3DBLEND_ONE,
      D3DBLEND_SRCALPHA,
      D3DBLEND_INVSRCALPHA
   };

   int i;

   for (i = 0; i < num_modes; i++) {
      if (al_mode == allegro_modes[i]) {
         return d3d_modes[i];
      }
   }

   TRACE("Unknown blend mode.");

   return D3DBLEND_ONE;
}

void _al_d3d_set_blender(void)
{
   int src, dst;

   al_get_blender(&src, &dst, NULL);

   src = d3d_al_blender_to_d3d(src);
   dst = d3d_al_blender_to_d3d(dst);

   if (IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_SRCBLEND, src) != D3D_OK)
      TRACE("Failed to set source blender");
   if (IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_DESTBLEND, dst) != D3D_OK)
      TRACE("Failed to set dest blender");
}



static DWORD d3d_blend_colors(
   ALLEGRO_BITMAP *target,
   ALLEGRO_COLOR *color,
   ALLEGRO_INDEPENDANT_COLOR *bc)
{
   ALLEGRO_COLOR result;
   float r, g, b, a;

   _al_unmap_rgba_f(target, color, &r, &g, &b, &a);

   _al_map_rgba_f(target, &result,
      r*bc->r,
      g*bc->g,
      b*bc->b,
      a*bc->a);

   return d3d_al_color_to_d3d(target->format, &result);
}

/* Dummy implementation of line. */
static void d3d_draw_line(ALLEGRO_DISPLAY *d, float fx, float fy, float tx, float ty,
   ALLEGRO_COLOR *color)
{
   static D3D_COLORED_VERTEX points[2] = { { 0.0f, 0.0f, 0.0f, 0 }, };
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_INDEPENDANT_COLOR *bc = _al_get_blend_color();
   DWORD d3d_color;
   
   if (_al_d3d_is_device_lost()) return;

   if (!_al_d3d_render_to_texture_supported()) {
      _al_draw_line_memory(fx, fy, tx, ty, color);
      return;
   }

   d3d_color = d3d_blend_colors(target, color, bc);

   if (target->parent) {
      fx += target->xofs;
      tx += target->xofs;
      fy += target->yofs;
      ty += target->yofs;
   }

   points[0].x = fx;
   points[0].y = fy;
   points[0].color = d3d_color;

   points[1].x = tx;
   points[1].y = ty;
   points[1].color = d3d_color;

   _al_d3d_lock_device();

   _al_d3d_set_blender();

   IDirect3DDevice9_SetFVF(_al_d3d_device, D3DFVF_COLORED_VERTEX);

   if (IDirect3DDevice9_DrawPrimitiveUP(_al_d3d_device, D3DPT_LINELIST, 1,
         points, sizeof(D3D_COLORED_VERTEX)) != D3D_OK) {
      TRACE("DrawPrimitive failed in d3d_draw_line.\n");
   }
   
   _al_d3d_unlock_device();
   
   if (target->flags & ALLEGRO_SYNC_MEMORY_COPY) {
      _al_d3d_sync_bitmap(target);
   }
}
 
/* Dummy implementation of a filled rectangle. */
static void d3d_draw_rectangle(ALLEGRO_DISPLAY *d, float tlx, float tly,
   float brx, float bry, ALLEGRO_COLOR *color, int flags)
{
   D3DRECT rect;
   float w = brx - tlx;
   float h = bry - tly;
   ALLEGRO_BITMAP *target;
   ALLEGRO_INDEPENDANT_COLOR *bc = _al_get_blend_color();
   DWORD d3d_color;

   if (!(flags & ALLEGRO_FILLED)) {
      d3d_draw_line(d, tlx, tly, brx, tly, color);
      d3d_draw_line(d, tlx, bry, brx, bry, color);
      d3d_draw_line(d, tlx, tly, tlx, bry, color);
      d3d_draw_line(d, brx, tly, brx, bry, color);
      return;
   }

   if (_al_d3d_is_device_lost()) return;
   
   if (!_al_d3d_render_to_texture_supported()) {
      _al_draw_rectangle_memory(tlx, tly, brx, bry, color, flags);
      return;
   }

   target = al_get_target_bitmap();

   d3d_color = d3d_blend_colors(target, color, bc);
   
   if (w < 1 || h < 1) {
      return;
   }

   if (target->parent) {
      tlx += target->xofs;
      brx += target->xofs;
      tly += target->yofs;
      bry += target->yofs;
   }
   
   rect.x1 = tlx;
   rect.y1 = tly;
   rect.x2 = brx;
   rect.y2 = bry;

   _al_d3d_lock_device();

   _al_d3d_set_blender();

   _al_d3d_draw_textured_quad(NULL,
      0.0f, 0.0f, w, h,
      tlx, tly, w, h,
      w/2, h/2, 0.0f,
      d3d_color, 0);

   _al_d3d_unlock_device();
   
   if (target->flags & ALLEGRO_SYNC_MEMORY_COPY) {
      _al_d3d_sync_bitmap(target);
   }
}

/* Dummy implementation of clear. */
static void d3d_clear(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color)
{
   ALLEGRO_DISPLAY_D3D* disp = (ALLEGRO_DISPLAY_D3D*)d;
   D3DRECT rect;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   rect.x1 = 0;
   rect.y1 = 0;
   rect.x2 = target->w;
   rect.y2 = target->h;

   /* Clip */
   if (rect.x1 < target->cl)
      rect.x1 = target->cl;
   if (rect.y1 < target->ct)
      rect.y1 = target->ct;
   if (rect.x2 > target->cr)
      rect.x2 = target->cr;
   if (rect.y2 > target->cb)
      rect.y2 = target->cb;

   if (target->parent) {
      rect.x1 += target->xofs;
      rect.y1 += target->yofs;
      rect.x2 += target->xofs;
      rect.y2 += target->yofs;
   }

   d3d_draw_rectangle(d, rect.x1, rect.y1, rect.x2+1, rect.y2+1, color, ALLEGRO_FILLED);
}

/* Dummy implementation of flip. */
static void d3d_flip_display(ALLEGRO_DISPLAY *d)
{
   ALLEGRO_DISPLAY_D3D* disp = (ALLEGRO_DISPLAY_D3D*)d;
   HRESULT hr;
   
   if (_al_d3d_is_device_lost()) return;
   _al_d3d_lock_device();

   IDirect3DDevice9_EndScene(_al_d3d_device);

   hr = IDirect3DSwapChain9_Present(disp->swap_chain, 0, 0, disp->window, NULL, 0);

   if (hr == D3DERR_DEVICELOST) {
      _al_d3d_device_lost = true;
      _al_d3d_unlock_device();
      return;
   }

   IDirect3DDevice9_BeginScene(_al_d3d_device);
   
   _al_d3d_unlock_device();
}

static bool d3d_update_display_region(ALLEGRO_DISPLAY *d,
   int x, int y,
   int width, int height)
{
   ALLEGRO_DISPLAY_D3D* disp = (ALLEGRO_DISPLAY_D3D*)d;
   HRESULT hr;
   RGNDATA *rgndata;
   bool ret;
   
   if (_al_d3d_is_device_lost()) return false;
   _al_d3d_lock_device();

   if (d->flags & ALLEGRO_SINGLEBUFFER) {
      ALLEGRO_DISPLAY_D3D* disp = (ALLEGRO_DISPLAY_D3D*)d;

      IDirect3DDevice9_EndScene(_al_d3d_device);

      RECT rect;
      rect.left = x;
      rect.right = x+width;
      rect.top = y;
      rect.bottom = y+height;

      rgndata = malloc(sizeof(RGNDATA)+sizeof(RECT)-1);
      rgndata->rdh.dwSize = sizeof(RGNDATAHEADER);
      rgndata->rdh.iType = RDH_RECTANGLES;
      rgndata->rdh.nCount = 1;
      rgndata->rdh.nRgnSize = sizeof(RECT);
      memcpy(&rgndata->rdh.rcBound, &rect, sizeof(RECT));
      memcpy(rgndata->Buffer, &rect, sizeof(RECT));

      hr = IDirect3DSwapChain9_Present(disp->swap_chain, &rect, &rect, disp->window, rgndata, 0);

      free(rgndata);

      if (hr == D3DERR_DEVICELOST) {
         _al_d3d_device_lost = true;
         _al_d3d_unlock_device();
         return true;
      }

      IDirect3DDevice9_BeginScene(_al_d3d_device);

      ret = true;
   }
   else {
      ret = false;
   }
   
   _al_d3d_unlock_device();

   return ret;
}

/*
 * Sets a clipping rectangle
 */
static void d3d_set_bitmap_clip(ALLEGRO_BITMAP *bitmap)
{
   float plane[4];
   int left, right, top, bottom;

   if (bitmap->parent) {
      left = bitmap->xofs + bitmap->cl;
      right = bitmap->xofs + bitmap->cr;
      top = bitmap->yofs + bitmap->ct;
      bottom = bitmap->yofs + bitmap->cb;
   }
   else {
      left = bitmap->cl;
      right = bitmap->cr;
      top = bitmap->ct;
      bottom = bitmap->cb;
      if (left == 0 && top == 0 &&
            right == (bitmap->w-1) &&
            bottom == (bitmap->h-1)) {
         IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_CLIPPLANEENABLE, 0);
         return;
      }
   }

   //right--;
   //bottom--;

   plane[0] = 1.0f / left;
   plane[1] = 0.0f;
   plane[2] = 0.0f;
   plane[3] = -1;
   IDirect3DDevice9_SetClipPlane(_al_d3d_device, 0, plane);

   plane[0] = -1.0f / right;
   plane[1] = 0.0f;
   plane[2] = 0.0f;
   plane[3] = 1;
   IDirect3DDevice9_SetClipPlane(_al_d3d_device, 1, plane);

   plane[0] = 0.0f;
   plane[1] = 1.0f / top;
   plane[2] = 0.0f;
   plane[3] = -1;
   IDirect3DDevice9_SetClipPlane(_al_d3d_device, 2, plane);

   plane[0] = 0.0f;
   plane[1] = -1.0f / bottom;
   plane[2] = 0.0f;
   plane[3] = 1;
   IDirect3DDevice9_SetClipPlane(_al_d3d_device, 3, plane);

   /* Enable the first four clipping planes */
   IDirect3DDevice9_SetRenderState(_al_d3d_device, D3DRS_CLIPPLANEENABLE, 0xF);
}

static bool d3d_resize_display(ALLEGRO_DISPLAY *d, int width, int height)
{
   ALLEGRO_DISPLAY_D3D *disp = (ALLEGRO_DISPLAY_D3D *)d;

   if (d3d_already_fullscreen) {
      d3d_destroy_display_internals(disp);
      d->w = width;
      d->h = height;
      disp->end_thread = false;
      disp->initialized = false;
      disp->init_failed = false;
      disp->thread_ended = false;
      if (!d3d_create_display_internals(disp, true)) {
         _AL_FREE(disp);
         return false;
      }
      al_set_current_display(d);
      al_set_target_bitmap(al_get_backbuffer());
      _al_d3d_recreate_bitmap_textures();
      return true;
   }
   else {
      bool ret;
      RECT win_size;
      WINDOWINFO wi;

      //_al_d3d_reset_device();

      win_size.left = 0;
      win_size.top = 0;
      win_size.right = width;
      win_size.bottom = height;

      wi.cbSize = sizeof(WINDOWINFO);
      GetWindowInfo(disp->window, &wi);

      AdjustWindowRectEx(&win_size, wi.dwStyle, FALSE, wi.dwExStyle);

      ret = SetWindowPos(disp->window, HWND_TOP,
         0, 0,
         win_size.right-win_size.left,
         win_size.bottom-win_size.top,
         SWP_NOMOVE|SWP_NOZORDER);

      /*
       * The clipping rectangle and bitmap size must be
       * changed to match the new size.
       */
      _al_push_target_bitmap();
      al_set_target_bitmap(&disp->backbuffer_bmp.bitmap);
      disp->backbuffer_bmp.bitmap.w = width;
      disp->backbuffer_bmp.bitmap.h = height;
      al_set_clipping_rectangle(0, 0, width-1, height-1);
      d3d_set_bitmap_clip(&disp->backbuffer_bmp.bitmap);
      _al_pop_target_bitmap();

      return ret;

      /*
      wi.cbSize = sizeof(WINDOWINFO);
      GetWindowInfo(disp->window, &wi);
      x = wi.rcClient.left;
      y = wi.rcClient.top;

      _al_event_source_lock(es);
      if (_al_event_source_needs_to_generate_event(es)) {
         AL_EVENT *event = _al_event_source_get_unused_event(es);
         if (event) {
            event->display.type = AL_EVENT_DISPLAY_RESIZE;
            event->display.timestamp = al_current_time();
            event->display.x = x;
            event->display.y = y;
            event->display.width = width;
            event->display.height = height;
            _al_event_source_emit_event(es, event);
         }
      }
      _al_event_source_unlock(es);
      */

   }
}

static bool d3d_acknowledge_resize(ALLEGRO_DISPLAY *d)
{
   if (_al_d3d_device) {
      WINDOWINFO wi;
      ALLEGRO_DISPLAY_D3D *disp = (ALLEGRO_DISPLAY_D3D *)d;

      wi.cbSize = sizeof(WINDOWINFO);
      GetWindowInfo(disp->window, &wi);
      d->w = wi.rcClient.right - wi.rcClient.left;
      d->h = wi.rcClient.bottom - wi.rcClient.top;

      return _al_d3d_reset_device();
   }

   return false;
}

/*
 * Returns false if the device is not in a usable state.
 */
bool _al_d3d_is_device_lost()
{
   HRESULT hr;

   return (_al_d3d_device_lost);
}

/*
 * Upload a rectangle of a compatibility bitmap.
 */
static void d3d_upload_compat_screen(BITMAP *bitmap, int x, int y, int w, int h)
{
   int cx;
   int cy;
   int pixel;
   LPDIRECT3DSURFACE9 render_target;
   RECT rect;
   D3DLOCKED_RECT locked_rect;

   if (_al_d3d_is_device_lost()) return;
   _al_d3d_lock_device();

   x += bitmap->x_ofs;
   y += bitmap->y_ofs;
   
   if (x < 0) {
      w -= -x;
      x = 0;
   }
   if (y < 0) {
      h -= -y;
      y = 0;
   }

   if (w <= 0 || h <= 0) {
      _al_d3d_unlock_device();
      return;
   }

   if (IDirect3DDevice9_GetRenderTarget(_al_d3d_device, 0, &render_target) != D3D_OK) {
      TRACE("d3d_upload_compat_bitmap: GetRenderTarget failed.\n");
      _al_d3d_unlock_device();
      return;
   }

   rect.left = x;
   rect.top = y;
   rect.right = x + w;
   rect.bottom = y + h;

   if (IDirect3DSurface9_LockRect(render_target, &locked_rect, &rect, 0) != D3D_OK) {
      IDirect3DSurface9_Release(render_target);
      _al_d3d_unlock_device();
      return;
   }

   _al_convert_compat_bitmap(
      screen,
      locked_rect.pBits, _al_current_display->format, locked_rect.Pitch,
      x, y, 0, 0, w, h);

   IDirect3DSurface9_UnlockRect(render_target);
   IDirect3DSurface9_Release(render_target);

   _al_d3d_unlock_device();

   al_update_display_region(x, y, w, h);
}

ALLEGRO_BITMAP *_al_d3d_create_bitmap(ALLEGRO_DISPLAY *d,
   int w, int h)
{
   ALLEGRO_BITMAP_D3D *bitmap = (ALLEGRO_BITMAP_D3D*)_AL_MALLOC(sizeof *bitmap);
   int format;
   int flags;

   format = al_get_new_bitmap_format();
   flags = al_get_new_bitmap_flags();

   if (!_al_pixel_format_is_real(format)) {
      format = d3d_choose_format(format);
      if (format < 0) {
         return NULL;
      }
   }
   
   bitmap->bitmap.vt = _al_bitmap_d3d_driver();
   bitmap->bitmap.memory = NULL;
   bitmap->bitmap.format = format;
   bitmap->bitmap.flags = flags;

   bitmap->video_texture = 0;
   bitmap->system_texture = 0;
   bitmap->initialized = false;
   bitmap->is_backbuffer = false;

   return &bitmap->bitmap;
}

ALLEGRO_BITMAP *d3d_create_sub_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *parent,
   int x, int y, int width, int height)
{
   ALLEGRO_BITMAP_D3D *bitmap = (ALLEGRO_BITMAP_D3D*)_AL_MALLOC(sizeof *bitmap);
   bitmap->texture_w = 0;
   bitmap->texture_h = 0;
   bitmap->video_texture = NULL;
   bitmap->system_texture = NULL;
   bitmap->initialized = false;
   bitmap->is_backbuffer = ((ALLEGRO_BITMAP_D3D *)parent)->is_backbuffer;
   bitmap->bitmap.vt = parent->vt;
   return (ALLEGRO_BITMAP *)bitmap;
}

static void d3d_set_target_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   ALLEGRO_DISPLAY_D3D *d3d_display = (ALLEGRO_DISPLAY_D3D *)bitmap->display;
   ALLEGRO_BITMAP *target;
   ALLEGRO_BITMAP_D3D *d3d_target;

   if (_al_d3d_is_device_lost()) return;
   
   if (bitmap->parent) {
      target = bitmap->parent;
   }
   else {
      target = bitmap;
   }
   d3d_target = (ALLEGRO_BITMAP_D3D *)target;

   /* Release the previous render target */
   if (d3d_current_texture_render_target != NULL) {
      _al_d3d_lock_device();
      IDirect3DSurface9_Release(d3d_current_texture_render_target);
      _al_d3d_unlock_device();
      d3d_current_texture_render_target = NULL;
   }

   /* Set the render target */
   if (d3d_target->is_backbuffer) {
      if (IDirect3DDevice9_SetRenderTarget(_al_d3d_device, 0, d3d_display->render_target) != D3D_OK) {
         TRACE("d3d_set_target_bitmap: Unable to set render target to texture surface.\n");
         IDirect3DSurface9_Release(d3d_current_texture_render_target);
         return;
      }
      _al_d3d_set_ortho_projection(display->w, display->h);
      _al_d3d_lock_device();
      IDirect3DDevice9_BeginScene(_al_d3d_device);
      d3d_target_display_before_device_lost = display;
      d3d_target_bitmap_before_device_lost = target;
      _al_d3d_unlock_device();
   }
   else {
      d3d_target_display_before_device_lost = display;
      d3d_target_bitmap_before_device_lost = target;
      if (_al_d3d_render_to_texture_supported()) {
         _al_d3d_lock_device();
         IDirect3DDevice9_EndScene(_al_d3d_device);
         if (IDirect3DTexture9_GetSurfaceLevel(d3d_target->video_texture, 0, &d3d_current_texture_render_target) != D3D_OK) {
            TRACE("d3d_set_target_bitmap: Unable to get texture surface level.\n");
            return;
         }
         if (IDirect3DDevice9_SetRenderTarget(_al_d3d_device, 0, d3d_current_texture_render_target) != D3D_OK) {
            TRACE("d3d_set_target_bitmap: Unable to set render target to texture surface.\n");
            IDirect3DSurface9_Release(d3d_current_texture_render_target);
            return;
         }
         _al_d3d_unlock_device();
         _al_d3d_set_ortho_projection(d3d_target->texture_w, d3d_target->texture_h);
         _al_d3d_lock_device();
         IDirect3DDevice9_BeginScene(_al_d3d_device);
         _al_d3d_unlock_device();
      }
   }

   _al_d3d_lock_device();

   d3d_reset_state();

   d3d_set_bitmap_clip(bitmap);

   _al_d3d_unlock_device();
}

static ALLEGRO_BITMAP *d3d_get_backbuffer()
{
   return (ALLEGRO_BITMAP *)&(((ALLEGRO_DISPLAY_D3D *)_al_current_display)->backbuffer_bmp);
}

static ALLEGRO_BITMAP *d3d_get_frontbuffer()
{
   return NULL;
}

static bool d3d_is_compatible_bitmap(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP *bitmap)
{
   return true;
}

static void d3d_switch_out(ALLEGRO_DISPLAY *display)
{
   _al_d3d_prepare_bitmaps_for_reset();
   d3d_bitmaps_prepared_for_reset = true;
}

static void  d3d_switch_in(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_D3D *d3d_disp = (ALLEGRO_DISPLAY_D3D *)display;

   if (al_is_mouse_installed())
      al_set_mouse_range(d3d_disp->mouse_range_x1, d3d_disp->mouse_range_y1,
         d3d_disp->mouse_range_x2, d3d_disp->mouse_range_y2);
}

static bool d3d_wait_for_vsync(ALLEGRO_DISPLAY *display)
{
   ALLEGRO_DISPLAY_D3D *d3d_display;
   D3DRASTER_STATUS status;

   if (!d3d_can_wait_for_vsync)
      return false;

   d3d_display = (ALLEGRO_DISPLAY_D3D *)display;

   do {
      IDirect3DSwapChain9_GetRasterStatus(d3d_display->swap_chain, &status);
   } while (!status.InVBlank);

   return true;
}

/* Obtain a reference to this driver. */
ALLEGRO_DISPLAY_INTERFACE *_al_display_d3d_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->create_display = d3d_create_display;
   vt->destroy_display = d3d_destroy_display;
   vt->set_current_display = d3d_set_current_display;
   vt->clear = d3d_clear;
   vt->draw_line = d3d_draw_line;
   vt->draw_rectangle = d3d_draw_rectangle;
   vt->flip_display = d3d_flip_display;
   vt->update_display_region = d3d_update_display_region;
   vt->acknowledge_resize = d3d_acknowledge_resize;
   vt->resize_display = d3d_resize_display;
   vt->create_bitmap = _al_d3d_create_bitmap;
   vt->upload_compat_screen = d3d_upload_compat_screen;
   vt->set_target_bitmap = d3d_set_target_bitmap;
   vt->get_backbuffer = d3d_get_backbuffer;
   vt->get_frontbuffer = d3d_get_frontbuffer;
   vt->is_compatible_bitmap = d3d_is_compatible_bitmap;
   vt->switch_out = d3d_switch_out;
   vt->switch_in = d3d_switch_in;
   vt->draw_memory_bitmap_region = NULL;
   vt->create_sub_bitmap = d3d_create_sub_bitmap;
   vt->wait_for_vsync = d3d_wait_for_vsync;

   return vt;
}

int _al_d3d_get_num_display_modes(int format, int refresh_rate, int flags)
{
   UINT num_modes;
   UINT i, j;
   D3DDISPLAYMODE display_mode;
   int matches = 0;

   /* If any, go through all formats */
   if (!_al_pixel_format_is_real(format)) {
      j = 0;
   }
   /* Else find the matching format */
   else {
      for (j = 0; allegro_formats[j] != -1; j++) {
         if (allegro_formats[j] == format)
	    break;
      }
      if (allegro_formats[j] == -1)
         return 0;
   }

   for (; allegro_formats[j] != -1; j++) {
      if (!_al_pixel_format_is_real(allegro_formats[j]))
         continue;

      num_modes = IDirect3D9_GetAdapterModeCount(_al_d3d, D3DADAPTER_DEFAULT, d3d_formats[j]);
   
      for (i = 0; i < num_modes; i++) {
         if (IDirect3D9_EnumAdapterModes(_al_d3d, D3DADAPTER_DEFAULT, _al_format_to_d3d(format), i, &display_mode) != D3D_OK) {
            return matches;
         }
         if (refresh_rate && display_mode.RefreshRate != (unsigned)refresh_rate)
            continue;
         matches++;
      }
   
      if (_al_pixel_format_is_real(format))
         break;
   }

   return matches;
}

ALLEGRO_DISPLAY_MODE *_al_d3d_get_display_mode(int index, int format,
   int refresh_rate, int flags, ALLEGRO_DISPLAY_MODE *mode)
{
   UINT num_modes;
   UINT i, j;
   D3DDISPLAYMODE display_mode;
   int matches = 0;

   /* If any, go through all formats */
   if (!_al_pixel_format_is_real(format)) {
      j = 0;
   }
   /* Else find the matching format */
   else {
      for (j = 0; allegro_formats[j] != -1; j++) {
         if (allegro_formats[j] == format)
	    break;
      }
      if (allegro_formats[j] == -1)
         return NULL;
   }

   for (; allegro_formats[j] != -1; j++) {
      if (!_al_pixel_format_is_real(allegro_formats[j]))
         continue;

      num_modes = IDirect3D9_GetAdapterModeCount(_al_d3d, D3DADAPTER_DEFAULT, d3d_formats[j]);
   
      for (i = 0; i < num_modes; i++) {
         if (IDirect3D9_EnumAdapterModes(_al_d3d, D3DADAPTER_DEFAULT, _al_format_to_d3d(format), i, &display_mode) != D3D_OK) {
            return NULL;
         }
         if (refresh_rate && display_mode.RefreshRate != (unsigned)refresh_rate)
            continue;
         if (matches == index) {
            mode->width = display_mode.Width;
            mode->height = display_mode.Height;
            mode->format = allegro_formats[j];
            mode->refresh_rate = display_mode.RefreshRate;
   	    return mode;
         }
         matches++;
      }
   
      if (_al_pixel_format_is_real(format))
         break;
   }

   return mode;
}

