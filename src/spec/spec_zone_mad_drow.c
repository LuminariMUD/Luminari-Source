/**************************************************************************
 *  File: spec/spec_zone_mad_drow.c                    Part of LuminariMUD *
 *  Usage: Mad Drow cube-slider procedures.                              *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "movement/door_state.h"
#include "utils.h"
#include "act.h"
#include "comm.h"
#include "interpreter.h"
#include "db.h"
#include "spec_zone_mad_drow.h"

/*****************/
/* Mad Drow */
/*****************/

bool open_msg = FALSE;
bool close_msg = FALSE;

struct slider_row
{
  int room;
  int door;
};

struct slider_row row_1_a_n_s[] = {{155521, EAST}, {155522, WEST}, {155530, EAST}, {155529, WEST},
                                   {155531, EAST}, {155532, WEST}, {155540, EAST}, {155539, WEST},
                                   {155541, EAST}, {155542, WEST}, {-1, -1}};
struct slider_row row_1_b_n_s[] = {{155546, EAST}, {155547, WEST}, {155555, EAST}, {155554, WEST},
                                   {155556, EAST}, {155557, WEST}, {155565, EAST}, {155564, WEST},
                                   {155566, EAST}, {155567, WEST}, {-1, -1}};
struct slider_row row_1_c_n_s[] = {{155571, EAST}, {155572, WEST}, {155580, EAST}, {155579, WEST},
                                   {155581, EAST}, {155582, WEST}, {155590, EAST}, {155589, WEST},
                                   {155591, EAST}, {155592, WEST}, {-1, -1}};
struct slider_row row_1_d_n_s[] = {{155596, EAST}, {155597, WEST}, {155605, EAST}, {155604, WEST},
                                   {155606, EAST}, {155607, WEST}, {155615, EAST}, {155614, WEST},
                                   {155616, EAST}, {155617, WEST}, {-1, -1}};
struct slider_row row_1_e_n_s[] = {{155625, EAST}, {155624, WEST}, {155626, EAST}, {155627, WEST},
                                   {155635, EAST}, {155634, WEST}, {155636, EAST}, {155637, WEST},
                                   {155645, EAST}, {155644, WEST}, {-1, -1}};
struct slider_row row_2_a_n_s[] = {{155522, EAST}, {155523, WEST}, {155529, EAST}, {155528, WEST},
                                   {155532, EAST}, {155533, WEST}, {155539, EAST}, {155538, WEST},
                                   {155542, EAST}, {155543, WEST}, {-1, -1}};
struct slider_row row_2_b_n_s[] = {{155547, EAST}, {155548, WEST}, {155554, EAST}, {155553, WEST},
                                   {155557, EAST}, {155558, WEST}, {155564, EAST}, {155563, WEST},
                                   {155567, EAST}, {155568, WEST}, {-1, -1}};
struct slider_row row_2_c_n_s[] = {{155572, EAST}, {155573, WEST}, {155579, WEST}, {155578, WEST},
                                   {155582, EAST}, {155583, WEST}, {155589, EAST}, {155588, WEST},
                                   {155592, EAST}, {155593, WEST}, {-1, -1}};
struct slider_row row_2_d_n_s[] = {{155597, EAST}, {155598, WEST}, {155604, EAST}, {155603, WEST},
                                   {155607, EAST}, {155608, WEST}, {155614, EAST}, {155613, WEST},
                                   {155617, EAST}, {155618, WEST}, {-1, -1}};
struct slider_row row_2_e_n_s[] = {{155624, EAST}, {155623, WEST}, {155627, EAST}, {155628, WEST},
                                   {155634, EAST}, {155633, WEST}, {155637, EAST}, {155638, WEST},
                                   {155644, EAST}, {155643, WEST}, {-1, -1}};
struct slider_row row_3_a_n_s[] = {{155523, EAST}, {155534, WEST}, {155528, EAST}, {155527, WEST},
                                   {155533, EAST}, {155534, WEST}, {155538, EAST}, {155537, WEST},
                                   {155543, EAST}, {155544, WEST}, {-1, -1}};
struct slider_row row_3_b_n_s[] = {{155548, EAST}, {155549, WEST}, {155553, EAST}, {155552, WEST},
                                   {155559, EAST}, {155559, WEST}, {155563, EAST}, {155562, WEST},
                                   {155568, EAST}, {155569, WEST}, {-1, -1}};
struct slider_row row_3_c_n_s[] = {{155573, EAST}, {155574, WEST}, {155578, EAST}, {155577, WEST},
                                   {155583, EAST}, {155584, WEST}, {155588, EAST}, {155587, WEST},
                                   {155593, EAST}, {155594, WEST}, {-1, -1}};
