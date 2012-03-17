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
 *      Android Java/JNI system driver
 *
 *      By Thomas Fjellstrom.
 *
 */

#include "allegro5/allegro.h"
#include "allegro5/platform/aintunix.h"
#include "allegro5/internal/aintern_android.h"
#include "allegro5/internal/aintern_tls.h"
#include "allegro5/platform/aintandroid.h"
#include "allegro5/platform/alandroid.h"
#include "allegro5/threads.h"

#include <dlfcn.h>
#include <jni.h>

#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_opengl.h"

ALLEGRO_DEBUG_CHANNEL("android")

struct system_data_t {
   JNIEnv *env;
   jobject activity_object;
   jclass input_stream_class;
   jclass illegal_argument_exception_class;
   
   ALLEGRO_SYSTEM_ANDROID *system;
   ALLEGRO_MUTEX *mutex;
   ALLEGRO_COND *cond;
   ALLEGRO_THREAD *trampoline;
   bool trampoline_running;
   JNIEnv *main_env;
   
   ALLEGRO_USTR *lib_dir;
   ALLEGRO_USTR *app_name;
   ALLEGRO_USTR *resources_dir;
   ALLEGRO_USTR *data_dir;
	ALLEGRO_USTR *apk_path;
	
   void *user_lib;
   int (*user_main)();
   
   int orientation;
};

static struct system_data_t system_data = { NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, false, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0 };
static JavaVM* javavm;

/* define here, so we have access to system_data */
JNIEnv *_jni_getEnv()
{
   return system_data.main_env;
}

int _al_android_get_display_orientation(void)
{
   int o = system_data.orientation;
   return o;
}

void __jni_checkException(JNIEnv *env, const char *file, const char *func, int line)
{
   jthrowable exc;
   
   exc = (*env)->ExceptionOccurred(env);
   if (exc) {
      ALLEGRO_DEBUG("GOT AN EXCEPTION @ %s:%i %s", file, line, func);
      /* We don't do much with the exception, except that
         we print a debug message for it, clear it, and 
         throw a new exception. */
      (*env)->ExceptionDescribe(env);
      (*env)->ExceptionClear(env);
      (*env)->FatalError(env, "EXCEPTION");
      //(*env)->ThrowNew(env, system_data.illegal_argument_exception_class, "thrown from C code");
   }
}

jobject _al_android_activity_object()
{
   return system_data.activity_object;
}

int _al_android_get_orientation()
{
   return system_data.orientation;
}

static void finish_activity(JNIEnv *env);

void *android_app_trampoline(ALLEGRO_THREAD *thr, void *arg)
{
   const char *argv[2] = { al_cstr(system_data.app_name), NULL };
   (void)thr; (void)arg;
   
   // setup main_env
   JavaVMAttachArgs attach_args = { JNI_VERSION_1_4, "main_trampoline", NULL };
   jint attach_ret = (*javavm)->AttachCurrentThread(javavm, &system_data.main_env, &attach_args);
   if(attach_ret != 0) {
      ALLEGRO_ERROR("failed to attach trampoline to jvm >:(");
      return NULL;
   }
   
   // chdir(al_cstr(system_data.data_dir));
   
   ALLEGRO_DEBUG("signaling running");
   
   al_lock_mutex(system_data.mutex);
   system_data.trampoline_running = true;
   al_broadcast_cond(system_data.cond);
   al_unlock_mutex(system_data.mutex);
   
   ALLEGRO_DEBUG("entering app's main function");
   
   int ret = (system_data.user_main)(1, (char **)argv);
   if(ret != 0) {
      ALLEGRO_DEBUG("app's main returned failure?");
   }
   
   /* I don't think android calls our atexit() stuff since we're in a shared lib
      so make sure al_uninstall_system is called */
   ALLEGRO_DEBUG("call exit funcs");
   al_uninstall_system();
   
   ALLEGRO_DEBUG("calling finish_activity");
   finish_activity(system_data.main_env);
   ALLEGRO_DEBUG("returning/exit-thread");
   
   jint detach_ret = (*javavm)->DetachCurrentThread(javavm);
   if(detach_ret != 0 ) {
      ALLEGRO_ERROR("failed to detach current thread");
   }
   
   return NULL;
}

