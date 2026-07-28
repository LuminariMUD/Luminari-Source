/*
 * Structured web onboarding emitter tests.
 *
 * These exercise the presentation adapter in src/systems/web_client. All
 * fixtures are synthetic; no live account, character, or credential data may be
 * used here. Save-path tests inject a deterministic persistence callback.
 */

#include "CuTest.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/comm.h"
#include "../../src/backgrounds.h"
#include "../../src/char_descs.h"
#include "../../src/db.h"
#include "../../src/deities.h"
#include "../../src/protocol.h"
#include "../../src/roleplay.h"
#include "../../src/systems/web_client/onboarding.h"

/* A descriptor with just enough state for the adapter to read. */
static void init_test_descriptor(struct descriptor_data *d, int state)
{
  memset(d, 0, sizeof(*d));
  d->desc_num = 42;
  d->login_time = 1000;
  d->connected = state;
  d->output = d->small_outbuf;
  d->bufspace = SMALL_BUFSIZE - 1;
  d->small_outbuf[0] = '\0';
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

struct expected_background_identity
{
  int background;
  const char *id;
  const char *wire;
  const char *media_key;
};

static const struct expected_background_identity expected_background_identities[] = {
    {BACKGROUND_ACOLYTE, "acolyte", "acolyte", "background/acolyte"},
    {BACKGROUND_CHARLATAN, "charlatan", "charlatan", "background/charlatan"},
    {BACKGROUND_CRIMINAL, "criminal-spy", "criminal-spy", "background/criminal-spy"},
    {BACKGROUND_ENTERTAINER, "entertainer", "entertainer", "background/entertainer"},
    {BACKGROUND_FOLK_HERO, "folk-hero", "folk-hero", "background/folk-hero"},
    {BACKGROUND_GLADIATOR, "gladiator", "gladiator", "background/gladiator"},
    {BACKGROUND_TRADER, "trader", "trader", "background/trader"},
    {BACKGROUND_HERMIT, "hermit", "hermit", "background/hermit"},
    {BACKGROUND_SQUIRE, "squire", "squire", "background/squire"},
    {BACKGROUND_NOBLE, "noble", "noble", "background/noble"},
    {BACKGROUND_OUTLANDER, "outlander", "outlander", "background/outlander"},
    {BACKGROUND_PIRATE, "pirate", "pirate", "background/pirate"},
    {BACKGROUND_SAGE, "sage", "sage", "background/sage"},
    {BACKGROUND_SAILOR, "sailor", "sailor", "background/sailor"},
    {BACKGROUND_SOLDIER, "soldier", "soldier", "background/soldier"},
    {BACKGROUND_URCHIN, "urchin", "urchin", "background/urchin"},
};

void TestBackgroundIdentityMappingsAreStableAndAccepted(CuTest *tc)
{
  size_t index = 0;

  for (index = 0;
       index < sizeof(expected_background_identities) / sizeof(expected_background_identities[0]);
       index++)
  {
    const struct expected_background_identity *expected = &expected_background_identities[index];

    CuAssertStrEquals(tc, expected->id, background_stable_id(expected->background));
    CuAssertStrEquals(tc, expected->wire, background_wire_value(expected->background));
    CuAssertStrEquals(tc, expected->media_key, background_media_key(expected->background));
    CuAssertIntEquals(tc, expected->background, background_from_input(expected->wire));
  }

  /* Multiword and slash-bearing display names remain usable from Telnet. */
  CuAssertIntEquals(tc, BACKGROUND_FOLK_HERO, background_from_input("folk hero"));
  CuAssertIntEquals(tc, BACKGROUND_FOLK_HERO, background_from_input("FOLK-HERO"));
  CuAssertIntEquals(tc, BACKGROUND_CRIMINAL, background_from_input("criminal/spy"));
  CuAssertIntEquals(tc, BACKGROUND_CRIMINAL, background_from_input("criminal-spy"));

  CuAssertIntEquals(tc, BACKGROUND_NONE, background_from_input(""));
  CuAssertIntEquals(tc, BACKGROUND_NONE, background_from_input("not-a-background"));
  CuAssertStrEquals(tc, "unknown", background_stable_id(NUM_BACKGROUNDS));
  CuAssertStrEquals(tc, "", background_wire_value(-1));
  CuAssertStrEquals(tc, "background/fallback", background_media_key(NUM_BACKGROUNDS));
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

void TestWebOnboardingNameValidationErrorsAreStructured(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  init_test_descriptor(&d, CON_GET_NAME);
  web_onboarding_set_error(&d, WEB_ONBOARDING_ERROR_NAME_TAKEN);

  CuAssertTrue(tc, d.web_onboarding_dirty);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"screen\":\"name\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"code\":\"name-taken\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"field\":\"name\""));
  CuAssertPtrNotNull(tc, strstr(payload, "already in use"));
  CuAssertPtrNotNull(tc, strstr(payload, "\"sensitiveInput\":false"));

  web_onboarding_reset(&d);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertTrue(tc, strstr(payload, "\"error\":") == NULL);
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

void TestWebOnboardingRoleplayChoicesUseStateMachineWireValues(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  init_test_descriptor(&d, CON_CHAR_RP_DECIDE);

  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertTrue(tc, json_is_balanced(payload));
  CuAssertPtrNotNull(tc, strstr(payload, "\"screen\":\"roleplay-decision\""));

  /* CON_CHAR_RP_DECIDE uses 1 for non-role-player, 2 for role-player, and 3
   * for deciding later. The media labels must preserve that exact mapping. */
  CuAssertPtrNotNull(tc, strstr(payload,
                                "\"id\":\"roleplayer\",\"label\":\"Fill in role-play details\","
                                "\"wireValue\":\"2\""));
  CuAssertPtrNotNull(tc, strstr(payload,
                                "\"id\":\"non-roleplayer\",\"label\":\"Skip role-play details\","
                                "\"wireValue\":\"1\""));
  CuAssertPtrNotNull(
      tc, strstr(payload, "\"id\":\"later\",\"label\":\"Decide later\",\"wireValue\":\"3\""));
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

  /* Linking has an explicit return path instead of treating an empty response
   * as a disconnect. */
  init_test_descriptor(&d, CON_ACCOUNT_ADD);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"submit\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"cancel\""));
}

/* ------------------------------------------------------------------------ */
/* Protocol v2 negotiation                                                   */
/* ------------------------------------------------------------------------ */

void TestWebOnboardingVersionListSelectsHighestMutual(CuTest *tc)
{
  struct descriptor_data d;

  init_test_descriptor(&d, CON_ACCOUNT_NAME);

  /* A client that only speaks v1 stays on v1. */
  web_onboarding_set_capability(&d, "1");
  CuAssertIntEquals(tc, 1, d.web_onboarding_version);

  /*
   * A v2-capable client advertises the list after the single-version
   * variable. Which version is selected depends on the build flag: with v2
   * compiled out, the source must refuse to be talked up into it.
   */
  web_onboarding_set_version_list(&d, "2,1");
#if WEB_ONBOARDING_ENABLE_V2
  CuAssertIntEquals(tc, WEB_ONBOARDING_PROTOCOL_VERSION_MAX, d.web_onboarding_version);
#else
  CuAssertIntEquals(tc, WEB_ONBOARDING_PROTOCOL_VERSION, d.web_onboarding_version);
#endif
}

void TestWebOnboardingVersionListRejectsUnsupportedVersions(CuTest *tc)
{
  struct descriptor_data d;

  init_test_descriptor(&d, CON_ACCOUNT_NAME);
  web_onboarding_set_capability(&d, "1");

  /* A version this build does not implement must never be selected. */
  web_onboarding_set_version_list(&d, "99");
  CuAssertIntEquals(tc, 1, d.web_onboarding_version);

  /* Garbage must not disturb an already-negotiated version. */
  web_onboarding_set_version_list(&d, "not-a-version");
  CuAssertIntEquals(tc, 1, d.web_onboarding_version);

  web_onboarding_set_version_list(&d, "");
  CuAssertIntEquals(tc, 1, d.web_onboarding_version);

  /* A mixed list still picks only what is supported. */
  web_onboarding_set_version_list(&d, "77,1");
  CuAssertIntEquals(tc, 1, d.web_onboarding_version);
}

void TestWebOnboardingPayloadEchoesNegotiatedVersion(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  init_test_descriptor(&d, CON_ACCOUNT_NAME);
  web_onboarding_set_capability(&d, "1");

  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  /* A v1 client must never be handed a document claiming a newer contract. */
  CuAssertPtrNotNull(tc, strstr(payload, "\"version\":1"));
  CuAssertTrue(tc, strstr(payload, "\"version\":2") == NULL);
}

