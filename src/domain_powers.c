/* ***********************************************************************
 *    File:   domain_powers.c                        Part of LuminariMUD  *
 * Purpose:   The code for handling all the individual domain powers      *
 *  Author:   Zusuk                                                       *
 *  Header:   domains_schools.h                                           *
 ************************************************************************ */

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "screen.h"
#include "handler.h"
#include "spells.h"
#include "domains_schools.h"
#include "assign_wpn_armor.h"
#include "mud_event.h"
#include "actions.h"
#include "fight.h"
#include "act.h"

/* inflict disease with your touch, requires successful touch attack */
ACMD(do_eviltouch)
{
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_EVIL_TOUCH))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_EVIL_TOUCH))
  {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_EVIL_TOUCH)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use this feat again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    {
      vict = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  /* Perform the unarmed touch attack */
  if (attack_roll(ch, vict, ATTACK_TYPE_UNARMED, TRUE, 1) > 0)
  {
    act("A \trred\tn aura shoots from your fingertips towards $N!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n shoots a \trred\tn aura towards you!", FALSE, ch, 0, vict, TO_VICT);
    act("$n shoots a \trred\tn aura towards $N!", FALSE, ch, 0, vict, TO_NOTVICT);
    call_magic(ch, vict, 0, SPELL_EYEBITE, 0, CLASS_LEVEL(ch, CLASS_CLERIC), CAST_INNATE);
  }
  else
  {
    /* missed */
    act("A \trred\tn aura shoots from your fingertips towards $N, but fails to land!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n shoots a \trred\tn aura towards you, but you are able to dodge it!", FALSE, ch, 0, vict, TO_VICT);
    act("$n shoots a \trred\tn aura towards $N, but $N is able to dodge it!", FALSE, ch, 0, vict, TO_NOTVICT);
  }

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_EVIL_TOUCH);

  USE_STANDARD_ACTION(ch);
}

