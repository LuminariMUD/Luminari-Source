#include "CuTest.h"

#include "../../src/conf.h"
#include "../../src/sysdep.h"
#include "../../src/structs.h"
#include "../../src/utils.h"
#include "../../src/db.h"
#include "../../src/player_rename.h"

#include <fcntl.h>
#include <unistd.h>

static int write_fixture_file(char *path, size_t path_size, const char *contents)
{
  int fd;
  int failed;
  FILE *file;

  strlcpy(path, "/tmp/luminari-rename-unit.XXXXXX", path_size);
  fd = mkstemp(path);
  if (fd < 0)
    return FALSE;
  file = fdopen(fd, "w");
  if (!file)
  {
    close(fd);
    unlink(path);
    return FALSE;
  }
  failed = fputs(contents, file) == EOF;
  if (fclose(file) != 0)
    failed = TRUE;
  if (failed)
  {
    unlink(path);
    return FALSE;
  }
  return TRUE;
}

static int read_fixture_file(const char *path, char *contents, size_t contents_size)
{
  FILE *file;
  size_t bytes;
  int failed;

  file = fopen(path, "r");
  if (!file)
    return FALSE;
  bytes = fread(contents, 1, contents_size - 1, file);
  contents[bytes] = '\0';
  failed = ferror(file);
  if (fclose(file) != 0)
    failed = TRUE;
  if (failed)
    return FALSE;
  return TRUE;
}

void Test_player_rename_validates_and_canonicalizes_name(CuTest *tc)
{
  char display[MAX_NAME_LENGTH + 1];
  char index[MAX_NAME_LENGTH + 1];
  enum player_rename_status status;

  status = player_rename_validate_name_for_test("Sourcechar", "targetchar", display,
                                                sizeof(display), index, sizeof(index));
  CuAssertIntEquals(tc, PLAYER_RENAME_OK, status);
  CuAssertStrEquals(tc, "Targetchar", display);
  CuAssertStrEquals(tc, "targetchar", index);
}

void Test_player_rename_rejects_invalid_and_case_only_names(CuTest *tc)
{
  enum player_rename_status status;
  char too_long[MAX_NAME_LENGTH + 2];

  status = player_rename_validate_name_for_test("Sourcechar", "Sourcechar", NULL, 0, NULL, 0);
  CuAssertIntEquals(tc, PLAYER_RENAME_INVALID_NAME, status);

  status = player_rename_validate_name_for_test("Sourcechar", "sOURCECHAR", NULL, 0, NULL, 0);
  CuAssertIntEquals(tc, PLAYER_RENAME_INVALID_NAME, status);

  status = player_rename_validate_name_for_test("Sourcechar", "a", NULL, 0, NULL, 0);
  CuAssertIntEquals(tc, PLAYER_RENAME_INVALID_NAME, status);

  memset(too_long, 'a', sizeof(too_long) - 1);
  too_long[sizeof(too_long) - 1] = '\0';
  status = player_rename_validate_name_for_test("Sourcechar", too_long, NULL, 0, NULL, 0);
  CuAssertIntEquals(tc, PLAYER_RENAME_INVALID_NAME, status);

  status = player_rename_validate_name_for_test("Sourcechar", "all", NULL, 0, NULL, 0);
  CuAssertIntEquals(tc, PLAYER_RENAME_INVALID_NAME, status);

  status = player_rename_validate_name_for_test("Sourcechar", "the", NULL, 0, NULL, 0);
  CuAssertIntEquals(tc, PLAYER_RENAME_INVALID_NAME, status);

  status = player_rename_validate_name_for_test("Sourcechar", "Invalid1", NULL, 0, NULL, 0);
  CuAssertIntEquals(tc, PLAYER_RENAME_INVALID_NAME, status);

  status = player_rename_validate_name_for_test("Sourcechar", "\xc3\xa9name", NULL, 0, NULL, 0);
  CuAssertIntEquals(tc, PLAYER_RENAME_INVALID_NAME, status);

  status = player_rename_validate_name_for_test("Sourcechar", "bcdfg", NULL, 0, NULL, 0);
  CuAssertIntEquals(tc, PLAYER_RENAME_INVALID_NAME, status);
}

