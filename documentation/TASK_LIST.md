# TASK LIST - Coder ToDo List


### Magic Stone Spell
- **Description**: Magic stone gives explosion message followed by random codes and numbers
- **Notes**: Previously working, now only works on PC targets
- **Reported by**: Andross (Level 7)
- **Status**: FIXED (2025-07-26) - But new bug discovered
- **Details**: The spell is implemented and creates objects (vnums 20871 or 9401), but without access to object database files, cannot verify explosion message issues.
- **Investigation Results**: 
  - SPELL_MAGIC_STONE (spell #251) is a creation spell that creates object 20871 (CAMPAIGN_DL) or 9401 (default)
  - Object has trigger #9401 "grenade - edited for magic stone" attached
  - Trigger was malformed with multiple commands concatenated on single lines
  - Fixed trigger formatting to properly execute commands
  - **Fix**: Properly formatted the DG Script trigger to have each command on its own line

### Dead NPCs Continue Taking Damage from Area Effects
- **Description**: When multiple magic stones explode, dead NPCs show multiple death messages
- **Reported by**: Zusuk (testing magic stones)
- **Status**: NEEDS INVESTIGATION
- **Details**: When 30 magic stones exploded, killing the Elven bartender with the first explosion, subsequent explosions continued to show "The Elven bartender is dead!" messages, suggesting the corpse was still being targeted by the area damage.
- **Expected Behavior**: Dead NPCs should not be targeted by subsequent area effects
- **Actual Behavior**: Dead NPCs continue to receive damage messages from area effects until extracted
- **Location**: DG Script trigger #9401 (magic stone grenade)
- **Current Script Code**:
  ```
  set room_var %actor.room%
  wait 3 s
  %echo% ... the %self.name% explodes in a blast of colors sending shards throughout.
  set target_char %room_var.people%
  while %target_char%
    set tmp_target %target_char.next_in_room%
    %send% %target_char% The shards rip through your body.
    %damage% %target_char% 30
    set target_char %tmp_target%
  done
  %purge% %self%
  ```
- **Issue**: The script loops through all people in room without checking if they're alive
- **Potential Fix**: Add condition to check if target is alive before applying damage



