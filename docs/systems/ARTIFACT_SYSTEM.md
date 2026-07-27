# Artifact System

## Overview

The artifact system manages eleven unique, levelable items. Normal zone
loading enforces one live instance per artifact. The system records
persistent ownership and binding, applies level-scaled bonuses, awards
artifact experience, and provides active, on-hit, and speech-invoked powers.

The system is compiled unconditionally. It becomes active at boot only for
artifact vnums whose object prototypes loaded successfully.

The implementation is native LuminariMUD code ported from the
RealmsOfLuminari design. The current code, rather than the upstream behavior,
is authoritative.

## File Map

| File | Responsibility |
| --- | --- |
| `src/world/spec_artifacts.h` | VNUMs, data model, tunables, and public API |
| `src/world/spec_artifacts.c` | Registry, persistence, gameplay, and commands |
| `unittests/CuTest/test_artifacts.c` | Production-linked regression tests |
| `scripts/provision_artifacts.sh` | World-file and help-index provisioning |
| `lib/world/world.artifact` | Generated ownership/progression state |

The main integration points are:

| File | Integration |
| --- | --- |
| `src/db.c` | Boot, shutdown, and zone-reset single-instance guards |
| `src/comm.c` | Dirty registry flush during the periodic character save |
| `src/handler.c` | Object acquisition, movement, equip, unequip, and extraction |
| `src/objsave.c` | Persistence-safe extraction during player object saves |
| `src/fight.c` | Resistance, combat XP, generic procs, and signature procs |
| `src/act.comm.c` | Called-effect phrase handling from `say` |
| `src/limits.c` | Class-oath burn damage during `point_update()` |
| `src/interpreter.c` | Player, ability, and staff command registration |
| `src/spells.h` | `SPELL_ARTIFACT_BONUS` affect identifier |

## Required World Data

Artifact code uses zone 1699 and the range 169900-169999. The VNUM constants
live in `src/world/spec_artifacts.h`; do not add them to the local
`src/vnums.h`.

The provisioning source is expected at `lib/world/artifacts/`:

| Source file | Required content |
| --- | --- |
| `1699.obj` | Object prototypes 169901 through 169911 |
| `1699.wld` | Vault room 169900 |
| `1699.zon` | Zone reset commands for the artifact objects |
| `1699.mob` | Oaken Defender mobile 169912 |
| `artifacts.hlp` | Player help entries |

`scripts/provision_artifacts.sh` copies missing files into the normal world
and help directories and adds their names to the corresponding indexes. It
does not overwrite existing deployed files, and repeated runs do not add
duplicate index entries. Both `scripts/setup.sh` and `scripts/deploy.sh` call
it.

### Current packaging limitation

`lib/world/artifacts/` is ignored by Git with the rest of the OLC-managed
world data. The package may exist on a development machine, but it is not
available in a fresh checkout. On a clean clone, the provisioner therefore
fails when it tries to copy its first missing source file. If provisioning is
skipped, `artifact_boot()` logs every missing artifact prototype; if none
load, the registry remains inactive.

This packaging issue must be resolved before a clean-clone deployment is
reliable. It is tracked in
`docs/project-management-zusuk/ongoing-projects/artifacts.md`.

The ownership file, `lib/world/world.artifact`, is also ignored, but
intentionally: it is generated runtime state and must not be distributed as
world content.

## Registry and Membership

`artifact_boot()` builds `art_index` from the compile-time
`artifact_templates[]` table. Each entry is accepted only when its object
prototype exists. The resulting array is sorted by VNUM and searched with
`artifact_search()`.

An object is an artifact if and only if its VNUM resolves in this registry.
There is no separate object flag. This keeps the membership test and the
artifact data in one source of truth.

The registry owns:

- owner character and account names;
- artifact level and cumulative experience;
- binding rule and bind timestamp;
- whether the live instance is in durable player or house storage;
- in-memory ability, proc, and called-effect cooldown stamps;
- template-derived bonuses, resistances, powers, and class oath.

