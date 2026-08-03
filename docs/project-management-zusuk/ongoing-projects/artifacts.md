# Artifact System Project Notes

## 0. Second-wave implementation status

This section tracks the in-progress work on the thirteen requested features
and the six HomelandMUD candidate artifacts. It is updated as work lands so
an interrupted session can resume from here.

Item 4 of the original request, the original-and-echo model, is **REJECTED**
and is not being implemented. Section 16 and section 18.3 below remain as
research notes only.

### 0.1 Feature status

| # | Feature | Status | Where |
| --- | --- | --- | --- |
| 1 | Public artifact chronicle/roster | Done | `artifact roster`, `artifact chronicle <name>` |
| 2 | Provenance and custody history | Done | `struct artifact_data` provenance block, v2.3 file |
| 3 | Acquisition and release policies | Done | `artifact_contracts[]` |
| 4 | Original-and-echo model | REJECTED | not implemented |
| 5 | Group-targeted artifact powers | Done | `ART_TARGET_GROUP_ROOM`, `ART_EFFECT_GROUP_VALOR` |
| 6 | Reusable signature-proc types | Done | `ART_SIG_*` library |
| 7 | Proc stacking and exclusion groups | Done | `ART_STACK_*`, `SPELL_ARTIFACT_SURGE` |
| 8 | Data-driven invocation channels | Done | `ART_INVOKE_*`, one matcher, `invoke` command |
| 9 | Progressive passive powers | Done | `artifact_passives[]` |
| 10 | Discovery-driven lore and clues | Done | `artifact_lore_stage()`, staged chronicle text |
| 11 | Persistent cooldowns | Done | v2.3 ownership format |
| 12 | Stronger metadata validation | Done | `artifact_validate_metadata()` |
| 13 | Safer recovery tooling | Done | `testartifact recover`, hardened `spawn`/`reload` |

### 0.2 New artifact status

All six candidates are registered, given a content contract, and reset into
the vault. Vnums are allocated in the existing artifact zone 1699.

| Vnum | Name | Signature proc | Called effect | Passives | Acquisition |
| --- | --- | --- | --- | --- | --- |
| 169913 | Vengeance | `ART_SIG_MERCY`, evil-gated offense branch | none | 3 | quest |
| 169914 | Earthcrier | `ART_SIG_KNOCKDOWN`, non-good wielder | none | 2 | boss |
| 169915 | Wyrmfang | `ART_SIG_WEIGHTED` | `invoke hunt` | 5 | boss |
| 169916 | Courage | none | `say courage` (group) | 4 | staff event |
| 169917 | Icedge | `ART_SIG_FLURRY` | `whisper <x> rime` | 3 | exploration |
| 169918 | Twilight | `ART_SIG_SURGE` | none | 4 | recovery only |

### 0.3 Files changed

- `src/magic/spells.h` - added `SPELL_ARTIFACT_PASSIVE` and `SPELL_ARTIFACT_SURGE`
  affect markers in the reserved 1606-1646 band.
- `src/spec_artifacts.h` - vnums, chronicle/acquisition/channel/proc/
  stacking constants, provenance and contract fields, new API.
- `src/spec_artifacts.c` - all runtime work.
- `src/interpreter.c` - registered `invoke`.
- `src/act.comm.do_spec_comm.c` - whisper channel hook.
- `lib/world/artifacts/1699.obj` - six new object prototypes.
- `lib/world/artifacts/1699.zon` - six new vault resets.
- `lib/world/artifacts/artifacts.hlp` - chronicle and invoke help entries.
- `scripts/provision_artifacts.sh` - merges artifacts added to the package
  into an already-provisioned world instead of skipping the file entirely.
- `unittests/CuTest/test_artifacts.c` - regression coverage, 57 tests.
- `docs/systems/ARTIFACT_SYSTEM.md` - behavior documentation.
- `docs/CHANGELOG.md`, `configure.ac` - release notes and version bump.

### 0.4 Remaining work

- Sections 1 through 4 below (packaging, live placement, booted-world
  integration coverage, and the balance pass) are unchanged by this work and
  are still open. Section 2 in particular: the six new artifacts reset into
  the vault, so their contracts declare an intended acquisition route that
  world content does not yet implement.
- Section 10's world-content half is builder work: the code stages the lore
  and hint text and gates it by discovery, but wiring NPC dialogue to it is
  done with ordinary DG scripts against `artifact chronicle`.

### 0.5 Verification performed

- `make -j$(nproc)` clean, no warnings.
- `make test` green at 145 tests, up from 133.
- `make install`; no stray root-level `circle` binary left behind.
- Booted `./bin/circle -d lib` against the development database: the log
  reads `Artifacts: initialized 17 artifacts.` with no metadata SYSERRs, and
  `lib/world/world.artifact` was rewritten in v2.3 form with a pre-existing
  v2.2 owner carried across and no provenance invented for it.
- `scripts/provision_artifacts.sh` run three times in a row: it added the six
  new prototypes and resets to an already-provisioned world on the first run
  and changed nothing on the next two.

---

Completed behavior is documented in
[`docs/systems/ARTIFACT_SYSTEM.md`](../../systems/ARTIFACT_SYSTEM.md), and
completed implementation work is recorded in
[`docs/CHANGELOG.md`](../../CHANGELOG.md).

Sections 1 through 8 track unfinished LuminariMUD work. The second half of
this file is the complete HomelandMUD artifact-system study requested for
future design and content work.

## 1. Put the deployment package under version control

`lib/world/artifacts/` contains the required zone, room, object, mobile, and
help sources, but `.gitignore` excludes the entire directory. A fresh clone
therefore has no input for `scripts/provision_artifacts.sh`, and both
`scripts/setup.sh` and `scripts/deploy.sh` fail when the provisioner tries to
copy the first missing source file.

Choose and implement one durable packaging model:

- move the package to a tracked data directory outside the OLC ignore rules;
- add a narrow Git exception for the package; or
- generate the complete package from a tracked source.

Acceptance criteria:

- a fresh clone contains or can generate object prototypes 169901-169911,
  vault room 169900, zone 1699, Oaken Defender mobile 169912, and artifact
  help entries;
- `scripts/provision_artifacts.sh` succeeds on a fresh clone and remains
  idempotent;
- setup and deployment no longer depend on files retained from a developer
  machine;
