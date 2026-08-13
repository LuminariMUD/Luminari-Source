/**
 * @file spec/spec_rol_combat.c
 * Identity-profiled combat adapters for converted Realms of Luminari mobiles.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "combat/fight.h"
#include "act.h"
#include "comm.h"
#include "db.h"
#include "dgscript/dg_scripts.h"
#include "graph.h"
#include "handler.h"
#include "interpreter.h"
#include "magic/domains_schools.h"
#include "magic/spells.h"
#include "movement/movement.h"
#include "mud_event.h"
#include "spec/spec_combat.h"
#include "spec/spec_context.h"
#include "spec/spec_dispatch.h"
#include "spec/spec_rol_avernus.h"
#include "spec/spec_rol_conversion.h"
#include "spec/spec_rol_darkhold.h"

#define ROL_BALOR_WHIP_VNUM 2093227
#define ROL_BALOR_SWORD_VNUM 2093228
#define ROL_DROW_CONCLAVE_BASE_VNUM 2093146
#define ROL_DROW_CONCLAVE_BARRACKS_VNUM 2093153
#define ROL_DROW_CONCLAVE_ROOM_COUNT 11
#define ROL_DROW_CONCLAVE_REDEPLOY (1U << 0)
#define ROL_PLANAR_HIT_BURST_COOLDOWN (PULSE_VIOLENCE * 3)

static bool rol_drow_conclave_alarm_sounding = false;

enum rol_monster_combat_effect
{
  ROL_MONSTER_PLANT_POISON = 0,
  ROL_MONSTER_LYCAN_TIGER,
  ROL_MONSTER_LYCAN_FOX,
  ROL_MONSTER_SPIDER_VENOM,
  ROL_MONSTER_ASHENTORIS,
  ROL_MONSTER_BANSHEE_WAIL,
  ROL_MONSTER_FOUR_ARMS,
  ROL_MONSTER_TENTACLE_SLAM,
  ROL_MONSTER_ROT_BRINGER,
  ROL_MONSTER_WINGED_DEVA,
  ROL_MONSTER_SMALL_PRISMATIC,
  ROL_MONSTER_CRITICAL_PRISMATIC,
  ROL_MONSTER_UBER_PRISMATIC,
  ROL_MONSTER_FIRE_BOSS,
  ROL_MONSTER_EARTH_BOSS,
  ROL_MONSTER_AIR_BOSS,
  ROL_MONSTER_WATER_BOSS,
  ROL_MONSTER_PIT_FIEND_BITE_TAIL,
  ROL_MONSTER_CHICKEN,
  ROL_MONSTER_KOBOLD_PRIEST,
  ROL_MONSTER_PIERCER,
  ROL_MONSTER_PURPLE_WORM,
  ROL_MONSTER_PHALANX,
  ROL_MONSTER_SKELETON,
  ROL_MONSTER_XEXOS,
  ROL_MONSTER_AGTHRODOS,
  ROL_MONSTER_TREE_SPIRIT,
  ROL_MONSTER_DRANUM,
  ROL_MONSTER_SWALLOW_WHOLE,
  ROL_MONSTER_SWALLOW_SPIT,
  ROL_MONSTER_MOVANIC_DEVA,
  ROL_MONSTER_CANTHUS,
  ROL_MONSTER_JOTUN_THRYM,
  ROL_MONSTER_JOTUN_LOKI,
  ROL_MONSTER_SUMMON_JESSICA_WISP,
  ROL_MONSTER_SUMMON_ROBYN_WISP,
  ROL_MONSTER_SUMMON_ROBYN_SERVANT,
  ROL_MONSTER_JURTREM,
  ROL_MONSTER_KAMERYNN,
  ROL_MONSTER_CRIMSON_FURY,
  ROL_MONSTER_BARBARIAN_SPIRITIST,
  ROL_MONSTER_TAKO_DEMON,
  ROL_MONSTER_WEREWOLF,
  ROL_MONSTER_JOTUN_MIMER,
  ROL_MONSTER_SEELIE_FAERIE,
  ROL_MONSTER_MANSCORPION_VENOM_LIGHT,
  ROL_MONSTER_MANSCORPION_VENOM_MEDIUM,
  ROL_MONSTER_MANSCORPION_VENOM_HEAVY,
  ROL_MONSTER_MANSCORPION_VENOM_KING,
  ROL_MONSTER_DOBLUTH_BANSHEE_WAIL,
  ROL_MONSTER_DOBLUTH_BLADESTORM,
  ROL_MONSTER_HIVE_SANDSTORM_BEAST,
  ROL_MONSTER_HIVE_SKRIAXIT_SANDSTORM,
  ROL_MONSTER_GREYCLOAK_BANSHEE_WAIL,
  ROL_MONSTER_GREYCLOAK_FUMES,
  ROL_MONSTER_GREYCLOAK_ARALESH,
  ROL_MONSTER_PLANAR_MANES_DEATH,
  ROL_MONSTER_PLANAR_BALOR,
  ROL_MONSTER_PLANAR_GLABREZU_GRAB,
  ROL_MONSTER_PLANAR_MARILITH_TAIL,
  ROL_MONSTER_PLANAR_SUCCUBUS_CHARM,
  ROL_MONSTER_PLANAR_VROCK_BURSTS,
  ROL_MONSTER_PLANAR_SPINAGON_SPIKES,
  ROL_MONSTER_AVERNUS_BARBAZU_BERSERK,
  ROL_MONSTER_AVERNUS_GELUGON_MERITOS,
  ROL_MONSTER_AVERNUS_GELUGON_HANARIEL,
  ROL_MONSTER_AVERNUS_BEL,
  ROL_MONSTER_AVERNUS_AUXILIARY,
  ROL_MONSTER_DARKHOLD_SHADOW_FIEND,
  ROL_MONSTER_DARKHOLD_SHADOW_DRAGON,
  ROL_MONSTER_GRIFFON_NONBARBARIAN,
  ROL_MONSTER_DUSK_PARALYSIS_GAZE,
  ROL_MONSTER_UM2_MANSCORPION_TAIL,
  ROL_MONSTER_UM2_WYVERN_TAIL,
  ROL_MONSTER_UM2_DROW_CONCLAVE_GUARD,
  ROL_MONSTER_RESIDUAL_MOBILE
};

struct rol_monster_combat_profile
{
  int mobile_vnum;
  enum rol_monster_combat_effect effect;
  int proc_denominator;
  const char *description;
};

enum rol_seelie_faerie_ability
{
  ROL_SEELIE_FAERIE_FIRE = (1U << 0),
  ROL_SEELIE_PRISMATIC = (1U << 1),
  ROL_SEELIE_SEARCH = (1U << 2)
};

struct rol_seelie_faerie_profile
{
  int mobile_vnum;
  unsigned int abilities;
};

static void rol_monster_stop_combat(struct char_data *victim);

static const char rol_griffon_guard_description[] =
    "Attacks visible non-Berserker mortals and remembers them while occupied.";

/* Keep this table sorted by converted mobile VNUM for binary lookup. */
static const struct rol_monster_combat_profile rol_monster_combat_profiles[] = {
    {150772, ROL_MONSTER_PLANT_POISON, 3, "Barbed-thorn poison volley."},
    {196007, ROL_MONSTER_SWALLOW_WHOLE, 10, "Rhemorhaz bite and whole-swallow attack."},
    {196013, ROL_MONSTER_JOTUN_MIMER, 1, "Mimer's gate challenge and return to post."},
    {196027, ROL_MONSTER_JOTUN_THRYM, 2, "Thrym's freezing paralysis bolt."},
    {196040, ROL_MONSTER_JOTUN_LOKI, 3, "Utgard-Loki's room-wide fear visions."},
    {196076, ROL_MONSTER_SWALLOW_WHOLE, 10, "Rhemorhaz bite and whole-swallow attack."},
    {2000207, ROL_MONSTER_PLANAR_BALOR, 1,
     "Balor equipment upkeep, elemental protection, and corpse suppression."},
    {2000212, ROL_MONSTER_PLANAR_GLABREZU_GRAB, 11,
     "Pincer capture, captive command restraint, and delayed combat."},
    {2000214, ROL_MONSTER_PLANAR_MANES_DEATH, 1, "Manes death burst of acidic vapor."},
    {2000215, ROL_MONSTER_PLANAR_MARILITH_TAIL, 11,
     "Tail capture, captive command restraint, and delayed combat."},
    {2000220, ROL_MONSTER_PLANAR_SUCCUBUS_CHARM, 4,
     "Male-target charm, Blackguard service, command restraint, and delayed kiss."},
    {2000221, ROL_MONSTER_PLANAR_VROCK_BURSTS, 1,
     "Screech, spore burst, and five-Vrock dance of ruin."},
    {2000233, ROL_MONSTER_PLANAR_SPINAGON_SPIKES, 1,
     "Five-in-six flaming-spike volley with a three-round cooldown."},
    {2000325, ROL_MONSTER_LYCAN_TIGER, 11, "Were-tiger tearing attack."},
    {2000326, ROL_MONSTER_LYCAN_FOX, 6, "Were-fox slashing attack."},
    {2000327, ROL_MONSTER_LYCAN_TIGER, 11, "Were-tiger tearing attack."},
    {2000328, ROL_MONSTER_LYCAN_TIGER, 11, "Were-tiger tearing attack."},
    {2000525, ROL_MONSTER_WEREWOLF, 1, "Werewolf idle aggression and social activity."},
    {2001228, ROL_MONSTER_RESIDUAL_MOBILE, 1, "Beavis ambient activity."},
    {2001229, ROL_MONSTER_RESIDUAL_MOBILE, 1, "Butthead ambient activity."},
    {2001407, ROL_MONSTER_CHICKEN, 25, "Source-exact chicken nest activity."},
    {2001436, ROL_MONSTER_TAKO_DEMON, 1, "Tako's pit ambush and escape interception."},
    {2001437, ROL_MONSTER_KOBOLD_PRIEST, 5, "Kobold-priest force wall and imp summoning."},
    {2004070, ROL_MONSTER_PIERCER, 1, "One-shot hidden piercer ambush."},
    {2004480, ROL_MONSTER_PURPLE_WORM, 5, "Purple-worm whole-swallow attack."},
    {2004530, ROL_MONSTER_PIERCER, 1, "One-shot hidden piercer ambush."},
    {2005023, ROL_MONSTER_SPIDER_VENOM, 15, "Random-player venom bite."},
    {2005718, ROL_MONSTER_RESIDUAL_MOBILE, 1, "Ancient brownie ankle attack."},
    {2010661, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010744, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010745, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010749, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010750, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010754, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010755, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010756, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010757, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010758, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010759, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010760, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010761, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010762, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2010763, ROL_MONSTER_GRIFFON_NONBARBARIAN, 1, rol_griffon_guard_description},
    {2012005, ROL_MONSTER_PHALANX, 1, "Phalanx retreat, reconfiguration, and exit guard."},
    {2012006, ROL_MONSTER_SKELETON, 20, "Splitting skeleton and rare passage trip."},
    {2012024, ROL_MONSTER_SKELETON, 20, "Splitting skeleton and rare passage trip."},
    {2012025, ROL_MONSTER_XEXOS, 1, "Xexos combat transformation."},
    {2012026, ROL_MONSTER_AGTHRODOS, 1, "Agthrodos idle reversion."},
    {2014015, ROL_MONSTER_RESIDUAL_MOBILE, 1, "Finn ambient and combat speech."},
    {2014026, ROL_MONSTER_TREE_SPIRIT, 1, "Root entanglement and child-root summons."},
    {2014029, ROL_MONSTER_RESIDUAL_MOBILE, 1, "Faerie mischief activity."},
    {2014601, ROL_MONSTER_PLANT_POISON, 3, "Barbed-thorn poison volley."},
    {2014605, ROL_MONSTER_BARBARIAN_SPIRITIST, 1, "Spirit curse, disarm, and cyclone."},
    {2015113, ROL_MONSTER_DRANUM, 9, "Dranum life-force drain."},
    {2015125, ROL_MONSTER_JURTREM, 20, "Jurtrem's sanctuary-dispelling gaze."},
    {2019701, ROL_MONSTER_CRIMSON_FURY, 11, "Crimson Fury minion purge and fire blast."},
    {2019750, ROL_MONSTER_CRIMSON_FURY, 11, "Crimson Fury minion purge and fire blast."},
    {2020247, ROL_MONSTER_RESIDUAL_MOBILE, 1, "Spell-casting interception and counterstrike."},
    {2020378, ROL_MONSTER_ASHENTORIS, 11, "Life drain and lava storm."},
    {2021786, ROL_MONSTER_DOBLUTH_BLADESTORM, 5, "Room-wide animated bladestorm."},
    {2021820, ROL_MONSTER_DOBLUTH_BANSHEE_WAIL, 4, "Room-wide painful wail and paralysis."},
    {2026208, ROL_MONSTER_RESIDUAL_MOBILE, 1, "Spell-casting interception and counterstrike."},
    {2026216, ROL_MONSTER_RESIDUAL_MOBILE, 1, "Spell-casting interception and counterstrike."},
    {2026225, ROL_MONSTER_SUMMON_ROBYN_SERVANT, 4, "Robyn's bounded servant summon."},
    {2026236, ROL_MONSTER_RESIDUAL_MOBILE, 1, "Spell-casting interception and counterstrike."},
    {2026238, ROL_MONSTER_SUMMON_JESSICA_WISP, 4, "Jessica's bounded wisp summon."},
    {2026241, ROL_MONSTER_SUMMON_ROBYN_WISP, 4, "Robyn's bounded wisp summon."},
    {2026242, ROL_MONSTER_SUMMON_ROBYN_WISP, 4, "Robyn's bounded wisp summon."},
    {2026243, ROL_MONSTER_SUMMON_ROBYN_WISP, 4, "Robyn's bounded wisp summon."},
    {2026244, ROL_MONSTER_RESIDUAL_MOBILE, 1, "Spell-casting interception and counterstrike."},
    {2026245, ROL_MONSTER_RESIDUAL_MOBILE, 1, "Spell-casting interception and counterstrike."},
    {2032623, ROL_MONSTER_AVERNUS_AUXILIARY, 1, "Kri'ik's mortal-shape death transition."},
    {2032629, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2032632, ROL_MONSTER_PLANAR_SPINAGON_SPIKES, 1,
     "Five-in-six flaming-spike volley with a three-round cooldown."},
    {2032640, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2032641, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2032642, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2032643, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2032644, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2032645, ROL_MONSTER_PLANAR_SPINAGON_SPIKES, 1,
     "Five-in-six flaming-spike volley with a three-round cooldown."},
    {2032646, ROL_MONSTER_PLANAR_SPINAGON_SPIKES, 1,
     "Five-in-six flaming-spike volley with a three-round cooldown."},
    {2032654, ROL_MONSTER_AVERNUS_AUXILIARY, 1, "Periodic rogue concealment."},
    {2032659, ROL_MONSTER_AVERNUS_AUXILIARY, 1, "Periodic rogue concealment."},
    {2032660, ROL_MONSTER_AVERNUS_AUXILIARY, 1, "Prisoner escort reward and return."},
    {2033000, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2033001, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2033003, ROL_MONSTER_AVERNUS_AUXILIARY, 1, "Erinyes death-illusion room transition."},
    {2033004, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2033005, ROL_MONSTER_AVERNUS_AUXILIARY, 1, "Alignment-sensitive captive-deva echoes."},
    {2033008, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2033009, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2033011, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2033014, ROL_MONSTER_AVERNUS_BEL, 16,
     "Bel's pit-fiend attacks, guard sacrifice, pet purge, wards, and shield counter."},
    {2033015, ROL_MONSTER_AVERNUS_GELUGON_MERITOS, 7,
     "Freezing tail and one-in-four caster-silencing bolt."},
    {2033016, ROL_MONSTER_AVERNUS_GELUGON_HANARIEL, 7,
     "Freezing tail and disarm-intercepting spear trip."},
    {2033020, ROL_MONSTER_PLANAR_SPINAGON_SPIKES, 1,
     "Five-in-six flaming-spike volley with a three-round cooldown."},
    {2033021, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2033022, ROL_MONSTER_AVERNUS_BARBAZU_BERSERK, 20,
     "One-in-twenty reactive Barbazu blood rage."},
    {2033026, ROL_MONSTER_AVERNUS_AUXILIARY, 1, "Black-altar healing and corpse suppression."},
    {2033027, ROL_MONSTER_AVERNUS_AUXILIARY, 1, "Dancing-dagger helper lifecycle."},
    {2034833, ROL_MONSTER_BANSHEE_WAIL, 3, "Room-wide sonic wail."},
    {2041900, ROL_MONSTER_SWALLOW_SPIT, 6, "Nonlethal whole-swallow and spit attack."},
    {2043358, ROL_MONSTER_MOVANIC_DEVA, 1, "Movanic-deva healing and wind assault."},
    {2043702, ROL_MONSTER_MANSCORPION_VENOM_MEDIUM, 7, "Four-tick manscorpion venom."},
    {2043703, ROL_MONSTER_MANSCORPION_VENOM_LIGHT, 31, "Six-tick manscorpion venom."},
    {2043705, ROL_MONSTER_HIVE_SANDSTORM_BEAST, 16, "Room-wide sandstorm damage and blindness."},
    {2043728, ROL_MONSTER_MANSCORPION_VENOM_LIGHT, 31, "Six-tick manscorpion venom."},
    {2043741, ROL_MONSTER_HIVE_SKRIAXIT_SANDSTORM, 1,
     "Three-round sandstorm through open adjacent rooms."},
    {2043742, ROL_MONSTER_HIVE_SKRIAXIT_SANDSTORM, 1,
     "Three-round sandstorm through open adjacent rooms."},
    {2043744, ROL_MONSTER_MANSCORPION_VENOM_LIGHT, 31, "Six-tick manscorpion venom."},
    {2043745, ROL_MONSTER_MANSCORPION_VENOM_MEDIUM, 7, "Four-tick manscorpion venom."},
    {2043746, ROL_MONSTER_MANSCORPION_VENOM_LIGHT, 31, "Six-tick manscorpion venom."},
    {2043756, ROL_MONSTER_MANSCORPION_VENOM_HEAVY, 11, "Two-tick manscorpion venom."},
    {2043758, ROL_MONSTER_MANSCORPION_VENOM_HEAVY, 11, "Two-tick manscorpion venom."},
    {2043759, ROL_MONSTER_MANSCORPION_VENOM_MEDIUM, 7, "Four-tick manscorpion venom."},
    {2043761, ROL_MONSTER_MANSCORPION_VENOM_LIGHT, 31, "Six-tick manscorpion venom."},
    {2043767, ROL_MONSTER_MANSCORPION_VENOM_KING, 25, "Lethal manscorpion-king venom."},
    {2043768, ROL_MONSTER_MANSCORPION_VENOM_HEAVY, 11, "Two-tick manscorpion venom."},
    {2043769, ROL_MONSTER_MANSCORPION_VENOM_HEAVY, 11, "Two-tick manscorpion venom."},
    {2043770, ROL_MONSTER_MANSCORPION_VENOM_HEAVY, 11, "Two-tick manscorpion venom."},
    {2043778, ROL_MONSTER_MANSCORPION_VENOM_HEAVY, 11, "Two-tick manscorpion venom."},
    {2043780, ROL_MONSTER_MANSCORPION_VENOM_MEDIUM, 7, "Four-tick manscorpion venom."},
    {2045116, ROL_MONSTER_FOUR_ARMS, 1, "Extra swing and crushing shockwave."},
    {2045146, ROL_MONSTER_TENTACLE_SLAM, 11, "Room-wide tentacle shockwave."},
    {2045182, ROL_MONSTER_ROT_BRINGER, 1, "One-time flesh helper below forty percent health."},
    {2051246, ROL_MONSTER_WINGED_DEVA, 11, "Healing lightning burst and earthquake."},
    {2051333, ROL_MONSTER_KAMERYNN, 3, "Kamerynn's damaging teleport strike."},
    {2051334, ROL_MONSTER_CANTHUS, 1, "Canthus pack summons and elemental breath."},
    {2053264, ROL_MONSTER_SMALL_PRISMATIC, 11, "Bound helper prismatic spray."},
    {2053265, ROL_MONSTER_CRITICAL_PRISMATIC, 20,
     "Prismatic burst adapted from a source critical event."},
    {2053266, ROL_MONSTER_UBER_PRISMATIC, 3, "Frequent prismatic spray."},
    {2059815, ROL_MONSTER_RESIDUAL_MOBILE, 1, "Delayed extraplanar vanishing."},
    {2059835, ROL_MONSTER_RESIDUAL_MOBILE, 1, "Delayed extraplanar vanishing."},
    {2062401, ROL_MONSTER_FIRE_BOSS, 2, "Room-wide elemental fire storm."},
    {2062402, ROL_MONSTER_EARTH_BOSS, 2, "Room-wide falling-rock assault."},
    {2062405, ROL_MONSTER_AIR_BOSS, 2, "Whirlwind strike and forced movement."},
    {2062406, ROL_MONSTER_WATER_BOSS, 2, "Room-wide tidal assault and silence."},
    {2062701, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie prism, fire, and search."},
    {2062702, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie search."},
    {2062703, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie prism, fire, and search."},
    {2062704, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie prism, fire, and search."},
    {2062705, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie prism, fire, and search."},
    {2062706, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie prism and search."},
    {2062707, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie illusionist prism, fire, and search."},
    {2062708, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie prism and fire."},
    {2062710, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie search."},
    {2062711, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie prism and search."},
    {2062712, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie prism, fire, and search."},
    {2062713, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie prism, fire, and search."},
    {2062714, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie prism and fire."},
    {2062715, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie search."},
    {2062716, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie search."},
    {2062717, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie prism and search."},
    {2062721, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie prism and fire."},
    {2062722, ROL_MONSTER_SEELIE_FAERIE, 1, "Seelie faerie fire."},
    {2081706, ROL_MONSTER_PIT_FIEND_BITE_TAIL, 16, "Venomous bite and crushing tail."},
    {2081746, ROL_MONSTER_PIT_FIEND_BITE_TAIL, 16, "Venomous bite and crushing tail."},
    {2081747, ROL_MONSTER_PIT_FIEND_BITE_TAIL, 16, "Venomous bite and crushing tail."},
    {2083224, ROL_MONSTER_PIT_FIEND_BITE_TAIL, 16, "Venomous bite and crushing tail."},
    {2089793, ROL_MONSTER_DUSK_PARALYSIS_GAZE, 4,
     "Baleful gaze that turns the first failed room target to stone."},
    {2089794, ROL_MONSTER_DUSK_PARALYSIS_GAZE, 2,
     "Baleful gaze that turns the first failed room target to stone."},
    {2092608, ROL_MONSTER_PIERCER, 1, "One-shot hidden piercer ambush."},
    {2093061, ROL_MONSTER_UM2_MANSCORPION_TAIL, 1,
     "Critical venom tail that inflicts two to twelve ticks of paralysis."},
    {2093102, ROL_MONSTER_UM2_DROW_CONCLAVE_GUARD, 1,
     "One-per-boot conclave alarm, redeployment, detection, and combat speech."},
    {2093108, ROL_MONSTER_UM2_DROW_CONCLAVE_GUARD, 1,
     "One-per-boot conclave alarm, redeployment, detection, and combat speech."},
    {2093109, ROL_MONSTER_UM2_DROW_CONCLAVE_GUARD, 1,
     "One-per-boot conclave alarm, redeployment, detection, and combat speech."},
    {2093110, ROL_MONSTER_UM2_DROW_CONCLAVE_GUARD, 1,
     "One-per-boot conclave alarm, redeployment, detection, and combat speech."},
    {2093111, ROL_MONSTER_UM2_DROW_CONCLAVE_GUARD, 1,
     "One-per-boot conclave alarm, redeployment, detection, and combat speech."},
    {2093112, ROL_MONSTER_UM2_DROW_CONCLAVE_GUARD, 1,
     "One-per-boot conclave alarm, redeployment, detection, and combat speech."},
    {2093202, ROL_MONSTER_PLANAR_SUCCUBUS_CHARM, 4,
     "Male-target charm, Blackguard service, command restraint, and delayed kiss."},
    {2093204, ROL_MONSTER_PLANAR_BALOR, 1,
     "Balor equipment upkeep, elemental protection, and corpse suppression."},
    {2093205, ROL_MONSTER_PLANAR_GLABREZU_GRAB, 11,
     "Pincer capture, captive command restraint, and delayed combat."},
    {2093206, ROL_MONSTER_PLANAR_MARILITH_TAIL, 11,
     "Tail capture, captive command restraint, and delayed combat."},
    {2093209, ROL_MONSTER_PLANAR_VROCK_BURSTS, 1,
     "Screech, spore burst, and five-Vrock dance of ruin."},
    {2093210, ROL_MONSTER_PLANAR_VROCK_BURSTS, 1,
     "Screech, spore burst, and five-Vrock dance of ruin."},
    {2093219, ROL_MONSTER_UM2_WYVERN_TAIL, 1, "Critical venom tail with a fatal paralysis save."},
    {2094505, ROL_MONSTER_DARKHOLD_SHADOW_FIEND, 1,
     "Successful-hit darkness and mind-steal attacks with independent cooldowns."},
    {2094506, ROL_MONSTER_DARKHOLD_SHADOW_DRAGON, 1,
     "Death unlocks and reveals the nearby northern passage."},
    {2094563, ROL_MONSTER_UM2_WYVERN_TAIL, 1, "Critical venom tail with a fatal paralysis save."},
    {2096631, ROL_MONSTER_GREYCLOAK_BANSHEE_WAIL, 6, "Room-wide Greycloak banshee wail."},
    {2096670, ROL_MONSTER_GREYCLOAK_FUMES, 11, "Room-wide noxious fumes."},
    {2096672, ROL_MONSTER_GREYCLOAK_ARALESH, 11, "Lethal blazing-eye beam."},
    {2097061, ROL_MONSTER_SWALLOW_WHOLE, 10, "Rhemorhaz bite and whole-swallow attack."},
};

/* Keep this table sorted by converted mobile VNUM for binary lookup. */
static const struct rol_seelie_faerie_profile rol_seelie_faerie_profiles[] = {
    {2062701, ROL_SEELIE_FAERIE_FIRE | ROL_SEELIE_PRISMATIC | ROL_SEELIE_SEARCH},
    {2062702, ROL_SEELIE_SEARCH},
    {2062703, ROL_SEELIE_FAERIE_FIRE | ROL_SEELIE_PRISMATIC | ROL_SEELIE_SEARCH},
    {2062704, ROL_SEELIE_FAERIE_FIRE | ROL_SEELIE_PRISMATIC | ROL_SEELIE_SEARCH},
    {2062705, ROL_SEELIE_FAERIE_FIRE | ROL_SEELIE_PRISMATIC | ROL_SEELIE_SEARCH},
    {2062706, ROL_SEELIE_PRISMATIC | ROL_SEELIE_SEARCH},
    {2062707, ROL_SEELIE_FAERIE_FIRE | ROL_SEELIE_PRISMATIC | ROL_SEELIE_SEARCH},
    {2062708, ROL_SEELIE_FAERIE_FIRE | ROL_SEELIE_PRISMATIC},
    {2062710, ROL_SEELIE_SEARCH},
    {2062711, ROL_SEELIE_PRISMATIC | ROL_SEELIE_SEARCH},
    {2062712, ROL_SEELIE_FAERIE_FIRE | ROL_SEELIE_PRISMATIC | ROL_SEELIE_SEARCH},
    {2062713, ROL_SEELIE_FAERIE_FIRE | ROL_SEELIE_PRISMATIC | ROL_SEELIE_SEARCH},
    {2062714, ROL_SEELIE_FAERIE_FIRE | ROL_SEELIE_PRISMATIC},
    {2062715, ROL_SEELIE_SEARCH},
    {2062716, ROL_SEELIE_SEARCH},
    {2062717, ROL_SEELIE_PRISMATIC | ROL_SEELIE_SEARCH},
    {2062721, ROL_SEELIE_FAERIE_FIRE | ROL_SEELIE_PRISMATIC},
    {2062722, ROL_SEELIE_FAERIE_FIRE},
};

static const struct rol_monster_combat_profile *rol_monster_combat_profile_for(int mobile_vnum)
{
  size_t high = sizeof(rol_monster_combat_profiles) / sizeof(rol_monster_combat_profiles[0]);
  size_t low = 0;
  size_t middle;

  while (low < high)
  {
    middle = low + (high - low) / 2;
    if (rol_monster_combat_profiles[middle].mobile_vnum < mobile_vnum)
      low = middle + 1;
    else
      high = middle;
  }
  if (low < sizeof(rol_monster_combat_profiles) / sizeof(rol_monster_combat_profiles[0]) &&
      rol_monster_combat_profiles[low].mobile_vnum == mobile_vnum)
    return &rol_monster_combat_profiles[low];

  return NULL;
}

size_t rol_monster_combat_profile_count(void)
{
  return sizeof(rol_monster_combat_profiles) / sizeof(rol_monster_combat_profiles[0]);
}

bool rol_monster_combat_profile(int mobile_vnum, int *proc_denominator, const char **description)
{
  const struct rol_monster_combat_profile *profile = rol_monster_combat_profile_for(mobile_vnum);

  if (profile == NULL)
    return false;
  if (proc_denominator != NULL)
    *proc_denominator = profile->proc_denominator;
  if (description != NULL)
    *description = profile->description;
  return true;
}

bool rol_griffon_guard_target_allowed(const struct char_data *target)
{
  if (target == NULL || IS_NPC(target) || target->player_specials == NULL ||
      GET_LEVEL(target) >= LVL_IMMORT)
    return false;

  /* RoL's Barbarian player race became the target's multiclass Berserker role. */
  return CLASS_LEVEL(target, CLASS_BERSERKER) <= 0;
}

bool rol_drow_conclave_guard_profile(int mobile_vnum)
{
  const struct rol_monster_combat_profile *profile = rol_monster_combat_profile_for(mobile_vnum);

  return profile != NULL && profile->effect == ROL_MONSTER_UM2_DROW_CONCLAVE_GUARD;
}

bool rol_drow_conclave_detect_guard_profile(int mobile_vnum)
{
  switch (mobile_vnum)
  {
  case 2093101:
  case 2093102:
  case 2093109:
  case 2093110:
  case 2093112:
    return true;
  default:
    return false;
  }
}

int rol_drow_conclave_destination_vnum(int deployment_index)
{
  switch (deployment_index)
  {
  case 0:
    return ROL_DROW_CONCLAVE_BASE_VNUM;
  case 1:
  case 2:
    return ROL_DROW_CONCLAVE_BASE_VNUM + 1;
  case 3:
    return ROL_DROW_CONCLAVE_BASE_VNUM + 9;
  default:
    return -1;
  }
}

const char *rol_drow_conclave_combat_line(int roll)
{
  static const char *const lines[] = {"The power of Lloth knows no equal!",
                                      "Prepare to die infidel!",
                                      "Guards! Help me!",
                                      "You are no match for my superior skills!",
                                      "You will fall before the wrath of Lloth!",
                                      "Sound the Alarm! We are under attack!"};

  if (roll < 1 || roll > (int)(sizeof(lines) / sizeof(lines[0])))
    return NULL;
  return lines[roll - 1];
}

#ifdef LUMINARI_CUTEST
void rol_drow_conclave_reset_alarm_for_tests(void)
{
  rol_drow_conclave_alarm_sounding = false;
}
#endif

bool rol_paralysis_hit_profile(int mobile_vnum, bool *critical_only, bool *fatal, int *duration_min,
                               int *duration_max)
{
  const struct rol_monster_combat_profile *profile = rol_monster_combat_profile_for(mobile_vnum);
  bool result_critical;
  bool result_fatal;
  int result_duration_min;
  int result_duration_max;

  if (profile == NULL)
    return false;
  switch (profile->effect)
  {
  case ROL_MONSTER_DUSK_PARALYSIS_GAZE:
    result_critical = false;
    result_fatal = false;
    result_duration_min = 10;
    result_duration_max = 10;
    break;
  case ROL_MONSTER_UM2_MANSCORPION_TAIL:
    result_critical = true;
    result_fatal = false;
    result_duration_min = 2;
    result_duration_max = 12;
    break;
  case ROL_MONSTER_UM2_WYVERN_TAIL:
    result_critical = true;
    result_fatal = true;
    result_duration_min = 0;
    result_duration_max = 0;
    break;
  default:
    return false;
  }

  if (critical_only != NULL)
    *critical_only = result_critical;
  if (fatal != NULL)
    *fatal = result_fatal;
  if (duration_min != NULL)
    *duration_min = result_duration_min;
  if (duration_max != NULL)
    *duration_max = result_duration_max;
  return true;
}

int rol_dusk_paralysis_proc_denominator(int level)
{
  int frequency = (61 - level) / 7;

  return MAX(0, frequency) + 1;
}

int rol_dusk_paralysis_save_bonus(int level)
{
  return -((30 - level) / 10);
}

bool rol_planar_death_profile(int mobile_vnum, bool *suppresses_corpse)
{
  const struct rol_monster_combat_profile *profile = rol_monster_combat_profile_for(mobile_vnum);

  if (profile == NULL || (profile->effect != ROL_MONSTER_PLANAR_MANES_DEATH &&
                          profile->effect != ROL_MONSTER_PLANAR_BALOR))
    return false;
  if (suppresses_corpse != NULL)
    *suppresses_corpse = profile->effect == ROL_MONSTER_PLANAR_BALOR;
  return true;
}

bool rol_planar_burst_profile(int mobile_vnum, bool *screech, bool *spores, bool *flame_spikes)
{
  const struct rol_monster_combat_profile *profile = rol_monster_combat_profile_for(mobile_vnum);
  bool is_vrock;
  bool is_spinagon;

  if (profile == NULL)
    return false;
  is_vrock = profile->effect == ROL_MONSTER_PLANAR_VROCK_BURSTS;
  is_spinagon = profile->effect == ROL_MONSTER_PLANAR_SPINAGON_SPIKES;
  if (!is_vrock && !is_spinagon)
    return false;
  if (screech != NULL)
    *screech = is_vrock;
  if (spores != NULL)
    *spores = is_vrock;
  if (flame_spikes != NULL)
    *flame_spikes = is_spinagon;
  return true;
}

bool rol_avernus_barbazu_profile(int mobile_vnum)
{
  const struct rol_monster_combat_profile *profile = rol_monster_combat_profile_for(mobile_vnum);

  return profile != NULL && profile->effect == ROL_MONSTER_AVERNUS_BARBAZU_BERSERK;
}

bool rol_avernus_gelugon_profile(int mobile_vnum, bool *freezing_tail, bool *silencing_bolt,
                                 bool *blocks_disarm)
{
  const struct rol_monster_combat_profile *profile = rol_monster_combat_profile_for(mobile_vnum);
  bool is_hanariel;
  bool is_meritos;

  if (profile == NULL)
    return false;
  is_meritos = profile->effect == ROL_MONSTER_AVERNUS_GELUGON_MERITOS;
  is_hanariel = profile->effect == ROL_MONSTER_AVERNUS_GELUGON_HANARIEL;
  if (!is_meritos && !is_hanariel)
    return false;
  if (freezing_tail != NULL)
    *freezing_tail = true;
  if (silencing_bolt != NULL)
    *silencing_bolt = is_meritos;
  if (blocks_disarm != NULL)
    *blocks_disarm = is_hanariel;
  return true;
}

bool rol_avernus_barbazu_berserk_roll_fires(int roll)
{
  return roll == 0;
}

bool rol_avernus_gelugon_tail_roll_fires(int roll)
{
  return roll == 0;
}

bool rol_avernus_meritos_silence_roll_fires(int roll)
{
  return roll == 0;
}

bool rol_planar_control_profile(int mobile_vnum, enum rol_planar_control_kind *kind,
                                int *proc_denominator)
{
  const struct rol_monster_combat_profile *profile = rol_monster_combat_profile_for(mobile_vnum);
  enum rol_planar_control_kind result;

  if (profile == NULL)
    return false;
  switch (profile->effect)
  {
  case ROL_MONSTER_PLANAR_GLABREZU_GRAB:
    result = ROL_PLANAR_CONTROL_GLABREZU;
    break;
  case ROL_MONSTER_PLANAR_MARILITH_TAIL:
    result = ROL_PLANAR_CONTROL_MARILITH;
    break;
  case ROL_MONSTER_PLANAR_SUCCUBUS_CHARM:
    result = ROL_PLANAR_CONTROL_SUCCUBUS;
    break;
  default:
    return false;
  }
  if (kind != NULL)
    *kind = result;
  if (proc_denominator != NULL)
    *proc_denominator = profile->proc_denominator;
  return true;
}

bool rol_planar_captive_command_allowed(const char *command)
{
  static const char *const allowed[] = {
      "score", "tell", "shout",  "look", "help",     "who",        "weather",  "save",
      "quit",  "time", "toggle", "ooc",  "commands", "attributes", "petition",
  };
  size_t index;

  if (command == NULL || command[0] == '\0')
    return false;
  for (index = 0; index < sizeof(allowed) / sizeof(allowed[0]); index++)
  {
    if (!str_cmp(command, allowed[index]))
      return true;
  }
  return false;
}

bool rol_planar_restrain_agility_evades(int agility, int roll)
{
  return roll >= 1 && roll <= 100 && roll < agility / 2;
}

bool rol_planar_restrain_constitution_survives(int constitution, int roll)
{
  return roll >= 1 && roll <= 85 && roll <= constitution;
}

bool rol_planar_succubus_charm_roll_fires(int roll)
{
  return roll == 0;
}

int rol_planar_succubus_kiss_delay_seconds(int hours)
{
  return hours >= 1 && hours <= 4 ? hours * SECS_PER_MUD_HOUR : 0;
}

bool rol_planar_vrock_dance_profile(int mobile_vnum)
{
  const struct rol_monster_combat_profile *profile = rol_monster_combat_profile_for(mobile_vnum);

  return profile != NULL && profile->effect == ROL_MONSTER_PLANAR_VROCK_BURSTS;
}

int rol_planar_vrock_dance_required_count(void)
{
  return 5;
}

int rol_planar_vrock_dance_step_seconds(void)
{
  return PULSE_VIOLENCE;
}

int rol_planar_vrock_dance_cooldown_seconds(void)
{
  return SECS_PER_MUD_DAY;
}

bool rol_planar_five_in_six_roll_fires(int roll)
{
  return roll >= 1 && roll <= 5;
}

bool rol_planar_screech_health_allows(int hit, int max_hit)
{
  if (max_hit <= 0)
    return false;
  return ((long long)hit * 100LL) / max_hit <= 15;
}

int rol_planar_screech_cooldown_seconds(int mobile_vnum)
{
  return rol_planar_burst_profile(mobile_vnum, NULL, NULL, NULL) &&
                 rol_monster_combat_profile_for(mobile_vnum)->effect ==
                     ROL_MONSTER_PLANAR_VROCK_BURSTS
             ? SECS_PER_MUD_DAY
             : 0;
}

int rol_planar_hit_burst_cooldown_seconds(int mobile_vnum)
{
  return rol_planar_burst_profile(mobile_vnum, NULL, NULL, NULL) ? ROL_PLANAR_HIT_BURST_COOLDOWN
                                                                 : 0;
}

bool rol_monster_successful_hit_profile(int mobile_vnum, struct rol_monster_hit_profile_view *view)
{
  const struct rol_monster_combat_profile *profile = rol_monster_combat_profile_for(mobile_vnum);
  struct rol_monster_hit_profile_view result = {0};

  if (profile == NULL)
    return false;
  result.proc_denominator = profile->proc_denominator;
  switch (profile->effect)
  {
  case ROL_MONSTER_DOBLUTH_BANSHEE_WAIL:
    result.base_damage = 150;
    result.damage_type = DAM_SOUND;
    break;
  case ROL_MONSTER_DOBLUTH_BLADESTORM:
    result.damage_type = DAM_SLASHING;
    break;
  case ROL_MONSTER_HIVE_SANDSTORM_BEAST:
    result.damage_dice_count = 10;
    result.damage_dice_size = 10;
    result.damage_type = DAM_EARTH;
    break;
  case ROL_MONSTER_GREYCLOAK_BANSHEE_WAIL:
    result.base_damage = 200;
    result.damage_variance = 10;
    result.damage_type = DAM_SOUND;
    break;
  case ROL_MONSTER_GREYCLOAK_FUMES:
    result.base_damage = 300;
    result.damage_variance = 10;
    result.damage_type = DAM_POISON;
    break;
  case ROL_MONSTER_GREYCLOAK_ARALESH:
    result.fatal = true;
    result.damage_type = DAM_LIGHT;
    break;
  default:
    return false;
  }

  if (view != NULL)
    *view = result;
  return true;
}

bool rol_monster_successful_hit_roll_fires(int mobile_vnum, int roll)
{
  return roll == 1 && rol_monster_successful_hit_profile(mobile_vnum, NULL);
}

bool rol_skriaxit_sandstorm_profile(int mobile_vnum, int *round_interval,
                                    bool *reaches_open_adjacent_rooms)
{
  const struct rol_monster_combat_profile *profile = rol_monster_combat_profile_for(mobile_vnum);

  if (profile == NULL || profile->effect != ROL_MONSTER_HIVE_SKRIAXIT_SANDSTORM)
    return false;
  if (round_interval != NULL)
    *round_interval = 3;
  if (reaches_open_adjacent_rooms != NULL)
    *reaches_open_adjacent_rooms = true;
  return true;
}

int rol_skriaxit_sandstorm_source_damage(int skriaxit_count)
{
  /* The bound source room loop resets this count before evaluating 3 * num. */
  (void)skriaxit_count;
  return 0;
}

int rol_skriaxit_sandstorm_advance_round(int current_round, bool *fires)
{
  bool result = false;

  current_round = MAX(0, current_round) + 1;
  if (current_round >= 3)
  {
    current_round = 0;
    result = true;
  }
  if (fires != NULL)
    *fires = result;
  return current_round;
}

bool rol_manscorpion_venom_profile(int mobile_vnum, int *proc_denominator, int *duration,
                                   bool *fatal_without_slow_poison)
{
  const struct rol_monster_combat_profile *profile = rol_monster_combat_profile_for(mobile_vnum);
  int venom_duration;
  bool fatal;

  if (profile == NULL)
    return false;
  fatal = false;
  switch (profile->effect)
  {
  case ROL_MONSTER_MANSCORPION_VENOM_LIGHT:
    venom_duration = 6;
    break;
  case ROL_MONSTER_MANSCORPION_VENOM_MEDIUM:
    venom_duration = 4;
    break;
  case ROL_MONSTER_MANSCORPION_VENOM_HEAVY:
    venom_duration = 2;
    break;
  case ROL_MONSTER_MANSCORPION_VENOM_KING:
    venom_duration = 1;
    fatal = true;
    break;
  default:
    return false;
  }

  if (proc_denominator != NULL)
    *proc_denominator = profile->proc_denominator;
  if (duration != NULL)
    *duration = venom_duration;
  if (fatal_without_slow_poison != NULL)
    *fatal_without_slow_poison = fatal;
  return true;
}

bool rol_manscorpion_venom_roll_fires(int mobile_vnum, int roll)
{
  if (!rol_manscorpion_venom_profile(mobile_vnum, NULL, NULL, NULL))
    return false;
  return roll == 1;
}

bool rol_manscorpion_apply_venom(struct char_data *victim, int duration)
{
  struct affected_type af;

  if (victim == NULL || duration <= 0 || !can_poison(victim) ||
      affected_by_spell(victim, AFFECT_ROL_MANSCORPION_VENOM))
    return false;

  new_affect(&af);
  af.spell = AFFECT_ROL_MANSCORPION_VENOM;
  af.duration = duration;
  af.location = APPLY_CON;
  af.modifier = -2;
  affect_to_char(victim, &af);
  return true;
}

static const struct rol_seelie_faerie_profile *rol_seelie_faerie_profile_for(int mobile_vnum)
{
  size_t high = sizeof(rol_seelie_faerie_profiles) / sizeof(rol_seelie_faerie_profiles[0]);
  size_t low = 0;
  size_t middle;

  while (low < high)
  {
    middle = low + (high - low) / 2;
    if (rol_seelie_faerie_profiles[middle].mobile_vnum < mobile_vnum)
      low = middle + 1;
    else
      high = middle;
  }
  if (low < sizeof(rol_seelie_faerie_profiles) / sizeof(rol_seelie_faerie_profiles[0]) &&
      rol_seelie_faerie_profiles[low].mobile_vnum == mobile_vnum)
    return &rol_seelie_faerie_profiles[low];

  return NULL;
}

bool rol_seelie_faerie_profile(int mobile_vnum, bool *faerie_fire, bool *prismatic, bool *search)
{
  const struct rol_seelie_faerie_profile *profile = rol_seelie_faerie_profile_for(mobile_vnum);

  if (profile == NULL)
    return false;
  if (faerie_fire != NULL)
    *faerie_fire = (profile->abilities & ROL_SEELIE_FAERIE_FIRE) != 0;
  if (prismatic != NULL)
    *prismatic = (profile->abilities & ROL_SEELIE_PRISMATIC) != 0;
  if (search != NULL)
    *search = (profile->abilities & ROL_SEELIE_SEARCH) != 0;
  return true;
}

bool rol_seelie_faerie_runs_while_disabled(int mobile_vnum)
{
  return rol_seelie_faerie_profile_for(mobile_vnum) != NULL;
}

int rol_seelie_prismatic_beam_count(int roll)
{
  if (roll < 2 || roll > 5)
    return 0;
  return roll / 2;
}

int rol_seelie_prismatic_damage(int color)
{
  static const int damage_by_color[] = {420, 280, 140, 0, 0, 0, 0, 0};

  if (color < 0 || color >= (int)(sizeof(damage_by_color) / sizeof(damage_by_color[0])))
    return 0;
  return damage_by_color[color];
}

int rol_seelie_search_stun_rounds(int mobile_vnum)
{
  const struct rol_seelie_faerie_profile *profile = rol_seelie_faerie_profile_for(mobile_vnum);

  if (profile == NULL || (profile->abilities & ROL_SEELIE_SEARCH) == 0)
    return 0;
  return mobile_vnum == 2062707 ? 6 : 3;
}

static bool rol_monster_fires(const struct rol_monster_combat_profile *profile)
{
  return profile->proc_denominator <= 1 || rand_number(1, profile->proc_denominator) == 1;
}

static bool rol_monster_room_target(const struct char_data *ch, const struct char_data *victim)
{
  if (ch == NULL || victim == NULL || victim == ch || IN_ROOM(ch) == NOWHERE ||
      IN_ROOM(victim) != IN_ROOM(ch) || GET_POS(victim) <= POS_DEAD ||
      GET_LEVEL(victim) >= LVL_IMMORT)
    return false;

  return !IS_NPC(victim) || IS_PET(victim);
}

static void rol_monster_stun(struct char_data *victim, int rounds)
{
  if (victim == NULL || rounds <= 0)
    return;

  if (GET_POS(victim) > POS_SITTING)
    GET_POS(victim) = POS_SITTING;
  resetCastingData(victim);
  if (can_stun(victim) && char_has_mud_event(victim, eSTUNNED) == NULL)
    attach_mud_event(new_mud_event(eSTUNNED, victim, NULL), PULSE_VIOLENCE * rounds);
}

static struct char_data *rol_monster_random_player(struct char_data *ch)
{
  struct char_data *candidate;
  struct char_data *selected = NULL;
  int eligible = 0;

  for (candidate = world[IN_ROOM(ch)].people; candidate != NULL;
       candidate = candidate->next_in_room)
  {
    if (!rol_monster_room_target(ch, candidate) || IS_NPC(candidate))
      continue;
    eligible++;
    if (rand_number(1, eligible) == 1)
      selected = candidate;
  }
  return selected;
}

static bool rol_dusk_paralysis_target_allowed(struct char_data *ch, struct char_data *victim)
{
  if (ch == NULL || victim == NULL || victim == ch || IN_ROOM(ch) == NOWHERE ||
      IN_ROOM(victim) != IN_ROOM(ch) || GET_LEVEL(victim) >= LVL_IMMORT ||
      affected_by_spell(victim, SPELL_HOLD_MONSTER) || !CAN_SEE(victim, ch))
    return false;
  if (!IS_NPC(victim))
    return true;
  return AFF_FLAGGED(victim, AFF_CHARM) || FIGHTING(victim) == ch;
}

static bool rol_monster_apply_paralysis(struct char_data *victim, int duration)
{
  struct affected_type affect;

  if (victim == NULL || duration <= 0)
    return false;
  new_affect(&affect);
  affect.spell = SPELL_HOLD_MONSTER;
  affect.duration = duration;
  SET_BIT_AR(affect.bitvector, AFF_PARALYZED);
  affect_to_char(victim, &affect);
  return true;
}

static int rol_monster_dusk_paralysis_hit(struct spec_event_context *context, struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next;
  int denominator;

  if (context == NULL || ch == NULL || FIGHTING(ch) == NULL)
    return FALSE;
  denominator = rol_dusk_paralysis_proc_denominator(GET_LEVEL(ch));
  if (rand_number(1, denominator) != 1)
    return FALSE;

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_dusk_paralysis_target_allowed(ch, victim))
      continue;
    if (paralysis_immunity(victim) ||
        savingthrow(ch, victim, SAVING_FORT, rol_dusk_paralysis_save_bonus(GET_LEVEL(ch)),
                    CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
    {
      act("You manage to avoid $n's baleful gaze!", TRUE, ch, NULL, victim, TO_VICT);
      continue;
    }

    act("$N's body turns to stone as $n stares at $M!", TRUE, ch, NULL, victim, TO_NOTVICT);
    act("Your body turns to stone as $n stares at you!", FALSE, ch, NULL, victim, TO_VICT);
    (void)rol_monster_apply_paralysis(victim, 10);
    return TRUE;
  }
  return FALSE;
}

static int rol_monster_venom_tail_hit(struct spec_event_context *context, struct char_data *ch)
{
  struct char_data *victim;
  bool critical_only;
  bool fatal;
  int duration_min;
  int duration_max;
  int duration;

  if (context == NULL || ch == NULL ||
      !rol_paralysis_hit_profile(GET_MOB_VNUM(ch), &critical_only, &fatal, &duration_min,
                                 &duration_max) ||
      (critical_only && !context->critical) || FIGHTING(ch) != context->target)
    return FALSE;
  victim = context->target;

  act("$n swings $s tail toward $N, stinging $M with it.", FALSE, ch, NULL, victim, TO_NOTVICT);
  act("You swing your tail toward $N, stinging $M with it.", FALSE, ch, NULL, victim, TO_CHAR);
  act("$n swings $s tail toward you, stinging you with it.", FALSE, ch, NULL, victim, TO_VICT);

  if (GET_LEVEL(victim) >= LVL_IMMORT || paralysis_immunity(victim) ||
      savingthrow(ch, victim, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
  {
    act("You feel slightly sick, but the feeling quickly subsides.", FALSE, ch, NULL, victim,
        TO_VICT);
    act("$N looks a bit pale, but fights on.", FALSE, ch, NULL, victim, TO_NOTVICT);
    act("$N looks a bit pale, but fights on.", FALSE, ch, NULL, victim, TO_CHAR);
    return TRUE;
  }

  if (fatal)
  {
    act("The poison takes effect quickly, seizing your nervous system. You go into convulsions.",
        FALSE, ch, NULL, victim, TO_VICT);
    act("$N looks slightly pale as $E goes into convulsions.", FALSE, ch, NULL, victim, TO_NOTVICT);
    act("$N looks slightly pale as $E goes into convulsions.", FALSE, ch, NULL, victim, TO_CHAR);
    context->invalidation |= SPEC_INVALIDATE_TARGET;
    die(victim, ch);
    return TRUE;
  }

  act("The poison takes effect quickly, seizing your nervous system. You stiffen up and cease "
      "to move.",
      FALSE, ch, NULL, victim, TO_VICT);
  act("$N looks slightly stiff and ceases to move.", FALSE, ch, NULL, victim, TO_NOTVICT);
  act("$N looks slightly stiff and ceases to move.", FALSE, ch, NULL, victim, TO_CHAR);
  duration = rand_number(duration_min, duration_max);
  (void)rol_monster_apply_paralysis(victim, duration);
  return TRUE;
}

static int rol_monster_manscorpion_hit(struct spec_event_context *context,
                                       const struct rol_monster_combat_profile *profile,
                                       struct char_data *ch)
{
  struct char_data *victim;
  bool fatal_without_slow_poison;
  int duration;
  int roll;

  if (context == NULL || profile == NULL || ch == NULL || FIGHTING(ch) == NULL ||
      !rol_manscorpion_venom_profile(GET_MOB_VNUM(ch), NULL, &duration, &fatal_without_slow_poison))
    return FALSE;

  roll = rand_number(1, profile->proc_denominator);
  if (!rol_manscorpion_venom_roll_fires(GET_MOB_VNUM(ch), roll))
    return FALSE;
  victim = rol_monster_random_player(ch);
  if (victim == NULL)
    return FALSE;

  act("$n whips $s massive tail into $N, flooding the wound with venom!", FALSE, ch, NULL, victim,
      TO_NOTVICT);
  act("$n whips $s massive tail into you, flooding the wound with venom!", FALSE, ch, NULL, victim,
      TO_VICT);

  if (fatal_without_slow_poison && !AFF2_FLAGGED(victim, AFF2_ROL_SLOW_POISON))
  {
    act("The venom overwhelms you; your body blackens and collapses!", FALSE, ch, NULL, victim,
        TO_VICT);
    act("$N falls, $S body blackened and contorted by the venom!", FALSE, ch, NULL, victim,
        TO_NOTVICT);
    if (victim == context->target)
      context->invalidation |= SPEC_INVALIDATE_TARGET;
    die(victim, ch);
    return FALSE;
  }

  if (!can_poison(victim) || affected_by_spell(victim, AFFECT_ROL_MANSCORPION_VENOM))
    return FALSE;
  if (savingthrow(ch, victim, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(victim), NOSCHOOL))
  {
    send_to_char(victim, "The toxins dissolve harmlessly in your bloodstream!\r\n");
    return FALSE;
  }
  if (rol_manscorpion_apply_venom(victim, duration))
  {
    send_to_char(victim, "You shudder in pain as the toxins flow through you.\r\n");
    act("$n shudders in pain and looks very pale.", TRUE, victim, NULL, NULL, TO_ROOM);
  }
  return FALSE;
}

static bool rol_monster_hit_area_target(struct char_data *ch, struct char_data *victim)
{
  return rol_monster_room_target(ch, victim) && aoeOK(ch, victim, -1);
}

static int rol_monster_successful_hit_damage(struct spec_event_context *context,
                                             struct char_data *ch, struct char_data *victim,
                                             int amount, int damage_type)
{
  bool is_hit_target;
  int result;

  if (context == NULL || ch == NULL || victim == NULL || amount < 0)
    return 0;
  is_hit_target = victim == context->target;
  result = damage(ch, victim, amount, -1, damage_type, FALSE);
  if (result < 0 && is_hit_target)
    context->invalidation |= SPEC_INVALIDATE_TARGET;
  return result;
}

static int rol_monster_bladestorm_weapon_damage(const struct obj_data *obj)
{
  if (obj == NULL)
    return 0;
  return MAX(0, GET_OBJ_VAL(obj, 1)) * MAX(0, GET_OBJ_VAL(obj, 2));
}

static int rol_monster_bladestorm_damage(struct char_data *ch)
{
  struct char_data *victim;
  int amount = 0;

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = victim->next_in_room)
  {
    if (victim == ch)
      continue;
    amount += rol_monster_bladestorm_weapon_damage(GET_EQ(victim, WEAR_WIELD_1));
    amount += rol_monster_bladestorm_weapon_damage(GET_EQ(victim, WEAR_WIELD_2H));
    amount += rol_monster_bladestorm_weapon_damage(GET_EQ(victim, WEAR_WIELD_OFFHAND));
  }
  return amount;
}

static void rol_monster_sandstorm_blind(struct char_data *ch, struct char_data *victim)
{
  struct affected_type af;

  if (!can_blind(victim) || rand_number(0, 1) != 0)
    return;
  act("The swirling sands blind you!", FALSE, ch, NULL, victim, TO_VICT);
  act("The swirling sands blind $N!", FALSE, ch, NULL, victim, TO_NOTVICT);
  new_affect(&af);
  af.spell = SPELL_BLINDNESS;
  af.duration = 3;
  SET_BIT_AR(af.bitvector, AFF_BLIND);
  affect_join(victim, &af, FALSE, FALSE, FALSE, FALSE);
}

static int rol_monster_dobluth_wail_hit(struct spec_event_context *context, struct char_data *ch,
                                        const struct rol_monster_hit_profile_view *view)
{
  struct char_data *victim;
  struct char_data *next;
  int amount;
  int result;

  act("$n wails horribly, and the shockwave fills you with pain and terror!", FALSE, ch, NULL, NULL,
      TO_ROOM);
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_hit_area_target(ch, victim))
      continue;
    amount = view->base_damage;
    if (savingthrow(ch, victim, SAVING_WILL, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
      amount /= 2;
    result = rol_monster_successful_hit_damage(context, ch, victim, amount, view->damage_type);
    if (result >= 0 &&
        !savingthrow(ch, victim, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
      rol_monster_stun(victim, rand_number(2, 4));
  }
  return TRUE;
}

static int rol_monster_bladestorm_hit(struct spec_event_context *context, struct char_data *ch,
                                      const struct rol_monster_hit_profile_view *view)
{
  struct char_data *victim;
  struct char_data *next;
  int amount = rol_monster_bladestorm_damage(ch);
  int target_amount;

  if (amount <= 0)
    return FALSE;
  act("Suddenly, $n raises $s arms and every weapon in the room tears free, careening through a "
      "vicious storm of steel before returning to its owner!",
      FALSE, ch, NULL, NULL, TO_ROOM);
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_hit_area_target(ch, victim))
      continue;
    target_amount = amount;
    if (savingthrow(ch, victim, SAVING_REFL, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
      target_amount /= 2;
    (void)rol_monster_successful_hit_damage(context, ch, victim, target_amount, view->damage_type);
  }
  return TRUE;
}

static int rol_monster_sandstorm_beast_hit(struct spec_event_context *context, struct char_data *ch,
                                           const struct rol_monster_hit_profile_view *view)
{
  struct char_data *victim;
  struct char_data *next;

  act("$n throws $mself into a spin, creating a tornado of sand and stones!", TRUE, ch, NULL, NULL,
      TO_ROOM);
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_hit_area_target(ch, victim))
      continue;
    rol_monster_sandstorm_blind(ch, victim);
    (void)rol_monster_successful_hit_damage(context, ch, victim,
                                            dice(view->damage_dice_count, view->damage_dice_size),
                                            view->damage_type);
  }
  return TRUE;
}

static int rol_monster_greycloak_area_hit(struct spec_event_context *context, struct char_data *ch,
                                          const struct rol_monster_hit_profile_view *view,
                                          bool wail)
{
  struct char_data *victim;
  struct char_data *next;
  int amount;

  if (wail && (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) || AFF_FLAGGED(ch, AFF_SILENCED)))
    return FALSE;
  if (wail)
    act("$n's wailing chills you to the bone!", FALSE, ch, NULL, NULL, TO_ROOM);
  else
    act("$n creates an enormous cloud of noxious fumes!", FALSE, ch, NULL, NULL, TO_ROOM);

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_hit_area_target(ch, victim))
      continue;
    amount = view->base_damage + rand_number(-view->damage_variance, view->damage_variance);
    if (savingthrow(ch, victim, SAVING_WILL, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
      amount /= 2;
    if (!wail)
      send_to_char(victim, "You suffer as the fumes engulf you.\r\n");
    (void)rol_monster_successful_hit_damage(context, ch, victim, amount, view->damage_type);
  }
  return TRUE;
}

static int rol_monster_aralesh_hit(struct spec_event_context *context, struct char_data *ch)
{
  struct char_data *owner = NULL;
  struct char_data *victim = FIGHTING(ch);

  if (victim == NULL || spec_context_validate_combat_target(ch, victim, true) != SPEC_CONTEXT_VALID)
    return FALSE;
  if (IS_NPC(victim) && victim->master != NULL && !IS_NPC(victim->master))
    owner = victim->master;

  act("$n emits a blazing beam of light from $s eyes, turning $N to ash!", TRUE, ch, NULL, victim,
      TO_NOTVICT);
  act("$n emits a blazing beam of light from $s eyes. You die in agony as your body turns to ash!",
      TRUE, ch, NULL, victim, TO_VICT);
  if (victim == context->target)
    context->invalidation |= SPEC_INVALIDATE_TARGET;
  die(victim, ch);

  if (owner != NULL && rol_monster_room_target(ch, owner) && !IS_NPC(owner))
  {
    act("$n's blazing gaze turns $N to ash beside $S fallen companion!", TRUE, ch, NULL, owner,
        TO_NOTVICT);
    act("$n's blazing gaze strikes you next, turning your body to ash!", TRUE, ch, NULL, owner,
        TO_VICT);
    if (owner == context->target)
      context->invalidation |= SPEC_INVALIDATE_TARGET;
    die(owner, ch);
  }
  return TRUE;
}

static int rol_monster_successful_hit(struct spec_event_context *context,
                                      const struct rol_monster_combat_profile *profile,
                                      struct char_data *ch)
{
  struct rol_monster_hit_profile_view view;

  if (context == NULL || profile == NULL || ch == NULL || FIGHTING(ch) == NULL ||
      !rol_monster_successful_hit_profile(GET_MOB_VNUM(ch), &view) ||
      !rol_monster_successful_hit_roll_fires(GET_MOB_VNUM(ch),
                                             rand_number(1, profile->proc_denominator)))
    return FALSE;

  switch (profile->effect)
  {
  case ROL_MONSTER_DOBLUTH_BANSHEE_WAIL:
    return rol_monster_dobluth_wail_hit(context, ch, &view);
  case ROL_MONSTER_DOBLUTH_BLADESTORM:
    return rol_monster_bladestorm_hit(context, ch, &view);
  case ROL_MONSTER_HIVE_SANDSTORM_BEAST:
    return rol_monster_sandstorm_beast_hit(context, ch, &view);
  case ROL_MONSTER_GREYCLOAK_BANSHEE_WAIL:
    return rol_monster_greycloak_area_hit(context, ch, &view, true);
  case ROL_MONSTER_GREYCLOAK_FUMES:
    return rol_monster_greycloak_area_hit(context, ch, &view, false);
  case ROL_MONSTER_GREYCLOAK_ARALESH:
    return rol_monster_aralesh_hit(context, ch);
  default:
    return FALSE;
  }
}

static bool rol_planar_cooldown_ready(time_t ready_at, time_t now)
{
  return ready_at <= 0 || ready_at <= now;
}

static struct char_data *rol_planar_first_captive(struct char_data *ch)
{
  struct follow_type *follower;

  for (follower = ch->followers; follower != NULL; follower = follower->next)
  {
    if (follower->follower != NULL && follower->follower->master == ch &&
        AFF_FLAGGED(follower->follower, AFF_CHARM))
      return follower->follower;
  }
  return NULL;
}

static void rol_planar_make_follower(struct char_data *victim, struct char_data *ch)
{
  if (victim->master != NULL)
    stop_follower(victim);
  add_follower(victim, ch);
  SET_BIT_AR(AFF_FLAGS(victim), AFF_CHARM);
}

static int rol_planar_restrain_hit(struct spec_event_context *context, struct char_data *ch,
                                   enum rol_planar_control_kind kind)
{
  struct char_data *victim = FIGHTING(ch);

  if (victim == NULL || ch->mob_specials.rol_planar_captive_active ||
      GET_LEVEL(victim) >= LVL_IMMORT || rand_number(0, 10) != 0 ||
      spec_context_validate_combat_target(ch, victim, false) != SPEC_CONTEXT_VALID)
    return FALSE;
  if (rol_planar_restrain_agility_evades(GET_REAL_DEX(victim), rand_number(1, 100)))
  {
    if (kind == ROL_PLANAR_CONTROL_GLABREZU)
      act("You narrowly evade $n's massive pincers!", FALSE, ch, NULL, victim, TO_VICT);
    else
      act("You narrowly evade $n's coiling tail!", FALSE, ch, NULL, victim, TO_VICT);
    return TRUE;
  }
  if (!rol_planar_restrain_constitution_survives(GET_REAL_CON(victim), rand_number(1, 85)))
  {
    if (kind == ROL_PLANAR_CONTROL_GLABREZU)
      act("$n catches you in $s massive pincers and crushes the life from you!", FALSE, ch, NULL,
          victim, TO_VICT);
    else
      act("$n's tail coils around you and crushes the life from you!", FALSE, ch, NULL, victim,
          TO_VICT);
    act("$n crushes $N to death!", FALSE, ch, NULL, victim, TO_NOTVICT);
    if (victim == context->target)
      context->invalidation |= SPEC_INVALIDATE_TARGET;
    die(victim, ch);
    return TRUE;
  }

  if (kind == ROL_PLANAR_CONTROL_GLABREZU)
  {
    act("$n catches $N in $s massive pincers and holds $M helpless!", FALSE, ch, NULL, victim,
        TO_NOTVICT);
    act("$n catches you in $s massive pincers and holds you helpless!", FALSE, ch, NULL, victim,
        TO_VICT);
  }
  else
  {
    act("$n's tail coils around $N and holds $M helpless!", FALSE, ch, NULL, victim, TO_NOTVICT);
    act("$n's tail coils around you and holds you helpless!", FALSE, ch, NULL, victim, TO_VICT);
  }
  rol_monster_stop_combat(ch);
  rol_monster_stop_combat(victim);
  rol_planar_make_follower(victim, ch);
  ch->mob_specials.rol_planar_captive_active = true;
  return TRUE;
}

static int rol_planar_restrain_activity(struct char_data *ch)
{
  struct char_data *captive;

  if (!ch->mob_specials.rol_planar_captive_active)
    return FALSE;
  captive = rol_planar_first_captive(ch);
  if (captive != NULL && IN_ROOM(captive) == IN_ROOM(ch) && FIGHTING(ch) == NULL)
  {
    (void)set_fighting(ch, captive);
    if (FIGHTING(captive) == NULL)
      (void)set_fighting(captive, ch);
    return TRUE;
  }
  ch->mob_specials.rol_planar_captive_active = false;
  return FALSE;
}

static bool rol_planar_succubus_target(struct char_data *ch, struct char_data *victim)
{
  return victim != ch && victim != ch->master && !IS_NPC(victim) && GET_SEX(victim) == SEX_MALE &&
         GET_LEVEL(victim) < LVL_IMMORT && !AFF_FLAGGED(victim, AFF_CHARM) &&
         !AFF_FLAGGED(victim, AFF_MIND_BLANK) && !char_has_object_flag(victim, ITEM_ROL_NO_CHARM) &&
         !is_immune_charm(ch, victim, FALSE);
}

static bool rol_planar_succubus_apply_charm(struct char_data *ch, struct char_data *victim)
{
  if (mag_resistance(ch, victim, 0) ||
      savingthrow(ch, victim, SAVING_WILL, -2, CAST_INNATE, GET_LEVEL(ch), ENCHANTMENT))
  {
    send_to_char(victim, "You shake off a wave of beguiling desire.\r\n");
    return false;
  }
  act("$n gazes into your eyes, and your will melts away.", FALSE, ch, NULL, victim, TO_VICT);
  act("$n gazes into $N's eyes, captivating $M completely.", FALSE, ch, NULL, victim, TO_NOTVICT);
  rol_planar_make_follower(victim, ch);
  return true;
}

static int rol_planar_succubus_activity(struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next;
  struct char_data *captive;
  time_t now = time(NULL);
  bool attempted = false;

  if (!AWAKE(ch))
    return FALSE;
  if (FIGHTING(ch) != NULL)
  {
    if (ch->mob_specials.rol_planar_captive_kill_at > 0 &&
        ch->mob_specials.rol_planar_captive_kill_at <= now)
      ch->mob_specials.rol_planar_captive_kill_at = now + SECS_PER_MUD_HOUR;
    return FALSE;
  }
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_planar_succubus_target(ch, victim))
      continue;
    if (CLASS_LEVEL(victim, CLASS_BLACKGUARD) > 0 && GET_LEVEL(victim) > GET_LEVEL(ch) &&
        ch->master == NULL)
    {
      act("$n recognizes your dark authority and kneels to serve you.", FALSE, ch, NULL, victim,
          TO_VICT);
      rol_planar_make_follower(ch, victim);
      return TRUE;
    }
    if (!rol_planar_succubus_charm_roll_fires(rand_number(0, 3)))
      continue;
    attempted = true;
    if (rol_planar_succubus_apply_charm(ch, victim))
    {
      time_t deadline = now + rol_planar_succubus_kiss_delay_seconds(rand_number(1, 4));

      if (ch->mob_specials.rol_planar_captive_kill_at <= 0 ||
          deadline < ch->mob_specials.rol_planar_captive_kill_at)
        ch->mob_specials.rol_planar_captive_kill_at = deadline;
    }
    break;
  }
  if (ch->mob_specials.rol_planar_captive_kill_at <= 0 ||
      ch->mob_specials.rol_planar_captive_kill_at > now)
    return TRUE;
  if (attempted)
  {
    ch->mob_specials.rol_planar_captive_kill_at = now + SECS_PER_MUD_HOUR;
    return TRUE;
  }
  captive = rol_planar_first_captive(ch);
  if (captive == NULL || IN_ROOM(captive) != IN_ROOM(ch))
  {
    ch->mob_specials.rol_planar_captive_kill_at = 0;
    return TRUE;
  }
  act("$n draws $N into a final kiss and drains away $S life.", FALSE, ch, NULL, captive,
      TO_NOTVICT);
  act("$n draws you into a final kiss, and the world goes black.", FALSE, ch, NULL, captive,
      TO_VICT);
  die(captive, ch);
  ch->mob_specials.rol_planar_captive_kill_at =
      rol_planar_first_captive(ch) != NULL ? now + SECS_PER_MUD_HOUR : 0;
  return TRUE;
}

static int rol_planar_vrock_screech(struct char_data *ch, time_t now)
{
  struct char_data *victim;
  struct char_data *next;

  if (!rol_planar_cooldown_ready(ch->mob_specials.rol_planar_screech_ready_at, now) ||
      !rol_planar_screech_health_allows(GET_HIT(ch), GET_MAX_HIT(ch)) ||
      AFF_FLAGGED(ch, AFF_SILENCED) || ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF))
    return FALSE;

  act("$n abruptly makes a terrifying screeching noise!", FALSE, ch, NULL, NULL, TO_ROOM);
  send_to_char(ch, "You abruptly make a terrifying screeching noise!\r\n");
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_hit_area_target(ch, victim) || IS_NPC(victim) ||
        rand_number(1, 100) <= GET_CON(victim))
      continue;
    rol_monster_stun(victim, 1);
  }
  ch->mob_specials.rol_planar_screech_ready_at = now + SECS_PER_MUD_DAY;
  return TRUE;
}

static int rol_planar_vrock_spores(struct spec_event_context *context, struct char_data *ch,
                                   time_t now)
{
  struct char_data *victim = FIGHTING(ch);

  if (!rol_planar_cooldown_ready(ch->mob_specials.rol_planar_spore_ready_at, now) ||
      !rol_planar_five_in_six_roll_fires(rand_number(0, 5)) || victim == NULL ||
      spec_context_validate_combat_target(ch, victim, false) != SPEC_CONTEXT_VALID)
    return FALSE;

  act("$n raises $s wings and showers $N with a cloud of spores.", FALSE, ch, NULL, victim,
      TO_NOTVICT);
  act("You raise your wings and shower $N with a cloud of spores.", FALSE, ch, NULL, victim,
      TO_CHAR);
  act("$n raises $s wings and showers you with a cloud of spores.", FALSE, ch, NULL, victim,
      TO_VICT);
  ch->mob_specials.rol_planar_spore_ready_at = now + ROL_PLANAR_HIT_BURST_COOLDOWN;
  (void)rol_monster_successful_hit_damage(context, ch, victim, dice(10, 2), DAM_POISON);
  return TRUE;
}

static int rol_planar_vrock_start_dance(struct char_data *ch, time_t now);

static int rol_planar_vrock_bursts(struct spec_event_context *context, struct char_data *ch)
{
  time_t now = time(NULL);
  int result = FALSE;

  if (rol_planar_vrock_screech(ch, now))
    result = TRUE;
  if (rol_planar_vrock_spores(context, ch, now))
    result = TRUE;
  return result;
}

static int rol_planar_vrock_hit(struct spec_event_context *context, struct char_data *ch)
{
  int result;

  if (ch->mob_specials.rol_planar_dance_stage != 0)
    return TRUE;
  result = rol_planar_vrock_bursts(context, ch);
  if (rol_planar_vrock_start_dance(ch, time(NULL)))
    result = TRUE;
  return result;
}

static bool rol_planar_vrock_dance_eligible(struct char_data *vrock, time_t now)
{
  if (vrock == NULL || !IS_NPC(vrock) || !rol_planar_vrock_dance_profile(GET_MOB_VNUM(vrock)) ||
      FIGHTING(vrock) == NULL || vrock->mob_specials.rol_planar_dance_stage != 0)
    return false;
  if (rol_planar_cooldown_ready(vrock->mob_specials.rol_planar_dance_reset_at, now))
    vrock->mob_specials.rol_planar_dance_reset_at = 0;
  return vrock->mob_specials.rol_planar_dance_reset_at == 0;
}

static void rol_planar_vrock_clear_dance(room_rnum room, time_t reset_at)
{
  struct char_data *vrock;

  for (vrock = world[room].people; vrock != NULL; vrock = vrock->next_in_room)
  {
    if (!IS_NPC(vrock) || !rol_planar_vrock_dance_profile(GET_MOB_VNUM(vrock)) ||
        vrock->mob_specials.rol_planar_dance_stage == 0)
      continue;
    vrock->mob_specials.rol_planar_dance_stage = 0;
    vrock->mob_specials.rol_planar_dance_step_at = 0;
    vrock->mob_specials.rol_planar_dance_reset_at = reset_at;
    vrock->mob_specials.rol_planar_dance_leader = false;
  }
}

static int rol_planar_vrock_start_dance(struct char_data *ch, time_t now)
{
  struct char_data *vrock;
  int count = 0;

  if (!rol_planar_vrock_dance_eligible(ch, now))
    return FALSE;
  for (vrock = world[IN_ROOM(ch)].people; vrock != NULL; vrock = vrock->next_in_room)
  {
    if (rol_planar_vrock_dance_eligible(vrock, now))
      count++;
  }
  if (count < rol_planar_vrock_dance_required_count())
    return FALSE;
  act("Five or more Vrocks lock claws and begin a terrible dance!", FALSE, ch, NULL, NULL, TO_ROOM);
  send_to_char(ch, "You lock claws with the other Vrocks and begin the dance of ruin!\r\n");
  for (vrock = world[IN_ROOM(ch)].people; vrock != NULL; vrock = vrock->next_in_room)
  {
    if (!rol_planar_vrock_dance_eligible(vrock, now))
      continue;
    vrock->mob_specials.rol_planar_dance_stage = 1;
    vrock->mob_specials.rol_planar_dance_step_at = now + rol_planar_vrock_dance_step_seconds();
    vrock->mob_specials.rol_planar_dance_leader = vrock == ch;
  }
  return TRUE;
}

static bool rol_planar_vrock_has_dance_leader(struct char_data *ch)
{
  struct char_data *vrock;

  for (vrock = world[IN_ROOM(ch)].people; vrock != NULL; vrock = vrock->next_in_room)
  {
    if (IS_NPC(vrock) && rol_planar_vrock_dance_profile(GET_MOB_VNUM(vrock)) &&
        vrock->mob_specials.rol_planar_dance_stage != 0 &&
        vrock->mob_specials.rol_planar_dance_leader)
      return true;
  }
  return false;
}

static int rol_planar_vrock_dance_activity(struct spec_event_context *context, struct char_data *ch)
{
  struct char_data *vrock;
  struct char_data *victim;
  struct char_data *next;
  time_t now = time(NULL);
  int count = 0;
  int damage_amount;
  int victim_damage;
  byte next_stage;

  if (ch->mob_specials.rol_planar_dance_stage == 0)
    return FALSE;
  if (!ch->mob_specials.rol_planar_dance_leader)
  {
    if (rol_planar_vrock_has_dance_leader(ch))
      return TRUE;
    ch->mob_specials.rol_planar_dance_leader = true;
  }
  if (ch->mob_specials.rol_planar_dance_step_at > now)
    return TRUE;
  for (vrock = world[IN_ROOM(ch)].people; vrock != NULL; vrock = vrock->next_in_room)
  {
    if (IS_NPC(vrock) && rol_planar_vrock_dance_profile(GET_MOB_VNUM(vrock)) &&
        vrock->mob_specials.rol_planar_dance_stage != 0 && FIGHTING(vrock) != NULL)
      count++;
  }
  if (count < rol_planar_vrock_dance_required_count())
  {
    act("The Vrocks lose their rhythm, and the terrible dance collapses.", FALSE, ch, NULL, NULL,
        TO_ROOM);
    rol_planar_vrock_clear_dance(IN_ROOM(ch), 0);
    return TRUE;
  }

  if (ch->mob_specials.rol_planar_dance_stage == 1)
    act("The circling Vrocks chant faster as crackling energy gathers around them.", FALSE, ch,
        NULL, NULL, TO_ROOM);
  else if (ch->mob_specials.rol_planar_dance_stage == 2)
    act("The Vrocks' wild chant rises to a deafening, ruinous crescendo!", FALSE, ch, NULL, NULL,
        TO_ROOM);
  else
  {
    act("The Vrocks' dance erupts in a devastating blast of lightning!", FALSE, ch, NULL, NULL,
        TO_ROOM);
    damage_amount = dice(20, 10);
    for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
    {
      next = victim->next_in_room;
      if (IS_NPC(victim) || !rol_monster_hit_area_target(ch, victim))
        continue;
      victim_damage = damage_amount;
      if (savingthrow(ch, victim, SAVING_REFL, 0, CAST_INNATE, GET_LEVEL(ch), EVOCATION))
        victim_damage /= 2;
      (void)rol_monster_successful_hit_damage(context, ch, victim, victim_damage, DAM_ELECTRIC);
    }
    rol_planar_vrock_clear_dance(IN_ROOM(ch), now + rol_planar_vrock_dance_cooldown_seconds());
    return TRUE;
  }

  next_stage = ch->mob_specials.rol_planar_dance_stage + 1;
  for (vrock = world[IN_ROOM(ch)].people; vrock != NULL; vrock = vrock->next_in_room)
  {
    if (!IS_NPC(vrock) || !rol_planar_vrock_dance_profile(GET_MOB_VNUM(vrock)) ||
        vrock->mob_specials.rol_planar_dance_stage == 0)
      continue;
    vrock->mob_specials.rol_planar_dance_stage = next_stage;
    vrock->mob_specials.rol_planar_dance_step_at = now + rol_planar_vrock_dance_step_seconds();
  }
  return TRUE;
}

static int rol_planar_spinagon_spikes(struct spec_event_context *context, struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next;
  time_t now = time(NULL);
  int spikes;
  int save_bonus;

  if (FIGHTING(ch) == NULL ||
      !rol_planar_cooldown_ready(ch->mob_specials.rol_planar_spike_ready_at, now) ||
      !rol_planar_five_in_six_roll_fires(rand_number(0, 5)))
    return FALSE;

  send_to_char(ch, "You shoot several flaming spikes out of your tail!\r\n");
  act("$n shoots several flaming spikes out of $s tail!", FALSE, ch, NULL, NULL, TO_ROOM);
  spikes = rand_number(2, 5);
  for (victim = world[IN_ROOM(ch)].people; victim != NULL && spikes > 0; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_hit_area_target(ch, victim))
      continue;
    spikes--;
    save_bonus = AFF_FLAGGED(victim, AFF_ELEMENT_PROT) ? 4 : 0;
    if (savingthrow(ch, victim, SAVING_REFL, save_bonus, CAST_INNATE, GET_LEVEL(ch), EVOCATION))
    {
      act("$N ducks as one of the spikes sizzles past $S head!", FALSE, ch, NULL, victim,
          TO_NOTVICT);
      send_to_char(victim, "You duck as a spike goes sizzling past your head!\r\n");
      continue;
    }
    act("$N screams as one of the flaming spikes slams into $S chest!", FALSE, ch, NULL, victim,
        TO_NOTVICT);
    send_to_char(victim, "You scream as a flaming spike slams into your chest!\r\n");
    (void)rol_monster_successful_hit_damage(context, ch, victim, dice(20, 2), DAM_FIRE);
  }
  ch->mob_specials.rol_planar_spike_ready_at = now + ROL_PLANAR_HIT_BURST_COOLDOWN;
  return TRUE;
}

static int rol_avernus_barbazu_berserk(struct char_data *ch, struct char_data *attacker)
{
  struct affected_type affect;

  if (ch == NULL || attacker == NULL || FIGHTING(ch) == NULL ||
      affected_by_spell(ch, AFFECT_ROL_BARBAZU_BERSERK) ||
      !rol_avernus_barbazu_berserk_roll_fires(rand_number(0, 19)))
    return FALSE;

  new_affect(&affect);
  affect.spell = AFFECT_ROL_BARBAZU_BERSERK;
  affect.duration = 5;
  affect.modifier = GET_HITROLL(ch);
  affect.location = APPLY_HITROLL;
  affect_to_char(ch, &affect);

  new_affect(&affect);
  affect.spell = AFFECT_ROL_BARBAZU_BERSERK;
  affect.duration = 5;
  affect.modifier = GET_DAMROLL(ch);
  affect.location = APPLY_DAMROLL;
  affect_to_char(ch, &affect);
  GET_HIT(ch) += GET_MAX_HIT(ch) / 2;

  act("$N's hit causes $n's eyes to turn bloodshot and fill with rage!", TRUE, ch, NULL, attacker,
      TO_NOTVICT);
  act("Your hit causes $n's eyes to fill with rage!", TRUE, ch, NULL, attacker, TO_VICT);
  act("$N causes your blood to boil as you enter a bloodlust-filled rage!", TRUE, ch, NULL,
      attacker, TO_CHAR);
  return TRUE;
}

static int rol_avernus_gelugon_tail(struct char_data *ch, struct char_data *victim)
{
  struct affected_type affect;

  if (ch == NULL || victim == NULL || !rol_avernus_gelugon_tail_roll_fires(rand_number(0, 6)) ||
      paralysis_immunity(victim) ||
      savingthrow(ch, victim, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
    return FALSE;

  new_affect(&affect);
  affect.spell = SPELL_HOLD_MONSTER;
  affect.duration = rand_number(1, 2);
  SET_BIT_AR(affect.bitvector, AFF_PARALYZED);
  affect_to_char(victim, &affect);

  act("Your tail radiates freezing cold that freezes $N.", FALSE, ch, NULL, victim, TO_CHAR);
  act("$n's tail radiates freezing cold, causing your bones to lock up.", FALSE, ch, NULL, victim,
      TO_VICT);
  act("$n's tail radiates freezing cold, causing $N to slow to a complete halt.", FALSE, ch, NULL,
      victim, TO_NOTVICT);
  return TRUE;
}

static bool rol_avernus_meritos_target(const struct char_data *candidate)
{
  return candidate != NULL && !IS_NPC(candidate) && GET_LEVEL(candidate) < LVL_IMMORT &&
         !AFF_FLAGGED(candidate, AFF_SILENCED) &&
         (IS_WIZARD(candidate) || IS_CLERIC(candidate) || IS_BARD(candidate));
}

static int rol_avernus_meritos_silence(struct char_data *ch)
{
  struct affected_type affect;
  struct char_data *target;

  if (ch == NULL || !rol_avernus_meritos_silence_roll_fires(rand_number(0, 3)))
    return FALSE;
  for (target = world[IN_ROOM(ch)].people; target != NULL; target = target->next_in_room)
    if (rol_avernus_meritos_target(target))
      break;

  /* The source called its save routine before checking target and could dereference NULL. */
  if (target == NULL || !can_silence(target) || mag_resistance(ch, target, 0) ||
      savingthrow(ch, target, SAVING_WILL, 5, CAST_INNATE, GET_LEVEL(ch), ILLUSION))
    return FALSE;

  new_affect(&affect);
  affect.spell = SPELL_SILENCE;
  affect.duration = 4;
  SET_BIT_AR(affect.bitvector, AFF_SILENCED);
  affect_to_char(target, &affect);

  act("$n sends an icy bolt that worms its way into $N's mouth.", FALSE, ch, NULL, target,
      TO_NOTVICT);
  act("$n sends an icy bolt into your mouth, causing your tongue to freeze and go numb!", FALSE, ch,
      NULL, target, TO_VICT);
  act("An icy bolt leaves your hand and streaks toward $N's mouth.", FALSE, ch, NULL, target,
      TO_CHAR);
  return TRUE;
}

static void rol_monster_prismatic(struct char_data *ch, int level)
{
  if (FIGHTING(ch) == NULL)
    return;

  act("$n opens $s hands and releases a prismatic spray!", FALSE, ch, NULL, NULL, TO_ROOM);
  call_magic(ch, FIGHTING(ch), NULL, SPELL_PRISMATIC_SPRAY, 0, level, CAST_INNATE);
}

static void rol_monster_lycan(struct char_data *ch, struct char_data *victim, bool tiger)
{
  struct spec_damage_result result;

  if (tiger)
  {
    act("$n tears into you with massive claws and teeth!", FALSE, ch, NULL, victim, TO_VICT);
    act("$n tears into $N with massive claws and teeth!", FALSE, ch, NULL, victim, TO_NOTVICT);
    result = spec_damage_current_target(ch, victim, dice(35, 10), -1, DAM_SLASHING, FALSE);
  }
  else
  {
    act("$n leaps onto you in a flurry of claws and teeth!", FALSE, ch, NULL, victim, TO_VICT);
    act("$n leaps onto $N in a flurry of claws and teeth!", FALSE, ch, NULL, victim, TO_NOTVICT);
    result = spec_damage_current_target(ch, victim, dice(20, 7), -1, DAM_SLASHING, FALSE);
  }
  (void)result;
}

static void rol_monster_pit_fiend_bite(struct char_data *ch, struct char_data *victim)
{
  struct spec_damage_result result;

  act("$n savagely bites down on your arm!", FALSE, ch, NULL, victim, TO_VICT);
  act("$n savagely bites down on $N's arm!", FALSE, ch, NULL, victim, TO_NOTVICT);
  result = spec_damage_current_target(ch, victim, dice(2, 6), -1, DAM_PUNCTURE, FALSE);
  if (result.status != SPEC_DAMAGE_TARGET_INVALIDATED)
    call_magic(ch, victim, NULL, SPELL_POISON, 0, 6, CAST_INNATE);
}

static void rol_monster_transfer_possessions(struct char_data *source,
                                             struct char_data *destination, bool preserve_equipment)
{
  struct obj_data *item;
  struct obj_data *next_item;
  int wear;

  for (item = source->carrying; item != NULL; item = next_item)
  {
    next_item = item->next_content;
    obj_from_char(item);
    obj_to_char(item, destination);
  }
  for (wear = 0; wear < NUM_WEARS; wear++)
    if (GET_EQ(source, wear) != NULL)
    {
      item = unequip_char(source, wear);
      if (preserve_equipment)
        equip_char(destination, item, wear);
      else
        obj_to_char(item, destination);
    }
  GET_GOLD(destination) += GET_GOLD(source);
  GET_GOLD(source) = 0;
}

static int rol_monster_replace(struct spec_event_context *context, struct char_data *ch,
                               int replacement_vnum, struct char_data *victim)
{
  struct char_data *replacement;

  replacement = read_mobile(replacement_vnum, VIRTUAL);
  if (replacement == NULL)
  {
    log("SYSERR: RoL monster %d cannot load replacement %d", GET_MOB_VNUM(ch), replacement_vnum);
    return FALSE;
  }
  char_to_room(replacement, IN_ROOM(ch));
  GET_MOB_LOADROOM(replacement) = IN_ROOM(ch);
  rol_monster_transfer_possessions(ch, replacement, true);
  extract_char(ch);
  context->invalidation |= SPEC_INVALIDATE_OWNER | SPEC_INVALIDATE_ACTOR;
  if (victim != NULL && GET_POS(victim) > POS_DEAD && VALID_ROOM_RNUM(IN_ROOM(victim)) &&
      IN_ROOM(victim) == IN_ROOM(replacement))
    (void)set_fighting(replacement, victim);
  return TRUE;
}

static int rol_monster_xexos_activity(struct spec_event_context *context, struct char_data *ch,
                                      enum rol_monster_combat_effect effect)
{
  struct char_data *victim = FIGHTING(ch);

  if (!AWAKE(ch))
    return FALSE;

  if (effect == ROL_MONSTER_XEXOS)
  {
    if (victim == NULL)
      return FALSE;
    do_say(ch, "That was NOT a good idea!", 0, 0);
    act("$n pulls a vial from a hidden pocket and quickly quaffs it.", TRUE, ch, NULL, NULL,
        TO_ROOM);
    act("Flesh rends and tears, reshaping $n into something monstrous!", TRUE, ch, NULL, NULL,
        TO_ROOM);
    return rol_monster_replace(context, ch, 2012026, victim);
  }

  if (victim != NULL || MEMORY(ch) != NULL)
    return FALSE;
  act("$n chuffs angrily, then reverts to $s normal form.", TRUE, ch, NULL, NULL, TO_ROOM);
  return rol_monster_replace(context, ch, 2012025, NULL);
}

static int rol_monster_piercer_activity(struct char_data *ch)
{
  struct char_data *victim;
  int amount;

  if (GET_MAX_HIT(ch) != 1)
  {
    GET_MAX_HIT(ch) = 1;
    GET_HIT(ch) = 1;
    GET_REAL_HITROLL(ch) = 100;
    GET_HITROLL(ch) = 100;
  }
  if (FIGHTING(ch) != NULL || !AFF_FLAGGED(ch, AFF_HIDE) || GET_POS(ch) < POS_STANDING)
    return FALSE;

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = victim->next_in_room)
  {
    if (IS_NPC(victim) || victim == ch || GET_LEVEL(victim) >= LVL_IMMORT ||
        AFF_FLAGGED(victim, AFF_HIDE) || !CAN_SEE(ch, victim))
      continue;
    if (AFF_FLAGGED(victim, AFF_AWARE) &&
        savingthrow(ch, victim, SAVING_REFL, -10, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
    {
      act("$n crashes to the ground, missing you by a hair!", FALSE, ch, NULL, victim, TO_VICT);
      act("$n crashes to the ground, missing $N by a hair!", FALSE, ch, NULL, victim, TO_NOTVICT);
    }
    else
    {
      amount = rand_number(MAX(1, GET_LEVEL(ch) * 5), MAX(1, GET_LEVEL(ch) * 15));
      (void)damage(ch, victim, amount, -1, DAM_PUNCTURE, FALSE);
    }
    REMOVE_BIT_AR(MOB_FLAGS(ch), MOB_SPEC);
    return TRUE;
  }
  return FALSE;
}

static int rol_monster_kobold_priest_activity(struct char_data *ch)
{
  struct char_data *candidate;
  struct char_data *imp;
  int imp_count = 0;

  if (ch->mob_specials.proc_fired > 0)
  {
    ch->mob_specials.proc_fired--;
    return FALSE;
  }
  ch->mob_specials.proc_fired = 4;
  if (FIGHTING(ch) == NULL)
    return FALSE;

  for (candidate = character_list; candidate != NULL; candidate = candidate->next)
    if (IS_NPC(candidate) && GET_MOB_VNUM(candidate) == 2001440)
      imp_count++;
  if (imp_count >= 5)
    return FALSE;
  if (rand_number(1, 100) >= 90)
  {
    act("$n curses as $s summoning fails.", TRUE, ch, NULL, NULL, TO_ROOM);
    return FALSE;
  }
  if ((imp = read_mobile(2001440, VIRTUAL)) == NULL)
  {
    log("SYSERR: RoL kobold priest cannot load imp 2001440");
    return FALSE;
  }
  char_to_room(imp, IN_ROOM(ch));
  GET_MOB_LOADROOM(imp) = IN_ROOM(ch);
  act("$n incants a powerful spell of summoning, and $N arrives to aid $m!", FALSE, ch, NULL, imp,
      TO_ROOM);
  if (FIGHTING(ch) != NULL)
    (void)set_fighting(imp, FIGHTING(ch));
  return TRUE;
}

static int rol_monster_phalanx_activity(struct char_data *ch)
{
  room_rnum origin;
  int roll;

  if (FIGHTING(ch) != NULL)
  {
    if (GET_HIT(ch) * 4 >= GET_MAX_HIT(ch) || GET_ROOM_VNUM(IN_ROOM(ch)) == 2012144)
      return FALSE;
    origin = IN_ROOM(ch);
    if (!perform_move(ch, UP, 0) || IN_ROOM(ch) == origin)
      return FALSE;
    GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) * 2);
    act("$n retreats upward and rapidly repairs $s damaged frame.", TRUE, ch, NULL, NULL, TO_ROOM);
    return TRUE;
  }

  if (GET_REAL_AC(ch) == -50)
  {
    act("$n forms a new configuration.", TRUE, ch, NULL, NULL, TO_ROOM);
    GET_REAL_AC(ch) = -100;
    ch->points.armor = -100;
    return TRUE;
  }
  roll = dice(3, 7);
  if (roll == 20)
  {
    act("$n splits apart to reorganize.", TRUE, ch, NULL, NULL, TO_ROOM);
    GET_REAL_AC(ch) = -50;
    ch->points.armor = -50;
  }
  else if (roll == 19)
    act("$n makes some crackling noises.", FALSE, ch, NULL, NULL, TO_ROOM);
  else if (roll == 7)
    act("A spark emanates from the interior of $n.", FALSE, ch, NULL, NULL, TO_ROOM);
  return FALSE;
}

static bool rol_monster_summon_helper(struct char_data *ch, int mobile_vnum,
                                      struct char_data *victim, const char *message)
{
  struct char_data *helper;

  if ((helper = read_mobile(mobile_vnum, VIRTUAL)) == NULL)
  {
    log("SYSERR: RoL monster %d cannot load helper %d", GET_MOB_VNUM(ch), mobile_vnum);
    return false;
  }
  char_to_room(helper, IN_ROOM(ch));
  GET_MOB_LOADROOM(helper) = IN_ROOM(ch);
  if (message != NULL)
    act(message, FALSE, ch, NULL, helper, TO_ROOM);
  if (victim != NULL && GET_POS(victim) > POS_DEAD && IN_ROOM(victim) == IN_ROOM(ch))
    (void)set_fighting(helper, victim);
  load_mtrigger(helper);
  return true;
}

static void rol_monster_tree_spirit(struct char_data *ch)
{
  struct char_data *victim = rol_monster_random_player(ch);
  int helper_vnum;
  int index;

  if (victim == NULL)
    return;
  if (savingthrow(ch, victim, SAVING_REFL, -5, CAST_INNATE, GET_LEVEL(ch), TRANSMUTATION))
    act("A root tendril bursts through the wall, but you leap out of its path!", FALSE, ch, NULL,
        victim, TO_VICT);
  else
  {
    act("A root tendril wraps around you and squeezes with crushing force!", FALSE, ch, NULL,
        victim, TO_VICT);
    call_magic(ch, victim, NULL, SPELL_ENTANGLE, 0, GET_LEVEL(ch), CAST_INNATE);
    rol_monster_stun(victim, 1);
  }

  if (rand_number(0, 1) != 0 || FIGHTING(ch) == NULL)
    return;
  act("$n bellows, 'My children, come and protect me!' Three roots burst into the room.", FALSE, ch,
      NULL, NULL, TO_ROOM);
  for (index = 0; index < 3; index++)
  {
    helper_vnum = rand_number(2014023, 2014025);
    rol_monster_summon_helper(ch, helper_vnum, FIGHTING(ch), NULL);
  }
}

static void rol_monster_dranum(struct char_data *ch, struct char_data *victim)
{
  struct char_data *target;
  struct char_data *next;
  int drained;

  if (rand_number(0, 8) == 0)
  {
    act("$n tears open a dark abyss in $s spectral torso, draining the room's life force!", TRUE,
        ch, NULL, NULL, TO_ROOM);
    for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
    {
      next = target->next_in_room;
      if (!rol_monster_room_target(ch, target))
        continue;
      drained = MIN(200, MAX(0, GET_HIT(target)));
      GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + drained);
      (void)damage(ch, target, drained, -1, DAM_NEGATIVE, FALSE);
      if (GET_POS(target) > POS_DEAD &&
          !savingthrow(ch, target, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), NECROMANCY))
        rol_monster_stun(target, 1);
    }
    return;
  }

  act("$n passes a spectral hand through you and drains your life force!", FALSE, ch, NULL, victim,
      TO_VICT);
  GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 500);
  (void)damage(ch, victim, 250, -1, DAM_NEGATIVE, FALSE);
}

static void rol_monster_movanic_deva(struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next;
  int amount;
  bool saved;

  if (rand_number(0, 10) == 0)
  {
    act("Holy light descends on $n and closes $s wounds.", TRUE, ch, NULL, NULL, TO_ROOM);
    GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 1000);
  }
  if (rand_number(0, 5) != 0)
    return;

  act("$n beats $s wings and sends a massive blast of wind through the room!", TRUE, ch, NULL, NULL,
      TO_ROOM);
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_room_target(ch, victim))
      continue;
    amount = IS_EVIL(victim) ? rand_number(100, 200) : rand_number(50, 100);
    saved = savingthrow(ch, victim, SAVING_REFL, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL);
    if (damage(ch, victim, amount, -1, DAM_AIR, FALSE) < 0)
      continue;
    if (!saved)
      rol_monster_stun(victim, 1);
  }
}

static void rol_monster_canthus(struct char_data *ch, struct char_data *victim)
{
  int helper_vnum;
  int index;
  int spell;

  if (rand_number(0, 7) == 0)
  {
    act("$n raises $s head in a blood-curdling howl as the pack rushes to help!", FALSE, ch, NULL,
        NULL, TO_ROOM);
    for (index = 0; index < 5; index++)
    {
      helper_vnum = rand_number(0, 1) == 0 ? 2051311 : 2051361;
      rol_monster_summon_helper(ch, helper_vnum, victim, NULL);
    }
  }

  ch->mob_specials.proc_fired = (ch->mob_specials.proc_fired + 1) % 4;
  if (ch->mob_specials.proc_fired != 0)
    return;
  switch (rand_number(1, 3))
  {
  case 1:
    spell = SPELL_FIRE_BREATHE;
    break;
  case 2:
    spell = SPELL_FROST_BREATHE;
    break;
  default:
    spell = SPELL_LIGHTNING_BREATHE;
    break;
  }
  act("Natural energy gathers at $n's jaws before erupting across the room!", TRUE, ch, NULL, NULL,
      TO_ROOM);
  call_magic(ch, NULL, NULL, spell, 0, GET_LEVEL(ch), CAST_INNATE);
}

static void rol_monster_jotun_thrym(struct char_data *ch, struct char_data *victim)
{
  if (savingthrow(ch, victim, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), EVOCATION))
  {
    act("A blue bolt streaks from $n's hands, but you dodge the beam!", FALSE, ch, NULL, victim,
        TO_VICT);
    return;
  }
  act("A blue bolt streaks from $n's hands and encases you in solid ice!", FALSE, ch, NULL, victim,
      TO_VICT);
  rol_monster_stun(victim, rand_number(4, 5));
}

static void rol_monster_jotun_loki(struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next;

  act("$n calls forth visions of immense horror!", FALSE, ch, NULL, NULL, TO_ROOM);
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_room_target(ch, victim) || GET_RACE(victim) == RACE_TYPE_GIANT)
      continue;
    call_magic(ch, victim, NULL, SPELL_FEAR, 0, GET_LEVEL(ch), CAST_INNATE);
  }
}

static void rol_monster_residual_summon(const struct rol_monster_combat_profile *profile,
                                        struct char_data *ch)
{
  struct char_data *victim = FIGHTING(ch);
  const char *message;
  int helper_vnum;
  int summon_limit;

  if (victim == NULL || !rol_monster_fires(profile))
    return;
  switch (profile->effect)
  {
  case ROL_MONSTER_SUMMON_JESSICA_WISP:
    helper_vnum = 2026261;
    summon_limit = 5;
    message = "$n calls for aid, and $N appears in a shimmer of moonlight.";
    break;
  case ROL_MONSTER_SUMMON_ROBYN_WISP:
    helper_vnum = 2026263;
    summon_limit = 5;
    message = "$n calls for aid, and $N coalesces from a pale wisp.";
    break;
  case ROL_MONSTER_SUMMON_ROBYN_SERVANT:
    helper_vnum = 2026262;
    summon_limit = 3;
    message = "$n calls for aid, and $N rushes into the fight.";
    break;
  default:
    return;
  }
  if (ch->mob_specials.proc_fired >= summon_limit)
    return;
  if (rol_monster_summon_helper(ch, helper_vnum, victim, message))
    ch->mob_specials.proc_fired++;
}

static void rol_monster_jurtrem(struct char_data *ch)
{
  struct char_data *victim = FIGHTING(ch);

  if (victim == NULL || rand_number(1, 20) != 1)
    return;
  act("$n fixes $N with a consuming stare and tears away $S protective aura!", TRUE, ch, NULL,
      victim, TO_NOTVICT);
  act("$n's consuming stare tears away your protective aura!", FALSE, ch, NULL, victim, TO_VICT);
  affect_from_char(victim, SPELL_SANCTUARY);
  REMOVE_BIT_AR(AFF_FLAGS(victim), AFF_SANCTUARY);
}

static void rol_monster_kamerynn(struct char_data *ch, struct char_data *victim)
{
  act("$n strikes you with explosive force and folds space around you!", FALSE, ch, NULL, victim,
      TO_VICT);
  act("$n strikes $N with explosive force, and space buckles around $M!", FALSE, ch, NULL, victim,
      TO_NOTVICT);
  if (damage(ch, victim, 150, -1, DAM_FORCE, FALSE) < 0)
    return;
  call_magic(ch, victim, NULL, SPELL_TELEPORT, 0, GET_LEVEL(ch), CAST_INNATE);
}

static void rol_monster_crimson_fury(struct char_data *ch)
{
  struct char_data *target;
  struct char_data *next;

  ch->mob_specials.proc_fired = (ch->mob_specials.proc_fired + 1) % 4;
  if (ch->mob_specials.proc_fired == 0)
  {
    for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
    {
      next = target->next_in_room;
      if (target != ch && IS_NPC(target) && GET_MOB_VNUM(target) == 2003050)
      {
        act("$n is consumed by the Crimson Fury's rage!", TRUE, target, NULL, NULL, TO_ROOM);
        extract_char(target);
      }
    }
  }
  if (rand_number(1, 11) != 1)
    return;

  act("$n erupts in a room-filling blast of crimson fire!", TRUE, ch, NULL, NULL, TO_ROOM);
  for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (target == ch || IS_NPC(target) || GET_LEVEL(target) >= LVL_IMMORT ||
        GET_POS(target) <= POS_DEAD)
      continue;
    (void)damage(ch, target, rand_number(300, 600), -1, DAM_FIRE, FALSE);
  }
}

static struct char_data *rol_monster_first_visible_player(struct char_data *ch)
{
  struct char_data *victim;

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = victim->next_in_room)
    if (rol_monster_room_target(ch, victim) && CAN_SEE(ch, victim))
      return victim;
  return NULL;
}

static void rol_monster_spiritist_curse(struct char_data *ch, struct char_data *victim)
{
  struct affected_type af;
  int location;

  act("$n invokes a spiteful spirit that settles over you like a crushing weight!", FALSE, ch, NULL,
      victim, TO_VICT);
  for (location = APPLY_HITROLL; location <= APPLY_DAMROLL; location++)
  {
    new_affect(&af);
    af.spell = SPELL_CURSE;
    af.duration = 5;
    af.location = location;
    af.modifier = -3;
    affect_to_char(victim, &af);
  }
}

static void rol_monster_spiritist(struct char_data *ch)
{
  struct char_data *victim;
  struct obj_data *weapon;
  int slot;

  if (!AWAKE(ch))
    return;
  victim = rol_monster_first_visible_player(ch);
  if (victim == NULL)
    return;
  switch (rand_number(0, 5))
  {
  case 0:
    act("An unseen spirit circles $n and whispers hungrily.", TRUE, ch, NULL, NULL, TO_ROOM);
    break;
  case 1:
    rol_monster_spiritist_curse(ch, victim);
    break;
  case 2:
    slot = GET_EQ(victim, WEAR_WIELD_1) != NULL ? WEAR_WIELD_1 : WEAR_WIELD_OFFHAND;
    weapon = GET_EQ(victim, slot);
    if (weapon != NULL)
    {
      act("An unseen spirit wrenches $p from your grasp!", FALSE, victim, weapon, NULL, TO_CHAR);
      weapon = unequip_char(victim, slot);
      SET_OBJ_FLAG(weapon, ITEM_HIDDEN);
      obj_to_room(weapon, IN_ROOM(victim));
    }
    break;
  case 3:
    act("$n calls a raging cyclone down upon $N!", TRUE, ch, NULL, victim, TO_NOTVICT);
    call_magic(ch, victim, NULL, SPELL_WHIRLWIND, 0, GET_LEVEL(ch), CAST_INNATE);
    break;
  default:
    break;
  }
}

static void rol_monster_tako_activity(struct char_data *ch)
{
  struct char_data *victim;
  room_rnum approach;
  room_rnum pit;

  approach = real_room(2001484);
  pit = real_room(2001485);
  if (FIGHTING(ch) != NULL || !VALID_ROOM_RNUM(approach) || !VALID_ROOM_RNUM(pit) ||
      IN_ROOM(ch) != pit)
    return;
  for (victim = world[approach].people; victim != NULL; victim = victim->next_in_room)
  {
    if (victim == ch || IS_NPC(victim) || GET_LEVEL(victim) >= LVL_IMMORT ||
        FIGHTING(victim) != NULL || !CAN_SEE(ch, victim))
      continue;
    if (rand_number(0, 100) >= 30)
      return;
    if (!valid_mortal_tele_dest(victim, pit, false))
      return;
    act("$n seizes you and hurls you into the pit!", FALSE, ch, NULL, victim, TO_VICT);
    act("$n seizes $N and hurls $M into the pit!", FALSE, ch, NULL, victim, TO_NOTVICT);
    char_from_room(victim);
    char_to_room(victim, pit);
    look_at_room(victim, 0);
    (void)set_fighting(ch, victim);
    return;
  }
}

static void rol_monster_werewolf_activity(struct char_data *ch)
{
  static const char *socials[] = {"growl", "scratch", "bark", "pant", "roar", "drool", "moan"};
  struct char_data *victim;
  int command;

  if (FIGHTING(ch) == NULL && GET_HIT(ch) > 50 && rand_number(1, 20) == 1)
  {
    for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = victim->next_in_room)
    {
      if (victim == ch || !IS_NPC(victim) || IS_PET(victim) || FIGHTING(victim) != NULL ||
          GET_POS(victim) <= POS_DEAD || !CAN_SEE(ch, victim))
        continue;
      (void)set_fighting(ch, victim);
      return;
    }
  }
  if (rand_number(1, 8) != 1)
    return;
  command = find_command(socials[rand_number(0, 6)]);
  if (command >= 0)
    do_action(ch, "", command, 0);
}

