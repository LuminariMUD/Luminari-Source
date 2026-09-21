# Craft Training and the Apprentice System: Discrepancies and Findings

Written 2026-09-20 following player reports regarding the `apprentice` command.

Status: resolved by the crafting consolidation (issue #212,
[crafting-consolidation-assessment.md](crafting-consolidation-assessment.md)).
`craft`, `craftscore`, and `apprentice` now read one set of ranks, the legacy
skills converted once per character (CrMg stage 1), and the `craft` mode
selector is gone. The analysis below is kept as the record of the report.

## Overview

Following the deployment of the Craft Trainer system (\[`src/craft/craft_training.c`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/craft/craft_training.c)), players noticed two discrepancies when comparing the output of the `apprentice` command with their existing crafting list in `craft`:

1. **Naming Discrepancies:** The skill names listed at the master artisan trainer differ from the skill names shown under `craft` (e.g., `tailoring` vs. `knitting`, `alchemy` vs. `chemistry`, `forestry` vs. `foresting`, compound words like `armorsmithing` vs. `armor smithing`).
2. **Rank 0 Display:** The trainer lists all skills at Rank 0, despite veteran players already having advanced ranks in those crafts (e.g., mining 9, foresting 10, knitting 10).

This document analyzes the root causes in the codebase, the mechanics behind each system, and the gameplay impact.

---

## Root Cause: Two Independent Crafting Subsystems

LuminariMUD currently contains two distinct crafting architectures that use separate data structures, persistence tags, and command handlers:

### 1. The Legacy / Kit Crafting System

