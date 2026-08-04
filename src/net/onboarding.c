/**************************************************************************
 *  File: onboarding.c                                                     *
 *  Usage: Structured account and character-creation state for web clients *
 *                                                                         *
 *  This module is a presentation adapter, not a second state machine. It  *
 *  reads the descriptor's current nanny() state and the same authoritative *
 *  race, class, alignment, unlock, and account data that the terminal      *
 *  prompts use, then publishes a bounded JSON document over MSDP.          *
 *                                                                         *
 *  Catalog previews stay descriptor-local. Protocol-v2 mutations revalidate *
 *  against descriptor state and use the shared checked character saves.    *
 *  The adapter never emits a password, password hash, or other secret.      *
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
#include "magic/spells.h"
#include "character/class.h"
#include "character/race.h"
#include "constants.h"
#include "protocol.h"
#include "account.h"
#include "character/backgrounds.h"
#include "character/deities.h"
#include "roleplay.h"
#include "char_descs.h"
#include "clan.h"
#include "character/feats.h"
#include "character/character_creation.h"
#include "character/character_creation_content.h"
#include "onboarding.h"

#include <json-c/json.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>

/* Screen identifiers shared with the web client contract. */
#define SCREEN_UNSUPPORTED NULL

struct onboarding_screen_info
{
  int state;              /* CON_ constant */
  const char *screen;     /* contract screen id */
  const char *mode;       /* contract mode */
  const char *title;      /* short heading */
  const char *prompt;     /* one-line instruction */
  const char *input_kind; /* none | text | password | choice | confirm | multiline */
  bool sensitive;         /* password entry */
  int min_version;        /* lowest protocol version that may see this screen */
};

/* Every state this adapter presents. Any other state falls back to the
 * classic terminal, which keeps working unchanged. */
