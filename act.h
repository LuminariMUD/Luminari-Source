/***
 **
 * @file act.h                        Part of LuminariMUD
 * Header file for the core act* c files.
 *
 * Part of the core tbaMUD source code distribution, which is a derivative
 * of, and continuation of, CircleMUD.
 *
 * All rights reserved.  See license for complete information.
 * Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University
 * CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.
 *
 * @todo Utility functions that could easily be moved elsewhere have been
 * marked. Suggest a review of all utility functions (aka. non ACMDs) and
 * determine if the utility functions should be placed into a lower level
 * (non-ACMD focused) shared module.
 *
 */
#ifndef _ACT_H_
#define _ACT_H_

#include "utils.h" /* for the ACMD macro */

#define CAN_CMD 0
#define CANT_CMD_PERM 1
#define CANT_CMD_TEMP 2

/* from accounts.c */
ACMD_DECL(do_accexp);
extern const bool locked_races[NUM_RACES];
int has_unlocked_race(struct char_data *ch, int race);
int has_unlocked_class(struct char_data *ch, int class);
int hands_available(struct char_data *ch);
int get_speed(struct char_data *ch, sbyte to_display);

/*****************************************************************************
 * Begin general helper defines for all act files
 * These encapsulate some standard "can do this" checks.
 ****************************************************************************/

/** Check if character can actually fight. */
#define PREREQ_CAN_FIGHT()                        \
  if (!MOB_CAN_FIGHT(ch))                         \
  {                                               \
    send_to_char(ch, "But you can't fight!\r\n"); \
    return;                                       \
  }

/** Check the specified function to see if we get back a CAN_CMD. */
#define PREREQ_CHECK(name) \
  if (name(ch, true))      \
    return;

/** Check if the character has enough daily uses of the specified feat. */
#define PREREQ_HAS_USES(feat, errormsg)                       \
  int uses_remaining;                                         \
  if ((uses_remaining = daily_uses_remaining(ch, feat)) == 0) \
  {                                                           \
    send_to_char(ch, errormsg);                               \
    return;                                                   \
  }                                                           \
                                                              \
  if (uses_remaining < 0)                                     \
  {                                                           \
    send_to_char(ch, "You are not experienced enough.\r\n");  \
    return;                                                   \
  }

/** Check if the character is in the specified position or better. */
#define PREREQ_IN_POSITION(req_pos, errmsg) \
  if (GET_POS(ch) <= req_pos)               \
  {                                         \
    send_to_char(ch, errmsg);               \
    return;                                 \
  }

/** Check if character is not a NPC. */
#define PREREQ_NOT_NPC()                             \
  if (IS_NPC(ch))                                    \
  {                                                  \
    send_to_char(ch, "But you don't know how!\r\n"); \
    return;                                          \
  }

/** Check if character is in a peaceful room. */
#define PREREQ_NOT_PEACEFUL_ROOM()                                               \
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))                                  \
  {                                                                              \
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n"); \
    return;                                                                      \
  }

/** CHeck if character is in a single-file room. */
#define PREREQ_NOT_SINGLEFILE_ROOM()                                               \
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE))                                  \
  {                                                                                \
    send_to_char(ch, "The area is way too cramped to perform this maneuver!\r\n"); \
    return;                                                                        \
  }

/** Check for the target feat - to be used only within the ACMDCHECK() macro. */
#define ACMDCHECK_PREREQ_HASFEAT(feat, errormsg) \
  ACMDCHECK_PERMFAIL_IF(!HAS_FEAT(ch, feat), errormsg)

/** Check for the specified condition and fail permanently if it's true.
 * In other words, the character doesn't have the ability to use this command.
 * To be used only within the ACMDCHECK() macro.
 */
#define ACMDCHECK_PERMFAIL_IF(code, errormsg) \
  if (code)                                   \
  {                                           \
    ACMD_ERRORMSG(errormsg);                  \
    return CANT_CMD_PERM;                     \
  }

/** Check for the specified condition and temporarily fail if it's true.
 * In other words, the character has the ability to use this command, but are
 * missing something else that would cause it to fail.
 * To be used only within the ACMDCHECK() macro.
 */
#define ACMDCHECK_TEMPFAIL_IF(code, errormsg) \
  if (code)                                   \
  {                                           \
    ACMD_ERRORMSG(errormsg);                  \
    return CANT_CMD_TEMP;                     \
  }

/*****************************************************************************
 * Begin Functions and defines for act.comm.c
 ****************************************************************************/
