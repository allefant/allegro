#include <X11/Xlib.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "allegro/internal/aintern_system.h"
#include "allegro/internal/aintern_bitmap.h"
#include "allegro/platform/aintxdummy.h"

typedef struct AL_SYSTEM_XDUMMY AL_SYSTEM_XDUMMY;
typedef struct AL_DISPLAY_XDUMMY AL_DISPLAY_XDUMMY;
typedef struct AL_BITMAP_XDUMMY AL_BITMAP_XDUMMY;

/* This is our version of AL_SYSTEM with driver specific extra data. */
struct AL_SYSTEM_XDUMMY
{
   AL_SYSTEM system; /* This must be the first member, we "derive" from it. */

   /* Driver specifics. */

   Display *xdisplay; /* The X11 display. */
   _AL_THREAD thread; /* background thread. */
   _AL_MUTEX lock; /* thread lock for whenever we access internals. */
};

struct AL_BITMAP_XDUMMY
{
   AL_BITMAP bitmap; /* This must be the first member. */
   
   /* Driver specifics. */
   
   GLuint texture; /* 0 means, not uploaded yet. */
   float left, top, right, bottom; /* Texture coordinates. */
   int is_backbuffer; /* This is not a real bitmap, but the backbuffer. */
};

/* This is our version of AL_DISPLAY with driver specific extra data. */
struct AL_DISPLAY_XDUMMY
{
   AL_DISPLAY display; /* This must be the first member. */
   
   /* Driver specifics. */

   AL_BITMAP *backbuffer;

   Window window;
   GLXWindow glxwindow;
   GLXContext context;
   Atom wm_delete_window_atom;
};

/* Functions private to the X11 driver. */
void _al_display_xdummy_configure(AL_DISPLAY *d, XEvent *event);
void _al_xwin_keyboard_handler(XKeyEvent *event, bool dga2_hack);
void _al_display_xdummy_closebutton(AL_DISPLAY *d, XEvent *xevent);
AL_BITMAP *_al_xdummy_create_bitmap(AL_DISPLAY *d, int w, int h);
void _xdummy_add_drawing_functions(AL_DISPLAY_INTERFACE *vt);
