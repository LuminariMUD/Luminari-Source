/* ************************************************************************
 *    File:   hunts.c                                Part of LuminariMUD  *
 * Purpose:   Hunt system                                                 *
 *  Author:   Gicker                                                      *
 ************************************************************************ */

#include <math.h>

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "oasis.h"
#include "screen.h"
#include "handler.h"
#include "constants.h"
#include "interpreter.h"
#include "race.h"
#include "wilderness.h"
#include "hunts.h"
#include "act.h"
#include "spec_abilities.h"
#include "assign_wpn_armor.h"

/* To Do
 *
 * Add reward system
 * Add special attacks system
 * 
 */

extern struct char_data *character_list;
extern struct room_data *world;

struct hunt_type hunt_table[NUM_HUNT_TYPES];
/* active hunts:
 * first field is the level, 0=10, 1=15, 2=20, 3=25, 4=30
 * second field, 0 = hunt type, 1 = reported x coord, 2 = reported y coord, 3 = actual x coord, 4 = actual y coord, 6 = has the hunt been loaded yet?
 */

int active_hunts[AHUNT_1][AHUNT_2];
int hunt_reset_timer;

void add_hunt(int hunt_type, int level, const char *name, const char *description, const char *long_description, int char_class, int alignment, int race_type,
              int subrace1, int subrace2, int subrace3, int size)
{
    hunt_table[hunt_type].level = level;
    hunt_table[hunt_type].name = name;
    hunt_table[hunt_type].description = description;
    hunt_table[hunt_type].long_description = long_description;
    hunt_table[hunt_type].char_class = char_class;
    hunt_table[hunt_type].alignment = alignment;
    hunt_table[hunt_type].race_type = race_type;
    hunt_table[hunt_type].subrace[0] = subrace1;
    hunt_table[hunt_type].subrace[1] = subrace2;
    hunt_table[hunt_type].subrace[2] = subrace3;
    hunt_table[hunt_type].size = size;
}

void add_hunt_ability(int hunt_type, int ability)
{
    hunt_table[hunt_type].abilities[ability] = true;
}

void init_hunts(void)
{
    int i = 0, j = 0;
    for (i = 0; i < NUM_HUNT_TYPES; i++)
    {
        hunt_table[i].level = 1;
        hunt_table[i].name = "Nothing";
        hunt_table[i].description = "Nothing";
        hunt_table[i].long_description = "Nothing";
        hunt_table[i].char_class = CLASS_WARRIOR;
        hunt_table[i].alignment = TRUE_NEUTRAL;
        hunt_table[i].race_type = RACE_TYPE_HUMANOID;
        hunt_table[i].subrace[0] = SUBRACE_UNKNOWN;
        hunt_table[i].subrace[1] = SUBRACE_UNKNOWN;
        hunt_table[i].subrace[2] = SUBRACE_UNKNOWN;
        hunt_table[i].size = SIZE_MEDIUM;
        for (j = 0; j < NUM_HUNT_ABILITIES; j++)
            hunt_table[i].abilities[j] = false;
    }
}

