# Thrown Weapons Implementation Plan

- Status: Proposed; implementation not started
- Analysis date: 2026-08-23
- Environment reviewed: development

This is a planning document, not a description of current game behavior.

## 1. Goal

Add native thrown-weapon combat with this player contract:

- A throwable weapon is an ordinary melee weapon while it is wielded. Normal commands such as
  `hit` use it in melee.
- The explicit command `throw <target> [direction]` starts a ranged attack mode comparable to
  the existing `fire` command.
- The wielded throwable establishes the weapon prototype to use. Each attack consumes one
  throwable `ITEM_WEAPON` with the same object VNUM, in this order:
  1. a matching copy in the equipped `WEAR_AMMO_POUCH` slot;
  2. a matching top-level copy in inventory;
  3. the originally wielded copy, after all reserve copies are gone.
- Every attack uses the actual selected object instance. Its enhancement, poison, scripts, and
  special abilities apply; another copy or the next pouch object must not stand in for it.
- When the last reserve is gone, the wielded copy is unequipped and thrown. If it does not
  return, the character is left unarmed in that slot and thrown mode ends cleanly.
- Existing bows, crossbows, slings, ammunition, `fire`, `reload`, and `collect` behavior must not
  regress.

The equipped `WEAR_AMMO_POUCH` slot is already the game's quiver/ammunition-belt slot. The
implementation should broaden what an `ITEM_AMMO_POUCH` can hold; it should not add another wear
slot or use `WEAR_ON_BACK` as a second ammunition source.

## 2. Current-state findings

The design below follows traced code rather than naming assumptions.

| Area | Current behavior or constraint |
|------|--------------------------------|
| Weapon metadata | `WEAPON_FLAG_THROWN` already marks dagger, knife, shortspear, spear, dart, javelin, throwing axe, light hammer, trident, sai, bola, net, shuriken, and athame profiles in `src/combat/assign_wpn_armor.c`. No live helper performs a thrown attack. |
| Magic metadata | `WEAPON_SPECAB_THROWING` and `WEAPON_SPECAB_RETURNING` exist in `src/combat/spec_abilities.h`, but their registrations have no procedures and neither ability currently implements throwing behavior. |
| Melee default | `do_hit` rejects a wielded profile with `WEAPON_FLAG_RANGED`. A thrown-only javelin is therefore melee-usable today, which is the correct default to preserve. |
| Launcher combat | `do_fire`, `can_fire_ammo()`, and the ranged branch of `perform_violence()` require a launcher plus compatible `ITEM_MISSILE` objects in the equipped ammo pouch. |
| Physical projectile | `hit(..., ATTACK_TYPE_RANGED)` removes the first pouch object and moves it to the target, target room, or another outcome. Several calculations still look back at the pouch or global `last_missile`, so the code can accidentally observe the next object rather than the object in flight. |
| Early exits | Wind wall and protection-from-arrows paths can return after detaching a missile without one centralized ownership/finalization step. Refactoring this is a prerequisite for safely adding durable thrown objects. |
| Ammo pouch | `perform_put()` currently allows only `ITEM_MISSILE` in `ITEM_AMMO_POUCH`. Contents and equipped-container nesting are already saved generically by object persistence. |
| Dart collision | `WEAPON_TYPE_DART` is both `WEAPON_FLAG_THROWN` and `WEAPON_FLAG_RANGED`. It is also the launcher paired to `AMMO_TYPE_DART`, and the RoL converter maps both thrown darts and blowguns to it. This must be split before darts can be melee by default without breaking blowguns. |
| Converted content | The RoL converter deliberately turns thrown ammunition into `ITEM_WEAPON` and throwing quivers into plain containers because the target currently lacks a throw command and ammo pouches reject weapons. The new feature must retire both workarounds. |
| Collection | `collect` only recognizes owner-tagged `ITEM_MISSILE` objects and requires a usable ammo pouch. `MISSILE_ID()` is stored on all object instances, so throwable weapons can use the same ownership tag without a schema change. |
| Persistence | `char_special_data.firing` is a non-persistent `bool`. Thrown combat needs an explicit non-persistent mode plus a stable prototype/slot anchor; it must not store a potentially dangling object pointer. |

Relevant prior analysis is in
[ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md](ROL_CONVERTER_WEAPON_TYPE_INFERENCE.md) and
[ROL_CONVERTER_OBJECT_FILE_REFERENCES.md](ROL_CONVERTER_OBJECT_FILE_REFERENCES.md).

## 3. Locked player-facing behavior

