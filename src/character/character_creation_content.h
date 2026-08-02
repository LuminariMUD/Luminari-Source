#ifndef CHARACTER_CREATION_CONTENT_H
#define CHARACTER_CREATION_CONTENT_H

#include <stddef.h>

#define CHARACTER_CREATION_HOMELAND_CANON_VERSION "homelands-1.0.0"
#define CHARACTER_CREATION_LANGUAGE_CANON_VERSION "homeland-languages-1.0.0"
#define CHARACTER_CREATION_COMPASS_CANON_VERSION "character-compass-1.0.0"

enum character_creation_inspiration_kind
{
  CHARACTER_CREATION_INSPIRATION_PERSONALITY = 0,
  CHARACTER_CREATION_INSPIRATION_IDEAL,
  CHARACTER_CREATION_INSPIRATION_BOND,
  CHARACTER_CREATION_INSPIRATION_FLAW,
  NUM_CHARACTER_CREATION_INSPIRATION_KINDS
};

struct character_creation_homeland
{
  int region;
  const char *content_id;
  const char *display_name;
  const char *place_kind;
  const char *geographic_parent;
  const char *political_sphere;
  int language;
  const char *summary;
  const char *description;
  const char *provenance;
};

struct character_creation_language
{
  int language;
  const char *content_id;
  const char *display_name;
  const char *help_summary;
};

struct character_creation_guidance
{
  const char *profile_id;
  const char *hub_summary;
  const char *screen_introduction;
  const char *editor_prompt;
  const char *generator_shape;
};

struct character_creation_background
{
  int background;
  const char *content_id;
  const char *story_promise;
  const char *biography;
  const char *seeds[NUM_CHARACTER_CREATION_INSPIRATION_KINDS][2];
};

const struct character_creation_homeland *character_creation_homeland_for_region(int region);
const struct character_creation_language *character_creation_language_for_index(int language);
const struct character_creation_guidance *
character_creation_guidance_for_profile(const char *profile_id);
const struct character_creation_background *character_creation_background_for_value(int background);
const char *character_creation_inspiration_seed(int background,
                                                enum character_creation_inspiration_kind kind,
                                                int index);

const char *character_creation_hometown_summary(int hometown);
const char *character_creation_hometown_description(int hometown);
const char *character_creation_content_provenance(void);

#endif /* CHARACTER_CREATION_CONTENT_H */
