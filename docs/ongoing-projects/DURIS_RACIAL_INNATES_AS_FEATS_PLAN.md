# Duris Racial Innates as Feats: Implementation Plan

Status: in progress. Created 2026-09-12; Phase 0 landed 2026-09-12.
See "Progress log" at the end for what is done and what a new session
should pick up next.
Companion study: [DURIS_RACE_CONVERSION.md](DURIS_RACE_CONVERSION.md).
Duris source verified at `/home/aiwithapex/projects/duris` (`src/classes/innates.c`
registration list and the implementation sites named per feat below). Our side
traced in `src/character/feats.c`, `src/character/race.c`, `src/structs.h`,
`src/utils.c`, `src/combat/fight.c`, `src/combat/act.offensive.c`,
`src/act.other.c`, `src/limits.c`, `src/magic/magic.c`, `src/obj/act.item.c`,
`src/mud_event.h`, `src/mud_event.c`, and `src/interpreter.c`.

## Goal and scope

Implement every Duris player-race innate that LuminariMUD does not already
cover as a feat that:

- is registered with `feato()` as `in_game = TRUE`, `can_learn = FALSE`,
  `FEAT_TYPE_INNATE_ABILITY`, so it shows in `feat info` and `race feats` but
  can never be picked at level-up or bought;
- has its mechanic implemented end to end and gated on `HAS_FEAT()`, never on
  `GET_RACE()`, so it can be granted to any race later with one
  `feat_race_assignment()` line;
- is not granted to any race by this work. Assignment is a separate, later
  decision per race. The only `assign_races()` changes are the two
  behaviour-preserving lines named in bucket B (crystal dwarf stability,
  half-troll bodyslam), which exist so that swapping a race check for a feat
  check leaves every existing race exactly as it was.

"Player-race innate" means every `ADD_RACIAL_INNATE()` line in Duris
`src/classes/innates.c` for the 37 player races listed in the companion study
(`RACE_HUMAN` through `RACE_TIEFLING`), including the lines that are
commented out there. Those innates (Dayblind, Miner, Calming, Barter, Flurry,
Barbarian Breath, Fireball, Mass Dispel, Webwrap) still have working code in
Duris; only their race registrations were removed, so they are converted like
the rest. NPC-only innates (elemental body, weapon immunity, fire aura, and
the like) are not in scope.

## Coverage matrix

Every player-race innate falls into one of three buckets.

### A. Covered today by a wired feat (no work)

| Duris innate | Our feat | Note |
|--------------|----------|------|
| Infravision | `FEAT_INFRAVISION` | |
| Ultravision | `FEAT_ULTRAVISION` | |
| Levitate | `FEAT_SLA_LEVITATE` | 3/day, `levitate` |
| Faerie Fire | `FEAT_SLA_FAERIE_FIRE` | 3/day, `faeriefire` |
| Darkness (Shade), Globe of Darkness (Drow) | `FEAT_SLA_DARKNESS` | 3/day, `darkness` |
| Enlarge | `FEAT_SLA_ENLARGE` | 3/day, `enlarge` |
| Innate Strength | `FEAT_SLA_STRENGTH` | 3/day, `strength` |
| Underdark Invisibility | `FEAT_SLA_INVIS` | Duris restricts to underdark rooms; ours does not. Accept |
| Fly | `FEAT_WINGS` | `fly` and `land` |
| Protection From Fire | `FEAT_TIEFLING_HELLISH_RESISTANCE` | DR 10 vs fire |
| Protection From Cold | `FEAT_MOUNTAIN_BORN` | 50 percent cold resistance |
| Magic Resistance (shrug) | `FEAT_HALF_DROW_SPELL_RESISTANCE` (SR 5 + half level), `FEAT_DROW_SPELL_RESISTANCE` (SR 10 + level) | Pick by shrug value per the study's conversion rule. Shrug 50 and above maps to `FEAT_LICH_SPELL_RESIST` once it is wired (bucket B); `FEAT_FAE_RESISTANCE` bundles DR 10 and is not a clean fit |
| Regeneration | `FEAT_TROLL_REGENERATION` | 3 hp per tick, 6 in combat. See B for the rank note |
| Innate Hide | `FEAT_NATURALLY_STEALTHY` | |
| Disappear | `FEAT_ONE_WITH_SHADOW` | Hide during a fight |
| Shade Movement | `FEAT_ONE_WITH_SHADOW` plus `FEAT_PRACTICED_SNEAK` | |
| Perception | `FEAT_KEEN_SENSES` | Duris adds a track bonus and map radius; ours is +2 perception |
| Gambler's Luck | `FEAT_LUCKY` | No luck stat here |
| Vampiric Touch | `FEAT_VAMPIRE_BLOOD_DRAIN`, `FEAT_VAMPIRE_ENERGY_DRAIN` | |
| Bite | `FEAT_POISON_BITE` (venom), `FEAT_DRACONIAN_BITE` (plain) | |
| Gaze | `FEAT_VAMPIRE_DOMINATE` | |
| Phantasmal Form | `FEAT_VAMPIRE_GASEOUS_FORM` | |
| Call Of The Grave | `FEAT_SUMMON_UNDEAD` | Animate dead at will |
| Charge | none needed | `charge` is a general command for every fighter |
| Dayblind | `FEAT_LIGHT_BLINDNESS` | Milder than Duris; only Half-Illithid has Dayblind wired there |