- `Test_artifact_world_package_contains_all_deployable_records` is either
  made hermetic or moved out of the unit suite into a deployment check.

## 2. Place artifacts in player-facing content

All eleven artifacts currently reset into the private Vault of Ages room
169900. This is suitable for staff staging but does not make them obtainable
through gameplay.

A builder must decide what content gates each artifact, then move the
relevant `O` reset commands into live zones or load the artifacts onto
selected mobiles. The existing single-instance guard covers `O`, `P`, `G`,
and `E` reset commands.

Acceptance criteria:

- every intended artifact has a documented acquisition path;
- placement works in each supported campaign where the artifact should
  exist;
- reboot and zone-reset testing confirms that ownership never creates a
  duplicate instance.

## 3. Add booted-world integration coverage

The production-linked unit suite covers the registry and logic that can run
without a loaded world. It does not drive a real player through the complete
feature.

Add an integration fixture that verifies:

- acquire, equip, bind, unequip, drop, save, reload, and destroy lifecycles;
- character-bound and account-bound rejection paths;
- level-scaled bonuses and highest-only resistance;
- all three active abilities;
- generic procs and all five signature procedures;
- called-effect success, refusal, and independent recharge behavior;
- class-oath burn damage and phrase hiding;
- player and staff command output;
- single-instance behavior across reboot and zone reset.

## 4. Perform a gameplay balance pass

The called effects and signature procedures follow the shape of the upstream
system but were scaled by hand for LuminariMUD and have not had a sustained
playtest.

Review at least:

- `bring annhilation forth`, currently
  `dice(10 + artifact_level * 4, 12) + character_level * 3` against every
  valid hostile target in the room;
- Kelrarin's 350-point mega blast and NPC sudden-death follow-up;
- Kelrom's group healback, which scales from 10% to 50% of triggering
  damage;
- the 2000-max-HP Horn of Henekar charm threshold;
- XP rates for critical and boss-tier combat, called effects, and
  multi-target Doom Blast;
- the ten-class-level oath threshold and `5d4` burn penalty.

Record approved balance changes in the changelog and update
`docs/systems/ARTIFACT_SYSTEM.md`.

## 5. Decide whether cooldowns must survive reboot - RESOLVED

Resolved in favour of persistence. The v2.3 ownership format stores the
ability, proc, and per-slot called-effect stamps; v1, v2.0, v2.1, and v2.2
files still load, and a stamp in the future is treated as ready rather than
as a longer wait.

## 6. Validate the effect table at boot - DONE

Implemented as `artifact_validate_metadata()`, run from `artifact_boot()` and
re-runnable from `testartifact verify`. It covers templates, contracts,
effects, and passive powers, logs a precise SYSERR naming the offending row,
and disables only the invalid effect. Coverage includes registry-resolvable
VNUMs, in-range and unique recharge slots, nonempty pre-normalized phrases,
and valid target and effect identifiers.

## 7. Fix Amaukekel's group recall ordering - DONE

Fixed with `artifact_collect_group()`, which snapshots the origin room and
the eligible members before anything moves. The same helper drives Courage's
group invocation. The prior defect was that `artifact_dimension_shift()`
recalled the caller before iterating, so the same-room check compared each
member's origin against the caller's destination and skipped nearby members.

## 8. Harden staff spawn and reload - DONE

`spawn` now refuses a durably owned artifact and changes nothing when it
refuses; `testartifact recover` is the audited override. `reload` flushes
dirty state first unless given the explicit `discard` mode. The prior gaps
were `spawn` accepting an artifact durably owned by an offline player (and
clearing `instance_persisted` in the process), and `reload` rebuilding the
registry without flushing dirty sub-threshold XP.

---

# HomelandMUD Artifact System Study

## 9. Executive finding

HomelandMUD does not contain a second implementation comparable to
LuminariMUD's current levelable, binding-aware, single-instance artifact
system. Its core is a much smaller public custody ledger:

- nine object VNUMs are hard-coded as artifacts;
- the last character passed each object through `obj_to_char()` is saved as
  its owner;
- every player can run `artifacts` to see the loaded artifact prototypes and
  those saved names;
- artifact powers are ordinary object special procedures and permanent
  object affects, not part of the ownership ledger; and
- uniqueness is described in help text and apparently managed through
  immortal-run releases, but is not enforced by code.

The source nevertheless contains material worth retaining:

1. six complete named artifact prototypes that are not in LuminariMUD's
   current eleven-item roster;
2. three more ledger entries whose prototypes are missing but whose special
   procedures preserve useful mechanics;
3. a "named original plus weaker general version" pattern for at least four
   surviving items and three missing pairs;
4. a public artifact chronicle or custody-roster concept;
5. staged release through staff events rather than only permanent zone
   placement;
6. new effect shapes: group buffs, control procs, conditional holy procs,
   temporary self-scaling combat surges, multi-strike bursts, and a whispered
   summon.

The Homeland ownership, persistence, and uniqueness code should not be
ported. LuminariMUD already has stronger implementations for all three.

## 10. Scope, provenance, and source path

The studied tree is:

`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD`

It is a nested Git checkout at commit
`0dfd8fc0053b2c5573463e43066ec3de669bd46f`, dated 2020-03-17. That checkout
has one commit, so there is no earlier local history from which to recover
the three missing object prototypes.

Every HomelandMUD path in this study is absolute. Line numbers refer to the
commit above.

### 10.1 Functional source map

