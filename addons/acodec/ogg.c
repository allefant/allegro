/*
 * Allegro5 Ogg Vorbis reader.
 * Can load samples and do streaming
 * author: Ryan Dickie (c) 2008
 */

#include "allegro5/allegro5.h"
#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_system.h"
#include "acodec.h"

ALLEGRO_DEBUG_CHANNEL("acodec")

/* FIXME: We need a ALLEGRO_CFG_TREMOR */
#if !defined(ALLEGRO_GP2XWIZ) && !defined(ALLEGRO_IPHONE)
#include <vorbis/vorbisfile.h>
#else
#include <tremor/ivorbisfile.h>
#define TREMOR 1
#endif

typedef struct AL_OV_DATA AL_OV_DATA;

struct AL_OV_DATA {
   OggVorbis_File *vf;
   vorbis_info *vi;
   ALLEGRO_FILE *file;
   int bitstream;
   double loop_start;
   double loop_end;
};


/* dynamic loading support (Windows only currently) */
#ifdef ALLEGRO_CFG_ACODEC_VORBISFILE_DLL
static void *ov_dll = NULL;
#endif

static struct
{
   int (*ov_clear)(OggVorbis_File *);
   int (*ov_open_callbacks)(void *, OggVorbis_File *, char *, long, ov_callbacks);
   ogg_int64_t (*ov_pcm_total)(OggVorbis_File *, int);
   vorbis_info *(*ov_info)(OggVorbis_File *, int);
#ifndef TREMOR
   double (*ov_time_total)(OggVorbis_File *, int);
   int (*ov_time_seek_lap)(OggVorbis_File *, double);
   double (*ov_time_tell)(OggVorbis_File *);
   long (*ov_read)(OggVorbis_File *, char *, int, int, int, int, int *);
#else
   ogg_int64_t (*ov_time_total)(OggVorbis_File *, int);
   int (*ov_time_seek)(OggVorbis_File *, ogg_int64_t);
   ogg_int64_t (*ov_time_tell)(OggVorbis_File *);
   long (*ov_read)(OggVorbis_File *, char *, int, int *);
#endif
} lib;


#ifdef ALLEGRO_CFG_ACODEC_VORBISFILE_DLL
static void shutdown_dynlib(void)
{
   if (ov_dll) {
      _al_close_library(ov_dll);
      ov_dll = NULL;
   }
}
#endif


static bool init_dynlib(void)
{
#ifdef ALLEGRO_CFG_ACODEC_VORBISFILE_DLL
   if (ov_dll) {
      return true;
   }

   ov_dll = _al_open_library(ALLEGRO_CFG_ACODEC_VORBISFILE_DLL);
   if (!ov_dll) {
      ALLEGRO_WARN("Could not load " ALLEGRO_CFG_ACODEC_VORBISFILE_DLL "\n");
      return false;
   }

   _al_add_exit_func(shutdown_dynlib, "shutdown_dynlib");

   #define INITSYM(x)   (lib.x = _al_import_symbol(ov_dll, #x))
#else
   #define INITSYM(x)   (lib.x = (x))
#endif

   memset(&lib, 0, sizeof(lib));

   INITSYM(ov_clear);
   INITSYM(ov_open_callbacks);
   INITSYM(ov_pcm_total);
   INITSYM(ov_info);
#ifndef TREMOR
   INITSYM(ov_time_total);
   INITSYM(ov_time_seek_lap);
   INITSYM(ov_time_tell);
   INITSYM(ov_read);
#else
   INITSYM(ov_time_total);
   INITSYM(ov_time_seek);
   INITSYM(ov_time_tell);
   INITSYM(ov_read);
#endif

   /* Check that all symbols are defined. */
   {
      intptr_t *p = (intptr_t *) &lib;
      size_t n = sizeof(lib) / sizeof(void *);
      unsigned i;

      for (i = 0; i < n; i++) {
         if (p[i] == 0) {
            ALLEGRO_ERROR("undefined symbol in lib structure\n");
            return false;
         }
      }
   }

   return true;

#undef INITSYM
}


