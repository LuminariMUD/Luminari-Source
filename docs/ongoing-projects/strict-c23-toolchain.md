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
- Baseline tier: `-Wall -Wextra -Wstrict-prototypes -Wold-style-definition -Wpointer-arith -Wformat-security -Wvla -Wredundant-decls -Wnested-externs -Wmissing-prototypes -Wjump-misses-init -Wshadow -Wdouble-promotion -Wfloat-equal -Wfloat-conversion -Wwrite-strings -Wcast-qual -Wundef -Walloca -Wimplicit-fallthrough -Wconversion -Wno-sign-conversion`, plus GCC's `-Wtrampolines -Walloc-size -Wbidi-chars=any -Wcalloc-transposed-args -Wflex-array-member-not-at-end -Wunterminated-string-initialization -Wcast-align=strict -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wformat-signedness` and Clang's
  `-Wcast-align`. Every common flag after `-Wvla`, the last five GCC flags, and
  the Clang flag were promoted from the migration tier by steps 2.3 to 3.5.
  Clang 18 does not know `-Wjump-misses-init`, so the probe drops it there.
  Clean on all four compilers; `-Werror` is refused with any other tier.
- Migration tier: `-Wnull-dereference` and GCC's `-Walloc-zero`, which depend
  on what the optimizer proves and so stay on the budget at zero rather than
  under `-Werror`. Held by
  `scripts/ci/check_warning_budget.py` against `scripts/ci/warning_budget_gcc-16.txt`
  and `scripts/ci/warning_budget_clang-22.txt`; growth in any class fails the
  new `warning-budget` job. Counting is by distinct site with make output sync,
  which was required to make Clang's numbers deterministic.
- Analysis tier: GCC `-fanalyzer` and Clang's opinionated extras, plus
  `-Wswitch-enum` and `-Wformat-nonliteral` (moved out of the budget by step
  3.5), `-Wsign-conversion` (step 4), and an ISO C23 `-Wpedantic` extension
  report, in the weekly, non-blocking
  `.github/workflows/toolchain-analysis.yml`. GCC's analyzer skips
  `src/character/class.c` (see the notes on the local analyzer run).

### Feature detection

- `-Werror` is stripped from the caller's CFLAGS for every configure and
  CMake probe and restored afterwards. The `struct in_addr` and `socklen_t`
  probes no longer emit diagnostics. Three unused `-Werror` probes were
  deleted.
- `scripts/ci/check_configure_probes.sh` configures both build systems with
  and without strict flags and fails when the generated `conf.h` differs. It caught the
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
- The local CI matrix (`scripts/ci/local/run.py`): all 28 jobs passed on
  `8584422ae` in 7 minutes (`--jobs 3 --cpus 4`), as they did on `70ff161ed`
  before the `class.c` analyzer exclusion. On the analyzer leak and null fixes
  (`96b8600c6`) 27 passed; the process-memory monitor test raced a fake MUD
  process that had not finished its exec, and the Clang production-profile job
  passed again once the test waited for it (`ea2f763ea`). The autorun
  supervision test raced the same way on `cd8b625b5`, reading `.mud.identity`
  before autorun wrote it, and all 28 jobs passed on `352b94fb3` once it waited.
  The first run failed as described in the notes below.

## Budget snapshot

| Compiler | At the start | Now (value conversion pass) |
| -- | -- | -- |
| GCC 16.2 | 11363 sites, 24 classes | 0 sites, 0 classes |
| Clang 22.1.8 | 22878 sites, 23 classes | 0 sites, 0 classes |

No class is left in the budget: value conversion is at zero on both compilers, and sign conversion (GCC 1807, Clang 3330) runs in the analysis tier since step 4.

## Burn-down progress

