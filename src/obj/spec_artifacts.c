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
#include "magic/spells.h"
#include "combat/fight.h"
#include "screen.h"
#include "lists.h"
#include "act.h"
#include "character/class.h"
#include "character/evolutions.h"
#include "olc/oasis.h"
#include "dgscript/dg_scripts.h"
#include "campaign.h"
#include "magic/domains_schools.h"
#include "spec_artifacts.h"

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

  /* Reusable signature proc.  ART_SIG_NONE means either none at all or one
   * of the seven hand-written procedures dispatched by vnum. */
  int sig_proc;   /* ART_SIG_*                                   */
  int sig_chance; /* percent per successful hit                  */
  int sig_align;  /* ART_ALIGN_*                                 */
};

/* Levels required in an artifact's class before it stops burning you.  ROL
 * asked only "is that your class"; LuminariMUD is multi-class, so the gate is
 * a depth of commitment instead. */
#define ART_CLASS_GATE 10

/* clang-format off */
static const struct artifact_template artifact_templates[] = {
    /* vnum, ability, desc, cd, cost, binding,
       {str,int,wis,dex,con,cha}, hit, dam, ac, hp, psp, mv, rphys, rmag, relem, proc,
       class, class level, sig proc, sig chance, sig alignment rule */

    {ART_VNUM_TRORXEK, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {0, 0, 2, 0, 2, 0}, 2, 2, 0, 25, 30, 0, 0, 10, 0, 12, CLASS_DRUID, ART_CLASS_GATE,
     ART_SIG_NONE, 0, ART_ALIGN_ANY},

    {ART_VNUM_AMAUKEKEL, "divineward", "Wraps you in a sanctuary of divine light", 600, 100,
     ARTIFACT_BIND_ON_EQUIP,
     {0, 2, 2, 0, 0, 0}, 1, 1, 2, 0, 50, 0, 0, 15, 0, 0, CLASS_CLERIC, ART_CLASS_GATE,
     ART_SIG_NONE, 0, ART_ALIGN_ANY},

    {ART_VNUM_FADE, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {0, 0, 0, 3, 0, 0}, 4, 2, 1, 20, 0, 25, 5, 0, 0, 16, CLASS_ROGUE, ART_CLASS_GATE,
     ART_SIG_NONE, 0, ART_ALIGN_ANY},

    {ART_VNUM_HENEKAR, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {0, 2, 1, 0, 0, 2}, 0, 0, 0, 0, 75, 0, 0, 15, 0, 0, CLASS_ROGUE, ART_CLASS_GATE,
     ART_SIG_NONE, 0, ART_ALIGN_ANY},

    {ART_VNUM_DOOMBRINGER, "doomblast", "Unleashes a wave of doom on everyone nearby", 180, 75,
     ARTIFACT_BIND_ON_PICKUP,
     {3, 0, 0, 1, 0, 0}, 4, 5, 0, 30, 0, 0, 0, 0, 0, 20, CLASS_WARRIOR, ART_CLASS_GATE,
     ART_SIG_NONE, 0, ART_ALIGN_ANY},

    {ART_VNUM_KELRARIN, "soulstrike", "Strikes a single target with soul energy", 300, 50,
     ARTIFACT_BIND_ON_EQUIP,
     {2, 0, 0, 0, 1, 0}, 3, 3, 0, 20, 0, 0, 0, 0, 5, 15, CLASS_UNDEFINED, 0,
     ART_SIG_NONE, 0, ART_ALIGN_ANY},

    {ART_VNUM_KELROM, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {2, 0, 1, 0, 2, 0}, 2, 4, 0, 40, 0, 0, 5, 0, 0, 14, CLASS_UNDEFINED, 0,
     ART_SIG_NONE, 0, ART_ALIGN_ANY},

    {ART_VNUM_GESEN, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_NONE,
     {1, 0, 0, 2, 0, 0}, 3, 2, 0, 0, 0, 30, 0, 0, 0, 18, CLASS_UNDEFINED, 0,
     ART_SIG_NONE, 0, ART_ALIGN_ANY},

    {ART_VNUM_STINGER, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_ACCOUNT,
     {1, 0, 0, 3, 0, 0}, 5, 1, 0, 0, 0, 50, 10, 0, 0, 18, CLASS_UNDEFINED, 0,
     ART_SIG_LIFESTEAL, ARTIFACT_STINGER_LIFESTEAL_CHANCE, ART_ALIGN_ANY},

    {ART_VNUM_AVERNUS, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {2, 0, 0, 1, 1, 0}, 4, 4, 0, 25, 0, 0, 0, 0, 10, 15, CLASS_UNDEFINED, 0,
     ART_SIG_NONE, 0, ART_ALIGN_ANY},

    {ART_VNUM_AEGIS, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {0, 0, 0, 0, 3, 0}, 0, 0, 4, 60, 0, 0, 12, 12, 12, 0, CLASS_UNDEFINED, 0,
     ART_SIG_NONE, 0, ART_ALIGN_ANY},

    /* ---- second wave ---------------------------------------------------- */

    /* Vengeance - the holy sword.  Sustains its bearer while wounded and turns
     * on the wicked while whole. */
    {ART_VNUM_VENGEANCE, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {0, 0, 1, 0, 1, 1}, 2, 2, 1, 20, 0, 0, 0, 8, 0, 0, CLASS_PALADIN, ART_CLASS_GATE,
     ART_SIG_MERCY, 8, ART_ALIGN_TARGET_EVIL},

    /* Earthcrier - the mithril maul.  A control weapon, not a damage one. */
    {ART_VNUM_EARTHCRIER, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_PICKUP,
     {3, 0, 0, 0, 1, 0}, 3, 4, 0, 25, 0, 0, 6, 0, 0, 0, CLASS_UNDEFINED, 0,
     ART_SIG_KNOCKDOWN, 8, ART_ALIGN_SELF_EVIL},

    /* Wyrmfang - the spear of dragons.  Awareness and pursuit, with a
     * multi-outcome strike. */
    {ART_VNUM_WYRMFANG, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {2, 0, 0, 2, 0, 0}, 4, 3, 0, 0, 0, 20, 0, 0, 6, 0, CLASS_UNDEFINED, 0,
     ART_SIG_WEIGHTED, 10, ART_ALIGN_ANY},

    /* Courage - the golden mace.  The first artifact whose signature power
     * helps everyone standing with you rather than only its bearer. */
    {ART_VNUM_COURAGE, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_EQUIP,
     {0, 0, 2, 0, 2, 0}, 2, 1, 0, 20, 20, 0, 0, 0, 5, 0, CLASS_CLERIC, ART_CLASS_GATE,
     ART_SIG_NONE, 0, ART_ALIGN_ANY},

    /* Icedge - the dagger of cold.  Fast, and it gets faster. */
    {ART_VNUM_ICEDGE, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_ACCOUNT,
     {0, 0, 0, 3, 0, 0}, 3, 2, 0, 0, 0, 20, 0, 4, 10, 0, CLASS_UNDEFINED, 0,
     ART_SIG_FLURRY, 6, ART_ALIGN_ANY},

    /* Twilight - the sword of destruction.  A surge, never a stat doubling. */
    {ART_VNUM_TWILIGHT, NULL, NULL, ARTIFACT_DEFAULT_COOLDOWN, 0, ARTIFACT_BIND_ON_PICKUP,
     {3, 0, 0, 1, 0, 0}, 4, 4, 0, 20, 0, 0, 0, 0, 0, 0, CLASS_UNDEFINED, 0,
     ART_SIG_SURGE, 6, ART_ALIGN_ANY},

    {-1, NULL, NULL, 0, 0, 0, {0, 0, 0, 0, 0, 0}, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     CLASS_UNDEFINED, 0, ART_SIG_NONE, 0, ART_ALIGN_ANY}};

/* --------------------------------------------------------------------------
 * The content contract
 *
 * Where an artifact comes from, which campaigns it exists in, whether its
 * bearer is named publicly, and the one line of lore and one line of hint the
 * chronicle is allowed to print.  Neither string may name a room or a vnum:
 * the roster is a rumour board, not a treasure map.
 *
 * An artifact with no row here is available everywhere, staged in the vault,
 * and keeps its bearer secret.  Boot validation says so out loud.
 * -------------------------------------------------------------------------- */
struct artifact_contract
{
  int vnum;
  int acquisition;
  int campaigns;
  int owner_policy;
  const char *lore;
  const char *acq_hint;
};

static const struct artifact_contract artifact_contracts[] = {
    {ART_VNUM_TRORXEK, ART_ACQ_VAULT, ART_CAMPAIGN_ALL, ART_OWNER_SECRET,
     "A staff that was persuaded out of its tree rather than cut from it.",
     "Staged for release; the forests have not given it up yet."},

    {ART_VNUM_AMAUKEKEL, ART_ACQ_VAULT, ART_CAMPAIGN_ALL, ART_OWNER_SECRET,
     "A rod of sun-colored metal that warms near what should not be walking.",
     "Staged for release; no shrine has yet admitted to holding it."},

    {ART_VNUM_FADE, ART_ACQ_VAULT, ART_CAMPAIGN_ALL, ART_OWNER_SECRET,
     "A blade the light bends around rather than touches.",
     "Staged for release; thieves trade the rumour, not the sword."},

    {ART_VNUM_HENEKAR, ART_ACQ_VAULT, ART_CAMPAIGN_ALL, ART_OWNER_SECRET,
     "A horn whose single clear note takes the fight out of a room.",
     "Staged for release."},

    {ART_VNUM_DOOMBRINGER, ART_ACQ_VAULT, ART_CAMPAIGN_ALL, ART_OWNER_PUBLIC,
     "A blade that turns the room black from the edges in.",
     "Staged for release; whoever carries it will not be able to hide it."},

    {ART_VNUM_KELRARIN, ART_ACQ_VAULT, ART_CAMPAIGN_ALL, ART_OWNER_SECRET,
     "A hammer that breaks over the wicked like the judgement of heaven.",
     "Staged for release."},

    {ART_VNUM_KELROM, ART_ACQ_VAULT, ART_CAMPAIGN_ALL, ART_OWNER_SECRET,
     "An axe that will not be turned on an animal, and says so.",
     "Staged for release."},

    {ART_VNUM_GESEN, ART_ACQ_VAULT, ART_CAMPAIGN_ALL, ART_OWNER_SECRET,
     "An axe that comes back, and brings something with it.",
     "Staged for release."},

    {ART_VNUM_STINGER, ART_ACQ_VAULT, ART_CAMPAIGN_ALL, ART_OWNER_SECRET,
     "A barb taken from something with five heads and no mercy in any of them.",
     "Staged for release."},

    {ART_VNUM_AVERNUS, ART_ACQ_VAULT, ART_CAMPAIGN_ALL, ART_OWNER_SECRET,
     "A black blade that keeps its bearer alive to keep swinging.",
     "Staged for release."},

    {ART_VNUM_AEGIS, ART_ACQ_VAULT, ART_CAMPAIGN_ALL, ART_OWNER_SECRET,
     "A breastplate repaired in every age, with no two patches alike and none yet broken.",
     "Staged for release."},

    /* ---- second wave: each one has a real, stated acquisition mode ------- */

    {ART_VNUM_VENGEANCE, ART_ACQ_QUEST, ART_CAMPAIGN_ALL, ART_OWNER_PUBLIC,
     "A sacred sword worked for a lifetime and finished by three hands, not one.",
     "The orders that made it still test who asks for it. Answer the test."},

    {ART_VNUM_EARTHCRIER, ART_ACQ_BOSS, ART_CAMPAIGN_ALL, ART_OWNER_SECRET,
     "A rune-cut mithril maul heavy enough that the ground hears it land.",
     "Something large enough to swing it two-handed is already carrying it."},

    {ART_VNUM_WYRMFANG, ART_ACQ_BOSS, ART_CAMPAIGN_ALL, ART_OWNER_PUBLIC,
     "An eight-foot night-black spear cut from a gargantuan black dragon's horn.",
     "It was taken from a dragon once. It will have to be taken again."},

    {ART_VNUM_COURAGE, ART_ACQ_STAFF_EVENT, ART_CAMPAIGN_ALL, ART_OWNER_PUBLIC,
     "A golden mace that makes the people standing beside its bearer braver.",
     "It is given, in the open, to someone who has already earned it."},

    {ART_VNUM_ICEDGE, ART_ACQ_EXPLORATION, ART_CAMPAIGN_ALL, ART_OWNER_SECRET,
     "One of the legendary daggers of Ochir Naal, ice-blue and adamantine.",
     "A cult held it. Something out of the blizzard took it, and took it home."},

    {ART_VNUM_TWILIGHT, ART_ACQ_RECOVERY, ART_CAMPAIGN_ALL, ART_OWNER_PUBLIC,
     "A huge black sword whose fire-giant runes give it only one name.",
     "It is not placed and it is not dropped. It is recovered, or it is lost."},

    {-1, ART_ACQ_UNSET, 0, ART_OWNER_SECRET, NULL, NULL}};

/* --------------------------------------------------------------------------
 * Progressive passive powers
 *
 * One power, one source of truth.  These are never object prototype affect
 * bits: they are source-tagged affects applied on equip, refreshed on
 * level-up, and stripped on unequip, and they unlock as the artifact grows.
 * -------------------------------------------------------------------------- */
struct artifact_passive
{
  int vnum;
  int min_level;    /* artifact level at which this unlocks     */
  int aff_flag;     /* AFF_* granted, or 0 for a pure modifier  */
  int location;     /* APPLY_* , or APPLY_NONE                  */
  int modifier;     /* flat; not multiplied by artifact level   */
  const char *desc; /* one line for artifact info               */
};

static const struct artifact_passive artifact_passives[] = {
    /* Wyrmfang: a hunter's senses, opening one at a time. */
    {ART_VNUM_WYRMFANG, 1, AFF_DETECT_INVIS, APPLY_NONE, 0, "sees what tries not to be seen"},
    {ART_VNUM_WYRMFANG, 2, AFF_INFRAVISION, APPLY_NONE, 0, "sees in the dark"},
    {ART_VNUM_WYRMFANG, 3, AFF_SENSE_LIFE, APPLY_NONE, 0, "feels life moving nearby"},
    {ART_VNUM_WYRMFANG, 4, AFF_FARSEE, APPLY_NONE, 0, "looks further than the room"},
    {ART_VNUM_WYRMFANG, 5, AFF_HASTE, APPLY_NONE, 0, "moves the way a hunting spear should"},
    {ART_VNUM_WYRMFANG, 5, AFF_DANGERSENSE, APPLY_NONE, 0,
     "feels danger beyond the next door"},

    /* Courage: defenses first, then the speed the legend remembers. */
    {ART_VNUM_COURAGE, 1, 0, APPLY_SAVING_WILL, 2, "steadies the nerve"},
    {ART_VNUM_COURAGE, 2, 0, APPLY_RES_ELECTRIC, 10, "turns the lightning aside"},
    {ART_VNUM_COURAGE, 3, 0, APPLY_SAVING_FORT, 2, "keeps its bearer standing"},
    {ART_VNUM_COURAGE, 4, AFF_HASTE, APPLY_NONE, 0, "carries its bearer forward"},

    /* Icedge: cold, and then the cold that answers back. */
    {ART_VNUM_ICEDGE, 1, 0, APPLY_RES_COLD, 15, "keeps the cold outside"},
    {ART_VNUM_ICEDGE, 3, 0, APPLY_SPELL_RES, 4, "shrugs off what is aimed at it"},
    {ART_VNUM_ICEDGE, 5, AFF_TRUE_SIGHT, APPLY_NONE, 0, "sees through the blizzard"},

    /* Twilight: the sword's own awareness, unlocking late. */
    {ART_VNUM_TWILIGHT, 2, AFF_INFRAVISION, APPLY_NONE, 0, "sees in the dark"},
    {ART_VNUM_TWILIGHT, 3, AFF_SENSE_LIFE, APPLY_NONE, 0, "feels life moving nearby"},
    {ART_VNUM_TWILIGHT, 4, AFF_FARSEE, APPLY_NONE, 0, "looks further than the room"},
    {ART_VNUM_TWILIGHT, 5, AFF_HASTE, APPLY_NONE, 0, "moves faster than a sword that size should"},

    /* Vengeance: the protections its makers argued about for a lifetime. */
    {ART_VNUM_VENGEANCE, 1, AFF_DETECT_INVIS, APPLY_NONE, 0, "sees what tries not to be seen"},
    {ART_VNUM_VENGEANCE, 3, 0, APPLY_SAVING_WILL, 3, "will not be talked out of anything"},
    {ART_VNUM_VENGEANCE, 5, 0, APPLY_RES_UNHOLY, 15, "refuses the unholy"},

    /* Earthcrier: it makes its bearer as hard to move as it is. */
    {ART_VNUM_EARTHCRIER, 2, 0, APPLY_SAVING_FORT, 3, "roots its bearer to the ground"},
    {ART_VNUM_EARTHCRIER, 4, 0, APPLY_RES_PUNCTURE, 10, "turns points aside"},

    {-1, 0, 0, APPLY_NONE, 0, NULL}};

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
  int channel;     /* ART_INVOKE_* - how the phrase reaches the artifact */
  int stack_group; /* ART_STACK_* , or ART_STACK_NONE                    */
  const char *desc; /* identify text */
};