## Persistence

`artifact_save()` rewrites the complete registry to
`lib/world/world.artifact.tmp` and atomically renames it over
`lib/world/world.artifact`.

The current v2.2 format is:

```text
# Artifact Ownership File v2.2
# Format: vnum owner account level exp bound_time instance_persisted
169901 noone noone 1 0 0 0
169905 Zusuk zusuk_acct 3 450 1700000000 1
```

The loader also accepts:

- ROL v1: `vnum owner timestamp`
- ROL v2.0: `vnum owner level exp binding_type bound_time`
- LuminariMUD v2.1: `vnum owner account level exp bound_time`

`noone`, `nobody`, `none`, and `no` all mean unowned.

Only ownership, account, level, experience, bind time, and durable-instance
state are persisted. Binding rules, bonuses, abilities, proc chances, class
oaths, and called-effect definitions come from the code-side templates. This
allows balance changes without migrating player state.

Cooldown state is not present in v2.2. Active abilities, generic procs, and
called effects all reset to ready whenever the server restarts. Persisting
these stamps requires a new save format.

Sub-threshold XP and other dirty state are flushed once per minute and again
during shutdown. Level-ups and ownership changes save immediately.

## Ownership and Single-Instance Rules

### Claiming and release

When a player acquires an unbound artifact, the registry records the
character and account and logs the claim to staff. Nested containers are
walked recursively, so putting an artifact inside a carried bag does not
bypass ownership tracking.

An unbound artifact returns to the unowned pool when it is dropped. A bound
artifact retains its owner when set down. Picking up a bound artifact never
rewrites its owner, although the other character may carry it; binding is
enforced when that character tries to wear or invoke it.

Player object saving uses an explicit persistence-extraction scope so logout
does not release ownership. Actual object destruction releases ownership and
binding. Locationless prototype clones, such as inspection copies, do not.

### Durable and recoverable instances

`instance_persisted` distinguishes an item stored in a player save or house
from one left in an ordinary room:

- a durable owned instance prevents a zone reset from loading another copy;
- any live instance already in `object_list` also prevents another copy;
- a bound artifact dropped in an ordinary room remains owned but is marked
  recoverable, allowing its zone reset to replace it after a reboot.

The four object-loading zone commands (`O`, `P`, `G`, and `E`) all call
`artifact_block_zone_load()`.

The staff interface calls an owned artifact "dropped" when no live instance
is currently on a character. This includes an instance in a room, in an
unheld container, or no longer in play.

## Binding

| Binding | When it binds | Who may use it afterward |
| --- | --- | --- |
| None | Never | Any holder |
| Bind on Pickup | First player pickup | The recorded character |
| Bind on Equip | First wear | The recorded character |
| Bind on Account | First wear | Any character on the recorded account |

`equip_char()` rejects a bound artifact before committing the wear and
returns it to inventory. Active abilities and called effects run the same
binding check. Characters at `LVL_IMMORT` or above bypass binding checks for
staff operations.

## Bonuses and Resistance

Every numeric bonus in `artifact_templates[]` is a base-per-artifact-level
value. Equipping a level 3 artifact applies three times its template values.

Supported bonuses are Strength, Intelligence, Wisdom, Dexterity,
Constitution, Charisma, hit roll, damage roll, Armor Class, hit points, PSP,
and movement. Armor Class is negated when applied because lower AC is better.

Bonuses use permanent `SPELL_ARTIFACT_BONUS` affects. Each affect stores
`registry_index + 1` in `affected_type.specific`, allowing unequip to remove
only the originating artifact's affects. Level-up refreshes those affects
immediately without requiring re-equip.

Physical, magical, and elemental resistance values are percentages and do
not scale with artifact level. When multiple equipped artifacts apply, only
the highest resistance for the incoming damage type is used.

| Bucket | Damage types |
| --- | --- |
| Physical | Slice, puncture, force, and bleeding |
| Elemental | Fire, cold, air, earth, acid, electric, water, light, and sound |
| Magical | Every other damage type |
| Never resisted | `DAM_RESERVED_DBC` |