/* called by JNI/Java */
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
   (void)reserved;
   javavm = vm;
   return JNI_VERSION_1_4;
}

JNIEXPORT bool Java_org_liballeg_app_AllegroActivity_nativeOnCreate(JNIEnv *env, jobject obj)
{
   ALLEGRO_PATH *lib_path = NULL;
   ALLEGRO_USTR *lib_fname = NULL;
   const char *full_path = NULL;
   ALLEGRO_SYSTEM_ANDROID *na_sys = NULL;
   jclass iae;
   
   // we're already initialized, we REALLY don't want to run all the stuff below again.
   if(system_data.system) {
      return true;
   }
   
   //_al_pthreads_tls_init();
   
   pthread_t self = pthread_self();
   ALLEGRO_DEBUG("pthread_self:%p", (void*)self);
   ALLEGRO_DEBUG("nativeOnCreate begin");
   
   memset(&system_data, 0, sizeof(system_data));
   
   ALLEGRO_DEBUG("grab activity global refs");
   system_data.env             = env;
   system_data.activity_object = (*env)->NewGlobalRef(env, obj);
   
   iae = (*env)->FindClass(env, "java/lang/IllegalArgumentException");
   system_data.illegal_argument_exception_class = (*env)->NewGlobalRef(env, iae);
   
   ALLEGRO_DEBUG("create mutex and cond objects");
   system_data.mutex = al_create_mutex();
   system_data.cond  = al_create_cond();

   ALLEGRO_DEBUG("get lib_dir, app_name, and data_dir");
   system_data.lib_dir  = _jni_callStringMethod(env, system_data.activity_object, "getLibraryDir", "()Ljava/lang/String;");
   system_data.app_name = _jni_callStringMethod(env, system_data.activity_object, "getAppName", "()Ljava/lang/String;");
   system_data.resources_dir = _jni_callStringMethod(env, system_data.activity_object, "getResourcesDir", "()Ljava/lang/String;");
	system_data.data_dir = _jni_callStringMethod(env, system_data.activity_object, "getPubDataDir", "()Ljava/lang/String;");
   system_data.apk_path = _jni_callStringMethod(env, system_data.activity_object, "getApkPath", "()Ljava/lang/String;");
	ALLEGRO_DEBUG("got lib_dir: %s, app_name: %s, resources_dir: %s, data_dir: %s, apk_path: %s", al_cstr(system_data.lib_dir), al_cstr(system_data.app_name), al_cstr(system_data.resources_dir), al_cstr(system_data.data_dir), al_cstr(system_data.apk_path));
   
   ALLEGRO_DEBUG("creating ALLEGRO_SYSTEM_ANDROID struct");
   na_sys = system_data.system = (ALLEGRO_SYSTEM_ANDROID*)al_malloc(sizeof *na_sys);
   memset(na_sys, 0, sizeof *na_sys);
   
   ALLEGRO_DEBUG("get system pointer");
   ALLEGRO_SYSTEM *sys = &na_sys->system;
   ALLEGRO_DEBUG("get system interface");
   sys->vt = _al_system_android_interface();
   
   ALLEGRO_DEBUG("init display vector");
   _al_vector_init(&sys->displays, sizeof(ALLEGRO_DISPLAY_ANDROID *));
   
   ALLEGRO_DEBUG("init time");
   _al_unix_init_time();
   
   ALLEGRO_DEBUG("strdup app_name");
   lib_fname = al_ustr_dup(system_data.app_name);
   al_ustr_insert_cstr(lib_fname, 0, "lib");
   al_ustr_append_cstr(lib_fname, ".so");
   
   lib_path = al_create_path_for_directory(al_cstr(system_data.lib_dir));
   al_set_path_filename(lib_path, al_cstr(lib_fname));
   
   full_path = al_path_cstr(lib_path, ALLEGRO_NATIVE_PATH_SEP);

   ALLEGRO_DEBUG("load user lib: %s", full_path);
   system_data.user_lib = dlopen(full_path, RTLD_LAZY|RTLD_GLOBAL);
   if(!system_data.user_lib) {
      ALLEGRO_ERROR("failed to load user app: '%s': %s", full_path, dlerror());
      return false;
   }
   
   ALLEGRO_DEBUG("grab user main");
   system_data.user_main = dlsym(system_data.user_lib, "main");
   if(!system_data.user_main) {
      ALLEGRO_ERROR("failed to locate main entry point in user app '%s': %s", full_path, dlerror());
      dlclose(system_data.user_lib);
      return false;
   }
   
   ALLEGRO_DEBUG("creating trampoline for app thread");
   system_data.trampoline = al_create_thread(android_app_trampoline, NULL);
   al_start_thread(system_data.trampoline);
   
   ALLEGRO_DEBUG("waiting for app trampoline to signal running");
   al_lock_mutex(system_data.mutex);
   while(!system_data.trampoline_running) {
      al_wait_cond(system_data.cond, system_data.mutex);
   }
   al_unlock_mutex(system_data.mutex);
   
   ALLEGRO_DEBUG("setup done. returning to dalkvik.");
   
   return true;
}