/* functions with subcommands */
/* do_gen_comm */
ACMD_DECL(do_gen_comm);
#define SCMD_HOLLER 0
#define SCMD_SHOUT 1
#define SCMD_GOSSIP 2
#define SCMD_CHAT SCMD_GOSSIP
#define SCMD_AUCTION 3
#define SCMD_GRATZ 4
#define SCMD_GEMOTE 5
/* do_qcomm */
ACMD_DECL(do_qcomm);
#define SCMD_QSAY 0
#define SCMD_QECHO 1
/* do_spec_com */
ACMD_DECL(do_spec_comm);
#define SCMD_WHISPER 0
#define SCMD_ASK 1
/* functions without subcommands */
ACMD_DECL(do_say);
ACMD_DECL(do_gsay);
ACMD_DECL(do_page);
ACMD_DECL(do_reply);
ACMD_DECL(do_tell);
ACMD_DECL(do_write);
/*****************************************************************************
 * Begin Functions and defines for act.informative.c
 ****************************************************************************/
/* Utility Functions */

// char creation help files
void perform_help(struct descriptor_data *d, const char *argument);

/* character info */
void perform_affects(struct char_data *ch, struct char_data *k);
void perform_abilities(struct char_data *ch, struct char_data *k);
void perform_cooldowns(struct char_data *ch, struct char_data *k);
void perform_resistances(struct char_data *ch, struct char_data *k);

// displaying more info -zusuk
void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode, int mxp_type);
#define SHOW_OBJ_SHORT 1
void lore_id_vict(struct char_data *ch, struct char_data *tch);

/** @todo Move to a utility library */
char *find_exdesc(char *word, struct extra_descr_data *list);
/** @todo Move to a mud centric string utility library */
void space_to_minus(char *str);
/** @todo Move to a help module? */
void game_info(const char *format, ...);
void free_history(struct char_data *ch, int type);
void free_recent_players(void);
/* functions with subcommands */
/* do_commands */
ACMD_DECL(do_commands);
#define SCMD_COMMANDS 0
#define SCMD_SOCIALS 1
#define SCMD_WIZHELP 2
#define SCMD_MANEUVERS 3
/* do_gen_ps */
ACMD_DECL(do_gen_ps);
#define SCMD_INFO 0
#define SCMD_HANDBOOK 1
#define SCMD_CREDITS 2
#define SCMD_NEWS 3
#define SCMD_WIZLIST 4
#define SCMD_POLICIES 5
#define SCMD_VERSION 6
#define SCMD_IMMLIST 7
#define SCMD_MOTD 8
#define SCMD_IMOTD 9
#define SCMD_CLEAR 10
#define SCMD_WHOAMI 11
/* do_look */
ACMD_DECL(do_look);
#define SCMD_LOOK 0
#define SCMD_READ 1
#define SCMD_HERE 2
ACMD_DECL(do_affects);
#define SCMD_AFFECTS 0
#define SCMD_COOLDOWNS 1
#define SCMD_RESISTANCES 2

/* functions without subcommands */
ACMD_DECL(do_innates);
ACMD_DECL(do_abilities);
ACMD_DECL(do_masterlist);
ACMD_DECL(do_areas);
ACMD_DECL(do_attacks);
ACMD_DECL(do_consider);
ACMD_DECL(do_defenses);
ACMD_DECL(do_damage);
ACMD_DECL(do_diagnose);
ACMD_DECL(do_disengage);
ACMD_DECL(do_equipment);
ACMD_DECL(do_examine);
ACMD_DECL(do_exits);
ACMD_DECL(do_survey);
ACMD_DECL(do_gold);
ACMD_DECL(do_help);
ACMD_DECL(do_history);
ACMD_DECL(do_inventory);
ACMD_DECL(do_levels);
ACMD_DECL(do_scan);
ACMD_DECL(do_score);
ACMD_DECL(do_spot);
ACMD_DECL(do_listen);
ACMD_DECL(do_time);
ACMD_DECL(do_toggle);
ACMD_DECL(do_users);
ACMD_DECL(do_weather);
ACMD_DECL(do_where);
ACMD_DECL(do_who);
ACMD_DECL(do_whois);
ACMD_DECL(do_track);
ACMD_DECL(do_hp);
ACMD_DECL(do_tnl);
ACMD_DECL(do_moves);
ACMD_DECL(do_divine_bond);
ACMD_DECL(do_fiendishboon);
ACMD_DECL(do_mercies);
ACMD_DECL(do_cruelties);
ACMD_DECL(do_touch_of_corruption);
ACMD_DECL(do_lichtouch);
ACMD_DECL(do_lichfear);
ACMD_DECL(do_maxhp);
ACMD_DECL(do_judgement);
ACMD_DECL(do_bane);
ACMD_DECL(do_slayer);
ACMD_DECL(do_true_judgement);