struct slider_row row_3_d_n_s[] = {{155598, EAST}, {155599, WEST}, {155603, EAST}, {155602, WEST},
                                   {155608, EAST}, {155609, WEST}, {155613, EAST}, {155612, WEST},
                                   {155618, EAST}, {155619, WEST}, {-1, -1}};
struct slider_row row_3_e_n_s[] = {{155623, EAST}, {155622, WEST}, {155628, EAST}, {155629, WEST},
                                   {155633, EAST}, {155632, WEST}, {155638, EAST}, {155639, WEST},
                                   {155643, EAST}, {155642, WEST}, {-1, -1}};
struct slider_row row_4_a_n_s[] = {{155524, EAST}, {155525, WEST}, {155527, EAST}, {155526, WEST},
                                   {155534, EAST}, {155535, WEST}, {155537, EAST}, {155536, WEST},
                                   {155544, EAST}, {155545, WEST}, {-1, -1}};
struct slider_row row_4_b_n_s[] = {{155549, EAST}, {155550, WEST}, {155552, EAST}, {155551, WEST},
                                   {155559, EAST}, {155560, WEST}, {155562, EAST}, {155561, WEST},
                                   {155569, EAST}, {155570, WEST}, {-1, -1}};
struct slider_row row_4_c_n_s[] = {{155574, EAST}, {155575, WEST}, {155577, EAST}, {155576, WEST},
                                   {155584, EAST}, {155585, WEST}, {155587, EAST}, {155586, WEST},
                                   {155594, EAST}, {155595, WEST}, {-1, -1}};
struct slider_row row_4_d_n_s[] = {{155599, EAST}, {155600, WEST}, {155602, EAST}, {155601, WEST},
                                   {155609, EAST}, {155610, WEST}, {155612, EAST}, {155611, WEST},
                                   {155619, EAST}, {155620, WEST}, {-1, -1}};
struct slider_row row_4_e_n_s[] = {{155622, EAST}, {155621, WEST}, {155629, EAST}, {155630, WEST},
                                   {155632, EAST}, {155631, WEST}, {155632, EAST}, {155631, WEST},
                                   {155639, EAST}, {155640, WEST}, {155642, EAST}, {155641, WEST},
                                   {-1, -1}};
struct slider_row row_1_a_e_w[] = {{155521, SOUTH}, {155530, NORTH}, {155522, SOUTH},
                                   {155523, SOUTH}, {155524, SOUTH}, {155525, SOUTH},
                                   {155529, NORTH}, {155528, NORTH}, {155527, NORTH},
                                   {155526, NORTH}, {-1, -1}};
struct slider_row row_1_b_e_w[] = {{155546, SOUTH}, {155547, SOUTH}, {155548, SOUTH},
                                   {155549, SOUTH}, {155550, SOUTH}, {155555, NORTH},
                                   {155554, NORTH}, {155553, NORTH}, {155552, NORTH},
                                   {155551, NORTH}, {-1, -1}};
struct slider_row row_1_c_e_w[] = {{155571, SOUTH}, {155572, SOUTH}, {155573, SOUTH},
                                   {155574, SOUTH}, {155575, SOUTH}, {155580, NORTH},
                                   {155579, NORTH}, {155578, NORTH}, {155577, NORTH},
                                   {155576, NORTH}, {-1, -1}};
struct slider_row row_1_d_e_w[] = {{155596, SOUTH}, {155597, SOUTH}, {155598, SOUTH},
                                   {155599, SOUTH}, {155600, SOUTH}, {155605, NORTH},
                                   {155604, NORTH}, {155603, NORTH}, {155602, NORTH},
                                   {155601, NORTH}, {-1, -1}};
struct slider_row row_1_e_e_w[] = {{155625, SOUTH}, {155624, SOUTH}, {155623, SOUTH},
                                   {155622, SOUTH}, {155621, SOUTH}, {155626, NORTH},
                                   {155627, NORTH}, {155628, NORTH}, {155629, NORTH},
                                   {155630, NORTH}, {-1, -1}};
struct slider_row row_2_a_e_w[] = {
    {155530, SOUTH}, {155531, NORTH}, {155529, SOUTH}, {155527, SOUTH}, {155526, SOUTH},
    {155532, NORTH}, {155533, NORTH}, {155534, NORTH}, {155535, NORTH}, {-1, -1}};
struct slider_row row_2_b_e_w[] = {{155555, SOUTH}, {155554, SOUTH}, {155553, SOUTH},
                                   {155552, SOUTH}, {155551, SOUTH}, {155556, NORTH},
                                   {155557, NORTH}, {155558, NORTH}, {155559, NORTH},
                                   {155560, NORTH}, {-1, -1}};
