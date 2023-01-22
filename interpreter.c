/**************************************************************************
 *  File: interpreter.c                                Part of LuminariMUD *
 *  Usage: Parse user commands, search for specials, call ACMD functions.  *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 *                                                                         *
 *  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
 *  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
 **************************************************************************/

#define __INTERPRETER_C__

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "handler.h"
#include "mail.h"
#include "screen.h"
#include "genolc.h"
#include "oasis.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "constants.h"
#include "act.h" /* ACMDs located within the act*.c files, char-creation help */
#include "ban.h"
#include "class.h"
#include "graph.h"
#include "hedit.h"
#include "house.h"
#include "config.h"
#include "modify.h" /* for do_skillset... */
#include "quest.h"
#include "hlquest.h"
#include "asciimap.h"
#include "prefedit.h"
#include "ibt.h"
#include "mud_event.h"
#include "race.h"
#include "clan.h"
#include "craft.h"
#include "treasure.h"
#include "feats.h"
#include "actions.h"
#include "actionqueues.h"
#include "combat_modes.h"
#include "traps.h"
#include "domains_schools.h"
#include "grapple.h"
#include "assign_wpn_armor.h"
#include "bardic_performance.h"
#include "spell_prep.h"
#include "crafts.h" /* NewCraft */
#include "new_mail.h"
#include "alchemy.h"
#include "helpers.h"
#include "staff_events.h"
#include "premadebuilds.h"
#include "missions.h"
#include "transport.h"
#include "hunts.h"
#include "fight.h" /* for init condensed combat */
#include "char_descs.h"

/* local (file scope) functions */
static int perform_dupe_check(struct descriptor_data *d);
static struct alias_data *find_alias(struct alias_data *alias_list, char *str);
static void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a);
static int _parse_name(char *arg, char *name);
static bool perform_new_char_dupe_check(struct descriptor_data *d);
/* sort_commands utility */
static int sort_commands_helper(const void *a, const void *b);

// external functions
void load_char_pets(struct char_data *ch);

/* globals defined here, used here and elsewhere */
int *cmd_sort_info = NULL;

struct command_info *complete_cmd_info;

ACMD_DECL(do_reboot);

/* This is the Master Command List. You can put new commands in, take commands
 * out, change the order they appear in, etc.  You can adjust the "priority"
 * of commands simply by changing the order they appear in the command list.
 * (For example, if you want "as" to mean "assist" instead of "ask", just put
 * "assist" above "ask" in the Master Command List. In general, utility
 * commands such as "at" should have high priority; infrequently used and
 * dangerously destructive commands should have low priority.
 *
 * Cooldowns for used actions are in the last column, {X, Y} where X is the
 * cooldown for a standard action used and Y is the cooldown for the Move action used. */

