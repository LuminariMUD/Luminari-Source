# Strict C23 toolchain: progress and remaining work

Tracking issue: #86. Branch: `strict-c23-toolchain`. Written 2026-09-14 on the
development host (16 cores, WSL2, Ubuntu 24.04).

Goal: a documented two-compiler contract (minimum and current GCC and Clang),
strict builds that are errors on every pull request, a ratchet for the known
warning debt, and feature detection that strict flags cannot influence.

## Completed

### Compilers and CI

- Compiler policy: GCC 13 and Clang 18 minimum (the `ubuntu-latest` runner
  image), GCC 16.2 and Clang 22.1.8 current (the `gcc:16.2` container image
  and apt.llvm.org). Documented with an update cadence in
  `docs/guides/SETUP_AND_BUILD_GUIDE.md`.
- The strict CMake job in `test.yml` is a matrix of all four compilers times
  Debug and Release, blocking, with `-Werror` on the baseline tier. The
  Autotools job configures with `--enable-werror`.
- `scripts/ci/check_compiler.sh` runs in every compiling job. It reads the
  preprocessor's predefined macros to prove the family and version, so
  installing Clang can never silently produce a GCC build. Configure and CMake
  both print the effective warning flags.
- `.github/actions/setup-build` works as root inside a compiler container.
- The local runner (`scripts/ci/local/run.py`) expands matrix `include`
  entries the way GitHub does, supports `container:` jobs through a second
  image (`scripts/ci/local/Dockerfile.gcc-16.2`), and installs Clang 22 in its
  main image.

### Warning tiers

- One flag list in `scripts/deployment/production_profile.sh`, probed per
  compiler and consumed by both `configure.ac` (`--enable-warning-tier`) and
  `CMakeLists.txt` (`LUMINARI_WARNING_TIER`). `DEVELOPER_MODE` is gone.
