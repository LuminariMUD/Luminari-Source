/* *************************************************************************
 *   File: resource_regeneration.h                     Part of LuminariMUD *
 *  Usage: Resource regeneration logging system header                    *
 * Author: Phase 6 Implementation - Step 4: Regeneration Logging         *
 ***************************************************************************
 * Header for regeneration event logging and monitoring functions         *
 ***************************************************************************/

#ifndef RESOURCE_REGENERATION_H
#define RESOURCE_REGENERATION_H

/* ===== REGENERATION SYSTEM FUNCTIONS ===== */

/* System control */
void set_regeneration_logging_enabled(bool enabled);
bool is_regeneration_logging_enabled(void);

/* Regeneration logging */
void log_regeneration_event(int zone_vnum, int x, int y, int resource_type, 
                           float old_level, float new_level, float regen_amount, 
                           const char *regen_type);
void show_regeneration_history(struct char_data *ch, int zone_vnum, int x, int y, int limit);

#endif /* RESOURCE_REGENERATION_H */