void load_hunts(void)
{
    init_hunts();

    add_hunt(HUNT_TYPE_BASILISK, 10, "basilisk", "This squat, reptilian monster has eight legs, bony spurs jutting from its back, and eyes that glow with pale green fire.",
             "A squat, reptilian basilisk is here feeding on a dead animal.", CLASS_WARRIOR, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST, 
             SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_MEDIUM);
    add_hunt_ability(HUNT_TYPE_BASILISK, HUNT_ABIL_PETRIFY);

    add_hunt(HUNT_TYPE_MANTICORE, 10, "manticore", "This creature has a vaguely humanoid head, the body of a lion, and the wings of a dragon. Its tail ends in long, sharp spikes.",
            "A large manticore is here staring at you menacingly.", CLASS_WARRIOR, LAWFUL_EVIL, RACE_TYPE_MAGICAL_BEAST,
            SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_LARGE);
    add_hunt_ability(HUNT_TYPE_MANTICORE, HUNT_ABIL_TAIL_SPIKES);

    add_hunt(HUNT_TYPE_WRAITH, 10, "wraith", "This ghostly creature is little more than a dark shape with two flickering pinpoints of light where its eyes should be.", 
      "A dark, ghostly wraith floats ominously before you.", CLASS_WARRIOR, LAWFUL_EVIL, RACE_TYPE_UNDEAD, 
      SUBRACE_INCORPOREAL, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_MEDIUM );
    add_hunt_ability(HUNT_TYPE_WRAITH, HUNT_ABIL_LEVEL_DRAIN);  

    add_hunt(HUNT_TYPE_SIREN, 10, "siren",  "This creature has the body of a hawk and the head of a beautiful woman with long, shining hair.", 
      "A beautiful siren sits before you singing a haunting song.", CLASS_BARD, CHAOTIC_NEUTRAL, RACE_TYPE_MAGICAL_BEAST, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_MEDIUM );
    add_hunt_ability(HUNT_TYPE_SIREN, HUNT_ABIL_CHARM);      add_hunt_ability(HUNT_TYPE_SIREN, HUNT_ABIL_CAUSE_FEAR);

    add_hunt(HUNT_TYPE_WILL_O_WISP, 10, "will o wisp",  "This faintly glowing ball of light bobs gently in the air, the nebulous image of what might be a skull visible somewhere in its depths.", 
      "A faintly glowing will o\' wisp floats before you.", CLASS_WARRIOR, CHAOTIC_EVIL, RACE_TYPE_ABERRATION, 
      SUBRACE_AIR, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_SMALL );
    add_hunt_ability(HUNT_TYPE_WILL_O_WISP, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_WILL_O_WISP, HUNT_ABIL_MAGIC_IMMUNITY);
    add_hunt_ability(HUNT_TYPE_WILL_O_WISP, HUNT_ABIL_INVISIBILITY);  

    add_hunt(HUNT_TYPE_BARGHEST, 15, "barghest",  "This snarling, canine beast pads forward on all fours, its slender front limbs looking more like hands than a wolf’s paws.", 
      "A large, snarling barghest paces the ground before you.", CLASS_WARRIOR, LAWFUL_EVIL, RACE_TYPE_OUTSIDER, 
      SUBRACE_EVIL, SUBRACE_SHAPECHANGER, SUBRACE_LAWFUL, SIZE_MEDIUM );
    add_hunt_ability(HUNT_TYPE_BARGHEST, HUNT_ABIL_CHARM);      add_hunt_ability(HUNT_TYPE_BARGHEST, HUNT_ABIL_BLINK);
    add_hunt_ability(HUNT_TYPE_BARGHEST, HUNT_ABIL_CAUSE_FEAR);  

    add_hunt(HUNT_TYPE_BLACK_PUDDING, 15, "black pudding",  "This black, amorphous blob piles up on itself, a quivering mound of midnight sludge that glistens darkly before surging forward.", 
      "A huge, amorphus black pudding blob glistens darkly.", CLASS_WARRIOR, TRUE_NEUTRAL, RACE_TYPE_OOZE, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_HUGE );
    add_hunt_ability(HUNT_TYPE_BLACK_PUDDING, HUNT_ABIL_ENGULF);  

    add_hunt(HUNT_TYPE_CHIMERA, 15, "chimera",  "This winged monster has the body of a lion, though two more heads flank its central feline one—a dragon and a horned goat.", 
      "A three-headed, winged chimera roars loadly.", CLASS_WARRIOR, CHAOTIC_EVIL, RACE_TYPE_MAGICAL_BEAST, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_LARGE );
    add_hunt_ability(HUNT_TYPE_CHIMERA, HUNT_ABIL_FLIGHT);      add_hunt_ability(HUNT_TYPE_CHIMERA, HUNT_ABIL_FIRE_BREATH);

    add_hunt(HUNT_TYPE_GHOST, 15, "ghost",  "This spectral, horrifying figure glides silently through the air, passing through solid objects as if they didn’t exist.", 
      "A translucent ghost floats around in front of you.", CLASS_WARRIOR, LAWFUL_GOOD, RACE_TYPE_UNKNOWN, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_FINE );
    add_hunt_ability(HUNT_TYPE_GHOST, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_GHOST, HUNT_ABIL_CORRUPTION);
    add_hunt_ability(HUNT_TYPE_GHOST, HUNT_ABIL_REGENERATION);  

    add_hunt(HUNT_TYPE_MEDUSA, 15, "medusa",  "This slender, attractive woman has strangely glowing eyes and a full head of hissing snakes for hair.", 
      "A terrifying medusa grins an evil grin.", CLASS_WARRIOR, LAWFUL_EVIL, RACE_TYPE_MONSTROUS_HUMANOID, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_MEDIUM );
    add_hunt_ability(HUNT_TYPE_MEDUSA, HUNT_ABIL_PETRIFY);      add_hunt_ability(HUNT_TYPE_MEDUSA, HUNT_ABIL_POISON);

    add_hunt(HUNT_TYPE_BEHIR, 20, "behir",  "This slithering, multilegged blue reptile has a fearsome head crowned with two large, curling horns.", 
      "A long, slithering blue behir sparks lightning from its maw.", CLASS_WARRIOR, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST, 
      SUBRACE_REPTILIAN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_HUGE );
    add_hunt_ability(HUNT_TYPE_BEHIR, HUNT_ABIL_LIGHTNING_BREATH);  

    add_hunt(HUNT_TYPE_EFREETI, 20, "efreeti",  "This muscular giant has crimson skin, smoldering eyes, and small black horns. Smoke rises in curls from its flesh.", 
      "A giant flaming efreeti smolders the ground here.", CLASS_SORCERER, LAWFUL_EVIL, RACE_TYPE_OUTSIDER, 
      SUBRACE_EXTRAPLANAR, SUBRACE_FIRE, SUBRACE_UNKNOWN, SIZE_LARGE );

    add_hunt(HUNT_TYPE_DJINNI, 20, "djinni",  "This creature stands nearly twice as tall as a human, although its lower torso trails away into a vortex of mist and wind.", 
      "A towering djinni gazes sternly from a cloud of thunder and mist.", CLASS_SORCERER, CHAOTIC_GOOD, RACE_TYPE_OUTSIDER, 
      SUBRACE_EXTRAPLANAR, SUBRACE_AIR, SUBRACE_UNKNOWN, SIZE_LARGE );
    add_hunt_ability(HUNT_TYPE_DJINNI, HUNT_ABIL_FLIGHT);  

    add_hunt(HUNT_TYPE_YOUNG_RED_DRAGON, 20, "young red dragon",  "A crown of cruel horns surrounds the head of this mighty dragon. Thick scales the color of molten rock cover its long body.", 
      "A young red dragon snorts flame from its nostrils.", CLASS_SORCERER, CHAOTIC_EVIL, RACE_TYPE_DRAGON, 
      SUBRACE_FIRE, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_LARGE );
    add_hunt_ability(HUNT_TYPE_YOUNG_RED_DRAGON, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_YOUNG_RED_DRAGON, HUNT_ABIL_FLIGHT);
    add_hunt_ability(HUNT_TYPE_YOUNG_RED_DRAGON, HUNT_ABIL_FIRE_BREATH);  

    add_hunt(HUNT_TYPE_ADULT_RED_DRAGON, 25, "adult red dragon",  "A crown of cruel horns surrounds the head of this mighty dragon. Thick scales the color of molten rock cover its long body.", 
      "An adult red dragon snorts flame from its nostrils.", CLASS_SORCERER, CHAOTIC_EVIL, RACE_TYPE_DRAGON, 
      SUBRACE_FIRE, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_HUGE );
    add_hunt_ability(HUNT_TYPE_ADULT_RED_DRAGON, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_ADULT_RED_DRAGON, HUNT_ABIL_FLIGHT);
    add_hunt_ability(HUNT_TYPE_ADULT_RED_DRAGON, HUNT_ABIL_FIRE_BREATH);  

    add_hunt(HUNT_TYPE_OLD_RED_DRAGON, 30, "old red dragon",  "A crown of cruel horns surrounds the head of this mighty dragon. Thick scales the color of molten rock cover its long body.", 
      "An old red dragon snorts flame from its nostrils.", CLASS_SORCERER, CHAOTIC_EVIL, RACE_TYPE_DRAGON, 
      SUBRACE_FIRE, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_GARGANTUAN );
    add_hunt_ability(HUNT_TYPE_OLD_RED_DRAGON, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_OLD_RED_DRAGON, HUNT_ABIL_FLIGHT);
    add_hunt_ability(HUNT_TYPE_OLD_RED_DRAGON, HUNT_ABIL_FIRE_BREATH);  

    add_hunt(HUNT_TYPE_YOUNG_BLUE_DRAGON, 20, "young blue dragon",  "With scales the color of the desert sky, this large, serpentine dragon moves with an unsettling grace.", 
      "A young blue dragon seethes lightning from its toothy maw.", CLASS_SORCERER, LAWFUL_EVIL, RACE_TYPE_DRAGON, 
      SUBRACE_AIR, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_LARGE );
    add_hunt_ability(HUNT_TYPE_YOUNG_BLUE_DRAGON, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_YOUNG_BLUE_DRAGON, HUNT_ABIL_FLIGHT);
    add_hunt_ability(HUNT_TYPE_YOUNG_BLUE_DRAGON, HUNT_ABIL_LIGHTNING_BREATH);  

    add_hunt(HUNT_TYPE_ADULT_BLUE_DRAGON, 25, "adult blue dragon",  "With scales the color of the desert sky, this large, serpentine dragon moves with an unsettling grace.", 
      "An adult blue dragon seethes lightning from its toothy maw.", CLASS_SORCERER, LAWFUL_EVIL, RACE_TYPE_DRAGON, 
      SUBRACE_AIR, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_HUGE );
    add_hunt_ability(HUNT_TYPE_ADULT_BLUE_DRAGON, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_ADULT_BLUE_DRAGON, HUNT_ABIL_FLIGHT);
    add_hunt_ability(HUNT_TYPE_ADULT_BLUE_DRAGON, HUNT_ABIL_LIGHTNING_BREATH);  

    add_hunt(HUNT_TYPE_OLD_BLUE_DRAGON, 30, "old blue dragon",  "With scales the color of the desert sky, this large, serpentine dragon moves with an unsettling grace.", 
      "An old blue dragon seethes lightning from its toothy maw.", CLASS_SORCERER, LAWFUL_EVIL, RACE_TYPE_DRAGON, 
      SUBRACE_AIR, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_GARGANTUAN );
    add_hunt_ability(HUNT_TYPE_OLD_BLUE_DRAGON, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_OLD_BLUE_DRAGON, HUNT_ABIL_FLIGHT);
    add_hunt_ability(HUNT_TYPE_OLD_BLUE_DRAGON, HUNT_ABIL_LIGHTNING_BREATH);  

    add_hunt(HUNT_TYPE_YOUNG_GREEN_DRAGON, 20, "young green dragon",  "Scales the color of emeralds armor this ferocious dragon. A single sharp horn protrudes from the end of its toothy snout.", 
      "A young green dragon drips hissing poison from its toothy maw.", CLASS_SORCERER, LAWFUL_EVIL, RACE_TYPE_DRAGON, 
      SUBRACE_EARTH, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_LARGE );
    add_hunt_ability(HUNT_TYPE_YOUNG_GREEN_DRAGON, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_YOUNG_GREEN_DRAGON, HUNT_ABIL_FLIGHT);
    add_hunt_ability(HUNT_TYPE_YOUNG_GREEN_DRAGON, HUNT_ABIL_POISON_BREATH);  

    add_hunt(HUNT_TYPE_ADULT_GREEN_DRAGON, 25, "adult green dragon",  "Scales the color of emeralds armor this ferocious dragon. A single sharp horn protrudes from the end of its toothy snout.", 
      "An adult green dragon drips hissing poison from its toothy maw.", CLASS_SORCERER, LAWFUL_EVIL, RACE_TYPE_DRAGON, 
      SUBRACE_EARTH, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_HUGE );
    add_hunt_ability(HUNT_TYPE_ADULT_GREEN_DRAGON, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_ADULT_GREEN_DRAGON, HUNT_ABIL_FLIGHT);
    add_hunt_ability(HUNT_TYPE_ADULT_GREEN_DRAGON, HUNT_ABIL_POISON_BREATH);  

    add_hunt(HUNT_TYPE_OLD_GREEN_DRAGON, 30, "old green dragon",  "Scales the color of emeralds armor this ferocious dragon. A single sharp horn protrudes from the end of its toothy snout.", 
      "An old green dragon drips hissing poison from its toothy maw.", CLASS_SORCERER, LAWFUL_EVIL, RACE_TYPE_DRAGON, 
      SUBRACE_EARTH, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_GARGANTUAN );
    add_hunt_ability(HUNT_TYPE_OLD_GREEN_DRAGON, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_OLD_GREEN_DRAGON, HUNT_ABIL_FLIGHT);
    add_hunt_ability(HUNT_TYPE_OLD_GREEN_DRAGON, HUNT_ABIL_POISON_BREATH);  

    add_hunt(HUNT_TYPE_YOUNG_WHITE_DRAGON, 20, "young white dragon",  "This dragon’s scales are a frosty white. Its head is crowned with slender horns, with a thin membrane stretched between them.", 
      "A young white dragon breathes frigid frost from its nostrils.", CLASS_SORCERER, CHAOTIC_EVIL, RACE_TYPE_DRAGON, 
      SUBRACE_COLD, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_LARGE );
    add_hunt_ability(HUNT_TYPE_YOUNG_WHITE_DRAGON, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_YOUNG_WHITE_DRAGON, HUNT_ABIL_FLIGHT);
    add_hunt_ability(HUNT_TYPE_YOUNG_WHITE_DRAGON, HUNT_ABIL_ACID_BREATH);  

    add_hunt(HUNT_TYPE_ADULT_WHITE_DRAGON, 25, "adult white dragon",  "This dragon’s scales are a frosty white. Its head is crowned with slender horns, with a thin membrane stretched between them.", 
      "An adult white dragon breathes frigid frost from its nostrils.", CLASS_SORCERER, CHAOTIC_EVIL, RACE_TYPE_DRAGON, 
      SUBRACE_COLD, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_HUGE );
    add_hunt_ability(HUNT_TYPE_ADULT_WHITE_DRAGON, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_ADULT_WHITE_DRAGON, HUNT_ABIL_FLIGHT);
    add_hunt_ability(HUNT_TYPE_ADULT_WHITE_DRAGON, HUNT_ABIL_ACID_BREATH);  

    add_hunt(HUNT_TYPE_OLD_WHITE_DRAGON, 30, "old white dragon",  "This dragon’s scales are a frosty white. Its head is crowned with slender horns, with a thin membrane stretched between them.", 
      "An old white dragon breathes frigid frost from its nostrils.", CLASS_SORCERER, CHAOTIC_EVIL, RACE_TYPE_DRAGON, 
      SUBRACE_COLD, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_GARGANTUAN );
    add_hunt_ability(HUNT_TYPE_OLD_WHITE_DRAGON, HUNT_ABIL_CAUSE_FEAR);      add_hunt_ability(HUNT_TYPE_OLD_WHITE_DRAGON, HUNT_ABIL_FLIGHT);
    add_hunt_ability(HUNT_TYPE_OLD_WHITE_DRAGON, HUNT_ABIL_ACID_BREATH);  

    add_hunt(HUNT_TYPE_DRAGON_TURTLE, 25, "dragon turtle",  "This long-tailed aquatic beast resembles a massive snapping turtle with draconic features.", 
      "A massive dragon turtle lumbers with thundering steps.", CLASS_WARRIOR, TRUE_NEUTRAL, RACE_TYPE_DRAGON, 
      SUBRACE_AQUATIC, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_HUGE );
    add_hunt_ability(HUNT_TYPE_DRAGON_TURTLE, HUNT_ABIL_SWALLOW);      add_hunt_ability(HUNT_TYPE_DRAGON_TURTLE, HUNT_ABIL_FIRE_BREATH);

    add_hunt(HUNT_TYPE_ROC, 25, "roc",  "This immense raptor unleashes a shrill cry as it bares its talons, each large enough to carry off a horse.", 
      "A massive roc, a huge flying bird of prey, sits majestically before you.", CLASS_WARRIOR, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_GARGANTUAN );
    add_hunt_ability(HUNT_TYPE_ROC, HUNT_ABIL_FLIGHT);      add_hunt_ability(HUNT_TYPE_ROC, HUNT_ABIL_GRAPPLE);

    add_hunt(HUNT_TYPE_PURPLE_WORM, 25, "purple worm",  "This enormous worm is covered with dark purple plates of chitinous armor. Its giant, tooth-filled mouth is the size of an ox.", 
      "A massive purple worm opens its huge maw, filled with razor sharp teeth.", CLASS_WARRIOR, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_GARGANTUAN );
    add_hunt_ability(HUNT_TYPE_PURPLE_WORM, HUNT_ABIL_SWALLOW);      add_hunt_ability(HUNT_TYPE_PURPLE_WORM, HUNT_ABIL_POISON);

    add_hunt(HUNT_TYPE_PYROHYDRA, 30, "pyrohydra",  "Multiple angry snake-like heads rise from the sleek, serpentine body of this terrifying monster.", 
      "A seven-headed, fire breathing pyrohydra threatens all in its sight.", CLASS_WARRIOR, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST, 
      SUBRACE_FIRE, SUBRACE_REPTILIAN, SUBRACE_UNKNOWN, SIZE_HUGE );
    add_hunt_ability(HUNT_TYPE_PYROHYDRA, HUNT_ABIL_REGENERATION);      add_hunt_ability(HUNT_TYPE_PYROHYDRA, HUNT_ABIL_FIRE_BREATH);

    add_hunt(HUNT_TYPE_BANDERSNATCH, 30, "bandersnatch",  "This six-limbed beast stalks forward with a fluid grace. Barbed quills run along its back, and its eyes glow with a blue light.", 
      "A massive six-limbed bandersnatch emanates waves of fear.", CLASS_WARRIOR, TRUE_NEUTRAL, RACE_TYPE_MAGICAL_BEAST, 
      SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_GARGANTUAN );
    add_hunt_ability(HUNT_TYPE_BANDERSNATCH, HUNT_ABIL_TAIL_SPIKES);      add_hunt_ability(HUNT_TYPE_BANDERSNATCH, HUNT_ABIL_CHARM);
    add_hunt_ability(HUNT_TYPE_BANDERSNATCH, HUNT_ABIL_REGENERATION);      add_hunt_ability(HUNT_TYPE_BANDERSNATCH, HUNT_ABIL_PARALYZE);

    add_hunt(HUNT_TYPE_BANSHEE, 30, "banshee",  "This beautiful, ghostly elven woman glides through the air, her long hair flowing around a face knotted into a mask of rage.", 
      "A beautiful, ghostly banshee wails with horrendous pain and malice.", CLASS_SORCERER, CHAOTIC_EVIL, RACE_TYPE_UNDEAD, 
      SUBRACE_INCORPOREAL, SUBRACE_UNKNOWN, SUBRACE_UNKNOWN, SIZE_MEDIUM );
    add_hunt_ability(HUNT_TYPE_BANSHEE, HUNT_ABIL_LEVEL_DRAIN);      add_hunt_ability(HUNT_TYPE_BANSHEE, HUNT_ABIL_CAUSE_FEAR);

}