static const struct artifact_effect artifact_effects[] = {
    /* Trorxek, the Staff of Ancient Oaks - the Oaken Defender */
    {ART_VNUM_TRORXEK, 0, "come oaken defender", ART_TARGET_NONE, ARTIFACT_RECHARGE_WEEK,
     ART_EFFECT_SUMMON_TREANT, ART_INVOKE_SAY, ART_STACK_NONE, "calls the Oaken Defender to fight at your side"},
    {ART_VNUM_TRORXEK, 1, "carpet of death", ART_TARGET_NONE, ARTIFACT_RECHARGE_DAY,
     ART_EFFECT_CREEPING_DOOM, ART_INVOKE_SAY, ART_STACK_NONE, "carpets the ground in creeping doom"},
    {ART_VNUM_TRORXEK, 2, "forest path home", ART_TARGET_NONE, ARTIFACT_RECHARGE_HOUR,
     ART_EFFECT_RECALL, ART_INVOKE_SAY, ART_STACK_NONE, "walks you home along the forest path"},
    {ART_VNUM_TRORXEK, 3, "moonlit path to", ART_TARGET_CHAR_WORLD, ARTIFACT_RECHARGE_DAY,
     ART_EFFECT_TRAVEL_TO, ART_INVOKE_SAY, ART_STACK_NONE, "opens a moonwell to a named traveller"},

    /* Amaukekel, the Rod of Light */
    {ART_VNUM_AMAUKEKEL, 0, "sunlit path to paradise", ART_TARGET_NONE, ARTIFACT_RECHARGE_WEEK,
     ART_EFFECT_DIMENSION_SHIFT, ART_INVOKE_SAY, ART_STACK_NONE, "shifts you and your group out of danger"},
    {ART_VNUM_AMAUKEKEL, 1, "give life to", ART_TARGET_OBJ_ROOM, ARTIFACT_RECHARGE_DAY,
     ART_EFFECT_RESURRECT, ART_INVOKE_SAY, ART_STACK_NONE, "restores the dead from a corpse at your feet"},
    {ART_VNUM_AMAUKEKEL, 2, "wrath of light", ART_TARGET_CHAR_ROOM, ARTIFACT_RECHARGE_HOUR,
     ART_EFFECT_DISPEL_EVIL, ART_INVOKE_SAY, ART_STACK_NONE, "calls down the wrath of light on the wicked"},

    /* Fade, the Shadowblade */
    {ART_VNUM_FADE, 0, "eyes of darkness", ART_TARGET_CHAR_ROOM, ARTIFACT_RECHARGE_DAY,
     ART_EFFECT_BLIND, ART_INVOKE_SAY, ART_STACK_NONE, "puts out an enemy's eyes"},
    {ART_VNUM_FADE, 1, "darken the world", ART_TARGET_NONE, ARTIFACT_RECHARGE_HOUR,
     ART_EFFECT_DARKNESS, ART_INVOKE_SAY, ART_STACK_NONE, "smothers the room in darkness"},
    {ART_VNUM_FADE, 2, "devour the soul", ART_TARGET_FIGHTING, ARTIFACT_RECHARGE_WEEK,
     ART_EFFECT_WEAKEN, ART_INVOKE_SAY, ART_STACK_NONE, "devours the soul of whoever you are fighting"},
    {ART_VNUM_FADE, 3, "shadowy path to", ART_TARGET_CHAR_WORLD, ARTIFACT_RECHARGE_HOUR,
     ART_EFFECT_TRAVEL_TO, ART_INVOKE_SAY, ART_STACK_NONE, "walks the shadows to a named traveller"},

    /* The Horn of Henekar */
    {ART_VNUM_HENEKAR, 0, "you see darkness", ART_TARGET_CHAR_ROOM, ARTIFACT_RECHARGE_6HOUR,
     ART_EFFECT_BLIND, ART_INVOKE_SAY, ART_STACK_NONE, "blinds a single listener"},
    {ART_VNUM_HENEKAR, 1, "peace to you", ART_TARGET_NONE, ARTIFACT_RECHARGE_12HOUR,
     ART_EFFECT_PACIFY, ART_INVOKE_SAY, ART_STACK_NONE, "ends every fight in the room"},
    {ART_VNUM_HENEKAR, 2, "join my quest", ART_TARGET_NONE, ARTIFACT_RECHARGE_12HOUR,
     ART_EFFECT_CHARM, ART_INVOKE_SAY, ART_STACK_NONE, "recruits the lesser creatures nearby"},
    {ART_VNUM_HENEKAR, 3, "sonic path to", ART_TARGET_CHAR_WORLD, ARTIFACT_RECHARGE_HOUR,
     ART_EFFECT_TRAVEL_TO, ART_INVOKE_SAY, ART_STACK_NONE, "rides its own note to a named traveller"},

    /* Doombringer */
    {ART_VNUM_DOOMBRINGER, 0, "bring annhilation forth", ART_TARGET_NONE, ARTIFACT_RECHARGE_WEEK,
     ART_EFFECT_ANNIHILATION, ART_INVOKE_SAY, ART_STACK_NONE, "annihilates everything hostile in the room"},
    {ART_VNUM_DOOMBRINGER, 1, "feel my power", ART_TARGET_CHAR_ROOM, ARTIFACT_RECHARGE_DAY,
     ART_EFFECT_BLACK_LIGHTNING, ART_INVOKE_SAY, ART_STACK_NONE, "calls black lightning down on one enemy"},
    {ART_VNUM_DOOMBRINGER, 2, "enrage me doombringer", ART_TARGET_NONE, ARTIFACT_RECHARGE_DAY,
     ART_EFFECT_ENRAGE, ART_INVOKE_SAY, ART_STACK_COMBAT_SURGE, "drives you into a killing rage"},

    /* Courage - the group invocation.  One cooldown and one XP award, however
     * many people it reaches. */
    {ART_VNUM_COURAGE, 0, "courage", ART_TARGET_GROUP_ROOM, ARTIFACT_RECHARGE_6HOUR,
     ART_EFFECT_GROUP_VALOR, ART_INVOKE_SAY, ART_STACK_MORALE,
     "puts heart and wind back into everyone standing with you"},

    /* Icedge - whispered, not shouted.  A dagger's power should not announce
     * itself to the room. */
    {ART_VNUM_ICEDGE, 0, "rime", ART_TARGET_NONE, ARTIFACT_RECHARGE_HOUR, ART_EFFECT_FROST_WARD,
     ART_INVOKE_WHISPER, ART_STACK_WARD, "sheathes you in rime"},

    /* Wyrmfang - an explicit command.  The spear does not answer to talk. */
    {ART_VNUM_WYRMFANG, 0, "hunt", ART_TARGET_NONE, ARTIFACT_RECHARGE_HOUR, ART_EFFECT_DRAGON_SIGHT,
     ART_INVOKE_COMMAND, ART_STACK_WARD, "opens the hunter's sight"},

    {-1, 0, NULL, ART_TARGET_NONE, 0, 0, ART_INVOKE_SAY, ART_STACK_NONE, NULL}};
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

static const char *artifact_signature_desc(int signature)
{
  switch (signature)
  {
  case ART_SIG_KNOCKDOWN:
    return "knock a foe from its feet";
  case ART_SIG_MERCY:
    return "heal you while wounded or smite an evil foe while healthy";
  case ART_SIG_WARD:
    return "raise a ward on a critical strike or dispel a foe";
  case ART_SIG_WEIGHTED:
    return "stagger, slow, or drive through a foe";
  case ART_SIG_SURGE:
    return "drive you into a temporary combat surge";
  case ART_SIG_FLURRY:
    return "unleash a flurry of additional attacks";
  case ART_SIG_LIFESTEAL:
    return "drain a foe's vitality as healing, with a fifteen-hit bad-luck limit";
  default:
    return "unleash a unique effect";
  }
}

/* --------------------------------------------------------------------------
 * Chronicle state
 *
 * Derived from the registry on every call.  There is no second list, and
 * nothing here is persisted: a stale roster is worse than no roster.
 * -------------------------------------------------------------------------- */

int artifact_state(struct artifact_data *art)
{
  if (!art)
    return ART_STATE_UNAWAKENED;

  if (!artifact_is_owned(art->vnum))
    return art->discovered ? ART_STATE_UNCLAIMED : ART_STATE_UNAWAKENED;

  /* Somebody in play is carrying or wearing it. */
  if (!artifact_is_dropped(art))
    return ART_STATE_HELD;

  /* Owned on record with no live bearer.  If the instance is sitting in a
   * player or house save it is merely out of sight; if it is not, the only
   * way back into play is an audited recovery. */
  return art->instance_persisted ? ART_STATE_LOST : ART_STATE_RECOVERABLE;
}

const char *artifact_state_name(int state)
{
  static const char *const names[NUM_ART_STATES] = {"unawakened", "unclaimed", "held", "lost",
                                                    "recoverable"};

  if (state < 0 || state >= NUM_ART_STATES)
    return "unknown";

  return names[state];
}

const char *artifact_acquisition_name(int acquisition)
{
  static const char *const names[NUM_ART_ACQ] = {
      "undeclared",       "a world boss",  "a quest",       "an exploration chain",
      "a seasonal event", "a staff event", "recovery only", "staged in the vault"};

  if (acquisition < 0 || acquisition >= NUM_ART_ACQ)
    return "undeclared";

  return names[acquisition];
}

const char *artifact_invoke_name(int channel)
{
  static const char *const names[NUM_ART_INVOKE] = {"say", "whisper", "invoke"};

  if (channel < 0 || channel >= NUM_ART_INVOKE)
    return "say";

  return names[channel];
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
    art->sig_proc = artifact_templates[i].sig_proc;
    art->sig_chance = artifact_templates[i].sig_chance;
    art->sig_align = artifact_templates[i].sig_align;
    return;
  }
}

/* Which campaign is compiled in.  One bit, decided once. */
static int artifact_current_campaign(void)
{
#if defined(CAMPAIGN_DL)
  return ART_CAMPAIGN_DL;
#elif defined(CAMPAIGN_FR)
  return ART_CAMPAIGN_FR;
#else
  return ART_CAMPAIGN_LUMINARI;
#endif
}

int artifact_campaign_available(int campaigns)
{
  return (campaigns & artifact_current_campaign()) ? TRUE : FALSE;
}

static const struct artifact_contract *artifact_contract_of(int vnum)
{
  int i = 0;

  for (i = 0; artifact_contracts[i].vnum != -1; i++)
    if (artifact_contracts[i].vnum == vnum)
      return &artifact_contracts[i];

  return NULL;
}

/* Stamp the content contract onto a registry entry.  An artifact with no
 * contract row still works; it is simply vault-staged, campaign-neutral, and
 * keeps its bearer's name to itself.  Boot validation reports the gap. */
static void artifact_apply_contract(struct artifact_data *art)
{
  const struct artifact_contract *contract = artifact_contract_of(art->vnum);

  if (!contract)
  {
    art->acquisition = ART_ACQ_UNSET;
    art->campaigns = ART_CAMPAIGN_ALL;
    art->owner_policy = ART_OWNER_SECRET;
    art->lore = NULL;
    art->acq_hint = NULL;
    art->available = TRUE;
    return;
  }

  art->acquisition = contract->acquisition;
  art->campaigns = contract->campaigns;
  art->owner_policy = contract->owner_policy;
  art->lore = contract->lore;
  art->acq_hint = contract->acq_hint;
  art->available = artifact_campaign_available(contract->campaigns);
}

/* --------------------------------------------------------------------------
 * Boot-time metadata validation
 *
 * Everything the tables assert about themselves is checked once, at boot, and
 * a precise SYSERR names the offending row.  A bad effect row is switched off
 * on its own; the artifact and every other effect it owns keep working.  The
 * registry itself is never disabled for a metadata fault.
 * -------------------------------------------------------------------------- */

static void artifact_normalize_speech(char *dst, size_t size, const char *src);

/* One flag per row of artifact_effects[].  Allocated at validation. */
static char *artifact_effect_disabled = NULL;
static int artifact_effect_count = 0;

static int artifact_effect_is_disabled(int index)
{
  if (!artifact_effect_disabled || index < 0 || index >= artifact_effect_count)
    return FALSE;

  return artifact_effect_disabled[index];
}

/* A public roster line must never hand out a room number or a vnum.  Any run
 * of four or more digits in lore or hint text is treated as exactly that. */
static int artifact_text_leaks_numbers(const char *text)
{
  int run = 0;

  if (!text)
    return FALSE;

  for (; *text; text++)
  {
    if (isdigit((unsigned char)*text))
    {
      if (++run >= 4)
        return TRUE;
      continue;
    }
    run = 0;
  }

  return FALSE;
}

static int artifact_phrase_is_normalized(const char *phrase)
{
  char normalized[MAX_INPUT_LENGTH] = {'\0'};

  if (!phrase || !*phrase)
    return FALSE;

  artifact_normalize_speech(normalized, sizeof(normalized), phrase);

  return (strcmp(normalized, phrase) == 0);
}

/* Does the dispatcher accept this effect with this target rule?  This is the
 * check that stops a phrase promising an argument the effect never reads, or
 * an effect that needs a victim being wired to a phrase that takes none. */
static int artifact_effect_target_ok(int effect, int target_type)
{
  switch (effect)
  {
  case ART_EFFECT_TRAVEL_TO:
    return (target_type == ART_TARGET_CHAR_WORLD);

  case ART_EFFECT_RESURRECT:
    return (target_type == ART_TARGET_OBJ_ROOM);

  case ART_EFFECT_DISPEL_EVIL:
  case ART_EFFECT_BLIND:
  case ART_EFFECT_BLACK_LIGHTNING:
    return (target_type == ART_TARGET_CHAR_ROOM);

  case ART_EFFECT_WEAKEN:
    return (target_type == ART_TARGET_FIGHTING || target_type == ART_TARGET_CHAR_ROOM);

  case ART_EFFECT_GROUP_VALOR:
    return (target_type == ART_TARGET_GROUP_ROOM);

  case ART_EFFECT_SUMMON_TREANT:
  case ART_EFFECT_CREEPING_DOOM:
  case ART_EFFECT_RECALL:
  case ART_EFFECT_DIMENSION_SHIFT:
  case ART_EFFECT_DARKNESS:
  case ART_EFFECT_PACIFY:
  case ART_EFFECT_CHARM:
  case ART_EFFECT_ANNIHILATION:
  case ART_EFFECT_ENRAGE:
  case ART_EFFECT_FROST_WARD:
  case ART_EFFECT_DRAGON_SIGHT:
    return (target_type == ART_TARGET_NONE);

  default:
    return FALSE;
  }
}

static int artifact_validate_effects(void)
{
  int i = 0, j = 0, problems = 0;
  int used_slots[ARTIFACT_MAX_EFFECTS];
  int owner_vnum = 0, per_artifact = 0;

  for (artifact_effect_count = 0; artifact_effects[artifact_effect_count].vnum != -1;
       artifact_effect_count++)
    ;

  if (artifact_effect_disabled)
    free(artifact_effect_disabled);

  CREATE(artifact_effect_disabled, char, MAX(1, artifact_effect_count));

  for (i = 0; i < artifact_effect_count; i++)
  {
    const struct artifact_effect *effect = &artifact_effects[i];
    int bad = FALSE;

    if (artifact_search(effect->vnum) < 0)
    {
      log("SYSERR: artifact effect row %d: vnum %d is not a registry artifact - effect disabled", i,
          effect->vnum);
      bad = TRUE;
    }

    if (effect->slot < 0 || effect->slot >= ARTIFACT_MAX_EFFECTS)
    {
      log("SYSERR: artifact effect row %d (vnum %d): slot %d is outside 0..%d - effect disabled", i,
          effect->vnum, effect->slot, ARTIFACT_MAX_EFFECTS - 1);
      bad = TRUE;
    }

    if (!artifact_phrase_is_normalized(effect->phrase))
    {
      log("SYSERR: artifact effect row %d (vnum %d): phrase \"%s\" is empty or not normalized - "
          "effect disabled",
          i, effect->vnum, effect->phrase ? effect->phrase : "(null)");
      bad = TRUE;
    }

    if (effect->target_type < 0 || effect->target_type >= NUM_ART_TARGETS)
    {
      log("SYSERR: artifact effect row %d (vnum %d): target type %d is unknown - effect disabled",
          i, effect->vnum, effect->target_type);
      bad = TRUE;
    }

    if (effect->effect <= 0 || effect->effect >= NUM_ART_EFFECTS)
    {
      log("SYSERR: artifact effect row %d (vnum %d): effect id %d is unknown - effect disabled", i,
          effect->vnum, effect->effect);
      bad = TRUE;
    }
    else if (!bad && !artifact_effect_target_ok(effect->effect, effect->target_type))
    {
      log("SYSERR: artifact effect row %d (vnum %d): effect %d cannot be driven by target rule %d "
          "- "
          "effect disabled",
          i, effect->vnum, effect->effect, effect->target_type);
      bad = TRUE;
    }

    if (effect->channel < 0 || effect->channel >= NUM_ART_INVOKE)
    {
      log("SYSERR: artifact effect row %d (vnum %d): invocation channel %d is unknown - effect "
          "disabled",
          i, effect->vnum, effect->channel);
      bad = TRUE;
    }

    if (effect->stack_group < 0 || effect->stack_group >= NUM_ART_STACK)
    {
      log("SYSERR: artifact effect row %d (vnum %d): stacking group %d is unknown - effect "
          "disabled",
          i, effect->vnum, effect->stack_group);
      bad = TRUE;
    }

    if (effect->recharge <= 0)
    {
      log("SYSERR: artifact effect row %d (vnum %d): recharge %d is not a positive interval - "
          "effect disabled",
          i, effect->vnum, effect->recharge);
      bad = TRUE;
    }

    if (!effect->desc || !*effect->desc)
    {
      log("SYSERR: artifact effect row %d (vnum %d): missing description - effect disabled", i,
          effect->vnum);
      bad = TRUE;
    }

    /* Two rows that answer to the same words on the same channel would make
     * the first one shadow the second forever. */
    for (j = 0; j < i && !bad; j++)
      if (artifact_effects[j].channel == effect->channel && artifact_effects[j].phrase &&
          effect->phrase && !strcmp(artifact_effects[j].phrase, effect->phrase))
      {
        log("SYSERR: artifact effect row %d (vnum %d): phrase \"%s\" on channel %s already belongs "
            "to vnum %d - effect disabled",
            i, effect->vnum, effect->phrase, artifact_invoke_name(effect->channel),
            artifact_effects[j].vnum);
        bad = TRUE;
      }

    if (bad)
    {
      artifact_effect_disabled[i] = TRUE;
      problems++;
    }
  }

  /* Duplicate slots on one artifact silently share a recharge timer, so the
   * second and later rows lose. */
  for (i = 0; i < artifact_effect_count; i++)
  {
    if (artifact_effect_disabled[i])
      continue;

    owner_vnum = artifact_effects[i].vnum;

    /* Only walk each artifact once. */
    for (j = 0; j < i; j++)
      if (artifact_effects[j].vnum == owner_vnum)
        break;
    if (j < i)
      continue;

    for (j = 0; j < ARTIFACT_MAX_EFFECTS; j++)
      used_slots[j] = -1;

    per_artifact = 0;

    for (j = i; j < artifact_effect_count; j++)
    {
      if (artifact_effects[j].vnum != owner_vnum || artifact_effect_disabled[j])
        continue;

      per_artifact++;

      if (artifact_effects[j].slot < 0 || artifact_effects[j].slot >= ARTIFACT_MAX_EFFECTS)
        continue;

      if (used_slots[artifact_effects[j].slot] >= 0)
      {
        log("SYSERR: artifact effect row %d (vnum %d): slot %d is already used by row %d - effect "
            "disabled",
            j, owner_vnum, artifact_effects[j].slot, used_slots[artifact_effects[j].slot]);
        artifact_effect_disabled[j] = TRUE;
        problems++;
        continue;
      }

      used_slots[artifact_effects[j].slot] = j;
    }

    if (per_artifact > ARTIFACT_MAX_EFFECTS)
    {
      log("SYSERR: artifact %d declares %d effects but only %d slots exist", owner_vnum,
          per_artifact, ARTIFACT_MAX_EFFECTS);
      problems++;
    }
  }

  return problems;
}