static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *dptr)
{
   AL_OV_DATA *ov = (AL_OV_DATA *)dptr;
   size_t ret = 0;

   ret = al_fread(ov->file, ptr, size*nmemb);

   return ret;
}


static int seek_callback(void *dptr, ogg_int64_t offset, int whence)
{
   AL_OV_DATA *ov = (AL_OV_DATA *)dptr;

   switch(whence) {
      case SEEK_SET: whence = ALLEGRO_SEEK_SET; break;
      case SEEK_CUR: whence = ALLEGRO_SEEK_CUR; break;
      case SEEK_END: whence = ALLEGRO_SEEK_END; break;
   }

   if(!al_fseek(ov->file, offset, whence)) {
      return -1;
   }

   return 0;
}


static long tell_callback(void *dptr)
{
   AL_OV_DATA *ov = (AL_OV_DATA *)dptr;
   int64_t ret = 0;

   ret = al_ftell(ov->file);
   if(ret == -1)
      return -1;

   return (long)ret;
}


static int close_callback(void *dptr)
{
   AL_OV_DATA *d = (void *)dptr;
   al_fclose(d->file);
   return 0;
}


static ov_callbacks callbacks = {
   read_callback,
   seek_callback,
   close_callback,
   tell_callback
};


ALLEGRO_SAMPLE *_al_load_ogg_vorbis(const char *filename)
{
   ALLEGRO_FILE *f;
   ALLEGRO_SAMPLE *spl;
   ASSERT(filename);
   
   ALLEGRO_INFO("Loading sample %s.\n", filename);
   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_WARN("Failed reading %s.\n", filename);
      return NULL;
   }

   spl = _al_load_ogg_vorbis_f(f);

   return spl;
}


ALLEGRO_SAMPLE *_al_load_ogg_vorbis_f(ALLEGRO_FILE *file)
{
   /* Note: decoding library returns floats.  I always return 16-bit (most
    * commonly supported).
    */
#ifdef ALLEGRO_LITTLE_ENDIAN
   const int endian = 0; /* 0 for Little-Endian, 1 for Big-Endian */
#else
   const int endian = 1; /* 0 for Little-Endian, 1 for Big-Endian */
#endif
   int word_size = 2; /* 1 = 8bit, 2 = 16-bit. nothing else */
   int signedness = 1; /* 0  for unsigned, 1 for signed */
   const int packet_size = 4096; /* suggestion for size to read at a time */
   OggVorbis_File vf;
   vorbis_info* vi;
   char *buffer;
   long pos;
   ALLEGRO_SAMPLE *sample;
   int channels;
   long rate;
   long total_samples;
   int bitstream;
   long total_size;
   AL_OV_DATA ov;

   if (!init_dynlib()) {
      return NULL;
   }

   ov.file = file;
   if (lib.ov_open_callbacks(&ov, &vf, NULL, 0, callbacks) < 0) {
      ALLEGRO_WARN("Audio file does not appear to be an Ogg bitstream.\n");
      al_fclose(file);
      return NULL;
   }

   vi = lib.ov_info(&vf, -1);

   channels = vi->channels;
   rate = vi->rate;
   total_samples = lib.ov_pcm_total(&vf, -1);
   bitstream = -1;
   total_size = total_samples * channels * word_size;

   ALLEGRO_DEBUG("channels %d\n", channels);
   ALLEGRO_DEBUG("word_size %d\n", word_size);
   ALLEGRO_DEBUG("rate %ld\n", rate);
   ALLEGRO_DEBUG("total_samples %ld\n", total_samples);
   ALLEGRO_DEBUG("total_size %ld\n", total_size);

   buffer = _AL_MALLOC_ATOMIC(total_size);
   if (!buffer) {
      al_fclose(file);
      return NULL;
   }

   pos = 0;
   while (pos < total_size) {
      /* XXX error handling */
#ifndef TREMOR
      long read = lib.ov_read(&vf, buffer + pos, packet_size, endian, word_size,
         signedness, &bitstream);
#else
      (void)endian;
      (void)signedness;
      long read = lib.ov_read(&vf, buffer + pos, packet_size, &bitstream);
#endif
      pos += read;
      if (read == 0)
         break;
   }

   lib.ov_clear(&vf);

   sample = al_create_sample(buffer, total_samples, rate,
      _al_word_size_to_depth_conf(word_size),
      _al_count_to_channel_conf(channels), true);

   if (!sample) {
      _AL_FREE(buffer);
   }

   return sample;
}