void TestWebOnboardingV2DisabledByDefault(CuTest *tc)
{
  struct descriptor_data d;

  init_test_descriptor(&d, CON_ACCOUNT_NAME);
  web_onboarding_set_capability(&d, "1");
  web_onboarding_set_version_list(&d, "2,1");

  /*
   * The role-play suite must stay off unless the build opted in, so that
   * rollback is a capability change rather than a redeploy.
   */
#if WEB_ONBOARDING_ENABLE_V2
  CuAssertTrue(tc, WEB_ONBOARDING_ENABLE_V2 == 1);
#else
  CuAssertTrue(tc, !web_onboarding_v2_enabled(&d));
#endif
}

/* ------------------------------------------------------------------------ */
/* Protocol v2 state coverage                                                */
/* ------------------------------------------------------------------------ */

/* Every role-play state the coverage matrix requires. */
static const int v2_roleplay_states[] = {
    CON_CHAR_RP_MENU,
    CON_GEN_DESCS_INTRO,
    CON_GEN_DESCS_DESCRIPTORS_1,
    CON_GEN_DESCS_ADJECTIVES_1,
    CON_GEN_DESCS_MENU,
    CON_GEN_DESCS_MENU_PARSE,
    CON_GEN_DESCS_DESCRIPTORS_2,
    CON_GEN_DESCS_ADJECTIVES_2,
    CON_PLR_DESC,
    CON_PLR_BG,
    CON_BACKGROUND_ARCHTYPE,
    CON_BACKGROUND_ARCHTYPE_CONFIRM,
    CON_CHARACTER_GOALS_IDEAS,
    CON_CHARACTER_GOALS_ENTER,
    CON_CHARACTER_PERSONALITY_IDEAS,
    CON_CHARACTER_PERSONALITY_ENTER,
    CON_CHARACTER_IDEALS_IDEAS,
    CON_CHARACTER_IDEALS_ENTER,
    CON_CHARACTER_BONDS_IDEAS,
    CON_CHARACTER_BONDS_ENTER,
    CON_CHARACTER_FLAWS_IDEAS,
    CON_CHARACTER_FLAWS_ENTER,
    CON_CHARACTER_AGE_SELECT,
    CON_QREGION,
    CON_QREGION_HELP,
    CON_CHARACTER_FACTION_SELECT,
    CON_CHARACTER_HOMETOWN_SELECT,
    CON_CHARACTER_DEITY_SELECT,
    CON_CHARACTER_DEITY_CONFIRM,
};

void TestWebOnboardingV1NeverSeesRoleplayScreens(CuTest *tc)
{
  size_t i = 0;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  /*
   * The safety property that makes v2 rollout reversible: a v1 client must
   * get no structured presentation for a role-play state, so the flow hands
   * off to the classic terminal exactly as it did before v2 existed.
   */
  for (i = 0; i < sizeof(v2_roleplay_states) / sizeof(v2_roleplay_states[0]); i++)
  {
    struct descriptor_data d;

    init_test_descriptor(&d, v2_roleplay_states[i]);
    web_onboarding_set_capability(&d, "1");

    CuAssertTrue(tc, !web_onboarding_build_payload(&d, payload, sizeof(payload)));
  }
}

/*
 * The v2 tests below are compiled unconditionally: the CuTest registry is
 * generated by scanning for test function names and does not honour #if, so a
 * conditionally-compiled test registers but fails to link. Each test instead
 * asserts the correct behaviour for whichever way the build flag is set.
 */

/** Negotiate the highest version this build offers. */
static void negotiate_best_version(struct descriptor_data *d)
{
  web_onboarding_set_capability(d, "1");
  web_onboarding_set_version_list(d, "2,1");
}

void TestWebOnboardingV2CoversEveryRoleplayState(CuTest *tc)
{
  size_t i = 0;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  for (i = 0; i < sizeof(v2_roleplay_states) / sizeof(v2_roleplay_states[0]); i++)
  {
    struct descriptor_data d;
    bool built = FALSE;

    init_test_descriptor(&d, v2_roleplay_states[i]);
    negotiate_best_version(&d);

    built = web_onboarding_build_payload(&d, payload, sizeof(payload));

#if WEB_ONBOARDING_ENABLE_V2
    /* No in-scope role-play state may produce an accidental handoff. */
    CuAssertTrue(tc, built);
    CuAssertTrue(tc, json_is_balanced(payload));
    CuAssertTrue(tc, !has_control_bytes(payload));
    CuAssertPtrNotNull(tc, strstr(payload, "\"version\":2"));
    CuAssertPtrNotNull(tc, strstr(payload, "\"mode\":\"roleplay-profile\""));
    /* The terminal must stay reachable from every structured state. */
    CuAssertPtrNotNull(tc, strstr(payload, "\"classic-terminal\""));
#else
    /* With v2 compiled out these states have no structured presentation. */
    CuAssertTrue(tc, !built);
#endif
  }
}

void TestWebOnboardingHubPublishesItemMetadataNotContent(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  init_test_descriptor(&d, CON_CHAR_RP_MENU);
  negotiate_best_version(&d);

  if (!web_onboarding_build_payload(&d, payload, sizeof(payload)))
  {
#if WEB_ONBOARDING_ENABLE_V2
    CuFail(tc, "the hub must have a structured presentation when v2 is enabled");
#endif
    return;
  }

  CuAssertPtrNotNull(tc, strstr(payload, "\"profile\":["));
  CuAssertPtrNotNull(tc, strstr(payload, "\"profile/background-story\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"status\":\"unset\""));
  /* The hub opens items; it never selects a catalog entry. */
  CuAssertPtrNotNull(tc, strstr(payload, "\"open\""));

  /* The hub is re-sent on every revision, so it must stay inside the cap. */
  CuAssertTrue(tc, strlen(payload) < WEB_ONBOARDING_MAX_PAYLOAD);
}

void TestWebOnboardingHubUsesRoleplayMenuWireValues(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];
  const char *expected_items[] = {
      "\"id\":\"profile/short-description\",\"label\":\"Short description\","
      "\"wireValue\":\"0\"",
      "\"id\":\"profile/long-description\",\"label\":\"Long description\","
      "\"wireValue\":\"1\"",
      "\"id\":\"profile/background-story\",\"label\":\"Background story\","
      "\"wireValue\":\"2\"",
      "\"id\":\"profile/background\",\"label\":\"Background\",\"wireValue\":\"3\"",
      "\"id\":\"profile/goals\",\"label\":\"Goals\",\"wireValue\":\"4\"",
      "\"id\":\"profile/personality\",\"label\":\"Personality\",\"wireValue\":\"5\"",
      "\"id\":\"profile/ideals\",\"label\":\"Ideals\",\"wireValue\":\"6\"",
      "\"id\":\"profile/bonds\",\"label\":\"Bonds\",\"wireValue\":\"7\"",
      "\"id\":\"profile/flaws\",\"label\":\"Flaws\",\"wireValue\":\"8\"",
      "\"id\":\"profile/age\",\"label\":\"Age\",\"wireValue\":\"9\"",
      "\"id\":\"profile/region\",\"label\":\"Homeland\",\"wireValue\":\"a\"",
      "\"id\":\"profile/faction\",\"label\":\"Faction\",\"wireValue\":\"b\"",
      "\"id\":\"profile/hometown\",\"label\":\"Hometown\",\"wireValue\":\"c\"",
      "\"id\":\"profile/deity\",\"label\":\"Deity\",\"wireValue\":\"d\"",
  };
  size_t index = 0;

  init_test_descriptor(&d, CON_CHAR_RP_MENU);
  negotiate_best_version(&d);

  if (!web_onboarding_build_payload(&d, payload, sizeof(payload)))
  {
#if WEB_ONBOARDING_ENABLE_V2
    CuFail(tc, "the hub must serialize the source menu values");
#endif
    return;
  }

  for (index = 0; index < sizeof(expected_items) / sizeof(expected_items[0]); index++)
    CuAssertPtrNotNull(tc, strstr(payload, expected_items[index]));
}

