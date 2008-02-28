/* This is only a dummy driver, not implementing most required things,
 * it's just here to give me some understanding of the base framework of a
 * system driver.
 */

#ifdef DEBUG_X11
extern int _Xdebug; /* part of Xlib */
#endif

#include "xglx.h"

static ALLEGRO_SYSTEM_INTERFACE *vt;

static void background_thread(_AL_THREAD *thread, void *arg)
{
   ALLEGRO_SYSTEM_XGLX *s = arg;
   XEvent event;
   unsigned int i;

   while (1) {
      ALLEGRO_DISPLAY_XGLX *d = NULL;
      XNextEvent(s->xdisplay, &event);

      _al_mutex_lock(&s->lock);

      // FIXME: With many windows, it's bad to loop through them all,
      // maybe can come up with a better system here.
      // TODO: am I supposed to access ._size?
      for (i = 0; i < s->system.displays._size; i++) {
         ALLEGRO_DISPLAY_XGLX **dptr = _al_vector_ref(&s->system.displays, i);
         d = *dptr;
         if (d->window == event.xany.window) {
            break;
         }
      }

      switch (event.type) {
         case KeyPress:
            _al_xwin_keyboard_handler(&event.xkey, false);
            break;
         case KeyRelease:
            _al_xwin_keyboard_handler(&event.xkey, false);
            break;
         case ButtonPress:
            _al_xwin_mouse_button_press_handler(event.xbutton.button);
            break;
         case ButtonRelease:
            _al_xwin_mouse_button_release_handler(event.xbutton.button);
            break;
         case MotionNotify:
            _al_xwin_mouse_motion_notify_handler(
               event.xmotion.x, event.xmotion.y);
            break;
         case ConfigureNotify:
            _al_display_xglx_configure(&d->ogl_display.display,  &event);
            _al_cond_signal(&s->resized);
            break;
         case MapNotify:
            _al_cond_signal(&s->mapped);
            break;
         case ClientMessage:
            if ((Atom)event.xclient.data.l[0] == d->wm_delete_window_atom) {
               _al_display_xglx_closebutton(&d->ogl_display.display, &event);
               break;
            }
      }

      _al_mutex_unlock(&s->lock);
   }
}

/* Create a new system object for the dummy X11 driver. */
static ALLEGRO_SYSTEM *initialize(int flags)
{
   ALLEGRO_SYSTEM_XGLX *s = _AL_MALLOC(sizeof *s);
   memset(s, 0, sizeof *s);

   #ifdef DEBUG_X11
   _Xdebug = 1;
   #endif

   _al_mutex_init(&s->lock);
   _al_cond_init(&s->mapped);
   _al_cond_init(&s->resized);

   _al_vector_init(&s->system.displays, sizeof (ALLEGRO_SYSTEM_XGLX *));

   XInitThreads();

   s->system.vt = vt;

   /* Get an X11 display handle. */
   s->xdisplay = XOpenDisplay(0);

   TRACE("xsystem: XGLX driver connected to X11 (%s %d).\n",
      ServerVendor(s->xdisplay), VendorRelease(s->xdisplay));
   TRACE("xsystem: X11 protocol version %d.%d.\n",
      ProtocolVersion(s->xdisplay), ProtocolRevision(s->xdisplay));

   _al_thread_create(&s->thread, background_thread, s);

   TRACE("xsystem: events thread spawned.\n");

   _al_xglx_store_video_mode(s);

   return &s->system;
}

static void shutdown_system(void)
{
   TRACE("shutting down.\n");
   /* Close all open displays. */
   ALLEGRO_SYSTEM *s = al_system_driver();
   while (s->displays._size)
   {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      al_destroy_display(d);
   }
}

// FIXME: This is just for now, the real way is of course a list of
// available display drivers. Possibly such drivers can be attached at runtime
// to the system driver, so addons could provide additional drivers.
static ALLEGRO_DISPLAY_INTERFACE *get_display_driver(void)
{
    return _al_display_xglx_driver();
}

static ALLEGRO_KEYBOARD_DRIVER *get_keyboard_driver(void)
{
   // FIXME: Select the best driver somehow
   return _al_xwin_keyboard_driver_list[0].driver;
}

static ALLEGRO_MOUSE_DRIVER *get_mouse_driver(void)
{
   // FIXME: Select the best driver somehow
   return _al_xwin_mouse_driver_list[0].driver;
}

/* Internal function to get a reference to this driver. */
ALLEGRO_SYSTEM_INTERFACE *_al_system_xglx_driver(void)
{
   if (vt) return vt;

   vt = _AL_MALLOC(sizeof *vt);
   memset(vt, 0, sizeof *vt);

   vt->initialize = initialize;
   vt->get_display_driver = get_display_driver;
   vt->get_keyboard_driver = get_keyboard_driver;
   vt->get_mouse_driver = get_mouse_driver;
   vt->get_num_display_modes = _al_xglx_get_num_display_modes;
   vt->get_display_mode = _al_xglx_get_display_mode;
   vt->shutdown_system = shutdown_system;
   
   return vt;
}

/* This is a function each platform must define to register all available
 * system drivers.
 */
void _al_register_system_interfaces()
{
   ALLEGRO_SYSTEM_INTERFACE **add;

#if defined ALLEGRO_UNIX
   /* This is the only system driver right now */
   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_xglx_driver();
#endif
}