int max_judgements_active(struct char_data *ch);
int num_judgements_active(struct char_data *ch);

bool is_door_locked(room_rnum room, int door);

/*****************************************************************************
 * Begin Functions and defines for rank.c
 ****************************************************************************/
void do_slug_rank(struct char_data *ch, const char *arg);
ACMD_DECL(do_rank);

/*****************************************************************************
 * Begin Functions and defines for act.item.c
 ****************************************************************************/
/* defines */
#define EXITN(room, door) (world[room].dir_option[door])
#define OPEN_DOOR(room, obj, door) ((obj) ? (REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) : (REMOVE_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define CLOSE_DOOR(room, obj, door) ((obj) ? (SET_BIT(GET_OBJ_VAL(obj, 1), CONT_CLOSED)) : (SET_BIT(EXITN(room, door)->exit_info, EX_CLOSED)))
#define LOCK_DOOR(room, obj, door) ((obj) ? (SET_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) : (SET_BIT(EXITN(room, door)->exit_info, EX_LOCKED_EASY)))
#define UNLOCK_DOOR(room, obj, door) ((obj) ? (REMOVE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) : (remove_locked_door_flags(room, door)))
#define IS_CLOSED(x, y) (EXIT_FLAGGED(world[(x)].dir_option[(y)], EX_CLOSED))
/* added subcommands for do_get() for group loot system */
#define GET_SUBCMD_NORMAL 0
#define GET_SUBCMD_GLOOT 1

/* HACK: Had to change this with the new lock strengths from homeland... */

#define TOGGLE_LOCK(room, obj, door) ((obj) ? (TOGGLE_BIT(GET_OBJ_VAL(obj, 1), CONT_LOCKED)) : (is_door_locked(room, door) ? UNLOCK_DOOR(room, obj, door) : LOCK_DOOR(room, obj, door)))

/*(TOGGLE_BIT(EXITN(room, door)->exit_info, EX_LOCKED)))*/

/* Utility Functions */
/** @todo Compare with needs of find_eq_pos_script. */
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
void name_from_drinkcon(struct obj_data *obj);
void name_to_drinkcon(struct obj_data *obj, int type);
void weight_change_object(struct obj_data *obj, int weight);
void perform_remove(struct char_data *ch, int pos, bool forced);
bool perform_give(struct char_data *ch, struct char_data *vict, struct obj_data *obj);
void perform_wear(struct char_data *ch, struct obj_data *obj, int where);
bool obj_should_fall(struct obj_data *obj);
bool char_should_fall(struct char_data *ch, bool silent);
bool perform_wield(struct char_data *ch, struct obj_data *obj, bool not_silent);
void start_auction(struct char_data *ch, struct obj_data *obj, int bid);
void auc_stat(struct char_data *ch, struct obj_data *obj);
void stop_auction(int type, struct char_data *ch);
void check_auction(void);
void auc_send_to_all(char *messg, bool buyer);
void list_consumables(struct char_data *ch, int type);

/* functions with subcommands */
/* do_drop */
ACMD_DECL(do_drop);
#define SCMD_DROP 0
#define SCMD_JUNK 1
#define SCMD_DONATE 2
/* do_eat */
ACMD_DECL(do_eat);
#define SCMD_EAT 0
#define SCMD_TASTE 1
#define SCMD_DRINK 2
#define SCMD_SIP 3
/* do_pour */
ACMD_DECL(do_pour);
#define SCMD_POUR 0
#define SCMD_FILL 1

/* AUCTIONING STATES */
#define AUC_NULL_STATE 0  /* not doing anything */
#define AUC_OFFERING 1    /* object has been offfered */
#define AUC_GOING_ONCE 2  /* object is going once! */
#define AUC_GOING_TWICE 3 /* object is going twice! */
#define AUC_LAST_CALL 4   /* last call for the object! */
#define AUC_SOLD 5

/* AUCTION CANCEL STATES */
#define AUC_NORMAL_CANCEL 6 /* normal cancellation of auction */
#define AUC_QUIT_CANCEL 7   /* auction canclled because player quit */
#define AUC_WIZ_CANCEL 8    /* auction cancelled by a god */

/* OTHER JUNK */
#define AUC_STAT 9
#define AUC_BID 10

/* functions without subcommands */
ACMD_DECL(do_drink);
ACMD_DECL(do_get);
ACMD_DECL(do_give);
ACMD_DECL(do_grab);
ACMD_DECL(do_priceset);
ACMD_DECL(do_put);
ACMD_DECL(do_remove);
ACMD_DECL(do_sac);
ACMD_DECL(do_wear);
ACMD_DECL(do_wield);
ACMD_DECL(do_auction);
ACMD_DECL(do_bid);
ACMD_DECL(do_channelspell);
ACMDCHECK(can_channel_energy);
ACMD_DECL(do_channelenergy);
ACMD_DECL(do_store);
ACMD_DECL(do_unstore);
ACMD_DECL(do_use_consumable);
ACMD_DECL(do_outfit);

