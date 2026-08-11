# Shop File (`.shp`) Format Reference

Shop files live in `lib/world/shp/` and are read by `boot_the_shops()` in
`src/obj/shop.c`. A shop binds a shopkeeper mobile to a room, a price
structure, a list of what it stocks, and a list of what it will buy.

Shops are edited in-game with `sedit` and listed with `slist`. Hand-editing is
supported but the format is unusually positional - see the warnings below.

## File Layout

```
CircleMUD v3.0 Shop File~
#<shop vnum>~
<producing list>
<buy profit>
<sell profit>
<buy-type list>
<message: no such item, shop side>~
<message: no such item, player side>~
<message: will not buy>~
<message: player cannot afford>~
<message: shop cannot afford>~
<message: buy success>~
<message: sell success>~
<temper when broke>
<shop flags bitvector>
<shopkeeper mob vnum>
<who not to trade with bitvector>
<room list>
<open time 1>
<close time 1>
<open time 2>
<close time 2>
$~
```

Every string field is terminated by `~`. Numeric fields are one per line.

### The version tag is load-bearing

The first line **must** contain the literal substring `v3.0`. The parser scans
each non-`#` line for it and sets `new_format = TRUE` when found. Without it,
the file is read in the pre-v3.0 format, where the producing and buy-type lists
are **fixed-length**: exactly `MAX_PROD` (5) and `MAX_TRADE` (5) entries, with
no `-1` terminator. Getting this wrong does not produce a clean error - it
misaligns every field after the lists and the shop loads as nonsense.

Write the standard header line and never remove it:

```
CircleMUD v3.0 Shop File~
```

### Real example

`lib/world/shp/1108.shp`, first record:

```
CircleMUD v3.0 Shop File~
#110801~
-1
1.00
1.00
-1
%s I can't let you buy that!  You aren't a &cmPurple Dragon&c0.~
%s You don't seem to have that.~
%s I can't be buying them kind.~
%s I can't afford that!~
%s Bah.  Come back when you have some real money.~
%s That'll be %d coins, thanks.~
%s I'll give you %d coins for that.~
0
0
110805
0
110852
-1
0
28
0
0
```

This shop stocks nothing (`-1` immediately), buys nothing (`-1` immediately),
sells and buys at face value, is kept by mob 110805 in room 110852, and is open
from hour 0 to hour 28 - that is, always.

## Field Reference

### Shop vnum

```
#<vnum>~
```

Note the trailing `~`. The header is read with `fread_string()`, so unlike zone
and world files it is a tilde-terminated string, not a bare line.

### Producing list

The vnums of objects the shop keeps in infinite supply. One vnum per line,
terminated by a line containing `-1`:

```
3020
3021
-1
```

A shop that produces nothing is a single `-1` line. Objects the shop has bought
from players are sold from actual inventory and do not belong here.

### Buy and sell profit

Two floating-point multipliers, in that order:

- **Buy profit** multiplies the price the *player pays*. `1.00` is face value;
  `1.50` marks everything up by half.
- **Sell profit** multiplies the price the *player receives*. `1.00` is face
  value; `0.50` halves it.

Write them with a decimal point (`1.00`, not `1`).

### Buy-type list

What the shop is willing to purchase. Terminated by `-1`. Each entry is an item
type, optionally followed by a keyword that further restricts the match:

```
Weapon
Armor/Shield sword
-1
```