Sites after each landed step, measured with the CI budget job's exact CMake
command inside the pinned images (`luminari-ci:local-gcc-16.2` and
`luminari-ci:local-fast` for Clang 22.1.8) on a snapshot of the working tree
with the example config headers. The first local run reproduced both committed
budget files exactly; with ccache a full budget build takes about two minutes
per compiler.

| Step | Change | GCC 16.2 | Clang 22.1.8 |
| -- | -- | -- | -- |
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
| 3.4 | const string tables, read-only string parameters, owned strings through mutable pointers; qualifier classes at zero and promoted to baseline | 2532 | 3971 |
| 3.5 | logic defects, dead branches, null guards, format attributes; `switch-enum` and `format-nonliteral` to the analysis tier; nine flags promoted to baseline | 2336 | 3853 |
| 4 | `-Wsign-conversion` to the analysis tier; value conversion stays on the budget | 529 | 523 |
| 5 | explicit narrowing casts, compound assignments, `dc_bonus` widened; `-Wconversion` promoted to baseline | 0 | 0 |

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
  tools) was left for step 3.4, which is done.

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
  400000000\.

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

Notes from step 3.4:

- `-Wwrite-strings` types string literals as `const char[]`. String tables
  and locals initialized from literals are `const char *` now (31
  declarations, found by walking back from each GCC site to its declaration),
  and so are the spatial strategy names, the autowiz level names, and the race
  keyword field.
- Functions that only read a string parameter take `const char *`: `findLine`
  and `walkdir` in the index tools, `create_craft_skill_check`,
  `show_string`, `replace_str`, `convert_from_tabs`, `is_casting_command`,
  `is_valid_paralyzed_command`, `set_imm_title`, and `sort_object_bag`.
  Functions that return literals return `const char *`, and
  `get_feat_value`, `get_mob_follower`, `char_has_mud_event`,
  `mob_has_known_spells`, and `obj_has_special_ability` take const objects,
  which removed the casts their const callers needed.
- The vessel commands cast their `const` argument to reach
  `one_argument_u` and friends; they call the bounded `one_argument`,
  `two_arguments`, and `three_arguments` now, which skip fill words the same
  way.
- Writes that went through those casts: `lore_id_vict` ran `CAP` on the
  shared race keyword in `race_list`, so one lore display capitalized the
  keyword for every later use; it capitalizes a copy now. `set_imm_title` wrote
  a terminator into an overlong caller title and copies with `strndup`
  instead. `sort_object_bag` and the event debug entity lookup copy their
  argument before `get_number` strips a numeric dot prefix from it in place,
  and the stored consumable commands copy theirs for parsers that take
  `char *`.
- Owned strings are held through mutable pointers (`wear_off_msg`, the help
  keyword index, the feat and shop listings, known names), and `column_list`
  takes `const char *const *`, which such arrays reach with a cast that adds
  qualifiers only. Weapon type names point at their literals instead of heap
  copies of them. Zone export builds its `tar` arguments in local buffers
  because `execvp` takes `char *const[]`.
