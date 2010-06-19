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
 *      Thread local storage.
 *
 *      By Trent Gamblin.
 *
 */

/* Title: Thread local storage
 *
 * Some of Allegro's "global" state is thread-specific.  For example, each
 * thread in the program will have it's own target bitmap, and calling
 * <al_set_target_bitmap> will only change the target bitmap of the calling
 * thread.  This reduces the problems with global state in multithreaded
 * programs.
 */


#include <string.h>
#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_file.h"
#include "allegro5/internal/aintern_fshook.h"

/* Thread local storage for various per-thread state. */
typedef struct thread_local_state {
   /* Display parameters */
   int new_display_refresh_rate;
   int new_display_flags;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS new_display_settings;
   /* Current display */
   ALLEGRO_DISPLAY *current_display;
   /* This is used for storing current transformation and blenders for
    * graphics operations while no current display is available.
    */

   /* FIXME: If we have a way to call a callback before a thread is
    * destroyed this field can be removed and instead memory_display
    * should be allocated on demand with al_malloc and then destroyed
    * with al_free in the callback. For now putting it here avoids
    * the allocation.
    */
   ALLEGRO_DISPLAY memory_display;

   /* Target bitmap */
   ALLEGRO_BITMAP *target_bitmap;
   /* Bitmap parameters */
   int new_bitmap_format;
   int new_bitmap_flags;
   /* Files */
   const ALLEGRO_FILE_INTERFACE *new_file_interface;
   const ALLEGRO_FS_INTERFACE *fs_interface;
   /* Error code */
   int allegro_errno;
} thread_local_state;

typedef struct INTERNAL_STATE {
   thread_local_state tls;
   ALLEGRO_BLENDER stored_blender;
   ALLEGRO_TRANSFORM stored_transform;
   int flags;
} INTERNAL_STATE;

ALLEGRO_STATIC_ASSERT(
   sizeof(ALLEGRO_STATE) > sizeof(INTERNAL_STATE));


static void initialize_tls_values(thread_local_state *tls)
{
   memset(tls, 0, sizeof *tls);

   tls->new_bitmap_format = ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA;
   tls->new_file_interface = &_al_file_interface_stdio;
   tls->fs_interface = &_al_fs_interface_stdio;
   
   _al_fill_display_settings(&tls->new_display_settings);
}


#if (defined ALLEGRO_MINGW32 && ( \
   __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 2) || \
   (__GNUC__ == 4 && __GNUC_MINOR__ == 2 && __GNUC_PATCHLEVEL__ < 1))) || \
   defined ALLEGRO_CFG_DLL_TLS

/*
 * MinGW 3.x doesn't have builtin thread local storage, so we
 * must use the Windows API.
 */

#include <windows.h>


/* Forward declaration to bypass strict warning. */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);


static DWORD tls_index;


static thread_local_state *tls_get(void)
{
   thread_local_state *t = TlsGetValue(tls_index);
   return t;
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{ 
   thread_local_state *data;

   (void)hinstDLL;
   (void)fdwReason;
   (void)lpvReserved;
 
   switch (fdwReason) { 
      case DLL_PROCESS_ATTACH: 
         if ((tls_index = TlsAlloc()) == TLS_OUT_OF_INDEXES) {
            return false;
	 }
            // No break: Initialize the index for first thread.
            // The attached process creates a new thread. 

      case DLL_THREAD_ATTACH: 
          // Initialize the TLS index for this thread.
          data = al_malloc(sizeof(*data));
          if (data != NULL) {
             TlsSetValue(tls_index, data);
             initialize_tls_values(data);
          }
             break; 
 
        // The thread of the attached process terminates.
      case DLL_THREAD_DETACH: 
         // Release the allocated memory for this thread.
         data = TlsGetValue(tls_index); 
         if (data != NULL) 
            al_free(data);
 
         break; 
 
      // DLL unload due to process termination or FreeLibrary. 
      case DLL_PROCESS_DETACH: 
         // Release the allocated memory for this thread.
         data = TlsGetValue(tls_index); 
         if (data != NULL) 
            al_free(data);
         // Release the TLS index.
         TlsFree(tls_index); 
         break; 
 
      default: 
         break; 
   } 
 
   return true; 
}


#else /* not defined ALLEGRO_MINGW32 */

#if defined(ALLEGRO_MSVC) || defined(ALLEGRO_BCC32)

#define THREAD_LOCAL __declspec(thread)
#define HAVE_NATIVE_TLS

#elif defined ALLEGRO_MACOSX || defined ALLEGRO_GP2XWIZ || defined ALLEGRO_IPHONE

#define THREAD_LOCAL

static pthread_key_t tls_key = 0;

static void pthreads_thread_destroy(void* ptr)
{
   al_free(ptr);
}


void _al_pthreads_tls_init(void)
{
   pthread_key_create(&tls_key, pthreads_thread_destroy);
}

static thread_local_state _tls;

static thread_local_state* pthreads_thread_init(void)
{
   /* Allocate and copy the 'template' object */
   thread_local_state* ptr = (thread_local_state*)al_malloc(sizeof(thread_local_state));
   memcpy(ptr, &_tls, sizeof(thread_local_state));
   pthread_setspecific(tls_key, ptr);
   return ptr;
}

/* This function is short so it can hopefully be inlined. */
static thread_local_state* tls_get(void)
{
   thread_local_state* ptr = (thread_local_state*)pthread_getspecific(tls_key);
   if (ptr == NULL)
   {
      /* Must create object */
      ptr = pthreads_thread_init();
      initialize_tls_values(ptr);
   }
   return ptr;
}

#else /* not MSVC/BCC32, not OSX */

#define THREAD_LOCAL __thread
#define HAVE_NATIVE_TLS

#endif /* end not MSVC/BCC32, not OSX */

static THREAD_LOCAL thread_local_state _tls;

#ifdef HAVE_NATIVE_TLS
static thread_local_state *tls_get(void)
{
   static THREAD_LOCAL thread_local_state *ptr = NULL;
   if (!ptr) {
      ptr = &_tls;
      initialize_tls_values(ptr);
   }
   return ptr;
}
#endif /* end HAVE_NATIVE_TLS */


#endif /* end not defined ALLEGRO_MINGW32 */



void _al_set_new_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *settings)
{
   thread_local_state *tls;
   if ((tls = tls_get()) == NULL)
      return;
   memmove(&tls->new_display_settings, settings, sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));
}



