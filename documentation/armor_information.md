# Piecemeal Armor System

Source: http://www.d20pfsrd.com/gamemastering/other-rules/piecemeal-armor

---

## Object Values
- **Value 0:** (stock) Armor Class apply (AC) — *not settable*  
- **Value 1:** Armor-type *(or switched with Value 2)* — *settable*  
- **Value 2:** *(unused/unspecified)*  
- **Value 3:** *(unused/unspecified)*  
- **Value 4:** Enhancement bonus — *settable*

---

## Builder-Set Fields
- Armor-type  
- Material (if changing from base material)  
- Enhancement bonus  
- Special abilities on armor *(not currently implemented)*  
- Custom cost (if changing from base cost)

---

## Auto-Assigned by Armor Struct
- Armor class (based on material, armor-type, and wear location)  
- Wear position  
- Base material  
- Cost  
- Maximum Dex bonus  
- Armor check penalty  
- Arcane spell failure chance  
- Weight  
- Don-time *(OPTIONAL)*  
- Remove-time *(OPTIONAL)*  
- Speed (30 ft) *(OPTIONAL)*  
- Speed (20 ft) *(OPTIONAL)*

> The armor type struct (which contains all data about various armor types) must have armor bonus **organized by wear location**, as this determines the armor bonus of body, arms, and legs.

---

## Idea
**Have the rating system suggest an enhancement bonus based on level, material, etc.**

---

## Armor Types: Protective Slots & Ratings

**Number of ‘protective’ slots, rating example (Full Plate = 8.0):**

| # | Slot  | Full Plate (8.0) |
|---|-------|-------------------|
| 1 | Head  | 1.5               |
| 2 | Body  | 3.5               |
| 3 | Arms  | 1.5               |
| 4 | Legs  | 1.5               |

---

## Armor Slots AC Distribution

**The rest of the armor slots AC (percent-based scaling example):**

| Slot            | 70  | 60  | 50  | 40  | 30  | 20  | 10  |
|-----------------|-----|-----|-----|-----|-----|-----|-----|
| Head (0.1875)   | 1.3 | 1.1 | 0.9 | 0.7 | 0.5 | 0.3 | 0.1 |
| Body (0.4375)   | 3.1 | 2.7 | 2.3 | 1.9 | 1.5 | 1.1 | 0.7 |
| Arms (0.1875)   | 1.3 | 1.1 | 0.9 | 0.7 | 0.5 | 0.3 | 0.1 |
| Legs (0.1875)   | 1.3 | 1.1 | 0.9 | 0.7 | 0.5 | 0.3 | 0.1 |

---

## Shields

| Shield  | AC  | Chk | Dex |
|---------|-----|-----|-----|
| Buckler | 0.5 | -1  | 0   |
| Medium  | 1.0 | -2  | 0   |
| Heavy   | 2.0 | -3  | 0   |
| *Tower* | 4.0 | -10 | 2   |

> Slots not listed above are considered supplemental and **should not exceed** the above numbers.

---

## Weight Distribution (lbs)

| Slot            | 50 | 45 | 40 | 25 | 15 | 10 |
|-----------------|----|----|----|----|----|----|
| Head (0.1875)   | 9  | 8  | 7  | 4  | 2  | 1  |
| Body (0.4375)   | 23 | 21 | 19 | 13 | 9  | 7  |
| Arms (0.1875)   | 9  | 8  | 7  | 4  | 2  | 1  |
| Legs (0.1875)   | 9  | 8  | 7  | 4  | 2  | 1  |

---

## Object Creation Logic
All modifications based on wear location are performed on object creation.  
**VAL0** of the object is set to the appropriate % of the Armor Bonus of the relevant armor type.

---

## Enhancement Bonus (Averaging)
Stacking four pieces of gear enhancement bonuses is problematic.  
**Solution:** Calculate the **average enhancement bonus** of all worn gear.

---

## Other Bonuses (Bonus Types)
Because armor is split across multiple slots, **handle stacking carefully** using “bonus types” (standard d20 ruleset).

- Example (typeless stacking problem):  
  - Boots: +2 AC bonus  
  - Ring of Protection #1: +3 AC bonus  
  - Ring of Protection #2: +2 AC bonus  
  - Typeless stacking = +7 total

- Example (typed bonus solution):  
  - Rings give **deflection** bonus  
  - Boots give **dodge** bonus  
  - Bonuses of the same type **do not stack** (only highest applies). Untyped and Dodge bonuses are the exceptions (they stack).  
  - Result: +3 (deflection) +2 (dodge) = +5 total

### Implementation
- `affected_type` struct gets a new field: **bonus_type** (integer).  
- Values set via `#define` statements.  
- When applying bonuses, process affects and **only apply the largest affect** per bonus type.

---

## Bonuses: Random Treasure Drops
Rules:
- **Armor pieces/weapons** (body, legs, arms, helm, shield, wield):  
  - Enhancement bonus = `level / 6` (max 5) + rarity (max 7)
- **Misc pieces**:  
  - Various bonuses, same evaluation as armor *except*  
    - +Hit-Points or +Movement-Points → 12 points per bonus point (max 96)

---

## Bonuses: In-Game Gear (Combat, Quests, Etc.)
- *(Follow similar logic to random treasure, per design needs.)*

---

## Bonuses: Crafted Gear
- **TBD** – similar to random treasure drops, but may enter **Epic or better** range with rare crafting components.

---

## More Info
Stat distribution and points value details are in the file:  
**“current stat distribution on random drops”**
