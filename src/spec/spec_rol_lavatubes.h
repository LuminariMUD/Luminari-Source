/**
 * @file spec/spec_rol_lavatubes.h
 * Converted Realms of Luminari Lavatubes special procedures.
 */

#ifndef LUMINARI_SPEC_ROL_LAVATUBES_H
#define LUMINARI_SPEC_ROL_LAVATUBES_H

struct char_data;
struct spec_event_context;

enum rol_lavatubes_snowvulture_outcome
{
  ROL_LAVATUBES_SNOWVULTURE_NONE = 0,
  ROL_LAVATUBES_SNOWVULTURE_SQUEAK,
  ROL_LAVATUBES_SNOWVULTURE_FLAP,
  ROL_LAVATUBES_SNOWVULTURE_DEVOUR
};

int rol_lavatubes_mobile(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_lavatubes_mobile_typed(struct spec_event_context *context);
int rol_lavatubes_object(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_lavatubes_object_typed(struct spec_event_context *context);
int rol_lavatubes_room(struct char_data *ch, void *me, int cmd, const char *argument);
int rol_lavatubes_room_typed(struct spec_event_context *context);

int rol_lavatubes_skeleton_key_break_chance(int dexterity_bonus);
enum rol_lavatubes_snowvulture_outcome rol_lavatubes_snowvulture_outcome(int roll);

#endif /* LUMINARI_SPEC_ROL_LAVATUBES_H */
