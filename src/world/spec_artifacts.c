/* ***************************************************************************
 *  File: spec_artifacts.c                            Part of LuminariMUD
 *  Usage: Artifact system - unique, single-instance items of power.
 *
 *  Ported and modernized from the RealmsOfLuminari artifact system.  Current
 *  behavior, integration, and deliberate deviations are documented in
 *    docs/systems/ARTIFACT_SYSTEM.md
 *
 *  Design notes:
 *
 *  1. Membership.  ROL flagged artifact objects with IDX_ARTIFACT on a
 *     per-object spec bitfield.  LuminariMUD has no equivalent, so the
 *     registry IS the membership test: an object is an artifact iff its vnum
 *     resolves in art_index.  One structure instead of two.
 *
 *  2. Affect sourcing.  Each stat affect carries `af.specific = index + 1`
 *     so removing one artifact strips only that artifact's affects.  ROL
 *     keyed affects by spell type alone and therefore wiped every artifact's
 *     bonuses whenever any one was removed.  `specific` is only otherwise
 *     consulted for APPLY_SKILL affects in affect_join(), so this is safe.
 *
 *  3. Ownership across logout.  Rent extraction is explicitly scoped by
 *     objsave.c.  Actual destruction clears ownership even when the object is
 *     still carried, while temporary prototype clones have no world location
 *     and are ignored.
 *
 *  4. Boss multipliers.  ROL keyed bonus XP off an ACT_BOSS mob flag that
 *     LuminariMUD does not have.  Here an NPC three or more levels above the
 *     attacker is boss-tier; base kill XP also scales with victim level.
 *************************************************************************** */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "constants.h"
#include "spells.h"
#include "fight.h"
#include "screen.h"
#include "lists.h"
#include "act.h"
#include "class.h"
#include "oasis.h"
#include "dg_scripts.h"
#include "world/spec_artifacts.h"

/* --------------------------------------------------------------------------
 * Global state
 * -------------------------------------------------------------------------- */

struct artifact_data *art_index = NULL;
int total_artifacts = 0;

static int artifact_dirty = FALSE;
static int artifact_persistence_extract_depth = 0;

/* XP needed to leave each level.  Index is the current level, so slot 0 is
 * unused and slot ARTIFACT_MAX_LEVEL is never read. */
static const int artifact_xp_table[ARTIFACT_MAX_LEVEL + 1] = {0, 100, 300, 600, 1000};

static const char *artifact_stat_names[ARTIFACT_NUM_STATS] = {
    "Strength", "Intelligence", "Wisdom", "Dexterity", "Constitution", "Charisma"};

static const int artifact_stat_apply[ARTIFACT_NUM_STATS] = {APPLY_STR, APPLY_INT, APPLY_WIS,
                                                            APPLY_DEX, APPLY_CON, APPLY_CHA};

static const char *artifact_binding_names[NUM_ARTIFACT_BINDINGS] = {
    "None", "Bind on Pickup (Soulbound)", "Bind on Equip", "Bind on Account"};

/* --------------------------------------------------------------------------
 * Artifact templates
 *
 * Stat blocks, abilities, and proc chances live in code, not in the save
 * file, so they can be rebalanced without migrating player data.  Only
 * ownership, level, XP, and binding are persisted.
 * -------------------------------------------------------------------------- */
struct artifact_template
{
  int vnum;
  const char *ability_name; /* NULL = no active ability */
  const char *ability_desc;
  int ability_cooldown;
  int ability_cost;
  int binding_type;
  int stat_bonus[ARTIFACT_NUM_STATS];
  int hitroll_bonus;
  int damroll_bonus;
  int ac_bonus;
  int hp_bonus;
  int psp_bonus;
  int move_bonus;
  int resist_physical;
  int resist_magical;
  int resist_element;
  int proc_chance;
  int class_restrict;  /* CLASS_UNDEFINED = anyone may wield it */
  int class_min_level; /* levels required in that class         */
};

/* Levels required in an artifact's class before it stops burning you.  ROL
 * asked only "is that your class"; LuminariMUD is multi-class, so the gate is
 * a depth of commitment instead. */
#define ART_CLASS_GATE 10

/* clang-format off */
static const struct artifact_template artifact_templates[] = {
    /* vnum, ability, desc, cd, cost, binding,
       {str,int,wis,dex,con,cha}, hit, dam, ac, hp, psp, mv, rphys, rmag, relem, proc,
       class, class level */

    {ART_VNUM_TRORXEK, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {0, 0, 2, 0, 2, 0}, 2, 2, 0, 25, 30, 0, 0, 10, 0, 12, CLASS_DRUID, ART_CLASS_GATE},

    {ART_VNUM_AMAUKEKEL, "divineward", "Wraps you in a sanctuary of divine light", 600, 100,
     ARTIFACT_BIND_ON_EQUIP,
     {0, 2, 2, 0, 0, 0}, 1, 1, 2, 0, 50, 0, 0, 15, 0, 0, CLASS_CLERIC, ART_CLASS_GATE},

    {ART_VNUM_FADE, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {0, 0, 0, 3, 0, 0}, 4, 2, 1, 20, 0, 25, 5, 0, 0, 16, CLASS_ROGUE, ART_CLASS_GATE},

    {ART_VNUM_HENEKAR, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {0, 2, 1, 0, 0, 2}, 0, 0, 0, 0, 75, 0, 0, 15, 0, 0, CLASS_ROGUE, ART_CLASS_GATE},

    {ART_VNUM_DOOMBRINGER, "doomblast", "Unleashes a wave of doom on everyone nearby", 180, 75,
     ARTIFACT_BIND_ON_PICKUP,
     {3, 0, 0, 1, 0, 0}, 4, 5, 0, 30, 0, 0, 0, 0, 0, 20, CLASS_WARRIOR, ART_CLASS_GATE},

    {ART_VNUM_KELRARIN, "soulstrike", "Strikes a single target with soul energy", 300, 50,
     ARTIFACT_BIND_ON_EQUIP,
     {2, 0, 0, 0, 1, 0}, 3, 3, 0, 20, 0, 0, 0, 0, 5, 15, CLASS_UNDEFINED, 0},

    {ART_VNUM_KELROM, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {2, 0, 1, 0, 2, 0}, 2, 4, 0, 40, 0, 0, 5, 0, 0, 14, CLASS_UNDEFINED, 0},

    {ART_VNUM_GESEN, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_NONE,
     {1, 0, 0, 2, 0, 0}, 3, 2, 0, 0, 0, 30, 0, 0, 0, 18, CLASS_UNDEFINED, 0},

    {ART_VNUM_STINGER, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_ACCOUNT,
     {1, 0, 0, 3, 0, 0}, 5, 1, 0, 0, 0, 50, 10, 0, 0, 18, CLASS_UNDEFINED, 0},

    {ART_VNUM_AVERNUS, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {2, 0, 0, 1, 1, 0}, 4, 4, 0, 25, 0, 0, 0, 0, 10, 15, CLASS_UNDEFINED, 0},

    {ART_VNUM_AEGIS, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {0, 0, 0, 0, 3, 0}, 0, 0, 4, 60, 0, 0, 12, 12, 12, 0, CLASS_UNDEFINED, 0},

    {-1, NULL, NULL, 0, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     CLASS_UNDEFINED, 0}};

/* --------------------------------------------------------------------------
 * Called effects
 *
 * ROL gave each artifact a hand-written spec proc that listened for a phrase
 * said aloud, fired an effect, and queued a recharge event.  This table is
 * that same content expressed as data, dispatched by artifact_do_effect().
 *
 * `phrase` is matched against normalized speech: lowercased, trailing
 * punctuation and whitespace stripped.  When target_type is not
 * ART_TARGET_NONE the phrase is a prefix and whatever follows it is the
 * target argument.
 * -------------------------------------------------------------------------- */
struct artifact_effect
{
  int vnum;
  int slot; /* 0 .. ARTIFACT_MAX_EFFECTS-1, unique per artifact */
  const char *phrase;
  int target_type;
  int recharge; /* seconds */
  int effect;
  const char *desc; /* identify text */
};

static const struct artifact_effect artifact_effects[] = {
    /* Trorxek, the Staff of Ancient Oaks - the Oaken Defender */
    {ART_VNUM_TRORXEK, 0, "come oaken defender", ART_TARGET_NONE, ARTIFACT_RECHARGE_WEEK,
     ART_EFFECT_SUMMON_TREANT, "calls the Oaken Defender to fight at your side"},
    {ART_VNUM_TRORXEK, 1, "carpet of death", ART_TARGET_NONE, ARTIFACT_RECHARGE_DAY,
     ART_EFFECT_CREEPING_DOOM, "carpets the ground in creeping doom"},
    {ART_VNUM_TRORXEK, 2, "forest path home", ART_TARGET_NONE, ARTIFACT_RECHARGE_HOUR,
     ART_EFFECT_RECALL, "walks you home along the forest path"},
    {ART_VNUM_TRORXEK, 3, "moonlit path to", ART_TARGET_CHAR_WORLD, ARTIFACT_RECHARGE_DAY,
     ART_EFFECT_TRAVEL_TO, "opens a moonwell to a named traveller"},

    /* Amaukekel, the Rod of Light */
    {ART_VNUM_AMAUKEKEL, 0, "sunlit path to paradise", ART_TARGET_NONE, ARTIFACT_RECHARGE_WEEK,
     ART_EFFECT_DIMENSION_SHIFT, "shifts you and your group out of danger"},
    {ART_VNUM_AMAUKEKEL, 1, "give life to", ART_TARGET_OBJ_ROOM, ARTIFACT_RECHARGE_DAY,
     ART_EFFECT_RESURRECT, "restores the dead from a corpse at your feet"},
    {ART_VNUM_AMAUKEKEL, 2, "wrath of light", ART_TARGET_CHAR_ROOM, ARTIFACT_RECHARGE_HOUR,
     ART_EFFECT_DISPEL_EVIL, "calls down the wrath of light on the wicked"},

    /* Fade, the Shadowblade */
    {ART_VNUM_FADE, 0, "eyes of darkness", ART_TARGET_CHAR_ROOM, ARTIFACT_RECHARGE_DAY,
     ART_EFFECT_BLIND, "puts out an enemy's eyes"},
    {ART_VNUM_FADE, 1, "darken the world", ART_TARGET_NONE, ARTIFACT_RECHARGE_HOUR,
     ART_EFFECT_DARKNESS, "smothers the room in darkness"},
    {ART_VNUM_FADE, 2, "devour the soul", ART_TARGET_FIGHTING, ARTIFACT_RECHARGE_WEEK,
     ART_EFFECT_WEAKEN, "devours the soul of whoever you are fighting"},
    {ART_VNUM_FADE, 3, "shadowy path to", ART_TARGET_CHAR_WORLD, ARTIFACT_RECHARGE_HOUR,
     ART_EFFECT_TRAVEL_TO, "walks the shadows to a named traveller"},

    /* The Horn of Henekar */
    {ART_VNUM_HENEKAR, 0, "you see darkness", ART_TARGET_CHAR_ROOM, ARTIFACT_RECHARGE_6HOUR,
     ART_EFFECT_BLIND, "blinds a single listener"},
    {ART_VNUM_HENEKAR, 1, "peace to you", ART_TARGET_NONE, ARTIFACT_RECHARGE_12HOUR,
     ART_EFFECT_PACIFY, "ends every fight in the room"},
    {ART_VNUM_HENEKAR, 2, "join my quest", ART_TARGET_NONE, ARTIFACT_RECHARGE_12HOUR,
     ART_EFFECT_CHARM, "recruits the lesser creatures nearby"},
    {ART_VNUM_HENEKAR, 3, "sonic path to", ART_TARGET_CHAR_WORLD, ARTIFACT_RECHARGE_HOUR,
     ART_EFFECT_TRAVEL_TO, "rides its own note to a named traveller"},

    /* Doombringer */
    {ART_VNUM_DOOMBRINGER, 0, "bring annhilation forth", ART_TARGET_NONE, ARTIFACT_RECHARGE_WEEK,
     ART_EFFECT_ANNIHILATION, "annihilates everything hostile in the room"},
    {ART_VNUM_DOOMBRINGER, 1, "feel my power", ART_TARGET_CHAR_ROOM, ARTIFACT_RECHARGE_DAY,
     ART_EFFECT_BLACK_LIGHTNING, "calls black lightning down on one enemy"},
    {ART_VNUM_DOOMBRINGER, 2, "enrage me doombringer", ART_TARGET_NONE, ARTIFACT_RECHARGE_DAY,
     ART_EFFECT_ENRAGE, "drives you into a killing rage"},

    {-1, 0, NULL, ART_TARGET_NONE, 0, 0, NULL}};
/* clang-format on */

/* --------------------------------------------------------------------------
 * Lookup
 * -------------------------------------------------------------------------- */

/* Binary search.  art_index is kept sorted by vnum at boot. */
int artifact_search(int vnum)
{
  int bot = 0, top = 0, mid = 0;

  if (!art_index || total_artifacts == 0)
    return -1;

  top = total_artifacts - 1;

  while (bot <= top)
  {
    mid = (bot + top) / 2;

    if (art_index[mid].vnum == vnum)
      return mid;

    if (art_index[mid].vnum > vnum)
      top = mid - 1;
    else
      bot = mid + 1;
  }

  return -1;
}

struct artifact_data *artifact_by_vnum(int vnum)
{
  int idx = artifact_search(vnum);

  if (idx < 0)
    return NULL;

  return &art_index[idx];
}

struct artifact_data *artifact_of_obj(struct obj_data *obj)
{
  if (!obj)
    return NULL;

  return artifact_by_vnum(GET_OBJ_VNUM(obj));
}

int artifact_is_artifact(struct obj_data *obj)
{
  return (artifact_of_obj(obj) != NULL);
}

/* Any of the sentinel names means unowned. */
int artifact_is_owned(int vnum)
{
  struct artifact_data *art = artifact_by_vnum(vnum);

  if (!art || !art->owner || !*art->owner)
    return FALSE;

  if (!str_cmp(art->owner, ARTIFACT_OWNER_NONE) || !str_cmp(art->owner, ARTIFACT_OWNER_INIT) ||
      !str_cmp(art->owner, "none") || !str_cmp(art->owner, "no"))
    return FALSE;

  return TRUE;
}

int artifact_xp_to_next(int level)
{
  if (level < 1 || level >= ARTIFACT_MAX_LEVEL)
    return 0;

  return artifact_xp_table[level];
}

const char *artifact_binding_name(int binding)
{
  if (binding < 0 || binding >= NUM_ARTIFACT_BINDINGS)
    return "Unknown";

  return artifact_binding_names[binding];
}

/* --------------------------------------------------------------------------
 * Boot, persistence, shutdown
 * -------------------------------------------------------------------------- */

