# Artisan Points System - Simplified Supply Orders

## Overview
The supply order system has been simplified to remove multiple contract types and replace them with a straightforward artisan points reward system. Players now accumulate artisan points by completing supply orders, which can be used for various purposes in the future.

## Changes Made

### 1. Added Artisan Points Tracking

**File: `src/structs.h`**
- Added `artisan_exp` field to `struct char_point_data` (line ~4780)
- Stores accumulated artisan points for each character

**File: `src/utils.h`**
- Added `GET_ARTISAN_EXP(ch)` macro to access artisan points

**File: `src/players.c`**
- Added save/load support for artisan points:
  - Save: `AExp: %d` tag (line ~2183)
  - Load: Parses `AExp` tag (line ~736)

### 2. Simplified Supply Order System

**File: `src/crafting_new.c`**

#### `complete_supply_order()` Function
- **Removed**: Complex contract type-based reward multipliers
- **Removed**: Reputation point rewards based on contract types
- **Added**: Simple artisan points calculation: `quantity * 10`
- **Updated**: Display message shows artisan points earned and total

#### `request_new_supply_order()` Function
- **Removed**: Contract type selection logic
- **Simplified**: Quantity generation now uses `dice(1, 3) + 1` (2-4 items)
- **Added**: Message informing players about artisan point rewards

#### `show_available_contracts()` Function
- **Completely Simplified**: Removed complex contract listing
- **New Behavior**: Shows simple information screen:
  - How to request a supply order
  - Current artisan points total
  - Note about future uses for artisan points

#### `calculate_supply_order_reward()` Function
- **Removed**: All contract type-based bonus multipliers:
  - Rush, Bulk, Quality, Prestige, Event bonuses
  - Reputation rank bonuses
- **Simplified**: Returns base reward calculation only

### 3. Contract Types (Now Deprecated)

The following contract types are no longer used but remain defined for backwards compatibility:
- `SUPPLY_CONTRACT_BASIC` (1)
- `SUPPLY_CONTRACT_RUSH` (2)
- `SUPPLY_CONTRACT_BULK` (3)
- `SUPPLY_CONTRACT_QUALITY` (4)
- `SUPPLY_CONTRACT_PRESTIGE` (5)
- `SUPPLY_CONTRACT_EVENT` (6)

## Player-Facing Changes

### Supply Order Commands
- `supplyorder list` - Shows simplified info screen with current artisan points
- `supplyorder request` - Gets a random supply order (2-4 items to craft)
- `supplyorder complete` - Completes order and awards artisan points

### Rewards
Players completing a supply order now receive:
1. **Gold**: Based on materials, quantity, and skill (unchanged)
2. **Experience**: Half of gold reward, capped at 250 (unchanged)
3. **Artisan Points**: 10 points per item crafted (NEW)

### Example Output
```
Congratulations! You have completed your supply order.
You receive 150 gold coins, 75 bonus experience points, and 30 artisan points!
You now have 120 total artisan points.
```

## Future Enhancements

The artisan points system is designed to support future features such as:
- Special crafting recipes unlockable with artisan points
- Unique cosmetic items
- Crafting bonuses or buffs
- Access to rare materials
- Crafting station upgrades
- Special titles or achievements

## Technical Notes

### Data Storage
- Artisan points are stored in the player file with tag `AExp`
- Format: `AExp: <number>`
- Default value: 0 (not saved if 0)

### Calculation
- Base artisan points per item: 10
- Formula: `artisan_points = supply_num_required * 10`
- No diminishing returns or caps

### Backwards Compatibility
- Existing supply order data structures remain unchanged
- Old contract type fields are ignored but not removed
- Players with existing supply orders can complete them normally

## Files Modified

1. `src/structs.h` - Added artisan_exp field
2. `src/utils.h` - Added GET_ARTISAN_EXP macro
3. `src/players.c` - Added save/load for artisan points
4. `src/crafting_new.c` - Simplified supply order system

## Compilation Status

✅ Successfully compiled with no errors or warnings
✅ All changes are backwards compatible with existing player data
✅ System is ready for testing

## Testing Checklist

- [ ] Request a new supply order
- [ ] Complete a supply order and verify artisan points awarded
- [ ] Check artisan points are saved/loaded correctly
- [ ] Verify `supplyorder list` shows current points
- [ ] Test with multiple supply orders to accumulate points
- [ ] Confirm existing supply orders can still be completed

---
**Last Updated**: October 17, 2025
**Status**: ✅ Complete and Compiled