- Baseline tier: `-Wall -Wextra -Wstrict-prototypes -Wold-style-definition
  -Wpointer-arith -Wformat-security -Wvla -Wredundant-decls -Wnested-externs
  -Wmissing-prototypes -Wjump-misses-init -Wshadow -Wdouble-promotion
  -Wfloat-equal -Wfloat-conversion` plus GCC's `-Wtrampolines
  -Walloc-size -Wbidi-chars=any -Wcalloc-transposed-args
  -Wflex-array-member-not-at-end -Wunterminated-string-initialization`. The
  last eight common flags were promoted from the migration tier by steps 2.3,
  2.4, 3.1, 3.3, and 3.2; Clang 18 does not know `-Wjump-misses-init`, so the probe
  drops it there. Clean on all four
  compilers; `-Werror` is refused with any other tier.
- Migration tier: conversions, switch coverage, `-Wformat=2`,
  allocation, duplicated conditions and branches, logical operators,
  fallthrough, `-Wwrite-strings`. Held by
  `scripts/ci/check_warning_budget.py` against `scripts/ci/warning_budget_gcc-16.txt`
  and `scripts/ci/warning_budget_clang-22.txt`; growth in any class fails the
  new `warning-budget` job. Counting is by distinct site with make output sync,
  which was required to make Clang's numbers deterministic.
- Analysis tier: GCC `-fanalyzer` and Clang's opinionated extras, plus an
  ISO C23 `-Wpedantic` extension report, in the weekly, non-blocking
  `.github/workflows/toolchain-analysis.yml`.

### Feature detection

- `-Werror` is stripped from the caller's CFLAGS for every configure and
  CMake probe and restored afterwards. The `struct in_addr` and `socklen_t`
  probes no longer emit diagnostics. Three unused `-Werror` probes were
  deleted.
- `scripts/ci/check_configure_probes.sh` configures both build systems with
  and without strict flags and fails when `src/conf.h` differs. It caught the
  `socklen_t` fallback and the `AC_CHECK_FUNCS` false negatives.

### Source fixes surfaced by the new compilers and flags

- NULL guards on all 119 special-procedure identify checks: the `SPECIAL`
  wrapper passes a NULL argument on pulse calls, so the unguarded `strcmp` was
  a latent crash that GCC 13 and 16 each proved at different inlining depths.
- An out-of-bounds read in the epic weapon specialization prerequisite: a
  FEAT number was used as a combat-feat array index (GCC 16.2 at -O3).
- A NULL name reaching `strchr` in the shop purchase message when the buyer
  carries nothing (GCC 16.2 at -O3).
- Four variable-length arrays removed (the repository already forbade them).
- An unused loop counter, a `strncpy` truncation, an uninitialized const
  pointer argument and a non-literal format call in tests.
- Both build systems now request an ELF build ID at link time, and the
  production profile probes for the CET property note on the linked image
  instead of trusting flag acceptance. A toolchain built from source without
  `--enable-cet` (the official gcc images) accepts `-fcf-protection` but
  cannot mark the binary; the profile now reports that honestly.

### Verification done locally

- Strict full builds: GCC 13, GCC 16, Clang 18, Clang 22, all zero errors.
- `make test` on the strict Autotools build: 1483 tests pass.
- The local CI matrix (`scripts/ci/local/run.py`) on the final commit.

## Budget snapshot

| Compiler | At the start | Now (after step 3.2) |
|----------|--------------|----------------------|
| GCC 16.2 | 11363 sites, 24 classes | 2824 sites, 14 classes |
| Clang 22.1.8 | 22878 sites, 23 classes | 4263 sites, 12 classes |

Largest remaining classes: sign conversion (GCC 1808, Clang 3331), value
conversion (GCC 533, Clang `implicit-int-conversion` 523), and the
production half of the discarded-qualifier warnings (191 each) with
`cast-qual` (100 each).

## Burn-down progress

Sites after each landed step, measured with the CI budget job's exact CMake
command inside the pinned images (`luminari-ci:local-gcc-16.2` and
`luminari-ci:local-fast` for Clang 22.1.8) on a snapshot of the working tree
with the example config headers. The first local run reproduced both committed
budget files exactly; with ccache a full budget build takes about two minutes
per compiler.

| Step | Change | GCC 16.2 | Clang 22.1.8 |
|------|--------|----------|--------------|
| start | committed budgets | 11363 | 22878 |
| 0 | `--list` and `--by-token`; `-Wswitch-default` dropped | 10706 | 22221 |
| 1.1 | `IS_SET_AR` casts the element before the mask | 10706 | 10578 |
| 2.1 | generated `test_prototypes.h` | 9223 | 9095 |
| 1.2 | `int` affect, ability, point, player and object fields | 8010 | 7955 |
| 2.2, 2.3 | format conversions, redundant and nested declarations | 6427 | 7946 |
| 2.6 | explicit fallthrough; `-Wredundant-decls` and `-Wnested-externs` promoted to baseline | 6427 | 7913 |
| 2.4 | `static` file-local functions, prototypes in owning headers, dead code removed; `-Wmissing-prototypes` promoted to baseline | 5775 | 7262 |
| 2.5 | const-correct test fixtures and five read-only parameters | 5188 | 6675 |
| 1.3a | explicit casts where 64-bit values narrow to `int` outside macros; `oedit` `max_val` is `int` | 4793 | 6278 |
| 1.3b | width-matched `long_min`, `size_min`, `u64_min` and friends where `MIN` and `MAX` truncated their arguments | 4701 | 6183 |
| 1.3c | narrowing inside macros and multi-line expressions; clan return widened; `look_at_room_number` guard fixed; Clang `shorten-64-to-32` at zero | 4535 | 6018 |
| 3.1 | case-local declarations scoped or hoisted; `jump-misses-init` at zero; flag promoted to baseline | 4087 | 5437 |
| 3.3 | 265 shadowing declarations renamed within their scope | 3822 | 5279 |
| 3.3 tail | `REMOVE_FROM_LIST_USING`; last three renames; `shadow` at zero; flag promoted to baseline | 3818 | 5275 |
| 3.2 | `float` is `double`; unused kdtree float API removed; float `MIN`/`MAX` clamps fixed; float-to-int conversions explicit; float classes at zero and promoted to baseline | 2824 | 4263 |

Every step was also verified with a host `make test` (1483 tests pass) before
it was committed, and each promotion to the baseline tier was first built at
the baseline tier with GCC 13 and Clang 18.
Also fixed on the way: the budget check counted only `file:line:col: error:`
lines, so a build that stopped on a missing header (`fatal error:`), a linker
failure, or a make `***` line still reported a trustworthy count. The CI step
pipes the build through `tee` without `pipefail`, so the check is the only
gate that sees such a failure; it now counts all four forms.

Notes from steps 2.2 and 2.3:

- GCC 16.2 emits no usable fix-it for `-Wformat-signedness` (it rewrites `%d`
  as `%d`), so the conversions were applied by a script that reads the
  directive back from the source at the reported column, or at the "format
  string is defined here" note for concatenated and macro-built literals.
  Index typedefs got `PRI_IDX` (or `SCN_IDX`), plain unsigned values `%u`.
- Three object listings in `oasis_list.c` printed the object count through
  `PRI_IDX`; `WILD_DEBUG_MEM` cast only the first operand of its size argument.
- Duplicate declarations were resolved toward the header that matches the
  defining source file; the legacy umbrella headers now include the owners
  (`act.h` includes `movement/movement.h`, `handler.h` the mob memory and
  utility headers, `oasis.h` `mob/mob_autoroll.h`). Block-scope `extern`
  declarations of `conn2` and `conn3` moved to `mysql.h`.
- `strlcat` is now probed like `strlcpy` (`HAVE_STRLCAT`), so the local
  fallback no longer redeclares the C library function on glibc 2.38 and
  later.

Notes from step 2.6:

- Clang does not accept fallthrough comments; intended fallthroughs now carry
  `[[fallthrough]];` and cases that only fell into a `break` got their own.
- `SKILL_DIRT_KICK` in `skill_lists.c` fell into the berserker rage check for
  a level 20, dexterity 17 character without 15 rogue levels, so such a
  berserker could use dirt kick. It now returns FALSE.
- The object editor's special ability value prompts fell through every later
  prompt into the `SYSERR` default case for abilities without a handler; each
  prompt now ends in `break`.
- Before the promotion both flags were built at the baseline tier with GCC 13
  and Clang 18 (the minimum compilers), all 746 objects, zero warnings.

Notes from step 2.4:

- Functions no other file names (comments and string literals ignored) became
  `static`. Cross-file functions got one prototype in the header that matches
  the defining source file, and every local copy in other `.c` files and
  tests was removed, since `-Wredundant-decls` is now an error. Files without
  their own header use the umbrella that already declares their neighbours
  (`act.h` for the `act.*.c` commands, `db.h` for `players.c`, `mudlim.h` for
  `limits.c`, `oasis.h` for the editors).
- The `db.c` test hooks were declared inside `db.h`'s `#ifndef __DB_C__`
  section, so `db.c` itself never saw them; they now sit with the other
  `LUMINARI_CUTEST` hooks.
