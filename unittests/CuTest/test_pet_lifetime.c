#include "CuTest.h"

#include "conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/handler.h"
#include "../../src/magic/spells.h"
#include "../../src/mud_event.h"
#include "../../src/dgscript/dg_event.h"
#include "../../src/event_runtime.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

extern char *serialize_pet_runtime_state_for_test(struct char_data *pet);
extern bool restore_pet_runtime_state_for_test(struct char_data *pet, const char *serialized);

static void begin_lifetime_pet(struct char_data *pet)
{
  if (!event_runtime_is_initialized())
    event_init();
  clear_char(pet);
  SET_BIT_AR(MOB_FLAGS(pet), MOB_ISNPC);
  SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);
}

static void end_lifetime_pet(struct char_data *pet)
{
  struct affected_type *affect;

  clear_char_event_list(pet);
  while (pet->affected)
  {
    affect = pet->affected;
    pet->affected = affect->next;
    free(affect);
  }
}

/* Returns a copy of serialized with its T line replaced (or removed when
 * replacement is NULL).  Returns NULL when there is no T line. */
static char *with_lifetime_line(const char *serialized, const char *replacement)
{
  const char *line;
  const char *next;
  size_t prefix;
  size_t size;
  char *copy;

  line = strstr(serialized, "\nT ");
  if (line == NULL)
    return NULL;
  line++;
  next = strchr(line, '\n');
  if (next == NULL)
    return NULL;
  next++;
  prefix = (size_t)(line - serialized);
  size = strlen(serialized) + (replacement ? strlen(replacement) + 2 : 1);
  copy = malloc(size);
  if (copy == NULL)
    return NULL;
  memcpy(copy, serialized, prefix);
  copy[prefix] = '\0';
  if (replacement)
  {
    strlcat(copy, replacement, size);
    strlcat(copy, "\n", size);
  }
  strlcat(copy, next, size);
  return copy;
}

static long long saved_deadline(const char *serialized)
{
  const char *line;
  int kind;
  long long deadline;

  line = strstr(serialized, "\nT ");
  if (line == NULL || sscanf(line, "\nT %d %lld", &kind, &deadline) != 2 || kind != 1)
    return -1;
  return deadline;
}

void Test_pet_lifetime_deadline_survives_save_and_restores_a_fresh_event(CuTest *tc)
{
  struct char_data source;
  struct char_data restored;
  struct mud_event_data *event;
  char *first;
  char *second;
  long long now;
  long long first_deadline;
  long long second_deadline;
  long remaining;
  bool restored_ok;
  enum pet_lifetime_kind source_kind;

  begin_lifetime_pet(&source);
  begin_lifetime_pet(&restored);
  attach_mud_event(new_mud_event(ePURGEMOB, &source, NULL), 90 * PASSES_PER_SEC);
  now = (long long)time(NULL);

  first = serialize_pet_runtime_state_for_test(&source);
  second = serialize_pet_runtime_state_for_test(&source);
  if (first == NULL || second == NULL)
  {
    free(first);
    free(second);
    end_lifetime_pet(&source);
    CuFail(tc, "could not serialize the timed follower");
    return;
  }
  first_deadline = saved_deadline(first);
  second_deadline = saved_deadline(second);
  source_kind = pet_lifetime_kind(&source);

  restored_ok = restore_pet_runtime_state_for_test(&restored, first);
  event = char_has_mud_event(&restored, ePURGEMOB);
  remaining = mud_event_is_live(event) ? mud_event_remaining(event) : -1;

  free(first);
  free(second);
  end_lifetime_pet(&source);
  end_lifetime_pet(&restored);

  CuAssertIntEquals(tc, PET_LIFETIME_DEADLINE, source_kind);
  /* The saved deadline is absolute real time, so repeated saves agree. */
  CuAssertTrue(tc, first_deadline >= now + 89 && first_deadline <= now + 92);
  CuAssertTrue(tc, second_deadline >= first_deadline && second_deadline <= first_deadline + 1);
  CuAssertTrue(tc, restored_ok);
  CuAssertTrue(tc, remaining >= 87L * PASSES_PER_SEC && remaining <= 92L * PASSES_PER_SEC);
}