static int artifact_compare_vnum(const void *a, const void *b)
{
  const struct artifact_data *art_a = (const struct artifact_data *)a;
  const struct artifact_data *art_b = (const struct artifact_data *)b;

  return art_a->vnum - art_b->vnum;
}

static void artifact_mark_dirty(void)
{
  artifact_dirty = TRUE;
}

/* Stamp the code-side template onto a registry entry.  Ownership and
 * progression fields are left alone; those come from the save file. */
static void artifact_apply_template(struct artifact_data *art)
{
  int i = 0, j = 0;

  art->ability_cooldown = ARTIFACT_DEFAULT_COOLDOWN;

  for (i = 0; artifact_templates[i].vnum != -1; i++)
  {
    if (artifact_templates[i].vnum != art->vnum)
      continue;

    art->ability_name = artifact_templates[i].ability_name;
    art->ability_desc = artifact_templates[i].ability_desc;
    art->ability_cooldown = artifact_templates[i].ability_cooldown;
    art->ability_cost = artifact_templates[i].ability_cost;

    /* The template owns the binding rule; the save file only records
     * whether a binding has already been taken (bound_time). */
    art->binding_type = artifact_templates[i].binding_type;

    for (j = 0; j < ARTIFACT_NUM_STATS; j++)
      art->stat_bonus[j] = artifact_templates[i].stat_bonus[j];

    art->hitroll_bonus = artifact_templates[i].hitroll_bonus;
    art->damroll_bonus = artifact_templates[i].damroll_bonus;
    art->ac_bonus = artifact_templates[i].ac_bonus;
    art->hp_bonus = artifact_templates[i].hp_bonus;
    art->psp_bonus = artifact_templates[i].psp_bonus;
    art->move_bonus = artifact_templates[i].move_bonus;
    art->resist_physical = artifact_templates[i].resist_physical;
    art->resist_magical = artifact_templates[i].resist_magical;
    art->resist_element = artifact_templates[i].resist_element;
    art->proc_chance = artifact_templates[i].proc_chance;
    art->class_restrict = artifact_templates[i].class_restrict;
    art->class_min_level = artifact_templates[i].class_min_level;
    return;
  }
}

/* --------------------------------------------------------------------------
 * Called-effect lookup
 * -------------------------------------------------------------------------- */

static const struct artifact_effect *artifact_effect_at(int vnum, int slot)
{
  int i = 0;

  for (i = 0; artifact_effects[i].vnum != -1; i++)
    if (artifact_effects[i].vnum == vnum && artifact_effects[i].slot == slot)
      return &artifact_effects[i];

  return NULL;
}

/* Seconds left before the effect in `slot` may be called again, 0 when it is
 * ready and 0 when the artifact has no such effect. */
int artifact_recharge_remaining(struct artifact_data *art, int slot)
{
  const struct artifact_effect *effect = NULL;
  long elapsed = 0;

  if (!art || slot < 0 || slot >= ARTIFACT_MAX_EFFECTS)
    return 0;

  if (art->effect_used[slot] == 0)
    return 0;

  if (!(effect = artifact_effect_at(art->vnum, slot)))
    return 0;

  elapsed = (long)(time(0) - art->effect_used[slot]);

  if (elapsed >= effect->recharge)
    return 0;

  return (int)(effect->recharge - elapsed);
}

const char *artifact_recharge_name(int seconds)
{
  switch (seconds)
  {
  case ARTIFACT_RECHARGE_HOUR:
    return "once an hour";
  case ARTIFACT_RECHARGE_6HOUR:
    return "once every six hours";
  case ARTIFACT_RECHARGE_12HOUR:
    return "twice a day";
  case ARTIFACT_RECHARGE_DAY:
    return "once a day";
  case ARTIFACT_RECHARGE_WEEK:
    return "once a week";
  default:
    return "rarely";
  }
}

/* ROL's MEM_ARTIFACT bucket lived in a debug.c this codebase does not have.
 * The number it wanted is small and exactly computable, so report it rather
 * than build a global accounting framework around one subsystem. */
size_t artifact_memory_used(void)
{
  size_t bytes = 0;
  int i = 0;

  if (!art_index)
    return 0;

  bytes = sizeof(struct artifact_data) * (size_t)total_artifacts;

  for (i = 0; i < total_artifacts; i++)
  {
    if (art_index[i].owner)
      bytes += strlen(art_index[i].owner) + 1;
    if (art_index[i].account)
      bytes += strlen(art_index[i].account) + 1;
  }

  return bytes;
}

/* Write the registry out.  Temp file plus atomic rename, so a crash during
 * the write cannot corrupt the data. */
void artifact_save(void)
{
  FILE *fl = NULL;
  char temp_file[MAX_INPUT_LENGTH] = {'\0'};
  time_t current_time = 0;
  int i = 0;

  if (!art_index || total_artifacts == 0)
    return;

  snprintf(temp_file, sizeof(temp_file), "%s.tmp", ARTIFACT_FILE);

  if (!(fl = fopen(temp_file, "w")))
  {
    log("SYSERR: artifact_save: cannot open %s for writing", temp_file);
    return;
  }

  current_time = time(0);
  fprintf(fl, "# Artifact Ownership File v2.2\n");
  fprintf(fl, "# Format: vnum owner account level exp bound_time instance_persisted\n");
  fprintf(fl, "# Generated: %s", ctime(&current_time));
  fprintf(fl, "\n");

  for (i = 0; i < total_artifacts; i++)
    fprintf(fl, "%d %s %s %d %d %ld %d\n", art_index[i].vnum,
            (art_index[i].owner && *art_index[i].owner) ? art_index[i].owner : ARTIFACT_OWNER_NONE,
            (art_index[i].account && *art_index[i].account) ? art_index[i].account
                                                            : ARTIFACT_OWNER_NONE,
            art_index[i].level, art_index[i].experience, (long)art_index[i].bound_time,
            art_index[i].instance_persisted ? 1 : 0);

  fclose(fl);

  if (rename(temp_file, ARTIFACT_FILE) != 0)
    log("SYSERR: artifact_save: failed to rename %s to %s", temp_file, ARTIFACT_FILE);
  else
    artifact_dirty = FALSE;
}

void artifact_save_if_dirty(void)
{
  if (artifact_dirty)
    artifact_save();
}

enum artifact_file_format
{
  ARTIFACT_FORMAT_UNKNOWN = 0,
  ARTIFACT_FORMAT_V1,
  ARTIFACT_FORMAT_V20,
  ARTIFACT_FORMAT_V21,
  ARTIFACT_FORMAT_V22
};

static enum artifact_file_format artifact_detect_record_format(const char *line)
{
  char owner[MAX_INPUT_LENGTH] = {'\0'};
  char third[MAX_INPUT_LENGTH] = {'\0'};
  int vnum = 0, fourth = 0, fifth = 0, seventh = 0;
  long sixth = 0;
  int parsed = 0;

  parsed = sscanf(line, "%d %511s %511s %d %d %ld %d", &vnum, owner, third, &fourth, &fifth, &sixth,
                  &seventh);

  if (parsed >= 7)
    return ARTIFACT_FORMAT_V22;
  if (parsed == 6)
    return is_number(third) ? ARTIFACT_FORMAT_V20 : ARTIFACT_FORMAT_V21;
  if (parsed == 3)
    return ARTIFACT_FORMAT_V1;

  return ARTIFACT_FORMAT_UNKNOWN;
}

static int artifact_load_record(const char *line, enum artifact_file_format format, int *vnum,
                                char *owner, char *account, int *level, int *exp, long *bound_time,
                                int *instance_persisted)
{
  int saved_binding = ARTIFACT_BIND_NONE;
  long legacy_timestamp = 0;

  switch (format)
  {
  case ARTIFACT_FORMAT_V22:
    return sscanf(line, "%d %511s %511s %d %d %ld %d", vnum, owner, account, level, exp, bound_time,
                  instance_persisted) == 7;

  case ARTIFACT_FORMAT_V21:
    *instance_persisted = TRUE;
    return sscanf(line, "%d %511s %511s %d %d %ld", vnum, owner, account, level, exp, bound_time) ==
           6;

  case ARTIFACT_FORMAT_V20:
    *instance_persisted = TRUE;
    return sscanf(line, "%d %511s %d %d %d %ld", vnum, owner, level, exp, &saved_binding,
                  bound_time) == 6;

  case ARTIFACT_FORMAT_V1:
    if (sscanf(line, "%d %511s %ld", vnum, owner, &legacy_timestamp) != 3)
      return FALSE;
    *bound_time = legacy_timestamp;
    *instance_persisted = TRUE;
    return TRUE;

  default:
    return FALSE;
  }
}

/* Build the registry from the template table, then overlay saved ownership
 * and progression.  Artifacts always exist even if the save file is missing;
 * a missing file just means nobody owns anything yet. */
void artifact_boot(void)
{
  FILE *fl = NULL;
  char line[READ_SIZE] = {'\0'};
  char owner[MAX_INPUT_LENGTH] = {'\0'};
  char account[MAX_INPUT_LENGTH] = {'\0'};
  struct artifact_data *art = NULL;
  enum artifact_file_format file_format = ARTIFACT_FORMAT_UNKNOWN;
  enum artifact_file_format record_format = ARTIFACT_FORMAT_UNKNOWN;
  int vnum = 0, level = 0, exp = 0, count = 0, i = 0, j = 0;
  int instance_persisted = FALSE;
  long bound_time = 0;

  if (art_index)
    artifact_shutdown();

  for (count = 0; artifact_templates[count].vnum != -1; count++)
    ;

  if (count == 0)
  {
    log("Artifacts: no artifacts defined.");
    return;
  }

  CREATE(art_index, struct artifact_data, count);
  total_artifacts = 0;

  for (i = 0; i < count; i++)
  {
    if (real_object(artifact_templates[i].vnum) == NOTHING)
    {
      log("SYSERR: artifact_boot: artifact vnum %d has no object prototype - skipped",
          artifact_templates[i].vnum);
      continue;
    }

    art = &art_index[total_artifacts];

    art->vnum = artifact_templates[i].vnum;
    art->owner = strdup(ARTIFACT_OWNER_NONE);
    art->account = strdup(ARTIFACT_OWNER_NONE);
    art->ch = NULL;
    art->level = 1;
    art->experience = 0;
    art->bound_time = 0;
    art->instance_persisted = FALSE;
    art->last_ability_use = 0;
    art->last_proc = 0;
    art->class_restrict = CLASS_UNDEFINED;
    art->class_min_level = 0;

    for (j = 0; j < ARTIFACT_MAX_EFFECTS; j++)
      art->effect_used[j] = 0;

    for (j = 0; j < ARTIFACT_NUM_STATS; j++)
      art->stat_bonus[j] = 0;

    art->hitroll_bonus = 0;
    art->damroll_bonus = 0;
    art->ac_bonus = 0;
    art->hp_bonus = 0;
    art->psp_bonus = 0;
    art->move_bonus = 0;
    art->resist_physical = 0;
    art->resist_magical = 0;
    art->resist_element = 0;
    art->ability_name = NULL;
    art->ability_desc = NULL;
    art->ability_cooldown = ARTIFACT_DEFAULT_COOLDOWN;
    art->ability_cost = 0;
    art->proc_chance = 0;
    art->binding_type = ARTIFACT_BIND_NONE;

    total_artifacts++;
  }

  if (total_artifacts == 0)
  {
    log("SYSERR: artifact_boot: no artifact prototypes loaded - system inactive.");
    free(art_index);
    art_index = NULL;
    return;
  }

  /* Binary search requires sorted vnums. */
  qsort(art_index, total_artifacts, sizeof(struct artifact_data), artifact_compare_vnum);

  for (i = 0; i < total_artifacts; i++)
    artifact_apply_template(&art_index[i]);

  /* Overlay persisted state. */
  if ((fl = fopen(ARTIFACT_FILE, "r")))
  {
    while (fgets(line, sizeof(line), fl))
    {
      if (line[0] == '#')
      {
        if (strstr(line, "v2.2"))
          file_format = ARTIFACT_FORMAT_V22;
        else if (strstr(line, "v2.1"))
          file_format = ARTIFACT_FORMAT_V21;
        else if (strstr(line, "v2.0"))
          file_format = ARTIFACT_FORMAT_V20;
        else if (strstr(line, " v1") || strstr(line, " V1"))
          file_format = ARTIFACT_FORMAT_V1;
        continue;
      }

      if (line[0] == '\n' || line[0] == '\r')
        continue;

      owner[0] = '\0';
      account[0] = '\0';
      level = 1;
      exp = 0;
      bound_time = 0;
      instance_persisted = FALSE;

      record_format = file_format == ARTIFACT_FORMAT_UNKNOWN ? artifact_detect_record_format(line)
                                                             : file_format;
      if (!artifact_load_record(line, record_format, &vnum, owner, account, &level, &exp,
                                &bound_time, &instance_persisted))
      {
        log("Artifacts: malformed ownership record ignored: %s", line);
        continue;
      }

      if (!(art = artifact_by_vnum(vnum)))
      {
        log("Artifacts: record for vnum %d is not a known artifact - ignored.", vnum);
        continue;
      }

      if (level < 1 || level > ARTIFACT_MAX_LEVEL)
        level = 1;
      if (exp < 0)
        exp = 0;

      if (art->owner)
        free(art->owner);
      art->owner = strdup(owner);

      if (!artifact_is_owned(vnum))
        instance_persisted = FALSE;

      if (record_format == ARTIFACT_FORMAT_V21 || record_format == ARTIFACT_FORMAT_V22)
      {
        if (art->account)
          free(art->account);
        art->account = strdup(account);
      }

      art->level = level;
      art->experience = exp;
      art->bound_time = (time_t)bound_time;
      art->instance_persisted = instance_persisted ? TRUE : FALSE;
    }

    fclose(fl);
  }
  else
  {
    log("Artifacts: no %s yet - starting all artifacts unowned.", ARTIFACT_FILE);
    artifact_save();
  }

  artifact_dirty = FALSE;
  log("Artifacts: initialized %d artifact%s.", total_artifacts, total_artifacts == 1 ? "" : "s");
}

void artifact_shutdown(void)
{
  int i = 0;

  if (!art_index)
    return;

  for (i = 0; i < total_artifacts; i++)
  {
    if (art_index[i].owner)
      free(art_index[i].owner);
    if (art_index[i].account)
      free(art_index[i].account);
    /* ability_name / ability_desc point into the static template table. */
  }

  free(art_index);
  art_index = NULL;
  total_artifacts = 0;
  artifact_dirty = FALSE;
  artifact_persistence_extract_depth = 0;
}

/* --------------------------------------------------------------------------
 * Ownership
 * -------------------------------------------------------------------------- */

