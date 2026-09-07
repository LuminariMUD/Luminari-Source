/**************************************************************************
 *  File: vessels/vessels_moving_rooms.c                Part of LuminariMUD *
 *  Usage: Legacy moving-room loading, scheduling, and relocation.          *
 *                                                                         *
 *  All rights reserved.  See license for complete information.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "movement/door_state.h"
#include "comm.h"
#include "db.h"
#include "spec/spec_dispatch.h"
#include "vessels_moving_rooms.h"

/*  read moving room data  */
void setup_moving_room(FILE *fl, int rroom, int vroom, char *line)
{
  int roomInfo[5];
  int connInfo[3];
  char buf[MAX_STRING_LENGTH];

  int connCnt = 0, j, connLine = 0;
  room_num fR[MAX_MOVING_ROOMS];
  int fD[MAX_MOVING_ROOMS];

  char errStr[100];
  char lineIn[256];
  char msg1[200], msg2[200], msg3[200];

  struct moving_room_data *newRoom;

  if (world[rroom].mover)
  {
    log("SYSERR: setup_moving_room - this room already has a mover assigned");
    return;
  }

  strcpy(errStr, "");
  strcpy(lineIn, "");
  strcpy(msg1, "");
  strcpy(msg2, "");
  strcpy(msg3, "");

  for (j = 0; j < MAX_MOVING_ROOMS; j++)
  {
    fR[j] = NOWHERE;
    fD[j] = -1;
  }

  if (sscanf(line, " %d %d %d %d %d ", roomInfo, roomInfo + 1, roomInfo + 2, roomInfo + 3,
             roomInfo + 4) != 5)
  {
    fprintf(stderr, "Format error, room #%d, M line\n", world[rroom].number);
    exit(1);
  }

  /*  now get the 3 room messages (if they exist)  */
  if (!get_line(fl, lineIn))
  {
    fprintf(stderr, " missing transit message when processing M...\n");
    exit(1);
  }
  if (lineIn[0] != '~')
  {
    strlcpy(msg1, lineIn, sizeof(msg1));
  }

  if (!get_line(fl, lineIn))
  {
    fprintf(stderr, " missing docking message when processing M...\n");
    exit(1);
  }
  if (lineIn[0] != '~')
  {
    strlcpy(msg2, lineIn, sizeof(msg2));
  }

  if (!get_line(fl, lineIn))
  {
    fprintf(stderr, " missing dest dock message when processing M...\n");
    exit(1);
  }
  if (lineIn[0] != '~')
  {
    strlcpy(msg3, lineIn, sizeof(msg3));
  }

  if (!get_line(fl, lineIn))
  {
    fprintf(stderr, " - get_line() returned 0 in world 'M' processing...\n");
    fprintf(stderr, "%s\n", lineIn);
    exit(1);
  }

  while (lineIn[0] != '~')
  {
    if (sscanf(lineIn, " %d %d %d ", connInfo, connInfo + 1, connInfo + 2) != 3)
    {
      fprintf(stderr, "Format error, room #%d, %d after M line\n", (connLine + 1),
              world[rroom].number);
      exit(1);
    }

    connLine++;

    if (connInfo[2] < 1)
    {
      connInfo[2] = 1;
    }
    else if (connInfo[2] > 50)
    {
      connInfo[2] = 50;
    }

    for (j = 0; j < connInfo[2]; j++)
    {
      connCnt++;

      if (connCnt < MAX_MOVING_ROOMS)
      {
        /*  store the conn room info  */
        fR[connCnt - 1] = (room_num)connInfo[0];
        fD[connCnt - 1] = connInfo[1];
      }
      else
      {
        log("setup_moving_room(): # of conneting rooms exceeded limit...");
      }
    }

    if (!get_line(fl, lineIn))
    {
      fprintf(stderr, " - get_line() returned 0 in world 'M' processing...\n");
      fprintf(stderr, "%s\n", buf);
      exit(1);
    }
  }

/*
 *  PDH 11/17/97
 *  lots of work for each new "moving room"
 *  must set up struct moving_room_data and add to
 *  the live room
 */
#ifdef DEBUGMEM
  CREATE(newRoom, struct moving_room_data, 1, M1);
#else
  CREATE(newRoom, struct moving_room_data, 1);
#endif

  newRoom->resetZonePulse = roomInfo[1];
  newRoom->currentInbound = -1;
  newRoom->destination = vroom;
  newRoom->inbound_dir = roomInfo[0];
  newRoom->randomMove = roomInfo[2];
  newRoom->exitInfo = roomInfo[3];
  newRoom->keyInfo = roomInfo[4];
#ifdef DEBUGMEM
  newRoom->keywords = str_dup("door", S19);
#else
  newRoom->keywords = strdup("door");
#endif

  if (strlen(msg1) > 5)
  {
#ifdef DEBUGMEM
    newRoom->msg_transit = str_dup(msg1, T19);
#else
    newRoom->msg_transit = strdup(msg1);
#endif
  }
  else
  {
    newRoom->msg_transit = NULL;
  }
  if (strlen(msg2) > 5)
  {
#ifdef DEBUGMEM
    newRoom->msg_docking = str_dup(msg2, U19);
#else
    newRoom->msg_docking = strdup(msg2);
#endif
  }
  else
  {
    newRoom->msg_docking = NULL;
  }
  if (strlen(msg3) > 5)
  {
#ifdef DEBUGMEM
    newRoom->msg_dest_docking = str_dup(msg3, V19);
#else
    newRoom->msg_dest_docking = strdup(msg3);
#endif
  }
  else
  {
    newRoom->msg_dest_docking = NULL;
  }
#ifdef DEBUGMEM
  CREATE(newRoom->from, room_num, connCnt + 1, N1);
  CREATE(newRoom->fromDir, int, connCnt + 1, O1);
#else
  CREATE(newRoom->from, room_num, connCnt + 1);
  CREATE(newRoom->fromDir, int, connCnt + 1);
#endif

  for (j = 0; j < connCnt; j++)
  {
    newRoom->from[j] = fR[j];
    newRoom->fromDir[j] = fD[j];
  }
  newRoom->from[j] = ENDMOVING;
  newRoom->fromDir[j] = -1;

  world[rroom].mover = newRoom;

  return;
}


