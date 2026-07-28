/**************************************************************************
 *  File: onboarding.c                                                     *
 *  Usage: Structured account and character-creation state for web clients *
 *                                                                         *
 *  This module is a presentation adapter, not a second state machine. It  *
 *  reads the descriptor's current nanny() state and the same authoritative *
 *  race, class, alignment, unlock, and account data that the terminal      *
 *  prompts use, then publishes a bounded JSON document over MSDP.          *
 *                                                                         *
 *  It never mutates a character, never decides availability on its own,    *
 *  and never emits a password, password hash, or other secret.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "handler.h"
#include "act.h"
#include "spells.h"
#include "class.h"
#include "race.h"
#include "constants.h"
#include "protocol.h"
#include "account.h"
#include "onboarding.h"

/* Screen identifiers shared with the web client contract. */
#define SCREEN_UNSUPPORTED NULL

struct onboarding_screen_info
{
  int state;              /* CON_ constant */
  const char *screen;     /* contract screen id */
  const char *mode;       /* contract mode */
  const char *title;      /* short heading */
  const char *prompt;     /* one-line instruction */
  const char *input_kind; /* none | text | password | choice | confirm */
  bool sensitive;         /* password entry */
};

/* Every state this adapter presents. Any other state falls back to the
 * classic terminal, which keeps working unchanged. */
static const struct onboarding_screen_info onboarding_screens[] = {
    {CON_ACCOUNT_NAME, "account-name", "account", "Welcome to Luminari",
     "Enter your account name, or a new one to create an account.", "text", FALSE},
    {CON_ACCOUNT_NAME_CONFIRM, "account-name-confirm", "account", "Confirm your account name",
     "Is the account name you entered spelled correctly?", "confirm", FALSE},
    {CON_PASSWORD, "account-password", "account", "Account password",
     "Enter the password for this account.", "password", TRUE},
    {CON_NEWPASSWD, "account-new-password", "account", "Choose a password",
     "Choose a password for your new account.", "password", TRUE},
    {CON_CNFPASSWD, "account-confirm-password", "account", "Confirm your password",
     "Enter the same password once more.", "password", TRUE},
    {CON_ACCOUNT_MENU, "account-menu", "account", "Your characters",
     "Choose a character to play, or create a new one.", "choice", FALSE},
    {CON_ACCOUNT_ADD, "account-link", "account", "Link an existing character",
     "Enter the name of the character you want to add to this account.", "text", FALSE},
    {CON_ACCOUNT_ADD_PWD, "account-link-password", "account", "Prove ownership",
     "Enter that character's own password.", "password", TRUE},
    {CON_GET_NAME, "name", "character-creation", "Name your character",
     "Enter the name your character will be known by.", "text", FALSE},
    {CON_NAME_CNFRM, "name-confirm", "character-creation", "Confirm the name",
     "Did you spell that name the way you meant to?", "confirm", FALSE},
    {CON_QSEX, "sex", "character-creation", "Choose an identity",
     "Select the sex your character is recorded as.", "choice", FALSE},
    {CON_QRACE, "race", "character-creation", "Choose your ancestry",
     "Select an ancestry to continue.", "choice", FALSE},
    {CON_QRACE_HELP, "race-detail", "character-creation", "Keep this ancestry?",
     "Confirm this ancestry, or go back and choose another.", "confirm", FALSE},
    {CON_QCLASS, "class", "character-creation", "Choose your path", "Select a class to continue.",
     "choice", FALSE},
    {CON_QCLASS_HELP, "class-detail", "character-creation", "Keep this path?",
     "Confirm this class, or go back and choose another.", "confirm", FALSE},
    {CON_CONFIRM_PREMADE, "build", "character-creation", "Guided or custom build",
     "Choose how your character's build is decided.", "choice", FALSE},
    {CON_QALIGN, "alignment", "character-creation", "Choose an alignment",
     "Only alignments valid for your ancestry and path are shown.", "choice", FALSE},
    {CON_SETPREFS, "preferences", "character-creation", "Recommended settings",
     "Apply the recommended settings bundle, or keep the defaults.", "choice", FALSE},
    {CON_CHAR_RP_DECIDE, "roleplay-decision", "character-creation", "Role-play details",
     "Decide whether to fill in role-play details now.", "choice", FALSE},
    {CON_RMOTD, "motd", "character-menu", "Message of the day",
     "Read the message of the day, then continue.", "confirm", FALSE},
    {CON_MENU, "character-menu", "character-menu", "Character menu",
     "Choose what to do with this character.", "choice", FALSE},
};

/* Stable media keys. Deliberately a table rather than a transformation of the
 * display name: internal race and class tokens are not all well formed, and
 * the client's art must not break when a display string is corrected. */
