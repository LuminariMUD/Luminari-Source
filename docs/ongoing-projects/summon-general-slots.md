# Ordinary summons beyond the first use general slots: plan

Tracking issue: #208. Written 2026-09-20 from a trace of `master` at
`e4897c4b8267f92a00066579664f8ec7ad83dced`; line
numbers refer to that revision. Branch: `fix/208-summon-general-slots`.

Status: complete (2026-09-20), PR #209 open. All five steps done; transcript in `docs/testing/pet-summon-general-slots-2026-09-20.txt`. Review follow-up (same day) recorded below.

## Progress

Update this list with every commit, so a new session can resume from it.

- [x] Step 1: regression tests that fail on `master` (5 of 29 `Test_pet_` cases failed before step 2).
- [x] Step 2: admission rule in `src/core/utils.c` (`summon_dedicated`, pool checks, `follower_uses_general_pool()`).
- [x] Step 3: denial reason `Summon: general slots N/N used` and PETS line `Ordinary summons: D/T dedicated, G in general slots.`
- [x] Step 4: help text. Dev DB rows `charmee` and `pets` updated (archived to `help_versions` first), `help.hlp` regenerated from the dev catalog; audit shows both dev layers matching. Production help is untouched.
- [x] Step 5: verification. Full suite OK (1682 tests) after restating the juggernaut e2e denial (`Test_gameplay_juggernaut_failed_publication_retains_daily_use` locked in the old cap); `make install` clean; live check on 4100 in a private network namespace as Kohdee with Charisma 14, recorded in `docs/testing/pet-summon-general-slots-2026-09-20.txt`.

## The defect

The game tells players one rule and enforces another.

What `pets` prints (`src/core/utils.c:1543-1547`) and what the CHARMEES help says
(`lib/text/help/help.hlp:5586-5588`):

> The first ordinary summon has its own slot; additional ones use general slots.

What the code does. `follower_category_limit()` (`src/core/utils.c:1287-1299`) gives the
ordinary-summon category a limit of 1, or 2 for a Summoner. `follower_category_available()`
(`src/core/utils.c:1301-1310`) checks that limit first and returns `false` before it ever looks at
general slots:

```c
if (counts->categories[category] >= follower_category_limit(ch, category))
  return false;
if (category == FOLLOWER_GENERAL ||
    (category == FOLLOWER_SUMMON && counts->categories[FOLLOWER_SUMMON] > 0))
  return counts->general_used < counts->general_limit;
```

`follower_admit()` (`src/core/utils.c:1327-1341`) repeats the same cap:
`counts->categories[category] + cost > follower_category_limit(ch, category)`.

Result for every non-Summoner: one ordinary summon, ever, no matter how many general slots are
free. The second cast of summon creature, nature's ally, or any other spell whose mob
`isSummonMob()` (`src/magic/magic.c:12517`) returns true is refused with "You cannot control that
summon group; use 'pets' to check your limits." (`src/magic/magic.c:13230-13234`), and `pets` then
shows "General slots: 0/5 used, 5 available. Ordinary summons: 1/1." That display is what
confused the player who reported this.

For a Summoner the general-slot branch is reachable exactly once: the second summon needs both cap
room and a free general slot, and a third is always refused.

History. The hard cap of 1 (2 for Summoner) predates the pet refactor: before #140
(`df61873bf`) `can_add_follower()` used `summons_allowed = 1` plus one for `IS_SUMMONER`. The
refactor kept that cap and added the "additional ones use general slots" wording, which was never
true for a non-Summoner. `Test_pet_policy_second_summon_requires_a_spare_general_slot`
(`unittests/CuTest/test_pet_policy.c:255-282`) and the Summoner branch of
`Test_pet_admission_uses_existing_categories_without_materialization`
(`unittests/CuTest/test_pet_policy.c:66-71`) lock in the cap. Both change in step 1.

## The rule after the fix

The owner chose to make the code match the message. The rule, in full:

1. Every character has dedicated ordinary-summon slots: 1, or 2 for a Summoner. A summon in a
   dedicated slot costs nothing else.
2. Every ordinary summon beyond the dedicated slots occupies one general slot. General slots
   stay `1 + max(0, Charisma bonus)` and are shared with the General category (charmed mobs and
   any pet that matches no named category).
