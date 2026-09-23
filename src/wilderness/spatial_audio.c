/*************************************************************************
*   File: spatial_audio.c                              Part of LuminariMUD *
*  Usage: Audio stimulus strategy implementation                           *
*  Author: Luminari Development Team                                       *
*                                                                          *
*  All rights reserved.  See license for complete information.            *
*                                                                          *
*  LuminariMUD is based on CircleMUD, Copyright (C) 1993, 94 by the       *
*  Department of Computer Science at the Johns Hopkins University         *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "core/sysdep.h"
#include <math.h>

#include "core/structs.h"
#include "core/utils.h"
#include "core/comm.h"
#include "wilderness.h"
#include "spatial_core.h"
#include "spatial_audio.h"

/* Forward declarations for strategy functions */
static int audio_calculate_intensity(struct spatial_context *ctx);
static int audio_generate_message(struct spatial_context *ctx, char *output, size_t max_len);
static int audio_apply_effects(struct spatial_context *ctx);
static bool audio_should_process_observer(struct spatial_context *ctx);

static int acoustic_calculate_obstruction(struct spatial_context *ctx, double *obstruction_factor);
static int acoustic_get_obstacles(struct spatial_context *ctx, struct obstacle_list *obstacles);
static bool acoustic_can_transmit(int terrain_type, int stimulus_type);

static int weather_terrain_audio_apply_modifiers(struct spatial_context *ctx, double *range_mod,
                                                 double *clarity_mod);
static int weather_terrain_audio_calculate_interference(struct spatial_context *ctx,
                                                        double *interference);
static int weather_terrain_audio_modify_message(struct spatial_context *ctx, char *message,
                                                size_t max_len);

/* Helper functions for existing audio calculations */
static double calculate_distance_3d(int x1, int y1, int z1, int x2, int y2, int z2);
static double calculate_terrain_modifier(int listener_x, int listener_y, int source_x,
                                         int source_y);
static double calculate_elevation_effect(int listener_x, int listener_y, int source_x,
                                         int source_y);
static double get_sector_audio_modifier(int sector_type);
static bool has_line_of_sound(int listener_x, int listener_y, int source_x, int source_y);
/* static const char *get_direction_string(int dx, int dy); -- Currently unused */

/* Terrain audio constants (from existing system) */
#define TERRAIN_FOREST_DAMPING 0.7
#define TERRAIN_MOUNTAIN_ECHO 1.3
#define TERRAIN_WATER_CARRY 1.1
#define TERRAIN_PLAINS_NORMAL 1.0
#define TERRAIN_UNDERGROUND_MUFFLED 0.5

/* AUDIO STIMULUS STRATEGY */
struct stimulus_strategy audio_stimulus_strategy = {
    .name = "Audio",
    .stimulus_type = STIMULUS_AUDIO,
    .base_range = AUDIO_BASE_RANGE,
    .calculate_intensity = audio_calculate_intensity,
    .generate_base_message = audio_generate_message,
    .apply_stimulus_effects = audio_apply_effects,
    .should_process_observer = audio_should_process_observer,
    .enabled = TRUE,
    .usage_count = 0,
    .performance_factor = 1.0};

/* ACOUSTIC LINE OF SIGHT STRATEGY */
struct los_strategy acoustic_los_strategy = {.name = "Acoustic",
                                             .supported_stimulus_types = (1 << STIMULUS_AUDIO),
                                             .calculate_obstruction =
                                                 acoustic_calculate_obstruction,
                                             .get_blocking_elements = acoustic_get_obstacles,
                                             .can_transmit_through = acoustic_can_transmit,
                                             .enabled = TRUE,
                                             .use_caching = TRUE,
                                             .cache_hits = 0,
                                             .cache_misses = 0};

