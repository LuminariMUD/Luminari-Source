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
- `6) Extra flags` toggles general item bitflags (`glow`, `magic`, `nodrop`, etc.). Enter the flag number to toggle, `0` to return. See the [Extra Flags Reference](#extra-flags-reference) below for the complete list.
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

## Extra Flags Reference
When you select `6) Extra flags` from the main menu, you can toggle any of the 114 available item flags by entering their number. Enter `0` to return to the main menu. Below is the complete list:

### Display & Visual Effects
| # | Flag Name | Code Constant | Description |
|---|-----------|---------------|-------------|
| 0 | Glows | ITEM_GLOW | Contributes +1 to room light level and allows object visibility in darkness, preventing total concealment from invisibility |
| 1 | Hums | ITEM_HUM | Display-only cosmetic flag showing humming sound descriptor when examining object; no gameplay mechanics |
| 5 | Invisible | ITEM_INVISIBLE | Makes item invisible to normal sight; only visible to characters with detect invisibility or true sight abilities |
| 6 | Magical | ITEM_MAGIC | Weapons bypass magical damage reduction (DR/magic), allowing harm to creatures immune to non-magical weapons; visible under detect magic |
| 38 | Floating | ITEM_FLOAT | Prevents object from falling when dropped in fly-required rooms; floats gracefully in mid-air instead of falling |
| 39 | Hidden | ITEM_HIDDEN | Item requires successful perception check (search command) to discover; partially implemented with treasure chest system |
| 40 | Magical-Light | ITEM_MAGLIGHT | Provides +1 magical light to rooms (both equipped and in inventory), improving visibility in dark areas independent of traditional light sources |
| 45 | Flaming | ITEM_FLAMING | Toggleable weapon ability dealing 1d6 fire damage on each hit when activated; wielder must toggle on/off |
| 46 | Frosty | ITEM_FROST | Toggleable weapon ability dealing 1d6 cold damage on each hit when activated; wielder must toggle on/off |
| 92 | Shocking Weapon | ITEM_SHOCK | Toggleable weapon ability dealing 1d6 electrical damage on each hit when activated; wielder must toggle on/off |

### Item Handling & Economy
| # | Flag Name | Code Constant | Description |
|---|-----------|---------------|-------------|
| 2 | Not-Rentable | ITEM_NORENT | Prevents item from being rented at rental facilities; items automatically extracted when character attempts to rent |
| 3 | Not-Donateable | ITEM_NODONATE | Prevents donation to donation rooms; if donated, item converts to junk and is disposed of |
| 4 | Immune-Invis | ITEM_NOINVIS | Prevents invisibility spell from being cast on item, protecting it from magical concealment effects |
| 7 | Not-Droppable | ITEM_NODROP | Cursed item that cannot be dropped, transferred, or removed from inventory except by staff with NOHASSLE mode; confers flag to containers it's placed in |
| 16 | Not-Sellable | ITEM_NOSELL | Prevents shopkeepers from purchasing item and blocks sell-related transactions at vendor shops |
| 17 | Quest-Item | ITEM_QUEST | Marks item as quest item with special shop handling; costs quest points instead of gold when purchased, displays as "qp" in shop listings |
| 41 | No-Locate | ITEM_NOLOCATE | Prevents item from being found by locate object and locate creature spells (defined but not actively enforced) |
| 42 | No-Burn | ITEM_NOBURN | Prevents item from being destroyed by disintegration spells; protects from destructive magical effects (defined but not fully implemented) |
| 43 | Transient | ITEM_TRANSIENT | Item crumbles and fades when dropped; drop dissipation mechanic currently defined but not fully implemented |
| 100 | No-Sacrifice | ITEM_NOSAC | Prevents item from being sacrificed to deities or salvaged for crafting materials; blocks sacrifice and salvage commands with explicit prevention messages |

### Alignment Restrictions
| # | Flag Name | Code Constant | Description |
|---|-----------|---------------|-------------|
| 9 | Anti-Good | ITEM_ANTI_GOOD | Prevents good-aligned characters from equipping; item fumbles to inventory with message "You try to use [item], but fumble it and let go" (immortals bypass) |
| 10 | Anti-Evil | ITEM_ANTI_EVIL | Prevents evil-aligned characters from equipping; item fumbles to inventory with message "You try to use [item], but fumble it and let go" (immortals bypass) |
| 11 | Anti-Neutral | ITEM_ANTI_NEUTRAL | Prevents neutral-aligned characters from equipping; item fumbles to inventory with message "You try to use [item], but fumble it and let go" (immortals bypass) |
| 60 | Anti-Lawful | ITEM_ANTI_LAWFUL | Prevents lawful-aligned characters from equipping; item fumbles to inventory with message "You try to use [item], but fumble it and let go" (immortals bypass) |
| 61 | Anti-Chaotic | ITEM_ANTI_CHAOTIC | Prevents chaotic-aligned characters from equipping; item fumbles to inventory with message "You try to use [item], but fumble it and let go" (immortals bypass) |

### Class Restrictions - Anti
**NOTE:** Class Anti flags are currently DISABLED for equipment (incompatibility with homeland zones) but still functional for portal entry restrictions.

| # | Flag Name | Code Constant | Description |
|---|-----------|---------------|-------------|
| 12 | Anti-Wizard | ITEM_ANTI_WIZARD | Would prevent Wizards from equipping if enabled; currently only blocks portal entry for Wizards |
| 13 | Anti-Cleric | ITEM_ANTI_CLERIC | Would prevent Clerics from equipping if enabled; currently only blocks portal entry for Clerics |
| 14 | Anti-Rogue | ITEM_ANTI_ROGUE | Would prevent Rogues from equipping if enabled; currently only blocks portal entry for Rogues |
| 15 | Anti-Warrior | ITEM_ANTI_WARRIOR | Would prevent Warriors from equipping if enabled; currently only blocks portal entry for Warriors |
| 22 | Anti-Monk | ITEM_ANTI_MONK | Would prevent Monks from equipping if enabled; currently only blocks portal entry for Monks |
| 23 | Anti-Druid | ITEM_ANTI_DRUID | Would prevent Druids from equipping if enabled; currently only blocks portal entry for Druids |
| 30 | Anti-Berserker | ITEM_ANTI_BERSERKER | Would prevent Berserkers from equipping if enabled; currently only blocks portal entry for Berserkers |
| 32 | Anti-Sorcerer | ITEM_ANTI_SORCERER | Would prevent Sorcerers from equipping if enabled; currently only blocks portal entry for Sorcerers |
| 34 | Anti-Paladin | ITEM_ANTI_PALADIN | Would prevent Paladins from equipping if enabled; currently only blocks portal entry for Paladins |
| 35 | Anti-Ranger | ITEM_ANTI_RANGER | Would prevent Rangers from equipping if enabled; currently only blocks portal entry for Rangers |
| 36 | Anti-Bard | ITEM_ANTI_BARD | Would prevent Bards from equipping if enabled; currently only blocks portal entry for Bards |
| 48 | Anti-WeaponMaster | ITEM_ANTI_WEAPONMASTER | Would prevent Weaponmasters from equipping if enabled; currently only blocks portal entry for Weaponmasters |
| 83 | Anti-Arcane-Archer | ITEM_ANTI_ARCANE_ARCHER | Would prevent Arcane Archers from equipping if enabled; currently only blocks portal entry for Arcane Archers |
| 84 | Anti-Stalwart-Defender | ITEM_ANTI_STALWART_DEFENDER | Would prevent Stalwart Defenders from equipping if enabled; currently only blocks portal entry for Stalwart Defenders |
| 85 | Anti-Shifter | ITEM_ANTI_SHIFTER | Would prevent Shifters from equipping if enabled; currently only blocks portal entry for Shifters |
| 86 | Anti-Duelist | ITEM_ANTI_DUELIST | Would prevent Duelists from equipping if enabled; currently only blocks portal entry for Duelists |
| 87 | Anti-Mystic-Theurge | ITEM_ANTI_MYSTIC_THEURGE | Would prevent Mystic Theurges from equipping if enabled; currently only blocks portal entry for Mystic Theurges |
| 88 | Anti-Alchemist | ITEM_ANTI_ALCHEMIST | Would prevent Alchemists from equipping if enabled; currently only blocks portal entry for Alchemists |
| 89 | Anti-Arcane-Shadow | ITEM_ANTI_ARCANE_SHADOW | Would prevent Arcane Shadows from equipping if enabled; currently only blocks portal entry for Arcane Shadows |
| 90 | Anti-Sacred-Fist | ITEM_ANTI_SACRED_FIST | Would prevent Sacred Fists from equipping if enabled; currently only blocks portal entry for Sacred Fists |
| 91 | Anti-Eldritch-Knight | ITEM_ANTI_ELDRITCH_KNIGHT | Would prevent Eldritch Knights from equipping if enabled; currently only blocks portal entry for Eldritch Knights |
| 97 | Anti-Warlock | ITEM_ANTI_WARLOCK | Would prevent Warlocks from equipping if enabled; currently only blocks portal entry for Warlocks |

### Class Restrictions - Required
**NOTE:** Required flags are ACTIVE and enforced; characters without the specified class levels cannot equip these items.

| # | Flag Name | Code Constant | Description |
|---|-----------|---------------|-------------|
| 62 | Wizard-Required | ITEM_REQ_WIZARD | Character must have Wizard levels to equip; blocks with message "You must have levels as a wizard to use [item]" |
| 63 | Cleric-Required | ITEM_REQ_CLERIC | Character must have Cleric levels to equip; blocks with message "You must have levels as a cleric to use [item]" |
| 64 | Rogue-Required | ITEM_REQ_ROGUE | Character must have Rogue levels to equip; blocks with message "You must have levels as a rogue to use [item]" |
| 65 | Warrior-Required | ITEM_REQ_WARRIOR | Character must have Warrior levels to equip; blocks with message "You must have levels as a warrior to use [item]" |
| 66 | Monk-Required | ITEM_REQ_MONK | Character must have Monk levels to equip; blocks with message "You must have levels as a monk to use [item]" |
| 67 | Druid-Required | ITEM_REQ_DRUID | Character must have Druid levels to equip; blocks with message "You must have levels as a druid to use [item]" |
| 68 | Berserker-Required | ITEM_REQ_BERSERKER | Character must have Berserker levels to equip; blocks with message "You must have levels as a berserker to use [item]" |
| 69 | Sorcerer-Required | ITEM_REQ_SORCERER | Character must have Sorcerer levels to equip; blocks with message "You must have levels as a sorcerer to use [item]" |
| 70 | Paladin-Required | ITEM_REQ_PALADIN | Character must have Paladin levels to equip; blocks with message "You must have levels as a paladin to use [item]" |
| 71 | Ranger-Required | ITEM_REQ_RANGER | Character must have Ranger levels to equip; blocks with message "You must have levels as a ranger to use [item]" |
| 72 | Bard-Required | ITEM_REQ_BARD | Character must have Bard levels to equip; blocks with message "You must have levels as a bard to use [item]" |
| 73 | Weaponmaster-Required | ITEM_REQ_WEAPONMASTER | Character must have Weaponmaster levels to equip; blocks with message "You must have levels as a weaponmaster to use [item]" |
| 74 | Arcane-Archer-Required | ITEM_REQ_ARCANE_ARCHER | Character must have Arcane Archer levels to equip; blocks with message "You must have levels as an arcane archer to use [item]" |
| 75 | Stalwart-Defender-Required | ITEM_REQ_STALWART_DEFENDER | Character must have Stalwart Defender levels to equip; blocks with message "You must have levels as a stalwart defender to use [item]" |
| 76 | Shifter-Required | ITEM_REQ_SHIFTER | Character must have Shifter levels to equip; blocks with message "You must have levels as a shifter to use [item]" |
| 77 | Duelist-Required | ITEM_REQ_DUELIST | Character must have Duelist levels to equip; blocks with message "You must have levels as a duelist to use [item]" |
| 78 | Mystic-Theurge-Required | ITEM_REQ_MYSTIC_THEURGE | Character must have Mystic Theurge levels to equip; blocks with message "You must have levels as a mystic theurge to use [item]" |
| 79 | Alchemist-Required | ITEM_REQ_ALCHEMIST | Character must have Alchemist levels to equip; blocks with message "You must have levels as an alchemist to use [item]" |
| 80 | Arcane-Shadow-Required | ITEM_REQ_ARCANE_SHADOW | Character must have Arcane Shadow levels to equip; blocks with message "You must have levels as an arcane shadow to use [item]" |
| 81 | Sacred-Fist-Required | ITEM_REQ_SACRED_FIST | Character must have Sacred Fist levels to equip; blocks with message "You must have levels as a sacred fist to use [item]" |
| 82 | Eldritch-Knight-Required | ITEM_REQ_ELDRITCH_KNIGHT | Character must have Eldritch Knight levels to equip; blocks with message "You must have levels as an eldritch knight to use [item]" |
| 96 | Warlock-Only | ITEM_REQ_WARLOCK | Character must have Warlock levels to equip; blocks with message "You must have levels as a warlock to use [item]" |

### Race Restrictions
**NOTE:** Race Anti flags are currently NOT checked during standard equipment; primarily used for vessel control systems and may be expanded in future.

| # | Flag Name | Code Constant | Description |
|---|-----------|---------------|-------------|
| 18 | Anti-Human | ITEM_ANTI_HUMAN | Intended to prevent Humans from using; currently enforced only in vessel control systems, not standard equipment |
| 19 | Anti-Elf | ITEM_ANTI_ELF | Intended to prevent Elves from using; currently enforced only in vessel control systems, not standard equipment |
| 20 | Anti-Dwarf | ITEM_ANTI_DWARF | Intended to prevent Dwarves from using; currently enforced only in vessel control systems, not standard equipment |
| 21 | Anti-Half-Troll | ITEM_ANTI_HALF_TROLL | Intended to prevent Half-Trolls from using; currently enforced only in vessel control systems, not standard equipment |
| 25 | Anti-Crystal-Dwarf | ITEM_ANTI_CRYSTAL_DWARF | Intended to prevent Crystal Dwarves from using; currently enforced only in vessel control systems, not standard equipment |
| 26 | Anti-Halfling | ITEM_ANTI_HALFLING | Intended to prevent Halflings from using; currently enforced only in vessel control systems, not standard equipment |
| 27 | Anti-Half-Elf | ITEM_ANTI_H_ELF | Intended to prevent Half-Elves from using; currently enforced only in vessel control systems, not standard equipment |
| 28 | Anti-Half-Orc | ITEM_ANTI_H_ORC | Intended to prevent Half-Orcs from using; currently enforced only in vessel control systems, not standard equipment |
| 29 | Anti-Gnome | ITEM_ANTI_GNOME | Intended to prevent Gnomes from using; currently enforced only in vessel control systems, not standard equipment |
| 31 | Anti-Trelux | ITEM_ANTI_TRELUX | Intended to prevent Trelux from using; currently enforced only in vessel control systems, not standard equipment |
| 37 | Anti-Arcana-Golem | ITEM_ANTI_ARCANA_GOLEM | Intended to prevent Arcana Golems from using; currently enforced only in vessel control systems, not standard equipment |
| 49 | Anti-Drow | ITEM_ANTI_DROW | Intended to prevent Drow from using; currently enforced only in vessel control systems, not standard equipment |
| 51 | Anti-Duergar | ITEM_ANTI_DUERGAR | Intended to prevent Duergar from using; currently enforced only in vessel control systems, not standard equipment |
| 93 | Anti-Lich | ITEM_ANTI_LICH | Intended to prevent Liches from using; currently enforced only in vessel control systems, not standard equipment |
| 94 | Anti-Vampire | ITEM_ANTI_VAMPIRE | Intended to prevent Vampires from using; currently enforced only in vessel control systems, not standard equipment |
| 95 | Vampire-Only | ITEM_VAMPIRE_ONLY | Intended to allow only Vampires to use (inverse of Anti flag); currently enforced only in vessel control systems, not standard equipment |

### Weapon Enhancements
| # | Flag Name | Code Constant | Description |
|---|-----------|---------------|-------------|
| 8 | Blessed | ITEM_BLESS | Cosmetic magical property granting blue glow when detected by magical senses (detect alignment, aura of good); no direct combat bonus |
| 47 | Ki-Focus | ITEM_KI_FOCUS | Enables monks to use special abilities (Stunning Fist, Quivering Palm, Water Whip, Gong of Summit, Fist of Unbroken Air) as alternative to monk-specific weapons; acts as monk weapon substitute |
| 50 | Masterwork | ITEM_MASTERWORK | Grants +1 enhancement bonus to attack rolls; only applies when weapon lacks existing magical enhancement (prevents stacking with magic bonuses) |
| 52 | Seeking | ITEM_SEEKING | Ranged weapons only; completely bypasses all concealment penalties, negating 50-100% miss chance from concealment effects; no direct damage modification |
| 53 | Adaptive | ITEM_ADAPTIVE | Ranged weapons only; applies full Strength modifier to damage rolls instead of limited amounts (composite bows normally cap STR bonus); uses complete STR bonus for damage |
| 54 | Agile | ITEM_AGILE | Light weapons only; substitutes Dexterity modifier for Strength in damage calculation (uses MAX of DEX or STR bonus); prevents 1.5x/3x STR multiplier penalties on two-handed or off-hand; damage bonus capped at weapon's agile value or DEX bonus (whichever is lower) |
| 55 | Corrosive | ITEM_CORROSIVE | Toggleable ability dealing 1d6 acid damage per strike on hit; generates "magical acid drips" visual effect; wielder must activate/deactivate |
| 56 | Disruption | ITEM_DISRUPTION | Holy energy enhancement effective against undead only; on-hit deals 2d6 holy damage; on-crit triggers Fortitude save (DC = 10 + level/2), failure causes (level/2 + 3)d6 holy damage; no effect on non-undead |
| 57 | Defending | ITEM_DEFENDING | Grants armor class bonus equal to weapon's enhancement value divided by 2; multiple defending weapons stack; applies to character's overall AC calculation |
| 58 | Vicious | ITEM_VICIOUS | Toggleable ability dealing necrotic/negative damage; on-hit: 3d6 damage to target + 1d4 recoil damage to wielder; on-crit: 6d6 damage to target; damages both attacker and defender (self-harm penalty); generates "black smoke whirls" visual |
| 59 | Vorpal | ITEM_VORPAL | Toggleable instant-death trigger; on-crit: 5% activation chance (1 on d20); successful activation: damage = target_HP + 100 (instant kill); only works on creatures with heads; excludes undead/constructs/oozes; bypassed by MOB_NOCHARM protection flag |

### System & Crafting Flags
| # | Flag Name | Code Constant | Description |
|---|-----------|---------------|-------------|
| 24 | Mold | ITEM_MOLD | Marks item as crafting template/pattern for weapon/armor creation; mold items cannot be worn and convert into actual crafted items during creation process; contains base stats for items to be created |
| 33 | Decaying | ITEM_DECAY | Marks portal-like objects or temporary items for automatic decay; objects fade away when timer reaches zero with fade message to observers before extraction |
| 44 | Auto-Proc | ITEM_AUTOPROC | Flags items for automatic calling via proc_update() heartbeat function; allows item special procedures to trigger periodically without explicit command; excludes unfinished weapons (weapon type with value 0) |
| 98 | Set-Stats-At-Load | ITEM_SET_STATS_AT_LOAD | When object loads, stats automatically recalculated based on type and level; calls set_weapon_object() for weapons or set_armor_object() for armor; preserves enhancement bonuses and cost values after recalculation |
| 99 | Extract-After-Use | ITEM_EXTRACT_AFTER_USE | Item destroyed after use; used for single-use keys that crumble to dust after opening doors; applies to both inventory and equipment slots |
| 101 | Has-Been-Downgraded | ITEM_DOWNGRADED | Marks item as already level-downgraded; prevents multiple downgrades; once set, downgrade command cannot be used on item again |
| 102 | Item-Has-Been-Identified | ITEM_IDENTIFIED | Marks object as identified/appraised; allows lore checks to always succeed on identified items; can be set through identification spells or crafting completion |
| 103 | Crafted-Item | ITEM_CRAFTED | Marks item as player-crafted; used in display systems to show crafting origin; affects how item descriptions are formatted in examine command |
| 104 | Can-Only-Equip-One | ITEM_ONLY_EQUIP_ONE | Restricts player to equipping only one instance simultaneously; currently defined but not actively implemented in equipment checks |
| 105 | Can-Only-Possess-One | ITEM_ONLY_POSSES_ONE | Restricts player to possessing only one instance at a time; currently defined but not actively implemented in codebase |
| 106 | Crafting-Smelter | ITEM_CRAFTING_SMELTER | Marks object as smelter station for metalworking crafts (bronze, steel, brass, alchemical silver, cold iron); required for metal refinement recipes |
| 107 | Crafting-Loom | ITEM_CRAFTING_LOOM | Marks object as loom station for tailoring; required for textile crafting recipes (satin, linen production) |
| 108 | Crafting-Forge | ITEM_CRAFTING_FORGE | Marks object as forge station for smithing; required for armorsmithing, metalworking, and weaponsmithing crafts |
| 109 | Crafting-Alchemy-Lab | ITEM_CRAFTING_ALCHEMY_LAB | Marks object as alchemy laboratory; required for alchemy craft recipes |
| 110 | Crafting-Jewelcrafting-Station | ITEM_CRAFTING_JEWELCRAFTING_STATION | Marks object as jewelcrafting workstation; required for jewelcrafting recipes |
| 111 | Crafting-Tannery | ITEM_CRAFTING_TANNERY | Marks object as tannery station for leatherworking; required for leather crafting recipes |
| 112 | Crafting-Carpentry-Table | ITEM_CRAFTING_CARPENTRY_TABLE | Marks object as carpentry workstation; required for woodworking/carpentry crafts |
| 113 | Trapped | ITEM_TRAPPED | Indicates object has trap mechanism attached via trap system; used to mark trapped chests/doors for comprehensive trap system (not currently actively used for runtime trap behavior) |

**Total: 114 flags (0-113)**

**Note:** These flags are defined in `src/structs.h` (lines 3958-4077) and their display names in `src/constants.c` (lines 2482-2598).

Happy building!