3. There is no other cap on ordinary summons. When the dedicated slots and the general slots are
   all full, the next ordinary summon is refused with a reason that names general slots.
4. Nothing changes for any other category. Elementals, genies, companions, familiars, mercenaries,
   golems, animated dead, shambler groups, wargs, orc hordes, and the artifact exceptions keep
   their own limits and never touch general slots.
5. The same accounting applies everywhere admission is decided: live casts
   (`can_add_summoned_followers()`, `can_add_follower()`), charm and quest followers
   (`can_add_follower_mobile()`), and the login and copyover restore path
   (`select_restorable_followers()`, `src/core/utils.c:1376-1400`). All of them already go through
   `follower_admit()` or `follower_category_available()`, so the change lands in one place.

Worked example, a wizard with a +4 Charisma bonus (5 general slots):

| Cast | Category | Result | Display afterwards |
| -- | -- | -- | -- |
| summon creature: dire tiger | Summon | dedicated slot | General 0/5, summons 1 (1 dedicated) |
| summon creature vii: air elemental | Elemental | own category, cap 1 | General 0/5 |
| summon creature: hell hound | Summon | general slot | General 1/5, summons 2 (1 dedicated, 1 general) |
| charm person: guard | General | general slot | General 2/5 |
| summon creature x3 | Summon | general slots | General 5/5 |
| summon creature | Summon | refused: "Summon: general slots 5/5 used" | unchanged |
| summon creature vii: earth elemental | Elemental | refused: "Elemental: 1/1 used" | unchanged |

## Ablation

Applied before implementation. Removed from the plan:

- No new configuration knob for the number of dedicated summon slots. It is a class fact
  (`IS_SUMMONER`), and the existing code already encodes it.
- No change to any other category, to `follower_control_cost()`, or to the display of
  animated-undead points.
- No rewrite of the CHARMEES help. Its rule sentence is already correct once the code is fixed;
  it only needs the Summoner dedicated-slot count.
- No new e2e boot test. The unit fixture in `test_pet_policy.c` already exercises
  `can_add_follower()` and `check_npc_followers()` without a world, and `mag_summons()` calls
  `can_add_summoned_followers()` which calls `can_add_follower()` for ordinary summons
  (`src/core/utils.c:1488`), so the unit path is the production path.
- The `docs/testing/pet-phase1-*.txt` transcripts are captured output from 2026-09-09 and are
  left as they are.

## Steps

### Step 1: regression tests that fail on `master`

File: `unittests/CuTest/test_pet_policy.c`. Use `begin_pet_policy_fixture()`
(`unittests/CuTest/test_pet_policy.c:129`), whose prototypes include `MOB_DIRE_WOLF` (an
`isSummonMob()` vnum) and `RETAINER_MOB_VNUM` (General).

Rewrite `Test_pet_policy_second_summon_requires_a_spare_general_slot` (line 255) so it states the
new rule for a non-Summoner, and add one Summoner test:

- `Test_pet_policy_extra_summons_consume_general_slots` (non-Summoner). Charisma 10, one dire
  wolf following: a second dire wolf is refused and `NPC_MODE_SPARE` is 0. Charisma 14 gives a
  bonus of 2 (`compute_charisma_bonus()`, `src/core/utils.c:529-537`), so three general slots:
  the second dire wolf is admitted, `NPC_MODE_SPARE` is 3 before it and 2 after it. Add dire
  wolves until `NPC_MODE_SPARE` is 0, then the next dire wolf is refused. A retainer is also
  refused at that point, and removing one summon admits the retainer again, which proves the
  two categories share one pool.
- `Test_pet_policy_summoner_has_two_dedicated_summon_slots`. `CLASS_LEVEL(owner, CLASS_SUMMONER) = 1`, Charisma 10: two dire wolves are admitted with `NPC_MODE_SPARE` still 1, a third dire wolf
  is admitted and `NPC_MODE_SPARE` becomes 0, a fourth is refused.
- The same test's `duplicate` check also asserted a non-Summoner's second dire badger is refused;
  it now asserts admission into the free general slot.
