#include "allegro5/allegro.h"
#include <allegro5/internal/aintern_system.h>
#include <allegro5/internal/aintern_display.h>

typedef struct ALLEGRO_SYSTEM_IPHONE {
    ALLEGRO_SYSTEM system;
    
    ALLEGRO_MUTEX *mutex;
    ALLEGRO_COND *cond;
   
   bool has_shutdown, wants_shutdown;
   int visuals_count;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS **visuals;    
} ALLEGRO_SYSTEM_IPHONE;

typedef struct ALLEGRO_DISPLAY_IPHONE {
    ALLEGRO_DISPLAY display;
} ALLEGRO_DISPLAY_IPHONE;


void _al_iphone_init_path(void);
void _al_iphone_add_view(ALLEGRO_DISPLAY *d);
void _al_iphone_make_view_current(void);
void _al_iphone_flip_view(void);
void _al_iphone_reset_framebuffer(void);
void _al_iphone_recreate_framebuffer(ALLEGRO_DISPLAY *);
ALLEGRO_SYSTEM_INTERFACE *_al_get_iphone_system_interface(void);
ALLEGRO_DISPLAY_INTERFACE *_al_get_iphone_display_interface(void);
ALLEGRO_KEYBOARD_DRIVER *_al_get_iphone_keyboard_driver(void);
ALLEGRO_MOUSE_DRIVER *_al_get_iphone_mouse_driver(void);
ALLEGRO_TOUCH_INPUT_DRIVER *_al_get_iphone_touch_input_driver(void);
ALLEGRO_JOYSTICK_DRIVER *_al_get_iphone_joystick_driver(void);
void _al_iphone_setup_opengl_view(ALLEGRO_DISPLAY *d);
//void _al_iphone_generate_mouse_event(unsigned int type,
//   int x, int y, unsigned int button, ALLEGRO_DISPLAY *d);
//void _al_iphone_generate_touch_event(unsigned int type, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *d);
void _al_iphone_update_visuals(void);
void _al_iphone_accelerometer_control(int frequency);
void _al_iphone_generate_joystick_event(float x, float y, float z);
void _al_iphone_await_termination(void);
void _al_iphone_get_screen_size(int *w, int *h);
int _al_iphone_get_orientation();
void _al_iphone_translate_from_screen(ALLEGRO_DISPLAY *d, int *x, int *y);
void _al_iphone_translate_to_screen(ALLEGRO_DISPLAY *d, int *x, int *y);
void _al_iphone_clip(ALLEGRO_BITMAP const *bitmap, int x_1, int y_1, int x_2, int y_2);

void _al_iphone_touch_input_handle_begin(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp);
void _al_iphone_touch_input_handle_end(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp);
void _al_iphone_touch_input_handle_move(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp);
void _al_iphone_touch_input_handle_cancel(int id, double timestamp, float x, float y, bool primary, ALLEGRO_DISPLAY *disp);