struct slider_row row_2_c_e_w[] = {{155580, SOUTH}, {155579, SOUTH}, {155578, SOUTH},
                                   {155577, SOUTH}, {155576, SOUTH}, {155581, NORTH},
                                   {155582, NORTH}, {155583, NORTH}, {155584, NORTH},
                                   {155585, NORTH}, {-1, -1}};
struct slider_row row_2_d_e_w[] = {{155605, SOUTH}, {155604, SOUTH}, {155603, SOUTH},
                                   {155602, SOUTH}, {155601, SOUTH}, {155606, NORTH},
                                   {155607, NORTH}, {155608, NORTH}, {155609, NORTH},
                                   {155610, NORTH}, {-1, -1}};
struct slider_row row_2_e_e_w[] = {{155626, SOUTH}, {155627, SOUTH}, {155628, SOUTH},
                                   {155629, SOUTH}, {155630, SOUTH}, {155635, NORTH},
                                   {155634, NORTH}, {155633, NORTH}, {155632, NORTH},
                                   {155631, NORTH}, {-1, -1}};
struct slider_row row_3_a_e_w[] = {{155531, SOUTH}, {155540, NORTH}, {155532, SOUTH},
                                   {155533, SOUTH}, {155534, SOUTH}, {155535, SOUTH},
                                   {155539, NORTH}, {155538, NORTH}, {155537, NORTH},
                                   {155536, NORTH}, {-1, -1}};
struct slider_row row_3_b_e_w[] = {{155556, SOUTH}, {155557, SOUTH}, {155558, SOUTH},
                                   {155559, SOUTH}, {155560, SOUTH}, {155565, NORTH},
                                   {155564, NORTH}, {155563, NORTH}, {155562, NORTH},
                                   {155561, NORTH}, {-1, -1}};
struct slider_row row_3_c_e_w[] = {{155581, SOUTH}, {155582, SOUTH}, {155583, SOUTH},
                                   {155584, SOUTH}, {155585, SOUTH}, {155590, NORTH},
                                   {155589, NORTH}, {155588, NORTH}, {155587, NORTH},
                                   {155586, NORTH}, {-1, -1}};
struct slider_row row_3_d_e_w[] = {{155606, SOUTH}, {155607, SOUTH}, {155608, SOUTH},
                                   {155609, SOUTH}, {155610, SOUTH}, {155615, NORTH},
                                   {155614, NORTH}, {155613, NORTH}, {155612, NORTH},
                                   {155611, NORTH}, {-1, -1}};
struct slider_row row_3_e_e_w[] = {{155635, SOUTH}, {155634, SOUTH}, {155633, SOUTH},
                                   {155632, SOUTH}, {155631, SOUTH}, {155636, NORTH},
                                   {155637, NORTH}, {155638, NORTH}, {155639, NORTH},
                                   {155640, NORTH}, {-1, -1}};
struct slider_row row_4_a_e_w[] = {{155540, SOUTH}, {155541, NORTH}, {155539, SOUTH},
                                   {155538, SOUTH}, {155537, SOUTH}, {155536, SOUTH},
                                   {155542, NORTH}, {155543, NORTH}, {155544, NORTH},
                                   {155545, NORTH}, {-1, -1}};
struct slider_row row_4_b_e_w[] = {{155565, SOUTH}, {155564, SOUTH}, {155563, SOUTH},
                                   {155562, SOUTH}, {155561, SOUTH}, {155566, NORTH},
                                   {155567, NORTH}, {155568, NORTH}, {155569, NORTH},
                                   {155570, NORTH}, {-1, -1}};
struct slider_row row_4_c_e_w[] = {{155590, SOUTH}, {155589, SOUTH}, {155588, SOUTH},
                                   {155587, SOUTH}, {155586, SOUTH}, {155591, NORTH},
                                   {155592, NORTH}, {155593, NORTH}, {155594, NORTH},
                                   {155595, NORTH}, {-1, -1}};
struct slider_row row_4_d_e_w[] = {{155615, SOUTH}, {155614, SOUTH}, {155613, SOUTH},
                                   {155612, SOUTH}, {155611, SOUTH}, {155616, NORTH},
                                   {155617, NORTH}, {155618, NORTH}, {155619, NORTH},
                                   {155620, NORTH}, {-1, -1}};
