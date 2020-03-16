/*/ \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \
\                                                             
/  Luminari Treasure System, Inspired by D20mud's Treasure System                                                           
/  Created By: Zusuk, original d20 code written by Gicker                                                           
\                                                             
/  using treasure.h as the header file currently                                                           
\         todo: CP system by Ornir                                               
/                                                                                                                                                                                       
\ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ / \ /*/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "dg_scripts.h"
#include "treasure.h"
#include "craft.h"
#include "assign_wpn_armor.h"
#include "oasis.h"
#include "item.h"

/***  utility functions ***/

/* utility function to label 'rare grade' gear */
char *label_rare_grade(int rare_grade)
{

  switch (rare_grade)
  {
  case RARE_GRADE_MYTHICAL:
    return "[Mythical] ";
  case RARE_GRADE_LEGENDARY:
    return "[Legendary] ";
  case RARE_GRADE_RARE:
    return "[Rare] ";
  }
  return "";
}

/* utility function to determine 'rare grade' - extra special items */
int determine_rare_grade()
{
  int roll = 0, rare_grade = RARE_GRADE_NORMAL;

  /* determine if rare or not */
  roll = dice(1, 100);

  if (roll == 1)
  {
    rare_grade = RARE_GRADE_MYTHICAL;
  }
  else if (roll <= 5)
  {
    rare_grade = RARE_GRADE_LEGENDARY;
  }
  else if (roll <= 15)
  {
    rare_grade = RARE_GRADE_RARE;
  }

  return rare_grade;
}

/* utility function that converts a grade rating to an enchantment rating */
int cp_convert_grade_enchantment(int grade)
{
  int enchantment = 0;

  switch (grade)
  {
  case GRADE_MINOR:
    if (rand_number(0, 1))
      enchantment = 2;
    else
      enchantment = 1;
    break;
  case GRADE_TYPICAL:
    if (rand_number(0, 1))
      enchantment = 3;
    else
      enchantment = 2;
    break;
  case GRADE_MEDIUM:
    if (rand_number(0, 2))
      enchantment = 3;
    else
      enchantment = 4;
    break;
  case GRADE_MAJOR:
    if (rand_number(0, 2))
      enchantment = 4;
    else
      enchantment = 5;
    break;
  case GRADE_SUPERIOR:
    if (rand_number(0, 3))
      enchantment = 5;
    else
      enchantment = 6;
    break;
  default: //GRADE_MUNDANE:
    if (rand_number(0, 1))
      enchantment = 1;
    else
      enchantment = 0;
    break;
  }

  return enchantment;
}

/* determine a random category for misc treasure */
int determine_rnd_misc_cat()
{
  int diceroll = dice(1, 11);
  int category = 0;

  switch (diceroll)
  {
  case 1: /*ring*/
  case 2:
    category = TRS_SLOT_FINGER;
    break;
  case 3: /*neck*/
  case 4:
    category = TRS_SLOT_NECK;
    break;
  case 5: /*wrist*/
  case 6:
    category = TRS_SLOT_WRIST;
    break;
  case 7: /*feet*/
    category = TRS_SLOT_FEET;
    break;
  case 8:                  /*hands*/
    if (dice(1, 10) == 10) /* monk gloves */
      category = TRS_SLOT_MONK_GLOVE;
    else
      category = TRS_SLOT_HANDS;
    break;
  case 9: /*about*/
    category = TRS_SLOT_ABOUT;
    break;
  case 10: /*waist*/
    category = TRS_SLOT_WAIST;
    break;
  case 11: /*held*/
    category = TRS_SLOT_HELD;
    break;
  }

  return category;
}

/* this function is used to inform ch and surrounding of a bazaar purchase */
void say_bazaar(struct char_data *ch, struct obj_data *obj)
{
  if (ch && obj)
  {
    /* message */
    send_to_char(ch, "Here is your item!\r\n");
    do_stat_object(ch, obj, ITEM_STAT_MODE_IDENTIFY_SPELL);
    act("$n \tYhas acquired\tn $p\tn\tY from the bazaar.\tn", FALSE, ch, obj, ch, TO_NOTVICT);
  }
}

/* this function is used to inform ch and surrounding of a treasure drop */
void say_treasure(struct char_data *ch, struct obj_data *obj)
{
  if (ch && obj && obj->short_description)
  {
    char buf[MAX_STRING_LENGTH] = {'\0'};

    send_to_char(ch, "\tYYou have found %s\tn\tY in a nearby lair (random treasure drop)!\tn\r\n", obj->short_description);

    snprintf(buf, MAX_STRING_LENGTH, "$n \tYhas found %s\tn\tY in a nearby lair (random treasure drop)!\tn", obj->short_description);
    act(buf, FALSE, ch, 0, ch, TO_NOTVICT);
  }
}

/* some spells are not appropriate for expendable items, this simple
 function returns TRUE if the spell is OK, FALSE if not */
bool valid_item_spell(int spellnum)
{
  /* just list exceptions NOTE if you add any exception here, better make
   sure wizards have another way of getting the spell
   * update: added 'research' so wizards have another way to acquire spells */
  switch (spellnum)
  {
  case SPELL_VENTRILOQUATE:
  case SPELL_MUMMY_DUST:
  case SPELL_DRAGON_KNIGHT:
  case SPELL_GREATER_RUIN:
  case SPELL_HELLBALL:
  case SPELL_EPIC_MAGE_ARMOR:
  case SPELL_EPIC_WARDING:
  case SPELL_FIRE_BREATHE:
  case SPELL_STENCH:
  case SPELL_ACID_SPLASH:
  case SPELL_RAY_OF_FROST:
  case SPELL_FSHIELD_DAM:
  case SPELL_CSHIELD_DAM:
  case SPELL_ASHIELD_DAM:
  case SPELL_DEATHCLOUD:
  case SPELL_ACID:
  case SPELL_INCENDIARY:
  case SPELL_FROST_BREATHE:
  case SPELL_LIGHTNING_BREATHE:
  case SPELL_UNUSED285:
  case SPELL_ANIMATE_DEAD:
  case SPELL_GREATER_ANIMATION:
  case SPELL_BLADES:
  case SPELL_CONTROL_WEATHER:
  case SPELL_I_DARKNESS:
    return FALSE;
  }
  return TRUE;
}

/* simple function to give a random metal type (currently unused) */
int choose_metal_material(void)
{
  switch (dice(1, 12))
  {
  case 1:
  case 2:
  case 3:
    return MATERIAL_BRONZE;
  case 4:
  case 5:
    return MATERIAL_STEEL;
  case 6:
    return MATERIAL_ALCHEMAL_SILVER;
  case 7:
    return MATERIAL_COLD_IRON;
  case 8:
    return MATERIAL_ADAMANTINE;
  case 9:
    return MATERIAL_MITHRIL;
  default:
    return MATERIAL_IRON;
  }
}

/* simple function to give a random precious metal type (currently unused) */
int choose_precious_metal_material(void)
{
  switch (dice(1, 9))
  {
  case 1:
  case 2:
    return MATERIAL_BRASS;
  case 3:
  case 4:
    return MATERIAL_SILVER;
  case 5:
    return MATERIAL_GOLD;
  case 6:
    return MATERIAL_PLATINUM;
  default:
    return MATERIAL_COPPER;
  }
}

/* simple function to give a random cloth type (currently unused) */
int choose_cloth_material(void)
{
  switch (dice(1, 12))
  {
  case 1:
  case 2:
  case 3:
    return MATERIAL_COTTON;
  case 4:
  case 5:
    return MATERIAL_WOOL;
  case 6:
    return MATERIAL_BURLAP;
  case 7:
    return MATERIAL_VELVET;
  case 8:
    return MATERIAL_SATIN;
  case 9:
    return MATERIAL_SILK;
  default:
    return MATERIAL_HEMP;
  }
}

/* determine appropriate stat bonus apply for this piece of gear */
int determine_stat_apply(int wear)
{
  int stat = APPLY_NONE;

  switch (wear)
  {
  case WEAR_FINGER_R:
  case WEAR_FINGER_L:
    switch (rand_number(1, 7))
    {
    case 1:
      stat = APPLY_WIS;
      break;
    case 2:
      stat = APPLY_SAVING_WILL;
      break;
    case 3:
      stat = APPLY_HIT;
      break;
    case 4:
      stat = APPLY_RES_FIRE;
      break;
    case 5:
      stat = APPLY_RES_PUNCTURE;
      break;
    case 6:
      stat = APPLY_RES_ILLUSION;
      break;
    case 7:
      stat = APPLY_RES_ENERGY;
      break;
    }
    break;
  case WEAR_NECK_1:
  case WEAR_NECK_2:
    switch (rand_number(1, 7))
    {
    case 1:
      stat = APPLY_INT;
      break;
    case 2:
      stat = APPLY_SAVING_REFL;
      break;
    case 3:
      stat = APPLY_RES_COLD;
      break;
    case 4:
      stat = APPLY_RES_AIR;
      break;
    case 5:
      stat = APPLY_RES_FORCE;
      break;
    case 6:
      stat = APPLY_RES_MENTAL;
      break;
    case 7:
      stat = APPLY_RES_WATER;
      break;
    }
    break;
  case WEAR_WRIST_R:
  case WEAR_WRIST_L:
    switch (rand_number(1, 6))
    {
    case 1:
      stat = APPLY_SAVING_FORT;
      break;
    case 2:
      stat = APPLY_PSP;
      break;
    case 3:
      stat = APPLY_RES_ELECTRIC;
      break;
    case 4:
      stat = APPLY_RES_UNHOLY;
      break;
    case 5:
      stat = APPLY_RES_SOUND;
      break;
    case 6:
      stat = APPLY_RES_LIGHT;
      break;
    }
    break;
  case WEAR_HOLD_1:
  case WEAR_HOLD_2:
  case WEAR_HOLD_2H:
    switch (rand_number(1, 3))
    {
    case 1:
      stat = APPLY_INT;
      break;
    case 2:
      stat = APPLY_CHA;
      break;
    case 3:
      stat = APPLY_HIT;
      break;
    }
    break;
  case WEAR_FEET:
    switch (rand_number(1, 3))
    {
    case 1:
      stat = APPLY_RES_POISON;
      break;
    case 2:
      stat = APPLY_DEX;
      break;
    case 3:
      stat = APPLY_MOVE;
      break;
    }
    break;
  case WEAR_HANDS:
    switch (rand_number(1, 3))
    {
    case 1:
      stat = APPLY_RES_SLICE;
      break;
    case 2:
      stat = APPLY_STR;
      break;
    case 3:
      stat = APPLY_RES_DISEASE;
      break;
    }
    break;
  case WEAR_ABOUT:
    switch (rand_number(1, 3))
    {
    case 1:
      stat = APPLY_RES_ACID;
      break;
    case 2:
      stat = APPLY_CHA;
      break;
    case 3:
      stat = APPLY_RES_NEGATIVE;
      break;
    }
    break;
  case WEAR_WAIST:
    switch (rand_number(1, 3))
    {
    case 1:
      stat = APPLY_RES_HOLY;
      break;
    case 2:
      stat = APPLY_CON;
      break;
    case 3:
      stat = APPLY_RES_EARTH;
      break;
    }
    break;
  default:
    stat = APPLY_NONE;
  }

  return stat;
}

/* pick a random feat to put on our special items */
int apply_bonus_feat(int rare_grade)
{

  /* just in case */
  if (rare_grade <= RARE_GRADE_NORMAL)
    return FEAT_UNDEFINED;
  if (rare_grade > RARE_GRADE_MYTHICAL)
    return FEAT_UNDEFINED;

  int feat_num = FEAT_UNDEFINED;
  int roll = 1;

  switch (rare_grade)
  {
  case RARE_GRADE_RARE:
    roll = dice(1, 93);
    break;
  case RARE_GRADE_LEGENDARY:
    roll = dice(1, 121);
    break;
  case RARE_GRADE_MYTHICAL:
    roll = dice(1, 151);
    break;
  }

  if (roll <= 3)
    feat_num = FEAT_ALERTNESS;
  else if (roll <= 6)
    feat_num = FEAT_ARMOR_PROFICIENCY_LIGHT;
  else if (roll <= 9)
    feat_num = FEAT_ARMOR_PROFICIENCY_MEDIUM;
  else if (roll <= 12)
    feat_num = FEAT_ENDURANCE;
  else if (roll <= 15)
    feat_num = FEAT_FAST_MOVEMENT;
  else if (roll <= 18)
    feat_num = FEAT_DETECT_EVIL;
  else if (roll <= 21)
    feat_num = FEAT_SLEEP_ENCHANTMENT_IMMUNITY;
  else if (roll <= 24)
    feat_num = FEAT_RESISTANCE_TO_ENCHANTMENTS;
  else if (roll <= 27)
    feat_num = FEAT_IRON_WILL;
  else if (roll <= 30)
    feat_num = FEAT_LIGHTNING_REFLEXES;
  else if (roll <= 33)
    feat_num = FEAT_STABILITY;
  else if (roll <= 36)
    feat_num = FEAT_TOUGHNESS;
  else if (roll <= 39)
    feat_num = FEAT_LUCKY;
  else if (roll <= 42)
    feat_num = FEAT_UNARMED_STRIKE;
  else if (roll <= 45)
    feat_num = FEAT_INFRAVISION;
  else if (roll <= 48)
    feat_num = FEAT_DETECT_GOOD;
  else if (roll <= 51)
    feat_num = FEAT_RESISTANCE_TO_ILLUSIONS;
  else if (roll <= 54)
    feat_num = FEAT_ARMOR_PROFICIENCY_SHIELD;
  else if (roll <= 57)
    feat_num = FEAT_ANIMAL_AFFINITY;
  else if (roll <= 60)
    feat_num = FEAT_DECEITFUL;
  else if (roll <= 63)
    feat_num = FEAT_DEFT_HANDS;
  else if (roll <= 66)
    feat_num = FEAT_DILIGENT;
  else if (roll <= 69)
    feat_num = FEAT_MAGICAL_APTITUDE;
  else if (roll <= 72)
    feat_num = FEAT_NIMBLE_FINGERS;
  else if (roll <= 75)
    feat_num = FEAT_PERSUASIVE;
  else if (roll <= 78)
    feat_num = FEAT_SELF_SUFFICIENT;
  else if (roll <= 81)
    feat_num = FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD;
  else if (roll <= 84)
    feat_num = FEAT_ARMOR_SPECIALIZATION_LIGHT;
  else if (roll <= 87)
    feat_num = FEAT_ARMOR_SPECIALIZATION_MEDIUM;
  else if (roll <= 90)
    feat_num = FEAT_STRONG_AGAINST_POISON;
  else if (roll <= 93)
    feat_num = FEAT_STRONG_AGAINST_DISEASE;

  /* nicer feats */
  else if (roll <= 95)
    feat_num = FEAT_ARMOR_SPECIALIZATION_HEAVY;
  else if (roll <= 97)
    feat_num = FEAT_BARDIC_KNOWLEDGE;
  else if (roll <= 99)
    feat_num = FEAT_TWO_WEAPON_DEFENSE;
  else if (roll <= 101)
    feat_num = FEAT_STEALTHY;
  else if (roll <= 103)
    feat_num = FEAT_RAPID_RELOAD;
  else if (roll <= 105)
    feat_num = FEAT_IMPROVED_SHIELD_PUNCH;
  else if (roll <= 107)
    feat_num = FEAT_IMPROVED_FEINT;
  else if (roll <= 109)
    feat_num = FEAT_IMPROVED_GRAPPLE;
  else if (roll <= 111)
    feat_num = FEAT_EXOTIC_WEAPON_PROFICIENCY;
  else if (roll <= 113)
    feat_num = FEAT_SNEAK_ATTACK;
  else if (roll <= 115)
    feat_num = FEAT_FAST_HEALER;
  else if (roll <= 117)
    feat_num = FEAT_ABLE_LEARNER;
  else if (roll <= 119)
    feat_num = FEAT_IMPROVED_TAUNTING;
  else if (roll <= 121)
    feat_num = FEAT_IMPROVED_INTIMIDATION;
  else if (roll <= 123)
    feat_num = FEAT_WEAPON_FINESSE;
  else if (roll <= 125)
    feat_num = FEAT_TWO_WEAPON_FIGHTING;
  else if (roll <= 127)
    feat_num = FEAT_SPRING_ATTACK;
  else if (roll <= 129)
    feat_num = FEAT_MOBILITY;
  else if (roll <= 131)
    feat_num = FEAT_MARTIAL_WEAPON_PROFICIENCY;
  else if (roll <= 133)
    feat_num = FEAT_IMPROVED_TRIP;
  else if (roll <= 135)
    feat_num = FEAT_IMPROVED_INITIATIVE;
  else if (roll <= 137)
    feat_num = FEAT_IMPROVED_DISARM;
  else if (roll <= 139)
    feat_num = FEAT_RAGE;
  else if (roll <= 141)
    feat_num = FEAT_DODGE;
  else if (roll <= 143)
    feat_num = FEAT_DEFLECT_ARROWS;
  else if (roll <= 145)
    feat_num = FEAT_COMBAT_REFLEXES;
  else if (roll <= 147)
    feat_num = FEAT_CLEAVE;
  else if (roll <= 149)
    feat_num = FEAT_ARMOR_PROFICIENCY_HEAVY;
  else if (roll <= 151)
    feat_num = FEAT_BLIND_FIGHT;

  /* just in case */
  if (feat_num < 0)
    feat_num = FEAT_UNDEFINED;
  if (feat_num >= NUM_FEATS)
    feat_num = FEAT_UNDEFINED;

  return feat_num;
}

