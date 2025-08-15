# Luminari MUD Changelog

## [Unreleased] - 2025-08-14

### Changed
- **Major Refactoring: Mobile AI System** - Split monolithic `mobact.c` (2086 lines) into 7 focused modules for improved maintainability:
  - `mob_act.c` - Main mobile activity loop (403 lines)
  - `mob_memory.c` - Memory management system (134 lines)
  - `mob_utils.c` - Utility functions (345 lines)
  - `mob_race.c` - Racial behavior implementations (77 lines)
  - `mob_psionic.c` - Psionic power functions (229 lines)
  - `mob_class.c` - Class-specific behaviors (319 lines)
  - `mob_spells.c` - Spell casting and data (718 lines)
  - This is a mechanical refactoring with zero functionality changes
  - Improves compilation times and code maintainability
  - Creates clear module boundaries for easier debugging and extension