Artifact resistance applies only to player victims.

## Class Oaths

Five artifacts require ten levels in a named class:

| Artifact | Required class |
| --- | --- |
| Trorxek | Druid |
| Amaukekel | Cleric |
| Fade | Rogue |
| Horn of Henekar | Rogue |
| Doombringer | Warrior |

The gate uses `CLASS_LEVEL()` so it works with LuminariMUD multiclass
characters. Wearing an artifact without the required depth is allowed, but
once per `point_update()` it deals `5d4` fire damage. Only one burn is
applied per update even when several equipped artifacts reject the wearer.

A rejected wielder cannot invoke that artifact's called speech effects and
cannot see their phrases in `artifact info`. The current active-ability path
does not apply the class-oath check; it applies binding, cooldown, and PSP
checks only. NPCs and characters at `LVL_IMMORT` or above are exempt from the
oath.

## Progression

Artifact XP is cumulative and is not spent at level-up:

| Transition | Cumulative XP required |
| --- | --- |
| 1 -> 2 | 100 |
| 2 -> 3 | 300 |
| 3 -> 4 | 600 |
| 4 -> 5 | 1000 |

The maximum artifact level is 5. One call to
`artifact_check_levelup()` advances at most one level. A level-up notifies
the holder, logs to staff, refreshes equipped bonuses, and saves the
registry.

### XP awards

| Event | XP | Recipient |
| --- | --- | --- |
| First equip while XP is zero | 10 | That artifact |
| Damaging hit on an NPC | 1 | One random equipped artifact |
| Critical damaging hit on an NPC | 3 | One random equipped artifact |
| Kill an NPC | `10 + victim_level / 5` | One random equipped artifact |
| Boss-tier hit | Base hit XP x2 | One random equipped artifact |
| Boss-tier kill | Base kill XP x3 | One random equipped artifact |
| Generic soul/heal/fear/doom/ultimate proc | 2/1/3/4/10 | Proc artifact |
| `soulstrike` | 15 | Kelrarin's Hammer |
| `divineward` | 20 | Amaukekel |
| `doomblast` | 10 per target | Doombringer |
| Successful called effect | 25 | Calling artifact |

A boss-tier target is an NPC at least three levels above the attacker.
Generic hit and kill XP is paid once, not once per equipped artifact. Five
percent of XP grants also print a progress message.

## Artifact Roster

VNUM 169900 is the Vault of Ages room, and 169912 is the Oaken Defender
mobile. Neither is an artifact registry entry.

| VNUM | Artifact | Binding | Oath | Active ability | Generic proc | Called effects |
| --- | --- | --- | --- | --- | --- | --- |
| 169901 | Trorxek, the Staff of Ancient Oaks | Equip | Druid | - | 12% | 4 |
| 169902 | Amaukekel, the Rod of Light | Equip | Cleric | `divineward` | - | 3 |
| 169903 | Fade, the Shadowblade | Equip | Rogue | - | 16% | 4 |
| 169904 | The Horn of Henekar | Equip | Rogue | - | - | 4 |
| 169905 | Doombringer | Pickup | Warrior | `doomblast` | 20% | 3 |
| 169906 | Kelrarin's Hammer | Equip | - | `soulstrike` | 15% | - |
| 169907 | Kelrom, the Axe of Pahluruk | Equip | - | - | 14% | - |
| 169908 | Gesen, the Returning Axe | None | - | - | 18% | - |
| 169909 | Tiamat's Stinger | Account | - | - | 18% | - |
| 169910 | Avernus, the Black Blade | Equip | - | - | 15% | - |
| 169911 | The Aegis of Ages | Equip | - | - | - | - |

`artifact_templates[]` is authoritative for each artifact's exact base
bonuses, resistance percentages, ability costs, cooldowns, and proc chance.

## Researched Artifact Candidates

The HomelandMUD source study identified the following candidates for future
content work. They are not current registry entries, have no allocated
LuminariMUD VNUMs, and must not be treated as implemented or deployable.