void Test_player_rename_paths_cross_shards(CuTest *tc)
{
  char old_path[MAX_FILEPATH];
  char new_path[MAX_FILEPATH];

  CuAssertTrue(tc, get_filename(old_path, sizeof(old_path), PLR_FILE, "Sourcechar"));
  CuAssertTrue(tc, get_filename(new_path, sizeof(new_path), PLR_FILE, "Targetchar"));
  CuAssertTrue(tc, strstr(old_path, "P-T/sourcechar.plr") != NULL);
  CuAssertTrue(tc, strstr(new_path, "P-T/targetchar.plr") != NULL);

  CuAssertTrue(tc, get_filename(old_path, sizeof(old_path), PLR_FILE, "Earlychar"));
  CuAssertTrue(tc, get_filename(new_path, sizeof(new_path), PLR_FILE, "Laterchar"));
  CuAssertTrue(tc, strstr(old_path, "A-E/earlychar.plr") != NULL);
  CuAssertTrue(tc, strstr(new_path, "K-O/laterchar.plr") != NULL);
}

void Test_player_rename_memory_preflight_is_bounded_and_case_insensitive(CuTest *tc)
{
  struct player_index_element fixture[2];
  struct player_index_element *saved_table = player_table;
  struct char_data *saved_character_list = character_list;
  int saved_top = top_of_p_table;
  struct char_data victim;
  struct char_data live_target;
  struct player_special_data live_specials;
  enum player_rename_status index_collision_status;
  enum player_rename_status live_collision_status;
  enum player_rename_status missing_status;
  enum player_rename_status duplicate_source_status;

  memset(fixture, 0, sizeof(fixture));
  memset(&victim, 0, sizeof(victim));
  memset(&live_target, 0, sizeof(live_target));
  memset(&live_specials, 0, sizeof(live_specials));
  fixture[0].id = 101;
  fixture[0].name = "sourcechar";
  fixture[1].id = 202;
  fixture[1].name = "TARGETCHAR";
  player_table = fixture;
  top_of_p_table = 1;
  character_list = NULL;
  victim.char_specials.saved.idnum = 101;

  index_collision_status =
      player_rename_memory_preflight_for_test(&victim, "Sourcechar", "Targetchar");

  fixture[1].name = "otherchar";
  live_target.player.name = "TARGETCHAR";
  live_target.player_specials = &live_specials;
  character_list = &live_target;
  live_collision_status =
      player_rename_memory_preflight_for_test(&victim, "Sourcechar", "Targetchar");

  victim.char_specials.saved.idnum = 303;
  character_list = NULL;
  missing_status = player_rename_memory_preflight_for_test(&victim, "Sourcechar", "Unusedchar");

  victim.char_specials.saved.idnum = 101;
  fixture[1].id = 101;
  fixture[1].name = "sourcechar";
  duplicate_source_status =
      player_rename_memory_preflight_for_test(&victim, "Sourcechar", "Unusedchar");

  player_table = saved_table;
  top_of_p_table = saved_top;
  character_list = saved_character_list;

  CuAssertIntEquals(tc, PLAYER_RENAME_NAME_EXISTS, index_collision_status);
  CuAssertIntEquals(tc, PLAYER_RENAME_NAME_EXISTS, live_collision_status);
  CuAssertIntEquals(tc, PLAYER_RENAME_PLAYER_NOT_FOUND, missing_status);
  CuAssertIntEquals(tc, PLAYER_RENAME_POSTCONDITION_FAILED, duplicate_source_status);
}