void create_hunts(void)
{

    int i = 0, j = 0;
    int which_hunt = 0;
    struct char_data *ch = NULL;

    // let's reset the timer.  This timer is used only to show players/staff how long before the next reset
    hunt_reset_timer = 1200;

    // let's erase existing active hunt table
    for (i = 0; i < AHUNT_1; i++)
    {
        for (j = 0; j < AHUNT_2; j++)
        {
            active_hunts[i][j] = 0;
        }
    }

    // now let's set a cooldown timer of 5 minutes on any existing hunt mobs

    for (ch = character_list; ch; ch = ch->next)
    {
      if (!IS_NPC(ch)) continue;
      if (!MOB_FLAGGED(ch, MOB_HUNTS_TARGET)) continue;
      ch->mob_specials.hunt_cooldown = 50;
    }

    // now let's load new hunts

    for (i = 0; i < AHUNT_1; i++)
    {
        which_hunt = select_a_hunt(((i*5)+10));
        if (which_hunt == NUM_HUNT_TYPES) continue;
        active_hunts[i][0] = which_hunt;
        select_hunt_coords(i);
    }
}

void check_hunt_room(room_rnum room)
{
  if (room == NOWHERE) return;

  int x = 0, y = 0, i = 0;

  x = world[room].coords[0];
  y = world[room].coords[1];

  for (i = 0; i < AHUNT_1; i++)
  {
    if (active_hunts[i][3] == x && active_hunts[i][4] == y)
    {
      create_hunt_mob(room, active_hunts[i][0]);
      return;
    }
  }

}

