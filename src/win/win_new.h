#ifndef _WIN_NEW_H
#define _WIN_NEW_H

#include "allegro/internal/aintern_system.h"

typedef struct AL_SYSTEM_WIN AL_SYSTEM_WIN;

/* This is our version of AL_SYSTEM with driver specific extra data. */
struct AL_SYSTEM_WIN
{
	AL_SYSTEM system; /* This must be the first member, we "derive" from it. */
};

AL_SYSTEM_WIN *_al_win_system;

AL_DISPLAY_INTERFACE *_al_display_d3d_driver(void);
AL_FUNC(bool, _al_d3d_init_display, ());

void _al_win_delete_thread_handle(DWORD handle);

HWND _al_win_create_window(AL_DISPLAY *display, int width, int height, int flags);
int _al_win_init_window();
void _al_win_ungrab_input();
HWND _al_win_create_hidden_window();

AL_VAR(HWND, _al_win_wnd);
AL_VAR(HWND, _al_win_compat_wnd);
AL_VAR(UINT, _al_win_msg_call_proc);
AL_VAR(UINT, _al_win_msg_suicide);

#endif
