# affect_update() Performance Optimization

## Problem
The `affect_update()` function was consuming 30% CPU constantly due to inefficient processing of all characters every 6 seconds.

## Root Causes
1. **Unnecessary MSDP Updates**: Every character (including NPCs) was getting full MSDP affect updates every 6 seconds
2. **NPCs Don't Need MSDP**: NPCs don't have client connections and can't receive MSDP data
3. **No Protocol Checking**: Even players without MSDP support were getting expensive string operations
4. **Heavy String Operations**: Complex string concatenation using strlcat() for every affected character

## Optimizations Implemented

### 1. Skip MSDP Updates for NPCs (magic.c)
```c
/* Skip MSDP updates for NPCs - they don't have descriptors and can't receive MSDP data */
if (!IS_NPC(i))
  update_msdp_affects(i);
```

### 2. Early Exit Checks in update_msdp_affects() (handler.c)
```c
/* Early exit if no character, no descriptor, or character is an NPC */
if (!ch || !ch->desc || IS_NPC(ch))
  return;

/* Skip if client doesn't support MSDP */
if (!ch->desc->pProtocol || !ch->desc->pProtocol->bMSDP)
  return;
```

### 3. Performance Logging
Added logging to track the number of characters processed every 100 updates (10 minutes):
```c
/* Log performance metrics every 100 updates (10 minutes) */
if (update_count % 100 == 0)
{
  log("PERF: affect_update() - Total: %d chars (%d NPCs, %d PCs) processed",
      char_count, npc_count, pc_count);
}
```

## Expected Performance Improvement
- **NPCs**: 100% reduction in MSDP processing overhead (typically 90%+ of all characters)
- **Players without MSDP**: 100% reduction in string operations
- **Overall**: Expected 80-90% reduction in CPU usage from affect_update()

## Testing
1. Monitor CPU usage before and after changes
2. Check performance logs for character counts
3. Verify MSDP still works for players with compatible clients
4. Ensure affects still expire correctly for all characters

### 4. Skip NPCs Without Affects
Added early exit for NPCs that have no affects to avoid unnecessary processing:
```c
/* Skip characters with no affects for better performance */
if (!i->affected && IS_NPC(i))
  continue;
```

## Future Optimizations
1. Consider caching MSDP strings and only rebuilding when affects change
2. Reduce affect_update frequency or stagger updates across multiple pulses
3. Implement more efficient string building (single sprintf vs multiple strlcat)