void TestWebOnboardingHubPublishesAuthoritativeStatusesWithoutPrivateText(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];
  char private_description[] = "SYNTHETIC_PRIVATE_PROFILE_SENTINEL";

  memset(&ch, 0, sizeof(ch));
  memset(&specials, 0, sizeof(specials));
  init_test_descriptor(&d, CON_CHAR_RP_MENU);
  negotiate_best_version(&d);
  assign_backgrounds();

  ch.player_specials = &specials;
  ch.player.description = private_description;
  specials.saved.sdesc_descriptor_1 = 1;
  specials.saved.sdesc_adjective_1 = 0;
  specials.saved.background_type = BACKGROUND_FOLK_HERO;
  specials.saved.character_age_saved = true;
  specials.saved.clan = 10;
  specials.saved.region = 1;
  specials.saved.hometown = 1;
  specials.saved.deity = 1;
  d.character = &ch;

  if (!web_onboarding_build_payload(&d, payload, sizeof(payload)))
  {
#if WEB_ONBOARDING_ENABLE_V2
    CuFail(tc, "the hub must serialize authoritative status metadata");
#endif
    return;
  }

  CuAssertTrue(tc, strstr(payload, private_description) == NULL);
  CuAssertPtrNotNull(
      tc, strstr(payload, "\"id\":\"profile/short-description\",\"label\":\"Short description\","
                          "\"wireValue\":\"0\",\"status\":\"unset\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"summary\":\"folk hero\""));
  CuAssertPtrNotNull(tc, strstr(payload, "A background is chosen once"));
  CuAssertPtrNotNull(tc, strstr(payload, "Age is chosen once"));
  CuAssertPtrNotNull(tc, strstr(payload, "already chosen a faction"));
  CuAssertPtrNotNull(tc, strstr(payload, "A hometown is chosen once"));
  CuAssertPtrNotNull(tc, strstr(payload, "A deity is chosen once"));

  specials.saved.sdesc_adjective_1 = 1;
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(
      tc, strstr(payload, "\"id\":\"profile/short-description\",\"label\":\"Short description\","
                          "\"wireValue\":\"0\",\"status\":\"configured\""));
}

void TestWebOnboardingBackgroundCatalogUsesStableIdentityAndFitsPayloadCap(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];
  size_t index = 0;

  init_test_descriptor(&d, CON_BACKGROUND_ARCHTYPE);
  d.pProtocol = ProtocolCreate();
  CuAssertPtrNotNull(tc, d.pProtocol);
  if (d.pProtocol == NULL)
    return;
  d.pProtocol->bMSDP = bool_t_true;
  negotiate_best_version(&d);
  assign_backgrounds();

  if (!web_onboarding_build_payload(&d, payload, sizeof(payload)))
  {
#if WEB_ONBOARDING_ENABLE_V2
    CuFail(tc, "the background catalog must fit in one bounded state payload");
#endif
    ProtocolDestroy(d.pProtocol);
    d.pProtocol = NULL;
    return;
  }

  CuAssertTrue(tc, json_is_balanced(payload));
  CuAssertTrue(tc, !has_control_bytes(payload));
  CuAssertTrue(tc, strlen(payload) < WEB_ONBOARDING_MAX_PAYLOAD);
  CuAssertPtrNotNull(tc, strstr(payload, "\"page\":{\"index\":0,\"count\":2,\"totalItems\":16}"));
  CuAssertPtrNotNull(tc,
                     strstr(payload, "\"actions\":[\"select\",\"detail\",\"next-page\",\"cancel\","
                                     "\"classic-terminal\"]"));

  for (index = 0; index < 12; index++)
  {
    int background = backgrounds_listed_alphabetically[index + 1];
    char fragment[256];

    snprintf(fragment, sizeof(fragment),
             "\"id\":\"%s\",\"label\":\"%s\",\"wireValue\":\"%s\","
             "\"enabled\":true,\"mediaKey\":\"%s\"",
             background_stable_id(background), background_list[background].name,
             background_wire_value(background), background_media_key(background));
    CuAssertPtrNotNull(tc, strstr(payload, fragment));
  }

  {
    int background = backgrounds_listed_alphabetically[13];
    char fragment[80];

    snprintf(fragment, sizeof(fragment), "\"id\":\"%s\"", background_stable_id(background));
    CuAssertTrue(tc, strstr(payload, fragment) == NULL);
  }
  CuAssertTrue(tc, web_onboarding_handle_catalog_control(&d, "__onboarding-next__"));
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"page\":{\"index\":1,\"count\":2,\"totalItems\":16}"));
  CuAssertPtrNotNull(tc, strstr(payload,
                                "\"actions\":[\"select\",\"detail\",\"previous-page\",\"cancel\","
                                "\"classic-terminal\"]"));

  for (index = 12; index < NUM_BACKGROUNDS - 1; index++)
  {
    int background = backgrounds_listed_alphabetically[index + 1];
    char fragment[256];

    snprintf(fragment, sizeof(fragment),
             "\"id\":\"%s\",\"label\":\"%s\",\"wireValue\":\"%s\","
             "\"enabled\":true,\"mediaKey\":\"%s\"",
             background_stable_id(background), background_list[background].name,
             background_wire_value(background), background_media_key(background));
    CuAssertPtrNotNull(tc, strstr(payload, fragment));
  }
  {
    int background = backgrounds_listed_alphabetically[1];
    char fragment[80];

    snprintf(fragment, sizeof(fragment), "\"id\":\"%s\"", background_stable_id(background));
    CuAssertTrue(tc, strstr(payload, fragment) == NULL);
  }
  web_onboarding_reset(&d);
  ProtocolDestroy(d.pProtocol);
  d.pProtocol = NULL;
}

void TestWebOnboardingEditorsRefuseLineSubmission(CuTest *tc)
{
  struct descriptor_data d;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  init_test_descriptor(&d, CON_PLR_BG);
  negotiate_best_version(&d);

  if (!web_onboarding_build_payload(&d, payload, sizeof(payload)))
  {
#if WEB_ONBOARDING_ENABLE_V2
    CuFail(tc, "the background editor must be structured when v2 is enabled");
#endif
    return;
  }

  CuAssertPtrNotNull(tc, strstr(payload, "\"inputKind\":\"multiline\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"save-text\""));
  /*
   * The decisive check: an editor must never offer `submit`, because that
   * would route private profile text through the command line.
   */
  CuAssertTrue(tc, strstr(payload, "\"submit\"") == NULL);
}

/* ------------------------------------------------------------------------ */
/* Protocol v2 editor transfer and checked role-play persistence             */
/* ------------------------------------------------------------------------ */

static bool editor_test_save_result = TRUE;
static int editor_test_save_calls = 0;
static bool editor_test_index_save_result = TRUE;
static int editor_test_index_save_calls = 0;

static bool editor_test_save_callback(struct char_data *ch, int mode)
{
  (void)ch;
  (void)mode;
  editor_test_save_calls++;
  return editor_test_save_result;
}

static bool editor_test_index_save_callback(void)
{
  editor_test_index_save_calls++;
  return editor_test_index_save_result;
}

static bool init_editor_descriptor(struct descriptor_data *d, struct char_data *ch,
                                   struct player_special_data *specials, int state)
{
  memset(ch, 0, sizeof(*ch));
  memset(specials, 0, sizeof(*specials));
  init_test_descriptor(d, state);

  d->pProtocol = ProtocolCreate();
  if (d->pProtocol == NULL)
    return FALSE;

  d->pProtocol->bMSDP = bool_t_true;
  d->web_onboarding_version = WEB_ONBOARDING_PROTOCOL_VERSION_MAX;
  d->web_onboarding_revision = 7;
  d->web_onboarding_last_state = state;
  d->character = ch;
  ch->desc = d;
  ch->player_specials = specials;
  editor_test_save_result = TRUE;
  editor_test_save_calls = 0;
  editor_test_index_save_result = TRUE;
  editor_test_index_save_calls = 0;
  roleplay_text_set_save_callback_for_test(editor_test_save_callback);
  roleplay_index_set_save_callback_for_test(editor_test_index_save_callback);
  return TRUE;
}

static void reset_editor_test_output(struct descriptor_data *d)
{
  if (d == NULL)
    return;

  if (d->large_outbuf != NULL)
  {
    d->output = d->large_outbuf->text;
    d->bufspace = LARGE_BUFSIZE - 1;
  }
  else
  {
    d->output = d->small_outbuf;
    d->bufspace = SMALL_BUFSIZE - 1;
  }

  d->output[0] = '\0';
  d->bufptr = 0;
}

static void cleanup_editor_descriptor(struct descriptor_data *d)
{
  web_onboarding_reset(d);
  if (d->pProtocol != NULL)
  {
    ProtocolDestroy(d->pProtocol);
    d->pProtocol = NULL;
  }
  if (d->large_outbuf != NULL)
  {
    free(d->large_outbuf->text);
    free(d->large_outbuf);
    d->large_outbuf = NULL;
    d->output = d->small_outbuf;
  }
  roleplay_text_set_save_callback_for_test(NULL);
  roleplay_index_set_save_callback_for_test(NULL);
}

static void editor_test_digest(const unsigned char *content, size_t content_bytes, char digest[65])
{
  unsigned char raw_digest[EVP_MAX_MD_SIZE];
  unsigned int raw_digest_bytes = 0;
  size_t index = 0;

  memset(raw_digest, 0, sizeof(raw_digest));
  digest[0] = '\0';
  if (EVP_Digest(content, content_bytes, raw_digest, &raw_digest_bytes, EVP_sha256(), NULL) != 1 ||
      raw_digest_bytes != 32)
    return;

  for (index = 0; index < raw_digest_bytes; index++)
    snprintf(digest + (index * 2), 3, "%02x", raw_digest[index]);
  digest[64] = '\0';
}