/*****************************************************************************
 * Begin Functions and defines for act.movement.c
 ****************************************************************************/

int has_boat(struct char_data *ch, room_rnum going_to);
int has_flight(struct char_data *ch);
int change_position(struct char_data *ch, int position);
int perform_move_full(struct char_data *ch, int dir, int need_specials_check, bool recursive);
int is_evaporating_key(struct char_data *ch, obj_vnum key);
int has_key(struct char_data *ch, obj_vnum key);
bool can_stand(struct char_data *ch);

/* Functions with subcommands */
/* do_gen_door */
ACMD_DECL(do_gen_door);
#define SCMD_OPEN 0
#define SCMD_CLOSE 1
#define SCMD_UNLOCK 2
#define SCMD_LOCK 3
#define SCMD_PICK 4
/* Functions without subcommands */
ACMD_DECL(do_disembark);
ACMD_DECL(do_enter);
ACMD_DECL(do_follow);
ACMD_DECL(do_unlead);
ACMD_DECL(do_leave);
ACMD_DECL(do_move);
ACMD_DECL(do_rest);
ACMD_DECL(do_sit);
ACMD_DECL(do_recline);
ACMD_DECL(do_sleep);
ACMD_DECL(do_stand);
ACMD_DECL(do_wake);
ACMD_DECL(do_pullswitch);

/* Switch info */
#define SWITCH_UNHIDE 0
#define SWITCH_UNLOCK 1
#define SWITCH_OPEN 2
/* Global variables from act.movement.c */
#ifndef __ACT_MOVEMENT_C__
extern const char *const cmd_door[];
#endif /* __ACT_MOVEMENT_C__ */

/*****************************************************************************
 * Begin Functions and defines for act.offensive.c
 ****************************************************************************/
/* functions */
void clear_rage(struct char_data *ch);
void clear_defensive_stance(struct char_data *ch);
void perform_stunningfist(struct char_data *ch);
void perform_quiveringpalm(struct char_data *ch);
void perform_deatharrow(struct char_data *ch);
void perform_rescue(struct char_data *ch, struct char_data *vict);
void perform_smite(struct char_data *ch, int smite_type);
void perform_rage(struct char_data *ch);
void perform_layonhands(struct char_data *ch, struct char_data *vict);
bool perform_knockdown(struct char_data *ch, struct char_data *vict,
                       int skill);
bool perform_shieldpunch(struct char_data *ch, struct char_data *vict);
void perform_headbutt(struct char_data *ch, struct char_data *vict);
void perform_sap(struct char_data *ch, struct char_data *vict);
void perform_kick(struct char_data *ch, struct char_data *vict);
void perform_slam(struct char_data *ch, struct char_data *vict);
bool perform_dirtkick(struct char_data *ch, struct char_data *vict);
void perform_assist(struct char_data *ch, struct char_data *helpee);
void perform_springleap(struct char_data *ch, struct char_data *vict);
bool perform_backstab(struct char_data *ch, struct char_data *vict);
int perform_collect(struct char_data *ch, bool silent);
void apply_blackguard_cruelty(struct char_data *ch, struct char_data *vict, char *cruelty);
void throw_hedging_weapon(struct char_data *ch);
void perform_true_judgement(struct char_data *ch);
/* Functions with subcommands */
/* do_hit */
ACMD_DECL(do_hit);
#define SCMD_HIT 0
ACMD_DECL(do_process_attack);
ACMD_DECL(do_fey_magic);
ACMDCHECK(can_fey_magic);
ACMD_DECL(do_grave_magic);
ACMDCHECK(can_grave_magic);
ACMD_DECL(do_mark);

