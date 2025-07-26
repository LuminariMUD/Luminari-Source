# CHANGELOG

## 2025-07-26

### Bug Fixes
- **Fixed Post Death Bash**: Added safety checks to `perform_knockdown()` in act.offensive.c to prevent dead characters from performing bash attacks. This fixes a race condition where mobs could bash players after the mob's death but before extraction from the game. The fix checks both position (POS_DEAD) and extraction flags (DEAD() macro) for both attacker and target.
- **Fixed Clairvoyance Fall Damage**: Modified `spell_clairvoyance()` in spells.c to use `look_at_room_number()` instead of physically moving the character to the target room. This prevents the caster from taking fall damage when casting clairvoyance on targets in falling rooms (like the eternal staircase).