void Test_player_rename_rewrites_only_identity_and_introductions(CuTest *tc)
{
  char path[MAX_FILEPATH];
  char contents[4096];
  unsigned int intro_changes = 0;

  CuAssertTrue(tc, write_fixture_file(path, sizeof(path),
                                      "Name: Sourcechar\n"
                                      "Pass: password-hash\n"
                                      "Acct: StaleAccount\n"
                                      "Todo:\n"
                                      "Remember ~ is ordinary todo text here.\n"
                                      "Name: Sourcechar is still todo text.\n"
                                      "~\n"
                                      "Desc: ignored tag value\n"
                                      "Name: Sourcechar is part of authored text.\n"
                                      "Acct: AuthoredAccount is also text.\n"
                                      "Intr:\n"
                                      "Sourcechar\n"
                                      "~\n"
                                      "Intr:\n"
                                      "Sourcechar\n"
                                      "Otherchar\n"
                                      "~\n"
                                      "Desc:\n"
                                      "Sourcechar remains in authored text.~\n"
                                      "Goal:\n"
                                      "Name: Sourcechar remains authored.~\n"));

  CuAssertTrue(tc, player_rename_rewrite_file_for_test(path, "Sourcechar", "Targetchar",
                                                       "FixtureAccount", TRUE, &intro_changes));
  CuAssertIntEquals(tc, 1, (int)intro_changes);
  CuAssertTrue(tc, read_fixture_file(path, contents, sizeof(contents)));
  CuAssertTrue(tc, strstr(contents, "Name: Targetchar\n") != NULL);
  CuAssertTrue(tc, strstr(contents, "Acct: FixtureAccount\n") != NULL);
  CuAssertTrue(tc, strstr(contents, "Todo:\n"
                                    "Remember ~ is ordinary todo text here.\n"
                                    "Name: Sourcechar is still todo text.\n"
                                    "~\n") != NULL);
  CuAssertTrue(tc, strstr(contents, "Intr:\nTargetchar\nOtherchar\n~\n") != NULL);
  CuAssertTrue(tc, strstr(contents, "Desc: ignored tag value\n"
                                    "Name: Sourcechar is part of authored text.\n"
                                    "Acct: AuthoredAccount is also text.\n"
                                    "Intr:\n"
                                    "Sourcechar\n"
                                    "~\n") != NULL);
  CuAssertTrue(tc, strstr(contents, "Sourcechar remains in authored text.") != NULL);
  CuAssertTrue(tc, strstr(contents, "Goal:\nName: Sourcechar remains authored.~\n") != NULL);

  unlink(path);
}

void Test_player_rename_adds_missing_durable_account_name(CuTest *tc)
{
  char path[MAX_FILEPATH];
  char contents[2048];

  CuAssertTrue(tc, write_fixture_file(path, sizeof(path),
                                      "Name: Sourcechar\n"
                                      "Pass: password-hash\n"));
  CuAssertTrue(tc, player_rename_rewrite_file_for_test(path, "Sourcechar", "Targetchar",
                                                       "FixtureAccount", TRUE, NULL));
  CuAssertTrue(tc, read_fixture_file(path, contents, sizeof(contents)));
  CuAssertTrue(tc, strstr(contents, "Name: Targetchar\n") != NULL);
  CuAssertTrue(tc, strstr(contents, "Acct: FixtureAccount\n") != NULL);

  unlink(path);
}