static const char *const race_media_keys[NUM_RACES] = {
    "race/human",              /* RACE_HUMAN */
    "race/moon-elf",           /* RACE_ELF */
    "race/mountain-dwarf",     /* RACE_DWARF */
    "race/half-troll",         /* RACE_H_TROLL */
    "race/crystal-dwarf",      /* RACE_CRYSTAL_DWARF */
    "race/lightfoot-halfling", /* RACE_HALFLING */
    "race/half-elf",           /* RACE_H_ELF */
    "race/half-orc",           /* RACE_H_ORC */
    "race/rock-gnome",         /* RACE_GNOME */
    "race/trelux",             /* RACE_TRELUX */
    "race/arcana-golem",       /* RACE_ARCANA_GOLEM */
    "race/drow",               /* RACE_DROW */
    "race/duergar",            /* RACE_DUERGAR */
    "race/high-elf",           /* RACE_HIGH_ELF */
    "race/wild-elf",           /* RACE_WOOD_ELF */
    "race/half-drow",          /* RACE_HALF_DROW */
    "race/dragonborn",         /* RACE_DRAGONBORN */
    "race/tiefling",           /* RACE_TIEFLING */
    "race/stout-halfling",     /* RACE_STOUT_HALFLING */
    "race/forest-gnome",       /* RACE_FOREST_GNOME */
    "race/gold-dwarf",         /* RACE_GOLD_DWARF */
    "race/aasimar",            /* RACE_AASIMAR */
    "race/tabaxi",             /* RACE_TABAXI */
    "race/goliath",            /* RACE_GOLIATH */
    "race/shade",              /* RACE_SHADE */
    "race/fae",                /* RACE_FAE */
    "race/goblin",             /* RACE_GOBLIN */
    "race/hobgoblin",          /* RACE_HOBGOBLIN */
};

static const char *const class_media_keys[NUM_CLASSES] = {
    "class/wizard",     /* CLASS_WIZARD */
    "class/cleric",     /* CLASS_CLERIC */
    "class/rogue",      /* CLASS_ROGUE */
    "class/warrior",    /* CLASS_WARRIOR */
    "class/monk",       /* CLASS_MONK */
    "class/druid",      /* CLASS_DRUID */
    "class/berserker",  /* CLASS_BERSERKER */
    "class/sorcerer",   /* CLASS_SORCERER */
    "class/paladin",    /* CLASS_PALADIN */
    "class/ranger",     /* CLASS_RANGER */
    "class/bard",       /* CLASS_BARD */
    NULL,               /* CLASS_WEAPON_MASTER (prestige) */
    NULL,               /* CLASS_ARCANE_ARCHER (prestige) */
    NULL,               /* CLASS_STALWART_DEFENDER (prestige) */
    NULL,               /* CLASS_SHIFTER (prestige) */
    NULL,               /* CLASS_DUELIST (prestige) */
    NULL,               /* CLASS_MYSTIC_THEURGE (prestige) */
    "class/alchemist",  /* CLASS_ALCHEMIST */
    NULL,               /* CLASS_ARCANE_SHADOW (prestige) */
    NULL,               /* CLASS_SACRED_FIST (prestige) */
    NULL,               /* CLASS_ELDRITCH_KNIGHT (prestige) */
    "class/psionicist", /* CLASS_PSIONICIST */
    NULL,               /* CLASS_SPELLSWORD (prestige) */
    NULL,               /* CLASS_SHADOW_DANCER (prestige) */
    "class/blackguard", /* CLASS_BLACKGUARD */
    NULL,               /* CLASS_ASSASSIN (prestige) */
    "class/inquisitor", /* CLASS_INQUISITOR */
    "class/summoner",   /* CLASS_SUMMONER */
    "class/warlock",    /* CLASS_WARLOCK */
    NULL,               /* CLASS_NECROMANCER (prestige) */
    NULL,               /* CLASS_KNIGHT_OF_SOLAMNIA */
    NULL,               /* CLASS_KNIGHT_OF_THE_THORN */
    NULL,               /* CLASS_KNIGHT_OF_THE_SKULL */
    NULL,               /* CLASS_KNIGHT_OF_THE_LILY */
    NULL,               /* CLASS_DRAGONRIDER */
    "class/artificer",  /* CLASS_ARTIFICER */
    NULL,               /* CLASS_PLACEHOLDER_1 */
    NULL,               /* CLASS_PLACEHOLDER_2 */
};

static const char *const alignment_media_keys[NUM_ALIGNMENTS] = {
    "alignment/lawful-good",    "alignment/neutral-good", "alignment/chaotic-good",
    "alignment/lawful-neutral", "alignment/true-neutral", "alignment/chaotic-neutral",
    "alignment/lawful-evil",    "alignment/neutral-evil", "alignment/chaotic-evil",
};

/* --------------------------------------------------------------------- */
/* Bounded JSON writer                                                    */
/* --------------------------------------------------------------------- */