static void artifact_set_owner(struct artifact_data *art, struct char_data *ch)
{
  if (art->owner)
    free(art->owner);
  if (art->account)
    free(art->account);

  if (ch && !IS_NPC(ch))
  {
    art->owner = strdup(GET_NAME(ch));
    art->account = strdup((GET_ACCOUNT_NAME(ch) && *GET_ACCOUNT_NAME(ch)) ? GET_ACCOUNT_NAME(ch)
                                                                          : ARTIFACT_OWNER_NONE);
  }
  else
  {
    art->owner = strdup(ARTIFACT_OWNER_NONE);
    art->account = strdup(ARTIFACT_OWNER_NONE);
  }

  artifact_mark_dirty();
}

int artifact_to_char(struct obj_data *obj, struct char_data *ch)
{
  struct artifact_data *art = NULL;
  int state_changed = FALSE;

  if (!obj || !ch || IS_NPC(ch))
    return FALSE;

  if (!(art = artifact_of_obj(obj)))
    return FALSE;

  art->ch = ch;

  /* A bound artifact never changes hands by being picked up.  Without this,
   * anyone who lifted a bound artifact would be written in as its owner and
   * would then pass their own binding check. */
  if (art->bound_time > 0)
  {
    if (!art->instance_persisted)
    {
      art->instance_persisted = TRUE;
      artifact_mark_dirty();
      state_changed = TRUE;
    }
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
      artifact_get_nested(obj, ch);
    if (state_changed)
      artifact_save();
    return TRUE;
  }

  /* Re-acquiring something you already own is not an ownership change. */
  if (art->owner && !str_cmp(art->owner, GET_NAME(ch)))
  {
    if (!art->instance_persisted)
    {
      art->instance_persisted = TRUE;
      artifact_mark_dirty();
      artifact_save();
    }
    return TRUE;
  }

  mudlog(NRM, LVL_STAFF, TRUE, "ARTIFACT: %s now holds %s", GET_NAME(ch), GET_OBJ_SHORT(obj));

  artifact_set_owner(art, ch);
  art->instance_persisted = TRUE;
  artifact_mark_dirty();

  /* Bind-on-pickup takes hold the moment it is taken, not when it is worn. */
  if (art->binding_type == ARTIFACT_BIND_ON_PICKUP)
  {
    art->bound_time = time(0);
    artifact_mark_dirty();
    send_to_char(ch, "\trThe artifact binds itself to your soul!\tn\r\n");
  }

  artifact_save();

  if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
    artifact_get_nested(obj, ch);

  return TRUE;
}

int artifact_from_char(struct obj_data *obj, struct char_data *ch)
{
  struct artifact_data *art = NULL;

  if (!obj)
    return FALSE;

  if (!(art = artifact_of_obj(obj)))
    return FALSE;

  /* A bound artifact keeps its owner when set down.  Mark its live instance
   * as non-persistent so a zone reset may recover it after a reboot. */
  if (art->bound_time > 0)
  {
    art->ch = NULL;
    art->instance_persisted = FALSE;
    artifact_mark_dirty();
    artifact_save();
    return TRUE;
  }

  if (artifact_is_owned(art->vnum))
    mudlog(NRM, LVL_STAFF, TRUE, "ARTIFACT: %s released %s", ch ? GET_NAME(ch) : "someone",
           GET_OBJ_SHORT(obj));

  artifact_set_owner(art, NULL);
  art->ch = NULL;
  art->instance_persisted = FALSE;
  artifact_mark_dirty();
  artifact_save();

  if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
    artifact_drop_nested(obj);

  return TRUE;
}

/* --------------------------------------------------------------------------
 * Nested containers
 *
 * An artifact inside a bag inside a chest still belongs to whoever holds the
 * chest.  These walk arbitrarily deep container trees.
 * -------------------------------------------------------------------------- */

void artifact_tag_nested(struct obj_data *obj, struct char_data *ch)
{
  struct artifact_data *art = NULL;
  struct obj_data *o = NULL;

  if (!obj)
    return;

  if ((art = artifact_of_obj(obj)))
    art->ch = ch;

  for (o = obj->contains; o; o = o->next_content)
    artifact_tag_nested(o, ch);
}

void artifact_get_nested(struct obj_data *obj, struct char_data *ch)
{
  struct obj_data *o = NULL;

  if (!obj || !ch)
    return;

  for (o = obj->contains; o; o = o->next_content)
  {
    if (artifact_is_artifact(o))
      artifact_to_char(o, ch);
    else
      artifact_get_nested(o, ch);
  }
}

void artifact_drop_nested(struct obj_data *obj)
{
  struct artifact_data *art = NULL;
  struct obj_data *o = NULL;

  if (!obj)
    return;

  for (o = obj->contains; o; o = o->next_content)
  {
    if ((art = artifact_of_obj(o)))
      artifact_from_char(o, art->ch);
    else
      artifact_drop_nested(o);
  }
}

static void artifact_set_room_persistence(struct obj_data *obj, int persisted)
{
  struct artifact_data *art = NULL;
  struct obj_data *contained = NULL;

  if (!obj)
    return;

  if ((art = artifact_of_obj(obj)))
  {
    if (art->bound_time > 0)
    {
      art->ch = NULL;
      if (art->instance_persisted != persisted)
      {
        art->instance_persisted = persisted;
        artifact_mark_dirty();
      }
    }
    else
    {
      artifact_from_char(obj, art->ch);
    }
  }

  for (contained = obj->contains; contained; contained = contained->next_content)
    artifact_set_room_persistence(contained, persisted);
}

/* --------------------------------------------------------------------------
 * Bonuses
 * -------------------------------------------------------------------------- */

/* Every affect this artifact creates is stamped with its registry index + 1
 * in `specific`, so removal can target exactly one artifact. */
static void artifact_add_affect(struct char_data *ch, struct artifact_data *art, int location,
                                int modifier)
{
  struct affected_type af;

  if (modifier == 0)
    return;

  new_affect(&af);
  af.spell = SPELL_ARTIFACT_BONUS;
  af.duration = -1;
  af.location = location;
  af.modifier = modifier;
  af.bonus_type = BONUS_TYPE_ENHANCEMENT;
  af.specific = (sh_int)(artifact_search(art->vnum) + 1);

  affect_to_char(ch, &af);
}

void artifact_apply_bonuses(struct char_data *ch, struct obj_data *obj)
{
  struct artifact_data *art = NULL;
  int i = 0;

  if (!ch || !obj)
    return;

  if (!(art = artifact_of_obj(obj)))
    return;

  for (i = 0; i < ARTIFACT_NUM_STATS; i++)
    artifact_add_affect(ch, art, artifact_stat_apply[i], art->stat_bonus[i] * art->level);

  artifact_add_affect(ch, art, APPLY_HITROLL, art->hitroll_bonus * art->level);
  artifact_add_affect(ch, art, APPLY_DAMROLL, art->damroll_bonus * art->level);
  artifact_add_affect(ch, art, APPLY_AC, -(art->ac_bonus * art->level)); /* lower AC is better */
  artifact_add_affect(ch, art, APPLY_HIT, art->hp_bonus * art->level);
  artifact_add_affect(ch, art, APPLY_PSP, art->psp_bonus * art->level);
  artifact_add_affect(ch, art, APPLY_MOVE, art->move_bonus * art->level);

  affect_total(ch);

  if (!ch->mute_equip_messages)
  {
    send_to_char(ch, "\tYThe artifact's power flows through you!\tn\r\n");
    act("$n glows briefly as $e dons $p.", TRUE, ch, obj, NULL, TO_ROOM);
  }
}

void artifact_remove_bonuses(struct char_data *ch, struct obj_data *obj)
{
  struct artifact_data *art = NULL;
  struct affected_type *af = NULL, *af_next = NULL;
  sh_int tag = 0;
  int removed = 0;

  if (!ch || !obj)
    return;

  if (!(art = artifact_of_obj(obj)))
    return;

  tag = (sh_int)(artifact_search(art->vnum) + 1);

  for (af = ch->affected; af; af = af_next)
  {
    af_next = af->next;

    if (af->spell == SPELL_ARTIFACT_BONUS && af->specific == tag)
    {
      affect_remove(ch, af);
      removed++;
    }
  }

  if (removed > 0)
  {
    affect_total(ch);
    if (!ch->mute_equip_messages)
    {
      send_to_char(ch, "\tYThe artifact's power fades.\tn\r\n");
      act("The glow fades from $p as $n removes it.", TRUE, ch, obj, NULL, TO_ROOM);
    }
  }
}

/* --------------------------------------------------------------------------
 * Binding
 * -------------------------------------------------------------------------- */

int artifact_can_use(struct char_data *ch, struct obj_data *obj, int silent)
{
  struct artifact_data *art = NULL;

  if (!ch || !obj)
    return FALSE;

  if (!(art = artifact_of_obj(obj)))
    return TRUE; /* not an artifact - no restriction */

  if (IS_NPC(ch))
    return TRUE;

  /* Staff are never locked out of their own tools. */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return TRUE;

  /* Nothing is bound yet - anyone may pick it up and claim it. */
  if (art->bound_time == 0)
    return TRUE;

  switch (art->binding_type)
  {
  case ARTIFACT_BIND_NONE:
    return TRUE;

  case ARTIFACT_BIND_ON_PICKUP:
  case ARTIFACT_BIND_ON_EQUIP:
    if (art->owner && str_cmp(art->owner, GET_NAME(ch)))
    {
      if (!silent)
        send_to_char(ch, "%s is bound to %s and will not answer to you.\r\n", GET_OBJ_SHORT(obj),
                     art->owner);
      return FALSE;
    }
    break;

  case ARTIFACT_BIND_ON_ACCOUNT:
    if (art->account && str_cmp(art->account, ARTIFACT_OWNER_NONE) &&
        (!GET_ACCOUNT_NAME(ch) || str_cmp(art->account, GET_ACCOUNT_NAME(ch))))
    {
      if (!silent)
        send_to_char(ch, "%s is bound to another's account.\r\n", GET_OBJ_SHORT(obj));
      return FALSE;
    }
    break;

  default:
    log("SYSERR: artifact_can_use: unknown binding type %d on vnum %d", art->binding_type,
        art->vnum);
    break;
  }

  return TRUE;
}

/* --------------------------------------------------------------------------
 * Class restriction and the burn penalty
 *
 * ROL asked "is this your class" and scorched you if the answer was no.
 * LuminariMUD is multi-class, so an artifact instead demands a minimum number
 * of levels in the class it belongs to.  Failing the check does not stop you
 * wielding it - it just hurts, every tick, exactly as ROL's OBJ_BURN did.
 * -------------------------------------------------------------------------- */

int artifact_class_ok(struct char_data *ch, struct artifact_data *art)
{
  if (!ch || !art)
    return TRUE;

  if (art->class_restrict == CLASS_UNDEFINED)
    return TRUE;

  if (IS_NPC(ch))
    return TRUE;

  /* Staff carry these around for testing; do not cook them for it. */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return TRUE;

  return (CLASS_LEVEL(ch, art->class_restrict) >= art->class_min_level);
}

/* Called once per tick from point_update() for every player. */
void artifact_burn_tick(struct char_data *ch)
{
  struct artifact_data *art = NULL;
  struct obj_data *obj = NULL;
  int i = 0, worst = 0;

  if (!ch || IS_NPC(ch) || !art_index)
    return;

  if (GET_POS(ch) <= POS_DEAD)
    return;

  /* One burn per tick no matter how many artifacts object to you: find the
   * offending piece, then apply the penalty once. */
  for (i = 0; i < NUM_WEARS; i++)
  {
    if (!(obj = GET_EQ(ch, i)))
      continue;

    if (!(art = artifact_of_obj(obj)))
      continue;

    if (artifact_class_ok(ch, art))
      continue;

    worst++;

    act("\tR$p rejects your touch and sears you to the bone!\tn", FALSE, ch, obj, NULL, TO_CHAR);
    act("$p flares white-hot and scorches $n!", FALSE, ch, obj, NULL, TO_ROOM);
    break;
  }

  if (!worst)
    return;

  damage(ch, ch, dice(ARTIFACT_BURN_DICE, ARTIFACT_BURN_SIDES), TYPE_UNDEFINED, DAM_FIRE, FALSE);
}

/* --------------------------------------------------------------------------
 * Progression
 * -------------------------------------------------------------------------- */

/* Reapply bonuses in place so a level-up takes effect immediately instead of
 * waiting for the wearer to re-equip. */
static void artifact_refresh_bonuses(struct artifact_data *art)
{
  struct char_data *ch = art->ch;
  struct obj_data *obj = NULL;
  int i = 0;

  if (!ch)
    return;

  for (i = 0; i < NUM_WEARS; i++)
  {
    obj = GET_EQ(ch, i);
    if (obj && (int)GET_OBJ_VNUM(obj) == art->vnum)
    {
      artifact_remove_bonuses(ch, obj);
      artifact_apply_bonuses(ch, obj);
      return;
    }
  }
}

static struct char_data *artifact_live_holder(struct obj_data *obj)
{
  struct obj_data *outer = obj;

  if (!obj)
    return NULL;

  while (outer->in_obj)
    outer = outer->in_obj;

  if (outer->worn_by)
    return outer->worn_by;

  return outer->carried_by;
}

static void artifact_reassociate_live_instances(void)
{
  struct artifact_data *art = NULL;
  struct char_data *holder = NULL;
  struct obj_data *obj = NULL;
  int old_mute = FALSE;

  for (obj = object_list; obj; obj = obj->next)
  {
    if (!(art = artifact_of_obj(obj)))
      continue;

    holder = artifact_live_holder(obj);
    if (holder)
      art->ch = holder;

    if (!obj->worn_by)
      continue;

    old_mute = obj->worn_by->mute_equip_messages;
    obj->worn_by->mute_equip_messages = TRUE;
    artifact_refresh_bonuses(art);
    obj->worn_by->mute_equip_messages = old_mute;
  }
}

void artifact_reload(void)
{
  artifact_boot();
  artifact_reassociate_live_instances();
}

void artifact_check_levelup(struct artifact_data *art)
{
  if (!art)
    return;

  if (art->level >= ARTIFACT_MAX_LEVEL)
    return;

  if (art->experience < artifact_xp_table[art->level])
    return;

  art->level++;
  artifact_mark_dirty();

  if (art->ch)
  {
    send_to_char(art->ch, "\tY### Your artifact has grown in power! (Level %d) ###\tn\r\n",
                 art->level);
    send_to_char(art->ch, "\tCEnhanced bonuses are now active.\tn\r\n");
  }

  mudlog(NRM, LVL_STAFF, TRUE, "ARTIFACT: %d reached level %d (owner: %s)", art->vnum, art->level,
         art->owner ? art->owner : ARTIFACT_OWNER_NONE);

  artifact_refresh_bonuses(art);
  artifact_save();
}