### B. Feat exists but is unwired, race-keyed, or misnamed (wire it)

These feats are registered in `feats.c` but the mechanic either checks
`GET_RACE()` or does not exist. Assigning the feat to a new race today would
do nothing. Each item replaces a race check with `HAS_FEAT()`; the existing
races keep the feat through their current `feat_race_assignment()` lines, so
behaviour does not change for them.

| Duris innate | Our feat | Current state | Work |
|--------------|----------|---------------|------|
| Vulnerable To Fire | `FEAT_WEAKNESS_TO_FIRE` | `compute_damtype_reduction()` in `src/combat/fight.c` checks `RACE_HALF_TROLL` (-50) | Check the feat instead |
| Vulnerable To Cold | `FEAT_VULNERABLE_TO_COLD` | Same function checks `RACE_TRELUX` (-20) under `DAM_COLD` | Check the feat instead |
| Leap | `FEAT_LEAP` | The 20 percent avoidance in `src/combat/fight.c` (near the "trelux leap" comment) checks `RACE_TRELUX` | Check the feat instead |
| Giant Avoidance | `FEAT_COMBAT_TRAINING_VS_GIANTS` | Registered; `compute_armor_class()` in `src/combat/fight.c` grants +4 AC vs larger attackers on a race list (dwarf, crystal dwarf, gnome, duergar, halfling) that is narrower than the eight races holding the feat | Replace the race list with the feat check and keep the existing +4 AC. No attack bonus: the mechanic already exists, only its gate changes. Fix the feat text ("+1 size bonus") to match |
| Horse Body, Spider Body (slot part) | `FEAT_LEONINE_FRAME` | Registered; the leg and foot block is enforced for Wemic and Trelux through the per-race table read by `character_wear_slot_restriction()` in `src/character/race.c`, not through the feat | Make `character_wear_slot_restriction()` also refuse `WEAR_LEGS` and `WEAR_FEET` when the feat is held. The race table rows stay, so existing races are unchanged. `FEAT_TRELUX_EQ` is left on the race table |
| Magic Resistance, shrug 50 and above | `FEAT_LICH_SPELL_RESIST` | `compute_spell_res()` in `src/magic/magic.c` grants SR 15 + level on `IS_LICH()`; the feat is registered but never consulted | Check the feat instead; Lich keeps it through its existing assignment |
| Horse Body, Spider Body (stability part) | `FEAT_STABILITY` | Registered; bash and trip resistance in `src/combat/act.offensive.c` checks dwarf, crystal dwarf and duergar. Crystal dwarf holds no `FEAT_STABILITY` assignment, so a plain feat check would strip it | Check the feat instead and add the one missing `feat_race_assignment(RACE_CRYSTAL_DWARF, FEAT_STABILITY, ...)` line so behaviour is unchanged. The full Duris immunity is the new `FEAT_QUADRUPED_BODY` below |
| Dauntless | `FEAT_KENDER_FEARLESSNESS` | Wired in `is_immune_fear()`; text said "Kender" | Done: name is now "fearlessness", text "Immune to fear, normal and magical". The constant is unchanged |
| Battle Rage | `FEAT_HASTE` | Registered `in_game = FALSE`, `FEAT_TYPE_CLASS_ABILITY`, no command, no consumer | Done: renamed "innate haste", `in_game = TRUE`, type innate, 1/day self haste through the SLA table below. The verb is `battlehaste` because `battlerage` is already the domain-power command |
| Bodyslam | `SKILL_BODYSLAM` | `bodyslam` exists; availability in `src/character/skill_lists.c` checks `RACE_HALF_TROLL` | New `FEAT_BODYSLAM` (bucket C), make the skill available when the feat is held, and assign the feat to `RACE_HALF_TROLL` so that race keeps the skill |
| Regeneration (stronger) | `FEAT_TROLL_REGENERATION` | Fixed 3 hp | Optional: allow stacking (`can_stack = TRUE`, +3 per rank) so a Duris Troll (10 per tick) can be expressed as ranks. Do only if a race needs it |

### C. New feats to build

Forty-eight feats. Prices are the companion study's RP values so later
assignment can be budgeted; where our mechanic is smaller than the Duris one
the lower price is given.

Level gates are converted from Duris's 56 levels to our 30 (multiply by
30/56): L11 to 6, L16 to 9, L21 to 11, L26 to 14, L31 to 17, L41 to 22, L46
to 25. They are recorded here for the later assignment step only.

## Shared infrastructure

Do these once, before the feat groups.

**Constants.** Add the 48 constants to the feat-ID block at the end of the
feat list in `src/structs.h`, contiguous from 1268. Move `FEAT_LAST_FEAT` to
1316 and `NUM_FEATS` to 1317, keeping the existing convention that
`FEAT_LAST_FEAT` is one past the highest ID. `MAX_FEATS` (1500) does not
change.

**Registration.** One `feato()` per feat in `assign_feats()` in
`src/character/feats.c`, in a new "Duris racial innates" block. Every line is
`in_game = TRUE`, `can_learn = FALSE`, `can_stack = FALSE`,
`FEAT_TYPE_INNATE_ABILITY`. Drawback feats use the same type; the description
says it is a drawback.

