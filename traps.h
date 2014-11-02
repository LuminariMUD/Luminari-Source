/* 
 * File:   traps.h
 * Author: Zusuk
 *
 * Created on 2 נובמבר 2014, 22:39
 */

#ifndef TRAPS_H
#define TRAPS_H

struct trap_event {
  struct char_data *ch;
  int effect;
};

/* fuctions defined in traps.c */
bool check_trap(struct char_data *ch, int trap_type, int room, struct obj_data *obj, int dir);

#endif	/* TRAPS_H */

