/* ***************************************************************************
 *  File: spec_artifacts.h                            Part of LuminariMUD
 *  Usage: Artifact system - unique, single-instance items of power that
 *         track persistent ownership, gain levels through use, bind to their
 *         owners, and carry procs and active abilities.
 *
 *  Ported and modernized from the RealmsOfLuminari artifact system.  Current
 *  behavior, integration, and deliberate deviations are documented in
 *    docs/systems/ARTIFACT_SYSTEM.md
 *************************************************************************** */

#ifndef _SPEC_ARTIFACTS_H_
#define _SPEC_ARTIFACTS_H_

/* --------------------------------------------------------------------------
 * VNUM allocation - zone 1699, range 169900-169999.
 *
 * src/vnums.h is gitignored local configuration and must not be edited, so
 * the artifact vnums live here.  Never hardcode these numbers elsewhere.
 * -------------------------------------------------------------------------- */
#define ARTIFACT_ZONE 1699
#define ARTIFACT_VNUM_BASE 169900

#define ART_VNUM_VAULT 169900 /* the vault room                       */
#define ART_VNUM_TRORXEK 169901
#define ART_VNUM_AMAUKEKEL 169902
#define ART_VNUM_FADE 169903
#define ART_VNUM_HENEKAR 169904
#define ART_VNUM_DOOMBRINGER 169905
#define ART_VNUM_KELRARIN 169906
#define ART_VNUM_KELROM 169907
#define ART_VNUM_GESEN 169908
#define ART_VNUM_STINGER 169909
#define ART_VNUM_AVERNUS 169910
#define ART_VNUM_AEGIS 169911

/* Not an artifact - the treant Trorxek calls.  Shipped in the same zone so
 * the effect does not depend on any campaign's world content. */
#define ART_VNUM_OAKEN_DEFENDER 169912

/* Second wave: the six complete HomelandMUD candidates, rebuilt on this
 * system rather than ported.  See docs/systems/ARTIFACT_SYSTEM.md. */
#define ART_VNUM_VENGEANCE 169913
#define ART_VNUM_EARTHCRIER 169914
#define ART_VNUM_WYRMFANG 169915
#define ART_VNUM_COURAGE 169916
#define ART_VNUM_ICEDGE 169917
#define ART_VNUM_TWILIGHT 169918

/* --------------------------------------------------------------------------
 * Data file
 * -------------------------------------------------------------------------- */
#define ARTIFACT_FILE LIB_WORLD "world.artifact"

/* Sentinel owner names.  Anything in this set means "unowned". */
#define ARTIFACT_OWNER_NONE "noone"
#define ARTIFACT_OWNER_INIT "nobody"

/* --------------------------------------------------------------------------
 * Progression
 * -------------------------------------------------------------------------- */
#define ARTIFACT_MAX_LEVEL 5
#define ARTIFACT_NUM_STATS 6

/* stat_bonus[] index order */
#define ART_STAT_STR 0
#define ART_STAT_INT 1
#define ART_STAT_WIS 2
#define ART_STAT_DEX 3
#define ART_STAT_CON 4
#define ART_STAT_CHA 5

/* XP awards */
#define ARTIFACT_XP_FIRST_EQUIP 10
#define ARTIFACT_XP_HIT 1
#define ARTIFACT_XP_CRIT 3
#define ARTIFACT_XP_KILL 10
#define ARTIFACT_XP_BOSS_HIT_MULT 2
#define ARTIFACT_XP_BOSS_KILL_MULT 3

/* LuminariMUD has no ACT_BOSS flag.  An NPC this many levels above its killer
 * is treated as the boss case the multipliers above were written for. */
#define ARTIFACT_BOSS_LEVEL_MARGIN 3
#define ARTIFACT_XP_PROC_SOUL 2
#define ARTIFACT_XP_PROC_HEAL 1
#define ARTIFACT_XP_PROC_FEAR 3
#define ARTIFACT_XP_PROC_DOOM 4
#define ARTIFACT_XP_PROC_ULTIMATE 10
#define ARTIFACT_XP_ABILITY_SOULSTRIKE 15
#define ARTIFACT_XP_ABILITY_DIVINEWARD 20
#define ARTIFACT_XP_ABILITY_DOOMBLAST 10