void create_hunt_mob(room_rnum room, int which_hunt)
{
  if (room == NOWHERE) return;

  struct char_data *mob = read_mobile(HUNTS_MOB_VNUM, VIRTUAL);
  char mob_descs[1000];
  int i = 0;

  if (!mob) {
    //send_to_char(ch, "Mob load error.\r\n");
    return;
  }

  if (is_hunt_mob_in_room(room, which_hunt))
  {
    send_to_room(room, "\tYYou've come across a hunt target!\tn\r\n");
    return;
  }

  // set descriptions
  mob->player.name = strdup(hunt_table[which_hunt].name);

  sprintf(mob_descs, "\tn%s %s", AN(hunt_table[which_hunt].name), hunt_table[which_hunt].name);
  for (i = 0; i < strlen(mob_descs); i++)
    mob_descs[i] = tolower(mob_descs[i]);
  mob->player.short_descr = strdup(mob_descs);

  if (!strcmp(hunt_table[which_hunt].long_description, "Nothing")) {
    sprintf(mob_descs, "\tn%s %s is here.\r\n", AN(hunt_table[which_hunt].name), hunt_table[which_hunt].name);
    mob->player.long_descr = strdup(mob_descs);
  } else {
    sprintf(mob_descs, "\tn%s\r\n", hunt_table[which_hunt].long_description);
    mob->player.long_descr = strdup(mob_descs);
  }

  if (!strcmp(hunt_table[which_hunt].description, "Nothing")) {
    sprintf(mob_descs, "\tn%s %s is here before you.\r\n", AN(hunt_table[which_hunt].name), hunt_table[which_hunt].name);
    mob->player.description = strdup(mob_descs);    
  } else {
    sprintf(mob_descs, "\tn%s\r\n", hunt_table[which_hunt].description);
    mob->player.description = strdup(mob_descs);
  }

  // set mob details
  GET_REAL_RACE(mob) = hunt_table[which_hunt].race_type;
  GET_SUBRACE(mob, 0) - hunt_table[which_hunt].subrace[0];
  GET_SUBRACE(mob, 1) - hunt_table[which_hunt].subrace[1];
  GET_SUBRACE(mob, 2) - hunt_table[which_hunt].subrace[2];
  mob->mob_specials.hunt_cooldown = -1;
  mob->mob_specials.hunt_type = which_hunt;
  
  // set stats
  GET_CLASS(mob) = hunt_table[which_hunt].char_class;
  GET_LEVEL(mob) = hunt_table[which_hunt].level;
  autoroll_mob(mob, TRUE, FALSE);
  GET_EXP(mob) = (GET_LEVEL(mob) * GET_LEVEL(mob) * 500);
  GET_GOLD(mob) = (GET_LEVEL(mob) * 100);
  set_alignment(mob, hunt_table[which_hunt].alignment);
  GET_REAL_MAX_HIT(mob) = GET_REAL_MAX_HIT(mob) * 7.5;
  GET_MAX_HIT(mob) = GET_REAL_MAX_HIT(mob);
	GET_HIT(mob) = GET_MAX_HIT(mob);
  GET_HITROLL(mob) += GET_LEVEL(mob) / 5;
  GET_DAMROLL(mob) += GET_LEVEL(mob) / 5;
  mob->points.armor += (int) (GET_LEVEL(mob) * 3.5);
  mob->points.size = hunt_table[which_hunt].size;
  // set flags
  SET_BIT_AR(MOB_FLAGS(mob), MOB_HUNTS_TARGET);
  SET_BIT_AR(MOB_FLAGS(mob), MOB_SENTINEL);

  X_LOC(mob) = world[room].coords[0];
  Y_LOC(mob) = world[room].coords[1];
  char_to_room(mob, room);
  
  send_to_room(room, "\tYYou've come across a hunt target: %s!\tn\r\n", mob->player.short_descr);
}

void select_hunt_coords(int which_hunt)
{
    int x = 0, y = 0;
    int terrain = 0;
    room_rnum room = NOWHERE;

    x = dice(1, 1301) - 651;
    y = dice(1, 1301) - 651;
    room = find_room_by_coordinates(x, y);
    if (room == NOWHERE)
      room = find_available_wilderness_room();
    if (room == NOWHERE)
      return;

    assign_wilderness_room(room, x, y);
    
    terrain = world[room].sector_type;

    switch (terrain)
    {
        case SECT_OCEAN:
        case SECT_UD_NOSWIM:
        case SECT_UD_WATER:
        case SECT_UNDERWATER:
        case SECT_WATER_NOSWIM:
        case SECT_WATER_SWIM:
        case SECT_INSIDE:
        case SECT_INSIDE_ROOM:
          select_hunt_coords(which_hunt);
          return;
    }

    active_hunts[which_hunt][1] = x;
    active_hunts[which_hunt][2] = y;

    select_reported_hunt_coords(which_hunt, 0);
}

void select_reported_hunt_coords(int which_hunt, int times_called)
{

    times_called++;

    if (times_called >= 20)
    {
        select_hunt_coords(which_hunt);
        return;
    }

    int x = 0, y = 0;
    int terrain = 0;
    room_rnum room = NOWHERE;

    x = dice(1, 11) - 6;
    y = dice(1, 11) - 6;
    
    x += active_hunts[which_hunt][1];
    y += active_hunts[which_hunt][2];

    room = find_room_by_coordinates(x, y);
    if (room == NOWHERE)
      room = find_available_wilderness_room();
    if (room == NOWHERE)
      return;

    assign_wilderness_room(room, x, y);
    
    terrain = world[room].sector_type;

    switch (terrain)
    {
        case SECT_OCEAN:
        case SECT_UD_NOSWIM:
        case SECT_UD_WATER:
        case SECT_UNDERWATER:
        case SECT_WATER_NOSWIM:
        case SECT_WATER_SWIM:
          select_reported_hunt_coords(which_hunt, times_called);
          return;
    }
    active_hunts[which_hunt][3] = x;
    active_hunts[which_hunt][4] = y;
}

int select_a_hunt(int level)
{
    int i = 0;
    int count = 0;
    int roll = 0;
    int which_hunt = NUM_HUNT_TYPES;

    for (i = 0; i < NUM_HUNT_TYPES; i++)
    {
        if (hunt_table[i].level == level)
          count++;
    }

    roll = dice(1, count);
    count = 0;

    for (i = 0; i < NUM_HUNT_TYPES; i++)
    {
        if (hunt_table[i].level == level)
        {
            count++;
            if (roll == count)
            {
                which_hunt = i;
                break;
            }
        }
    }

    return which_hunt;
}