/* Award XP to one specific artifact. */
void artifact_grant_xp_obj(struct char_data *ch, struct obj_data *obj, int amount)
{
  struct artifact_data *art = NULL;

  if (!ch || !obj || amount <= 0 || IS_NPC(ch))
    return;

  if (!(art = artifact_of_obj(obj)))
    return;

  if (art->level >= ARTIFACT_MAX_LEVEL)
    return;

  art->experience += amount;
  artifact_mark_dirty();

  if (rand_number(1, 100) <= ARTIFACT_XP_NOTIFY_CHANCE)
    send_to_char(ch, "\tcYour %s glows softly. (%d/%d XP)\tn\r\n", GET_OBJ_SHORT(obj),
                 art->experience, artifact_xp_table[art->level]);

  artifact_check_levelup(art);
}

/* Award generic combat XP to exactly one equipped artifact, chosen at random.
 *
 * ROL spread every grant across everything you were wearing, so stacking
 * artifacts multiplied progression.  Procs and abilities were already fixed to
 * pay only the artifact that earned them; this closes the last of that defect
 * without punishing a player for wearing a second artifact - the pool of XP is
 * the same either way, it simply lands in one place. */
void artifact_grant_xp(struct char_data *ch, int amount)
{
  struct obj_data *obj = NULL, *candidates[NUM_WEARS];
  int i = 0, count = 0;

  if (!ch || amount <= 0 || IS_NPC(ch))
    return;

  for (i = 0; i < NUM_WEARS; i++)
    if ((obj = GET_EQ(ch, i)) && artifact_is_artifact(obj))
      candidates[count++] = obj;

  if (count == 0)
    return;

  artifact_grant_xp_obj(ch, candidates[count == 1 ? 0 : rand_number(0, count - 1)], amount);
}

/* --------------------------------------------------------------------------
 * Core-file hooks
 * -------------------------------------------------------------------------- */

void artifact_obj_to_char(struct obj_data *obj, struct char_data *ch)
{
  if (!obj || !ch || !art_index)
    return;

  if (artifact_is_artifact(obj))
    artifact_to_char(obj, ch);
  else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
    artifact_get_nested(obj, ch);
}

void artifact_obj_from_char(struct obj_data *obj)
{
  struct artifact_data *art = NULL;

  if (!obj || !art_index)
    return;

  if ((art = artifact_of_obj(obj)))
    art->ch = NULL;
  else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
    artifact_tag_nested(obj, NULL);
}

void artifact_obj_to_room(struct obj_data *obj)
{
  int persisted = FALSE;

  if (!obj || !art_index)
    return;

  if (IN_ROOM(obj) != NOWHERE && ROOM_FLAGGED(IN_ROOM(obj), ROOM_HOUSE))
    persisted = TRUE;

  artifact_set_room_persistence(obj, persisted);
  artifact_save_if_dirty();
}

/* Returns FALSE when the equip must be refused.  The caller is responsible
 * for putting the object back, mirroring how equip_char() already handles
 * invalid_class(). */
int artifact_on_equip(struct char_data *ch, struct obj_data *obj, int pos)
{
  struct artifact_data *art = NULL;

  (void)pos;

  if (!ch || !obj || !art_index)
    return TRUE;

  if (!(art = artifact_of_obj(obj)))
  {
    if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
      artifact_get_nested(obj, ch);
    return TRUE;
  }

  /* equip_char() has already gated on this and messaged the player; this is
   * a silent backstop for any other caller. */
  if (!artifact_can_use(ch, obj, TRUE))
    return FALSE;

  if (!IS_NPC(ch))
  {
    artifact_to_char(obj, ch);

    /* Bind-on-equip and bind-on-account take hold on the first wear.
     * Bind-on-pickup has already bound in artifact_to_char(). */
    if ((art->binding_type == ARTIFACT_BIND_ON_EQUIP ||
         art->binding_type == ARTIFACT_BIND_ON_ACCOUNT) &&
        !art->bound_time)
    {
      art->bound_time = time(0);
      artifact_set_owner(art, ch);
      art->instance_persisted = TRUE;
      artifact_mark_dirty();
      send_to_char(ch, "\trThe artifact binds itself to you!\tn\r\n");
      artifact_save();
    }
  }

  artifact_apply_bonuses(ch, obj);

  if (art->experience == 0)
    artifact_grant_xp_obj(ch, obj, ARTIFACT_XP_FIRST_EQUIP);

  return TRUE;
}

void artifact_on_unequip(struct char_data *ch, struct obj_data *obj)
{
  if (!ch || !obj || !art_index)
    return;

  if (artifact_is_artifact(obj))
    artifact_remove_bonuses(ch, obj);
  else if (GET_OBJ_TYPE(obj) == ITEM_CONTAINER)
    artifact_tag_nested(obj, ch);
}

void artifact_begin_persistence_extract(void)
{
  artifact_persistence_extract_depth++;
}

void artifact_end_persistence_extract(void)
{
  if (artifact_persistence_extract_depth <= 0)
  {
    log("SYSERR: artifact_end_persistence_extract called without a matching begin");
    artifact_persistence_extract_depth = 0;
    return;
  }

  artifact_persistence_extract_depth--;
}

/* Called at the top of extract_obj(), while location links are still valid.
 * Rent extraction is explicitly scoped by objsave.c.  A locationless object
 * is a temporary prototype clone (for example, do_vstat), not the live
 * artifact instance. */
void artifact_on_extract(struct obj_data *obj)
{
  struct artifact_data *art = NULL;

  if (!obj || !art_index)
    return;

  if (!(art = artifact_of_obj(obj)))
    return;

  if (artifact_persistence_extract_depth > 0)
  {
    art->ch = NULL;
    return;
  }

  if (!obj->carried_by && !obj->worn_by && !obj->in_obj && IN_ROOM(obj) == NOWHERE)
    return;

  if (artifact_is_owned(art->vnum))
    mudlog(NRM, LVL_STAFF, TRUE, "ARTIFACT: destroyed instance of %s; ownership released",
           GET_OBJ_SHORT(obj));

  artifact_set_owner(art, NULL);
  art->ch = NULL;
  art->bound_time = 0;
  art->instance_persisted = FALSE;
  artifact_mark_dirty();
  artifact_save();
}

/* Single-instance enforcement.  TRUE means the just-loaded object must be
 * extracted again: someone already owns this artifact, or an instance is
 * already in play. */
int artifact_block_zone_load(obj_rnum obj_rnum)
{
  int vnum = 0;

  if (!art_index || obj_rnum == NOTHING)
    return FALSE;

  vnum = obj_index[obj_rnum].vnum;

  if (artifact_search(vnum) < 0)
    return FALSE;

  if (obj_index[obj_rnum].number > 0)
    return TRUE;

  if (artifact_is_owned(vnum) && artifact_by_vnum(vnum)->instance_persisted)
    return TRUE;

  return FALSE;
}

/* --------------------------------------------------------------------------
 * Combat integration
 * -------------------------------------------------------------------------- */

static int artifact_resist_for_damtype(struct artifact_data *art, int dam_type)
{
  switch (dam_type)
  {
  case DAM_SLICE:
  case DAM_PUNCTURE:
  case DAM_FORCE:
  case DAM_BLEEDING:
    return art->resist_physical;

  case DAM_FIRE:
  case DAM_COLD:
  case DAM_AIR:
  case DAM_EARTH:
  case DAM_ACID:
  case DAM_ELECTRIC:
  case DAM_WATER:
  case DAM_LIGHT:
  case DAM_SOUND:
    return art->resist_element;

  case DAM_RESERVED_DBC:
    return 0;

  default:
    return art->resist_magical;
  }
}

/* Highest applicable resistance wins; they do not stack. */
int artifact_damage_resist(struct char_data *victim, int dam, int dam_type)
{
  struct artifact_data *art = NULL;
  struct obj_data *obj = NULL;
  int best = 0, resist = 0, reduced = 0, i = 0;

  if (!victim || dam <= 0 || !art_index)
    return dam;

  if (IS_NPC(victim))
    return dam;

  for (i = 0; i < NUM_WEARS; i++)
  {
    if (!(obj = GET_EQ(victim, i)))
      continue;

    if (!(art = artifact_of_obj(obj)))
      continue;

    resist = artifact_resist_for_damtype(art, dam_type);
    if (resist > best)
      best = resist;
  }

  if (best <= 0)
    return dam;

  reduced = (dam * best) / 100;

  if (reduced > 0)
    send_to_char(victim, "\tYYour artifacts shimmer, absorbing some of the damage!\tn\r\n");

  return dam - reduced;
}

/* LuminariMUD has no ACT_BOSS flag.  A mob meaningfully above its attacker is
 * the closest honest analogue, and it is the same thing ROL's builders were
 * flagging by hand. */
static int artifact_is_boss(struct char_data *ch, struct char_data *victim)
{
  if (!ch || !victim || !IS_NPC(victim))
    return FALSE;

  return (GET_LEVEL(victim) >= GET_LEVEL(ch) + ARTIFACT_BOSS_LEVEL_MARGIN);
}

void artifact_combat_hit(struct char_data *ch, struct char_data *victim, int dam, int is_critical)
{
  int amount = 0;

  if (!ch || !victim || dam <= 0 || !art_index)
    return;

  if (IS_NPC(ch) || !IS_NPC(victim))
    return;

  amount = is_critical ? ARTIFACT_XP_CRIT : ARTIFACT_XP_HIT;

  if (artifact_is_boss(ch, victim))
    amount *= ARTIFACT_XP_BOSS_HIT_MULT;

  artifact_grant_xp(ch, amount);
}

void artifact_combat_kill(struct char_data *ch, struct char_data *victim)
{
  int amount = 0;

  if (!ch || !victim || !art_index)
    return;

  if (IS_NPC(ch) || !IS_NPC(victim))
    return;

  /* Base award still scales with the victim's level, which rewards the same
   * behavior ROL's boss flag was reaching for at every level of play. */
  amount = ARTIFACT_XP_KILL + (GET_LEVEL(victim) / 5);

  if (artifact_is_boss(ch, victim))
    amount *= ARTIFACT_XP_BOSS_KILL_MULT;

  artifact_grant_xp(ch, amount);
}

/* --------------------------------------------------------------------------
 * Signature weapon procedures
 *
 * ROL wrote these by hand, one per artifact, and rolled them on every hit
 * independently of any shared cooldown.  They run before the generic proc
 * system and do not consume it.
 *
 * Returns TRUE when the victim died, so the caller stops touching it.
 * -------------------------------------------------------------------------- */

/* The power an artifact brings to bear: its wielder's experience sharpened by
 * however far the artifact itself has grown. */
static int artifact_effect_level(struct char_data *ch, struct artifact_data *art)
{
  return MAX(20, GET_LEVEL(ch) + (art->level * 2));
}

/* Kelrarin's Hammer: the thrown hammer, and the alignment-gated mega blast. */
static int artifact_proc_kelrarin(struct char_data *ch, struct char_data *victim,
                                  struct obj_data *weapon, struct artifact_data *art)
{
  int amount = 0;

  /* The mega blast: only for the near-saintly, and only while unhurt. */
  if (GET_ALIGNMENT(ch) > ARTIFACT_KELRARIN_MEGA_ALIGN &&
      GET_HIT(ch) >= (GET_MAX_HIT(ch) * 9) / 10 && rand_number(1, ARTIFACT_KELRARIN_MEGA_ODDS) == 1)
  {
    act("\tW$p blazes like a fallen star and BREAKS over $N!\tn", FALSE, ch, weapon, victim,
        TO_CHAR);
    act("\tW$p blazes like a fallen star and BREAKS over $N!\tn", FALSE, ch, weapon, victim,
        TO_NOTVICT);
    act("\tW$p breaks over you like the judgement of heaven!\tn", FALSE, ch, weapon, victim,
        TO_VICT);

    if (damage(ch, victim, ARTIFACT_KELRARIN_MEGA_DAMAGE, TYPE_UNDEFINED, DAM_HOLY, FALSE) == -1)
      return TRUE;

    /* ROL's SuddenDeath: anything left standing but badly broken simply ends. */
    if (IS_NPC(victim) && GET_HIT(victim) < ARTIFACT_KELRARIN_MEGA_DAMAGE)
    {
      act("\tW$N is unmade where $E stands.\tn", FALSE, ch, weapon, victim, TO_CHAR);
      act("\tW$N is unmade where $E stands.\tn", FALSE, ch, weapon, victim, TO_NOTVICT);

      if (damage(ch, victim, GET_HIT(victim) + 100, TYPE_UNDEFINED, DAM_HOLY, FALSE) == -1)
        return TRUE;
    }

    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_ULTIMATE);
    return FALSE;
  }

  if (rand_number(1, ARTIFACT_KELRARIN_THROW_ODDS) != 1)
    return FALSE;

  /* ROL's flat ceiling of 250; here the hammer only hits that hard once the
   * artifact has grown into it. */
  amount = rand_number(1, MAX(50, (ARTIFACT_KELRARIN_THROW_MAX * art->level) / ARTIFACT_MAX_LEVEL));

  act("\tY$n hurls $p; it strikes $N and returns to $s hand!\tn", FALSE, ch, weapon, victim,
      TO_NOTVICT);
  act("\tYYou hurl $p; it strikes $N and returns to your hand!\tn", FALSE, ch, weapon, victim,
      TO_CHAR);
  act("\tY$p hurtles into you and flies back to $n!\tn", FALSE, ch, weapon, victim, TO_VICT);

  /* Full lifesteal: every point taken is a point returned. */
  GET_HIT(ch) = MIN(GET_HIT(ch) + amount, GET_MAX_HIT(ch));
  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SOUL);

  return (damage(ch, victim, amount, TYPE_UNDEFINED, DAM_FORCE, FALSE) == -1);
}

/* Kelrom, the Axe of Pahluruk: it will not be turned on an animal. */
static int artifact_proc_kelrom(struct char_data *ch, struct char_data *victim,
                                struct obj_data *weapon, struct artifact_data *art, int dam)
{
  struct char_data *tch = NULL;
  int healed = 0;

  if (IS_ANIMAL(victim))
  {
    act("\tR$p twists in your grip and turns on you for what you have struck!\tn", FALSE, ch,
        weapon, NULL, TO_CHAR);
    act("\tR$p twists in $n's grip and turns on $m!\tn", FALSE, ch, weapon, NULL, TO_ROOM);

    damage(ch, ch, GET_HIT(ch) + 100, TYPE_UNDEFINED, DAM_HOLY, FALSE);
    return FALSE; /* the victim is untouched; the wielder is not */
  }

  if (dam <= 0)
    return FALSE;

  healed = MAX(1, (dam * ARTIFACT_KELROM_HEALBACK_PERCENT * art->level) / 100);

