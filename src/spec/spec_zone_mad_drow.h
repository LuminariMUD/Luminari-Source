/**
 * @file spec/spec_zone_mad_drow.h
 * Public API for the Mad Drow cube-slider procedures.
 *
 * All rights reserved. See license for complete information.
 */

#ifndef LUMINARI_SPEC_ZONE_MAD_DROW_H
#define LUMINARI_SPEC_ZONE_MAD_DROW_H

#include <stdbool.h>

struct char_data;
struct slider_row;

extern bool open_msg;
extern bool close_msg;

void open_exit(struct slider_row row);
void close_exit(struct slider_row row);
void open_row(struct slider_row *row);
void close_row(struct slider_row *row);
void toggle_row(struct slider_row *row);

int cube_slider(struct char_data *ch, void *me, int cmd, const char *argument);

#endif /* LUMINARI_SPEC_ZONE_MAD_DROW_H */