void Test_player_rename_accepts_legacy_numeric_introductions(CuTest *tc)
{
  char path[MAX_FILEPATH];
  char contents[2048];
  unsigned int intro_changes = 0;

  CuAssertTrue(tc, write_fixture_file(path, sizeof(path),
                                      "Name: Sourcechar\n"
                                      "Id  : 101\n"
                                      "Intr:\n"
                                      "5001\n"
                                      "5002\n"
                                      "-1\n"
                                      "Intr:\n"
                                      "Sourcechar\n"
                                      "~\n"));
  CuAssertTrue(tc, player_rename_file_identity_matches_for_test(path, "Sourcechar", 101));
  CuAssertTrue(tc, player_rename_rewrite_file_for_test(path, "Sourcechar", "Targetchar", NULL, TRUE,
                                                       &intro_changes));
  CuAssertIntEquals(tc, 1, (int)intro_changes);
  CuAssertTrue(tc, read_fixture_file(path, contents, sizeof(contents)));
  CuAssertTrue(tc, strstr(contents, "Intr:\n5001\n5002\n-1\n") != NULL);
  CuAssertTrue(tc, strstr(contents, "Intr:\nTargetchar\n~\n") != NULL);

  unlink(path);
}

void Test_player_rename_ignores_identity_tags_in_nested_records(CuTest *tc)
{
  char path[MAX_FILEPATH];
  char contents[8192];
  unsigned int intro_changes = 0;

  CuAssertTrue(tc, write_fixture_file(path, sizeof(path),
                                      "Name: Sourcechar\n"
                                      "Id  : 101\n"
                                      "Acct: StaleAccount\n"
                                      "Alis: 1\n"
                                      "Name: Sourcechar\n"
                                      "Acct: Alias replacement text\n"
                                      "1\n"
                                      "Vars: 3\n"
                                      "Name: Sourcechar\n"
                                      "Acct: Variable text\n"
                                      "Intr:\n"
                                      "Dvis: ignored tag value\n"
                                      "1\n"
                                      "0\n"
                                      "Name: Sourcechar\n"
                                      "Acct: Device short text\n"
                                      "Intr:\n"
                                      "1 10 90 3 0\n"
                                      "101\n"
                                      "-1\n"
                                      "-1\n"
                                      "-1\n"
                                      "Lvls:\n"
                                      "3\n"
                                      "0\n"
                                      "0\n"
                                      "0\n"
                                      "-1\n"
                                      "Intr:\n"
                                      "Sourcechar\n"
                                      "~\n"));

  CuAssertTrue(tc, player_rename_file_identity_matches_for_test(path, "Sourcechar", 101));
  CuAssertTrue(tc, player_rename_rewrite_file_for_test(path, "Sourcechar", "Targetchar",
                                                       "FixtureAccount", TRUE, &intro_changes));
  CuAssertIntEquals(tc, 1, (int)intro_changes);
  CuAssertTrue(tc, read_fixture_file(path, contents, sizeof(contents)));
  CuAssertTrue(tc, strstr(contents, "Name: Targetchar\nId  : 101\n"
                                    "Acct: FixtureAccount\n") != NULL);
  CuAssertTrue(tc, strstr(contents, "Alis: 1\n"
                                    "Name: Sourcechar\n"
                                    "Acct: Alias replacement text\n"
                                    "1\n") != NULL);
  CuAssertTrue(tc, strstr(contents, "Vars: 3\n"
                                    "Name: Sourcechar\n"
                                    "Acct: Variable text\n"
                                    "Intr:\n") != NULL);
  CuAssertTrue(tc, strstr(contents, "Dvis: ignored tag value\n"
                                    "1\n"
                                    "0\n"
                                    "Name: Sourcechar\n"
                                    "Acct: Device short text\n"
                                    "Intr:\n") != NULL);
  CuAssertTrue(tc, strstr(contents, "Intr:\nTargetchar\n~\n") != NULL);

  unlink(path);
}

void Test_unlinked_player_rename_preserves_existing_account_hint(CuTest *tc)
{
  char path[MAX_FILEPATH];
  char contents[2048];

  CuAssertTrue(tc, write_fixture_file(path, sizeof(path),
                                      "Name: Sourcechar\n"
                                      "Acct: LegacyAccountHint\n"
                                      "Pass: password-hash\n"));
  CuAssertTrue(
      tc, player_rename_rewrite_file_for_test(path, "Sourcechar", "Targetchar", NULL, TRUE, NULL));
  CuAssertTrue(tc, read_fixture_file(path, contents, sizeof(contents)));
  CuAssertTrue(tc, strstr(contents, "Name: Targetchar\n") != NULL);
  CuAssertTrue(tc, strstr(contents, "Acct: LegacyAccountHint\n") != NULL);

  unlink(path);
}