static int artifact_validate_templates(void)
{
  int i = 0, j = 0, problems = 0;

  for (i = 0; artifact_templates[i].vnum != -1; i++)
  {
    /* One artifact, one row.  A duplicate vnum is the only way this system
     * could ever grow a second "original", so it is a hard error. */
    for (j = 0; j < i; j++)
      if (artifact_templates[j].vnum == artifact_templates[i].vnum)
      {
        log("SYSERR: artifact template row %d: vnum %d is already defined by row %d", i,
            artifact_templates[i].vnum, j);
        problems++;
      }

    if (artifact_templates[i].sig_proc < 0 || artifact_templates[i].sig_proc >= NUM_ART_SIG)
    {
      log("SYSERR: artifact template (vnum %d): signature proc %d is unknown",
          artifact_templates[i].vnum, artifact_templates[i].sig_proc);
      problems++;
    }

    if (artifact_templates[i].sig_align < 0 || artifact_templates[i].sig_align >= NUM_ART_ALIGN)
    {
      log("SYSERR: artifact template (vnum %d): alignment rule %d is unknown",
          artifact_templates[i].vnum, artifact_templates[i].sig_align);
      problems++;
    }

    if (artifact_templates[i].sig_chance < 0 || artifact_templates[i].sig_chance > 100)
    {
      log("SYSERR: artifact template (vnum %d): signature chance %d is not a percentage",
          artifact_templates[i].vnum, artifact_templates[i].sig_chance);
      problems++;
    }

    if (artifact_templates[i].sig_proc != ART_SIG_NONE && artifact_templates[i].sig_chance <= 0)
    {
      log("SYSERR: artifact template (vnum %d): signature proc %d can never fire at %d%%",
          artifact_templates[i].vnum, artifact_templates[i].sig_proc,
          artifact_templates[i].sig_chance);
      problems++;
    }

    if (artifact_templates[i].sig_proc == ART_SIG_NONE && artifact_templates[i].sig_chance > 0)
    {
      log("SYSERR: artifact template (vnum %d): signature chance %d%% with no signature proc",
          artifact_templates[i].vnum, artifact_templates[i].sig_chance);
      problems++;
    }

    if (artifact_templates[i].binding_type < 0 ||
        artifact_templates[i].binding_type >= NUM_ARTIFACT_BINDINGS)
    {
      log("SYSERR: artifact template (vnum %d): binding type %d is unknown",
          artifact_templates[i].vnum, artifact_templates[i].binding_type);
      problems++;
    }
  }

  return problems;
}

static int artifact_validate_contracts(void)
{
  int i = 0, j = 0, problems = 0;

  for (i = 0; artifact_contracts[i].vnum != -1; i++)
  {
    const struct artifact_contract *contract = &artifact_contracts[i];

    for (j = 0; j < i; j++)
      if (artifact_contracts[j].vnum == contract->vnum)
      {
        log("SYSERR: artifact contract row %d: vnum %d is already declared by row %d", i,
            contract->vnum, j);
        problems++;
      }

    if (artifact_search(contract->vnum) < 0)
    {
      log("SYSERR: artifact contract row %d: vnum %d is not a registry artifact", i,
          contract->vnum);
      problems++;
      continue;
    }

    if (contract->acquisition <= ART_ACQ_UNSET || contract->acquisition >= NUM_ART_ACQ)
    {
      log("SYSERR: artifact contract (vnum %d): acquisition %d is unknown or undeclared",
          contract->vnum, contract->acquisition);
      problems++;
    }

    if ((contract->campaigns & ART_CAMPAIGN_ALL) == 0)
    {
      log("SYSERR: artifact contract (vnum %d): available in no campaign at all", contract->vnum);
      problems++;
    }

    if (contract->owner_policy < 0 || contract->owner_policy >= NUM_ART_OWNER_POLICY)
    {
      log("SYSERR: artifact contract (vnum %d): owner policy %d is unknown", contract->vnum,
          contract->owner_policy);
      problems++;
    }

    if (!contract->lore || !*contract->lore)
    {
      log("SYSERR: artifact contract (vnum %d): missing public lore line", contract->vnum);
      problems++;
    }
    else if (artifact_text_leaks_numbers(contract->lore))
    {
      log("SYSERR: artifact contract (vnum %d): lore line looks like it names a room or vnum",
          contract->vnum);
      problems++;
    }

    if (!contract->acq_hint || !*contract->acq_hint)
    {
      log("SYSERR: artifact contract (vnum %d): missing acquisition hint", contract->vnum);
      problems++;
    }
    else if (artifact_text_leaks_numbers(contract->acq_hint))
    {
      log("SYSERR: artifact contract (vnum %d): acquisition hint looks like it names a room or "
          "vnum",
          contract->vnum);
      problems++;
    }
  }

  /* Every artifact that made it into the registry should have said where it
   * comes from.  This is a warning, not a fault: the artifact still works. */
  for (i = 0; i < total_artifacts; i++)
    if (!artifact_contract_of(art_index[i].vnum))
      log("Artifacts: vnum %d has no acquisition contract - treated as vault-staged.",
          art_index[i].vnum);

  return problems;
}

static int artifact_validate_passives(void)
{
  int i = 0, j = 0, problems = 0, per_artifact = 0;

  for (i = 0; artifact_passives[i].vnum != -1; i++)
  {
    const struct artifact_passive *passive = &artifact_passives[i];

    if (artifact_search(passive->vnum) < 0)
    {
      log("SYSERR: artifact passive row %d: vnum %d is not a registry artifact", i, passive->vnum);
      problems++;
      continue;
    }

    if (passive->min_level < 1 || passive->min_level > ARTIFACT_MAX_LEVEL)
    {
      log("SYSERR: artifact passive row %d (vnum %d): unlock level %d is outside 1..%d", i,
          passive->vnum, passive->min_level, ARTIFACT_MAX_LEVEL);
      problems++;
    }

    if (passive->aff_flag == 0 && passive->location == APPLY_NONE)
    {
      log("SYSERR: artifact passive row %d (vnum %d): grants neither a flag nor a modifier", i,
          passive->vnum);
      problems++;
    }

    if (passive->location != APPLY_NONE && passive->modifier == 0)
    {
      log("SYSERR: artifact passive row %d (vnum %d): apply %d with a zero modifier", i,
          passive->vnum, passive->location);
      problems++;
    }

    if (!passive->desc || !*passive->desc)
    {
      log("SYSERR: artifact passive row %d (vnum %d): missing description", i, passive->vnum);
      problems++;
    }

    /* The same flag twice on one artifact would be removed once and kept
     * forever by the other row. */
    for (j = 0; j < i; j++)
      if (artifact_passives[j].vnum == passive->vnum && passive->aff_flag != 0 &&
          artifact_passives[j].aff_flag == passive->aff_flag)
      {
        log("SYSERR: artifact passive row %d (vnum %d): flag %d is already granted by row %d", i,
            passive->vnum, passive->aff_flag, j);
        problems++;
      }
  }

  for (i = 0; i < total_artifacts; i++)
  {
    per_artifact = 0;

    for (j = 0; artifact_passives[j].vnum != -1; j++)
      if (artifact_passives[j].vnum == art_index[i].vnum)
        per_artifact++;

    if (per_artifact > ART_PASSIVE_MAX_PER_ARTIFACT)
    {
      log("SYSERR: artifact %d declares %d passive powers; the limit is %d", art_index[i].vnum,
          per_artifact, ART_PASSIVE_MAX_PER_ARTIFACT);
      problems++;
    }
  }

  return problems;
}

/* Run every table check.  Returns the number of problems found; the caller
 * logs the summary.  Called from artifact_boot() after the registry exists,
 * and again by 'testartifact verify'. */
int artifact_validate_metadata(void)
{
  int problems = 0;

  if (!art_index || total_artifacts == 0)
    return 0;

  problems += artifact_validate_templates();
  problems += artifact_validate_contracts();
  problems += artifact_validate_effects();
  problems += artifact_validate_passives();

  return problems;
}

/* --------------------------------------------------------------------------
 * Called-effect lookup
 * -------------------------------------------------------------------------- */

static const struct artifact_effect *artifact_effect_at(int vnum, int slot)
{
  int i = 0;

  for (i = 0; artifact_effects[i].vnum != -1; i++)
    if (artifact_effects[i].vnum == vnum && artifact_effects[i].slot == slot &&
        !artifact_effect_is_disabled(i))
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
    if (art_index[i].first_owner)
      bytes += strlen(art_index[i].first_owner) + 1;
    if (art_index[i].first_account)
      bytes += strlen(art_index[i].first_account) + 1;
  }

  return bytes;
}

/* Write the registry out.  Temp file plus atomic rename, so a crash during
 * the write cannot corrupt the data. */
void artifact_save(void)
{
  FILE *fl = NULL;
  char temp_file[MAX_INPUT_LENGTH] = {'\0'};
  char generated_at[32] = {'\0'};
  time_t current_time = 0;
  int i = 0, j = 0;

  if (!art_index || total_artifacts == 0)
    return;

  snprintf(temp_file, sizeof(temp_file), "%s.tmp", ARTIFACT_FILE);

  if (!(fl = fopen_restricted(temp_file, "w")))
  {
    log("SYSERR: artifact_save: cannot open %s for writing", temp_file);
    return;
  }

  current_time = time(0);
  fprintf(fl, "# Artifact Ownership File v2.4\n");
  fprintf(fl, "# Format: vnum owner account level exp bound_time instance_persisted\n");
  fprintf(fl, "#         first_owner first_account first_claimed last_claimed\n");
  fprintf(fl,
          "#         claims transfers destroys recoveries overrides discovered discovered_at\n");
  fprintf(fl, "#         last_ability last_proc last_signature effect_used[0..%d]\n",
          ARTIFACT_MAX_EFFECTS - 1);
  if (!ctime_r(&current_time, generated_at))
    strlcpy(generated_at, "Unknown\n", sizeof(generated_at));
  fprintf(fl, "# Generated: %s", generated_at);
  fprintf(fl, "\n");

  for (i = 0; i < total_artifacts; i++)
  {
    fprintf(fl, "%d %s %s %d %d %ld %d", art_index[i].vnum,
            (art_index[i].owner && *art_index[i].owner) ? art_index[i].owner : ARTIFACT_OWNER_NONE,
            (art_index[i].account && *art_index[i].account) ? art_index[i].account
                                                            : ARTIFACT_OWNER_NONE,
            art_index[i].level, art_index[i].experience, (long)art_index[i].bound_time,
            art_index[i].instance_persisted ? 1 : 0);

    fprintf(fl, " %s %s %ld %ld",
            (art_index[i].first_owner && *art_index[i].first_owner) ? art_index[i].first_owner
                                                                    : ARTIFACT_OWNER_NONE,
            (art_index[i].first_account && *art_index[i].first_account) ? art_index[i].first_account
                                                                        : ARTIFACT_OWNER_NONE,
            (long)art_index[i].first_claimed_at, (long)art_index[i].last_claimed_at);

    fprintf(fl, " %d %d %d %d %d %d %ld", art_index[i].claim_count, art_index[i].transfer_count,
            art_index[i].destroy_count, art_index[i].recovery_count, art_index[i].override_count,
            art_index[i].discovered ? 1 : 0, (long)art_index[i].discovered_at);

    fprintf(fl, " %ld %ld %ld", (long)art_index[i].last_ability_use, (long)art_index[i].last_proc,
            (long)art_index[i].last_signature_proc);

    for (j = 0; j < ARTIFACT_MAX_EFFECTS; j++)
      fprintf(fl, " %ld", (long)art_index[i].effect_used[j]);

    fprintf(fl, "\n");
  }

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
  ARTIFACT_FORMAT_V22,
  ARTIFACT_FORMAT_V23,
  ARTIFACT_FORMAT_V24
};

/* One parsed line.  Every format fills the fields it has and leaves the rest
 * at the defaults set by artifact_record_init(), so a v1 file loads into the
 * same structure a v2.4 file does. */
struct artifact_record
{
  int vnum;
  char owner[MAX_INPUT_LENGTH];
  char account[MAX_INPUT_LENGTH];
  int level;
  int exp;
  long bound_time;
  int instance_persisted;

  char first_owner[MAX_INPUT_LENGTH];
  char first_account[MAX_INPUT_LENGTH];
  long first_claimed_at;
  long last_claimed_at;
  int claim_count;
  int transfer_count;
  int destroy_count;
  int recovery_count;
  int override_count;
  int discovered;
  long discovered_at;

  long last_ability_use;
  long last_proc;
  long last_signature_proc;
  long effect_used[ARTIFACT_MAX_EFFECTS];

  int has_history;   /* the file carried provenance          */
  int has_cooldowns; /* the file carried recharge stamps     */
};

static void artifact_record_init(struct artifact_record *rec)
{
  int i = 0;

  memset(rec, 0, sizeof(*rec));

  rec->level = 1;
  strlcpy(rec->first_owner, ARTIFACT_OWNER_NONE, sizeof(rec->first_owner));
  strlcpy(rec->first_account, ARTIFACT_OWNER_NONE, sizeof(rec->first_account));

  for (i = 0; i < ARTIFACT_MAX_EFFECTS; i++)
    rec->effect_used[i] = 0;
}

/* Count whitespace-separated fields.  v2.3 and v2.4 are distinguished from
 * older formats by length; v2.4 adds one signature-cooldown column.
 */
static int artifact_count_fields(const char *line)
{
  int fields = 0, in_field = FALSE;

  for (; *line && *line != '\n' && *line != '\r'; line++)
  {
    if (isspace((unsigned char)*line))
    {
      in_field = FALSE;
      continue;
    }
    if (!in_field)
    {
      fields++;
      in_field = TRUE;
    }
  }

  return fields;
}

#define ARTIFACT_V23_FIELDS (20 + ARTIFACT_MAX_EFFECTS)
#define ARTIFACT_V24_FIELDS (21 + ARTIFACT_MAX_EFFECTS)

static enum artifact_file_format artifact_detect_record_format(const char *line)
{
  char owner[MAX_INPUT_LENGTH] = {'\0'};
  char third[MAX_INPUT_LENGTH] = {'\0'};
  int vnum = 0, fourth = 0, fifth = 0, seventh = 0;
  long sixth = 0;
  int parsed = 0, fields = 0;

  fields = artifact_count_fields(line);

  if (fields >= ARTIFACT_V24_FIELDS)
    return ARTIFACT_FORMAT_V24;
  if (fields >= ARTIFACT_V23_FIELDS)
    return ARTIFACT_FORMAT_V23;

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

/* Read the trailing v2.3/v2.4 columns, which are all numeric and positional.
 * The head of the line has already been consumed by the v2.2 parse. */
static int artifact_load_modern_tail(const char *line, struct artifact_record *rec,
                                     int has_signature_cooldown)
{
  const char *p = line;
  int i = 0, consumed = 0;

  /* Skip the seven v2.2 columns.  Walking whitespace-separated fields by hand
   * avoids sscanf assignment suppression, which cannot carry a field width
   * portably. */
  for (i = 0; i < 7; i++)
  {
    while (*p && isspace((unsigned char)*p))
      p++;
    if (!*p)
      return FALSE;
    while (*p && !isspace((unsigned char)*p))
      p++;
  }

  if (sscanf(p, " %511s %511s %ld %ld%n", rec->first_owner, rec->first_account,
             &rec->first_claimed_at, &rec->last_claimed_at, &consumed) != 4)
    return FALSE;
  p += consumed;

  if (sscanf(p, " %d %d %d %d %d %d %ld%n", &rec->claim_count, &rec->transfer_count,
             &rec->destroy_count, &rec->recovery_count, &rec->override_count, &rec->discovered,
             &rec->discovered_at, &consumed) != 7)
    return FALSE;
  p += consumed;

  if (sscanf(p, " %ld %ld%n", &rec->last_ability_use, &rec->last_proc, &consumed) != 2)
    return FALSE;
  p += consumed;

  if (has_signature_cooldown)
  {
    if (sscanf(p, " %ld%n", &rec->last_signature_proc, &consumed) != 1)
      return FALSE;
    p += consumed;
  }

  for (i = 0; i < ARTIFACT_MAX_EFFECTS; i++)
  {
    if (sscanf(p, " %ld%n", &rec->effect_used[i], &consumed) != 1)
      return FALSE;
    p += consumed;
  }

  rec->has_history = TRUE;
  rec->has_cooldowns = TRUE;

  return TRUE;
}

static int artifact_load_record(const char *line, enum artifact_file_format format,
                                struct artifact_record *rec)
{
  int saved_binding = ARTIFACT_BIND_NONE;
  long legacy_timestamp = 0;

