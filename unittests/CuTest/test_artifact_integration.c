/* ***************************************************************************
 *  File: test_artifact_integration.c                 Part of LuminariMUD
 *  Usage: Booted-world integration coverage for the artifact system.
 *
 *  test_artifacts.c covers the parts of src/obj/spec_artifacts.c that need no
 *  world: registry lookup, the XP curve, file round-tripping.  This file is
 *  the other half.  It stands up a real registry through artifact_boot()
 *  against the shipped artifact_templates[], artifact_contracts[],
 *  artifact_passives[], and artifact_effects[] tables, puts a real player in
 *  a real room, and drives production entry points from acquisition through
 *  destruction.
 *
 *  Everything here runs inside a scratch directory, so no real game data is
 *  read or written.
 *************************************************************************** */

#include "CuTest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/interpreter.h"
#include "../../src/actionqueues.h"
#include "../../src/combat/fight.h"
#include "../../src/magic/domains_schools.h"
#include "../../src/magic/spells.h"
#include "../../src/net/protocol.h"
#include "../../src/obj/spec_artifacts.h"

/* Production look helper exercised by Wyrmfang's danger-sense contract. */
void check_dangersense(struct char_data *ch, room_rnum room);

/* --------------------------------------------------------------------------
 * The fixture
 *
 * artifact_boot() only registers a template whose vnum resolves through
 * real_object(), so the object table has to exist before the registry does.
 * Building it by hand from ART_VNUM_* rather than from world files keeps this
 * hermetic while still booting the real shipped metadata.
 * -------------------------------------------------------------------------- */

#define ARTINT_OBJ_COUNT 17

static const int artint_vnums[ARTINT_OBJ_COUNT] = {
    ART_VNUM_TRORXEK,     ART_VNUM_AMAUKEKEL, ART_VNUM_FADE,    ART_VNUM_HENEKAR,
    ART_VNUM_DOOMBRINGER, ART_VNUM_KELRARIN,  ART_VNUM_KELROM,  ART_VNUM_GESEN,
    ART_VNUM_STINGER,     ART_VNUM_AVERNUS,   ART_VNUM_AEGIS,   ART_VNUM_VENGEANCE,
    ART_VNUM_EARTHCRIER,  ART_VNUM_WYRMFANG,  ART_VNUM_COURAGE, ART_VNUM_ICEDGE,
    ART_VNUM_TWILIGHT};

struct artint_fixture
{
  /* saved globals */
  struct index_data *saved_obj_index;
  struct obj_data *saved_obj_proto;
  obj_rnum saved_top_of_objt;
  struct room_data *saved_world;
  room_rnum saved_top_of_world;
  struct zone_data *saved_zone_table;
  zone_rnum saved_top_of_zone_table;
  struct index_data *saved_mob_index;
  mob_rnum saved_top_of_mobt;
  char saved_cwd[PATH_MAX];
  int in_sandbox;

  /* fixture state */
  struct index_data indexes[ARTINT_OBJ_COUNT];
  struct obj_data protos[ARTINT_OBJ_COUNT];
  struct room_data rooms[2];
  struct zone_data zones[1];
  struct index_data mobile_index[1];

  struct char_data actor;
  struct char_data bystander;
  struct char_data victim;
  struct descriptor_data descriptor;

  char sandbox[PATH_MAX];
};

/* The registry only has to be walked by index; the vnum table above is the
 * authority on membership. */
static int artint_rnum_of(int vnum)
{
  int i = 0;

  for (i = 0; i < ARTINT_OBJ_COUNT; i++)
    if (artint_vnums[i] == vnum)
      return i;

  return NOTHING;
}

static void artint_clear_output(struct artint_fixture *fixture)
{
  fixture->descriptor.small_outbuf[0] = '\0';
  fixture->descriptor.output = fixture->descriptor.small_outbuf;
  fixture->descriptor.bufptr = 0;
  fixture->descriptor.bufspace = SMALL_BUFSIZE - 1;
}

static int artint_said(struct artint_fixture *fixture, const char *needle)
{
  return (strstr(fixture->descriptor.output, needle) != NULL);
}

static void artint_make_player(struct char_data *ch, const char *name, room_rnum room)
{
  clear_char(ch);
  CREATE(ch->player_specials, struct player_special_data, 1);
  ch->player.name = strdup(name);
  ch->player.short_descr = strdup(name);
  GET_LEVEL(ch) = 20;
  GET_REAL_STR(ch) = 10;
  GET_REAL_DEX(ch) = 10;
  GET_REAL_CON(ch) = 10;
  GET_REAL_INT(ch) = 10;
  GET_REAL_WIS(ch) = 10;
  GET_REAL_CHA(ch) = 10;
  ch->aff_abils = ch->real_abils;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = 500;
  GET_MAX_HIT(ch) = 500;
  GET_MOVE(ch) = 200;
  GET_MAX_MOVE(ch) = 200;
  GET_PSP(ch) = 200;
  GET_MAX_PSP(ch) = 200;
  GET_REAL_RACE(ch) = RACE_HUMAN;
  IN_ROOM(ch) = room;
  GET_ATTACK_QUEUE(ch) = create_attack_queue();
}

static void artint_free_player(struct char_data *ch)
{
  while (ch->affected != NULL)
    affect_remove_no_total(ch, ch->affected);

  if (ch->player.name)
    free(ch->player.name);
  if (ch->player.short_descr)
    free(ch->player.short_descr);
  if (ch->player_specials)
    free(ch->player_specials);
  if (GET_ATTACK_QUEUE(ch))
    free_attack_queue(GET_ATTACK_QUEUE(ch));
  GET_ATTACK_QUEUE(ch) = NULL;

  ch->player.name = NULL;
  ch->player.short_descr = NULL;
  ch->player_specials = NULL;
}

static void artint_make_npc(struct char_data *ch, const char *name, room_rnum room)
{
  clear_char(ch);
  SET_BIT_AR(MOB_FLAGS(ch), MOB_ISNPC);
  ch->player_specials = &dummy_mob;
  ch->player.short_descr = (char *)name;
  ch->player.name = (char *)name;
  GET_LEVEL(ch) = 15;
  GET_POS(ch) = POS_STANDING;
  GET_HIT(ch) = 400;
  GET_MAX_HIT(ch) = 400;
  IN_ROOM(ch) = room;
  GET_ATTACK_QUEUE(ch) = create_attack_queue();
}

/* Returns FALSE when the sandbox could not be made; every test bails out in
 * that case rather than touching lib/. */
static int artint_begin(struct artint_fixture *fixture)
{
  char dir[] = "/tmp/lum_artifact_integration_XXXXXX";
  char worlddir[PATH_MAX];
  int i = 0;

  memset(fixture, 0, sizeof(*fixture));

  if (!getcwd(fixture->saved_cwd, sizeof(fixture->saved_cwd)))
    return FALSE;

  if (!mkdtemp(dir))
    return FALSE;

  snprintf(worlddir, sizeof(worlddir), "%s/world", dir);
  if (mkdir(worlddir, 0700) != 0)
    return FALSE;

  if (chdir(dir) != 0)
    return FALSE;

  snprintf(fixture->sandbox, sizeof(fixture->sandbox), "%s", dir);
  fixture->in_sandbox = TRUE;

  fixture->saved_obj_index = obj_index;
  fixture->saved_obj_proto = obj_proto;
  fixture->saved_top_of_objt = top_of_objt;
  fixture->saved_world = world;
  fixture->saved_top_of_world = top_of_world;
  fixture->saved_zone_table = zone_table;
  fixture->saved_top_of_zone_table = top_of_zone_table;
  fixture->saved_mob_index = mob_index;
  fixture->saved_top_of_mobt = top_of_mobt;

  for (i = 0; i < ARTINT_OBJ_COUNT; i++)
  {
    fixture->indexes[i].vnum = artint_vnums[i];
    clear_object(&fixture->protos[i]);
    GET_OBJ_RNUM(&fixture->protos[i]) = i;
    fixture->protos[i].name = (char *)"artifact test";
    fixture->protos[i].short_description = (char *)"a test artifact";
    fixture->protos[i].description = (char *)"A test artifact lies here.";
    GET_OBJ_TYPE(&fixture->protos[i]) = ITEM_WEAPON;
  }

  obj_index = fixture->indexes;
  obj_proto = fixture->protos;
  top_of_objt = ARTINT_OBJ_COUNT - 1;

  fixture->rooms[0].number = 169900;
  fixture->rooms[0].zone = 0;
  fixture->rooms[0].sector_type = SECT_INSIDE;
  fixture->rooms[0].name = (char *)"Artifact integration origin";
  fixture->rooms[0].description = (char *)"A production-linked artifact test room.\r\n";
  fixture->rooms[1].number = 169901;
  fixture->rooms[1].zone = 0;
  fixture->rooms[1].sector_type = SECT_INSIDE;
  fixture->rooms[1].name = (char *)"Artifact integration annex";
  fixture->rooms[1].description = (char *)"A second artifact test room.\r\n";

  fixture->zones[0].number = 1699;
  fixture->zones[0].bot = 169900;
  fixture->zones[0].top = 169999;
  fixture->zones[0].min_level = -1;
  fixture->zones[0].max_level = LVL_IMPL;

  world = fixture->rooms;
  top_of_world = 1;
  zone_table = fixture->zones;
  top_of_zone_table = 0;

  /* damage() resolves the victim's prototype, so the mobile table has to
   * exist for any proc that deals damage. */
  fixture->mobile_index[0].vnum = 1;
  mob_index = fixture->mobile_index;
  top_of_mobt = 0;

  artint_make_player(&fixture->actor, "Artifactor", 0);
  artint_make_player(&fixture->bystander, "Bystander", 0);
  artint_make_npc(&fixture->victim, "a test victim", 0);

  fixture->rooms[0].people = &fixture->actor;
  fixture->actor.next_in_room = &fixture->bystander;
  fixture->bystander.next_in_room = &fixture->victim;

  memset(&fixture->descriptor, 0, sizeof(fixture->descriptor));
  fixture->descriptor.character = &fixture->actor;
  fixture->descriptor.pProtocol = ProtocolCreate();
  artint_clear_output(fixture);
  fixture->actor.desc = &fixture->descriptor;

  artifact_boot();

  return (art_index != NULL && total_artifacts == ARTINT_OBJ_COUNT);
}

static void artint_end(struct artint_fixture *fixture)
{
  artifact_shutdown();

  fixture->actor.desc = NULL;
  if (fixture->descriptor.pProtocol)
  {
    ProtocolDestroy(fixture->descriptor.pProtocol);
    fixture->descriptor.pProtocol = NULL;
  }
  if (fixture->descriptor.large_outbuf)
  {
    free(fixture->descriptor.large_outbuf->text);
    free(fixture->descriptor.large_outbuf);
    fixture->descriptor.large_outbuf = NULL;
    fixture->descriptor.output = fixture->descriptor.small_outbuf;
    if (buf_largecount > 0)
      buf_largecount--;
  }

  fixture->rooms[0].people = NULL;
  fixture->rooms[1].people = NULL;
  fixture->actor.next_in_room = NULL;
  fixture->bystander.next_in_room = NULL;
  fixture->victim.next_in_room = NULL;

  while (fixture->victim.affected != NULL)
    affect_remove_no_total(&fixture->victim, fixture->victim.affected);

  if (GET_ATTACK_QUEUE(&fixture->victim))
    free_attack_queue(GET_ATTACK_QUEUE(&fixture->victim));
  GET_ATTACK_QUEUE(&fixture->victim) = NULL;

  artint_free_player(&fixture->actor);
  artint_free_player(&fixture->bystander);

  obj_index = fixture->saved_obj_index;
  obj_proto = fixture->saved_obj_proto;
  top_of_objt = fixture->saved_top_of_objt;
  world = fixture->saved_world;
  top_of_world = fixture->saved_top_of_world;
  zone_table = fixture->saved_zone_table;
  top_of_zone_table = fixture->saved_top_of_zone_table;
  mob_index = fixture->saved_mob_index;
  top_of_mobt = fixture->saved_top_of_mobt;

  if (fixture->in_sandbox && chdir(fixture->saved_cwd) != 0)
    fixture->in_sandbox = FALSE;
}

/* A live instance of one artifact, in the actor's inventory but not yet
 * claimed. */
static void artint_instance(struct artint_fixture *fixture, struct obj_data *obj, int vnum)
{
  clear_object(obj);
  GET_OBJ_RNUM(obj) = artint_rnum_of(vnum);
  obj->name = (char *)"artifact test";
  obj->short_description = (char *)"a test artifact";
  obj->description = (char *)"A test artifact lies here.";
  GET_OBJ_TYPE(obj) = ITEM_WEAPON;
  IN_ROOM(obj) = NOWHERE;
  (void)fixture;
}

/* Put an instance in the actor's inventory the way get/give does. */
static void artint_carry(struct artint_fixture *fixture, struct obj_data *obj)
{
  obj->carried_by = &fixture->actor;
  obj->next_content = fixture->actor.carrying;
  fixture->actor.carrying = obj;
  IN_ROOM(obj) = NOWHERE;
  obj_index[GET_OBJ_RNUM(obj)].number++;
}

static void artint_uncarry(struct artint_fixture *fixture, struct obj_data *obj)
{
  struct obj_data **prev = &fixture->actor.carrying;

  while (*prev)
  {
    if (*prev == obj)
    {
      *prev = obj->next_content;
      break;
    }
    prev = &(*prev)->next_content;
  }

  obj->carried_by = NULL;
  obj->next_content = NULL;
  if (obj_index[GET_OBJ_RNUM(obj)].number > 0)
    obj_index[GET_OBJ_RNUM(obj)].number--;
}

struct artint_lethal_result
{
  int hook_reported_death;
  int extraction_marked;
  int proc_message_seen;
  int downstream_vampiric_seen;
};

/* Drive the real successful-hit boundary with a disposable production mobile.
 * The one-hit target is heap allocated through read_mobile() because a lethal
 * damage() call marks NPCs for deferred extraction. */
static int artint_run_lethal_outer_hook(struct artint_fixture *fixture, struct obj_data *obj,
                                        int artifact_vnum, int generic_proc,
                                        struct artint_lethal_result *result)
{
  struct artifact_data *art = NULL;
  struct char_data mobile_proto;
  struct char_data *victim = NULL;
  struct char_data *saved_character_list = NULL;
  struct char_data *saved_mob_proto = NULL;
  struct obj_data *saved_object_list = NULL;
  int base_damage = 0;