- `limits.c` and `tactical_effects.c` each define an unrelated `hazard_tick`;
  both are file-local now.
- Obsolete, never-called code was deleted: `do_drink_old`, `do_eat_old`,
  `load_contextual_hints_legacy`, `test_tokenize`, the two spatial audio test
  emitters and the three retired `spatial_visual_meteor_*` emitters (both
  families are forbidden by `test_pubsub_retirement.sh`), the unregistered
  `BoundsCheckingSuite`, and the unused
  `parseadminlevel` and `parselast` copies in `util/rebuildMailIndex.c`.
- Never-called functions that look like unfinished features keep a prototype
  in their header instead of being deleted. They are dead-code candidates for
  a follow-up issue: `char_has_any_item_activation_ability_cooldowns`,
  `do_homelands`, `get_copyover_state_string`, `class_prereq_attribute`,
  `class_prereq_weapon_proficiency`, `process_level_feats`, `sort_evolutions`,
  `feat_prereq_race`, `free_feats`, `get_draconic_heritage_subfeat`,
  `reset_training_points`, `free_single_clan_data`, `do_detectmagic`,
  `has_piercing_weapon`, `can_enable_mode`, `set_encounter_terrain_all_roads`,
  `autoDiagnose`, `send_to_clan`, `mysql_board_get_config_by_room`,
  `alchemist_can_brew_spell`, `consume_brew_materials`, `has_brew_materials`,
  `has_golem_follower`, `assign_feat_spell_slots`, `world_has_mud_event`,
  `after_world_load`, `save_paths`, `save_regions`, `is_house_owner`,
  `assign_weighted_random_bonuses`, `choose_cloth_material`,
  `choose_metal_material`, `choose_precious_metal_material`,
  `perform_mob_name_list`, `compute_ranged_weapon_actual_value`,
  `perform_zone_restat`, `perform_lichdrain`, `clanportal`, `obj_proc_ready`,
  `can_paralyze`, `clear_group_marks`, `display_dam_type`,
  `get_direction_vnum`, `is_exit_locked`, `is_ghost`, `is_swimming`,
  `spell_level_ch`, `init_vessel_db`, `clear_hint_cache`,
  `get_time_weight_for_category`, `init_hint_cache`, `load_contextual_hints`,
  and `get_base_regeneration_rate`.

