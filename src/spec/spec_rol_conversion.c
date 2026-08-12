/**
 * @file spec/spec_rol_conversion.c
 * Shared adapters for active Realms of Luminari special procedures.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "character/guild_services.h"
#include "character/evolutions.h"
#include "combat/fight.h"
#include "comm.h"
#include "constants.h"
#include "db.h"
#include "dgscript/dg_scripts.h"
#include "graph.h"
#include "handler.h"
#include "helpers.h"
#include "interpreter.h"
#include "magic/domains_schools.h"
#include "magic/spells.h"
#include "mob/mob_utils.h"
#include "mud_event.h"
#include "mudlim.h"
#include "obj/shop.h"
#include "spec_combat.h"
#include "spec_context.h"
#include "spec_dispatch.h"
#include "spec_rol_conversion.h"
#include "spec_rol_totem.h"

#include <limits.h>

#define ROL_GATE_MAX_SUMMONS 5
#define ROL_BANANA_PEEL_VNUM 2001234
#define ROL_BANANA_FRUIT_VNUM 2001235
#define ROL_BANANA_PEEL_DECAY_TICKS 8
#define ROL_WATERDEEP_CASINO_EXIT_VNUM 2003254
#define ROL_WATERDEEP_BOUNCER_MAX_ROUTE 4
#define ROL_GUILD_CLASS(class_id) (1ULL << (class_id))
#define ROL_GUILD_RACE(race_id) (1ULL << (race_id))
#define ROL_MAJOR_BEHOLDER_EYES 10
#define ROL_MAJOR_BEHOLDER_COOLDOWN_BITS 2
#define ROL_MAJOR_BEHOLDER_COOLDOWN_MASK 3U
#define ROL_MAJOR_BEHOLDER_COOLDOWN_ROUNDS 3
#define ROL_GITH_RECLAIMER_VNUM 2019790
#define ROL_GITH_CHARGE_TIMER_SLOT 3

enum rol_weapon_effect
{
  ROL_WEAPON_HAMMER = 0,
  ROL_WEAPON_ICY_DAGGER,
  ROL_WEAPON_GLIMMERING_BURST,
  ROL_WEAPON_GITHYANKI_TWO_HANDED,
  ROL_WEAPON_GITHYANKI_CHARGED,
  ROL_WEAPON_VALHALLA_SCEPTER,
  ROL_WEAPON_SLENDER_ELVEN,
  ROL_WEAPON_NIGHTBRINGER,
  ROL_WEAPON_KIRIN_HORN,
  ROL_WEAPON_WINDSONG,
  ROL_WEAPON_SHADOW_DAGGER,
  ROL_WEAPON_FIRE_GIANT_SWORD,
  ROL_WEAPON_ACID_LONGSWORD,
  ROL_WEAPON_BARBED_SWORD,
  ROL_WEAPON_RIPPLING_FLAMES,
  ROL_WEAPON_JEWELED_FANG,
  ROL_WEAPON_BLACK_FLAMES,
  ROL_WEAPON_MOONBLADE_STARSONG,
  ROL_WEAPON_CRIMSON_DAGGER,
  ROL_WEAPON_MIELIKKI_SCIMITAR,
  ROL_WEAPON_FLAMBERGE,
  ROL_WEAPON_ORB,
  ROL_WEAPON_DOOMBRINGER,
  ROL_WEAPON_TAHLSHARA,
  ROL_WEAPON_ROCKCRUSHER,
  ROL_WEAPON_CYMRIC_HUGH,
  ROL_WEAPON_TORMENT,
  ROL_WEAPON_PAHLURUK_ROOT,
  ROL_WEAPON_REVERSE_DIRK,
  ROL_WEAPON_FRULGHIEM,
  ROL_WEAPON_SPHERE_LIGHTNING,
  ROL_WEAPON_HALRUAA_ENCHANTER,
  ROL_WEAPON_HALRUAA_ILLUSION,
  ROL_WEAPON_HALRUAA_INVOKER,
  ROL_WEAPON_HALRUAA_MAGEBANE,
  ROL_WEAPON_HALRUAA_DWARVEN_HAMMER,
  ROL_WEAPON_MYTH_DARKEN_AURA,
  ROL_WEAPON_MYTH_GLEAMING_BURST
};

struct rol_weapon_profile
{
  int object_vnum;
  enum rol_weapon_effect effect;
  int proc_denominator;
  bool critical_only;
  const char *description;
};

static const struct rol_weapon_profile rol_weapon_profiles[] = {
    {2004505, ROL_WEAPON_HAMMER, 22, false, "Chain-lightning proc."},
    {2013307, ROL_WEAPON_ICY_DAGGER, 1, true,
     "Critical cold burst; berserkers may invoke an ice storm."},
    {2014837, ROL_WEAPON_GLIMMERING_BURST, 28, false,
     "Glimmering mental burst with a chance to stun."},
    {2019886, ROL_WEAPON_GITHYANKI_TWO_HANDED, 23, false,
     "Silver-sword severing strike with a rare Gith reclaimer."},
    {2019900, ROL_WEAPON_GITHYANKI_CHARGED, 101, false,
     "Charged vorpal strike or Gith reclaimer; ten activations destroy the blade."},
    {2019912, ROL_WEAPON_VALHALLA_SCEPTER, 29, false,
     "Ancestral reverse swings and ranger or troll healing."},
    {2020075, ROL_WEAPON_SLENDER_ELVEN, 1, true,
     "Critical elven wound; incorporeal targets are immune."},
    {2026014, ROL_WEAPON_NIGHTBRINGER, 26, false, "Drowsing sleep proc."},
    {2034840, ROL_WEAPON_KIRIN_HORN, 26, false, "Lightning energy pulse."},
    {2038025, ROL_WEAPON_WINDSONG, 33, false,
     "Ranger-only blur flurry; rejects an ineligible wielder."},
    {2038095, ROL_WEAPON_WINDSONG, 33, false,
     "Ranger-only blur flurry; rejects an ineligible wielder."},
    {2040135, ROL_WEAPON_SHADOW_DAGGER, 1, true, "Critical shadow damage and a backstab burst."},
    {2080547, ROL_WEAPON_FIRE_GIANT_SWORD, 36, false, "Burning fire-giant flare."},
    {2089462, ROL_WEAPON_ACID_LONGSWORD, 1, true, "Critical acid spray."},
    {2091305, ROL_WEAPON_BARBED_SWORD, 22, false, "Freezing waves of torment."},
    {2095776, ROL_WEAPON_RIPPLING_FLAMES, 1, true,
     "Critical white-hot flare that heals fire creatures."},
    {2095851, ROL_WEAPON_JEWELED_FANG, 1, true, "Critical heart-piercing strike."},
    {2095876, ROL_WEAPON_BLACK_FLAMES, 26, false, "Engulfing black-flame cold burst."},
    {2095878, ROL_WEAPON_MOONBLADE_STARSONG, 31, false,
     "Nighttime star flare; say 'labelas' for weekly group barkskin."},
    {2098330, ROL_WEAPON_CRIMSON_DAGGER, 11, false,
     "Crimson critical strike or strength and agility drain."},
    {2019933, ROL_WEAPON_MIELIKKI_SCIMITAR, 31, false,
     "Ranger or Druid creeping-doom strike; rejects other wielders."},
    {2025030, ROL_WEAPON_FLAMBERGE, 22, false,
     "Flaming burst that heals Fire Elementals and Efreeti."},
    {2009054, ROL_WEAPON_ORB, 26, false,
     "Class-weighted cold burst; arcane criticals may raise a cold shield."},
    {2025018, ROL_WEAPON_DOOMBRINGER, 26, false, "Five-strike Doombringer flurry."},
    {2001010, ROL_WEAPON_TAHLSHARA, 26, false,
     "Bladesong recovery, knockdown, and wielder healing."},
    {2080034, ROL_WEAPON_ROCKCRUSHER, 29, false, "Grounded localized-earthquake knockdown."},
    {2080038, ROL_WEAPON_ROCKCRUSHER, 29, false, "Grounded localized-earthquake knockdown."},
    {2026233, ROL_WEAPON_CYMRIC_HUGH, 31, false, "Green beam carrying target-native harm."},
    {2026248, ROL_WEAPON_CYMRIC_HUGH, 31, false, "Green beam carrying target-native harm."},
    {2015116, ROL_WEAPON_TORMENT, 26, false, "Tormenting poison and blindness strike."},
    {2013308, ROL_WEAPON_PAHLURUK_ROOT, 21, false, "Entangling root strike."},
    {2097117, ROL_WEAPON_REVERSE_DIRK, 26, false, "Nonrecursive reverse strike."},
    {2001005, ROL_WEAPON_FRULGHIEM, 31, false, "Clenched-fist strike."},
    {2014023, ROL_WEAPON_SPHERE_LIGHTNING, 26, false, "Double lightning-bolt strike."},
    {2024405, ROL_WEAPON_SPHERE_LIGHTNING, 26, false, "Double lightning-bolt strike."},
    {2053266, ROL_WEAPON_HALRUAA_ENCHANTER, 27, false, "Halruaan enchanter debuff strike."},
    {2053263, ROL_WEAPON_HALRUAA_ILLUSION, 27, false, "Halruaan illusion debuff strike."},
    {2053259, ROL_WEAPON_HALRUAA_INVOKER, 26, false, "Halruaan flameheart burst."},
    {2053289, ROL_WEAPON_HALRUAA_MAGEBANE, 22, false,
     "NPC arcane-caster damage and casting interruption."},
    {2053290, ROL_WEAPON_HALRUAA_MAGEBANE, 22, false,
     "NPC arcane-caster damage and casting interruption."},
    {2053291, ROL_WEAPON_HALRUAA_MAGEBANE, 22, false,
     "NPC arcane-caster damage and casting interruption."},
    {2053292, ROL_WEAPON_HALRUAA_MAGEBANE, 22, false,
     "NPC arcane-caster damage and casting interruption."},
    {2053243, ROL_WEAPON_HALRUAA_DWARVEN_HAMMER, 28, false, "Runic freezing-cold burst."},
    {2083238, ROL_WEAPON_MYTH_DARKEN_AURA, 22, false,
     "Evil-wielder negative burst, blindness, and withering."},
    {2083235, ROL_WEAPON_MYTH_GLEAMING_BURST, 23, false,
     "Good-wielder faerie outline and blindness burst."},
};

struct rol_undead_drain_profile
{
  int mobile_vnum;
  int chance_sides;
  int marker_affect;
  int armor_penalty;
  int dexterity_penalty;
  int strength_penalty;
  int will_penalty;
  int fortitude_penalty;
  int slow_duration;
  const char *victim_message;
  const char *room_message;
  const char *attacker_message;
};

struct rol_waterdeep_bouncer_profile
{
  int mobile_vnum;
  size_t route_length;
  int route[ROL_WATERDEEP_BOUNCER_MAX_ROUTE];
};

static const struct rol_waterdeep_bouncer_profile rol_waterdeep_bouncer_profiles[] = {
    {2005523, 4, {2005532, 2005531, 2005530, 2003258}},
    {2005541, 3, {2005531, 2005530, 2003258, 0}},
    {2005542, 2, {2005530, 2003258, 0, 0}},
    {2005543, 4, {2005533, 2005531, 2005530, 2003258}},
};

static const struct rol_undead_drain_profile rol_undead_drain_profiles[] = {
    {2001256, 16, AFFECT_ROL_UNDEAD_MELEE_DRAIN, -1, -5, 0, 0, 0, 0,
     "You feel sickened as $n digs $s claws into you.",
     "$n digs $s claws into $N, who suddenly looks pale.", "$N pales as you rip into $S flesh."},
    {2001257, 21, AFFECT_ROL_UNDEAD_SPELL_DRAIN, 0, 0, -5, -1, 0, 0,
     "You feel weakened as $n draws on your life force.", "$n drains $N, who recoils in pain.",
     "$N recoils as you drain some of $S life force."},
    {2001258, 16, AFFECT_ROL_UNDEAD_MELEE_DRAIN, -2, -10, 0, 0, 0, 0,
     "You feel drained as $n touches you.", "$n touches $N, who suddenly looks drained.",
     "You touch $N, draining $S life force."},
    {2001259, 21, AFFECT_ROL_UNDEAD_MELEE_DRAIN, -2, -15, 0, 0, 0, 2,
     "As $n draws on your life force, your movements become sluggish.",
     "As $n draws on $N's life force, $S movements become sluggish.",
     "You draw on $N's life force, retarding $S movement."},
    {2001260, 21, AFFECT_ROL_UNDEAD_SPELL_DRAIN, 0, 0, -10, -1, 0, 0,
     "You feel weakened as $n draws on your life force.", "$n drains $N, who recoils in pain.",
     "$N recoils as you drain some of $S life force."},
    {2001261, 21, AFFECT_ROL_UNDEAD_MELEE_DRAIN, -3, -15, -15, 0, 0, -1,
     "Your willpower leaves you as $n tears into your very being!",
     "$n tears into $N, leaving $M quivering in fear!",
     "You tear into $N, rending $S very essence!"},
    {2001262, 21, AFFECT_ROL_UNDEAD_SPELL_DRAIN, 0, 0, -10, -1, -1, 0,
     "You feel weakened as $n draws on your life force.", "$n drains $N, who recoils in pain.",
     "$N recoils as you drain some of $S life force."},
};

struct rol_guild_guard_rule
{
  int room_vnum;
  int direction;
  unsigned long long class_mask;
  unsigned long long race_mask;
  bool protects;
};

struct rol_alert_profile
{
  int caller_vnum;
  const char *message;
  const int *helper_vnums;
  size_t helper_count;
};

struct rol_fixed_bodyguard_profile
{
  int bodyguard_vnum;
  int protected_vnum;
};

enum rol_command_sentinel_rule
{
  ROL_SENTINEL_GOOD_RACE_OVER_LEVEL = 0,
  ROL_SENTINEL_NON_ORC,
  ROL_SENTINEL_OVER_LEVEL,
  ROL_SENTINEL_CHANCE
};

struct rol_command_sentinel_profile
{
  int mobile_vnum;
  int room_vnum;
  int direction;
  int threshold;
  enum rol_command_sentinel_rule rule;
  const char *victim_message;
  const char *room_message;
};

enum rol_toll_keeper_kind
{
  ROL_TOLL_KEEPER_FEE_GATE = 0,
  ROL_TOLL_KEEPER_BRIDGE,
  ROL_TOLL_KEEPER_TICKET
};

struct rol_toll_keeper_profile
{
  int mobile_vnum;
  int room_vnum;
  enum rol_toll_keeper_kind kind;
  int direction;
  int destination_a;
  int destination_b;
  int fee_gold;
  int ticket_vnum;
  int entered_object_vnum;
};

enum rol_travel_portal_kind
{
  ROL_TRAVEL_PORTAL_DIMENSIONAL_FOLD = 0,
  ROL_TRAVEL_PORTAL_WATERDEEP,
  ROL_TRAVEL_PORTAL_ELF_GATE,
  ROL_TRAVEL_PORTAL_SHAMAN_SPORES,
  ROL_TRAVEL_PORTAL_BLIP,
  ROL_TRAVEL_PORTAL_ILLUSION_FOUNTAIN
};

struct rol_travel_portal_profile
{
  int object_vnum;
  enum rol_travel_portal_kind kind;
  int fixed_destination_vnum;
  int reward_vnum;
};

enum rol_death_effect
{
  ROL_DEATH_EFFECT_NONE = 0,
  ROL_DEATH_EFFECT_REPLACE,
  ROL_DEATH_EFFECT_REPLACE_SPELLUP,
  ROL_DEATH_EFFECT_DROP_OBJECT,
  ROL_DEATH_EFFECT_RETURN_TO_MASTER,
  ROL_DEATH_EFFECT_SHADOW_DARKNESS,
  ROL_DEATH_EFFECT_SPORE_POISON,
  ROL_DEATH_EFFECT_BALOR_BURST,
  ROL_DEATH_EFFECT_STONE_CRUMBLE
};

struct rol_death_profile
{
  int mobile_vnum;
  const char *message;
  const char *secondary_message;
  int replacement_vnum;
  int object_vnum;
  enum rol_death_effect effect;
  bool suppress_corpse;
};

enum rol_ambient_profile_id
{
  ROL_AMBIENT_WANDERER = 0,
  ROL_AMBIENT_DRUNK_ONE,
  ROL_AMBIENT_DRUNK_TWO,
  ROL_AMBIENT_DRUNK_THREE,
  ROL_AMBIENT_HOMELESS_ONE,
  ROL_AMBIENT_HOMELESS_TWO,
  ROL_AMBIENT_CAT_ONE,
  ROL_AMBIENT_MERCHANT_ONE,
  ROL_AMBIENT_MERCHANT_TWO,
  ROL_AMBIENT_FARMER_ONE,
  ROL_AMBIENT_BAKER_ONE,
  ROL_AMBIENT_BAKER_TWO,
  ROL_AMBIENT_MAGE_ONE,
  ROL_AMBIENT_CLERIC_ONE,
  ROL_AMBIENT_ARTILLERY_ONE,
  ROL_AMBIENT_WARRIOR_ONE,
  ROL_AMBIENT_MERCENARY_ONE,
  ROL_AMBIENT_MERCENARY_TWO,
  ROL_AMBIENT_MERCENARY_THREE,
  ROL_AMBIENT_CASINO_ONE,
  ROL_AMBIENT_CASINO_TWO,
  ROL_AMBIENT_YOUTH_ONE,
  ROL_AMBIENT_YOUTH_TWO,
  ROL_AMBIENT_TAILOR_ONE,
  ROL_AMBIENT_SHOPPER_ONE,
  ROL_AMBIENT_SHOPPER_TWO,
  ROL_AMBIENT_ASSASSIN_ONE,
  ROL_AMBIENT_BRIGAND_ONE,
  ROL_AMBIENT_FISHERMAN_ONE,
  ROL_AMBIENT_FISHERMAN_TWO,
  ROL_AMBIENT_SAILOR_ONE,
  ROL_AMBIENT_SEAMAN_ONE,
  ROL_AMBIENT_NAVAL_ONE,
  ROL_AMBIENT_NAVAL_TWO,
  ROL_AMBIENT_NAVAL_FOUR,
  ROL_AMBIENT_SEABIRD_ONE,
  ROL_AMBIENT_SEABIRD_TWO,
  ROL_AMBIENT_COMMONER_ONE,
  ROL_AMBIENT_COMMONER_THREE,
  ROL_AMBIENT_COMMONER_FOUR,
  ROL_AMBIENT_COMMONER_FIVE,
  ROL_AMBIENT_COMMONER_SIX,
  ROL_AMBIENT_WATERDEEP_GUARD_ONE,
  ROL_AMBIENT_WATERDEEP_GUARD_TWO,
};

struct rol_ambient_mobile_profile
{
  int mobile_vnum;
  enum rol_ambient_profile_id profile_id;
};

struct rol_ambient_action
{
  enum rol_ambient_profile_id profile_id;
  int roll;
  bool speech;
  const char *message;
};

enum rol_source_periodic_action_kind
{
  ROL_SOURCE_PERIODIC_ROOM_ACTION = 0,
  ROL_SOURCE_PERIODIC_SPEECH
};

struct rol_source_periodic_profile
{
  int mobile_vnum;
  int profile_id;
  int roll_min;
  int roll_max;
  int dice_count;
  int dice_sides;
  bool require_awake;
  bool require_sleeping;
  bool suppress_fighting;
};

struct rol_source_periodic_outcome
{
  int profile_id;
  int roll;
  size_t first_action;
  size_t action_count;
};

struct rol_source_periodic_action
{
  enum rol_source_periodic_action_kind kind;
  bool hide;
  const char *message;
};

#include "spec_rol_periodic_profiles.inc"

enum rol_state_periodic_state
{
  ROL_STATE_PERIODIC_IDLE = 0,
  ROL_STATE_PERIODIC_FIGHTING
};

struct rol_state_periodic_profile
{
  int mobile_vnum;
  int profile_id;
  int idle_dice_count;
  int idle_dice_sides;
  int fighting_dice_count;
  int fighting_dice_sides;
  bool cumulative_idle_while_fighting;
};

struct rol_state_periodic_outcome
{
  int profile_id;
  enum rol_state_periodic_state state;
  int roll;
  size_t first_action;
  size_t action_count;
};

#include "spec_rol_state_periodic_profiles.inc"

static const int rol_demogorgon_helpers[] = {2019830, 2019850, 2019880};
static const int rol_drisinil_helpers[] = {2059812, 2059815, 2059814};
static const int rol_tukra_helpers[] = {2059832, 2059833, 2059834};
static const int rol_imix_helpers[] = {2025402, 2025404, 2025405, 2025408};
static const int rol_imix_pet_helpers[] = {2025410, 2025405, 2025404};
static const int rol_yancbin_helpers[] = {2024410, 2024415, 2024420, 2024450};
static const int rol_xzix_helpers[] = {2062421, 2062444, 2062433};
static const int rol_drgun_helpers[] = {2062422, 2062442, 2062434};
static const int rol_limj_helpers[] = {2062420, 2062443, 2062432};
static const int rol_duyrn_helpers[] = {2062423, 2062441, 2062335};

static const struct rol_alert_profile rol_alert_profiles[] = {
    {2019920,
     "You will pay for attacking me mortal worms!  Denizens of Darkness, Come and Feast upon %s!",
     rol_demogorgon_helpers, sizeof(rol_demogorgon_helpers) / sizeof(rol_demogorgon_helpers[0])},
    {2019921,
     "You will pay for attacking me mortal worms!  Denizens of Darkness, Come and Feast upon %s!",
     rol_demogorgon_helpers, sizeof(rol_demogorgon_helpers) / sizeof(rol_demogorgon_helpers[0])},
    {2024440, "Denizens of air!  Come and destroy %s!", rol_yancbin_helpers,
     sizeof(rol_yancbin_helpers) / sizeof(rol_yancbin_helpers[0])},
    {2025406, "Denizens of fire!  Come and destroy %s!", rol_imix_helpers,
     sizeof(rol_imix_helpers) / sizeof(rol_imix_helpers[0])},
    {2025409, "Those loyal to Imix!  Come and destroy %s!", rol_imix_pet_helpers,
     sizeof(rol_imix_pet_helpers) / sizeof(rol_imix_pet_helpers[0])},
    {2059810, "Ssussun pholor dos %s!!  A'Quarthus Velg'Larn ulu ussa!!", rol_drisinil_helpers,
     sizeof(rol_drisinil_helpers) / sizeof(rol_drisinil_helpers[0])},
    {2059830, "(%s!! Ut baruk KneeCappers Ai-Menu!!", rol_tukra_helpers,
     sizeof(rol_tukra_helpers) / sizeof(rol_tukra_helpers[0])},
    {2062401, "Come to my aid!", rol_xzix_helpers,
     sizeof(rol_xzix_helpers) / sizeof(rol_xzix_helpers[0])},
    {2062402, "Come to my aid my minions!", rol_drgun_helpers,
     sizeof(rol_drgun_helpers) / sizeof(rol_drgun_helpers[0])},
    {2062405, "Come protect me my pets!", rol_limj_helpers,
     sizeof(rol_limj_helpers) / sizeof(rol_limj_helpers[0])},
    {2062406, "Protect me my minions!", rol_duyrn_helpers,
     sizeof(rol_duyrn_helpers) / sizeof(rol_duyrn_helpers[0])},
};

static const struct rol_fixed_bodyguard_profile rol_fixed_bodyguard_profiles[] = {
    {2097040, 2097023},
    {2097041, 2097029},
    {2097042, 2097008},
};

static const struct rol_command_sentinel_profile rol_command_sentinel_profiles[] = {
    {2001438, 2001483, WEST, 20, ROL_SENTINEL_CHANCE,
     "You try to leave the room but are shoved back by $n!",
     "$N tries to leave the room but is shoved back by $n!"},
    {2010301, 2010320, SOUTH, 0, ROL_SENTINEL_NON_ORC,
     "$n whispers, 'We don't want your type here. Get lost.'",
     "$n whispers something to $N, stopping $M with one hand."},
    {2010302, 2010302, SOUTH, 20, ROL_SENTINEL_OVER_LEVEL,
     "$n whispers, 'This area is far below you, unless you wish to fight me.'",
     "$n whispers something to $N, stopping $M with one hand."},
    {2081508, 2081596, SOUTH, 10, ROL_SENTINEL_GOOD_RACE_OVER_LEVEL,
     "$n lays a hand upon your shoulder and says, 'Ye may not pass.'",
     "$n lays a hand upon $N's shoulder and says, 'Ye may not pass.'"},
};

static const struct rol_toll_keeper_profile rol_toll_keeper_profiles[] = {
    {2001919, 2001863, ROL_TOLL_KEEPER_BRIDGE, -1, 2001862, 2001864, 5, -1, -1},
    {2007210, 2007680, ROL_TOLL_KEEPER_FEE_GATE, NORTH, 2007681, -1, 20, -1, -1},
    {2007335, 2007431, ROL_TOLL_KEEPER_FEE_GATE, SOUTH, 2007432, -1, 10, -1, -1},
    {2011106, 2005313, ROL_TOLL_KEEPER_TICKET, -1, -1, -1, 0, 2005341, 2011100},
    {2011306, 2005399, ROL_TOLL_KEEPER_TICKET, -1, -1, -1, 0, 2005341, 2011300},
    {2011542, 2011666, ROL_TOLL_KEEPER_FEE_GATE, UP, 2011667, -1, 500, -1, -1},
    {2014202, 2014237, ROL_TOLL_KEEPER_BRIDGE, -1, 2014236, 2014238, 5, -1, -1},
    {2098357, 2098425, ROL_TOLL_KEEPER_TICKET, -1, -1, -1, 0, 2000046, 2098451},
    {2098358, 2014312, ROL_TOLL_KEEPER_TICKET, -1, -1, -1, 0, 2000046, 2098451},
};

static const struct rol_travel_portal_profile rol_travel_portal_profiles[] = {
    {2000882, ROL_TRAVEL_PORTAL_DIMENSIONAL_FOLD, -1, -1},
    {2003088, ROL_TRAVEL_PORTAL_ILLUSION_FOUNTAIN, 2005582, -1},
    {2005515, ROL_TRAVEL_PORTAL_WATERDEEP, -1, -1},
    {2005516, ROL_TRAVEL_PORTAL_WATERDEEP, -1, -1},
    {2008112, ROL_TRAVEL_PORTAL_ELF_GATE, -1, -1},
    {2008113, ROL_TRAVEL_PORTAL_ELF_GATE, -1, -1},
    {2021500, ROL_TRAVEL_PORTAL_SHAMAN_SPORES, -1, -1},
    {2021501, ROL_TRAVEL_PORTAL_SHAMAN_SPORES, -1, -1},
    {2041941, ROL_TRAVEL_PORTAL_BLIP, -1, 2041900},
};

static const struct rol_death_profile rol_death_profiles[] = {
    {196030, "$n explodes in a mass of fire and energy!", NULL, 0, 0, ROL_DEATH_EFFECT_BALOR_BURST,
     true},
    {2000200, "As $n dies, $e melts into the shadows of the room.",
     "Suddenly shadows seem to cover a lot more of the room than before.", 0, 0,
     ROL_DEATH_EFFECT_SHADOW_DARKNESS, true},
    {2000202, "$n dissipates into a cloud of oily green smoke.", NULL, 0, 0, ROL_DEATH_EFFECT_NONE,
     true},
    {2000499, "$n slowly fades away out of existence...", NULL, 0, 0,
     ROL_DEATH_EFFECT_RETURN_TO_MASTER, true},
    {2000902, "The treant crashes into the ground and melts into the earth.", NULL, 0, 0,
     ROL_DEATH_EFFECT_NONE, true},
    {2000903, "A phantom steed fades into nothingness.", NULL, 0, 0, ROL_DEATH_EFFECT_NONE, true},
    {2000905, "The dark shade melts back into the shadows.", NULL, 0, 0, ROL_DEATH_EFFECT_NONE,
     true},
    {2000906, "A water mephit blinks out of existence.", NULL, 0, 0, ROL_DEATH_EFFECT_NONE, true},
    {2000907, "A fire mephit blinks out of existence.", NULL, 0, 0, ROL_DEATH_EFFECT_NONE, true},
    {2000908, "An earth mephit blinks out of existence.", NULL, 0, 0, ROL_DEATH_EFFECT_NONE, true},
    {2000909, "An air mephit blinks out of existence.", NULL, 0, 0, ROL_DEATH_EFFECT_NONE, true},
    {2001250, "With a loud puffing sound, the fire elemental dissipates into smoke.", NULL, 0, 0,
     ROL_DEATH_EFFECT_NONE, true},
    {2001251,
     "With a loud crash, the elemental dives into the ground. Only small stones remain "
     "where it once stood.",
     NULL, 0, 0, ROL_DEATH_EFFECT_NONE, true},
    {2001252, "With a gentle swooshing sound, the air elemental simply disappears.", NULL, 0, 0,
     ROL_DEATH_EFFECT_NONE, true},
    {2001253,
     "With a splash, the water elemental crashes to the ground leaving only a puddle "
     "behind.",
     NULL, 0, 0, ROL_DEATH_EFFECT_NONE, true},
    {2001433, "$n stops fighting and silently crumbles into a pile of stones.", NULL, 0, 2001438,
     ROL_DEATH_EFFECT_STONE_CRUMBLE, false},
    {2003050, "With a loud puffing sound, the fire elemental dissipates into smoke.", NULL, 0, 0,
     ROL_DEATH_EFFECT_NONE, true},
    {2003051,
     "With a loud crash, the elemental dives into the ground. Only small stones remain "
     "where it once stood.",
     NULL, 0, 0, ROL_DEATH_EFFECT_NONE, true},
    {2003052, "With a gentle swooshing sound, the air elemental simply disappears.", NULL, 0, 0,
     ROL_DEATH_EFFECT_NONE, true},
    {2003053,
     "With a splash, the water elemental crashes to the ground leaving only a puddle "
     "behind.",
     NULL, 0, 0, ROL_DEATH_EFFECT_NONE, true},
    {2012022, "$n crumples, a noxious gas escaping its interior.", NULL, 0, 0,
     ROL_DEATH_EFFECT_SPORE_POISON, false},
    {2012023, "$n crumples, a noxious gas escaping its interior.", NULL, 0, 0,
     ROL_DEATH_EFFECT_SPORE_POISON, false},
    {2053268, "$n's form shimmers and changes. A mighty demonic creature appears in $s place.",
     NULL, 2053269, 0, ROL_DEATH_EFFECT_REPLACE_SPELLUP, true},
    {2053269, "$n's form shimmers and changes. A shifting creature appears in $s place.", NULL,
     2053270, 0, ROL_DEATH_EFFECT_REPLACE_SPELLUP, true},
    {2053270, "As $n falls dead, one of $s eyes pops out and rolls around.", NULL, 0, 2053254,
     ROL_DEATH_EFFECT_DROP_OBJECT, false},
    {2053362, "$n falls to the ground, $s body rapidly disintegrating into dust.", NULL, 0, 0,
     ROL_DEATH_EFFECT_NONE, false},
    {2088812, "$n, vanquished, dissolves into ethereal vapors and disappears.", NULL, 0, 0,
     ROL_DEATH_EFFECT_NONE, false},
    {2088813, "$n dies, crumbling into powder which a sudden breeze sweeps away.", NULL, 0, 0,
     ROL_DEATH_EFFECT_NONE, false},
    {2088814, "As $n dies, $e shatters into crystal dust which quickly dissipates.", NULL, 0, 0,
     ROL_DEATH_EFFECT_NONE, false},
    {2088815, "As $n dies, $e disintegrates in a flash of bright light!", NULL, 0, 0,
     ROL_DEATH_EFFECT_NONE, false},
    {2090812, "The pure blood scholar screams as $e transforms into a wraith!", NULL, 2090914, 0,
     ROL_DEATH_EFFECT_REPLACE, true},
    {2090819, "The pure blood apprentice screams as $e transforms into a wraith!", NULL, 2090915, 0,
     ROL_DEATH_EFFECT_REPLACE, true},
    {2090837, "The pure blood apprentice screams as $e transforms into a wraith!", NULL, 2090916, 0,
     ROL_DEATH_EFFECT_REPLACE, true},
    {2090866, "The pure blood sorcerer screams as $e transforms into a lich!", NULL, 2090917, 0,
     ROL_DEATH_EFFECT_REPLACE, true},
    {2092613, NULL, NULL, 0, 0, ROL_DEATH_EFFECT_NONE, true},
    {2097003, NULL, NULL, 2097056, 0, ROL_DEATH_EFFECT_REPLACE, true},
};

static const struct rol_ambient_mobile_profile rol_ambient_mobile_profiles[] = {
    {2004830, ROL_AMBIENT_WANDERER},
    {2003064, ROL_AMBIENT_DRUNK_ONE},
    {2066037, ROL_AMBIENT_DRUNK_ONE},
    {2002836, ROL_AMBIENT_DRUNK_TWO},
    {2003006, ROL_AMBIENT_DRUNK_TWO},
    {2003203, ROL_AMBIENT_DRUNK_THREE},
    {2003236, ROL_AMBIENT_DRUNK_THREE},
    {2002816, ROL_AMBIENT_HOMELESS_ONE},
    {2003007, ROL_AMBIENT_HOMELESS_ONE},
    {2003065, ROL_AMBIENT_HOMELESS_ONE},
    {2002815, ROL_AMBIENT_HOMELESS_TWO},
    {2003066, ROL_AMBIENT_CAT_ONE},
    {2003090, ROL_AMBIENT_CAT_ONE},
    {2003009, ROL_AMBIENT_MERCHANT_ONE},
    {2005310, ROL_AMBIENT_MERCHANT_TWO},
    {2003010, ROL_AMBIENT_FARMER_ONE},
    {2003011, ROL_AMBIENT_BAKER_ONE},
    {2003012, ROL_AMBIENT_BAKER_TWO},
    {2003014, ROL_AMBIENT_MAGE_ONE},
    {2003030, ROL_AMBIENT_CLERIC_ONE},
    {2005321, ROL_AMBIENT_ARTILLERY_ONE},
    {2003018, ROL_AMBIENT_WARRIOR_ONE},
    {2003201, ROL_AMBIENT_MERCENARY_ONE},
    {2003210, ROL_AMBIENT_MERCENARY_ONE},
    {2002812, ROL_AMBIENT_MERCENARY_TWO},
    {2003242, ROL_AMBIENT_MERCENARY_TWO},
    {2002827, ROL_AMBIENT_MERCENARY_THREE},
    {2002835, ROL_AMBIENT_MERCENARY_THREE},
    {2003243, ROL_AMBIENT_MERCENARY_THREE},
    {2003204, ROL_AMBIENT_CASINO_ONE},
    {2003205, ROL_AMBIENT_CASINO_TWO},
    {2002813, ROL_AMBIENT_YOUTH_ONE},
    {2003232, ROL_AMBIENT_YOUTH_ONE},
    {2002829, ROL_AMBIENT_YOUTH_TWO},
    {2003234, ROL_AMBIENT_TAILOR_ONE},
    {2003235, ROL_AMBIENT_SHOPPER_ONE},
    {2003240, ROL_AMBIENT_SHOPPER_TWO},
    {2002825, ROL_AMBIENT_ASSASSIN_ONE},
    {2002830, ROL_AMBIENT_BRIGAND_ONE},
    {2005300, ROL_AMBIENT_FISHERMAN_ONE},
    {2005302, ROL_AMBIENT_FISHERMAN_TWO},
    {2005303, ROL_AMBIENT_SAILOR_ONE},
    {2005305, ROL_AMBIENT_SEAMAN_ONE},
    {2005307, ROL_AMBIENT_NAVAL_ONE},
    {2005308, ROL_AMBIENT_NAVAL_TWO},
    {2005320, ROL_AMBIENT_NAVAL_FOUR},
    {2005317, ROL_AMBIENT_SEABIRD_ONE},
    {2005318, ROL_AMBIENT_SEABIRD_TWO},
    {2003038, ROL_AMBIENT_COMMONER_ONE},
    {2005316, ROL_AMBIENT_COMMONER_THREE},
    {2002832, ROL_AMBIENT_COMMONER_FOUR},
    {2002833, ROL_AMBIENT_COMMONER_FIVE},
    {2002834, ROL_AMBIENT_COMMONER_SIX},
    {2003059, ROL_AMBIENT_WATERDEEP_GUARD_ONE},
    {2003070, ROL_AMBIENT_WATERDEEP_GUARD_ONE},
    {2003035, ROL_AMBIENT_WATERDEEP_GUARD_TWO},
};

static const struct rol_ambient_action rol_ambient_actions[] = {
    {ROL_AMBIENT_WANDERER, 2, false, "$n examines the animal tracks on the ground."},
    {ROL_AMBIENT_WANDERER, 3, true, "God, I love the outdoors!"},
    {ROL_AMBIENT_WANDERER, 4, false, "$n looks at you with a curious expression."},
    {ROL_AMBIENT_WANDERER, 5, false, "$n gazes off onto the horizon, looking for something."},
    {ROL_AMBIENT_DRUNK_ONE, 2, true, "Heeeeyyyy, matie, got any whiskey?"},
    {ROL_AMBIENT_DRUNK_ONE, 3, false, "$n mumbles something incoherent."},
    {ROL_AMBIENT_DRUNK_ONE, 4, false, "$n turns green and nearly hurls, but amazingly recovers."},
    {ROL_AMBIENT_DRUNK_ONE, 5, false, "$n stumbles and nearly falls, lost in his drunken stupor."},
    {ROL_AMBIENT_DRUNK_TWO, 2, true,
     "OOoohhh! Loookie what weee have  here, a worthless ball offf horse manuure.."},
    {ROL_AMBIENT_DRUNK_TWO, 3, false,
     "$n points at you and laughs uncontrollably for several minutes.."},
    {ROL_AMBIENT_DRUNK_TWO, 4, false,
     "$n flips you the bird and mumbles something incoherent under his breath."},
    {ROL_AMBIENT_DRUNK_TWO, 5, false,
     "$n begins singing loudly, though his awful tone makes you cringe."},
    {ROL_AMBIENT_DRUNK_TWO, 5, false, "Dogs can be heard howling in the distance."},
    {ROL_AMBIENT_DRUNK_THREE, 2, true, "Hey, pssssst, you. Yeah, you."},
    {ROL_AMBIENT_DRUNK_THREE, 2, true, "Know of any good places to gamble around here?"},
    {ROL_AMBIENT_DRUNK_THREE, 3, false,
     "$n loses his balance and falls to the ground, cursing all the while."},
    {ROL_AMBIENT_DRUNK_THREE, 4, false,
     "$n stares off into space, seemingly lost in some mindless thought."},
    {ROL_AMBIENT_DRUNK_THREE, 5, false, "$n shouts annoyingly, 'Where is that damn bartender!'"},
    {ROL_AMBIENT_HOMELESS_ONE, 2, true, "Alms for the poor?"},
    {ROL_AMBIENT_HOMELESS_ONE, 3, true, "Could you spare a few coins?"},
    {ROL_AMBIENT_HOMELESS_ONE, 4, false, "$n looks at you pleadingly."},
    {ROL_AMBIENT_HOMELESS_ONE, 5, false, "$n sniffs sadly, looking depressed."},
    {ROL_AMBIENT_HOMELESS_TWO, 2, true,
     "Could ya spare a few coins? Just a few? I gots nuttin' ta eat tonight.."},
    {ROL_AMBIENT_HOMELESS_TWO, 2, false, "$n whimpers quietly."},
    {ROL_AMBIENT_HOMELESS_TWO, 3, false,
     "$n is overcome with a fit of coughing. He doesn't look well."},
    {ROL_AMBIENT_HOMELESS_TWO, 4, false, "$n looks utterly miserable."},
    {ROL_AMBIENT_HOMELESS_TWO, 5, false, "$n holds out his hands, begging for food."},
    {ROL_AMBIENT_CAT_ONE, 2, false, "$n scratches at an itch."},
    {ROL_AMBIENT_CAT_ONE, 3, false, "$n dives at something on the ground, playing."},
    {ROL_AMBIENT_CAT_ONE, 4, false, "$n looks at you and mews, purring for attention."},
    {ROL_AMBIENT_CAT_ONE, 5, false, "$n approaches and bumps your leg, looking for attention."},
    {ROL_AMBIENT_MERCHANT_ONE, 2, true,
     "You wouldn't happen to know where the bazaar is, would you?"},
    {ROL_AMBIENT_MERCHANT_ONE, 3, false,
     "$n looks condescendingly at you, as if you're less than scum."},
    {ROL_AMBIENT_MERCHANT_ONE, 4, false,
     "$n looks you up and down, probably sizing up whether or not you're worth the effort."},
    {ROL_AMBIENT_MERCHANT_ONE, 5, false, "$n smirks arrogantly."},
    {ROL_AMBIENT_MERCHANT_TWO, 2, true, "GOD, where is that blasted ship!"},
    {ROL_AMBIENT_MERCHANT_TWO, 3, false,
     "$n stares out the door, scanning the harbor for his ship."},
    {ROL_AMBIENT_MERCHANT_TWO, 4, true,
     "Receptionist! Get that damn ship here! I've been waiting forever!"},
    {ROL_AMBIENT_MERCHANT_TWO, 5, false,
     "$n looks impatient, as if he's waited years for his ship to come in."},
    {ROL_AMBIENT_FARMER_ONE, 2, true, "I hate these big cities."},
    {ROL_AMBIENT_FARMER_ONE, 2, false, "$n frowns."},
    {ROL_AMBIENT_FARMER_ONE, 3, false, "$n smiles warmly at you."},
    {ROL_AMBIENT_FARMER_ONE, 4, false, "$n looks a bit lost."},
    {ROL_AMBIENT_FARMER_ONE, 5, false, "$n looks a bit timid in this huge city."},
    {ROL_AMBIENT_BAKER_ONE, 2, true, "Do you have a reason to be here? Not that I mind."},
    {ROL_AMBIENT_BAKER_ONE, 3, false, "$n looks around for something to clean."},
    {ROL_AMBIENT_BAKER_ONE, 4, false, "$n looks out the window at the glorious city."},
    {ROL_AMBIENT_BAKER_ONE, 5, false, "$n smiles warmly at you."},
    {ROL_AMBIENT_BAKER_TWO, 2, true, "Hey, ma! Can we go outside and play?"},
    {ROL_AMBIENT_BAKER_TWO, 3, false, "$n crashes into a table while running around."},
    {ROL_AMBIENT_BAKER_TWO, 4, false, "$n looks around for something to play with."},
    {ROL_AMBIENT_BAKER_TWO, 5, false, "$n runs around the room, playing wildly."},
    {ROL_AMBIENT_MAGE_ONE, 2, false, "$n attempts a spell."},
    {ROL_AMBIENT_MAGE_ONE, 2, true, "Tass Mohjak Tamarilon Deiliak!"},
    {ROL_AMBIENT_MAGE_ONE, 2, false, "$n frowns in frustration."},
    {ROL_AMBIENT_MAGE_ONE, 3, false, "$n stares blankly into space, contemplating something."},
    {ROL_AMBIENT_MAGE_ONE, 4, false, "$n looks at you curiously."},
    {ROL_AMBIENT_MAGE_ONE, 5, false, "$n studies his spellbook intently."},
    {ROL_AMBIENT_CLERIC_ONE, 2, true, "Go in peace, friend, all are welcome here."},
    {ROL_AMBIENT_CLERIC_ONE, 2, false, "$n smiles warmly at you."},
    {ROL_AMBIENT_CLERIC_ONE, 3, false, "$n bows before you in reverence."},
    {ROL_AMBIENT_CLERIC_ONE, 4, false, "$n performs a magical gesture of some kind."},
    {ROL_AMBIENT_CLERIC_ONE, 5, false,
     "$n sings a hymn in praise to the Gods. It is quite beautiful."},
    {ROL_AMBIENT_ARTILLERY_ONE, 2, false,
     "$n takes a long, deep breath as a cool breeze blows by."},
    {ROL_AMBIENT_ARTILLERY_ONE, 2, true, "Hell of a day, isn't it.."},
    {ROL_AMBIENT_ARTILLERY_ONE, 3, false, "$n checks the readiness of the catapult."},
    {ROL_AMBIENT_ARTILLERY_ONE, 4, true,
     "You should consider a career in the navy, strong as you are."},
    {ROL_AMBIENT_ARTILLERY_ONE, 5, false, "$n scans the horizon line intently."},
    {ROL_AMBIENT_WARRIOR_ONE, 2, true, "Don't you wish you were as strong and mighty as I?"},
    {ROL_AMBIENT_WARRIOR_ONE, 3, false,
     "$n sizes you up, as if considering your battle capabilities."},
    {ROL_AMBIENT_WARRIOR_ONE, 4, false, "$n screws up a sword maneuver, blushing furiously."},
    {ROL_AMBIENT_WARRIOR_ONE, 5, false, "$n shadow boxes, showing off his battle prowess."},
    {ROL_AMBIENT_MERCENARY_ONE, 2, true, "If ya need a hired hand, I'm yer man."},
    {ROL_AMBIENT_MERCENARY_ONE, 3, false,
     "$n keeps his hand on the hilt of his weapon while near you."},
    {ROL_AMBIENT_MERCENARY_ONE, 4, false, "$n stops suddenly as if having heard something odd."},
    {ROL_AMBIENT_MERCENARY_ONE, 4, false, "After a few moments, $n continues on his way."},
    {ROL_AMBIENT_MERCENARY_ONE, 5, false, "$n eyes you suspiciously."},
    {ROL_AMBIENT_MERCENARY_TWO, 2, true,
     "Get lost, kid, or I might decide to relieve you of your pathetic existence."},
    {ROL_AMBIENT_MERCENARY_TWO, 3, false,
     "$n growls as you, resembling a not-so-trained Doberman."},
    {ROL_AMBIENT_MERCENARY_TWO, 4, false, "$n glares icily at you."},
    {ROL_AMBIENT_MERCENARY_TWO, 5, false, "$n casts you a wary glance."},
    {ROL_AMBIENT_MERCENARY_THREE, 2, true, "Hey, waiter, bring me another when you come around."},
    {ROL_AMBIENT_MERCENARY_THREE, 3, false,
     "$n lets off a roaring belch that echoes around the room."},
    {ROL_AMBIENT_MERCENARY_THREE, 4, false, "$n gives you a casual glance."},
    {ROL_AMBIENT_MERCENARY_THREE, 5, false, "$n takes a long draught from his mug."},
    {ROL_AMBIENT_CASINO_ONE, 2, false,
     "$n moves some gambling chips around so fast you almost can't follow his movements."},
    {ROL_AMBIENT_CASINO_ONE, 3, false, "$n shuffles the cards with the ease of a skilled pro."},
    {ROL_AMBIENT_CASINO_ONE, 4, true, "Dealer raises 20."},
    {ROL_AMBIENT_CASINO_ONE, 5, false, "$n deals out a card to one of the gamblers."},
    {ROL_AMBIENT_CASINO_ONE, 6, true, "Feel lucky tonight, boys?"},
    {ROL_AMBIENT_CASINO_ONE, 7, false,
     "$n makes a perfect poker face, looking as rigid as a board.."},
    {ROL_AMBIENT_CASINO_TWO, 2, true, "I'll raise 20."},
    {ROL_AMBIENT_CASINO_TWO, 2, false, "$n studies his cards carefully."},
    {ROL_AMBIENT_CASINO_TWO, 3, false, "$n studies his cards carefully."},
    {ROL_AMBIENT_CASINO_TWO, 4, true, "C'mon, lady luck don't let me down!"},
    {ROL_AMBIENT_CASINO_TWO, 5, false, "$n makes an admirable poker face."},
    {ROL_AMBIENT_CASINO_TWO, 6, false, "$n nods his head."},
    {ROL_AMBIENT_YOUTH_ONE, 2, true, "Piss off, ya big pile of horse dung."},
    {ROL_AMBIENT_YOUTH_ONE, 3, false, "$n looks at you with eyes both angry and hateful."},
    {ROL_AMBIENT_YOUTH_ONE, 4, false, "$n spits at the ground in front of you."},
    {ROL_AMBIENT_YOUTH_ONE, 5, false, "$n glares at you with contempt."},
    {ROL_AMBIENT_YOUTH_TWO, 2, true, "Do-do you have anything I could eat?"},
    {ROL_AMBIENT_YOUTH_TWO, 3, false, "$n looks at you pleadingly."},
    {ROL_AMBIENT_YOUTH_TWO, 4, false, "$n holds out a feeble hand."},
    {ROL_AMBIENT_YOUTH_TWO, 5, false, "$n shivers in fear."},
    {ROL_AMBIENT_TAILOR_ONE, 2, true,
     "Hello. You don't look like a cityguard, are you here for a fitting?"},
    {ROL_AMBIENT_TAILOR_ONE, 3, false, "$n starts picking up small pieces of lint and thread."},
    {ROL_AMBIENT_TAILOR_ONE, 4, false,
     "$n looks at you and says, 'You could stand to loose a few pounds.'"},
    {ROL_AMBIENT_TAILOR_ONE, 4, false, "$n winks at you in amusement."},
    {ROL_AMBIENT_TAILOR_ONE, 5, false, "$n sorts through his many measuring tapes."},
    {ROL_AMBIENT_SHOPPER_ONE, 2, true, "Hi there!  Hope you're havin' more luck than me!"},
    {ROL_AMBIENT_SHOPPER_ONE, 2, false, "$n smiles at you."},
    {ROL_AMBIENT_SHOPPER_ONE, 3, false,
     "$n looks around frustrated, as if he can't find what he wants to buy."},
    {ROL_AMBIENT_SHOPPER_ONE, 4, false,
     "$n says, 'You can never find what you want in this damn bazaar!"},
    {ROL_AMBIENT_SHOPPER_ONE, 5, false, "$n browses through the goods for sale here."},
    {ROL_AMBIENT_SHOPPER_TWO, 2, true, "Hi there, having any luck today?"},
    {ROL_AMBIENT_SHOPPER_TWO, 3, false, "$n counts her money carefully."},
    {ROL_AMBIENT_SHOPPER_TWO, 4, false, "$n browses through the items for sale."},
    {ROL_AMBIENT_SHOPPER_TWO, 5, false, "$n smiles at you and says, 'Good day.'"},
    {ROL_AMBIENT_ASSASSIN_ONE, 2, false, "$n bows before you."},
    {ROL_AMBIENT_ASSASSIN_ONE, 2, true, "Walk in shadows, friend."},
    {ROL_AMBIENT_ASSASSIN_ONE, 3, false,
     "$n does a quick dodge in front of you, showing off his skill."},
    {ROL_AMBIENT_ASSASSIN_ONE, 4, false, "$n watches you intently, a devious look in his eye."},
    {ROL_AMBIENT_ASSASSIN_ONE, 5, false,
     "$n makes a lightning-fast move as he practices his backstab."},
    {ROL_AMBIENT_BRIGAND_ONE, 2, true, "Greetings, mate!"},
    {ROL_AMBIENT_BRIGAND_ONE, 3, false, "$n looks off onto the horizon."},
    {ROL_AMBIENT_BRIGAND_ONE, 4, false, "$n whistles a chipper tune."},
    {ROL_AMBIENT_BRIGAND_ONE, 5, false, "$n looks at you with a curious expression."},
    {ROL_AMBIENT_FISHERMAN_ONE, 2, true, "Damn fish ain't been biting all day."},
    {ROL_AMBIENT_FISHERMAN_ONE, 3, false, "$n stares off onto the horizon."},
    {ROL_AMBIENT_FISHERMAN_ONE, 4, false, "$n slowly reels in his line."},
    {ROL_AMBIENT_FISHERMAN_ONE, 5, false, "$n casts his line into the harbor."},
    {ROL_AMBIENT_FISHERMAN_TWO, 2, true,
     "I.. I.. I-I looove fishhhing..  I-It's sooooo relaxing, y'know?"},
    {ROL_AMBIENT_FISHERMAN_TWO, 2, true, "D-Do you like fishing?"},
    {ROL_AMBIENT_FISHERMAN_TWO, 3, false, "$n burps loudly."},
    {ROL_AMBIENT_FISHERMAN_TWO, 4, false, "$n pukes over the size of the pier."},
    {ROL_AMBIENT_FISHERMAN_TWO, 5, false, "$n mumbles something incoherent."},
    {ROL_AMBIENT_SAILOR_ONE, 2, true, "Don't get in my way, mate. I got work to do."},
    {ROL_AMBIENT_SAILOR_ONE, 3, false, "$n looks across the dock for something or someone."},
    {ROL_AMBIENT_SAILOR_ONE, 4, false, "$n looks as though he's been working hard all day."},
    {ROL_AMBIENT_SAILOR_ONE, 5, false, "$n gives you a casual glance."},
    {ROL_AMBIENT_SEAMAN_ONE, 2, true, "Out of my way, kid!"},
    {ROL_AMBIENT_SEAMAN_ONE, 3, false, "$n looks annoyingly at you."},
    {ROL_AMBIENT_SEAMAN_ONE, 4, false, "$n looks very proud of himself."},
    {ROL_AMBIENT_SEAMAN_ONE, 5, false, "$n gives you an icy stare."},
    {ROL_AMBIENT_NAVAL_ONE, 2, true, "hey, could you hand some of those nails?"},
    {ROL_AMBIENT_NAVAL_ONE, 3, false, "$n pounds at the ship plates."},
    {ROL_AMBIENT_NAVAL_ONE, 4, false, "$n sweats from the strenuous work."},
    {ROL_AMBIENT_NAVAL_ONE, 5, false, "$n works diligently at his job."},
    {ROL_AMBIENT_NAVAL_TWO, 2, true, "Looks good, boys. Keep it up."},
    {ROL_AMBIENT_NAVAL_TWO, 3, false, "$n inspects the underside of the ship for flaws."},
    {ROL_AMBIENT_NAVAL_TWO, 4, false, "$n hands some nails to the worker."},
    {ROL_AMBIENT_NAVAL_TWO, 5, false, "$n looks over the ship plans."},
    {ROL_AMBIENT_NAVAL_FOUR, 2, true, "Howdy."},
    {ROL_AMBIENT_NAVAL_FOUR, 2, false, "$n smiles warmly at you."},
    {ROL_AMBIENT_NAVAL_FOUR, 3, true, "Just make sure you're not on the gates when I open them."},
    {ROL_AMBIENT_NAVAL_FOUR, 4, false, "$n looks around the harbor, taking in everything."},
    {ROL_AMBIENT_NAVAL_FOUR, 5, false, "$n scans the horizon for sea vessels."},
    {ROL_AMBIENT_SEABIRD_ONE, 2, false, "$n chirps loudly."},
    {ROL_AMBIENT_SEABIRD_ONE, 3, false, "$n pecks at something on the ground."},
    {ROL_AMBIENT_SEABIRD_ONE, 4, false, "$n looks at you warily."},
    {ROL_AMBIENT_SEABIRD_ONE, 5, false, "$n flies close by, looking for a handout."},
    {ROL_AMBIENT_SEABIRD_TWO, 2, false,
     "$n notices something on the ground, and stares at it intently."},
    {ROL_AMBIENT_SEABIRD_TWO, 3, false, "$n flaps it's wings about on the ground."},
    {ROL_AMBIENT_SEABIRD_TWO, 4, false, "$n stands absolutely still, as if trying to look stoic."},
    {ROL_AMBIENT_SEABIRD_TWO, 5, false, "$n stares at you intently."},
    {ROL_AMBIENT_COMMONER_ONE, 2, true, "Hello."},
    {ROL_AMBIENT_COMMONER_ONE, 3, false, "$n purposefully averts his gaze."},
    {ROL_AMBIENT_COMMONER_ONE, 4, false, "$n whistles softly to himself."},
    {ROL_AMBIENT_COMMONER_ONE, 5, false, "$n looks for a second, then looks away quickly."},
    {ROL_AMBIENT_COMMONER_THREE, 2, true, "Beautiful, isn't it?"},
    {ROL_AMBIENT_COMMONER_THREE, 2, false, "$n smiles at you."},
    {ROL_AMBIENT_COMMONER_THREE, 3, false,
     "$n takes a deep breath as the breeze blows in, looking very relaxed."},
    {ROL_AMBIENT_COMMONER_THREE, 4, false, "$n closes her eyes, and looks deep in thought."},
    {ROL_AMBIENT_COMMONER_THREE, 5, false, "$n gazes long across the ocean, lost in thought."},
    {ROL_AMBIENT_COMMONER_FOUR, 2, true, "You think you can take me, eh?"},
    {ROL_AMBIENT_COMMONER_FOUR, 3, false, "$n spins around on the mat."},
    {ROL_AMBIENT_COMMONER_FOUR, 4, false, "$n growls."},
    {ROL_AMBIENT_COMMONER_FOUR, 5, false, "$n grunts as he tries a difficult move."},
    {ROL_AMBIENT_COMMONER_FIVE, 2, true, "Get 'im!  Don't let get behind ya!"},
    {ROL_AMBIENT_COMMONER_FIVE, 3, false, "$n roots for her man."},
    {ROL_AMBIENT_COMMONER_FIVE, 4, false, "$n gasps as the struggle intensifies."},
    {ROL_AMBIENT_COMMONER_FIVE, 5, false, "$n cheers enthusiastically!"},
    {ROL_AMBIENT_COMMONER_SIX, 2, true, "Go, dad, go!"},
    {ROL_AMBIENT_COMMONER_SIX, 3, false, "$n runs around in excitement."},
    {ROL_AMBIENT_COMMONER_SIX, 4, false, "$n cheers wildly."},
    {ROL_AMBIENT_COMMONER_SIX, 5, false, "$n hoots with joy as the struggle continues."},
    {ROL_AMBIENT_WATERDEEP_GUARD_ONE, 2, true, "Good day, citizen!"},
    {ROL_AMBIENT_WATERDEEP_GUARD_ONE, 3, false,
     "$n looks at you intently for a moment, then smiles."},
    {ROL_AMBIENT_WATERDEEP_GUARD_ONE, 4, false,
     "$n looks around, observing everything for trouble."},
    {ROL_AMBIENT_WATERDEEP_GUARD_ONE, 5, false, "$n scans the area for signs of trouble."},
    {ROL_AMBIENT_WATERDEEP_GUARD_TWO, 2, true, "Hell of a view, isn't it?"},
    {ROL_AMBIENT_WATERDEEP_GUARD_TWO, 3, false,
     "$n looks you up and down for a moment, then goes back to his duties."},
    {ROL_AMBIENT_WATERDEEP_GUARD_TWO, 4, false, "$n scans the area for signs of trouble."},
    {ROL_AMBIENT_WATERDEEP_GUARD_TWO, 5, false, "$n scans the landscape intently."},
};

/* Only rooms reached by active converted guild_guard bindings are retained.
 * Target VNUMs are the source room VNUMs under the Phase 4 +2,000,000 offset. */
