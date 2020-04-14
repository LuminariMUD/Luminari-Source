/* Alchemy subsystems for the alchemist class */
/* Gicker aka Stephen Squires, July 2019 */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "modify.h"
#include "feats.h"
#include "class.h"
#include "mud_event.h"
#include "assign_wpn_armor.h"
#include "domains_schools.h"
#include "spell_prep.h"
#include "alchemy.h"
#include "actions.h"
#include "act.h"
#include "fight.h"

// external functions
int attack_roll(struct char_data *ch, struct char_data *victim, int attack_type, int is_touch, int attack_number);
int damage(struct char_data *ch, struct char_data *victim, int dam, int w_type, int dam_type, int offhand);
int is_player_grouped(struct char_data *target, struct char_data *group);

const char *alchemical_discovery_names[NUM_ALC_DISCOVERIES] = {
    "normal bomb",
    "acid bomb",
    "blinding bomb",
    "boneshard bomb",
    "celestial poisons",
    "chameleon",
    "cognatogen",
    "concussive bomb",
    "confusion bomb",
    "dispelling bomb",
    "elemental mutagen",
    "enhance potion",
    "extend potion",
    "fast bombs",
    "fire brand",
    "force bomb",
    "frost bomb",
    "grand cognatogen",
    "grand inspiring cognatogen",
    "grand mutagen",
    "greater cognatogen",
    "greater inspiring cognatogen",
    "greater mutagen",
    "healing bomb",
    "curing touch",
    "holy bomb",
    "immolation bomb",
    "infuse mutagen",
    "infusion",
    "inspiring cognatogen",
    "malignant poison",
    "poison bomb",
    "precise bomb",
    "preserve organs",
    "profane bomb",
    "psychokinetic tincture",
    "shock bomb",
    "spontaneous healing",
    "sticky bombs",
    "stink bomb",
    "sunlight bomb",
    "tanglefoot bomb",
    "vestigial arm",
    "wings"};

const char *alchemical_discovery_descriptions[NUM_ALC_DISCOVERIES] = {
    "Deals 1d6/rank fire damage.",
    "Deals 1d6/rank acid damage with additional 1d6 damage 1 round later.",
    "Chance to blind direct target, and dazzle splash targets.",
    "Deals 1d6/rank piercing damage, and direct targets can take 1d4 bleed damage.",
    "Your poisons affect Undead and Evil Outsiders",
    "Bonus to stealth checks, +4 at first then +8 at alchemist level 10.",
    "Can use a mutagen to improve mental abilities instead of physical ones. +4 to selected mental ability, and -2 to associated physical ability, plus +2 natural armor bonus.",
    "Deals 1d4/rank sonic damage with chance to deafen.",
    "Chance to make direct targets confused, damage is reduced by 2d6, minimum of 1d6.",
    "Chance to dispel magic on direct target.  Does no damage.",
    "Can apply mutagen benefits to elemental resitance and an associated skill check bonus.",
    "increases the caster level on any quaffed potion to the alchemist's class level.",
    "Any potions quaffed last twice as long.",
    "Can make and throw bombs with a move action instead of a standard action.",
    "Allows use of a bomb that makes your wielded weapons flaming, and and alchemist level 10, also flaming burst.",
    "Deals 1d4/rank force damage with direct targets possibly being knocked prone.",
    "Deals 1d6/rank frost damage with direct targets having a chance to be staggered.",
    "As cognatogen but ability bonus is +8 for one ability, +6 for another and +4 for the third, plus natural ac is +4",
    "As inspiring cognatogen but +4 dodge AC, +4 reflex saves, but with -6 to strength and consistution.  Also includes some skill bonuses.",
    "As mutagen, but +6 natural armor, +8 to one physical ability, +6 to a second and +4 to the third.",
    "As cognatogen but +4 natural AC and +6 to one mental ability, and +4 to a second.",
    "As inspiring cognatogen, but +2 dodge ac, +2 reflex saves and -4 to strength and constitution, plus some bonuses to certain skills.",
    "As mutagen, but +6 to one ability and +4 to a second.  -2 to associated abilities.  +4 natural ac.",
    "Heals target 1d4/rank.",
    "As spontaneous healing discovery, but can heal the amount to another creature as a swift action.  Also increases maximum healing total to 5x Alchemist level.",
    "Deals 1d6/rank holy damage. Evil targets may be staggered. Neutral targets take 1/2 damage and good ones take none.",
    "Deals 1d6+int mod fire damage each round for # of rounds equal to # of bomb ranks.",
    "Reduces the penalties associated with mutagens and cognatogens by 1.",
    "Allows others to benefit from extracts",
    "+2 dodge ac, -2 strength and constitution, +1 to attack rolls and weapon damage.",
    "Increases save DCs of alchemist's poisons by +4, and increases duration by 50 percent.",
    "Kills weak creatures outright and deals 1d4 consitution damage continuously until suiccessful fortitude save.",
    "+2 to attack rolls with bombs.",
    "25 percent chance of nullifying any critical hit or sneak attack against the alchemist.",
    "Deals 1d6/rank unholy damage. Good targets may be staggered. Neutral targets take 1/2 damage and evil ones take none.",
    "For every 4 alchemist levels, this tincture bestows a single spirit that gives the alchemist a cumulative +1 bonus to deflection ac.  The alchemist can 'launch' one spirit per round which will make the target frightened if a mind-affecting fear save is failed.",
    "Deals 1d6/rank electricity damage with chance to dazzle direct targets for 1d4 rounds.",
    "Can heal self 5 hp once per round as a free action, for a total healing amount of alchemist level x2.",
    "Causes damage dealing bombs to deal an extra round of splash damage, and causes affect causing bombs to last an additional round.",
    "Makes all targets nauseated on failed save for 1 round / bomb rank",
    "Deals 1d6/rank radiant damage, chance to blind, Undead take +2 damage/bomb rank and are staggered on failed save.",
    "Chance to entangle those caught in effect.",
    "Grows a third arm from torso which acts as an additional hand for most purposes.",
    "Alchemist grows wings that will allow him to fly as per the fly spell."};

const char *grand_alchemical_discovery_names[NUM_GR_ALC_DISCOVERIES] = {
    "none",
    "awakened intellect",
    "fast healing",
    "poison touch",
    "true mutagen"};

const char *grand_alchemical_discovery_descriptions[NUM_GR_ALC_DISCOVERIES] = {
    "none",
    "Your base intelligence raises by 2 permanently",
    "Heal 5 hp per round permanently",
    "Is able to poison others with a touch (poisontouch command)",
    "Mutagens now bestow +8 to natural ac, str, dex and con, while -2 to int, wis and cha."};

const char *bomb_types[NUM_BOMB_TYPES] = {
    "none",
    "normal",
    "acid",
    "blinding",
    "boneshard",
    "concussive",
    "confusion",
    "dispelling",
    "fire-brand",
    "force",
    "frost",
    "healing",
    "holy",
    "immolation",
    "poison",
    "profane",
    "shock",
    "stink",
    "sunlight",
    "tanglefoot"};

const char *bomb_descriptions[NUM_BOMB_TYPES] = {
    "",
    "Deals 1d6/rank fire damage.",
    "Deals 1d6/rank acid damage with additional 1d6 damage 1 round later.",
    "Chance to blind direct target, and dazzle splash targets.",
    "Deals 1d6/rank piercing damage, and direct targets can take 1d4 bleed damage.",
    "Deals 1d4/rank sonic damage with chance to deafen.",
    "Chance to make direct targets confused, damage is reduced by 2d6, minimum of 1d6.",
    "Chance to dispel magic on direct target.  Does no damage.",
    "Makes wielded weapons flaming, and flaming burst at alc level 10.",
    "Deals 1d4/rank force damage with direct targets possibly being knocked prone.",
    "Deals 1d6/rank frost damage with direct targets having a chance to be staggered.",
    "Heals target 1d4/rank.",
    "Deals 1d6/rank holy damage. Evil targets may be staggered. Neutral targets take 1/2 damage and good ones take none.",
    "Deals 1d6+int mod fire damage each round for # of rounds equal to # of bomb ranks.",
    "Kills weak creatures outright and deals 1d4 consitution damage continuously until suiccessful fortitude save.",
    "Deals 1d6/rank unholy damage. Good targets may be staggered. Neutral targets take 1/2 damage and evil ones take none.",
    "Deals 1d6/rank electricity damage with chance to dazzle direct targets for 1d4 rounds.",
    "Makes all targets nauseated on failed save for 1 round / bomb rank",
    "Deals 1d6/rank radiant damage, chance to blind, Undead take +2 damage/bomb rank and are staggered on failed save.",
    "Chance to entangle those caught in effect."};

const char *discovery_requisites[NUM_ALC_DISCOVERIES] = {
    "none",                                             // none
    "none",                                             // acid
    "alchemist level 8",                                // blinding
    "none",                                             // boneshard
    "alchemist level 8",                                //celestial poisons
    "none",                                             // chameleon
    "none",                                             // cognatogen
    "alchemist level 6",                                // concussive bomb
    "alchemist level 8",                                // confusion bomb
    "alchemist level 6",                                // dispelling bomb
    "none",                                             // elemental mutagen
    "none",                                             // enhance potion
    "none",                                             // extend potion
    "alchemist level 8",                                // fast bombs
    "none",                                             //fire branks
    "alchemist level 8",                                // force bomb
    "none",                                             // frost bomb
    "alchemist level 16, greater cognatogen",           // grand cognatogen
    "alchemist level 16, greater inspiring cognatogen", // grand inspiring cognatogen
    "alchemist level 16, greater mutagen",              // grand mutagen
    "alchemist level 12, cognatogen",                   // greater cognatogen
    "alchemist level 12, inspiring cognatogen",         // greater inpisiring cognatogen
    "alchemist level 12",                               // greater mutagen
    "none",                                             // healing bomb
    "alchemist level 6, spontaneous healing",           // curing touch
    "alchemist level 8",                                // holy bomb
    "alchemist level 3",                                // immolation bomb
    "none",                                             // infuse mutagen
    "none",                                             // infusion
    "none",                                             // inspiring cognatogen
    "alchemist level 10",                               // malignant poison
    "alchemist level 12",                               // poison bomb
    "none",                                             //precise bomb
    "none",                                             // preserve organs
    "alchemist level 8",                                // profane bomb
    "alchemist level 4",                                // psychokinetic tincture
    "none",                                             // shock bomb
    "none",                                             // spontaneous healing
    "alchemist level 10",                               // sticky bomb
    "none",                                             // stink bomb
    "alchemist level 10, blinding bomb",                // sunlight bomb
    "none",                                             // tanglefoot bomb
    "none",                                             // vestigial arm
    "alchemist level 6"                                 // wings
};

const char *bomb_requisites[NUM_BOMB_TYPES] = {
    "none",                                                      // not a bomb
    "none",                                                      // normal bomb
    "none",                                                      // acid bomb
    "8 ranks in alchemist class",                                // blinding bomb
    "none",                                                      // boneshard bomb
    "6 ranks in alchemist class",                                // concussive bomb
    "8 ranks in alchemist class",                                // confusion bomb
    "6 ranks in alchemist class",                                // dispelling bomb
    "none",                                                      // fire brand
    "8 ranks in alchemist class",                                // force bomb
    "none",                                                      // frost bomb
    "6 ranks in alchemist class, spontaneous healing discovery", // healing bomb
    "8 ranks in alchemist class",                                // holy bomb
    "3 ranks in alchemist class",                                // immolation bomb
    "12 ranks in alchemist class",                               // poison bomb
    "8 ranks in alchemist class",                                // profane bomb
    "none",                                                      // shock bomb
    "none",                                                      // stink bomb
    "10 ranks in alchemist class, blinding bomb",                // sunlight bomb
    "none"                                                       // tanglefoot bomb
};