/* A called effect recharges in hours or days, not seconds, so one is worth
 * considerably more than an ability press. */
#define ARTIFACT_XP_CALLED_EFFECT 25

/* Percent chance an XP grant also prints a progress line. */
#define ARTIFACT_XP_NOTIFY_CHANCE 5

/* --------------------------------------------------------------------------
 * Binding
 * -------------------------------------------------------------------------- */
#define ARTIFACT_BIND_NONE 0       /* freely traded                        */
#define ARTIFACT_BIND_ON_PICKUP 1  /* soulbound on acquisition             */
#define ARTIFACT_BIND_ON_EQUIP 2   /* binds the first time it is worn      */
#define ARTIFACT_BIND_ON_ACCOUNT 3 /* any character on the owner's account */
#define NUM_ARTIFACT_BINDINGS 4

/* --------------------------------------------------------------------------
 * Abilities and procs
 * -------------------------------------------------------------------------- */
#define ARTIFACT_DEFAULT_COOLDOWN 300

/* Weapon proc kinds.  The kind rolled is rand_number(1, level), so a higher
 * level artifact can roll anything up to its own level. */
#define ARTIFACT_PROC_SOUL 1     /* dice(level, 6) negative damage        */
#define ARTIFACT_PROC_HEAL 2     /* dice(level, 4) self heal              */
#define ARTIFACT_PROC_FEAR 3     /* AFF_FEAR, 1 + level/2 rounds          */
#define ARTIFACT_PROC_DOOM 4     /* dice(level, 8) damage, level 4+       */
#define ARTIFACT_PROC_ULTIMATE 5 /* level 5 only, 5% inner roll, execute  */

/* Seconds of internal cooldown between weapon procs on one artifact. */
#define ARTIFACT_PROC_ICD 30

/* Doom Blast target cap. */
#define ARTIFACT_DOOMBLAST_MAX_TARGETS 5

/* Annihilation's target cap, one plus the artifact's level, so a young blade
 * clears a skirmish and a grown one clears a room.  Before the balance pass
 * this effect had no cap at all and its total output scaled with however many
 * hostiles happened to be standing there. */
#define ARTIFACT_ANNIHILATION_TARGETS_PER_LEVEL 1
#define ARTIFACT_ANNIHILATION_TARGETS_BASE 1

/* --------------------------------------------------------------------------
 * Class restriction and the burn penalty
 *
 * ROL gated an artifact to a single class and scorched anyone who failed the
 * check.  LuminariMUD is multi-class, so the gate is a minimum number of
 * levels in the named class rather than "is that your class".
 * -------------------------------------------------------------------------- */
/* The burn scales with whoever it is rejecting.  A flat 5d4 is 5 to 20 points,
 * which a level-30 character does not notice; the percentage is what makes the
 * refusal mean something at every tier, and the dice are its floor. */
#define ARTIFACT_BURN_DICE 5
#define ARTIFACT_BURN_SIDES 4
#define ARTIFACT_BURN_PERCENT 3 /* percent of max HP, floored by the dice */

/* --------------------------------------------------------------------------
 * Called effects - the per-artifact special procedures
 *
 * Each artifact owns up to ARTIFACT_MAX_EFFECTS effects, invoked by saying a
 * phrase aloud.  Each effect recharges independently.  Recharge stamps live
 * in memory only: ROL drove them off its event queue, which likewise did not
 * survive a reboot.
 * -------------------------------------------------------------------------- */
#define ARTIFACT_MAX_EFFECTS 4

#define ARTIFACT_RECHARGE_HOUR 3600
#define ARTIFACT_RECHARGE_6HOUR 21600
#define ARTIFACT_RECHARGE_12HOUR 43200
#define ARTIFACT_RECHARGE_DAY 86400
#define ARTIFACT_RECHARGE_WEEK 604800

