/**************************************************************************
 *  File: genmob.c                                     Part of LuminariMUD *
 *  Usage: Generic OLC Library - Mobiles.                                  *
 *                                                                         *
 *  Copyright 1996 by Harvey Gilpin, 1997-2001 by George Greer.            *
 **************************************************************************/

#include "conf.h"
#include "sysdep.h"
#include "structs.h"
#include "utils.h"
#include "db.h"
#include "shop.h"
#include "handler.h"
#include "genolc.h"
#include "genmob.h"
#include "genzon.h"
#include "dg_olc.h"
#include "spells.h"
#include "actionqueues.h"

/* local functions */
static void extract_mobile_all(mob_vnum vnum);

int add_mobile(struct char_data *mob, mob_vnum vnum)
{
  int rnum, i, found = FALSE, shop, cmd_no;
  zone_rnum zone;
  struct char_data *live_mob;

  if ((rnum = real_mobile(vnum)) != NOBODY)
  {
    /* Copy over the mobile and free() the old strings. */
    copy_mobile(&mob_proto[rnum], mob);

    /* Now re-point all existing mobile strings to here. */
    for (live_mob = character_list; live_mob; live_mob = live_mob->next)
      if (rnum == live_mob->nr)
        update_mobile_strings(live_mob, &mob_proto[rnum]);

    add_to_save_list(zone_table[real_zone_by_thing(vnum)].number, SL_MOB);
    log("GenOLC: add_mobile: Updated existing mobile #%d.", vnum);
    return rnum;
  }

  RECREATE(mob_proto, struct char_data, top_of_mobt + 2);
  RECREATE(mob_index, struct index_data, top_of_mobt + 2);
  top_of_mobt++;

  for (i = top_of_mobt; i > 0; i--)
  {
    if (vnum > mob_index[i - 1].vnum)
    {
      mob_proto[i] = *mob;
      mob_proto[i].nr = i;
      copy_mobile_strings(mob_proto + i, mob);
      mob_index[i].vnum = vnum;
      mob_index[i].number = 0;
      mob_index[i].func = 0;
      found = i;
      break;
    }
    mob_index[i] = mob_index[i - 1];
    mob_proto[i] = mob_proto[i - 1];
    mob_proto[i].nr++;
  }
  if (!found)
  {
    mob_proto[0] = *mob;
    mob_proto[0].nr = 0;
    copy_mobile_strings(&mob_proto[0], mob);
    mob_index[0].vnum = vnum;
    mob_index[0].number = 0;
    mob_index[0].func = 0;
  }

  log("GenOLC: add_mobile: Added mobile %d at index #%d.", vnum, found);

  /* Update live mobile rnums. */
  for (live_mob = character_list; live_mob; live_mob = live_mob->next)
    GET_MOB_RNUM(live_mob) += (GET_MOB_RNUM(live_mob) != NOTHING && GET_MOB_RNUM(live_mob) >= found);

  /* Update zone table. */
  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++)
      if (ZCMD(zone, cmd_no).command == 'M')
        ZCMD(zone, cmd_no).arg1 += (ZCMD(zone, cmd_no).arg1 >= found);

  /* Update shop keepers. */
  if (shop_index)
    for (shop = 0; shop <= top_shop; shop++)
      SHOP_KEEPER(shop) += (SHOP_KEEPER(shop) != NOTHING && SHOP_KEEPER(shop) >= found);

  add_to_save_list(zone_table[real_zone_by_thing(vnum)].number, SL_MOB);
  return found;
}

int copy_mobile(struct char_data *to, struct char_data *from)
{
  free_mobile_strings(to);
  *to = *from;
  check_mobile_strings(from);
  copy_mobile_strings(to, from);
  return TRUE;
}