Notes from step 2.5:

- C23 static-storage compound literals would have been the natural writable
  literal, but Clang 18 rejects them. Test fixtures that store a string in a
  `char *` field now call `CuMutableString`, which copies into a fixed 8 MB
  arena that lives for the whole run and is never freed, like the literals it
  replaces (no heap, so no leak reports).
- Casts on literals passed to parameters that are already `const char *`, and
  `(void *)` casts inside `CuAssertPtr*`, were simply removed.
- `count_color_chars`, `check_flags_by_name_ar`, `remove_var`,
  `get_char_account_name`, and `get_obj_in_list` never write their string
  argument and now take `const char *`. Functions that do write
  (`trigedit_parse` through `smash_tilde`, `nanny`), would cascade into
  non-const helpers (`find_skill_num`, `is_substring`, `show_string`,
  `test_load_zones`), or sit in a function-pointer table typed `char *`
  (`prefedit_parse` in `nanny`'s OLC dispatch) receive a mutable copy from the
  tests instead.
- The production half of the class (about 290 sites: string tables declared
  `char *[]`, `one_argument_u((char *)argument, ...)`, `findLine` in the index
  tools) is step 3.4.

Notes from step 1.3 (first pass):

- `asciiflag_conv` returns a 64-bit `bitvector_t` but every caller stores one
  32-bit flag-array element; those 114 assignments now cast to `int`, which is
  the truncation the implicit conversion already performed.
- Other narrowing sites were cast from the range Clang underlines as the
  converted expression, only where that range is outside every macro. A first
  attempt that also cast macro arguments was discarded: Clang reports such a
  site at an argument's spelling location, and casting one `MIN` or `MAX`
  operand changes the comparison, so those sites are left for hand work.
- Before casting, the `long` sources were checked against their ceilings:
  experience is capped at `EXP_MAX` (2100000000), gold and clan treasure at
  `MAX_BANK` (2140000000), both below `INT_MAX`.
- The object editor's `max_val` was `long` although its largest value is
  400000000.

Notes from step 1.3 (second pass):

- `MIN` and `MAX` are not macros here but `int MIN(int, int)` and
  `int MAX(int, int)` in `utils.c`, so a wider argument is truncated before
  the comparison. Seventy calls passed `long`, `size_t`, `uint64_t`, or
  `long long` values. They now call width-matched `static inline` helpers in
  `utils.h` (`long_min`/`long_max`, `size_min`/`size_max`,
  `u64_min`/`u64_max`, `llong_min`/`llong_max`). Several were real defects:
  - `delay_activity` computed `MIN(LONG_MAX - delay, remaining_delay) + delay`;
    `LONG_MAX - delay` truncated to `-1 - delay`, so every extension set the
    remaining delay to -1.
  - The staff event, transport, and moving-room tick conversions clamped
    `MIN((game_tick_t)INT_MAX, ticks)` after truncating `ticks`, so a count of
    2^31 or more came back truncated instead of saturating at `INT_MAX`.
  - Artifact claim and discovery times passed `time_t` through `int`, which
    fails after January 2038.
  - The rent withdrawal in `Crash_load_objs` subtracted gold from an
    `unsigned long` cost; it now subtracts in signed arithmetic.

Notes from step 1.3 (third pass):

- Narrowing inside macro expansions was fixed at the use, not inside the
  macro: `CuAssertIntEquals` arguments and the `GET_IDNUM`, `GET_EXP`, and
  `GET_PREF` values convert explicitly, `APPEND_TO_BUF` converts its `size_t`
  offset where it calls `snprintf_append`, and multi-line expressions (the
  experience displays, string-length sums, MSDP values) cast the whole value.
- The clan investment return is `long`: an investment of up to `MAX_BANK`
  plus its return does not fit in `int`. Its log message now uses `%ld`.
- `look_at_room_number` took a `long`, so its `room_number < 0` guard never
  fired for the `room_rnum` values every caller passes, and `NOWHERE` reached
  `ROOM_FLAGS()`. It now takes a `room_rnum` and rejects `NOWHERE` and rooms
  past `top_of_world`.
- Clang's `shorten-64-to-32` class is empty after this pass. It stays in the
  migration tier until GCC's `-Wconversion`, which covers the same family plus
  `int` to narrower types, is also empty.

Notes from step 3.1:

- Every warning traced back to an initialized local declared inside one
  `case` body of a long switch, which each later label jumped over; bracing
  that body scopes the declaration to its case.
- Bracing is wrong when a later case uses the variable: `event_crafting`'s
  divide subcommand declared the loop counter that the other repeat loops
  assign and reuse, so it is declared at the top of the function instead.
- Clang also reported the `goto save_char_restore` in every `BUFFER_WRITE`
  after `save_char_checked` declared the legacy talent bitset words mid-function;
  they are declared with the other locals now.
- `-Wjump-misses-init` moved to the baseline tier after a clean baseline
  build with GCC 13; Clang 18 rejects the option, and the per-compiler probe
  leaves it out there.

Notes from step 3.3:

- Renames are scripted from the GCC diagnostics and cover exactly the
  declaration's scope: the rest of the enclosing block for a local, the
  function body for a parameter (never the rest of its parameter list, which
  may still use a same-named typedef), and never member accesses, strings, or
  comments. All token edits in a file are computed against the original text
  and applied once, so two renames in one function cannot shift each other.
- Names follow the shadowed entity: `NAME_id` for locals named after the vnum
  and rnum typedefs, `NAME_value` for locals named after globals, and
  `inner_NAME` for inner locals that reused an outer name.
- A rename is skipped when a macro expanded in the same scope uses the name as
  a free identifier, since the macro would then silently refer to the outer
  variable. The check first looked at every macro body in `src/`, which held
  back four sites; it now ignores a macro whose parameter has the same name,
  which cleared the `index` rename in `find_replacement` and the `ch` rename in
  `make_prompt`.
- `REMOVE_FROM_LIST` hardcodes a `temp` cursor, so both affect removal
  functions declared a damage-reduction `temp` that shadowed the function's
  affect `temp`. The macro is now a wrapper over `REMOVE_FROM_LIST_USING`,
  which takes the cursor variable, and those two functions pass `dr_temp`.
- `-Wshadow` moved to the baseline tier after clean baseline builds with GCC
  13 and Clang 18, so a new shadowing declaration now fails the `-Werror`
  jobs instead of the budget.

Notes from step 3.2:

- `float` is `double` in every file outside the kdtree library and the bundled
  `snprintf`: 798 declarations in 77 files, 447 `f` literal suffixes, four
  float math calls, `strtof`, and the five `scanf` conversions that read into
  the changed variables (`%lf` now). Struct fields changed too, instead of the
  planned casts at each assignment; nothing that changed is written to disk as
  a binary record.
- kdtree's float API (`kd_insertf`, `kd_nearestf`, `kd_nearest_rangef`,
  `kd_res_itemf` and their three-coordinate forms) had no callers and is gone,
  with three of its `alloca` buffers. `kd_res_item3` tested `*x` instead of
  `x`, so it dereferenced a null pointer and skipped a zero coordinate, and it
  always returned 0; it now checks the pointers and returns the item's data
  like `kd_res_item`.