static const struct onboarding_screen_info onboarding_screens[] = {
    {CON_ACCOUNT_NAME, "account-name", "account", "Welcome to Luminari",
     "Enter your account name, or a new one to create an account.", "text", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_ACCOUNT_NAME_CONFIRM, "account-name-confirm", "account", "Confirm your account name",
     "Is the account name you entered spelled correctly?", "confirm", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_PASSWORD, "account-password", "account", "Account password",
     "Enter the password for this account.", "password", TRUE, WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_NEWPASSWD, "account-new-password", "account", "Choose a password",
     "Choose a password for your new account.", "password", TRUE, WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_CNFPASSWD, "account-confirm-password", "account", "Confirm your password",
     "Enter the same password once more.", "password", TRUE, WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_ACCOUNT_MENU, "account-menu", "account", "Your characters",
     "Choose a character to play, or create a new one.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_ACCOUNT_ADD, "account-link", "account", "Link an existing character",
     "Enter the name of the character you want to add to this account.", "text", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_ACCOUNT_ADD_PWD, "account-link-password", "account", "Prove ownership",
     "Enter that character's own password.", "password", TRUE, WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_GET_NAME, "name", "character-creation", "Name your character",
     "Enter the name your character will be known by.", "text", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_NAME_CNFRM, "name-confirm", "character-creation", "Confirm the name",
     "Did you spell that name the way you meant to?", "confirm", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_QSEX, "sex", "character-creation", "Choose an identity",
     "Select the sex your character is recorded as.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_QRACE, "race", "character-creation", "Choose your ancestry",
     "Select an ancestry to continue.", "choice", FALSE, WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_QRACE_HELP, "race-detail", "character-creation", "Keep this ancestry?",
     "Confirm this ancestry, or go back and choose another.", "confirm", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_QCLASS, "class", "character-creation", "Choose your path", "Select a class to continue.",
     "choice", FALSE, WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_QCLASS_HELP, "class-detail", "character-creation", "Keep this path?",
     "Confirm this class, or go back and choose another.", "confirm", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_CONFIRM_PREMADE, "build", "character-creation", "Guided or custom build",
     "Choose how your character's build is decided.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_QALIGN, "alignment", "character-creation", "Choose an alignment",
     "Only alignments valid for your ancestry and path are shown.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_SETPREFS, "preferences", "character-creation", "Recommended settings",
     "Apply the recommended settings bundle, or keep the defaults.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_CHAR_RP_DECIDE, "roleplay-decision", "character-creation", "Role-play details",
     "Decide whether to fill in role-play details now.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_RMOTD, "motd", "character-menu", "Message of the day",
     "Read the message of the day, then continue.", "confirm", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION},
    {CON_MENU, "character-menu", "character-menu", "Character menu",
     "Choose what to do with this character.", "choice", FALSE, WEB_ONBOARDING_PROTOCOL_VERSION},

    /* Protocol v2: the role-play identity suite. A v1 client never sees any of
     * these; find_screen() filters them out and the flow falls back to the
     * classic terminal exactly as it did before. */
    {CON_CHAR_RP_MENU, "roleplay-hub", "roleplay-profile", "Role-play profile",
     "Choose a detail to fill in. Everything here is optional.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_GEN_DESCS_INTRO, "short-desc-intro", "roleplay-profile", "Describe yourself",
     "Others see this description before they learn your name.", "none", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_GEN_DESCS_DESCRIPTORS_1, "short-desc-feature-1", "roleplay-profile", "Choose a feature",
     "Which feature should others notice first?", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_GEN_DESCS_ADJECTIVES_1, "short-desc-adjective-1", "roleplay-profile",
     "Choose a descriptor", "Pick the word that fits best.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_GEN_DESCS_MENU, "short-desc-preview", "roleplay-profile", "Your description",
     "Review the description built so far.", "none", FALSE, WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_GEN_DESCS_MENU_PARSE, "short-desc-review", "roleplay-profile", "Your description",
     "Keep this description, or start again.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_GEN_DESCS_DESCRIPTORS_2, "short-desc-feature-2", "roleplay-profile",
     "Choose a second feature", "Add another detail, or keep what you have.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_GEN_DESCS_ADJECTIVES_2, "short-desc-adjective-2", "roleplay-profile",
     "Choose a second descriptor", "Pick the word that fits best.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_PLR_DESC, "long-description", "roleplay-profile", "Long description",
     "Describe what others see when they look closely.", "multiline", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_PLR_BG, "background-story", "roleplay-profile", "Background story",
     "Write your history. Leave it blank to skip.", "multiline", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_BACKGROUND_ARCHTYPE, "background-catalog", "roleplay-profile", "Background",
     "Pick the life you led before adventuring.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_BACKGROUND_ARCHTYPE_CONFIRM, "background-confirm", "roleplay-profile",
     "Confirm this background", "This choice is permanent.", "confirm", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_GOALS_IDEAS, "goals-ideas", "roleplay-profile", "Goals",
     "Shape a goal outline, or write your own.", "none", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_GOALS_ENTER, "goals-editor", "roleplay-profile", "Goals",
     "Write what your character wants, why it matters, and what complicates it.", "multiline",
     FALSE, WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_PERSONALITY_IDEAS, "personality-ideas", "roleplay-profile", "Personality",
     "Pick an inspiration theme, or write your own.", "none", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_PERSONALITY_ENTER, "personality-editor", "roleplay-profile", "Personality",
     "Write the habits, mannerisms, tastes, and contradictions others can encounter.", "multiline",
     FALSE, WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_IDEALS_IDEAS, "ideals-ideas", "roleplay-profile", "Ideals",
     "Pick an inspiration theme, or write your own.", "none", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_IDEALS_ENTER, "ideals-editor", "roleplay-profile", "Ideals",
     "Write the principles your character protects when choices become costly.", "multiline", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_BONDS_IDEAS, "bonds-ideas", "roleplay-profile", "Bonds",
     "Pick an inspiration theme, or write your own.", "none", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_BONDS_ENTER, "bonds-editor", "roleplay-profile", "Bonds",
     "Write the people, places, promises, or possessions your character cannot treat as ordinary.",
     "multiline", FALSE, WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_FLAWS_IDEAS, "flaws-ideas", "roleplay-profile", "Flaws",
     "Pick an inspiration theme, or write your own.", "none", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_FLAWS_ENTER, "flaws-editor", "roleplay-profile", "Flaws",
     "Write a fear, vice, blind spot, or weakness that can create meaningful trouble.", "multiline",
     FALSE, WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_AGE_SELECT, "age-select", "roleplay-profile", "Age",
     "Choose how many years your character carries.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_QREGION, "region-catalog", "roleplay-profile", "Homeland",
     "Choose the region your character comes from.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_QREGION_HELP, "region-confirm", "roleplay-profile", "Confirm this homeland",
     "This choice is permanent.", "confirm", FALSE, WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_FACTION_SELECT, "faction-select", "roleplay-profile", "Faction",
     "Choose an allegiance, or remain unaffiliated.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_HOMETOWN_SELECT, "hometown-select", "roleplay-profile", "Hometown",
     "Choose where your character calls home.", "choice", FALSE,
     WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_DEITY_SELECT, "deity-select", "roleplay-profile", "Deity",
     "Choose a deity to follow, or none.", "choice", FALSE, WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
    {CON_CHARACTER_DEITY_CONFIRM, "deity-confirm", "roleplay-profile", "Confirm this deity",
     "This choice is permanent.", "confirm", FALSE, WEB_ONBOARDING_PROTOCOL_VERSION_MAX},
};

struct onboarding_error_info
{
  enum web_onboarding_error error;
  const char *code;
  const char *message;
  const char *field;
};

static const struct onboarding_error_info onboarding_errors[] = {
    {WEB_ONBOARDING_ERROR_INVALID_NAME, "invalid-name",
     "That name cannot be used. Please choose another.", "name"},
    {WEB_ONBOARDING_ERROR_NAME_TAKEN, "name-taken",
     "That character name is already in use. Please choose another.", "name"},
    {WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER, "editor-invalid-transfer",
     "That editor transfer was rejected. Your character was not changed.", "editor"},
    {WEB_ONBOARDING_ERROR_EDITOR_STALE, "editor-stale",
     "The editor changed before that action arrived. Review the current field and try again.",
     "editor"},
    {WEB_ONBOARDING_ERROR_EDITOR_TOO_LARGE, "editor-too-large",
     "That text is longer than this field allows. Your character was not changed.", "editor"},
    {WEB_ONBOARDING_ERROR_EDITOR_INVALID_CONTENT, "editor-invalid-content",
     "That text contains invalid or unsupported characters. Your character was not changed.",
     "editor"},
    {WEB_ONBOARDING_ERROR_EDITOR_RATE_LIMITED, "editor-rate-limited",
     "Too many profile saves arrived too quickly. Wait briefly and try again.", "editor"},
    {WEB_ONBOARDING_ERROR_EDITOR_SAVE_FAILED, "editor-save-failed",
     "The profile could not be saved. The previous value is still in place.", "editor"},
    {WEB_ONBOARDING_ERROR_EDITOR_LOAD_FAILED, "editor-load-failed",
     "The existing profile text could not be loaded safely. Use the classic terminal to review it.",
     "editor"},
    {WEB_ONBOARDING_ERROR_ROLEPLAY_INVALID_SELECTION, "roleplay-invalid-selection",
     "That choice is no longer available. Your character was not changed.", "roleplay"},
    {WEB_ONBOARDING_ERROR_ROLEPLAY_LOCKED, "roleplay-locked",
     "That choice can no longer be changed. Your character was not changed.", "roleplay"},
    {WEB_ONBOARDING_ERROR_ROLEPLAY_SAVE_FAILED, "roleplay-save-failed",
     "That choice could not be saved. Your character was not changed.", "roleplay"},
    {WEB_ONBOARDING_ERROR_WORKFLOW_INVALID, "workflow-invalid",
     "That workflow action was rejected. Your character was not changed.", "workflow"},
    {WEB_ONBOARDING_ERROR_WORKFLOW_STALE, "workflow-stale",
     "Character creation changed before that action arrived. Review the current screen and try "
     "again.",
     "workflow"},
    {WEB_ONBOARDING_ERROR_WORKFLOW_UNAVAILABLE, "workflow-unavailable",
     "That workflow action is not available on the current screen.", "workflow"},
    {WEB_ONBOARDING_ERROR_RESTART_FAILED, "restart-failed",
     "The character could not be removed safely. Its recoverable creation record was kept.",
     "workflow"},
};

enum web_onboarding_persistence_result
{
  WEB_ONBOARDING_PERSISTENCE_RESULT_NONE = 0,
  WEB_ONBOARDING_PERSISTENCE_RESULT_ACCEPTED,
  WEB_ONBOARDING_PERSISTENCE_RESULT_SAVED,
  WEB_ONBOARDING_PERSISTENCE_RESULT_FAILED
};

struct web_onboarding_inbound_transfer
{
  bool active;
  enum roleplay_text_field field;
  int expected_state;
  int revision;
  int chunk_count;
  int next_index;
  size_t total_bytes;
  size_t received_bytes;
  int64_t started_at_ms;
  struct char_data *character;
  struct account_data *account;
  unsigned char *content;
  char flow_id[64];
  char field_id[WEB_ONBOARDING_EDITOR_MAX_ID_BYTES + 1];
  char transfer_id[WEB_ONBOARDING_EDITOR_MAX_ID_BYTES + 1];
  char digest[65];
};

struct web_onboarding_outbound_transfer
{
  bool active;
  bool begin_sent;
  enum roleplay_text_field field;
  int expected_state;
  int revision;
  int chunk_count;
  int next_index;
  size_t total_bytes;
  size_t sent_bytes;
  int64_t started_at_ms;
  struct char_data *character;
  struct account_data *account;
  unsigned char *content;
  char flow_id[64];
  char field_id[WEB_ONBOARDING_EDITOR_MAX_ID_BYTES + 1];
  char transfer_id[WEB_ONBOARDING_EDITOR_MAX_ID_BYTES + 1];
  char digest[65];
};

/*
 * Opaque from structs.h: private editor bytes and their lifecycle stay owned
 * by this module rather than leaking into general descriptor code.
 */
struct web_onboarding_session
{
  struct web_onboarding_inbound_transfer inbound;
  struct web_onboarding_outbound_transfer outbound;
  enum web_onboarding_persistence_result persistence_result;
  int persistence_result_state;
  unsigned int content_revisions[ROLEPLAY_TEXT_FIELD_COUNT];
  int catalog_state;
  int catalog_page;
  int64_t rate_window_started_at_ms;
  unsigned int rate_window_commits;
  size_t rate_window_bytes;
};

static void clear_inbound_transfer(struct descriptor_data *d);
static void clear_outbound_transfer(struct descriptor_data *d);
static void clear_onboarding_session(struct descriptor_data *d);

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
/* Descriptor-local editor lifecycle                                      */
/* --------------------------------------------------------------------- */

static void wipe_private_memory(void *memory, size_t bytes)
{
  volatile unsigned char *cursor = memory;

  while (cursor != NULL && bytes > 0)
  {
    *cursor++ = 0;
    bytes--;
  }
}

static struct web_onboarding_session *ensure_onboarding_session(struct descriptor_data *d)
{
  if (d == NULL)
    return NULL;

  if (d->web_onboarding_session == NULL)
  {
    d->web_onboarding_session = calloc(1, sizeof(*d->web_onboarding_session));
    if (d->web_onboarding_session != NULL)
    {
      d->web_onboarding_session->persistence_result_state = -1;
      d->web_onboarding_session->catalog_state = -1;
    }
  }

  return d->web_onboarding_session;
}

static void clear_inbound_transfer(struct descriptor_data *d)
{
  struct web_onboarding_session *session = NULL;
  struct web_onboarding_inbound_transfer *transfer = NULL;

  if (d == NULL || (session = d->web_onboarding_session) == NULL)
    return;

  transfer = &session->inbound;
  if (transfer->content != NULL)
  {
    /*
     * Wipe the full allocation, not merely the bytes received so far. This
     * also clears a partially filled final chunk and the terminating byte.
     */
    wipe_private_memory(transfer->content, transfer->total_bytes + 1);
    free(transfer->content);
  }

  wipe_private_memory(transfer, sizeof(*transfer));
}

static void clear_outbound_transfer(struct descriptor_data *d)
{
  struct web_onboarding_session *session = NULL;
  struct web_onboarding_outbound_transfer *transfer = NULL;

  if (d == NULL || (session = d->web_onboarding_session) == NULL)
    return;

  transfer = &session->outbound;
  if (transfer->content != NULL)
  {
    wipe_private_memory(transfer->content, transfer->total_bytes + 1);
    free(transfer->content);
  }

  wipe_private_memory(transfer, sizeof(*transfer));
}

static void clear_editor_transfers(struct descriptor_data *d)
{
  clear_inbound_transfer(d);
  clear_outbound_transfer(d);
}

static void clear_onboarding_session(struct descriptor_data *d)
{
  if (d == NULL || d->web_onboarding_session == NULL)
    return;

  clear_editor_transfers(d);
  wipe_private_memory(d->web_onboarding_session, sizeof(*d->web_onboarding_session));
  free(d->web_onboarding_session);
  d->web_onboarding_session = NULL;
}

static int64_t onboarding_now_ms(void)
{
  struct timespec now;

  if (clock_gettime(CLOCK_MONOTONIC, &now) == 0)
    return ((int64_t)now.tv_sec * 1000) + (now.tv_nsec / 1000000);

  return (int64_t)time(NULL) * 1000;
}

static void onboarding_flow_id(struct descriptor_data *d, char *flow_id, size_t flow_id_size)
{
  if (flow_id == NULL || flow_id_size == 0)
    return;

  if (d == NULL)
  {
    flow_id[0] = '\0';
    return;
  }

  snprintf(flow_id, flow_id_size, "%d-%ld", d->desc_num, (long)d->login_time);
}

static void set_editor_outcome(struct descriptor_data *d, enum web_onboarding_error error,
                               enum web_onboarding_persistence_result persistence_result)
{
  struct web_onboarding_session *session = ensure_onboarding_session(d);

  if (d == NULL)
    return;

  d->web_onboarding_error = error;
  if (session != NULL)
  {
    session->persistence_result = persistence_result;
    session->persistence_result_state = d->connected;
  }
  web_onboarding_mark_dirty(d);
}

void web_onboarding_report_roleplay_commit(struct descriptor_data *d,
                                           enum roleplay_commit_result result)
{
  enum web_onboarding_error error = WEB_ONBOARDING_ERROR_NONE;
  enum web_onboarding_persistence_result persistence = WEB_ONBOARDING_PERSISTENCE_RESULT_FAILED;

  if (d == NULL || !web_onboarding_v2_enabled(d))
    return;

  if (result == ROLEPLAY_COMMIT_OK)
    persistence = WEB_ONBOARDING_PERSISTENCE_RESULT_SAVED;
  else if (result == ROLEPLAY_COMMIT_LOCKED)
    error = WEB_ONBOARDING_ERROR_ROLEPLAY_LOCKED;
  else if (result == ROLEPLAY_COMMIT_SAVE_FAILED || result == ROLEPLAY_COMMIT_INDEX_SAVE_FAILED)
    error = WEB_ONBOARDING_ERROR_ROLEPLAY_SAVE_FAILED;
  else
    error = WEB_ONBOARDING_ERROR_ROLEPLAY_INVALID_SELECTION;

  set_editor_outcome(d, error, persistence);
}

#define WEB_ONBOARDING_CATALOG_NEXT "__onboarding-next__"
#define WEB_ONBOARDING_CATALOG_PREVIOUS "__onboarding-previous__"

bool web_onboarding_handle_catalog_control(struct descriptor_data *d, const char *value)
{
  struct web_onboarding_session *session = NULL;
  int delta = 0;

  if (d == NULL || value == NULL || !web_onboarding_v2_enabled(d))
    return FALSE;
  if (!strcmp(value, WEB_ONBOARDING_CATALOG_NEXT))
    delta = 1;
  else if (!strcmp(value, WEB_ONBOARDING_CATALOG_PREVIOUS))
    delta = -1;
  else
    return FALSE;

  session = ensure_onboarding_session(d);
  if (session == NULL)
    return TRUE;
  if (session->catalog_state != d->connected)
  {
    session->catalog_state = d->connected;
    session->catalog_page = 0;
  }
  session->catalog_page += delta;
  if (session->catalog_page < 0)
    session->catalog_page = 0;
  web_onboarding_mark_dirty(d);
  return TRUE;
}

static bool transfer_lifecycle_is_current(struct descriptor_data *d,
                                          struct web_onboarding_inbound_transfer *transfer)
{
  char flow_id[64];

  if (d == NULL || transfer == NULL || !transfer->active || d->character == NULL)
    return FALSE;

  onboarding_flow_id(d, flow_id, sizeof(flow_id));
  return web_onboarding_v2_enabled(d) && d->connected == transfer->expected_state &&
         d->web_onboarding_revision == transfer->revision && d->character == transfer->character &&
         d->account == transfer->account && !strcmp(flow_id, transfer->flow_id) &&
         roleplay_text_field_from_state(d->connected) == transfer->field;
}

static bool transfer_is_expired(const struct web_onboarding_inbound_transfer *transfer,
                                int64_t now_ms)
{
  if (transfer == NULL || !transfer->active)
    return FALSE;

  return now_ms < transfer->started_at_ms ||
         now_ms - transfer->started_at_ms > WEB_ONBOARDING_EDITOR_TRANSFER_TIMEOUT_MS;
}

static void reject_editor_transfer(struct descriptor_data *d, enum web_onboarding_error error)
{
  clear_inbound_transfer(d);
  set_editor_outcome(d, error, WEB_ONBOARDING_PERSISTENCE_RESULT_FAILED);
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

  if (version != d->web_onboarding_version)
    clear_editor_transfers(d);

  d->web_onboarding_version = version;
  d->web_onboarding_last_state = -1;
  d->web_onboarding_dirty = TRUE;
}

/* Highest protocol version implemented by this source build. */
static int web_onboarding_max_offered_version(void)
{
  return WEB_ONBOARDING_PROTOCOL_VERSION_MAX;
}

void web_onboarding_set_version_list(struct descriptor_data *d, const char *value)
{
  const char *cursor = NULL;
  int best = 0;
  int ceiling = web_onboarding_max_offered_version();

  if (d == NULL || value == NULL)
    return;

  /*
   * Pick the highest version both sides support. The list is client-supplied,
   * so it is scanned defensively: only digits and separators are meaningful
   * and anything above our ceiling is ignored rather than trusted.
   */
  for (cursor = value; *cursor != '\0';)
  {
    if (*cursor >= '0' && *cursor <= '9')
    {
      int candidate = 0;

      while (*cursor >= '0' && *cursor <= '9')
      {
        if (candidate < 10000)
          candidate = (candidate * 10) + (*cursor - '0');
        ++cursor;
      }

      if (candidate >= WEB_ONBOARDING_PROTOCOL_VERSION && candidate <= ceiling && candidate > best)
        best = candidate;
    }
    else
    {
      ++cursor;
    }
  }

  if (best == 0)
    return;

  if (best != d->web_onboarding_version)
    clear_editor_transfers(d);

  d->web_onboarding_version = best;
  d->web_onboarding_last_state = -1;
  d->web_onboarding_dirty = TRUE;
}

/*
 * Version to stamp on an emitted document.
 *
 * Deliberately not web_onboarding_version(), which also asserts that MSDP is
 * ready: by the time a payload is being built the descriptor's version has
 * already been negotiated, and a document must never claim v0. An unset
 * version means "no list was advertised", which is exactly v1.
 */
static int web_onboarding_payload_version(struct descriptor_data *d)
{
  int version = (d != NULL) ? d->web_onboarding_version : 0;
  int ceiling = web_onboarding_max_offered_version();

  if (version < WEB_ONBOARDING_PROTOCOL_VERSION)
    return WEB_ONBOARDING_PROTOCOL_VERSION;
  if (version > ceiling)
    return ceiling;

  return version;
}

int web_onboarding_version(struct descriptor_data *d)
{
  if (!web_onboarding_enabled(d))
    return 0;

  return d->web_onboarding_version;
}

bool web_onboarding_v2_enabled(struct descriptor_data *d)
{
  return web_onboarding_version(d) >= WEB_ONBOARDING_PROTOCOL_VERSION_MAX;
}

bool web_onboarding_enabled(struct descriptor_data *d)
{
  if (d == NULL || d->pProtocol == NULL || !d->pProtocol->bMSDP)
    return FALSE;

  /*
   * Any version from v1 up to what this build offers is acceptable. The
   * capability handlers already clamped the value, so a client cannot select
   * a version this build does not implement.
   */
  return d->web_onboarding_version >= WEB_ONBOARDING_PROTOCOL_VERSION &&
         d->web_onboarding_version <= web_onboarding_max_offered_version();
}

void web_onboarding_mark_dirty(struct descriptor_data *d)
{
  if (d != NULL)
    d->web_onboarding_dirty = TRUE;
}

void web_onboarding_set_error(struct descriptor_data *d, enum web_onboarding_error error)
{
  if (d == NULL)
    return;

  d->web_onboarding_error = error;
  web_onboarding_mark_dirty(d);
}

void web_onboarding_reset(struct descriptor_data *d)
{
  if (d == NULL)
    return;

  clear_onboarding_session(d);
  d->web_onboarding_version = 0;
  d->web_onboarding_revision = 0;
  d->web_onboarding_last_state = -1;
  d->web_onboarding_dirty = FALSE;
  d->web_onboarding_error = WEB_ONBOARDING_ERROR_NONE;
}

/* --------------------------------------------------------------------- */
/* Protocol-v2 inbound editor actions                                     */
/* --------------------------------------------------------------------- */

static const char *skip_json_whitespace(const char *cursor, const char *end)
{
  while (cursor < end && (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n'))
    cursor++;

  return cursor;
}

/*
 * The action envelopes are deliberately flat. Scan their member names before
 * json-c parses values so duplicate keys cannot be collapsed with "last one
 * wins" semantics. Nested values and escaped member names are not part of the
 * wire contract and are rejected.
 */
static bool flat_json_members_are_unique(const char *payload, size_t payload_bytes)
{
  char keys[12][32];
  char key[32];
  const char *cursor = payload;
  const char *end = payload + payload_bytes;
  size_t key_count = 0;
  size_t key_length = 0;
  size_t index = 0;
  bool escaped = FALSE;

  cursor = skip_json_whitespace(cursor, end);
  if (cursor >= end || *cursor++ != '{')
    return FALSE;

  cursor = skip_json_whitespace(cursor, end);
  if (cursor < end && *cursor == '}')
  {
    cursor = skip_json_whitespace(cursor + 1, end);
    return cursor == end;
  }

  while (cursor < end)
  {
    cursor = skip_json_whitespace(cursor, end);
    if (cursor >= end || *cursor++ != '"')
      return FALSE;

    key_length = 0;
    while (cursor < end && *cursor != '"')
    {
      unsigned char byte = (unsigned char)*cursor++;

      if (byte == '\\' || byte < 0x20 || byte >= 0x7f || key_length + 1 >= sizeof(key))
        return FALSE;
      key[key_length++] = (char)byte;
    }
    if (cursor >= end || *cursor++ != '"' || key_length == 0 || key_count >= 12)
      return FALSE;
    key[key_length] = '\0';

    for (index = 0; index < key_count; index++)
      if (!strcmp(keys[index], key))
        return FALSE;
    strlcpy(keys[key_count++], key, sizeof(keys[0]));

    cursor = skip_json_whitespace(cursor, end);
    if (cursor >= end || *cursor++ != ':')
      return FALSE;
    cursor = skip_json_whitespace(cursor, end);
    if (cursor >= end || *cursor == '{' || *cursor == '[')
      return FALSE;

    if (*cursor == '"')
    {
      cursor++;
      escaped = FALSE;
      while (cursor < end)
      {
        if (escaped)
        {
          escaped = FALSE;
          cursor++;
          continue;
        }
        if (*cursor == '\\')
        {
          escaped = TRUE;
          cursor++;
          continue;
        }
        if (*cursor == '"')
        {
          cursor++;
          break;
        }
        if ((unsigned char)*cursor < 0x20)
          return FALSE;
        cursor++;
      }
      if (cursor > end || (cursor == end && *(cursor - 1) != '"') || escaped)
        return FALSE;
    }
    else
    {
      const char *value_start = cursor;

      while (cursor < end && *cursor != ',' && *cursor != '}')
      {
        if (*cursor == '{' || *cursor == '[' || *cursor == ']' || *cursor == '"')
          return FALSE;
        cursor++;
      }
      if (cursor == value_start)
        return FALSE;
    }

    cursor = skip_json_whitespace(cursor, end);
    if (cursor >= end)
      return FALSE;
    if (*cursor == ',')
    {
      cursor++;
      continue;
    }
    if (*cursor == '}')
    {
      cursor = skip_json_whitespace(cursor + 1, end);
      return cursor == end;
    }
    return FALSE;
  }

  return FALSE;
}

static json_object *parse_editor_envelope(const char *payload)
{
  struct json_tokener *tokener = NULL;
  json_object *root = NULL;
  enum json_tokener_error error;
  const char *cursor = NULL;
  size_t payload_bytes = 0;
  size_t parsed_bytes = 0;

  if (payload == NULL)
    return NULL;

  payload_bytes = strlen(payload);
  if (payload_bytes == 0 || payload_bytes > WEB_ONBOARDING_MAX_PAYLOAD ||
      !flat_json_members_are_unique(payload, payload_bytes))
    return NULL;

  tokener = json_tokener_new_ex(8);
  if (tokener == NULL)
    return NULL;

  json_tokener_set_flags(tokener, JSON_TOKENER_STRICT | JSON_TOKENER_VALIDATE_UTF8);
  root = json_tokener_parse_ex(tokener, payload, (int)payload_bytes);
  error = json_tokener_get_error(tokener);
  parsed_bytes = json_tokener_get_parse_end(tokener);

  cursor = payload + MIN(parsed_bytes, payload_bytes);
  cursor = skip_json_whitespace(cursor, payload + payload_bytes);
  if (error != json_tokener_success || root == NULL ||
      !json_object_is_type(root, json_type_object) || cursor != payload + payload_bytes)
  {
    if (root != NULL)
      json_object_put(root);
    root = NULL;
  }

  json_tokener_free(tokener);
  return root;
}

static bool json_object_has_exact_keys(json_object *root, const char *const *keys, size_t key_count)
{
  json_object *unused = NULL;
  size_t index = 0;

  if (root == NULL || (size_t)json_object_object_length(root) != key_count)
    return FALSE;

  for (index = 0; index < key_count; index++)
    if (!json_object_object_get_ex(root, keys[index], &unused))
      return FALSE;

  return TRUE;
}

static const char *json_required_string(json_object *root, const char *key, size_t max_bytes)
{
  json_object *value = NULL;
  const char *text = NULL;
  size_t text_bytes = 0;

  if (!json_object_object_get_ex(root, key, &value) ||
      !json_object_is_type(value, json_type_string))
    return NULL;

  text = json_object_get_string(value);
  text_bytes = (size_t)json_object_get_string_len(value);
  if (text == NULL || text_bytes == 0 || text_bytes > max_bytes || strlen(text) != text_bytes)
    return NULL;

  return text;
}

static bool json_required_integer(json_object *root, const char *key, int64_t minimum,
                                  int64_t maximum, int64_t *result)
{
  json_object *value = NULL;
  int64_t number = 0;

  if (!json_object_object_get_ex(root, key, &value) || !json_object_is_type(value, json_type_int))
    return FALSE;

  number = json_object_get_int64(value);
  if (number < minimum || number > maximum)
    return FALSE;

  *result = number;
  return TRUE;
}

static bool editor_id_is_valid(const char *id)
{
  const unsigned char *cursor = (const unsigned char *)id;
  size_t bytes = 0;

  if (id == NULL)
    return FALSE;

  bytes = strlen(id);
  if (bytes == 0 || bytes > WEB_ONBOARDING_EDITOR_MAX_ID_BYTES)
    return FALSE;

  for (; *cursor != '\0'; cursor++)
    if (!isalnum(*cursor) && *cursor != '-' && *cursor != '_' && *cursor != '.' && *cursor != ':' &&
        *cursor != '/')
      return FALSE;

  return TRUE;
}

static bool digest_is_valid(const char *digest)
{
  size_t index = 0;

  if (digest == NULL || strlen(digest) != 64)
    return FALSE;

  for (index = 0; index < 64; index++)
    if (!((digest[index] >= '0' && digest[index] <= '9') ||
          (digest[index] >= 'a' && digest[index] <= 'f')))
      return FALSE;

  return TRUE;
}

static int expected_editor_chunk_count(size_t total_bytes)
{
  if (total_bytes == 0)
    return 0;

  return (int)((total_bytes + WEB_ONBOARDING_EDITOR_MAX_CHUNK_BYTES - 1) /
               WEB_ONBOARDING_EDITOR_MAX_CHUNK_BYTES);
}

static bool decode_editor_chunk(const char *encoded, unsigned char *decoded, size_t *decoded_bytes)
{
  char canonical[WEB_ONBOARDING_EDITOR_MAX_BASE64_BYTES + 1];
  size_t encoded_bytes = 0;
  size_t padding = 0;
  size_t index = 0;
  int canonical_bytes = 0;
  int result = 0;

  if (encoded == NULL || decoded == NULL || decoded_bytes == NULL)
    return FALSE;

  encoded_bytes = strlen(encoded);
  if (encoded_bytes == 0 || encoded_bytes > WEB_ONBOARDING_EDITOR_MAX_BASE64_BYTES ||
      encoded_bytes % 4 != 0)
    return FALSE;

  if (encoded[encoded_bytes - 1] == '=')
    padding++;
  if (encoded[encoded_bytes - 2] == '=')
    padding++;

  for (index = 0; index < encoded_bytes - padding; index++)
    if (!isalnum((unsigned char)encoded[index]) && encoded[index] != '+' && encoded[index] != '/')
      return FALSE;
  for (; index < encoded_bytes; index++)
    if (encoded[index] != '=')
      return FALSE;

  result = EVP_DecodeBlock(decoded, (const unsigned char *)encoded, (int)encoded_bytes);
  if (result < 0 || (size_t)result < padding)
    return FALSE;

  *decoded_bytes = (size_t)result - padding;
  if (*decoded_bytes > WEB_ONBOARDING_EDITOR_MAX_CHUNK_BYTES)
    return FALSE;

  canonical_bytes = EVP_EncodeBlock((unsigned char *)canonical, decoded, (int)*decoded_bytes);
  if (canonical_bytes < 0 || (size_t)canonical_bytes != encoded_bytes)
  {
    OPENSSL_cleanse(canonical, sizeof(canonical));
    return FALSE;
  }
  canonical[canonical_bytes] = '\0';
  if (CRYPTO_memcmp(canonical, encoded, encoded_bytes) != 0)
  {
    OPENSSL_cleanse(canonical, sizeof(canonical));
    return FALSE;
  }

  OPENSSL_cleanse(canonical, sizeof(canonical));
  return TRUE;
}

static bool editor_digest_hex(const unsigned char *content, size_t content_bytes, char result[65])
{
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_bytes = 0;
  size_t index = 0;
  bool success = FALSE;

  memset(digest, 0, sizeof(digest));
  result[0] = '\0';
  if (EVP_Digest(content, content_bytes, digest, &digest_bytes, EVP_sha256(), NULL) == 1 &&
      digest_bytes == 32)
  {
    for (index = 0; index < digest_bytes; index++)
      snprintf(result + (index * 2), 3, "%02x", digest[index]);
    result[64] = '\0';
    success = TRUE;
  }

  OPENSSL_cleanse(digest, sizeof(digest));
  return success;
}

static bool editor_digest_matches(const unsigned char *content, size_t content_bytes,
                                  const char *expected_hex)
{
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned char expected[32];
  unsigned int digest_bytes = 0;
  size_t index = 0;
  bool matches = FALSE;

  for (index = 0; index < sizeof(expected); index++)
  {
    unsigned int high = 0;
    unsigned int low = 0;
    char high_char = expected_hex[index * 2];
    char low_char = expected_hex[(index * 2) + 1];

    high = (unsigned int)(high_char <= '9' ? high_char - '0' : high_char - 'a' + 10);
    low = (unsigned int)(low_char <= '9' ? low_char - '0' : low_char - 'a' + 10);
    expected[index] = (unsigned char)((high << 4) | low);
  }

  if (EVP_Digest(content, content_bytes, digest, &digest_bytes, EVP_sha256(), NULL) == 1 &&
      digest_bytes == sizeof(expected))
    matches = CRYPTO_memcmp(digest, expected, sizeof(expected)) == 0;

  OPENSSL_cleanse(digest, sizeof(digest));
  OPENSSL_cleanse(expected, sizeof(expected));
  return matches;
}

static bool consume_editor_rate_budget(struct web_onboarding_session *session, size_t bytes,
                                       int64_t now_ms)
{
  if (session == NULL)
    return FALSE;

  if (session->rate_window_started_at_ms == 0 || now_ms < session->rate_window_started_at_ms ||
      now_ms - session->rate_window_started_at_ms > WEB_ONBOARDING_EDITOR_RATE_WINDOW_MS)
  {
    session->rate_window_started_at_ms = now_ms;
    session->rate_window_commits = 0;
    session->rate_window_bytes = 0;
  }

  if (session->rate_window_commits + 1 > WEB_ONBOARDING_EDITOR_MAX_COMMITS_PER_WINDOW ||
      session->rate_window_bytes + bytes > WEB_ONBOARDING_EDITOR_MAX_BYTES_PER_WINDOW)
    return FALSE;

  session->rate_window_commits++;
  session->rate_window_bytes += bytes;
  return TRUE;
}

static void leave_terminal_editor(struct descriptor_data *d, bool abort_edit)
{
  if (d == NULL)
    return;

  if (abort_edit && d->str != NULL)
  {
    if (*d->str != NULL)
    {
      wipe_private_memory(*d->str, strlen(*d->str));
      free(*d->str);
    }
    *d->str = d->backstr;
    d->backstr = NULL;
  }
  else if (d->backstr != NULL)
  {
    wipe_private_memory(d->backstr, strlen(d->backstr));
    free(d->backstr);
    d->backstr = NULL;
  }

  d->str = NULL;
  d->max_str = 0;
  d->mail_to = 0;
  if (d->character != NULL && !IS_NPC(d->character))
    REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
}

static bool begin_editor_transfer(struct descriptor_data *d, json_object *root, int64_t now_ms)
{
  static const char *const keys[] = {"v",          "action",  "phase",      "flowId",
                                     "revision",   "fieldId", "transferId", "totalBytes",
                                     "chunkCount", "digest"};
  struct web_onboarding_session *session = NULL;
  struct web_onboarding_inbound_transfer *transfer = NULL;
  enum roleplay_text_field field = ROLEPLAY_TEXT_FIELD_INVALID;
  const char *action = NULL;
  const char *phase = NULL;
  const char *flow_id = NULL;
  const char *field_id = NULL;
  const char *transfer_id = NULL;
  const char *digest = NULL;
  char expected_flow_id[64];
  int64_t version = 0;
  int64_t revision = 0;
  int64_t total_bytes = 0;
  int64_t chunk_count = 0;
  unsigned char *content = NULL;

  if (!json_object_has_exact_keys(root, keys, sizeof(keys) / sizeof(keys[0])))
    return FALSE;

  action = json_required_string(root, "action", 24);
  phase = json_required_string(root, "phase", 16);
  flow_id = json_required_string(root, "flowId", WEB_ONBOARDING_EDITOR_MAX_ID_BYTES);
  field_id = json_required_string(root, "fieldId", WEB_ONBOARDING_EDITOR_MAX_ID_BYTES);
  transfer_id = json_required_string(root, "transferId", WEB_ONBOARDING_EDITOR_MAX_ID_BYTES);
  digest = json_required_string(root, "digest", 64);
  if (!json_required_integer(root, "v", WEB_ONBOARDING_PROTOCOL_VERSION_MAX,
                             WEB_ONBOARDING_PROTOCOL_VERSION_MAX, &version) ||
      !json_required_integer(root, "revision", 0, 1000000, &revision) ||
      !json_required_integer(root, "totalBytes", 0, WEB_ONBOARDING_EDITOR_MAX_CONTENT_BYTES,
                             &total_bytes) ||
      !json_required_integer(root, "chunkCount", 0, WEB_ONBOARDING_EDITOR_MAX_CHUNKS,
                             &chunk_count) ||
      action == NULL || strcmp(action, "save-text") || phase == NULL || strcmp(phase, "begin") ||
      !editor_id_is_valid(flow_id) || !editor_id_is_valid(field_id) ||
      !editor_id_is_valid(transfer_id) || !digest_is_valid(digest))
    return FALSE;

  (void)version;
  field = roleplay_text_field_from_id(field_id);
  onboarding_flow_id(d, expected_flow_id, sizeof(expected_flow_id));
  if (d->character == NULL || IS_NPC(d->character) || field == ROLEPLAY_TEXT_FIELD_INVALID ||
      field != roleplay_text_field_from_state(d->connected) ||
      (size_t)total_bytes > roleplay_text_field_max_bytes(field) ||
      (int)chunk_count != expected_editor_chunk_count((size_t)total_bytes))
  {
    reject_editor_transfer(d, (size_t)total_bytes > roleplay_text_field_max_bytes(field)
                                  ? WEB_ONBOARDING_ERROR_EDITOR_TOO_LARGE
                                  : WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER);
    return TRUE;
  }

  if (strcmp(flow_id, expected_flow_id) || revision != d->web_onboarding_revision)
  {
    reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_STALE);
    return TRUE;
  }

  session = ensure_onboarding_session(d);
  if (session == NULL)
  {
    set_editor_outcome(d, WEB_ONBOARDING_ERROR_EDITOR_SAVE_FAILED,
                       WEB_ONBOARDING_PERSISTENCE_RESULT_FAILED);
    return TRUE;
  }

  if (session->inbound.active)
  {
    if (!transfer_is_expired(&session->inbound, now_ms))
    {
      reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER);
      return TRUE;
    }
    clear_inbound_transfer(d);
  }

  /*
   * A client must receive and verify the authoritative existing value before
   * it may replace it. The outbound buffer is cleared only after its commit
   * frame has been queued, so an early or forged upload cannot blind-write a
   * field whose download is still in progress.
   */
  if (session->outbound.active)
  {
    reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER);
    return TRUE;
  }

  content = calloc((size_t)total_bytes + 1, sizeof(unsigned char));
  if (content == NULL)
  {
    set_editor_outcome(d, WEB_ONBOARDING_ERROR_EDITOR_SAVE_FAILED,
                       WEB_ONBOARDING_PERSISTENCE_RESULT_FAILED);
    return TRUE;
  }

  transfer = &session->inbound;
  transfer->active = TRUE;
  transfer->field = field;
  transfer->expected_state = d->connected;
  transfer->revision = (int)revision;
  transfer->chunk_count = (int)chunk_count;
  transfer->total_bytes = (size_t)total_bytes;
  transfer->started_at_ms = now_ms;
  transfer->character = d->character;
  transfer->account = d->account;
  transfer->content = content;
  strlcpy(transfer->flow_id, flow_id, sizeof(transfer->flow_id));
  strlcpy(transfer->field_id, field_id, sizeof(transfer->field_id));
  strlcpy(transfer->transfer_id, transfer_id, sizeof(transfer->transfer_id));
  strlcpy(transfer->digest, digest, sizeof(transfer->digest));

  session->persistence_result = WEB_ONBOARDING_PERSISTENCE_RESULT_NONE;
  d->web_onboarding_error = WEB_ONBOARDING_ERROR_NONE;
  return TRUE;
}

