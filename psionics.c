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
#include "act.h"

struct psionic_power_data psionic_powers[MAX_SPELLS + 1];

void spello(int spl, const char *name, int max_psp, int min_psp,
            int psp_change, int minpos, int targets, int violent, int routines,
            const char *wearoff, int time, int memtime, int school, bool quest);

void unused_psionic_power(int pwr)
{
    psionic_powers[pwr].psp_cost = 0;
    psionic_powers[pwr].can_augment = false;
    psionic_powers[pwr].augment_amount = 0;
    psionic_powers[pwr].max_augment = 0;
    psionic_powers[pwr].power_type = PSIONIC_POWER_TYPE_NONE;
}

void psiono(int pwr, const char *name, int psp_cost, bool can_augment, int augment_amount, int max_augment, int power_type, int targets, int violent, int routines, const char *wearoff, int time)
{
    psionic_powers[pwr].psp_cost = psp_cost;
    psionic_powers[pwr].can_augment = can_augment;
    psionic_powers[pwr].augment_amount = augment_amount;
    psionic_powers[pwr].max_augment = max_augment;
    psionic_powers[pwr].power_type = power_type;
    spello(pwr, name, 0, 0, 0, POS_FIGHTING, targets, violent, routines, wearoff, time, 0, NOSCHOOL, false);
}

