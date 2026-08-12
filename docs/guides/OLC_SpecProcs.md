# OLC: SpecProc Editing and Persistence

This note explains how builders can assign and persist special procedures (SpecProcs) using the OLC
editors for mobs, objects, and rooms.

## Overview

- All three editors provide `Z) SpecProc`.
- Select from the centralized registry defined in `src/spec/spec_registry.c`.
- Each editor lists only builder-visible definitions compatible with that mobile, object, or room.
- Selections apply at save time and now persist across reboots via world files.
- In game, `HELP SPECIALS` provides the shorter builder and staff reference, including the
  staff-only `specbind` diagnostic.

Three states are intentionally separate:

- A **definition** is immutable registry metadata: canonical name, aliases, compatible owners,
  supported events, prerequisites, category, description, and allowed binding sources.
- An **authored binding** is the exact name requested by a world file or explicit OLC action. It is
  preserved even when the name is unknown or incompatible and installs no callback.
- An **effective binding** is the callback left in the prototype slot after world loading, parser
  hooks, legacy assignments, shops, and quests contribute in their established boot order.

OLC edits authored state. Startup diagnostics observe effective state. Neither surface changes the
callback slot or event-gateway dispatch rules.

## Usage (medit/oedit/redit)

- From the main menu, press `Z` to open the SpecProc selector.
- Read the category, description, supported events, and per-event prerequisites beneath each entry.
- Enter a number to choose a SpecProc; enter `0` to clear.
- The current selection is shown in the menu. Save as usual to apply.

Numbers are specific to the filtered editor view. The current views contain 54 mobile definitions,
34 object definitions, and 17 room definitions in canonical registry order. The saved world record
uses the procedure name, not the displayed number.

Prerequisites describe runtime scheduling; selecting a procedure does not set them automatically:

- `MOB_SPEC` enables mobile activity and combat-turn callbacks that require it.
- `ITEM_AUTOPROC` enables object auto-pulses that require it.
- `carried`, `equipped`, and `combat` state describe where an event can run.
- `prerequisites: none` means that event has no registry-level flag or placement requirement.

Converted RoL demon, devil, and umber-hulk prototypes are a deliberate exception to the
single-SpecProc scheduling model. The converter writes `RoL-Demon`, `RoL-Devil`, or
`RoL-Umberhulk` mobile flags, and independent runtime hooks preserve their source boot and combat
behavior beside any ordinary persistent SpecProc. These converter-owned flags are not additional
authored SpecProcs and should not be added to unrelated new mobiles.

Converted RoL death flags also run outside the single named SpecProc slot. Familiar, mount,
summoned-monster, and shaman-spirit flags provide their source fade messages and suppress corpses.
`MOB_ROL_BLACK_VAPOR_DEATH` replaces Luminari's generic undead crumble message with the source
Bloodstone vapor message while retaining the target's no-corpse policy. These flags are
converter-owned and should not be assigned to unrelated mobiles.

Forty-five converted mobiles use VNUM-owned death profiles rather than flags. In addition to the
tentacle, mephit, elemental, treant, phantom-steed, and dark-shade no-corpse profiles, the table
preserves source death bursts, darkness, poison gas, returned possessions, stone-pile inventory,
replacement forms, dropped objects, loose possessions, splitting skeletons, and ordinary-corpse
messages. Three replacement families also retain their source cleric-retargeting activity. These
profiles run without consuming the named SpecProc slot, are converter-owned, and do not change
arbitrary mobiles' death behavior.

`RoL Bloodstone Critter` is mobile-owned and requires `MOB_SPEC`. While awake and idle, the four
converted Bloodstone critters use Luminari's current `snarl` and `growl` socials at the source
two-in-81 activity-pulse cadence. This procedure is converter-owned.

`RoL Designated Follower` is mobile-owned and requires `MOB_SPEC`. Five converted Icecrag guards
find their fixed NPC leaders when awake and colocated, then use the target follower system to move
with them and participate in their fights. The mapping and procedure are converter-owned.

