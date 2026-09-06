# RoL converter and integration completion plan

Branch: [rol-converter-integration-113-117](https://github.com/LuminariMUD/Luminari-Source/tree/rol-converter-integration-113-117)

Status: planned; implementation and release validation have not started under this plan.
Prepared: 2026-09-06. Inspected branch revision: `7c5677ec132bda12f98b2ba630f372bfb0b46651`.
Plan-ablation review: 2026-09-06, against all five live issue checklists and current code.

## Outcome and scope

**Outcome:** Complete the acceptance criteria of all five issues below, with deterministic
conversion, explicit content and player-progression decisions, focused regressions, and a verified
development release rehearsal. Each issue remains the source of truth for its requirements and
completion status. A documented, approved content-only disposition can satisfy an issue that
explicitly permits it; an unresolved decision or an unexplained loss cannot.

**Non-goals:** This planning task changes no converter, game, database, or world data. Future
implementation does not include production deployment, the C file organization work in #96,
replacement of the existing weapon classifier, a new conversion framework, or automatic creation
of new base classes. Existing public interfaces, persistence formats, and worldfile grammars remain
the implementation path. Any decision that requires expanding that path needs a concrete scope
revision before work begins. Rehearse world application on a disposable development copy;
deployment to an existing server needs its own concrete release scope, as #115 requires.

**Files:** This planning task changes only this document. Implementation files and their purposes
are identified per step below; they are a bounded change map, not a requirement to edit every file.
Never edit the customized `src/campaign.h`, `src/mud_options.h`, or `src/vnums.h`, or change
`lib/.env` or `lib/mysql_config`. Use existing target constants and supported data fields.

**Proof:** Each issue has an exit gate below. Preserve before/after conversion evidence for the
mechanical split, test each accepted behavior change, then run the existing full release gates
against the final candidate. Report commands, results, skipped checks, and unresolved decisions.

## Issues and execution order

- [#113](https://github.com/LuminariMUD/Luminari-Source/issues/113): Infer target armor
  families and wearable slots instead of passing source values.
- [#114](https://github.com/LuminariMUD/Luminari-Source/issues/114): Finish object level,
  extension, color, and ship-light mapping policies.
- [#115](https://github.com/LuminariMUD/Luminari-Source/issues/115): Review remaining weapon
  inference fallbacks before content release.
- [#116](https://github.com/LuminariMUD/Luminari-Source/issues/116): Split parsers and
  emitters by worldfile type behind stable facades.
- [#117](https://github.com/LuminariMUD/Luminari-Source/issues/117): Decide missing player
  archetypes and assign converted spell kits.

| Step | Work | Dependency and exit |
|------|------|---------------------|
| 0 | Freeze inputs and refresh inventories | Establish the evidence used by every later step. |
| 1 | #116: split parsers and emitters | Preserve baseline behavior before changing rules. |
| 2 | #113: armor families and slots | Resolve armor policy, then implement and audit it. |
| 3 | #114: remaining object policies | Finish the shared object path, including proc disposition. |
| 4 | #117: player kits and archetype decisions | Resolve progression decisions and implement accepted access. |
| 5 | #115: final weapon review | Review the actual output of the completed conversion rules. |
| 6 | Combined development rehearsal and closure | Validate and seal the final branch candidate. |

Use separate coherent commits for the mechanical split, armor behavior, each remaining object
policy, approved player-kit changes, and reviewed weapon overrides. Steps 2 and 3 share object
metadata decisions; finish those decisions before signing off their combined output. Prepare
review inventories once, then refresh them when their inputs or implementation change. Product
decisions block only their dependent behavior changes; they need not delay the mechanical split.

Plan-ablation result: retain all five issue requirements and the existing Phase 7/8 gates.
Reuse current inventory, diagnostics, reporting, and test infrastructure; add policy files only
for reviewed exceptions that need them. Limit new gameplay regressions and help edits to accepted
changes and demonstrated gaps. Remove mandatory cleanup of unrelated world warnings and use a
disposable copy for the apply rehearsal. Keep one implementation behind facade re-exports and
store generated evidence in existing ignored run storage; no new framework or service is needed.

## Current evidence that changes the plan

- [rol_source.py](../../scripts/world/wtool_lib/rol_source.py) owns all seven source grammars,
  shared record types, parser dispatch, and active-corpus loading. Its object parser recognizes
  three source economy fields; source affect words must not become a target level or timer.
- [rol_transform.py](../../scripts/world/wtool_lib/rol_transform.py) owns six emitters.
  `emit_object()` defaults object level to 1, carries source armor values through, and emits
  `G/H/I` on the inferred weapon path. `convert_text()` recognizes `&+X` and `&N`/`&n` and
  already protects literal `@`, tilde framing, ASCII, and LF.
- [rol_soc.py](../../scripts/world/wtool_lib/rol_soc.py) already compiles SOC into DG triggers
  and attachments. `.shp` means shops; ship objects are a separate `.obj` conversion concern.
- [apply_ac()](../../src/handler.c) includes `WEAR_TAIL` as an existing RoL exception. The
  historical armor study's blanket treatment of tail gear as ineligible is no longer current.
  The current converter also distinguishes tail rings from dedicated tail gear.
- [Armor constants](../../src/structs.h) include the duplicate chain-head entry at index 26.
  [load_armor()](../../src/combat/assign_wpn_armor.c) supplies actual family penalties and
  slot definitions. Do not calculate armor indices using offsets or assume a uniform grid.
- The installed reference source is `EXAMPLE/RealmsOfLuminari`. Its `src/db.c:read_object()`
  force-sets source `ITEM_LIT` for `ITEM_SHIP`; that state need not appear in the source file.
- [test_unassigned_spells.c](../../unittests/CuTest/test_unassigned_spells.c) already covers
  the support, damage/control, summon/event, and elemental-embodiment groups implicated by the
  later 14-spell claim. Current registrations also exist in
  [spell_parser.c](../../src/magic/spell_parser.c). Registration and existing tests do not
  prove complete behavior or approved player access; audit each spell before adding handlers.
- Current test discovery finds **509 world-tool tests, with zero import/discovery errors**.
  Relevant module counts are source 13, transform 136, weapon mapping 38, target objects 4,
  Phase 7 eight, and Phase 8 nine. These are planning observations, not fixed acceptance counts
  or claims that those tests passed. All declared Phase 8 code/documentation evidence paths exist.
- Phase 7/8 compare candidate findings against the baseline and reject active errors on selected
  records; they do not require every existing world warning to disappear. Keep these gates and
  review changed findings instead of imposing a new global `--strict` requirement.

## Step 0: freeze the baseline and prepare decisions

- [ ] 0.1 Record the implementation starting commit, source-package identity and hashes, active
  index/dependency closure, conversion-policy version/hash, target constants, calculator binary
  identity, and the frozen development target world. Confirm `APP_ENV=development`. Refresh
  these records if inputs change; historical run directories are not current proof.
- [ ] 0.2 Run `make test-world-tools` before the split and retain its log and discovered tests.
  Use existing fixtures and `tests/rol_reference.py` to capture representative output for
  `.mob`, `.obj`, `.wld`, `.zon`, `.shp`, `.qst`, and `.soc`, plus ledger decisions, diagnostics,
  exclusions, and hashes. Build only the baseline/discovery/plan/audit inputs this comparison
  consumes; the complete release workflow runs against the final candidate in step 6.
- [ ] 0.3 Recount active, well-formed, excluded, armor-eligible, other-wearable, weapon, launcher,
  ammunition, quiver, ship, and exceptional-color records. Run
  `rol_weapon_mapping.audit()` on the selected corpus. Identify every fallback, override, and
  name/type disagreement. Keep source path, source VNUM, source hash, and emitted target identity
  in the review evidence; a VNUM alone is insufficient when records collide across packages.
- [ ] 0.4 Build one disposition table for the gaps in #113/#114 from the actual source loader,
  its object procedures, the converter, target loader/writer, and runtime consumers. Record
  source meaning, target representation or named loss, rationale, affected records, and existing
  or needed regression coverage. Include armor AC/warmth/prestige/proc, extensions, nonweapon
  metadata, source colors, and implicit ship lighting; steps 2/3 complete this same table.
- [ ] 0.5 Record existing policy decisions and identify unresolved owner/builder choices for
  armor penalties/curses, non-armor AC scaling/stacking, item levels, named losses, and the five
  player identities. Build the spell/class matrix once in 117.1. Present concrete unresolved
  choices before their implementation; honor decisions already made and continue independent work.

Exit: the selected inputs can be reproduced, baseline failures/skips are recorded, and policy
questions have named evidence and a review point. Historical counts such as 2,527 armor records,
25 weapon fallbacks, 54 color strings, 12 ships, 428 tests, or 75 plus 14 spells are review leads.
Never substitute them for a fresh inventory. Keep existing corpus-specific assertions tied to
their pinned package; do not weaken release gates to make a different package pass.

## Step 1: split format ownership without behavior changes (#116)

Files: `scripts/world/wtool_lib/`, both `Makefile.am` and `CMakeLists.txt`, and the existing
world-tool tests. The following flat layout supplies one implementation per format:

| Module | Responsibility |
|--------|----------------|
| `rol_conversion_types.py` | Shared record/reference/diagnostic/corpus/result/resolver types. |
| `rol_source_common.py` | Segmentation, source rows and tilde strings, diagnostic construction. |
| `rol_transform_common.py` | Shared text conversion, bounded strings, bits, directive helpers. |
| `rol_mobiles.py` | Mobile parser/emitter and mappings; call existing identity/calculator helpers. |
| `rol_objects.py` | Object parser, object mappings, values, extensions, emitter. |
| `rol_rooms.py` | Room parser, flags/sectors, exits, traps, emitter. |
| `rol_zones.py` | Zone parser, reset references, equipment-position map, emitter. |
| `rol_shops.py` | Shop parser, restrictions, stock, hours, messages, emitter. |
| `rol_quests.py` | Quest parser, references/rewards, HLQ emitter. |
| Existing `rol_soc.py` | SOC parser plus the existing trigger compiler and attachments. |
| Existing `rol_source.py` / `rol_transform.py` | Stable dispatch/reporting and export facades. |

- [ ] 116.1 Inventory actual imports, including mappings and private helpers imported by
  `rol_capability_audit.py`, `rol_mobile_identity.py`, tests, and phase orchestration. Preserve
  signatures, returned types, exported names, exception behavior, and diagnostics.
- [ ] 116.2 Move shared types and only helpers with actual cross-format callers first. Change
  imports that would create cycles to their owners; retain existing callers of stable facades.
  In particular, mobile identity must not import `rol_source.py` after that facade starts
  importing the mobile format module. Keep existing focused helpers in their current owners.
- [ ] 116.3 Move each grammar together with its format's conversion logic. Move SOC's imports
  of `RolRecord` and `convert_text` to the shared owners before moving its parser into
  `rol_soc.py`. Preserve one-way imports: shared code imports no format; formats import shared
  code or an explicit format owner; facades import formats. Keep object/mobile affect maps and
  shop/object type-map dependencies explicit. Re-export compatibility names from the facades.
- [ ] 116.4 Update both converter source manifests for every added/moved module. Extend
  `rol_phase8._CODE_EVIDENCE_PATHS` to cover the actual output-affecting modules and data,
  including inference/override inputs used by subsequent steps. Reuse the existing evidence
  mechanism: test coverage of these paths and rejection of a changed captured input at completion.
  Preserve CLI arguments, artifact contracts, and ledger behavior; no new dependency scanner.
- [ ] 116.5 Extend existing import/round-trip tests where necessary. Run all discovered world
  tests and compare the representative before/after conversion output byte for byte. Compare
  diagnostics and action decisions as well. Expected code-evidence hashes and documented
  run-time metadata may differ; emitted world bytes and conversion decisions must not.

Exit: all seven formats have clear owners, existing callers work through stable facades, no
duplicate implementations or cycles remain, both manifests are complete, and the mechanical
change passes the full world suite with equivalent conversion output. Keep inference, level,
color, and spell-rule changes out of this commit series.

## Step 2: armor families, wearable slots, and AC (#113)

Files: the object format module from step 1, existing transform/object tests, and manifests.
Extract `rol_armor_mapping.py` if the emitter and audit need a shared inference owner; add
`rol_armor_overrides.json` only for reviewed exceptions, following the existing weapon pattern.
Read target `structs.h`, `combat/assign_wpn_armor.c`, `handler.c`, and `db.c` as contracts;
change runtime code only for a demonstrated acceptance defect that conversion cannot express.

- [ ] 113.1 Resolve disposition before classification. Separate BODY/HEAD/ARMS/LEGS/SHIELD
  armor, the existing dedicated-tail exception, and other armor-typed wearables. Decide empty
  wear masks, mixed armor/non-armor masks, multi-slot items, placeholders, and cursed shackles
  individually or by a reviewed rule. Preserve tail-ring normalization and takeability.
- [ ] 113.2 Approve the relationship between source protection and target family penalties.
  Choose families from identity/material evidence, never AC magnitude. Explicitly review armor
  check penalty, spell failure, dexterity cap, movement, and proficiency. Preserve source AC
  where intended; do not add `ITEM_SET_STATS_AT_LOAD`, which can replace it with table stats.
- [ ] 113.3 Implement deterministic family inference: reviewed override, specific-to-general
  identity keywords, reviewed material fallback, then a named conservative disposition.
  Resolve `(family, slot)` using explicit current target constants/table entries. Reject
  undefined, uninitialized, duplicate-only, and mismatched slot results; canonicalize chain
  helmets instead of selecting the duplicate entry. Do not renumber target armor constants.
- [ ] 113.4 Implement the approved non-armor treatment, using `ITEM_WORN` plus supported
  `APPLY_AC_NEW` when appropriate. Source armor value 0 is positive for protection; source
  apply 17 uses the opposite sign. Do not call the existing armor-apply helper on value 0
  without adapting that convention. Decide scale, rounding, bonus type/stacking, and curses;
  preserve existing applies and prevent double-counted protection.
- [ ] 113.5 Consume or diagnose source warmth, prestige, and proc values before replacing
  armor slots. Step 3 owns the shared proc/extension mapping. Preserve source record data for
  that mapping rather than destroying inputs or emitting stale values in unrelated target slots.
- [ ] 113.6 Use existing reporting patterns for a stable audit per record: source identity,
  slot/values, disposition, inferred family and target constant/index, rule/override, AC result,
  penalties, and named losses.
  Review every fallback and ambiguous/penalized case against the refreshed corpus.
- [ ] 113.7 Test every supported family/slot, zero/positive/negative protection, curses,
  conflicting masks, empty masks, placeholders, overrides, duplicate indices, rings and tail
  gear, existing applies, and stable diagnostics. Parse source fixtures through the real source
  facade, emit, then parse with `objects.py`; verify runtime wear/AC/penalty behavior in step 6.

Exit: every selected armor record has a reviewed disposition, valid target family/slot behavior,
and intentional protection/penalties. Other wearables do not silently lose or invert AC, tail
behavior remains correct, and no raw source metadata masquerades as target armor identity.

## Step 3: remaining object policies (#114)

Files: `rol_objects.py`, the shared text helper, existing conversion policy/mapping data where
needed, `rol_special.py` and reconciliation code only where a reviewed procedure requires it,
existing source/transform/object tests, and the relevant source/evidence manifests.

- [ ] 114.1 Approve and document an object-level policy: a reproducible mapping into 1..30
  with evidence and explicit exceptions, or an explicit acceptance of permanent level 1.
  The source loader has no object-level field. Do not invent one, confuse affect words with
  economy fields, or conflate item level with a magic item's separately bounded caster level.
  Test the inputs, bounds, and repeatability the chosen rule actually uses. If level 1 is
  accepted, retain that default with coverage and documentation; no level calculator is needed.
- [ ] 114.2 Complete the table from 0.4 for target `B` (spellbook), `C` (special abilities),
  `K` (activated spells), `S` (weapon proc spells), and nonweapon `G/H/I` (proficiency,
  material, size). Read source behavior and target loader, `olc/genobj.c`, and runtime consumers.
  Emit only supported semantics; use explicit named losses when no equivalent is approved.
  Preserve extension ordering, field counts, capacities, source spell-ID mapping, and defaults.
- [ ] 114.3 Audit source weapon/armor proc values and source procedures against existing `Z`
  preservation and DG attachments. Assign each behavior exactly one owner: existing special
  procedure, supported extension/trigger, or an approved named loss. Implement missing accepted
  equivalents without duplicate effects. Retain existing `E/A/Z/T`, trap, and reference behavior.
  Treat loader-ignored trailing source data according to the actual source loader.
- [ ] 114.4 Inventory and decide all exceptional color forms: `&-L`, `&-<`, `&=`, `&_`, `&%`,
  `&&`, `&$`, `&c`, `&I`, `&<`, and `&g`. Trace valid source escapes and distinguish malformed
  or literal content. Convert or intentionally strip supported presentation effects and report
  unsupported ones. Preserve current literal-`@` escaping, tilde safety, ASCII, and LF. Because
  the helper is shared, verify room/mobile/shop/quest/SOC text as well as object strings.
- [ ] 114.5 Reproduce source loader-forced ship light: source `ITEM_SHIP` becomes a target
  boat with the active target `ITEM_MAGLIGHT` flag through the existing flag mapping. Preserve
  unrelated flags and boat behavior. Test ships with and without authored light, ordinary boats,
  target parsing, and carried/dropped/moved lighting. Verify the runtime result: emitting the
  flag alone is insufficient proof. Repair only a demonstrated gap in the required light behavior.
- [ ] 114.6 Extend deterministic fixtures and target-parser round trips for every approved
  mapping, boundary, and named loss. Verify bounded extension counts and valid spell references.
  Produce a human-readable loss report with source/target identities and affected-record totals.

Exit: item levels are intentional, every representable extension/metadata/procedure behavior is
mapped or explicitly disposed of, all listed color forms are reviewed, ship lighting survives,
and changes preserve previously supported object data and stable output.

## Step 4: player identities and converted spell kits (#117)

Files: `src/character/class.c`, existing class/perk/feat/premade-build owners actually needed
by the accepted design, `src/magic/domains_schools.c`, and the existing magic handlers and
registration files only where a verified gap needs repair. Reuse `src/spec/spec_rol_totem.c`,
`test_unassigned_spells.c`, `test_spells_skills_production.c`, and `test_spec_mechanics.c`.
Player-visible changes also require both the help database and `lib/text/help/help.hlp`.

- [ ] 117.1 Rebuild the entire pinned per-class matrix, including shared-class spells and the
  six historically unregistered spells. Deduplicate by canonical target spell ID; do not sum
  per-class counts. Record source name/ID/class/learning level, target ID and aliases, real
  handler/dispatch, current class/domain access, proposed target level/acquisition path,
  prerequisites, balance rationale, and final player-access or content-only disposition.
- [ ] 117.2 Verify each of the later 14 claimed gaps: heal undead, dark wrath, unholy aura,
  camouflage, cyclone, lich touch, lava burst, ice layer, call lycanthrope, Tazrik's frenzied
  hound, and water/fire/earth/air elemental embodiment. Trace the current registration through
  its actual handler and cleanup behavior; reuse existing implementations and repair only
  demonstrated defects. Do not reopen already implemented handler work from historical prose.
- [ ] 117.3 Record the five product decisions below with a concrete progression/access table.
  Use existing class, specialty, feat, perk, companion, and multiclass systems as the initial
  implementation path. A choice to preserve only converted content must be explicit and approved.

| Identity | Initial path to evaluate | Decision required for completion |
|----------|--------------------------|----------------------------------|
| Shaman | Cleric spirit/totem progression using the existing totem implementation | Whether this is a player identity; acquisition levels, Wisdom use, spirit kit, and totem progression. Current conversion requires Cleric 21 and does not establish full Shaman progression. |
| Elementalist | Wizard specialty/perk progression with elemental choice and embodiment | Specialization restrictions, embodiment access, and the minor creation/thunder lance/air blast/earthblood/earth fog/fire fog kit. |
| Battlechanter | Bard martial/shamanic package | Player access to song of travel and whether a distinct war-performance progression is wanted. |
| Dire Raider | Ranger/Warrior dire-wolf bond package | Companion acquisition/scaling, mounted interactions, prerequisites, and its spell kit. |
| Mercenary | Documented disabled-source Warrior/Rogue build | Preserve that disposition unless a concrete requirement warrants enabling or adding something. |

- [ ] 117.4 Assign every accepted spell to explicit class/domain levels and acquisition paths.
  Use `class.c`'s spell assignments and `init_spell_levels()` plus the domain registry, rather
  than treating converter mobile class mappings as player access. Implement only the approved
  progression changes. If a full new base class is chosen, amend this plan with its complete
  progression, compatibility, persistence, and testing scope before introducing class IDs.
- [ ] 117.5 Test approved access after the real class/domain initialization sequence, including
  immediately below/at acquisition level, eligible/ineligible classes, prerequisites, learning,
  preparation or spontaneous casting, domain restrictions, and multiclass interactions.
  Preserve content-only spells as unavailable to players. Retain valid registration-default
  tests that call only `mag_assign_spells()`; add assertions after full initialization for actual
  class/domain access. Change an old expectation only if behavior at its tested stage changes.
- [ ] 117.6 Reuse existing handler regressions. Add balance/cleanup cases where new player
  access or an accepted behavior change introduces a concrete risk: targeting/saves, costs,
  duration/stacking, cooldowns, totems, summons/companions, or embodiment transitions. Exercise
  interactions with affected class features; run the full suite and accepted kits in step 6.
- [ ] 117.7 Update affected class/spell/feat help entries in the database and `help.hlp`, plus
  relevant durable documentation, using existing editing/verification patterns. Check those
  entries against the development MUD display; a repository-wide help reconciliation is outside
  this issue. Record excluded identities and content-only decisions in the same decision matrix.

Exit: all five identities have explicit outcomes; every audited spell has working accepted
access or an approved content-only disposition; acquisition/balance regressions pass; and the
affected help entries and in-game behavior agree. Deferring a required progression
decision does not close this issue.

## Step 5: review final weapon inference output (#115)

Files: existing `rol_weapon_mapping.py`, `rol_weapon_overrides.json`, and their tests. Update
the object emitter only for a demonstrated integration defect. Keep the established classifier.

- [ ] 115.1 Regenerate `rol_weapon_mapping.audit()` after object-rule changes, using the exact
  selected source package. Review every fallback, override, and name/type disagreement with
  source text/declared type, rule, target type, and emitted equipment behavior visible together.
- [ ] 115.2 Revisit historical leads if present: quest 7073-7098; noshow 50403/33033; god 1294;
  hiltless dagger 21005; and disagreements 1011, 4798, 20112, 20261, 21721, 58634, 58912,
  58916, 59366, 83036, 83217, 91237, 94534, and 94719. Search the refreshed audit for new
  cases as well. Record keep/change/exclude decisions and the builder rationale.
- [ ] 115.3 Verify throwing intent versus melee javelins, thrown darts versus blowgun ammo,
  quiver capacity, launcher/ammunition compatibility, and conservative archer loadouts without
  eligible ammunition. Retain existing enhancement/durability regressions. Review every override
  against its current source record, rather than treating the historical override counts as fixed.
- [ ] 115.4 Commit only approved overrides or demonstrated rule fixes, with focused regressions
  in `test_rol_weapon_mapping.py` and the existing transform suite. Preserve coverage of the
  actual target tables and emitted-file parsing; use `test_thrown_weapons.c` for runtime
  throwing behavior that changes. Do not add another simulated combat implementation.
- [ ] 115.5 Regenerate the final audit and candidate. Exercise wield, fire, throw, quiver use,
  and representative mobile loadouts during step 6. Attach the reviewed report and runtime
  results to the issue when implementing; an unchecked audit report is not builder sign-off.

Exit: no undefined target weapon/ammunition type, unreviewed fallback, stale override, or
unresolved disagreement remains. Accepted fallbacks may remain with recorded justification.
Actual emitted equipment works in development and the final conversion evidence passes.

## Step 6: combined development rehearsal and branch completion

- [ ] 6.1 Generate the baseline/discovery/plan/capability/special evidence required by the final
  rules, then Phase 7 milestones at batches 4/8/12 and an independent final repeat. Reuse valid
  inputs only when their hashes and dependencies still match. The candidate must contain the
  complete package/dependency closure required by Phase 8. Use its existing preservation,
  diagnostics, reference, reset, shop, quest, SOC, trap, procedure, and mechanics-isolation checks.
- [ ] 6.2 Run `make test-world-tools`, then `make test` and `make install`. Capture complete
  logs and exit statuses. `make install` must follow the root tests; do not leave a root-level
  `luminari` artifact. Validate the complete staged candidate and run the read-only development
  persistence gate. Apply the existing Phase 7/8 finding gates and review every changed warning;
  unrelated baseline warnings do not require cleanup. Use fresh logs for the final code/binary.
- [ ] 6.3 Syntax-boot the staged candidate, then run a bounded staff-login and behavior smoke
  test through `autorun.sh` in an isolated development runtime root on an unused port. Use its
  existing project-root, binary-directory, lib-directory, and port overrides; preserve any
  already running development server. Capture the MUD's own log, converted-zone resets, and
  graceful shutdown, as required by `rol_phase8._code_gates()`.
- [ ] 6.4 Include armor equip/unequip AC and penalties, tail/ring behavior, item levels,
  representable extensions/procs, displayed colors, ship lighting, weapons/ammunition, and
  approved player kits/help in the smoke transcript. Ollama, I3, and Discord availability are
  outside this branch's verification scope. Use no production service or production database.
- [ ] 6.5 Seal the candidate with the existing `rol-phase8` command and fresh world-tools,
  CuTest, install, syntax, and runtime logs. Verify output-affecting files and policies are in
  the code evidence and all source/target/binary preconditions match.
- [ ] 6.6 Rehearse the exact apply-plan on a disposable development lib whose starting world
  matches the frozen baseline. Keep a verified snapshot of its complete world, then use
  `rol-phase8-apply` and `rol-phase8-completion` to prove the candidate and no-op reapplication.
  Restore the disposable world and compare it with the snapshot. Use existing copy/hash tools;
  the apply tool provides no backup or rollback. Record the actual rehearsal destination.
- [ ] 6.7 Update durable conversion documentation, release notes, and this checklist with the
  actual decisions, commands/results, evidence locations/hashes, remaining approved losses,
  and commit references. Review each issue's original checklist against that evidence. Keep
  an issue open if its required review or runtime proof is missing. Publish implementation and
  closure evidence when executing the plan; production release remains a separate scope.

Exit: all five issue gates pass against the same final revision and candidate, development
rehearsal/recovery/idempotency evidence is available, affected help entries agree, and the
branch contains only the work required for these issues. No unreviewed disposition, fabricated
test result, skipped required corpus test, or pending design choice is counted as completion.

## Verification commands and evidence

Run these from the repository root during implementation. Define `ROL_SOURCE_ROOT` as the
selected source snapshot, `ROL_BASELINE_WORLD` as the frozen target world, `ROL_RUN_DIR` as a
new evidence directory, `ROL_CANDIDATE_LIB` as the isolated complete candidate lib, and
`ROL_DEVELOPMENT_LIB` as the verified development lib containing the existing database config.
Use distinct fresh output directories for each run. The commands below are templates, not
evidence that generation, tests, database access, or runtime execution has occurred.

```sh
python3 scripts/world/wtool.py --world-root "$ROL_BASELINE_WORLD" rol-baseline \
  --source-root "$ROL_SOURCE_ROOT" --output-dir "$ROL_RUN_DIR/baseline"
python3 scripts/world/wtool.py --world-root "$ROL_BASELINE_WORLD" rol-discover \
  --source-root "$ROL_SOURCE_ROOT" --output-dir "$ROL_RUN_DIR/discovery"
python3 scripts/world/wtool.py rol-plan \
  --discovery-dir "$ROL_RUN_DIR/discovery" --output-dir "$ROL_RUN_DIR/plan"
```

For focused Python iteration, use the existing suites at their current paths:

```sh
PYTHONPATH=scripts/world python3 -m unittest \
  tests.test_rol_source tests.test_rol_transform tests.test_rol_weapon_mapping \
  tests.test_objects tests.test_rol_phase8
```

Run `make test-world-tools` before and after the mechanical split. Run the full block below
once the final implementation is ready; during iteration, rerun only checks affected by a change
or unresolved failure. Capture logs with exit-status preservation; Phase 8 consumes complete logs.

```sh
make test-world-tools
make test
make install
python3 scripts/world/wtool.py --world-root "$ROL_CANDIDATE_LIB/world" validate --all
python3 scripts/world/wtool.py --world-root "$ROL_CANDIDATE_LIB/world" --json \
  rol-persistence-check --development-lib-root "$ROL_DEVELOPMENT_LIB"
bin/luminari -c -d "$ROL_CANDIDATE_LIB"
```

Use the capability/special/Phase 7/Phase 8 invocation sequence in
[WORLD_VALIDATOR_CLI.md](../utilities/WORLD_VALIDATOR_CLI.md#realms-of-luminari-phase-7-and-phase-8)
and `python3 scripts/world/wtool.py <command> --help`, substituting the frozen world and
disposable rehearsal lib for examples using `lib/world` or `lib`. Record validator findings and
exit status; release acceptance uses the existing baseline-delta and selected-record gates.

The native C suite has no individual-test filter: `make test` runs the production-linked
executable and its required checks. Extend
existing test files first; if a new C source/test file is necessary, register it in both build
systems and in the appropriate CuTest source/test lists. Accept hook formatting and revalidate
any resulting code changes before committing.

For parser/emitter changes, the primary proof is source-parse to emit to target-parse coverage,
plus production loader/runtime checks for changed semantics. For the mechanical split, retain
byte comparisons. For gameplay, protect observable behavior and boundaries rather than simply
copying the implementation into a test. Do not add infrastructure or unrelated test coverage.

Planning checks performed on 2026-09-06: live issue bodies and current source/tests/config were
read; `unittest.TestLoader.discover('scripts/world/tests', top_level_dir='scripts/world')`
found 509 tests with no discovery errors; declared Phase 8 evidence paths were checked for
existence; and `python3 scripts/world/wtool.py rol-phase8 --help` succeeded. No test methods,
conversion run, database write, syntax boot, or gameplay smoke test was executed for planning.
The plan-ablation review rechecked live issue scope, module ownership, evidence/apply gates,
object and spell contracts, and validation commands. Implementation checkboxes remain open.

## Revision-pinned design evidence

These historical studies contain the detailed cases and full class matrices. Reconcile them
with current code and the selected source package; they are not fresh counts, accepted design
decisions, or runtime proof. Keep them revision-pinned instead of restoring retired notes.

- [Armor inference study](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/ROL_CONVERTER_ARMOR_TYPE_INFERENCE.md)
- [Source/target object format mapping](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/OBJ_FILE_FORMAT_MAPPING.md)
- [Remaining object mapping references](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/ROL_CONVERTER_OBJECT_FILE_REFERENCES.md)
- [Weapon review study](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md)
- [Parser/emitter organization study](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/todo-zusuk/ROL_CONVERTER_FILE_ORGANIZATION_SCOPE.md)
- [Player-class and per-class spell matrices](https://github.com/LuminariMUD/Luminari-Source/blob/c7c7d44a7f47e5fc155859eaf359391b827f85ea/docs/ongoing-projects/ROL_CLASS_EQUIVALENCE_GAPS.md)
