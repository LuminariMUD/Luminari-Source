# Alchemist Perk Trees - Pathfinder Expertise System

## Overview

Alchemists in Pathfinder are scientific experimenters who mix mutagens, craft alchemical items, and discover extraordinary abilities through their research. These three perk trees represent the core pillars of the Pathfinder Alchemist: personal enhancement through mutagens, specialized discoveries that unlock unique powers, and the art of crafting powerful extracts (alchemical spells). Each tree has four tiers of progression, with powerful capstones at Tier IV.

**The Three Trees:**
1. **Mutagenist** – Mutagen crafting, personal enhancement, and physical transformation
2. **Bomb Craftsman** – Alchemical bomb creation, elemental infusions, and tactical explosives
3. **Extract Master** – Alchemical extracts (bottled spells), infusions, and arcane knowledge

---

## TREE 1: MUTAGENIST
*Unlock your body's potential through alchemical transformation. Craft mutagens to enhance strength, dexterity, and other capabilities.*

### TIER I – Cost: 1 point each

#### Mutagen I
- **Max Ranks:** 3
- **Description:** Your mutagens give +1 to str, con and dex per rank.
- **Mechanics:** When you drink a mutagen you crafted, apply an alchemical bonus of +1 per rank to STR, DEX, and CON (all three attributes). The bonus updates all derived stats (attack, AC from DEX, HP from CON) immediately and ends with the mutagen. Alchemical ability bonuses from other sources do not stack; use the highest.
- **Effect Type:** Personal enhancement buff
- **Prerequisites:** None

#### Hardy Constitution I
- **Max Ranks:** 3
- **Description:** When you drink a mutagen you created, gain +1 max p HP per character level per rank, lasting for the mutagen's duration.
- **Mechanics:** On consuming your own mutagen, increase maximum HP by (character level × ranks) and immediately raise current HP by the same amount (this is bonus max HP, not temporary HP). The bonus persists only while the mutagen is active; when it ends, reduce max HP by the same amount and lower current HP if needed (never below 1). Re-applying a mutagen refreshes and recalculates the bonus instead of stacking.
- **Effect Type:** Survivability boost
- **Prerequisites:** None

#### Alchemical Reflexes
- **Max Ranks:** 1
- **Description:** While a mutagen is active, gain +1 dodge bonus to AC and +1 bonus to Reflex saves.
- **Mechanics:** While under your mutagen, gain a +1 dodge bonus to AC (applies to touch, stacks with other dodge bonuses) and +1 untyped bonus to Reflex saves. The dodge bonus is lost while flat-footed, helpless, or otherwise unable to react; the save bonus is not.
- **Effect Type:** Defensive synergy
- **Prerequisites:** None

#### Natural Armor
- **Max Ranks:** 1
- **Description:** While a mutagen is active, your skin hardens and you gain +2 natural armor bonus to AC.
- **Mechanics:** While under your mutagen, gain a +2 natural armor bonus to AC. This does not stack with other natural armor bonuses (use the highest), but enhancement bonuses to natural armor still apply. Does not apply to touch AC.
- **Effect Type:** Defensive transformation
- **Prerequisites:** None

---

### TIER II – Cost: 2 points each

#### Mutagen II
- **Max Ranks:** 2
- **Description:** Additional +1 to ability score per rank when you drink a mutagen.
- **Effect Type:** Enhancement scaling
- **Prerequisites:** Mutagen I (2 ranks)

#### Persistence Mutagen
- **Max Ranks:** 1
- **Description:** Mutagens durations doubled.
- **Effect Type:** Duration extension and management
- **Prerequisites:** Mutagen I (1 rank)

#### Infused with Vigor
- **Max Ranks:** 1
- **Description:** When drinking your own mutagen, regain 1d6 + (1 per character level) hit points. Once per day, gain fast healing 1 for 10 rounds while a mutagen is active.
- **Effect Type:** Healing synergy
- **Prerequisites:** Hardy Constitution I (2 ranks)