**Daily uses.** For each active feat: a `dailyfeat(FEAT_X, eX)` call next to
the existing SLA entries in `feats.c`, a `case FEAT_X:` in `get_daily_uses()`
in `src/utils.c`, a new `event_id` in `src/mud_event.h`, and a
`PERSIST_CHARACTER_EVENT(eX)` row in `src/mud_event.c` beside `eSLA_LEVITATE`.
Events needed: `eSLA_FARSEE`, `eSLA_STONESKIN`, `eSLA_LIGHTNING_BOLT`,
`eSLA_FIRE_SHIELD`, `eSLA_FIRE_STORM`, `eSLA_SHADOW_JUMP`, `eSLA_PLANE_SHIFT`,
`eSLA_PSIONIC_BLAST`, `eSLA_SCARE`, `eSLA_HASTE`, `eSLA_FIREBALL`,
`eSLA_MASS_DISPEL`, `eSLA_FROST_BREATH`, `eSLA_WEB`, `eSUMMON_WARG`,
`eSUMMON_HORDE`, `eSTAMPEDE`, `eRACIAL_FLURRY`.

**One command for spell-like abilities.** The existing SLAs each have a
bespoke `ACMD` (`do_levitate`, `do_enlarge`, and so on). This plan adds
fourteen more SLAs, so add one table and one handler instead of fourteen
copies:

```
struct racial_sla_info
{
  int feat;         /* FEAT_x, gates use and daily count */
  int spellnum;     /* spell cast with call_magic() at character level */
  int target;       /* enum racial_sla_target: self, room, opponent,
                       world character, or every other character here */
  int flags;        /* RSLA_FLAG_COMBAT_ONLY, RSLA_FLAG_SIZE_LIMIT,
                       RSLA_FLAG_PASS_ARG (argument handed to cast_arg2) */
  const char *verb; /* command name, for messages */
};
ACMD(do_racial_sla); /* subcmd indexes racial_sla_table[] */
const struct racial_sla_info *racial_sla_lookup(int subcmd); /* for tests */
```

As built (Phase 0): the table, `racial_sla_lookup()`, and `do_racial_sla`
live in `src/act.other.c` directly before `do_invisiblerogue`; the
`ACMD_DECL` and lookup prototype are in `src/act.h` beside `do_levitate`
(that is where the existing SLA declarations are, not `interpreter.h`); the
`SCMD_RSLA_*` indices and `NUM_RACIAL_SLAS` are in `src/interpreter.h`; the
fourteen `cmd_info[]` rows follow the `levitate` row in `src/interpreter.c`.
The handler does the `HAS_FEAT`, precondition, `daily_uses_remaining()`,
target parsing, `call_magic()` (`CAST_INNATE`), and
`start_daily_use_cooldown()` sequence that `do_levitate` does today, and
refuses a self-target cast while `affected_by_spell()` for that spell so a
daily use is never wasted. No new source file was needed, so `Makefile.am`
and `CMakeLists.txt` changed only for the test file.

Verb collisions: `battlerage` already exists (domain power), so the haste
verb is `battlehaste`. The rest (farsee, stoneskin, throwlightning,
fireshield, firestorm, shadowdoor, planeshift, mindblast, roar, fireball,
massdispel, frostbreath, webwrap, flurry, summonwarg, summonhorde, stampede,
doorbash) are free. `calm` and `mine` already exist and are not reused.

**Help.** One entry per feat (keyword is the feat name) and one per new
command, in both `lib/text/help/help.hlp` and the help database, per the
repository rule. Model the text on the existing `ULTRAVISION` entry for
passives and the `BODYSLAM` entry for commands.

**Tests.** A new production-linked suite
`unittests/CuTest/test_racial_innate_feats.c`, added to `cutest_SOURCES` and
`cutest_test_files` in `Makefile.am` and to `CUTEST_TEST_SOURCES` in
`CMakeLists.txt`. Fixture style follows `test_rol_feats.c`. Coverage listed
per group below; every group also asserts the registry facts for its feats
(`in_game` true, `can_learn` false, type innate, name not "Unused Feat").

**Documentation.** Add rows for the new trait classes to the RP trait table
in `docs/guides/PLAYER_RACES_REFERENCE.md` (weapon-family mastery, terrain
stealth, sun vulnerability, quadruped body, party-scaling damage). Do not add
the feats to any race section there; that happens at assignment time. Index
this plan in `docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md`.

## Feat specifications

Each entry: Duris mechanic (with source site), our mechanic on the 30-level
scale, hook, and RP.

### Group 1: passive defence and resistance

**FEAT_SUN_VULNERABILITY** (drawback, RP -3). Duris: `INNATE_VULN_SUN`; hit
and move regeneration are zero in sunlight (`hit_regen()` and `move_regen()`
in `src/world/limits.c`), plus `sun_damage_check()` in `src/world/handler.c`
deals 1d8 scaled by race per check, suppressed by globe of darkness, forest,
swamp, or twilight rooms. Ours: no hit or move regeneration while
`IN_SUNLIGHT(ch)` and not `is_covered(ch)`; 1d8 sun damage per round in
`update_damage_and_effects_over_time_one()` in `src/limits.c`, using the
`TYPE_SUN_DAMAGE` path Vampire Weaknesses already uses; suppressed in
`SECT_FOREST` and `SECT_MARSHLAND` and under any darkness room affect. Hook:
`hit_gain()`, `move_gain()`, and the Vampire Weaknesses block in
`update_damage_and_effects_over_time_one()`. Test: regen returns zero in a
sunlit room with the feat and normal without it; no damage in a forest room.