  switch (format)
  {
  case ARTIFACT_FORMAT_V24:
    if (sscanf(line, "%d %511s %511s %d %d %ld %d", &rec->vnum, rec->owner, rec->account,
               &rec->level, &rec->exp, &rec->bound_time, &rec->instance_persisted) != 7)
      return FALSE;
    return artifact_load_modern_tail(line, rec, TRUE);

  case ARTIFACT_FORMAT_V23:
    if (sscanf(line, "%d %511s %511s %d %d %ld %d", &rec->vnum, rec->owner, rec->account,
               &rec->level, &rec->exp, &rec->bound_time, &rec->instance_persisted) != 7)
      return FALSE;
    return artifact_load_modern_tail(line, rec, FALSE);

  case ARTIFACT_FORMAT_V22:
    return sscanf(line, "%d %511s %511s %d %d %ld %d", &rec->vnum, rec->owner, rec->account,
                  &rec->level, &rec->exp, &rec->bound_time, &rec->instance_persisted) == 7;

  case ARTIFACT_FORMAT_V21:
    rec->instance_persisted = TRUE;
    return sscanf(line, "%d %511s %511s %d %d %ld", &rec->vnum, rec->owner, rec->account,
                  &rec->level, &rec->exp, &rec->bound_time) == 6;

  case ARTIFACT_FORMAT_V20:
    rec->instance_persisted = TRUE;
    return sscanf(line, "%d %511s %d %d %d %ld", &rec->vnum, rec->owner, &rec->level, &rec->exp,
                  &saved_binding, &rec->bound_time) == 6;

  case ARTIFACT_FORMAT_V1:
    if (sscanf(line, "%d %511s %ld", &rec->vnum, rec->owner, &legacy_timestamp) != 3)
      return FALSE;
    rec->bound_time = legacy_timestamp;
    rec->instance_persisted = TRUE;
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
  struct artifact_record rec;
  struct artifact_data *art = NULL;
  enum artifact_file_format file_format = ARTIFACT_FORMAT_UNKNOWN;
  enum artifact_file_format record_format = ARTIFACT_FORMAT_UNKNOWN;
  time_t now = time(0);
  int count = 0, i = 0, j = 0;

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
    art->last_signature_proc = 0;
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
    art->sig_proc = ART_SIG_NONE;
    art->sig_chance = 0;
    art->sig_align = ART_ALIGN_ANY;
    art->sig_miss_streak = 0;

    art->first_owner = strdup(ARTIFACT_OWNER_NONE);
    art->first_account = strdup(ARTIFACT_OWNER_NONE);
    art->first_claimed_at = 0;
    art->last_claimed_at = 0;
    art->claim_count = 0;
    art->transfer_count = 0;
    art->destroy_count = 0;
    art->recovery_count = 0;
    art->override_count = 0;
    art->discovered = FALSE;
    art->discovered_at = 0;

    art->acquisition = ART_ACQ_UNSET;
    art->campaigns = ART_CAMPAIGN_ALL;
    art->owner_policy = ART_OWNER_SECRET;
    art->available = TRUE;
    art->lore = NULL;
    art->acq_hint = NULL;

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
  {
    artifact_apply_template(&art_index[i]);
    artifact_apply_contract(&art_index[i]);
  }

  /* Overlay persisted state. */
  if ((fl = fopen(ARTIFACT_FILE, "r")))
  {
    while (fgets(line, sizeof(line), fl))
    {
      if (line[0] == '#')
      {
        if (strstr(line, "v2.4"))
          file_format = ARTIFACT_FORMAT_V24;
        else if (strstr(line, "v2.3"))
          file_format = ARTIFACT_FORMAT_V23;
        else if (strstr(line, "v2.2"))
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

      artifact_record_init(&rec);

      record_format = file_format == ARTIFACT_FORMAT_UNKNOWN ? artifact_detect_record_format(line)
                                                             : file_format;
      if (!artifact_load_record(line, record_format, &rec))
      {
        log("Artifacts: malformed ownership record ignored: %s", line);
        continue;
      }

      if (!(art = artifact_by_vnum(rec.vnum)))
      {
        log("Artifacts: record for vnum %d is not a known artifact - ignored.", rec.vnum);
        continue;
      }

      if (rec.level < 1 || rec.level > ARTIFACT_MAX_LEVEL)
        rec.level = 1;
      if (rec.exp < 0)
        rec.exp = 0;

      if (art->owner)
        free(art->owner);
      art->owner = strdup(rec.owner);

      if (!artifact_is_owned(rec.vnum))
        rec.instance_persisted = FALSE;

      if (record_format == ARTIFACT_FORMAT_V21 || record_format == ARTIFACT_FORMAT_V22 ||
          record_format == ARTIFACT_FORMAT_V23 || record_format == ARTIFACT_FORMAT_V24)
      {
        if (art->account)
          free(art->account);
        art->account = strdup(rec.account);
      }

      art->level = rec.level;
      art->experience = rec.exp;
      art->bound_time = (time_t)rec.bound_time;
      art->instance_persisted = rec.instance_persisted ? TRUE : FALSE;

      if (rec.has_history)
      {
        if (art->first_owner)
          free(art->first_owner);
        art->first_owner = strdup(rec.first_owner);

        if (art->first_account)
          free(art->first_account);
        art->first_account = strdup(rec.first_account);

        art->first_claimed_at = (time_t)MAX(0L, rec.first_claimed_at);
        art->last_claimed_at = (time_t)MAX(0L, rec.last_claimed_at);
        art->claim_count = MAX(0, rec.claim_count);
        art->transfer_count = MAX(0, rec.transfer_count);
        art->destroy_count = MAX(0, rec.destroy_count);
        art->recovery_count = MAX(0, rec.recovery_count);
        art->override_count = MAX(0, rec.override_count);
        art->discovered = rec.discovered ? TRUE : FALSE;
        art->discovered_at = (time_t)MAX(0L, rec.discovered_at);
      }
      else if (artifact_is_owned(rec.vnum))
      {
        /* An older file proves the artifact has been found, but not by whom
         * or when.  Record the discovery without inventing a first bearer. */
        art->discovered = TRUE;
      }

      if (rec.has_cooldowns)
      {
        /* A stamp in the future is a clock that moved backwards, not a power
         * that is owed a longer wait.  Treat it as ready. */
        art->last_ability_use = (rec.last_ability_use > 0 && rec.last_ability_use <= (long)now)
                                    ? (time_t)rec.last_ability_use
                                    : 0;
        art->last_proc =
            (rec.last_proc > 0 && rec.last_proc <= (long)now) ? (time_t)rec.last_proc : 0;
        art->last_signature_proc =
            (rec.last_signature_proc > 0 && rec.last_signature_proc <= (long)now)
                ? (time_t)rec.last_signature_proc
                : 0;

        for (j = 0; j < ARTIFACT_MAX_EFFECTS; j++)
          art->effect_used[j] = (rec.effect_used[j] > 0 && rec.effect_used[j] <= (long)now)
                                    ? (time_t)rec.effect_used[j]
                                    : 0;
      }
    }

    fclose(fl);
  }
  else
  {
    log("Artifacts: no %s yet - starting all artifacts unowned.", ARTIFACT_FILE);
    artifact_save();
  }

  artifact_dirty = FALSE;

  count = artifact_validate_metadata();

  if (count > 0)
    log("SYSERR: Artifacts: %d metadata problem%s found; affected rows are disabled.", count,
        count == 1 ? "" : "s");

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
    if (art_index[i].first_owner)
      free(art_index[i].first_owner);
    if (art_index[i].first_account)
      free(art_index[i].first_account);
    /* ability_name, ability_desc, lore, and acq_hint all point into the
     * static tables and are never owned by the registry. */
  }

  free(art_index);
  art_index = NULL;
  total_artifacts = 0;
  artifact_dirty = FALSE;
  artifact_persistence_extract_depth = 0;

  if (artifact_effect_disabled)
  {
    free(artifact_effect_disabled);
    artifact_effect_disabled = NULL;
  }
  artifact_effect_count = 0;
}

/* --------------------------------------------------------------------------
 * Ownership
 * -------------------------------------------------------------------------- */

/* --------------------------------------------------------------------------
 * Provenance
 *
 * Custody history is written here and read nowhere else that decides
 * anything.  No binding, uniqueness, or zone-reset check ever consults it:
 * `owner` still answers "who holds it", and these answer "what has happened
 * to it".  Keeping the two apart is the whole point of the separation.
 * -------------------------------------------------------------------------- */

/* A new character has claimed this artifact. */
static void artifact_record_claim(struct artifact_data *art, struct char_data *ch)
{
  time_t now = time(0);

  if (!art || !ch || IS_NPC(ch))
    return;

  art->claim_count++;
  art->last_claimed_at = now;

  if (!art->discovered)
  {
    art->discovered = TRUE;
    art->discovered_at = now;
  }

  if (art->first_claimed_at == 0)
  {
    if (art->first_owner)
      free(art->first_owner);
    art->first_owner = strdup(GET_NAME(ch));

    if (art->first_account)
      free(art->first_account);
    art->first_account =
        strdup((GET_ACCOUNT_NAME(ch) && *GET_ACCOUNT_NAME(ch)) ? GET_ACCOUNT_NAME(ch)
                                                               : ARTIFACT_OWNER_NONE);

    art->first_claimed_at = now;
  }

  artifact_mark_dirty();
}

/* The artifact has been let go of - dropped, given away, or otherwise
 * released back into the world. */
static void artifact_record_release(struct artifact_data *art)
{
  if (!art)
    return;

  art->transfer_count++;
  artifact_mark_dirty();
}

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

  art->sig_miss_streak = 0;
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
  artifact_record_claim(art, ch);
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
  {
    mudlog(NRM, LVL_STAFF, TRUE, "ARTIFACT: %s released %s", ch ? GET_NAME(ch) : "someone",
           GET_OBJ_SHORT(obj));
    artifact_record_release(art);
  }

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

  artifact_apply_passives(ch, art);

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

    if ((af->spell == SPELL_ARTIFACT_BONUS || af->spell == SPELL_ARTIFACT_PASSIVE) &&
        af->specific == tag)
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
 * Progressive passive powers
 *
 * Senses, speed, protections, and saving-throw grants are affects owned by
 * the artifact, not bits on the object prototype.  They unlock by artifact
 * level, carry the artifact's own tag so removal is exact, and are reapplied
 * whenever the artifact levels up.
 *
 * Do not split one power between here and the prototype.  If a power is in
 * this table it must not also be an ITEM_AFF bit, or unequipping will strip
 * one copy and leave the other.
 * -------------------------------------------------------------------------- */

void artifact_apply_passives(struct char_data *ch, struct artifact_data *art)
{
  struct affected_type af;
  sh_int tag = 0;
  int i = 0;

  if (!ch || !art)
    return;

  tag = (sh_int)(artifact_search(art->vnum) + 1);

  for (i = 0; artifact_passives[i].vnum != -1; i++)
  {
    if (artifact_passives[i].vnum != art->vnum)
      continue;

    if (art->level < artifact_passives[i].min_level)
      continue;

    new_affect(&af);
    af.spell = SPELL_ARTIFACT_PASSIVE;
    af.duration = -1;
    af.location = artifact_passives[i].location;
    af.modifier = artifact_passives[i].modifier;
    af.bonus_type = BONUS_TYPE_ENHANCEMENT;
    af.specific = tag;

    if (artifact_passives[i].aff_flag != 0)
      SET_BIT_AR(af.bitvector, artifact_passives[i].aff_flag);

    affect_to_char(ch, &af);
  }
}

void artifact_remove_passives(struct char_data *ch, struct artifact_data *art)
{
  struct affected_type *af = NULL, *af_next = NULL;
  sh_int tag = 0;

  if (!ch || !art)
    return;

  tag = (sh_int)(artifact_search(art->vnum) + 1);

  for (af = ch->affected; af; af = af_next)
  {
    af_next = af->next;

    if (af->spell == SPELL_ARTIFACT_PASSIVE && af->specific == tag)
      affect_remove(ch, af);
  }

  affect_total(ch);
}

/* --------------------------------------------------------------------------
 * Stacking groups
 *
 * Two temporary artifact powers in the same group never stack.  The one
 * already running holds; the second refuses, costs nothing, and says so.
 * Every temporary affect an artifact creates is stamped with its group in
 * `specific` so it can be found again without guessing at spell numbers.
 * -------------------------------------------------------------------------- */

int artifact_stack_active(struct char_data *ch, int group)
{
  struct affected_type *af = NULL;

  if (!ch || group == ART_STACK_NONE)
    return FALSE;

  for (af = ch->affected; af; af = af->next)
    if (af->spell == SPELL_ARTIFACT_SURGE && af->specific == (sh_int)group)
      return TRUE;

  return FALSE;
}

void artifact_stack_clear(struct char_data *ch, int group)
{
  struct affected_type *af = NULL, *af_next = NULL;
  int removed = 0;

  if (!ch || group == ART_STACK_NONE)
    return;

  for (af = ch->affected; af; af = af_next)
  {
    af_next = af->next;

    if (af->spell == SPELL_ARTIFACT_SURGE && af->specific == (sh_int)group)
    {
      affect_remove(ch, af);
      removed++;
    }
  }

  if (removed)
    affect_total(ch);
}

/* Apply one bounded, source-tagged, group-exclusive temporary modifier.  The
 * caller checks artifact_stack_active() first; this just stamps the affect. */
static void artifact_add_temp_affect(struct char_data *ch, int group, int location, int modifier,
                                     int bonus_type, int duration, int aff_flag)
{
  struct affected_type af;

  if (!ch)
    return;

  new_affect(&af);
  af.spell = SPELL_ARTIFACT_SURGE;
  af.duration = duration;
  af.location = location;
  af.modifier = modifier;
  af.bonus_type = bonus_type;
  af.specific = (sh_int)group;

  if (aff_flag != 0)
    SET_BIT_AR(af.bitvector, aff_flag);

  affect_to_char(ch, &af);
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
  int i = 0, worst = 0, burn = 0;

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

  /* Proportional to whoever is being rejected, with the historical dice as
   * the floor.  A flat 5d4 is 5 to 20 points, which is exactly the tier of
   * character most likely to be carrying an artifact it has no claim to. */
  burn = MAX(dice(ARTIFACT_BURN_DICE, ARTIFACT_BURN_SIDES),
             (GET_MAX_HIT(ch) * ARTIFACT_BURN_PERCENT) / 100);

  damage(ch, ch, burn, TYPE_UNDEFINED, DAM_FIRE, FALSE);
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

  art->destroy_count++;

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
 * ROL wrote these by hand, one per artifact.  Their table-owned rolls run
 * before the generic proc system.  Each handler explicitly owns its recharge
 * policy; Doombringer keeps the source's independent one-third-hour stamp.
 *
 * Returns TRUE when the victim died, so the caller stops touching it.
 * -------------------------------------------------------------------------- */

/* The power an artifact brings to bear: its wielder's experience sharpened by
 * however far the artifact itself has grown. */
static int artifact_effect_level(struct char_data *ch, struct artifact_data *art)
{
  return MAX(20, GET_LEVEL(ch) + (art->level * 2));
}

/* Fade, the Shadowblade: siphon a living foe through normal negative damage,
 * then return only a quarter of the life actually taken. */
static int artifact_proc_fade(struct char_data *ch, struct char_data *victim,
                              struct obj_data *weapon, struct artifact_data *art, int dam,
                              int is_critical)
{
  int amount = 0, damage_dealt = 0, healing = 0, victim_died = FALSE, victim_hit = 0;

  (void)dam;
  (void)is_critical;

  if (!IS_NPC(victim) || IS_DRAGON(victim) || IS_UNDEAD(victim))
    return FALSE;

  amount = MAX(1, (ARTIFACT_FADE_DRAIN_MAX_DAMAGE * art->level) / ARTIFACT_MAX_LEVEL);
  victim_hit = MAX(0, GET_HIT(victim));

  act("\tD$p drinks $N's life force and feeds it back to you!\tn", FALSE, ch, weapon, victim,
      TO_CHAR);
  act("\tD$N screams as $n's $p draws out $S life force.\tn", FALSE, ch, weapon, victim,
      TO_NOTVICT);
  act("\tD$p draws the warmth out of your body and into $n.\tn", FALSE, ch, weapon, victim,
      TO_VICT);

  damage_dealt = damage(ch, victim, amount, TYPE_UNDEFINED, DAM_NEGATIVE, FALSE);

  /* A lethal damage() call may extract the victim.  Use the pre-hit snapshot
   * to account for life removed without touching the victim again. */
  if (damage_dealt < 0)
  {
    victim_died = TRUE;
    damage_dealt = MIN(amount, victim_hit);
  }

  if (damage_dealt <= 0)
    return victim_died;

  if (GET_POS(ch) > POS_DEAD && GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    healing = (damage_dealt * ARTIFACT_FADE_DRAIN_HEAL_PERCENT) / 100;
    healing = MIN(healing, GET_MAX_HIT(ch) - GET_HIT(ch));
    if (healing > 0)
    {
      GET_HIT(ch) += healing;
      send_to_char(ch, "\tDThe stolen life restores %d hit point%s.\tn\r\n", healing,
                   healing == 1 ? "" : "s");
    }
  }

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);
  return victim_died;
}

/* Doombringer: turn one successful hit into a bounded run of real main-hand
 * attacks.  Its source ceiling was five; artifact progression earns that
 * ceiling one attack at a time. */
static int artifact_in_doombringer_burst = FALSE;
#ifdef LUMINARI_CUTEST
static int artifact_test_doombringer_attacks = 0;
#endif

static int artifact_proc_doombringer(struct char_data *ch, struct char_data *victim,
                                     struct obj_data *weapon, struct artifact_data *art, int dam,
                                     int is_critical)
{
  int extra_attacks = 0, i = 0, target_unavailable = FALSE, target_was_good = FALSE;

  (void)dam;
  (void)is_critical;

  if (!IS_NPC(victim) || artifact_in_doombringer_burst)
    return FALSE;

  /* The inherited procedure had its own one-third-MUD-hour recharge.  Keep it
   * separate so the generic proc cannot continually mask the named identity. */
  if (art->last_signature_proc > 0 &&
      (time(0) - art->last_signature_proc) < ARTIFACT_DOOMBRINGER_BURST_COOLDOWN)
    return FALSE;

  extra_attacks = MIN(ARTIFACT_DOOMBRINGER_BURST_MAX_ATTACKS, MAX(1, art->level));
  target_was_good = IS_GOOD(victim);
  art->last_signature_proc = time(0);
  artifact_mark_dirty();

  act("\tRBlack tendrils race down $p, and you explode into a killing frenzy!\tn", FALSE, ch,
      weapon, victim, TO_CHAR);
  act("\tRBlack tendrils race down $n's $p as $e explodes into a killing frenzy!\tn", FALSE, ch,
      weapon, victim, TO_NOTVICT);
  act("\tR$n blurs around you as $p drives in again and again!\tn", FALSE, ch, weapon, victim,
      TO_VICT);

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);
  artifact_in_doombringer_burst = TRUE;

  for (i = 0; i < extra_attacks; i++)
  {
    /* FIGHTING() is safe to compare after a lethal hit even when the mobile
     * has been queued for extraction.  Do not dereference the old target. */
    if (i > 0 && FIGHTING(ch) != victim)
    {
      target_unavailable = TRUE;
      break;
    }

#ifdef LUMINARI_CUTEST
    artifact_test_doombringer_attacks++;
#endif
    hit(ch, victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);

    if (FIGHTING(ch) != victim)
    {
      target_unavailable = TRUE;
      break;
    }
  }

  artifact_in_doombringer_burst = FALSE;

  if (target_was_good)
  {
    GET_ALIGNMENT(ch) = MAX(-1000, GET_ALIGNMENT(ch) - ARTIFACT_DOOMBRINGER_ALIGNMENT_COST);
    send_to_char(ch, "\tRA sliver of your conscience goes dark.\tn\r\n");
  }