void Test_player_rename_rewrite_failure_keeps_original_file(CuTest *tc)
{
  char path[MAX_FILEPATH];
  char contents[2048];
  const char *original = "Name: Sourcechar\nName: Duplicate\nAcct: FixtureAccount\n";

  CuAssertTrue(tc, write_fixture_file(path, sizeof(path), original));
  CuAssertTrue(tc, !player_rename_rewrite_file_for_test(path, "Sourcechar", "Targetchar",
                                                        "FixtureAccount", TRUE, NULL));
  CuAssertTrue(tc, read_fixture_file(path, contents, sizeof(contents)));
  CuAssertStrEquals(tc, original, contents);

  unlink(path);
}

void Test_player_rename_requires_matching_unique_pfile_identity(CuTest *tc)
{
  char path[MAX_FILEPATH];

  CuAssertTrue(tc, write_fixture_file(path, sizeof(path),
                                      "Name: Sourcechar\n"
                                      "Id  : 101\n"
                                      "Desc:\n"
                                      "Name: Authored text\n"
                                      "Id  : 999\n"
                                      "~\n"));
  CuAssertTrue(tc, player_rename_file_identity_matches_for_test(path, "Sourcechar", 101));
  CuAssertTrue(tc, !player_rename_file_identity_matches_for_test(path, "Sourcechar", 202));
  unlink(path);

  CuAssertTrue(tc, write_fixture_file(path, sizeof(path),
                                      "Name: Sourcechar\n"
                                      "Id  : 101\n"
                                      "Id  : 101\n"));
  CuAssertTrue(tc, !player_rename_file_identity_matches_for_test(path, "Sourcechar", 101));
  unlink(path);

  CuAssertTrue(tc, write_fixture_file(path, sizeof(path),
                                      "Name: Sourcechar\n"
                                      "Id  : 101\n"
                                      "Acct: FirstAccount\n"
                                      "Acct: SecondAccount\n"));
  CuAssertTrue(tc, !player_rename_file_identity_matches_for_test(path, "Sourcechar", 101));
  unlink(path);
}

void Test_player_rename_generated_text_uses_name_boundaries(CuTest *tc)
{
  char *text = strdup("pcorpse Sourcechar pcorpse_Sourcechar Sourcecharisma");

  player_rename_replace_generated_name_for_test(&text, "Sourcechar", "Targetchar");
  CuAssertStrEquals(tc, "pcorpse Targetchar pcorpse_Targetchar Sourcecharisma", text);
  free(text);
}

void Test_saved_clone_derives_identity_from_current_owner(CuTest *tc)
{
  struct char_data clone;
  char *authored_long;
  char *authored_description;
  char derived_name[64];
  char derived_short[64];
  int long_unchanged;
  int description_unchanged;
  int applied;

  memset(&clone, 0, sizeof(clone));
  clone.player.name = strdup("Savedoldname");
  clone.player.short_descr = strdup("Saved old short");
  clone.player.long_descr = strdup("Authored long text");
  clone.player.description = strdup("Authored description text");
  authored_long = clone.player.long_descr;
  authored_description = clone.player.description;

  applied = apply_clone_owner_identity_for_test(&clone, "Currentowner");

  strlcpy(derived_name, clone.player.name ? clone.player.name : "", sizeof(derived_name));
  strlcpy(derived_short, clone.player.short_descr ? clone.player.short_descr : "",
          sizeof(derived_short));
  long_unchanged = clone.player.long_descr == authored_long;
  description_unchanged = clone.player.description == authored_description;

  free(clone.player.name);
  free(clone.player.short_descr);
  free(clone.player.long_descr);
  free(clone.player.description);

  CuAssertTrue(tc, applied);
  CuAssertStrEquals(tc, "Currentowner", derived_name);
  CuAssertStrEquals(tc, "Currentowner", derived_short);
  CuAssertTrue(tc, long_unchanged);
  CuAssertTrue(tc, description_unchanged);
}

