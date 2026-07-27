# Artifact System

## Status: LIVE

Implemented, hooked into the core, built clean, boot-verified, unit-tested.
The system is compiled in unconditionally - there is no feature guard.

Both layers of the ROL system are now ported: the systemic layer (ownership,
binding, progression, persistence, single-instance enforcement) and the
content layer (the per-artifact special procedures, speech invocation,
per-effect recharge timers, class oaths and the burn penalty, and the special
identify text). Section 3 records how each piece was ported and where it
departs from the upstream.

This document describes **the system as it exists in this repository today**.
Where it refers to RealmsOfLuminari (ROL), the upstream it was ported from,
those sections are labelled as upstream reference and do not describe our
behavior.

---

# 1. Orientation

## Source files

| File | Role |
| --- | --- |
| `src/world/spec_artifacts.h` | vnum constants, data model, tunables, API |
| `src/world/spec_artifacts.c` | entire implementation (~1900 lines) |
| `unittests/CuTest/test_artifacts.c` | production-linked regression tests |

## Core files carrying hooks

| File | Hooks |
| --- | --- |
| `src/db.c` | boot, shutdown, 4 zone-reset guards |
| `src/handler.c` | 6 object-lifecycle hooks |
| `src/fight.c` | resistance, hit XP, kill XP, weapon procs |
| `src/interpreter.c` | 5 command registrations |
| `src/act.comm.c` | speech invocation of called effects |
| `src/limits.c` | class-restriction burn tick |
| `src/spells.h` | `SPELL_ARTIFACT_BONUS` = 1610 |

## Data

| Path | Contents |
| --- | --- |
| `lib/world/world.artifact` | runtime ownership state, gitignored |
| `lib/world/artifacts/1699.*` | deployment package for zone 1699, including the Oaken Defender mob (169912) |
| `lib/world/artifacts/artifacts.hlp` | artifact help entries |
| `scripts/provision_artifacts.sh` | idempotent world/index/help provisioning |

**`lib/world/artifacts/` is gitignored** (`.gitignore:287`), alongside every
other OLC-edited world file. An earlier revision of this document called the
package "tracked"; it never was. The consequence is that the whole deployment
package - the eleven object prototypes, the vault room, the zone file, the
Oaken Defender mob, and the help entries - exists only on whatever machine
authored it and will not survive a fresh clone. `provision_artifacts.sh` can
only copy files that are already present, so on a clean checkout it copies
nothing and `artifact_boot()` logs eleven missing-prototype errors and shuts
the system off.

Two things follow, both decisions for a maintainer rather than code changes:

1. Either the package needs a home inside version control (a `data/` path
   outside the ignore rule, or a targeted `!lib/world/artifacts/` negation),
   or it needs to be reproducible from something that is.
2. `Test_artifact_world_package_contains_all_deployable_records` reads that
   directory directly, so it passes only where the package happens to exist.
   It is a deployment check masquerading as a unit test.

## Commands

- Player: `artifact [list | info <item> | progress | abilities | help]`
- Abilities: `soulstrike [target]`, `divineward`, `doomblast`
- Called effects: spoken aloud with `say` - see section 3, gap 1
- Staff (`LVL_STAFF`): `testartifact <status|verify|save|reload|spawn|list|reset>`

## VNUM allocation

Zone **1699**, range **169900-169999**. Verified free at allocation time: no
`.zon`, `.wld`, `.obj`, or `.mob` file used the zone, and no record anywhere
matched `#1699xx`.

`src/vnums.h` is gitignored local configuration and must not be edited, so
these constants live in `src/world/spec_artifacts.h`.

---

# 2. How the system works

## 2.1 Membership

An object is an artifact if and only if its vnum resolves in the `art_index`
registry (`artifact_is_artifact()`). There is no per-object flag.

The registry is built at boot from the `artifact_templates[]` table in
`spec_artifacts.c`, not from the save file. An artifact whose object
prototype fails to load is logged and skipped.

## 2.2 Registry and lookup

`art_index` is a flat array sorted by vnum and searched with binary search
(`artifact_search()`). `total_artifacts` is the count. The sort at boot is
load-bearing.

## 2.3 Persistence

File: `lib/world/world.artifact`, written whole via a `.tmp` file and an
atomic `rename()`. One writer, one format.

```
# Artifact Ownership File v2.2
# Format: vnum owner account level exp bound_time instance_persisted
169901 noone noone 1 0 0 0
169905 Zusuk zusuk_acct 3 450 1700000000 1
```

Owner sentinels meaning unowned: `noone`, `nobody`, `none`, `no`.

Stat blocks, abilities, and proc chances are **deliberately not persisted** -
they live in `artifact_templates[]` so they can be rebalanced by editing code
without migrating player data. Only ownership, account, level, XP, and
`bound_time`, and whether the live instance is in durable player/house storage
are saved.

## 2.4 Single-instance enforcement

`artifact_block_zone_load()` is consulted by all four object-loading zone
reset commands (`O`, `P`, `G`, `E`). It returns TRUE - meaning do not load -
when a durable saved instance is owned, or when any instance is already in
play. A bound artifact dropped in a normal room retains its owner but is
marked recoverable, allowing the zone to replace the vanished room instance
after reboot.