const char *bomb_damage_messages[NUM_BOMB_TYPES][3] = {
    {"", "", ""}, // none bomb
    {             // Normal
     "$n tosses a bomb that explodes in flames all around you.",
     "You toss a bomb that explodes in flames all around $N",
     "$n tosses a bomb that explodes in flames all around $N"},
    {// Acid
     "$n tosses a bomb that explodes, sprayingn acid all over you.",
     "You toss a bomb that explodes, spraying acid all over $N.",
     "$n tosses a bomb that explodes, spraying acid all over $N"},
    {"$n tosses a bomb that explodes in a bright flash of light in front of you!",
     "You toss a bomb that explodes in a bright flash of light in front of $N!",
     "$n tosses a bomb that explodes in a bright flash of light in front of $N!"},
    {// Boneshard
     "$n tosses a bomb at you that explodes in a spray of tiny bone shards!",
     "You toss a bomb at $N that explodes in a spray of tiny bone shards!",
     "$n tosses a bomb at $N that explodes in a spray of tiny bone shards!"},
    {// concussive
     "$n tosses a bomb at you that explodes with powerful shock waves.",
     "You toss a bomb at $N that explodes with powerful shock waves.",
     "$n tosses a bomb at $N that explodes with powerful shock waves."},
    {// confusion
     "$n tosses a bomb at you that explodes into searing and disorienting particles.",
     "You toss a bomb at $N that explodes into searing and disorienting particles.",
     "$n tosses a bomb at $N that explodes into searing and disorienting particles."},
    {// dispelling
     "$n tosses a bomb at you that explodes with waves of magic-nullifying energy.",
     "You toss a bomb at $N that explodes with waves of magic-nullifying energy.",
     "$n tosses a bomb at $N that explodes with waves of magic-nullifying energy."},
    {// fire brand
     "$n tosses a bomb at $s feet, lighting $s weapons aflame.",
     "You toss a bomb at your feet, lighting your weapons aflame.",
     "$n tosses a bomb at $s feet, lighting $s weapons aflame."},
    {// force
     "$n tosses a bomb at you that explodes with staggering force.",
     "You toss a bomb at $N that explodes with staggering force.",
     "$n tosses a bomb at $N that explodes with staggering force."},
    {// frost
     "$n tosses a bomb at you that explodes with paralyzing cold.",
     "You toss a bomb at $N that explodes with paralyzing cold.",
     "$n tosses a bomb at $N that explodes with paralyzing cold."},
    {// healing
     "$n tosses a bomb at you that fills you with healing warmth.",
     "You toss a bomb at $N that fills $M with healing warmth.",
     "$n tosses a bomb at $N that fills $M with healing warmth."},
    {// holy
     "$n tosses a bomb at you that explodes in radiant light.",
     "You toss a bomb at $N that explodes in radiant light.",
     "$n tosses a bomb at $N that explodes in radiant light."},
    {
        // immolation
        "$n tosses a bomb at you that explodes in a ball of bubbling magma.",
        "You toss a bomb at $N that explodes in a ball of bubbling magma.",
        "$n tosses a bomb at $N that explodes in a ball of bubbling magma.",
    },
    {// poison
     "$n tosses a bomb at you that explodes in a cloud of poisonous, killing gas.",
     "You toss a bomb at $N that explodes in a cloud of poisonous, killing gas.",
     "$n tosses a bomb at $N that explodes in a cloud of poisonous, killing gas."},
    {// profane
     "$n tosses a bomb at you that explodes in a wave of debilitaing unholy power.",
     "You toss a bomb at $N that explodes in a wave of debilitaing unholy power.",
     "$n tosses a bomb at $N that explodes in a wave of debilitaing unholy power."},
    {// shock
     "$n tosses a bomb at you that explodes in dozens of streaking lightning bolts.",
     "You toss a bomb at $N that explodes in dozens of streaking lightning bolts.",
     "$n tosses a bomb at $N that explodes in dozens of streaking lightning bolts."},
    {// stink
     "$n tosses a bomb at you that explodes in a cloud of nauseating gas.",
     "You toss a bomb at $N that explodes in a cloud of nauseating gas.",
     "$n tosses a bomb at $N that explodes in a cloud of nauseating gas."},
    {// sunlight
     "$n tosses a bomb at you that explodes in brilliant flash of radiant energy.",
     "You toss a bomb at $N that explodes in brilliant flash of radiant energy.",
     "$n tosses a bomb at $N that explodes in brilliant flash of radiant energy."},
    {// tanglefoot
     "$n tosses a bomb at you that explodes in several globs of sticky goo.",
     "You toss a bomb at $N that explodes in several globs of sticky goo.",
     "$n tosses a bomb at $N that explodes in several globs of sticky goo."}};

int num_of_bombs_preparable(struct char_data *ch)
{

  int num = 0;

  num += CLASS_LEVEL(ch, CLASS_ALCHEMIST);

  num += GET_REAL_INT_BONUS(ch);

  return num;
}

int num_of_bombs_prepared(struct char_data *ch)
{

  int i = 0,
      num_prepped = 0;

  for (i = 0; i < num_of_bombs_preparable(ch); i++)
  {
    if (GET_BOMB(ch, i) == BOMB_NONE)
      continue;
    num_prepped++;
  }
  return num_prepped;
}

int discovery_to_bomb_type(int discovery)
{

  switch (discovery)
  {
  case ALC_DISC_NONE:
    return BOMB_NORMAL;
  case ALC_DISC_ACID_BOMBS:
    return BOMB_ACID;
  case ALC_DISC_BLINDING_BOMBS:
    return BOMB_BLINDING;
  case ALC_DISC_BONESHARD_BOMBS:
    return BOMB_BONESHARD;
  case ALC_DISC_CONCUSSIVE_BOMBS:
    return BOMB_CONCUSSIVE;
  case ALC_DISC_CONFUSION_BOMBS:
    return BOMB_CONFUSION;
  case ALC_DISC_DISPELLING_BOMBS:
    return BOMB_DISPELLING;
  case ALC_DISC_FIRE_BRAND:
    return BOMB_FIRE_BRAND;
  case ALC_DISC_FORCE_BOMBS:
    return BOMB_FORCE;
  case ALC_DISC_FROST_BOMBS:
    return BOMB_FROST;
  case ALC_DISC_HEALING_BOMBS:
    return BOMB_HEALING;
  case ALC_DISC_HOLY_BOMBS:
    return BOMB_HOLY;
  case ALC_DISC_IMMOLATION_BOMBS:
    return BOMB_IMMOLATION;
  case ALC_DISC_POISON_BOMBS:
    return BOMB_POISON;
  case ALC_DISC_PROFANE_BOMBS:
    return BOMB_PROFANE;
  case ALC_DISC_SHOCK_BOMBS:
    return BOMB_SHOCK;
  case ALC_DISC_STINK_BOMBS:
    return BOMB_STINK;
  case ALC_DISC_SUNLIGHT_BOMBS:
    return BOMB_SUNLIGHT;
  case ALC_DISC_TANGLEFOOT_BOMBS:
    return BOMB_TANGLEFOOT;
  default:
    return -1;
  }

  return -1;
}

int bomb_type_to_discovery(int bomb)
{

  switch (bomb)
  {
  case BOMB_NORMAL:
    return ALC_DISC_NONE;
  case BOMB_ACID:
    return ALC_DISC_ACID_BOMBS;
  case BOMB_BLINDING:
    return ALC_DISC_BLINDING_BOMBS;
  case BOMB_BONESHARD:
    return ALC_DISC_BONESHARD_BOMBS;
  case BOMB_CONCUSSIVE:
    return ALC_DISC_CONCUSSIVE_BOMBS;
  case BOMB_CONFUSION:
    return ALC_DISC_CONFUSION_BOMBS;
  case BOMB_DISPELLING:
    return ALC_DISC_DISPELLING_BOMBS;
  case BOMB_FIRE_BRAND:
    return ALC_DISC_FIRE_BRAND;
  case BOMB_FORCE:
    return ALC_DISC_FORCE_BOMBS;
  case BOMB_FROST:
    return ALC_DISC_FROST_BOMBS;
  case BOMB_HEALING:
    return ALC_DISC_HEALING_BOMBS;
  case BOMB_HOLY:
    return ALC_DISC_HOLY_BOMBS;
  case BOMB_IMMOLATION:
    return ALC_DISC_IMMOLATION_BOMBS;
  case BOMB_POISON:
    return ALC_DISC_POISON_BOMBS;
  case BOMB_PROFANE:
    return ALC_DISC_PROFANE_BOMBS;
  case BOMB_SHOCK:
    return ALC_DISC_SHOCK_BOMBS;
  case BOMB_STINK:
    return ALC_DISC_STINK_BOMBS;
  case BOMB_SUNLIGHT:
    return ALC_DISC_SUNLIGHT_BOMBS;
  case BOMB_TANGLEFOOT:
    return ALC_DISC_TANGLEFOOT_BOMBS;
  default:
    return -1;
  }

  return -1;
}

int find_open_bomb_slot(struct char_data *ch)
{
  int i = 0;

  for (i = 0; i < num_of_bombs_preparable(ch); i++)
  {
    if (ch->player_specials->saved.bombs[i] != BOMB_NONE)
      continue;
    return i;
  }
  return -1;
}

void list_bomb_types_known(struct char_data *ch)
{
  int i = 0, j = 0;

  send_to_char(ch, "%-20s - %s\r\n", "normal", bomb_descriptions[0]);

  for (i = 0; i < NUM_ALC_DISCOVERIES; i++)
  {
    j = discovery_to_bomb_type(i);
    if (KNOWS_DISCOVERY(ch, i) && j != -1)
    {
      send_to_char(ch, "%-20s - %s\r\n", bomb_types[j], bomb_descriptions[j]);
    }
  }

  return;
}

ACMD(do_bombs)
{

  if (!HAS_FEAT(ch, FEAT_BOMBS))
  {
    send_to_char(ch, "You do not know anything about making to tossing bombs.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Would you like to 'toss' a bomb, 'discard' a bomb, 'list' your bombs or 'make' a bomb?\r\n");
    return;
  }

  char ccmd[100], // toss, list or make
      scmd[100],  // second half for next two
      slot[10],   // which bomb slot to toss or make
      spec[100];  // either target name for toss or discovery type for make

  int i = 0,     // used for various calculations/loops
      bSlot = 0, // what bomb slot is being referenced
      type = 0;  // the type of bomb to use

  struct char_data *target = NULL;

  half_chop(argument, ccmd, scmd);

  if (is_abbrev(ccmd, "toss"))
  {

    if (!*scmd)
    {
      send_to_char(ch, "You need to supply a bomb type and optionally a target as well.\r\n");
      return;
    }

    two_arguments(scmd, slot, spec);

    if (!*slot)
    {
      send_to_char(ch, "You need to specify a bomb type.  You must have prepared these already using the 'bomb make' command.\r\n");
      return;
    }

    for (i = 0; i < num_of_bombs_preparable(ch); i++)
    {
      if (is_abbrev(slot, bomb_types[GET_BOMB(ch, i)]))
      {
        type = GET_BOMB(ch, i);
        bSlot = i;
        break;
      }
    }
    if (i >= num_of_bombs_preparable(ch))
    {
      send_to_char(ch, "You don't have any %s bombs prepared.\r\n", slot);
      return;
    }

    if (*spec)
    {
      if (!(target = get_char_vis(ch, spec, NULL, FIND_CHAR_ROOM)))
      {
        send_to_char(ch, "%s", CONFIG_NOPERSON);
        return;
      }
    }

    if (!target && !FIGHTING(ch))
    {
      send_to_char(ch, "If you are not in combat, you need to specify a target.\r\n");
      return;
    }

    if (!target && FIGHTING(ch))
    {
      target = FIGHTING(ch);
    }

    if (target == ch)
    {
      switch (type)
      {
      case BOMB_HEALING:
      case BOMB_FIRE_BRAND:
        break; // these are ok. ^^^
      default: // these aren't >>>
        send_to_char(ch, "You cannot throw a harmful bomb at yourself.");
        return;
      }
    }

    if (!is_player_grouped(ch, target) && !IS_NPC(target) && (!PRF_FLAGGED(ch, PRF_PVP) || !PRF_FLAGGED(target, PRF_PVP)))
    {
      switch (type)
      {
      case BOMB_HEALING:
        break; // these are ok. ^^^
      default: // these aren't >>>
        send_to_char(ch, "You cannot throw a harmful bomb at a player if you or they are not flagged as pvp.");
        return;
      }
    }

    if (is_player_grouped(ch, target))
    {
      switch (type)
      {
      case BOMB_HEALING:
      case BOMB_FIRE_BRAND:
        break; // these are ok. ^^^
      default: // these aren't >>>
        send_to_char(ch, "You cannot use that type of bomb on party members.");
        return;
      }
    }

    if (!is_action_available(ch, KNOWS_DISCOVERY(ch, ALC_DISC_FAST_BOMBS) ? ACTION_MOVE : ACTION_STANDARD, TRUE))
      return;

    if (!target)
      target = FIGHTING(ch);

    if (bomb_is_friendly(type) || attack_roll(ch, target, ATTACK_TYPE_RANGED, TRUE, 1) >= 0)
    {
      // we hit!

      perform_bomb_effect(ch, target, type);
    }
    else
    {
      // we missed :(
      send_to_char(ch, "You throw a bomb, but it misses its intended target.\r\n");
      act("$n throws a bomb, but it misses its intended target and explodes a safe distance away.", TRUE, ch, 0, 0, TO_ROOM);
      // we want to start combat if not already in combat
      if (!FIGHTING(target))
      {
        hit(target, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);
      }
    }

    // let's remove the bomb they just tossed
    GET_BOMB(ch, bSlot) = BOMB_NONE;
    save_char(ch, 0);
    if (KNOWS_DISCOVERY(ch, ALC_DISC_FAST_BOMBS))
      USE_MOVE_ACTION(ch);
    else
      USE_STANDARD_ACTION(ch);
    return;
  }
  else if (is_abbrev(ccmd, "make"))
  {

    if (num_of_bombs_prepared(ch) >= num_of_bombs_preparable(ch))
    {
      send_to_char(ch, "You cannot create any more bombs.  Please use 'bombs discard'  to open up some slots, or use the bombs in battle using 'bombs toss'\r\n");
      return;
    }

    if ((bSlot = find_open_bomb_slot(ch)) == -1)
    {
      send_to_char(ch, "There seems to be an error.  Please inform a staff member with error code BOMB_ERR_1.\r\n");
      return;
    }

    if (!*scmd)
    {
      send_to_char(ch, "You need to supply the type of bomb you want to make.  You know the following:\r\n");
      list_bomb_types_known(ch);
      return;
    }

    for (i = 1; i < NUM_BOMB_TYPES; i++)
    {
      if (is_abbrev(scmd, bomb_types[i]))
      {
        if (i != BOMB_NORMAL)
          if (!KNOWS_DISCOVERY(ch, bomb_type_to_discovery(i)))
            continue;
        type = i;
        break;
      }
    }

    if (i >= NUM_BOMB_TYPES)
    {
      send_to_char(ch, "That is not a valid bomb type, or you don't know how to make that kind of bombs.  Here's a list of bomb types you know how to make:\r\n");
      list_bomb_types_known(ch);
      return;
    }

    if (!is_action_available(ch, KNOWS_DISCOVERY(ch, ALC_DISC_FAST_BOMBS) ? ACTION_MOVE : ACTION_STANDARD, TRUE))
      return;

    ch->player_specials->saved.bombs[bSlot] = type;

    send_to_char(ch, "You've successfully created a %s bomb.\r\n",
                 bomb_types[type]);

    save_char(ch, 0);

    if (KNOWS_DISCOVERY(ch, ALC_DISC_FAST_BOMBS))
      USE_MOVE_ACTION(ch);
    else
      USE_STANDARD_ACTION(ch);

    // we want to add a cooldown event here at some point

    return;
  }
  else if (is_abbrev(ccmd, "list"))
  {

    int num_bombs[NUM_BOMB_TYPES];
    int open_slots = 0;

    for (i = 0; i < NUM_BOMB_TYPES; i++)
      num_bombs[i] = BOMB_NONE;

    send_to_char(ch, "You have prepared the following bombs:\r\n");

    for (i = 0; i < num_of_bombs_preparable(ch); i++)
    {
      if (GET_BOMB(ch, i) != BOMB_NONE)
        num_bombs[GET_BOMB(ch, i)]++;
      else
        open_slots++;
    }

    for (i = 0; i < NUM_BOMB_TYPES; i++)
    {
      if (num_bombs[i] > 0)
      {
        send_to_char(ch, "x%2d: %-12s - %s\r\n", num_bombs[i], bomb_types[i], bomb_descriptions[i]);
      }
    }

    send_to_char(ch, "Open Bomb Slots: %d\r\n", open_slots);

    return;
  }
  else if (is_abbrev(ccmd, "discard"))
  {

    if (!*scmd)
    {
      send_to_char(ch, "You need to specify which bomb type you want to discard.\r\n"
                       "Type 'bombs list' too see your available bombs.\r\n");
      return;
    }

    for (i = 0; i < num_of_bombs_preparable(ch); i++)
    {
      if (is_abbrev(scmd, bomb_types[GET_BOMB(ch, i)]))
      {
        type = GET_BOMB(ch, i);
        bSlot = i;
        break;
      }
    }
    if (i >= num_of_bombs_preparable(ch))
    {
      send_to_char(ch, "You don't have any %s bombs prepared.\r\n", scmd);
      return;
    }

    if (GET_BOMB(ch, bSlot) == BOMB_NONE)
    {
      send_to_char(ch, "There is no bomb of that type to discard.\r\n"
                       "Type 'bombs list' too see your available slots.\r\n");
      return;
    }

    send_to_char(ch, "You have discarded a %s bomb.\r\n", bomb_types[GET_BOMB(ch, bSlot)]);
    GET_BOMB(ch, bSlot) = 0;
    save_char(ch, 0);
    return;
  }
  else
  { // no match on ccmd variable
    send_to_char(ch, "Would you like to 'toss' a bomb, 'discard' a bomb, 'list' your bombs or 'make' a bomb?\r\n");
  }
}

