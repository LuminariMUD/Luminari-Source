/**************************************************************************
 *  File: rol_feats.h                                  Part of LuminariMUD *
 *  Usage: Feats converted from Realms of Luminari player skills.          *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#ifndef _ROL_FEATS_H_
#define _ROL_FEATS_H_

/* shadow: covert tailing */
void stop_shadowing(struct char_data *ch, bool notify);
void clear_shadow_links(struct char_data *ch);
void shadow_movement_complete(struct char_data *ch);
void shadowers_follow(struct char_data *ch, room_rnum was_in, int dir);

/* establish camp: wilderness campsite */
int camp_recovery_bonus(struct char_data *ch, int gain);
#ifdef LUMINARI_CUTEST
void test_camp_shelter_char(struct char_data *ch);
#endif

/* accompany: supporting another performer */
void stop_accompanying(struct char_data *ch, bool notify);
void clear_accompany_links(struct char_data *ch);
int accompaniment_bonus(struct char_data *ch);
bool accompany_takeover(struct char_data *ch, int performance_num);

/* commands */
ACMD_DECL(do_shadow);
ACMD_DECL(do_calm);
ACMD_DECL(do_camp);
ACMD_DECL(do_garrote);
ACMD_DECL(do_accompany);

ACMDCHECK(can_shadow);
ACMDCHECK(can_calm);
ACMDCHECK(can_camp);
ACMDCHECK(can_garrote);
ACMDCHECK(can_accompany);

#endif /* _ROL_FEATS_H_ */