## 2.5 Ownership

| Function | Trigger |
| --- | --- |
| `artifact_to_char()` | a PC acquires it; claims ownership, logs to staff |
| `artifact_from_char()` | released back to the pool |
| `artifact_tag_nested()` | repoint holder recursively, ownership unchanged |
| `artifact_get_nested()` | transfer every artifact inside a container |
| `artifact_drop_nested()` | release every artifact inside a container |

A **bound** artifact never changes owner on pickup and is not released when
set down. Only unbound artifacts return to the pool.

Ownership survives logout because objsave wraps post-serialization extraction
in an explicit persistence scope. Actual destruction clears ownership even
while the object is carried, and locationless prototype clones are ignored.

## 2.6 Binding

| Type | Binds when |
| --- | --- |
| `ARTIFACT_BIND_NONE` | never |
| `ARTIFACT_BIND_ON_PICKUP` | the moment a PC takes it (`artifact_to_char()`) |
| `ARTIFACT_BIND_ON_EQUIP` | first wear |
| `ARTIFACT_BIND_ON_ACCOUNT` | first wear; checked against `GET_ACCOUNT_NAME()` |

Enforced in `equip_char()`, which refuses the wear and returns the object to
inventory - the same shape as its existing `invalid_class()` refusal.
`artifact_on_equip()` re-checks silently as a backstop for any other caller.

Staff at `LVL_IMMORT` or above bypass binding checks.

## 2.7 Stat bonuses

Applied as `SPELL_ARTIFACT_BONUS` affects with `duration = -1`. Every bonus
is multiplied by the artifact's current level.

Locations: `APPLY_STR/INT/WIS/DEX/CON/CHA`, `APPLY_HITROLL`, `APPLY_DAMROLL`,
`APPLY_AC` (negated - lower AC is better), `APPLY_HIT`, `APPLY_PSP`,
`APPLY_MOVE`.

Each affect carries `af.specific = registry_index + 1`, so
`artifact_remove_bonuses()` strips exactly one artifact's affects. Safe
because `specific` is only otherwise consulted for `APPLY_SKILL` affects in
`affect_join()`.

## 2.8 Progression

| Level | XP to next |
| --- | --- |
| 1 -> 2 | 100 |
| 2 -> 3 | 300 |
| 3 -> 4 | 600 |
| 4 -> 5 | 1000 |

`ARTIFACT_MAX_LEVEL` = 5. XP is cumulative and never spent. One level per
`artifact_check_levelup()` call. A level-up notifies the owner, logs to
staff, **reapplies bonuses in place**, and forces a save.

XP actually awarded:

| Event | XP | Path |
| --- | --- | --- |
| first equip (only when `experience == 0`) | 10 | targeted |
| successful damaging hit on an NPC | 1, or 3 on a critical | one random equipped artifact |
| kill an NPC | 10 + victim level / 5 | one random equipped artifact |
| either of the above against a boss-tier NPC | x2 hit, x3 kill | one random equipped artifact |
| weapon proc: soul / heal / fear / doom / ultimate | 2 / 1 / 3 / 4 / 10 | targeted |
| ability soulstrike / divineward / doomblast | 15 / 20 / 10 per target | targeted |
| any called effect | 25 | targeted |

"Targeted" means only the artifact that earned it. Generic combat XP pays one
artifact chosen at random from those equipped - see gap 6 in section 3.
"Boss-tier" means an NPC at least `ARTIFACT_BOSS_LEVEL_MARGIN` levels above
its attacker - see gap 4.

5% of grants also print a progress line.

## 2.9 Damage resistance

Three percentages: physical, magical, elemental. Checked across all worn
slots; the single highest applicable value wins - **they do not stack**.
Applied in `damage()` immediately after the deflective-screen reduction, and
only for PC victims.

Damage-type buckets:

- physical: `DAM_SLICE`, `DAM_PUNCTURE`, `DAM_FORCE`, `DAM_BLEEDING`
- elemental: `DAM_FIRE`, `DAM_COLD`, `DAM_AIR`, `DAM_EARTH`, `DAM_ACID`,
  `DAM_ELECTRIC`, `DAM_WATER`, `DAM_LIGHT`, `DAM_SOUND`
- magical: everything else
- `DAM_RESERVED_DBC`: never resisted

## 2.10 Weapon procs

Gate: `proc_chance` percent per successful hit, plus a 30-second internal
cooldown per artifact (`ARTIFACT_PROC_ICD`) so a fast weapon cannot chain
procs.

Kind: `rand_number(1, level)`, so a higher-level artifact can roll anything
up to its own level.

| Roll | Effect |
| --- | --- |
| 1 | soul damage, `dice(level, 6)`, `DAM_NEGATIVE` |
| 2 | self heal, `dice(level, 4)`, only if wounded |
| 3 | `AFF_FEAR`, duration `1 + level/2` |
| 4 | doom damage, `dice(level, 8)` - requires level 4+ |
| 5 | ultimate: level 5, NPC victim only, victim level <= attacker level, further 5% roll, then executes for `GET_HIT(victim) + 100` |

## 2.11 Active abilities