**FEAT_DAYBLIND** (drawback, RP -4). Duris: `IS_DAYBLIND()` in
`src/core/utils.h`: the character is treated as blind in daylight unless
Eyeless, in a twilight room, or under globe of darkness. The `IS_BLIND()`
macro's dayblind term is itself commented out there, so it was only ever
partly live. Ours: while `IN_SUNLIGHT(ch)` and not `is_covered(ch)` and not
under a darkness room affect, the character cannot see (same effect as
`AFF_BLIND` for vision and attack penalties), with `FEAT_EYELESS` as an
override. Hook: a helper `is_dayblinded(ch)` consulted wherever `AFF_BLIND`
is read for vision in `src/utils.h` and `src/utils.c`. Test: cannot see in a
sunlit outdoor room; can see indoors and under darkness.

**FEAT_MAGIC_VULNERABILITY** (drawback, RP -1). Duris: `MAGIC_VULNERABILITY`
in `src/combat/dam_mods.c`, +10 percent spell damage. Ours: +10 percent
damage when the attack type is a spell (attack type below `TYPE_HIT`). Hook:
`compute_damtype_reduction()` in `src/combat/fight.c`, which already receives
`w_type`; subtract 10 from the reduction when it is a spell number. Test: a
damaging spell does 10 percent more; a weapon hit is unchanged.

**FEAT_MAGICAL_REDUCTION** (RP 1). Duris: `MAGICAL_REDUCTION`, -20 percent on
`SPLDAM_GENERIC` (untyped) spell damage. Ours: 20 percent reduction to
`DAM_FORCE` and `DAM_ENERGY`. Hook: `compute_damtype_reduction()`. Test: force
damage reduced by 20 percent; fire unchanged.

**FEAT_THICK_HIDE** (Troll Skin, RP 3). Duris: `INNATE_TROLL_SKIN`, melee
damage multiplied by `dam_factor[DF_TROLLSKIN]` (0.85) in
`src/combat/fight.c`. Ours: 15 percent reduction to slashing, piercing, and
bludgeoning damage. Hook: `compute_damtype_reduction()` under the physical
damage cases. Test: weapon damage reduced by 15 percent; fire unchanged.

**FEAT_SACRILEGIOUS_POWER** (RP 3). Duris: `INNATE_SACRILEGIOUS_POWER` in
`src/combat/dam_mods.c`, holy damage taken multiplied by 0.75 at level 46,
0.5 at 51, 0.25 at 56. Ours: holy damage reduced 25 percent at level 20, 50
percent at 25, 75 percent at 30. Hook: `compute_damtype_reduction()` under
`DAM_HOLY`. Test: reduction steps at levels 19, 20, 25, 30.

**FEAT_SPELL_ABSORB** (RP 2). Duris: `INNATE_SPELL_ABSORB` in
`src/combat/fight.c`: on spell damage of 50 or more, chance of level/2
percent to absorb the spell and refill an undead spell slot. Our slot system
differs, so the slot refund is dropped. Ours: when a damaging spell hits,
level/2 percent chance to take no damage, with a room message. Hook: the
spell-damage path in `damage()` in `src/combat/fight.c`, before resistance.
Test: with the roll forced, damage is zero and the message is sent.

**FEAT_EYELESS** (RP 1). Duris: `INNATE_EYELESS` sets `AFF5_NOBLIND`. Ours:
immune to the blinded condition, and `can_see` treats the character as
sighted while blinded, sharing the existing `FEAT_BLINDSENSE` branch. Hook:
where the blindness affect is applied in `mag_affects()` in
`src/magic/magic.c`, and beside the existing `FEAT_BLINDSENSE` vision check.
Test: blindness affect is refused; the feat does not grant darkvision.

**FEAT_QUICK_THINKING** (RP 1.5). Duris: `INNATE_QUICK_THINKING` in
`src/core/utility.c` and `src/net/sparser.c`: 15 percent automatic success on
INT and POW saves, and a second roll on a failed save. Ours: when a Will save
fails, 15 percent chance to reroll it once. Hook: `savingthrow()` in
`src/magic/magic.c` (or wherever Will saves resolve; trace before editing).
Test: with the roll forced, a failed Will save is retried; Fortitude is not.

**FEAT_GROUNDFIGHTING** (RP 1). Duris: `INNATE_GROUNDFIGHTING` halves the
dodge penalty for not standing (`src/combat/fight.c`). Ours: no attack roll
or AC penalty for being prone or sitting. Hook: the position penalties in
`compute_attack_bonus()` and `compute_armor_class()` in `src/combat/fight.c`.
Test: prone AC and attack equal standing values with the feat.

**FEAT_QUADRUPED_BODY** (Horse Body, Spider Body, RP 1.5). Duris:
`INNATE_HORSE_BODY` and `INNATE_SPIDER_BODY` in `src/cmd/actoff.c`,
`src/combat/grapple.c`, `src/classes/mount.c`: bash, trip, tackle, and
ground-slam fail against the character unless the attacker is larger; the
character cannot mount. Ours: `perform_knockdown()` fails automatically when
the attacker's size is equal or smaller; `do_mount` refuses. The equipment
slot loss is the wired `FEAT_LEONINE_FRAME` (bucket B), assigned alongside
this feat later. Hook: `perform_knockdown()` in `src/combat/act.offensive.c`,
`do_mount` in `src/act.other.c`. Test: knockdown from a same-size attacker
fails; from a larger attacker it proceeds to the normal roll.