- The pre-commit hook pins clang-format 18.1.8; format with that binary, or the
  hook and a local clang-format disagree. The hook no longer skips
  `src/olc/genolc.c` and `src/core/utils.h` (issue #89).
- `-Wwrite-strings` and `-Wcast-qual` moved to the baseline tier after clean
  baseline builds with GCC 13 and Clang 18.

Notes from step 3.5:

- Logic defects: the social editor accepted any position or level because its
  range tests joined the bounds with `&&`; the high-level quest editor tested
  `location >= NUM_CLASSES` twice and let negative classes through;
  `MOB_ADV_BLACKGUARD_MOUNT` shared vnum 1234 with the basic mount, so the
  advanced mount was never summoned (1236, the other mob tagged
  `blackguardmount` besides the basic and epic mounts, is the advanced one);
  and the track command printed `GET_NAME(vict)` after finding no victim, a
  null dereference when a charmed follower is told to track an unknown name.
- Unreachable code removed: the second trap effect range check in the object
  stat display and the object list (always false since trap effects start at
  1), the duergar weapon branch (`WPT_DUERGAR` has the value of `WPT_DWARF`),
  the reduced immortal usage text of the idea, bug, and typo commands
  (`LVL_IMMORTAL` is `LVL_IMMORT`), a duplicated `LVL_GRSTAFF` level name, a
  doubled `player_specials` test, and `NOTHING || NOWHERE` tests (one value).
- `errno_would_block()` in `sysdep.h` replaces the `EAGAIN`/`EWOULDBLOCK`
  pairs, which compare one value twice on Linux.
- Identical branches merged: 23 identical tails in the treasure bonus tables,
  the immortal and mortal spellbook displays, the house and player object id
  columns, and the pilot spell list choice. The casting check and the
  encounter join guard (both delays are six seconds) no longer repeat one
  statement in two branches.
- Null guards where a lookup can fail: action queue dequeues, the perk purchase
  confirmation, clan claim removal, protection from arrows, door state
  capture, spell battle expiry, GMCP send, pour and fill, the vrock screech
  cooldown, the action cooldown event, and `strip_colors`. The link-loss path
  dropped a redundant `if` that made the compiler assume a null character.
  Tests return after a failed pointer assertion, since `CuAssertPtrNotNull`
  does not tell the compiler the pointer is set.
- The TTYPE client name buffer is a fixed array instead of `alloca`;
  `remove_cmd_from_list` returns when the list holds only its terminator and
  `generate_river` stops when it starts in water, instead of `calloc(0)`.
  Social body parts and the board editor's storage pass through `void *`, and
  the ELF note header is copied out of the note bytes instead of cast.
- Format attributes: 29 printf-style functions (loggers, buffer appenders,
  error setters, `send_to_ship`, `send_to_clan`, `i3_log`, and friends) carry
  `format(printf, ...)`, `read_line` carries `format(scanf, 2, 0)`, and
  `format_time_string` carries `format(strftime, 2, 0)`. The checks this
  enabled found 41 signedness mismatches and 18 empty format strings. The
  mismatches are cast to the printed type rather than given new directives,
  so `NOWHERE` still prints as -1 and the player save file is unchanged.
- `-Wswitch-enum` moved to the analysis tier: GCC reported 4224 enumerators
  missing from 44 switches that already have a `default`, most of them over the
  event id enum, and `-Wswitch` in `-Wall` still covers switches without one.
  `-Wformat-nonliteral` moved there too: the remaining sites print from format
  tables by design (auction messages, wall descriptions, clan command help,
  description templates, spec alert messages, the snprintf self-test). The
  migration tier no longer lists `-Wformat=2`: the baseline's `-Wformat` and
  `-Wformat-security` cover the rest of it, and `-Wformat-y2k` would only flag
  the `%c` and `%m/%d/%y` display dates the strftime attribute exposed.
- `-Wundef`, `-Walloca`, `-Wimplicit-fallthrough`, GCC's `-Wcast-align=strict`,
  `-Wduplicated-cond`, `-Wduplicated-branches`, `-Wlogical-op`, and
  `-Wformat-signedness`, and Clang's `-Wcast-align` moved to the baseline tier
  after clean baseline builds with GCC 13 and Clang 18. `-Wnull-dereference`
  and `-Walloc-zero` stay in the migration tier at zero: both report what the
  optimizer proves, so another optimization level could fail a `-Werror` build
  that is clean here.

Notes from step 4:

- Measured after step 3: 1807 GCC 16.2 sites in 207 files and 3330 Clang
  22.1.8 sites in 229 files, past the 3000 the plan set as the limit for fixing
  them in place. The identifiers at the reported columns are the type-system
  disagreements the plan expected: `atoi` results stored in vnum and index
  types (146 GCC, 152 Clang), `snprintf_append` offsets (75), `NOTHING` and
  `NOWHERE` compared with signed values (Clang 143), the `TOGGLE_BIT_AR` and
  `PRF_TOG_CHK` bit toggles (Clang 150), and loop counters and lengths used as
  sizes.
- `-Wsign-conversion` moved to the analysis tier on both compilers.
  `-Wconversion` stays in the migration tier: its 529 GCC and 523 Clang sites
  are narrowing that can lose data, which is what the budget should hold. In C
  `-Wconversion` enables `-Wsign-conversion` on both compilers, so the
  migration tier lists `-Wno-sign-conversion` after it; the analysis tier's own
  `-Wsign-conversion` comes later on the command line and wins.

Notes from the value conversion pass:

- Nearly every site stored an `int` in a `byte`, `ubyte`, `char`, `sh_int`, or
  `sbyte` field: race and weapon tables, clan privileges, preference values,
  conditions, room light, affect locations. Step 1.2 had already widened the
  fields that hold running totals, so the rest take explicit casts that keep
  today's truncation.
- The Clang excerpts drive a copy of the step 1.3 range script for
  `-Wimplicit-int-conversion`: 380 sites cast where the underline shows the
  converted expression. It skips ranges that contain an assignment, because
  casting `x += y` changes nothing.
- Hand-cast sites: `tolower` and `toupper` results stored in `char` (52),
  `RANGE`, `LIMIT`, and `GET_LEVEL` results assigned to narrow fields, the
  summon damage dice, the handler's affect location and modifier arguments
  (`NUM_APPLIES` is 75, so a `byte` location holds every value), the ASCII map
  coordinates, and port numbers passed to `htons`. `MOB_SET_FEAT`,
  `SET_ABILITY`, and `VESSEL_REPAIR_FIELD` cast inside the macro; the last uses
  `typeof(cur)` because it repairs fields of several types.
- GCC also reports compound assignments into narrow fields, which Clang does
  not; those read `x = (T)(x op (y))` now. `dc_bonus` was the exception: 47
  compound updates (40 `+=`, 5 `++`, 2 `-=`) adjust it, so it is an `int`
  instead of a `byte` that wrapped past 127. The logon record keeps its `int`
  fields and casts the `long` ids, because `struct last_entry` is written with
  `fwrite`.
- `-Wconversion` moved to the baseline tier, with `-Wno-sign-conversion` after
  it, after clean baseline builds with GCC 13 and Clang 18. The analysis
  tier's `-Wsign-conversion` still comes later on its command line.
- `check_warning_budget.py` refused every log without warnings, taking it for a
  build without the migration tier, so a clean migration build would have failed
  the budget job. It now accepts such a log when the budget file lists no
  classes and the log shows compilation; while classes remain, an empty log
  still fails.

Notes from the local CI run and the analyzer triage:

- The first local CI run on the promoted tree failed in ways the budget
  builds could not see, because they build one optimized configuration and
  never run the tests:
  - The step 2.2 format pass had printed index typedefs with `PRI_IDX` or
    `%u`, so a `NOTHING` or `NOWHERE` vnum became 4294967295 where it used to
    be -1. The pet object decoder rejects that, and three pet persistence
    tests failed in every container (the development host passed them, so
    only the clean CI database exposed it). Signed output with an explicit
    `(int)` is back wherever text is read back or compared: object save
    records, the object file recipient line, the moving room key, clan hall
    and claim lines, the IBT room, craft vnums, vessel cargo and crew rows,
    and the DG script variables for clans, zones, and exits. Display-only
    uses keep `PRI_IDX`.
  - Clang 22.1.8 at `-O2` miscompiles `vessel_commodity_price` in the shape
    step 1.3 gave it (saturate at `INT_MAX`, then `llong_max(1, price)`): the
    saturation is dropped. A standalone copy reproduces it, and `-O0`, Clang
    18, and GCC are correct; a plain lower clamp avoids it. Other uses of the
    width-matched helpers were checked in the same harness and compile
    correctly.
  - `make cutest` compiled test objects before `test_prototypes.h` existed
    (`BUILT_SOURCES` only orders all, check, and install), which broke the
    sanitizer, coverage, and memory-check jobs.
  - With `-Wconversion` in the baseline, a Debug build reports the `size_t`
    hint count passed to `dice()`, which the optimized builds fold away. Debug
    baseline builds are now clean on GCC 13, GCC 16, Clang 18, and Clang 22.
  - The Clang CMake production-profile job once reported that the migration
    tier dropped `-Wcast-align`; the same probe keeps it when run on its own,
    and the job passed in the rerun.
- `check_warning_budget.py` accepts a log without warnings only while the
  budget file is empty, so the budget job still passes with both budgets at
  zero.
- A local GCC 16.2 analysis-tier build of the server target:
  `class.c` never finished: after 40 minutes its compile had grown past 30
  GiB in `load_class_list`, which registers every class in one 5300-line
  function, so both build systems now compile that file with `-fno-analyzer`.
  With it left out, a runner-shaped rebuild of the server target without
  ccache (4 CPUs, 16 GiB, `-j4`) took 206 seconds, or 645 compile seconds
  over 331 files. The heaviest compiles were `fight.c` (89 seconds, 6.6 GiB),
  `crafting_new.c` (71 seconds), and `magic.c` (3.4 GiB).
  Distinct analyzer sites by class, after the fixes below:
  41 `malloc-leak`, 11 `out-of-bounds`, 3 `use-of-uninitialized-value`, 3
  `fd-leak`, 2 `null-dereference`, and 1 each of `tainted-array-index`,
  `use-after-free`, and `imprecise-fp-arithmetic` (63 sites).
  Fixed from the triage: a double free between `free_claim` and
  `remove_claim_from_list`; `ascii_convert_house` returning failure at end of
  file without closing its files; `board_load_board` leaking its `FILE` on
  every corrupt-file return; `fread_flags` and the say family indexing before
  an empty or short string; a dangling `d->str` in `playing_string_cleanup`;
  a `size_t` passed to `ProtocolOutput` as an `int` pointer; a NULL
  `argument` in two spec procedures; unchecked `fopen` in the map writers; a
  shift by -1 in `find_race_bitvector`; and an unchecked `close_type` index
  from the logon file. Confirmed false positives: the tokenizer over-reads
  (the array is NULL-terminated), `perform_complex_alias` (indexes are bounded
  by `num_of_tokens`), the Discord and terrain server sockets (every error
  path closes them), `insert_object`, and `count_commands`.
  A second pass fixed 21 `null-dereference` sites and 24 leaks. `ACMDU` handed a
  NULL argument to six command bodies, and `buyarmor` and `buyweapons` read the
  argument of a NULL-argument call; kick, slam, and faerie fire kept a NULL
  victim when the fight was in another room; a zone `M` command used a mobile
  that failed to load; `find_case`, `find_done`, and the `break` handler walked
  off the end of a trigger whose nested `while` or `switch` has no `done`; a
  quest-complete countdown, a room trap event, and a mob `dg_cast` without a mob
  used a NULL character or room; and `eldritch_blast`, the connection pool, and
  bone armor used a pointer they had not checked. The leaks were a `strdup`
  passed to callees that never free it (`do_hit`, `do_charge`, three alchemy
  commands, mission mobs, and the vendor armor list), setters that overwrote a
  string without freeing it (crafting keywords and descriptions, buildwalk, the
  retainer recipient, new mail, the supply order description, and the vampire
  cloak rename, which now leaves prototype strings alone), and early returns in
  `replace_str`, `House_save_control`, `do_eqrating`, and `load_dr`. The two
  `null-dereference` reports left follow the NULL check inside
  `get_character_transport` into commands whose character is never NULL. The
  `malloc-leak` reports left are pointers stored into character, account, OLC,
  object, and list structures that the analyzer stops tracking; several are
  setters fixed above for their old value.
  A third pass fixed the smaller classes: unchecked `strdup` and `malloc`
  results in the help import, board posts, `get_number`, the template lookups,
  and the wilderness map; a help cache check after use; `zmalloc_check` writing
  to a log it never checked; an unchecked tag copy when merging help entries;
  and a NULL argument in the rune scimitar's dodge proc. Two reports are left in
  those classes: the logon file's `close_type` is already bounds-checked on both
  sides, and the floating-point allocation size is in `RidgedMultifractal2D`,
  which nothing calls. The same pass stopped crafting from freeing strings an
  object still shares with its prototype: `restring`, `reforge`, `create`, bone
  armor, the harvest node reset, the new reforge command, and the vampire cloak
  now use `free_object_string`, and `restring` no longer frees the description
  twice or frees the prototype's extra descriptions.

## Remaining work

1. GitHub-side confirmation. The first GitHub run of pull request #185 (merged
   as `706318e9f`) found one gap local CI could not see: the strict gcc-16 jobs
   run in the `gcc:16.2` container and reach MariaDB as `mariadb`, which the
   test runtime preparer rejected as a non-loopback host. The preparer now
   accepts that service name in CI, and the local runner resolves it the same
   way instead of rewriting the host; confirm the next `master` run of Build &
   Test passes.