static bool accept_editor_chunk(struct descriptor_data *d, json_object *root, int64_t now_ms)
{
  static const char *const keys[] = {"phase", "transferId", "index", "data"};
  struct web_onboarding_session *session = NULL;
  struct web_onboarding_inbound_transfer *transfer = NULL;
  const char *phase = NULL;
  const char *transfer_id = NULL;
  const char *data = NULL;
  unsigned char decoded[WEB_ONBOARDING_EDITOR_MAX_CHUNK_BYTES + 3];
  int64_t index = 0;
  size_t decoded_bytes = 0;
  size_t remaining = 0;
  size_t expected_bytes = 0;

  if (!json_object_has_exact_keys(root, keys, sizeof(keys) / sizeof(keys[0])))
    return FALSE;

  phase = json_required_string(root, "phase", 16);
  transfer_id = json_required_string(root, "transferId", WEB_ONBOARDING_EDITOR_MAX_ID_BYTES);
  data = json_required_string(root, "data", WEB_ONBOARDING_EDITOR_MAX_BASE64_BYTES);
  if (phase == NULL || strcmp(phase, "chunk") || !editor_id_is_valid(transfer_id) ||
      !json_required_integer(root, "index", 0, WEB_ONBOARDING_EDITOR_MAX_CHUNKS - 1, &index) ||
      data == NULL)
    return FALSE;

  session = d->web_onboarding_session;
  if (session == NULL || !session->inbound.active)
  {
    reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER);
    return TRUE;
  }
  transfer = &session->inbound;

  if (transfer_is_expired(transfer, now_ms) || !transfer_lifecycle_is_current(d, transfer) ||
      strcmp(transfer_id, transfer->transfer_id))
  {
    reject_editor_transfer(d, transfer_is_expired(transfer, now_ms)
                                  ? WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER
                                  : WEB_ONBOARDING_ERROR_EDITOR_STALE);
    return TRUE;
  }

  if (index != transfer->next_index || index >= transfer->chunk_count ||
      !decode_editor_chunk(data, decoded, &decoded_bytes))
  {
    OPENSSL_cleanse(decoded, sizeof(decoded));
    reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER);
    return TRUE;
  }

  remaining = transfer->total_bytes - transfer->received_bytes;
  expected_bytes = MIN(remaining, (size_t)WEB_ONBOARDING_EDITOR_MAX_CHUNK_BYTES);
  if (decoded_bytes != expected_bytes ||
      transfer->received_bytes + decoded_bytes > transfer->total_bytes)
  {
    OPENSSL_cleanse(decoded, sizeof(decoded));
    reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_TOO_LARGE);
    return TRUE;
  }

  memcpy(transfer->content + transfer->received_bytes, decoded, decoded_bytes);
  transfer->received_bytes += decoded_bytes;
  transfer->next_index++;
  OPENSSL_cleanse(decoded, sizeof(decoded));
  return TRUE;
}

static bool commit_editor_transfer(struct descriptor_data *d, json_object *root, int64_t now_ms)
{
  static const char *const keys[] = {"phase", "transferId"};
  struct web_onboarding_session *session = NULL;
  struct web_onboarding_inbound_transfer *transfer = NULL;
  enum roleplay_text_commit_result commit_result = ROLEPLAY_TEXT_COMMIT_INVALID_FIELD;
  enum roleplay_text_field field = ROLEPLAY_TEXT_FIELD_INVALID;
  const char *phase = NULL;
  const char *transfer_id = NULL;
  size_t content_bytes = 0;

  if (!json_object_has_exact_keys(root, keys, sizeof(keys) / sizeof(keys[0])))
    return FALSE;

  phase = json_required_string(root, "phase", 16);
  transfer_id = json_required_string(root, "transferId", WEB_ONBOARDING_EDITOR_MAX_ID_BYTES);
  if (phase == NULL || strcmp(phase, "commit") || !editor_id_is_valid(transfer_id))
    return FALSE;

  session = d->web_onboarding_session;
  if (session == NULL || !session->inbound.active)
  {
    reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER);
    return TRUE;
  }
  transfer = &session->inbound;

  if (transfer_is_expired(transfer, now_ms) || !transfer_lifecycle_is_current(d, transfer) ||
      strcmp(transfer_id, transfer->transfer_id))
  {
    reject_editor_transfer(d, transfer_is_expired(transfer, now_ms)
                                  ? WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER
                                  : WEB_ONBOARDING_ERROR_EDITOR_STALE);
    return TRUE;
  }

  if (transfer->next_index != transfer->chunk_count ||
      transfer->received_bytes != transfer->total_bytes ||
      !editor_digest_matches(transfer->content, transfer->total_bytes, transfer->digest))
  {
    reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER);
    return TRUE;
  }

  content_bytes = transfer->total_bytes;
  field = transfer->field;
  if (!consume_editor_rate_budget(session, content_bytes, now_ms))
  {
    reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_RATE_LIMITED);
    return TRUE;
  }

  commit_result =
      roleplay_text_commit_checked(d->character, field, transfer->content, content_bytes);
  clear_inbound_transfer(d);

  if (commit_result != ROLEPLAY_TEXT_COMMIT_OK)
  {
    enum web_onboarding_error error = WEB_ONBOARDING_ERROR_EDITOR_SAVE_FAILED;

    if (commit_result == ROLEPLAY_TEXT_COMMIT_TOO_LARGE)
      error = WEB_ONBOARDING_ERROR_EDITOR_TOO_LARGE;
    else if (commit_result == ROLEPLAY_TEXT_COMMIT_INVALID_CONTENT)
      error = WEB_ONBOARDING_ERROR_EDITOR_INVALID_CONTENT;
    reject_editor_transfer(d, error);
    return TRUE;
  }

  session->content_revisions[field]++;
  leave_terminal_editor(d, FALSE);
  show_character_rp_menu(d);
  STATE(d) = CON_CHAR_RP_MENU;
  set_editor_outcome(d, WEB_ONBOARDING_ERROR_NONE, WEB_ONBOARDING_PERSISTENCE_RESULT_SAVED);
  return TRUE;
}