static void rol_monster_jotun_mimer_activity(struct char_data *ch)
{
  room_rnum home = GET_MOB_LOADROOM(ch);

  if (!AWAKE(ch) || FIGHTING(ch) != NULL || !VALID_ROOM_RNUM(home) || IN_ROOM(ch) == home)
    return;
  act("$n disappears in a puff of smoke.", TRUE, ch, NULL, NULL, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, home);
  act("$n appears in a puff of smoke and resumes $s post.", TRUE, ch, NULL, NULL, TO_ROOM);
}

static bool rol_seelie_prismatic_apply_beam(struct char_data *ch, struct char_data *target,
                                            int color)
{
  static const char *color_names[] = {"red",    "orange", "yellow", "blue",
                                      "indigo", "green",  "violet", "azure"};
  int amount;
  int spell;

  if (!rol_monster_room_target(ch, target) || color < 0 ||
      color >= (int)(sizeof(color_names) / sizeof(color_names[0])))
    return false;
  send_to_char(target, "A %s beam from the prismatic rainbow strikes you!\r\n", color_names[color]);
  act("A beam from the prismatic rainbow strikes $n!", FALSE, target, NULL, NULL, TO_ROOM);

  amount = rol_seelie_prismatic_damage(color);
  if (amount > 0)
  {
    if (savingthrow(ch, target, SAVING_WILL, 0, CAST_INNATE, GET_LEVEL(ch), ILLUSION))
      amount /= 2;
    return damage(ch, target, amount, SPELL_PRISMATIC_SPRAY, DAM_ILLUSION, FALSE) >= 0;
  }

  switch (color)
  {
  case 3:
    spell = SPELL_HOLD_MONSTER;
    break;
  case 4:
    spell = SPELL_FEEBLEMIND;
    break;
  case 5:
    spell = SPELL_POISON;
    break;
  case 6:
    spell = SPELL_DISPEL_MAGIC;
    break;
  case 7:
    spell = SPELL_BLINDNESS;
    break;
  default:
    return true;
  }
  call_magic(ch, target, NULL, spell, 0, GET_LEVEL(ch), CAST_INNATE);
  return rol_monster_room_target(ch, target);
}