void perform_bomb_effect(struct char_data *ch, struct char_data *victim, int bomb_type)
{

  switch (bomb_type)
  {
  case BOMB_NORMAL:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_damage(ch, victim, bomb_type); // fire damage
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      perform_bomb_splash_damage(ch, victim, bomb_type); // fire damage
    break;
  case BOMB_ACID:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_damage(ch, victim, bomb_type); // acid damage
    perform_bomb_direct_effect(ch, victim, bomb_type); // DoT acid effect
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      perform_bomb_splash_damage(ch, victim, bomb_type); // acid damage
    break;
  case BOMB_BLINDING:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_effect(ch, victim, bomb_type); // blind
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      perform_bomb_splash_effect(ch, victim, bomb_type); // dazzle
    break;
  case BOMB_BONESHARD:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_damage(ch, victim, bomb_type); // piercing damage
    perform_bomb_direct_effect(ch, victim, bomb_type); // DoT bleed effect
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      perform_bomb_splash_damage(ch, victim, bomb_type); // piercing damage
    break;
  case BOMB_CONCUSSIVE:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_damage(ch, victim, bomb_type); // sonic damage
    perform_bomb_direct_effect(ch, victim, bomb_type); // deafen
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      perform_bomb_splash_damage(ch, victim, bomb_type); // sonic damage
    break;
  case BOMB_CONFUSION:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_damage(ch, victim, bomb_type); // reduced fire damage
    perform_bomb_direct_effect(ch, victim, bomb_type); // confusion
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      perform_bomb_splash_damage(ch, victim, bomb_type); // reduced fire damage
    break;
  case BOMB_DISPELLING:
    send_bomb_direct_message(ch, victim, bomb_type);
    perform_bomb_spell_effect(ch, victim, bomb_type); // dispel magic
    break;
  case BOMB_FIRE_BRAND:
    send_bomb_direct_message(ch, victim, bomb_type);
    perform_bomb_self_effect(ch, victim, bomb_type); // make weapons flaming and flaming burst (at lvl 10)
    break;
  case BOMB_FORCE:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_damage(ch, victim, bomb_type); // reduced force damage
    perform_bomb_direct_effect(ch, victim, bomb_type); // knock prone
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      perform_bomb_splash_damage(ch, victim, bomb_type); // reduced force damage
    break;
  case BOMB_FROST:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_damage(ch, victim, bomb_type); // cold damage
    perform_bomb_direct_effect(ch, victim, bomb_type); // staggered
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      perform_bomb_splash_damage(ch, victim, bomb_type); // cold damage
    break;
  case BOMB_HEALING:
    send_bomb_direct_message(ch, victim, bomb_type);
    perform_bomb_direct_healing(ch, victim, bomb_type); // healing
    break;
  case BOMB_HOLY:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_damage(ch, victim, bomb_type); // holy damage
    perform_bomb_direct_effect(ch, victim, bomb_type); // evil victs are staggered
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      perform_bomb_splash_damage(ch, victim, bomb_type); // holy damage
    break;
  case BOMB_IMMOLATION:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_damage(ch, victim, bomb_type); // reduced fire damage
    perform_bomb_direct_effect(ch, victim, bomb_type); // DoT effect
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      perform_bomb_splash_damage(ch, victim, bomb_type); // reduced fire damage
    break;
  case BOMB_POISON:
    send_bomb_direct_message(ch, victim, bomb_type);
    perform_bomb_spell_effect(ch, victim, bomb_type); // cloudkill
    break;
  case BOMB_PROFANE:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_damage(ch, victim, bomb_type); // unholy damage
    perform_bomb_direct_effect(ch, victim, bomb_type); // good victs are staggered
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      perform_bomb_splash_damage(ch, victim, bomb_type); // unholy damage
    break;
  case BOMB_SHOCK:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_damage(ch, victim, bomb_type); // electricity damage
    perform_bomb_direct_effect(ch, victim, bomb_type); // dazzled
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      perform_bomb_splash_damage(ch, victim, bomb_type); // electricity damage
    break;
  case BOMB_STINK:
    send_bomb_direct_message(ch, victim, bomb_type);
    perform_bomb_spell_effect(ch, victim, bomb_type); // stinking cloud
    break;
  case BOMB_SUNLIGHT:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_damage(ch, victim, bomb_type); // fire damage, +2 damage per die for undead and oozes
    perform_bomb_direct_effect(ch, victim, bomb_type); // blind, undead sensitive to sunlight staggered
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
    {
      perform_bomb_splash_damage(ch, victim, bomb_type); // fire damage, +2 damage per die for undead and oozes
      perform_bomb_splash_effect(ch, victim, bomb_type); // dazzle
    }
    break;
  case BOMB_TANGLEFOOT:
    send_bomb_direct_message(ch, victim, bomb_type);
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      send_bomb_splash_message(ch, victim, bomb_type);
    perform_bomb_direct_effect(ch, victim, bomb_type); // entangled
    if (PRF_FLAGGED(ch, PRF_AOE_BOMBS))
      perform_bomb_splash_effect(ch, victim, bomb_type); // entangled
    break;
  }
  if (KNOWS_DISCOVERY(ch, ALC_DISC_STICKY_BOMBS))
  {
    add_sticky_bomb_effect(ch, victim, bomb_type);
  }
}

void send_bomb_direct_message(struct char_data *ch, struct char_data *victim, int bomb_type)
{

  char buf[200];

  sprintf(buf, "%s", bomb_damage_messages[bomb_type][1]);
  act(buf, FALSE, ch, 0, victim, TO_CHAR);
  if (ch == victim)
  {
    sprintf(buf, "%s", bomb_damage_messages[bomb_type][2]);
    act(buf, FALSE, ch, 0, victim, TO_ROOM);
  }
  else
  {
    sprintf(buf, "%s", bomb_damage_messages[bomb_type][0]);
    act(buf, FALSE, ch, 0, victim, TO_VICT);
    sprintf(buf, "%s", bomb_damage_messages[bomb_type][2]);
    act(buf, FALSE, ch, 0, victim, TO_NOTVICT);
  }
}

void send_bomb_splash_message(struct char_data *ch, struct char_data *victim, int bomb_type)
{

  struct char_data *tch = NULL;
  char buf[200];

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (is_player_grouped(ch, tch))
      continue;
    if (IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM))
      continue;
    if (tch == ch || tch == victim)
      continue;
    if (!IS_NPC(tch) && !PRF_FLAGGED(ch, PRF_PVP))
      continue;

    sprintf(buf, "%s", bomb_damage_messages[bomb_type][0]);
    act(buf, FALSE, ch, 0, tch, TO_VICT);
    sprintf(buf, "%s", bomb_damage_messages[bomb_type][1]);
    act(buf, FALSE, ch, 0, tch, TO_CHAR);
    sprintf(buf, "%s", bomb_damage_messages[bomb_type][2]);
    act(buf, FALSE, ch, 0, tch, TO_NOTVICT);
  }
}

void perform_bomb_direct_damage(struct char_data *ch, struct char_data *victim, int bomb_type)
{

  int ndice = HAS_FEAT(ch, FEAT_BOMBS),
      sdice = 6,
      damMod = GET_INT_BONUS(ch),
      damType = DAM_FIRE,
      saveType = SAVING_REFL,
      dam = 0,
      active = FALSE;
  char buf[200];

  switch (bomb_type)
  {
  // if it's not listed here, either it doesn't deal damage to the direct target, or it uses
  // the default values above.
  case BOMB_NORMAL:
    damType = DAM_FIRE;
    active = TRUE;
    break;
  case BOMB_ACID:
    damType = DAM_ACID;
    active = TRUE;
    break;
  case BOMB_BONESHARD:
    damType = DAM_PUNCTURE;
    active = TRUE;
    break;
  case BOMB_CONCUSSIVE:
    sdice = 4;
    damType = DAM_SOUND;
    active = TRUE;
    break;
  case BOMB_CONFUSION:
    ndice = MAX(1, ndice - 2);
    active = TRUE;
    break;
  case BOMB_FORCE:
    sdice = 4;
    damType = DAM_FORCE;
    active = TRUE;
    break;
  case BOMB_FROST:
    damType = DAM_COLD;
    active = TRUE;
    break;
  case BOMB_HOLY:
    damType = DAM_HOLY;
    active = TRUE;
    break;
  case BOMB_IMMOLATION:
    ndice = 1;
    active = TRUE;
    break;
  case BOMB_PROFANE:
    damType = DAM_UNHOLY;
    active = TRUE;
    break;
  case BOMB_SHOCK:
    damType = DAM_ELECTRIC;
    active = TRUE;
    break;
  case BOMB_SUNLIGHT:
    damType = DAM_LIGHT;
    active = TRUE;
    if (IS_UNDEAD(victim) || IS_OOZE(victim))
      damMod += HAS_FEAT(ch, FEAT_BOMBS) * 2;
    break;
  }

  if (!active)
    return;

  dam = dice(ndice, sdice) + damMod;

  if (bomb_type == BOMB_HOLY)
  {
    if (IS_GOOD(victim))
      dam = 0;
    if (IS_NEUTRAL(victim))
      dam /= 2;
  }
  if (bomb_type == BOMB_PROFANE)
  {
    if (IS_EVIL(victim))
      dam = 0;
    if (IS_NEUTRAL(victim))
      dam /= 2;
  }

  if (mag_savingthrow(ch, victim, saveType, 0, CAST_BOMB, GET_LEVEL(ch), SCHOOL_NOSCHOOL))
  {
    if ((!IS_NPC(victim)) && saveType == SAVING_REFL && // evasion
        (HAS_FEAT(victim, FEAT_EVASION) ||
         (HAS_FEAT(victim, FEAT_IMPROVED_EVASION))))
      dam /= 2;
    dam /= 2;
  }
  else if ((!IS_NPC(victim)) && saveType == SAVING_REFL && // evasion
           (HAS_FEAT(victim, FEAT_IMPROVED_EVASION)))
  {
    dam /= 2;
  }

  dam = damage(ch, victim, dam, SKILL_BOMB_TOSS, damType, SKILL_BOMB_TOSS);

  if (dam > 0 && FALSE)
  {
    sprintf(buf, "Your bomb deals %d damage to $N", dam);
    act(buf, FALSE, ch, 0, victim, TO_CHAR);
    sprintf(buf, "$n's bomb deals %d damage to You", dam);
    act(buf, FALSE, ch, 0, victim, TO_VICT);
  }
}