JNIEXPORT void JNICALL Java_org_liballeg_app_AllegroActivity_nativeOnPause(JNIEnv *env, jobject obj)
{
   ALLEGRO_SYSTEM *sys = &system_data.system->system;
   ALLEGRO_DISPLAY *d = NULL;
   ALLEGRO_EVENT event;

   (void)env; (void)obj;
   
   ALLEGRO_DEBUG("pause activity\n");
   
   /* no display, just skip */
   if(!_al_vector_size(&sys->displays)) {
      ALLEGRO_DEBUG("no display, not sending SWITCH_OUT event");
      return;
   }
   
   d = *(ALLEGRO_DISPLAY**)_al_vector_ref(&sys->displays, 0);
   ASSERT(d != NULL);
   
   ALLEGRO_DEBUG("locking display event source: %p %p", d, &d->es);
   
   _al_event_source_lock(&d->es);
   
   if(_al_event_source_needs_to_generate_event(&d->es)) {
      ALLEGRO_DEBUG("emit event");
      event.display.type = ALLEGRO_EVENT_DISPLAY_SWITCH_OUT;
      event.display.timestamp = al_current_time();
      _al_event_source_emit_event(&d->es, &event);
   }
   
   ALLEGRO_DEBUG("unlocking display event source");
   _al_event_source_unlock(&d->es);
}

JNIEXPORT void JNICALL Java_org_liballeg_app_AllegroActivity_nativeOnResume(JNIEnv *env, jobject obj)
{
   ALLEGRO_SYSTEM *sys = &system_data.system->system;
   ALLEGRO_DISPLAY *d = NULL;
   
   (void)obj;
   
   ALLEGRO_DEBUG("resume activity");
   
   if(!system_data.system || !sys) {
      ALLEGRO_DEBUG("no system driver");
      return;
   }

   if(!_al_vector_size(&sys->displays)) {
      ALLEGRO_DEBUG("no display, not sending SWITCH_IN event");
      return;
   }
   
   d = *(ALLEGRO_DISPLAY**)_al_vector_ref(&sys->displays, 0);
   ALLEGRO_DEBUG("got display: %p", d);
   
   //if(!((ALLEGRO_DISPLAY_ANDROID*)d)->surface_object) {
   //   ALLEGRO_DEBUG("display is not fully initialized yet, skip re-init and display switch in");
   //   return;
   //}
   
   if(!((ALLEGRO_DISPLAY_ANDROID*)d)->created) {
      _al_android_create_surface(env, true); // request android create our surface
   }
   
   // ??
}

JNIEXPORT void JNICALL Java_org_liballeg_app_AllegroActivity_nativeOnDestroy(JNIEnv *env, jobject obj)
{
   (void)obj;
   
   ALLEGRO_DEBUG("destroy activity");
   if(!system_data.user_lib) {
      ALLEGRO_DEBUG("user lib not loaded.");
      return;
   }
   
   system_data.user_main = NULL;
   if(dlclose(system_data.user_lib) != 0) {
      ALLEGRO_ERROR("failed to unload user lib: %s", dlerror());
      return;
   }
   
   (*env)->DeleteGlobalRef(env, system_data.activity_object);
   (*env)->DeleteGlobalRef(env, system_data.illegal_argument_exception_class);
   (*env)->DeleteGlobalRef(env, system_data.input_stream_class);
   
   free(system_data.system);
   
   memset(&system_data, 0, sizeof(system_data));
}