Each ability is its own command; the typed command name selects which
equipped artifact answers (`CMD_NAME` matched against `ability_name`).

Shared gate: binding check, cooldown against `ability_cooldown`, and psp
against `ability_cost`.

| Ability | Artifact | Cost | Cooldown | Effect |
| --- | --- | --- | --- | --- |
| `soulstrike` | Kelrarin's Hammer | 50 psp | 300s | single target, `dice(5+level, 20) + level*20 + charlevel*2` |
| `divineward` | Amaukekel | 100 psp | 600s | self sanctuary, `5 + level` rounds |
| `doomblast` | Doombringer | 75 psp | 180s | room AoE, max 5 targets, `dice(3+level, 15) + charlevel` each |

Doom Blast counts valid targets before spending anything, and refuses if
there are none. All targeting respects `aoeOK()`.

## 2.12 Hook sites

```
db.c          boot_db()            -> artifact_boot()   (before zone resets)
              destroy_db()         -> artifact_shutdown()
              reset_zone() O/P/G/E -> artifact_block_zone_load()
handler.c     obj_to_char()        -> artifact_obj_to_char()
              obj_from_char()      -> artifact_obj_from_char()
              obj_to_room()        -> artifact_obj_to_room()
              equip_char()         -> artifact_can_use() guard, artifact_on_equip()
              unequip_char()       -> artifact_on_unequip()
              extract_obj()        -> artifact_on_extract()
fight.c       damage()             -> artifact_damage_resist()
              handle_successful_attack() -> artifact_combat_hit(),
                                            artifact_weapon_proc()
                                            (both receive is_critical)
              dam_killed_vict()    -> artifact_combat_kill()
act.comm.c    do_say()             -> artifact_speech_trigger()
limits.c      point_update()       -> artifact_burn_tick()
interpreter.c cmd_info[]           -> artifact, soulstrike, divineward,
                                      doomblast, testartifact
```

## 2.13 Artifact roster

| VNUM | Name | Item type | Binding | Class | Ability | Proc | Called | Signature |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 169900 | the Vault of Ages | room | - | - | - | - | - | - |
| 169901 | Trorxek, the Staff of Ancient Oaks | quarterstaff | on equip | Druid | - | 12% | 4 | blinding crit |
| 169902 | Amaukekel, the Rod of Light | light mace | on equip | Cleric | divineward | - | 3 | - |
| 169903 | Fade, the Shadowblade | short sword | on equip | Rogue | - | 16% | 4 | - |
| 169904 | The Horn of Henekar | held | on equip | Rogue | - | - | 4 | - |
| 169905 | Doombringer | great sword | on pickup | Warrior | doomblast | 20% | 3 | - |
| 169906 | Kelrarin's Hammer | warhammer | on equip | - | soulstrike | 15% | - | thrown hammer, mega blast |
| 169907 | Kelrom, the Axe of Pahluruk | battle axe | on equip | - | - | 14% | - | animal taboo, group healback |
| 169908 | Gesen, the Returning Axe | hand axe | none | - | - | 18% | - | thrown axe, harm |
| 169909 | Tiamat's Stinger | rapier | on account | - | - | 18% | - | - |
| 169910 | Avernus, the Black Blade | long sword | on equip | - | - | 15% | - | emergency full heal |
| 169911 | The Aegis of Ages | body armor | on equip | - | - | - | - | - |
| 169912 | the Oaken Defender | mob | - | - | - | - | - | summoned by Trorxek |

"Class" is the oath a wielder must have `ART_CLASS_GATE` levels in or be
burned every tick. "Called" is the number of speech-invoked effects.

---

# 3. The ROL content layer

Every gap this section used to catalogue has now been closed. What follows
records how each was closed and where it departs from the upstream, since a
few could not be ported literally.

The one deliberate exception is `tagBogusArtifact()`, which stays unported -
see deviation 3 in section 4.

## Gap 1: the per-artifact special procedures - PORTED

ROL gave each artifact a hand-written spec proc with its own personality.
All of it now exists, rebuilt on LuminariMUD rather than transcribed.

### Invocation by speech

`artifact_speech_trigger()` is called from `do_say()` in `act.comm.c`, on the
raw pre-formatting copy of what was said. Speech is normalized - lowercased,
whitespace collapsed, trailing punctuation stripped - so `"Carpet of
death!"`, `"carpet of death"`, and the period `do_say()` appends to
unpunctuated speech all match one table entry.

The artifact must be worn or carried. Saying the words never suppresses the
speech itself; that is the point of saying them.

### Per-effect recharge timers

Each artifact carries `effect_used[ARTIFACT_MAX_EFFECTS]`, one stamp per
effect slot, checked by `artifact_recharge_remaining()`. Intervals are
`ARTIFACT_RECHARGE_HOUR` / `_6HOUR` / `_12HOUR` / `_DAY` / `_WEEK`.

These stamps live in memory only and reset on reboot. ROL's equivalents were
event-queue entries that did not survive a reboot either, so this matches
upstream behavior rather than falling short of it.

### The effects

Eighteen called effects across five artifacts, in
`artifact_effects[]`, dispatched by `artifact_do_effect()`:

| Artifact | Phrase | Recharge | Effect |
| --- | --- | --- | --- |
| Trorxek | `come oaken defender` | week | summons the Oaken Defender |
| Trorxek | `carpet of death` | day | `SPELL_CREEPING_DOOM` |
| Trorxek | `forest path home` | hour | `SPELL_WORD_OF_RECALL` |
| Trorxek | `moonlit path to <t>` | day | travel to a named player |
| Amaukekel | `sunlit path to paradise` | week | recalls the caster and the group |
| Amaukekel | `give life to <corpse>` | day | `SPELL_RESURRECT` on a corpse in the room |
| Amaukekel | `wrath of light <t>` | hour | `SPELL_DISPEL_EVIL` |
| Fade | `eyes of darkness <t>` | day | `SPELL_BLINDNESS` |
| Fade | `darken the world` | hour | `SPELL_DARKNESS` |
| Fade | `devour the soul` | week | `SPELL_ENFEEBLEMENT` on your opponent |
| Fade | `shadowy path to <t>` | hour | travel to a named player |
| Henekar | `you see darkness <t>` | 6h | `SPELL_BLINDNESS` |
| Henekar | `peace to you` | 12h | stops every fight in the room |
| Henekar | `join my quest` | 12h | charms NPCs under 2000 max hp |
| Henekar | `sonic path to <t>` | hour | travel to a named player |
| Doombringer | `bring annhilation forth` | week | room-wide `dice(10+level*4, 12) + charlevel*3` |
| Doombringer | `feel my power <t>` | day | black lightning, `dice(8+level*3, 14) + charlevel*2` |
| Doombringer | `enrage me doombringer` | day | +hitroll / +damroll / +Str for 10 rounds |

A successful call awards `ARTIFACT_XP_CALLED_EFFECT` (25) to that artifact
alone.

Substitutions made where LuminariMUD has no upstream equivalent:

- **Moonwell / dimension shift / shadow path / sonic path** all collapse onto
  two primitives: `artifact_travel_to()` (walk to a named player) and
  `artifact_dimension_shift()` (recall the caster and everyone grouped with
  them in the room). `artifact_travel_to()` observes the same guards every
  other travel effect does - `valid_mortal_tele_dest()`, `ROOM_NOTELEPORT`,
  `AFF_NOTELEPORT`, the outer-plane restrictions - and fires the greet
  triggers on arrival.
- **The treant.** ROL's `OAKEN_DEFENDER_VNUM` pointed at its own world. Ours
  ships as mob **169912** inside the artifact zone package
  (`lib/world/artifacts/1699.mob`), so the effect never depends on which
  campaign is compiled in. Its level scales with the artifact.
- **Weaken** is `SPELL_ENFEEBLEMENT`; **pacify** is implemented directly
  (`stop_fighting()` on everyone in the room) since no spell matches.
- **Charm** is implemented directly rather than through `SPELL_CHARM`, so it
  can enforce ROL's 2000-hp cap. It respects `MOB_NOCHARM`, `circle_follow()`,
  and refuses anything above the caller's level. The number recruited scales
  from one at artifact level 1 to `ARTIFACT_CHARM_MAX` at level 5.

### The signature weapon procedures

`artifact_signature_proc()` rolls before the generic proc system on every
successful hit and ignores `ARTIFACT_PROC_ICD`, exactly as ROL's did. It
returns TRUE when the victim died so the caller stops touching it.

| Artifact | Behavior |
| --- | --- |
| Trorxek | blinding strike on a critical hit |
| Kelrarin | 1-in-29 thrown hammer with full lifesteal; at alignment > 990 and 90%+ hp, a 1-in-33 mega blast for 350 followed by ROL's sudden-death check |
| Kelrom | instantly kills a wielder who strikes a `RACE_TYPE_ANIMAL`; otherwise a group-scoped healback |
| Gesen | 1-in-31 thrown axe carrying `SPELL_HARM` |
| Avernus | below 100 hp, a 30%-plus-level chance to full-heal the wielder |

Where ROL used flat numbers, ours scale with artifact level: Kelrarin's throw
ceiling, Kelrom's healback percentage, Gesen's harm caster level, and
Avernus's heal chance.

## Gap 2: class restrictions and the burn penalty - PORTED

Templates carry `class_restrict` and `class_min_level`. Because LuminariMUD
is multi-class, the gate is a **depth of commitment**, not an identity check:
`ART_CLASS_GATE` (10) levels in the named class.

| Artifact | Class |
| --- | --- |
| Trorxek | Druid |
| Amaukekel | Cleric |
| Fade | Rogue |
| Horn of Henekar | Rogue |
| Doombringer | Warrior |

The rest are unrestricted. Failing the check does not stop you wearing the
artifact - it burns you `dice(5, 4)` `DAM_FIRE` every tick from
`artifact_burn_tick()`, hooked into `point_update()`. One burn per tick no
matter how many artifacts object. A failing wielder also cannot invoke any
called effect and is not shown the phrases. `LVL_IMMORT` and above are
exempt.

## Gap 3: special identify text - PORTED

`artifact_show_called_effects()` reproduces ROL's `PROC_SPECIAL_ID` inside
`artifact info <item>`: every phrase the artifact answers to, what it does,
how often it recharges, and whether it is ready right now. It is generated
from the same table the dispatcher reads, so the two cannot drift apart.