struct slider_row row_4_e_e_w[] = {{155636, SOUTH}, {155637, SOUTH}, {155638, SOUTH},
                                   {155639, SOUTH}, {155640, SOUTH}, {155645, NORTH},
                                   {155644, NORTH}, {155643, NORTH}, {155642, NORTH},
                                   {155641, NORTH}, {-1, -1}};
struct slider_row row_1_a_u_d[] = {{155521, DOWN}, {155546, UP}, {155530, DOWN}, {155555, UP},
                                   {155531, DOWN}, {155556, UP}, {155540, DOWN}, {155565, UP},
                                   {155541, DOWN}, {155566, UP}, {-1, -1}};
struct slider_row row_1_b_u_d[] = {{155522, DOWN}, {155547, UP}, {155529, DOWN}, {155554, UP},
                                   {155532, DOWN}, {155557, UP}, {155539, DOWN}, {155564, UP},
                                   {155542, DOWN}, {155567, UP}, {-1, -1}};
struct slider_row row_1_c_u_d[] = {{155523, DOWN}, {155528, DOWN}, {155533, DOWN}, {155538, DOWN},
                                   {155543, DOWN}, {155548, UP},   {155553, UP},   {155558, UP},
                                   {155563, UP},   {155568, UP},   {-1, -1}};
struct slider_row row_1_d_u_d[] = {{155524, DOWN}, {155527, DOWN}, {155534, DOWN}, {155537, DOWN},
                                   {155544, DOWN}, {155549, UP},   {155552, UP},   {155559, UP},
                                   {155562, UP},   {155569, UP},   {-1, -1}};
struct slider_row row_1_e_u_d[] = {{155525, DOWN}, {155526, DOWN}, {155535, DOWN}, {155536, DOWN},
                                   {155545, DOWN}, {155550, UP},   {155551, UP},   {155560, UP},
                                   {155561, UP},   {155570, UP},   {-1, -1}};
struct slider_row row_2_a_u_d[] = {{155546, DOWN}, {155571, UP},   {155555, DOWN}, {155556, DOWN},
                                   {155565, DOWN}, {155566, DOWN}, {155580, UP},   {155581, UP},
                                   {155590, UP},   {155591, UP},   {-1, -1}};
struct slider_row row_2_b_u_d[] = {{155547, DOWN}, {155554, DOWN}, {155557, DOWN}, {155564, DOWN},
                                   {155567, DOWN}, {155572, UP},   {155569, UP},   {155582, UP},
                                   {155589, UP},   {155592, UP},   {-1, -1}};
struct slider_row row_2_c_u_d[] = {{155548, DOWN}, {155553, DOWN}, {155558, DOWN}, {155563, DOWN},
                                   {155568, DOWN}, {155573, UP},   {155578, UP},   {155583, UP},
                                   {155588, UP},   {155593, UP},   {-1, -1}};
struct slider_row row_2_d_u_d[] = {{155549, DOWN}, {155552, DOWN}, {155559, DOWN}, {155562, DOWN},
                                   {155569, DOWN}, {155574, UP},   {155577, UP},   {155584, UP},
                                   {155587, UP},   {155594, UP},   {-1, -1}};
struct slider_row row_2_e_u_d[] = {{155550, DOWN}, {155551, DOWN}, {155560, DOWN}, {155561, DOWN},
                                   {155570, DOWN}, {155575, UP},   {155576, UP},   {155585, UP},
                                   {155586, UP},   {155595, UP},   {-1, -1}};
struct slider_row row_3_a_u_d[] = {{155571, DOWN}, {155596, UP},   {155580, DOWN}, {155581, DOWN},
                                   {155590, DOWN}, {155591, DOWN}, {155596, UP},   {155605, UP},
                                   {155606, UP},   {155615, UP},   {155616, UP},   {-1, -1}};
struct slider_row row_3_b_u_d[] = {{155572, DOWN}, {155579, DOWN}, {155582, DOWN}, {155589, DOWN},
                                   {155592, DOWN}, {155597, UP},   {155604, UP},   {155607, UP},
                                   {155614, UP},   {155617, UP},   {-1, -1}};
struct slider_row row_3_c_u_d[] = {{155573, DOWN}, {155578, DOWN}, {155583, DOWN}, {155588, DOWN},
                                   {155593, DOWN}, {155598, UP},   {155603, UP},   {155608, UP},
                                   {155613, UP},   {155618, UP},   {-1, -1}};
struct slider_row row_3_d_u_d[] = {{155574, DOWN}, {155577, DOWN}, {155584, DOWN}, {155587, DOWN},
                                   {155594, DOWN}, {155599, UP},   {155602, UP},   {155609, UP},
                                   {155612, UP},   {155619, UP},   {-1, -1}};