  if (!fixture || !obj || !result)
    return FALSE;

  memset(result, 0, sizeof(*result));
  saved_character_list = character_list;
  saved_mob_proto = mob_proto;
  saved_object_list = object_list;

  clear_char(&mobile_proto);
  SET_BIT_AR(MOB_FLAGS(&mobile_proto), MOB_ISNPC);
  mobile_proto.player_specials = &dummy_mob;
  mobile_proto.player.name = (char *)"artifact lethal target";
  mobile_proto.player.short_descr = (char *)"an artifact lethal target";
  GET_MOB_RNUM(&mobile_proto) = 0;
  GET_LEVEL(&mobile_proto) = 1;
  GET_CLASS(&mobile_proto) = CLASS_WARRIOR;
  GET_REAL_RACE(&mobile_proto) = RACE_HUMAN;
  GET_REAL_SIZE(&mobile_proto) = SIZE_MEDIUM;
  GET_POS(&mobile_proto) = POS_STANDING;
  GET_DEFAULT_POS(&mobile_proto) = POS_STANDING;
  GET_HIT(&mobile_proto) = 2;
  GET_PSP(&mobile_proto) = 2;
  GET_MAX_HIT(&mobile_proto) = 1;

  character_list = NULL;
  object_list = NULL;
  mob_proto = &mobile_proto;
  victim = read_mobile(0, REAL);
  if (!victim)
  {
    character_list = saved_character_list;
    object_list = saved_object_list;
    mob_proto = saved_mob_proto;
    return FALSE;
  }

  GET_HIT(victim) = 1;
  GET_MAX_HIT(victim) = 1;
  char_to_room(victim, 0);
  SET_BIT_AR(AFF_FLAGS(&fixture->actor), AFF_VAMPIRIC_TOUCH);

  artint_instance(fixture, obj, artifact_vnum);
  artint_carry(fixture, obj);
  GET_EQ(&fixture->actor, WEAR_WIELD_2H) = obj;
  obj->worn_by = &fixture->actor;
  obj->worn_on = WEAR_WIELD_2H;

  art = artifact_by_vnum(artifact_vnum);
  if (!art)
  {
    extract_char(victim);
    extract_pending_chars();
    character_list = saved_character_list;
    object_list = saved_object_list;
    mob_proto = saved_mob_proto;
    GET_EQ(&fixture->actor, WEAR_WIELD_2H) = NULL;
    obj->worn_by = NULL;
    artint_uncarry(fixture, obj);
    return FALSE;
  }

  art->level = generic_proc ? 1 : ARTIFACT_MAX_LEVEL;
  art->last_proc = 0;
  if (generic_proc)
  {
    art->sig_proc = ART_SIG_NONE;
    art->sig_chance = 0;
    art->proc_chance = 100;
  }
  else
  {
    art->sig_chance = 100;
    art->proc_chance = 0;
  }

  FIGHTING(&fixture->actor) = victim;
  FIGHTING(victim) = &fixture->actor;
  base_damage = 1 + compute_hit_damage(&fixture->actor, victim, TYPE_HIT, 10, MODE_NORMAL_HIT,
                                       FALSE, ATTACK_TYPE_PRIMARY, DAM_SLICE);
  GET_HIT(victim) = base_damage - 10;
  GET_MAX_HIT(victim) = MAX(1, base_damage);
  GET_HIT(&fixture->actor) = GET_MAX_HIT(&fixture->actor);
  artint_clear_output(fixture);

  result->hook_reported_death =
      test_handle_successful_artifact_attack(&fixture->actor, victim, obj, 1, FALSE, DAM_SLICE);
  result->extraction_marked = MOB_FLAGGED(victim, MOB_NOTDEADYET);
  result->proc_message_seen =
      generic_proc ? artint_said(fixture, "tears at") : artint_said(fixture, "Five colors");
  result->downstream_vampiric_seen = artint_said(fixture, "vampiric");

  if (!MOB_FLAGGED(victim, MOB_NOTDEADYET))
    extract_char(victim);
  extract_pending_chars();

  while (object_list)
    extract_obj(object_list);

  character_list = saved_character_list;
  object_list = saved_object_list;
  mob_proto = saved_mob_proto;
  FIGHTING(&fixture->actor) = NULL;
  fixture->actor.last_attacker = NULL;
  GET_EQ(&fixture->actor, WEAR_WIELD_2H) = NULL;
  obj->worn_by = NULL;
  obj->worn_on = -1;
  artint_uncarry(fixture, obj);

  return TRUE;
}

struct artint_identity_case
{
  int vnum;
  const char *ability_name;
  int generic_proc_chance;
  int signature_proc;
  int hand_proc_vnum;
  int hand_entry_odds;
  int called_effects[ARTIFACT_MAX_EFFECTS];
  int called_channels[ARTIFACT_MAX_EFFECTS];
  int called_stack_groups[ARTIFACT_MAX_EFFECTS];
  int passive_count;
};

struct artint_passive_case
{
  int vnum;
  int min_level;
  int aff_flag;
  int location;
  int modifier;
};

/* This is an identity contract, not a coverage shortcut.  A zero signature,
 * zero effect, NULL ability, or NOTHING hand procedure is a deliberate none.
 * Confirmed gaps stay explicit until their individual audit item changes the
 * production behavior and this row together. */
/* clang-format off */
static const struct artint_identity_case artint_identity_cases[ARTINT_OBJ_COUNT] = {
    {ART_VNUM_TRORXEK, NULL, 12, ART_SIG_NONE, ART_VNUM_TRORXEK, 0,
     {ART_EFFECT_SUMMON_TREANT, ART_EFFECT_CREEPING_DOOM, ART_EFFECT_RECALL,
      ART_EFFECT_TRAVEL_TO},
     {ART_INVOKE_SAY, ART_INVOKE_SAY, ART_INVOKE_SAY, ART_INVOKE_SAY},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 0},
    {ART_VNUM_AMAUKEKEL, "divineward", 0, ART_SIG_NONE, NOTHING, 0,
     {ART_EFFECT_DIMENSION_SHIFT, ART_EFFECT_RESURRECT, ART_EFFECT_DISPEL_EVIL, 0},
     {ART_INVOKE_SAY, ART_INVOKE_SAY, ART_INVOKE_SAY, NOTHING},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 0},
    {ART_VNUM_FADE, NULL, 16, ART_SIG_NONE, ART_VNUM_FADE, ARTIFACT_FADE_DRAIN_ODDS,
     {ART_EFFECT_BLIND, ART_EFFECT_DARKNESS, ART_EFFECT_WEAKEN, ART_EFFECT_TRAVEL_TO},
     {ART_INVOKE_SAY, ART_INVOKE_SAY, ART_INVOKE_SAY, ART_INVOKE_SAY},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 0},
    {ART_VNUM_HENEKAR, NULL, 0, ART_SIG_NONE, NOTHING, 0,
     {ART_EFFECT_BLIND, ART_EFFECT_PACIFY, ART_EFFECT_CHARM, ART_EFFECT_TRAVEL_TO},
     {ART_INVOKE_SAY, ART_INVOKE_SAY, ART_INVOKE_SAY, ART_INVOKE_SAY},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 0},
    {ART_VNUM_DOOMBRINGER, "doomblast", 20, ART_SIG_NONE, ART_VNUM_DOOMBRINGER,
     ARTIFACT_DOOMBRINGER_BURST_ODDS,
     {ART_EFFECT_ANNIHILATION, ART_EFFECT_BLACK_LIGHTNING, ART_EFFECT_ENRAGE, 0},
     {ART_INVOKE_SAY, ART_INVOKE_SAY, ART_INVOKE_SAY, NOTHING},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_COMBAT_SURGE, ART_STACK_NONE}, 0},
    {ART_VNUM_KELRARIN, "soulstrike", 15, ART_SIG_NONE, ART_VNUM_KELRARIN, 0,
     {0, 0, 0, 0}, {NOTHING, NOTHING, NOTHING, NOTHING},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 0},
    {ART_VNUM_KELROM, NULL, 14, ART_SIG_NONE, ART_VNUM_KELROM, 0,
     {0, 0, 0, 0}, {NOTHING, NOTHING, NOTHING, NOTHING},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 0},
    {ART_VNUM_GESEN, NULL, 18, ART_SIG_NONE, ART_VNUM_GESEN, 0,
     {0, 0, 0, 0}, {NOTHING, NOTHING, NOTHING, NOTHING},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 0},
    {ART_VNUM_STINGER, NULL, 18, ART_SIG_LIFESTEAL, NOTHING, 0,
     {0, 0, 0, 0}, {NOTHING, NOTHING, NOTHING, NOTHING},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 0},
    {ART_VNUM_AVERNUS, NULL, 15, ART_SIG_NONE, ART_VNUM_AVERNUS,
     ARTIFACT_AVERNUS_DRAIN_ODDS,
     {0, 0, 0, 0}, {NOTHING, NOTHING, NOTHING, NOTHING},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 0},
    {ART_VNUM_AEGIS, NULL, 0, ART_SIG_NONE, NOTHING, 0,
     {0, 0, 0, 0}, {NOTHING, NOTHING, NOTHING, NOTHING},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 0},
    {ART_VNUM_VENGEANCE, NULL, 0, ART_SIG_MERCY, NOTHING, 0,
     {0, 0, 0, 0}, {NOTHING, NOTHING, NOTHING, NOTHING},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 3},
    {ART_VNUM_EARTHCRIER, NULL, 0, ART_SIG_KNOCKDOWN, NOTHING, 0,
     {0, 0, 0, 0}, {NOTHING, NOTHING, NOTHING, NOTHING},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 2},
    {ART_VNUM_WYRMFANG, NULL, 0, ART_SIG_WEIGHTED, NOTHING, 0,
     {ART_EFFECT_DRAGON_SIGHT, 0, 0, 0},
     {ART_INVOKE_COMMAND, NOTHING, NOTHING, NOTHING},
     {ART_STACK_WARD, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 6},
    {ART_VNUM_COURAGE, NULL, 0, ART_SIG_NONE, NOTHING, 0,
     {ART_EFFECT_GROUP_VALOR, 0, 0, 0},
     {ART_INVOKE_SAY, NOTHING, NOTHING, NOTHING},
     {ART_STACK_MORALE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 4},
    {ART_VNUM_ICEDGE, NULL, 0, ART_SIG_FLURRY, NOTHING, 0,
     {ART_EFFECT_FROST_WARD, 0, 0, 0},
     {ART_INVOKE_WHISPER, NOTHING, NOTHING, NOTHING},
     {ART_STACK_WARD, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 3},
    {ART_VNUM_TWILIGHT, NULL, 0, ART_SIG_SURGE, NOTHING, 0,
     {0, 0, 0, 0}, {NOTHING, NOTHING, NOTHING, NOTHING},
     {ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE, ART_STACK_NONE}, 4}};

static const struct artint_passive_case artint_passive_cases[] = {
    {ART_VNUM_WYRMFANG, 1, AFF_DETECT_INVIS, APPLY_NONE, 0},
    {ART_VNUM_WYRMFANG, 2, AFF_INFRAVISION, APPLY_NONE, 0},
    {ART_VNUM_WYRMFANG, 3, AFF_SENSE_LIFE, APPLY_NONE, 0},
    {ART_VNUM_WYRMFANG, 4, AFF_FARSEE, APPLY_NONE, 0},
    {ART_VNUM_WYRMFANG, 5, AFF_HASTE, APPLY_NONE, 0},
    {ART_VNUM_WYRMFANG, 5, AFF_DANGERSENSE, APPLY_NONE, 0},
    {ART_VNUM_COURAGE, 1, 0, APPLY_SAVING_WILL, 2},
    {ART_VNUM_COURAGE, 2, 0, APPLY_RES_ELECTRIC, 10},
    {ART_VNUM_COURAGE, 3, 0, APPLY_SAVING_FORT, 2},
    {ART_VNUM_COURAGE, 4, AFF_HASTE, APPLY_NONE, 0},
    {ART_VNUM_ICEDGE, 1, 0, APPLY_RES_COLD, 15},
    {ART_VNUM_ICEDGE, 3, 0, APPLY_SPELL_RES, 4},
    {ART_VNUM_ICEDGE, 5, AFF_TRUE_SIGHT, APPLY_NONE, 0},
    {ART_VNUM_TWILIGHT, 2, AFF_INFRAVISION, APPLY_NONE, 0},
    {ART_VNUM_TWILIGHT, 3, AFF_SENSE_LIFE, APPLY_NONE, 0},
    {ART_VNUM_TWILIGHT, 4, AFF_FARSEE, APPLY_NONE, 0},
    {ART_VNUM_TWILIGHT, 5, AFF_HASTE, APPLY_NONE, 0},
    {ART_VNUM_VENGEANCE, 1, AFF_DETECT_INVIS, APPLY_NONE, 0},
    {ART_VNUM_VENGEANCE, 3, 0, APPLY_SAVING_WILL, 3},
    {ART_VNUM_VENGEANCE, 5, 0, APPLY_RES_UNHOLY, 15},
    {ART_VNUM_EARTHCRIER, 2, 0, APPLY_SAVING_FORT, 3},
    {ART_VNUM_EARTHCRIER, 4, 0, APPLY_RES_PUNCTURE, 10}};
/* clang-format on */

static const struct artint_passive_case *artint_expected_passive(int vnum, int ordinal)
{
  int i = 0, found = 0;

  for (i = 0; i < (int)(sizeof(artint_passive_cases) / sizeof(artint_passive_cases[0])); i++)
  {
    if (artint_passive_cases[i].vnum != vnum)
      continue;
    if (found == ordinal)
      return &artint_passive_cases[i];
    found++;
  }

  return NULL;
}

static void artint_record_identity_mismatch(char *failure, size_t failure_size, int vnum,
                                            const char *field, int expected, int actual)
{
  if (!failure || failure[0] != '\0' || expected == actual)
    return;

  snprintf(failure, failure_size, "artifact %d identity %s: expected %d, got %d", vnum, field,
           expected, actual);
}

/* --------------------------------------------------------------------------
 * The fixture itself
 * -------------------------------------------------------------------------- */

void Test_artifact_integration_boot_registers_every_shipped_artifact(CuTest *tc)
{
  struct artint_fixture fixture;
  int i = 0, contracted = 0, in_order = TRUE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  for (i = 0; i < total_artifacts; i++)
  {
    if (i > 0 && art_index[i].vnum <= art_index[i - 1].vnum)
      in_order = FALSE;
    if (art_index[i].acquisition != ART_ACQ_UNSET)
      contracted++;
  }

  CuAssertIntEquals(tc, ARTINT_OBJ_COUNT, total_artifacts);
  CuAssertIntEquals(tc, TRUE, in_order);
  /* Every shipped artifact has a contract row; a missing one is the gap the
   * validator reports. */
  CuAssertIntEquals(tc, ARTINT_OBJ_COUNT, contracted);
  /* The validator returns a problem count, so a clean boot is zero. */
  CuAssertIntEquals(tc, 0, artifact_validate_metadata());

  artint_end(&fixture);
}

void Test_artifact_integration_aegis_lore_matches_breastplate_identity(CuTest *tc)
{
  struct artint_fixture fixture;
  struct artifact_data *art = NULL;
  int lore_matches = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  art = artifact_by_vnum(ART_VNUM_AEGIS);
  lore_matches =
      art && art->lore && strstr(art->lore, "breastplate") && !strstr(art->lore, "shield");

  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, lore_matches);
}

void Test_artifact_integration_every_artifact_has_an_explicit_identity_contract(CuTest *tc)
{
  struct artint_fixture fixture;
  struct artifact_test_identity_data actual;
  const struct artint_identity_case *expected = NULL;
  const struct artint_passive_case *passive = NULL;
  char failure[256], field[64];
  int i = 0, j = 0;

  failure[0] = '\0';
  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  artint_record_identity_mismatch(failure, sizeof(failure), 0, "roster count", ARTINT_OBJ_COUNT,
                                  total_artifacts);

  for (i = 0; i < ARTINT_OBJ_COUNT && failure[0] == '\0'; i++)
  {
    expected = &artint_identity_cases[i];
    memset(&actual, 0, sizeof(actual));
    artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum, "fixture roster vnum",
                                    expected->vnum, artint_vnums[i]);
    artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum,
                                    "registered roster vnum", expected->vnum, art_index[i].vnum);

    if (failure[0] == '\0' && !artifact_identity_for_test(expected->vnum, &actual))
      snprintf(failure, sizeof(failure), "artifact %d has no production identity", expected->vnum);

    if (failure[0] == '\0' &&
        ((expected->ability_name == NULL) != (actual.ability_name == NULL) ||
         (expected->ability_name && strcmp(expected->ability_name, actual.ability_name))))
      snprintf(failure, sizeof(failure), "artifact %d active ability: expected %s, got %s",
               expected->vnum, expected->ability_name ? expected->ability_name : "none",
               actual.ability_name ? actual.ability_name : "none");

    artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum, "generic proc chance",
                                    expected->generic_proc_chance, actual.generic_proc_chance);
    artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum, "signature proc",
                                    expected->signature_proc, actual.signature_proc);
    artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum, "hand procedure",
                                    expected->hand_proc_vnum, actual.hand_proc_vnum);
    artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum, "hand entry odds",
                                    expected->hand_entry_odds, actual.hand_entry_odds);

    for (j = 0; j < ARTIFACT_MAX_EFFECTS && failure[0] == '\0'; j++)
    {
      snprintf(field, sizeof(field), "called effect slot %d", j);
      artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum, field,
                                      expected->called_effects[j], actual.called_effects[j]);
      snprintf(field, sizeof(field), "called channel slot %d", j);
      artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum, field,
                                      expected->called_channels[j], actual.called_channels[j]);
      snprintf(field, sizeof(field), "called stack group slot %d", j);
      artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum, field,
                                      expected->called_stack_groups[j],
                                      actual.called_stack_groups[j]);
    }

    artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum, "passive count",
                                    expected->passive_count, actual.passive_count);
    for (j = 0; j < expected->passive_count && failure[0] == '\0'; j++)
    {
      passive = artint_expected_passive(expected->vnum, j);
      if (!passive)
      {
        snprintf(failure, sizeof(failure), "artifact %d expected passive %d is undefined",
                 expected->vnum, j);
        break;
      }

      snprintf(field, sizeof(field), "passive %d level", j);
      artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum, field,
                                      passive->min_level, actual.passives[j].min_level);
      snprintf(field, sizeof(field), "passive %d affect", j);
      artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum, field,
                                      passive->aff_flag, actual.passives[j].aff_flag);
      snprintf(field, sizeof(field), "passive %d location", j);
      artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum, field,
                                      passive->location, actual.passives[j].location);
      snprintf(field, sizeof(field), "passive %d modifier", j);
      artint_record_identity_mismatch(failure, sizeof(failure), expected->vnum, field,
                                      passive->modifier, actual.passives[j].modifier);
    }
  }

  artint_end(&fixture);
  CuAssert(tc, failure[0] ? failure : "artifact identity contract mismatch", failure[0] == '\0');
}