#### Cellular Adaptation
- **Max Ranks:** 1
- **Description:** While a mutagen is active, gain 5/- damage reduction.
- **Effect Type:** Specialized resistances
- **Prerequisites:** Natural Armor

---

### TIER III – Cost: 3 points each

#### Improved Mutagen
- **Max Ranks:** 1
- **Description:** Mutagens now grant an additional +4 to the chosen ability score.
- **Effect Type:** Enhancement amplification
- **Prerequisites:** Mutagen II (1 rank), Persistence Mutagen

#### Unstable Mutagen
- **Max Ranks:** 1
- **Description:** When enabled, your mutagens become unstable and grant +50% more powerful effects, but now have a 10% chance per round in combat, while active to "backlash" and deal your level in damage to yourself. Uses the unstablemutagen command to toggle on and off.
- **Effect Type:** High-risk/high-reward buff
- **Prerequisites:** Mutagen II (1 rank), Cellular Adaptation

#### Universal Mutagen
- **Max Ranks:** 1
- **Description:** Uses the universal mutagen command. When used, the next mutagen used applies the highest bonus to all abiity scores, but only lasts a maximum of 5 minutes. However upon using this, you can't use mutagens at all for 30 minutes.
- **Effect Type:** Party buff capability
- **Prerequisites:** Improved Mutagen, Persistence Mutagen

#### Mutagenic Mastery
- **Max Ranks:** 1
- **Description:** Mutagens now offer an addiitional +2 to all ability scores.
- **Effect Type:** Utility and action economy
- **Prerequisites:** Mutagen II (any), Infused with Vigor

---

### TIER IV – CAPSTONE PERKS (2) – Cost: 5 points each, Max Rank: 1

#### Perfect Mutagen (Capstone)
- **Max Ranks:** 1
- **Description:** Your mutagens are perfected and now grant +4 to the chosen ability score and +2 to the other ability scores. You gain immunity to the backlash effect of Unstable Mutagen.
- **Effect Type:** Personal transformation capstone
- **Prerequisites:** Mutagenic Mastery, Improved Mutagen

#### Chimeric Transmutation (Capstone)
- **Max Ranks:** 1
- **Description:** While under the effect of mutagens, you can now use a breath weapon that deals 3d6 fire damage 3d6 poison damage and 3d6 cold damage, once per combat, as a swift action.
- **Effect Type:** Advanced transmutation capstone
- **Prerequisites:** Universal Mutagen, Unstable Mutagen

---

## TREE 2: BOMB CRAFTSMAN
*Harness the power of alchemical explosives. Mix, throw, and detonate devastating bombs in combat.*

### TIER I – Cost: 1 point each

#### Alchemical Bomb I
- **Max Ranks:** 3
- **Description:** Your bombs deal +3 damage per rank.
- **Effect Type:** Ranged explosive attack
- **Prerequisites:** None

#### Precise Bombs
- **Max Ranks:** 1
- **Description:** Bombs gain a +3 bonus to the ranged touch attack roll.
- **Prerequisites:** None

#### Splash Damage
- **Max Ranks:** 1
- **Description:** Bombs with splash effects now deal +3 extra splash damage with save dcs being +2 higher.
- **Effect Type:** Area effect enhancement
- **Prerequisites:** None

#### Quick Bomb
- **Max Ranks:** 1
- **Description:** 10% chance to throw a bomb as a swift action.
- **Effect Type:** Action economy
- **Prerequisites:** None

---

### TIER II – Cost: 2 points each

#### Alchemical Bomb II
- **Max Ranks:** 2
- **Description:** Additional +3 damage per rank.
- **Effect Type:** Damage scaling
- **Prerequisites:** Alchemical Bomb I (2 ranks)

#### Elemental Bomb
- **Max Ranks:** 1
- **Description:** Your bombs that deal elemental damage (fire, cold, acid, electric) byass 10 points of damage type reduction and also deal an extra 1d6 damage.
- **Effect Type:** Elemental specialization
- **Prerequisites:** Alchemical Bomb I (1 rank)