struct json_writer
{
  char *buf;
  size_t size;
  size_t len;
  bool overflow;
};

static void json_raw(struct json_writer *w, const char *text)
{
  size_t text_len = 0;

  if (w->overflow)
    return;

  text_len = strlen(text);
  if (w->len + text_len + 1 >= w->size)
  {
    w->overflow = TRUE;
    return;
  }

  memcpy(w->buf + w->len, text, text_len);
  w->len += text_len;
  w->buf[w->len] = '\0';
}

static void json_number(struct json_writer *w, int value)
{
  char scratch[32];

  snprintf(scratch, sizeof(scratch), "%d", value);
  json_raw(w, scratch);
}

/* Write a JSON string, stripping MUD colour codes, ANSI escapes, and control
 * bytes so the payload cannot forge layout or terminal output. */
/*
 * Long catalog text is cut to fit the payload budget. Cutting mid-word reads
 * as corruption, so back up to the last word boundary and mark the elision.
 */
static void json_string_truncated(struct json_writer *w, const char *value, size_t max_chars);

static void json_string(struct json_writer *w, const char *value, size_t max_chars)
{
  size_t written = 0;
  const char *p = NULL;

  json_raw(w, "\"");

  if (value != NULL)
  {
    for (p = value; *p && written < max_chars && !w->overflow; p++)
    {
      unsigned char c = (unsigned char)*p;

      /* Codebase colour codes are a tab followed by one selector byte. */
      if (c == '\t')
      {
        if (*(p + 1))
          p++;
        continue;
      }

      /* ANSI escape sequences. */
      if (c == 0x1b)
      {
        while (*(p + 1) && *(p + 1) != 'm')
          p++;
        if (*(p + 1))
          p++;
        continue;
      }

      if (c == '\r')
        continue;

      if (c == '\n')
      {
        json_raw(w, " ");
        written++;
        continue;
      }

      if (c < 0x20 || c == 0x7f)
        continue;

      if (c == '"' || c == '\\')
      {
        char escaped[3];

        escaped[0] = '\\';
        escaped[1] = (char)c;
        escaped[2] = '\0';
        json_raw(w, escaped);
        written++;
        continue;
      }

      {
        char single[2];

        single[0] = (char)c;
        single[1] = '\0';
        json_raw(w, single);
        written++;
      }
    }
  }

  json_raw(w, "\"");
}

/*
 * Copy at most max_chars display characters from value, stopping at the last
 * word boundary and appending an ellipsis when text was dropped. Colour codes
 * and control bytes are removed by json_string(), so the count here is a safe
 * upper bound rather than an exact glyph count.
 */
static void json_string_truncated(struct json_writer *w, const char *value, size_t max_chars)
{
  char scratch[1024];
  size_t limit = 0;
  size_t index = 0;
  size_t last_space = 0;

  if (value == NULL)
  {
    json_string(w, NULL, max_chars);
    return;
  }

  limit = max_chars;
  if (limit > sizeof(scratch) - 4)
    limit = sizeof(scratch) - 4;

  if (strlen(value) <= limit)
  {
    json_string(w, value, max_chars);
    return;
  }

  for (index = 0; index < limit; index++)
  {
    scratch[index] = value[index];
    if (value[index] == ' ')
      last_space = index;
  }

  /* Only honour the word boundary if it does not throw away most of the text. */
  if (last_space > limit / 2)
    index = last_space;

  scratch[index] = '\0';
  strcat(scratch, "...");

  json_string(w, scratch, max_chars + 4);
}

static void json_field_string(struct json_writer *w, const char *name, const char *value,
                              size_t max_chars)
{
  json_raw(w, "\"");
  json_raw(w, name);
  json_raw(w, "\":");
  json_string(w, value, max_chars);
}

static void json_field_string_truncated(struct json_writer *w, const char *name, const char *value,
                                        size_t max_chars)
{
  json_raw(w, "\"");
  json_raw(w, name);
  json_raw(w, "\":");
  json_string_truncated(w, value, max_chars);
}

static void json_field_number(struct json_writer *w, const char *name, int value)
{
  json_raw(w, "\"");
  json_raw(w, name);
  json_raw(w, "\":");
  json_number(w, value);
}

static void json_field_bool(struct json_writer *w, const char *name, bool value)
{
  json_raw(w, "\"");
  json_raw(w, name);
  json_raw(w, "\":");
  json_raw(w, value ? "true" : "false");
}

/* --------------------------------------------------------------------- */
/* Capability handling                                                    */
/* --------------------------------------------------------------------- */

void web_onboarding_set_capability(struct descriptor_data *d, const char *value)
{
  int version = 0;

  if (d == NULL || value == NULL)
    return;

  version = atoi(value);
  if (version < 0 || version > 1000)
    version = 0;

  d->web_onboarding_version = version;
  d->web_onboarding_last_state = -1;
  d->web_onboarding_dirty = TRUE;
}