JNIEXPORT void JNICALL Java_org_liballeg_app_AllegroActivity_nativeOnAccel(JNIEnv *env, jobject obj, jint id, jfloat x, jfloat y, jfloat z)
{
   (void)env; (void)obj; (void)id;
   //ALLEGRO_DEBUG("got some accelerometer data!");
   _al_android_generate_joystick_event(x, y, z);
}

JNIEXPORT void JNICALL Java_org_liballeg_app_AllegroActivity_nativeOnOrientationChange(JNIEnv *env, jobject obj, int orientation, bool init)
{
   ALLEGRO_SYSTEM *sys = &system_data.system->system;
   ALLEGRO_DISPLAY *d = NULL;
   ALLEGRO_EVENT event;

   (void)env; (void)obj;
   
   ALLEGRO_DEBUG("got orientation change!");
   
   system_data.orientation = orientation;
      
   if(!init) {
         
      /* no display, just skip */
      if(!_al_vector_size(&sys->displays)) {
         ALLEGRO_DEBUG("no display, not sending orientation change event");
         return;
      }
      
      d = *(ALLEGRO_DISPLAY**)_al_vector_ref(&sys->displays, 0);
      ASSERT(d != NULL);
      
      ALLEGRO_DEBUG("locking display event source: %p %p", d, &d->es);
      
      _al_event_source_lock(&d->es);
      
      if(_al_event_source_needs_to_generate_event(&d->es)) {
         ALLEGRO_DEBUG("emit event");
         event.display.type = ALLEGRO_EVENT_DISPLAY_ORIENTATION;
         event.display.timestamp = al_current_time();
         event.display.orientation = orientation;
         _al_event_source_emit_event(&d->es, &event);
      }
      
      ALLEGRO_DEBUG("unlocking display event source");
      _al_event_source_unlock(&d->es);
   
   }
}

JNIEXPORT void JNICALL Java_org_liballeg_app_AllegroActivity_nativeCreateDisplay(JNIEnv *env, jobject obj)
{
   (void)obj;
   ALLEGRO_DEBUG("nativeCreateDisplay begin");
   
   _al_android_create_surface(env, false);
   
   ALLEGRO_DEBUG("nativeCreateDisplay end");
}

static void finish_activity(JNIEnv *env)
{
   ALLEGRO_DEBUG("pre post");
   _jni_callVoidMethod(env, system_data.activity_object, "postFinish");
   ALLEGRO_DEBUG("post post");
}

static ALLEGRO_SYSTEM *android_initialize(int flags)
{
   (void)flags;
   
   ALLEGRO_DEBUG("init dummy");
   return &system_data.system->system;
}

static int android_get_num_video_adapters(void)
{
   return 1;
}

static void android_shutdown_system()
{
   ALLEGRO_SYSTEM *s = al_get_system_driver();
  /* Close all open displays. */
   while (_al_vector_size(&s->displays) > 0) {
      ALLEGRO_DISPLAY **dptr = _al_vector_ref(&s->displays, 0);
      ALLEGRO_DISPLAY *d = *dptr;
      _al_destroy_display_bitmaps(d);
      al_destroy_display(d);
   }
   _al_vector_free(&s->displays);  
}

static ALLEGRO_SYSTEM_INTERFACE *android_vt;

ALLEGRO_SYSTEM_INTERFACE *_al_system_android_interface()
{
   if(android_vt)
      return android_vt;
   
   android_vt = al_malloc(sizeof *android_vt);
   memset(android_vt, 0, sizeof *android_vt);
   
   android_vt->initialize = android_initialize;
   android_vt->get_display_driver = _al_get_android_display_driver;
   android_vt->get_keyboard_driver = _al_get_android_keyboard_driver;
   //android_vt->get_mouse_driver = _al_get_android_na_mouse_driver;
   android_vt->get_touch_input_driver = _al_get_android_touch_input_driver;
   android_vt->get_joystick_driver = _al_get_android_joystick_driver;
   android_vt->get_num_video_adapters = android_get_num_video_adapters;
   //android_vt->get_monitor_info = _al_get_android_na_montior_info;
   android_vt->get_path = _al_android_get_path;
   android_vt->shutdown_system = android_shutdown_system;
   
   return android_vt;
}