- `MIN` and `MAX` truncated doubles to `int`, as step 1.3 found for wide
  integers. Several were real defects, now calling `FLOATMIN` and `FLOATMAX`:
  the visual obstruction factor and settlement resource richness were clamped
  to 0 or 1, the vessel hazard projection likewise, the perception intensity
  kept for an observer lost its fraction, and `increase_anger` (currently
  uncalled) could never add less than a whole point. The shop price floor
  gives the same result either way.
- The 78 remaining double-to-integer conversions, mostly damage, healing, and
  duration multipliers such as `dam *= 1.5`, are explicit and truncate as
  before: a compound assignment becomes `dam = (int)(dam * 1.5)`, and a
  conversion inside a `MIN` or `MAX` argument casts that argument.
- The ship record holds a test-enforced 5 KiB budget. Its double coordinates
  pushed it to 5128 bytes; `discovery_chance`, a percentage compared with
  `rand_number(1, 100)`, is an `int` now, which brings it back to 5120.
- Exact floating comparisons: `greyhawk_bearing`'s due-north or due-south
  branch returned what the general formula already gives and is gone, and
  its due-east or due-west test uses `DBL_EPSILON`; the mission reward
  multiplier is derived from the integer difficulty; a zero segment length is
  `<= 0.0`. Tests compare doubles with `CuAssertDblEquals` or a tolerance; the
  suite caught two that compared the new doubles against `float` literals.