| Candidate | Source completeness | Potential role |
| --- | --- | --- |
| Vengeance | Complete prototype and procedure | Holy weapon with wounded-heal or healthy-offense proc |
| Earthcrier | Complete prototype and procedure | Two-handed control weapon with a knockdown proc |
| Wyrmfang, the Spear of Dragons | Complete prototype | Dragon-themed perception and pursuit spear |
| Courage | Complete prototype and procedure | Group-support weapon with a spoken invocation |
| Icedge, the Dagger of Cold | Complete prototype and quest lore | Cold-defense dagger with exploration-led acquisition |
| Twilight, the Sword of Destruction | Complete prototype and procedure | Destruction weapon with a bounded combat-surge design |
| Homeland VNUM 501 | Procedure only; prototype missing | Dark-avenger flurry and summoned-companion concepts |
| Homeland VNUM 513 | Procedure only; prototype missing | Weighted multi-outcome halberd control proc |
| Homeland VNUM 599 | Procedure only; prototype missing | Alignment-conditioned protection and dispel proc |

The complete audit, original VNUMs, counterpart pattern, porting cautions,
recommendations, and absolute HomelandMUD source paths are maintained in
`docs/project-management-zusuk/ongoing-projects/artifacts.md`.

## Player Commands

| Command | Purpose |
| --- | --- |
| `artifact` or `artifact help` | Explain artifacts, binding, oaths, and progression |
| `artifact list` | List carried and equipped artifacts |
| `artifact info <item>` | Show ownership, bonuses, powers, oath, and recharge state |
| `artifact progress` | Show level, XP, and progress bars |
| `artifact abilities` | Show equipped active abilities, PSP costs, and cooldowns |

`artifact info` generates its called-effect text from the same table used by
the dispatcher, so the displayed phrases and recharge periods cannot drift
from runtime behavior.

## Active Abilities

The ability command must match an equipped artifact's `ability_name`.
Binding, PSP cost, and per-artifact cooldown are checked before the effect
runs.

| Command | Artifact | Cost | Cooldown | Effect |
| --- | --- | --- | --- | --- |
| `soulstrike [target]` | Kelrarin's Hammer | 50 PSP | 300 sec | Negative damage; opponent default |
| `divineward` | Amaukekel | 100 PSP | 600 sec | Sanctuary for `5 + artifact_level` rounds |
| `doomblast` | Doombringer | 75 PSP | 180 sec | Room attack; at most five targets |

`soulstrike` deals
`dice(5 + artifact_level, 20) + artifact_level * 20 + character_level * 2`.
Each `doomblast` target takes
`dice(3 + artifact_level, 15) + character_level`.

Hostile target selection uses `aoeOK()`. Doom Blast counts valid targets
before spending PSP or starting its cooldown.

## Generic Weapon Procs

An artifact weapon with a nonzero proc chance rolls once per successful hit,
subject to a 30-second internal cooldown for that artifact. If the chance
succeeds, the proc kind is `rand_number(1, artifact_level)`.

| Roll | Effect |
| --- | --- |
| 1 | `dice(level, 6)` negative soul damage |
| 2 | `dice(level, 4)` self-heal when wounded |
| 3 | Fear for `1 + level / 2` rounds |
| 4 | `dice(level, 8)` negative doom damage |
| 5 | At level 5, a further 5% chance to execute an NPC no higher than the wielder |

Signature procedures run before this generic system. They have their own
odds, do not consume the generic internal cooldown, and may occur on the same
hit as a generic proc if the victim survives.

## Signature Weapon Procedures

| Artifact | Behavior |
| --- | --- |
| Trorxek | Every eligible critical hit blinds for `1 + level / 2` rounds |
| Kelrarin | 1-in-29 returning throw with level-scaled force damage and full lifesteal |
| Kelrarin | Above 990 alignment and at least 90% HP, 1-in-33 holy blast plus NPC execute check |
| Kelrom | Kills its wielder for striking an animal; otherwise applies group healback |
| Gesen | 1-in-31 returning throw that invokes `SPELL_HARM` |
| Avernus | Below 100 HP, a `30 + 2 * level` percent chance to restore the wielder to full HP |

