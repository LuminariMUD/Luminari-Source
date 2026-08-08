/**
 * @file spec_objects.h
 * Public API for general object special procedures.
 */

#ifndef LUMINARI_SPEC_OBJECTS_H
#define LUMINARI_SPEC_OBJECTS_H

struct char_data;
struct obj_data;

void weapons_spells(const char *to_ch, const char *to_vict, const char *to_room,
                    struct char_data *ch, struct char_data *vict, struct obj_data *obj, int spl);

int acidstaff(struct char_data *ch, void *me, int cmd, const char *argument);
int acidsword(struct char_data *ch, void *me, int cmd, const char *argument);
int air_sphere(struct char_data *ch, void *me, int cmd, const char *argument);
int ancient_moonblade(struct char_data *ch, void *me, int cmd, const char *argument);
int angel_leggings(struct char_data *ch, void *me, int cmd, const char *argument);
int bloodaxe(struct char_data *ch, void *me, int cmd, const char *argument);
int bolthammer(struct char_data *ch, void *me, int cmd, const char *argument);
int celestial_sword(struct char_data *ch, void *me, int cmd, const char *argument);
int ches(struct char_data *ch, void *me, int cmd, const char *argument);
int clang_bracer(struct char_data *ch, void *me, int cmd, const char *argument);
int courage(struct char_data *ch, void *me, int cmd, const char *argument);
int disruption_mace(struct char_data *ch, void *me, int cmd, const char *argument);
int dorfaxe(struct char_data *ch, void *me, int cmd, const char *argument);
int dragon_robes(struct char_data *ch, void *me, int cmd, const char *argument);
int dragonbone_hammer(struct char_data *ch, void *me, int cmd, const char *argument);
int etherealness(struct char_data *ch, void *me, int cmd, const char *argument);
int flaming_scimitar(struct char_data *ch, void *me, int cmd, const char *argument);
int flamingwhip(struct char_data *ch, void *me, int cmd, const char *argument);
int floating_teleport(struct char_data *ch, void *me, int cmd, const char *argument);
int fog_dagger(struct char_data *ch, void *me, int cmd, const char *argument);
int forest_idol(struct char_data *ch, void *me, int cmd, const char *argument);
int frosty_scimitar(struct char_data *ch, void *me, int cmd, const char *argument);
int greatsword(struct char_data *ch, void *me, int cmd, const char *argument);
int halberd(struct char_data *ch, void *me, int cmd, const char *argument);
int haste_bracers(struct char_data *ch, void *me, int cmd, const char *argument);
int hellfire(struct char_data *ch, void *me, int cmd, const char *argument);
int helmblade(struct char_data *ch, void *me, int cmd, const char *argument);
int magma(struct char_data *ch, void *me, int cmd, const char *argument);
int malevolence(struct char_data *ch, void *me, int cmd, const char *argument);
int menzo_chokers(struct char_data *ch, void *me, int cmd, const char *argument);
int monk_glove(struct char_data *ch, void *me, int cmd, const char *argument);
int monk_glove_cold(struct char_data *ch, void *me, int cmd, const char *argument);
int nutty_bracer(struct char_data *ch, void *me, int cmd, const char *argument);
int prismorb(struct char_data *ch, void *me, int cmd, const char *argument);
int purity(struct char_data *ch, void *me, int cmd, const char *argument);
int rughnark(struct char_data *ch, void *me, int cmd, const char *argument);
int rune_scimitar(struct char_data *ch, void *me, int cmd, const char *argument);
int sarn(struct char_data *ch, void *me, int cmd, const char *argument);
int skullsmasher(struct char_data *ch, void *me, int cmd, const char *argument);
int snakewhip(struct char_data *ch, void *me, int cmd, const char *argument);
int sparksword(struct char_data *ch, void *me, int cmd, const char *argument);
int speed_gaunts(struct char_data *ch, void *me, int cmd, const char *argument);
int spiderdagger(struct char_data *ch, void *me, int cmd, const char *argument);
int spikeshield(struct char_data *ch, void *me, int cmd, const char *argument);
int stability_boots(struct char_data *ch, void *me, int cmd, const char *argument);
int star_circlet(struct char_data *ch, void *me, int cmd, const char *argument);
int storage_chest(struct char_data *ch, void *me, int cmd, const char *argument);
int tormblade(struct char_data *ch, void *me, int cmd, const char *argument);
int tyrantseye(struct char_data *ch, void *me, int cmd, const char *argument);
int vengeance(struct char_data *ch, void *me, int cmd, const char *argument);
int viperdagger(struct char_data *ch, void *me, int cmd, const char *argument);
int warbow(struct char_data *ch, void *me, int cmd, const char *argument);
int whisperwind(struct char_data *ch, void *me, int cmd, const char *argument);
int witherdirk(struct char_data *ch, void *me, int cmd, const char *argument);
int xvim_artifact(struct char_data *ch, void *me, int cmd, const char *argument);
int xvim_normal(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_OBJECTS_H */