These decisions should be treated as acceptance criteria. Changing one during implementation
requires updating tests and this plan before proceeding.

### 3.1 Eligibility and wielded anchor

A specific object is throwable when all of the following are true:

- it is `ITEM_WEAPON`;
- its weapon-type index is valid;
- its weapon profile has `WEAPON_FLAG_THROWN`, or that object has
  `WEAPON_SPECAB_THROWING`;
- transferring it is legal (for example, it is not blocked by `ITEM_NODROP`, binding, or another
  existing possession restriction).

`WEAPON_SPECAB_THROWING` therefore makes an otherwise melee-only weapon throwable.
`WEAPON_SPECAB_RETURNING` does not by itself make a weapon throwable.

The anchor is the first eligible equipped weapon in the existing ranged-weapon slot priority:

1. `WEAR_WIELD_2H`;
2. `WEAR_WIELD_1`;
3. `WEAR_WIELD_OFFHAND`.

The initial implementation deliberately has no weapon-name argument. If more than one throwable
is wielded, the documented priority makes the result deterministic. A future enhancement could
add `throw <weapon> at <target>`, but it is not needed for the requested behavior.

The command records only the anchor's VNUM and wear slot. It must not retain a raw object pointer
between combat pulses. Before every attack, the current object in that slot must still be an
eligible weapon with the recorded VNUM; otherwise throwing mode stops with a useful message.

### 3.2 Reserve selection

For each individual attack, resolve a fresh physical projectile:

1. Scan the equipped `ITEM_AMMO_POUCH` contents in list order for the first eligible
   `ITEM_WEAPON` whose `GET_OBJ_VNUM()` equals the anchor VNUM.
2. If none is found, scan the character's top-level carrying list in list order for the first
   eligible matching object.
3. If none is found, use and unequip the matching anchor in its recorded wear slot.
4. If none is available, clear throwing mode and stop its automatic combat routine. Do not fall
   through to an unrelated melee attack in the same pulse.

Additional rules:

- VNUM equality is the identity rule because that is the requested contract. Short description,
  weapon type, cost, and object name are not substitutes.
- Each selected copy must independently pass the throwable and transfer checks. This protects
  customized same-VNUM instances.
- A prototype-less object whose VNUM is `NOTHING` may be thrown only as the wielded anchor. All
  prototype-less inventory objects must not be treated as identical merely because they share
  the sentinel value.
- Do not recursively search ordinary containers. Only the equipped ammo-pouch contents and
  top-level inventory are reserves.
- Do not consume a matching object from another equipment slot as reserve ammunition.
- Pouch copies take priority over loose inventory copies; the wielded object is always last.
- Re-run selection for iterative, Rapid Shot, and Quick Draw attacks. Each successful attack
  attempt detaches exactly one object and stops cleanly when the next object is unavailable.

### 3.3 Command and target rules

Register `throw` as a new command with the same minimum position, level, and standard-action
category as `fire`:

```text
throw <target>
throw <target> <direction>
```

The command should share target-resolution code with `fire`, including:

- same-room targeting;
- a target in one adjacent room when a valid direction is supplied;
- the current Far Shot or ranger Longshot gate for adjacent-room attacks;
- closed-exit, peaceful-room, PvP, charm/pet, visibility, wilderness-coordinate, group-assist,
  and action-economy handling;
- the current rule that an already active fight cannot be converted by issuing `fire` or
  `throw` mid-round.

Do not invent multi-room range traversal in this feature. `weapon_list[].range` is not enforced
as room distance by `fire` today. Thrown attacks should first match the current same-room or
one-adjacent-room contract, and range-unit work can be planned separately.

`fire` remains launcher-only. Do not alias `throw` to `fire` and do not mark all thrown weapon
profiles as `WEAPON_FLAG_RANGED`; either change would break the melee-by-default requirement.

### 3.4 Combat rules

Use a distinct `ATTACK_TYPE_THROWN` rather than disguising a thrown weapon as launcher ammo. Use
the currently unused mode value 22 without renumbering existing attack types or changing the
persisted weapon-attack verb IDs, and add its display label in `src/constants.c`.

Introduce explicit predicates and audit every current `ATTACK_TYPE_RANGED` comparison:

- `is_launcher_attack()` for bow, crossbow, sling, and blowgun-only behavior;
- `is_thrown_attack()` for the new mode;
- `is_ranged_weapon_attack()` for rules shared by launchers and thrown weapons;
- `has_physical_projectile()` for ownership/finalization behavior.

Do not mechanically replace every ranged comparison. The intended split is:

| Rule | Launcher attack | Thrown attack |
|------|-----------------|---------------|
| Default wielded use | `fire`; melee command rejected | Normal melee unless `throw` was issued |
| Attack object | Wielded launcher | Actual selected throwable object |
| Physical projectile | Compatible `ITEM_MISSILE` | Actual `ITEM_WEAPON` copy |
| Ability to hit | Dexterity/ranged calculation | Dexterity/ranged calculation |
| Strength to damage | Preserve current launcher/composite rules | Full Strength modifier, including a penalty |
| Weapon focus/family/proficiency | Launcher's existing family rules | Selected throwable's real family and proficiency |
| Point Blank, Precise, Deadly Aim, mounted penalty, ranged defenses | Yes | Yes |
| Base iterative attacks | Consume one missile per attack | Consume one weapon per attack |
| Rapid Shot and ranger Quick Draw | Preserve current behavior | Apply and consume one copy per added attack |
| Manyshot and Epic Manyshot | Preserve current launcher behavior | Launcher-only; do not turn an arrow-volley feat into extra thrown weapons |
| Reload and autoreload | Preserve current behavior | Never |
| Imbued Arrow, warbow, arrow-only magic | Preserve current behavior | Never |
| Missile value-slot break chance | Preserve current behavior | Never read `ITEM_WEAPON` value 2 as break chance |
| Returning special ability | Not applicable to the missile | Applies to the selected throwable |

Ranger ranged-perk helpers in `src/character/perks.c` currently test
`WEAPON_FLAG_RANGED`. Where a perk is genuinely about a ranged attack, pass attack mode or use
the shared predicate so thrown weapons qualify. Where a feature is explicitly about arrows,
launchers, or reloading, retain the launcher-only check. The same semantic audit is required in
`src/combat/fight.c`, `src/act.other.c`, `src/obj/act.item.c`, `src/quest/hunts.c`, and OLC code.

### 3.5 Projectile ownership and outcomes

Build one explicit, stack-owned `projectile_attack_context` for each attack. It should contain at
least:

- attack kind;
- explicit attack weapon;
- explicit physical projectile;
- original source (pouch, inventory, or wield slot);
- recorded anchor VNUM and wear slot;
- resolved target room and wilderness location needed for cross-room placement;
- whether the object was detached;
- final disposition.

Pass the explicit attack weapon to attack-bonus, damage, critical, poison, object-script,
artifact, and weapon-special-ability paths. Existing helpers that rediscover the weapon with
`get_wielded()` or rediscover ammo through `GET_EQ(...)->contains` need a context-aware variant.
Keep wrappers for non-projectile callers where that limits churn.

When the selected object is the wielded anchor, use the normal `unequip_char()` path so worn
effects and equipment bookkeeping are removed exactly once. Keep that now-unequipped object as
the explicit attack weapon in the context; do not clear the equipment pointer by hand and do not
rediscover a different weapon after unequipping it.

Replace implicit `last_missile` dependence with this context. A single finalization helper must
own every detached projectile on every return path:

| Outcome | Non-returning throwable disposition |
|---------|--------------------------------------|
| Miss | Target room |
| Hit, target survives | Target inventory, matching current physical-arrow behavior |
| Hit, target dies | Corpse or target room through the normal death/item path; never a dead character's orphaned inventory |
| Deflect Arrows or wind wall | Target room |
| Snatch Arrows | Defender inventory if capacity permits, otherwise target room |
| Protection from Arrows or another negation | Target room |
| Object explicitly destroyed/extracted by a proc | No second move or extraction |
| Invalidated attack before detachment | Original source, unchanged |

Tag a detached throwable with `MISSILE_ID(projectile) = GET_IDNUM(ch)` so only its thrower can
collect it under the normal ownership rule.

For `WEAPON_SPECAB_RETURNING`, resolve the attack normally and then return the actual object to
the attacker unless it was explicitly destroyed or successfully snatched:

1. If it was the wielded anchor and the recorded slot is still empty, re-equip it there.
2. Otherwise put it in top-level inventory if normal carrying limits allow it.
3. If it cannot be caught or carried, place it in the attacker's room.

A successful Snatch Arrows takes precedence over Returning so that the defensive feat has a
meaningful ownership result. Clear `MISSILE_ID` when an object returns to the attacker. Immediate
return after resolution is preferred to a delayed event: it is deterministic, avoids dangling
object references between pulses, and still supplies the intended reusable-weapon behavior.

### 3.6 Combat mode state