bool web_onboarding_enabled(struct descriptor_data *d)
{
  if (d == NULL || d->pProtocol == NULL || !d->pProtocol->bMSDP)
    return FALSE;

  return d->web_onboarding_version == WEB_ONBOARDING_PROTOCOL_VERSION;
}

void web_onboarding_mark_dirty(struct descriptor_data *d)
{
  if (d != NULL)
    d->web_onboarding_dirty = TRUE;
}

void web_onboarding_reset(struct descriptor_data *d)
{
  if (d == NULL)
    return;

  d->web_onboarding_version = 0;
  d->web_onboarding_revision = 0;
  d->web_onboarding_last_state = -1;
  d->web_onboarding_dirty = FALSE;
}

const char *web_onboarding_race_media_key(int race)
{
  if (race < 0 || race >= NUM_RACES || race_media_keys[race] == NULL)
    return "race/fallback";

  return race_media_keys[race];
}

const char *web_onboarding_class_media_key(int chclass)
{
  if (chclass < 0 || chclass >= NUM_CLASSES || class_media_keys[chclass] == NULL)
    return "class/fallback";

  return class_media_keys[chclass];
}

/* --------------------------------------------------------------------- */
/* Catalog builders                                                       */
/* --------------------------------------------------------------------- */

static const struct onboarding_screen_info *find_screen(int state)
{
  size_t i = 0;

  for (i = 0; i < sizeof(onboarding_screens) / sizeof(onboarding_screens[0]); i++)
  {
    if (onboarding_screens[i].state == state)
      return &onboarding_screens[i];
  }

  return NULL;
}

static void build_sex_choices(struct json_writer *w)
{
  json_raw(w, "{");
  json_field_string(w, "id", "male", 32);
  json_raw(w, ",");
  json_field_string(w, "label", "Male", 32);
  json_raw(w, ",");
  json_field_string(w, "wireValue", "m", 8);
  json_raw(w, ",");
  json_field_bool(w, "enabled", TRUE);
  json_raw(w, ",");
  json_field_string(w, "mediaKey", "identity/sex-male", 64);
  json_raw(w, "},{");
  json_field_string(w, "id", "female", 32);
  json_raw(w, ",");
  json_field_string(w, "label", "Female", 32);
  json_raw(w, ",");
  json_field_string(w, "wireValue", "f", 8);
  json_raw(w, ",");
  json_field_bool(w, "enabled", TRUE);
  json_raw(w, ",");
  json_field_string(w, "mediaKey", "identity/sex-female", 64);
  json_raw(w, "}");
}

/* Races the server would actually accept right now, using the same lock and
 * playable checks that nanny() uses. */
static void build_race_choices(struct json_writer *w, struct descriptor_data *d)
{
  int race = 0;
  bool first = TRUE;

  /* A descriptor can lose its character mid-creation (link loss, extraction)
   * while the state poll still runs, so never assume one is attached. */
  if (d->character == NULL)
    return;

  for (race = 0; race < NUM_RACES; race++)
  {
    if (!race_list[race].is_pc)
      continue;

    if (is_locked_race(race) && !has_unlocked_race(d->character, race))
      continue;

    if (!first)
      json_raw(w, ",");
    first = FALSE;

    json_raw(w, "{");
    json_field_string(w, "id", race_list[race].name, 48);
    json_raw(w, ",");
    json_field_string(w, "label", race_list[race].type, 48);
    json_raw(w, ",");
    json_field_string(w, "wireValue", race_list[race].type, 48);
    json_raw(w, ",");
    json_field_bool(w, "enabled", TRUE);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", web_onboarding_race_media_key(race), 64);
    json_raw(w, ",");
    json_field_string_truncated(w, "summary", race_list[race].descrip, 220);
    json_raw(w, ",\"facts\":[{");
    json_field_string(w, "label", "Size", 24);
    json_raw(w, ",");
    json_field_string(w, "value", sizes[(int)race_list[race].size], 24);
    json_raw(w, ",");
    json_field_string(w, "glyph", "size", 24);
    json_raw(w, "}");

    if (race_list[race].level_adjustment > 0)
    {
      char scratch[16];

      snprintf(scratch, sizeof(scratch), "+%d", race_list[race].level_adjustment);
      json_raw(w, ",{");
      json_field_string(w, "label", "Level adjustment", 32);
      json_raw(w, ",");
      json_field_string(w, "value", scratch, 16);
      json_raw(w, ",");
      json_field_string(w, "glyph", "level-adjustment", 32);
      json_raw(w, "}");
    }

    json_raw(w, "]}");

    if (w->overflow)
      return;
  }
}

/* Classes valid for the already-chosen race, using the same checks as
 * nanny()'s CON_QCLASS handler. */