/* --------------------------------------------------------------------------
 * Lifecycle: acquire, equip, bind, unequip, drop, save, reload, destroy
 * -------------------------------------------------------------------------- */

void Test_artifact_integration_full_lifecycle(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int claimed = FALSE, bound_on_equip = FALSE, bonuses_applied = FALSE;
  int bonuses_removed = FALSE, kept_owner_on_drop = FALSE, released = FALSE;
  int owner_survived_reload = FALSE, level_survived_reload = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  artint_instance(&fixture, &obj, ART_VNUM_TRORXEK);
  artint_carry(&fixture, &obj);

  /* acquire */
  artifact_obj_to_char(&obj, &fixture.actor);
  art = artifact_by_vnum(ART_VNUM_TRORXEK);
  claimed = (art && art->owner && !str_cmp(art->owner, "Artifactor") && art->instance_persisted &&
             art->discovered && art->claim_count == 1);

  /* equip - bind on equip, bonuses, first-equip XP */
  CuAssertIntEquals(tc, TRUE, artifact_on_equip(&fixture.actor, &obj, WEAR_WIELD_1));
  bound_on_equip = (art->bound_time > 0);
  bonuses_applied = (fixture.actor.affected != NULL);

  /* unequip */
  artifact_on_unequip(&fixture.actor, &obj);
  bonuses_removed = (fixture.actor.affected == NULL);

  /* drop: a bound artifact keeps its owner but loses persistence, so a zone
   * reset can recover it after a reboot */
  artint_uncarry(&fixture, &obj);
  artifact_obj_from_char(&obj);
  artifact_from_char(&obj, &fixture.actor);
  kept_owner_on_drop = (!str_cmp(art->owner, "Artifactor") && !art->instance_persisted);

  /* save and reload: the registry is rebuilt from disk */
  artifact_save();
  artifact_reload();
  art = artifact_by_vnum(ART_VNUM_TRORXEK);
  owner_survived_reload = (art && !str_cmp(art->owner, "Artifactor"));
  level_survived_reload = (art && art->level >= 1);

  /* destroy: the instance is gone, so ownership is released outright */
  artint_carry(&fixture, &obj);
  artifact_on_extract(&obj);
  art = artifact_by_vnum(ART_VNUM_TRORXEK);
  released = (art && !artifact_is_owned(ART_VNUM_TRORXEK) && art->bound_time == 0 &&
              !art->instance_persisted && art->destroy_count == 1);
  artint_uncarry(&fixture, &obj);

  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, claimed);
  CuAssertIntEquals(tc, TRUE, bound_on_equip);
  CuAssertIntEquals(tc, TRUE, bonuses_applied);
  CuAssertIntEquals(tc, TRUE, bonuses_removed);
  CuAssertIntEquals(tc, TRUE, kept_owner_on_drop);
  CuAssertIntEquals(tc, TRUE, owner_survived_reload);
  CuAssertIntEquals(tc, TRUE, level_survived_reload);
  CuAssertIntEquals(tc, TRUE, released);
}

/* --------------------------------------------------------------------------
 * Rejection paths
 * -------------------------------------------------------------------------- */

void Test_artifact_integration_character_binding_rejects_a_stranger(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int owner_may_use = FALSE, stranger_refused = FALSE, stranger_told_why = FALSE;
  int equip_refused = FALSE, staff_exempt = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  /* Trorxek binds on equip. */
  artint_instance(&fixture, &obj, ART_VNUM_TRORXEK);
  artint_carry(&fixture, &obj);
  artifact_obj_to_char(&obj, &fixture.actor);
  artifact_on_equip(&fixture.actor, &obj, WEAR_WIELD_1);
  artifact_on_unequip(&fixture.actor, &obj);

  art = artifact_by_vnum(ART_VNUM_TRORXEK);
  owner_may_use = artifact_can_use(&fixture.actor, &obj, TRUE);

  /* The bystander has no descriptor, so drive the message through the actor's
   * by moving the descriptor for the duration of the check. */
  fixture.actor.desc = NULL;
  fixture.bystander.desc = &fixture.descriptor;
  fixture.descriptor.character = &fixture.bystander;
  artint_clear_output(&fixture);

  stranger_refused = !artifact_can_use(&fixture.bystander, &obj, FALSE);
  stranger_told_why = artint_said(&fixture, "is bound to Artifactor");
  equip_refused = !artifact_on_equip(&fixture.bystander, &obj, WEAR_WIELD_1);

  /* Staff are never locked out of their own tools. */
  GET_LEVEL(&fixture.bystander) = LVL_IMMORT;
  staff_exempt = artifact_can_use(&fixture.bystander, &obj, TRUE);
  GET_LEVEL(&fixture.bystander) = 20;

  fixture.bystander.desc = NULL;
  fixture.descriptor.character = &fixture.actor;
  fixture.actor.desc = &fixture.descriptor;

  artint_uncarry(&fixture, &obj);
  artint_end(&fixture);

  CuAssertPtrNotNull(tc, art);
  CuAssertIntEquals(tc, TRUE, owner_may_use);
  CuAssertIntEquals(tc, TRUE, stranger_refused);
  CuAssertIntEquals(tc, TRUE, stranger_told_why);
  CuAssertIntEquals(tc, TRUE, equip_refused);
  CuAssertIntEquals(tc, TRUE, staff_exempt);
}

void Test_artifact_integration_account_binding_rejects_another_account(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int same_account_ok = FALSE, other_account_refused = FALSE, told_why = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  /* Tiamat's Stinger binds on account. */
  art = artifact_by_vnum(ART_VNUM_STINGER);
  CuAssertPtrNotNull(tc, art);
  CuAssertIntEquals(tc, ARTIFACT_BIND_ON_ACCOUNT, art->binding_type);

  GET_ACCOUNT_NAME(&fixture.actor) = strdup("firstaccount");
  GET_ACCOUNT_NAME(&fixture.bystander) = strdup("otheraccount");

  artint_instance(&fixture, &obj, ART_VNUM_STINGER);
  artint_carry(&fixture, &obj);
  artifact_obj_to_char(&obj, &fixture.actor);
  artifact_on_equip(&fixture.actor, &obj, WEAR_WIELD_1);
  artifact_on_unequip(&fixture.actor, &obj);

  same_account_ok = artifact_can_use(&fixture.actor, &obj, TRUE);

  fixture.actor.desc = NULL;
  fixture.bystander.desc = &fixture.descriptor;
  fixture.descriptor.character = &fixture.bystander;
  artint_clear_output(&fixture);

  other_account_refused = !artifact_can_use(&fixture.bystander, &obj, FALSE);
  told_why = artint_said(&fixture, "bound to another's account");

  fixture.bystander.desc = NULL;
  fixture.descriptor.character = &fixture.actor;
  fixture.actor.desc = &fixture.descriptor;

  artint_uncarry(&fixture, &obj);
  free(GET_ACCOUNT_NAME(&fixture.actor));
  free(GET_ACCOUNT_NAME(&fixture.bystander));
  GET_ACCOUNT_NAME(&fixture.actor) = NULL;
  GET_ACCOUNT_NAME(&fixture.bystander) = NULL;
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, same_account_ok);
  CuAssertIntEquals(tc, TRUE, other_account_refused);
  CuAssertIntEquals(tc, TRUE, told_why);
}

/* --------------------------------------------------------------------------
 * Level-scaled bonuses and highest-only resistance
 * -------------------------------------------------------------------------- */

static int artint_affect_modifier(struct char_data *ch, int location)
{
  struct affected_type *af = NULL;
  int total = 0;

  for (af = ch->affected; af; af = af->next)
    if (af->location == location)
      total += af->modifier;

  return total;
}

void Test_artifact_integration_bonuses_scale_with_artifact_level(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int hit_at_one = 0, hit_at_five = 0, hp_at_five = 0;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  /* Doombringer: +4 hitroll, +30 hp per artifact level. */
  art = artifact_by_vnum(ART_VNUM_DOOMBRINGER);
  CuAssertPtrNotNull(tc, art);

  artint_instance(&fixture, &obj, ART_VNUM_DOOMBRINGER);
  artint_carry(&fixture, &obj);

  art->level = 1;
  artifact_apply_bonuses(&fixture.actor, &obj);
  hit_at_one = artint_affect_modifier(&fixture.actor, APPLY_HITROLL);
  artifact_remove_bonuses(&fixture.actor, &obj);

  art->level = ARTIFACT_MAX_LEVEL;
  artifact_apply_bonuses(&fixture.actor, &obj);
  hit_at_five = artint_affect_modifier(&fixture.actor, APPLY_HITROLL);
  hp_at_five = artint_affect_modifier(&fixture.actor, APPLY_HIT);
  artifact_remove_bonuses(&fixture.actor, &obj);

  artint_uncarry(&fixture, &obj);
  artint_end(&fixture);

  CuAssertIntEquals(tc, 4, hit_at_one);
  CuAssertIntEquals(tc, 4 * ARTIFACT_MAX_LEVEL, hit_at_five);
  CuAssertIntEquals(tc, 30 * ARTIFACT_MAX_LEVEL, hp_at_five);
}