#### Concussive Bomb
- **Max Ranks:** 1
- **Description:** Your bombs have a 10% chance to attempt a knockdown against the target. This knockdown cannot be countered/reversed against the bomb thrower.
- **Effect Type:** Crowd control
- **Prerequisites:** Splash Damage

#### Poison Bomb
- **Max Ranks:** 1
- **Description:** Your bombs have a 10% chance to make the target poisoned (AFF_POISON)
- **Effect Type:** DoT and debuff
- **Prerequisites:** Alchemical Bomb I (1 rank)

---

### TIER III – Cost: 3 points each

#### Inferno Bomb
- **Max Ranks:** 1
- **Description:** Your bombs have a 10% chance to deal +2d6 fire damage.
- **Effect Type:** Advanced fire specialization
- **Prerequisites:** Elemental Bomb (fire), Alchemical Bomb II (1 rank)

#### Cluster Bomb
- **Max Ranks:** 1
- **Description:** Your bombs that deal damage have a 10% chance to become cluster bombs, which will have the bomb hit 3 times, but at 75% damage each.
- **Effect Type:** Multi-target enhancement
- **Prerequisites:** Alchemical Bomb II (1 rank), Precise Bombs

#### Calculated Throw
- **Max Ranks:** 1
- **Description:** Your bombs are extra precise, making DCs to resist their effects to be +3 higher.
- **Effect Type:** Precision and safety
- **Prerequisites:** Precise Bombs, Quick Bomb

#### Bomb Mastery
- **Max Ranks:** 1
- **Description:** Bombs now deal +2d6 extra damage.
- **Effect Type:** Damage and crowd control combination
- **Prerequisites:** Alchemical Bomb II (any), Concussive Bomb or Poison Bomb

---

### TIER IV – CAPSTONE PERKS (2) – Cost: 5 points each, Max Rank: 1

#### Bombardier Savant (Capstone)
- **Max Ranks:** 1
- **Description:** You are now an expert bomb thrower. Bombs gain +3 to ranged touch attacks and +6d6 damage. You can throw 2 bombs when starting combat with a bomb throw.
- **Effect Type:** Explosives mastery capstone
- **Prerequisites:** Bomb Mastery, Calculated Throw

#### Volatile Catalyst (Capstone)
- **Max Ranks:** 1
- **Description:** Your bombs now have a chance to trigger additional bombs you have prepared. Whenever you throw a bomb, there is a 1% per bomb you have prepared that, that bomb will be thrown as well. Ensure this is not recursive so that it won't infinite loop by making the extra bomb throws volatile as well. Create a volatilecatalyst comand that must be toggled on for this effect to take place.
- **Effect Type:** Chain reaction capstone
- **Prerequisites:** Inferno Bomb, Cluster Bomb

---

---

## TREE 3: EXTRACT MASTER
*Brew bottled infusions and harness the power of magic through alchemical extraction. Your extracts become extensions of your will.*

### TIER I – Cost: 1 point each

#### Alchemical Extract I
- **Max Ranks:** 3
- **Description:** Your extracts now have a 3% chance per rank to not expend a use.
- **Effect Type:** Spell bottling
- **Prerequisites:** None

#### Infusion I
- **Max Ranks:** 3
- **Description:** Your extract saving throw DCs and +1 higher per rank.
- **Effect Type:** Focused specialization
- **Prerequisites:** None

#### Swift Extraction
- **Max Ranks:** 1
- **Description:** Extracts take 20% less time to prepare.
- **Effect Type:** Crafting speed
- **Prerequisites:** None

#### Resonant Extract
- **Max Ranks:** 1
- **Description:** Extracts you create gain the "resonant" property. They have a 5% chance to affect all members in your party.
- **Effect Type:** Party synergy
- **Prerequisites:** None

---