void perform_bomb_splash_damage(struct char_data *ch, struct char_data *victim, int bomb_type)
{
  if (!PRF_FLAGGED(ch, PRF_AOE_BOMBS))
    return;

  int ndice = HAS_FEAT(ch, FEAT_BOMBS),
      damMod = GET_INT_BONUS(ch),
      damType = DAM_FIRE,
      saveType = SAVING_REFL,
      dam = 0,
      active = FALSE;
  struct char_data *tch = NULL;
  char buf[200];

  switch (bomb_type)
  {
  // if it's not listed here, either it doesn't deal damage to the direct target, or it uses
  // the default values above.
  case BOMB_NORMAL:
    damType = DAM_FIRE;
    active = TRUE;
    break;
  case BOMB_ACID:
    damType = DAM_ACID;
    active = TRUE;
    break;
  case BOMB_BONESHARD:
    damType = DAM_PUNCTURE;
    active = TRUE;
    break;
  case BOMB_CONCUSSIVE:
    damType = DAM_SOUND;
    active = TRUE;
    break;
  case BOMB_CONFUSION:
    ndice = MAX(1, ndice - 2);
    active = TRUE;
    break;
  case BOMB_FORCE:
    damType = DAM_FORCE;
    active = TRUE;
    break;
  case BOMB_FROST:
    damType = DAM_COLD;
    active = TRUE;
    break;
  case BOMB_HOLY:
    damType = DAM_HOLY;
    active = TRUE;
    break;
  case BOMB_PROFANE:
    damType = DAM_UNHOLY;
    active = TRUE;
    break;
  case BOMB_SHOCK:
    damType = DAM_ELECTRIC;
    active = TRUE;
    break;
  case BOMB_IMMOLATION:
    ndice = 1;
    active = TRUE;
    break;
  case BOMB_SUNLIGHT:
    damType = DAM_LIGHT;
    active = TRUE;
    if (IS_UNDEAD(victim) || IS_OOZE(victim))
      damMod += (HAS_FEAT(ch, FEAT_BOMBS) / 2) + 2;
    break;
  }

  if (!active)
    return;

  dam = ndice + damMod;

  if (bomb_type == BOMB_HOLY)
  {
    if (IS_GOOD(victim))
      dam = 0;
    if (IS_NEUTRAL(victim))
      dam /= 2;
  }
  if (bomb_type == BOMB_PROFANE)
  {
    if (IS_EVIL(victim))
      dam = 0;
    if (IS_NEUTRAL(victim))
      dam /= 2;
  }

  if (mag_savingthrow(ch, victim, saveType, 0, CAST_BOMB, GET_LEVEL(ch), SCHOOL_NOSCHOOL))
  {
    if ((!IS_NPC(victim)) && saveType == SAVING_REFL && // evasion
        (HAS_FEAT(victim, FEAT_EVASION) ||
         (HAS_FEAT(victim, FEAT_IMPROVED_EVASION))))
      dam /= 2;
    dam /= 2;
  }
  else if ((!IS_NPC(victim)) && saveType == SAVING_REFL && // evasion
           (HAS_FEAT(victim, FEAT_IMPROVED_EVASION)))
  {
    dam /= 2;
  }

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (is_player_grouped(ch, tch))
      continue;
    if (IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM))
      continue;
    if (tch == ch || tch == victim)
      continue;
    if (!IS_NPC(tch) && !PRF_FLAGGED(ch, PRF_PVP))
      continue;

    dam = damage(ch, tch, dam, SKILL_BOMB_TOSS, damType, SKILL_BOMB_TOSS);

    if (dam > 0 && FALSE)
    {
      sprintf(buf, "Your bomb deals %d damage to $N", dam);
      act(buf, FALSE, ch, 0, tch, TO_CHAR);
      sprintf(buf, "$n's bomb deals %d damage to You", dam);
      act(buf, FALSE, ch, 0, tch, TO_VICT);
    }
  }
}

void perform_bomb_direct_effect(struct char_data *ch, struct char_data *victim, int bomb_type)
{

  int saveType = SAVING_FORT,
      noAffectOnSave = TRUE;
  const char *to_vict = NULL, *to_room = NULL;
  const char *to_vict2 = NULL, *to_room2 = NULL;
  struct affected_type af, af2, af3;

  // initialize the affect we're passing
  new_affect(&af);
  new_affect(&af2);
  new_affect(&af3);

  switch (bomb_type)
  {
  case BOMB_ACID:
    af.spell = BOMB_AFFECT_ACID;
    af.duration = 1;
    af.modifier = HAS_FEAT(ch, FEAT_BOMBS) + GET_INT_BONUS(ch);
    to_vict = "The acid burns into your flesh";
    to_room = "The acid burns into $n's flash";
    break;
  case BOMB_BLINDING:
    af.spell = BOMB_AFFECT_BLINDING;
    af.duration = 10;
    af.location = APPLY_HITROLL;
    af.modifier = -4;
    SET_BIT_AR(af.bitvector, AFF_BLIND);

    af2.spell = BOMB_AFFECT_BLINDING;
    af2.duration = 10;
    af2.location = APPLY_AC_NEW;
    af2.modifier = -4;
    SET_BIT_AR(af2.bitvector, AFF_BLIND);

    to_vict = "You've been blinded!";
    to_room = "$n has been blinded!";
    break;
  case BOMB_BONESHARD:
    af.spell = BOMB_AFFECT_BONESHARD;
    af.duration = 10;
    SET_BIT_AR(af.bitvector, AFF_BLEED);
    to_vict = "You stard to bleed.";
    to_room = "$n starts to bleed.";
    break;
  case BOMB_CONCUSSIVE:
    af.spell = BOMB_AFFECT_CONCUSSIVE;
    af.duration = 10;
    SET_BIT_AR(af.bitvector, AFF_DEAF);
    to_vict = "You have been defeaned!";
    to_room = "$n has been deafened!";
    break;
  case BOMB_CONFUSION:
    af.spell = BOMB_AFFECT_CONFUSION;
    af.duration = CLASS_LEVEL(ch, CLASS_ALCHEMIST);
    SET_BIT_AR(af.bitvector, AFF_CONFUSED);
    to_vict = "You feel confused.";
    to_room = "$n looks confused.";
    break;
  case BOMB_FORCE:
    if (!mag_savingthrow(ch, victim, saveType, 0, CAST_BOMB, GET_LEVEL(ch), SCHOOL_NOSCHOOL))
    {
      act("You've been knocked prone!", FALSE, ch, 0, victim, TO_VICT);
      act("$N has been knocked prone!", FALSE, ch, 0, victim, TO_ROOM);
      GET_POS(victim) = POS_SITTING;
      WAIT_STATE(victim, PULSE_VIOLENCE * 2);
    }
    return;
  case BOMB_FROST:
    af.spell = BOMB_AFFECT_FROST;
    af.duration = 1;
    SET_BIT_AR(af.bitvector, AFF_STAGGERED);
    to_vict = "You feel staggered.";
    to_room = "$n looks staggered.";
    break;
  case BOMB_HOLY:
    if (IS_EVIL(victim))
    {
      af.spell = BOMB_AFFECT_HOLY;
      af.duration = 1;
      SET_BIT_AR(af.bitvector, AFF_STAGGERED);
      to_vict = "You feel staggered.";
      to_room = "$n looks staggered.";
    }
    break;
  case BOMB_IMMOLATION:
    if ((HAS_FEAT(ch, FEAT_BOMBS) - 1) > 0)
    {
      af.spell = BOMB_AFFECT_IMMOLATION;
      af.duration = HAS_FEAT(ch, FEAT_BOMBS) - 1;
      af.modifier = 3 + GET_INT_BONUS(ch);
      to_vict = "You're covered in searing, molten rock.";
      to_room = "$n is covered in searing, molten rock.";
    }
    break;
  case BOMB_PROFANE:
    if (IS_GOOD(victim))
    {
      af.spell = BOMB_AFFECT_PROFANE;
      af.duration = 1;
      SET_BIT_AR(af.bitvector, AFF_STAGGERED);
      to_vict = "You feel staggered.";
      to_room = "$n looks staggered.";
    }
    break;
  case BOMB_SHOCK:
    af.spell = BOMB_AFFECT_SHOCK;
    af.duration = dice(1, 4);
    SET_BIT_AR(af.bitvector, AFF_DAZZLED);
    to_vict = "You feel dazzled.";
    to_room = "$n looks dazzled.";
    break;
  case BOMB_SUNLIGHT:
    af.spell = BOMB_AFFECT_SUNLIGHT;
    af.duration = 10;
    af.location = APPLY_HITROLL;
    af.modifier = -4;
    SET_BIT_AR(af.bitvector, AFF_BLIND);

    af.spell = BOMB_AFFECT_SUNLIGHT;
    af2.duration = 10;
    af2.location = APPLY_AC_NEW;
    af2.modifier = -4;
    SET_BIT_AR(af2.bitvector, AFF_BLIND);

    to_vict = "You've been blinded!";
    to_room = "$n has been blinded!";

    if (IS_UNDEAD(victim))
    {
      af3.spell = BOMB_AFFECT_SUNLIGHT;
      af3.duration = 1;
      SET_BIT_AR(af3.bitvector, AFF_STAGGERED);
      to_vict2 = "You feel staggered.";
      to_room2 = "$n looks staggered.";
    }
    break;
  case BOMB_TANGLEFOOT:
    af.spell = BOMB_AFFECT_TANGLEFOOT;
    af.duration = dice(2, 4);
    SET_BIT_AR(af.bitvector, AFF_ENTANGLED);
    to_vict = "You're all tangled up in sticky goo.";
    to_room = "$n is all tangled up in sticky goo.";
    break;
  }

  if (KNOWS_DISCOVERY(ch, ALC_DISC_STICKY_BOMBS))
  {
    af.duration++;
    af2.duration++;
    af3.duration++;
  }

  if (af.spell == 0)
    return;

  if (!noAffectOnSave || !mag_savingthrow(ch, victim, saveType, 0, CAST_BOMB, GET_LEVEL(ch), SCHOOL_NOSCHOOL))
  {

    if (to_vict != NULL)
      act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
    if (to_room != NULL)
      act(to_room, TRUE, victim, 0, ch, TO_ROOM);

    affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);

    if (af2.spell != 0)
    {
      if (to_vict2 != NULL)
        act(to_vict2, FALSE, victim, 0, ch, TO_CHAR);
      if (to_room2 != NULL)
        act(to_room2, TRUE, victim, 0, ch, TO_ROOM);
      affect_join(victim, &af2, TRUE, FALSE, FALSE, FALSE);
    }
    if (af3.spell != 0)
    {
      if (to_vict2 != NULL)
        act(to_vict2, FALSE, victim, 0, ch, TO_CHAR);
      if (to_room2 != NULL)
        act(to_room2, TRUE, victim, 0, ch, TO_ROOM);
      affect_join(victim, &af3, TRUE, FALSE, FALSE, FALSE);
    }
  }
}

void perform_bomb_splash_effect(struct char_data *ch, struct char_data *victim, int bomb_type)
{

  if (!PRF_FLAGGED(ch, PRF_AOE_BOMBS))
    return;

  int saveType = SAVING_FORT,
      noAffectOnSave = TRUE;
  struct char_data *tch = NULL;
  const char *to_vict = NULL, *to_room = NULL;
  const char *to_vict2 = NULL, *to_room2 = NULL;
  struct affected_type af, af2;

  // initialize the affect we're passing
  new_affect(&af);
  new_affect(&af2);

  switch (bomb_type)
  {
  case BOMB_BLINDING:
    af.spell = BOMB_AFFECT_BLINDING;
    af.duration = 10;
    SET_BIT_AR(af.bitvector, AFF_DAZZLED);
    to_vict = "You've been dazzled!";
    to_room = "$n has been dazzled!";
    break;
  case BOMB_SUNLIGHT:
    af.spell = BOMB_AFFECT_SUNLIGHT;
    af.duration = 10;
    SET_BIT_AR(af.bitvector, AFF_DAZZLED);
    to_vict = "You've been dazzled!";
    to_room = "$n has been dazzled!";
    if (IS_UNDEAD(victim))
    {
      af2.spell = BOMB_AFFECT_SUNLIGHT;
      af2.duration = 1;
      SET_BIT_AR(af2.bitvector, AFF_STAGGERED);
      to_vict2 = "You feel staggered.";
      to_room2 = "$n looks staggered.";
    }
    break;
  case BOMB_TANGLEFOOT:
    af.spell = BOMB_AFFECT_TANGLEFOOT;
    af.duration = dice(2, 4);
    SET_BIT_AR(af.bitvector, AFF_ENTANGLED);
    to_vict = "You're all tangled up in sticky goo.";
    to_room = "$n is all tangled up in sticky goo.";
    break;
  }

  if (af.spell == 0)
    return;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (is_player_grouped(ch, tch))
      continue;
    if (IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM))
      continue;
    if (tch == ch || tch == victim)
      continue;
    if (!IS_NPC(tch) && !PRF_FLAGGED(ch, PRF_PVP))
      continue;

    if (!noAffectOnSave || !mag_savingthrow(ch, victim, saveType, 0, CAST_BOMB, GET_LEVEL(ch), SCHOOL_NOSCHOOL))
    {

      if (to_vict != NULL)
        act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
      if (to_room != NULL)
        act(to_room, TRUE, victim, 0, ch, TO_ROOM);

      affect_join(victim, &af, TRUE, FALSE, FALSE, FALSE);

      if (af2.spell != 0)
      {
        if (to_vict2 != NULL)
          act(to_vict2, FALSE, victim, 0, ch, TO_CHAR);
        if (to_room2 != NULL)
          act(to_room2, TRUE, victim, 0, ch, TO_ROOM);
        affect_join(victim, &af2, TRUE, FALSE, FALSE, FALSE);
      }
    }
  }
}