void Test_artifact_integration_resistance_takes_the_highest_not_the_sum(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data weapon;
  struct obj_data armor;
  struct artifact_data *fade = NULL;
  struct artifact_data *aegis = NULL;
  int one_artifact = 0, two_artifacts = 0;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  /* Fade resists 5% physical; the Aegis of Ages resists 12%.  Wearing both
   * must apply 12%, never 17%. */
  fade = artifact_by_vnum(ART_VNUM_FADE);
  aegis = artifact_by_vnum(ART_VNUM_AEGIS);
  CuAssertPtrNotNull(tc, fade);
  CuAssertPtrNotNull(tc, aegis);
  CuAssertIntEquals(tc, 5, fade->resist_physical);
  CuAssertIntEquals(tc, 12, aegis->resist_physical);

  artint_instance(&fixture, &weapon, ART_VNUM_FADE);
  artint_instance(&fixture, &armor, ART_VNUM_AEGIS);

  GET_EQ(&fixture.actor, WEAR_WIELD_1) = &weapon;
  weapon.worn_by = &fixture.actor;
  weapon.worn_on = WEAR_WIELD_1;

  one_artifact = artifact_damage_resist(&fixture.actor, 100, DAM_SLICE);

  GET_EQ(&fixture.actor, WEAR_BODY) = &armor;
  armor.worn_by = &fixture.actor;
  armor.worn_on = WEAR_BODY;

  two_artifacts = artifact_damage_resist(&fixture.actor, 100, DAM_SLICE);

  GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
  GET_EQ(&fixture.actor, WEAR_BODY) = NULL;
  artint_end(&fixture);

  CuAssertIntEquals(tc, 95, one_artifact);
  CuAssertIntEquals(tc, 88, two_artifacts);
}

void Test_artifact_integration_wyrmfang_unlocks_source_danger_sense_at_level_five(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int locked_at_four = FALSE, silent_at_four = FALSE;
  int active_at_five = FALSE, sensed_danger = FALSE;
  int info_described = FALSE, removed_cleanly = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  art = artifact_by_vnum(ART_VNUM_WYRMFANG);
  CuAssertPtrNotNull(tc, art);
  artint_instance(&fixture, &obj, ART_VNUM_WYRMFANG);
  artint_carry(&fixture, &obj);

  fixture.bystander.next_in_room = NULL;
  fixture.rooms[1].people = &fixture.victim;
  IN_ROOM(&fixture.victim) = 1;
  SET_BIT_AR(MOB_FLAGS(&fixture.victim), MOB_AGGRESSIVE);

  art->level = 4;
  artifact_apply_passives(&fixture.actor, art);
  locked_at_four = !AFF_FLAGGED(&fixture.actor, AFF_DANGERSENSE);
  artint_clear_output(&fixture);
  check_dangersense(&fixture.actor, 1);
  silent_at_four = !artint_said(&fixture, "You feel danger there");
  artifact_remove_passives(&fixture.actor, art);

  art->level = ARTIFACT_MAX_LEVEL;
  artifact_apply_passives(&fixture.actor, art);
  active_at_five = !!AFF_FLAGGED(&fixture.actor, AFF_DANGERSENSE);
  artint_clear_output(&fixture);
  check_dangersense(&fixture.actor, 1);
  sensed_danger = artint_said(&fixture, "You feel danger there");

  artint_clear_output(&fixture);
  artifact_show_info_for_test(&fixture.actor, &obj);
  info_described = artint_said(&fixture, "[active]") &&
                   artint_said(&fixture, "feels danger beyond the next door");

  artifact_remove_passives(&fixture.actor, art);
  removed_cleanly = !AFF_FLAGGED(&fixture.actor, AFF_DANGERSENSE);

  REMOVE_BIT_AR(MOB_FLAGS(&fixture.victim), MOB_AGGRESSIVE);
  fixture.rooms[1].people = NULL;
  IN_ROOM(&fixture.victim) = 0;
  fixture.bystander.next_in_room = &fixture.victim;
  artint_uncarry(&fixture, &obj);
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, locked_at_four);
  CuAssertIntEquals(tc, TRUE, silent_at_four);
  CuAssertIntEquals(tc, TRUE, active_at_five);
  CuAssertIntEquals(tc, TRUE, sensed_danger);
  CuAssertIntEquals(tc, TRUE, info_described);
  CuAssertIntEquals(tc, TRUE, removed_cleanly);
}

/* --------------------------------------------------------------------------
 * Active abilities
 * -------------------------------------------------------------------------- */

void Test_artifact_integration_every_active_ability_is_reachable(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int i = 0, abilities = 0, listed = 0;
  static const int ability_vnums[3] = {ART_VNUM_AMAUKEKEL, ART_VNUM_DOOMBRINGER, ART_VNUM_KELRARIN};

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  for (i = 0; i < total_artifacts; i++)
    if (art_index[i].ability_name)
      abilities++;

  /* 'artifact abilities' lists exactly what the bearer is actually holding. */
  for (i = 0; i < 3; i++)
  {
    art = artifact_by_vnum(ability_vnums[i]);
    CuAssertPtrNotNull(tc, art);
    CuAssertPtrNotNull(tc, (void *)art->ability_name);

    artint_instance(&fixture, &obj, ability_vnums[i]);
    artint_carry(&fixture, &obj);
    GET_EQ(&fixture.actor, WEAR_WIELD_1) = &obj;
    obj.worn_by = &fixture.actor;
    obj.worn_on = WEAR_WIELD_1;

    artint_clear_output(&fixture);
    do_artifact(&fixture.actor, (char *)"abilities", 0, 0);
    if (artint_said(&fixture, art->ability_name))
      listed++;

    GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
    obj.worn_by = NULL;
    artint_uncarry(&fixture, &obj);
  }

  artint_end(&fixture);

  CuAssertIntEquals(tc, 3, abilities);
  CuAssertIntEquals(tc, 3, listed);
}

/* An ability is invoked by typing its own name, so the template's
 * ability_name has to exist in the interpreter's command table and route to
 * do_artifact_ability.  An ability added to the template table but never
 * registered would otherwise be silently unreachable. */
void Test_artifact_integration_every_ability_name_is_a_registered_command(CuTest *tc)
{
  struct artint_fixture fixture;
  char missing[MAX_STRING_LENGTH] = {'\0'};
  int i = 0, j = 0, abilities = 0, registered = 0, found = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  for (i = 0; i < total_artifacts; i++)
  {
    if (!art_index[i].ability_name)
      continue;

    abilities++;
    found = FALSE;

    for (j = 0; *cmd_info[j].command != '\n'; j++)
      if (!str_cmp(cmd_info[j].command, art_index[i].ability_name) &&
          cmd_info[j].command_pointer == do_artifact_ability)
      {
        found = TRUE;
        break;
      }

    if (found)
      registered++;
    else
      snprintf(missing, sizeof(missing), "ability '%s' on vnum %d is not a registered command",
               art_index[i].ability_name, art_index[i].vnum);
  }

  artint_end(&fixture);

  CuAssertTrue(tc, abilities > 0);
  if (registered != abilities)
  {
    CuFail(tc, missing);
    return;
  }
  CuAssertIntEquals(tc, abilities, registered);
}

/* --------------------------------------------------------------------------
 * Called effects: success, refusal, and independent recharge
 * -------------------------------------------------------------------------- */

void Test_artifact_integration_called_effect_refuses_while_recharging(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int first_ok = FALSE, second_refused = FALSE, told_remaining = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  /* Wyrmfang's 'invoke hunt' is self-targeted, so it cannot fail for want of
   * a victim. */
  artint_instance(&fixture, &obj, ART_VNUM_WYRMFANG);
  artint_carry(&fixture, &obj);
  artifact_obj_to_char(&obj, &fixture.actor);
  art = artifact_by_vnum(ART_VNUM_WYRMFANG);
  CuAssertPtrNotNull(tc, art);

  artint_clear_output(&fixture);
  first_ok = artifact_command_trigger(&fixture.actor, "hunt");

  artint_clear_output(&fixture);
  second_refused = !artifact_command_trigger(&fixture.actor, "hunt");
  told_remaining = artint_said(&fixture, "is spent");

  artint_uncarry(&fixture, &obj);
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, first_ok);
  CuAssertIntEquals(tc, TRUE, second_refused);
  CuAssertIntEquals(tc, TRUE, told_remaining);
}

void Test_artifact_integration_called_effects_share_the_declared_ward_group(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data wyrmfang, icedge;
  struct artifact_data *wyrmfang_art = NULL, *icedge_art = NULL;
  int hunt_ok = FALSE, ward_active = FALSE;
  int rime_refused = FALSE, refusal_explained = FALSE, rime_recharge_free = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  artint_instance(&fixture, &wyrmfang, ART_VNUM_WYRMFANG);
  artint_carry(&fixture, &wyrmfang);
  artifact_obj_to_char(&wyrmfang, &fixture.actor);
  wyrmfang_art = artifact_by_vnum(ART_VNUM_WYRMFANG);
  CuAssertPtrNotNull(tc, wyrmfang_art);

  artint_instance(&fixture, &icedge, ART_VNUM_ICEDGE);
  artint_carry(&fixture, &icedge);
  artifact_obj_to_char(&icedge, &fixture.actor);
  icedge_art = artifact_by_vnum(ART_VNUM_ICEDGE);
  CuAssertPtrNotNull(tc, icedge_art);

  artint_clear_output(&fixture);
  hunt_ok = artifact_command_trigger(&fixture.actor, "hunt");
  ward_active = artifact_stack_active(&fixture.actor, ART_STACK_WARD);

  artint_clear_output(&fixture);
  rime_refused = !artifact_whisper_trigger(&fixture.actor, "rime");
  refusal_explained = artint_said(&fixture, "already wearing one ward");
  rime_recharge_free = (icedge_art->effect_used[0] == 0);

  artifact_stack_clear(&fixture.actor, ART_STACK_WARD);
  artint_uncarry(&fixture, &icedge);
  artint_uncarry(&fixture, &wyrmfang);
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, hunt_ok);
  CuAssertIntEquals(tc, TRUE, ward_active);
  CuAssertIntEquals(tc, TRUE, rime_refused);
  CuAssertIntEquals(tc, TRUE, refusal_explained);
  CuAssertIntEquals(tc, TRUE, rime_recharge_free);
}

void Test_artifact_integration_effect_slots_recharge_independently(CuTest *tc)
{
  struct artint_fixture fixture;
  struct artifact_data *art = NULL;
  int slot0_spent = 0, slot1_ready = 0, slot2_ready = 0;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  /* Trorxek has four effect slots.  Spending one must not spend the rest. */
  art = artifact_by_vnum(ART_VNUM_TRORXEK);
  CuAssertPtrNotNull(tc, art);

  art->effect_used[0] = time(0);
  slot0_spent = artifact_recharge_remaining(art, 0);
  slot1_ready = artifact_recharge_remaining(art, 1);
  slot2_ready = artifact_recharge_remaining(art, 2);

  artint_end(&fixture);

  CuAssertTrue(tc, slot0_spent > 0);
  CuAssertIntEquals(tc, 0, slot1_ready);
  CuAssertIntEquals(tc, 0, slot2_ready);
}

void Test_artifact_integration_invocation_channels_do_not_cross(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data spear;
  struct obj_data dagger;
  int say_ignores_command_phrase = FALSE, whisper_ignores_command_phrase = FALSE;
  int say_ignores_whisper_phrase = FALSE, whisper_answers = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  artint_instance(&fixture, &spear, ART_VNUM_WYRMFANG);
  artint_carry(&fixture, &spear);
  artint_instance(&fixture, &dagger, ART_VNUM_ICEDGE);
  artint_carry(&fixture, &dagger);
  artifact_obj_to_char(&spear, &fixture.actor);
  artifact_obj_to_char(&dagger, &fixture.actor);

  say_ignores_command_phrase = !artifact_speech_trigger(&fixture.actor, "hunt");
  whisper_ignores_command_phrase = !artifact_whisper_trigger(&fixture.actor, "hunt");
  say_ignores_whisper_phrase = !artifact_speech_trigger(&fixture.actor, "rime");
  whisper_answers = artifact_whisper_trigger(&fixture.actor, "rime");

  artint_uncarry(&fixture, &dagger);
  artint_uncarry(&fixture, &spear);
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, say_ignores_command_phrase);
  CuAssertIntEquals(tc, TRUE, whisper_ignores_command_phrase);
  CuAssertIntEquals(tc, TRUE, say_ignores_whisper_phrase);
  CuAssertIntEquals(tc, TRUE, whisper_answers);
}

/* --------------------------------------------------------------------------
 * Procs: the generic library and every signature shape
 *
 * artifact_weapon_proc() rolls for its chance, so these force the roll by
 * setting the chance to certainty.  Every other gate the runtime applies -
 * the internal cooldown, the alignment rule, target legality - is left alone.
 * -------------------------------------------------------------------------- */

struct artint_proc_case
{
  int vnum;
  int sig_proc;
  const char *what;
};

static const struct artint_proc_case artint_signature_cases[] = {
    {ART_VNUM_EARTHCRIER, ART_SIG_KNOCKDOWN, "knockdown"},
    {ART_VNUM_VENGEANCE, ART_SIG_MERCY, "mercy"},
    {ART_VNUM_WYRMFANG, ART_SIG_WEIGHTED, "weighted"},
    {ART_VNUM_TWILIGHT, ART_SIG_SURGE, "surge"},
    {ART_VNUM_ICEDGE, ART_SIG_FLURRY, "flurry"},
    {ART_VNUM_STINGER, ART_SIG_LIFESTEAL, "lifesteal"}};

#define ARTINT_SIGNATURE_COUNT                                                                     \
  ((int)(sizeof(artint_signature_cases) / sizeof(artint_signature_cases[0])))

void Test_artifact_integration_every_signature_shape_is_wired(CuTest *tc)
{
  struct artint_fixture fixture;
  struct artifact_data *art = NULL;
  int i = 0, matched = 0, all_have_a_chance = TRUE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  /* Each shape in the library is claimed by exactly the artifact the roster
   * says owns it, and none of them is registered with a zero chance. */
  for (i = 0; i < ARTINT_SIGNATURE_COUNT; i++)
  {
    art = artifact_by_vnum(artint_signature_cases[i].vnum);
    if (!art)
      continue;
    if (art->sig_proc == artint_signature_cases[i].sig_proc)
      matched++;
    if (art->sig_chance <= 0)
      all_have_a_chance = FALSE;
  }

  artint_end(&fixture);

  CuAssertIntEquals(tc, ARTINT_SIGNATURE_COUNT, matched);
  CuAssertIntEquals(tc, TRUE, all_have_a_chance);
}

