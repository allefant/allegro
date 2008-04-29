/**
 * allegro audio codec loader
 * author: Ryan Dickie (c) 2008
 * todo: unicode file paths ;)
 * todo: add streaming support
 */

#ifndef ACODEC_H
#define ACODEC_H

#include "allegro5/audio.h"
AL_FUNC(ALLEGRO_SAMPLE*, al_load_sample,(const char* filename));
AL_FUNC(ALLEGRO_STREAM*, al_load_stream,(const char* filename));

#endif