It is class-gated the way ROL's was. An artifact that does not recognize you
tells you nothing.

`artifact info` also names the class oath and warns you when you are failing
it.

## Gap 4: critical-hit and boss XP multipliers - PORTED

`is_critical` is threaded from `handle_successful_attack()` into
`artifact_combat_hit()`, so `ARTIFACT_XP_CRIT` (3) now replaces
`ARTIFACT_XP_HIT` (1) on a critical. This also closes limitation 1.

LuminariMUD still has no `ACT_BOSS` flag, so `artifact_is_boss()` defines the
boss case by level instead: an NPC at least `ARTIFACT_BOSS_LEVEL_MARGIN` (3)
levels above its attacker. `ARTIFACT_XP_BOSS_HIT_MULT` and
`ARTIFACT_XP_BOSS_KILL_MULT` are live against that test. No constant in the
header is dead any longer.

Kill XP keeps its `10 + victim level / 5` base and applies the boss
multiplier on top.

## Legacy save-file compatibility

Our current format is **v2.2**. The loader also parses both documented ROL
layouts separately:

- ROL v1: `vnum owner timestamp`
- ROL v2.0: `vnum owner level exp binding_type bound_time`
- LuminariMUD v2.1: `vnum owner account level exp bound_time`

The `pos` field and the persistent `art_f` file handle that ROL used for
in-place record rewriting do not exist in our struct. Legacy binding rules
are still taken from the current code-side templates.

Recharge stamps and class restrictions are **not** persisted, for the same
reason stat blocks are not: they belong to the template, not to the player's
save data.

## Gap 6: generic combat XP is now targeted - PORTED

Per the decision recorded here, `artifact_grant_xp()` collects every equipped
artifact and pays exactly one, chosen at random. The XP pool is unchanged; it
simply lands in one place instead of being multiplied by how many artifacts
you happen to be wearing. ROL defect 3 is now fully fixed.

## Gap 7: minor omissions - PORTED

- **`MEM_ARTIFACT`** - this codebase has no `debug.c` and no global
  memory-accounting framework, so building one around a single subsystem was
  not the right trade. `artifact_memory_used()` computes the number ROL's
  bucket was reporting - registry array plus every owner and account string -
  and `testartifact status` prints it.
- **"dropped" ownership state** - `artifact_is_dropped()` reports an artifact
  that still has an owner on record but no live instance on anyone's person.
  `testartifact status` counts owned / dropped / unowned, and `testartifact
  list` carries a State column.
- **`tagBogusArtifact()`** - still deliberately not ported. See deviation 3.

---

# 4. Deliberate deviations from ROL

Changes made on purpose, distinct from the gaps above.

1. **Membership is the registry, not a flag.** ROL set `IDX_ARTIFACT`
   (BIT_16 of `obj_index[].spec_flag`) and tested it at ~30 call sites.
   LuminariMUD has no equivalent bitfield, and a second structure to keep in
   sync is a liability. Same semantics, one fewer thing to desync.

2. **Mana became psp.** ROL's `mana_bonus` is our `psp_bonus`; abilities
   spend `GET_PSP()`.

3. **`tagBogusArtifact()` dropped.** ROL overloaded `obj->cost = -1` to mark
   throwaway copies so extracting them would not strip the real owner - a
   hack its own comment concedes "should likely be fixed by creating a new
   data member". It was needed because ROL's save path created temp copies.
   LuminariMUD's `Crash_save()` serializes in place, so instead the extract
   hook skips release while a PC still carries or wears the object.

4. **Single writer, one format.** ROL had two conflicting writers: in-place
   `fseek()` rewrites of 80-byte v1 records, and whole-file v2 saves. Since a
   v2 record is not 80 bytes wide, an in-place write after a v2 save landed
   at the wrong offset. We have `artifact_save()` and nothing else.

5. **`BIND_ON_ACCOUNT` uses real accounts.** ROL left this as a
   character-name check with a TODO. LuminariMUD has accounts, so we compare
   `GET_ACCOUNT_NAME()`.

6. **Kill XP scales with victim level** rather than a boss flag - see gap 4.

7. **`artifact abilities` is real.** ROL shipped it as a "still being
   implemented" stub. Ours lists equipped abilities with live cooldown and
   cost, since the registry already holds the data.

8. **`testartifact reset <vnum>` added.** Not in ROL. Clears ownership and
   binding on one artifact, for when a player is deleted or an item needs to
   re-enter circulation.

---

# 5. Upstream reference: ROL source map

**This section describes RealmsOfLuminari, not this repository.** It is kept
so anyone porting the remaining content layer knows where to look.

Paths are relative to `EXAMPLE/RealmsOfLuminari/`.