  return target_unavailable;
}

/* Kelrarin's Hammer: the thrown hammer, and the alignment-gated mega blast. */
static int artifact_proc_kelrarin(struct char_data *ch, struct char_data *victim,
                                  struct obj_data *weapon, struct artifact_data *art, int dam,
                                  int is_critical)
{
  int amount = 0, mega = 0;

  (void)dam;
  (void)is_critical;

  /* The mega blast: only for the near-saintly, and only while unhurt. */
  if (GET_ALIGNMENT(ch) > ARTIFACT_KELRARIN_MEGA_ALIGN &&
      GET_HIT(ch) >= (GET_MAX_HIT(ch) * 9) / 10 && rand_number(1, ARTIFACT_KELRARIN_MEGA_ODDS) == 1)
  {
    /* Scaled by artifact level, exactly as the lesser throw below is.  A
     * flat 350 meant a level-1 hammer carried a level-5 nuke. */
    mega = MAX(ARTIFACT_KELRARIN_MEGA_MIN,
               (ARTIFACT_KELRARIN_MEGA_DAMAGE * art->level) / ARTIFACT_MAX_LEVEL);

    act("\tW$p blazes like a fallen star and BREAKS over $N!\tn", FALSE, ch, weapon, victim,
        TO_CHAR);
    act("\tW$p blazes like a fallen star and BREAKS over $N!\tn", FALSE, ch, weapon, victim,
        TO_NOTVICT);
    act("\tW$p breaks over you like the judgement of heaven!\tn", FALSE, ch, weapon, victim,
        TO_VICT);

    if (damage(ch, victim, mega, TYPE_UNDEFINED, DAM_HOLY, FALSE) == -1)
      return TRUE;

    /* ROL's SuddenDeath: anything left standing but badly broken simply ends.
     * Not against a boss, though.  Unrestricted, a one-in-thirty-three roll on
     * every swing was a reliable execute on any worn-down boss, which no other
     * artifact in the roster can do. */
    if (IS_NPC(victim) && !artifact_is_boss(ch, victim) && GET_HIT(victim) < mega)
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
                                struct obj_data *weapon, struct artifact_data *art, int dam,
                                int is_critical)
{
  struct char_data *tch = NULL;
  int amount = 0, bearer_share = 0, group_share = 0, total_healed = 0;

  (void)is_critical;

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

  /* This always-checked healback has its own recharge.  Sharing last_proc
   * made Kelrom's advertised generic proc unreachable on every damaging hit. */
  if (art->last_signature_proc > 0 &&
      (time(0) - art->last_signature_proc) < ARTIFACT_KELROM_HEALBACK_COOLDOWN)
    return FALSE;

  bearer_share = MAX(1, (dam * ARTIFACT_KELROM_HEALBACK_PERCENT * art->level) / 100);

  if (GROUP(ch))
  {
    /* The bearer takes the wounds, so the bearer takes the full share; the
     * rest of the group gets half. */
    group_share = MAX(1, (bearer_share * ARTIFACT_KELROM_GROUP_SHARE) / 100);

    simple_list(NULL);

    while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
    {
      if (IN_ROOM(tch) != IN_ROOM(ch))
        continue;
      if (GET_HIT(tch) >= GET_MAX_HIT(tch))
        continue;

      amount = MIN(tch == ch ? bearer_share : group_share, GET_MAX_HIT(tch) - GET_HIT(tch));
      GET_HIT(tch) += amount;
      total_healed += amount;
      send_to_char(tch, "\tGA warm green light spills from Kelrom and knits your wounds.\tn\r\n");
    }

    simple_list(NULL);
  }
  else if (GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    amount = MIN(bearer_share, GET_MAX_HIT(ch) - GET_HIT(ch));
    GET_HIT(ch) += amount;
    total_healed += amount;
    send_to_char(ch, "\tGA warm green light spills from Kelrom and knits your wounds.\tn\r\n");
  }

  /* A full-health party has received no proc.  Leave both its recharge and
   * progression untouched so the next eligible hit can still help. */
  if (total_healed <= 0)
    return FALSE;

  art->last_signature_proc = time(0);
  artifact_mark_dirty();
  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_HEAL);
  return FALSE;
}

/* Gesen, the Returning Axe: thrown, and it carries a full harm with it. */
static int artifact_proc_gesen(struct char_data *ch, struct char_data *victim,
                               struct obj_data *weapon, struct artifact_data *art, int dam,
                               int is_critical)
{
  (void)dam;
  (void)is_critical;

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

/* Avernus, the Black Blade: Bladesong keeps its wielder moving between the
 * blade's rarer life-stealing strikes. */
static int artifact_avernus_emergency_heal(struct char_data *ch, struct obj_data *weapon)
{
  if (GET_POS(ch) <= POS_DEAD || GET_HIT(ch) >= ARTIFACT_AVERNUS_HEAL_THRESHOLD ||
      GET_HIT(ch) >= GET_MAX_HIT(ch))
    return FALSE;

  GET_HIT(ch) = GET_MAX_HIT(ch);

  act("\tW$p drinks deep and pours everything it has back into you!\tn", FALSE, ch, weapon, NULL,
      TO_CHAR);
  act("\tW$p flares black and $n's wounds close before your eyes.\tn", FALSE, ch, weapon, NULL,
      TO_ROOM);

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_HEAL);
  return TRUE;
}

static void artifact_avernus_bladesong(struct char_data *ch, struct char_data *victim,
                                       struct obj_data *weapon)
{
  int healing = 0, missing = 0;

  if (GET_POS(ch) <= POS_DEAD)
    return;

  act("\tLYou charge, evade, and parry as $p guides you through the Bladesong.\tn", FALSE, ch,
      weapon, victim, TO_CHAR);
  act("\tL$n charges and parries with impossible grace as $p guides every step.\tn", FALSE, ch,
      weapon, victim, TO_NOTVICT);

  missing = MAX(0, GET_MAX_HIT(ch) - GET_HIT(ch));
  if (missing <= ARTIFACT_AVERNUS_BLADESONG_MIN_MISSING)
    return;

  healing = MIN(ARTIFACT_AVERNUS_BLADESONG_HEAL, missing);
  GET_HIT(ch) += healing;
  send_to_char(ch, "\tLThe rhythm closes %d hit point%s of your wounds.\tn\r\n", healing,
               healing == 1 ? "" : "s");
}

static void artifact_avernus_recover(struct char_data *ch, struct char_data *victim,
                                     struct obj_data *weapon)
{
  if (GET_POS(ch) <= POS_SLEEPING || GET_POS(ch) >= POS_FIGHTING)
    return;

  /* Bladesong defeats an ordinary knockdown, not sleep, paralysis, or a pin. */
  if (AFF_FLAGGED(ch, AFF_SLEEP) || AFF_FLAGGED(ch, AFF_PARALYZED) || AFF_FLAGGED(ch, AFF_PINNED))
    return;

  char_from_furniture(ch);
  change_position(ch, FIGHTING(ch) ? POS_FIGHTING : POS_STANDING);

  act("\tL$p pulls you from the ground and back into a fighting stance!\tn", FALSE, ch, weapon,
      victim, TO_CHAR);
  act("\tL$p pulls $n from the ground and back into a fighting stance!\tn", FALSE, ch, weapon,
      victim, TO_NOTVICT);
}

/* This reaction is checked on every successful hit.  Returning TRUE asks the
 * dispatcher to skip only Avernus's 1-in-31 main strike on a hit where the
 * emergency heal already spent the blade's attention. */
static int artifact_react_avernus(struct char_data *ch, struct char_data *victim,
                                  struct obj_data *weapon, struct artifact_data *art, int dam,
                                  int is_critical)
{
  (void)dam;
  (void)is_critical;

  if (GET_HIT(ch) < ARTIFACT_AVERNUS_HEAL_THRESHOLD &&
      rand_number(1, 100) <=
          ARTIFACT_AVERNUS_HEAL_CHANCE + (art->level * ARTIFACT_AVERNUS_HEAL_CHANCE_PER_LEVEL) &&
      artifact_avernus_emergency_heal(ch, weapon))
    return TRUE;

  if (rand_number(1, ARTIFACT_AVERNUS_BLADESONG_ODDS) == 1)
    artifact_avernus_bladesong(ch, victim, weapon);

  artifact_avernus_recover(ch, victim, weapon);
  return FALSE;
}

/* The inherited ceiling is 250 points transferred and three times that much
 * victim damage.  Artifact progression earns one fifth of the ceiling per
 * level; healing follows damage actually inflicted and never exceeds the
 * wielder's missing hit points. */
static int artifact_proc_avernus(struct char_data *ch, struct char_data *victim,
                                 struct obj_data *weapon, struct artifact_data *art, int dam,
                                 int is_critical)
{
  int damage_dealt = 0, healing = 0, requested_damage = 0, transfer = 0;
  int transfer_cap = 0, victim_died = FALSE, victim_hit = 0;

  (void)dam;
  (void)is_critical;

  if (!IS_LIVING(victim))
    return FALSE;

  transfer_cap = MAX(1, (ARTIFACT_AVERNUS_DRAIN_MAX_TRANSFER * art->level) / ARTIFACT_MAX_LEVEL);
  victim_hit = MAX(0, GET_HIT(victim));
  transfer = MIN(transfer_cap, victim_hit);
  if (transfer < transfer_cap)
    transfer = MIN(transfer_cap, transfer + ARTIFACT_AVERNUS_DRAIN_DEATH_MARGIN);
  requested_damage = transfer * ARTIFACT_AVERNUS_DRAIN_DAMAGE_MULTIPLIER;

  if (requested_damage <= 0)
    return FALSE;

  act("\tL$p glows black-white and draws $N's life through its blade!\tn", FALSE, ch, weapon,
      victim, TO_CHAR);
  act("\tL$p flares in $n's hands as a dark stream runs from $N into its blade!\tn", FALSE, ch,
      weapon, victim, TO_NOTVICT);
  act("\tL$p tears your life away in a flash of black-white light!\tn", FALSE, ch, weapon, victim,
      TO_VICT);

  damage_dealt = damage(ch, victim, requested_damage, TYPE_UNDEFINED, DAM_NEGATIVE, FALSE);

  /* A lethal damage() call may extract the victim.  Account from the pre-hit
   * snapshot without touching it again. */
  if (damage_dealt < 0)
  {
    victim_died = TRUE;
    damage_dealt = MIN(requested_damage, victim_hit);
  }

  if (damage_dealt <= 0)
    return victim_died;

  if (GET_POS(ch) > POS_DEAD && GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    healing = damage_dealt / ARTIFACT_AVERNUS_DRAIN_DAMAGE_MULTIPLIER;
    healing = MIN(healing, transfer);
    healing = MIN(healing, GET_MAX_HIT(ch) - GET_HIT(ch));
    if (healing > 0)
    {
      GET_HIT(ch) += healing;
      send_to_char(ch, "\tLThe stolen life restores %d hit point%s.\tn\r\n", healing,
                   healing == 1 ? "" : "s");
    }
  }

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);
  return victim_died;
}

/* Trorxek, the Staff of Ancient Oaks: a blinding strike on a critical hit. */
static int artifact_proc_trorxek(struct char_data *ch, struct char_data *victim,
                                 struct obj_data *weapon, struct artifact_data *art, int dam,
                                 int is_critical)
{
  struct affected_type af;

  (void)dam;

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

typedef int (*artifact_hand_proc_fn)(struct char_data *ch, struct char_data *victim,
                                     struct obj_data *weapon, struct artifact_data *art, int dam,
                                     int is_critical);

struct artifact_hand_proc_entry
{
  int vnum;
  artifact_hand_proc_fn reaction; /* always checked before the named roll */
  artifact_hand_proc_fn handler;
  int proc_odds; /* optional roll before the handler; zero = handler-owned */
  const char *description;
};

static const struct artifact_hand_proc_entry artifact_hand_procs[] = {
    {ART_VNUM_TRORXEK, NULL, artifact_proc_trorxek, 0, NULL},
    {ART_VNUM_FADE, NULL, artifact_proc_fade, ARTIFACT_FADE_DRAIN_ODDS,
     "siphon a living non-dragon NPC"},
    {ART_VNUM_DOOMBRINGER, NULL, artifact_proc_doombringer, ARTIFACT_DOOMBRINGER_BURST_ODDS,
     "burst into extra attacks against an NPC"},
    {ART_VNUM_KELRARIN, NULL, artifact_proc_kelrarin, 0, NULL},
    {ART_VNUM_KELROM, NULL, artifact_proc_kelrom, 0, NULL},
    {ART_VNUM_GESEN, NULL, artifact_proc_gesen, 0, NULL},
    {ART_VNUM_AVERNUS, artifact_react_avernus, artifact_proc_avernus, ARTIFACT_AVERNUS_DRAIN_ODDS,
     "steal a living foe's vitality for triple damage"},
    {-1, NULL, NULL, 0, NULL}};

static const struct artifact_hand_proc_entry *artifact_hand_proc_for_vnum(int vnum)
{
  int i = 0;

  for (i = 0; artifact_hand_procs[i].vnum != -1; i++)
    if (artifact_hand_procs[i].vnum == vnum)
      return &artifact_hand_procs[i];

  return NULL;
}

/* --------------------------------------------------------------------------
 * The reusable signature-proc library
 *
 * The seven procedures above are one function per artifact, inherited from
 * ROL.  Everything added since is a shape from this library, selected by a
 * row in artifact_templates[].  A new artifact reuses a shape; it does not
 * add a function.
 *
 * Every shape here obeys the same rules:
 *
 *   - the artifact's own internal cooldown gates it;
 *   - it uses normal damage, affect, and saving-throw helpers;
 *   - temporary affects it creates are source-tagged and group-exclusive;
 *   - target legality, immunity, and saves are explicit;
 *   - exactly one XP award per successful proc; and
 *   - chance selection is one call to rand_number(), so a test can pin it.
 * -------------------------------------------------------------------------- */

/* Re-entrancy guard for the flurry shape: the extra swings it buys must not
 * roll procs of their own.  ROL's equivalent was a global hit counter that
 * every character in the world shared. */
static int artifact_in_flurry = FALSE;

/* Does this artifact's alignment rule permit the proc to fire at all? */
static int artifact_align_ok(struct char_data *ch, struct char_data *victim, int rule)
{
  switch (rule)
  {
  case ART_ALIGN_ANY:
    return TRUE;

  case ART_ALIGN_TARGET_EVIL:
    return (victim && IS_EVIL(victim));

  case ART_ALIGN_TARGET_GOOD:
    return (victim && IS_GOOD(victim));

  case ART_ALIGN_SELF_GOOD:
    return (ch && IS_GOOD(ch));

  case ART_ALIGN_SELF_EVIL:
    return (ch && !IS_GOOD(ch));

  default:
    return TRUE;
  }
}

/* Earthcrier's shape: a controlled knockdown with a save and real immunity
 * rules.  ROL forced the target to sitting with no check at all. */
static int artifact_proc_knockdown(struct char_data *ch, struct char_data *victim,
                                   struct obj_data *weapon, struct artifact_data *art)
{
  int dc = ARTIFACT_KNOCKDOWN_DC + art->level;

  if (MOB_FLAGGED(victim, MOB_NOBASH) || AFF_FLAGGED(victim, AFF_FREE_MOVEMENT) ||
      IS_INCORPOREAL(victim) || GET_POS(victim) <= POS_SITTING)
    return FALSE;

  /* savingthrow() adds its own base DC.  Pass only the remainder so the
   * declared 14 + artifact-level challenge is the one actually rolled. */
  if (savingthrow(ch, victim, SAVING_REFL, 0, CAST_WEAPON_SPELL, dc - SAVING_THROW_BASE_DC,
                  NOSCHOOL))
  {
    act("$p slams down beside $N, who rides it out.", FALSE, ch, weapon, victim, TO_CHAR);
    act("$p slams down beside you and you ride it out.", FALSE, ch, weapon, victim, TO_VICT);
    return FALSE;
  }

  act("\tY$p lands like a falling wall and takes $N off $S feet!\tn", FALSE, ch, weapon, victim,
      TO_CHAR);
  act("\tY$p lands like a falling wall and takes $N off $S feet!\tn", FALSE, ch, weapon, victim,
      TO_NOTVICT);
  act("\tYThe ground comes up and hits you.\tn", FALSE, ch, weapon, victim, TO_VICT);

  GET_POS(victim) = POS_SITTING;
  WAIT_STATE(victim, PULSE_VIOLENCE * ARTIFACT_KNOCKDOWN_WAIT);

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);
  return FALSE;
}

/* Vengeance's shape: sustain while wounded, offense while whole.  The heal
 * branch is unconditional - a wounded bearer is a wounded bearer.  The
 * offense branch is the one the alignment rule gates. */
static int artifact_proc_mercy(struct char_data *ch, struct char_data *victim,
                               struct obj_data *weapon, struct artifact_data *art)
{
  int amount = 0;

  if (GET_MAX_HIT(ch) > 0 && (GET_HIT(ch) * 100) / GET_MAX_HIT(ch) < ARTIFACT_MERCY_WOUNDED_PERCENT)
  {
    amount = ARTIFACT_MERCY_HEAL_BASE + (art->level * 10) +
             dice(ARTIFACT_MERCY_HEAL_DICE, ARTIFACT_MERCY_HEAL_SIDES);

    GET_HIT(ch) = MIN(GET_HIT(ch) + amount, GET_MAX_HIT(ch));

    act("\tW$p pours light into you and the worst of it closes.\tn", FALSE, ch, weapon, NULL,
        TO_CHAR);
    act("\tW$p pours light into $n and $s wounds close.\tn", FALSE, ch, weapon, NULL, TO_ROOM);

    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);
    return FALSE;
  }

  if (!artifact_align_ok(ch, victim, art->sig_align))
    return FALSE;

  amount = dice(4 + art->level, 12) + (GET_LEVEL(ch) * 2);

  act("\tW$p finds something in $N that it has been waiting for.\tn", FALSE, ch, weapon, victim,
      TO_CHAR);
  act("\tW$p finds something in $N that it has been waiting for.\tn", FALSE, ch, weapon, victim,
      TO_NOTVICT);
  act("\tWThe light in $p is looking straight at you.\tn", FALSE, ch, weapon, victim, TO_VICT);

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);

  return (damage(ch, victim, amount, TYPE_UNDEFINED, DAM_HOLY, FALSE) == -1);
}

/* Tiamat's Stinger: steal only the life the normal damage pipeline actually
 * takes.  The source procedure moved hit points directly, ignored mitigation,
 * and could heal beyond maximum HP; none of those shortcuts belong here. */
static int artifact_proc_lifesteal(struct char_data *ch, struct char_data *victim,
                                   struct obj_data *weapon, struct artifact_data *art)
{
  int amount = 0, damage_dealt = 0, healing = 0, victim_died = FALSE, victim_hit = 0;

  amount =
      dice(ARTIFACT_STINGER_LIFESTEAL_DICE_BASE + art->level, ARTIFACT_STINGER_LIFESTEAL_DIE_SIZE) +
      (art->level * ARTIFACT_STINGER_LIFESTEAL_BONUS_PER_LEVEL) + GET_LEVEL(ch);
  victim_hit = MAX(0, GET_HIT(victim));

  act("\tMFive colors race along $p as it bites into $N and drinks deep!\tn", FALSE, ch, weapon,
      victim, TO_CHAR);
  act("\tMFive colors race along $p as it bites into $N and drinks deep!\tn", FALSE, ch, weapon,
      victim, TO_NOTVICT);
  act("\tMFive colors burn through $p as it tears the life from you!\tn", FALSE, ch, weapon, victim,
      TO_VICT);

  damage_dealt = damage(ch, victim, amount, TYPE_UNDEFINED, DAM_NEGATIVE, FALSE);

  /* damage() returns -1 after a kill.  At that point the victim may already
   * be extracted, so use the HP snapshot rather than touching it again. */
  if (damage_dealt < 0)
  {
    victim_died = TRUE;
    damage_dealt = MIN(amount, victim_hit);
  }

  if (damage_dealt <= 0)
    return victim_died;

  if (GET_POS(ch) > POS_DEAD && GET_HIT(ch) < GET_MAX_HIT(ch))
  {
    healing = MIN(damage_dealt, GET_MAX_HIT(ch) - GET_HIT(ch));
    GET_HIT(ch) += healing;
    send_to_char(ch, "\tMStolen vitality closes your wounds, restoring %d hit point%s.\tn\r\n",
                 healing, healing == 1 ? "" : "s");
  }

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);
  return victim_died;
}

/* Tormblade's shape: an alignment-conditioned critical that wards its bearer,
 * and an ordinary hit that occasionally strips what the target is hiding
 * behind. */
static int artifact_proc_ward(struct char_data *ch, struct char_data *victim,
                              struct obj_data *weapon, struct artifact_data *art, int is_critical)
{
  if (!artifact_align_ok(ch, victim, art->sig_align))
    return FALSE;