static bool ogg_stream_seek(ALLEGRO_AUDIO_STREAM *stream, double time)
{
   AL_OV_DATA *extra = (AL_OV_DATA *) stream->extra;
   if (time >= extra->loop_end)
      return false;
#ifndef TREMOR
   return (lib.ov_time_seek_lap(extra->vf, time) != -1);
#else
   return lib.ov_time_seek(extra->vf, time*1000) != -1;
#endif
}


static bool ogg_stream_rewind(ALLEGRO_AUDIO_STREAM *stream)
{
   AL_OV_DATA *extra = (AL_OV_DATA *) stream->extra;
   return ogg_stream_seek(stream, extra->loop_start);
}


static double ogg_stream_get_position(ALLEGRO_AUDIO_STREAM *stream)
{
   AL_OV_DATA *extra = (AL_OV_DATA *) stream->extra;
#ifndef TREMOR
   return lib.ov_time_tell(extra->vf);
#else
   return lib.ov_time_tell(extra->vf)/1000.0;
#endif
}


static double ogg_stream_get_length(ALLEGRO_AUDIO_STREAM *stream)
{
   AL_OV_DATA *extra = (AL_OV_DATA *) stream->extra;
#ifndef TREMOR
   double ret = lib.ov_time_total(extra->vf, -1);
#else
   double ret = lib.ov_time_total(extra->vf, -1)/1000.0;
#endif
   return ret;
}


static bool ogg_stream_set_loop(ALLEGRO_AUDIO_STREAM *stream, double start, double end)
{
   AL_OV_DATA *extra = (AL_OV_DATA *) stream->extra;

   extra->loop_start = start;
   extra->loop_end = end;
   
   return true;
}


/* To be called when stream is destroyed */
static void ogg_stream_close(ALLEGRO_AUDIO_STREAM *stream)
{
   AL_OV_DATA *extra = (AL_OV_DATA *) stream->extra;
   ALLEGRO_EVENT quit_event;


   quit_event.type = _KCM_STREAM_FEEDER_QUIT_EVENT_TYPE;
   al_emit_user_event(al_get_audio_stream_event_source(stream), &quit_event, NULL);
   al_join_thread(stream->feed_thread, NULL);
   al_destroy_thread(stream->feed_thread);

   lib.ov_clear(extra->vf);
   _AL_FREE(extra->vf);
   _AL_FREE(extra);
   stream->extra = NULL;
   stream->feed_thread = NULL;
}


static size_t ogg_stream_update(ALLEGRO_AUDIO_STREAM *stream, void *data,
                                size_t buf_size)
{
   AL_OV_DATA *extra = (AL_OV_DATA *) stream->extra;
	
#ifdef ALLEGRO_LITTLE_ENDIAN
   const int endian = 0;      /* 0 for Little-Endian, 1 for Big-Endian */
#else
   const int endian = 1;      /* 0 for Little-Endian, 1 for Big-Endian */
#endif
   const int word_size = 2;   /* 1 = 8bit, 2 = 16-bit. nothing else */
   const int signedness = 1;  /* 0 for unsigned, 1 for signed */

   unsigned long pos = 0;
   int read_length = buf_size;
#ifndef TREMOR
   double ctime = lib.ov_time_tell(extra->vf);
#else
   double ctime = lib.ov_time_tell(extra->vf)/1000.0;
#endif
   double rate = extra->vi->rate;
   double btime = ((double)buf_size / (double)word_size) / rate;
   
   if (stream->spl.loop == _ALLEGRO_PLAYMODE_STREAM_ONEDIR) {
      if (ctime + btime > extra->loop_end) {
         read_length = (extra->loop_end - ctime) * rate * (double)word_size;
         if (read_length < 0)
            return 0;
         read_length += read_length % word_size;
         }
      }
   while (pos < (unsigned long)read_length) {
#ifndef TREMOR
      unsigned long read = lib.ov_read(extra->vf, (char *)data + pos,
         read_length - pos, endian, word_size, signedness, &extra->bitstream);
#else
      (void)endian;
      (void)signedness;
      unsigned long read = lib.ov_read(extra->vf, (char *)data + pos,
         read_length - pos, &extra->bitstream);
#endif
      pos += read;
	   
      /* If nothing read then now to silence from here to the end. */
      if (read == 0) {
         int silence = _al_kcm_get_silence(stream->spl.spl_data.depth);
         memset((char *)data + pos, silence, buf_size - pos);
         /* return the number of usefull byes written */
         return pos;
      }
   }

   return pos;
}