- **Implementation:** \[`src/craft/craft.c`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/craft/craft.c), \[`src/character/skill_lists.c`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/character/skill_lists.c).
- **Storage:** Stored in the character's skill array (`GET_SKILL(ch, skill)`), persisted in player files under the `Skil` tag.
- **Skill Range:** \[`SKILL_MINING`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/magic/spells.h#L1185) (2071) through \[`SKILL_FAST_CRAFTER`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/magic/spells.h#L1194) (2080).
- **Display Command:** When a character executes `craft` under `CRAFTING_SYSTEM_KITS`, \[`do_craft()`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/craft/crafts.c#L802-L816) calls \[`do_practice()`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/act/act.other.c#L7401-L7422), which invokes \[`list_crafting_skills()`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/character/skill_lists.c#L441-L466) to print the legacy skills.

### 2. The Modern Materials-and-Motes Crafting System

- **Implementation:** \[`src/craft/crafting_new.c`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/craft/crafting_new.c), \[`src/craft/craft_training.c`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/craft/craft_training.c).
- **Storage:** Stored in the character's ability array (`GET_ABILITY(ch, ability)`) and experience array (`GET_CRAFT_SKILL_EXP(ch, ability)`), persisted under the `Abil` and `AbEx` tags.
- **Ability Range:** \[`START_CRAFT_ABILITIES`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/magic/spells.h#L1594) (34) through \[`END_HARVEST_ABILITIES`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/magic/spells.h#L1621) (51).
- **Display Command:** Displayed by \[`show_craft_score()`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/craft/crafting_new.c#L4324-L4369) via `craftscore` (when configured in motes mode) or by \[`craft_training_list()`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/craft/craft_training.c#L82-L103) when using `apprentice` at a craft trainer.

---

## Comparison of Skill Names

The skill names displayed by `apprentice` are populated from \[`ability_names[]`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/core/constants.c#L3170-L3182) in \[`src/core/constants.c`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/core/constants.c), whereas `craft` displays the names registered in \[`spell_info[]`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/magic/spell_parser.c#L6813-L6822) in \[`src/magic/spell_parser.c`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/magic/spell_parser.c).

| Legacy Skill Name (`craft`) | Skill Number | Modern Ability Name (`apprentice`) | Ability Number | Notes |
| :- | :- | :- | :- | :- |
| `knitting` | 2074 | `tailoring` | 35 | Renamed to reflect general garment and cloth crafting |
| `chemistry` | 2075 | `alchemy` | 36 | Renamed to match fantasy setting and alchemical crafting |
| `foresting` | 2073 | `forestry` | 50 | Modernized naming |
| `jewelry making` | 2078 | `jewelcrafting` | 40 | Modernized single-word title |
| `armor smithing` | 2076 | `armorsmithing` | 37 | Formatted as a single compound word |
| `weapon smithing` | 2077 | `weaponsmithing` | 38 | Formatted as a single compound word |
| `leather working` | 2079 | `leatherworking` | 41 | Formatted as a single compound word |
| `mining` | 2071 | `mining` | 48 | Identical name |
| `hunting` | 2072 | `hunting` | 49 | Identical name |
| *(None)* | - | `woodworking` | 34 | New craft track in modern system |
| *(None)* | - | `metalworking` | 44 | New craft track in modern system |
| *(None)* | - | `gathering` | 51 | New harvest track in modern system |
| `fast crafter` | 2080 | *(None)* | - | Crafting speed bonus handled via talents in modern system |

---

## Why Ranks Appear as 0 in `apprentice`

1. **Independent State:** The player's crafting progress earned under the legacy system was saved into the character's `GET_SKILL` values. The modern system reads `GET_ABILITY(ch, ability)`. Because the character has never earned experience or trained in the new ability tracks, their base ability rank is 0.
2. **Experience-Driven Leveling:** In the modern system, ranks advance according to cumulative experience stored in `GET_CRAFT_SKILL_EXP(ch, ability)` via \[`gain_craft_exp()`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/craft/crafting_new.c#L5050-L5090). Each new rank requires an increasing amount of experience (e.g., Rank 0 -> Rank 1 requires 1,000 EXP). With 0 EXP recorded, the player is evaluated at Rank 0.

---

## Behavioral and Gameplay Impact

### 1. Training with `apprentice` Does Not Affect Legacy Skills

If a player enters a contract (`apprentice <skill> confirm`), the following occurs:

- The player pays the fee and is logged out for six hours.
- Upon returning, \[`gain_craft_exp()`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/craft/crafting_new.c#L5050-L5090) awards 500 craft experience in the modern ability (e.g., `ABILITY_HARVEST_MINING`), progressing them halfway to Rank 1.
- **No legacy skills are modified.** The player's legacy `mining` skill (Rank 9 in `craft`) remains at 9.

### 2. Runtime Configuration (`CONFIG_CRAFTING_SYSTEM`)

- In `lib/etc/config`, the runtime option is set to `crafting_system = 1` (`CRAFTING_SYSTEM_KITS`).
- Because the server runs in kits mode:
  - The `craft` command routes to \[`do_practice()`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/act/act.other.c#L7401-L7422) and only displays the legacy skills.
  - The `craftscore` command also routes to \[`do_practice()`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/act/act.other.c#L7401-L7422).
  - Consequently, players training through `apprentice` cannot inspect their modern ability ranks or experience using `craft` or `craftscore`. They can only view their ranks when standing at the craft trainer (`apprentice`) or interacting with `newcraft` and `supplyorder`.

---

## Recommendations and Options

1. **Helpfile and Communication Clarification:**
   Update the `APPRENTICE` and `CRAFT` helpfiles to explain that `apprentice` trains modern crafting abilities used in `newcraft`, `craftmaterials`, and `supplyorder`, rather than the legacy kit skills.
2. **Decouple `craftscore` from `CONFIG_CRAFTING_SYSTEM`:**
   In \[`src/craft/crafting_new.c`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/craft/crafting_new.c#L6656-L6670), \[`do_craft_score()`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/craft/crafting_new.c#L6656-L6670) currently redirects to `do_practice` when `crafting_system = 1`. Updating `do_craft_score` to always execute \[`do_craft_score_new()`\](file:///home/aiwithapex/projects/Luminari-Source-issue-208/src/craft/crafting_new.c#L6672-L6675) would allow players to view their modern ability ranks, experience, and talent points at any time regardless of the global kit/mote switch.
3. **Transition / Migration Plan:**
   If the server is transitioning away from the legacy kit crafting system to the modern materials-and-motes architecture, consider a one-time migration pass to initialize modern ability experience (`GET_CRAFT_SKILL_EXP`) based on a character's legacy skill ranks (`GET_SKILL`), preventing veteran crafters from having their prior progression separated from the new features.