void perform_bomb_direct_healing(struct char_data *ch, struct char_data *victim, int bomb_type)
{

  int healing = 0, new_hp = 0;
  const char *to_vict = NULL, *to_room = NULL;

  switch (bomb_type)
  {
  case BOMB_HEALING:
    healing = dice(HAS_FEAT(ch, FEAT_BOMBS), 4) + GET_INT_BONUS(ch);
    to_vict = "You feel much better!";
    to_room = "$n seems to feel much better.";
    break;
  }

  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);

  new_hp = GET_HIT(victim) + healing;
  if (new_hp > GET_MAX_HIT(victim))
    new_hp = GET_MAX_HIT(victim);
  GET_HIT(victim) = new_hp;
}

void perform_bomb_self_effect(struct char_data *ch, struct char_data *victim, int bomb_type)
{

  const char *to_vict = NULL, *to_room = NULL;
  struct affected_type af;

  new_affect(&(af));

  switch (bomb_type)
  {
  case BOMB_FIRE_BRAND:
    af.spell = BOMB_AFFECT_FIRE_BRAND;
    af.duration = 10;
    to_vict = "Your weapons are set ablaze.";
    to_room = "$ns weapons are set ablaze.";
    break;
  }

  if (af.spell == 0)
    return;

  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);

  affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
}

void perform_bomb_spell_effect(struct char_data *ch, struct char_data *victim, int bomb_type)
{

  int spellnum = -1;

  switch (bomb_type)
  {
  case BOMB_POISON:
    spellnum = SPELL_CLOUDKILL;
    break;
  case BOMB_STINK:
    spellnum = SPELL_STINKING_CLOUD;
    break;
  case BOMB_DISPELLING:
    spellnum = SPELL_DISPEL_MAGIC;
    break;
  }

  if (spellnum == -1)
    return;

  call_magic(ch, victim, NULL, spellnum, 0, CLASS_LEVEL(ch, CLASS_ALCHEMIST), CAST_BOMB);
}

/* Alchemical discovery prerequisites */
int can_learn_discovery(struct char_data *ch, int discovery)
{
  switch (discovery)
  {
  case ALC_DISC_BONESHARD_BOMBS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 8)
      return TRUE;
    break;
  case ALC_DISC_BLINDING_BOMBS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 8)
      return TRUE;
    break;
  case ALC_DISC_CELESTIAL_POISONS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 8)
      return TRUE;
    break;
  case ALC_DISC_CONCUSSIVE_BOMBS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 6)
      return TRUE;
    break;
  case ALC_DISC_CONFUSION_BOMBS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 8)
      return TRUE;
    break;
  case ALC_DISC_DISPELLING_BOMBS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 6)
      return TRUE;
    break;
  case ALC_DISC_FAST_BOMBS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 8)
      return TRUE;
    break;
  case ALC_DISC_FORCE_BOMBS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 8)
      return TRUE;
    break;
  case ALC_DISC_GRAND_COGNATOGEN:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 16 && KNOWS_DISCOVERY(ch, ALC_DISC_GREATER_COGNATOGEN))
      return TRUE;
    break;
  case ALC_DISC_GRAND_INSPIRING_COGNATOGEN:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 16 && KNOWS_DISCOVERY(ch, ALC_DISC_GREATER_INSPIRING_COGNATOGEN))
      return TRUE;
    break;
  case ALC_DISC_GRAND_MUTAGEN:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 16 && KNOWS_DISCOVERY(ch, ALC_DISC_GREATER_MUTAGEN))
      return TRUE;
    break;
  case ALC_DISC_GREATER_COGNATOGEN:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 12 && KNOWS_DISCOVERY(ch, ALC_DISC_COGNATOGEN))
      return TRUE;
    break;
  case ALC_DISC_GREATER_INSPIRING_COGNATOGEN:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 12 && KNOWS_DISCOVERY(ch, ALC_DISC_INSPIRING_COGNATOGEN))
      return TRUE;
    break;
  case ALC_DISC_GREATER_MUTAGEN:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 12)
      return TRUE;
    break;
  case ALC_DISC_HEALING_TOUCH:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 6 && KNOWS_DISCOVERY(ch, ALC_DISC_SPONTANEOUS_HEALING))
      return TRUE;
    break;
  case ALC_DISC_HOLY_BOMBS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 8)
      return TRUE;
    break;
  case ALC_DISC_IMMOLATION_BOMBS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 3)
      return TRUE;
    break;
  case ALC_DISC_MALIGNANT_POISON:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 10)
      return TRUE;
    break;
  case ALC_DISC_POISON_BOMBS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 12)
      return TRUE;
    break;
  case ALC_DISC_PROFANE_BOMBS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 8)
      return TRUE;
    break;
  case ALC_DISC_PSYCHOKINETIC_TINCTURE:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 4)
      return TRUE;
    break;
  case ALC_DISC_STICKY_BOMBS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 10)
      return TRUE;
    break;
  case ALC_DISC_SUNLIGHT_BOMBS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 10 && KNOWS_DISCOVERY(ch, ALC_DISC_BLINDING_BOMBS))
      return TRUE;
    break;
  case ALC_DISC_WINGS:
    if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) >= 6)
      return TRUE;
    break;
  default:
    return TRUE; // all other discoveries can be taken at any time
  }
  return FALSE; // if the conditions above fail, they can't learn it yet.
}

int num_alchemical_discoveries_known(struct char_data *ch)
{
  int i = 0;
  int num_chosen = 0;

  for (i = 0; i < NUM_ALC_DISCOVERIES; i++)
    if (KNOWS_DISCOVERY(ch, i))
      num_chosen++;
  return num_chosen;
}

sbyte has_alchemist_discoveries_unchosen(struct char_data *ch)
{

  if (!ch)
    return false;

  if (IS_NPC(ch))
    return false;

  int num_avail = HAS_FEAT(ch, FEAT_ALCHEMICAL_DISCOVERY);
  int num_chosen = num_alchemical_discoveries_known(ch);

  if ((num_avail - num_chosen) > 0)
    return TRUE;

  return FALSE;
}

sbyte has_alchemist_discoveries_unchosen_study(struct char_data *ch)
{

  if (!ch)
    return false;

  if (IS_NPC(ch))
    return false;

  int num_avail = HAS_FEAT(ch, FEAT_ALCHEMICAL_DISCOVERY);
  int num_chosen = num_alchemical_discoveries_known(ch);
  int i = 0;
  int num_study = 0;

  for (i = 0; i < NUM_ALC_DISCOVERIES; i++)
    if (LEVELUP(ch)->discoveries[i])
      num_study++;

  if ((num_avail - num_chosen - num_study) > 0)
    return TRUE;

  return FALSE;
}

int list_alchemical_discoveries(struct char_data *ch)
{
  int i = 0;
  int num = 0;

  for (i = 0; i < NUM_ALC_DISCOVERIES; i++)
  {
    if (KNOWS_DISCOVERY(ch, i))
    {
      send_to_char(ch, "%-15s : %s\r\n", alchemical_discovery_names[i], alchemical_discovery_descriptions[i]);
      num++;
    }
  }
  return num;
}

ACMD(do_discoveries)
{

  if (!CLASS_LEVEL(ch, CLASS_ALCHEMIST))
  {
    send_to_char(ch, "You need to have some levels in the alchemist class to view your learned alchemical discoveries.\r\n");
    return;
  }

  int num = 0, i = 0;
  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "ALCHEMICAL DISCOVERIES KNOWN:\r\n");
    if (list_alchemical_discoveries(ch) == 0)
      send_to_char(ch, "No discoveries known.");
    send_to_char(ch, "\r\n");
    if ((num = (HAS_FEAT(ch, FEAT_ALCHEMICAL_DISCOVERY) - num_alchemical_discoveries_known(ch))) > 0)
    {
      send_to_char(ch, "You have %d discover%s available to be learned.  Discoveries may be learned at your trainer via the study menu.\r\n", num, num > 1 ? "ies" : "y");
    }
    send_to_char(ch, "You can type 'discoveries all' to see all available discoveries.\r\n");
    send_to_char(ch, "You can type 'discoveries (discovery name)' to see full information on the discovery specified.\r\n");
    send_to_char(ch, "\r\n");
  }
  else if (is_abbrev(argument, "all"))
  {
    send_to_char(ch, "ALCHEMICAL DISCOVERIES:\r\n");
    for (i = 1; i < NUM_ALC_DISCOVERIES; i++)
    {
      send_to_char(ch, "%-20s - %s\r\n", alchemical_discovery_names[i], alchemical_discovery_descriptions[i]);
    }
    send_to_char(ch, "You can type 'discoveries all' to see all available discoveries.\r\n");
    send_to_char(ch, "You can type 'discoveries (discovery name)' to see full information on the discovery specified.\r\n");
    send_to_char(ch, "\r\n");
  }
  else
  {
    for (i = 1; i < NUM_ALC_DISCOVERIES; i++)
    {
      if (is_abbrev(argument, alchemical_discovery_names[i]))
      {
        send_to_char(ch, "%-20s:\r\nDesc: %s\r\nRequirements: %s\r\n", alchemical_discovery_names[i], alchemical_discovery_descriptions[i], discovery_requisites[i]);
        return;
      }
    }
    send_to_char(ch, "That is not an available option.\r\n");
    send_to_char(ch, "You can type 'discoveries all' to see all available discoveries.\r\n");
    send_to_char(ch, "You can type 'discoveries (discovery name)' to see full information on the discovery specified.\r\n");
    send_to_char(ch, "\r\n");
  }
}

ACMD(do_grand_discoveries)
{

  if (CLASS_LEVEL(ch, CLASS_ALCHEMIST) < 20)
  {
    send_to_char(ch, "You need to have at least 20 alchemist levels to learn a grand discovery.\r\n");
    return;
  }

  int num = 0, i = 0;
  skip_spaces(&argument);

  if (!GET_GRAND_DISCOVERY(ch))
  {
    if (!*argument)
    {
      send_to_char(ch, "GRAND ALCHEMICAL DISCOVERIES AVAILABLE:\r\n");
      for (i = 1; i < NUM_GR_ALC_DISCOVERIES; i++)
      {
        send_to_char(ch, "%-20s - %s\r\n", grand_alchemical_discovery_names[i], grand_alchemical_discovery_descriptions[i]);
      }
      send_to_char(ch, "\r\n"
                       "To choose one, type 'gdiscovery learn (grand discovery name)\r\n"
                       "You may only choose one, and must respec to change that choice if desired.\r\n"
                       "\r\n");
      return;
    }
    char arg1[200], arg2[200];
    half_chop(argument, arg1, arg2);
    if (!*arg1 || !is_abbrev(arg1, "learn"))
    {
      send_to_char(ch, "To learn a grand discovery type 'gdiscovery learn (grand discovery name)\r\n");
      return;
    }
    if (!*arg2)
    {
      send_to_char(ch, "You must specify a grand discovery to learn.  Type 'gdiscovery' by itself to see a list.\r\n");
      return;
    }
    for (i = 1; i < NUM_GR_ALC_DISCOVERIES; i++)
    {
      if (is_abbrev(arg2, grand_alchemical_discovery_names[i]))
        break;
    }
    if (i >= NUM_GR_ALC_DISCOVERIES)
    {
      send_to_char(ch, "That is not a valid grand discovery. Type 'gdiscovery' by itself to see a list.\r\n");
      return;
    }
    GET_GRAND_DISCOVERY(ch) = i;
    send_to_char(ch, "You have learned the grand discovery: %s!\r\n", grand_alchemical_discovery_names[i]);
    if (i == GR_ALC_DISC_AWAKENED_INTELLECT)
    {
      GET_REAL_INT(ch) += 2;
    }
    save_char(ch, 0);
    return;
  }
  else
  {
    send_to_char(ch, "You know the grand discovery:\r\n%-20s : %s\r\n", grand_alchemical_discovery_names[GET_GRAND_DISCOVERY(ch)], grand_alchemical_discovery_descriptions[GET_GRAND_DISCOVERY(ch)]);
    return;
  }

  if (!*argument)
  {
    send_to_char(ch, "GRAND ALCHEMICAL DISCOVERIES AVAILABLE:\r\n");
    if (list_alchemical_discoveries(ch) == 0)
      send_to_char(ch, "No discoveries known.");
    send_to_char(ch, "\r\n");
    if ((num = (HAS_FEAT(ch, FEAT_ALCHEMICAL_DISCOVERY) - num_alchemical_discoveries_known(ch))) > 0)
    {
      send_to_char(ch, "You have %d discover%s available to be learned.  Discoveries may be learned at your trainer via the study menu.\r\n", num, num > 1 ? "ies" : "y");
    }
    send_to_char(ch, "You can type 'discoveries all' to see all available discoveries.\r\n");
    send_to_char(ch, "You can type 'discoveries (discovery name)' to see full information on the discovery specified.\r\n");
    send_to_char(ch, "\r\n");
  }
  else if (is_abbrev(argument, "all"))
  {
    send_to_char(ch, "ALCHEMICAL DISCOVERIES:\r\n");
    for (i = 1; i < NUM_ALC_DISCOVERIES; i++)
    {
      send_to_char(ch, "%-20s - %s\r\n", alchemical_discovery_names[i], alchemical_discovery_descriptions[i]);
    }
    send_to_char(ch, "You can type 'discoveries all' to see all available discoveries.\r\n");
    send_to_char(ch, "You can type 'discoveries (discovery name)' to see full information on the discovery specified.\r\n");
    send_to_char(ch, "\r\n");
  }
  else
  {
    for (i = 1; i < NUM_ALC_DISCOVERIES; i++)
    {
      if (is_abbrev(argument, alchemical_discovery_names[i]))
      {
        send_to_char(ch, "%-20s:\r\nDesc: %s\r\nRequirements: %s\r\n", alchemical_discovery_names[i], alchemical_discovery_descriptions[i], discovery_requisites[i]);
        return;
      }
    }
    send_to_char(ch, "That is not an available option.\r\n");
    send_to_char(ch, "You can type 'discoveries all' to see all available discoveries.\r\n");
    send_to_char(ch, "You can type 'discoveries (discovery name)' to see full information on the discovery specified.\r\n");
    send_to_char(ch, "\r\n");
  }
}