| Area | Absolute HomelandMUD source |
| --- | --- |
| Registry structure, nine-VNUM membership switch, save/load, player listing | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:881-1010` |
| Acquisition hook | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/handler.c:700-752` |
| Boot call | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/db.c:383-396` |
| Public command declaration and registration | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/interpreter.c:117`, `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/interpreter.c:408` |
| Public declarations | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/utils.h:25-28` |
| Artifact and counterpart special-procedure assignments | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:766-835` |
| Missing-prototype assignment warning | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:198-204` |
| Weapon special dispatch, including critical marker | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/combat/fight.c:2420-2426`, `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/combat/fight.c:2645-2677` |
| Per-object special timers | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/limits.c:548-556` |
| Timer cadence | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/comm.c:799-800`, `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/utils.h:186-187` |
| Owner data snapshot | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/misc/artifacts:1-10` |
| Player-facing design statement | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/text/help/fullhelp.hlp:520-529` |
| Player-object restore paths that bypass the acquisition hook | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/pfile.c:670-715` |
| Quest reward path that does use the acquisition hook | `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/quest/quest.c:640-662` |

### 10.2 Exhaustive search result

The functional audit found:

- 9 VNUMs in `is_artifact()`;
- 9 persisted owner rows;
- 6 surviving object prototypes;
- 3 missing object prototypes: 501, 513, and 599;
- 7 artifact VNUMs assigned special procedures;
- 2 surviving artifact VNUMs with static powers only: 17022 and 43600;
- 0 `O`, `G`, `E`, or `P` zone-reset commands that load any of the nine
  artifact object VNUMs; and
- 0 quest input/output command records that directly require or award any of
  the nine artifact object VNUMs.

The Icedge name does occur in quest dialogue as lore and an acquisition
hint, but the quest does not load the object.

## 11. What HomelandMUD means by "artifact"

The help entry describes three related ideas:

1. unique items awarded through immortal-run quests;
2. hard-to-get items whose first copy is named and whose later copies have a
   generalized description; and
3. a planned smaller tier of "true artifacts" released only when staff
   decide the time is right.

Source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/text/help/fullhelp.hlp:520-529`.

The implementation does not encode those tiers. `is_artifact()` is only a
switch over nine VNUMs:

`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:889-905`.

There is no artifact object flag, registry template, release state,
first-copy marker, duplicate check, zone-reset guard, binding rule, artifact
level, artifact experience, or artifact-specific cooldown store. This means
"artifact" in Homeland is best understood as a socially recognized named
collectible with a public last-recipient record. Special powers are separate
object features.

## 12. End-to-end runtime trace

### 12.1 Boot and registry construction

`boot_db()` loads object prototypes and assigns object special procedures
before it calls `load_artifacts()`:

- special assignment:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/db.c:333-343`;
- artifact load:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/db.c:383-396`.

`load_artifacts()` scans the object prototype table. For each loaded
prototype accepted by `is_artifact()`, it pushes an entry containing the
VNUM and owner `"none"` into a global C++ `list<sArtifact>`. It then reads
the bundled owner file and overwrites matching owner names.

Source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:947-991`.

Consequences:

- a hard-coded VNUM whose prototype is absent never enters the live list;
- an ownership-file row cannot restore a missing registry member;
- missing prototypes are omitted from the player command;
- `push_front()` reverses prototype-table order;
- calling the loader again would append duplicate list entries because it
  never clears or frees the existing list; and
- the loop uses `i < top_of_objt`, even though that variable is the last
  valid index elsewhere in the code, so the final prototype can be skipped.

The special assignment layer separately logs a `SYSERR` when it tries to
assign a function to a missing object prototype:

`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:198-204`.

### 12.2 Acquisition and owner changes

Every normal `obj_to_char(object, ch)` call:

1. inserts the object into the character inventory;
2. saves the character;
3. calls `is_artifact(object)`; and
4. if true, replaces the matching registry name with `GET_NAME(ch)` and
   rewrites the whole artifact file.

Source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/handler.c:700-752`.

This is a last-recipient hook, not reliable ownership:

- it does not reject a duplicate instance;
- it does not distinguish a player from an NPC;
- giving or zone-loading an artifact to a mobile records the mobile name;
- it does not verify whether the recipient can use the object;
- it does not store an account, character ID, timestamp, or acquisition
  source;
- it updates on any later transfer, so the first discoverer is lost; and
- it saves even when the artifact list is empty or no matching row exists.

Quest rewards normally pass through `obj_to_char()` and therefore update the
ledger:

`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/quest/quest.c:650-660`.

A direct player-to-player give also passes through it:

`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.item.c:1418-1443`.

### 12.3 Paths the owner hook misses

Normal player-file restoration deliberately uses lower-level
`obj_to_char_tail()`, `raw_equip()`, and `obj_to_obj_tail()` for valid
inventory, equipped, and nested objects. Those paths do not call
`update_artifact_owner()`.

Source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/pfile.c:670-715`.

This is reasonable for an unchanged owner at login, but it exposes the
system's incomplete coverage:

- transferring a container does not recursively discover an artifact inside
  it;
- lower-level loads and equips can bypass the ledger;
- a nested artifact is updated only if it is later removed through a path
  that calls `obj_to_char()`; and
- no boot reconciliation compares the saved owner, live object graph, and
  player storage.

LuminariMUD's current nested traversal, persistence-extraction scope, and
live-instance reconciliation are all stronger and should remain
authoritative.

### 12.4 Drop, destruction, logout, and character deletion

HomelandMUD has no artifact calls in `obj_from_char()`, `obj_to_room()`,
`obj_to_obj()`, `extract_obj()`, player deletion, or character rename.
Accordingly:

- dropping an artifact leaves the previous name displayed as owner;
- destroying it leaves the previous name displayed;
- losing it in an untracked container leaves the previous name displayed;
- deleting or renaming the character leaves a stale string; and
- the saved name cannot prove that any instance still exists.

The ledger therefore means "last character observed receiving this VNUM",
not "current durable owner".

### 12.5 Persistence format and write behavior

The file is plain text:

```text
96081 Karaz
43600 Lathander
...
501 Nrok
$
```

Source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/misc/artifacts:1-10`.

Each record is `vnum single_token_name`; `$` terminates the file.
`save_artifacts()` writes `artifacts.new`, closes it, removes the old file,
then renames the new file.

