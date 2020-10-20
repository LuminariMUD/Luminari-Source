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

/* To Do
 *
 * Add reward system
 * Add special abilities functionality
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

    add_hunt(HUNT_TYPE_ROC, 25, "roc",  "This immense raptor unleashes a shrill cry as it bares its talons, each large enough to carry off a horse.t", 
      "", CLASS_WARRIOR, TRUE_NEUTRAL, RACE_TYPE_ANIMAL, 
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

  struct char_data *mob = read_mobile(8100, VIRTUAL);
  char mob_descs[1000];

  if (!mob) {
    //send_to_char(ch, "Mob load error.\r\n");
    return;
  }

  // set descriptions
  mob->player.name = strdup(hunt_table[which_hunt].name);
  sprintf(mob_descs, "%s %s", AN(hunt_table[which_hunt].name), hunt_table[which_hunt].name);
  mob->player.short_descr = strdup(mob_descs);
  if (!strcmp(hunt_table[which_hunt].long_description, "Nothing")) {
    sprintf(mob_descs, "%s %s is here.\r\n", AN(hunt_table[which_hunt].name), hunt_table[which_hunt].name);
    mob->player.long_descr = strdup(mob_descs);
  } else {
    sprintf(mob_descs, "%s\r\n", hunt_table[which_hunt].long_description);
    mob->player.long_descr = strdup(mob_descs);
  }
  if (!strcmp(hunt_table[which_hunt].description, "Nothing")) {
    sprintf(mob_descs, "%s %s is here before you.\r\n", AN(hunt_table[which_hunt].name), hunt_table[which_hunt].name);
    mob->player.description = strdup(mob_descs);    
  } else {
    sprintf(mob_descs, "%s\r\n", hunt_table[which_hunt].description);
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
  // set flags
  SET_BIT_AR(MOB_FLAGS(mob), MOB_HUNTS_TARGET);
  SET_BIT_AR(MOB_FLAGS(mob), MOB_SENTINEL);

  X_LOC(mob) = world[room].coords[0];
  Y_LOC(mob) = world[room].coords[1];
  char_to_room(mob, room);
  
  send_to_room(room, "\tYYou've come across a hunt target: %s!\tn\r\n", CAP(mob->player.short_descr));
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

    x = dice(1, 31) -16;
    y = dice(1, 31) -16;
    
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
   if (type == HUNT_REWARD_TYPE_TRINKET)
   {
     send_to_char(ch,
          "Component Type          - Effect\r\n"
          "----------------------------------------------\r\n"
          "1.  basilisk scales     - Freedom of Movement\r\n"
          "2.  manticore hide      - Infravision\r\n"
          "3.  wraith dust         - Sense Life\r\n"
          "4.  siren hair          - Water Walk\r\n"
          "5.  will o wisp dust    - Invisibility\r\n"
          "6.  barghest hide       - Blinking\r\n"
          "7.  black pudding slime - Death Ward\r\n"
          "8.  chimera hide        - Detect Alignment\r\n"
          "9.  ghost dust          - Detect Invisibility\r\n"
          "10. medusa snake head   - Ultravision\r\n"
          "11. behir scale         - Electric Shield\r\n"
          "12. efreeti dust        - Fire Shield\r\n"
          "13. djinni dust         - Haste\r\n"
          "14. roc feather         - Flying\r\n"
          "15. purple worm hide    - Dangersense\r\n"
          "16. pyrohydra hide      - Regeneration\r\n"
          "17. bandersnatch hide   - Minor Globe\r\n"
          "18. banshee dust        - Displacement\r\n"
     );
     send_to_char(ch, "In order to redeem your hunt trophies, you will need 10 of the type specified and decide whether you wish it on a ring, pendant or bracer.\r\n");
     send_to_char(ch, "Eg. buy 2 ring\r\n");
     send_to_char(ch, "This will purchase a ring of infravision, provided you have 10 manticore hides in your inventory.\r\n");
   }
   else
   {
     send_to_char(ch,
          "Component Type            - Weapon Effect\r\n"
          "----------------------------------------------\r\n"
          "1.  basilisk tooth        - Seeking\r\n"
          "2.  manticore spike       - Merciful\r\n"
          "3.  wraith essence        - Disruption\r\n"
          "4.  chimera tooth         - Speed\r\n"
          "5.  black pudding essence - Wounding\r\n"
          "6.  barghest tooth        - Bane (randomly determined)\r\n"
          "7.  will o wisp essence   - Dancing\r\n"
          "8.  siren tongue          - Defending\r\n"
          "9.  efreeti finger        - Flaming\r\n"
          "10. behir tusk            - Shocking\r\n"
          "11. medusa eyes           - Blinding\r\n"
          "12. ghost essence         - Ghost Touch\r\n"
          "13. banshee essence       - Unholy\r\n"
          "14. bandersnatch tooth    - Vorpal\r\n"
          "15. pyrohydra tooth       - Bane (randomly determined)\r\n"
          "16. purple worm tooth     - Vicious\r\n"
          "17. roc talon             - Keen\r\n"
          "18. djinni finger         - Thundering\r\n"
          "19. dragon horn           - Bane (randomly determined)\r\n"
     );
     send_to_char(ch, "In order to redeem your hunt trophies, you will need 10 of the type specified.\r\n");
     send_to_char(ch, "Eg. buy 2\r\n");
     send_to_char(ch, "This will purchase weapon oil that applies the merciful affect when applied to a weapon, provided you have 10 manticore spikes in your inventory.\r\n");
   }
}

SPECIAL(huntsmaster)
{
  if (!CMD_IS("hunts") && !CMD_IS("list")) return 0;

  int i = 0;
  char reported[20], actual[20];
  int hours = 0, minutes = 0, seconds = 0;
  char arg1[200], arg2[200];

  half_chop(argument, arg1, arg2);

  if (CMD_IS("hunts")) {
    send_to_char(ch, "%-20s %-5s %-16s %s\r\n", "Hunt Target", "Level", "Last Seen Coords", (GET_LEVEL(ch) >= LVL_IMMORT) ? "Actual Coords" : "");
    send_to_char(ch, "---------------------------------------------------------\r\n");

    for (i = 0; i < 5; i++)
    {
      snprintf(reported, sizeof(reported), "%4d, %-4d", active_hunts[i][1], active_hunts[i][2]);
      snprintf(actual, sizeof(actual), "%4d, %-d", active_hunts[i][3], active_hunts[i][4]);
      send_to_char(ch, "%-20s %-5d %-16s %s\r\n", hunt_table[active_hunts[i][0]].name, hunt_table[active_hunts[i][0]].level, reported, (GET_LEVEL(ch) >= LVL_IMMORT) ? actual : "");
    }

    hours = hunt_reset_timer / 600;
    minutes = (hunt_reset_timer - (hours * 600)) / 10;
    seconds = hunt_reset_timer - ((hours * 600) + (minutes * 10));

    send_to_char(ch, "\r\nTime until next hunt targets: %d hour(s), %d minute(s), %d seconds", hours, minutes, seconds * 6 );
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
  else if (CMD_IS("buy"))
  {

  }

  return 1;
}

void award_hunt_materials(struct char_data *ch, int which_hunt)
{
  struct obj_data *obj1 = NULL, *obj2 = NULL;
  int roll = 0;

  switch (which_hunt) {
    
    case HUNT_TYPE_BASILISK:
      obj1 = read_object(60000, VIRTUAL);
      obj2 = read_object(60001, VIRTUAL);
      break;
    case HUNT_TYPE_MANTICORE:
      obj1 = read_object(60002, VIRTUAL);
      obj2 = read_object(60003, VIRTUAL);
      break;
    case HUNT_TYPE_WRAITH:
      obj1 = read_object(60004, VIRTUAL);
      obj2 = read_object(60005, VIRTUAL);
      break;
    case HUNT_TYPE_SIREN:
      obj1 = read_object(60006, VIRTUAL);
      obj2 = read_object(60007, VIRTUAL);
      break;
    case HUNT_TYPE_WILL_O_WISP:
      obj1 = read_object(60008, VIRTUAL);
      obj2 = read_object(60009, VIRTUAL);
      break;
    case HUNT_TYPE_BARGHEST:
      obj1 = read_object(60010, VIRTUAL);
      obj2 = read_object(60011, VIRTUAL);
      break;
    case HUNT_TYPE_BLACK_PUDDING:
      obj1 = read_object(60012, VIRTUAL);
      obj2 = read_object(60013, VIRTUAL);
      break;
    case HUNT_TYPE_CHIMERA:
      obj1 = read_object(60014, VIRTUAL);
      obj2 = read_object(60015, VIRTUAL);
      break;
    case HUNT_TYPE_GHOST:
      obj1 = read_object(60016, VIRTUAL);
      obj2 = read_object(60017, VIRTUAL);
      break;
    case HUNT_TYPE_MEDUSA:
      obj1 = read_object(60018, VIRTUAL);
      obj2 = read_object(60019, VIRTUAL);
      break;
    case HUNT_TYPE_BEHIR:
      obj1 = read_object(60020, VIRTUAL);
      obj2 = read_object(60021, VIRTUAL);
      break;
    case HUNT_TYPE_EFREETI:
      obj1 = read_object(60022, VIRTUAL);
      obj2 = read_object(60023, VIRTUAL);
      break;
    case HUNT_TYPE_DJINNI:
      obj1 = read_object(60024, VIRTUAL);
      obj2 = read_object(60025, VIRTUAL);
      break;
    case HUNT_TYPE_ROC:
      obj1 = read_object(60029, VIRTUAL);
      obj2 = read_object(60030, VIRTUAL);
      break;
    case HUNT_TYPE_PURPLE_WORM:
      obj1 = read_object(60031, VIRTUAL);
      obj2 = read_object(60032, VIRTUAL);
      break;
    case HUNT_TYPE_PYROHYDRA:
      obj1 = read_object(60033, VIRTUAL);
      obj2 = read_object(60034, VIRTUAL);
      break;
    case HUNT_TYPE_BANDERSNATCH:
      obj1 = read_object(60035, VIRTUAL);
      obj2 = read_object(60036, VIRTUAL);
      break;
    case HUNT_TYPE_BANSHEE:
      obj1 = read_object(60037, VIRTUAL);
      obj2 = read_object(60038, VIRTUAL);
      break;
    case HUNT_TYPE_ADULT_RED_DRAGON:
    case HUNT_TYPE_OLD_RED_DRAGON:
    case HUNT_TYPE_YOUNG_BLUE_DRAGON:
    case HUNT_TYPE_ADULT_BLUE_DRAGON:
    case HUNT_TYPE_OLD_BLUE_DRAGON:
    case HUNT_TYPE_YOUNG_GREEN_DRAGON:
    case HUNT_TYPE_ADULT_GREEN_DRAGON:
    case HUNT_TYPE_OLD_GREEN_DRAGON:
    case HUNT_TYPE_YOUNG_BLACK_DRAGON:
    case HUNT_TYPE_ADULT_BLACK_DRAGON:
    case HUNT_TYPE_OLD_BLACK_DRAGON:
    case HUNT_TYPE_YOUNG_WHITE_DRAGON:
    case HUNT_TYPE_ADULT_WHITE_DRAGON:
    case HUNT_TYPE_OLD_WHITE_DRAGON:
    case HUNT_TYPE_DRAGON_TURTLE:
      roll = dice(1, 3);
      if (roll == 1)
        obj1 = read_object(60026, VIRTUAL);
      else if (roll == 2)
        obj1 = read_object(60027, VIRTUAL);
      else
        obj1 = read_object(60028, VIRTUAL);      
      obj2 = read_object(60039, VIRTUAL);
      break;
  }
  
  if (obj1) {
    obj_to_char(obj1, ch);
    act("You harvest $p from the creature's remains.", true, ch, obj1, 0, TO_ROOM);
    act("$n harvests $p from the creature's remains.", true, ch, obj1, 0, TO_ROOM);
  }

  if (obj2) {
    obj_to_char(obj2, ch);
    act("You harvest $p from the creature's remains.", true, ch, obj2, 0, TO_ROOM);
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
      send_to_char(ch, "You've been rewarded %d quest points!\r\n", GET_LEVEL(hunt_mob) * 20);
    }
  }

}