  if (is_critical)
  {
    if (artifact_stack_active(ch, ART_STACK_WARD))
      return FALSE;

    artifact_add_temp_affect(ch, ART_STACK_WARD, APPLY_AC, -(2 + art->level), BONUS_TYPE_DEFLECTION,
                             2 + (art->level / 2), 0);
    artifact_add_temp_affect(ch, ART_STACK_WARD, APPLY_SAVING_WILL, 1 + (art->level / 2),
                             BONUS_TYPE_DEFLECTION, 2 + (art->level / 2), 0);
    affect_total(ch);

    act("\tW$p answers the blow and closes around you.\tn", FALSE, ch, weapon, NULL, TO_CHAR);
    act("\tW$p answers the blow and closes around $n.\tn", FALSE, ch, weapon, NULL, TO_ROOM);

    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);
    return FALSE;
  }

  act("\tW$p sweeps through whatever $N was standing behind.\tn", FALSE, ch, weapon, victim,
      TO_CHAR);
  act("\tWSomething you were relying on is simply not there any more.\tn", FALSE, ch, weapon,
      victim, TO_VICT);

  call_magic(ch, victim, NULL, SPELL_DISPEL_MAGIC, 0, artifact_effect_level(ch, art),
             CAST_WEAPON_SPELL);

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);
  return (DEAD(victim) || GET_POS(victim) == POS_DEAD);
}

/* The halberd's shape: one roll, several weighted outcomes, and a real
 * chance of nothing at all.  Weights are literal here so the distribution is
 * visible rather than implied. */
static int artifact_proc_weighted(struct char_data *ch, struct char_data *victim,
                                  struct obj_data *weapon, struct artifact_data *art)
{
  int roll = rand_number(1, 100);

  if (roll <= 25) /* stagger */
  {
    if (AFF_FLAGGED(victim, AFF_STUN) || MOB_FLAGGED(victim, MOB_NOBASH))
      return FALSE;

    act("\tY$p catches $N wrong and $E stops making sense for a moment.\tn", FALSE, ch, weapon,
        victim, TO_CHAR);
    act("\tYThe world goes quiet and sideways.\tn", FALSE, ch, weapon, victim, TO_VICT);

    WAIT_STATE(victim, PULSE_VIOLENCE);
    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);
    return FALSE;
  }

  if (roll <= 45) /* slow */
  {
    if (AFF_FLAGGED(victim, AFF_SLOW))
      return FALSE;

    act("\tY$p opens $N up and something in $M gives.\tn", FALSE, ch, weapon, victim, TO_CHAR);
    call_magic(ch, victim, NULL, SPELL_SLOW, 0, artifact_effect_level(ch, art), CAST_WEAPON_SPELL);

    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);
    return (DEAD(victim) || GET_POS(victim) == POS_DEAD);
  }

  if (roll <= 70) /* a driven thrust */
  {
    act("\tY$n drives $p home to the haft!\tn", FALSE, ch, weapon, victim, TO_NOTVICT);
    act("\tYYou drive $p home to the haft!\tn", FALSE, ch, weapon, victim, TO_CHAR);
    act("\tY$p goes in further than it should be able to.\tn", FALSE, ch, weapon, victim, TO_VICT);

    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);

    return (damage(ch, victim, dice(3 + art->level, 10) + (GET_LEVEL(ch) * 2), TYPE_UNDEFINED,
                   DAM_PUNCTURE, FALSE) == -1);
  }

  /* Everything else: the spear finds nothing worth doing. */
  return FALSE;
}

/* Twilight's shape: a bounded, level-scaled surge that never reads the totals
 * its bearer has already earned.  ROL added the wielder's current hit and
 * damage rolls to themselves, compounding every other bonus in the game. */
static int artifact_proc_surge(struct char_data *ch, struct obj_data *weapon,
                               struct artifact_data *art)
{
  if (artifact_stack_active(ch, ART_STACK_COMBAT_SURGE))
    return FALSE;

  artifact_add_temp_affect(ch, ART_STACK_COMBAT_SURGE, APPLY_HITROLL,
                           ARTIFACT_SURGE_HITROLL * art->level, BONUS_TYPE_MORALE,
                           ARTIFACT_SURGE_DURATION, 0);
  artifact_add_temp_affect(ch, ART_STACK_COMBAT_SURGE, APPLY_DAMROLL,
                           ARTIFACT_SURGE_DAMROLL * art->level, BONUS_TYPE_MORALE,
                           ARTIFACT_SURGE_DURATION, 0);
  affect_total(ch);

  act("\tD$p goes cold in your hand and everything gets easier.\tn", FALSE, ch, weapon, NULL,
      TO_CHAR);
  act("\tD$p goes dark, and $n starts moving differently.\tn", FALSE, ch, weapon, NULL, TO_ROOM);

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);
  return FALSE;
}

/* Icedge's shape: a bounded burst of extra attacks.  The guard is per call,
 * not global, and the extra swings cannot proc anything themselves. */
static int artifact_proc_flurry(struct char_data *ch, struct char_data *victim,
                                struct obj_data *weapon, struct artifact_data *art)
{
  int extra = 0, i = 0;

  if (artifact_in_flurry)
    return FALSE;

  extra = rand_number(ARTIFACT_FLURRY_MIN,
                      MIN(ARTIFACT_FLURRY_MAX, ARTIFACT_FLURRY_MIN + (art->level / 2)));

  act("\tC$p goes somewhere ahead of your hand and you follow it.\tn", FALSE, ch, weapon, victim,
      TO_CHAR);
  act("\tC$p blurs, and $n is suddenly everywhere $N is not.\tn", FALSE, ch, weapon, victim,
      TO_NOTVICT);

  artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SIGNATURE);

  artifact_in_flurry = TRUE;

  for (i = 0; i < extra; i++)
  {
    if (DEAD(victim) || GET_POS(victim) == POS_DEAD || IN_ROOM(victim) != IN_ROOM(ch))
      break;

    hit(ch, victim, TYPE_UNDEFINED, DAM_COLD, 0, FALSE);
  }

  artifact_in_flurry = FALSE;

  return (DEAD(victim) || GET_POS(victim) == POS_DEAD);
}

/* Select and run whatever shape the template named.  Returns TRUE when the
 * victim is gone. */
static int artifact_reusable_chance_roll(int forced_roll)
{
  if (forced_roll >= 1 && forced_roll <= 100)
    return forced_roll;

  return rand_number(1, 100);
}

static int artifact_reusable_proc_with_roll(struct char_data *ch, struct char_data *victim,
                                            struct obj_data *weapon, struct artifact_data *art,
                                            int is_critical, int forced_roll)
{
  int force_proc = FALSE;

  if (art->sig_proc == ART_SIG_NONE)
    return FALSE;

  if (art->sig_proc != ART_SIG_LIFESTEAL && art->sig_chance <= 0)
    return FALSE;

  /* Stinger inherits a per-hit roll, not a once-per-cooldown power.  It still
   * stamps last_proc when it fires so the generic proc cannot double-trigger
   * on that swing, but neither a generic proc nor an earlier drain may stop
   * its next successful hit from rolling. */
  if (art->sig_proc != ART_SIG_LIFESTEAL && art->last_proc > 0 &&
      (time(0) - art->last_proc) < ARTIFACT_PROC_ICD)
    return FALSE;

  /* Every shape but mercy is gated wholesale by the alignment rule; mercy
   * applies it only to its offense branch. */
  if (art->sig_proc != ART_SIG_MERCY && !artifact_align_ok(ch, victim, art->sig_align))
    return FALSE;

  if (art->sig_proc == ART_SIG_LIFESTEAL)
  {
    art->sig_miss_streak++;
    force_proc = art->sig_miss_streak >= ARTIFACT_STINGER_LIFESTEAL_GUARANTEE;

    if (!force_proc &&
        (art->sig_chance <= 0 || artifact_reusable_chance_roll(forced_roll) > art->sig_chance))
      return FALSE;

    art->sig_miss_streak = 0;
  }
  /* A critical raises the ward without a roll; its ordinary dispel still uses
   * the configured per-hit chance. */
  else if (!(art->sig_proc == ART_SIG_WARD && is_critical) &&
           artifact_reusable_chance_roll(forced_roll) > art->sig_chance)
    return FALSE;

  art->last_proc = time(0);
  artifact_mark_dirty();

  switch (art->sig_proc)
  {
  case ART_SIG_KNOCKDOWN:
    return artifact_proc_knockdown(ch, victim, weapon, art);
  case ART_SIG_MERCY:
    return artifact_proc_mercy(ch, victim, weapon, art);
  case ART_SIG_WARD:
    return artifact_proc_ward(ch, victim, weapon, art, is_critical);
  case ART_SIG_WEIGHTED:
    return artifact_proc_weighted(ch, victim, weapon, art);
  case ART_SIG_SURGE:
    return artifact_proc_surge(ch, weapon, art);
  case ART_SIG_FLURRY:
    return artifact_proc_flurry(ch, victim, weapon, art);
  case ART_SIG_LIFESTEAL:
    return artifact_proc_lifesteal(ch, victim, weapon, art);
  default:
    log("SYSERR: artifact_reusable_proc: unknown shape %d on vnum %d", art->sig_proc, art->vnum);
    return FALSE;
  }
}

static int artifact_reusable_proc(struct char_data *ch, struct char_data *victim,
                                  struct obj_data *weapon, struct artifact_data *art,
                                  int is_critical)
{
  return artifact_reusable_proc_with_roll(ch, victim, weapon, art, is_critical, 0);
}

/* Dispatch.  Returns TRUE when the victim is gone. */
static int artifact_signature_proc(struct char_data *ch, struct char_data *victim,
                                   struct obj_data *weapon, struct artifact_data *art, int dam,
                                   int is_critical, int force_hand_proc)
{
  const struct artifact_hand_proc_entry *hand_proc = NULL;

  if (art->sig_proc != ART_SIG_NONE)
    return artifact_reusable_proc(ch, victim, weapon, art, is_critical);

  if (!(hand_proc = artifact_hand_proc_for_vnum(art->vnum)))
    return FALSE;

  if (!force_hand_proc && hand_proc->reaction &&
      hand_proc->reaction(ch, victim, weapon, art, dam, is_critical))
    return FALSE;

  if (!force_hand_proc && hand_proc->proc_odds > 0 && rand_number(1, hand_proc->proc_odds) != 1)
    return FALSE;

  return hand_proc->handler(ch, victim, weapon, art, dam, is_critical);
}

static int artifact_generic_proc(struct char_data *ch, struct char_data *victim,
                                 struct obj_data *weapon, struct artifact_data *art, int proc_type)
{
  struct affected_type af;
  int amount = 0, proc_fired = FALSE, victim_died = FALSE;

  switch (proc_type)
  {
  case ARTIFACT_PROC_SOUL:
    proc_fired = TRUE;
    amount = dice(art->level, 6);
    act("$p glows with dark energy as it tears at $N's soul!", FALSE, ch, weapon, victim, TO_CHAR);
    act("$p glows with dark energy as it tears at $N's soul!", FALSE, ch, weapon, victim,
        TO_NOTVICT);
    act("$p tears at your very soul!", FALSE, ch, weapon, victim, TO_VICT);
    victim_died = (damage(ch, victim, amount, TYPE_UNDEFINED, DAM_NEGATIVE, FALSE) == -1);
    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_SOUL);
    break;

  case ARTIFACT_PROC_HEAL:
    if (GET_HIT(ch) >= GET_MAX_HIT(ch))
      break;
    proc_fired = TRUE;
    amount = dice(art->level, 4);
    GET_HIT(ch) = MIN(GET_HIT(ch) + amount, GET_MAX_HIT(ch));
    act("$p glows with holy light, healing your wounds!", FALSE, ch, weapon, NULL, TO_CHAR);
    act("$p glows with holy light, healing $n's wounds!", FALSE, ch, weapon, NULL, TO_ROOM);
    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_HEAL);
    break;

  case ARTIFACT_PROC_FEAR:
    if (AFF_FLAGGED(victim, AFF_FEAR))
      break;
    proc_fired = TRUE;
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
    proc_fired = TRUE;
    act("$p curses $N with impending doom!", FALSE, ch, weapon, victim, TO_CHAR);
    act("$p curses $N with impending doom!", FALSE, ch, weapon, victim, TO_NOTVICT);
    act("You feel doomed!", FALSE, ch, weapon, victim, TO_VICT);
    victim_died =
        (damage(ch, victim, dice(art->level, 8), TYPE_UNDEFINED, DAM_NEGATIVE, FALSE) == -1);
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
    proc_fired = TRUE;
    act("$p ERUPTS with ultimate power, utterly destroying $N!", FALSE, ch, weapon, victim,
        TO_CHAR);
    act("$p ERUPTS with ultimate power, utterly destroying $N!", FALSE, ch, weapon, victim,
        TO_NOTVICT);
    victim_died =
        (damage(ch, victim, GET_HIT(victim) + 100, TYPE_UNDEFINED, DAM_NEGATIVE, FALSE) == -1);
    artifact_grant_xp_obj(ch, weapon, ARTIFACT_XP_PROC_ULTIMATE);
    break;

  default:
    break;
  }

  /* The displayed percentage is an attempt rate.  An ineligible branch is
   * silent and leaves the artifact ready for a later hit. */
  if (proc_fired)
  {
    art->last_proc = time(0);
    artifact_mark_dirty();
  }
  return victim_died;
}

int artifact_weapon_proc(struct char_data *ch, struct char_data *victim, struct obj_data *weapon,
                         int dam, int is_critical)
{
  struct artifact_data *art = NULL;
  int proc_type = 0;

  if (!ch || !victim || !weapon || !art_index)
    return FALSE;

  if (!(art = artifact_of_obj(weapon)))
    return FALSE;

  /* Extra swings bought by a bounded multi-hit proc are free hits, not fresh
   * chances at every proc the artifact owns. */
  if (artifact_in_flurry || artifact_in_doombringer_burst)
    return FALSE;

  /* The hand-written procedures roll first and answer to nothing else. */
  if (artifact_signature_proc(ch, victim, weapon, art, dam, is_critical, FALSE))
    return TRUE;

  if (art->proc_chance <= 0)
    return FALSE;

  /* Internal cooldown, so a fast weapon cannot chain procs every swing. */
  if (art->last_proc > 0 && (time(0) - art->last_proc) < ARTIFACT_PROC_ICD)
    return FALSE;

  if (rand_number(1, 100) > art->proc_chance)
    return FALSE;

  proc_type = rand_number(1, art->level);
  return artifact_generic_proc(ch, victim, weapon, art, proc_type);
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

/* Collect the invoker's eligible same-room group members into `out`, without
 * touching anyone yet.
 *
 * Selection has to finish before any effect runs.  An effect that moves,
 * extracts, or ungroups a participant would otherwise change the very list
 * being walked, and any later "same room as the caller" test would be
 * comparing against wherever the caller had already been sent.  So: snapshot
 * the origin room, snapshot the members, then act.
 *
 * `include_self` decides whether the caller is in the returned list; the
 * caller is always eligible when it is set, group or no group.  Returns the
 * number collected. */
static int artifact_collect_group(struct char_data *ch, room_rnum origin, struct char_data **out,
                                  int max_out, int include_self)
{
  struct char_data *tch = NULL;
  int count = 0;

  if (!ch || !out || max_out <= 0)
    return 0;

  if (include_self)
    out[count++] = ch;

  if (!GROUP(ch))
    return count;

  simple_list(NULL);

  while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) != NULL)
  {
    if (count >= max_out)
      break;
    if (tch == ch)
      continue;
    if (IN_ROOM(tch) != origin)
      continue;
    if (DEAD(tch) || GET_POS(tch) <= POS_DEAD)
      continue;

    out[count++] = tch;
  }

  simple_list(NULL);

  return count;
}

/* Amaukekel's sunlit path: the whole group leaves at once.
 *
 * The origin room is captured before anyone moves.  Recalling the caller
 * first and then asking "is this member in my room" compares each member's
 * real location against the caller's destination, which quietly skips every
 * ordinary nearby group member. */
static int artifact_dimension_shift(struct char_data *ch, struct obj_data *obj)
{
  struct char_data *targets[ARTIFACT_VALOR_MAX_TARGETS];
  room_rnum origin = IN_ROOM(ch);
  int count = 0, moved = 0, i = 0;

  if (origin == NOWHERE)
    return FALSE;

  count = artifact_collect_group(ch, origin, targets, ARTIFACT_VALOR_MAX_TARGETS, TRUE);

  act("\tW$n lifts $p and a corridor of sunlight opens in the air.\tn", FALSE, ch, obj, NULL,
      TO_ROOM);
  send_to_char(ch, "\tWYou lift the rod and a corridor of sunlight opens before you.\tn\r\n");

  for (i = 0; i < count; i++)
  {
    /* A previous recall may have moved or extracted this one. */
    if (!targets[i] || DEAD(targets[i]))
      continue;
    if (targets[i] != ch && IN_ROOM(targets[i]) != origin)
      continue;

    call_magic(ch, targets[i], NULL, SPELL_WORD_OF_RECALL, 0, MAX(20, GET_LEVEL(ch)),
               CAST_WEAPON_SPELL);
    moved++;
  }

  return (moved > 0);
}

/* Courage's shape: the first artifact power that helps the people standing
 * with its bearer rather than the bearer alone.
 *
 * One cooldown and one XP award per activation, however many answer to it.
 * If nobody is eligible the invocation refuses outright and costs nothing -
 * the caller only stamps the recharge when this returns TRUE. */
static int artifact_group_valor(struct char_data *ch, struct obj_data *obj,
                                struct artifact_data *art, int stack_group)
{
  struct char_data *targets[ARTIFACT_VALOR_MAX_TARGETS];
  room_rnum origin = IN_ROOM(ch);
  int count = 0, reached = 0, i = 0;

  if (origin == NOWHERE)
    return FALSE;

  count = artifact_collect_group(ch, origin, targets, ARTIFACT_VALOR_MAX_TARGETS, TRUE);

  for (i = 0; i < count; i++)
  {
    if (!targets[i] || DEAD(targets[i]))
      continue;
    if (IN_ROOM(targets[i]) != origin)
      continue;

    /* Morale does not stack with itself, and a refusal for one person is not
     * a refusal for the group. */
    if (artifact_stack_active(targets[i], stack_group))
    {
      send_to_char(targets[i], "You are already as brave as you are going to get.\r\n");
      continue;
    }

    artifact_add_temp_affect(targets[i], stack_group, APPLY_HITROLL,
                             ARTIFACT_VALOR_HITROLL + art->level, BONUS_TYPE_MORALE,
                             ARTIFACT_VALOR_DURATION, 0);
    artifact_add_temp_affect(targets[i], stack_group, APPLY_SAVING_WILL,
                             ARTIFACT_VALOR_SAVES + (art->level / 2), BONUS_TYPE_MORALE,
                             ARTIFACT_VALOR_DURATION, 0);
    artifact_add_temp_affect(targets[i], stack_group, APPLY_HIT,
                             ARTIFACT_VALOR_HP_PER_LEVEL * art->level, BONUS_TYPE_MORALE,
                             ARTIFACT_VALOR_DURATION, 0);
    affect_total(targets[i]);

    send_to_char(targets[i], "\tYSomething goes through you, and you stop being afraid.\tn\r\n");
    reached++;
  }

  if (!reached)
  {
    send_to_char(ch, "Nobody here needs the courage you are offering.\r\n");
    return FALSE;
  }