static const struct rol_guild_guard_rule rol_guild_guard_rules[] = {
    {2002951, NORTH, ROL_GUILD_CLASS(CLASS_ASSASSIN) | ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2003055, SOUTH,
     ROL_GUILD_CLASS(CLASS_WARRIOR) | ROL_GUILD_CLASS(CLASS_BERSERKER) |
         ROL_GUILD_CLASS(CLASS_BLACKGUARD),
     0, true},
    {2003067, NORTH, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2003283, EAST, ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2004128, NORTH, 0, 0, false},
    {2005510, EAST, ROL_GUILD_CLASS(CLASS_WARRIOR), 0, true},
    {2005520, SOUTH, ROL_GUILD_CLASS(CLASS_MONK), 0, true},
    {2005570, EAST, ROL_GUILD_CLASS(CLASS_WIZARD), 0, true},
    {2007669, NORTH, ROL_GUILD_CLASS(CLASS_WARRIOR) | ROL_GUILD_CLASS(CLASS_BLACKGUARD), 0, true},
    {2007817, DOWN, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2007837, WEST, ROL_GUILD_CLASS(CLASS_ASSASSIN) | ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2007844, EAST, ROL_GUILD_CLASS(CLASS_WIZARD) | ROL_GUILD_CLASS(CLASS_SORCERER), 0, true},
    {2007864, WEST, ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2007880, WEST, ROL_GUILD_CLASS(CLASS_NECROMANCER), 0, true},
    {2008014, SOUTH, ROL_GUILD_CLASS(CLASS_WARRIOR), 0, true},
    {2008044, EAST, 0, 0, false},
    {2008046, EAST, 0, 0, false},
    {2008053, WEST, 0, 0, false},
    {2008070, WEST, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2008087, EAST, 0, ROL_GUILD_RACE(RACE_ELF) | ROL_GUILD_RACE(RACE_HALF_ELF), false},
    {2008113, SOUTH, ROL_GUILD_CLASS(CLASS_WIZARD) | ROL_GUILD_CLASS(CLASS_SORCERER), 0, true},
    {2008137, SOUTH, ROL_GUILD_CLASS(CLASS_DRUID), 0, true},
    {2008200, WEST, ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2008305, EAST, ROL_GUILD_CLASS(CLASS_RANGER), 0, true},
    {2008311, SOUTH, ROL_GUILD_CLASS(CLASS_NECROMANCER), 0, true},
    {2008318, NORTH, ROL_GUILD_CLASS(CLASS_BARD), 0, true},
    {2011603, WEST, ROL_GUILD_CLASS(CLASS_WARRIOR), 0, true},
    {2011633, WEST, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2011685, EAST, 0, 0, false},
    {2011812, UP, 0, 0, false},
    {2015314, NORTH, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2015333, NORTH, ROL_GUILD_CLASS(CLASS_WARRIOR), 0, true},
    {2015506, NORTH, ROL_GUILD_CLASS(CLASS_BERSERKER), 0, true},
    {2015660, SOUTH, ROL_GUILD_CLASS(CLASS_WARRIOR), 0, true},
    {2016007, WEST, ROL_GUILD_CLASS(CLASS_WARRIOR), 0, true},
    {2016056, NORTH, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2016145, EAST, ROL_GUILD_CLASS(CLASS_WIZARD) | ROL_GUILD_CLASS(CLASS_SORCERER), 0, true},
    {2016192, NORTH, ROL_GUILD_CLASS(CLASS_WIZARD) | ROL_GUILD_CLASS(CLASS_SORCERER), 0, true},
    {2016283, SOUTH, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2016383, SOUTH, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2016392, SOUTH, ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2016408, NORTH, ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2019950, SOUTH, 0, 0, false},
    {2019951, SOUTH, 0, 0, false},
    {2019954, SOUTH, 0, 0, false},
    {2025001, NORTH, 0, 0, false},
    {2025201, NORTH, 0, 0, false},
    {2034367, SOUTH, ROL_GUILD_CLASS(CLASS_WIZARD) | ROL_GUILD_CLASS(CLASS_SORCERER), 0, true},
    {2034406, WEST, ROL_GUILD_CLASS(CLASS_ASSASSIN) | ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2034406, EAST, ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2050624, WEST, 0, 0, true},
    {2066028, SOUTH, ROL_GUILD_CLASS(CLASS_ROGUE), 0, true},
    {2066065, WEST, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2066078, SOUTH, ROL_GUILD_CLASS(CLASS_WARRIOR), 0, true},
    {2066084, NORTH, ROL_GUILD_CLASS(CLASS_WIZARD) | ROL_GUILD_CLASS(CLASS_SORCERER), 0, true},
    {2066088, EAST, ROL_GUILD_CLASS(CLASS_CLERIC), 0, true},
    {2090847, SOUTH, 0, 0, true},
    {2090849, EAST, 0, 0, true},
};

struct rol_gate_recipe
{
  const char *alias;
  int family_flag;
  int chance;
  int minimum;
  int maximum;
  int cooldown_seconds;
  const char *summons[6];
};

/* These recipes preserve the source aliases, attempt cooldowns, success ranges,
 * and summon families. Recipes whose source branches could summon several
 * independent groups use one bounded mixed group in the target. */
static const struct rol_gate_recipe rol_gate_recipes[] = {
    {"babau", MOB_ROL_DEMON, 40, 1, 3, SECS_PER_MUD_DAY, {"babau", "cambion", NULL}},
    {"balor",
     MOB_ROL_DEMON,
     100,
     1,
     4,
     SECS_PER_MUD_HOUR / 2,
     {"balor", "glabrezu", "hezrou", "marilith", "nalfeshnee", "vrock"}},
    {"bar-lgura", MOB_ROL_DEMON, 36, 1, 3, SECS_PER_MUD_DAY, {"bar-lgura", NULL}},
    {"chasme",
     MOB_ROL_DEMON,
     40,
     1,
     4,
     SECS_PER_MUD_HOUR * 2,
     {"manes", "cambion", "chasme", NULL}},
    {"dretch", MOB_ROL_DEMON, 50, 1, 3, SECS_PER_MUD_DAY, {"dretch", NULL}},
    {"glabrezu", MOB_ROL_DEMON, 50, 1, 1, SECS_PER_MUD_DAY, {"babau", "chasme", "nabassu", NULL}},
    {"hezrou",
     MOB_ROL_DEMON,
     35,
     1,
     4,
     SECS_PER_MUD_HOUR,
     {"balor", "glabrezu", "hezrou", "marilith", "nalfeshnee", "vrock"}},
    {"marilith",
     MOB_ROL_DEMON,
     36,
     1,
     4,
     SECS_PER_MUD_HOUR / 2,
     {"babau", "chasme", "nabassu", "cambion", "dretch", NULL}},
    {"molydeus",
     MOB_ROL_DEMON,
     36,
     1,
     3,
     SECS_PER_MUD_HOUR / 2,
     {"molydeus", "chasme", "babau", NULL}},
    {"nabassu",
     MOB_ROL_DEMON,
     46,
     1,
     4,
     SECS_PER_MUD_HOUR * 2,
     {"nabassu", "cambion", "manes", NULL}},
    {"nalfeshnee", MOB_ROL_DEMON, 50, 1, 3, SECS_PER_MUD_HOUR * 2, {"vrock", "babau", NULL}},
    {"rutterkin",
     MOB_ROL_DEMON,
     50,
     1,
     3,
     SECS_PER_MUD_DAY,
     {"dretch", "manes", "rutterkin", NULL}},
    {"succubus", MOB_ROL_DEMON, 40, 1, 1, SECS_PER_MUD_HOUR * 2, {"balor", NULL}},
    {"incubus", MOB_ROL_DEMON, 40, 1, 1, SECS_PER_MUD_HOUR * 2, {"balor", NULL}},
    {"vrock", MOB_ROL_DEMON, 50, 1, 4, SECS_PER_MUD_DAY, {"nalfeshnee", "manes", NULL}},
    {"abishai", MOB_ROL_DEVIL, 45, 1, 3, SECS_PER_MUD_DAY, {"abishai", "lemure", NULL}},
    {"amnizu", MOB_ROL_DEVIL, 40, 1, 3, SECS_PER_MUD_DAY, {"abishai", "erinyes", NULL}},
    {"barbazu", MOB_ROL_DEVIL, 43, 1, 3, SECS_PER_MUD_DAY, {"abishai", "barbazu", NULL}},
    {"cornugon",
     MOB_ROL_DEVIL,
     60,
     1,
     5,
     SECS_PER_MUD_DAY,
     {"barbazu", "abishai", "cornugon", NULL}},
    {"erinyes", MOB_ROL_DEVIL, 43, 1, 4, SECS_PER_MUD_DAY, {"spinagon", "barbazu", NULL}},
    {"gelugon", MOB_ROL_DEVIL, 60, 1, 3, SECS_PER_MUD_DAY, {"barbazu", "abishai", NULL}},
    {"hamatula", MOB_ROL_DEVIL, 43, 1, 3, SECS_PER_MUD_DAY, {"abishai", "hamatula", NULL}},
    {"osyluth", MOB_ROL_DEVIL, 43, 1, 4, SECS_PER_MUD_DAY, {"nupperibo", "osyluth", NULL}},
    {"fiend",
     MOB_ROL_DEVIL,
     100,
     1,
     2,
     SECS_PER_MUD_HOUR / 2,
     {"amnizu", "cornugon", "gelugon", "abishai", "barbazu", NULL}},
    {"spinagon", MOB_ROL_DEVIL, 36, 1, 3, SECS_PER_MUD_DAY, {"spinagon", NULL}},
    {NULL, 0, 0, 0, 0, 0, {NULL}},
};