| Area | ROL location |
| --- | --- |
| Artifact special procedures + abilities | artifact implementation source (2813 lines) |
| Data structure | `src/structs.h:805` |
| Feature flag | `src/config.h:45` (`#define ARTIFACT`) |
| Ownership core, bonuses, binding, XP | `src/handler.c:4296-5134` |
| Boot / persistence / cleanup | `src/db.c:5293-5665` |
| Zone-reset single-instance guard | `src/db.c:3689, 3733, 3786, 3831` |
| Combat integration | `src/combat.c:2355, 2799, 3054, 3478` |
| Player command | `src/actinf.c:7927-8276` |
| Wizard command | `src/actwiz.c:10656-10842` |
| Command table | `src/interp.c:1941-1946`, `src/interp.h:747-751` |
| Spec-proc flag | `src/specs.include.h:65` (`IDX_ARTIFACT`, BIT_16) |
| Spec-proc assignment | `src/specs.assign.c:1717-1729, 1863` |
| Affect type | `src/spells.h:694` (`SPELL_ARTIFACT_BONUS` = 527) |
| Crash-save protection | `src/files.c:946, 1082, 1495, 1505, 4888, 4899` |
| Memory accounting | `src/debug.c:342-358` (`MEM_ARTIFACT`) |
| Prototypes | `src/prototypes.h:159-161, 1296-1309` |
| Data file | `areas/world.artifact` (v2.0, 12 records) |
| ROL's own doc | `docs/reference/artifacts.md` (136 lines, high level) |

~623 artifact references outside that implementation source.

## 5.1 The eleven special procedures (gap 1 detail)

Each is an object spec proc invoked with a `calltype` decoded by the
`PARSE_ARG` macro (`type = arg - (arg % 10000)`). `PROC_*` calltypes are
multiples of 10000: `PROC_COMMAND` 0, `PROC_EVENT` 870000,
`PROC_INITIALIZE` 890000, `PROC_SPECIAL_ID` 960000, `PROC_WEAPON_HIT`
990000.

Shared shape: `PROC_INITIALIZE` registers per-effect recharge timers and
returns an `IDX_*` mask of wanted events; `PROC_SPECIAL_ID` emits class-gated
identify text; `PROC_EVENT` clears a spent-effect bit when its timer fires
and burns a wielder who fails the class restriction; the command path matches
a phrase said aloud, fires the effect, sets the spent bit, and queues the
recharge event.

| ROL vnum | Proc | Class | Effects |
| --- | --- | --- | --- |
| 1043 | `OakenDefender` | Druid | "come oaken defender" summon treant /week; "carpet of death" creeping doom /day; "forest path home" recall /hour; "moonlit path to <t>" moonwell /day. Blinding-strike weapon crit. +75 hp, +25 Con, 5d4. |
| 1044 | `Amaukekel` | Cleric | "sunlit path to paradise" dimension shift /week; "give life to <corpse>" resurrect /day; "wrath of light <t>" dispel evil /hour. +100 hp, +10 max Con. |
| 1042 | `Fade2` | Thief | "eyes of darkness <t>" blind /day; "darken the world" darkness /hour; "devour the soul" weaken /week; "shadowy path to <t>" /hour. +100 hp, +20 hitroll, 5d4. |
| 1046 | `HornOfHenekar` | Thief | "you see darkness <t>" blind /6h; "peace to you" pacify /12h; "join my quest" charm mobs under 2000 hp /12h; "sonic path to <t>" /hour. +100 hp, -15 save vs spell. |
| 1050 | `Doombringer` | Warrior | "bring annhilation forth!" /week; "feel my power <t>" black lightning /day; "enrage me doombringer!" mega doom /day. +8 hit, +8 dam, 5d4. |
| 1007, 1009 | `Kelrarin` | - | Weapon hit. 1-in-29 thrown-hammer strike up to 250 damage with full lifesteal. At alignment > 990 and near-full hp, a 1-in-33 mega blast for 350 damage plus a sudden-death check under 350 hp. |
| 1048 | `Kelrom` | - | Weapon hit. Instantly kills the wielder if they strike a `RACE_ANIMAL`; otherwise a group-scoped healback proc. |
| 5343 | `Gesen` | - | Weapon hit. 1-in-31 thrown-axe strike casting full harm. |
| 19730 | `New_Avernus` | - | Weapon hit. Below 100 hp, 30% chance to full-heal the wielder. Identify text advertises +20 hit / +8 dam and "Bladesong". |
| 1045 | `NeverLooseItem` | - | **Not an artifact.** A staff debug toolbelt (`tt <who>`, `recall`, `cc`, `heal`) that happens to live in this file. Do not port as an artifact. |
| 1047 | `MinorHealback` | - | **Dead.** `return 0;` before its body runs, and commented out of the assignment table. |

All nine of the real artifact procs above are now ported - see section 3, gap
1, for what each became here. `NeverLooseItem` and `MinorHealback` were not
and will not be: one is not an artifact and the other never ran.

ROL's spec-proc framework itself (`initializeObjectFeature`,
`rechargeObjectFeature`, `checkObjectRestrictions`,
`performRestrictionPenalty`, `specialProcedureIdentifyObject`,
`usedFeatureMessage`, the `BACKUP_*`/`RESTORE_*` macros) was **not**
transcribed. Its jobs are done directly instead:

| ROL machinery | Ours |
| --- | --- |
| `PROC_COMMAND` phrase matching | `artifact_speech_trigger()` from `do_say()` |
| `PROC_INITIALIZE` + `PROC_EVENT` recharge queue | `effect_used[]` stamps + `artifact_recharge_remaining()` |
| `checkObjectRestrictions` / `performRestrictionPenalty` | `artifact_class_ok()` + `artifact_burn_tick()` |
| `specialProcedureIdentifyObject` | `artifact_show_called_effects()` |
| `spell_creeping` | `SPELL_CREEPING_DOOM` |
| `spell_moonwell` | `artifact_travel_to()` |
| `spell_dimension_shift` | `artifact_dimension_shift()` |
| `cast_full_harm` | `SPELL_HARM` via `call_magic()` |
| `SuddenDeath` | inline in `artifact_proc_kelrarin()` |

## 5.2 ROL's own artifact stat table

For reference only - our stat blocks differ and use our own vnums.

| ROL vnum | Name | Stats | Combat | Resources | Resist | Proc |
| --- | --- | --- | --- | --- | --- | --- |
| 1007 / 1009 | Kelrarin's Hammer | +2 Str, +1 Con | +3 hit, +3 dam | - | - | 15% |
| 1044 | Rod of Light | +2 Int, +2 Wis | +2 AC | +50 mana | - | - |
| 1050 | Doombringer | +3 Str, +1 Dex | +4 hit, +5 dam | +30 hp | - | 20% |
| 1008 | Tiamat's Stinger | +3 Dex, +1 Str | +5 hit | +50 move | 10% phys | 18% |
| 1046 | Horn of Henekar | +2 Int, +1 Wis, +1 Cha | - | +75 mana | 15% magic | - |

All other ROL vnums got defaults: level 1, no bonuses, no ability, 300s
cooldown, no proc.

---

# 6. ROL defects fixed in this port

Found while mapping the upstream. All are fixed here unless noted.

| # | ROL defect | Status |
| --- | --- | --- |
| 1 | `remove_artifact_bonuses()` strips every artifact's affects, not one artifact's - wearing two artifacts was broken | Fixed via `af.specific` source tagging |
| 2 | A level-up did not reapply bonuses; re-equip was required | Fixed - `artifact_refresh_bonuses()` |
| 3 | `grant_artifact_xp()` awarded to every equipped artifact | Fixed - procs and abilities are targeted; generic combat XP pays one random equipped artifact |
| 4 | Weapon-proc internal cooldown written but never read; the documented "30-second cooldown" did not exist | Fixed - `ARTIFACT_PROC_ICD` is enforced |
| 5 | `ability_cost` never populated; all three costs were hardcoded literals and `artifact info` always printed 0 | Fixed - populated from the template table |
| 6 | In-place v1 record writes conflicted with v2 whole-file saves | Fixed - one writer |
| 7 | `uniqueArtifact()` declared in two files, defined in none | Not carried over |
| 8 | `check_artifact_duplicates()` / `validate_artifact()` declared inside `do_testartifact()`, defined nowhere | Not carried over; checks are inline |
| 9 | `tagBogusArtifact()` overloaded `obj->cost = -1` as a flag | Not carried over - see deviation 3 |
| 10 | `BIND_ON_ACCOUNT` was a character-name check with a TODO | Fixed - real account comparison |
| 11 | `Amaukekel`'s `PROC_EVENT` had a stray `;` and an unconditional `return FALSE`, making its restriction check unreachable | Fixed by construction - the restriction check is one shared `artifact_class_ok()`, not per-proc code |
| 12 | `OakenDefender`'s defender branch had `return TRUE;` before its summon logic, making the treant summon unreachable | Fixed - `artifact_summon_treant()` runs, and the treant is a tracked prototype (169912) rather than a dangling vnum |

---

# 7. Known limitations of this implementation

Shortcomings of our own code, distinct from anything upstream.

1. **`ARTIFACT_ZONE`, `ARTIFACT_VNUM_BASE`, and `ART_VNUM_VAULT` are unused**
   in the implementation. They document the allocation but nothing reads
   them.
2. **Recharge stamps do not survive a reboot.** A restart hands every called
   effect back ready. ROL's event-queue timers behaved the same way, so this
   matches upstream rather than falling short of it, but it is still an
   exploitable seam on a MUD that reboots often. Persisting them means a v2.3
   save format.
3. **No `SPECIAL()` procedures.** Artifact behavior hangs off the core-file
   hooks and the speech trigger, not off LuminariMUD's spec-proc dispatch, so
   nothing artifact-specific can react to being looked at or examined.
4. **The registry is fixed at compile time.** Adding an artifact requires
   editing `artifact_templates[]` (and `artifact_effects[]` for called
   effects) and rebuilding. There is no OLC support.
5. **Effect slots are assigned by hand.** `artifact_effects[].slot` must be
   unique per artifact and inside `ARTIFACT_MAX_EFFECTS`; nothing at runtime
   enforces it beyond a `SYSERR` on an out-of-range slot. A duplicate slot
   would silently share one recharge clock between two effects.

---

# 8. Verification performed

## Original port

- `make -j$(nproc)`: clean, zero warnings.
- `make test`: 104/104 pass (19 new artifact tests, 85 pre-existing).
- `make install`: `bin/circle` updated, no stray root binary.
- Boot #1: registry initialized 11 artifacts; `world.artifact` created; zone
  1699 reset without error.
