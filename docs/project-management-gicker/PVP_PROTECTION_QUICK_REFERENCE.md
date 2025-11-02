# PVP Protection System - Quick Reference

## Summary
Implemented comprehensive PVP protection that prevents players from attacking each other unless BOTH have PVP enabled or are in the arena.

## What Was Changed

### 1. Core Function: `pvp_ok()` in `src/utils.c`
**Before**: Just returned `true` (no protection)
**After**: Full validation checking:
- CONFIG_PK_ALLOWED setting
- Both players' PRF_PVP flags
- Arena exception (rooms 138600-138608)
- Pet/follower master PVP flags

### 2. Direct Combat: `do_hit()` in `src/act.offensive.c`
Added PVP check before combat initiation

### 3. Assist: `perform_assist()` in `src/act.offensive.c`
Replaced simple CONFIG check with full pvp_ok() validation

### 4. Spells: `call_magic()` in `src/spell_parser.c`
Added PVP check for all violent/damage spells

### 5. Backstab: `do_backstab()` in `src/act.offensive.c`
Added PVP check before backstab attempt

### 6. Knockdown: `perform_knockdown()` in `src/act.offensive.c`
Added PVP check covering bash, trip, bodyslam, etc.

## How It Works

### Rule Hierarchy (in order):
1. **NPC vs NPC**: Always allowed ✓
2. **CONFIG_PK_ALLOWED = NO**: Only arena combat allowed
3. **Arena Rooms (138600-138608)**: Always bypass PVP flags ✓
4. **PC vs PC**: Both must have PRF_PVP enabled ✓
5. **Pet Combat**: Master's PVP flag must be enabled ✓
6. **PC vs NPC**: Always allowed (normal mobs) ✓

## Player Experience

### Enabling PVP:
```
> pvp
Your PVP flag is now ON. You may attack and be attacked by other PVP-enabled players.
You cannot turn PVP off for 15 minutes.
```

### Attack Blocked (Attacker):
```
> kill playerB
You must enable PVP (type 'pvp') before attacking other players.
```

### Attack Blocked (Target):
```
> kill playerB
Your target does not have PVP enabled and cannot be attacked.
```

### Config Disabled:
```
> kill playerB
Player killing is not allowed on this MUD.
```

### In Arena:
```
> kill playerB
[Combat initiates regardless of PVP flags]
```

## Commands Protected

### Direct Combat:
- hit, kill, attack → `do_hit()`
- assist → `perform_assist()`

### Spells:
- ALL offensive spells → `call_magic()`
  - Damage spells (fireball, magic missile, etc.)
  - Debuffs (hold person, blindness, etc.)
  - Charms (charm person, dominate, etc.)

### Special Abilities:
- backstab → `do_backstab()`
- bash, trip, bodyslam → `perform_knockdown()`
- kick, charge, etc. → calls `hit()` (protected)

### Already Protected:
- lich touch → `perform_lichtouch()` (had pvp_ok)
- touch of corruption → `do_touchofcorruption()` (had pvp_ok)

## Configuration

### Global Setting:
**File**: `src/config.c`
```c
int pk_allowed = NO;  // Set to YES to enable PVP globally
```

### Arena Definition:
**File**: `src/utils.h`
```c
#define ARENA_START 138600
#define ARENA_END 138608
```

### Player Flag:
**Command**: `pvp` (toggles PRF_PVP flag)
**Prefedit**: Option 'K' in More Preferences menu
**Cooldown**: 15 minutes after enabling before can disable

## Testing Checklist

- [ ] Attack with PVP OFF → blocked
- [ ] Attack target with PVP OFF → blocked
- [ ] Both PVP ON → combat works
- [ ] Arena combat (PVP OFF) → works
- [ ] Spell with PVP OFF → blocked
- [ ] Pet attack (master PVP OFF) → blocked
- [ ] Backstab with PVP OFF → blocked
- [ ] Bash/trip with PVP OFF → blocked
- [ ] NPC combat (any PVP status) → works
- [ ] CONFIG_PK_ALLOWED OFF → only arena works

## Files Modified

1. `src/utils.c` - Rewrote pvp_ok() function
2. `src/act.offensive.c` - Added checks to do_hit, perform_assist, do_backstab, perform_knockdown
3. `src/spell_parser.c` - Added check to call_magic

## Compilation

```bash
cd /home/krynn/code
make
```

**Status**: ✅ Compiled successfully (no errors)

## Documentation

- Full details: `docs/PVP_PROTECTION_SYSTEM.md`
- Related: `docs/PVP_FLAG_IMPLEMENTATION.md`
- Related: `docs/PVP_COOLDOWN_DISPLAY.md`
- Related: `docs/PVP_TIMER_INITIALIZATION_FIX.md`

## Key Design Decisions

1. **Mutual Consent Required**: Both players must opt-in (prevents griefing)
2. **Arena Exception**: Allows organized PVP events without flag hassle
3. **Pet Protection**: Prevents indirect PVP through pets/followers
4. **Comprehensive Coverage**: All combat forms protected (not just basic attacks)
5. **Clear Messages**: Players always know why combat was blocked
6. **Config Override**: MUD admins can disable PVP entirely if desired

## Troubleshooting

**Q: Players with PVP ON can't fight**
A: Check CONFIG_PK_ALLOWED is enabled, not in ROOM_PEACEFUL

**Q: Arena not working**
A: Verify room vnums are 138600-138608

**Q: Pet can't attack players**
A: Pet master must have PVP enabled, target must have PVP enabled

**Q: Getting "Player killing not allowed"**
A: CONFIG_PK_ALLOWED is disabled, go to arena or ask admin to enable