**FEAT_WATER_BREATHING** (RP 0.5). Duris: `INNATE_WATERBREATH` sets
`AFF_WATERBREATH` permanently. Ours: the drowning and underwater checks treat
the character as having `AFF_WATER_BREATH`. Hook: wherever
`AFF_FLAGGED(ch, AFF_WATER_BREATH)` is read (trace; `src/limits.c` and
`src/movement/`). Test: no drowning damage in `SECT_UNDERWATER`.

**FEAT_UNDEAD_FEALTY** (RP 1). Duris: `INNATE_UNDEAD_FEALTY` in
`src/core/utility.c`: undead at least 10 levels below the character do not
aggro on it. Ours: same rule, undead race family only. Hook: the
`MOB_AGGRESSIVE` target loop in `src/mob/mob_act.c`, beside the existing
`FEAT_ONE_OF_US` (undead) and `FEAT_SOUL_OF_THE_FEY` (animal) exemptions.
`FEAT_ONE_OF_US` is not reused because it is a sorcerer bloodline bundle with
cold immunity and DR. Test: an undead mob 10 levels lower
skips the character; a living mob does not.

### Group 2: passive offence

**Weapon-family mastery** (four feats, RP 2 each): `FEAT_AXE_MASTERY`
(`WEAPON_FAMILY_AXE`), `FEAT_HAMMER_MASTERY` (`WEAPON_FAMILY_HAMMER`),
`FEAT_LONGSWORD_MASTERY` (`WEAPON_TYPE_LONG_SWORD`), `FEAT_GREATSWORD_MASTERY`
(Two-Handed Sword Mastery, `WEAPON_TYPE_GREAT_SWORD`). Duris:
`INNATE_AXE_MASTER`, `INNATE_HAMMER_MASTER`, `INNATE_LONGSWORD_MASTER` in
`src/magic/affects.c` add level/8 hit and level/12 (hammer: level/10)
damage while the primary weapon matches; `TWO_HANDED_SWORD_MASTERY` in
`src/core/utility.c` grants the 2H slashing skill at 100. Ours: +1 attack
and +1 damage per 8 character levels (maximum +3 each at level 24) while the
primary weapon matches. One helper, `racial_weapon_mastery_bonus(ch, wielded)`,
returning the bonus, called from `compute_attack_bonus()` and
`compute_damage_bonus()` in `src/combat/fight.c`. Test: bonus is 0 at level
7, 1 at 8, 3 at 24, 0 with a non-matching weapon.

**FEAT_HATRED** (RP 1). Duris: `INNATE_HATRED` in `src/classes/innates.c`
runs a timed event that triggers a rage when a hated race is in the room.
Ours: +1 attack and +2 damage against evil-aligned opponents, the Bloodhunt
model. Hook: beside `FEAT_BLOODHUNT` in `compute_attack_bonus()` and
`compute_damage_bonus()`. Test: bonus vs an evil target, none vs neutral.

**FEAT_BATTLE_FRENZY** (RP 1). Duris: `INNATE_BATTLE_FRENZY` in
`src/combat/fight.c`, 1 in 21 chance per hit on a humanoid to trigger an
extra attack. Ours: 5 percent chance on each successful melee hit against a
humanoid to gain one extra attack that round. Hook: `hit()` in
`src/combat/fight.c` after a successful attack. Test: with the roll forced,
an extra attack is queued; not against a non-humanoid.

**FEAT_WARCALLERS_FURY** (RP 2). Duris: `INNATE_WARCALLERS_FURY` in
`src/combat/dam_mods.c`: +2 to +15 percent damage by group size in room,
plus 1/30 per other Orog in the group. Ours: +1 damage per grouped member in
the room (self included), maximum +5. Hook: `compute_damage_bonus()`; group
walk copied from `FEAT_AUTHORITATIVE` in `src/utils.c`. Test: +2 with two
members present, +5 cap with seven.

**FEAT_RRAKKMA** (RP 1.5). Duris: `INNATE_RRAKKMA` in `src/combat/fight.c`
and `src/classes/innates.c`: -10 AC (better) and +5 shrug per other grouped
Githzerai in the room, capped at 5. Ours: +1 AC and +2 to saves against
spells per other grouped character in the room who also has this feat,
maximum 5 counted. Hook: `compute_armor_class()` and the save bonus path in
`src/magic/magic.c`. Test: no bonus alone; +1 AC with one other feat holder.

### Group 3: terrain and utility

**FEAT_OUTDOOR_STEALTH** (RP 1), **FEAT_SWAMP_STEALTH** (RP 0.5),
**FEAT_UNDERDARK_STEALTH** (RP 1). Duris: `INNATE_OUTDOOR_SNEAK`,
`INNATE_UD_SNEAK` (sneak at will in that terrain, `src/classes/innates.c`),
`INNATE_SWAMP_SNEAK` (hidden on the map in swamp terrain and no boat needed
in swamp water, `src/world/map.c`, `src/cmd/actmove.c`). Ours: +6 to stealth
checks in the matching sector set, the Bathed In Moonlight model. Outdoor is
any outdoor non-underdark sector; swamp is `SECT_MARSHLAND`; underdark is
`SECT_UD_WILD` through `SECT_UD_NOGROUND`. The Duris swamp boat waiver is
dropped; Swamp Stealth keeps only the stealth bonus. One helper
`racial_terrain_stealth_bonus(ch)` called where `FEAT_MOON_ELF_BATHED_IN_MOONLIGHT`
is applied in `src/character/abilities.c`. Test: bonus by sector for each
feat; zero elsewhere.

