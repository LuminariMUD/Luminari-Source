# DG Script Fixes Required

This document lists DG Script errors found in the system logs that require builder-level access to fix.

## 1. Dragon Egg Timer Script (VNum 1015)

**Error**: "remote: invalid arguments 'remote egg_hatch_total_time '"

**Current broken syntax**:
```
remote egg_hatch_total_time
```

**Issue**: The `remote` command is missing required parameters.

**Correct syntax**:
```
remote <target> <variable> <value>
```

**Example fix**:
```
remote %self% egg_hatch_total_time 100
```

## 2. Objects in NOWHERE Executing Scripts

The following objects have been observed executing scripts while in NOWHERE location:
- **Dragon eggs (VNum 1015)** - Timer script
- **Webbed cocoons (VNum 1920)** - Unknown script
- **Red liquid streams (VNum 193)** - Unknown script

**Note**: Code-level protections have been added to prevent crashes from objects in NOWHERE, but the scripts themselves should be reviewed to ensure they handle invalid locations gracefully.

## Recommended Actions

1. Review and fix the dragon egg timer script syntax
2. Audit scripts for VNums 1920 and 193 to ensure they handle NOWHERE locations
3. Consider adding location checks in the scripts themselves as defensive programming