cpp_extern const struct command_info cmd_info[] = {
    {"RESERVED", "", 0, 0, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL}, /* this must be first -- for specprocs */
    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    /* directions must come before other commands but after RESERVED */
    {"north", "n", POS_RECLINING, do_move, 0, SCMD_NORTH, FALSE, ACTION_MOVE, {0, 0}, NULL},
    {"east", "e", POS_RECLINING, do_move, 0, SCMD_EAST, FALSE, ACTION_MOVE, {0, 0}, NULL},
    {"south", "s", POS_RECLINING, do_move, 0, SCMD_SOUTH, FALSE, ACTION_MOVE, {0, 0}, NULL},
    {"west", "w", POS_RECLINING, do_move, 0, SCMD_WEST, FALSE, ACTION_MOVE, {0, 0}, NULL},
    {"up", "u", POS_RECLINING, do_move, 0, SCMD_UP, FALSE, ACTION_MOVE, {0, 0}, NULL},
    {"down", "d", POS_RECLINING, do_move, 0, SCMD_DOWN, FALSE, ACTION_MOVE, {0, 0}, NULL},
    {"northwest", "northw", POS_RECLINING, do_move, 0, SCMD_NW, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"nw", "nw", POS_RECLINING, do_move, 0, SCMD_NW, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"northeast", "northe", POS_RECLINING, do_move, 0, SCMD_NE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"ne", "ne", POS_RECLINING, do_move, 0, SCMD_NE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"southeast", "southe", POS_RECLINING, do_move, 0, SCMD_SE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"se", "se", POS_RECLINING, do_move, 0, SCMD_SE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"southwest", "southw", POS_RECLINING, do_move, 0, SCMD_SW, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"sw", "sw", POS_RECLINING, do_move, 0, SCMD_SW, FALSE, ACTION_NONE, {0, 0}, NULL},
#ifdef CAMPAIGN_FR
    // we want i to default to inventory, that's why it's up here.
    {"inventory", "i", POS_DEAD, do_inventory, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"inside", "ins", POS_RECLINING, do_move, 0, SCMD_IN, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"outside", "out", POS_RECLINING, do_move, 0, SCMD_OUT, FALSE, ACTION_NONE, {0, 0}, NULL},
#endif
    /* now, the main list */

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"abilities", "abilities", POS_DEAD, do_abilities, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"abort", "abort", POS_FIGHTING, do_abort, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    //  { "acconvert", "acconvert", POS_DEAD, do_acconvert, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"account", "account", POS_DEAD, do_account, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"at", "at", POS_DEAD, do_at, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"advance", "adv", POS_DEAD, do_advance, LVL_GRSTAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"aedit", "aed", POS_DEAD, do_oasis_aedit, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"alias", "ali", POS_DEAD, do_alias, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"affects", "aff", POS_DEAD, do_affects, 0, SCMD_AFFECTS, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"afk", "afk", POS_DEAD, do_gen_tog, 0, SCMD_AFK, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"aoebombs", "aoeb", POS_RECLINING, do_gen_tog, 0, SCMD_AOE_BOMBS, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"apotheosis", "apoth", POS_STANDING, do_sorcerer_arcane_apotheosis, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"areas", "are", POS_DEAD, do_areas, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"assist", "as", POS_FIGHTING, do_assist, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"ask", "ask", POS_RECLINING, do_spec_comm, 0, SCMD_ASK, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"astat", "ast", POS_DEAD, do_astat, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"attach", "attach", POS_DEAD, do_attach, LVL_BUILDER, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"attacks", "attacks", POS_DEAD, do_attacks, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"attackqueue", "attackq", POS_DEAD, do_queue, 0, SCMD_ATTACK_QUEUE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"auction", "auct", POS_SLEEPING, do_auction, 0, SCMD_AUCTION, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"auctalk", "aucta", POS_SLEEPING, do_gen_comm, 0, SCMD_AUCTION, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"augment", "augment", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"autoexits", "autoex", POS_DEAD, do_gen_tog, 0, SCMD_AUTOEXIT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"autoassist", "autoass", POS_DEAD, do_gen_tog, 0, SCMD_AUTOASSIST, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"autocollect", "autocollect", POS_DEAD, do_gen_tog, 0, SCMD_AUTOCOLLECT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"autoconsider", "autoconsider", POS_DEAD, do_gen_tog, 0, SCMD_AUTOCONSIDER, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"autodoor", "autodoor", POS_DEAD, do_gen_tog, 0, SCMD_AUTODOOR, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"autogold", "autogold", POS_DEAD, do_gen_tog, 0, SCMD_AUTOGOLD, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"autohit", "autohit", POS_DEAD, do_gen_tog, 0, SCMD_AUTOHIT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"autokey", "autokey", POS_DEAD, do_gen_tog, 0, SCMD_AUTOKEY, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"autoloot", "autoloot", POS_DEAD, do_gen_tog, 0, SCMD_AUTOLOOT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"automap", "automap", POS_DEAD, do_gen_tog, 0, SCMD_AUTOMAP, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"autosac", "autosac", POS_DEAD, do_gen_tog, 0, SCMD_AUTOSAC, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"autoscan", "autoscan", POS_DEAD, do_gen_tog, 0, SCMD_AUTOSCAN, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"autosplit", "autospl", POS_DEAD, do_gen_tog, 0, SCMD_AUTOSPLIT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"abilityset", "abilityset", POS_SLEEPING, do_abilityset, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"autocraft", "autocraft", POS_STANDING, do_not_here, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"adjure", "adjure", POS_RESTING, do_gen_preparation, 0, SCMD_ADJURE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"autofire", "autofire", POS_FIGHTING, do_autofire, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"applyoil", "applyoil", POS_STANDING, do_applyoil, 1, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, NULL},
    {"applypoison", "applypoison", POS_STANDING, do_applypoison, 1, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, NULL},
    {"abundantstep", "abundantstep", POS_STANDING, do_abundantstep, 1, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"animatedead", "animatedead", POS_STANDING, do_animatedead, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"autoreload", "autoreload", POS_DEAD, do_gen_tog, 0, SCMD_AUTORELOAD, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"accexp", "accexp", POS_DEAD, do_accexp, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"aciddart", "aciddart", POS_FIGHTING, do_aciddart, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"auraofjustice", "auraofjustice", POS_FIGHTING, do_aura_of_justice, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_smiteevil},
    {"auraofprotection", "auraofprotection", POS_FIGHTING, do_auraofprotection, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"auraofvengeance", "auraofvengeance", POS_FIGHTING, do_aura_of_vengeance, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_smitegood},
    {"armorinfo", "armorinfo", POS_DEAD, do_armorinfo, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"armorlist", "armorlist", POS_DEAD, do_armorlist, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"armorlist2", "armorlist2", POS_DEAD, do_armorlist_old, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"arrowswarm", "arrowswarm", POS_FIGHTING, do_arrowswarm, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_arrowswarm},
    {"award", "award", POS_DEAD, do_award, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"autostand", "autostand", POS_DEAD, do_gen_tog, 0, SCMD_AUTO_STAND, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"aqref", "aqref", POS_DEAD, do_aqref, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"backstab", "ba", POS_STANDING, do_backstab, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_backstab},
    {"ban", "ban", POS_DEAD, do_ban, LVL_GRSTAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"baneweapon", "banew", POS_RECLINING, do_bane, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"bandage", "bandage", POS_FIGHTING, do_bandage, 1, 0, FALSE, ACTION_STANDARD, {0, 0}, 0},
    {"balance", "bal", POS_STANDING, do_not_here, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"bash", "bash", POS_FIGHTING, do_process_attack, 1, AA_TRIP, FALSE, ACTION_NONE, {0, 0}, can_bash},
    {"bid", "bid", POS_SLEEPING, do_bid, 0, SCMD_AUCTION, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"brief", "br", POS_DEAD, do_gen_tog, 0, SCMD_BRIEF, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"buffself", "buffself", POS_DEAD, do_buffself, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"buildwalk", "buildwalk", POS_STANDING, do_gen_tog, LVL_BUILDER, SCMD_BUILDWALK, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"buy", "bu", POS_STANDING, do_not_here, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"bug", "bug", POS_DEAD, do_ibt, 0, SCMD_BUG, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"breathe", "breathe", POS_FIGHTING, do_breathe, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_breathe},
    {"blank", "blank", POS_RECLINING, do_consign_to_oblivion, 0, SCMD_BLANK, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"blooddrain", "blooddrain", POS_RESTING, do_blood_drain, 0, 0, FALSE, ACTION_STANDARD, {0, 0}, NULL},
    {"bombs", "bombs", POS_RESTING, do_bombs, 0, 0, FALSE, ACTION_STANDARD, {0, 0}, NULL},
    {"boosts", "boost", POS_RECLINING, do_boosts, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"buck", "buck", POS_FIGHTING, do_buck, 1, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"bodyslam", "bodyslam", POS_FIGHTING, do_bodyslam, 1, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, can_bodyslam},
    {"bind", "bind", POS_FIGHTING, do_bind, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"blessedtouch", "blessedtouch", POS_STANDING, do_blessedtouch, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"battlerage", "battlerage", POS_STANDING, do_battlerage, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"bazaar", "bazaar", POS_STANDING, do_not_here, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"cast", "c", POS_SITTING, do_gen_cast, 1, SCMD_CAST_SPELL, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"carriage", "car", POS_RECLINING, do_carriage, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"catsclaws", "catsclaws", POS_FIGHTING, do_tabaxi_claw_attack, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_tabaxi_claw_attack},
    {"cedit", "cedit", POS_DEAD, do_oasis_cedit, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"chat", "chat", POS_SLEEPING, do_gen_comm, 0, SCMD_GOSSIP, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"changelog", "cha", POS_DEAD, do_changelog, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"channelenergy", "channele", POS_FIGHTING, do_channelenergy, 1, 0, TRUE, ACTION_STANDARD, {0, 0}, can_channel_energy},
    {"channelspell", "channels", POS_RECLINING, do_channelspell, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"check", "ch", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"checkload", "checkl", POS_DEAD, do_checkloadstatus, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"childrenofthenight", "children", POS_DEAD, do_children_of_the_night, 1, 0, FALSE, ACTION_STANDARD, {0, 0}, can_children_of_the_night},
    {"close", "clo", POS_SITTING, do_gen_door, 0, SCMD_CLOSE, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"clan", "cla", POS_DEAD, do_clan, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"clanset", "clans", POS_DEAD, do_clanset, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"clantalk", "cl", POS_DEAD, do_clantalk, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"clear", "cle", POS_DEAD, do_gen_ps, 0, SCMD_CLEAR, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"cls", "cls", POS_DEAD, do_gen_ps, 0, SCMD_CLEAR, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"consider", "con", POS_RECLINING, do_consider, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"condemn", "condemn", POS_RECLINING, do_gen_preparation, 0, SCMD_CONDEMN, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"corruptingtouch", "corruptingtouch", POS_FIGHTING, do_touch_of_corruption, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"commune", "commune", POS_RESTING, do_gen_preparation, 0, SCMD_COMMUNE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"commands", "com", POS_DEAD, do_commands, 0, SCMD_COMMANDS, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"compact", "comp", POS_DEAD, do_gen_tog, 0, SCMD_COMPACT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"compel", "compel", POS_RECLINING, do_gen_preparation, 0, SCMD_COMPEL, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"concoct", "conc", POS_RESTING, do_gen_preparation, 0, SCMD_CONCOCT, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"cooldowns", "coo", POS_DEAD, do_affects, 0, SCMD_COOLDOWNS, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"copyover", "copyover", POS_DEAD, do_copyover, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"copyroom", "copyroom", POS_DEAD, do_copyroom, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"credits", "cred", POS_DEAD, do_gen_ps, 0, SCMD_CREDITS, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"ct", "ct", POS_DEAD, do_clantalk, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"craftedit", "crafte", POS_DEAD, do_oasis_craftedit, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"create", "create", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"checkcraft", "checkcraft", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"compose", "compose", POS_RESTING, do_gen_preparation, 0, SCMD_COMPOSE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"cosmicawareness", "cosmicawareness", POS_RESTING, do_cosmic_awareness, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    //{ "convert", "covert", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"crystalfist", "crystalf", POS_FIGHTING, do_crystalfist, 0, 0, FALSE, ACTION_NONE, {0, 0}, can_crystalfist},
    {"crystalbody", "crystalb", POS_FIGHTING, do_crystalbody, 0, 0, FALSE, ACTION_NONE, {0, 0}, can_crystalbody},
    {"call", "call", POS_FIGHTING, do_call, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"chant", "chant", POS_RESTING, do_gen_preparation, 0, SCMD_CHANT, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"checkapproved", "checkapproved", POS_DEAD, do_checkapproved, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"charge", "charge", POS_FIGHTING, do_charge, 1, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, can_charge},
    {"circle", "circle", POS_FIGHTING, do_circle, 1, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, can_circle},
    {"collect", "collect", POS_STANDING, do_collect, 1, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    /* we are just using the old do_practice function for crafting for now */
    {"craft", "craft", POS_RECLINING, do_practice, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    /* newer crafting system */
    {"crafting", "crafting", POS_STANDING, do_craft, 0, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, NULL},
    {"comeandgetme", "comeandgetme", POS_FIGHTING, do_comeandgetme, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_comeandgetme},
    {"curingtouch", "curingtouch", POS_STANDING, do_curingtouch, 0, 0, FALSE, ACTION_SWIFT, {6, 0}, NULL},
    {"cursetouch", "cursetouch", POS_FIGHTING, do_cursetouch, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"copycat", "copycat", POS_FIGHTING, do_copycat, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"class", "class", POS_DEAD, do_class, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"combatroll", "combatroll", POS_DEAD, do_gen_tog, 0, SCMD_COMBATROLL, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"coordconvert", "coordconvert", POS_SLEEPING, do_coordconvert, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"cmdlev", "cmdlev", POS_DEAD, do_cmdlev, LVL_BUILDER, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"cexchange", "cexchange", POS_RECLINING, do_cexchange, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"createspawn", "createspawn", POS_STANDING, do_create_vampire_spawn, 1, 0, FALSE, ACTION_STANDARD, {0, 0}, can_create_vampire_spawn},
    {"cruelties", "cruelties", POS_RECLINING, do_cruelties, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"condensed", "condensed", POS_DEAD, do_gen_tog, 0, SCMD_CONDENSED, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"carefulpet", "carefulpet", POS_DEAD, do_gen_tog, 0, SCMD_CAREFUL_PET, TRUE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/
    {"date", "da", POS_DEAD, do_date, 1, SCMD_DATE, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"dc", "dc", POS_DEAD, do_dc, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"deletepath", "deletepath", POS_DEAD, do_deletepath, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"deposit", "depo", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"detach", "detach", POS_DEAD, do_detach, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
#ifdef CAMPAIGN_FR
    {"deity", "deity", POS_DEAD, do_devote, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"devote", "devote", POS_DEAD, do_devote, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
#endif
    {"diagnose", "diag", POS_RECLINING, do_diagnose, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"dig", "dig", POS_DEAD, do_dig, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"discard", "discard", POS_RECLINING, do_consign_to_oblivion, 0, SCMD_DISCARD, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"discharge", "discharge", POS_RECLINING, do_discharge, 0, 0, FALSE, ACTION_STANDARD, {0, 0}, NULL},
    {"discoveries", "discov", POS_RECLINING, do_discoveries, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"disengage", "disen", POS_STANDING, do_disengage, 1, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"display", "disp", POS_DEAD, do_display, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"divinebond", "divineb", POS_DEAD, do_divine_bond, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"dominate", "dominate", POS_FIGHTING, do_vampiric_dominate, 1, 0, FALSE, ACTION_STANDARD, {0, 0}, can_vampiric_dominate},
    {"donate", "don", POS_RECLINING, do_drop, 0, SCMD_DONATE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"doublemanifest", "doublemanifest", POS_FIGHTING, do_double_manifest, 1, 0, FALSE, ACTION_SWIFT, {0, 0}, can_double_manifest},
    {"drink", "dri", POS_RECLINING, do_drink, 0, SCMD_DRINK, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"drop", "dro", POS_RECLINING, do_drop, 0, SCMD_DROP, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"dismount", "dismount", POS_FIGHTING, do_dismount, 0, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"dismiss", "dismiss", POS_FIGHTING, do_dismiss, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"disenchant", "disenchant", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"dirtkick", "dirtkick", POS_FIGHTING, do_dirtkick, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_dirtkick},
    {"disembark", "disembark", POS_STANDING, do_disembark, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"detecttrap", "detecttrap", POS_STANDING, do_detecttrap, 1, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, NULL},
    {"disabletrap", "disabletrap", POS_STANDING, do_disabletrap, 1, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, NULL},
    {"disarm", "disarm", POS_FIGHTING, do_process_attack, 1, AA_DISARM, FALSE, ACTION_NONE, {0, 0}, can_disarm},
    {"domain", "domain", POS_DEAD, do_domain, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"disguise", "disguise", POS_RESTING, do_disguise, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"destructiveaura", "destructiveaura", POS_FIGHTING, do_destructiveaura, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"destructivesmite", "destructivesmite", POS_FIGHTING, do_destructivesmite, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"deadlyaim", "deadlyaim", POS_FIGHTING, do_mode, 1, MODE_DEADLY_AIM, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"deatharrow", "deatharrow", POS_FIGHTING, do_deatharrow, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_deatharrow},
    {"defenses", "defenses", POS_DEAD, do_defenses, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"defensivestance", "defensivestance", POS_FIGHTING, do_defensive_stance, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_defensive_stance},
    {"darkness", "darkness", POS_FIGHTING, do_darkness, 1, 0, FALSE, ACTION_MOVE, {0, 0}, NULL},
    {"dracbreath", "dracbreath", POS_FIGHTING, do_sorcerer_breath_weapon, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_sorcerer_breath_weapon},
    {"dracclaws", "dracclaws", POS_FIGHTING, do_sorcerer_claw_attack, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_sorcerer_claw_attack},
    {"dracwings", "dracwings", POS_FIGHTING, do_sorcerer_draconic_wings, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"dragbreath", "dragbreath", POS_FIGHTING, do_dragonborn_breath_weapon, 1, 0, FALSE, ACTION_STANDARD, {0, 0}, can_dragonborn_breath_weapon},
    {"dragonfear", "dragonfear", POS_FIGHTING, do_dragonfear, 1, 0, FALSE, ACTION_SWIFT, {6, 0}, can_dragonfear},
    {"dragonmagic", "dragonmagic", POS_FIGHTING, do_dragonmagic, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_dragonmagic},
    {"damage", "damage", POS_DEAD, do_damage, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"dice", "dice", POS_RECLINING, do_dice, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"eat", "ea", POS_RECLINING, do_eat, 0, SCMD_EAT, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"echo", "ec", POS_SLEEPING, do_echo, LVL_IMMORT, SCMD_ECHO, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"efreetimagic", "efreetimagic", POS_FIGHTING, do_efreetimagic, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_efreetimagic},
    {"emote", "em", POS_RECLINING, do_echo, 0, SCMD_EMOTE, TRUE, ACTION_NONE, {0, 0}, NULL},
    {":", ":", POS_RECLINING, do_echo, 1, SCMD_EMOTE, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"enter", "ent", POS_STANDING, do_enter, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"encounter", "enc", POS_RECLINING, do_encounter, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"encounterinfo", "encounterinfo", POS_DEAD, do_encounterinfo, LVL_IMMORT, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"enlarge", "enlarge", POS_FIGHTING, do_enlarge, 1, 0, FALSE, ACTION_MOVE, {0, 0}, NULL},
    {"equipment", "eq", POS_SLEEPING, do_equipment, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"eqstats", "eqst", POS_SLEEPING, do_not_here, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"exits", "ex", POS_RECLINING, do_exits, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"examine", "exa", POS_RECLINING, do_examine, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"exchange", "exch", POS_RECLINING, do_not_here, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"expertise", "expertise", POS_FIGHTING, do_mode, 1, MODE_COMBAT_EXPERTISE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"export", "export", POS_DEAD, do_export_zone, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"eqrating", "eqrating", POS_SLEEPING, do_eqrating, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"emptybody", "emptybody", POS_RECLINING, do_emptybody, 1, 0, FALSE, ACTION_MOVE, {0, 6}, can_emptybody},
    {"ethshift", "ethshift", POS_STANDING, do_ethshift, 0, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, NULL},
    {"evilscythe", "evilscythe", POS_STANDING, do_evilscythe, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"eviltouch", "eviltouch", POS_STANDING, do_eviltouch, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"extracts", "extracts", POS_RECLINING, do_spells, 1, SCMD_CONCOCT, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"extractlist", "extractlist", POS_RECLINING, do_spelllist, 1, SCMD_CONCOCT, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"eyeofknowledge", "eyeofknowledge", POS_STANDING, do_eyeofknowledge, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"exempt", "exempt", POS_RECLINING, do_consign_to_oblivion, 0, SCMD_EXEMPT, FALSE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/
    {"feats", "fea", POS_SLEEPING, do_feats, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"featlisting", "featlisting", POS_SLEEPING, do_featlisting, LVL_IMMORT, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"force", "force", POS_SLEEPING, do_force, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"fiendishboon", "fiendishb", POS_DEAD, do_fiendishboon, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"fill", "fil", POS_STANDING, do_pour, 0, SCMD_FILL, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"file", "file", POS_SLEEPING, do_file, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"flee", "fl", POS_FIGHTING, do_flee, 1, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"flightlist", "flightlist", POS_RESTING, do_flightlist, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"follow", "fol", POS_RECLINING, do_follow, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"forget", "forget", POS_RECLINING, do_consign_to_oblivion, 0, SCMD_FORGET, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"freeze", "freeze", POS_DEAD, do_wizutil, LVL_GRSTAFF, SCMD_FREEZE, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"frightful", "frightful", POS_FIGHTING, do_dragonfear, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"fly", "fly", POS_FIGHTING, do_fly, 1, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"fire", "fire", POS_FIGHTING, do_fire, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"flurryofblows", "flurryofblows", POS_FIGHTING, do_mode, 1, MODE_FLURRY_OF_BLOWS, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"feint", "feint", POS_FIGHTING, do_process_attack, 1, AA_FEINT, FALSE, ACTION_SWIFT, {0, 0}, can_feint},
    {"firebolt", "firebolt", POS_FIGHTING, do_firebolt, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"freegrapple", "freegrapple", POS_RECLINING, do_free_grapple, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"faeriefire", "faeriefire", POS_FIGHTING, do_process_attack, 1, AA_FAERIE_FIRE, FALSE, ACTION_NONE, {0, 0}, can_faeriefire},
    {"favoredenemies", "favoredenemies", POS_DEAD, do_favoredenemies, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"feymagic", "feymagic", POS_FIGHTING, do_fey_magic, 1, 0, FALSE, ACTION_SWIFT, {0, 0}, can_fey_magic},
    {"findmagic", "findmagic", POS_DEAD, do_findmagic, LVL_BUILDER, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"finddoor", "finddoor", POS_DEAD, do_finddoor, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"featset", "featset", POS_SLEEPING, do_featset, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"get", "g", POS_RECLINING, do_get, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"gdiscovery", "gdisc", POS_RECLINING, do_grand_discoveries, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"gecho", "gecho", POS_DEAD, do_gecho, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"gemote", "gem", POS_SLEEPING, do_gen_comm, 0, SCMD_GEMOTE, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"genmap", "genmap", POS_SLEEPING, do_genmap, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"genriver", "genriver", POS_SLEEPING, do_genriver, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"give", "giv", POS_RECLINING, do_give, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"goto", "go", POS_SLEEPING, do_goto, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"gold", "gol", POS_RECLINING, do_gold, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},

    {"gossip", "gos", POS_SLEEPING, do_gen_comm, 0, SCMD_GOSSIP, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"group", "gr", POS_RECLINING, do_group, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"grab", "grab", POS_RECLINING, do_grab, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"grats", "grat", POS_SLEEPING, do_gen_comm, 0, SCMD_GRATZ, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"greport", "grepo", POS_RECLINING, do_greport, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"gsay", "gsay", POS_SLEEPING, do_gsay, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"gtell", "gt", POS_SLEEPING, do_gsay, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"gain", "gain", POS_RECLINING, do_gain, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"gaseousform", "gaseous", POS_FIGHTING, do_vampiric_gaseous_form, 1, 0, FALSE, ACTION_STANDARD, {0, 0}, can_vampiric_gaseous_form},
    {"guard", "guard", POS_FIGHTING, do_guard, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_guard},
    {"grapple", "grapple", POS_RECLINING, do_grapple, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"gravemagic", "grave", POS_FIGHTING, do_grave_magic, 1, 0, FALSE, ACTION_MOVE, {6, 0}, NULL},
    {"goodlance", "goodlance", POS_STANDING, do_goodlance, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"goodtouch", "goodtouch", POS_STANDING, do_goodtouch, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"guimode", "guimode", POS_DEAD, do_gen_tog, 1, SCMD_GUI_MODE, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"glore", "glore", POS_RESTING, do_glore, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"help", "h", POS_DEAD, do_help, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"happyhour", "ha", POS_DEAD, do_happyhour, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"here", "here", POS_RECLINING, do_look, 0, SCMD_HERE, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"hedit", "hedit", POS_DEAD, do_oasis_hedit, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"helpcheck", "helpch", POS_DEAD, do_helpcheck, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"hide", "hi", POS_RECLINING, do_hide, 1, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"hindex", "hind", POS_DEAD, do_hindex, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"handbook", "handb", POS_DEAD, do_gen_ps, LVL_IMMORT, SCMD_HANDBOOK, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"hcontrol", "hcontrol", POS_DEAD, do_hcontrol, LVL_GRSTAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"history", "history", POS_DEAD, do_history, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"hit", "hit", POS_FIGHTING, do_hit, 0, SCMD_HIT, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"hold", "hold", POS_RECLINING, do_grab, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"holler", "holler", POS_RECLINING, do_gen_comm, 1, SCMD_HOLLER, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"holylight", "holy", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_HOLYLIGHT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"holyweapon", "holyw", POS_DEAD, do_holyweapon, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"house", "house", POS_RECLINING, do_house, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"harvest", "harvest", POS_STANDING, do_harvest, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"hlqedit", "hlqedit", POS_DEAD, do_hlqedit, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"hlqlist", "hlqlist", POS_DEAD, do_hlqlist, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"headbutt", "headbutt", POS_FIGHTING, do_process_attack, 1, AA_HEADBUTT, FALSE, ACTION_NONE, {0, 0}, can_headbutt},
    {"hitall", "hitall", POS_FIGHTING, do_hitall, 1, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, NULL},
    {"handleanimal", "handleanimal", POS_STANDING, do_handleanimal, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"healingtouch", "healingtouch", POS_STANDING, do_healingtouch, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"hp", "hp", POS_DEAD, do_hp, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"hsedit", "hsedit", POS_DEAD, do_oasis_hsedit, LVL_GRSTAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"hunts", "hunts", POS_RECLINING, do_not_here, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"hsort", "hsort", POS_RECLINING, do_hsort, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

#ifndef CAMPAIGN_FR
    {"inventory", "i", POS_DEAD, do_inventory, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
#endif
    {"identify", "id", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"idea", "ide", POS_DEAD, do_ibt, 0, SCMD_IDEA, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"iedit", "iedit", POS_DEAD, do_iedit, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"imbibe", "imb", POS_SITTING, do_gen_cast, 1, SCMD_CAST_EXTRACT, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"imotd", "imo", POS_DEAD, do_gen_ps, LVL_IMMORT, SCMD_IMOTD, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"immlist", "imm", POS_DEAD, do_gen_ps, 0, SCMD_IMMLIST, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"info", "info", POS_SLEEPING, do_gen_ps, 0, SCMD_INFO, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"invis", "invi", POS_DEAD, do_invis, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"invisduergar", "invisduergar", POS_FIGHTING, do_invisduergar, 1, 0, FALSE, ACTION_MOVE, {0, 0}, NULL},
    {"innates", "innates", POS_DEAD, do_innates, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"intimidate", "intimidate", POS_FIGHTING, do_intimidate, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_intimidate},
    {"icicle", "icicle", POS_FIGHTING, do_icicle, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"imbuearrow", "imbuearrow", POS_FIGHTING, do_imbuearrow, 1, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"impromptu", "impromptu", POS_FIGHTING, do_impromptu, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_impromptu},
    {"invisiblerogue", "invisiblerogue", POS_DEAD, do_invisiblerogue, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"innerfire", "innerfire", POS_FIGHTING, do_innerfire, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"invoke", "invoke", POS_FIGHTING, do_use_consumable, 0, SCMD_INVOKE, FALSE, ACTION_SWIFT, {0, 6}, NULL},
    {"insectbeing", "insectbeing", POS_FIGHTING, do_insectbeing, 0, 0, FALSE, ACTION_NONE, {0, 0}, can_insectbeing},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"junk", "j", POS_RECLINING, do_drop, 0, SCMD_JUNK, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"judgement", "judge", POS_RECLINING, do_judgement, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"kill", "k", POS_FIGHTING, do_kill, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"kick", "ki", POS_FIGHTING, do_process_attack, 1, AA_KICK, FALSE, ACTION_NONE, {6, 0}, can_kick},
    {"keycheck", "keycheck", POS_STANDING, do_keycheck, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"look", "l", POS_RECLINING, do_look, 0, SCMD_LOOK, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"list", "lis", POS_STANDING, do_not_here, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"lore", "lore", POS_RESTING, do_lore, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"land", "land", POS_FIGHTING, do_land, 1, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"landmarks", "landmarks", POS_DEAD, do_landmarks, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"last", "last", POS_DEAD, do_last, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"layonhands", "layonhands", POS_FIGHTING, do_layonhands, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_layonhands},
    {"loot", "loot", POS_STANDING, do_loot, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"leave", "lea", POS_STANDING, do_leave, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"levels", "lev", POS_DEAD, do_levels, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"listen", "listen", POS_STANDING, do_listen, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"links", "lin", POS_STANDING, do_links, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"lock", "loc", POS_SITTING, do_gen_door, 0, SCMD_LOCK, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"load", "load", POS_DEAD, do_load, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"loadmagic", "loadmagic", POS_DEAD, do_loadmagic, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"loadmagicspecific", "loadmagicspecific", POS_DEAD, do_loadmagicspecific, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"lightningarc", "lightningarc", POS_FIGHTING, do_lightningarc, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"levitate", "levitate", POS_FIGHTING, do_levitate, 1, 0, FALSE, ACTION_MOVE, {0, 0}, NULL},
    {"lms", "lms", POS_DEAD, do_loadmagicspecific, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"lichfear", "lichfear", POS_FIGHTING, do_lichfear, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"lichtouch", "lichtouch", POS_FIGHTING, do_lichtouch, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"manifest", "m", POS_SITTING, do_manifest, 1, SCMD_CAST_PSIONIC, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"maneuvers", "maneuvers", POS_DEAD, do_commands, 0, SCMD_MANEUVERS, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"memorize", "memorize", POS_RESTING, do_gen_preparation, 0, SCMD_MEMORIZE, FALSE, ACTION_NONE, {0, 0}, NULL},
    //  { "mail", "mail", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"mail", "mail", POS_STANDING, do_new_mail, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"map", "map", POS_STANDING, do_map, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"mark", "mark", POS_STANDING, do_mark, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"mastermind", "mastermind", POS_FIGHTING, do_mastermind, 1, 0, FALSE, ACTION_SWIFT, {0, 0}, can_mastermind},
    {"maxhp", "maxhp", POS_DEAD, do_maxhp, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"medit", "med", POS_DEAD, do_oasis_medit, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"meditate", "meditate", POS_RESTING, do_gen_preparation, 0, SCMD_MEDITATE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"mercies", "mercies", POS_DEAD, do_mercies, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"mission", "mission", POS_RESTING, do_missions, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"mlist", "mlist", POS_DEAD, do_oasis_list, LVL_BUILDER, SCMD_OASIS_MLIST, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"mcopy", "mcopy", POS_DEAD, do_oasis_copy, LVL_STAFF, CON_MEDIT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"motd", "motd", POS_DEAD, do_gen_ps, 0, SCMD_MOTD, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"msgedit", "msgedit", POS_DEAD, do_msgedit, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"mute", "mute", POS_DEAD, do_wizutil, LVL_STAFF, SCMD_MUTE, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"mount", "mount", POS_FIGHTING, do_mount, 0, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"masterlist", "masterlist", POS_DEAD, do_masterlist, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"massinvis", "massinvis", POS_FIGHTING, do_massinvis, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"moves", "moves", POS_DEAD, do_moves, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"news", "news", POS_SLEEPING, do_gen_ps, 0, SCMD_NEWS, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"noauction", "noauction", POS_DEAD, do_gen_tog, 0, SCMD_NOAUCTION, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"noclantalk", "noclant", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_NOCLANTALK, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"nogossip", "nogossip", POS_DEAD, do_gen_tog, 0, SCMD_NOGOSSIP, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"nograts", "nograts", POS_DEAD, do_gen_tog, 0, SCMD_NOGRATZ, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"nohassle", "nohassle", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_NOHASSLE, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"norage", "norage", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_NORAGE, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"norepeat", "norepeat", POS_DEAD, do_gen_tog, 0, SCMD_NOREPEAT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"noshout", "noshout", POS_SLEEPING, do_gen_tog, 1, SCMD_NOSHOUT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"nosummon", "nosummon", POS_DEAD, do_gen_tog, 1, SCMD_NOSUMMON, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"notell", "notell", POS_DEAD, do_gen_tog, 1, SCMD_NOTELL, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"notitle", "notitle", POS_DEAD, do_wizutil, LVL_STAFF, SCMD_NOTITLE, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"nowiz", "nowiz", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_NOWIZ, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"nop", "nop", POS_DEAD, do_nop, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"nochat", "nochat", POS_DEAD, do_gen_tog, 0, SCMD_NOGOSSIP, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"nohint", "nohint", POS_DEAD, do_gen_tog, 0, SCMD_NOHINT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"nocharmierescues", "nocharmierescues", POS_DEAD, do_gen_tog, 0, SCMD_NOCHARMIERESCUES, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"nofollow", "nofollow", POS_DEAD, do_gen_tog, 0, SCMD_NO_FOLLOW, TRUE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"open", "o", POS_SITTING, do_gen_door, 0, SCMD_OPEN, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"oconvert", "oconvert", POS_DEAD, do_oconvert, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"order", "ord", POS_RECLINING, do_order, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"offer", "off", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"olc", "olc", POS_DEAD, do_show_save_list, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"olist", "olist", POS_DEAD, do_oasis_list, LVL_BUILDER, SCMD_OASIS_OLIST, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"oedit", "oedit", POS_DEAD, do_oasis_oedit, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"oset", "oset", POS_DEAD, do_oset, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"ocopy", "ocopy", POS_DEAD, do_oasis_copy, LVL_STAFF, CON_OEDIT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"omit", "omit", POS_RECLINING, do_consign_to_oblivion, 0, SCMD_OMIT, FALSE, ACTION_NONE, {0, 0}, NULL},
    //{ "objlist", "objlist", POS_DEAD, do_objlist, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"obind", "obind", POS_DEAD, do_obind, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"osay", "osay", POS_RECLINING, do_osay, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"ooc", "ooc", POS_RECLINING, do_osay, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"outfit", "outfit", POS_RECLINING, do_outfit, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"pathlist", "pathlist", POS_DEAD, do_oasis_list, LVL_BUILDER, SCMD_OASIS_PATHLIST, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"put", "p", POS_RECLINING, do_put, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"peace", "pe", POS_DEAD, do_peace, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"pick", "pi", POS_STANDING, do_gen_door, 1, SCMD_PICK, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"pilfer", "pil", POS_STANDING, do_loot, 1, SCMD_PILFER, FALSE, ACTION_STANDARD, {0, 0}, NULL},
    {"practice", "pr", POS_RECLINING, do_practice, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"page", "pag", POS_DEAD, do_page, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"pardon", "pardon", POS_DEAD, do_wizutil, LVL_STAFF, SCMD_PARDON, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"plist", "plist", POS_DEAD, do_plist, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"poisonbreath", "poisonb", POS_FIGHTING, do_poisonbreath, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_poisonbreath},
    {"poisontouch", "poisont", POS_FIGHTING, do_poisontouch, 1, 0, FALSE, ACTION_STANDARD, {0, 0}, NULL},
    {"policy", "pol", POS_DEAD, do_gen_ps, 0, SCMD_POLICIES, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"pour", "pour", POS_STANDING, do_pour, 0, SCMD_POUR, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"powers", "powers", POS_RECLINING, do_spells, 1, SCMD_POWERS, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"powerslist", "powerslist", POS_RECLINING, do_spelllist, 1, SCMD_POWERS, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"powerattack", "powerattack", POS_FIGHTING, do_mode, 1, MODE_POWER_ATTACK, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"prompt", "pro", POS_DEAD, do_display, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"prefedit", "pre", POS_DEAD, do_oasis_prefedit, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"purify", "purify", POS_FIGHTING, do_purify, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"purge", "purge", POS_DEAD, do_purge, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"prayer", "prayer", POS_RECLINING, do_gen_preparation, 0, SCMD_PRAY, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"perform", "perform", POS_FIGHTING, do_perform, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"powerfulblow", "powerfulblow", POS_FIGHTING, do_powerfulblow, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_powerfulblow},
    {"pin", "pin", POS_FIGHTING, do_pin, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"pixiedust", "pixiedust", POS_RECLINING, do_pixiedust, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_pixiedust},
    {"pixieinvis", "pixieinvis", POS_RECLINING, do_pixieinvis, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_pixieinvis},
    {"players", "players", POS_DEAD, do_players, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    //{ "play", "play", POS_FIGHTING, do_play, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"psionicfocus", "psionicfocus", POS_FIGHTING, do_psionic_focus, 1, 0, FALSE, ACTION_SWIFT, {0, 0}, can_psionic_focus},
    {"psychokinetic", "psychokinetic", POS_FIGHTING, do_psychokinetic, 1, 0, FALSE, ACTION_STANDARD, {0, 0}, NULL},
    {"push", "push", POS_STANDING, do_pullswitch, 0, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"pull", "pull", POS_STANDING, do_pullswitch, 0, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    //{ "plist", "plist", POS_DEAD, do_plist, LVL_GRSTAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"perfmon", "perfmon", POS_DEAD, do_perfmon, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"priceset", "priceset", POS_RECLINING, do_priceset, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"pets", "pets", POS_RECLINING, do_pets, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"queue", "q", POS_DEAD, do_queue, 0, SCMD_ACTION_QUEUE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"qedit", "qedit", POS_DEAD, do_oasis_qedit, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"qlist", "qlist", POS_DEAD, do_oasis_list, LVL_BUILDER, SCMD_OASIS_QLIST, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"quaff", "qua", POS_FIGHTING, do_use_consumable, 0, SCMD_QUAFF, FALSE, ACTION_SWIFT, {0, 6}, NULL},
    {"qecho", "qec", POS_DEAD, do_qcomm, LVL_STAFF, SCMD_QECHO, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"quest", "que", POS_DEAD, do_quest, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"qui", "qui", POS_DEAD, do_quit, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
#ifdef CAMPAIGN_FR
    {"quickchant", "quickc", POS_FIGHTING, do_quick_chant, 0, SCMD_QUICK_CHANT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"quickmind", "quickm", POS_FIGHTING, do_quick_chant, 0, SCMD_QUICK_MIND, TRUE, ACTION_NONE, {0, 0}, NULL},
#endif
    {"quit", "quit", POS_DEAD, do_quit, 0, SCMD_QUIT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"qsay", "qsay", POS_RECLINING, do_qcomm, 0, SCMD_QSAY, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"qref", "qref", POS_DEAD, do_qref, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"qview", "qview", POS_DEAD, do_qview, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"quiveringpalm", "quiveringpalm", POS_FIGHTING, do_quiveringpalm, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_quiveringpalm},
    {"qcopy", "qcopy", POS_DEAD, do_oasis_copy, LVL_STAFF, CON_QEDIT, TRUE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"rest", "re", POS_RECLINING, do_rest, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"reply", "r", POS_SLEEPING, do_reply, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"rapidshot", "rapidshot", POS_FIGHTING, do_mode, 1, MODE_RAPID_SHOT, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"read", "rea", POS_RECLINING, do_look, 0, SCMD_READ, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"reload", "reload", POS_FIGHTING, do_reload, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"reloadimm", "reloadimm", POS_DEAD, do_reboot, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"recite", "reci", POS_FIGHTING, do_use_consumable, 0, SCMD_RECITE, FALSE, ACTION_SWIFT, {0, 6}, NULL},
    {"receive", "rece", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"recent", "recent", POS_DEAD, do_recent, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"remove", "rem", POS_RESTING, do_remove, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"rent", "rent", POS_STANDING, do_not_here, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"report", "repo", POS_RECLINING, do_report, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"reroll", "rero", POS_DEAD, do_wizutil, LVL_GRSTAFF, SCMD_REROLL, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"rescue", "resc", POS_FIGHTING, do_rescue, 1, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, can_rescue},
    {"resistances", "res", POS_DEAD, do_affects, 0, SCMD_RESISTANCES, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"restore", "resto", POS_DEAD, do_restore, LVL_GRSTAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"return", "retu", POS_DEAD, do_return, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"redit", "redit", POS_DEAD, do_oasis_redit, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"reglist", "reglist", POS_DEAD, do_oasis_list, LVL_BUILDER, SCMD_OASIS_REGLIST, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"revoke", "revoke", POS_RECLINING, do_revoke, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"rlist", "rlist", POS_DEAD, do_oasis_list, LVL_BUILDER, SCMD_OASIS_RLIST, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"rcopy", "rcopy", POS_DEAD, do_oasis_copy, LVL_STAFF, CON_REDIT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"roomflags", "roomflags", POS_DEAD, do_gen_tog, LVL_IMMORT, SCMD_SHOWVNUMS, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"respec", "respec", POS_STANDING, do_respec, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"recharge", "recharge", POS_STANDING, do_recharge, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"resize", "resize", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"restring", "restring", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"rage", "rage", POS_FIGHTING, do_rage, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_rage},
    {"recline", "recline", POS_SLEEPING, do_recline, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"renewedvigor", "renewedvigor", POS_RECLINING, do_renewedvigor, 1, 0, FALSE, ACTION_SWIFT, {0, 0}, can_renewedvigor},
    {"research", "research", POS_STANDING, do_not_here, 0, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, NULL},
    {"releasegrapple", "releasegrapple", POS_FIGHTING, do_free_grapple, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"reneweddefense", "reneweddefense", POS_RECLINING, do_reneweddefense, 1, 0, FALSE, ACTION_SWIFT, {0, 0}, can_reneweddefense},
    {"races", "races", POS_DEAD, do_race, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"racefix", "racefix", POS_DEAD, do_racefix, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"rank", "rank", POS_DEAD, do_rank, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"rp", "rp", POS_DEAD, do_gen_tog, 0, SCMD_RP, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"resetpassword", "resetpassword", POS_DEAD, do_resetpassword, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"rsay", "rs", POS_RECLINING, do_rsay, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"sacrifice", "sac", POS_RECLINING, do_sac, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
#ifdef CAMPAIGN_FR
    {"say", "s", POS_RECLINING, do_osay, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
#else
    {"say", "s", POS_RECLINING, do_say, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
#endif
    {"sail", "sail", POS_RECLINING, do_sail, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"score", "sc", POS_DEAD, do_score, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"scan", "sca", POS_RECLINING, do_scan, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"scopy", "scopy", POS_DEAD, do_oasis_copy, LVL_STAFF, CON_SEDIT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"scribe", "scribe", POS_RESTING, do_scribe, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"sit", "si", POS_RECLINING, do_sit, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"'", "'", POS_RECLINING, do_say, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"save", "sav", POS_SLEEPING, do_save, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"saveall", "saveall", POS_DEAD, do_saveall, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    //  { "savemobs"  , "savemobs" , POS_DEAD    , do_savemobs  , LVL_IMPL, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"search", "sea", POS_STANDING, do_search, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"sell", "sell", POS_STANDING, do_not_here, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"sedit", "sedit", POS_DEAD, do_oasis_sedit, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"send", "send", POS_SLEEPING, do_send, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"set", "set", POS_DEAD, do_set, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"setcloak", "setcloak", POS_RECLINING, do_not_here, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"setbaneweapon", "setb", POS_RECLINING, do_setbaneweapon, 0, SCMD_SHOUT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"setroomname", "setr", POS_DEAD, do_setroomname, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"setroomdesc", "setroomd", POS_DEAD, do_setroomdesc, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"setroomsect", "setrooms", POS_DEAD, do_setroomsect, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"setworldsect", "setw", POS_DEAD, do_setworldsect, LVL_GRSTAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"shadowcast", "shc", POS_SITTING, do_gen_cast, 1, SCMD_CAST_SHADOW, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"shout", "sho", POS_RECLINING, do_gen_comm, 0, SCMD_SHOUT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"show", "show", POS_DEAD, do_show, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"showwearoff", "showwearoff", POS_DEAD, do_showwearoff, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"shutdow", "shutdow", POS_DEAD, do_shutdown, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"shutdown", "shutdown", POS_DEAD, do_shutdown, LVL_IMPL, SCMD_SHUTDOWN, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"sip", "sip", POS_RECLINING, do_drink, 0, SCMD_SIP, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"skillset", "skillset", POS_SLEEPING, do_skillset, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"sleep", "sl", POS_SLEEPING, do_sleep, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"slist", "slist", POS_SLEEPING, do_oasis_list, LVL_BUILDER, SCMD_OASIS_SLIST, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"smashdefense", "smashdefense", POS_RESTING, do_gen_tog, 1, SCMD_SMASH_DEFENSE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"smiteevil", "smiteevil", POS_FIGHTING, do_smiteevil, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_smiteevil},
    {"smitegood", "smitegood", POS_FIGHTING, do_smitegood, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_smitegood},
    {"sneak", "sneak", POS_STANDING, do_sneak, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"snoop", "snoop", POS_DEAD, do_snoop, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"socials", "socials", POS_DEAD, do_commands, 0, SCMD_SOCIALS, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"speak", "speak", POS_RECLINING, do_speak, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"spelllist", "spelllist", POS_RECLINING, do_spelllist, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"spells", "spells", POS_RECLINING, do_spells, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"split", "split", POS_SITTING, do_split, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"spot", "spot", POS_STANDING, do_spot, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"stand", "st", POS_RECLINING, do_stand, 0, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"stat", "stat", POS_DEAD, do_stat, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"steal", "ste", POS_STANDING, do_steal, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"stonesendurance", "stonese", POS_FIGHTING, do_stones_endurance, 0, 0, TRUE, ACTION_NONE, {0, 0}, can_stones_endurance},
    {"store", "store", POS_FIGHTING, do_store, 1, 0, FALSE, ACTION_MOVE, {0, 0}, NULL},
    {"stunningfist", "stunningfist", POS_FIGHTING, do_stunningfist, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_stunningfist},
    {"study", "study", POS_RECLINING, do_study, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"strength", "strength", POS_FIGHTING, do_strength, 1, 0, FALSE, ACTION_MOVE, {0, 0}, NULL},
    {"swallow", "swallow", POS_RECLINING, do_swallow, 1, 0, FALSE, ACTION_STANDARD, {0, 0}, NULL},
    {"switch", "switch", POS_DEAD, do_switch, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"shapechange", "shapechange", POS_FIGHTING, do_wildshape, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"skills", "skills", POS_RECLINING, do_train, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"supplyorder", "supplyorder", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"spellbattle", "spellbattle", POS_STANDING, do_spellbattle, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"spellquests", "spellquests", POS_DEAD, do_spellquests, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"singlefile", "singlefile", POS_DEAD, do_singlefile, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"sap", "sap", POS_FIGHTING, do_sap, 1, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, can_sap},
    {"shieldpunch", "shieldp", POS_FIGHTING, do_process_attack, 1, AA_SHIELDPUNCH, FALSE, ACTION_NONE, {0, 0}, can_shieldpunch},
    {"shieldcharge", "shieldc", POS_FIGHTING, do_shieldcharge, 1, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, can_shieldcharge},
    {"shieldslam", "shields", POS_FIGHTING, do_shieldslam, 1, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, can_shieldslam},
    {"slam", "slam", POS_FIGHTING, do_slam, 1, 0, FALSE, ACTION_STANDARD, {0, 0}, can_slam},
    {"slayer", "slay", POS_RECLINING, do_slayer, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"springleap", "springleap", POS_RECLINING, do_springleap, 1, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"surpriseaccuracy", "surpriseaccuracy", POS_FIGHTING, do_surpriseaccuracy, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_surpriseaccuracy},
    {"struggle", "struggle", POS_RECLINING, do_struggle, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"seekerarrow", "seekerarrow", POS_FIGHTING, do_seekerarrow, 1, 0, FALSE, ACTION_NONE, {0, 0}, can_seekerarrow},
    {"survey", "survey", POS_RECLINING, do_survey, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"sacredflames", "sacredflames", POS_FIGHTING, do_sacredflames, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"staffevents", "staffevents", POS_SLEEPING, do_staffevents, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"summon", "summon", POS_RECLINING, do_summon, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"statcap", "statcap", POS_RECLINING, do_statcap, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"tell", "t", POS_DEAD, do_tell, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"take", "ta", POS_RECLINING, do_get, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"tailspikes", "tailspikes", POS_FIGHTING, do_tailspikes, 1, 0, FALSE, ACTION_SWIFT, {6, 0}, can_tailspikes},
    {"taste", "tas", POS_RECLINING, do_eat, 0, SCMD_TASTE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"taunt", "taunt", POS_FIGHTING, do_taunt, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_taunt},
    {"teleport", "tele", POS_DEAD, do_teleport, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"tedit", "tedit", POS_DEAD, do_tedit, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL}, /* XXX: Oasisify */
    {"thaw", "thaw", POS_DEAD, do_wizutil, LVL_GRSTAFF, SCMD_THAW, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"title", "title", POS_DEAD, do_title, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"time", "time", POS_DEAD, do_time, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"tinker", "tinker", POS_STANDING, do_tinker, 0, 0, TRUE, ACTION_NONE, {0, 0}, can_tinker},
    {"toggle", "toggle", POS_DEAD, do_toggle, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"track", "track", POS_STANDING, do_track, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"train", "tr", POS_RECLINING, do_train, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"transfer", "transfer", POS_SLEEPING, do_trans, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"treatinjury", "treatinjury", POS_RECLINING, do_treatinjury, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_treatinjury},
    {"trip", "trip", POS_FIGHTING, do_process_attack, 1, AA_TRIP, FALSE, ACTION_NONE, {0, 0}, can_trip},
    //  { "_trip", "_trip", POS_FIGHTING, do_trip, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"trigedit", "trigedit", POS_DEAD, do_oasis_trigedit, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"truejudgement", "truejudge", POS_FIGHTING, do_true_judgement, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"turnundead", "turnundead", POS_FIGHTING, do_turnundead, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_turnundead},
    {"typo", "typo", POS_DEAD, do_ibt, 0, SCMD_TYPO, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"tlist", "tlist", POS_DEAD, do_oasis_list, LVL_BUILDER, SCMD_OASIS_TLIST, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"tcopy", "tcopy", POS_DEAD, do_oasis_copy, LVL_STAFF, CON_TRIGEDIT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"tstat", "tstat", POS_DEAD, do_tstat, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"tailsweep", "tailsweep", POS_FIGHTING, do_tailsweep, 1, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, can_tailsweep},
    {"tame", "tame", POS_FIGHTING, do_tame, 0, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, NULL},
    {"totaldefense", "totaldefense", POS_FIGHTING, do_mode, 1, MODE_TOTAL_DEFENSE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"tnl", "tnl", POS_DEAD, do_tnl, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"todo", "todo", POS_DEAD, do_todo, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"unholyweapon", "unholyw", POS_DEAD, do_holyweapon, 1, 1, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"unlock", "unlock", POS_SITTING, do_gen_door, 0, SCMD_UNLOCK, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"unban", "unban", POS_DEAD, do_unban, LVL_GRSTAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"unaffect", "unaffect", POS_DEAD, do_wizutil, LVL_STAFF, SCMD_UNAFFECT, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"uncommune", "uncommune", POS_RECLINING, do_consign_to_oblivion, 0, SCMD_UNCOMMUNE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"unstore", "unstore", POS_FIGHTING, do_unstore, 1, 0, FALSE, ACTION_MOVE, {0, 0}, NULL},
    {"uptime", "uptime", POS_DEAD, do_date, 1, SCMD_UPTIME, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"use", "use", POS_FIGHTING, do_use_consumable, 0, SCMD_USE, FALSE, ACTION_SWIFT, {0, 6}, NULL},
    {"usestoredconsumables", "usestoredconsumables", POS_DEAD, do_gen_tog, 0, SCMD_USE_STORED_CONSUMABLES, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"users", "users", POS_DEAD, do_users, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"utter", "ut", POS_RECLINING, do_utter, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"unadjure", "unadjure", POS_RECLINING, do_consign_to_oblivion, 0, SCMD_UNADJURE, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"unbind", "unbind", POS_DEAD, do_unbind, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"unlead", "unlead", POS_RECLINING, do_unlead, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"uncondemn", "uncondemn", POS_RECLINING, do_consign_to_oblivion, 0, SCMD_UNCONDEMN, FALSE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"value", "val", POS_STANDING, do_not_here, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"vampireform", "vampireform", POS_FIGHTING, do_vampiric_shape_change, 0, 0, FALSE, ACTION_STANDARD, {0, 0}, can_vampiric_shape_change},
    {"version", "ver", POS_DEAD, do_gen_ps, 0, SCMD_VERSION, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"visible", "vis", POS_RECLINING, do_visible, 1, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"vnum", "vnum", POS_DEAD, do_vnum, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"vstat", "vstat", POS_DEAD, do_vstat, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"vdelete", "vdelete", POS_DEAD, do_vdelete, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"vanish", "vanish", POS_DEAD, do_vanish, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"wake", "wake", POS_SLEEPING, do_wake, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"walkto", "walkto", POS_STANDING, do_walkto, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"wear", "wea", POS_RESTING, do_wear, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"weather", "weather", POS_RECLINING, do_weather, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"who", "wh", POS_DEAD, do_who, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"whois", "whoi", POS_DEAD, do_whois, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"whoami", "whoami", POS_DEAD, do_gen_ps, 0, SCMD_WHOAMI, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"where", "where", POS_RECLINING, do_where, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"whirlwind", "whirl", POS_FIGHTING, do_whirlwind, 0, 0, FALSE, ACTION_STANDARD | ACTION_MOVE, {6, 6}, can_whirlwind},
    //{ "whirlwind", "whirlwind", POS_FIGHTING, do_mode, 1, MODE_WHIRLWIND_ATTACK, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"whisper", "whisper", POS_RECLINING, do_spec_comm, 0, SCMD_WHISPER, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"wield", "wie", POS_RESTING, do_wield, 0, 0, FALSE, ACTION_MOVE, {0, 6}, NULL},
    {"withdraw", "withdraw", POS_STANDING, do_not_here, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"wiznet", "wiz", POS_DEAD, do_wiznet, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {";", ";", POS_DEAD, do_wiznet, LVL_IMMORT, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"wizhelp", "wizhelp", POS_SLEEPING, do_commands, LVL_IMMORT, SCMD_WIZHELP, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"wizlist", "wizlist", POS_DEAD, do_gen_ps, 0, SCMD_WIZLIST, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"wizupdate", "wizupde", POS_DEAD, do_wizupdate, LVL_GRSTAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"wizlock", "wizlock", POS_DEAD, do_wizlock, LVL_IMPL, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"write", "write", POS_STANDING, do_write, 1, 0, FALSE, ACTION_NONE, {0, 0}, NULL},
    {"wholenessofbody", "wholenessofbody", POS_RECLINING, do_wholenessofbody, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, can_wholenessofbody},
    {"wildshape", "wildshape", POS_RESTING, do_wildshape, 1, 0, FALSE, ACTION_STANDARD, {6, 0}, NULL},
    {"weaponinfo", "weaponinfo", POS_DEAD, do_weaponinfo, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"weaponlist", "weaponlist", POS_DEAD, do_weaponlist, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"weaponlist2", "weaponlist2", POS_DEAD, do_weaponlist_old, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"weaponproficiencies", "weaponprof", POS_DEAD, do_weaponproficiencies, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"weapontypes", "weapontypes", POS_DEAD, do_weapontypes, 0, 0, TRUE, ACTION_NONE, {0, 0}, NULL},

    /* {"command", "sort_as", minimum_position, *command_pointer, minimum_level, subcmd, ignore_wait, actions_required, {action_cooldowns}, *command_check_pointer},*/

    {"zreset", "zreset", POS_DEAD, do_zreset, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"zedit", "zedit", POS_DEAD, do_oasis_zedit, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"zlist", "zlist", POS_DEAD, do_oasis_list, 0, SCMD_OASIS_ZLIST, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"zlock", "zlock", POS_DEAD, do_zlock, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"zunlock", "zunlock", POS_DEAD, do_zunlock, LVL_STAFF, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"zcheck", "zcheck", POS_DEAD, do_zcheck, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},
    {"zpurge", "zpurge", POS_DEAD, do_zpurge, LVL_BUILDER, 0, TRUE, ACTION_NONE, {0, 0}, NULL},

    /* test commands for new prep system */
    //{ "memtest", "memtest", POS_RESTING, do_gen_preparation, LVL_BUILDER, SCMD_MEMORIZE, FALSE, ACTION_NONE, {0, 0}, NULL},
    //{ "fortest", "fortest", POS_RESTING, do_consign_to_oblivion, LVL_BUILDER, SCMD_FORGET, FALSE, ACTION_NONE, {0, 0}, NULL},
    //{ "medtest", "medtest", POS_RESTING, do_gen_preparation, LVL_BUILDER, SCMD_MEDITATE, FALSE, ACTION_NONE, {0, 0}, NULL},
    //{ "comtest", "comtest", POS_RESTING, do_gen_preparation, LVL_BUILDER, SCMD_COMPOSE, FALSE, ACTION_NONE, {0, 0}, NULL},

    {"\n", "zzzzzzz", 0, 0, 0, 0, FALSE, ACTION_NONE, {0, 0}, NULL}}; /* this must be last */

/* Thanks to Melzaren for this change to allow DG Scripts to be attachable
 *to player's while still disallowing them to manually use the DG-Commands. */
const struct mob_script_command_t mob_script_commands[] = {

    /* DG trigger commands. minimum_level should be set to -1. */
    {"masound", do_masound, 0},
    {"mkill", do_mkill, 0},
    {"mjunk", do_mjunk, 0},
    {"mdamage", do_mdamage, 0},
    {"mdoor", do_mdoor, 0},
    {"mecho", do_mecho, 0},
    {"mgecho", do_mgecho, 0},
    {"mrecho", do_mrecho, 0},
    {"mechoaround", do_mechoaround, 0},
    {"msend", do_msend, 0},
    {"mload", do_mload, 0},
    {"mpurge", do_mpurge, 0},
    {"mgoto", do_mgoto, 0},
    {"mat", do_mat, 0},
    {"mteleport", do_mteleport, 0},
    {"mforce", do_mforce, 0},
    {"mhunt", do_mhunt, 0},
    {"mremember", do_mremember, 0},
    {"mforget", do_mforget, 0},
    {"mtransform", do_mtransform, 0},
    {"mzoneecho", do_mzoneecho, 0},
    {"mfollow", do_mfollow, 0},
    {"\n", do_not_here, 0}};

int script_command_interpreter(struct char_data *ch, char *arg)
{
  /* DG trigger commands */

  int i;
  char first_arg[MAX_INPUT_LENGTH] = {'\0'};
  char *line;

  skip_spaces(&arg);
  if (!*arg)
    return 0;

  line = any_one_arg(arg, first_arg);

  for (i = 0; *mob_script_commands[i].command_name != '\n'; i++)
    if (!str_cmp(first_arg, mob_script_commands[i].command_name))
      break; // NB - only allow full matches.

  if (*mob_script_commands[i].command_name == '\n')
    return 0; // no matching commands.

  /* Poiner to the command? */
  ((*mob_script_commands[i].command_pointer)(ch, line, 0,
                                             mob_script_commands[i].subcmd));
  return 1; // We took care of execution. Let caller know.
}

const char *reserved[] = {
    "a",
    "an",
    "self",
    "me",
    "all",
    "room",
    "someone",
    "something",
    "\n"};

static int sort_commands_helper(const void *a, const void *b)
{
  return strcmp(complete_cmd_info[*(const int *)a].sort_as,
                complete_cmd_info[*(const int *)b].sort_as);
}

void sort_commands(void)
{
  int a, num_of_cmds = 0;

  while (complete_cmd_info[num_of_cmds].command[0] != '\n')
    num_of_cmds++;
  num_of_cmds++; /* \n */

  CREATE(cmd_sort_info, int, num_of_cmds);

  for (a = 0; a < num_of_cmds; a++)
    cmd_sort_info[a] = a;

  /* Don't sort the RESERVED or \n entries. */
  qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}

/* This is the actual command interpreter called from game_loop() in comm.c
 * It makes sure you are the proper level and position to execute the command,
 * then calls the appropriate function. */
void command_interpreter(struct char_data *ch, char *argument)
{
  int cmd = 0, length = 0;
  char *line = NULL;
  char arg[MAX_INPUT_LENGTH] = {'\0'};

  /* just drop to next line for hitting CR */
  skip_spaces(&argument);
  if (!*argument)
    return;

  /* special case to handle one-character, non-alphanumeric commands; requested
   * by many people so "'hi" or ";godnet test" is possible. Patch sent by Eric
   * Green and Stefan Wasilewski. */
  if (!isalpha(*argument))
  {
    arg[0] = argument[0];
    arg[1] = '\0';
    line = argument + 1;
  }
  else
    line = any_one_arg(argument, arg);

  /* Since all command triggers check for valid_dg_target before acting, the levelcheck
   * here has been removed. Otherwise, find the command. */
  {
    int cont;                 /* continue the command checks */
    char saystring[] = "say"; /* replacement string */

    /* if we are using the arglist in scripts, we have an issue with intercepting
       commands that have more than one access point, example:  "say" and "'" both
       will call the same command in the code, but are sending a different ARG to
       the command trigger checks...  resolution for now is to just replace the
       "'" with "say" when we find it here. -Zusuk */

    if (is_abbrev(arg, "'"))
    {
      cont = command_wtrigger(ch, saystring, line); /* any world triggers ? */
    }
    else
      cont = command_wtrigger(ch, arg, line); /* any world triggers ? */

    if (!cont)
    {
      if (is_abbrev(arg, "'"))
      {
        cont = command_mtrigger(ch, saystring, line); /* any mobile triggers ? */
      }
      else
        cont = command_mtrigger(ch, arg, line); /* any mobile triggers ? */
    }
    if (!cont)
    {
      if (is_abbrev(arg, "'"))
      {
        cont = command_otrigger(ch, saystring, line); /* any object triggers ? */
      }
      else
        cont = command_otrigger(ch, arg, line); /* any object triggers ? */
    }
    if (cont)
    {
      return; /* yes, command trigger took over */
    }
  }

  /* Allow IMPLs to switch into mobs to test the commands. */
  if (IS_NPC(ch) && ch->desc && GET_LEVEL(ch->desc->original) >= LVL_IMPL)
  {
    if (script_command_interpreter(ch, argument))
      return;
  }

  for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
    if (complete_cmd_info[cmd].command_pointer != do_action &&
        !strncmp(complete_cmd_info[cmd].command, arg, length))
      if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level)
        break;

  /* it's not a 'real' command, so it's a social */

  if (*complete_cmd_info[cmd].command == '\n')
    for (length = strlen(arg), cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
      if (complete_cmd_info[cmd].command_pointer == do_action &&
          !strncmp(complete_cmd_info[cmd].command, arg, length))
        if (GET_LEVEL(ch) >= complete_cmd_info[cmd].minimum_level)
          break;

  if (*complete_cmd_info[cmd].command == '\n')
  {
    int found = 0;
    send_to_char(ch, "Huh!?!\r\n");

    for (cmd = 0; *cmd_info[cmd].command != '\n'; cmd++)
    {
      if (*arg != *cmd_info[cmd].command || cmd_info[cmd].minimum_level > GET_LEVEL(ch))
        continue;

      /* Only apply levenshtein counts if the command is not a trigger command. */
      if ((levenshtein_distance(arg, cmd_info[cmd].command) <= 2) &&
          (cmd_info[cmd].minimum_level >= 0))
      {
        if (!found)
        {
          send_to_char(ch, "\r\nDid you mean:\r\n");
          found = 1;
        }
        send_to_char(ch, "  %s\r\n", cmd_info[cmd].command);
      }
    }
    send_to_char(ch, "\tDYou can also check the help index, type 'hindex <keyword>'\tn\r\n");
  }
  else if ((AFF_FLAGGED(ch, AFF_PARALYZED)) &&
           GET_LEVEL(ch) < LVL_IMPL &&
           !is_abbrev(complete_cmd_info[cmd].command, "affects"))
  {
    send_to_char(ch, "You try, but you are unable to move!\r\n");
    if (AFF_FLAGGED(ch, AFF_FREE_MOVEMENT))
    {
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_PARALYZED);
      send_to_char(ch, "Your free movement breaks the paralysis!\r\n");
      act("$n's free movement breaks the paralysis!",
          TRUE, ch, 0, 0, TO_ROOM);
    }
  }
  else if ((AFF_FLAGGED(ch, AFF_STUN)) &&
           GET_LEVEL(ch) < LVL_IMPL &&
           !is_abbrev(complete_cmd_info[cmd].command, "affects"))
  {
    send_to_char(ch, "You try, but you are unable to move!\r\n");
    if (AFF_FLAGGED(ch, AFF_FREE_MOVEMENT))
    {
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_STUN);
      send_to_char(ch, "Your free movement breaks the paralysis!\r\n");
      act("$n's free movement breaks the paralysis!",
          TRUE, ch, 0, 0, TO_ROOM);
    }
  }
  else if ((char_has_mud_event(ch, eSTUNNED)) &&
           GET_LEVEL(ch) < LVL_IMPL &&
           !is_abbrev(complete_cmd_info[cmd].command, "affects"))
  {
    send_to_char(ch, "You try, but you are unable to move!\r\n");
    if (AFF_FLAGGED(ch, AFF_FREE_MOVEMENT))
    {
      change_event_duration(ch, eSTUNNED, 0);
      send_to_char(ch, "Your free movement breaks the stun!\r\n");
      act("$n's free movement breaks the stun!",
          TRUE, ch, 0, 0, TO_ROOM);
    }
  }
  else if (AFF_FLAGGED(ch, AFF_DAZED) && GET_LEVEL(ch) < LVL_IMPL &&
           !is_abbrev(complete_cmd_info[cmd].command, "affects"))
    send_to_char(ch, "You are too dazed to do anything!\r\n");
  else if (!IS_NPC(ch) && PLR_FLAGGED(ch, PLR_FROZEN) && GET_LEVEL(ch) < LVL_IMPL)
    send_to_char(ch, "You try, but the mind-numbing cold prevents you...\r\n");
  else if (complete_cmd_info[cmd].command_pointer == NULL)
    send_to_char(ch, "Sorry, that command hasn't been implemented yet.\r\n");
  else if (IS_NPC(ch) && complete_cmd_info[cmd].minimum_level >= LVL_IMMORT)
    send_to_char(ch, "You can't use immortal commands while switched.\r\n");
  else if (IS_CASTING(ch) && !is_abbrev(complete_cmd_info[cmd].command, "abort") && !IS_NPC(ch))
    send_to_char(ch, "You are too busy casting [you can 'abort' the spell]...\r\n");
  else if (AFF_FLAGGED(ch, AFF_HIDE) && !AFF_FLAGGED(ch, AFF_SNEAK))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    send_to_char(ch, "You slowly step out of the shadows... (command removed hide, try sneaking before hiding)\r\n");
  }
  else if (AFF_FLAGGED(ch, AFF_HIDE) && AFF_FLAGGED(ch, AFF_SNEAK) &&
           !is_abbrev(complete_cmd_info[cmd].command, "look") &&
           !is_abbrev(complete_cmd_info[cmd].command, "trip") &&
           !is_abbrev(complete_cmd_info[cmd].command, "north") &&
           !is_abbrev(complete_cmd_info[cmd].command, "up") &&
           !is_abbrev(complete_cmd_info[cmd].command, "down") &&
           !is_abbrev(complete_cmd_info[cmd].command, "east") &&
           !is_abbrev(complete_cmd_info[cmd].command, "west") &&
           !is_abbrev(complete_cmd_info[cmd].command, "south") &&
           !is_abbrev(complete_cmd_info[cmd].command, "northwest") &&
           !is_abbrev(complete_cmd_info[cmd].command, "northeast") &&
           !is_abbrev(complete_cmd_info[cmd].command, "southeast") &&
           !is_abbrev(complete_cmd_info[cmd].command, "southwest") &&
           !is_abbrev(complete_cmd_info[cmd].command, "nw") &&
           !is_abbrev(complete_cmd_info[cmd].command, "ne") &&
           !is_abbrev(complete_cmd_info[cmd].command, "se") &&
           !is_abbrev(complete_cmd_info[cmd].command, "sw") &&
           !is_abbrev(complete_cmd_info[cmd].command, "inside") &&
           !is_abbrev(complete_cmd_info[cmd].command, "outside") &&
           !is_abbrev(complete_cmd_info[cmd].command, "get") &&  /* maybe re-analyze this one */
           !is_abbrev(complete_cmd_info[cmd].command, "take") && /* maybe re-analyze this one */
           !is_abbrev(complete_cmd_info[cmd].command, "group") &&
           !is_abbrev(complete_cmd_info[cmd].command, "affects") &&
           !is_abbrev(complete_cmd_info[cmd].command, "gtell") &&
           !is_abbrev(complete_cmd_info[cmd].command, "gsay") &&
           !is_abbrev(complete_cmd_info[cmd].command, "consider") &&
           !is_abbrev(complete_cmd_info[cmd].command, "backstab") &&
           !is_abbrev(complete_cmd_info[cmd].command, "steal") &&
           !is_abbrev(complete_cmd_info[cmd].command, "hit") &&
           !is_abbrev(complete_cmd_info[cmd].command, "kill") &&
           !is_abbrev(complete_cmd_info[cmd].command, "sit") &&
           !is_abbrev(complete_cmd_info[cmd].command, "equipment") &&
           !is_abbrev(complete_cmd_info[cmd].command, "idea") &&
           !is_abbrev(complete_cmd_info[cmd].command, "bug") &&
           !is_abbrev(complete_cmd_info[cmd].command, "typo") &&
           !is_abbrev(complete_cmd_info[cmd].command, "inventory") &&
           !is_abbrev(complete_cmd_info[cmd].command, "stand") &&
           !is_abbrev(complete_cmd_info[cmd].command, "sap") &&
           !is_abbrev(complete_cmd_info[cmd].command, "scan") &&
           !is_abbrev(complete_cmd_info[cmd].command, "who") &&
           !is_abbrev(complete_cmd_info[cmd].command, "score") &&
           !is_abbrev(complete_cmd_info[cmd].command, "queue") &&
           !is_abbrev(complete_cmd_info[cmd].command, "help") &&
           !is_abbrev(complete_cmd_info[cmd].command, "feat") &&
           !is_abbrev(complete_cmd_info[cmd].command, "tnl") &&
           !is_abbrev(complete_cmd_info[cmd].command, "prefedit") &&
           !is_abbrev(complete_cmd_info[cmd].command, "bug") &&
           !is_abbrev(complete_cmd_info[cmd].command, "typo") &&
           !is_abbrev(complete_cmd_info[cmd].command, "idea") &&
           !is_abbrev(complete_cmd_info[cmd].command, "applypoison") &&
           !is_abbrev(complete_cmd_info[cmd].command, "fire") &&
           !is_abbrev(complete_cmd_info[cmd].command, "disabletrap") &&
           !is_abbrev(complete_cmd_info[cmd].command, "detecttrap") &&
           !is_abbrev(complete_cmd_info[cmd].command, "cast") &&
           !is_abbrev(complete_cmd_info[cmd].command, "mark") &&
           !is_abbrev(complete_cmd_info[cmd].command, "races") &&
           !is_abbrev(complete_cmd_info[cmd].command, "class") &&
           !is_abbrev(complete_cmd_info[cmd].command, "cooldowns") &&
           !is_abbrev(complete_cmd_info[cmd].command, "abilities") &&
           !is_abbrev(complete_cmd_info[cmd].command, "resistances") &&
           !is_abbrev(complete_cmd_info[cmd].command, "lore") &&
           !is_abbrev(complete_cmd_info[cmd].command, "powerattack") &&
           !is_abbrev(complete_cmd_info[cmd].command, "expertise") &&
           !is_abbrev(complete_cmd_info[cmd].command, "ooc") &&
           !is_abbrev(complete_cmd_info[cmd].command, "chat") &&
           !is_abbrev(complete_cmd_info[cmd].command, "osay") &&
           !is_abbrev(complete_cmd_info[cmd].command, "rest") &&
           !is_abbrev(complete_cmd_info[cmd].command, "save") &&
#ifdef CAMPAIGN_FR
           !is_abbrev(complete_cmd_info[cmd].command, "say") &&
#endif
           !is_abbrev(complete_cmd_info[cmd].command, "attackqueue"))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    send_to_char(ch, "You step out of the shadows...  (command removed hide)\r\n");
  }
  else if (AFF_FLAGGED(ch, AFF_HIDE) && AFF_FLAGGED(ch, AFF_SNEAK) && is_abbrev(complete_cmd_info[cmd].command, "cast") && !HAS_FEAT(ch, FEAT_MAGICAL_AMBUSH))
  {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);
    send_to_char(ch, "You step out of the shadows...  (attempting to cast without 'magical ambush' removes hidden status)\r\n");
  }
  else if (char_has_mud_event(ch, eCRAFTING) &&
           !is_abbrev(complete_cmd_info[cmd].command, "gossip") &&
           !is_abbrev(complete_cmd_info[cmd].command, "gemote") &&
           !is_abbrev(complete_cmd_info[cmd].command, "chat") &&
           !is_abbrev(complete_cmd_info[cmd].command, "look") &&
           !is_abbrev(complete_cmd_info[cmd].command, "score") &&
           !is_abbrev(complete_cmd_info[cmd].command, "group") &&
           !is_abbrev(complete_cmd_info[cmd].command, "say") &&
           !is_abbrev(complete_cmd_info[cmd].command, "'") &&
           !is_abbrev(complete_cmd_info[cmd].command, "help") &&
           !is_abbrev(complete_cmd_info[cmd].command, "reply") &&
           !is_abbrev(complete_cmd_info[cmd].command, "prefedit") &&
           !is_abbrev(complete_cmd_info[cmd].command, "bug") &&
           !is_abbrev(complete_cmd_info[cmd].command, "typo") &&
           !is_abbrev(complete_cmd_info[cmd].command, "idea") &&
           !is_abbrev(complete_cmd_info[cmd].command, "tell"))
    send_to_char(ch, "You are too busy crafting. [Available commands: gossip/"
                     "chat/gemote/look/score/group/say/tell/reply/help/prefedit/bug/typo/idea]\r\n");
  else if (GET_POS(ch) < complete_cmd_info[cmd].minimum_position)
    switch (GET_POS(ch))
    {
    case POS_DEAD:
      send_to_char(ch, "Lie still; you are DEAD!!! :-(\r\n");
      break;
    case POS_INCAP:
    case POS_MORTALLYW:
      send_to_char(ch, "You are in a pretty bad shape, unable to do anything!\r\n");
      break;
    case POS_STUNNED:
      send_to_char(ch, "All you can do right now is think about the stars!\r\n");
      break;
    case POS_SLEEPING:
      send_to_char(ch, "In your dreams, or what?\r\n");
      break;
    case POS_RECLINING:
      send_to_char(ch, "You can't do that while reclining...\r\n");
      break;
    case POS_RESTING:
      send_to_char(ch, "Nah... You feel too relaxed to do that..\r\n");
      break;
    case POS_SITTING:
      send_to_char(ch, "Maybe you should get on your feet first?\r\n");
      break;
    case POS_FIGHTING:
      send_to_char(ch, "No way!  You're fighting for your life!\r\n");
      break;
      //    } else if (HAS_WAIT(ch) && complete_cmd_info[cmd].ignore_wait == FALSE) {
      //      send_to_char(ch, "You need to wait longer before you are able to do that.\r\n");
    }
  else if (!IS_NPC(ch) &&
           ((IS_SET(complete_cmd_info[cmd].actions_required, ACTION_STANDARD) && !is_action_available(ch, atSTANDARD, FALSE)) ||
            ((IS_SET(complete_cmd_info[cmd].actions_required, ACTION_MOVE) && (!is_action_available(ch, atMOVE, FALSE) &&
                                                                               !is_action_available(ch, atSTANDARD, FALSE))))))
  {
    if (pending_actions(ch) > MAX_QUEUE_SIZE)
    {
      send_to_char(ch, "The action queue is full.\r\n");
    }
    else
    {
      /* Add to the queue */
      struct action_data *action;
      CREATE(action, struct action_data, 1);
      action->argument = strdup(argument);
      action->actions_required = complete_cmd_info[cmd].actions_required;

      enqueue_action(GET_QUEUE(ch), action);
      send_to_char(ch, "The command was added to the queue.\r\n");
    }
  }
  else if (no_specials || !special(ch, cmd, line))
  {
    ((*complete_cmd_info[cmd].command_pointer)(ch, line, cmd, complete_cmd_info[cmd].subcmd));
  }
}

/* Routines to handle aliasing. */
static struct alias_data *find_alias(struct alias_data *alias_list, char *str)
{
  while (alias_list != NULL)
  {
    if (*str == *alias_list->alias) /* hey, every little bit counts :-) */
      if (!strcmp(str, alias_list->alias))
        return (alias_list);

    alias_list = alias_list->next;
  }

  return (NULL);
}

void free_alias(struct alias_data *a)
{
  if (a->alias)
    free(a->alias);
  if (a->replacement)
    free(a->replacement);
  free(a);
}

/* The interface to the outside world: do_alias */
ACMDU(do_alias)
{
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  char *repl;
  struct alias_data *a, *temp;

  if (IS_NPC(ch))
    return;

  repl = any_one_arg(argument, arg);

  if (!*arg)
  { /* no argument specified -- list currently defined aliases */
    send_to_char(ch, "Currently defined aliases:\r\n");
    if ((a = GET_ALIASES(ch)) == NULL)
      send_to_char(ch, " None.\r\n");
    else
    {
      while (a != NULL)
      {
        send_to_char(ch, "%-15s %s\r\n", a->alias, a->replacement);
        a = a->next;
      }
    }
  }
  else
  { /* otherwise, add or remove aliases */
    /* is this an alias we've already defined? */
    if ((a = find_alias(GET_ALIASES(ch), arg)) != NULL)
    {
      REMOVE_FROM_LIST(a, GET_ALIASES(ch), next);
      free_alias(a);
    }
    /* if no replacement string is specified, assume we want to delete */
    if (!*repl)
    {
      if (a == NULL)
        send_to_char(ch, "No such alias.\r\n");
      else
        send_to_char(ch, "Alias deleted.\r\n");
    }
    else
    { /* otherwise, either add or redefine an alias */
      if (!str_cmp(arg, "alias"))
      {
        send_to_char(ch, "You can't alias 'alias'.\r\n");
        return;
      }
      CREATE(a, struct alias_data, 1);
      a->alias = strdup(arg);
      delete_doubledollar(repl);
      a->replacement = strdup(repl);
      if (strchr(repl, ALIAS_SEP_CHAR) || strchr(repl, ALIAS_VAR_CHAR))
        a->type = ALIAS_COMPLEX;
      else
        a->type = ALIAS_SIMPLE;
      a->next = GET_ALIASES(ch);
      GET_ALIASES(ch) = a;
      save_char(ch, 0);
      send_to_char(ch, "Alias ready.\r\n");
    }
  }
}

/* Valid numeric replacements are only $1 .. $9 (makes parsing a little easier,
 * and it's not that much of a limitation anyway.)  Also valid is "$*", which
 * stands for the entire original line after the alias. ";" is used to delimit
 * commands. */
#define NUM_TOKENS 9

static void perform_complex_alias(struct txt_q *input_q, char *orig, struct alias_data *a)
{
  struct txt_q temp_queue;
  char *tokens[NUM_TOKENS], *temp, *write_point;
  char buf2[MAX_RAW_INPUT_LENGTH] = {'\0'}, buf[MAX_RAW_INPUT_LENGTH] = {'\0'}; /* raw? */
  int num_of_tokens = 0, num;

  /* First, parse the original string */
  strcpy(buf2, orig); /* strcpy: OK (orig:MAX_INPUT_LENGTH < buf2:MAX_RAW_INPUT_LENGTH) */
  temp = strtok(buf2, " ");
  while (temp != NULL && num_of_tokens < NUM_TOKENS)
  {
    tokens[num_of_tokens++] = temp;
    temp = strtok(NULL, " ");
  }

  /* initialize */
  write_point = buf;
  temp_queue.head = temp_queue.tail = NULL;

  /* now parse the alias */
  for (temp = a->replacement; *temp; temp++)
  {
    if (*temp == ALIAS_SEP_CHAR)
    {
      *write_point = '\0';
      buf[MAX_INPUT_LENGTH - 1] = '\0';
      write_to_q(buf, &temp_queue, 1);
      write_point = buf;
    }
    else if (*temp == ALIAS_VAR_CHAR)
    {
      temp++;
      if ((num = *temp - '1') < num_of_tokens && num >= 0)
      {
        strcpy(write_point, tokens[num]); /* strcpy: OK */
        write_point += strlen(tokens[num]);
      }
      else if (*temp == ALIAS_GLOB_CHAR)
      {
        strcpy(write_point, orig); /* strcpy: OK */
        write_point += strlen(orig);
      }
      else if ((*(write_point++) = *temp) == '$') /* redouble $ for act safety */
        *(write_point++) = '$';
    }
    else
      *(write_point++) = *temp;
  }

  *write_point = '\0';
  buf[MAX_INPUT_LENGTH - 1] = '\0';
  write_to_q(buf, &temp_queue, 1);

  /* push our temp_queue on to the _front_ of the input queue */
  if (input_q->head == NULL)
    *input_q = temp_queue;
  else
  {
    temp_queue.tail->next = input_q->head;
    input_q->head = temp_queue.head;
  }
}

/* Given a character and a string, perform alias replacement on it.
 * Return values:
 *   0: String was modified in place; call command_interpreter immediately.
 *   1: String was _not_ modified in place; rather, the expanded aliases
 *      have been placed at the front of the character's input queue. */
int perform_alias(struct descriptor_data *d, char *orig, size_t maxlen)
{
  char first_arg[MAX_INPUT_LENGTH] = {'\0'}, *ptr;
  struct alias_data *a, *tmp;

  /* Mobs don't have alaises. */
  if (IS_NPC(d->character))
    return (0);

  /* bail out immediately if the guy doesn't have any aliases */
  if ((tmp = GET_ALIASES(d->character)) == NULL)
    return (0);

  /* find the alias we're supposed to match */
  ptr = any_one_arg(orig, first_arg);

  /* bail out if it's null */
  if (!*first_arg)
    return (0);

  /* if the first arg is not an alias, return without doing anything */
  if ((a = find_alias(tmp, first_arg)) == NULL)
    return (0);

  if (a->type == ALIAS_SIMPLE)
  {
    strlcpy(orig, a->replacement, maxlen);
    return (0);
  }
  else
  {
    perform_complex_alias(&d->input, ptr, a);
    return (1);
  }
}

/* Various other parsing utilities. */

int is_number(const char *str)
{
  if (*str == '-')
    str++;
  if (!*str)
    return (0);
  while (*str)
    if (!isdigit(*(str++)))
      return (0);

  return (1);
}

/* Given a string, change all instances of double dollar signs ($$) to single
 * dollar signs ($).  When strings come in, all $'s are changed to $$'s to
 * avoid having users be able to crash the system if the inputted string is
 * eventually sent to act().  If you are using user input to produce screen
 * output AND YOU ARE SURE IT WILL NOT BE SENT THROUGH THE act() FUNCTION
 * (i.e., do_gecho, do_title, but NOT do_say), you can call
 * delete_doubledollar() to make the output look correct.
 * Modifies the string in-place. */
char *delete_doubledollar(char *string)
{
  char *ddread, *ddwrite;

  /* If the string has no dollar signs, return immediately */
  if ((ddwrite = strchr(string, '$')) == NULL)
    return (string);

  /* Start from the location of the first dollar sign */
  ddread = ddwrite;

  while (*ddread)                            /* Until we reach the end of the string... */
    if ((*(ddwrite++) = *(ddread++)) == '$') /* copy one char */
      if (*ddread == '$')
        ddread++; /* skip if we saw 2 $'s in a row */

  *ddwrite = '\0';

  return (string);
}

int reserved_word(char *argument)
{
  return (search_block(argument, reserved, TRUE) >= 0);
}

/* one_word is like any_one_arg, except that words in quotes ("") are
 * considered one word. No longer ignores fill words.  -dak */
char *one_word(char *argument, char *first_arg)
{
  skip_spaces(&argument);

  if (*argument == '\"')
  {
    argument++;
    while (*argument && *argument != '\"')
    {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }
    argument++;
  }
  else
  {
    while (*argument && !isspace(*argument))
    {
      *(first_arg++) = LOWER(*argument);
      argument++;
    }
  }

  *first_arg = '\0';
  return (argument);
}

/* Same as one_argument except that it takes three args and returns the rest;
 * ignores fill words */
/* unsafe version */
char *three_arguments_u(char *argument, char *first_arg, char *second_arg, char *third_arg)
{
  return (one_argument_u(one_argument_u(one_argument_u(argument, first_arg), second_arg), third_arg)); /* :-) */
}
/* safe version */
const char *three_arguments(
    const char *argument,
    char *first_arg, size_t n1, char *second_arg, size_t n2, char *third_arg, size_t n3)
{
  return (one_argument(one_argument(one_argument(argument, first_arg, n1), second_arg, n2), third_arg, n3)); /* :-) */
}

/* Determine if a given string is an abbreviation of another.
 * Returns 1 if arg1 is an abbreviation of arg2. */
int is_abbrev(const char *arg1, const char *arg2)
{
  if (!*arg1)
    return (0);
  if (!*arg2)
    return (0);

  for (; *arg1 && *arg2; arg1++, arg2++)
    if (LOWER(*arg1) != LOWER(*arg2))
      return (0);

  if (!*arg1)
    return (1);
  else
    return (0);
}

/* Used in specprocs, mostly.  (Exactly) matches "command" to cmd number */
int find_command(const char *command)
{
  int cmd;

  for (cmd = 0; *complete_cmd_info[cmd].command != '\n'; cmd++)
    if (!strcmp(complete_cmd_info[cmd].command, command))
      return (cmd);

  return (-1);
}

int special(struct char_data *ch, int cmd, char *arg)
{
  struct obj_data *i;
  struct char_data *k;
  int j;


  if (IN_ROOM(ch) == NOWHERE) return 0;

  /* special in room? */
  if (GET_ROOM_SPEC(IN_ROOM(ch)) != NULL)
    if (GET_ROOM_SPEC(IN_ROOM(ch))(ch, world + IN_ROOM(ch), cmd, arg))
      return (1);

  /* special in equipment list? */
  for (j = 0; j < NUM_WEARS; j++)
    if (GET_EQ(ch, j) && GET_OBJ_SPEC(GET_EQ(ch, j)) != NULL)
      if (GET_OBJ_SPEC(GET_EQ(ch, j))(ch, GET_EQ(ch, j), cmd, arg))
        return (1);

  /* special in inventory? */
  for (i = ch->carrying; i; i = i->next_content)
    if (GET_OBJ_SPEC(i) != NULL)
      if (GET_OBJ_SPEC(i)(ch, i, cmd, arg))
        return (1);

  /* special in mobile present? */
  if (IN_ROOM(ch) != NOWHERE)
  {
    for (k = world[IN_ROOM(ch)].people; k; k = k->next_in_room)
      if (!MOB_FLAGGED(k, MOB_NOTDEADYET))
        if (GET_MOB_SPEC(k) && GET_MOB_SPEC(k)(ch, k, cmd, arg))
          return (1);
  }

  /* special in object present? */
  if (IN_ROOM(ch) != NOWHERE)
  {
    for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
      if (GET_OBJ_SPEC(i) != NULL)
        if (GET_OBJ_SPEC(i)(ch, i, cmd, arg))
          return (1);
  }

  return (0);
}

/* Stuff for controlling the non-playing sockets (get name, pwd etc).
 * This function needs to die. */
static int _parse_name(char *arg, char *name)
{
  int i;

  skip_spaces(&arg);
  for (i = 0; (*name = *arg); arg++, i++, name++)
    if (!isalpha(*arg))
      return (1);

  if (!i)
    return (1);

  return (0);
}

#define RECON 1
#define USURP 2
#define UNSWITCH 3

/* This function seems a bit over-extended. */
static int perform_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  struct char_data *target = NULL, *ch, *next_ch;
  int mode = 0;
  int pref_temp = 0; /* for "last" log */
  int id = GET_IDNUM(d->character);

  /* Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number. */

  for (k = descriptor_list; k; k = next_k)
  {
    next_k = k->next;

    if (k == d)
      continue;

    if (k->original && (GET_IDNUM(k->original) == id))
    {
      /* Original descriptor was switched, booting it and restoring normal body control. */

      write_to_output(d, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
      pref_temp = GET_PREF(k->character);
      if (!target)
      {
        target = k->original;
        mode = UNSWITCH;
      }
      if (k->character)
        k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
    }
    else if (k->character && GET_IDNUM(k->character) == id && k->original)
    {
      /* Character taking over their own body, while an immortal was switched to it. */

      do_return(k->character, NULL, 0, 0);
    }
    else if (k->character && GET_IDNUM(k->character) == id)
    {
      /* Character taking over their own body. */
      pref_temp = GET_PREF(k->character);

      if (!target && STATE(k) == CON_PLAYING)
      {
        write_to_output(k, "\r\nThis body has been usurped!\r\n");
        target = k->character;
        mode = USURP;
      }
      k->character->desc = NULL;
      k->character = NULL;
      k->original = NULL;
      write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
      STATE(k) = CON_CLOSE;
    }
  }

  /* Now, go through the character list, deleting all characters that are not
   * already marked for deletion from the above step (i.e., in the CON_HANGUP
   * state), and have not already been selected as a target for switching into.
   * In addition, if we haven't already found a target, choose one if one is
   * available (while still deleting the other duplicates, though theoretically
   * none should be able to exist). */
  for (ch = character_list; ch; ch = next_ch)
  {
    next_ch = ch->next;

    if (IS_NPC(ch))
      continue;
    if (GET_IDNUM(ch) != id)
      continue;

    /* ignore chars with descriptors (already handled by above step) */
    if (ch->desc)
      continue;

    /* don't extract the target char we've found one already */
    if (ch == target)
      continue;

    /* we don't already have a target and found a candidate for switching */
    if (!target)
    {
      target = ch;
      mode = RECON;
      pref_temp = GET_PREF(ch);
      continue;
    }

    /* we've found a duplicate - blow him away, dumping his eq in limbo. */
    if (IN_ROOM(ch) != NOWHERE)
      char_from_room(ch);
    char_to_room(ch, 1);
    extract_char(ch);
  }

  /* no target for switching into was found - allow login to continue */
  if (!target)
  {
    GET_PREF(d->character) = rand_number(1, 128000);
    if (GET_HOST(d->character))
      free(GET_HOST(d->character));
    GET_HOST(d->character) = strdup(d->host);
    return 0;
  }

  if (GET_HOST(target))
    free(GET_HOST(target));
  GET_HOST(target) = strdup(d->host);

  GET_PREF(target) = pref_temp;
  add_llog_entry(target, LAST_RECONNECT);

  /* Okay, we've found a target.  Connect d to target. */
  free_char(d->character); /* get rid of the old char */
  d->character = target;
  d->character->desc = d;
  d->original = NULL;
  d->character->char_specials.timer = 0;
  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
  STATE(d) = CON_PLAYING;
  MXPSendTag(d, "<VERSION>");

  switch (mode)
  {
  case RECON:
    write_to_output(d, "Reconnecting.\r\n");
    act("$n has reconnected.", TRUE, d->character, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(0, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    if (has_mail(GET_IDNUM(d->character)))
      write_to_output(d, "You have mail waiting.\r\n");
    break;
  case USURP:
    write_to_output(d, "You take over your own body, already in use!\r\n");
    act("$n suddenly keels over in pain, surrounded by a white aura...\r\n"
        "$n's body has been taken over by a new spirit!",
        TRUE, d->character, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
           "%s has re-logged in ... disconnecting old socket.", GET_NAME(d->character));
    break;
  case UNSWITCH:
    write_to_output(d, "Reconnecting to unswitched char.");
    mudlog(NRM, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "%s [%s] has reconnected.", GET_NAME(d->character), d->host);
    break;
  }

  return (1);
}

/* New Char dupe-check called at the start of character creation */
static bool perform_new_char_dupe_check(struct descriptor_data *d)
{
  struct descriptor_data *k, *next_k;
  bool found = FALSE;

  /* Now that this descriptor has successfully logged in, disconnect all
   * other descriptors controlling a character with the same ID number. */

  for (k = descriptor_list; k; k = next_k)
  {
    next_k = k->next;

    if (k == d)
      continue;

    /* these 6 checks added by zusuk to try and find dupe bug */
    if (!k)
      continue;
    if (!k->character)
      continue;
    if (!GET_NAME(k->character))
      continue;
    if (!d)
      continue;
    if (!d->character)
      continue;
    if (!GET_NAME(d->character))
      continue;
    /*****/

    /* Do the player names match? */
    if (!strcmp(GET_NAME(k->character), GET_NAME(d->character)))
    {
      /* Check the other character is still in creation? */
      if ((STATE(k) > CON_PLAYING) && (STATE(k) < CON_QCLASS))
      {
        /* Boot the older one */
        k->character->desc = NULL;
        k->character = NULL;
        k->original = NULL;
        write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
        STATE(k) = CON_CLOSE;

        mudlog(NRM, LVL_STAFF, TRUE, "Multiple logins detected in char creation for %s.", GET_NAME(d->character));

        found = TRUE;
      }
      else
      {
        /* Something went VERY wrong, boot both chars */
        k->character->desc = NULL;
        k->character = NULL;
        k->original = NULL;
        write_to_output(k, "\r\nMultiple login detected -- disconnecting.\r\n");
        STATE(k) = CON_CLOSE;

        d->character->desc = NULL;
        d->character = NULL;
        d->original = NULL;
        write_to_output(d, "\r\nSorry, due to multiple connections, all your connections are being closed.\r\n");
        write_to_output(d, "\r\nPlease reconnect.\r\n");
        STATE(d) = CON_CLOSE;

        mudlog(NRM, LVL_STAFF, TRUE, "SYSERR: Multiple logins with 1st in-game and the 2nd in char creation.");

        found = TRUE;
      }
    }
  }
  return (found);
}

/* load the player, put them in the right room - used by copyover_recover too
     this is also used for some initializing -zusuk */
int enter_player_game(struct descriptor_data *d)
{
  int load_result = -1;
  room_vnum load_room = NOWHERE;
  int i = 0;

  reset_char(d->character);

  if (PLR_FLAGGED(d->character, PLR_INVSTART))
    GET_INVIS_LEV(d->character) = GET_LEVEL(d->character);

  /* We have to place the character in a room before equipping them
   * or equip_char() will gripe about the person in NOWHERE. */
  if ((load_room = GET_LOADROOM(d->character)) != NOWHERE)
    load_room = real_room(load_room);

  /* If char was saved with NOWHERE, or real_room above failed... */
  if (load_room == NOWHERE)
  {
    if (GET_LEVEL(d->character) >= LVL_IMMORT)
      load_room = r_immort_start_room;
    else
      load_room = r_mortal_start_room;
  }

  /* send frozen characters to a special room if assigned */
  if (PLR_FLAGGED(d->character, PLR_FROZEN))
    load_room = r_frozen_start_room;

  /* copyover */
  GET_ID(d->character) = GET_IDNUM(d->character);

  /* find_char helper */
  add_to_lookup_table(GET_ID(d->character), (void *)d->character);

  /* After moving saving of variables to the player file, this should only
   * be called in case nothing was found in the pfile. If something was
   * found, SCRIPT(ch) will be set. */
  if (!SCRIPT(d->character))
    read_saved_vars(d->character);

  d->character->next = character_list;
  character_list = d->character;
  char_to_room(d->character, load_room);
  load_result = Crash_load(d->character);

  /* load up their pets, new system by gicksta */
  load_char_pets(d->character);

  /* Save the character and their object file */
  save_char(d->character, 0);
  Crash_crashsave(d->character);

  /* Check for a login trigger in the players' start room */
  login_wtrigger(&world[IN_ROOM(d->character)], d->character);

  /* this is already called in perform_dupe_check() before we get here, shouldn't be needed here -Nashak */
  /* MXPSendTag(d, "<VERSION>"); */

  new_mail_alert(d->character, FALSE);

  /* START PLAYER STAT HACKS */

  /* movement hack.  We changed movement to be out of 1,000
     need to check for old characters and get them up to speed. */
  if (GET_REAL_MAX_MOVE(d->character) < 400)
    GET_REAL_MAX_MOVE(d->character) *= 10;

  /* END PLAYER STAT HACKS */

  /* if they are on a mission, but a reboot/copyover has cleared the mobs, reload the mission mobs */
  create_mission_on_entry(d->character);

  /* If they're wildshaped, they re-enter as their natural form */
  wildshape_return(d->character);

  /* make sure we assign any new spells */
  for (i = 0; i < NUM_CLASSES; i++)
  {
    if (CLASS_LEVEL(d->character, i))
      init_class(d->character, i, CLASS_LEVEL(d->character, i));
  }

#ifdef CAMPAIGN_FR
  if (!race_list[GET_REAL_RACE(d->character)].is_pc)
    GET_REAL_RACE(d->character) = RACE_HUMAN;
#endif

  /* initialize the characters condensed combat data struct */
  init_condensed_combat_data(d->character);

  /* all done! */
  return load_result;
}

/* protocol handling event */
EVENTFUNC(get_protocols)
{
  struct descriptor_data *d;
  struct mud_event_data *pMudEvent;
  char buf[MAX_STRING_LENGTH] = {'\0'};
  int len;

  if (event_obj == NULL)
    return 0;

  pMudEvent = (struct mud_event_data *)event_obj;
  d = (struct descriptor_data *)pMudEvent->pStruct;

  /* Clear extra white space from the "protocol scroll" */
  write_to_output(d, "[H[J");

  len = snprintf(buf, MAX_STRING_LENGTH, "\tO[\toClient\tO] \tw%s\tn | ", d->pProtocol->pVariables[eMSDP_CLIENT_ID]->pValueString);

  if (d->pProtocol->pVariables[eMSDP_256_COLORS]->ValueInt)
    len += snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toColors\tO] \tw256\tn | ");
  else if (d->pProtocol->pVariables[eMSDP_ANSI_COLORS]->ValueInt)
    len += snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toColors\tO] \twAnsi\tn | ");
  else
    len += snprintf(buf + len, MAX_STRING_LENGTH - len, "[Colors] No Color | ");

  len += snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toMXP\tO] \tw%s\tn | ", d->pProtocol->bMXP ? "Yes" : "No");
  len += snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toMSDP\tO] \tw%s\tn | ", d->pProtocol->bMSDP ? "Yes" : "No");
  len += snprintf(buf + len, MAX_STRING_LENGTH - len, "\tO[\toGMCP\tO] \tw%s\tn\r\n\r\n", d->pProtocol->bGMCP ? "Yes" : "No");

  write_to_output(d, buf, 0);

  write_to_output(d, GREETINGS, 0);
  STATE(d) = CON_ACCOUNT_NAME; // CON_GET_NAME;
  return 0;
}

/* deal with newcomers and other non-playing sockets */
void nanny(struct descriptor_data *d, char *arg)
{
  int load_result = 0; /* Overloaded variable */
  int player_i = 0;
  int i = 0; /* incrementor */

  /* OasisOLC states */
  struct
  {
    int state;
    void (*func)(struct descriptor_data *, char *);
  } olc_functions[] = {
      {CON_OEDIT, oedit_parse},
      {CON_IEDIT, oedit_parse},
      {CON_ZEDIT, zedit_parse},
      {CON_SEDIT, sedit_parse},
      {CON_MEDIT, medit_parse},
      {CON_REDIT, redit_parse},
      {CON_CEDIT, cedit_parse},
      {CON_TRIGEDIT, trigedit_parse},
      {CON_AEDIT, aedit_parse},
      {CON_HEDIT, hedit_parse},
      {CON_HSEDIT, hsedit_parse},
      {CON_QEDIT, qedit_parse},
      {CON_HLQEDIT, hlqedit_parse},
      {CON_PREFEDIT, prefedit_parse},
      {CON_IBTEDIT, ibtedit_parse},
      {CON_CLANEDIT, clanedit_parse},
      {CON_MSGEDIT, msgedit_parse},
      {CON_STUDY, study_parse},
      /* NewCraft */
      {CON_CRAFTEDIT, craftedit_parse},
      {-1, NULL}};

  skip_spaces(&arg);

  /* Quick check for the OLC states. */
  for (player_i = 0; olc_functions[player_i].state >= 0; player_i++)
    if (STATE(d) == olc_functions[player_i].state)
    {
      (*olc_functions[player_i].func)(d, arg);
      return;
    }

  /* Not in OLC. */
  switch (STATE(d))
  {
  case CON_GET_PROTOCOL:
    write_to_output(d, "Collecting Protocol Information... Please Wait.\r\n");
    return;
    break;

  case CON_ACCOUNT_NAME:
    /* Initialize account data for this descriptor. */
    if (d->account == NULL)
    {
      CREATE(d->account, struct account_data, 1);
      d->account->name = NULL;
      for (i = 0; i < MAX_CHARS_PER_ACCOUNT; i++)
        d->account->character_names[i] = NULL;
    }
    d->character = NULL;
    if (!*arg)
      STATE(d) = CON_CLOSE;
    else
    {
      char buf[MAX_INPUT_LENGTH] = {'\0'}, tmp_name[MAX_INPUT_LENGTH] = {'\0'};

      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
          strlen(tmp_name) > MAX_NAME_LENGTH || !valid_name(tmp_name) ||
          fill_word(strcpy(buf, tmp_name)) || reserved_word(buf))
      { /* strcpy: OK (mutual MAX_INPUT_LENGTH) */
        write_to_output(d, "Invalid account name, please try another.\r\nName: ");
        return;
      }

      if ((load_account(tmp_name, d->account)) > -1)
      {
        if (!valid_name(tmp_name))
        {
          write_to_output(d, "Invalid account name, please try another.\r\nName: ");
          return;
        }
        write_to_output(d, "Password: ");
        d->idle_tics = 0;
        STATE(d) = CON_PASSWORD;
        ProtocolNoEcho(d, true);
      }
      else
      {
        if (!valid_name(tmp_name))
        {
          write_to_output(d, "Invalid account name, please try another.\r\nName: ");
          return;
        }
        CREATE(d->account->name, char, strlen(tmp_name) + 1);
        strcpy(d->account->name, CAP(tmp_name)); /* strcpy: OK (size checked above) */

        write_to_output(d, "Did I get that right, %s (Y/N)?", tmp_name);
        STATE(d) = CON_ACCOUNT_NAME_CONFIRM;
      }
    }
    break;
  case CON_ACCOUNT_NAME_CONFIRM: /* wait for conf. of new name    */
    if (UPPER(*arg) == 'Y')
    {
      if (isbanned(d->host) >= BAN_NEW)
      {
        mudlog(NRM, LVL_STAFF, true, "Request for new account %s denied from [%s] (siteban)", d->account->name, d->host);
        write_to_output(d, "Your site (domain/IP address) has been denied access from making new characters.\r\nPlease contact"
                           "ornir@luminarimud.com if you feel this should not apply to you.\r\n");
        STATE(d) = CON_CLOSE;
        return;
      }
      if (circle_restrict)
      {
        write_to_output(d, "The game is currently closed to making new account.  Please check our website at "
                           "http://www.luminarimud.com/\r\nfor more information.\r\n");
        mudlog(NRM, LVL_STAFF, true, "Request for new account %s denied from [%s] (wizlock)", d->account->name, d->host);
        STATE(d) = CON_CLOSE;
        return;
      }
      write_to_output(d, "New Account.\r\nGive me a Password: ");
      STATE(d) = CON_NEWPASSWD;
    }
    else if (*arg == 'n' || *arg == 'N')
    {
      write_to_output(d, "Okay, what IS it, then? ");
      free(d->account->name);
      d->account->name = NULL;
      STATE(d) = CON_ACCOUNT_NAME;
    }
    else
      write_to_output(d, "Please type Yes or No: ");
    break;

  case CON_ACCOUNT_MENU:
    ProtocolNoEcho(d, false); /* turn echo back on */
    if (d->character)
      free_char(d->character);
    d->character = NULL;

    if (d->character == NULL)
    {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);

      new_mobile_data(d->character);
      GET_HOST(d->character) = strdup(d->host);
      d->character->desc = d;
    }
    switch (*arg)
    {
    case 'C':
    case 'c':
      GET_PREMADE_BUILD_CLASS(d->character) = -1;
      /* Create a new character */
      /* New policy - Character naming. */
      write_to_output(d, "All character names must meet a certain standard of quality in order to foster a more immersive and creative environment.  The staff retains the right to make the final judgment of whether a character name meets this standard.\r\n");
      write_to_output(d, "Some guidelines:\r\n\r\n");
      write_to_output(d, "1. Do not use names such as Joe, Bob, Larry, Carl and so forth.\r\n");
      write_to_output(d, "2.'Exotic' proper names like 'Xavier' and such may be acceptable\r\n");
      write_to_output(d, "2. Do not name yourself after a deity, fictional or otherwise.\r\n");
      write_to_output(d, "3. Do not use the names of well-known fictional characters.\r\n");
      write_to_output(d, "4. Names should fit with the fantasy theme of the mud.\r\n\r\n");
      write_to_output(d, "If we find your name is not suitable for our environment, an immortal staff member will appear before you and offer you a rename.  Please be nice and civil, and we will return with the same. If you need help developing a name for your character there are many websites that generate random names.\r\n\r\n");
      write_to_output(d, "What will your new character be called? : ");
      STATE(d) = CON_GET_NAME;
      return;
    case 'A':
    case 'a':
      /* Add an existing character to this account. */
      write_to_output(d, "Which character you wish to add to this account? : ");
      STATE(d) = CON_ACCOUNT_ADD;
      return;
    case 'q':
    case 'Q':
      write_to_output(d, "Quitting.\r\n");
      STATE(d) = CON_CLOSE;
      return;
    default:
      if (atoi(arg) < 1 || atoi(arg) > (MAX_CHARS_PER_ACCOUNT))
      {
        write_to_output(d, "The number must be between 1 and %d.\r\n", MAX_CHARS_PER_ACCOUNT);
        return;
      }
      else if (d->account->character_names[atoi(arg) - 1] == NULL)
      {
        write_to_output(d, "That character doesn't exist.  Please choose another.  Your Choice: ");
        return;
      }
      else if ((player_i = load_char(d->account->character_names[atoi(arg) - 1], d->character)) > -1)
      {
        GET_PFILEPOS(d->character) = player_i;
        if (PLR_FLAGGED(d->character, PLR_DELETED))
        {
          show_account_menu(d);
          write_to_output(d, "This character has been deleted.  Please speak to an immortal about having the deleted flag removed.\r\n");
          return;
        }
        else
        {
          GET_PFILEPOS(d->character) = player_i;
          if (GET_LEVEL(d->character) < circle_restrict)
          {
            write_to_output(d, "The game is temporarily open for staff members only.  Please refer to the website for more "
                               "intormation.\r\nhttp://www.luminarimud.com/\r\n");
            STATE(d) = CON_CLOSE;
            mudlog(NRM, LVL_STAFF, true, "Request for login denied for %s [%s] (wizlock)", GET_NAME(d->character), d->host);
            return;
          }

          if (AddRecentPlayer(GET_NAME(d->character), d->host, FALSE, FALSE) == FALSE)
          {
            mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "Failure to AddRecentPlayer (returned FALSE).");
          }

          /* undo it just in case they are set */
          REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
          REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
          REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_CRYO);
          d->idle_tics = 0;
          write_to_output(d, "Character loaded.  Press enter to continue.\r\n");

          /* check and make sure no other copies of this player are logged in */
          if (perform_dupe_check(d))
            return;

          if (IS_IMMORTAL(d->character))
            write_to_output(d, "%s\r\n *** PRESS RETURN ***", imotd);
          else
            write_to_output(d, "%s\r\n *** PRESS RETURN ***", motd);

          if (GET_INVIS_LEV(d->character))
            mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), true,
                   "%s [%s] has connected. (invis %d)", GET_NAME(d->character), d->host,
                   GET_INVIS_LEV(d->character));
          else
          {
            //    if (!PRF_FLAGGED(d->character, PRF_ANONYMOUS))
            //      mudlog(BRF, LVL_STAFF, true, "%s has connected.", GET_NAME(d->character));
            mudlog(BRF, LVL_IMMORT, true, "%s [%s] has connected.", GET_NAME(d->character), d->host);
          }
          STATE(d) = CON_RMOTD;
        }
      }
    }

    break;
  case CON_ACCOUNT_ADD: /* wait for input of char name to add to account */
    if (d->character == NULL)
    {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);

      new_mobile_data(d->character);
      /* Allocate mobile event list */
      // d->character->events = create_list();

      GET_HOST(d->character) = strdup(d->host);
      d->character->desc = d;
    }
    if (!*arg)
      STATE(d) = CON_CLOSE;
    else
    {
      char buf[MAX_INPUT_LENGTH] = {'\0'}, tmp_name[MAX_INPUT_LENGTH] = {'\0'};
      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
          strlen(tmp_name) > MAX_NAME_LENGTH || !valid_name(tmp_name) ||
          fill_word(strcpy(buf, tmp_name)) || reserved_word(buf))
      { /* strcpy: OK (mutual MAX_INPUT_LENGTH) */
        write_to_output(d, "Invalid character name.\r\n");
        STATE(d) = CON_ACCOUNT_MENU;
        show_account_menu(d);
        return;
      }
      if ((player_i = load_char(tmp_name, d->character)) > -1)
      {
        /* Player found! */
        char *acct_name = get_char_account_name(GET_NAME(d->character));
        if (acct_name != NULL)
        {
          if (!strcmp(acct_name, d->account->name))
            write_to_output(d, "That character is already linked to your account!\r\n");
          else
            write_to_output(d, "That character is linked to another account.\r\n");
          free(acct_name);
          STATE(d) = CON_ACCOUNT_MENU;
          show_account_menu(d);
          return;
        }
        if (PLR_FLAGGED(d->character, PLR_DELETED))
        {
          write_to_output(d, "That character has been deleted.\r\n");
          STATE(d) = CON_ACCOUNT_MENU;
          show_account_menu(d);
          return;
        }
        /* Now challenge with the password. */
        write_to_output(d, "Please enter the character password for %s : ", GET_NAME(d->character));
        STATE(d) = CON_ACCOUNT_ADD_PWD;
        ProtocolNoEcho(d, true);
        return;
      }
      else
      {
        write_to_output(d, "That character does not exist, please create a new character.\r\n");
        STATE(d) = CON_ACCOUNT_MENU;
        show_account_menu(d);
        return;
      }
    }
    break;
  case CON_GET_NAME: /* wait for input of name */
    if (d->character == NULL)
    {
      CREATE(d->character, struct char_data, 1);
      clear_char(d->character);
      CREATE(d->character->player_specials, struct player_special_data, 1);

      new_mobile_data(d->character);
      /* Allocate mobile event list */
      // d->character->events = create_list();

      GET_HOST(d->character) = strdup(d->host);
      d->character->desc = d;
    }
    if (!*arg)
      STATE(d) = CON_CLOSE;
    else
    {
      char buf[MAX_INPUT_LENGTH] = {'\0'}, tmp_name[MAX_INPUT_LENGTH] = {'\0'};

      if ((_parse_name(arg, tmp_name)) || strlen(tmp_name) < 2 ||
          strlen(tmp_name) > MAX_NAME_LENGTH || !valid_name(tmp_name) ||
          fill_word(strcpy(buf, tmp_name)) || reserved_word(buf))
      { /* strcpy: OK (mutual MAX_INPUT_LENGTH) */
        write_to_output(d, "Invalid name, please try another.\r\nName: ");
        return;
      }
      if ((player_i = load_char(tmp_name, d->character)) > -1)
      {
        GET_PFILEPOS(d->character) = player_i;

        if (PLR_FLAGGED(d->character, PLR_DELETED))
        {
          /* Make sure old files are removed so the new player doesn't get the
           * deleted player's equipment. */
          if ((player_i = get_ptable_by_name(tmp_name)) >= 0)
            remove_player(player_i);

          /* We get a false positive from the original deleted character. */
          free_char(d->character);

          /* Check for multiple creations. */
          if (!valid_name(tmp_name))
          {
            write_to_output(d, "Invalid name, please try another.\r\nName: ");
            return;
          }
          CREATE(d->character, struct char_data, 1);
          clear_char(d->character);
          CREATE(d->character->player_specials, struct player_special_data, 1);

          new_mobile_data(d->character);
          /* Allocate mobile event list */
          // d->character->events = create_list();

          if (GET_HOST(d->character))
            free(GET_HOST(d->character));
          GET_HOST(d->character) = strdup(d->host);

          d->character->desc = d;
          CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
          strcpy(d->character->player.name, CAP(tmp_name)); /* strcpy: OK (size checked above) */
          GET_PFILEPOS(d->character) = player_i;

          /*
            if (d->pProtocol && (d->pProtocol->pVariables[eMSDP_ANSI_COLORS] ||
                 d->pProtocol->pVariables[eMSDP_256_COLORS])) {
              SET_BIT_AR(PRF_FLAGS(d->character), PRF_COLOR_1);
              SET_BIT_AR(PRF_FLAGS(d->character), PRF_COLOR_2);
            }
             */

          write_to_output(d, "Did I get that right, %s (\t(Y\t)/\t(N\t))? ", tmp_name);
          STATE(d) = CON_NAME_CNFRM;
        }
        else
        {
          write_to_output(d, "There is already a character by that name.  Please try another.\r\nName: ");
          return;
        }
      }
      else
      {
        /* player unknown -- make new character */

        /* Check for multiple creations of a character. */
        if (!valid_name(tmp_name))
        {
          write_to_output(d, "Invalid name, please try another.\r\nName: ");
          return;
        }
        CREATE(d->character->player.name, char, strlen(tmp_name) + 1);
        strcpy(d->character->player.name, CAP(tmp_name)); /* strcpy: OK (size checked above) */

        /*
          if (d->pProtocol && (d->pProtocol->pVariables[eMSDP_ANSI_COLORS] ||
               d->pProtocol->pVariables[eMSDP_256_COLORS])) {
            SET_BIT_AR(PRF_FLAGS(d->character), PRF_COLOR_1);
            SET_BIT_AR(PRF_FLAGS(d->character), PRF_COLOR_2);
          }
           */

        write_to_output(d, "Did I get that right, %s (\t(Y\t)/\t(N\t))? ", tmp_name);
        STATE(d) = CON_NAME_CNFRM;
      }
    }
    break;

  case CON_NAME_CNFRM: /* wait for conf. of new name    */
    if (UPPER(*arg) == 'Y')
    {
      if (isbanned(d->host) >= BAN_NEW)
      {
        mudlog(NRM, LVL_STAFF, TRUE, "Request for new char %s denied from [%s] (siteban)", GET_PC_NAME(d->character), d->host);
        write_to_output(d, "Sorry, new characters are not allowed from your site!\r\n");
        STATE(d) = CON_CLOSE;
        return;
      }
      if (circle_restrict)
      {
        write_to_output(d, "Sorry, new players can't be created at the moment.\r\n");
        mudlog(NRM, LVL_STAFF, TRUE, "Request for new char %s denied from [%s] (wizlock)", GET_PC_NAME(d->character), d->host);
        STATE(d) = CON_CLOSE;
        return;
      }
      perform_new_char_dupe_check(d);

      /* dummy check added by zusuk 03/05/13 */
      if (!d->character)
      {
        mudlog(NRM, LVL_STAFF, TRUE, "d->character is NULL (nanny, interpreter.c)");
        write_to_output(d, "Due to conflict, your character did not initialize correctly, please reconnect and try again...\r\n");
        STATE(d) = CON_CLOSE;
        return;
      }

      write_to_output(d, "\r\nWhat is your sex (\t(M\t)/\t(F\t))? ");
      STATE(d) = CON_QSEX;
      break;
    }
    else if (*arg == 'n' || *arg == 'N')
    {
      write_to_output(d, "Okay, what IS it, then? ");
      free(d->character->player.name);
      d->character->player.name = NULL;
      STATE(d) = CON_GET_NAME;
    }
    else
      write_to_output(d, "Please type Yes or No: ");
    break;
  case CON_ACCOUNT_ADD_PWD:
    ProtocolNoEcho(d, true);
    write_to_output(d, "\r\n");

    if (!*arg || strncmp(CRYPT(arg, GET_NAME(d->character)), GET_PASSWD(d->character), MAX_PWD_LENGTH))
      write_to_output(d, "Wrong password.\r\n");
    else
    {
      /* Success! */
      // save_char(d->character, 0);

      for (i = 0; (i < MAX_CHARS_PER_ACCOUNT) && (d->account->character_names[i] != NULL); i++)
        ;
      if (i == MAX_CHARS_PER_ACCOUNT)
      {
        write_to_output(d, "You have reached the maximum number of characters on this account.\r\n"
                           "Remove a character before adding a new one.\r\n");
      }
      else
      {
        d->account->character_names[i] = strdup(GET_NAME(d->character));
        save_account(d->account);
      }
    }
    free_char(d->character);
    show_account_menu(d);
    STATE(d) = CON_ACCOUNT_MENU;
    break;
  case CON_PASSWORD: /* get pwd for known player      */
    /* To really prevent duping correctly, the player's record should be reloaded
     * from disk at this point (after the password has been typed).  However I'm
     * afraid that trying to load a character over an already loaded character is
     * going to cause some problem down the road that I can't see at the moment.
     * So to compensate, I'm going to (1) add a 15 or 20-second time limit for
     * entering a password, and (2) re-add the code to cut off duplicates when a
     * player quits.  JE 6 Feb 96 */

    // echo_on(d);
    ProtocolNoEcho(d, false); /* turn echo back on */

    /* New echo_on() eats the return on telnet. Extra space better than none. */
    write_to_output(d, "\r\n");

    if (!*arg)
      STATE(d) = CON_CLOSE;
    else
    {
      if (strncmp(CRYPT(arg, d->account->name), d->account->password, MAX_PWD_LENGTH))
      {
        mudlog(BRF, LVL_STAFF, TRUE, "Bad PW: %s [%s]", d->account->name, d->host);
        d->account->bad_password_count++;
        // save_char(d->character, 0);
        if (++(d->bad_pws) >= CONFIG_MAX_BAD_PWS)
        { /* 3 strikes and you're out. */
          write_to_output(d, "Wrong password... disconnecting.\r\n");
          STATE(d) = CON_CLOSE;
        }
        else
        {
          write_to_output(d, "Wrong password.\r\nPassword: ");
          // echo_off(d);
          ProtocolNoEcho(d, true);
        }
        return;
      }

      /* Password was correct. */
      load_result = d->account->bad_password_count;
      d->bad_pws = 0;

      if (isbanned(d->host) == BAN_SELECT &&
          !PLR_FLAGGED(d->character, PLR_SITEOK))
      {
        write_to_output(d, "Sorry, this char has not been cleared for login from your site!\r\n");
        STATE(d) = CON_CLOSE;
        mudlog(NRM, LVL_STAFF, TRUE, "Connection attempt for %s denied from %s", GET_NAME(d->character), d->host);
        return;
      }

      if (load_result)
      {
        write_to_output(d, "\r\n\r\n\007\007\007"
                           "\tR%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.\tn\r\n",
                        load_result,
                        (load_result > 1) ? "S" : "");
        d->account->bad_password_count = 0;
      }

      show_account_menu(d);
      STATE(d) = CON_ACCOUNT_MENU;
      break;
      /*
                if (perform_dupe_check(d))
                  return;

                if (GET_LEVEL(d->character) >= LVL_IMMORT)
                  write_to_output(d, "%s", imotd);
                else
                  write_to_output(d, "%s", motd);

                if (GET_INVIS_LEV(d->character))
                  mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "%s has connected. (invis %d)", GET_NAME(d->character), GET_INVIS_LEV(d->character));
                else
                  mudlog(BRF, LVL_IMMORT, TRUE, "%s has connected.", GET_NAME(d->character));
         */
      /* Add to the list of 'recent' players (since last reboot) */
      /*          if (AddRecentPlayer(GET_NAME(d->character), d->host, FALSE, FALSE) == FALSE) {
                  mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE, "Failure to AddRecentPlayer (returned FALSE).");
                }

                if (load_result) {
                  write_to_output(d, "\r\n\r\n\007\007\007"
                          "%s%d LOGIN FAILURE%s SINCE LAST SUCCESSFUL LOGIN.%s\r\n",
                          CCRED(d->character, C_SPR), load_result,
                          (load_result > 1) ? "S" : "", CCNRM(d->character, C_SPR));
                  GET_BAD_PWS(d->character) = 0;
                }
                write_to_output(d, "\r\n*** PRESS RETURN: ");
                STATE(d) = CON_RMOTD;
         */
    }
    break;

  case CON_NEWPASSWD:
  case CON_CHPWD_GETNEW:
    if (!*arg || strlen(arg) > MAX_PWD_LENGTH || strlen(arg) < 3 ||
        !str_cmp(arg, d->account->name))
    {
      write_to_output(d, "\r\nIllegal password.\r\nPassword: ");
      return;
    }
    strncpy(d->account->password, CRYPT(arg, d->account->name), MAX_PWD_LENGTH); /* strncpy: OK (G_P:MAX_PWD_LENGTH+1) */
    *(d->account->password + MAX_PWD_LENGTH) = '\0';

    write_to_output(d, "\r\nPlease retype password: ");
    if (STATE(d) == CON_NEWPASSWD)
      STATE(d) = CON_CNFPASSWD;
    else
      STATE(d) = CON_CHPWD_VRFY;
    break;

  case CON_CNFPASSWD:
  case CON_CHPWD_VRFY:
    if (strncmp(CRYPT(arg, d->account->name), d->account->password,
                MAX_PWD_LENGTH))
    {
      write_to_output(d, "\r\nPasswords don't match... start over.\r\nPassword: ");
      if (STATE(d) == CON_CNFPASSWD)
        STATE(d) = CON_NEWPASSWD;
      else
        STATE(d) = CON_CHPWD_GETNEW;
      return;
    }
    // echo_on(d);
    ProtocolNoEcho(d, false);

    if (STATE(d) == CON_CNFPASSWD)
    {
      save_account(d->account);
      show_account_menu(d);
      STATE(d) = CON_ACCOUNT_MENU;
    }
    else
    {
      write_to_output(d, "\r\nDone.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    }
    break;
    /*
            if (STATE(d) == CON_CNFPASSWD) {
              write_to_output(d, "\r\nWhat is your sex (\t(M\t)/\t(F\t))? ");
              STATE(d) = CON_QSEX;
            } else {
              save_char(d->character, 0);
              write_to_output(d, "\r\nDone.\r\n%s", CONFIG_MENU);
              STATE(d) = CON_MENU;
            }
            break;
       */
  case CON_QSEX: /* query sex of new user         */
    switch (*arg)
    {
    case 'm':
    case 'M':
      d->character->player.sex = SEX_MALE;
      break;
    case 'f':
    case 'F':
      d->character->player.sex = SEX_FEMALE;
      break;
    default:
      write_to_output(d, "That is not a sex..\r\n"
                         "What IS your sex? ");
      return;
    }
#ifdef CAMPAIGN_FR
    write_to_output(d, "Races of Faerun\r\n\r\n");
    for (i = 0; i < NUM_EXTENDED_PC_RACES; i++)
    {
      if ((!is_locked_race(i) || has_unlocked_race(d->character, i)) && race_list[i].is_pc)
        write_to_output(d, "%s\r\n", race_list[i].type);
    }
#else
    write_to_output(d, "Races of Luminari\r\n\r\n");
    for (i = 0; i < NUM_RACES; i++)
    {
      if (!is_locked_race(i) || has_unlocked_race(d->character, i))
        write_to_output(d, "%s\r\n", race_list[i].type);
    }
#endif
    write_to_output(d, "\r\nRace Selection (type 'human' if you do not know "
                       "what to pick): ");
    STATE(d) = CON_QRACE;
    break;

  case CON_QRACE:
    load_result = parse_race_long(arg);
    if (load_result == RACE_UNDEFINED)
    {
      write_to_output(d, "\r\nThat's not a race.\r\nRace: ");
      return;
    }
    else if (is_locked_race(load_result) && !has_unlocked_race(d->character, load_result))
    {
      write_to_output(d, "\r\nYou have not unlocked that race yet, type 'account' "
                         "in-game to view your unlocked races.\r\nRace: ");
      return;
    }
    else
      GET_REAL_RACE(d->character) = load_result;

    switch (load_result)
    {
    case RACE_HUMAN:
      perform_help(d, "race-human");
      break;
    case RACE_ELF:
      perform_help(d, "race-moon-elf");
      break;
#ifdef CAMPAIGN_FR
    case RACE_DWARF:
      perform_help(d, "race-shield-dwarf");
      break;
#else
    case RACE_DWARF:
      perform_help(d, "race-mountain-dwarf");
      break;
#endif
    case RACE_DUERGAR:
      perform_help(d, "race-duergar");
      break;
    case RACE_HALFLING:
      perform_help(d, "race-lightfoot-halfling");
      break;
    case RACE_H_ELF:
      perform_help(d, "race-halfelf");
      break;
    case RACE_H_ORC:
      perform_help(d, "race-halforc");
      break;
    case RACE_GNOME:
      perform_help(d, "race-rock-gnome");
      break;
    case RACE_DROW:
      perform_help(d, "race-drow");
      break;
#ifdef CAMPAIGN_FR
    case RACE_LICH:
      perform_help(d, "race-lich");
      break;
    case RACE_VAMPIRE:
      perform_help(d, "race-vampire");
      break;
#else
    case RACE_HALF_TROLL:
      perform_help(d, "race-half-troll");
      break;
    case RACE_ARCANA_GOLEM:
      perform_help(d, "race-arcana-golem");
      break;
    case RACE_CRYSTAL_DWARF:
      perform_help(d, "race-crystal-dwarf");
      break;
    case RACE_TRELUX:
      perform_help(d, "race-trelux");
      break;
#endif
    case RACE_FAE:
      perform_help(d, "race-fae");
      break;
    case RACE_HIGH_ELF:
      perform_help(d, "race-high-elf");
      break;
#ifdef CAMPAIGN_FR
    case RACE_WOOD_ELF:
      perform_help(d, "race-wood-elf");
      break;
#else
    case RACE_WILD_ELF:
      perform_help(d, "race-wild-elf");
      break;
#endif
    case RACE_HALF_DROW:
      perform_help(d, "race-half-drow");
      break;
    case RACE_DRAGONBORN:
      perform_help(d, "race-dragonborn");
      break;
    case RACE_TIEFLING:
      perform_help(d, "race-tiefling");
      break;
    case RACE_STOUT_HALFLING:
      perform_help(d, "race-stout-halfling");
      break;
    case RACE_FOREST_GNOME:
      perform_help(d, "race-forest-gnome");
      break;
    case RACE_GOLD_DWARF:
      perform_help(d, "race-gold-dwarf");
      break;
    case RACE_AASIMAR:
      perform_help(d, "race-aasimar");
      break;
    case RACE_TABAXI:
      perform_help(d, "race-tabaxi");
      break;
    case RACE_GOLIATH:
      perform_help(d, "race-goliath");
      break;
    case RACE_SHADE:
      perform_help(d, "race-shade");
      break;
    default:
      write_to_output(d, "\r\nCommand not understood.\r\n");
      return;
    }
    write_to_output(d, "Do you want to select this race? (y/n) : ");
    STATE(d) = CON_QRACE_HELP;
    break;

  case CON_QRACE_HELP:

    if (UPPER(*arg) == 'Y')
      write_to_output(d, "\r\nRace Confirmed!\r\n");
    else if (UPPER(*arg) != 'N')
    {
      write_to_output(d, "\r\nY)es to confirm N)o to reselect.\r\n");
      write_to_output(d, "Do you want to select this race? (y/n) : ");
      STATE(d) = CON_QRACE_HELP;
      return;
    }
    else
    {
#ifdef CAMPAIGN_FR
      write_to_output(d, "Races of Faerun\r\n\r\n");
      for (i = 0; i < NUM_EXTENDED_PC_RACES; i++)
#else
      write_to_output(d, "Races of Luminari\r\n\r\n");
      for (i = 0; i < NUM_RACES; i++)
#endif
      {
        if (!is_locked_race(i) || has_unlocked_race(d->character, i))
          write_to_output(d, "%s\r\n", race_list[i].type);
      }
      write_to_output(d, "\r\nRace Selection (type 'human' if you do not know "
                         "what to pick): ");
      STATE(d) = CON_QRACE;
      return;
    }

    /* display class menu */
#ifdef CAMPAIGN_FR
    write_to_output(d, "Classes of Faerun\r\n\r\n");
#else
    write_to_output(d, "Classes of Luminari\r\n\r\n");
#endif

    for (i = 0; i < NUM_CLASSES; i++)
    {
      if (class_list[i].prestige_class)
        continue;

      if (!class_list[i].in_game)
        continue;

      if (CLSLIST_LOCK(i) && !has_unlocked_class(d->character, i))
        continue;

      if (!valid_class_race_alignment(i, GET_REAL_RACE(d->character)))
        continue;

      write_to_output(d, "%-10s - %s\r\n", CLSLIST_NAME(i), class_short_descriptions[i]);
    }

    write_to_output(d, "\r\nClass Selection (type 'warrior' if you do not know "
                       "what to pick): ");

    STATE(d) = CON_QCLASS;

    break;

  case CON_QREGION:

    if (is_abbrev(arg, "quit") || is_abbrev(arg, "Quit"))
    {
      write_to_output(d, "Homeland region selection aborted.\r\n");
      STATE(d) = CON_MENU;
      write_to_output(d, "%s", CONFIG_MENU);
      return;
    }

    load_result = atoi(arg);
    if (load_result <= REGION_NONE || load_result >= NUM_REGIONS)
    {
      write_to_output(d, "\r\nThat's not a region.\r\nRegion: ");
      return;
    }
    else
      GET_REGION(d->character) = load_result;

    display_region_info(d->character, load_result);

    write_to_output(d, "Do you want to select this region? (y/n) : ");
    STATE(d) = CON_QREGION_HELP;

    break;

  case CON_QREGION_HELP:

    if (UPPER(*arg) == 'Y')
      write_to_output(d, "\r\nRegion Confirmed! Press Enter to Continue.\r\n");
    else if (UPPER(*arg) != 'N')
    {
      write_to_output(d, "\r\nY)es to confirm N)o to reselect.\r\n");
      write_to_output(d, "Do you want to select this region? (y/n) : ");
      STATE(d) = CON_QREGION_HELP;
      return;
    }
    else
    {
      write_to_output(d, "\tcRegions of Faerun\tn\r\n\r\n");
      for (i = 1; i < NUM_REGIONS; i++)
      {
        write_to_output(d, "%-2d) %-20s ", i, regions[i]);
        if (((i - 1) % 3) == 2)
          send_to_char(d->character, "\r\n");
      }
      if (((i - 1) % 3) != 2)
        send_to_char(d->character, "\r\n");
      write_to_output(d, "\r\n\r\nRegion selection is mainly a role playign choice, but it also awards an associated language and\r\n"
                         "may be integrated into future game systems.\r\n");
      write_to_output(d, "Type 'quit' to exit out of region selection.\r\n");
      write_to_output(d, "\r\nRegion Selection (select %d for 'Sword Coast' if you do not know what to pick): ", REGION_THE_SWORD_COAST);
      STATE(d) = CON_QREGION;
      return;
    }

    write_to_output(d, "%s", CONFIG_MENU);

    STATE(d) = CON_MENU;
    break;

  case CON_QCLASS:

    load_result = parse_class_long(arg);

    if (load_result == CLASS_UNDEFINED)
    {
      write_to_output(d, "\r\nThat's not a class.\r\nClass: ");
      return;
    }
    else if (CLSLIST_LOCK(load_result) && !has_unlocked_class(d->character, load_result))
    {
      write_to_output(d, "\r\nLocked/prestige classes cannot be taken at 1st level.\r\nClass: ");
      return;
    }
    else if (class_list[load_result].prestige_class)
    {
      write_to_output(d, "\r\nPrestige/locked classes cannot be taken at 1st level.\r\nClass: ");
      return;
    }
    else if (!valid_class_race_alignment(load_result, GET_REAL_RACE(d->character)))
    {
      write_to_output(d, "\r\nDue to your race choice, that class isn't available as a choice (alignment restrictions).\r\nClass: ");
      return;
    }
    else
    {
      GET_CLASS(d->character) = load_result;
    }

    /* display class help files */
    switch (load_result)
    {
    case CLASS_WIZARD:
      perform_help(d, "class-wizard");
      break;
    case CLASS_CLERIC:
      perform_help(d, "class-cleric");
      break;
    case CLASS_ROGUE:
      perform_help(d, "class-rogue");
      break;
      //        case CLASS_ASSASSIN:
      //          perform_help(d, "class-assassin");
      //          break;
      //        case CLASS_SHADOW_DANCER:
      //          perform_help(d, "class-shadowdancer");
      //          break;
    case CLASS_MYSTIC_THEURGE:
      perform_help(d, "class-mystictheurge");
      break;
    case CLASS_WARRIOR:
      perform_help(d, "class-warrior");
      break;
    case CLASS_WEAPON_MASTER:
      perform_help(d, "class-weaponmaster");
      break;
    case CLASS_SHIFTER:
      perform_help(d, "class-shifter");
      break;
    case CLASS_STALWART_DEFENDER:
      perform_help(d, "class-stalwartdefender");
      break;
    case CLASS_DUELIST:
      perform_help(d, "class-duelist");
      break;
    case CLASS_ARCANE_ARCHER:
      perform_help(d, "class-arcanearcher");
      break;
    case CLASS_ARCANE_SHADOW:
      perform_help(d, "class-arcaneshadow");
      break;
    case CLASS_ELDRITCH_KNIGHT:
      perform_help(d, "class-eldritchknight");
      break;
    case CLASS_SACRED_FIST:
      perform_help(d, "class-sacredfist");
      break;
    case CLASS_PALADIN:
      perform_help(d, "class-paladin");
      break;
    case CLASS_BLACKGUARD:
      perform_help(d, "class-blackguard");
      break;
    case CLASS_MONK:
      perform_help(d, "class-monk");
      break;
    case CLASS_RANGER:
      perform_help(d, "class-ranger");
      break;
    case CLASS_DRUID:
      perform_help(d, "class-druid");
      break;
    case CLASS_BERSERKER:
      perform_help(d, "class-berserker");
      break;
    case CLASS_SORCERER:
      perform_help(d, "class-sorcerer");
      break;
    case CLASS_BARD:
      perform_help(d, "class-bard");
      break;
    case CLASS_ALCHEMIST:
      perform_help(d, "class-alchemist");
      break;
    case CLASS_PSIONICIST:
      perform_help(d, "class-psionicist");
      break;
    case CLASS_INQUISITOR:
      perform_help(d, "class-inquisitor");
      break;

    default:
      write_to_output(d, "\r\nCommand not understood.\r\n");
      return;
    }

    write_to_output(d, "Do you want to select this class? (y/n) : ");

    STATE(d) = CON_QCLASS_HELP;

    break;

  case CON_QCLASS_HELP:

    if (UPPER(*arg) == 'Y')
      write_to_output(d, "\r\nClass Confirmed!\r\n");
    else if (UPPER(*arg) != 'N')
    {
      write_to_output(d, "\r\nY)es to confirm N)o to reselect.\r\n");
      write_to_output(d, "Do you want to select this class? (y/n) : ");
      STATE(d) = CON_QCLASS_HELP;
      return;
    }
    else
    {

      /* display class menu */
      write_to_output(d, "Classes of Luminari\r\n\r\n");

      for (i = 0; i < NUM_CLASSES; i++)
      {
        if (class_list[i].prestige_class)
          continue;

        if (CLSLIST_LOCK(i) && !has_unlocked_class(d->character, i))
          continue;

        if (!valid_class_race_alignment(i, GET_REAL_RACE(d->character)))
          continue;

        write_to_output(d, "%-10s - %s\r\n", CLSLIST_NAME(i), class_short_descriptions[i]);
      }

      write_to_output(d, "\r\n *Note the class list may be restricted due to racial alignment restrictions.\r\n"
                         "(Type 'warrior' if you do not know what to pick)  Choice: ");

      STATE(d) = CON_QCLASS;

      return;
    }

    write_to_output(d, "\r\nDo you want to use a premade build or a custom build?\r\n"
                       "A premade build is recommended for new players to the game.  It will choose\r\n"
                       "your skills, feats and ability scores each level.  A custom build means you\r\n"
                       "choose everything as you level up.  Neither choice is set in stone.  You can\r\n"
                       "use the respec command in-game to restart your character from level one without\r\n"
                       "losing anything at all, letting you rebuild your character from level 1 again\r\n"
                       "either as a premade or custom build.\r\n\r\n"
                       "Enter your choice (premade or custom) : ");
    STATE(d) = CON_CONFIRM_PREMADE;
    return;

  case CON_CONFIRM_PREMADE:

    if (is_abbrev(arg, "premade"))
    {
      write_to_output(d, "\r\nPremade Build Confirmed!\r\n");
      GET_PREMADE_BUILD_CLASS(d->character) = GET_CLASS(d->character);
    }
    else if (is_abbrev(arg, "custom"))
    {
      write_to_output(d, "\r\nCustom Build Confirmed!\r\n");
    }
    else
    {
      write_to_output(d, "\r\nPlease specify either premade or custom: ");
      return;
    }

#define CHARGEN_NO_STATISTICS
#ifndef CHARGEN_NO_STATISTICS
    int stat_points = 25;

    /* Next step: Assign base ability scores */
    write_to_output(d, "\r\n");
    write_to_output(d, "Please select your base ability scores :\r\n"
                       "Str: %d%s Dex: %d%s Con: %d%s Int: %d%s Wis: %d%s Cha: %d%s\r\n",
                    GET_STR(d->character), "",
                    GET_DEX(d->character), "",
                    GET_CON(d->character), "",
                    GET_INT(d->character), "",
                    GET_WIS(d->character), "",
                    GET_CHA(d->character), "");
    write_to_output(d, "[%d points left] Enter ability score, Q (done) or ? for help : ", stat_points);

  case CON_QSTATS:
    /* This is displayed when the player has changed an ability score,
     * so we need to capture the changed score and apply changes, then
     * display the remaining points.
     *
     * Formula is : Start at 8, 1:1 to 14, 1:2 to 16, 1:3 to 18
     * Standard point buy is 25.
     *
     * If the value is '?' then we need to get help.  If the value is 'Q'
     * then we get confirmation and then move on to the next step. */
    if (UPPER(*arg) == 'Q')
      write_to_output(d, "\r\nBase ability scores set!\r\n");
    else if (UPPER(*arg) == '?')
    {
      perform_help(d, "point-buy");
      break;
    }
    else
    {
      /* Here is where we check for ability names, etc. */
      if (is_abbrev(arg, "str"))
      {
        /* Calculate the maximum we can set this stat to (based on points) and display
         * that to the player. Don't forget the racial bonuses! */
      }
      if (is_abbrev(arg, "dex"))
      {
      }
      if (is_abbrev(arg, "con"))
      {
      }
      if (is_abbrev(arg, "int"))
      {
      }
      if (is_abbrev(arg, "wis"))
      {
      }
      if (is_abbrev(arg, "cha"))
      {
      }
    }
#endif
    /* start initial alignment selection code */
    write_to_output(d, "\r\nSelect Alignment\r\n"
                       "*Note: you may be restricted by your race/class.  If you don't know which to select, select 'true neutral'\r\n"
                       "\r\n- Good characters and creatures protect innocent life. Evil characters "
                       "and creatures debase or destroy innocent life, whether for fun or profit.\r\n"
                       "\r\n- Lawful characters tell the truth, keep their word, respect authority, "
                       "honor tradition, and judge those who fall short of their duties. Chaotic "
                       "characters follow their consciences, resent being told what to do, favor "
                       "new ideas over tradition, and do what they promise if they feel like it.\r\n\r\n");
    for (i = 0; i < NUM_ALIGNMENTS; i++)
    {
      if (valid_align_by_class(i, GET_CLASS(d->character)) &&
          valid_align_by_race(i, GET_REAL_RACE(d->character)))
        write_to_output(d, "%d) %s\r\n", i, alignment_names[i]);
    }
    write_to_output(d, "\r\n");

    STATE(d) = CON_QALIGN;
    break;

  case CON_QALIGN:

    if (!isdigit(*arg))
    {
      write_to_output(d, "That is not a number!\r\n");
      STATE(d) = CON_QALIGN;
      return;
    }

    i = atoi(arg);
    if (i < 0 || i > (NUM_ALIGNMENTS - 1) ||
        !valid_align_by_class(i, GET_CLASS(d->character)) ||
        !valid_align_by_race(i, GET_REAL_RACE(d->character)))
    {
      write_to_output(d, "\r\nInvalid Choice!  Please Select Alignment\r\n");
      for (i = 0; i < NUM_ALIGNMENTS; i++)
      {
        if (valid_align_by_class(i, GET_CLASS(d->character)) &&
            valid_align_by_race(i, GET_REAL_RACE(d->character)))
          write_to_output(d, "%d) %s\r\n", i, alignment_names[i]);
      }
      write_to_output(d, "\r\n");

      STATE(d) = CON_QALIGN;
      return;
    }
    else
    {
      set_alignment(d->character, i);
      write_to_output(d, "\r\nAlignment Selected!\r\n");
    }

    /******************************/
    /* ok begin processing player */

    /* dummy check for olc state */
    if (d->olc)
    {
      free(d->olc);
      d->olc = NULL;
    }

    if (GET_PFILEPOS(d->character) < 0)
      GET_PFILEPOS(d->character) = create_entry(GET_PC_NAME(d->character));

    /* Now GET_NAME() will work properly. */
    init_char(d->character);
    save_char(d->character, 0);
    save_account(d->account);
    save_player_index();

    /* print message of the day to player */
    write_to_output(d, "%s\r\n*** PRESS RETURN: ", motd);
    STATE(d) = CON_RMOTD;

    /* make sure the last log is updated correctly. */
    GET_PREF(d->character) = rand_number(1, 128000);
    GET_HOST(d->character) = strdup(d->host);

    mudlog(NRM, LVL_STAFF, TRUE, "%s [%s] new player.", GET_NAME(d->character), d->host);

    /* Add to the list of 'recent' players (since last reboot) */
    if (AddRecentPlayer(GET_NAME(d->character), d->host, TRUE, FALSE) == FALSE)
    {
      mudlog(BRF, MAX(LVL_IMMORT, GET_INVIS_LEV(d->character)), TRUE,
             "Failure to AddRecentPlayer (returned FALSE).");
    }

    break;

  case CON_RMOTD: /* read CR after printing motd   */
    write_to_output(d, "%s", CONFIG_MENU);
    if (IS_HAPPYHOUR > 0)
    {
      write_to_output(d, "\r\n");
      write_to_output(d, "\tyThere is currently a Happyhour!\tn\r\n");
      write_to_output(d, "\r\n");
    }
    if (IS_STAFF_EVENT)
    {
      write_to_output(d, "\r\n");
      write_to_output(d, "\tyA staff-ran event is taking place!\tn\r\n");
      write_to_output(d, "\r\n");
    }
    add_llog_entry(d->character, LAST_CONNECT);
    STATE(d) = CON_MENU;

    break;

  case CON_GEN_DESCS_INTRO:
    HandleStateGenericsDescsIntro(d, arg);
    break;

  case CON_GEN_DESCS_DESCRIPTORS_1:
    HandleStateGenericDescsDescriptors1(d, arg);
    break;

  case CON_GEN_DESCS_ADJECTIVES_1:
    HandleStateGenericDescsAdjectives1(d, arg);
    break;

  case CON_GEN_DESCS_DESCRIPTORS_2:
    HandleStateGenericDescsDescriptors2(d, arg);
    break;

  case CON_GEN_DESCS_ADJECTIVES_2:
    HandleStateGenericDescsAdjectives2(d, arg);
    break;

  case CON_GEN_DESCS_MENU:
    HandleStateGenericDescsMenu(d, arg);
    break;

  case CON_GEN_DESCS_MENU_PARSE:
    HandleStateGenericDescsParseMenuChoice(d, arg);
    break;

  case CON_MENU:
  { /* get selection from main menu  */

    switch (*arg)
    {
    case '0':
      //          write_to_output(d, "Goodbye.\r\n");
      add_llog_entry(d->character, LAST_QUIT);
      //          STATE(d) = CON_CLOSE;
      STATE(d) = CON_ACCOUNT_MENU;
      show_account_menu(d);
      save_char(d->character, 0);
      free_char(d->character);
      break;

    case '1':
      load_result = enter_player_game(d);
      send_to_char(d->character, "%s", CONFIG_WELC_MESSG);

      /* Clear their load room if it's not persistant. */
      if (!PLR_FLAGGED(d->character, PLR_LOADROOM))
        GET_LOADROOM(d->character) = NOWHERE;
      save_char(d->character, 0);

      greet_mtrigger(d->character, -1);
      greet_memory_mtrigger(d->character);

      act("$n has entered the game.", TRUE, d->character, 0, 0, TO_ROOM);

      STATE(d) = CON_PLAYING;
      // MXPSendTag( d, "<VERSION>" ); this is already called in perform_dupe_check() before we get here, shouldn't be needed here.. -Nashak
      if (GET_LEVEL(d->character) == 0)
      {
        do_start(d->character);
        newbieEquipment(d->character);
        send_to_char(d->character, "%s", CONFIG_START_MESSG);
      }
      look_at_room(d->character, 0);
      if (has_mail(GET_IDNUM(d->character)))
        send_to_char(d->character, "You have mail waiting.\r\n");
      if (load_result == 2)
      { /* rented items lost */
        send_to_char(d->character, "\r\n\007You could not afford your rent!\r\n"
                                   "Your possesions have been donated to the Salvation Army!\r\n");
      }
      d->has_prompt = 0;
      /* We've updated to 3.1 - some bits might be set wrongly: */
      REMOVE_BIT_AR(PRF_FLAGS(d->character), PRF_BUILDWALK);

      /* being extra careful, init memming status */
      int x;
      for (x = 0; x < NUM_CASTERS; x++)
        IS_PREPARING(d->character, x) = FALSE;
      break;

    case '2':
      if (d->character->player.description)
      {
        write_to_output(d, "Current description:\r\n%s", d->character->player.description);
        /* Don't free this now... so that the old description gets loaded as the
         * current buffer in the editor.  Do setup the ABORT buffer here, however. */
        d->backstr = strdup(d->character->player.description);
      }
      write_to_output(d, "Enter the new text you'd like others to see when they look at you.\r\n");
      send_editor_help(d);
      d->str = &d->character->player.description;
      d->max_str = PLR_DESC_LENGTH;
      STATE(d) = CON_PLR_DESC;
      break;

#ifdef CAMPAIGN_FR
    case '3':
      if (d->character->player.background)
      {
        write_to_output(d, "Current Character Background:\r\n%s", d->character->player.background);
        /* Don't free this now... so that the old background gets loaded as the
         * current buffer in the editor.  Do setup the ABORT buffer here, however. */
        d->backstr = strdup(d->character->player.background);
      }
      write_to_output(d, "Enter your character background story.\r\n");
      send_editor_help(d);
      d->str = &d->character->player.background;
      d->max_str = PLR_BG_LENGTH;
      STATE(d) = CON_PLR_BG;
      break;

    case '4':

      if (GET_REGION(d->character))
      {
        write_to_output(d, "\r\n\tcYou have already chosen a homeland region.  To change it you will need to ask a staff member to do it.\r\n\r\n\tn");
        return;
      }

      write_to_output(d, "\tcRegions of Faerun\tn\r\n\r\n");
      for (i = 1; i < NUM_REGIONS; i++)
      {
        write_to_output(d, "%-2d) %-20s ", i, regions[i]);
        if (((i - 1) % 3) == 2)
          send_to_char(d->character, "\r\n");
      }
      if (((i - 1) % 3) != 2)
        send_to_char(d->character, "\r\n");
      write_to_output(d, "\r\n\r\nRegion selection is mainly a role playign choice, but it also awards an associated language and\r\n"
                         "may be integrated into future game systems.\r\n");
      write_to_output(d, "Type 'quit' to exit out of region selection.\r\n");
      write_to_output(d, "\r\nRegion Selection (select %d for 'Sword Coast' if you do not know what to pick): ", REGION_THE_SWORD_COAST);

      STATE(d) = CON_QREGION;
      break;

    case '5':
      if (GET_PC_DESCRIPTOR_1(d->character) > 0 && GET_PC_ADJECTIVE_1(d->character) > 0)
      {
        send_to_char(d->character, "\r\n");
        send_to_char(d->character, "You have already chosen your description.  To change it you'll need to request a description reset from a staff member.\r\n");
        send_to_char(d->character, "\r\n");
        break;
      }
      send_to_char(d->character, "\r\n");
      send_to_char(d->character, "SET CHARACTER SHORT DESCRIPTION: PRESS ENTER\r\n");
      send_to_char(d->character, "\r\n");
      STATE(d) = CON_GEN_DESCS_INTRO;
      break;

    case '6':
      page_string(d, background, 0);
      STATE(d) = CON_RMOTD;
      break;

    case '7':
      write_to_output(d, "\r\nEnter your old password: ");
      // echo_off(d);
      ProtocolNoEcho(d, true);
      STATE(d) = CON_CHPWD_GETOLD;
      break;

    case '8':
      write_to_output(d, "\r\nEnter your password for verification: ");
      // echo_off(d);
      ProtocolNoEcho(d, true);
      STATE(d) = CON_DELCNF1;
      break;
#else

    case '3':
      page_string(d, background, 0);
      STATE(d) = CON_RMOTD;
      break;

    case '4':
      write_to_output(d, "\r\nEnter your old password: ");
      // echo_off(d);
      ProtocolNoEcho(d, true);
      STATE(d) = CON_CHPWD_GETOLD;
      break;

    case '5':
      write_to_output(d, "\r\nEnter your password for verification: ");
      // echo_off(d);
      ProtocolNoEcho(d, true);
      STATE(d) = CON_DELCNF1;
      break;
#endif
    default:
      write_to_output(d, "\r\nThat's not a menu choice!\r\n%s", CONFIG_MENU);
      break;
    }
    break;
  }

  case CON_CHPWD_GETOLD:
    if (strncmp(CRYPT(arg, d->account->password), d->account->password, MAX_PWD_LENGTH))
    {
      // echo_on(d);
      ProtocolNoEcho(d, false);
      write_to_output(d, "\r\nIncorrect password.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    }
    else
    {
      write_to_output(d, "\r\nEnter a new password: ");
      STATE(d) = CON_CHPWD_GETNEW;
    }
    return;

  case CON_DELCNF1:
    // echo_on(d);
    ProtocolNoEcho(d, false);
    if (strncmp(CRYPT(arg, d->account->password), d->account->password, MAX_PWD_LENGTH))
    {
      write_to_output(d, "\r\nIncorrect password.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    }
    else
    {
      write_to_output(d, "\r\nYOU ARE ABOUT TO DELETE THIS CHARACTER PERMANENTLY.\r\n"
                         "ARE YOU ABSOLUTELY SURE?\r\n\r\n"
                         "Please type \"yes\" to confirm: ");
      STATE(d) = CON_DELCNF2;
    }
    break;

  case CON_DELCNF2:
    if (!strcmp(arg, "yes") || !strcmp(arg, "YES"))
    {
      if (PLR_FLAGGED(d->character, PLR_FROZEN))
      {
        write_to_output(d, "You try to kill yourself, but the ice stops you.\r\n"
                           "Character not deleted.\r\n\r\n");
        STATE(d) = CON_CLOSE;
        return;
      }
      if (GET_LEVEL(d->character) < LVL_GRSTAFF)
        SET_BIT_AR(PLR_FLAGS(d->character), PLR_DELETED);
      save_char(d->character, 0);
      Crash_delete_file(GET_NAME(d->character));
      /* If the selfdelete_fastwipe flag is set (in config.c), remove all the
       * player's immediately. */
      if (selfdelete_fastwipe)
        if ((player_i = get_ptable_by_name(GET_NAME(d->character))) >= 0)
        {
          SET_BIT(player_table[player_i].flags, PINDEX_SELFDELETE);
          remove_player(player_i);
        }
      remove_char_from_account(d->character, d->account);
      delete_variables(GET_NAME(d->character));
      write_to_output(d, "Character '%s' deleted!\r\n", GET_NAME(d->character));
      mudlog(NRM, LVL_STAFF, TRUE, "%s (lev %d) has self-deleted.", GET_NAME(d->character), GET_LEVEL(d->character));
      STATE(d) = CON_ACCOUNT_MENU;
      show_account_menu(d);
      return;
    }
    else
    {
      write_to_output(d, "\r\nCharacter not deleted.\r\n%s", CONFIG_MENU);
      STATE(d) = CON_MENU;
    }
    break;

    /* It is possible, if enough pulses are missed, to kick someone off while they
     * are at the password prompt. We'll let the game_loop()axe them. */
  case CON_CLOSE:
    break;

  default:
    log("SYSERR: Nanny: illegal state of con'ness (%d) for '%s'; closing connection.",
        STATE(d), d->character ? GET_NAME(d->character) : "<unknown>");
    STATE(d) = CON_DISCONNECT; /* Safest to do. */
    break;
  }
}
