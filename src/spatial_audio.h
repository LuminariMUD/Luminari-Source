/*************************************************************************
*   File: spatial_audio.h                              Part of LuminariMUD *
*  Usage: Audio stimulus strategy interface                                *
*  Author: Luminari Development Team                                       *
*                                                                          *
*  All rights reserved.  See license for complete information.            *
*                                                                          *
*  LuminariMUD is based on CircleMUD, Copyright (C) 1993, 94 by the       *
*  Department of Computer Science at the Johns Hopkins University         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef _SPATIAL_AUDIO_H_
#define _SPATIAL_AUDIO_H_

#include "spatial_core.h"

/* Audio-specific constants */
#define AUDIO_BASE_RANGE                  1500.0f
#define AUDIO_ECHO_RANGE                  800.0f
#define AUDIO_WHISPER_RANGE               50.0f
#define AUDIO_SHOUT_RANGE                 500.0f
#define AUDIO_THUNDER_RANGE               3000.0f

/* Audio message types based on intensity and characteristics */
typedef enum {
    AUDIO_MSG_CLEAR,        /* Crystal clear audio */
    AUDIO_MSG_DISTANT,      /* Distant but recognizable */
    AUDIO_MSG_MUFFLED,      /* Muffled by obstacles */
    AUDIO_MSG_ECHO,         /* Echoing off terrain */
    AUDIO_MSG_FAINT,        /* Barely audible */
    AUDIO_MSG_RUMBLE        /* Deep rumbling sound */
} audio_message_type_t;

/* Audio source types */
typedef enum {
    AUDIO_SOURCE_VOICE,     /* Human/creature voice */
    AUDIO_SOURCE_COMBAT,    /* Weapon clashes, battle sounds */
    AUDIO_SOURCE_MAGIC,     /* Magical spell sounds */
    AUDIO_SOURCE_NATURE,    /* Thunder, wind, water */
    AUDIO_SOURCE_CRAFT,     /* Hammering, chopping, etc. */
    AUDIO_SOURCE_MOVEMENT   /* Footsteps, hoofbeats, etc. */
} audio_source_type_t;

/* Audio frequency bands - different sounds travel differently */
typedef enum {
    AUDIO_FREQ_LOW,         /* Deep sounds (thunder, drums) */
    AUDIO_FREQ_MID,         /* Normal speech, most sounds */
    AUDIO_FREQ_HIGH         /* High pitched sounds (whistles, screams) */
} audio_frequency_t;

/* Audio system external interface */
extern struct spatial_system audio_system;

/* Audio system functions */
int spatial_audio_init(void);
int spatial_audio_test_thunder(int thunder_x, int thunder_y, const char *thunder_desc);
int spatial_audio_test_shout(int source_x, int source_y, const char *shout_message);

#endif /* _SPATIAL_AUDIO_H_ */
