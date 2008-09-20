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
 *      Some definitions for internal use by the Windows library code.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTWIN_H
#define AINTWIN_H

#ifndef ALLEGRO_H
   #error must include allegro.h first
#endif

#ifndef ALLEGRO_WINDOWS
   #error bad include
#endif


#include "allegro5/winalleg.h"
#include "allegro5/internal/aintern_display.h"

#ifndef SCAN_DEPEND
   /* workaround for buggy MinGW32 headers */
   #ifdef ALLEGRO_MINGW32
      #ifndef HMONITOR_DECLARED
         #define HMONITOR_DECLARED
      #endif
      #if (defined _HRESULT_DEFINED) && (defined WINNT)
         #undef WINNT
      #endif
   #endif

   #include <objbase.h>  /* for LPGUID */
#endif


#ifdef __cplusplus
   extern "C" {
#endif

/* generals */
AL_VAR(HINSTANCE, allegro_inst);
AL_VAR(HANDLE, allegro_thread);
AL_VAR(CRITICAL_SECTION, allegro_critical_section);
AL_VAR(int, _dx_ver);

#define _enter_critical()  EnterCriticalSection(&allegro_critical_section)
#define _exit_critical()   LeaveCriticalSection(&allegro_critical_section)

AL_FUNC(int, init_directx_window, (void));
AL_FUNC(void, exit_directx_window, (void));
AL_FUNC(int, get_dx_ver, (void));
AL_FUNC(int, adjust_window, (int w, int h));
AL_FUNC(void, restore_window_style, (void));
AL_FUNC(void, save_window_pos, (void));


/* main window */
#define WND_TITLE_SIZE  128

AL_ARRAY(char, wnd_title);
AL_VAR(int, wnd_x);
AL_VAR(int, wnd_y);
AL_VAR(int, wnd_width);
AL_VAR(int, wnd_height);
AL_VAR(int, wnd_sysmenu);

AL_FUNCPTR(void, user_close_proc, (void));


/* switch routines */
AL_VAR(int, _win_app_foreground);

AL_FUNC(void, sys_directx_display_switch_init, (void));
AL_FUNC(void, sys_directx_display_switch_exit, (void));
AL_FUNC(int, sys_directx_set_display_switch_mode, (int mode));

AL_FUNC(void, _win_switch_in, (void));
AL_FUNC(void, _win_switch_out, (void));
AL_FUNC(void, _win_reset_switch_mode, (void));

AL_FUNC(int, _win_thread_switch_out, (void));


/* main window routines */
AL_FUNC(int, wnd_call_proc, (int (*proc)(void)));
AL_FUNC(void, wnd_schedule_proc, (int (*proc)(void)));


/* input routines */
AL_FUNC(void, _win_input_exit, (void));
bool _win_input_register_event(HANDLE event_id, void (*event_handler)(void*), void*);
bool _win_input_unregister_event(HANDLE event_id);


/* keyboard routines */
VOID CALLBACK _al_win_key_dinput_acquire(ULONG_PTR param);
VOID CALLBACK _al_win_key_dinput_unacquire(ULONG_PTR param);
bool _al_win_attach_key_input(_AL_KEY_DINPUT *key_input);
bool _al_win_dettach_key_input(_AL_KEY_DINPUT *key_input);


/* mouse routines */
AL_VAR(HCURSOR, _win_hcursor);
AL_FUNC(int, mouse_dinput_acquire, (void));
AL_FUNC(int, mouse_dinput_unacquire, (void));
AL_FUNC(int, mouse_dinput_grab, (void));
AL_FUNC(int, mouse_set_syscursor, (void));
AL_FUNC(int, mouse_set_sysmenu, (int state));


/* joystick routines */
AL_FUNC(int, _al_win_joystick_dinput_acquire, (void));
AL_FUNC(int, _al_win_joystick_dinput_unacquire, (void));


/* thread routines */
AL_FUNC(void, _win_thread_init, (void));
AL_FUNC(void, _win_thread_exit, (void));



/* file routines */
AL_VAR(int, _al_win_unicode_filenames);


/* error handling */
AL_FUNC(char* , win_err_str, (long err));
AL_FUNC(void, thread_safe_trace, (char *msg, ...));

#if DEBUGMODE >= 2
   #define _TRACE                 thread_safe_trace
#else
   #define _TRACE                 1 ? (void) 0 : thread_safe_trace
#endif


#ifdef __cplusplus
   }
#endif



/*----------------------------------------------------------------------*
 *									*
 *	New stuff							*
 *									*
 *----------------------------------------------------------------------*/

/* TODO: integrate this above */
/* TODO: a lot of this is repeated from aintunix.h */


#include "allegro5/platform/aintwthr.h"


AL_BEGIN_EXTERN_C

/* time */
AL_FUNC(void, _al_win_init_time, (void));
AL_FUNC(void, _al_win_shutdown_time, (void));

AL_END_EXTERN_C


#endif          /* !defined AINTWIN_H */

