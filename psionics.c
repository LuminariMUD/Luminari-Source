/** 
 * Main file for psionic abilities following pathfinder rules
 * found at d20pfsrd.com.  Created by Gicker aka Stephen Squires
 * February 2021 
 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "comm.h"
#include "db.h"
#include "dg_scripts.h"
#include "fight.h"
#include "constants.h"
#include "mud_event.h"
#include "spec_procs.h"
#include "class.h"
#include "actions.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "grapple.h"
#include "spell_prep.h"
#include "alchemy.h"
#include "missions.h"
#include "psionics.h"

struct psionic_power_data psionic_powers[MAX_SPELLS+1];

void spello(int spl, const char *name, int max_psp, int min_psp,
            int psp_change, int minpos, int targets, int violent, int routines,
            const char *wearoff, int time, int memtime, int school, bool quest);


void unused_psionic_power(int pwr)
{
    psionic_powers[pwr].psp_cost = 0;
    psionic_powers[pwr].can_augment = false;
    psionic_powers[pwr].power_type = PSIONIC_POWER_TYPE_NONE;
}

void psiono(int pwr, const char *name, int psp_cost, bool can_augment, int power_type, int targets, int violent, int routines, const char *wearoff, int time)
{
    psionic_powers[pwr].psp_cost = psp_cost;
    psionic_powers[pwr].can_augment = can_augment;
    psionic_powers[pwr].power_type = power_type;
    spello(pwr, name, 0, 0, 0, POS_FIGHTING, targets, violent, routines, wearoff, time, 0, NOSCHOOL, false);
}

void assign_psionic_powers(void)
{
    int i = 0;

    for (i = 0; i <= TOP_SPELL_DEFINE; i++)
        unused_psionic_power(i);

    // level 1 psionic powers
    psiono(PSIONIC_BROKER, "broker", 1, true, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic persuasive edge expires.", 1);
    psiono(PSIONIC_CALL_TO_MIND, "call to mind", 1, true, TELEPATHY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic knowledge of lore fades away.", 1);
    psiono(PSIONIC_CATFALL, "catfall", 1, true, PSYCHOPORTATION, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your ability to catfall hs expired.", 1);
    psiono(PSIONIC_CRYSTAL_SHARD, "crystal shard", 1, true, METACREATIVITY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 1);
    psiono(PSIONIC_DECELERATION, "deceleration", 1, true, PSYCHOPORTATION, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "Your psychic deceleration expires.", 1);
    psiono(PSIONIC_DEMORALIZE, "demoralize", 1, true, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_MASSES, "You are no longer demoralized.", 1);
    psiono(PSIONIC_ECTOPLASMIC_SHEEN, "ectoplasmic sheen", 1, true, METACREATIVITY, TAR_IGNORE, false, MAG_ROOM, "The ectoplasmic sheen evaporates completely.", 1);
    psiono(PSIONIC_ENERGY_RAY, "energy ray", 1, true, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 1);
    psiono(PSIONIC_FORCE_SCREEN, "force screen", 1, true, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your force screen blinks out of existence.", 1);
    psiono(PSIONIC_FORTIFY, "fortify", 1, true, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic fortification has subsided.", 1);
    psiono(PSIONIC_INERTIAL_ARMOR, "inertial armor", 1, true, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your inertial armor blinks out of existence.", 1);
    psiono(PSIONIC_INEVITABLE_STRIKE, "inevitable strike", 1, true, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "You expend your inevitable strike!", 1);
    psiono(PSIONIC_MIND_THRUST, "mind thrust", 1, true, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 1);
    psiono(PSIONIC_DEFENSIVE_PRECOGNITION, "defensive precognition", 1, true, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your defensive precognition expires.", 1);
    psiono(PSIONIC_OFFENSIVE_PRECOGNITION, "offensive precognition", 1, true, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your offensive precognition expires.", 1);
    psiono(PSIONIC_OFFENSIVE_PRESCIENCE, "offensive prescience", 1, true, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your offensive prescience expires.", 1);
    psiono(PSIONIC_SLUMBER, "slumber", 1, true, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "You are no longer compelled to slumber.", 1);
    psiono(PSIONIC_VIGOR, "vigor", 1, true, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic vigor subsides.", 1);

    // level 2 psionic powers
    psiono(PSIONIC_BESTOW_POWER, "bestow power", 3, true, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, false, MAG_POINTS, NULL, 2);
    psiono(PSIONIC_BIOFEEDBACK, "biofeedback", 3, true, TELEPATHY, TAR_CHAR_ROOM, false, MAG_AFFECTS, "You feel your biofeedback protection expire.", 2);
    psiono(PSIONIC_BODY_EQUILLIBRIUM, "body equillibrium", 3, false, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "You feel your body equillibrium fade.", 2);
    psiono(PSIONIC_BREACH, "breach", 3, false, PSYCHOPORTATION, TAR_IGNORE, false, MAG_MANUAL, NULL, 2);
    psiono(PSIONIC_CONCEALING_AMORPHA, "concealing amorpha", 3, true, METACREATIVITY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_GROUPS, "The concealing amorpha surrounding you dissipates.", 2);
    psiono(PSIONIC_CONCUSSION_BLAST, "concussion blast", 3, true, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AREAS, NULL, 2);
    psiono(PSIONIC_DETECT_HOSTILE_INTENT, "detect hostile intent", 3, false, TELEPATHY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic ability to detect nearby danger has passed.", 2);
    psiono(PSIONIC_ELFSIGHT, "elfsight", 3, false, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your elfsight wears off.", 2);
    psiono(PSIONIC_ENERGY_ADAPTATION_SPECIFIED, "specified energy adaptation", 3, true, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your specified energy adaptation has expired.", 2);
    psiono(PSIONIC_ENERGY_PUSH, "energy push", 3, true, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 2);
    psiono(PSIONIC_ENERGY_STUN, "energy stun", 3, true, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AREAS, NULL, 2);
    psiono(PSIONIC_INFLICT_PAIN, "inflict pain", 3, true, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "The psychic pain tormenting you finally ends.", 2);
    psiono(PSIONIC_MENTAL_DISRUPTION, "mental disruption", 3, true, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_MASSES, "THe psychic mental disruption affecting you expires.", 2);
    psiono(PSIONIC_PSIONIC_LOCK, "psionic lock", 3, false, PSYCHOPORTATION, TAR_IGNORE, false, MAG_MANUAL, NULL, 2);
    psiono(PSIONIC_PSYCHIC_BODYGUARD, "psychic bodyguard", 3, true, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, false, MAG_AFFECTS, "Your psychic bodyguard affect expires.", 2);
    psiono(PSIONIC_RECALL_AGONY, "recall agony", 3, true, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 2);
    psiono(PSIONIC_SHARE_PAIN, "share pain", 3, false, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_NOT_SELF, false, MAG_AFFECTS, "You are no longer sharing pain weith you ally.", 2);
    psiono(PSIONIC_SWARM_OF_CRYSTALS, "swarm of crystals", 3, true, METACREATIVITY, TAR_CHAR_ROOM |TAR_NOT_SELF, true, MAG_AREAS, NULL, 2);
    psiono(PSIONIC_THOUGHT_SHIELD, "thought shield", 3, true, TELEPATHY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your thought shield expires.", 2);

    // level 3 psionic powers
    psiono(PSIONIC_BODY_ADJUSTMENT, "body adjustment", 5, true, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_POINTS, NULL, 3);
    psiono(PSIONIC_CONCUSSIVE_ONSLAUGHT, "concussive onslaught", 5, true, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AREAS, NULL, 3);
    psiono(PSIONIC_DISPEL_PSIONICS, "dispel psionic", 5, true, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, false, MAG_MANUAL, NULL, 3);
    psiono(PSIONIC_ENDORPHIN_SURGE, "endorphin surge", 5, true, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your endorphin surge wears off.", 3);
    psiono(PSIONIC_ENERGY_BURST, "energy burst", 5, true, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AREAS, NULL, 3);
    psiono(PSIONIC_ENERGY_RETORT, "energy retort", 5, true, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your energy retort protection fades.", 3);
    psiono(PSIONIC_ERADICATE_INVISIBILITY, "eradicate invisibility", 5, true, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, false, MAG_MASSES, NULL, 3);
    psiono(PSIONIC_HEIGHTENED_VISION, "heightened vision", 5, false, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your heightened vision returns to normal.", 3);
    psiono(PSIONIC_MENTAL_BARRIER, "mental barrier", 5, true, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your mental barrier blinks out of existence.", 3);
    psiono(PSIONIC_MIND_TRAP, "mind trap", 5, true, TELEPATHY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your mind trap triggers!", 3);
    psiono(PSIONIC_PSIONIC_BLAST, "psionic blast", 5, true, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_MASSES, "The stunning affect of being hit by a psionic blast expires.", 3);
    psiono(PSIONIC_FORCED_SHARED_PAIN, "forced shared pain", 5, true, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "You're no longer being forced to share pain.", 3);
    psiono(PSIONIC_SHARPENED_EDGE, "sharpened edge", 5, false, METACREATIVITY, TAR_CHAR_ROOM, false, MAG_AFFECTS, "The sharpened edge of your weapons fades.", 3);
    psiono(PSIONIC_UBIQUITUS_VISION, "ubiquitus vision", 5, false, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your ubiquitus vision fades.", 3);

    // level 4 psionic powers
    psiono(PSIONIC_DEADLY_FEAR, "deadly fear", 7, true, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 4);
    psiono(PSIONIC_DEATH_URGE, "death urge", 7, true, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "You are longer compelled to harm yourself.", 4);
    psiono(PSIONIC_EMPATHIC_FEEDBACK, "empathetic feedback", 7, true, TELEPATHY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your empathetic feedback protection ends.", 4);
    psiono(PSIONIC_ENERGY_ADAPTATION, "energy adaptation", 7, true, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your specified energy adaptation has expired.", 4);
    psiono(PSIONIC_INCITE_PASSION, "incite passion", 7, false, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "Your irrationally heightened animal instics return to normal.", 4);
    psiono(PSIONIC_INTELLECT_FORTRESS, "intellect fortress", 7, true, PSYCHOKINESIS, TAR_CHAR_ROOM, false, MAG_GROUPS, "The intellect fortress guarding your mind expires.", 4);
    psiono(PSIONIC_MOMENT_OF_TERROR, "moment of terror", 7, true, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "Your terrors finally fade away.", 4);
    psiono(PSIONIC_POWER_LEECH, "power leech", 7, false, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "You finally expel the power leech from your psyche.", 4);
    psiono(PSIONIC_SLIP_THE_BONDS, "slip the bonds", 7, false, PSYCHOPORTATION, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic ability to slip bonds has expired.", 4);
    psiono(PSIONIC_WITHER, "wither", 7, true, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "Your withered limbs and body finally return to normal.", 4);
    psiono(PSIONIC_WALL_OF_ECTOPLASM, "wall of ectoplasm", 7, false, METACREATIVITY, TAR_IGNORE, false, MAG_MANUAL, "The wall of ectoplasm hardens and falls to dust.", 4);

    // level 5 psionic powers
    psiono(PSIONIC_ADAPT_BODY, "adapt body", 9, false, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic adaptability has ended.", 5);
    psiono(PSIONIC_ECTOPLASMIC_SHAMBLER, "ectoplasmic shambler", 9, false, METACREATIVITY, TAR_IGNORE, false, MAG_SUMMONS, NULL, 5);
    psiono(PSIONIC_PIERCE_VEIL, "pierce the veils", 9, false, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your ability to pierce hidden veils has expired.", 5);
    psiono(PSIONIC_PLANAR_TRAVEL, "planar travel", 9, false, PSYCHOPORTATION, TAR_IGNORE, false, MAG_CREATIONS, NULL, 5);
    psiono(PSIONIC_POWER_RESISTANCE, "power resistance", 9, false, CLAIRSENTIENCE, TAR_CHAR_ROOM, false, MAG_AFFECTS, "Your power resistance has faded.", 5);
    psiono(PSIONIC_PSYCHIC_CRUSH, "psychic crush", 9, true, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 5);
    psiono(PSIONIC_PSYCHOPORTATION, "psychoport", 9, false, PSYCHOPORTATION, TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_MANUAL, NULL, 5);
    psiono(PSIONIC_SHATTER_MIND_BLANK, "shatter mind blank", 9, true, TELEPATHY, TAR_IGNORE, true, MAG_MASSES, NULL, 5);
    psiono(PSIONIC_SHRAPNEL_BURST, "shrapnel burst", 9, true, METACREATIVITY, TAR_IGNORE, true, MAG_AREAS, NULL, 5);
    psiono(PSIONIC_TOWER_OF_IRON_WILL, "tower of iron will", 9, true, TELEPATHY, TAR_IGNORE, false, MAG_GROUPS, "The tower of iron will protecting your mind fades.", 5);
    psiono(PSIONIC_UPHEAVAL, "upheaval", 9, true, PSYCHOPORTATION, TAR_IGNORE, true, MAG_AREAS | MAG_ROOM, "The rubble blocking movement in this area turns to dust.", 5);

    // level 6 psionic powers
    psiono(PSIONIC_BREATH_OF_THE_BLACK_DRAGON, "breath of the black dragon", 11, true, PSYCHOMETABOLISM, TAR_IGNORE, true, MAG_AREAS, NULL, 6);
    psiono(PSIONIC_BRUTALIZE_WOUNDS, "brutalize wounds", 11, false, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "The psychic brutalizing of your wounds suddenly ends.", 6);
    psiono(PSIONIC_DISINTEGRATION, "disintegration", 11, true, PSYCHOPORTATION, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 6);
    psiono(PSIONIC_REMOTE_VIEW_TRAP, "remote view trap", 11, false, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your remote view trap expires.", 6);
    psiono(PSIONIC_SUSTAINED_FLIGHT, "sustained flight", 11, false, PSYCHOPORTATION, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your sustained flight ends.", 6);

    // level 7 psionic powers
    psiono(PSIONIC_BARRED_MIND_PERSONAL, "personal barred mind", 13, true, TELEPATHY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your barred mind effect passes.", 7);
    psiono(PSIONIC_COSMIC_AWARENESS, "cosmic awareness", 13, true, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your cosmic awareness expires.", 7);
    psiono(PSIONIC_ENERGY_CONVERSION, "energy conversion", 13, false, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your energy conversion ability has expired.", 7);
    psiono(PSIONIC_ENERGY_WAVE, "energy wave", 13, true, PSYCHOKINESIS, TAR_IGNORE, true, MAG_AREAS, NULL, 7);
    psiono(PSIONIC_EVADE_BURST, "evade burst", 13, true, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your evade burst ability has ended.", 7);
    psiono(PSIONIC_OAK_BODY, "oak body", 13, true, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your oak body returns to normal.", 7);
    psiono(PSIONIC_PSYCHOSIS, "psychosis", 13, true, TELEPATHY, TAR_CHAR_ROOM |TAR_NOT_SELF, true, MAG_AFFECTS, "You no longer feel any signs of psychosis.", 7);
    psiono(PSIONIC_ULTRABLAST, "ultrabast", 13, true, TELEPATHY, TAR_IGNORE, true, MAG_AREAS, NULL, 7);

    // level 8 psionic powers
    psiono(PSIONIC_BARRED_MIND, "barred mind", 15, true, TELEPATHY, TAR_CHAR_ROOM, false, MAG_AFFECTS, "Your barred mind effect passes.", 8);
    psiono(PSIONIC_BODY_OF_IRON, "body of iron", 15, false, METACREATIVITY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your iron body returns to normal.", 8);
    psiono(PSIONIC_PSYCHOPORT_GREATER, "greater psychoport", 15, false, PSYCHOPORTATION, TAR_CHAR_WORLD, false, MAG_MANUAL, NULL, 8);
    psiono(PSIONIC_RECALL_DEATH, "recall death", 15, true, CLAIRSENTIENCE, TAR_CHAR_ROOM, true, MAG_DAMAGE, NULL, 8);
    psiono(PSIONIC_SHADOW_BODY, "shadow body", 15, false, PSYCHOMETABOLISM, TAR_CHAR_ROOM|TAR_SELF_ONLY, false, MAG_AFFECTS, "Your shadowed-body returns to normal.", 8);
    psiono(PSIONIC_TRUE_METABOLISM, "true metabolism", 15, false, PSYCHOMETABOLISM, TAR_CHAR_ROOM|TAR_SELF_ONLY, false, MAG_AFFECTS, "Your enhanced metabolism returns to normal.", 8);

    // level 9 psionic powers
    psiono(PSIONIC_APOPSI, "apopsi", 17, false, TELEPATHY, TAR_CHAR_ROOM|TAR_NOT_SELF, true, MAG_AFFECTS, "Your addled mind returns to normal.", 9);
    psiono(PSIONIC_ASSIMILATE, "assimilate", 17, false, PSYCHOMETABOLISM, TAR_CHAR_ROOM|TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 9);
    psiono(PSIONIC_TIMELESS_BODY, "timeless body", 17, false, PSYCHOPORTATION, TAR_CHAR_ROOM|TAR_SELF_ONLY, false, MAG_AFFECTS, "Your timeless body effect expires.", 9);
}

ACMD(do_manifest)
{

    char augment[200], pass_arg[200];

    half_chop((char *)argument, augment, pass_arg);

    if (!*augment || !*pass_arg)
    {
        send_to_char(ch, "You must specify the number of power points to augment your power with, or 0.\r\n"
                         "You must follow that with the power name in single quotes, optionally followed by the target where applicable.\r\n"
                         "Example: manifest 2 'mind thrust' ogre\r\n"
                         "This will manifest the mind thrust against the first target named ogre and with the power augmented with 2 power points.\r\n"
        );
        return;
    }
    if (!isdigit(*augment))
    {
        send_to_char(ch, "You must specify the number of power points to augment your power with, or 0.\r\n"
                         "You must follow that with the power name in single quotes, optionally followed by the target where applicable.\r\n"
                         "Example: manifest 2 'mind thrust' ogre\r\n"
                         "This will manifest the mind thrust against the first target named ogre and with the power augmented with 2 power points.\r\n"
        );
        return;
    }
    int extra_pp = atoi(augment);
    GET_AUGMENT_PSP(ch) = extra_pp;
    do_gen_cast(ch, (const char *) pass_arg, cmd, SCMD_CAST_PSIONIC);
}

int max_augment_psp_allowed(struct char_data *ch, int spellnum)
{
    int limit = GET_PSIONIC_LEVEL(ch);
    limit -= psionic_powers[spellnum].psp_cost;
    return MAX(limit, 0);
}

int adjust_augment_psp_for_spell(struct char_data *ch, int spellnum)
{
    if (GET_AUGMENT_PSP(ch) > 0)
    {
      if (GET_AUGMENT_PSP(ch) > max_augment_psp_allowed(ch, spellnum))
        GET_AUGMENT_PSP(ch) = max_augment_psp_allowed(ch, spellnum);
      if (GET_PSP(ch) > GET_AUGMENT_PSP(ch))
        GET_AUGMENT_PSP(ch) = GET_PSP(ch);
    }
    return GET_AUGMENT_PSP(ch);
}