ALLEGRO_PATH *_al_android_get_path(int id)
{
   ALLEGRO_PATH *path = NULL;
   
   switch(id) {
      case ALLEGRO_RESOURCES_PATH:
         /* path to bundle's files */
         path = al_create_path_for_directory(al_cstr(system_data.resources_dir));
         break;
         
      case ALLEGRO_TEMP_PATH:
      case ALLEGRO_USER_DATA_PATH:
      case ALLEGRO_USER_HOME_PATH:
      case ALLEGRO_USER_SETTINGS_PATH:
      case ALLEGRO_USER_DOCUMENTS_PATH:
         /* path to sdcard */
         path = al_create_path_for_directory(al_cstr(system_data.data_dir));
         break;
         
      case ALLEGRO_EXENAME_PATH:
         /* bundle path + bundle name */
         // FIXME! 
         path = al_create_path(al_cstr(system_data.apk_path));
         break;
			
		default:
			path = al_create_path_for_directory("/DANGER/WILL/ROBINSON");
			break;
   }
   
   return path;
}

/*
 * load image using in memory buffer and a couple extra copies
ALLEGRO_BITMAP *_al_android_load_image_f(ALLEGRO_FILE *fh, int flags)
{
   int buffer_len = al_fsize(fh);
   ALLEGRO_BITMAP *bitmap = NULL;
   int bitmap_w = 0, bitmap_h = 0, pitch = 0;
   jbyteArray byteArray;
   jbyte *buffer = NULL;
   jobject jbitmap;
   jobject byte_buffer;
   ALLEGRO_STATE state;
   
   ALLEGRO_DEBUG("load image begin");
   
   // buffer +1 = 1, copies 1
   byteArray = _jni_call(system_data.main_env, jbyteArray, NewByteArray, buffer_len);
   buffer = _jni_call(system_data.main_env, jbyte*, GetByteArrayElements, byteArray, NULL);
   
   int read_len = al_fread(fh, buffer, buffer_len);
   if(read_len != buffer_len) {
      ALLEGRO_ERROR("wanted %i bytes, only got %i", buffer_len, read_len);
      return NULL;
   }
   
   _jni_callv(system_data.main_env, ReleaseByteArrayElements, byteArray, buffer, 0);
   
   // buffer +1 = 2, copies 2
   jbitmap = _jni_callObjectMethodV(system_data.main_env, system_data.activity_object, "decodeBitmapByteArray", "([B)Landroid/graphics/Bitmap;", byteArray);
   if(!jbitmap) {
      ALLEGRO_ERROR("failed to decode bitmap :(");
      return NULL;
   }
   
   // buffer -1 = 1
   _jni_callv(system_data.main_env, DeleteLocalRef, byteArray);
   
   bitmap_w = _jni_callIntMethod(system_data.main_env, jbitmap, "getWidth");
   bitmap_h = _jni_callIntMethod(system_data.main_env, jbitmap, "getHeight");
   pitch  = _jni_callIntMethod(system_data.main_env, jbitmap, "getRowBytes");
   
   buffer_len = pitch * bitmap_h;
   
   ALLEGRO_DEBUG("bitmap w:%i, h:%i, pitch:%i, sz:%i bl:%i", bitmap_w, bitmap_h, pitch, bitmap_w * bitmap_h, buffer_len);
   
   // buffer +1 = 2, copies 3
   buffer = al_calloc(1, buffer_len);
   if(!buffer) {
      ALLEGRO_ERROR("failed to allocate memory");
      return NULL;
   }
   
   // FIXME: we can save a copy and allocation here by using the AndroidBitmap_lockPixels api, but that is only available on Android 2.2 and above.
   byte_buffer = _jni_call(system_data.main_env, jobject, NewDirectByteBuffer, buffer, buffer_len);
   if(!byte_buffer) {
      ALLEGRO_ERROR("failed to allocate DirectByteBuffer");
      return NULL;
   }
   
   // copies 4
   _jni_callVoidMethodV(system_data.main_env, jbitmap, "copyPixelsToBuffer", "(Ljava/nio/Buffer;)V", byte_buffer);
   
   // buffer -1 = 1
   _jni_callv(system_data.main_env, DeleteLocalRef, jbitmap);
   
   // flip bitmap
   ALLEGRO_DEBUG("do flip");
   
   // FIXME: this flipping code needs work.

   unsigned char *flipped = al_malloc(bitmap_w * bitmap_h * 4);
   ASSERT(flipped != NULL);
   
   _al_convert_bitmap_data(buffer + pitch * (bitmap_h-1), ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, -pitch,
                           flipped, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, bitmap_w * 4,
                           0, 0, 0, 0, bitmap_w, bitmap_h);
   
   al_free(buffer);
   
   ALLEGRO_DEBUG("done flip");
   
   al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
   
   bitmap = al_create_bitmap(bitmap_w, bitmap_h);
   al_restore_state(&state);
   if(!bitmap) {
      al_free(buffer);
      return NULL;
   }

   ALLEGRO_DEBUG("create bitmap ok");
   
   // buffer +1, = 2, copies 5
   ALLEGRO_DEBUG("bitmap->extra:%p", bitmap->extra);

   _al_ogl_upload_bitmap_memory(bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, flipped);

   ALLEGRO_DEBUG("upload bitmap memory ok");

   // buffer -1 = 1
   _jni_callv(system_data.main_env, DeleteLocalRef, byte_buffer);
   //al_free(buffer);
   
   ALLEGRO_DEBUG("load image end");
   return bitmap;
   
}*/