/* function to adjust the bonus value based on the apply location */

/* called by random_bonus_value(), cp_modify_object_applies(), */
int adjust_bonus_value(int apply_location, int bonus)
{
  int adjusted_bonus = bonus;
  switch (apply_location)
  {
  case APPLY_HIT:
    adjusted_bonus = bonus * 12;
    break;
  case APPLY_MOVE:
    adjusted_bonus = bonus * 120;
    break;
    /* no modifications */
  default:
    break;
  }
  return MIN(RANDOM_BONUS_CAP, adjusted_bonus);
}

/* assign bonus-types to the bonus */

/* called by: cp_modify_object_applies() */
int adjust_bonus_type(int apply_location)
{
  switch (apply_location)
  {
  case APPLY_SAVING_FORT:
  case APPLY_SAVING_REFL:
  case APPLY_SAVING_WILL:
    return BONUS_TYPE_RESISTANCE;
  default:
    return BONUS_TYPE_ENHANCEMENT;
    break;
  }
}

/* function that returns bonus value based on apply-value and level */

/* called by award_random_crystal() */
int random_bonus_value(int apply_value, int level, int mod)
{
  int bonus = MAX(1, (level / BONUS_FACTOR) + mod);
  return adjust_bonus_value(apply_value, bonus);
}

/* when grouped, determine random recipient from group */
struct char_data *find_treasure_recipient(struct char_data *ch)
{
  struct group_data *group = NULL;
  struct char_data *target = NULL;

  /* assign group data */
  if ((group = ch->group) == NULL)
    return ch;

  /* pick random group member */
  target = random_from_list(group->members);

  /* same room? */
  if (IN_ROOM(ch) != IN_ROOM(target))
    target = ch;

  return (target);
}

/***  primary functions ***/

/* this function determines whether the character will get treasure or not
 *   for example, called before make_corpse() when killing a mobile */
void determine_treasure(struct char_data *ch, struct char_data *mob)
{
  int gold = 0;
  int level = 0;
  char buf[MEDIUM_STRING] = {'\0'};
  int grade = GRADE_MUNDANE, max_grade = GRADE_MUNDANE;

  if (IS_NPC(ch))
    return;
  if (!IS_NPC(mob))
    return;

  gold = dice(1, GET_LEVEL(mob)) * 10;
  level = GET_LEVEL(mob);

  /* determine maximum grade of treasure */
  if (level >= 25)
  {
    max_grade = GRADE_SUPERIOR;
  }
  else if (level >= 20)
  {
    max_grade = GRADE_MAJOR;
  }
  else if (level >= 15)
  {
    max_grade = GRADE_MEDIUM;
  }
  else if (level >= 10)
  {
    max_grade = GRADE_TYPICAL;
  }
  else if (level >= 5)
  {
    max_grade = GRADE_MINOR;
  }
  else
  {
    max_grade = GRADE_MUNDANE;
  }

  /* okay now determine grade */
  grade = dice(1, max_grade);

  if (dice(1, 100) <= MAX(TREASURE_PERCENT, HAPPY_TREASURE))
  {
    award_magic_item(1, ch, grade);
    snprintf(buf, MEDIUM_STRING, "\tYYou have found %d coins hidden on $N's corpse!\tn", gold);
    act(buf, FALSE, ch, 0, mob, TO_CHAR);
    snprintf(buf, MEDIUM_STRING, "$n \tYhas has found %d coins hidden on $N's corpse!\tn", gold);
    act(buf, FALSE, ch, 0, mob, TO_NOTVICT);
    GET_GOLD(ch) += gold;
    /* does not split this gold, maybe change later */
  }
}

/* character should get treasure, roll dice for what items to give out */
void award_magic_item(int number, struct char_data *ch, int grade)
{
  int i = 0;
  int roll = 0;
  if (number <= 0)
    number = 1;

  if (number >= 50)
    number = 50;

  /* crystals drop 5% of the time.
   * scrolls/wands/potions/staves drop 40% of the time. (each 10%)
   * trinkets (bracelets, rings, etc. including cloaks, boots and gloves) drop 20% of the time.
   * armor (head, arms, legs, body) drop 25% of the time.
   * weapons drop 10% of the time. */
  for (i = 0; i < number; i++)
  {
    roll = dice(1, 100);
    if (roll <= 5) /*  1 - 5    5%  */
      award_random_crystal(ch, grade);
    else if (roll <= 15) /*  6 - 15   10% */
      award_magic_weapon(ch, grade);
    else if (roll <= 55)
    { /* 16 - 55   40% */
      switch (dice(1, 6))
      {
      case 1:
        award_expendable_item(ch, grade, TYPE_SCROLL);
        break;
      case 2:
        award_expendable_item(ch, grade, TYPE_POTION);
        break;
      case 3:
        award_expendable_item(ch, grade, TYPE_WAND);
        break;
      case 4:
        award_expendable_item(ch, grade, TYPE_STAFF);
        break;
      default: /* 5- 6 */
        award_magic_ammo(ch, grade);
        break;
      }
    }
    else if (roll <= 75)
    { /* 56 - 75   20% */
      award_misc_magic_item(ch, determine_rnd_misc_cat(), cp_convert_grade_enchantment(grade));
    }
    else
    { /* 76 - 100  25% */
      award_magic_armor(ch, grade, -1);
    }
  }
}

int random_apply_value(void)
{
  int val = APPLY_NONE;

  /* There will be different groupings based on item type and wear location,
   * for example weapons will get hit/dam bonus (the +) and armor will get
   * ac_apply_new bonus (the +). */
  switch (dice(1, 12))
  {
  case 1:
    val = APPLY_HIT;
    break;
  case 2:
    val = APPLY_STR;
    break;
  case 3:
    val = APPLY_CON;
    break;
  case 4:
    val = APPLY_DEX;
    break;
  case 5:
    val = APPLY_INT;
    break;
  case 6:
    val = APPLY_WIS;
    break;
  case 7:
    val = APPLY_CHA;
    break;
  case 8:
    val = APPLY_MOVE;
    break;
  case 9:
    val = APPLY_SAVING_FORT;
    break;
  case 10:
    val = APPLY_SAVING_REFL;
    break;
  case 11:
    val = APPLY_SAVING_WILL;
    break;
  default:
    switch (rand_number(1, 20))
    {
    case 1:
      val = APPLY_RES_FIRE;
      break;
    case 2:
      val = APPLY_RES_COLD;
      break;
    case 3:
      val = APPLY_RES_AIR;
      break;
    case 4:
      val = APPLY_RES_EARTH;
      break;
    case 5:
      val = APPLY_RES_ACID;
      break;
    case 6:
      val = APPLY_RES_HOLY;
      break;
    case 7:
      val = APPLY_RES_ELECTRIC;
      break;
    case 8:
      val = APPLY_RES_UNHOLY;
      break;
    case 9:
      val = APPLY_RES_SLICE;
      break;
    case 10:
      val = APPLY_RES_PUNCTURE;
      break;
    case 11:
      val = APPLY_RES_FORCE;
      break;
    case 12:
      val = APPLY_RES_SOUND;
      break;
    case 13:
      val = APPLY_RES_POISON;
      break;
    case 14:
      val = APPLY_RES_DISEASE;
      break;
    case 15:
      val = APPLY_RES_NEGATIVE;
      break;
    case 16:
      val = APPLY_RES_ILLUSION;
      break;
    case 17:
      val = APPLY_RES_MENTAL;
      break;
    case 18:
      val = APPLY_RES_LIGHT;
      break;
    case 19:
      val = APPLY_RES_ENERGY;
      break;
    case 20:
      val = APPLY_RES_WATER;
      break;
    }
  }
  return val;
}

/* function to create a random crystal */
void award_random_crystal(struct char_data *ch, int grade)
{
  int color1 = -1, color2 = -1, desc = -1, roll = 0;
  struct obj_data *obj = NULL;
  char buf[MEDIUM_STRING] = {'\0'};
  char buf2[20] = {'\0'};
  int rare_grade = RARE_GRADE_NORMAL;

  if ((obj = read_object(CRYSTAL_PROTOTYPE, VIRTUAL)) == NULL)
  {
    log("SYSERR:  get_random_crystal read_object returned NULL");
    return;
  }

  /* determine if rare or not, start building string */
  rare_grade = determine_rare_grade();
  snprintf(buf2, 20, label_rare_grade(rare_grade));

  /* this is just to make sure the item is set correctly */
  GET_OBJ_TYPE(obj) = ITEM_CRYSTAL;
  GET_OBJ_MATERIAL(obj) = MATERIAL_CRYSTAL;

  /* random color(s) and description */
  color1 = rand_number(0, NUM_A_COLORS - 1);
  color2 = rand_number(0, NUM_A_COLORS - 1);
  while (color2 == color1)
    color2 = rand_number(0, NUM_A_COLORS - 1);
  desc = rand_number(0, NUM_A_CRYSTAL_DESCS - 1);

  roll = dice(1, 100);

  // two colors and descriptor
  if (roll >= 91)
  {
    snprintf(buf, MEDIUM_STRING, "%sarcanite crystal %s %s %s", buf2, colors[color1], colors[color2],
             crystal_descs[desc]);
    obj->name = strdup(buf);
    snprintf(buf, MEDIUM_STRING, "%sa %s, %s and %s arcanite crystal", buf2, crystal_descs[desc],
             colors[color1], colors[color2]);
    obj->short_description = strdup(buf);
    snprintf(buf, MEDIUM_STRING, "%sA %s, %s and %s arcanite crystal lies here.", buf2, crystal_descs[desc],
             colors[color1], colors[color2]);
    obj->description = strdup(buf);

    // one color and descriptor
  }
  else if (roll >= 66)
  {
    snprintf(buf, MEDIUM_STRING, "%sarcanite crystal %s %s", buf2, colors[color1], crystal_descs[desc]);
    obj->name = strdup(buf);
    snprintf(buf, MEDIUM_STRING, "%sa %s %s arcanite crystal", buf2, crystal_descs[desc], colors[color1]);
    obj->short_description = strdup(buf);
    snprintf(buf, MEDIUM_STRING, "%sA %s %s arcanite crystal lies here.", buf2, crystal_descs[desc],
             colors[color1]);
    obj->description = strdup(buf);

    // two colors no descriptor
  }
  else if (roll >= 41)
  {
    snprintf(buf, MEDIUM_STRING, "%sarcanite crystal %s %s", buf2, colors[color1], colors[color2]);
    obj->name = strdup(buf);
    snprintf(buf, MEDIUM_STRING, "%sa %s and %s arcanite crystal", buf2, colors[color1], colors[color2]);
    obj->short_description = strdup(buf);
    snprintf(buf, MEDIUM_STRING, "%sA %s and %s arcanite crystal lies here.", buf2, colors[color1], colors[color2]);
    obj->description = strdup(buf);
  }
  else if (roll >= 21)
  { // one color no descriptor
    snprintf(buf, MEDIUM_STRING, "%sarcanite crystal %s", buf2, colors[color1]);
    obj->name = strdup(buf);
    snprintf(buf, MEDIUM_STRING, "%sa %s arcanite crystal", buf2, colors[color1]);
    obj->short_description = strdup(buf);
    snprintf(buf, MEDIUM_STRING, "%sA %s arcanite crystal lies here.", buf2, colors[color1]);
    obj->description = strdup(buf);

    // descriptor only
  }
  else
  {
    snprintf(buf, MEDIUM_STRING, "%sarcanite crystal %s", buf2, crystal_descs[desc]);
    obj->name = strdup(buf);
    snprintf(buf, MEDIUM_STRING, "%sa %s arcanite crystal", buf2, crystal_descs[desc]);
    obj->short_description = strdup(buf);
    snprintf(buf, MEDIUM_STRING, "%sA %s arcanite crystal lies here.", buf2, crystal_descs[desc]);
    obj->description = strdup(buf);
  }

  cp_modify_object_applies(ch, obj, cp_convert_grade_enchantment(grade), CP_TYPE_CRYSTAL, rare_grade, FALSE);
}

/* awards potions or scroll or wand or staff */

/* reminder, stock:
   obj value 0:  spell level
   obj value 1:  potion/scroll - spell #1; staff/wand - max charges
   obj value 2:  potion/scroll - spell #2; staff/wand - charges remaining
   obj value 3:  potion/scroll - spell #3; staff/wand - spell #1 */