static void build_class_choices(struct json_writer *w, struct descriptor_data *d)
{
  int chclass = 0;
  bool first = TRUE;

  if (d->character == NULL)
    return;

  for (chclass = 0; chclass < NUM_CLASSES; chclass++)
  {
    if (!CLSLIST_INGAME(chclass) || CLSLIST_PRESTIGE(chclass))
      continue;

    if (CLSLIST_LOCK(chclass) && !has_unlocked_class(d->character, chclass))
      continue;

    if (!valid_class_race_alignment(chclass, GET_REAL_RACE(d->character)))
      continue;

    if (!first)
      json_raw(w, ",");
    first = FALSE;

    json_raw(w, "{");
    json_field_string(w, "id", CLSLIST_NAME(chclass), 48);
    json_raw(w, ",");
    json_field_string(w, "label", CLSLIST_NAME(chclass), 48);
    json_raw(w, ",");
    json_field_string(w, "wireValue", CLSLIST_NAME(chclass), 48);
    json_raw(w, ",");
    json_field_bool(w, "enabled", TRUE);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", web_onboarding_class_media_key(chclass), 64);
    json_raw(w, ",");
    json_field_string_truncated(w, "summary", CLSLIST_DESCRIP(chclass), 220);
    json_raw(w, ",\"facts\":[{");
    json_field_string(w, "label", "Hit die", 24);
    json_raw(w, ",\"value\":\"d");
    json_number(w, CLSLIST_HPS(chclass));
    json_raw(w, "\",");
    json_field_string(w, "glyph", "hit-die", 24);
    json_raw(w, "},{");
    json_field_string(w, "label", "Primary attributes", 32);
    json_raw(w, ",");
    json_field_string(w, "value", CLSLIST_ATTRIBUTE(chclass), 64);
    json_raw(w, ",");
    json_field_string(w, "glyph", "primary-attributes", 32);
    json_raw(w, "}]}");

    if (w->overflow)
      return;
  }
}

/* Only alignments valid for both the chosen race and class. */
static void build_alignment_choices(struct json_writer *w, struct descriptor_data *d)
{
  int align = 0;
  bool first = TRUE;

  if (d->character == NULL)
    return;

  for (align = 0; align < NUM_ALIGNMENTS; align++)
  {
    char wire[8];

    if (!valid_align_by_class(align, GET_CLASS(d->character)))
      continue;

    if (!valid_align_by_race(align, GET_REAL_RACE(d->character)))
      continue;

    if (!first)
      json_raw(w, ",");
    first = FALSE;

    snprintf(wire, sizeof(wire), "%d", align);

    json_raw(w, "{");
    json_field_string(w, "id", alignment_media_keys[align], 48);
    json_raw(w, ",");
    json_field_string(w, "label", alignment_names_nocolor[align], 32);
    json_raw(w, ",");
    json_field_string(w, "wireValue", wire, 8);
    json_raw(w, ",");
    json_field_bool(w, "enabled", TRUE);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", alignment_media_keys[align], 48);
    json_raw(w, "}");
  }
}

static void build_simple_choice(struct json_writer *w, const char *id, const char *label,
                                const char *wire, const char *media_key, const char *summary)
{
  json_raw(w, "{");
  json_field_string(w, "id", id, 48);
  json_raw(w, ",");
  json_field_string(w, "label", label, 64);
  json_raw(w, ",");
  json_field_string(w, "wireValue", wire, 16);
  json_raw(w, ",");
  json_field_bool(w, "enabled", TRUE);
  json_raw(w, ",");
  json_field_string(w, "mediaKey", media_key, 64);
  json_raw(w, ",");
  json_field_string(w, "summary", summary, 220);
  json_raw(w, "}");
}