- In `Test_pet_admission_uses_existing_categories_without_materialization` (line 66-71) the
  Summoner branch currently ends with two summons and asserts a third is refused. Under the new
  rule the third summon is admitted (two dedicated, Charisma 10 gives one general slot). Change
  that assertion to expect admission, then add a third summon to the follower list and assert
  the fourth is refused.
- Extend `Test_pet_policy_staged_selection_is_deterministic_and_explains_denials` (line 404) with
  a staged dire wolf that is refused when general slots are full, and assert the reason text is
  `Summon: general slots N/N used` (exact string from step 3).
- Extend `Test_pet_policy_display_handles_missing_rooms_and_reports_real_capacity` (line 284) to
  assert the new summons line from step 3. Keep its existing `General slots: 1/3 used, 2 available` assertion unchanged.

Build and run: `make -j$(nproc) cutest && CUTEST_FILTER=Test_pet_ ./cutest`. The new and changed
tests must fail on `master` before step 2 and pass after it.

### Step 2: admission rule in `src/core/utils.c`

All edits are in the follower block, `src/core/utils.c:1124-1400`.

- Add `int summon_dedicated;` to `struct follower_count_data` (line 1175). Set it in
  `count_followers()` next to `general_limit` (line 1268): `IS_SUMMONER(ch) ? 2 : 1`. Nowhere
  else computes it.
- `follower_recount_general()` (line 1251): `general_used = categories[FOLLOWER_GENERAL] + MAX(0, categories[FOLLOWER_SUMMON] - summon_dedicated)`. Update its comment.
- `follower_category_limit()` (line 1287): remove the `FOLLOWER_SUMMON` branch and the
  `FOLLOWER_GENERAL` branch. After this step the function is only called for the fixed-cap
  categories; General and Summon are decided entirely by the pool checks below, so the function
  must not be asked about them. Add a one-line comment saying so.
- `follower_category_available()` (line 1301):
  - General: `general_used < general_limit`.
  - Summon: `categories[FOLLOWER_SUMMON] < summon_dedicated || general_used < general_limit`.
  - Everything else: unchanged category cap.
- `follower_admit()` (line 1327): the second condition (`categories + cost > limit`) applies
  only to the fixed-cap categories; for General and Summon the availability check is the whole
  rule. Keep the cost check for the other categories (it is what makes elite undead cost two
  points).
- `can_add_summoned_followers()` (line 1447): no change. Ordinary summons reach
  `can_add_follower()` at line 1488, which now applies the new rule.
- `check_npc_followers()` `NPC_MODE_SPARE` (line 1508): no change; `spare` already derives from
  `general_used`.

No signature changes. `IS_SUMMONER` is `src/core/utils.h:2366` and already used in this file.

### Step 3: denial reason and PETS display

- `follower_denial_reason()` (`src/core/utils.c:1344-1358`): for Summon, name general slots when
  the dedicated slots are full, which is the only way a summon is now refused:
  `Summon: general slots %d/%d used`. Remove the now-meaningless comparison against
  `follower_category_limit(ch, FOLLOWER_SUMMON)`.

- Display (`src/core/utils.c:1541-1548`). Replace the two summons sentences with lines that show
  where each summon is counted. Keep the `General slots: %d/%d used, %d available.` fragment
  byte-identical, the display test asserts it. Proposed:

  ```
  3 pets. General slots: 1/5 used, 4 available.
  Ordinary summons: 2 (1 in a dedicated slot, 1 in general slots). Summoners have 2 dedicated.
  General slots are shared by charmed followers and ordinary summons beyond your dedicated
  slots. Other categories have their own limits and never use general slots.
  ```

  Implemented as the character's own dedicated count on one line:
  `Ordinary summons: 1/1 dedicated, 1 in general slots.` followed by the two explanatory
  sentences. No conditional Summoner note.

- The refusal message in `mag_summons()` (`src/magic/magic.c:13232`) stays; `pets` now explains
  the limit correctly.

### Step 4: help text

Both places, per `AGENTS.md`: the development help database and `lib/text/help/help.hlp`.

- CHARMEES (`lib/text/help/help.hlp:5578-5602`). Replace the two sentences at lines 5586-5588
  with: "Your first ordinary summon has a dedicated slot (a Summoner has two). Every ordinary
  summon after that uses one general slot, so with free general slots you can keep summoning.
  Companion bonds, mercenaries, golems, genies, elementals, and special summons use their own
  limits and never use general slots."