2. Done: `toolchain-analysis.yml` ran by hand on `master` after the merge (run
   34894872217\) and passed. The GCC 16.2 analysis build took 5 minutes 46
   seconds, 6.5 minutes for the whole job against its 120-minute timeout, and
   reported the same 1713 distinct warning sites in 11 classes as the local
   runner-shaped build. The Clang 22 analysis build took about a minute, and
   both GNU extension reports passed.
3. Done: the analyzer triage. Every class is triaged (see the notes above); the
   reports left are the false positives listed there.
4. Done: the migration budget is burned down. Steps 0, 1.1, 1.2, and 2.1 to
   2.6 are done (see the progress table), step 1.3 is done for 64-bit
   narrowing, steps 3 and 4 are done, and value conversion is at zero, so the
   budget files list no classes.
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
| -- | -- | -- |
| `IS_SET_AR` in `src/core/utils.h` | about 11000 Clang `sign-conversion` | the `&` of an `int` array element with the `unsigned` `Q_BIT` mask converts the element; `IS_NPC` alone is 3084 sites, `GET_NAME` 1248, `AFF_FLAGGED` 939, the `*_FLAGGED` family and every colour macro (they expand to `PRF_FLAGGED`) the rest |
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

### Step 3: file-focused hand work (done)

1. `jump-misses-init` (done). The 446 GCC and 579 Clang sites came from
   sixteen declarations: seven case bodies braced, two declarations hoisted.
2. `double-promotion` and `float-conversion` (done; see the step 3.2 notes).
3. `shadow` (done; see the step 3.3 notes).
4. `-Wwrite-strings` and `cast-qual` in `src/` (done; see the step 3.4 notes).
5. Small classes (done; see the step 3.5 notes).

### Step 4: the sign-conversion tail (done: analysis tier)

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
| -- | -- | -- | -- | -- |
| start | 11363 | 11363 | 22878 | 22878 |
| steps 1.1, 1.2 | about 9500 | 8010 | about 8500 | 7955 |
| step 2 | about 5800 | 5188 | about 5300 | 6675 |
| step 3 | about 2800 | 2336 | about 3000 | 3853 |
| step 4 | 0 or the sign-conversion tail | 529 | same | 523 |

The steps landed as separate commits on the `strict-c23-toolchain` branch,
each with its lowered budget files, rather than as separate pull requests.

When items 1 and 2 are confirmed, this document's enduring content is already
in the setup, CMake, and testing guides; file items 3 and 4 as issues and
delete this file.
