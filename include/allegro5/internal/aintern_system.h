#ifndef __al_included_allegro5_aintern_system_h
#define __al_included_allegro5_aintern_system_h

#include "allegro5/system.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_events.h"
#include "allegro5/internal/aintern_joystick.h"
#include "allegro5/internal/aintern_keyboard.h"
#include "allegro5/internal/aintern_mouse.h"
#include "allegro5/internal/aintern_vector.h"

typedef struct ALLEGRO_SYSTEM_INTERFACE ALLEGRO_SYSTEM_INTERFACE;

struct ALLEGRO_SYSTEM_INTERFACE
{
   int id;
   ALLEGRO_SYSTEM *(*initialize)(int flags);
   ALLEGRO_DISPLAY_INTERFACE *(*get_display_driver)(void);
   ALLEGRO_KEYBOARD_DRIVER *(*get_keyboard_driver)(void);
   ALLEGRO_MOUSE_DRIVER *(*get_mouse_driver)(void);
   ALLEGRO_JOYSTICK_DRIVER *(*get_joystick_driver)(void);
   int (*get_num_display_modes)(void);
   ALLEGRO_DISPLAY_MODE *(*get_display_mode)(int index, ALLEGRO_DISPLAY_MODE *mode);
   void (*shutdown_system)(void);
   int (*get_num_video_adapters)(void);
   void (*get_monitor_info)(int adapter, ALLEGRO_MONITOR_INFO *info);
   ALLEGRO_MOUSE_CURSOR *(*create_mouse_cursor)(ALLEGRO_BITMAP *bmp, int x_focus, int y_focus);
   void (*destroy_mouse_cursor)(ALLEGRO_MOUSE_CURSOR *cursor);
   bool (*get_cursor_position)(int *ret_x, int *ret_y);
   ALLEGRO_PATH *(*get_path)(int id);
   bool (*inhibit_screensaver)(bool inhibit);
   void (*thread_init)(ALLEGRO_THREAD *thread);
   void (*thread_exit)(ALLEGRO_THREAD *thread);
   void *(*open_library)(const char *filename);
   void *(*import_symbol)(void *library, const char *symbol);
   void (*close_library)(void *handle);
};

struct ALLEGRO_SYSTEM
{
   ALLEGRO_SYSTEM_INTERFACE *vt;
   _AL_VECTOR displays; /* Keep a list of all displays attached to us. */
   ALLEGRO_CONFIG *config;
};


AL_FUNC(void, _al_register_system_interfaces, (void));
AL_VAR(_AL_VECTOR, _al_system_interfaces);
AL_VAR(_AL_DTOR_LIST *, _al_dtor_list);

AL_FUNC(void *, _al_open_library, (const char *filename));
AL_FUNC(void *, _al_import_symbol, (void *library, const char *symbol));
AL_FUNC(void, _al_close_library, (void *library));

#endif
