/*
 * Allegro5 Ogg Vorbis reader.
 * Can load samples and do streaming
 * author: Ryan Dickie (c) 2008
 */

#include "allegro5/acodec.h"
#include "allegro5/internal/aintern_acodec.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/threads.h"


#ifdef ALLEGRO_CFG_ACODEC_VORBIS

#include <vorbis/vorbisfile.h>


typedef struct AL_OV_DATA AL_OV_DATA;

struct AL_OV_DATA {
   OggVorbis_File *vf;
   vorbis_info *vi;
   ALLEGRO_FS_ENTRY *file;
   int bitstream;
};


static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *dptr)
{
   AL_OV_DATA *ov = (AL_OV_DATA *)dptr;
   size_t ret = 0;

   ret = al_fs_entry_read(ov->file, size*nmemb, ptr);

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

   if(!al_fs_entry_seek(ov->file, offset, whence)) {
      return -1;
   }

   return 0;
}

static long tell_callback(void *dptr)
{
   AL_OV_DATA *ov = (AL_OV_DATA *)dptr;
   int64_t ret = 0;

   ret = al_fs_entry_tell(ov->file);
   if(ret == -1)
      return -1;

   return (long)ret;
}

ov_callbacks callbacks = {
   read_callback,
   seek_callback,
   NULL,
   tell_callback
};

/* Function: al_load_sample_data_oggvorbis
 */
ALLEGRO_SAMPLE_DATA *al_load_sample_data_oggvorbis(const char *filename)
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
   ALLEGRO_FS_ENTRY *file;
   char *buffer;
   long pos;
   ALLEGRO_SAMPLE_DATA *sample;
   int channels;
   long rate;
   long total_samples;
   int bitstream;
   long total_size;
   AL_OV_DATA ov;

   file = al_fs_entry_open(filename, "rb");
   if (file == NULL) {
      TRACE("%s failed to open.\n", filename);
      return NULL;
   }

   ov.file = file;
   if (ov_open_callbacks(&ov, &vf, NULL, 0, callbacks) < 0) {
      TRACE("%s does not appear to be an Ogg bitstream.\n", filename);
      al_fs_entry_close(file);
      return NULL;
   }

   vi = ov_info(&vf, -1);

   channels = vi->channels;
   rate = vi->rate;
   total_samples = ov_pcm_total(&vf, -1);
   bitstream = -1;
   total_size = total_samples * channels * word_size;

   TRACE("Loaded sample %s with properties:\n", filename);
   TRACE("    channels %d\n", channels);
   TRACE("    word_size %d\n", word_size);
   TRACE("    rate %ld\n", rate);
   TRACE("    total_samples %ld\n", total_samples);
   TRACE("    total_size %ld\n", total_size);

   buffer = _AL_MALLOC_ATOMIC(total_size);
   if (!buffer) {
      al_fs_entry_close(file);
      return NULL;
   }

   pos = 0;
   while (pos < total_size) {
      /* XXX error handling */
      long read = ov_read(&vf, buffer + pos, packet_size, endian, word_size,
         signedness, &bitstream);
      pos += read;
      if (read == 0)
         break;
   }

   ov_clear(&vf);
   al_fs_entry_close(file);

   sample = al_create_sample_data(buffer, total_samples, rate,
      _al_word_size_to_depth_conf(word_size),
      _al_count_to_channel_conf(channels), true);

   if (!sample) {
      _AL_FREE(buffer);
   }

   return sample;
}


static bool ogg_stream_rewind(ALLEGRO_STREAM *stream)
{
   AL_OV_DATA *extra = (AL_OV_DATA *) stream->extra;
   return (ov_raw_seek(extra->vf, 0) != -1);
}


/* To be called when stream is destroyed */
static void ogg_stream_close(ALLEGRO_STREAM *stream)
{
   AL_OV_DATA *extra = (AL_OV_DATA *) stream->extra;
   ALLEGRO_EVENT quit_event;

   quit_event.type = _KCM_STREAM_FEEDER_QUIT_EVENT_TYPE;
   al_emit_user_event(*((ALLEGRO_EVENT_SOURCE**)stream), &quit_event, NULL);
   al_join_thread(stream->feed_thread, NULL);
   al_destroy_thread(stream->feed_thread);

   ov_clear(extra->vf);
   al_fs_entry_close(extra->file);
   _AL_FREE(extra->vf);
   _AL_FREE(extra);
   stream->extra = NULL;
   stream->feed_thread = NULL;
}


static size_t ogg_stream_update(ALLEGRO_STREAM *stream, void *data,
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
   while (pos < buf_size) {
      unsigned long read = ov_read(extra->vf, (char *)data + pos,
                                   buf_size - pos, endian, word_size,
                                   signedness, &extra->bitstream);
      pos += read;

      /* If nothing read then now to silence from here to the end. */
      if (read == 0) {
         int silence = _al_audio_get_silence(stream->spl.spl_data.depth);
         memset((char *)data + pos, silence, buf_size - pos);
         /* return the number of usefull byes written */
         return pos;
      }
   }

   return pos;
}


/* Function: al_load_stream_oggvorbis
 */
ALLEGRO_STREAM *al_load_stream_oggvorbis(size_t buffer_count,
                                         unsigned long samples,
                                         const char *filename)
{
   const int word_size = 2; /* 1 = 8bit, 2 = 16-bit. nothing else */
   ALLEGRO_FS_ENTRY* file = NULL;
   OggVorbis_File* vf;
   vorbis_info* vi;
   int channels;
   long rate;
   long total_samples;
   long total_size;
   AL_OV_DATA* extra;
   ALLEGRO_STREAM* stream;

   extra = _AL_MALLOC(sizeof(AL_OV_DATA));
   if (extra == NULL) {
      TRACE("Failed to allocate AL_OV_DATA struct.\n");
      return NULL;
   }
   
   file = al_fs_entry_open(filename, "rb");
   if (file == NULL) {
      TRACE("%s failed to open\n", filename);
      return NULL;
   }
   
   extra->file = file;
   
   vf = _AL_MALLOC(sizeof(OggVorbis_File));
   if (ov_open_callbacks(extra, vf, NULL, 0, callbacks) < 0) {
      TRACE("ogg: Input does not appear to be an Ogg bitstream.\n");
      al_fs_entry_close(file);
      return NULL;
   }

   extra->vf = vf;

   vi = ov_info(vf, -1);
   channels = vi->channels;
   rate = vi->rate;
   total_samples = ov_pcm_total(vf,-1);
   total_size = total_samples * channels * word_size;

   extra->vi = vi;

   extra->bitstream = -1;

   TRACE("ogg: loaded sample %s with properties:\n", filename);
   TRACE("ogg:     channels %d\n", channels);
   TRACE("ogg:     word_size %d\n", word_size);
   TRACE("ogg:     rate %ld\n", rate);
   TRACE("ogg:     total_samples %ld\n", total_samples);
   TRACE("ogg:     total_size %ld\n", total_size);

   stream = al_create_stream(buffer_count, samples, rate,
            _al_word_size_to_depth_conf(word_size),
            _al_count_to_channel_conf(channels));

   stream->extra = extra;

   stream->feed_thread = al_create_thread(_al_kcm_feed_stream, stream);
   stream->quit_feed_thread = false;
   stream->feeder = ogg_stream_update;
   stream->rewind_feeder = ogg_stream_rewind;
   stream->unload_feeder = ogg_stream_close;
   al_start_thread(stream->feed_thread);

   return stream;
}


#endif /* ALLEGRO_CFG_ACODEC_VORBIS */

/* vim: set sts=3 sw=3 et: */