sbyte bomb_is_friendly(int bomb)
{

  switch (bomb)
  {
  case BOMB_HEALING:
  case BOMB_FIRE_BRAND:
    return TRUE;
  }
  return FALSE;
}

/* a function to clear mutagen effects and do other dirty work associated with that */
void clear_mutagen(struct char_data *ch)
{

  send_to_char(ch, "Your mutagen's effect expires...\r\n");
  act("$n mutagenic enhancement has faded.", FALSE, ch, NULL, NULL, TO_ROOM);
}

ACMDCHECK(can_swallow)
{
  ACMDCHECK_PERMFAIL_IF(!HAS_FEAT(ch, FEAT_MUTAGEN), "You don't know how to prepare a mutagen or cognatogen.\r\n");
  return CAN_CMD;
}

void perform_mutagen(struct char_data *ch, char *arg2)
{

  struct affected_type af, af2, af3, af4, af5, af6, af7;
  int duration = 0, mod1 = 0, mod2 = 0, mod3 = 0, mod4 = 0;

  new_affect(&af);
  new_affect(&af2);
  new_affect(&af3);
  new_affect(&af4);
  new_affect(&af5);
  new_affect(&af6);
  new_affect(&af7);

  af.bonus_type = af2.bonus_type = af3.bonus_type = af4.bonus_type = af5.bonus_type = af6.bonus_type = af7.bonus_type = BONUS_TYPE_ALCHEMICAL;

  /* duration */
  duration = 100 * CLASS_LEVEL(ch, CLASS_ALCHEMIST);

  if (is_abbrev(arg2, "strength"))
  {
    af.location = APPLY_STR;
    af2.location = APPLY_INT;
    if (KNOWS_DISCOVERY(ch, ALC_DISC_GRAND_MUTAGEN))
    {
      mod1 = 8;
      af3.location = APPLY_DEX;
      mod3 = 6;
      af4.location = APPLY_CON;
      mod4 = 4;
    }
    else if (KNOWS_DISCOVERY(ch, ALC_DISC_GREATER_MUTAGEN))
    {
      mod1 = 6;
      af3.location = APPLY_DEX;
      mod3 = 4;
    }
    else
    {
      mod1 = 4;
    }
  }
  else if (is_abbrev(arg2, "dexterity"))
  {
    af.location = APPLY_DEX;
    af2.location = APPLY_WIS;
    if (KNOWS_DISCOVERY(ch, ALC_DISC_GRAND_MUTAGEN))
    {
      mod1 = 8;
      af3.location = APPLY_CON;
      mod3 = 6;
      af4.location = APPLY_STR;
      mod4 = 4;
    }
    else if (KNOWS_DISCOVERY(ch, ALC_DISC_GREATER_MUTAGEN))
    {
      af3.location = APPLY_CON;
      mod3 = 4;
    }
    else
    {
      mod1 = 4;
    }
  }
  else if (is_abbrev(arg2, "constitution"))
  {
    af.location = APPLY_CON;
    af2.location = APPLY_CHA;
    if (KNOWS_DISCOVERY(ch, ALC_DISC_GRAND_MUTAGEN))
    {
      mod1 = 8;
      af3.location = APPLY_STR;
      mod3 = 6;
      af4.location = APPLY_DEX;
      mod4 = 4;
    }
    else if (KNOWS_DISCOVERY(ch, ALC_DISC_GREATER_MUTAGEN))
    {
      af3.location = APPLY_STR;
      mod3 = 4;
    }
    else
    {
      mod1 = 4;
    }
  }
  else
  {
    send_to_char(ch, "Do you want your mutagen to affect your strength (-int), dexterity (-wis) or constitution (-cha)?\r\n");
    return;
  }

  // this is the penalty to ability score associated the physical ability chosen above
  mod2 = -2;
  if (KNOWS_DISCOVERY(ch, ALC_DISC_INFUSE_MUTAGEN))
    mod2++;

  if (GET_GRAND_DISCOVERY(ch) == GR_ALC_DISC_TRUE_MUTAGEN)
  {
    af.modifier = af3.modifier = af4.modifier = af5.modifier = 8;
    af2.modifier = af6.modifier = af7.modifier = mod2;
    af2.location = APPLY_INT;
    af6.location = APPLY_WIS;
    af7.location = APPLY_CHA;
    af.location = APPLY_STR;
    af3.location = APPLY_DEX;
    af4.location = APPLY_CON;
  }
  else if (KNOWS_DISCOVERY(ch, ALC_DISC_GRAND_MUTAGEN))
  {
    af5.modifier = 6;
  }
  else if (KNOWS_DISCOVERY(ch, ALC_DISC_GREATER_MUTAGEN))
  {
    af5.modifier = 4;
  }
  else
  {
    af5.modifier = 2;
  }

  if (mod1 != 0)
  {
    af.modifier = mod1;
    af.spell = SKILL_MUTAGEN;
    af.duration = duration;
    affect_to_char(ch, &af);
  }
  if (mod2 != 0)
  {
    af2.modifier = mod2;
    af2.spell = SKILL_MUTAGEN;
    af2.duration = duration;
    affect_to_char(ch, &af2);
  }
  if (mod3 != 0)
  {
    af3.modifier = mod3;
    af3.spell = SKILL_MUTAGEN;
    af3.duration = duration;
    affect_to_char(ch, &af3);
  }
  if (mod4 != 0)
  {
    af4.modifier = mod4;
    af4.spell = SKILL_MUTAGEN;
    af4.duration = duration;
    affect_to_char(ch, &af4);
  }
  if (af5.modifier != 0)
  {
    af5.location = APPLY_AC_NEW;
    af5.bonus_type = BONUS_TYPE_NATURALARMOR;
    af5.spell = SKILL_MUTAGEN;
    af5.duration = duration;
    affect_to_char(ch, &af5);
  }
  if (af6.location != APPLY_NONE)
  {
    af6.spell = SKILL_MUTAGEN;
    af6.duration = duration;
    affect_to_char(ch, &af6);
  }
  if (af7.location != APPLY_NONE)
  {
    af7.spell = SKILL_MUTAGEN;
    af7.duration = duration;
    affect_to_char(ch, &af7);
  }

  act("$n swallows a vial of murky looking substance and grows more physically powerful before your eyes.", FALSE, ch, 0, 0, TO_ROOM);
  act("You swallow a vial of mutagen and grow more physically powerful in an instant.", FALSE, ch, 0, 0, TO_CHAR);
}

void perform_elemental_mutagen(struct char_data *ch, char *arg2)
{

  struct affected_type af, af2;
  int duration = 0, mod1 = 5, mod2 = 5;

  new_affect(&af);
  new_affect(&af2);

  af.bonus_type = af2.bonus_type = BONUS_TYPE_ALCHEMICAL;

  /* duration */
  duration = 100 * CLASS_LEVEL(ch, CLASS_ALCHEMIST);

  if (is_abbrev(arg2, "air"))
  {
    af.location = APPLY_RES_ELECTRIC;
    af2.location = APPLY_SKILL;
    af2.specific = ABILITY_PERCEPTION;
  }
  else if (is_abbrev(arg2, "earth"))
  {
    af.location = APPLY_RES_ACID;
    af2.location = APPLY_SKILL;
    af2.specific = ABILITY_CLIMB;
  }
  else if (is_abbrev(arg2, "fire"))
  {
    af.location = APPLY_RES_FIRE;
    af2.location = APPLY_SKILL;
    af2.specific = ABILITY_ACROBATICS;
  }
  else if (is_abbrev(arg2, "water"))
  {
    af.location = APPLY_RES_COLD;
    af2.location = APPLY_SKILL;
    af2.specific = ABILITY_SWIM;
  }
  else
  {
    send_to_char(ch, "Do you want your elemental mutagen to affect air (electricity, perception), earth (acid, climb), fire (fire, acrobatics) or water (cold, swim)?\r\n"
                     "You will gain resistance 5 in the associated element and +5 to the associated skill.\r\n");
    return;
  }

  if (KNOWS_DISCOVERY(ch, ALC_DISC_GRAND_INSPIRING_COGNATOGEN))
  {
    mod1 += 5;
    mod2 += 2;
  }
  if (KNOWS_DISCOVERY(ch, ALC_DISC_GREATER_INSPIRING_COGNATOGEN))
  {
    mod1 += 5;
    mod2 += 2;
  }
  if (GET_GRAND_DISCOVERY(ch) == GR_ALC_DISC_TRUE_MUTAGEN)
  {
    mod1 += 5;
    mod2 += 2;
  }

  if (mod1 != 0)
  {
    af.modifier = mod1;
    af.spell = SKILL_MUTAGEN;
    af.duration = duration;
    af.modifier = mod1;
    affect_to_char(ch, &af);
  }
  if (mod2 != 0)
  {
    af2.spell = SKILL_MUTAGEN;
    af2.duration = duration;
    af2.modifier = mod2;
    affect_to_char(ch, &af2);
  }

  act("$n swallows a vial of murky looking substance and looks more resistant before your eyes.", FALSE, ch, 0, 0, TO_ROOM);
  act("You swallow a vial of inspiring cognatogen and feel more resistant in an instant.", FALSE, ch, 0, 0, TO_CHAR);
}

void perform_cognatogen(struct char_data *ch, char *arg2)
{

  struct affected_type af, af2, af3, af4, af5, af6, af7;
  int duration = 0, mod1 = 0, mod2 = 0, mod3 = 0, mod4 = 0;

  new_affect(&af);
  new_affect(&af2);
  new_affect(&af3);
  new_affect(&af4);
  new_affect(&af5);
  new_affect(&af6);
  new_affect(&af7);

  af.bonus_type = af2.bonus_type = af3.bonus_type = af4.bonus_type = af5.bonus_type = af6.bonus_type = af7.bonus_type = BONUS_TYPE_ALCHEMICAL;

  /* duration */
  duration = 100 * CLASS_LEVEL(ch, CLASS_ALCHEMIST);

  if (is_abbrev(arg2, "intelligence"))
  {
    af.location = APPLY_INT;
    af2.location = APPLY_STR;
    if (KNOWS_DISCOVERY(ch, ALC_DISC_GRAND_COGNATOGEN))
    {
      mod1 = 8;
      af3.location = APPLY_WIS;
      mod3 = 6;
      af4.location = APPLY_CHA;
      mod4 = 4;
    }
    else if (KNOWS_DISCOVERY(ch, ALC_DISC_GREATER_COGNATOGEN))
    {
      af3.location = APPLY_WIS;
      mod3 = 4;
    }
    else
    {
      mod1 = 4;
    }
  }
  else if (is_abbrev(arg2, "wisdom"))
  {
    af.location = APPLY_WIS;
    af2.location = APPLY_DEX;
    if (KNOWS_DISCOVERY(ch, ALC_DISC_GRAND_COGNATOGEN))
    {
      mod1 = 8;
      af3.location = APPLY_CHA;
      mod3 = 6;
      af4.location = APPLY_INT;
      mod4 = 4;
    }
    else if (KNOWS_DISCOVERY(ch, ALC_DISC_GREATER_COGNATOGEN))
    {
      af3.location = APPLY_CHA;
      mod3 = 4;
    }
    else
    {
      mod1 = 4;
    }
  }
  else if (is_abbrev(arg2, "charisma"))
  {
    af.location = APPLY_CHA;
    af2.location = APPLY_CON;
    if (KNOWS_DISCOVERY(ch, ALC_DISC_GRAND_COGNATOGEN))
    {
      mod1 = 8;
      af3.location = APPLY_INT;
      mod3 = 6;
      af4.location = APPLY_WIS;
      mod4 = 4;
    }
    else if (KNOWS_DISCOVERY(ch, ALC_DISC_GREATER_COGNATOGEN))
    {
      af3.location = APPLY_INT;
      mod3 = 4;
    }
    else
    {
      mod1 = 4;
    }
  }
  else
  {
    send_to_char(ch, "Do you want your cognatogen to affect your intelligence (-str), wisdom (-dex) or charisma (-con)?\r\n");
    return;
  }

  // this is the penalty to ability score associated the physical ability chosen above
  mod2 = -2;
  if (KNOWS_DISCOVERY(ch, ALC_DISC_INFUSE_MUTAGEN))
    mod2++;

  if (GET_GRAND_DISCOVERY(ch) == GR_ALC_DISC_TRUE_MUTAGEN)
  {
    af.modifier = af3.modifier = af4.modifier = af5.modifier = 8;
    af2.modifier = af6.modifier = af7.modifier = mod2;
    af.location = APPLY_INT;
    af3.location = APPLY_WIS;
    af4.location = APPLY_CHA;
    af2.location = APPLY_STR;
    af6.location = APPLY_DEX;
    af7.location = APPLY_CON;
  }
  else if (KNOWS_DISCOVERY(ch, ALC_DISC_GRAND_MUTAGEN))
  {
    af5.modifier = 6;
  }
  else if (KNOWS_DISCOVERY(ch, ALC_DISC_GREATER_MUTAGEN))
  {
    af5.modifier = 4;
  }
  else
  {
    af5.modifier = 2;
  }

  if (mod1 != 0)
  {
    af.modifier = mod1;
    af.spell = SKILL_COGNATOGEN;
    af.duration = duration;
    affect_to_char(ch, &af);
  }
  if (mod2 != 0)
  {
    af2.modifier = mod2;
    af2.spell = SKILL_COGNATOGEN;
    af2.duration = duration;
    affect_to_char(ch, &af2);
  }
  if (mod3 != 0)
  {
    af3.modifier = mod3;
    af3.spell = SKILL_COGNATOGEN;
    af3.duration = duration;
    affect_to_char(ch, &af3);
  }
  if (mod4 != 0)
  {
    af4.modifier = mod4;
    af4.spell = SKILL_COGNATOGEN;
    af4.duration = duration;
    affect_to_char(ch, &af4);
  }
  if (af5.modifier != 0)
  {
    af5.location = APPLY_AC_NEW;
    af5.bonus_type = BONUS_TYPE_NATURALARMOR;
    af5.spell = SKILL_COGNATOGEN;
    af5.duration = duration;
    affect_to_char(ch, &af5);
  }
  if (af6.location != APPLY_NONE)
  {
    af6.spell = SKILL_COGNATOGEN;
    af6.duration = duration;
    affect_to_char(ch, &af6);
  }
  if (af7.location != APPLY_NONE)
  {
    af7.spell = SKILL_COGNATOGEN;
    af7.duration = duration;
    affect_to_char(ch, &af7);
  }

  act("$n swallows a vial of murky looking substance and grows more mentally powerful before your eyes.", FALSE, ch, 0, 0, TO_ROOM);
  act("You swallow a vial of mutagen and grow more mentally powerful in an instant.", FALSE, ch, 0, 0, TO_CHAR);
}