static bool cancel_editor(struct descriptor_data *d, json_object *root)
{
  static const char *const keys[] = {"v", "action", "phase", "flowId", "revision", "fieldId"};
  enum roleplay_text_field field = ROLEPLAY_TEXT_FIELD_INVALID;
  struct web_onboarding_session *session = NULL;
  const char *action = NULL;
  const char *phase = NULL;
  const char *flow_id = NULL;
  const char *field_id = NULL;
  char expected_flow_id[64];
  int64_t version = 0;
  int64_t revision = 0;

  if (!json_object_has_exact_keys(root, keys, sizeof(keys) / sizeof(keys[0])))
    return FALSE;

  action = json_required_string(root, "action", 24);
  phase = json_required_string(root, "phase", 16);
  flow_id = json_required_string(root, "flowId", WEB_ONBOARDING_EDITOR_MAX_ID_BYTES);
  field_id = json_required_string(root, "fieldId", WEB_ONBOARDING_EDITOR_MAX_ID_BYTES);
  if (!json_required_integer(root, "v", WEB_ONBOARDING_PROTOCOL_VERSION_MAX,
                             WEB_ONBOARDING_PROTOCOL_VERSION_MAX, &version) ||
      !json_required_integer(root, "revision", 0, 1000000, &revision) || action == NULL ||
      strcmp(action, "cancel-editor") || phase == NULL || strcmp(phase, "cancel") ||
      !editor_id_is_valid(flow_id) || !editor_id_is_valid(field_id))
    return FALSE;

  (void)version;
  field = roleplay_text_field_from_id(field_id);
  onboarding_flow_id(d, expected_flow_id, sizeof(expected_flow_id));
  if (field == ROLEPLAY_TEXT_FIELD_INVALID ||
      field != roleplay_text_field_from_state(d->connected) || strcmp(flow_id, expected_flow_id) ||
      revision != d->web_onboarding_revision)
  {
    reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_STALE);
    return TRUE;
  }

  clear_editor_transfers(d);
  leave_terminal_editor(d, TRUE);
  show_character_rp_menu(d);
  STATE(d) = CON_CHAR_RP_MENU;
  session = ensure_onboarding_session(d);
  if (session != NULL)
  {
    session->persistence_result = WEB_ONBOARDING_PERSISTENCE_RESULT_NONE;
    session->persistence_result_state = -1;
  }
  d->web_onboarding_error = WEB_ONBOARDING_ERROR_NONE;
  web_onboarding_mark_dirty(d);
  return TRUE;
}

static bool handle_workflow_action(struct descriptor_data *d, json_object *root)
{
  static const char *const keys[] = {"v", "action", "flowId", "revision"};
  const char *action = json_required_string(root, "action", 32);
  const char *flow_id = NULL;
  char expected_flow_id[64];
  int64_t version = 0;
  int64_t revision = 0;

  if (action == NULL || (strcmp(action, "back") && strcmp(action, "restart-character")))
    return FALSE;

  if (!json_object_has_exact_keys(root, keys, sizeof(keys) / sizeof(keys[0])))
  {
    web_onboarding_set_error(d, WEB_ONBOARDING_ERROR_WORKFLOW_INVALID);
    return TRUE;
  }

  flow_id = json_required_string(root, "flowId", WEB_ONBOARDING_EDITOR_MAX_ID_BYTES);
  if (!json_required_integer(root, "v", WEB_ONBOARDING_PROTOCOL_VERSION_MAX,
                             WEB_ONBOARDING_PROTOCOL_VERSION_MAX, &version) ||
      !json_required_integer(root, "revision", 0, 1000000, &revision) ||
      !editor_id_is_valid(flow_id))
  {
    web_onboarding_set_error(d, WEB_ONBOARDING_ERROR_WORKFLOW_INVALID);
    return TRUE;
  }
  (void)version;

  onboarding_flow_id(d, expected_flow_id, sizeof(expected_flow_id));
  if (strcmp(flow_id, expected_flow_id) || revision != d->web_onboarding_revision)
  {
    web_onboarding_set_error(d, WEB_ONBOARDING_ERROR_WORKFLOW_STALE);
    return TRUE;
  }

  if (!strcmp(action, "back"))
  {
    if (!character_creation_can_back(d))
    {
      web_onboarding_set_error(d, WEB_ONBOARDING_ERROR_WORKFLOW_UNAVAILABLE);
      return TRUE;
    }

    clear_editor_transfers(d);
    if (!character_creation_back(d))
      web_onboarding_set_error(d, WEB_ONBOARDING_ERROR_WORKFLOW_UNAVAILABLE);
    else
    {
      d->web_onboarding_error = WEB_ONBOARDING_ERROR_NONE;
      web_onboarding_mark_dirty(d);
    }
    return TRUE;
  }

  if (!character_creation_can_restart(d))
  {
    web_onboarding_set_error(d, WEB_ONBOARDING_ERROR_WORKFLOW_UNAVAILABLE);
    return TRUE;
  }

  clear_editor_transfers(d);
  if (character_creation_restart(d) != CHARACTER_CREATION_RESTART_OK)
    web_onboarding_set_error(d, WEB_ONBOARDING_ERROR_RESTART_FAILED);
  else
  {
    d->web_onboarding_error = WEB_ONBOARDING_ERROR_NONE;
    web_onboarding_mark_dirty(d);
  }
  return TRUE;
}

#ifdef WEB_ONBOARDING_FOCUSED_PROTOCOL_HARNESS
__attribute__((unused))
#endif
static void
handle_editor_action_at(struct descriptor_data *d, const char *payload, int64_t now_ms)
{
  json_object *root = NULL;
  json_object *phase_object = NULL;
  const char *phase = NULL;
  bool handled = FALSE;

  if (d == NULL)
    return;

  if (!web_onboarding_v2_enabled(d))
  {
    clear_editor_transfers(d);
    return;
  }

  root = parse_editor_envelope(payload);
  if (root == NULL)
  {
    reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER);
    return;
  }

  if (handle_workflow_action(d, root))
  {
    json_object_put(root);
    return;
  }

  if (json_object_object_get_ex(root, "phase", &phase_object) &&
      json_object_is_type(phase_object, json_type_string))
    phase = json_object_get_string(phase_object);

  if (phase != NULL && !strcmp(phase, "begin"))
    handled = begin_editor_transfer(d, root, now_ms);
  else if (phase != NULL && !strcmp(phase, "chunk"))
    handled = accept_editor_chunk(d, root, now_ms);
  else if (phase != NULL && !strcmp(phase, "commit"))
    handled = commit_editor_transfer(d, root, now_ms);
  else if (phase != NULL && !strcmp(phase, "cancel"))
    handled = cancel_editor(d, root);

  json_object_put(root);
  if (!handled)
    reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER);
}

void web_onboarding_handle_action(struct descriptor_data *d, const char *payload)
{
#ifdef WEB_ONBOARDING_FOCUSED_PROTOCOL_HARNESS
  /*
   * The focused Telnet parser harness links only protocol.c and this adapter.
   * Record exact dispatch without pulling the character database, JSON, and
   * crypto dependency graph into that deliberately tiny executable.
   */
  if (d != NULL && payload != NULL)
  {
    d->web_onboarding_revision = (int)strlen(payload);
    d->web_onboarding_dirty = TRUE;
  }
#else
  handle_editor_action_at(d, payload, onboarding_now_ms());
#endif
}

#ifdef LUMINARI_CUTEST
void web_onboarding_handle_action_at_for_test(struct descriptor_data *d, const char *payload,
                                              int64_t now_ms)
{
  handle_editor_action_at(d, payload, now_ms);
}

bool web_onboarding_has_active_transfer_for_test(struct descriptor_data *d)
{
  return d != NULL && d->web_onboarding_session != NULL &&
         d->web_onboarding_session->inbound.active;
}

bool web_onboarding_has_active_outbound_transfer_for_test(struct descriptor_data *d)
{
  return d != NULL && d->web_onboarding_session != NULL &&
         d->web_onboarding_session->outbound.active;
}
#endif

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

/*
 * Find the presentation for a state, but only if the client negotiated a
 * version that understands it.
 *
 * A v1 client asking about a role-play state gets NULL, which is exactly the
 * pre-existing behaviour: no structured presentation, so the flow hands off to
 * the classic terminal. This is what keeps v2 screens from ever reaching a
 * client that could not render them.
 */
