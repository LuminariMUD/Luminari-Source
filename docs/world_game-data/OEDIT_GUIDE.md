# OEDIT Guide for Builders

## Getting Started
- Enter the editor with `oedit <vnum>` to modify an existing object or `oedit create <vnum>` for a fresh prototype.
- Work is buffered in memory until you quit with `Q` followed by `Y`. Choosing `N` abandons your changes.
- Menus accept single-character choices. Numeric submenus expect plain numbers (type `10`, not `a`).
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

### Text Fields (1-4)
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
  - Choose a slot (1-5) to edit.
  - Select the apply from the list (`STR`, `DEX`, `AC`, `FEAT`, etc.). Applies that require a specific school/skill prompt for a follow-up selection.
  - Enter the modifier and pick a bonus type (stacking rules). A value of `0/None` clears the slot.
- The main menu shows an **EQ Rating** and **Suggested affects** after you save and re-enter; use them as guidance, not hard rules.

### Descriptions & Scripts
- `E) Extra descriptions` lets you add keyword-triggered flavor text. Each entry needs both keywords and a description, otherwise it will be discarded when you exit the submenu.
- `S) Script` jumps into the DG Script editor. Note that scripting is disabled when editing a live instance via `iedit`.

## Editing Values (C)

Selecting `C` zeroes all 16 value slots before prompting. The sequence of
prompts varies by item type; set the type first so the correct path is taken.