- The bundled `snprintf` converts its `long double` values explicitly, and
  `util/shopconv` reads profit factors as `double`.
- `-Wdouble-promotion`, `-Wfloat-equal`, and `-Wfloat-conversion` moved to the
  baseline tier after clean baseline builds with GCC 13 and Clang 18.
  `-Wfloat-conversion` was never listed on its own (`-Wconversion` enables
  it), so listing it in the baseline keeps float-to-integer narrowing fatal
  while the rest of `-Wconversion` stays on the budget.

## Remaining work

1. GitHub-side confirmation. Container jobs, the apt.llvm.org install step,
   and `actions/cache` inside the `gcc:16.2` container cannot be replicated
   locally. Open the pull request and watch the first run; the compiler check
   step is the first thing that would fail if the runner's toolchain differs.
2. Dispatch `toolchain-analysis.yml` once by hand to confirm its wall time
   fits the job timeout. Locally the GCC `-fanalyzer` build of the whole tree
   took well over half an hour on three cores.
3. Triage the analysis findings. The first local GCC 16.2 analyzer run
   reported 66 `malloc-leak`, 39 `null-dereference`, 27
   `possible-null-argument`, 10 `out-of-bounds`, and 4 `use-after-free`
   sites. These are candidate bugs, not noise, and deserve their own issue.
4. Burn down the rest of the migration budget. Steps 0, 1.1, 1.2, and 2.1 to
   2.6 are done (see the progress table), step 1.3 is done for 64-bit
   narrowing, and steps 3.1 to 3.3 are done. Left: step 3 (production
   write-strings and cast-qual, small classes, the rest of GCC `conversion`),
   and the step 4 sign-conversion decision.
