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

#include <string.h>
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_file.h"
#include "allegro5/internal/aintern_fshook.h"
#include "allegro5/internal/aintern_tls.h"


/* Thread local storage for various per-thread state. */
typedef struct thread_local_state {
   /* New display parameters */
   int new_display_flags;
   int new_display_refresh_rate;
   int new_display_adapter;
   int new_window_x;
   int new_window_y;
   ALLEGRO_EXTRA_DISPLAY_SETTINGS new_display_settings;

   /* Current display */
   ALLEGRO_DISPLAY *current_display;

   /* Target bitmap */
   ALLEGRO_BITMAP *target_bitmap;

   /* Blender */
   ALLEGRO_BLENDER current_blender;

   /* Bitmap parameters */
   int new_bitmap_format;
   int new_bitmap_flags;

   /* Files */
   const ALLEGRO_FILE_INTERFACE *new_file_interface;
   const ALLEGRO_FS_INTERFACE *fs_interface;

   /* Error code */
   int allegro_errno;

   /* Destructor ownership count */
   int dtor_owner_count;
} thread_local_state;

typedef struct INTERNAL_STATE {
   thread_local_state tls;
   ALLEGRO_BLENDER stored_blender;
   ALLEGRO_TRANSFORM stored_transform;
   int flags;
} INTERNAL_STATE;

ALLEGRO_STATIC_ASSERT(
   sizeof(ALLEGRO_STATE) > sizeof(INTERNAL_STATE));


static void initialize_blender(ALLEGRO_BLENDER *b)
{
   b->blend_op = ALLEGRO_ADD;
   b->blend_source = ALLEGRO_ONE,
   b->blend_dest = ALLEGRO_INVERSE_ALPHA;
   b->blend_alpha_op = ALLEGRO_ADD;
   b->blend_alpha_source = ALLEGRO_ONE;
   b->blend_alpha_dest = ALLEGRO_INVERSE_ALPHA;
}


static void initialize_tls_values(thread_local_state *tls)
{
   memset(tls, 0, sizeof *tls);

   tls->new_display_adapter = ALLEGRO_DEFAULT_DISPLAY_ADAPTER;
   tls->new_window_x = INT_MAX;
   tls->new_window_y = INT_MAX;

   initialize_blender(&tls->current_blender);
   tls->new_bitmap_flags = ALLEGRO_VIDEO_BITMAP;
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


#else /* not MinGW < 4.2.1 */

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


#endif /* end not MinGW < 4.2,1 */



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



/* Function: al_set_new_display_flags
 */
void al_set_new_display_flags(int flags)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_display_flags = flags;
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



/* Function: al_set_new_display_refresh_rate
 */
void al_set_new_display_refresh_rate(int refresh_rate)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_display_refresh_rate = refresh_rate;
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



/* Function: al_set_new_display_adapter
 */
void al_set_new_display_adapter(int adapter)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;

   if (adapter < 0) {
      tls->new_display_adapter = ALLEGRO_DEFAULT_DISPLAY_ADAPTER;
   }
   tls->new_display_adapter = adapter;
}



/* Function: al_get_new_display_adapter
 */
int al_get_new_display_adapter(void)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return ALLEGRO_DEFAULT_DISPLAY_ADAPTER;
   return tls->new_display_adapter;
}



/* Function: al_set_new_window_position
 */
void al_set_new_window_position(int x, int y)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_window_x = x;
   tls->new_window_y = y;
}



/* Function: al_get_new_window_position
 */
void al_get_new_window_position(int *x, int *y)
{
   thread_local_state *tls;
   int new_window_x = INT_MAX;
   int new_window_y = INT_MAX;

   if ((tls = tls_get()) != NULL) {
      new_window_x = tls->new_window_x;
      new_window_y = tls->new_window_y;
   }

   if (x)
      *x = new_window_x;
   if (y)
      *y = new_window_y;
}



/* Make the given display current, without changing the target bitmap.
 * This is used internally to change the current display transiently.
 */
bool _al_set_current_display_only(ALLEGRO_DISPLAY *display)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return false;

   if (tls->current_display &&
         tls->current_display->vt &&
         tls->current_display->vt->unset_current_display) {
      tls->current_display->vt->unset_current_display(tls->current_display);
      tls->current_display = NULL;
   }

   if (display &&
         display->vt &&
         display->vt->set_current_display) {
      if (!display->vt->set_current_display(display))
         return false;
   }

   tls->current_display = display;
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



/* Function: al_set_target_bitmap
 */
void al_set_target_bitmap(ALLEGRO_BITMAP *bitmap)
{
   thread_local_state *tls;
   ALLEGRO_DISPLAY *old_display;
   ALLEGRO_DISPLAY *new_display;

   if ((tls = tls_get()) == NULL)
      return;

   old_display = tls->current_display;

   if (bitmap == NULL) {
      /* Explicitly releasing the current rendering context. */
      new_display = NULL;
   }
   else if (bitmap->flags & ALLEGRO_MEMORY_BITMAP) {
      /* Setting a memory bitmap doesn't change the rendering context. */
      new_display = old_display;
   }
   else {
      new_display = bitmap->display;
   }

   /* Change the rendering context if necessary. */
   if (old_display != new_display) {
      if (old_display &&
            old_display->vt &&
            old_display->vt->unset_current_display) {
         old_display->vt->unset_current_display(old_display);
      }

      tls->current_display = new_display;

      if (new_display &&
            new_display->vt &&
            new_display->vt->set_current_display) {
         new_display->vt->set_current_display(new_display);
      }
   }

   /* Change the target bitmap itself. */
   tls->target_bitmap = bitmap;

   if (bitmap &&
         !(bitmap->flags & ALLEGRO_MEMORY_BITMAP) &&
         new_display &&
         new_display->vt &&
         new_display->vt->set_target_bitmap) {
      new_display->vt->set_target_bitmap(new_display, bitmap);

      new_display->vt->update_transformation(new_display, bitmap);
   }
}