**FEAT_FOREST_SIGHT** (RP 0.5). Duris: `INNATE_FOREST_SIGHT` in
`src/world/map.c` lifts the map view cap in forest rooms. Our wilderness map
has no forest cap to lift, so: +4 to perception and spot in `SECT_FOREST`.
Hook: the same abilities helper as the stealth feats. Test: bonus in forest,
none in a field.

**FEAT_SEADOG** (RP 0.5). Duris: `INNATE_SEADOG` in `src/ships/ship_utils.c`
(+2 ship max speed) and `src/ships/ship_shop.c` (10 percent better sale). Ours:
+1 to the vessel's effective speed while the character is at the helm. Hook:
`get_terrain_speed_modifier()` callers in `src/vessels/` (trace the helm
speed path before editing). Lowest priority in the plan; ship sales have no
counterpart here and are dropped. Test: speed helper result with and without
the feat.

**FEAT_MINER** (RP 0.5). Duris: `INNATE_MINER` in `src/world/map.c` shows
mines and gem mines on the map at distance. Ours: +4 harvest skill level for
`RESOURCE_MINERALS`, `RESOURCE_STONE`, and `RESOURCE_CRYSTAL`. Hook:
`get_harvest_skill_level()` in `src/wilderness/resource_system.c`. Test:
skill level differs by 4 for minerals, unchanged for herbs.

**FEAT_BARTER** (RP 0.5). Duris: `INNATE_BARTER` in `src/economy/shop.c`:
25 percent better price on a Charisma check, else 10 percent worse. Ours: a
flat +10 to the character's side of the price modifier (the same weight as
10 points of Charisma) in both directions. Hook: `buy_price()` and
`sell_price()` in `src/obj/shop.c`, next to the appraise term. Test: buy
price lower and sell price higher with the feat.

**FEAT_CALMING** (RP 1.5). Duris: `INNATE_CALMING` in `src/mob/mobact.c`,
`src/world/handler.c`, `src/cmd/interp.c`: aggressive mobs skip the
character 75 percent of the time and delay their attack when within five
levels. Ours: an aggressive mob whose level is within five of the character
skips it 50 percent of the time on each aggression check. Hook: the same
`MOB_AGGRESSIVE` target loop as Undead Fealty. Test: with the roll forced,
the mob skips; a mob six levels higher does not.

### Group 4: active abilities through the SLA table

All 3/day unless stated; all use `call_magic()` at character level.

| Feat | Verb | Spell | Target | Duris source | RP |
|------|------|-------|--------|--------------|----|
| `FEAT_SLA_FARSEE` | `farsee` | `SPELL_FARSEE` | self | `INNATE_FARSEE` (permanent affect in Duris) | 0.5 |
| `FEAT_SLA_STONESKIN` | `stoneskin` | `SPELL_STONESKIN` | self, 1/day | `do_stone_skin()`, 300 s cooldown | 2 |
| `FEAT_SLA_LIGHTNING_BOLT` | `throwlightning` | `SPELL_LIGHTNING_BOLT` | current opponent, combat only | `do_throw_lightning()` | 1.5 |
| `FEAT_SLA_FIRE_SHIELD` | `fireshield` | `SPELL_FIRE_SHIELD` | self, 1/day | `INNATE_FIRESHIELD` | 2 |
| `FEAT_SLA_FIRE_STORM` | `firestorm` | `SPELL_FIRE_STORM` | room, 1/day | `INNATE_FIRESTORM` (no Duris implementation; spell exists here) | 2 |
| `FEAT_SLA_SHADOW_JUMP` | `shadowdoor <target>` | `SPELL_SHADOW_JUMP` | one character anywhere in the world, 1/day | `do_shadow_door()` casts dimension door; there is no dimension door here and `spell_shadow_jump()` with no target jumps to your own room, so the verb takes a target like the shadowdancer spell | 1 |
| `FEAT_SLA_PLANE_SHIFT` | `planeshift <astral or ethereal or elemental or prime>` | `SPELL_PLANE_SHIFT` | self, 1/day; the plane name is required and copied to `cast_arg2`, which `spell_plane_shift()` reads | `do_shift_astral()` and `do_shift_prime()`; one feat covers both directions | 2 |
| `FEAT_SLA_PSIONIC_BLAST` | `mindblast` | `PSIONIC_PSIONIC_BLAST` | current opponent; the power is `MAG_MASSES` here, so it stuns every hostile in the room and the target only has to exist | `INNATE_BLAST`, `spell_innate_blast()` | 2 |
| `FEAT_SLA_SCARE` | `roar` | `SPELL_SCARE` | single opponent | `INNATE_OGREROAR` in `src/classes/new_skills.c` | 1.5 |
| `FEAT_HASTE` (existing, repurposed as "innate haste") | `battlehaste` | `SPELL_HASTE` | self, 1/day | `do_battle_rage()`, 60 s haste. `battlerage` is taken by the domain power | 2 |
| `FEAT_SLA_FIREBALL` | `fireball` | `SPELL_FIREBALL` | single opponent | `do_fireball()`; its cooldown gate is commented out in Duris, ours uses the daily gate | 1.5 |
| `FEAT_SLA_MASS_DISPEL` | `massdispel` | `SPELL_DISPEL_MAGIC` | every other character in the room, 1/day | `do_mass_dispel()` | 2 |
| `FEAT_SLA_FROST_BREATH` | `frostbreath` | `SPELL_CONE_OF_COLD` (level d6 cold, single target). `SPELL_FROST_BREATHE` is the dragon breath: `MAG_AREAS`, level d16, far above the Duris level d4 | single opponent | `INNATE_BARB_BREATH`, level d4 cold | 2 |
| `FEAT_SLA_WEB` | `webwrap` | `SPELL_WEB` | single opponent at most one size larger | `webwrap()` in `src/classes/innates.c`, minor paralysis 5 to 10 rounds | 1.5 |