int unlinkMovingRoom(struct moving_room_data *theRoom, struct oldNextMove *ONMdata, int cibIdx)
{
  struct door_state_operation operations[2] = {0};
  char errStr[128];

  if ((ONMdata->oldRoom != NOWHERE) && (ONMdata->oldRoom != ENDMOVING))
  {
    /*  unlink old room - both sides  */

    /*  check if rooms sync up  */
    if (theRoom->from[cibIdx] != ONMdata->oldRoom)
    {
      snprintf(errStr, sizeof(errStr),
               "SPEC(move_room): [%d] from[cibIdx] != oldRoom (or <= 0) (%d/%d %d)",
               (int)ONMdata->moveRoom, theRoom->from[cibIdx], ONMdata->oldRoom, cibIdx);
      log("%s", errStr);
      return 0;
    }

    /*  check if dest room dir is clean...  */
    if (world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir] == NULL)
    {
      log("SPEC(move_room): moving room has not set a connecting room...");
      return 0;
    }

    /*  check if old conn room dir is clean...  */
    if (world[real_room(ONMdata->oldRoom)].dir_option[ONMdata->oldDir] == NULL)
    {
      sprintf(errStr, "SPEC(move_room): [%d] old conn room %d dir %d not set...",
              (int)ONMdata->moveRoom, ONMdata->oldRoom, ONMdata->oldDir);
      log("%s", errStr);
      return 0;
    }

    door_state_begin(&operations[0], real_room(ONMdata->moveRoom), theRoom->inbound_dir, false,
                     DOMAIN_DOOR_EDIT);
    door_state_begin(&operations[1], real_room(ONMdata->oldRoom), ONMdata->oldDir, false,
                     DOMAIN_DOOR_EDIT);

    /* log("SPEC(move): all is Ok to unlink the old room..."); */

#ifdef DEBUGMEM
    freeusg(world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir], P9);
#else
    free(world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir]);
#endif

    world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir] = NULL;

#ifdef DEBUGMEM
    freeusg(world[real_room(ONMdata->oldRoom)].dir_option[ONMdata->oldDir], O9);
#else
    free(world[real_room(ONMdata->oldRoom)].dir_option[ONMdata->oldDir]);
#endif
    world[real_room(ONMdata->oldRoom)].dir_option[ONMdata->oldDir] = NULL;

    /* log("SPEC(move): all is Ok after unlinking the old room..."); */
    sprintf(errStr, "SPEC(moving_rooms): [%d] unlinked %d  FROM  %d", (int)ONMdata->moveRoom,
            ONMdata->oldRoom, theRoom->destination);
    /* mudlog(errStr, CMP, LVL_QUEST, TRUE); */
  }

  door_state_finish(&operations[0]);
  door_state_finish(&operations[1]);
  return 1;
}

