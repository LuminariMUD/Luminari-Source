# Legacy Documentation Directory

This directory contains historical documentation files that have been preserved for reference purposes. **These files should not be used as primary documentation.**

## Documentation Status

✅ **CONSOLIDATED** - All relevant documentation has been consolidated into the modern documentation structure under various `docs/` subdirectories

## What Was Consolidated

The following types of documentation have been successfully consolidated:

### Removed Files (Consolidated)
- **License Documentation** - `license.txt`, `license.doc`, `license.pdf`, `license.tex`
  - Consolidated into: [legal/README.md](../legal/README.md)
- **Administrator Guides** - `admin.pdf`, `admin.tex`, `running.doc`, `UnixShellAdminGuide.pdf`
  - Consolidated into: [admin/README.md](../admin/README.md)
- **Building Documentation** - `building.pdf`, `building.tex`, `socials.*`, `shop.doc`, etc.
  - Consolidated into: [building_game-data/README.md](../building_game-data/README.md)
- **Developer Documentation** - `coding.pdf`, `coding.tex`, `hacker.*`, `database.doc`, etc.
  - Consolidated into: [development/README.md](../development/README.md)
- **Installation Guides** - `porting.pdf`, `porting.tex`, various platform-specific files
  - Consolidated into: [guides/SETUP_AND_BUILD_GUIDE.md](../guides/SETUP_AND_BUILD_GUIDE.md)
- **Utilities Documentation** - `utils.pdf`, `utils.tex`, `utils.doc`
  - Consolidated into: [utilities/README.md](../utilities/README.md)
- **General Documentation** - `README*`, `FAQ.*`, `files.*`, `wizhelp.*`
  - Consolidated into: [TECHNICAL_DOCUMENTATION_MASTER_INDEX.md](../TECHNICAL_DOCUMENTATION_MASTER_INDEX.md)

## Remaining Files

The following files remain for historical reference or because they contain platform-specific build configurations:

### Build Configuration Files
- `Makefile.*` - Platform-specific makefiles (Amiga, OS/2, Windows, etc.)
- `conf.h.*` - Platform-specific configuration headers
- `Smakefile` - SAS/C makefile for Amiga
- `build_circlemud.com` - VMS build script

### Utility Scripts
- `autorun.*` - Various platform autorun scripts
- `macrun.pl` - Mac-specific run script
- `vms_autorun.com` - VMS autorun script
- `do_mail` - Mail utility script

### Development Tools
- `licheck` - License checking utility
- `header` - Header file template
- `practiceProject` - Practice/example project

### Reference Files
- `files` - File listing reference

## Using Modern Documentation

**For current documentation, use the consolidated documentation:**

- **Main Hub**: [docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md](../TECHNICAL_DOCUMENTATION_MASTER_INDEX.md)
- **Quick Start**: [README.md](../../README.md)
- **Comprehensive Index**: [docs/TECHNICAL_DOCUMENTATION_MASTER_INDEX.md](../TECHNICAL_DOCUMENTATION_MASTER_INDEX.md)

### By Topic
- **Administration**: [admin/README.md](../admin/README.md)
- **Building**: [building_game-data/README.md](../building_game-data/README.md)
- **Development**: [development/README.md](../development/README.md)
- **Installation**: [guides/SETUP_AND_BUILD_GUIDE.md](../guides/SETUP_AND_BUILD_GUIDE.md)
- **Utilities**: [utilities/README.md](../utilities/README.md)
- **Legal**: [legal/README.md](../legal/README.md)
- **Systems**: [systems/](../systems/)

## Historical Context

These legacy files represent the evolution of Luminari MUD documentation:

- **Original Format**: Plain text, PDF, and LaTeX documents
- **Organization**: Scattered across multiple files and formats
- **Maintenance**: Difficult to update and keep current
- **Accessibility**: Limited navigation and cross-referencing

The consolidated documentation addresses these issues with:

- **Modern Format**: Markdown with proper structure
- **Unified Organization**: Logical hierarchy and clear navigation
- **Easy Maintenance**: Single-source documentation
- **Enhanced Accessibility**: Cross-references and comprehensive indexing

## Migration Notes

If you were previously using files from this directory:

1. **Find the equivalent section** in the consolidated documentation
2. **Update any bookmarks or references** to point to the new location
3. **Report any missing information** if you find content that wasn't properly consolidated
4. **Use the new navigation aids** for faster access to information

## Preservation Policy

These remaining files are preserved for:

- **Historical reference** - Understanding the project's documentation evolution
- **Platform-specific needs** - Some build configurations may still be relevant
- **Development archaeology** - Researchers studying MUD development history
- **Backup purposes** - Ensuring no critical information is lost

---

**Legacy Directory Status**: Cleaned and Documented  
**Consolidation Date**: 2025-07-27  
**Primary Documentation**: [../TECHNICAL_DOCUMENTATION_MASTER_INDEX.md](../TECHNICAL_DOCUMENTATION_MASTER_INDEX.md)