static const struct rol_gate_recipe *rol_gate_recipe_for(const struct char_data *ch)
{
  const struct rol_gate_recipe *recipe;

  if (ch == NULL || !IS_NPC(ch) || GET_NAME(ch) == NULL || isname("nogate", GET_NAME(ch)))
    return NULL;

  for (recipe = rol_gate_recipes; recipe->alias != NULL; recipe++)
    if (MOB_FLAGGED(ch, recipe->family_flag) && isname(recipe->alias, GET_NAME(ch)))
      return recipe;

  return NULL;
}

static mob_rnum rol_gate_template(const char *alias, int family_flag)
{
  mob_rnum rnum;
  struct char_data *prototype;

  for (rnum = 0; rnum <= top_of_mobt; rnum++)
  {
    prototype = mob_proto + rnum;
    if (MOB_FLAGGED(prototype, family_flag) && GET_NAME(prototype) != NULL &&
        isname("nogate", GET_NAME(prototype)) && isname(alias, GET_NAME(prototype)))
      return rnum;
  }

  return NOBODY;
}

static void rol_purge_gated_inventory(struct char_data *ch)
{
  struct obj_data *obj;
  int wear;

  while (ch->carrying != NULL)
  {
    obj = ch->carrying;
    obj_from_char(obj);
    extract_obj(obj);
  }
  for (wear = 0; wear < NUM_WEARS; wear++)
    if (GET_EQ(ch, wear) != NULL)
      extract_obj(unequip_char(ch, wear));
}

static void rol_gate_one(struct char_data *ch, const char *alias, int family_flag)
{
  struct char_data *summoned;
  mob_rnum rnum;

  if ((rnum = rol_gate_template(alias, family_flag)) == NOBODY)
  {
    log("SYSERR: RoL gate template '%s' is unavailable for mobile %d", alias, GET_MOB_VNUM(ch));
    return;
  }
  if ((summoned = read_mobile(rnum, REAL)) == NULL)
    return;

  char_to_room(summoned, IN_ROOM(ch));
  summoned->mob_specials.rol_gated_creature = true;
  summoned->mob_specials.rol_gate_expire_at = time(NULL) + (4 * SECS_PER_MUD_HOUR);
  act("With an arcane motion, $n gates in $N!", FALSE, ch, NULL, summoned, TO_ROOM);

  if (!isname("rutterkin", GET_NAME(summoned)))
  {
    if (GROUP(ch) == NULL)
      create_group(ch);
    add_follower(summoned, ch);
    if (GROUP(ch) != NULL && GROUP(summoned) == NULL)
      join_group(summoned, GROUP(ch));
  }

  if (FIGHTING(ch) != NULL && FIGHTING(summoned) == NULL)
    set_fighting(summoned, FIGHTING(ch));
}

static void rol_attempt_planar_gate(struct char_data *ch)
{
  const struct rol_gate_recipe *recipe;
  const char *alias;
  int chance;
  int count;
  int option_count;
  int index;
  time_t now;

  if (ch == NULL || ch->mob_specials.rol_gated_creature ||
      (ch->master != NULL && !IS_NPC(ch->master)))
    return;
  if (rand_number(0, 5) != 0 || (recipe = rol_gate_recipe_for(ch)) == NULL)
    return;

  now = time(NULL);
  if (ch->mob_specials.rol_gate_cooldown_until > now)
    return;
  ch->mob_specials.rol_gate_cooldown_until = now + recipe->cooldown_seconds;

  chance = recipe->chance;
  if (ch->master != NULL)
    chance /= 2;
  if (rand_number(0, 99) >= chance)
    return;

  for (option_count = 0; recipe->summons[option_count] != NULL; option_count++)
    ;
  count = rand_number(recipe->minimum, recipe->maximum);
  count = MIN(count, ROL_GATE_MAX_SUMMONS);
  for (index = 0; index < count; index++)
  {
    alias = recipe->summons[rand_number(0, option_count - 1)];
    rol_gate_one(ch, alias, recipe->family_flag);
  }
}

static obj_rnum rol_umberhulk_claws_template(void)
{
  obj_rnum rnum;

  for (rnum = 0; rnum <= top_of_objt; rnum++)
    if (obj_proto[rnum].name != NULL && strcmp(obj_proto[rnum].name, "claws") == 0)
      return rnum;
  return NOTHING;
}

static void rol_equip_umberhulk_claws(struct char_data *ch)
{
  struct obj_data *claws;
  obj_rnum rnum;

  if (GET_EQ(ch, WEAR_WIELD_1) != NULL || (rnum = rol_umberhulk_claws_template()) == NOTHING)
    return;
  if ((claws = read_object(rnum, REAL)) == NULL)
    return;
  equip_char(ch, claws, WEAR_WIELD_1);
}

bool rol_corpse_devourer_can_consume(const struct obj_data *obj)
{
  if (obj == NULL)
    return false;

  if (GET_OBJ_TYPE(obj) == ITEM_FOOD)
    return true;

  return IS_CORPSE(obj) && GET_OBJ_VAL(obj, 4) == 0;
}

int rol_poison_bite_roll_ceiling(int level)
{
  return MAX(0, 61 - level);
}

int rol_umberhulk_proc_chance(int level)
{
  return MIN(100, MAX(0, (level * 17) / 10));
}

int rol_planar_gate_cooldown_seconds(const struct char_data *ch)
{
  const struct rol_gate_recipe *recipe = rol_gate_recipe_for(ch);

  return recipe != NULL ? recipe->cooldown_seconds : 0;
}

bool rol_conversion_death_retargets_clerics(int vnum)
{
  return vnum == 2053268 || vnum == 2053269 || vnum == 2097003;
}

static bool rol_death_replacement_activity(struct char_data *ch)
{
  struct char_data *candidate;
  struct char_data *cleric = NULL;
  int attacker_count = 0;
  int vnum;

  if (ch == NULL || !IS_NPC(ch) || !AWAKE(ch) || FIGHTING(ch) == NULL ||
      !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return false;
  vnum = GET_MOB_VNUM(ch);
  if (!rol_conversion_death_retargets_clerics(vnum))
    return false;

  for (candidate = world[IN_ROOM(ch)].people; candidate != NULL;
       candidate = candidate->next_in_room)
  {
    if (FIGHTING(candidate) != ch)
      continue;
    attacker_count++;
    if (candidate != FIGHTING(ch) && IS_CLERIC(candidate) && CAN_SEE(ch, candidate))
      cleric = candidate;
  }
  if (attacker_count <= 1 || cleric == NULL)
    return false;

  stop_fighting(ch);
  set_fighting(ch, cleric);
  return true;
}

bool rol_automatic_race_activity(struct char_data *ch)
{
  if (ch == NULL || !IS_NPC(ch))
    return false;

  if (ch->mob_specials.rol_gated_creature && ch->mob_specials.rol_gate_expire_at > 0 &&
      ch->mob_specials.rol_gate_expire_at <= time(NULL))
  {
    act("$n disappears in a cloud of acrid black smoke.", FALSE, ch, NULL, NULL, TO_ROOM);
    rol_purge_gated_inventory(ch);
    extract_char(ch);
    return true;
  }

  if (MOB_FLAGGED(ch, MOB_ROL_UMBERHULK))
    rol_equip_umberhulk_claws(ch);

  if (rol_death_replacement_activity(ch))
    return true;

  return false;
}

void rol_automatic_race_combat_turn(struct char_data *ch)
{
  struct char_data *victim;
  int effect;

  if (ch == NULL || !IS_NPC(ch) || (victim = FIGHTING(ch)) == NULL)
    return;

  if (MOB_FLAGGED(ch, MOB_ROL_DEMON) || MOB_FLAGGED(ch, MOB_ROL_DEVIL))
    rol_attempt_planar_gate(ch);

  if (!MOB_FLAGGED(ch, MOB_ROL_UMBERHULK) ||
      rand_number(0, 100) > rol_umberhulk_proc_chance(GET_LEVEL(ch)))
    return;

  effect = rand_number(0, 8);
  if (effect < 2 && !IS_PET(victim))
  {
    act("$n focuses $s many eyes on $N, clouding $S thoughts!", TRUE, ch, NULL, victim, TO_NOTVICT);
    act("$n focuses $s many eyes on you, clouding your thoughts!", TRUE, ch, NULL, victim, TO_VICT);
    call_magic(ch, victim, NULL, SPELL_CONFUSION, 0, GET_LEVEL(ch), CAST_INNATE);
  }
  else
  {
    act("$n snaps at $N with crushing mandibles!", TRUE, ch, NULL, victim, TO_NOTVICT);
    hit(ch, victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);
  }
}

static const struct rol_alert_profile *rol_alert_profile_for(int caller_vnum)
{
  size_t index;

  for (index = 0; index < sizeof(rol_alert_profiles) / sizeof(rol_alert_profiles[0]); index++)
    if (rol_alert_profiles[index].caller_vnum == caller_vnum)
      return &rol_alert_profiles[index];

  return NULL;
}

static const struct rol_ambient_mobile_profile *rol_ambient_profile_for(int mobile_vnum)
{
  size_t index;

  for (index = 0;
       index < sizeof(rol_ambient_mobile_profiles) / sizeof(rol_ambient_mobile_profiles[0]);
       index++)
    if (rol_ambient_mobile_profiles[index].mobile_vnum == mobile_vnum)
      return &rol_ambient_mobile_profiles[index];

  return NULL;
}

int rol_waterdeep_ambient_roll_sides(int mobile_vnum)
{
  const struct rol_ambient_mobile_profile *profile = rol_ambient_profile_for(mobile_vnum);

  if (profile == NULL)
    return 0;
  if (profile->profile_id == ROL_AMBIENT_CASINO_ONE)
    return 7;
  if (profile->profile_id == ROL_AMBIENT_CASINO_TWO)
    return 6;

  return 5;
}

bool rol_waterdeep_ambient_room_allows(int mobile_vnum, int room_vnum)
{
  const struct rol_ambient_mobile_profile *profile = rol_ambient_profile_for(mobile_vnum);

  if (profile == NULL)
    return false;

  return profile->profile_id != ROL_AMBIENT_MERCHANT_TWO || room_vnum == 2005400;
}

bool rol_waterdeep_ambient_fighting_allows(int mobile_vnum, bool fighting)
{
  const struct rol_ambient_mobile_profile *profile = rol_ambient_profile_for(mobile_vnum);

  if (profile == NULL)
    return false;

  if (profile->profile_id == ROL_AMBIENT_WATERDEEP_GUARD_ONE ||
      profile->profile_id == ROL_AMBIENT_WATERDEEP_GUARD_TWO)
    return !fighting;

  return true;
}

const char *rol_waterdeep_ambient_message(int mobile_vnum, int roll, int message_index,
                                          bool *speech)
{
  const struct rol_ambient_mobile_profile *profile = rol_ambient_profile_for(mobile_vnum);
  size_t index;

  if (profile == NULL || message_index < 0)
    return NULL;

  for (index = 0; index < sizeof(rol_ambient_actions) / sizeof(rol_ambient_actions[0]); index++)
  {
    if (rol_ambient_actions[index].profile_id != profile->profile_id ||
        rol_ambient_actions[index].roll != roll)
      continue;
    if (message_index-- != 0)
      continue;
    if (speech != NULL)
      *speech = rol_ambient_actions[index].speech;
    return rol_ambient_actions[index].message;
  }

  return NULL;
}

int rol_waterdeep_ambient(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *speaker = me;
  const char *message;
  int message_index;
  int roll;
  int sides;
  bool speech;

  (void)argument;

  if (speaker == NULL && cmd == 0)
    speaker = ch;
  if (speaker == NULL || cmd != 0 || !IS_NPC(speaker) || IN_ROOM(speaker) == NOWHERE ||
      GET_POS(speaker) < POS_STANDING ||
      !rol_waterdeep_ambient_room_allows(GET_MOB_VNUM(speaker), GET_ROOM_VNUM(IN_ROOM(speaker))) ||
      !rol_waterdeep_ambient_fighting_allows(GET_MOB_VNUM(speaker), FIGHTING(speaker) != NULL) ||
      (sides = rol_waterdeep_ambient_roll_sides(GET_MOB_VNUM(speaker))) == 0)
    return FALSE;

  roll = dice(2, sides);
  for (message_index = 0; (message = rol_waterdeep_ambient_message(GET_MOB_VNUM(speaker), roll,
                                                                   message_index, &speech)) != NULL;
       message_index++)
  {
    if (speech)
      do_say(speaker, message, 0, 0);
    else
      act(message, TRUE, speaker, NULL, NULL, TO_ROOM);
  }

  return FALSE;
}

const char *rol_alert_message(int caller_vnum)
{
  const struct rol_alert_profile *profile = rol_alert_profile_for(caller_vnum);

  return profile != NULL ? profile->message : NULL;
}

bool rol_alert_helper_matches(int caller_vnum, int helper_vnum)
{
  const struct rol_alert_profile *profile = rol_alert_profile_for(caller_vnum);
  size_t index;

  if (profile == NULL)
    return false;

  for (index = 0; index < profile->helper_count; index++)
    if (profile->helper_vnums[index] == helper_vnum)
      return true;

  return false;
}

static bool rol_alert_helper_can_answer(struct char_data *helper, struct char_data *caller,
                                        struct char_data *victim)
{
  int distance;

  if (helper == NULL || caller == NULL || victim == NULL || helper == caller || helper == victim ||
      !IS_NPC(helper) || !rol_alert_helper_matches(GET_MOB_VNUM(caller), GET_MOB_VNUM(helper)) ||
      IN_ROOM(helper) == NOWHERE || IN_ROOM(caller) == NOWHERE ||
      GET_ROOM_ZONE(IN_ROOM(helper)) != GET_ROOM_ZONE(IN_ROOM(caller)) || !AWAKE(helper) ||
      FIGHTING(helper) != NULL || HUNTING(helper) != NULL || AFF_FLAGGED(helper, AFF_CHARM) ||
      MOB_FLAGGED(helper, MOB_NOKILL) || !ok_damage_shopkeeper(victim, helper))
    return false;

  distance = count_rooms_between(IN_ROOM(helper), IN_ROOM(caller));
  return distance >= 0 && distance <= 100;
}

static int rol_alert_combat_turn(struct char_data *caller)
{
  const struct rol_alert_profile *profile;
  struct char_data *helper;
  struct char_data *victim;
  const char *victim_name;
  char alert[MAX_STRING_LENGTH];
  char message[MAX_STRING_LENGTH];

  if (caller == NULL || !IS_NPC(caller) || IN_ROOM(caller) == NOWHERE ||
      (profile = rol_alert_profile_for(GET_MOB_VNUM(caller))) == NULL)
    return FALSE;

  victim = FIGHTING(caller);
  if (victim == NULL)
  {
    caller->mob_specials.rol_alert_fired = false;
    return FALSE;
  }
  if (caller->mob_specials.rol_alert_fired || ROOM_FLAGGED(IN_ROOM(caller), ROOM_SOUNDPROOF) ||
      !AWAKE(caller) || IS_CASTING(caller) || AFF_FLAGGED(caller, AFF_SILENCED) ||
      AFF_FLAGGED(caller, AFF_PARALYZED))
    return FALSE;

  victim_name = CAN_SEE(caller, victim) ? GET_NAME(victim) : "Someone";
  snprintf(message, sizeof(message), profile->message, victim_name);
  snprintf(alert, sizeof(alert), "\r\n%.256s shouts, '%.1024s'\r\n", GET_NAME(caller), message);
  send_to_zone(alert, GET_ROOM_ZONE(IN_ROOM(caller)));

  for (helper = character_list; helper != NULL; helper = helper->next)
    if (rol_alert_helper_can_answer(helper, caller, victim))
      HUNTING(helper) = victim;

  caller->mob_specials.rol_alert_fired = true;
  return TRUE;
}

int rol_alert_caller(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *caller = me;

  (void)argument;

  if (caller == NULL && cmd == 0)
    caller = ch;
  if (cmd != 0)
    return FALSE;

  return rol_alert_combat_turn(caller);
}

bool rol_yggdrasil_vnum(int vnum)
{
  return vnum >= 2062800 && vnum <= 2062804;
}

int rol_yggdrasil_release_move(int current_move)
{
  return current_move / 2;
}

static long rol_yggdrasil_tenderness(const struct char_data *candidate)
{
  long tenderness = GET_MAX_HIT(candidate);

  if (IS_CLERIC(candidate))
    tenderness *= 75;
  else if (IS_WIZARD(candidate) || IS_SORCERER(candidate) || IS_PSI_TYPE(candidate) ||
           IS_BARD(candidate))
    tenderness *= 50;
  else if (IS_ROGUE(candidate))
    tenderness *= -1;
  else if (IS_WARRIOR(candidate))
    tenderness *= -10;

  if (!AFF_FLAGGED(candidate, AFF_CHARM))
    tenderness *= 2;

  return tenderness;
}

static struct char_data *rol_yggdrasil_juiciest(struct char_data *caller)
{
  struct char_data *candidate;
  struct char_data *tank;
  struct char_data *juiciest = NULL;
  long best_tenderness = LONG_MIN;
  long tenderness;

  if (caller == NULL || IN_ROOM(caller) == NOWHERE || (tank = FIGHTING(caller)) == NULL)
    return NULL;

  for (candidate = world[IN_ROOM(caller)].people; candidate != NULL;
       candidate = candidate->next_in_room)
  {
    if (IS_NPC(candidate) || GET_LEVEL(candidate) >= LVL_IMMORT || !CAN_SEE(caller, candidate) ||
        (FIGHTING(candidate) != caller &&
         (GROUP(candidate) == NULL || GROUP(tank) == NULL || GROUP(candidate) != GROUP(tank))))
      continue;

    tenderness = rol_yggdrasil_tenderness(candidate);
    if (juiciest == NULL || tenderness > best_tenderness)
    {
      juiciest = candidate;
      best_tenderness = tenderness;
    }
  }

  return juiciest;
}

EVENTFUNC(event_rol_yggdrasil_release)
{
  struct mud_event_data *event = event_obj;
  struct char_data *victim;

  if (event == NULL || (victim = event->pStruct) == NULL)
    return 0;

  act("You break free of the entangling branches!", FALSE, victim, NULL, victim, TO_CHAR);
  act("$n breaks free of the entangling branches!", FALSE, victim, NULL, victim, TO_ROOM);
  if (affected_by_spell(victim, SPELL_ENTANGLE))
    affect_from_char(victim, SPELL_ENTANGLE);
  GET_MOVE(victim) = rol_yggdrasil_release_move(GET_MOVE(victim));
  return 0;
}

int rol_yggdrasil_branch(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct affected_type affect;
  struct char_data *caller = me;
  struct char_data *victim;
  int duration;

  (void)argument;

  if (caller == NULL && cmd == 0)
    caller = ch;
  if (caller == NULL || cmd != 0 || !IS_NPC(caller) || !rol_yggdrasil_vnum(GET_MOB_VNUM(caller)) ||
      (victim = FIGHTING(caller)) == NULL)
    return FALSE;

  if (rand_number(0, 1) == 0)
  {
    struct char_data *juiciest = rol_yggdrasil_juiciest(caller);

    if (juiciest != NULL)
      victim = juiciest;
  }
  if (rand_number(0, 1) != 0 || AFF_FLAGGED(victim, AFF_ENTANGLED) ||
      char_has_mud_event(victim, eROL_YGGDRASIL_RELEASE) != NULL)
    return FALSE;

  if (savingthrow(caller, victim, SAVING_REFL, -10, CAST_INNATE, GET_LEVEL(caller), TRANSMUTATION))
  {
    act("$N breaks free of the entangling branches!", FALSE, caller, NULL, victim, TO_CHAR);
    act("You break free of the entangling branches!", FALSE, caller, NULL, victim, TO_VICT);
    act("$N breaks free of the entangling branches!", FALSE, caller, NULL, victim, TO_NOTVICT);
    return FALSE;
  }

  act("$N is secured by the entangling branches!", FALSE, caller, NULL, victim, TO_CHAR);
  act("You are secured by branches and cannot escape!", FALSE, caller, NULL, victim, TO_VICT);
  act("$N is secured by the entangling branches!", FALSE, caller, NULL, victim, TO_NOTVICT);
  new_affect(&affect);
  affect.spell = SPELL_ENTANGLE;
  affect.duration = -1;
  SET_BIT_AR(affect.bitvector, AFF_ENTANGLED);
  affect_to_char(victim, &affect);

  duration = rand_number(4, 12);
  NEW_EVENT(eROL_YGGDRASIL_RELEASE, victim, NULL, PULSE_VIOLENCE * duration);
  return FALSE;
}

static const struct rol_death_profile *rol_death_profile_for(int vnum)
{
  size_t high = sizeof(rol_death_profiles) / sizeof(rol_death_profiles[0]);
  size_t low = 0;
  size_t middle;

  while (low < high)
  {
    middle = low + (high - low) / 2;
    if (rol_death_profiles[middle].mobile_vnum < vnum)
      low = middle + 1;
    else
      high = middle;
  }
  if (low < sizeof(rol_death_profiles) / sizeof(rol_death_profiles[0]) &&
      rol_death_profiles[low].mobile_vnum == vnum)
    return &rol_death_profiles[low];

  return NULL;
}

const char *rol_conversion_death_message(int vnum)
{
  const struct rol_death_profile *profile = rol_death_profile_for(vnum);

  return profile != NULL ? profile->message : NULL;
}

bool rol_conversion_death_suppresses_corpse(int vnum)
{
  const struct rol_death_profile *profile = rol_death_profile_for(vnum);

  return profile != NULL && profile->suppress_corpse;
}

int rol_conversion_death_replacement_vnum(int vnum)
{
  const struct rol_death_profile *profile = rol_death_profile_for(vnum);

  return profile != NULL ? profile->replacement_vnum : 0;
}

int rol_conversion_death_object_vnum(int vnum)
{
  const struct rol_death_profile *profile = rol_death_profile_for(vnum);

  return profile != NULL ? profile->object_vnum : 0;
}

static void rol_death_transfer_to_mobile(struct char_data *source, struct char_data *replacement)
{
  struct obj_data *item;
  struct obj_data *next_item;
  int wear;

  for (item = source->carrying; item != NULL; item = next_item)
  {
    next_item = item->next_content;
    obj_from_char(item);
    obj_to_char(item, replacement);
  }
  for (wear = 0; wear < NUM_WEARS; wear++)
    if (GET_EQ(source, wear) != NULL)
      equip_char(replacement, unequip_char(source, wear), wear);
}

static void rol_add_permanent_affect(struct char_data *ch, int spell, int affect_flag)
{
  struct affected_type affect;

  if (affected_by_spell(ch, spell))
    return;
  new_affect(&affect);
  affect.spell = spell;
  affect.duration = -1;
  SET_BIT_AR(affect.bitvector, affect_flag);
  affect_to_char(ch, &affect);
}

static void rol_death_apply_transmuter_spellup(struct char_data *ch)
{
  rol_add_permanent_affect(ch, SPELL_ENDURE_ELEMENTS, AFF_ELEMENT_PROT);
  rol_add_permanent_affect(ch, SPELL_DETECT_INVIS, AFF_DETECT_INVIS);
  SET_BIT_AR(AFF_FLAGS(ch), AFF_FARSEE);
  rol_add_permanent_affect(ch, SPELL_SENSE_LIFE, AFF_SENSE_LIFE);
  rol_add_permanent_affect(ch, SPELL_INFRAVISION, AFF_INFRAVISION);
  rol_add_permanent_affect(ch, SPELL_FIRE_SHIELD, AFF_FSHIELD);
  rol_add_permanent_affect(ch, SPELL_GLOBE_OF_INVULN, AFF_GLOBE_OF_INVULN);
  rol_add_permanent_affect(ch, SPELL_HASTE, AFF_HASTE);
  rol_add_permanent_affect(ch, SPELL_PROT_FROM_GOOD, AFF_PROTECT_GOOD);
  rol_add_permanent_affect(ch, SPELL_VAMPIRIC_TOUCH, AFF_VAMPIRIC_TOUCH);
  rol_add_permanent_affect(ch, SPELL_FLY, AFF_FLYING);
}

static void rol_death_replace_mobile(struct char_data *ch, const struct rol_death_profile *profile)
{
  struct char_data *replacement;

  if (profile->replacement_vnum <= 0 ||
      (replacement = read_mobile(profile->replacement_vnum, VIRTUAL)) == NULL)
  {
    log("SYSERR: RoL death replacement %d for mobile %d is unavailable", profile->replacement_vnum,
        GET_MOB_VNUM(ch));
    return;
  }

  char_to_room(replacement, IN_ROOM(ch));
  GET_MOB_LOADROOM(replacement) = IN_ROOM(ch);
  rol_death_transfer_to_mobile(ch, replacement);
  if (profile->effect == ROL_DEATH_EFFECT_REPLACE_SPELLUP)
    rol_death_apply_transmuter_spellup(replacement);
  if (profile->mobile_vnum >= 2090812 && profile->mobile_vnum <= 2090866)
  {
    MEMORY(replacement) = MEMORY(ch);
    MEMORY(ch) = NULL;
  }
}

static void rol_death_drop_object(struct char_data *ch, int object_vnum)
{
  struct obj_data *obj;

  if (object_vnum <= 0 || (obj = read_object(object_vnum, VIRTUAL)) == NULL)
  {
    log("SYSERR: RoL death object %d for mobile %d is unavailable", object_vnum, GET_MOB_VNUM(ch));
    return;
  }
  obj_to_room(obj, IN_ROOM(ch));
}

static void rol_death_return_to_master(struct char_data *ch)
{
  struct char_data *master = ch->master;
  struct obj_data *item;
  struct obj_data *next_item;
  struct obj_data *money;
  int wear;

  if (master != NULL)
  {
    send_to_char(master, "A shadowy hole opens and deposits your servant's possessions into your "
                         "inventory.\r\n");
    GET_GOLD(master) += GET_GOLD(ch);
    GET_GOLD(ch) = 0;
    for (wear = 0; wear < NUM_WEARS; wear++)
      if (GET_EQ(ch, wear) != NULL)
        obj_to_char(unequip_char(ch, wear), master);
    for (item = ch->carrying; item != NULL; item = next_item)
    {
      next_item = item->next_content;
      obj_from_char(item);
      obj_to_char(item, master);
    }
    return;
  }

  if (GET_GOLD(ch) > 0)
  {
    money = create_money(GET_GOLD(ch));
    GET_GOLD(ch) = 0;
    obj_to_room(money, IN_ROOM(ch));
  }
  for (wear = 0; wear < NUM_WEARS; wear++)
    if (GET_EQ(ch, wear) != NULL)
      obj_to_room(unequip_char(ch, wear), IN_ROOM(ch));
  for (item = ch->carrying; item != NULL; item = next_item)
  {
    next_item = item->next_content;
    obj_from_char(item);
    obj_to_room(item, IN_ROOM(ch));
  }
}

static void rol_death_spore_poison(struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next_victim;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    act("The gas dissipates harmlessly.", FALSE, ch, NULL, NULL, TO_ROOM);
    return;
  }
  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next_victim)
  {
    next_victim = victim->next_in_room;
    if (victim != ch)
      call_magic(ch, victim, NULL, SPELL_POISON, 0, 24, CAST_INNATE);
  }
}

static void rol_death_heat_blind(struct char_data *victim)
{
  struct affected_type affect;

  if (AFF_FLAGGED(victim, AFF_BLIND) || !IS_DARK(IN_ROOM(victim)) ||
      GET_LEVEL(victim) >= LVL_IMMORT ||
      (!AFF_FLAGGED(victim, AFF_INFRAVISION) && !AFF_FLAGGED(victim, AFF_DARKVISION)))
    return;
  send_to_char(victim, "Aaarrrggghhh! The heat blinds you!\r\n");
  new_affect(&affect);
  affect.spell = SPELL_BLINDNESS;
  affect.duration = rand_number(1, 4);
  SET_BIT_AR(affect.bitvector, AFF_BLIND);
  affect_to_char(victim, &affect);
}

static void rol_death_balor_burst(struct char_data *ch)
{
  struct char_data *victim;
  struct char_data *next_victim;
  int damage_amount;

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = next_victim)
  {
    next_victim = victim->next_in_room;
    if (victim == ch || GET_LEVEL(victim) >= LVL_IMMORT)
      continue;
    rol_death_heat_blind(victim);
    damage_amount = AFF_FLAGGED(victim, AFF_ELEMENT_PROT) ? 150 : 250;
    GET_HIT(victim) -= damage_amount;
    update_pos(victim);
    if (GET_HIT(victim) < -10)
      die(victim, ch);
  }
}

static void rol_death_stone_crumble(struct char_data *ch, int object_vnum)
{
  struct obj_data *pile;
  struct obj_data *item;
  struct obj_data *next_item;
  struct obj_data *money;
  int wear;

  if (object_vnum <= 0 || (pile = read_object(object_vnum, VIRTUAL)) == NULL)
  {
    log("SYSERR: RoL stone-crumble object %d for mobile %d is unavailable", object_vnum,
        GET_MOB_VNUM(ch));
    return;
  }
  if (GET_OBJ_TYPE(pile) != ITEM_CONTAINER)
  {
    log("SYSERR: RoL stone-crumble object %d is not a container", object_vnum);
    extract_obj(pile);
    return;
  }
  for (item = ch->carrying; item != NULL; item = next_item)
  {
    next_item = item->next_content;
    obj_from_char(item);
    obj_to_obj(item, pile);
  }
  for (wear = 0; wear < NUM_WEARS; wear++)
    if (GET_EQ(ch, wear) != NULL)
      obj_to_obj(unequip_char(ch, wear), pile);
  if (GET_GOLD(ch) > 0)
  {
    money = create_money(GET_GOLD(ch));
    obj_to_obj(money, pile);
  }
  obj_to_room(pile, IN_ROOM(ch));
}

static void rol_apply_death_effect(struct char_data *ch, const struct rol_death_profile *profile)
{
  switch (profile->effect)
  {
  case ROL_DEATH_EFFECT_REPLACE:
  case ROL_DEATH_EFFECT_REPLACE_SPELLUP:
    rol_death_replace_mobile(ch, profile);
    break;
  case ROL_DEATH_EFFECT_DROP_OBJECT:
    rol_death_drop_object(ch, profile->object_vnum);
    break;
  case ROL_DEATH_EFFECT_RETURN_TO_MASTER:
    rol_death_return_to_master(ch);
    break;
  case ROL_DEATH_EFFECT_SHADOW_DARKNESS:
    call_magic(ch, NULL, NULL, SPELL_DARKNESS, 0, 20, CAST_INNATE);
    break;
  case ROL_DEATH_EFFECT_SPORE_POISON:
    rol_death_spore_poison(ch);
    break;
  case ROL_DEATH_EFFECT_BALOR_BURST:
    rol_death_balor_burst(ch);
    break;
  case ROL_DEATH_EFFECT_STONE_CRUMBLE:
    rol_death_stone_crumble(ch, profile->object_vnum);
    break;
  case ROL_DEATH_EFFECT_NONE:
    break;
  }
}

bool rol_handle_conjured_death(struct char_data *ch)
{
  const struct rol_death_profile *profile;
  const char *message = NULL;

  if (ch == NULL || !IS_NPC(ch))
    return false;

  profile = rol_death_profile_for(GET_MOB_VNUM(ch));
  if (profile != NULL)
  {
    if (profile->message != NULL)
      act(profile->message, FALSE, ch, NULL, NULL, TO_ROOM);
    if (profile->secondary_message != NULL)
      act(profile->secondary_message, FALSE, ch, NULL, NULL, TO_ROOM);
    rol_apply_death_effect(ch, profile);
    return profile->suppress_corpse;
  }

  if (MOB_FLAGGED(ch, MOB_ROL_BLACK_VAPOR_DEATH))
    message = "$n turns into a black vapor and seeps into the ground.";
  else if (MOB_FLAGGED(ch, MOB_ROL_FADE_FAMILIAR))
    message = "$n slowly fades away into the netherworld...";
  else if (MOB_FLAGGED(ch, MOB_ROL_FADE_MOUNT))
    message = "$n vanishes in a puff of white smoke!";
  else if (MOB_FLAGGED(ch, MOB_ROL_FADE_MONSTER))
    message = "$n disappears in a flash of bright light!";
  else if (MOB_FLAGGED(ch, MOB_ROL_TOTEM_SPIRIT))
  {
    message = rol_totem_spirit_death_message(GET_MOB_VNUM(ch));
    if (message == NULL)
      message = "$n quickly fades back into the spirit world...";
  }

  if (message == NULL)
    return false;

  act(message, FALSE, ch, NULL, NULL, TO_ROOM);
  return true;
}

static bool rol_breath_ready(struct char_data *ch)
{
  if (ch == NULL || !IS_NPC(ch) || FIGHTING(ch) == NULL)
    return false;

  ch->mob_specials.proc_fired = (ch->mob_specials.proc_fired + 1) % 4;
  return ch->mob_specials.proc_fired == 0;
}

static int rol_breath_weapon(struct char_data *ch, int spell)
{
  rol_alert_combat_turn(ch);
  if (!rol_breath_ready(ch))
    return FALSE;

  call_magic(ch, NULL, NULL, spell, 0, GET_LEVEL(ch), CAST_INNATE);
  return FALSE;
}

static int rol_breath_attack(struct char_data *ch, int damage_type, const char *self_message,
                             const char *victim_message, const char *room_message)
{
  struct char_data *victim;
  struct spec_damage_result result;
  int dice_count;

  if (!rol_breath_ready(ch) || (victim = FIGHTING(ch)) == NULL)
    return FALSE;

  act(self_message, FALSE, ch, NULL, victim, TO_CHAR);
  act(victim_message, FALSE, ch, NULL, victim, TO_VICT);
  act(room_message, FALSE, ch, NULL, victim, TO_NOTVICT);
  dice_count = MAX(1, GET_LEVEL(ch) / 2);
  result = spec_damage_current_target(ch, victim, dice(dice_count, 6), -1, damage_type, FALSE);
  return result.status == SPEC_DAMAGE_TARGET_INVALIDATED;
}

int rol_breath_weapon_fire(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_weapon(ch, SPELL_FIRE_BREATHE);
}

int rol_breath_weapon_cold(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_weapon(ch, SPELL_FROST_BREATHE);
}

int rol_breath_weapon_acid(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_weapon(ch, SPELL_ACID_BREATHE);
}

int rol_breath_weapon_gas(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_weapon(ch, SPELL_GAS_BREATHE);
}

