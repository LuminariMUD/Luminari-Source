# OEDIT Guide for Builders

## Getting Started
- Enter the editor with `oedit <vnum>` to modify an existing object or `oedit create <vnum>` for a fresh prototype.
- Work is buffered in memory until you quit with `Q` followed by `Y`. Choosing `N` abandons your changes.
- Menus accept single‐character choices. Numeric submenus expect plain numbers (type `10`, not `a`).
- String entries use the standard line editor (`~` on a blank line finishes multiline input).
- All edits target the prototype; mobile/program reloads pick up the new values after you `Q`+`Y` or `redit save` the zone.

## Main Menu Snapshot
```
-- Item number : [####]
1) Keywords               H) Material           T) Spellbook menu
2) Short description      8) Weight             EQ Rating ...
3) Long description       I) Size               Suggested affects ...
4) Action description     9) Cost               W) Copy object
5) Type                   A) Cost/Day           X) Delete object
6) Extra flags            B) Timer              Q) Quit
7) Wear flags             C) Values             
                          D) Applies menu       F) Weapon Spells
                          E) Extra descriptions J) Special Abilities
                          K) Activated Spells   M) Minimum Level
                          P) Permanent affects  R) Mob Recipient
                          S) Script             
```

### Text Fields (1–4)
- `1) Keywords` populate the name list used by `get`/`oload`.
- `2) Short description` shows in room/inventory lists.
- `3) Long description` is what appears on the ground.
- `4) Action description` is optional flavor for `look <obj>` or context actions.

### Object Identity
- `5) Type` drives downstream menus. Set this first after clearing values; many other prompts depend on it.
- `6) Extra flags` toggles general item bitflags (`glow`, `magic`, `nodrop`, etc.). Enter the flag number to toggle, `0` to return.
- `7) Wear flags` toggles where the item can be equipped. The editor resets all wear bits when you pick a weapon type, so revisit after `5)` if needed.
- `H) Material` selects from the global material list (0 = undefined). Material influences repair, crafting, and auto-generated descriptions.
- `I) Size` maps to the size checks used for wielding and containers.
- `R) Mob Recipient` restricts who can receive the item via `give`. Enter the NPC vnum or `0` to allow any NPC; players may always trade it.

### Economy & Lifespan
- `8) Weight` and `9) Cost` are raw integers; weight is in pounds.
- `A) Cost/Day` sets rent/upkeep.
- `B) Timer` controls decay. `0` means the item never times out; positive values count down in mud pulses.
- `M) Minimum Level` enforces a baseline usage level and feeds suggested enhancement output.

### Derived Power
- `P) Permanent affects` toggles permanent `affected_by` flags (e.g., `SANCTUARY`). Use these sparingly; they behave like always-on spell affects.
- `D) Applies menu` manages up to `MAX_OBJ_AFFECT` (5) slot entries.
  - Choose a slot (1–5) to edit.
  - Select the apply from the list (`STR`, `DEX`, `AC`, `FEAT`, etc.). Applies that require a specific school/skill prompt for a follow-up selection.
  - Enter the modifier and pick a bonus type (stacking rules). A value of `0/None` clears the slot.
- The main menu shows an **EQ Rating** and **Suggested affects** after you save and re-enter; use them as guidance, not hard rules.

### Descriptions & Scripts
- `E) Extra descriptions` lets you add keyword-triggered flavor text. Each entry needs both keywords and a description, otherwise it will be discarded when you exit the submenu.
- `S) Script` jumps into the DG Script editor. Note that scripting is disabled when editing a live instance via `iedit`.

## Editing Values (C)
Selecting `C` zeroes all 16 value slots before prompting. The sequence of prompts varies by item type; set the type first so the correct path is taken. Below are the most common cases:

### Weapons (`ITEM_WEAPON`)
- Picking the weapon type auto-populates dice, cost, weight, material, size, and wear slots from `weapon_list`.
- After the type is chosen you go straight to the enhancement prompt (`Value5`).
- You can revisit `C` if you need to tweak damage dice (`Value2/Value3`) or attack type (`Value4`); be aware that re-running `C` resets enhancements.

### Armor (`ITEM_ARMOR` and `ITEM_CLANARMOR`)
- Selecting the armor subtype (`Value2`) auto-fills AC, size, material, and wear flags via `set_armor_object`.
- Enhancement bonus is collected at `Value5` (0–10). Clan armor prompts for clan ID at `Value2`.