  act("\tY$n raises $p and the sound of it goes straight through everyone here.\tn", FALSE, ch, obj,
      NULL, TO_ROOM);
  send_to_char(ch, "\tYYou raise the mace, and %d take heart from it.\tn\r\n", reached);

  return TRUE;
}

/* Icedge's whispered ward. */
static int artifact_frost_ward(struct char_data *ch, struct obj_data *obj,
                               struct artifact_data *art, int stack_group)
{
  if (artifact_stack_active(ch, stack_group))
  {
    send_to_char(ch, "You are already wearing one ward; a second will not settle.\r\n");
    return FALSE;
  }

  artifact_add_temp_affect(ch, stack_group, APPLY_RES_COLD, 10 + (art->level * 5),
                           BONUS_TYPE_DEFLECTION, 5 + art->level, 0);
  artifact_add_temp_affect(ch, stack_group, APPLY_AC, -(1 + art->level), BONUS_TYPE_DEFLECTION,
                           5 + art->level, 0);
  affect_total(ch);

  act("\tCFrost crawls up $n and settles into a shell.\tn", FALSE, ch, obj, NULL, TO_ROOM);
  send_to_char(ch, "\tCYou breathe the word and rime closes over you.\tn\r\n");

  return TRUE;
}

/* Wyrmfang's commanded hunter's sight. */
static int artifact_dragon_sight(struct char_data *ch, struct obj_data *obj,
                                 struct artifact_data *art, int stack_group)
{
  if (artifact_stack_active(ch, stack_group))
  {
    send_to_char(ch, "The spear is already showing you everything it intends to.\r\n");
    return FALSE;
  }

  artifact_add_temp_affect(ch, stack_group, APPLY_NONE, 0, BONUS_TYPE_ENHANCEMENT,
                           5 + (art->level * 2), AFF_DETECT_ALIGN);
  artifact_add_temp_affect(ch, stack_group, APPLY_HITROLL, 1 + (art->level / 2),
                           BONUS_TYPE_ENHANCEMENT, 5 + (art->level * 2), 0);
  affect_total(ch);

  act("\tG$n levels $p and goes very still.\tn", FALSE, ch, obj, NULL, TO_ROOM);
  send_to_char(ch, "\tGThe spear shows you what is worth killing.\tn\r\n");

  return TRUE;
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
  int recruited = 0, cap = 0, hp_cap = 0;

  if (IN_ROOM(ch) == NOWHERE)
    return FALSE;

  /* How large a thing will answer, scaled by artifact level.  ROL's flat 2000
   * is the ceiling a grown horn reaches, not the entry price: a 2000-max-HP
   * mobile is a mini-boss in most content, and the contract line for this
   * power is "recruits the lesser creatures nearby". */
  hp_cap = MAX(1, (ARTIFACT_CHARM_MAX_HP * art->level) / ARTIFACT_MAX_LEVEL);

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
    if (GET_MAX_HIT(vict) > hp_cap)
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
  int hit_count = 0, amount = 0, cap = 0;

  if (IN_ROOM(ch) == NOWHERE)
    return FALSE;

  /* One target plus one per artifact level.  Uncapped, this was the only
   * multi-target power in the roster whose total output rose with however
   * many hostiles happened to be standing in the room; Doom Blast has had a
   * flat cap of five since it was written. */
  cap = ARTIFACT_ANNIHILATION_TARGETS_BASE + (ARTIFACT_ANNIHILATION_TARGETS_PER_LEVEL * art->level);

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

  for (vict = world[IN_ROOM(ch)].people; vict && hit_count < cap; vict = next_vict)
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
static int artifact_enrage(struct char_data *ch, struct obj_data *obj, struct artifact_data *art,
                           int stack_group)
{
  int i = 0;
  const int locations[3] = {APPLY_HITROLL, APPLY_DAMROLL, APPLY_STR};
  const int modifiers[3] = {4, 6, 4};

  /* Rage and Twilight's surge are the same kind of thing happening to the
   * same person, so they share a group and the first one wins. */
  if (artifact_stack_active(ch, stack_group))
  {
    send_to_char(ch, "You are already as far gone as the blade can take you.\r\n");
    return FALSE;
  }

  for (i = 0; i < 3; i++)
    artifact_add_temp_affect(ch, stack_group, locations[i], modifiers[i] + art->level,
                             BONUS_TYPE_MORALE, ARTIFACT_ENRAGE_DURATION, 0);

  affect_total(ch);

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
    return artifact_enrage(ch, obj, art, effect->stack_group);

  case ART_EFFECT_GROUP_VALOR:
    return artifact_group_valor(ch, obj, art, effect->stack_group);

  case ART_EFFECT_FROST_WARD:
    return artifact_frost_ward(ch, obj, art, effect->stack_group);

  case ART_EFFECT_DRAGON_SIGHT:
    return artifact_dragon_sight(ch, obj, art, effect->stack_group);

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

/* Does this target rule expect an argument after the phrase? */
static int artifact_target_takes_argument(int target_type)
{
  return (target_type != ART_TARGET_NONE && target_type != ART_TARGET_FIGHTING &&
          target_type != ART_TARGET_GROUP_ROOM);
}

/* One matcher for every invocation channel.
 *
 * The phrase, the channel it answers on, its displayed help, and the runtime
 * dispatch all come from the same row of artifact_effects[].  Adding a
 * channel to an artifact is a data change; it is never a second copy of this
 * function. */
static int artifact_invoke_trigger(struct char_data *ch, const char *speech, int channel)
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

    if (artifact_effect_is_disabled(i))
      continue;

    if (effect->channel != channel)
      continue;

    phrase_len = strlen(effect->phrase);

    if (strncmp(said, effect->phrase, phrase_len) != 0)
      continue;

    if (!artifact_target_takes_argument(effect->target_type))
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
    artifact_mark_dirty();
    artifact_grant_xp_obj(ch, obj, ARTIFACT_XP_CALLED_EFFECT);
    artifact_save_if_dirty();

    return TRUE;
  }

  return FALSE;
}

/* Hooked into do_say().  Returns TRUE when the speech invoked something, but
 * the speech itself is never suppressed - saying the words aloud is the
 * point. */
int artifact_speech_trigger(struct char_data *ch, const char *speech)
{
  return artifact_invoke_trigger(ch, speech, ART_INVOKE_SAY);
}

/* Hooked into do_whisper(), after the target has been resolved.  A whispered
 * word is not shouted across the room, which is the whole reason an artifact
 * would ask for this channel. */
int artifact_whisper_trigger(struct char_data *ch, const char *speech)
{
  return artifact_invoke_trigger(ch, speech, ART_INVOKE_WHISPER);
}

/* The explicit command channel, reached through 'invoke <phrase>'.  For
 * artifacts whose powers are not spoken at all. */
int artifact_command_trigger(struct char_data *ch, const char *speech)
{
  return artifact_invoke_trigger(ch, speech, ART_INVOKE_COMMAND);
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
               "  artifact                  - show this help\r\n"
               "  artifact roster           - the chronicle: every artifact and its state\r\n"
               "  artifact chronicle <name> - what is known of one artifact\r\n"
               "  artifact list             - list the artifacts you carry\r\n"
               "  artifact info <item>      - detailed information on one artifact\r\n"
               "  artifact progress         - level and experience of your artifacts\r\n"
               "  artifact abilities        - abilities you can invoke right now\r\n"
               "  invoke <word>             - artifact powers with a spoken command\r\n"
               "\r\n"
               "\tYThe chronicle:\tn\r\n"
               "  \tcartifact roster\tn shows what the world knows: whether each artifact\r\n"
               "  is unawakened, unclaimed, held, lost, or recoverable. Names appear\r\n"
               "  once an artifact has been found; how it is found becomes common\r\n"
               "  knowledge only after somebody has done it. Bearers are named only\r\n"
               "  when that particular artifact's story is a public one.\r\n"
               "\r\n"
               "\tYBinding:\tn\r\n"
               "  None            - can be freely traded\r\n"
               "  Bind on Pickup  - soulbound the moment you take it\r\n"
               "  Bind on Equip   - bound the first time you wear it\r\n"
               "  Bind on Account - usable by any character on your account\r\n"
               "\r\n"
               "\tYCalled effects:\tn\r\n"
               "  Some artifacts answer to words. Hold or wear the artifact and\r\n"
               "  use the channel it listens on: \tcsay\tn the phrase aloud, \tcwhisper\tn it\r\n"
               "  to someone, or \tcinvoke\tn it as a command. Each artifact decides\r\n"
               "  which; \tcartifact info <item>\tn tells you exactly how to speak to it,\r\n"
               "  if it is willing to tell you. Each effect recharges on its own\r\n"
               "  clock, from once an hour to once a week, and those clocks now\r\n"
               "  survive a reboot.\r\n"
               "\r\n"
               "\tYAlways-on powers:\tn\r\n"
               "  Some artifacts grant senses, speed, or protections simply for\r\n"
               "  being worn. Many of those stay shut until the artifact has grown\r\n"
               "  into them. \tcartifact info <item>\tn shows both what is active now\r\n"
               "  and what is still locked.\r\n"
               "\r\n"
               "\tYStacking:\tn\r\n"
               "  Temporary artifact powers of the same kind do not stack. A rage\r\n"
               "  and a battle surge are the same thing happening to you, and the\r\n"
               "  first one holds until it runs out.\r\n"
               "\r\n"
               "\tYOaths:\tn\r\n"
               "  A few artifacts are sworn to one class and demand real depth\r\n"
               "  in it. Wear one without that depth and it will burn you every\r\n"
               "  tick you keep it on. It will conceal and refuse every named\r\n"
               "  power, whether called with words or used as an active command.\r\n"
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
    if (artifact_effects[i].vnum == art->vnum && !artifact_effect_is_disabled(i))
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

    if (effect->vnum != art->vnum || artifact_effect_is_disabled(i))
      continue;

    /* The channel, the phrase, and the argument shape all come from the same
     * row that dispatches the effect, so this can never drift from what the
     * runtime actually accepts. */
    switch (effect->channel)
    {
    case ART_INVOKE_WHISPER:
      send_to_char(ch, "  \tcwhisper <someone> \"%s%s\"\tn\r\n", effect->phrase,
                   artifact_target_takes_argument(effect->target_type) ? " <target>" : "");
      break;

    case ART_INVOKE_COMMAND:
      send_to_char(ch, "  \tcinvoke %s%s\tn\r\n", effect->phrase,
                   artifact_target_takes_argument(effect->target_type) ? " <target>" : "");
      break;

    default:
      send_to_char(ch, "  \tcsay \"%s%s\"\tn\r\n", effect->phrase,
                   artifact_target_takes_argument(effect->target_type) ? " <target>" : "");
      break;
    }

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
  const struct artifact_hand_proc_entry *hand_proc = NULL;
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

  /* Passive powers, including the ones this artifact has not grown into yet.
   * Showing the locked ones is the point: they are what progression buys. */
  {
    int shown = 0;

    for (i = 0; artifact_passives[i].vnum != -1; i++)
    {
      if (artifact_passives[i].vnum != art->vnum)
        continue;

      if (!shown++)
        send_to_char(ch, "\r\n\tYAlways-on powers:\tn\r\n");

      if (art->level >= artifact_passives[i].min_level)
        send_to_char(ch, "  \tg[active]\tn  %s\r\n", artifact_passives[i].desc);
      else
        send_to_char(ch, "  \tD[level %d]\tn %s\r\n", artifact_passives[i].min_level,
                     artifact_passives[i].desc);
    }
  }

  if (art->proc_chance > 0)
    send_to_char(ch,
                 "\r\n\tYCombat:\tn %d%% chance per hit to attempt a special strike.\r\n"
                 "  Attempts that cannot affect anything spend no cooldown.\r\n",
                 art->proc_chance);

  if (art->vnum == ART_VNUM_KELROM)
    send_to_char(ch,
                 "\r\n\tYSignature:\tn Damaging hits restore %d%% of their damage as group "
                 "healing.\r\n"
                 "  The bearer receives the full share; nearby group members receive %d%% of "
                 "it.\r\n"
                 "  Independent %d-second recharge; no recharge is spent when no one needs "
                 "healing.\r\n"
                 "  Striking an animal instead turns the axe's punishment on its wielder.\r\n",
                 ARTIFACT_KELROM_HEALBACK_PERCENT * art->level, ARTIFACT_KELROM_GROUP_SHARE,
                 ARTIFACT_KELROM_HEALBACK_COOLDOWN);

  hand_proc = artifact_hand_proc_for_vnum(art->vnum);
  if (hand_proc && hand_proc->proc_odds > 0 && hand_proc->description)
  {
    send_to_char(ch, "\r\n\tYSignature:\tn 1-in-%d chance per hit to %s.\r\n", hand_proc->proc_odds,
                 hand_proc->description);

    if (art->vnum == ART_VNUM_FADE)
      send_to_char(ch,
                   "  Drain: %d x artifact level negative damage, up to %d.\r\n"
                   "  Healing equals %d%% of damage inflicted, capped by missing hit points.\r\n",
                   ARTIFACT_FADE_DRAIN_MAX_DAMAGE / ARTIFACT_MAX_LEVEL,
                   ARTIFACT_FADE_DRAIN_MAX_DAMAGE, ARTIFACT_FADE_DRAIN_HEAL_PERCENT);
    else if (art->vnum == ART_VNUM_DOOMBRINGER)
      send_to_char(ch,
                   "  Burst: one extra main-hand attack per artifact level, up to %d.\r\n"
                   "  Independent %d-second recharge; good targets cost %d alignment.\r\n",
                   ARTIFACT_DOOMBRINGER_BURST_MAX_ATTACKS, ARTIFACT_DOOMBRINGER_BURST_COOLDOWN,
                   ARTIFACT_DOOMBRINGER_ALIGNMENT_COST);
    else if (art->vnum == ART_VNUM_AVERNUS)
      send_to_char(
          ch,
          "  Drain: up to %d x artifact level transferred from a living foe; triple damage.\r\n"
          "  Healing follows one-third of damage inflicted and is capped by missing hit points.\r\n"
          "  Bladesong may restore %d hit points or recover from an ordinary knockdown.\r\n"
          "  Below %d HP, it has a %d + (%d x level)%% chance per hit to restore full HP.\r\n",
          ARTIFACT_AVERNUS_DRAIN_MAX_TRANSFER / ARTIFACT_MAX_LEVEL, ARTIFACT_AVERNUS_BLADESONG_HEAL,
          ARTIFACT_AVERNUS_HEAL_THRESHOLD, ARTIFACT_AVERNUS_HEAL_CHANCE,
          ARTIFACT_AVERNUS_HEAL_CHANCE_PER_LEVEL);
  }

  if (art->sig_proc != ART_SIG_NONE && art->sig_chance > 0)
  {
    send_to_char(ch, "\r\n\tYSignature:\tn %d%% chance per hit to %s.\r\n", art->sig_chance,
                 artifact_signature_desc(art->sig_proc));

    if (art->sig_proc == ART_SIG_LIFESTEAL)
      send_to_char(ch,
                   "  Drain: (2 + artifact level)d10 + (10 x artifact level) + wielder level.\r\n"
                   "  Healing equals damage inflicted, capped by missing hit points.\r\n");
    else if (art->sig_proc == ART_SIG_KNOCKDOWN)
      send_to_char(ch,
                   "  Save: base Reflex DC %d + artifact level (%d at this level).\r\n"
                   "  No effect on no-bash, free-moving, incorporeal, or already-down foes.\r\n",
                   ARTIFACT_KNOCKDOWN_DC, ARTIFACT_KNOCKDOWN_DC + art->level);
  }

  if (art->ability_name && !artifact_class_ok(ch, art))
  {
    send_to_char(ch, "\r\n\tYAbility:\tn It keeps its own counsel. That power is not for you.\r\n");
  }
  else if (art->ability_name)
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
      send_to_char(
          ch, "  \trIt does not recognize you, burns you, and withholds its named powers.\tn\r\n");
  }

  send_to_char(ch, "\tY=====================================\tn\r\n");
}

/* --------------------------------------------------------------------------
 * The chronicle
 *
 * A public roster of what exists, what state it is in, and roughly how it is
 * come by.  Display policy, in one place:
 *
 *   - artifacts not enabled for the running campaign are not listed at all;
 *   - an undiscovered artifact is a rumour: state and lore, but no name;
 *   - the current bearer is named only when the artifact's contract makes
 *     bearers public AND it is actually held;
 *   - otherwise the chronicle names the first bearer, which is history and
 *     cannot be used to find anyone;
 *   - the acquisition hint is written in world terms and never carries a room
 *     or a vnum - boot validation refuses any hint that looks like it does;
 *   - every line is derived from the registry when asked for.  There is no
 *     second list to go stale.
 * -------------------------------------------------------------------------- */

/* How much of an artifact's story this character has earned.  Stage 0 is a
 * rumour, stage 1 is a named legend, stage 2 is the acquisition route. */
static int artifact_lore_stage(struct char_data *ch, struct artifact_data *art)
{
  if (!art)
    return 0;

  if (ch && !IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT)
    return 2;

  if (!art->discovered)
    return 0;

  /* Once an artifact has been claimed and let go of at least once, how it is
   * found stops being a secret: somebody has done it and talked. */
  if (art->claim_count > 1 || art->transfer_count > 0)
    return 2;

  return 1;
}

/* The name the chronicle is willing to print. */
static const char *artifact_public_name(struct artifact_data *art)
{
  obj_rnum rnum = real_object(art->vnum);

  if (!art->discovered)
    return "\tD(a name not yet spoken aloud)\tn";

  if (rnum == NOTHING)
    return "an artifact whose shape is not recorded";

  return obj_proto[rnum].short_description;
}

/* The bearer line, obeying the artifact's own owner policy. */
static void artifact_show_bearer(struct char_data *ch, struct artifact_data *art, int state)
{
  int owned = artifact_is_owned(art->vnum);

  if (art->owner_policy == ART_OWNER_PUBLIC && owned && state == ART_STATE_HELD)
  {
    send_to_char(ch, "    Bearer: \tW%s\tn\r\n", art->owner);
    return;
  }

  if (art->first_claimed_at > 0 && art->first_owner &&
      str_cmp(art->first_owner, ARTIFACT_OWNER_NONE))
  {
    send_to_char(ch, "    First bearer: \tW%s\tn\r\n", art->first_owner);
    return;
  }

  if (owned)
    send_to_char(ch, "    Its bearer is not a matter of public record.\r\n");
}

static void artifact_show_roster(struct char_data *ch)
{
  struct artifact_data *art = NULL;
  int i = 0, shown = 0, state = 0, stage = 0;

  send_to_char(ch, "\tY========== The Artifact Chronicle ==========\tn\r\n");
  send_to_char(ch, "What is known of the artifacts of this world.\r\n");

  for (i = 0; i < total_artifacts; i++)
  {
    art = &art_index[i];

    if (!art->available)
      continue;

    state = artifact_state(art);
    stage = artifact_lore_stage(ch, art);

    send_to_char(ch, "\r\n  \tW%s\tn  \tc[%s]\tn\r\n", artifact_public_name(art),
                 artifact_state_name(state));

    if (art->lore && *art->lore)
      send_to_char(ch, "    %s\r\n", art->lore);

    artifact_show_bearer(ch, art, state);

    if (stage >= 2 && art->acq_hint && *art->acq_hint)
      send_to_char(ch, "    \tgHow it is found:\tn %s\r\n", art->acq_hint);
    else if (stage >= 1)
      send_to_char(ch, "    \tDHow it is found is not yet common knowledge.\tn\r\n");

    shown++;
  }

  if (!shown)
    send_to_char(ch, "\r\nNothing is recorded here.\r\n");

  send_to_char(ch, "\r\n\tcartifact chronicle <name>\tn tells what is known of one of them.\r\n");
  send_to_char(ch, "\tY===========================================\tn\r\n");
}