ALLEGRO_AUDIO_STREAM *_al_load_ogg_vorbis_audio_stream(const char *filename,
   size_t buffer_count, unsigned int samples)
{
   ALLEGRO_FILE *f;
   ALLEGRO_AUDIO_STREAM *stream;
   ASSERT(filename);

   ALLEGRO_INFO("Loading stream %s.\n", filename);
   f = al_fopen(filename, "rb");
   if (!f) {
      ALLEGRO_WARN("Failed reading %s.\n", filename);
      return NULL;
   }

   stream = _al_load_ogg_vorbis_audio_stream_f(f, buffer_count, samples);

   return stream;
}


ALLEGRO_AUDIO_STREAM *_al_load_ogg_vorbis_audio_stream_f(ALLEGRO_FILE *file,
   size_t buffer_count, unsigned int samples)
{
   const int word_size = 2; /* 1 = 8bit, 2 = 16-bit. nothing else */
   OggVorbis_File* vf;
   vorbis_info* vi;
   int channels;
   long rate;
   long total_samples;
   long total_size;
   AL_OV_DATA* extra;
   ALLEGRO_AUDIO_STREAM* stream;

   if (!init_dynlib()) {
      return NULL;
   }

   extra = _AL_MALLOC(sizeof(AL_OV_DATA));
   if (extra == NULL) {
      ALLEGRO_ERROR("Failed to allocate AL_OV_DATA struct.\n");
      return NULL;
   }

   extra->file = file;
   
   vf = _AL_MALLOC(sizeof(OggVorbis_File));
   if (lib.ov_open_callbacks(extra, vf, NULL, 0, callbacks) < 0) {
      ALLEGRO_WARN("ogg: Input does not appear to be an Ogg bitstream.\n");
      al_fclose(file);
      return NULL;
   }

   extra->vf = vf;

   vi = lib.ov_info(vf, -1);
   channels = vi->channels;
   rate = vi->rate;
   total_samples = lib.ov_pcm_total(vf, -1);
   total_size = total_samples * channels * word_size;

   extra->vi = vi;

   extra->bitstream = -1;

   ALLEGRO_DEBUG("channels %d\n", channels);
   ALLEGRO_DEBUG("word_size %d\n", word_size);
   ALLEGRO_DEBUG("rate %ld\n", rate);
   ALLEGRO_DEBUG("total_samples %ld\n", total_samples);
   ALLEGRO_DEBUG("total_size %ld\n", total_size);
	
   stream = al_create_audio_stream(buffer_count, samples, rate,
            _al_word_size_to_depth_conf(word_size),
            _al_count_to_channel_conf(channels));
   if (!stream) {
      lib.ov_clear(vf);
      free(vf);
      return NULL;
   }

   stream->extra = extra;

   extra->loop_start = 0.0;
   extra->loop_end = ogg_stream_get_length(stream);
   stream->feed_thread = al_create_thread(_al_kcm_feed_stream, stream);
   stream->quit_feed_thread = false;
   stream->feeder = ogg_stream_update;
   stream->rewind_feeder = ogg_stream_rewind;
   stream->seek_feeder = ogg_stream_seek;
   stream->get_feeder_position = ogg_stream_get_position;
   stream->get_feeder_length = ogg_stream_get_length;
   stream->set_feeder_loop = ogg_stream_set_loop;
   stream->unload_feeder = ogg_stream_close;
   al_start_thread(stream->feed_thread);
	
   return stream;
}


/* vim: set sts=3 sw=3 et: */