void Test_pet_lifetime_rejects_expired_and_malformed_records(CuTest *tc)
{
  struct char_data source;
  struct char_data restored;
  char *durable;
  char *record;
  char expired_line[64];
  const char *malformed[] = {"T 2 5", "T 0 5", "T 1 0", "T 1 -5", "T 1 x", "T 1", "T 0 0 7"};
  bool expired_rejected;
  bool malformed_rejected;
  bool missing_rejected;
  bool legacy_line_rejected;
  bool legacy_durable;
  bool no_event_after_rejection;
  size_t i;

  begin_lifetime_pet(&source);
  begin_lifetime_pet(&restored);
  durable = serialize_pet_runtime_state_for_test(&source);
  if (durable == NULL || strstr(durable, "\nT 0 0\n") == NULL)
  {
    free(durable);
    end_lifetime_pet(&source);
    CuFail(tc, "durable follower did not serialize a durable lifetime record");
    return;
  }

  snprintf(expired_line, sizeof(expired_line), "T 1 %lld", (long long)time(NULL) - 5);
  record = with_lifetime_line(durable, expired_line);
  expired_rejected = record != NULL && !restore_pet_runtime_state_for_test(&restored, record);
  no_event_after_rejection = char_has_mud_event(&restored, ePURGEMOB) == NULL;
  free(record);

  malformed_rejected = true;
  for (i = 0; i < sizeof(malformed) / sizeof(malformed[0]); i++)
  {
    record = with_lifetime_line(durable, malformed[i]);
    malformed_rejected = malformed_rejected && record != NULL &&
                         !restore_pet_runtime_state_for_test(&restored, record);
    free(record);
  }

  /* A version 4 record must carry its lifetime line. */
  record = with_lifetime_line(durable, NULL);
  missing_rejected = record != NULL && !restore_pet_runtime_state_for_test(&restored, record);
  /* Older versions never carried one; a stray line is malformed there. */
  if (record != NULL)
    record[2] = '3';
  legacy_durable = record != NULL && restore_pet_runtime_state_for_test(&restored, record) &&
                   char_has_mud_event(&restored, ePURGEMOB) == NULL &&
                   pet_lifetime_kind(&restored) == PET_LIFETIME_DURABLE;
  free(record);
  durable[2] = '3';
  legacy_line_rejected = !restore_pet_runtime_state_for_test(&restored, durable);

  free(durable);
  end_lifetime_pet(&source);
  end_lifetime_pet(&restored);

  CuAssertTrue(tc, expired_rejected);
  CuAssertTrue(tc, no_event_after_rejection);
  CuAssertTrue(tc, malformed_rejected);
  CuAssertTrue(tc, missing_rejected);
  CuAssertTrue(tc, legacy_durable);
  CuAssertTrue(tc, legacy_line_rejected);
}

void Test_pet_lifetime_decoys_never_persist_without_a_deadline(CuTest *tc)
{
  struct char_data decoy;
  struct char_data restored;
  char *record;
  char *legacy;
  long long now;
  long long deadline;
  enum pet_lifetime_kind decoy_kind;
  bool spent_rejected;
  bool legacy_rejected;

  begin_lifetime_pet(&decoy);
  begin_lifetime_pet(&restored);
  decoy.pet_source_spell = SPELL_MISLEAD;
  restored.pet_source_spell = SPELL_MISLEAD;
  now = (long long)time(NULL);
  record = serialize_pet_runtime_state_for_test(&decoy);
  if (record == NULL)
  {
    end_lifetime_pet(&decoy);
    CuFail(tc, "could not serialize the decoy");
    return;
  }
  deadline = saved_deadline(record);
  decoy_kind = pet_lifetime_kind(&decoy);
  spent_rejected = !restore_pet_runtime_state_for_test(&restored, record);
  /* A pre-lifetime record for a decoy is expired, not promoted to durable. */
  legacy = with_lifetime_line(record, NULL);
  if (legacy != NULL)
    legacy[2] = '3';
  legacy_rejected = legacy != NULL && !restore_pet_runtime_state_for_test(&restored, legacy) &&
                    char_has_mud_event(&restored, ePURGEMOB) == NULL;
  free(legacy);
  free(record);
  end_lifetime_pet(&decoy);
  end_lifetime_pet(&restored);

  CuAssertIntEquals(tc, PET_LIFETIME_DEADLINE, decoy_kind);
  /* A decoy with no live event is saved already spent, never as durable. */
  CuAssertTrue(tc, deadline >= now && deadline <= now + 1);
  CuAssertTrue(tc, spent_rejected);
  CuAssertTrue(tc, legacy_rejected);
}

