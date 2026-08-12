/**
 * @file spec/spec_rol_residual_mobile.c
 * Residual identity-profiled mobile behavior for the RoL conversion.
 */

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"

#include "act.h"
#include "combat/fight.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "magic/domains_schools.h"
#include "magic/spells.h"
#include "mud_event.h"
#include "mudlim.h"
#include "spec/spec_dispatch.h"
#include "spec/spec_rol_conversion.h"

enum rol_residual_mobile_effect
{
  ROL_RESIDUAL_BEAVIS = 0,
  ROL_RESIDUAL_BUTTHEAD,
  ROL_RESIDUAL_ANCIENT_BROWNIE,
  ROL_RESIDUAL_FINN,
  ROL_RESIDUAL_FAERIE,
  ROL_RESIDUAL_ROLL_WITH_IT,
  ROL_RESIDUAL_VANISH
};

struct rol_residual_mobile_profile
{
  int mobile_vnum;
  enum rol_residual_mobile_effect effect;
  const char *description;
};

/* Keep this table sorted by converted mobile VNUM for binary lookup. */
static const struct rol_residual_mobile_profile rol_residual_mobile_profiles[] = {
    {2001228, ROL_RESIDUAL_BEAVIS, "Beavis ambient activity."},
    {2001229, ROL_RESIDUAL_BUTTHEAD, "Butthead ambient activity."},
    {2005718, ROL_RESIDUAL_ANCIENT_BROWNIE, "Ancient brownie ankle attack."},
    {2014015, ROL_RESIDUAL_FINN, "Finn ambient and combat speech."},
    {2014029, ROL_RESIDUAL_FAERIE, "Faerie mischief activity."},
    {2020247, ROL_RESIDUAL_ROLL_WITH_IT, "Spell-casting interception and counterstrike."},
    {2026208, ROL_RESIDUAL_ROLL_WITH_IT, "Spell-casting interception and counterstrike."},
    {2026216, ROL_RESIDUAL_ROLL_WITH_IT, "Spell-casting interception and counterstrike."},
    {2026236, ROL_RESIDUAL_ROLL_WITH_IT, "Spell-casting interception and counterstrike."},
    {2026244, ROL_RESIDUAL_ROLL_WITH_IT, "Spell-casting interception and counterstrike."},
    {2026245, ROL_RESIDUAL_ROLL_WITH_IT, "Spell-casting interception and counterstrike."},
    {2059815, ROL_RESIDUAL_VANISH, "Delayed extraplanar vanishing."},
    {2059835, ROL_RESIDUAL_VANISH, "Delayed extraplanar vanishing."},
};

static const struct rol_residual_mobile_profile *rol_residual_mobile_profile_for(int mobile_vnum)
{
  size_t high = sizeof(rol_residual_mobile_profiles) / sizeof(rol_residual_mobile_profiles[0]);
  size_t low = 0;
  size_t middle;

  while (low < high)
  {
    middle = low + (high - low) / 2;
    if (rol_residual_mobile_profiles[middle].mobile_vnum < mobile_vnum)
      low = middle + 1;
    else
      high = middle;
  }
  if (low < sizeof(rol_residual_mobile_profiles) / sizeof(rol_residual_mobile_profiles[0]) &&
      rol_residual_mobile_profiles[low].mobile_vnum == mobile_vnum)
    return &rol_residual_mobile_profiles[low];
  return NULL;
}

size_t rol_residual_mobile_profile_count(void)
{
  return sizeof(rol_residual_mobile_profiles) / sizeof(rol_residual_mobile_profiles[0]);
}

bool rol_residual_mobile_profile(int mobile_vnum, const char **description)
{
  const struct rol_residual_mobile_profile *profile = rol_residual_mobile_profile_for(mobile_vnum);

  if (profile == NULL)
    return false;
  if (description != NULL)
    *description = profile->description;
  return true;
}

static void rol_residual_say(struct char_data *ch, const char *message)
{
  if (message != NULL)
    do_say(ch, message, 0, 0);
}

static void rol_residual_social(struct char_data *ch, const char *social, const char *target)
{
  int command = find_command(social);

  if (command >= 0)
    do_action(ch, target != NULL ? target : "", command, 0);
}