Replace the non-persistent `bool char_special_data.firing` with an explicit non-persistent mode:

```text
PROJECTILE_MODE_NONE
PROJECTILE_MODE_LAUNCHER
PROJECTILE_MODE_THROWN
```

Add a recorded thrown-anchor VNUM and wear slot beside it. Replace all `FIRING(ch)` reads and
writes with clear mode helpers; do not rely on assigning integer 2 to a C `bool`.

State transitions must satisfy these rules:

- `fire` selects launcher mode; `throw` selects thrown mode.
- `set_fighting()` may infer launcher mode only when no explicit mode is already set.
- `stop_fighting()`, character extraction, logout cleanup, target loss, invalid anchor, and source
  exhaustion clear mode and anchor state together.
- Reloading may resume launcher mode, never thrown mode.
- `perform_violence()` dispatches by explicit mode. A failed thrown readiness check does not
  silently become launcher fire or melee.
- If the wielded anchor is the last projectile and has Returning, successful re-equip keeps the
  throwing mode valid. Otherwise the now-empty slot ends the mode after that attack.

## 4. Data-model compatibility

### 4.1 Split dart from blowgun without renumbering

Append, do not insert, a new `WEAPON_TYPE_BLOWGUN` after `WEAPON_TYPE_ATHAME`, and raise
`NUM_WEAPON_TYPES` accordingly. Existing numeric weapon IDs are stored in world and player
objects, so no established constant may move.

Then:

- remove `WEAPON_FLAG_RANGED` from `WEAPON_TYPE_DART`, retaining
  `WEAPON_FLAG_THROWN | WEAPON_FLAG_SIMPLE` and its existing thrown-weapon profile;
- register `WEAPON_TYPE_BLOWGUN` as a simple ranged launcher with a reviewed blowgun damage,
  critical, range, size, material, and family profile;
- pair `AMMO_TYPE_DART` with `WEAPON_TYPE_BLOWGUN`, not `WEAPON_TYPE_DART`, in
  `has_ammo_in_pouch()` and related reload/readiness helpers;
- keep dart crafting recipes on `WEAPON_TYPE_DART`;
- update ranged OLC filtering, weapon-list display, proficiency/range helpers, and every switch
  where `WEAPON_TYPE_DART` currently means the launcher rather than the thrown weapon;
- audit native world objects with weapon type 14. Retype only genuine blowgun prototypes to the
  new append-only ID; leave thrown darts on 14.

This migration is required even if the initial world has few blowguns. Leaving the shared type in
place would make either darts non-melee or blowguns behave as thrown melee weapons.

### 4.2 Ammo pouch contract

Change `ITEM_AMMO_POUCH` admission to accept:

- any `ITEM_MISSILE`; or
- an eligible throwable `ITEM_WEAPON`.

Continue to reject ordinary melee weapons, launchers, nested containers, and other item types.
Capacity remains `value[0]` as an object count; container flags, key, and corpse value slots do
not change.

Once mixed projectile types are legal, launcher lookup must scan for the first missile compatible
with the wielded launcher rather than assuming `ammo_pouch->contains` is usable. A javelin at the
head of a quiver must not make arrows below it invisible, and an arrow must not block a matching
throwable farther down the same pouch.

No player-object schema change is expected: `ITEM_AMMO_POUCH` already persists as a container and
`ITEM_WEAPON` contents already serialize. Add a save/load characterization test with mixed pouch
contents before declaring this assumption proven.

### 4.3 Converted RoL data

Update the converter only after the native runtime contract is tested:

- in `scripts/world/wtool_lib/rol_weapon_mapping.py`, append the mirrored blowgun constant and
  increment its mirrored count;
- map source range type 10 (`Dart`) to the thrown `WEAPON_TYPE_DART`;
- map source range type 16 (`Blowgun`) to `WEAPON_TYPE_BLOWGUN`;
- convert source type-10 thrown dart missiles to `ITEM_WEAPON`/`WEAPON_TYPE_DART`;
- keep source type-16 blowgun darts as `ITEM_MISSILE`/`AMMO_TYPE_DART`;
- convert source throwing quivers to `ITEM_AMMO_POUCH`, removing the temporary plain-container
  diagnostic and updating expected type counts;
- retain source quiver equipment position 23, which already maps to `WEAR_AMMO_POUCH`;
- update drift tests that parse `setweapon()` flags, ranged types, ammo-paired launchers, and
  numeric constants;
- regenerate conversion reports and inspect every affected dart, blowgun, throwing quiver, and
  `MOB_ROL_ARCHER` loadout before applying regenerated world data to development.