/* How an effect finds what it acts on. */
#define ART_TARGET_NONE 0       /* no target                              */
#define ART_TARGET_CHAR_ROOM 1  /* a character in the room                */
#define ART_TARGET_CHAR_WORLD 2 /* a player anywhere                      */
#define ART_TARGET_OBJ_ROOM 3   /* an object in the room                  */
#define ART_TARGET_FIGHTING 4   /* whoever the invoker is fighting        */
#define ART_TARGET_GROUP_ROOM 5 /* the invoker's group, same room only    */
#define NUM_ART_TARGETS 6

/* --------------------------------------------------------------------------
 * Invocation channels
 *
 * An effect's phrase, its displayed help, and the channel it answers on all
 * come from one row of artifact_effects[].  Adding a channel to an artifact is
 * a data change, not a second dispatcher.
 * -------------------------------------------------------------------------- */
#define ART_INVOKE_SAY 0     /* spoken aloud - say <phrase>              */
#define ART_INVOKE_WHISPER 1 /* whisper <someone> <phrase>               */
#define ART_INVOKE_COMMAND 2 /* invoke <phrase>                          */
#define NUM_ART_INVOKE 3

/* The effects themselves. */
#define ART_EFFECT_SUMMON_TREANT 1    /* Trorxek:    come oaken defender   */
#define ART_EFFECT_CREEPING_DOOM 2    /* Trorxek:    carpet of death       */
#define ART_EFFECT_RECALL 3           /* Trorxek:    forest path home      */
#define ART_EFFECT_TRAVEL_TO 4        /* Trorxek/Fade/Henekar: path to <t> */
#define ART_EFFECT_DIMENSION_SHIFT 5  /* Amaukekel:  sunlit path           */
#define ART_EFFECT_RESURRECT 6        /* Amaukekel:  give life to <corpse> */
#define ART_EFFECT_DISPEL_EVIL 7      /* Amaukekel:  wrath of light        */
#define ART_EFFECT_BLIND 8            /* Fade/Henekar: blinding word       */
#define ART_EFFECT_DARKNESS 9         /* Fade:       darken the world      */
#define ART_EFFECT_WEAKEN 10          /* Fade:       devour the soul       */
#define ART_EFFECT_PACIFY 11          /* Henekar:    peace to you          */
#define ART_EFFECT_CHARM 12           /* Henekar:    join my quest         */
#define ART_EFFECT_ANNIHILATION 13    /* Doombringer: bring annhilation    */
#define ART_EFFECT_BLACK_LIGHTNING 14 /* Doombringer: feel my power        */
#define ART_EFFECT_ENRAGE 15          /* Doombringer: enrage me            */
#define ART_EFFECT_GROUP_VALOR 16     /* Courage:    morale and vitality   */
#define ART_EFFECT_FROST_WARD 17      /* Icedge:     rime shield           */
#define ART_EFFECT_DRAGON_SIGHT 18    /* Wyrmfang:   the hunter's sight    */
#define NUM_ART_EFFECTS 19

/* Tunables for the called effects. */
/* ROL's flat 2000 is the ceiling, not the entry price: a level-1 horn calls
 * lesser creatures, and only a grown one can call something that large.  The
 * contract line is "recruits the lesser creatures nearby", and a 2000-max-HP
 * mobile is a mini-boss in most content. */
#define ARTIFACT_CHARM_MAX_HP 2000 /* ROL's cap, reached at artifact level 5 */
#define ARTIFACT_CHARM_MAX 3       /* how many may answer at once         */
#define ARTIFACT_ENRAGE_DURATION 10

/* Courage's group invocation.  One cooldown and one XP award per activation,
 * however many answer to it. */
#define ARTIFACT_VALOR_DURATION 15
#define ARTIFACT_VALOR_HITROLL 2 /* + artifact level                     */
#define ARTIFACT_VALOR_SAVES 1   /* + artifact level / 2                 */
#define ARTIFACT_VALOR_HP_PER_LEVEL 8
#define ARTIFACT_VALOR_MAX_TARGETS 12