The full slot-by-slot layout for every type lives in the
[Object Value Reference](#object-value-reference) below. Note that the editor's
`ValueN` labels are one higher than the file-format `value[N-1]` slot they
write. The notes here cover the cases that behave unusually:

### Weapons (`ITEM_WEAPON`)
- Picking the weapon type auto-populates dice, cost, weight, material, size, and wear slots from `weapon_list`.
- After the type is chosen you go straight to the enhancement prompt (`Value5`, which is `value[4]`).
- You can revisit `C` if you need to tweak damage dice or attack type; be aware that re-running `C` resets enhancements.

### Armor (`ITEM_ARMOR` and `ITEM_CLANARMOR`)
- Normal armor takes its subtype at `Value2` (`value[1]`) and auto-fills AC, size, material, and wear flags via `set_armor_object()`.
- Clan armor uses that same slot for the clan vnum instead. The value must identify an existing clan.
- Both armor types collect an enhancement bonus at `Value5` (`value[4]`).

### Ammunition & Ranged
- `ITEM_FIREWEAPON`: prompts for weapon type, damage dice, then break chance (2-98). Enhancement is `Value5`.
- `ITEM_MISSILE`: prompts for ammo category, then break chance, then enhancement at `Value5`.
- `ITEM_AMMO_POUCH` / `ITEM_CONTAINER`: capacity in pounds (`-1` unlimited), then a flag toggle for the closeable/lockable bits - enter `0` when done - then the key vnum (`-1` for none).

### Consumables
- `ITEM_POTION` / `ITEM_SCROLL`: spell level first, then up to three spells. Use `-1` or `0` to clear a slot.
- `ITEM_WAND` / `ITEM_STAFF`: spell level, max charges, charges remaining, then the spell to cast.
- `ITEM_POISON`: poison spell, level, applications, and hits per application, across the first four slots.

### Lights & Miscellaneous
- `ITEM_LIGHT`: the first two slots are unused, so the editor jumps straight to burn hours (`-1` infinite, `0` burnt out).
- `ITEM_DRINKCON` / `ITEM_FOUNTAIN`: capacity, current units, liquid type, then an optional spell on drink.
- `ITEM_PORTAL`: which slots you are asked for depends on the portal type chosen at the first prompt. A clanhall portal asks for nothing further.
- `ITEM_TRAP`: likewise branches - the second prompt is a direction, an object vnum, or skipped entirely depending on the trigger.
- `ITEM_TREASURE_CHEST`: loot tier, loot type, random-load flag, search DC, then pick-lock DC.
- `ITEM_GEAR_OUTFIT`: outfit type, enhancement, material, then apply type and modifier.

For unusual object types, read the prompts carefully; each menu is sourced from `src/olc/oedit.c` and mirrors the in-game expectations.

## Combat Enhancements
- `F) Weapon Spells` manages up to `MAX_WEAPON_SPELLS` (4) procs.
  - Choose the slot number. Enter `-1` at the spell list to clear the slot.
  - Set the cast level (>=1), the chance to proc (`1-50`%), then whether it fires only in combat (`1` offensive) or defensively (`0`).
- `J) Special Abilities` attaches structured weapon spec-abilities.
  - `N` creates a new ability; `E` selects an existing one by position; `C` removes all.
  - For each ability choose the type, minimum level (1-34), activation methods (toggle numbers; `0` finishes), optional command word, and ability-specific values (e.g., Bane race/subrace).
- `K) Activated Spells` configures a charged use effect (wands, staves, oddities).
  - Enter `0` at the level prompt to remove the activation.
  - Valid levels are 1-30; spells are chosen from the filtered list that follows.
  - Uses must be between 1 and `MAX_NUMBER_OF_ACTIVATED_SPELL_USES` (5). Items regenerate one use every five minutes in-game.

## Spellbooks (`T`)
- Spellbooks can store up to `SPELLBOOK_SIZE` entries (200). The prompt shows existing spells grouped by circle and the next empty slot.
- Selecting a slot opens the spell-picker. Enter `0` to clear the slot.
- Pages are assigned automatically to at least 1 and default to half of the spell's minimum wizard level (integer division). Builders cannot override pages directly in OEDIT.

## Copying, Deleting, and Saving
- `W) Copy object` clones an existing prototype into the current buffer (useful for variants). You still need to `Q`+`Y` to persist the changes.
- `X) Delete object` permanently removes the prototype after confirmation; instances already in the world remain until extracted.
- `Q) Quit` prompts to save. Respond with `Y` to write to disk (and update live prototypes) or `N` to discard.

## Tips & Verification
- Keep object levels, enhancement bonuses, and pricing consistent; rely on `EQ Rating` and the suggested bonus hints but always sanity-check against existing loot.
- After major edits, `oload <vnum>` in a controlled area and `stat obj` to verify wear flags, applies, and scripts.
- Use `plist`/`olist` to ensure numbering stays within zone limits and to spot accidental duplicates.
- For DG scripts, coordinate with an implementor when attaching complex triggers.
- Cross-reference `src/olc/oedit.c`, `src/obj/treasure.c`, and `constants.c` for the definitive lists of types, flags, and ability IDs.

After saving, validate the owning zone and inspect the object from the same
parsed model used by the checks:

```sh
python3 scripts/world/wtool.py validate --zone 30
python3 scripts/world/wtool.py show obj 3000
python3 scripts/world/wtool.py refs obj 3000
```

For raw flag fields, decode all four serialized chunks or encode an
unambiguous source macro or OLC display name:

```sh
python3 scripts/world/wtool.py flags decode obj-extra 0 0 0 0
python3 scripts/world/wtool.py flags encode obj-wear ITEM_WEAR_TAKE ITEM_WEAR_HOLD
```

See the [World Validator CLI](../utilities/WORLD_VALIDATOR_CLI.md) for strict
mode, JSON output, findings, and exit statuses.

## Extra Flags Reference
When you select `6) Extra flags` from the main menu, you can toggle any of the
116 available item flags. Enter `0` to return to the main menu.

**The `#` column below is the bit number stored in the `.obj` file. The editor
prompts for `bit + 1`.** To toggle `Glows` (bit 0) you type `1`. The wear-flag
menu uses the same offset. Below is the complete list:

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
| 114 | Costs-Account-Experience | ITEM_ACCOUNT_EXP | Purchasing or acquiring the item is paid for in account experience rather than gold |
| 115 | Can-Be-Reforged | ITEM_REFORGEABLE | Item is eligible for the reforging path, which rerolls or upgrades its stats through the crafting system |
| 116 | RoL-Anti-Good-Race | ITEM_ROL_ANTI_GOOD_RACE | RoL compatibility restriction; prevents mortal members of the source good-race family from equipping the item or reciting it as a scroll |
| 117 | RoL-No-Identify | ITEM_ROL_NO_IDENTIFY | RoL compatibility restriction; blocks identify, mass identify, lore, and greater lore for mortals |
| 118 | RoL-No-Summon | ITEM_ROL_NO_SUMMON | RoL compatibility protection; prevents the wearer from being moved by summon and group summon |
| 119 | RoL-No-Sleep | ITEM_ROL_NO_SLEEP | RoL compatibility protection; prevents sleep effects while the item is carried or worn |
| 120 | RoL-No-Charm | ITEM_ROL_NO_CHARM | RoL compatibility protection; prevents charm effects while the item is carried or worn |
| 121 | RoL-Two-Handed | ITEM_ROL_TWO_HANDED | RoL compatibility equipment rule; the item always requires two hands regardless of its size |
| 122 | RoL-Anti-Evil-Race | ITEM_ROL_ANTI_EVIL_RACE | RoL compatibility restriction; prevents mortal members of the source evil-race family from equipping the item or reciting it as a scroll |
| 123 | RoL-Whole-Body | ITEM_ROL_WHOLE_BODY | RoL compatibility equipment rule; body armor also occupies the conceptual arm and leg coverage, so arm and leg gear cannot overlap it |
| 124 | RoL-Whole-Head | ITEM_ROL_WHOLE_HEAD | RoL compatibility equipment rule; head gear also occupies the conceptual face and eye coverage, so face and eye gear cannot overlap it |

**Total: 125 flags (bits 0-124, `NUM_ITEM_FLAGS`)**

**Note:** These flags are defined in `src/structs.h` as the `ITEM_*` define block
ending at `ITEM_ROL_WHOLE_HEAD`, and their display names in the `extra_bits[]`
table in `src/constants.c`.

The RoL source `DARK` object flag is intentionally not persisted. Source tracing
confirmed that it requested light recalculation but was never consumed by the
source light counters; making it darken target rooms would add behavior that the
source game did not have.

## Item Types Reference

`5) Type` accepts a number from this list. The number you type at the menu is the
same as the type constant - unlike the flag menus, item types are not offset.
Type 0 (`UNDEFINED`) is not a usable type; setting it leaves the object inert.

Setting the type is the first thing you do, because it decides which value
prompts `C) Values` shows you. See [Object Value Reference](#object-value-reference)
for what each type stores in its value slots.

| # | Name | Constant | Notes |
|---|------|----------|-------|
| 0 | UNDEFINED | - | Placeholder. Not a usable type. |
| 1 | Light | ITEM_LIGHT | Light source; burn hours live in value 2. |
| 2 | Scroll | ITEM_SCROLL | Up to three spells, consumed on read. |
| 3 | Wand | ITEM_WAND | Charged, single-target spell device. |
| 4 | Staff | ITEM_STAFF | Charged, area-effect spell device. |
| 5 | Weapon | ITEM_WEAPON | Melee weapon. Type selection auto-fills dice and stats. |
| 6 | Furniture | ITEM_FURNITURE | Sittable; value 0 is occupant capacity. |
| 7 | Ranged-Weapon | ITEM_FIREWEAPON | Bows and crossbows. Marked deprecated in the source but still in use. |
| 8 | Treasure | ITEM_TREASURE | Valuables that are not coins. No values. |
| 9 | Armor/Shield | ITEM_ARMOR | Armor. Subtype selection auto-fills AC, size, material, wear. |
| 10 | Potion | ITEM_POTION | Up to three spells, consumed on quaff. |
| 11 | Wearable | ITEM_WORN | General worn gear with no type-specific behavior. |
| 12 | Other | ITEM_OTHER | Catch-all. No values. |
| 13 | Trash | ITEM_TRASH | Shopkeepers refuse to buy. No values. |
| 14 | Ammo | ITEM_MISSILE | Ammunition for ranged weapons. |
| 15 | Container | ITEM_CONTAINER | Holds objects; capacity, flags, and key vnum in values. |
| 16 | Note | ITEM_NOTE | Writable with a pen. No values. |
| 17 | Liquid-Cont | ITEM_DRINKCON | Drink container. |
| 18 | Key | ITEM_KEY | Opens a matching lock by vnum. No values. |
| 19 | Food | ITEM_FOOD | Edible; value 0 is duration in rounds. |
| 20 | Money | ITEM_MONEY | Coin pile; value 0 is the gold amount. |
| 21 | Pen | ITEM_PEN | Writes on notes. No values. |
| 22 | Boat | ITEM_BOAT | Permits water travel. No values. |
| 23 | Fountain | ITEM_FOUNTAIN | Fixed drink source; same value layout as a drink container. |
| 24 | Clan-Armor | ITEM_CLANARMOR | Armor restricted to a clan; value 1 is the clan vnum. |
| 25 | Crafting Crystal | ITEM_CRYSTAL | Crafting input. |
| 26 | Essence | ITEM_ESSENCE | Crafting input. |
| 27 | Crafting Material | ITEM_MATERIAL | Crafting input. |
| 28 | Spellbook | ITEM_SPELLBOOK | Stores spells; managed from `T) Spellbook menu`, not `C`. |
| 29 | Portal | ITEM_PORTAL | Destination travel object. Value layout depends on portal mode. |
| 30 | Plant | ITEM_PLANT | Target for the plant-transport spell. No values. |
| 31 | Trap | ITEM_TRAP | Trap object; trigger, target, and effect live in the values. |
| 32 | Teleport | ITEM_TELEPORT | Teleports on command. |
| 33 | Poison | ITEM_POISON | Weapon poison; spell, level, applications, hits per application. |
| 34 | Summon | ITEM_SUMMON | Summons a mob on command. |
| 35 | Switch | ITEM_SWITCH | Lever or button that manipulates a door in another room. |
| 36 | Ammo-Pouch | ITEM_AMMO_POUCH | Container specialized for ammunition. |
| 37 | Pick | ITEM_PICK | Grants a bonus to lock picking. |
| 38 | Instrument | ITEM_INSTRUMENT | Bard instrument; subtype, difficulty reduction, effectiveness bonus, breakability. |
| 39 | Disguise | ITEM_DISGUISE | Kit used by the disguise command. |
| 40 | Wall | ITEM_WALL | Magical wall object, as created by wall spells. |
| 41 | Bowl | ITEM_BOWL | Mixing vessel for recipes. |
| 42 | Ingredient | ITEM_INGREDIENT | Used with a bowl for recipes. |
| 43 | Blocker | ITEM_BLOCKER | Blocks movement in one direction. |
| 44 | Wagon | ITEM_WAGON | Carries resources for trade. |
| 45 | Resources | ITEM_RESOURCE | Trade goods carried by a wagon. |
| 46 | Pet | ITEM_PET | Converts into a mobile follower on purchase. |
| 47 | Blueprint | ITEM_BLUEPRINT | NewCraft recipe; value 0 is the craft ID. |
| 48 | Treasure Chest | ITEM_TREASURE_CHEST | Lootable chest used by the `loot` command. |
| 49 | Hunt Trophy | ITEM_HUNT_TROPHY | Marks a hunt target mob. |
| 50 | Weapon Oil | ITEM_WEAPON_OIL | Applied to a weapon for a temporary effect. |
| 51 | Gear Outfit | ITEM_GEAR_OUTFIT | Preset gear bundle; expands into a full kit. |
| 52 | Drink | ITEM_DRINK | Newer drink system; replaces drink containers and fountains. |
| 53 | Vehicle | ITEM_VEHICLE | General vehicle object. |
| 54 | Ship-Object | ITEM_SHIP_OBJECT | Outcast-style ship object. |
| 55 | Vessel | ITEM_VESSEL | Unified vessel system object. |
| 56 | Greyhawk-Ship | ITEM_GREYHAWK_SHIP | Greyhawk ship; value 0 interior room vnum, value 1 ship index. |
| 57 | Crafting-Tool | ITEM_CRAFTING_TOOL | Tool granting a bonus to one crafting ability. |

**Total: 58 types (0-57, `NUM_ITEM_TYPES`)**

## Wear Flags Reference

`7) Wear flags` toggles the slots an item can occupy. **The menu numbers are
offset by one from the bit numbers**: the list below gives the bit index that
appears in the `.obj` file, and the editor prompts for `bit + 1`. To set
`Finger` (bit 1) you type `2`. The same offset applies to the extra-flag menu.

Bit 0 (`(Takeable)`) is not a slot - it controls whether the item can be picked
up at all. Nearly every object needs it. An item with wear flags but without
`(Takeable)` can be equipped only by staff loading it directly.

| Bit | Menu # | Name | Constant |
|-----|--------|------|----------|
| 0 | 1 | (Takeable) | ITEM_WEAR_TAKE |
| 1 | 2 | Finger | ITEM_WEAR_FINGER |
| 2 | 3 | Neck | ITEM_WEAR_NECK |
| 3 | 4 | Body | ITEM_WEAR_BODY |
| 4 | 5 | Head | ITEM_WEAR_HEAD |
| 5 | 6 | Legs | ITEM_WEAR_LEGS |
| 6 | 7 | Feet | ITEM_WEAR_FEET |
| 7 | 8 | Hands | ITEM_WEAR_HANDS |
| 8 | 9 | Arms | ITEM_WEAR_ARMS |
| 9 | 10 | Shield | ITEM_WEAR_SHIELD |
| 10 | 11 | About-Body | ITEM_WEAR_ABOUT |
| 11 | 12 | Waist | ITEM_WEAR_WAIST |
| 12 | 13 | Wrist | ITEM_WEAR_WRIST |
| 13 | 14 | Wield | ITEM_WEAR_WIELD |
| 14 | 15 | Hold | ITEM_WEAR_HOLD |
| 15 | 16 | Face | ITEM_WEAR_FACE |
| 16 | 17 | Ammo-Pouch | ITEM_WEAR_AMMO_POUCH |
| 17 | 18 | Ears | ITEM_WEAR_EAR |
| 18 | 19 | Eyes | ITEM_WEAR_EYES |
| 19 | 20 | Badge | ITEM_WEAR_BADGE |
| 20 | 21 | Instrument | ITEM_WEAR_INSTRUMENT |
| 21 | 22 | Shoulders | ITEM_WEAR_SHOULDERS |
| 22 | 23 | Ankle | ITEM_WEAR_ANKLE |
| 23 | 24 | Sheath | ITEM_WEAR_SHEATH |
| 24 | 25 | Gathering-Tool | ITEM_WEAR_CRAFT_SICKLE |
| 25 | 26 | Forestry-Tool | ITEM_WEAR_CRAFT_AXE |
| 26 | 27 | Hunting-Tool | ITEM_WEAR_CRAFT_KNIFE |
| 27 | 28 | Mining-Tool | ITEM_WEAR_CRAFT_PICKAXE |
| 28 | 29 | Alchemy-Tool | ITEM_WEAR_CRAFT_ALCHEMY |
| 29 | 30 | Armorsmithing-Tool | ITEM_WEAR_CRAFT_ARMOR_HAMMER |
| 30 | 31 | Jewelcrafting-Tool | ITEM_WEAR_CRAFT_JEWEL_PLIERS |
| 31 | 32 | Tailoring-Tool | ITEM_WEAR_CRAFT_NEEDLE |
| 32 | 33 | Weaponsmithing-Tool | ITEM_WEAR_CRAFT_WEAPON_HAMMER |
| 33 | 34 | On-Back | ITEM_WEAR_ON_BACK |

**Total: 34 wear bits (0-33, `NUM_ITEM_WEARS`)**

The nine crafting-tool slots (bits 24-32) are worn simultaneously with normal
gear and pair with `ITEM_CRAFTING_TOOL` objects. A gathering sickle occupies
`Gathering-Tool`, not `Hold`, so carrying a full tool set costs a player no
combat slots.

## Object Value Reference

Every object carries 16 integer value slots. What they mean is decided entirely
by the item type - there is no shared meaning across types, and a value that is
meaningful for one type is ignored for another.

### Two naming schemes, one array

This trips up nearly everyone:

- The **file format and the code** use `value[0]` through `value[15]`.
- The **OEDIT prompts** are labelled `Value1` through `Value6`.

`ValueN` in the editor writes `value[N-1]`. The table below uses the file-format
numbering. When a prompt says "Value5: Enhancement bonus", that is `value[4]`.

### The value line in a `.obj` file

The second numeric line of an object record holds the values. `parse_object()`
in `src/db.c` accepts **exactly 4 integers or exactly 16** - any other count is
a fatal boot error:

```
SYSERR: Format error in second numeric line (expecting 4 or 16 args, got N)
```

Four values is the legacy CircleMUD format and is silently upgraded, leaving
`value[4]` through `value[15]` at zero. Since enhancement bonuses live in
`value[4]`, **any object that needs an enhancement must use the 16-value form.**
Write all 16 for anything new.

### Per-type value layout

Slots not listed are unused for that type and should be left at 0.

| Type | value[0] | value[1] | value[2] | value[3] | value[4] |
|------|----------|----------|----------|----------|----------|
| Light | unused | unused | Hours of light (`-1` infinite, `0` burnt out) | - | - |
| Scroll | Spell level | Spell 1 | Spell 2 | Spell 3 | - |
| Potion | Spell level | Spell 1 | Spell 2 | Spell 3 | - |
| Wand | Spell level | Max charges | Charges remaining | Spell | - |
| Staff | Spell level | Max charges | Charges remaining | Spell | - |
| Weapon | Weapon type (`weapon_list`) | Number of damage dice | Damage die size | Attack message type | Enhancement bonus |
| Ranged-Weapon | Weapon type | Number of damage dice | Breaking probability (2-98) | - | Enhancement bonus |
| Ammo | Ammo category | unused | Breaking probability | - | Enhancement bonus |
| Armor/Shield | Armor class apply | Armor subtype (`armor_list`) | - | - | Enhancement bonus |
| Clan-Armor | Armor class apply | Clan vnum | - | - | Enhancement bonus |
| Container | Max weight held (`-1` unlimited) | Container flags (bitvector) | Key vnum (`-1` none) | - | - |
| Ammo-Pouch | Max weight held (`-1` unlimited) | Container flags (bitvector) | Key vnum (`-1` none) | - | - |
| Liquid-Cont | Max drink units (`-1` unlimited) | Current drink units | Liquid type | Spell on drink (`0` none) | - |
| Fountain | Max drink units (`-1` unlimited) | Current drink units | Liquid type | Spell on drink (`0` none) | - |
| Food | Duration in rounds (6s each) | - | - | - | - |
| Drink | Duration in rounds (6s each) | - | - | - | - |
| Money | Number of gold coins | - | - | - | - |
| Furniture | Number of people it holds | - | - | - | - |
| Portal | Portal type | Target room vnum, or low vnum of range | High vnum of range (random only) | - | - |
| Trap | Trap trigger type | Direction or target object vnum | Trap effect | - | - |
| Switch | Activating command (`0` pull, `1` push) | Room vnum to manipulate | Direction (`0`n `1`e `2`s `3`w `4`u `5`d) | - | - |
| Poison | Poison spell | Poison level | Applications | Hits per application | - |
| Instrument | Instrument subtype | Difficulty reduction (0-30) | Effectiveness bonus (0-10) | Breakability chance in 11,111 per performance verse (0 unbreakable) | - |
| Blueprint | Craft ID number | - | - | - | - |
| Treasure Chest | Loot tier / level | Loot type | Random-load chest (`1`/`0`) | Search DC (`0` not hidden) | Pick Lock DC (`0` unlocked) |
| Greyhawk-Ship | Interior room vnum | Ship index (0-499, unique) | - | - | - |
| Crafting-Tool | Crafting ability (`ABILITY_*`) | Bonus amount | Quality / durability | Special flags | - |
| Gear Outfit | Outfit type (`1` weapon, `2` armor set) | Enhancement bonus | Material | Body apply type | Apply modifier amount |
| Wearable | Special worn value (e.g. monk glove enhancement) | - | - | - | - |

Types with no values at all: `Treasure`, `Trash`, `Other`, `Note`, `Pen`, `Key`,
`Boat`, `Plant`. Selecting `C` on these returns straight to the main menu.

### Notes on specific types

- **Weapon and Armor auto-fill.** Choosing the weapon type or armor subtype runs
  `set_weapon_object()` / `set_armor_object()`, which overwrite dice, cost,
  weight, material, size, and wear flags from the master tables. Set those
  first, then adjust. Re-running `C` resets the enhancement bonus.
- **Portal values shift by mode.** With `PORTAL_NORMAL` or `PORTAL_CHECKFLAGS`,
  `value[1]` is the destination room. With `PORTAL_RANDOM`, `value[1]` and
  `value[2]` bound a vnum range. `PORTAL_CLANHALL` needs no room at all - it
  always sends the player to their own clan hall.
- **Instrument breakability is a numerator.** A value from 0 through 11,111 is
  rolled once per performance verse while the instrument is used. Zero is
  unbreakable, 1 is a 1-in-11,111 chance, 30 is a 30-in-11,111 chance, and
  11,111 always breaks.
- **Trap values shift by trigger.** `value[1]` is a direction for the door
  triggers, an object vnum for the container and get-object triggers, and unused
  for the rest.
- **Container flags are a bitvector**, not an index. The editor toggles bits;
  entering the same number twice clears it.
- **Enhancement lives in `value[4]`** for weapons, ranged weapons, ammo, and
  both armor types. This is the single most common reason a hand-written object
  ends up with no enhancement: the record was written with only four values.

Happy building!