5. Cadence. Bump the current versions in `test.yml`,
   `scripts/ci/local/Dockerfile*`, and the setup guide within a month of each
   GCC or LLVM point release; raise the minimum when the runner image drops a
   compiler. Regenerate both budget files whenever a pinned compiler changes,
   since counts are compiler-specific.
6. The gcc toolchain PPA on the development host ships a GCC 16 trunk
   snapshot, not 16.2. Use the `luminari-ci:local-gcc-16.2` image for anything
   that must match CI.

## Burn-down plan for the migration budget

Measured on the budget logs behind the two baseline files. The ordering is by
sites cleared per hour of work: root-cause edits in headers first, generated
or scripted edits second, hand edits last. Every step ends the same way: build
the migration tier with both pinned compilers, run
`check_warning_budget.py --update` for each, commit the lowered budget files
with the fix, and move any class that reached zero on both compilers from the
migration list to the baseline list in `production_profile.sh`.

### Where the sites actually are

| Lever | Sites it clears | Evidence |
|-------|-----------------|----------|
| `IS_SET_AR` in `src/utils.h` | about 11000 Clang `sign-conversion` | the `&` of an `int` array element with the `unsigned` `Q_BIT` mask converts the element; `IS_NPC` alone is 3084 sites, `GET_NAME` 1248, `AFF_FLAGGED` 939, the `*_FLAGGED` family and every colour macro (they expand to `PRF_FLAGGED`) the rest |
| `sh_int` and `byte` fields in `struct affected_type` and friends | about 1450 GCC `conversion`, about 1000 Clang `implicit-int-conversion` | 975 sites are `int` to `sh_int`, 281 `int` to `byte`, 195 `int` to `sbyte`; `src/magic/magic.c` alone has 509 |
| generated test prototypes | 1485 of 2123 `missing-prototypes` | every `Test*` function in `unittests/CuTest/` |
| scripted `-Wformat` conversions | 964 GCC `format=` | all are `%d` with an unsigned or vnum argument; GCC's fix-it turned out to be a no-op, see the step 2.2 notes |
| four files for `jump-misses-init` | 579 | `magic.c` 303, `players.c` 135, `study.c` 66, `act.item.c` 43 |
| `float` locals in `src/wilderness/` | most of 554 `double-promotion` and 303 `float-conversion` | three wilderness files hold over 200 sites |
| duplicate `extern` lines | 567 `redundant-decls` | 27 redeclare `conn`, 18 `world`, 17 `mysql_available` |

### Step 0: tooling (done)

- Add `--list CLASS` to `scripts/ci/check_warning_budget.py` that prints the
  distinct sites of one class grouped by file, and `--by-token CLASS` that
  groups them by the identifier at the diagnostic column. Both were needed to
  produce the table above and are needed again after each step.