void award_expendable_item(struct char_data *ch, int grade, int type)
{
  int class = CLASS_UNDEFINED, spell_level = 1, spell_num = SPELL_RESERVED_DBC;
  int color1 = 0, color2 = 0, desc = 0, roll = dice(1, 100), i = 0;
  struct obj_data *obj = NULL;
  char keywords[MEDIUM_STRING] = {'\0'}, buf[MAX_STRING_LENGTH] = {'\0'};

  /* first determine which class the scroll will be,
   then determine what level spell the scroll will be */
  switch (rand_number(0, NUM_CLASSES - 1))
  {
  case CLASS_CLERIC:
    class = CLASS_CLERIC;
    break;
  case CLASS_DRUID:
    class = CLASS_DRUID;
    break;
  case CLASS_SORCERER:
    class = CLASS_SORCERER;
    break;
  case CLASS_PALADIN:
    class = CLASS_PALADIN;
    break;
  case CLASS_RANGER:
    class = CLASS_RANGER;
    break;
  case CLASS_BARD:
    class = CLASS_BARD;
    break;
  default: /* we favor wizards heavily since they NEED scrolls */
    class = CLASS_WIZARD;
    break;
  }

  /* determine level */
  /* -note- that this is caster level, not spell-circle */
  switch (grade)
  {
  case GRADE_MUNDANE:
    spell_level = rand_number(1, 5);
    break;
  case GRADE_MINOR:
    spell_level = rand_number(2, 10);
    break;
  case GRADE_TYPICAL:
    spell_level = rand_number(3, 15);
    break;
  case GRADE_MEDIUM:
    spell_level = rand_number(5, 20);
    break;
  case GRADE_MAJOR:
    spell_level = rand_number(7, 25);
    break;
  default:
    spell_level = rand_number(10, 30);
    break;
  }

  /* Loop through spell list randomly! conditions of exit:
     - invalid spell (sub-function)
     - does not match class
     - does not match level
   */
  int loop_counter = 0; // just in case
  do
  {
    spell_num = rand_number(1, NUM_SPELLS - 1);
    loop_counter++;

    if (loop_counter >= 999)
      return;
  } while (spell_level < spell_info[spell_num].min_level[class] ||
           !valid_item_spell(spell_num));

  /* first assign two random colors for usage */
  color1 = rand_number(0, NUM_A_COLORS - 1);
  color2 = rand_number(0, NUM_A_COLORS - 1);
  /* make sure they are not the same colors */
  while (color2 == color1)
    color2 = rand_number(0, NUM_A_COLORS - 1);

  /* load prototype */

  if ((obj = read_object(ITEM_PROTOTYPE, VIRTUAL)) == NULL)
  {
    log("SYSERR:  award_expendable_item returned NULL");
    return;
  }

  switch (type)
  {
  case TYPE_POTION:
    /* assign a description (potions only) */
    desc = rand_number(0, NUM_A_POTION_DESCS - 1);

    // two colors and descriptor
    if (roll >= 91)
    {
      snprintf(keywords, MEDIUM_STRING, "potion-%s", spell_info[spell_num].name);
      for (i = 0; i < strlen(keywords); i++)
        if (keywords[i] == ' ')
          keywords[i] = '-';
      snprintf(buf, MAX_STRING_LENGTH, "vial potion %s %s %s %s %s", colors[color1], colors[color2],
               potion_descs[desc], spell_info[spell_num].name, keywords);
      obj->name = strdup(buf);
      snprintf(buf, MAX_STRING_LENGTH, "a glass vial filled with a %s, %s and %s liquid",
               potion_descs[desc], colors[color1], colors[color2]);
      obj->short_description = strdup(buf);
      snprintf(buf, MAX_STRING_LENGTH, "A glass vial filled with a %s, %s and %s liquid lies here.",
               potion_descs[desc], colors[color1], colors[color2]);
      obj->description = strdup(buf);

      // one color and descriptor
    }
    else if (roll >= 66)
    {
      snprintf(keywords, MEDIUM_STRING, "potion-%s", spell_info[spell_num].name);
      for (i = 0; i < strlen(keywords); i++)
        if (keywords[i] == ' ')
          keywords[i] = '-';
      snprintf(buf, MAX_STRING_LENGTH, "vial potion %s %s %s %s", colors[color1],
               potion_descs[desc], spell_info[spell_num].name, keywords);
      obj->name = strdup(buf);
      snprintf(buf, MAX_STRING_LENGTH, "a glass vial filled with a %s %s liquid",
               potion_descs[desc], colors[color1]);
      obj->short_description = strdup(buf);
      snprintf(buf, MAX_STRING_LENGTH, "A glass vial filled with a %s and %s liquid lies here.",
               potion_descs[desc], colors[color1]);
      obj->description = strdup(buf);

      // two colors no descriptor
    }
    else if (roll >= 41)
    {
      snprintf(keywords, MEDIUM_STRING, "potion-%s", spell_info[spell_num].name);
      for (i = 0; i < strlen(keywords); i++)
        if (keywords[i] == ' ')
          keywords[i] = '-';
      snprintf(buf, MAX_STRING_LENGTH, "vial potion %s %s %s %s", colors[color1], colors[color2],
               spell_info[spell_num].name, keywords);
      obj->name = strdup(buf);
      snprintf(buf, MAX_STRING_LENGTH, "a glass vial filled with a %s and %s liquid", colors[color1],
               colors[color2]);
      obj->short_description = strdup(buf);
      snprintf(buf, MAX_STRING_LENGTH, "A glass vial filled with a %s and %s liquid lies here.",
               colors[color1], colors[color2]);
      obj->description = strdup(buf);

      // one color no descriptor
    }
    else
    {
      snprintf(keywords, MEDIUM_STRING, "potion-%s", spell_info[spell_num].name);
      for (i = 0; i < strlen(keywords); i++)
        if (keywords[i] == ' ')
          keywords[i] = '-';
      snprintf(buf, MAX_STRING_LENGTH, "vial potion %s %s %s", colors[color1],
               spell_info[spell_num].name, keywords);
      obj->name = strdup(buf);
      snprintf(buf, MAX_STRING_LENGTH, "a glass vial filled with a %s liquid",
               colors[color1]);
      obj->short_description = strdup(buf);
      snprintf(buf, MAX_STRING_LENGTH, "A glass vial filled with a %s liquid lies here.",
               colors[color1]);
      obj->description = strdup(buf);
    }

    GET_OBJ_VAL(obj, 0) = spell_level;
    GET_OBJ_VAL(obj, 1) = spell_num;

    GET_OBJ_MATERIAL(obj) = MATERIAL_GLASS;
    GET_OBJ_COST(obj) = MIN(1000, 5 * spell_level);
    GET_OBJ_LEVEL(obj) = dice(1, spell_level);
    break;

  case TYPE_WAND:
    snprintf(keywords, MEDIUM_STRING, "wand-%s", spell_info[spell_num].name);
    for (i = 0; i < strlen(keywords); i++)
      if (keywords[i] == ' ')
        keywords[i] = '-';

    snprintf(buf, MAX_STRING_LENGTH, "wand wooden runes %s %s %s", colors[color1],
             spell_info[spell_num].name, keywords);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "a wooden wand covered in %s runes", colors[color1]);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "A wooden wand covered in %s runes lies here.", colors[color1]);
    obj->description = strdup(buf);

    GET_OBJ_VAL(obj, 0) = spell_level;
    GET_OBJ_VAL(obj, 1) = 10;
    GET_OBJ_VAL(obj, 2) = dice(1, 10);
    GET_OBJ_VAL(obj, 3) = spell_num;

    GET_OBJ_SIZE(obj) = SIZE_SMALL;
    GET_OBJ_COST(obj) = MIN(1000, 50 * spell_level);
    GET_OBJ_MATERIAL(obj) = MATERIAL_WOOD;
    GET_OBJ_TYPE(obj) = ITEM_WAND;
    SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HOLD);
    GET_OBJ_LEVEL(obj) = dice(1, spell_level);
    break;

  case TYPE_STAFF:
    snprintf(keywords, MEDIUM_STRING, "staff-%s", spell_info[spell_num].name);
    for (i = 0; i < strlen(keywords); i++)
      if (keywords[i] == ' ')
        keywords[i] = '-';

    snprintf(buf, MAX_STRING_LENGTH, "staff wooden runes %s %s %s", colors[color1],
             spell_info[spell_num].name, keywords);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "a wooden staff covered in %s runes", colors[color1]);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "A wooden staff covered in %s runes lies here.",
             colors[color1]);
    obj->description = strdup(buf);

    GET_OBJ_VAL(obj, 0) = spell_level;
    GET_OBJ_VAL(obj, 1) = 8;
    GET_OBJ_VAL(obj, 2) = dice(1, 8);
    GET_OBJ_VAL(obj, 3) = spell_num;

    GET_OBJ_COST(obj) = MIN(1000, 110 * spell_level);
    GET_OBJ_MATERIAL(obj) = MATERIAL_WOOD;
    GET_OBJ_TYPE(obj) = ITEM_STAFF;
    SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HOLD);
    GET_OBJ_LEVEL(obj) = dice(1, spell_level);
    break;

  default: // Type-Scrolls
    snprintf(keywords, MEDIUM_STRING, "scroll-%s", spell_info[spell_num].name);
    for (i = 0; i < strlen(keywords); i++)
      if (keywords[i] == ' ')
        keywords[i] = '-';

    snprintf(buf, MAX_STRING_LENGTH, "scroll ink %s %s %s", colors[color1],
             spell_info[spell_num].name, keywords);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "a scroll written in %s ink", colors[color1]);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "A scroll written in %s ink lies here.", colors[color1]);
    obj->description = strdup(buf);

    GET_OBJ_VAL(obj, 0) = spell_level;
    GET_OBJ_VAL(obj, 1) = spell_num;

    GET_OBJ_COST(obj) = MIN(1000, 10 * spell_level);
    GET_OBJ_MATERIAL(obj) = MATERIAL_PAPER;
    GET_OBJ_TYPE(obj) = ITEM_SCROLL;
    GET_OBJ_LEVEL(obj) = dice(1, spell_level);
    break;
  }

  REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MOLD);
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);

  obj_to_char(obj, ch);

  /* inform ch and surrounding that they received this item */
  say_treasure(ch, obj);
}

/* this is a very simplified version of this function, the original version was
 incomplete and creating some very strange gear with some crazy stats.  The
 original version is right below this.  the bonus_value is measure in enchantment
 bonus as the currency, so we have to make decisions on the value of receiving
 other stats such as strength, hps, etc */
void cp_modify_object_applies(struct char_data *ch, struct obj_data *obj,
                              int enchantment_grade, int cp_type, int rare_grade, int silent_mode)
{
  int bonus_value = enchantment_grade, bonus_location = APPLY_NONE;
  bool has_enhancement = FALSE;
  int feat_num = FEAT_UNDEFINED;

  /* items that will only get an enhancement bonus */
  if (CAN_WEAR(obj, ITEM_WEAR_WIELD) || CAN_WEAR(obj, ITEM_WEAR_SHIELD) ||
      CAN_WEAR(obj, ITEM_WEAR_HEAD) || CAN_WEAR(obj, ITEM_WEAR_BODY) ||
      CAN_WEAR(obj, ITEM_WEAR_LEGS) || CAN_WEAR(obj, ITEM_WEAR_ARMS) ||
      cp_type == CP_TYPE_AMMO)
  {
    /* object value 4 for these particular objects are their enchantment bonus */
    GET_OBJ_VAL(obj, 4) = bonus_value;
    has_enhancement = TRUE;
  }
  else if (cp_type == CP_TYPE_CRYSTAL)
  {
    bonus_location = random_apply_value();
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_FINGER))
  {
    bonus_location = determine_stat_apply(WEAR_FINGER_R);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_NECK))
  {
    bonus_location = determine_stat_apply(WEAR_NECK_1);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_FEET))
  {
    bonus_location = determine_stat_apply(WEAR_FEET);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_HANDS))
  {
    bonus_location = determine_stat_apply(WEAR_HANDS);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))
  {
    bonus_location = determine_stat_apply(WEAR_ABOUT);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_WAIST))
  {
    bonus_location = determine_stat_apply(WEAR_WAIST);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_WRIST))
  {
    bonus_location = determine_stat_apply(WEAR_WRIST_R);
  }
  else if (CAN_WEAR(obj, ITEM_WEAR_HOLD))
  {
    bonus_location = determine_stat_apply(WEAR_HOLD_1);
  }

  if (!has_enhancement)
  {
    obj->affected[0].location = bonus_location;
    obj->affected[0].modifier = adjust_bonus_value(bonus_location, bonus_value);
    obj->affected[0].bonus_type = adjust_bonus_type(bonus_location);
  }

  /* rare grade */
  if (rare_grade > RARE_GRADE_NORMAL)
  {
    feat_num = apply_bonus_feat(rare_grade);
    //send_to_char(ch, "debug: %d\r\n", feat_num);
    if (feat_num != FEAT_UNDEFINED)
    {
      obj->affected[1].location = APPLY_FEAT;
      obj->affected[1].modifier = feat_num;
      obj->affected[1].bonus_type = adjust_bonus_type(APPLY_FEAT);
    }

    switch (rare_grade)
    {
    case RARE_GRADE_LEGENDARY:
      if (has_enhancement)
        bonus_location = random_apply_value();
      obj->affected[2].location = bonus_location;
      obj->affected[2].modifier = adjust_bonus_value(bonus_location, 1);
      obj->affected[2].bonus_type = BONUS_TYPE_INHERENT;
      break;
    case RARE_GRADE_MYTHICAL:
      if (has_enhancement)
        bonus_location = random_apply_value();
      obj->affected[2].location = bonus_location;
      obj->affected[2].modifier = adjust_bonus_value(bonus_location, dice(1, 2));
      obj->affected[2].bonus_type = BONUS_TYPE_INHERENT;
      break;
    }
  }

  /* lets modify this ammo's breakability (base 30%) */
  if (cp_type == CP_TYPE_AMMO)
    GET_OBJ_VAL(obj, 2) -= MIN(29, (dice(5, bonus_value) + bonus_value));

  /* object level (min level to use) */
  GET_OBJ_LEVEL(obj) = bonus_value * 5;

  if (cp_type == CP_TYPE_AMMO)
    ;
  else // set value
    GET_OBJ_COST(obj) = (1 + GET_OBJ_LEVEL(obj)) * 100 + (bonus_value * 5);

  REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MOLD); // make sure not mold

  if (bonus_value >= 1)
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC); // add magic tag

  obj_to_char(obj, ch); // deliver object

  /* inform ch and surrounding that they received this item */
  if (!silent_mode)
    say_treasure(ch, obj);
  else
    say_bazaar(ch, obj);

  /* staff will get a free ID here -zusuk */
  if (GET_LEVEL(ch) >= LVL_IMMORT)
    do_stat_object(ch, obj, ITEM_STAT_MODE_IMMORTAL);
}