ALLEGRO_BITMAP *_al_android_load_image_f(ALLEGRO_FILE *fh, int flags)
{
   int x, y;
   jobject byte_buffer;
   jobject jbitmap;

   jmethodID input_stream_ctor;
   jobject input_stream;
   uint8_t *buffer = 0;
   int buffer_len = al_fsize(fh);
   ALLEGRO_BITMAP *bitmap = NULL;
   int bitmap_w = 0, bitmap_h = 0;
   ALLEGRO_STATE state;
   
   (void)flags;
   
   input_stream_ctor = _jni_call(system_data.main_env, jclass, GetMethodID, system_data.input_stream_class, "<init>", "(I)V");
   
   jint fhi = (jint)fh;
   
   input_stream = _jni_call(system_data.main_env, jobject, NewObject, system_data.input_stream_class, input_stream_ctor, fhi);

   if(!input_stream) {
      ALLEGRO_ERROR("failed to create new AllegroInputStream object");
      return NULL;
   }
   
   jbitmap = _jni_callObjectMethodV(system_data.main_env, system_data.activity_object, "decodeBitmap_f", "(Lorg/liballeg/app/AllegroInputStream;)Landroid/graphics/Bitmap;", input_stream);
   ASSERT(jbitmap != NULL);
   
   _jni_callv(system_data.main_env, DeleteLocalRef, input_stream);

   bitmap_w = _jni_callIntMethod(system_data.main_env, jbitmap, "getWidth");
   bitmap_h = _jni_callIntMethod(system_data.main_env, jbitmap, "getHeight");
   
   int pitch = _jni_callIntMethod(system_data.main_env, jbitmap, "getRowBytes");
   buffer_len = pitch * bitmap_h;
   
   ALLEGRO_DEBUG("bitmap size: %i", buffer_len);
   
   buffer = al_malloc(buffer_len);
   if(!buffer)
      return NULL;
   
   ALLEGRO_DEBUG("malloc %i bytes ok", buffer_len);
   
   // FIXME: at some point add support for the ndk AndroidBitmap api
   //        need to check for header at build time, and android version at runtime
   //        if thats even possible, might need to try and dynamically load the lib? I dunno.
   //        That would get rid of this buffer allocation and copy.
   byte_buffer = _jni_call(system_data.main_env, jobject, NewDirectByteBuffer, buffer, buffer_len);
   
   _jni_callVoidMethodV(system_data.main_env, jbitmap, "copyPixelsToBuffer", "(Ljava/nio/Buffer;)V", byte_buffer);
   
   // tell java we don't need the byte_buffer object
   _jni_callv(system_data.main_env, DeleteLocalRef, byte_buffer);

   // tell java we're done with the bitmap as well
   _jni_callv(system_data.main_env, DeleteLocalRef, jbitmap);

   al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
   bitmap = al_create_bitmap(bitmap_w, bitmap_h);
   al_restore_state(&state);
   if(!bitmap) {
      al_free(buffer);
      return NULL;
   }
   
   //_al_ogl_upload_bitmap_memory(bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, buffer);
   ALLEGRO_DEBUG("HERE");
   ALLEGRO_LOCKED_REGION *lr = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_WRITEONLY);
   for (y = 0; y < bitmap_h; y++) {
      for (x = 0; x < bitmap->w; x++) {
         memcpy(
            ((uint8_t *)lr->data)+lr->pitch*y,
            buffer+pitch*y,
            bitmap_w*4
         );
      }
   }
   al_unlock_bitmap(bitmap);
   ALLEGRO_DEBUG("HERE2");
   
   al_free(buffer);
   
   return bitmap;
}

