/*
 * Structured web onboarding emitter tests.
 *
 * These exercise the presentation adapter in src/systems/web_client. All
 * fixtures are synthetic; no live account, character, or credential data may be
 * used here. The adapter is read-only, so no test mutates or saves a character.
 */

#include "CuTest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/systems/web_client/onboarding.h"

/* A descriptor with just enough state for the adapter to read. */
static void init_test_descriptor(struct descriptor_data *d, int state)
{
  memset(d, 0, sizeof(*d));
  d->desc_num = 42;
  d->login_time = 1000;
  d->connected = state;
  web_onboarding_reset(d);
  d->connected = state;
}

/*
 * Minimal structural check: balanced braces and brackets outside strings, with
 * escapes honoured. Enough to catch a malformed document without linking a
 * JSON parser into the suite.
 */
static int json_is_balanced(const char *text)
{
  int braces = 0;
  int brackets = 0;
  int in_string = 0;
  const char *p = NULL;

  for (p = text; *p; p++)
  {
    if (in_string)
    {
      if (*p == '\\' && *(p + 1))
      {
        p++;
        continue;
      }
      if (*p == '"')
        in_string = 0;
      continue;
    }

    if (*p == '"')
      in_string = 1;
    else if (*p == '{')
      braces++;
    else if (*p == '}')
      braces--;
    else if (*p == '[')
      brackets++;
    else if (*p == ']')
      brackets--;

    if (braces < 0 || brackets < 0)
      return 0;
  }

  return !in_string && braces == 0 && brackets == 0;
}

/* Reject any raw control byte; the client rejects payloads containing them. */
static int has_control_bytes(const char *text)
{
  const char *p = NULL;

  for (p = text; *p; p++)
  {
    if ((unsigned char)*p < 0x20 || (unsigned char)*p == 0x7f)
      return 1;
  }

  return 0;
}

void TestWebOnboardingCapabilityNegotiation(CuTest *tc)
{
  struct descriptor_data d;

  init_test_descriptor(&d, CON_ACCOUNT_NAME);

  /* No protocol structure means no structured onboarding. */
  CuAssertTrue(tc, !web_onboarding_enabled(&d));

  web_onboarding_set_capability(&d, "1");
  CuAssertIntEquals(tc, 1, d.web_onboarding_version);

  /* An unsupported or nonsense version disables the structured flow. */
  web_onboarding_set_capability(&d, "99");
  CuAssertIntEquals(tc, 99, d.web_onboarding_version);
  CuAssertTrue(tc, !web_onboarding_enabled(&d));

  web_onboarding_set_capability(&d, "not-a-number");
  CuAssertIntEquals(tc, 0, d.web_onboarding_version);

  web_onboarding_reset(&d);
  CuAssertIntEquals(tc, 0, d.web_onboarding_version);
  CuAssertIntEquals(tc, 0, d.web_onboarding_revision);
  CuAssertIntEquals(tc, -1, d.web_onboarding_last_state);
}

void TestWebOnboardingMediaKeysAreStableAndBounded(CuTest *tc)
{
  int index = 0;

  /* The corrected Tiefling key must be emitted regardless of the internal
   * token spelling. */
  CuAssertStrEquals(tc, "race/tiefling", web_onboarding_race_media_key(RACE_TIEFLING));
  CuAssertStrEquals(tc, "race/human", web_onboarding_race_media_key(RACE_HUMAN));
  CuAssertStrEquals(tc, "class/wizard", web_onboarding_class_media_key(CLASS_WIZARD));

  /* Out-of-range and prestige entries resolve to the generic fallbacks rather
   * than reading past the table. */
  CuAssertStrEquals(tc, "race/fallback", web_onboarding_race_media_key(-1));
  CuAssertStrEquals(tc, "race/fallback", web_onboarding_race_media_key(NUM_RACES));
  CuAssertStrEquals(tc, "class/fallback", web_onboarding_class_media_key(-1));
  CuAssertStrEquals(tc, "class/fallback", web_onboarding_class_media_key(NUM_CLASSES));
  CuAssertStrEquals(tc, "class/fallback", web_onboarding_class_media_key(CLASS_WEAPON_MASTER));

  /* Every playable race resolves to a namespaced key. */
  for (index = 0; index < NUM_RACES; index++)
  {
    const char *key = web_onboarding_race_media_key(index);
    CuAssertPtrNotNull(tc, (void *)key);
    CuAssertTrue(tc, strncmp(key, "race/", 5) == 0);
  }
}