/* Account characters, loaded through the same path the terminal menu uses. */
static void build_account_characters(struct json_writer *w, struct descriptor_data *d)
{
  int slot = 0;
  bool first = TRUE;

  if (d->account == NULL)
    return;

  for (slot = 0; slot < MAX_CHARS_PER_ACCOUNT; slot++)
  {
    struct char_data *tch = NULL;
    char wire[8];
    char classes[MAX_INPUT_LENGTH];
    size_t classes_len = 0;
    int class_index = 0;
    int class_count = 0;

    if (d->account->character_names[slot] == NULL)
      continue;

    CREATE(tch, struct char_data, 1);
    clear_char(tch);
    CREATE(tch->player_specials, struct player_special_data, 1);
    new_mobile_data(tch);

    if (load_char(d->account->character_names[slot], tch) < 0)
    {
      free_char(tch);
      continue;
    }

    if (PLR_FLAGGED(tch, PLR_DELETED))
    {
      free_char(tch);
      continue;
    }

    classes[0] = '\0';
    for (class_index = 0; class_index < MAX_CLASSES; class_index++)
    {
      if (CLASS_LEVEL(tch, class_index))
      {
        if (class_count)
          classes_len += snprintf(classes + classes_len, sizeof(classes) - classes_len, "/");
        classes_len += snprintf(classes + classes_len, sizeof(classes) - classes_len, "%s",
                                CLSLIST_NAME(class_index));
        class_count++;
      }
    }

    snprintf(wire, sizeof(wire), "%d", slot + 1);

    if (!first)
      json_raw(w, ",");
    first = FALSE;

    json_raw(w, "{");
    json_field_string(w, "id", d->account->character_names[slot], 40);
    json_raw(w, ",");
    json_field_string(w, "name", d->account->character_names[slot], 40);
    json_raw(w, ",");
    json_field_string(w, "wireValue", wire, 8);
    json_raw(w, ",");
    json_field_number(w, "level", GET_LEVEL(tch));
    json_raw(w, ",");
    json_field_string(w, "race", race_list[GET_REAL_RACE(tch)].type, 40);
    json_raw(w, ",");
    json_field_string(w, "raceMediaKey", web_onboarding_race_media_key(GET_REAL_RACE(tch)), 64);
    json_raw(w, ",");
    json_field_string(w, "classSummary", classes, 120);
    json_raw(w, "}");

    free_char(tch);

    if (w->overflow)
      return;
  }
}

/* --------------------------------------------------------------------- */
/* State emission                                                         */
/* --------------------------------------------------------------------- */

/*
 * Which parts of the character the server has actually decided yet.
 *
 * The CON_ constants are not in flow order, so these must be explicit state
 * lists rather than numeric comparisons. Reporting a field early shows the
 * player a choice they have not made: GET_CLASS() defaults to 0, which would
 * otherwise display "Wizard" before the class step is even reached.
 */
static bool selection_has_race(int state)
{
  switch (state)
  {
  case CON_QRACE_HELP:
  case CON_QCLASS:
  case CON_QCLASS_HELP:
  case CON_CONFIRM_PREMADE:
  case CON_QALIGN:
  case CON_SETPREFS:
  case CON_CHAR_RP_DECIDE:
  case CON_RMOTD:
  case CON_MENU:
    return TRUE;
  default:
    return FALSE;
  }
}

static bool selection_has_class(int state)
{
  switch (state)
  {
  case CON_QCLASS_HELP:
  case CON_CONFIRM_PREMADE:
  case CON_QALIGN:
  case CON_SETPREFS:
  case CON_CHAR_RP_DECIDE:
  case CON_RMOTD:
  case CON_MENU:
    return TRUE;
  default:
    return FALSE;
  }
}

static bool selection_has_sex(int state)
{
  return state == CON_QRACE || selection_has_race(state);
}

/* Alignment is set as the alignment step is completed. */
static bool selection_has_alignment(int state)
{
  switch (state)
  {
  case CON_SETPREFS:
  case CON_CHAR_RP_DECIDE:
  case CON_RMOTD:
  case CON_MENU:
    return TRUE;
  default:
    return FALSE;
  }
}

static void build_selection(struct json_writer *w, struct descriptor_data *d)
{
  struct char_data *ch = d->character;
  int state = d->connected;
  bool first = TRUE;

  json_raw(w, "\"selection\":{");

  if (ch == NULL)
  {
    json_raw(w, "}");
    return;
  }

  if (GET_PC_NAME(ch) != NULL && *GET_PC_NAME(ch))
  {
    json_field_string(w, "name", GET_PC_NAME(ch), 40);
    first = FALSE;
  }

  if (selection_has_sex(state) && (GET_SEX(ch) == SEX_MALE || GET_SEX(ch) == SEX_FEMALE))
  {
    if (!first)
      json_raw(w, ",");
    first = FALSE;
    json_field_string(w, "sex", GET_SEX(ch) == SEX_MALE ? "Male" : "Female", 16);
  }

  if (selection_has_race(state) && GET_REAL_RACE(ch) >= 0 && GET_REAL_RACE(ch) < NUM_RACES)
  {
    if (!first)
      json_raw(w, ",");
    first = FALSE;
    json_field_string(w, "race", race_list[GET_REAL_RACE(ch)].type, 40);
    json_raw(w, ",");
    json_field_string(w, "raceMediaKey", web_onboarding_race_media_key(GET_REAL_RACE(ch)), 64);
  }

  if (selection_has_class(state) && GET_CLASS(ch) >= 0 && GET_CLASS(ch) < NUM_CLASSES)
  {
    if (!first)
      json_raw(w, ",");
    first = FALSE;
    json_field_string(w, "className", CLSLIST_MENU(GET_CLASS(ch)), 40);
    json_raw(w, ",");
    json_field_string(w, "classMediaKey", web_onboarding_class_media_key(GET_CLASS(ch)), 64);
  }

  if (selection_has_alignment(state) && GET_ALIGNMENT(ch) >= -1000 && GET_ALIGNMENT(ch) <= 1000)
  {
    if (!first)
      json_raw(w, ",");
    first = FALSE;
    json_field_string(w, "alignment", get_align_by_num(GET_ALIGNMENT(ch)), 32);
  }

  json_raw(w, "}");
}