struct slider_row row_3_e_u_d[] = {{155575, DOWN}, {155576, DOWN}, {155585, DOWN}, {155586, DOWN},
                                   {155595, DOWN}, {155600, UP},   {155601, UP},   {155610, UP},
                                   {155611, UP},   {155620, UP},   {-1, -1}};
struct slider_row row_4_a_u_d[] = {{155596, DOWN}, {155625, UP},   {155605, DOWN}, {155606, DOWN},
                                   {155615, DOWN}, {155616, DOWN}, {155626, UP},   {155635, UP},
                                   {155636, UP},   {155645, UP},   {-1, -1}};
struct slider_row row_4_b_u_d[] = {{155597, DOWN}, {155604, DOWN}, {155607, DOWN}, {155614, DOWN},
                                   {155617, DOWN}, {155624, UP},   {155627, UP},   {155634, UP},
                                   {155637, UP},   {155644, UP},   {-1, -1}};
struct slider_row row_4_c_u_d[] = {{155598, DOWN}, {155603, DOWN}, {155608, DOWN}, {155613, DOWN},
                                   {155618, DOWN}, {155623, UP},   {155628, UP},   {155633, UP},
                                   {155638, UP},   {155643, UP},   {-1, -1}};
struct slider_row row_4_d_u_d[] = {{155599, DOWN}, {155602, DOWN}, {155609, DOWN}, {155612, DOWN},
                                   {155619, DOWN}, {155622, UP},   {155629, UP},   {155632, UP},
                                   {155639, UP},   {155642, UP},   {-1, -1}};
struct slider_row row_4_e_u_d[] = {{155600, DOWN}, {155601, DOWN}, {155610, DOWN}, {155611, DOWN},
                                   {155620, DOWN}, {155621, UP},   {155630, UP},   {155631, UP},
                                   {155640, UP},   {155641, UP},   {-1, -1}};

void open_exit(struct slider_row row)
{
  door_state_update(row.room, row.door, EX_CLOSED | EX_LOCKED, EX_PICKPROOF, false,
                    DOMAIN_DOOR_GAMEPLAY);
}

void close_exit(struct slider_row row)
{
  door_state_update(row.room, row.door, 0, EX_CLOSED | EX_LOCKED | EX_PICKPROOF, false,
                    DOMAIN_DOOR_GAMEPLAY);
}

static void send_to_cube(const char *echo)
{
  struct descriptor_data *d;

  for (d = descriptor_list; d; d = d->next)
  {
    if (!d->character)
      continue;
    if (GET_ROOM_VNUM(d->character->in_room) < 155521)
      continue;
    if (GET_ROOM_VNUM(d->character->in_room) > 155641)
      continue;

    if (!AWAKE(d->character))
      continue;
    send_to_char(d->character, "%s", echo);
  }
}

void open_row(struct slider_row *row)
{
  open_msg = TRUE;
  while (row->room != -1)
    open_exit(*row++);
}

void close_row(struct slider_row *row)
{
  close_msg = TRUE;
  while (row->room != -1)
    close_exit(*row++);
}

void toggle_row(struct slider_row *row)
{
  if (IS_CLOSED(row->room, row->door))
    open_row(row);
  else
    close_row(row);
}

// How long average probability between polls..
#define TOG_DELAY 20
// Probability for a wall to toggle..  close to 1000 less likely.
#define TOG_FREQ 975

SPECIAL(cube_slider)
{
  if (cmd)
    return 0;

  if (!rand_number(0, TOG_DELAY))
    return 0;

  open_msg = FALSE;
  close_msg = FALSE;

  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_a_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_b_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_c_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_d_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_e_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_a_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_b_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_c_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_d_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_e_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_a_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_b_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_c_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_d_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_e_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_a_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_b_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_c_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_d_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_e_n_s);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_a_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_b_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_c_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_d_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_e_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_a_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_b_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_c_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_d_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_e_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_a_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_b_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_c_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_d_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_e_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_a_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_b_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_c_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_d_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_e_e_w);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_a_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_b_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_c_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_d_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_1_e_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_a_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_b_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_c_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_d_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_2_e_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_a_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_b_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_c_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_d_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_3_e_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_a_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_b_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_c_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_d_u_d);
  if (dice(1, 1000) > TOG_FREQ)
    toggle_row(row_4_e_u_d);

  if (open_msg)
    send_to_cube("\tD\tLThe whole area rumbles loudly as a dividing wall slams shut.\tn\r\n");
  if (close_msg)
    send_to_cube("\tDEverything begins to shake and rumble as a dividing wall opens.\tn\r\n");

  return 1;
}

/*****************/
/* End Mad Drow */
/*****************/