/* Functions without subcommands */
ACMD_DECL(do_cexchange);
// ACMD(do_cexchange);
ACMD_DECL(do_fire);
ACMD_DECL(do_aura_of_vengeance);
ACMD_DECL(do_aura_of_justice);
ACMD_DECL(do_reload);
ACMD_DECL(do_autofire);
ACMD_DECL(do_collect);
ACMD_DECL(do_hitall);
ACMDCHECK(can_hitall);
ACMD_DECL(do_guard);
ACMDCHECK(can_guard);
ACMD_DECL(do_charge);
ACMDCHECK(can_charge);
ACMD_DECL(do_circle);
ACMDCHECK(can_circle);
ACMD_DECL(do_bodyslam);
ACMDCHECK(can_bodyslam);
ACMD_DECL(do_springleap);
ACMDCHECK(can_springleap);
ACMD_DECL(do_feint);
ACMDCHECK(can_feint);
ACMD_DECL(do_headbutt);
ACMDCHECK(can_headbutt);
ACMD_DECL(do_shieldpunch);
ACMDCHECK(can_shieldpunch);
ACMD_DECL(do_disarm);
ACMDCHECK(can_disarm);
ACMD_DECL(do_shieldcharge);
ACMDCHECK(can_shieldcharge);
ACMD_DECL(do_shieldslam);
ACMDCHECK(can_shieldslam);
ACMD_DECL(do_dirtkick);
ACMDCHECK(can_dirtkick);
ACMD_DECL(do_sap);
ACMDCHECK(can_sap);
ACMD_DECL(do_assist);
ACMD_DECL(do_rage);
ACMDCHECK(can_rage);
ACMD_DECL(do_sacredflames);
ACMD_DECL(do_innerfire);
ACMD_DECL(do_defensive_stance);
ACMDCHECK(can_defensive_stance);
ACMD_DECL(do_turnundead);
ACMDCHECK(can_turnundead);
ACMD_DECL(do_bash);
ACMDCHECK(can_bash);
ACMD_DECL(do_call);
ACMD_DECL(do_fly);
ACMD_DECL(do_levitate);
ACMD_DECL(do_strength);
ACMD_DECL(do_invisduergar);
ACMD_DECL(do_enlarge);
ACMD_DECL(do_darkness);
ACMD_DECL(do_invisiblerogue);
ACMD_DECL(do_land);
ACMD_DECL(do_frightful);
ACMD_DECL(do_breathe);
ACMDCHECK(can_breathe);
ACMD_DECL(do_tailsweep);
ACMDCHECK(can_tailsweep);
ACMD_DECL(do_backstab);
ACMDCHECK(can_backstab);
ACMD_DECL(do_flee);
ACMD_DECL(do_stunningfist);
ACMDCHECK(can_stunningfist);
ACMD_DECL(do_quiveringpalm);
ACMDCHECK(can_quiveringpalm);
ACMD_DECL(do_deatharrow);
ACMDCHECK(can_deatharrow);
ACMD_DECL(do_faeriefire);
ACMDCHECK(can_faeriefire);
ACMD_DECL(do_kick);
ACMDCHECK(can_kick);
ACMD_DECL(do_slam);
ACMDCHECK(can_slam);
ACMD_DECL(do_seekerarrow);
ACMDCHECK(can_seekerarrow);
ACMD_DECL(do_arrowswarm);
ACMDCHECK(can_arrowswarm);
ACMD_DECL(do_smiteevil);
ACMDCHECK(can_smiteevil);
ACMD_DECL(do_smitegood);
ACMDCHECK(can_smitegood);
ACMD_DECL(do_kill);
ACMD_DECL(do_layonhands);
ACMDCHECK(can_layonhands);
ACMD_DECL(do_order);
ACMD_DECL(do_applypoison);
ACMD_DECL(do_sorcerer_arcane_apotheosis);
ACMD_DECL(do_imbuearrow);
ACMD_DECL(do_abundantstep);
ACMD_DECL(do_animatedead);
ACMD_DECL(do_rescue);
ACMDCHECK(can_rescue);
ACMD_DECL(do_taunt);
ACMDCHECK(can_taunt);
ACMD_DECL(do_intimidate);
ACMDCHECK(can_intimidate);
ACMD_DECL(do_treatinjury);
ACMDCHECK(can_treatinjury);
ACMD_DECL(do_emptybody);
ACMDCHECK(can_emptybody);
ACMD_DECL(do_wholenessofbody);
ACMDCHECK(can_wholenessofbody);
ACMD_DECL(do_trip);
ACMDCHECK(can_trip);
ACMD_DECL(do_whirlwind);
ACMDCHECK(can_whirlwind);
ACMD_DECL(do_crystalfist);
ACMDCHECK(can_crystalfist);
ACMD_DECL(do_crystalbody);
ACMDCHECK(can_crystalbody);
ACMD_DECL(do_surpriseaccuracy);
ACMDCHECK(can_surpriseaccuracy);
ACMD_DECL(do_powerfulblow);
ACMDCHECK(can_powerfulblow);
ACMD_DECL(do_renewedvigor);
ACMDCHECK(can_renewedvigor);
ACMD_DECL(do_reneweddefense);
ACMDCHECK(can_reneweddefense);
ACMD_DECL(do_comeandgetme);
ACMDCHECK(can_comeandgetme);
ACMD_DECL(do_sorcerer_breath_weapon);
ACMDCHECK(can_sorcerer_breath_weapon);
ACMD_DECL(do_sorcerer_claw_attack);
ACMDCHECK(can_sorcerer_claw_attack);
ACMD_DECL(do_sorcerer_draconic_wings);
ACMD_DECL(do_impromptu);
ACMDCHECK(can_impromptu);
ACMD_DECL(do_favoredenemies);
ACMD_DECL(do_summon);
ACMD_DECL(do_dice);
ACMD_DECL(do_applyoil);
ACMD_DECL(do_setbaneweapon);
ACMD_DECL(do_cosmic_awareness);
ACMD_DECL(do_discharge);
ACMD_DECL(do_revoke);
ACMD_DECL(do_children_of_the_night);
ACMDCHECK(can_children_of_the_night);
ACMDCHECK(can_create_vampire_spawn);
ACMD_DECL(do_create_vampire_spawn);
ACMDCHECK(can_vampiric_gaseous_form);
ACMD_DECL(do_vampiric_gaseous_form);
ACMDCHECK(can_vampiric_shape_change);
ACMD_DECL(do_vampiric_shape_change);
ACMDCHECK(can_vampiric_dominate);
ACMD_DECL(do_vampiric_dominate);
/*****************************************************************************
 * Begin Functions and defines for act.other.c
 ****************************************************************************/