static int rol_seelie_prismatic_activity(struct char_data *ch)
{
  struct char_data *next;
  struct char_data *target;
  struct char_data *victim;
  int beams;
  int color;
  int index;
  int previous_color;

  victim = FIGHTING(ch);
  if (victim == NULL || IS_CASTING(ch) || rand_number(0, 2) != 0)
    return FALSE;

  if (GET_POS(ch) < POS_STANDING || AFF_FLAGGED(ch, AFF_STUN) ||
      char_has_mud_event(ch, eSTUNNED) != NULL)
  {
    if (rand_number(0, 100) >= 22)
    {
      send_to_char(ch, "You are too stunned to gather your strength.\r\n");
      act("$n tries to gather $s strength, but fails.", FALSE, ch, NULL, NULL, TO_ROOM);
      return FALSE;
    }
    change_position(ch, POS_STANDING);
    send_to_char(ch, "You gather your strength and flutter high into the air.\r\n");
    act("$n gathers $s strength and flutters high into the air.", FALSE, ch, NULL, NULL, TO_ROOM);
  }

  send_to_char(ch, "A rainbow of colors forms as your wings beat wildly.\r\n");
  act("A rainbow of colors forms as $n's wings beat wildly.", FALSE, ch, NULL, NULL, TO_ROOM);
  for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (!rol_monster_room_target(ch, target))
      continue;
    beams = rol_seelie_prismatic_beam_count(rand_number(2, 5));
    previous_color = -1;
    for (index = 0; index < beams; index++)
    {
      do
      {
        color = rand_number(0, 7);
      } while (color == previous_color);
      previous_color = color;
      if (!rol_seelie_prismatic_apply_beam(ch, target, color))
        break;
    }
  }
  return FALSE;
}

