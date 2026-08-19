# Artifact Placement Plan - World-Building Handoff

Status: ready for a world builder. No code work is outstanding for this.

This document is the content brief for the one open item in the artifact
system: placing artifacts in player-facing content. It is written to be
handed to the production world-building agent. The system it plugs into is
documented in
[`ARTIFACT_SYSTEM.md`](../systems/ARTIFACT_SYSTEM.md). Nothing in
it requires a source change: the artifact runtime, the reset guards, the
roster, and the contract metadata are all already in place and are described
below only so the placement decisions respect them.

The engineering side deliberately stops here. Live world data
(`lib/world/zon`, `lib/world/wld`, `lib/world/mob`, `lib/world/obj`) is not
under version control and is owned by builders, so this repository does not
place artifacts itself.

## 1. What exists today

All seventeen artifacts reset into the private Vault of Ages, room 169900,
from the tracked package `lib/world/artifacts/1699.zon`. That room has no
exits. This is correct for staging and wrong for gameplay: nothing in the
world hands an artifact to a player.

Provisioning copies the package into the live world:

```bash
scripts/provision_artifacts.sh
```

It is idempotent and never rewrites a prototype a builder has already edited
through OLC. It adds object prototypes and vault resets that are missing and
leaves existing records in place. A builder's placement edits remain safe:
once an `O` reset for an artifact exists in `lib/world/zon/1699.zon`, the
provisioner will not add a second one, and once a prototype exists it is never
overwritten.

## 2. The single-instance contract placement must respect

`artifact_block_zone_load()` in `src/obj/spec_artifacts.c:2788` is called from
all four object-bearing reset commands in `src/db.c` - `O` (room), `P` (into
a container), `G` (into mobile inventory), and `E` (equipped on a mobile). A
reset is silently skipped when either:

- an instance of that artifact object already exists anywhere in the game; or
- the artifact is durably owned and its instance lives in a player or house
  save.

Consequences a builder can rely on:

- **Multiple resets for one artifact are safe.** Placing the same artifact on
  two different mobiles in two different zones does not create two copies. The
  first reset that fires wins; every later one is skipped for as long as the
  instance exists. This is the intended way to give an artifact more than one
  possible source without risking duplication.
- **Percentage arguments still work.** The guard runs before the percentage
  roll, so a `50` chance reset simply means the artifact may not appear this
  cycle.
- **Removing the vault reset is optional.** Leaving the vault `O` reset in
  place as a last-resort fallback is harmless, but it will usually win the
  race on a cold boot because zone 1699 resets early. If an artifact should
  come from live content, **delete or comment its vault reset** in
  `lib/world/zon/1699.zon`. Do not delete the room, the prototypes, or the
  zone header.
- **Do not use `load obj` or a DG `%load%` to hand out an artifact.** Those
  paths bypass the reset guard. Use resets, or award through a quest reward
  that routes the object through the normal acquisition hook.

## 3. Placement rules that apply to every artifact

1. One artifact, one intended source zone. Secondary resets are a fallback,
   not a second faucet.
2. The carrier mobile must be non-`!SENTINEL`-hostile enough to actually be
   fought, and must not be a shopkeeper. An artifact in a shop inventory is
   purchasable and defeats the acquisition contract.
3. Never place an artifact in a room a player can reach without opposition
   when its contract says `boss` or `quest`.
4. Prefer `E` (equipped) over `G` (carried) for weapons on a boss: an equipped
   artifact is used against the party, which is the point.
5. Artifacts bound on pickup (`ARTIFACT_BIND_ON_PICKUP`) must not be placed
   somewhere a player can trip over them by accident. See the binding column
   in section 4.
6. Record the placement in the zone's builder notes and in
   `docs/systems/ARTIFACT_SYSTEM.md`'s roster, so the acquisition hint the
   `artifact chronicle` command shows matches reality.

## 4. Per-artifact brief

Binding, class oath, and signature proc are already implemented and are given
here because they constrain who the placement should be aimed at. "Oath"
means the artifact burns a bearer who has ten or more levels outside the
listed class; it does not stop them equipping it.

### 4.1 First wave - currently `ART_ACQ_VAULT`

These eleven have no acquisition mode chosen yet. **Choosing one is the first
decision this handoff needs.** Until a builder picks a mode, they stay vault
staged and the roster honestly reports them as unreleased. When a mode is
chosen, the contract row in `artifact_contracts[]`
(`src/obj/spec_artifacts.c:229`) must be updated to match - that is a
one-line source change and the only code work this whole task can generate.