/* WEATHER/TERRAIN AUDIO MODIFIER STRATEGY */
struct modifier_strategy weather_terrain_audio_modifier_strategy = {
    .name = "Weather_Terrain_Audio",
    .applicable_stimulus_types = (1 << STIMULUS_AUDIO),
    .apply_environmental_modifiers = weather_terrain_audio_apply_modifiers,
    .calculate_interference = weather_terrain_audio_calculate_interference,
    .modify_message = weather_terrain_audio_modify_message,
    .enabled = TRUE,
    .modifier_strength = 1.0};

/* AUDIO SYSTEM CONFIGURATION */
struct spatial_system audio_system = {.system_name = "Audio",
                                      .system_id = 2,
                                      .stimulus = &audio_stimulus_strategy,
                                      .line_of_sight = &acoustic_los_strategy,
                                      .modifiers = &weather_terrain_audio_modifier_strategy,
                                      .enabled = TRUE,
                                      .global_range_multiplier = 1.0,
                                      .global_intensity_multiplier = 1.0,
                                      .total_processed = 0,
                                      .successful_transmissions = 0,
                                      .avg_processing_time_ms = 0.0};

/*
 * AUDIO STIMULUS STRATEGY IMPLEMENTATION
 */

/*
 * Calculate audio intensity based on distance and frequency
 */
static int audio_calculate_intensity(struct spatial_context *ctx)
{
  double distance_factor;

  if (!ctx)
  {
    return SPATIAL_ERROR_INVALID_PARAM;
  }

  /* Use existing 3D distance calculation with elevation weighting */
  ctx->distance = calculate_distance_3d(ctx->source_x, ctx->source_y, ctx->source_z,
                                        ctx->observer_x, ctx->observer_y, ctx->observer_z);

  /* Audio intensity calculation - different from visual */
  if (ctx->distance <= 0.0)
  {
    distance_factor = 1.0; /* Same location */
  }
  else
  {
    /* Audio follows inverse square law but with frequency-dependent modifications */
    double base_range = audio_stimulus_strategy.base_range;

    /* Different frequencies travel differently */
    double frequency_modifier = 1.0;
    if (ctx->audio_frequency == AUDIO_FREQ_LOW)
    {
      frequency_modifier = 1.3;         /* Low frequencies travel further */
      base_range = AUDIO_THUNDER_RANGE; /* Thunder has extended range */
    }
    else if (ctx->audio_frequency == AUDIO_FREQ_HIGH)
    {
      frequency_modifier = 0.6; /* High frequencies attenuate faster */
    }

    /* Much more aggressive audio dropoff for realistic behavior */
    /* Audio should drop off much faster than visual */
    double effective_range = base_range * frequency_modifier;

    /* Exponential decay with steep initial dropoff */
    distance_factor = 1.0 / (1.0 + (ctx->distance * ctx->distance / (effective_range * 0.01)));

    /* Additional linear attenuation for very close sounds to make distance matter more */
    if (ctx->distance > 5.0)
    {
      distance_factor *= (1.0 / (1.0 + (ctx->distance / 50.0)));
    }

    /* Apply frequency-specific attenuation */
    if (ctx->audio_frequency == AUDIO_FREQ_HIGH)
    {
      /* High frequencies drop off much faster with distance */
      distance_factor *= (1.0 / (1.0 + (ctx->distance / 30.0)));
    }
    else if (ctx->audio_frequency == AUDIO_FREQ_LOW)
    {
      /* Low frequencies still maintain some intensity but not too much */
      distance_factor = sqrt(distance_factor); /* Less severe dropoff */
    }
  }

  ctx->distance_attenuation = distance_factor;

  spatial_log("SPATIAL: Audio intensity calculated: %.6f at distance %.2f (freq: %d)",
              ctx->base_intensity * ctx->distance_attenuation, ctx->distance, ctx->audio_frequency);
  spatial_log("SPATIAL: Audio distance calculation: distance=%.2f, factor=%.6f", ctx->distance,
              distance_factor);

  return SPATIAL_SUCCESS;
}

/*
 * Generate audio message based on clarity and distance
 */