void list_hunt_rewards(struct char_data *ch, int type)
{
    int i = 0;
    struct obj_data *obj = NULL;
   if (type == HUNT_REWARD_TYPE_TRINKET)
   {
     send_to_char(ch,
          "%-30s - %s\r\n"
          "----------------------------------------------\r\n", "Component Type (x10)", "Effect");
     for (i = 1; i < NUM_HUNT_TYPES; i++)
     {
       if (i >= HUNT_TYPE_YOUNG_RED_DRAGON && i <= HUNT_TYPE_OLD_WHITE_DRAGON) continue;
       obj = read_object(get_hunt_armor_drop_vnum(i), VIRTUAL);
       if (obj)
        send_to_char(ch, "%-30s - %s\r\n", obj->short_description, affected_bits[hunts_special_armor_type(i)]);
     }
     send_to_char(ch, "%-30s - %s\r\n", "dragon bone", "crafting material");
     send_to_char(ch, "%-30s - %s\r\n", "dragon hide", "crafting material");
     send_to_char(ch, "%-30s - %s\r\n", "dragon scale", "crafting material");
   }
   else
   {
     send_to_char(ch,
          "%-30s - %s\r\n"
          "----------------------------------------------\r\n", "Component Type (x10)", "Effect");
     for (i = 1; i < NUM_HUNT_TYPES; i++)
     {
       if (i >= HUNT_TYPE_YOUNG_BLUE_DRAGON && i <= HUNT_TYPE_OLD_WHITE_DRAGON) continue;
       obj = read_object(get_hunt_weapon_drop_vnum(i), VIRTUAL);
       if (obj)
        send_to_char(ch, "%-30s - %s\r\n", obj->short_description, special_ability_info[hunts_special_weapon_type(i)].name);
     }
   }
}

