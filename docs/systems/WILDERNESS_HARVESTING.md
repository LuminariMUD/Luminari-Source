# Wilderness category harvesting

Issue: https://github.com/LuminariMUD/Luminari-Source/issues/145

`harvest <category>` uses wilderness terrain, coordinate resource levels and
regenerating depletion to produce usable crafting rewards. `harvest` lists the
categories available at the current location. Each attempt occupies one full
round (six seconds) and awards its result at completion. Movement, damage,
combat, loss of eligibility, or `activity cancel` ends the attempt without a
reward. Commands cannot start overlapping activities. The same rules apply to
`gather` and `mine` for their supported categories.

An explicitly named legacy object node retains its existing command path. The
separate room-node crafting system also retains its existing behavior. `search`
is reserved and has no harvesting behavior or dependency.

## Configuration

Set `WILDERNESS_HARVEST_CRAFTING=TRUE` in `lib/.env`. The default is true when
omitted. The setting is read when a harvest starts and rechecked at completion;
disabling it cancels any pending category harvest at its next check.

`FALSE` restores the earlier immediate wilderness-material storage behavior.
It does not change node spawning or node rewards. Credentials and other local
configuration stay in their existing ignored files.

The maintained example is `lib/.env_example`, as used by deployment scripts.
`lib/.env.example` is a symlink to that same template, so either spelling gives
the same default without maintaining duplicate configuration.

## Quality and rewards

The wilderness quality roll uses the relevant harvesting ability rank, its
proficient talent bonus, the success roll, and the existing Miner racial bonus
where applicable. It reads harvesting abilities from crafting's ability storage,
not the unrelated ordinary skill-spell array. Mining handles minerals, crystal,
stone, clay and salt; Forestry handles wood; Hunting handles game; Gathering
handles vegetation, herbs and water.

The following table maps each successful material harvest to existing crafting
material IDs. All subtypes use the same row except the two ore exceptions below.
Wilderness qualities and crafting grades are separate concepts; the selected
materials have at least the corresponding grade, so a tool floor survives payout.

| Category | Poor (1) | Common (2) | Uncommon (3) | Rare (4) | Legendary (5) |
| --- | --- | --- | --- | --- | --- |
| vegetation | hemp | flax | wool | silk | satin |
| minerals | tin | bronze | iron | steel | mithril |
| wood | ash | maple | mahagony | valenwood | ironwood |
| game | low-grade hide | medium-grade hide | high-grade hide | pristine hide | dragonscale |

Legendary adamantine ore produces adamantine, and rare cold-iron ore produces
cold iron. Both exceptions preserve the material grade floor. These rewards
credit `GET_CRAFT_MAT`, the same balances consumed by existing crafting recipes.

The other categories have no full set of equivalent graded crafting materials.
They supply crafting motes directly, preserving quality as a useful yield bonus:
each raw unit yields 1, 2, 3, 4 or 5 motes for Poor through Legendary quality.
This avoids awarding unusable wilderness storage entries or turning water into
an unrelated metal.

| Category | Crafting mote |
| --- | --- |
| water (all subtypes) | water |
| herbs (all subtypes) | light |
| stone and clay (all subtypes) | earth |
| salt (all subtypes) | ice |
| crystal: arcanite | air |
| crystal: nethermote, voidshards | dark |
| crystal: sunstone, bloodstone | fire |
| crystal: dreamquartz | light |
| crystal: frostgem | ice |
| crystal: stormcrystal | lightning |

Motes credit `GET_CRAFT_MOTES`, which existing equipment, instrument and other
crafting recipes consume. A harvest credits one primary balance only. Overflow
or a failed payout awards neither experience nor resource depletion. No second
copy is placed in the old wilderness `stored_materials` inventory.

Successful attempts yield 2-4 raw units, with an extra 2-4 on a natural 100.
The efficient talent can add two units. Material harvests can also award the
node-style random bonus motes; primary mote harvests already include their
quality bonus. The harvesting ability gains experience using `gain_craft_exp`,
including its existing insightful talent handling. Failed rolls earn the small
failure experience award and deplete one raw unit. Successful depletion,
cascades and conservation use actual raw units harvested, not the multiplied
mote count.

## Harvest tools

The authored object prototypes are in
[data/harvest-tools/harvest-tools.obj](../../data/harvest-tools/harvest-tools.obj).
VNUM definitions live in `src/harvest_vnums.h`, which is included by the example
VNUM configuration. Existing customized `src/vnums.h` files need no edits.

| VNUM | Name | Minimum quality |
| --- | --- | --- |
| 1251 | poor harvest tool | Poor |
| 1252 | common harvest tool | Common |
| 1253 | uncommon harvest tool | Uncommon |
| 1254 | rare harvest tool | Rare |
| 1255 | legendary harvest tool | Legendary |

The highest qualifying VNUM in top-level inventory or any equipment slot sets
`max(rolled_quality, tool_quality)` at completion. Names alone do not identify a
tool. Tools inside containers must be taken out. Tools are retained after use,
do not stack, and do not grant success or bypass terrain and depletion checks.
A tool removed before completion supplies no benefit.

## Delivery and verification

Install the five prototypes into the site's world data as described in
[data/harvest-tools/README.md](../../data/harvest-tools/README.md).
Apply `sql/components/help_wilderness_harvest.sql` to the target help database
alongside `lib/text/help/help.hlp`. The migration updates the harvesting entries
and their command aliases; it preserves the unrelated help entries themselves.

Production-linked tests in `unittests/CuTest/test_gameplay_e2e.c` cover command
dispatch, delayed payout, the tool floors, rollback, material mapping, overflow,
and interrupted work. Run the normal `make test` gate and follow with
`make install`. World content validation:

```sh
python3 scripts/world/wtool.py validate --paths data/harvest-tools/harvest-tools.obj --strict
python3 scripts/ci/check_build_parity.py
```