static int audio_generate_message(struct spatial_context *ctx, char *output, size_t max_len)
{
  audio_message_type_t msg_type;
  double clarity = ctx->final_intensity;

  spatial_log("SPATIAL: audio_generate_message called - ctx=%p, output=%p, source_desc=%p", ctx,
              output, ctx ? ctx->source_description : NULL);
  spatial_log("SPATIAL: Audio final intensity: %.6f for message generation", clarity);

  if (!ctx || !output || !ctx->source_description)
  {
    spatial_log(
        "SPATIAL: audio_generate_message validation failed - ctx=%p, output=%p, source_desc=%p",
        ctx, output, ctx ? ctx->source_description : NULL);
    return SPATIAL_ERROR_INVALID_PARAM;
  }

  /* Determine message type based on final intensity - audio thresholds */
  if (clarity >= 0.8)
  {
    msg_type = AUDIO_MSG_CLEAR;
  }
  else if (clarity >= 0.5)
  {
    msg_type = AUDIO_MSG_DISTANT;
  }
  else if (clarity >= 0.3)
  {
    msg_type = AUDIO_MSG_MUFFLED;
  }
  else if (clarity >= 0.15)
  {
    msg_type = AUDIO_MSG_ECHO;
  }
  else if (clarity >= 0.05)
  {
    msg_type = AUDIO_MSG_FAINT;
  }
  else
  {
    msg_type = AUDIO_MSG_RUMBLE;
  }

  /* Generate message based on type and add directional information */
  const char *direction_str = spatial_direction_to_string(ctx->direction, ctx->distance);

  switch (msg_type)
  {
  /* NOLINTNEXTLINE(bugprone-branch-clone) -- DISTANT may want its own wording; not guessed */
  case AUDIO_MSG_CLEAR:
    if (ctx->direction == SPATIAL_DIR_HERE)
    {
      snprintf(output, max_len, "You hear %s.", ctx->source_description);
    }
    else
    {
      snprintf(output, max_len, "You hear %s from %s.", ctx->source_description, direction_str);
    }
    break;

  case AUDIO_MSG_DISTANT:
    if (ctx->direction == SPATIAL_DIR_HERE)
    {
      snprintf(output, max_len, "You hear %s.", ctx->source_description);
    }
    else
    {
      snprintf(output, max_len, "You hear %s from %s.", ctx->source_description, direction_str);
    }
    break;

  case AUDIO_MSG_MUFFLED:
    if (ctx->direction == SPATIAL_DIR_HERE)
    {
      snprintf(output, max_len, "You hear the muffled sound of %s.", ctx->source_description);
    }
    else
    {
      snprintf(output, max_len, "You hear the muffled sound of %s from %s.",
               ctx->source_description, direction_str);
    }
    break;

  case AUDIO_MSG_ECHO:
    if (ctx->direction == SPATIAL_DIR_HERE)
    {
      snprintf(output, max_len, "An echo of %s reaches you.", ctx->source_description);
    }
    else
    {
      snprintf(output, max_len, "An echo of %s reaches you from %s.", ctx->source_description,
               direction_str);
    }
    break;

  case AUDIO_MSG_FAINT:
    if (ctx->direction == SPATIAL_DIR_HERE)
    {
      snprintf(output, max_len, "You faintly hear %s.", ctx->source_description);
    }
    else
    {
      snprintf(output, max_len, "You faintly hear %s from %s.", ctx->source_description,
               direction_str);
    }
    break;

  case AUDIO_MSG_RUMBLE:
    if (ctx->direction == SPATIAL_DIR_HERE)
    {
      snprintf(output, max_len, "You hear a low rumble nearby.");
    }
    else
    {
      snprintf(output, max_len, "You hear a low rumble from %s.", direction_str);
    }
    break;
  }

  spatial_log("SPATIAL: audio_generate_message returning SUCCESS");
  return SPATIAL_SUCCESS;
}

