#ifndef __al_included_allegro5_aintern_file_h
#define __al_included_allegro5_aintern_file_h

#ifdef __cplusplus
   extern "C" {
#endif


extern const ALLEGRO_FILE_INTERFACE _al_file_interface_stdio;

struct ALLEGRO_FILE
{
   const ALLEGRO_FILE_INTERFACE *vtable;
   void *userdata;
};

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