Kelrarin's throw ceiling grows from 50 at level 1 to 250 at level 5.
Kelrom's healback grows from 10% to 50% of triggering damage. Signature
effects scale their spell or effect level through
`artifact_effect_level()`.

## Called Effects

Five artifacts listen to ordinary `say` speech while carried or equipped.
Speech is lowercased, repeated whitespace is collapsed, and a trailing
period, exclamation point, question mark, or comma is removed before
matching. Saying the phrase is never suppressed from the room.

Each artifact has up to four independent effect slots. A successful effect
starts only that slot's recharge and awards 25 XP. Failed target checks do
not spend the recharge.

| Artifact | Phrase | Recharge | Effect |
| --- | --- | --- | --- |
| Trorxek | `come oaken defender` | 1 week | Summon the level-scaled Oaken Defender follower |
| Trorxek | `carpet of death` | 1 day | Invoke `SPELL_CREEPING_DOOM` |
| Trorxek | `forest path home` | 1 hour | Invoke `SPELL_WORD_OF_RECALL` |
| Trorxek | `moonlit path to <target>` | 1 day | Travel to a visible named player |
| Amaukekel | `sunlit path to paradise` | 1 week | Recall wielder; group defect tracked |
| Amaukekel | `give life to <corpse>` | 1 day | Invoke `SPELL_RESURRECT` on a room corpse |
| Amaukekel | `wrath of light <target>` | 1 hour | Invoke `SPELL_DISPEL_EVIL` |
| Fade | `eyes of darkness <target>` | 1 day | Invoke `SPELL_BLINDNESS` |
| Fade | `darken the world` | 1 hour | Invoke `SPELL_DARKNESS` |
| Fade | `devour the soul` | 1 week | Invoke `SPELL_ENFEEBLEMENT` on the current opponent |
| Fade | `shadowy path to <target>` | 1 hour | Travel to a visible named player |
| Henekar | `you see darkness <target>` | 6 hours | Invoke `SPELL_BLINDNESS` |
| Henekar | `peace to you` | 12 hours | Stop every fight in the room |
| Henekar | `join my quest` | 12 hours | Charm eligible NPCs with at most 2000 max HP |
| Henekar | `sonic path to <target>` | 1 hour | Travel to a visible named player |
| Doombringer | `bring annhilation forth` | 1 week | Damage every valid hostile target in the room |
| Doombringer | `feel my power <target>` | 1 day | Level-scaled electrical damage to one target |
| Doombringer | `enrage me doombringer` | 1 day | Ten-round Str/hit/damage morale bonuses |

The misspelling `annhilation` is part of the live invocation phrase and must
be preserved unless both data and player-facing documentation are migrated
together.

Travel effects honor visibility, `AFF_NOTELEPORT`, `ROOM_NOTELEPORT`,
`valid_mortal_tele_dest()`, and elemental, ethereal, and astral plane
restrictions. Hostile room targets and Doombringer's room attack use
`aoeOK()`.

## Staff Operations

`testartifact` requires `LVL_STAFF`.

| Command | Purpose |
| --- | --- |
| `testartifact status` | Count owned, dropped, and unowned entries; show memory and data path |
| `testartifact verify` | Check live duplicates, levels, owners, and prototypes |
| `testartifact save` | Write v2.2 state immediately |
| `testartifact reload` | Rebuild the registry and reassociate live holders and bonuses |
| `testartifact spawn <vnum>` | Create an artifact in the staff member's room if none is live |
| `testartifact list` | Show every artifact's owner, state, binding, and location |
| `testartifact reset <vnum>` | Clear ownership and binding for one artifact |

`reset` changes durable gameplay state and should be used only when an item
must re-enter circulation, such as after deleting its owner.

