
typedef struct ALLEGRO_VIDEO_INTERFACE {
   bool (*open_video)(ALLEGRO_VIDEO *video);
   bool (*close_video)(ALLEGRO_VIDEO *video);
   bool (*start_video)(ALLEGRO_VIDEO *video);
   bool (*pause_video)(ALLEGRO_VIDEO *video);
   bool (*seek_video)(ALLEGRO_VIDEO *video);
   bool (*update_video)(ALLEGRO_VIDEO *video);
} ALLEGRO_VIDEO_INTERFACE;

struct ALLEGRO_VIDEO {
   ALLEGRO_VIDEO_INTERFACE *vtable;
   
   /* video */
   ALLEGRO_BITMAP *current_frame;
   double video_position;
   double fps;
   int width, height;
   
   /* audio */
   ALLEGRO_MIXER *mixer;
   ALLEGRO_VOICE *voice;
   int audio_buffer_count;
   int audio_samples_per_buffer;
   ALLEGRO_AUDIO_STREAM *audio;
   double audio_position;
   double audio_rate;

   /* general */
   ALLEGRO_EVENT_SOURCE es;
   ALLEGRO_PATH *filename;
   bool paused;
   double audio_offset;
   double position;
   double seek_to;
   double aspect_ratio;

   /* implementation specific */
   void *data;
};

extern ALLEGRO_VIDEO_INTERFACE *_al_video_vtable;
