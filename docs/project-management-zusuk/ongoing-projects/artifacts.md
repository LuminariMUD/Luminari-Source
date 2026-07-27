# Artifact System

## Status: LIVE

Implemented, hooked into the core, built clean, boot-verified, unit-tested.
The system is compiled in unconditionally - there is no feature guard.

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
| `src/spells.h` | `SPELL_ARTIFACT_BONUS` = 1610 |

## Data

| Path | Contents |
| --- | --- |
| `lib/world/world.artifact` | runtime ownership state, gitignored |
| `lib/world/artifacts/1699.*` | tracked deployment package for zone 1699 |
| `lib/world/artifacts/artifacts.hlp` | tracked artifact help entries |
| `scripts/provision_artifacts.sh` | idempotent world/index/help provisioning |

## Commands

- Player: `artifact [list | info <item> | progress | abilities | help]`
- Abilities: `soulstrike [target]`, `divineward`, `doomblast`
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
| successful damaging hit on an NPC | 1 | all equipped artifacts |
| kill an NPC | 10 + victim level / 5 | all equipped artifacts |
| weapon proc: soul / heal / fear / doom / ultimate | 2 / 1 / 3 / 4 / 10 | targeted |
| ability soulstrike / divineward / doomblast | 15 / 20 / 10 per target | targeted |

"Targeted" means only the artifact that earned it. Generic combat XP still
spreads across every equipped artifact - see gap 6 in section 3.

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
              dam_killed_vict()    -> artifact_combat_kill()
interpreter.c cmd_info[]           -> artifact, soulstrike, divineward,
                                      doomblast, testartifact