  if (GROUP(ch))
  {
    simple_list(NULL);

    while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
    {
      if (IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      if (GET_HIT(tch) >= GET_MAX_HIT(tch))
        continue;

      GET_HIT(tch) = MIN(GET_HIT(tch) + healed, GET_MAX_HIT(tch));
      send_to_char(tch, "\tGA warm green light spills from Kelrom and knits your wounds.\tn\r\n");
    }

    simple_list(NULL);
  }
  else if (GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    GET_HIT(ch) = MIN(GET_HIT(ch) + healed, GET_MAX_HIT(ch));
    send_to_char(ch, "\tGA warm green light spills from Kelrom and knits your wounds.\tn\r\n");
  }

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_HEAL);
  return FALSE;
}

/* Gesen, the Returning Axe: thrown, and it carries a full harm with it. */
static int artifact_proc_gesen(struct char_data *ch, struct char_data *victim,
                               struct obj_data *weapon, struct artifact_data *art)
{
  if (rand_number(1, ARTIFACT_GESEN_THROW_ODDS) != 1)
    return FALSE;

  act("\tc$n whips $p end over end into $N; it flies back to $s hand!\tn", FALSE, ch, weapon,
      victim, TO_NOTVICT);
  act("\tcYou whip $p end over end into $N; it flies back to your hand!\tn", FALSE, ch, weapon,
      victim, TO_CHAR);
  act("\tc$p spins into you and away again!\tn", FALSE, ch, weapon, victim, TO_VICT);

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_DOOM);

  call_magic(ch, victim, NULL, SPELL_HARM, 0, artifact_effect_level(ch, art), CAST_WEAPON_SPELL);

  return (DEAD(victim) || GET_POS(victim) == POS_DEAD);
}

/* Avernus, the Black Blade: it keeps its wielder alive to keep swinging. */
static int artifact_proc_avernus(struct char_data *ch, struct obj_data *weapon,
                                 struct artifact_data *art)
{
  if (GET_HIT(ch) >= ARTIFACT_AVERNUS_HEAL_THRESHOLD)
    return FALSE;

  /* A higher-level blade is quicker to spend itself keeping you upright. */
  if (rand_number(1, 100) > ARTIFACT_AVERNUS_HEAL_CHANCE + (art->level * 2))
    return FALSE;

  GET_HIT(ch) = GET_MAX_HIT(ch);

  act("\tW$p drinks deep and pours everything it has back into you!\tn", FALSE, ch, weapon, NULL,
      TO_CHAR);
  act("\tW$p flares black and $n's wounds close before your eyes.\tn", FALSE, ch, weapon, NULL,
      TO_ROOM);

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_HEAL);
  return FALSE;
}

/* Trorxek, the Staff of Ancient Oaks: a blinding strike on a critical hit. */
static int artifact_proc_trorxek(struct char_data *ch, struct char_data *victim,
                                 struct obj_data *weapon, struct artifact_data *art,
                                 int is_critical)
{
  struct affected_type af;

  if (!is_critical)
    return FALSE;

  if (AFF_FLAGGED(victim, AFF_BLIND) || MOB_FLAGGED(victim, MOB_NOBLIND))
    return FALSE;

  new_affect(&af);
  af.spell = SPELL_BLINDNESS;
  af.duration = 1 + (art->level / 2);
  af.location = APPLY_HITROLL;
  af.modifier = -4;
  SET_BIT_AR(af.bitvector, AFF_BLIND);
  affect_to_char(victim, &af);

  act("\tY$p bursts into green light and blinds $N!\tn", FALSE, ch, weapon, victim, TO_CHAR);
  act("\tY$p bursts into green light and blinds $N!\tn", FALSE, ch, weapon, victim, TO_NOTVICT);
  act("\tYGreen light bursts from $p and you can see nothing at all!\tn", FALSE, ch, weapon, victim,
      TO_VICT);

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_FEAR);
  return FALSE;
}

/* Dispatch.  Returns TRUE when the victim is gone. */
static int artifact_signature_proc(struct char_data *ch, struct char_data *victim,
                                   struct obj_data *weapon, struct artifact_data *art, int dam,
                                   int is_critical)
{
  switch (art->vnum)
  {
  case ART_VNUM_TRORXEK:
    return artifact_proc_trorxek(ch, victim, weapon, art, is_critical);
  case ART_VNUM_KELRARIN:
    return artifact_proc_kelrarin(ch, victim, weapon, art);
  case ART_VNUM_KELROM:
    return artifact_proc_kelrom(ch, victim, weapon, art, dam);
  case ART_VNUM_GESEN:
    return artifact_proc_gesen(ch, victim, weapon, art);
  case ART_VNUM_AVERNUS:
    return artifact_proc_avernus(ch, weapon, art);
  default:
    return FALSE;
  }
}

void artifact_weapon_proc(struct char_data *ch, struct char_data *victim, struct obj_data *weapon,
                          int dam, int is_critical)
{
  struct artifact_data *art = NULL;
  struct affected_type af;
  int proc_type = 0, amount = 0;

  if (!ch || !victim || !weapon || !art_index)
    return;

  if (!(art = artifact_of_obj(weapon)))
    return;

  /* The hand-written procedures roll first and answer to nothing else. */
  if (artifact_signature_proc(ch, victim, weapon, art, dam, is_critical))
    return;

  if (art->proc_chance <= 0)
    return;

  /* Internal cooldown, so a fast weapon cannot chain procs every swing. */
  if (art->last_proc > 0 && (time(0) - art->last_proc) < ARTIFACT_PROC_ICD)
    return;

  if (rand_number(1, 100) > art->proc_chance)
    return;

  proc_type = rand_number(1, art->level);

  switch (proc_type)
  {
  case ARTIFACT_PROC_SOUL:
    amount = dice(art->level, 6);
    act("$p glows with dark energy as it tears at $N's soul!", FALSE, ch, weapon, victim, TO_CHAR);
    act("$p glows with dark energy as it tears at $N's soul!", FALSE, ch, weapon, victim,
        TO_NOTVICT);
    act("$p tears at your very soul!", FALSE, ch, weapon, victim, TO_VICT);
    damage(ch, victim, amount, TYPE_UNDEFINED, DAM_NEGATIVE, FALSE);
    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SOUL);
    break;

  case ARTIFACT_PROC_HEAL:
    if (GET_HIT(ch) >= GET_MAX_HIT(ch))
      break;
    amount = dice(art->level, 4);
    GET_HIT(ch) = MIN(GET_HIT(ch) + amount, GET_MAX_HIT(ch));
    act("$p glows with holy light, healing your wounds!", FALSE, ch, weapon, NULL, TO_CHAR);
    act("$p glows with holy light, healing $n's wounds!", FALSE, ch, weapon, NULL, TO_ROOM);
    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_HEAL);
    break;

  case ARTIFACT_PROC_FEAR:
    if (AFF_FLAGGED(victim, AFF_FEAR))
      break;
    new_affect(&af);
    af.spell = SPELL_FEAR;
    af.duration = 1 + (art->level / 2);
    SET_BIT_AR(af.bitvector, AFF_FEAR);
    affect_to_char(victim, &af);
    act("$p emanates waves of terror at $N!", FALSE, ch, weapon, victim, TO_CHAR);
    act("$p emanates waves of terror at $N!", FALSE, ch, weapon, victim, TO_NOTVICT);
    act("You are overcome with terror!", FALSE, ch, weapon, victim, TO_VICT);
    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_FEAR);
    break;

  case ARTIFACT_PROC_DOOM:
    if (art->level < 4)
      break;
    act("$p curses $N with impending doom!", FALSE, ch, weapon, victim, TO_CHAR);
    act("$p curses $N with impending doom!", FALSE, ch, weapon, victim, TO_NOTVICT);
    act("You feel doomed!", FALSE, ch, weapon, victim, TO_VICT);
    damage(ch, victim, dice(art->level, 8), TYPE_UNDEFINED, DAM_NEGATIVE, FALSE);
    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_DOOM);
    break;

  case ARTIFACT_PROC_ULTIMATE:
    /* Level 5 only, never against players or high-level foes, and then only
     * one time in twenty. */
    if (art->level < ARTIFACT_MAX_LEVEL || !IS_NPC(victim))
      break;
    if (GET_LEVEL(victim) > GET_LEVEL(ch))
      break;
    if (rand_number(1, 100) > 5)
      break;
    act("$p ERUPTS with ultimate power, utterly destroying $N!", FALSE, ch, weapon, victim,
        TO_CHAR);
    act("$p ERUPTS with ultimate power, utterly destroying $N!", FALSE, ch, weapon, victim,
        TO_NOTVICT);
    damage(ch, victim, GET_HIT(victim) + 100, TYPE_UNDEFINED, DAM_NEGATIVE, FALSE);
    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_ULTIMATE);
    break;

  default:
    break;
  }

  art->last_proc = time(0);
}

/* --------------------------------------------------------------------------
 * Called effects
 *
 * ROL's eleven hand-written spec procs, rebuilt on LuminariMUD.  Each artifact
 * listens for its own phrases in ordinary speech, fires the matching effect,
 * and puts that one effect - not the whole artifact - on its own recharge.
 * -------------------------------------------------------------------------- */

/* Lowercase, collapse runs of whitespace, and drop trailing punctuation, so
 * "Carpet of death!" and "carpet  of  death" both match one table entry.
 * do_say() appends a period to anything unpunctuated, which is exactly the
 * kind of thing this has to survive. */
static void artifact_normalize_speech(char *dst, size_t size, const char *src)
{
  size_t out = 0;
  int in_space = FALSE;

  if (!dst || size == 0)
    return;

  dst[0] = '\0';

  if (!src)
    return;

  while (*src && isspace((unsigned char)*src))
    src++;

  for (; *src && out + 1 < size; src++)
  {
    if (isspace((unsigned char)*src))
    {
      in_space = TRUE;
      continue;
    }

    if (in_space && out > 0)
      dst[out++] = ' ';
    in_space = FALSE;

    if (out + 1 >= size)
      break;

    dst[out++] = LOWER(*src);
  }

  dst[out] = '\0';

  while (out > 0 && (dst[out - 1] == '.' || dst[out - 1] == '!' || dst[out - 1] == '?' ||
                     dst[out - 1] == ',' || dst[out - 1] == ' '))
    dst[--out] = '\0';
}

/* Walk the wielder to wherever a named traveller is standing.  Every
 * "<something> path to <name>" effect lands here. */
static int artifact_travel_to(struct char_data *ch, struct obj_data *obj, const char *name)
{
  struct char_data *target = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  room_rnum dest = NOWHERE;

  strlcpy(arg, name, sizeof(arg));

  if (!(target = get_player_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
  {
    send_to_char(ch, "The path gropes for %s and finds nothing.\r\n", arg);
    return FALSE;
  }

  if (target == ch)
  {
    send_to_char(ch, "You are already exactly where you are.\r\n");
    return FALSE;
  }

  dest = IN_ROOM(target);

  if (dest == NOWHERE || IN_ROOM(ch) == NOWHERE)
  {
    send_to_char(ch, "The path will not open.\r\n");
    return FALSE;
  }

  if (AFF_FLAGGED(ch, AFF_NOTELEPORT) || AFF_FLAGGED(target, AFF_NOTELEPORT))
  {
    send_to_char(ch, "The path refuses to open.\r\n");
    return FALSE;
  }

  if (!valid_mortal_tele_dest(ch, dest, TRUE) || !valid_mortal_tele_dest(ch, IN_ROOM(ch), TRUE) ||
      ROOM_FLAGGED(dest, ROOM_NOTELEPORT) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_NOTELEPORT))
  {
    send_to_char(ch, "A bright flash closes the path before you can step through.\r\n");
    return FALSE;
  }

  /* The same plane restrictions every other travel effect observes. */
  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ELEMENTAL) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ETH_PLANE) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_ASTRAL_PLANE) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(dest), ZONE_ELEMENTAL) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(dest), ZONE_ETH_PLANE) ||
      ZONE_FLAGGED(GET_ROOM_ZONE(dest), ZONE_ASTRAL_PLANE))
  {
    send_to_char(ch, "There is no path between these planes.\r\n");
    return FALSE;
  }

  if (FIGHTING(ch))
    stop_fighting(ch);

  act("$n steps into $p and is gone.", TRUE, ch, obj, NULL, TO_ROOM);

  char_from_room(ch);

  if (ZONE_FLAGGED(GET_ROOM_ZONE(dest), ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[dest].coords[0];
    Y_LOC(ch) = world[dest].coords[1];
  }

  char_to_room(ch, dest);

  act("$n steps out of nowhere at all.", TRUE, ch, obj, NULL, TO_ROOM);
  look_at_room(ch, 0);

  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);

  return TRUE;
}

/* Amaukekel's sunlit path: the whole group leaves at once. */
static int artifact_dimension_shift(struct char_data *ch, struct obj_data *obj)
{
  struct char_data *tch = NULL;
  int moved = 0;

  act("\tW$n lifts $p and a corridor of sunlight opens in the air.\tn", FALSE, ch, obj, NULL,
      TO_ROOM);
  send_to_char(ch, "\tWYou lift the rod and a corridor of sunlight opens before you.\tn\r\n");

  call_magic(ch, ch, NULL, SPELL_WORD_OF_RECALL, 0, MAX(20, GET_LEVEL(ch)), CAST_WEAPON_SPELL);
  moved++;

  if (GROUP(ch))
  {
    simple_list(NULL);

    while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
    {
      if (tch == ch || IN_ROOM(tch) != IN_ROOM(ch))
        continue;

      call_magic(ch, tch, NULL, SPELL_WORD_OF_RECALL, 0, MAX(20, GET_LEVEL(ch)), CAST_WEAPON_SPELL);
      moved++;
    }

    simple_list(NULL);
  }

  return (moved > 0);
}

/* Trorxek's Oaken Defender. */
static int artifact_summon_treant(struct char_data *ch, struct obj_data *obj,
                                  struct artifact_data *art)
{
  struct char_data *mob = NULL;

  if (check_npc_followers(ch, NPC_MODE_SPARE, 0) <= 0)
  {
    send_to_char(ch, "You already command as many as will answer you.\r\n");
    return FALSE;
  }

  if (!(mob = read_mobile(ART_VNUM_OAKEN_DEFENDER, VIRTUAL)))
  {
    send_to_char(ch, "The staff strains, but nothing answers.\r\n");
    log("SYSERR: artifact_summon_treant: mob prototype %d is missing", ART_VNUM_OAKEN_DEFENDER);
    return FALSE;
  }

  if (ZONE_FLAGGED(GET_ROOM_ZONE(IN_ROOM(ch)), ZONE_WILDERNESS))
  {
    X_LOC(mob) = world[IN_ROOM(ch)].coords[0];
    Y_LOC(mob) = world[IN_ROOM(ch)].coords[1];
  }