int rol_breath_weapon_lightning(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_weapon(ch, SPELL_LIGHTNING_BREATHE);
}

int rol_breath_attack_acid(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_attack(ch, DAM_ACID, "You spray \tLacid\tn at $N!",
                           "$n sprays \tLacid\tn at you!", "$n sprays \tLacid\tn at $N!");
}

int rol_breath_attack_lightning(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);
  return rol_breath_attack(ch, DAM_ELECTRIC, "You breathe \tBlightning\tn at $N!",
                           "$n breathes \tBlightning\tn at you!",
                           "$n breathes \tBlightning\tn at $N!");
}

int rol_corpse_devourer(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct obj_data *obj;
  struct obj_data *contained;
  struct obj_data *next;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || cmd || !AWAKE(ch) || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;

  for (obj = world[IN_ROOM(ch)].contents; obj != NULL; obj = obj->next_content)
  {
    if (!rol_corpse_devourer_can_consume(obj))
      continue;

    if (IS_CORPSE(obj))
    {
      for (contained = obj->contains; contained != NULL; contained = next)
      {
        next = contained->next_content;
        obj_from_obj(contained);
        obj_to_room(contained, IN_ROOM(ch));
      }
    }

    act("$n savagely devours $o.", FALSE, ch, obj, NULL, TO_ROOM);
    extract_obj(obj);
    return TRUE;
  }

  return FALSE;
}

int rol_poison_bite(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || cmd || (victim = FIGHTING(ch)) == NULL)
    return FALSE;

  if (spec_context_validate_combat_target(ch, victim, true) != SPEC_CONTEXT_VALID)
    return FALSE;

  if (rand_number(0, rol_poison_bite_roll_ceiling(GET_LEVEL(ch))) != 0)
    return FALSE;

  act("$n bites $N!", TRUE, ch, NULL, victim, TO_NOTVICT);
  act("$n bites you!", TRUE, ch, NULL, victim, TO_VICT);
  call_magic(ch, victim, NULL, SPELL_POISON, 0, GET_LEVEL(ch), CAST_WEAPON_SPELL);
  return TRUE;
}

static void rol_thief_steal(struct char_data *ch, struct char_data *victim)
{
  int gold;

  if (IS_NPC(victim) || GET_LEVEL(victim) >= LVL_IMMORT || ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
    return;

  if (AWAKE(victim) && rand_number(0, GET_LEVEL(ch)) == 0)
  {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, NULL, victim, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, NULL, victim, TO_NOTVICT);
    return;
  }

  gold = (GET_GOLD(victim) * rand_number(1, 10)) / 100;
  if (gold > 0)
  {
    increase_gold(ch, gold);
    decrease_gold(victim, gold);
  }
}

int rol_thief(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *victim;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || cmd || GET_POS(ch) != POS_STANDING || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = victim->next_in_room)
    if (!IS_NPC(victim) && GET_LEVEL(victim) < LVL_IMMORT)
      rol_thief_steal(ch, victim);

  return TRUE;
}

bool rol_bloodstone_portal_survives(int current_hit, int hit_loss)
{
  return current_hit - MAX(0, hit_loss) >= -10;
}

bool rol_portal_door_race_allows(bool rejects_good, int race)
{
  return rejects_good ? !rol_race_is_good(race) : !rol_race_is_evil(race);
}

static const struct rol_travel_portal_profile *rol_travel_portal_profile_for(int object_vnum)
{
  size_t index;

  for (index = 0;
       index < sizeof(rol_travel_portal_profiles) / sizeof(rol_travel_portal_profiles[0]); index++)
    if (rol_travel_portal_profiles[index].object_vnum == object_vnum)
      return &rol_travel_portal_profiles[index];

  return NULL;
}

int rol_travel_portal_destination_slot(int object_vnum, int roll)
{
  const struct rol_travel_portal_profile *profile = rol_travel_portal_profile_for(object_vnum);

  if (profile == NULL || profile->fixed_destination_vnum >= 0)
    return -1;
  if (profile->kind == ROL_TRAVEL_PORTAL_ELF_GATE)
    return roll >= 0 && roll < 4 ? roll : -1;
  return 0;
}

int rol_travel_portal_fixed_destination(int object_vnum)
{
  const struct rol_travel_portal_profile *profile = rol_travel_portal_profile_for(object_vnum);

  return profile != NULL ? profile->fixed_destination_vnum : -1;
}

int rol_travel_portal_reward_vnum(int object_vnum)
{
  const struct rol_travel_portal_profile *profile = rol_travel_portal_profile_for(object_vnum);

  return profile != NULL ? profile->reward_vnum : -1;
}

bool rol_travel_portal_actor_allowed(int object_vnum, const struct char_data *ch)
{
  const struct rol_travel_portal_profile *profile = rol_travel_portal_profile_for(object_vnum);

  if (profile == NULL || ch == NULL)
    return false;

  switch (profile->kind)
  {
  case ROL_TRAVEL_PORTAL_ELF_GATE:
    return GET_RACE(ch) == RACE_ELF && GET_LEVEL(ch) >= 20;
  case ROL_TRAVEL_PORTAL_SHAMAN_SPORES:
    return !IS_NPC(ch) && CLASS_LEVEL(ch, CLASS_CLERIC) > 0;
  case ROL_TRAVEL_PORTAL_ILLUSION_FOUNTAIN:
    return !IS_NPC(ch) && CLASS_LEVEL(ch, CLASS_WIZARD) > 0;
  default:
    return true;
  }
}

static struct obj_data *rol_travel_portal_selected_object(struct char_data *ch,
                                                          struct obj_data *obj,
                                                          const char *argument,
                                                          bitvector_t locations)
{
  struct char_data *dummy = NULL;
  struct obj_data *selected = NULL;
  char name[MAX_INPUT_LENGTH];

  one_argument(argument, name, sizeof(name));
  if (!*name || !generic_find(name, locations, ch, &dummy, &selected) || selected != obj)
    return NULL;
  return selected;
}

static room_rnum rol_travel_portal_destination(const struct rol_travel_portal_profile *profile,
                                               const struct obj_data *obj)
{
  int destination_vnum;
  int slot;

  if (profile->fixed_destination_vnum >= 0)
    destination_vnum = profile->fixed_destination_vnum;
  else
  {
    slot = rol_travel_portal_destination_slot(
        profile->object_vnum, profile->kind == ROL_TRAVEL_PORTAL_ELF_GATE ? rand_number(0, 3) : 0);
    if (slot < 0)
      return NOWHERE;
    destination_vnum = GET_OBJ_VAL(obj, slot);
  }

  return real_room(destination_vnum);
}

static bool rol_travel_portal_destination_allows(struct char_data *ch, room_rnum destination,
                                                 bool arena_parity)
{
  if (!VALID_ROOM_RNUM(destination) || !valid_mortal_tele_dest(ch, destination, false))
    return false;
  return !arena_parity ||
         ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA) == ROOM_FLAGGED(destination, ROOM_ARENA);
}

static void rol_travel_portal_move(struct char_data *ch, struct obj_data *obj,
                                   room_rnum destination, enum rol_travel_portal_kind kind)
{
  switch (kind)
  {
  case ROL_TRAVEL_PORTAL_DIMENSIONAL_FOLD:
    act("$n enters the dimensional fold and reappears elsewhere...", FALSE, ch, obj, NULL, TO_ROOM);
    send_to_char(ch, "You enter the dimensional fold and reappear elsewhere...\r\n");
    break;
  case ROL_TRAVEL_PORTAL_WATERDEEP:
    send_to_char(ch, "You step into the portal.\r\n"
                     "Multi-colored lights flash and dance all about you!\r\n"
                     "You feel yourself stretch and twist across an extra-dimensional plane, "
                     "then...\r\n");
    act("$n steps through the portal.", FALSE, ch, obj, NULL, TO_ROOM);
    break;
  case ROL_TRAVEL_PORTAL_ELF_GATE:
    act("As you step into $p, there is a blinding flash of light!", FALSE, ch, obj, NULL, TO_CHAR);
    send_to_char(ch, "You are ripped through a dark and star-filled void; pain sears through\r\n"
                     "your body! When you again open your eyes, you are elsewhere...\r\n");
    act("$n wades into $p.", FALSE, ch, obj, NULL, TO_ROOM);
    act("$n slowly fades out of existence.", FALSE, ch, NULL, NULL, TO_ROOM);
    break;
  case ROL_TRAVEL_PORTAL_SHAMAN_SPORES:
    send_to_char(
        ch, "As you inhale the mushroom spores your vision blurs and a warm numb feeling\r\n"
            "overwhelms your mind. Color and sound blend into one strange sense and you\r\n"
            "feel your soul leaving your body with a slightly painful stretching sensation.\r\n"
            "Your vision fades to black and then blazes outward in a million vivid textures.\r\n");
    act("As $n inhales the spores, the flames from the fire roar higher and higher, and\r\n"
        "swirling mists blanket the area. When they clear $n is gone.",
        FALSE, ch, obj, NULL, TO_ROOM);
    act("$n slowly fades out of existence.", FALSE, ch, NULL, NULL, TO_ROOM);
    break;
  case ROL_TRAVEL_PORTAL_BLIP:
    if (obj->carried_by == ch)
    {
      act("$p in $n's hands suddenly glows brightly!", FALSE, ch, obj, NULL, TO_ROOM);
      act("$p in your hands suddenly glows brightly!", FALSE, ch, obj, NULL, TO_CHAR);
    }
    else
      act("$p suddenly glows brightly!", FALSE, ch, obj, NULL, TO_ROOM);
    act("$n slowly fades out of existence.", FALSE, ch, NULL, NULL, TO_ROOM);
    break;
  case ROL_TRAVEL_PORTAL_ILLUSION_FOUNTAIN:
    act("$n wades into the fountain's waters and disappears!", TRUE, ch, obj, NULL, TO_ROOM);
    send_to_char(ch, "You wade into the illusory waters and appear elsewhere!\r\n");
    break;
  }

  char_from_room(ch);
  char_to_room(ch, destination);

  switch (kind)
  {
  case ROL_TRAVEL_PORTAL_DIMENSIONAL_FOLD:
    act("$n steps out of a dimensional fold.", FALSE, ch, NULL, NULL, TO_ROOM);
    break;
  case ROL_TRAVEL_PORTAL_ELF_GATE:
  case ROL_TRAVEL_PORTAL_SHAMAN_SPORES:
  case ROL_TRAVEL_PORTAL_BLIP:
  case ROL_TRAVEL_PORTAL_ILLUSION_FOUNTAIN:
    act("$n slowly fades into existence.", TRUE, ch, NULL, NULL, TO_ROOM);
    break;
  case ROL_TRAVEL_PORTAL_WATERDEEP:
    break;
  }
}

static void rol_travel_portal_consume(struct char_data *ch, struct obj_data *obj)
{
  if (obj->carried_by == ch)
    act("$p in your hands shatters and the pieces disappear in smoke.", TRUE, ch, obj, NULL,
        TO_CHAR);
  else
    act("$p shatters and the pieces disappear in smoke.", TRUE, ch, obj, NULL, TO_ROOM);
  extract_obj(obj);
}

int rol_travel_portal(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct obj_data *obj = me;
  const struct rol_travel_portal_profile *profile;
  struct obj_data *reward;
  room_rnum destination;
  const char *look_argument;
  bitvector_t locations;
  bool looking;

  if (ch == NULL || obj == NULL || argument == NULL || !cmd || !VALID_ROOM_RNUM(IN_ROOM(ch)) ||
      (profile = rol_travel_portal_profile_for(GET_OBJ_VNUM(obj))) == NULL)
    return FALSE;

  looking = profile->kind == ROL_TRAVEL_PORTAL_DIMENSIONAL_FOLD && CMD_IS("look");
  if (!looking)
  {
    if (profile->kind == ROL_TRAVEL_PORTAL_SHAMAN_SPORES)
    {
      if (!CMD_IS("use"))
        return FALSE;
    }
    else if (!CMD_IS("enter"))
      return FALSE;
  }

  look_argument = argument;
  if (looking)
  {
    skip_spaces_c(&look_argument);
    if (strn_cmp(look_argument, "in ", 3) != 0)
      return FALSE;
    look_argument += 3;
  }

  locations = profile->kind == ROL_TRAVEL_PORTAL_SHAMAN_SPORES ? FIND_OBJ_INV
              : profile->kind == ROL_TRAVEL_PORTAL_BLIP
                  ? FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM
                  : FIND_OBJ_ROOM;
  if (rol_travel_portal_selected_object(ch, obj, look_argument, locations) == NULL)
    return FALSE;

  if ((profile->kind == ROL_TRAVEL_PORTAL_SHAMAN_SPORES ||
       profile->kind == ROL_TRAVEL_PORTAL_BLIP) &&
      GET_OBJ_VAL(obj, 2) == 0)
  {
    send_to_char(ch, "Nothing happens.\r\n");
    return TRUE;
  }

  destination = rol_travel_portal_destination(profile, obj);
  if (looking)
  {
    if (!VALID_ROOM_RNUM(destination))
    {
      send_to_char(ch, "The portal leads nowhere. Please tell a staff member.\r\n");
      return TRUE;
    }
    act("You peer into $p and see...", FALSE, ch, obj, NULL, TO_CHAR);
    look_at_room_number(ch, 0, destination);
    return TRUE;
  }

  if (profile->kind == ROL_TRAVEL_PORTAL_ELF_GATE &&
      !rol_travel_portal_actor_allowed(profile->object_vnum, ch))
  {
    if (GET_RACE(ch) != RACE_ELF)
      send_to_char(ch, "You are not of true elf blood; you may not enter this gate.\r\n");
    else
      send_to_char(ch, "The gate flares briefly, but refuses to transport someone of your "
                       "level.\r\n");
    return FALSE;
  }

  if (profile->kind == ROL_TRAVEL_PORTAL_ILLUSION_FOUNTAIN &&
      !rol_travel_portal_actor_allowed(profile->object_vnum, ch))
    return FALSE;

  if (!rol_travel_portal_destination_allows(ch, destination,
                                            profile->kind == ROL_TRAVEL_PORTAL_DIMENSIONAL_FOLD ||
                                                profile->kind == ROL_TRAVEL_PORTAL_SHAMAN_SPORES))
  {
    if (profile->kind == ROL_TRAVEL_PORTAL_DIMENSIONAL_FOLD)
      send_to_char(ch, "A strong force pushes you back!\r\n");
    else
      send_to_char(ch, "Nothing happens.\r\n");
    return TRUE;
  }

  if (profile->kind == ROL_TRAVEL_PORTAL_SHAMAN_SPORES &&
      !rol_travel_portal_actor_allowed(profile->object_vnum, ch))
  {
    act("$n snorts $p up $s nose, then sneezes explosively, covering the room in a thin layer "
        "of speckled slime.",
        TRUE, ch, obj, NULL, TO_ROOM);
    send_to_char(ch, "You inhale the spores and suddenly your face feels like it is on fire!\r\n"
                     "You sneeze violently and stagger...\r\n");
    attach_mud_event(new_mud_event(eSTUNNED, ch, NULL), 10 * PULSE_VIOLENCE);
    extract_obj(obj);
    return TRUE;
  }

  rol_travel_portal_move(ch, obj, destination, profile->kind);

  if (profile->kind == ROL_TRAVEL_PORTAL_WATERDEEP && GET_LEVEL(ch) < LVL_IMMORT)
  {
    GET_HIT(ch) = MAX(0, GET_HIT(ch) - MAX(0, GET_OBJ_VAL(obj, 1)));
    update_pos(ch);
  }
  else if (profile->kind == ROL_TRAVEL_PORTAL_SHAMAN_SPORES)
    extract_obj(obj);
  else if (profile->kind == ROL_TRAVEL_PORTAL_BLIP)
  {
    reward = read_object(profile->reward_vnum, VIRTUAL);
    if (reward == NULL)
    {
      send_to_char(ch, "The portal's reward is unavailable. Please tell a staff member.\r\n");
      log("SYSERR: RoL travel portal object %d cannot load reward %d", profile->object_vnum,
          profile->reward_vnum);
      return TRUE;
    }
    obj_to_char(reward, ch);
    if (GET_OBJ_VAL(obj, 2) > 0 && --GET_OBJ_VAL(obj, 2) == 0)
      rol_travel_portal_consume(ch, obj);
  }

  return TRUE;
}

int rol_portal_door(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct obj_data *obj = me;
  struct obj_data *selected;
  const char *name_argument = argument;
  room_rnum destination;
  char name[MAX_INPUT_LENGTH];
  bool looking;

  if (ch == NULL || obj == NULL || argument == NULL || !cmd || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;

  looking = CMD_IS("look");
  if (!looking && !CMD_IS("enter"))
    return FALSE;
  if (looking)
  {
    skip_spaces_c(&name_argument);
    if (strn_cmp(name_argument, "in ", 3) != 0)
      return FALSE;
    name_argument += 3;
  }

  one_argument(name_argument, name, sizeof(name));
  if (!*name)
    return FALSE;
  selected = get_obj_in_list_vis(ch, name, NULL, world[IN_ROOM(ch)].contents);
  if (selected != obj)
    return FALSE;

  destination = real_room(GET_OBJ_VAL(obj, 0));
  if (!VALID_ROOM_RNUM(destination))
  {
    send_to_char(ch, "The portal leads nowhere. Please tell a staff member.\r\n");
    log("SYSERR: RoL portal door object %d has invalid destination %d", GET_OBJ_VNUM(obj),
        GET_OBJ_VAL(obj, 0));
    return TRUE;
  }
  if (looking)
  {
    act("You peer into $p and see...", FALSE, ch, obj, NULL, TO_CHAR);
    look_at_room_number(ch, 0, destination);
    return TRUE;
  }

  if ((!IS_NPC(ch) && GET_LEVEL(ch) < 20) ||
      ROOM_FLAGGED(IN_ROOM(ch), ROOM_ARENA) != ROOM_FLAGGED(destination, ROOM_ARENA) ||
      (GET_LEVEL(ch) < LVL_IMMORT &&
       !rol_portal_door_race_allows(GET_OBJ_VAL(obj, 3) != 0, GET_RACE(ch))) ||
      !valid_mortal_tele_dest(ch, destination, false))
  {
    send_to_char(ch, "A strong force pushes you back!\r\n");
    return TRUE;
  }

  act("$p suddenly glows brightly!", FALSE, ch, obj, NULL, TO_ROOM);
  act("$n enters $p and disappears among the mist.", FALSE, ch, obj, NULL, TO_ROOM);
  char_from_room(ch);
  send_to_char(ch, "You enter the portal and reappear elsewhere...\r\n");
  char_to_room(ch, destination);
  act("$n steps out of a shimmering portal.", FALSE, ch, NULL, NULL, TO_ROOM);
  look_at_room(ch, 0);
  return TRUE;
}

int rol_bloodstone_portal(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct obj_data *obj = me;
  struct obj_data *entered;
  room_rnum destination;
  char name[MAX_INPUT_LENGTH];
  int hit_loss;

  if (ch == NULL || obj == NULL || argument == NULL || !cmd || !CMD_IS("enter") || !AWAKE(ch) ||
      !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;

  one_argument(argument, name, sizeof(name));
  if (!*name)
    return FALSE;
  entered = get_obj_in_list_vis(ch, name, NULL, world[IN_ROOM(ch)].contents);
  if (entered != obj)
    return FALSE;

  destination = real_room(GET_OBJ_VAL(obj, 0));
  if (!VALID_ROOM_RNUM(destination))
  {
    send_to_char(ch, "The portal leads nowhere. Please tell a staff member.\r\n");
    log("SYSERR: RoL Bloodstone portal object %d has invalid destination %d", GET_OBJ_VNUM(obj),
        GET_OBJ_VAL(obj, 0));
    return TRUE;
  }
  if (!valid_mortal_tele_dest(ch, destination, false))
  {
    send_to_char(ch, "An unseen force pushes you back!\r\n");
    return TRUE;
  }

  act("$p suddenly glows brightly!", FALSE, ch, obj, NULL, TO_ROOM);
  act("$n enters $p and fades into the ether.", TRUE, ch, obj, NULL, TO_ROOM);
  send_to_char(ch, "Your mind and body are overcome with seizures of pain!\r\n"
                   "In the blink of an eye you are whisked away...\r\n");
  char_from_room(ch);
  send_to_char(ch, "You enter the portal and reappear elsewhere.\r\n");
  char_to_room(ch, destination);
  act("$n slowly fades into view.", TRUE, ch, NULL, NULL, TO_ROOM);

  if (GET_LEVEL(ch) >= LVL_IMMORT)
    return TRUE;

  hit_loss = rand_number(1, 20);
  if (!rol_bloodstone_portal_survives(GET_HIT(ch), hit_loss))
  {
    send_to_char(ch, "The stress of the magic proves too much for you!\r\n");
    raw_kill(ch, ch);
    return TRUE;
  }

  GET_HIT(ch) -= hit_loss;
  GET_MOVE(ch) = MAX(0, GET_MOVE(ch) - rand_number(1, 30));
  update_pos(ch);
  send_to_char(ch, "You feel weakened by your passage through the portal.\r\n");
  return TRUE;
}

int rol_magic_pool(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct obj_data *obj = me;
  room_rnum destination;
  char name[MAX_INPUT_LENGTH];
  int damage_amount;

  if (ch == NULL || obj == NULL || argument == NULL || !cmd || !CMD_IS("enter"))
    return FALSE;

  one_argument(argument, name, sizeof(name));
  if (!*name || obj->name == NULL || !isname(name, obj->name))
    return FALSE;

  destination = real_room(GET_OBJ_VAL(obj, 0));
  if (!VALID_ROOM_RNUM(destination))
  {
    send_to_char(ch, "The pool leads nowhere. Please tell a staff member.\r\n");
    log("SYSERR: RoL magic pool object %d has invalid destination %d", GET_OBJ_VNUM(obj),
        GET_OBJ_VAL(obj, 0));
    return TRUE;
  }

  act("As you step into $p, there is a blinding flash of light!", FALSE, ch, obj, NULL, TO_CHAR);
  send_to_char(ch, "You are ripped through a dark and star-filled void; pain sears through\r\n"
                   "your body. When you open your eyes, you are elsewhere...\r\n");
  act("$n wades into $p.", FALSE, ch, obj, NULL, TO_ROOM);

  damage_amount = MAX(0, GET_OBJ_VAL(obj, 1));
  if (GET_LEVEL(ch) < LVL_IMMORT)
    GET_HIT(ch) = MAX(0, GET_HIT(ch) - damage_amount);

  act("$n slowly fades out of existence.", FALSE, ch, NULL, NULL, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, destination);
  act("$n slowly fades into existence.", FALSE, ch, NULL, NULL, TO_ROOM);
  return TRUE;
}

static room_rnum rol_random_room_in_zone(zone_rnum zone)
{
  room_rnum room;
  int room_count = 0;
  int selected;

  if (zone == NOWHERE || zone > top_of_zone_table)
    return NOWHERE;

  for (room = 0; room <= top_of_world; room++)
    if (world[room].zone == zone)
      room_count++;

  if (room_count == 0)
    return NOWHERE;

  selected = rand_number(0, room_count - 1);
  for (room = 0; room <= top_of_world; room++)
  {
    if (world[room].zone != zone)
      continue;
    if (selected-- == 0)
      return room;
  }

  return NOWHERE;
}

int rol_auto_distributor(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct room_data *room = me;
  room_rnum destination;
  zone_rnum zone;

  UNUSED(cmd);
  UNUSED(argument);

  if (ch == NULL || room == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;
  if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT)
    return FALSE;

  zone = world[IN_ROOM(ch)].zone;
  destination = rol_random_room_in_zone(zone);
  if (!VALID_ROOM_RNUM(destination))
  {
    send_to_char(ch, "The distributing magic fails. Please tell a staff member.\r\n");
    log("SYSERR: RoL auto distributor room %d has no valid destination in zone %d", room->number,
        zone);
    return TRUE;
  }

  act("$n slowly fades out of existence.", FALSE, ch, NULL, NULL, TO_ROOM);
  char_from_room(ch);
  if (ZONE_FLAGGED(world[destination].zone, ZONE_WILDERNESS))
  {
    X_LOC(ch) = world[destination].coords[0];
    Y_LOC(ch) = world[destination].coords[1];
  }
  char_to_room(ch, destination);
  act("$n enters.", FALSE, ch, NULL, NULL, TO_ROOM);
  return TRUE;
}

static bool rol_guild_guard_has_class(const struct char_data *ch, unsigned long long class_mask)
{
  int class_id;

  if (ch == NULL || class_mask == 0)
    return false;

  if (IS_NPC(ch))
    return GET_CLASS(ch) >= 0 && GET_CLASS(ch) < 64 &&
           (class_mask & ROL_GUILD_CLASS(GET_CLASS(ch))) != 0;

  for (class_id = 0; class_id < MAX_CLASSES && class_id < 64; class_id++)
    if ((class_mask & ROL_GUILD_CLASS(class_id)) != 0 && CLASS_LEVEL(ch, class_id) > 0)
      return true;

  return false;
}

bool rol_class_guild_allows(const struct char_data *ch, enum rol_guild_family family)
{
  if (ch == NULL || IS_NPC(ch))
    return false;

  switch (family)
  {
  case ROL_GUILD_FAMILY_MAGE:
    return CLASS_LEVEL(ch, CLASS_WIZARD) > 0 || CLASS_LEVEL(ch, CLASS_SORCERER) > 0 ||
           CLASS_LEVEL(ch, CLASS_SUMMONER) > 0 || CLASS_LEVEL(ch, CLASS_WARLOCK) > 0 ||
           CLASS_LEVEL(ch, CLASS_NECROMANCER) > 0;
  case ROL_GUILD_FAMILY_THIEF:
    return CLASS_LEVEL(ch, CLASS_ROGUE) > 0 || CLASS_LEVEL(ch, CLASS_BARD) > 0 ||
           CLASS_LEVEL(ch, CLASS_ASSASSIN) > 0 || CLASS_LEVEL(ch, CLASS_DUELIST) > 0 ||
           CLASS_LEVEL(ch, CLASS_SHADOW_DANCER) > 0 || CLASS_LEVEL(ch, CLASS_ARCANE_SHADOW) > 0;
  case ROL_GUILD_FAMILY_WARRIOR:
    return CLASS_LEVEL(ch, CLASS_WARRIOR) > 0 || CLASS_LEVEL(ch, CLASS_MONK) > 0 ||
           CLASS_LEVEL(ch, CLASS_BERSERKER) > 0 || CLASS_LEVEL(ch, CLASS_PALADIN) > 0 ||
           CLASS_LEVEL(ch, CLASS_RANGER) > 0 || CLASS_LEVEL(ch, CLASS_BLACKGUARD) > 0 ||
           CLASS_LEVEL(ch, CLASS_WEAPON_MASTER) > 0 ||
           CLASS_LEVEL(ch, CLASS_STALWART_DEFENDER) > 0 ||
           CLASS_LEVEL(ch, CLASS_ARCANE_ARCHER) > 0 || CLASS_LEVEL(ch, CLASS_SHIFTER) > 0 ||
           CLASS_LEVEL(ch, CLASS_SACRED_FIST) > 0 || CLASS_LEVEL(ch, CLASS_ELDRITCH_KNIGHT) > 0 ||
           CLASS_LEVEL(ch, CLASS_SPELLSWORD) > 0 || CLASS_LEVEL(ch, CLASS_KNIGHT_OF_SOLAMNIA) > 0 ||
           CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_THORN) > 0 ||
           CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_SKULL) > 0 ||
           CLASS_LEVEL(ch, CLASS_KNIGHT_OF_THE_LILY) > 0 || CLASS_LEVEL(ch, CLASS_DRAGONRIDER) > 0;
  case ROL_GUILD_FAMILY_CLERIC:
    return CLASS_LEVEL(ch, CLASS_CLERIC) > 0 || CLASS_LEVEL(ch, CLASS_DRUID) > 0 ||
           CLASS_LEVEL(ch, CLASS_INQUISITOR) > 0;
  case ROL_GUILD_FAMILY_BARD:
    return CLASS_LEVEL(ch, CLASS_BARD) > 0;
  default:
    return false;
  }
}

bool rol_waterdeep_guild_allows(int room_vnum, const struct char_data *ch)
{
  if (ch == NULL || IS_NPC(ch))
    return false;

  switch (room_vnum)
  {
  case 2005505:
    return CLASS_LEVEL(ch, CLASS_PALADIN) > 0;
  case 2005512:
    return CLASS_LEVEL(ch, CLASS_WARRIOR) > 0;
  case 2005524:
    return CLASS_LEVEL(ch, CLASS_MONK) > 0;
  case 2005537:
    return CLASS_LEVEL(ch, CLASS_BARD) > 0;
  case 2005544:
    return CLASS_LEVEL(ch, CLASS_RANGER) > 0;
  case 2005568:
    return CLASS_LEVEL(ch, CLASS_DRUID) > 0;
  case 2005581:
  case 2003044:
    return rol_class_guild_allows(ch, ROL_GUILD_FAMILY_MAGE);
  case 2003073:
    return rol_class_guild_allows(ch, ROL_GUILD_FAMILY_CLERIC);
  case 2003061:
    return rol_class_guild_allows(ch, ROL_GUILD_FAMILY_WARRIOR);
  case 2003289:
  case 2002956:
    return CLASS_LEVEL(ch, CLASS_ROGUE) > 0;
  default:
    return false;
  }
}

static int rol_class_guild_room(struct char_data *ch, void *me, int cmd, const char *argument,
                                enum rol_guild_family family)
{
  if (ch == NULL)
    return FALSE;

  if (IS_NPC(ch) || cmd == 0 || (!CMD_IS("practice") && !CMD_IS("train") && !CMD_IS("boosts")))
    return guild(ch, me, cmd, argument);

  if (!rol_class_guild_allows(ch, family))
  {
    send_to_char(ch, "You cannot practice here!\r\n");
    return TRUE;
  }

  return guild(ch, me, cmd, argument);
}

int rol_mage_guild_room(struct char_data *ch, void *me, int cmd, const char *argument)
{
  return rol_class_guild_room(ch, me, cmd, argument, ROL_GUILD_FAMILY_MAGE);
}

int rol_thief_guild_room(struct char_data *ch, void *me, int cmd, const char *argument)
{
  return rol_class_guild_room(ch, me, cmd, argument, ROL_GUILD_FAMILY_THIEF);
}

int rol_warrior_guild_room(struct char_data *ch, void *me, int cmd, const char *argument)
{
  return rol_class_guild_room(ch, me, cmd, argument, ROL_GUILD_FAMILY_WARRIOR);
}

int rol_cleric_guild_room(struct char_data *ch, void *me, int cmd, const char *argument)
{
  return rol_class_guild_room(ch, me, cmd, argument, ROL_GUILD_FAMILY_CLERIC);
}

int rol_bard_guild_room(struct char_data *ch, void *me, int cmd, const char *argument)
{
  return rol_class_guild_room(ch, me, cmd, argument, ROL_GUILD_FAMILY_BARD);
}

int rol_waterdeep_guild_room(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct room_data *room = me;

  if (ch == NULL)
    return FALSE;

  if (IS_NPC(ch) || cmd == 0 || (!CMD_IS("practice") && !CMD_IS("train") && !CMD_IS("boosts")))
    return guild(ch, me, cmd, argument);

  if (room == NULL || !rol_waterdeep_guild_allows(room->number, ch))
  {
    send_to_char(ch, "You cannot practice here!\r\n");
    return TRUE;
  }

  return guild(ch, me, cmd, argument);
}

bool rol_guild_guard_allows(int room_vnum, int direction, const struct char_data *ch)
{
  const struct rol_guild_guard_rule *rule;
  size_t rule_index;

  for (rule_index = 0;
       rule_index < sizeof(rol_guild_guard_rules) / sizeof(rol_guild_guard_rules[0]); rule_index++)
  {
    rule = &rol_guild_guard_rules[rule_index];
    if (rule->room_vnum != room_vnum || rule->direction != direction)
      continue;

    if (rule->class_mask != 0)
      return rol_guild_guard_has_class(ch, rule->class_mask);
    if (rule->race_mask != 0)
      return ch != NULL && GET_RACE(ch) >= 0 && GET_RACE(ch) < 64 &&
             (rule->race_mask & ROL_GUILD_RACE(GET_RACE(ch))) != 0;
    return false;
  }

  return true;
}

bool rol_guild_guard_protects(int room_vnum)
{
  size_t rule_index;

  for (rule_index = 0;
       rule_index < sizeof(rol_guild_guard_rules) / sizeof(rol_guild_guard_rules[0]); rule_index++)
    if (rol_guild_guard_rules[rule_index].room_vnum == room_vnum &&
        rol_guild_guard_rules[rule_index].protects)
      return true;

  return false;
}

int rol_guild_guard_passage_destination(int room_vnum, int direction)
{
  switch (room_vnum)
  {
  case 2002951:
    return direction == NORTH ? 2002952 : 0;
  case 2003055:
    return direction == SOUTH ? 2003056 : 0;
  case 2003067:
    return direction == NORTH ? 2003068 : 0;
  case 2003283:
    return direction == EAST ? 2003284 : 0;
  case 2005510:
    return direction == EAST ? 2005511 : 0;
  case 2005520:
    return direction == SOUTH ? 2005521 : 0;
  case 2005570:
    return direction == EAST ? 2005571 : 0;
  case 2007669:
    return direction == NORTH ? 2007670 : 0;
  case 2007817:
    return direction == DOWN ? 2007818 : 0;
  case 2007837:
    return direction == WEST ? 2007843 : 0;
  case 2007844:
    return direction == EAST ? 2007845 : 0;
  case 2007864:
    return direction == WEST ? 2007865 : 0;
  case 2007880:
    return direction == WEST ? 2007881 : 0;
  default:
    return 0;
  }
}

bool rol_guild_guard_trips_rejected(int room_vnum, int direction)
{
  return room_vnum == 2002951 && direction == NORTH;
}

static room_rnum rol_guild_guard_teleport_destination(struct char_data *victim)
{
  room_rnum room;
  room_rnum selected = NOWHERE;
  zone_rnum zone;
  int eligible = 0;

  if (victim == NULL || !VALID_ROOM_RNUM(IN_ROOM(victim)))
    return NOWHERE;

  zone = world[IN_ROOM(victim)].zone;
  for (room = 0; room <= top_of_world; room++)
  {
    if (room == IN_ROOM(victim) || world[room].zone != zone ||
        !valid_mortal_tele_dest(victim, room, true))
      continue;

    eligible++;
    if (rand_number(1, eligible) == 1)
      selected = room;
  }

  return selected;
}

