/**
 * allegro audio codec loader
 * author: Ryan Dickie (c) 2008
 * todo: unicode file paths ;)
 * todo: add streaming support
 */

#ifndef AINTERN_ACODEC_H
#define AINTERN_ACODEC_H

#include "allegro5/audio.h"
#include "allegro5/internal/aintern_acodec_cfg.h"

#if defined(ALLEGRO_CFG_ACODEC_FLAC)
   ALLEGRO_SAMPLE* al_load_sample_flac(const char *filename);
#endif

#if defined(ALLEGRO_CFG_ACODEC_SNDFILE)
   ALLEGRO_SAMPLE* al_load_sample_sndfile(const char *filename);
#endif

#if defined(ALLEGRO_CFG_ACODEC_VORBIS)
   ALLEGRO_SAMPLE* al_load_sample_oggvorbis(const char *filename);
#endif

#endif