The type may be written either as the **name** exactly as it appears in
`item_types[]` (see the [OEDIT Guide](OEDIT_GUIDE.md#item-types-reference)) or
as its **number**. Text after the type name is taken as a required keyword.
Anything after a `;` on the line is a comment and is discarded.

An unrecognised type name that is also not a number logs
`SYSERR: Invalid shop buy-type line` and is counted as an error.

### Messages

Seven tilde-terminated strings, in this fixed order:

| # | Sent when |
|---|-----------|
| 1 | The shop does not stock the requested item |
| 2 | The player does not have the item they are trying to sell |
| 3 | The shop will not buy this kind of item |
| 4 | The shop cannot afford the purchase |
| 5 | The player cannot afford the purchase |
| 6 | A purchase succeeds |
| 7 | A sale succeeds |

`%s` expands to the player's name. Messages 6 and 7 additionally take a
`%d` for the coin amount. Getting the count or order wrong shifts every
subsequent numeric field, so count your tildes.

### Temper when broke

An integer selecting the shopkeeper's reaction when it runs out of money.

### Shop flags

A bitvector controlling shopkeeper behavior:

| Bit value | Constant | Effect |
|-----------|----------|--------|
| 1 | `WILL_START_FIGHT` | The shopkeeper will attack thieves |
| 2 | `WILL_BANK_MONEY` | Excess gold is moved to the shop's bank |
| 4 | `HAS_UNLIMITED_CASH` | Reserved; unlimited-cash behavior is disabled |
| 8 | `BLACK_MARKET_SHOP` | Requires the criminal background |
| 16 | `NOBLE_SHOP` | Requires the noble background |
| 32 | `ROAMING_SHOP` | Operates wherever the shopkeeper currently is |

`ROAMING_SHOP` is a conversion compatibility flag for legacy shops that follow their
keeper instead of operating in a fixed room. Such a shop may have an empty room list;
`sedit` displays and persists the flag like the other shop flags.

### Shopkeeper mob vnum

The vnum of the mobile that runs the shop. `boot_the_shops()` converts it to a
real number immediately; `assign_the_shopkeepers()` then attaches the
`shop_keeper` special procedure to that prototype and sets `MOB_CUSTOM_GOLD`
and `MOB_NO_AI` on it, along with 100,000 gold.

Two consequences worth knowing:

- **You do not assign a spec-proc to a shopkeeper yourself.** It is done for
  you. Any callback the mob already had is saved as one runtime-only secondary and runs before shop
  behavior. This compatibility wrapper is not a persisted multiple-procedure chain.
- **The shopkeeper's gold in the `.mob` file is ignored.** It is overwritten at
  boot.

A shopkeeper vnum past the end of the mobile table calls `abort()`. This is one
of the few world-data errors that crashes rather than exiting cleanly.

### Who not to trade with

A bitvector of `TRADE_NO*` values (`src/obj/shop.h`). Bit 0 is `TRADE_NOGOOD`,
bit 1 `TRADE_NOEVIL`, bit 2 `TRADE_NONEUTRAL`, then the class restrictions from
bit 3, then the race restrictions from bit 15. `0` means the shop trades with
everyone.

### Room list

The vnums of rooms this shop operates in, terminated by `-1`. In practice this
is one room, matching where the shopkeeper is loaded by the zone file.

### Hours

Four integers: open time 1, close time 1, open time 2, close time 2. These are
game hours, allowing a shop to close for a midday break. A shop that is always
open uses `0` and `28` for the first pair and `0`/`0` for the second, since the
game day does not reach hour 28.

## Validation and Lookup

Validate the zone package containing the shop after saving it. The normal
indexed world remains available for keeper, product, room, and item-type
checks:

```sh
python3 scripts/world/wtool.py validate --zone 30
```

Shop vnums can overlap room, mobile, and object vnums, so lookup is always
typed. `show` displays the normalized shop record; `refs` displays its outgoing
products, keeper, and room references plus incoming references from other
records:

```sh
python3 scripts/world/wtool.py show shop 3000
python3 scripts/world/wtool.py refs shop 3000
```

See the [World Validator CLI](../utilities/WORLD_VALIDATOR_CLI.md) for strict
mode, JSON output, and staging-world selection.

## Related

- [Zone File Format Reference](ZONE_FILE_FORMAT.md) - the shopkeeper must be
  loaded into the shop's room by an `M` reset command
- [OEDIT Guide](OEDIT_GUIDE.md) - item types used in the buy-type list
- [OLC System](../systems/OLC_ONLINE_CREATION_SYSTEM.md) - `sedit` and `slist`