void Test_artifact_integration_dormant_ward_respects_noncritical_chance(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int rejected_free = FALSE, accepted = FALSE, critical_bypassed_roll = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  /* No live template claims ART_SIG_WARD.  Aegis is a neutral carrier for
   * testing the dormant reusable shape without changing artifact identity. */
  art = artifact_by_vnum(ART_VNUM_AEGIS);
  CuAssertPtrNotNull(tc, art);
  art->level = ARTIFACT_MAX_LEVEL - 1;
  art->sig_proc = ART_SIG_WARD;
  art->sig_chance = 40;
  art->sig_align = ART_ALIGN_ANY;
  art->experience = 0;
  art->last_proc = 0;

  artint_instance(&fixture, &obj, ART_VNUM_AEGIS);
  artint_carry(&fixture, &obj);

  artint_clear_output(&fixture);
  artifact_force_signature_roll_for_test(&fixture.actor, &fixture.victim, &obj, FALSE, 41);
  rejected_free =
      art->last_proc == 0 && art->experience == 0 && fixture.descriptor.output[0] == '\0';

  art->last_proc = 0;
  art->experience = 0;
  artint_clear_output(&fixture);
  artifact_force_signature_roll_for_test(&fixture.actor, &fixture.victim, &obj, FALSE, 40);
  accepted = art->last_proc > 0 && art->experience == ARTIFACT_XP_PROC_SIGNATURE &&
             artint_said(&fixture, "sweeps through whatever");

  art->last_proc = 0;
  art->experience = 0;
  artint_clear_output(&fixture);
  artifact_force_signature_roll_for_test(&fixture.actor, &fixture.victim, &obj, TRUE, 100);
  critical_bypassed_roll = art->last_proc > 0 && art->experience == ARTIFACT_XP_PROC_SIGNATURE &&
                           artifact_stack_active(&fixture.actor, ART_STACK_WARD) &&
                           artint_said(&fixture, "closes around you");

  artifact_stack_clear(&fixture.actor, ART_STACK_WARD);
  artint_uncarry(&fixture, &obj);
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, rejected_free);
  CuAssertIntEquals(tc, TRUE, accepted);
  CuAssertIntEquals(tc, TRUE, critical_bypassed_roll);
}

void Test_artifact_integration_signature_procs_run_without_a_roll(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int i = 0, fired = 0;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  for (i = 0; i < ARTINT_SIGNATURE_COUNT; i++)
  {
    art = artifact_by_vnum(artint_signature_cases[i].vnum);
    if (!art)
      continue;

    /* Certainty, no internal cooldown, and an alignment rule that cannot
     * refuse: what is left is the shape itself. */
    art->sig_chance = 100;
    art->last_proc = 0;
    art->sig_align = ART_ALIGN_ANY;
    art->level = ARTIFACT_MAX_LEVEL;

    artint_instance(&fixture, &obj, artint_signature_cases[i].vnum);
    artint_carry(&fixture, &obj);
    GET_EQ(&fixture.actor, WEAR_WIELD_1) = &obj;
    obj.worn_by = &fixture.actor;
    obj.worn_on = WEAR_WIELD_1;

    /* A wounded bearer, so the mercy shape takes its heal branch, and a live
     * victim in the same room for the shapes that need one.  Both sides are
     * already engaged: a proc fires mid-combat, and pre-engaging keeps
     * damage() from having to start a fight, which needs the event queue. */
    GET_HIT(&fixture.actor) = GET_MAX_HIT(&fixture.actor) / 2;
    GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);
    GET_POS(&fixture.victim) = POS_STANDING;
    FIGHTING(&fixture.actor) = &fixture.victim;
    FIGHTING(&fixture.victim) = &fixture.actor;
    artifact_stack_clear(&fixture.actor, ART_STACK_COMBAT_SURGE);

    artint_clear_output(&fixture);
    artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);

    /* Every shape either says something, moves the victim, heals the bearer,
     * or raises a stack group.  A shape that did nothing at all is broken. */
    if (fixture.descriptor.output[0] != '\0' || GET_POS(&fixture.victim) != POS_STANDING ||
        GET_HIT(&fixture.actor) != GET_MAX_HIT(&fixture.actor) / 2 ||
        GET_HIT(&fixture.victim) != GET_MAX_HIT(&fixture.victim) ||
        artifact_stack_active(&fixture.actor, ART_STACK_COMBAT_SURGE))
      fired++;

    GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
    obj.worn_by = NULL;
    artint_uncarry(&fixture, &obj);
    artifact_stack_clear(&fixture.actor, ART_STACK_COMBAT_SURGE);
    FIGHTING(&fixture.actor) = NULL;
    FIGHTING(&fixture.victim) = NULL;
    fixture.actor.last_attacker = NULL;
    fixture.victim.last_attacker = NULL;
    GET_POS(&fixture.victim) = POS_STANDING;
    while (fixture.victim.affected != NULL)
      affect_remove_no_total(&fixture.victim, fixture.victim.affected);
  }

  artint_end(&fixture);

  CuAssertIntEquals(tc, ARTINT_SIGNATURE_COUNT, fired);
}

void Test_artifact_integration_earthcrier_uses_declared_knockdown_dc(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int level_one_dc = 0, level_five_dc = 0, info_described = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  art = artifact_by_vnum(ART_VNUM_EARTHCRIER);
  CuAssertPtrNotNull(tc, art);
  CuAssertIntEquals(tc, ART_SIG_KNOCKDOWN, art->sig_proc);
  CuAssertIntEquals(tc, 14, ARTIFACT_KNOCKDOWN_DC);

  artint_instance(&fixture, &obj, ART_VNUM_EARTHCRIER);
  artint_carry(&fixture, &obj);
  GET_EQ(&fixture.actor, WEAR_WIELD_1) = &obj;
  obj.worn_by = &fixture.actor;
  obj.worn_on = WEAR_WIELD_1;

  art->sig_chance = 100;
  art->sig_align = ART_ALIGN_ANY;
  /* A zeroed player has NOSCHOOL as its specialty, which the general save
   * system treats as a situational +2 match.  Use a real, unrelated school so
   * this assertion observes Earthcrier's unmodified base challenge. */
  GET_SPECIALTY_SCHOOL(&fixture.actor) = ABJURATION;
  FIGHTING(&fixture.actor) = &fixture.victim;
  FIGHTING(&fixture.victim) = &fixture.actor;

  art->level = 1;
  art->last_proc = 0;
  GET_POS(&fixture.victim) = POS_STANDING;
  test_reset_savingthrow_observation();
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  level_one_dc = test_get_last_savingthrow_challenge();

  art->level = ARTIFACT_MAX_LEVEL;
  art->last_proc = 0;
  GET_POS(&fixture.victim) = POS_STANDING;
  test_reset_savingthrow_observation();
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  level_five_dc = test_get_last_savingthrow_challenge();

  artint_clear_output(&fixture);
  artifact_show_info_for_test(&fixture.actor, &obj);
  info_described = artint_said(&fixture, "base Reflex DC 14 + artifact level") &&
                   artint_said(&fixture, "19 at this level") &&
                   artint_said(&fixture, "free-moving") &&
                   artint_said(&fixture, "already-down foes");

  GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
  obj.worn_by = NULL;
  artint_uncarry(&fixture, &obj);
  FIGHTING(&fixture.actor) = NULL;
  FIGHTING(&fixture.victim) = NULL;
  fixture.actor.last_attacker = NULL;
  fixture.victim.last_attacker = NULL;
  artint_end(&fixture);

  CuAssertIntEquals(tc, 15, level_one_dc);
  CuAssertIntEquals(tc, 19, level_five_dc);
  CuAssertIntEquals(tc, TRUE, info_described);
}

void Test_artifact_integration_fade_siphons_only_living_npcs(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  time_t cooldown_before = 0;
  int original_race = 0;
  int level_one_damage = 0, level_one_healing = 0;
  int level_five_damage = 0, level_five_healing = 0;
  int cooldown_ignored = FALSE, healing_capped = FALSE, info_described = FALSE;
  int undead_refused = FALSE, dragon_refused = FALSE, player_refused = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  art = artifact_by_vnum(ART_VNUM_FADE);
  CuAssertPtrNotNull(tc, art);
  CuAssertIntEquals(tc, ART_SIG_NONE, art->sig_proc);
  CuAssertIntEquals(tc, 16, art->proc_chance);
  CuAssertIntEquals(tc, 16, ARTIFACT_FADE_DRAIN_ODDS);
  CuAssertIntEquals(tc, 200, ARTIFACT_FADE_DRAIN_MAX_DAMAGE);
  CuAssertIntEquals(tc, 25, ARTIFACT_FADE_DRAIN_HEAL_PERCENT);

  artint_instance(&fixture, &obj, ART_VNUM_FADE);
  artint_carry(&fixture, &obj);
  GET_EQ(&fixture.actor, WEAR_WIELD_1) = &obj;
  obj.worn_by = &fixture.actor;
  obj.worn_on = WEAR_WIELD_1;

  artint_clear_output(&fixture);
  artifact_show_info_for_test(&fixture.actor, &obj);
  info_described = artint_said(&fixture, "Combat:") && artint_said(&fixture, "16% chance") &&
                   artint_said(&fixture, "Signature:") && artint_said(&fixture, "1-in-16") &&
                   artint_said(&fixture, "40 x artifact level") && artint_said(&fixture, "25%");

  FIGHTING(&fixture.actor) = &fixture.victim;
  FIGHTING(&fixture.victim) = &fixture.actor;
  original_race = GET_REAL_RACE(&fixture.victim);
  cooldown_before = time(0);
  art->last_proc = cooldown_before;

  art->level = 1;
  GET_HIT(&fixture.actor) = 100;
  GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);
  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  level_one_damage = GET_MAX_HIT(&fixture.victim) - GET_HIT(&fixture.victim);
  level_one_healing = GET_HIT(&fixture.actor) - 100;
  cooldown_ignored = art->last_proc == cooldown_before;

  art->level = ARTIFACT_MAX_LEVEL;
  GET_HIT(&fixture.actor) = 100;
  GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);
  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  level_five_damage = GET_MAX_HIT(&fixture.victim) - GET_HIT(&fixture.victim);
  level_five_healing = GET_HIT(&fixture.actor) - 100;

  GET_HIT(&fixture.actor) = GET_MAX_HIT(&fixture.actor) - 1;
  GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  healing_capped = GET_HIT(&fixture.actor) == GET_MAX_HIT(&fixture.actor);

  GET_REAL_RACE(&fixture.victim) = RACE_TYPE_UNDEAD;
  GET_HIT(&fixture.actor) = 100;
  GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);
  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  undead_refused = GET_HIT(&fixture.actor) == 100 &&
                   GET_HIT(&fixture.victim) == GET_MAX_HIT(&fixture.victim) &&
                   fixture.descriptor.output[0] == '\0';

  GET_REAL_RACE(&fixture.victim) = RACE_TYPE_DRAGON;
  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  dragon_refused = GET_HIT(&fixture.actor) == 100 &&
                   GET_HIT(&fixture.victim) == GET_MAX_HIT(&fixture.victim) &&
                   fixture.descriptor.output[0] == '\0';

  GET_REAL_RACE(&fixture.victim) = original_race;
  GET_HIT(&fixture.bystander) = GET_MAX_HIT(&fixture.bystander);
  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.bystander, &obj, FALSE);
  player_refused = GET_HIT(&fixture.bystander) == GET_MAX_HIT(&fixture.bystander) &&
                   fixture.descriptor.output[0] == '\0';

  GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
  obj.worn_by = NULL;
  artint_uncarry(&fixture, &obj);
  FIGHTING(&fixture.actor) = NULL;
  FIGHTING(&fixture.victim) = NULL;
  fixture.actor.last_attacker = NULL;
  fixture.victim.last_attacker = NULL;
  artint_end(&fixture);

  CuAssertIntEquals(tc, 40, level_one_damage);
  CuAssertIntEquals(tc, 10, level_one_healing);
  CuAssertIntEquals(tc, 200, level_five_damage);
  CuAssertIntEquals(tc, 50, level_five_healing);
  CuAssertIntEquals(tc, TRUE, cooldown_ignored);
  CuAssertIntEquals(tc, TRUE, healing_capped);
  CuAssertIntEquals(tc, TRUE, undead_refused);
  CuAssertIntEquals(tc, TRUE, dragon_refused);
  CuAssertIntEquals(tc, TRUE, player_refused);
  CuAssertIntEquals(tc, TRUE, info_described);
}