static int rol_seelie_faerie_fire_activity(struct char_data *ch)
{
  struct affected_type af;
  struct char_data *target;
  struct char_data *next;
  bool was_hidden;

  if (FIGHTING(ch) == NULL || IS_CASTING(ch) ||
      char_has_mud_event(ch, eROL_SEELIE_FAERIE_FIRE) != NULL || rand_number(0, 5) != 0)
    return FALSE;

  send_to_char(ch, "You utter a magical word, and the area glows with purplish light.\r\n");
  act("$n utters a magical word, and the area glows with purplish light.", FALSE, ch, NULL, NULL,
      TO_ROOM);
  attach_mud_event(new_mud_event(eROL_SEELIE_FAERIE_FIRE, ch, NULL), (3 * SECS_PER_MUD_DAY) RL_SEC);

  for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (!rol_monster_room_target(ch, target) || affected_by_spell(target, SPELL_FAERIE_FIRE))
      continue;

    was_hidden = AFF_FLAGGED(target, AFF_HIDE);
    if (AFF_FLAGGED(target, AFF_INVISIBLE))
      appear(target, true);
    if (AFF_FLAGGED(target, AFF_HIDE))
      REMOVE_BIT_AR(AFF_FLAGS(target), AFF_HIDE);
    if (was_hidden)
      send_to_char(target, "The purplish light ruins your hiding place.\r\n");

    new_affect(&af);
    af.spell = SPELL_FAERIE_FIRE;
    af.duration = 3;
    af.location = APPLY_AC_NEW;
    af.modifier = -2;
    SET_BIT_AR(af.bitvector, AFF_FAERIE_FIRE);
    affect_to_char(target, &af);
    send_to_char(target, "You are surrounded by an outline of purplish flames!\r\n");
    act("$n is surrounded by an outline of purplish flames!", TRUE, target, NULL, NULL, TO_ROOM);
  }
  return TRUE;
}