static void editor_test_begin(struct descriptor_data *d, const char *field_id,
                              const char *transfer_id, const unsigned char *content,
                              size_t content_bytes, const char *digest_override, int64_t now_ms)
{
  char envelope[1024];
  char digest[65];
  int chunk_count = content_bytes == 0
                        ? 0
                        : (int)((content_bytes + WEB_ONBOARDING_EDITOR_MAX_CHUNK_BYTES - 1) /
                                WEB_ONBOARDING_EDITOR_MAX_CHUNK_BYTES);

  editor_test_digest(content, content_bytes, digest);
  snprintf(envelope, sizeof(envelope),
           "{\"v\":2,\"action\":\"save-text\",\"phase\":\"begin\","
           "\"flowId\":\"42-1000\",\"revision\":%d,\"fieldId\":\"%s\","
           "\"transferId\":\"%s\",\"totalBytes\":%zu,\"chunkCount\":%d,"
           "\"digest\":\"%s\"}",
           d->web_onboarding_revision, field_id, transfer_id, content_bytes, chunk_count,
           digest_override != NULL ? digest_override : digest);
  web_onboarding_handle_action_at_for_test(d, envelope, now_ms);
}

static void editor_test_chunks(struct descriptor_data *d, const char *transfer_id,
                               const unsigned char *content, size_t content_bytes, int64_t now_ms)
{
  char encoded[WEB_ONBOARDING_EDITOR_MAX_BASE64_BYTES + 1];
  char envelope[WEB_ONBOARDING_MAX_PAYLOAD + 1];
  size_t offset = 0;
  int index = 0;

  while (offset < content_bytes)
  {
    size_t raw_bytes = MIN(content_bytes - offset, (size_t)WEB_ONBOARDING_EDITOR_MAX_CHUNK_BYTES);
    int encoded_bytes = EVP_EncodeBlock((unsigned char *)encoded, content + offset, (int)raw_bytes);

    encoded[encoded_bytes] = '\0';
    snprintf(envelope, sizeof(envelope),
             "{\"phase\":\"chunk\",\"transferId\":\"%s\",\"index\":%d,\"data\":\"%s\"}",
             transfer_id, index, encoded);
    web_onboarding_handle_action_at_for_test(d, envelope, now_ms);
    offset += raw_bytes;
    index++;
  }
}

static void editor_test_commit(struct descriptor_data *d, const char *transfer_id, int64_t now_ms)
{
  char envelope[256];

  snprintf(envelope, sizeof(envelope), "{\"phase\":\"commit\",\"transferId\":\"%s\"}", transfer_id);
  web_onboarding_handle_action_at_for_test(d, envelope, now_ms);
}

static void editor_test_transfer(struct descriptor_data *d, const char *field_id,
                                 const char *transfer_id, const unsigned char *content,
                                 size_t content_bytes, const char *digest_override, int64_t now_ms)
{
  editor_test_begin(d, field_id, transfer_id, content, content_bytes, digest_override, now_ms);
  if (!web_onboarding_has_active_transfer_for_test(d))
    return;
  editor_test_chunks(d, transfer_id, content, content_bytes, now_ms);
  if (!web_onboarding_has_active_transfer_for_test(d))
    return;
  editor_test_commit(d, transfer_id, now_ms);
}

void TestRoleplayTextFieldsShareStableSlotsAndLimits(CuTest *tc)
{
  struct char_data ch;

  memset(&ch, 0, sizeof(ch));
  CuAssertIntEquals(tc, ROLEPLAY_TEXT_FIELD_LONG_DESCRIPTION,
                    roleplay_text_field_from_state(CON_PLR_DESC));
  CuAssertIntEquals(tc, ROLEPLAY_TEXT_FIELD_BACKGROUND_STORY,
                    roleplay_text_field_from_id("background-story"));
  CuAssertIntEquals(tc, ROLEPLAY_TEXT_FIELD_FLAWS,
                    roleplay_text_field_from_state(CON_CHARACTER_FLAWS_ENTER));
  CuAssertIntEquals(tc, ROLEPLAY_TEXT_FIELD_INVALID, roleplay_text_field_from_id("not-a-field"));
  CuAssertStrEquals(tc, "goals", roleplay_text_field_id(ROLEPLAY_TEXT_FIELD_GOALS));
  CuAssertIntEquals(tc, PLR_DESC_LENGTH,
                    (int)roleplay_text_field_max_bytes(ROLEPLAY_TEXT_FIELD_LONG_DESCRIPTION));
  CuAssertIntEquals(tc, PLR_BG_LENGTH,
                    (int)roleplay_text_field_max_bytes(ROLEPLAY_TEXT_FIELD_BACKGROUND_STORY));
  CuAssertPtrEquals(tc, &ch.player.ideals,
                    roleplay_text_field_slot(&ch, ROLEPLAY_TEXT_FIELD_IDEALS));
}

void TestRoleplayTextCommitNormalizesAndRollsBackAtomically(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  const unsigned char normalized_input[] = "first\r\nsecond~\rthird\t";
  const unsigned char replacement[] = "replacement";
  char *original = NULL;

  memset(&ch, 0, sizeof(ch));
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;
  ch.player.background = strdup("original");
  original = ch.player.background;
  editor_test_save_result = TRUE;
  editor_test_save_calls = 0;
  roleplay_text_set_save_callback_for_test(editor_test_save_callback);

  CuAssertIntEquals(tc, ROLEPLAY_TEXT_COMMIT_OK,
                    roleplay_text_commit_checked(&ch, ROLEPLAY_TEXT_FIELD_BACKGROUND_STORY,
                                                 normalized_input, sizeof(normalized_input) - 1));
  CuAssertIntEquals(tc, 1, editor_test_save_calls);
  CuAssertStrEquals(tc, "first\nsecond \nthird\t", ch.player.background);
  CuAssertTrue(tc, ch.player.background != original);

  original = ch.player.background;
  editor_test_save_result = FALSE;
  CuAssertIntEquals(tc, ROLEPLAY_TEXT_COMMIT_SAVE_FAILED,
                    roleplay_text_commit_checked(&ch, ROLEPLAY_TEXT_FIELD_BACKGROUND_STORY,
                                                 replacement, sizeof(replacement) - 1));
  CuAssertPtrEquals(tc, original, ch.player.background);
  CuAssertStrEquals(tc, "first\nsecond \nthird\t", ch.player.background);

  free(ch.player.background);
  ch.player.background = NULL;
  roleplay_text_set_save_callback_for_test(NULL);
}

void TestRoleplayTextCommitRejectsInvalidBytesBeforeSave(CuTest *tc)
{
  struct char_data ch;
  struct player_special_data specials;
  const unsigned char invalid_utf8[] = {0xc0, 0xaf};
  const unsigned char invalid_control[] = {'a', 0x01, 'b'};

  memset(&ch, 0, sizeof(ch));
  memset(&specials, 0, sizeof(specials));
  ch.player_specials = &specials;
  editor_test_save_result = TRUE;
  editor_test_save_calls = 0;
  roleplay_text_set_save_callback_for_test(editor_test_save_callback);

  CuAssertIntEquals(tc, ROLEPLAY_TEXT_COMMIT_INVALID_CONTENT,
                    roleplay_text_commit_checked(&ch, ROLEPLAY_TEXT_FIELD_GOALS, invalid_utf8,
                                                 sizeof(invalid_utf8)));
  CuAssertIntEquals(tc, ROLEPLAY_TEXT_COMMIT_INVALID_CONTENT,
                    roleplay_text_commit_checked(&ch, ROLEPLAY_TEXT_FIELD_GOALS, invalid_control,
                                                 sizeof(invalid_control)));
  CuAssertIntEquals(tc, 0, editor_test_save_calls);
  CuAssertPtrEquals(tc, NULL, ch.player.goals);
  roleplay_text_set_save_callback_for_test(NULL);
}

void TestWebOnboardingEmptyEditorPublishesAuthoritativeMetadata(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_PLR_DESC));
  if (d.pProtocol == NULL)
    return;
  if (web_onboarding_build_payload(&d, payload, sizeof(payload)))
  {
#if WEB_ONBOARDING_ENABLE_V2
    CuAssertPtrNotNull(tc, strstr(payload,
                                  "\"editor\":{\"fieldId\":\"long-description\",\"maxBytes\":4096,"
                                  "\"contentRevision\":0,\"empty\":true}"));
#else
    CuFail(tc, "a v2 editor payload must not build when protocol v2 is disabled");
#endif
  }
  else
  {
#if WEB_ONBOARDING_ENABLE_V2
    CuFail(tc, "an empty v2 editor must publish source field metadata");
#endif
  }

  cleanup_editor_descriptor(&d);
}

