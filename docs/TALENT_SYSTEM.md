# Talent System Documentation

## Overview
The talent system provides progression rewards for advancing crafting and harvesting skills. Players earn 1 talent point each time they increase any crafting or harvesting skill level.

## Core Mechanics

### Earning Talent Points
- **Trigger**: Gain 1 talent point per crafting/harvesting skill level increase
- **Function**: `gain_talent_point(ch, amount)` in talents.c
- **Storage**: `GET_TALENT_POINTS(ch)` in player file (Tlpt: tag)

### Learning Talents
- **Cost Formula**: Costs DOUBLE each rank
  - Rank 1: base cost
  - Rank 2: base × 2
  - Rank 3: base × 4
  - Rank 4: base × 8
  - Rank 5: base × 16
  - etc.
- **Requirements**: 
  - Sufficient talent points
  - Sufficient gold
  - Talent must be in_game (enabled)
  - Not at max rank

### Data Storage
- **Talent Points**: Saved in pfile with `Tlpt:` tag
- **Talent Ranks**: Saved in pfile with `Tlrk:` tag (64 entries)
- **Legacy Support**: Old bitset format (Tlbt:) maintained for backwards compatibility

## Player Commands

### `talents`
Lists all available talent categories with talent counts.
- Shows how many talents are in each crafting/harvesting category
- Clean organized view by profession
- Starting point for browsing talents

### `talents <category>`
Lists all talents in a specific category **alphabetically in two columns**.
- Example: `talents woodworking`, `talents alchemy`, `talents mining`
- Shows only talents for that specific crafting/harvesting skill
- Supports abbreviations (e.g., `talents wood`, `talents alch`)
- Displays current ranks, costs in compact format: `Rank TP/GP`
- Color-coded status (learned, available, max)

### `talents available`
Lists only talents the player can currently afford, **sorted alphabetically in two columns**.
- Filters by talent points and gold available
- Shows only learnable talents across all categories
- Compact format: `ID) Name Rank TP/GP`

### `talents all`
Lists ALL active talents **alphabetically in two columns**.
- Shows only enabled (in-game) talents
- Useful for seeing complete list without filtering by category or affordability
- Compact two-column format

### `talents info <number>`
Shows detailed information about a specific talent.
- Displays talent name, description, and current/max rank
- Shows whether you can currently learn it
- **Multi-column cost progression** showing all ranks with TP/GP costs
- Color-coded ranks: Green=learned, Yellow=next, White=future
- Costs displayed in 3 columns for easy reading

### `talents info <number>`
Shows detailed information about a specific talent.
- Full description
- Current rank vs max rank
- Cost progression for all ranks
- Whether player can learn it

### `talents learn <number>`
Learn or rank up a talent.
- Deducts talent points and gold
- Increases talent rank by 1
- Shows confirmation message with costs

## Admin Commands

### `talento <player> points [amount]`
View or set a player's talent points.
- No amount: shows current points
- With amount: sets points to that value
- Logs to mudlog

### `talento <player> <talent_num> [rank]`
View or set a player's talent rank.
- No rank: shows current rank for that talent
- With rank: sets talent to that rank (0-255)
- Logs to mudlog

## Talent Categories

Talents are organized by crafting/harvesting profession for easy browsing:

### Crafting Categories
- **Woodworking** (34) - Carpentry and woodcraft
- **Tailoring** (35) - Cloth and fabric work
- **Alchemy** (36) - Potion and elixir creation
- **Armorsmithing** (37) - Armor forging
- **Weaponsmithing** (38) - Weapon forging
- **Bowmaking** (39) - DISABLED - reserved for future
- **Jewelcrafting** (40) - Gems and jewelry
- **Leatherworking** (41) - Leather crafting
- **Trapmaking** (42) - DISABLED - reserved for future
- **Poisonmaking** (43) - DISABLED - reserved for future
- **Metalworking** (44) - General metalwork
- **Fishing** (45) - DISABLED - reserved for future
- **Cooking** (46) - DISABLED - reserved for future

### Harvesting Categories
- **Mining** (48) - Ore and mineral extraction
- **Hunting** (49) - Animal tracking and harvesting
- **Forestry** (50) - Wood and timber harvesting
- **Gathering** (51) - Herb and plant gathering

### General Category
- **General** - Reserved for talents not tied to specific skills

### Talent Types Per Category
Each active crafting/harvesting skill has 5 talent types:
1. **Proficient** - +1 to skill checks (1pt/1000gp, max 5 ranks)
2. **Rapid** - -1 second task time (1pt/1000gp, max 5 ranks)
3. **Expertise** - +1% crit chance (2pt/2500gp, max 5 ranks)
4. **Efficient** - Material savings chance (1pt/2500gp, max 10 ranks)
5. **Insightful** - Extra XP chance (2pt/5000gp, max 5 ranks)

## Cost Examples

### Sample Display Output

#### `talents` command (category list):
```
Talent Categories
==============================================================================
You have 5 unspent talent points.  Gold: 25000

  Woodworking          - 5 talents
  Tailoring            - 5 talents
  Alchemy              - 5 talents
  Armorsmithing        - 5 talents
  Weaponsmithing       - 5 talents
  Jewelcrafting        - 5 talents
  Leatherworking       - 5 talents
  Metalworking         - 5 talents
  Mining               - 5 talents
  Hunting              - 5 talents
  Forestry             - 5 talents
  Gathering            - 5 talents

Use 'talents <category>' to view talents in a specific category.
Example: 'talents woodworking' or 'talents alchemy'
```