static void extract_mobile_all(mob_vnum vnum)
{
  struct char_data *next, *ch;
  int i;

  for (ch = character_list; ch; ch = next)
  {
    next = ch->next;
    if (GET_MOB_VNUM(ch) == vnum)
    {
      if ((i = GET_MOB_RNUM(ch)) != NOBODY)
      {
        if (ch->player.name && ch->player.name != mob_proto[i].player.name)
          free(ch->player.name);
        ch->player.name = NULL;

        if (ch->player.title && ch->player.title != mob_proto[i].player.title)
          free(ch->player.title);
        ch->player.title = NULL;

        if (ch->player.short_descr && ch->player.short_descr != mob_proto[i].player.short_descr)
          free(ch->player.short_descr);
        ch->player.short_descr = NULL;

        if (ch->player.long_descr && ch->player.long_descr != mob_proto[i].player.long_descr)
          free(ch->player.long_descr);
        ch->player.long_descr = NULL;

        if (ch->player.description && ch->player.description != mob_proto[i].player.description)
          free(ch->player.description);
        ch->player.description = NULL;

        if (ch->player.walkin && ch->player.walkin != mob_proto[i].player.walkin)
          free(ch->player.walkin);
        ch->player.walkin = NULL;

        if (ch->player.walkout && ch->player.walkout != mob_proto[i].player.walkout)
          free(ch->player.walkout);
        ch->player.walkout = NULL;

        /* free script proto list if it's not the prototype */
        if (ch->proto_script && ch->proto_script != mob_proto[i].proto_script)
          free_proto_script(ch, MOB_TRIGGER);
        ch->proto_script = NULL;
      }
      extract_char(ch);
    }
  }
}