### TIER II – Cost: 2 points each

#### Alchemical Extract II
- **Max Ranks:** 2
- **Description:** Additional 3% chance per rank for extract not use up a slot.
- **Effect Type:** Crafting scaling
- **Prerequisites:** Alchemical Extract I (2 ranks)

#### Infusion II
- **Max Ranks:** 2
- **Description:** Extract dcs another +1 higher per rank.
- **Effect Type:** Advanced specialization
- **Prerequisites:** Infusion I (2 ranks)

#### Concentrated Essence
- **Max Ranks:** 1
- **Description:** All extracts have a 20% chance to have the empowered metamagic effect applied.
- **Effect Type:** Power concentration
- **Prerequisites:** Swift Extraction

#### Persistent Extraction
- **Max Ranks:** 1
- **Description:** All extracts have a 20% chance to have the extended metamagic effect applied.
- **Effect Type:** Duration extension
- **Prerequisites:** Resonant Extract

---

### TIER III – Cost: 3 points each

#### Healing Extraction
- **Max Ranks:** 1
- **Description:** All extracts will heal the user (level/2) (min 1) hp when used.
- **Effect Type:** Advanced spell bottling
- **Prerequisites:** Alchemical Extract II (1 rank), Infusion II (1 rank)

#### Alchemical Compatibility
- **Max Ranks:** 1
- **Description:** All extracts when used will automatically be applied to any other party member with alchemnist levels in the same room.
- **Effect Type:** Combination effects
- **Prerequisites:** Concentrated Essence, Persistent Extraction

#### Discovery Extraction
- **Max Ranks:** 1
- **Description:** Extracts have a 10% chance to give a +1 INT bonus to the user. This effect is cumulative and lasts 2 minutes. Each time it applies, the int bonus goes up by one and the duration resets to 2 minutes. This will repeat to a maximum of 10 times.
- **Effect Type:** Unique effect unlocking
- **Prerequisites:** Greater Extraction, Infusion II (any)

#### Master Alchemist
- **Max Ranks:** 1
- **Description:** Your extracts and bombs have a 10% chance to have the maximize metamagic efect applied to them.
- **Effect Type:** Crafting excellence
- **Prerequisites:** Alchemical Compatibility, Discovery Extraction

---

### TIER IV – CAPSTONE PERKS (2) – Cost: 5 points each, Max Rank: 1

#### Eternal Extract (Capstone)
- **Max Ranks:** 1
- **Description:** Extracts have a 5% chance to last 1 hour (unless the duration would be higher than 1 hour anyway)
- **Effect Type:** Unlimited extraction capstone
- **Prerequisites:** Master Alchemist, Greater Extraction

#### Quintessential Extraction (Capstone)
- **Max Ranks:** 1
- **Description:** Anytime an extract is used the user heals 10 damage and increases max hp by 10. This lasts for 5 minutes, and stacks up to +100 max hp.
- **Effect Type:** Ultimate alchemy mastery capstone
- **Prerequisites:** Discovery Extraction, Alchemical Compatibility

---

## Implementation Notes

### Alchemist Perk System Design Philosophy (Pathfinder-Inspired)

This perk system draws from the Pathfinder Alchemist class, which represents the pinnacle of combining science, magic, and practical alchemy to produce devastating effects and powerful enhancements.

1. **Three Core Mechanical Pillars:**

   - **Mutagenist (Personal Enhancement):** Based on the *Mutagen* class feature from Pathfinder. Creates personal enhancement buffs granting ability score bonuses. Mechanically similar to the Pathfinder alchemist's core identity of self-transformation. Perks emphasize personal survivability, exotic mutagen combinations, and scaling enhancement.

   - **Bomb Craftsman (Combat Explosives):** Based on the *Bomb* class feature from Pathfinder. Creates ranged touch attack weapons that deal area damage with elemental specializations and crowd control effects. Mechanically similar to Pathfinder's devastating bomb mechanics with +1d6 per level scaling and elemental discoveries. Perks emphasize damage output, elemental specialization, chain reactions, and area control.

   - **Extract Master (Spell Bottling):** Based on the *Extract* class feature from Pathfinder. Creates prepared spell-like infusions that function as potions or stored magical effects. Mechanically similar to Pathfinder's prepared spell system integrated with alchemy. Perks emphasize versatility, spell level progression, and party support through distributed extracts.