`RoL Fixed Bodyguard` is mobile-owned and requires `MOB_SPEC`. Converted Icecrag bodyguards
2097040-2097042 watch for attacks against their assigned mobiles 2097023, 2097029, and 2097008
respectively. While awake and colocated, they use the target rescue mechanic when the assigned
mobile has an attacker. The fixed mapping and procedure are converter-owned.

`RoL Floating Pool` is object-owned and requires `ITEM_AUTOPROC`. Four converted Ethereal objects
left in rooms have the source-documented 12 percent chance per object pulse to float through one
random open cardinal exit. Closed, hidden, blocked, invalid, and `ROOM_NOMOB` destinations are
excluded. The procedure and its pulse contract are converter-owned.

`RoL Bloodstone Portal` is object-owned. Four converted portals remap object value 0 to the target
destination room and intercept `enter` when an awake character selects that exact room object. A
mortal that passes target teleport-admission checks moves to the destination, loses 1-20 hit points
and 1-30 movement points, and dies only when the hit-point loss would leave them below -10. Staff
retain the source stress immunity. The procedure and value remapping are converter-owned.

`RoL Portal Door` is object-owned. Four converted miscellaneous portals remap object value 0 to
the destination room and use value 3 to reject source-good or source-evil races. `look in` previews
the loaded destination. `enter` preserves the source level-20 and arena-boundary gates, applies
target teleport-admission safety, and moves only the character selecting that exact portal object.
The procedure and destination remapping are converter-owned.

`RoL Travel Portal` is object-owned and identity-keyed to nine converted objects. It preserves a
dimensional fold with `look in` preview, two Waterdeep portals with value-one mortal damage, a
Wizard-only converted illusionist fountain, two level-20 Elf gates with four randomized destination
slots, two carried Cleric spores with a non-Cleric stun path, and a Blip portal that gives converted
badge 2041900. Destination values are remapped by the converter. Do not assign this procedure to
unrelated objects; their identities have no travel profile.

`RoL Item Blocker` is object-owned and reads the blocked cardinal direction from object value 0.
While an aggressive NPC occupies the room, it blocks mortal players and player pets from moving
or unlocking a door in that direction. This procedure and its six ATD objects are converter-owned.

`RoL Magic Pool` is an object-owned conversion procedure. Its object value 0 is the destination
room VNUM and value 1 is fixed entry damage. The converter remaps the destination; builders should
not assign the procedure to an unrelated object without configuring both values deliberately.

`RoL Auto Distributor` is room-owned. Any command from a non-staff character is intercepted and
moves that character to a randomly selected loaded room in the same zone. It is intended only for
converted RoL boundary rooms; attaching it elsewhere turns that room into a randomizing trap.

`RoL Shadow Giant` is mobile-owned and runs on mobile-activity pulses while the giant is fighting.
Its source 1-in-21 trigger spooks every player and charmed pet in the room for mental damage and a
possible short stun. Converted angel identity is retained for the source immunity list. This
procedure is intended for the converted RoL shadow-giant family rather than general new mobiles.

`RoL Guild Guard` is mobile-owned and requires `MOB_SPEC`. It preserves the converted room-specific
class and race gates, acts only from the guard's original load room, and retaliates when protected
guards are attacked. Its room rules are converter-owned; use the ordinary `Guild Guard` procedure
for new Luminari guild entrances. The converted rules include six Bloodstone class entrances;
source Antipaladin maps to Blackguard, Shaman to Cleric, and Lich to Necromancer. Seven Waterdeep
guild guards also compose their class gate and guardian retaliation with generated, source-hashed
idle and fighting flavor tables.

`RoL Waterdeep Guild Room` is room-owned and converter-owned. Twelve converted Waterdeep guild
rooms retain their exact or class-family admission gate while delegating accepted `practice`,
`train`, and `boosts` commands to the current target guild service. The source mercenary room maps
to the target Warrior class, and any qualifying class in a multiclass build is sufficient.