ALLEGRO_EXTRA_DISPLAY_SETTINGS *_al_get_new_display_settings(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return &tls->new_display_settings;
}


/* Function: al_set_new_display_refresh_rate
 */
void al_set_new_display_refresh_rate(int refresh_rate)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_display_refresh_rate = refresh_rate;
}



/* Function: al_set_new_display_flags
 */
void al_set_new_display_flags(int flags)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_display_flags = flags;
}


/* Function: al_get_new_display_refresh_rate
 */
int al_get_new_display_refresh_rate(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_display_refresh_rate;
}



/* Function: al_get_new_display_flags
 */
int al_get_new_display_flags(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_display_flags;
}



/* Function: al_set_current_display
 */
bool al_set_current_display(ALLEGRO_DISPLAY *display)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return false;

   if (tls->current_display &&
         tls->current_display->vt->unset_current_display) {
      tls->current_display->vt->unset_current_display(tls->current_display);
      tls->current_display = NULL;
   }

   if (display) {
      if (!display->vt) {
         tls->current_display = display;
         return true;
      }
      else if (display->vt->set_current_display(display)) {
         tls->current_display = display;
         al_set_target_bitmap(al_get_backbuffer());
         return true;
      }
      else {
         return false;
      }
   }
   else {
      tls->current_display = NULL;
   }

   return true;
}



/* Function: al_get_current_display
 */
ALLEGRO_DISPLAY *al_get_current_display(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return NULL;
   return tls->current_display;
}



ALLEGRO_DISPLAY *_al_get_current_display(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return NULL;
   if (tls->current_display)
      return tls->current_display;

   if (!(tls->memory_display.flags & ALLEGRO_INTERNAL)) {
      tls->memory_display.flags |= ALLEGRO_INTERNAL;
      al_identity_transform(&tls->memory_display.cur_transform);
      _al_initialize_blender(&tls->memory_display.cur_blender);
   }
   return &tls->memory_display;
}



/* Function: al_set_target_bitmap
 */
void al_set_target_bitmap(ALLEGRO_BITMAP *bitmap)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   tls->target_bitmap = bitmap;
   if (bitmap != NULL) {
      if (!(bitmap->flags & ALLEGRO_MEMORY_BITMAP)) {
         ALLEGRO_DISPLAY *display = tls->current_display;
         ASSERT(display);
         display->vt->set_target_bitmap(display, bitmap);
      }
   }
}



/* Function: al_get_target_bitmap
 */
ALLEGRO_BITMAP *al_get_target_bitmap(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->target_bitmap;
}



/* Function: al_set_new_bitmap_format
 */
void al_set_new_bitmap_format(int format)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_bitmap_format = format;
}



/* Function: al_set_new_bitmap_flags
 */
void al_set_new_bitmap_flags(int flags)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_bitmap_flags = flags;
}



/* Function: al_get_new_bitmap_format
 */
int al_get_new_bitmap_format(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_bitmap_format;
}