static int rol_seelie_search_activity(struct char_data *ch)
{
  struct char_data *target;
  int rounds;

  if (IS_CASTING(ch))
    return FALSE;
  for (target = world[IN_ROOM(ch)].people; target != NULL; target = target->next_in_room)
  {
    if (!rol_monster_room_target(ch, target))
      continue;
    if (AFF_FLAGGED(target, AFF_HIDE))
    {
      act("Sparkling lights reveal $n's hiding spot as a faerie knocks $m to the ground!", FALSE,
          target, NULL, NULL, TO_ROOM);
      send_to_char(target, "Sparkling lights reveal your hiding spot as a faerie knocks you to the "
                           "ground!\r\n");
      REMOVE_BIT_AR(AFF_FLAGS(target), AFF_HIDE);
      change_position(target, POS_RECLINING);
      resetCastingData(target);
      rounds = rol_seelie_search_stun_rounds(GET_MOB_VNUM(ch));
      if (rounds > 0 && can_stun(target) && char_has_mud_event(target, eSTUNNED) == NULL)
        attach_mud_event(new_mud_event(eSTUNNED, target, NULL), PULSE_VIOLENCE * rounds);
    }
    /* The source procedure consumes its event after the first eligible target. */
    return TRUE;
  }
  return FALSE;
}