void TestWebOnboardingExistingEditorEmitsVerifiedContentAfterState(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
#if WEB_ONBOARDING_ENABLE_V2
  const unsigned char normalized[] = "existing\nstory ";
  char encoded[128];
  char digest[65];
  char emitted[WEB_ONBOARDING_MAX_PAYLOAD + 4096];
  const char *state_marker = NULL;
  const char *content_marker = NULL;
  int ticks = 0;
  int encoded_bytes = 0;
#endif

  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_PLR_BG));
  if (d.pProtocol == NULL)
    return;

#if WEB_ONBOARDING_ENABLE_V2
  ch.player.background = strdup("existing\r\nstory~");
  d.str = &ch.player.background;
  d.web_onboarding_dirty = TRUE;
  web_onboarding_tick(&d);

  strlcpy(emitted, d.output, sizeof(emitted));
  CuAssertPtrNotNull(tc, strstr(emitted, WEB_ONBOARDING_MSDP_VARIABLE));
  CuAssertTrue(tc, strstr(emitted, WEB_ONBOARDING_CONTENT_VARIABLE) == NULL);
  CuAssertTrue(tc, web_onboarding_has_active_outbound_transfer_for_test(&d));
  reset_editor_test_output(&d);

  while (web_onboarding_has_active_outbound_transfer_for_test(&d) && ticks < 5)
  {
    web_onboarding_tick(&d);
    strlcat(emitted, d.output, sizeof(emitted));
    reset_editor_test_output(&d);
    ticks++;
  }

  state_marker = strstr(emitted, WEB_ONBOARDING_MSDP_VARIABLE);
  content_marker = strstr(emitted, WEB_ONBOARDING_CONTENT_VARIABLE);
  CuAssertPtrNotNull(tc, state_marker);
  CuAssertPtrNotNull(tc, content_marker);
  CuAssertTrue(tc, state_marker < content_marker);
  CuAssertPtrNotNull(tc, strstr(emitted, "\"empty\":false"));
  CuAssertPtrNotNull(tc, strstr(emitted, "\"transferId\":\"source-42-8-1-0\""));
  CuAssertPtrNotNull(tc, strstr(emitted, "\"phase\":\"begin\""));
  CuAssertPtrNotNull(tc, strstr(emitted, "\"phase\":\"commit\""));
  CuAssertTrue(tc, strstr(emitted, "existing\r\nstory") == NULL);

  encoded_bytes = EVP_EncodeBlock((unsigned char *)encoded, normalized, sizeof(normalized) - 1);
  encoded[encoded_bytes] = '\0';
  editor_test_digest(normalized, sizeof(normalized) - 1, digest);
  CuAssertPtrNotNull(tc, strstr(emitted, encoded));
  CuAssertPtrNotNull(tc, strstr(emitted, digest));
  CuAssertTrue(tc, !web_onboarding_has_active_outbound_transfer_for_test(&d));
#else
  CuAssertTrue(tc, WEB_ONBOARDING_ENABLE_V2 == 0);
#endif

  free(ch.player.background);
  ch.player.background = NULL;
  cleanup_editor_descriptor(&d);
}

void TestWebOnboardingMaximumEditorDownloadIsPacedWithoutOverflow(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
#if WEB_ONBOARDING_ENABLE_V2
  int begin_frames = 0;
  int chunk_frames = 0;
  int commit_frames = 0;
  int ticks = 0;
#endif

  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_PLR_BG));
  if (d.pProtocol == NULL)
    return;

#if WEB_ONBOARDING_ENABLE_V2
  ch.player.background = malloc(WEB_ONBOARDING_EDITOR_MAX_CONTENT_BYTES + 1);
  CuAssertPtrNotNull(tc, ch.player.background);
  if (ch.player.background != NULL)
  {
    memset(ch.player.background, 'x', WEB_ONBOARDING_EDITOR_MAX_CONTENT_BYTES);
    ch.player.background[WEB_ONBOARDING_EDITOR_MAX_CONTENT_BYTES] = '\0';
    d.str = &ch.player.background;
    d.web_onboarding_dirty = TRUE;
    web_onboarding_tick(&d);

    CuAssertPtrNotNull(tc, strstr(d.output, "\"totalBytes\":49152"));
    CuAssertPtrNotNull(tc, strstr(d.output, "\"chunkCount\":8"));
    CuAssertTrue(tc, strstr(d.output, WEB_ONBOARDING_CONTENT_VARIABLE) == NULL);
    CuAssertTrue(tc, (size_t)d.bufptr < LARGE_BUFSIZE);
    reset_editor_test_output(&d);

    while (web_onboarding_has_active_outbound_transfer_for_test(&d) &&
           ticks < WEB_ONBOARDING_EDITOR_MAX_CHUNKS + 3)
    {
      web_onboarding_tick(&d);
      CuAssertPtrNotNull(tc, strstr(d.output, WEB_ONBOARDING_CONTENT_VARIABLE));
      CuAssertTrue(tc, strstr(d.output, "OVERFLOW") == NULL);
      CuAssertTrue(tc, (size_t)d.bufptr < LARGE_BUFSIZE);
      if (strstr(d.output, "\"phase\":\"begin\"") != NULL)
        begin_frames++;
      if (strstr(d.output, "\"phase\":\"chunk\"") != NULL)
        chunk_frames++;
      if (strstr(d.output, "\"phase\":\"commit\"") != NULL)
        commit_frames++;
      reset_editor_test_output(&d);
      ticks++;
    }

    CuAssertIntEquals(tc, 1, begin_frames);
    CuAssertIntEquals(tc, WEB_ONBOARDING_EDITOR_MAX_CHUNKS, chunk_frames);
    CuAssertIntEquals(tc, 1, commit_frames);
    CuAssertIntEquals(tc, WEB_ONBOARDING_EDITOR_MAX_CHUNKS + 2, ticks);
    CuAssertTrue(tc, !web_onboarding_has_active_outbound_transfer_for_test(&d));
  }
#else
  CuAssertTrue(tc, WEB_ONBOARDING_ENABLE_V2 == 0);
#endif

  free(ch.player.background);
  ch.player.background = NULL;
  cleanup_editor_descriptor(&d);
}

void TestWebOnboardingInvalidExistingEditorContentFailsClosed(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
#if WEB_ONBOARDING_ENABLE_V2
  const unsigned char invalid_content[] = {'p', 'r', 'i', 'v', 'a', 't', 'e', 0x01, 'x', '\0'};
#endif

  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_PLR_BG));
  if (d.pProtocol == NULL)
    return;

#if WEB_ONBOARDING_ENABLE_V2
  ch.player.background = malloc(sizeof(invalid_content));
  CuAssertPtrNotNull(tc, ch.player.background);
  if (ch.player.background != NULL)
  {
    memcpy(ch.player.background, invalid_content, sizeof(invalid_content));
    d.str = &ch.player.background;
    d.web_onboarding_dirty = TRUE;
    web_onboarding_tick(&d);

    CuAssertPtrNotNull(tc, strstr(d.output, WEB_ONBOARDING_MSDP_VARIABLE));
    CuAssertPtrNotNull(tc, strstr(d.output, "\"code\":\"editor-load-failed\""));
    CuAssertTrue(tc, strstr(d.output, WEB_ONBOARDING_CONTENT_VARIABLE) == NULL);
    CuAssertTrue(tc, strstr(d.output, "\"editor\":") == NULL);
    CuAssertTrue(tc, !web_onboarding_has_active_outbound_transfer_for_test(&d));
  }
#else
  CuAssertTrue(tc, WEB_ONBOARDING_ENABLE_V2 == 0);
#endif

  free(ch.player.background);
  ch.player.background = NULL;
  cleanup_editor_descriptor(&d);
}

void TestWebOnboardingStateWaitsForOutputCapacity(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;

  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_PLR_BG));
  if (d.pProtocol == NULL)
    return;

#if WEB_ONBOARDING_ENABLE_V2
  d.web_onboarding_dirty = TRUE;
  d.bufptr = LARGE_BUFSIZE - 1;
  d.bufspace = 0;
  web_onboarding_tick(&d);

  CuAssertIntEquals(tc, 7, d.web_onboarding_revision);
  CuAssertTrue(tc, d.web_onboarding_dirty);
  CuAssertTrue(tc, d.output[0] == '\0');
  CuAssertTrue(tc, !web_onboarding_has_active_outbound_transfer_for_test(&d));

  reset_editor_test_output(&d);
  web_onboarding_tick(&d);
  CuAssertIntEquals(tc, 8, d.web_onboarding_revision);
  CuAssertTrue(tc, !d.web_onboarding_dirty);
  CuAssertPtrNotNull(tc, strstr(d.output, WEB_ONBOARDING_MSDP_VARIABLE));
#else
  CuAssertTrue(tc, WEB_ONBOARDING_ENABLE_V2 == 0);
#endif

  cleanup_editor_descriptor(&d);
}

