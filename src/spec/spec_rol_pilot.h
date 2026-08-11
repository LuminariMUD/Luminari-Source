/**
 * @file spec/spec_rol_pilot.h
 * Pilot Realms of Luminari special-procedure adapters.
 */

#ifndef LUMINARI_SPEC_ROL_PILOT_H
#define LUMINARI_SPEC_ROL_PILOT_H

struct char_data;

int rol_breath_attack_fire(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_hulburg_beholder_major(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_hulburg_beholder_minor(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_money_changer(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_plant_attacks_blindness(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_plant_attacks_paralysis(struct char_data *ch, void *me, int cmd, const char *argument);

int rol_cemetery_black_blade(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_cemetery_cloak_meteors(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_cemetery_disruption(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_cemetery_gleaming_blade(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_cemetery_lightsaber(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_cemetery_skeletal_hand(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_flaming_tanthorian(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_longsword_tanthorian(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_murlynds_spoon(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_muspel_bec_de_corbin(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_muspel_crystal_scimitar(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_muspel_dagger_whispers(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_muspel_dragon_lance(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_muspel_duergar_battlehammer(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_muspel_recurve_bow(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_muspel_spider_dagger(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_obj_drain(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_thorn_shield(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ROL_PILOT_H */