/*
 * Apply audio-specific effects
 */
static int audio_apply_effects(struct spatial_context *ctx)
{
  if (!ctx)
  {
    return SPATIAL_ERROR_INVALID_PARAM;
  }

  /* Future: Could add audio-specific effects like:
     * - Doppler effect for moving sources
     * - Reverb based on terrain
     * - Frequency filtering through obstacles
     */

  return SPATIAL_SUCCESS;
}

/*
 * Check if observer should process audio stimuli
 */
static bool audio_should_process_observer(struct spatial_context *ctx)
{
  if (!ctx || !ctx->observer)
  {
    return FALSE;
  }

  /* Skip NPCs unless specifically flagged */
  if (IS_NPC(ctx->observer))
  {
    return FALSE;
  }

  /* Check if player is conscious and can hear */
  if (GET_POS(ctx->observer) <= POS_STUNNED)
  {
    return FALSE;
  }

  /* Future: Check for deafness, magical silence, etc. */

  return TRUE;
}

/*
 * ACOUSTIC LINE OF SIGHT STRATEGY IMPLEMENTATION
 */

/*
 * Calculate acoustic obstruction - sound can bend around obstacles
 */
static int acoustic_calculate_obstruction(struct spatial_context *ctx, double *obstruction_factor)
{
  double total_obstruction = 0.0;

  if (!ctx || !obstruction_factor)
  {
    return SPATIAL_ERROR_INVALID_PARAM;
  }

  *obstruction_factor = 0.0;

  /* Use existing line of sound calculation */
  if (!has_line_of_sound(ctx->observer_x, ctx->observer_y, ctx->source_x, ctx->source_y))
  {
    /* Sound is blocked but can still partially get through */
    total_obstruction = 0.6; /* Audio is less blocked than visual */
  }

  /* Audio can bend around obstacles better than light */
  *obstruction_factor = total_obstruction * 0.7; /* Reduce obstruction for audio */

  spatial_log("SPATIAL: Acoustic obstruction: %.6f from (%d,%d) to (%d,%d)", *obstruction_factor,
              ctx->source_x, ctx->source_y, ctx->observer_x, ctx->observer_y);

  return SPATIAL_SUCCESS;
}

/*
 * Get blocking elements along acoustic path
 */
static int acoustic_get_obstacles(struct spatial_context *ctx, struct obstacle_list *obstacles)
{
  if (!ctx || !obstacles)
  {
    return SPATIAL_ERROR_INVALID_PARAM;
  }

  /* Reset obstacle list */
  obstacles->count = 0;

  /* TODO: Implement detailed acoustic obstacle detection */

  return SPATIAL_SUCCESS;
}

/*
 * Check if sound can transmit through terrain type
 */
static bool acoustic_can_transmit(int terrain_type, int stimulus_type)
{
  /* Only process audio stimuli */
  if (stimulus_type != STIMULUS_AUDIO)
  {
    return FALSE;
  }

  /* Sound can transmit through most terrain types, unlike light */
  switch (terrain_type)
  {
  case SECT_MOUNTAIN:    /* Solid rock blocks sound */
  case SECT_UD_NOGROUND: /* Void/no medium */
    return FALSE;
  default:
    return TRUE; /* Most terrain allows sound transmission */
  }
}

/*
 * WEATHER/TERRAIN AUDIO MODIFIER STRATEGY IMPLEMENTATION
 */

/*
 * Apply weather and terrain modifiers to audio range and clarity
 */