int linkMovingRoom(struct moving_room_data *theRoom, struct oldNextMove *ONMdata, int cibIdx)
{
  struct door_state_operation operation = {0};
  struct room_direction_data *rdd;
  char errStr[100];

  if (ONMdata->nextRoom != NOWHERE)
  {
    /*  link up new room - both sides  */

    /*  check if dest room dir is clean...  */
    if (world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir] != NULL)
    {
      log("SPEC(move_room): moving room hasn't unset last connecting room...");

      rdd = world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir];
      sprintf(errStr, "SPEC(move): [%d] rdd - desc:%s:  key:%s:  ei:%d:  key:%d:  to:%d:",
              (int)ONMdata->moveRoom,
              (rdd->general_description == NULL) ? "" : rdd->general_description,
              (rdd->keyword == NULL) ? "" : rdd->keyword, rdd->exit_info, rdd->key, rdd->to_room);
      log("%s", errStr);

      door_state_finish(&operation);
      return 0;
    }

    door_state_begin(&operation, real_room(ONMdata->nextRoom), ONMdata->nextDir, false,
                     DOMAIN_DOOR_EDIT);

    /*  check if conn room dir is clean...  */
    if (world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir] != NULL)
    {
      sprintf(errStr, "SPEC(move_room): [%d] conn room has dir %d set...", (int)ONMdata->moveRoom,
              ONMdata->nextDir);
      log("%s", errStr);

      rdd = world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir];
      sprintf(errStr, "SPEC(move): [%d] rdd - desc:%s:  key:%s:  ei:%d:  key:%d:  to:%d:",
              (int)ONMdata->moveRoom,
              (rdd->general_description == NULL) ? "" : rdd->general_description,
              (rdd->keyword == NULL) ? "" : rdd->keyword, rdd->exit_info, rdd->key, rdd->to_room);
      log("%s", errStr);

/*  pdh 5/3/01 - don't return - instead remove the offending exit
return 0;
*/
#ifdef DEBUGMEM
      freeusg(world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir], N9);
#else
      free(world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir]);
#endif
      world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir] = NULL;
    }

    if (theRoom->from[cibIdx] != ENDMOVING)
    {
      log("SPEC(move_room): theRoom->from[cibIdx] != ENDMOVING");
      door_state_finish(&operation);
      return 0;
    }

    /* log("SPEC(move): all is Ok to link the new room..."); */

#ifdef DEBUGMEM
    CREATE(world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir],
           struct room_direction_data, 1, M9);
#else
    CREATE(world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir],
           struct room_direction_data, 1);
#endif
    world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir]->general_description = NULL;
    world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir]->keyword = NULL;
    world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir]->exit_info = 0;
    world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir]->key = -1;
    world[real_room(ONMdata->nextRoom)].dir_option[ONMdata->nextDir]->to_room =
        real_room(theRoom->destination);

    /*  play 'docking' message  */
    if (theRoom->msg_docking)
    {
      struct char_data *cc;

      for (cc = world[real_room(ONMdata->moveRoom)].people; cc; cc = cc->next_in_room)
      {
        if (cc->desc)
        {
          send_to_char(cc, "%s\r\n", theRoom->msg_docking);
        }
      }
    }

#ifdef DEBUGMEM
    CREATE(world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir],
           struct room_direction_data, 1, L9);
#else
    CREATE(world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir],
           struct room_direction_data, 1);
#endif
    world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir]->general_description =
        NULL;
    world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir]->keyword = NULL;
    world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir]->exit_info = 0;
    world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir]->key = -1;
    world[real_room(ONMdata->moveRoom)].dir_option[theRoom->inbound_dir]->to_room =
        real_room(ONMdata->nextRoom);

    /*  play 'dest_docking' message  */
    if (theRoom->msg_dest_docking)
    {
      struct char_data *cc;

      for (cc = world[real_room(ONMdata->nextRoom)].people; cc; cc = cc->next_in_room)
      {
        if (cc->desc)
        {
          send_to_char(cc, "%s\r\n", theRoom->msg_dest_docking);
        }
      }
    }

    /* log("SPEC(move): all complete: linked the new room"); */
    sprintf(errStr, "SPEC(moving_rooms): [%d] unlinked %d and linked %d  TO  %d",
            (int)ONMdata->moveRoom, ONMdata->oldRoom, ONMdata->nextRoom, theRoom->destination);
    /* mudlog(errStr, CMP, LVL_QUEST, TRUE); */
  }
  else
  {
    /*  play 'transit' message  */
    if (theRoom->msg_transit)
    {
      struct char_data *cc;

      for (cc = world[real_room(ONMdata->moveRoom)].people; cc; cc = cc->next_in_room)
      {
        if (cc->desc)
        {
          send_to_char(cc, "%s\r\n", theRoom->msg_transit);
        }
      }
    }
  }

  door_state_finish(&operation);
  return 1;
}