Test (in place): for each row, `get_daily_uses()` returns the configured
count, `feat_list[].event` is the row's event, `racial_sla_lookup()` finds
the row, the verb refuses without the feat, a failed precondition (no
opponent, not fighting, no plane name) does not spend a use, and one use
starts the cooldown event. `call_magic()` itself is not driven from the
test; that is the Phase 6 in-game check.

### Group 5: active abilities with bespoke commands

**FEAT_BODYSLAM** (RP 1). Existing `bodyslam` command and `SKILL_BODYSLAM`;
only the availability rule changes (bucket B). Test: the skill is available
with the feat and not without, independent of race.

**FEAT_DOORBASH** (RP 0.5). Duris: `do_doorbash()` in
`src/classes/innates.c` (and the Centaur `do_doorkick()` variant, merged
here). Ours: finish the commented-out `do_doorbash` in
`src/movement/movement.c`: `doorbash <direction>`; closed, non-pickproof exit;
success on d300 at or below strength plus level; success opens the exit on
both sides and breaks the lock, failure deals 1d6 plus a short wait. Register
the command and `ACMD_DECL`. Test: refused without the feat; opens a closed
door with the roll forced; pickproof refused.

**FEAT_STAMPEDE** (RP 1). Duris: `do_stampede()` in `src/cmd/actnew.c`
(refused in single-file rooms and without footing). Ours: `stampede`
attempts `perform_knockdown()` against every opponent fighting the character
and deals unarmed damage to each on success; usable once per 3 rounds
(`eSTAMPEDE` cooldown, not a daily use). Refused in `ROOM_SINGLEFILE`. Hook:
new `ACMD(do_stampede)` in `src/combat/act.offensive.c`. Test: refused
without the feat; hits every opponent in the room.

**FEAT_RACIAL_FLURRY** (RP 2). Duris: `do_flurry()` in
`src/classes/innates.c`: `AFF2_FLURRY` for four combat rounds, granting the
maximum attack count. Ours: `flurry`, 1/day: one extra attack per round for
four rounds, applied as a short affect that sets `AFF_HASTE` without
stacking with real haste. Distinct from the monk `FEAT_FLURRY_OF_BLOWS`
passive. Hook: new `ACMD(do_racial_flurry)` in `src/act.other.c`; the extra
attack comes from the existing haste handling in `src/combat/fight.c`.
Test: refused without the feat; affect present for four rounds; refused
while already hasted.

**FEAT_SUMMON_WARG** (RP 1.5). Duris: `do_summon_warg()` in
`src/classes/new_skills.c`, outdoors only, 1/day, delayed arrival, warg mount.
Ours: 1/day, outdoors, loads a warg mount mob that follows the character;
reuse the `FEAT_CALL_MOUNT` loading path in `src/act.other.c` with a warg
vnum. Needs a warg mob; add `MOB_VNUM_RACIAL_WARG` to `src/vnums.example.h`
(never hardcode) and a mob in the world files. Test: refused indoors; mob
loaded and following.

**FEAT_SUMMON_HORDE** (RP 1.5). Duris: `do_summon_horde()` in
`src/classes/new_skills.c`, thrice weekly, prime plane only, orcs arrive over
time. Ours: 1/day, loads two to four orc warrior mobs (vnum in
`vnums.example.h`) as timed followers, the `FEAT_VAMPIRE_CHILDREN_OF_THE_NIGHT`
model in `src/act.other.c`. Test: refused without the feat; followers loaded
with the expiry affect.

## Phases and checklist

- [x] Phase 0, infrastructure: constants, `feato()` block, events, daily-use
      cases, SLA table and `do_racial_sla`, test file skeleton in both build
      lists. Build clean with `-Wall -Wextra`. Done 2026-09-12.
- [x] Phase 1, bucket B wiring: fire and cold vulnerability, leap, giant
      training, leonine frame, stability, lich spell resistance, fearlessness
      text, haste repurpose, bodyslam availability. Existing races behave
      exactly as before; the test asserts the affected races still hold the
      feats that replaced the race checks. Done 2026-09-12.
- [ ] Phase 2, Group 1 (passive defence) and Group 2 (passive offence).
- [ ] Phase 3, Group 3 (terrain and utility) and Group 4 (SLA table rows).
- [ ] Phase 4, Group 5 (bespoke commands), including the warg and orc mob
      vnums and world entries.
