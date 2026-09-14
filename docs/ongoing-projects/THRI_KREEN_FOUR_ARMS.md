# Thri-Kreen four-arm wielding: Duris study and LuminariMUD mapping

Status: design study for issue #168, written 2026-09-14. The extra-attack
stand-in first proposed for this issue was rejected; the target is the full
mechanic: real weapon slots, real doubled limb slots, real extra swings, and
a save format that carries them. Duris source verified at
`/home/aiwithapex/projects/duris`; our side traced in `src/structs.h`,
`src/obj/act.item.c`, `src/obj/objsave.c`, `src/handler.c`,
`src/combat/fight.c`, `src/character/race.c`, `src/constants.c`,
`src/act.informative.c`, `src/players.c`, and `src/db.c`.

Companion references: the Duris racial mechanics gap list and race conversion
study (revision-pinned links in issue #168), race point budgets in
`docs/guides/PLAYER_RACES_REFERENCE.md`, save format in
`docs/systems/SAVE_SYSTEMS_BREAKDOWN.md`.

## Part 1: how Duris does it

### The race

Duris Thri-Kreen (`RACE_THRIKREEN`, mob race code `TK`) are seven-foot
insectoid nomads with four arms. Their help entry (`help/duris_help_parsed.hlp`)
promises: wield up to four weapons at once, "including dual two-handed weapons
or archery combinations"; four wrist items, two sets of sleeves, two sets of
gloves; no body armor, footwear, finger rings, or earrings.

| Duris element | Where | Value |
|---------------|-------|-------|
| Stats (Str/Agi/Dex/Con/Pow/Int/Wis/Cha/Luck) | help entry | 115/130/125/105/70/65/65/75/90 |
| Innates | `src/classes/innates.c:657` | Dayvision 1, Ultravision 1, Bite 11, Leap 21, Vulnerable to Cold 1 |
| Cold damage taken | `src/combat/dam_mods.c:336` | -0.3 (they take 30 percent less cold damage; the innate adds the vulnerability and paralysis save on top) |
| Lost slots | `src/cmd/actinf.c` `has_eq_slot()` | finger (both), body, feet, ear (both) |
| Cannot ride | `src/classes/mount.c:171` | "You cannot ride." |
| Leap dodge | `src/combat/fight.c` `leapSucceed()` | agility/7 percent, bounded 1 to 20, level difference adjusts |
| Epic skill deny | `src/classes/epic_skills.c:229` | cannot learn Devastating Critical |
| Creation side | `src/core/constant.c:1606` | neutral: player picks a racewar side |
| Combat pulse | help entry | "very bad" (slowest attack round in the game) |

### The four-hand predicate

```
#define HAS_FOUR_HANDS(ch) \
    ((GET_RACE(ch) == RACE_THRIKREEN) || (IS_AFFECTED3((ch), AFF3_FOUR_ARMS)))
```
(`src/core/utils.h:945`). `AFF3_FOUR_ARMS` is an equipment affect flag that
item enhancement (`src/item/enhance.c:1381`) and auction search
(`src/economy/auction_houses.c:3566`, "that grant the wearer four arms to
fight with") expose, so any race can gain the mechanic from gear. Every
consumer tests the predicate, never the race, except the slot denials above.

### Wear positions

`src/core/defines.h`:

| Position | Number | Note |
|----------|--------|------|
| `PRIMARY_WEAPON` / `WIELD` | 16 | |
| `SECONDARY_WEAPON` / `WIELD2` | 17 | |
| `HOLD` | 18 | one held slot for everyone |
| `THIRD_WEAPON` / `WIELD3` | 25 | four-hand only |
| `FOURTH_WEAPON` / `WIELD4` | 26 | four-hand only |
| `WEAR_ARMS_2` | 31 | "worn on lower arms" |
| `WEAR_HANDS_2` | 32 | "worn on lower hands" |
| `WEAR_WRIST_LR`, `WEAR_WRIST_LL` | 33, 34 | "worn on lower wrist" |

Display strings are in `where[]` (`src/core/common.c:341`): `<third weapon>`,
`<fourth weapon>`, `<worn on lower arms>`, `<worn on lower hands>`,
`<worn on lower wrist>`. `has_eq_slot()` (`src/cmd/actinf.c:8334`) returns
false for all seven positions unless `HAS_FOUR_HANDS()`.

### Hand accounting and placement (`src/cmd/actobj.c`)

`get_numb_free_hands()` (line 6989): capacity is 2, plus 2 for four hands.
Each of HOLD, SHIELD, WIELD, WIELD2, WIELD3, WIELD4 subtracts
`wield_item_size()`, which is 2 for a two-handed weapon or a `ITEM_TWOHANDS`
object (1 for giants), else 1. So a four-handed character can carry two
two-handers, or a shield plus three one-handers, or a held item plus three
weapons, and so on.

`free_hand_slot()` (line 7022) picks the slot. With four hands it walks the
pairs (PRIMARY, SECONDARY) then (THIRD, FOURTH): a one-hander fills the first
empty primary of a pair, or the pair's secondary if the primary holds a
one-hander; a two-hander takes a primary whose secondary is empty. Without
four hands, only the first pair is considered. Held implements may also sit in
weapon slots (they are storage roles, not anatomy).

The wield case (line 7870) then applies the training gate. The whole
dual-wield block (`SKILL_DUAL_WIELD` requirement, and the off-hand weight rule
"weight x3 must not exceed the strength wield limit") is skipped for four-hand
characters: `if (!HAS_FOUR_HANDS(ch)) { ... }`. Thri-Kreen never need the
dual wield skill to fill four slots and never hit the off-hand weight cap.

Wearing (same file): hands (case 7), arms (case 8), and wrists (case 11) each
branch on `HAS_FOUR_HANDS()` to fill the second set, with messages "You can't
wear any more on your hands/arms" and "You already wear something around all
your wrists".

### The attack round (`src/combat/fight.c` `calculate_attacks()`, line 9117)

Duris builds an array of weapon slots to swing this round with
`ADD_ATTACK(slot)`. For the non-monk path:

| Trigger | Base swing | Four-hand mirror | Chance |
|---------|------------|------------------|--------|
| Every round (unless slowed) | PRIMARY | THIRD | dual wield skill / 2 + 50 percent |
| Dual wield roll succeeds | SECONDARY | FOURTH | same |
| Improved two-weapon roll | SECONDARY | FOURTH | same |
| Double attack | PRIMARY | THIRD, and FOURTH | skill/2 + 50, skill/2 + 45 |
| Triple attack | PRIMARY | THIRD, and SECONDARY | skill/2 + 45 each |
| Quadruple attack | PRIMARY | THIRD | skill/2 + 45 |
| Haste | PRIMARY | THIRD | skill/2 + 50 |

So at zero dual wield skill each mirror fires half the time; at 100 skill the
third weapon swings with every primary swing and the fourth with every
secondary swing. A four-armed warrior therefore roughly doubles the weapon
attacks of a two-armed one. Duris has no off-hand strength or hit penalty tied
to a slot: `pv_common()` receives only the weapon, so the third weapon is as
good as the first and the fourth as the second.

Other combat consumers: `proccing_slots[]` (defensive procs check all four
weapon slots and the doubled slots), parry requires a weapon in any of the
four slots (line 8793), riposte picks the fourth weapon 1 in 5, then the third
1 in 4, then the secondary 1 in 3 (line 4152), `mangle` and
`critical_disarm()` can strip any of the four, thrown-weapon lookup and count
(`src/combat/range.c`) scan all four, the two-weapon hit/dam bonus
(`src/classes/epic_skills.c:1195`) counts a weapon in any of the three extra
slots, and object specials that must be wielded accept any weapon slot
(`src/specs/specs.object.c`).

### Persistence

Duris does not save the slot number as a stable equipment position. Rent
files write the equipment array index, and `restore_wear[MAX_WEAR]`
(`src/core/files.c:4026`) maps THIRD_WEAPON and FOURTH_WEAPON to wear keyword
12 (wield). On load `wear(ch, obj, 12)` is called, which re-runs
`free_hand_slot()`; so a saved third weapon lands in whichever weapon slot is
free at restore time. The doubled limb slots map to their normal keywords
(7 hands, 8 arms, 11 wrist) and refill the second set the same way. This is
also why a Thri-Kreen who loses four hands (item removed) simply fails to
re-wear the extras on next login.

### Mobs

`empty_slot_for_weapon()` (`src/mob/mobact.c:10521`) has a four-hand branch
that never returns WIELD3 or WIELD4 (the branch is broken: it returns WIELD2
whether or not it is occupied). Mobs with four arms therefore only get four
weapons when the zone loads them directly. There is nothing to copy here.

## Part 2: what LuminariMUD has today

### Wear positions and hands

`src/structs.h:1738` defines 44 positions (`NUM_WEARS 44`). Hands are modeled
as six slots: `WEAR_WIELD_1` 16, `WEAR_HOLD_1` 17, `WEAR_WIELD_OFFHAND` 18,
`WEAR_HOLD_2` 19, `WEAR_WIELD_2H` 20, `WEAR_HOLD_2H` 21, plus `WEAR_SHIELD`
11. Positions 28 to 31 and 42 are marked "currently unused; reserved for
compatibility" but each already has a wear flag, a keyword, and a display
string, so they are not free numbers; new positions append at 44.

`hands_have()` (`src/obj/act.item.c:4193`) is already the extension point:
a `switch (GET_RACE(ch))` with only a default of 2, then +1 for the alchemist
Vestigial Arm discovery. `hands_used()` counts one per wield/hold/shield slot,
two per 2H slot, and an extra one for a two-handed ranged weapon.
`hands_needed_full()` decides one or two hands from object size versus
character size (Monkey Grip and Powerful Build reduce it) or the
`ITEM_ROL_TWO_HANDED` flag.

`perform_wear_impl()` places hand gear: a two-hander asked for `WEAR_WIELD_1`
is redirected to `WEAR_WIELD_2H`; a one-hander asked for an occupied
`WEAR_WIELD_1` or `WEAR_HOLD_1` goes to `where + 2` (the offhand pair). The
paired slots (finger, neck, wrist, ear, ankle) use the "if right is taken,
`where++`" rule, which relies on the left slot being the next number.

`character_wear_slot_restriction()` (`src/character/race.c:144`) is the
anatomy gate: the tail slot, the Leonine Frame feat, and the per-race
`wear_slot_restrictions[NUM_WEARS]` messages set by
`set_race_wear_restriction()`. `equip_char()` refuses a slot the character
cannot use and drops the item to inventory, so zone `E` commands and loads
cannot bypass it. NPCs currently bypass the race table (the function returns
NULL for `IS_NPC()` before the lookup) but not the feat checks above it.

### Attack routine

`perform_attacks()` (`src/combat/fight.c:15847`) counts attacks first
(`bonus_mainhand_attacks` from BAB, flurry, haste, and so on) then executes.
`dual = is_dual_wielding(ch)`: true with an offhand weapon, a double weapon in
the 2H slot, or the Trelux race. When dual, the base is one
`ATTACK_TYPE_PRIMARY` hit and one `ATTACK_TYPE_OFFHAND` hit; then the
main-hand bonus attacks; then, still under `dual`, extra offhand hits for
Improved (-5), Greater (-10), and Perfect (0) Two-Weapon Fighting, and the
Wilderness Warrior perks. Phase split is `attack_number_runs_in_phase()`.

Weapon lookup is by attack type: `get_wielded()` (line 10545) maps PRIMARY to
`WEAR_WIELD_1` then `WEAR_WIELD_2H`, OFFHAND to `WEAR_WIELD_OFFHAND` or the
double weapon. `compute_attack_bonus()` applies
`dual_wielding_penalty(ch, offhand)` (-6/-10 base, -4/-8 light weapon, -2 with
Two-Weapon Fighting); `compute_damage_bonus()` gives the offhand half strength
and the 2H slot one and a half. The `attacks` command runs the same routine in
display mode.

### Persistence

Player object files (`lib/plrobjs/*/name.objs`) write each object with
`Loc : <n>` where `n` is wear position + 1 (0 inventory, negative bag depth),
via `Crash_save(obj, ch, fp, j + 1)`. Loading rejects `Loc` outside
`[-MAX_BAG_ROWS, NUM_WEARS]` as malformed (`src/obj/objsave.c:4224`), and
`auto_equip()` (line 594) re-validates the object's wear flag for the saved
slot, dropping to inventory on mismatch. The same record format serves the
optional MySQL object backup (`OBJSAVE_DB`), house files, pet equipment
(`Crash_save_pet()`, `players.c` pet hashing over `NUM_WEARS`), the sheathed
weapon table (`player_save_objs_sheathed.sheathed_position`), and copyover.
`object_database_wear_slots` stores item wear flags, not positions. Zone
files carry the position in `E` commands; `zedit` lists `equipment_types[]`
and bounds on `NUM_WEARS`. Player files store no equipment positions
themselves.

Everything that iterates equipment does `for (j = 0; j < NUM_WEARS; j++)` and
scales automatically. The fixed tables that must grow are listed in Part 4.

### The nearest race: Trelux

Trelux (ID 9, epic, 30000) is the existing insectoid: Ultravision, Vital,
Hardy, Vulnerable To Cold, Exoskeleton, Leap, Wings, Trelux Eq, Pincers,
Insectbeing; no finger, hands, shield, wield, hold, leg, or foot slots.
Thri-Kreen is the mirror image: the same insect body but with hands, and the
whole race is what the hands can do.

## Part 3: the mapping

### Design rules

1. The mechanic is feat-gated, never race-gated, like every other Duris
   conversion: `FEAT_FOUR_ARMS`, innate, `in_game`, not learnable, not
   stackable. Items can grant it through the existing `APPLY_FEAT` path that
   `get_feat_value()` already scans on worn gear, which reproduces
   `AFF3_FOUR_ARMS` with no new affect flag.
2. Slots are real equipment positions saved by number, not re-derived on
   load. Our `Loc` format already does this; Duris's re-wear trick is not
   copied.
3. The two extra weapon slots form a second pair that mirrors the first pair
   in every rule (placement, penalties, phase), so the combat code treats
   "hand pair" as the unit and the existing two-weapon feats keep meaning.
4. Nothing changes for a two-armed character: every new branch is behind
   `has_four_arms(ch)` and every new slot is refused by the anatomy gate.

### New wear positions (append; `NUM_WEARS` 44 to 51)

| Constant | Number | Wear flag | Display (`wear_where`) | `equipment_types` |
|----------|--------|-----------|------------------------|-------------------|
| `WEAR_WIELD_3` | 44 | `ITEM_WEAR_WIELD` | `{Wielded Third}` | Wielded in third hand |
| `WEAR_WIELD_4` | 45 | `ITEM_WEAR_WIELD` | `{Wielded Fourth}` | Wielded in fourth hand |
| `WEAR_WIELD_2H_2` | 46 | `ITEM_WEAR_WIELD` | `{Wielded Twohanded 2}` | Wielded two-handed, second pair |
| `WEAR_ARMS_2` | 47 | `ITEM_WEAR_ARMS` | `{Worn On Lower Arms}` | Worn on lower arms |
| `WEAR_HANDS_2` | 48 | `ITEM_WEAR_HANDS` | `{Worn On Lower Hands}` | Worn on lower hands |
| `WEAR_WRIST_R2` | 49 | `ITEM_WEAR_WRIST` | `{Worn Around Wrist}` | Worn around lower right wrist |
| `WEAR_WRIST_L2` | 50 | `ITEM_WEAR_WRIST` | `{Worn Around Wrist}` | Worn around lower left wrist |

No new item wear flags: existing WIELD, ARMS, HANDS, and WRIST flags cover
the new positions, so no object file, OLC, or `object_database_wear_slots`
change. `WEAR_WRIST_R2` and `WEAR_WRIST_L2` are adjacent so the `where++`
pairing rule works unchanged. `WEAR_WIELD_2H_2` keeps the "two-hander lives in
a 2H slot" invariant that `compute_damage_bonus()`, `is_using_double_weapon()`
and `hands_used()` rely on; Duris's "dual two-handed weapons" then works
without special-casing weapon size in the wield slots. Held items do not get
a third pair: `WEAR_HOLD_1`/`WEAR_HOLD_2` already allow two held items and
Duris itself has one HOLD slot. A four-armed character can still wield two
and hold two, or wield three and carry a shield, because the hand budget, not
the slot count, is the limit.

### Predicate and hand budget

```
bool has_four_arms(const struct char_data *ch);   /* HAS_FEAT(ch, FEAT_FOUR_ARMS) > 0 */
```
in `src/utils.c` beside the other Duris helpers. `hands_have()` adds 2 when it
holds (the Vestigial Arm discovery still stacks; a five-handed alchemist
Thri-Kreen has one spare hand for a shield, which is the same rule as today
for a three-handed one). `hands_used()` adds one each for `WEAR_WIELD_3` and
`WEAR_WIELD_4` (plus one for a two-handed ranged weapon there) and two for
`WEAR_WIELD_2H_2`.

`character_wear_slot_restriction()` gains, before the NPC early return so
mobs are covered too:

```
if (wear_slot >= WEAR_WIELD_3 && wear_slot <= WEAR_WRIST_L2 && !has_four_arms(ch))
  return "You do not have enough arms for that.";
```

This one check makes `equip_char()`, zone `E` commands, `auto_equip()` on
load, and every wear command refuse the seven slots for everyone else.

### Placement

`perform_wear_impl()` hand juggling, four-arm branch only:

- A one-hander asked for `WEAR_WIELD_1`: try `WEAR_WIELD_1`, then
  `WEAR_WIELD_OFFHAND`, then `WEAR_WIELD_3`, then `WEAR_WIELD_4` (the first
  free slot in pair order, matching Duris `free_hand_slot()`).
- A two-hander asked for `WEAR_WIELD_1`: `WEAR_WIELD_2H`, then
  `WEAR_WIELD_2H_2`. A 2H slot may only be filled when its pair's one-hand
  slots are empty (pair 1: WIELD_1/OFFHAND/HOLD_1/HOLD_2; pair 2:
  WIELD_3/WIELD_4). The hand budget already refuses the impossible cases; the
  pair rule keeps the display honest.
- `WEAR_ARMS`, `WEAR_HANDS`: if occupied, `WEAR_ARMS_2` / `WEAR_HANDS_2`.
- `WEAR_WRIST_R`: if both first-pair wrists are taken, `WEAR_WRIST_R2`, then
  the `where++` rule reaches `WEAR_WRIST_L2`.
- New `already_wearing[]` and `find_eq_pos()` keyword rows ("arms", "hands",
  "wrist" reuse the same keyword; the three weapon rows are `!RESERVED!` like
  the existing wield rows).
- `wear_message()` needs the seven new message pairs.

The dual-wield gates in `do_wield()` (no second ranged weapon, no mixing
ranged with melee) stay as they are; Duris's "archery combinations" are not
carried over because our ranged routine is a separate path that returns before
melee, and a bow already needs two hands in `hands_used()`. Note this as a
deliberate deviation. The off-hand weight and training exemption Duris grants
four-handers is not copied either: our dual-wield penalty is a to-hit penalty,
not a gate, and it applies to the second pair the same way (see combat).

### Removal and loss of the feat

`perform_remove()` is unchanged for the new slots. When the feat leaves (an
item granting it is removed, or a wildshape ends), `affect_total()` is the
natural hook: after recomputing, if `hands_used(ch) > hands_have(ch)` or a
four-arm slot is filled while `!has_four_arms(ch)`, force-remove
`WEAR_WIELD_2H_2`, `WEAR_WIELD_4`, `WEAR_WIELD_3`, `WEAR_WRIST_L2`,
`WEAR_WRIST_R2`, `WEAR_HANDS_2`, `WEAR_ARMS_2` in that order with the
"Your extra arms fade and you drop..." messages. The same helper runs after
`auto_equip()` on load, so a character whose four-arm item was deleted from
the world loads clean instead of tripping the `hands_available()` SYSERR.

### Combat

Add `ATTACK_TYPE_THIRD 23` and `ATTACK_TYPE_FOURTH 24` (after
`ATTACK_TYPE_THROWN 22`) so every existing `switch (attack_type)` keeps its
meaning and the new cases are explicit.

| Function | THIRD | FOURTH |
|----------|-------|--------|
| `get_wielded()` | `WEAR_WIELD_3`, else `WEAR_WIELD_2H_2` | `WEAR_WIELD_4`, else the double weapon in `WEAR_WIELD_2H_2` |
| `compute_attack_bonus()` | as PRIMARY, with `dual_wielding_penalty(ch, FALSE)` when the pair is dual | as OFFHAND, `dual_wielding_penalty(ch, TRUE)` |
| `compute_damage_bonus()` | full strength (as PRIMARY); 1.5x in `WEAR_WIELD_2H_2` | half strength (as OFFHAND) |
| `dual_wielding_penalty()` | reads the pair's weapons | reads the pair's weapons |
| sneak, finesse, weapon focus, mastery, procs | same paths as PRIMARY/OFFHAND (they key on the weapon object) | same |

`is_dual_wielding()` stays the first-pair test. Add
`is_dual_wielding_second_pair(ch)` (a weapon in `WEAR_WIELD_4`, or a double
weapon in `WEAR_WIELD_2H_2`).

In `perform_attacks()`, after the first-pair execution and before the
evolution attacks, when `has_four_arms(ch)` and a weapon sits in the second
pair:

- One `ATTACK_TYPE_THIRD` hit mirrors the base primary hit.
- If the second pair is dual, one `ATTACK_TYPE_FOURTH` hit mirrors the base
  offhand hit.
- Each main-hand bonus attack (`bonus_mainhand_attacks`, including haste)
  is mirrored by a THIRD attack.
- Each extra offhand attack from Improved, Greater, and Perfect Two-Weapon
  Fighting is mirrored by a FOURTH attack at the same penalty.

Every mirror is rolled: it fires on `rand_number(1, 100) <= four_arm_mirror_chance(ch)`
where the chance is 50 percent, +25 with `FEAT_TWO_WEAPON_FIGHTING`, +25 with
`FEAT_IMPROVED_TWO_WEAPON_FIGHTING` (100 percent). This is Duris's
"dual wield skill / 2 + 50" expressed in our feat ladder: an untrained
Thri-Kreen swings the extra arms half the time; a trained one always. Display
mode (`attacks` command) prints "Third hand" and "Fourth hand" rows with the
same chance note, and `RETURN_NUM_ATTACKS` counts the mirrors at their
expected value (the mirrors are rolled per swing only in the normal routine,
as the Air Embodiment proc already does with `mode != 2`).

Phase assignment uses `attack_number_runs_in_phase()` like every other bonus
attack, so the extra swings spread across the round instead of stacking in
phase 1. Vital Strike zeroes the mirrors with the bonus attacks it already
zeroes. Ranged and eldritch blast paths return before this block and never see
it.

Other consumers to extend, all keyed on "is there a weapon in any wield
slot" and currently listing the three slots by hand: `has_speed_weapon()`,
weapon-proficiency and armor scans in `src/combat/assign_wpn_armor.c`
(these loop `NUM_WEARS` and need nothing), disarm targets, the
`is_wielding_type()` melee/ranged mix check, `two_weapon` display in `score`,
and the corpse transfer (loops, nothing needed).

### Persistence

- `NUM_WEARS` becomes 51. `Loc` values 45 to 51 appear in `.objs`, house,
  pet, and copyover records. Existing files are untouched: no old position
  moves, and nothing is renumbered.
- `auto_equip()` gets the seven cases (WIELD flag for the three weapon slots,
  ARMS, HANDS, WRIST for the rest). `character_wear_slot_restriction()` then
  drops a four-arm slot to inventory when the character no longer qualifies,
  the same path a Trelux takes for a saved glove.
- Compatibility direction: a new binary reads every old file. An old binary
  reading a new file with `Loc` 45 or above treats the record as malformed
  and aborts that object load (`goto malformed`), so a downgrade after
  release must be preceded by removing four-arm gear; document this in
  `docs/systems/SAVE_SYSTEMS_BREAKDOWN.md` under the `.objs` format.
- The pet hash in `players.c` iterates `NUM_WEARS`, so pet fingerprints
  change only for pets with four-arm gear (none exist today).
- `scripts/world/wtool_constants.json` must be resynced (`NUM_WEARS` and
  `NUM_FEATS` both move).

### Race data for a Thri-Kreen defined as data

Everything below is `add_race()` data plus feat grants; no race constant is
needed by any mechanic.

| Duris | Ours |
|-------|------|
| Stats 115/130/125/105/70/65/65/75/90 | Study conversion +2/+1/-4/-4/+3/-3 (Str/Con/Int/Wis/Dex/Cha) |
| Size Medium | `SIZE_MEDIUM` |
| Four arms, four wrists, two arms, two hands | `FEAT_FOUR_ARMS` at level 1 |
| Ultravision | `FEAT_ULTRAVISION` |
| Dayvision | dropped (no implementation in Duris) |
| Bite at 11 (paralysing venom) | `FEAT_POISON_BITE` (feat 59) is the existing venomous bite; grant it at gate 6 (11 rescaled by 30/56). A paralysis rider is a follow-up |
| Leap at 21 | `FEAT_LEAP` at gate 11 |
| Cold damage -30 percent taken | `FEAT_VULNERABLE_TO_COLD` (-20) already exists; a -30 protection rank is a follow-up like the Weakness To Fire note in the gap list |
| Vulnerable to cold, paralysis save | `FEAT_VULNERABLE_TO_COLD` |
| No body, feet, finger, ear | `set_race_wear_restriction()` for `WEAR_BODY`, `WEAR_FEET`, `WEAR_FINGER_R/L`, `WEAR_EAR_R/L` |
| Cannot ride | `FEAT_QUADRUPED_BODY` refuses mounts but also resists knockdown; a mount-only refusal needs a new `set_race_...` flag or a small feat |
| Devastating Critical deny | not applicable (no such epic skill) |
| Very bad combat pulse | not expressible (rounds are fixed); the study folds it into the melee factor |

### Race point price

`docs/guides/PLAYER_RACES_REFERENCE.md` rows to add:

| Trait | Example | RP |
|-------|---------|----|
| Four arms: two extra weapon slots, doubled wrist, sleeve and glove slots, mirrored attacks | Four Arms | 8 |

Thri-Kreen under our table: ability 2 (6 positive, 11 penalty capped at 4),
size 0, traits 1 (Ultravision) + 8 (Four Arms) + 2 (bite) + 5 (Leap) = 16,
drawbacks -1 (Vulnerable To Cold) - 6 (six lost slots, at the cap) = -7,
total 11. That sits at the bottom of the Advanced band (12 to 16) but the
single 8-point trait breaks composition rule 2 (no trait over 30 percent of
budget) for Advanced (4.2) and only just for Epic (7.2). Recommendation:
register it as an Epic race at 30000 like Trelux, its body-double, with the
mirrored attacks as the level-scaling trait rule 4 asks for. The tier is a
data choice and can move.

## Part 4: change inventory

| Area | Files | Change |
|------|-------|--------|
| Constants | `src/structs.h` | seven `WEAR_*` positions, `NUM_WEARS 51`, `FEAT_FOUR_ARMS`, `ATTACK_TYPE_THIRD/FOURTH` |
| Tables sized `NUM_WEARS` | `src/constants.c` (`wear_where`, `equipment_types`), `src/act.informative.c` (`eq_ordering_1`), `src/obj/act.item.c` (`wear_bitvectors`, `already_wearing`, `find_eq_pos` keywords, `wear_message`) | seven rows each; `CHECK_TABLE_SIZE` enforces it at compile time |
| Feat | `src/character/feats.c` | `feato(FEAT_FOUR_ARMS, ...)` in the Duris block |
| Predicate and budget | `src/utils.c`, `src/obj/act.item.c` | `has_four_arms()`, `hands_have()`, `hands_used()`, placement branch, forced-removal helper |
| Anatomy gate | `src/character/race.c` | four-arm clause in `character_wear_slot_restriction()` |
| Combat | `src/combat/fight.c` | `get_wielded()`, `compute_attack_bonus()`, `compute_damage_bonus()`, `dual_wielding_penalty()`, `is_dual_wielding_second_pair()`, mirror block in `perform_attacks()`, display rows |
| Persistence | `src/obj/objsave.c` | `auto_equip()` cases; load-time cleanup hook |
| Docs | `docs/systems/SAVE_SYSTEMS_BREAKDOWN.md`, `docs/guides/PLAYER_RACES_REFERENCE.md`, `docs/systems/GAME_MECHANICS_SYSTEMS.md` | format note, price row, mechanic note |
| Help | `lib/text/help/help.hlp`, `sql/components/help_duris_racial_innate_entries.sql` | `FOUR-ARMS` entry |
| Constants sync | `scripts/world/wtool_constants.json` | `NUM_WEARS`, `NUM_FEATS` |
| Tests | `unittests/CuTest/test_racial_innate_feats.c` (or a new `test_four_arms.c` in both build lists) | see below |

### Tests the issue asks for

- Equip: with the feat, a fourth one-hander lands in `WEAR_WIELD_4` and a
  second two-hander in `WEAR_WIELD_2H_2`; hands and arms fill the second set;
  the fifth wrist item is refused with the wrist message. Without the feat,
  `WEAR_WIELD_3` is refused by `character_wear_slot_restriction()` and
  `equip_char()` drops the object to inventory.
- Remove: removing the first-pair weapon leaves the second pair in place;
  clearing the feat force-removes the seven slots in order and leaves the
  first pair alone.
- Extraction: `extract_char()` and the corpse transfer move all seven slots
  (loop over `NUM_WEARS`), verified by counting corpse contents.
- Persistence: a round trip through `objsave_save_obj_record()` and the load
  parser keeps `Loc` 45 to 51 and `auto_equip()` restores each slot; the same
  record loaded for a character without the feat lands in inventory; a `Loc`
  of 44 (the old upper bound) still loads.
- Attacks: `perform_attacks(RETURN_NUM_ATTACKS)` with a third weapon adds the
  mirrored primary count, with a fourth adds the mirrored offhand count, and
  the ranged count with a bow is unchanged; `get_wielded()` returns the right
  object for the two new attack types; `compute_damage_bonus()` gives THIRD
  full strength and FOURTH half.

### Deviations from Duris, recorded

- Mirrors are feat-scaled (50/75/100 percent) rather than skill-scaled.
- The second pair carries our dual-wield to-hit penalties and half offhand
  strength; Duris has neither. Duris compensates with the slowest combat pulse
  in the game, which we cannot express.
- No ranged weapon in the extra hands; no third held item.
- Held items never occupy weapon slots.
- Slots are saved by number and restored in place, not re-wielded on load.
- The item route to four arms is an `APPLY_FEAT` affect, not a new affect flag.