- PETS (`lib/text/help/help.hlp:24720-24721`, "Limits depend on follower type..."): add "Ordinary
  summons beyond your dedicated slot use general slots; see CHARMEE."
- SUMMON-CHOICES (`lib/text/help/help.hlp:35683`): "Repeated casts cannot bypass the control
  limits." is still true. Leave it.
- Database: update the same entries with the help-sync skill's bounded workflow, then regenerate
  `help.hlp` from the catalog the way the last two "Regenerate help.hlp" commits on `master` did.
  Read only `APP_ENV` from `lib/.env` first; this plan targets the development database only.
  Production help publication is a separate, owner-scoped step.

### Step 5: verification

1. `make -j$(nproc) test && make install`. Full suite, no filter; confirm no root-level
   `luminari` binary remains.
2. `CUTEST_FILTER=Test_pet_ ./cutest` and `CUTEST_FILTER=Test_necromancer ./cutest` (the
   necromancer tests call `can_add_follower_by_flag()` for animated dead, which must be
   unaffected).
3. `pre-commit run --files` on every changed file.
4. Live check on the development server (port 4100, `MUD_PORT=4100 ./scripts/autorun/autorun.sh`
   after `make install`, or a copyover): as a non-Summoner caster with a positive Charisma bonus,
   cast an ordinary summon twice, run `pets` after each, confirm the second is admitted and the
   general count rises by one; cast until refused and confirm the refusal count matches `pets`;
   charm a mob at that point and confirm it is refused for the same reason; `dismiss` one summon
   and confirm the charm then succeeds. Log out and back in with two summons following: ordinary
   summons are session-only (`pet_lifetime_status`), so they must be gone, and a saved charmed
   follower must restore under the same rule. Record the transcript under `docs/testing/` with
   today's date, matching the existing `pet-phase1-*.txt` files.
5. Confirm `help charmees` and `help pets` in game show the step 4 text.

## Files touched

| File | Change |
| -- | -- |
| `src/core/utils.c` | steps 2 and 3 |
| `unittests/CuTest/test_pet_policy.c` | step 1 |
| `lib/text/help/help.hlp` | step 4, regenerated from the catalog |
| development help database | step 4 |
| `unittests/CuTest/test_gameplay_e2e.c` | step 5: juggernaut test expects the second shambler in the general slot |
| `docs/testing/pet-summon-general-slots-2026-09-20.txt` | step 5 transcript |

`Makefile.am` and `CMakeLists.txt` are not touched: no source file is added or removed.

## Decisions taken in this plan

- Summoners keep an advantage as a second dedicated slot rather than a larger general pool, so
  their benefit does not depend on Charisma.
- General slots remain one pool shared by charmed followers and overflow summons. Making them
  separate pools would be a rule change beyond the reported defect.
- The refusal message from `mag_summons()` is unchanged. It already points at `pets`, and after
  step 3 `pets` gives the real reason.

## Review follow-up (2026-09-20)

The PR #209 review raised four points; all are in the branch.

1. Owner class lookups (`IS_SUMMONER`, Necromancer) went straight to `player_specials`. They now
   go through `follower_owner_class_level()`, which returns 0 for an NPC owner or a missing
   `player_specials`, so the mob-caster checks in `src/mob/mob_spells.c` never read player data.
2. A wild creature charmed in the world that shares its prototype with a summon spell (dire wolf,
   dire bear, hound, and the rest of `isSummonMob()`) was classified as an ordinary summon and
   took the dedicated summon slot, in play and on restore. `follower_category()` now treats it
   as a General follower: only a prototype query (pre-spawn check) or a mobile that records its
   summoning spell is a summon. This is the same distinction `pet_lifetime_kind()` already made.
3. `follower_category_limit()` returned 1 for General and Summon after the refactor. It reports
   the general-slot pool and the dedicated summon slots again, and `count_followers()` reads both
   from it so there is one source for each number.
4. The unrelated craft-training notes commit was dropped from the branch.

Tests: `Test_pet_policy_charmed_wild_animal_is_a_general_follower` and
`Test_pet_policy_npc_owner_uses_base_allowances_without_player_specials`; the existing
dire-wolf fixtures now record their summoning spell, as live summons do.
