// Character background system
// From D&D 5e and PF1e

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "backgrounds.h"
#include "treasure.h"
#include "handler.h"
#include "fight.h"
#include "spec_procs.h"
#include "clan.h"
#include "dg_scripts.h"
#include "feats.h"

int background_sort_info[NUM_BACKGROUNDS];
struct background_data background_list[NUM_BACKGROUNDS];

int backgrounds_listed_alphabetically[NUM_BACKGROUNDS] =
{
  BACKGROUND_NONE,
  BACKGROUND_ACOLYTE,
  BACKGROUND_CHARLATAN,
  BACKGROUND_CRIMINAL,
  BACKGROUND_ENTERTAINER,
  BACKGROUND_FOLK_HERO,
  BACKGROUND_GLADIATOR,
  BACKGROUND_HERMIT,
  BACKGROUND_NOBLE,
  BACKGROUND_OUTLANDER,
  BACKGROUND_PIRATE,
  BACKGROUND_SAGE,
  BACKGROUND_SAILOR,
  BACKGROUND_SOLDIER,
  BACKGROUND_SQUIRE,
  BACKGROUND_TRADER,
  BACKGROUND_URCHIN 
};

int compare_backgrounds(const void *x, const void *y)
{
  int a = *(const int *)x,
      b = *(const int *)y;

  return strcmp(background_list[a].name, background_list[b].name);
}

/* sort backgrounds called at boot up */
void sort_backgrounds(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a < NUM_BACKGROUNDS; a++)
    background_sort_info[a] = a;

  qsort(&background_sort_info[1], NUM_BACKGROUNDS, sizeof(int), compare_backgrounds);
}

static void backgroundo(int background, const char *name, int skill_one, int skill_two, int featnum, const char *desc)
{
    background_list[background].name = name;
    background_list[background].desc = desc;
    background_list[background].skills[0] = skill_one;
    background_list[background].skills[1] = skill_two;
    background_list[background].feat = featnum;
}

void initialize_background_list(void)
{
  int i;

  /* initialize the list of feats */
  for (i = 0; i < NUM_BACKGROUNDS; i++)
  {
    background_list[i].name = "none";
    background_list[i].desc = "Unused Background";
    background_list[i].skills[0] = 0;
    background_list[i].skills[1] = 0;
    background_list[i].feat = 0;
  }
}

