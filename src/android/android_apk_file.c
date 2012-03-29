#include <allegro5/allegro.h>
#include <allegro5/internal/aintern_android.h>

ALLEGRO_DEBUG_CHANNEL("android");


typedef struct ALLEGRO_FILE_APK ALLEGRO_FILE_APK;

struct ALLEGRO_FILE_APK
{
   jobject apk;
   bool error_indicator;
};

/* forward declaration */
static const ALLEGRO_FILE_INTERFACE file_apk_vtable;

const ALLEGRO_FILE_INTERFACE *_al_get_apk_file_vtable(void)
{
   return &file_apk_vtable;
}


#define streq(a, b)  (0 == strcmp(a, b))


static ALLEGRO_FILE_APK *cast_stream(ALLEGRO_FILE *f)
{
   return (ALLEGRO_FILE_APK *)al_get_file_userdata(f);
}

static void apk_set_errno(ALLEGRO_FILE_APK *fp)
{
   al_set_errno(-1);

   if (fp) {
      fp->error_indicator = true;
   }
}

static jobject APK_openRead(const char *filename)
{
   JNIEnv *jnienv = _al_android_get_jnienv();

   jmethodID ctor = _jni_call(jnienv, jclass, GetMethodID, _al_android_apk_stream_class(), "<init>", "(Lorg/liballeg/app/AllegroActivity;Ljava/lang/String;)V");

   jobject is = _jni_call(jnienv, jobject, NewObject, _al_android_apk_stream_class(), ctor, _al_android_activity_object(), (*jnienv)->NewStringUTF(jnienv, filename));
   
   jboolean res = _jni_callBooleanMethodV(_al_android_get_jnienv(), is, "open", "()Z");

   if (res == false)
      return NULL;

   return is;
}

static void APK_close(jobject apk_stream)
{
   _jni_callVoidMethod(_al_android_get_jnienv(), apk_stream, "close");
}

static bool APK_seek(jobject apk_stream, long bytes)
{
   jboolean res = _jni_callBooleanMethodV(_al_android_get_jnienv(), apk_stream, "seek", "(J)Z", (jlong)bytes);
   return res;
}

static bool APK_tell(jobject apk_stream)
{
   long res = _jni_callLongMethodV(_al_android_get_jnienv(), apk_stream, "tell", "()J");
   return res;
}

static int APK_read(jobject apk_stream, char *buf, int len)
{
   JNIEnv *jnienv = _al_android_get_jnienv();

   jbyteArray b = (*jnienv)->NewByteArray(jnienv, len);

   jint res = _jni_callIntMethodV(jnienv, apk_stream, "read", "([B)I", b);

   if (res > 0) {
      (*jnienv)->GetByteArrayRegion(jnienv, b, 0, res, (jbyte *)buf);
   }

   _jni_callv(jnienv, DeleteLocalRef, b);

   return res;
}

static long APK_size(jobject apk_stream)
{
   long res = _jni_callLongMethod(_al_android_get_jnienv(), apk_stream, "size");
   return res;
}

static void *file_apk_fopen(const char *filename, const char *mode)
{
   ALLEGRO_FILE_APK *fp;
   jobject apk;

   if (streq(mode, "r") || streq(mode, "rb"))
      apk = APK_openRead(filename);
   else
      return NULL;

   if (!apk) {
      apk_set_errno(NULL);
      return NULL;
   }

   fp = al_malloc(sizeof(*fp));
   if (!fp) {
      al_set_errno(ENOMEM);
      APK_close(apk);
      return NULL;
   }

   fp->apk = apk;
   fp->error_indicator = false;

   return fp;
}


static void file_apk_fclose(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_APK *fp = cast_stream(f);

   APK_close(fp->apk);

   al_free(fp);
}


static size_t file_apk_fread(ALLEGRO_FILE *f, void *buf, size_t buf_size)
{
   ALLEGRO_FILE_APK *fp = cast_stream(f);
   int n;

   if (buf_size == 0)
      return 0;

   n = APK_read(fp->apk, buf, buf_size);
   if (n < 0) {
      apk_set_errno(fp);
      return 0;
   }
   return n;
}


static size_t file_apk_fwrite(ALLEGRO_FILE *f, const void *buf,
   size_t buf_size)
{
   // Not supported
   (void)f;
   (void)buf;
   (void)buf_size;
   return 0;
}


static bool file_apk_fflush(ALLEGRO_FILE *f)
{
   // Not supported
   (void)f;
   return true;
}


static int64_t file_apk_ftell(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_APK *fp = cast_stream(f);
   return APK_tell(fp->apk);
}


static bool file_apk_seek(ALLEGRO_FILE *f, int64_t offset, int whence)
{
   ALLEGRO_FILE_APK *fp = cast_stream(f);
   long base;
   
   switch (whence) {
      case ALLEGRO_SEEK_SET:
         base = 0;
         break;

      case ALLEGRO_SEEK_CUR:
         base = APK_tell(fp->apk);
         if (base < 0) {
            apk_set_errno(fp);
            return false;
         }
         break;

      case ALLEGRO_SEEK_END:
         base = APK_size(fp->apk);
         if (base < 0) {
            apk_set_errno(fp);
            return false;
         }
         break;

      default:
         al_set_errno(EINVAL);
         return false;
   }

   ALLEGRO_DEBUG("in allegro, seek to base=%ld ofs=%lld total=%lld", base, offset, base+offset);

   if (!APK_seek(fp->apk, base + offset)) {
      apk_set_errno(fp);
      return false;
   }
   
   return true;
}


static bool file_apk_feof(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_APK *fp = cast_stream(f);
   jboolean res = _jni_callBooleanMethodV(_al_android_get_jnienv(), fp->apk, "eof", "()Z");
   return res;
}


static bool file_apk_ferror(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_APK *fp = cast_stream(f);

   return fp->error_indicator;
}


static void file_apk_fclearerr(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_APK *fp = cast_stream(f);

   fp->error_indicator = false;
}

static int file_apk_ungetc(ALLEGRO_FILE *f, int c)
{
   ALLEGRO_FILE_APK *fp = cast_stream(f);
   _jni_callVoidMethodV(_al_android_get_jnienv(), fp->apk, "ungetc", "(I)V");
   return c;
}

static off_t file_apk_fsize(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_APK *fp = cast_stream(f);
   return APK_size(fp->apk);
}

static const ALLEGRO_FILE_INTERFACE file_apk_vtable =
{
   file_apk_fopen,
   file_apk_fclose,
   file_apk_fread,
   file_apk_fwrite,
   file_apk_fflush,
   file_apk_ftell,
   file_apk_seek,
   file_apk_feof,
   file_apk_ferror,
   file_apk_fclearerr,
   file_apk_ungetc,
   file_apk_fsize
};


/* Function: al_set_apk_file_interface
 */
void al_set_apk_file_interface(void)
{
   al_set_new_file_interface(&file_apk_vtable);
   //_al_set_apk_fs_interface();
}


/* Function: al_get_allegro_apk_file_version
 */
uint32_t al_get_allegro_apk_file_version(void)
{
   return ALLEGRO_VERSION_INT;
}


/* vim: set sts=3 sw=3 et: */