### Ammunition & Ranged
- `ITEM_FIREWEAPON`: `Value1` sets damage dice, `Value3` sets break chance (2–98), `Value5` is enhancement.
- `ITEM_MISSILE`: `Value0` picks ammo category, `Value3` sets break chance, `Value5` handles enhancement.
- `ITEM_AMMO_POUCH` / `ITEM_CONTAINER`: `Value0` capacity (lbs; `-1` unlimited). `Value1` opens a flag toggle for closeable/lockable bits; enter `0` when done. `Value2` key vnum (`-1` none).

### Consumables
- `ITEM_POTION` / `ITEM_SCROLL`: `Value0` spell level. `Value1–3` choose spells (use `-1` or `0` to clear a slot). `Value4` accepts an optional third spell.
- `ITEM_WAND` / `ITEM_STAFF`: `Value0` spell level, `Value1` max charges, `Value2` current charges, `Value3` spell to cast. A value of `-1` clears a spell slot.
- `ITEM_POISON`: prompts for poison spell, level, applications, and hits per application across the first four values.

### Lights & Miscellaneous
- `ITEM_LIGHT`: jump straight to `Value3` for burn hours (`-1` = infinite, `0` = burnt out).
- `ITEM_DRINKCON` / `ITEM_FOUNTAIN`: values cover capacity, remaining units, and liquid type.
- `ITEM_PORTAL`: values depend on portal mode; the prompts call out the required vnums or ranges.
- `ITEM_TREASURE_CHEST`: values cover loot tier, guaranteed loot type, random chest flag, search DC, pick DC, and optional trap type.
- `ITEM_GEAR_OUTFIT`: values configure preset gear bundles—type, enhancement, material, applies, and bonus types.

For unusual object types, read the prompts carefully; each menu is sourced from `src/oedit.c` and mirrors the in-game expectations.

## Combat Enhancements
- `F) Weapon Spells` manages up to `MAX_WEAPON_SPELLS` (4) procs.
  - Choose the slot number. Enter `-1` at the spell list to clear the slot.
  - Set the cast level (≥1), the chance to proc (`1–50`%), then whether it fires only in combat (`1` offensive) or defensively (`0`).
- `J) Special Abilities` attaches structured weapon spec-abilities.
  - `N` creates a new ability; `E` selects an existing one by position; `C` removes all.
  - For each ability choose the type, minimum level (1–34), activation methods (toggle numbers; `0` finishes), optional command word, and ability-specific values (e.g., Bane race/subrace).
- `K) Activated Spells` configures a charged use effect (wands, staves, oddities).
  - Enter `0` at the level prompt to remove the activation.
  - Valid levels are 1–30; spells are chosen from the filtered list that follows.
  - Uses must be between 1 and `MAX_NUMBER_OF_ACTIVATED_SPELL_USES` (5). Items regenerate one use every five minutes in-game.

## Spellbooks (`T`)
- Spellbooks can store up to `SPELLBOOK_SIZE` entries (200). The prompt shows existing spells grouped by circle and the next empty slot.
- Selecting a slot opens the spell-picker. Enter `0` to clear the slot.
- Pages are assigned automatically to at least 1 and default to half of the spell’s minimum wizard level (integer division). Builders cannot override pages directly in OEDIT.

## Copying, Deleting, and Saving
- `W) Copy object` clones an existing prototype into the current buffer (useful for variants). You still need to `Q`+`Y` to persist the changes.
- `X) Delete object` permanently removes the prototype after confirmation; instances already in the world remain until extracted.
- `Q) Quit` prompts to save. Respond with `Y` to write to disk (and update live prototypes) or `N` to discard.

## Tips & Verification
- Keep object levels, enhancement bonuses, and pricing consistent; rely on `EQ Rating` and the suggested bonus hints but always sanity-check against existing loot.
- After major edits, `oload <vnum>` in a controlled area and `stat obj` to verify wear flags, applies, and scripts.
- Use `plist`/`olist` to ensure numbering stays within zone limits and to spot accidental duplicates.
- For DG scripts, coordinate with an implementor when attaching complex triggers.
- Cross-reference `src/oedit.c`, `src/treasure.c`, and `constants.c` for the definitive lists of types, flags, and ability IDs.

Happy building!