SPECIAL(huntsmaster)
{
  if (!CMD_IS("hunts") && !CMD_IS("list") && !CMD_IS("exchange")) return 0;

  int i = 0;
  char reported[20], actual[20];
  char objdesc[200], subdesc[50];
  int hours = 0, minutes = 0, seconds = 0;
  char arg1[200], arg2[200];
  struct obj_data *obj1 = NULL, *obj2 = NULL, *obj3 = NULL, *next_obj = NULL;
  bool trinket = false;
  int count = 0, wear_slot = 0;

  half_chop(argument, arg1, arg2);

  if (CMD_IS("hunts")) {
    send_to_char(ch, "%-20s %-5s %-16s %s\r\n", "Hunt Target", "Level", "Last Seen Coords", (GET_LEVEL(ch) >= LVL_IMMORT) ? "Actual Coords" : "");
    send_to_char(ch, "---------------------------------------------------------\r\n");

    for (i = 0; i < 5; i++)
    {
      if (active_hunts[i][0] == 0) continue; // hunt already defeated
      count++;
      snprintf(reported, sizeof(reported), "%4d, %-4d", active_hunts[i][1], active_hunts[i][2]);
      snprintf(actual, sizeof(actual), "%4d, %-d", active_hunts[i][3], active_hunts[i][4]);
      send_to_char(ch, "%-20s %-5d %-16s %s\r\n", hunt_table[active_hunts[i][0]].name, hunt_table[active_hunts[i][0]].level, reported, (GET_LEVEL(ch) >= LVL_IMMORT) ? actual : "");
    }

    hours = hunt_reset_timer / 600;
    minutes = (hunt_reset_timer - (hours * 600)) / 10;
    seconds = hunt_reset_timer - ((hours * 600) + (minutes * 10));

    send_to_char(ch, "\r\nTime until next hunt targets: %d hour(s), %d minute(s), %d seconds", hours, minutes, seconds * 6 );

    if (count == 0)
    {
      send_to_char(ch, "There are no active hunts right now.\r\n");
      return 1;
    }

  }
  else if (CMD_IS("list"))
  {
    if (!*arg1)
    {
      send_to_char(ch, "Would you like to list 'trinket' rewards, or 'weapon' rewards?\r\n"
                       "Ie. list trinkets or list weapons\r\n");
    }
    else if (is_abbrev(arg1, "trinkets"))
    {
      list_hunt_rewards(ch, HUNT_REWARD_TYPE_TRINKET);
    }
    else if (is_abbrev(arg1, "weapons"))
    {
      list_hunt_rewards(ch, HUNT_REWARD_TYPE_WEAPON_OIL);
    }
    else
    {
      send_to_char(ch, "Would you like to list 'trinket' rewards, or 'weapon' rewards?\r\n"
                       "Ie. list trinkets or list weapons\r\n");
    }
    return 1;
  }
  else if (CMD_IS("exchange"))
  {
    if (!*arg1)
    {
      send_to_char(ch, "You need to specify the hunt trophies you want to exchange.  You'll need ten of them to complete the exchange.\r\n");
      return 1;
    }
    if (!(obj1 = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
    {
      send_to_char(ch, "You do not have anything by that description in your inventory.\r\n");
      return 1;
    }
    if (GET_OBJ_TYPE(obj1) != ITEM_HUNT_TROPHY)
    {
      send_to_char(ch, "That is not an hunt trophy.\r\n");
      return 1;
    }
    for (obj2 = ch->carrying; obj2; obj2 = obj2->next_content)
    {
     if (GET_OBJ_VNUM(obj2) == GET_OBJ_VNUM(obj1)) count++;
    }
    if (count < 10)
    {
      send_to_char(ch, "You need at least 10 of '%s' in your inventory to make the exchange.\r\n", obj1->short_description);
      return 1;
    }
    if (GET_OBJ_VAL(obj1, 0) == 0)
      GET_OBJ_VAL(obj1, 0) = obj_vnum_to_hunt_type(GET_OBJ_VNUM(obj1));
    trinket = is_hunt_trophy_a_trinket(GET_OBJ_VNUM(obj1));
    if (trinket)
    {

      // we want to find out what wear slot the item will be
      if (!*arg2)
      {
        send_to_char(ch, "You need to specify whether you want a ring, a pendant or a bracer with the %s enchantment.\r\n",
                     affected_bits[hunts_special_armor_type(GET_OBJ_VAL(obj1, 0))]);
        return 1;
      }
      if (is_abbrev(arg2, "ring"))
      {
        snprintf(subdesc, sizeof(subdesc), "ring");
        wear_slot = ITEM_WEAR_FINGER;
      }
      else if (is_abbrev(arg2, "pendant"))
      {
        snprintf(subdesc, sizeof(subdesc), "pendant");
        wear_slot = ITEM_WEAR_NECK;
      }
      else if (is_abbrev(arg2, "bracer"))
      {
        snprintf(subdesc, sizeof(subdesc), "bracer");
        wear_slot = ITEM_WEAR_WRIST;
      }
      else
      {
        send_to_char(ch, "You need to specify whether you want a ring, a pendant or a bracer with the %s enchantment.\r\n",
                     affected_bits[hunts_special_armor_type(GET_OBJ_VAL(obj1, 0))]);
        return 1;
      }

      // get the base item.  We'll be modifying it below
      obj2 = read_object(HUNTS_REWARD_ITEM_VNUM, VIRTUAL);
      if (!obj2)
      {
        send_to_char(ch, "There was an issue.  Please inform staff using error code HUNTEXCHANGE001.\r\n");
        return 1;
      }

      // set item descriptions
      snprintf(objdesc, sizeof(objdesc), "%s %s", subdesc, affected_bits[hunts_special_armor_type(GET_OBJ_VAL(obj1, 0))]);
      for (i = 0; i < sizeof(objdesc); i++)
      {
        objdesc[i] = tolower(objdesc[i]);
      }
      obj2->name = strdup(objdesc);
      snprintf(objdesc, sizeof(objdesc), "%s %s of %s", AN(subdesc), subdesc, affected_bits[hunts_special_armor_type(GET_OBJ_VAL(obj1, 0))]);
      for (i = 0; i < sizeof(objdesc); i++)
      {
        objdesc[i] = tolower(objdesc[i]);
      }
      obj2->short_description = strdup(objdesc);
      snprintf(objdesc, sizeof(objdesc), "%s %s of %s lies here.", AN(subdesc), subdesc, affected_bits[hunts_special_armor_type(GET_OBJ_VAL(obj1, 0))]);
      for (i = 0; i < sizeof(objdesc); i++)
      {
        objdesc[i] = tolower(objdesc[i]);
      }
      objdesc[0] = toupper(objdesc[0]);
      obj2->description = strdup(objdesc);
      
      // set wear slot
    SET_BIT_AR(GET_OBJ_WEAR(obj2), wear_slot);
    // set minimum wear level
    GET_OBJ_LEVEL(obj2) = hunt_table[GET_OBJ_VAL(obj1, 0)].level - 5;
    // set item material
    GET_OBJ_MATERIAL(obj2) = MATERIAL_GOLD;
    // and finally set the affect on the item
    SET_BIT_AR(GET_OBJ_AFFECT(obj2), hunts_special_armor_type(GET_OBJ_VAL(obj1, 0)));
    }
    else
    {
     // get the base item.  We'll be modifying it below
      obj2 = read_object(HUNTS_REWARD_ITEM_VNUM, VIRTUAL);
      if (!obj2)
      {
        send_to_char(ch, "There was an issue.  Please inform staff using error code HUNTEXCHANGE002.\r\n");
        return 1;
      }

      // set item descriptions
      snprintf(objdesc, sizeof(objdesc), "vial weapon oil %s", special_ability_info[hunts_special_weapon_type(GET_OBJ_VAL(obj1, 0))].name);
      for (i = 0; i < sizeof(objdesc); i++)
      {
        objdesc[i] = tolower(objdesc[i]);
      }
      obj2->name = strdup(objdesc);
      snprintf(objdesc, sizeof(objdesc), "a vial of -%s- weapon oil", special_ability_info[hunts_special_weapon_type(GET_OBJ_VAL(obj1, 0))].name);
      for (i = 0; i < sizeof(objdesc); i++)
      {
        objdesc[i] = tolower(objdesc[i]);
      }
      obj2->short_description = strdup(objdesc);
      snprintf(objdesc, sizeof(objdesc), "A vial of -%s- weapon oil lies here.", special_ability_info[hunts_special_weapon_type(GET_OBJ_VAL(obj1, 0))].name);
      for (i = 0; i < sizeof(objdesc); i++)
      {
        objdesc[i] = tolower(objdesc[i]);
      }
      objdesc[0] = toupper(objdesc[0]);
      obj2->description = strdup(objdesc);

      GET_OBJ_TYPE(obj2) = ITEM_WEAPON_OIL;
      GET_OBJ_MATERIAL(obj2) = MATERIAL_GLASS;
      GET_OBJ_VAL(obj2, 0) = hunts_special_weapon_type(GET_OBJ_VAL(obj1, 0));
      GET_OBJ_LEVEL(obj2) = 1;
      GET_OBJ_COST(obj2) = 100000;
    }
    if (!obj2)
    {
      send_to_char(ch, "There was an issue.  Please inform staff using error code HUNTEXCHANGE002.\r\n");
      return 1;
    }
    obj_to_char(obj2, ch);
    send_to_char(ch, "You exchange 10x %s for your reward.\r\n", obj1->short_description);
    send_to_char(ch, "You receive %s as a reward for your hunts.\r\n", obj2->short_description);
    // let's extract the hunt items now
    count = 0;
    for (obj3 = ch->carrying; obj3; obj3 = next_obj)
    {
      next_obj = obj3->next_content;
      if (count >= 10) break;
     if (GET_OBJ_VNUM(obj3) == GET_OBJ_VNUM(obj1))
     {
       count++;
       obj_from_char(obj3);
       //extract_obj(obj3); // extracting it killed the loop.  Should fix this!
     }
    }
    return 1;
  }
  return 1;
}

void award_hunt_materials(struct char_data *ch, int which_hunt)
{
  struct obj_data *obj1 = NULL, *obj2 = NULL;

  obj1 = read_object(get_hunt_armor_drop_vnum(which_hunt), VIRTUAL);
  obj2 = read_object(get_hunt_weapon_drop_vnum(which_hunt), VIRTUAL);
  
  if (obj1) {
    GET_OBJ_VAL(obj1, 0) = which_hunt;
    obj_to_char(obj1, ch);
    act("You harvest $p from the creature's remains.", true, ch, obj1, 0, TO_CHAR);
    act("$n harvests $p from the creature's remains.", true, ch, obj1, 0, TO_ROOM);
  }

  if (obj2) {
    GET_OBJ_VAL(obj2, 0) = which_hunt;
    obj_to_char(obj2, ch);
    act("You harvest $p from the creature's remains.", true, ch, obj2, 0, TO_CHAR);
    act("$n harvests $p from the creature's remains.", true, ch, obj2, 0, TO_ROOM);
  }

}

void drop_hunt_mob_rewards(struct char_data *ch, struct char_data *hunt_mob)
{
  if (!ch || ! hunt_mob) return;

  if (IN_ROOM(ch) == NOWHERE || IN_ROOM(hunt_mob) == NOWHERE) return;
  
  if (IS_NPC(ch))
  { 
    // if the mob has a master, award the master
    // if the master is in a different room or the mob has no master, award the mob
    if (ch->master && IN_ROOM(ch) == IN_ROOM(ch->master))
    {
      drop_hunt_mob_rewards(ch->master, hunt_mob);
      return;
    }
  }
  

  struct char_data *tch = NULL;
  struct char_data *master = NULL;

  if (ch->master) master = ch->master;
  else master = ch;

  // only the party leader will get the hunt-specific drops
  award_hunt_materials(master, hunt_mob->mob_specials.hunt_type);

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
  {
    if (IS_NPC(tch)) continue;
    if (is_player_grouped(tch, ch))
    {
      GET_QUESTPOINTS(tch) += GET_LEVEL(hunt_mob) * 20;
      GET_GOLD(tch) += GET_LEVEL(hunt_mob) * 500;
      send_to_char(tch, "You've been rewarded %d quest points, and %d gold!\r\n", GET_LEVEL(hunt_mob) * 20, GET_LEVEL(hunt_mob) * 500);
      
    }
  }

}

int hunts_special_weapon_type(int hunt_record)
{
  switch (hunt_record)
  {
    case  HUNT_TYPE_BASILISK:
      return WEAPON_SPECAB_SEEKING;
    case  HUNT_TYPE_MANTICORE:
      return WEAPON_SPECAB_ADAPTIVE;
    case  HUNT_TYPE_WRAITH:
      return WEAPON_SPECAB_DISRUPTION;
    case  HUNT_TYPE_SIREN:
      return WEAPON_SPECAB_DEFENDING;
    case  HUNT_TYPE_WILL_O_WISP:
      return WEAPON_SPECAB_EXHAUSTING;
    case  HUNT_TYPE_BLACK_PUDDING:
      return WEAPON_SPECAB_CORROSIVE;
    case  HUNT_TYPE_CHIMERA:
      return WEAPON_SPECAB_SPEED;
    case  HUNT_TYPE_GHOST:
      return WEAPON_SPECAB_GHOST_TOUCH;
    case  HUNT_TYPE_MEDUSA:
      return WEAPON_SPECAB_BLINDING;
    case  HUNT_TYPE_BEHIR:
      return WEAPON_SPECAB_SHOCK;
    case  HUNT_TYPE_EFREETI:
      return WEAPON_SPECAB_FLAMING;
    case  HUNT_TYPE_DJINNI:
      return WEAPON_SPECAB_THUNDERING;
    case  HUNT_TYPE_YOUNG_RED_DRAGON:
    case  HUNT_TYPE_YOUNG_BLUE_DRAGON:
    case  HUNT_TYPE_YOUNG_GREEN_DRAGON:
    case  HUNT_TYPE_YOUNG_BLACK_DRAGON:
    case  HUNT_TYPE_YOUNG_WHITE_DRAGON:
      return WEAPON_SPECAB_AGILE;
    case  HUNT_TYPE_ADULT_RED_DRAGON:
    case  HUNT_TYPE_ADULT_BLUE_DRAGON:
    case  HUNT_TYPE_ADULT_BLACK_DRAGON:
    case  HUNT_TYPE_ADULT_WHITE_DRAGON:
    case  HUNT_TYPE_ADULT_GREEN_DRAGON:
      return WEAPON_SPECAB_WOUNDING;
    case  HUNT_TYPE_OLD_RED_DRAGON:
    case  HUNT_TYPE_OLD_BLUE_DRAGON:
    case  HUNT_TYPE_OLD_GREEN_DRAGON:
    case  HUNT_TYPE_OLD_BLACK_DRAGON:
    case  HUNT_TYPE_OLD_WHITE_DRAGON:
      return WEAPON_SPECAB_LUCKY;
    case  HUNT_TYPE_DRAGON_TURTLE:
      return WEAPON_SPECAB_BEWILDERING;
    case  HUNT_TYPE_ROC:
      return WEAPON_SPECAB_KEEN;
    case  HUNT_TYPE_PURPLE_WORM:
      return WEAPON_SPECAB_VICIOUS;
    case  HUNT_TYPE_PYROHYDRA:
      return WEAPON_SPECAB_INVIGORATING;
    case  HUNT_TYPE_BANDERSNATCH:
      return WEAPON_SPECAB_VORPAL;
    case  HUNT_TYPE_BANSHEE:
      return WEAPON_SPECAB_VAMPIRIC;
    case  HUNT_TYPE_BARGHEST:
    default:
      return WEAPON_SPECAB_BANE;
  }
  return 0;
}

int hunts_special_armor_type(int hunt_record)
{
  switch (hunt_record)
  {
    case  HUNT_TYPE_BASILISK:
      return AFF_FREE_MOVEMENT;  
    case  HUNT_TYPE_MANTICORE:
      return AFF_INFRAVISION;
    case  HUNT_TYPE_WRAITH:
      return AFF_SENSE_LIFE;
    case  HUNT_TYPE_SIREN:
      return AFF_WATERWALK;
    case  HUNT_TYPE_WILL_O_WISP:
      return AFF_INVISIBLE;
    case  HUNT_TYPE_BLACK_PUDDING:
      return AFF_DEATH_WARD;
    case  HUNT_TYPE_CHIMERA:
      return AFF_DETECT_ALIGN;
    case  HUNT_TYPE_GHOST:
      return AFF_DETECT_INVIS;
    case  HUNT_TYPE_MEDUSA:
      return AFF_ULTRAVISION;
    case  HUNT_TYPE_BEHIR:
      return AFF_ESHIELD;
    case  HUNT_TYPE_EFREETI:
      return AFF_FSHIELD;
    case  HUNT_TYPE_DJINNI:
      return AFF_HASTE;      
    case  HUNT_TYPE_DRAGON_TURTLE:
      return AFF_WATER_BREATH;
    case  HUNT_TYPE_ROC:
      return AFF_FLYING;      
    case  HUNT_TYPE_PURPLE_WORM:
      return AFF_DANGERSENSE;
    case  HUNT_TYPE_PYROHYDRA:
      return AFF_REGEN;
    case  HUNT_TYPE_BANDERSNATCH:
      return AFF_MINOR_GLOBE;
    case  HUNT_TYPE_BANSHEE:
      return AFF_BLUR;
    case  HUNT_TYPE_BARGHEST:
      return AFF_BLINKING;
    default:
      return 0;
  }
  return 0;
}

int get_hunt_armor_drop_vnum(int hunt_record)
{
 switch (hunt_record)
  {
    case  HUNT_TYPE_BASILISK:
      return 60000;
    case  HUNT_TYPE_MANTICORE:
      return 60002;
    case  HUNT_TYPE_WRAITH:
      return 60004;
    case  HUNT_TYPE_SIREN:
      return 60006;
    case  HUNT_TYPE_WILL_O_WISP:
      return 60008;
    case  HUNT_TYPE_BARGHEST:
      return 60010;
    case  HUNT_TYPE_BLACK_PUDDING:
      return 60012;
    case  HUNT_TYPE_CHIMERA:
      return 60014;
    case  HUNT_TYPE_GHOST:
      return 60016;
    case  HUNT_TYPE_MEDUSA:
      return 60018;
    case  HUNT_TYPE_BEHIR:
      return 60020;
    case  HUNT_TYPE_EFREETI:
      return 60022;
    case  HUNT_TYPE_DJINNI:
      return 60024;
    case  HUNT_TYPE_YOUNG_RED_DRAGON:
    case  HUNT_TYPE_YOUNG_BLUE_DRAGON:
    case  HUNT_TYPE_YOUNG_GREEN_DRAGON:
    case  HUNT_TYPE_YOUNG_BLACK_DRAGON:
    case  HUNT_TYPE_YOUNG_WHITE_DRAGON:
    case  HUNT_TYPE_ADULT_RED_DRAGON:
    case  HUNT_TYPE_ADULT_BLUE_DRAGON:
    case  HUNT_TYPE_ADULT_BLACK_DRAGON:
    case  HUNT_TYPE_ADULT_WHITE_DRAGON:
    case  HUNT_TYPE_ADULT_GREEN_DRAGON:
    case  HUNT_TYPE_OLD_RED_DRAGON:
    case  HUNT_TYPE_OLD_BLUE_DRAGON:
    case  HUNT_TYPE_OLD_GREEN_DRAGON:
    case  HUNT_TYPE_OLD_BLACK_DRAGON:
    case  HUNT_TYPE_OLD_WHITE_DRAGON:
      switch(dice(1, 3))
      {
        case 1:  return 60026; // dragonhide
        case 2:  return 60027; // dagonscale
        default: return 60028; // dragonbone
      }
    case  HUNT_TYPE_DRAGON_TURTLE:
      return 60040;
    case  HUNT_TYPE_ROC:
      return 60029;
    case  HUNT_TYPE_PURPLE_WORM:
      return 60031;
    case  HUNT_TYPE_PYROHYDRA:
      return 60033;
    case  HUNT_TYPE_BANDERSNATCH:
      return 60035;
    case  HUNT_TYPE_BANSHEE:
      return 60037;
    default:
      return 0;
  }
  return 0; 
}

int get_hunt_weapon_drop_vnum(int hunt_record)
{
 switch (hunt_record)
  {
    case  HUNT_TYPE_BASILISK:
      return 60001;
    case  HUNT_TYPE_MANTICORE:
      return 60003;
    case  HUNT_TYPE_WRAITH:
      return 60005;
    case  HUNT_TYPE_SIREN:
      return 60007;
    case  HUNT_TYPE_WILL_O_WISP:
      return 60009;
    case  HUNT_TYPE_BARGHEST:
      return 60011;
    case  HUNT_TYPE_BLACK_PUDDING:
      return 60013;
    case  HUNT_TYPE_CHIMERA:
      return 60015;
    case  HUNT_TYPE_GHOST:
      return 60017;
    case  HUNT_TYPE_MEDUSA:
      return 60019;
    case  HUNT_TYPE_BEHIR:
      return 60021;
    case  HUNT_TYPE_EFREETI:
      return 60023;
    case  HUNT_TYPE_DJINNI:
      return 60025;
    case  HUNT_TYPE_YOUNG_RED_DRAGON:
    case  HUNT_TYPE_YOUNG_BLUE_DRAGON:
    case  HUNT_TYPE_YOUNG_GREEN_DRAGON:
    case  HUNT_TYPE_YOUNG_BLACK_DRAGON:
    case  HUNT_TYPE_YOUNG_WHITE_DRAGON:
      return 60039;
    case  HUNT_TYPE_ADULT_RED_DRAGON:
    case  HUNT_TYPE_ADULT_BLUE_DRAGON:
    case  HUNT_TYPE_ADULT_BLACK_DRAGON:
    case  HUNT_TYPE_ADULT_WHITE_DRAGON:
    case  HUNT_TYPE_ADULT_GREEN_DRAGON:
      return 60041;
    case  HUNT_TYPE_OLD_RED_DRAGON:
    case  HUNT_TYPE_OLD_BLUE_DRAGON:
    case  HUNT_TYPE_OLD_GREEN_DRAGON:
    case  HUNT_TYPE_OLD_BLACK_DRAGON:
    case  HUNT_TYPE_OLD_WHITE_DRAGON:
      return 60042;
    case  HUNT_TYPE_DRAGON_TURTLE:
      return 60043;
    case  HUNT_TYPE_ROC:
      return 60030;
    case  HUNT_TYPE_PURPLE_WORM:
      return 60032;
    case  HUNT_TYPE_PYROHYDRA:
      return 60034;
    case  HUNT_TYPE_BANDERSNATCH:
      return 60036;
    case  HUNT_TYPE_BANSHEE:
      return 60038;
    default:
      return 0;
  }
  return 0; 
}

bool is_hunt_trophy_a_trinket(int vnum)
{
  switch (vnum)
  {
    case 60000:
    case 60002:
    case 60004:
    case 60006:
    case 60008:
    case 60010:
    case 60012:
    case 60014:
    case 60016:
    case 60018:
    case 60020:
    case 60022:
    case 60024:
    case 60040:
    case 60029:
    case 60031:
    case 60033:
    case 60035:
    case 60037:
      return true;
  }
  return false;
}

void remove_hunts_mob(int which_hunt)
{
  int i = 0, j = 0;

  for (i = 0; i < AHUNT_1; i++)
  {
      if (active_hunts[i][0] == which_hunt)
      {
        for (j = 0; j < AHUNT_2; j++)
          active_hunts[i][j] = 0;
      }
  }
}

bool is_hunt_mob_in_room(room_rnum room, int which_hunt)
{
  if (room == NOWHERE) return false;

  struct char_data *tch = NULL;

  for (tch = world[room].people; tch; tch = tch->next_in_room)
  {
    if (!IS_NPC(tch) || !MOB_FLAGGED(tch, MOB_HUNTS_TARGET)) continue;
    if (tch->mob_specials.hunt_type == which_hunt) return true;
  }
  return false;
}

// true means the description goes before : a vorpal long sword
// false means it goes after: a light mace of disruption
bool weapon_specab_desc_position(int specab)
{
  switch (specab)
  {
    case WEAPON_SPECAB_SEEKING: return false;
    case WEAPON_SPECAB_ADAPTIVE: return true;
    case WEAPON_SPECAB_DISRUPTION: return false;
    case WEAPON_SPECAB_DEFENDING: return true;
    case WEAPON_SPECAB_EXHAUSTING: return false;
    case WEAPON_SPECAB_CORROSIVE: return true;
    case WEAPON_SPECAB_SPEED: return false;
    case WEAPON_SPECAB_GHOST_TOUCH: return true;
    case WEAPON_SPECAB_BLINDING: return false;
    case WEAPON_SPECAB_SHOCK: return true;
    case WEAPON_SPECAB_FLAMING: return true;
    case WEAPON_SPECAB_THUNDERING: return false;
    case WEAPON_SPECAB_AGILE: return true;
    case WEAPON_SPECAB_WOUNDING: return false;
    case WEAPON_SPECAB_LUCKY: return true;
    case WEAPON_SPECAB_BEWILDERING: return false;
    case WEAPON_SPECAB_KEEN: return true;
    case WEAPON_SPECAB_VICIOUS: return true;
    case WEAPON_SPECAB_INVIGORATING: return true;
    case WEAPON_SPECAB_VORPAL: return true;
    case WEAPON_SPECAB_VAMPIRIC: return true;
    case WEAPON_SPECAB_BANE: return false;
  }

  return false;
}

bool is_weapon_specab_compatible(struct char_data *ch, int weapon_type, int specab, bool output)
{
  switch (specab)
  {
    case WEAPON_SPECAB_SEEKING:
    case WEAPON_SPECAB_ADAPTIVE:
      if (IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_RANGED))
        return true;
      if (output)
        send_to_char(ch, "This weapon ability can only be added to ranged weapons.\r\n");
      return false;
    case WEAPON_SPECAB_VORPAL:
      if (weapon_list[weapon_type].damageTypes == DAMAGE_TYPE_SLASHING &&
          !IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_RANGED))
        return true;
      if (output)
        send_to_char(ch, "This weapon ability can only be added to slashing melee weapons.\r\n");
      return false;
    default:
      if (!IS_SET(weapon_list[weapon_type].weaponFlags, WEAPON_FLAG_RANGED))
        return true;
      if (output)
        send_to_char(ch, "This weapon ability can only be added to melee weapons.\r\n");
      return false;
  }
  return true;
}