/* Functions with subcommands */
void invoke_happyhour(struct char_data *ch);
bool is_prompt_empty(struct char_data *ch);
void set_bonus_attributes(struct char_data *ch, int str, int con, int dex, int ac);
void list_forms(struct char_data *ch);
void perform_shapechange(struct char_data *ch, char *arg, int mode);
void wildshape_return(struct char_data *ch);
void perform_wildshape(struct char_data *ch, int form_num, int spellnum);
void perform_perform(struct char_data *ch);
void perform_call(struct char_data *ch, int call_type, int level);
void update_msdp_group(struct char_data *ch);
void update_msdp_inventory(struct char_data *ch);
bool wildshape_engine(struct char_data *ch, const char *argument, int mode);
void show_hints(void);
void display_todo(struct char_data *ch, struct char_data *vict);
void respec_engine(struct char_data *ch, int class, char *arg, bool silent);
int perform_tailsweep(struct char_data *ch);
int perform_dragonbite(struct char_data *ch, struct char_data *vict);
void perform_children_of_the_night(struct char_data *ch);
void perform_save(struct char_data *ch, int mode);

/* do_gen_tog */
ACMD_DECL(do_gen_tog);

/* sub-command defines */
#define SCMD_NOSUMMON 0
#define SCMD_NOHASSLE 1
#define SCMD_BRIEF 2
#define SCMD_COMPACT 3
#define SCMD_NOTELL 4
#define SCMD_NOAUCTION 5
#define SCMD_NOSHOUT 6
#define SCMD_NOGOSSIP 7
#define SCMD_NOGRATZ 8
#define SCMD_NOWIZ 9
#define SCMD_QUEST 10
#define SCMD_SHOWVNUMS 11
#define SCMD_NOREPEAT 12
#define SCMD_HOLYLIGHT 13
#define SCMD_SLOWNS 14
#define SCMD_AUTOEXIT 15
#define SCMD_TRACK 16
#define SCMD_CLS 17
#define SCMD_BUILDWALK 18
#define SCMD_AFK 19
#define SCMD_AUTOLOOT 20
#define SCMD_AUTOGOLD 21
#define SCMD_AUTOSPLIT 22
#define SCMD_AUTOSAC 23
#define SCMD_AUTOASSIST 24
#define SCMD_AUTOMAP 25
#define SCMD_AUTOKEY 26
#define SCMD_AUTODOOR 27
#define SCMD_NOCLANTALK 28
#define SCMD_COLOR 29
#define SCMD_SYSLOG 30
#define SCMD_WIMPY 31
#define SCMD_PAGELENGTH 32
#define SCMD_SCREENWIDTH 33
#define SCMD_AUTOSCAN 34
#define SCMD_AUTORELOAD 35
#define SCMD_COMBATROLL 36
#define SCMD_GUI_MODE 37
#define SCMD_NOHINT 38
#define SCMD_AUTOCOLLECT 39
#define SCMD_RP 40
#define SCMD_AOE_BOMBS 41
#define SCMD_AUTOCONSIDER 42
#define SCMD_SMASH_DEFENSE 43
#define SCMD_NOCHARMIERESCUES 44
#define SCMD_USE_STORED_CONSUMABLES 45
#define SCMD_AUTO_STAND 46