void perform_inspiring_cognatogen(struct char_data *ch)
{

  struct affected_type af, af2, af3, af4, af5;
  int duration = 0, mod1 = 0, mod2 = 0, mod3 = 0, mod4 = 0, mod5 = 0;

  new_affect(&af);
  new_affect(&af2);
  new_affect(&af3);
  new_affect(&af4);
  new_affect(&af5);

  af.bonus_type = af2.bonus_type = af3.bonus_type = af4.bonus_type = af5.bonus_type = BONUS_TYPE_ALCHEMICAL;

  /* duration */
  duration = 100 * CLASS_LEVEL(ch, CLASS_ALCHEMIST);

  af.location = APPLY_AC_NEW;
  af.bonus_type = BONUS_TYPE_DODGE;
  af5.location = APPLY_SKILL;

  if (KNOWS_DISCOVERY(ch, ALC_DISC_GRAND_INSPIRING_COGNATOGEN))
  {
    mod1 = 4;
    mod5 = 4;
    af2.location = APPLY_SAVING_REFL;
    mod2 = 4;
    af3.location = APPLY_STR;
    af3.modifier = KNOWS_DISCOVERY(ch, ALC_DISC_INFUSE_MUTAGEN) ? -3 : -4;
    af4.location = APPLY_CON;
    af4.modifier = KNOWS_DISCOVERY(ch, ALC_DISC_INFUSE_MUTAGEN) ? -3 : -4;
  }
  else if (KNOWS_DISCOVERY(ch, ALC_DISC_GREATER_INSPIRING_COGNATOGEN))
  {
    mod5 = 3;
    af2.location = APPLY_SAVING_REFL;
    mod2 = 4;
    af3.location = APPLY_STR;
    af3.modifier = KNOWS_DISCOVERY(ch, ALC_DISC_INFUSE_MUTAGEN) ? -1 : -2;
    af4.location = APPLY_CON;
    af4.modifier = KNOWS_DISCOVERY(ch, ALC_DISC_INFUSE_MUTAGEN) ? -1 : -2;
  }
  else
  {
    mod1 = 2;
    mod5 = 2;
  }

  // this is the penalty to ability score associated the physical ability chosen above
  mod2 = -2;
  if (KNOWS_DISCOVERY(ch, ALC_DISC_INFUSE_MUTAGEN))
    mod2++;

  if (mod1 != 0)
  {
    af.modifier = mod1;
    af.spell = SKILL_INSPIRING_COGNATOGEN;
    af.duration = duration;
    affect_to_char(ch, &af);
  }
  if (mod2 != 0)
  {
    af2.modifier = mod2;
    af2.spell = SKILL_INSPIRING_COGNATOGEN;
    af2.duration = duration;
    affect_to_char(ch, &af2);
  }
  if (mod3 != 0)
  {
    af3.modifier = mod3;
    af3.spell = SKILL_INSPIRING_COGNATOGEN;
    af3.duration = duration;
    affect_to_char(ch, &af3);
  }
  if (mod4 != 0)
  {
    af4.modifier = mod4;
    af4.spell = SKILL_INSPIRING_COGNATOGEN;
    af4.duration = duration;
    affect_to_char(ch, &af4);
  }
  if (mod5 != 0)
  {
    af5.modifier = mod5;
    af5.spell = SKILL_INSPIRING_COGNATOGEN;
    af5.duration = duration;
    affect_to_char(ch, &af5);
  }

  act("$n swallows a vial of murky looking substance and grows more inspired before your eyes.", FALSE, ch, 0, 0, TO_ROOM);
  act("You swallow a vial of inspiring cognatogen and feel more inspired in an instant.", FALSE, ch, 0, 0, TO_CHAR);
}

ACMD(do_swallow)
{
  char arg1[100], arg2[100];

  // If currently raging, all this does is stop.
  if (affected_by_spell(ch, SKILL_MUTAGEN))
  {
    clear_mutagen(ch);
    affect_from_char(ch, SKILL_MUTAGEN);
    return;
  }
  if (affected_by_spell(ch, SKILL_COGNATOGEN))
  {
    clear_mutagen(ch);
    affect_from_char(ch, SKILL_COGNATOGEN);
    return;
  }
  if (affected_by_spell(ch, SKILL_INSPIRING_COGNATOGEN))
  {
    clear_mutagen(ch);
    affect_from_char(ch, SKILL_INSPIRING_COGNATOGEN);
    return;
  }

  PREREQ_CHECK(can_swallow);

  if (!IS_NPC(ch))
  {
    PREREQ_HAS_USES(FEAT_MUTAGEN, "You must wait some time before you can prepare another mutagen or cognatogen.\r\n");
  }

  two_arguments(argument, arg1, arg2);

  if (!*arg1)
  {
    send_to_char(ch, "Are you trying to swallow a mutagen, elemental-mutagen, cognatogen or inspiring-cognatogen?\r\n");
    return;
  }

  if (is_abbrev(arg1, "mutagen"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Do you want your mutagen to affect your strength (-int), dexterity (-wis) or constitution (-cha)?\r\n");
      return;
    }
    perform_mutagen(ch, strdup(arg2));
  }
  else if (is_abbrev(arg1, "elemental-mutagen"))
  {
    if (!KNOWS_DISCOVERY(ch, ALC_DISC_ELEMENTAL_MUTAGEN))
    {
      send_to_char(ch, "You don't know how to prepare an elemental mutagen.\r\n");
      return;
    }
    if (!*arg2)
    {
      send_to_char(ch, "Do you want your elemental mutagen to affect air (electricity, perception), earth (acid, climb), fire (fire, acrobatics) or water (cold, swim)?\r\n"
                       "You will gain resistance 5 in the associated element and +5 to the associated skill.\r\n");
      return;
    }
    perform_elemental_mutagen(ch, strdup(arg2));
  }
  else if (is_abbrev(arg1, "cognatogen"))
  {
    if (!KNOWS_DISCOVERY(ch, ALC_DISC_COGNATOGEN))
    {
      send_to_char(ch, "You don't know how to prepare a cognatogen.\r\n");
      return;
    }
    if (!*arg2)
    {
      send_to_char(ch, "Do you want your cognatogen to affect your intelligence (-str), wisdom (-dex) or charisma (-con)?\r\n");
      return;
    }
    perform_cognatogen(ch, strdup(arg2));
  }
  else if (is_abbrev(arg1, "inspiring-cognatogen"))
  {
    if (!KNOWS_DISCOVERY(ch, ALC_DISC_INSPIRING_COGNATOGEN))
    {
      send_to_char(ch, "You don't know how to prepare an isnpiring cognatogen.\r\n");
      return;
    }
    perform_inspiring_cognatogen(ch);
  }
  else
  {
    send_to_char(ch, "Are you trying to swallow a mutagen, elemental-mutagen, cognatogen or inspiring-cognatogen?\r\n");
    return;
  }

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_MUTAGEN);

  if (!IS_NPC(ch) && CLASS_LEVEL(ch, CLASS_ALCHEMIST) < 1)
  {
    USE_STANDARD_ACTION(ch);
  }
}

void add_sticky_bomb_effect(struct char_data *ch, struct char_data *vict, int bomb_type)
{
  if (!ch || !vict)
    return;

  int damage = HAS_FEAT(ch, FEAT_BOMBS) + GET_INT_BONUS(ch);
  int dam_type = 0;

  switch (bomb_type)
  {
  case BOMB_NORMAL:
    dam_type = DAM_FIRE;
    break;
  case BOMB_ACID:
    dam_type = DAM_ACID;
    break;
  case BOMB_CONCUSSIVE:
    dam_type = DAM_SOUND;
    break;
  case BOMB_FORCE:
    dam_type = DAM_FORCE;
    break;
  case BOMB_FROST:
    dam_type = DAM_COLD;
    break;
  case BOMB_HOLY:
    dam_type = DAM_HOLY;
    break;
  case BOMB_PROFANE:
    dam_type = DAM_UNHOLY;
    break;
  case BOMB_SHOCK:
    dam_type = DAM_ELECTRIC;
    break;
  case BOMB_SUNLIGHT:
    dam_type = DAM_LIGHT;
    if (IS_UNDEAD(vict) || IS_OOZE(vict))
      damage += (HAS_FEAT(ch, FEAT_BOMBS) / 2) + 2;
    break;
  }

  vict->player_specials->sticky_bomb[0] = bomb_type;
  vict->player_specials->sticky_bomb[1] = dam_type;
  vict->player_specials->sticky_bomb[2] = damage;
}

