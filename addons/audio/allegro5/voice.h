/**
 * ALLEGRO_VOICE header file
 * User visible voice functions
 * Ryan Dickie
 *
 * TODO: documentation
 */

#ifndef A5_VOICE_H
#define A5_VOICE_H

/* struct ALLEGRO_VOICE:
*/
typedef struct ALLEGRO_VOICE ALLEGRO_VOICE;
A5_AUDIO_FUNC(ALLEGRO_VOICE*, al_voice_create, (ALLEGRO_SAMPLE* sample));
A5_AUDIO_FUNC(ALLEGRO_VOICE*, al_voice_create_stream, (ALLEGRO_STREAM* stream));
A5_AUDIO_FUNC(void, al_voice_destroy, (ALLEGRO_VOICE *voice));
A5_AUDIO_FUNC(unsigned long, al_voice_get_position, (const ALLEGRO_VOICE* voice));
A5_AUDIO_FUNC(bool, al_voice_is_playing, (const ALLEGRO_VOICE* voice));
A5_AUDIO_FUNC(int, al_voice_set_position, (ALLEGRO_VOICE* voice, unsigned long position));
A5_AUDIO_FUNC(int, al_voice_set_loop_mode, (ALLEGRO_VOICE* voice, ALLEGRO_AUDIO_ENUM loop_mode));
A5_AUDIO_FUNC(int, al_voice_get_loop_mode, (ALLEGRO_VOICE* voice, ALLEGRO_AUDIO_ENUM loop_mode));
A5_AUDIO_FUNC(int, al_voice_pause, (ALLEGRO_VOICE* voice));
A5_AUDIO_FUNC(int, al_voice_stop, (ALLEGRO_VOICE* voice));
A5_AUDIO_FUNC(int, al_voice_start, (ALLEGRO_VOICE* voice));
#endif