| Vnum | Name | Binding | Oath | Suggested mode | Placement note |
| --- | --- | --- | --- | --- | --- |
| 169901 | Trorxek, Staff of Ancient Oaks | on equip | druid | boss | A deep-forest or fey-court boss. Its called effect summons the Oaken Defender, mobile 169912, which is already in the package. |
| 169902 | Amaukekel, Rod of Light | on equip | cleric | quest | A temple or church chain. Its powers are resurrection and dispel evil; the giver should be a church. |
| 169903 | Fade, the Shadowblade | on equip | rogue | exploration | A thieves' quarter or shadow plane chain. Contract hint already says thieves trade the rumour, not the sword. |
| 169904 | Horn of Henekar | on equip | rogue | quest | Its charm effect is capped by target max HP; keep the source away from any zone with charm-exploitable mobiles. |
| 169905 | Doombringer | **on pickup** | warrior | boss | Owner policy is already public. A high-tier boss; do not leave it lootable from a corpse in a public road room. |
| 169906 | Kelrarin's Hammer | on equip | none | boss | Anti-evil themed; a fiend or undead boss reads best. |
| 169907 | Kelrom | on equip | none | quest | Refuses to strike animals. A druidic or ranger giver fits. |
| 169908 | Gesen | **none** | none | exploration | The only unbound artifact in the roster. It can change hands freely, so it is the safest one to place somewhere reachable. |
| 169909 | Tiamat's Stinger | **on account** | none | boss | A chromatic dragon boss. Account binding means one per account, ever. |
| 169910 | Avernus | on equip | none | boss | Infernal theme. |
| 169911 | Aegis of Ages | on equip | none | quest | Pure defensive breastplate armor; a fortress or siege chain fits. |

### 4.2 Second wave - acquisition mode already declared

These six already declare a mode in their contract. Placement must implement
the mode that is declared, or the contract row must be changed to match what
was built. Do not leave the two out of step: `artifact chronicle <name>` shows
the hint to players.

#### Vengeance - 169913 - `ART_ACQ_QUEST`, owner public

- Binding: on equip. Oath: paladin.
- Signature proc: `ART_SIG_MERCY`, one hit in eight - heals its bearer while
  wounded, strikes evil targets while whole.
- Passives at artifact levels 1/3/5: detect invisibility, +3 will, 15% unholy
  resistance.
- Contract hint: "The orders that made it still test who asks for it. Answer
  the test."
- **Build**: a paladin-facing quest with an actual test - a moral choice, an
  escort, or a trial by combat against something evil. The giver should be an
  order of Tyr, Torm, or Ilmater, or the campaign equivalent. Award it through
  a quest reward, not a floor drop. Owner is public, so the chronicle will
  name the holder; the quest text should say so.

#### Earthcrier - 169914 - `ART_ACQ_BOSS`, owner secret

- Binding: **on pickup**. Oath: none, but the signature proc only fires for a
  non-good wielder (`ART_ALIGN_SELF_EVIL`).
- Signature proc: `ART_SIG_KNOCKDOWN`, one hit in eight, with save and
  immunity rules.
- Contract hint: "Something large enough to swing it two-handed is already
  carrying it."
- **Build**: `E` it onto a large, two-handed-weapon boss - giant, ogre
  chieftain, or troll king. Because it binds on pickup, the corpse loot must
  not be in a room shared with a busy public route.

#### Wyrmfang - 169915 - `ART_ACQ_BOSS`, owner public

- Binding: on equip. Oath: none.
- Signature proc: `ART_SIG_WEIGHTED`, one hit in ten.
- Called effect: `invoke hunt` - an `ART_INVOKE_COMMAND` channel, not speech.
- Passives at artifact levels 1-5 unlock one sense per level: detect
  invisibility, infravision, sense life, farsee, haste.
- Contract hint: "It was taken from a dragon once. It will have to be taken
  again."
- **Build**: `E` it onto a black dragon or a dragon-tainted boss. The hint
  promises a dragon; the placement must deliver one. This is the artifact
  whose progression rewards long-term use, so it suits a boss players can
  reach at mid-tier and grow into.

#### Courage - 169916 - `ART_ACQ_STAFF_EVENT`, owner public

- Binding: on equip. Oath: cleric.
- No signature proc. Called effect: saying `courage` while wearing it buffs
  every eligible group member in the room, one cooldown and one XP award
  regardless of group size.
- Passives at artifact levels 1-4: +2 will, 10% electric resistance, +2
  fortitude, haste.
- Contract hint: "It is given, in the open, to someone who has already earned
  it."