Source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:908-926`.

Valuable idea:

- a complete small registry is cheap to rewrite after a claim.

Defects not to copy:

- removing the old file before rename creates a data-loss window and is not
  an atomic replacement;
- `fprintf`, `fclose`, `remove`, and `rename` results are ignored;
- there is no file version or migration path;
- malformed and duplicate rows are not validated;
- `sscanf()` has no field widths and its return value is ignored;
- EOF before `$` is not handled safely;
- the loader never closes its `FILE *`;
- the loader does not clear old list entries or old strings; and
- the format cannot represent accounts, spaces, timestamps, history,
  binding, progression, location, or instance state.

LuminariMUD's versioned temp-file write and atomic rename are already the
correct base.

### 12.6 Player command

`artifacts` is available to every player at `POS_RESTING` or higher:

`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/interpreter.c:408`.

It prints each loaded artifact prototype's short description and saved owner
name:

`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:994-1010`.

This is the clearest feature LuminariMUD does not presently expose. The
current Luminari `artifact list` shows only the caller's carried and equipped
artifacts, while the global roster is staff-only through `testartifact
list`.

The concept is useful, but Homeland's exact presentation is not:

- it reveals every loaded artifact and exact owner without discovery,
  consent, privacy, or campaign checks;
- it has no heading, status, lore, acquisition hint, or "unknown" state;
- it silently prints nothing if the list is empty;
- it uses `r_num > 0`, incorrectly excluding a valid prototype at real index
  zero; and
- stale owners are indistinguishable from verified owners.

### 12.7 Combat dispatch and called powers

Object special procedures are assigned by VNUM in `assign_objects()`.
After a successful weapon hit, `weapon_special()` calls the assigned
procedure and passes `"critical"` when the attack was a critical hit.

Sources:

- `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:766-835`;
- `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/combat/fight.c:2420-2426`;
- `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/combat/fight.c:2645-2677`.

These procedures are not connected to `sArtifact` or the owner file. A
generalized counterpart receives the same procedure independently.

Called powers use ordinary command dispatch. Courage listens for `say`;
VNUM 501 listens for `whisper`. This is broader than LuminariMUD's current
artifact dispatcher, which is called only from `do_say()`.

### 12.8 Recharge timers

Several ordinary object procedures use four in-memory `spec_timer[]` slots.
`point_update()` decrements them once per MUD hour:

- timer storage:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/structs.h:1141-1155`;
- decrement:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/limits.c:548-556`;
- one MUD hour is 120 real seconds:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/utils.h:186-187`.

The timers are not included in Homeland's player-object save format, so
reboot or object reload resets them. This does not solve LuminariMUD's open
cooldown-persistence question.

It also creates a concrete documentation defect in Courage: identify text
says "once a week", but the code sets `12 * 5`, or 60 MUD hours. At 120 real
seconds per MUD hour that is 7,200 real seconds, only two real hours and 2.5
MUD days.

Sources:

- `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:4844-4850`;
- `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:4854-4883`.

## 13. Authoritative nine-VNUM roster audit

| Homeland VNUM | Prototype in checkout | Identity supported by source | Special procedure | Saved snapshot name | Disposition |
| --- | --- | --- | --- | --- | --- |
| 501 | Missing | Xvim artifact or avenger; exact prototype name unknown | `xvim_artifact` | Nrok | Preserve mechanics as research only; do not invent canonical identity |
| 513 | Missing | Halberd; exact prototype name unknown | `halberd` | Dartan | Preserve mechanics as research only |
| 599 | Missing | Tormblade; exact prototype name unknown | `tormblade` | Dartan | Preserve mechanics as research only |
| 1199 | Present | Vengeance | `vengeance` | Gosric | Candidate artifact |
| 1850 | Present | Earthcrier | `skullsmasher` | Vari | Candidate artifact |
| 17022 | Present | Wyrmfang, the Spear of Dragons | None | Gosric | Candidate artifact |
| 39250 | Present | Courage | `courage` | Ranon | Candidate artifact |
| 43600 | Present | Icedge, the Dagger of Cold | None | Lathander | Candidate artifact |
| 96081 | Present | Twilight, the Sword of Destruction | `twilight` | Karaz | Candidate artifact |

Membership source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:889-905`.

Snapshot-owner source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/misc/artifacts:1-10`.

The saved names prove only the state of the bundled snapshot. They are not
evidence that the objects or players still exist.

The three saved rows whose prototypes are now missing strongly suggest that
the snapshot came from a deployment with different or older world data, or
that obsolete rows were retained. This is an inference, not enough evidence
to reconstruct any missing prototype or canonical name.

## 14. Candidate inventory: six complete named artifacts

These six have complete object records and are additions to the research
inventory. They are not yet approved LuminariMUD registry entries, assigned
new VNUMs, balanced, or placed in content. They appear in
`docs/systems/ARTIFACT_SYSTEM.md` only in its explicitly non-runtime
research-candidate inventory.

### 14.1 Static mechanical inventory

The weapon dice, modifiers, flags, and permanent affects below are decoded
from the object records using Homeland's object format and constants:

- object parser:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/db.c:1363-1497`;
- item, wear, extra-flag, and apply constants:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/structs.h:670-806`;
- permanent-affect names:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/constants.c:294-399`.

| Artifact | Weapon | Static modifiers | Permanent states | Important restrictions |
| --- | --- | --- | --- | --- |
| Vengeance | 2d8 slashing, wield | +4 hit, +4 damage | Protection from evil, minor globe | Good-aligned and paladin-oriented mask; no-locate, no-burn |
| Earthcrier | 8d4 bludgeoning, two-handed | +8 hit, +6 damage | None | Anti-good, no-drop, no-sell, no-locate, no-burn; several class exclusions |
| Wyrmfang | 8d4 piercing, two-handed | +8 hit, +8 damage | Detect invisibility, sense life, infravision, farsee, haste, danger sense | Anti-evil, no-drop, no-burn; several class exclusions |
| Courage | 3d6 crushing, wield | +35 HP, +8 hit | Haste, protection from lightning, no-sleep, brave | Cleric/druid-oriented mask; magical light, no-burn |
| Icedge | 4d7 piercing, wield | +6 hit, +6 damage, +2 magic resistance | Protection from cold | No-drop, no-sell, magical light, no-locate, no-burn |
| Twilight | 8d4 slashing, two-handed | +8 hit, +8 damage | Sense life, infravision, farsee, haste | Hidden; excludes mage, cleric, and rogue |

These numbers are historical design evidence, not recommended LuminariMUD
balance values. Luminari's artifact bonuses scale across five artifact
levels, while Homeland's values are flat.

### 14.2 Vengeance

Canonical source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/obj/10.obj:715-742`.

Identity and lore:

- a sacred holy sword named Vengeance;
- marble hilt, ruby pommel, and holy symbols of Tyr, Torm, and Ilmater;
- explicitly described as a long-crafted masterpiece.