ALLEGRO_BITMAP *_al_android_load_image(const char *filename, int flags)
{
   int x, y;
   jobject jbitmap;
   ALLEGRO_BITMAP *bitmap = NULL;
   int bitmap_w = 0, bitmap_h = 0;
   ALLEGRO_STATE state;
   
   (void)flags;
   
   jbitmap = _jni_callObjectMethodV(system_data.main_env, system_data.activity_object, "decodeBitmap", "(Ljava/lang/String;)Landroid/graphics/Bitmap;",
      (*system_data.main_env)->NewStringUTF(system_data.main_env, filename));
   ASSERT(jbitmap != NULL);
   
   bitmap_w = _jni_callIntMethod(system_data.main_env, jbitmap, "getWidth");
   bitmap_h = _jni_callIntMethod(system_data.main_env, jbitmap, "getHeight");
   
   ALLEGRO_DEBUG("bitmap dimensions: %d, %d", bitmap_w, bitmap_h);
   
   al_store_state(&state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
   bitmap = al_create_bitmap(bitmap_w, bitmap_h);
   al_restore_state(&state);
   if(!bitmap) {
      return NULL;
   }

   ALLEGRO_LOCKED_REGION *lr = al_lock_bitmap(bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, ALLEGRO_LOCK_WRITEONLY);

   jintArray ia = jbitmap = _jni_callObjectMethodV(system_data.main_env, system_data.activity_object, "getPixels", "(Landroid/graphics/Bitmap;)[I", jbitmap);
   jint *arr = (*system_data.main_env)->GetIntArrayElements(system_data.main_env, ia, 0);
   uint32_t *src = (uint32_t *)arr;
   uint32_t c;
   uint8_t a;
   for (y = 0; y < bitmap_h; y++) {
      /* Can use this for NO_PREMULTIPLIED_ALPHA I think
      _al_convert_bitmap_data(
         ((uint8_t *)arr) + (bitmap_w * 4) * y, ALLEGRO_PIXEL_FORMAT_ARGB_8888, bitmap_w*4,
         ((uint8_t *)lr->data) + lr->pitch * y, ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE, lr->pitch,
         0, 0, 0, 0, bitmap_w, 1
      );
      else use this: */
      uint32_t *dst = (uint32_t *)(((uint8_t *)lr->data) + lr->pitch * y);
      for (x = 0; x < bitmap_w; x++) {
         c = *src++;
         a = (c >> 24) & 0xff;
         c = (a << 24) | (((c >> 16) & 0xff) * a / 255) | ((((c >> 8) & 0xff) * a / 255) << 8) | (((c & 0xff) * a / 255) << 16);
         *dst++ = c;
      }
   }
   (*system_data.main_env)->ReleaseIntArrayElements(system_data.main_env, ia, arr, JNI_ABORT);

   // tell java we're done with the bitmap as well
   _jni_callv(system_data.main_env, DeleteLocalRef, jbitmap);

   al_unlock_bitmap(bitmap);

   return bitmap;
}

/* register system interfaces */

void _al_register_system_interfaces(void)
{
   ALLEGRO_SYSTEM_INTERFACE **add;

   /* add the native activity driver */
   add = _al_vector_alloc_back(&_al_system_interfaces);
   *add = _al_system_android_interface();
   
   /* TODO: add the non native activity driver */
}

/* vim: set sts=3 sw=3 et: */
