# Crafted construct prototypes

These records provide the twelve material/size recipe mobiles at 19600-19611.
The corpse-based bone construct is at 19612.
Zone 196 has no resets and exists only to contain the prototypes.

From a development checkout, run:

```sh
python3 scripts/world/install_pet_constructs.py
python3 scripts/world/wtool.py validate --paths data/pet-constructs/196.mob data/pet-constructs/196.zon --strict
```

The installer checks the whole destination range before writing, refuses different
existing records (allowing additions after unchanged mobile records), and adds
the zone and mobile files to their normal indexes. A
second invocation leaves matching files unchanged. Reload the world through the
normal development server restart to make newly installed prototypes available.

`src/pet_vnums.h` defines the new recipe IDs. The customized `src/vnums.h` remains
unchanged. Legacy golem IDs are recognized for maintenance of existing pets, but
new recipes use only the dedicated range. Do not overwrite the animals occupying
the old 16500 range.

Wood has higher accuracy and Dexterity, stone has improved armor and durability
with damage reduction 5, and iron has heavier damage with damage reduction 8.
Each material has four sizes. The recipes retain their original resource costs,
class feats and crafting checks. These are permanent
construct followers under the ordinary golem allowance, with zero gold and no
scripted spawning or resets.

`craft create golem animate <corpse>` binds a medium bone golem using the native
corpse eligibility checks, one-golem allowance, 40 bone, and six of each mote.
It uses standard/move actions and an Arcana DC 25 check. Construct Wood Golem or
Summon Greater Undead grants access. Failure spends reagents but retains the
corpse; success transfers loot and consumes it once. The construct has DR 3,
uses bone for repair, and yields 20 bone once on dismantling. Gear must be
retrieved before dismantling. See the BONE-GOLEM help entry for the full rules.