void Test_artifact_integration_doombringer_scales_its_extra_attack_burst(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  time_t generic_stamp = 0, signature_stamp = 0;
  int level_one_attacks = 0, level_five_attacks = 0, alignment_after = 0;
  int cooldown_blocked = FALSE, generic_independent = FALSE, nested_procs_suppressed = FALSE;
  int player_refused = FALSE, info_described = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  art = artifact_by_vnum(ART_VNUM_DOOMBRINGER);
  CuAssertPtrNotNull(tc, art);
  CuAssertIntEquals(tc, ART_SIG_NONE, art->sig_proc);
  CuAssertIntEquals(tc, 20, art->proc_chance);
  CuAssertIntEquals(tc, 31, ARTIFACT_DOOMBRINGER_BURST_ODDS);
  CuAssertIntEquals(tc, 5, ARTIFACT_DOOMBRINGER_BURST_MAX_ATTACKS);
  CuAssertIntEquals(tc, SECS_PER_MUD_HOUR / 3, ARTIFACT_DOOMBRINGER_BURST_COOLDOWN);
  CuAssertIntEquals(tc, 1, ARTIFACT_DOOMBRINGER_ALIGNMENT_COST);

  artint_instance(&fixture, &obj, ART_VNUM_DOOMBRINGER);
  artint_carry(&fixture, &obj);
  GET_EQ(&fixture.actor, WEAR_WIELD_1) = &obj;
  obj.worn_by = &fixture.actor;
  obj.worn_on = WEAR_WIELD_1;

  artint_clear_output(&fixture);
  artifact_show_info_for_test(&fixture.actor, &obj);
  info_described = artint_said(&fixture, "Combat:") && artint_said(&fixture, "20% chance") &&
                   artint_said(&fixture, "Signature:") && artint_said(&fixture, "1-in-31") &&
                   artint_said(&fixture, "one extra main-hand attack per artifact level") &&
                   artint_said(&fixture, "Independent 25-second recharge") &&
                   artint_said(&fixture, "cost 1 alignment");

  FIGHTING(&fixture.actor) = &fixture.victim;
  FIGHTING(&fixture.victim) = &fixture.actor;
  GET_MAX_HIT(&fixture.victim) = 100000;
  GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);
  art->level = 1;
  art->last_proc = 0;
  art->last_signature_proc = 0;
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  level_one_attacks = artifact_doombringer_attacks_for_test();

  art->proc_chance = 100;
  art->last_proc = 0;
  artint_clear_output(&fixture);
  artifact_force_doombringer_nested_proc_for_test(&fixture.actor, &fixture.victim, &obj);
  nested_procs_suppressed = art->last_proc == 0 && fixture.descriptor.output[0] == '\0';

  GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);
  GET_ALIGNMENT(&fixture.actor) = 0;
  GET_ALIGNMENT(&fixture.victim) = 1000;
  art->level = ARTIFACT_MAX_LEVEL;
  art->proc_chance = 20;
  generic_stamp = time(0);
  art->last_proc = generic_stamp;
  art->last_signature_proc = 0;
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  level_five_attacks = artifact_doombringer_attacks_for_test();
  alignment_after = GET_ALIGNMENT(&fixture.actor);
  signature_stamp = art->last_signature_proc;
  generic_independent = art->last_proc == generic_stamp;

  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  cooldown_blocked = artifact_doombringer_attacks_for_test() == 0 &&
                     art->last_signature_proc == signature_stamp &&
                     art->last_proc == generic_stamp && fixture.descriptor.output[0] == '\0';

  art->last_signature_proc = 0;
  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.bystander, &obj, FALSE);
  player_refused = artifact_doombringer_attacks_for_test() == 0 && art->last_signature_proc == 0 &&
                   fixture.descriptor.output[0] == '\0';

  GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
  obj.worn_by = NULL;
  artint_uncarry(&fixture, &obj);
  FIGHTING(&fixture.actor) = NULL;
  FIGHTING(&fixture.victim) = NULL;
  fixture.actor.last_attacker = NULL;
  fixture.victim.last_attacker = NULL;
  artint_end(&fixture);

  CuAssertIntEquals(tc, 1, level_one_attacks);
  CuAssertIntEquals(tc, 5, level_five_attacks);
  CuAssertIntEquals(tc, -1, alignment_after);
  CuAssertIntEquals(tc, TRUE, cooldown_blocked);
  CuAssertIntEquals(tc, TRUE, generic_independent);
  CuAssertIntEquals(tc, TRUE, nested_procs_suppressed);
  CuAssertIntEquals(tc, TRUE, player_refused);
  CuAssertIntEquals(tc, TRUE, info_described);
}

void Test_artifact_integration_avernus_restores_its_full_bladesong_package(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  time_t generic_stamp = 0;
  int original_race = 0;
  int level_one_damage = 0, level_one_healing = 0;
  int level_five_damage = 0, level_five_healing = 0;
  int cooldown_ignored = FALSE, healing_capped = FALSE, info_described = FALSE;
  int undead_refused = FALSE, construct_refused = FALSE;
  int emergency_healed = FALSE, bladesong_healed = FALSE;
  int knockdown_recovered = FALSE, pin_respected = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  art = artifact_by_vnum(ART_VNUM_AVERNUS);
  CuAssertPtrNotNull(tc, art);
  CuAssertIntEquals(tc, ART_SIG_NONE, art->sig_proc);
  CuAssertIntEquals(tc, 15, art->proc_chance);
  CuAssertIntEquals(tc, 31, ARTIFACT_AVERNUS_DRAIN_ODDS);
  CuAssertIntEquals(tc, 250, ARTIFACT_AVERNUS_DRAIN_MAX_TRANSFER);
  CuAssertIntEquals(tc, 3, ARTIFACT_AVERNUS_DRAIN_DAMAGE_MULTIPLIER);
  CuAssertIntEquals(tc, 11, ARTIFACT_AVERNUS_BLADESONG_ODDS);
  CuAssertIntEquals(tc, 2, ARTIFACT_AVERNUS_BLADESONG_HEAL);

  artint_instance(&fixture, &obj, ART_VNUM_AVERNUS);
  artint_carry(&fixture, &obj);
  GET_EQ(&fixture.actor, WEAR_WIELD_1) = &obj;
  obj.worn_by = &fixture.actor;
  obj.worn_on = WEAR_WIELD_1;

  artint_clear_output(&fixture);
  artifact_show_info_for_test(&fixture.actor, &obj);
  info_described = artint_said(&fixture, "Combat:") && artint_said(&fixture, "15% chance") &&
                   artint_said(&fixture, "Signature:") && artint_said(&fixture, "1-in-31") &&
                   artint_said(&fixture, "50 x artifact level") &&
                   artint_said(&fixture, "triple damage") &&
                   artint_said(&fixture, "ordinary knockdown") &&
                   artint_said(&fixture, "30 + (2 x level)% chance");

  FIGHTING(&fixture.actor) = &fixture.victim;
  FIGHTING(&fixture.victim) = &fixture.actor;
  original_race = GET_REAL_RACE(&fixture.victim);
  GET_MAX_HIT(&fixture.victim) = 2000;
  generic_stamp = time(0);
  art->last_proc = generic_stamp;

  art->level = 1;
  GET_HIT(&fixture.actor) = 100;
  GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  level_one_damage = GET_MAX_HIT(&fixture.victim) - GET_HIT(&fixture.victim);
  level_one_healing = GET_HIT(&fixture.actor) - 100;
  cooldown_ignored = art->last_proc == generic_stamp;

  art->level = ARTIFACT_MAX_LEVEL;
  GET_HIT(&fixture.actor) = 100;
  GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  level_five_damage = GET_MAX_HIT(&fixture.victim) - GET_HIT(&fixture.victim);
  level_five_healing = GET_HIT(&fixture.actor) - 100;

  GET_HIT(&fixture.actor) = GET_MAX_HIT(&fixture.actor) - 1;
  GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  healing_capped = GET_HIT(&fixture.actor) == GET_MAX_HIT(&fixture.actor);

  GET_REAL_RACE(&fixture.victim) = RACE_TYPE_UNDEAD;
  GET_HIT(&fixture.actor) = 100;
  GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);
  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  undead_refused = GET_HIT(&fixture.actor) == 100 &&
                   GET_HIT(&fixture.victim) == GET_MAX_HIT(&fixture.victim) &&
                   fixture.descriptor.output[0] == '\0';

  GET_REAL_RACE(&fixture.victim) = RACE_TYPE_CONSTRUCT;
  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  construct_refused = GET_HIT(&fixture.actor) == 100 &&
                      GET_HIT(&fixture.victim) == GET_MAX_HIT(&fixture.victim) &&
                      fixture.descriptor.output[0] == '\0';
  GET_REAL_RACE(&fixture.victim) = original_race;

  art->level = 1;
  GET_HIT(&fixture.actor) = 50;
  GET_POS(&fixture.actor) = POS_FIGHTING;
  artint_clear_output(&fixture);
  artifact_force_avernus_survival_for_test(&fixture.actor, &fixture.victim, &obj, TRUE, FALSE);
  emergency_healed = GET_HIT(&fixture.actor) == GET_MAX_HIT(&fixture.actor) &&
                     artint_said(&fixture, "pours everything it has back into you");

  GET_HIT(&fixture.actor) = GET_MAX_HIT(&fixture.actor) - 20;
  artint_clear_output(&fixture);
  artifact_force_avernus_survival_for_test(&fixture.actor, &fixture.victim, &obj, FALSE, TRUE);
  bladesong_healed = GET_HIT(&fixture.actor) == GET_MAX_HIT(&fixture.actor) - 18 &&
                     artint_said(&fixture, "rhythm closes 2 hit points");

  GET_HIT(&fixture.actor) = GET_MAX_HIT(&fixture.actor);
  GET_POS(&fixture.actor) = POS_SITTING;
  artint_clear_output(&fixture);
  artifact_force_avernus_survival_for_test(&fixture.actor, &fixture.victim, &obj, FALSE, FALSE);
  knockdown_recovered = GET_POS(&fixture.actor) == POS_FIGHTING &&
                        artint_said(&fixture, "back into a fighting stance");

  GET_POS(&fixture.actor) = POS_SITTING;
  SET_BIT_AR(AFF_FLAGS(&fixture.actor), AFF_PINNED);
  artint_clear_output(&fixture);
  artifact_force_avernus_survival_for_test(&fixture.actor, &fixture.victim, &obj, FALSE, FALSE);
  pin_respected = GET_POS(&fixture.actor) == POS_SITTING && fixture.descriptor.output[0] == '\0';
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.actor), AFF_PINNED);

  GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
  obj.worn_by = NULL;
  artint_uncarry(&fixture, &obj);
  FIGHTING(&fixture.actor) = NULL;
  FIGHTING(&fixture.victim) = NULL;
  fixture.actor.last_attacker = NULL;
  fixture.victim.last_attacker = NULL;
  artint_end(&fixture);

  CuAssertIntEquals(tc, 150, level_one_damage);
  CuAssertIntEquals(tc, 50, level_one_healing);
  CuAssertIntEquals(tc, 750, level_five_damage);
  CuAssertIntEquals(tc, 250, level_five_healing);
  CuAssertIntEquals(tc, TRUE, cooldown_ignored);
  CuAssertIntEquals(tc, TRUE, healing_capped);
  CuAssertIntEquals(tc, TRUE, undead_refused);
  CuAssertIntEquals(tc, TRUE, construct_refused);
  CuAssertIntEquals(tc, TRUE, emergency_healed);
  CuAssertIntEquals(tc, TRUE, bladesong_healed);
  CuAssertIntEquals(tc, TRUE, knockdown_recovered);
  CuAssertIntEquals(tc, TRUE, pin_respected);
  CuAssertIntEquals(tc, TRUE, info_described);
}

void Test_artifact_integration_stinger_lifesteal_drains_and_heals_safely(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int actor_before = 0, victim_before = 0, damage_dealt = 0, healing = 0;
  int first_fired = FALSE, cooldown_stamped = FALSE, second_fired = FALSE;
  int generic_refused = FALSE, healing_capped = FALSE, signature_described = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  art = artifact_by_vnum(ART_VNUM_STINGER);
  CuAssertPtrNotNull(tc, art);
  CuAssertIntEquals(tc, ART_SIG_LIFESTEAL, art->sig_proc);
  CuAssertIntEquals(tc, 10, ARTIFACT_STINGER_LIFESTEAL_CHANCE);
  CuAssertIntEquals(tc, ARTIFACT_STINGER_LIFESTEAL_CHANCE, art->sig_chance);

  art->sig_chance = 100;
  art->last_proc = 0;
  art->level = ARTIFACT_MAX_LEVEL;

  artint_instance(&fixture, &obj, ART_VNUM_STINGER);
  artint_carry(&fixture, &obj);
  GET_EQ(&fixture.actor, WEAR_WIELD_1) = &obj;
  obj.worn_by = &fixture.actor;
  obj.worn_on = WEAR_WIELD_1;

  artint_clear_output(&fixture);
  artifact_show_info_for_test(&fixture.actor, &obj);
  signature_described = artint_said(&fixture, "drain a foe's vitality") &&
                        artint_said(&fixture, "Healing equals damage inflicted") &&
                        artint_said(&fixture, "10 x artifact level");

  FIGHTING(&fixture.actor) = &fixture.victim;
  FIGHTING(&fixture.victim) = &fixture.actor;
  GET_HIT(&fixture.actor) = 100;
  GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);
  actor_before = GET_HIT(&fixture.actor);
  victim_before = GET_HIT(&fixture.victim);

  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  damage_dealt = victim_before - GET_HIT(&fixture.victim);
  healing = GET_HIT(&fixture.actor) - actor_before;
  first_fired = damage_dealt > 0 && artint_said(&fixture, "Stolen vitality closes your wounds");
  cooldown_stamped = art->last_proc > 0;

  /* This source-style per-hit proc must roll again even though its first
   * drain stamped the generic proc cooldown. */
  actor_before = GET_HIT(&fixture.actor);
  victim_before = GET_HIT(&fixture.victim);
  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  second_fired = GET_HIT(&fixture.actor) > actor_before &&
                 GET_HIT(&fixture.victim) < victim_before &&
                 artint_said(&fixture, "Stolen vitality closes your wounds");

  /* A drain still refreshes last_proc so the generic table cannot also fire
   * on the same hit or during its own 30-second recharge. */
  art->sig_chance = 0;
  art->proc_chance = 100;
  actor_before = GET_HIT(&fixture.actor);
  victim_before = GET_HIT(&fixture.victim);
  artint_clear_output(&fixture);
  artifact_weapon_proc(&fixture.actor, &fixture.victim, &obj, 10, FALSE);
  generic_refused = GET_HIT(&fixture.actor) == actor_before &&
                    GET_HIT(&fixture.victim) == victim_before &&
                    fixture.descriptor.output[0] == '\0';

  /* Healing can consume only missing hit points, even when the drain deals
   * more damage than the wielder needs. */
  art->sig_chance = 100;
  GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);
  GET_HIT(&fixture.actor) = GET_MAX_HIT(&fixture.actor) - 1;
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  healing_capped = GET_HIT(&fixture.actor) == GET_MAX_HIT(&fixture.actor);

  GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
  obj.worn_by = NULL;
  artint_uncarry(&fixture, &obj);
  FIGHTING(&fixture.actor) = NULL;
  FIGHTING(&fixture.victim) = NULL;
  fixture.actor.last_attacker = NULL;
  fixture.victim.last_attacker = NULL;
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, first_fired);
  CuAssertTrue(tc, damage_dealt > 0);
  CuAssertIntEquals(tc, damage_dealt, healing);
  CuAssertIntEquals(tc, TRUE, cooldown_stamped);
  CuAssertIntEquals(tc, TRUE, second_fired);
  CuAssertIntEquals(tc, TRUE, generic_refused);
  CuAssertIntEquals(tc, TRUE, healing_capped);
  CuAssertIntEquals(tc, TRUE, signature_described);
}