static int rol_seelie_faerie_activity(struct char_data *ch)
{
  const struct rol_seelie_faerie_profile *profile;

  profile = rol_seelie_faerie_profile_for(GET_MOB_VNUM(ch));
  if (profile == NULL)
    return FALSE;
  if ((profile->abilities & ROL_SEELIE_PRISMATIC) != 0)
    (void)rol_seelie_prismatic_activity(ch);
  if ((profile->abilities & ROL_SEELIE_FAERIE_FIRE) != 0 && rol_seelie_faerie_fire_activity(ch))
    return TRUE;
  if ((profile->abilities & ROL_SEELIE_SEARCH) != 0)
    return rol_seelie_search_activity(ch);
  return FALSE;
}

static void rol_monster_swallow(struct spec_event_context *context, struct char_data *ch,
                                struct char_data *victim, bool spits)
{
  bool can_swallow;
  int swallow_sides = spits ? 6 : 10;

  can_swallow = CAN_SEE(ch, victim) && (spits || (GET_WEIGHT(ch) >= 20 * GET_WEIGHT(victim) &&
                                                  GET_HEIGHT(ch) >= 20 * GET_HEIGHT(victim)));
  if (can_swallow && rand_number(1, swallow_sides) == 1)
  {
    act("$n opens $s enormous maw and swallows you whole!", FALSE, ch, NULL, victim, TO_VICT);
    act("$n opens $s enormous maw and swallows $N whole!", FALSE, ch, NULL, victim, TO_NOTVICT);
    rol_monster_transfer_possessions(victim, ch, false);
    if (spits)
    {
      act("$n gurgles and spits you back out, disgusted by your smell!", FALSE, ch, NULL, victim,
          TO_VICT);
      rol_monster_stop_combat(victim);
    }
    else
    {
      die(victim, ch);
      context->invalidation |= SPEC_INVALIDATE_TARGET;
    }
    return;
  }
  if (!CAN_SEE(ch, victim) || rand_number(1, 10) != 1)
    return;
  act("$n's enormous maw darts out and razor-sharp teeth tear into you!", FALSE, ch, NULL, victim,
      TO_VICT);
  (void)damage(ch, victim, spits ? 100 : 200, -1, DAM_PUNCTURE, FALSE);
}

static void rol_monster_purple_worm(struct spec_event_context *context, struct char_data *ch,
                                    struct char_data *victim)
{
  if (rand_number(0, 4) == 0)
    return;
  act("$n opens $s gaping maw and swallows you whole!", FALSE, ch, NULL, victim, TO_VICT);
  act("$n swallows $N whole and looks refreshed!", FALSE, ch, NULL, victim, TO_NOTVICT);
  if (!AFF_FLAGGED(ch, AFF_BLACKMANTLE))
    GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 500);
  die(victim, ch);
  context->invalidation |= SPEC_INVALIDATE_TARGET;
}

static void rol_monster_pit_fiend_tail(struct spec_event_context *context, struct char_data *ch,
                                       struct char_data *victim)
{
  /* The source captive/charm state is unsafe in the target follower model.  A
   * bounded stun preserves the successful restraint without changing ownership. */
  if (rand_number(1, 11) != 1)
    return;
  if (savingthrow(ch, victim, SAVING_REFL, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
  {
    act("$n attempts to coil $s tail around you, but you evade it!", FALSE, ch, NULL, victim,
        TO_VICT);
    return;
  }
  if (!savingthrow(ch, victim, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
  {
    act("$n coils $s tail around you and crushes you into a bloody pulp!", FALSE, ch, NULL, victim,
        TO_VICT);
    die(victim, ch);
    context->invalidation |= SPEC_INVALIDATE_TARGET;
    return;
  }
  act("$n coils $s tail around you, completely restricting your movement!", FALSE, ch, NULL, victim,
      TO_VICT);
  rol_monster_stun(victim, 2);
}

static void rol_monster_spider_venom(struct char_data *ch)
{
  struct char_data *victim = rol_monster_random_player(ch);

  if (victim == NULL)
    return;
  act("$n rears up and sinks $s fangs deep into you!", FALSE, ch, NULL, victim, TO_VICT);
  act("$n rears up and sinks $s fangs deep into $N!", FALSE, ch, NULL, victim, TO_NOTVICT);
  call_magic(ch, victim, NULL, SPELL_POISON, 0, 4, CAST_INNATE);
}

static void rol_monster_plant_poison(struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next;
  bool announced = false;

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_room_target(ch, victim) || IS_NPC(victim) || rand_number(1, 3) != 1 ||
        savingthrow(ch, victim, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
      continue;
    if (!announced)
    {
      act("$n launches a volley of barbed red thorns!", TRUE, ch, NULL, NULL, TO_ROOM);
      announced = true;
    }
    act("One of $n's thorns pierces your skin!", FALSE, ch, NULL, victim, TO_VICT);
    call_magic(ch, victim, NULL, SPELL_POISON, 0, GET_LEVEL(ch), CAST_INNATE);
  }
}

static void rol_monster_banshee_wail(struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next;
  int amount;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF) || AFF_FLAGGED(ch, AFF_SILENCED))
    return;
  act("$n's wail chills you to the bone!", FALSE, ch, NULL, NULL, TO_ROOM);
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_room_target(ch, victim) || IS_NPC(victim))
      continue;
    amount = rand_number(190, 210);
    if (savingthrow(ch, victim, SAVING_WILL, 0, CAST_INNATE, GET_LEVEL(ch), NECROMANCY))
      amount /= 2;
    (void)damage(ch, victim, amount, -1, DAM_SOUND, FALSE);
  }
}

static void rol_monster_shockwave(struct char_data *ch, int save_type)
{
  struct char_data *victim;
  struct char_data *next;

  act("$n crashes forward and sends a crushing shockwave through the room!", TRUE, ch, NULL, NULL,
      TO_ROOM);
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_room_target(ch, victim) ||
        savingthrow(ch, victim, save_type, 0, CAST_INNATE, GET_LEVEL(ch), NOSCHOOL))
      continue;
    act("The shockwave sends you crashing to the ground!", FALSE, ch, NULL, victim, TO_VICT);
    act("The shockwave sends $N crashing to the ground!", FALSE, ch, NULL, victim, TO_NOTVICT);
    rol_monster_stun(victim, rand_number(1, 2));
  }
}

static void rol_monster_four_arms(struct char_data *ch, struct char_data *victim)
{
  if (rand_number(1, 16) == 1)
    rol_monster_shockwave(ch, SAVING_FORT);
  if (FIGHTING(ch) == victim && GET_POS(victim) > POS_DEAD)
    (void)hit(ch, victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);
}

static void rol_monster_rot_bringer(struct char_data *ch, struct char_data *victim)
{
  struct char_data *helper;

  if (PROC_FIRED(ch) || GET_HIT(ch) * 100 >= GET_MAX_HIT(ch) * 40)
    return;
  if ((helper = read_mobile(2045193, VIRTUAL)) == NULL)
  {
    log("SYSERR: RoL Rot Bringer helper 2045193 is unavailable");
    return;
  }

  PROC_FIRED(ch) = TRUE;
  act("$n claws at a bloody basin as a massive ball of flesh rises to defend $m!", FALSE, ch, NULL,
      NULL, TO_ROOM);
  char_to_room(helper, IN_ROOM(ch));
  GET_MOB_LOADROOM(helper) = IN_ROOM(ch);
  add_follower(helper, ch);
  (void)set_fighting(helper, victim);
}