/* do_quit */
ACMD_DECL(do_quit);
#define SCMD_QUI 0
#define SCMD_QUIT 1
/* do_use */
ACMD_DECL(do_use);
#define SCMD_USE 0
#define SCMD_QUAFF 1
#define SCMD_RECITE 2
#define SCMD_INVOKE 3
/* do_utter */
ACMD_DECL(do_utter);
/* do_diplomacy */
ACMD_DECL(do_diplomacy);
#define SCMD_MURMUR 0
#define SCMD_PROPAGANDA 1
#define SCMD_LOBBY 2
/* used by diplomacy skills */
#define DIP_SKILL (diplomacy_types[dip_num].skill)
#define DIP_INCR (diplomacy_types[dip_num].increase)
#define DIP_WAIT (diplomacy_types[dip_num].wait)
/* Functions without subcommands */
ACMD_DECL(do_recharge);
ACMD_DECL(do_nop);
ACMD_DECL(do_buck);
ACMD_DECL(do_dismount);
ACMD_DECL(do_mount);
ACMD_DECL(do_dismiss);
ACMD_DECL(do_tame);
ACMD_DECL(do_boosts);
ACMD_DECL(do_respec);
ACMD_DECL(do_gain);
ACMD_DECL(do_display);
ACMD_DECL(do_shapechange);
ACMD_DECL(do_group);
ACMD_DECL(do_greport);
ACMD_DECL(do_purify);
ACMD_DECL(do_happyhour);
ACMD_DECL(do_hide);
ACMD_DECL(do_lore);
ACMD_DECL(do_not_here);
ACMD_DECL(do_practice);
ACMD_DECL(do_report);
ACMD_DECL(do_save);
ACMD_DECL(do_search);
ACMD_DECL(do_sneak);
ACMD_DECL(do_spelllist);
ACMD_DECL(do_spells);
ACMD_DECL(do_split);
ACMD_DECL(do_steal);
ACMD_DECL(do_title);
ACMD_DECL(do_train);
ACMD_DECL(do_visible);
ACMD_DECL(do_wildshape);
ACMD_DECL(do_vanish);
ACMD_DECL(do_disguise);
ACMD_DECL(do_ethshift);
ACMD_DECL(do_handleanimal);
// ACMD_DECL(do_nohints);
ACMD_DECL(do_todo);

/*****************************************************************************
 * Begin Functions and defines for act.social.c
 ****************************************************************************/
/* Utility Functions */
void free_social_messages(void);
/** @todo free_action should be moved to a utility function module. */
void free_action(struct social_messg *mess);
/** @todo command list functions probably belong in interpreter */
void free_command_list(void);
/** @todo command list functions probably belong in interpreter */
void create_command_list(void);
/* Functions without subcommands */
ACMD_DECL(do_action);
ACMD_DECL(do_gmote);

/******************
 * memorize
 *******************/
ACMD_DECL(do_gen_forget);

#define SCMD_FORGET 1
#define SCMD_BLANK 2
#define SCMD_UNCOMMUNE 3
#define SCMD_OMIT 4
#define SCMD_UNADJURE 5
#define SCMD_DISCARD 6

ACMD_DECL(do_gen_memorize);

#define SCMD_MEMORIZE 1
#define SCMD_PRAY 2
#define SCMD_COMMUNE 3
#define SCMD_MEDITATE 4
#define SCMD_CHANT 5
#define SCMD_ADJURE 6
#define SCMD_COMPOSE 7
#define SCMD_CONCOCT 8
#define SCMD_POWERS 9

/*****************************************************************************
 * Begin Functions and defines for act.wizard.c
 ****************************************************************************/
