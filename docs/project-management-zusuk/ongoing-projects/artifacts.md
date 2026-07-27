# Artifact System - Remaining Work

Completed behavior is documented in
[`docs/systems/ARTIFACT_SYSTEM.md`](../../systems/ARTIFACT_SYSTEM.md), and
completed implementation work is recorded in
[`docs/CHANGELOG.md`](../../CHANGELOG.md).

Only unfinished work belongs in this file.

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

## 5. Decide whether cooldowns must survive reboot

Active-ability, generic-proc, and called-effect timestamps currently live
only in memory. Restarting the server makes every power ready. This matches
the upstream called-effect event-queue behavior but creates an exploitable
reset on servers that reboot often.

If persistence is required:

- introduce a v2.3 ownership format;
- store ability, proc, and effect-slot last-use stamps as appropriate;
- preserve v1, v2.0, v2.1, and v2.2 loading;
- define behavior for removed, reordered, and newly added effect slots;
- add migration, round-trip, expired-timer, and clock-skew tests.

## 6. Validate the effect table at boot

`artifact_effects[]` assigns recharge slots by hand. Runtime rejects an
out-of-range slot, but duplicate slots on one artifact silently share a
recharge timer. The current shape test also duplicates expected phrases
instead of inspecting a public validation result.

Add boot-time validation for:

- VNUMs that resolve to registry artifacts;
- slots within `0 .. ARTIFACT_MAX_EFFECTS - 1`;
- unique slots per artifact;
- nonempty, pre-normalized phrases;
- valid target and effect identifiers;
- target requirements consistent with the phrase and dispatcher.

The system should log a precise `SYSERR` and disable only the invalid effect,
not the entire artifact registry.

## 7. Fix Amaukekel's group recall ordering

`artifact_dimension_shift()` recalls the caller before it iterates the group.
The later same-room check therefore compares each member's original room with
the caller's destination room, so ordinary nearby group members are skipped.

Capture the origin room before moving anyone, select eligible group members
from that origin, and then recall the caller and selected members safely even
if movement triggers mutate the group or room lists.

Acceptance criteria:

- the caller and every eligible grouped character in the origin room move;
- absent, ungrouped, and differently located characters do not move;
- iteration remains safe when recall triggers move or extract a participant;
- a booted-world regression test covers solo and grouped use.

## 8. Harden staff spawn and reload

`testartifact spawn <vnum>` rejects an existing live object but does not
reject an artifact recorded as durably owned while its player is offline.
Spawning that VNUM into an ordinary room can also change
`instance_persisted` to false, weakening the next zone-reset guard.

`testartifact reload` rebuilds the registry without first flushing dirty
sub-threshold XP or other deferred state.

Acceptance criteria:

- `spawn` rejects a durably owned artifact unless an explicit,
  separately-audited recovery operation is used;
- a rejected spawn does not mutate ownership or persistence flags;
- `reload` saves dirty state first or requires an explicit discard mode;
- staff output explains every refusal and destructive override;
- regression tests cover offline ownership and dirty reload state.