static void rol_monster_ashentoris(struct char_data *ch, struct char_data *victim)
{
  struct char_data *target;
  struct char_data *next;
  struct spec_damage_result result;

  act("A blazing beam of black energy drains $N as lava erupts through the room!", TRUE, ch, NULL,
      victim, TO_ROOM);
  result = spec_damage_current_target(ch, victim, 200, -1, DAM_NEGATIVE, FALSE);
  GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 900);

  for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (!rol_monster_room_target(ch, target))
      continue;
    (void)damage(ch, target, 400, -1, DAM_FIRE, FALSE);
  }
  (void)result;
}

static void rol_monster_winged_deva(struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next;
  int result;

  act("A blazing thunderclap strikes the room as $n calls to the heavens!", TRUE, ch, NULL, NULL,
      TO_ROOM);
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_room_target(ch, victim))
      continue;
    result = damage(ch, victim, 300, -1, DAM_ELECTRIC, FALSE);
    if (result >= 0)
      GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 300);
  }
  if (rand_number(1, 4) == 1)
    call_magic(ch, NULL, NULL, SPELL_EARTHQUAKE, 0, 60, CAST_INNATE);
}

static void rol_monster_area_damage(struct char_data *ch, enum rol_monster_combat_effect effect)
{
  struct char_data *victim;
  struct char_data *next;
  struct char_data *silence_target = NULL;
  int amount;
  int damage_type;
  bool saved;

  if (IS_CASTING(ch))
    return;
  if (effect == ROL_MONSTER_FIRE_BOSS)
    act("A storm of fire erupts from $n and showers the room!", TRUE, ch, NULL, NULL, TO_ROOM);
  else if (effect == ROL_MONSTER_EARTH_BOSS)
    act("$n calls down an avalanche of crushing rocks!", TRUE, ch, NULL, NULL, TO_ROOM);
  else if (effect == ROL_MONSTER_WATER_BOSS)
    act("A giant tidal wave surges out from $n!", TRUE, ch, NULL, NULL, TO_ROOM);

  if (effect == ROL_MONSTER_WATER_BOSS)
    silence_target = rol_monster_random_player(ch);

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_room_target(ch, victim))
      continue;

    if (effect == ROL_MONSTER_FIRE_BOSS)
    {
      amount = rand_number(150, 350);
      damage_type = DAM_FIRE;
      saved = savingthrow(ch, victim, SAVING_REFL, 0, CAST_INNATE, GET_LEVEL(ch), EVOCATION);
    }
    else if (effect == ROL_MONSTER_EARTH_BOSS)
    {
      amount = rand_number(50, 200);
      damage_type = DAM_EARTH;
      saved = savingthrow(ch, victim, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), CONJURATION);
    }
    else
    {
      amount = rand_number(100, 300);
      damage_type = DAM_WATER;
      saved = savingthrow(ch, victim, SAVING_REFL, 0, CAST_INNATE, GET_LEVEL(ch), EVOCATION);
    }
    if (saved)
      amount /= 2;
    if (damage(ch, victim, amount, -1, damage_type, FALSE) < 0)
      continue;

    if (effect == ROL_MONSTER_EARTH_BOSS && !saved)
      rol_monster_stun(victim, 2);
    else if (effect == ROL_MONSTER_WATER_BOSS && victim == silence_target)
      call_magic(ch, victim, NULL, SPELL_SILENCE, 0, 2, CAST_INNATE);
  }
}

static void rol_monster_stop_combat(struct char_data *victim)
{
  struct char_data *fighter;
  struct char_data *next;

  if (FIGHTING(victim) != NULL)
    stop_fighting(victim);
  for (fighter = combat_list; fighter != NULL; fighter = next)
  {
    next = fighter->next_fighting;
    if (FIGHTING(fighter) == victim)
      stop_fighting(fighter);
  }
}

static void rol_monster_air_boss(struct char_data *ch)
{
  struct char_data *victim = rol_monster_random_player(ch);
  room_rnum destination = NOWHERE;
  int amount;
  int checked;
  int direction;
  bool saved;

  if (victim == NULL || IS_CASTING(ch))
    return;
  amount = rand_number(100, 450);
  saved = savingthrow(ch, victim, SAVING_REFL, 0, CAST_INNATE, GET_LEVEL(ch), EVOCATION);
  if (saved)
    amount /= 2;

  act("A roaring whirlwind slams directly into you!", FALSE, ch, NULL, victim, TO_VICT);
  act("A roaring whirlwind slams directly into $N!", FALSE, ch, NULL, victim, TO_NOTVICT);
  if (damage(ch, victim, amount, -1, DAM_AIR, FALSE) < 0 || saved)
    return;

  direction = rand_number(0, NUM_OF_DIRS - 1);
  for (checked = 0; checked < NUM_OF_DIRS; checked++)
  {
    if (direction >= NUM_OF_DIRS)
      direction = 0;
    if (CAN_GO(victim, direction) &&
        valid_mortal_tele_dest(victim, EXIT(victim, direction)->to_room, false))
    {
      destination = EXIT(victim, direction)->to_room;
      break;
    }
    direction++;
  }
  if (destination == NOWHERE)
  {
    rol_monster_stun(victim, 2);
    return;
  }

  rol_monster_stop_combat(victim);
  act("The whirlwind hurls $n from the room!", TRUE, victim, NULL, NULL, TO_ROOM);
  char_from_room(victim);
  char_to_room(victim, destination);
  act("$n tumbles in on $s back!", FALSE, victim, NULL, NULL, TO_ROOM);
  rol_monster_stun(victim, 2);
}

static int rol_monster_small_prismatic_activity(struct spec_event_context *context,
                                                struct char_data *ch)
{
  struct char_data *master = ch->master;

  if (master == NULL || IN_ROOM(master) != IN_ROOM(ch) ||
      (FIGHTING(ch) == NULL && FIGHTING(master) == NULL))
  {
    act("$n vanishes in a swirl of color.", FALSE, ch, NULL, NULL, TO_ROOM);
    extract_char(ch);
    context->invalidation |= SPEC_INVALIDATE_OWNER | SPEC_INVALIDATE_ACTOR;
    return TRUE;
  }
  if (FIGHTING(ch) == NULL && FIGHTING(master) != NULL)
    (void)set_fighting(ch, FIGHTING(master));
  return FALSE;
}

static int rol_griffon_guard_react(struct char_data *guard, struct char_data *target)
{
  if (guard == NULL || target == NULL || !rol_griffon_guard_target_allowed(target) ||
      !VALID_ROOM_RNUM(IN_ROOM(guard)) || IN_ROOM(target) != IN_ROOM(guard) ||
      !CAN_SEE(guard, target))
    return FALSE;

  if (FIGHTING(guard) != NULL)
  {
    if (MOB_FLAGGED(guard, MOB_MEMORY))
      remember(guard, target);
    return FALSE;
  }

  act("$n bellows a fierce battle cry and attacks $N!", TRUE, guard, NULL, target, TO_NOTVICT);
  act("$n bellows a fierce battle cry and attacks you!", TRUE, guard, NULL, target, TO_VICT);
  (void)hit(guard, target, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);
  return FALSE;
}

static int rol_griffon_guard_activity(struct char_data *guard)
{
  struct char_data *target;

  for (target = world[IN_ROOM(guard)].people; target != NULL; target = target->next_in_room)
  {
    if (!rol_griffon_guard_target_allowed(target) || !CAN_SEE(guard, target))
      continue;
    return rol_griffon_guard_react(guard, target);
  }
  return FALSE;
}

static bool rol_drow_conclave_intruder_allowed(struct char_data *guard, struct char_data *intruder)
{
  return guard != NULL && intruder != NULL && !IS_NPC(intruder) &&
         GET_LEVEL(intruder) < LVL_IMMORT && CAN_SEE(guard, intruder);
}

static void rol_drow_conclave_enable_detection(void)
{
  struct char_data *guard;
  room_rnum room;
  int room_offset;

  for (room_offset = 0; room_offset < ROL_DROW_CONCLAVE_ROOM_COUNT; room_offset++)
  {
    room = real_room(ROL_DROW_CONCLAVE_BASE_VNUM + room_offset);
    if (!VALID_ROOM_RNUM(room))
    {
      log("SYSERR: RoL drow conclave alarm room %d is missing.",
          ROL_DROW_CONCLAVE_BASE_VNUM + room_offset);
      continue;
    }
    for (guard = world[room].people; guard != NULL; guard = guard->next_in_room)
    {
      if (rol_drow_conclave_detect_guard_profile(GET_MOB_VNUM(guard)))
        SET_BIT_AR(AFF_FLAGS(guard), AFF_DETECT_INVIS);
    }
  }
}

static bool rol_drow_conclave_assign_destination(struct char_data *guard, int deployment_index)
{
  int destination_vnum = rol_drow_conclave_destination_vnum(deployment_index);
  room_rnum destination;

  if (guard == NULL || destination_vnum < 0)
    return false;
  destination = real_room(destination_vnum);
  if (!VALID_ROOM_RNUM(destination))
  {
    log("SYSERR: RoL drow conclave deployment room %d is missing.", destination_vnum);
    return false;
  }
  GET_MOB_LOADROOM(guard) = destination;
  PROC_FIRED(guard) |= (int)ROL_DROW_CONCLAVE_REDEPLOY;
  return true;
}

static void rol_drow_conclave_deploy_barracks(void)
{
  struct char_data *guard;
  room_rnum barracks;
  int deployment_index;

  barracks = real_room(ROL_DROW_CONCLAVE_BARRACKS_VNUM);
  if (!VALID_ROOM_RNUM(barracks))
  {
    log("SYSERR: RoL drow conclave barracks room %d is missing.", ROL_DROW_CONCLAVE_BARRACKS_VNUM);
    return;
  }

  /* The source begins on the barracks room list but advances through global `next` links.
   * Keep its documented one-pass selection order without escaping into unrelated rooms. */
  guard = world[barracks].people;
  for (deployment_index = 0; deployment_index < 3; deployment_index++)
  {
    while (guard != NULL && GET_MOB_VNUM(guard) != 2093102)
      guard = guard->next_in_room;
    if (guard == NULL)
      return;
    (void)rol_drow_conclave_assign_destination(guard, deployment_index);
    guard = guard->next_in_room;
  }

  while (guard != NULL && GET_MOB_VNUM(guard) != 2093109)
    guard = guard->next_in_room;
  if (guard != NULL)
    (void)rol_drow_conclave_assign_destination(guard, 3);
}

static int rol_drow_conclave_guard_activity(struct char_data *guard)
{
  struct char_data *intruder;
  const char *combat_line;

  if ((unsigned int)PROC_FIRED(guard) & ROL_DROW_CONCLAVE_REDEPLOY)
  {
    if (GET_MOB_LOADROOM(guard) == IN_ROOM(guard))
      PROC_FIRED(guard) &= ~(int)ROL_DROW_CONCLAVE_REDEPLOY;
    else if (FIGHTING(guard) == NULL && GET_POS(guard) == POS_STANDING)
    {
      hunt_loadroom(guard);
      return FALSE;
    }
  }

  if (FIGHTING(guard) == NULL)
  {
    for (intruder = world[IN_ROOM(guard)].people; intruder != NULL;
         intruder = intruder->next_in_room)
    {
      if (rol_drow_conclave_intruder_allowed(guard, intruder))
        break;
    }
    if (intruder != NULL && !rol_drow_conclave_alarm_sounding)
    {
      rol_drow_conclave_alarm_sounding = true;
      do_say(guard, "I have sounded the alarm! There is no escape!", 0, 0);
      rol_drow_conclave_enable_detection();
      rol_drow_conclave_deploy_barracks();
      return TRUE;
    }
    return FALSE;
  }

  combat_line = rol_drow_conclave_combat_line(rand_number(1, 20));
  if (combat_line == NULL)
    return FALSE;
  do_say(guard, combat_line, 0, 0);
  return TRUE;
}

static int rol_monster_command(struct spec_event_context *context,
                               const struct rol_monster_combat_profile *profile,
                               struct char_data *ch)
{
  struct char_data *actor = context->actor;
  enum rol_planar_control_kind control_kind;
  const char *command;
  room_rnum destination;
  int direction;
  bool fleeing;

  if (actor == NULL || actor == ch || context->command <= 0 || !VALID_ROOM_RNUM(IN_ROOM(actor)) ||
      IN_ROOM(actor) != IN_ROOM(ch))
    return FALSE;

  if (profile->effect == ROL_MONSTER_GRIFFON_NONBARBARIAN)
    return rol_griffon_guard_react(ch, actor);

  command = complete_cmd_info != NULL ? complete_cmd_info[context->command].command : NULL;
  if (profile->effect == ROL_MONSTER_AVERNUS_GELUGON_HANARIEL && command != NULL &&
      !str_cmp(command, "disarm") && GET_LEVEL(actor) < LVL_IMMORT)
  {
    change_position(actor, POS_SITTING);
    SET_WAIT(actor, PULSE_VIOLENCE * 3);
    act("As you attempt a disarm, $n slides by and trips you with $s spear!", FALSE, ch, NULL,
        actor, TO_VICT);
    act("As $N attempts a disarm, $n slides by and flicks $N to the ground with $s spear!", FALSE,
        ch, NULL, actor, TO_NOTVICT);
    return TRUE;
  }
  if (rol_planar_control_profile(GET_MOB_VNUM(ch), &control_kind, NULL) && actor->master == ch &&
      AFF_FLAGGED(actor, AFF_CHARM) && !rol_planar_captive_command_allowed(command))
  {
    if (control_kind == ROL_PLANAR_CONTROL_GLABREZU)
      send_to_char(actor, "The crushing pincers leave you unable to do that.\r\n");
    else if (control_kind == ROL_PLANAR_CONTROL_MARILITH)
      send_to_char(actor, "The coiling tail leaves you unable to do that.\r\n");
    else
      send_to_char(actor, "Your thoughts are too hazy with devotion to do that.\r\n");
    return TRUE;
  }

  fleeing = command != NULL && !str_cmp(command, "flee");
  if (!IS_MOVE(context->command) && !fleeing)
    return FALSE;
  direction = IS_MOVE(context->command) ? complete_cmd_info[context->command].subcmd : -1;

  switch (profile->effect)
  {
  case ROL_MONSTER_KOBOLD_PRIEST:
    if (GET_LEVEL(actor) >= LVL_IMMORT)
      return FALSE;
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == 2001482 && direction == WEST)
    {
      destination = real_room(2001485);
      if (!VALID_ROOM_RNUM(destination) || !valid_mortal_tele_dest(actor, destination, false))
      {
        log("SYSERR: RoL kobold priest pit room 2001485 is unavailable");
        return TRUE;
      }
      act("$n cackles as $N tumbles from the dais into the sacrificial pit!", FALSE, ch, NULL,
          actor, TO_NOTVICT);
      send_to_char(actor, "The kobold priest cackles as you tumble into the sacrificial pit!\r\n");
      char_from_room(actor);
      char_to_room(actor, destination);
      look_at_room(actor, 0);
      return TRUE;
    }
    if (GET_ROOM_VNUM(IN_ROOM(ch)) == 2001482 &&
        (direction == NORTH || direction == SOUTH || direction == EAST))
    {
      act("$n makes a strange gesture, and an invisible wall blocks your way!", FALSE, ch, NULL,
          actor, TO_VICT);
      return TRUE;
    }
    if (GET_ROOM_VNUM(IN_ROOM(ch)) != 2001482 &&
        (direction == NORTH || direction == EAST || direction == SOUTH || direction == UP ||
         direction == DOWN) &&
        EXIT(actor, direction) != NULL && rand_number(1, 100) > 20)
    {
      act("$n makes a strange gesture, and an invisible wall blocks your way!", FALSE, ch, NULL,
          actor, TO_VICT);
      return TRUE;
    }
    return FALSE;
  case ROL_MONSTER_PHALANX:
    if (direction != UP)
      return FALSE;
    act("$n whirrs around the ceiling and prevents you from climbing upward.", TRUE, ch, NULL,
        actor, TO_VICT);
    return TRUE;
  case ROL_MONSTER_SKELETON:
    if (!CAN_SEE(ch, actor) || rand_number(1, 20) != 1)
      return FALSE;
    if (fleeing)
    {
      act("As you turn to flee, $n trips you!", FALSE, ch, NULL, actor, TO_VICT);
      GET_POS(actor) = MIN(GET_POS(actor), POS_SITTING);
    }
    else
      act("As you try to leave, $n leaps in front of you!", FALSE, ch, NULL, actor, TO_VICT);
    rol_monster_stun(actor, 1);
    return TRUE;
  case ROL_MONSTER_TREE_SPIRIT:
    if (direction != DOWN)
      return FALSE;
    act("$n grabs you and throws you across the room before you can leave!", FALSE, ch, NULL, actor,
        TO_VICT);
    if (damage(ch, actor, dice(8, 10), -1, DAM_BLUDGEON, FALSE) < 0)
      context->invalidation |= SPEC_INVALIDATE_ACTOR;
    return TRUE;
  case ROL_MONSTER_TAKO_DEMON:
    if (direction != UP || GET_ROOM_VNUM(IN_ROOM(ch)) != 2001485 ||
        GET_LEVEL(actor) >= LVL_IMMORT || rand_number(0, 100) <= 50)
      return FALSE;
    act("$n blocks your escape from the pit with a wicked grin.", FALSE, ch, NULL, actor, TO_VICT);
    return TRUE;
  case ROL_MONSTER_JOTUN_MIMER:
    if (direction != WEST || IN_ROOM(ch) != GET_MOB_LOADROOM(ch))
      return FALSE;
    if (GET_LEVEL(actor) < LVL_IMMORT && GET_RACE(actor) != RACE_TYPE_GIANT)
    {
      act("$n bars your path west and declares that only giants may pass.", FALSE, ch, NULL, actor,
          TO_VICT);
      return TRUE;
    }
    act("$n bows gravely and allows you to pass west.", FALSE, ch, NULL, actor, TO_VICT);
    return FALSE;
  default:
    return FALSE;
  }
}

static bool rol_skriaxit_sandstorm_target(struct char_data *ch, struct char_data *victim)
{
  struct char_data *owner;

  if (ch == NULL || victim == NULL || victim == ch || !VALID_ROOM_RNUM(IN_ROOM(victim)) ||
      ROOM_FLAGGED(IN_ROOM(victim), ROOM_PEACEFUL) || IS_INCORPOREAL(victim) ||
      (IS_ELEMENTAL(victim) &&
       (HAS_SUBRACE(victim, SUBRACE_AIR) || HAS_SUBRACE(victim, SUBRACE_EARTH))))
    return false;
  if (!IS_NPC(victim))
    return GET_LEVEL(victim) < LVL_IMMORT && aoeOK(ch, victim, -1);
  if (!IS_PET(victim) || (owner = victim->master) == NULL || IS_NPC(owner) ||
      GET_LEVEL(owner) >= LVL_IMMORT)
    return false;
  return aoeOK(ch, owner, -1);
}

static bool rol_skriaxit_sandstorm_resisted(struct char_data *ch, struct char_data *victim)
{
  int resistance = compute_spell_res(ch, victim, 0);

  return resistance > 0 && d20(ch) + 48 < resistance;
}

static void rol_skriaxit_sandstorm_dispel(struct char_data *ch, struct char_data *victim)
{
  struct affected_type *af;
  const char *wearoff;

  if (rol_skriaxit_sandstorm_resisted(ch, victim))
    return;
  for (af = victim->affected; af != NULL; af = af->next)
  {
    if (af->spell < 1 || af->spell > TOP_SPELL_DEFINE ||
        savingthrow(ch, victim, SAVING_WILL, 0, CAST_INNATE, 48, NOSCHOOL))
      continue;
    wearoff = get_wearoff(af->spell);
    if (wearoff != NULL && wearoff[0] != '\0' && wearoff[0] != '!')
      send_to_char(victim, "%s\r\n", wearoff);
    affect_remove(victim, af);
    return;
  }
}

static void rol_skriaxit_sandstorm_room(struct char_data *ch, room_rnum room)
{
  struct char_data *victim;
  struct char_data *next;

  if (ch == NULL || !VALID_ROOM_RNUM(room))
    return;
  send_to_room(room, "\tyA violent sandstorm blasts through the area.\tn\r\n");
  for (victim = world[room].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_skriaxit_sandstorm_target(ch, victim))
      continue;

    /* Source damage is zero because its room loop resets the passed Skriaxit count. */
    rol_skriaxit_sandstorm_dispel(ch, victim);
  }
}