ACMD(do_blessedtouch)
{
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_BLESSED_TOUCH))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_BLESSED_TOUCH))
  {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  if (!GROUP(ch))
  {
    send_to_char(ch, "You can't use this ability if you're not in a group (group new)!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_BLESSED_TOUCH)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use this feat again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  if (!*argument)
  {
    vict = ch;
  }
  else
  {
    one_argument(argument, arg, sizeof(arg));

    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
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

  if (vict == ch)
  {
    send_to_char(ch, "You heal yourself with your power!\r\n");
    act("$n glows white and gains some power!", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  else
  {
    act("A \tWwhite\tn aura shoots from your fingertips towards $N!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n shoots a \tWwhite\tn aura towards you!", FALSE, ch, 0, vict, TO_VICT);
    act("$n shoots a \tWwhite\tn aura towards $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  call_magic(ch, vict, NULL, SPELL_AID, 0, CLASS_LEVEL(ch, CLASS_CLERIC), CAST_INNATE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_BLESSED_TOUCH);

  USE_STANDARD_ACTION(ch);
}

ACMD(do_goodtouch)
{
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_GOOD_TOUCH))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_GOOD_TOUCH))
  {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_GOOD_TOUCH)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use this feat again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  if (!*argument)
  {
    vict = ch;
  }
  else
  {
    one_argument(argument, arg, sizeof(arg));

    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
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

  if (vict == ch)
  {
    send_to_char(ch, "You heal yourself with your power!\r\n");
    act("$n glows white and heals some afflictions!", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  else
  {
    act("A \tWwhite\tn aura shoots from your fingertips towards $N!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n shoots a \tWwhite\tn aura towards you!", FALSE, ch, 0, vict, TO_VICT);
    act("$n shoots a \tWwhite\tn aura towards $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  mag_unaffects(CLASS_LEVEL(ch, CLASS_CLERIC), ch, vict,
                NULL, SPELL_REMOVE_POISON, 0, CAST_INNATE);
  mag_unaffects(CLASS_LEVEL(ch, CLASS_CLERIC), ch, vict,
                NULL, SPELL_REMOVE_DISEASE, 0, CAST_INNATE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_GOOD_TOUCH);

  USE_STANDARD_ACTION(ch);
}

ACMD(do_evilscythe)
{
}

ACMD(do_goodlance)
{
}

ACMD(do_healingtouch)
{
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_HEALING_TOUCH))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_HEALING_TOUCH))
  {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_HEALING_TOUCH)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use this feat again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  if (!*argument)
  {
    vict = ch;
  }
  else
  {
    one_argument(argument, arg, sizeof(arg));

    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

#if !defined(CAMPAIGN_DL) && !defined(CAMPAIGN_FR)
  if (GET_HIT(vict) > GET_MAX_HIT(vict) / 2)
  {
    send_to_char(ch, "Your target is not injured enough to use this feat!\r\n");
    return;
  }
#endif

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  if (vict == ch)
  {
    send_to_char(ch, "You heal yourself with your power!\r\n");
    act("$n glows white and heals some wounds!", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  else
  {
    act("A \tWwhite\tn aura shoots from your fingertips towards $N!", FALSE, ch, 0, vict, TO_CHAR);
    act("$n shoots a \tWwhite\tn aura towards you!", FALSE, ch, 0, vict, TO_VICT);
    act("$n shoots a \tWwhite\tn aura towards $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  }
  GET_HIT(vict) += 20 + dice(1, 4) + CLASS_LEVEL(ch, CLASS_CLERIC) / 2;

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_HEALING_TOUCH);

  USE_STANDARD_ACTION(ch);
}

/* summon a wizard eye */
ACMD(do_eyeofknowledge)
{
  int uses_remaining = 0;

  if (!has_domain_power(ch, DOMAIN_POWER_EYE_OF_KNOWLEDGE))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_EYE_OF_KNOWLEDGE))
  {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_EYE_OF_KNOWLEDGE)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use this feat again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  call_magic(ch, NULL, 0, SPELL_WIZARD_EYE, 0, CLASS_LEVEL(ch, CLASS_CLERIC), CAST_INNATE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_EYE_OF_KNOWLEDGE);

  USE_STANDARD_ACTION(ch);
}

/* able to replicate mirror image spell */
ACMD(do_copycat)
{
  int uses_remaining = 0;

  if (!has_domain_power(ch, DOMAIN_POWER_COPYCAT))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_COPYCAT))
  {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_COPYCAT)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use this feat again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  call_magic(ch, ch, 0, SPELL_MIRROR_IMAGE, 0, CLASS_LEVEL(ch, CLASS_CLERIC), CAST_INNATE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_COPYCAT);

  USE_STANDARD_ACTION(ch);
}

/* able to replicate invisibility sphere spell */
ACMD(do_massinvis)
{
  int uses_remaining = 0;

  if (!has_domain_power(ch, DOMAIN_POWER_MASS_INVIS))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_MASS_INVIS))
  {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  if (!GROUP(ch))
  {
    send_to_char(ch, "This will only work if you are grouped!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_MASS_INVIS)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use this feat again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  call_magic(ch, ch, 0, SPELL_INVISIBILITY_SPHERE, 0, CLASS_LEVEL(ch, CLASS_CLERIC), CAST_INNATE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_MASS_INVIS);

  USE_STANDARD_ACTION(ch);
}

/* engine for aura of protection */
#define AURA_OF_PROTECTION_AFFECTS 4
void perform_auraofprotection(struct char_data *ch)
{
  struct affected_type af[AURA_OF_PROTECTION_AFFECTS];
  int i = 0, duration = 0;
  struct char_data *tch = NULL;

  if (!GROUP(ch))
    return;

  duration = 1;

  /* init affect array */
  for (i = 0; i < AURA_OF_PROTECTION_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SKILL_AURA_OF_PROTECTION;
    af[i].duration = duration;
  }

  af[0].location = APPLY_AC_NEW;
  af[0].modifier = MAX(1, CLASS_LEVEL(ch, CLASS_CLERIC) / 6);

  af[1].location = APPLY_SAVING_REFL;
  af[1].modifier = MAX(1, CLASS_LEVEL(ch, CLASS_CLERIC) / 6);
  af[2].location = APPLY_SAVING_FORT;
  af[2].modifier = MAX(1, CLASS_LEVEL(ch, CLASS_CLERIC) / 6);
  af[3].location = APPLY_SAVING_WILL;
  af[3].modifier = MAX(1, CLASS_LEVEL(ch, CLASS_CLERIC) / 6);

  USE_STANDARD_ACTION(ch);

  act("$n glows with a \tWwhite\tn aura!!", FALSE, ch, NULL, NULL, TO_ROOM);
  act("You activate your protective aura!!", FALSE, ch, NULL, NULL, TO_CHAR);

  while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) !=
         NULL)
  {
    if (IN_ROOM(tch) != IN_ROOM(ch))
      continue;
    if (affected_by_spell(tch, SKILL_AURA_OF_PROTECTION))
      continue;
    for (i = 0; i < AURA_OF_PROTECTION_AFFECTS; i++)
      affect_join(tch, af + i, FALSE, FALSE, FALSE, FALSE);
    act("A protective aura from $n enhances you!", FALSE, ch, NULL, tch, TO_VICT);
    act("A protective aura surrounds $N!", FALSE, ch, NULL, tch, TO_ROOM);
  }

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_AURA_OF_PROTECTION);
}

ACMD(do_auraofprotection)
{
  int uses_remaining = 0;

  if (!IS_NPC(ch) && !HAS_FEAT(ch, FEAT_AURA_OF_PROTECTION))
  {
    send_to_char(ch, "You don't know how to!\r\n");
    return;
  }

  if (!has_domain_power(ch, DOMAIN_POWER_AURA_OF_PROTECTION))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!GROUP(ch))
  {
    send_to_char(ch, "This will only work if you are grouped!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_AURA_OF_PROTECTION)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use your protection aura.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  perform_auraofprotection(ch);
}
#undef AURA_OF_PROTECTION_AFFECTS

/* //this is in act.other.c, shared with monks 'outsider' feat
ACMD(do_ethshift) {
}
*/

ACMD(do_battlerage)
{
  struct affected_type af, aftwo;
  int bonus = 0, duration = 0, uses_remaining = 0;

  if (AFF_FLAGGED(ch, AFF_FATIGUED))
  {
    send_to_char(ch, "You are too fatigued to battle rage!\r\n");
    return;
  }

  if (affected_by_spell(ch, SKILL_RAGE))
  {
    send_to_char(ch, "You are already raging!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_BATTLE_RAGE))
  {
    send_to_char(ch, "You don't know how to battle rage.\r\n");
    return;
  }

  if (!IS_NPC(ch) && ((uses_remaining = daily_uses_remaining(ch, FEAT_BATTLE_RAGE)) == 0))
  {
    send_to_char(ch, "You must recover before you can go into a battle rage.\r\n");
    return;
  }

  /* duration */
  duration = 5;

  /* bonus */
  bonus = CLASS_LEVEL(ch, CLASS_CLERIC) / 4;
  if (bonus <= 0)
  {
    send_to_char(ch, "You are not powerful enough to battle rage! (minimum 4 levels in clerci class to use)\r\n");
    return;
  }

  send_to_char(ch, "You go into a battle \tRR\trA\tRG\trE\tn!.\r\n");
  act("$n goes into a battle \tRR\trA\tRG\trE\tn!", FALSE, ch, 0, 0, TO_ROOM);

  new_affect(&af);
  new_affect(&aftwo);

  af.spell = SKILL_RAGE;
  af.duration = duration;
  af.location = APPLY_HITROLL;
  af.modifier = bonus;

  aftwo.spell = SKILL_RAGE;
  aftwo.duration = duration;
  aftwo.location = APPLY_DAMROLL;
  aftwo.modifier = bonus;

  affect_to_char(ch, &af);
  affect_to_char(ch, &aftwo);

  if (!IS_NPC(ch))
  {
    start_daily_use_cooldown(ch, FEAT_BATTLE_RAGE);
    USE_STANDARD_ACTION(ch);
  }
}

/* engine for destructive aura */
#define DESTRUCTIVE_AURA_AFFECTS 1
void perform_destructiveaura(struct char_data *ch)
{
  struct affected_type af[DESTRUCTIVE_AURA_AFFECTS];
  int i = 0, duration = 0;
  struct char_data *tch = NULL;

  if (!GROUP(ch))
    return;

  duration = 1;

  /* init affect array */
  for (i = 0; i < DESTRUCTIVE_AURA_AFFECTS; i++)
  {
    new_affect(&(af[i]));
    af[i].spell = SKILL_DESTRUCTIVE_AURA;
    af[i].duration = duration;
  }

  af[0].location = APPLY_DAMROLL;
  af[0].modifier = MAX(1, CLASS_LEVEL(ch, CLASS_CLERIC) / 2);

  USE_STANDARD_ACTION(ch);

  act("$n glows with a ominous \trred\tn aura!!", FALSE, ch, NULL, NULL, TO_ROOM);
  act("You activate your destructive aura!!", FALSE, ch, NULL, NULL, TO_CHAR);

  while ((tch = (struct char_data *)simple_list(GROUP(ch)->members)) !=
         NULL)
  {
    if (IN_ROOM(tch) != IN_ROOM(ch))
      continue;
    if (affected_by_spell(tch, SKILL_DESTRUCTIVE_AURA))
      continue;
    for (i = 0; i < DESTRUCTIVE_AURA_AFFECTS; i++)
      affect_join(tch, af + i, FALSE, FALSE, FALSE, FALSE);
    act("A destructive aura from $n enhances you!", FALSE, ch, NULL, tch, TO_VICT);
    act("A destructive aura surrounds $N!", FALSE, ch, NULL, tch, TO_ROOM);
  }

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_DESTRUCTIVE_AURA);
}

/* destructive aura - give damage bonus to your group */
ACMD(do_destructiveaura)
{
  int uses_remaining = 0;

  if (!IS_NPC(ch) && !HAS_FEAT(ch, FEAT_DESTRUCTIVE_AURA))
  {
    send_to_char(ch, "You don't know how to!\r\n");
    return;
  }

  if (!has_domain_power(ch, DOMAIN_POWER_DESTRUCTIVE_AURA))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!GROUP(ch))
  {
    send_to_char(ch, "This will only work if you are grouped!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_DESTRUCTIVE_AURA)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use destructive aura.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  perform_destructiveaura(ch);
}
#undef DESTRUCTIVE_AURA_AFFECTS

/* an unrestricted smite! similar to smite-evil/smite-good, not as powerful
   but less restrictive */
ACMD(do_destructivesmite)
{
  int uses_remaining = 0;

  if (IS_NPC(ch) || !HAS_FEAT(ch, FEAT_DESTRUCTIVE_SMITE))
  {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }

  if (!has_domain_power(ch, DOMAIN_POWER_DESTRUCTIVE_SMITE))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_DESTRUCTIVE_SMITE)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use destructive smite.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  perform_smite(ch, SMITE_TYPE_DESTRUCTION);
}

/* you can fire lightning out of your fingers like a sith lord! :p */
ACMD(do_lightningarc)
{
  int dam = 0;
  int damtype = DAM_ELECTRIC;
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_LIGHTNING_ARC))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_LIGHTNING_ARC))
  {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  /*
  if (!CLASS_LEVEL(ch, CLASS_CLERIC)) {
    send_to_char(ch, "You do not have any clerical powers!\r\n");
    return;
  }
  */

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_LIGHTNING_ARC)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use another lightning arc.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    {
      vict = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  dam = 10 + dice(1, 6) + CLASS_LEVEL(ch, CLASS_CLERIC) / 2;
  act("An \tBarc of lightning\tn shoots from your fingertips towards $N!", FALSE, ch, 0, vict, TO_CHAR);
  act("$n shoots an \tBarc of lightning\tn towards you!", FALSE, ch, 0, vict, TO_VICT);
  act("$n shoots an \tBarc of lightning\tn towards $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  damage(ch, vict, dam, -1, damtype, FALSE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_LIGHTNING_ARC);

  USE_STANDARD_ACTION(ch);
}

/* earth domain - fire an acid dart at an opponent */
ACMD(do_aciddart)
{
  int dam = 0;
  int damtype = DAM_ACID;
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_ACID_DART))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_ACID_DART))
  {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  /*
  if (!CLASS_LEVEL(ch, CLASS_CLERIC)) {
    send_to_char(ch, "You do not have any clerical powers!\r\n");
    return;
  }
  */

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_ACID_DART)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use another acid dart.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    {
      vict = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  dam = 10 + dice(1, 6) + CLASS_LEVEL(ch, CLASS_CLERIC) / 2;
  act("An \tGacid dart\tn shoots from your fingertips towards $N!", FALSE, ch, 0, vict, TO_CHAR);
  act("$n shoots an \tGacid dart\tn towards you!", FALSE, ch, 0, vict, TO_VICT);
  act("$n shoots an \tGacid dart\tn towards $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  damage(ch, vict, dam, -1, damtype, FALSE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_ACID_DART);

  USE_STANDARD_ACTION(ch);
}

/* release a bolt of fire at your opponent! */
ACMD(do_firebolt)
{
  int dam = 0;
  int damtype = DAM_FIRE;
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_FIRE_BOLT))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_FIRE_BOLT))
  {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  /*
  if (!CLASS_LEVEL(ch, CLASS_CLERIC)) {
    send_to_char(ch, "You do not have any clerical powers!\r\n");
    return;
  }
  */

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_FIRE_BOLT)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use another fire bolt.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    {
      vict = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  dam = 10 + dice(1, 6) + CLASS_LEVEL(ch, CLASS_CLERIC) / 2;
  act("A \tRfire bolt\tn shoots from your fingertips towards $N!", FALSE, ch, 0, vict, TO_CHAR);
  act("$n shoots a1 \tRfire bolt\tn towards you!", FALSE, ch, 0, vict, TO_VICT);
  act("$n shoots a \tRfire bolt\tn towards $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  damage(ch, vict, dam, -1, damtype, FALSE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_FIRE_BOLT);

  USE_STANDARD_ACTION(ch);
}

/* fire a deadly icicle at an opponent! */
ACMD(do_icicle)
{
  int dam = 0;
  int damtype = DAM_COLD;
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_ICICLE))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_ICICLE))
  {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  /*
  if (!CLASS_LEVEL(ch, CLASS_CLERIC)) {
    send_to_char(ch, "You do not have any clerical powers!\r\n");
    return;
  }
  */

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_ICICLE)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use another icicle.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    {
      vict = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  dam = 10 + dice(1, 6) + CLASS_LEVEL(ch, CLASS_CLERIC) / 2;
  act("An \tWicicle\tn shoots from your fingertips towards $N!", FALSE, ch, 0, vict, TO_CHAR);
  act("$n shoots an \tWicicle\tn towards you!", FALSE, ch, 0, vict, TO_VICT);
  act("$n shoots an \tWicicle\tn towards $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  damage(ch, vict, dam, -1, damtype, FALSE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_ICICLE);

  USE_STANDARD_ACTION(ch);
}

/* can 'curse' an opponent with your touch */
ACMD(do_cursetouch)
{
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_CURSE_TOUCH))
  {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_CURSE_TOUCH))
  {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  /*
  if (!CLASS_LEVEL(ch, CLASS_CLERIC)) {
    send_to_char(ch, "You do not have any clerical powers!\r\n");
    return;
  }
  */

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_CURSE_TOUCH)) == 0)
  {
    send_to_char(ch, "You must recover the divine energy required to use this feat again.\r\n");
    return;
  }

  if (uses_remaining < 0)
  {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  one_argument(argument, arg, sizeof(arg));
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
  {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
    {
      vict = FIGHTING(ch);
    }
    else
    {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (vict == ch)
  {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL))
  {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
      ch->next_in_room != vict && vict->next_in_room != ch)
  {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  act("A \trred\tn aura shoots from your fingertips towards $N!", FALSE, ch, 0, vict, TO_CHAR);
  act("$n shoots a \trred\tn aura towards you!", FALSE, ch, 0, vict, TO_VICT);
  act("$n shoots a \trred\tn aura towards $N!", FALSE, ch, 0, vict, TO_NOTVICT);
  call_magic(ch, vict, 0, SPELL_CURSE, 0, CLASS_LEVEL(ch, CLASS_CLERIC), CAST_INNATE);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_CURSE_TOUCH);

  USE_STANDARD_ACTION(ch);
}
