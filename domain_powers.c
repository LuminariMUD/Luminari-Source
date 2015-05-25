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

ACMD(do_lightningarc) {
  int dam = 0;
  int damtype = DAM_ELECTRIC;
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_LIGHTNING_ARC)) {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_LIGHTNING_ARC)) {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  /*
  if (!CLASS_LEVEL(ch, CLASS_CLERIC)) {
    send_to_char(ch, "You do not have any clerical powers!\r\n");
    return;
  }
  */

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_LIGHTNING_ARC)) == 0) {
    send_to_char(ch, "You must recover the divine energy required to use another lightning arc.\r\n");
    return;
  }

  if (uses_remaining < 0) {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  one_argument(argument, arg);
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
          ch->next_in_room != vict && vict->next_in_room != ch) {
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

ACMD(do_aciddart) {
  int dam = 0;
  int damtype = DAM_ACID;
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_ACID_DART)) {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_ACID_DART)) {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  /*
  if (!CLASS_LEVEL(ch, CLASS_CLERIC)) {
    send_to_char(ch, "You do not have any clerical powers!\r\n");
    return;
  }
  */

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_ACID_DART)) == 0) {
    send_to_char(ch, "You must recover the divine energy required to use another acid dart.\r\n");
    return;
  }

  if (uses_remaining < 0) {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  one_argument(argument, arg);
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
          ch->next_in_room != vict && vict->next_in_room != ch) {
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

ACMD(do_firebolt) {
  int dam = 0;
  int damtype = DAM_FIRE;
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_FIRE_BOLT)) {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_FIRE_BOLT)) {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  /*
  if (!CLASS_LEVEL(ch, CLASS_CLERIC)) {
    send_to_char(ch, "You do not have any clerical powers!\r\n");
    return;
  }
  */

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_FIRE_BOLT)) == 0) {
    send_to_char(ch, "You must recover the divine energy required to use another fire bolt.\r\n");
    return;
  }

  if (uses_remaining < 0) {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  one_argument(argument, arg);
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
          ch->next_in_room != vict && vict->next_in_room != ch) {
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

ACMD(do_icicle) {
  int dam = 0;
  int damtype = DAM_COLD;
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_ICICLE)) {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_ICICLE)) {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  /*
  if (!CLASS_LEVEL(ch, CLASS_CLERIC)) {
    send_to_char(ch, "You do not have any clerical powers!\r\n");
    return;
  }
  */

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_ICICLE)) == 0) {
    send_to_char(ch, "You must recover the divine energy required to use another icicle.\r\n");
    return;
  }

  if (uses_remaining < 0) {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  one_argument(argument, arg);
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
          ch->next_in_room != vict && vict->next_in_room != ch) {
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


ACMD(do_cursetouch) {
  int uses_remaining = 0;
  char arg[MAX_INPUT_LENGTH] = {'\0'};
  struct char_data *vict = NULL;

  if (!has_domain_power(ch, DOMAIN_POWER_CURSE_TOUCH)) {
    send_to_char(ch, "You do not have that domain power!\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_CURSE_TOUCH)) {
    send_to_char(ch, "You do not have that feat!\r\n");
    return;
  }

  /*
  if (!CLASS_LEVEL(ch, CLASS_CLERIC)) {
    send_to_char(ch, "You do not have any clerical powers!\r\n");
    return;
  }
  */

  if ((uses_remaining = daily_uses_remaining(ch, FEAT_CURSE_TOUCH)) == 0) {
    send_to_char(ch, "You must recover the divine energy required to use another icicle.\r\n");
    return;
  }

  if (uses_remaining < 0) {
    send_to_char(ch, "You are not experienced enough.\r\n");
    return;
  }

  one_argument(argument, arg);
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch))) {
      vict = FIGHTING(ch);
    } else {
      send_to_char(ch, "Target who?\r\n");
      return;
    }
  }

  if (vict == ch) {
    send_to_char(ch, "Aren't we funny today...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SINGLEFILE) &&
          ch->next_in_room != vict && vict->next_in_room != ch) {
    send_to_char(ch, "You simply can't reach that far.\r\n");
    return;
  }

  call_magic(ch, vict, 0, SPELL_CURSE, CLASS_LEVEL(ch, CLASS_CLERIC), CAST_INNATE);

  act("A \trred\tn aura shoots from your fingertips towards $N!", FALSE, ch, 0, vict, TO_CHAR);
  act("$n shoots a \trred\tn aura towards you!", FALSE, ch, 0, vict, TO_VICT);
  act("$n shoots a \trred\tn aura towards $N!", FALSE, ch, 0, vict, TO_NOTVICT);

  if (!IS_NPC(ch))
    start_daily_use_cooldown(ch, FEAT_CURSE_TOUCH);

  USE_STANDARD_ACTION(ch);
}