Converted `MOB_ROL_ARCHER` NPCs with a throwable anchor should select thrown mode when no usable
launcher mode exists, then use the same source order and depletion behavior as players. Do not
make all NPCs automatically throw merely because they happen to wield a dagger.

Production data or code deployment is outside this implementation task. Validate and apply to the
development world first; production requires the repository's normal explicit release approval.

## 5. Proposed code organization

Keep subsystem membership flat under `src/combat/`:

- Add `src/combat/projectiles.c` and `src/combat/projectiles.h` for eligibility, mode state,
  compatible-pouch scanning, thrown source selection, detachment, and final disposition.
- Update both `Makefile.am` and `CMakeLists.txt` when those files are added.
- Include the header as `"projectiles.h"` from files inside `src/combat/` and as
  `"combat/projectiles.h"` from other subsystems.
- Keep attack-roll and damage resolution in `src/combat/fight.c`; do not move unrelated legacy
  combat code as part of this feature.
- Factor the common `fire`/`throw` target and direction resolution within
  `src/combat/act.offensive.c` so the commands cannot drift on PvP, rooms, wilderness, assists,
  or action use.

Primary files expected to change:

| File | Planned responsibility |
|------|------------------------|
| `src/structs.h` | Append blowgun ID, add thrown attack mode, replace boolean firing state, add non-persistent anchor fields. |
| `src/utils.h` | Projectile-mode and anchor accessors. |
| `src/constants.c` | `ATTACK_TYPE_THROWN` display label and any checked tables. |
| `src/combat/projectiles.[ch]` | New classification, selection, context, and object-lifecycle helpers. |
| `src/combat/assign_wpn_armor.[ch]` | Dart/blowgun profiles, launcher/ammo pairing, readiness helpers, shared slot priority. |
| `src/combat/fight.c` | Context-aware `hit()` path, ranged-rule predicates, thrown damage, mode dispatch, exact object finalization. |
| `src/combat/act.offensive.c` | `do_throw`, shared target resolver, throw-aware `collect`, launcher-only reload/fire calls. |
| `src/act.h`, `src/interpreter.c` | Command declaration and registration. |
| `src/obj/act.item.c` | Ammo-pouch admission and capacity messages. |
| `src/character/perks.c` | Semantic ranged-perk classification and Quick Draw readiness. |
| `src/olc/oedit.c`, `src/act.informative.c` | Builder/player display and value help for blowguns and throwable pouch contents. |
| `src/quest/hunts.c`, `src/act.other.c`, other flag consumers | Audited launcher-versus-thrown classification; change only where semantics require it. |
| `scripts/world/wtool_lib/rol_weapon_mapping.py`, `rol_transform.py` | Converter mappings and removal of the throwing-quiver workaround. |
| `scripts/world/tests/` | Converter constant, inference, type-count, and equipment tests. |

Do not modify `src/vnums.h`, `src/campaign.h`, or `src/mud_options.h` for this work.

## 6. Sequenced implementation plan

### Phase 0: Characterize existing launcher behavior

1. Add production-linked tests around the existing projectile lifecycle before refactoring:
   compatible ammo selection, one-object consumption, hit, miss, Deflect/Snatch, wind wall,
   protection, cross-room landing, reload, and exhaustion.
2. Add a regression proving a native javelin remains melee-usable without `throw`; retain the
   existing javelin profile test.
3. Add a mixed-pouch persistence round trip using current generic object save/load machinery.
4. Record current `fire` messages/action use needed by manual QA.

Checkpoint: all new characterization tests pass without thrown functionality. Any exposed
existing object-loss defect gets a focused regression test before its fix.

### Phase 1: Establish metadata and classification

1. Append `WEAPON_TYPE_BLOWGUN`; split dart and blowgun flags and ammo pairing.
2. Implement null-safe `is_throwable_weapon()` and launcher/thrown/shared attack predicates.
3. Consolidate equipped weapon slot priority so launcher and thrown callers do not disagree.
4. Add the explicit projectile mode and anchor state, including one clear/reset helper.
5. Audit all `WEAPON_FLAG_RANGED`, `WEAPON_TYPE_DART`, `ATTACK_TYPE_RANGED`, and `FIRING(ch)`
   call sites and classify them in the launcher/shared/thrown decision table before editing.

Checkpoint: darts are melee by default; blowguns still pass launcher/ammo readiness; all existing
weapon IDs retain their numeric values.

### Phase 2: Generalize pouch and projectile selection