int delete_mobile(mob_rnum refpt)
{
  struct char_data *live_mob;
  struct char_data *proto;
  int counter, cmd_no;
  mob_vnum vnum;
  zone_rnum zone;

#if CIRCLE_UNSIGNED_INDEX
  if (refpt == NOBODY || refpt > top_of_mobt)
  {
#else
  if (refpt < 0 || refpt > top_of_mobt)
  {
#endif
    log("SYSERR: GenOLC: delete_mobile: Invalid rnum %d.", refpt);
    return NOBODY;
  }

  vnum = mob_index[refpt].vnum;
  proto = &mob_proto[refpt];

  extract_mobile_all(vnum);
  extract_char(proto);

  for (counter = refpt; counter < top_of_mobt; counter++)
  {
    mob_index[counter] = mob_index[counter + 1];
    mob_proto[counter] = mob_proto[counter + 1];
    mob_proto[counter].nr--;
  }

  top_of_mobt--;
  RECREATE(mob_index, struct index_data, top_of_mobt + 1);
  RECREATE(mob_proto, struct char_data, top_of_mobt + 1);

  /* Update live mobile rnums. */
  for (live_mob = character_list; live_mob; live_mob = live_mob->next)
    GET_MOB_RNUM(live_mob) -= (GET_MOB_RNUM(live_mob) >= refpt);

  /* Update zone table. */
  for (zone = 0; zone <= top_of_zone_table; zone++)
    for (cmd_no = 0; ZCMD(zone, cmd_no).command != 'S'; cmd_no++)
      if (ZCMD(zone, cmd_no).command == 'M')
      {
        if (ZCMD(zone, cmd_no).arg1 == refpt)
        {
          delete_zone_command(&zone_table[zone], cmd_no);
        }
        else
          ZCMD(zone, cmd_no).arg1 -= (ZCMD(zone, cmd_no).arg1 > refpt);
      }

  /* Update shop keepers. */
  if (shop_index)
    for (counter = 0; counter <= top_shop; counter++)
      SHOP_KEEPER(counter) -= (SHOP_KEEPER(counter) >= refpt);

  save_mobiles(real_zone_by_thing(vnum));

  return refpt;
}

int copy_mobile_strings(struct char_data *t, struct char_data *f)
{
  // int i = 0;

  if (f->player.name)
    t->player.name = strdup(f->player.name);
  if (f->player.title)
    t->player.title = strdup(f->player.title);
  if (f->player.short_descr)
    t->player.short_descr = strdup(f->player.short_descr);
  if (f->player.long_descr)
    t->player.long_descr = strdup(f->player.long_descr);
  if (f->player.description)
    t->player.description = strdup(f->player.description);
  if (f->player.walkin)
    t->player.walkin = strdup(f->player.walkin);
  if (f->player.walkout)
    t->player.walkout = strdup(f->player.walkout);
  /*if (ECHO_COUNT(f) > 0) {
    if (ECHO_ENTRIES(t) == NULL)
      CREATE(ECHO_ENTRIES(t), char *, 1);

    for (i = 0; i < ECHO_COUNT(f); i++)
      if (ECHO_ENTRIES(f)[i])
        ECHO_ENTRIES(t)[i] = strdup(ECHO_ENTRIES(f)[i]);
    ECHO_COUNT(t) = ECHO_COUNT(f);
  }*/
  return TRUE;
}

int update_mobile_strings(struct char_data *t, struct char_data *f)
{
  // int i = 0;

  if (f->player.name)
    t->player.name = f->player.name;
  if (f->player.title)
    t->player.title = f->player.title;
  if (f->player.short_descr)
    t->player.short_descr = f->player.short_descr;
  if (f->player.long_descr)
    t->player.long_descr = f->player.long_descr;
  if (f->player.description)
    t->player.description = f->player.description;
  if (f->player.walkin)
    t->player.walkin = f->player.walkin;
  if (f->player.walkout)
    t->player.walkout = f->player.walkout;
  /*if (ECHO_COUNT(f) > 0) {
    if (ECHO_ENTRIES(t) == NULL)
      CREATE(ECHO_ENTRIES(t), char *, 1);
    for (i = 0; i < ECHO_COUNT(f); i++)
      if (ECHO_ENTRIES(f)[i])
        ECHO_ENTRIES(t)[i] = strdup(ECHO_ENTRIES(f)[i]);
    ECHO_COUNT(t) = ECHO_COUNT(f);
  }*/
  return TRUE;
}

int free_mobile_strings(struct char_data *mob)
{
  if (mob->player.name)
    free(mob->player.name);
  if (mob->player.title)
    free(mob->player.title);
  if (mob->player.short_descr)
    free(mob->player.short_descr);
  if (mob->player.long_descr)
    free(mob->player.long_descr);
  if (mob->player.description)
    free(mob->player.description);
  if (mob->player.walkin)
    free(mob->player.walkin);
  if (mob->player.walkout)
    free(mob->player.walkout);
  return TRUE;
}

/* Free a mobile structure that has been edited. Take care of existing mobiles
 * and their mob_proto! */
int free_mobile(struct char_data *mob)
{
  mob_rnum i;
  int j = 0;

  if (mob == NULL)
    return FALSE;

  /* Non-prototyped mobile.  Also known as new mobiles. */
  if ((i = GET_MOB_RNUM(mob)) == NOBODY)
  {
    free_mobile_strings(mob);
    /* free script proto list */
    free_proto_script(mob, MOB_TRIGGER);
  }
  else
  { /* Prototyped mobile. */
    if (mob->player.name && mob->player.name != mob_proto[i].player.name)
      free(mob->player.name);
    if (mob->player.title && mob->player.title != mob_proto[i].player.title)
      free(mob->player.title);
    if (mob->player.short_descr && mob->player.short_descr != mob_proto[i].player.short_descr)
      free(mob->player.short_descr);
    if (mob->player.long_descr && mob->player.long_descr != mob_proto[i].player.long_descr)
      free(mob->player.long_descr);
    if (mob->player.description && mob->player.description != mob_proto[i].player.description)
      free(mob->player.description);
    if (mob->player.walkin && mob->player.walkin != mob_proto[i].player.walkin)
      free(mob->player.walkin);
    if (mob->player.walkout && mob->player.walkout != mob_proto[i].player.walkout)
      free(mob->player.walkout);
    /* free script proto list if it's not the prototype */
    if (mob->proto_script && mob->proto_script != mob_proto[i].proto_script)
      free_proto_script(mob, MOB_TRIGGER);
    if (mob->mob_specials.echo_entries && mob->mob_specials.echo_entries != mob_proto[i].mob_specials.echo_entries)
    {
      for (j = 0; j < mob->mob_specials.echo_count; j++)
        free(mob->mob_specials.echo_entries[j]);
      free(mob->mob_specials.echo_entries);
    }
  }
  while (mob->affected)
    affect_remove(mob, mob->affected);

  /* free any assigned scripts */
  if (SCRIPT(mob))
    extract_script(mob, MOB_TRIGGER);

  /* Free the action queues */
  if (GET_QUEUE(mob))
    free_action_queue(GET_QUEUE(mob));
  if (GET_ATTACK_QUEUE(mob))
    free_attack_queue(GET_ATTACK_QUEUE(mob));

  free(mob);
  return TRUE;
}

int save_mobiles(zone_rnum rznum)
{
  zone_vnum vznum;
  FILE *mobfd;
  room_vnum i;
  mob_rnum rmob;
  int written;
  char mobfname[64], usedfname[64];

#if CIRCLE_UNSIGNED_INDEX
  if (rznum == NOWHERE || rznum > top_of_zone_table)
  {
#else
  if (rznum < 0 || rznum > top_of_zone_table)
  {
#endif
    log("SYSERR: GenOLC: save_mobiles: Invalid real zone number %d. (0-%d)", rznum, top_of_zone_table);
    return FALSE;
  }

  vznum = zone_table[rznum].number;
  snprintf(mobfname, sizeof(mobfname), "%s%d.new", MOB_PREFIX, vznum);
  if ((mobfd = fopen(mobfname, "w")) == NULL)
  {
    mudlog(BRF, LVL_STAFF, TRUE, "SYSERR: GenOLC: Cannot open mob file for writing.");
    return FALSE;
  }

  for (i = genolc_zone_bottom(rznum); i <= zone_table[rznum].top; i++)
  {
    if ((rmob = real_mobile(i)) == NOBODY)
      continue;
    check_mobile_strings(&mob_proto[rmob]);
    if (write_mobile_record(i, &mob_proto[rmob], mobfd) < 0)
      log("SYSERR: GenOLC: Error writing mobile #%d.", i);
  }
  fputs("$\n", mobfd);
  written = ftell(mobfd);
  fclose(mobfd);
  snprintf(usedfname, sizeof(usedfname), "%s%d.mob", MOB_PREFIX, vznum);
  remove(usedfname);
  rename(mobfname, usedfname);

  if (in_save_list(vznum, SL_MOB))
    remove_from_save_list(vznum, SL_MOB);
  log("GenOLC: '%s' saved, %d bytes written.", usedfname, written);
  return written;
}

int write_mobile_espec(mob_vnum mvnum, struct char_data *mob, FILE *fd)
{
  int i = 0;
  char buf[MAX_STRING_LENGTH] = {'\0'}, buf2[MAX_STRING_LENGTH] = {'\0'};

  if (GET_ATTACK(mob) != 0)
    fprintf(fd, "BareHandAttack: %d\n", GET_ATTACK(mob));
  if (GET_STR(mob) != 11)
    fprintf(fd, "Str: %d\n", GET_STR(mob));
  if (GET_ADD(mob) != 0)
    fprintf(fd, "StrAdd: %d\n", GET_ADD(mob));
  if (GET_DEX(mob) != 11)
    fprintf(fd, "Dex: %d\n", GET_DEX(mob));
  if (GET_INT(mob) != 11)
    fprintf(fd, "Int: %d\n", GET_INT(mob));
  if (GET_WIS(mob) != 11)
    fprintf(fd, "Wis: %d\n", GET_WIS(mob));
  if (GET_CON(mob) != 11)
    fprintf(fd, "Con: %d\n", GET_CON(mob));
  if (GET_CHA(mob) != 11)
    fprintf(fd, "Cha: %d\n", GET_CHA(mob));
  if (GET_SAVE(mob, SAVING_FORT) != 0)
    fprintf(fd, "SavingFort: %d\n", GET_SAVE(mob, SAVING_FORT));
  if (GET_SAVE(mob, SAVING_REFL) != 0)
    fprintf(fd, "SavingRefl: %d\n", GET_SAVE(mob, SAVING_REFL));
  if (GET_SAVE(mob, SAVING_WILL) != 0)
    fprintf(fd, "SavingWill: %d\n", GET_SAVE(mob, SAVING_WILL));
  if (GET_SAVE(mob, SAVING_POISON) != 0)
    fprintf(fd, "SavingPoison: %d\n", GET_SAVE(mob, SAVING_POISON));
  if (GET_SAVE(mob, SAVING_DEATH) != 0)
    fprintf(fd, "SavingDeath: %d\n", GET_SAVE(mob, SAVING_DEATH));
  if (GET_RESISTANCES(mob, DAM_FIRE) != 0)
    fprintf(fd, "ResFire: %d\n", GET_RESISTANCES(mob, DAM_FIRE));
  if (GET_RESISTANCES(mob, DAM_COLD) != 0)
    fprintf(fd, "ResCold: %d\n", GET_RESISTANCES(mob, DAM_COLD));
  if (GET_RESISTANCES(mob, DAM_AIR) != 0)
    fprintf(fd, "ResAir: %d\n", GET_RESISTANCES(mob, DAM_AIR));
  if (GET_RESISTANCES(mob, DAM_EARTH) != 0)
    fprintf(fd, "ResEarth: %d\n", GET_RESISTANCES(mob, DAM_EARTH));
  if (GET_RESISTANCES(mob, DAM_ACID) != 0)
    fprintf(fd, "ResAcid: %d\n", GET_RESISTANCES(mob, DAM_ACID));
  if (GET_RESISTANCES(mob, DAM_HOLY) != 0)
    fprintf(fd, "ResHoly: %d\n", GET_RESISTANCES(mob, DAM_HOLY));
  if (GET_RESISTANCES(mob, DAM_ELECTRIC) != 0)
    fprintf(fd, "ResElectric: %d\n", GET_RESISTANCES(mob, DAM_ELECTRIC));
  if (GET_RESISTANCES(mob, DAM_UNHOLY) != 0)
    fprintf(fd, "ResUnholy: %d\n", GET_RESISTANCES(mob, DAM_UNHOLY));
  if (GET_RESISTANCES(mob, DAM_SLICE) != 0)
    fprintf(fd, "ResSlice: %d\n", GET_RESISTANCES(mob, DAM_SLICE));
  if (GET_RESISTANCES(mob, DAM_PUNCTURE) != 0)
    fprintf(fd, "ResPuncture: %d\n", GET_RESISTANCES(mob, DAM_PUNCTURE));
  if (GET_RESISTANCES(mob, DAM_FORCE) != 0)
    fprintf(fd, "ResForce: %d\n", GET_RESISTANCES(mob, DAM_FORCE));
  if (GET_RESISTANCES(mob, DAM_SOUND) != 0)
    fprintf(fd, "ResSound: %d\n", GET_RESISTANCES(mob, DAM_SOUND));
  if (GET_RESISTANCES(mob, DAM_POISON) != 0)
    fprintf(fd, "ResPoison: %d\n", GET_RESISTANCES(mob, DAM_POISON));
  if (GET_RESISTANCES(mob, DAM_DISEASE) != 0)
    fprintf(fd, "ResDisease: %d\n", GET_RESISTANCES(mob, DAM_DISEASE));
  if (GET_RESISTANCES(mob, DAM_NEGATIVE) != 0)
    fprintf(fd, "ResNegative: %d\n", GET_RESISTANCES(mob, DAM_NEGATIVE));
  if (GET_RESISTANCES(mob, DAM_ILLUSION) != 0)
    fprintf(fd, "ResIllusion: %d\n", GET_RESISTANCES(mob, DAM_ILLUSION));
  if (GET_RESISTANCES(mob, DAM_MENTAL) != 0)
    fprintf(fd, "ResMental: %d\n", GET_RESISTANCES(mob, DAM_MENTAL));
  if (GET_RESISTANCES(mob, DAM_LIGHT) != 0)
    fprintf(fd, "ResLight: %d\n", GET_RESISTANCES(mob, DAM_LIGHT));
  if (GET_RESISTANCES(mob, DAM_ENERGY) != 0)
    fprintf(fd, "ResEnergy: %d\n", GET_RESISTANCES(mob, DAM_ENERGY));
  if (GET_RESISTANCES(mob, DAM_WATER) != 0)
    fprintf(fd, "ResWater: %d\n", GET_RESISTANCES(mob, DAM_WATER));
  if (GET_SUBRACE(mob, 0) != -1)
    fprintf(fd, "SubRace 1: %d\n", GET_SUBRACE(mob, 0));
  if (GET_SUBRACE(mob, 1) != -1)
    fprintf(fd, "SubRace 2: %d\n", GET_SUBRACE(mob, 1));
  if (GET_SUBRACE(mob, 2) != -1)
    fprintf(fd, "SubRace 3: %d\n", GET_SUBRACE(mob, 2));
  if (GET_RACE(mob) != -1)
    fprintf(fd, "Race: %d\n", GET_RACE(mob));
  if (GET_CLASS(mob) != -1)
    fprintf(fd, "Class: %d\n", GET_CLASS(mob));
  if (GET_SIZE(mob) != -1)
    fprintf(fd, "Size: %d\n", GET_SIZE(mob));
  if (GET_WALKIN(mob))
    fprintf(fd, "Walkin: %s\n", GET_WALKIN(mob));
  if (GET_WALKOUT(mob))
    fprintf(fd, "Walkout: %s\n", GET_WALKOUT(mob));
  if (ECHO_ENTRIES(mob) && ECHO_COUNT(mob) > 0)
  {
    if (ECHO_IS_ZONE(mob)) // we don't need to save it if it's false
      fprintf(fd, "EchoZone: %d\n", ECHO_IS_ZONE(mob));
    if (ECHO_SEQUENTIAL(mob))
      fprintf(fd, "EchoSequential: %d\n", ECHO_SEQUENTIAL(mob));
    if (ECHO_FREQ(mob))
      fprintf(fd, "EchoFreq: %d\n", ECHO_FREQ(mob));
    /* storing the echo count is probably unnecessary, we can probably just
     * give all mobiles an echo_entries array, just don't populate unless
     * needed.. for now, the echo_entries array isn't CREATEd unless an
     * EchoCount value is found, although the value has no effect on the
     * amount of memory allocated -Nashak */
    fprintf(fd, "EchoCount: %d\n", ECHO_COUNT(mob));
    for (i = 0; i < ECHO_COUNT(mob); i++)
      if (ECHO_ENTRIES(mob)[i] != NULL)
        fprintf(fd, "Echo: %s\n", ECHO_ENTRIES(mob)[i]);
  }
  /* paths */
  if (PATH_SIZE(mob))
  {
    snprintf(buf, sizeof(buf), "Path: %d:", PATH_RESET(mob));
    for (i = 0; i < PATH_SIZE(mob); i++)
    {
      snprintf(buf2, sizeof(buf2), "%d ", GET_PATH(mob, i));
      strlcat(buf, buf2, sizeof(buf));
    }
    strlcat(buf, "\n", sizeof(buf));
    fprintf(fd, "%s", buf);
  }
  
  // Deprecated by MFeat
  // for (i = 0; i < NUM_FEATS; i++)
  //   if (HAS_FEAT(mob, i))
  //     fprintf(fd, "E\nFeat: %d %d\n", i, HAS_FEAT(mob, i));

  for (i = 0; i < NUM_FEATS; i++)
    if (MOB_HAS_FEAT(mob, i))
      fprintf(fd, "MFeat: %d %d\n", i, MOB_HAS_FEAT(mob, i));

  fprintf(fd, "DR_MOD: %d\n", GET_DR_MOD(mob));

  for (i = 0; i < NUM_SPELLS; i++)
  {
    if (MOB_KNOWS_SPELL(mob, i))
      fprintf(fd, "KnownSpell: %d\n", i);
  }

  /* finalize */
  fputs("E\n", fd);
  return TRUE;
}

int write_mobile_record(mob_vnum mvnum, struct char_data *mob, FILE *fd)
{
  int pos = GET_DEFAULT_POS(mob);

  char ldesc[MAX_STRING_LENGTH] = {'\0'};
  char ddesc[MAX_STRING_LENGTH] = {'\0'};
  char buf[MAX_STRING_LENGTH] = {'\0'};

  ldesc[MAX_STRING_LENGTH - 1] = '\0';
  ddesc[MAX_STRING_LENGTH - 1] = '\0';
  strip_cr(strncpy(ldesc, GET_LDESC(mob), MAX_STRING_LENGTH - 1));
  strip_cr(strncpy(ddesc, GET_DDESC(mob), MAX_STRING_LENGTH - 1));

  snprintf(buf, sizeof(buf), "#%d\n"
                             "%s%c\n"
                             "%s%c\n"
                             "%s%c\n"
                             "%s%c\n",
           mvnum,
           GET_ALIAS(mob), STRING_TERMINATOR,
           GET_SDESC(mob), STRING_TERMINATOR,
           ldesc, STRING_TERMINATOR,
           ddesc, STRING_TERMINATOR);

  fprintf(fd, convert_from_tabs(buf), 0);

  fprintf(fd, "%d %d %d %d %d %d %d %d %d E\n"
              "%d %d %d %dd%d+%d %dd%d+%d\n",
          MOB_FLAGS(mob)[0], MOB_FLAGS(mob)[1],
          MOB_FLAGS(mob)[2], MOB_FLAGS(mob)[3],
          AFF_FLAGS(mob)[0], AFF_FLAGS(mob)[1],
          AFF_FLAGS(mob)[2], AFF_FLAGS(mob)[3],
          GET_ALIGNMENT(mob),
          /* line 2 */
          /* AC -> we are doing a two-fold conversion here (HACK ALERT)
             (1) reduce factor by 10
             (2) 20 - 3rd edition DnD = 2nd edition DnD AC
             WHY?!  Because all of our old mobile files are saved as
             2nd edition DnD AC!
           * this is the opposite of what is done in db.c's parse_simple_mob */
          GET_LEVEL(mob), 20 - GET_HITROLL(mob), (20 - (GET_AC(mob) / 10)), GET_HIT(mob),
          GET_PSP(mob), GET_MOVE(mob), GET_NDD(mob), GET_SDD(mob),
          GET_DAMROLL(mob));

  /* position fighting is deprecated */
  if (pos == POS_FIGHTING)
    pos = POS_STANDING;

  fprintf(fd, "%d %d\n"
              "%d %d %d\n",
          GET_GOLD(mob), GET_EXP(mob),
          GET_POS(mob), pos, GET_SEX(mob));

  if (write_mobile_espec(mvnum, mob, fd) < 0)
    log("SYSERR: GenOLC: Error writing E-specs for mobile #%d.", mvnum);

  script_save_to_disk(fd, mob, MOB_TRIGGER);

#if CONFIG_GENOLC_MOBPROG
  if (write_mobile_mobprog(mvnum, mob, fd) < 0)
    log("SYSERR: GenOLC: Error writing MobProgs for mobile #%d.", mvnum);
#endif

  return TRUE;
}

void check_mobile_strings(struct char_data *mob)
{
  mob_vnum mvnum = mob_index[mob->nr].vnum;
  check_mobile_string(mvnum, &GET_LDESC(mob), "long description");
  check_mobile_string(mvnum, &GET_DDESC(mob), "detailed description");
  check_mobile_string(mvnum, &GET_ALIAS(mob), "alias list");
  check_mobile_string(mvnum, &GET_SDESC(mob), "short description");
  // check_mobile_string(mvnum, &GET_WALKIN(mob), "walkin");
  // check_mobile_string(mvnum, &GET_WALKOUT(mob), "walkout");
}

void check_mobile_string(mob_vnum i, char **string, const char *desc)
{
  if (*string == NULL || **string == '\0')
  {
    char smbuf[128];
    snprintf(smbuf, sizeof(smbuf), "GenOLC: Mob #%d has an invalid %s.", i, desc);
    mudlog(BRF, LVL_STAFF, TRUE, "%s", smbuf);
    if (*string)
      free(*string);
    *string = strdup("An undefined string.\n");
  }
}