- [ ] Phase 5, help in both stores for every feat and command; RP trait
      table rows; master index entry; `feat info` and `race feats` checked in
      game for one feat from each group.
- [ ] Phase 6, verification: `make -j$(nproc)`, `make test`, `make install`
      (no root `luminari` binary left), a login on port 4100 through
      `MUD_PORT=4100 ./scripts/autorun/autorun.sh`, and a staff character
      granted one feat per group with `set`/`feat` to exercise each verb.

Each phase is a reviewable commit. Phases 2 through 4 can run in any order
after Phase 0.

## Dropped and out of scope

| Item | Reason |
|------|--------|
| Dayvision | Registered on eight Duris races; zero implementation uses. No-op |
| Summon Totem, Project Image | No implementation in Duris (companion study appendix) |
| Webwrap (Drider) | Registration commented out in Duris; only the arachnid NPC race has it |
| Doorkick | Same mechanic as Doorbash with centaur flavour text; merged into `FEAT_DOORBASH` |
| Charge stun for Minotaur | General `charge` already exists; the Duris stun and one-room reach are a later upgrade if a race needs it |
| Swamp boat waiver, ship sale bonus, potion-spill resistance | Small side effects with no clean counterpart here; the parent feat keeps its main effect |
| Giant wield (two-handed weapons in one hand) | Race-keyed `IS_GIANT()` rule, not an innate. Tracked in the companion study; would be a separate feat that adjusts `hands_needed_full()` |
| Four arms (Thri-Kreen) | Body property, not an innate |
| Assigning any feat to a race, `innates` command output, RP rescoring | Deferred to the assignment step |

## Ablation record

- One SLA handler with a table instead of fourteen `ACMD` copies: fourteen real
  consumers justify the table; each existing bespoke SLA command is left
  alone.
- No new source file: the commands go beside their existing siblings in
  `src/act.other.c` and `src/combat/act.offensive.c`, so the build lists do
  not change.
- Reuse over new where the existing feat is generic: fearlessness (text
  change only), haste (repurpose an unused feat), bodyslam (availability
  rule), leonine frame and stability (wire). New constants only where no
  existing feat expresses the mechanic.
- Terrain stealth is three feats sharing one helper rather than one
  parameterised feat, because the feat table has no per-feat parameter and
  three sector sets are needed.
- Dropped mechanics with no implementation in Duris rather than inventing
  them.
- Giant training keeps the +4 AC that already exists instead of a new +1 AC
  and +1 attack pair: only the gate moves from a race list to the feat.
- Leonine frame hooks the existing `character_wear_slot_restriction()`
  helper instead of adding a check to `act.item.c`; the race table already
  carries the same restriction for Wemic and Trelux.
- The `FEAT_HASTE` "(3x/day)" special case in the feat list display was
  removed rather than rewritten; the short description carries "1/day" like
  the other SLA feats.

## Progress log

Keep this current. A new session should be able to continue from here
without re-reading the conversation.

- 2026-09-12, Phase 0 done. Files: `src/structs.h` (48 constants 1268 to
  1315, `FEAT_LAST_FEAT` 1316, `NUM_FEATS` 1317), `src/mud_event.h` (18
  events before `eMUD_EVENT_COUNT`), `src/mud_event_list.c` (18 rows at the
  end; `eSTAMPEDE` is an `event_countdown` row, the rest
  `event_daily_use_cooldown`), `src/mud_event.c` (17 `PERSIST_CHARACTER_EVENT`
  rows; `eSTAMPEDE` is a three-round cooldown and is not persisted),
  `src/character/feats.c` (Duris block after `FEAT_LEONINE_FRAME`, `FEAT_HASTE`
  moved into it, 17 `dailyfeat()` lines, `FEAT_HASTE` display special case
  removed), `src/utils.c` (`get_daily_uses()` cases), `src/interpreter.h`
  (`SCMD_RSLA_*`), `src/act.h`, `src/act.other.c`, `src/interpreter.c`,
  `unittests/CuTest/test_racial_innate_feats.c` (four tests), `Makefile.am`,
  `CMakeLists.txt`, and `unittests/CuTest/test_syntax_check_boot.c` (persisted
  event count 93 to 110).
- 2026-09-12, Phase 1 done. `src/combat/fight.c`: fire (-50) and cold (-20)
  vulnerability in `compute_damtype_reduction()`, the leap dodge in
  `damage_handling_with_weapon()`, and the +4 AC vs larger attackers in
  `compute_armor_class()` now check the feats. `src/magic/magic.c`:
  `compute_spell_res()` checks `FEAT_LICH_SPELL_RESIST`.
  `src/combat/act.offensive.c`: both stability checks in `perform_knockdown()`
  use `FEAT_STABILITY`. `src/character/skill_lists.c`: bodyslam needs
  `FEAT_BODYSLAM`. `src/character/race.c`: `character_wear_slot_restriction()`
  refuses legs and feet for `FEAT_LEONINE_FRAME`; crystal dwarf gains the
  `FEAT_STABILITY` assignment and half-troll the `FEAT_BODYSLAM` assignment
  it needed to keep its behaviour. `src/character/feats.c`: fearlessness and
  giant-training text, lich long text says 15 + level. Seven tests added.
- Next: Phases 2 to 4 in any order, then 5 and 6.