- **Build**: no zone reset. This one is released by hand. What the builder
  owes is the *event*: a documented staff procedure, an announcement text, and
  a named ceremony location. Use `testartifact spawn` to bring it into play at
  the event and hand it over normally. Keep the vault reset **removed** so a
  reboot cannot quietly release it.

#### Icedge - 169917 - `ART_ACQ_EXPLORATION`, owner secret

- Binding: **on account**. Oath: none.
- Signature proc: `ART_SIG_FLURRY`, one hit in six.
- Called effect: `whisper <someone> rime` - an `ART_INVOKE_WHISPER` channel.
  A dagger's power does not announce itself to the room, which suits a
  found-not-awarded artifact.
- Passives at artifact levels 1/3/5: 15% cold resistance, +4 spell resistance,
  true sight.
- Contract hint: "A cult held it. Something out of the blizzard took it, and
  took it home."
- **Build**: a clue chain, not a boss fight. The Homeland source this was
  drawn from put the lore in NPC dialogue and never loaded the object; do
  better than that. Minimum shape: a cult NPC who mentions the theft, a
  blizzard-region trail, and a lair at the end holding the dagger. The final
  container or carrier is where the `O`/`P`/`G` reset goes. Account binding
  means each account gets exactly one shot at owning it, so the chain should
  be findable rather than random.

#### Twilight - 169918 - `ART_ACQ_RECOVERY`, owner public

- Binding: **on pickup**. Oath: none.
- Signature proc: `ART_SIG_SURGE`, one hit in six, in stacking group
  `ART_STACK_COMBAT_SURGE` so it cannot stack with Doombringer's rage.
- Passives at artifact levels 2-5: infravision, sense life, farsee, haste.
- Contract hint: "It is not placed and it is not dropped. It is recovered, or
  it is lost."
- **Build**: no reset at all, in any zone. Remove its vault reset. Twilight
  exists only through `testartifact recover`, which is audited. The builder
  deliverable is the *lore* explaining why it is out of the world and the
  staff policy for when a recovery is granted.

## 5. Discovery, lore, and the chronicle

`artifact chronicle <name>` and `artifact roster` stage what a player sees:

- an undiscovered artifact shows state and lore but no name - it is a rumour;
- discovery is set the first time an artifact is legitimately claimed and is
  persisted in the v2.3 ownership file;
- the acquisition hint text comes from the contract row, not from world data.

So world content and contract text must agree. If a builder puts Icedge in a
volcano, the hint about a blizzard becomes a lie that players will follow.
Either build the blizzard chain or change the contract string.

Wiring NPC dialogue to staged lore is ordinary DG-script work against
`artifact chronicle`. That is the recommended way to seed clues: a mob greet
or speech trigger that repeats the same hint the chronicle shows.

## 6. Availability

Artifact availability is global. Placement only needs to account for the
current Luminari world and no longer has per-campaign metadata.

## 7. Acceptance criteria and how to verify them

Task 2's acceptance criteria, restated as checks a builder can run.

**Every intended artifact has a documented acquisition path.**

```
artifact roster
artifact chronicle <name>
```

Nothing may still read `Staged for release` once its mode is chosen. Compare
against the roster table in `docs/systems/ARTIFACT_SYSTEM.md`.

**Placement works.** After provisioning and a boot, for each placed artifact:

```
testartifact list
```

The artifact must show an instance in play, in the intended room or on the
intended mobile.

**Ownership never creates a duplicate instance.** This is the one that must be
tested rather than assumed:

1. Have a test character claim the artifact and log off with it.
2. Force the source zone to reset (`zreset <zone>`). Confirm with
   `testartifact list` that no second instance appeared.
3. Reboot. Confirm again after boot, and confirm the boot log has no artifact
   SYSERRs.
4. Repeat with the artifact left on the ground in a live room rather than
   carried - the guard's first branch covers this case separately from the
   durable-owner branch.
5. Repeat for an artifact with a secondary reset in a second zone, if any
   were placed.

**Boot validation stays clean.** `artifact_validate_metadata()` runs at boot
and is re-runnable:

```
testartifact verify
```

Any contract row edited during this work is checked here. A row naming a vnum
with no prototype, or an out-of-range recharge slot, is reported by name.

## 8. Source-change budget for this task

The only source edits placement should ever require:

- `artifact_contracts[]` in `src/obj/spec_artifacts.c` - the acquisition
  mode, campaign mask, owner policy, lore line, and hint line for an artifact
  whose placement was just decided;
- the roster table in `docs/systems/ARTIFACT_SYSTEM.md`;
- a changelog entry.

If placement seems to need anything else - a new effect, a new proc shape, a
new reset command type - stop and raise it, because the runtime was built to
avoid exactly that.