static void rol_guild_guard_stop_victim_combat(struct char_data *victim)
{
  struct char_data *fighter;
  struct char_data *next;

  if (victim == NULL)
    return;

  if (FIGHTING(victim) != NULL)
    stop_fighting(victim);

  for (fighter = combat_list; fighter != NULL; fighter = next)
  {
    next = fighter->next_fighting;
    if (FIGHTING(fighter) == victim)
      stop_fighting(fighter);
  }
}

static int rol_guild_guard_protection(struct char_data *guard, struct char_data *victim)
{
  room_rnum destination;
  long loss;

  if (guard == NULL || victim == NULL || IS_NPC(victim) ||
      spec_context_validate_combat_target(guard, victim, true) != SPEC_CONTEXT_VALID)
    return FALSE;

  act("$n says, 'Begone from here, outlaw! None may attack guild guardians!'", FALSE, guard, NULL,
      victim, TO_ROOM);
  act("$n presses a small metal pin on $s chest, which flares with brilliant blue light!", FALSE,
      guard, NULL, victim, TO_ROOM);
  send_to_char(victim, "A wrenching pain drains your life force away!\r\n");

  loss = MIN((long)GET_LEVEL(victim) * 5000L, MAX(0L, GET_EXP(victim) - 2L));
  GET_EXP(victim) -= loss;

  call_magic(guard, victim, NULL, SPELL_DISPEL_MAGIC, 0, 60, CAST_INNATE);
  call_magic(guard, victim, NULL, SPELL_CURSE, 0, 60, CAST_INNATE);
  if (!affected_by_spell(victim, SPELL_POISON))
    call_magic(guard, victim, NULL, SPELL_POISON, 0, 120, CAST_INNATE);
  call_magic(guard, victim, NULL, SPELL_BLINDNESS, 0, 60, CAST_INNATE);
  call_magic(guard, victim, NULL, SPELL_SLOW, 0, 60, CAST_INNATE);

  if (GET_POS(victim) <= POS_DEAD || !VALID_ROOM_RNUM(IN_ROOM(victim)))
    return TRUE;

  GET_HIT(victim) = 1;
  update_pos(victim);
  destination = rol_guild_guard_teleport_destination(victim);
  rol_guild_guard_stop_victim_combat(victim);

  if (!VALID_ROOM_RNUM(destination))
    return TRUE;

  act("$n slowly fades out of existence.", FALSE, victim, NULL, NULL, TO_ROOM);
  char_from_room(victim);
  if (ZONE_FLAGGED(world[destination].zone, ZONE_WILDERNESS))
  {
    X_LOC(victim) = world[destination].coords[0];
    Y_LOC(victim) = world[destination].coords[1];
  }
  char_to_room(victim, destination);
  act("$n slowly fades into existence.", FALSE, victim, NULL, NULL, TO_ROOM);
  look_at_room(victim, 0);
  entry_memory_mtrigger(victim);
  greet_mtrigger(victim, -1);
  greet_memory_mtrigger(victim);
  return TRUE;
}

int rol_guild_guard(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *guard = me;
  room_rnum destination;
  int current_room_vnum;
  int direction;
  int passage_vnum;

  UNUSED(argument);

  if (guard == NULL || !IS_NPC(guard) || !VALID_ROOM_RNUM(IN_ROOM(guard)) ||
      GET_MOB_LOADROOM(guard) != IN_ROOM(guard))
    return FALSE;

  current_room_vnum = GET_ROOM_VNUM(IN_ROOM(guard));
  if (cmd == 0)
    return FALSE;

  if (ch == NULL || complete_cmd_info == NULL || !IS_MOVE(cmd))
    return FALSE;
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_GUARD))
    return FALSE;

  direction = complete_cmd_info[cmd].subcmd;
  if ((!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT) ||
      rol_guild_guard_allows(current_room_vnum, direction, ch))
  {
    passage_vnum = rol_guild_guard_passage_destination(current_room_vnum, direction);
    if (passage_vnum == 0)
      return FALSE;

    destination = real_room(passage_vnum);
    if (!VALID_ROOM_RNUM(destination) || !valid_mortal_tele_dest(ch, destination, false))
    {
      send_to_char(ch, "The guarded passage leads nowhere. Please tell a staff member.\r\n");
      log("SYSERR: RoL guild guard in room %d has invalid passage destination %d",
          current_room_vnum, passage_vnum);
      return TRUE;
    }

    act("$n steps aside and ushers you through the guarded passage.", FALSE, guard, NULL, ch,
        TO_VICT);
    act("$n steps aside and ushers $N through the guarded passage.", FALSE, guard, NULL, ch,
        TO_NOTVICT);
    char_from_room(ch);
    if (ZONE_FLAGGED(world[destination].zone, ZONE_WILDERNESS))
    {
      X_LOC(ch) = world[destination].coords[0];
      Y_LOC(ch) = world[destination].coords[1];
    }
    char_to_room(ch, destination);
    act("$n arrives through the guarded passage.", FALSE, ch, NULL, NULL, TO_ROOM);
    look_at_room(ch, 0);
    entry_memory_mtrigger(ch);
    greet_mtrigger(ch, -1);
    greet_memory_mtrigger(ch);
    return TRUE;
  }

  act("$n humiliates you, and blocks your way.", FALSE, guard, NULL, ch, TO_VICT);
  act("$n humiliates $N, and blocks $S way.", FALSE, guard, NULL, ch, TO_NOTVICT);
  if (rol_guild_guard_trips_rejected(current_room_vnum, direction))
    GET_POS(ch) = POS_SITTING;
  return TRUE;
}

int rol_guild_guard_typed(struct spec_event_context *context)
{
  struct char_data *guard;
  int current_room_vnum;

  if (context == NULL || context->owner_type != SPEC_OWNER_MOBILE)
    return FALSE;

  guard = context->owner;
  if (guard == NULL || !IS_NPC(guard) || !VALID_ROOM_RNUM(IN_ROOM(guard)) ||
      GET_MOB_LOADROOM(guard) != IN_ROOM(guard))
    return FALSE;

  switch (context->event)
  {
  case SPEC_EVENT_COMMAND:
    return rol_guild_guard(context->actor, guard, context->command, context->argument);
  case SPEC_EVENT_MOBILE_ACTIVITY:
    current_room_vnum = GET_ROOM_VNUM(IN_ROOM(guard));
    if (rol_guild_guard_protects(current_room_vnum) && FIGHTING(guard) != NULL &&
        !IS_NPC(FIGHTING(guard)))
      return FALSE;
    return rol_state_periodic(guard, guard, 0, context->argument);
  case SPEC_EVENT_MOBILE_COMBAT_TURN:
    current_room_vnum = GET_ROOM_VNUM(IN_ROOM(guard));
    if (rol_guild_guard_protects(current_room_vnum) && FIGHTING(guard) != NULL)
      return rol_guild_guard_protection(guard, FIGHTING(guard));
    return FALSE;
  default:
    return FALSE;
  }
}

int rol_major_beholder_eye_spell(int eye)
{
  static const int eye_spells[ROL_MAJOR_BEHOLDER_EYES] = {
      SPELL_FIREBALL,
      SPELL_ACID_ARROW,
      SPELL_SLOW,
      SPELL_RAY_OF_ENFEEBLEMENT,
      PSIONIC_WITHER,
      SPELL_DISPEL_MAGIC,
      SPELL_PRISMATIC_SPRAY,
      SPELL_HOLD_MONSTER,
      SPELL_HARM,
      SPELL_FINGER_OF_DEATH,
  };

  if (eye < 0 || eye >= ROL_MAJOR_BEHOLDER_EYES)
    return -1;
  return eye_spells[eye];
}

int rol_major_beholder_eye_cooldown(int state, int eye)
{
  unsigned int shift;

  if (eye < 0 || eye >= ROL_MAJOR_BEHOLDER_EYES)
    return -1;
  shift = (unsigned int)eye * ROL_MAJOR_BEHOLDER_COOLDOWN_BITS;
  return (int)(((unsigned int)state >> shift) & ROL_MAJOR_BEHOLDER_COOLDOWN_MASK);
}

static int rol_major_beholder_set_cooldown(int state, int eye, int rounds)
{
  unsigned int encoded;
  unsigned int shift;

  if (eye < 0 || eye >= ROL_MAJOR_BEHOLDER_EYES)
    return state;

  shift = (unsigned int)eye * ROL_MAJOR_BEHOLDER_COOLDOWN_BITS;
  encoded = (unsigned int)state & ~(ROL_MAJOR_BEHOLDER_COOLDOWN_MASK << shift);
  encoded |= ((unsigned int)MIN(ROL_MAJOR_BEHOLDER_COOLDOWN_ROUNDS, MAX(0, rounds)) << shift);
  return (int)encoded;
}

int rol_major_beholder_advance_cooldowns(int state, unsigned int fired_eye_mask)
{
  int cooldown;
  int eye;

  for (eye = 0; eye < ROL_MAJOR_BEHOLDER_EYES; eye++)
  {
    cooldown = rol_major_beholder_eye_cooldown(state, eye);
    if ((fired_eye_mask & (1U << eye)) != 0)
      cooldown = ROL_MAJOR_BEHOLDER_COOLDOWN_ROUNDS;
    else if (cooldown > 0)
      cooldown--;
    state = rol_major_beholder_set_cooldown(state, eye, cooldown);
  }
  return state;
}

static struct char_data *rol_major_beholder_target(struct char_data *ch)
{
  struct char_data *target;
  int target_count = 0;

  target = npc_find_target(ch, &target_count);
  if (target == NULL)
    target = FIGHTING(ch);

  if (target != NULL && IS_PET(target) && target->master != NULL &&
      IN_ROOM(target->master) == IN_ROOM(target))
    target = target->master;

  if (spec_context_validate_combat_target(ch, target, false) != SPEC_CONTEXT_VALID)
    return NULL;
  return target;
}

static bool rol_major_beholder_mass_dispel(struct char_data *ch)
{
  struct char_data *target;
  struct char_data *next;
  bool cast = false;

  for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (target == ch || (IS_NPC(target) && !IS_PET(target)))
      continue;
    call_magic(ch, target, NULL, SPELL_DISPEL_MAGIC, 0, GET_LEVEL(ch), CAST_INNATE);
    cast = true;
  }
  return cast;
}

static bool rol_major_beholder_cast_eye(struct char_data *ch, struct char_data *target, int eye)
{
  static const char *ordinals[ROL_MAJOR_BEHOLDER_EYES] = {
      "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth", "ninth", "tenth",
  };
  char message[MAX_INPUT_LENGTH];

  snprintf(message, sizeof(message), "$n fixes $s %s eyestalk upon $N!", ordinals[eye]);
  act(message, FALSE, ch, NULL, target, TO_NOTVICT);
  snprintf(message, sizeof(message), "$n fixes $s %s eyestalk upon you!", ordinals[eye]);
  act(message, FALSE, ch, NULL, target, TO_VICT);

  if (eye == 5)
    return rol_major_beholder_mass_dispel(ch);

  call_magic(ch, target, NULL, rol_major_beholder_eye_spell(eye), 0, GET_LEVEL(ch), CAST_INNATE);
  if (eye == 3 && spec_context_validate_combat_target(ch, target, false) == SPEC_CONTEXT_VALID)
    call_magic(ch, target, NULL, SPELL_FEEBLEMIND, 0, GET_LEVEL(ch), CAST_INNATE);
  return true;
}

int rol_major_beholder(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *target;
  bool fired = false;
  int eye;
  int state;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || !IS_NPC(ch) || cmd || FIGHTING(ch) == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;

  state = rol_major_beholder_advance_cooldowns(ch->mob_specials.proc_fired, 0);
  for (eye = 0; eye < ROL_MAJOR_BEHOLDER_EYES; eye++)
  {
    if (rol_major_beholder_eye_cooldown(state, eye) != 0 || rand_number(0, 2) != 0)
      continue;
    if ((target = rol_major_beholder_target(ch)) == NULL)
      break;
    if (rol_major_beholder_cast_eye(ch, target, eye))
    {
      state = rol_major_beholder_set_cooldown(state, eye, ROL_MAJOR_BEHOLDER_COOLDOWN_ROUNDS);
      fired = true;
    }
  }

  ch->mob_specials.proc_fired = state;
  return fired;
}

bool rol_lich_energy_drain_together(const struct char_data *candidate,
                                    const struct char_data *primary)
{
  if (candidate == NULL || primary == NULL)
    return false;

  if (candidate == primary || candidate->master == primary || primary->master == candidate)
    return true;
  if (candidate->master != NULL && candidate->master == primary->master)
    return true;
  if (GROUP(candidate) != NULL && GROUP(candidate) == GROUP(primary))
    return true;
  if (candidate->master != NULL && GROUP(candidate->master) != NULL &&
      GROUP(candidate->master) == GROUP(primary))
    return true;
  if (primary->master != NULL && GROUP(primary->master) != NULL &&
      GROUP(candidate) == GROUP(primary->master))
    return true;

  return false;
}

int rol_lich_energy_drain_victim_hit(int current_hit, bool death_warded)
{
  if (current_hit <= 0)
    return current_hit;

  return death_warded ? 0 : -5;
}

int rol_lich_energy_drain_healer_hit(int current_hit, int drained_hit, bool blackmantled)
{
  if (blackmantled || drained_hit <= 0)
    return current_hit;
  if (current_hit > INT_MAX - drained_hit)
    return INT_MAX;

  return current_hit + drained_hit;
}

long rol_lich_energy_drain_stun_duration(long remaining)
{
  long duration = PULSE_VIOLENCE * 2;

  if (remaining <= 0)
    return duration;
  if (remaining > LONG_MAX - duration)
    return LONG_MAX;

  return remaining + duration;
}

static void rol_lich_energy_drain_stun(struct char_data *victim)
{
  struct mud_event_data *stun_event;
  long duration;
  long remaining;

  if (!can_stun(victim))
    return;

  stun_event = char_has_mud_event(victim, eSTUNNED);
  if (stun_event == NULL)
  {
    attach_mud_event(new_mud_event(eSTUNNED, victim, NULL), rol_lich_energy_drain_stun_duration(0));
    return;
  }

  remaining = stun_event->pEvent != NULL ? event_time(stun_event->pEvent) : 0;
  duration = rol_lich_energy_drain_stun_duration(remaining);
  change_event_duration(victim, eSTUNNED, duration);
}

int rol_lich_energy_drain(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *primary;
  struct char_data *victim;
  int drained_hit;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || !IS_NPC(ch) || cmd || IS_CASTING(ch) || (primary = FIGHTING(ch)) == NULL ||
      !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;

  for (victim = world[IN_ROOM(ch)].people; victim != NULL; victim = victim->next_in_room)
  {
    if (GET_HIT(victim) <= 0 ||
        (victim != primary && !rol_lich_energy_drain_together(victim, primary)) ||
        rand_number(0, 4) != 0)
      continue;

    act("\tWYou reach out and suck the life force away from $N!\tn", TRUE, ch, NULL, victim,
        TO_CHAR);
    act("$n \trturns and gazes at you wickedly, and you freeze in place.\tn\r\n"
        "$n \tWreaches out with a skeletal hand and touches you!\tn\r\n"
        "\tWYou scream as your life force flows away from you.\tn",
        FALSE, ch, NULL, victim, TO_VICT);
    act("$n \trturns and gazes at $N, who freezes in place.\tn\r\n"
        "$n \tWreaches out and sucks the life force from $N!\tn",
        TRUE, ch, NULL, victim, TO_NOTVICT);

    drained_hit = GET_HIT(victim);
    GET_HIT(ch) = rol_lich_energy_drain_healer_hit(GET_HIT(ch), drained_hit,
                                                   AFF_FLAGGED(ch, AFF_BLACKMANTLE));
    GET_HIT(victim) =
        rol_lich_energy_drain_victim_hit(drained_hit, AFF_FLAGGED(victim, AFF_DEATH_WARD));
    update_pos(victim);

    rol_lich_energy_drain_stun(victim);
    break;
  }

  /* The source callback deliberately allows the ordinary NPC action to continue. */
  return FALSE;
}

static struct obj_data *rol_bandit_owned_wagon(struct char_data *ch)
{
  struct obj_data *obj;

  if (ch == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return NULL;

  for (obj = world[IN_ROOM(ch)].contents; obj != NULL; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_WAGON && GET_OBJ_VAL(obj, 3) == GET_IDNUM(ch))
      return obj;

  return NULL;
}

int rol_bandit_cargo_value(struct char_data *ch)
{
  struct obj_data *obj;
  struct obj_data *wagon;
  long long total = 0;

  if (ch == NULL)
    return 0;

  for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    if (GET_OBJ_TYPE(obj) == ITEM_RESOURCE)
      total += GET_OBJ_COST(obj);

  wagon = rol_bandit_owned_wagon(ch);
  if (wagon != NULL)
    for (obj = wagon->contains; obj != NULL; obj = obj->next_content)
      total += GET_OBJ_COST(obj);

  return (int)MIN((long long)INT_MAX, MAX(0LL, total));
}

int rol_bandit_fee_gold(int target_vnum, int cargo_value, int alignment, int carried_gold)
{
  long long base_platinum;

  base_platinum = MAX(0, cargo_value) / 1000;
  if (base_platinum == 0)
    return ROL_BANDIT_DEMAND_PASS;

  switch (target_vnum)
  {
  case 2099501:
    return 50;
  case 2099502:
    return (int)MIN((long long)INT_MAX, (base_platinum / 3) * 10);
  case 2099503:
    return (int)MIN((long long)INT_MAX, (base_platinum / 2) * 10);
  case 2099504:
    return (int)MIN((long long)INT_MAX, base_platinum * 10);
  case 2099505:
    return carried_gold > 0 ? carried_gold : ROL_BANDIT_DEMAND_TAKE_WAGON;
  case 2099506:
    if (alignment >= 350)
      return 100;
    if (alignment <= -350)
      return ROL_BANDIT_DEMAND_ATTACK;
    return carried_gold > 0 ? carried_gold : 100;
  case 2099507:
    return ROL_BANDIT_DEMAND_ATTACK;
  default:
    return ROL_BANDIT_DEMAND_PASS;
  }
}

static bool rol_bandit_is_alone(struct char_data *bandit)
{
  if (bandit == NULL || !VALID_ROOM_RNUM(IN_ROOM(bandit)))
    return true;

  return world[IN_ROOM(bandit)].people == bandit && bandit->next_in_room == NULL;
}

static void rol_bandit_vanish(struct char_data *bandit)
{
  rol_purge_gated_inventory(bandit);
  extract_char(bandit);
}

static void rol_bandit_attack(struct char_data *bandit, struct char_data *victim,
                              const char *message)
{
  if (message != NULL)
    do_say(bandit, message, 0, 0);
  hit(bandit, victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, ATTACK_TYPE_PRIMARY);
}

static bool rol_bandit_take_wagon(struct char_data *bandit, struct char_data *victim)
{
  struct obj_data *wagon;

  wagon = rol_bandit_owned_wagon(victim);
  if (wagon == NULL)
    return false;

  act("$n grabs your wagon.", FALSE, bandit, NULL, victim, TO_VICT);
  act("$n grabs $N's wagon.", TRUE, bandit, NULL, victim, TO_NOTVICT);
  extract_obj(wagon);
  return true;
}

static void rol_bandit_announce_demand(struct char_data *bandit, int target_vnum, int fee_gold)
{
  char message[MAX_INPUT_LENGTH];

  switch (target_vnum)
  {
  case 2099501:
    do_say(bandit, "You have to pay to pass. The toll is 50 gold coins.", 0, 0);
    break;
  case 2099502:
    do_say(bandit, "You had better pay, or your head will fall from your neck!", 0, 0);
    snprintf(message, sizeof(message), "The price for your life is %d gold coins.", fee_gold);
    do_say(bandit, message, 0, 0);
    break;
  case 2099503:
    do_say(bandit, "Have you ever experienced a blade in your belly?", 0, 0);
    snprintf(message, sizeof(message), "If you do not want to, pay me %d gold coins.", fee_gold);
    do_say(bandit, message, 0, 0);
    break;
  case 2099504:
    do_say(bandit, "Life is so dangerous today!", 0, 0);
    snprintf(message, sizeof(message),
             "For example, you will die if you do not hand me %d gold coins.", fee_gold);
    do_say(bandit, message, 0, 0);
    break;
  case 2099505:
    do_say(bandit, "It is a hard life being a merchant!", 0, 0);
    do_say(bandit, "But it is an even worse life being a bandit.", 0, 0);
    do_say(bandit, "Give me all your gold coins and leave your wagon to me.", 0, 0);
    break;
  case 2099506:
    if (fee_gold == 100)
    {
      do_say(bandit, "Poor people need your money more than you do.", 0, 0);
      do_say(bandit, "Pay a 100 gold toll and you will be free.", 0, 0);
    }
    else
    {
      do_say(bandit, "I really dislike people who refuse to take a side.", 0, 0);
      snprintf(message, sizeof(message), "A donation of %d gold coins could redeem you.", fee_gold);
      do_say(bandit, message, 0, 0);
    }
    break;
  }
}

static bool rol_bandit_blocks_command(int cmd)
{
  if (cmd <= 0 || complete_cmd_info == NULL)
    return false;

  return IS_MOVE(cmd) || CMD_IS("flee") || CMD_IS("get");
}

int rol_bandit(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *bandit = me;
  long long paid;
  int before_gold;
  int cargo_value;
  int fee_gold;
  int target_vnum;
  time_t now;

  if (bandit == NULL && cmd == 0)
    bandit = ch;
  if (bandit == NULL || !IS_NPC(bandit))
    return FALSE;

  now = time(NULL);
  if (bandit->mob_specials.rol_bandit_expire_at == 0)
    bandit->mob_specials.rol_bandit_expire_at = now + (10 * SECS_PER_MUD_HOUR);

  if (cmd == 0)
  {
    if (bandit->mob_specials.rol_bandit_expire_at > 0 &&
        now >= bandit->mob_specials.rol_bandit_expire_at)
    {
      bandit->mob_specials.rol_bandit_expire_at = (time_t)-1;
      if (rol_bandit_is_alone(bandit))
        rol_bandit_vanish(bandit);
      return TRUE;
    }
    return FALSE;
  }

  if (ch == NULL || IS_NPC(ch) || !AWAKE(bandit) || FIGHTING(bandit) != NULL ||
      complete_cmd_info == NULL)
    return FALSE;

  if (bandit->mob_specials.rol_bandit_victim_id == GET_IDNUM(ch))
  {
    if (CMD_IS("camp") || CMD_IS("leavecart"))
    {
      rol_bandit_attack(bandit, ch, "Are you trying to swindle me?");
      return TRUE;
    }

    if (CMD_IS("give"))
    {
      before_gold = GET_GOLD(bandit);
      do_give(ch, argument, cmd, 0);
      paid = (long long)GET_GOLD(bandit) - before_gold;
      if (paid < bandit->mob_specials.rol_bandit_fee_gold)
      {
        rol_bandit_attack(bandit, ch, "You are REALLY foolish. Die!");
        return TRUE;
      }

      if (GET_MOB_VNUM(bandit) == 2099505 && !rol_bandit_take_wagon(bandit, ch))
      {
        rol_bandit_attack(bandit, ch, "You promised me a wagon. Die!");
        return TRUE;
      }

      if (GET_MOB_VNUM(bandit) == 2099506)
      {
        do_say(bandit, "That was very nice of you.", 0, 0);
        act("$n bows deeply, then disappears.", FALSE, bandit, NULL, ch, TO_ROOM);
      }
      else
      {
        do_say(bandit, "That was wise of you.", 0, 0);
        act("$n quickly disappears.", FALSE, bandit, NULL, ch, TO_ROOM);
      }
      rol_bandit_vanish(bandit);
      return TRUE;
    }

    if (!rol_bandit_blocks_command(cmd))
      return FALSE;

    if (rand_number(1, 5) == 5)
      rol_bandit_attack(bandit, ch, "I am tired of you. Die!");
    else
    {
      act("$n stops you.", FALSE, bandit, NULL, ch, TO_VICT);
      act("$n stops $N.", TRUE, bandit, NULL, ch, TO_NOTVICT);
    }
    return TRUE;
  }

  if (bandit->mob_specials.rol_bandit_victim_id != 0 || !rol_bandit_blocks_command(cmd))
    return FALSE;

  target_vnum = GET_MOB_VNUM(bandit);
  cargo_value = rol_bandit_cargo_value(ch);
  fee_gold = rol_bandit_fee_gold(target_vnum, cargo_value, GET_ALIGNMENT(ch), GET_GOLD(ch));
  if (fee_gold == ROL_BANDIT_DEMAND_PASS)
    return FALSE;

  act("$n stops you.", FALSE, bandit, NULL, ch, TO_VICT);
  act("$n stops $N.", TRUE, bandit, NULL, ch, TO_NOTVICT);
  bandit->mob_specials.rol_bandit_victim_id = GET_IDNUM(ch);
  bandit->mob_specials.rol_bandit_fee_gold = MAX(0, fee_gold);

  if (fee_gold == ROL_BANDIT_DEMAND_ATTACK)
  {
    rol_bandit_attack(bandit, ch,
                      target_vnum == 2099506 ? "Evil is a malady, and I am the cure." : NULL);
    return TRUE;
  }

  if (fee_gold == ROL_BANDIT_DEMAND_TAKE_WAGON)
  {
    do_say(bandit, "You are terribly poor. I will take your wagon instead.", 0, 0);
    if (!rol_bandit_take_wagon(bandit, ch))
    {
      rol_bandit_attack(bandit, ch, "No wagon either? Die!");
      return TRUE;
    }
    act("$n quickly disappears.", FALSE, bandit, NULL, ch, TO_ROOM);
    rol_bandit_vanish(bandit);
    return TRUE;
  }

  rol_bandit_announce_demand(bandit, target_vnum, fee_gold);
  return TRUE;
}

bool rol_sister_knight_vnum(int vnum)
{
  return vnum >= 2026218 && vnum <= 2026222;
}

static bool rol_sister_knight_can_answer(struct char_data *helper, struct char_data *caller,
                                         struct char_data *victim)
{
  int distance;

  if (helper == NULL || caller == NULL || victim == NULL || helper == caller || helper == victim ||
      !IS_NPC(helper) || !rol_sister_knight_vnum(GET_MOB_VNUM(helper)) ||
      IN_ROOM(helper) == NOWHERE || IN_ROOM(caller) == NOWHERE ||
      GET_ROOM_ZONE(IN_ROOM(helper)) != GET_ROOM_ZONE(IN_ROOM(caller)) || !AWAKE(helper) ||
      FIGHTING(helper) != NULL || HUNTING(helper) != NULL || AFF_FLAGGED(helper, AFF_CHARM) ||
      MOB_FLAGGED(helper, MOB_NOKILL) || !ok_damage_shopkeeper(victim, helper))
    return false;

  distance = count_rooms_between(IN_ROOM(helper), IN_ROOM(caller));
  return distance >= 0 && distance <= 100;
}

int rol_sister_knight(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *caller = me;
  struct char_data *helper;
  struct char_data *victim;
  const char *victim_name;
  char message[MAX_STRING_LENGTH];

  (void)argument;

  if (caller == NULL && cmd == 0)
    caller = ch;
  if (caller == NULL || !IS_NPC(caller) || !rol_sister_knight_vnum(GET_MOB_VNUM(caller)) ||
      IN_ROOM(caller) == NOWHERE)
    return FALSE;

  victim = FIGHTING(caller);
  if (victim == NULL)
  {
    PROC_FIRED(caller) = FALSE;
    return FALSE;
  }
  if (cmd != 0 || PROC_FIRED(caller) || ROOM_FLAGGED(IN_ROOM(caller), ROOM_SOUNDPROOF) ||
      !AWAKE(caller) || IS_CASTING(caller) || AFF_FLAGGED(caller, AFF_SILENCED) ||
      AFF_FLAGGED(caller, AFF_PARALYZED))
    return FALSE;

  victim_name = CAN_SEE(caller, victim) ? GET_NAME(victim) : "Someone";
  snprintf(message, sizeof(message),
           "\r\n%s shouts, 'Come, my sisters, we are under attack by %s!'\r\n", GET_NAME(caller),
           victim_name);
  send_to_zone(message, GET_ROOM_ZONE(IN_ROOM(caller)));

  for (helper = character_list; helper != NULL; helper = helper->next)
    if (rol_sister_knight_can_answer(helper, caller, victim))
      HUNTING(helper) = victim;

  PROC_FIRED(caller) = TRUE;
  return TRUE;
}

const char *rol_bloodstone_critter_social(int roll)
{
  switch (roll)
  {
  case 0:
    return "snarl";
  case 1:
    return "growl";
  default:
    return NULL;
  }
}

int rol_bloodstone_critter(struct char_data *ch, void *me, int cmd, const char *argument)
{
  const char *social;
  int social_command;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || !IS_NPC(ch) || cmd || !AWAKE(ch) || FIGHTING(ch) != NULL)
    return FALSE;

  social = rol_bloodstone_critter_social(rand_number(0, 80));
  if (social == NULL || (social_command = find_command(social)) < 0)
    return FALSE;

  do_action(ch, "", social_command, 0);
  return TRUE;
}

static const struct rol_source_periodic_profile *rol_source_periodic_profile_for(int mobile_vnum)
{
  size_t high = sizeof(rol_source_periodic_profiles) / sizeof(rol_source_periodic_profiles[0]);
  size_t low = 0;
  size_t middle;

  while (low < high)
  {
    middle = low + (high - low) / 2;
    if (rol_source_periodic_profiles[middle].mobile_vnum < mobile_vnum)
      low = middle + 1;
    else
      high = middle;
  }

  if (low < sizeof(rol_source_periodic_profiles) / sizeof(rol_source_periodic_profiles[0]) &&
      rol_source_periodic_profiles[low].mobile_vnum == mobile_vnum)
    return &rol_source_periodic_profiles[low];

  return NULL;
}

static const struct rol_source_periodic_outcome *rol_source_periodic_outcome_for(int profile_id,
                                                                                 int roll)
{
  size_t high = sizeof(rol_source_periodic_outcomes) / sizeof(rol_source_periodic_outcomes[0]);
  size_t low = 0;
  size_t middle;
  const struct rol_source_periodic_outcome *outcome;

  while (low < high)
  {
    middle = low + (high - low) / 2;
    outcome = &rol_source_periodic_outcomes[middle];
    if (outcome->profile_id < profile_id ||
        (outcome->profile_id == profile_id && outcome->roll < roll))
      low = middle + 1;
    else
      high = middle;
  }

  if (low < sizeof(rol_source_periodic_outcomes) / sizeof(rol_source_periodic_outcomes[0]))
  {
    outcome = &rol_source_periodic_outcomes[low];
    if (outcome->profile_id == profile_id && outcome->roll == roll)
      return outcome;
  }

  return NULL;
}

size_t rol_source_periodic_profile_count(void)
{
  return sizeof(rol_source_periodic_profiles) / sizeof(rol_source_periodic_profiles[0]);
}

bool rol_source_periodic_profile_bounds(int mobile_vnum, int *roll_min, int *roll_max,
                                        bool *requires_awake, bool *suppresses_fighting)
{
  const struct rol_source_periodic_profile *profile = rol_source_periodic_profile_for(mobile_vnum);

  if (profile == NULL)
    return false;
  if (roll_min != NULL)
    *roll_min = profile->roll_min;
  if (roll_max != NULL)
    *roll_max = profile->roll_max;
  if (requires_awake != NULL)
    *requires_awake = profile->require_awake;
  if (suppresses_fighting != NULL)
    *suppresses_fighting = profile->suppress_fighting;
  return true;
}

bool rol_source_periodic_dice_shape(int mobile_vnum, int *dice_count, int *dice_sides)
{
  const struct rol_source_periodic_profile *profile = rol_source_periodic_profile_for(mobile_vnum);

  if (profile == NULL || profile->dice_count <= 0 || profile->dice_sides <= 0)
    return false;
  if (dice_count != NULL)
    *dice_count = profile->dice_count;
  if (dice_sides != NULL)
    *dice_sides = profile->dice_sides;
  return true;
}

bool rol_source_periodic_requires_sleeping(int mobile_vnum)
{
  const struct rol_source_periodic_profile *profile = rol_source_periodic_profile_for(mobile_vnum);

  return profile != NULL && profile->require_sleeping;
}

size_t rol_source_periodic_outcome_action_count(int mobile_vnum, int roll)
{
  const struct rol_source_periodic_profile *profile = rol_source_periodic_profile_for(mobile_vnum);
  const struct rol_source_periodic_outcome *outcome;

  if (profile == NULL)
    return 0;
  outcome = rol_source_periodic_outcome_for(profile->profile_id, roll);
  return outcome != NULL ? outcome->action_count : 0;
}

const char *rol_source_periodic_outcome_action(int mobile_vnum, int roll, size_t action_index,
                                               bool *speech, bool *hide)
{
  const struct rol_source_periodic_profile *profile = rol_source_periodic_profile_for(mobile_vnum);
  const struct rol_source_periodic_outcome *outcome;
  const struct rol_source_periodic_action *action;

  if (profile == NULL)
    return NULL;
  outcome = rol_source_periodic_outcome_for(profile->profile_id, roll);
  if (outcome == NULL || action_index >= outcome->action_count)
    return NULL;
  action = &rol_source_periodic_actions[outcome->first_action + action_index];
  if (speech != NULL)
    *speech = action->kind == ROL_SOURCE_PERIODIC_SPEECH;
  if (hide != NULL)
    *hide = action->hide;
  return action->message;
}

