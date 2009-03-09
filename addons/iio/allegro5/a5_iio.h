#ifndef A5_IIO_H
#define A5_IIO_H

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_IIO_SRC
         #define _A5_IIO_DLL __declspec(dllexport)
      #else
         #define _A5_IIO_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_IIO_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_IIO_FUNC(type, name, args)      _A5_IIO_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define A5_IIO_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define A5_IIO_FUNC(type, name, args)      extern _A5_IIO_DLL type name args
#else
   #define A5_IIO_FUNC      AL_FUNC
#endif


#ifdef __cplusplus
extern "C" {
#endif

typedef ALLEGRO_BITMAP *(*IIO_LOADER_FUNCTION)(const char *filename);
typedef ALLEGRO_BITMAP *(*IIO_FS_LOADER_FUNCTION)(ALLEGRO_FS_ENTRY *pf);
typedef int (*IIO_SAVER_FUNCTION)(const char *filename, ALLEGRO_BITMAP *bitmap);
typedef int (*IIO_FS_SAVER_FUNCTION)(ALLEGRO_FS_ENTRY *pf, ALLEGRO_BITMAP *bitmap);


A5_IIO_FUNC(bool, al_iio_init, (void));
A5_IIO_FUNC(bool, al_iio_add_handler, (const char *ext, IIO_LOADER_FUNCTION loader, IIO_SAVER_FUNCTION saver, IIO_FS_LOADER_FUNCTION fs_loader, IIO_FS_SAVER_FUNCTION fs_saver));
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_iio_load, (const char *filename));
A5_IIO_FUNC(ALLEGRO_BITMAP *, al_iio_load_entry, (ALLEGRO_FS_ENTRY *pf, const char *ident));
A5_IIO_FUNC(int, al_iio_save, (const char *filename, ALLEGRO_BITMAP *bitmap));
A5_IIO_FUNC(int, al_iio_save_entry, (ALLEGRO_FS_ENTRY *pf, const char *ident, ALLEGRO_BITMAP *bitmap));

#ifdef __cplusplus
}
#endif

#endif // A5_IIO_H