static int weather_terrain_audio_apply_modifiers(struct spatial_context *ctx, double *range_mod,
                                                 double *clarity_mod)
{
  double terrain_modifier, elevation_effect;
  double weather_range_mod = 1.0;
  double weather_clarity_mod = 1.0;

  if (!ctx || !range_mod || !clarity_mod)
  {
    return SPATIAL_ERROR_INVALID_PARAM;
  }

  /* Calculate terrain effects using existing system */
  terrain_modifier =
      calculate_terrain_modifier(ctx->observer_x, ctx->observer_y, ctx->source_x, ctx->source_y);
  elevation_effect =
      calculate_elevation_effect(ctx->observer_x, ctx->observer_y, ctx->source_x, ctx->source_y);

  /* Weather effects on audio */
  switch (ctx->weather_conditions)
  {
  case 0: /* Clear */
    weather_range_mod = 1.0;
    weather_clarity_mod = 1.0;
    break;
  case 1:                    /* Cloudy */
    weather_range_mod = 1.1; /* Clouds can enhance sound */
    weather_clarity_mod = 0.95;
    break;
  case 2:                    /* Rainy */
    weather_range_mod = 0.7; /* Rain dampens sound */
    weather_clarity_mod = 0.6;
    break;
  case 3:                    /* Foggy */
    weather_range_mod = 0.8; /* Fog muffles sound */
    weather_clarity_mod = 0.7;
    break;
  case 4:                    /* Storm */
    weather_range_mod = 0.4; /* Storm overwhelms most sounds */
    weather_clarity_mod = 0.3;
    break;
  default:
    break;
  }

  /* Combine all modifiers */
  *range_mod = terrain_modifier * elevation_effect * weather_range_mod;
  *clarity_mod = weather_clarity_mod;

  spatial_log("SPATIAL: Audio modifiers - range: %.6f, clarity: %.6f, terrain: %.6f, elevation: "
              "%.6f, weather: %d",
              *range_mod, *clarity_mod, terrain_modifier, elevation_effect,
              ctx->weather_conditions);

  return SPATIAL_SUCCESS;
}

/*
 * Calculate environmental interference for audio
 */
static int weather_terrain_audio_calculate_interference(struct spatial_context *ctx,
                                                        double *interference)
{
  if (!ctx || !interference)
  {
    return SPATIAL_ERROR_INVALID_PARAM;
  }

  *interference = 0.0;

  /* Environmental noise interference */
  switch (ctx->weather_conditions)
  {
  case 2: /* Rain */
    *interference += 0.3;
    break;
  case 4: /* Storm */
    *interference += 0.8;
    break;
  default:
    break;
  }

  return SPATIAL_SUCCESS;
}

/*
 * Modify audio message based on environmental conditions
 */
static int weather_terrain_audio_modify_message(struct spatial_context *ctx, char *message,
                                                size_t max_len __attribute__((unused)))
{
  if (!ctx || !message)
  {
    return SPATIAL_ERROR_INVALID_PARAM;
  }

  /* Add environmental descriptors based on conditions */
  if (ctx->weather_conditions == 2)
  /* NOLINTNEXTLINE(bugprone-branch-clone) -- placeholders for per-weather wording */
  { /* Rain */
    /* Could add "through the rain" to the message */
  }
  else if (ctx->weather_conditions == 4)
  { /* Storm */
    /* Could add "barely audible over the storm" */
  }

  return SPATIAL_SUCCESS;
}

/*
 * HELPER FUNCTIONS (from existing spatial audio system)
 */

/*
 * Calculate 3D distance with elevation weighting
 */
static double calculate_distance_3d(int x1, int y1, int z1, int x2, int y2, int z2)
{
  /* Weight Z distance less than X/Y for wilderness */
  int dx = x2 - x1;
  int dy = y2 - y1;
  int dz = (z2 - z1) / 4; /* Elevation differences are less significant */

  return sqrt(dx * dx + dy * dy + dz * dz);
}

/*
 * Calculate terrain modifier for audio transmission
 */
static double calculate_terrain_modifier(int listener_x, int listener_y, int source_x, int source_y)
{
  int listener_sector = get_modified_sector_type(real_zone(WILD_ZONE_VNUM), listener_x, listener_y);
  int source_sector = get_modified_sector_type(real_zone(WILD_ZONE_VNUM), source_x, source_y);

  double listener_mod = get_sector_audio_modifier(listener_sector);
  double source_mod = get_sector_audio_modifier(source_sector);

  /* Average the modifiers */
  return (listener_mod + source_mod) / 2.0;
}