/* Function: al_set_target_backbuffer
 */
void al_set_target_backbuffer(ALLEGRO_DISPLAY *display)
{
   al_set_target_bitmap(al_get_backbuffer(display));
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



/* Function: al_set_blender
 */
void al_set_blender(int op, int src, int dst)
{
   al_set_separate_blender(op, src, dst, op, src, dst);
}



/* Function: al_set_separate_blender
 */
void al_set_separate_blender(int op, int src, int dst,
   int alpha_op, int alpha_src, int alpha_dst)
{
   thread_local_state *tls;
   ALLEGRO_BLENDER *b;

   if ((tls = tls_get()) == NULL)
      return;

   b = &tls->current_blender;

   b->blend_op = op;
   b->blend_source = src;
   b->blend_dest = dst;
   b->blend_alpha_op = alpha_op;
   b->blend_alpha_source = alpha_src;
   b->blend_alpha_dest = alpha_dst;
}



/* Function: al_get_blender
 */
void al_get_blender(int *op, int *src, int *dst)
{
   al_get_separate_blender(op, src, dst, NULL, NULL, NULL);
}



/* Function: al_get_separate_blender
 */
void al_get_separate_blender(int *op, int *src, int *dst,
   int *alpha_op, int *alpha_src, int *alpha_dst)
{
   thread_local_state *tls;
   ALLEGRO_BLENDER *b;

   if ((tls = tls_get()) == NULL)
      return;

   b = &tls->current_blender;

   if (op)
      *op = b->blend_op;

   if (src)
      *src = b->blend_source;

   if (dst)
      *dst = b->blend_dest;

   if (alpha_op)
      *alpha_op = b->blend_alpha_op;

   if (alpha_src)
      *alpha_src = b->blend_alpha_source;

   if (alpha_dst)
      *alpha_dst = b->blend_alpha_dest;
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

   /* Assume ALLEGRO_VIDEO_BITMAP if ALLEGRO_MEMORY_BITMAP is not set. */
   if (!(flags & ALLEGRO_MEMORY_BITMAP))
      flags |= ALLEGRO_VIDEO_BITMAP;

   tls->new_bitmap_flags = flags;
}



/* Function: al_add_new_bitmap_flag
 */
void al_add_new_bitmap_flag(int flag)
{
   thread_local_state *tls;

   if ((tls = tls_get()) == NULL)
      return;
   tls->new_bitmap_flags |= flag;
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
      _STORE(new_display_flags);
      _STORE(new_display_refresh_rate);
      _STORE(new_display_adapter);
      _STORE(new_window_x);
      _STORE(new_window_y);
      _STORE(new_display_settings);
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
      stored->stored_blender = tls->current_blender;
   }

   if (flags & ALLEGRO_STATE_NEW_FILE_INTERFACE) {
      _STORE(new_file_interface);
      _STORE(fs_interface);
   }
   
   if (flags & ALLEGRO_STATE_TRANSFORM) {
      ALLEGRO_BITMAP *target = al_get_target_bitmap();
      if (!target)
         al_identity_transform(&stored->stored_transform);
      else
         stored->stored_transform = target->transform;
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
      _STORE(new_display_flags);
      _STORE(new_display_refresh_rate);
      _STORE(new_display_adapter);
      _STORE(new_window_x);
      _STORE(new_window_y);
      _STORE(new_display_settings);
   }
   
   if (flags & ALLEGRO_STATE_NEW_BITMAP_PARAMETERS) {
      _STORE(new_bitmap_format);
      _STORE(new_bitmap_flags);
   }
   
   if (flags & ALLEGRO_STATE_DISPLAY) {
      _STORE(current_display);
      _al_set_current_display_only(tls->current_display);
   }

   if (flags & ALLEGRO_STATE_TARGET_BITMAP) {
      _STORE(target_bitmap);
      al_set_target_bitmap(tls->target_bitmap);
   }
   
   if (flags & ALLEGRO_STATE_BLENDER) {
      tls->current_blender = stored->stored_blender;
   }

   if (flags & ALLEGRO_STATE_NEW_FILE_INTERFACE) {
      _STORE(new_file_interface);
      _STORE(fs_interface);
   }
   
   if (flags & ALLEGRO_STATE_TRANSFORM) {
      ALLEGRO_BITMAP *bitmap = al_get_target_bitmap();
      if (bitmap)
         al_use_transform(&stored->stored_transform);
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



int *_al_tls_get_dtor_owner_count(void)
{
   thread_local_state *tls;

   tls = tls_get();
   return &tls->dtor_owner_count;
}



/* vim: set sts=3 sw=3 et: */
