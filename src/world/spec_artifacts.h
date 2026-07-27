/* ***************************************************************************
 *  File: spec_artifacts.h                            Part of LuminariMUD
 *  Usage: Artifact system - unique, single-instance items of power that
 *         track persistent ownership, gain levels through use, bind to their
 *         owners, and carry procs and active abilities.
 *
 *  Ported and modernized from the RealmsOfLuminari artifact system.  The
 *  full feature-by-feature mapping, including every deviation and every
 *  upstream defect fixed here, is recorded in
 *    docs/project-management-zusuk/ongoing-projects/artifacts.md
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
#define ARTIFACT_XP_PROC_SOUL 2
#define ARTIFACT_XP_PROC_HEAL 1
#define ARTIFACT_XP_PROC_FEAR 3
#define ARTIFACT_XP_PROC_DOOM 4
#define ARTIFACT_XP_PROC_ULTIMATE 10
#define ARTIFACT_XP_ABILITY_SOULSTRIKE 15
#define ARTIFACT_XP_ABILITY_DIVINEWARD 20
#define ARTIFACT_XP_ABILITY_DOOMBLAST 10

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
  int level;               /* 1 .. ARTIFACT_MAX_LEVEL                */
  int experience;          /* running XP total                       */
  int binding_type;        /* ARTIFACT_BIND_*                        */
  time_t bound_time;       /* when binding occurred, 0 = unbound     */
  int instance_persisted;  /* instance is in a player/house save     */
  time_t last_ability_use; /* ability cooldown stamp                 */
  time_t last_proc;        /* weapon proc internal cooldown stamp    */

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
int artifact_xp_to_next(int level);
const char *artifact_binding_name(int binding);

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
void artifact_combat_hit(struct char_data *ch, struct char_data *victim, int dam);
void artifact_combat_kill(struct char_data *ch, struct char_data *victim);
void artifact_weapon_proc(struct char_data *ch, struct char_data *victim, struct obj_data *weapon);

/* Commands */
ACMD_DECL(do_artifact);
ACMD_DECL(do_artifact_ability);
ACMD_DECL(do_testartifact);

#endif /* _SPEC_ARTIFACTS_H_ */

/*EOF*/