void Test_artifact_integration_stinger_lifesteal_caps_its_dry_streak(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int i = 0, quiet_before_limit = TRUE, fired_at_limit = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  art = artifact_by_vnum(ART_VNUM_STINGER);
  CuAssertPtrNotNull(tc, art);
  CuAssertIntEquals(tc, 15, ARTIFACT_STINGER_LIFESTEAL_GUARANTEE);

  art->sig_chance = 0;
  art->proc_chance = 0;
  art->last_proc = time(0);
  art->level = ARTIFACT_MAX_LEVEL;

  artint_instance(&fixture, &obj, ART_VNUM_STINGER);
  artint_carry(&fixture, &obj);
  GET_EQ(&fixture.actor, WEAR_WIELD_1) = &obj;
  obj.worn_by = &fixture.actor;
  obj.worn_on = WEAR_WIELD_1;

  FIGHTING(&fixture.actor) = &fixture.victim;
  FIGHTING(&fixture.victim) = &fixture.actor;
  GET_HIT(&fixture.actor) = 100;
  GET_HIT(&fixture.victim) = GET_MAX_HIT(&fixture.victim);

  for (i = 1; i < ARTIFACT_STINGER_LIFESTEAL_GUARANTEE; i++)
  {
    artint_clear_output(&fixture);
    artifact_weapon_proc(&fixture.actor, &fixture.victim, &obj, 10, FALSE);
    if (fixture.descriptor.output[0] != '\0')
      quiet_before_limit = FALSE;
  }

  CuAssertIntEquals(tc, ARTIFACT_STINGER_LIFESTEAL_GUARANTEE - 1, art->sig_miss_streak);

  artint_clear_output(&fixture);
  artifact_weapon_proc(&fixture.actor, &fixture.victim, &obj, 10, FALSE);
  fired_at_limit = artint_said(&fixture, "Stolen vitality closes your wounds") &&
                   art->sig_miss_streak == 0 && GET_HIT(&fixture.actor) > 100;

  GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
  obj.worn_by = NULL;
  artint_uncarry(&fixture, &obj);
  FIGHTING(&fixture.actor) = NULL;
  FIGHTING(&fixture.victim) = NULL;
  fixture.actor.last_attacker = NULL;
  fixture.victim.last_attacker = NULL;
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, quiet_before_limit);
  CuAssertIntEquals(tc, TRUE, fired_at_limit);
}

void Test_artifact_integration_surge_will_not_stack_with_itself(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int first_raised_it = FALSE, second_refused = FALSE, cleared = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  art = artifact_by_vnum(ART_VNUM_TWILIGHT);
  CuAssertPtrNotNull(tc, art);
  art->sig_chance = 100;
  art->last_proc = 0;
  art->level = ARTIFACT_MAX_LEVEL;

  artint_instance(&fixture, &obj, ART_VNUM_TWILIGHT);
  artint_carry(&fixture, &obj);
  GET_EQ(&fixture.actor, WEAR_WIELD_1) = &obj;
  obj.worn_by = &fixture.actor;
  obj.worn_on = WEAR_WIELD_1;

  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  first_raised_it = artifact_stack_active(&fixture.actor, ART_STACK_COMBAT_SURGE);

  /* A second surge while one is running must not add a second set of
   * affects. */
  art->last_proc = 0;
  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  second_refused = (fixture.descriptor.output[0] == '\0');

  artifact_stack_clear(&fixture.actor, ART_STACK_COMBAT_SURGE);
  cleared = !artifact_stack_active(&fixture.actor, ART_STACK_COMBAT_SURGE);

  GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
  obj.worn_by = NULL;
  artint_uncarry(&fixture, &obj);
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, first_raised_it);
  CuAssertIntEquals(tc, TRUE, second_refused);
  CuAssertIntEquals(tc, TRUE, cleared);
}

void Test_artifact_integration_generic_proc_respects_its_cooldown(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int silent_on_cooldown = FALSE, no_proc_at_zero_chance = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  /* The Aegis of Ages has no signature shape and no generic proc chance, so
   * it is the clean case for the generic path's guards. */
  art = artifact_by_vnum(ART_VNUM_AEGIS);
  CuAssertPtrNotNull(tc, art);
  CuAssertIntEquals(tc, ART_SIG_NONE, art->sig_proc);
  CuAssertIntEquals(tc, 0, art->proc_chance);

  artint_instance(&fixture, &obj, ART_VNUM_AEGIS);
  artint_carry(&fixture, &obj);

  artint_clear_output(&fixture);
  artifact_weapon_proc(&fixture.actor, &fixture.victim, &obj, 10, FALSE);
  no_proc_at_zero_chance = (fixture.descriptor.output[0] == '\0');

  /* Give it a certain proc but leave the internal cooldown running: the
   * cooldown wins. */
  art->proc_chance = 100;
  art->last_proc = time(0);
  artint_clear_output(&fixture);
  artifact_weapon_proc(&fixture.actor, &fixture.victim, &obj, 10, FALSE);
  silent_on_cooldown = (fixture.descriptor.output[0] == '\0');

  artint_uncarry(&fixture, &obj);
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, no_proc_at_zero_chance);
  CuAssertIntEquals(tc, TRUE, silent_on_cooldown);
}

void Test_artifact_integration_generic_no_op_attempts_do_not_spend_cooldown(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int full_heal_free = FALSE, repeated_fear_free = FALSE, invalid_ultimate_free = FALSE;
  int successful_heal_stamps = FALSE, info_described = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  /* Aegis has no signature handler, so it is a clean carrier for exercising
   * each branch of the shared generic library. */
  art = artifact_by_vnum(ART_VNUM_AEGIS);
  CuAssertPtrNotNull(tc, art);
  art->level = ARTIFACT_MAX_LEVEL;
  art->proc_chance = 100;
  art->experience = 0;
  art->last_proc = 0;

  artint_instance(&fixture, &obj, ART_VNUM_AEGIS);
  artint_carry(&fixture, &obj);

  GET_HIT(&fixture.actor) = GET_MAX_HIT(&fixture.actor);
  artint_clear_output(&fixture);
  artifact_force_generic_proc_for_test(&fixture.actor, &fixture.victim, &obj, ARTIFACT_PROC_HEAL);
  full_heal_free =
      art->last_proc == 0 && art->experience == 0 && fixture.descriptor.output[0] == '\0';

  SET_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_FEAR);
  art->last_proc = 0;
  artint_clear_output(&fixture);
  artifact_force_generic_proc_for_test(&fixture.actor, &fixture.victim, &obj, ARTIFACT_PROC_FEAR);
  repeated_fear_free =
      art->last_proc == 0 && art->experience == 0 && fixture.descriptor.output[0] == '\0';
  REMOVE_BIT_AR(AFF_FLAGS(&fixture.victim), AFF_FEAR);

  GET_LEVEL(&fixture.victim) = GET_LEVEL(&fixture.actor) + 1;
  art->last_proc = 0;
  artint_clear_output(&fixture);
  artifact_force_generic_proc_for_test(&fixture.actor, &fixture.victim, &obj,
                                       ARTIFACT_PROC_ULTIMATE);
  invalid_ultimate_free =
      art->last_proc == 0 && art->experience == 0 && fixture.descriptor.output[0] == '\0';

  /* XP stops at level 5, so step back one level to prove that a successful
   * branch stamps the clock and awards its normal proc XP. */
  art->level = ARTIFACT_MAX_LEVEL - 1;
  GET_HIT(&fixture.actor) = GET_MAX_HIT(&fixture.actor) - 100;
  art->last_proc = 0;
  artint_clear_output(&fixture);
  artifact_force_generic_proc_for_test(&fixture.actor, &fixture.victim, &obj, ARTIFACT_PROC_HEAL);
  successful_heal_stamps = art->last_proc > 0 && art->experience == ARTIFACT_XP_PROC_HEAL &&
                           GET_HIT(&fixture.actor) > GET_MAX_HIT(&fixture.actor) - 100 &&
                           artint_said(&fixture, "healing your wounds");

  artint_clear_output(&fixture);
  artifact_show_info_for_test(&fixture.actor, &obj);
  info_described = artint_said(&fixture, "Attempts that cannot affect anything spend no cooldown");

  artint_uncarry(&fixture, &obj);
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, full_heal_free);
  CuAssertIntEquals(tc, TRUE, repeated_fear_free);
  CuAssertIntEquals(tc, TRUE, invalid_ultimate_free);
  CuAssertIntEquals(tc, TRUE, successful_heal_stamps);
  CuAssertIntEquals(tc, TRUE, info_described);
}

void Test_artifact_integration_lethal_signature_stops_outer_hit_pipeline(CuTest *tc)
{
  struct artint_fixture fixture;
  struct artint_lethal_result result;
  struct obj_data obj;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  if (!artint_run_lethal_outer_hook(&fixture, &obj, ART_VNUM_STINGER, FALSE, &result))
  {
    artint_end(&fixture);
    CuFail(tc, "could not run the lethal signature fixture");
    return;
  }

  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, result.hook_reported_death);
  CuAssertIntEquals(tc, TRUE, result.extraction_marked);
  CuAssertIntEquals(tc, TRUE, result.proc_message_seen);
  CuAssertIntEquals(tc, FALSE, result.downstream_vampiric_seen);
}

void Test_artifact_integration_lethal_generic_proc_stops_outer_hit_pipeline(CuTest *tc)
{
  struct artint_fixture fixture;
  struct artint_lethal_result result;
  struct obj_data obj;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  /* A level-1 generic table can select only the soul-damage branch. */
  if (!artint_run_lethal_outer_hook(&fixture, &obj, ART_VNUM_AEGIS, TRUE, &result))
  {
    artint_end(&fixture);
    CuFail(tc, "could not run the lethal generic-proc fixture");
    return;
  }

  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, result.hook_reported_death);
  CuAssertIntEquals(tc, TRUE, result.extraction_marked);
  CuAssertIntEquals(tc, TRUE, result.proc_message_seen);
  CuAssertIntEquals(tc, FALSE, result.downstream_vampiric_seen);
}

/* --------------------------------------------------------------------------
 * Class oath: burn damage and phrase hiding
 * -------------------------------------------------------------------------- */

void Test_artifact_integration_class_oath_burns_and_hides_phrases(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  struct char_data *actor = NULL;
  int hp_before = 0, hp_after = 0;
  int phrases_hidden = FALSE, phrases_shown = FALSE, refused_effect = FALSE;
  int oathkeeper_ok = FALSE, oathbreaker_not_ok = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  actor = &fixture.actor;

  /* Trorxek is sworn to druids. */
  art = artifact_by_vnum(ART_VNUM_TRORXEK);
  CuAssertPtrNotNull(tc, art);
  CuAssertIntEquals(tc, CLASS_DRUID, art->class_restrict);
  CuAssertTrue(tc, art->class_min_level > 0);

  artint_instance(&fixture, &obj, ART_VNUM_TRORXEK);
  artint_carry(&fixture, &obj);
  GET_EQ(&fixture.actor, WEAR_WIELD_1) = &obj;
  obj.worn_by = &fixture.actor;
  obj.worn_on = WEAR_WIELD_1;

  /* No druid levels: it does not recognize the bearer. */
  CLASS_LEVEL(actor, CLASS_DRUID) = 0;
  oathbreaker_not_ok = !artifact_class_ok(&fixture.actor, art);

  artint_clear_output(&fixture);
  artifact_show_info_for_test(&fixture.actor, &obj);
  phrases_hidden = artint_said(&fixture, "It keeps its own counsel") &&
                   !artint_said(&fixture, "forest path home");

  /* The runtime agrees with the display: the words do nothing. */
  artint_clear_output(&fixture);
  refused_effect = !artifact_speech_trigger(&fixture.actor, "forest path home") &&
                   artint_said(&fixture, "does not care what you want");

  /* The burn is real damage, once per tick. */
  hp_before = GET_HIT(&fixture.actor);
  artifact_burn_tick(&fixture.actor);
  hp_after = GET_HIT(&fixture.actor);

  /* Enough druid levels: the artifact answers. */
  CLASS_LEVEL(actor, CLASS_DRUID) = art->class_min_level;
  oathkeeper_ok = artifact_class_ok(&fixture.actor, art);

  artint_clear_output(&fixture);
  artifact_show_info_for_test(&fixture.actor, &obj);
  phrases_shown = artint_said(&fixture, "forest path home");

  GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
  obj.worn_by = NULL;
  artint_uncarry(&fixture, &obj);
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, oathbreaker_not_ok);
  CuAssertIntEquals(tc, TRUE, phrases_hidden);
  CuAssertIntEquals(tc, TRUE, refused_effect);
  CuAssertTrue(tc, hp_after < hp_before);
  CuAssertIntEquals(tc, TRUE, oathkeeper_ok);
  CuAssertIntEquals(tc, TRUE, phrases_shown);
}

/* --------------------------------------------------------------------------
 * Player and staff command output
 * -------------------------------------------------------------------------- */

void Test_artifact_integration_player_commands_produce_output(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  int help_ok = FALSE, list_empty_ok = FALSE, list_shows_held = FALSE;
  int roster_ok = FALSE, progress_ok = FALSE, info_ok = FALSE, usage_ok = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  artint_clear_output(&fixture);
  do_artifact(&fixture.actor, (char *)"help", 0, 0);
  help_ok = (fixture.descriptor.output[0] != '\0');

  artint_clear_output(&fixture);
  do_artifact(&fixture.actor, (char *)"list", 0, 0);
  list_empty_ok = (fixture.descriptor.output[0] != '\0');

  artint_instance(&fixture, &obj, ART_VNUM_AEGIS);
  artint_carry(&fixture, &obj);
  artifact_obj_to_char(&obj, &fixture.actor);

  artint_clear_output(&fixture);
  do_artifact(&fixture.actor, (char *)"list", 0, 0);
  list_shows_held = artint_said(&fixture, "a test artifact");

  artint_clear_output(&fixture);
  do_artifact(&fixture.actor, (char *)"roster", 0, 0);
  roster_ok = (fixture.descriptor.output[0] != '\0');

  artint_clear_output(&fixture);
  do_artifact(&fixture.actor, (char *)"progress", 0, 0);
  progress_ok = (fixture.descriptor.output[0] != '\0');

  artint_clear_output(&fixture);
  do_artifact(&fixture.actor, (char *)"info artifact", 0, 0);
  info_ok = (fixture.descriptor.output[0] != '\0');

  artint_clear_output(&fixture);
  do_artifact(&fixture.actor, (char *)"nonsense", 0, 0);
  usage_ok = artint_said(&fixture, "Usage: artifact");

  artint_uncarry(&fixture, &obj);
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, help_ok);
  CuAssertIntEquals(tc, TRUE, list_empty_ok);
  CuAssertIntEquals(tc, TRUE, list_shows_held);
  CuAssertIntEquals(tc, TRUE, roster_ok);
  CuAssertIntEquals(tc, TRUE, progress_ok);
  CuAssertIntEquals(tc, TRUE, info_ok);
  CuAssertIntEquals(tc, TRUE, usage_ok);
}