Procedure:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:3794-3832`.

On roughly one hit in sixteen:

- if the wielder is injured, four of five successful proc branches heal
  `100 + 4d25`; or
- otherwise it invokes `SPELL_HOLY_WORD`.

The unnamed sacred holy avenger at VNUM 10017 has the same 2d8 base,
+4 hit/+4 damage, protection from evil, and minor globe, but procs only
about one hit in twenty-six and heals `60 + 4d15`.

Counterpart sources:

- prototype:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/obj/100.obj:229-253`;
- world placement inside object 10016:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/zon/100.zon:25`;
- shared procedure assignment:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:803-804`.

Value to LuminariMUD:

- strong holy/paladin artifact identity;
- a conditional signature proc that selects sustain while wounded and
  offense while healthy;
- an anti-evil power package not represented by the current roster; and
- a clean original/echo pairing.

Porting requirements:

- replace legacy class bitmasks with a Luminari class oath or explicit use
  policy;
- adapt deity lore by campaign;
- use normal Luminari damage, healing, saving-throw, and target rules;
- balance both branches; and
- do not cast an unrestricted room spell merely because a weapon proc fired.

### 14.3 Earthcrier

Canonical source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/obj/18.obj:36-55`.

Identity and lore:

- a named, rune-etched mithril maul;
- a large, heavily weighted presentation through its two-handed record;
- evil or antihero restrictions in its historical flag mask.

Procedure:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:3613-3647`.

On roughly one hit in sixteen, a material and bashable target is forced to
sitting and receives one combat-pulse wait state. Targets with `MOB_NOBASH`
or innate immateriality are immune.

The unnamed VNUM 1849 uses the same 8d4 base, +8 hit, +6 damage, and
procedure, but only procs about one hit in twenty-six.

Counterpart sources:

- prototype:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/obj/18.obj:17-35`;
- 50 percent mobile equipment reset:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/zon/18.zon:18-20`;
- shared procedure assignment:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:805-806`;
- historical "unnamed clone" confirmation:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/text/news:172-176`.

Value to LuminariMUD:

- a two-handed control artifact;
- a signature knockdown/stagger proc, absent from the current artifact
  roster; and
- a clear demonstration that artifact control effects need immunity and
  saving-throw policy.

Porting requirements:

- use Luminari combat-position and action-economy helpers;
- honor size, stability, incorporeal, boss, and control-immunity rules;
- add a save or combat maneuver check;
- apply the existing artifact proc internal cooldown; and
- remove the unused Homeland `ITEM_AUTOPROC` behavior from the design.

### 14.4 Wyrmfang, the Spear of Dragons

Canonical source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/obj/170.obj:400-424`.

Identity and lore:

- a long night-black spear made from a gargantuan black dragon horn;
- covered in arcane symbols and runes;
- focused on awareness, speed, and heavy martial statistics.

There is no special-procedure assignment for VNUM 17022. Its identity comes
entirely from the prototype, permanent affects, and restrictions.

Value to LuminariMUD:

- a perception and pursuit artifact rather than another activated nuke;
- a spear, a weapon type not represented in the current eleven artifacts;
- a potential dragon-hunter or dragon-tainted progression theme; and
- evidence that `artifact info` needs to describe always-on status grants if
  such powers are adopted.

Porting requirements:

- decide whether haste, farsee, danger sense, and detection belong in the
  object prototype or in `artifact_templates[]`;
- avoid giving every historical permanent state at artifact level 1;
- define campaign-specific black-dragon lore; and
- consider unlocking senses by artifact level rather than scaling only
  numeric bonuses.

### 14.5 Courage

Canonical source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/obj/392.obj:25-40`.

Identity:

- a golden mace named Courage;
- +35 HP, +8 hit, haste, lightning protection, no-sleep, and brave;
- cleric/druid-oriented historical restrictions.

Called power:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:4844-4883`.

Saying `courage` while wearing the mace applies both vitality and courage to
the wielder's group, or to the wielder when solo.

The generalized VNUM 39251:

- is named only "a golden mace";
- deals 3d5 instead of 3d6;
- grants +30 rather than +35 HP;
- lacks haste; and
- shares the called group power.

Counterpart sources:

- prototype:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/obj/392.obj:41-57`;
- shared procedure assignment:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:807-808`.

The combat branch at
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:4890-4902`
is unfinished: it rolls a chance, contains only a TODO, and then returns
success on every combat call. It has no mechanic worth porting.

Value to LuminariMUD:

- the best complete new called effect in Homeland;
- an artifact that supports the whole group instead of primarily rewarding
  personal damage;
- a natural cleric, druid, marshal, or leadership-themed item; and
- a concrete extension for `artifact_do_effect()`: a group-buff target/effect
  type.

Porting requirements:

- use a real recharge constant and display generated from the same data;
- snapshot or safely iterate the group if spell effects can move or extract
  members;
- require same-room and eligibility checks;
- prevent duplicate or abusive stacking;
- adapt the two spells to Luminari equivalents; and
- discard the dead combat branch.

### 14.6 Icedge, the Dagger of Cold

Canonical source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/obj/436.obj:1-24`.

Identity and lore:

- an ice-blue adamantine dagger with a dragon-tooth and platinum pommel;
- one of the legendary daggers of Ochir Naal, Prophet of Tiamat;
- static cold protection and magic resistance.

No special procedure is assigned. A quest NPC provides a world clue:

- the Cult of the Dragon once held Icedge;
- an ice creature stole it during a sudden blizzard; and
- the creature may have taken it to its master.

Lore source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/qst/24.qst:1840-1875`.

Value to LuminariMUD:

- a dagger artifact, absent from the current roster;
- an exploration-led acquisition model where dialogue reveals a chain of
  clues rather than directly awarding the item;
- a cold-defense artifact with Tiamat-adjacent lore that can complement or
  deliberately contrast Tiamat's Stinger; and
- an example of meaningful artifact lore living in world dialogue.

Porting requirements:

- decide whether the Tiamat link is compatible with the selected campaign;
- build an actual acquisition chain rather than only copying the clue;
- add a signature cold effect or level-unlocked powers if flat resistance is
  not enough to distinguish it; and
- avoid duplicating Tiamat's Stinger's role.

### 14.7 Twilight, the Sword of Destruction

Canonical source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/obj/960.obj:1245-1264`.

Identity and lore:

- a huge black sword of unknown metal;
- fire-giant runes name it Twilight;
- permanent sense life, infravision, farsee, and haste;
- +8 hit and +8 damage.

Procedure:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/jotunheim.c:467-510`.