void assign_backgrounds(void)
{
    initialize_background_list();

    backgroundo(BACKGROUND_ACOLYTE, "acolyte", ABILITY_SENSE_MOTIVE, ABILITY_RELIGION, FEAT_BG_ACOLYTE,
        "You have spent your life in the service of a temple to a specific god or pantheon of gods. "
        "You act as an intermediary between the realm of the holy and the mortal world, performing "
        "sacred rites and offering sacrifices in order to conduct worshipers into the presence of "
        "the divine. You are not necessarily a cleric-performing sacred rites is not the same thing "
        "as channeling divine power.");

    backgroundo(BACKGROUND_CHARLATAN, "charlatan", ABILITY_SLEIGHT_OF_HAND, ABILITY_BLUFF, FEAT_BG_CHARLATAN,
        "You have always had a way with people. You know what makes them tick, you can tease out their "
        "hearts' desires after a few minutes of conversation, and with a few leading questions you can "
        "read them like they were children's books. It's a useful talent, and one that you're perfectly "
        "willing to use for your advantage. You know what people want and you deliver, or rather, you "
        "promise to deliver. Common sense should steer people away from things that sound too good to "
        "be true, but common sense seems to be in short supply when you're around. The bottle of pink-colored "
        "liquid will surely cure that unseemly rash, this ointment - nothing more than a bit of fat with a "
        "sprinkle of silver dust - can restore youth and vigor, and there's a bridge in the city that just "
        "happens to be for sale. These marvels sound implausible, but you make them sound like the real deal.");

    backgroundo(BACKGROUND_CRIMINAL, "criminal/spy", ABILITY_BLUFF, ABILITY_STEALTH, FEAT_BG_CRIMINAL,
        "You are an experienced criminal with a history of breaking the law. You have spent a lot of time "
        "among other criminals and still have contacts within the criminal underworld. You're far closer "
        "than most people to the world of murder, theft, and violence that pervades the underbelly of "
        "civilization, and you have survived up to this point by flouting the rules and regulations of society.");

    backgroundo(BACKGROUND_ENTERTAINER, "entertainer", ABILITY_ACROBATICS, ABILITY_PERFORM, FEAT_BG_ENTERTAINER,
        "You thrive in front of an audience. You know how to entrance them, entertain them, and even inspire them. "
        "Your poetics can stir the hearts of those who hear you, awakening grief or joy, laughter or anger. Your "
        "music raises their spirits or captures their sorrow. Your dance steps captivate, your humor cuts to the "
        "quick. Whatever techniques you use, your art is your life.");

    backgroundo(BACKGROUND_FOLK_HERO, "folk hero", ABILITY_HANDLE_ANIMAL, ABILITY_SURVIVAL, FEAT_BG_FOLK_HERO,
        "You come from a humble social rank, but you are destined for so much more. Already the people of your "
        "home village regard you as their champion, and your destiny calls you to stand against the tyrants and "
        "monsters that threaten the common folk everywhere.");

    backgroundo(BACKGROUND_GLADIATOR, "gladiator", ABILITY_ACROBATICS, ABILITY_PERFORM, FEAT_BG_GLADIATOR,
        "A gladiator is as much an entertainer as any minstrel or circus performer, trained to make the arts "
        "of combat into a spectacle the crowd can enjoy. This kind of flashy combat is your entertainer routine, "
        "though you might also have some skills as a tumbler or actor.");

    backgroundo(BACKGROUND_TRADER, "trader", ABILITY_SENSE_MOTIVE, ABILITY_DIPLOMACY, FEAT_BG_TRADER,
        "You are a member of an artisan's guild, skilled in a particular field and closely associated "
        "with other artisans. You are a well-established part of the mercantile world, freed by talent "
        "and wealth from the constraints of a feudal social order. You learned your skills as an apprentice "
        "to a master artisan, under the sponsorship of your guild, until you became a master in your own right.");

    backgroundo(BACKGROUND_HERMIT, "hermit", ABILITY_HEAL, ABILITY_RELIGION, FEAT_BG_HERMIT,
        "You lived in seclusion-either in a sheltered community such as a monastery, or entirely alone-for "
        "a formative part of your life. In your time apart from the clamor of society, you found quiet, "
        "solitude, and perhaps some of the answers you were looking for.");

    backgroundo(BACKGROUND_SQUIRE, "squire", ABILITY_HISTORY, ABILITY_DIPLOMACY, FEAT_BG_SQUIRE,
        "You trained at the feet of a knight, maintaining their gear and supporting them at "
        "tourneys and in battle. Now you search for a challenge that will prove you worthy of "
        "full knighthood, or you've spurned pomp and ceremony to test yourself in honest, albeit "
        "less formal, combat.");

    backgroundo(BACKGROUND_NOBLE, "noble", ABILITY_HISTORY, ABILITY_DIPLOMACY, FEAT_BG_NOBLE,
        "You understand wealth, power, and privilege. You carry a noble title, and your family "
        "owns land, collects taxes, and wields significant political influence. You might be a "
        "pampered aristocrat unfamiliar with work or discomfort, a former merchant just elevated "
        "to the nobility, or a disinherited scoundrel with a disproportionate sense of entitlement. "
        "Or you could be an honest, hard-working landowner who cares deeply about the people who live "
        "and work on your land, keenly aware of your responsibility to them.");

    backgroundo(BACKGROUND_OUTLANDER, "outlander", ABILITY_ATHLETICS, ABILITY_SURVIVAL, FEAT_BG_OUTLANDER,
        "You grew up in the wilds, far from the comforts of town and technology. You've witnessed the "
        "migration of herds larger than forests, survived weather more extreme than any city-dweller "
        "could comprehend, and enjoyed the solitude of being the only thinking creature for miles in "
        "any direction. The wilds are in your blood, whether you were a nomad, an explorer, a recluse, "
        "a hunter-gatherer, or even a marauder. Even in places where you don't know the specific features "
        "of the terrain, you know the ways of the wild.");

    backgroundo(BACKGROUND_PIRATE, "pirate", ABILITY_ATHLETICS, ABILITY_PERCEPTION, FEAT_BG_PIRATE,
        "You sailed on a seagoing vessel for years. In that time, you faced down mighty storms, monsters "
        "of the deep, and those who wanted to sink your craft to the bottomless depths. Your first love is "
        "the distant line of the horizon, but the time has come to try your hand at something new.");

    backgroundo(BACKGROUND_SAGE, "sage", ABILITY_ARCANA, ABILITY_HISTORY, FEAT_BG_SAGE,
        "You spent years learning the lore of the multiverse. You scoured manuscripts, studied scrolls, "
        "and listened to the greatest experts on the subjects that interest you. Your efforts have made "
        "you a master in your fields of study.");
    
    backgroundo(BACKGROUND_SAILOR, "sailor", ABILITY_ATHLETICS, ABILITY_PERCEPTION, FEAT_BG_SAILOR,
        "You sailed on a seagoing vessel for years. In that time, you faced down mighty storms, monsters "
        "of the deep, and those who wanted to sink your craft to the bottomless depths. Your first love is "
        "the distant line of the horizon, but the time has come to try your hand at something new.");

    backgroundo(BACKGROUND_SOLDIER, "soldier", ABILITY_ATHLETICS, ABILITY_INTIMIDATE, FEAT_BG_SOLDIER,
        "War has been your life for as long as you care to remember. You trained as a youth, studied the "
        "use of weapons and armor, learned basic survival techniques, including how to stay alive on the "
        "battlefield. You might have been part of a standing national army or a mercenary company, or "
        "perhaps a member of a local militia who rose to prominence during a recent war.");

    backgroundo(BACKGROUND_URCHIN, "urchin", ABILITY_SLEIGHT_OF_HAND, ABILITY_STEALTH, FEAT_BG_URCHIN,
        "You grew up on the streets alone, orphaned, and poor. You had no one to watch over you or to "
        "provide for you, so you learned to provide for yourself. You fought fiercely over food and kept "
        "a constant watch out for other desperate souls who might steal from you. You slept on rooftops "
        "and in alleyways, exposed to the elements, and endured sickness without the advantage of medicine "
        "or a place to recuperate. You've survived despite all odds, and did so through cunning, strength, "
        "speed, or some combination of each.");
}

#define TEMPLE_COST_CURE    100
#define TEMPLE_COST_HEAL    500
#define TEMPLE_COST_REMOVE_POISON   250
#define TEMPLE_COST_REMOVE_DISEASE  350
#define TEMPLE_COST_REMOVE_CURSE    350
#define TEMPLE_COST_REJUV   100
#define TEMPLE_COST_RESTORATION 250
#define TEMPLE_COST_REMOVE_BLINDNESS 50
#define TEMPLE_COST_REMOVE_DEAFNESS 50

bool has_acolyte_in_group(struct char_data *ch)
{
    struct char_data *tch = NULL;
    bool acolyte = false;

    if (!ch || IN_ROOM(ch) == NOWHERE)
        return false;

    for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room)
    {
        if (IS_NPC(tch)) continue;
        if (!is_player_grouped(tch, ch)) continue;
        if (HAS_FEAT(tch, FEAT_BG_ACOLYTE))
        {
            acolyte = true;
            break;
        }
    }
    return acolyte;
}

bool temple_blessing_cost_handling(struct char_data *ch, int blessing)
{
    if (GET_GOLD(ch) < blessing && !has_acolyte_in_group(ch))
    {
        send_to_char(ch, "You need %d gold to purchase this blessing.\r\n", blessing);
        return false;
    }

    send_to_char(ch, "You receive the requested blessing.\r\n");
    act("$n is blessed by the temple priest.", FALSE, ch, 0, 0, TO_ROOM);

    if (!has_acolyte_in_group(ch))
    {
        GET_GOLD(ch) -= blessing;
        send_to_char(ch, "The blessing costs you %d gold coins.\r\n", blessing);
    }

    return true;
}