1. Allow eligible throwable weapons in `ITEM_AMMO_POUCH` while preserving capacity and safety
   checks.
2. Replace top-object assumptions with compatible-item scans for launcher ammunition.
3. Implement and unit-test the exact pouch, inventory, wielded-last source order.
4. Handle customized same-VNUM objects, `NOTHING`, nested containers, transfer restrictions,
   unrelated VNUMs, and mixed pouch contents.
5. Return a context holding the actual selected object; do not alter ownership until attack
   preconditions have passed.

Checkpoint: selection tests prove which exact object would be used, but no player command is yet
enabled.

### Phase 3: Centralize physical projectile lifecycle

1. Refactor launcher `hit()` to use the explicit projectile context and one finalizer.
2. Pass the actual launcher/missile through calculations and procs instead of reading the pouch
   head or global `last_missile` after detachment.
3. Cover every early return after detachment, including wind wall, protection, Deflect/Snatch,
   target death, script extraction, and attack invalidation.
4. Preserve `ITEM_MISSILE` break chance and imbued-arrow behavior only for launcher missiles.
5. Run the full launcher characterization suite before adding thrown dispatch.

Checkpoint: `fire`, `reload`, and `collect` are behaviorally unchanged except that tested
projectile-loss defects are fixed.

### Phase 4: Add `throw` and thrown combat rounds

1. Register and implement `do_throw`, reusing the shared ranged target resolver.
2. Resolve and record the anchor, verify at least one projectile, set thrown mode, and perform the
   opening `ATTACK_TYPE_THROWN` attack.
3. Extend `perform_violence()` to dispatch launcher versus thrown rounds by explicit mode.
4. Apply Dexterity to hit, full Strength to damage, shared ranged defenses/penalties, weapon
   family bonuses from the actual object, and the locked feat split.
5. Consume a fresh copy for every iterative/Rapid Shot/Quick Draw attack, then unequip and throw
   the anchor last.
6. Stop cleanly on exhaustion, invalid anchor, blocked transfer, lost target, or combat end.
7. Ensure same-room attacks enter ongoing combat and adjacent-room attacks match `fire` cleanup
   and action behavior.

Checkpoint: the requested three-source depletion scenario passes as both a unit/integration test
and a manual development playthrough.

### Phase 5: Complete object outcomes, collection, and NPC use

1. Implement Returning with the stated destruction/Snatch precedence and fallback placement.
2. Extend `collect` to recognize owner-tagged throwable `ITEM_WEAPON` objects in the room and
   corpses. Prefer the equipped compatible pouch; if none or full, use top-level inventory within
   carrying limits. Preserve existing missile collection behavior.
3. Add `MOB_ROL_ARCHER` thrown-mode selection without changing unrelated NPC AI.
4. Verify poison, enhancement, critical, scripts, weapon abilities, and artifact hooks use the
   selected copy and survive object invalidation safely.
5. Leave net entanglement, bola trip maneuvers, and new thrown-only feats out of scope unless an
   existing generic weapon flag already supplies that behavior. Basic thrown delivery must not
   invent a second maneuver system.

Checkpoint: every detached object has exactly one final owner/location or one explicit extraction,
including magical and defensive outcomes.

### Phase 6: Update builders, converter, world data, and help

1. Update OEDIT value descriptions and `docs/world_game-data/OEDIT_GUIDE.md` for thrown flags,
   blowguns, and ammo-pouch contents.
2. Update the RoL converter and its tests as specified in Section 4.3.
3. Audit native weapon-type-14 prototypes and regenerate converted development data. Review the
   affected-object report before applying it.
4. Add matching player help to both authoritative stores:
   - `lib/text/help/help.hlp`;
   - idempotent `sql/components/help_thrown_weapons_entries.sql` plus
     `verify_help_thrown_weapons_entries.sql`.
5. Add the help component to `sql/components/ci_schema_manifest.txt` and document its development
   application/verification command.
6. Cover `THROW`, `THROWN-WEAPONS`, `AMMO`, `QUIVERS`, `FIRE`, and `COLLECT`, including source
   priority, wielded-last behavior, recovery, and Returning.
7. Update `docs/systems/COMBAT_SYSTEM.md`, relevant testing documentation, the master technical
   index if needed, and `docs/CHANGELOG.md` only when implementation is complete.

Checkpoint: flat-file and database help agree, SQL verification passes, and converter reports no
unreviewed dart/blowgun/quiver ambiguity.

### Phase 7: Full validation and development release gate