On roughly one hit in seventeen, if neither a Twilight surge nor a
Malevolence surge is already active, it adds the wielder's current hit roll
to hit roll and current damage roll to damage roll for two ticks. In effect,
it snapshots and temporarily doubles both values.

The generalized VNUM 96090:

- is a black longsword of destruction;
- retains 8d4, +8 hit, and +8 damage;
- lacks permanent haste;
- procs about one hit in nineteen; and
- uses the same mutually exclusive surge type.

Counterpart sources:

- prototype:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/obj/960.obj:1412-1432`;
- procedure:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/jotunheim.c:422-465`;
- shared assignments:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:828-835`;
- expiration messaging:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/magic/spells.c:102-108`.

Value to LuminariMUD:

- a fire-giant/destruction artifact with strong visual identity;
- a temporary combat-surge proc rather than direct damage;
- a useful "exclusive proc family" concept that prevents incompatible rage
  effects from stacking; and
- another complete original/echo pair.

Porting requirements:

- never copy the raw current-stat doubling formula; it compounds every other
  bonus and is difficult to balance;
- use a bounded, artifact-level-scaled hit/damage bonus;
- source-tag the temporary affects;
- give the surge an explicit stacking group or exclusion tag;
- honor the artifact proc internal cooldown; and
- test expiration after unequip, death, dispel, and artifact level changes.

## 15. Three prototype-missing artifacts

These entries belong in the research inventory, but not in an approved
artifact roster under invented names. Their VNUMs remain in the membership
switch and owner file, and their procedure assignments preserve mechanics.

### 15.1 VNUM 501: Xvim artifact or avenger

Evidence:

- membership:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:893`;
- assignment:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:783-785`;
- procedure:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:5224-5328`;
- saved recipient:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/misc/artifacts:9`.

Recoverable behavior:

- about two rolls in thirty-six begin a four-to-six additional-hit burst;
- another rare nested roll deals 600-800 raw damage and lags the room;
- three rolls in thirty-six can heal an injured wielder by up to 90;
- `whisper nightmare` summons a globally limited charmed nightmare; and
- the procedure's messages call the object an avenger tied to Iyachtu Xvim.

Defects:

- the object prototype and canonical name are missing;
- the multi-hit counter is a global `force_blur`, not per character or item;
- the high-damage branch bypasses normal Luminari damage handling;
- the summon has no per-object recharge;
- the summon cap is global prototype count rather than owner capacity; and
- line 5322 assigns the new HP expression to the player's current HP while
  setting only the pet's maximum HP.

Potential value:

- a whispered invocation channel;
- a level-scaled signature companion;
- a bounded flurry proc; and
- a dark-avenger artifact concept.

Use those shapes only. Do not port the raw procedure.

### 15.2 VNUM 513: missing halberd

Evidence:

- membership:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:894`;
- assignment:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:772-773`;
- procedure:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:3265-3346`;
- saved recipient:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/misc/artifacts:8`.

Recoverable behavior on a roll from 0 through 30:

- one result stuns for `5 + 3d4`;
- one result grants two extra non-proccing attacks;
- two results apply slowness and 50 + 5d10 direct damage; and
- all other results do nothing.

Potential value:

- a polearm artifact;
- a multi-outcome control proc; and
- an identify description generated from actual proc capabilities.

The name, lore, prototype stats, and restrictions cannot be recovered from
this checkout.

### 15.3 VNUM 599: missing Tormblade

Evidence:

- membership:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:895`;
- assignment:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:812-813`;
- procedure:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:3348-3393`;
- saved recipient:
  `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/misc/artifacts:7`.

Recoverable behavior, only against evil targets:

- a critical hit grants two ticks of protection from evil, biofeedback, and
  -20 AC; and
- a non-critical hit has about a one-in-thirty-one chance to invoke dispel
  magic.

Potential value:

- a reactive anti-evil critical proc;
- a conditional dispel proc; and
- a proc whose behavior depends on target alignment.

The prototype and canonical name are missing. "Tormblade" is the procedure
identifier, not sufficient evidence for a final display name.

## 16. The named-original and general-version pattern

Homeland's most distinctive content pattern is not its owner file. It is the
pairing of a named first edition with a weaker or less prestigious general
version.

| Named artifact | General counterpart | Shared identity | Named-version advantage |
| --- | --- | --- | --- |
| Vengeance, 1199 | Sacred holy avenger, 10017 | Same base dice, bonuses, permanent protection, and proc family | Proc 1-in-16 vs 1-in-26; larger heal; named lore |
| Earthcrier, 1850 | Wicked mithril maul, 1849 | Same base dice, bonuses, and knockdown family | Proc 1-in-16 vs 1-in-26; named lore |
| Courage, 39250 | Golden mace, 39251 | Same invocation and most defenses | 3d6 vs 3d5, +35 vs +30 HP, permanent haste, named lore |
| Twilight, 96081 | Longsword of destruction, 96090 | Same base dice, bonuses, and surge family | Proc 1-in-17 vs 1-in-19, permanent haste, named lore |
| Missing 501 | Missing 502 | Xvim avenger procedure family | Stronger damage/proc variant and nightmare invocation |
| Missing 513 | Missing 510 | Halberd procedure family | Unknown |
| Missing 599 | Missing 596 | Tormblade procedure family | Unknown |

Pair-assignment source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:772-813`.

Twilight pair source:
`/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:828-835`.

This pattern can fit LuminariMUD if represented explicitly:

- the original remains the one registry-controlled, levelable artifact;
- an echo or replica is an ordinary non-artifact object with weaker static
  powers;
- the echo never participates in artifact ownership, binding, progression,
  cooldown, or single-instance checks;
- the echo uses a different VNUM and unmistakable display text;
- `artifact info` identifies the original as such;
- lore can explain how echoes entered the world; and
- builders can place echoes in repeatable content without weakening the
  original's uniqueness.

This should be optional per artifact. It is especially suitable for
Vengeance, Earthcrier, Courage, and Twilight because the source already
contains complete paired evidence.

## 17. Capability comparison with current LuminariMUD