/* this is ornir's original version, taken out until he has time to finish it */
/* Here is where the significant changes start (cp = Creation Points) - Ornir */
/*void cp_modify_object_applies(struct char_data *ch, struct obj_data *obj,
        int rare_grade, int level, int cp_type) {
  int max_slots = 0;
  int current_slot = 1;
  int current_cp = 0;
  int max_bonus = 0;
  int max_bonus_cp_cost = 0;
  int bonus_location = 0;
  int bonus_value = 0;
  int i = 0;
  bool duplicate_affect = FALSE;
  char buf[MAX_STRING_LENGTH] = {'\0'};

  switch (cp_type) {
    case CP_TYPE_MISC:
      max_slots = 1; // Trinkets
      break;
    case CP_TYPE_ARMOR:
      max_slots = 2; // AC_APPLY takes 1!
      break;
    case CP_TYPE_WEAPON:
      max_slots = 3; // TOHIT/TODAM takes 2!
      break;
    default:
      break;
  }

  // Get the base CP for the item based on the level.
  current_cp = CP_BASE_VALUE(level);

  // Add bonus CP and slots for rarity
  current_cp += rare_grade * 100;
  if (rare_grade >= 2)
    max_slots += 1;

  // DEBUG
  if (GET_LEVEL(ch) >= LVL_IMMORTAL)
    send_to_char(ch, "\tyItem created, level: %d CP: %d\tn\r\n", level, current_cp);

  // Add bonuses, one bonus to each slot.
  while (current_slot <= max_slots) {

    // Determine bonus location, check if first bonus too
    switch (cp_type) {
      case CP_TYPE_ARMOR:
        // Since this is armor, the first bonus is ALWAYS AC
        // Note: this is just a marker now, we have enhancement bonus instead
        if(current_slot == 1)
          bonus_location = APPLY_AC_NEW;
        else
          bonus_location = random_armor_apply_value();
        break;

      case CP_TYPE_WEAPON:
        // Since this is a weapon, the first 2 bonuses are TOHIT and TODAM
        // Note: this is just a marker now, we have enhancement bonus instead
        if(current_slot == 1)
          bonus_location = APPLY_HITROLL; // We Apply TODAM later...
        else
          bonus_location = random_weapon_apply_value();
        break;
      default: // misc types
        bonus_location = random_apply_value();
        break;
    }

    // Check for duplicate affects
    duplicate_affect = FALSE;
    for(i = 0; i > MAX_OBJ_AFFECT; i++){
      if(obj->affected[i].location == bonus_location){
        duplicate_affect = TRUE;
        break;
      }
    }
    // Only add the affect if it is not a duplicate affect location!
    if(duplicate_affect != TRUE) {

      // Based on CP remaining, how HIGH a bonus can we get here?
      switch (cp_type) {
        case CP_TYPE_ARMOR:
          max_bonus = TREASURE_MAX_BONUS;
          break;
        default:
          max_bonus = TREASURE_MAX_BONUS + 5;
          break;
      }
      max_bonus_cp_cost = CP_COST(max_bonus);

      while ((max_bonus > 0) && (max_bonus_cp_cost > current_cp)) {
        max_bonus--;
        max_bonus_cp_cost = CP_COST(max_bonus);
      }

      // If we CAN apply a bonus, based on CP, then determine value.
      if (max_bonus > 0) {
        // Choose a bonus value from 1 to that bonus amount.
        bonus_value = rand_number(1, max_bonus);
        current_cp -= CP_COST(bonus_value);
        if (cp_type == CP_TYPE_WEAPON && bonus_location == APPLY_HITROLL) {
          GET_OBJ_VAL(obj, 4) = adjust_bonus_value(APPLY_DAMROLL, bonus_value); // Set enhancement bonus
          current_slot++;
          // added this code to handle armor enhancement -zusuk
        } else if (cp_type == CP_TYPE_ARMOR && bonus_location == APPLY_AC_NEW) {
          // it doesn't matter we're sending APPLY_DAMROLL here
          GET_OBJ_VAL(obj, 4) = adjust_bonus_value(APPLY_DAMROLL, bonus_value); // Set enhancement bonus.
          //current_slot++;
        } else {
          obj->affected[current_slot - 1].location = bonus_location;
          obj->affected[current_slot - 1].modifier = adjust_bonus_value(bonus_location, bonus_value);
          obj->affected[current_slot - 1].bonus_type = adjust_bonus_type(bonus_location);
        }

      }
    }
    current_slot++;
  } // end while

  GET_OBJ_LEVEL(obj) = level;
  GET_OBJ_COST(obj) = (1+GET_OBJ_LEVEL(obj)) * 100;  // set value
  REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MOLD);  // make sure not mold
  if (level >= 5)
    SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);  // add magic tag

  obj_to_char(obj, ch); // deliver object

  //inform ch and surrounding that they received this item
  say_treasure(ch, obj);
}
 */

/* Give away random magic ammo */
void award_magic_ammo(struct char_data *ch, int grade)
{
  struct obj_data *obj = NULL;
  int armor_desc_rollA = 0;
  int armor_desc_rollB = 0;
  int rare_grade = RARE_GRADE_NORMAL;
  int level = 0;
  char desc[MEDIUM_STRING] = {'\0'};
  char desc1[MEDIUM_STRING] = {'\0'};
  char desc2[MEDIUM_STRING] = {'\0'};
  char desc3[MEDIUM_STRING] = {'\0'};
  char desc4[MEDIUM_STRING] = {'\0'};
  char desc5[MEDIUM_STRING] = {'\0'};
  char keywords[MEDIUM_STRING] = {'\0'};

  /* ok load blank object */
  if ((obj = read_object(AMMO_PROTO, VIRTUAL)) == NULL)
  {
    log("SYSERR: award_magic_ammo created NULL object");
    return;
  }

  /* determine if rare or not, start building string */
  rare_grade = determine_rare_grade();
  snprintf(desc1, MEDIUM_STRING, "%s", label_rare_grade(rare_grade));

  /* pick a random ammo, 0 = undefined */
  armor_desc_rollA = rand_number(1, NUM_AMMO_TYPES - 1);

  /* now set up this new object */
  set_ammo_object(obj, armor_desc_rollA);
  /* we should have a completely usable ammo now, just missing descrip/stats */

  /* set the object material, check for upgrade */
  GET_OBJ_MATERIAL(obj) = possible_material_upgrade(GET_OBJ_MATERIAL(obj), grade);

  /* determine level */
  switch (grade)
  {
  case GRADE_MUNDANE:
    level = rand_number(1, 5);
    break;
  case GRADE_MINOR:
    level = rand_number(2, 10);
    break;
  case GRADE_TYPICAL:
    level = rand_number(3, 15);
    break;
  case GRADE_MEDIUM:
    level = rand_number(5, 20);
    break;
  case GRADE_MAJOR:
    level = rand_number(8, 25);
    break;
  default: // superior grade
    level = rand_number(12, 30);
    break;
  }

  /* BEGIN DESCRIPTION SECTION */

  /* arrow, bolt, dart
   * a|an [ammo_descs, ex. dwarven-made] [material] [arrow|bolt|dart]
   *   with a [piercing_descs, ex. grooved] tip
   * sling bullets
   * a/an [ammo_descs] [material] sling-bullet
   */
  armor_desc_rollA = rand_number(0, NUM_A_AMMO_DESCS - 1);

  /* a dwarven-made */
  snprintf(desc2, MEDIUM_STRING, "%s %s", AN(ammo_descs[armor_desc_rollA]),
           ammo_descs[armor_desc_rollA]);

  /* mithril sling-bullet */
  snprintf(desc3, MEDIUM_STRING, "%s %s", material_name[GET_OBJ_MATERIAL(obj)],
           ammo_types[GET_OBJ_VAL(obj, 0)]);

  /* sling bullets done! */
  if (GET_OBJ_VAL(obj, 0) == AMMO_TYPE_STONE)
    ;
  /* with a grooved tip (arrows, bolts, darts) */
  else
  {
    armor_desc_rollB = rand_number(1, NUM_A_AMMO_HEAD_DESCS - 1);
    snprintf(desc4, MEDIUM_STRING, "with %s %s tip", AN(ammo_head_descs[armor_desc_rollB]), ammo_head_descs[armor_desc_rollB]);
  }

  /* we are adding "ammo" as a keyword here for ease of handling for players */
  snprintf(keywords, MEDIUM_STRING, "ammo %s %s %s %s", ammo_head_descs[armor_desc_rollB], material_name[GET_OBJ_MATERIAL(obj)], ammo_types[GET_OBJ_VAL(obj, 0)], ammo_descs[armor_desc_rollA]);
  snprintf(desc, MEDIUM_STRING, "%s %s %s %s", desc1, desc2, desc3, desc4);

  /* finished descrips, so lets assign them */
  obj->name = strdup(keywords); /* key words */
  // Set descriptions
  obj->short_description = strdup(desc);
  desc[0] = toupper(desc[0]);
  snprintf(desc5, MEDIUM_STRING, "%s is lying here.", desc);
  desc5[0] = toupper(desc5[0]);
  obj->description = strdup(desc5);

  /* END DESCRIPTION SECTION */

  /* BONUS SECTION */
  SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_MAGIC);
  cp_modify_object_applies(ch, obj, cp_convert_grade_enchantment(grade), CP_TYPE_AMMO, rare_grade, FALSE);
  /* END BONUS SECTION */
}

/* Give away random magic armor
 * (includes:  body/head/legs/arms/shield)
 * 1)  determine material
 * 2)  determine grade
 * 3)  determine Creation Points
 * 4)  determine AC bonus (Always first stat...)
 * 5)  craft description based on object and bonuses */
void give_magic_armor(struct char_data *ch, int selection, int enchantment, bool silent_mode)
{
  struct obj_data *obj = NULL;
  int roll = 0, armor_desc_roll = 0, crest_num = 0;
  int color1 = 0, color2 = 0, level = 0;
  char desc[MEDIUM_STRING] = {'\0'};
  char keywords[MEDIUM_STRING] = {'\0'};

  /* ok load blank object */
  if ((obj = read_object(ARMOR_PROTO, VIRTUAL)) == NULL)
  {
    log("SYSERR: give_magic_armor created NULL object");
    return;
  }

  /* now set up this new object */
  set_armor_object(obj, selection);
  /* we should have a completely usable armor now, just missing descrip/stats */

  /* a suit of (body), or a pair of (arm/leg), or AN() (helm) */
  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_BODY))
  {
    snprintf(desc, MEDIUM_STRING, "%s%s", desc, "a suit of");
  }
  else if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HEAD) ||
           IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_SHIELD))
  {
    armor_desc_roll = rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS - 1);
    snprintf(desc, MEDIUM_STRING, "%s%s", desc,
             AN(armor_special_descs[armor_desc_roll]));
  }
  else
  {
    snprintf(desc, MEDIUM_STRING, "%s%s", desc, "a pair of");
  }

  /* set the object material, check for upgrade */
  GET_OBJ_MATERIAL(obj) = possible_material_upgrade(GET_OBJ_MATERIAL(obj), enchantment);

  /* determine level */
  switch (enchantment)
  {
  case 0:
  case 1:
    level = 0;
    break;
  case 2:
    level = 5;
    break;
  case 3:
    level = 10;
    break;
  case 4:
    level = 15;
    break;
  case 5:
    level = 20;
    break;
  default: /*6*/
    level = 25;
    break;
  }

  /* BEGIN DESCRIPTION SECTION */

  /* first assign two random colors for usage */
  color1 = rand_number(0, NUM_A_COLORS - 1);
  color2 = rand_number(0, NUM_A_COLORS - 1);
  /* make sure they are not the same colors */
  while (color2 == color1)
    color2 = rand_number(0, NUM_A_COLORS - 1);
  crest_num = rand_number(0, NUM_A_ARMOR_CRESTS - 1);

  /* start with keyword string */
  snprintf(keywords, MEDIUM_STRING, "%s %s", keywords, armor_list[GET_ARMOR_TYPE(obj)].name);
  snprintf(keywords, MEDIUM_STRING, "%s %s", keywords, material_name[GET_OBJ_MATERIAL(obj)]);

  roll = dice(1, 3);
  if (roll == 3)
  { // armor spec adjective in desc?
    snprintf(desc, MEDIUM_STRING, "%s %s", desc,
             armor_special_descs[armor_desc_roll]);
    snprintf(keywords, MEDIUM_STRING, "%s %s", keywords,
             armor_special_descs[armor_desc_roll]);
  }

  roll = dice(1, 5);
  if (roll >= 4)
  { // color describe #1?
    snprintf(desc, MEDIUM_STRING, "%s %s", desc, colors[color1]);
    snprintf(keywords, MEDIUM_STRING, "%s %s", keywords, colors[color1]);
  }
  else if (roll == 3)
  { // two colors
    snprintf(desc, MEDIUM_STRING, "%s %s and %s", desc, colors[color1], colors[color2]);
    snprintf(keywords, MEDIUM_STRING, "%s %s and %s", keywords, colors[color1], colors[color2]);
  }

  // Insert the material type, then armor type
  snprintf(desc, MEDIUM_STRING, "%s %s", desc, material_name[GET_OBJ_MATERIAL(obj)]);
  snprintf(desc, MEDIUM_STRING, "%s %s", desc, armor_list[GET_ARMOR_TYPE(obj)].name);

  roll = dice(1, 8);
  if (roll >= 7)
  { // crest?
    snprintf(desc, MEDIUM_STRING, "%s with %s %s crest", desc,
             AN(armor_crests[crest_num]),
             armor_crests[crest_num]);
    snprintf(keywords, MEDIUM_STRING, "%s with %s %s crest", keywords,
             AN(armor_crests[crest_num]),
             armor_crests[crest_num]);
  }
  else if (roll >= 5)
  { // or symbol?
    snprintf(desc, MEDIUM_STRING, "%s covered in symbols of %s %s", desc,
             AN(armor_crests[crest_num]),
             armor_crests[crest_num]);
    snprintf(keywords, MEDIUM_STRING, "%s covered in symbols of %s %s", keywords,
             AN(armor_crests[crest_num]),
             armor_crests[crest_num]);
  }

  // keywords
  obj->name = strdup(keywords);
  // Set descriptions
  obj->short_description = strdup(desc);
  desc[0] = toupper(desc[0]);
  snprintf(desc, MEDIUM_STRING, "%s is lying here.", desc);
  obj->description = strdup(desc);

  /* END DESCRIPTION SECTION */

  /* BONUS SECTION */
  cp_modify_object_applies(ch, obj, enchantment, CP_TYPE_ARMOR, RARE_GRADE_NORMAL, silent_mode);
  /* END BONUS SECTION */
}

/* Give away random magic armor
 * (includes:  body/head/legs/arms/shield)
 * 1)  determine armor type
 * 2)  determine material
 * 3)  determine rarity
 * 3)  determine Creation Points
 * 4)  determine AC bonus (Always first stat...)
 * 5)  craft description based on object and bonuses */