void TestWebOnboardingEditorTransferCommitsOnlyAfterCheckedSave(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
  const unsigned char content[] = "An invented history.\nSecond line.";
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  (void)payload;
  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_PLR_BG));
  if (d.pProtocol == NULL)
    return;

#if WEB_ONBOARDING_ENABLE_V2
  d.str = &ch.player.background;
  editor_test_transfer(&d, "background-story", "transfer-happy", content, sizeof(content) - 1, NULL,
                       1000);

  CuAssertIntEquals(tc, 1, editor_test_save_calls);
  CuAssertStrEquals(tc, (const char *)content, ch.player.background);
  CuAssertIntEquals(tc, CON_CHAR_RP_MENU, d.connected);
  CuAssertTrue(tc, !web_onboarding_has_active_transfer_for_test(&d));
  CuAssertPtrEquals(tc, NULL, d.str);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"persistenceResult\":\"saved\""));
#else
  editor_test_transfer(&d, "background-story", "transfer-happy", content, sizeof(content) - 1, NULL,
                       1000);
  CuAssertIntEquals(tc, 0, editor_test_save_calls);
  CuAssertPtrEquals(tc, NULL, ch.player.background);
#endif

  free(ch.player.background);
  ch.player.background = NULL;
  cleanup_editor_descriptor(&d);
}

void TestWebOnboardingEditorSaveFailureRestoresEmptyField(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
  const unsigned char content[] = "This must roll back.";
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];

  (void)payload;
  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_CHARACTER_GOALS_ENTER));
  if (d.pProtocol == NULL)
    return;
  editor_test_save_result = FALSE;

#if WEB_ONBOARDING_ENABLE_V2
  d.str = &ch.player.goals;
  editor_test_transfer(&d, "goals", "transfer-fail", content, sizeof(content) - 1, NULL, 1000);

  CuAssertIntEquals(tc, 1, editor_test_save_calls);
  CuAssertPtrEquals(tc, NULL, ch.player.goals);
  CuAssertIntEquals(tc, CON_CHARACTER_GOALS_ENTER, d.connected);
  CuAssertTrue(tc, !web_onboarding_has_active_transfer_for_test(&d));
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"persistenceResult\":\"failed\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"code\":\"editor-save-failed\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"fieldId\":\"goals\""));
#else
  editor_test_transfer(&d, "goals", "transfer-fail", content, sizeof(content) - 1, NULL, 1000);
  CuAssertIntEquals(tc, 0, editor_test_save_calls);
#endif

  cleanup_editor_descriptor(&d);
}

void TestWebOnboardingEditorRejectsDigestOrderTimeoutAndDuplicateKeys(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
  const unsigned char content[] = "synthetic";
  const unsigned char one_byte[] = "f";
  const char *wrong_digest = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
  char envelope[1200];

  (void)tc;
  (void)content;
  (void)wrong_digest;
  (void)envelope;
  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_PLR_BG));
  if (d.pProtocol == NULL)
    return;

#if WEB_ONBOARDING_ENABLE_V2
  editor_test_transfer(&d, "background-story", "bad-digest", content, sizeof(content) - 1,
                       wrong_digest, 1000);
  CuAssertIntEquals(tc, 0, editor_test_save_calls);
  CuAssertPtrEquals(tc, NULL, ch.player.background);
  CuAssertTrue(tc, !web_onboarding_has_active_transfer_for_test(&d));

  d.web_onboarding_error = WEB_ONBOARDING_ERROR_NONE;
  editor_test_begin(&d, "background-story", "bad-order", content, sizeof(content) - 1, NULL, 2000);
  snprintf(envelope, sizeof(envelope),
           "{\"phase\":\"chunk\",\"transferId\":\"bad-order\",\"index\":1,"
           "\"data\":\"c3ludGhldGlj\"}");
  web_onboarding_handle_action_at_for_test(&d, envelope, 2000);
  CuAssertTrue(tc, !web_onboarding_has_active_transfer_for_test(&d));
  CuAssertIntEquals(tc, 0, editor_test_save_calls);

  editor_test_begin(&d, "background-story", "timeout", content, sizeof(content) - 1, NULL, 3000);
  CuAssertTrue(tc, web_onboarding_has_active_transfer_for_test(&d));
  editor_test_chunks(&d, "timeout", content, sizeof(content) - 1,
                     3000 + WEB_ONBOARDING_EDITOR_TRANSFER_TIMEOUT_MS + 1);
  CuAssertTrue(tc, !web_onboarding_has_active_transfer_for_test(&d));
  CuAssertIntEquals(tc, 0, editor_test_save_calls);

  snprintf(envelope, sizeof(envelope),
           "{\"v\":2,\"action\":\"save-text\",\"phase\":\"begin\","
           "\"flowId\":\"42-1000\",\"revision\":7,\"revision\":7,"
           "\"fieldId\":\"background-story\",\"transferId\":\"duplicate-key\","
           "\"totalBytes\":0,\"chunkCount\":0,"
           "\"digest\":\"e3b0c44298fc1c149afbf4c8996fb924"
           "27ae41e4649b934ca495991b7852b855\"}");
  web_onboarding_handle_action_at_for_test(&d, envelope, 4000);
  CuAssertTrue(tc, !web_onboarding_has_active_transfer_for_test(&d));
  CuAssertIntEquals(tc, 0, editor_test_save_calls);

  editor_test_begin(&d, "background-story", "noncanonical-base64", one_byte, sizeof(one_byte) - 1,
                    NULL, 5000);
  snprintf(envelope, sizeof(envelope),
           "{\"phase\":\"chunk\",\"transferId\":\"noncanonical-base64\","
           "\"index\":0,\"data\":\"Zh==\"}");
  web_onboarding_handle_action_at_for_test(&d, envelope, 5000);
  CuAssertTrue(tc, !web_onboarding_has_active_transfer_for_test(&d));
  CuAssertIntEquals(tc, 0, editor_test_save_calls);
#endif

  cleanup_editor_descriptor(&d);
}

void TestWebOnboardingEditorHonorsExactSourceFieldLimits(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
  unsigned char *content = NULL;

  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_PLR_BG));
  if (d.pProtocol == NULL)
    return;

#if WEB_ONBOARDING_ENABLE_V2
  content = malloc(WEB_ONBOARDING_EDITOR_MAX_CONTENT_BYTES);
  CuAssertPtrNotNull(tc, content);
  if (content != NULL)
  {
    memset(content, 'x', WEB_ONBOARDING_EDITOR_MAX_CONTENT_BYTES);
    d.str = &ch.player.background;
    editor_test_transfer(&d, "background-story", "exact-limit", content,
                         WEB_ONBOARDING_EDITOR_MAX_CONTENT_BYTES, NULL, 1000);
    CuAssertIntEquals(tc, 1, editor_test_save_calls);
    CuAssertPtrNotNull(tc, ch.player.background);
    CuAssertIntEquals(tc, WEB_ONBOARDING_EDITOR_MAX_CONTENT_BYTES,
                      ch.player.background != NULL ? (int)strlen(ch.player.background) : 0);
  }
#else
  CuAssertTrue(tc, WEB_ONBOARDING_ENABLE_V2 == 0);
#endif

  free(content);
  free(ch.player.background);
  ch.player.background = NULL;
  cleanup_editor_descriptor(&d);
}

void TestWebOnboardingEditorEnforcesCommitAndByteRateBudgets(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
  const unsigned char short_content[] = "x";
  unsigned char *large_content = NULL;
  char transfer_id[64];
  int64_t byte_window_ms = 1000 + WEB_ONBOARDING_EDITOR_RATE_WINDOW_MS + 1;
  int index = 0;

  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_PLR_BG));
  if (d.pProtocol == NULL)
    return;