void Test_saved_clone_does_not_free_shared_prototype_strings(CuTest *tc)
{
  struct char_data prototype;
  struct char_data clone;
  struct char_data *saved_mob_proto = mob_proto;
  mob_rnum saved_top_of_mobt = top_of_mobt;
  int applied;

  memset(&prototype, 0, sizeof(prototype));
  memset(&clone, 0, sizeof(clone));
  prototype.player.name = "shared prototype name";
  prototype.player.short_descr = "shared prototype short";
  mob_proto = &prototype;
  top_of_mobt = 0;
  clone.nr = 0;
  clone.player.name = prototype.player.name;
  clone.player.short_descr = prototype.player.short_descr;

  applied = apply_clone_owner_identity_for_test(&clone, "Currentowner");

  mob_proto = saved_mob_proto;
  top_of_mobt = saved_top_of_mobt;

  CuAssertTrue(tc, applied);
  CuAssertStrEquals(tc, "Currentowner", clone.player.name);
  CuAssertStrEquals(tc, "Currentowner", clone.player.short_descr);
  CuAssertStrEquals(tc, "shared prototype name", prototype.player.name);
  CuAssertStrEquals(tc, "shared prototype short", prototype.player.short_descr);

  free(clone.player.name);
  free(clone.player.short_descr);
}

void Test_descriptorless_account_name_is_preserved_for_save(CuTest *tc)
{
  struct char_data character;
  struct player_special_data player_specials;

  memset(&character, 0, sizeof(character));
  memset(&player_specials, 0, sizeof(player_specials));
  character.player_specials = &player_specials;
  GET_ACCOUNT_NAME(&character) = strdup("FixtureAccount");

  CuAssertStrEquals(tc, "FixtureAccount", player_file_account_name(&character));

  free(GET_ACCOUNT_NAME(&character));
}

void Test_missing_player_specials_has_no_account_name(CuTest *tc)
{
  struct char_data character;

  memset(&character, 0, sizeof(character));

  CuAssertPtrEquals(tc, NULL, (void *)player_file_account_name(&character));
}

void Test_live_account_name_takes_precedence_for_save(CuTest *tc)
{
  struct char_data character;
  struct player_special_data player_specials;
  struct descriptor_data descriptor;
  struct account_data account;

  memset(&character, 0, sizeof(character));
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&account, 0, sizeof(account));
  character.player_specials = &player_specials;
  character.desc = &descriptor;
  descriptor.account = &account;
  account.name = "LiveAccount";
  GET_ACCOUNT_NAME(&character) = strdup("LoadedAccount");

  CuAssertStrEquals(tc, "LiveAccount", player_file_account_name(&character));

  free(GET_ACCOUNT_NAME(&character));
}

void Test_empty_live_account_name_falls_back_to_stored_name(CuTest *tc)
{
  struct char_data character;
  struct player_special_data player_specials;
  struct descriptor_data descriptor;
  struct account_data account;

  memset(&character, 0, sizeof(character));
  memset(&player_specials, 0, sizeof(player_specials));
  memset(&descriptor, 0, sizeof(descriptor));
  memset(&account, 0, sizeof(account));
  character.player_specials = &player_specials;
  character.desc = &descriptor;
  descriptor.account = &account;
  account.name = "";
  GET_ACCOUNT_NAME(&character) = strdup("LoadedAccount");

  CuAssertStrEquals(tc, "LoadedAccount", player_file_account_name(&character));

  free(GET_ACCOUNT_NAME(&character));
}