  char_to_room(mob, IN_ROOM(ch));
  IS_CARRYING_W(mob) = 0;
  IS_CARRYING_N(mob) = 0;
  SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);
  GET_LEVEL(mob) = MIN(30, MAX(10, artifact_effect_level(ch, art) - 10));
  autoroll_mob(mob, TRUE, FALSE);
  add_follower(mob, ch);

  act("\tG$n raises $p and the earth splits as something enormous stands up out of it.\tn", FALSE,
      ch, obj, NULL, TO_ROOM);
  send_to_char(ch, "\tGThe Oaken Defender answers, and puts itself between you and harm.\tn\r\n");

  return TRUE;
}

/* Amaukekel's resurrection: a corpse at your feet, named aloud. */
static int artifact_resurrect_corpse(struct char_data *ch, struct obj_data *obj, const char *name)
{
  struct obj_data *corpse = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  strlcpy(arg, name, sizeof(arg));

  if (IN_ROOM(ch) == NOWHERE)
    return FALSE;

  if (!(corpse = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)))
  {
    send_to_char(ch, "There is nothing here by that name to give life to.\r\n");
    return FALSE;
  }

  if (!IS_CORPSE(corpse))
  {
    send_to_char(ch, "%s was never alive to begin with.\r\n", GET_OBJ_SHORT(corpse));
    return FALSE;
  }

  act("\tW$n holds $p over $P and light pours out of it.\tn", FALSE, ch, obj, corpse, TO_ROOM);
  send_to_char(ch, "\tWYou hold the rod over the fallen and light pours out of it.\tn\r\n");

  return (call_magic(ch, ch, corpse, SPELL_RESURRECT, 0, MAX(30, GET_LEVEL(ch)),
                     CAST_WEAPON_SPELL) > 0);
}

/* The Horn of Henekar's peace: every fight in the room simply stops. */
static int artifact_pacify_room(struct char_data *ch, struct obj_data *obj)
{
  struct char_data *vict = NULL, *next_vict = NULL;
  int stopped = 0;

  if (IN_ROOM(ch) == NOWHERE)
    return FALSE;

  for (vict = world[IN_ROOM(ch)].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (!FIGHTING(vict))
      continue;

    stop_fighting(vict);
    stopped++;
  }

  if (!stopped)
  {
    send_to_char(ch, "There is no violence here to still.\r\n");
    return FALSE;
  }

  act("\tC$n sounds $p, and a single clear note takes all the fight out of the room.\tn", FALSE, ch,
      obj, NULL, TO_ROOM);
  send_to_char(ch, "\tCYou sound the horn, and the fighting stops.\tn\r\n");

  return TRUE;
}

/* The Horn of Henekar's recruitment: the lesser creatures nearby fall in. */
static int artifact_charm_room(struct char_data *ch, struct obj_data *obj,
                               struct artifact_data *art)
{
  struct char_data *vict = NULL, *next_vict = NULL;
  int recruited = 0, cap = 0;

  if (IN_ROOM(ch) == NOWHERE)
    return FALSE;

  /* One at level 1, up to ARTIFACT_CHARM_MAX once the horn has grown, and
   * never more than the follower engine has room for.  ROL had no such limit
   * because it had no pet-slot accounting to overrun. */
  cap = MAX(1, (ARTIFACT_CHARM_MAX * art->level) / ARTIFACT_MAX_LEVEL);
  cap = MIN(cap, check_npc_followers(ch, NPC_MODE_SPARE, 0));

  if (cap <= 0)
  {
    send_to_char(ch, "You already command as many as will answer you.\r\n");
    return FALSE;
  }

  for (vict = world[IN_ROOM(ch)].people; vict && recruited < cap; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (vict == ch || !IS_NPC(vict))
      continue;
    if (!CAN_SEE(ch, vict))
      continue;
    if (AFF_FLAGGED(vict, AFF_CHARM) || MOB_FLAGGED(vict, MOB_NOCHARM))
      continue;
    if (GET_MAX_HIT(vict) > ARTIFACT_CHARM_MAX_HP)
      continue;
    if (GET_LEVEL(vict) > GET_LEVEL(ch))
      continue;
    if (circle_follow(vict, ch))
      continue;

    if (vict->master)
      stop_follower(vict);

    SET_BIT_AR(AFF_FLAGS(vict), AFF_CHARM);
    add_follower(vict, ch);

    act("$N falls in behind $n.", FALSE, ch, NULL, vict, TO_NOTVICT);
    act("You find yourself wanting very much to go where $n is going.", FALSE, ch, NULL, vict,
        TO_VICT);
    recruited++;
  }

  if (!recruited)
  {
    send_to_char(ch, "Nothing here is willing to follow you.\r\n");
    return FALSE;
  }

  act("\tC$n sounds $p and the note settles into the bones of everything listening.\tn", FALSE, ch,
      obj, NULL, TO_ROOM);
  send_to_char(ch, "\tCYou sound the horn, and %d answer%s.\tn\r\n", recruited,
               recruited == 1 ? "s" : "");

  return TRUE;
}

/* Doombringer's annihilation: everything hostile in the room, at once. */
static int artifact_annihilation(struct char_data *ch, struct obj_data *obj,
                                 struct artifact_data *art)
{
  struct char_data *vict = NULL, *next_vict = NULL;
  int hit_count = 0, amount = 0;

  if (IN_ROOM(ch) == NOWHERE)
    return FALSE;

  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
    if (vict != ch && CAN_SEE(ch, vict) && aoeOK(ch, vict, -1))
      hit_count++;

  if (!hit_count)
  {
    send_to_char(ch, "There is nothing here worth annihilating.\r\n");
    return FALSE;
  }

  act("\tR$n raises $p and the room turns black from the edges in!\tn", FALSE, ch, obj, NULL,
      TO_ROOM);
  send_to_char(ch, "\tRYou raise the blade and call annihilation forth!\tn\r\n");

  hit_count = 0;

  for (vict = world[IN_ROOM(ch)].people; vict; vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (vict == ch || !CAN_SEE(ch, vict) || !aoeOK(ch, vict, -1))
      continue;

    amount = dice(10 + (art->level * 4), 12) + (GET_LEVEL(ch) * 3);

    act("\tR$N comes apart.\tn", FALSE, ch, obj, vict, TO_NOTVICT);
    act("\tRYou come apart.\tn", FALSE, ch, obj, vict, TO_VICT);
    damage(ch, vict, amount, TYPE_UNDEFINED, DAM_NEGATIVE, FALSE);

    hit_count++;
  }

  return (hit_count > 0);
}

/* Doombringer's black lightning. */
static int artifact_black_lightning(struct char_data *ch, struct obj_data *obj,
                                    struct artifact_data *art, struct char_data *victim)
{
  int amount = dice(8 + (art->level * 3), 14) + (GET_LEVEL(ch) * 2);

  act("\tD$n points $p and black lightning leaps from it into $N!\tn", FALSE, ch, obj, victim,
      TO_NOTVICT);
  act("\tDYou point $p and black lightning leaps from it into $N!\tn", FALSE, ch, obj, victim,
      TO_CHAR);
  act("\tDBlack lightning arcs out of $p and into you!\tn", FALSE, ch, obj, victim, TO_VICT);

  damage(ch, victim, amount, TYPE_UNDEFINED, DAM_ELECTRIC, FALSE);

  return TRUE;
}

/* Doombringer's rage. */
static int artifact_enrage(struct char_data *ch, struct obj_data *obj, struct artifact_data *art)
{
  struct affected_type af;
  int i = 0;
  const int locations[3] = {APPLY_HITROLL, APPLY_DAMROLL, APPLY_STR};
  const int modifiers[3] = {4, 6, 4};

  for (i = 0; i < 3; i++)
  {
    new_affect(&af);
    af.spell = SPELL_RAGE;
    af.duration = ARTIFACT_ENRAGE_DURATION;
    af.location = locations[i];
    af.modifier = modifiers[i] + art->level;
    af.bonus_type = BONUS_TYPE_MORALE;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }

  act("\tR$n's eyes go flat and $e stops looking like $e knows you.\tn", FALSE, ch, obj, NULL,
      TO_ROOM);
  send_to_char(ch, "\tRDoombringer takes the rest of you and gives back only the killing.\tn\r\n");

  return TRUE;
}

/* Resolve whatever this effect needs to act on, then run it. */
static int artifact_do_effect(struct char_data *ch, struct obj_data *obj, struct artifact_data *art,
                              const struct artifact_effect *effect, const char *target_arg)
{
  struct char_data *victim = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int level = artifact_effect_level(ch, art);

  strlcpy(arg, target_arg ? target_arg : "", sizeof(arg));

  /* Effects that name a character in the room resolve it once, here. */
  if (effect->target_type == ART_TARGET_CHAR_ROOM)
  {
    if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    {
      send_to_char(ch, "There is nobody here by that name.\r\n");
      return FALSE;
    }
    if (victim == ch)
    {
      send_to_char(ch, "You would regret turning that on yourself.\r\n");
      return FALSE;
    }
    if (!aoeOK(ch, victim, -1))
    {
      send_to_char(ch, "You can't do that to them.\r\n");
      return FALSE;
    }
  }
  else if (effect->target_type == ART_TARGET_FIGHTING)
  {
    if (!(victim = FIGHTING(ch)))
    {
      send_to_char(ch, "There is nothing here to take hold of.\r\n");
      return FALSE;
    }
  }

  switch (effect->effect)
  {
  case ART_EFFECT_SUMMON_TREANT:
    return artifact_summon_treant(ch, obj, art);

  case ART_EFFECT_CREEPING_DOOM:
    act("\tg$n lowers $p and the ground begins to move.\tn", FALSE, ch, obj, NULL, TO_ROOM);
    send_to_char(ch, "\tgYou lower the staff and the ground begins to move.\tn\r\n");
    call_magic(ch, NULL, NULL, SPELL_CREEPING_DOOM, 0, level, CAST_WEAPON_SPELL);
    return TRUE;

  case ART_EFFECT_RECALL:
    act("\tg$n steps between two trees that were not there a moment ago.\tn", FALSE, ch, obj, NULL,
        TO_ROOM);
    send_to_char(ch, "\tgThe forest opens a path home and you take it.\tn\r\n");
    call_magic(ch, ch, NULL, SPELL_WORD_OF_RECALL, 0, level, CAST_WEAPON_SPELL);
    return TRUE;

  case ART_EFFECT_TRAVEL_TO:
    return artifact_travel_to(ch, obj, arg);

  case ART_EFFECT_DIMENSION_SHIFT:
    return artifact_dimension_shift(ch, obj);

  case ART_EFFECT_RESURRECT:
    return artifact_resurrect_corpse(ch, obj, arg);

  case ART_EFFECT_DISPEL_EVIL:
    act("\tW$n turns $p on $N and the light comes out of it like a verdict.\tn", FALSE, ch, obj,
        victim, TO_NOTVICT);
    act("\tWYou turn $p on $N.\tn", FALSE, ch, obj, victim, TO_CHAR);
    call_magic(ch, victim, NULL, SPELL_DISPEL_EVIL, 0, level, CAST_WEAPON_SPELL);
    return TRUE;

  case ART_EFFECT_BLIND:
    act("\tD$n speaks, and $N claws at $S own eyes.\tn", FALSE, ch, obj, victim, TO_NOTVICT);
    act("\tDYou speak, and $N claws at $S own eyes.\tn", FALSE, ch, obj, victim, TO_CHAR);
    call_magic(ch, victim, NULL, SPELL_BLINDNESS, 0, level, CAST_WEAPON_SPELL);
    return TRUE;

  case ART_EFFECT_DARKNESS:
    act("\tD$n says something quiet and the light goes out of the room.\tn", FALSE, ch, obj, NULL,
        TO_ROOM);
    send_to_char(ch, "\tDYou say something quiet and the light goes out of the room.\tn\r\n");
    call_magic(ch, NULL, NULL, SPELL_DARKNESS, 0, level, CAST_WEAPON_SPELL);
    return TRUE;

  case ART_EFFECT_WEAKEN:
    act("\tD$p drinks something out of $N that $E is not going to get back.\tn", FALSE, ch, obj,
        victim, TO_NOTVICT);
    act("\tD$p drinks something out of $N that $E is not going to get back.\tn", FALSE, ch, obj,
        victim, TO_CHAR);
    act("\tDSomething leaves you, and it does not come back.\tn", FALSE, ch, obj, victim, TO_VICT);
    call_magic(ch, victim, NULL, SPELL_ENFEEBLEMENT, 0, level, CAST_WEAPON_SPELL);
    return TRUE;

  case ART_EFFECT_PACIFY:
    return artifact_pacify_room(ch, obj);

  case ART_EFFECT_CHARM:
    return artifact_charm_room(ch, obj, art);

  case ART_EFFECT_ANNIHILATION:
    return artifact_annihilation(ch, obj, art);

  case ART_EFFECT_BLACK_LIGHTNING:
    return artifact_black_lightning(ch, obj, art, victim);

  case ART_EFFECT_ENRAGE:
    return artifact_enrage(ch, obj, art);

  default:
    log("SYSERR: artifact_do_effect: unknown effect %d on vnum %d", effect->effect, art->vnum);
    return FALSE;
  }
}

/* Is this artifact close enough to hear its wielder?  ROL required the object
 * to be on your person; so do we. */
static struct obj_data *artifact_held_instance(struct char_data *ch, int vnum)
{
  struct obj_data *obj = NULL;
  int i = 0;

  for (i = 0; i < NUM_WEARS; i++)
    if ((obj = GET_EQ(ch, i)) && (int)GET_OBJ_VNUM(obj) == vnum)
      return obj;

  for (obj = ch->carrying; obj; obj = obj->next_content)
    if ((int)GET_OBJ_VNUM(obj) == vnum)
      return obj;

  return NULL;
}

/* Hooked into do_say().  Returns TRUE when the speech invoked something, but
 * the speech itself is never suppressed - saying the words aloud is the
 * point. */