1. Format changed C files with the repository formatter and accept include-comment alignment.
2. Run converter/unit tooling relevant to changed scripts.
3. Run root production-linked tests:

   ```bash
   make test
   make install
   ```

   `make install` is mandatory after `make test`; verify no root-level `luminari` artifact remains.
4. Run focused protocol tests only as part of the repository's normal broad regression sweep; no
   protocol behavior is expected to change.
5. Start local development through `autorun.sh`, not `luminari.service`, and perform the manual
   matrix in Section 8.3.
6. Review compiler warnings, sanitizer/Valgrind findings where practical, SQL help verification,
   generated world diffs, and `git diff --check`.
7. Confirm the final diff does not modify local configuration or credentials and contains no
   unplanned production operation.

Checkpoint: all definition-of-done items pass on development before any production release is
proposed.

## 7. Automated test plan

Prefer a focused production-linked suite such as
`unittests/CuTest/test_thrown_weapons.c`. If added, list it in all three required build locations:

- `cutest_SOURCES` in `Makefile.am`;
- `cutest_test_files` in `Makefile.am`;
- `CUTEST_TEST_SOURCES` in `CMakeLists.txt`.

Tests should use production functions, not a standalone mirror.

### 7.1 Classification and data tests

- Every native thrown profile is eligible and remains non-ranged by default.
- `WEAPON_SPECAB_THROWING` grants eligibility to an otherwise melee-only weapon.
- Returning alone, an invalid weapon index, a non-weapon, and a pure launcher are rejected.
- Dart is thrown-only; blowgun is ranged-only and pairs with `AMMO_TYPE_DART`.
- Established weapon IDs 0 through 79 do not move; blowgun is append-only.
- OLC and converter mirrors fail loudly if numeric constants or flag sets drift.

### 7.2 Selection and pouch tests

- Pouch copy precedes inventory copy; inventory precedes wielded anchor.
- Two or more copies are consumed in stable list order, one per attack.
- An unrelated VNUM and a same-name/different-VNUM object remain untouched.
- A customized same-VNUM object applies its own state and must itself be throwable.
- `NOTHING` permits only the anchor.
- Ordinary carried containers are not searched; other wield slots are not reserves.
- Ammo pouch accepts missiles and throwable weapons, rejects ordinary weapons, and enforces
  capacity.
- A mixed pouch lets arrows, bolts, stones, blowgun darts, and matching thrown weapons skip
  incompatible entries and find their own first compatible object.
- Mixed pouch contents and equipped position survive object save/load.

### 7.3 Command and state tests

- Syntax, missing anchor, missing target, invalid direction, closed exit, peaceful rooms, PvP,
  visibility, morph/wildshape, action availability, and existing-fight gates match `fire` where
  applicable.
- Normal `hit` with a javelin remains melee and does not consume another javelin.
- `throw` sets thrown mode; `fire` sets launcher mode; reload cannot set thrown mode.
- Same-room opening attack continues in thrown mode; cross-room cleanup matches launcher fire.
- Stop fighting, extraction, invalid anchor, slot change, target loss, and exhaustion clear mode
  plus anchor state.
- No readiness failure falls through into a free melee or launcher attack.

### 7.4 Attack and physical-lifecycle tests

- Dexterity, Strength, proficiency, family focus, Point Blank, Precise, Deadly Aim, Rapid Shot,
  Manyshot exclusion, Quick Draw, and iterative penalties follow the locked table.
- Each added attack consumes a distinct copy; too few copies truncate safely.
- The final attack unequips and throws the anchor; its worn slot is empty afterward.
- Enhancement, poison, critical range, scripts, and special abilities come from the selected copy,
  not the anchor or next pouch item.
- Hit, miss, target death, cross-room hit, Deflect, Snatch, wind wall, protection, proc extraction,
  and invalidation each leave exactly one correct disposition.
- A thrown `ITEM_WEAPON` never reads its value 2 as missile break chance.
- Returning re-equips the anchor when possible, otherwise returns to inventory or the attacker's
  room; Snatch and destruction take precedence.
- Fire/reload/archery regression tests remain green after the shared lifecycle refactor.

### 7.5 Converter and NPC tests

- Source range type 10 and its thrown ammunition become throwable darts.
- Source range type 16 and its ammunition become a blowgun plus `AMMO_TYPE_DART` missiles.
- Throwing quivers become equipped ammo pouches and retain appropriate capacity/contents.
- Converted thrown `MOB_ROL_ARCHER` loadouts choose thrown mode; unrelated mobs do not.
- Converter counts, diagnostics, object values, and zone equipment positions match reviewed
  fixtures.