- Decide two policy questions before touching code, because they change the
  target by 700 sites:
  - Drop `-Wswitch-default` (657 sites, one message: "switch missing default
    case"). `-Wswitch` in the baseline already reports unhandled enum values,
    and `-Wswitch-enum` stays. Adding an empty `default: break;` to 657
    switches adds nothing to correctness.
  - Keep `-Wjump-misses-init`. It is C++-compatibility wording but every site
    is a `case` label jumping over an initialized declaration, which the
    style guide already forbids (declarations at the top of blocks).

### Step 1: header and type roots (done)

1. `IS_SET_AR`: cast the array element to `unsigned int` before the mask, or
   store flag arrays as `unsigned int` if the ASCII loaders and savers agree.
   The cast is the one-line version; measure after it. Expect Clang
   `sign-conversion` to fall from 15028 to under 3000.
2. Widen `spell`, `duration`, `modifier`, `specific` in `struct affected_type`
   and the `byte` and `sbyte` fields that `magic.c`, `db.c`, `players.c`, and
   `objsave.c` assign from `int`. Player and object files are ASCII, so no
   on-disk layout changes; check the MySQL column widths for the same fields.
   Expect GCC `conversion` to fall by about 1450 and Clang
   `implicit-int-conversion` by about 1000.
3. `size_t` to `int` (Clang `shorten-64-to-32`, 602; GCC `conversion` 222):
   the results of `strlen`, `sizeof`, and `snprintf` stored in `int`. Change
   the local to `size_t` where it only feeds another size, cast where it feeds
   an `int` API. Scripted with the site list; review by file.

### Step 2: generated and scripted edits (done)

1. Make `unittests/CuTest/make-tests.sh` also write
   `unittests/CuTest/test_prototypes.h` and include it from `CuTest.h`. Both
   build systems already run the generator before compiling. Clears 1485.
2. `-Wformat` conversions. The plan was to apply GCC's
   `-fdiagnostics-generate-patch` hunks, but GCC 16.2 rewrites `%d` as `%d`,
   so a column-driven script did the work instead (notes above).
3. Delete the flagged `extern` lines for `redundant-decls`: a script that
   removes a flagged line when it is a single-line declaration ending in `;`
   and the same symbol is declared in an included header. Clears 567.
4. The remaining 638 `missing-prototypes` in `src/`: for each flagged
   function, if no other file references the name, prepend `static`;
   otherwise add the prototype to the header that matches the source file.
   Scripted; the `static` half is safe by construction, the header half needs
   a compile to confirm.
5. Tests: `-Wwrite-strings` and `cast-qual` (591 test sites) are string
   literals assigned to `char *` and casts that strip `const`. Change the
   test locals to `const char *`; where a production API takes `char *` for a
   value it never writes, constify that parameter instead of casting.
6. Clang `implicit-fallthrough` (33): insert `[[fallthrough]];` where the
   existing comment says so. GCC already accepts the comments.

### Step 3: file-focused hand work (two to three days, about 2500 sites)

1. `jump-misses-init` (done). The 446 GCC and 579 Clang sites came from
   sixteen declarations: seven case bodies braced, two declarations hoisted.
2. `double-promotion` and `float-conversion` (done; see the step 3.2 notes).
3. `shadow` (done; see the step 3.3 notes).
4. `-Wwrite-strings` in `src/` (188): the 23 in `bsd-snprintf.c` are
   `findLine` and friends taking `char *`; constify the parameters.
5. Small classes, one sitting: `null-dereference` 48, `switch-enum` 45,
   `logical-op` 31, `format-nonliteral` 25,
   `float-equal` 22, `duplicated-branches` 21, `cast-align` 8, `undef` 8,
   `alloca` 4, `duplicated-cond` 3, `alloc-zero` 1. The `null-dereference`
   sites are candidate bugs; the rest are style and fold into whatever is
   nearby.

### Step 4: the sign-conversion tail (decision point)

After step 1 the remaining `sign-conversion` sites are the ones the type
system genuinely disagrees about: `int` counters indexed into `size_t`, `int`
arguments to the unsigned `vnum` types, and `enum` status values compared to
`int`. Measure with `--by-token`. If fewer than 3000 remain, fix them in the
same file-focused way as step 3 and promote the flag. If more remain, the
honest choice is to move `-Wsign-conversion` to the analysis tier and keep
`-Wconversion` in migration; the issue asked for the family to be tracked,
not for every `int` index to become `size_t`.

### Expected trajectory

| After | GCC 16.2 expected | GCC actual | Clang 22.1.8 expected | Clang actual |
|-------|-------------------|------------|-----------------------|--------------|
| start | 11363 | 11363 | 22878 | 22878 |
| steps 1.1, 1.2 | about 9500 | 8010 | about 8500 | 7955 |
| step 2 | about 5800 | 5188 | about 5300 | 6675 |
| step 3 | about 2800 | | about 3000 | |
| step 4 | 0 or the sign-conversion tail | | same | |

The steps landed as separate commits on the `strict-c23-toolchain` branch,
each with its lowered budget files, rather than as separate pull requests.


When items 1 and 2 are confirmed, this document's enduring content is already
in the setup, CMake, and testing guides; file items 3 and 4 as issues and
delete this file.
