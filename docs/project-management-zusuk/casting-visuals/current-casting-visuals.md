# Casting Visuals System Documentation

## Overview

This document provides a comprehensive mapping of all elements related to "casting visuals" in LuminariMUD - the messages and visual feedback displayed during spellcasting, including start, progress, completion, and interruption scenarios.

---

## Table of Contents

1. [Core Data Structures](#core-data-structures)
2. [Key Functions](#key-functions)
3. [Casting Start Visuals](#casting-start-visuals)
4. [Casting Progress Visuals](#casting-progress-visuals)
5. [Casting Completion Visuals](#casting-completion-visuals)
6. [Casting Interruption/Abort Visuals](#casting-interruptionabort-visuals)
7. [Syllable Obfuscation System](#syllable-obfuscation-system)
8. [Spellcraft Identification](#spellcraft-identification)
9. [Consumable Item Visuals](#consumable-item-visuals)
10. [Magic Failure Visuals](#magic-failure-visuals)
11. [File Locations](#file-locations)

---

## Core Data Structures

### Casting State Variables (structs.h:5829-5835)

Located in `struct char_special_data`:

```c
/** casting (time) **/
bool isCasting;               // casting or not
int castingTime;              // casting time remaining
int castingSpellnum;          // spell being cast
int castingMetamagic;         // spell metamagic flags
int castingClass;             // casting class used
struct char_data *castingTCH; // target character of spell
struct obj_data *castingTOBJ; // target object of spell
```

### Accessor Macros (utils.h:1175-1181)

```c
#define IS_CASTING(ch) ((ch)->char_specials.isCasting)
#define CASTING_TIME(ch) ((ch)->char_specials.castingTime)
#define CASTING_TCH(ch) ((ch)->char_specials.castingTCH)
#define CASTING_TOBJ(ch) ((ch)->char_specials.castingTOBJ)
#define CASTING_SPELLNUM(ch) ((ch)->char_specials.castingSpellnum)
#define CASTING_METAMAGIC(ch) ((ch)->char_specials.castingMetamagic)
#define CASTING_CLASS(ch) ((ch)->char_specials.castingClass)
```

### Metamagic Flags (spells.h:22-31)

```c
#define METAMAGIC_NONE 0
#define METAMAGIC_QUICKEN (1 << 0)
#define METAMAGIC_MAXIMIZE (1 << 1)
#define METAMAGIC_HEIGHTEN (1 << 2)
#define METAMAGIC_ARCANE_ADEPT (1 << 3)
#define METAMAGIC_EMPOWER (1 << 4)
#define METAMAGIC_SILENT (1 << 5)
#define METAMAGIC_STILL (1 << 6)
#define METAMAGIC_EXTEND (1 << 7)
```

---

## Key Functions

### Primary Functions (spell_parser.c)

| Function | Line | Purpose |
|----------|------|---------|
| `say_spell()` | 231-338 | Displays casting visual messages to room |
| `resetCastingData()` | 1435-1444 | Clears all casting state variables |
| `castingCheckOk()` | 1446-1528 | Validates casting can continue |
| `finishCasting()` | 1531-1618 | Handles spell completion |
| `event_casting()` | 1620-1734 | Casting time event handler |
| `cast_spell()` | 1740+ | Entry point for NPC casts |
| `do_gen_cast()` | 2272+ | Entry point for PC casts |
| `do_abort()` | 2253-2266 | Voluntary abort command |
| `concentration_check()` | 151-215 | Combat concentration check |

### Support Functions

| Function | File | Purpose |
|----------|------|---------|
| `change_position()` | movement_position.c:381 | Position change interrupt |
| `command_can_be_used_while_casting()` | interpreter.c:4816 | Command whitelist check |

---

## Casting Start Visuals

### Standard Spell Casting (spell_parser.c:2201-2205)

**To Caster:**
```
You begin casting your spell...
```

**To Room (via say_spell with start=TRUE):**

| Target Type | Message Format |
|-------------|----------------|
| Self | `$n weaves $s hands in an intricate pattern and begins to chant the words, '<spell>'` |
| Character | `$n weaves $s hands in an intricate pattern and begins to chant the words, '<spell>' at $N` |
| Object | `$n stares at $p and begins chanting the words, '<spell>'` |
| No target | `$n begins chanting the words, '<spell>'` |

**Message to Target (if targeting another character):**
```
$n weaves $s hands in an intricate pattern and begins to chant the words, '<spell>' at you.
```

### Class-Specific Start Messages

#### Alchemist (spell_parser.c:2193-2194)
```
To Caster: You begin preparing your extract...
To Room:   $n begins preparing an extract.
```

#### Psionicist (spell_parser.c:2198-2199)
```
To Caster: You begin to manifest your power...
To Room:   $n begins manifesting a power.
```

### Instant Cast (No Casting Time) (spell_parser.c:2163)
```
To Caster: Okay.  (CONFIG_OK)
```
Then `say_spell()` is called with `start=FALSE` for completion message.

---

## Casting Progress Visuals

### Progress Display (spell_parser.c:1663-1675)

During each casting tick, the caster sees:

```
<Action>: [metamagic modifiers] <spell name> ***
```

Where:
- **Action** = "Casting" (spells), "Preparing" (alchemist), or "Manifesting" (psionicist)
- **Metamagic modifiers** (if any): "quickened ", "empowered ", "silent ", "still ", "extended ", "maximized "
- **Asterisks** = One `*` per remaining casting time unit

**Example:**
```
Casting: empowered extended fireball ****
```

---

## Casting Completion Visuals

### Standard Completion (spell_parser.c:1573, 1580)

**Visual to Room (via say_spell with start=FALSE):**

| Target Type | Message Format |
|-------------|----------------|
| Self | `$n closes $s eyes and utters the words, '<spell>'` |
| Character | `$n stares at $N and utters the words, '<spell>'` |
| Object | `$n stares at $p and utters the words, '<spell>'` |
| No target | `$n utters the words, '<spell>'` |

**Message to Target:**
```
$n stares at you and utters the words, '<spell>'.
```

**Completion Message to Caster:**
```
You complete your spell...     (standard)
You complete the extract...    (alchemist)
You complete your manifestation... (psionicist)
```

### Double Manifestation (Psionicist) (spell_parser.c:1599)
```
[DOUBLE MANIFEST!]
```

---

## Casting Interruption/Abort Visuals

### Voluntary Abort (spell_parser.c:2264)
**Command:** `abort`
```
To Caster: You abort your spell.
```

### Concentration Check Failed (spell_parser.c:208-209)
```
To Caster: You lost your concentration!
To Room:   $n's concentration is lost, and spell is aborted!
```

### Deafness Fumble (spell_parser.c:198-199)
```
To Caster: Your deafness has made you fumble your spell!
To Room:   $n seems to have fumbled his spell for some reason.
```

### Position Change Interrupt (movement_position.c:393-395)
Triggered when position drops to SITTING or below during casting:
```
To Room:   $n's spell is interrupted!
To Caster: Your spell is aborted!
```

### Position Check Failed (spell_parser.c:1455-1457)
```
To Room:   $n is unable to continue $s spell in $s current position!
To Caster: You are unable to continue your spell in your current position! (spell aborted)
```

### Target Object Unavailable (spell_parser.c:1466-1468)
```
To Room:   $n is unable to continue $s spell!
To Caster: You are unable to find the object for your spell! (spell aborted)
```

### Target Vanished (spell_parser.c:1488-1490)
```
To Room:   $n is unable to continue $s spell!
To Caster: Your target has vanished! (spell aborted)
```

### Target Left Room (spell_parser.c:1499-1501)
```
To Room:   $n is unable to continue $s spell!
To Caster: You are unable to find the target for your spell! (spell aborted)
```

### Nauseated (spell_parser.c:1509-1511)
```
To Caster: You are too nauseated to continue casting!
To Room:   $n seems to be too nauseated to continue casting!
```

### Dazed/Stunned/Paralyzed (spell_parser.c:1519-1521)
```
To Caster: You are unable to continue casting!
To Room:   $n seems to be unable to continue casting!
```

### Busy Casting Message (interpreter.c:1408)
When attempting non-whitelisted commands while casting:
```
You are too busy casting [you can 'abort' the spell]...
```

### Already Casting (spell_parser.c:1888)
```
You are already attempting to cast!
```

### Not Casting (spell_parser.c:2260)
When trying to abort while not casting:
```
You aren't casting!
```

---

## Syllable Obfuscation System

### Purpose
Converts spell names into "magical sounding" phrases for observers who fail spellcraft checks.

### Syllable Table (spell_parser.c:72-137)

```c
static struct syllable syls[] = {
    {" ", " "},
    {"ar", "abra"},
    {"ate", "i"},
    {"cau", "kada"},
    {"blind", "nose"},
    {"bur", "mosa"},
    {"cu", "judi"},
    {"de", "oculo"},
    {"dis", "mar"},
    {"ect", "kamina"},
    {"en", "uns"},
    {"gro", "cra"},
    {"light", "dies"},
    {"lo", "hi"},
    {"magi", "kari"},
    {"mon", "bar"},
    {"mor", "zak"},
    {"move", "sido"},
    {"ness", "lacri"},
    {"ning", "illa"},
    {"per", "duda"},
    {"ra", "gru"},
    {"re", "candus"},
    {"son", "sabru"},
    {"tect", "infra"},
    {"tri", "cula"},
    {"ven", "nofo"},
    {"word of", "inset"},
    /* Single letter substitutions */
    {"a", "i"}, {"b", "v"}, {"c", "q"}, {"d", "m"}, {"e", "o"},
    {"f", "y"}, {"g", "t"}, {"h", "p"}, {"i", "u"}, {"j", "y"},
    {"k", "t"}, {"l", "r"}, {"m", "w"}, {"n", "b"}, {"o", "a"},
    {"p", "s"}, {"q", "d"}, {"r", "f"}, {"s", "g"}, {"t", "h"},
    {"u", "e"}, {"v", "z"}, {"w", "x"}, {"x", "n"}, {"y", "l"},
    {"z", "k"},
    {"", ""},
    /* Number conversions (Hebrew) */
    {"1", "echad"}, {"2", "shtayim"}, {"3", "shelosh"},
    {"4", "arba"}, {"5", "chamesh"}, {"6", "sheish"},
    {"7", "shevah"}, {"8", "shmoneh"}, {"9", "teisha"},
    {"0", "efes"}
};
```

### Example Conversions
- "cure light" -> "qefo dutph" (approximate)
- "fireball" -> "yufovir" (approximate)

---

## Spellcraft Identification

### Mechanism (spell_parser.c:311-319)

Observers in the room make a spellcraft check:
- **DC:** 20 (hardcoded as `dc_of_id`)
- **Check:** `compute_ability(observer, ABILITY_SPELLCRAFT) + d20()`
- **NPC fallback:** `10 + d20()`

### Results
- **Success (attempt > dc_of_id):** See actual spell name
- **Failure:** See obfuscated syllables

---

## Consumable Item Visuals

### Potion Quaffing (act.item.c:6541-6546)
```
To Caster: You quaff a potion of '<spell name>'.
To Room:   $n quaffs a potion
```

### Scroll Recitation (act.item.c:6651-6656)
```
To Caster: You recite a scroll of '<spell name>'.
To Room:   $n recites a scroll.
```

### Wand Use (act.item.c:6766-6772)
```
To Caster: You point a wand of '<spell>' at $N.
To Target: $n points a wand at YOU!
To Room:   $n points a wand at $N.
```

### Staff Invocation (act.item.c:6856-6861)
```
To Caster: You invoke a staff of '<spell>'.
To Room:   $n invokes a staff.
```

### With Metamagic
All consumable messages include metamagic prefixes when applicable:
```
You quaff a potion of 'empowered cure light'.
```

---

## Magic Failure Visuals

### Anti-Magic Room/Zone (spell_parser.c:528-529, 542-543)
```
To Caster: Your magic fizzles out and dies!
To Room:   $n's magic fizzles out and dies...
```

### Peaceful Room (spell_parser.c:557-558)
For violent spells in peaceful rooms:
```
To Caster: A flash of white light fills the room, dispelling your violent magic!
To Room:   White light from no particular source suddenly fills the room, then vanishes.
```

### Arcane Spell Failure (armor penalty) (spell_parser.c:607-608, 619-620)
```
To Caster: Your armor ends up hampering your spell! (arcane spell failure)
To Room:   $n's spell is hampered by $s armor!
```

### Inert Spell (magic.c:12882)
```
Your spell is completely inert!
```

---

## File Locations

### Primary Files

| File | Key Content |
|------|-------------|
| `src/spell_parser.c` | Core casting system, say_spell(), event_casting() |
| `src/structs.h` | Casting data structures (char_special_data) |
| `src/utils.h` | CASTING_* macros |
| `src/spells.h` | Metamagic defines, cast type defines |
| `src/movement_position.c` | Position change interrupt |
| `src/interpreter.c` | Command blocking during cast |
| `src/act.item.c` | Consumable item casting visuals |
| `src/magic.c` | Magic failure messages |

### Line Number Quick Reference

| Component | File:Line |
|-----------|-----------|
| Syllable table | spell_parser.c:72-137 |
| say_spell() | spell_parser.c:231-338 |
| concentration_check() | spell_parser.c:151-215 |
| resetCastingData() | spell_parser.c:1435-1444 |
| castingCheckOk() | spell_parser.c:1446-1528 |
| finishCasting() | spell_parser.c:1531-1618 |
| event_casting() | spell_parser.c:1620-1734 |
| Casting progress display | spell_parser.c:1663-1675 |
| Start messages | spell_parser.c:2191-2205 |
| do_abort() | spell_parser.c:2253-2266 |
| Casting state struct | structs.h:5829-5835 |
| Casting macros | utils.h:1175-1181 |
| Metamagic defines | spells.h:22-31 |
| Position interrupt | movement_position.c:391-397 |
| Command whitelist | interpreter.c:4816-4850 |

---

## Commands Allowed While Casting

The following commands can be used while casting (interpreter.c:4819-4847):
- look, group, affects, gtell, gsay
- equipment, inventory, who, score
- queue, help, feat, tnl, prefedit
- races, classes, cooldowns, abilities
- resistances, ooc, chat, osay, save
- attackqueue
- say (Campaign FR/DL only)

---

## Events System Integration

### Casting Event (mud_event.h:34)
```c
eCASTING,  // casting time event
```

### Event Creation (spell_parser.c:2215-2219)
```c
// NPC casting - 2 passes per second
NEW_EVENT(eCASTING, ch, NULL, 2 * PASSES_PER_SEC);

// PC casting - 1 pass per second
NEW_EVENT(eCASTING, ch, NULL, 1 * PASSES_PER_SEC);
```

---

## Color Codes Used

| Code | Color | Usage |
|------|-------|-------|
| `\tn` | Normal | Reset color |
| `\tc` | Cyan | Casting action text |
| `\tC` | Bright Cyan | Spell name |
| `\tm` | Magenta | "intricate" emphasis |
| `\tW` | Bright White | Special messages (DOUBLE MANIFEST) |

---

## Summary Flow Diagram

```
[Player types 'cast <spell>']
         |
         v
    do_gen_cast()
         |
         v
[Validation checks pass?] --No--> [Error message, return]
         |
        Yes
         v
[Casting time > 0?] --No--> [Instant cast: CONFIG_OK + say_spell(FALSE)]
         |                            |
        Yes                           v
         |                   [call_magic() - spell executes]
         v
[Start messages]
- "You begin casting your spell..."
- say_spell(TRUE) to room
         |
         v
[IS_CASTING = TRUE]
[NEW_EVENT(eCASTING)]
         |
         v
    [Event Loop]
         |
         v
[castingCheckOk()?] --No--> [Interrupt message, resetCastingData()]
         |
        Yes
         v
[concentration_check()?] --No--> [Concentration lost message, resetCastingData()]
         |
        Yes
         v
[Display progress: "Casting: <spell> ****"]
[CASTING_TIME--]
         |
         v
[CASTING_TIME <= 0?] --No--> [Return to event loop]
         |
        Yes
         v
    finishCasting()
         |
         v
[say_spell(FALSE) - completion visual]
["You complete your spell..."]
[call_magic() - spell executes]
[resetCastingData()]
```

---

## Notes for Future Development

1. **Adding New Casting Messages:** Modify `say_spell()` in spell_parser.c
2. **Adding New Interrupt Conditions:** Add to `castingCheckOk()` or `concentration_check()`
3. **Class-Specific Visuals:** Check CASTING_CLASS() and add conditional messages
4. **Metamagic Visual Effects:** Add to the snprintf in event_casting() progress display
5. **New Syllable Conversions:** Add to the `syls[]` array (longer substrings first)

---

*Document generated for LuminariMUD casting visuals system analysis*
*Last updated: 2025*