/* Utility Functions */
/** @todo should probably be moved to a more general file handler module */
void clean_llog_entries(void);
/** @todo This should be moved to a more general utility file */
int script_command_interpreter(struct char_data *ch, char *arg);
room_rnum find_target_room(struct char_data *ch, const char *rawroomstr);
void perform_immort_vis(struct char_data *ch);
void snoop_check(struct char_data *ch);
bool change_player_name(struct char_data *ch, struct char_data *vict, char *new_name);
bool AddRecentPlayer(char *chname, char *chhost, bool newplr, bool cpyplr);
int get_eq_score(obj_rnum a);
/* Functions with subcommands */
/* do_date */
ACMD_DECL(do_date);
#define SCMD_DATE 0
#define SCMD_UPTIME 1
/* do_echo */
ACMD_DECL(do_echo);
#define SCMD_ECHO 0
#define SCMD_EMOTE 1
/* do_last */
ACMD_DECL(do_last);
#define SCMD_LIST_ALL 1
/* do_shutdown */
ACMD_DECL(do_shutdown);
#define SCMD_SHUTDOW 0
#define SCMD_SHUTDOWN 1
/* do_wizutil */
ACMD_DECL(do_wizutil);
#define SCMD_REROLL 0
#define SCMD_PARDON 1
#define SCMD_NOTITLE 2
#define SCMD_MUTE 3
#define SCMD_FREEZE 4
#define SCMD_THAW 5
#define SCMD_UNAFFECT 6
/* Functions without subcommands */
ACMD_DECL(do_setroomsect);
ACMD_DECL(do_setworldsect);
ACMD_DECL(do_setroomname);
ACMD_DECL(do_setroomdesc);
ACMD_DECL(do_hlqlist);
ACMD_DECL(do_advance);
ACMD_DECL(do_objlist);
ACMD_DECL(do_singlefile);
ACMD_DECL(do_at);
ACMD_DECL(do_checkloadstatus);
ACMD_DECL(do_copyover);
ACMD_DECL(do_dc);
ACMD_DECL(do_changelog);
ACMD_DECL(do_file);
ACMD_DECL(do_force);
ACMD_DECL(do_gecho);
ACMD_DECL(do_goto);
ACMD_DECL(do_invis);
ACMD_DECL(do_links);
ACMD_DECL(do_keycheck);
ACMD_DECL(do_load);
ACMD_DECL(do_oset);
ACMD_DECL(do_peace);
ACMD_DECL(do_plist);
ACMD_DECL(do_purge);
ACMD_DECL(do_recent);
ACMD_DECL(do_restore);
ACMD_DECL(do_return);
ACMD_DECL(do_saveall);
ACMD_DECL(do_savemobs);
ACMD_DECL(do_send);
ACMD_DECL(do_set);
ACMD_DECL(do_show);
ACMD_DECL(do_snoop);
ACMD_DECL(do_stat);
ACMD_DECL(do_switch);
ACMD_DECL(do_teleport);
ACMD_DECL(do_trans);
ACMD_DECL(do_vnum);
ACMD_DECL(do_vstat);
ACMD_DECL(do_wizlock);
ACMD_DECL(do_wiznet);
ACMD_DECL(do_wizupdate);
ACMD_DECL(do_zcheck);
ACMD_DECL(do_zlock);
ACMD_DECL(do_zpurge);
ACMD_DECL(do_zreset);
ACMD_DECL(do_zunlock);
ACMD_DECL(do_afflist);
ACMD_DECL(do_typelist);
ACMD_DECL(do_eqrating);
ACMD_DECL(do_coordconvert);
ACMD_DECL(do_genmap);
ACMD_DECL(do_genriver);
ACMD_DECL(do_deletepath);
ACMD_DECL(do_oconvert);
ACMD_DECL(do_acconvert);
ACMD_DECL(do_findmagic);
ACMD_DECL(do_cmdlev);
ACMD_DECL(do_obind);
ACMD_DECL(do_unbind);
// ACMD_DECL(do_plist);
ACMD_DECL(do_finddoor);
ACMD_DECL(do_bombs);
ACMD_DECL(do_bandage);
ACMD_DECL(do_players);
ACMD_DECL(do_copyroom);
ACMD_DECL(do_loot);
ACMD_DECL(do_weapontypes);
ACMD_DECL(do_weaponproficiencies);
ACMD_DECL(do_weaponinfo);
ACMD_DECL(do_armorinfo);
ACMD_DECL(do_autocon);
ACMD_DECL(do_perfmon);
ACMD_DECL(do_showwearoff);
ACMD_DECL(do_poisonbreath);
ACMDCHECK(can_poisonbreath);
ACMD_DECL(do_tailspikes);
ACMDCHECK(can_tailspikes);
ACMD_DECL(do_pixiedust);
ACMD_DECL(do_pixieinvis);
ACMDCHECK(can_pixiedust);
ACMDCHECK(can_pixieinvis);
ACMD_DECL(do_dragonfear);
ACMDCHECK(can_dragonfear);
ACMDCHECK(can_efreetimagic);
ACMD_DECL(do_efreetimagic);
ACMDCHECK(can_dragonmagic);
ACMD_DECL(do_dragonmagic);
ACMD_DECL(do_resetpassword);
ACMD_DECL(do_holyweapon);
ACMD_DECL(do_award);

// encounters.c
ACMD_DECL(do_encounterinfo);
ACMD_DECL(do_encounter);

// psionics.c

ACMD_DECL(do_psionic_focus);
ACMDCHECK(can_psionic_focus);
ACMD_DECL(do_double_manifest);
ACMDCHECK(can_double_manifest);

// deities.c
ACMD_DECL(do_devote);

#endif /* _ACT_H_ */
