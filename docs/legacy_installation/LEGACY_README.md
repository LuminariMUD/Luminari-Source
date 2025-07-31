# Legacy Documentation Directory

This directory contains historical documentation files that have been preserved for reference purposes. **These files should not be used as primary documentation.**

## Documentation Status

✅ **CONSOLIDATED** - All relevant documentation has been consolidated into the modern documentation structure at `docs/consolidated/`

## What Was Consolidated

The following types of documentation have been successfully consolidated:

### Removed Files (Consolidated)
- **License Documentation** - `license.txt`, `license.doc`, `license.pdf`, `license.tex`
  - Consolidated into: [legal/README.md](../consolidated/legal/README.md)
- **Administrator Guides** - `admin.pdf`, `admin.tex`, `running.doc`, `UnixShellAdminGuide.pdf`
  - Consolidated into: [admin/README.md](../consolidated/admin/README.md)
- **Building Documentation** - `building.pdf`, `building.tex`, `socials.*`, `shop.doc`, etc.
  - Consolidated into: [building/README.md](../consolidated/building/README.md)
- **Developer Documentation** - `coding.pdf`, `coding.tex`, `hacker.*`, `database.doc`, etc.
  - Consolidated into: [development/README.md](../consolidated/development/README.md)
- **Installation Guides** - `porting.pdf`, `porting.tex`, various platform-specific files
  - Consolidated into: [installation/README.md](../consolidated/installation/README.md)
- **Utilities Documentation** - `utils.pdf`, `utils.tex`, `utils.doc`
  - Consolidated into: [utilities/README.md](../consolidated/utilities/README.md)
- **General Documentation** - `README*`, `FAQ.*`, `files.*`, `wizhelp.*`
  - Consolidated into: [consolidated/README.md](../consolidated/README.md)

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

- **Main Hub**: [docs/consolidated/README.md](../consolidated/README.md)
- **Quick Navigation**: [docs/consolidated/NAVIGATION.md](../consolidated/NAVIGATION.md)
- **Comprehensive Index**: [docs/consolidated/INDEX.md](../consolidated/INDEX.md)

### By Topic
- **Administration**: [admin/README.md](../consolidated/admin/README.md)
- **Building**: [building/README.md](../consolidated/building/README.md)
- **Development**: [development/README.md](../consolidated/development/README.md)
- **Installation**: [installation/README.md](../consolidated/installation/README.md)
- **Utilities**: [utilities/README.md](../consolidated/utilities/README.md)
- **Legal**: [legal/README.md](../consolidated/legal/README.md)
- **History**: [history/README.md](../consolidated/history/README.md)

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
**Primary Documentation**: [../consolidated/README.md](../consolidated/README.md)