int rol_source_periodic(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *speaker = me;
  const struct rol_source_periodic_profile *profile;
  const struct rol_source_periodic_outcome *outcome;
  const struct rol_source_periodic_action *action;
  int roll;
  size_t index;

  (void)argument;

  if (speaker == NULL && cmd == 0)
    speaker = ch;
  if (speaker == NULL || cmd != 0 || !IS_NPC(speaker) || IN_ROOM(speaker) == NOWHERE)
    return FALSE;

  profile = rol_source_periodic_profile_for(GET_MOB_VNUM(speaker));
  if (profile == NULL || (profile->require_awake && !AWAKE(speaker)) ||
      (profile->require_sleeping && GET_POS(speaker) != POS_SLEEPING) ||
      (profile->suppress_fighting && FIGHTING(speaker) != NULL))
    return FALSE;

  if (profile->dice_count > 0 && profile->dice_sides > 0)
    roll = dice(profile->dice_count, profile->dice_sides);
  else
    roll = rand_number(profile->roll_min, profile->roll_max);
  outcome = rol_source_periodic_outcome_for(profile->profile_id, roll);
  if (outcome == NULL)
    return FALSE;

  for (index = 0; index < outcome->action_count; index++)
  {
    action = &rol_source_periodic_actions[outcome->first_action + index];
    if (action->kind == ROL_SOURCE_PERIODIC_SPEECH)
      do_say(speaker, action->message, 0, 0);
    else
      act(action->message, action->hide, speaker, NULL, NULL, TO_ROOM);
  }

  return TRUE;
}

static const struct rol_state_periodic_profile *rol_state_periodic_profile_for(int mobile_vnum)
{
  size_t high = sizeof(rol_state_periodic_profiles) / sizeof(rol_state_periodic_profiles[0]);
  size_t low = 0;
  size_t middle;

  while (low < high)
  {
    middle = low + (high - low) / 2;
    if (rol_state_periodic_profiles[middle].mobile_vnum < mobile_vnum)
      low = middle + 1;
    else
      high = middle;
  }

  if (low < sizeof(rol_state_periodic_profiles) / sizeof(rol_state_periodic_profiles[0]) &&
      rol_state_periodic_profiles[low].mobile_vnum == mobile_vnum)
    return &rol_state_periodic_profiles[low];
  return NULL;
}

static const struct rol_state_periodic_outcome *
rol_state_periodic_outcome_for(int profile_id, enum rol_state_periodic_state state, int roll)
{
  size_t high = sizeof(rol_state_periodic_outcomes) / sizeof(rol_state_periodic_outcomes[0]);
  size_t low = 0;
  size_t middle;
  const struct rol_state_periodic_outcome *outcome;

  while (low < high)
  {
    middle = low + (high - low) / 2;
    outcome = &rol_state_periodic_outcomes[middle];
    if (outcome->profile_id < profile_id ||
        (outcome->profile_id == profile_id && outcome->state < state) ||
        (outcome->profile_id == profile_id && outcome->state == state && outcome->roll < roll))
      low = middle + 1;
    else
      high = middle;
  }

  if (low < sizeof(rol_state_periodic_outcomes) / sizeof(rol_state_periodic_outcomes[0]))
  {
    outcome = &rol_state_periodic_outcomes[low];
    if (outcome->profile_id == profile_id && outcome->state == state && outcome->roll == roll)
      return outcome;
  }
  return NULL;
}

size_t rol_state_periodic_profile_count(void)
{
  return sizeof(rol_state_periodic_profiles) / sizeof(rol_state_periodic_profiles[0]);
}

bool rol_state_periodic_dice(int mobile_vnum, bool fighting, int *dice_count, int *dice_sides)
{
  const struct rol_state_periodic_profile *profile = rol_state_periodic_profile_for(mobile_vnum);
  int count;
  int sides;

  if (profile == NULL)
    return false;
  count = fighting ? profile->fighting_dice_count : profile->idle_dice_count;
  sides = fighting ? profile->fighting_dice_sides : profile->idle_dice_sides;
  if (count == 0 || sides == 0)
    return false;
  if (dice_count != NULL)
    *dice_count = count;
  if (dice_sides != NULL)
    *dice_sides = sides;
  return true;
}

bool rol_state_periodic_runs_idle_while_fighting(int mobile_vnum)
{
  const struct rol_state_periodic_profile *profile = rol_state_periodic_profile_for(mobile_vnum);

  return profile != NULL && profile->cumulative_idle_while_fighting;
}

size_t rol_state_periodic_outcome_action_count(int mobile_vnum, bool fighting, int roll)
{
  const struct rol_state_periodic_profile *profile = rol_state_periodic_profile_for(mobile_vnum);
  const struct rol_state_periodic_outcome *outcome;
  enum rol_state_periodic_state state =
      fighting ? ROL_STATE_PERIODIC_FIGHTING : ROL_STATE_PERIODIC_IDLE;

  if (profile == NULL)
    return 0;
  outcome = rol_state_periodic_outcome_for(profile->profile_id, state, roll);
  return outcome != NULL ? outcome->action_count : 0;
}

const char *rol_state_periodic_outcome_action(int mobile_vnum, bool fighting, int roll,
                                              size_t action_index, bool *speech, bool *hide)
{
  const struct rol_state_periodic_profile *profile = rol_state_periodic_profile_for(mobile_vnum);
  const struct rol_state_periodic_outcome *outcome;
  const struct rol_source_periodic_action *action;
  enum rol_state_periodic_state state =
      fighting ? ROL_STATE_PERIODIC_FIGHTING : ROL_STATE_PERIODIC_IDLE;

  if (profile == NULL)
    return NULL;
  outcome = rol_state_periodic_outcome_for(profile->profile_id, state, roll);
  if (outcome == NULL || action_index >= outcome->action_count)
    return NULL;
  action = &rol_state_periodic_actions[outcome->first_action + action_index];
  if (speech != NULL)
    *speech = action->kind == ROL_SOURCE_PERIODIC_SPEECH;
  if (hide != NULL)
    *hide = action->hide;
  return action->message;
}

static void rol_state_periodic_emit(struct char_data *speaker,
                                    const struct rol_state_periodic_profile *profile,
                                    enum rol_state_periodic_state state)
{
  const struct rol_state_periodic_outcome *outcome;
  const struct rol_source_periodic_action *action;
  int dice_count;
  int dice_sides;
  int roll;
  size_t index;

  if (state == ROL_STATE_PERIODIC_FIGHTING)
  {
    dice_count = profile->fighting_dice_count;
    dice_sides = profile->fighting_dice_sides;
  }
  else
  {
    dice_count = profile->idle_dice_count;
    dice_sides = profile->idle_dice_sides;
  }
  if (dice_count == 0 || dice_sides == 0)
    return;

  roll = dice(dice_count, dice_sides);
  outcome = rol_state_periodic_outcome_for(profile->profile_id, state, roll);
  if (outcome == NULL)
    return;

  for (index = 0; index < outcome->action_count; index++)
  {
    action = &rol_state_periodic_actions[outcome->first_action + index];
    if (action->kind == ROL_SOURCE_PERIODIC_SPEECH)
      do_say(speaker, action->message, 0, 0);
    else
      act(action->message, action->hide, speaker, NULL, NULL, TO_ROOM);
  }
}

int rol_state_periodic(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *speaker = me;
  const struct rol_state_periodic_profile *profile;
  bool fighting;

  (void)argument;

  if (speaker == NULL && cmd == 0)
    speaker = ch;
  if (speaker == NULL || cmd != 0 || !IS_NPC(speaker) || !AWAKE(speaker) ||
      IN_ROOM(speaker) == NOWHERE)
    return FALSE;

  profile = rol_state_periodic_profile_for(GET_MOB_VNUM(speaker));
  fighting = FIGHTING(speaker) != NULL;
  if (profile == NULL || (!fighting && GET_POS(speaker) < POS_STANDING))
    return FALSE;

  if (fighting)
  {
    rol_state_periodic_emit(speaker, profile, ROL_STATE_PERIODIC_FIGHTING);
    if (profile->cumulative_idle_while_fighting)
      rol_state_periodic_emit(speaker, profile, ROL_STATE_PERIODIC_IDLE);
  }
  else
    rol_state_periodic_emit(speaker, profile, ROL_STATE_PERIODIC_IDLE);
  return FALSE;
}

static const struct rol_waterdeep_bouncer_profile *
rol_waterdeep_bouncer_profile_for(int mobile_vnum)
{
  size_t index;

  for (index = 0;
       index < sizeof(rol_waterdeep_bouncer_profiles) / sizeof(rol_waterdeep_bouncer_profiles[0]);
       index++)
  {
    if (rol_waterdeep_bouncer_profiles[index].mobile_vnum == mobile_vnum)
      return &rol_waterdeep_bouncer_profiles[index];
  }
  return NULL;
}

int rol_waterdeep_bouncer_home_vnum(int mobile_vnum)
{
  const struct rol_waterdeep_bouncer_profile *profile =
      rol_waterdeep_bouncer_profile_for(mobile_vnum);

  return profile != NULL ? profile->route[0] : 0;
}

size_t rol_waterdeep_bouncer_route_length(int mobile_vnum)
{
  const struct rol_waterdeep_bouncer_profile *profile =
      rol_waterdeep_bouncer_profile_for(mobile_vnum);

  return profile != NULL ? profile->route_length : 0;
}

static struct char_data *rol_waterdeep_peacekeeper_offender(struct char_data *keeper)
{
  struct char_data *candidate;
  struct char_data *offender = NULL;
  int lowest_alignment = 1000;

  if (keeper == NULL || !VALID_ROOM_RNUM(IN_ROOM(keeper)))
    return NULL;

  for (candidate = world[IN_ROOM(keeper)].people; candidate != NULL;
       candidate = candidate->next_in_room)
  {
    if (FIGHTING(candidate) == NULL || !CAN_SEE(keeper, candidate) ||
        (!IS_NPC(candidate) && !IS_NPC(FIGHTING(candidate))) ||
        GET_ALIGNMENT(candidate) >= lowest_alignment)
      continue;
    lowest_alignment = GET_ALIGNMENT(candidate);
    offender = candidate;
  }

  if (offender == NULL || FIGHTING(offender) == NULL || GET_ALIGNMENT(FIGHTING(offender)) < 0)
    return NULL;
  return offender;
}

static void rol_waterdeep_peacekeeper_return_home(struct char_data *keeper, room_rnum home)
{
  if (keeper == NULL || !VALID_ROOM_RNUM(home) || IN_ROOM(keeper) == home)
    return;

  act("$n looks around dazedly and realizes $e must get back to work.", FALSE, keeper, NULL, NULL,
      TO_ROOM);
  act("$n slowly fades from view.", TRUE, keeper, NULL, NULL, TO_ROOM);
  char_from_room(keeper);
  char_to_room(keeper, home);
  act("$n pops into view with a sulphurous bang.", FALSE, keeper, NULL, NULL, TO_ROOM);
}

static int rol_waterdeep_bouncer(struct char_data *keeper,
                                 const struct rol_waterdeep_bouncer_profile *profile)
{
  struct char_data *offender;
  room_rnum route[ROL_WATERDEEP_BOUNCER_MAX_ROUTE];
  size_t index;

  for (index = 0; index < profile->route_length; index++)
  {
    route[index] = real_room(profile->route[index]);
    if (!VALID_ROOM_RNUM(route[index]))
    {
      log("SYSERR: RoL Waterdeep bouncer %d has invalid route room %d", GET_MOB_VNUM(keeper),
          profile->route[index]);
      return FALSE;
    }
  }

  offender = rol_waterdeep_peacekeeper_offender(keeper);
  if (offender == NULL || IN_ROOM(keeper) != route[0])
  {
    rol_waterdeep_peacekeeper_return_home(keeper, route[0]);
    return FALSE;
  }

  rol_guild_guard_stop_victim_combat(offender);
  act("$n yells, 'HEY! No fighting in here, dead beat!'", FALSE, keeper, NULL, offender, TO_ROOM);
  act("The bouncer grabs you by the collar and drags you out of the tavern!", FALSE, offender, NULL,
      keeper, TO_CHAR);
  act("$n grabs $N by the collar and drags $M out of the tavern!", FALSE, keeper, NULL, offender,
      TO_NOTVICT);

  for (index = 1; index < profile->route_length; index++)
  {
    char_from_room(keeper);
    char_to_room(keeper, route[index]);
    char_from_room(offender);
    char_to_room(offender, route[index]);

    if (index + 1 < profile->route_length)
    {
      act("A bouncer storms into the room, dragging $N by the collar!", FALSE, keeper, NULL,
          offender, TO_NOTVICT);
      act("$N kicks and flails around as $E is dragged out.", FALSE, keeper, NULL, offender,
          TO_NOTVICT);
      send_to_char(offender, "You are dragged through the tavern kicking and screaming.\r\n");
    }
    else
    {
      act("A bouncer throws $N from the tavern, and $E lands in a heap!", FALSE, keeper, NULL,
          offender, TO_NOTVICT);
      act("The bouncer throws you onto the ground. You land in a heap.", FALSE, offender, NULL,
          keeper, TO_CHAR);
      act("The bouncer snarls, 'Next time I'll break your neck, punk!'", FALSE, offender, NULL,
          keeper, TO_CHAR);
    }
  }

  GET_POS(offender) = POS_SITTING;
  char_from_room(keeper);
  char_to_room(keeper, route[0]);
  act("A bouncer walks back in, smiling smugly.", TRUE, keeper, NULL, NULL, TO_ROOM);
  return TRUE;
}

static int rol_waterdeep_casino_bouncer(struct char_data *keeper)
{
  struct char_data *offender;
  room_rnum destination;
  room_rnum home = GET_MOB_LOADROOM(keeper);

  if (!VALID_ROOM_RNUM(home))
  {
    log("SYSERR: RoL casino bouncer %d has no valid load room", GET_MOB_VNUM(keeper));
    return FALSE;
  }
  if (IN_ROOM(keeper) != home)
  {
    rol_waterdeep_peacekeeper_return_home(keeper, home);
    return FALSE;
  }

  offender = rol_waterdeep_peacekeeper_offender(keeper);
  if (offender == NULL)
    return FALSE;
  destination = real_room(ROL_WATERDEEP_CASINO_EXIT_VNUM);
  if (!VALID_ROOM_RNUM(destination))
  {
    log("SYSERR: RoL casino bouncer %d has invalid exit room %d", GET_MOB_VNUM(keeper),
        ROL_WATERDEEP_CASINO_EXIT_VNUM);
    return FALSE;
  }

  rol_guild_guard_stop_victim_combat(offender);
  act("$n yells, 'HEY! No fighting in here, dead beat!'", FALSE, keeper, NULL, offender, TO_ROOM);
  act("The bouncer picks you up and tosses you out of the bar!", FALSE, offender, NULL, keeper,
      TO_CHAR);
  act("$n throws $N out into the street!", FALSE, keeper, NULL, offender, TO_NOTVICT);
  char_from_room(offender);
  char_to_room(offender, destination);
  GET_POS(offender) = POS_SITTING;
  act("$n lands in a heap after being thrown from the tavern!", FALSE, offender, NULL, NULL,
      TO_ROOM);
  send_to_char(offender, "The bouncer snarls, 'Next time I'll break your neck, punk!'\r\n");
  return TRUE;
}

static void rol_waterdeep_off_duty_guard_ambient(struct char_data *guard)
{
  switch (dice(2, 6))
  {
  case 2:
    do_say(guard, "How's about some entertainment, bartender! Where's that dancer.", 0, 0);
    act("$n grins evilly.", TRUE, guard, NULL, NULL, TO_ROOM);
    break;
  case 3:
    act("$n sways slightly from being drunk.", TRUE, guard, NULL, NULL, TO_ROOM);
    break;
  case 4:
    act("$n says, 'Hey, bartender! Bring me another, dammit.'", TRUE, guard, NULL, NULL, TO_ROOM);
    break;
  case 5:
    act("$n laughs heartily, nearly falling off the barstool.", TRUE, guard, NULL, NULL, TO_ROOM);
    break;
  case 6:
    act("$n drinks deeply from a bottle, then lets out a roaring belch.", TRUE, guard, NULL, NULL,
        TO_ROOM);
    break;
  default:
    break;
  }
}

static int rol_waterdeep_off_duty_guard(struct char_data *guard)
{
  struct char_data *offender;

  if (FIGHTING(guard) != NULL || GET_POS(guard) >= POS_STANDING)
    rol_waterdeep_off_duty_guard_ambient(guard);
  if (FIGHTING(guard) != NULL || ROOM_FLAGGED(IN_ROOM(guard), ROOM_PEACEFUL))
    return FALSE;

  offender = rol_waterdeep_peacekeeper_offender(guard);
  if (offender == NULL)
    return FALSE;

  do_say(guard, "Heeey! You c-can't do that, I'm a guard of Waterdeep!", 0, 0);
  act("$n jumps off the stool and joins the fight with a drunken grin.", FALSE, guard, NULL, NULL,
      TO_ROOM);
  return set_fighting(guard, offender) ? TRUE : FALSE;
}

int rol_waterdeep_peacekeeper(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *keeper = me;
  const struct rol_waterdeep_bouncer_profile *profile;

  (void)argument;

  if (keeper == NULL && cmd == 0)
    keeper = ch;
  if (keeper == NULL || cmd != 0 || !IS_NPC(keeper) || !AWAKE(keeper) ||
      !VALID_ROOM_RNUM(IN_ROOM(keeper)))
    return FALSE;

  profile = rol_waterdeep_bouncer_profile_for(GET_MOB_VNUM(keeper));
  if (profile != NULL)
  {
    if (FIGHTING(keeper) != NULL || AFF_FLAGGED(keeper, AFF_CHARM))
      return FALSE;
    return rol_waterdeep_bouncer(keeper, profile);
  }
  if (GET_MOB_VNUM(keeper) == 2003207)
  {
    if (FIGHTING(keeper) != NULL)
      return FALSE;
    return rol_waterdeep_casino_bouncer(keeper);
  }
  if (GET_MOB_VNUM(keeper) == 2003229)
    return rol_waterdeep_off_duty_guard(keeper);
  return FALSE;
}

static mob_vnum rol_designated_follower_leader_vnum(mob_vnum follower_vnum)
{
  switch (follower_vnum)
  {
  case 2097009:
    return 2097012;
  case 2097018:
  case 2097019:
    return 2097020;
  case 2097036:
  case 2097037:
    return 2097035;
  default:
    return NOBODY;
  }
}

int rol_designated_follower(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *follower = me;
  struct char_data *leader;
  mob_vnum leader_vnum;

  (void)ch;
  (void)argument;

  if (follower == NULL || !IS_NPC(follower) || cmd != 0 || !AWAKE(follower) ||
      !VALID_ROOM_RNUM(IN_ROOM(follower)))
    return FALSE;

  leader_vnum = rol_designated_follower_leader_vnum(GET_MOB_VNUM(follower));
  if (leader_vnum == NOBODY)
    return FALSE;

  if (follower->master != NULL)
  {
    leader = follower->master;
    if (IS_NPC(leader) && IN_ROOM(leader) == IN_ROOM(follower) && GET_POS(follower) > POS_SITTING &&
        FIGHTING(follower) == NULL && FIGHTING(leader) != NULL &&
        !AFF2_FLAGGED(follower, AFF2_ROL_DOCILE) && !MOB_FLAGGED(follower, MOB_NOKILL))
    {
      perform_assist(follower, leader);
      return TRUE;
    }
    return FALSE;
  }

  for (leader = world[IN_ROOM(follower)].people; leader != NULL; leader = leader->next_in_room)
  {
    if (leader != follower && IS_NPC(leader) && GET_MOB_VNUM(leader) == leader_vnum)
    {
      add_follower(follower, leader);
      return TRUE;
    }
  }

  return FALSE;
}

bool rol_fixed_bodyguard_protects(int bodyguard_vnum, int protected_vnum)
{
  size_t index;

  for (index = 0;
       index < sizeof(rol_fixed_bodyguard_profiles) / sizeof(rol_fixed_bodyguard_profiles[0]);
       index++)
    if (rol_fixed_bodyguard_profiles[index].bodyguard_vnum == bodyguard_vnum)
      return rol_fixed_bodyguard_profiles[index].protected_vnum == protected_vnum;

  return false;
}

int rol_fixed_bodyguard(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *bodyguard = me;
  struct char_data *candidate;
  struct char_data *protected;

  (void)ch;
  (void)argument;

  if (bodyguard == NULL || !IS_NPC(bodyguard) || cmd != 0 || !AWAKE(bodyguard) ||
      !VALID_ROOM_RNUM(IN_ROOM(bodyguard)))
    return FALSE;

  for (protected = world[IN_ROOM(bodyguard)].people; protected != NULL;
       protected = protected->next_in_room)
  {
    if (protected == bodyguard || !IS_NPC(protected) ||
        !rol_fixed_bodyguard_protects(GET_MOB_VNUM(bodyguard), GET_MOB_VNUM(protected)))
      continue;

    for (candidate = world[IN_ROOM(bodyguard)].people; candidate != NULL;
         candidate = candidate->next_in_room)
    {
      if (candidate != bodyguard && FIGHTING(candidate) == protected)
      {
        perform_rescue(bodyguard, protected);
        return TRUE;
      }
    }
  }

  return FALSE;
}

bool rol_floating_pool_should_move(int roll)
{
  return roll >= 1 && roll <= 12;
}

static bool rol_floating_pool_exit_is_eligible(room_rnum room, int direction)
{
  struct room_direction_data *exit;
  room_rnum destination;

  if (!VALID_ROOM_RNUM(room) || direction < NORTH || direction > DOWN)
    return false;

  exit = world[room].dir_option[direction];
  if (exit == NULL)
    return false;

  destination = exit->to_room;
  return VALID_ROOM_RNUM(destination) &&
         !EXIT_FLAGGED(exit, EX_CLOSED | EX_HIDDEN | EX_HIDDEN_MEDIUM | EX_HIDDEN_HARD |
                                 EX_HIDDEN_EASY | EX_BLOCKED) &&
         !ROOM_FLAGGED(destination, ROOM_NOMOB);
}

int rol_floating_pool(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct obj_data *obj = me;
  int directions[NUM_OF_DIRS];
  room_rnum destination;
  room_rnum origin;
  int direction;
  int direction_count = 0;

  (void)ch;
  (void)argument;

  if (obj == NULL || cmd != 0 || !VALID_ROOM_RNUM(IN_ROOM(obj)) ||
      !rol_floating_pool_should_move(rand_number(1, 100)))
    return FALSE;

  origin = IN_ROOM(obj);
  for (direction = NORTH; direction <= DOWN; direction++)
    if (rol_floating_pool_exit_is_eligible(origin, direction))
      directions[direction_count++] = direction;

  if (direction_count == 0)
    return FALSE;

  direction = directions[rand_number(0, direction_count - 1)];
  destination = world[origin].dir_option[direction]->to_room;
  send_to_room(origin, "\tLThe pool floats silently away through the swirling ether...\tn\r\n");
  obj_from_room(obj);
  obj_to_room(obj, destination);
  send_to_room(destination, "\tLA smoky pool floats into the area.\tn\r\n");
  return TRUE;
}

static int rol_item_blocker_unlock_direction(struct char_data *ch, const char *argument)
{
  char type[MAX_INPUT_LENGTH];
  char direction[MAX_INPUT_LENGTH];
  int door;

  if (ch == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return NOWHERE;

  two_arguments(argument, type, sizeof(type), direction, sizeof(direction));
  if (direction[0] != '\0')
  {
    door = search_block(direction, dirs, FALSE);
    if (door < NORTH || door > DOWN || EXIT(ch, door) == NULL ||
        EXIT_FLAGGED(EXIT(ch, door),
                     EX_HIDDEN | EX_HIDDEN_MEDIUM | EX_HIDDEN_HARD | EX_HIDDEN_EASY | EX_BLOCKED))
      return NOWHERE;
    if (EXIT(ch, door)->keyword != NULL && !isname(type, EXIT(ch, door)->keyword))
      return NOWHERE;
    return door;
  }

  for (door = NORTH; door <= DOWN; door++)
    if (EXIT(ch, door) != NULL && EXIT(ch, door)->keyword != NULL &&
        !EXIT_FLAGGED(EXIT(ch, door), EX_HIDDEN | EX_HIDDEN_MEDIUM | EX_HIDDEN_HARD |
                                          EX_HIDDEN_EASY | EX_BLOCKED) &&
        isname(type, EXIT(ch, door)->keyword))
      return door;

  return NOWHERE;
}

int rol_item_blocker(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct obj_data *obj = me;
  struct char_data *aggressor;
  const char *command;
  bool morphed;
  int block_direction;
  int attempted_direction = NOWHERE;
  char message[MAX_STRING_LENGTH];

  if (ch == NULL || obj == NULL || cmd <= 0 || complete_cmd_info == NULL ||
      complete_cmd_info[cmd].command == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)))
    return FALSE;

  morphed = !IS_NPC(ch) && ch->player_specials != NULL && IS_MORPHED(ch);
  if (!morphed && ((IS_NPC(ch) && !IS_PET(ch)) || GET_LEVEL(ch) >= LVL_IMMORT))
    return FALSE;

  for (aggressor = world[IN_ROOM(ch)].people; aggressor != NULL;
       aggressor = aggressor->next_in_room)
    if (IS_NPC(aggressor) && MOB_FLAGGED(aggressor, MOB_AGGRESSIVE))
      break;
  if (aggressor == NULL)
    return FALSE;

  block_direction = GET_OBJ_VAL(obj, 0);
  if (block_direction < NORTH || block_direction > DOWN)
    return FALSE;

  command = complete_cmd_info[cmd].command;
  for (attempted_direction = NORTH; attempted_direction <= DOWN; attempted_direction++)
    if (!strcmp(command, dirs[attempted_direction]))
      break;
  if (attempted_direction > DOWN)
  {
    if (strcmp(command, "unlock"))
      return FALSE;
    attempted_direction = rol_item_blocker_unlock_direction(ch, argument);
  }
  if (attempted_direction != block_direction)
    return FALSE;

  snprintf(message, sizeof(message), "%s is blocking your path%s%s!\r\n", GET_NAME(aggressor),
           !strcmp(command, "unlock") ? " to the " : "",
           !strcmp(command, "unlock") ? dirs[block_direction] : "");
  CAP(message);
  send_to_char(ch, "%s", message);
  return TRUE;
}

static const struct rol_command_sentinel_profile *
rol_command_sentinel_profile_for(int mobile_vnum, int room_vnum, int direction)
{
  size_t index;

  for (index = 0;
       index < sizeof(rol_command_sentinel_profiles) / sizeof(rol_command_sentinel_profiles[0]);
       index++)
    if (rol_command_sentinel_profiles[index].mobile_vnum == mobile_vnum &&
        rol_command_sentinel_profiles[index].room_vnum == room_vnum &&
        rol_command_sentinel_profiles[index].direction == direction)
      return &rol_command_sentinel_profiles[index];

  return NULL;
}

bool rol_command_sentinel_blocks_passage(int mobile_vnum, int room_vnum, int direction,
                                         const struct char_data *ch, int chance_roll)
{
  const struct rol_command_sentinel_profile *profile;

  if (ch == NULL || (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT) ||
      (profile = rol_command_sentinel_profile_for(mobile_vnum, room_vnum, direction)) == NULL)
    return false;

  switch (profile->rule)
  {
  case ROL_SENTINEL_GOOD_RACE_OVER_LEVEL:
    return GET_LEVEL(ch) > profile->threshold && rol_race_is_good(GET_RACE(ch));
  case ROL_SENTINEL_NON_ORC:
    return GET_RACE(ch) != RACE_HALF_ORC;
  case ROL_SENTINEL_OVER_LEVEL:
    return GET_LEVEL(ch) > profile->threshold;
  case ROL_SENTINEL_CHANCE:
    return chance_roll > profile->threshold;
  default:
    return false;
  }
}

bool rol_command_sentinel_is_necromancer(const struct char_data *ch)
{
  if (ch == NULL)
    return false;

  if (IS_NPC(ch))
    return GET_CLASS(ch) == CLASS_NECROMANCER;

  return ch->player_specials != NULL && CLASS_LEVEL(ch, CLASS_NECROMANCER) > 0;
}

int rol_command_sentinel_glyph_damage(const struct char_data *ch)
{
  if (ch != NULL && (AFF_FLAGGED(ch, AFF_MINOR_GLOBE) || AFF_FLAGGED(ch, AFF_GLOBE_OF_INVULN)))
    return 25;

  return 1;
}

static int rol_command_sentinel_mobile(struct char_data *ch, struct char_data *sentinel, int cmd)
{
  const struct rol_command_sentinel_profile *profile;
  int direction;
  int current_room_vnum;
  int chance_roll = 0;

  if (ch == NULL || sentinel == NULL || !IS_NPC(sentinel) || cmd <= 0 ||
      complete_cmd_info == NULL || !IS_MOVE(cmd) || !VALID_ROOM_RNUM(IN_ROOM(sentinel)))
    return FALSE;

  direction = complete_cmd_info[cmd].subcmd;
  current_room_vnum = GET_ROOM_VNUM(IN_ROOM(sentinel));
  profile = rol_command_sentinel_profile_for(GET_MOB_VNUM(sentinel), current_room_vnum, direction);
  if (profile == NULL)
    return FALSE;

  if (profile->rule == ROL_SENTINEL_CHANCE)
    chance_roll = rand_number(1, 100);
  if (!rol_command_sentinel_blocks_passage(GET_MOB_VNUM(sentinel), current_room_vnum, direction, ch,
                                           chance_roll))
    return FALSE;

  act(profile->victim_message, FALSE, sentinel, NULL, ch, TO_VICT);
  act(profile->room_message, FALSE, sentinel, NULL, ch, TO_NOTVICT);
  return TRUE;
}

static bool rol_command_sentinel_cage_allows(int cmd)
{
  if (cmd <= 0 || complete_cmd_info == NULL || complete_cmd_info[cmd].command == NULL)
    return true;

  return CMD_IS("say") || CMD_IS("'") || CMD_IS("petition") || CMD_IS("project") || CMD_IS("help");
}

static int rol_command_sentinel_room(struct char_data *ch, struct room_data *room, int cmd)
{
  int damage_amount;

  if (ch == NULL || room == NULL || cmd <= 0 || complete_cmd_info == NULL)
    return FALSE;

  switch (room->number)
  {
  case 2000001:
    if ((!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT) || rol_command_sentinel_cage_allows(cmd))
      return FALSE;
    send_to_char(ch, "The wardens of the cage disallow all commands except say, petition, project, "
                     "and help.\r\n");
    return TRUE;
  case 2046990:
    if (!IS_MOVE(cmd) || complete_cmd_info[cmd].subcmd != DOWN)
      return FALSE;
    if (rol_command_sentinel_is_necromancer(ch))
    {
      send_to_char(ch, "You feel the slight tingle of magic from the glyph as you descend.\r\n");
      return FALSE;
    }
    act("$n is blasted by a bright red bolt of magical energy from the glyph of warding!", FALSE,
        ch, NULL, NULL, TO_ROOM);
    send_to_char(ch, "A powerful bolt of bright red energy blasts you backwards several feet!\r\n");
    damage_amount = rol_command_sentinel_glyph_damage(ch);
    GET_HIT(ch) = MAX(1, GET_HIT(ch) - damage_amount);
    return TRUE;
  default:
    return FALSE;
  }
}

int rol_command_sentinel(struct char_data *ch, void *me, int cmd, const char *argument)
{
  (void)ch;
  (void)me;
  (void)cmd;
  (void)argument;

  /* Owner type is required, so direct legacy calls fail safely. */
  return FALSE;
}

int rol_command_sentinel_typed(struct spec_event_context *context)
{
  if (context == NULL || context->event != SPEC_EVENT_COMMAND)
    return FALSE;

  if (context->owner_type == SPEC_OWNER_MOBILE)
    return rol_command_sentinel_mobile(context->actor, context->owner, context->command);
  if (context->owner_type == SPEC_OWNER_ROOM)
    return rol_command_sentinel_room(context->actor, context->owner, context->command);

  return FALSE;
}

static const struct rol_toll_keeper_profile *rol_toll_keeper_profile_for(int mobile_vnum)
{
  size_t index;

  for (index = 0; index < sizeof(rol_toll_keeper_profiles) / sizeof(rol_toll_keeper_profiles[0]);
       index++)
    if (rol_toll_keeper_profiles[index].mobile_vnum == mobile_vnum)
      return &rol_toll_keeper_profiles[index];

  return NULL;
}

int rol_toll_keeper_fee_gold(int mobile_vnum)
{
  const struct rol_toll_keeper_profile *profile = rol_toll_keeper_profile_for(mobile_vnum);

  return profile != NULL ? profile->fee_gold : 0;
}

int rol_toll_keeper_destination(int mobile_vnum, bool first_side)
{
  const struct rol_toll_keeper_profile *profile = rol_toll_keeper_profile_for(mobile_vnum);

  if (profile == NULL)
    return -1;

  return first_side || profile->destination_b < 0 ? profile->destination_a : profile->destination_b;
}

bool rol_toll_keeper_ticket_matches(int mobile_vnum, int room_vnum, int entered_object_vnum,
                                    int ticket_vnum)
{
  const struct rol_toll_keeper_profile *profile = rol_toll_keeper_profile_for(mobile_vnum);

  return profile != NULL && profile->kind == ROL_TOLL_KEEPER_TICKET &&
         profile->room_vnum == room_vnum && profile->entered_object_vnum == entered_object_vnum &&
         profile->ticket_vnum == ticket_vnum;
}

bool rol_toll_keeper_payment_syntax_valid(int mobile_vnum, const char *argument)
{
  const struct rol_toll_keeper_profile *profile = rol_toll_keeper_profile_for(mobile_vnum);
  char amount_text[MAX_INPUT_LENGTH];
  char currency[MAX_INPUT_LENGTH];
  const char *remainder;

  if (profile == NULL || profile->kind != ROL_TOLL_KEEPER_FEE_GATE || argument == NULL)
    return false;
  if (profile->mobile_vnum != 2007210)
    return true;

  remainder = one_argument(argument, amount_text, sizeof(amount_text));
  one_argument(remainder, currency, sizeof(currency));
  return is_number(amount_text) &&
         (!str_cmp(currency, "gold") || !str_cmp(currency, "coin") || !str_cmp(currency, "coins"));
}

static void rol_toll_keeper_social(struct char_data *keeper, const char *social)
{
  int command;

  if (keeper == NULL || social == NULL || (command = find_command(social)) < 0)
    return;

  do_action(keeper, "", command, 0);
}