SPECIAL(temple)
{
    if (!CMD_IS("temple"))
        return 0;

    char arg[200];
    struct char_data *temple = (struct char_data *) me;

    one_argument(argument, arg, sizeof(arg));

    if (!*arg)
    {
        send_to_char(ch, "Please select from the following options:\r\n");
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "cure", "casts cure critical wounds", TEMPLE_COST_CURE);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "heal", "casts heal", TEMPLE_COST_HEAL);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "vision", "casts cure blindness", TEMPLE_COST_REMOVE_BLINDNESS);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "hearing", "casts cure deafness", TEMPLE_COST_REMOVE_DEAFNESS);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "poison", "casts remove poison", TEMPLE_COST_REMOVE_POISON);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "disease", "casts cure disease", TEMPLE_COST_REMOVE_DISEASE);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "curse", "casts remove curse", TEMPLE_COST_REMOVE_CURSE);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "restore", "casts restoration", TEMPLE_COST_RESTORATION);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "stamina", "casts vigorize critical", TEMPLE_COST_REJUV);
        send_to_char(ch, "\r\n");
        return 1;
    }
    else if (is_abbrev(arg, "cure"))
    {
        if (temple_blessing_cost_handling(ch, TEMPLE_COST_CURE))
        {
            call_magic(temple, ch, 0, SPELL_CURE_CRITIC, 0, GET_LEVEL(temple), CAST_SPELL);
            return 1;
        }
    }
    else if (is_abbrev(arg, "heal"))
    {
        if (temple_blessing_cost_handling(ch, TEMPLE_COST_HEAL))
        {
            call_magic(temple, ch, 0, SPELL_HEAL, 0, GET_LEVEL(temple), CAST_SPELL);
            return 1;
        }
    }
    else if (is_abbrev(arg, "vision"))
    {
        if (temple_blessing_cost_handling(ch, TEMPLE_COST_REMOVE_BLINDNESS))
        {
            call_magic(temple, ch, 0, SPELL_CURE_BLIND, 0, GET_LEVEL(temple), CAST_SPELL);
            return 1;
        }
    }
    else if (is_abbrev(arg, "hearing"))
    {
        if (temple_blessing_cost_handling(ch, TEMPLE_COST_REMOVE_DEAFNESS))
        {
            call_magic(temple, ch, 0, SPELL_CURE_DEAFNESS, 0, GET_LEVEL(temple), CAST_SPELL);
            return 1;
        }
    }
    else if (is_abbrev(arg, "poison"))
    {
        if (temple_blessing_cost_handling(ch, TEMPLE_COST_REMOVE_POISON))
        {
            call_magic(temple, ch, 0, SPELL_REMOVE_POISON, 0, GET_LEVEL(temple), CAST_SPELL);
            return 1;
        }
    }
    else if (is_abbrev(arg, "disease"))
    {
        if (temple_blessing_cost_handling(ch, TEMPLE_COST_REMOVE_DISEASE))
        {
            call_magic(temple, ch, 0, SPELL_REMOVE_DISEASE, 0, GET_LEVEL(temple), CAST_SPELL);
            return 1;
        }
    }
    else if (is_abbrev(arg, "curse"))
    {
        if (temple_blessing_cost_handling(ch, TEMPLE_COST_REMOVE_CURSE))
        {
            call_magic(temple, ch, 0, SPELL_REMOVE_CURSE, 0, GET_LEVEL(temple), CAST_SPELL);
            return 1;
        }
    }
    else if (is_abbrev(arg, "restore"))
    {
        if (temple_blessing_cost_handling(ch, TEMPLE_COST_RESTORATION))
        {
            call_magic(temple, ch, 0, SPELL_RESTORATION, 0, GET_LEVEL(temple), CAST_SPELL);
            return 1;
        }
    }
    else if (is_abbrev(arg, "stamina"))
    {
        if (temple_blessing_cost_handling(ch, TEMPLE_COST_REJUV))
        {
            call_magic(temple, ch, 0, SPELL_VIGORIZE_CRITICAL, 0, GET_LEVEL(temple), CAST_SPELL);
            return 1;
        }
    }
    else
    {
        send_to_char(ch, "Please select from the following options:\r\n");
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "cure", "casts cure critical wounds", TEMPLE_COST_CURE);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "heal", "casts heal", TEMPLE_COST_HEAL);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "vision", "casts cure blindness", TEMPLE_COST_REMOVE_BLINDNESS);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "hearing", "casts cure deafness", TEMPLE_COST_REMOVE_DEAFNESS);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "poison", "casts remove poison", TEMPLE_COST_REMOVE_POISON);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "disease", "casts cure disease", TEMPLE_COST_REMOVE_DISEASE);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "curse", "casts remove curse", TEMPLE_COST_REMOVE_CURSE);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "restore", "casts restoration", TEMPLE_COST_RESTORATION);
        send_to_char(ch, "-- %-15s %-30s%-d coins\r\n", "stamina", "casts vigorize critical", TEMPLE_COST_REJUV);
        send_to_char(ch, "\r\n");
        return 1;
    }
    return 1;
}