| Capability | HomelandMUD | Current LuminariMUD | Finding |
| --- | --- | --- | --- |
| Membership | Nine hard-coded switch cases | Registry built from validated templates and loaded prototypes | Keep Luminari |
| Uniqueness | No enforcement | Live-object and durable-owner zone-load guards | Keep Luminari |
| Ownership | Last recipient name | Character and account ownership with durable-instance state | Keep Luminari |
| Binding | None | Pickup, equip, account, or no binding | Keep Luminari |
| Drop/destruction lifecycle | No updates | Explicit release and persistence-safe extraction | Keep Luminari |
| Nested containers | Incomplete | Recursive acquisition/drop tracking | Keep Luminari |
| Progression | None | Five levels and cumulative XP | Keep Luminari |
| Public global roster | Yes, raw owner list | No; personal list and staff global list only | Adapt Homeland concept |
| First-discoverer history | Intended socially, overwritten technically | Not persisted | Add a real chronicle if desired |
| Staff-timed release | Described in help | Possible manually, not modeled | Add release/acquisition metadata |
| Original plus replica | Present in content | Not modeled | Valuable optional addition |
| Invocation channels | `say` and `whisper` in different procs | `say` only | Generalize only if a candidate needs it |
| Group called buff | Courage | No equivalent called-effect type | Valuable addition |
| Control weapon proc | Earthcrier and missing halberd | Fear only; no knockdown signature | Valuable addition |
| Conditional holy proc | Vengeance and missing Tormblade | No equivalent | Valuable addition |
| Temporary exclusive surge | Twilight | No proc-family exclusion model | Valuable addition |
| Passive status package | Prototype permanent-affect bits | Artifact template documents numeric bonuses/resistance only | Decide and document one source of truth |
| Cooldown persistence | No | No | Homeland does not close the existing gap |
| Validation/testing | None found | Production-linked tests and staff verification | Keep and extend Luminari |

## 18. Recommended additions, in priority order

### 18.1 Add an artifact chronicle or public roster

This is the clearest low-risk feature inspired by Homeland.

Suggested player command:

```text
artifact roster
```

Recommended display policy:

- show artifact name only after global discovery, unless lore intentionally
  makes it public from the beginning;
- show `unawakened`, `unclaimed`, `held`, `lost`, or `destroyed/recoverable`
  rather than exposing internal flags;
- make exact owner display a per-artifact policy or an owner opt-in;
- otherwise show a historical epithet such as "last known bearer";
- include a short lore line or acquisition hint, not a room/VNUM;
- hide artifacts not enabled for the active campaign; and
- derive state from the authoritative registry, never from a second list.

New persistent fields worth considering:

- `first_owner`;
- `first_account`;
- `first_claimed_at`;
- `last_claimed_at`;
- `claim_count`; and
- `discovered`.

Do not overload `owner` with history. If a full custody chain is wanted, use
an append-only history table or journal with claim, transfer, drop, destroy,
reset, recovery, and staff-override events.

This work can share the already contemplated v2.3 migration with persisted
cooldowns, avoiding two consecutive artifact-file format changes.

### 18.2 Model acquisition and release policy

Current project task 2 requires a documented acquisition path for every
artifact. Homeland's help makes staff-run releases a first-class social
option.

Add documentation or template metadata for:

- acquisition type: boss, quest, exploration chain, staff event, seasonal
  event, or recovery;
- campaign availability;
- public lore and secret builder notes;
- release policy: permanent world placement, scheduled window, or manual
  one-time release;
- replica VNUM, if any; and
- whether owner identity is public.

The metadata does not need to drive every world reset. Its first job is to
make the content contract explicit and keep the roster, help, and builder
plan aligned.

### 18.3 Support optional artifact echoes

Implement the original/echo model described in section 16 only after the
deployment package and live acquisition paths are resolved.

Minimum invariants:

- only the original VNUM is in `artifact_templates[]`;
- echo VNUMs fail `artifact_is_artifact()`;
- echo loads never affect original ownership or `instance_persisted`;
- echo procedures cannot mutate the original's cooldown or XP;
- names and identify output cannot mislead a player about which is unique;
- original and echo balance are tested independently; and
- world documentation explains why replicas exist.

### 18.4 Add Courage as the first new effect prototype

Of all Homeland mechanics, Courage's group invocation fills the clearest
gameplay gap.

Proposed Luminari shape:

- artifact theme: leadership, courage, or communal endurance;
- called phrase: builder-approved and generated from the effect table;
- target: eligible same-room group members captured safely before effects
  begin;
- result: bounded morale plus vitality/temporary HP;
- recharge: balance-approved real-time interval;
- XP: one successful called-effect award, not one per group member; and
- refusal: no cooldown or XP when nobody is eligible.

Whether the final item retains the name Courage is a campaign/content
decision.

### 18.5 Add a small library of signature-proc shapes

Useful candidates, in recommended order:

1. Earthcrier-style controlled knockdown with a save and boss immunity.
2. Vengeance-style wounded-heal versus healthy-offense branch.
3. Tormblade-style alignment-conditioned dispel/protection critical.
4. Twilight-style bounded temporary combat surge with a stacking group.
5. Halberd-style weighted multi-outcome proc.
6. Xvim-style bounded flurry, only after eliminating the global counter.

Every new proc should use:

- the existing per-artifact internal cooldown;
- normal Luminari damage and affect helpers;
- source-tagged affects;
- explicit target legality, immunity, and save rules;
- one XP award per successful proc; and
- deterministic unit-test seams for chance selection.

### 18.6 Generalize invocation channels only when needed

Homeland demonstrates both spoken and whispered artifact words. Instead of
hard-wiring a second hook, extend called-effect data with an invocation
channel only if a selected artifact needs it:

```text
ART_INVOKE_SAY
ART_INVOKE_WHISPER
ART_INVOKE_COMMAND
```

Then route the relevant communication commands through one matcher. The
effect phrase, displayed help, and runtime channel must come from the same
table.

### 18.7 Decide how passive status grants are represented

Wyrmfang, Courage, Icedge, and Twilight rely heavily on permanent state
bits. Current artifact templates expose numeric stats and three resistance
buckets, while object prototypes can independently carry ordinary permanent
affects.

Choose one documented model:

1. keep status grants in object prototypes and teach `artifact info` to
   report them; or
2. add an artifact-template passive-power list, apply source-tagged affects,
   and unlock them by artifact level.