/* --------------------------------------------------------------------------
 * Chronicle - the public artifact roster
 *
 * The state shown to players is derived from the registry every time it is
 * asked for.  There is deliberately no second list to drift out of step.
 * -------------------------------------------------------------------------- */
#define ART_STATE_UNAWAKENED 0  /* nobody has ever claimed it             */
#define ART_STATE_UNCLAIMED 1   /* discovered, currently free             */
#define ART_STATE_HELD 2        /* owned, and a live instance is carried  */
#define ART_STATE_LOST 3        /* owned, but the bearer is not in play   */
#define ART_STATE_RECOVERABLE 4 /* owned on record, no instance anywhere  */
#define NUM_ART_STATES 5

/* Whether the roster names the current bearer. */
#define ART_OWNER_SECRET 0 /* never named; "last known bearer" only       */
#define ART_OWNER_PUBLIC 1 /* named once the artifact is discovered       */
#define NUM_ART_OWNER_POLICY 2

/* --------------------------------------------------------------------------
 * Acquisition and release policy
 *
 * This is the content contract for an artifact: where it comes from, which
 * campaigns it exists in, and how it re-enters play once lost.  It drives the
 * roster, the help text, and boot validation - not zone resets.
 * -------------------------------------------------------------------------- */
#define ART_ACQ_UNSET 0
#define ART_ACQ_BOSS 1        /* carried by a named world boss            */
#define ART_ACQ_QUEST 2       /* awarded by a quest                       */
#define ART_ACQ_EXPLORATION 3 /* found at the end of an exploration chain */
#define ART_ACQ_SEASONAL 4    /* released in a recurring seasonal window  */
#define ART_ACQ_STAFF_EVENT 5 /* released by hand at a staff-run event    */
#define ART_ACQ_RECOVERY 6    /* only ever re-enters through recovery     */
#define ART_ACQ_VAULT 7       /* staged in the vault, not yet placed      */
#define NUM_ART_ACQ 8

/* Campaign availability.  A bitmask so one artifact may exist in several. */
#define ART_CAMPAIGN_LUMINARI (1 << 0)
#define ART_CAMPAIGN_DL (1 << 1)
#define ART_CAMPAIGN_FR (1 << 2)
#define ART_CAMPAIGN_ALL (ART_CAMPAIGN_LUMINARI | ART_CAMPAIGN_DL | ART_CAMPAIGN_FR)

/* --------------------------------------------------------------------------
 * Reusable signature-proc shapes
 *
 * Seven inherited procedures remain hand-written.  Table-driven signature
 * powers select a shape, chance, and alignment condition here so the same
 * behavior can be reused without another vnum-specific dispatch function.
 * -------------------------------------------------------------------------- */
#define ART_SIG_NONE 0
#define ART_SIG_KNOCKDOWN 1 /* save-or-fall, with immunity rules         */
#define ART_SIG_MERCY 2     /* heal while wounded, strike while healthy  */
#define ART_SIG_WARD 3      /* alignment-conditioned protection/dispel   */
#define ART_SIG_WEIGHTED 4  /* one of several weighted outcomes          */
#define ART_SIG_SURGE 5     /* bounded temporary combat surge            */
#define ART_SIG_FLURRY 6    /* bounded burst of extra attacks            */
#define ART_SIG_LIFESTEAL 7 /* damage returned as healing                */
#define NUM_ART_SIG 8

/* When a reusable proc is allowed to fire at all. */
#define ART_ALIGN_ANY 0         /* no condition                          */
#define ART_ALIGN_TARGET_EVIL 1 /* only against evil                     */
#define ART_ALIGN_TARGET_GOOD 2 /* only against good                     */
#define ART_ALIGN_SELF_GOOD 3   /* only for a good wielder               */
#define ART_ALIGN_SELF_EVIL 4   /* only for a non-good wielder           */
#define NUM_ART_ALIGN 5

