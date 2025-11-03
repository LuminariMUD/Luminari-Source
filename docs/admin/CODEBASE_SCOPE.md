# LuminariMUD Codebase Scope Analysis

## Executive Summary

**Total Project Size**: 401 MB  
**Total Lines of Code**: ~387,471 lines (C/H files)  
**World Data Files**: 3,727 files  
**Active Development Files**: 330 source files (171 C, 159 H)  

## Source Code Breakdown

### Core Language Distribution

| File Type | Count | Lines of Code | Size |
|-----------|-------|---------------|------|
| **C Files** | 171 | 345,512 | 12 MB |
| **Header Files** | 159 | 41,959 | 1.9 MB |
| **PHP Files** | 7 | 3,296 | ~100 KB |
| **Shell Scripts** | 16 | - | - |
| **Makefiles** | 19 | - | - |

### Top 20 Largest C Files (by lines)

1. `fight.c` - 13,223 lines (Combat system)
2. `magic.c` - 12,582 lines (Magic system)
3. `spec_procs.c` - 11,359 lines (Special procedures)
4. `act.offensive.c` - 10,785 lines (Offensive actions)
5. `utils.c` - 10,728 lines (Utility functions)
6. `act.wizard.c` - 10,334 lines (Admin commands)
7. `class.c` - 10,133 lines (Class system)
8. `act.other.c` - 9,789 lines (Misc commands)
9. `act.informative.c` - 9,434 lines (Info commands)
10. `feats.c` - 8,909 lines (Feat system)
11. `act.item.c` - 8,299 lines (Item commands)
12. `db.c` - 7,564 lines (Database)
13. `constants.c` - 6,793 lines (Game constants)
14. `treasure.c` - 6,341 lines (Loot system)
15. `spell_parser.c` - 6,281 lines (Spell parsing)
16. `crafting_new.c` - 6,146 lines (Crafting system)
17. `study.c` - 5,967 lines (Study/research system)
18. `race.c` - 5,725 lines (Race system)
19. `clan.c` - 5,670 lines (Clan system)
20. `spell_prep.c` - 4,962 lines (Spell preparation)

### Directory Structure & Sizes

| Directory | Size | Description |
|-----------|------|-------------|
| **lib/** | 232 MB | World data, player files, configuration |
| **src/** | 59 MB | Source code files |
| **docs/** | 2.3 MB | Documentation |
| **util/** | 912 KB | PHP utilities and tools |

## World Data Analysis

### World Files Distribution

| File Type | Count | Size | Purpose |
|-----------|-------|------|---------|
| **WLD (Rooms)** | 581 | 41 MB | Room descriptions, exits, flags |
| **MOB (Mobiles)** | 581 | 8.9 MB | NPCs, monsters, stats |
| **OBJ (Objects)** | 580 | 4.6 MB | Items, equipment, treasures |
| **ZON (Zones)** | 582 | 3.3 MB | Zone resets, timers, commands |
| **SHP (Shops)** | 575 | - | Shop definitions, keepers |
| **TRG (Triggers)** | 356 | - | DG Script triggers |
| **HLQ (HL Quests)** | 321 | - | Homeland quest definitions |
| **QST (Quests)** | 186 | - | Quest definitions |

**Total World Files**: 3,727  
**Total World Data Size**: ~58 MB  

## PHP Utilities Breakdown

| File | Lines | Purpose |
|------|-------|---------|
| `enter_encounter.php` | 730 | Encounter management |
| `enter_hunt.php` | 581 | Hunt system management |
| `bonuses.php` | 475 | Bonus calculations |
| `enter_spell_help.php` | 463 | Spell help editor |
| `bonus_breakdown.php` | 452 | Bonus analysis |
| `config.php` | 370 | Configuration management |
| `autoload.php` | 225 | Class autoloader |

**Total PHP Lines**: 3,296

## Codebase Characteristics

### Scale Indicators
- **Large-scale MUD**: 580+ zones with comprehensive world data
- **Feature-rich**: Full D&D 3.5/Pathfinder implementation
- **Mature codebase**: ~387K lines of well-structured C code
- **Active content**: 581 mobile types, 580 object types
- **Dynamic scripting**: 356 DG Script triggers
- **Quest systems**: 507 total quests (186 standard + 321 homeland)

### Technical Complexity
- **Core Systems**: 171 C files averaging ~2,020 lines each
- **Modular Design**: Clear separation between subsystems
- **Database Integration**: MySQL backend for persistence
- **Event-Driven**: Action queues, timed events, triggers
- **Multi-Character**: Account system supporting multiple characters

### Major Subsystems by Size
1. **Combat** (fight.c, act.offensive.c): ~24,000 lines
2. **Magic** (magic.c, spell_parser.c, spell_prep.c): ~23,800 lines
3. **Character Development** (class.c, race.c, feats.c): ~24,700 lines
4. **Commands** (act.*.c files): ~50,000+ lines
5. **World/Database** (db.c, constants.c): ~14,300 lines
6. **Crafting/Economy** (crafting_new.c, treasure.c): ~12,400 lines

### Development Activity Areas
- **Most Complex**: Combat, magic, and character systems
- **Most Content**: 580+ zones with full room/mob/object data
- **Most Dynamic**: 356 triggers + 507 quests
- **Most Maintained**: act.*.c command files (player interaction)

## Summary Statistics

| Metric | Value |
|--------|-------|
| **Total Files** | 4,057+ |
| **Source Files** | 330 (C/H) |
| **World Data Files** | 3,727 |
| **Total LoC (C)** | 345,512 |
| **Total LoC (H)** | 41,959 |
| **Total LoC (PHP)** | 3,296 |
| **Combined LoC** | ~390,767 |
| **Average C File Size** | 2,020 lines |
| **Largest File** | fight.c (13,223 lines) |
| **Total Zones** | 582 |
| **Total Rooms** | 581 files (est. 10,000+ rooms) |
| **Total NPCs** | 581 files (est. 5,000+ mob types) |
| **Total Items** | 580 files (est. 8,000+ object types) |

## Codebase Assessment

This is a **mature, large-scale MUD codebase** with:
- Professional-grade architecture (~390K LoC)
- Comprehensive game content (580+ zones)
- Full D&D 3.5/Pathfinder ruleset implementation
- Advanced features (crafting, quests, scripting)
- Significant world content (3,700+ data files)
- Active maintenance and development

The codebase represents approximately **10-15 years** of development effort based on size and complexity, making it one of the more substantial MUD codebases in active development.