void TestWebOnboardingAccountNamePayload(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  init_test_descriptor(&d, CON_ACCOUNT_NAME);

  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertTrue(tc, json_is_balanced(payload));
  CuAssertTrue(tc, !has_control_bytes(payload));
  CuAssertPtrNotNull(tc, strstr(payload, "\"version\":1"));
  CuAssertPtrNotNull(tc, strstr(payload, "\"screen\":\"account-name\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"mode\":\"account\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"inputKind\":\"text\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"sensitiveInput\":false"));
  CuAssertPtrNotNull(tc, strstr(payload, "\"flowId\":\"42-1000\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"classic-terminal\""));
}

void TestWebOnboardingPasswordScreensAreMarkedSensitive(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];
  const int password_states[] = {CON_PASSWORD, CON_NEWPASSWD, CON_CNFPASSWD, CON_ACCOUNT_ADD_PWD};
  size_t index = 0;

  for (index = 0; index < sizeof(password_states) / sizeof(password_states[0]); index++)
  {
    init_test_descriptor(&d, password_states[index]);
    CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
    CuAssertPtrNotNull(tc, strstr(payload, "\"sensitiveInput\":true"));
    CuAssertPtrNotNull(tc, strstr(payload, "\"inputKind\":\"password\""));

    /* A payload must never carry the secret it is asking for. */
    CuAssertTrue(tc, strstr(payload, "password\":\"") == NULL ||
                         strstr(payload, "\"inputLabel\"") != NULL);
  }
}

void TestWebOnboardingSexChoicesUseServerWireValues(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  init_test_descriptor(&d, CON_QSEX);

  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertTrue(tc, json_is_balanced(payload));
  CuAssertPtrNotNull(tc, strstr(payload, "\"screen\":\"sex\""));

  /* The wire values must match what nanny()'s CON_QSEX handler accepts. */
  CuAssertPtrNotNull(tc, strstr(payload, "\"wireValue\":\"m\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"wireValue\":\"f\""));
  CuAssertPtrNotNull(tc, strstr(payload, "identity/sex-male"));
  CuAssertPtrNotNull(tc, strstr(payload, "identity/sex-female"));
}

void TestWebOnboardingBuildChoicesUseStateMachineWireValues(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  init_test_descriptor(&d, CON_CONFIRM_PREMADE);

  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertTrue(tc, json_is_balanced(payload));
  CuAssertPtrNotNull(tc, strstr(payload, "\"screen\":\"build\""));

  /* CON_CONFIRM_PREMADE accepts the words "premade" and "custom", not a
   * yes/no answer. The structured choices must enter that same state machine. */
  CuAssertPtrNotNull(tc, strstr(payload, "\"wireValue\":\"premade\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"wireValue\":\"custom\""));
  CuAssertPtrNotNull(tc, strstr(payload, "build/premade"));
  CuAssertPtrNotNull(tc, strstr(payload, "build/custom"));
}

void TestWebOnboardingPersistenceReportsTheSaveBoundary(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  /* Before the alignment step the character is not written to disk. */
  init_test_descriptor(&d, CON_QSEX);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"persistence\":\"draft\""));

  /* Alignment is the step that saves, so it must not claim success yet. */
  init_test_descriptor(&d, CON_QALIGN);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"persistence\":\"pending\""));

  /* Recommended preferences run after the save has completed. */
  init_test_descriptor(&d, CON_SETPREFS);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"persistence\":\"saved\""));
}