static int rol_toll_keeper_activity(struct char_data *keeper)
{
  const struct rol_toll_keeper_profile *profile;
  int roll;

  if (keeper == NULL || !IS_NPC(keeper) || !AWAKE(keeper) || FIGHTING(keeper) != NULL ||
      !VALID_ROOM_RNUM(IN_ROOM(keeper)) ||
      (profile = rol_toll_keeper_profile_for(GET_MOB_VNUM(keeper))) == NULL ||
      (int)GET_ROOM_VNUM(IN_ROOM(keeper)) != profile->room_vnum)
    return FALSE;

  switch (profile->mobile_vnum)
  {
  case 2007210:
    roll = rand_number(0, 80);
    switch (roll)
    {
    case 0:
      do_say(keeper, "Welcome to the Barony of Bloodstone!", 0, 0);
      rol_toll_keeper_social(keeper, "smile");
      return TRUE;
    case 1:
      do_say(keeper, "I am the official tax collector of the Keep.", 0, 0);
      do_say(keeper, "You must pay me 20 gold coins as tribute to enter.", 0, 0);
      return TRUE;
    case 2:
      do_say(keeper, "I will not let you pass unless you pay the tribute!", 0, 0);
      return TRUE;
    case 3:
      do_say(keeper, "The Keep is far to the north.", 0, 0);
      rol_toll_keeper_social(keeper, "smile");
      return TRUE;
    case 4:
      act("$n holds out $s hand for some coins.", TRUE, keeper, NULL, NULL, TO_ROOM);
      return TRUE;
    case 5:
      do_say(keeper, "Pay now, or die now; 'tis a simple choice, is it not?", 0, 0);
      rol_toll_keeper_social(keeper, "cackle");
      return TRUE;
    default:
      return FALSE;
    }
  case 2007335:
    roll = rand_number(0, 60);
    switch (roll)
    {
    case 0:
      do_say(keeper, "Wait a minute, you don't look old enough to enter!", 0, 0);
      return TRUE;
    case 1:
      do_say(keeper, "If I hear any complaints, I'll come in after ya!", 0, 0);
      return TRUE;
    case 2:
      do_say(keeper, "In or out! Don't crowd the alley.", 0, 0);
      return TRUE;
    case 3:
      do_say(keeper, "HA! Laughter is what you will hear if you shed those clothes.", 0, 0);
      return TRUE;
    case 4:
      do_say(keeper, "Be sure to carry a heavy purse.", 0, 0);
      return TRUE;
    default:
      return FALSE;
    }
  case 2011542:
    roll = rand_number(0, 80);
    switch (roll)
    {
    case 0:
      do_say(keeper, "Welcome to Paradise!", 0, 0);
      rol_toll_keeper_social(keeper, "cackle");
      return TRUE;
    case 1:
      rol_toll_keeper_social(keeper, "sing");
      do_say(keeper, "Gimme lots of money, or I'm gonna eat you....", 0, 0);
      return TRUE;
    case 2:
      do_say(keeper, "I will not let you pass unless you pay the tribute!", 0, 0);
      return TRUE;
    case 3:
      do_say(keeper, "Paradise lies ahead...", 0, 0);
      rol_toll_keeper_social(keeper, "smile");
      return TRUE;
    case 4:
      act("$n holds out $s hand for some coins.", TRUE, keeper, NULL, NULL, TO_ROOM);
      return TRUE;
    case 5:
      do_say(keeper, "Pay now, or die now; 'tis a simple choice, is it not?", 0, 0);
      rol_toll_keeper_social(keeper, "cackle");
      return TRUE;
    default:
      return FALSE;
    }
  default:
    return FALSE;
  }
}

static bool rol_toll_keeper_move(struct char_data *ch,
                                 const struct rol_toll_keeper_profile *profile, bool first_side)
{
  room_rnum destination;
  int destination_vnum;

  destination_vnum =
      first_side || profile->destination_b < 0 ? profile->destination_a : profile->destination_b;
  destination = real_room(destination_vnum);
  if (!VALID_ROOM_RNUM(destination))
  {
    log("SYSERR: RoL toll keeper %d has invalid destination %d", profile->mobile_vnum,
        destination_vnum);
    send_to_char(ch, "The way beyond is unavailable. Please tell a staff member.\r\n");
    return false;
  }

  char_from_room(ch);
  char_to_room(ch, destination);
  switch (profile->mobile_vnum)
  {
  case 2007210:
    act("$n arrives from the south.", TRUE, ch, NULL, NULL, TO_ROOM);
    break;
  case 2007335:
    act("$n enters from the north.", TRUE, ch, NULL, NULL, TO_ROOM);
    break;
  case 2011542:
    act("$n arrives from the passage below.", TRUE, ch, NULL, NULL, TO_ROOM);
    break;
  default:
    act("$n lands in a pile here from the direction of the bridge!", TRUE, ch, NULL, NULL, TO_ROOM);
    break;
  }
  return true;
}

static void rol_toll_keeper_demand(struct char_data *keeper,
                                   const struct rol_toll_keeper_profile *profile)
{
  switch (profile->mobile_vnum)
  {
  case 2007210:
    do_say(keeper, "You cannot enter the Baron's realm without paying homage to my lord!", 0, 0);
    do_say(keeper, "The price to enter is 20 gold coins!", 0, 0);
    break;
  case 2007335:
    do_say(keeper, "Sorry, friend, but you'll have to pay to enter!", 0, 0);
    do_say(keeper, "The price is 10 gold coins.", 0, 0);
    break;
  case 2011542:
    do_say(keeper, "Paradise is off limits to freeloaders!", 0, 0);
    do_say(keeper, "The price to enter is 500 gold coins!", 0, 0);
    break;
  default:
    break;
  }
}

static void rol_toll_keeper_underpayment(struct char_data *keeper,
                                         const struct rol_toll_keeper_profile *profile)
{
  char message[128];

  snprintf(message, sizeof(message), "The entry fee is %d gold coins. Try again.",
           profile->fee_gold);
  do_say(keeper, message, 0, 0);
}

static void rol_toll_keeper_approve(struct char_data *keeper, struct char_data *ch,
                                    const struct rol_toll_keeper_profile *profile, bool npc)
{
  switch (profile->mobile_vnum)
  {
  case 2007210:
    do_say(keeper, "'Tis wise of you to part with your money instead of your head.", 0, 0);
    do_say(keeper, "Welcome to the Barony of Bloodstone.", 0, 0);
    act("$n steps aside to let you pass northwards.", FALSE, keeper, NULL, ch, TO_VICT);
    act("$n steps aside to let $N pass northwards.", TRUE, keeper, NULL, ch, TO_NOTVICT);
    break;
  case 2007335:
    do_say(keeper, "That'll do!", 0, 0);
    act(npc ? "$n steps aside to let $N enter." : "$n steps aside to let you enter.", FALSE, keeper,
        NULL, ch, npc ? TO_NOTVICT : TO_VICT);
    if (!npc)
      act("$n steps aside to let $N enter.", TRUE, keeper, NULL, ch, TO_NOTVICT);
    break;
  case 2011542:
    do_say(keeper, "Welcome to Paradise.", 0, 0);
    act(npc ? "$n steps aside to let $N up the passage."
            : "$n steps aside to let you continue up the passage.",
        FALSE, keeper, NULL, ch, npc ? TO_NOTVICT : TO_VICT);
    if (!npc)
      act("$n steps aside to let $N further up the passage.", TRUE, keeper, NULL, ch, TO_NOTVICT);
    break;
  default:
    break;
  }
}

static int rol_toll_keeper_fee_gate(struct char_data *ch, struct char_data *keeper, int cmd,
                                    const char *argument,
                                    const struct rol_toll_keeper_profile *profile)
{
  int before_gold;
  int paid;

  if (!AWAKE(keeper) || FIGHTING(keeper) != NULL)
    return FALSE;

  if (IS_MOVE(cmd) && complete_cmd_info[cmd].subcmd == profile->direction)
  {
    if (IS_NPC(ch))
    {
      act("$N gives $n some money.", TRUE, keeper, NULL, ch, TO_NOTVICT);
      rol_toll_keeper_approve(keeper, ch, profile, true);
      rol_toll_keeper_move(ch, profile, true);
      return TRUE;
    }
    act("$n stops you.", FALSE, keeper, NULL, ch, TO_VICT);
    act("$n stops $N.", TRUE, keeper, NULL, ch, TO_NOTVICT);
    rol_toll_keeper_demand(keeper, profile);
    return TRUE;
  }

  if (!CMD_IS("give"))
    return FALSE;

  if (!rol_toll_keeper_payment_syntax_valid(profile->mobile_vnum, argument))
  {
    do_say(keeper, "Are you mocking me? Pay the tribute in gold coins.", 0, 0);
    rol_toll_keeper_social(keeper, "push");
    return TRUE;
  }

  before_gold = GET_GOLD(keeper);
  do_give(ch, argument, cmd, 0);
  paid = GET_GOLD(keeper) - before_gold;
  if (paid < profile->fee_gold)
  {
    rol_toll_keeper_underpayment(keeper, profile);
    return TRUE;
  }

  rol_toll_keeper_approve(keeper, ch, profile, false);
  rol_toll_keeper_move(ch, profile, true);
  return TRUE;
}

static int rol_toll_keeper_bridge(struct char_data *ch, struct char_data *keeper, int cmd,
                                  const char *argument,
                                  const struct rol_toll_keeper_profile *profile)
{
  struct char_data *first;
  int before_gold;
  int paid;

  if (!CMD_IS("give") || ch == keeper)
    return FALSE;

  before_gold = GET_GOLD(keeper);
  do_give(ch, argument, cmd, 0);
  paid = GET_GOLD(keeper) - before_gold;
  if (paid == 0)
    return TRUE;
  if (paid < profile->fee_gold)
  {
    do_say(keeper, "You STILL need to pay me 5 gold coins, pal.", 0, 0);
    return TRUE;
  }

  for (first = world[IN_ROOM(keeper)].people; first != NULL && first != keeper && first != ch;
       first = first->next_in_room)
    ;
  rol_toll_keeper_social(keeper, "smile");
  act("$N picks you up and tosses you to the other side of the bridge!", FALSE, ch, NULL, keeper,
      TO_CHAR);
  act("$N throws $n to the other side of the bridge!", TRUE, ch, NULL, keeper, TO_NOTVICT);
  if (rol_toll_keeper_move(ch, profile, first == keeper))
    GET_POS(ch) = POS_SITTING;
  return TRUE;
}

static struct obj_data *rol_toll_keeper_ticket(struct char_data *ch, struct char_data *keeper,
                                               const struct rol_toll_keeper_profile *profile)
{
  struct obj_data *ticket;

  for (ticket = ch->carrying; ticket != NULL; ticket = ticket->next_content)
    if ((int)GET_OBJ_VNUM(ticket) == profile->ticket_vnum)
      return ticket;
  for (ticket = keeper->carrying; ticket != NULL; ticket = ticket->next_content)
    if ((int)GET_OBJ_VNUM(ticket) == profile->ticket_vnum)
      return ticket;

  return NULL;
}

static int rol_toll_keeper_ticket_taker(struct char_data *ch, struct char_data *keeper, int cmd,
                                        const char *argument,
                                        const struct rol_toll_keeper_profile *profile)
{
  struct obj_data *entered;
  struct obj_data *ticket;
  char name[MAX_INPUT_LENGTH];

  if (!CMD_IS("enter") || !AWAKE(keeper) || !CAN_SEE(keeper, ch))
    return FALSE;

  one_argument(argument, name, sizeof(name));
  if (!*name)
    return FALSE;
  entered = get_obj_in_list_vis(keeper, name, NULL, world[IN_ROOM(keeper)].contents);
  if (entered == NULL || (int)GET_OBJ_VNUM(entered) != profile->entered_object_vnum)
    return FALSE;

  ticket = rol_toll_keeper_ticket(ch, keeper, profile);
  if (ticket == NULL)
  {
    act("$N says, 'You must have a ticket to proceed.'", FALSE, ch, NULL, keeper, TO_CHAR);
    return TRUE;
  }

  if (ticket->carried_by == ch)
  {
    act("$N tears up the ticket in your hand.", FALSE, ch, NULL, keeper, TO_CHAR);
    act("$N tears up the ticket in $n's hand.", FALSE, ch, NULL, keeper, TO_ROOM);
  }
  else
    act("$n tears up the ticket.", FALSE, keeper, NULL, NULL, TO_ROOM);
  obj_from_char(ticket);
  extract_obj(ticket);
  act("Then $E says to you, 'You may proceed.'", FALSE, ch, NULL, keeper, TO_CHAR);
  act("Then $E says to $n, 'You may proceed.'", FALSE, ch, NULL, keeper, TO_ROOM);
  return FALSE;
}

static int rol_toll_keeper_command(struct char_data *ch, struct char_data *keeper, int cmd,
                                   const char *argument)
{
  const struct rol_toll_keeper_profile *profile;

  if (ch == NULL || keeper == NULL || !IS_NPC(keeper) || cmd <= 0 || argument == NULL ||
      complete_cmd_info == NULL || !VALID_ROOM_RNUM(IN_ROOM(keeper)) ||
      (profile = rol_toll_keeper_profile_for(GET_MOB_VNUM(keeper))) == NULL ||
      (int)GET_ROOM_VNUM(IN_ROOM(keeper)) != profile->room_vnum)
    return FALSE;

  switch (profile->kind)
  {
  case ROL_TOLL_KEEPER_FEE_GATE:
    return rol_toll_keeper_fee_gate(ch, keeper, cmd, argument, profile);
  case ROL_TOLL_KEEPER_BRIDGE:
    return rol_toll_keeper_bridge(ch, keeper, cmd, argument, profile);
  case ROL_TOLL_KEEPER_TICKET:
    return rol_toll_keeper_ticket_taker(ch, keeper, cmd, argument, profile);
  default:
    return FALSE;
  }
}

int rol_toll_keeper(struct char_data *ch, void *me, int cmd, const char *argument)
{
  (void)ch;
  (void)me;
  (void)cmd;
  (void)argument;

  /* Typed dispatch supplies the owner/event distinction. */
  return FALSE;
}

int rol_toll_keeper_typed(struct spec_event_context *context)
{
  if (context == NULL || context->owner_type != SPEC_OWNER_MOBILE)
    return FALSE;

  if (context->event == SPEC_EVENT_COMMAND)
    return rol_toll_keeper_command(context->actor, context->owner, context->command,
                                   context->argument);
  if (context->event == SPEC_EVENT_MOBILE_ACTIVITY)
    return rol_toll_keeper_activity(context->owner);

  return FALSE;
}

enum rol_banana_peel_outcome rol_banana_peel_classify(int intelligence_roll, int dexterity_roll)
{
  if (intelligence_roll > 4)
    return ROL_BANANA_PEEL_AVOID;
  if (dexterity_roll == 1)
    return ROL_BANANA_PEEL_KNOCKOUT;
  if (dexterity_roll >= 2 && dexterity_roll <= 5)
    return ROL_BANANA_PEEL_FALL;
  if (dexterity_roll >= 6 && dexterity_roll <= 10)
    return ROL_BANANA_PEEL_STUMBLE;
  return ROL_BANANA_PEEL_DANCE;
}

static bool rol_banana_attacker_is_aggressive(struct char_data *attacker, struct char_data *victim)
{
  if (!IS_NPC(attacker))
    return false;

  return MOB_FLAGGED(attacker, MOB_AGGRESSIVE) ||
         (MOB_FLAGGED(attacker, MOB_ROL_AGGR_RACE_EVIL) && rol_race_is_evil(GET_RACE(victim))) ||
         (MOB_FLAGGED(attacker, MOB_ROL_AGGR_RACE_GOOD) && rol_race_is_good(GET_RACE(victim))) ||
         (MOB_FLAGGED(attacker, MOB_AGGR_EVIL) && IS_EVIL(victim)) ||
         (MOB_FLAGGED(attacker, MOB_AGGR_NEUTRAL) && IS_NEUTRAL(victim)) ||
         (MOB_FLAGGED(attacker, MOB_AGGR_GOOD) && IS_GOOD(victim));
}

static void rol_banana_stop_merciful_attackers(struct char_data *ch)
{
  struct char_data *attacker;
  struct char_data *next_attacker;

  for (attacker = combat_list; attacker != NULL; attacker = next_attacker)
  {
    next_attacker = attacker->next_fighting;
    if (FIGHTING(attacker) == ch && !rol_banana_attacker_is_aggressive(attacker, ch))
      stop_fighting(attacker);
  }
}

static void rol_banana_apply_sleep(struct char_data *ch)
{
  struct affected_type af;

  new_affect(&af);
  af.spell = SPELL_SLEEP;
  af.duration = rand_number(4, 6);
  SET_BIT_AR(af.bitvector, AFF_SLEEP);
  affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  if (FIGHTING(ch) != NULL)
    stop_fighting(ch);
  rol_banana_stop_merciful_attackers(ch);
  if (GET_POS(ch) > POS_SLEEPING)
    change_position(ch, POS_SLEEPING);
}

static int rol_banana_eat(struct spec_event_context *context, struct obj_data *obj,
                          struct char_data *ch, const char *argument)
{
  struct obj_data *peel;
  char name[MAX_INPUT_LENGTH];

  if (GET_OBJ_VNUM(obj) != ROL_BANANA_FRUIT_VNUM || IS_NPC(ch))
    return FALSE;

  one_argument(argument, name, sizeof(name));
  if (strcasecmp(name, "banana") != 0)
    return FALSE;

  if (GET_COND(ch, HUNGER) > 20)
  {
    send_to_char(ch, "No thanks, you are absolutely stuffed and cannot eat another bite.\r\n");
    return TRUE;
  }

  act("$n eats $p, then arrogantly tosses the peel on the ground to rot.", TRUE, ch, obj, 0,
      TO_ROOM);
  act("You eat $p, then arrogantly toss the peel on the ground to rot.", FALSE, ch, obj, 0,
      TO_CHAR);
  gain_condition(ch, HUNGER, GET_OBJ_VAL(obj, 0));
  WAIT_STATE(ch, PULSE_VIOLENCE);
  if (GET_COND(ch, HUNGER) > 20)
    send_to_char(ch, "You feel comfortably sated.\r\n");

  extract_obj(obj);
  context->invalidation |= SPEC_INVALIDATE_OWNER;

  peel = read_object(ROL_BANANA_PEEL_VNUM, VIRTUAL);
  if (peel == NULL)
    return TRUE;
  SET_BIT_AR(GET_OBJ_EXTRA(peel), ITEM_DECAY);
  GET_OBJ_TIMER(peel) = ROL_BANANA_PEEL_DECAY_TICKS;
  obj_to_room(peel, IN_ROOM(ch));
  return TRUE;
}

static int rol_banana_move(struct obj_data *obj, struct char_data *ch, int cmd)
{
  enum rol_banana_peel_outcome outcome;
  int intelligence_roll;
  int dexterity_roll;

  if (GET_OBJ_VNUM(obj) != ROL_BANANA_PEEL_VNUM || complete_cmd_info == NULL || !IS_MOVE(cmd) ||
      GET_LEVEL(ch) >= LVL_IMMORT || RIDING(ch) != NULL || is_flying(ch) ||
      AFF_FLAGGED(ch, AFF_LEVITATE))
    return FALSE;

  intelligence_roll = rand_number(1, MAX(1, GET_INT(ch)));
  if (intelligence_roll > 4)
    return FALSE;
  dexterity_roll = rand_number(1, MAX(1, GET_DEX(ch)));
  outcome = rol_banana_peel_classify(intelligence_roll, dexterity_roll);

  switch (outcome)
  {
  case ROL_BANANA_PEEL_AVOID:
    return FALSE;
  case ROL_BANANA_PEEL_KNOCKOUT:
    act("You slip on a banana peel, fall, and pass out when your head hits the ground!", FALSE, ch,
        0, 0, TO_CHAR);
    act("$n slips on a banana peel, falls, and passes out when $s head hits the ground!", TRUE, ch,
        0, 0, TO_ROOM);
    rol_banana_apply_sleep(ch);
    GET_HIT(ch) = MAX(1, GET_HIT(ch) - 15);
    return TRUE;
  case ROL_BANANA_PEEL_FALL:
    GET_HIT(ch) = MAX(1, GET_HIT(ch) - dexterity_roll);
    act("You slip on a banana peel and fall over with a shriek and a thump!", TRUE, ch, 0, 0,
        TO_CHAR);
    act("$n slips on a banana peel, shrieks, and falls over!", TRUE, ch, 0, 0, TO_ROOM);
    change_position(ch, POS_SITTING);
    WAIT_STATE(ch, PULSE_VIOLENCE);
    return TRUE;
  case ROL_BANANA_PEEL_STUMBLE:
    send_to_char(ch, "You step on a banana peel!\r\n"
                     "Arms flailing, you barely maintain your balance.\r\n");
    act("$n steps on a banana peel and, arms flailing wildly, barely maintains $s balance.", TRUE,
        ch, 0, 0, TO_ROOM);
    WAIT_STATE(ch, PULSE_VIOLENCE);
    return TRUE;
  case ROL_BANANA_PEEL_DANCE:
    send_to_char(ch, "You step on a banana peel, but dance your way out of danger.\r\n");
    act("$n steps on a banana peel, but with a quick smirk resumes $s travel.", TRUE, ch, 0, 0,
        TO_ROOM);
    return FALSE;
  default:
    return FALSE;
  }
}

int rol_banana(struct char_data *ch, void *me, int cmd, const char *argument)
{
  (void)ch;
  (void)me;
  (void)cmd;
  (void)argument;

  /* Typed dispatch supplies exact owner identity and invalidation. */
  return FALSE;
}

int rol_banana_typed(struct spec_event_context *context)
{
  struct char_data *ch;
  struct obj_data *obj;
  int cmd;

  if (context == NULL || context->owner_type != SPEC_OWNER_OBJECT ||
      context->event != SPEC_EVENT_COMMAND)
    return FALSE;

  ch = context->actor;
  obj = context->owner;
  cmd = context->command;
  if (ch == NULL || obj == NULL || !AWAKE(ch) || cmd <= 0 || complete_cmd_info == NULL)
    return FALSE;

  if (CMD_IS("eat"))
    return rol_banana_eat(context, obj, ch, context->argument);
  return rol_banana_move(obj, ch, cmd);
}

static const struct rol_undead_drain_profile *rol_undead_drain_profile_for(int mobile_vnum)
{
  size_t index;

  for (index = 0; index < sizeof(rol_undead_drain_profiles) / sizeof(rol_undead_drain_profiles[0]);
       index++)
    if (rol_undead_drain_profiles[index].mobile_vnum == mobile_vnum)
      return &rol_undead_drain_profiles[index];

  return NULL;
}

bool rol_undead_drain_profile(int mobile_vnum, int *chance_sides, int *marker_affect,
                              int *armor_penalty, int *dexterity_penalty, int *strength_penalty,
                              int *will_penalty, int *fortitude_penalty, int *slow_duration)
{
  const struct rol_undead_drain_profile *profile = rol_undead_drain_profile_for(mobile_vnum);

  if (profile == NULL)
    return false;

  if (chance_sides != NULL)
    *chance_sides = profile->chance_sides;
  if (marker_affect != NULL)
    *marker_affect = profile->marker_affect;
  if (armor_penalty != NULL)
    *armor_penalty = profile->armor_penalty;
  if (dexterity_penalty != NULL)
    *dexterity_penalty = profile->dexterity_penalty;
  if (strength_penalty != NULL)
    *strength_penalty = profile->strength_penalty;
  if (will_penalty != NULL)
    *will_penalty = profile->will_penalty;
  if (fortitude_penalty != NULL)
    *fortitude_penalty = profile->fortitude_penalty;
  if (slow_duration != NULL)
    *slow_duration = profile->slow_duration;
  return true;
}

static void rol_undead_drain_affect(struct char_data *victim, int marker_affect, int duration,
                                    int location, int modifier, bool slows)
{
  struct affected_type af;

  if (modifier == 0 && !slows)
    return;

  new_affect(&af);
  af.spell = marker_affect;
  af.duration = duration;
  af.location = location;
  af.modifier = modifier;
  af.bonus_type = BONUS_TYPE_UNIVERSAL;
  if (slows)
    SET_BIT_AR(af.bitvector, AFF_SLOW);
  affect_to_char(victim, &af);
}

static void rol_undead_drain_apply(const struct rol_undead_drain_profile *profile,
                                   struct char_data *victim)
{
  struct affected_type slow;
  int duration = rand_number(2, 3);

  rol_undead_drain_affect(victim, profile->marker_affect, duration, APPLY_AC_NEW,
                          profile->armor_penalty, false);
  rol_undead_drain_affect(victim, profile->marker_affect, duration, APPLY_DEX,
                          profile->dexterity_penalty, false);
  rol_undead_drain_affect(victim, profile->marker_affect, duration, APPLY_STR,
                          profile->strength_penalty, profile->slow_duration < 0);
  rol_undead_drain_affect(victim, profile->marker_affect, duration, APPLY_SAVING_WILL,
                          profile->will_penalty, false);
  rol_undead_drain_affect(victim, profile->marker_affect, duration, APPLY_SAVING_FORT,
                          profile->fortitude_penalty, false);

  if (profile->slow_duration > 0)
  {
    new_affect(&slow);
    slow.spell = SPELL_SLOW;
    slow.duration = profile->slow_duration;
    SET_BIT_AR(slow.bitvector, AFF_SLOW);
    affect_to_char(victim, &slow);
  }
}

int rol_undead_drain(struct char_data *ch, void *me, int cmd, const char *argument)
{
  const struct rol_undead_drain_profile *profile;
  struct char_data *victim;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || !IS_NPC(ch) || cmd != 0 || FIGHTING(ch) == NULL)
    return FALSE;

  profile = rol_undead_drain_profile_for(GET_MOB_VNUM(ch));
  victim = FIGHTING(ch);
  if (profile == NULL || IN_ROOM(victim) != IN_ROOM(ch) || IS_UNDEAD(victim) ||
      affected_by_spell(victim, profile->marker_affect) ||
      affected_by_spell(victim, SPELL_DEATH_WARD) ||
      rand_number(0, profile->chance_sides - 1) != 0 ||
      savingthrow(ch, victim, SAVING_WILL, -5, CAST_INNATE, GET_LEVEL(ch), ENCHANTMENT))
    return FALSE;

  rol_undead_drain_apply(profile, victim);
  act(profile->victim_message, TRUE, ch, NULL, victim, TO_VICT);
  act(profile->room_message, TRUE, ch, NULL, victim, TO_NOTVICT);
  act(profile->attacker_message, TRUE, ch, NULL, victim, TO_CHAR);
  return FALSE;
}

int rol_shadow_giant_spook_damage(bool save_succeeded)
{
  int amount = dice(25, 8);

  return save_succeeded ? amount / 2 : amount;
}

bool rol_shadow_giant_spook_immune(struct char_data *target)
{
  if (target == NULL)
    return true;

  if (IS_UNDEAD(target) || IS_DRAGON(target))
    return true;

  return IS_NPC(target) &&
         (MOB_FLAGGED(target, MOB_ROL_DEMON) || MOB_FLAGGED(target, MOB_ROL_DEVIL) ||
          MOB_FLAGGED(target, MOB_ROL_ANGEL) || HAS_SUBRACE(target, SUBRACE_ANGEL));
}

bool rol_shadow_giant_stun_succeeds(int level, int chance_roll, int penalty_roll)
{
  return chance_roll < (level * 2) - penalty_roll;
}

static void rol_shadow_giant_spook(struct char_data *ch, struct char_data *target)
{
  bool saved;
  int amount;

  if (rol_shadow_giant_spook_immune(target))
  {
    act("$N laughs as you attempt to spook $M.", TRUE, ch, NULL, target, TO_CHAR);
    return;
  }

  saved = savingthrow(ch, target, SAVING_WILL, 0, CAST_INNATE, 30, ILLUSION);
  amount = rol_shadow_giant_spook_damage(saved);
  damage(ch, target, amount, -1, DAM_MENTAL, FALSE);

  if (GET_POS(target) <= POS_DEAD || !can_stun(target) || char_has_mud_event(target, eSTUNNED) ||
      !rol_shadow_giant_stun_succeeds(GET_LEVEL(ch), rand_number(1, 100), rand_number(1, 5)))
    return;

  attach_mud_event(new_mud_event(eSTUNNED, target, NULL), PULSE_VIOLENCE * rand_number(1, 3));
}

int rol_shadow_giant(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *target;
  struct char_data *next;

  UNUSED(me);
  UNUSED(argument);

  if (ch == NULL || !IS_NPC(ch) || cmd || FIGHTING(ch) == NULL || !VALID_ROOM_RNUM(IN_ROOM(ch)) ||
      rand_number(0, 20) != 0)
    return FALSE;

  act("You pull your face off and scare the bejezus out of $N.", FALSE, ch, NULL, FIGHTING(ch),
      TO_CHAR);
  act("The Shadow Giant reaches up and pulls his face off.", FALSE, ch, NULL, FIGHTING(ch),
      TO_ROOM);

  for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (IS_NPC(target) && !IS_PET(target))
      continue;
    rol_shadow_giant_spook(ch, target);
  }

  return FALSE;
}

static const struct rol_weapon_profile *rol_weapon_profile_for(int object_vnum)
{
  size_t index;

  for (index = 0; index < sizeof(rol_weapon_profiles) / sizeof(rol_weapon_profiles[0]); index++)
    if (rol_weapon_profiles[index].object_vnum == object_vnum)
      return &rol_weapon_profiles[index];

  return NULL;
}

size_t rol_weapon_profile_count(void)
{
  return sizeof(rol_weapon_profiles) / sizeof(rol_weapon_profiles[0]);
}

bool rol_weapon_profile(int object_vnum, int *proc_denominator, bool *critical_only,
                        const char **description)
{
  const struct rol_weapon_profile *profile = rol_weapon_profile_for(object_vnum);

  if (profile == NULL)
    return false;
  if (proc_denominator != NULL)
    *proc_denominator = profile->proc_denominator;
  if (critical_only != NULL)
    *critical_only = profile->critical_only;
  if (description != NULL)
    *description = profile->description;
  return true;
}

static int rol_weapon_slot(const struct char_data *ch, const struct obj_data *obj)
{
  int wear;

  if (ch == NULL || obj == NULL)
    return -1;
  for (wear = 0; wear < NUM_WEARS; wear++)
    if (GET_EQ(ch, wear) == obj)
      return wear;
  return -1;
}

static bool rol_weapon_primary_slot(int slot)
{
  return slot == WEAR_WIELD_1 || slot == WEAR_WIELD_2H;
}

static bool rol_weapon_sneak_attack(int attack_type)
{
  return attack_type == ATTACK_TYPE_PRIMARY_SNEAK || attack_type == ATTACK_TYPE_OFFHAND_SNEAK;
}

static struct spec_damage_result rol_weapon_damage(struct char_data *ch, struct char_data *victim,
                                                   int amount, int damage_type)
{
  return spec_damage_current_target(ch, victim, MAX(0, amount), -1, damage_type, FALSE);
}

static int rol_weapon_cast(struct char_data *ch, struct obj_data *obj, struct char_data *victim,
                           int spell, int level)
{
  if (ch == NULL || victim == NULL)
    return 0;
  return call_magic(ch, victim, obj, spell, 0, MAX(1, level), CAST_WEAPON_SPELL);
}

static void rol_weapon_summon_reclaimer(struct char_data *ch, struct obj_data *obj)
{
  struct char_data *summoned;
  mob_rnum rnum;

  if (ch == NULL || IN_ROOM(ch) == NOWHERE)
    return;
  if ((rnum = real_mobile(ROL_GITH_RECLAIMER_VNUM)) == NOBODY)
  {
    log("SYSERR: RoL weapon proc cannot find Gith reclaimer mobile %d", ROL_GITH_RECLAIMER_VNUM);
    return;
  }
  if ((summoned = read_mobile(rnum, REAL)) == NULL)
    return;

  char_to_room(summoned, IN_ROOM(ch));
  act("A thunderclap announces a Githyanki knight arriving to reclaim $p!", FALSE, summoned, obj,
      NULL, TO_ROOM);
  if (FIGHTING(summoned) == NULL)
    set_fighting(summoned, ch);
}

static int rol_weapon_hammer(struct char_data *ch, struct obj_data *obj, struct char_data *victim)
{
  struct char_data *next;
  struct char_data *target;

  if (rand_number(0, 21) != 0)
    return FALSE;

  act("Your $p glows brightly as lightning bolts streak from it!", FALSE, ch, obj, victim, TO_CHAR);
  act("$n's $p glows brightly as lightning bolts streak from it!", FALSE, ch, obj, victim, TO_ROOM);
  rol_weapon_cast(ch, obj, victim, SPELL_LIGHTNING_BOLT, 51);

  if (!VALID_ROOM_RNUM(IN_ROOM(ch)))
    return TRUE;
  for (target = world[IN_ROOM(ch)].people; target != NULL; target = next)
  {
    next = target->next_in_room;
    if (target == victim || target == ch || rand_number(0, 3) != 0 || !CAN_SEE(ch, target) ||
        !aoeOK(ch, target, SPELL_LIGHTNING_BOLT))
      continue;
    rol_weapon_cast(ch, obj, target, SPELL_LIGHTNING_BOLT, 51);
  }
  return TRUE;
}

static int rol_weapon_githyanki_two_handed(struct char_data *ch, struct obj_data *obj,
                                           struct char_data *victim)
{
  struct spec_damage_result result;
  bool instant = false;
  int amount = 0;

  if (rand_number(0, 22) == 0)
    amount = MIN(100, GET_LEVEL(ch) * 2) + GET_LEVEL(ch);
  if (amount == 0 && rand_number(0, 99) == 0)
  {
    if (GET_LEVEL(victim) < 51)
    {
      instant = true;
      amount = MAX(1000, GET_MAX_HIT(victim) * 2);
    }
    else
      amount = MIN(200, GET_LEVEL(ch) * 4) + GET_LEVEL(ch);
  }
  if (amount == 0)
    return FALSE;

  act("The silver nimbus around your $p flares and cuts deeply into $N!", FALSE, ch, obj, victim,
      TO_CHAR);
  result = rol_weapon_damage(ch, victim, amount, DAM_SLASHING);
  if (result.status == SPEC_DAMAGE_TARGET_INVALIDATED && !IS_NPC(ch) &&
      rand_number(0, instant ? 9 : 99) == 0)
    rol_weapon_summon_reclaimer(ch, obj);
  return TRUE;
}