/*
 * Get audio modifier for sector type
 */
static double get_sector_audio_modifier(int sector_type)
{
  switch (sector_type)
  {
  case SECT_FOREST:
    return TERRAIN_FOREST_DAMPING;
  case SECT_MOUNTAIN:
  case SECT_HILLS:
    return TERRAIN_MOUNTAIN_ECHO;
  case SECT_WATER_SWIM:
  case SECT_WATER_NOSWIM:
  case SECT_OCEAN:
    return TERRAIN_WATER_CARRY;
  case SECT_FIELD:
    return TERRAIN_PLAINS_NORMAL;
  case SECT_UD_WILD:
  case SECT_UD_NOGROUND:
    return TERRAIN_UNDERGROUND_MUFFLED;
  default:
    return TERRAIN_PLAINS_NORMAL;
  }
}

/*
 * Calculate elevation effect on sound transmission
 */
static double calculate_elevation_effect(int listener_x, int listener_y, int source_x, int source_y)
{
  int listener_elev = get_modified_elevation(listener_x, listener_y);
  int source_elev = get_modified_elevation(source_x, source_y);

  int elevation_diff = abs(source_elev - listener_elev);

  /* Sound carries better downhill, worse uphill */
  if (source_elev > listener_elev)
  {
    /* Sound coming from higher elevation - carries better */
    return 1.0 + (elevation_diff * 0.01); /* 1% bonus per elevation unit */
  }
  else if (source_elev < listener_elev)
  {
    /* Sound coming from lower elevation - reduced */
    return 1.0 - (elevation_diff * 0.005); /* 0.5% penalty per elevation unit */
  }
  else
  {
    /* Same elevation */
    return 1.0;
  }
}

/*
 * Check if there's a line of sound between two points
 */
static bool has_line_of_sound(int listener_x, int listener_y, int source_x, int source_y)
{
  /* Simplified line of sound - audio can bend around obstacles better than light */
  int dx = abs(source_x - listener_x);
  int dy = abs(source_y - listener_y);

  /* Check for major terrain obstacles along the path */
  int steps = MAX(dx, dy);
  if (steps == 0)
    return TRUE;

  double step_x = (double)(source_x - listener_x) / steps;
  double step_y = (double)(source_y - listener_y) / steps;

  int i;
  for (i = 1; i < steps; i++)
  {
    int check_x = listener_x + (int)(i * step_x);
    int check_y = listener_y + (int)(i * step_y);

    int sector = get_modified_sector_type(real_zone(WILD_ZONE_VNUM), check_x, check_y);

    /* Only major obstacles block sound completely */
    if (sector == SECT_MOUNTAIN)
    {
      /* Check if we can go around the mountain */
      if (i > steps * 0.3 && i < steps * 0.7)
      {
        return FALSE; /* Mountain in the middle blocks sound */
      }
    }
  }

  return TRUE;
}

/*
 * Get direction string for audio messages
 * Currently unused but kept for future audio directional messages
 */
#if 0
static const char *get_direction_string(int dx, int dy) {
    if (abs(dx) <= 2 && abs(dy) <= 2) {
        return "nearby";
    }

    if (abs(dx) > abs(dy)) {
        if (dx > 0) {
            if (dy > 5) return "to the northeast";
            else if (dy < -5) return "to the southeast";
            else return "to the east";
        } else {
            if (dy > 5) return "to the northwest";
            else if (dy < -5) return "to the southwest";
            else return "to the west";
        }
    } else {
        if (dy > 0) {
            if (dx > 5) return "to the northeast";
            else if (dx < -5) return "to the northwest";
            else return "to the north";
        } else {
            if (dx > 5) return "to the southeast";
            else if (dx < -5) return "to the southwest";
            else return "to the south";
        }
    }
}
#endif