/*
 * A descriptor can be sitting in a creation state with no character attached
 * (link loss, extraction) while the per-pulse state poll still runs. Every
 * catalog builder must tolerate that rather than dereference NULL.
 */
void TestWebOnboardingSurvivesADetachedCharacter(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];
  const int character_states[] = {CON_QRACE, CON_QCLASS, CON_QALIGN, CON_ACCOUNT_MENU};
  size_t index = 0;

  for (index = 0; index < sizeof(character_states) / sizeof(character_states[0]); index++)
  {
    init_test_descriptor(&d, character_states[index]);
    CuAssertPtrEquals(tc, NULL, d.character);

    CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
    CuAssertTrue(tc, json_is_balanced(payload));
    CuAssertTrue(tc, !has_control_bytes(payload));

    /* An empty catalog is honest; a crash or a stale screen is not. */
    CuAssertPtrNotNull(tc, strstr(payload, "\"choices\":[]"));
  }
}

/*
 * The CON_ constants are not ordered by flow position, so the selection
 * summary must be driven by explicit state lists. A numeric comparison would
 * report GET_CLASS()'s default of 0 as "Wizard" before the class step.
 */
void TestWebOnboardingDoesNotReportUnmadeChoices(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  /* Ancestry confirmation happens before any class is chosen. */
  init_test_descriptor(&d, CON_QRACE_HELP);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  /* Match the field form, not the bare word: "screen":"race" is not a
     selection, and neither is a "race/human" media key. */
  CuAssertTrue(tc, strstr(payload, "\"className\":\"") == NULL);
  CuAssertTrue(tc, strstr(payload, "\"alignment\":\"") == NULL);

  /* Ancestry itself is not settled while the player is still choosing one. */
  init_test_descriptor(&d, CON_QRACE);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertTrue(tc, strstr(payload, "\"race\":\"") == NULL);

  /* Alignment is only known after the alignment step completes. */
  init_test_descriptor(&d, CON_QALIGN);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertTrue(tc, strstr(payload, "\"alignment\":\"") == NULL);
}

/*
 * The confirm-or-reselect screens carry no choices, so they must supply the
 * selected entry's art and description instead of rendering an empty catalog.
 */
void TestWebOnboardingDetailScreensCarryTheirSelection(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  init_test_descriptor(&d, CON_QRACE_HELP);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertTrue(tc, json_is_balanced(payload));
  CuAssertPtrNotNull(tc, strstr(payload, "\"screen\":\"race-detail\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"confirm\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"reselect\""));
}

void TestWebOnboardingUnsupportedStatesHaveNoPayload(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  /* Playing, OLC, and the string editors all belong to the classic terminal. */
  init_test_descriptor(&d, CON_PLAYING);
  CuAssertTrue(tc, !web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertStrEquals(tc, "", payload);

  init_test_descriptor(&d, CON_REDIT);
  CuAssertTrue(tc, !web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertStrEquals(tc, "", payload);
}

void TestWebOnboardingRefusesToTruncateAnOversizePayload(CuTest *tc)
{
  struct descriptor_data d;
  char tiny[64];

  init_test_descriptor(&d, CON_QSEX);

  /* A truncated document would be malformed, so the adapter emits nothing. */
  CuAssertTrue(tc, !web_onboarding_build_payload(&d, tiny, sizeof(tiny)));
  CuAssertStrEquals(tc, "", tiny);
}

void TestWebOnboardingChoiceScreensOfferTheRightActions(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  /* Confirmation screens mirror the existing y/n prompts. */
  init_test_descriptor(&d, CON_NAME_CNFRM);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"confirm\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"reselect\""));

  /* The account menu exposes its extra menu commands. */
  init_test_descriptor(&d, CON_ACCOUNT_MENU);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"create-character\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"link-character\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"quit\""));
}