static const struct onboarding_screen_info *find_screen_for_version(int state, int version)
{
  size_t i = 0;

  for (i = 0; i < sizeof(onboarding_screens) / sizeof(onboarding_screens[0]); i++)
  {
    if (onboarding_screens[i].state != state)
      continue;

    if (onboarding_screens[i].min_version > version)
      return NULL;

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

static void build_content_metadata(struct json_writer *w, const char *content_id,
                                   const char *canon_version, const char *provenance)
{
  if (content_id != NULL)
  {
    json_raw(w, ",");
    json_field_string(w, "contentId", content_id, 80);
  }
  if (canon_version != NULL)
  {
    json_raw(w, ",");
    json_field_string(w, "canonVersion", canon_version, 64);
  }
  if (provenance != NULL)
  {
    json_raw(w, ",");
    json_field_string(w, "provenance", provenance, 240);
  }
}

static int catalog_total_items(struct descriptor_data *d, int state);
static void catalog_page_bounds(struct descriptor_data *d, int state, int total, int *start,
                                int *end, int *page_index, int *page_count);

static bool race_is_selectable(struct descriptor_data *d, int race)
{
  if (d == NULL || d->character == NULL || race < 0 || race >= NUM_RACES)
    return FALSE;
  if (!race_list[race].is_pc)
    return FALSE;
  return !is_locked_race(race) || has_unlocked_race(d->character, race);
}

/* Races the server would actually accept right now, using the same lock and
 * playable checks that nanny() uses. */
static void build_race_choices(struct json_writer *w, struct descriptor_data *d)
{
  bool detailed = web_onboarding_v2_enabled(d);
  int total = catalog_total_items(d, CON_QRACE);
  int start = 0;
  int end = total;
  int page_index = 0;
  int page_count = 0;
  int position = 0;
  int race = 0;
  bool first = TRUE;

  if (total <= 0)
    return;

  if (detailed)
  {
    catalog_page_bounds(d, CON_QRACE, total, &start, &end, &page_index, &page_count);
    (void)page_index;
    (void)page_count;
  }

  for (race = 0; race < NUM_RACES; race++)
  {
    if (!race_is_selectable(d, race))
      continue;

    if (position < start)
    {
      position++;
      continue;
    }
    if (position >= end)
      break;
    position++;

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
    if (detailed)
    {
      json_raw(w, ",");
      json_field_string_truncated(w, "description", race_list[race].descrip, 900);
      json_raw(w, ",");
      json_field_bool(w, "inspectable", TRUE);
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

      json_raw(w, "]");
    }

    json_raw(w, "}");

    if (w->overflow)
      return;
  }
}

static bool class_is_selectable(struct descriptor_data *d, int chclass)
{
  if (d == NULL || d->character == NULL || chclass < 0 || chclass >= NUM_CLASSES)
    return FALSE;
  if (!CLSLIST_INGAME(chclass) || CLSLIST_PRESTIGE(chclass))
    return FALSE;
  if (CLSLIST_LOCK(chclass) && !has_unlocked_class(d->character, chclass))
    return FALSE;
  return valid_class_race_alignment(chclass, GET_REAL_RACE(d->character));
}

/* Classes valid for the already-chosen race, using the same checks as
 * nanny()'s CON_QCLASS handler. */
static void build_class_choices(struct json_writer *w, struct descriptor_data *d)
{
  bool detailed = web_onboarding_v2_enabled(d);
  int total = catalog_total_items(d, CON_QCLASS);
  int start = 0;
  int end = total;
  int page_index = 0;
  int page_count = 0;
  int position = 0;
  int chclass = 0;
  bool first = TRUE;

  if (total <= 0)
    return;

  if (detailed)
  {
    catalog_page_bounds(d, CON_QCLASS, total, &start, &end, &page_index, &page_count);
    (void)page_index;
    (void)page_count;
  }

  for (chclass = 0; chclass < NUM_CLASSES; chclass++)
  {
    if (!class_is_selectable(d, chclass))
      continue;

    if (position < start)
    {
      position++;
      continue;
    }
    if (position >= end)
      break;
    position++;

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
    if (detailed)
    {
      json_raw(w, ",");
      json_field_string_truncated(w, "description", CLSLIST_DESCRIP(chclass), 900);
      json_raw(w, ",");
      json_field_bool(w, "inspectable", TRUE);
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
    }
    else
      json_raw(w, "}");

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
    char wire[16];

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
    char wire[16];
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
          classes_len = snprintf_append(classes, sizeof(classes), classes_len, "/");
        classes_len =
            snprintf_append(classes, sizeof(classes), classes_len, "%s", CLSLIST_NAME(class_index));
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

static const struct onboarding_error_info *find_onboarding_error(int error)
{
  size_t index = 0;

  for (index = 0; (size_t)index < sizeof(onboarding_errors) / sizeof(onboarding_errors[0]); index++)
    if ((int)onboarding_errors[index].error == error)
      return &onboarding_errors[index];

  return NULL;
}

static void build_error(struct json_writer *w, int error)
{
  const struct onboarding_error_info *info = find_onboarding_error(error);

  if (info == NULL)
    return;

  json_raw(w, ",\"error\":{");
  json_field_string(w, "code", info->code, 40);
  json_raw(w, ",");
  json_field_string(w, "message", info->message, 160);
  json_raw(w, ",");
  json_field_string(w, "field", info->field, 40);
  json_raw(w, "}");
}

/* --------------------------------------------------------------------- */
/* Protocol v2: role-play profile hub                                     */
/* --------------------------------------------------------------------- */

/*
 * One hub item as the source sees it.
 *
 * `wire` is the exact menu value the existing CON_CHAR_RP_MENU parser already
 * accepts, so opening an item drives the unchanged terminal handler rather
 * than a second, parallel code path.
 */
struct rp_hub_item
{
  const char *id;
  const char *label;
  const char *description;
  /*
   * Exact value CON_CHAR_RP_MENU's parser accepts. Taken from the switch in
   * nanny(), not invented: opening an item must drive the unchanged terminal
   * handler rather than a parallel code path.
   */
  const char *wire;
  const char *media_key;
};

static const struct rp_hub_item rp_hub_items[] = {
    {"profile/short-description", "Short description",
     "A compact first-glance phrase shown when others encounter your character.", "0",
     "roleplay/short-description"},
    {"profile/long-description", "Long description",
     "What another character can observe when they look at yours more closely.", "1",
     "roleplay/long-description"},
    {"profile/background-story", "Background story",
     "The formative events and choices that brought your character to the present.", "2",
     "roleplay/background-story"},
    {"profile/background", "Background",
     "A permanent life archetype that grants skills and a special ability.", "3",
     "background/fallback"},
    {"profile/goals", "Goals", NULL, "4", "roleplay/goals"},
    {"profile/personality", "Personality", NULL, "5", "roleplay/personality"},
    {"profile/ideals", "Ideals", NULL, "6", "roleplay/ideals"},
    {"profile/bonds", "Bonds", NULL, "7", "roleplay/bonds"},
    {"profile/flaws", "Flaws", NULL, "8", "roleplay/flaws"},
    {"profile/age", "Age",
     "The character's stage of life and its source-defined ability adjustments.", "9",
     "roleplay/profile-hub"},
    {"profile/region", "Homeland",
     "The formative origin jurisdiction whose culture and Heart Tongue shaped them.", "a",
     "region/fallback"},
    {"profile/faction", "Faction",
     "An organization the character joins, with allies, duties, and enemies.", "b",
     "faction/fallback"},
    {"profile/hometown", "Hometown",
     "The city tied to recall, donation services, and hometown-dependent abilities.", "c",
     "hometown/fallback"},
    {"profile/deity", "Deity",
     "The divine power the character follows-or an explicit choice to follow none.", "d",
     "deity/fallback"},
};

/*
 * Whether a stored string counts as set.
 *
 * The source uses both NULL and empty strings for "unset" depending on the
 * field, so both are treated the same rather than trusting one convention.
 */
static bool rp_text_is_set(const char *value)
{
  return value != NULL && *value != '\0';
}

/* Current value of a hub item's backing text field, or NULL when unset. */
static const char *rp_item_text(struct char_data *ch, const char *id)
{
  if (ch == NULL)
    return NULL;

  if (!strcmp(id, "profile/long-description"))
    return ch->player.description;
  if (!strcmp(id, "profile/background-story"))
    return ch->player.background;
  if (!strcmp(id, "profile/goals"))
    return ch->player.goals;
  if (!strcmp(id, "profile/personality"))
    return ch->player.personality;
  if (!strcmp(id, "profile/ideals"))
    return ch->player.ideals;
  if (!strcmp(id, "profile/bonds"))
    return ch->player.bonds;
  if (!strcmp(id, "profile/flaws"))
    return ch->player.flaws;

  return NULL;
}

/*
 * Server-authored status for one hub item, and a safe short label for its
 * current value.
 *
 * Every rule here mirrors the guard the terminal handler already applies in
 * nanny(). None of it is re-derived by the browser, and none of it may be
 * more permissive than the terminal: if the handler would refuse to open an
 * item, the hub must not report it as mutable.
 */
static const char *rp_item_status(struct char_data *ch, const char *id, const char **reason,
                                  const char **summary)
{
  *reason = NULL;
  *summary = NULL;

  if (ch == NULL)
    return "unset";

  /* Short description is generated from descriptor/adjective selections. */
  if (!strcmp(id, "profile/short-description"))
    return (GET_PC_DESCRIPTOR_1(ch) > 0 && GET_PC_ADJECTIVE_1(ch) > 0) ? "configured" : "unset";

  if (!strcmp(id, "profile/background"))
  {
    if (GET_BACKGROUND(ch) > 0 && GET_BACKGROUND(ch) < NUM_BACKGROUNDS)
    {
      *summary = background_list[GET_BACKGROUND(ch)].name;
      *reason = "A background is chosen once and cannot be changed here.";
      return "locked";
    }
    return "unset";
  }

  if (!strcmp(id, "profile/age"))
  {
    /* Both guards are taken verbatim from the CON_CHAR_RP_MENU handler. */
    if (IS_DRACONIAN(ch))
    {
      *reason = "Draconians cannot change their age, and are always considered adult.";
      return "unavailable";
    }
    if (ch->player_specials->saved.character_age_saved)
    {
      *reason = "Age is chosen once. Contact a staff member to change it.";
      return "locked";
    }
    return "unset";
  }

  if (!strcmp(id, "profile/faction"))
  {
    if (GET_CLAN(ch) > 0)
    {
      *reason = "You have already chosen a faction. You can resign in game.";
      return "locked";
    }
    return "unset";
  }

  if (!strcmp(id, "profile/hometown"))
  {
    if (GET_HOMETOWN(ch))
    {
      *reason = "A hometown is chosen once. Contact a staff member to change it.";
      return "locked";
    }
    return "unset";
  }

  if (!strcmp(id, "profile/deity"))
  {
    if (GET_DEITY(ch))
    {
      *reason = "A deity is chosen once. Contact a staff member to change it.";
      return "locked";
    }
    return "unset";
  }

  if (!strcmp(id, "profile/region"))
  {
    if (GET_REGION(ch) > REGION_NONE && GET_REGION(ch) < NUM_REGIONS)
    {
      *summary = regions[GET_REGION(ch)];
      *reason = "A homeland is chosen once. Contact a staff member to change it.";
      return "locked";
    }
    return "unset";
  }

  return rp_text_is_set(rp_item_text(ch, id)) ? "configured" : "unset";
}

/*
 * Emit the hub summary.
 *
 * Deliberately metadata only: whether a field is set, and a short safe label
 * where one exists. The bodies of the private text fields never appear here,
 * both to stay inside the 15,000-byte document cap and because the hub is
 * re-sent on every revision.
 */
static void build_profile_items(struct json_writer *w, struct descriptor_data *d)
{
  struct char_data *ch = d->character;
  size_t i = 0;

  json_raw(w, "\"profile\":[");

  for (i = 0; i < sizeof(rp_hub_items) / sizeof(rp_hub_items[0]); i++)
  {
    const struct rp_hub_item *item = &rp_hub_items[i];
    const struct character_creation_guidance *guidance =
        character_creation_guidance_for_profile(item->id);
    const char *description = guidance != NULL ? guidance->hub_summary : item->description;
    const char *reason = NULL;
    const char *summary = NULL;
    const char *status = rp_item_status(ch, item->id, &reason, &summary);
    bool is_open = !strcmp(status, "unset") || !strcmp(status, "configured");

    if (i > 0)
      json_raw(w, ",");

    json_raw(w, "{");
    json_field_string(w, "id", item->id, 64);
    json_raw(w, ",");
    json_field_string(w, "label", item->label, 64);
    json_raw(w, ",");
    json_field_string(w, "wireValue", item->wire, 8);
    json_raw(w, ",");
    json_field_string(w, "status", status, 16);
    json_raw(w, ",");
    json_field_bool(w, "mutable", is_open);
    json_raw(w, ",");
    json_field_bool(w, "required", FALSE);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", item->media_key, 64);
    if (description != NULL)
    {
      json_raw(w, ",");
      json_field_string(w, "description", description, 240);
    }
    if (summary != NULL)
    {
      json_raw(w, ",");
      json_field_string(w, "summary", summary, 100);
    }
    if (reason != NULL)
    {
      json_raw(w, ",");
      json_field_string(w, "reason", reason, 200);
    }
    json_raw(w, "}");
  }

  json_raw(w, "],");
}

/*
 * Background archetype catalog.
 *
 * Stable IDs, action values, and media keys come from backgrounds.c, where
 * they are keyed by the persistent background constant and shared with the
 * terminal parser. Index 0 is skipped because it is BACKGROUND_NONE.
 */
#define WEB_ONBOARDING_CATALOG_PAGE_SIZE 12

static int catalog_page_size(int state)
{
  if (state == CON_QRACE || state == CON_QCLASS)
    return 6;
  if (state == CON_QREGION)
    return 3;
  if (state == CON_BACKGROUND_ARCHTYPE)
    return 6;
  return WEB_ONBOARDING_CATALOG_PAGE_SIZE;
}

static void choice_separator(struct json_writer *w, bool *first)
{
  if (!*first)
    json_raw(w, ",");
  *first = FALSE;
}

static int selectable_region_count(void)
{
  int region = 0;
  int count = 0;

  for (region = 1; region < NUM_REGIONS; region++)
    if (is_selectable_region(region))
      count++;
  return count;
}

static int selectable_hometown_count(void)
{
  int hometown = 0;
  int count = 0;

  for (hometown = 1; hometown < NUM_CITIES; hometown++)
    if (roleplay_hometown_is_selectable(hometown))
      count++;
  return count;
}

static int selectable_deity_count(void)
{
  int deity = 0;
  int count = 1; /* Explicit no-deity choice. */

  for (deity = 1; deity < NUM_DEITIES; deity++)
    if (deity_list[deity].pantheon != DEITY_PANTHEON_NONE)
      count++;
  return count;
}

static int selectable_race_count(struct descriptor_data *d)
{
  int race = 0;
  int count = 0;

  for (race = 0; race < NUM_RACES; race++)
    if (race_is_selectable(d, race))
      count++;
  return count;
}

static int selectable_class_count(struct descriptor_data *d)
{
  int chclass = 0;
  int count = 0;

  for (chclass = 0; chclass < NUM_CLASSES; chclass++)
    if (class_is_selectable(d, chclass))
      count++;
  return count;
}

static int catalog_total_items(struct descriptor_data *d, int state)
{
  switch (state)
  {
  case CON_QRACE:
    return selectable_race_count(d);
  case CON_QCLASS:
    return selectable_class_count(d);
  case CON_BACKGROUND_ARCHTYPE:
    return NUM_BACKGROUNDS - 1;
  case CON_CHARACTER_AGE_SELECT:
    return NUM_CHARACTER_AGES;
  case CON_QREGION:
    return selectable_region_count();
  case CON_CHARACTER_FACTION_SELECT:
    return num_of_clans + 1;
  case CON_CHARACTER_HOMETOWN_SELECT:
    return selectable_hometown_count();
  case CON_CHARACTER_DEITY_SELECT:
    return selectable_deity_count();
  default:
    return 0;
  }
}

static bool catalog_allows_cancel(int state)
{
  return state != CON_QRACE && state != CON_QCLASS;
}

static bool catalog_uses_pagination(struct descriptor_data *d, int state)
{
  if (state == CON_QRACE || state == CON_QCLASS)
    return web_onboarding_v2_enabled(d);
  return TRUE;
}

static void catalog_page_bounds(struct descriptor_data *d, int state, int total, int *start,
                                int *end, int *page_index, int *page_count)
{
  struct web_onboarding_session *session = ensure_onboarding_session(d);
  int page_size = catalog_page_size(state);
  int count = MAX(1, (total + page_size - 1) / page_size);
  int page = 0;

  if (session != NULL)
  {
    if (session->catalog_state != state)
    {
      session->catalog_state = state;
      session->catalog_page = 0;
    }
    if (session->catalog_page >= count)
      session->catalog_page = count - 1;
    if (session->catalog_page < 0)
      session->catalog_page = 0;
    page = session->catalog_page;
  }

  *start = page * page_size;
  *end = MIN(total, *start + page_size);
  *page_index = page;
  *page_count = count;
}

static void build_page(struct json_writer *w, struct descriptor_data *d, int state)
{
  int total = catalog_total_items(d, state);
  int start = 0;
  int end = 0;
  int page_index = 0;
  int page_count = 0;

  if (total <= 0 || !catalog_uses_pagination(d, state))
    return;
  catalog_page_bounds(d, state, total, &start, &end, &page_index, &page_count);
  (void)start;
  (void)end;

  json_raw(w, ",\"page\":{");
  json_field_number(w, "index", page_index);
  json_raw(w, ",");
  json_field_number(w, "count", page_count);
  json_raw(w, ",");
  json_field_number(w, "totalItems", total);
  json_raw(w, "}");
}

static void build_background_choices(struct json_writer *w, struct descriptor_data *d, bool paged)
{
  int total = NUM_BACKGROUNDS - 1;
  int start = 0;
  int end = total;
  int page_index = 0;
  int page_count = 1;
  int position = 0;
  bool first = TRUE;

  if (paged)
    catalog_page_bounds(d, CON_BACKGROUND_ARCHTYPE, total, &start, &end, &page_index, &page_count);
  (void)page_index;
  (void)page_count;

  for (position = start + 1; position <= end; position++)
  {
    int background = backgrounds_listed_alphabetically[position];
    int feat = BACKGROUND_NONE;
    char skill_bonuses[200];
    const struct character_creation_background *content =
        character_creation_background_for_value(background);

    if (background <= BACKGROUND_NONE || background >= NUM_BACKGROUNDS ||
        background_list[background].name == NULL)
      continue;
    feat = background_list[background].feat;

    choice_separator(w, &first);
    json_raw(w, "{");
    json_field_string(w, "id", background_stable_id(background), 64);
    json_raw(w, ",");
    json_field_string(w, "label", background_list[background].name, 64);
    json_raw(w, ",");
    json_field_string(w, "wireValue", background_wire_value(background), 64);
    json_raw(w, ",");
    json_field_bool(w, "enabled", TRUE);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", background_media_key(background), 64);
    if (content != NULL && content->story_promise != NULL)
    {
      json_raw(w, ",");
      json_field_string(w, "summary", content->story_promise, 240);
    }
    if (background_list[background].desc != NULL)
    {
      json_raw(w, ",");
      json_field_string_truncated(w, "description", background_list[background].desc, 900);
    }
    json_raw(w, ",");
    json_field_bool(w, "inspectable", TRUE);
    build_content_metadata(w, content != NULL ? content->content_id : NULL,
                           content != NULL ? CHARACTER_CREATION_COMPASS_CANON_VERSION : NULL,
                           content != NULL ? character_creation_content_provenance() : NULL);

    snprintf(skill_bonuses, sizeof(skill_bonuses), "%s +2, %s +2",
             ability_names[background_list[background].skills[0]],
             ability_names[background_list[background].skills[1]]);
    json_raw(w, ",\"facts\":[{");
    json_field_string(w, "label", "Skill bonuses", 40);
    json_raw(w, ",");
    json_field_string(w, "value", skill_bonuses, 200);
    json_raw(w, "}");
    if (feat > 0 && feat < NUM_FEATS && feat_list[feat].name != NULL)
    {
      json_raw(w, ",{");
      json_field_string(w, "label", "Special ability", 40);
      json_raw(w, ",");
      json_field_string(w, "value", feat_list[feat].name, 120);
      json_raw(w, "},{");
      json_field_string(w, "label", "Ability effect", 40);
      json_raw(w, ",");
      json_field_string_truncated(w, "value", feat_list[feat].description, 400);
      json_raw(w, "}");
    }
    json_raw(w, "]}");
  }
}

/*
 * The four background-themed idea menus use alphabetic list positions, not
 * the background parser's stable name tokens. Preserve that legacy wire
 * contract while still exposing stable IDs and media keys to the browser.
 */
static const char *inspiration_profile_for_state(int state)
{
  switch (state)
  {
  case CON_CHARACTER_PERSONALITY_IDEAS:
    return "profile/personality";
  case CON_CHARACTER_IDEALS_IDEAS:
    return "profile/ideals";
  case CON_CHARACTER_BONDS_IDEAS:
    return "profile/bonds";
  case CON_CHARACTER_FLAWS_IDEAS:
    return "profile/flaws";
  default:
    return NULL;
  }
}

static enum character_creation_inspiration_kind inspiration_kind_for_state(int state)
{
  switch (state)
  {
  case CON_CHARACTER_IDEALS_IDEAS:
    return CHARACTER_CREATION_INSPIRATION_IDEAL;
  case CON_CHARACTER_BONDS_IDEAS:
    return CHARACTER_CREATION_INSPIRATION_BOND;
  case CON_CHARACTER_FLAWS_IDEAS:
    return CHARACTER_CREATION_INSPIRATION_FLAW;
  case CON_CHARACTER_PERSONALITY_IDEAS:
  default:
    return CHARACTER_CREATION_INSPIRATION_PERSONALITY;
  }
}

static void build_idea_background_choices(struct json_writer *w, int state)
{
  int position = 0;
  bool first = TRUE;
  char wire[16];
  enum character_creation_inspiration_kind kind = inspiration_kind_for_state(state);
  const struct character_creation_guidance *guidance =
      character_creation_guidance_for_profile(inspiration_profile_for_state(state));

  for (position = 1; position < NUM_BACKGROUNDS; position++)
  {
    int background = backgrounds_listed_alphabetically[position];
    char description[720];
    const struct character_creation_background *content =
        character_creation_background_for_value(background);
    const char *first_seed = character_creation_inspiration_seed(background, kind, 0);
    const char *second_seed = character_creation_inspiration_seed(background, kind, 1);

    if (background <= BACKGROUND_NONE || background >= NUM_BACKGROUNDS ||
        background_list[background].name == NULL)
      continue;

    description[0] = '\0';
    if (content != NULL && first_seed != NULL && second_seed != NULL)
      snprintf(description, sizeof(description),
               "This theme can inspire suggestions such as: \"%s\" or \"%s\"", first_seed,
               second_seed);
    snprintf(wire, sizeof(wire), "%d", position);
    choice_separator(w, &first);
    json_raw(w, "{");
    json_field_string(w, "id", background_stable_id(background), 64);
    json_raw(w, ",");
    json_field_string(w, "label", background_list[background].name, 64);
    json_raw(w, ",");
    json_field_string(w, "wireValue", wire, 16);
    json_raw(w, ",");
    json_field_bool(w, "enabled", TRUE);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", background_media_key(background), 64);
    if (content != NULL && content->story_promise != NULL)
    {
      json_raw(w, ",");
      json_field_string(w, "summary", content->story_promise, 240);
    }
    else if (background_list[background].desc != NULL)
    {
      json_raw(w, ",");
      json_field_string_truncated(w, "summary", background_list[background].desc, 400);
    }
    if (description[0] != '\0')
    {
      json_raw(w, ",");
      json_field_string_truncated(w, "description", description, 700);
    }
    json_raw(w, ",");
    json_field_bool(w, "inspectable",
                    description[0] != '\0' || background_list[background].desc != NULL);
    if (content != NULL)
      build_content_metadata(w, content->content_id, CHARACTER_CREATION_COMPASS_CANON_VERSION,
                             character_creation_content_provenance());
    if (guidance != NULL)
    {
      json_raw(w, ",\"facts\":[{");
      json_field_string(w, "label", "Suggestion shape", 40);
      json_raw(w, ",");
      json_field_string(w, "value", guidance->generator_shape, 400);
      json_raw(w, "}]");
    }
    json_raw(w, "}");
  }
}

static void build_short_feature_choices(struct json_writer *w, struct descriptor_data *d)
{
  int feature = 0;
  bool first = TRUE;
  char id[64];
  char wire[16];

  for (feature = 1; feature <= NUM_FEATURE_TYPES; feature++)
  {
    bool enabled = d->character != NULL && short_desc_feature_allowed(d->character, feature);

    snprintf(id, sizeof(id), "feature/%s", short_desc_feature_id(feature));
    snprintf(wire, sizeof(wire), "%d", feature);
    choice_separator(w, &first);
    json_raw(w, "{");
    json_field_string(w, "id", id, 64);
    json_raw(w, ",");
    json_field_string(w, "label", short_desc_feature_label(feature), 64);
    json_raw(w, ",");
    json_field_string(w, "wireValue", wire, 16);
    json_raw(w, ",");
    json_field_bool(w, "enabled", enabled);
    if (!enabled)
    {
      json_raw(w, ",");
      json_field_string(w, "lockReason", "This feature is not available to your ancestry.", 100);
    }
    json_raw(w, "}");
  }
}

static void build_short_adjective_choices(struct json_writer *w, int feature)
{
  int adjective = 0;
  int count = short_desc_adjective_count(feature);
  bool first = TRUE;
  char id[80];
  char wire[16];

  for (adjective = 1; adjective <= count; adjective++)
  {
    const char *label = short_desc_adjective_label(feature, adjective);

    if (label == NULL)
      continue;
    snprintf(id, sizeof(id), "adjective/%s/%d", short_desc_feature_id(feature), adjective);
    snprintf(wire, sizeof(wire), "%d", adjective);
    choice_separator(w, &first);
    json_raw(w, "{");
    json_field_string(w, "id", id, 80);
    json_raw(w, ",");
    json_field_string(w, "label", label, 100);
    json_raw(w, ",");
    json_field_string(w, "wireValue", wire, 16);
    json_raw(w, ",");
    json_field_bool(w, "enabled", TRUE);
    json_raw(w, "}");
  }
}

static void build_age_choices(struct json_writer *w, struct descriptor_data *d)
{
  int start = 0;
  int end = 0;
  int page_index = 0;
  int page_count = 0;
  int age = 0;
  bool first = TRUE;
  char wire[16];

  catalog_page_bounds(d, CON_CHARACTER_AGE_SELECT, NUM_CHARACTER_AGES, &start, &end, &page_index,
                      &page_count);
  (void)page_index;
  (void)page_count;

  for (age = start; age < end; age++)
  {
    int ability = 0;
    bool first_fact = TRUE;
    char modifier[24];

    snprintf(wire, sizeof(wire), "%d", age + 1);
    choice_separator(w, &first);
    json_raw(w, "{");
    json_field_string(w, "id", roleplay_age_stable_id(age), 64);
    json_raw(w, ",");
    json_field_string(w, "label", character_ages[age], 64);
    json_raw(w, ",");
    json_field_string(w, "wireValue", wire, 16);
    json_raw(w, ",");
    json_field_bool(w, "enabled", TRUE);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", roleplay_age_media_key(age), 64);
    json_raw(w, ",");
    json_field_bool(w, "inspectable", TRUE);
    json_raw(w, ",\"facts\":[");
    for (ability = 0; ability < 6; ability++)
    {
      if (character_age_attributes[age][ability] == 0)
        continue;
      choice_separator(w, &first_fact);
      snprintf(modifier, sizeof(modifier), "%+d", character_age_attributes[age][ability]);
      json_raw(w, "{");
      json_field_string(w, "label", ability_score_names[ability], 32);
      json_raw(w, ",");
      json_field_string(w, "value", modifier, 24);
      json_raw(w, "}");
    }
    json_raw(w, "]}");
  }
}

static void build_region_choices(struct json_writer *w, struct descriptor_data *d)
{
  int total = selectable_region_count();
  int start = 0;
  int end = 0;
  int page_index = 0;
  int page_count = 0;
  int region = 0;
  int position = 0;
  bool first = TRUE;
  char id[64];
  char wire[16];

  catalog_page_bounds(d, CON_QREGION, total, &start, &end, &page_index, &page_count);
  (void)page_index;
  (void)page_count;

  for (region = 1; region < NUM_REGIONS; region++)
  {
    const struct character_creation_homeland *content = NULL;

    if (!is_selectable_region(region))
      continue;
    if (position++ < start)
      continue;
    if (position > end)
      break;

    content = character_creation_homeland_for_region(region);
    roleplay_region_stable_id(region, id, sizeof(id));
    snprintf(wire, sizeof(wire), "%d", region);
    choice_separator(w, &first);
    json_raw(w, "{");
    json_field_string(w, "id", id, 64);
    json_raw(w, ",");
    json_field_string(w, "label", regions[region], 80);
    json_raw(w, ",");
    json_field_string(w, "wireValue", wire, 16);
    json_raw(w, ",");
    json_field_bool(w, "enabled", TRUE);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", roleplay_region_media_key(region), 64);
    if (content != NULL)
    {
      json_raw(w, ",");
      json_field_string(w, "summary", content->summary, 300);
      json_raw(w, ",");
      json_field_string_truncated(w, "description", content->description, 1600);
      json_raw(w, ",");
      json_field_bool(w, "inspectable", TRUE);
      build_content_metadata(w, content->content_id, CHARACTER_CREATION_HOMELAND_CANON_VERSION,
                             content->provenance);
    }
    json_raw(w, ",\"facts\":[{");
    json_field_string(w, "label", "Language", 32);
    json_raw(w, ",");
    json_field_string(w, "value", get_region_language_name(region), 80);
    if (content != NULL)
    {
      json_raw(w, "},{");
      json_field_string(w, "label", "Place kind", 32);
      json_raw(w, ",");
      json_field_string(w, "value", content->place_kind, 80);
      json_raw(w, "},{");
      json_field_string(w, "label", "Geographic parent", 40);
      json_raw(w, ",");
      json_field_string(w, "value", content->geographic_parent, 100);
      json_raw(w, "},{");
      json_field_string(w, "label", "Political sphere", 40);
      json_raw(w, ",");
      json_field_string(w, "value", content->political_sphere, 100);
    }
    json_raw(w, "}]}");
  }
}

static void build_faction_choices(struct json_writer *w, struct descriptor_data *d)
{
  int total = num_of_clans + 1;
  int start = 0;
  int end = 0;
  int page_index = 0;
  int page_count = 0;
  int position = 0;
  bool first = TRUE;
  char id[64];
  char wire[16];

  catalog_page_bounds(d, CON_CHARACTER_FACTION_SELECT, total, &start, &end, &page_index,
                      &page_count);
  (void)page_index;
  (void)page_count;

  for (position = start; position < end; position++)
  {
    int selection = position;
    int faction_vnum = 0;
    clan_rnum clan = NO_CLAN;
    const char *label = "Adventurer / No faction";
    const char *description = "Choose your own allies and enemies without belonging to a faction.";

    if (selection > 0)
    {
      clan = real_clan(selection);
      if (clan == NO_CLAN)
        continue;
      faction_vnum = clan_list[clan].vnum;
      label = CLAN_NAME(clan);
      description = clan_list[clan].description;
    }

    roleplay_faction_stable_id(faction_vnum, id, sizeof(id));
    snprintf(wire, sizeof(wire), "%d", selection);
    choice_separator(w, &first);
    json_raw(w, "{");
    json_field_string(w, "id", id, 64);
    json_raw(w, ",");
    json_field_string(w, "label", label, 100);
    json_raw(w, ",");
    json_field_string(w, "wireValue", wire, 16);
    json_raw(w, ",");
    json_field_bool(w, "enabled", TRUE);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", roleplay_faction_media_key(faction_vnum), 64);
    if (description != NULL)
    {
      json_raw(w, ",");
      json_field_string_truncated(w, "description", description, 900);
      json_raw(w, ",");
      json_field_bool(w, "inspectable", TRUE);
    }
    json_raw(w, "}");
  }
}

static void build_hometown_choices(struct json_writer *w, struct descriptor_data *d)
{
  int total = selectable_hometown_count();
  int start = 0;
  int end = 0;
  int page_index = 0;
  int page_count = 0;
  int hometown = 0;
  int position = 0;
  bool first = TRUE;
  char id[64];
  char wire[16];

  catalog_page_bounds(d, CON_CHARACTER_HOMETOWN_SELECT, total, &start, &end, &page_index,
                      &page_count);
  (void)page_index;
  (void)page_count;

  for (hometown = 1; hometown < NUM_CITIES; hometown++)
  {
    const char *summary = NULL;
    const char *description = NULL;

    if (!roleplay_hometown_is_selectable(hometown))
      continue;
    if (position++ < start)
      continue;
    if (position > end)
      break;

    summary = character_creation_hometown_summary(hometown);
    description = character_creation_hometown_description(hometown);
    roleplay_hometown_stable_id(hometown, id, sizeof(id));
    snprintf(wire, sizeof(wire), "%d", hometown);
    choice_separator(w, &first);
    json_raw(w, "{");
    json_field_string(w, "id", id, 64);
    json_raw(w, ",");
    json_field_string(w, "label", cities[hometown], 80);
    json_raw(w, ",");
    json_field_string(w, "wireValue", wire, 16);
    json_raw(w, ",");
    json_field_bool(w, "enabled", TRUE);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", roleplay_hometown_media_key(hometown), 64);
    if (summary != NULL)
    {
      json_raw(w, ",");
      json_field_string(w, "summary", summary, 240);
    }
    if (description != NULL)
    {
      json_raw(w, ",");
      json_field_string_truncated(w, "description", description, 900);
      json_raw(w, ",");
      json_field_bool(w, "inspectable", TRUE);
      build_content_metadata(w, "hometown/ashenport", CHARACTER_CREATION_HOMELAND_CANON_VERSION,
                             character_creation_content_provenance());
    }
    json_raw(w, "}");
  }
}

static void build_deity_choices(struct json_writer *w, struct descriptor_data *d)
{
  int total = selectable_deity_count();
  int start = 0;
  int end = 0;
  int page_index = 0;
  int page_count = 0;
  int deity = 0;
  int position = 0;
  bool first = TRUE;
  char id[64];
  char wire[16];

  catalog_page_bounds(d, CON_CHARACTER_DEITY_SELECT, total, &start, &end, &page_index, &page_count);
  (void)page_index;
  (void)page_count;

  for (deity = 0; deity < NUM_DEITIES; deity++)
  {
    const char *label = "None";
    const char *summary = "Follow no deity.";

    if (deity > 0 && deity_list[deity].pantheon == DEITY_PANTHEON_NONE)
      continue;
    if (position++ < start)
      continue;
    if (position > end)
      break;
    if (deity > 0)
    {
      label = deity_list[deity].name;
      summary = deity_list[deity].portfolio;
    }

    roleplay_deity_stable_id(deity, id, sizeof(id));
    snprintf(wire, sizeof(wire), "%d", deity);
    choice_separator(w, &first);
    json_raw(w, "{");
    json_field_string(w, "id", id, 64);
    json_raw(w, ",");
    json_field_string(w, "label", label, 80);
    json_raw(w, ",");
    json_field_string(w, "wireValue", wire, 16);
    json_raw(w, ",");
    json_field_bool(w, "enabled", TRUE);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", roleplay_deity_media_key(deity), 64);
    if (summary != NULL)
    {
      json_raw(w, ",");
      json_field_string(w, "summary", summary, 200);
      json_raw(w, ",");
      json_field_bool(w, "inspectable", TRUE);
    }
    if (deity > 0 && deity_list[deity].description != NULL)
    {
      json_raw(w, ",");
      json_field_string_truncated(w, "description", deity_list[deity].description, 900);
    }
    json_raw(w, "}");
  }
}

static bool is_idea_state(int state)
{
  return state == CON_CHARACTER_GOALS_IDEAS || state == CON_CHARACTER_PERSONALITY_IDEAS ||
         state == CON_CHARACTER_IDEALS_IDEAS || state == CON_CHARACTER_BONDS_IDEAS ||
         state == CON_CHARACTER_FLAWS_IDEAS;
}

static void build_examples(struct json_writer *w, struct descriptor_data *d, int state)
{
  int index = 0;

  if (!is_idea_state(state) || d->roleplay_pending.example_state != state ||
      d->roleplay_pending.example_count <= 0)
    return;

  json_raw(w, ",\"examples\":[");
  for (index = 0; index < d->roleplay_pending.example_count; index++)
  {
    char id[32];

    if (index > 0)
      json_raw(w, ",");
    snprintf(id, sizeof(id), "example/%d", index);
    json_raw(w, "{");
    json_field_string(w, "id", id, 32);
    json_raw(w, ",");
    json_field_string(w, "text", d->roleplay_pending.examples[index],
                      ROLEPLAY_EXAMPLE_MAX_BYTES - 1);
    json_raw(w, "}");
  }
  json_raw(w, "]");
}

static void build_action_name(struct json_writer *w, bool *first, const char *action)
{
  choice_separator(w, first);
  json_string(w, action, 32);
}

static void build_actions(struct json_writer *w, struct descriptor_data *d,
                          const struct onboarding_screen_info *screen)
{
  bool first = TRUE;
  int total = catalog_total_items(d, screen->state);
  int start = 0;
  int end = 0;
  int page_index = 0;
  int page_count = 0;
  bool paged = catalog_uses_pagination(d, screen->state);

  json_raw(w, "\"actions\":[");

  if (screen->state == CON_CHAR_RP_MENU)
  {
    /* The hub opens items and leaves; it never selects a catalog entry. */
    build_action_name(w, &first, "open");
    build_action_name(w, &first, "quit");
  }
  else if (!strcmp(screen->input_kind, "multiline"))
  {
    /*
     * Editors are driven entirely by the chunked transfer path. `submit` is
     * deliberately absent: profile text must never travel as a command line.
     */
    build_action_name(w, &first, "save-text");
    build_action_name(w, &first, "cancel-editor");
  }
  else if (screen->state == CON_GEN_DESCS_INTRO || screen->state == CON_GEN_DESCS_MENU)
  {
    build_action_name(w, &first, "continue");
    if (screen->state == CON_GEN_DESCS_INTRO && !d->forced_short_desc_setup)
      build_action_name(w, &first, "cancel");
  }
  else if (screen->state == CON_GEN_DESCS_MENU_PARSE)
  {
    build_action_name(w, &first, "confirm");
    build_action_name(w, &first, "reselect");
    if (d->roleplay_pending.short_descriptor_2 == 0)
      build_action_name(w, &first, "continue");
    if (!d->forced_short_desc_setup)
      build_action_name(w, &first, "cancel");
  }
  else if (screen->state == CON_CHARACTER_GOALS_IDEAS)
  {
    build_action_name(w, &first, "generate-example");
    build_action_name(w, &first, "continue");
    build_action_name(w, &first, "cancel");
  }
  else if (screen->state == CON_CHARACTER_PERSONALITY_IDEAS ||
           screen->state == CON_CHARACTER_IDEALS_IDEAS ||
           screen->state == CON_CHARACTER_BONDS_IDEAS || screen->state == CON_CHARACTER_FLAWS_IDEAS)
  {
    build_action_name(w, &first, "select");
    build_action_name(w, &first, "continue");
    build_action_name(w, &first, "cancel");
  }
  else if (total > 0)
  {
    build_action_name(w, &first, "select");
    if (paged)
    {
      catalog_page_bounds(d, screen->state, total, &start, &end, &page_index, &page_count);
      (void)start;
      (void)end;
      if (page_index > 0)
        build_action_name(w, &first, "previous-page");
      if (page_index + 1 < page_count)
        build_action_name(w, &first, "next-page");
    }
    if (catalog_allows_cancel(screen->state))
      build_action_name(w, &first, "cancel");
  }
  else if (!strcmp(screen->input_kind, "confirm"))
  {
    build_action_name(w, &first, "confirm");
    build_action_name(w, &first, "reselect");
  }
  else if (screen->state == CON_ACCOUNT_ADD)
  {
    build_action_name(w, &first, "submit");
    build_action_name(w, &first, "cancel");
  }
  else if (!strcmp(screen->input_kind, "choice"))
    build_action_name(w, &first, "select");
  else
    build_action_name(w, &first, "submit");

  if (screen->state == CON_ACCOUNT_MENU)
  {
    build_action_name(w, &first, "create-character");
    build_action_name(w, &first, "link-character");
    build_action_name(w, &first, "quit");
  }

  if (character_creation_can_back(d))
    build_action_name(w, &first, "back");
  if (character_creation_can_restart(d))
    build_action_name(w, &first, "restart-character");

  build_action_name(w, &first, "classic-terminal");
  json_raw(w, "]");
}

static bool is_short_description_state(int state)
{
  return state == CON_GEN_DESCS_INTRO || state == CON_GEN_DESCS_DESCRIPTORS_1 ||
         state == CON_GEN_DESCS_ADJECTIVES_1 || state == CON_GEN_DESCS_MENU ||
         state == CON_GEN_DESCS_MENU_PARSE || state == CON_GEN_DESCS_DESCRIPTORS_2 ||
         state == CON_GEN_DESCS_ADJECTIVES_2;
}

static void build_short_description_help(struct json_writer *w, struct descriptor_data *d,
                                         int state)
{
  char *description = NULL;

  if (!is_short_description_state(state) || d->character == NULL ||
      !d->roleplay_pending.short_description_active)
    return;

  description = current_short_desc_for_values(
      d->character, d->roleplay_pending.short_descriptor_1, d->roleplay_pending.short_adjective_1,
      d->roleplay_pending.short_descriptor_2, d->roleplay_pending.short_adjective_2);
  if (description != NULL)
  {
    json_raw(w, ",");
    json_field_string(w, "help", description, 500);
    free(description);
  }
}

static const char *guidance_profile_for_state(int state)
{
  switch (state)
  {
  case CON_CHARACTER_GOALS_IDEAS:
  case CON_CHARACTER_GOALS_ENTER:
    return "profile/goals";
  case CON_CHARACTER_PERSONALITY_IDEAS:
  case CON_CHARACTER_PERSONALITY_ENTER:
    return "profile/personality";
  case CON_CHARACTER_IDEALS_IDEAS:
  case CON_CHARACTER_IDEALS_ENTER:
    return "profile/ideals";
  case CON_CHARACTER_BONDS_IDEAS:
  case CON_CHARACTER_BONDS_ENTER:
    return "profile/bonds";
  case CON_CHARACTER_FLAWS_IDEAS:
  case CON_CHARACTER_FLAWS_ENTER:
    return "profile/flaws";
  default:
    return NULL;
  }
}

static bool is_guidance_editor_state(int state)
{
  return state == CON_CHARACTER_GOALS_ENTER || state == CON_CHARACTER_PERSONALITY_ENTER ||
         state == CON_CHARACTER_IDEALS_ENTER || state == CON_CHARACTER_BONDS_ENTER ||
         state == CON_CHARACTER_FLAWS_ENTER;
}

static void build_screen_help(struct json_writer *w, struct descriptor_data *d, int state)
{
  const char *profile_id = guidance_profile_for_state(state);
  const struct character_creation_guidance *guidance =
      character_creation_guidance_for_profile(profile_id);

  if (guidance == NULL)
  {
    build_short_description_help(w, d, state);
    return;
  }

  json_raw(w, ",");
  json_field_string(w, "help",
                    is_guidance_editor_state(state) ? guidance->editor_prompt
                                                    : guidance->screen_introduction,
                    700);
  if (!is_guidance_editor_state(state))
  {
    json_raw(w, ",");
    json_field_string(w, "generatorShape", guidance->generator_shape, 700);
  }
  if (state == CON_CHARACTER_PERSONALITY_IDEAS || state == CON_CHARACTER_IDEALS_IDEAS ||
      state == CON_CHARACTER_BONDS_IDEAS || state == CON_CHARACTER_FLAWS_IDEAS)
  {
    json_raw(w, ",");
    json_field_string(w, "nonPersistenceNotice",
                      "This only shapes suggestions. It will not set or change your permanent "
                      "Background.",
                      200);
  }
}

static void build_control_wire(struct json_writer *w, struct descriptor_data *d, int state)
{
  int total = catalog_total_items(d, state);
  bool paged = total > 0 && catalog_uses_pagination(d, state);
  const char *continue_wire = NULL;
  const char *generate_wire = NULL;
  const char *cancel_wire = NULL;

  if (state == CON_GEN_DESCS_INTRO)
  {
    continue_wire = "1";
    if (!d->forced_short_desc_setup)
      cancel_wire = "0";
  }
  else if (state == CON_GEN_DESCS_MENU)
    continue_wire = "1";
  else if (state == CON_GEN_DESCS_MENU_PARSE)
  {
    json_raw(w, ",\"controlWire\":{");
    json_field_string(w, "confirm", "1", 8);
    json_raw(w, ",");
    json_field_string(w, "reselect", "2", 8);
    if (d->roleplay_pending.short_descriptor_2 == 0)
    {
      json_raw(w, ",");
      json_field_string(w, "continue", "3", 8);
    }
    if (!d->forced_short_desc_setup)
    {
      json_raw(w, ",");
      json_field_string(w, "cancel", "0", 8);
    }
    json_raw(w, "}");
    return;
  }
  else if (state == CON_CHARACTER_GOALS_IDEAS)
  {
    continue_wire = "q";
    generate_wire = "1";
    cancel_wire = "0";
  }
  else if (state == CON_CHARACTER_PERSONALITY_IDEAS || state == CON_CHARACTER_IDEALS_IDEAS ||
           state == CON_CHARACTER_BONDS_IDEAS || state == CON_CHARACTER_FLAWS_IDEAS)
  {
    continue_wire = "q";
    cancel_wire = "0";
  }
  else if (total > 0 && catalog_allows_cancel(state))
    cancel_wire = "quit";
  else if (state == CON_CHAR_RP_MENU)
  {
    json_raw(w, ",\"controlWire\":{");
    json_field_string(w, "quit", "q", 8);
    json_raw(w, "}");
    return;
  }

  if (continue_wire == NULL && generate_wire == NULL && cancel_wire == NULL && !paged)
    return;

  json_raw(w, ",\"controlWire\":{");
  if (continue_wire != NULL)
    json_field_string(w, "continue", continue_wire, 8);
  if (generate_wire != NULL)
  {
    if (continue_wire != NULL)
      json_raw(w, ",");
    json_field_string(w, "generate-example", generate_wire, 8);
  }
  if (cancel_wire != NULL)
  {
    if (continue_wire != NULL || generate_wire != NULL)
      json_raw(w, ",");
    json_field_string(w, "cancel", cancel_wire, 16);
  }
  if (paged)
  {
    if (continue_wire != NULL || generate_wire != NULL || cancel_wire != NULL)
      json_raw(w, ",");
    json_field_string(w, "next-page", WEB_ONBOARDING_CATALOG_NEXT, 32);
    json_raw(w, ",");
    json_field_string(w, "previous-page", WEB_ONBOARDING_CATALOG_PREVIOUS, 32);
  }
  json_raw(w, "}");
}

static const char *persistence_result_name(enum web_onboarding_persistence_result result)
{
  switch (result)
  {
  case WEB_ONBOARDING_PERSISTENCE_RESULT_ACCEPTED:
    return "accepted";
  case WEB_ONBOARDING_PERSISTENCE_RESULT_SAVED:
    return "saved";
  case WEB_ONBOARDING_PERSISTENCE_RESULT_FAILED:
    return "failed";
  default:
    return NULL;
  }
}

static bool prepare_outbound_transfer(struct descriptor_data *d,
                                      const struct onboarding_screen_info *screen)
{
  struct web_onboarding_session *session = NULL;
  struct web_onboarding_outbound_transfer *transfer = NULL;
  enum roleplay_text_commit_result normalize_result = ROLEPLAY_TEXT_COMMIT_OK;
  enum roleplay_text_field field = ROLEPLAY_TEXT_FIELD_INVALID;
  const char *value = NULL;
  char *normalized = NULL;
  size_t normalized_bytes = 0;
  unsigned int content_revision = 0;

  clear_outbound_transfer(d);
  if (d == NULL || screen == NULL || strcmp(screen->input_kind, "multiline") ||
      !web_onboarding_v2_enabled(d) || d->character == NULL)
    return TRUE;

  field = roleplay_text_field_from_state(screen->state);
  if (field == ROLEPLAY_TEXT_FIELD_INVALID)
    return TRUE;

  value = roleplay_text_field_value(d->character, field);
  if (value == NULL || *value == '\0')
    return TRUE;

  normalize_result = roleplay_text_normalize_copy(field, (const unsigned char *)value,
                                                  strlen(value), &normalized, &normalized_bytes);
  if (normalize_result != ROLEPLAY_TEXT_COMMIT_OK || normalized == NULL)
  {
    d->web_onboarding_error = WEB_ONBOARDING_ERROR_EDITOR_LOAD_FAILED;
    return FALSE;
  }

  session = ensure_onboarding_session(d);
  if (session == NULL)
  {
    wipe_private_memory(normalized, normalized_bytes + 1);
    free(normalized);
    d->web_onboarding_error = WEB_ONBOARDING_ERROR_EDITOR_LOAD_FAILED;
    return FALSE;
  }

  transfer = &session->outbound;
  transfer->active = TRUE;
  transfer->field = field;
  transfer->expected_state = d->connected;
  transfer->revision = d->web_onboarding_revision;
  transfer->chunk_count = expected_editor_chunk_count(normalized_bytes);
  transfer->total_bytes = normalized_bytes;
  transfer->started_at_ms = onboarding_now_ms();
  transfer->character = d->character;
  transfer->account = d->account;
  transfer->content = (unsigned char *)normalized;
  onboarding_flow_id(d, transfer->flow_id, sizeof(transfer->flow_id));
  strlcpy(transfer->field_id, roleplay_text_field_id(field), sizeof(transfer->field_id));
  content_revision = session->content_revisions[field];
  snprintf(transfer->transfer_id, sizeof(transfer->transfer_id), "source-%d-%d-%d-%u", d->desc_num,
           d->web_onboarding_revision, field, content_revision);

  if (!editor_digest_hex(transfer->content, transfer->total_bytes, transfer->digest))
  {
    clear_outbound_transfer(d);
    d->web_onboarding_error = WEB_ONBOARDING_ERROR_EDITOR_LOAD_FAILED;
    return FALSE;
  }

  return TRUE;
}

static bool outbound_transfer_is_current(struct descriptor_data *d,
                                         const struct web_onboarding_outbound_transfer *transfer)
{
  char flow_id[64];
  int64_t now_ms = 0;

  if (d == NULL || transfer == NULL || !transfer->active || d->character == NULL)
    return FALSE;

  now_ms = onboarding_now_ms();
  onboarding_flow_id(d, flow_id, sizeof(flow_id));
  return web_onboarding_v2_enabled(d) && d->connected == transfer->expected_state &&
         d->web_onboarding_revision == transfer->revision && d->character == transfer->character &&
         d->account == transfer->account && !strcmp(flow_id, transfer->flow_id) &&
         roleplay_text_field_from_state(d->connected) == transfer->field &&
         now_ms >= transfer->started_at_ms &&
         now_ms - transfer->started_at_ms <= WEB_ONBOARDING_EDITOR_TRANSFER_TIMEOUT_MS;
}

/*
 * Empty fields are immediately editable. Non-empty fields describe the exact
 * prepared side-channel transfer; the state is sent before its chunks, so a
 * gateway can bind every private byte to this revision before accepting it.
 */
static void build_editor_metadata(struct json_writer *w, struct descriptor_data *d,
                                  const struct onboarding_screen_info *screen)
{
  struct web_onboarding_session *session = NULL;
  struct web_onboarding_outbound_transfer *transfer = NULL;
  enum roleplay_text_field field = ROLEPLAY_TEXT_FIELD_INVALID;
  const char *value = NULL;
  unsigned int content_revision = 0;

  if (screen == NULL || strcmp(screen->input_kind, "multiline") || d->character == NULL)
    return;

  field = roleplay_text_field_from_state(screen->state);
  if (field == ROLEPLAY_TEXT_FIELD_INVALID)
    return;

  value = roleplay_text_field_value(d->character, field);
  session = d->web_onboarding_session;
  if (session != NULL)
    content_revision = session->content_revisions[field];

  if (value != NULL && *value != '\0')
  {
    transfer = session != NULL ? &session->outbound : NULL;
    if (transfer == NULL || !transfer->active || transfer->field != field ||
        transfer->revision != d->web_onboarding_revision)
      return;
  }

  json_raw(w, ",\"editor\":{");
  json_field_string(w, "fieldId", roleplay_text_field_id(field), 64);
  json_raw(w, ",");
  json_field_number(w, "maxBytes", (int)roleplay_text_field_max_bytes(field));
  json_raw(w, ",");
  json_field_number(w, "contentRevision", (int)content_revision);

  if (value == NULL || *value == '\0')
  {
    json_raw(w, ",");
    json_field_bool(w, "empty", TRUE);
  }
  else
  {
    json_raw(w, ",");
    json_field_bool(w, "empty", FALSE);
    json_raw(w, ",");
    json_field_string(w, "transferId", transfer->transfer_id, 120);
    json_raw(w, ",");
    json_field_number(w, "totalBytes", (int)transfer->total_bytes);
    json_raw(w, ",");
    json_field_number(w, "chunkCount", transfer->chunk_count);
    json_raw(w, ",");
    json_field_string(w, "digest", transfer->digest, 64);
  }
  json_raw(w, "}");
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
  char id[64];
  char fact_value[200];
  int background = BACKGROUND_NONE;
  int region = REGION_NONE;
  int deity = 0;
  int feat = 0;
  const struct character_creation_background *background_content = NULL;
  const struct character_creation_homeland *homeland_content = NULL;

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
    return;
  }

  if (screen->state == CON_BACKGROUND_ARCHTYPE_CONFIRM && d->roleplay_pending.background_active)
  {
    background = d->roleplay_pending.background;
    if (background <= BACKGROUND_NONE || background >= NUM_BACKGROUNDS ||
        background_list[background].name == NULL)
      return;

    feat = background_list[background].feat;
    background_content = character_creation_background_for_value(background);
    json_raw(w, "\"detail\":{");
    json_field_string(w, "id", background_stable_id(background), 64);
    json_raw(w, ",");
    json_field_string(w, "label", background_list[background].name, 64);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", background_media_key(background), 64);
    if (background_list[background].desc != NULL)
    {
      json_raw(w, ",");
      json_field_string_truncated(w, "description", background_list[background].desc, 900);
    }
    build_content_metadata(
        w, background_content != NULL ? background_content->content_id : NULL,
        background_content != NULL ? CHARACTER_CREATION_COMPASS_CANON_VERSION : NULL,
        background_content != NULL ? character_creation_content_provenance() : NULL);
    snprintf(fact_value, sizeof(fact_value), "%s +2, %s +2",
             ability_names[background_list[background].skills[0]],
             ability_names[background_list[background].skills[1]]);
    json_raw(w, ",\"facts\":[{");
    json_field_string(w, "label", "Skill bonuses", 40);
    json_raw(w, ",");
    json_field_string(w, "value", fact_value, 200);
    json_raw(w, "}");
    if (feat > 0 && feat < NUM_FEATS && feat_list[feat].name != NULL)
    {
      json_raw(w, ",{");
      json_field_string(w, "label", "Special ability", 40);
      json_raw(w, ",");
      json_field_string(w, "value", feat_list[feat].name, 120);
      json_raw(w, "},{");
      json_field_string(w, "label", "Ability effect", 40);
      json_raw(w, ",");
      json_field_string_truncated(w, "value", feat_list[feat].description, 400);
      json_raw(w, "}");
    }
    json_raw(w, "],");
    json_field_string(w, "irreversibleWarning",
                      "A background is permanent and also grants the listed mechanics.", 200);
    json_raw(w, "},");
    return;
  }

  if (screen->state == CON_QREGION_HELP && d->roleplay_pending.region_active)
  {
    region = d->roleplay_pending.region;
    if (region <= REGION_NONE || region >= NUM_REGIONS || !is_selectable_region(region))
      return;

    roleplay_region_stable_id(region, id, sizeof(id));
    homeland_content = character_creation_homeland_for_region(region);
    json_raw(w, "\"detail\":{");
    json_field_string(w, "id", id, 64);
    json_raw(w, ",");
    json_field_string(w, "label", regions[region], 80);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", roleplay_region_media_key(region), 64);
    json_raw(w, ",");
    json_field_string(
        w, "summary",
        homeland_content != NULL
            ? homeland_content->summary
            : "Your homeland is a role-play choice and grants its associated language.",
        300);
    if (homeland_content != NULL)
    {
      json_raw(w, ",");
      json_field_string_truncated(w, "description", homeland_content->description, 1600);
      build_content_metadata(w, homeland_content->content_id,
                             CHARACTER_CREATION_HOMELAND_CANON_VERSION,
                             homeland_content->provenance);
    }
    json_raw(w, ",\"facts\":[{");
    json_field_string(w, "label", "Language", 40);
    json_raw(w, ",");
    json_field_string(w, "value", get_region_language_name(region), 80);
    if (homeland_content != NULL)
    {
      json_raw(w, "},{");
      json_field_string(w, "label", "Place kind", 40);
      json_raw(w, ",");
      json_field_string(w, "value", homeland_content->place_kind, 80);
      json_raw(w, "},{");
      json_field_string(w, "label", "Geographic parent", 40);
      json_raw(w, ",");
      json_field_string(w, "value", homeland_content->geographic_parent, 100);
      json_raw(w, "},{");
      json_field_string(w, "label", "Political sphere", 40);
      json_raw(w, ",");
      json_field_string(w, "value", homeland_content->political_sphere, 100);
    }
    json_raw(w, "}],");
    json_field_string(w, "irreversibleWarning",
                      "A homeland cannot be changed here after it is saved.", 200);
    json_raw(w, "},");
    return;
  }

  if (screen->state == CON_CHARACTER_DEITY_CONFIRM && d->roleplay_pending.deity_active)
  {
    deity = d->roleplay_pending.deity;
    if (deity < 0 || deity >= NUM_DEITIES ||
        (deity > 0 && deity_list[deity].pantheon == DEITY_PANTHEON_NONE))
      return;

    roleplay_deity_stable_id(deity, id, sizeof(id));
    json_raw(w, "\"detail\":{");
    json_field_string(w, "id", id, 64);
    json_raw(w, ",");
    json_field_string(w, "label", deity == 0 ? "None" : deity_list[deity].name, 80);
    json_raw(w, ",");
    json_field_string(w, "mediaKey", roleplay_deity_media_key(deity), 64);
    if (deity == 0)
    {
      json_raw(w, ",");
      json_field_string(w, "summary", "Follow no deity.", 200);
    }
    else
    {
      if (deity_list[deity].description != NULL)
      {
        json_raw(w, ",");
        json_field_string_truncated(w, "description", deity_list[deity].description, 900);
      }
      json_raw(w, ",\"facts\":[{");
      json_field_string(w, "label", "Alignment", 40);
      json_raw(w, ",");
      json_field_string(w, "value",
                        GET_ALIGN_STRING(deity_list[deity].ethos, deity_list[deity].alignment), 80);
      json_raw(w, "}");
      if (deity_list[deity].portfolio != NULL)
      {
        json_raw(w, ",{");
        json_field_string(w, "label", "Portfolio", 40);
        json_raw(w, ",");
        json_field_string(w, "value", deity_list[deity].portfolio, 200);
        json_raw(w, "}");
      }
      json_raw(w, "]");
    }
    json_raw(w, ",");
    json_field_string(w, "irreversibleWarning",
                      "This spiritual choice cannot be changed here after it is saved.", 200);
    json_raw(w, "},");
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
  case CON_BACKGROUND_ARCHTYPE:
    build_background_choices(w, d, TRUE);
    break;
  case CON_GEN_DESCS_DESCRIPTORS_1:
  case CON_GEN_DESCS_DESCRIPTORS_2:
    build_short_feature_choices(w, d);
    break;
  case CON_GEN_DESCS_ADJECTIVES_1:
    build_short_adjective_choices(w, d->roleplay_pending.short_descriptor_1);
    break;
  case CON_GEN_DESCS_ADJECTIVES_2:
    build_short_adjective_choices(w, d->roleplay_pending.short_descriptor_2);
    break;
  case CON_CHARACTER_PERSONALITY_IDEAS:
  case CON_CHARACTER_IDEALS_IDEAS:
  case CON_CHARACTER_BONDS_IDEAS:
  case CON_CHARACTER_FLAWS_IDEAS:
    build_idea_background_choices(w, screen->state);
    break;
  case CON_CHARACTER_AGE_SELECT:
    build_age_choices(w, d);
    break;
  case CON_QREGION:
    build_region_choices(w, d);
    break;
  case CON_CHARACTER_FACTION_SELECT:
    build_faction_choices(w, d);
    break;
  case CON_CHARACTER_HOMETOWN_SELECT:
    build_hometown_choices(w, d);
    break;
  case CON_CHARACTER_DEITY_SELECT:
    build_deity_choices(w, d);
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
    build_simple_choice(w, "premade", "Guided build", "premade", "build/premade",
                        "The server picks a proven set of choices for you.");
    json_raw(w, ",");
    build_simple_choice(w, "custom", "Custom build", "custom", "build/custom",
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
    build_simple_choice(w, "roleplayer", "Fill in role-play details", "2",
                        "roleplay/choice-roleplayer",
                        "Describe your character's background, goals, and personality.");
    json_raw(w, ",");
    build_simple_choice(w, "non-roleplayer", "Skip role-play details", "1",
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
           web_onboarding_payload_version(d), flow_id, d->web_onboarding_revision);

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
  /* Echo the negotiated version, not the build's maximum. */
  json_field_number(&writer, "version", web_onboarding_payload_version(d));
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
  build_screen_help(&writer, d, screen->state);
  json_raw(&writer, ",");
  json_field_string(&writer, "inputKind", screen->input_kind, 16);
  json_raw(&writer, ",");
  json_field_bool(&writer, "sensitiveInput", screen->sensitive);
  json_raw(&writer, ",");
  json_field_string(&writer, "persistence", persistence_for_state(screen->state), 16);
  if (d->web_onboarding_session != NULL)
  {
    const char *persistence_result =
        persistence_result_name(d->web_onboarding_session->persistence_result);

    if (persistence_result != NULL &&
        d->web_onboarding_session->persistence_result_state == screen->state)
    {
      json_raw(&writer, ",");
      json_field_string(&writer, "persistenceResult", persistence_result, 16);
    }
  }
  build_error(&writer, d->web_onboarding_error);
  build_editor_metadata(&writer, d, screen);
  json_raw(&writer, ",");
  build_selected_detail(&writer, d, screen);
  if (screen->state == CON_CHAR_RP_MENU)
    build_profile_items(&writer, d);
  build_choices(&writer, d, screen);
  build_page(&writer, d, screen->state);
  build_examples(&writer, d, screen->state);
  if (is_short_description_state(screen->state) && d->forced_short_desc_setup)
  {
    json_raw(&writer, ",");
    json_field_bool(&writer, "blocking", TRUE);
  }
  build_control_wire(&writer, d, screen->state);
  json_raw(&writer, ",\"characters\":[");
  if (screen->state == CON_ACCOUNT_MENU)
    build_account_characters(&writer, d);
  json_raw(&writer, "],");
  build_selection(&writer, d);
  json_raw(&writer, ",");
  build_actions(&writer, d, screen);
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

  screen = find_screen_for_version(d->connected, web_onboarding_payload_version(d));
  if (screen == NULL)
    return FALSE;

  return build_state_payload(d, screen, buf, buf_size);
}

static void fail_outbound_transfer(struct descriptor_data *d)
{
  clear_outbound_transfer(d);
  if (d != NULL)
    d->web_onboarding_error = WEB_ONBOARDING_ERROR_EDITOR_LOAD_FAILED;
}

static void emit_outbound_transfer_frame(struct descriptor_data *d)
{
  struct web_onboarding_session *session = NULL;
  struct web_onboarding_outbound_transfer *transfer = NULL;
  char begin[1024];
  char encoded[WEB_ONBOARDING_EDITOR_MAX_BASE64_BYTES + 1];
  char envelope[WEB_ONBOARDING_MAX_PAYLOAD + 1];
  char commit[256];
  size_t raw_bytes = 0;
  int encoded_bytes = 0;
  int written = 0;
  protocol_error_t result;

  if (d == NULL || (session = d->web_onboarding_session) == NULL)
    return;

  transfer = &session->outbound;
  if (!outbound_transfer_is_current(d, transfer))
  {
    clear_outbound_transfer(d);
    return;
  }

  if (!transfer->begin_sent)
  {
    written =
        snprintf(begin, sizeof(begin),
                 "{\"v\":2,\"phase\":\"begin\",\"flowId\":\"%s\",\"revision\":%d,"
                 "\"fieldId\":\"%s\",\"transferId\":\"%s\",\"totalBytes\":%zu,"
                 "\"chunkCount\":%d,\"digest\":\"%s\"}",
                 transfer->flow_id, transfer->revision, transfer->field_id, transfer->transfer_id,
                 transfer->total_bytes, transfer->chunk_count, transfer->digest);
    if (written < 0 || (size_t)written >= sizeof(begin))
    {
      fail_outbound_transfer(d);
      return;
    }
    result = MSDPSendPair(d, WEB_ONBOARDING_CONTENT_VARIABLE, begin);
    if (result == PROTOCOL_ERROR_BUFFER_FULL)
      return;
    if (result != PROTOCOL_SUCCESS)
    {
      fail_outbound_transfer(d);
      return;
    }
    transfer->begin_sent = TRUE;
    return;
  }

  if (transfer->sent_bytes < transfer->total_bytes)
  {
    raw_bytes = MIN(transfer->total_bytes - transfer->sent_bytes,
                    (size_t)WEB_ONBOARDING_EDITOR_MAX_CHUNK_BYTES);

    encoded_bytes = EVP_EncodeBlock((unsigned char *)encoded,
                                    transfer->content + transfer->sent_bytes, (int)raw_bytes);
    if (encoded_bytes <= 0 || encoded_bytes > WEB_ONBOARDING_EDITOR_MAX_BASE64_BYTES)
    {
      fail_outbound_transfer(d);
      return;
    }
    encoded[encoded_bytes] = '\0';

    written = snprintf(envelope, sizeof(envelope),
                       "{\"phase\":\"chunk\",\"transferId\":\"%s\","
                       "\"index\":%d,\"data\":\"%s\"}",
                       transfer->transfer_id, transfer->next_index, encoded);
    if (written < 0 || (size_t)written >= sizeof(envelope))
    {
      OPENSSL_cleanse(encoded, sizeof(encoded));
      fail_outbound_transfer(d);
      return;
    }
    result = MSDPSendPair(d, WEB_ONBOARDING_CONTENT_VARIABLE, envelope);
    if (result == PROTOCOL_ERROR_BUFFER_FULL)
    {
      OPENSSL_cleanse(encoded, sizeof(encoded));
      return;
    }
    if (result != PROTOCOL_SUCCESS)
    {
      OPENSSL_cleanse(encoded, sizeof(encoded));
      fail_outbound_transfer(d);
      return;
    }
    transfer->sent_bytes += raw_bytes;
    transfer->next_index++;
    OPENSSL_cleanse(encoded, sizeof(encoded));
    return;
  }

  if (transfer->next_index != transfer->chunk_count)
  {
    fail_outbound_transfer(d);
    return;
  }

  written = snprintf(commit, sizeof(commit), "{\"phase\":\"commit\",\"transferId\":\"%s\"}",
                     transfer->transfer_id);
  if (written < 0 || (size_t)written >= sizeof(commit))
  {
    fail_outbound_transfer(d);
    return;
  }
  result = MSDPSendPair(d, WEB_ONBOARDING_CONTENT_VARIABLE, commit);
  if (result == PROTOCOL_ERROR_BUFFER_FULL)
    return;
  if (result != PROTOCOL_SUCCESS)
  {
    fail_outbound_transfer(d);
    return;
  }
  clear_outbound_transfer(d);
}

static void emit_state(struct descriptor_data *d, const struct onboarding_screen_info *screen)
{
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];
  protocol_error_t result;

  d->web_onboarding_revision++;
  prepare_outbound_transfer(d, screen);

  if (!build_state_payload(d, screen, payload, sizeof(payload)))
  {
    clear_outbound_transfer(d);
    return;
  }

  result = MSDPSendPair(d, WEB_ONBOARDING_MSDP_VARIABLE, payload);
  if (result != PROTOCOL_SUCCESS)
  {
    clear_outbound_transfer(d);
    d->web_onboarding_revision--;
    d->web_onboarding_dirty = TRUE;
    return;
  }
}

void web_onboarding_tick(struct descriptor_data *d)
{
  const struct onboarding_screen_info *screen = NULL;
  struct web_onboarding_session *session = NULL;
  bool state_changed = FALSE;

  if (d == NULL)
    return;

  if (!web_onboarding_enabled(d))
  {
    clear_editor_transfers(d);
    return;
  }

  session = d->web_onboarding_session;
  if (session != NULL && session->inbound.active)
  {
    if (transfer_is_expired(&session->inbound, onboarding_now_ms()))
      reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_INVALID_TRANSFER);
    else if (!transfer_lifecycle_is_current(d, &session->inbound))
      reject_editor_transfer(d, WEB_ONBOARDING_ERROR_EDITOR_STALE);
  }

  state_changed = d->connected != d->web_onboarding_last_state;
  if (!state_changed && !d->web_onboarding_dirty)
  {
    emit_outbound_transfer_frame(d);
    return;
  }

  if (state_changed)
  {
    clear_editor_transfers(d);
    d->web_onboarding_error = WEB_ONBOARDING_ERROR_NONE;
    if (session != NULL && session->persistence_result_state != d->connected)
    {
      session->persistence_result = WEB_ONBOARDING_PERSISTENCE_RESULT_NONE;
      session->persistence_result_state = -1;
    }
  }

  d->web_onboarding_last_state = d->connected;
  d->web_onboarding_dirty = FALSE;

  screen = find_screen_for_version(d->connected, web_onboarding_payload_version(d));
  if (screen == NULL)
  {
    /* Entering play or any unsupported state hands control back to the
     * classic terminal and clears every private onboarding surface. */
    clear_editor_transfers(d);
    emit_cleared(d);
    return;
  }

  emit_state(d, screen);
}