## 8. Manual development QA

### 8.1 Core depletion scenario

Create one throwable prototype and distinct instances of the same VNUM with observable
instance-level differences. Equip one, place two in the equipped ammo pouch, and carry two loose.
Then `throw` a same-room target and verify this exact order:

```text
pouch copy 1 -> pouch copy 2 -> inventory copy 1 -> inventory copy 2 -> wielded copy
```

Verify the wield slot empties only for the last attack, each object lands/embeds exactly once,
and combat stops without a surprise melee attack.

### 8.2 Variants

- Repeat with only inventory reserves, only pouch reserves, and only the wielded anchor.
- Mix arrows and thrown weapons in one pouch; alternate `fire` and `throw` with the appropriate
  wielded weapon.
- Wield two throwable weapons and confirm the documented anchor priority.
- Change/remove the anchor during combat and confirm safe shutdown.
- Test a `WEAPON_SPECAB_THROWING` melee weapon and a Returning weapon.
- Test insufficient carry capacity on Returning and `collect`.
- Test misses, criticals, poison, a target death/corpse, Deflect Arrows, Snatch Arrows, wind wall,
  protection from arrows, peaceful rooms, adjacent directions, and wilderness coordinates.
- Test a converted dart thrower and converted blowgun user.
- Save/logout/reconnect with a mixed equipped pouch, then repeat both commands.

### 8.3 Regression sweep

- Bow with arrows, crossbow with bolts and reload, sling with stones, and blowgun with darts.
- `autofire`, autoreload, Rapid Shot, Manyshot, Epic Manyshot, Quick Draw, `collect`, and ranged
  group assists.
- Melee dagger, spear, javelin, throwing axe, net, and shuriken attacks without `throw`.
- OEDIT creation/display of dart, blowgun, thrown weapons, missiles, and ammo pouches.
- Converted `MOB_ROL_ARCHER` behavior and ordinary NPC melee behavior.

## 9. Risks and controls

| Risk | Control |
|------|---------|
| A detached object is lost or duplicated on an early return | One context owns detachment and one finalizer covers every outcome; assert/test exact disposition. |
| Calculations use the next pouch item | Pass the selected instance explicitly; remove pouch-head and `last_missile` rediscovery. |
| Wielded thrown weapons become unusable in melee | Keep thrown and ranged flags distinct; split dart from blowgun; retain a `do_hit` regression. |
| Appending a type shifts persisted IDs | Append blowgun at 80 and assert all prior constants in tests. Never insert or renumber. |
| Mixed pouches break launchers | Scan for the first compatible object rather than assuming list head compatibility. |
| A stale object pointer survives between combat pulses | Persist only mode, VNUM, and wear slot; resolve objects afresh for every attack. |
| Weapon procs extract or move the projectile | Revalidate lifecycle state after proc boundaries and make finalization idempotent. |
| Rapid/iterative attacks create free projectiles | Resolve and detach one real object for every `hit()` call; stop when unavailable. |
| Converter silently changes many objects | Extend drift tests, produce affected-object reports, review ambiguity, and apply only to development first. |
| Help diverges | Ship matching flat-file and idempotent SQL help plus a verifier in the same change. |

## 10. Definition of done

The feature is complete only when all of the following are true:

- [ ] Wielding any native thrown-only weapon still permits ordinary melee combat.
- [ ] `throw <target> [direction]` works through the existing ranged target and action gates.
- [ ] Matching objects are consumed from equipped ammo pouch, then inventory, then the wielded
      anchor, using strict VNUM identity and one actual instance per attack.
- [ ] The last non-returning anchor is unequipped and thrown; mode stops without melee fallback.
- [ ] Hit, miss, defenses, death, extraction, collection, and Returning have tested, unique object
      ownership outcomes.
- [ ] Dart and blowgun are separate append-only weapon profiles with correct melee/launcher
      behavior and converter mappings.
- [ ] Mixed ammo pouches work for launchers and thrown weapons and survive persistence.
- [ ] Converted throwing quivers and `MOB_ROL_ARCHER` throwers work in the development world.
- [ ] Existing fire, reload, autofire, autoreload, archery feats, and missile collection regressions
      pass.
- [ ] Root `make test` passes, `make install` follows it, and no root `luminari` artifact remains.
- [ ] Converter tests, SQL help verification, manual development QA, and `git diff --check` pass.
- [ ] `lib/text/help/help.hlp`, database help components, OEDIT guide, combat documentation, and
      changelog are updated together.
- [ ] No protected local header, credential file, or production environment was modified.