int obj_vnum_to_hunt_type(int vnum)
{
  switch (vnum)
  {
    case 60000: case 60001: return HUNT_TYPE_BASILISK;
    case 60002: case 60003: return HUNT_TYPE_MANTICORE;
    case 60004: case 60005: return HUNT_TYPE_WRAITH;
    case 60006: case 60007: return HUNT_TYPE_SIREN;
    case 60008: case 60009: return HUNT_TYPE_WILL_O_WISP;
    case 60010: case 60011: return HUNT_TYPE_BARGHEST;
    case 60012: case 60013: return HUNT_TYPE_BLACK_PUDDING;
    case 60014: case 60015: return HUNT_TYPE_CHIMERA;
    case 60016: case 60017: return HUNT_TYPE_GHOST;
    case 60018: case 60019: return HUNT_TYPE_MEDUSA;
    case 60020: case 60021: return HUNT_TYPE_BEHIR;
    case 60022: case 60023: return HUNT_TYPE_EFREETI;
    case 60024: case 60025: return HUNT_TYPE_DJINNI;
    case 60039: return HUNT_TYPE_YOUNG_WHITE_DRAGON;
    case 60041: return HUNT_TYPE_ADULT_GREEN_DRAGON;
    case 60042: return HUNT_TYPE_OLD_WHITE_DRAGON;
    case 60040: case 60043: return HUNT_TYPE_DRAGON_TURTLE;
    case 60029: case 60030: return HUNT_TYPE_ROC;
    case 60031: case 60032: return HUNT_TYPE_PURPLE_WORM;
    case 60033: case 60034: return HUNT_TYPE_PYROHYDRA;
    case 60035: case 60036: return HUNT_TYPE_BANDERSNATCH;
    case 60037: case 60038: return HUNT_TYPE_BANSHEE;
  }
  return 0;
}

bool is_specab_upgradeable(int specab_source, int specab_apply)
{

  switch (specab_source)
  {
    case WEAPON_SPECAB_FLAMING:
      if (specab_apply == WEAPON_SPECAB_FLAMING)
        return true;
      return false;
        case WEAPON_SPECAB_FROST:
      if (specab_apply == WEAPON_SPECAB_FROST)
        return true;
      return false;
        case WEAPON_SPECAB_SHOCK:
      if (specab_apply == WEAPON_SPECAB_SHOCK)
        return true;
      return false;
        case WEAPON_SPECAB_CORROSIVE:
      if (specab_apply == WEAPON_SPECAB_CORROSIVE)
        return true;
      return false;
  }
  return false;
}