/* --------------------------------------------------------------------------
 * Stacking groups
 *
 * Two temporary artifact powers in the same group never stack.  The first one
 * holds until it expires; the second refuses and costs nothing.  Every
 * temporary affect an artifact creates is stamped with its group so it can be
 * found again without guessing at spell numbers.
 * -------------------------------------------------------------------------- */
#define ART_STACK_NONE 0
#define ART_STACK_COMBAT_SURGE 1 /* Twilight's surge, Doombringer's rage  */
#define ART_STACK_MORALE 2       /* Courage's valor                       */
#define ART_STACK_WARD 3         /* Vengeance's ward, Icedge's rime       */
#define NUM_ART_STACK 4

/* Twilight's surge: bounded, level-scaled, and never derived from totals the
 * wielder has already earned. */
#define ARTIFACT_SURGE_DURATION 4
#define ARTIFACT_SURGE_HITROLL 2 /* multiplied by artifact level          */
#define ARTIFACT_SURGE_DAMROLL 3 /* multiplied by artifact level          */

/* Earthcrier's knockdown. */
#define ARTIFACT_KNOCKDOWN_DC 14 /* + artifact level                      */
#define ARTIFACT_KNOCKDOWN_WAIT 1

/* Icedge's flurry. */
#define ARTIFACT_FLURRY_MIN 2
#define ARTIFACT_FLURRY_MAX 4

/* Vengeance's mercy branch. */
#define ARTIFACT_MERCY_WOUNDED_PERCENT 60 /* below this, it heals instead  */
#define ARTIFACT_MERCY_HEAL_BASE 40
#define ARTIFACT_MERCY_HEAL_DICE 4
#define ARTIFACT_MERCY_HEAL_SIDES 15

/* Tiamat's Stinger.  ROL's procedure directly moved up to 200 hit points on a
 * 1-in-21 per-hit roll, bypassing mitigation and maximum-HP limits.  This
 * version keeps the independent per-hit behavior but uses 10% to avoid common
 * forty-plus-hit droughts, scales with the artifact and wielder, then heals
 * only the damage the normal combat pipeline actually applied. */
#define ARTIFACT_STINGER_LIFESTEAL_CHANCE 10
#define ARTIFACT_STINGER_LIFESTEAL_GUARANTEE 15
#define ARTIFACT_STINGER_LIFESTEAL_DICE_BASE 2
#define ARTIFACT_STINGER_LIFESTEAL_DIE_SIZE 10
#define ARTIFACT_STINGER_LIFESTEAL_BONUS_PER_LEVEL 10

/* XP for the reusable shapes.  One award per successful proc. */
#define ARTIFACT_XP_PROC_SIGNATURE 3

/* --------------------------------------------------------------------------
 * Progressive passive powers
 *
 * Status grants live in artifact_passives[], not in object prototype affect
 * bits, so one power has exactly one source of truth and can unlock as the
 * artifact grows.  Applied as source-tagged affects on equip, refreshed on
 * level-up, and removed cleanly on unequip.
 * -------------------------------------------------------------------------- */
#define ART_PASSIVE_MAX_PER_ARTIFACT 6

/* --------------------------------------------------------------------------
 * Signature weapon-hit procedures
 *
 * Independent of the generic proc system: these roll on every successful hit
 * and ignore the shared internal cooldown, exactly as ROL's did.
 * -------------------------------------------------------------------------- */