void Test_pet_lifetime_status_reports_each_policy(CuTest *tc)
{
  struct char_data pet;
  struct affected_type control;
  char status[64];
  bool durable;
  bool timed_control;
  bool deadline;

  begin_lifetime_pet(&pet);
  pet_lifetime_status(&pet, status, sizeof(status));
  durable = pet_lifetime_kind(&pet) == PET_LIFETIME_DURABLE && !strcmp(status, "durable");

  new_affect(&control);
  control.spell = SPELL_CHARM_MONSTER;
  control.duration = 9;
  SET_BIT_AR(control.bitvector, AFF_CHARM);
  affect_to_char(&pet, &control);
  pet_lifetime_status(&pet, status, sizeof(status));
  timed_control =
      pet_lifetime_kind(&pet) == PET_LIFETIME_CONTROL && strstr(status, "timed control") != NULL;

  attach_mud_event(new_mud_event(ePURGEMOB, &pet, NULL), 125 * PASSES_PER_SEC);
  pet_lifetime_status(&pet, status, sizeof(status));
  deadline = pet_lifetime_kind(&pet) == PET_LIFETIME_DEADLINE &&
             strstr(status, "expires in 2m") != NULL && strstr(status, "real time") != NULL;

  end_lifetime_pet(&pet);

  CuAssertTrue(tc, durable);
  CuAssertTrue(tc, timed_control);
  CuAssertTrue(tc, deadline);
}

/* Ordinary spell summons outside the kept families last only for the session:
 * they are not saved, the keeper refuses them, and a saved record of one is
 * rejected on restore. */
void Test_pet_lifetime_session_summons_are_never_saved(CuTest *tc)
{
  struct char_data summon;
  struct char_data restored;
  struct affected_type control;
  char status[64];
  char *record;
  bool session;
  bool kept_family;
  bool timed_control_kept;
  bool deadline_not_boarded;
  bool record_rejected;
  bool legacy_rejected;

  begin_lifetime_pet(&summon);
  begin_lifetime_pet(&restored);
  summon.pet_source_spell = SPELL_SUMMON_CREATURE_1;
  pet_lifetime_status(&summon, status, sizeof(status));
  session = pet_lifetime_kind(&summon) == PET_LIFETIME_SESSION && !pet_lifetime_persists(&summon) &&
            !pet_keeper_accepts(&summon) && strstr(status, "not saved") != NULL;

  record = serialize_pet_runtime_state_for_test(&summon);
  if (record == NULL)
  {
    end_lifetime_pet(&summon);
    end_lifetime_pet(&restored);
    CuFail(tc, "could not serialize the session summon");
    return;
  }
  record_rejected = !restore_pet_runtime_state_for_test(&restored, record);
  record[2] = '3';
  legacy_rejected = !restore_pet_runtime_state_for_test(&restored, record);
  free(record);

  /* Permanent creations keep even though a spell made them. */
  SET_BIT_AR(MOB_FLAGS(&summon), MOB_ANIMATED_DEAD);
  kept_family = pet_lifetime_kind(&summon) == PET_LIFETIME_DURABLE &&
                pet_lifetime_persists(&summon) && pet_keeper_accepts(&summon);
  REMOVE_BIT_AR(MOB_FLAGS(&summon), MOB_ANIMATED_DEAD);

  new_affect(&control);
  control.spell = SPELL_CHARM_MONSTER;
  control.duration = 9;
  SET_BIT_AR(control.bitvector, AFF_CHARM);
  affect_to_char(&summon, &control);
  timed_control_kept =
      pet_lifetime_kind(&summon) == PET_LIFETIME_CONTROL && pet_keeper_accepts(&summon);

  attach_mud_event(new_mud_event(ePURGEMOB, &summon, NULL), 60 * PASSES_PER_SEC);
  deadline_not_boarded = pet_lifetime_kind(&summon) == PET_LIFETIME_DEADLINE &&
                         pet_lifetime_persists(&summon) && !pet_keeper_accepts(&summon);

  end_lifetime_pet(&summon);
  end_lifetime_pet(&restored);

  CuAssertTrue(tc, session);
  CuAssertTrue(tc, record_rejected);
  CuAssertTrue(tc, legacy_rejected);
  CuAssertTrue(tc, kept_family);
  CuAssertTrue(tc, timed_control_kept);
  CuAssertTrue(tc, deadline_not_boarded);
}