#if WEB_ONBOARDING_ENABLE_V2
  for (index = 0; index < WEB_ONBOARDING_EDITOR_MAX_COMMITS_PER_WINDOW; index++)
  {
    d.connected = CON_PLR_BG;
    d.str = &ch.player.background;
    snprintf(transfer_id, sizeof(transfer_id), "commit-budget-%d", index);
    editor_test_transfer(&d, "background-story", transfer_id, short_content,
                         sizeof(short_content) - 1, NULL, 1000);
    CuAssertIntEquals(tc, index + 1, editor_test_save_calls);
    d.output = d.small_outbuf;
    d.small_outbuf[0] = '\0';
    d.bufptr = 0;
    d.bufspace = SMALL_BUFSIZE - 1;
  }

  d.connected = CON_PLR_BG;
  d.str = &ch.player.background;
  editor_test_transfer(&d, "background-story", "commit-budget-rejected", short_content,
                       sizeof(short_content) - 1, NULL, 1000);
  CuAssertIntEquals(tc, WEB_ONBOARDING_EDITOR_MAX_COMMITS_PER_WINDOW, editor_test_save_calls);
  CuAssertIntEquals(tc, WEB_ONBOARDING_ERROR_EDITOR_RATE_LIMITED, d.web_onboarding_error);
  CuAssertStrEquals(tc, "x", ch.player.background);

  large_content = malloc(48000);
  CuAssertPtrNotNull(tc, large_content);
  if (large_content != NULL)
  {
    memset(large_content, 'b', 48000);
    for (index = 0; index < 10; index++)
    {
      d.connected = CON_PLR_BG;
      d.str = &ch.player.background;
      snprintf(transfer_id, sizeof(transfer_id), "byte-budget-%d", index);
      editor_test_transfer(&d, "background-story", transfer_id, large_content, 48000, NULL,
                           byte_window_ms);
      CuAssertIntEquals(tc, WEB_ONBOARDING_EDITOR_MAX_COMMITS_PER_WINDOW + index + 1,
                        editor_test_save_calls);
      d.output = d.small_outbuf;
      d.small_outbuf[0] = '\0';
      d.bufptr = 0;
      d.bufspace = SMALL_BUFSIZE - 1;
    }

    d.connected = CON_PLR_BG;
    d.str = &ch.player.background;
    editor_test_transfer(&d, "background-story", "byte-budget-rejected", large_content, 48000, NULL,
                         byte_window_ms);
    CuAssertIntEquals(tc, WEB_ONBOARDING_EDITOR_MAX_COMMITS_PER_WINDOW + 10,
                      editor_test_save_calls);
    CuAssertIntEquals(tc, WEB_ONBOARDING_ERROR_EDITOR_RATE_LIMITED, d.web_onboarding_error);
    CuAssertIntEquals(tc, 48000,
                      ch.player.background != NULL ? (int)strlen(ch.player.background) : 0);
  }
#else
  CuAssertTrue(tc, WEB_ONBOARDING_ENABLE_V2 == 0);
#endif

  free(large_content);
  free(ch.player.background);
  ch.player.background = NULL;
  cleanup_editor_descriptor(&d);
}

void TestWebOnboardingEditorCancelAndResetNeverMutate(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
  const unsigned char content[] = "not committed";
  char envelope[512];

  (void)tc;
  (void)content;
  (void)envelope;
  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_PLR_BG));
  if (d.pProtocol == NULL)
    return;

#if WEB_ONBOARDING_ENABLE_V2
  ch.player.background = strdup("original");
  d.str = &ch.player.background;
  d.backstr = strdup(ch.player.background);
  snprintf(envelope, sizeof(envelope),
           "{\"v\":2,\"action\":\"cancel-editor\",\"phase\":\"cancel\","
           "\"flowId\":\"42-1000\",\"revision\":7,"
           "\"fieldId\":\"background-story\"}");
  web_onboarding_handle_action_at_for_test(&d, envelope, 1000);
  CuAssertStrEquals(tc, "original", ch.player.background);
  CuAssertIntEquals(tc, 0, editor_test_save_calls);
  CuAssertIntEquals(tc, CON_CHAR_RP_MENU, d.connected);
  CuAssertPtrEquals(tc, NULL, d.str);

  d.connected = CON_PLR_BG;
  d.web_onboarding_revision = 8;
  editor_test_begin(&d, "background-story", "reset-me", content, sizeof(content) - 1, NULL, 2000);
  CuAssertTrue(tc, web_onboarding_has_active_transfer_for_test(&d));
  web_onboarding_reset(&d);
  CuAssertTrue(tc, !web_onboarding_has_active_transfer_for_test(&d));
#endif

  free(ch.player.background);
  ch.player.background = NULL;
  cleanup_editor_descriptor(&d);
}

void TestWebOnboardingRoleplayCatalogsPublishAuthoritativeChoices(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];
  const int states[] = {
      CON_CHARACTER_AGE_SELECT,     CON_QREGION,
      CON_CHARACTER_FACTION_SELECT, CON_CHARACTER_HOMETOWN_SELECT,
      CON_CHARACTER_DEITY_SELECT,
  };
#if WEB_ONBOARDING_ENABLE_V2
  const char *expected[] = {
      "\"id\":\"age/adult\",\"label\":\"adult\",\"wireValue\":\"1\"",
      "\"id\":\"region/1\",\"label\":\"Ashenport\",\"wireValue\":\"1\"",
      "\"id\":\"faction/0\",\"label\":\"Adventurer / No faction\",\"wireValue\":\"0\"",
      "\"id\":\"hometown/1\",\"label\":\"Ashenport\",\"wireValue\":\"1\"",
      "\"id\":\"deity/0\",\"label\":\"None\",\"wireValue\":\"0\"",
  };
#endif
  size_t index = 0;

  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, states[0]));
  if (d.pProtocol == NULL)
    return;
  assign_deities();

  for (index = 0; index < sizeof(states) / sizeof(states[0]); index++)
  {
    bool built;

    d.connected = states[index];
    built = web_onboarding_build_payload(&d, payload, sizeof(payload));
#if WEB_ONBOARDING_ENABLE_V2
    CuAssertTrue(tc, built);
    CuAssertTrue(tc, json_is_balanced(payload));
    CuAssertTrue(tc, !has_control_bytes(payload));
    CuAssertTrue(tc, strlen(payload) < WEB_ONBOARDING_MAX_PAYLOAD);
    CuAssertPtrNotNull(tc, strstr(payload, expected[index]));
    CuAssertPtrNotNull(tc, strstr(payload, "\"controlWire\":{\"cancel\":\"quit\""));
    CuAssertPtrNotNull(tc, strstr(payload, "\"classic-terminal\""));
#else
    CuAssertTrue(tc, !built);
#endif
  }

  cleanup_editor_descriptor(&d);
}

void TestWebOnboardingRoleplayDetailsIdeasAndControlsAreStateExact(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
  char payload[WEB_ONBOARDING_MAX_PAYLOAD + 1];
#if WEB_ONBOARDING_ENABLE_V2
  char expected_choice[160];
  int background = BACKGROUND_FOLK_HERO;
  int first_background = BACKGROUND_NONE;
  int deity = 0;
#endif

  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_GEN_DESCS_INTRO));
  if (d.pProtocol == NULL)
    return;
  assign_backgrounds();
  assign_deities();
  GET_REAL_RACE(&ch) = RACE_HUMAN;
  d.roleplay_pending.short_description_active = TRUE;
  d.roleplay_pending.short_descriptor_1 = FEATURE_TYPE_EYES;
  d.roleplay_pending.short_adjective_1 = 1;

#if WEB_ONBOARDING_ENABLE_V2
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"controlWire\":{\"continue\":\"1\",\"cancel\":\"0\"}"));
  CuAssertPtrNotNull(tc,
                     strstr(payload, "\"actions\":[\"continue\",\"cancel\",\"classic-terminal\"]"));
  CuAssertPtrNotNull(tc, strstr(payload, "\"help\":"));

  d.forced_short_desc_setup = TRUE;
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"blocking\":true"));
  CuAssertPtrNotNull(tc, strstr(payload, "\"controlWire\":{\"continue\":\"1\"}"));
  CuAssertTrue(tc, strstr(payload, "\"cancel\"") == NULL);

  d.connected = CON_GEN_DESCS_MENU_PARSE;
  d.forced_short_desc_setup = FALSE;
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"controlWire\":{\"confirm\":\"1\",\"reselect\":\"2\","
                                         "\"continue\":\"3\",\"cancel\":\"0\"}"));
  d.roleplay_pending.short_descriptor_2 = FEATURE_TYPE_NOSE;
  d.roleplay_pending.short_adjective_2 = 1;
  d.forced_short_desc_setup = TRUE;
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"controlWire\":{\"confirm\":\"1\",\"reselect\":\"2\"}"));
  CuAssertTrue(tc, strstr(payload, "\"continue\"") == NULL);
  CuAssertTrue(tc, strstr(payload, "\"cancel\"") == NULL);

  d.connected = CON_BACKGROUND_ARCHTYPE_CONFIRM;
  d.roleplay_pending.background_active = TRUE;
  d.roleplay_pending.background = background;
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"detail\":{\"id\":\"folk-hero\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"label\":\"Skill bonuses\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"irreversibleWarning\":"));

  d.connected = CON_QREGION_HELP;
  d.roleplay_pending.region_active = TRUE;
  d.roleplay_pending.region = 1;
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"detail\":{\"id\":\"region/1\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"label\":\"Language\""));

  for (deity = 1; deity < NUM_DEITIES; deity++)
    if (deity_list[deity].pantheon != DEITY_PANTHEON_NONE)
      break;
  CuAssertTrue(tc, deity < NUM_DEITIES);
  d.connected = CON_CHARACTER_DEITY_CONFIRM;
  d.roleplay_pending.deity_active = TRUE;
  d.roleplay_pending.deity = deity;
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  CuAssertPtrNotNull(tc, strstr(payload, "\"detail\":{\"id\":\"deity/"));
  CuAssertPtrNotNull(tc, strstr(payload, "\"label\":\"Alignment\""));

  first_background = backgrounds_listed_alphabetically[1];
  d.connected = CON_CHARACTER_PERSONALITY_IDEAS;
  d.roleplay_pending.example_state = CON_CHARACTER_PERSONALITY_IDEAS;
  d.roleplay_pending.example_count = 2;
  strlcpy(d.roleplay_pending.examples[0], "A patient listener.", ROLEPLAY_EXAMPLE_MAX_BYTES);
  strlcpy(d.roleplay_pending.examples[1], "A restless traveler.", ROLEPLAY_EXAMPLE_MAX_BYTES);
  CuAssertTrue(tc, web_onboarding_build_payload(&d, payload, sizeof(payload)));
  snprintf(expected_choice, sizeof(expected_choice),
           "\"id\":\"%s\",\"label\":\"%s\",\"wireValue\":\"1\"",
           background_stable_id(first_background), background_list[first_background].name);
  CuAssertPtrNotNull(tc, strstr(payload, expected_choice));
  CuAssertPtrNotNull(tc, strstr(payload, "\"examples\":[{\"id\":\"example/0\""));
  CuAssertPtrNotNull(tc, strstr(payload, "\"continue\":\"q\",\"cancel\":\"0\""));