/* One artifact's chronicle entry: lore, acquisition, and custody history. */
static void artifact_show_chronicle(struct char_data *ch, const char *name)
{
  struct artifact_data *art = NULL, *match = NULL;
  char claimed_at[32] = {'\0'};
  obj_rnum rnum = NOTHING;
  int i = 0, state = 0, stage = 0;

  for (i = 0; i < total_artifacts && !match; i++)
  {
    art = &art_index[i];

    if (!art->available || !art->discovered)
      continue;

    if ((rnum = real_object(art->vnum)) == NOTHING)
      continue;

    if (isname(name, obj_proto[rnum].name) || is_abbrev(name, obj_proto[rnum].short_description))
      match = art;
  }

  if (!match)
  {
    send_to_char(ch, "The chronicle records nothing by that name.\r\n");
    return;
  }

  state = artifact_state(match);
  stage = artifact_lore_stage(ch, match);

  send_to_char(ch, "\tY===== %s \tY=====\tn\r\n", artifact_public_name(match));

  if (match->lore && *match->lore)
    send_to_char(ch, "%s\r\n", match->lore);

  send_to_char(ch, "\r\n\tcState:       \tW%s\tn\r\n", artifact_state_name(state));
  send_to_char(ch, "\tcComes from:  \tW%s\tn\r\n", artifact_acquisition_name(match->acquisition));

  if (stage >= 2 && match->acq_hint && *match->acq_hint)
    send_to_char(ch, "\tcHow:         \tW%s\tn\r\n", match->acq_hint);
  else
    send_to_char(ch, "\tcHow:         \tDnot yet common knowledge\tn\r\n");

  send_to_char(ch, "\r\n\tYCustody:\tn\r\n");

  artifact_show_bearer(ch, match, state);

  if (match->first_claimed_at > 0)
  {
    if (ctime_r(&match->first_claimed_at, claimed_at))
      send_to_char(ch, "    First claimed %s", claimed_at);
  }
  else
    send_to_char(ch, "    It has never been claimed.\r\n");

  if (match->last_claimed_at > 0)
  {
    if (ctime_r(&match->last_claimed_at, claimed_at))
      send_to_char(ch, "    Last claimed  %s", claimed_at);
  }

  send_to_char(ch, "    Claimed %d time%s, released %d time%s.\r\n", match->claim_count,
               match->claim_count == 1 ? "" : "s", match->transfer_count,
               match->transfer_count == 1 ? "" : "s");

  if (match->destroy_count > 0)
    send_to_char(ch, "    Destroyed and reforged %d time%s.\r\n", match->destroy_count,
                 match->destroy_count == 1 ? "" : "s");

  if (match->recovery_count > 0)
    send_to_char(ch, "    Recovered by the gods %d time%s.\r\n", match->recovery_count,
                 match->recovery_count == 1 ? "" : "s");

  send_to_char(ch, "\tY=========================================\tn\r\n");
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
  int i = 0, found = 0, remaining = 0, withheld = 0;

  send_to_char(ch, "\tY========== Artifact Abilities ==========\tn\r\n");

  for (i = 0; i < NUM_WEARS; i++)
  {
    if (!(obj = GET_EQ(ch, i)))
      continue;
    if (!(art = artifact_of_obj(obj)))
      continue;
    if (!art->ability_name)
      continue;
    if (!artifact_class_ok(ch, art))
    {
      withheld++;
      continue;
    }

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

  if (withheld > 0)
    send_to_char(ch, "  %d sworn artifact%s withhold%s %s named power%s from you.\r\n", withheld,
                 withheld == 1 ? "" : "s", withheld == 1 ? "s" : "",
                 withheld == 1 ? "its" : "their", withheld == 1 ? "" : "s");

  if (!found && !withheld)
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

  if (is_abbrev(arg1, "roster"))
  {
    artifact_show_roster(ch);
    return;
  }

  if (is_abbrev(arg1, "chronicle"))
  {
    if (!*arg2)
    {
      artifact_show_roster(ch);
      return;
    }

    artifact_show_chronicle(ch, arg2);
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

  send_to_char(ch, "Usage: artifact [list | roster | chronicle <name> | info <item> | progress | "
                   "abilities | help]\r\n");
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

  if (!artifact_class_ok(ch, art))
  {
    send_to_char(ch, "\tr%s rejects your command and withholds its power.\tn\r\n",
                 GET_OBJ_SHORT(obj));
    return FALSE;
  }

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

  /* Cooldowns are persisted from v2.3 onward.  The once-a-minute dirty flush
   * in the heartbeat picks this up; an ability cooldown is measured in
   * minutes, so a minute of drift is not worth a synchronous write. */
  artifact_mark_dirty();
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

  /* One award per use, however many it reached.  Multiplying by the target
   * count made a crowded room the fastest progression path in the system:
   * five targets every 180 seconds outpaced every called effect, which each
   * grant one flat award no matter how many they touch. */
  artifact_grant_xp_obj(ch, obj, ARTIFACT_XP_ABILITY_DOOMBLAST);
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
 * Player command: invoke
 *
 * The explicit invocation channel.  Nothing here knows any phrases: it hands
 * the whole argument to the same matcher say and whisper use, and the table
 * decides whether anything answers.
 * -------------------------------------------------------------------------- */

ACMD(do_artifact_invoke)
{
  struct obj_data *obj = NULL;
  int i = 0, found = 0;

  if (!ch || IS_NPC(ch))
    return;

  skip_spaces_c(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Usage: invoke <word>\r\n\r\n");
    send_to_char(ch, "Words the artifacts you are carrying will answer to:\r\n");

    for (i = 0; artifact_effects[i].vnum != -1; i++)
    {
      if (artifact_effects[i].channel != ART_INVOKE_COMMAND || artifact_effect_is_disabled(i))
        continue;

      if (!(obj = artifact_held_instance(ch, artifact_effects[i].vnum)))
        continue;

      send_to_char(ch, "  \tcinvoke %s%s\tn - %s\r\n", artifact_effects[i].phrase,
                   artifact_target_takes_argument(artifact_effects[i].target_type) ? " <target>"
                                                                                   : "",
                   artifact_effects[i].desc);
      found++;
    }

    if (!found)
      send_to_char(ch, "  Nothing you are carrying answers to a spoken command.\r\n");

    return;
  }

  if (!artifact_command_trigger(ch, argument))
    send_to_char(ch, "Nothing you are carrying answers to that.\r\n");
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
    send_to_char(ch, "Usage: testartifact <status|verify|save|reload|spawn|recover|list|reset>\r\n"
                     "  status         - artifact system status\r\n"
                     "  verify         - check duplicates, records, and table metadata\r\n"
                     "  save           - write the artifact file now\r\n"
                     "  reload         - flush dirty state, then rebuild the registry\r\n"
                     "  reload discard - rebuild without saving; deferred state is lost\r\n"
                     "  spawn <vnum>   - create an unowned artifact here\r\n"
                     "  recover <vnum> - audited recovery of a lost or offline-owned artifact\r\n"
                     "  list           - every artifact, owner, state, and location\r\n"
                     "  reset <vnum>   - clear ownership and binding on one artifact\r\n");
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

    /* Re-run the boot-time table checks.  Details go to the log, where they
     * name the offending row precisely; the staff member gets the count. */
    count = artifact_validate_metadata();

    if (count > 0)
    {
      send_to_char(ch, "\tr%d metadata problem%s in the artifact tables; see the log.\tn\r\n",
                   count, count == 1 ? "" : "s");
      ok = FALSE;
    }
    else
      send_to_char(ch, "\tgTable metadata validated.\tn\r\n");

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
    /* Rebuilding the registry throws away everything held only in memory -
     * sub-threshold XP, cooldown stamps, and any ownership change that has
     * not reached the file yet.  Flush first unless explicitly told not to. */
    if (!str_cmp(arg2, "discard"))
    {
      mudlog(BRF, LVL_STAFF, TRUE, "ARTIFACT: %s reloaded the registry, discarding dirty state",
             GET_NAME(ch));
      send_to_char(ch, "\trDiscarding unsaved artifact state.\tn\r\n");
    }
    else
    {
      artifact_save_if_dirty();
      send_to_char(ch, "Unsaved artifact state flushed to %s.\r\n", ARTIFACT_FILE);
    }

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

    /* An artifact recorded as owned may simply be in an offline player's
     * save.  Spawning a second one would put two in the world and, worse,
     * clear instance_persisted on the way past, so the next zone reset would
     * happily make a third.  Recovery is the audited path for this. */
    if (artifact_is_owned(vnum))
    {
      art = artifact_by_vnum(vnum);
      send_to_char(ch, "\trArtifact %d is on record as belonging to %s.\tn\r\n", vnum,
                   art->owner ? art->owner : "someone");
      send_to_char(ch,
                   "Ordinary spawning would create a duplicate and weaken the reset guard.\r\n");
      send_to_char(ch, "Use \tctestartifact recover %d\tn if it is genuinely lost.\r\n", vnum);
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

  /* An audited recovery: the one sanctioned way to put a lost artifact back
   * into play without weakening the uniqueness guarantee.  It refuses while a
   * live instance exists, states plainly what it is overriding, records the
   * recovery in the artifact's own history, and logs it. */
  if (is_abbrev(arg, "recover"))
  {
    vnum = atoi(arg2);

    if (!(art = artifact_by_vnum(vnum)))
    {
      send_to_char(ch, "Usage: testartifact recover <artifact vnum>\r\n");
      return;
    }

    for (obj = object_list; obj; obj = obj->next)
      if ((int)GET_OBJ_VNUM(obj) == vnum)
      {
        send_to_char(ch,
                     "\trRefused: a live instance of artifact %d is already in play (%s).\tn\r\n",
                     vnum, artifact_locate(art));
        send_to_char(ch, "Recovery is for artifacts that are lost, not for artifacts you can "
                         "reach.\r\n");
        return;
      }

    if (!artifact_is_owned(vnum))
    {
      send_to_char(ch, "\trRefused: artifact %d is unowned; there is nothing to recover.\tn\r\n",
                   vnum);
      send_to_char(ch, "Use \tctestartifact spawn %d\tn instead.\r\n", vnum);
      return;
    }

    send_to_char(ch, "\tYRecovering artifact %d.\tn\r\n", vnum);
    send_to_char(ch, "  Overriding ownership by \tW%s\tn", art->owner ? art->owner : "someone");
    if (art->account && str_cmp(art->account, ARTIFACT_OWNER_NONE))
      send_to_char(ch, " (account %s)", art->account);
    send_to_char(ch, ".\r\n");
    send_to_char(ch, "  Clearing binding and the persisted-instance flag.\r\n");
    send_to_char(ch, "  Custody history is kept; this is recorded as a recovery.\r\n");

    if (!(obj = read_object(vnum, VIRTUAL)))
    {
      send_to_char(ch, "\trFailed to create the replacement instance; nothing was changed.\tn\r\n");
      return;
    }

    /* Provenance survives a recovery.  Only current ownership is cleared. */
    artifact_set_owner(art, NULL);
    art->bound_time = 0;
    art->ch = NULL;
    art->instance_persisted = FALSE;
    art->recovery_count++;
    artifact_mark_dirty();
    artifact_save();

    obj_to_room(obj, IN_ROOM(ch));

    mudlog(BRF, LVL_STAFF, TRUE,
           "ARTIFACT: %s RECOVERED artifact %d (%s) - previous owner released", GET_NAME(ch), vnum,
           GET_OBJ_SHORT(obj));

    act("$n calls something back out of the world that had been lost.", FALSE, ch, obj, NULL,
        TO_ROOM);
    send_to_char(ch, "\tgRecovered %s.\tn\r\n", GET_OBJ_SHORT(obj));
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
    art->override_count++;
    artifact_mark_dirty();
    artifact_save();

    mudlog(BRF, LVL_STAFF, TRUE, "ARTIFACT: %s reset ownership of artifact %d", GET_NAME(ch), vnum);
    send_to_char(ch, "Artifact %d is unowned and unbound again.\r\n", vnum);
    return;
  }

  if (is_abbrev(arg, "list"))
  {
    send_to_char(ch, "\tY Vnum  Lv  Owner                State        Acquisition           "
                     "Location\tn\r\n");
    send_to_char(ch, "------  --  -------------------  -----------  --------------------  "
                     "------------\r\n");

    for (i = 0; i < total_artifacts; i++)
      send_to_char(
          ch, "%6d  %2d  %-19s  %-11s  %-20s  %s%s\r\n", art_index[i].vnum, art_index[i].level,
          artifact_is_owned(art_index[i].vnum) ? art_index[i].owner : "-",
          artifact_state_name(artifact_state(&art_index[i])),
          artifact_acquisition_name(art_index[i].acquisition), artifact_locate(&art_index[i]),
          art_index[i].available ? "" : " \tD(not in this campaign)\tn");

    send_to_char(ch, "\r\nBinding and custody detail: \tcartifact chronicle <name>\tn\r\n");
    return;
  }

  send_to_char(ch, "Unknown subcommand. Type 'testartifact' for usage.\r\n");
}

/* --------------------------------------------------------------------------
 * Test seams
 *
 * The integration suite drives the real display and proc paths rather than
 * re-deriving what they should print.  These exist only in the test build.
 * -------------------------------------------------------------------------- */
#ifdef LUMINARI_CUTEST
void artifact_show_info_for_test(struct char_data *ch, struct obj_data *obj)
{
  artifact_show_info(ch, obj);
}

/* Select one generic branch without its outer random roll.  Branch eligibility,
 * cooldown stamping, output, XP, and effects remain production behavior. */
int artifact_force_generic_proc_for_test(struct char_data *ch, struct char_data *victim,
                                         struct obj_data *weapon, int proc_type)
{
  struct artifact_data *art = NULL;

  if (!ch || !victim || !weapon)
    return FALSE;

  if (!(art = artifact_of_obj(weapon)))
    return FALSE;

  return artifact_generic_proc(ch, victim, weapon, art, proc_type);
}

/* Drive the reusable signature dispatcher with an exact percentage roll.
 * Eligibility, cooldown, output, XP, and the selected shape remain production
 * behavior. */
int artifact_force_signature_roll_for_test(struct char_data *ch, struct char_data *victim,
                                           struct obj_data *weapon, int is_critical,
                                           int chance_roll)
{
  struct artifact_data *art = NULL;

  if (!ch || !victim || !weapon || chance_roll < 1 || chance_roll > 100)
    return FALSE;

  if (!(art = artifact_of_obj(weapon)))
    return FALSE;

  return artifact_reusable_proc_with_roll(ch, victim, weapon, art, is_critical, chance_roll);
}

/* Chance rolls make procs untestable as written.  A test forces the shape it
 * wants by name, skipping only the roll: every other gate - the internal
 * cooldown, the alignment rule, target legality - still applies. */
int artifact_force_signature_proc_for_test(struct char_data *ch, struct char_data *victim,
                                           struct obj_data *weapon, int is_critical)
{
  struct artifact_data *art = NULL;

  artifact_test_doombringer_attacks = 0;

  if (!ch || !victim || !weapon)
    return FALSE;

  if (!(art = artifact_of_obj(weapon)))
    return FALSE;

  return artifact_signature_proc(ch, victim, weapon, art, 10, is_critical, TRUE);
}

int artifact_doombringer_attacks_for_test(void)
{
  return artifact_test_doombringer_attacks;
}

int artifact_force_doombringer_nested_proc_for_test(struct char_data *ch, struct char_data *victim,
                                                    struct obj_data *weapon)
{
  int victim_died = FALSE;

  artifact_in_doombringer_burst = TRUE;
  victim_died = artifact_weapon_proc(ch, victim, weapon, 10, FALSE);
  artifact_in_doombringer_burst = FALSE;

  return victim_died;
}

void artifact_force_avernus_survival_for_test(struct char_data *ch, struct char_data *victim,
                                              struct obj_data *weapon, int emergency_heal,
                                              int bladesong_heal)
{
  struct artifact_data *art = NULL;

  if (!ch || !victim || !weapon)
    return;

  if (!(art = artifact_of_obj(weapon)) || art->vnum != ART_VNUM_AVERNUS)
    return;

  if (emergency_heal)
    artifact_avernus_emergency_heal(ch, weapon);
  if (bladesong_heal)
    artifact_avernus_bladesong(ch, victim, weapon);
  artifact_avernus_recover(ch, victim, weapon);
}

/* Read the exact production tables and dispatch lookup into one stable test
 * record.  A zero effect or a NOTHING hand procedure means an explicit none. */
int artifact_identity_for_test(int vnum, struct artifact_test_identity_data *identity)
{
  const struct artifact_effect *effect = NULL;
  const struct artifact_hand_proc_entry *hand_proc = NULL;
  struct artifact_data *art = NULL;
  int i = 0, passive_count = 0;

  if (!identity)
    return FALSE;

  memset(identity, 0, sizeof(*identity));
  identity->hand_proc_vnum = NOTHING;
  for (i = 0; i < ARTIFACT_MAX_EFFECTS; i++)
    identity->called_channels[i] = NOTHING;

  if (!(art = artifact_by_vnum(vnum)))
    return FALSE;

  identity->ability_name = art->ability_name;
  identity->generic_proc_chance = art->proc_chance;
  identity->signature_proc = art->sig_proc;
  hand_proc = artifact_hand_proc_for_vnum(vnum);
  if (hand_proc)
  {
    identity->hand_proc_vnum = hand_proc->vnum;
    identity->hand_entry_odds = hand_proc->proc_odds;
  }

  for (i = 0; i < ARTIFACT_MAX_EFFECTS; i++)
  {
    if (!(effect = artifact_effect_at(vnum, i)))
      continue;
    identity->called_effects[i] = effect->effect;
    identity->called_channels[i] = effect->channel;
    identity->called_stack_groups[i] = effect->stack_group;
  }

  for (i = 0; artifact_passives[i].vnum != -1; i++)
  {
    if (artifact_passives[i].vnum != vnum)
      continue;

    if (passive_count < ARTIFACT_TEST_MAX_PASSIVES)
    {
      identity->passives[passive_count].min_level = artifact_passives[i].min_level;
      identity->passives[passive_count].aff_flag = artifact_passives[i].aff_flag;
      identity->passives[passive_count].location = artifact_passives[i].location;
      identity->passives[passive_count].modifier = artifact_passives[i].modifier;
    }
    passive_count++;
  }

  identity->passive_count = passive_count;
  return TRUE;
}
#endif

/*EOF*/