void Test_artifact_integration_chronicle_hides_an_undiscovered_name(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  int hidden_before = FALSE, named_after = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  art = artifact_by_vnum(ART_VNUM_DOOMBRINGER);
  CuAssertPtrNotNull(tc, art);
  CuAssertIntEquals(tc, FALSE, art->discovered);

  /* Undiscovered: the roster carries the lore but not the name. */
  artint_clear_output(&fixture);
  do_artifact(&fixture.actor, (char *)"roster", 0, 0);
  hidden_before = !artint_said(&fixture, "a test artifact");

  /* Claiming it is what makes it public. */
  artint_instance(&fixture, &obj, ART_VNUM_DOOMBRINGER);
  artint_carry(&fixture, &obj);
  artifact_obj_to_char(&obj, &fixture.actor);
  CuAssertIntEquals(tc, TRUE, art->discovered);

  artint_clear_output(&fixture);
  do_artifact(&fixture.actor, (char *)"roster", 0, 0);
  named_after = artint_said(&fixture, "a test artifact");

  artint_uncarry(&fixture, &obj);
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, hidden_before);
  CuAssertIntEquals(tc, TRUE, named_after);
}

void Test_artifact_integration_staff_commands_report_the_registry(CuTest *tc)
{
  struct artint_fixture fixture;
  int list_ok = FALSE, verify_ok = FALSE, usage_ok = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  GET_LEVEL(&fixture.actor) = LVL_IMPL;

  artint_clear_output(&fixture);
  do_testartifact(&fixture.actor, (char *)"list", 0, 0);
  list_ok = (fixture.descriptor.output[0] != '\0');

  artint_clear_output(&fixture);
  do_testartifact(&fixture.actor, (char *)"verify", 0, 0);
  verify_ok = (fixture.descriptor.output[0] != '\0');

  artint_clear_output(&fixture);
  do_testartifact(&fixture.actor, (char *)"", 0, 0);
  usage_ok = (fixture.descriptor.output[0] != '\0');

  GET_LEVEL(&fixture.actor) = 20;
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, list_ok);
  CuAssertIntEquals(tc, TRUE, verify_ok);
  CuAssertIntEquals(tc, TRUE, usage_ok);
}

void Test_artifact_integration_staff_spawn_refuses_a_durably_owned_artifact(CuTest *tc)
{
  struct artint_fixture fixture;
  struct artifact_data *art = NULL;
  int refused = FALSE, owner_untouched = FALSE, persistence_untouched = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  art = artifact_by_vnum(ART_VNUM_AEGIS);
  CuAssertPtrNotNull(tc, art);

  free(art->owner);
  art->owner = strdup("Someoneelse");
  art->bound_time = time(0);
  art->instance_persisted = TRUE;

  GET_LEVEL(&fixture.actor) = LVL_IMPL;
  artint_clear_output(&fixture);
  do_testartifact(&fixture.actor, (char *)"spawn 169911", 0, 0);
  refused = (fixture.descriptor.output[0] != '\0');
  owner_untouched = !str_cmp(art->owner, "Someoneelse");
  persistence_untouched = art->instance_persisted;
  GET_LEVEL(&fixture.actor) = 20;

  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, refused);
  CuAssertIntEquals(tc, TRUE, owner_untouched);
  CuAssertIntEquals(tc, TRUE, persistence_untouched);
}

/* --------------------------------------------------------------------------
 * Single-instance behavior across reboot and zone reset
 * -------------------------------------------------------------------------- */

void Test_artifact_integration_zone_reset_never_makes_a_second_instance(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  obj_rnum rnum = NOTHING;
  int blocked_while_live = FALSE, allowed_when_gone = FALSE;
  int blocked_while_owned_offline = FALSE, allowed_after_release = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  rnum = artint_rnum_of(ART_VNUM_AVERNUS);
  CuAssertTrue(tc, rnum != NOTHING);

  /* An instance in play blocks the reset outright. */
  artint_instance(&fixture, &obj, ART_VNUM_AVERNUS);
  artint_carry(&fixture, &obj);
  blocked_while_live = artifact_block_zone_load(rnum);

  /* Take it out of play, but leave it durably owned and persisted, as though
   * the owner logged off with it.  The reset must still be refused. */
  artifact_obj_to_char(&obj, &fixture.actor);
  artint_uncarry(&fixture, &obj);
  blocked_while_owned_offline = artifact_block_zone_load(rnum);

  /* Release it: now the world may put it back. */
  artifact_from_char(&obj, &fixture.actor);
  allowed_when_gone = !artifact_block_zone_load(rnum);

  /* And an unowned, uninstanced artifact was never blocked. */
  allowed_after_release = !artifact_block_zone_load(artint_rnum_of(ART_VNUM_GESEN));

  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, blocked_while_live);
  CuAssertIntEquals(tc, TRUE, blocked_while_owned_offline);
  CuAssertIntEquals(tc, TRUE, allowed_when_gone);
  CuAssertIntEquals(tc, TRUE, allowed_after_release);
}

void Test_artifact_integration_ownership_survives_a_reboot(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  obj_rnum rnum = NOTHING;
  int owner_survived = FALSE, binding_survived = FALSE, persistence_survived = FALSE;
  int level_survived = FALSE, blocked_after_reboot = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  rnum = artint_rnum_of(ART_VNUM_ICEDGE);
  artint_instance(&fixture, &obj, ART_VNUM_ICEDGE);
  artint_carry(&fixture, &obj);
  artifact_obj_to_char(&obj, &fixture.actor);
  artifact_on_equip(&fixture.actor, &obj, WEAR_WIELD_1);

  art = artifact_by_vnum(ART_VNUM_ICEDGE);
  art->level = 3;
  art->experience = artifact_xp_to_next(2);
  artifact_save();

  /* Reboot: the instance goes away with the process, the file does not. */
  artifact_on_unequip(&fixture.actor, &obj);
  artint_uncarry(&fixture, &obj);
  artifact_shutdown();
  artifact_boot();

  art = artifact_by_vnum(ART_VNUM_ICEDGE);
  owner_survived = (art && !str_cmp(art->owner, "Artifactor"));
  binding_survived = (art && art->bound_time > 0);
  persistence_survived = (art && art->instance_persisted);
  level_survived = (art && art->level == 3);
  blocked_after_reboot = artifact_block_zone_load(rnum);

  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, owner_survived);
  CuAssertIntEquals(tc, TRUE, binding_survived);
  CuAssertIntEquals(tc, TRUE, persistence_survived);
  CuAssertIntEquals(tc, TRUE, level_survived);
  CuAssertIntEquals(tc, TRUE, blocked_after_reboot);
}

/* --------------------------------------------------------------------------
 * Balance-pass regressions
 *
 * Each of these locks in a decision from the gameplay balance pass recorded
 * in docs/systems/ARTIFACT_SYSTEM.md.
 * -------------------------------------------------------------------------- */

void Test_artifact_integration_oath_burn_scales_with_the_bearer(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  struct char_data *actor = NULL;
  int small_burn = 0, large_burn = 0;
  int dice_ceiling = ARTIFACT_BURN_DICE * ARTIFACT_BURN_SIDES;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  actor = &fixture.actor;
  art = artifact_by_vnum(ART_VNUM_TRORXEK);
  CuAssertPtrNotNull(tc, art);

  artint_instance(&fixture, &obj, ART_VNUM_TRORXEK);
  artint_carry(&fixture, &obj);
  GET_EQ(actor, WEAR_WIELD_1) = &obj;
  obj.worn_by = actor;
  obj.worn_on = WEAR_WIELD_1;
  CLASS_LEVEL(actor, CLASS_DRUID) = 0;

  /* A small bearer takes at least the historical dice. */
  GET_MAX_HIT(actor) = 200;
  GET_HIT(actor) = 200;
  artifact_burn_tick(actor);
  small_burn = 200 - GET_HIT(actor);

  /* A large one takes a share of what it actually has, which the flat 5d4
   * could never reach. */
  GET_MAX_HIT(actor) = 5000;
  GET_HIT(actor) = 5000;
  artifact_burn_tick(actor);
  large_burn = 5000 - GET_HIT(actor);

  GET_EQ(actor, WEAR_WIELD_1) = NULL;
  obj.worn_by = NULL;
  artint_uncarry(&fixture, &obj);
  artint_end(&fixture);

  CuAssertTrue(tc, small_burn >= ARTIFACT_BURN_DICE);
  CuAssertTrue(tc, large_burn > dice_ceiling);
}

void Test_artifact_integration_kelrom_healback_and_generic_use_independent_cooldowns(CuTest *tc)
{
  struct artint_fixture fixture;
  struct obj_data obj;
  struct artifact_data *art = NULL;
  time_t healback_stamp = 0;
  int exp_after_heal = 0, first_healed = 0, generic_damage = 0;
  int generic_reachable = FALSE, heal_awarded_xp = FALSE, info_described = FALSE;
  int no_heal_was_free = FALSE, second_healed = 0, second_was_free = FALSE;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  art = artifact_by_vnum(ART_VNUM_KELROM);
  CuAssertPtrNotNull(tc, art);
  art->level = 1;
  art->experience = 0;
  art->last_proc = 0;
  art->last_signature_proc = 0;

  artint_instance(&fixture, &obj, ART_VNUM_KELROM);
  artint_carry(&fixture, &obj);
  GET_EQ(&fixture.actor, WEAR_WIELD_1) = &obj;
  obj.worn_by = &fixture.actor;
  obj.worn_on = WEAR_WIELD_1;

  /* A no-heal attempt is not a successful proc and spends nothing. */
  GET_HIT(&fixture.actor) = GET_MAX_HIT(&fixture.actor);
  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  no_heal_was_free = art->last_signature_proc == 0 && art->last_proc == 0 && art->experience == 0 &&
                     fixture.descriptor.output[0] == '\0';

  GET_HIT(&fixture.actor) = 100;
  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  first_healed = GET_HIT(&fixture.actor) - 100;
  healback_stamp = art->last_signature_proc;
  exp_after_heal = art->experience;
  heal_awarded_xp = exp_after_heal == ARTIFACT_XP_PROC_HEAL && art->last_proc == 0 &&
                    artint_said(&fixture, "warm green light");

  GET_HIT(&fixture.actor) = 100;
  artint_clear_output(&fixture);
  artifact_force_signature_proc_for_test(&fixture.actor, &fixture.victim, &obj, FALSE);
  second_healed = GET_HIT(&fixture.actor) - 100;
  second_was_free = art->last_signature_proc == healback_stamp &&
                    art->experience == exp_after_heal && fixture.descriptor.output[0] == '\0';

  /* The active healback recharge must not shadow Kelrom's generic clock.
   * Level 1 makes the certain generic roll select soul damage deterministically. */
  art->proc_chance = 100;
  art->last_proc = 0;
  FIGHTING(&fixture.actor) = &fixture.victim;
  FIGHTING(&fixture.victim) = &fixture.actor;
  generic_damage = GET_HIT(&fixture.victim);
  artint_clear_output(&fixture);
  artifact_weapon_proc(&fixture.actor, &fixture.victim, &obj, 10, FALSE);
  generic_damage -= GET_HIT(&fixture.victim);
  generic_reachable = generic_damage > 0 && art->last_proc > 0 &&
                      art->last_signature_proc == healback_stamp &&
                      artint_said(&fixture, "tears at");

  art->proc_chance = 14;
  artint_clear_output(&fixture);
  artifact_show_info_for_test(&fixture.actor, &obj);
  info_described = artint_said(&fixture, "14% chance per hit") &&
                   artint_said(&fixture, "restore 10% of their damage") &&
                   artint_said(&fixture, "Independent 30-second recharge") &&
                   artint_said(&fixture, "no recharge is spent");

  GET_EQ(&fixture.actor, WEAR_WIELD_1) = NULL;
  obj.worn_by = NULL;
  artint_uncarry(&fixture, &obj);
  FIGHTING(&fixture.actor) = NULL;
  FIGHTING(&fixture.victim) = NULL;
  fixture.actor.last_attacker = NULL;
  fixture.victim.last_attacker = NULL;
  artint_end(&fixture);

  CuAssertIntEquals(tc, TRUE, no_heal_was_free);
  CuAssertIntEquals(tc, 1, first_healed);
  CuAssertIntEquals(tc, TRUE, heal_awarded_xp);
  CuAssertIntEquals(tc, 0, second_healed);
  CuAssertIntEquals(tc, TRUE, second_was_free);
  CuAssertIntEquals(tc, TRUE, generic_reachable);
  CuAssertIntEquals(tc, TRUE, info_described);
}

void Test_artifact_integration_multi_target_powers_are_capped(CuTest *tc)
{
  struct artint_fixture fixture;
  int annihilation_cap_at_one = 0, annihilation_cap_at_five = 0;

  if (!artint_begin(&fixture))
  {
    artint_end(&fixture);
    CuFail(tc, "could not boot the artifact integration fixture");
    return;
  }

  /* Both multi-target powers are bounded, and annihilation's bound grows with
   * the artifact rather than with how crowded the room happens to be. */
  annihilation_cap_at_one =
      ARTIFACT_ANNIHILATION_TARGETS_BASE + (ARTIFACT_ANNIHILATION_TARGETS_PER_LEVEL * 1);
  annihilation_cap_at_five = ARTIFACT_ANNIHILATION_TARGETS_BASE +
                             (ARTIFACT_ANNIHILATION_TARGETS_PER_LEVEL * ARTIFACT_MAX_LEVEL);

  artint_end(&fixture);

  CuAssertTrue(tc, annihilation_cap_at_one >= 1);
  CuAssertTrue(tc, annihilation_cap_at_five > annihilation_cap_at_one);
  CuAssertTrue(tc, ARTIFACT_DOOMBLAST_MAX_TARGETS > 0);
}