int artifact_speech_trigger(struct char_data *ch, const char *speech)
{
  char said[MAX_INPUT_LENGTH] = {'\0'};
  const struct artifact_effect *effect = NULL;
  struct artifact_data *art = NULL;
  struct obj_data *obj = NULL;
  const char *target_arg = NULL;
  size_t phrase_len = 0;
  int i = 0, remaining = 0;

  if (!ch || IS_NPC(ch) || !speech || !art_index)
    return FALSE;

  artifact_normalize_speech(said, sizeof(said), speech);

  if (!*said)
    return FALSE;

  for (i = 0; artifact_effects[i].vnum != -1; i++)
  {
    effect = &artifact_effects[i];
    phrase_len = strlen(effect->phrase);

    if (strncmp(said, effect->phrase, phrase_len) != 0)
      continue;

    if (effect->target_type == ART_TARGET_NONE || effect->target_type == ART_TARGET_FIGHTING)
    {
      if (said[phrase_len] != '\0')
        continue;
      target_arg = "";
    }
    else
    {
      if (said[phrase_len] != ' ')
        continue;

      target_arg = said + phrase_len + 1;

      if (!*target_arg)
        continue;
    }

    if (!(obj = artifact_held_instance(ch, effect->vnum)))
      continue;

    if (!(art = artifact_of_obj(obj)))
      continue;

    if (!artifact_can_use(ch, obj, FALSE))
      return FALSE;

    if (!artifact_class_ok(ch, art))
    {
      send_to_char(ch, "\tr%s hears you, and does not care what you want.\tn\r\n",
                   GET_OBJ_SHORT(obj));
      return FALSE;
    }

    if (effect->slot < 0 || effect->slot >= ARTIFACT_MAX_EFFECTS)
    {
      log("SYSERR: artifact_speech_trigger: vnum %d effect slot %d out of range", effect->vnum,
          effect->slot);
      return FALSE;
    }

    if ((remaining = artifact_recharge_remaining(art, effect->slot)) > 0)
    {
      send_to_char(ch, "%s is spent; that power returns in %d minute%s.\r\n", GET_OBJ_SHORT(obj),
                   MAX(1, remaining / 60), (remaining / 60) == 1 ? "" : "s");
      return FALSE;
    }

    if (!artifact_do_effect(ch, obj, art, effect, target_arg))
      return FALSE;

    art->effect_used[effect->slot] = time(0);
    artifact_grant_xp_obj(ch, obj, ARTIFACT_XP_CALLED_EFFECT);

    return TRUE;
  }

  return FALSE;
}

/* --------------------------------------------------------------------------
 * Player command: artifact
 * -------------------------------------------------------------------------- */

static void artifact_show_help(struct char_data *ch)
{
  send_to_char(ch,
               "\tY========== Artifact System ==========\tn\r\n"
               "\r\n"
               "\tcArtifacts\tn are unique items of power. Each one exists only\r\n"
               "once in the world, and each grows stronger as it is used.\r\n"
               "\r\n"
               "\tYCommands:\tn\r\n"
               "  artifact              - show this help\r\n"
               "  artifact list         - list the artifacts you carry\r\n"
               "  artifact info <item>  - detailed information on one artifact\r\n"
               "  artifact progress     - level and experience of your artifacts\r\n"
               "  artifact abilities    - abilities you can invoke right now\r\n"
               "\r\n"
               "\tYBinding:\tn\r\n"
               "  None            - can be freely traded\r\n"
               "  Bind on Pickup  - soulbound the moment you take it\r\n"
               "  Bind on Equip   - bound the first time you wear it\r\n"
               "  Bind on Account - usable by any character on your account\r\n"
               "\r\n"
               "\tYCalled effects:\tn\r\n"
               "  Some artifacts answer to words said aloud. Hold or wear the\r\n"
               "  artifact and simply \tcsay\tn the phrase. Each effect recharges on\r\n"
               "  its own clock, from once an hour to once a week.\r\n"
               "  \tcartifact info <item>\tn lists the phrases an artifact will\r\n"
               "  answer to, if it is willing to tell you.\r\n"
               "\r\n"
               "\tYOaths:\tn\r\n"
               "  A few artifacts are sworn to one class and demand real depth\r\n"
               "  in it. Wear one without that depth and it will burn you every\r\n"
               "  tick you keep it on, and it will not answer when you speak.\r\n"
               "\r\n"
               "\tYProgression:\tn\r\n"
               "  Artifacts gain experience from combat and from their own\r\n"
               "  abilities. As they level (1-%d) every bonus they grant grows,\r\n"
               "  and their special effects become more dangerous.\r\n"
               "\tY=====================================\tn\r\n",
               ARTIFACT_MAX_LEVEL);
}

/* ROL's PROC_SPECIAL_ID: the bespoke, class-gated identify text listing an
 * artifact's called effects and how often each may be used.  Reproduced here
 * from the same table the effects themselves are dispatched from, so the two
 * cannot drift apart. */
static void artifact_show_called_effects(struct char_data *ch, struct artifact_data *art)
{
  const struct artifact_effect *effect = NULL;
  int i = 0, found = 0, remaining = 0, gated = FALSE;

  for (i = 0; artifact_effects[i].vnum != -1; i++)
    if (artifact_effects[i].vnum == art->vnum)
      found++;

  if (!found)
    return;

  gated = !artifact_class_ok(ch, art);

  send_to_char(ch, "\r\n\tYCalled effects:\tn\r\n");

  if (gated)
  {
    send_to_char(ch, "  It keeps its own counsel. Whatever words wake it, they are\r\n"
                     "  not for you.\r\n");
    return;
  }

  for (i = 0; artifact_effects[i].vnum != -1; i++)
  {
    effect = &artifact_effects[i];

    if (effect->vnum != art->vnum)
      continue;

    send_to_char(
        ch, "  \tc\"%s%s\"\tn\r\n", effect->phrase,
        (effect->target_type == ART_TARGET_NONE || effect->target_type == ART_TARGET_FIGHTING)
            ? ""
            : " <target>");
    send_to_char(ch, "      %s - %s", effect->desc, artifact_recharge_name(effect->recharge));

    remaining = artifact_recharge_remaining(art, effect->slot);

    if (remaining > 0)
      send_to_char(ch, ", \trspent for another %d minute%s\tn\r\n", MAX(1, remaining / 60),
                   (remaining / 60) == 1 ? "" : "s");
    else
      send_to_char(ch, ", \tgready\tn\r\n");
  }
}

static void artifact_show_info(struct char_data *ch, struct obj_data *obj)
{
  struct artifact_data *art = NULL;
  int i = 0;

  if (!(art = artifact_of_obj(obj)))
  {
    send_to_char(ch, "That is not an artifact.\r\n");
    return;
  }

  send_to_char(ch, "\tY===== %s \tY=====\tn\r\n", GET_OBJ_SHORT(obj));
  send_to_char(ch, "\tcLevel:   \tW%d\tn of %d\r\n", art->level, ARTIFACT_MAX_LEVEL);
  send_to_char(ch, "\tcOwner:   \tW%s\tn\r\n",
               artifact_is_owned(art->vnum) ? art->owner : "unclaimed");
  send_to_char(ch, "\tcBinding: \tW%s\tn%s\r\n", artifact_binding_name(art->binding_type),
               art->bound_time ? " (already bound)" : "");

  send_to_char(ch, "\r\n\tYBonuses at this level:\tn\r\n");
  for (i = 0; i < ARTIFACT_NUM_STATS; i++)
    if (art->stat_bonus[i] > 0)
      send_to_char(ch, "  \tc%-14s \tW+%d\tn\r\n", artifact_stat_names[i],
                   art->stat_bonus[i] * art->level);

  if (art->hitroll_bonus > 0)
    send_to_char(ch, "  \tc%-14s \tW+%d\tn\r\n", "Hitroll", art->hitroll_bonus * art->level);
  if (art->damroll_bonus > 0)
    send_to_char(ch, "  \tc%-14s \tW+%d\tn\r\n", "Damroll", art->damroll_bonus * art->level);
  if (art->ac_bonus > 0)
    send_to_char(ch, "  \tc%-14s \tW-%d\tn\r\n", "Armor Class", art->ac_bonus * art->level);
  if (art->hp_bonus > 0)
    send_to_char(ch, "  \tc%-14s \tW+%d\tn\r\n", "Hit Points", art->hp_bonus * art->level);
  if (art->psp_bonus > 0)
    send_to_char(ch, "  \tc%-14s \tW+%d\tn\r\n", "PSP", art->psp_bonus * art->level);
  if (art->move_bonus > 0)
    send_to_char(ch, "  \tc%-14s \tW+%d\tn\r\n", "Movement", art->move_bonus * art->level);

  if (art->resist_physical > 0 || art->resist_magical > 0 || art->resist_element > 0)
  {
    send_to_char(ch, "\r\n\tYResistances:\tn\r\n");
    if (art->resist_physical > 0)
      send_to_char(ch, "  \tc%-14s \tW%d%%\tn\r\n", "Physical", art->resist_physical);
    if (art->resist_magical > 0)
      send_to_char(ch, "  \tc%-14s \tW%d%%\tn\r\n", "Magical", art->resist_magical);
    if (art->resist_element > 0)
      send_to_char(ch, "  \tc%-14s \tW%d%%\tn\r\n", "Elemental", art->resist_element);
  }

  if (art->proc_chance > 0)
    send_to_char(ch, "\r\n\tYCombat:\tn %d%% chance per hit to unleash a special strike.\r\n",
                 art->proc_chance);

  if (art->ability_name)
  {
    send_to_char(ch, "\r\n\tYAbility:\tn \tc%s\tn - %s\r\n", art->ability_name,
                 art->ability_desc ? art->ability_desc : "");
    send_to_char(ch, "  Cooldown %d seconds, costs %d psp.\r\n", art->ability_cooldown,
                 art->ability_cost);
  }

  artifact_show_called_effects(ch, art);

  if (art->class_restrict != CLASS_UNDEFINED)
  {
    send_to_char(ch, "\r\n\tYSworn to:\tn %s, %d level%s deep.\r\n",
                 CLSLIST_NAME(art->class_restrict), art->class_min_level,
                 art->class_min_level == 1 ? "" : "s");

    if (!artifact_class_ok(ch, art))
      send_to_char(ch, "  \trIt does not recognize you, and it burns you for holding it.\tn\r\n");
  }

  send_to_char(ch, "\tY=====================================\tn\r\n");
}

static void artifact_show_list(struct char_data *ch)
{
  struct obj_data *obj = NULL;
  int i = 0, found = 0;

  send_to_char(ch, "\tY========== Your Artifacts ==========\tn\r\n");

  send_to_char(ch, "\r\n\tcEquipped:\tn\r\n");
  for (i = 0; i < NUM_WEARS; i++)
    if ((obj = GET_EQ(ch, i)) && artifact_is_artifact(obj))
    {
      send_to_char(ch, "  \tW%-22s\tn %s\r\n", wear_where[i], GET_OBJ_SHORT(obj));
      found++;
    }
  if (!found)
    send_to_char(ch, "  None equipped.\r\n");

  send_to_char(ch, "\r\n\tcCarried:\tn\r\n");
  found = 0;
  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (artifact_is_artifact(obj))
    {
      send_to_char(ch, "  %s\r\n", GET_OBJ_SHORT(obj));
      found++;
    }
  if (!found)
    send_to_char(ch, "  None carried.\r\n");

  send_to_char(ch, "\tY====================================\tn\r\n");
}

static void artifact_show_one_progress(struct char_data *ch, struct obj_data *obj,
                                       const char *suffix)
{
  struct artifact_data *art = NULL;
  int needed = 0, filled = 0, j = 0;

  if (!(art = artifact_of_obj(obj)))
    return;

  send_to_char(ch, "\r\n\tW%s\tn%s\r\n", GET_OBJ_SHORT(obj), suffix);

  if (art->level >= ARTIFACT_MAX_LEVEL)
  {
    send_to_char(ch, "  Level %d \tY(maximum)\tn\r\n", art->level);
    return;
  }

  needed = artifact_xp_to_next(art->level);
  send_to_char(ch, "  Level %d - %d/%d experience to the next level\r\n", art->level,
               art->experience, needed);

  filled = MIN(20, (art->experience * 20) / needed);

  send_to_char(ch, "  [");
  for (j = 0; j < 20; j++)
    send_to_char(ch, "%s", j < filled ? "\tY=\tn" : "-");
  send_to_char(ch, "] %d%%\r\n", MIN(100, (art->experience * 100) / needed));
}

static void artifact_show_progress(struct char_data *ch)
{
  struct obj_data *obj = NULL;
  int i = 0, found = 0;

  send_to_char(ch, "\tY========== Artifact Progression ==========\tn\r\n");

  for (i = 0; i < NUM_WEARS; i++)
    if ((obj = GET_EQ(ch, i)) && artifact_is_artifact(obj))
    {
      artifact_show_one_progress(ch, obj, "");
      found++;
    }

  for (obj = ch->carrying; obj; obj = obj->next_content)
    if (artifact_is_artifact(obj))
    {
      artifact_show_one_progress(ch, obj, " \tc(carried)\tn");
      found++;
    }

  if (!found)
    send_to_char(ch, "\r\nYou have no artifacts.\r\n");

  send_to_char(ch, "\tY=========================================\tn\r\n");
}

static void artifact_show_abilities(struct char_data *ch)
{
  struct obj_data *obj = NULL;
  struct artifact_data *art = NULL;
  int i = 0, found = 0, remaining = 0;

  send_to_char(ch, "\tY========== Artifact Abilities ==========\tn\r\n");

  for (i = 0; i < NUM_WEARS; i++)
  {
    if (!(obj = GET_EQ(ch, i)))
      continue;
    if (!(art = artifact_of_obj(obj)))
      continue;
    if (!art->ability_name)
      continue;

    send_to_char(ch, "  \tc%-12s\tn %s\r\n", art->ability_name,
                 art->ability_desc ? art->ability_desc : "");

    remaining = (int)(art->ability_cooldown - (time(0) - art->last_ability_use));
    if (art->last_ability_use > 0 && remaining > 0)
      send_to_char(ch, "               \tRready in %d second%s\tn, %d psp\r\n", remaining,
                   remaining == 1 ? "" : "s", art->ability_cost);
    else
      send_to_char(ch, "               \tGready\tn, %d psp\r\n", art->ability_cost);

    found++;
  }

  if (!found)
    send_to_char(ch, "  You have no artifact abilities equipped.\r\n");

  send_to_char(ch, "\tY=======================================\tn\r\n");
}

ACMD(do_artifact)
{
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *obj = NULL;
  const char *rest = NULL;
  int i = 0;

  if (!ch || IS_NPC(ch))
    return;

  rest = one_argument(argument, arg1, sizeof(arg1));
  one_argument(rest, arg2, sizeof(arg2));

  if (!*arg1 || !str_cmp(arg1, "help"))
  {
    artifact_show_help(ch);
    return;
  }

  if (is_abbrev(arg1, "list"))
  {
    artifact_show_list(ch);
    return;
  }

  if (is_abbrev(arg1, "progress"))
  {
    artifact_show_progress(ch);
    return;
  }

  if (is_abbrev(arg1, "abilities"))
  {
    artifact_show_abilities(ch);
    return;
  }

  if (is_abbrev(arg1, "info"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Information about which artifact?\r\n");
      return;
    }

    obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying);

    if (!obj)
      for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(ch, i) && isname(arg2, GET_EQ(ch, i)->name))
        {
          obj = GET_EQ(ch, i);
          break;
        }

    if (!obj)
    {
      send_to_char(ch, "You don't have that.\r\n");
      return;
    }

    artifact_show_info(ch, obj);
    return;
  }

  send_to_char(ch, "Usage: artifact [list | info <item> | progress | abilities | help]\r\n");
}