void award_magic_armor(struct char_data *ch, int grade, int wear_slot)
{
  struct obj_data *obj = NULL;
  int roll = 0, armor_desc_roll = 0, crest_num = 0;
  int rare_grade = 0, color1 = 0, color2 = 0, level = 0;
  char desc[MEDIUM_STRING] = {'\0'};
  char keywords[MEDIUM_STRING] = {'\0'};

  /* ok load blank object */
  if ((obj = read_object(ARMOR_PROTO, VIRTUAL)) == NULL)
  {
    log("SYSERR: award_magic_armor created NULL object");
    return;
  }

  /* pick a random armor, 0 = undefined */
  do
  {
    roll = rand_number(1, NUM_SPEC_ARMOR_TYPES - 1);
  } while (armor_list[roll].wear != wear_slot && wear_slot != -1);

  /* now set up this new object */
  set_armor_object(obj, roll);
  /* we should have a completely usable armor now, just missing descrip/stats */

  /* determine if rare or not, start building string */
  rare_grade = determine_rare_grade();
  snprintf(desc, MEDIUM_STRING, label_rare_grade(rare_grade));

  /* a suit of (body), or a pair of (arm/leg), or AN() (helm) */
  if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_BODY))
  {
    strncat(desc, "a suit of", MEDIUM_STRING - strlen(desc));
  }
  else if (IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_HEAD) ||
           IS_SET_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_SHIELD))
  {
    armor_desc_roll = rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS - 1);
    strncat(desc, AN(armor_special_descs[armor_desc_roll]), MEDIUM_STRING - strlen(desc));
  }
  else
  {
    strncat(desc, "a pair of", MEDIUM_STRING - strlen(desc));
  }

  /* set the object material, check for upgrade */
  GET_OBJ_MATERIAL(obj) = possible_material_upgrade(GET_OBJ_MATERIAL(obj), grade);

  /* determine level */
  switch (grade)
  {
  case GRADE_MUNDANE:
    level = rand_number(1, 5);
    break;
  case GRADE_MINOR:
    level = rand_number(2, 10);
    break;
  case GRADE_TYPICAL:
    level = rand_number(3, 15);
    break;
  case GRADE_MEDIUM:
    level = rand_number(5, 20);
    break;
  case GRADE_MAJOR:
    level = rand_number(8, 25);
    break;
  default: // superior grade
    level = rand_number(12, 30);
    break;
  }

  /* BEGIN DESCRIPTION SECTION */

  /* first assign two random colors for usage */
  color1 = rand_number(0, NUM_A_COLORS - 1);
  color2 = rand_number(0, NUM_A_COLORS - 1);
  /* make sure they are not the same colors */
  while (color2 == color1)
    color2 = rand_number(0, NUM_A_COLORS - 1);
  crest_num = rand_number(0, NUM_A_ARMOR_CRESTS - 1);

  /* start with keyword string */
  strncat(keywords, " ", MEDIUM_STRING - strlen(keywords));
  strncat(keywords, armor_list[GET_ARMOR_TYPE(obj)].name, MEDIUM_STRING - strlen(keywords));
  strncat(keywords, " ", MEDIUM_STRING - strlen(keywords));
  strncat(keywords, material_name[GET_OBJ_MATERIAL(obj)], MEDIUM_STRING - strlen(keywords));

  roll = dice(1, 3);
  if (roll == 3)
  { // armor spec adjective in desc?
    strncat(desc, " ", MEDIUM_STRING - strlen(desc));
    strncat(desc, armor_special_descs[armor_desc_roll], MEDIUM_STRING - strlen(desc));
    strncat(keywords, " ", MEDIUM_STRING - strlen(keywords));
    strncat(keywords, armor_special_descs[armor_desc_roll], MEDIUM_STRING - strlen(keywords));
  }

  roll = dice(1, 5);
  if (roll >= 4)
  { // color describe #1?
    strncat(desc, " ", MEDIUM_STRING - strlen(desc));
    strncat(desc, colors[color1], MEDIUM_STRING - strlen(desc));
    strncat(keywords, " ", MEDIUM_STRING - strlen(keywords));
    strncat(keywords, colors[color1], MEDIUM_STRING - strlen(keywords));
  }
  else if (roll == 3)
  { // two colors
    strncat(desc, " ", MEDIUM_STRING - strlen(desc));
    strncat(desc, colors[color1], MEDIUM_STRING - strlen(desc));
    strncat(desc, " and ", MEDIUM_STRING - strlen(desc));
    strncat(desc, colors[color2], MEDIUM_STRING - strlen(desc));
    strncat(keywords, " ", MEDIUM_STRING - strlen(keywords));
    strncat(keywords, colors[color1], MEDIUM_STRING - strlen(keywords));
    strncat(keywords, " and ", MEDIUM_STRING - strlen(keywords));
    strncat(keywords, colors[color2], MEDIUM_STRING - strlen(keywords));
  }

  // Insert the material type, then armor type
  if (GET_OBJ_MATERIAL(obj) = !MATERIAL_LEATHER) // leather is redudant in description here
  {
    strncat(desc, " ", MEDIUM_STRING - strlen(desc));
    strncat(desc, material_name[GET_OBJ_MATERIAL(obj)], MEDIUM_STRING - strlen(desc));
  }
  strncat(desc, " ", MEDIUM_STRING - strlen(desc));
  strncat(desc, armor_list[GET_ARMOR_TYPE(obj)].name, MEDIUM_STRING - strlen(desc));

  roll = dice(1, 8);
  if (roll >= 7)
  { // crest?
    char tmp[SMALL_STRING];
    snprintf(tmp, SMALL_STRING, " with %s %s crest",
             AN(armor_crests[crest_num]),
             armor_crests[crest_num]);
    strncat(desc, tmp, MEDIUM_STRING - strlen(desc));
    strncat(keywords, tmp, MEDIUM_STRING - strlen(keywords));
  }
  else if (roll >= 5)
  { // or symbol?
    char tmp[SMALL_STRING];
    snprintf(tmp, SMALL_STRING, " covered in symbols of %s %s",
             AN(armor_crests[crest_num]),
             armor_crests[crest_num]);
    strncat(desc, tmp, MEDIUM_STRING - strlen(desc));
    strncat(keywords, tmp, MEDIUM_STRING - strlen(keywords));
  }

  // keywords
  obj->name = strdup(keywords);
  // Set descriptions
  obj->short_description = strdup(desc);
  desc[0] = toupper(desc[0]);
  strncat(desc, " is lying here.", MEDIUM_STRING);
  obj->description = strdup(desc);

  /* END DESCRIPTION SECTION */

  /* BONUS SECTION */
  cp_modify_object_applies(ch, obj, cp_convert_grade_enchantment(grade), CP_TYPE_ARMOR, rare_grade, FALSE);
  /* END BONUS SECTION */
}

/* automatically set object up to be a given ammo type, also setting breakability
   ammo object values:
 * 0 : ammo type
   2 : break probability
 */
void set_ammo_object(struct obj_data *obj, int type)
{

  GET_OBJ_TYPE(obj) = ITEM_MISSILE;

  /* Ammo Type, 0 Value */
  GET_OBJ_VAL(obj, 0) = type;

  /* Base breakability 30% */
  GET_OBJ_VAL(obj, 2) = 30;

  /* for convenience we are going to go ahead and set some other values */
  GET_OBJ_COST(obj) = 10;
  GET_OBJ_WEIGHT(obj) = 0;

  /* base material */
  switch (type)
  {
  case AMMO_TYPE_STONE:
    GET_OBJ_MATERIAL(obj) = MATERIAL_STONE;
    break;
  default:
    GET_OBJ_MATERIAL(obj) = MATERIAL_WOOD;
    break;
  }
}

/* automatically set object up to be a given armor type 
   armor object values:
 * 0 : armor bonus
   1 : the base armor-type, i.e. plate vambraces
 * everything else is computed via the armor_list[] */
void set_armor_object(struct obj_data *obj, int type)
{
  int wear_inc;

  GET_OBJ_TYPE(obj) = ITEM_ARMOR;

  /* Armor Type, 2nd Value */
  GET_OBJ_VAL(obj, 1) = type;

  /* auto set ac apply, 1st value */
  GET_OBJ_VAL(obj, 0) =
      armor_list[GET_OBJ_VAL(obj, 1)].armorBonus;

  /* for convenience we are going to go ahead and set some other values */
  GET_OBJ_COST(obj) =
      armor_list[GET_OBJ_VAL(obj, 1)].cost + 1;
  GET_OBJ_WEIGHT(obj) =
      armor_list[GET_OBJ_VAL(obj, 1)].weight;
  GET_OBJ_MATERIAL(obj) =
      armor_list[GET_OBJ_VAL(obj, 1)].material;

  /* set the proper wear bits! */

  /* going to go ahead and reset all the bits off */
  for (wear_inc = 0; wear_inc < NUM_ITEM_WEARS; wear_inc++)
  {
    REMOVE_BIT_AR(GET_OBJ_WEAR(obj), wear_inc);
  }

  /* now set take bit */
  TOGGLE_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);

  /* now set the appropriate wear flag bit */
  TOGGLE_BIT_AR(GET_OBJ_WEAR(obj),
                armor_list[GET_OBJ_VAL(obj, 1)].wear);
}

/* automatically set object up to be a given weapon type */

/* weapon object values:
   0 : the base weapon-type, i.e. long sword
   1 : number of damage dice
   2 : size of damage dice
 * everything else is computed via the weapon_list[] */
void set_weapon_object(struct obj_data *obj, int type)
{
  int wear_inc;

  GET_OBJ_TYPE(obj) = ITEM_WEAPON;

  /* Weapon Type */
  GET_OBJ_VAL(obj, 0) = type;

  /* Set damdice  and size based on weapon type. */
  GET_OBJ_VAL(obj, 1) = weapon_list[GET_OBJ_VAL(obj, 0)].numDice;
  GET_OBJ_VAL(obj, 2) = weapon_list[GET_OBJ_VAL(obj, 0)].diceSize;
  /* cost */
  GET_OBJ_COST(obj) = weapon_list[GET_OBJ_VAL(obj, 0)].cost + 1;
  /* weight */
  GET_OBJ_WEIGHT(obj) = weapon_list[GET_OBJ_VAL(obj, 0)].weight;
  /* material */
  GET_OBJ_MATERIAL(obj) = weapon_list[GET_OBJ_VAL(obj, 0)].material;
  /* size */
  GET_OBJ_SIZE(obj) = weapon_list[GET_OBJ_VAL(obj, 0)].size;

  /* set the proper wear bits */

  /* going to go ahead and reset all the bits off */
  for (wear_inc = 0; wear_inc < NUM_ITEM_WEARS; wear_inc++)
  {
    REMOVE_BIT_AR(GET_OBJ_WEAR(obj), wear_inc);
  }

  /* now set take bit */
  TOGGLE_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);

  /* now set the appropriate wear flag bit */
  TOGGLE_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_WIELD);
}

/* given: base material (base_mat), item grade (grade)
   return: possible material upgrade */
int possible_material_upgrade(int base_mat, int grade)
{
  int material = base_mat;
  int roll = dice(1, 100); /* randomness */

  /* sometimes we carry enchantment value into here, which would be
     from 0-6, this will fix that */
  if (grade < GRADE_MUNDANE)
    grade = GRADE_MUNDANE;
  if (grade > GRADE_SUPERIOR)
    grade = GRADE_SUPERIOR;

  switch (material)
  {
  case MATERIAL_BRONZE:
    switch (grade)
    {
    case GRADE_MUNDANE:
      if (roll <= 75)
        material = MATERIAL_BRONZE;
      else
        material = MATERIAL_IRON;
      break;
    case GRADE_MINOR:
    case GRADE_TYPICAL:
      if (roll <= 75)
        material = MATERIAL_IRON;
      else
        material = MATERIAL_STEEL;
      break;
    case GRADE_MEDIUM:
      if (roll <= 50)
        material = MATERIAL_IRON;
      else if (roll <= 80)
        material = MATERIAL_STEEL;
      else if (roll <= 95)
        material = MATERIAL_COLD_IRON;
      else
        material = MATERIAL_ALCHEMAL_SILVER;
      break;
    case GRADE_MAJOR:
      if (roll <= 50)
        material = MATERIAL_COLD_IRON;
      else if (roll <= 80)
        material = MATERIAL_ALCHEMAL_SILVER;
      else
        material = MATERIAL_ADAMANTINE;
      break;
    default: // superior grade
      if (roll <= 45)
        material = MATERIAL_COLD_IRON;
      else if (roll <= 70)
        material = MATERIAL_ALCHEMAL_SILVER;
      else if (roll <= 85)
        material = MATERIAL_MITHRIL;
      else
        material = MATERIAL_ADAMANTINE;
      break;
    }
    break;
  case MATERIAL_LEATHER:
    switch (grade)
    {
    case GRADE_MUNDANE:
      material = MATERIAL_LEATHER;
      break;
    case GRADE_MINOR:
      material = MATERIAL_LEATHER;
      break;
    case GRADE_MEDIUM:
      material = MATERIAL_LEATHER;
      break;
    case GRADE_MAJOR:
      if (roll <= 90)
        material = MATERIAL_LEATHER;
      else
        material = MATERIAL_DRAGONHIDE;
      break;
    default: // superior grade
      if (roll <= 70)
        material = MATERIAL_LEATHER;
      else
        material = MATERIAL_DRAGONHIDE;
      break;
    }
    break;
  case MATERIAL_COTTON:
    switch (grade)
    {
    case GRADE_MUNDANE:
      if (roll <= 75)
        material = MATERIAL_HEMP;
      else
        material = MATERIAL_COTTON;
      break;
    case GRADE_MINOR:
    case GRADE_TYPICAL:
      if (roll <= 50)
        material = MATERIAL_HEMP;
      else if (roll <= 80)
        material = MATERIAL_COTTON;
      else
        material = MATERIAL_WOOL;
      break;
    case GRADE_MEDIUM:
      if (roll <= 50)
        material = MATERIAL_COTTON;
      else if (roll <= 80)
        material = MATERIAL_WOOL;
      else if (roll <= 95)
        material = MATERIAL_VELVET;
      else
        material = MATERIAL_SATIN;
      break;
    case GRADE_MAJOR:
      if (roll <= 50)
        material = MATERIAL_WOOL;
      else if (roll <= 80)
        material = MATERIAL_VELVET;
      else
        material = MATERIAL_SATIN;
      break;
    default: // major grade
      if (roll <= 30)
        material = MATERIAL_WOOL;
      else if (roll <= 60)
        material = MATERIAL_VELVET;
      else if (roll <= 75)
        material = MATERIAL_SATIN;
      else
        material = MATERIAL_SILK;
      break;
    }
    break;
  case MATERIAL_WOOD:
    switch (grade)
    {
    case GRADE_MUNDANE:
    case GRADE_MINOR:
    case GRADE_TYPICAL:
    case GRADE_MEDIUM:
      material = MATERIAL_WOOD;
      break;
    case GRADE_MAJOR:
      if (roll <= 75)
        material = MATERIAL_WOOD;
      else
        material = MATERIAL_DARKWOOD;
      break;
    default: // superior grade
      if (roll <= 60)
        material = MATERIAL_WOOD;
      else
        material = MATERIAL_DARKWOOD;
      break;
    }
    break;
  }

  return material;
}

/* give away random magic weapon, method:
 * 1)  determine weapon
 * 2)  determine material
 * 3)  assign description
 * 4)  determine modifier (if applicable)
 * 5)  determine amount (if applicable)
 */
#define SHORT_STRING 80

