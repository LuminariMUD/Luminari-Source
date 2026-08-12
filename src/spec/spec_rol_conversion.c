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
#include "spec_rol_conversion.h"
#include "spec_rol_totem.h"

#include <limits.h>

#define ROL_GATE_MAX_SUMMONS 5
#define ROL_GUILD_CLASS(class_id) (1ULL << (class_id))
#define ROL_GUILD_RACE(race_id) (1ULL << (race_id))
#define ROL_MAJOR_BEHOLDER_EYES 10
#define ROL_MAJOR_BEHOLDER_COOLDOWN_BITS 2
#define ROL_MAJOR_BEHOLDER_COOLDOWN_MASK 3U
#define ROL_MAJOR_BEHOLDER_COOLDOWN_ROUNDS 3

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

struct rol_death_profile
{
  int mobile_vnum;
  const char *message;
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
};

static const struct rol_death_profile rol_death_profiles[] = {
    {2000202, "$n dissipates into a cloud of oily green smoke."},
    {2000902, "The treant crashes into the ground and melts into the earth."},
    {2000903, "A phantom steed fades into nothingness."},
    {2000905, "The dark shade melts back into the shadows."},
    {2000906, "A water mephit blinks out of existence."},
    {2000907, "A fire mephit blinks out of existence."},
    {2000908, "An earth mephit blinks out of existence."},
    {2000909, "An air mephit blinks out of existence."},
    {2001250, "With a loud puffing sound, the fire elemental dissipates into smoke."},
    {2001251, "With a loud crash, the elemental dives into the ground. Only small stones remain "
              "where it once stood."},
    {2001252, "With a gentle swooshing sound, the air elemental simply disappears."},
    {2001253, "With a splash, the water elemental crashes to the ground leaving only a puddle "
              "behind."},
    {2003050, "With a loud puffing sound, the fire elemental dissipates into smoke."},
    {2003051, "With a loud crash, the elemental dives into the ground. Only small stones remain "
              "where it once stood."},
    {2003052, "With a gentle swooshing sound, the air elemental simply disappears."},
    {2003053, "With a splash, the water elemental crashes to the ground leaving only a puddle "
              "behind."},
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
    {2004128, NORTH, 0, 0, false},
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

const char *rol_conversion_death_message(int vnum)
{
  size_t index;

  for (index = 0; index < sizeof(rol_death_profiles) / sizeof(rol_death_profiles[0]); index++)
    if (rol_death_profiles[index].mobile_vnum == vnum)
      return rol_death_profiles[index].message;

  return NULL;
}

bool rol_handle_conjured_death(struct char_data *ch)
{
  const char *message = NULL;

  if (ch == NULL || !IS_NPC(ch))
    return false;

  message = rol_conversion_death_message(GET_MOB_VNUM(ch));
  if (message != NULL)
  {
    act(message, FALSE, ch, NULL, NULL, TO_ROOM);
    return true;
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
  int current_room_vnum;
  int direction;

  UNUSED(argument);

  if (guard == NULL || !IS_NPC(guard) || !VALID_ROOM_RNUM(IN_ROOM(guard)) ||
      GET_MOB_LOADROOM(guard) != IN_ROOM(guard))
    return FALSE;

  current_room_vnum = GET_ROOM_VNUM(IN_ROOM(guard));
  if (cmd == 0)
  {
    if (rol_guild_guard_protects(current_room_vnum) && FIGHTING(guard) != NULL)
      return rol_guild_guard_protection(guard, FIGHTING(guard));
    return FALSE;
  }

  if (ch == NULL || complete_cmd_info == NULL || !IS_MOVE(cmd))
    return FALSE;
  if (!IS_NPC(ch) && GET_LEVEL(ch) >= LVL_IMMORT)
    return FALSE;
  if (IS_NPC(ch) && MOB_FLAGGED(ch, MOB_GUARD))
    return FALSE;

  direction = complete_cmd_info[cmd].subcmd;
  if (rol_guild_guard_allows(current_room_vnum, direction, ch))
    return FALSE;

  act("$n humiliates you, and blocks your way.", FALSE, guard, NULL, ch, TO_VICT);
  act("$n humiliates $N, and blocks $S way.", FALSE, guard, NULL, ch, TO_NOTVICT);
  return TRUE;
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
                                        bool *suppresses_fighting)
{
  const struct rol_source_periodic_profile *profile = rol_source_periodic_profile_for(mobile_vnum);

  if (profile == NULL)
    return false;
  if (roll_min != NULL)
    *roll_min = profile->roll_min;
  if (roll_max != NULL)
    *roll_max = profile->roll_max;
  if (suppresses_fighting != NULL)
    *suppresses_fighting = profile->suppress_fighting;
  return true;
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
  if (speaker == NULL || cmd != 0 || !IS_NPC(speaker) || !AWAKE(speaker) ||
      IN_ROOM(speaker) == NOWHERE)
    return FALSE;

  profile = rol_source_periodic_profile_for(GET_MOB_VNUM(speaker));
  if (profile == NULL || (profile->suppress_fighting && FIGHTING(speaker) != NULL))
    return FALSE;

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

int rol_state_periodic(struct char_data *ch, void *me, int cmd, const char *argument)
{
  struct char_data *speaker = me;
  const struct rol_state_periodic_profile *profile;
  const struct rol_state_periodic_outcome *outcome;
  const struct rol_source_periodic_action *action;
  enum rol_state_periodic_state state;
  bool fighting;
  int dice_count;
  int dice_sides;
  int roll;
  size_t index;

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

  dice_count = fighting ? profile->fighting_dice_count : profile->idle_dice_count;
  dice_sides = fighting ? profile->fighting_dice_sides : profile->idle_dice_sides;
  if (dice_count == 0 || dice_sides == 0)
    return FALSE;

  state = fighting ? ROL_STATE_PERIODIC_FIGHTING : ROL_STATE_PERIODIC_IDLE;
  roll = dice(dice_count, dice_sides);
  outcome = rol_state_periodic_outcome_for(profile->profile_id, state, roll);
  if (outcome == NULL)
    return FALSE;

  for (index = 0; index < outcome->action_count; index++)
  {
    action = &rol_state_periodic_actions[outcome->first_action + index];
    if (action->kind == ROL_SOURCE_PERIODIC_SPEECH)
      do_say(speaker, action->message, 0, 0);
    else
      act(action->message, action->hide, speaker, NULL, NULL, TO_ROOM);
  }
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

bool rol_update_mobile_home_after_move(struct char_data *ch, int source_room, int destination_room)
{
  if (ch == NULL || !IS_NPC(ch) || !VALID_ROOM_RNUM(source_room) ||
      !VALID_ROOM_RNUM(destination_room) || !ROOM_FLAGGED(source_room, ROOM_ROL_HOME_RESET))
    return false;

  GET_MOB_LOADROOM(ch) = destination_room;
  return true;
}
