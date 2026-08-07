# LuminariMUD Technical Documentation Master Index

Last Updated: 2026-08-07
Version: 1.3

## Documentation Overview

This master index provides a comprehensive guide to all technical documentation for LuminariMUD. Documents are organized by category and purpose to help developers, builders, and administrators quickly find the information they need.

### Quick Links
- [Setup & Getting Started](#setup--getting-started)
- [Architecture & Core Systems](#architecture--core-systems)
- [Game Systems](#game-systems)
- [Development Guides](#development-guides)
- [Building & Content Creation](#building--content-creation)
- [Administration & Operations](#administration--operations)
- [Project Management](#project-management)

---

## Setup & Getting Started

### Essential First Steps
- **[README.md](../README.md)** - Project overview, quick start, and essential information
- **[LUMINARI_OVERVIEW.md](guides/LUMINARI_OVERVIEW.md)** - What LuminariMUD is: the world, its systems, and what makes it distinct (written for players and newcomers to MUDs)
- **[DEPLOYMENT_GUIDE.md](deployment/DEPLOYMENT_GUIDE.md)** - Complete setup and deployment instructions
- **[CONTRIBUTING.md](../CONTRIBUTING.md)** - How to contribute to the project
- **[CODE_OF_CONDUCT.md](../CODE_OF_CONDUCT.md)** - Community guidelines and expectations

### Configuration & Installation
- **[CMAKE_BUILD_GUIDE.md](development/CMAKE_BUILD_GUIDE.md)** - Alternative build system using CMake
- **[IMPORT_DB.md](guides/IMPORT_DB.md)** - Database import procedures
- **[VESSEL_SCHEMA_DEPLOYMENT.md](deployment/VESSEL_SCHEMA_DEPLOYMENT.md)** - Vessel schema deployment, verification, and rollback procedure

---

## Architecture & Core Systems

### System Architecture
- **[ARCHITECTURE.md](systems/ARCHITECTURE.md)** - Overall system design and module organization
- **[CORE_SERVER_ARCHITECTURE.md](systems/CORE_SERVER_ARCHITECTURE.md)** - Game loop, networking, and core engine
- **[Special-Procedure Boot Control Plane](systems/CORE_SERVER_ARCHITECTURE.md#special-procedure-boot-control-plane)** - Definition, authored/effective binding, boot precedence, diagnostics, and compatibility boundaries
- **[DATA_STRUCTURES_AND_MEMORY.md](development/DATA_STRUCTURES_AND_MEMORY.md)** - Memory management and data structures
- **[COMMAND_SYSTEM_AND_INTERPRETER.md](systems/COMMAND_SYSTEM_AND_INTERPRETER.md)** - Command parsing and execution

### Database & Persistence
- **[DATABASE_INTEGRATION.md](systems/DATABASE_INTEGRATION.md)** - MySQL integration and schema
- **[SAVE_SYSTEMS_BREAKDOWN.md](systems/SAVE_SYSTEMS_BREAKDOWN.md)** - Save file formats and persistence
- **[HELP_SYSTEM.md](systems/HELP_SYSTEM.md)** - Database-first help content, SQL migration, verification, and legacy import/export workflow

### Communication & Protocols
- **[PROTOCOL_SYSTEMS.md](systems/PROTOCOL_SYSTEMS.md)** - Telnet, MSDP, GMCP, and client protocols
- **[MUDLET_GUI_PACKAGE.md](systems/MUDLET_GUI_PACKAGE.md)** - Mudlet GUI package
  identity, updates, and web deployment
- **[WEB_ONBOARDING_SYSTEM.md](systems/WEB_ONBOARDING_SYSTEM.md)** - Versioned MSDP account, character-creation, and role-play presentation for Luminari Web
- **[CLIENT_CAPABILITIES_AND_PREFERENCES.md](systems/CLIENT_CAPABILITIES_AND_PREFERENCES.md)** - How client capability auto-detection and player PRF flags interact, what persists, and when
- **[INTERMUD3.md](systems/INTERMUD3.md)** - Inter-MUD communication system
- **[INTERMUD3_GATEWAY_API.md](systems/INTERMUD3_GATEWAY_API.md)** - External I3 Gateway API reference (JSON-RPC 2.0)
- **[INTERMUD3_SECURITY_AUDIT.md](systems/INTERMUD3_SECURITY_AUDIT.md)** - I3 client security audit (remediated 2025-08-26)
- **[LOCAL_INTERMUD3_E2E.md](runbooks/LOCAL_INTERMUD3_E2E.md)** - Local gateway and MUD end-to-end operations runbook

---

## Game Systems

### Product Requirements

- **[Vessel System Product Requirements](product-requirements/VESSEL_SYSTEM_REQUIREMENTS.md)** -
  Durable vessel requirements, wilderness contract, release criteria, and
  authoritative release-gate state

### Core Mechanics
- **[GAME_MECHANICS_SYSTEMS.md](systems/GAME_MECHANICS_SYSTEMS.md)** - Overview of all game systems
- **[BARD_PERKS.md](systems/perks/BARD_PERKS.md)** - Base performance contract and Bard perk trees
- **[COMBAT_SYSTEM.md](systems/COMBAT_SYSTEM.md)** - Combat mechanics and formulas
- **[SPELL_PREPARATION_SYSTEM.md](systems/SPELL_PREPARATION_SYSTEM.md)** - Magic and spell preparation
- **[PLAYER_MANAGEMENT_SYSTEM.md](systems/PLAYER_MANAGEMENT_SYSTEM.md)** - Character and account management

### Advanced Systems
- **[ARTIFACT_SYSTEM.md](systems/ARTIFACT_SYSTEM.md)** - Unique artifact
  registry, ownership, binding, progression, powers, persistence, deployment,
  and staff operations
- **[VESSEL_SYSTEM.md](systems/VESSEL_SYSTEM.md)** - Ships, airships, submarines, and land vehicles (carts, wagons, mounts) with wilderness navigation, multi-room interiors, and unified transport interface
- **[VESSEL_BENCHMARKS.md](testing/VESSEL_BENCHMARKS.md)** - Current and
  historical vessel performance, memory, and test evidence
- **[CLAN_SYSTEM.md](systems/CLAN_SYSTEM.md)** - Guild and clan functionality
- **[MOUNT_SYSTEM.md](systems/MOUNT_SYSTEM.md)** - Mounts, paladin/blackguard mounts, and dragon mounts (code reference)
- **[CASTING_VISUALS_SYSTEM.md](systems/CASTING_VISUALS_SYSTEM.md)** - School, class, metamagic, progress, and environmental spellcasting visuals
- **[CHARACTER_RENAME_SYSTEM.md](systems/CHARACTER_RENAME_SYSTEM.md)** - Authoritative player rename across index, files, accounts, and database
- **[STAFF_EVENT_SYSTEM.md](systems/STAFF_EVENT_SYSTEM.md)** - Staff-managed game events
- **[wilderness_system.md](world_game-data/wilderness_system.md)** - Dynamic wilderness generation
- **[SPATIAL_SYSTEMS_ARCHITECTURE.md](systems/SPATIAL_SYSTEMS_ARCHITECTURE.md)** - Spatial visual and audio systems with triple strategy pattern
- **[RESOURCE_SYSTEM_TECHNICAL.md](systems/RESOURCE_SYSTEM_TECHNICAL.md)** - Wilderness resource system with flexible region effects
- **[RESOURCE_REGENERATION_SYSTEM.md](systems/RESOURCE_REGENERATION_SYSTEM.md)** - Resource regeneration with seasonal and weather effects
- **[RESOURCE_REGENERATION_QUICK_REFERENCE.md](systems/RESOURCE_REGENERATION_QUICK_REFERENCE.md)** - Quick reference for resource regeneration system
- **[DYNAMIC_RESOURCE_DESCRIPTIONS_PLAN.md](systems/DYNAMIC_RESOURCE_DESCRIPTIONS_PLAN.md)** - Dynamic environment descriptions based on resource state
- **[AI_SERVICE_README.md](systems/AI_SERVICE_README.md)** - AI service integration

### Building & Scripting
- **[OLC_ONLINE_CREATION_SYSTEM.md](systems/OLC_ONLINE_CREATION_SYSTEM.md)** - Online building tools
- **[SCRIPTING_SYSTEM_DG.md](systems/SCRIPTING_SYSTEM_DG.md)** - DG Scripts documentation

### Utility Systems
- **[UTILITY_SYSTEMS.md](systems/UTILITY_SYSTEMS.md)** - Helper systems and utilities

---

## Development Guides

### Core Development
- **[DEVELOPER_GUIDE_AND_API.md](guides/DEVELOPER_GUIDE_AND_API.md)** - Comprehensive developer reference
- **[RESOURCE_SYSTEM_REFERENCE.md](guides/RESOURCE_SYSTEM_REFERENCE.md)** - Resource system player/admin reference
- **[RESOURCE_REGENERATION_API.md](development/RESOURCE_REGENERATION_API.md)** - Resource regeneration API for developers
- **[PHASE_4_INSTALLATION.md](guides/PHASE_4_INSTALLATION.md)** - Phase 4 region effects system setup and testing guide
- **[CLAUDE.md](../CLAUDE.md)** - AI assistant guidelines and codebase patterns
- **[TESTING_GUIDE.md](guides/TESTING_GUIDE.md)** - Testing procedures and frameworks
- **[Special Procedure Phase 00 Validation](testing/SPECIAL_PROCEDURE_PHASE_00_VALIDATION.md)** - Registry-safety acceptance, 78-test ownership, evidence matrix, and reproducible phase gates
- **[RESOURCE_SYSTEM_TESTING.md](testing/RESOURCE_SYSTEM_TESTING.md)** - Resource system testing guide
- **[VESSEL_SYSTEM_TESTING.md](testing/VESSEL_SYSTEM_TESTING.md)** - Vessel system manual regression script (30 steps)
- **[LOCAL_DEV_LOGIN_QUICK_GUIDE.md](testing/LOCAL_DEV_LOGIN_QUICK_GUIDE.md)** - Credential-safe local boot, Kohdee login, and character/account logout smoke test
- **[CASTING_VISUALS_TESTING.md](testing/CASTING_VISUALS_TESTING.md)** - Casting visuals in-game verification checklist
- **[vessel_test_results.md](testing/vessel_test_results.md)** - Historical Phase 00 vessel test record (superseded; see VESSEL_BENCHMARKS.md)
- **[CuTest_README.md](testing/CuTest_README.md)** - Unit testing with CuTest
- **[PROTOCOL_PARSER_HARNESS.md](testing/PROTOCOL_PARSER_HARNESS.md)** - Focused protocol safety and framing regression matrix

### Performance & Optimization
- **[PERFORMANCE_OPTIMIZATIONS.md](development/PERFORMANCE_OPTIMIZATIONS.md)** - Performance tuning guide
- **[affect_update_optimization.md](development/affect_update_optimization.md)** - Specific optimization case study

### Troubleshooting
- **[TROUBLESHOOTING_AND_MAINTENANCE.md](guides/TROUBLESHOOTING_AND_MAINTENANCE.md)** - Common issues and solutions

---

## Building & Content Creation

### Building Guides
- **[Builder Quickstart](world_game-data/BUILDER_QUICKSTART.md)** - One zone from empty to bootable; start here
- **[Builder's Manual](world_game-data/builder_manual.md)** - Overview of building tools and processes
- **[OLC SpecProc Editing](guides/OLC_SpecProcs.md)** - Owner-aware selection, prerequisites, authored persistence, effective diagnostics, and moving-room safety
- **[World Validator CLI](utilities/WORLD_VALIDATOR_CLI.md)** - Read-only world parsing, validation, typed lookup, flags, and documentation checks
- **[ultimate-mud-writing-guide.md](guides/ultimate-mud-writing-guide.md)** - Writing compelling MUD content

### World File Formats
- **[Zone File Format](world_game-data/ZONE_FILE_FORMAT.md)** - `.zon` header fields, reset commands, and parser gotchas
- **[Shop File Format](world_game-data/SHOP_FILE_FORMAT.md)** - `.shp` field-by-field reference
- **[Quest File Format](world_game-data/QUEST_FILE_FORMAT.md)** - `.qst` grammar, source-backed types and flags, rewards, references, and chain rules
- **[High-Level Quest File Format](world_game-data/HLQUEST_FILE_FORMAT.md)** - `.hlq` host entries, command legality, bounds, references, and runtime order
- **[OEDIT Guide](world_game-data/OEDIT_GUIDE.md)** - Object editor, item types, wear slots, and the object value vectors
- **[Room Flags](world_game-data/ROOM_FLAGS.md)** - All 42 room flags
- **[Mobile Flags](world_game-data/MOB_FLAGS.md)** - All 105 mobile action flags

### Equipment & Items
- **[CRAFTING_SYSTEM_NOTES.md](world_game-data/CRAFTING_SYSTEM_NOTES.md)** - Crafting system design notes (partial; verify against `src/craft/`)
- **[gear_guide.md](world_game-data/gear_guide.md)** - Combined gear stat distribution (equipment balance, loot generation, stats by wear location)

---

## Administration & Operations

### Admin Tools
- **[Admin README](admin/README.md)** - Administration overview
- **[FAQ.md](admin/FAQ.md)** - Frequently asked questions

### Utilities
- **[Utilities README](utilities/README.md)** - External utility programs

### Legal & Licensing
- **[LICENSE](../LICENSE)** - Project licensing information
- **[Legal README](legal/README.md)** - Legal considerations

---

## Project Management

### Planning & Tracking
- **[TASK_LIST.md](TASK_LIST.md)** - Current development priorities
- **[docs-audit.md](docs-audit.md)** - Vessel documentation source-of-truth and evidence audit
- **[CHANGELOG.md](CHANGELOG.md)** - Recent changes and updates

### Project Documents
- **[Ongoing projects](ongoing-projects/)** -
  Active work: artifact placement handoff, AI conversation history, SKORE
  phases 3-4, event-system merge, and in-game backlogs
- **[artifact-placement-plan.md](ongoing-projects/artifact-placement-plan.md)** -
  World-building brief for placing artifacts in player-facing content, the
  artifact system's one remaining open item
- **[AI_TODO_IDEAS.md](ongoing-projects/AI_TODO_IDEAS.md)** - AI conversation history implementation plan
- **[DO_SKORE_PROJECT.md](ongoing-projects/DO_SKORE_PROJECT.md)** - Skore system phases 3-4
- **[IDEA_LIST.md](ongoing-projects/IDEA_LIST.md)** - Player and staff feature idea backlog

### Historical Records
- **[Previous Changelogs](previous_changelogs/)** - Historical development records

---

## Document Categories

### By Audience

#### For New Developers
1. README.md
2. DEPLOYMENT_GUIDE.md
3. DEVELOPER_GUIDE_AND_API.md
4. ARCHITECTURE.md
5. CONTRIBUTING.md

#### For System Administrators
1. DEPLOYMENT_GUIDE.md
2. DATABASE_INTEGRATION.md
3. TROUBLESHOOTING_AND_MAINTENANCE.md
4. PHP_TOOLS_README.md

#### For Content Builders
1. OLC_ONLINE_CREATION_SYSTEM.md
2. SCRIPTING_SYSTEM_DG.md
3. ultimate-mud-writing-guide.md
4. Building guides in world_game-data/
5. world/STARTER_AREA.md

#### For Core Contributors
1. DEVELOPER_GUIDE_AND_API.md
2. PERFORMANCE_OPTIMIZATIONS.md
3. TESTING_GUIDE.md
4. All system documentation

### By Technical Depth

#### Introductory (Beginner)
- README.md
- FAQ.md
- CODE_OF_CONDUCT.md
- ultimate-mud-writing-guide.md

#### Intermediate
- DEPLOYMENT_GUIDE.md
- OLC_ONLINE_CREATION_SYSTEM.md
- GAME_MECHANICS_SYSTEMS.md
- Building guides

#### Advanced
- ARCHITECTURE.md
- CORE_SERVER_ARCHITECTURE.md
- DEVELOPER_GUIDE_AND_API.md
- PERFORMANCE_OPTIMIZATIONS.md

#### Reference
- DATABASE_INTEGRATION.md
- SCRIPTING_SYSTEM_DG.md
- COMMAND_SYSTEM_AND_INTERPRETER.md
- PROTOCOL_SYSTEMS.md

---

## Finding Information

### Quick Search Tips
1. **By Feature**: Look in `systems/` directory
2. **By Task**: Check `guides/` directory
3. **By Problem**: See TROUBLESHOOTING_AND_MAINTENANCE.md
4. **By Code**: Refer to DEVELOPER_GUIDE_AND_API.md

### Document Naming Convention
- `*_SYSTEM.md` - System documentation
- `*_GUIDE.md` - How-to guides
- `*_README.md` - Overview documents
- `*_TODO.md` - Planning documents
- `*_TASK_LIST.md` - Work tracking

### Documentation Standards
- All docs use Markdown format
- Code examples follow GNU C23 and the established source style
- Internal links use relative paths
- External links include descriptions

---

## Notes

### Recent Updates (2025)
- Documentation reorganized into categorical directories
- Removed redundant `/guides/` paths
- Updated to reflect GNU C23 compliance
- Added performance optimization documentation
- Enhanced security documentation

### Documentation Gaps
- Need more examples in API documentation
- Combat formulas need detailed breakdown
- Scripting cookbook would be helpful
- Video tutorials for OLC system

### Contributing to Documentation
1. Follow markdown best practices
2. Include code examples where relevant
3. Keep technical accuracy paramount
4. Update this index when adding new docs
5. Test all commands and code examples

---

*This index is maintained as part of the LuminariMUD project. For corrections or additions, please submit a pull request or contact the development team.*