The second model better supports progression and a single source of truth,
but requires validation and careful removal. Do not split one power between
both sources.

## 19. Source defects and behaviors that must not be ported

1. **No duplicate prevention.** Artifact membership is only the switch at
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:889-905`,
   and an exhaustive search of
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/zon/`
   found no artifact-aware reset guard. Retain all four current Luminari
   zone-load guards.
2. **Owner means last recipient, not current owner.** The receive hook is at
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/handler.c:748-751`,
   with no corresponding drop or extraction hook. Retain Luminari's current
   lifecycle state.
3. **NPCs can become saved owners.** The same `obj_to_char()` hook has no
   `IS_NPC()` check. Never regress Luminari's player and account checks.
4. **Nested container transfers can bypass tracking.** The restore routes at
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/pfile.c:670-715`
   bypass the non-recursive receive hook. Retain recursive hooks and
   reconciliation.
5. **Three registry VNUMs have no prototype.** VNUMs 501, 513, and 599 are
   absent from every object source under
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/lib/world/obj/`.
   Validate every candidate before registration.
6. **Missing special-procedure assignments log at boot.** The assignment
   warning is at
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_assign.c:198-204`.
   Disable invalid content precisely and test it.
7. **The public command can reveal stale or private owner names.** Its raw
   output is at
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:994-1010`.
   Add discovery and privacy policy.
8. **The loader can duplicate entries and leaks its file handle.** The load
   loop is at
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:947-991`.
   Retain a clean rebuild and explicit cleanup.
9. **Persistence replacement is not atomic.** The remove-before-rename path
   is at
   `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:908-926`.
   Retain Luminari's temporary-file plus atomic-rename design.
10. **Malformed file handling is unsafe.** The unchecked parsing is at
    `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/act.informative.c:963-988`.
    Retain versioned, bounded parsing.
11. **Cooldowns reset, and labels drift from values.** Courage's label and
    timer are at
    `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:4844-4883`,
    and its timer storage is at
    `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/structs.h:1141-1155`.
    Keep effect data authoritative and decide Luminari's v2.3 persistence
    policy independently.
12. **Xvim uses a global extra-hit counter.** Dispatch and mutation appear at
    `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/combat/fight.c:2645-2652`
    and
    `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:5231-5251`.
    Store proc state per action or artifact.
13. **Xvim directly edits HP and position and contains an HP assignment
    bug.** The raw damage branch is at
    `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:5254-5283`,
    and the summon assignment is at
    `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:5319-5324`.
    Use normal damage and summon APIs.
14. **Courage's combat branch is a TODO that still returns success.** It is
    at
    `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/spec_procs.c:4890-4902`.
    Discard it.
15. **Twilight doubles already-derived combat totals.** The calculation is
    at
    `/home/aiwithapex/projects/Luminari-Source/EXAMPLE/HomelandMUD/src/jotunheim.c:492-507`.
    Replace it with bounded bonuses.

## 20. Relationship to `ARTIFACT_SYSTEM.md`

The system document remains authoritative for implemented behavior. It now
contains a clearly labeled research-candidate inventory; the remaining
details below belong there only after the related runtime work is approved
and implemented.

### 20.1 Roster documentation

If any Homeland candidate is approved, add it to the authoritative Artifact
Roster with:

- final Luminari VNUM define;
- final name;
- campaign availability;
- binding;
- class oath;
- base-per-level bonuses;
- passive powers;
- active ability;
- generic and signature procs;
- called effects;
- acquisition path;
- public-owner policy; and
- optional echo VNUM.

### 20.2 New design sections

Add sections only when implemented:

- Artifact Chronicle and Discovery;
- Acquisition and Release Policies;
- Artifact Echoes and Replicas;
- Passive Status Powers;
- Invocation Channels; and
- Proc Stacking Groups.

### 20.3 Recorded research-candidate inventory

The current research inventory now contains:

1. Vengeance;
2. Earthcrier;
3. Wyrmfang, the Spear of Dragons;
4. Courage;
5. Icedge, the Dagger of Cold;
6. Twilight, the Sword of Destruction;
7. missing VNUM 501, Xvim artifact/avenger mechanics only;
8. missing VNUM 513, halberd mechanics only; and
9. missing VNUM 599, Tormblade mechanics only.

Only the first six have enough source evidence for an identity-level content
proposal. The last three must remain unnamed research entries unless another
authoritative source is found.

## 21. Suggested implementation phases and acceptance criteria

### Phase A: content decisions

- approve, reject, or rename each of the six complete candidates;
- assign campaign availability;
- select original-only or original-plus-echo;
- define an acquisition path and lore owner for every approved candidate;
- decide whether artifact bearers are public; and
- record balance targets before coding.

### Phase B: chronicle and acquisition metadata

- add one authoritative metadata source;
- implement `artifact roster`;
- preserve current owner and account semantics;
- add first-claim/history state without weakening reset guards;
- migrate old artifact files safely;
- make visibility campaign- and discovery-aware; and
- cover privacy, stale-character, reset, destruction, and staff override.

### Phase C: one vertical-slice artifact

Courage is the preferred slice because it exercises:

- a new item;
- a group target;
- a called effect;
- recharge;
- public lore;
- an acquisition path;
- optional echo behavior; and
- chronicle discovery.

Acceptance tests should cover solo use, same-room group selection, absent
members, ineligible members, mutation during effect application, cooldown
refusal, reboot policy, one XP award, original/echo separation, and
single-instance recovery.

### Phase D: signature-proc candidates

Add at most one proc shape at a time, with deterministic regression tests and
a gameplay balance pass. Earthcrier's knockdown is the simplest next
candidate; Twilight's surge is the highest balance risk.

## 22. Final assessment

HomelandMUD is valuable as content archaeology, not as a replacement system.
Its safe contributions are:

- six complete artifact identities;
- three incomplete procedural concepts;
- the public artifact-chronicle idea;
- staff-timed release as a documented acquisition mode;
- named originals with repeatable weaker echoes;
- group and conditional proc designs; and
- world-dialogue acquisition clues.

LuminariMUD should retain its current registry, persistence, ownership,
binding, progression, nested-object handling, uniqueness guards, and tests.
Any adopted Homeland content should be rebuilt on those foundations and
promoted from the research-candidate inventory into the authoritative
Artifact Roster only after it becomes runtime behavior.