```

## 2.13 Artifact roster

| VNUM | Name | Item type | Binding | Ability | Proc |
| --- | --- | --- | --- | --- | --- |
| 169900 | the Vault of Ages | room | - | - | - |
| 169901 | Trorxek, the Staff of Ancient Oaks | quarterstaff | on equip | - | 12% |
| 169902 | Amaukekel, the Rod of Light | light mace | on equip | divineward | - |
| 169903 | Fade, the Shadowblade | short sword | on equip | - | 16% |
| 169904 | The Horn of Henekar | held | on equip | - | - |
| 169905 | Doombringer | great sword | on pickup | doomblast | 20% |
| 169906 | Kelrarin's Hammer | warhammer | on equip | soulstrike | 15% |
| 169907 | Kelrom, the Axe of Pahluruk | battle axe | on equip | - | 14% |
| 169908 | Gesen, the Returning Axe | hand axe | none | - | 18% |
| 169909 | Tiamat's Stinger | rapier | on account | - | 18% |
| 169910 | Avernus, the Black Blade | long sword | on equip | - | 15% |
| 169911 | The Aegis of Ages | body armor | on equip | - | - |

---

# 3. NOT ported from ROL

**Read this section before assuming any ROL behavior exists here.**

The *systemic* layer of the ROL artifact system is fully ported. The
*content* layer largely is not. These eleven artifacts carry ROL's names but
not ROL's behavior.

## Gap 1: the eleven per-artifact special procedures

This is the largest gap. In ROL each artifact had a hand-written spec proc
with its own personality. None of it is here:

- **Invocation by speech.** `say "carpet of death"`, `"forest path home"`,
  `"bring annhilation forth!"`, and so on. Nothing in our port listens to
  `say` at all.
- **Per-effect recharge timers.** Separate 1/hour, 1/day, 1/week cooldowns
  for multiple effects on the same object, driven by ROL's event queue. We
  have one cooldown per artifact.
- **The signature effects themselves.** Summon treant, creeping doom,
  moonwell, resurrection, dimension shift, charm, pacify, blind, darkness,
  teleport-to-player, thrown-weapon return strikes, the alignment-gated mega
  blast, Kelrom killing its wielder for striking an animal, group healback.
  None exist. Our artifacts use the generic proc/ability system instead.

Section 5 catalogues all eleven procs in detail for anyone porting them
later.

## Gap 2: class restrictions and the burn penalty

ROL gated artifacts by class (`OBJ_RESTRICT_CLASS`) and scorched a wielder
who failed the check (`performRestrictionPenalty(..., OBJ_BURN)`). A
non-druid holding Trorxek took damage every tick.

Not ported. Our artifacts have no class restriction whatsoever - binding is
the only gate.

## Gap 3: special identify text

ROL's `PROC_SPECIAL_ID` emitted bespoke, class-gated identify text per
artifact, listing its called effects and their cooldowns.

Not ported. `artifact info <item>` shows generated stat/ability output
instead, which is arguably more useful but is not the same feature.

## Gap 4: critical-hit and boss XP multipliers

ROL granted 3 XP for a critical hit instead of 1, and multiplied hit XP by 2
and kill XP by 3 against `ACT_BOSS` mobs.

Not ported. `artifact_combat_hit()` grants a flat 1 XP regardless of whether
the hit was a critical. `ARTIFACT_XP_CRIT`, `ARTIFACT_XP_BOSS_HIT_MULT`, and
`ARTIFACT_XP_BOSS_KILL_MULT` are defined in the header but **never
referenced** - they are dead constants kept as documentation of intent.

LuminariMUD has no `ACT_BOSS` equivalent, so kill XP scales with victim level
(`10 + level/5`) instead. The crit case has no such excuse and is simply
unimplemented - see section 7.

## Legacy save-file compatibility

Our current format is **v2.2**. The loader also parses both documented ROL
layouts separately:

- ROL v1: `vnum owner timestamp`
- ROL v2.0: `vnum owner level exp binding_type bound_time`
- LuminariMUD v2.1: `vnum owner account level exp bound_time`

The `pos` field and the persistent `art_f` file handle that ROL used for
in-place record rewriting do not exist in our struct. Legacy binding rules
are still taken from the current code-side templates.

## Gap 6: XP still spreads across all equipped artifacts

Procs and abilities grant XP only to the artifact that earned it. Generic
combat XP (hits and kills) still goes to **every** equipped artifact, which
is ROL's behavior. This is a partial fix of ROL defect 3, not a complete one.
Whether it should be fully targeted is a design question, not a bug.

## Gap 7: minor omissions

- **`MEM_ARTIFACT`** - ROL's memory-accounting bucket in `debug.c`. Not
  ported; we do not track artifact allocations separately.
- **"dropped" ownership state** - ROL's `testartifact status` reported
  owned / dropped / unowned. Ours reports owned / unowned only; there is no
  distinct "dropped" state in our model.
- **`tagBogusArtifact()`** - see deviation 3 in section 4. Deliberately not
  ported.

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
| Artifact special procedures + abilities | `src/specs.artifacts.c` (2813 lines) |
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

~623 artifact references outside `specs.artifacts.c`.

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

Porting these requires rebuilding ROL's spec-proc framework on LuminariMUD's
`SPECIAL()` dispatch (`initializeObjectFeature`, `rechargeObjectFeature`,
`checkObjectRestrictions`, `performRestrictionPenalty`,
`specialProcedureIdentifyObject`, `godMaintenanceCommands`,
`usedFeatureMessage`, the `BACKUP_*`/`RESTORE_*` macros), plus LuminariMUD
equivalents for `spell_creeping`, `spell_moonwell`, `spell_dimension_shift`,
`cast_full_harm`, and `SuddenDeath`.

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
| 3 | `grant_artifact_xp()` awarded to every equipped artifact | Partially fixed - see gap 6 |
| 4 | Weapon-proc internal cooldown written but never read; the documented "30-second cooldown" did not exist | Fixed - `ARTIFACT_PROC_ICD` is enforced |
| 5 | `ability_cost` never populated; all three costs were hardcoded literals and `artifact info` always printed 0 | Fixed - populated from the template table |
| 6 | In-place v1 record writes conflicted with v2 whole-file saves | Fixed - one writer |
| 7 | `uniqueArtifact()` declared in two files, defined in none | Not carried over |
| 8 | `check_artifact_duplicates()` / `validate_artifact()` declared inside `do_testartifact()`, defined nowhere | Not carried over; checks are inline |
| 9 | `tagBogusArtifact()` overloaded `obj->cost = -1` as a flag | Not carried over - see deviation 3 |
| 10 | `BIND_ON_ACCOUNT` was a character-name check with a TODO | Fixed - real account comparison |
| 11 | `Amaukekel`'s `PROC_EVENT` had a stray `;` and an unconditional `return FALSE`, making its restriction check unreachable | N/A - proc not ported |
| 12 | `OakenDefender`'s defender branch had `return TRUE;` before its summon logic, making the treant summon unreachable | N/A - proc not ported |

---

# 7. Known limitations of this implementation

Distinct from the ROL gaps in section 3 - these are shortcomings of our own
code.

1. **Critical hits grant no bonus XP.** `artifact_combat_hit()` does not
   receive or inspect hit type, so `ARTIFACT_XP_CRIT` is unused. Fixing this
   means threading the crit flag through from `handle_successful_attack()`.
2. **Generic combat XP is untargeted** - see gap 6.
3. **`ARTIFACT_ZONE`, `ARTIFACT_VNUM_BASE`, and `ART_VNUM_VAULT` are unused**
   in the implementation. They document the allocation but nothing reads
   them.
4. **No spec procs.** The artifacts have no `SPECIAL()` procedures at all, so
   nothing artifact-specific can react to being worn, removed, or looked at
   beyond the generic hooks.
5. **The registry is fixed at compile time.** Adding an artifact requires
   editing `artifact_templates[]` and rebuilding. There is no OLC support.

---

# 8. Verification performed

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

## Test coverage

`unittests/CuTest/test_artifacts.c` covers binary search (hits, misses,
empty registry), ownership sentinels, the XP curve including out-of-range
input, level-up behavior and the max-level ceiling, binding-name mapping,
save round-tripping in a sandboxed temp directory, shutdown idempotency, and
NULL-safety of the entire hook surface.

**Not covered by tests:** anything needing a booted world - the equip/unequip
path, binding enforcement in `equip_char()`, damage resistance, weapon procs,
the three abilities, and both command handlers. Those were exercised only by
the boot runs above, which do not simulate a player.

## Two bugs found and fixed during implementation

Introduced by the port itself, caught during review:

1. **Binding bypass on pickup.** `artifact_to_char()` rewrote the owner
   unconditionally, so anyone who picked up a bound artifact became its owner
   and then passed their own binding check - binding was completely defeated.
   Fixed: a bound artifact never changes owner on pickup.
2. **Bind-on-account never bound.** Only `ARTIFACT_BIND_ON_EQUIP` set
   `bound_time`, so account-bound artifacts stayed permanently unbound.
   Fixed: bind-on-equip and bind-on-account both bind on first wear;
   bind-on-pickup binds in `artifact_to_char()`.

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

1. Port the eleven special procedures (gap 1) - the largest missing feature.
2. Add class restrictions (gap 2) so artifacts read as class-defining.
3. Thread crit state into `artifact_combat_hit()` (limitation 1).
4. Decide whether generic combat XP should be targeted (gap 6).
5. Add per-artifact special identify text (gap 3).
6. Integration tests that boot a world and drive a fake player through
   equip / bind / proc / ability.

## Note on the paused code

`src/specs.artifacts.c` / `src/specs.artifacts.h` remain in the tree and are
**not** part of this system. That file is a verbatim paste of ROL's
`src/specs.artifacts.c` wrapped in `#ifdef NOT_CONVERTED`, which is
`#undef`'d immediately above it, so the whole file compiles to nothing. It is
not HomelandMUD code - HomelandMUD has no artifact source at all, only a
`lib/misc/artifacts` data file. It can be deleted once the special procedures
in section 5.1 are either ported or formally abandoned.