- Boot #2: seeded artifact 169905 as owned by Zusuk at level 3 / 450 XP with
  an account and a bind timestamp; confirmed every field loaded back and
  persisted across shutdown.
- Boot #3: with zone spawn commands added, zone 1699 parsed and reset with no
  `ZONE ERROR` and no `SYSERR`.

## Content-layer port (section 3)

- `make -j$(nproc)`: clean, zero warnings.
- `gcc -Wall -Wextra -fsyntax-only src/world/spec_artifacts.c`: clean.
- `make test`: 133/133 pass (33 artifact tests, 14 of them new).
- `make install`: `bin/circle` updated, no stray root binary.
- Boot: zero `SYSERR` lines. Registry initialized 11 artifacts; zone 1699
  reset without error. The two pre-existing `SYSERR: Help entry does not have
  a min level` lines from `artifacts.hlp` were traced to a `#` terminator that
  should have been `#0`, and fixed.
- Oaken Defender prototype confirmed loaded by differential mob count: 14659
  with `1699.mob` in place against 14658 with it emptied.

## Test coverage

`unittests/CuTest/test_artifacts.c` covers binary search (hits, misses, empty
registry), ownership sentinels, the XP curve including out-of-range input,
level-up behavior and the max-level ceiling, binding-name mapping, save
round-tripping in a sandboxed temp directory, shutdown idempotency, and
NULL-safety of the entire hook surface.

The content-layer port added coverage for:

- per-effect recharge timers: ready when never used, counting down from full,
  clearing once elapsed, and rejecting both out-of-range slots and a stale
  stamp on a slot with no effect behind it
- every recharge-interval name
- speech invocation refusals: ordinary talk, empty and NULL input, a phrase
  prefix that is not the whole phrase, a targeted phrase with no target, and
  the right words without the artifact on your person
- effect-table integrity: every phrase is already in the normalized form the
  matcher requires, and every one of the eighteen is a clean miss rather than
  a crash or a false positive
- the class check with missing arguments and on an unrestricted artifact
- the dropped ownership state and registry memory accounting
- generic combat XP landing on exactly one of two equipped artifacts

**Not covered by tests:** anything needing a booted world - the equip/unequip
path, binding enforcement in `equip_char()`, damage resistance, weapon procs
including all five signature procedures, the three abilities, the called
effects actually firing, the burn tick, and both command handlers. Those were
exercised only by the boot runs above, which do not simulate a player.

## Bugs found and fixed during implementation

Introduced by the port itself, caught during review:

1. **Binding bypass on pickup.** `artifact_to_char()` rewrote the owner
   unconditionally, so anyone who picked up a bound artifact became its owner
   and then passed their own binding check - binding was completely defeated.
   Fixed: a bound artifact never changes owner on pickup.
2. **Bind-on-account never bound.** Only `ARTIFACT_BIND_ON_EQUIP` set
   `bound_time`, so account-bound artifacts stayed permanently unbound.
   Fixed: bind-on-equip and bind-on-account both bind on first wear;
   bind-on-pickup binds in `artifact_to_char()`.
3. **`artifacts.hlp` used `#` as its entry terminator** where the loader wants
   `#<min level>`, so both artifact help entries logged
   `SYSERR: Help entry does not have a min level` on every boot. Fixed to
   `#0` in both the tracked package and the deployed copy.
4. **`artifact_is_dropped()` walked the container chain with its own loop
   cursor**, which would have reassigned the `object_list` iterator mid-scan
   and skipped the rest of the world. Fixed with a separate pointer.

---

# 9. Open decisions and next steps

## Decision left to a builder: where artifacts live

The eleven artifacts spawn into room 169900, "the Vault of Ages", flagged
NOMOB / INDOORS / PRIVATE. They are therefore staff-staged: reachable with
`goto`, distributable with `testartifact spawn`, but not lootable by players
where they currently sit.

Placing them in live content means editing other zones' files and deciding
what content each artifact should gate - a world-design call, not a code one.
Move the `O` lines out of `lib/world/zon/1699.zon` into the target zones, or
load them onto mobs, and the single-instance guard handles the rest.

## Candidate follow-up work, roughly by value

1. Integration tests that boot a world and drive a fake player through
   equip / bind / proc / ability / called effect / burn tick. This is now the
   single largest hole: the entire content layer is verified only by boot runs
   and by tests of the parts that do not need a world.
2. Balance pass on the called effects. The numbers are ROL's shape scaled to
   LuminariMUD's, chosen by hand and never played. `bring annhilation forth`
   in particular (`dice(10 + level*4, 12) + charlevel*3` room-wide) deserves a
   second look against real content.
3. Persist recharge stamps (limitation 2), which needs a v2.3 save format.
4. Validate `artifact_effects[]` at boot: unique slots per artifact, slots in
   range, phrases already normalized, every vnum a real artifact. Today this
   is a unit test over a hand-copied phrase list rather than a runtime check
   over the real table (limitation 5).
5. Decide where the artifacts actually live, per the section above.
6. Get the deployment package into version control, per the warning in
   section 1. Until that happens the entire artifact system is one fresh
   clone away from booting into eleven missing-prototype errors, and none of
   the content ported here can run.