#### `talents alchemy` command (category view - two columns):
```
Alchemy Talents (Alphabetical)
==============================================================================
You have 5 unspent talent points.  Gold: 25000

12) Alchemy - Efficient        0/10  1pt/2500gp    15) Alchemy - Proficient       2/5   4pt/4000gp
14) Alchemy - Expertise        0/5   2pt/2500gp    16) Alchemy - Rapid            0/5   1pt/1000gp
13) Alchemy - Insightful       0/5   2pt/5000gp

Use 'talents learn <number>' to learn or rank up a talent.
Use 'talents info <number>' to see details about a talent.
Use 'talents' to see all categories.
```

#### `talents info 15` command (multi-column format):
```
=== Alchemy - Proficient ===
Status: [ACTIVE]
Your Rank: 2/5
Next Rank Cost: 4 talent points and 4000 gold
Can Learn: YES

Description:
+1 to skill checks for the alchemy skill

Cost Progression (Rank: TP/GP):
  Rank 1: 1/1000           Rank 2: 2/2000           Rank 3: 4/4000
  Rank 4: 8/8000           Rank 5: 16/16000
```

### Proficient Talents (base: 1pt, 1000gp, max 5)
- Rank 1: 1 pt, 1,000 gp
- Rank 2: 2 pt, 2,000 gp
- Rank 3: 4 pt, 4,000 gp
- Rank 4: 8 pt, 8,000 gp
- Rank 5: 16 pt, 16,000 gp
- **Total**: 31 talent points, 31,000 gold

### Expertise Talents (base: 2pt, 2500gp, max 5)
- Rank 1: 2 pt, 2,500 gp
- Rank 2: 4 pt, 5,000 gp
- Rank 3: 8 pt, 10,000 gp
- Rank 4: 16 pt, 20,000 gp
- Rank 5: 32 pt, 40,000 gp
- **Total**: 62 talent points, 77,500 gold

### Efficient Talents (base: 1pt, 2500gp, max 10)
- Rank 1: 1 pt, 2,500 gp
- Rank 2: 2 pt, 5,000 gp
- Rank 3: 4 pt, 10,000 gp
- Rank 4: 8 pt, 20,000 gp
- Rank 5: 16 pt, 40,000 gp
- Rank 6: 32 pt, 80,000 gp
- Rank 7: 64 pt, 160,000 gp
- Rank 8: 128 pt, 320,000 gp
- Rank 9: 256 pt, 640,000 gp
- Rank 10: 512 pt, 1,280,000 gp
- **Total**: 1,023 talent points, 2,557,500 gold

## Implementation Details

### Key Functions
- `init_talents()` - Initialize all talent definitions
- `get_talent_rank(ch, talent)` - Get current rank
- `has_talent(ch, talent)` - Check if learned (rank > 0)
- `talent_max_ranks(talent)` - Get max possible rank
- `talent_next_point_cost(ch, talent)` - Calculate next rank point cost
- `talent_next_gold_cost(ch, talent)` - Calculate next rank gold cost
- `can_learn_talent(ch, talent)` - Check if player can afford next rank
- `learn_talent(ch, talent)` - Learn/rank up talent (deducts costs)
- `gain_talent_point(ch, amount)` - Award talent points

### Files Modified
- `src/talents.c` - Implementation
- `src/talents.h` - Declarations and constants
- `src/interpreter.c` - Command registration
- `src/players.c` - Already had save/load support
- `src/structs.h` - Already had player_special_data_saved fields

### Database Storage
Talent data is stored in ASCII player files:
```
Tlpt: <talent_points>
Tlrk: <rank0> <rank1> ... <rank63>
Tlbt: <bits1> <bits2>  # legacy bitset for compatibility
```

## Integration Points

### When Talent Points Are Awarded

The `gain_talent_point()` function is **already integrated** into the crafting system and is called automatically whenever:
- A crafting skill increases (abilities 34-46)
- A harvesting skill increases (abilities 48-51)

**Implementation location:**
- `src/crafting_new.c` - Lines 4070-4084 in the `award_crafting_skill_exp()` function

```c
if (GET_CRAFT_SKILL_EXP(ch, abil) >= craft_skill_level_exp(ch, get_craft_skill_value(ch, abil)+1))
{
    send_to_char(ch, "\tYYour skill in '%s' has increased from %d to %d!\r\n\tn", 
                 ability_names[abil], get_craft_skill_value(ch, abil), 
                 get_craft_skill_value(ch, abil)+1);
    SET_ABILITY(ch, abil, get_craft_skill_value(ch, abil)+1);
    /* Award 1 crafting talent point per crafting/harvesting skill level up */
    gain_talent_point(ch, 1);
}
```

**What this means:**
- Every time a player gains a level in any crafting or harvesting skill through normal gameplay (crafting items, harvesting resources, etc.), they automatically receive 1 talent point
- No additional integration needed - the system is fully functional!
- Players will see: "You gain 1 crafting talent point!" message when their skill increases

### Checking Talent Effects
To check if a player has a talent and apply its effects:
```c
int proficient_ranks = get_talent_rank(ch, TALENT_PROFICIENT_WOODWORKING);
if (proficient_ranks > 0) {
  skill_bonus += proficient_ranks; // Each rank gives +1
}
```

## Future Enhancements

### Possible Additions
1. Add talent prerequisites (require certain skill levels)
2. Add talent trees (require other talents first)
3. Add talent synergies (bonuses when multiple talents learned)
4. Add talent refund/respec system
5. Enable currently disabled skills (bowmaking, trapmaking, etc.)
6. Add more talent types (legendary, master, grandmaster)

### Balancing Notes
- The doubling cost creates exponential growth making high ranks very expensive
- This is intentional to create meaningful choices
- Players must decide between broad (many talents) vs deep (high ranks)
- Gold costs ensure economic engagement with crafting profits
- Consider talent point earning rate when adding new sources