static int rol_weapon_githyanki_charged(struct spec_event_context *context, struct char_data *ch,
                                        struct obj_data *obj, struct char_data *victim)
{
  int slot;
  int choice;

  if (rand_number(0, 100) != 0)
    return FALSE;

  if (GET_OBJ_SPECTIMER(obj, ROL_GITH_CHARGE_TIMER_SLOT) == 0)
    GET_OBJ_SPECTIMER(obj, ROL_GITH_CHARGE_TIMER_SLOT) = -10;
  GET_OBJ_SPECTIMER(obj, ROL_GITH_CHARGE_TIMER_SLOT)++;
  if (GET_OBJ_SPECTIMER(obj, ROL_GITH_CHARGE_TIMER_SLOT) == 0)
  {
    act("A booming voice condemns your attempt to contain Gith's power!", FALSE, ch, obj, victim,
        TO_CHAR);
    act("$n's $p explodes into a thousand pieces and knocks $m flat!", FALSE, ch, obj, victim,
        TO_ROOM);
    change_position(ch, POS_SITTING);
    slot = rol_weapon_slot(ch, obj);
    if (slot >= 0)
      extract_obj(unequip_char(ch, slot));
    context->invalidation |= SPEC_INVALIDATE_OWNER;
    return TRUE;
  }

  choice = rand_number(1, 5);
  if (choice <= 3 && GET_LEVEL(victim) < 51)
  {
    act("Your Silver Sword sings through the air and seeks $N's neck!", FALSE, ch, obj, victim,
        TO_CHAR);
    (void)rol_weapon_damage(ch, victim, MAX(1000, GET_MAX_HIT(victim) * 2), DAM_SLASHING);
  }
  else if (choice >= 4 && !IS_NPC(ch))
    rol_weapon_summon_reclaimer(ch, obj);
  return TRUE;
}

static struct obj_data *rol_weapon_extra_attack_owner;

static void rol_weapon_extra_attacks(struct char_data *ch, struct obj_data *obj,
                                     struct char_data *victim, int count, int attack_type)
{
  int swing;

  rol_weapon_extra_attack_owner = obj;
  for (swing = 0; swing < count && GET_POS(ch) > POS_DEAD && FIGHTING(ch) == victim; swing++)
    hit(ch, victim, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, attack_type);
  rol_weapon_extra_attack_owner = NULL;
}

static int rol_weapon_valhalla(struct char_data *ch, struct obj_data *obj, struct char_data *victim,
                               int slot)
{
  bool offhand = slot == WEAR_WIELD_OFFHAND;
  bool troll = IS_HALF_TROLL(ch);
  int extra_swings = 0;

  if (rand_number(0, 28) == 0)
  {
    if (offhand)
      extra_swings = 1 + (rand_number(0, 2) == 0);
    else if (troll)
      extra_swings = 1;
    else
      extra_swings = 2 + (rand_number(0, 2) == 0);

    act("Ancestral power makes your $p reverse its swing and strike again!", FALSE, ch, obj, victim,
        TO_CHAR);
    rol_weapon_extra_attacks(ch, obj, victim, extra_swings,
                             offhand ? ATTACK_TYPE_OFFHAND : ATTACK_TYPE_PRIMARY);
  }

  if (((offhand && IS_RANGER(ch)) || (!offhand && troll)) && rand_number(0, 23) == 0)
  {
    act("Ancestral power pours through your $p and restores your vitality.", FALSE, ch, obj, NULL,
        TO_CHAR);
    if (!AFF_FLAGGED(ch, AFF_BLACKMANTLE))
      GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 50);
  }
  return extra_swings > 0;
}

static int rol_weapon_windsong(struct char_data *ch, struct obj_data *obj, struct char_data *victim,
                               int slot)
{
  int extra_swings;

  if (!IS_RANGER(ch))
  {
    act("Your $p sends waves of pain through you, forcing you to drop it!", FALSE, ch, obj, NULL,
        TO_CHAR);
    act("$n shrieks and drops $p!", FALSE, ch, obj, NULL, TO_ROOM);
    if (GET_HIT(ch) > 50)
      GET_HIT(ch) = 1;
    obj_to_room(unequip_char(ch, slot), IN_ROOM(ch));
    return TRUE;
  }
  if (rand_number(0, 32) != 0)
    return FALSE;

  extra_swings = 0;
  if (GET_RACE(ch) == RACE_ELF || GET_RACE(ch) == RACE_HIGH_ELF)
    extra_swings = 3;
  else if (GET_RACE(ch) == RACE_HALF_ELF)
    extra_swings = 2;
  if (rand_number(0, 2) == 0)
    extra_swings++;
  if (rand_number(0, 2) == 0)
    extra_swings++;
  if (rand_number(0, 3) == 0)
    extra_swings++;

  act("Your $p blurs as it comes to life and lashes repeatedly at $N!", FALSE, ch, obj, victim,
      TO_CHAR);
  rol_weapon_extra_attacks(ch, obj, victim, extra_swings,
                           slot == WEAR_WIELD_OFFHAND ? ATTACK_TYPE_OFFHAND : ATTACK_TYPE_PRIMARY);
  return TRUE;
}

static int rol_weapon_shadow_dagger(struct spec_event_context *context, struct char_data *ch,
                                    struct obj_data *obj, struct char_data *victim)
{
  struct spec_damage_result result;
  int amount;

  if (context->critical)
  {
    amount = MAX(1, (context->damage * 15) / 100);
    act("Dark shadowy tendrils flow from your $p and burrow into $N's wounds.", FALSE, ch, obj,
        victim, TO_CHAR);
    (void)rol_weapon_damage(ch, victim, amount, DAM_NEGATIVE);
    return TRUE;
  }
  if (!rol_weapon_sneak_attack(context->attack_type) || rand_number(0, 3) != 0)
    return FALSE;

  act("Swirling shadows lift your $p and drive it repeatedly into $N!", FALSE, ch, obj, victim,
      TO_CHAR);
  if (can_stun(victim) && !char_has_mud_event(victim, eSTUNNED))
    attach_mud_event(new_mud_event(eSTUNNED, victim, NULL), PULSE_VIOLENCE * 2);
  amount = rand_number(150, 200);
  result = rol_weapon_damage(ch, victim, amount, DAM_PUNCTURE);
  if (result.status != SPEC_DAMAGE_TARGET_INVALIDATED && !affected_by_spell(ch, SPELL_MAGE_ARMOR) &&
      !affected_by_spell(ch, SPELL_SHIELD))
    rol_weapon_cast(ch, obj, ch, SPELL_MAGE_ARMOR, 35);
  return TRUE;
}

static int rol_weapon_cold_burst(struct char_data *ch, struct obj_data *obj,
                                 struct char_data *victim, int dice_count, int dice_sides,
                                 const char *message)
{
  int amount = dice(dice_count, dice_sides);

  if (savingthrow(ch, victim, SAVING_REFL, 0, CAST_WEAPON_SPELL, MIN(30, GET_LEVEL(ch)), EVOCATION))
    amount /= 2;
  act(message, FALSE, ch, obj, victim, TO_CHAR);
  (void)rol_weapon_damage(ch, victim, amount, DAM_COLD);
  return TRUE;
}

static int rol_weapon_moonblade_command(struct char_data *ch, struct obj_data *obj, int cmd,
                                        const char *argument)
{
  struct char_data *target;

  if (cmd <= 0 || argument == NULL || (!CMD_IS("say") && !CMD_IS("'")))
    return FALSE;
  skip_spaces_c(&argument);
  if (str_cmp(argument, "labelas"))
    return FALSE;
  if (spec_context_validate_worn_object(ch, obj) != SPEC_CONTEXT_VALID)
    return FALSE;

  if (GET_OBJ_SPECTIMER(obj, 0) > 0)
  {
    send_to_char(ch, "The trees do not answer your call.\r\n");
    return TRUE;
  }

  act("You invoke Labelas, and protective bark flows toward your companions.", FALSE, ch, obj, NULL,
      TO_CHAR);
  for (target = world[IN_ROOM(ch)].people; target != NULL; target = target->next_in_room)
    if (target == ch || (GROUP(ch) != NULL && GROUP(target) == GROUP(ch)))
      rol_weapon_cast(ch, obj, target, SPELL_BARKSKIN, 30);
  GET_OBJ_SPECTIMER(obj, 0) = 168;
  return TRUE;
}

static int rol_weapon_crimson_drain(struct char_data *ch, struct obj_data *obj,
                                    struct char_data *victim)
{
  bool landed;
  int choice = rand_number(1, 3);

  if (choice == 1)
  {
    landed = !affected_by_spell(victim, SPELL_RAY_OF_ENFEEBLEMENT);
    rol_weapon_cast(ch, obj, victim, SPELL_RAY_OF_ENFEEBLEMENT, 30);
    landed = landed && affected_by_spell(victim, SPELL_RAY_OF_ENFEEBLEMENT);
    if (landed)
      rol_weapon_cast(ch, obj, ch, SPELL_STRENGTH, 10);
  }
  else
  {
    landed = !affected_by_spell(victim, SPELL_SLOW);
    rol_weapon_cast(ch, obj, victim, SPELL_SLOW, 30);
    landed = landed && affected_by_spell(victim, SPELL_SLOW);
    if (landed)
      rol_weapon_cast(ch, obj, ch, SPELL_GRACE, 10);
  }
  return TRUE;
}

static bool rol_weapon_arcane_caster(const struct char_data *ch)
{
  return IS_WIZARD(ch) || IS_SORCERER(ch) || IS_BARD(ch) || IS_NECROMANCER(ch) || IS_SUMMONER(ch) ||
         CLASS_LEVEL(ch, CLASS_WARLOCK) > 0;
}

static bool rol_weapon_grounded_target(struct char_data *victim)
{
  int sector;

  if (victim == NULL || IN_ROOM(victim) == NOWHERE || IS_DRAGON(victim) || IS_INCORPOREAL(victim) ||
      AFF_FLAGGED(victim, AFF_FLYING))
    return false;

  sector = SECT(IN_ROOM(victim));
  return sector != SECT_WATER_SWIM && sector != SECT_WATER_NOSWIM && sector != SECT_FLYING &&
         sector != SECT_UNDERWATER && sector != SECT_OCEAN && sector != SECT_UD_WATER &&
         sector != SECT_UD_NOSWIM && sector != SECT_UD_NOGROUND && sector != SECT_RIVER;
}

static int rol_weapon_mielikki(struct char_data *ch, struct obj_data *obj, struct char_data *victim,
                               int slot)
{
  if (!IS_RANGER(ch) && !IS_DRUID(ch))
  {
    act("Your $p glows brightly and stings you, forcing you to drop it!", FALSE, ch, obj, NULL,
        TO_CHAR);
    act("$n cries out and drops $p!", FALSE, ch, obj, NULL, TO_ROOM);
    if (GET_HIT(ch) > 50)
      GET_HIT(ch) = 1;
    obj_to_room(unequip_char(ch, slot), IN_ROOM(ch));
    return TRUE;
  }
  if (rand_number(0, 30) != 0)
    return FALSE;

  act("Your $p glows brightly as a huge swarm of insects joins the attack!", FALSE, ch, obj, victim,
      TO_CHAR);
  rol_weapon_cast(ch, obj, victim, SPELL_CREEPING_DOOM, 51);
  return TRUE;
}

static int rol_weapon_flamberge(struct char_data *ch, struct obj_data *obj,
                                struct char_data *victim)
{
  int amount;

  if (rand_number(0, 21) != 0)
    return FALSE;

  amount = dice(35, 10);
  if (GET_RACE(victim) == RACE_FIRE_ELEMENTAL || GET_RACE(victim) == RACE_EFREETI)
  {
    act("Your flaming $p sends soothing fire into $N's wounds.", FALSE, ch, obj, victim, TO_CHAR);
    GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + amount);
  }
  else
  {
    act("Your $p erupts and hurls a gigantic fireball into $N!", FALSE, ch, obj, victim, TO_CHAR);
    (void)rol_weapon_damage(ch, victim, amount, DAM_FIRE);
  }
  return TRUE;
}

static int rol_weapon_orb(struct spec_event_context *context, struct char_data *ch,
                          struct obj_data *obj, struct char_data *victim)
{
  bool arcane = rol_weapon_arcane_caster(ch) && !IS_BARD(ch);
  int roll_sides = arcane ? 14 : (IS_BARD(ch) ? 20 : 25);
  int triggered = FALSE;

  if (context->critical && arcane && !affected_by_spell(ch, SPELL_COLD_SHIELD) &&
      rand_number(0, 2) == 0)
  {
    act("Your $p erupts in dark light and raises a chilly aura around you.", FALSE, ch, obj, victim,
        TO_CHAR);
    rol_weapon_cast(ch, obj, ch, SPELL_COLD_SHIELD, 51);
    triggered = TRUE;
  }
  if (rand_number(0, roll_sides) != 0)
    return triggered;

  act("Your $p radiates a black beam that chills $N's soul!", FALSE, ch, obj, victim, TO_CHAR);
  (void)rol_weapon_damage(ch, victim, dice(20, 10), DAM_COLD);
  return TRUE;
}

static int rol_weapon_tahlshara(struct char_data *ch, struct obj_data *obj,
                                struct char_data *victim)
{
  int triggered = FALSE;

  if (rand_number(0, 10) == 0)
  {
    act("You dance with elven grace, bladesinging to the rhythm of battle.", FALSE, ch, obj, victim,
        TO_CHAR);
    triggered = TRUE;
  }
  if (GET_POS(ch) < POS_STANDING)
  {
    act("You spin from the ground and flow back into a fighting stance!", FALSE, ch, obj, victim,
        TO_CHAR);
    change_position(ch, POS_STANDING);
    triggered = TRUE;
  }
  if (rand_number(0, 25) != 0)
    return triggered;

  act("Your bladesong becomes a frenzy that topples $N and restores your vitality!", FALSE, ch, obj,
      victim, TO_CHAR);
  if (can_stun(victim))
  {
    if (!char_has_mud_event(victim, eSTUNNED))
      attach_mud_event(new_mud_event(eSTUNNED, victim, NULL), PULSE_VIOLENCE);
    change_position(victim, POS_SITTING);
  }
  GET_HIT(ch) = MIN(GET_MAX_HIT(ch), GET_HIT(ch) + 250);
  return TRUE;
}

static int rol_weapon_rockcrusher(struct char_data *ch, struct obj_data *obj,
                                  struct char_data *victim)
{
  if (rand_number(0, 28) != 0 || !rol_weapon_grounded_target(victim))
    return FALSE;

  act("You crash your $p into the ground, and a localized earthquake topples $N!", FALSE, ch, obj,
      victim, TO_CHAR);
  if (can_stun(victim))
  {
    if (!char_has_mud_event(victim, eSTUNNED))
      attach_mud_event(new_mud_event(eSTUNNED, victim, NULL), PULSE_VIOLENCE * rand_number(1, 3));
    change_position(victim, POS_SITTING);
  }
  return TRUE;
}

static int rol_weapon_entangling_root(struct char_data *ch, struct obj_data *obj,
                                      struct char_data *victim)
{
  struct affected_type affect;

  if (rand_number(0, 20) != 0 || IS_INCORPOREAL(victim) ||
      affected_by_spell(victim, SPELL_ENTANGLE) ||
      char_has_mud_event(victim, eROL_YGGDRASIL_RELEASE) != NULL)
    return FALSE;

  act("Roots lash from your $p and coil around $N's feet!", FALSE, ch, obj, victim, TO_CHAR);
  if (savingthrow(ch, victim, SAVING_REFL, GET_STR_BONUS(victim), CAST_WEAPON_SPELL,
                  MIN(30, GET_LEVEL(ch)), TRANSMUTATION))
  {
    act("$N tears free of the grasping roots!", FALSE, ch, obj, victim, TO_CHAR);
    return TRUE;
  }

  new_affect(&affect);
  affect.spell = SPELL_ENTANGLE;
  affect.duration = -1;
  affect.location = APPLY_DEX;
  affect.modifier = -2;
  SET_BIT_AR(affect.bitvector, AFF_ENTANGLED);
  affect_to_char(victim, &affect);
  NEW_EVENT(eROL_YGGDRASIL_RELEASE, victim, NULL, PULSE_VIOLENCE * 8);
  return TRUE;
}

static int rol_weapon_halruaa_enchanter(struct char_data *ch, struct obj_data *obj,
                                        struct char_data *victim)
{
  int choice;

  if (rand_number(0, 26) != 0)
    return FALSE;
  choice = rand_number(0, 3);
  act("Your $p releases a brilliant aquamarine beam at $N!", FALSE, ch, obj, victim, TO_CHAR);
  if (choice == 0)
    rol_weapon_cast(ch, obj, victim, SPELL_RAY_OF_ENFEEBLEMENT, GET_LEVEL(ch));
  else
    rol_weapon_cast(ch, obj, victim, SPELL_SLOW, GET_LEVEL(ch));
  return TRUE;
}

static int rol_weapon_halruaa_illusion(struct char_data *ch, struct obj_data *obj,
                                       struct char_data *victim)
{
  int choice;

  if (rand_number(0, 26) != 0)
    return FALSE;
  choice = rand_number(0, 3);
  act("Your $p writhes with living shadow and releases a spectral beam at $N!", FALSE, ch, obj,
      victim, TO_CHAR);
  if (choice == 0)
    rol_weapon_cast(ch, obj, victim, SPELL_RAINBOW_PATTERN, GET_LEVEL(ch));
  else if (choice == 1)
    rol_weapon_cast(ch, obj, victim, SPELL_SLOW, GET_LEVEL(ch));
  else
    rol_weapon_cast(ch, obj, victim, SPELL_FAERIE_FIRE, GET_LEVEL(ch));
  return TRUE;
}

static int rol_weapon_magebane(struct char_data *ch, struct obj_data *obj, struct char_data *victim)
{
  struct spec_damage_result result;

  if (rand_number(0, 21) != 0 || !IS_NPC(victim) || !rol_weapon_arcane_caster(victim))
    return FALSE;

  act("Your $p surrounds $N in a red aura of disruptive energy!", FALSE, ch, obj, victim, TO_CHAR);
  result = rol_weapon_damage(ch, victim, dice(8, 10), DAM_FORCE);
  if (result.status != SPEC_DAMAGE_TARGET_INVALIDATED && IS_CASTING(victim) &&
      rand_number(0, 2) == 0)
  {
    act("The pain shatters $N's concentration!", FALSE, ch, obj, victim, TO_CHAR);
    resetCastingData(victim);
  }
  return TRUE;
}

static int rol_weapon_darken_aura(struct char_data *ch, struct obj_data *obj,
                                  struct char_data *victim)
{
  if (!IS_EVIL(ch) || rand_number(0, 21) != 0)
    return FALSE;

  act("Your $p erupts in a suffocating burst of negative energy around $N!", FALSE, ch, obj, victim,
      TO_CHAR);
  if (!affected_by_spell(victim, SPELL_BLINDNESS))
  {
    rol_weapon_cast(ch, obj, victim, SPELL_BLINDNESS, 51);
    return TRUE;
  }
  if (rand_number(0, 3) == 0)
    rol_weapon_cast(ch, obj, victim, SPELL_RAY_OF_ENFEEBLEMENT, 51);
  (void)rol_weapon_damage(ch, victim, dice(15, 10), DAM_NEGATIVE);
  return TRUE;
}

static int rol_weapon_gleaming_burst(struct char_data *ch, struct obj_data *obj,
                                     struct char_data *victim)
{
  bool outlined;

  if (!IS_GOOD(ch) || rand_number(0, 22) != 0)
    return FALSE;

  act("A brilliant flash erupts from your $p and leaves a gleaming aura around $N!", FALSE, ch, obj,
      victim, TO_CHAR);
  outlined = affected_by_spell(victim, SPELL_FAERIE_FIRE);
  if (outlined && rand_number(0, 5) == 0 && !affected_by_spell(victim, SPELL_BLINDNESS))
    rol_weapon_cast(ch, obj, victim, SPELL_BLINDNESS, 51);
  if (!outlined)
    rol_weapon_cast(ch, obj, victim, SPELL_FAERIE_FIRE, 51);
  return TRUE;
}

static int rol_weapon_hit(struct spec_event_context *context,
                          const struct rol_weapon_profile *profile, struct char_data *ch,
                          struct obj_data *obj, struct char_data *victim, int slot)
{
  struct spec_damage_result result;
  int amount;

  if (rol_weapon_extra_attack_owner == obj)
    return FALSE;

  switch (profile->effect)
  {
  case ROL_WEAPON_HAMMER:
    return rol_weapon_hammer(ch, obj, victim);
  case ROL_WEAPON_ICY_DAGGER:
    if (!affected_by_spell(ch, SPELL_RESIST_ENERGY))
      rol_weapon_cast(ch, obj, ch, SPELL_RESIST_ENERGY, 30);
    if (!context->critical)
      return FALSE;
    if (IS_BERSERKER(ch) && rand_number(0, 9) == 0)
    {
      act("Your icy dagger calls down a violent storm of ice and hail!", FALSE, ch, obj, victim,
          TO_CHAR);
      rol_weapon_cast(ch, obj, victim, SPELL_ICE_STORM, 30);
      return TRUE;
    }
    return rol_weapon_cold_burst(ch, obj, victim, 10, IS_BERSERKER(ch) ? 20 : 10,
                                 "Icy particles from your $p form a spear that impales $N!");
  case ROL_WEAPON_GLIMMERING_BURST:
    if (rand_number(0, 27) != 0)
      return FALSE;
    act("Dazzling patterns from your $p tear at $N's mind!", FALSE, ch, obj, victim, TO_CHAR);
    result = rol_weapon_damage(ch, victim, dice(8, 10), DAM_MENTAL);
    if (result.status != SPEC_DAMAGE_TARGET_INVALIDATED && rand_number(0, 5) == 0 &&
        can_stun(victim) && !char_has_mud_event(victim, eSTUNNED))
      attach_mud_event(new_mud_event(eSTUNNED, victim, NULL), PULSE_VIOLENCE);
    return TRUE;
  case ROL_WEAPON_GITHYANKI_TWO_HANDED:
    return rol_weapon_githyanki_two_handed(ch, obj, victim);
  case ROL_WEAPON_GITHYANKI_CHARGED:
    return rol_weapon_githyanki_charged(context, ch, obj, victim);
  case ROL_WEAPON_VALHALLA_SCEPTER:
    return rol_weapon_valhalla(ch, obj, victim, slot);
  case ROL_WEAPON_SLENDER_ELVEN:
    if (!context->critical)
      return FALSE;
    if (IS_INCORPOREAL(victim))
    {
      act("Your $p passes harmlessly through $N's immaterial form.", FALSE, ch, obj, victim,
          TO_CHAR);
      return TRUE;
    }
    amount = rand_number(50, 150);
    if (savingthrow(ch, victim, SAVING_REFL, 0, CAST_WEAPON_SPELL, MIN(30, GET_LEVEL(ch)),
                    EVOCATION))
      amount /= 2;
    act("Your slender elven blade plunges deeply into $N!", FALSE, ch, obj, victim, TO_CHAR);
    (void)rol_weapon_damage(ch, victim, amount, DAM_SLASHING);
    return TRUE;
  case ROL_WEAPON_NIGHTBRINGER:
    if (rand_number(0, 25) != 0)
      return FALSE;
    act("Your $p glows as it strikes $N, leaving $M woozy.", FALSE, ch, obj, victim, TO_CHAR);
    rol_weapon_cast(ch, obj, victim, SPELL_SLEEP, 30);
    return TRUE;
  case ROL_WEAPON_KIRIN_HORN:
    if (rand_number(0, 25) != 0)
      return FALSE;
    act("Your $p flares white and unleashes a bolt of pure energy!", FALSE, ch, obj, victim,
        TO_CHAR);
    rol_weapon_cast(ch, obj, victim, SPELL_LIGHTNING_BOLT, 51);
    return TRUE;
  case ROL_WEAPON_WINDSONG:
    return rol_weapon_windsong(ch, obj, victim, slot);
  case ROL_WEAPON_SHADOW_DAGGER:
    return rol_weapon_shadow_dagger(context, ch, obj, victim);
  case ROL_WEAPON_FIRE_GIANT_SWORD:
    if (rand_number(0, 35) != 0)
      return FALSE;
    amount = MIN(100, MAX(0, GET_LEVEL(ch) * 2));
    act("Your great red-steel longsword blazes and severely burns $N!", FALSE, ch, obj, victim,
        TO_CHAR);
    (void)rol_weapon_damage(ch, victim, amount, DAM_FIRE);
    return TRUE;
  case ROL_WEAPON_ACID_LONGSWORD:
    if (!context->critical)
      return FALSE;
    act("Acid erupts along your $p and sprays toward $N!", FALSE, ch, obj, victim, TO_CHAR);
    rol_weapon_cast(ch, obj, victim, SPELL_ACID_ARROW, 30);
    return TRUE;
  case ROL_WEAPON_BARBED_SWORD:
    if (rand_number(0, 21) != 0)
      return FALSE;
    return rol_weapon_cold_burst(ch, obj, victim, 35, 10,
                                 "Green freezing waves from your $p engulf $N!");
  case ROL_WEAPON_RIPPLING_FLAMES:
    if (!context->critical)
      return FALSE;
    amount = rand_number(100, 200);
    if (GET_RACE(victim) == RACE_FIRE_ELEMENTAL || GET_RACE(victim) == RACE_EFREETI)
    {
      act("Your $p flares white hot, and $N smiles as the flames restore $S vitality.", FALSE, ch,
          obj, victim, TO_CHAR);
      GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + amount);
      return TRUE;
    }
    act("Your $p flares white hot and scars $N!", FALSE, ch, obj, victim, TO_CHAR);
    (void)rol_weapon_damage(ch, victim, amount, DAM_FIRE);
    return TRUE;
  case ROL_WEAPON_JEWELED_FANG:
    if (!context->critical)
      return FALSE;
    act("Your jeweled fang plunges into $N's chest with a sickening thud!", FALSE, ch, obj, victim,
        TO_CHAR);
    (void)rol_weapon_damage(ch, victim, rand_number(75, 100), DAM_PUNCTURE);
    return TRUE;
  case ROL_WEAPON_BLACK_FLAMES:
    if (rand_number(0, 25) != 0)
      return FALSE;
    return rol_weapon_cold_burst(ch, obj, victim, 37, 9,
                                 "Black flames from your $p engulf $N and drain away heat!");
  case ROL_WEAPON_MOONBLADE_STARSONG:
    if (!((time_info.hours < 6 || time_info.hours > 18) && OUTSIDE(ch)) || rand_number(0, 30) != 0)
      return FALSE;
    act("The stars brighten as your moonblade envelops $N in faerie magic!", FALSE, ch, obj, victim,
        TO_CHAR);
    rol_weapon_cast(ch, obj, victim, SPELL_FAERIE_FIRE, 30);
    return TRUE;
  case ROL_WEAPON_CRIMSON_DAGGER:
    if (rand_number(0, 10) != 0)
      return FALSE;
    if (context->critical && rol_weapon_primary_slot(slot))
    {
      act("Your crimson dagger flares and burrows into $N's forehead!", FALSE, ch, obj, victim,
          TO_CHAR);
      rol_weapon_cast(ch, obj, victim, SPELL_BLINDNESS, 20);
      (void)rol_weapon_damage(ch, victim, dice(50, 4), DAM_PUNCTURE);
      return TRUE;
    }
    if (!context->critical && rand_number(0, 2) == 0)
      return rol_weapon_crimson_drain(ch, obj, victim);
    return FALSE;
  case ROL_WEAPON_MIELIKKI_SCIMITAR:
    return rol_weapon_mielikki(ch, obj, victim, slot);
  case ROL_WEAPON_FLAMBERGE:
    return rol_weapon_flamberge(ch, obj, victim);
  case ROL_WEAPON_ORB:
    return rol_weapon_orb(context, ch, obj, victim);
  case ROL_WEAPON_DOOMBRINGER:
    if (rand_number(0, 25) != 0)
      return FALSE;
    act("Your $p hums with power and strikes $N over and over!", FALSE, ch, obj, victim, TO_CHAR);
    rol_weapon_extra_attacks(
        ch, obj, victim, 5, slot == WEAR_WIELD_OFFHAND ? ATTACK_TYPE_OFFHAND : ATTACK_TYPE_PRIMARY);
    return TRUE;
  case ROL_WEAPON_TAHLSHARA:
    return rol_weapon_tahlshara(ch, obj, victim);
  case ROL_WEAPON_ROCKCRUSHER:
    return rol_weapon_rockcrusher(ch, obj, victim);
  case ROL_WEAPON_CYMRIC_HUGH:
    if (rand_number(0, 30) != 0)
      return FALSE;
    act("Your $p explodes with color and sends a green beam toward $N!", FALSE, ch, obj, victim,
        TO_CHAR);
    rol_weapon_cast(ch, obj, victim, SPELL_HARM, 50);
    return TRUE;
  case ROL_WEAPON_TORMENT:
    if (IS_DRAGON(victim) || rand_number(0, 25) != 0)
      return FALSE;
    act("Your dark $p bites into $N's neck and releases a tormenting aura!", FALSE, ch, obj, victim,
        TO_CHAR);
    if (rol_weapon_cast(ch, obj, victim, SPELL_POISON, 51) >= 0)
      rol_weapon_cast(ch, obj, victim, SPELL_BLINDNESS, 51);
    return TRUE;
  case ROL_WEAPON_PAHLURUK_ROOT:
    return rol_weapon_entangling_root(ch, obj, victim);
  case ROL_WEAPON_REVERSE_DIRK:
    if (rand_number(0, 25) != 0)
      return FALSE;
    act("Your $p reverses its swing and strikes $N again!", FALSE, ch, obj, victim, TO_CHAR);
    rol_weapon_extra_attacks(
        ch, obj, victim, 1, slot == WEAR_WIELD_OFFHAND ? ATTACK_TYPE_OFFHAND : ATTACK_TYPE_PRIMARY);
    return TRUE;
  case ROL_WEAPON_FRULGHIEM:
    if (rand_number(0, 30) != 0)
      return FALSE;
    act("A huge clenched fist surges from your glowing $p and crashes into $N!", FALSE, ch, obj,
        victim, TO_CHAR);
    rol_weapon_cast(ch, obj, victim, SPELL_CLENCHED_FIST, 60);
    return TRUE;
  case ROL_WEAPON_SPHERE_LIGHTNING:
    if (rand_number(0, 25) != 0)
      return FALSE;
    act("A huge sphere of lightning blasts from your $p and strikes $N!", FALSE, ch, obj, victim,
        TO_CHAR);
    if (rol_weapon_cast(ch, obj, victim, SPELL_LIGHTNING_BOLT, 60) >= 0)
      rol_weapon_cast(ch, obj, victim, SPELL_LIGHTNING_BOLT, 60);
    return TRUE;
  case ROL_WEAPON_HALRUAA_ENCHANTER:
    return rol_weapon_halruaa_enchanter(ch, obj, victim);
  case ROL_WEAPON_HALRUAA_ILLUSION:
    return rol_weapon_halruaa_illusion(ch, obj, victim);
  case ROL_WEAPON_HALRUAA_INVOKER:
    if (rand_number(0, 25) != 0)
      return FALSE;
    act("Your $p releases a searing burst of flameheart energy at $N!", FALSE, ch, obj, victim,
        TO_CHAR);
    (void)rol_weapon_damage(ch, victim, dice((GET_LEVEL(ch) / 5) + 1, 12), DAM_FIRE);
    return TRUE;
  case ROL_WEAPON_HALRUAA_MAGEBANE:
    return rol_weapon_magebane(ch, obj, victim);
  case ROL_WEAPON_HALRUAA_DWARVEN_HAMMER:
    if (rand_number(0, 27) != 0)
      return FALSE;
    return rol_weapon_cold_burst(ch, obj, victim, 8, 10,
                                 "Runes flare along your $p and freezing cold engulfs $N!");
  case ROL_WEAPON_MYTH_DARKEN_AURA:
    return rol_weapon_darken_aura(ch, obj, victim);
  case ROL_WEAPON_MYTH_GLEAMING_BURST:
    return rol_weapon_gleaming_burst(ch, obj, victim);
  default:
    return FALSE;
  }
}

int rol_weapon_proc(struct char_data *ch, void *me, int cmd, const char *argument)
{
  UNUSED(ch);
  UNUSED(me);
  UNUSED(cmd);
  UNUSED(argument);

  /* Typed dispatch supplies the hit target, damage, attack kind, and invalidation contract. */
  return FALSE;
}

int rol_weapon_proc_typed(struct spec_event_context *context)
{
  const struct rol_weapon_profile *profile;
  struct char_data *ch;
  struct char_data *victim;
  struct obj_data *obj;
  int slot;

  if (context == NULL || context->owner_type != SPEC_OWNER_OBJECT || context->owner == NULL)
    return FALSE;
  obj = context->owner;
  ch = context->actor;
  profile = rol_weapon_profile_for(GET_OBJ_VNUM(obj));
  if (profile == NULL || ch == NULL)
    return FALSE;

  if (context->event == SPEC_EVENT_ITEM_IDENTIFY)
  {
    send_to_char(ch, "Special Effects: %s\r\n", profile->description);
    return TRUE;
  }
  if (context->event == SPEC_EVENT_COMMAND)
  {
    if (profile->effect != ROL_WEAPON_MOONBLADE_STARSONG)
      return FALSE;
    return rol_weapon_moonblade_command(ch, obj, context->command, context->argument);
  }
  if (context->event != SPEC_EVENT_WEAPON_HIT ||
      spec_context_validate_worn_object(ch, obj) != SPEC_CONTEXT_VALID ||
      spec_context_validate_combat_target(ch, context->target, false) != SPEC_CONTEXT_VALID ||
      (slot = rol_weapon_slot(ch, obj)) < 0)
    return FALSE;

  victim = context->target;
  return rol_weapon_hit(context, profile, ch, obj, victim, slot);
}

bool rol_update_mobile_home_after_move(struct char_data *ch, int source_room, int destination_room)
{
  if (ch == NULL || !IS_NPC(ch) || !VALID_ROOM_RNUM(source_room) ||
      !VALID_ROOM_RNUM(destination_room) || !ROOM_FLAGGED(source_room, ROOM_ROL_HOME_RESET))
    return false;

  GET_MOB_LOADROOM(ch) = destination_room;
  return true;
}