`RoL Major Beholder` is mobile-owned and requires `MOB_SPEC`. Its ten source eye identities each
have an independent three-combat-turn cooldown and a one-in-three chance to fire while ready. The
target-native mappings cover fire, acid, slow, enfeeblement/feeblemind, wither, room-wide dispel,
prismatic spray, hold monster, harm, and finger of death. Pet targets redirect to an eligible master
in the room. The source engine's critical-hit callback has no target combat-turn equivalent, so its
all-unused-eyes critical burst is not available. Use this converter-owned procedure only for the
converted `major_beholder` family.

`RoL Monster Combat` is mobile-owned and requires `MOB_SPEC`. Forty-five converted mobile
identities across thirty-five source families share identity-keyed combat, activity, and command
profiles. They cover plant poison, lycans, spider venom, shockwaves, celestial and prismatic
effects, Elemental Tower bosses, kobolds, piercers, purple worms, phalanxes, splitting skeletons,
transformations, tree spirits, Dranum, swallow attacks, Canthus, Jotuns, and pit fiends. Elemental
Tower alerts and pit-fiend tails compose through this single persisted procedure. Four lycans, the
small prismatic elemental, purple worm, phalanx, and two skeleton identities also retain their
source death behavior. The critical prismatic identity maps its unavailable NPC-critical callback
to a documented one-in-20 combat-turn burst. Do not assign this converter-owned procedure to
unrelated mobiles.

`RoL Lich Energy Drain` is mobile-owned and requires `MOB_SPEC`. On both activity pulses and
combat turns, each eligible current opponent or party member has the source one-in-five chance to
lose all current hit points plus five; target Death Ward maps the source protection-from-undead
case and leaves the victim at zero instead. The lich receives the victim's former current hit
points unless Blackmantled, and each drain adds two combat rounds of stun. The callback does
not fire while the lich is casting. Use this converter-owned procedure only for the converted
`lich_energy_drain` family.

`RoL Trade Bandit` is mobile-owned and requires `MOB_SPEC`. It intercepts movement, `flee`, and
`get` for a merchant carrying converted resources or owning a loaded wagon, then applies one of
seven converter-owned toll personalities. Source platinum tolls use ten target gold per platinum;
resource and wagon-content costs retain their source copper-scale threshold. Payment uses the
ordinary `give <amount> gold <bandit>` command. Some variants demand all carried gold and the
owned wagon, vary by alignment, or attack immediately. Do not assign this procedure to unrelated
mobiles: its behavior is keyed to converted mobile VNUMs 2099501-2099507.

`RoL Alert Caller` is mobile-owned and requires `MOB_SPEC`. Eleven converted callers broadcast their
source-specific combat warning once per fight and send only their configured awake, idle helpers in
the same zone to pursue a reachable attacker within 100 rooms. Soundproof rooms, silence,
paralysis, casting, and sleep suppress the call. The Imix and Yancbin callers compose the same alert
profile beside their existing fire or lightning breath procedure, so they retain both behaviors
without a second persisted SpecProc. Elemental Tower callers 2062401, 2062402, 2062405, and 2062406
use the same adapter with their source-specific helper groups.

`RoL Yggdrasil Branch` is mobile-owned and requires `MOB_SPEC`. Converted mobiles 2062800-2062804
make a 50 percent entangle attempt against either the current opponent or the source-style
vulnerable group target. A failed Reflex save with the source -10 modifier entangles the target for
four to twelve combat rounds. Release removes the entangle and halves current movement. The timed
effect and its mobile association are converter-owned.

`RoL Waterdeep Ambient` is mobile-owned and requires `MOB_SPEC`. Fifty-six converted citizens
across 44 source families emit their authored speech and room actions on the original two-die
periodic distributions while standing. Multi-line outcomes retain their ordering and the casino
player retains its source fall-through outcome. Converted merchant 2005310 emits its harbor dialog
only in room 2005400; converted guards 2003035, 2003059, and 2003070 remain quiet while fighting.
The profiles are keyed to converted mobile identity; do not assign this procedure to unrelated
mobiles.