#define ARTIFACT_FADE_DRAIN_ODDS 16
#define ARTIFACT_FADE_DRAIN_MAX_DAMAGE 200
#define ARTIFACT_FADE_DRAIN_HEAL_PERCENT 25
#define ARTIFACT_DOOMBRINGER_BURST_ODDS 31
#define ARTIFACT_DOOMBRINGER_BURST_MAX_ATTACKS 5
#define ARTIFACT_DOOMBRINGER_BURST_COOLDOWN (SECS_PER_MUD_HOUR / 3)
#define ARTIFACT_DOOMBRINGER_ALIGNMENT_COST 1
#define ARTIFACT_KELRARIN_THROW_ODDS 29
#define ARTIFACT_KELRARIN_THROW_MAX 250
#define ARTIFACT_KELRARIN_MEGA_ODDS 33
#define ARTIFACT_KELRARIN_MEGA_DAMAGE 350
#define ARTIFACT_KELRARIN_MEGA_ALIGN 990
#define ARTIFACT_GESEN_THROW_ODDS 31
#define ARTIFACT_AVERNUS_HEAL_THRESHOLD 100
#define ARTIFACT_AVERNUS_HEAL_CHANCE 30
#define ARTIFACT_AVERNUS_HEAL_CHANCE_PER_LEVEL 2
#define ARTIFACT_AVERNUS_BLADESONG_ODDS 11
#define ARTIFACT_AVERNUS_BLADESONG_HEAL 2
#define ARTIFACT_AVERNUS_BLADESONG_MIN_MISSING 10
#define ARTIFACT_AVERNUS_DRAIN_ODDS 31
#define ARTIFACT_AVERNUS_DRAIN_MAX_TRANSFER 250
#define ARTIFACT_AVERNUS_DRAIN_DEATH_MARGIN 9
#define ARTIFACT_AVERNUS_DRAIN_DAMAGE_MULTIPLIER 3
/* Kelrom's healback.  The bearer gets the full share; everyone else in the
 * group gets half of it.  Before the balance pass this fired on every single
 * hit with no cooldown and gave the whole group the full amount, which made a
 * level-5 Kelrom a party-wide 50%-of-damage lifesteal on every swing. */
#define ARTIFACT_KELROM_HEALBACK_PERCENT 10
#define ARTIFACT_KELROM_GROUP_SHARE 50 /* percent of the bearer's share    */
#define ARTIFACT_KELROM_HEALBACK_COOLDOWN ARTIFACT_PROC_ICD

/* Kelrarin's mega blast scales like its own lesser throw does; a level-1
 * hammer should not carry a level-5 nuke. */
#define ARTIFACT_KELRARIN_MEGA_MIN 100

/* --------------------------------------------------------------------------
 * Data model
 * -------------------------------------------------------------------------- */
struct artifact_data
{
  /* Identity and ownership */
  char *owner;          /* owner name, or a sentinel                 */
  char *account;        /* owning account name, for BIND_ON_ACCOUNT  */
  int vnum;             /* object vnum                               */
  struct char_data *ch; /* last holder, NULL when released           */

  /* Progression */
  int level;                  /* 1 .. ARTIFACT_MAX_LEVEL                */
  int experience;             /* running XP total                       */
  int binding_type;           /* ARTIFACT_BIND_*                        */
  time_t bound_time;          /* when binding occurred, 0 = unbound     */
  int instance_persisted;     /* instance is in a player/house save     */
  time_t last_ability_use;    /* ability cooldown stamp                 */
  time_t last_proc;           /* weapon proc internal cooldown stamp    */
  time_t last_signature_proc; /* independent signature cooldown stamp */

  /* Called-effect recharge stamps, one per effect slot.  Persisted from v2.3
   * onward: a server that reboots often must not hand every power back. */
  time_t effect_used[ARTIFACT_MAX_EFFECTS];

  /* Provenance and custody.  Kept strictly separate from current ownership:
   * `owner` answers "who holds it now", these answer "what has happened to
   * it".  Nothing here is ever consulted by a binding or uniqueness check. */
  char *first_owner;       /* first character ever to claim it          */
  char *first_account;     /* that character's account                 */
  time_t first_claimed_at; /* when it was first claimed, 0 = never      */
  time_t last_claimed_at;  /* when it last changed hands                */
  int claim_count;         /* claims by a new owner                     */
  int transfer_count;      /* releases into the world                   */
  int destroy_count;       /* live instances destroyed                  */
  int recovery_count;      /* audited staff recoveries                  */
  int override_count;      /* audited staff resets and overrides        */
  int discovered;          /* TRUE once anyone has ever claimed it      */
  time_t discovered_at;    /* when that happened                        */