/* Function: al_get_new_bitmap_flags
 */
int al_get_new_bitmap_flags(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->new_bitmap_flags;
}



#define _STORE(x) stored->tls.x = tls->x;
/* Function: al_store_state
 */
void al_store_state(ALLEGRO_STATE *state, int flags)
{
   thread_local_state *tls;
   INTERNAL_STATE *stored;

   if ((tls = tls_get()) == NULL)
      return;

   stored = (void *)state;
   stored->flags = flags;

   if (flags & ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS) {
      _STORE(new_display_refresh_rate);
      _STORE(new_display_flags);
   }
   
   if (flags & ALLEGRO_STATE_NEW_BITMAP_PARAMETERS) {
      _STORE(new_bitmap_format);
      _STORE(new_bitmap_flags);
   }
   
   if (flags & ALLEGRO_STATE_DISPLAY) {
      _STORE(current_display);
   }
   
   if (flags & ALLEGRO_STATE_TARGET_BITMAP) {
      _STORE(target_bitmap);
   }

   if (flags & ALLEGRO_STATE_BLENDER) {
      stored->stored_blender = _al_get_current_display()->cur_blender;
   }

   if (flags & ALLEGRO_STATE_NEW_FILE_INTERFACE) {
      _STORE(new_file_interface);
      _STORE(fs_interface);
   }
   
   if (flags & ALLEGRO_STATE_TRANSFORM) {
      stored->stored_transform = _al_get_current_display()->cur_transform;
   }
}
#undef _STORE



#define _STORE(x) tls->x = stored->tls.x;
/* Function: al_restore_state
 */
void al_restore_state(ALLEGRO_STATE const *state)
{
   thread_local_state *tls;
   INTERNAL_STATE *stored;
   int flags;

   if ((tls = tls_get()) == NULL)
      return;
   
   stored = (void *)state;
   flags = stored->flags;

   if (flags & ALLEGRO_STATE_NEW_DISPLAY_PARAMETERS) {
      _STORE(new_display_refresh_rate);
      _STORE(new_display_flags);
   }
   
   if (flags & ALLEGRO_STATE_NEW_BITMAP_PARAMETERS) {
      _STORE(new_bitmap_format);
      _STORE(new_bitmap_flags);
   }
   
   if (flags & ALLEGRO_STATE_DISPLAY) {
      _STORE(current_display);
      al_set_current_display(tls->current_display);
   }
   
   if (flags & ALLEGRO_STATE_TARGET_BITMAP) {
      _STORE(target_bitmap);
      al_set_target_bitmap(tls->target_bitmap);
   }
   
   if (flags & ALLEGRO_STATE_BLENDER) {
      _al_get_current_display()->cur_blender = stored->stored_blender;
   }

   if (flags & ALLEGRO_STATE_NEW_FILE_INTERFACE) {
      _STORE(new_file_interface);
      _STORE(fs_interface);
   }
   
   if (flags & ALLEGRO_STATE_TRANSFORM) {
      _al_get_current_display()->cur_transform = stored->stored_transform;
   }
}
#undef _STORE



/* Function: al_get_new_file_interface
 * FIXME: added a work-around for the situation where TLS has not yet been
 * initialised when this function is called. This may happen if Allegro
 * tries to load a configuration file before the system has been
 * initialised. Should probably rethink the logic here...
 */
const ALLEGRO_FILE_INTERFACE *al_get_new_file_interface(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return &_al_file_interface_stdio;

   /* FIXME: this situation should never arise because tls_ has the stdio
    * interface set as a default, but it arises on OS X if
    * pthread_getspecific() is called before pthreads_thread_init()...
    */
   if (tls->new_file_interface)
      return tls->new_file_interface;
   else
      return &_al_file_interface_stdio;
}



/* Function: al_set_new_file_interface
 */
void al_set_new_file_interface(const ALLEGRO_FILE_INTERFACE *file_interface)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_file_interface = file_interface;
}



/* Function: al_get_fs_interface
 */
const ALLEGRO_FS_INTERFACE *al_get_fs_interface(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return &_al_fs_interface_stdio;

   if (tls->fs_interface)
      return tls->fs_interface;
   else
      return &_al_fs_interface_stdio;
}



/* Function: al_set_fs_interface
 */
void al_set_fs_interface(const ALLEGRO_FS_INTERFACE *fs_interface)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->fs_interface = fs_interface;
}



/* Function: al_get_errno
 */
int al_get_errno(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return 0;
   return tls->allegro_errno;
}



/* Function: al_set_errno
 */
void al_set_errno(int errnum)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->allegro_errno = errnum;
}

/* vim: set sts=3 sw=3 et: */
