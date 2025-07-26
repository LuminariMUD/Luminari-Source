# CHANGELOG

## 2025-07-26

### Bug Fixes
- **Fixed Post Death Bash**: Added safety checks to `perform_knockdown()` in act.offensive.c to prevent dead characters from performing bash attacks. This fixes a race condition where mobs could bash players after the mob's death but before extraction from the game. The fix checks both position (POS_DEAD) and extraction flags (DEAD() macro) for both attacker and target.
- **Fixed Clairvoyance Fall Damage**: Modified `spell_clairvoyance()` in spells.c to use `look_at_room_number()` instead of physically moving the character to the target room. This prevents the caster from taking fall damage when casting clairvoyance on targets in falling rooms (like the eternal staircase).
- **Fixed Password Echo Loop**: Fixed a race condition in interpreter.c where sending password in the same packet as username could cause echo negotiation loops. Modified all password input states (CON_PASSWORD, CON_NEWPASSWD, CON_CHPWD_GETOLD, CON_DELCNF1, CON_ACCOUNT_ADD_PWD, CON_CHPWD_GETNEW) to clear pending input queue before entering password mode, preventing premature password processing.
- **Fixed Dead NPCs Taking Damage**: Fixed issue where dead NPCs continued receiving damage from DG Script area effects. Added death check (GET_POS() <= POS_DEAD || DEAD()) to script_damage() in dg_misc.c to prevent damage to already dead characters.

### Code Cleanup
- **Commented out zone loading debug messages**: Commented out "failed percentage check" log messages in db.c for 'P', 'G', and 'E' zone commands to reduce log spam during zone resets.