`RoL Source Periodic` is mobile-owned and requires `MOB_SPEC`. One hundred converted Bloodstone,
Icecrag, Menden, Fun, Mobile, Realm, Lavatubes, Tower of Sorcery, and Waterdeep mobiles across 94
source families use 367 source random outcomes containing 601 ordered speech or room-visible
actions. The generated profiles preserve each source random range or dice expression, fall-through
order, room text, visibility setting, awake or sleeping gate, and combat gate. Fun mobile 2001230,
jester 2003069, and cricket 2014048 retain source profiles that do not require an awake mobile.
Waterdeep guard 2003212 runs only while sleeping. Fun mobile 2001230, jester 2003069, and Menden
magus 2088806 also retain source profiles that continue during combat. The profiles are keyed to
converted mobile identity; do not assign this converter-owned procedure to unrelated mobiles.

`RoL Stateful Periodic` is mobile-owned and requires `MOB_SPEC`. Thirty-three converted Waterdeep
mobiles have generated idle and fighting profiles containing 258 source outcomes and 266 ordered
speech or room-visible actions. Idle tables require an awake, standing mobile. Fighting tables run
instead whenever a mobile has a current opponent; this makes the source-authored combat tables
usable even where the source tested standing position before fighting state. Guildmaster 2003020
has no authored fighting table and remains quiet in combat. Twenty-six profiles use this procedure
directly; seven Waterdeep guild guards compose the same generated tables through `RoL Guild Guard`.
The profiles are keyed to converted mobile identity; do not assign either converter-owned
procedure to unrelated mobiles.

`RoL Shaman Totem` is object-owned and must be held or wielded. It preserves the 21 converted
totem identities, permanent player/object bonding, source-race gating, three summon attempts per
seven MUD days, and a single active spirit. Because the source Shaman class maps to Cleric, a
Cleric can bond at any class level, but summoning requires Cleric level 21. Cleric level and
Wisdom replace the unavailable source trainable skill.
The procedure and `MOB_ROL_TOTEM_SPIRIT` flag are converter-owned and should not be assigned to
unrelated objects or mobiles.

The five `RoL Ship` definitions preserve the source system for seven converted fixed-interior
ships. `RoL Ship` is assigned to a hull object, `RoL Ship Control` to its panel object, `RoL Ship
Exit` and `RoL Ship Lookout` to interior rooms, and `RoL Ship Navigator` to the route mobile. The
navigator requires `MOB_SPEC` for its combat-turn crew response. The converter owns the hull,
interior, route, and navigator associations; assigning one of these procedures to unrelated content
does not create a new operable ship.

## File Format Persistence

The selected SpecProc is stored by name and resolved at boot.

- Mobs (E-spec block):
  - Line written inside the `E` specs section:
    - `SpecProc: <Name>`

- Objects:
  - A `Z` block is added:
    - Line 1: `Z`
    - Line 2: `<Name>`

- Rooms:
  - A `Z` block is added before the terminating `S` line:
    - Line 1: `Z`
    - Line 2: `<Name>`

If the name is not recognized in the SpecProc registry, the function is not assigned at boot, but
the authored name remains attached to the prototype for diagnostics and later saves.

## Authored Binding State

World loading now keeps an owned authored-binding record on each mobile, object, or room prototype.
The record includes the exact requested name, owner and VNUM, source kind, source location, resolved
registry definition, and resolution status. Canonical names and aliases resolve to the same immutable
definition while preserving the spelling that appeared in the world file.

Unknown names and definitions that are incompatible with the owner or source remain available for
diagnostics, but they do not install a callback. Boot warnings identify the persisted field, owner,
VNUM, requested name, and reason. Prototype copies and OLC editing use independent owned records so
editing or deleting one prototype cannot invalidate another.

Disk writers use this authored record whenever it exists. Exact loaded aliases, unknown names, and
owner- or source-incompatible names therefore survive unrelated OLC saves. A later hard-coded
callback override remains effective at runtime but is not promoted into the authored world field.
Only prototypes with no authored record use callback reverse lookup as a compatibility fallback.