void award_magic_weapon(struct char_data *ch, int grade)
{
  struct obj_data *obj = NULL;
  int roll = 0;
  int rare_grade = 0, color1 = 0, color2 = 0, level = 0, roll2 = 0, roll3 = 0;
  char desc[MEDIUM_STRING] = {'\0'};
  char hilt_color[SHORT_STRING] = {'\0'}, head_color[SHORT_STRING] = {'\0'};
  char special[SHORT_STRING] = {'\0'};
  char buf[MAX_STRING_LENGTH] = {'\0'};

  /* ok load blank object */
  if ((obj = read_object(WEAPON_PROTO, VIRTUAL)) == NULL)
  {
    log("SYSERR: award_magic_weapon created NULL object");
    return;
  }

  /* pick a random weapon, 0 = undefined 1 = unarmed */
  roll = rand_number(2, NUM_WEAPON_TYPES - 1);

  /* now set up this new object */
  set_weapon_object(obj, roll);
  /* we should have a completely usable weapon now, just missing descripts/stats */

  /* determine if rare or not, start building string */
  rare_grade = determine_rare_grade();
  snprintf(desc, MEDIUM_STRING, label_rare_grade(rare_grade));

  /* ok assigning final material here, check for upgrade */
  GET_OBJ_MATERIAL(obj) =
      possible_material_upgrade(GET_OBJ_MATERIAL(obj), grade);

  /* determine level */
  switch (grade)
  {
  case GRADE_MUNDANE:
    level = rand_number(1, 5);
    break;
  case GRADE_MINOR:
    level = rand_number(2, 10);
    break;
  case GRADE_TYPICAL:
    level = rand_number(3, 15);
    break;
  case GRADE_MEDIUM:
    level = rand_number(5, 20);
    break;
  case GRADE_MAJOR:
    level = rand_number(8, 25);
    break;
  default: // superior grade
    level = rand_number(12, 30);
    break;
  }

  // pick a pair of random colors for usage
  /* first assign two random colors for usage */
  color1 = rand_number(0, NUM_A_COLORS - 1);
  color2 = rand_number(0, NUM_A_COLORS - 1);
  /* make sure they are not the same colors */
  while (color2 == color1)
    color2 = rand_number(0, NUM_A_COLORS - 1);

  snprintf(head_color, SHORT_STRING, "%s", colors[color1]);
  snprintf(hilt_color, SHORT_STRING, "%s", colors[color2]);
  if (IS_BLADE(obj))
    snprintf(special, SHORT_STRING, "%s%s", desc, blade_descs[rand_number(0, NUM_A_BLADE_DESCS - 1)]);
  else if (IS_PIERCE(obj))
    snprintf(special, SHORT_STRING, "%s%s", desc, piercing_descs[rand_number(0, NUM_A_PIERCING_DESCS - 1)]);
  else //blunt
    snprintf(special, SHORT_STRING, "%s%s", desc, blunt_descs[rand_number(0, NUM_A_BLUNT_DESCS - 1)]);

  roll = dice(1, 100);
  roll2 = rand_number(0, NUM_A_HEAD_TYPES - 1);
  roll3 = rand_number(0, NUM_A_HANDLE_TYPES - 1);

  // special, head color, hilt color
  if (roll >= 91)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s %s %s", weapon_list[GET_WEAPON_TYPE(obj)].name,
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], special,
             hilt_color,
             handle_types[roll3]);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s, %s-%s %s %s with %s %s %s", a_or_an(special), special,
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s, %s-%s %s %s with %s %s %s lies here.", a_or_an(special),
             special, head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // special, head color
  }
  else if (roll >= 81)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s", weapon_list[GET_WEAPON_TYPE(obj)].name,
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], special);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s, %s-%s %s %s", a_or_an(special), special,
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s, %s-%s %s %s lies here.", a_or_an(special),
             special, head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // special, hilt color
  }
  else if (roll >= 71)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s %s %s", weapon_list[GET_WEAPON_TYPE(obj)].name,
             material_name[GET_OBJ_MATERIAL(obj)], special, hilt_color,
             handle_types[roll3]);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s %s with %s %s %s", a_or_an(special), special,
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s %s with %s %s %s lies here.", a_or_an(special),
             special, material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // head color, hilt color
  }
  else if (roll >= 41)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s %s",
             weapon_list[GET_WEAPON_TYPE(obj)].name, head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)],
             hilt_color,
             handle_types[roll3]);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s with %s %s %s", a_or_an(head_color),
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s with %s %s %s lies here.", a_or_an(head_color),
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // head color
  }
  else if (roll >= 31)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s", weapon_list[GET_WEAPON_TYPE(obj)].name,
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)]);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s", a_or_an(head_color),
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s lies here.", a_or_an(head_color),
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // hilt color
  }
  else if (roll >= 21)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s %s",
             weapon_list[GET_WEAPON_TYPE(obj)].name, material_name[GET_OBJ_MATERIAL(obj)],
             hilt_color,
             handle_types[roll3]);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s with %s %s %s", a_or_an((char *)material_name[GET_OBJ_MATERIAL(obj)]),
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s with %s %s %s lies here.",
             a_or_an((char *)material_name[GET_OBJ_MATERIAL(obj)]),
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // special
  }
  else if (roll >= 11)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s", weapon_list[GET_WEAPON_TYPE(obj)].name,
             material_name[GET_OBJ_MATERIAL(obj)], special);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s %s", a_or_an(special), special,
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s %s lies here.", a_or_an(special), special,
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // none
  }
  else
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s",
             weapon_list[GET_WEAPON_TYPE(obj)].name, material_name[GET_OBJ_MATERIAL(obj)]);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s", a_or_an((char *)material_name[GET_OBJ_MATERIAL(obj)]),
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s lies here.",
             a_or_an((char *)material_name[GET_OBJ_MATERIAL(obj)]),
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);
  }

  /* object is fully described
   base object is taken care of including material, now set random stats, etc */
  cp_modify_object_applies(ch, obj, cp_convert_grade_enchantment(grade), CP_TYPE_WEAPON, rare_grade, FALSE);
}
#undef SHORT_STRING

/* give away magic weapon, method:
 * 1)  determine material
 * 2)  assign description
 * 3)  determine modifier (if applicable)
 */
#define SHORT_STRING 80

void give_magic_weapon(struct char_data *ch, int selection, int enchantment, bool silent_mode)
{
  struct obj_data *obj = NULL;
  int roll = 0;
  int color1 = 0, color2 = 0, level = 0, roll2 = 0, roll3 = 0;
  char desc[MEDIUM_STRING] = {'\0'};
  char hilt_color[SHORT_STRING] = {'\0'}, head_color[SHORT_STRING] = {'\0'};
  char special[SHORT_STRING] = {'\0'};
  char buf[MAX_STRING_LENGTH] = {'\0'};

  /* ok load blank object */
  if ((obj = read_object(WEAPON_PROTO, VIRTUAL)) == NULL)
  {
    log("SYSERR: give_magic_weapon created NULL object");
    return;
  }

  /* now set up this new object */
  set_weapon_object(obj, selection);
  /* we should have a completely usable weapon now, just missing descripts/stats */

  /* ok assigning final material here, check for upgrade */
  GET_OBJ_MATERIAL(obj) =
      possible_material_upgrade(GET_OBJ_MATERIAL(obj), enchantment);

  /* determine level */
  switch (enchantment)
  {
  case 0:
  case 1:
    level = 0;
    break;
  case 2:
    level = 5;
    break;
  case 3:
    level = 10;
    break;
  case 4:
    level = 15;
    break;
  case 5:
    level = 20;
    break;
  default: /*6*/
    level = 25;
    break;
  }

  // pick a pair of random colors for usage
  /* first assign two random colors for usage */
  color1 = rand_number(0, NUM_A_COLORS - 1);
  color2 = rand_number(0, NUM_A_COLORS - 1);
  /* make sure they are not the same colors */
  while (color2 == color1)
    color2 = rand_number(0, NUM_A_COLORS - 1);

  snprintf(head_color, SHORT_STRING, "%s", colors[color1]);
  snprintf(hilt_color, SHORT_STRING, "%s", colors[color2]);
  if (IS_BLADE(obj))
    snprintf(special, SHORT_STRING, "%s%s", desc, blade_descs[rand_number(0, NUM_A_BLADE_DESCS - 1)]);
  else if (IS_PIERCE(obj))
    snprintf(special, SHORT_STRING, "%s%s", desc, piercing_descs[rand_number(0, NUM_A_PIERCING_DESCS - 1)]);
  else //blunt
    snprintf(special, SHORT_STRING, "%s%s", desc, blunt_descs[rand_number(0, NUM_A_BLUNT_DESCS - 1)]);

  roll = dice(1, 100);
  roll2 = rand_number(0, NUM_A_HEAD_TYPES - 1);
  roll3 = rand_number(0, NUM_A_HANDLE_TYPES - 1);

  // special, head color, hilt color
  if (roll >= 91)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s %s %s", weapon_list[GET_WEAPON_TYPE(obj)].name,
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], special,
             hilt_color,
             handle_types[roll3]);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s, %s-%s %s %s with %s %s %s", a_or_an(special), special,
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s, %s-%s %s %s with %s %s %s lies here.", a_or_an(special),
             special, head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // special, head color
  }
  else if (roll >= 81)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s", weapon_list[GET_WEAPON_TYPE(obj)].name,
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], special);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s, %s-%s %s %s", a_or_an(special), special,
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s, %s-%s %s %s lies here.", a_or_an(special),
             special, head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // special, hilt color
  }
  else if (roll >= 71)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s %s %s", weapon_list[GET_WEAPON_TYPE(obj)].name,
             material_name[GET_OBJ_MATERIAL(obj)], special, hilt_color,
             handle_types[roll3]);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s %s with %s %s %s", a_or_an(special), special,
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s %s with %s %s %s lies here.", a_or_an(special),
             special, material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // head color, hilt color
  }
  else if (roll >= 41)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s %s",
             weapon_list[GET_WEAPON_TYPE(obj)].name, head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)],
             hilt_color,
             handle_types[roll3]);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s with %s %s %s", a_or_an(head_color),
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s with %s %s %s lies here.", a_or_an(head_color),
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // head color
  }
  else if (roll >= 31)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s", weapon_list[GET_WEAPON_TYPE(obj)].name,
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)]);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s", a_or_an(head_color),
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s-%s %s %s lies here.", a_or_an(head_color),
             head_color, head_types[roll2],
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // hilt color
  }
  else if (roll >= 21)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s %s",
             weapon_list[GET_WEAPON_TYPE(obj)].name, material_name[GET_OBJ_MATERIAL(obj)],
             hilt_color,
             handle_types[roll3]);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s with %s %s %s", a_or_an((char *)material_name[GET_OBJ_MATERIAL(obj)]),
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s with %s %s %s lies here.",
             a_or_an((char *)material_name[GET_OBJ_MATERIAL(obj)]),
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name,
             a_or_an(hilt_color), hilt_color,
             handle_types[roll3]);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // special
  }
  else if (roll >= 11)
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s", weapon_list[GET_WEAPON_TYPE(obj)].name,
             material_name[GET_OBJ_MATERIAL(obj)], special);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s %s", a_or_an(special), special,
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s %s lies here.", a_or_an(special), special,
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);

    // none
  }
  else
  {
    snprintf(buf, MAX_STRING_LENGTH, "%s %s",
             weapon_list[GET_WEAPON_TYPE(obj)].name, material_name[GET_OBJ_MATERIAL(obj)]);
    obj->name = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s", a_or_an((char *)material_name[GET_OBJ_MATERIAL(obj)]),
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    obj->short_description = strdup(buf);
    snprintf(buf, MAX_STRING_LENGTH, "%s %s %s lies here.",
             a_or_an((char *)material_name[GET_OBJ_MATERIAL(obj)]),
             material_name[GET_OBJ_MATERIAL(obj)], weapon_list[GET_WEAPON_TYPE(obj)].name);
    *buf = UPPER(*buf);
    obj->description = strdup(buf);
  }

  /* object is fully described
   base object is taken care of including material, now set random stats, etc */
  cp_modify_object_applies(ch, obj, enchantment, CP_TYPE_WEAPON, RARE_GRADE_NORMAL, silent_mode);
}
#undef SHORT_STRING

/* give away specific misc item
 * 1) finger, 2) neck, 3) feet, 4) hands, 5) about, 6) waist, 7) wrist, 8) held
 *   9) monk gloves (hands)
 * method:
 * 1)  determine item
 * 2)  determine material
 * 3)  assign description
 * 4)  determine modifier (if applicable)
 * 5)  determine amount (if applicable)
 */
#define SHORT_STRING 80