#else
  CuAssertTrue(tc, !web_onboarding_build_payload(&d, payload, sizeof(payload)));
#endif

  cleanup_editor_descriptor(&d);
}

void TestRoleplaySelectionCommitsAreAtomicAcrossCharacterFields(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct char_data *character = &ch;
  struct player_special_data specials;
  enum roleplay_commit_result short_failure;
  enum roleplay_commit_result short_success;
  enum roleplay_commit_result background_failure;
  enum roleplay_commit_result background_success;
  enum roleplay_commit_result age_failure;
  enum roleplay_commit_result age_success;
  enum roleplay_commit_result region_failure;
  enum roleplay_commit_result region_success;
  enum roleplay_commit_result hometown_failure;
  enum roleplay_commit_result hometown_success;
  enum roleplay_commit_result deity_failure;
  enum roleplay_commit_result deity_success;
  int feat = 0;
  int deity = 0;

  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_CHAR_RP_MENU));
  if (d.pProtocol == NULL)
    return;
  assign_backgrounds();
  assign_deities();
  GET_REAL_RACE(character) = RACE_HUMAN;

  GET_PC_DESCRIPTOR_1(character) = FEATURE_TYPE_NOSE;
  GET_PC_ADJECTIVE_1(character) = 2;
  d.roleplay_pending.short_description_active = TRUE;
  d.roleplay_pending.short_descriptor_1 = FEATURE_TYPE_EYES;
  d.roleplay_pending.short_adjective_1 = 1;
  editor_test_save_result = FALSE;
  short_failure = roleplay_commit_short_description(&d);
  CuAssertIntEquals(tc, FEATURE_TYPE_NOSE, GET_PC_DESCRIPTOR_1(character));
  CuAssertIntEquals(tc, 2, GET_PC_ADJECTIVE_1(character));
  editor_test_save_result = TRUE;
  short_success = roleplay_commit_short_description(&d);
  CuAssertIntEquals(tc, FEATURE_TYPE_EYES, GET_PC_DESCRIPTOR_1(character));
  CuAssertIntEquals(tc, 1, GET_PC_ADJECTIVE_1(character));

  feat = background_list[BACKGROUND_FOLK_HERO].feat;
  d.roleplay_pending.background_active = TRUE;
  d.roleplay_pending.background = BACKGROUND_FOLK_HERO;
  editor_test_save_result = FALSE;
  background_failure = roleplay_commit_background(&d);
  CuAssertIntEquals(tc, BACKGROUND_NONE, GET_BACKGROUND(character));
  CuAssertIntEquals(tc, 0, HAS_FEAT(character, feat));
  editor_test_save_result = TRUE;
  background_success = roleplay_commit_background(&d);
  CuAssertIntEquals(tc, BACKGROUND_FOLK_HERO, GET_BACKGROUND(character));
  CuAssertTrue(tc, HAS_FEAT(character, feat));

  editor_test_save_result = FALSE;
  age_failure = roleplay_commit_age(&d, CHARACTER_AGE_MIDDLE_AGED);
  CuAssertTrue(tc, !specials.saved.character_age_saved);
  editor_test_save_result = TRUE;
  age_success = roleplay_commit_age(&d, CHARACTER_AGE_MIDDLE_AGED);
  CuAssertTrue(tc, specials.saved.character_age_saved);
  CuAssertIntEquals(tc, CHARACTER_AGE_MIDDLE_AGED, specials.saved.character_age);

  d.roleplay_pending.region_active = TRUE;
  d.roleplay_pending.region = 1;
  editor_test_save_result = FALSE;
  region_failure = roleplay_commit_region(&d);
  CuAssertIntEquals(tc, REGION_NONE, GET_REGION(character));
  editor_test_save_result = TRUE;
  region_success = roleplay_commit_region(&d);
  CuAssertIntEquals(tc, 1, GET_REGION(character));

  editor_test_save_result = FALSE;
  hometown_failure = roleplay_commit_hometown(&d, 1);
  CuAssertIntEquals(tc, 0, GET_HOMETOWN(character));
  editor_test_save_result = TRUE;
  hometown_success = roleplay_commit_hometown(&d, 1);
  CuAssertIntEquals(tc, 1, GET_HOMETOWN(character));

  for (deity = 1; deity < NUM_DEITIES; deity++)
    if (deity_list[deity].pantheon != DEITY_PANTHEON_NONE)
      break;
  d.roleplay_pending.deity_active = TRUE;
  d.roleplay_pending.deity = deity;
  editor_test_save_result = FALSE;
  deity_failure = roleplay_commit_deity(&d);
  CuAssertIntEquals(tc, 0, GET_DEITY(character));
  editor_test_save_result = TRUE;
  deity_success = roleplay_commit_deity(&d);
  CuAssertIntEquals(tc, deity, GET_DEITY(character));

  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_SAVE_FAILED, short_failure);
  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_OK, short_success);
  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_SAVE_FAILED, background_failure);
  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_OK, background_success);
  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_SAVE_FAILED, age_failure);
  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_OK, age_success);
  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_SAVE_FAILED, region_failure);
  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_OK, region_success);
  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_SAVE_FAILED, hometown_failure);
  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_OK, hometown_success);
  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_SAVE_FAILED, deity_failure);
  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_OK, deity_success);
  cleanup_editor_descriptor(&d);
}

void TestRoleplayFactionCommitRollsBackCharacterAndPlayerIndex(CuTest *tc)
{
  struct descriptor_data d;
  struct char_data ch;
  struct player_special_data specials;
  struct player_index_element fixture[1];
  struct player_index_element *saved_player_table = player_table;
  int saved_top_of_p_table = top_of_p_table;
  enum roleplay_commit_result index_failure;
  enum roleplay_commit_result character_failure;
  enum roleplay_commit_result success;
  int clan_after_index_failure = -1;
  int clan_after_character_failure = -1;
  int index_calls_after_character_failure = -1;

  memset(fixture, 0, sizeof(fixture));
  CuAssertTrue(tc, init_editor_descriptor(&d, &ch, &specials, CON_CHAR_RP_MENU));
  if (d.pProtocol == NULL)
    return;
  ch.player.name = "synthetic-roleplay-character";
  fixture[0].name = "synthetic-roleplay-character";
  fixture[0].clan = 17;
  player_table = fixture;
  top_of_p_table = 0;

  editor_test_index_save_result = FALSE;
  index_failure = roleplay_commit_faction(&d, 0);
  clan_after_index_failure = fixture[0].clan;

  editor_test_index_save_result = TRUE;
  editor_test_save_result = FALSE;
  character_failure = roleplay_commit_faction(&d, 0);
  clan_after_character_failure = fixture[0].clan;
  index_calls_after_character_failure = editor_test_index_save_calls;

  editor_test_save_result = TRUE;
  success = roleplay_commit_faction(&d, 0);

  player_table = saved_player_table;
  top_of_p_table = saved_top_of_p_table;

  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_INDEX_SAVE_FAILED, index_failure);
  CuAssertIntEquals(tc, 17, clan_after_index_failure);
  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_SAVE_FAILED, character_failure);
  CuAssertIntEquals(tc, 17, clan_after_character_failure);
  CuAssertIntEquals(tc, 3, index_calls_after_character_failure);
  CuAssertIntEquals(tc, ROLEPLAY_COMMIT_OK, success);
  CuAssertIntEquals(tc, 0, fixture[0].clan);
  cleanup_editor_descriptor(&d);
}