/* --------------------------------------------------------------------------
 * Artifact abilities
 *
 * Each ability is its own command; the command name selects which equipped
 * artifact answers.
 * -------------------------------------------------------------------------- */

static int artifact_ability_ready(struct char_data *ch, struct obj_data *obj,
                                  struct artifact_data *art)
{
  int remaining = 0;

  if (!artifact_can_use(ch, obj, FALSE))
    return FALSE;

  if (art->last_ability_use > 0)
  {
    remaining = (int)(art->ability_cooldown - (time(0) - art->last_ability_use));
    if (remaining > 0)
    {
      send_to_char(ch, "That ability is not ready for another %d second%s.\r\n", remaining,
                   remaining == 1 ? "" : "s");
      return FALSE;
    }
  }

  if (GET_PSP(ch) < art->ability_cost)
  {
    send_to_char(ch, "You lack the psionic energy to invoke that.\r\n");
    return FALSE;
  }

  return TRUE;
}

static void artifact_ability_spend(struct char_data *ch, struct artifact_data *art)
{
  GET_PSP(ch) -= art->ability_cost;
  art->last_ability_use = time(0);
}

static void artifact_ability_soulstrike(struct char_data *ch, struct obj_data *obj,
                                        struct artifact_data *art, const char *argument)
{
  struct char_data *victim = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  int amount = 0;

  one_argument(argument, arg, sizeof(arg));

  if (!*arg)
  {
    if (!(victim = FIGHTING(ch)))
    {
      send_to_char(ch, "Strike whom with soul energy?\r\n");
      return;
    }
  }
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "They aren't here.\r\n");
    return;
  }

  if (victim == ch)
  {
    send_to_char(ch, "You can't turn that on yourself.\r\n");
    return;
  }

  if (!aoeOK(ch, victim, -1))
  {
    send_to_char(ch, "You can't attack them.\r\n");
    return;
  }

  act("\tW$n raises $p high, channeling soul energy!\tn", FALSE, ch, obj, NULL, TO_ROOM);
  act("\tWYou channel soul energy through $p!\tn", FALSE, ch, obj, NULL, TO_CHAR);
  act("\tRA bolt of pure soul energy strikes $N!\tn", FALSE, ch, obj, victim, TO_NOTVICT);
  act("\tRA bolt of soul energy strikes you!\tn", FALSE, ch, obj, victim, TO_VICT);

  amount = dice(5 + art->level, 20) + (art->level * 20) + (GET_LEVEL(ch) * 2);

  artifact_ability_spend(ch, art);
  artifact_grant_xp_obj(ch, obj, ARTIFACT_XP_ABILITY_SOULSTRIKE);

  damage(ch, victim, amount, TYPE_UNDEFINED, DAM_NEGATIVE, FALSE);
}

static void artifact_ability_divineward(struct char_data *ch, struct obj_data *obj,
                                        struct artifact_data *art, const char *argument)
{
  struct affected_type af;

  (void)argument;

  if (AFF_FLAGGED(ch, AFF_SANCTUARY))
  {
    send_to_char(ch, "You are already protected by divine power.\r\n");
    return;
  }

  new_affect(&af);
  af.spell = SPELL_SANCTUARY;
  af.duration = 5 + art->level;
  SET_BIT_AR(af.bitvector, AFF_SANCTUARY);
  affect_to_char(ch, &af);

  act("\tW$n is surrounded by a divine protective aura!\tn", FALSE, ch, obj, NULL, TO_ROOM);
  send_to_char(ch, "\tWA divine ward surrounds you with protective energy!\tn\r\n");

  artifact_ability_spend(ch, art);
  artifact_grant_xp_obj(ch, obj, ARTIFACT_XP_ABILITY_DIVINEWARD);
}

static void artifact_ability_doomblast(struct char_data *ch, struct obj_data *obj,
                                       struct artifact_data *art, const char *argument)
{
  struct char_data *vict = NULL, *next_vict = NULL;
  int amount = 0, targets = 0;

  (void)argument;

  if (IN_ROOM(ch) == NOWHERE)
    return;

  /* Count valid targets before spending anything. */
  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
    if (vict != ch && CAN_SEE(ch, vict) && aoeOK(ch, vict, -1))
      targets++;

  if (targets == 0)
  {
    send_to_char(ch, "There are no valid targets here.\r\n");
    return;
  }

  act("\tR$n raises $p and unleashes a wave of doom!\tn", FALSE, ch, obj, NULL, TO_ROOM);
  send_to_char(ch, "\tRYou unleash a devastating blast of doom energy!\tn\r\n");

  artifact_ability_spend(ch, art);

  targets = 0;
  for (vict = world[IN_ROOM(ch)].people; vict && targets < ARTIFACT_DOOMBLAST_MAX_TARGETS;
       vict = next_vict)
  {
    next_vict = vict->next_in_room;

    if (vict == ch || !CAN_SEE(ch, vict) || !aoeOK(ch, vict, -1))
      continue;

    amount = dice(3 + art->level, 15) + GET_LEVEL(ch);

    act("\tR$N is blasted by doom energy!\tn", FALSE, ch, obj, vict, TO_NOTVICT);
    act("\tRYou are blasted by doom energy!\tn", FALSE, ch, obj, vict, TO_VICT);
    damage(ch, vict, amount, TYPE_UNDEFINED, DAM_NEGATIVE, FALSE);

    targets++;
  }

  artifact_grant_xp_obj(ch, obj, ARTIFACT_XP_ABILITY_DOOMBLAST * targets);
}

ACMD(do_artifact_ability)
{
  struct obj_data *obj = NULL;
  struct artifact_data *art = NULL, *found = NULL;
  const char *ability = NULL;
  int i = 0;

  if (!ch || IS_NPC(ch))
    return;

  ability = CMD_NAME;

  for (i = 0; i < NUM_WEARS; i++)
  {
    if (!GET_EQ(ch, i))
      continue;

    art = artifact_of_obj(GET_EQ(ch, i));
    if (!art || !art->ability_name)
      continue;

    if (!str_cmp(art->ability_name, ability))
    {
      obj = GET_EQ(ch, i);
      found = art;
      break;
    }
  }

  if (!obj || !found)
  {
    send_to_char(ch, "You have no artifact equipped that grants that power.\r\n");
    return;
  }

  if (!artifact_ability_ready(ch, obj, found))
    return;

  if (!str_cmp(ability, "soulstrike"))
    artifact_ability_soulstrike(ch, obj, found, argument);
  else if (!str_cmp(ability, "divineward"))
    artifact_ability_divineward(ch, obj, found, argument);
  else if (!str_cmp(ability, "doomblast"))
    artifact_ability_doomblast(ch, obj, found, argument);
  else
    send_to_char(ch, "That artifact ability is not implemented.\r\n");
}

/* --------------------------------------------------------------------------
 * Staff command: testartifact
 * -------------------------------------------------------------------------- */

/* ROL's third ownership state.  An artifact is "dropped" when it still has an
 * owner on record but no live instance is on that owner's person - it is lying
 * in a room, sitting in a container, or simply not in play.  Ours are all
 * bound artifacts that were set down: they keep their owner but release the
 * instance so a zone reset can recover them. */
int artifact_is_dropped(struct artifact_data *art)
{
  struct obj_data *obj = NULL, *outer = NULL;

  if (!art || !artifact_is_owned(art->vnum))
    return FALSE;

  for (obj = object_list; obj; obj = obj->next)
  {
    if ((int)GET_OBJ_VNUM(obj) != art->vnum)
      continue;

    /* Held or worn by anyone at all, however deeply nested - not dropped. */
    for (outer = obj; outer->in_obj; outer = outer->in_obj)
      ;

    if (outer->worn_by || outer->carried_by)
      return FALSE;
  }

  return TRUE;
}

static const char *artifact_locate(struct artifact_data *art)
{
  struct obj_data *obj = NULL;

  for (obj = object_list; obj; obj = obj->next)
  {
    if ((int)GET_OBJ_VNUM(obj) != art->vnum)
      continue;

    if (obj->worn_by)
      return "worn";
    if (obj->carried_by)
      return "carried";
    if (obj->in_obj)
      return "in container";
    if (IN_ROOM(obj) != NOWHERE)
      return "in a room";

    return "limbo";
  }

  return "not in play";
}

ACMD(do_testartifact)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  char arg2[MAX_INPUT_LENGTH] = {'\0'};
  struct obj_data *obj = NULL;
  struct artifact_data *art = NULL;
  const char *rest = NULL;
  int i = 0, owned = 0, dropped = 0, unowned = 0, count = 0, vnum = 0, ok = TRUE;

  if (!ch)
    return;

  rest = one_argument(argument, arg, sizeof(arg));
  one_argument(rest, arg2, sizeof(arg2));

  if (!*arg)
  {
    send_to_char(ch, "Usage: testartifact <status|verify|save|reload|spawn|list|reset>\r\n"
                     "  status       - artifact system status\r\n"
                     "  verify       - check for duplicates and bad records\r\n"
                     "  save         - write the artifact file now\r\n"
                     "  reload       - re-read the artifact file\r\n"
                     "  spawn <vnum> - create an artifact here\r\n"
                     "  list         - every artifact, owner, and location\r\n"
                     "  reset <vnum> - clear ownership and binding on one artifact\r\n");
    return;
  }

  if (!art_index || total_artifacts == 0)
  {
    send_to_char(ch, "The artifact system is not initialized.\r\n");
    return;
  }

  if (is_abbrev(arg, "status"))
  {
    for (i = 0; i < total_artifacts; i++)
    {
      if (!artifact_is_owned(art_index[i].vnum))
        unowned++;
      else if (artifact_is_dropped(&art_index[i]))
        dropped++;
      else
        owned++;
    }

    send_to_char(ch, "\tYArtifact System Status\tn\r\n");
    send_to_char(ch, "----------------------\r\n");
    send_to_char(ch, "Total artifacts : %d\r\n", total_artifacts);
    send_to_char(ch, "Owned           : %d\r\n", owned);
    send_to_char(ch, "Dropped         : %d\r\n", dropped);
    send_to_char(ch, "Unowned         : %d\r\n", unowned);
    send_to_char(ch, "Registry memory : %lu bytes\r\n", (unsigned long)artifact_memory_used());
    send_to_char(ch, "Data file       : %s\r\n", ARTIFACT_FILE);
    return;
  }

  if (is_abbrev(arg, "verify"))
  {
    send_to_char(ch, "\tYVerifying artifact integrity\tn\r\n");
    send_to_char(ch, "----------------------------\r\n");

    for (i = 0; i < total_artifacts; i++)
    {
      count = 0;
      for (obj = object_list; obj; obj = obj->next)
        if ((int)GET_OBJ_VNUM(obj) == art_index[i].vnum)
          count++;

      if (count > 1)
      {
        send_to_char(ch, "\tr%d instances of artifact %d are in play!\tn\r\n", count,
                     art_index[i].vnum);
        ok = FALSE;
      }

      if (!art_index[i].owner)
      {
        send_to_char(ch, "\trArtifact %d has a NULL owner string.\tn\r\n", art_index[i].vnum);
        ok = FALSE;
      }

      if (art_index[i].level < 1 || art_index[i].level > ARTIFACT_MAX_LEVEL)
      {
        send_to_char(ch, "\trArtifact %d has out-of-range level %d.\tn\r\n", art_index[i].vnum,
                     art_index[i].level);
        ok = FALSE;
      }

      if (real_object(art_index[i].vnum) == NOTHING)
      {
        send_to_char(ch, "\trArtifact %d has no object prototype.\tn\r\n", art_index[i].vnum);
        ok = FALSE;
      }
    }

    if (ok)
      send_to_char(ch, "\tgAll %d artifacts verified.\tn\r\n", total_artifacts);
    else
      send_to_char(ch, "\trVerification found problems.\tn\r\n");
    return;
  }

  if (is_abbrev(arg, "save"))
  {
    artifact_save();
    send_to_char(ch, "Artifacts saved to %s.\r\n", ARTIFACT_FILE);
    return;
  }

  if (is_abbrev(arg, "reload"))
  {
    artifact_reload();
    send_to_char(ch, "Artifacts reloaded. Total: %d\r\n", total_artifacts);
    return;
  }

  if (is_abbrev(arg, "spawn"))
  {
    vnum = atoi(arg2);

    if (vnum <= 0)
    {
      send_to_char(ch, "Usage: testartifact spawn <vnum>\r\n");
      return;
    }

    if (artifact_search(vnum) < 0)
    {
      send_to_char(ch, "%d is not an artifact vnum.\r\n", vnum);
      return;
    }

    for (obj = object_list; obj; obj = obj->next)
      if ((int)GET_OBJ_VNUM(obj) == vnum)
      {
        send_to_char(ch, "That artifact is already in play.\r\n");
        return;
      }

    if (!(obj = read_object(vnum, VIRTUAL)))
    {
      send_to_char(ch, "Failed to create that artifact.\r\n");
      return;
    }

    obj_to_room(obj, IN_ROOM(ch));
    send_to_char(ch, "Spawned %s.\r\n", GET_OBJ_SHORT(obj));
    act("$n reaches into the weave and draws forth $p!", FALSE, ch, obj, NULL, TO_ROOM);
    return;
  }

  if (is_abbrev(arg, "reset"))
  {
    vnum = atoi(arg2);

    if (!(art = artifact_by_vnum(vnum)))
    {
      send_to_char(ch, "Usage: testartifact reset <artifact vnum>\r\n");
      return;
    }

    artifact_set_owner(art, NULL);
    art->bound_time = 0;
    art->ch = NULL;
    art->instance_persisted = FALSE;
    artifact_mark_dirty();
    artifact_save();

    mudlog(BRF, LVL_STAFF, TRUE, "ARTIFACT: %s reset ownership of artifact %d", GET_NAME(ch), vnum);
    send_to_char(ch, "Artifact %d is unowned and unbound again.\r\n", vnum);
    return;
  }

  if (is_abbrev(arg, "list"))
  {
    send_to_char(ch,
                 "\tY Vnum  Lv  Owner                State    Binding            Location\tn\r\n");
    send_to_char(ch,
                 "------  --  -------------------  -------  -----------------  ------------\r\n");

    for (i = 0; i < total_artifacts; i++)
      send_to_char(
          ch, "%6d  %2d  %-19s  %-7s  %-17s  %s\r\n", art_index[i].vnum, art_index[i].level,
          artifact_is_owned(art_index[i].vnum) ? art_index[i].owner : "-",
          !artifact_is_owned(art_index[i].vnum) ? "unowned"
          : artifact_is_dropped(&art_index[i])  ? "dropped"
                                                : "owned",
          artifact_binding_name(art_index[i].binding_type), artifact_locate(&art_index[i]));
    return;
  }

  send_to_char(ch, "Unknown subcommand. Type 'testartifact' for usage.\r\n");
}

/*EOF*/