/*
 * PUBLIC INTERFACE FUNCTIONS
 */

/*
 * Initialize audio system
 */
int spatial_audio_init(void)
{
  spatial_log("Initializing audio spatial system...");

  /* Register the audio system */
  if (spatial_register_system(&audio_system) != SPATIAL_SUCCESS)
  {
    spatial_log("ERROR: Failed to register audio system");
    return SPATIAL_ERROR_MEMORY;
  }

  spatial_log("Audio spatial system initialized successfully");
  return SPATIAL_SUCCESS;
}

/* Deliver one event-driven sound to eligible active wilderness players. */
int spatial_audio_emit(int source_x, int source_y, int source_z, const char *sound_desc,
                       double intensity, int frequency, int range)
{
  struct spatial_context *ctx;
  struct char_data *ch;
  struct descriptor_data *descriptor;
  int processed_count = 0;

  if (!sound_desc)
  {
    return SPATIAL_ERROR_INVALID_PARAM;
  }

  spatial_log("Emitting sound at (%d, %d, %d): %s", source_x, source_y, source_z, sound_desc);

  /* Create properly initialized context */
  ctx = spatial_create_context();
  if (!ctx)
  {
    spatial_log("ERROR: Failed to create spatial context");
    return SPATIAL_ERROR_MEMORY;
  }

  /* Set source information */
  ctx->source_x = source_x;
  ctx->source_y = source_y;
  ctx->source_z = source_z;
  ctx->source_description = sound_desc; /* Borrowed for synchronous delivery. */
  ctx->base_intensity = intensity;
  ctx->audio_frequency = frequency;

  /* Set effective range */
  ctx->effective_range = range;

  /* Only connected observers can receive a sound; do not traverse the NPC population. */
  for (descriptor = descriptor_list; descriptor; descriptor = descriptor->next)
  {
    ch = descriptor->character;
    if (ch == NULL || IS_NPC(ch) || IN_ROOM(ch) == NOWHERE || IN_ROOM(ch) > top_of_world)
      continue;
    if (!ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
      continue;

    /* Check if player is within audio range using horizontal distance */
    double horizontal_distance = sqrt((X_LOC(ch) - source_x) * (X_LOC(ch) - source_x) +
                                      (Y_LOC(ch) - source_y) * (Y_LOC(ch) - source_y));
    if (horizontal_distance > range)
    {
      spatial_log("Player %s too far horizontally (%.2f > %d)", GET_NAME(ch), horizontal_distance,
                  range);
      continue;
    }

    /* Set observer information */
    ctx->observer = ch;
    ctx->observer_x = X_LOC(ch);
    ctx->observer_y = Y_LOC(ch);
    ctx->observer_z = get_modified_elevation(X_LOC(ch), Y_LOC(ch));
    ctx->active_system = &audio_system;

    /* Use horizontal distance for audio intensity calculations */
    ctx->distance = horizontal_distance;

    /* Update direction calculation for this observer */
    spatial_update_direction(ctx);

    spatial_log("Testing audio for player %s at (%d, %d, %d) distance: %.2f", GET_NAME(ch),
                ctx->observer_x, ctx->observer_y, ctx->observer_z, horizontal_distance);

    /* Process audio stimulus */
    int result = spatial_process_stimulus(ctx, &audio_system);
    spatial_log("Spatial process result for %s: %d", GET_NAME(ch), result);

    if (result == SPATIAL_SUCCESS)
    {
      send_to_char(ch, "\r\n%s\r\n", ctx->processed_message);
      processed_count++;
      spatial_log("Sound effect delivered to %s", GET_NAME(ch));
    }
    else
    {
      spatial_log("Sound processing failed for %s with error %d", GET_NAME(ch), result);
    }
  }

  /* Cleanup */
  spatial_free_context(ctx);

  spatial_log("Sound event processed for %d players", processed_count);
  return SPATIAL_SUCCESS;
}