void give_misc_magic_item(struct char_data *ch, int category, int enchantment, bool silent_mode)
{
  struct obj_data *obj = NULL;
  int vnum = -1, material = MATERIAL_BRONZE;
  int level = 0;
  char desc[MEDIUM_STRING] = {'\0'}, armor_name[MEDIUM_STRING] = {'\0'};
  char keywords[MEDIUM_STRING] = {'\0'};
  char desc2[SHORT_STRING] = {'\0'}, desc3[SHORT_STRING] = {'\0'};
  char buf[MEDIUM_STRING] = {'\0'};

  /* assign base material
   * and last but not least, give appropriate start of description
   *  */
  switch (category)
  {
  case 1: /*finger*/
    vnum = RING_MOLD;
    material = MATERIAL_COPPER;
    snprintf(armor_name, MEDIUM_STRING, ring_descs[rand_number(0, NUM_A_RING_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, gemstones[rand_number(0, NUM_A_GEMSTONES - 1)]);
    break;
  case 2: /*neck*/
    vnum = NECKLACE_MOLD;
    material = MATERIAL_COPPER;
    snprintf(armor_name, MEDIUM_STRING, neck_descs[rand_number(0, NUM_A_NECK_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, gemstones[rand_number(0, NUM_A_GEMSTONES - 1)]);
    break;
  case 3: /*feet*/
    vnum = BOOTS_MOLD;
    material = MATERIAL_LEATHER;
    snprintf(armor_name, MEDIUM_STRING, boot_descs[rand_number(0, NUM_A_BOOT_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS - 1)]);
    snprintf(desc3, SHORT_STRING, colors[rand_number(0, NUM_A_COLORS - 1)]);
    break;
  case 4: /*hands*/
    vnum = GLOVES_MOLD;
    material = MATERIAL_LEATHER;
    snprintf(armor_name, MEDIUM_STRING, hands_descs[rand_number(0, NUM_A_HAND_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS - 1)]);
    snprintf(desc3, SHORT_STRING, colors[rand_number(0, NUM_A_COLORS - 1)]);
    break;
  case 5: /*about*/
    vnum = CLOAK_MOLD;
    material = MATERIAL_COTTON;
    snprintf(armor_name, MEDIUM_STRING, cloak_descs[rand_number(0, NUM_A_CLOAK_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, armor_crests[rand_number(0, NUM_A_ARMOR_CRESTS - 1)]);
    snprintf(desc3, SHORT_STRING, colors[rand_number(0, NUM_A_COLORS - 1)]);
    break;
  case 6: /*waist*/
    vnum = BELT_MOLD;
    material = MATERIAL_LEATHER;
    snprintf(armor_name, MEDIUM_STRING, waist_descs[rand_number(0, NUM_A_WAIST_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS - 1)]);
    snprintf(desc3, SHORT_STRING, colors[rand_number(0, NUM_A_COLORS - 1)]);
    break;
  case 7: /*wrist*/
    vnum = WRIST_MOLD;
    material = MATERIAL_COPPER;
    snprintf(armor_name, MEDIUM_STRING, wrist_descs[rand_number(0, NUM_A_WRIST_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, gemstones[rand_number(0, NUM_A_GEMSTONES - 1)]);
    break;
  case 8: /*held*/
    vnum = HELD_MOLD;
    material = MATERIAL_ONYX;
    snprintf(armor_name, MEDIUM_STRING, crystal_descs[rand_number(0, NUM_A_CRYSTAL_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, colors[rand_number(0, NUM_A_COLORS - 1)]);
    break;
  case 9: /*monk gloves*/
    vnum = GLOVES_MOLD;
    material = MATERIAL_LEATHER;
    snprintf(armor_name, MEDIUM_STRING, monk_glove_descs[rand_number(0, NUM_A_MONK_GLOVE_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS - 1)]);
    snprintf(desc3, SHORT_STRING, colors[rand_number(0, NUM_A_COLORS - 1)]);
    break;
  }

  /* we already determined 'base' material, now
   determine whether an upgrade was achieved by enchantment */
  switch (material)
  {

  case MATERIAL_COPPER:
    switch (enchantment)
    {
    case 0:
    case 1:
      material = MATERIAL_COPPER;
      break;
    case 2:
      material = MATERIAL_BRASS;
      break;
    case 3:
      material = MATERIAL_SILVER;
      break;
    case 4:
      material = MATERIAL_GOLD;
      break;
    default: /* 5 or 6 */
      material = MATERIAL_PLATINUM;
      break;
    }
    break; /*end copper*/

  case MATERIAL_LEATHER:
    switch (enchantment)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
      material = MATERIAL_LEATHER;
      break;
    default: /* 5 or 6 */
      material = MATERIAL_DRAGONHIDE;
      break;
    }
    break; /*end leather*/

  case MATERIAL_COTTON:
    switch (enchantment)
    {
    case 0:
      material = MATERIAL_HEMP;
      break;
    case 1:
      material = MATERIAL_COTTON;
      break;
    case 2:
      material = MATERIAL_WOOL;
      break;
    case 3:
      material = MATERIAL_VELVET;
      break;
    case 4:
    case 5:
      material = MATERIAL_SATIN;
      break;
    default: /* 6 */
      material = MATERIAL_SILK;
      break;
    }
    break; /*end cotton*/

    /* options:  crystal, obsidian, onyx, ivory, pewter; just random */
  case MATERIAL_ONYX:
    switch (dice(1, 5))
    {
    case 1:
      material = MATERIAL_ONYX;
      break;
    case 2:
      material = MATERIAL_PEWTER;
      break;
    case 3:
      material = MATERIAL_IVORY;
      break;
    case 4:
      material = MATERIAL_OBSIDIAN;
      break;
    default: /* 5 */
      material = MATERIAL_CRYSTAL;
      break;
    }
    break; /*end onyx*/
  }

  /* determine level */
  switch (enchantment)
  {
  case 0:
  case 1:
    level = 0;
    break;
  case 2:
    level = 5;
    break;
  case 3:
    level = 10;
    break;
  case 4:
    level = 15;
    break;
  case 5:
    level = 20;
    break;
  default: /*6*/
    level = 25;
    break;
  }

  /* ok load object */
  if ((obj = read_object(vnum, VIRTUAL)) == NULL)
  {
    log("SYSERR: give_misc_magic_item created NULL object");
    return;
  }

  /* special handling for monk gloves, etc */
  switch (category)
  {
  case 9:
    GET_OBJ_VAL(obj, 0) = enchantment;
    break;
  default:
    break;
  }

  /* set material */
  GET_OBJ_MATERIAL(obj) = material;

  /* put together a descrip */
  switch (vnum)
  {
  case RING_MOLD:
  case NECKLACE_MOLD:
  case WRIST_MOLD:
    snprintf(keywords, MEDIUM_STRING, "%s %s set with %s gemstone",
             armor_name, material_name[material], desc2);
    obj->name = strdup(keywords);
    snprintf(buf, MEDIUM_STRING, "%s %s %s set with %s %s gemstone",
             AN(material_name[material]), material_name[material],
             armor_name, AN(desc2), desc2);
    strncpy(desc, buf, MEDIUM_STRING - strlen(desc));
    obj->short_description = strdup(desc);
    strncpy(desc, " lies here.", MEDIUM_STRING - strlen(desc));
    obj->description = strdup(CAP(desc));
    break;
  case BOOTS_MOLD:
  case GLOVES_MOLD:
    snprintf(keywords, MEDIUM_STRING, "%s pair %s %s", armor_name, desc2, desc3);
    obj->name = strdup(keywords);
    snprintf(buf, MEDIUM_STRING, "a pair of %s %s %s", desc2, desc3,
             armor_name);
    strncpy(desc, buf, MEDIUM_STRING - strlen(desc));
    obj->short_description = strdup(desc);
    snprintf(desc, MEDIUM_STRING, "A pair of %s %s %s lie here.", desc2, desc3,
             armor_name);
    obj->description = strdup(desc);
    break;
  case CLOAK_MOLD:
    snprintf(keywords, MEDIUM_STRING, "%s %s %s %s bearing crest", armor_name, desc2,
             material_name[material], desc3);
    obj->name = strdup(keywords);
    snprintf(buf, MEDIUM_STRING, "%s %s %s %s bearing the crest of %s %s", AN(desc3), desc3,
             material_name[material], armor_name, AN(desc2),
             desc2);
    strncpy(desc, buf, MEDIUM_STRING - strlen(desc));
    obj->short_description = strdup(desc);
    snprintf(desc, MEDIUM_STRING, "%s %s %s %s bearing the crest of %s %s is lying here.", AN(desc3), desc3,
             material_name[material], armor_name, AN(desc2),
             desc2);
    obj->description = strdup(CAP(desc));
    break;
  case BELT_MOLD:
    snprintf(keywords, MEDIUM_STRING, "%s %s %s", armor_name, desc2, desc3);
    obj->name = strdup(keywords);
    snprintf(buf, MEDIUM_STRING, "%s %s %s %s", AN(desc2), desc2, desc3,
             armor_name);
    strncpy(desc, buf, MEDIUM_STRING - strlen(desc));
    obj->short_description = strdup(desc);
    snprintf(desc, MEDIUM_STRING, "%s %s %s %s lie here.", AN(desc2), desc2, desc3,
             armor_name);
    obj->description = strdup(desc);
    break;
  case HELD_MOLD:
    snprintf(keywords, MEDIUM_STRING, "%s %s orb", armor_name, desc2);
    obj->name = strdup(keywords);
    snprintf(buf, MEDIUM_STRING, "a %s %s orb", desc2, armor_name);
    strncpy(desc, buf, MEDIUM_STRING - strlen(desc));
    obj->short_description = strdup(desc);
    snprintf(desc, MEDIUM_STRING, "A %s %s orb is lying here.", desc2, armor_name);
    obj->description = strdup(desc);
    break;
  }

  /* level, bonus and cost */
  cp_modify_object_applies(ch, obj, enchantment, CP_TYPE_MISC, RARE_GRADE_NORMAL, silent_mode);
}
#undef SHORT_STRING

/* give away specific misc item
 * 1) finger, 2) neck, 3) feet, 4) hands, 5) about, 6) waist, 7) wrist, 8) held
 *   9) monk gloves (hands)
 * method:
 * 1)  determine item
 * 2)  determine material
 * 3)  assign description
 * 4)  determine modifier (if applicable)
 * 5)  determine amount (if applicable)
 */
#define SHORT_STRING 80

void award_misc_magic_item(struct char_data *ch, int category, int grade)
{
  struct obj_data *obj = NULL;
  int vnum = -1, material = MATERIAL_BRONZE;
  int level = 0;
  char desc[MEDIUM_STRING] = {'\0'}, armor_name[MEDIUM_STRING] = {'\0'};
  char keywords[MEDIUM_STRING] = {'\0'}, buf[MEDIUM_STRING] = {'\0'};
  char desc2[SHORT_STRING] = {'\0'}, desc3[SHORT_STRING] = {'\0'};
  int rare_grade = RARE_GRADE_NORMAL;

  /* determine if rare or not, start building string */
  rare_grade = determine_rare_grade();
  snprintf(desc, MEDIUM_STRING, label_rare_grade(rare_grade));

  /* assign base material
   * and last but not least, give appropriate start of description
   *  */
  switch (category)
  {
  case 1: /*finger*/
    vnum = RING_MOLD;
    material = MATERIAL_COPPER;
    snprintf(armor_name, MEDIUM_STRING, ring_descs[rand_number(0, NUM_A_RING_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, gemstones[rand_number(0, NUM_A_GEMSTONES - 1)]);
    break;
  case 2: /*neck*/
    vnum = NECKLACE_MOLD;
    material = MATERIAL_COPPER;
    snprintf(armor_name, MEDIUM_STRING, neck_descs[rand_number(0, NUM_A_NECK_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, gemstones[rand_number(0, NUM_A_GEMSTONES - 1)]);
    break;
  case 3: /*feet*/
    vnum = BOOTS_MOLD;
    material = MATERIAL_LEATHER;
    snprintf(armor_name, MEDIUM_STRING, boot_descs[rand_number(0, NUM_A_BOOT_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS - 1)]);
    snprintf(desc3, SHORT_STRING, colors[rand_number(0, NUM_A_COLORS - 1)]);
    break;
  case 4: /*hands*/
    vnum = GLOVES_MOLD;
    material = MATERIAL_LEATHER;
    snprintf(armor_name, MEDIUM_STRING, hands_descs[rand_number(0, NUM_A_HAND_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS - 1)]);
    snprintf(desc3, SHORT_STRING, colors[rand_number(0, NUM_A_COLORS - 1)]);
    break;
  case 5: /*about*/
    vnum = CLOAK_MOLD;
    material = MATERIAL_COTTON;
    snprintf(armor_name, MEDIUM_STRING, cloak_descs[rand_number(0, NUM_A_CLOAK_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, armor_crests[rand_number(0, NUM_A_ARMOR_CRESTS - 1)]);
    snprintf(desc3, SHORT_STRING, colors[rand_number(0, NUM_A_COLORS - 1)]);
    break;
  case 6: /*waist*/
    vnum = BELT_MOLD;
    material = MATERIAL_LEATHER;
    snprintf(armor_name, MEDIUM_STRING, waist_descs[rand_number(0, NUM_A_WAIST_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS - 1)]);
    snprintf(desc3, SHORT_STRING, colors[rand_number(0, NUM_A_COLORS - 1)]);
    break;
  case 7: /*wrist*/
    vnum = WRIST_MOLD;
    material = MATERIAL_COPPER;
    snprintf(armor_name, MEDIUM_STRING, wrist_descs[rand_number(0, NUM_A_WRIST_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, gemstones[rand_number(0, NUM_A_GEMSTONES - 1)]);
    break;
  case 8: /*held*/
    vnum = HELD_MOLD;
    material = MATERIAL_ONYX;
    snprintf(armor_name, MEDIUM_STRING, crystal_descs[rand_number(0, NUM_A_CRYSTAL_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, colors[rand_number(0, NUM_A_COLORS - 1)]);
    break;
  case 9: /*monk gloves*/
    vnum = GLOVES_MOLD;
    material = MATERIAL_LEATHER;
    snprintf(armor_name, MEDIUM_STRING, monk_glove_descs[rand_number(0, NUM_A_MONK_GLOVE_DESCS - 1)]);
    snprintf(desc2, SHORT_STRING, armor_special_descs[rand_number(0, NUM_A_ARMOR_SPECIAL_DESCS - 1)]);
    snprintf(desc3, SHORT_STRING, colors[rand_number(0, NUM_A_COLORS - 1)]);
    break;
  }

  /* we already determined 'base' material, now
   determine whether an upgrade was achieved by grade */
  switch (material)
  {

  case MATERIAL_COPPER:
    switch (grade)
    {
    case 0:
    case 1:
      material = MATERIAL_COPPER;
      break;
    case 2:
      material = MATERIAL_BRASS;
      break;
    case 3:
      material = MATERIAL_SILVER;
      break;
    case 4:
      material = MATERIAL_GOLD;
      break;
    default: /* 5 or 6 */
      material = MATERIAL_PLATINUM;
      break;
    }
    break; /*end copper*/

  case MATERIAL_LEATHER:
    switch (grade)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
      material = MATERIAL_LEATHER;
      break;
    default: /* 5 or 6 */
      material = MATERIAL_DRAGONHIDE;
      break;
    }
    break; /*end leather*/

  case MATERIAL_COTTON:
    switch (grade)
    {
    case 0:
      material = MATERIAL_HEMP;
      break;
    case 1:
      material = MATERIAL_COTTON;
      break;
    case 2:
      material = MATERIAL_WOOL;
      break;
    case 3:
      material = MATERIAL_VELVET;
      break;
    case 4:
    case 5:
      material = MATERIAL_SATIN;
      break;
    default: /* 6 */
      material = MATERIAL_SILK;
      break;
    }
    break; /*end cotton*/

    /* options:  crystal, obsidian, onyx, ivory, pewter; just random */
  case MATERIAL_ONYX:
    switch (dice(1, 5))
    {
    case 1:
      material = MATERIAL_ONYX;
      break;
    case 2:
      material = MATERIAL_PEWTER;
      break;
    case 3:
      material = MATERIAL_IVORY;
      break;
    case 4:
      material = MATERIAL_OBSIDIAN;
      break;
    default: /* 5 */
      material = MATERIAL_CRYSTAL;
      break;
    }
    break; /*end onyx*/
  }

  /* determine level */
  switch (grade)
  {
  case 0:
  case 1:
    level = 0;
    break;
  case 2:
    level = 5;
    break;
  case 3:
    level = 10;
    break;
  case 4:
    level = 15;
    break;
  case 5:
    level = 20;
    break;
  default: /*6*/
    level = 25;
    break;
  }

  /* ok load object */
  if ((obj = read_object(vnum, VIRTUAL)) == NULL)
  {
    log("SYSERR: award_misc_magic_item created NULL object");
    return;
  }

  /* special handling for monk gloves, etc */
  switch (category)
  {
  case 9: /* monk gloves */
    GET_OBJ_VAL(obj, 0) = grade;
    break;
  default:
    break;
  }

  /* set material */
  GET_OBJ_MATERIAL(obj) = material;

  /* put together a descrip */
  switch (vnum)
  {
  case RING_MOLD:
  case NECKLACE_MOLD:
  case WRIST_MOLD:
    snprintf(keywords, MEDIUM_STRING, "%s %s set with %s gemstone",
             armor_name, material_name[material], desc2);
    obj->name = strdup(keywords);
    snprintf(buf, MEDIUM_STRING, "%s %s %s set with %s %s gemstone",
             AN(material_name[material]), material_name[material],
             armor_name, AN(desc2), desc2);
    strncpy(desc, buf, MEDIUM_STRING - strlen(desc));
    obj->short_description = strdup(desc);
    snprintf(desc, MEDIUM_STRING, "%s %s %s set with %s %s gemstone lies here.",
             AN(material_name[material]), material_name[material],
             armor_name, AN(desc2), desc2);
    obj->description = strdup(CAP(desc));
    break;
  case BOOTS_MOLD:
  case GLOVES_MOLD:
    snprintf(keywords, MEDIUM_STRING, "%s pair %s %s", armor_name, desc2, desc3);
    obj->name = strdup(keywords);
    snprintf(buf, MEDIUM_STRING, "a pair of %s %s %s", desc2, desc3,
             armor_name);
    strncpy(desc, buf, MEDIUM_STRING - strlen(desc));
    obj->short_description = strdup(desc);
    snprintf(desc, MEDIUM_STRING, "A pair of %s %s %s lie here.", desc2, desc3,
             armor_name);
    obj->description = strdup(desc);
    break;
  case CLOAK_MOLD:
    snprintf(keywords, MEDIUM_STRING, "%s %s %s %s bearing crest", armor_name, desc2,
             material_name[material], desc3);
    obj->name = strdup(keywords);
    snprintf(buf, MEDIUM_STRING, "%s %s %s %s bearing the crest of %s %s", AN(desc3), desc3,
             material_name[material], armor_name, AN(desc2),
             desc2);
    strncpy(desc, buf, MEDIUM_STRING - strlen(desc));
    obj->short_description = strdup(desc);
    snprintf(desc, MEDIUM_STRING, "%s %s %s %s bearing the crest of %s %s is lying here.", AN(desc3), desc3,
             material_name[material], armor_name, AN(desc2),
             desc2);
    obj->description = strdup(CAP(desc));
    break;
  case BELT_MOLD:
    snprintf(keywords, MEDIUM_STRING, "%s %s %s", armor_name, desc2, desc3);
    obj->name = strdup(keywords);
    snprintf(buf, MEDIUM_STRING, "%s %s %s %s", AN(desc2), desc2, desc3,
             armor_name);
    strncpy(desc, buf, MEDIUM_STRING - strlen(desc));
    obj->short_description = strdup(desc);
    snprintf(desc, MEDIUM_STRING, "%s %s %s %s lie here.", AN(desc2), desc2, desc3,
             armor_name);
    obj->description = strdup(desc);
    break;
  case HELD_MOLD:
    snprintf(keywords, MEDIUM_STRING, "%s %s orb", armor_name, desc2);
    obj->name = strdup(keywords);
    snprintf(buf, MEDIUM_STRING, "a %s %s orb", desc2, armor_name);
    strncpy(desc, buf, MEDIUM_STRING - strlen(desc));
    obj->short_description = strdup(desc);
    snprintf(desc, MEDIUM_STRING, "A %s %s orb is lying here.", desc2, armor_name);
    obj->description = strdup(desc);
    break;
  }

  /* level, bonus and cost */
  cp_modify_object_applies(ch, obj, grade, CP_TYPE_MISC, rare_grade, FALSE);
}
#undef SHORT_STRING

/* Load treasure on a mob. -Ornir */
void load_treasure(char_data *mob)
{
  int level = 0;
  int grade = GRADE_MUNDANE, max_grade = GRADE_MUNDANE;

  if (!IS_NPC(mob))
    return;

  level = GET_LEVEL(mob);

  /* determine maximum grade of treasure */
  if (level >= 25)
  {
    max_grade = GRADE_SUPERIOR;
  }
  else if (level >= 20)
  {
    max_grade = GRADE_MAJOR;
  }
  else if (level >= 15)
  {
    max_grade = GRADE_MEDIUM;
  }
  else if (level >= 10)
  {
    max_grade = GRADE_TYPICAL;
  }
  else if (level >= 5)
  {
    max_grade = GRADE_MINOR;
  }
  else
  {
    max_grade = GRADE_MUNDANE;
  }

  /* okay now determine grade */
  grade = dice(1, max_grade);

  /* Give the mob one magic item. */
  award_magic_item(1, mob, grade);
}

/* utility function for bazaar below - misc armoring such
   as rings, necklaces, bracelets, etc */
void disp_misc_type_menu(struct char_data *ch)
{

  send_to_char(ch,
               "1) finger\r\n"
               "2) neck\r\n"
               "3) feet\r\n"
               "4) hands\r\n"
               "5) about\r\n"
               "6) waist\r\n"
               "7) wrist\r\n"
               "8) hold\r\n"
               "9) monk-gloves\r\n");
}

/* command to load specific treasure */

/* bazaar <item category> <selection from category> <enchantment level> */
SPECIAL(bazaar)
{
  char arg1[MAX_STRING_LENGTH] = {'\0'};
  char arg2[MAX_STRING_LENGTH] = {'\0'};
  char arg3[MAX_STRING_LENGTH] = {'\0'};
  int enchant = 0;
  int selection = 0;
  int type = 0;
  int cost = 0;

  if (CMD_IS("bazaar"))
  {

    three_arguments(argument, arg1, arg2, arg3);

    if (!*arg1)
    {
      send_to_char(ch, "\tcSyntax:\tn bazaar <item category> <selection number> <enchantment level>\r\n");
      send_to_char(ch, "\tcItem Categories:\tn armor, weapon or misc.\r\n");
      send_to_char(ch, "\tcIf you type:\tn bazaar <item category> with no extra arguments, "
                       "it will display the 'selection number' choices\r\n");
      send_to_char(ch, "\tcEnchantment Level:\tn 0-6\r\n");
      return TRUE;
    }

    /* set our category */
    if (*arg1)
    {
      if (is_abbrev(arg1, "armor"))
        type = 1;
      else if (is_abbrev(arg1, "weapon"))
        type = 2;
      else if (is_abbrev(arg1, "misc"))
        type = 3;
      else
      {
        send_to_char(ch, "The first argument must be an Item Category: armor, weapon or misc.\r\n");
        return TRUE;
      }
    }

    /* list our possible selections, then EXIT */
    if (*arg1 && !*arg2)
    {
      send_to_char(ch, "Please refer to selection number below for the 2nd argument:\r\n");
      switch (type)
      {
      case 1: /* armor */
        oedit_disp_armor_type_menu(ch->desc);
        break;
      case 2: /* weapon */
        oedit_disp_weapon_type_menu(ch->desc);
        break;
      case 3: /* misc */
        disp_misc_type_menu(ch);
        break;
      default:
        break;
      }
      return TRUE;
    }

    /* selection number! */
    if (!*arg2)
    {
      send_to_char(ch, "Second argument required, the second argument must be a 'selection number'\r\n");
      send_to_char(ch, "If you type: bazaar <item category> with no extra arguments, "
                       "it will display the 'selection number' choices\r\n");
      return TRUE;
    }
    else if (*arg2 && !isdigit(arg2[0]))
    {
      send_to_char(ch, "The second argument must be an integer.\r\n");
      return TRUE;
    }

    /* missing or invalid enchantment level */
    if (!*arg3)
    {
      send_to_char(ch, "You need to select an enchantment level as the third argument (0-6).\r\n");
      return TRUE;
    }
    else if (*arg3 && !isdigit(arg3[0]))
    {
      send_to_char(ch, "The third argument must be an integer.\r\n");
      return TRUE;
    }

    /* more checks of validity of 2nd argument (selection number) */
    if (*arg2)
    {
      selection = atoi(arg2);

      switch (type)
      {
      case 1: /* armor */
        if (selection <= 0 || selection >= NUM_SPEC_ARMOR_TYPES)
        {
          send_to_char(ch, "Invalid value for 'selection number'!\r\n");
          oedit_disp_armor_type_menu(ch->desc);
          return TRUE;
        }
        break;
      case 2: /* weapon */
        if (selection <= 1 || selection >= NUM_WEAPON_TYPES)
        {
          send_to_char(ch, "Invalid value for 'selection number'!\r\n");
          oedit_disp_weapon_type_menu(ch->desc);
          if (selection == 1)
            send_to_char(ch, "\tRPlease do not select UNARMED.\t\n\r\n");
          return TRUE;
        }
        break;
      case 3: /* misc */
        if (selection <= 0 || selection > NUM_MISC_CATEGORIES)
        {
          send_to_char(ch, "Invalid value for 'selection number'!\r\n");
          disp_misc_type_menu(ch);
          return TRUE;
        }
        break;
      default:
        send_to_char(ch, "Invalid value for 'selection number'!\r\n");
        return TRUE;
      }
    } /* should be valid! */

    /* more checks of validity of 3rd argument (enchantment level) */
    if (*arg3)
      enchant = atoi(arg3);
    if (enchant < 0 || enchant > 6)
    {
      send_to_char(ch, "Invalid!  Enchantment Levels: 0-6 only\r\n");
      return TRUE;
    }

    /* quest point cost */
    cost = 1 + (enchant * enchant) + (50 * enchant);
    if (cost > GET_QUESTPOINTS(ch))
    {
      send_to_char(ch, "You do not have enough questpoints, %d required.\r\n", cost);
      return TRUE;
    }
    else
    {
      GET_QUESTPOINTS(ch) -= cost;
      send_to_char(ch, "You pay %d quest points.  You have %d left.\r\n",
                   cost, GET_QUESTPOINTS(ch));
    }

    /* we should be ready to go! */
    switch (type)
    {
    case 1: /* armor */
      give_magic_armor(ch, selection, enchant, TRUE);
      break;
    case 2: /* weapon */
      give_magic_weapon(ch, selection, enchant, TRUE);
      break;
    case 3: /* misc */
      give_misc_magic_item(ch, selection, enchant, TRUE);
      break;
    default:
      break;
    }

    return TRUE; /*end*/
  }

  return FALSE;
}

/* staff tool to load random items */
ACMD(do_loadmagicspecific)
{
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  char arg3[MAX_STRING_LENGTH];
  int grade = 0;
  int number = 0;
  int i = 0;

  three_arguments(argument, arg1, arg2, arg3);

  if (!*arg1)
  {
    send_to_char(ch, "Syntax: lms [mundane|minor|typical|medium|major|superior] "
                     "[weapon|shield|body|legs|arms|head|crystal|ammo|finger|neck|wrist|feet|monk|hand|about|waist|held|scroll|potion|wand|staff] <AMOUNT>\r\n");
    return;
  }
  if (!*arg2)
  {
    send_to_char(ch, "2nd argument must be: [weapon|shield|body|legs|arms|head|crystal|ammo|finger|neck|wrist|feet|monk|hand|about|waist|held|scroll|potion|wand|staff]\r\n");
    return;
  }

  /* amount */
  if (!*arg3)
  {
    number = 1;
  }
  else
  {
    number = atoi(arg3);
    if (number <= 0)
      number = 1;

    if (number > 25)
      number = 25;
  }

  if (is_abbrev(arg1, "mundane"))
    grade = GRADE_MUNDANE;
  else if (is_abbrev(arg1, "minor"))
    grade = GRADE_MINOR;
  else if (is_abbrev(arg1, "typical"))
    grade = GRADE_TYPICAL;
  else if (is_abbrev(arg1, "medium"))
    grade = GRADE_MEDIUM;
  else if (is_abbrev(arg1, "major"))
    grade = GRADE_MAJOR;
  else if (is_abbrev(arg1, "superior"))
    grade = GRADE_SUPERIOR;
  else
  {
    send_to_char(ch, "Syntax: lms [mundane|minor|typical|medium|major|superior] "
                     "[weapon|shield|body|legs|arms|head|crystal|ammo|finger|neck|wrist|feet|monk|hand|about|waist|held|scroll|potion|wand|staff]\r\n");
    return;
  }

  for (i = 0; i < number; i++)
  {
    if (is_abbrev(arg2, "weapon"))
      award_magic_weapon(ch, grade);
    else if (is_abbrev(arg2, "body"))
      award_magic_armor(ch, grade, ITEM_WEAR_BODY);
    else if (is_abbrev(arg2, "legs"))
      award_magic_armor(ch, grade, ITEM_WEAR_LEGS);
    else if (is_abbrev(arg2, "arms"))
      award_magic_armor(ch, grade, ITEM_WEAR_ARMS);
    else if (is_abbrev(arg2, "head"))
      award_magic_armor(ch, grade, ITEM_WEAR_HEAD);
    else if (is_abbrev(arg2, "crystal"))
      award_random_crystal(ch, grade);
    else if (is_abbrev(arg2, "shield"))
      award_magic_armor(ch, grade, ITEM_WEAR_SHIELD);
    else if (is_abbrev(arg2, "ammo"))
      award_magic_ammo(ch, grade);
    else if (is_abbrev(arg2, "finger"))
      award_misc_magic_item(ch, TRS_SLOT_FINGER, grade);
    else if (is_abbrev(arg2, "neck"))
      award_misc_magic_item(ch, TRS_SLOT_NECK, grade);
    else if (is_abbrev(arg2, "wrist"))
      award_misc_magic_item(ch, TRS_SLOT_WRIST, grade);
    else if (is_abbrev(arg2, "feet"))
      award_misc_magic_item(ch, TRS_SLOT_FEET, grade);
    else if (is_abbrev(arg2, "monk"))
      award_misc_magic_item(ch, TRS_SLOT_MONK_GLOVES, grade);
    else if (is_abbrev(arg2, "hand"))
      award_misc_magic_item(ch, TRS_SLOT_HAND, grade);
    else if (is_abbrev(arg2, "about"))
      award_misc_magic_item(ch, TRS_SLOT_ABOUT, grade);
    else if (is_abbrev(arg2, "waist"))
      award_misc_magic_item(ch, TRS_SLOT_WAIST, grade);
    else if (is_abbrev(arg2, "held"))
      award_misc_magic_item(ch, TRS_SLOT_HELD, grade);
    else if (is_abbrev(arg2, "scroll"))
      award_expendable_item(ch, grade, TYPE_SCROLL);
    else if (is_abbrev(arg2, "potion"))
      award_expendable_item(ch, grade, TYPE_POTION);
    else if (is_abbrev(arg2, "wand"))
      award_expendable_item(ch, grade, TYPE_WAND);
    else if (is_abbrev(arg2, "staff"))
      award_expendable_item(ch, grade, TYPE_STAFF);
  }

  send_to_char(ch, "You attempt to create some items!\r\n");

  return;
}

/* staff tool to load random items */
ACMD(do_loadmagic)
{
  char arg1[MAX_STRING_LENGTH];
  char arg2[MAX_STRING_LENGTH];
  int number = 1;
  int grade = 0;

  two_arguments(argument, arg1, arg2);

  if (!*arg1)
  {
    send_to_char(ch, "Syntax: loadmagic [mundane | minor | typical | medium | major | superior] [# of items]\r\n");
    send_to_char(ch, "See also: loadmagicspecific (lms)\r\n");
    return;
  }

  if (*arg2 && !isdigit(arg2[0]))
  {
    send_to_char(ch, "The second number must be an integer.\r\n");
    return;
  }

  if (is_abbrev(arg1, "mundane"))
    grade = GRADE_MUNDANE;
  else if (is_abbrev(arg1, "minor"))
    grade = GRADE_MINOR;
  else if (is_abbrev(arg1, "typical"))
    grade = GRADE_TYPICAL;
  else if (is_abbrev(arg1, "medium"))
    grade = GRADE_MEDIUM;
  else if (is_abbrev(arg1, "major"))
    grade = GRADE_MAJOR;
  else if (is_abbrev(arg1, "superior"))
    grade = GRADE_SUPERIOR;
  else
  {
    send_to_char(ch, "Syntax: loadmagic [mundane | minor | typical | medium | major | superior] [# of items]\r\n");
    return;
  }

  if (*arg2)
    number = atoi(arg2);

  if (number <= 0)
    number = 1;

  if (number >= 50)
    number = 50;

  award_magic_item(number, ch, grade);
}