Builder actions are explicit. Selecting a registry entry replaces any prior record with its
canonical name. Entering `0` clears both the authored record and callback, so the field is omitted
on the next zone save. Merely opening and saving an editor preserves the existing requested name,
including unresolved content that a builder may need to repair later.

To repair an unresolved or incompatible authored name, select a compatible definition from the
current editor. That explicit action replaces the old request with the selected canonical name.
Use `0` only when the intended result is no authored procedure. OLC does not offer free-form entry
of an unregistered name.

## Moving Rooms

A moving room's `M` data and a named room SpecProc both own the room's single callback slot. They
cannot be combined. REdit refuses to open the SpecProc selector for a moving room, rejects a
defensive internal save containing both forms of ownership, and the room writer rejects the whole
zone before opening its output file if it would emit both `M` and `Z` for any room.

Boot enforces the same rule in either field order. A room file containing `M` followed by `Z`, or
`Z` followed by `M`, stops loading with a diagnostic naming the room VNUM and conflicting field.
Choose either moving-room behavior or a registry-backed room SpecProc before saving the zone.

## Startup Diagnostics

Boot logs effective binding provenance after the normal assignment sequence. Each contribution is
one `SPEC_BIND` line containing the mode, owner, VNUM, step, source, requested name, installed
handler, outcome, source location, and saved secondary handler. A `SPEC_BIND_FINAL` line then gives
the authored name, contribution and collision counts, and final source and handler. The surrounding
`SPEC_BIND_SUMMARY` lines provide aggregate prototype, contribution, and collision counts.

This report is a boot-time snapshot. Later OLC reassignment does not rewrite the recorded chain.

Immortal staff can inspect that snapshot without searching the boot log:

```text
specbind <mob|obj|room> <vnum>
```

The command shows the prototype's current effective callback, every ordered contribution and its
outcome, the source location recorded at boot, collision count, saved shop or quest secondary, and
the chosen source. `mob`/`mobile` and `obj`/`object` are accepted. The command is read-only: it does
not change the callback, authored request, or recorded history.

Contribution outcomes have these meanings:

- `selected`: the contribution installed the first resolved callback after no callback was active.
- `unresolved`: the authored request resolved to no callback.
- `overridden`: a later source replaced a different active callback.
- `reasserted`: a later source installed the same callback again.
- `wrapped`: a shop or quest wrapper became effective and preserved the displayed `secondary`
  callback for its existing delegation behavior.

The `source` field distinguishes `world`, `parser-hook`, `legacy-assignment`, `shop`, and `quest`.
Normal boot reports every source that actually contributed. With `-s`, named world and moving-room
parser records are still reported, while the guarded legacy, shop, and quest assignment sources are
absent. This describes the existing boot path; it does not add a new runtime dispatch switch.

The boot-log lines and `specbind` output are diagnostics, not world-file input or OLC mutations. The
prototype callback pointer remains runtime authority. A collision count reports that more than one
source contributed; it does not create a multiple-handler chain.

## Compatibility Boundary Through Phase 06

Phases 00-06 change registration, selection, persistence safety, observability, call-site routing,
two eligible legacy assignments, source ownership, narrow runtime safety mechanics, and two typed
implementations. They do not
change the `SPECIAL` callback ABI, command-owner
traversal, heartbeat positions, caller-specific return handling, activation flags, world-file
grammar, or established assignment precedence. Shop and quest wrappers keep their existing
saved-secondary behavior.

