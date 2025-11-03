# LuminariMUD Shop Systems Documentation

## Overview

LuminariMUD features a comprehensive shop system that allows players to buy and sell items through various types of vendors. The system includes traditional shopkeepers, specialized vendors, pet shops, and player-owned shops. All shop systems integrate with the game's economy, including support for quest points, clan discounts, and transaction taxes.

## Table of Contents

1. [Standard Shop System](#standard-shop-system)
2. [Specialized Vendors](#specialized-vendors)
3. [Pet Shop System](#pet-shop-system)
4. [Player-Owned Shops](#player-owned-shops)
5. [Technical Implementation](#technical-implementation)
6. [Shop Configuration](#shop-configuration)
7. [Clan Economy Integration](#clan-economy-integration)

## Standard Shop System

### Core Features

The standard shop system is implemented in `src/shop.c` and `src/shop.h` using the `shop_keeper` special procedure.

#### Available Commands

- **list [item]** - Display items for sale, optionally filtered by name
- **buy <item>** - Purchase an item from the shop
- **sell <item>** - Sell an item to the shopkeeper
- **value <item>** - Check the sell price of an item
- **identify <item>** - View detailed stats of an item for sale

#### Shop Characteristics

- **Operating Hours**: Shops can have two operating periods per day
- **Customer Restrictions**: Can restrict by alignment, class, or race
- **Banking System**: Shops can use banking to manage large amounts of gold
- **Production**: Shops can produce unlimited quantities of certain items
- **Sorting**: Automatically groups identical items in inventory

### Shop Flags

Located in `src/shop.h:172-177`:

- **WILL_START_FIGHT** - Shopkeeper will attack thieves
- **WILL_BANK_MONEY** - Uses banking system for gold over 15,000
- **HAS_UNLIMITED_CASH** - Disabled in current implementation (per code comment)
- **BLACK_MARKET_SHOP** - Only trades with criminals (requires FEAT_BG_CRIMINAL)
- **NOBLE_SHOP** - Only trades with nobility (requires FEAT_BG_NOBLE)

### Pricing System

#### Buy Price Formula
```c
price = base_cost * shop_buyprofit * charisma_modifier
```

The charisma modifier includes:
- Shopkeeper's CHA + Appraise skill
- Buyer's CHA + Appraise skill
- -10 bonus for Folk Hero or Noble backgrounds in hometown
- Clan shop discounts (if applicable)

#### Sell Price Formula
```c
price = base_cost * shop_sellprofit * charisma_modifier
```

Maximum sell price is capped at the buying price to prevent exploits.

### Trading Restrictions

Shops can restrict customers based on:

#### Alignment
- TRADE_NOGOOD
- TRADE_NOEVIL
- TRADE_NONEUTRAL

#### Classes
- TRADE_NOWIZARD, TRADE_NOCLERIC, TRADE_NOROGUE
- TRADE_NOWARRIOR, TRADE_NOMONK, TRADE_NOBERSERKER
- TRADE_NODRUID (defined but not implemented), TRADE_NOSORCERER, TRADE_NOPALADIN
- TRADE_NORANGER, TRADE_NOBARD (defined but not implemented), TRADE_NOWEAPONMASTER

**Note:** DRUID and BARD class restrictions are defined in shop.h but not fully implemented in the validation code.

#### Races
- TRADE_NOHUMAN, TRADE_NOELF, TRADE_NODWARF
- TRADE_NOHALFTROLL, TRADE_NOHALFLING
- TRADE_NOH_ELF, TRADE_NOH_ORC, TRADE_NOGNOME
- TRADE_NOARCANAGOLEM, TRADE_NODROW, TRADE_NODUERGAR

## Specialized Vendors

### Weapon Vendors (buyweapons)

Located in `src/spec_procs.c:10631`

Specialized vendors that sell weapons based on vendor level:
- **Level 1-10**: Mundane and masterwork weapons
- **Level 11-15**: +1 magical weapons
- **Level 16-20**: +2 magical weapons
- **Level 21-25**: +3 magical weapons
- **Level 26-30**: +4 magical weapons
- **Level 31+**: +5 magical weapons

**Commands:**
- `list [mundane|masterwork]` - View available weapons
- `buy [mundane|masterwork] <weapon_name>` - Purchase a weapon

**Pricing:**
- Masterwork weapons cost 300 gold more than mundane
- Magical weapons have enhanced pricing based on bonus level

### Armor Vendors (buyarmor)

Located in `src/spec_procs.c:10450`

Similar to weapon vendors but for armor:
- Sells body, arms, legs, head armor and shields
- Enhancement bonuses up to +4 (stops at level 30, unlike weapons which go to +5)
- Masterwork armor costs 50 gold more per piece (200 for shields)

**Commands:**
- `list [mundane|masterwork] [body|arms|legs|head|shield]` - View available armor
- `buy [mundane|masterwork] <armor_name>` - Purchase armor

### Crafting Mold Vendors (buymolds)

Located in `src/spec_procs.c:10910`

Sell crafting molds for the crafting system:
- **Weapon molds** - For crafting weapons
- **Armor molds** - For crafting armor pieces
- **Accessory molds** - For rings, bracers, belts, boots, gloves, necklaces, cloaks

**Commands:**
- `list [weapons|armor|accessories]` - View available molds
- `buy <mold_name>` - Purchase a mold for 100 gold

## Pet Shop System

### Pet Shop Special Procedure

Located in `src/spec_procs.c:5906`

Pet shops allow players to purchase animal companions. The shop reads available pets from the room adjacent to the shop (room + 1).

**Commands:**
- `list` - View available pets and prices
- `buy <pet_name> [custom_name]` - Purchase a pet, optionally with a custom name

**Pricing:** `Level * 300 gold coins`

**Pet Stats Adjustments:**
Based on pet level and configuration values:
- Levels 1-10: Uses CONFIG_SUMMON_LEVEL_1_10_* modifiers
- Levels 11-20: Uses CONFIG_SUMMON_LEVEL_11_20_* modifiers
- Levels 21-30: Uses CONFIG_SUMMON_LEVEL_21_30_* modifiers

These modifiers apply to:
- Hit points (HP)
- Armor class (AC)
- Hit and damage rolls
- Damage dice

### Pet Object System (bought_pet)

Located in `src/spec_procs.c:9991`

Alternative pet system using objects that convert to mobile followers when purchased through regular shops.

**How it works:**
1. Object VNUM must match the desired mobile VNUM
2. When a player purchases an object with the `bought_pet` special procedure
3. The object is automatically converted to a mobile follower
4. The mobile is loaded from the matching VNUM and becomes the player's pet
5. The object is removed from the player's inventory

## Player-Owned Shops

Located in `src/spec_procs.c:1703`

Player-owned shops are tied to the house system, allowing players to sell items from their house storage.

**Features:**
- Items are stored in the house's private room
- Automatic gold collection goes to house storage
- Supports item identification before purchase
- Transaction saving prevents duplication exploits

**Commands:**
- `list` - View items for sale
- `buy <#|item_name>` - Purchase an item
- `identify <#|item_name>` - Examine an item before buying

**Setup Requirements:**
- House must have an atrium room
- Shop keeper must be assigned the `player_owned_shops` special procedure
- Items for sale must be placed in the house storage room

## Technical Implementation

### Shop Data Structure

From `src/shop.h:32-56`:

```c
struct shop_data {
    room_vnum vnum;              // Virtual number of shop
    obj_vnum *producing;         // Items produced (unlimited)
    float profit_buy;            // Buy price multiplier
    float profit_sell;           // Sell price multiplier
    struct shop_buy_data *type;  // Item types bought
    char *no_such_item1;         // Message when keeper lacks item
    char *no_such_item2;         // Message when player lacks item
    char *missing_cash1;         // Message when keeper lacks gold
    char *missing_cash2;         // Message when player lacks gold
    char *do_not_buy;           // Message for refused items
    char *message_buy;          // Purchase message
    char *message_sell;         // Sale message
    int temper1;                // Reaction to broke customers
    bitvector_t bitvector;      // Shop flags
    mob_rnum keeper;            // Shopkeeper mob rnum
    int with_who;               // Customer restrictions
    room_vnum *in_room;         // Shop room(s)
    int open1, open2;           // Opening hours
    int close1, close2;         // Closing hours
    int bankAccount;            // Banked gold
    int lastsort;               // Inventory sort counter
    SPECIAL(*func);             // Secondary special procedure
};
```

### Shop Loading Process

1. **boot_the_shops()** (`src/shop.c:1424`) - Loads shop definitions from files
2. **assign_the_shopkeepers()** (`src/shop.c:1500`) - Assigns shop_keeper spec_proc to mobs
3. Shop data stored in global `shop_index` array

### Shop File Format

Shops use a versioned file format (v3.0) with the following structure:
- Shop number (#VNUM)
- Producing items list
- Buy/sell profit multipliers
- Item types bought
- Shop messages (7 different types)
- Temperament and flags
- Keeper mob VNUM
- Trading restrictions
- Room locations
- Operating hours

### Important Macros

From `src/shop.h:125-142`:

- `SHOP_NUM(i)` - Shop virtual number
- `SHOP_KEEPER(i)` - Shopkeeper mob rnum
- `SHOP_PRODUCT(i, num)` - Produced item
- `SHOP_BUYTYPE(i, num)` - Bought item type
- `SHOP_BUYPROFIT(i)` - Buy price multiplier
- `SHOP_SELLPROFIT(i)` - Sell price multiplier
- `SHOP_BITVECTOR(i)` - Shop flags
- `SHOP_TRADE_WITH(i)` - Customer restrictions

## Shop Configuration

### Global Settings

From `src/shop.h:183-184`:
- **MIN_OUTSIDE_BANK**: 5000 gold (withdrawal threshold)
- **MAX_OUTSIDE_BANK**: 15000 gold (deposit threshold)

### Standard Messages

Located in `src/shop.h:186-195`:
- MSG_NOT_OPEN_YET
- MSG_NOT_REOPEN_YET
- MSG_CLOSED_FOR_DAY
- MSG_NO_STEAL_HERE
- MSG_NO_SEE_CHAR
- MSG_NO_SELL_ALIGN
- MSG_NO_SELL_CLASS
- MSG_NO_SELL_RACE
- MSG_NO_USED_WANDSTAFF
- MSG_CANT_KILL_KEEPER

### Shop Assignment

Shops are assigned to mobs in `src/spec_assign.c`:

```c
// Standard shops use shop files
// Specialized vendors use direct assignment:
ASSIGNMOB(vnum, buyweapons);
ASSIGNMOB(vnum, buyarmor);
ASSIGNMOB(vnum, buymolds);
ASSIGNMOB(vnum, player_owned_shops);

// Pet shops assigned to rooms:
ASSIGNROOM(vnum, pet_shops);
```

**Standard Shop Assignment Process:**
1. Shop definitions loaded from shop files during boot
2. Shopkeeper mobs automatically get `shop_keeper` special procedure
3. Mobs receive `MOB_CUSTOM_GOLD` and `MOB_NO_AI` flags
4. Shopkeepers start with 100,000 gold by default

## Clan Economy Integration

The shop system integrates with the clan economy (`src/clan_economy.c`):

### Features

1. **Clan Discounts** (`src/shop.c:553`)
   - Members get discounts at clan-affiliated shops
   - Discount rates configurable per clan

2. **Transaction Taxes** (`src/shop.c:740,1000`)
   - Clan collects taxes on member transactions
   - Applied to both buying and selling
   - Tax rates configurable per clan

3. **Transaction Types**
   - TRANS_SHOP_BUY - Buying from shops
   - TRANS_SHOP_SELL - Selling to shops
   - TRANS_PLAYER_TRADE - Player-to-player trades
   - TRANS_AUCTION - Auction house transactions

### API Functions

- `apply_clan_shop_discount(price, ch, shop_nr)` - Apply clan discount
- `collect_clan_transaction_tax(ch, amount, type)` - Collect transaction tax

## Usage Examples

### Creating a Standard Shop

1. Create shop definition in world files
2. Assign `shop_keeper` spec_proc to mob
3. Configure shop parameters (hours, restrictions, messages)
4. Add items to produce or buy lists

### Setting Up a Pet Shop

1. Create two adjacent rooms
2. Place pets in the second room
3. Assign `pet_shops` spec_proc to first room
4. Pets automatically priced by level

### Configuring Player Shop

1. Player must own a house
2. Assign mob with `player_owned_shops` spec_proc
3. Place mob in house atrium
4. Items in house storage become available for sale

## Debugging

### Debug Macros

- `PLAYER_SHOP_DEBUG` - Enables debug output for player shops

### Common Issues

1. **Shop not trading**: Check customer restrictions and operating hours
2. **Items not appearing**: Verify shop production list or keeper inventory
3. **Pricing issues**: Check charisma/appraise modifiers and shop profit settings
4. **Pet shop empty**: Ensure pets are in room adjacent to shop

## Known Issues and Bugs

### Implementation Bugs

1. **Noble Shop Flag Bug** (`src/shop.c:150`)
   - The code incorrectly checks `BLACK_MARKET_SHOP` flag when validating `NOBLE_SHOP` access
   - This causes noble shops to not work correctly
   - Fix: Line 150 should check `IS_SET(SHOP_BITVECTOR(shop_nr), NOBLE_SHOP)`

2. **Missing Class Restrictions**
   - `TRADE_NODRUID` is defined but not implemented in validation checks
   - `TRADE_NOBARD` is defined but not implemented in validation checks
   - These restrictions won't actually prevent druids/bards from shopping

### Documentation Notes

- Some features mentioned in code comments may not be fully implemented
- The `HAS_UNLIMITED_CASH` flag exists but is intentionally disabled
- Armor vendors have a lower maximum enhancement level (+4) compared to weapons (+5)

## See Also

- House System Documentation
- Crafting System Documentation
- Clan System Documentation
- Object and Mobile Creation Documentation