void assign_psionic_powers(void)
{
    int i = 0;

    for (i = PSIONIC_POWER_START; i <= PSIONIC_POWER_END; i++)
        unused_psionic_power(i);

    // level 1 psionic powers
    psiono(PSIONIC_BROKER, "broker", 1, true, 2, 100, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic persuasive edge expires.", 1);
    psiono(PSIONIC_CALL_TO_MIND, "call to mind", 1, true, 2, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic knowledge of lore fades away.", 1);
    psiono(PSIONIC_CATFALL, "catfall", 1, false, 0, 0, PSYCHOPORTATION, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your ability to catfall has expired.", 1);
    psiono(PSIONIC_CRYSTAL_SHARD, "crystal shard", 1, true, 1, 100, METACREATIVITY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 1);
    psiono(PSIONIC_DECELERATION, "deceleration", 1, true, 2, 100, PSYCHOPORTATION, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "Your psychic deceleration expires.", 1);
    psiono(PSIONIC_DEMORALIZE, "demoralize", 1, true, 2, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_MASSES, "You are no longer demoralized.", 1);
    // psiono(PSIONIC_ECTOPLASMIC_SHEEN, "ectoplasmic sheen", 1, true, METACREATIVITY, TAR_IGNORE, false, MAG_ROOM, "The ectoplasmic sheen evaporates completely.", 1);
    psiono(PSIONIC_ENERGY_RAY, "energy ray", 1, true, 1, 100, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 1);
    psiono(PSIONIC_FORCE_SCREEN, "force screen", 1, true, 4, 100, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your force screen blinks out of existence.", 1);
    psiono(PSIONIC_FORTIFY, "fortify", 1, true, 2, 100, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic fortification has subsided.", 1);
    psiono(PSIONIC_INERTIAL_ARMOR, "inertial armor", 1, true, 2, 100, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your inertial armor blinks out of existence.", 1);
    psiono(PSIONIC_INEVITABLE_STRIKE, "inevitable strike", 1, true, 1, 100, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "You expend your inevitable strike!", 1);
    psiono(PSIONIC_MIND_THRUST, "mind thrust", 1, true, 1, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 1);
    psiono(PSIONIC_DEFENSIVE_PRECOGNITION, "defensive precognition", 1, true, 3, 100, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your defensive precognition expires.", 1);
    psiono(PSIONIC_OFFENSIVE_PRECOGNITION, "offensive precognition", 1, true, 3, 100, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your offensive precognition expires.", 1);
    psiono(PSIONIC_OFFENSIVE_PRESCIENCE, "offensive prescience", 1, true, 3, 100, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your offensive prescience expires.", 1);
    psiono(PSIONIC_SLUMBER, "slumber", 1, true, 1, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "You are no longer compelled to slumber.", 1);
    psiono(PSIONIC_VIGOR, "vigor", 1, true, 3, 100, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic vigor subsides.", 1);

    // level 2 psionic powers
    psiono(PSIONIC_BESTOW_POWER, "bestow power", 3, true, 3, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, false, MAG_POINTS, NULL, 2);
    psiono(PSIONIC_BIOFEEDBACK, "biofeedback", 3, true, 3, 100, TELEPATHY, TAR_CHAR_ROOM, false, MAG_AFFECTS, "You feel your biofeedback protection expire.", 2);
    psiono(PSIONIC_BODY_EQUILIBRIUM, "body equilibrium", 3, true, 1, 100, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "You feel your body equillibrium fade.", 2);
    psiono(PSIONIC_BREACH, "breach", 3, false, 1, 100, PSYCHOPORTATION, TAR_IGNORE, false, MAG_MANUAL, NULL, 2);
    psiono(PSIONIC_CONCEALING_AMORPHA, "concealing amorpha", 3, true, 3, 100, METACREATIVITY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "The concealing amorpha surrounding you dissipates.", 2);
    psiono(PSIONIC_CONCUSSION_BLAST, "concussion blast", 3, true, 2, 100, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AREAS, NULL, 2);
    psiono(PSIONIC_DETECT_HOSTILE_INTENT, "detect hostile intent", 3, false, 0, 0, TELEPATHY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic ability to detect nearby danger has passed.", 2);
    psiono(PSIONIC_ELFSIGHT, "elfsight", 3, false, 0, 0, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your elfsight wears off.", 2);
    psiono(PSIONIC_ENERGY_ADAPTATION_SPECIFIED, "specified energy adaptation", 3, false, 0, 0, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your specified energy adaptation has expired.", 2);
    psiono(PSIONIC_ENERGY_PUSH, "energy push", 3, true, 2, 100, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 2);
    psiono(PSIONIC_ENERGY_STUN, "energy stun", 3, true, 1, 100, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_MASSES, NULL, 2);
    psiono(PSIONIC_INFLICT_PAIN, "inflict pain", 3, true, 2, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "The psychic pain tormenting you finally ends.", 2);
    psiono(PSIONIC_MENTAL_DISRUPTION, "mental disruption", 3, true, 2, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_MASSES, "THe psychic mental disruption affecting you expires.", 2);
    // psiono(PSIONIC_PSIONIC_LOCK, "psionic lock", 3, false, PSYCHOPORTATION, TAR_IGNORE, false, MAG_MANUAL, NULL, 2);
    psiono(PSIONIC_PSYCHIC_BODYGUARD, "psychic bodyguard", 3, true, 2, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, false, MAG_AFFECTS, "Your psychic bodyguard affect expires.", 2);
    psiono(PSIONIC_RECALL_AGONY, "recall agony", 3, true, 1, 100, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 2);
    // psiono(PSIONIC_SHARE_PAIN, "share pain", 3, false, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_NOT_SELF, false, MAG_AFFECTS, "You are no longer sharing pain weith you ally.", 2);
    psiono(PSIONIC_SWARM_OF_CRYSTALS, "swarm of crystals", 3, true, 1, 100, METACREATIVITY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AREAS, NULL, 2);
    psiono(PSIONIC_THOUGHT_SHIELD, "thought shield", 3, true, 1, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your thought shield expires.", 2);

    // level 3 psionic powers
    psiono(PSIONIC_BODY_ADJUSTMENT, "body adjustment", 5, true, 1, 100, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_POINTS, NULL, 3);
    psiono(PSIONIC_CONCUSSIVE_ONSLAUGHT, "concussive onslaught", 5, true, 2, 100, PSYCHOKINESIS, TAR_IGNORE, true, MAG_MANUAL, NULL, 3);
    // psiono(PSIONIC_DISPEL_PSIONICS, "dispel psionic", 5, true, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, false, MAG_MANUAL, NULL, 3);
    psiono(PSIONIC_ENDORPHIN_SURGE, "endorphin surge", 5, true, 6, 6, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your endorphin surge wears off.", 3);
    psiono(PSIONIC_ENERGY_BURST, "energy burst", 5, true, 1, 100, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AREAS, NULL, 3);
    psiono(PSIONIC_ENERGY_RETORT, "energy retort", 5, true, 1, 100, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your energy retort protection fades.", 3);
    psiono(PSIONIC_ERADICATE_INVISIBILITY, "eradicate invisibility", 5, true, 2, 100, PSYCHOKINESIS, TAR_CHAR_ROOM | TAR_NOT_SELF, false, MAG_MASSES, NULL, 3);
    psiono(PSIONIC_HEIGHTENED_VISION, "heightened vision", 5, false, 0, 0, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your heightened vision returns to normal.", 3);
    psiono(PSIONIC_MENTAL_BARRIER, "mental barrier", 5, true, 1, 100, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your mental barrier blinks out of existence.", 3);
    psiono(PSIONIC_MIND_TRAP, "mind trap", 5, true, 1, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your mind trap triggers!", 3);
    psiono(PSIONIC_PSIONIC_BLAST, "psionic blast", 5, true, 2, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_MASSES, "The stunning affect of being hit by a psionic blast expires.", 3);
    // psiono(PSIONIC_FORCED_SHARED_PAIN, "forced shared pain", 5, true, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "You're no longer being forced to share pain.", 3);
    psiono(PSIONIC_SHARPENED_EDGE, "sharpened edge", 5, false, 0, 0, METACREATIVITY, TAR_CHAR_ROOM, false, MAG_AFFECTS, "The sharpened edge of your weapons fades.", 3);
    psiono(PSIONIC_UBIQUITUS_VISION, "ubiquitous vision", 5, false, 0, 0, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your ubiquitus vision fades.", 3);

    // level 4 psionic powers
    psiono(PSIONIC_DEADLY_FEAR, "deadly fear", 7, true, 2, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 4);
    psiono(PSIONIC_DEATH_URGE, "death urge", 7, true, 2, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "You are longer compelled to harm yourself.", 4);
    psiono(PSIONIC_EMPATHIC_FEEDBACK, "empathetic feedback", 7, 3, 100, true, TELEPATHY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your empathetic feedback protection ends.", 4);
    psiono(PSIONIC_ENERGY_ADAPTATION, "energy adaptation", 7, false, 0, 0, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your specified energy adaptation has expired.", 4);
    psiono(PSIONIC_INCITE_PASSION, "incite passion", 7, false, 0, 0, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "Your irrationally heightened animal instics return to normal.", 4);
    psiono(PSIONIC_INTELLECT_FORTRESS, "intellect fortress", 7, false, 0, 0, PSYCHOKINESIS, TAR_CHAR_ROOM, false, MAG_GROUPS, "The intellect fortress guarding your mind expires.", 4);
    psiono(PSIONIC_MOMENT_OF_TERROR, "moment of terror", 7, true, 4, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "Your terrors finally fade away.", 4);
    psiono(PSIONIC_POWER_LEECH, "power leech", 7, false, 0, 0, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "You finally expel the power leech from your psyche.", 4);
    psiono(PSIONIC_SLIP_THE_BONDS, "slip the bonds", 7, false, 0, 0, PSYCHOPORTATION, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic ability to slip bonds has expired.", 4);
    psiono(PSIONIC_WITHER, "wither", 7, true, 4, 4, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "Your withered limbs and body finally return to normal.", 4);
    psiono(PSIONIC_WALL_OF_ECTOPLASM, "wall of ectoplasm", 7, false, 0, 0, METACREATIVITY, TAR_IGNORE, false, MAG_MANUAL, "The wall of ectoplasm hardens and falls to dust.", 4);

    // level 5 psionic powers
    // psiono(PSIONIC_ADAPT_BODY, "adapt body", 9, false, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your psychic adaptability has ended.", 5);
    psiono(PSIONIC_ECTOPLASMIC_SHAMBLER, "ectoplasmic shambler", 9, false, 0, 0, METACREATIVITY, TAR_IGNORE, false, MAG_SUMMONS, NULL, 5);
    psiono(PSIONIC_PIERCE_VEIL, "pierce the veils", 9, false, 0, 0, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your ability to pierce hidden veils has expired.", 5);
    psiono(PSIONIC_PLANAR_TRAVEL, "planar travel", 9, false, 0, 0, PSYCHOPORTATION, TAR_IGNORE, false, MAG_CREATIONS, NULL, 5);
    psiono(PSIONIC_POWER_RESISTANCE, "power resistance", 9, false, 0, 0, CLAIRSENTIENCE, TAR_CHAR_ROOM, false, MAG_AFFECTS, "Your power resistance has faded.", 5);
    psiono(PSIONIC_PSYCHIC_CRUSH, "psychic crush", 9, true, 2, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 5);
    psiono(PSIONIC_PSYCHOPORTATION, "psychoport", 9, false, 0, 0, PSYCHOPORTATION, TAR_CHAR_WORLD | TAR_NOT_SELF, false, MAG_MANUAL, NULL, 5);
    psiono(PSIONIC_SHATTER_MIND_BLANK, "shatter mind blank", 9, false, 0, 0, TELEPATHY, TAR_IGNORE, true, MAG_MASSES, NULL, 5);
    psiono(PSIONIC_SHRAPNEL_BURST, "shrapnel burst", 9, true, 2, 100, METACREATIVITY, TAR_IGNORE, true, MAG_AREAS, NULL, 5);
    psiono(PSIONIC_TOWER_OF_IRON_WILL, "tower of iron will", 9, true, 2, 100, TELEPATHY, TAR_IGNORE, false, MAG_GROUPS, "The tower of iron will protecting your mind fades.", 5);
    psiono(PSIONIC_UPHEAVAL, "upheaval", 9, true, 2, 100, PSYCHOPORTATION, TAR_IGNORE, true, MAG_AREAS | MAG_ROOM, "The rubble blocking movement in this area turns to dust.", 5);

    // level 6 psionic powers
    psiono(PSIONIC_BREATH_OF_THE_BLACK_DRAGON, "breath of the black dragon", 11, true, 2, 100, PSYCHOMETABOLISM, TAR_IGNORE, true, MAG_AREAS, NULL, 6);
    psiono(PSIONIC_BRUTALIZE_WOUNDS, "brutalize wounds", 11, false, 0, 0, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "The psychic brutalizing of your wounds suddenly ends.", 6);
    psiono(PSIONIC_DISINTEGRATION, "disintegration", 11, true, 2, 100, PSYCHOPORTATION, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE, NULL, 6);
    // psiono(PSIONIC_REMOTE_VIEW_TRAP, "remote view trap", 11, false, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your remote view trap expires.", 6);
    psiono(PSIONIC_SUSTAINED_FLIGHT, "sustained flight", 11, false, 0, 0, PSYCHOPORTATION, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your sustained flight ends.", 6);

    // level 7 psionic powers
    // psiono(PSIONIC_BARRED_MIND_PERSONAL, "personal barred mind", 13, true, TELEPATHY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your barred mind effect passes.", 7);
    psiono(PSIONIC_COSMIC_AWARENESS, "cosmic awareness", 13, false, 0, 0, CLAIRSENTIENCE, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your cosmic awareness expires.", 7);
    psiono(PSIONIC_ENERGY_CONVERSION, "energy conversion", 13, false, 0, 0, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your energy conversion ability has expired.", 7);
    // psiono(PSIONIC_ENERGY_WAVE, "energy wave", 13, true, PSYCHOKINESIS, TAR_IGNORE, true, MAG_AREAS, NULL, 7);
    psiono(PSIONIC_EVADE_BURST, "evade burst", 13, true, 4, 4, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your evade burst ability has ended.", 7);
    psiono(PSIONIC_OAK_BODY, "oak body", 13, true, 1, 100, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your oak body returns to normal.", 7);
    psiono(PSIONIC_PSYCHOSIS, "psychosis", 13, true, 2, 100, TELEPATHY, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_AFFECTS, "You no longer feel any signs of psychosis.", 7);
    psiono(PSIONIC_ULTRABLAST, "ultrablast", 13, true, 1, 100, TELEPATHY, TAR_IGNORE, true, MAG_AREAS, NULL, 7);

    // level 8 psionic powers
    // psiono(PSIONIC_BARRED_MIND, "barred mind", 15, true, TELEPATHY, TAR_CHAR_ROOM, false, MAG_AFFECTS, "Your barred mind effect passes.", 8);
    psiono(PSIONIC_BODY_OF_IRON, "body of iron", 15, false, 0, 0, METACREATIVITY, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your iron body returns to normal.", 8);
    psiono(PSIONIC_PSYCHOPORT_GREATER, "greater psychoport", 15, false, 0, 0, PSYCHOPORTATION, TAR_CHAR_WORLD, false, MAG_MANUAL, NULL, 8);
    psiono(PSIONIC_RECALL_DEATH, "recall death", 15, false, 0, 0, CLAIRSENTIENCE, TAR_CHAR_ROOM, true, MAG_DAMAGE, NULL, 8);
    psiono(PSIONIC_SHADOW_BODY, "shadow body", 15, false, 0, 0, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your shadowed-body returns to normal.", 8);
    psiono(PSIONIC_TRUE_METABOLISM, "true metabolism", 15, false, 0, 0, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_SELF_ONLY, false, MAG_AFFECTS, "Your enhanced metabolism returns to normal.", 8);

    // level 9 psionic powers
    // psiono(PSIONIC_APOPSI, "apopsi", 17, false, TELEPATHY, TAR_CHAR_ROOM|TAR_NOT_SELF, true, MAG_AFFECTS, "Your addled mind returns to normal.", 9);
    psiono(PSIONIC_ASSIMILATE, "assimilate", 17, false, 0, 0, PSYCHOMETABOLISM, TAR_CHAR_ROOM | TAR_NOT_SELF, true, MAG_DAMAGE | MAG_AFFECTS, NULL, 9);
    // psiono(PSIONIC_TIMELESS_BODY, "timeless body", 17, false, PSYCHOPORTATION, TAR_CHAR_ROOM|TAR_SELF_ONLY, false, MAG_AFFECTS, "Your timeless body effect expires.", 9);
}

#define MANIFEST_NO_ARG "You must specify the number of power points to augment your power with, or 0.\r\n"                                            \
                        "You must follow that with the power name in single quotes, optionally followed by the target where applicable.\r\n"           \
                        "Example: manifest 2 'mind thrust' ogre\r\n"                                                                                   \
                        "This will manifest the mind thrust against the first target named ogre and with the power augmented with 2 power points.\r\n" \
                        "You may also type manifest energytype (fire|cold|lightning|acid|sonic) to set your energy type for energy based powers.\r\n"

ACMD(do_manifest)
{

    char augment[200], pass_arg[200], final_pass[201];

    half_chop((char *)argument, augment, pass_arg);

    // Safety check.  Should be handled by resetCastingData in spell_parser.c
    GET_AUGMENT_PSP(ch) = 0;

    if (!*augment || !*pass_arg)
    {
        send_to_char(ch, MANIFEST_NO_ARG);
        return;
    }
    if (!isdigit(*augment))
    {
        if (is_abbrev(augment, "energytype"))
        {
            if (is_abbrev(pass_arg, "fire"))
            {
                GET_PSIONIC_ENERGY_TYPE(ch) = DAM_FIRE;
                send_to_char(ch, "You've set your psionic energy type to fire.\r\n");
            }
            else if (is_abbrev(pass_arg, "cold"))
            {
                GET_PSIONIC_ENERGY_TYPE(ch) = DAM_COLD;
                send_to_char(ch, "You've set your psionic energy type to cold.\r\n");
            }
            else if (is_abbrev(pass_arg, "lightning"))
            {
                GET_PSIONIC_ENERGY_TYPE(ch) = DAM_ELECTRIC;
                send_to_char(ch, "You've set your psionic energy type to lightning.\r\n");
            }
            else if (is_abbrev(pass_arg, "acid"))
            {
                GET_PSIONIC_ENERGY_TYPE(ch) = DAM_ACID;
                send_to_char(ch, "You've set your psionic energy type to acid.\r\n");
            }
            else if (is_abbrev(pass_arg, "sonic") || is_abbrev(pass_arg, "sound"))
            {
                GET_PSIONIC_ENERGY_TYPE(ch) = DAM_SOUND;
                send_to_char(ch, "You've set your psionic energy type to sonic.\r\n");
            }
            else
            {
                send_to_char(ch, MANIFEST_NO_ARG);
            }
            return;
        }
        else
        {
            send_to_char(ch, MANIFEST_NO_ARG);
            return;
        }
    }

    int extra_pp = atoi(augment);
    GET_AUGMENT_PSP(ch) = extra_pp;
    snprintf(final_pass, sizeof(final_pass), " %s", pass_arg);
    do_gen_cast(ch, (const char *)final_pass, cmd, SCMD_CAST_PSIONIC);
}

#undef MANIFEST_NO_ARG

int max_augment_psp_allowed(struct char_data *ch, int spellnum)
{
    int limit = GET_PSIONIC_LEVEL(ch);
    limit -= psionic_powers[spellnum].psp_cost;
    if (affected_by_spell(ch, PSIONIC_ABILITY_PSIONIC_FOCUS))
    {
        if (HAS_REAL_FEAT(ch, FEAT_PROFICIENT_AUGMENTING))
            limit++;
        if (HAS_REAL_FEAT(ch, FEAT_EXPERT_AUGMENTING))
            limit++;
        if (HAS_REAL_FEAT(ch, FEAT_MASTER_AUGMENTING))
            limit++;
    }
    limit = MIN(limit, psionic_powers[spellnum].max_augment);
    limit = MAX(limit, 0);
    return limit;
}

int adjust_augment_psp_for_spell(struct char_data *ch, int spellnum)
{
    if (GET_AUGMENT_PSP(ch) > 0)
    {
        if (GET_AUGMENT_PSP(ch) > max_augment_psp_allowed(ch, spellnum))
        {
            GET_AUGMENT_PSP(ch) = max_augment_psp_allowed(ch, spellnum);
        }
        if (GET_PSP(ch) < GET_AUGMENT_PSP(ch))
        {
            GET_AUGMENT_PSP(ch) = GET_PSP(ch);
        }
    }

    return MIN(GET_AUGMENT_PSP(ch), psionic_powers[spellnum].max_augment);
}

int get_augment_casting_time_adjustment(struct char_data *ch)
{
    // we'll allow for two free augment psp before we start increasing cast time
    int adjustment = (GET_AUGMENT_PSP(ch) - 2) / 2;
    if (HAS_REAL_FEAT(ch, FEAT_PROFICIENT_AUGMENTING))
        adjustment--;
    if (HAS_REAL_FEAT(ch, FEAT_EXPERT_AUGMENTING))
        adjustment--;
    if (HAS_REAL_FEAT(ch, FEAT_MASTER_AUGMENTING))
        adjustment--;
    return MAX(0, adjustment);
}

ACMDU(do_cosmic_awareness)
{
    if (!affected_by_spell(ch, PSIONIC_COSMIC_AWARENESS))
    {
        send_to_char(ch, "You are not under the effect of the cosmic awareness psionic power.\r\n");
        return;
    }

    skip_spaces(&argument);

    if (!*argument)
    {
        send_to_char(ch, "To use your cosmic awareness ability, type: cosmicawareness use\r\n");
        return;
    }

    if (!is_abbrev(argument, "use"))
    {
        send_to_char(ch, "To use your cosmic awareness ability, type: cosmicawareness use\r\n");
        return;
    }

    if (ch->player_specials->cosmic_awareness)
    {
        send_to_char(ch, "You have already used your cosmic awareness.  It will take effect the next time you have to roll a d20, such as attack rolls, skill checks, saving throws, etc.\r\n");
        return;
    }

    ch->player_specials->cosmic_awareness = true;
    send_to_char(ch, "You use your cosmic awareness ability.  The next d20 roll you make will add a +%d to the roll.\r\n"
                     "Some example d20 rolls are: attack rolls, skill checks, saving throws, etc.\r\n",
                 GET_PSIONIC_LEVEL(ch));
    affect_from_char(ch, PSIONIC_COSMIC_AWARENESS);
}

ACMDU(do_discharge)
{
    int dam_type;
    struct char_data *victim = NULL;
    char buf[MEDIUM_STRING];
    skip_spaces(&argument);

    if (!affected_by_spell(ch, PSIONIC_ENERGY_CONVERSION))
    {
        send_to_char(ch, "You are not under the effect of the energy conversion power.\r\n");
        return;
    }
    if (!*argument)
    {
        send_to_char(ch, "You can discharge any of the following by typing 'discharge (energy type)'\r\n"
                         "fire      [%-2d] damage absorbed\r\n"
                         "cold      [%-2d] damage absorbed\r\n"
                         "acid      [%-2d] damage absorbed\r\n"
                         "sonic     [%-2d] damage absorbed\r\n"
                         "electric  [%-2d] damage absorbed\r\n",
                     ch->player_specials->energy_conversion[DAM_FIRE],
                     ch->player_specials->energy_conversion[DAM_COLD],
                     ch->player_specials->energy_conversion[DAM_ACID],
                     ch->player_specials->energy_conversion[DAM_SOUND],
                     ch->player_specials->energy_conversion[DAM_ELECTRIC]);
        return;
    }

    if (!FIGHTING(ch))
    {
        send_to_char(ch, "This ability can only be used in combat.\r\n");
        return;
    }

    dam_type = search_block(argument, damtypes, FALSE);
    victim = FIGHTING(ch);

    switch (dam_type)
    {
    case DAM_FIRE:
    case DAM_COLD:
    case DAM_ACID:
    case DAM_SOUND:
    case DAM_ELECTRIC:
        if (ch->player_specials->energy_conversion[dam_type] <= 0)
        {
            send_to_char(ch, "You do not have any %s energy absorbed right now.\r\n", damtypes[dam_type]);
            return;
        }
        if (power_resistance(ch, victim, 0))
        {
            return;
        }
        if (!attack_roll(ch, victim, ATTACK_TYPE_PRIMARY, TRUE, 0))
        {
            snprintf(buf, sizeof(buf), "A beam of %s energy fired by $n at $N goes wide.", damtypes[dam_type]);
            act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
            snprintf(buf, sizeof(buf), "Your beam of %s energy fired at $N goes wide.", damtypes[dam_type]);
            act(buf, FALSE, ch, 0, victim, TO_CHAR);
            snprintf(buf, sizeof(buf), "$n's beam of %s energy fired at YOU goes wide.", damtypes[dam_type]);
            act(buf, FALSE, ch, 0, victim, TO_VICT);
            return;
        }
        snprintf(buf, sizeof(buf), "$n fires a beam of %s at $N!", damtypes[dam_type]);
        act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
        snprintf(buf, sizeof(buf), "You fire a beam of %s at $N!", damtypes[dam_type]);
        act(buf, FALSE, ch, 0, victim, TO_CHAR);
        snprintf(buf, sizeof(buf), "$n fires a beam of %s at YOU!", damtypes[dam_type]);
        act(buf, FALSE, ch, 0, victim, TO_VICT);
        damage(ch, victim, ch->player_specials->energy_conversion[dam_type], PSIONIC_ENERGY_CONVERSION, dam_type, FALSE);
        ch->player_specials->energy_conversion[dam_type] = 0;
        return;
    }

    send_to_char(ch, "That is not a valid energy type.  Please choose from among: fire, cold, acid, sonic, electric\r\n");
}

ACMDCHECK(can_psionic_focus)
{
    ACMDCHECK_PREREQ_HASFEAT(FEAT_PSIONIC_FOCUS, "You have no idea how.\r\n");
    return CAN_CMD;
}

ACMD(do_psionic_focus)
{

    PREREQ_NOT_NPC();
    PREREQ_CHECK(can_psionic_focus);
    PREREQ_HAS_USES(FEAT_PSIONIC_FOCUS, "You have not yet recovered your psionic focus.\r\n");

    struct affected_type af;

    new_affect(&af);

    start_daily_use_cooldown(ch, FEAT_PSIONIC_FOCUS);

    af.duration = 12;

    affect_to_char(ch, &af);

    send_to_char(ch, "You enter a deep psionic focus.\r\n");
    act("$n seems to enter a deep focus.", FALSE, ch, 0, 0, TO_ROOM);
}

ACMDCHECK(can_double_manifest)
{
    ACMDCHECK_PREREQ_HASFEAT(FEAT_DOUBLE_MANIFEST, "You have no idea how.\r\n");
    return CAN_CMD;
}

ACMD(do_double_manifest)
{

    PREREQ_NOT_NPC();
    PREREQ_CHECK(can_double_manifest);
    PREREQ_HAS_USES(FEAT_DOUBLE_MANIFEST, "You are not yet able to perform a double manifest.\r\n");

    struct affected_type af;

    new_affect(&af);

    start_daily_use_cooldown(ch, FEAT_DOUBLE_MANIFEST);

    af.duration = 120;

    affect_to_char(ch, &af);

    send_to_char(ch, "You prepare to perform a double manifest.\r\n");
}