/* The character is written to disk at the end of the alignment step, so any
 * state after it reports a confirmed save and any state before it reports an
 * unsaved draft. The UI must not celebrate before this says "saved". */
static const char *persistence_for_state(int state)
{
  switch (state)
  {
  case CON_GET_NAME:
  case CON_NAME_CNFRM:
  case CON_QSEX:
  case CON_QRACE:
  case CON_QRACE_HELP:
  case CON_QCLASS:
  case CON_QCLASS_HELP:
  case CON_CONFIRM_PREMADE:
    return "draft";
  case CON_QALIGN:
    return "pending";
  case CON_SETPREFS:
  case CON_CHAR_RP_DECIDE:
    return "saved";
  default:
    return "none";
  }
}

static void build_actions(struct json_writer *w, const struct onboarding_screen_info *screen)
{
  json_raw(w, "\"actions\":[");

  if (!strcmp(screen->input_kind, "confirm"))
    json_raw(w, "\"confirm\",\"reselect\"");
  else if (!strcmp(screen->input_kind, "choice"))
    json_raw(w, "\"select\",\"detail\"");
  else
    json_raw(w, "\"submit\"");

  if (screen->state == CON_ACCOUNT_MENU)
    json_raw(w, ",\"create-character\",\"link-character\",\"quit\"");

  json_raw(w, ",\"classic-terminal\"]");
}

/*
 * The confirm-or-reselect screens carry no choice list: the MUD is asking a
 * yes/no question about the selection it already holds. Emit that selection's
 * artwork and description so the client can present a real screen instead of
 * an empty catalog.
 */
static void build_selected_detail(struct json_writer *w, struct descriptor_data *d,
                                  const struct onboarding_screen_info *screen)
{
  struct char_data *ch = d->character;

  if (ch == NULL)
    return;

  if (screen->state == CON_QRACE_HELP && GET_REAL_RACE(ch) >= 0 && GET_REAL_RACE(ch) < NUM_RACES)
  {
    json_field_string(w, "mediaKey", web_onboarding_race_media_key(GET_REAL_RACE(ch)), 64);
    json_raw(w, ",");
    json_field_string_truncated(w, "help", race_list[GET_REAL_RACE(ch)].descrip, 900);
    json_raw(w, ",");
    return;
  }

  if (screen->state == CON_QCLASS_HELP && GET_CLASS(ch) >= 0 && GET_CLASS(ch) < NUM_CLASSES)
  {
    json_field_string(w, "mediaKey", web_onboarding_class_media_key(GET_CLASS(ch)), 64);
    json_raw(w, ",");
    json_field_string_truncated(w, "help", CLSLIST_DESCRIP(GET_CLASS(ch)), 900);
    json_raw(w, ",");
  }
}

static void build_choices(struct json_writer *w, struct descriptor_data *d,
                          const struct onboarding_screen_info *screen)
{
  json_raw(w, "\"choices\":[");

  switch (screen->state)
  {
  case CON_QSEX:
    build_sex_choices(w);
    break;
  case CON_QRACE:
    build_race_choices(w, d);
    break;
  case CON_QCLASS:
    build_class_choices(w, d);
    break;
  case CON_QALIGN:
    build_alignment_choices(w, d);
    break;
  case CON_CONFIRM_PREMADE:
    build_simple_choice(w, "premade", "Guided build", "y", "build/premade",
                        "The server picks a proven set of choices for you.");
    json_raw(w, ",");
    build_simple_choice(w, "custom", "Custom build", "n", "build/custom",
                        "You make each build decision yourself as you level.");
    break;
  case CON_SETPREFS:
    build_simple_choice(w, "recommended", "Use recommended settings", "y",
                        "preferences/recommended",
                        "Turns on the settings most new players find helpful.");
    json_raw(w, ",");
    build_simple_choice(w, "manual", "Keep the defaults", "n", "preferences/manual",
                        "Leave every setting as it is; you can change them in game.");
    break;
  case CON_CHAR_RP_DECIDE:
    build_simple_choice(w, "roleplayer", "Fill in role-play details", "1",
                        "roleplay/choice-roleplayer",
                        "Describe your character's background, goals, and personality.");
    json_raw(w, ",");
    build_simple_choice(w, "non-roleplayer", "Skip role-play details", "2",
                        "roleplay/choice-non-roleplayer", "Head straight into the world.");
    json_raw(w, ",");
    build_simple_choice(w, "later", "Decide later", "3", "roleplay/choice-later",
                        "Leave the profile bookmarked and return to it in game.");
    break;
  default:
    break;
  }

  json_raw(w, "]");
}