2. **Progression Balance:**
   - Tier I (1 point): Core ability unlocks (Mutagen I, Bomb I, Extract I) with basic synergies
   - Tier II (2 points): Feature expansion (additional abilities, elemental choices, specializations)
   - Tier III (3 points): Advanced mastery (combination mechanics, multi-effect abilities)
   - Tier IV (5 points): Capstone perks representing "ultimate" mastery of each tree

   Each tier builds on prerequisites from earlier tiers, creating a natural progression path.

3. **Pathfinder Mechanic References:**
   - **Mutagen Duration:** 10-20 minutes base (from Pathfinder's "10 minutes per alchemist level"), extendable to hours
   - **Mutagen Ability Boost:** +2 per ability score per rank (matches Pathfinder's +2 to one ability)
   - **Bomb Base Damage:** 1d6 + (1d6 per rank) (from Pathfinder's "1d6 per alchemist level")
   - **Bomb Area:** 5-foot radius splash (from Pathfinder's "5-foot radius")
   - **Extracts:** Prepared daily, can contain spells of levels 0-4 (from Pathfinder's spell levels 0-6 as alchemists progress)
   - **Infusions:** Alchemical discoveries that modify bomb, mutagen, and extract effects

4. **Synergy with Existing Systems:**
   - Mutagen bonuses integrate with ability score systems (STR for damage, DEX for AC, CON for HP, etc.)
   - Bombs use ranged touch attack mechanics (similar to rogue sneak attack delivery)
   - Extracts integrate with existing potion/buff systems
   - Saves use existing DC formulas (10 + character level + modifiers)
   - Action economy integrates with swift/standard/full-round action system

5. **Scaling Mechanics:**
   - **Mutagens:** Scale off ranks in Mutagen I-II perks and character level
   - **Bombs:** Scale with ranks in Alchemical Bomb perks, character level, and elemental specializations
   - **Extracts:** Scale with ranks in Alchemical Extract perks and spell levels available
   - **Synergies:** Infusion perks enhance parent tree (Mutagen II enhances Mutagen I, etc.)

6. **Crossover Mechanics:**
   - Mutagen perks can enhance bomb throws (Alchemical Reflexes grants AC/saves)
   - Extract perks can be distributed to allies (Resonant Extract and Discovery effects)
   - All three trees benefit from "mastery" high-level perks that unlock capstone abilities
   - Party synergy builds naturally from allowing allies to consume mutagens/extracts

### Potential Future Expansions

- **Prestige Paths:** Combine two trees for hybrid abilities (Mutagen + Bombs = "Volatile Mutagen Bombs")
- **Discoveries:** Unlock special perk effects tied to creature types defeated (dragon bombs, undead extracts)
- **Seasonal Alchemy:** Limited-time event perks for temporary exclusive effects
- **Transmutation Puzzles:** Dungeon encounters requiring alchemical solutions
- **Craft Recipes:** Rare formula drops from bosses that unlock new perk combinations
- **PvP Mechanics:** Balanced versions of perks for player-versus-player combat

---

## Cost Summary

- **Tier I:** 1 point × 4 perks = 4 points per tree
- **Tier II:** 2 points × 4 perks = 8 points per tree
- **Tier III:** 3 points × 4 perks = 12 points per tree
- **Tier IV:** 5 points × 2 perks = 10 points per tree

**Total per tree:** 34 points  
**Total for all three trees:** 102 points

An Alchemist can fully spec into one tree at character creation/early levels, with room to add perks from other trees or pursue capstones as they level.