ACMDU(do_swindle)
{
  struct char_data *vict = NULL;
  int grade, gold;
  char buf[200];

  if (!HAS_REAL_FEAT(ch, FEAT_BG_CHARLATAN))
  {
    send_to_char(ch, "Only charlatans can 'swindle'.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Who do you want to swindle?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "You don't see anyone here by that description.\r\n");
    return;
  }

  if (!IS_NPC(vict))
  {
    send_to_char(ch, "You can't swindle players.\r\n");
    return;
  }

  if (GET_RACE(vict) != RACE_TYPE_HUMANOID && GET_RACE(vict) != RACE_TYPE_GIANT && 
        GET_RACE(vict) != RACE_TYPE_MONSTROUS_HUMANOID && GET_RACE(vict) != RACE_TYPE_DRAGON &&
        GET_RACE(vict) != RACE_TYPE_FEY && GET_RACE(vict) != RACE_TYPE_LYCANTHROPE &&
        GET_RACE(vict) != RACE_TYPE_OUTSIDER)
  {
    send_to_char(ch, "They aren't intelligent enough to swindle.\r\n");
    return;
  }

  if (vict->char_specials.swindle_cooldown > 0)
  {
    act("$N is already wary of being swindled.", TRUE, ch, 0, vict, TO_CHAR);
    return;
  }

    // you can only try once;
    vict->char_specials.swindle_cooldown = 100;

  int skill = skill_roll(ch, ABILITY_DECEPTION);
  int dc = skill_roll(vict, ABILITY_INSIGHT);

  if (skill < dc)
  {
    appear(ch, FALSE);
    act("$N recognizes you con and attacks!", FALSE, ch, 0, vict, TO_CHAR);
    act("You recognize $n's con!", FALSE, ch, 0, vict, TO_VICT);
    act("$N recognizes $n's con and attacks!", FALSE, ch, 0, vict, TO_NOTVICT);
    hit(vict, ch, TYPE_UNDEFINED, DAM_RESERVED_DBC, 0, FALSE);

    return;
  }
  else
  {    
    gold = dice(1, GET_LEVEL(vict)) * 5;
    snprintf(buf, sizeof(buf), "You manage to swindle $N out of %d coins.", gold);
    act(buf, FALSE, ch, 0, vict, TO_CHAR);
    snprintf(buf, sizeof(buf), "$n convinces you to give $m %d coins.", gold);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    act("$n convinces $N to give $M some coins.", FALSE, ch, 0, vict, TO_NOTVICT);

    if (dice(1, 20) == 1)
    {
        ch->char_specials.which_treasure_message = CUSTOM_TREASURE_MESSAGE_SWINDLE;
        grade = quick_grade_check(GET_LEVEL(vict));
        switch(dice(1, 10))
        {
            case 1:
            award_random_crystal(ch, grade);
            break;
            case 2: case 3:
            award_expendable_item(ch, grade, TYPE_SCROLL);
            break;
            case 5:  case 6:  case 7: 
            award_expendable_item(ch, grade, TYPE_POTION);
            break;
            case 8: 
            award_expendable_item(ch, grade, TYPE_WAND);
            break;
            case 9: 
            award_expendable_item(ch, grade, TYPE_STAFF);
            break;
            case 10: 
            award_misc_magic_item(ch, determine_rnd_misc_cat(), cp_convert_grade_enchantment(grade));
            break;
        }
        ch->char_specials.which_treasure_message = CUSTOM_TREASURE_MESSAGE_NONE;
        return;
    }
  }
}

ACMDU(do_entertain)
{
  struct char_data *vict = NULL;
  int grade, gold, i;
  char buf[200];
  struct affected_type af[3];

  if (!HAS_REAL_FEAT(ch, FEAT_BG_ENTERTAINER))
  {
    send_to_char(ch, "Only those with the entertainer background can 'entertain'.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Who do you want to entertain?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "You don't see anyone here by that description.\r\n");
    return;
  }

  if (!IS_NPC(vict))
  {
    send_to_char(ch, "You can't 'entertain' players.\r\n");
    return;
  }

  if (GET_RACE(vict) != RACE_TYPE_HUMANOID && GET_RACE(vict) != RACE_TYPE_GIANT && 
        GET_RACE(vict) != RACE_TYPE_MONSTROUS_HUMANOID && GET_RACE(vict) != RACE_TYPE_DRAGON &&
        GET_RACE(vict) != RACE_TYPE_FEY && GET_RACE(vict) != RACE_TYPE_LYCANTHROPE &&
        GET_RACE(vict) != RACE_TYPE_OUTSIDER)
  {
    send_to_char(ch, "They aren't intelligent enough to be entertained.\r\n");
    return;
  }

  if (vict->char_specials.entertain_cooldown> 0)
  {
    act("$N has already been entertained recently.", TRUE, ch, 0, vict, TO_CHAR);
    return;
  }

    // you can only try once;
    vict->char_specials.entertain_cooldown = 100;

  int skill = skill_roll(ch, ABILITY_PERFORM);
  int dc = skill_roll(vict, ABILITY_DISCIPLINE);

  if (skill < dc)
  {
    appear(ch, FALSE);
    act("$N looks unimpressed and moves on.", FALSE, ch, 0, vict, TO_CHAR);
    act("You are unimpressed with $n's performance.", FALSE, ch, 0, vict, TO_VICT);
    act("$N looks unimpressed with $n's performance.", FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }
  else
  {    
    gold = dice(1, GET_LEVEL(vict)) * 4;
    snprintf(buf, sizeof(buf), "You impress $N with your performance, who tips you %d coins.", gold);
    act(buf, FALSE, ch, 0, vict, TO_CHAR);
    snprintf(buf, sizeof(buf), "You are impressed with $n's performance, and you tip $m %d coins.", gold);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    act("$N looks impressed with $n's performance, and tips $m some coins.", FALSE, ch, 0, vict, TO_NOTVICT);

    if (dice(1, 20) == 1)
    {
        ch->char_specials.which_treasure_message = CUSTOM_TREASURE_MESSAGE_SWINDLE;
        grade = quick_grade_check(GET_LEVEL(vict));
        switch(dice(1, 10))
        {
            case 1:
            award_random_crystal(ch, grade);
            break;
            case 2: case 3:
            award_expendable_item(ch, grade, TYPE_SCROLL);
            break;
            case 5:  case 6:  case 7: 
            award_expendable_item(ch, grade, TYPE_POTION);
            break;
            case 8: 
            award_expendable_item(ch, grade, TYPE_WAND);
            break;
            case 9: 
            award_expendable_item(ch, grade, TYPE_STAFF);
            break;
            case 10: 
            award_misc_magic_item(ch, determine_rnd_misc_cat(), cp_convert_grade_enchantment(grade));
            break;
        }
        ch->char_specials.which_treasure_message = CUSTOM_TREASURE_MESSAGE_NONE;
        return;
    }

    if (!affected_by_spell(ch, ABILITY_ENTERTAIN_INSPIRATION))
    {
        for (i = 0; i < 3; i++)
        {
            new_affect(&(af[i]));
            af[i].spell = ABILITY_ENTERTAIN_INSPIRATION;
            af[i].location = APPLY_SKILL;
            af[i].modifier = 3;
            af[i].bonus_type = BONUS_TYPE_MORALE;
            af[i].duration = 50;
        }

        af[0].specific = ABILITY_PERSUASION;
        af[1].specific = ABILITY_DECEPTION;
        af[2].specific = ABILITY_PERFORM;

        for (i = 0; i < 3; i++)
        {
            affect_to_char(ch, (&(af[i])));
        }
        
        act("You are inspired by the results of your performance.", TRUE, ch, 0, 0, TO_CHAR);
    }

  }
}

ACMDU(do_tribute)
{
  struct char_data *vict = NULL;
  int grade, gold;
  char buf[200];

  if (!HAS_REAL_FEAT(ch, FEAT_BG_FOLK_HERO))
  {
    send_to_char(ch, "Only those with the folk hero background can receive 'tributes'.\r\n");
    return;
  }

    if (!is_in_hometown(ch))
    {
        send_to_char(ch, "You can only request a tribute when you're in your hometown.\r\n");
        return;
    }

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Who do you want to request a tribute from?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "You don't see anyone here by that description.\r\n");
    return;
  }

  if (!IS_NPC(vict))
  {
    send_to_char(ch, "You can't receive a 'tribute' from players.\r\n");
    return;
  }

  if (GET_RACE(vict) != RACE_TYPE_HUMANOID && GET_RACE(vict) != RACE_TYPE_GIANT && 
        GET_RACE(vict) != RACE_TYPE_MONSTROUS_HUMANOID && GET_RACE(vict) != RACE_TYPE_DRAGON &&
        GET_RACE(vict) != RACE_TYPE_FEY && GET_RACE(vict) != RACE_TYPE_LYCANTHROPE &&
        GET_RACE(vict) != RACE_TYPE_OUTSIDER)
  {
    send_to_char(ch, "They aren't intelligent enough to understand a request for a tribute.\r\n");
    return;
  }

  if (vict->char_specials.tribute_cooldown> 0)
  {
    act("$N has already given a tribute recently.", TRUE, ch, 0, vict, TO_CHAR);
    return;
  }

    // you can only try once;
    vict->char_specials.tribute_cooldown = 100;

  int skill = skill_roll(ch, ABILITY_PERSUASION);
  int dc = skill_roll(vict, ABILITY_INSIGHT);

  if (skill < dc)
  {
    appear(ch, FALSE);
    act("$N denies your request for a tribute.", FALSE, ch, 0, vict, TO_CHAR);
    act("You deny $n $s request for a tribute.", FALSE, ch, 0, vict, TO_VICT);
    act("$N denies $n $s request for a tribute.", FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }
  else
  {    
    gold = dice(1, GET_LEVEL(vict)) * 3;
    snprintf(buf, sizeof(buf), "$N knows of your accolades and offers you a tribute of %d coins.", gold);
    act(buf, FALSE, ch, 0, vict, TO_CHAR);
    snprintf(buf, sizeof(buf), "You know of $n's accolades and offer $m a tribute of %d coins.", gold);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    act("$N knows of $n's accolades and offers $m a tribute of some coins.", FALSE, ch, 0, vict, TO_NOTVICT);

    if (dice(1, 20) == 1)
    {
        ch->char_specials.which_treasure_message = CUSTOM_TREASURE_MESSAGE_TRIBUTE;
        grade = quick_grade_check(GET_LEVEL(vict));
        switch(dice(1, 10))
        {
            case 1:
            award_random_crystal(ch, grade);
            break;
            case 2: case 3:
            award_expendable_item(ch, grade, TYPE_SCROLL);
            break;
            case 5:  case 6:  case 7: 
            award_expendable_item(ch, grade, TYPE_POTION);
            break;
            case 8: 
            award_expendable_item(ch, grade, TYPE_WAND);
            break;
            case 9: 
            award_expendable_item(ch, grade, TYPE_STAFF);
            break;
            case 10: 
            award_misc_magic_item(ch, determine_rnd_misc_cat(), cp_convert_grade_enchantment(grade));
            break;
        }
        ch->char_specials.which_treasure_message = CUSTOM_TREASURE_MESSAGE_NONE;
        return;
    }
  }
}

ACMDU(do_extort)
{
  struct char_data *vict = NULL;
  int grade, gold;
  char buf[200];

  if (!HAS_REAL_FEAT(ch, FEAT_BG_PIRATE))
  {
    send_to_char(ch, "Only those with the pirate background can 'extort' others.\r\n");
    return;
  }

  skip_spaces(&argument);

  if (!*argument)
  {
    send_to_char(ch, "Who do you want to extort?\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, argument, NULL, FIND_CHAR_ROOM)))
  {
    send_to_char(ch, "You don't see anyone here by that description.\r\n");
    return;
  }

  if (!IS_NPC(vict))
  {
    send_to_char(ch, "You can't extort players.\r\n");
    return;
  }

  if (GET_RACE(vict) != RACE_TYPE_HUMANOID && GET_RACE(vict) != RACE_TYPE_GIANT && 
        GET_RACE(vict) != RACE_TYPE_MONSTROUS_HUMANOID && GET_RACE(vict) != RACE_TYPE_DRAGON &&
        GET_RACE(vict) != RACE_TYPE_FEY && GET_RACE(vict) != RACE_TYPE_LYCANTHROPE &&
        GET_RACE(vict) != RACE_TYPE_OUTSIDER)
  {
    send_to_char(ch, "They aren't intelligent enough to understand extortion.\r\n");
    return;
  }

  if (vict->char_specials.extortion_cooldown> 0)
  {
    act("$N has already been a victim of extortion recently.", TRUE, ch, 0, vict, TO_CHAR);
    return;
  }

    // you can only try once;
    vict->char_specials.extortion_cooldown = 100;

  int skill = skill_roll(ch, ABILITY_INTIMIDATE);
  int dc = skill_roll(vict, ABILITY_DISCIPLINE);

  if (skill < dc)
  {
    appear(ch, FALSE);
    act("$N denies your attempt at extortion.", FALSE, ch, 0, vict, TO_CHAR);
    act("You deny $n's attempt at extortion.", FALSE, ch, 0, vict, TO_VICT);
    act("$N denies $n's attempt at extortion.", FALSE, ch, 0, vict, TO_NOTVICT);
    return;
  }
  else
  {    
    gold = dice(1, GET_LEVEL(vict)) * 3;
    snprintf(buf, sizeof(buf), "$N backs down to your extortion and hands you %d coins.", gold);
    act(buf, FALSE, ch, 0, vict, TO_CHAR);
    snprintf(buf, sizeof(buf), "You back down to $n's extortion and give $m %d coins.", gold);
    act(buf, FALSE, ch, 0, vict, TO_VICT);
    act("$N backs down to $n's extortion and gives $m some coins.", FALSE, ch, 0, vict, TO_NOTVICT);

    if (dice(1, 20) == 1)
    {
        ch->char_specials.which_treasure_message = CUSTOM_TREASURE_MESSAGE_EXTORTION;
        grade = quick_grade_check(GET_LEVEL(vict));
        switch(dice(1, 10))
        {
            case 1:
            award_random_crystal(ch, grade);
            break;
            case 2: case 3:
            award_expendable_item(ch, grade, TYPE_SCROLL);
            break;
            case 5:  case 6:  case 7: 
            award_expendable_item(ch, grade, TYPE_POTION);
            break;
            case 8: 
            award_expendable_item(ch, grade, TYPE_WAND);
            break;
            case 9: 
            award_expendable_item(ch, grade, TYPE_STAFF);
            break;
            case 10: 
            award_misc_magic_item(ch, determine_rnd_misc_cat(), cp_convert_grade_enchantment(grade));
            break;
        }
        ch->char_specials.which_treasure_message = CUSTOM_TREASURE_MESSAGE_NONE;
        return;
    }
  }
}


ACMDU(do_forgeas)
{

  if (IS_NPC(ch))
  {
    send_to_char(ch, "NPCs cannot forge signatures.\r\n");
    return;
  }

  if (!*argument)
  {
    send_to_char(ch, "Please enter in the name of the person you're trying to forge as.\r\n");
    return;
  }

  skip_spaces(&argument);

  if ((strlen(argument) - count_color_chars(argument)) > 50)
  {
    send_to_char(ch, "The name you forge as cannot exceed 50 characters. ( not including color code characters)\r\n");
    return;
  }

  if (ch->player_specials->forge_as_signature)
    free(ch->player_specials->forge_as_signature);
  ch->player_specials->forge_as_signature = strdup(argument);
  ch->player_specials->forge_check = d20(ch) + compute_ability(ch, ABILITY_LINGUISTICS);
  send_to_char(ch, "The next note you write or relay will be forged as if signed by, \"%s\".\r\n", argument);

}

ACMD(do_relay)
{
  struct obj_data *obj = NULL;
  int y = 0;
  struct char_data *recipient = NULL;
  char arg1[200], arg2[LONG_STRING];
  char buf[MAX_INPUT_LENGTH*4];

  if (IS_NPC(ch))
  {
    send_to_char(ch, "Only PCs can relay messages.\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_BG_CRIMINAL))
  {
    send_to_char(ch, "You must have the criminal background to relay a message.\r\n");
    return;
  }

  half_chop_c(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "Please specify the recipient of your relayed note.\r\n");
    return;
  }

  if (!*arg2)
  {
    send_to_char(ch, "What message do you want to relay to them?\r\n");
    return;
  }

  if (!(recipient = get_char_vis(ch, arg1, NULL, FIND_CHAR_WORLD)))
  {
    send_to_char(ch, "Your contacts can't find anyone by that name.\r\n");
    return;
  }

  if (IN_ROOM(recipient) != NOWHERE)
  {
    if (zone_table[world[IN_ROOM(recipient)].zone].city == CITY_NONE)
    {
      send_to_char(ch, "Your contacts can't find anyone by that name.\r\n");
      return;
    }
  }
  else
  {
    send_to_char(ch, "Your contacts can't find anyone by that name.\r\n");
    return;
  }

  if (IS_NPC(recipient))
  {
    send_to_char(ch, "That is an NPC.  Please specify a player name. Try using 2.name, 3.name, etc.\r\n");
    return;
  }

  snprintf(buf, sizeof(buf), "%s\n\nSigned by: %s\n", strfrmt((char *) argument, 80, 1, FALSE, FALSE, FALSE), 
            ch->player_specials->forge_as_signature ? ch->player_specials->forge_as_signature : GET_NAME(ch));

  obj = create_obj();
  obj->item_number = 1;
  if (ch->player_specials->forge_check > 0)
  {
    GET_OBJ_VAL(obj, 0) = ch->player_specials->forge_check;
  }
  obj->name = strdup("rolled piece paper");
  obj->short_description = strdup("a rolled piece of paper");
  obj->description = strdup("A rolled piece of paper lies here.");

  GET_OBJ_TYPE(obj) = ITEM_NOTE;
  for (y = 0; y < TW_ARRAY_MAX; y++)
    obj->obj_flags.wear_flags[y] = 0;
  SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
  GET_OBJ_WEIGHT(obj) = 1;
  GET_OBJ_COST(obj) = 30;
  GET_OBJ_RENT(obj) = 10;
  obj->action_description = strdup(buf);

  obj_to_char(obj, recipient);

  ch->player_specials->forge_check = 0;
  if (ch->player_specials->forge_as_signature)
    free(ch->player_specials->forge_as_signature);

  act("You hand $p to a street urchin, who runs off to find your contact.", FALSE, 0, obj, ch, TO_CHAR);
  act("$n hands $p to a street urchin, who runs off.", FALSE, ch, obj, 0, TO_ROOM);

  act("A street urchin runs up and gives you a rolled piece of paper.", FALSE, 0, 0, recipient, TO_VICT);
  act("A street urchin runs up and gives $n a rolled piece of paper.", FALSE, recipient, 0, 0, TO_ROOM);
}

ACMD(do_forage)
{

  int skill, dc, result, roll;

  if (IS_NPC(ch))
  {
    send_to_char(ch, "NPCs cannot forage.\r\n");
  }

  if (GET_FORAGE_COOLDOWN(ch) > 0)
  {
    send_to_char(ch, "You've recently foraged and will have to wait.\r\n");
    return;
  }

  if (!IN_WILDERNESS(ch))
  {
    send_to_char(ch, "You must be in the wild to forage.\r\n");
    return;
  }

  skill = compute_ability(ch, ABILITY_NATURE);
  dc = 15;
  roll = d20(ch);
  result = roll + skill - dc;

  send_to_char(ch, "You attempt to forage for some food... Roll %d + %d (nature skill) for total %d vs. dc %d\r\n",
                  roll, skill, roll + skill, dc);

  if (result < 0)
  {
    send_to_char(ch, "You fail to find anything edible.\r\n");
    GET_FORAGE_COOLDOWN(ch) = 100;
    return;
  }

  award_random_food_item(ch, result, 1);  
}

#define RETAINER_SYNTAX ("Proper syntax is:\r\n" \
                        "-- retainer call\r\n" \
                        "-- retainer sell\r\n" \
                        "-- retainer recipient (mail recipient name)\r\n" \
                        "-- retainer mail (message to send)\r\n" \
                        "See HELP RETAINER for more information")

ACMD(do_retainer)
{
  struct obj_data *obj = NULL, *next_obj = NULL;
  int y = 0, gold = 0;
  struct char_data *recipient = NULL, *retainer = NULL;
  char arg1[200], arg2[LONG_STRING];
  char buf[MAX_INPUT_LENGTH*4];

  if (!HAS_FEAT(ch, FEAT_BG_SQUIRE))
  {
    send_to_char(ch, "Only those with the squire background can call their retainer.\r\n");
    return;
  }

  half_chop_c(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

  if (!*arg1)
  {
    send_to_char(ch, "%s", RETAINER_SYNTAX);
    return;
  }
  else if (is_abbrev(arg1, "call"))
  {
    if (is_retainer_in_room(ch))
    {
      send_to_char(ch, "Your retainer is already here.\r\n");
      return;
    }

    if (GET_RETAINER_COOLDOWN(ch) > 0)
    {
      send_to_char(ch, "You cannot call your retainer yet.\r\n");
      return;
    }

    retainer = read_mobile(RETAINER_MOB_VNUM, VIRTUAL);
    
    if (!retainer)
    {
      send_to_char(ch, "The system could not find the retainer mob. Please inform staff with code ERRRET001.\r\n");
      return;
    }
    SET_BIT_AR(AFF_FLAGS(retainer), AFF_CHARM);
    char_to_room(retainer, IN_ROOM(ch));
    act("You call forth $N.", TRUE, ch, 0, retainer, TO_CHAR);
    act("$n calls forth $N.", TRUE, ch, 0, retainer, TO_ROOM);
    add_follower(retainer, ch);
    GET_RETAINER_COOLDOWN(ch) = 100;
    return;
  }
  else if (is_abbrev(arg1, "sell"))
  {
    if ((retainer = get_retainer_from_room(ch)) == NULL)
    {
      send_to_char(ch, "You need to call your retainer in order to sell items.\r\n");
      return; 
    }

    if ((obj = retainer->carrying) == NULL)
    {
      send_to_char(ch, "Your retainer isn't carrying anything.  Give any items you wish to sell to your retainer and then type: retainer sell\r\n");
      return;
    }

    for (obj = retainer->carrying; obj; obj = next_obj)
    {
      next_obj = obj->next_content;
      gold += (int) (GET_OBJ_COST(obj) * (0.15));
      obj_from_char(obj);
      extract_obj(obj);
    }

    send_to_char(ch, "Your retainer gives you a bank note worth %d coins, and then departs to sell the items you gave them.\r\n", gold);
    act("$N gives $n a slip of paper and then hurries off.", TRUE, ch, 0, retainer, TO_ROOM);
    GET_GOLD(ch) += gold;
    extract_char(retainer);
    return;
  }
  else if (is_abbrev(arg1, "recipient"))
  {
    if (!*arg2)
    {
      send_to_char(ch, "Please specify your mail recipient's name.\r\n");
      return;
    }

    GET_RETAINER_MAIL_RECIPIENT(ch) = strdup(arg2);
    send_to_char(ch, "You've set your retainer mail recipient to '%s'.\r\n", arg2);
    return;
  }
  else if (is_abbrev(arg1, "mail"))
  {

    if ((retainer = get_retainer_from_room(ch)) == NULL)
    {
      send_to_char(ch, "You need to call your retainer in order to send a message. Use 'retainer call' to do so.\r\n");
      return;
    }

    if (!*arg2)
    {
      send_to_char(ch, "What would you like your mssage to say?\r\n");
      return;
    }

    if (GET_RETAINER_MAIL_RECIPIENT(ch) == NULL)
    {
      send_to_char(ch, "You need to specify your mail recipient first by typing: retainer recipient (recipient name).\r\n");
      return;
    }

    if (!(recipient = get_char_vis(ch, GET_RETAINER_MAIL_RECIPIENT(ch), NULL, FIND_CHAR_WORLD)))
    {
      send_to_char(ch, "Your contacts can't find anyone by the name of '%s'.\r\n", GET_RETAINER_MAIL_RECIPIENT(ch));
      return;
    }

    if (IN_ROOM(recipient) != NOWHERE)
    {
      if (are_clans_allied(zone_table[world[IN_ROOM(recipient)].zone].faction, GET_CLAN(ch)))
      {
        send_to_char(ch, "Your retainer can only send to recipients in allied zones. This means you must be clanned and both you and your recipient are in a clan allied zone.\r\n");
        return;
      }
    }
    else
    {
      send_to_char(ch, "Your contacts can't find anyone by the name of '%s'.\r\n", GET_RETAINER_MAIL_RECIPIENT(ch));
      return; 
    }

    if (IS_NPC(recipient))
    {
      send_to_char(ch, "That is an NPC.  Please specify a player name. Try using 2.name, 3.name, etc.\r\n");
      return;
    }

    snprintf(buf, sizeof(buf), "%s\n\nSigned by: %s\n", strfrmt((char *) arg2, 80, 1, FALSE, FALSE, FALSE), GET_NAME(ch));

    obj = create_obj();
    obj->item_number = 1;
    if (ch->player_specials->forge_check > 0)
    {
      GET_OBJ_VAL(obj, 0) = ch->player_specials->forge_check;
    }
    obj->name = strdup("rolled piece paper");
    obj->short_description = strdup("a rolled piece of paper");
    obj->description = strdup("A rolled piece of paper lies here.");

    GET_OBJ_TYPE(obj) = ITEM_NOTE;
    for (y = 0; y < TW_ARRAY_MAX; y++)
      obj->obj_flags.wear_flags[y] = 0;
    SET_BIT_AR(GET_OBJ_WEAR(obj), ITEM_WEAR_TAKE);
    GET_OBJ_WEIGHT(obj) = 1;
    GET_OBJ_COST(obj) = 30;
    GET_OBJ_RENT(obj) = 10;
    obj->action_description = strdup(buf);

    obj_to_char(obj, recipient);

    act("You hand $p to youor retainer, who runs off to find your contact.", FALSE, 0, obj, ch, TO_VICT);
    act("$n hands $p to $s retainer, who runs off.", FALSE, ch, obj, 0, TO_ROOM);

    act("A hired retainer runs up and gives you a rolled piece of paper.", FALSE, 0, 0, recipient, TO_VICT);
    act("A hired retainer runs up and gives $n a rolled piece of paper.", FALSE, recipient, 0, 0, TO_ROOM);

    GET_RETAINER_MAIL_RECIPIENT(ch) = NULL;
    extract_char(retainer);

    return;
  }
  else
  {
    send_to_char(ch, "%s", RETAINER_SYNTAX);
    return;
  }
  
}
ACMD(do_shortcut)
{
  char arg1[200];
  room_vnum rvnum = 0;
  room_rnum target_room = NOWHERE;
  struct char_data *vict;

  one_argument(argument, arg1, sizeof(arg1));

  if (!*arg1)
  {
    send_to_char(ch, "You must specify a player, mob or room vnum. Rooms vnum can "
                     "be found by typing roomvnum, which will display the vnum for the room you're in.\r\n");
    return;
  }

  if (GET_HOMETOWN(ch) == CITY_NONE)
  {
    send_to_char(ch, "You do not have a hometown set. This is required to use this ability.\r\n"
                     "You can do so by quitting to the main game menu and selecting the option there.\r\n");
    return;
  }

  if (atoi(arg1) > 0)
  {
    target_room = real_room(rvnum);
  }
  else
  {
    if (!(vict = get_char_vis(ch, arg1, NULL, FIND_CHAR_WORLD)))
    {
      send_to_char(ch, "You can't find anyone by that description.\r\n");
      return;
    }
    target_room = IN_ROOM(vict);
  }

  if (target_room == NOWHERE)
  {
    send_to_char(ch, "You can't find a path to that target.\r\n");
    return;
  }

  if (zone_table[world[target_room].zone].city != GET_HOMETOWN(ch))
  {
    send_to_char(ch, "You can't find a path to that target.\r\n");
    return;
  }

  act("You find a shortcut to you destination.", TRUE, ch, 0, 0, TO_CHAR);
  act("$n leaves the area suddenly.", TRUE, ch, 0, 0, TO_ROOM);
  char_from_room(ch);
  char_to_room(ch, target_room);
  act("$n arrives suddenly.", TRUE, ch, 0, 0, TO_ROOM);
  look_at_room(ch, 0);
  entry_memory_mtrigger(ch);
  greet_mtrigger(ch, -1);
  greet_memory_mtrigger(ch);
}

void show_background_help(struct char_data *ch, int background)
{

  char buf[500];
  int i = 0;

  if (background <= BACKGROUND_NONE || background >= NUM_BACKGROUNDS)
  {
    send_to_char(ch, "Background type is invalid.\r\n");
    return;
  }

  snprintf(buf, sizeof(buf), "%s BACKGROUND", background_list[background].name);

  for (i = 0; i < strlen(buf); i++)
  {
    buf[i] = toupper(buf[i]);
  }

  draw_line(ch, 80, '-', '-');
  text_line(ch, buf, 80, '-', '-');
  draw_line(ch, 80, '-', '-');

  snprintf(buf, sizeof(buf), "%s", background_list[background].desc);
  send_to_char(ch, "%s", strfrmt(buf, 80, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');
  send_to_char(ch, "Skill Bonuses: +2 to %s, +2 to %s.\r\n", 
              ability_names[background_list[background].skills[0]], ability_names[background_list[background].skills[1]]);
  draw_line(ch, 80, '-', '-');
  snprintf(buf, sizeof(buf), "Special Ability: %s", feat_list[background_list[background].feat].description);
  send_to_char(ch, "%s", strfrmt(buf, 80, 1, 0, 0, 0));
  draw_line(ch, 80, '-', '-');

}