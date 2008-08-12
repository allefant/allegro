#ifndef _WIN_NEW_H
#define _WIN_NEW_H

#include "allegro5/internal/aintern_system.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ALLEGRO_SYSTEM_WIN ALLEGRO_SYSTEM_WIN;

/* This is our version of ALLEGRO_SYSTEM with driver specific extra data. */
struct ALLEGRO_SYSTEM_WIN
{
	ALLEGRO_SYSTEM system; /* This must be the first member, we "derive" from it. */
};

AL_VAR(ALLEGRO_SYSTEM_WIN *, _al_win_system);

void _al_win_delete_thread_handle(DWORD handle);

HWND _al_win_create_window(ALLEGRO_DISPLAY *display, int width, int height, int flags);
HWND _al_win_create_faux_fullscreen_window(LPCTSTR devname, ALLEGRO_DISPLAY *display,
	int x1, int y1, int width, int height, int refresh_rate, int flags);
int _al_win_init_window();
void _al_win_ungrab_input(void);
HWND _al_win_create_hidden_window();

AL_VAR(HWND, _al_win_wnd);
AL_VAR(HWND, _al_win_compat_wnd);
AL_VAR(UINT, _al_win_msg_call_proc);
AL_VAR(UINT, _al_win_msg_suicide);

void _al_win_set_display_icon(ALLEGRO_DISPLAY *display ,ALLEGRO_BITMAP *bitmap);
void _al_win_get_window_pos(HWND window, RECT *pos);





void _al_win_set_window_position(HWND window, int x, int y);
void _al_win_get_window_position(HWND window, int *x, int *y);
void _al_win_remove_window_frame(HWND window, int w, int h);





#if defined ALLEGRO_CFG_D3D

AL_FUNC(int, _al_d3d_get_num_display_modes,
   (int format, int refresh_rate, int flags));
AL_FUNC(ALLEGRO_DISPLAY_MODE *, _al_d3d_get_display_mode,
   (int index, int format, int refresh_rate, int flags, ALLEGRO_DISPLAY_MODE *mode));

AL_FUNC(ALLEGRO_DISPLAY_INTERFACE *, _al_display_d3d_driver, (void));
AL_FUNC(bool, _al_d3d_init_display, ());
AL_FUNC(int, _al_d3d_get_num_video_adapters, (void));
AL_FUNC(void, _al_d3d_get_monitor_info, (int adapter, ALLEGRO_MONITOR_INFO *info));

#endif /*  defined ALLEGRO_CFG_D3D */

#if defined ALLEGRO_CFG_OPENGL

AL_FUNC(int, _al_wgl_get_num_display_modes,
   (int format, int refresh_rate, int flags));
AL_FUNC(ALLEGRO_DISPLAY_MODE *, _al_wgl_get_display_mode,
   (int index, int format, int refresh_rate, int flags, ALLEGRO_DISPLAY_MODE *mode));

ALLEGRO_DISPLAY_INTERFACE *_al_display_wgl_driver(void);
AL_FUNC(bool, _al_wgl_init_display, ());

#endif /*  defined ALLEGRO_CFG_OPENGL */

#ifdef __cplusplus
}
#endif


#endif