Typed dispatch is an engine-side implementation detail: nothing a builder selects, sees, or saves in
OLC changed. Bank and Vampire Cloak retain their canonical rows and callback-slot identities while
their behavior receives explicit event context. The other 90 registered definitions use
compatibility dispatch; across the source tree, 204 legacy behavior implementations remain. A
validated declarative table owns the two Luminari assignments whose handlers are registered and
whose VNUMs are symbolic. Converted RoL definitions use explicit world-authored names.
Unsupported numeric, computed, and campaign-compatibility assignments remain visible through the
same effective-binding diagnostics. Phase 06 deliberately closed without general
multiple-procedure composition: no current content needs a second persisted handler, so OLC still
selects, clears, and saves zero or one authored name. Shop and quest saved secondaries are
runtime-only compatibility pointers, not additional builder-editable rows. Any future chain requires
an approved consumer plus versioned loading, deterministic order, and complete OLC reorder, clear,
unresolved-name, and save semantics.

For the implementation boundaries, see
[Developer Guide and API](DEVELOPER_GUIDE_AND_API.md#special-procedure-control-plane). For the exact
production-linked evidence, see
[Phase 00 Validation](../testing/SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md),
[Phase 01 Validation](../testing/SPECIAL_PROCEDURE_PHASE_01_VALIDATION.md),
[Phase 02 Validation](../testing/SPECIAL_PROCEDURE_PHASE_02_VALIDATION.md),
[Phase 03 Validation](../testing/SPECIAL_PROCEDURE_PHASE_03_VALIDATION.md),
[Phase 04 Validation](../testing/SPECIAL_PROCEDURE_PHASE_04_VALIDATION.md),
[Phase 05 Validation](../testing/SPECIAL_PROCEDURE_PHASE_05_VALIDATION.md), and
[Phase 06 Validation](../testing/SPECIAL_PROCEDURE_PHASE_06_VALIDATION.md).

## Notes and Tips

- Names should match a canonical name or explicit alias in `src/spec/spec_registry.c`; other names
  remain persisted and diagnosable but do not install a callback.
- The selector shows canonical definitions only. An alias such as `Guildmaster` still loads for
  compatibility but does not create a duplicate menu row; selecting the entry saves `Guild`.
- `Guild` is the mobile-owned training procedure. `RoL Guild Room` exposes the same current
  training service through an unrestricted converted room binding. `RoL Mage Guild Room`,
  `RoL Thief Guild Room`, `RoL Warrior Guild Room`, `RoL Cleric Guild Room`, and `RoL Bard Guild
  Room` expose it only to matching class families; any qualifying class in a multiclass build is
  sufficient. `RoL Waterdeep Guild Room` applies the room-specific gate for twelve converted
  Waterdeep guilds. All seven room procedures are available only in `redit`.
- A procedure hidden from builders, disallowed for world binding, or incompatible with the edited
  owner is not selectable. Invalid and out-of-range input leaves the current selection unchanged.
- Registry metadata is validated before world parsing. An invalid registry is a programmer error
  that stops boot; an unknown persisted name remains a content error and is not assigned.
- A moving room cannot also select or persist a named room SpecProc because both require the same
  callback slot.
- Clearing a SpecProc removes the corresponding lines from the world file on next save.
- The selector is 1-based; `0` always clears.

## Troubleshooting

- Change not taking effect after save: ensure the zone was saved and the game reloaded the zone or rebooted.
- Procedure does not run on a pulse: reopen the selector and check the event's required flags and
  placement. The selector deliberately does not change those flags for you.
- Procedure is absent from one editor: it is not compatible with that owner type or is not allowed
  for builder-authored world binding.
- REdit refuses the SpecProc selector: the room has moving-room ownership; remove that configuration
  before assigning a named room procedure.
- Persistence missing after reboot: verify the saved name is a canonical name or alias in the
  definition registry and has not been renamed.
- `specbind` reports no contributions: the prototype received no world, parser, legacy, shop, or
  quest callback write during this boot. Confirm the VNUM and whether the server was started with
  `-s`.
- File merge conflicts: the `SpecProc`/`Z` entries are safe to keep; ensure the SpecProc name remains on its own line as shown above.

## Examples
- Mob E-spec example snippet:
  - `SpecProc: Receptionist`
- Object snippet:
  - `Z` then `Bank`
- Room snippet:
  - `Z` then `Wizard Library`