ACMD(do_curingtouch)
{
  int uses_remaining = 0;
  char arg1[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!KNOWS_DISCOVERY(ch, ALC_DISC_SPONTANEOUS_HEALING))
  {
    send_to_char(ch, "You do not know that alchemist discovery!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_CURING_TOUCH)) == 0)
  {
    send_to_char(ch, "You have no alchemical reserves to apply a curing touch.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  one_argument(argument, arg1);

  if (!*arg1)
  {
    vict = ch;
  }
  else
  {

    if (!KNOWS_DISCOVERY(ch, ALC_DISC_HEALING_TOUCH))
    {
      send_to_char(ch, "You can only perform a curing touch on yourself.  The curing touch alchemical discovery allows for perform this on others as well as increasing the amount you can heal.");
      return;
    }

    if (!(vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_ROOM)))
    {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  if (GET_HIT(vict) >= GET_MAX_HIT(vict))
  {
    send_to_char(ch, "That person does not need curing.\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "You cure yourself with your alchemical salve!\r\n");
    act("$n heals some wounds with an alchemical salve!", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  else
  {
    act("You cure $N with an alchemical salve.", FALSE, ch, 0, vict, TO_CHAR);
    act("$n cures you with an alchemical salve", FALSE, ch, 0, vict, TO_VICT);
    act("$n cures $N with an alchemical salve", FALSE, ch, 0, vict, TO_NOTVICT);
  }

  GET_HIT(vict) += 5;

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_CURING_TOUCH);

  USE_SWIFT_ACTION(ch);
}

// psychokinetic tincture alchemist discovery
// using the tincture creates spirits that circle the alchemist
// with one spirit per 4 alchemist levels.  Each spirit gives
// the alchemist +1 deflectionn AC.  The alchemist can 'launch'
// a spirit at the foe they are fighting, who must make a will
// save or be frightened for 1 round / alchemist level.  The
// PRF_FRIGHTEN flag, if turned on will cause the taregt to try
// and flee.  If turned off the target will not attempt to flee.
// In either case, they suffer -2 to attack rolls, saving throws,
// skill checks and ability checks.

ACMD(do_psychokinetic)
{

  char buf[200];

  // do they know how to do it?
  if (!KNOWS_DISCOVERY(ch, ALC_DISC_PSYCHOKINETIC_TINCTURE))
  {
    send_to_char(ch, "You do not know the psychokinetic tincture alchemist discovery.\r\n");
    return;
  }

  skip_spaces(&argument);

  // check to see if they specified an argument
  if (!*argument)
  {
    send_to_char(ch, "You need to specify either 'apply' to activate, 'dispel' to end the effect or 'launch' to send one of the psychokinetic spirits at your currently targetted foe.\r\n");
    return;
  }

  if (is_abbrev(argument, "apply"))
  {

    // check if they have any uses available
    if (!IS_NPC(ch))
    {
      PREREQ_HAS_USES(FEAT_PSYCHOKINETIC, "You must wait some time before you can apply another psychokinetic tincture.\r\n");
    }

    if (affected_by_spell(ch, ALC_DISC_AFFECT_PSYCHOKINETIC))
    {
      send_to_char(ch, "You are already under the effect of a psychokinetic tincture.  You may either 'launch' the psychokinetic spirits in battle, or use 'psychokinetic dispel' to end the effect voluntarily.\r\n");
      return;
    }

    struct affected_type af;

    af.spell = ALC_DISC_AFFECT_PSYCHOKINETIC;
    af.duration = 50 * CLASS_LEVEL(ch, CLASS_ALCHEMIST);
    af.modifier = MAX(1, CLASS_LEVEL(ch, CLASS_ALCHEMIST) / 4);
    af.location = APPLY_AC_NEW;
    af.bonus_type = BONUS_TYPE_DEFLECTION;
    if (PRF_FLAGGED(ch, PRF_FRIGHTENED))
      SET_BIT_AR(af.bitvector, AFF_FEAR);
    else
      SET_BIT_AR(af.bitvector, AFF_SHAKEN);

    affect_to_char(ch, &af);
    sprintf(buf, "You drink your psychokinetic tincture and are suddenly surrounded by %d protective spirit%s.\r\n", af.modifier, af.modifier == 1 ? "" : "s");
    act(buf, FALSE, ch, 0, 0, TO_CHAR);
    sprintf(buf, "$n drinks a strange fluid and is suddenly surrounded by %d protective spirit%s.\r\n", af.modifier, af.modifier == 1 ? "" : "s");
    act(buf, FALSE, ch, 0, 0, TO_ROOM);

    if (!IS_NPC(ch))
      start_daily_use_cooldown(ch, FEAT_MUTAGEN);

    if (!IS_NPC(ch) && CLASS_LEVEL(ch, CLASS_ALCHEMIST) < 1)
    {
      USE_STANDARD_ACTION(ch);
    }

    return;
  }
  else if (is_abbrev(argument, "dispel"))
  {

    if (!affected_by_spell(ch, ALC_DISC_AFFECT_PSYCHOKINETIC))
    {
      send_to_char(ch, "You are not under the effect of a psychokinetic tincture.\r\n");
      return;
    }

    affect_from_char(ch, ALC_DISC_AFFECT_PSYCHOKINETIC);
    send_to_char(ch, "You banish the psychokinetic spirits.\r\n");
    return;
  }
  else if (is_abbrev(argument, "launch"))
  {

    struct char_data *victim = NULL;

    if (!(victim = FIGHTING(ch)))
    {
      send_to_char(ch, "This can only be done in battle.\r\n");
      return;
    }

    act("You launch one of your psychokinetic spirits at $N.", FALSE, ch, 0, victim, TO_CHAR);
    act("$n launchs one of $s psychokinetic spirits at You.", FALSE, ch, 0, victim, TO_VICT);
    act("$n launchs one of $s psychokinetic spirits at $N.", FALSE, ch, 0, victim, TO_NOTVICT);

    if (is_immune_fear(ch, victim, TRUE))
      return;
    if (is_immune_mind_affecting(ch, victim, TRUE))
      return;

    if (mag_savingthrow(ch, victim, SAVING_WILL, GET_RESISTANCES(victim, DAM_MENTAL), CAST_BOMB, CLASS_LEVEL(ch, CLASS_ALCHEMIST), SCHOOL_NOSCHOOL))
    {
      act("$N resists the fear effect.", FALSE, ch, 0, victim, TO_CHAR);
      act("You resist the fear effect.", FALSE, ch, 0, victim, TO_VICT);
    }
    else
    {
      act("You are filled with terror.", FALSE, ch, 0, victim, TO_VICT);
      act("$N is filled with terror.", FALSE, ch, 0, victim, TO_ROOM);
      if (PRF_FLAGGED(ch, PRF_FRIGHTENED))
      {
        do_flee(victim, 0, 0, 0);
      }

      struct affected_type *af = NULL;
      for (af = ch->affected; af; af = af->next)
      {
        if (af->spell == ALC_DISC_AFFECT_PSYCHOKINETIC)
        {
          af->modifier -= 1;
          if (af->modifier <= 0)
          {
            affect_from_char(ch, ALC_DISC_AFFECT_PSYCHOKINETIC);
            send_to_char(ch, "You have launched the last of your psychokinetic spirits.\r\n");
          }
        }
      }
    }

    if (!IS_NPC(ch) && CLASS_LEVEL(ch, CLASS_ALCHEMIST) < 1)
    {
      USE_STANDARD_ACTION(ch);
    }
    return;
  }
  else
  {
    send_to_char(ch, "You need to specify either 'apply' to activate, 'dispel' to end the effect or 'launch' to send one of the psychokinetic spirits at your currently targetted foe.\r\n");
    return;
  }
}

ACMD(do_poisontouch)
{
  if (GET_GRAND_DISCOVERY(ch) != GR_ALC_DISC_POISON_TOUCH)
  {
    send_to_char(ch, "You do not know how to perform a poison touch.\r\n");
    return;
  }

  struct char_data *vict = FIGHTING(ch);

  if (!vict)
  {
    send_to_char(ch, "You can only use this ability in combat.\r\n");
    return;
  }

  if (attack_roll(ch, vict, ATTACK_TYPE_PRIMARY, TRUE, 1) > 0)
  {

    if (check_poison_resist(ch, vict, CASTING_TYPE_ANY, CLASS_LEVEL(ch, CLASS_ALCHEMIST)))
    {
      act("You touch $n with your poisonous touch, but $E resists!", FALSE, ch, 0, vict, TO_CHAR);
      act("$n touches You with $s poisonous touch, but You resist!", FALSE, ch, 0, vict, TO_VICT);
      act("$n touches $N with $s poisonous touch, but $E resists!", FALSE, ch, 0, vict, TO_NOTVICT);
    }
    else
    {

      struct affected_type af;

      af.spell = SPELL_POISON;
      SET_BIT_AR(af.bitvector, AFF_POISON);
      af.location = APPLY_CON;
      af.modifier = dice(1, 3);
      af.bonus_type = BONUS_TYPE_ALCHEMICAL;
      af.duration = 10;

      affect_join(vict, &af, TRUE, FALSE, TRUE, FALSE);

      act("You touch $N with your poisonous touch and $E becomes very ill!", FALSE, ch, 0, vict, TO_CHAR);
      act("$n touches You with $s poisonous touch and You become very ill!", FALSE, ch, 0, vict, TO_VICT);
      act("$n touches $N with $s poisonous touch and $E becomes very ill!", FALSE, ch, 0, vict, TO_NOTVICT);
    }
  }
  else
  {
    act("You try to poison $N with your poisonous touch, but fail.", TRUE, ch, 0, vict, TO_CHAR);
    act("$n tries to poison You with $s poisonous touch, but fails.", TRUE, ch, 0, vict, TO_VICT);
    act("$n tries to poison $N with $s poisonous touch, but fails.", TRUE, ch, 0, vict, TO_NOTVICT);
  }

  USE_STANDARD_ACTION(ch);
}

int find_discovery_num(char *name)
{
  int index, ok;
  char *temp, *temp2;
  char first[256], first2[256];

  for (index = 0; index < NUM_ALC_DISCOVERIES; index++)
  {
    if (is_abbrev(name, alchemical_discovery_names[index]))
      return (index);

    ok = TRUE;
    /* It won't be changed, but other uses of this function elsewhere may. */
    temp = any_one_arg((char *)alchemical_discovery_names[index], first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok)
    {
      if (!is_abbrev(first2, first))
        ok = FALSE;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }
    if (ok && !*first2)
      return (index);
  }

  return (-1);
}

/* display_discovery_info()
 *
 * Show information about a particular discovery, dynamically
 * generated to tailor the output to a particular player.
 *
 * (NOTE: The headers of the sections above will be colored
 * differently, making them stand out.) */
bool display_discovery_info(struct char_data *ch, char *discoveryname)
{
  int discovery = -1;
  char buf[MAX_STRING_LENGTH];

  //  static int line_length = 57;
  static int line_length = 80;

  skip_spaces(&discoveryname);

  discovery = find_discovery_num(discoveryname);

  if (discovery == -1)
  {
    /* Not found - Maybe put in a soundex list here? */
    //send_to_char(ch, "Could not find that discovery.\r\n");
    return FALSE;
  }

  /* We found the discovery, and the discovery number is stored in 'discovery'. */
  /* Display the discovery info, formatted. */
  send_to_char(ch, "\tC\r\n");
  //text_line(ch, "discovery Information", line_length, '-', '-');
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tcDiscovery     : \tn%s\r\n",
               alchemical_discovery_names[discovery]);
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /*  Here display the prerequisites */
  sprintf(buf, "\tCPrerequisites : \tn%s\r\n", discovery_requisites[discovery]);
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /* This we will need to buffer and wrap so that it will fit in the space provided. */
  sprintf(buf, "\tcDescription   : \tn%s\r\n",
          alchemical_discovery_descriptions[discovery]);
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn\r\n");

  return TRUE;
}

int find_grand_discovery_num(char *name)
{
  int index, ok;
  char *temp, *temp2;
  char first[256], first2[256];

  for (index = 1; index < NUM_GR_ALC_DISCOVERIES; index++)
  {
    if (is_abbrev(name, grand_alchemical_discovery_names[index]))
      return (index);

    ok = TRUE;
    /* It won't be changed, but other uses of this function elsewhere may. */
    temp = any_one_arg((char *)grand_alchemical_discovery_names[index], first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok)
    {
      if (!is_abbrev(first2, first))
        ok = FALSE;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }
    if (ok && !*first2)
      return (index);
  }

  return (-1);
}

/* display_grand_discovery_info()
 *
 * Show information about a particular discovery, dynamically
 * generated to tailor the output to a particular player.
 *
 * (NOTE: The headers of the sections above will be colored
 * differently, making them stand out.) */
bool display_grand_discovery_info(struct char_data *ch, char *discoveryname)
{
  int discovery = -1;
  char buf[MAX_STRING_LENGTH];

  //  static int line_length = 57;
  static int line_length = 80;

  skip_spaces(&discoveryname);
  discovery = find_grand_discovery_num(discoveryname);

  if (discovery == -1)
  {
    /* Not found - Maybe put in a soundex list here? */
    //send_to_char(ch, "Could not find that discovery.\r\n");
    return FALSE;
  }

  /* We found the discovery, and the discovery number is stored in 'discovery'. */
  /* Display the discovery info, formatted. */
  send_to_char(ch, "\tC\r\n");
  //text_line(ch, "discovery Information", line_length, '-', '-');
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tcGrand Discovery    : \tn%s\r\n",
               grand_alchemical_discovery_names[discovery]);
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /*  Here display the prerequisites */
  sprintf(buf, "\tCPrerequisites : \tnLevel 20 Alchemist\r\n");
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  /* This we will need to buffer and wrap so that it will fit in the space provided. */
  sprintf(buf, "\tcDescription : \tn%s\r\n",
          grand_alchemical_discovery_descriptions[discovery]);
  send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn\r\n");

  return TRUE;
}

bool display_bomb_types(struct char_data *ch, char *keyword)
{
  if (!is_abbrev(keyword, "alchemist bombs") && !is_abbrev(keyword, "alchemist-bombs"))
    return FALSE;
  char buf[MAX_STRING_LENGTH];

  //  static int line_length = 57;
  static int line_length = 80;

  send_to_char(ch, "\tC\r\n");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tcAlchemist Bomb Types: \tn\r\n");
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  int i = 0;
  for (i = 1; i < NUM_BOMB_TYPES; i++)
  {
    sprintf(buf, "\tC%s bomb\tn\r\n", bomb_types[i]);
    send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  }

  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tcType HELP (BOMB NAME) for information on a specific bomb.\tn\r\n");
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn\r\n");
  return TRUE;
}

bool display_discovery_types(struct char_data *ch, char *keyword)
{

  if (!is_abbrev(keyword, "alchemist discoveries") && !is_abbrev(keyword, "alchemical discoveries") && !is_abbrev(keyword, "discoveries") &&
      !is_abbrev(keyword, "alchemist grand discoveries") && !is_abbrev(keyword, "alchemical grand discoveries") && !is_abbrev(keyword, "grand discoveries"))
    return FALSE;

  bool grand = FALSE;

  if (is_abbrev(keyword, "alchemist discoveries") || is_abbrev(keyword, "alchemical discoveries") || is_abbrev(keyword, "discoveries"))
    grand = FALSE;
  else
    grand = TRUE;

  char buf[MAX_STRING_LENGTH];

  //  static int line_length = 57;
  static int line_length = 80;

  send_to_char(ch, "\tC\r\n");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tcAlchemist %sDiscoveries: \tn\r\n", grand ? "Grand " : "");
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  int i = 0;
  for (i = 1; i < (grand ? NUM_GR_ALC_DISCOVERIES : NUM_ALC_DISCOVERIES); i++)
  {
    sprintf(buf, "\tC%s\tn\r\n", grand ? grand_alchemical_discovery_names[i] : alchemical_discovery_names[i]);
    send_to_char(ch, "%s", strfrmt(buf, line_length, 1, FALSE, FALSE, FALSE));
  }

  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tcType HELP (DISCOVERY NAME) for information on a specific bomb or discovery.\r\n"
                   "Discoveries can be learned in the study menu. Grand discoveries can be learned\r\n"
                   "with the 'gdiscovery' command.\tn\r\n");
  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');

  send_to_char(ch, "\tC");
  draw_line(ch, line_length, '-', '-');
  send_to_char(ch, "\tn\r\n");

  return TRUE;
}
