/* 
 * File:   traps.h
 * Author: Zusuk
 *
 * Created on 2 נובמבר 2014, 22:39
 */

#ifndef TRAPS_H
#define TRAPS_H

/* the trap event that we attach to characters */
struct trap_event {
  struct char_data *ch;
  int effect;
};

/* fuctions defined in traps.c */
bool check_trap(struct char_data *ch, int trap_type, int room, struct obj_data *obj, int dir);
void set_off_trap(struct char_data *ch, struct obj_data *trap);
bool is_trap_detected(struct obj_data *trap);
void set_trap_detected(struct obj_data *trap);

/* ACMD */
ACMD(do_disabletrap);
ACMD(do_detecttrap);


#endif	/* TRAPS_H */