/* A valid, minimal document in play mode. The client treats this as "the
 * structured experience is finished here" and returns to the terminal. */
static void emit_cleared(struct descriptor_data *d)
{
  char payload[512];
  char flow_id[64];

  d->web_onboarding_revision++;
  snprintf(flow_id, sizeof(flow_id), "%d-%ld", d->desc_num, (long)d->login_time);
  snprintf(payload, sizeof(payload),
           "{\"version\":%d,\"flowId\":\"%s\",\"revision\":%d,\"mode\":\"play\","
           "\"screen\":\"handoff\",\"title\":\"In the world\",\"prompt\":\"\","
           "\"inputKind\":\"none\",\"sensitiveInput\":false,\"persistence\":\"none\","
           "\"choices\":[],\"characters\":[],\"selection\":{},\"actions\":[]}",
           WEB_ONBOARDING_PROTOCOL_VERSION, flow_id, d->web_onboarding_revision);

  MSDPSendPair(d, WEB_ONBOARDING_MSDP_VARIABLE, payload);
}

static bool build_state_payload(struct descriptor_data *d,
                                const struct onboarding_screen_info *screen, char *buf,
                                size_t buf_size)
{
  char flow_id[64];
  struct json_writer writer;

  writer.buf = buf;
  writer.size = buf_size;
  writer.len = 0;
  writer.overflow = FALSE;
  buf[0] = '\0';

  snprintf(flow_id, sizeof(flow_id), "%d-%ld", d->desc_num, (long)d->login_time);

  json_raw(&writer, "{");
  json_field_number(&writer, "version", WEB_ONBOARDING_PROTOCOL_VERSION);
  json_raw(&writer, ",");
  json_field_string(&writer, "flowId", flow_id, 63);
  json_raw(&writer, ",");
  json_field_number(&writer, "revision", d->web_onboarding_revision);
  json_raw(&writer, ",");
  json_field_string(&writer, "mode", screen->mode, 32);
  json_raw(&writer, ",");
  json_field_string(&writer, "screen", screen->screen, 32);
  json_raw(&writer, ",");
  json_field_string(&writer, "title", screen->title, 100);
  json_raw(&writer, ",");
  json_field_string(&writer, "prompt", screen->prompt, 200);
  json_raw(&writer, ",");
  json_field_string(&writer, "inputKind", screen->input_kind, 16);
  json_raw(&writer, ",");
  json_field_bool(&writer, "sensitiveInput", screen->sensitive);
  json_raw(&writer, ",");
  json_field_string(&writer, "persistence", persistence_for_state(screen->state), 16);
  json_raw(&writer, ",");
  build_selected_detail(&writer, d, screen);
  build_choices(&writer, d, screen);
  json_raw(&writer, ",\"characters\":[");
  if (screen->state == CON_ACCOUNT_MENU)
    build_account_characters(&writer, d);
  json_raw(&writer, "],");
  build_selection(&writer, d);
  json_raw(&writer, ",");
  build_actions(&writer, screen);
  json_raw(&writer, "}");

  if (writer.overflow)
  {
    /* Better to say nothing than to send a truncated document: the client
     * treats a malformed payload as a reason to fall back to the terminal. */
    log("SYSERR: web_onboarding: payload overflow for screen %s", screen->screen);
    buf[0] = '\0';
    return FALSE;
  }

  return TRUE;
}

bool web_onboarding_build_payload(struct descriptor_data *d, char *buf, size_t buf_size)
{
  const struct onboarding_screen_info *screen = NULL;

  if (d == NULL || buf == NULL || buf_size == 0)
    return FALSE;

  buf[0] = '\0';

  screen = find_screen(d->connected);
  if (screen == NULL)
    return FALSE;

  return build_state_payload(d, screen, buf, buf_size);
}

static void emit_state(struct descriptor_data *d, const struct onboarding_screen_info *screen)
{
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  d->web_onboarding_revision++;

  if (!build_state_payload(d, screen, payload, sizeof(payload)))
    return;

  MSDPSendPair(d, WEB_ONBOARDING_MSDP_VARIABLE, payload);
}

void web_onboarding_tick(struct descriptor_data *d)
{
  const struct onboarding_screen_info *screen = NULL;

  if (!web_onboarding_enabled(d))
    return;

  if (d->connected == d->web_onboarding_last_state && !d->web_onboarding_dirty)
    return;

  d->web_onboarding_last_state = d->connected;
  d->web_onboarding_dirty = FALSE;

  screen = find_screen(d->connected);
  if (screen == NULL)
  {
    /* Entering play, or entering a state this adapter does not present (OLC,
     * the string editors, the role-play suite). Both cases hand control back
     * to the classic terminal and clear every private onboarding surface. */
    emit_cleared(d);
    return;
  }

  emit_state(d, screen);
}