  /* Class restriction.  CLASS_UNDEFINED means anyone may wield it. */
  int class_restrict;
  int class_min_level;

  /* Stat modifiers - each is multiplied by `level` when applied */
  int stat_bonus[ARTIFACT_NUM_STATS];
  int hitroll_bonus;
  int damroll_bonus;
  int ac_bonus; /* applied as a negative APPLY_AC modifier           */
  int hp_bonus;
  int psp_bonus;
  int move_bonus;

  /* Damage resistance, percent.  Highest applicable wins; no stacking. */
  int resist_physical;
  int resist_magical;
  int resist_element;

  /* Active ability */
  const char *ability_name; /* also the command that invokes it      */
  const char *ability_desc;
  int ability_cooldown; /* seconds                                   */
  int ability_cost;     /* psp                                       */

  /* Weapon proc chance, percent per successful hit.  0 disables. */
  int proc_chance;

  /* Reusable signature proc - a shape from the library, not new code. */
  int sig_proc;        /* ART_SIG_*                                  */
  int sig_chance;      /* percent per successful hit                 */
  int sig_align;       /* ART_ALIGN_*                                */
  int sig_miss_streak; /* runtime-only bad-luck protection      */

  /* Content contract - see the template table.  Copied in at boot so the
   * roster and validator read one place. */
  int acquisition;      /* ART_ACQ_*                                  */
  int campaigns;        /* ART_CAMPAIGN_* bitmask                     */
  int owner_policy;     /* ART_OWNER_SECRET or ART_OWNER_PUBLIC       */
  int available;        /* FALSE when disabled for this campaign      */
  const char *lore;     /* one public line, no room or vnum in it     */
  const char *acq_hint; /* how it is found, in the same terms         */
};

/* --------------------------------------------------------------------------
 * Global state
 * -------------------------------------------------------------------------- */
extern struct artifact_data *art_index; /* sorted by vnum */
extern int total_artifacts;

/* --------------------------------------------------------------------------
 * API
 * -------------------------------------------------------------------------- */

/* Boot, shutdown, persistence */
void artifact_boot(void);
void artifact_shutdown(void);
void artifact_save(void);
void artifact_save_if_dirty(void);
void artifact_reload(void);

/* Lookup */
int artifact_search(int vnum);
struct artifact_data *artifact_by_vnum(int vnum);
struct artifact_data *artifact_of_obj(struct obj_data *obj);
int artifact_is_artifact(struct obj_data *obj);
int artifact_is_owned(int vnum);
int artifact_is_dropped(struct artifact_data *art);
int artifact_xp_to_next(int level);
const char *artifact_binding_name(int binding);
int artifact_recharge_remaining(struct artifact_data *art, int slot);
const char *artifact_recharge_name(int seconds);
size_t artifact_memory_used(void);

/* Chronicle, provenance, and the content contract */
int artifact_state(struct artifact_data *art);
const char *artifact_state_name(int state);
const char *artifact_acquisition_name(int acquisition);
const char *artifact_invoke_name(int channel);
int artifact_campaign_available(int campaigns);
int artifact_validate_metadata(void);

/* Stacking groups */
int artifact_stack_active(struct char_data *ch, int group);
void artifact_stack_clear(struct char_data *ch, int group);

/* Progressive passive powers */
void artifact_apply_passives(struct char_data *ch, struct artifact_data *art);
void artifact_remove_passives(struct char_data *ch, struct artifact_data *art);

/* Class restriction and the burn penalty */
int artifact_class_ok(struct char_data *ch, struct artifact_data *art);
void artifact_burn_tick(struct char_data *ch);

/* Called effects.  Each returns TRUE when the input invoked an artifact.
 * All three are thin wrappers over one channel-aware matcher. */
int artifact_speech_trigger(struct char_data *ch, const char *speech);
int artifact_whisper_trigger(struct char_data *ch, const char *speech);
int artifact_command_trigger(struct char_data *ch, const char *speech);

/* Ownership */
int artifact_to_char(struct obj_data *obj, struct char_data *ch);
int artifact_from_char(struct obj_data *obj, struct char_data *ch);