int prepMovingRoom(struct moving_room_data *theRoom, struct oldNextMove *ONMdata, int *cibIdx,
                   int *nextIdx)
{
  int fromRoomCnt = 0;

  /*  get count of rooms in array  */
  for (fromRoomCnt = 0; theRoom->from[fromRoomCnt] != ENDMOVING; fromRoomCnt++)
  {
    ;
  }

  if (fromRoomCnt <= 1)
  {
    log("SPECIAL(moving_rooms): 1 or smaller length room array");
    return 0;
  }

  if ((theRoom->currentInbound < 0) || (theRoom->currentInbound >= fromRoomCnt))
  {
    *cibIdx = fromRoomCnt;
  }
  else
  {
    *cibIdx = theRoom->currentInbound;
  }

  ONMdata->oldRoom = theRoom->from[*cibIdx];
  ONMdata->oldDir = theRoom->fromDir[*cibIdx];
  ONMdata->moveRoom = theRoom->destination;

  if (theRoom->randomMove)
  {
    /*  randomly select  */
    *nextIdx = dice(1, fromRoomCnt) - 1;
  }
  else
  {
    /*  next in line  */
    *nextIdx = *cibIdx + 1;

    if ((*nextIdx <= 0) || (*nextIdx >= fromRoomCnt) || (theRoom->from[*nextIdx] == ENDMOVING))
    {
      /*  start over  */
      *nextIdx = 0;
    }
  }

  ONMdata->nextRoom = theRoom->from[*nextIdx];
  ONMdata->nextDir = theRoom->fromDir[*nextIdx];

  /*
      sprintf(errStr, "SPECIAL(moving_rooms): [%d] old: %d (%d) next: %d (%d)",
      (int)moveRoom, oldRoom, oldDir, nextRoom, nextDir);
      log(errStr);
      */

  /*  PDH  5/1/98 - OLD
     *  if ( (ONMdata->nextRoom == NOWHERE) || (ONMdata->nextDir == -1) ) {
     *
     *  now, the next room info CAN be to nowhere
     *  (ie. disconnect the room)
     */
  if ((ONMdata->nextRoom == ENDMOVING) ||
      ((ONMdata->nextRoom != NOWHERE) && (ONMdata->nextDir == -1)))
  {
    /*  it's a bust  */
    log("SPEC(move): nextRoom or nextDir was bogus!");
    return 0;
  }

  return fromRoomCnt;
}

SPECIAL(moving_rooms)
{
  int fromRoomCnt = 0;
  struct oldNextMove ONM;
  int cibIdx = -1, nextIdx = -1;

  struct moving_room_data *theRoom;

  ONM.nextDir = -1;
  ONM.oldDir = -1;
  ONM.nextRoom = NOWHERE;
  ONM.oldRoom = NOWHERE;
  ONM.moveRoom = NOWHERE;

  if ((ch == NULL) && (me != NULL) && (cmd == 0) && (argument == NULL))
  {
    /* log("SPECIAL(moving_rooms)"); */
  }
  else
  {
    return 0;
  }

  /*  PDH 11/17/97  ugly cast... but needed  */
  theRoom = (struct moving_room_data *)me;

  if ((fromRoomCnt = prepMovingRoom(theRoom, &ONM, &cibIdx, &nextIdx)) < 1)
  {
    return 0;
  }

  /*  everything is set now, so make the changes  */

  if (!unlinkMovingRoom(theRoom, &ONM, cibIdx))
  {
    return 0;
  }

  cibIdx = fromRoomCnt; /* NOWHERE */
  theRoom->currentInbound = cibIdx;

  if (!linkMovingRoom(theRoom, &ONM, cibIdx))
  {
    return 0;
  }

  /*  set state of struct moving_room_data  */
  cibIdx = nextIdx;
  theRoom->currentInbound = cibIdx;

  return 1;
}