static int rol_skriaxit_sandstorm_activity(struct char_data *ch)
{
  struct room_direction_data *exit;
  bool fires;
  int direction;

  ch->mob_specials.proc_fired =
      rol_skriaxit_sandstorm_advance_round(ch->mob_specials.proc_fired, &fires);
  if (!fires || ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
    return FALSE;

  rol_skriaxit_sandstorm_room(ch, IN_ROOM(ch));
  for (direction = NORTH; direction <= DOWN; direction++)
  {
    exit = world[IN_ROOM(ch)].dir_option[direction];
    if (exit == NULL || exit->to_room == NOWHERE || IS_SET(exit->exit_info, EX_CLOSED) ||
        !VALID_ROOM_RNUM(exit->to_room) || world[exit->to_room].people == NULL)
      continue;
    rol_skriaxit_sandstorm_room(ch, exit->to_room);
  }
  return FALSE;
}

static void rol_balor_equip_weapon(struct char_data *ch, int object_vnum, int wear_slot)
{
  struct obj_data *weapon;

  if (GET_EQ(ch, wear_slot) != NULL || real_object(object_vnum) == NOTHING)
    return;
  weapon = read_object(object_vnum, VIRTUAL);
  if (weapon != NULL)
    equip_char(ch, weapon, wear_slot);
}

static void rol_balor_ensure_weapons(struct char_data *ch)
{
  rol_balor_equip_weapon(ch, ROL_BALOR_SWORD_VNUM, WEAR_WIELD_1);
  rol_balor_equip_weapon(ch, ROL_BALOR_WHIP_VNUM, WEAR_WIELD_OFFHAND);
}

static int rol_monster_planar_death(struct spec_event_context *context,
                                    const struct rol_monster_combat_profile *profile,
                                    struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next;

  if (profile->effect == ROL_MONSTER_PLANAR_BALOR)
    return TRUE;
  if (profile->effect != ROL_MONSTER_PLANAR_MANES_DEATH)
    return FALSE;

  act("$n dissolves into a cloud of stinking acidic vapor!", FALSE, ch, NULL, NULL, TO_ROOM);
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next)
  {
    next = victim->next_in_room;
    if (!rol_monster_hit_area_target(ch, victim) ||
        (IS_NPC(victim) && MOB_FLAGGED(victim, MOB_ROL_DEMON)) ||
        savingthrow(ch, victim, SAVING_FORT, 0, CAST_INNATE, GET_LEVEL(ch), CONJURATION))
      continue;
    send_to_char(victim, "The acidic vapor sears your skin and lungs!\r\n");
    (void)rol_monster_successful_hit_damage(context, ch, victim, dice(4, 6), DAM_ACID);
  }
  return FALSE;
}

static int rol_monster_activity(struct spec_event_context *context,
                                const struct rol_monster_combat_profile *profile,
                                struct char_data *ch)
{
  int roll;

  switch (profile->effect)
  {
  case ROL_MONSTER_PLANAR_BALOR:
    rol_balor_ensure_weapons(ch);
    return FALSE;
  case ROL_MONSTER_PLANAR_GLABREZU_GRAB:
  case ROL_MONSTER_PLANAR_MARILITH_TAIL:
    return rol_planar_restrain_activity(ch);
  case ROL_MONSTER_PLANAR_SUCCUBUS_CHARM:
    return rol_planar_succubus_activity(ch);
  case ROL_MONSTER_PLANAR_VROCK_BURSTS:
    return rol_planar_vrock_dance_activity(context, ch);
  case ROL_MONSTER_HIVE_SKRIAXIT_SANDSTORM:
    return rol_skriaxit_sandstorm_activity(ch);
  case ROL_MONSTER_SMALL_PRISMATIC:
    return rol_monster_small_prismatic_activity(context, ch);
  case ROL_MONSTER_CHICKEN:
    if (!AWAKE(ch))
      return FALSE;
    roll = rand_number(1, 25);
    if (roll == 1)
      act("$n clucks contentedly on $s nest.", TRUE, ch, NULL, NULL, TO_ROOM);
    else if (roll == 2)
      act("$n becomes frightened and looks around, sensing danger nearby.", TRUE, ch, NULL, NULL,
          TO_ROOM);
    return FALSE;
  case ROL_MONSTER_KOBOLD_PRIEST:
    return rol_monster_kobold_priest_activity(ch);
  case ROL_MONSTER_PIERCER:
    return rol_monster_piercer_activity(ch);
  case ROL_MONSTER_PURPLE_WORM:
    if (GET_MAX_HIT(ch) != 20000)
      GET_MAX_HIT(ch) = GET_HIT(ch) = 20000;
    return FALSE;
  case ROL_MONSTER_PHALANX:
    return rol_monster_phalanx_activity(ch);
  case ROL_MONSTER_XEXOS:
  case ROL_MONSTER_AGTHRODOS:
    return rol_monster_xexos_activity(context, ch, profile->effect);
  case ROL_MONSTER_TREE_SPIRIT:
    if (FIGHTING(ch) != NULL)
      rol_monster_tree_spirit(ch);
    return FALSE;
  case ROL_MONSTER_SUMMON_JESSICA_WISP:
  case ROL_MONSTER_SUMMON_ROBYN_WISP:
  case ROL_MONSTER_SUMMON_ROBYN_SERVANT:
    rol_monster_residual_summon(profile, ch);
    return FALSE;
  case ROL_MONSTER_JURTREM:
    rol_monster_jurtrem(ch);
    return FALSE;
  case ROL_MONSTER_CRIMSON_FURY:
    rol_monster_crimson_fury(ch);
    return FALSE;
  case ROL_MONSTER_BARBARIAN_SPIRITIST:
    rol_monster_spiritist(ch);
    return FALSE;
  case ROL_MONSTER_TAKO_DEMON:
    rol_monster_tako_activity(ch);
    return FALSE;
  case ROL_MONSTER_WEREWOLF:
    rol_monster_werewolf_activity(ch);
    return FALSE;
  case ROL_MONSTER_JOTUN_MIMER:
    rol_monster_jotun_mimer_activity(ch);
    return FALSE;
  case ROL_MONSTER_SEELIE_FAERIE:
    return rol_seelie_faerie_activity(ch);
  case ROL_MONSTER_GRIFFON_NONBARBARIAN:
    return rol_griffon_guard_activity(ch);
  case ROL_MONSTER_UM2_DROW_CONCLAVE_GUARD:
    return rol_drow_conclave_guard_activity(ch);
  default:
    return FALSE;
  }
}

int rol_monster_combat(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);

  return FALSE;
}

int rol_monster_combat_typed(struct spec_event_context *context)
{
  const struct rol_monster_combat_profile *profile;
  struct char_data *ch;
  struct char_data *victim;
  int avernus_result;

  if (context == NULL || context->owner_type != SPEC_OWNER_MOBILE || context->owner == NULL)
    return FALSE;
  ch = context->owner;
  if (!IS_NPC(ch) || !VALID_ROOM_RNUM(IN_ROOM(ch)) ||
      (profile = rol_monster_combat_profile_for(GET_MOB_VNUM(ch))) == NULL)
    return FALSE;

  if (context->event == SPEC_EVENT_MOBILE_DEATH)
  {
    if (profile->effect == ROL_MONSTER_DARKHOLD_SHADOW_DRAGON)
      return rol_darkhold_mobile_death(context, ch);
    avernus_result = rol_avernus_mobile_event(context, ch);
    if (avernus_result != FALSE)
      return avernus_result;
    return rol_monster_planar_death(context, profile, ch);
  }

  if (profile->effect == ROL_MONSTER_RESIDUAL_MOBILE)
    return rol_residual_mobile_typed(context);

  if (context->event == SPEC_EVENT_COMMAND)
  {
    avernus_result = rol_avernus_mobile_event(context, ch);
    if (avernus_result != FALSE)
      return avernus_result;
    return rol_monster_command(context, profile, ch);
  }
  if (context->event == SPEC_EVENT_MOBILE_ACTIVITY)
  {
    avernus_result = rol_avernus_mobile_event(context, ch);
    if (avernus_result != FALSE)
      return avernus_result;
    return rol_monster_activity(context, profile, ch);
  }
  if (context->event == SPEC_EVENT_MOBILE_HIT)
  {
    if (spec_context_validate_combat_target(ch, context->target, false) != SPEC_CONTEXT_VALID)
      return FALSE;
    if (profile->effect == ROL_MONSTER_DARKHOLD_SHADOW_FIEND)
      return rol_darkhold_mobile_hit(context, ch);
    if (profile->effect == ROL_MONSTER_PLANAR_VROCK_BURSTS)
      return rol_planar_vrock_hit(context, ch);
    if (profile->effect == ROL_MONSTER_PLANAR_GLABREZU_GRAB)
      return rol_planar_restrain_hit(context, ch, ROL_PLANAR_CONTROL_GLABREZU);
    if (profile->effect == ROL_MONSTER_PLANAR_MARILITH_TAIL)
      return rol_planar_restrain_hit(context, ch, ROL_PLANAR_CONTROL_MARILITH);
    if (profile->effect == ROL_MONSTER_PLANAR_SPINAGON_SPIKES)
      return rol_planar_spinagon_spikes(context, ch);
    if (profile->effect == ROL_MONSTER_AVERNUS_GELUGON_MERITOS ||
        profile->effect == ROL_MONSTER_AVERNUS_GELUGON_HANARIEL)
    {
      (void)rol_avernus_gelugon_tail(ch, context->target);
      if (profile->effect == ROL_MONSTER_AVERNUS_GELUGON_MERITOS)
        (void)rol_avernus_meritos_silence(ch);
      return FALSE;
    }
    if (profile->effect == ROL_MONSTER_DUSK_PARALYSIS_GAZE)
      return rol_monster_dusk_paralysis_hit(context, ch);
    if (profile->effect == ROL_MONSTER_UM2_MANSCORPION_TAIL ||
        profile->effect == ROL_MONSTER_UM2_WYVERN_TAIL)
      return rol_monster_venom_tail_hit(context, ch);
    if (rol_manscorpion_venom_profile(GET_MOB_VNUM(ch), NULL, NULL, NULL))
      return rol_monster_manscorpion_hit(context, profile, ch);
    return rol_monster_successful_hit(context, profile, ch);
  }
  if (context->event == SPEC_EVENT_MOBILE_WAS_HIT)
  {
    if (spec_context_validate_combat_target(ch, context->target, false) != SPEC_CONTEXT_VALID)
      return FALSE;
    if (profile->effect == ROL_MONSTER_AVERNUS_BARBAZU_BERSERK)
      return rol_avernus_barbazu_berserk(ch, FIGHTING(ch));
    return FALSE;
  }
  if (context->event != SPEC_EVENT_MOBILE_COMBAT_TURN || (victim = FIGHTING(ch)) == NULL ||
      spec_context_validate_combat_target(ch, victim, true) != SPEC_CONTEXT_VALID)
    return FALSE;

  if (profile->effect == ROL_MONSTER_FIRE_BOSS || profile->effect == ROL_MONSTER_EARTH_BOSS ||
      profile->effect == ROL_MONSTER_AIR_BOSS || profile->effect == ROL_MONSTER_WATER_BOSS)
    (void)rol_alert_caller(ch, ch, 0, "");

  switch (profile->effect)
  {
  case ROL_MONSTER_CHICKEN:
    if (!AWAKE(ch) && rand_number(1, 4) == 1)
      act("$n screams and tries to run away!", TRUE, ch, NULL, NULL, TO_ROOM);
    return FALSE;
  case ROL_MONSTER_DRANUM:
    rol_monster_dranum(ch, victim);
    return FALSE;
  case ROL_MONSTER_SWALLOW_WHOLE:
    rol_monster_swallow(context, ch, victim, false);
    return FALSE;
  case ROL_MONSTER_SWALLOW_SPIT:
    rol_monster_swallow(context, ch, victim, true);
    return FALSE;
  case ROL_MONSTER_MOVANIC_DEVA:
    rol_monster_movanic_deva(ch);
    return FALSE;
  case ROL_MONSTER_CANTHUS:
    rol_monster_canthus(ch, victim);
    return FALSE;
  case ROL_MONSTER_PURPLE_WORM:
    rol_monster_purple_worm(context, ch, victim);
    return FALSE;
  case ROL_MONSTER_PIT_FIEND_BITE_TAIL:
  case ROL_MONSTER_AVERNUS_BEL:
    if (rand_number(1, 16) == 1)
      rol_monster_pit_fiend_bite(ch, victim);
    if ((context->invalidation & SPEC_INVALIDATE_TARGET) == 0)
      rol_monster_pit_fiend_tail(context, ch, victim);
    return FALSE;
  case ROL_MONSTER_CRIMSON_FURY:
    rol_monster_crimson_fury(ch);
    return FALSE;
  case ROL_MONSTER_BARBARIAN_SPIRITIST:
    rol_monster_spiritist(ch);
    return FALSE;
  case ROL_MONSTER_MANSCORPION_VENOM_LIGHT:
  case ROL_MONSTER_MANSCORPION_VENOM_MEDIUM:
  case ROL_MONSTER_MANSCORPION_VENOM_HEAVY:
  case ROL_MONSTER_MANSCORPION_VENOM_KING:
  case ROL_MONSTER_DOBLUTH_BANSHEE_WAIL:
  case ROL_MONSTER_DOBLUTH_BLADESTORM:
  case ROL_MONSTER_HIVE_SANDSTORM_BEAST:
  case ROL_MONSTER_HIVE_SKRIAXIT_SANDSTORM:
  case ROL_MONSTER_GREYCLOAK_BANSHEE_WAIL:
  case ROL_MONSTER_GREYCLOAK_FUMES:
  case ROL_MONSTER_GREYCLOAK_ARALESH:
  case ROL_MONSTER_DARKHOLD_SHADOW_FIEND:
  case ROL_MONSTER_DARKHOLD_SHADOW_DRAGON:
  case ROL_MONSTER_GRIFFON_NONBARBARIAN:
  case ROL_MONSTER_DUSK_PARALYSIS_GAZE:
  case ROL_MONSTER_UM2_MANSCORPION_TAIL:
  case ROL_MONSTER_UM2_WYVERN_TAIL:
  case ROL_MONSTER_UM2_DROW_CONCLAVE_GUARD:
    return FALSE;
  default:
    break;
  }

  if (profile->effect != ROL_MONSTER_PLANT_POISON && profile->effect != ROL_MONSTER_FOUR_ARMS &&
      profile->effect != ROL_MONSTER_ROT_BRINGER && !rol_monster_fires(profile))
    return FALSE;

  switch (profile->effect)
  {
  case ROL_MONSTER_PLANT_POISON:
    rol_monster_plant_poison(ch);
    break;
  case ROL_MONSTER_LYCAN_TIGER:
    rol_monster_lycan(ch, victim, true);
    break;
  case ROL_MONSTER_LYCAN_FOX:
    rol_monster_lycan(ch, victim, false);
    break;
  case ROL_MONSTER_SPIDER_VENOM:
    rol_monster_spider_venom(ch);
    break;
  case ROL_MONSTER_ASHENTORIS:
    rol_monster_ashentoris(ch, victim);
    break;
  case ROL_MONSTER_BANSHEE_WAIL:
    rol_monster_banshee_wail(ch);
    break;
  case ROL_MONSTER_FOUR_ARMS:
    rol_monster_four_arms(ch, victim);
    break;
  case ROL_MONSTER_TENTACLE_SLAM:
    rol_monster_shockwave(ch, SAVING_REFL);
    break;
  case ROL_MONSTER_ROT_BRINGER:
    rol_monster_rot_bringer(ch, victim);
    break;
  case ROL_MONSTER_WINGED_DEVA:
    rol_monster_winged_deva(ch);
    break;
  case ROL_MONSTER_SMALL_PRISMATIC:
    rol_monster_prismatic(ch, 15);
    break;
  case ROL_MONSTER_CRITICAL_PRISMATIC:
    rol_monster_prismatic(ch, 45);
    break;
  case ROL_MONSTER_UBER_PRISMATIC:
    rol_monster_prismatic(ch, 51);
    break;
  case ROL_MONSTER_FIRE_BOSS:
  case ROL_MONSTER_EARTH_BOSS:
  case ROL_MONSTER_WATER_BOSS:
    rol_monster_area_damage(ch, profile->effect);
    break;
  case ROL_MONSTER_AIR_BOSS:
    rol_monster_air_boss(ch);
    break;
  case ROL_MONSTER_JOTUN_THRYM:
    rol_monster_jotun_thrym(ch, victim);
    break;
  case ROL_MONSTER_JOTUN_LOKI:
    rol_monster_jotun_loki(ch);
    break;
  case ROL_MONSTER_KAMERYNN:
    rol_monster_kamerynn(ch, victim);
    break;
  case ROL_MONSTER_PIT_FIEND_BITE_TAIL:
  case ROL_MONSTER_AVERNUS_BEL:
  case ROL_MONSTER_AVERNUS_AUXILIARY:
  case ROL_MONSTER_CHICKEN:
  case ROL_MONSTER_KOBOLD_PRIEST:
  case ROL_MONSTER_PIERCER:
  case ROL_MONSTER_PURPLE_WORM:
  case ROL_MONSTER_PHALANX:
  case ROL_MONSTER_SKELETON:
  case ROL_MONSTER_XEXOS:
  case ROL_MONSTER_AGTHRODOS:
  case ROL_MONSTER_TREE_SPIRIT:
  case ROL_MONSTER_DRANUM:
  case ROL_MONSTER_SWALLOW_WHOLE:
  case ROL_MONSTER_SWALLOW_SPIT:
  case ROL_MONSTER_MOVANIC_DEVA:
  case ROL_MONSTER_CANTHUS:
  case ROL_MONSTER_SUMMON_JESSICA_WISP:
  case ROL_MONSTER_SUMMON_ROBYN_WISP:
  case ROL_MONSTER_SUMMON_ROBYN_SERVANT:
  case ROL_MONSTER_JURTREM:
  case ROL_MONSTER_CRIMSON_FURY:
  case ROL_MONSTER_BARBARIAN_SPIRITIST:
  case ROL_MONSTER_TAKO_DEMON:
  case ROL_MONSTER_WEREWOLF:
  case ROL_MONSTER_JOTUN_MIMER:
  case ROL_MONSTER_SEELIE_FAERIE:
  case ROL_MONSTER_MANSCORPION_VENOM_LIGHT:
  case ROL_MONSTER_MANSCORPION_VENOM_MEDIUM:
  case ROL_MONSTER_MANSCORPION_VENOM_HEAVY:
  case ROL_MONSTER_MANSCORPION_VENOM_KING:
  case ROL_MONSTER_DOBLUTH_BANSHEE_WAIL:
  case ROL_MONSTER_DOBLUTH_BLADESTORM:
  case ROL_MONSTER_HIVE_SANDSTORM_BEAST:
  case ROL_MONSTER_HIVE_SKRIAXIT_SANDSTORM:
  case ROL_MONSTER_GREYCLOAK_BANSHEE_WAIL:
  case ROL_MONSTER_GREYCLOAK_FUMES:
  case ROL_MONSTER_GREYCLOAK_ARALESH:
  case ROL_MONSTER_PLANAR_MANES_DEATH:
  case ROL_MONSTER_PLANAR_BALOR:
  case ROL_MONSTER_PLANAR_GLABREZU_GRAB:
  case ROL_MONSTER_PLANAR_MARILITH_TAIL:
  case ROL_MONSTER_PLANAR_SUCCUBUS_CHARM:
  case ROL_MONSTER_PLANAR_VROCK_BURSTS:
  case ROL_MONSTER_PLANAR_SPINAGON_SPIKES:
  case ROL_MONSTER_AVERNUS_BARBAZU_BERSERK:
  case ROL_MONSTER_AVERNUS_GELUGON_MERITOS:
  case ROL_MONSTER_AVERNUS_GELUGON_HANARIEL:
  case ROL_MONSTER_DARKHOLD_SHADOW_FIEND:
  case ROL_MONSTER_DARKHOLD_SHADOW_DRAGON:
  case ROL_MONSTER_GRIFFON_NONBARBARIAN:
  case ROL_MONSTER_DUSK_PARALYSIS_GAZE:
  case ROL_MONSTER_UM2_MANSCORPION_TAIL:
  case ROL_MONSTER_UM2_WYVERN_TAIL:
  case ROL_MONSTER_UM2_DROW_CONCLAVE_GUARD:
  case ROL_MONSTER_RESIDUAL_MOBILE:
    break;
  }
  return FALSE;
}