/* Nested containers */
void artifact_tag_nested(struct obj_data *obj, struct char_data *ch);
void artifact_get_nested(struct obj_data *obj, struct char_data *ch);
void artifact_drop_nested(struct obj_data *obj);

/* Bonuses, binding, progression */
void artifact_apply_bonuses(struct char_data *ch, struct obj_data *obj);
void artifact_remove_bonuses(struct char_data *ch, struct obj_data *obj);
int artifact_can_use(struct char_data *ch, struct obj_data *obj, int silent);
void artifact_grant_xp(struct char_data *ch, int amount);
void artifact_grant_xp_obj(struct char_data *ch, struct obj_data *obj, int amount);
void artifact_check_levelup(struct artifact_data *art);

/* Core-file hooks */
void artifact_obj_to_char(struct obj_data *obj, struct char_data *ch);
void artifact_obj_from_char(struct obj_data *obj);
void artifact_obj_to_room(struct obj_data *obj);
int artifact_on_equip(struct char_data *ch, struct obj_data *obj, int pos);
void artifact_on_unequip(struct char_data *ch, struct obj_data *obj);
void artifact_on_extract(struct obj_data *obj);
void artifact_begin_persistence_extract(void);
void artifact_end_persistence_extract(void);
int artifact_block_zone_load(obj_rnum obj_rnum);
int artifact_damage_resist(struct char_data *victim, int dam, int dam_type);
void artifact_combat_hit(struct char_data *ch, struct char_data *victim, int dam, int is_critical);
void artifact_combat_kill(struct char_data *ch, struct char_data *victim);
/* Returns TRUE when secondary proc damage killed the victim. */
int artifact_weapon_proc(struct char_data *ch, struct char_data *victim, struct obj_data *weapon,
                         int dam, int is_critical);

/* Commands */
ACMD_DECL(do_artifact);
ACMD_DECL(do_artifact_ability);
ACMD_DECL(do_artifact_invoke);
ACMD_DECL(do_testartifact);

/* Test seams - present only in the CuTest build. */
#ifdef LUMINARI_CUTEST
#define ARTIFACT_TEST_MAX_PASSIVES ART_PASSIVE_MAX_PER_ARTIFACT

struct artifact_test_passive_data
{
  int min_level;
  int aff_flag;
  int location;
  int modifier;
};

struct artifact_test_identity_data
{
  const char *ability_name;
  int generic_proc_chance;
  int signature_proc;
  int hand_proc_vnum;
  int hand_entry_odds;
  int called_effects[ARTIFACT_MAX_EFFECTS];
  int called_channels[ARTIFACT_MAX_EFFECTS];
  int called_stack_groups[ARTIFACT_MAX_EFFECTS];
  int passive_count;
  struct artifact_test_passive_data passives[ARTIFACT_TEST_MAX_PASSIVES];
};

void artifact_show_info_for_test(struct char_data *ch, struct obj_data *obj);
int artifact_force_generic_proc_for_test(struct char_data *ch, struct char_data *victim,
                                         struct obj_data *weapon, int proc_type);
int artifact_force_signature_roll_for_test(struct char_data *ch, struct char_data *victim,
                                           struct obj_data *weapon, int is_critical,
                                           int chance_roll);
int artifact_force_signature_proc_for_test(struct char_data *ch, struct char_data *victim,
                                           struct obj_data *weapon, int is_critical);
int artifact_force_doombringer_nested_proc_for_test(struct char_data *ch, struct char_data *victim,
                                                    struct obj_data *weapon);
int artifact_doombringer_attacks_for_test(void);
void artifact_force_avernus_survival_for_test(struct char_data *ch, struct char_data *victim,
                                              struct obj_data *weapon, int emergency_heal,
                                              int bladesong_heal);
int artifact_identity_for_test(int vnum, struct artifact_test_identity_data *identity);
#endif

#endif /* _SPEC_ARTIFACTS_H_ */

/*EOF*/
