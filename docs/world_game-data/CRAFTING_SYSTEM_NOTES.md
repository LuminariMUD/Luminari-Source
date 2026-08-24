# LuminariMUD Crafting System Notes

> **Status: design notes, not a system reference.** This file records the
> original design intent of the crafting system - the material/skill mapping,
> the weapon resize ladder, and the command list. It is not a complete
> description of current behavior, and it has never covered the newer crafting
> paths (`ITEM_CRAFTING_TOOL` gear, the crafting station flags, or the harvest
> ability set). Verify anything here against `src/craft/` before relying on it.
>
> The skill numbers below were re-derived from `src/magic/spells.h` on
> 2026-08-04. They previously listed the pre-`START_SKILLS` values (471-485),
> which have not been correct since skills moved to the 2000 base.

## Overview

The LuminariMUD crafting system provides item creation and enhancement
capabilities based on D&D 3.5/Pathfinder mechanics. Players gather materials,
learn crafting skills, and create custom equipment.

## Known Gaps

* No complete documentation for all crafting commands.
* No worked examples for each crafting skill.
* Material acquisition methods are undocumented.
* The crafting station flags (`ITEM_CRAFTING_FORGE`, `ITEM_CRAFTING_LOOM`, and
  the rest - see the [OEDIT Guide](OEDIT_GUIDE.md)) are not covered here at all.
* The nine crafting-tool wear slots are reserved but currently unused; no
  active `ITEM_CRAFTING_TOOL` prototypes exist. See the
  [OEDIT Guide](OEDIT_GUIDE.md#wear-flags-reference) for their assigned bits.

## Weapon Resize Chart

```
F      D      T       S     M      L       H      G      C      ?      ??
1d2 -> 1d3 -> 1d4 -> 1d6 -> 1d8 -> 2d6  -> 3d6 -> 4d6 -> 6d6 -> 8d6 -> 12d6
1d1 -> 2d1 -> 2d3 -> 1d7 -> 2d4 -> 1d12 -> 4d4 -> 6d4 -> 5d8 -> 6d8 -> 8d10
       3d1 -> 2d2 -> 3d2 -> 1d9 -> 1d10 -> 2d8 -> 3d8 -> 4d8 -> 8d7 -> 9d8
```

## Materials and Skills

| Material class | Gathering skill |
|----------------|-----------------|
| Hard metal | Mining |
| Leather | Hunting |
| Wood | Foresting |
| Cloth | Knitting |
| Crystals / Essences | Chemistry |

### Gathering Skills

| Constant | Number | Purpose |
|----------|--------|---------|
| `SKILL_MINING` | 2071 | Acquiring hard metals |
| `SKILL_HUNTING` | 2072 | Acquiring leather, dragonhide |
| `SKILL_FORESTING` | 2073 | Acquiring wood, darkwood |
| `SKILL_KNITTING` | 2074 | Acquiring cloth, creating cloth armor |
| `SKILL_CHEMISTRY` | 2075 | Processing crystal, essences |

### Production Skills

| Constant | Number | Purpose |
|----------|--------|---------|
| `SKILL_ARMOR_SMITHING` | 2076 | Creating metal armor |
| `SKILL_WEAPON_SMITHING` | 2077 | Creating weapons |
| `SKILL_JEWELRY_MAKING` | 2078 | Creating miscellaneous worn pieces |
| `SKILL_LEATHER_WORKING` | 2079 | Creating non-metal armor |
| `SKILL_FAST_CRAFTER` | 2080 | Increases speed of all crafting events |

### Specialization Skills

| Constant | Number | Purpose |
|----------|--------|---------|
| `SKILL_BONE_ARMOR` | 2081 | Create metal-equivalent armor using bone |
| `SKILL_ELVEN_CRAFTING` | 2082 | Produce lighter armor |
| `SKILL_MASTERWORK_CRAFTING` | 2083 | Higher chance to produce rare, legendary, or mythic results |
| `SKILL_DRACONIC_CRAFTING` | 2084 | Higher bonus without affecting level (design intent; verify) |
| `SKILL_DWARVEN_CRAFTING` | 2085 | Craft using rare heavy metals |

## Material List

**Hard metals:** bronze, iron, steel, cold iron, alchemical silver, mithril,
adamantine

**Precious metals:** copper, brass, silver, gold, platinum

**Leathers:** leather, dragonhide

**Woods:** wood, darkwood

**Cloth:** burlap, hemp, cotton, wool, velvet, satin, silk

## Crafting Commands

All of the following are registered in `cmd_info[]` (`src/interpreter.c`).

| Command | Effect |
|---------|--------|
| `create` | Create/craft an object using materials and skills |
| `checkcraft` | Check the result the `create` command would produce |
| `augment` | Combine essences to make them stronger |
| `convert` | Convert ten of one material into something else |
| `disenchant` | Create essence from magical items |
| `resize` | Resize an object for a different character size |
| `restring` | Rename an object (cosmetic only) |
| `autocraft` | Crafting quest system; supply orders |

### Usage Examples

```
create sword steel
augment essence fire essence ice
resize sword large
restring sword "a gleaming steel blade"
```