static struct char_data *rol_residual_random_player(struct char_data *ch)
{
  struct char_data *candidate;
  struct char_data *selected = NULL;
  int eligible = 0;

  for (candidate = world[IN_ROOM(ch)].people; candidate != NULL;
       candidate = candidate->next_in_room)
  {
    if (candidate == ch || IS_NPC(candidate) || GET_LEVEL(candidate) >= LVL_IMMORT ||
        GET_POS(candidate) <= POS_DEAD || !CAN_SEE(ch, candidate))
      continue;
    eligible++;
    if (rand_number(1, eligible) == 1)
      selected = candidate;
  }
  return selected;
}

static void rol_residual_steal(struct char_data *ch, struct char_data *victim)
{
  int gold;

  if (victim == NULL || IS_NPC(victim) || GET_LEVEL(victim) >= LVL_IMMORT || !CAN_SEE(ch, victim))
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

static void rol_residual_beavis(struct char_data *ch, bool butthead)
{
  static const char *beavis_says[] = {
      "Heh hehe that sucks dude!",
      "Dude, this is like cool!",
      "Heh, whoa dude that was cool!",
      "I think this is cool or something.",
      "Look at those chicks, huh huh huh",
      "huh huh huh huh huh huh huh huh huh",
      "We're there dude!",
      "This video SUCKS!",
      "Shutup asswipe!",
      "Metallica kicks ass!",
      "White Zombie RULES!",
      "Mmmmm tastes like chicken.",
      "Fire fire fire fire fire!",
      "Shutup ButtHead, I'll kick your ass!",
  };
  static const char *butthead_says[] = {
      "Heh hehe that sucks dude!",
      "Dude, this is like cool!",
      "Heh, whoa dude that was cool!",
      "I think this is cool or something.",
      "Look at those chicks, huh huh huh",
      "huh huh huh huh huh huh huh huh huh",
      "We're there dude!",
      "This video SUCKS!",
      "Shutup asswipe!",
      "Metallica kicks ass!",
      "White Zombie RULES!",
  };
  int roll = rand_number(0, 40);

  if (!butthead && roll >= 0 && roll <= 13)
  {
    rol_residual_say(ch, beavis_says[roll]);
    return;
  }
  if (butthead && roll >= 0 && roll <= 10)
  {
    rol_residual_say(ch, butthead_says[roll]);
    return;
  }
  if (roll == 15)
    rol_residual_social(ch, "bang", NULL);
  else if (roll == 16)
    rol_residual_social(ch, "mosh", NULL);
  else if (roll == 17)
    rol_residual_social(ch, "bang", butthead ? "beavis" : "butthead");
  else if (!butthead && roll == 18)
    rol_residual_say(ch, "Change it or kill me, Butthead!");
  else if (!butthead && roll == 19)
    rol_residual_say(ch, "Isn't this new band, Schlong?");
  else if (!butthead && roll == 20)
    rol_residual_say(ch, "Nachos rule! They rule!");
  else if (butthead && roll == 13)
    rol_residual_say(ch, "Settle down Beavis.");
  else if (butthead && roll == 14)
    rol_residual_say(ch, "I'll kick your ass!");
  else if (butthead && roll == 18)
    rol_residual_say(ch, "Shutup butt munch!");
  else if (butthead && roll == 19)
    rol_residual_say(ch, "Shutup dillhole!");
  else if (butthead && roll == 20)
    rol_residual_say(ch, "Don't bogart my log Beavis!");
  else if (butthead && roll == 21)
    rol_residual_say(ch, "No, thats Prong");
  else if (butthead && roll == 22)
    rol_residual_say(ch, "Hey Beavis, we're cool huh.");
  else if (butthead && roll == 23)
    rol_residual_say(ch, "Nachos rule! They rule!");
  else if (butthead && roll == 24)
    rol_residual_say(ch, "Nudi... n u i d i s... heh nude people.");
}

static void rol_residual_finn(struct char_data *ch)
{
  int roll = rand_number(1, 15);

  if (FIGHTING(ch) != NULL)
  {
    if (roll == 1)
      rol_residual_say(ch, "You think you can actually beat me? I laugh at your attempt.");
    else if (roll == 2)
      rol_residual_say(ch, "Cease this foolishness before I am forced to destroy you.");
    else if (roll == 3)
      rol_residual_say(ch, "Where shall I instruct my page to deliver your corpse?");
    return;
  }
  switch (roll)
  {
  case 1:
    rol_residual_say(ch, "If you are new to this realm, please go to Anna's cottage.");
    break;
  case 2:
    rol_residual_say(ch, "I wish I could leave this blasted realm.");
    break;
  case 3:
    rol_residual_say(ch, "I cannot believe I lost my ring and my way home.");
    break;
  case 4:
    act("$n searches through $s travel gear fruitlessly, then sighs.", TRUE, ch, NULL, NULL,
        TO_ROOM);
    break;
  case 5:
    rol_residual_say(ch, "New travelers should seek Anna's cottage in the Faerie Forest.");
    break;
  case 6:
    rol_residual_say(ch, "Make sure you travel this realm with care.");
    break;
  default:
    break;
  }
}

static void rol_residual_faerie(struct char_data *ch)
{
  struct char_data *victim;

  if (FIGHTING(ch) != NULL)
    return;
  switch (rand_number(1, 15))
  {
  case 1:
    if ((victim = rol_residual_random_player(ch)) != NULL)
    {
      act("$n tickles you into a fit of hysterics.", FALSE, ch, NULL, victim, TO_VICT);
      act("$n tickles $N into a fit of hysterics.", TRUE, ch, NULL, victim, TO_NOTVICT);
    }
    break;
  case 2:
    if ((victim = rol_residual_random_player(ch)) != NULL)
    {
      act("$n dances around you in a merry little jig.", FALSE, ch, NULL, victim, TO_VICT);
      act("$n dances around $N in a merry little jig.", TRUE, ch, NULL, victim, TO_NOTVICT);
    }
    break;
  case 3:
    if ((victim = rol_residual_random_player(ch)) != NULL)
    {
      rol_residual_steal(ch, victim);
      act("$n whistles innocently and grins mischievously.", TRUE, ch, NULL, NULL, TO_ROOM);
    }
    break;
  case 4:
    if ((victim = rol_residual_random_player(ch)) != NULL)
    {
      act("With a cry of laughter, $n falls down giggling at you.", FALSE, ch, NULL, victim,
          TO_VICT);
      act("With a cry of laughter, $n falls down giggling at $N.", TRUE, ch, NULL, victim,
          TO_NOTVICT);
    }
    break;
  case 5:
    if ((victim = rol_residual_random_player(ch)) != NULL)
    {
      act("$n looks at you and says, 'You're new here, eh?'", FALSE, ch, NULL, victim, TO_VICT);
      act("$n looks at $N and asks whether $E is new here.", TRUE, ch, NULL, victim, TO_NOTVICT);
    }
    break;
  default:
    break;
  }
}

static void rol_residual_ancient_brownie(struct spec_event_context *context, struct char_data *ch)
{
  struct char_data *victim = FIGHTING(ch);
  struct char_data *pet_owner = NULL;
  int result;

  if (victim == NULL || rand_number(0, 5) != 0 || rand_number(0, 3) != 0)
    return;
  act("$n dives at your ankles and bites viciously; your blood burns like acid!", FALSE, ch, NULL,
      victim, TO_VICT);
  act("$n dives at $N's ankles and bites viciously!", TRUE, ch, NULL, victim, TO_NOTVICT);
  if (IS_NPC(victim))
  {
    if (victim->master != NULL && !IS_NPC(victim->master))
      pet_owner = victim->master;
    result = damage(ch, victim, MAX(1, GET_HIT(victim) + 1), -1, DAM_ACID, FALSE);
    if (result < 0)
      context->invalidation |= SPEC_INVALIDATE_TARGET;
    if (pet_owner != NULL && GET_POS(pet_owner) > POS_DEAD)
    {
      act("The sudden loss of your companion lashes back as a blinding headache!", FALSE, ch, NULL,
          pet_owner, TO_VICT);
      call_magic(ch, pet_owner, NULL, SPELL_BLINDNESS, 0, GET_LEVEL(ch), CAST_INNATE);
    }
    return;
  }
  result = damage(ch, victim, 200, -1, DAM_ACID, FALSE);
  if (result < 0)
  {
    context->invalidation |= SPEC_INVALIDATE_TARGET;
    return;
  }
  if (!AFF_FLAGGED(victim, AFF_PARALYZED) && !paralysis_immunity(victim))
    call_magic(ch, victim, NULL, SPELL_HOLD_PERSON, 0, GET_LEVEL(ch), CAST_INNATE);
}

static int rol_residual_roll_with_it(struct spec_event_context *context, struct char_data *ch)
{
  struct char_data *caster = context->actor;

  if (caster == NULL || caster == ch || context->command < 0 ||
      complete_cmd_info[context->command].command_pointer != do_gen_cast || IS_NPC(caster) ||
      IS_INCORPOREAL(caster) || GET_POS(ch) <= POS_INCAP || AFF_FLAGGED(ch, AFF_PARALYZED))
    return FALSE;

  do_gen_cast(caster, context->argument, context->command,
              complete_cmd_info[context->command].subcmd);
  if (FIGHTING(ch) == NULL || rand_number(0, 5) != 0)
    return TRUE;

  act("$n notices your incantation, rolls aside, and smashes you in the throat!", FALSE, ch, NULL,
      caster, TO_VICT);
  act("$n notices $N's incantation, rolls aside, and smashes $M in the throat!", TRUE, ch, NULL,
      caster, TO_NOTVICT);
  if (damage(ch, caster, 50, -1, DAM_BLUDGEON, FALSE) < 0)
    context->invalidation |= SPEC_INVALIDATE_ACTOR;
  else if (GET_POS(caster) > POS_SITTING)
    GET_POS(caster) = POS_SITTING;
  return TRUE;
}

static void rol_residual_activity(const struct rol_residual_mobile_profile *profile,
                                  struct char_data *ch)
{
  switch (profile->effect)
  {
  case ROL_RESIDUAL_BEAVIS:
    rol_residual_beavis(ch, false);
    break;
  case ROL_RESIDUAL_BUTTHEAD:
    rol_residual_beavis(ch, true);
    break;
  case ROL_RESIDUAL_FINN:
    rol_residual_finn(ch);
    break;
  case ROL_RESIDUAL_FAERIE:
    rol_residual_faerie(ch);
    break;
  case ROL_RESIDUAL_VANISH:
    if (char_has_mud_event(ch, ePURGEMOB) == NULL)
      attach_mud_event(new_mud_event(ePURGEMOB, ch, NULL), 14400);
    break;
  case ROL_RESIDUAL_ANCIENT_BROWNIE:
  case ROL_RESIDUAL_ROLL_WITH_IT:
    break;
  }
}

int rol_residual_mobile_typed(struct spec_event_context *context)
{
  const struct rol_residual_mobile_profile *profile;
  struct char_data *ch;

  if (context == NULL || context->owner_type != SPEC_OWNER_MOBILE || context->owner == NULL)
    return FALSE;
  ch = context->owner;
  if (!IS_NPC(ch) || !VALID_ROOM_RNUM(IN_ROOM(ch)) ||
      (profile = rol_residual_mobile_profile_for(GET_MOB_VNUM(ch))) == NULL)
    return FALSE;

  switch (context->event)
  {
  case SPEC_EVENT_COMMAND:
    if (profile->effect == ROL_RESIDUAL_ROLL_WITH_IT)
      return rol_residual_roll_with_it(context, ch);
    break;
  case SPEC_EVENT_MOBILE_ACTIVITY:
    rol_residual_activity(profile, ch);
    break;
  case SPEC_EVENT_MOBILE_COMBAT_TURN:
    if (profile->effect == ROL_RESIDUAL_ANCIENT_BROWNIE)
      rol_residual_ancient_brownie(context, ch);
    break;
  default:
    break;
  }
  return FALSE;
}