`spawn` checks only for a live object. It does not reject an artifact recorded
as durably owned while its player is offline, and room placement can then
mark that registry entry recoverable. Staff must inspect `list` before using
`spawn` until this command is hardened.

The eleven objects currently reset into the private Vault of Ages room
169900. That is a staff staging area, not a player-facing distribution
mechanism. Placement in live content remains a builder decision.

## Development and Extension

### Rebalancing an artifact

Edit its entry in `artifact_templates[]`. Template values are applied at boot
and are not stored in v2.2, so a restart applies the new balance without a
data migration.

### Adding an artifact

1. Allocate a VNUM in the reserved range and add a named constant to
   `src/world/spec_artifacts.h`.
2. Create and deploy its object prototype.
3. Add one `artifact_templates[]` entry.
4. Add any speech powers to `artifact_effects[]`.
5. Add production-linked tests in `unittests/CuTest/test_artifacts.c`.
6. Verify clean boot, single-instance reset behavior, save round trips, and
   player lifecycle behavior.

There is no artifact OLC. Membership and behavior remain compile-time data.

Each `artifact_effects[]` slot must be unique within its artifact and between
zero and `ARTIFACT_MAX_EFFECTS - 1`. Runtime rejects an out-of-range slot,
but duplicate slots currently share a recharge stamp silently. Boot-time
table validation is still planned.

### Changing persisted fields

Increment the ownership-file version, keep the older loaders, add explicit
format detection, and add round-trip tests. Do not add a second writer or
perform fixed-width in-place record updates.

## Testing

Run the production-linked suite from the repository root:

```sh
make test
make install
```

`unittests/CuTest/test_artifacts.c` currently contains 45 artifact test
functions. Coverage includes registry search, ownership sentinels, XP and
level boundaries, binding names, v1, v2.0, and current-format persistence,
dirty saves, extraction scopes, destruction, recoverable drops,
single-instance guards, reload reassociation, affect-message suppression,
NULL safety, recharge arithmetic, phrase refusal paths, table shape, class
checks, dropped-state reporting, memory accounting, random single-recipient
combat XP, and critical-hit XP.

The test that reads `lib/world/artifacts/` is a deployment-package check, not
a hermetic unit test. It passes only on machines that retain the ignored
package.

World-dependent behavior is not automated end to end. A booted-world player
harness is still needed for equip and binding enforcement, resistance,
generic and signature procs, active abilities, called effects, burn damage,
and both command handlers.

## Design Decisions and Current Limitations

- Registry membership replaces the upstream artifact object flag.
- PSP replaces the upstream mana resource.
- Account binding compares the real account name, not a character-name
  approximation.
- One whole-file writer replaces conflicting legacy save paths.
- Boss XP uses a three-level margin because LuminariMUD has no `ACT_BOSS`
  flag.
- Artifact behavior uses explicit core hooks and a speech dispatcher rather
  than `SPECIAL()` procedures.
- Active-ability, generic-proc, and called-effect cooldown state is lost at
  reboot.
- The registry and effect table require a rebuild and have no OLC editor.
- The ignored world-data package prevents reliable clean-clone deployment.
- Effect-slot uniqueness is not validated at boot.
- World-driven behavior lacks automated integration coverage.
- Group recall moves the caller before selecting same-room group members, so
  `sunlit path to paradise` normally recalls only the caller.
- `testartifact spawn` does not honor durable offline ownership, and
  `testartifact reload` does not flush dirty state before rebuilding.

Actionable follow-up work is maintained in
`docs/project-management-zusuk/ongoing-projects/artifacts.md`.

## Related Documentation

- `docs/systems/COMBAT_SYSTEM.md`
- `docs/systems/SAVE_SYSTEMS_BREAKDOWN.md`
- `docs/systems/COMMAND_SYSTEM_AND_INTERPRETER.md`
- `docs/systems/OLC_ONLINE_CREATION_SYSTEM.md`
- `docs/guides/TESTING_GUIDE.md`
- `docs/CHANGELOG.md`
